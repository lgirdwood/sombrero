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

#include "sombrero.h"
#include "local.h"
#include "ops.h"
#include "mask.h"
#include "config.h"

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

static void uchar_to_float_1d(struct smbrr *i, const unsigned char *c)
{
	int x;
	float *f = i->adu;

	for (x = 0; x < i->width; x++)
			f[x] = (float)c[x];
}

static void ushort_to_float_1d(struct smbrr *i, const unsigned short *c)
{
	int x;
	float *f = i->adu;

	for (x = 0; x < i->width; x++)
			f[x] = (float)c[x];
}

static void uint_to_float_1d(struct smbrr *i, const unsigned int *c)
{
	int x;
	float *f = i->adu;

	for (x = 0; x < i->width; x++)
			f[x] = (float)c[x];
}

static void float_to_uchar_1d(struct smbrr *i, unsigned char *c)
{
	int x;
	float *f = i->adu;

	for (x = 0; x < i->width; x++)
			c[x] = (unsigned char)f[x];
}

static void float_to_ushort_1d(struct smbrr *i, unsigned short *c)
{
	int x;
	float *f = i->adu;

	for (x = 0; x < i->width; x++)
			c[x] = (unsigned short)f[x];
}

static void uint_to_uint_1d(struct smbrr *i, const unsigned int *c)
{
	int x;
	uint32_t *f = i->s;

	for (x = 0; x < i->width; x++)
			f[x] = (uint32_t)c[x];
}

static void ushort_to_uint_1d(struct smbrr *i, const unsigned short *c)
{
	int x;
	uint32_t *f = i->s;

	for (x = 0; x < i->width; x++)
			f[x] = (uint32_t)c[x];
}

static void uchar_to_uint_1d(struct smbrr *i, const unsigned char *c)
{
	int x;
	uint32_t *f = i->s;

	for (x = 0; x < i->width; x++)
			f[x] = (uint32_t)c[x];
}

static void float_to_uint_1d(struct smbrr *i, const float *c)
{
	int x;
	uint32_t *f = i->s;

	for (x = 0; x < i->width; x++)
			f[x] = (uint32_t)c[x];
}

static void float_to_float_1d(struct smbrr *i, const float *c)
{
	float *f = i->adu;

	memcpy(f, c, i->width * sizeof(float));
}

static void uint_to_uchar_1d(struct smbrr *i, unsigned char *c)
{
	int x;
	uint32_t *f = i->s;

	for (x = 0; x < i->width; x++)
			c[x] = (unsigned char)f[x];
}

static void uint_to_ushort_1d(struct smbrr *i, unsigned short *c)
{
	int x;
	uint32_t *f = i->s;

	for (x = 0; x < i->width; x++)
			c[x] = (unsigned short)f[x];
}

static void uchar_to_float_2d(struct smbrr *i, const unsigned char *c)
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

static void ushort_to_float_2d(struct smbrr *i, const unsigned short *c)
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

static void uint_to_float_2d(struct smbrr *i, const unsigned int *c)
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

static void float_to_uchar_2d(struct smbrr *i, unsigned char *c)
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

static void float_to_ushort_2d(struct smbrr *i, unsigned short *c)
{
	int x, y, coffset, foffset;
	float *f = i->adu;

	for (x = 0; x < i->width; x++) {
		for (y = 0; y < i->height; y++) {
			coffset = y * i->stride + x;
			foffset = y * i->width + x;
			c[coffset] = (unsigned short)f[foffset];
		}
	}
}

static void uint_to_uint_2d(struct smbrr *i, const unsigned int *c)
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

static void ushort_to_uint_2d(struct smbrr *i, const unsigned short *c)
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

static void uchar_to_uint_2d(struct smbrr *i, const unsigned char *c)
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

static void float_to_uint_2d(struct smbrr *i, const float *c)
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

static void float_to_float_2d(struct smbrr *i, const float *c)
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

static void uint_to_uchar_2d(struct smbrr *i, unsigned char *c)
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

static void uint_to_ushort_2d(struct smbrr *i, unsigned short *c)
{
	int x, y, coffset, foffset;
	uint32_t *f = i->s;

	for (x = 0; x < i->width; x++) {
		for (y = 0; y < i->height; y++) {
			coffset = y * i->stride + x;
			foffset = y * i->width + x;
			c[coffset] = (unsigned short)f[foffset];
		}
	}
}

