/*
 *  Copyright (C) 2026 Liam Girdwood
 */

#ifndef _CL_CTX_H
#define _CL_CTX_H

#ifdef HAVE_OPENCL
#define CL_TARGET_OPENCL_VERSION 200
#include <CL/cl.h>

struct smbrr_cl_context {
	cl_device_id device_id;
	cl_context context;
	cl_command_queue command_queue;
	cl_program program;

	/* Cached kernels to avoid constant clCreateKernel overhead */
	cl_kernel k_add;
	cl_kernel k_add_sig;
	cl_kernel k_subtract;
	cl_kernel k_subtract_sig;
	cl_kernel k_mult_value;
	cl_kernel k_add_value;
	cl_kernel k_subtract_value;
	cl_kernel k_reset_value;
	cl_kernel k_set_sig_value;
	cl_kernel k_set_value_sig;
	cl_kernel k_new_significance;
	cl_kernel k_set_value;
	cl_kernel k_abs;
	cl_kernel k_zero_negative;
	cl_kernel k_find_limits;
	cl_kernel k_normalise;
	cl_kernel k_anscombe;
	cl_kernel k_mult_add;
	cl_kernel k_mult_subtract;
	cl_kernel k_copy_sig;

	cl_kernel k_atrous_conv_1d;
	cl_kernel k_atrous_conv_2d;
	cl_kernel k_atrous_conv_sig_1d;
	cl_kernel k_atrous_conv_sig_2d;

	cl_kernel k_get_mean;
	cl_kernel k_get_sigma;
};

extern struct smbrr_cl_context *g_cl_ctx;

#endif /* HAVE_OPENCL */

#endif /* _CL_CTX_H */
