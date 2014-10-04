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

/*! \fn struct smbrr_wavelet *smbrr_wavelet_new(struct smbrr_image *image,
	unsigned int num_scales)
* \param image Image
* \param num_scales Number of wavelet scales.
* \return Wavelet pointer on success or NULL on failure..
*
* Create a wavelet with num_scales based on image.
*/
struct smbrr_wavelet *smbrr_wavelet_new(struct smbrr_image *image,
	unsigned int num_scales)
{
	struct smbrr_wavelet *w = NULL;
	int i;

	if (num_scales > SMBRR_MAX_SCALES || num_scales == 0)
		return NULL;

	w = calloc(1, sizeof(*w));
	if (w == NULL)
		return NULL;

	w->height = image->height;
	w->width = image->width;
	w->num_scales = num_scales;
	w->c[0] = image;

	w->object_map = calloc(w->height * w->width, sizeof(struct object*));
	if (w->object_map == NULL)
		goto m_err;

	for (i = 1; i < num_scales; i++) {
		w->c[i] = smbrr_image_new(SMBRR_IMAGE_FLOAT, w->width,
			w->height, image->stride, 0, NULL);
		if (w->c[i] == NULL)
			goto c_err;
	}

	for (i = 0; i < num_scales - 1; i++) {
		w->w[i] = smbrr_image_new(SMBRR_IMAGE_FLOAT, w->width,
			w->height, image->stride, 0, NULL);
		if (w->w[i] == NULL)
			goto w_err;
	}

	for (i = 0; i < num_scales - 1; i++) {
		w->s[i] = smbrr_image_new(SMBRR_IMAGE_UINT32, w->width,
			w->height, image->stride, 0, NULL);
		if (w->s[i] == NULL)
			goto s_err;
	}

	return w;

s_err:
	for (--i; i >= 0; i--)
		smbrr_image_free(w->s[i]);
	i = num_scales - 1;

w_err:
	for (--i; i >= 0; i--)
		smbrr_image_free(w->w[i]);
	i = num_scales - 1;

c_err:
	for (--i; i >= 1; i--)
		smbrr_image_free(w->c[i]);

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
* Free wavelet and it's images.
*/
void smbrr_wavelet_free(struct smbrr_wavelet *w)
{
	int i;

	if (w == NULL)
		return;

	smbrr_wavelet_object_free_all(w);

	for (i = 0; i < w->num_scales - 1; i++)
		smbrr_image_free(w->s[i]);

	for (i = 0; i < w->num_scales - 1; i++)
		smbrr_image_free(w->w[i]);

	for (i = 1; i < w->num_scales; i++)
		smbrr_image_free(w->c[i]);

	free(w->object_map);
	free(w);
}

/*! \fn void struct smbrr_image *smbrr_wavelet_image_get_scale(
	struct smbrr_wavelet *w, unsigned int scale)
* \param w Wavelet
* \param scale Wavelet scale.
* \return Wavelet image
*
* Returns wavelet scale image at scale.
*/
struct smbrr_image *smbrr_wavelet_image_get_scale(struct smbrr_wavelet *w,
	unsigned int scale)
{
	if (scale > w->num_scales - 1)
		return NULL;

	return w->c[scale];
}

/*! \fn struct smbrr_image *smbrr_wavelet_image_get_wavelet(
	struct smbrr_wavelet *w, unsigned int scale)
* \param w Wavelet
* \param scale Wavelet scale.
* \return Wavelet image
*
* Returns wavelet image at scale.
*/
struct smbrr_image *smbrr_wavelet_image_get_wavelet(struct smbrr_wavelet *w,
	unsigned int scale)
{
	if (scale > w->num_scales - 2)
		return NULL;

	return w->w[scale];
}

/*! \fn struct smbrr_image *smbrr_wavelet_image_get_significant(
	struct smbrr_wavelet *w, unsigned int scale)
* \param w Wavelet
* \param scale Wavelet scale.
* \return Wavelet image
*
* Returns wavelet significant image at scale.
*/
struct smbrr_image *smbrr_wavelet_image_get_significant(struct smbrr_wavelet *w,
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
	struct smbrr_image *A, *B, *C;
	int i;

	for (i = 0; i < a->num_scales - 1; i++) {
		A = smbrr_wavelet_image_get_wavelet(a, i);
		B = smbrr_wavelet_image_get_wavelet(b, i);
		C = smbrr_wavelet_image_get_wavelet(c, i);
		smbrr_image_add(A, B, C);
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
	struct smbrr_image *A, *B, *C;
	int i;

	for (i = 0; i < a->num_scales - 1; i++) {
		A = smbrr_wavelet_image_get_wavelet(a, i);
		B = smbrr_wavelet_image_get_wavelet(b, i);
		C = smbrr_wavelet_image_get_wavelet(c, i);
		smbrr_image_subtract(A, B, C);
	}
}
