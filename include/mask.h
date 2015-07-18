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

#ifndef _MASK_H
#define _MASK_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

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
static const float linear_mask_2d[3][3] = {
	{M_1_16, M_1_8, M_1_16},
	{M_1_8, M_1_4, M_1_8},
	{M_1_16, M_1_8, M_1_16},
};

static const float linear_mask_inverse_2d[3][3] = {
	{IM_1_16, IM_1_8, IM_1_16},
	{IM_1_8, IM_1_4, IM_1_8},
	{IM_1_16, IM_1_8, IM_1_16},
};

/* bicubic spline */
static const float bicubic_mask_2d[5][5] = {
	{M_1_256, M_1_64, M_3_128, M_1_64, M_1_256},
	{M_1_64, M_1_16, M_3_32, M_1_16, M_1_64},
	{M_3_128, M_3_32, M_9_64, M_3_32, M_3_128},
	{M_1_64, M_1_16, M_3_32, M_1_16, M_1_64},
	{M_1_256, M_1_64, M_3_128, M_1_64, M_1_256},
};

static const float bicubic_mask_inverse_2d[5][5] = {
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

static inline int conv_mask_set_2d(struct smbrr_wavelet_2d *w,
	enum smbrr_wavelet_mask mask)
{
	switch (mask) {
	case SMBRR_WAVELET_MASK_LINEAR:
		w->mask.data = (float*)linear_mask_2d;
		w->mask.width = 3;
		w->mask.height = 3;
		w->mask_type = mask;
		break;
	case SMBRR_WAVELET_MASK_BICUBIC:
		w->mask.data = (float*)bicubic_mask_2d;
		w->mask.width = 5;
		w->mask.height = 5;
		w->mask_type = mask;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static inline int deconv_mask_set_2d(struct smbrr_wavelet_2d *w,
	enum smbrr_wavelet_mask mask)
{
	switch (mask) {
	case SMBRR_WAVELET_MASK_LINEAR:
		w->mask.data = (float*)linear_mask_inverse_2d;
		w->mask.width = 3;
		w->mask.height = 3;
		w->mask_type = mask;
		break;
	case SMBRR_WAVELET_MASK_BICUBIC:
		w->mask.data = (float*)bicubic_mask_inverse_2d;
		w->mask.width = 5;
		w->mask.height = 5;
		w->mask_type = mask;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static inline int conv_mask_set_1d(struct smbrr_wavelet_1d *w,
	enum smbrr_wavelet_mask mask)
{
	switch (mask) {
	case SMBRR_WAVELET_MASK_LINEAR:
		w->mask.data = (float*)linear_mask_1d;
		w->mask.length = 3;
		w->mask_type = mask;
		break;
	case SMBRR_WAVELET_MASK_BICUBIC:
		w->mask.data = (float*)bicubic_mask_1d;
		w->mask.length = 5;
		w->mask_type = mask;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static inline int deconv_mask_set_1d(struct smbrr_wavelet_1d *w,
	enum smbrr_wavelet_mask mask)
{
	switch (mask) {
	case SMBRR_WAVELET_MASK_LINEAR:
		w->mask.data = (float*)linear_mask_inverse_1d;
		w->mask.length = 3;
		w->mask_type = mask;
		break;
	case SMBRR_WAVELET_MASK_BICUBIC:
		w->mask.data = (float*)bicubic_mask_inverse_1d;
		w->mask.length = 5;
		w->mask_type = mask;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

#endif
#endif
