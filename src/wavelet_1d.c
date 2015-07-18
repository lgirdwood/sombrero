/*
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  Copyright (C) 2012 Liam Girdwood
 */


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "sombrero.h"
#include "local.h"
#include "ops.h"
#include "mask.h"
#include "config.h"

static void set_conv_ops(struct smbrr_wavelet_1d *w)
{
	unsigned int cpu_flags = cpu_get_flags();

#if defined HAVE_FMA
	if (cpu_flags & CPU_X86_FMA) {
		w->ops = &conv1d_ops_fma;
		return;
	}
#endif
#if defined HAVE_AVX2
	if (cpu_flags & CPU_X86_AVX2) {
		w->ops = &conv1d_ops_avx2;
		return;
	}
#endif
#if defined HAVE_AVX
	if (cpu_flags & CPU_X86_AVX) {
		w->ops = &conv1d_ops_avx;
		return;
	}
#endif
#if defined HAVE_SSE42
	if (cpu_flags & CPU_X86_SSE4_2) {
		w->ops = &conv1d_ops_sse42;
		return;
	}
#endif

	/* default C implementation */
	w->ops = &conv1d_ops;
}

/*! \fn struct smbrr_wavelet_1d *smbrr_wavelet1d_new(struct smbrr_signal *signal,
	unsigned int num_scales)
* \param signal Image
* \param num_scales Number of wavelet scales.
* \return Wavelet pointer on success or NULL on failure..
*
* Create a wavelet with num_scales based on signal.
*/
struct smbrr_wavelet_1d *smbrr_wavelet1d_new(struct smbrr_signal *signal,
	unsigned int num_scales)
{
	struct smbrr_wavelet_1d *w = NULL;
	int i;

	if (num_scales > SMBRR_MAX_SCALES || num_scales == 0)
		return NULL;

	w = calloc(1, sizeof(*w));
	if (w == NULL)
		return NULL;

	w->length = signal->length;
	w->num_scales = num_scales;
	w->c[0] = signal;
	set_conv_ops(w);

	w->object_map = calloc(w->length, sizeof(struct object*));
	if (w->object_map == NULL)
		goto m_err;

	for (i = 1; i < num_scales; i++) {
		w->c[i] = smbrr_signal_new(SMBRR_DATA_FLOAT, w->length, 0, NULL);
		if (w->c[i] == NULL)
			goto c_err;
	}

	for (i = 0; i < num_scales - 1; i++) {
		w->w[i] = smbrr_signal_new(SMBRR_DATA_FLOAT, w->length, 0, NULL);
		if (w->w[i] == NULL)
			goto w_err;
	}

	for (i = 0; i < num_scales - 1; i++) {
		w->s[i] = smbrr_signal_new(SMBRR_DATA_UINT32, w->length, 0, NULL);
		if (w->s[i] == NULL)
			goto s_err;
	}

	return w;

s_err:
	for (--i; i >= 0; i--)
		smbrr_signal_free(w->s[i]);
	i = num_scales - 1;

w_err:
	for (--i; i >= 0; i--)
		smbrr_signal_free(w->w[i]);
	i = num_scales - 1;

c_err:
	for (--i; i >= 1; i--)
		smbrr_signal_free(w->c[i]);

m_err:
	free(w);
	return NULL;
}

struct smbrr_wavelet_1d *
	smbrr_wavelet1d_new_from_object(struct smbrr_object *object)
{
	struct smbrr_wavelet_1d *w = NULL;

	return w;
}

/*! \fn void smbrr_wavelet1d_free(struct smbrr_wavelet_1d *w)
* \param w Wavelet
*
* Free wavelet and it's signals.
*/
void smbrr_wavelet1d_free(struct smbrr_wavelet_1d *w)
{
	int i;

	if (w == NULL)
		return;

	//smbrr_wavelet1d_object_free_all(w);

	for (i = 0; i < w->num_scales - 1; i++)
		smbrr_signal_free(w->s[i]);

	for (i = 0; i < w->num_scales - 1; i++)
		smbrr_signal_free(w->w[i]);

	for (i = 1; i < w->num_scales; i++)
		smbrr_signal_free(w->c[i]);

	free(w->object_map);
	free(w);
}

/*! \fn void struct smbrr_signal *smbrr_wavelet1d_signal_get_scale(
	struct smbrr_wavelet_1d *w, unsigned int scale)
* \param w Wavelet
* \param scale Wavelet scale.
* \return Wavelet signal
*
* Returns wavelet scale signal at scale.
*/
struct smbrr_signal *smbrr_wavelet1d_signal_get_scale(
	struct smbrr_wavelet_1d *w, unsigned int scale)
{
	if (scale > w->num_scales - 1)
		return NULL;

	return w->c[scale];
}

/*! \fn struct smbrr_signal *smbrr_wavelet1d_signal_get_wavelet(
	struct smbrr_wavelet_1d *w, unsigned int scale)
* \param w Wavelet
* \param scale Wavelet scale.
* \return Wavelet signal
*
* Returns wavelet signal at scale.
*/
struct smbrr_signal *smbrr_wavelet1d_signal_get_wavelet(
	struct smbrr_wavelet_1d *w, unsigned int scale)
{
	if (scale > w->num_scales - 2)
		return NULL;

	return w->w[scale];
}

/*! \fn struct smbrr_signal *smbrr_wavelet1d_signal_get_significant(
	struct smbrr_wavelet_1d *w, unsigned int scale)
* \param w Wavelet
* \param scale Wavelet scale.
* \return Wavelet signal
*
* Returns wavelet significant signal at scale.
*/
struct smbrr_signal *smbrr_wavelet1d_signal_get_significant(
	struct smbrr_wavelet_1d *w, unsigned int scale)
{
	if (scale > w->num_scales - 2)
		return NULL;

	return w->s[scale];
}

/*! \fn void smbrr_wavelet1d_add(struct smbrr_wavelet_1d *a,
	 struct smbrr_wavelet_1d *b, struct smbrr_wavelet_1d *c)
* \param a Wavelet A
* \param b Wavelet B
* \param c Wavelet C
*
* Wavelet A = B + C.
*/
void smbrr_wavelet1d_add(struct smbrr_wavelet_1d *a,
	struct smbrr_wavelet_1d *b, struct smbrr_wavelet_1d *c)
{
	struct smbrr_signal *A, *B, *C;
	int i;

	for (i = 0; i < a->num_scales - 1; i++) {
		A = smbrr_wavelet1d_signal_get_wavelet(a, i);
		B = smbrr_wavelet1d_signal_get_wavelet(b, i);
		C = smbrr_wavelet1d_signal_get_wavelet(c, i);
		smbrr_signal_add(A, B, C);
	}
}

/*! \fn void smbrr_wavelet1d_subtract(struct smbrr_wavelet_1d *a,
	 struct smbrr_wavelet_1d *b, struct smbrr_wavelet_1d *c)
* \param a Wavelet A
* \param b Wavelet B
* \param c Wavelet C
*
* Wavelet A = B - C.
*/
void smbrr_wavelet1d_subtract(struct smbrr_wavelet_1d *a,
	struct smbrr_wavelet_1d *b, struct smbrr_wavelet_1d *c)
{
	struct smbrr_signal *A, *B, *C;
	int i;

	for (i = 0; i < a->num_scales - 1; i++) {
		A = smbrr_wavelet1d_signal_get_wavelet(a, i);
		B = smbrr_wavelet1d_signal_get_wavelet(b, i);
		C = smbrr_wavelet1d_signal_get_wavelet(c, i);
		smbrr_signal_subtract(A, B, C);
	}
}
