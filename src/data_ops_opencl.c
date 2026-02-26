/*
 *  Copyright (C) 2026 Liam Girdwood
 */

#include "config.h"
#include "local.h"
#include "ops.h"
#include <stdio.h>

#ifdef HAVE_OPENCL
#include <CL/cl.h>
#include "cl_ctx.h"

#include "cl_ctx.h"

/**
 * \brief Synchronize data back from the OpenCL GPU buffer to the CPU RAM.
 *
 * \param s Pointer to the smbrr data structure.
 *
 * If the OpenCL buffer (cl_adu) is newer than the CPU buffer (adu),
 * this functions reads the data back via `clEnqueueReadBuffer`.
 */
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

/**
 * \brief Mark an OpenCL GPU buffer as the latest version of the data.
 *
 * \param s Pointer to the smbrr data structure.
 *
 * Sets `cl_state = 1` indicating that operations have modified the OpenCL
 * buffer and `sync_to_cpu` will be required before reading changes on the CPU.
 */
static inline void mark_gpu_modified(struct smbrr *s)
{
	s->cl_state = 1;
}

/**
 * \brief Enqueue and execute an OpenCL kernel operating on a single data buffer.
 *
 * \param k The OpenCL kernel to run.
 * \param a The target data buffer.
 */
static inline void run_kernel_1_buffer(cl_kernel k, struct smbrr *a)
{
	clSetKernelArg(k, 0, sizeof(cl_mem), &a->cl_adu);
	clSetKernelArg(k, 1, sizeof(int), &a->elems);
	size_t global_item_size = a->elems;
	clEnqueueNDRangeKernel(g_cl_ctx->command_queue, k, 1, NULL,
						   &global_item_size, NULL, 0, NULL, NULL);
	mark_gpu_modified(a);
}

/**
 * \brief Enqueue and execute an OpenCL kernel operating on a single data buffer with a single float parameter.
 *
 * \param k The OpenCL kernel to run.
 * \param a The target data buffer.
 * \param val The generic float scalar parameter passed to the kernel.
 */
static inline void run_kernel_1_buffer_float(cl_kernel k, struct smbrr *a,
											 float val)
{
	clSetKernelArg(k, 0, sizeof(cl_mem), &a->cl_adu);
	clSetKernelArg(k, 1, sizeof(float), &val);
	clSetKernelArg(k, 2, sizeof(int), &a->elems);
	size_t global_item_size = a->elems;
	clEnqueueNDRangeKernel(g_cl_ctx->command_queue, k, 1, NULL,
						   &global_item_size, NULL, 0, NULL, NULL);
	mark_gpu_modified(a);
}

/**
 * \brief Enqueue and execute an OpenCL kernel operating on a single data buffer with a single unsigned integer parameter.
 *
 * \param k The OpenCL kernel to run.
 * \param a The target data buffer.
 * \param val The generic uint32_t scalar parameter passed to the kernel.
 */
static inline void run_kernel_1_buffer_uint(cl_kernel k, struct smbrr *a,
											uint32_t val)
{
	clSetKernelArg(k, 0, sizeof(cl_mem), &a->cl_adu);
	clSetKernelArg(k, 1, sizeof(uint32_t), &val);
	clSetKernelArg(k, 2, sizeof(int), &a->elems);
	size_t global_item_size = a->elems;
	clEnqueueNDRangeKernel(g_cl_ctx->command_queue, k, 1, NULL,
						   &global_item_size, NULL, 0, NULL, NULL);
	mark_gpu_modified(a);
}

/**
 * \brief Enqueue and execute an OpenCL kernel operating across three data buffers.
 *
 * \param k The OpenCL kernel to run.
 * \param a Target Data buffer A.
 * \param b Source Data buffer B.
 * \param c Source Data buffer C.
 */