static int get(struct smbrr *data, enum smbrr_source_type adu, void **buf)
{
	if (buf == NULL)
		return -EINVAL;

	switch (adu) {
	case SMBRR_SOURCE_UINT8:

		switch (data->type) {
		case SMBRR_DATA_1D_UINT32:
			uint_to_uchar_1d(data, *buf);
			break;
		case SMBRR_DATA_2D_UINT32:
			uint_to_uchar_2d(data, *buf);
			break;
		case SMBRR_DATA_1D_FLOAT:
			float_to_uchar_1d(data, *buf);
			break;
		case SMBRR_DATA_2D_FLOAT:
			float_to_uchar_2d(data, *buf);
			break;
		}
		break;

	case SMBRR_SOURCE_UINT16:

		switch (data->type) {
		case SMBRR_DATA_1D_UINT32:
			uint_to_ushort_1d(data, *buf);
			break;
		case SMBRR_DATA_2D_UINT32:
			uint_to_ushort_2d(data, *buf);
			break;
		case SMBRR_DATA_1D_FLOAT:
			float_to_ushort_1d(data, *buf);
			break;
		case SMBRR_DATA_2D_FLOAT:
			float_to_ushort_2d(data, *buf);
			break;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int convert(struct smbrr *data, enum smbrr_data_type type)
{
	int offset;

	if (type == data->type)
		return 0;

	switch (type) {
	case SMBRR_DATA_1D_UINT32:
		switch (data->type) {
		case SMBRR_DATA_1D_FLOAT:
			for (offset = 0; offset < data->elems; offset++)
				data->s[offset] = (uint32_t)data->adu[offset];
			break;
		case SMBRR_DATA_1D_UINT32:
			break;
		default:
			return -EINVAL;
		}
		break;
	case SMBRR_DATA_1D_FLOAT:
		switch (data->type) {
		case SMBRR_DATA_1D_UINT32:
			for (offset = 0; offset < data->elems; offset++)
				data->adu[offset] = (float)data->s[offset];
			break;
		case SMBRR_DATA_1D_FLOAT:
			break;
		default:
			return -EINVAL;
		}
		break;
	case SMBRR_DATA_2D_UINT32:
		switch (data->type) {
		case SMBRR_DATA_2D_FLOAT:
			for (offset = 0; offset < data->elems; offset++)
				data->s[offset] = (uint32_t)data->adu[offset];
			break;
		case SMBRR_DATA_2D_UINT32:
			break;
		default:
			return -EINVAL;
		}
		break;
	case SMBRR_DATA_2D_FLOAT:
		switch (data->type) {
		case SMBRR_DATA_2D_UINT32:
			for (offset = 0; offset < data->elems; offset++)
				data->adu[offset] = (float)data->s[offset];
			break;
		case SMBRR_DATA_2D_FLOAT:
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	data->type = type;
	return 0;
}

static void find_limits(struct smbrr *data, float *min, float *max)
{
	float *adu = data->adu, _min, _max;
	int offset;

	_min = 1.0e6;
	_max = -1.0e6;

	for (offset = 0; offset < data->elems; offset++) {

		if (adu[offset] > _max)
			_max = adu[offset];

		if (adu[offset] < _min)
			_min = adu[offset];
	}

	*max = _max;
	*min = _min;
}

static void normalise(struct smbrr *data, float min, float max)
{
	float _min, _max, _range, range, factor;
	float *adu = data->adu;
	size_t offset;

	smbrr_find_limits(data, &_min, &_max);

	_range = _max - _min;
	range = max - min;
	factor = range / _range;

	for (offset = 0; offset < data->elems; offset++)
		adu[offset] = (adu[offset] - _min) * factor;
}

static void add(struct smbrr *a, struct smbrr *b,
	struct smbrr *c)
{
	float *A = a->adu;
	float *B = b->adu;
	float *C = c->adu;
	size_t offset;

	/* A = B + C */
	for (offset = 0; offset < a->elems; offset++)
		A[offset] = B[offset] + C[offset];
}

static void add_sig(struct smbrr *a, struct smbrr *b,
	struct smbrr *c, struct smbrr *s)
{
	float *A = a->adu;
	float *B = b->adu;
	float *C = c->adu;
	uint32_t *S = s->s;
	int offset;

	/* iff S then A = B + C */
	for (offset = 0; offset < a->elems; offset++) {
		if (S[offset])
			A[offset] = B[offset] + C[offset];
	}
}

static void mult_add(struct smbrr *dest, struct smbrr *a,
	struct smbrr *b, float c)
{
	float *A = a->adu;
	float *B = b->adu;
	float *D = dest->adu;
	size_t offset;

	/* dest = a + b * c */
	for (offset = 0; offset < dest->elems; offset++)
		D[offset] = A[offset] + B[offset] * c;
}

static void mult_subtract(struct smbrr *dest, struct smbrr *a,
	struct smbrr *b, float c)
{
	float *A = a->adu;
	float *B = b->adu;
	float *D = dest->adu;
	size_t offset;

	/* dest = a - b * c */
	for (offset = 0; offset < dest->elems; offset++)
		D[offset] = A[offset] - B[offset] * c;
}

static void subtract(struct smbrr *a, struct smbrr *b,
	struct smbrr *c)
{
	float *A = a->adu;
	float *B = b->adu;
	float *C = c->adu;
	size_t offset;

	/* A = B - C */
	for (offset = 0; offset < a->elems; offset++)
		A[offset] = B[offset] - C[offset];
}

static void subtract_sig(struct smbrr *a, struct smbrr *b,
	struct smbrr *c, struct smbrr *s)
{
	float *A = a->adu, *B = b->adu, *C = c->adu;
	uint32_t *S = s->s;
	int offset;

	/* iff S then A = B - C */
	for (offset = 0; offset < a->elems; offset++) {
		if (S[offset])
			A[offset] = B[offset] - C[offset];
	}
}

static void add_value(struct smbrr *data, float value)
{
	size_t offset;
	float *adu = data->adu;

	for (offset = 0; offset < data->elems; offset++)
		adu[offset] += value;
}

static void add_value_sig(struct smbrr *data,
	struct smbrr *sdata, float value)
{
	float *adu = data->adu;
	int offset;

	for (offset = 0; offset < data->elems; offset++) {
		if (sdata->s[offset])
			adu[offset] += value;
	}
}

static void subtract_value(struct smbrr *data, float value)
{
	size_t offset;
	float *adu = data->adu;

	for (offset = 0; offset < data->elems; offset++)
		adu[offset] -= value;
}

static void mult_value(struct smbrr *data, float value)
{
	size_t offset;
	float *adu = data->adu;

	for (offset = 0; offset < data->elems; offset++)
		adu[offset] *= value;
}

static void reset_value(struct smbrr *data, float value)
{
	float *adu = data->adu;
	int offset;

	for (offset = 0; offset < data->elems; offset++)
		adu[offset] = value;
}

static void set_sig_value(struct smbrr *data, uint32_t value)
{
	uint32_t *adu = data->s;
	int offset;

	for (offset = 0; offset < data->elems; offset++)
		adu[offset] = value;

	if (value == 0)
		data->sig_pixels = 0;
	else
		data->sig_pixels = data->elems;
}

static void set_value_sig(struct smbrr *data,
	struct smbrr *sdata, float sig_value)
{
	float *adu = data->adu;
	uint32_t *sig = sdata->s;
	int offset;

	for (offset = 0; offset < data->elems; offset++) {
		if (sig[offset])
			adu[offset] = sig_value;
	}
}

static void clear_negative(struct smbrr *data)
{
	float *i = data->adu;
	int offset;

	for (offset = 0; offset < data->elems; offset++)
		if (i[offset] < 0.0)
			i[offset] = 0.0;
}

static void sabs(struct smbrr *data)
{
	float *i = data->adu;
	int offset;

	for (offset = 0; offset < data->elems; offset++)
		i[offset] = fabs(i[offset]);
}

static float get_mean(struct smbrr *data)
{
	float mean = 0.0;
	int i;

	for (i = 0; i < data->elems; i++)
		mean += data->adu[i];

	mean /= (float)data->elems;
	return mean;
}

static float get_mean_sig(struct smbrr *data,
	struct smbrr *sdata)
{
	float mean_sig = 0.0;
	int i, ssize = 0;

	if (data->height != sdata->height ||
		data->width != sdata->width)
		return 0.0;

	for (i = 0; i < data->elems; i++) {

		if (!sdata->s[i])
			continue;

		mean_sig += data->adu[i];
		ssize++;
	}

	mean_sig /= (float)ssize;
	return mean_sig;
}

static float get_sigma(struct smbrr *data, float mean)
{
	float t, sigma = 0.0;
	int i;

	for (i = 0; i < data->elems;  i++) {
		t = data->adu[i] - mean;
		t *= t;
		sigma += t;
	}

	sigma /= (float) data->elems;
	sigma = sqrtf(sigma);
	return sigma;
}

static float get_norm(struct smbrr *data)
{
	float norm = 0.0;
	int i;

	for (i = 0; i < data->elems;  i++)
		norm += data->adu[i] * data->adu[i];

	return sqrtf(norm);
}

static void copy_sig(struct smbrr *dest, struct smbrr *src, struct smbrr *sig)
{
	int i;

	/* bottom scale has not sig data */
	if (sig == NULL) {
		memcpy(dest->adu, src->adu, src->elems * sizeof(float));
		return;
	}

	for (i = 0; i < dest->elems; i++) {
		if (sig->s[i])
			dest->adu[i] = src->adu[i];
		else
			dest->adu[i] = 0.0;
	}
}

static int sign(struct smbrr *s, struct smbrr *n)
{
	int i;

	if (s->elems != n->elems)
		return -EINVAL;

	for (i = 0; i < s->elems; i++) {
		if (n->adu[i] < 0.0)
			s->adu[i] = -s->adu[i];
	}

	return 0;
}

static float get_sigma_sig(struct smbrr *data,
	struct smbrr *sdata, float mean_sig)
{
	float t, sigma_sig = 0.0;
	int i, ssize = 0;

	for (i = 0; i < data->elems;  i++) {

		if (!sdata->s[i])
			continue;

		t = data->adu[i] - mean_sig;
		t *= t;
		sigma_sig += t;
		ssize++;
	}

	sigma_sig /= (float) ssize;
	sigma_sig = sqrtf(sigma_sig);
	return sigma_sig;
}

static void anscombe(struct smbrr *data, float gain, float bias,
	float readout)
{
	float hgain, cgain, r;
	int i;

	/* HAIP Equ 18.9 */
	hgain = gain / 2.0;
	cgain = (gain * gain) * 0.375;
	r = readout * readout;
	r += cgain;

	for (i = 0; i < data->elems;  i++)
		 data->adu[i] = hgain * sqrtf(gain * (data->adu[i] - bias) + r);
}

static void new_significance(struct smbrr *data,
	struct smbrr *sdata, float sigma)
{
	int i;

	if (data->height != sdata->height ||
		data->width != sdata->width)
		return;

	/* clear the old significance data */
	bzero(sdata->s, sizeof(uint32_t) * sdata->elems);
	sdata->sig_pixels = 0;

	for (i = 0; i < data->elems; i++) {
		if (data->adu[i] >= sigma) {
			sdata->s[i] = 1;
			sdata->sig_pixels++;
		}
	}
}

static int psf_1d(struct smbrr *src, struct smbrr *dest,
	enum smbrr_wavelet_mask mask)
{
	const float *data;
	int width, maskoff, clip, c, srcoff;
	float *s, *d;

	switch (mask) {
	case SMBRR_WAVELET_MASK_LINEAR:
		data = (float*)linear_mask_inverse_1d;
		c = 3;
		clip = 1;
		break;
	case SMBRR_WAVELET_MASK_BICUBIC:
		data = (float*)bicubic_mask_inverse_1d;
		c = 5;
		clip = 2;
		break;
	default:
		return -EINVAL;
	}

	s = src->adu;
	d = dest->adu;

	/* clear each scale */
	reset_value(dest, 0.0);

	/* signal width loop */
	for (width = clip; width < src->width - clip; width++) {

		/* mask  loop */
		for (maskoff = 0; maskoff < c; maskoff++) {
			srcoff = width - clip + maskoff;
			d[width] += s[srcoff] * data[maskoff];
		}
	}

	return 0;
}

static int psf_2d(struct smbrr *src, struct smbrr *dest,
	enum smbrr_wavelet_mask mask)
{
	const float *data;
	int height, width;
	int x, y, xc, yc;
	int offy, offx, pixel, offxy, maskxy;
	float *s, *d;

	switch (mask) {
	case SMBRR_WAVELET_MASK_LINEAR:
		data = (float*)linear_mask_inverse_2d;
		xc = 3;
		yc = 3;
		break;
	case SMBRR_WAVELET_MASK_BICUBIC:
		data = (float*)bicubic_mask_inverse_2d;
		xc = 5;
		yc = 5;
		break;
	default:
		return -EINVAL;
	}

	s = src->adu;
	d = dest->adu;

	/* clear each scale */
	reset_value(dest, 0.0);

	/* data height loop */
	for (height = 0; height < src->height; height++) {

		/* data width loop */
		for (width = 0; width < src->width; width++) {

			pixel = height * src->width + width;

			/* mask y loop */
			for (y = 0; y < yc; y++) {
				offy = y_boundary(height, height + (y - yc));

				/* mask x loop */
				for (x = 0; x < xc; x++) {
					offx = x_boundary(width, width + (x - xc));
					offxy = data_get_offset(src, offx, offy);
					maskxy = mask_get_offset(xc, x, y);
					d[pixel] += s[offxy] * data[maskxy];
				}
			}
		}
	}

	return 0;
}

const struct data_ops OPS(data_ops_1d) = {
	.abs = sabs,
	.sign =  sign,
	.find_limits =find_limits,
	.get_mean = get_mean,
	.get_sigma = get_sigma,
	.get_mean_sig = get_mean_sig,
	.get_sigma_sig = get_sigma_sig,
	.get_norm = get_norm,
	.normalise = normalise,
	.add = add,
	.add_value_sig = add_value_sig,
	.add_sig = add_sig,
	.subtract = subtract,
	.subtract_sig = subtract_sig,
	.add_value = add_value,
	.subtract_value = subtract_value,
	.mult_value = mult_value,
	.reset_value = reset_value,
	.set_value_sig = set_value_sig,
	.convert = convert,
	.set_sig_value = set_sig_value,
	.clear_negative = clear_negative,
	.mult_add = mult_add,
	.mult_subtract = mult_subtract,
	.anscombe = anscombe,
	.new_significance = new_significance,
	.copy_sig = copy_sig,
	.get = get,
	.psf = psf_1d,

	.uchar_to_float = uchar_to_float_1d,
	.ushort_to_float = ushort_to_float_1d,
	.uint_to_float = uint_to_float_1d,
	.float_to_uchar = float_to_uchar_1d,
	.uint_to_uint = uint_to_uint_1d,
	.ushort_to_uint = ushort_to_uint_1d,
	.uchar_to_uint = uchar_to_uint_1d,
	.float_to_uint = float_to_uint_1d,
	.float_to_float = float_to_float_1d,
	.uint_to_uchar = uint_to_uchar_1d,
};

const struct data_ops OPS(data_ops_2d) = {
	.abs = sabs,
	.sign =  sign,
	.find_limits = find_limits,
	.get_mean = get_mean,
	.get_sigma = get_sigma,
	.get_mean_sig = get_mean_sig,
	.get_sigma_sig = get_sigma_sig,
	.get_norm = get_norm,
	.normalise = normalise,
	.add = add,
	.add_value_sig = add_value_sig,
	.add_sig = add_sig,
	.subtract = subtract,
	.subtract_sig = subtract_sig,
	.add_value = add_value,
	.subtract_value = subtract_value,
	.mult_value = mult_value,
	.reset_value = reset_value,
	.set_value_sig = set_value_sig,
	.convert = convert,
	.set_sig_value = set_sig_value,
	.clear_negative = clear_negative,
	.mult_add = mult_add,
	.mult_subtract = mult_subtract,
	.anscombe = anscombe,
	.new_significance = new_significance,
	.copy_sig = copy_sig,
	.get = get,
	.psf = psf_2d,

	.uchar_to_float = uchar_to_float_2d,
	.ushort_to_float = ushort_to_float_2d,
	.uint_to_float = uint_to_float_2d,
	.float_to_uchar = float_to_uchar_2d,
	.uint_to_uint = uint_to_uint_2d,
	.ushort_to_uint = ushort_to_uint_2d,
	.uchar_to_uint = uchar_to_uint_2d,
	.float_to_uint = float_to_uint_2d,
	.float_to_float = float_to_float_2d,
	.uint_to_uchar = uint_to_uchar_2d,
};
