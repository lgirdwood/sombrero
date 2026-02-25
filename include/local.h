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
 *  Copyright (C) 2026 Liam Girdwood
 */

#ifndef _LOCAL_H
#define _LOCAL_H

#include <errno.h>
#include "sombrero.h"
#include <stdint.h>

#ifdef HAVE_OPENCL
#define CL_TARGET_OPENCL_VERSION 200
#include <CL/cl.h>
#include "cl_ctx.h"
#endif

/**
 * \defgroup simd_flags SIMD Optimization Flags
 * \brief Bitmask flags for CPU capability detection.
 * @{
 */
#define CPU_X86_SSE4_2 1 /**< SSE 4.2 capability. */
#define CPU_X86_AVX 2 /**< AVX capability. */
#define CPU_X86_AVX2 4 /**< AVX2 capability. */
#define CPU_X86_FMA 8 /**< FMA capability. */
#define CPU_X86_AVX512 16 /**< AVX512 capability. */
/** @} */

int cpu_get_flags(void);
void check_cpu_flags(void);
void smbrr_cl_sync(struct smbrr *s);
void smbrr_wavelet_cl_sync(struct smbrr_wavelet *w);

/**
 * \struct structure
 * \brief Internal representation of a detected wavelet structure.
 */
struct structure {
	unsigned int object_id; /**< Object ID. */
	unsigned int max_pixel; /**< Maximum pixel index. */
	unsigned int id; /**< Structure ID. */
	unsigned int size; /**< Size in pixels. */
	unsigned int merged; /**< Merged flag. */
	unsigned int pruned; /**< Pruned flag. */
	unsigned int has_root; /**< Has root flag. */
	unsigned int num_branches; /**< Number of branches. */
	unsigned int scale; /**< Wavelet scale. */
	struct smbrr_coord minXy; /**< Minimum X, Y coord. */
	struct smbrr_coord minxY; /**< Minimum x, maximum Y coord. */
	struct smbrr_coord maxXy; /**< Maximum X, minimum Y coord. */
	struct smbrr_coord maxxY; /**< Maximum X, Y coord. */
	float max_value; /**< Maximum pixel value. */
	unsigned int root; /**< Root ID. */
	unsigned int *branch; /**< Branch ID pointers. */
};

/**
 * \struct object
 * \brief Internal representation of a composite wavelet object.
 */
struct object {
	struct smbrr_object o; /**< Public object interface. */

	/* structures that make up this object per scale */
	unsigned int
		structure[SMBRR_MAX_SCALES]; /**< Array of structural components. */
	unsigned int start_scale; /**< Minimum wavelet scale. */
	unsigned int end_scale; /**< Maximum wavelet scale. */
	unsigned int pruned; /**< Pruned status flag. */

	/* reconstructed wavelet data of object */
	struct smbrr *data; /**< Reconstructed output data. */
};

/**
 * \struct wavelet_mask
 * \brief Convolution mask definition.
 */
struct wavelet_mask {
	union {
		unsigned int width; /**< Mask width (2D). */
		unsigned int length; /**< Mask length (1D). */
	};

	unsigned int height; /**< Mask height (2D). */
	const float *data; /**< Mask coefficient elements. */
};

/** \cond */
struct data_ops;

/** \endcond */

/**
 * \struct smbrr
 * \brief Opaque data context storing image buffers and dimensionality.
 */
struct smbrr {
	union {
		float *adu; /**< Float array data. */
		uint32_t *s; /**< Unsigned 32bit array data. */
	};
	enum smbrr_data_type type; /**< Type of elements (1D/2D). */
	unsigned int sig_pixels; /**< Count of significant pixels. */
	unsigned int width; /**< Data width. */
	unsigned int height; /**< Data height. */
	unsigned int elems; /**< Total element count. */
	unsigned int stride; /**< Dimension stride. */
	const struct data_ops *ops; /**< Bound data operations. */
#ifdef HAVE_OPENCL
	cl_mem cl_adu; /**< OpenCL backing array. */
	int cl_state; /**< 0=CPU valid, 1=GPU valid, 2=Both valid */
#endif
};

/** \cond */
struct signal_ops;

/** \endcond */

/**
 * \struct smbrr_wavelet
 * \brief State representation of a decomposed wavelet iteration.
 */
struct smbrr_wavelet {
	struct smbrr *smbrr; /**< Source image/signal context. */

	unsigned int width; /**< Reference dimension width. */
	unsigned int height; /**< Reference dimension height. */
	const struct convolution_ops *ops; /**< Convolution primitives. */

	/* convolution wavelet mask */
	struct wavelet_mask mask; /**< Sub-band wavelet mask. */
	enum smbrr_conv conv_type; /**< Convolution filter type. */
	enum smbrr_wavelet_mask mask_type; /**< Active wavelet mask matrix. */

	/* data scales */
	int num_scales; /**< Total data scales. */
	struct smbrr
		*c[SMBRR_MAX_SCALES]; /**< Original continuous data sub-scales. */
	struct smbrr *w[SMBRR_MAX_SCALES - 1]; /**< Wavelet sparse discrete scales. */
	struct smbrr *s[SMBRR_MAX_SCALES -
					1]; /**< Significant boolean states across scales. */

	/* structures */
	unsigned int num_structures[SMBRR_MAX_SCALES -
								1]; /**< Computed structural quantities. */
	struct structure
		*structure[SMBRR_MAX_SCALES - 1]; /**< Allocated internal structures. */

	/* objects */
	struct object *objects; /**< Active identified objects. */
	struct object **objects_sorted; /**< Sorted composite structures. */
	int num_objects; /**< Number of detected elements. */
	struct object **object_map; /**< Component mapping matrix. */

	/* dark */
	float dark; /**< Baseline dark value. */
	struct smbrr *dark_data; /**< Generated 2D dark map. */

	/* ccd  / receiver */
	float gain; /**< Global gain setting. */
	float bias; /**< Global bias configuration. */
	float readout; /**< General readout value. */
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
