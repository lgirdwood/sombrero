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

#define M_1_16		(1.0 / 16.0)
#define M_1_8		(1.0 / 8.0)
#define M_1_4		(1.0 / 4.0)
#define M_1_256		(1.0 / 256.0)
#define M_1_64		(1.0 / 64.0)
#define M_3_128		(3.0 / 128.0)
#define M_3_32		(3.0 / 32.0)
#define M_9_64		(9.0 / 64.0)

#define IM_1_16		(16.0)
#define IM_1_8		(8.0)
#define IM_1_4		(4.0)
#define IM_1_256	(256.0)
#define IM_1_64		(64.0)
#define IM_3_128	(1.0 / (3.0 / 128.0))
#define IM_3_32		(1.0 / (3.0 / 32.0))
#define IM_9_64		(1.0 / (9.0 / 64.0))


/* linear interpolation mask */
static const float linear_mask[3][3] = {
	{M_1_16, M_1_8, M_1_16},
	{M_1_8, M_1_4, M_1_8},
	{M_1_16, M_1_8, M_1_16},
};

static const float linear_mask_inverse[3][3] = {
	{IM_1_16, IM_1_8, IM_1_16},
	{IM_1_8, IM_1_4, IM_1_8},
	{IM_1_16, IM_1_8, IM_1_16},
};

/* bicubic spline */
static const float bicubic_mask[5][5] = {
	{M_1_256, M_1_64, M_3_128, M_1_64, M_1_256},
	{M_1_64, M_1_16, M_3_32, M_1_16, M_1_64},
	{M_3_128, M_3_32, M_9_64, M_3_32, M_3_128},
	{M_1_64, M_1_16, M_3_32, M_1_16, M_1_64},
	{M_1_256, M_1_64, M_3_128, M_1_64, M_1_256},
};

static const float bicubic_mask_inverse[5][5] = {
	{IM_1_256, IM_1_64, IM_3_128, IM_1_64, IM_1_256},
	{IM_1_64, IM_1_16, IM_3_32, IM_1_16, IM_1_64},
	{IM_3_128, IM_3_32, IM_9_64, IM_3_32, IM_3_128},
	{IM_1_64, IM_1_16, IM_3_32, IM_1_16, IM_1_64},
	{IM_1_256, IM_1_64, IM_3_128, IM_1_64, IM_1_256},
};

/* K amplification for each wavelet scale */
static const float k_amp[5][8] = {
	{1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},	/* none */
	{1.0, 1.0, 1.0, 1.0, 1.0, 2.0, 4.0, 8.0},	/* low pass */
	{1.0, 1.0, 2.0, 4.0, 2.0, 1.0, 1.0, 1.0},	/* mid pass */
	{4.0, 2.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},	/* high pass */
	{1.0, 1.0, 1.5, 2.0, 4.0, 4.0, 4.0, 8.0},	/* low-mid pass */
};

static inline int y_boundary(unsigned int height, int offy)
{
	/* handle boundaries by mirror */
	if (offy < 0)
		offy = 0 - offy;
	else if (offy >= height)
		offy = height - (offy - height) - 1;

	return offy;
}

static inline int x_boundary(unsigned int width, int offx)
{
	/* handle boundaries by mirror */
	if (offx < 0)
		offx = 0 - offx;
	else if (offx >= width)
		offx = width - (offx - width) - 1;

	return offx;
}

