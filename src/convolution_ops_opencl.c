/*
 *  Copyright (C) 2026 Liam Girdwood
 */

#include "config.h"
#include "local.h"
#include "ops.h"
#include "mask.h"

#ifdef HAVE_OPENCL
#include <CL/cl.h>
#include "cl_ctx.h"

static inline void sync_to_cpu(struct smbrr *s)
{
	if (g_cl_ctx && s->cl_state == 1) {
		size_t el_size = (s->type == SMBRR_DATA_1D_UINT32 ||
						  s->type == SMBRR_DATA_2D_UINT32) ?
							 sizeof(uint32_t) :
							 sizeof(float);
		clEnqueueReadBuffer(g_cl_ctx->command_queue, s->cl_adu, CL_TRUE, 0,
							s->elems * el_size, s->adu, 0, NULL, NULL);
		s->cl_state = 2;
	}
}

static inline void mark_gpu_modified(struct smbrr *s)
{
	s->cl_state = 1;
}

static void cl_atrous_conv_1d(struct smbrr_wavelet *wavelet)
{
	int scale, scale2;
	cl_int err;

	cl_mem mask_buf = clCreateBuffer(g_cl_ctx->context,
									 CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
									 wavelet->mask.width * sizeof(float),
									 (void *)wavelet->mask.data, &err);
	if (err != CL_SUCCESS) {
		/* Fallback on mask upload failure */
		conv_ops_1d.atrous_conv(wavelet);
		return;
	}

	/* scale loop */
	for (scale = 1; scale < wavelet->num_scales; scale++) {
		struct smbrr *c = wavelet->c[scale];
		struct smbrr *c_prev = wavelet->c[scale - 1];

		/* clear each scale */
		smbrr_set_value(c, 0.0);

		scale2 = 1 << (scale - 1);

		clSetKernelArg(g_cl_ctx->k_atrous_conv_1d, 0, sizeof(cl_mem),
					   &c->cl_adu);
		clSetKernelArg(g_cl_ctx->k_atrous_conv_1d, 1, sizeof(cl_mem),
					   &c_prev->cl_adu);
		clSetKernelArg(g_cl_ctx->k_atrous_conv_1d, 2, sizeof(cl_mem),
					   &mask_buf);
		clSetKernelArg(g_cl_ctx->k_atrous_conv_1d, 3, sizeof(int),
					   &wavelet->width);
		clSetKernelArg(g_cl_ctx->k_atrous_conv_1d, 4, sizeof(int),
					   &wavelet->mask.width);
		clSetKernelArg(g_cl_ctx->k_atrous_conv_1d, 5, sizeof(int), &scale2);

		size_t global_item_size = wavelet->width;
		clEnqueueNDRangeKernel(g_cl_ctx->command_queue,
							   g_cl_ctx->k_atrous_conv_1d, 1, NULL,
							   &global_item_size, NULL, 0, NULL, NULL);
		mark_gpu_modified(c);
	}

	clReleaseMemObject(mask_buf);

	for (scale = 1; scale < wavelet->num_scales; scale++)
		smbrr_subtract(wavelet->w[scale - 1], wavelet->c[scale - 1],
					   wavelet->c[scale]);
}