static inline void run_kernel_3_buffers(cl_kernel k, struct smbrr *a,
										struct smbrr *b, struct smbrr *c)
{
	clSetKernelArg(k, 0, sizeof(cl_mem), &a->cl_adu);
	clSetKernelArg(k, 1, sizeof(cl_mem), &b->cl_adu);
	clSetKernelArg(k, 2, sizeof(cl_mem), &c->cl_adu);
	clSetKernelArg(k, 3, sizeof(int), &a->elems);
	size_t global_item_size = a->elems;
	clEnqueueNDRangeKernel(g_cl_ctx->command_queue, k, 1, NULL,
						   &global_item_size, NULL, 0, NULL, NULL);
	mark_gpu_modified(a);
}

/**
 * \brief Enqueue and execute an OpenCL kernel operating across four data buffers.
 *
 * \param k The OpenCL kernel to run.
 * \param a Target Data buffer A.
 * \param b Source Data buffer B.
 * \param c Source Data buffer C.
 * \param s Condition mask data buffer.
 */
static inline void run_kernel_4_buffers(cl_kernel k, struct smbrr *a,
										struct smbrr *b, struct smbrr *c,
										struct smbrr *s)
{
	clSetKernelArg(k, 0, sizeof(cl_mem), &a->cl_adu);
	clSetKernelArg(k, 1, sizeof(cl_mem), &b->cl_adu);
	clSetKernelArg(k, 2, sizeof(cl_mem), &c->cl_adu);
	clSetKernelArg(k, 3, sizeof(cl_mem), &s->cl_adu);
	clSetKernelArg(k, 4, sizeof(int), &a->elems);
	size_t global_item_size = a->elems;
	clEnqueueNDRangeKernel(g_cl_ctx->command_queue, k, 1, NULL,
						   &global_item_size, NULL, 0, NULL, NULL);
	mark_gpu_modified(a);
}

/**
 * \brief Enqueue and execute an OpenCL kernel across two data buffers and a single float scalar parameter.
 *
 * \param k The OpenCL kernel to run.
 * \param dest Target Data buffer destination.
 * \param src Source Data buffer.
 * \param val Generic float scalar parameter.
 */
static inline void run_kernel_2_buffers_float(cl_kernel k, struct smbrr *dest,
											  struct smbrr *src, float val)
{
	clSetKernelArg(k, 0, sizeof(cl_mem), &dest->cl_adu);
	clSetKernelArg(k, 1, sizeof(cl_mem), &src->cl_adu);
	clSetKernelArg(k, 2, sizeof(float), &val);
	clSetKernelArg(k, 3, sizeof(int), &dest->elems);
	size_t global_item_size = dest->elems;
	clEnqueueNDRangeKernel(g_cl_ctx->command_queue, k, 1, NULL,
						   &global_item_size, NULL, 0, NULL, NULL);
	mark_gpu_modified(dest);
}

/**
 * \brief Enqueue and execute an OpenCL kernel across three data buffers and a single float scalar parameter.
 *
 * \param k The OpenCL kernel to run.
 * \param dest Target Data buffer destination.
 * \param a Source Data buffer A.
 * \param b Source Data buffer B.
 * \param c Generic float scalar parameter.
 */
static inline void run_kernel_3_buffers_float(cl_kernel k, struct smbrr *dest,
											  struct smbrr *a, struct smbrr *b,
											  float c)
{
	clSetKernelArg(k, 0, sizeof(cl_mem), &dest->cl_adu);
	clSetKernelArg(k, 1, sizeof(cl_mem), &a->cl_adu);
	clSetKernelArg(k, 2, sizeof(cl_mem), &b->cl_adu);
	clSetKernelArg(k, 3, sizeof(float), &c);
	clSetKernelArg(k, 4, sizeof(int), &dest->elems);
	size_t global_item_size = dest->elems;
	clEnqueueNDRangeKernel(g_cl_ctx->command_queue, k, 1, NULL,
						   &global_item_size, NULL, 0, NULL, NULL);
	mark_gpu_modified(dest);
}

