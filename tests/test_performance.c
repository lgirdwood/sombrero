#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "../examples/bmp.h"
#include "../examples/fits.h"
#include "sombrero.h"

static double get_time_diff(struct timespec start, struct timespec end)
{
	return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

static int run_pipeline(const char *ifile, const char *ofile, int use_opencl,
						double *elapsed_time)
{
	struct smbrr *image, *oimage;
	struct smbrr_wavelet *w;
	struct bitmap *bmp;
	const void *data;
	int ret, width, height, stride, scales = 9;
	enum smbrr_source_type depth;
	int use_fits = 0;
	struct timespec start_time, end_time;

	if (use_opencl) {
		if (smbrr_init_opencl(0) != 0) {
			fprintf(stderr, "Failed to initialize OpenCL\n");
			return -1;
		}
	}

	if (strstr(ifile, ".fit") != NULL) {
		use_fits = 1;
		ret = fits_load(ifile, &data, &width, &height, &depth, &stride);
		if (ret < 0)
			return ret;
	} else {
		ret = bmp_load(ifile, &bmp, &data);
		if (ret < 0)
			return ret;
		height = bmp_height(bmp);
		width = bmp_width(bmp);
		depth = bmp_depth(bmp);
		stride = bmp_stride(bmp);
	}

	image = smbrr_new(SMBRR_DATA_2D_FLOAT, width, height, stride, depth, data);
	if (!image)
		return -EINVAL;

	oimage = smbrr_new(SMBRR_DATA_2D_FLOAT, width, height, stride, depth, NULL);
	if (!oimage)
		return -EINVAL;

	/* Start measuring time here */
	clock_gettime(CLOCK_MONOTONIC, &start_time);

	w = smbrr_wavelet_new(image, scales);
	if (!w)
		return -EINVAL;

	ret = smbrr_wavelet_convolution(w, SMBRR_CONV_ATROUS,
									SMBRR_WAVELET_MASK_LINEAR);
	if (ret < 0)
		return ret;

	ret = smbrr_wavelet_ksigma_clip(w, 1, 0.001);
	if (ret < 0)
		return ret;

	for (int i = 0; i < scales - 1; i++) {
		int structures = smbrr_wavelet_structure_find(w, i);
		if (structures < 0)
			return -EINVAL;
	}

	int objects = smbrr_wavelet_structure_connect(w, 0, scales - 2);
	if (objects < 0)
		return -EINVAL;

	/* End measuring time here */
	clock_gettime(CLOCK_MONOTONIC, &end_time);
	*elapsed_time = get_time_diff(start_time, end_time);

	fprintf(stdout, "Pipeline built %d objects in %s mode\n", objects,
			use_opencl ? "OpenCL" : "CPU");

	if (!use_fits)
		free(bmp);
	smbrr_wavelet_free(w);
	smbrr_free(oimage);
	smbrr_free(image);

#ifdef HAVE_OPENCL
	if (use_opencl) {
		smbrr_free_opencl();
	}
#endif

	return 0;
}

int main(int argc, char *argv[])
{
	char *ifile = NULL;
	char *ofile = NULL;
	int opt;
	double cpu_time = 0.0, gpu_time = 0.0;

	while ((opt = getopt(argc, argv, "i:o:")) != -1) {
		switch (opt) {
		case 'i':
			ifile = optarg;
			break;
		case 'o':
			ofile = optarg;
			break;
		}
	}

	if (ifile == NULL || ofile == NULL) {
		fprintf(stderr, "Usage: %s -i <input> -o <output_prefix>\n", argv[0]);
		return -EINVAL;
	}

	fprintf(stdout, "--- Running CPU Performance Test ---\n");
	if (run_pipeline(ifile, ofile, 0, &cpu_time) != 0) {
		fprintf(stderr, "CPU pipeline failed\n");
		return -1;
	}
	fprintf(stdout, "CPU pipeline time: %.4f seconds\n\n", cpu_time);

#ifdef HAVE_OPENCL
	fprintf(stdout, "--- Running OpenCL Performance Test ---\n");
	if (run_pipeline(ifile, ofile, 1, &gpu_time) != 0) {
		fprintf(stderr, "OpenCL pipeline failed or fell back\n");
	} else {
		fprintf(stdout, "OpenCL pipeline time: %.4f seconds\n\n", gpu_time);

		if (gpu_time > 0) {
			double speedup = cpu_time / gpu_time;
			fprintf(stdout, "=== SPEEDUP SUMMARY ===\n");
			fprintf(stdout, "OpenCL was %.2fx %s than CPU\n",
					speedup > 1.0 ? speedup : 1.0 / speedup,
					speedup > 1.0 ? "faster" : "slower");
		}
	}
#else
	fprintf(stdout,
			"OpenCL not compiled into this build. Skipping OpenCL test.\n");
#endif

	return 0;
}
