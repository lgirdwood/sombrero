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

struct smbrr_image;
struct smbrr_signal;
struct smbrr_wavelet_2d;

struct image_ops {

	float (*get_mean)(struct smbrr_image *image);
	float (*get_sigma)(struct smbrr_image *image, float mean);
	float (*get_mean_sig)(struct smbrr_image *image,
		struct smbrr_image *simage);
	float (*get_sigma_sig)(struct smbrr_image *image,
		struct smbrr_image *simage, float mean);
	float (*get_norm)(struct smbrr_image *image);
	void (*normalise)(struct smbrr_image *image, float min, float max);
	void (*add)(struct smbrr_image *a, struct smbrr_image *b,
		struct smbrr_image *c);
	void (*add_value_sig)(struct smbrr_image *image,
		struct smbrr_image *simage, float value);
	void (*add_sig)(struct smbrr_image *a, struct smbrr_image *b,
		struct smbrr_image *c, struct smbrr_image *s);
	void (*subtract)(struct smbrr_image *a, struct smbrr_image *b,
		struct smbrr_image *c);
	void (*subtract_sig)(struct smbrr_image *a, struct smbrr_image *b,
		struct smbrr_image *c, struct smbrr_image *s);
	void (*add_value)(struct smbrr_image *a, float value);
	void (*subtract_value)(struct smbrr_image *a, float value);
	void (*mult_value)(struct smbrr_image *a, float value);
	void (*reset_value)(struct smbrr_image *a, float value);
	void (*set_value_sig)(struct smbrr_image *a,
		struct smbrr_image *s, float value);
	int (*convert)(struct smbrr_image *a, enum smbrr_data_type type);
	void (*set_sig_value)(struct smbrr_image *a, uint32_t value);
	void (*clear_negative)(struct smbrr_image *a);
	int (*copy)(struct smbrr_image *dest, struct smbrr_image *src);
	void (*fma)(struct smbrr_image *dest, struct smbrr_image *a,
		struct smbrr_image *b, float c);
	void (*fms)(struct smbrr_image *dest, struct smbrr_image *a,
		struct smbrr_image *b, float c);
	void (*anscombe)(struct smbrr_image *image, float gain, float bias,
		float readout);
	void (*new_significance)(struct smbrr_image *a,
		struct smbrr_image *s, float sigma);
	void (*find_limits)(struct smbrr_image *image, float *min, float *max);
	int (*get)(struct smbrr_image *image, enum smbrr_adu adu,
		void **buf);
	int (*psf)(struct smbrr_image *src, struct smbrr_image *dest,
		enum smbrr_wavelet_mask mask);

	/* conversion */
	void (*uchar_to_float)(struct smbrr_image *i, const unsigned char *c);
	void (*ushort_to_float)(struct smbrr_image *i, const unsigned short *c);
	void (*uint_to_float)(struct smbrr_image *i, const unsigned int *c);
	void (*float_to_uchar)(struct smbrr_image *i, unsigned char *c);
	void (*uint_to_uint)(struct smbrr_image *i, const unsigned int *c);
	void (*ushort_to_uint)(struct smbrr_image *i, const unsigned short *c);
	void (*uchar_to_uint)(struct smbrr_image *i, const unsigned char *c);
	void (*float_to_uint)(struct smbrr_image *i, const float *c);
	void (*float_to_float)(struct smbrr_image *i, const float *c);
	void (*uint_to_uchar)(struct smbrr_image *i, unsigned char *c);
};

struct signal_ops {