/* Overridden Element-wise OpenCL operations */
static void cl_add(struct smbrr *a, struct smbrr *b, struct smbrr *c)
{
	run_kernel_3_buffers(g_cl_ctx->k_add, a, b, c);
}

static void cl_add_sig(struct smbrr *a, struct smbrr *b, struct smbrr *c,
					   struct smbrr *s)
{
	run_kernel_4_buffers(g_cl_ctx->k_add_sig, a, b, c, s);
}

static void cl_subtract(struct smbrr *a, struct smbrr *b, struct smbrr *c)
{
	run_kernel_3_buffers(g_cl_ctx->k_subtract, a, b, c);
}

static void cl_subtract_sig(struct smbrr *a, struct smbrr *b, struct smbrr *c,
							struct smbrr *s)
{
	run_kernel_4_buffers(g_cl_ctx->k_subtract_sig, a, b, c, s);
}

static void cl_add_value(struct smbrr *data, float value)
{
	run_kernel_1_buffer_float(g_cl_ctx->k_add_value, data, value);
}

static void cl_subtract_value(struct smbrr *data, float value)
{
	run_kernel_1_buffer_float(g_cl_ctx->k_subtract_value, data, value);
}

static void cl_mult_value(struct smbrr *data, float value)
{
	run_kernel_1_buffer_float(g_cl_ctx->k_mult_value, data, value);
}

static void cl_reset_value(struct smbrr *data, float value)
{
	run_kernel_1_buffer_float(g_cl_ctx->k_reset_value, data, value);
}

static void cl_set_sig_value(struct smbrr *data, uint32_t value)
{
	if (value == 0)
		data->sig_pixels = 0;
	else
		data->sig_pixels = data->elems;
	run_kernel_1_buffer_uint(g_cl_ctx->k_set_sig_value, data, value);
}

static void cl_set_value_sig(struct smbrr *data, struct smbrr *sdata,
							 float value)
{
	run_kernel_2_buffers_float(g_cl_ctx->k_set_value_sig, data, sdata, value);
}

static void cl_clear_negative(struct smbrr *data)
{
	run_kernel_1_buffer(g_cl_ctx->k_zero_negative, data);
}

static void cl_abs(struct smbrr *data)
{
	run_kernel_1_buffer(g_cl_ctx->k_abs, data);
}

static void cl_mult_add(struct smbrr *dest, struct smbrr *a, struct smbrr *b,
						float c)
{
	run_kernel_3_buffers_float(g_cl_ctx->k_mult_add, dest, a, b, c);
}

static void cl_mult_subtract(struct smbrr *dest, struct smbrr *a,
							 struct smbrr *b, float c)
{
	run_kernel_3_buffers_float(g_cl_ctx->k_mult_subtract, dest, a, b, c);
}

static void cl_anscombe(struct smbrr *data, float gain, float bias,
						float readout)
{
	clSetKernelArg(g_cl_ctx->k_anscombe, 0, sizeof(cl_mem), &data->cl_adu);
	clSetKernelArg(g_cl_ctx->k_anscombe, 1, sizeof(float), &gain);
	clSetKernelArg(g_cl_ctx->k_anscombe, 2, sizeof(float), &bias);
	clSetKernelArg(g_cl_ctx->k_anscombe, 3, sizeof(float), &readout);
	clSetKernelArg(g_cl_ctx->k_anscombe, 4, sizeof(int), &data->elems);
	size_t global_item_size = data->elems;
	clEnqueueNDRangeKernel(g_cl_ctx->command_queue, g_cl_ctx->k_anscombe, 1,
						   NULL, &global_item_size, NULL, 0, NULL, NULL);
	mark_gpu_modified(data);
}

