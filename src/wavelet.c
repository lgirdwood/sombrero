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

static const struct convolution_ops* get_1d_conv_ops(void)
{
	unsigned int cpu_flags = cpu_get_flags();

#if defined HAVE_FMA
	if (cpu_flags & CPU_X86_FMA)
		return &conv_ops_1d_fma;
#endif
#if defined HAVE_AVX2
	if (cpu_flags & CPU_X86_AVX2)
		return &conv_ops_1d_avx2;
#endif
#if defined HAVE_AVX
	if (cpu_flags & CPU_X86_AVX)
		return &conv_ops_1d_avx;
#endif
#if defined HAVE_SSE42
	if (cpu_flags & CPU_X86_SSE4_2)
		return &conv_ops_1d_sse42;
#endif

	/* default C implementation */
	return &conv_ops_1d;
}

static const struct convolution_ops* get_2d_conv_ops(void)
{
	unsigned int cpu_flags = cpu_get_flags();

#if defined HAVE_FMA
	if (cpu_flags & CPU_X86_FMA)
		return &conv_ops_2d_fma;
#endif
#if defined HAVE_AVX2
	if (cpu_flags & CPU_X86_AVX2)
		return &conv_ops_2d_avx2;
#endif
#if defined HAVE_AVX
	if (cpu_flags & CPU_X86_AVX)
		return &conv_ops_2d_avx;
#endif
#if defined HAVE_SSE42
	if (cpu_flags & CPU_X86_SSE4_2)
		return &conv_ops_2d_sse42;
#endif

	/* default C implementation */
	return &conv_ops_2d;
}

/*! \fn struct smbrr_wavelet *smbrr_wavelet_new(struct smbrr *data,
	unsigned int num_scales)
* \param data Image
* \param num_scales Number of wavelet scales.
* \return Wavelet pointer on success or NULL on failure..
*
* Create a wavelet with num_scales based on data.
*/
struct smbrr_wavelet *smbrr_wavelet_new(struct smbrr *src,
	unsigned int num_scales)
{
	struct smbrr_wavelet *w = NULL;
	const struct convolution_ops* ops;
	int wtype, stype;
	int i;

	if (num_scales > SMBRR_MAX_SCALES || num_scales == 0)
		return NULL;

	switch (src->type) {
	case SMBRR_DATA_1D_UINT32:
	case SMBRR_DATA_2D_UINT32:
		return NULL;
	case SMBRR_DATA_1D_FLOAT:
		wtype = src->type;
		stype = SMBRR_DATA_1D_UINT32;
		ops = get_1d_conv_ops();
		break;
	case SMBRR_DATA_2D_FLOAT:
		wtype = src->type;
		stype = SMBRR_DATA_2D_UINT32;
		ops = get_2d_conv_ops();
		break;
	default:
		return NULL;
	}

	w = calloc(1, sizeof(*w));
	if (w == NULL)
		return NULL;

	w->height = src->height;
	w->width = src->width;
	w->num_scales = num_scales;
	w->c[0] = src;
	w->ops = ops;

	w->object_map = calloc(w->height * w->width, sizeof(struct object*));
	if (w->object_map == NULL)
		goto m_err;

	for (i = 1; i < num_scales; i++) {
		w->c[i] = smbrr_new(wtype, w->width, w->height, src->stride, 0, NULL);
		if (w->c[i] == NULL)
			goto c_err;
	}

	for (i = 0; i < num_scales - 1; i++) {
		w->w[i] = smbrr_new(wtype, w->width, w->height, src->stride, 0, NULL);
		if (w->w[i] == NULL)
			goto w_err;
	}

	for (i = 0; i < num_scales - 1; i++) {
		w->s[i] = smbrr_new(stype, w->width, w->height, src->stride, 0, NULL);
		if (w->s[i] == NULL)
			goto s_err;
	}

	return w;

s_err:
	for (--i; i >= 0; i--)
		smbrr_free(w->s[i]);
	i = num_scales - 1;

w_err:
	for (--i; i >= 0; i--)
		smbrr_free(w->w[i]);
	i = num_scales - 1;

c_err:
	for (--i; i >= 1; i--)
		smbrr_free(w->c[i]);

m_err:
	free(w);
	return NULL;
}

