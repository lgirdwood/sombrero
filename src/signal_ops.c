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

static void uchar_to_float(struct smbrr_signal *i, const unsigned char *c)
{
	int x;
	float *f = i->adu;

	for (x = 0; x < i->length; x++)
			f[x] = (float)c[x];
}

static void ushort_to_float(struct smbrr_signal *i, const unsigned short *c)
{
	int x;
	float *f = i->adu;

	for (x = 0; x < i->length; x++)
			f[x] = (float)c[x];
}

static void uint_to_float(struct smbrr_signal *i, const unsigned int *c)
{
	int x;
	float *f = i->adu;

	for (x = 0; x < i->length; x++)
			f[x] = (float)c[x];
}

static void float_to_uchar(struct smbrr_signal *i, unsigned char *c)
{
	int x;
	float *f = i->adu;

	for (x = 0; x < i->length; x++)
			c[x] = (unsigned char)f[x];
}

static void uint_to_uint(struct smbrr_signal *i, const unsigned int *c)
{
	int x;
	uint32_t *f = i->s;

	for (x = 0; x < i->length; x++)
			f[x] = (uint32_t)c[x];
}

static void ushort_to_uint(struct smbrr_signal *i, const unsigned short *c)
{
	int x;
	uint32_t *f = i->s;

	for (x = 0; x < i->length; x++)
			f[x] = (uint32_t)c[x];
}

static void uchar_to_uint(struct smbrr_signal *i, const unsigned char *c)
{
	int x;
	uint32_t *f = i->s;

	for (x = 0; x < i->length; x++)
			f[x] = (uint32_t)c[x];
}

static void float_to_uint(struct smbrr_signal *i, const float *c)
{
	int x;
	uint32_t *f = i->s;

	for (x = 0; x < i->length; x++)
			f[x] = (uint32_t)c[x];
}

static void float_to_float(struct smbrr_signal *i, const float *c)
{
	float *f = i->adu;

	memcpy(f, c, i->length * sizeof(float));
}

static void uint_to_uchar(struct smbrr_signal *i, unsigned char *c)
{
	int x;
	uint32_t *f = i->s;

	for (x = 0; x < i->length; x++)
			c[x] = (unsigned char)f[x];
}

