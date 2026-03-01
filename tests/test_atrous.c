#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Added for strstr

#include "../examples/bmp.h"
#include "../examples/fits.h"
#include "sombrero.h"

int main(int argc, char *argv[])
{
	struct smbrr *image, *simage, *wimage, *oimage; // Added oimage
	struct smbrr_wavelet *w;
	struct bitmap *bmp = NULL; // Initialize bmp to NULL
	const void *data;
	int ret, width, height, stride, scales = 9;
	enum smbrr_source_type depth;
	char *ifile = NULL, *ofile = NULL;
	char outfile[64]; // Added outfile
	int use_fits = 0; // Added use_fits
	float mean, sigma;

	/* Expected values from examples/atrous on wiz-ha-x.bmp */
	float expected_scale_mean[] = { 0.570, 0.570, 0.570, 0.570, 0.570,
									0.570, 0.570, 0.570, 0.571 };
	float expected_scale_sigma[] = { 1.655, 1.394, 1.026, 0.685, 0.498,
									 0.422, 0.386, 0.362, 0.324 };
	float expected_wavelet_mean[] = { 0.000, 0.000, 0.000, 0.000,
									  0.000, 0.000, 0.000, 0.000 };
	float expected_wavelet_sigma[] = { 0.488, 0.581, 0.551, 0.363,
									   0.197, 0.120, 0.077, 0.069 };

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <input.bmp> <output_prefix>\n", argv[0]);
		return -EINVAL;
	}

	ifile = argv[1];
	ofile = argv[2];

#ifdef HAVE_OPENCL
	int cl_err = smbrr_init_opencl(0);
	if (cl_err == 0) {
		fprintf(stdout, "OpenCL initialized successfully.\n");
	} else {
		fprintf(stderr, "OpenCL initialization failed, falling back to CPU.\n");
	}
#endif

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

	if (use_fits) {
		fprintf(stdout, "FITS loaded: width %d height %d stride %d depth %d\n",
				width, height, stride, depth);
	}

	image = smbrr_new(SMBRR_DATA_2D_FLOAT, width, height, stride, depth, data);
	if (image == NULL) {
		return -EINVAL;
	}

	oimage = smbrr_new(SMBRR_DATA_2D_FLOAT, width, height, stride, depth,
					   NULL); // Initialize oimage
	if (oimage == NULL) {
		smbrr_free(image);
		return -EINVAL;
	}

	w = smbrr_wavelet_new(image, scales);
	if (w == NULL) {
		smbrr_free(image);
		smbrr_free(oimage); // Free oimage on error
		return -EINVAL;
	}

	ret = smbrr_wavelet_convolution(w, SMBRR_CONV_ATROUS,
									SMBRR_WAVELET_MASK_LINEAR);
	if (ret < 0) {
		smbrr_wavelet_free(w);
		smbrr_free(image);
		smbrr_free(oimage); // Free oimage on error
		return ret;
	}

	for (int i = 0; i < scales; i++) {
		simage = smbrr_wavelet_get_scale(w, i);
		sprintf(outfile, "%s-scale-%d", ofile, i);
		if (use_fits)
			fits_image_save(simage, outfile);
		else
			bmp_image_save(simage, bmp, outfile);

		mean = smbrr_get_mean(simage);
		sigma = smbrr_get_sigma(simage, mean);
		fprintf(stdout, "scale %d mean %3.3f sigma %3.3f\n", i, mean, sigma);

		/* Validate scale values within a reasonable tolerance */
		if (!use_fits && (fabs(mean - expected_scale_mean[i]) > 0.002 ||
						  fabs(sigma - expected_scale_sigma[i]) > 0.002)) {
			fprintf(stderr, "Scale %d validation failed!\n", i);
			smbrr_wavelet_free(w);
			smbrr_free(image);
			smbrr_free(oimage); // Free oimage on error
			return -EINVAL;
		}

		if (i < scales - 1) {
			/* save each wavelet layer for visualisation */
			wimage = smbrr_wavelet_get_wavelet(w, i);
			smbrr_set_value(oimage, 0.0);
			smbrr_add_value(oimage, 127.0); /* this brings out the detail */
			smbrr_add(oimage, oimage, wimage);
			sprintf(outfile, "%s-wavelet-%d", ofile, i);
			if (use_fits)
				fits_image_save(oimage, outfile);
			else
				bmp_image_save(oimage, bmp, outfile);

			mean = smbrr_get_mean(wimage);
			sigma = smbrr_get_sigma(wimage, mean);
			fprintf(stdout, "wavelet %d mean %3.3f sigma %3.3f\n", i, mean,
					sigma);

			/* Validate wavelet values */
			if (!use_fits &&
				(fabs(mean - expected_wavelet_mean[i]) > 0.002 ||
				 fabs(sigma - expected_wavelet_sigma[i]) > 0.002)) {
				fprintf(stderr, "Wavelet %d validation failed!\n", i);
				return -EINVAL;
			}
		}
	}

	free(bmp);
	smbrr_wavelet_free(w);
	smbrr_free(image);

	return 0;
}
