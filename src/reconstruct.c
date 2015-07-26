/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (C) 2013 Liam Girdwood <lgirdwood@gmail.com>
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "sombrero.h"
#include "local.h"

/* A tilda = deconvolve data where wavelet is significant except at scale 0 */
static struct smbrr *data_get_A_tilda(struct smbrr_wavelet *w,
	enum smbrr_wavelet_mask mask, enum smbrr_clip sigma_clip)
{
	smbrr_wavelet_deconvolution(w, SMBRR_CONV_ATROUS, mask);
	return smbrr_wavelet_get_scale(w, 0);
}

/* deconvolve data where wavelet are significant at all scales */
static float data_get_A(struct smbrr *data,
	enum smbrr_wavelet_mask mask, int scales, enum smbrr_clip sigma_clip)
{
	struct smbrr_wavelet *w;
	struct smbrr *i;
	float norm;

	w = smbrr_wavelet_new(data, scales);
	if (w == NULL)
		return 0;

	/* convolve each scale */
	smbrr_wavelet_convolution(w, SMBRR_CONV_ATROUS, mask);

	/* convolve each scale using sig vales from previous convolve at each scale */
	smbrr_wavelet_new_significant(w, sigma_clip);

	smbrr_wavelet_significant_convolution(w, SMBRR_CONV_ATROUS, mask);
	smbrr_wavelet_significant_deconvolution(w, SMBRR_CONV_ATROUS, mask,
		SMBRR_GAIN_NONE);
	i = smbrr_wavelet_get_scale(w, 0);

	norm = smbrr_get_norm(i);
	smbrr_wavelet_free(w);
	return norm;
}

static float calc_residual_thres(struct smbrr_wavelet *w)
{
	float res = 0.0;
	int i;

	for (i = 0; i < w->num_scales - 1; i++)
		res += smbrr_get_norm(w->w[i]);
	res /= ((w->num_scales - 1) * w->c[0]->elems);

	return res;
}

/* w1= w0 - A(O) */
static float calc_residual_wavelet(struct smbrr_wavelet *w0,
	struct smbrr_wavelet *w1, enum smbrr_clip sigma_clip,
	enum smbrr_wavelet_mask mask)
{
	/* A0 */
	smbrr_wavelet_convolution(w0, SMBRR_CONV_ATROUS, mask);
	smbrr_wavelet_new_significant(w0, sigma_clip);

	/* w1 = w1 - w0 */
	smbrr_wavelet_significant_subtract(w1, w1, w0);

	return calc_residual_thres(w1);
}

/* alpha = sqr(norm (atilda(wr))) / sqr(norm(A(R)) */
static float calc_alphaN(struct smbrr *R, struct smbrr_wavelet *w,
	enum smbrr_wavelet_mask mask, int scales, enum smbrr_clip sigma_clip)
{
	float nAw, nAr;

	/* sqr of norm of data A~w */
	nAw = smbrr_get_norm(R);
	nAw *= nAw;

	/* sqr of norm of data Ar */
	nAr = data_get_A(R, mask, scales, sigma_clip);
	nAr *= nAr;

	return nAw / nAr;
}

int smbrr_reconstruct(struct smbrr *O,
	enum smbrr_wavelet_mask mask, float threshold, int scales,
	enum smbrr_clip sigma_clip)
{
	struct smbrr *R , *O0, *O1;
	struct smbrr_wavelet *wr0, *wr1;
	float alpha, thresh, thresh_delta, thresh_old = 1.0e6;
	int ret = -ENOMEM, tries = 10;

	/* diff data O0 */
	O0 = smbrr_new(O->type, O->width, O->height, 0, 0, NULL);
	if (O0 == NULL)
		return -ENOMEM;
	smbrr_copy(O0, O);

	/* diff data O1 */
	O1 = smbrr_new(O->type, O->width, O->height, 0, 0, NULL);
	if (O1 == NULL)
		goto err_O1;
	smbrr_copy(O1, O);

	/* Residual wavelet 0 */
	wr0 = smbrr_wavelet_new(O0, scales);
	if (wr0 == NULL)
		goto err_wr0;

	/* Residual wavelet 1 */
	wr1 = smbrr_wavelet_new(O1, scales);
	if (wr1 == NULL)
		goto err_wr1;

	while (tries--) {
		/* wr 1 is residual */
		thresh = calc_residual_wavelet(wr0, wr1, sigma_clip, mask);

		/* have we converged */
		thresh_delta = fabs(thresh - thresh_old);
		if (thresh_delta < threshold)
			break;
		thresh_old = thresh;

		/* get residual image from wr 1*/
		R = data_get_A_tilda(wr1, mask, sigma_clip);

		alpha = calc_alphaN(R, wr1, mask, scales, sigma_clip);

		/* step 3 - calculate new data O - add correction to O */
		/* O(n+1) = O(n) + alpha * R after which O is O(n+1) */
		smbrr_mult_add(O1, O0, R, alpha);

		/* step 4 - Set all pixels in O(n+1) < 0 to = 0 */
		smbrr_zero_negative(O1);

		/* step 5 - wr 1 is residual, O1 is image */
		smbrr_wavelet_set_elems(wr0, O1);
	}

	smbrr_copy(O1, O);
	ret = 0;

	smbrr_wavelet_free(wr1);
err_wr1:
	smbrr_wavelet_free(wr0);
err_wr0:
	smbrr_free(O1);
err_O1:
	smbrr_free(O0);
	return ret;
}
