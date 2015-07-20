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
	struct smbrr *sdata;

	/* perform deconvolution on sig coefficients */
	smbrr_wavelet_new_significant(w, sigma_clip);

	/* clear sig pixels at scale 0 to lessen artifacts on restored data */
	sdata = smbrr_wavelet_get_significant(w, 0);
	if (sdata->sig_pixels)
		smbrr_significant_set_sig_value(sdata, 0);

	smbrr_wavelet_significant_deconvolution(w, SMBRR_CONV_ATROUS, mask,
		SMBRR_GAIN_NONE);
	return smbrr_wavelet_get_scale(w, 0);
}

/* deconvolve data where wavelet are significant at all scales */
static struct smbrr *data_get_A(struct smbrr *data,
	enum smbrr_wavelet_mask mask, int scales, enum smbrr_clip sigma_clip)
{
	struct smbrr_wavelet *w;
	struct smbrr *i;

	w = smbrr_wavelet_new(data, scales);
	if (w == NULL)
		return NULL;

	/* convolve each scale */
	smbrr_wavelet_convolution(w, SMBRR_CONV_ATROUS, mask);

	/* convolve each scale using sig vales from previous convolve at each scale */
	smbrr_wavelet_new_significant(w, sigma_clip);
	smbrr_wavelet_significant_convolution(w, SMBRR_CONV_ATROUS, mask);
	smbrr_wavelet_significant_deconvolution(w, SMBRR_CONV_ATROUS, mask,
		SMBRR_GAIN_NONE);
	i = smbrr_wavelet_get_scale(w, 0);

	smbrr_wavelet_free(w);
	return i;
}

/* alpha = sqr(norm (atilda(wr))) / sqr(norm(A(R)) */
static float calc_alpha(struct smbrr *R, struct smbrr_wavelet *w,
	enum smbrr_wavelet_mask mask, int scales, enum smbrr_clip sigma_clip)
{
	struct smbrr *Aw, *Ar;
	float nAw, nAr;

	Aw = data_get_A_tilda(w, mask, sigma_clip);

	/* sqr of norm of data A~w */
	nAw = smbrr_get_norm(Aw);
	nAw *= nAw;

	/* sqr of norm of data Ar */
	Ar = data_get_A(R, mask, scales, sigma_clip);
	nAr = smbrr_get_norm(Ar);
	nAr *= nAr;

	return nAw / nAr;
}

/* beta = sqr(norm(atilda(wr(n+1)))) / sqr(norm(atilda(wr(n))) */
static float calc_beta(struct smbrr_wavelet *w, struct smbrr_wavelet *w1,
	enum smbrr_wavelet_mask mask, enum smbrr_clip sigma_clip)
{
	struct smbrr *Aw, *Aw1;
	float nAw, nAw1;

	Aw = data_get_A_tilda(w, mask, sigma_clip);
	Aw1 = data_get_A_tilda(w1, mask, sigma_clip);

	/* sqr of norm of data A~w */
	nAw = smbrr_get_norm(Aw);
	nAw *= nAw;

	/* sqr of norm of data A~w */
	nAw1 = smbrr_get_norm(Aw1);
	nAw1 *= nAw1;

	return nAw1 / nAw;
}

static float calc_residual_thres(struct smbrr_wavelet *w)
{
	float res = 0.0;
	int i;

	for (i = 0; i < w->num_scales - 1; i++)
		res += smbrr_get_norm(w->w[i]);
	res /= ((w->num_scales - 1) * w->c[0]->elems);

	printf("residual thresh = %e\n", res);

	return res;
}

int smbrr_reconstruct(struct smbrr *O,
	enum smbrr_wavelet_mask mask, float threshold, int scales,
	enum smbrr_clip sigma_clip)
{
	struct smbrr *R = NULL, *O0 = NULL, *O1 = NULL, *tmpI;
	struct smbrr_wavelet *Wi = NULL, *wr0 = NULL, *wr1 = NULL, *tmpW;
	float alpha, beta;
	int tries = 2, ret = -ENOMEM;

	/* diff data O0 */
	O0 = smbrr_new(O->type, O->width, O->height, 0, 0, NULL);
	if (O0 == NULL)
		goto out;
	smbrr_copy(O0, O);