struct smbrr_wavelet *smbrr_wavelet_new_from_object(struct smbrr_object *object)
{
	struct smbrr_wavelet *w = NULL;

	return w;
}

/*! \fn void smbrr_wavelet_free(struct smbrr_wavelet *w)
* \param w Wavelet
*
* Free wavelet and it's datas.
*/
void smbrr_wavelet_free(struct smbrr_wavelet *w)
{
	int i;

	if (w == NULL)
		return;

	smbrr_wavelet_object_free_all(w);

	for (i = 0; i < w->num_scales - 1; i++)
		smbrr_free(w->s[i]);

	for (i = 0; i < w->num_scales - 1; i++)
		smbrr_free(w->w[i]);

	for (i = 1; i < w->num_scales; i++)
		smbrr_free(w->c[i]);

	free(w->object_map);
	free(w);
}

/*! \fn void struct smbrr *smbrr_wavelet_data_get_scale(
	struct smbrr_wavelet *w, unsigned int scale)
* \param w Wavelet
* \param scale Wavelet scale.
* \return Wavelet data
*
* Returns wavelet scale data at scale.
*/
struct smbrr *smbrr_wavelet_get_data_scale(struct smbrr_wavelet *w,
	unsigned int scale)
{
	if (scale > w->num_scales - 1)
		return NULL;

	return w->c[scale];
}

/*! \fn struct smbrr *smbrr_wavelet_data_get_wavelet(
	struct smbrr_wavelet *w, unsigned int scale)
* \param w Wavelet
* \param scale Wavelet scale.
* \return Wavelet data
*
* Returns wavelet data at scale.
*/
struct smbrr *smbrr_wavelet_get_data_wavelet(struct smbrr_wavelet *w,
	unsigned int scale)
{
	if (scale > w->num_scales - 2)
		return NULL;

	return w->w[scale];
}

/*! \fn struct smbrr *smbrr_wavelet_data_get_significant(
	struct smbrr_wavelet *w, unsigned int scale)
* \param w Wavelet
* \param scale Wavelet scale.
* \return Wavelet data
*
* Returns wavelet significant data at scale.
*/
struct smbrr *smbrr_wavelet_get_data_significant(struct smbrr_wavelet *w,
	unsigned int scale)
{
	if (scale > w->num_scales - 2)
		return NULL;

	return w->s[scale];
}

/*! \fn void smbrr_wavelet_add(struct smbrr_wavelet *a,
	 struct smbrr_wavelet *b, struct smbrr_wavelet *c)
* \param a Wavelet A
* \param b Wavelet B
* \param c Wavelet C
*
* Wavelet A = B + C.
*/
void smbrr_wavelet_add(struct smbrr_wavelet *a, struct smbrr_wavelet *b,
	struct smbrr_wavelet *c)
{
	struct smbrr *A, *B, *C;
	int i;

	for (i = 0; i < a->num_scales - 1; i++) {
		A = smbrr_wavelet_get_data_wavelet(a, i);
		B = smbrr_wavelet_get_data_wavelet(b, i);
		C = smbrr_wavelet_get_data_wavelet(c, i);
		smbrr_add(A, B, C);
	}
}

/*! \fn void smbrr_wavelet_subtract(struct smbrr_wavelet *a,
	 struct smbrr_wavelet *b, struct smbrr_wavelet *c)
* \param a Wavelet A
* \param b Wavelet B
* \param c Wavelet C
*
* Wavelet A = B - C.
*/
void smbrr_wavelet_subtract(struct smbrr_wavelet *a, struct smbrr_wavelet *b,
	struct smbrr_wavelet *c)
{
	struct smbrr *A, *B, *C;
	int i;

	for (i = 0; i < a->num_scales - 1; i++) {
		A = smbrr_wavelet_get_data_wavelet(a, i);
		B = smbrr_wavelet_get_data_wavelet(b, i);
		C = smbrr_wavelet_get_data_wavelet(c, i);
		smbrr_subtract(A, B, C);
	}
}
