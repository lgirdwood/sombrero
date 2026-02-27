/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (C) 2026 Liam Girdwood <lgirdwood@gmail.com>
 *
 */

#include <errno.h> // IWYU pragma: keep
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bmp.h"
#include "fits.h"
#include "sombrero.h"

static void usage(char *argv[])
{
	fprintf(stdout,
			"Usage:%s [-k clip strength] [-s sigma delta]"
			"[-A gain strength] [-S scales] -i infile.bmp -o outfile\n",
			argv[0]);
	fprintf(stdout, "Generic options\n");
	fprintf(stdout, " -i Input bitmap file - only greyscale supported\n");
	fprintf(stdout, " -o Output file name\n");
	fprintf(stdout, "Wavelet options\n");
	fprintf(stdout,
			" -k K-Sigma clip strength. Default 1. Values 0 .. 5 (gentle "
			"-> strong)\n");
	fprintf(stdout,
			" -A Gain strength. Default 0. Values 0 .. 4 (low .. high freq)\n");
	fprintf(stdout, " -S Number of scales to process. Default and max 9\n");
	exit(0);
}

int main(int argc, char *argv[])
{
	struct smbrr *image, *simage, *wimage;
	struct smbrr_wavelet *w;
	struct bitmap *bmp = NULL;
	const void *data;
	int ret, width, height, stride, i, opt, k = 1, a = 0, scales = 9;
	enum smbrr_source_type depth;
	float sigma, mean;
	char outfile[64], *ifile = NULL, *ofile = NULL;
	int use_fits = 0;

	/* Parse command line arguments */
	while ((opt = getopt(argc, argv, "i:k:s:A:S:o:")) != -1) {
		switch (opt) {
		case 'i':
			ifile = optarg;
			break;
		case 'o':
			ofile = optarg;
			break;
		case 'k':
			k = atoi(optarg);
			if (k < 0 || k > 5)
				usage(argv);
			break;
		case 'A':
			a = atoi(optarg);
			if (a < 0 || a > 4)
				usage(argv);
			break;
		case 'S':
			scales = atoi(optarg);
			if (scales < 1 || scales > SMBRR_MAX_SCALES)
				usage(argv);
			break;
		default: /* '?' */
			usage(argv);
		}
	}

	if (ifile == NULL || ofile == NULL)
		usage(argv);

	/* Remove extension from output file name */
	char *ext = strrchr(ofile, '.');
	if (ext && (strcmp(ext, ".bmp") == 0 || strcmp(ext, ".fit") == 0 ||
				strcmp(ext, ".fits") == 0))
		*ext = '\0';

	/* Load input image from FITS or BMP file */
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
	fprintf(stdout, "Image width %d height %d stride %d\n", width, height,
			stride);

	/* Create an internal libsmbrr 2D image object from the loaded data */
	image = smbrr_new(SMBRR_DATA_2D_FLOAT, width, height, stride, depth, data);
	if (image == NULL) {
		fprintf(stderr, "can't create new image\n");
		return -EINVAL;
	}

	/* Create a new wavelet transform computing context */
	w = smbrr_wavelet_new(image, scales);
	if (w == NULL) {
		fprintf(stderr, "can't create new wavelet\n");
		return -EINVAL;
	}

	/* Perform a'trous wavelet convolution on the image */
	ret = smbrr_wavelet_convolution(w, SMBRR_CONV_ATROUS,
									SMBRR_WAVELET_MASK_LINEAR);
	if (ret < 0) {
		fprintf(stderr, "wavelet convolution failed\n");
		return ret;
	}

	// oimage is no longer used in the new logic.

	/* Process and save each wavelet scale and its corresponding detail (wavelet) */
	for (i = 0; i < scales; i++) {
		/* Get the lower-resolution scale image memory reference for this level */
		simage = smbrr_wavelet_get_scale(w, i);
		/* Get the detail (wavelet) coefficients memory reference for this level */
		wimage = smbrr_wavelet_get_wavelet(w, i);

		/* Calculate statistics and save the scale image */
		mean = smbrr_get_mean(simage);
		sigma = smbrr_get_sigma(simage, mean);

		fprintf(stdout, "scale %d mean %3.3f sigma %3.3f\n", i, mean, sigma);
		sprintf(outfile, "%s-scale-%d", ofile, i);
		if (use_fits)
			fits_image_save(simage, outfile);
		else
			bmp_image_save(simage, bmp, outfile);

		/* The last scale contains only the background image and no corresponding detail wavelet */
		if (i < scales - 1) {
			/* Calculate statistics and save the wavelet (detail) image */
			mean = smbrr_get_mean(wimage);
			sigma = smbrr_get_sigma(wimage, mean);
			fprintf(stdout, "wavelet %d mean %3.3f sigma %3.3f\n", i, mean,
					sigma);

			sprintf(outfile, "%s-aw-%d", ofile, i);
			if (use_fits)
				fits_image_save(wimage, outfile);
			else
				bmp_image_save(wimage, bmp, outfile);
		}
	}

	/* Clean up and free allocated resources */
	if (bmp)
		free(bmp);
	smbrr_wavelet_free(w);
	smbrr_free(image);

	return 0;
}

/* Trigger IDE re-parse */
