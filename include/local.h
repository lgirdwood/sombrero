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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

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
};

struct smbrr_wavelet {
	struct smbrr *smbrr;

	unsigned int width;
	unsigned int height;

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

#endif
#endif
