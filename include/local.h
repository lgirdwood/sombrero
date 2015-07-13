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

#ifndef _LOCAL_H
#define _LOCAL_H

#include <stdint.h>
#include <errno.h>
#include <sombrero.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

/* SIMD optimisation flags */
#define CPU_X86_SSE4_2		1
#define CPU_X86_AVX			2
#define CPU_X86_AVX2			4
#define CPU_X86_FMA			8

int cpu_get_flags(void);

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

struct structure {
	unsigned int object_id;
	unsigned int max_pixel;
	unsigned int id;
	unsigned int size;	/* in pixels */
	unsigned int merged;
	unsigned int pruned;
	unsigned int has_root;
	unsigned int num_branches;
	unsigned int scale;
	struct smbrr_coord minXy;
	struct smbrr_coord minxY;
	struct smbrr_coord maxXy;
	struct smbrr_coord maxxY;
	float max_value;
	unsigned int root, *branch;
};

struct object {
	struct smbrr_object o;

	/* structures that make up this object per scale */
	unsigned int structure[SMBRR_MAX_SCALES];
	unsigned int start_scale;
	unsigned int end_scale;
	unsigned int pruned;

	/* reconstructed wavelet image of object */
	struct smbrr_image *image;
};

struct wavelet_mask {
	unsigned int width;
	unsigned int height;
	const float *data;
};

struct image_ops;

struct smbrr_image {
	union {
		float *adu;
		uint32_t *s;
	};
	enum smbrr_image_type type;
	unsigned int sig_pixels;
	unsigned int width;
	unsigned int height;
	unsigned int size;
	unsigned int stride;
	const struct image_ops *ops;
};

struct signal_ops;

struct smbrr_signal {
	union {
		float *adu;
		uint32_t *s;
	};
	enum smbrr_image_type type;
	unsigned int sig_pixels;
	unsigned int length;
	unsigned int size;
	const struct signal_ops *ops;
};

struct smbrr_wavelet {
	struct smbrr *smbrr;

	unsigned int width;
	unsigned int height;
	const struct convolution2d_ops *ops;

	/* convolution wavelet mask */
	struct wavelet_mask mask;
	enum smbrr_conv conv_type;
	enum smbrr_wavelet_mask mask_type;

	/* image scales */
	int num_scales;
	struct smbrr_image *c[SMBRR_MAX_SCALES];	/* image scales - original is c[0] */
	struct smbrr_image *w[SMBRR_MAX_SCALES - 1];	/* wavelet coefficients scales */
	struct smbrr_image *s[SMBRR_MAX_SCALES - 1];	/* significance images */

	/* structures */
	unsigned int num_structures[SMBRR_MAX_SCALES - 1];
	struct structure *structure[SMBRR_MAX_SCALES - 1];

	/* objects */
	struct object *objects;
	struct object **objects_sorted;
	int num_objects;
	struct object **object_map;

	/* dark */
	float dark;
	struct smbrr_image *dark_image;

	/* ccd */
	float gain;
	float bias;
	float readout;
};

static inline int image_get_offset(struct smbrr_image *image, int offx, int offy)
{
	return offy * image->width + offx;
}

static inline int mask_get_offset(int width, int offx, int offy)
{
	return offy * width + offx;
}

static inline int image_get_x(struct smbrr_image *image, unsigned int pixel)
{
	return pixel % image->width;
}

static inline int image_get_y(struct smbrr_image *image, unsigned int pixel)
{
	return pixel / image->width;
}

static inline int wavelet_get_x(struct smbrr_wavelet *w, unsigned int pixel)
{
	return pixel % w->width;
}

static inline int wavelet_get_y(struct smbrr_wavelet *w, unsigned int pixel)
{
	return pixel / w->width;
}

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
	int (*convert)(struct smbrr_image *a, enum smbrr_image_type type);
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
	int (*convert)(struct smbrr_signal *a, enum smbrr_image_type type);
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
	void (*atrous_conv)(struct smbrr_wavelet *wavelet);
	void (*atrous_conv_sig)(struct smbrr_wavelet *wavelet);
	void (*atrous_deconv_object)(struct smbrr_wavelet *w,
		struct smbrr_object *object);
};

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

/* linear interpolation mask */
static const float linear_mask_1d[3] = {
	M_1_8, M_1_4, M_1_8,
};

static const float linear_mask_inverse_1d[3] = {
	IM_1_8, IM_1_4, IM_1_8,
};

/* bicubic spline */
static const float bicubic_mask_1d[5] = {
	M_3_128, M_3_32, M_9_64, M_3_32, M_3_128,
};

static const float bicubic_mask_inverse_1d[5] = {
	IM_3_128, IM_3_32, IM_9_64, IM_3_32, IM_3_128,
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

static inline int conv_mask_set(struct smbrr_wavelet *w,
	enum smbrr_wavelet_mask mask)
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

static inline int deconv_mask_set(struct smbrr_wavelet *w,
	enum smbrr_wavelet_mask mask)
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

#endif
#endif
