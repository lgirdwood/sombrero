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

/*! \fn float smbrr_image_get_mean(struct smbrr_image *image)
* \param image Image
* \return Image mean
*
* Calculate mean value of all image pixels.
*/
float smbrr_image_get_mean(struct smbrr_image *image)
{
	float mean = 0.0;
	int i;

	for (i = 0; i < image->size; i++)
		mean += image->adu[i];

	mean /= (float)image->size;
	return mean;
}

/*! \fn float smbrr_image_get_mean_sig(struct smbrr_image *image)
* \param image Image
* \param simage Significant image
* \return mean value of significant pixels.
*
* Calculate the mean value of significant image pixels. i.e mean value
* of all image(pixel) where simage(pixel) > 0.
*/
float smbrr_image_get_mean_sig(struct smbrr_image *image,
	struct smbrr_image *simage)
{
	float mean_sig = 0.0;
	int i, ssize = 0;

	if (image->height != simage->height ||
		image->width != simage->width)
		return 0.0;

	for (i = 0; i < image->size; i++) {

		if (!simage->s[i])
			continue;

		mean_sig += image->adu[i];
		ssize++;
	}

	mean_sig /= (float)ssize;
	return mean_sig;
}

/*! \fn float smbrr_image_get_sigma(struct smbrr_image *image, float mean)
* \param image Image
* \param mean
* \return Standard deviation of all image pixels.
*
* Calculate standard deviation (sigma) of image pixels.
*/
float smbrr_image_get_sigma(struct smbrr_image *image, float mean)
{
	float t, sigma = 0.0;
	int i;

	for (i = 0; i < image->size;  i++) {
		t = image->adu[i] - mean;
		t *= t;
		sigma += t;
	}

	sigma /= (float) image->size;
	sigma = sqrtf(sigma);
	return sigma;
}

/*! \fn float smbrr_image_get_norm(struct smbrr_image *image)
* \param image Image
* \return Image norm
*
* Calculate norm of image pixels. i.e sqrt(sum of pixels)
*/
float smbrr_image_get_norm(struct smbrr_image *image)
{
	float norm = 0.0;
	int i;

	for (i = 0; i < image->size;  i++)
		norm += image->adu[i];

	return sqrtf(fabsf(norm));
}

/*! \fn float smbrr_image_get_sigma_sig(struct smbrr_image *image,
	struct smbrr_image *simage, float mean_sig)
* \param image Image
* \param simage Significant image
* \param mean_sig Mean value of significant pixels
* \return Standard deviation of significant pixels.
*
* Calculate standard deviation (sigma) of significant image pixels. i.e
* standard deviation of all image(pixel) where simage(pixel) > 0.
*/
float smbrr_image_get_sigma_sig(struct smbrr_image *image,
	struct smbrr_image *simage, float mean_sig)
{
	float t, sigma_sig = 0.0;
	int i, ssize = 0;

	for (i = 0; i < image->size;  i++) {

		if (!simage->s[i])
			continue;

		t = image->adu[i] - mean_sig;
		t *= t;
		sigma_sig += t;
		ssize++;
	}

	sigma_sig /= (float) ssize;
	sigma_sig = sqrtf(sigma_sig);
	return sigma_sig;
}

/*! \fn void smbrr_image_anscombe(struct smbrr_image *image, float gain,
	float bias, float readout)
* \param image Image
* \param gain CCD amplifier gain in photo-electrons per ADU
* \param bias Image bias in ADUs
* \param readout Readout noise in RMS electrons.
*
* Convert an Poisson noise in an image to an image with a constant Guassian
* deviation suitable for wavelet convolutions.
*/
void smbrr_image_anscombe(struct smbrr_image *image, float gain, float bias,
	float readout)
{
	float hgain, cgain, r;
	int i;

	/* HAIP Equ 18.9 */
	hgain = gain / 2.0;
	cgain = (gain * gain) * 0.375;
	r = readout * readout;
	r += cgain;

	for (i = 0; i < image->size;  i++)
		 image->adu[i] = hgain * sqrtf(gain * (image->adu[i] - bias) + r);
}

/*! \fn void smbrr_image_new_significance(struct smbrr_image *image,
	struct smbrr_image *simage, float sigma)
* \param image Image
* \param simage Significant image
* \param sigma sigma
*
* Create new significance image from image for pixels above sigma.
*/
void smbrr_image_new_significance(struct smbrr_image *image,
	struct smbrr_image *simage, float sigma)
{
	int i;

	if (image->height != simage->height ||
		image->width != simage->width)
		return;

	/* clear the old significance data */
	bzero(simage->s, sizeof(uint32_t) * simage->size);
	simage->sig_pixels = 0;

	for (i = 0; i < image->size; i++) {
		if (image->adu[i] >= sigma) {
			simage->s[i] = 1;
			simage->sig_pixels++;
		}
	}
}

#define D1(x)	(1.0 / x)
#define M1(x)	(1.0 * x)