static void cl_normalise(struct smbrr *data, float min, float max)
{
	clSetKernelArg(g_cl_ctx->k_normalise, 0, sizeof(cl_mem), &data->cl_adu);
	clSetKernelArg(g_cl_ctx->k_normalise, 1, sizeof(float), &min);
	clSetKernelArg(g_cl_ctx->k_normalise, 2, sizeof(float), &max);
	clSetKernelArg(g_cl_ctx->k_normalise, 3, sizeof(int), &data->elems);
	size_t global_item_size = data->elems;
	clEnqueueNDRangeKernel(g_cl_ctx->command_queue, g_cl_ctx->k_normalise, 1,
						   NULL, &global_item_size, NULL, 0, NULL, NULL);
	mark_gpu_modified(data);
}

static void cl_new_significance(struct smbrr *data, struct smbrr *sdata,
								float sigma)
{
	clSetKernelArg(g_cl_ctx->k_new_significance, 0, sizeof(cl_mem),
				   &data->cl_adu);
	clSetKernelArg(g_cl_ctx->k_new_significance, 1, sizeof(cl_mem),
				   &sdata->cl_adu);
	clSetKernelArg(g_cl_ctx->k_new_significance, 2, sizeof(float), &sigma);
	clSetKernelArg(g_cl_ctx->k_new_significance, 3, sizeof(int), &data->elems);
	size_t global_item_size = data->elems;
	clEnqueueNDRangeKernel(g_cl_ctx->command_queue,
						   g_cl_ctx->k_new_significance, 1, NULL,
						   &global_item_size, NULL, 0, NULL, NULL);
	mark_gpu_modified(sdata);

	/* Update sig_pixels count on CPU side by syncing and reading */
	sync_to_cpu(sdata);
	sdata->sig_pixels = 0;
	for (int i = 0; i < sdata->elems; i++)
		if (sdata->s[i])
			sdata->sig_pixels++;
}

static void cl_copy_sig(struct smbrr *dest, struct smbrr *src,
						struct smbrr *sig)
{
	if (!sig) {
		clEnqueueCopyBuffer(g_cl_ctx->command_queue, src->cl_adu, dest->cl_adu,
							0, 0, dest->elems * sizeof(float), 0, NULL, NULL);
	} else {
		run_kernel_3_buffers(g_cl_ctx->k_copy_sig, dest, src, sig);
	}
	mark_gpu_modified(dest);
}

/* Fallbacks for Reductions calling 1d/2d CPU ops */
static int cl_sign_data_ops_1d(struct smbrr *s, struct smbrr *n)
{
	sync_to_cpu(s);
	sync_to_cpu(n);
	return data_ops_1d.sign(s, n);
}

static int cl_sign_data_ops_2d(struct smbrr *s, struct smbrr *n)
{
	sync_to_cpu(s);
	sync_to_cpu(n);
	return data_ops_2d.sign(s, n);
}

static void cl_find_limits_data_ops_1d(struct smbrr *data, float *min,
									   float *max)
{
	sync_to_cpu(data);
	data_ops_1d.find_limits(data, min, max);
}

static void cl_find_limits_data_ops_2d(struct smbrr *data, float *min,
									   float *max)
{
	sync_to_cpu(data);
	data_ops_2d.find_limits(data, min, max);
}

static float cl_get_mean_data_ops_1d(struct smbrr *data)
{
	sync_to_cpu(data);
	return data_ops_1d.get_mean(data);
}

static float cl_get_mean_data_ops_2d(struct smbrr *data)
{
	sync_to_cpu(data);
	return data_ops_2d.get_mean(data);
}

static float cl_get_sigma_data_ops_1d(struct smbrr *data, float mean)
{
	sync_to_cpu(data);
	return data_ops_1d.get_sigma(data, mean);
}

static float cl_get_sigma_data_ops_2d(struct smbrr *data, float mean)
{
	sync_to_cpu(data);
	return data_ops_2d.get_sigma(data, mean);
}

