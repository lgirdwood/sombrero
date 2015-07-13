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
 *  Copyright (C) 2015 Liam Girdwood
 */


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "config.h"
#include "sombrero.h"
#include "local.h"

/* function suffixes for SIMD ops */
#ifdef OPS_SSE42
	#define OPS(a) a ## _sse42
#elif OPS_AVX
	#define OPS(a) a ## _avx
#elif OPS_AVX2
	#define OPS(a) a ## _avx2
#elif OPS_FMA
	#define OPS(a) a ## _fma
#else
	#define OPS(a) a
#endif

static void uchar_to_float(struct smbrr_image *i, const unsigned char *c)
{
	int x, y, foffset, coffset;
	float *f = i->adu;

	for (x = 0; x < i->width; x++) {
		for (y = 0; y < i->height; y++) {
			coffset = y * i->stride + x;
			foffset = y * i->width + x;
			f[foffset] = (float)c[coffset];
		}
	}
}

static void ushort_to_float(struct smbrr_image *i, const unsigned short *c)
{
	int x, y, foffset, coffset;
	float *f = i->adu;

	for (x = 0; x < i->width; x++) {
		for (y = 0; y < i->height; y++) {
			coffset = y * i->stride + x;
			foffset = y * i->width + x;
			f[foffset] = (float)c[coffset];
		}
	}
}

static void uint_to_float(struct smbrr_image *i, const unsigned int *c)
{
	int x, y, foffset, coffset;
	float *f = i->adu;

	for (x = 0; x < i->width; x++) {
		for (y = 0; y < i->height; y++) {
			coffset = y * i->stride + x;
			foffset = y * i->width + x;
			f[foffset] = (float)c[coffset];
		}
	}
}

static void float_to_uchar(struct smbrr_image *i, unsigned char *c)
{
	int x, y, coffset, foffset;
	float *f = i->adu;

	for (x = 0; x < i->width; x++) {
		for (y = 0; y < i->height; y++) {
			coffset = y * i->stride + x;
			foffset = y * i->width + x;
			c[coffset] = (unsigned char)f[foffset];
		}
	}
}

static void uint_to_uint(struct smbrr_image *i, const unsigned int *c)
{
	int x, y, foffset, coffset;
	uint32_t *f = i->s;

	for (x = 0; x < i->width; x++) {
		for (y = 0; y < i->height; y++) {
			coffset = y * i->stride + x;
			foffset = y * i->width + x;
			f[foffset] = (uint32_t)c[coffset];
		}
	}
}

static void ushort_to_uint(struct smbrr_image *i, const unsigned short *c)
{
	int x, y, foffset, coffset;
	uint32_t *f = i->s;

	for (x = 0; x < i->width; x++) {
		for (y = 0; y < i->height; y++) {
			coffset = y * i->stride + x;
			foffset = y * i->width + x;
			f[foffset] = (uint32_t)c[coffset];
		}
	}
}

static void uchar_to_uint(struct smbrr_image *i, const unsigned char *c)
{
	int x, y, foffset, coffset;
	uint32_t *f = i->s;

	for (x = 0; x < i->width; x++) {
		for (y = 0; y < i->height; y++) {
			coffset = y * i->stride + x;
			foffset = y * i->width + x;
			f[foffset] = (uint32_t)c[coffset];
		}
	}
}

static void float_to_uint(struct smbrr_image *i, const float *c)
{
	int x, y, foffset, coffset;
	uint32_t *f = i->s;

	for (x = 0; x < i->width; x++) {
		for (y = 0; y < i->height; y++) {
			coffset = y * i->stride + x;
			foffset = y * i->width + x;
			f[foffset] = (uint32_t)c[coffset];
		}
	}
}

static void float_to_float(struct smbrr_image *i, const float *c)
{
	int x, y, foffset, coffset;
	float *f = i->adu;

	for (x = 0; x < i->width; x++) {
		for (y = 0; y < i->height; y++) {
			coffset = y * i->stride + x;
			foffset = y * i->width + x;
			f[foffset] = c[coffset];
		}
	}
}

static void uint_to_uchar(struct smbrr_image *i, unsigned char *c)
{
	int x, y, coffset, foffset;
	uint32_t *f = i->s;

	for (x = 0; x < i->width; x++) {
		for (y = 0; y < i->height; y++) {
			coffset = y * i->stride + x;
			foffset = y * i->width + x;
			c[coffset] = (unsigned char)f[foffset];
		}
	}
}

