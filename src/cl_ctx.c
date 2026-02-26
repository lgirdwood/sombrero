/*
 *  Copyright (C) 2026 Liam Girdwood
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_OPENCL
#include "cl_ctx.h"
#include "kernels.h"

struct smbrr_cl_context *g_cl_ctx = NULL;

/**
 * \brief Print an OpenCL program build log.
 *
 * \param program The OpenCL program that failed to build.
 * \param device The target device.
 */
static void cl_compile_error(cl_program program, cl_device_id device)
{
	size_t log_size;
	clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL,
						  &log_size);
	char *log = malloc(log_size);
	clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log,
						  NULL);
	fprintf(stderr, "OpenCL Build Error: %s\n", log);
	free(log);
}

/**
 * \brief Initialize the OpenCL context and compile standard kernels.
 * \ingroup opencl
 *
 * \param device_index Index of the OpenCL device to select.
 * \return 0 on success, or -1 if OpenCL initialization failed.
 *
 * Automatically called when OpenCL support is enabled to set up the global GPU context.
 */
int smbrr_init_opencl(int device_index)
{
	cl_int err;
	cl_uint num_platforms, num_devices;
	cl_platform_id *platforms;
	cl_device_id *devices;

	if (g_cl_ctx != NULL)
		return 0; // Already initialized

	err = clGetPlatformIDs(0, NULL, &num_platforms);
	if (err != CL_SUCCESS || num_platforms == 0) {
		fprintf(
			stderr,
			"OpenCL: Failed to find any platforms (err=%d, num_platforms=%d)\n",
			err, num_platforms);
		return -1;
	}

	platforms = malloc(sizeof(cl_platform_id) * num_platforms);
	clGetPlatformIDs(num_platforms, platforms, NULL);

	// Grab first platform devices for simplicity, ideally we'd iterate
	err =
		clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices);
	if (err != CL_SUCCESS || num_devices == 0) {
		fprintf(
			stderr,
			"OpenCL: Failed to find any devices on platform 0 (err=%d, num_devices=%d)\n",
			err, num_devices);
		free(platforms);
		return -1;
	}

	devices = malloc(sizeof(cl_device_id) * num_devices);
	clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, num_devices, devices,
				   NULL);

	int idx = (device_index >= 0 && device_index < num_devices) ? device_index :
																  0;
	cl_device_id device = devices[idx];

	g_cl_ctx = calloc(1, sizeof(struct smbrr_cl_context));
	g_cl_ctx->device_id = device;

	g_cl_ctx->context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
	if (err != CL_SUCCESS) {
		fprintf(stderr, "OpenCL: Failed to create context (err=%d)\n", err);
		goto cleanup_fail;
	}

#ifdef CL_VERSION_2_0
	g_cl_ctx->command_queue = clCreateCommandQueueWithProperties(
		g_cl_ctx->context, device, NULL, &err);
#else
	g_cl_ctx->command_queue =
		clCreateCommandQueue(g_cl_ctx->context, device, 0, &err);
#endif
	if (err != CL_SUCCESS)
		goto cleanup_fail;

	size_t source_len = 0;
	g_cl_ctx->program = clCreateProgramWithSource(
		g_cl_ctx->context, 1, &opencl_source_string, NULL, &err);
	if (err != CL_SUCCESS)
		goto cleanup_fail;

	err = clBuildProgram(g_cl_ctx->program, 1, &device, NULL, NULL, NULL);
	if (err != CL_SUCCESS) {
		cl_compile_error(g_cl_ctx->program, device);
		goto cleanup_fail;
	}

	/* Cache kernels */
	g_cl_ctx->k_add = clCreateKernel(g_cl_ctx->program, "add", &err);
	g_cl_ctx->k_add_sig = clCreateKernel(g_cl_ctx->program, "add_sig", &err);
	g_cl_ctx->k_subtract = clCreateKernel(g_cl_ctx->program, "subtract", &err);
	g_cl_ctx->k_subtract_sig =
		clCreateKernel(g_cl_ctx->program, "subtract_sig", &err);
	g_cl_ctx->k_mult_value =
		clCreateKernel(g_cl_ctx->program, "mult_value", &err);
	g_cl_ctx->k_add_value =
		clCreateKernel(g_cl_ctx->program, "add_value", &err);
	g_cl_ctx->k_subtract_value =
		clCreateKernel(g_cl_ctx->program, "subtract_value", &err);
	g_cl_ctx->k_reset_value =
		clCreateKernel(g_cl_ctx->program, "reset_value", &err);
	g_cl_ctx->k_set_sig_value =
		clCreateKernel(g_cl_ctx->program, "set_sig_value", &err);
	g_cl_ctx->k_set_value_sig =
		clCreateKernel(g_cl_ctx->program, "set_value_sig", &err);
	g_cl_ctx->k_new_significance =
		clCreateKernel(g_cl_ctx->program, "new_significance", &err);
	g_cl_ctx->k_set_value =
		clCreateKernel(g_cl_ctx->program, "reset_value",
					   &err); // Fallback for k_set_value if it's used
	g_cl_ctx->k_abs = clCreateKernel(g_cl_ctx->program, "abs_k", &err);
	g_cl_ctx->k_zero_negative =
		clCreateKernel(g_cl_ctx->program, "clear_negative", &err);
	g_cl_ctx->k_normalise =
		clCreateKernel(g_cl_ctx->program, "normalise_k", &err);
	g_cl_ctx->k_anscombe =
		clCreateKernel(g_cl_ctx->program, "anscombe_k", &err);
	g_cl_ctx->k_mult_add =
		clCreateKernel(g_cl_ctx->program, "mult_add_k", &err);
	g_cl_ctx->k_mult_subtract =
		clCreateKernel(g_cl_ctx->program, "mult_subtract_k", &err);
	g_cl_ctx->k_copy_sig = clCreateKernel(g_cl_ctx->program, "copy_sig", &err);
	g_cl_ctx->k_atrous_conv_1d =
		clCreateKernel(g_cl_ctx->program, "atrous_conv_1d", &err);
	g_cl_ctx->k_atrous_conv_2d =
		clCreateKernel(g_cl_ctx->program, "atrous_conv_2d", &err);

	free(devices);
	free(platforms);
	return 0;