	float (*get_mean)(struct smbrr_signal *signal);
	float (*get_sigma)(struct smbrr_signal *signal, float mean);
	float (*get_mean_sig)(struct smbrr_signal *signal,
		struct smbrr_signal *ssignal);
	float (*get_sigma_sig)(struct smbrr_signal *signal,
		struct smbrr_signal *ssignal, float mean);
	float (*get_norm)(struct smbrr_signal *signal);
	void (*normalise)(struct smbrr_signal *signal, float min, float max);
	void (*add)(struct smbrr_signal *a, struct smbrr_signal *b,
		struct smbrr_signal *c);
	void (*add_value_sig)(struct smbrr_signal *signal,
		struct smbrr_signal *ssignal, float value);
	void (*add_sig)(struct smbrr_signal *a, struct smbrr_signal *b,
		struct smbrr_signal *c, struct smbrr_signal *s);
	void (*subtract)(struct smbrr_signal *a, struct smbrr_signal *b,
		struct smbrr_signal *c);
	void (*subtract_sig)(struct smbrr_signal *a, struct smbrr_signal *b,
		struct smbrr_signal *c, struct smbrr_signal *s);
	void (*add_value)(struct smbrr_signal *a, float value);
	void (*subtract_value)(struct smbrr_signal *a, float value);
	void (*mult_value)(struct smbrr_signal *a, float value);
	void (*reset_value)(struct smbrr_signal *a, float value);
	void (*set_value_sig)(struct smbrr_signal *a,
		struct smbrr_signal *s, float value);
	int (*convert)(struct smbrr_signal *a, enum smbrr_data_type type);
	void (*set_sig_value)(struct smbrr_signal *a, uint32_t value);
	void (*clear_negative)(struct smbrr_signal *a);
	int (*copy)(struct smbrr_signal *dest, struct smbrr_signal *src);
	void (*fma)(struct smbrr_signal *dest, struct smbrr_signal *a,
		struct smbrr_signal *b, float c);
	void (*fms)(struct smbrr_signal *dest, struct smbrr_signal *a,
		struct smbrr_signal *b, float c);
	void (*anscombe)(struct smbrr_signal *signal, float gain, float bias,
		float readout);
	void (*new_significance)(struct smbrr_signal *a,
		struct smbrr_signal *s, float sigma);
	void (*find_limits)(struct smbrr_signal *signal, float *min, float *max);
	int (*get)(struct smbrr_signal *signal, enum smbrr_adu adu,
		void **buf);
	int (*psf)(struct smbrr_signal *src, struct smbrr_signal *dest,
		enum smbrr_wavelet_mask mask);

	/* conversion */
	void (*uchar_to_float)(struct smbrr_signal *i, const unsigned char *c);
	void (*ushort_to_float)(struct smbrr_signal *i, const unsigned short *c);
	void (*uint_to_float)(struct smbrr_signal *i, const unsigned int *c);
	void (*float_to_uchar)(struct smbrr_signal *i, unsigned char *c);
	void (*uint_to_uint)(struct smbrr_signal *i, const unsigned int *c);
	void (*ushort_to_uint)(struct smbrr_signal *i, const unsigned short *c);
	void (*uchar_to_uint)(struct smbrr_signal *i, const unsigned char *c);
	void (*float_to_uint)(struct smbrr_signal *i, const float *c);
	void (*float_to_float)(struct smbrr_signal *i, const float *c);
	void (*uint_to_uchar)(struct smbrr_signal *i, unsigned char *c);
};

struct convolution2d_ops {
	void (*atrous_conv)(struct smbrr_wavelet_2d *wavelet);
	void (*atrous_conv_sig)(struct smbrr_wavelet_2d *wavelet);
	void (*atrous_deconv_object)(struct smbrr_wavelet_2d *w,
		struct smbrr_object *object);
};

struct convolution1d_ops {
	void (*atrous_conv)(struct smbrr_wavelet_1d *wavelet);
	void (*atrous_conv_sig)(struct smbrr_wavelet_1d *wavelet);
	void (*atrous_deconv_object)(struct smbrr_wavelet_1d *w,
		struct smbrr_object *object);
};

extern const struct image_ops image_ops;
extern const struct image_ops image_ops_sse42;
extern const struct image_ops image_ops_avx;
extern const struct image_ops image_ops_avx2;
extern const struct image_ops image_ops_fma;

extern const struct signal_ops signal_ops;
extern const struct signal_ops signal_ops_sse42;
extern const struct signal_ops signal_ops_avx;
extern const struct signal_ops signal_ops_avx2;
extern const struct signal_ops signal_ops_fma;

extern const struct convolution2d_ops conv2d_ops;
extern const struct convolution2d_ops conv2d_ops_sse42;
extern const struct convolution2d_ops conv2d_ops_avx;
extern const struct convolution2d_ops conv2d_ops_avx2;
extern const struct convolution2d_ops conv2d_ops_fma;

extern const struct convolution1d_ops conv1d_ops;
extern const struct convolution1d_ops conv1d_ops_sse42;
extern const struct convolution1d_ops conv1d_ops_avx;
extern const struct convolution1d_ops conv1d_ops_avx2;
extern const struct convolution1d_ops conv1d_ops_fma;

#endif
#endif