static int conv_mask_set(struct smbrr_wavelet *w, enum smbrr_wavelet_mask mask)
{
	switch (mask) {
	case SMBRR_WAVELET_MASK_LINEAR:
		w->mask.data = (float*)linear_mask;
		w->mask.width = 3;
		w->mask.height = 3;
		w->mask_type = mask;
		break;
	case SMBRR_WAVELET_MASK_BICUBIC:
		w->mask.data = (float*)bicubic_mask;
		w->mask.width = 5;
		w->mask.height = 5;
		w->mask_type = mask;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int deconv_mask_set(struct smbrr_wavelet *w, enum smbrr_wavelet_mask mask)
{
	switch (mask) {
	case SMBRR_WAVELET_MASK_LINEAR:
		w->mask.data = (float*)linear_mask_inverse;
		w->mask.width = 3;
		w->mask.height = 3;
		w->mask_type = mask;
		break;
	case SMBRR_WAVELET_MASK_BICUBIC:
		w->mask.data = (float*)bicubic_mask_inverse;
		w->mask.width = 5;
		w->mask.height = 5;
		w->mask_type = mask;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/*! \fn int smbrr_image_psf(struct smbrr_image *src, struct smbrr_image *dest,
	enum smbrr_wavelet_mask mask)
* \param src Source image
* \param dest Destination Image
* \param mask PSF convolution mask
* \return 0 for success.
*
* Perform Point Spread Function on source image and store in destination image
* using wavelet masks to blur image.
*/
int smbrr_image_psf(struct smbrr_image *src, struct smbrr_image *dest,
	enum smbrr_wavelet_mask mask)
{
	const float *data;
	int height, width;
	int x, y, xc, yc;
	int offy, offx, pixel, offxy, maskxy;
	float *s, *d;

	switch (mask) {
	case SMBRR_WAVELET_MASK_LINEAR:
		data = (float*)linear_mask_inverse;
		xc = 3;
		yc = 3;
		break;
	case SMBRR_WAVELET_MASK_BICUBIC:
		data = (float*)bicubic_mask_inverse;
		yc = 5;
		yc = 5;
		break;
	default:
		return -EINVAL;
	}

	s = src->adu;
	d = dest->adu;

	/* clear each scale */
	smbrr_image_reset_value(dest, 0.0);

	/* image height loop */
	for (height = 0; height < src->height; height++) {

		/* image width loop */
		for (width = 0; width < src->width; width++) {

			pixel = height * src->width + width;

			/* mask y loop */
			for (y = 0; y < yc; y++) {
				offy = y_boundary(height, height + (y - yc));

				/* mask x loop */
				for (x = 0; x < xc; x++) {
					offx = x_boundary(width, width + (x - xc));
					offxy = image_get_offset(src, offx, offy);
					maskxy = mask_get_offset(xc, x, y);
					d[pixel] += s[offxy] * data[maskxy];
				}
			}
		}
	}

	return 0;
}

/* create Wi and Ci from C0 */
static void atrous_conv(struct smbrr_wavelet *wavelet)
{
	int scale, scale2, height;
	float *c, *_c;

	/* scale loop */
	for (scale = 1; scale < wavelet->num_scales; scale++) {

		/* clear each scale */
		smbrr_image_reset_value(wavelet->c[scale], 0.0);

		c = wavelet->c[scale]->adu;
		_c = wavelet->c[scale - 1]->adu;
		scale2 = 1 << (scale - 1);

		/* image height loop */
#pragma omp parallel for \
		firstprivate(c, _c, scale, scale2, wavelet) \
		schedule(static, 50)

		for (height = 0; height < wavelet->height; height++) {

			int offy, offx, pixel, offxy, maskxy;
			int width, x, y, xc, yc;

			xc = wavelet->mask.width >> 1;
			yc = wavelet->mask.height >> 1;

			/* image width loop */
			for (width = 0; width < wavelet->width; width++) {

				pixel = height * wavelet->width + width;

				/* mask y loop */
				for (y = 0; y < wavelet->mask.height; y++) {

					offy = y_boundary(wavelet->height,
						height + ((y - yc) * scale2));

					/* mask x loop */
					for (x = 0; x < wavelet->mask.width; x++) {

						offx = x_boundary(wavelet->width,
							width + ((x - xc) * scale2));

						offxy = image_get_offset(wavelet->c[scale], offx, offy);
						maskxy = mask_get_offset(wavelet->mask.width, x, y);

						c[pixel] += _c[offxy] * wavelet->mask.data[maskxy];
					}
				}
			}
		}
	}

	/* create wavelet */
#pragma omp parallel for schedule(static, 1)
	for (scale = 1; scale < wavelet->num_scales; scale++)
		smbrr_image_subtract(wavelet->w[scale - 1], wavelet->c[scale - 1],
				wavelet->c[scale]);

}

/* create Wi and Ci from C0 if S */
static void atrous_conv_sig(struct smbrr_wavelet *wavelet)
{
	int scale, height, scale2;
	float *c, *_c;

	/* scale loop */
	for (scale = 1; scale < wavelet->num_scales; scale++) {

		/* clear each scale */
		smbrr_image_reset_value(wavelet->c[scale], 0.0);

		/* dont run loop if there are no sig pixels at this scale */
		if (wavelet->s[scale - 1]->sig_pixels == 0)
			continue;

		c = wavelet->c[scale]->adu;
		_c = wavelet->c[scale - 1]->adu;
		scale2 = 1 << (scale - 1);

		/* image height loop */
#pragma omp parallel for \
		firstprivate(c, _c, scale, scale2, wavelet) \
		schedule(static, 50)

		for (height = 0; height < wavelet->height; height++) {

			int offy, offx, pixel, offxy, maskxy;
			int width, x, y, xc, yc;

			xc = wavelet->mask.width >> 1;
			yc = wavelet->mask.height >> 1;

			/* image width loop */
			for (width = 0; width < wavelet->width; width++) {

				pixel = height * wavelet->width + width;

				/* mask y loop */
				for (y = 0; y < wavelet->mask.height; y++) {

					offy = y_boundary(wavelet->height,
						height + ((y - yc) * scale2));

					/* mask x loop */
					for (x = 0; x < wavelet->mask.width; x++) {

						offx = x_boundary(wavelet->width,
							width + ((x - xc) * scale2));

						offxy = image_get_offset(wavelet->c[scale], offx, offy);

						/* only apply wavelet if sig */
						if (!wavelet->s[scale - 1]->s[offxy])
							continue;

						maskxy = mask_get_offset(wavelet->mask.width, x, y);

						c[pixel] += _c[offxy] * wavelet->mask.data[maskxy];
					}
				}
			}
		}
	}
	/* create wavelet */
#pragma omp parallel for schedule(static, 1)
	for (scale = 1; scale < wavelet->num_scales; scale++)
		smbrr_image_subtract(wavelet->w[scale - 1], wavelet->c[scale - 1],
				wavelet->c[scale]);

}

/* C0 = C(scale - 1) + sum of wavelets; */
static void atrous_deconv(struct smbrr_wavelet *wavelet)
{
	int scale;

	/* set initial starting image as C[scales - 1] */
	smbrr_image_copy(wavelet->c[0], wavelet->c[wavelet->num_scales - 1]);

	/* add each wavelet scale */
	for (scale = wavelet->num_scales - 2; scale > 0; scale--)
		smbrr_image_add(wavelet->c[0], wavelet->c[0], wavelet->w[scale]);
}

/* C0 = C(scale - 1) + sum of wavelets if W(pixel) is significant; */
static void atrous_deconv_sig(struct smbrr_wavelet *wavelet,
	enum smbrr_gain gain)
{
	int scale;

	/* set initial starting image as C[scales - 1] */
	smbrr_image_copy(wavelet->c[0], wavelet->c[wavelet->num_scales - 1]);

	/* add each wavelet scale */
	for (scale = wavelet->num_scales - 2; scale > 0; scale--) {

		/* dont run loop if there are no sig pixels at this scale */
		if (wavelet->s[scale]->sig_pixels == 0)
			continue;

		if (gain != SMBRR_GAIN_NONE)
			smbrr_image_mult_value(wavelet->w[scale], k_amp[gain][scale]);
		smbrr_image_add_sig(wavelet->c[0], wavelet->c[0],
			wavelet->w[scale], wavelet->s[scale]);
	}
}

/* C0 = C(scale - 1) + sum of wavelets; */
static void atrous_deconv_object(struct smbrr_wavelet *w,
	struct smbrr_object *object)
{
	struct structure *s;
	struct object *o = (struct object *)object;
	struct smbrr_image *image = o->image, *simage, *wimage;
	int scale, x, y, id, ix, iy, pixel, ipixel, start, end;

	ix = object->minXy.x;
	iy = object->minxY.y;
	start = o->start_scale;
	end = o->end_scale;

	for (scale = end; scale >= start; scale --) {
		s = w->structure[object->scale] +
			o->structure[object->scale];
		id = s->id + 2;
		simage = w->s[scale];
		wimage = w->w[scale];

		for (x = s->minXy.x; x <= s->maxXy.x; x++) {
			for (y = s->minxY.y; y <= s->maxxY.y; y++) {

				pixel = simage->width * y + x;

				if (simage->s[pixel] == id) {
					ipixel = image->width * (y - iy) + (x - ix);
					if(ipixel > image->size)
						printf("ipxel %d size %d width %d x %d y %d ix %d iy %d\n",
								ipixel, image->size, image->width, x,y, ix, iy);
					image->adu[ipixel] += wimage->adu[pixel];
				}
			}
		}
	}
}

/*! \fn int smbrr_wavelet_convolution(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask)
* \param w wavelet
* \param conv wavelet convolution type
* \param mask wavelet convolution mask
* \return 0 for success.
*
* Convolve image into wavelets using all pixels.
*/
int smbrr_wavelet_convolution(struct smbrr_wavelet *w, enum smbrr_conv conv,
	enum smbrr_wavelet_mask mask)
{
	int ret;

	ret = conv_mask_set(w, mask);
	if (ret < 0)
		return ret;

	switch (conv) {
	case SMBRR_CONV_ATROUS:
		w->conv_type = conv;
		atrous_conv(w);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/*! \fn int smbrr_wavelet_convolution_sig(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask)
* \param w wavelet
* \param conv wavelet convolution type
* \param mask wavelet convolution mask
* \return 0 for success.
*
* Convolve image into wavelets using significant pixels only.
*/
int smbrr_wavelet_convolution_sig(struct smbrr_wavelet *w, enum smbrr_conv conv,
	enum smbrr_wavelet_mask mask)
{
	int ret;

	ret = conv_mask_set(w, mask);
	if (ret < 0)
		return ret;

	switch (conv) {
	case SMBRR_CONV_ATROUS:
		w->conv_type = conv;
		atrous_conv_sig(w);
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
* De-convolve wavelet scales into image using all pixels.
*/
int smbrr_wavelet_deconvolution(struct smbrr_wavelet *w, enum smbrr_conv conv,
	enum smbrr_wavelet_mask mask)
{
	int ret;

	ret = deconv_mask_set(w, mask);
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

/*! \fn int smbrr_wavelet_deconvolution_sig(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask)
* \param w wavelet
* \param conv wavelet convolution type
* \param mask wavelet convolution mask
* \return 0 for success.
*
* De-convolve wavelet scales into image using significant pixels only.
*/
int smbrr_wavelet_deconvolution_sig(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask, enum smbrr_gain gain)
{
	int ret;

	ret = deconv_mask_set(w, mask);
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
* De-convolve wavelet scales into object image using significant pixels only.
*/
int smbrr_wavelet_deconvolution_object(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask,
	struct smbrr_object *object)
{
	int ret;

	ret = deconv_mask_set(w, mask);
	if (ret < 0)
		return ret;

	switch (conv) {
	case SMBRR_CONV_ATROUS:
		w->conv_type = conv;
		atrous_deconv_object(w, object);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
