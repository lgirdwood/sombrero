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

static void atrous_deconv(struct smbrr_wavelet *wavelet)
{
	int scale;

	/* set initial starting data as C[scales - 1] */
	smbrr_copy(wavelet->c[0], wavelet->c[wavelet->num_scales - 1]);

	/* add each wavelet scale */
	for (scale = wavelet->num_scales - 2; scale > 0; scale--)
		smbrr_add(wavelet->c[0], wavelet->c[0], wavelet->w[scale]);
}

/* C0 = C(scale - 1) + sum of wavelets if W(pixel) is significant; */
static void atrous_deconv_sig(struct smbrr_wavelet *wavelet,
	enum smbrr_gain gain)
{
	int scale;

	/* set initial starting data as significant C[scales - 1] */
	smbrr_significant_copy(wavelet->c[0], wavelet->c[wavelet->num_scales - 1],
		wavelet->s[wavelet->num_scales - 1]);

	/* add each wavelet scale */
	for (scale = wavelet->num_scales - 2; scale > 0; scale--) {

		/* dont run loop if there are no sig pixels at this scale */
		if (wavelet->s[scale]->sig_pixels == 0)
			continue;

		if (gain != SMBRR_GAIN_NONE)
			smbrr_mult_value(wavelet->w[scale], k_amp[gain][scale]);
		smbrr_significant_add(wavelet->c[0], wavelet->c[0],
			wavelet->w[scale], wavelet->s[scale]);
	}
}

/*! \fn int smbrr_wavelet_convolution(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask)
* \param w wavelet
* \param conv wavelet convolution type
* \param mask wavelet convolution mask
* \return 0 for success.
*
* Convolve data into wavelets using all pixels.
*/
int smbrr_wavelet_convolution(struct smbrr_wavelet *w, enum smbrr_conv conv,
	enum smbrr_wavelet_mask mask)
{
	int ret;

	ret = conv_mask_set_2d(w, mask);
	if (ret < 0)
		return ret;

	switch (conv) {
	case SMBRR_CONV_ATROUS:
		w->conv_type = conv;
		w->ops->atrous_conv(w);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/*! \fn int smbrr_wavelet_significant_convolution(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask)
* \param w wavelet
* \param conv wavelet convolution type
* \param mask wavelet convolution mask
* \return 0 for success.
*
* Convolve data into wavelets using significant pixels only.
*/
int smbrr_wavelet_significant_convolution(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask)
{
	int ret;

	ret = conv_mask_set_2d(w, mask);
	if (ret < 0)
		return ret;

	switch (conv) {
	case SMBRR_CONV_ATROUS:
		w->conv_type = conv;
		w->ops->atrous_conv_sig(w);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/*! \fn int smbrr_wavelet_deconvolution(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask)
* \param w wavelet
* \param conv wavelet convolution type
* \param mask wavelet convolution mask
* \return 0 for success.
*
* De-convolve wavelet scales into data using all pixels.
*/
int smbrr_wavelet_deconvolution(struct smbrr_wavelet *w, enum smbrr_conv conv,
	enum smbrr_wavelet_mask mask)
{
	int ret;

	ret = deconv_mask_set_2d(w, mask);
	if (ret < 0)
		return ret;

	switch (conv) {
	case SMBRR_CONV_ATROUS:
		w->conv_type = conv;
		atrous_deconv(w);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/*! \fn int smbrr_wavelet_significant_deconvolution(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask, enum smbrr_gain gain)
* \param w wavelet
* \param conv wavelet convolution type
* \param mask wavelet convolution mask
* \param gain wavelte convolution gain
* \return 0 for success.
*
* De-convolve wavelet scales into data using significant pixels only.
*/
int smbrr_wavelet_significant_deconvolution(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask, enum smbrr_gain gain)
{
	int ret;

	ret = deconv_mask_set_2d(w, mask);
	if (ret < 0)
		return ret;

	switch (conv) {
	case SMBRR_CONV_ATROUS:
		w->conv_type = conv;
		atrous_deconv_sig(w, gain);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/*! \fn int smbrr_wavelet_deconvolution_object(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask,
	struct smbrr_object *object)
* \param w wavelet
* \param conv wavelet convolution type
* \param mask wavelet convolution mask
* \param object Object to deconvolve
* \return 0 for success.
*
* De-convolve wavelet scales into object data using significant pixels only.
*/
int smbrr_wavelet_deconvolution_object(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask,
	struct smbrr_object *object)
{
	int ret;

	ret = deconv_mask_set_2d(w, mask);
	if (ret < 0)
		return ret;

	switch (conv) {
	case SMBRR_CONV_ATROUS:
		w->conv_type = conv;
		w->ops->atrous_deconv_object(w, object);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
