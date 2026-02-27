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
			"Usage:%s [-g gain] [-b bias] [-r readout]"
			" [-a] [-k clip strength] [-s sigma delta] [-A gain strength]"
			" [-S scales] -i infile.bmp -o outfile\n",
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
	fprintf(stdout, " -s Sigma delta. Default 0.001\n");
	fprintf(stdout, " -S Number of scales to process. Default and max 9\n");
	fprintf(stdout, "CCD options\n");
	fprintf(stdout, " -a Enable Anscombe transform using -g -b -r below\n");
	fprintf(stdout,
			" -g CCD amplifier gain in photo-electrons per ADU. Default 5.0\n");
	fprintf(stdout, " -b Image bias in ADUs. Default 50.0\n");
	fprintf(stdout, " -r Readout noise in RMS electrons. Default 100.0\n");
	exit(0);
}

int main(int argc, char *argv[])
{
	struct smbrr_wavelet *w;
	struct smbrr *image, *oimage, *simage;
	struct bitmap *bmp;
	const void *data;
	int ret, width, height, stride, i, opt, anscombe = 0, k = 1, a = 0,
											scales = 9;
	enum smbrr_source_type depth;
	float gain = 5.0, bias = 50.0, readout = 100.0, sigma_delta = 0.001;
	char *ifile = NULL, *ofile = NULL;
	char outfile[64];
	int use_fits = 0;

	/* Parse command line arguments */
	while ((opt = getopt(argc, argv, "g:b:r:i:ak:s:A:S:o:")) != -1) {
		switch (opt) {
		case 'g':
			gain = atof(optarg);
			break;
		case 'b':
			bias = atof(optarg);
			break;
		case 'r':
			readout = atof(optarg);
			break;
		case 'i':
			ifile = optarg;
			break;
		case 'o':
			ofile = optarg;
			break;
		case 'a':
			anscombe = 1;
			break;
		case 'k':
			k = atoi(optarg);
			if (k < 0 || k > 5)
				usage(argv);
			break;
		case 's':
			sigma_delta = atof(optarg);
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
	if (image == NULL)
		return -EINVAL;

	/* Create an empty image object for storing our output */
	oimage = smbrr_new(SMBRR_DATA_2D_FLOAT, width, height, stride, depth, NULL);
	if (oimage == NULL)
		return -EINVAL;

	if (anscombe) {
		/* Perform Anscombe transform on input image to stabilize variance */
		fprintf(stdout,
				"Performing Anscombe transform with "
				"gain = %3.3f, bias = %3.3f, readout = %3.3f\n",
				gain, bias, readout);

		smbrr_anscombe(image, gain, bias, readout);
	}

	/* Create a new wavelet transform computing context */
	w = smbrr_wavelet_new(image, scales);
	if (w == NULL)
		return -EINVAL;

	/* Perform a'trous wavelet convolution on the image */
	ret = smbrr_wavelet_convolution(w, SMBRR_CONV_ATROUS,
									SMBRR_WAVELET_MASK_LINEAR);
	if (ret < 0)
		return ret;

	fprintf(stdout, "Using K sigma strength %d delta %f\n", k, sigma_delta);
	/* Perform K-sigma clipping to exclude noise and identify significant structures */
	smbrr_wavelet_ksigma_clip(w, k, sigma_delta);

	/* Deconvolve the wavelet by removing insignificant structures based on the clipping */
	ret = smbrr_wavelet_significant_deconvolution(w, SMBRR_CONV_ATROUS,
												  SMBRR_WAVELET_MASK_LINEAR, a);
	if (ret < 0)
		return ret;
	/* Save the resulting image containing only the significant structures (noise removed) */
	sprintf(outfile, "%s-ksigma", ofile);
	if (use_fits)
		fits_image_save(image, outfile);
	else
		bmp_image_save(image, bmp, outfile);

	for (i = 0; i < scales - 1; i++) {
		/* Combine each significant scale into a single output image for visualisation */
		simage = smbrr_wavelet_get_significant(w, i);
		smbrr_significant_add_value(oimage, simage,
									16 + (1 << ((scales - 1) - i)));
	}

	/* Save the combined significant structures image */
	sprintf(outfile, "%s-sigall", ofile);
	if (use_fits)
		fits_image_save(oimage, outfile);
	else
		bmp_image_save(oimage, bmp, outfile);

	for (i = 0; i < scales - 1; i++) {
		/* Extract and save each individual significant scale separately */
		simage = smbrr_wavelet_get_significant(w, i);
		smbrr_set_value(oimage, 0.0);
		smbrr_significant_set_value(oimage, simage, 127);
		sprintf(outfile, "%s-sig-%d", ofile, i);
		if (use_fits)
			fits_image_save(oimage, outfile);
		else
			bmp_image_save(oimage, bmp, outfile);
	}

	/* Clean up and free allocated resources */
	if (!use_fits)
		free(bmp);
	smbrr_wavelet_free(w);
	smbrr_free(oimage);
	smbrr_free(image);
	return 0;
}