static int image_get(struct smbrr_image *image, enum smbrr_adu adu,
	void **buf)
{
	if (buf == NULL)
		return -EINVAL;

	switch (adu) {
	case SMBRR_ADU_8:

		switch (image->type) {
		case SMBRR_IMAGE_UINT32:
			uint_to_uchar(image, *buf);
			break;
		case SMBRR_IMAGE_FLOAT:
			float_to_uchar(image, *buf);
			break;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int image_convert(struct smbrr_image *image,
	enum smbrr_image_type type)
{
	int offset;

	if (type == image->type)
		return 0;

	switch (type) {
	case SMBRR_IMAGE_UINT32:
		switch (image->type) {
		case SMBRR_IMAGE_FLOAT:
			for (offset = 0; offset < image->size; offset++)
				image->s[offset] = (uint32_t)image->adu[offset];
			break;
		case SMBRR_IMAGE_UINT32:
			break;
		}
		break;
	case SMBRR_IMAGE_FLOAT:
		switch (image->type) {
		case SMBRR_IMAGE_UINT32:
			for (offset = 0; offset < image->size; offset++)
				image->adu[offset] = (float)image->s[offset];
			break;
		case SMBRR_IMAGE_FLOAT:
			break;
		}
		break;
	default:
		return -EINVAL;
	}

	image->type = type;
	return 0;
}

static void image_find_limits(struct smbrr_image *image, float *min, float *max)
{
	float *adu = image->adu, _min, _max;
	int offset;

	_min = 1.0e6;
	_max = -1.0e6;

	for (offset = 0; offset < image->size; offset++) {

		if (adu[offset] > _max)
			_max = adu[offset];

		if (adu[offset] < _min)
			_min = adu[offset];
	}

	*max = _max;
	*min = _min;
}

static void image_normalise(struct smbrr_image *image, float min, float max)
{
	float _min, _max, _range, range, factor;
	float *adu = image->adu;
	size_t offset;

	image_find_limits(image, &_min, &_max);

	_range = _max - _min;
	range = max - min;
	factor = range / _range;

	for (offset = 0; offset < image->size; offset++)
		adu[offset] = (adu[offset] - _min) * factor;
}

static void image_add(struct smbrr_image *a, struct smbrr_image *b,
	struct smbrr_image *c)
{
	float *A = a->adu;
	float *B = b->adu;
	float *C = c->adu;
	size_t offset;

	/* A = B + C */
	for (offset = 0; offset < a->size; offset++)
		A[offset] = B[offset] + C[offset];
}

static void image_add_sig(struct smbrr_image *a, struct smbrr_image *b,
	struct smbrr_image *c, struct smbrr_image *s)
{
	float *A = a->adu;
	float *B = b->adu;
	float *C = c->adu;
	uint32_t *S = s->s;
	int offset;

	/* iff S then A = B + C */
	for (offset = 0; offset < a->size; offset++) {
		if (S[offset])
			A[offset] = B[offset] + C[offset];
	}
}

static void image_fma(struct smbrr_image *dest, struct smbrr_image *a,
	struct smbrr_image *b, float c)
{
	float *A = a->adu;
	float *B = b->adu;
	float *D = dest->adu;
	size_t offset;

	/* dest = a + b * c */
	for (offset = 0; offset < dest->size; offset++)
		D[offset] = A[offset] + B[offset] * c;
}

static void image_fms(struct smbrr_image *dest, struct smbrr_image *a,
	struct smbrr_image *b, float c)
{
	float *A = a->adu;
	float *B = b->adu;
	float *D = dest->adu;
	size_t offset;

	/* dest = a - b * c */
	for (offset = 0; offset < dest->size; offset++)
		D[offset] = A[offset] - B[offset] * c;
}

static void image_subtract(struct smbrr_image *a, struct smbrr_image *b,
	struct smbrr_image *c)
{
	float *A = a->adu;
	float *B = b->adu;
	float *C = c->adu;
	size_t offset;

	/* A = B - C */
	for (offset = 0; offset < a->size; offset++)
		A[offset] = B[offset] - C[offset];
}

static void image_subtract_sig(struct smbrr_image *a, struct smbrr_image *b,
	struct smbrr_image *c, struct smbrr_image *s)
{
	float *A = a->adu, *B = b->adu, *C = c->adu;
	uint32_t *S = s->s;
	int offset;

	/* iff S then A = B - C */
	for (offset = 0; offset < a->size; offset++) {
		if (S[offset])
			A[offset] = B[offset] - C[offset];
	}
}

static void image_add_value(struct smbrr_image *image, float value)
{
	size_t offset;
	float *adu = image->adu;

	for (offset = 0; offset < image->size; offset++)
		adu[offset] += value;
}

static void image_add_value_sig(struct smbrr_image *image,
	struct smbrr_image *simage, float value)
{
	float *adu = image->adu;
	int offset;

	for (offset = 0; offset < image->size; offset++) {
		if (simage->s[offset])
			adu[offset] += value;
	}
}

static void image_subtract_value(struct smbrr_image *image, float value)
{
	size_t offset;
	float *adu = image->adu;

	for (offset = 0; offset < image->size; offset++)
		adu[offset] -= value;
}

static void image_mult_value(struct smbrr_image *image, float value)
{
	size_t offset;
	float *adu = image->adu;

	for (offset = 0; offset < image->size; offset++)
		adu[offset] *= value;
}

static void image_reset_value(struct smbrr_image *image, float value)
{
	float *adu = image->adu;
	int offset;

	for (offset = 0; offset < image->size; offset++)
		adu[offset] = value;
}

static void image_set_sig_value(struct smbrr_image *image, uint32_t value)
{
	uint32_t *adu = image->s;
	int offset;

	for (offset = 0; offset < image->size; offset++)
		adu[offset] = value;

	if (value == 0)
		image->sig_pixels = 0;
	else
		image->sig_pixels = image->size;
}

static void image_set_value_sig(struct smbrr_image *image,
	struct smbrr_image *simage, float sig_value)
{
	float *adu = image->adu;
	uint32_t *sig = simage->s;
	int offset;

	for (offset = 0; offset < image->size; offset++) {
		if (sig[offset])
			adu[offset] = sig_value;
	}
}

static void image_clear_negative(struct smbrr_image *image)
{
	float *i = image->adu;
	int offset;

	for (offset = 0; offset < image->size; offset++)
		if (i[offset] < 0.0)
			i[offset] = 0.0;
}

static float image_get_mean(struct smbrr_image *image)
{
	float mean = 0.0;
	int i;

	for (i = 0; i < image->size; i++)
		mean += image->adu[i];

	mean /= (float)image->size;
	return mean;
}

static float image_get_mean_sig(struct smbrr_image *image,
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

static float image_get_sigma(struct smbrr_image *image, float mean)
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

static float image_get_norm(struct smbrr_image *image)
{
	float norm = 0.0;
	int i;

	for (i = 0; i < image->size;  i++)
		norm += image->adu[i] * image->adu[i];

	return sqrtf(norm);
}

static float image_get_sigma_sig(struct smbrr_image *image,
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

static void image_anscombe(struct smbrr_image *image, float gain, float bias,
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

static void image_new_significance(struct smbrr_image *image,
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

static int image_psf(struct smbrr_image *src, struct smbrr_image *dest,
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
		xc = 5;
		yc = 5;
		break;
	default:
		return -EINVAL;
	}

	s = src->adu;
	d = dest->adu;

	/* clear each scale */
	image_reset_value(dest, 0.0);

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


const struct image_ops OPS(image_ops) = {
	.find_limits =image_find_limits,
	.get_mean = image_get_mean,
	.get_sigma = image_get_sigma,
	.get_mean_sig = image_get_mean_sig,
	.get_sigma_sig = image_get_sigma_sig,
	.get_norm = image_get_norm,
	.normalise = image_normalise,
	.add = image_add,
	.add_value_sig = image_add_value_sig,
	.add_sig = image_add_sig,
	.subtract = image_subtract,
	.subtract_sig = image_subtract_sig,
	.add_value = image_add_value,
	.subtract_value = image_subtract_value,
	.mult_value = image_mult_value,
	.reset_value = image_reset_value,
	.set_value_sig = image_set_value_sig,
	.convert = image_convert,
	.set_sig_value = image_set_sig_value,
	.clear_negative = image_clear_negative,
	.fma = image_fma,
	.fms = image_fms,
	.anscombe = image_anscombe,
	.new_significance = image_new_significance,
	.get = image_get,
	.psf = image_psf,

	.uchar_to_float = uchar_to_float,
	.ushort_to_float = ushort_to_float,
	.uint_to_float = uint_to_float,
	.float_to_uchar = float_to_uchar,
	.uint_to_uint = uint_to_uint,
	.ushort_to_uint = ushort_to_uint,
	.uchar_to_uint = uchar_to_uint,
	.float_to_uint = float_to_uint,
	.float_to_float = float_to_float,
	.uint_to_uchar = uint_to_uchar,
};
