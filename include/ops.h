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

#ifndef _OPS_H
#define _OPS_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

struct smbrr;
struct smbrr_wavelet;

struct data_ops {

	float (*get_mean)(struct smbrr *data);
	float (*get_sigma)(struct smbrr *data, float mean);
	float (*get_mean_sig)(struct smbrr *data,
		struct smbrr *sdata);
	float (*get_sigma_sig)(struct smbrr *data,
		struct smbrr *sdata, float mean);
	float (*get_norm)(struct smbrr *data);
	void (*normalise)(struct smbrr *data, float min, float max);
	void (*add)(struct smbrr *a, struct smbrr *b,
		struct smbrr *c);
	void (*add_value_sig)(struct smbrr *data,
		struct smbrr *sdata, float value);
	void (*add_sig)(struct smbrr *a, struct smbrr *b,
		struct smbrr *c, struct smbrr *s);
	void (*subtract)(struct smbrr *a, struct smbrr *b,
		struct smbrr *c);
	void (*subtract_sig)(struct smbrr *a, struct smbrr *b,
		struct smbrr *c, struct smbrr *s);
	void (*add_value)(struct smbrr *a, float value);
	void (*subtract_value)(struct smbrr *a, float value);
	void (*mult_value)(struct smbrr *a, float value);
	void (*reset_value)(struct smbrr *a, float value);
	void (*set_value_sig)(struct smbrr *a,
		struct smbrr *s, float value);
	int (*convert)(struct smbrr *a, enum smbrr_type type);
	void (*set_sig_value)(struct smbrr *a, uint32_t value);
	void (*clear_negative)(struct smbrr *a);
	int (*copy)(struct smbrr *dest, struct smbrr *src);
	void (*mult_add)(struct smbrr *dest, struct smbrr *a,
		struct smbrr *b, float c);
	void (*mult_subtract)(struct smbrr *dest, struct smbrr *a,
		struct smbrr *b, float c);
	void (*anscombe)(struct smbrr *data, float gain, float bias,
		float readout);
	void (*new_significance)(struct smbrr *a,
		struct smbrr *s, float sigma);
	void (*find_limits)(struct smbrr *data, float *min, float *max);
	int (*get)(struct smbrr *data, enum smbrr_adu adu,
		void **buf);
	int (*psf)(struct smbrr *src, struct smbrr *dest,
		enum smbrr_wavelet_mask mask);

	/* conversion */
	void (*uchar_to_float)(struct smbrr *i, const unsigned char *c);
	void (*ushort_to_float)(struct smbrr *i, const unsigned short *c);
	void (*uint_to_float)(struct smbrr *i, const unsigned int *c);
	void (*float_to_uchar)(struct smbrr *i, unsigned char *c);
	void (*uint_to_uint)(struct smbrr *i, const unsigned int *c);
	void (*ushort_to_uint)(struct smbrr *i, const unsigned short *c);
	void (*uchar_to_uint)(struct smbrr *i, const unsigned char *c);
	void (*float_to_uint)(struct smbrr *i, const float *c);
	void (*float_to_float)(struct smbrr *i, const float *c);
	void (*uint_to_uchar)(struct smbrr *i, unsigned char *c);
};

struct convolution_ops {
	void (*atrous_conv)(struct smbrr_wavelet *wavelet);
	void (*atrous_conv_sig)(struct smbrr_wavelet *wavelet);
	void (*atrous_deconv_object)(struct smbrr_wavelet *w,
		struct smbrr_object *object);
};

extern const struct data_ops data_ops_1d;
extern const struct data_ops data_ops_1d_sse42;
extern const struct data_ops data_ops_1d_avx;
extern const struct data_ops data_ops_1d_avx2;
extern const struct data_ops data_ops_1d_fma;

extern const struct data_ops data_ops_2d;
extern const struct data_ops data_ops_2d_sse42;
extern const struct data_ops data_ops_2d_avx;
extern const struct data_ops data_ops_2d_avx2;
extern const struct data_ops data_ops_2d_fma;

extern const struct convolution_ops conv_ops_1d;
extern const struct convolution_ops conv_ops_1d_sse42;
extern const struct convolution_ops conv_ops_1d_avx;
extern const struct convolution_ops conv_ops_1d_avx2;
extern const struct convolution_ops conv_ops_1d_fma;

extern const struct convolution_ops conv_ops_2d;
extern const struct convolution_ops conv_ops_2d_sse42;
extern const struct convolution_ops conv_ops_2d_avx;
extern const struct convolution_ops conv_ops_2d_avx2;
extern const struct convolution_ops conv_ops_2d_fma;

#endif
#endif