static float cl_get_mean_sig_data_ops_1d(struct smbrr *data,
										 struct smbrr *sdata)
{
	sync_to_cpu(data);
	sync_to_cpu(sdata);
	return data_ops_1d.get_mean_sig(data, sdata);
}

static float cl_get_mean_sig_data_ops_2d(struct smbrr *data,
										 struct smbrr *sdata)
{
	sync_to_cpu(data);
	sync_to_cpu(sdata);
	return data_ops_2d.get_mean_sig(data, sdata);
}

static float cl_get_sigma_sig_data_ops_1d(struct smbrr *data,
										  struct smbrr *sdata, float mean)
{
	sync_to_cpu(data);
	sync_to_cpu(sdata);
	return data_ops_1d.get_sigma_sig(data, sdata, mean);
}

static float cl_get_sigma_sig_data_ops_2d(struct smbrr *data,
										  struct smbrr *sdata, float mean)
{
	sync_to_cpu(data);
	sync_to_cpu(sdata);
	return data_ops_2d.get_sigma_sig(data, sdata, mean);
}

static float cl_get_norm_data_ops_1d(struct smbrr *data)
{
	sync_to_cpu(data);
	return data_ops_1d.get_norm(data);
}

static float cl_get_norm_data_ops_2d(struct smbrr *data)
{
	sync_to_cpu(data);
	return data_ops_2d.get_norm(data);
}

static void cl_add_value_sig_data_ops_1d(struct smbrr *data,
										 struct smbrr *sdata, float value)
{
	sync_to_cpu(data);
	sync_to_cpu(sdata);
	data_ops_1d.add_value_sig(data, sdata, value);
}

static void cl_add_value_sig_data_ops_2d(struct smbrr *data,
										 struct smbrr *sdata, float value)
{
	sync_to_cpu(data);
	sync_to_cpu(sdata);
	data_ops_2d.add_value_sig(data, sdata, value);
}

static int cl_convert_data_ops_1d(struct smbrr *data, enum smbrr_data_type type)
{
	sync_to_cpu(data);
	return data_ops_1d.convert(data, type);
}

static int cl_convert_data_ops_2d(struct smbrr *data, enum smbrr_data_type type)
{
	sync_to_cpu(data);
	return data_ops_2d.convert(data, type);
}

static int cl_get_data_ops_1d(struct smbrr *data, enum smbrr_source_type adu,
							  void **buf)
{
	sync_to_cpu(data);
	return data_ops_1d.get(data, adu, buf);
}

static int cl_get_data_ops_2d(struct smbrr *data, enum smbrr_source_type adu,
							  void **buf)
{
	sync_to_cpu(data);
	return data_ops_2d.get(data, adu, buf);
}

static int cl_psf_data_ops_1d(struct smbrr *src, struct smbrr *dest,
							  enum smbrr_wavelet_mask mask)
{
	sync_to_cpu(src);
	return data_ops_1d.psf(src, dest, mask);
}

static int cl_psf_data_ops_2d(struct smbrr *src, struct smbrr *dest,
							  enum smbrr_wavelet_mask mask)
{
	sync_to_cpu(src);
	return data_ops_2d.psf(src, dest, mask);
}

static void cl_uchar_to_float_data_ops_1d(struct smbrr *s,
										  const unsigned char *c)
{
	sync_to_cpu(s);
	data_ops_1d.uchar_to_float(s, c);
}

static void cl_uchar_to_float_data_ops_2d(struct smbrr *s,
										  const unsigned char *c)
{
	sync_to_cpu(s);
	data_ops_2d.uchar_to_float(s, c);
}

static void cl_ushort_to_float_data_ops_1d(struct smbrr *s,
										   const unsigned short *c)
{
	sync_to_cpu(s);
	data_ops_1d.ushort_to_float(s, c);
}

static void cl_ushort_to_float_data_ops_2d(struct smbrr *s,
										   const unsigned short *c)
{
	sync_to_cpu(s);
	data_ops_2d.ushort_to_float(s, c);
}