cleanup_fail:
	if (g_cl_ctx->program)
		clReleaseProgram(g_cl_ctx->program);
	if (g_cl_ctx->command_queue)
		clReleaseCommandQueue(g_cl_ctx->command_queue);
	if (g_cl_ctx->context)
		clReleaseContext(g_cl_ctx->context);
	free(g_cl_ctx);
	g_cl_ctx = NULL;
	free(devices);
	free(platforms);
	return -1;
}

/**
 * \brief Release all OpenCL contexts, kernels, queues, and memory.
 * \ingroup opencl
 *
 * Should be called to clean up the OpenCL state created by `smbrr_init_opencl`.
 */
void smbrr_free_opencl(void)
{
	if (!g_cl_ctx)
		return;

	if (g_cl_ctx->k_add)
		clReleaseKernel(g_cl_ctx->k_add);
	if (g_cl_ctx->k_add_sig)
		clReleaseKernel(g_cl_ctx->k_add_sig);
	if (g_cl_ctx->k_subtract)
		clReleaseKernel(g_cl_ctx->k_subtract);
	if (g_cl_ctx->k_subtract_sig)
		clReleaseKernel(g_cl_ctx->k_subtract_sig);
	if (g_cl_ctx->k_mult_value)
		clReleaseKernel(g_cl_ctx->k_mult_value);
	if (g_cl_ctx->k_add_value)
		clReleaseKernel(g_cl_ctx->k_add_value);
	if (g_cl_ctx->k_subtract_value)
		clReleaseKernel(g_cl_ctx->k_subtract_value);
	if (g_cl_ctx->k_reset_value)
		clReleaseKernel(g_cl_ctx->k_reset_value);
	if (g_cl_ctx->k_set_sig_value)
		clReleaseKernel(g_cl_ctx->k_set_sig_value);
	if (g_cl_ctx->k_set_value_sig)
		clReleaseKernel(g_cl_ctx->k_set_value_sig);
	if (g_cl_ctx->k_new_significance)
		clReleaseKernel(g_cl_ctx->k_new_significance);
	if (g_cl_ctx->k_set_value)
		clReleaseKernel(g_cl_ctx->k_set_value);
	if (g_cl_ctx->k_abs)
		clReleaseKernel(g_cl_ctx->k_abs);
	if (g_cl_ctx->k_zero_negative)
		clReleaseKernel(g_cl_ctx->k_zero_negative);
	if (g_cl_ctx->k_normalise)
		clReleaseKernel(g_cl_ctx->k_normalise);
	if (g_cl_ctx->k_anscombe)
		clReleaseKernel(g_cl_ctx->k_anscombe);
	if (g_cl_ctx->k_mult_add)
		clReleaseKernel(g_cl_ctx->k_mult_add);
	if (g_cl_ctx->k_mult_subtract)
		clReleaseKernel(g_cl_ctx->k_mult_subtract);
	if (g_cl_ctx->k_copy_sig)
		clReleaseKernel(g_cl_ctx->k_copy_sig);
	if (g_cl_ctx->k_atrous_conv_1d)
		clReleaseKernel(g_cl_ctx->k_atrous_conv_1d);
	if (g_cl_ctx->k_atrous_conv_sig_1d)
		clReleaseKernel(g_cl_ctx->k_atrous_conv_sig_1d);
	if (g_cl_ctx->k_atrous_conv_2d)
		clReleaseKernel(g_cl_ctx->k_atrous_conv_2d);
	if (g_cl_ctx->k_atrous_conv_sig_2d)
		clReleaseKernel(g_cl_ctx->k_atrous_conv_sig_2d);

	if (g_cl_ctx->program)
		clReleaseProgram(g_cl_ctx->program);
	if (g_cl_ctx->command_queue)
		clReleaseCommandQueue(g_cl_ctx->command_queue);
	if (g_cl_ctx->context)
		clReleaseContext(g_cl_ctx->context);

	free(g_cl_ctx);
	g_cl_ctx = NULL;
}

#else
int smbrr_init_opencl(int device_index)
{
	return -1;
}

void smbrr_free_opencl(void)
{
}
#endif
