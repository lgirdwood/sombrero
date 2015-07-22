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

	/* reconstructed wavelet data of object */
	struct smbrr *data;
};

struct wavelet_mask {
	union {
		unsigned int width;
		unsigned int length;
	};
	unsigned int height; /* not used on 1d */
	const float *data;
};

struct data_ops;

struct smbrr {
	union {
		float *adu;
		uint32_t *s;
	};
	enum smbrr_data_type type;
	unsigned int sig_pixels;
	unsigned int width;
	unsigned int height;
	unsigned int elems;
	unsigned int stride;
	const struct data_ops *ops;
};

struct signal_ops;

struct smbrr_wavelet {
	struct smbrr *smbrr;

	unsigned int width;
	unsigned int height;
	const struct convolution_ops *ops;

	/* convolution wavelet mask */
	struct wavelet_mask mask;
	enum smbrr_conv conv_type;
	enum smbrr_wavelet_mask mask_type;

	/* data scales */
	int num_scales;
	struct smbrr *c[SMBRR_MAX_SCALES];	/* data scales - original is c[0] */
	struct smbrr *w[SMBRR_MAX_SCALES - 1];	/* wavelet coefficients scales */
	struct smbrr *s[SMBRR_MAX_SCALES - 1];	/* significance datas */

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
	struct smbrr *dark_data;

	/* ccd  / receiver */
	float gain;
	float bias;
	float readout;
};

static inline int data_get_offset(struct smbrr *data, int offx, int offy)
{
	return offy * data->width + offx;
}

static inline int mask_get_offset(int width, int offx, int offy)
{
	return offy * width + offx;
}

static inline int data_get_x(struct smbrr *data, unsigned int pixel)
{
	return pixel % data->width;
}

static inline int data_get_y(struct smbrr *data, unsigned int pixel)
{
	return pixel / data->width;
}

static inline int wavelet_get_x(struct smbrr_wavelet *w, unsigned int pixel)
{
	return pixel % w->width;
}

static inline int wavelet_get_y(struct smbrr_wavelet *w, unsigned int pixel)
{
	return pixel / w->width;
}

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

#endif
#endif