/* K sigma clip for each wavelet scale */
static const struct smbrr_clip_coeff k_sigma[6] = {
	/* very gentle */
	{.coeff = {M1(2.0), M1(1.0), D1(2.0), D1(4.0), D1(8.0), D1(16.0),
		D1(32.0), D1(64.0), D1(128.0), D1(256.0), D1(512.0)},},
	/* gentle */
	{.coeff = {M1(3.0), M1(2.0), M1(1.0), D1(2.0), D1(4.0), D1(8.0),
		D1(16.0), D1(32.0), D1(64.0), D1(128.0), D1(256.0)},},
	/* normal */
	{.coeff = {M1(4.0), M1(3.0), M1(2.0), M1(1.0), D1(2.0), D1(4.0),
		D1(8.0), D1(16.0), D1(32.0), D1(64.0), D1(128.0)},},
	/* strong */
	{.coeff = {M1(5.0), M1(4.0), M1(3.0), M1(2.0), M1(1.0), D1(2.0),
		D1(4.0), D1(8.0), D1(16.0), D1(32.0), D1(64.0)},},
	/* very strong */
	{.coeff = {M1(6.0), M1(5.0), M1(4.0), M1(3.0), M1(2.0), M1(1.0),
		D1(2.0), D1(4.0), D1(8.0), D1(16.0), D1(32.0)},},
	/* very very strong */
	{.coeff = {M1(7.0), M1(6.0), M1(5.0), M1(4.0), M1(3.0), M1(2.0),
		M1(1.0), D1(2.0), D1(4.0), D1(8.0), D1(16.0)},},
};

static void clip_scale(struct smbrr_wavelet *w, int scale,
		const struct smbrr_clip_coeff *c, float sig_delta)
{
	struct smbrr_image *image, *simage;
	float mean, sigma;
	float mean_sig, sigma_sig, sigma_sig_old = 0.0;

	image = smbrr_wavelet_image_get_wavelet(w, scale);
	simage = smbrr_wavelet_image_get_significant(w, scale);

	mean = smbrr_image_get_mean(image);
	sigma = smbrr_image_get_sigma(image, mean);

	sigma_sig = sigma;

	/* clip scale until sigma is less than delta */
	do {
		sigma_sig_old = sigma_sig;

		/* create new sig pixel image for scale */
		smbrr_image_new_significance(image, simage,
			c->coeff[scale] * sigma_sig_old);

		/* calculate new mean and sigma for sig pixels */
		mean_sig = smbrr_image_get_mean_sig(image, simage);
		smbrr_image_get_sigma_sig(image, simage, mean_sig);

		/* keep going if sigma difference is above delta */
	} while (fabsf(sigma_sig - sigma_sig_old) > sig_delta);
}

/*! \fn int smbrr_wavelet_ksigma_clip(struct smbrr_wavelet *w,
	enum smbrr_clip clip, float sig_delta)
* \param w wavelet
* \param clip clipping strength
* \param sig_delta clipping sigma delta
*
* Clip each wavelet scale based on strength until sigma for each scale is less
* than sig_delta.
*/
int smbrr_wavelet_ksigma_clip(struct smbrr_wavelet *w, enum smbrr_clip clip,
	float sig_delta)
{
	const struct smbrr_clip_coeff *coeff;
	int i;

	if (clip < SMBRR_CLIP_VGENTLE || clip > SMBRR_CLIP_VVSTRONG)
		return -EINVAL;

	coeff = &k_sigma[clip];
#pragma omp parallel for firstprivate(w, clip, sig_delta, coeff) schedule(dynamic, 1)
	for (i = 0; i < w->num_scales - 1; i++)
		 clip_scale(w, i, coeff, sig_delta);

	return 0;
}

/*! \fn int smbrr_wavelet_ksigma_clip(struct smbrr_wavelet *w,
	enum smbrr_clip clip, float sig_delta)
* \param w wavelet
* \param clip clipping strength
* \param sig_delta clipping sigma delta
*
* Clip each wavelet scale based on strength until sigma for each scale is less
* than sig_delta.
*/
int smbrr_wavelet_ksigma_clip_custom(struct smbrr_wavelet *w,
	struct smbrr_clip_coeff *coeff, float sig_delta)
{
	int i;

	for (i = 0; i < w->num_scales - 1; i++)
		 clip_scale(w, i, coeff, sig_delta);

	return 0;
}

/*! \fn int smbrr_wavelet_new_significance(struct smbrr_wavelet *w,
	enum smbrr_clip sigma_clip)
* \param w wavelet
* \param sigma_clip clipping strength
*
* Create new significance images at each scale for wavelet using clip strength
* sigma_clip.
*/
int smbrr_wavelet_new_significance(struct smbrr_wavelet *w,
	enum smbrr_clip sigma_clip)
{
	struct smbrr_image *W, *S;
	const struct smbrr_clip_coeff *c;
	float mean, sigma;
	int i;

	if (sigma_clip < SMBRR_CLIP_VGENTLE || sigma_clip > SMBRR_CLIP_VVSTRONG)
		return -EINVAL;

	c = &k_sigma[sigma_clip];

	for (i = 0; i < w->num_scales - 1; i++) {
		W = smbrr_wavelet_image_get_wavelet(w, i);
		S = smbrr_wavelet_image_get_significant(w, i);

		mean = smbrr_image_get_mean(W);
		sigma = smbrr_image_get_sigma(W, mean);

		smbrr_image_new_significance(W, S,
			c->coeff[i] * sigma);
	}

	return 0;
}