static void cl_uint_to_float_data_ops_1d(struct smbrr *s, const unsigned int *c)
{
	sync_to_cpu(s);
	data_ops_1d.uint_to_float(s, c);
}

static void cl_uint_to_float_data_ops_2d(struct smbrr *s, const unsigned int *c)
{
	sync_to_cpu(s);
	data_ops_2d.uint_to_float(s, c);
}

static void cl_float_to_uchar_data_ops_1d(struct smbrr *s, unsigned char *c)
{
	sync_to_cpu(s);
	data_ops_1d.float_to_uchar(s, c);
}

static void cl_float_to_uchar_data_ops_2d(struct smbrr *s, unsigned char *c)
{
	sync_to_cpu(s);
	data_ops_2d.float_to_uchar(s, c);
}

static void cl_uint_to_uint_data_ops_1d(struct smbrr *s, const unsigned int *c)
{
	sync_to_cpu(s);
	data_ops_1d.uint_to_uint(s, c);
}

static void cl_uint_to_uint_data_ops_2d(struct smbrr *s, const unsigned int *c)
{
	sync_to_cpu(s);
	data_ops_2d.uint_to_uint(s, c);
}

static void cl_ushort_to_uint_data_ops_1d(struct smbrr *s,
										  const unsigned short *c)
{
	sync_to_cpu(s);
	data_ops_1d.ushort_to_uint(s, c);
}

static void cl_ushort_to_uint_data_ops_2d(struct smbrr *s,
										  const unsigned short *c)
{
	sync_to_cpu(s);
	data_ops_2d.ushort_to_uint(s, c);
}

static void cl_uchar_to_uint_data_ops_1d(struct smbrr *s,
										 const unsigned char *c)
{
	sync_to_cpu(s);
	data_ops_1d.uchar_to_uint(s, c);
}

static void cl_uchar_to_uint_data_ops_2d(struct smbrr *s,
										 const unsigned char *c)
{
	sync_to_cpu(s);
	data_ops_2d.uchar_to_uint(s, c);
}

static void cl_float_to_uint_data_ops_1d(struct smbrr *s, const float *c)
{
	sync_to_cpu(s);
	data_ops_1d.float_to_uint(s, c);
}

static void cl_float_to_uint_data_ops_2d(struct smbrr *s, const float *c)
{
	sync_to_cpu(s);
	data_ops_2d.float_to_uint(s, c);
}

static void cl_float_to_float_data_ops_1d(struct smbrr *s, const float *c)
{
	sync_to_cpu(s);
	data_ops_1d.float_to_float(s, c);
}

static void cl_float_to_float_data_ops_2d(struct smbrr *s, const float *c)
{
	sync_to_cpu(s);
	data_ops_2d.float_to_float(s, c);
}

static void cl_uint_to_uchar_data_ops_1d(struct smbrr *s, unsigned char *c)
{
	sync_to_cpu(s);
	data_ops_1d.uint_to_uchar(s, c);
}

static void cl_uint_to_uchar_data_ops_2d(struct smbrr *s, unsigned char *c)
{
	sync_to_cpu(s);
	data_ops_2d.uint_to_uchar(s, c);
}