static void cl_atrous_conv_sig_1d(struct smbrr_wavelet *wavelet)
{
	int scale, scale2;
	cl_int err;

	cl_mem mask_buf = clCreateBuffer(g_cl_ctx->context,
									 CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
									 wavelet->mask.width * sizeof(float),
									 (void *)wavelet->mask.data, &err);
	if (err != CL_SUCCESS) {
		conv_ops_1d.atrous_conv_sig(wavelet);
		return;
	}

	for (scale = 1; scale < wavelet->num_scales; scale++) {
		struct smbrr *c = wavelet->c[scale];
		struct smbrr *c_prev = wavelet->c[scale - 1];
		struct smbrr *sig = wavelet->s[scale - 1];

		smbrr_set_value(c, 0.0);

		sync_to_cpu(sig);
		if (sig->sig_pixels == 0)
			continue;

		scale2 = 1 << (scale - 1);

		clSetKernelArg(g_cl_ctx->k_atrous_conv_sig_1d, 0, sizeof(cl_mem),
					   &c->cl_adu);
		clSetKernelArg(g_cl_ctx->k_atrous_conv_sig_1d, 1, sizeof(cl_mem),
					   &c_prev->cl_adu);
		clSetKernelArg(g_cl_ctx->k_atrous_conv_sig_1d, 2, sizeof(cl_mem),
					   &mask_buf);
		clSetKernelArg(g_cl_ctx->k_atrous_conv_sig_1d, 3, sizeof(cl_mem),
					   &sig->cl_adu);
		clSetKernelArg(g_cl_ctx->k_atrous_conv_sig_1d, 4, sizeof(int),
					   &wavelet->width);
		clSetKernelArg(g_cl_ctx->k_atrous_conv_sig_1d, 5, sizeof(int),
					   &wavelet->mask.width);
		clSetKernelArg(g_cl_ctx->k_atrous_conv_sig_1d, 6, sizeof(int), &scale2);

		size_t global_item_size = wavelet->width;
		clEnqueueNDRangeKernel(g_cl_ctx->command_queue,
							   g_cl_ctx->k_atrous_conv_sig_1d, 1, NULL,
							   &global_item_size, NULL, 0, NULL, NULL);
		mark_gpu_modified(c);
	}

	clReleaseMemObject(mask_buf);

	for (scale = 1; scale < wavelet->num_scales; scale++)
		smbrr_subtract(wavelet->w[scale - 1], wavelet->c[scale - 1],
					   wavelet->c[scale]);
}

static void cl_atrous_conv_2d(struct smbrr_wavelet *wavelet)
{
	int scale, scale2;
	cl_int err;

	cl_mem mask_buf = clCreateBuffer(
		g_cl_ctx->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		wavelet->mask.width * wavelet->mask.height * sizeof(float),
		(void *)wavelet->mask.data, &err);
	if (err != CL_SUCCESS) {
		conv_ops_2d.atrous_conv(wavelet);
		return;
	}

	for (scale = 1; scale < wavelet->num_scales; scale++) {
		struct smbrr *c = wavelet->c[scale];
		struct smbrr *c_prev = wavelet->c[scale - 1];

		smbrr_set_value(c, 0.0);
		scale2 = 1 << (scale - 1);

		clSetKernelArg(g_cl_ctx->k_atrous_conv_2d, 0, sizeof(cl_mem),
					   &c->cl_adu);
		clSetKernelArg(g_cl_ctx->k_atrous_conv_2d, 1, sizeof(cl_mem),
					   &c_prev->cl_adu);
		clSetKernelArg(g_cl_ctx->k_atrous_conv_2d, 2, sizeof(cl_mem),
					   &mask_buf);
		clSetKernelArg(g_cl_ctx->k_atrous_conv_2d, 3, sizeof(int),
					   &wavelet->width);
		clSetKernelArg(g_cl_ctx->k_atrous_conv_2d, 4, sizeof(int),
					   &wavelet->height);
		clSetKernelArg(g_cl_ctx->k_atrous_conv_2d, 5, sizeof(int),
					   &wavelet->mask.width);
		clSetKernelArg(g_cl_ctx->k_atrous_conv_2d, 6, sizeof(int), &scale2);

		size_t global_item_size[2] = { wavelet->width, wavelet->height };
		clEnqueueNDRangeKernel(g_cl_ctx->command_queue,
							   g_cl_ctx->k_atrous_conv_2d, 2, NULL,
							   global_item_size, NULL, 0, NULL, NULL);
		mark_gpu_modified(c);
	}

	clReleaseMemObject(mask_buf);

	for (scale = 1; scale < wavelet->num_scales; scale++)
		smbrr_subtract(wavelet->w[scale - 1], wavelet->c[scale - 1],
					   wavelet->c[scale]);
}