static int signal_get(struct smbrr_signal *signal, enum smbrr_adu adu,
	void **buf)
{
	if (buf == NULL)
		return -EINVAL;

	switch (adu) {
	case SMBRR_ADU_8:

		switch (signal->type) {
		case SMBRR_DATA_UINT32:
			uint_to_uchar(signal, *buf);
			break;
		case SMBRR_DATA_FLOAT:
			float_to_uchar(signal, *buf);
			break;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int signal_convert(struct smbrr_signal *signal,
	enum smbrr_data_type type)
{
	int offset;

	if (type == signal->type)
		return 0;

	switch (type) {
	case SMBRR_DATA_UINT32:
		switch (signal->type) {
		case SMBRR_DATA_FLOAT:
			for (offset = 0; offset < signal->size; offset++)
				signal->s[offset] = (uint32_t)signal->adu[offset];
			break;
		case SMBRR_DATA_UINT32:
			break;
		}
		break;
	case SMBRR_DATA_FLOAT:
		switch (signal->type) {
		case SMBRR_DATA_UINT32:
			for (offset = 0; offset < signal->size; offset++)
				signal->adu[offset] = (float)signal->s[offset];
			break;
		case SMBRR_DATA_FLOAT:
			break;
		}
		break;
	default:
		return -EINVAL;
	}

	signal->type = type;
	return 0;
}

static void signal_find_limits(struct smbrr_signal *signal, float *min, float *max)
{
	float *adu = signal->adu, _min, _max;
	int offset;

	_min = 1.0e6;
	_max = -1.0e6;

	for (offset = 0; offset < signal->size; offset++) {

		if (adu[offset] > _max)
			_max = adu[offset];

		if (adu[offset] < _min)
			_min = adu[offset];
	}

	*max = _max;
	*min = _min;
}

static void signal_normalise(struct smbrr_signal *signal, float min, float max)
{
	float _min, _max, _range, range, factor;
	float *adu = signal->adu;
	size_t offset;

	signal_find_limits(signal, &_min, &_max);

	_range = _max - _min;
	range = max - min;
	factor = range / _range;

	for (offset = 0; offset < signal->size; offset++)
		adu[offset] = (adu[offset] - _min) * factor;
}

static void signal_add(struct smbrr_signal *a, struct smbrr_signal *b,
	struct smbrr_signal *c)
{
	float *A = a->adu;
	float *B = b->adu;
	float *C = c->adu;
	size_t offset;

	/* A = B + C */
	for (offset = 0; offset < a->size; offset++)
		A[offset] = B[offset] + C[offset];
}

static void signal_add_sig(struct smbrr_signal *a, struct smbrr_signal *b,
	struct smbrr_signal *c, struct smbrr_signal *s)
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

static void signal_fma(struct smbrr_signal *dest, struct smbrr_signal *a,
	struct smbrr_signal *b, float c)
{
	float *A = a->adu;
	float *B = b->adu;
	float *D = dest->adu;
	size_t offset;

	/* dest = a + b * c */
	for (offset = 0; offset < dest->size; offset++)
		D[offset] = A[offset] + B[offset] * c;
}

static void signal_fms(struct smbrr_signal *dest, struct smbrr_signal *a,
	struct smbrr_signal *b, float c)
{
	float *A = a->adu;
	float *B = b->adu;
	float *D = dest->adu;
	size_t offset;

	/* dest = a - b * c */
	for (offset = 0; offset < dest->size; offset++)
		D[offset] = A[offset] - B[offset] * c;
}

static void signal_subtract(struct smbrr_signal *a, struct smbrr_signal *b,
	struct smbrr_signal *c)
{
	float *A = a->adu;
	float *B = b->adu;
	float *C = c->adu;
	size_t offset;

	/* A = B - C */
	for (offset = 0; offset < a->size; offset++)
		A[offset] = B[offset] - C[offset];
}

static void signal_subtract_sig(struct smbrr_signal *a, struct smbrr_signal *b,
	struct smbrr_signal *c, struct smbrr_signal *s)
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

static void signal_add_value(struct smbrr_signal *signal, float value)
{
	size_t offset;
	float *adu = signal->adu;

	for (offset = 0; offset < signal->size; offset++)
		adu[offset] += value;
}

static void signal_add_value_sig(struct smbrr_signal *signal,
	struct smbrr_signal *ssignal, float value)
{
	float *adu = signal->adu;
	int offset;

	for (offset = 0; offset < signal->size; offset++) {
		if (ssignal->s[offset])
			adu[offset] += value;
	}
}

static void signal_subtract_value(struct smbrr_signal *signal, float value)
{
	size_t offset;
	float *adu = signal->adu;

	for (offset = 0; offset < signal->size; offset++)
		adu[offset] -= value;
}

static void signal_mult_value(struct smbrr_signal *signal, float value)
{
	size_t offset;
	float *adu = signal->adu;

	for (offset = 0; offset < signal->size; offset++)
		adu[offset] *= value;
}

static void signal_reset_value(struct smbrr_signal *signal, float value)
{
	float *adu = signal->adu;
	int offset;

	for (offset = 0; offset < signal->size; offset++)
		adu[offset] = value;
}

static void signal_set_sig_value(struct smbrr_signal *signal, uint32_t value)
{
	uint32_t *adu = signal->s;
	int offset;

	for (offset = 0; offset < signal->size; offset++)
		adu[offset] = value;

	if (value == 0)
		signal->sig_pixels = 0;
	else
		signal->sig_pixels = signal->size;
}

static void signal_set_value_sig(struct smbrr_signal *signal,
	struct smbrr_signal *ssignal, float sig_value)
{
	float *adu = signal->adu;
	uint32_t *sig = ssignal->s;
	int offset;

	for (offset = 0; offset < signal->size; offset++) {
		if (sig[offset])
			adu[offset] = sig_value;
	}
}

static void signal_clear_negative(struct smbrr_signal *signal)
{
	float *i = signal->adu;
	int offset;

	for (offset = 0; offset < signal->size; offset++)
		if (i[offset] < 0.0)
			i[offset] = 0.0;
}

static float signal_get_mean(struct smbrr_signal *signal)
{
	float mean = 0.0;
	int i;

	for (i = 0; i < signal->size; i++)
		mean += signal->adu[i];

	mean /= (float)signal->size;
	return mean;
}

static float signal_get_mean_sig(struct smbrr_signal *signal,
	struct smbrr_signal *ssignal)
{
	float mean_sig = 0.0;
	int i, ssize = 0;

	if (signal->length != ssignal->length)
		return 0.0;

	for (i = 0; i < signal->size; i++) {

		if (!ssignal->s[i])
			continue;

		mean_sig += signal->adu[i];
		ssize++;
	}

	mean_sig /= (float)ssize;
	return mean_sig;
}

static float signal_get_sigma(struct smbrr_signal *signal, float mean)
{
	float t, sigma = 0.0;
	int i;

	for (i = 0; i < signal->size;  i++) {
		t = signal->adu[i] - mean;
		t *= t;
		sigma += t;
	}

	sigma /= (float) signal->size;
	sigma = sqrtf(sigma);
	return sigma;
}

static float signal_get_norm(struct smbrr_signal *signal)
{
	float norm = 0.0;
	int i;

	for (i = 0; i < signal->size;  i++)
		norm += signal->adu[i] * signal->adu[i];

	return sqrtf(norm);
}

static float signal_get_sigma_sig(struct smbrr_signal *signal,
	struct smbrr_signal *ssignal, float mean_sig)
{
	float t, sigma_sig = 0.0;
	int i, ssize = 0;

	for (i = 0; i < signal->size;  i++) {

		if (!ssignal->s[i])
			continue;

		t = signal->adu[i] - mean_sig;
		t *= t;
		sigma_sig += t;
		ssize++;
	}

	sigma_sig /= (float) ssize;
	sigma_sig = sqrtf(sigma_sig);
	return sigma_sig;
}

static void signal_anscombe(struct smbrr_signal *signal, float gain, float bias,
	float readout)
{
	float hgain, cgain, r;
	int i;

	/* HAIP Equ 18.9 */
	hgain = gain / 2.0;
	cgain = (gain * gain) * 0.375;
	r = readout * readout;
	r += cgain;

	for (i = 0; i < signal->size;  i++)
		 signal->adu[i] = hgain * sqrtf(gain * (signal->adu[i] - bias) + r);
}

static void signal_new_significance(struct smbrr_signal *signal,
	struct smbrr_signal *ssignal, float sigma)
{
	int i;

	if (signal->length != ssignal->length)
		return;

	/* clear the old significance data */
	bzero(ssignal->s, sizeof(uint32_t) * ssignal->size);
	ssignal->sig_pixels = 0;

	for (i = 0; i < signal->size; i++) {
		if (signal->adu[i] >= sigma) {
			ssignal->s[i] = 1;
			ssignal->sig_pixels++;
		}
	}
}

static int signal_psf(struct smbrr_signal *src, struct smbrr_signal *dest,
	enum smbrr_wavelet_mask mask)
{
	const float *data;
	int length, maskoff, clip, c, srcoff;
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
	signal_reset_value(dest, 0.0);

	/* signal length loop */
	for (length = clip; length < src->length - clip; length++) {

		/* mask  loop */
		for (maskoff = 0; maskoff < c; maskoff++) {
			srcoff = length - clip + maskoff;
			d[length] += s[srcoff] * data[maskoff];
		}
	}

	return 0;
}

const struct signal_ops OPS(signal_ops) = {
	.find_limits =signal_find_limits,
	.get_mean = signal_get_mean,
	.get_sigma = signal_get_sigma,
	.get_mean_sig = signal_get_mean_sig,
	.get_sigma_sig = signal_get_sigma_sig,
	.get_norm = signal_get_norm,
	.normalise = signal_normalise,
	.add = signal_add,
	.add_value_sig = signal_add_value_sig,
	.add_sig = signal_add_sig,
	.subtract = signal_subtract,
	.subtract_sig = signal_subtract_sig,
	.add_value = signal_add_value,
	.subtract_value = signal_subtract_value,
	.mult_value = signal_mult_value,
	.reset_value = signal_reset_value,
	.set_value_sig = signal_set_value_sig,
	.convert = signal_convert,
	.set_sig_value = signal_set_sig_value,
	.clear_negative = signal_clear_negative,
	.fma = signal_fma,
	.fms = signal_fms,
	.anscombe = signal_anscombe,
	.new_significance = signal_new_significance,
	.get = signal_get,
	.psf = signal_psf,

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