	/* diff data O1 */
	O1 = smbrr_new(O->type, O->width, O->height, 0, 0, NULL);
	if (O1 == NULL)
		goto out;
	smbrr_copy(O1, O);

	/* O wavelet */
	Wi = smbrr_wavelet_new(O, scales);
	if (Wi == NULL)
		goto out;
	//smbrr_data_dump(O, "O");

	/* Residual wavelet 0 */
	wr0 = smbrr_wavelet_new(O0, scales);
	if (wr0 == NULL)
		goto out;

	/* Residual wavelet 1 */
	wr1 = smbrr_wavelet_new(O1, scales);
	if (wr1 == NULL)
		goto out;

	/* Residual Image R */
	R = smbrr_new(O->type, O->width, O->height, 0, 0, NULL);
	if (R == NULL)
		goto out;

	/* get residual wavelet wr0 */

	/* Wi */
	smbrr_wavelet_convolution(Wi, SMBRR_CONV_ATROUS, mask);

	/* A(O) */
	smbrr_wavelet_convolution(wr0, SMBRR_CONV_ATROUS, mask);

	smbrr_wavelet_new_significant(wr0, sigma_clip);

	smbrr_wavelet_significant_convolution(wr0, SMBRR_CONV_ATROUS, mask);
	//smbrr_wavelet_dump_scale(wr0, 0, "Wr0-C0");
	/* wr0 */

	smbrr_wavelet_subtract(wr0, Wi, wr0);

	/* get initial residual data R */
	smbrr_copy(R, data_get_A_tilda(wr0, mask, sigma_clip));
	//smbrr_data_dump(R, "R");

	do {

		/* step 2 - calculate convergance parameter alpha */
		alpha = calc_alpha(R, wr0, mask, scales, sigma_clip);
		//printf("alpha %f\n", alpha);

		/* step 3 - calculate new data O - add correction to O */
		/* O(n+1) = O(n) + alpha * R after which O is O(n+1) */
		smbrr_mult_add(O0, O0, R, alpha);
		//smbrr_data_dump(O0, "O0-%d", tries);

		/* step 4 - Set all pixels in O(n+1) < 0 to = 0 */
		smbrr_zero_negative(O0);

		/* step 5 - calculate new residual wavelet using new data O */
		/* wr(n+1) = Wi - A(O(n+1)) */

		/* A(O(n+1)) - recalulate convolution using O(n+1) */
		smbrr_wavelet_convolution(wr0, SMBRR_CONV_ATROUS, mask);

		smbrr_wavelet_new_significant(wr0, sigma_clip);
		smbrr_wavelet_significant_convolution(wr0, SMBRR_CONV_ATROUS, mask);

		/* wr1 = Wi - A(O(n+1)) where wr0 is A(O(n+1)) */
		smbrr_wavelet_subtract(wr1, Wi, wr0);

		/* step 6 - if (norm(w(n+1)) < threshold) then stop  */
		if (calc_residual_thres(wr1) < threshold)
			break;

		//smbrr_data_dump(O0, "O0-%d", tries);

		/* step 7 - calculate new convergence parameter beta */
		beta = calc_beta(wr0, wr1, mask, sigma_clip);
		//printf("beta %f\n", beta);

		/* step 8 - calculate new residual data R */
		/* R(n+1) = atilda(wr(n+1)) + beta * R(n) */

		O1 = data_get_A_tilda(wr1, mask, sigma_clip);
		smbrr_mult_add(R, O1, R, beta);
		//smbrr_data_dump(O1, "O1-%d", tries);

		/* swap wavelet */
		tmpW = wr0;
		tmpI = O0;
		wr0 = wr1;
		O0 = O1;
		wr1 = tmpW;
		O1 = tmpI;

	} while (tries--);

	smbrr_copy(O, O0);
	ret = 0;
out:

	smbrr_wavelet_free(wr1);
	smbrr_wavelet_free(wr0);
	smbrr_wavelet_free(Wi);
	smbrr_free(R);
	smbrr_free(O0);
	smbrr_free(O1);

	return ret;
}