static void cl_atrous_conv_sig_2d(struct smbrr_wavelet *wavelet)
{
	int scale, scale2;
	cl_int err;

	cl_mem mask_buf = clCreateBuffer(
		g_cl_ctx->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		wavelet->mask.width * wavelet->mask.height * sizeof(float),
		(void *)wavelet->mask.data, &err);
	if (err != CL_SUCCESS) {
		conv_ops_2d.atrous_conv_sig(wavelet);
		return;
	}

	for (scale = 1; scale < wavelet->num_scales; scale++) {
		struct smbrr *c = wavelet->c[scale];
		struct smbrr *c_prev = wavelet->c[scale - 1];
		struct smbrr *sig = wavelet->s[scale - 1];

		smbrr_set_value(c, 0.0);
		sync_to_cpu(sig);
		if (sig->sig_pixels == 0)
			continue;

		scale2 = 1 << (scale - 1);

		clSetKernelArg(g_cl_ctx->k_atrous_conv_sig_2d, 0, sizeof(cl_mem),
					   &c->cl_adu);
		clSetKernelArg(g_cl_ctx->k_atrous_conv_sig_2d, 1, sizeof(cl_mem),
					   &c_prev->cl_adu);
		clSetKernelArg(g_cl_ctx->k_atrous_conv_sig_2d, 2, sizeof(cl_mem),
					   &mask_buf);
		clSetKernelArg(g_cl_ctx->k_atrous_conv_sig_2d, 3, sizeof(cl_mem),
					   &sig->cl_adu);
		clSetKernelArg(g_cl_ctx->k_atrous_conv_sig_2d, 4, sizeof(int),
					   &wavelet->width);
		clSetKernelArg(g_cl_ctx->k_atrous_conv_sig_2d, 5, sizeof(int),
					   &wavelet->height);
		clSetKernelArg(g_cl_ctx->k_atrous_conv_sig_2d, 6, sizeof(int),
					   &wavelet->mask.width);
		clSetKernelArg(g_cl_ctx->k_atrous_conv_sig_2d, 7, sizeof(int), &scale2);

		size_t global_item_size[2] = { wavelet->width, wavelet->height };
		clEnqueueNDRangeKernel(g_cl_ctx->command_queue,
							   g_cl_ctx->k_atrous_conv_sig_2d, 2, NULL,
							   global_item_size, NULL, 0, NULL, NULL);
		mark_gpu_modified(c);
	}

	clReleaseMemObject(mask_buf);

	for (scale = 1; scale < wavelet->num_scales; scale++)
		smbrr_subtract(wavelet->w[scale - 1], wavelet->c[scale - 1],
					   wavelet->c[scale]);
}

/* Fallback to CPU for deconv Object processing */
static void cl_atrous_deconv_object_1d(struct smbrr_wavelet *w,
									   struct smbrr_object *object)
{
	for (int scale = 1; scale < w->num_scales; scale++) {
		sync_to_cpu(w->c[scale]);
		sync_to_cpu(w->w[scale - 1]);
		if (scale == 1)
			sync_to_cpu(w->c[0]);
	}
	conv_ops_1d.atrous_deconv_object(w, object);
}

static void cl_atrous_deconv_object_2d(struct smbrr_wavelet *w,
									   struct smbrr_object *object)
{
	for (int scale = 1; scale < w->num_scales; scale++) {
		sync_to_cpu(w->c[scale]);
		sync_to_cpu(w->w[scale - 1]);
		if (scale == 1)
			sync_to_cpu(w->c[0]);
	}
	conv_ops_2d.atrous_deconv_object(w, object);
}

const struct convolution_ops conv_ops_1d_opencl = {
	.atrous_conv = cl_atrous_conv_1d,
	.atrous_conv_sig = cl_atrous_conv_sig_1d,
	.atrous_deconv_object = cl_atrous_deconv_object_1d,
};

const struct convolution_ops conv_ops_2d_opencl = {
	.atrous_conv = cl_atrous_conv_2d,
	.atrous_conv_sig = cl_atrous_conv_sig_2d,
	.atrous_deconv_object = cl_atrous_deconv_object_2d,
};

#endif