const struct data_ops data_ops_1d_opencl = {
	.abs = cl_abs,
	.sign = cl_sign_data_ops_1d,
	.find_limits = cl_find_limits_data_ops_1d,
	.get_mean = cl_get_mean_data_ops_1d,
	.get_sigma = cl_get_sigma_data_ops_1d,
	.get_mean_sig = cl_get_mean_sig_data_ops_1d,
	.get_sigma_sig = cl_get_sigma_sig_data_ops_1d,
	.get_norm = cl_get_norm_data_ops_1d,
	.normalise = cl_normalise,
	.add = cl_add,
	.add_value_sig = cl_add_value_sig_data_ops_1d,
	.add_sig = cl_add_sig,
	.subtract = cl_subtract,
	.subtract_sig = cl_subtract_sig,
	.add_value = cl_add_value,
	.subtract_value = cl_subtract_value,
	.mult_value = cl_mult_value,
	.reset_value = cl_reset_value,
	.set_value_sig = cl_set_value_sig,
	.convert = cl_convert_data_ops_1d,
	.set_sig_value = cl_set_sig_value,
	.clear_negative = cl_clear_negative,
	.mult_add = cl_mult_add,
	.mult_subtract = cl_mult_subtract,
	.anscombe = cl_anscombe,
	.new_significance = cl_new_significance,
	.copy_sig = cl_copy_sig,
	.get = cl_get_data_ops_1d,
	.psf = cl_psf_data_ops_1d,

	/* Ignore type conversions for GPU for now, CPU fallback */
	.uchar_to_float = cl_uchar_to_float_data_ops_1d,
	.ushort_to_float = cl_ushort_to_float_data_ops_1d,
	.uint_to_float = cl_uint_to_float_data_ops_1d,
	.float_to_uchar = cl_float_to_uchar_data_ops_1d,
	.uint_to_uint = cl_uint_to_uint_data_ops_1d,
	.ushort_to_uint = cl_ushort_to_uint_data_ops_1d,
	.uchar_to_uint = cl_uchar_to_uint_data_ops_1d,
	.float_to_uint = cl_float_to_uint_data_ops_1d,
	.float_to_float = cl_float_to_float_data_ops_1d,
	.uint_to_uchar = cl_uint_to_uchar_data_ops_1d,
};

const struct data_ops data_ops_2d_opencl = {
	.abs = cl_abs,
	.sign = cl_sign_data_ops_2d,
	.find_limits = cl_find_limits_data_ops_2d,
	.get_mean = cl_get_mean_data_ops_2d,
	.get_sigma = cl_get_sigma_data_ops_2d,
	.get_mean_sig = cl_get_mean_sig_data_ops_2d,
	.get_sigma_sig = cl_get_sigma_sig_data_ops_2d,
	.get_norm = cl_get_norm_data_ops_2d,
	.normalise = cl_normalise,
	.add = cl_add,
	.add_value_sig = cl_add_value_sig_data_ops_2d,
	.add_sig = cl_add_sig,
	.subtract = cl_subtract,
	.subtract_sig = cl_subtract_sig,
	.add_value = cl_add_value,
	.subtract_value = cl_subtract_value,
	.mult_value = cl_mult_value,
	.reset_value = cl_reset_value,
	.set_value_sig = cl_set_value_sig,
	.convert = cl_convert_data_ops_2d,
	.set_sig_value = cl_set_sig_value,
	.clear_negative = cl_clear_negative,
	.mult_add = cl_mult_add,
	.mult_subtract = cl_mult_subtract,
	.anscombe = cl_anscombe,
	.new_significance = cl_new_significance,
	.copy_sig = cl_copy_sig,
	.get = cl_get_data_ops_2d,
	.psf = cl_psf_data_ops_2d,

	.uchar_to_float = cl_uchar_to_float_data_ops_2d,
	.ushort_to_float = cl_ushort_to_float_data_ops_2d,
	.uint_to_float = cl_uint_to_float_data_ops_2d,
	.float_to_uchar = cl_float_to_uchar_data_ops_2d,
	.uint_to_uint = cl_uint_to_uint_data_ops_2d,
	.ushort_to_uint = cl_ushort_to_uint_data_ops_2d,
	.uchar_to_uint = cl_uchar_to_uint_data_ops_2d,
	.float_to_uint = cl_float_to_uint_data_ops_2d,
	.float_to_float = cl_float_to_float_data_ops_2d,
	.uint_to_uchar = cl_uint_to_uchar_data_ops_2d,
};

#endif
