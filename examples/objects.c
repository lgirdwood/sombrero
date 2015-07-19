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
 * Copyright (C) 2012 Liam Girdwood <lgirdwood@gmail.com>
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <sys/time.h>

#include "sombrero.h"
#include "bmp.h"
#include "debug.h"

static void usage(char *argv[])
{
	fprintf(stdout, "Usage:%s [-g gain] [-b bias] [-r readout]"
			" [-a] [-k clip strength] [-s sigma delta] [-A gain strength]"
			" [-S scales] -i infile.bmp -o outfile\n", argv[0]);
	fprintf(stdout, "Generic options\n");
	fprintf(stdout, " -i Input bitmap file - only greyscale supported\n");
	fprintf(stdout, " -o Output file name\n");
	fprintf(stdout, " -t Time execution\n");
	fprintf(stdout, "Wavelet options\n");
	fprintf(stdout, " -k K-Sigma clip strength. Default 1. Values 0 .. 5 (gentle -> strong)\n");
	fprintf(stdout, " -A Gain strength. Default 0. Values 0 .. 4 (low .. high freq)\n");
	fprintf(stdout, " -s Sigma delta. Default 0.001\n");
	fprintf(stdout, " -S Number of scales to process. Default and max 9\n");
	fprintf(stdout, "CCD options\n");
	fprintf(stdout, " -a Enable Anscombe transform using -g -b -r below\n");
	fprintf(stdout, " -g CCD amplifier gain in photo-electrons per ADU. Default 5.0\n");
	fprintf(stdout, " -b Image bias in ADUs. Default 50.0\n");
	fprintf(stdout, " -r Readout noise in RMS electrons. Default 100.0\n");
	exit(0);
}

static struct timeval start, end;

inline static void start_timer(int time)
{
	if (time)
		gettimeofday(&start, NULL);
}

static void print_timer(int time, const char *text)
{
	double secs;

	if (!time)
		return;

	gettimeofday(&end, NULL);
	secs = ((end.tv_sec * 1000000 + end.tv_usec) -
		(start.tv_sec * 1000000 + start.tv_usec)) / 1000000.0;

	fprintf(stdout, "Time for %s %3.1f msecs\n", text, secs * 1000.0);
	start_timer(1);
}


int main(int argc, char *argv[])
{
	struct smbrr_wavelet *w;
	struct smbrr_object *object;
	struct smbrr *image, *simage, *oimage;
	struct bitmap *bmp;
	const void *data;
	int ret, width, height, stride, i, opt, anscombe = 0, k = 1,
			a = 0, scales = 9, structures, objects, time = 0;
	enum smbrr_adu depth;
	float gain = 5.0, bias = 50.0, readout = 100.0, sigma_delta = 0.001;
	char *ifile = NULL, *ofile = NULL;
	char outfile[64];

	while ((opt = getopt(argc, argv, "g:b:r:i:atk:s:A:S:o:")) != -1) {
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
		case 't':
			time = 1;
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
			if (scales < 1 || scales > SMBRR_MAX_SCALES - 1)
				usage(argv);
			break;
		default: /* '?' */
			usage(argv);
		}
	}

	if (ifile == NULL || ofile == NULL)
		usage(argv);

	ret = bmp_load(ifile, &bmp, &data);
	if (ret < 0)
		return ret;

	height = bmp_height(bmp);
	width = bmp_width(bmp);
	depth = bmp_depth(bmp);
	stride = bmp_stride(bmp);
	fprintf(stdout, "Image width %d height %d stride %d\n",
		width, height, stride);

	start_timer(time);

	image = smbrr_new(SMBRR_DATA_2D_FLOAT, width, height, stride,
		depth, data);
	if (image == NULL)
		return -EINVAL;

	oimage = smbrr_new(SMBRR_DATA_2D_FLOAT, width, height, stride,
			depth, NULL);
		if (oimage == NULL)
			return -EINVAL;

	print_timer(time, "image new");

	if (anscombe) {
		/* perform Anscombe transform on input image */
		fprintf(stdout, "Performing Anscombe transform with "
			"gain = %3.3f, bias = %3.3f, readout = %3.3f\n",
			gain, bias, readout);

		smbrr_anscombe(image, gain, bias, readout);
	}

	print_timer(time, "anscombe");

	w = smbrr_wavelet_new(image, scales);
	if (w == NULL)
		return -EINVAL;

	print_timer(time, "wavelet new");

	ret = smbrr_wavelet_convolution(w, SMBRR_CONV_ATROUS,
		SMBRR_WAVELET_MASK_LINEAR);
	if (ret < 0)
		return ret;

	print_timer(time, "wavelet_convolution");

	fprintf(stdout, "Using K sigma strength %d delta %f\n", k, sigma_delta);
	smbrr_wavelet_ksigma_clip(w, k, sigma_delta);

	print_timer(time, "ksigma clip");

	for (i = 0; i < scales - 1; i++) {
		structures = smbrr_wavelet_structure_find(w, i);
		print_timer(time, "find structures");

		fprintf(stdout, "Found %d structures at scale %d\n", structures, i);

		/* save each structure scale for visualisation */
		simage = smbrr_wavelet_get_data_significant(w, i);
		smbrr_reset_value(oimage, 0.0);
		smbrr_significant_set_value(oimage, simage, 1);
		sprintf(outfile, "%s-struct-%d", ofile, i);
		bmp_image_save(oimage, bmp, outfile);

		print_timer(time, "image save");
	}

	/* connect structures */
	objects = smbrr_wavelet_structure_connect(w, 0, scales - 2);
	fprintf(stdout,"Found %d objects\n", objects);
	print_timer(time, "connect objects");

	for (i = 0; i < objects; i++) {
		object = smbrr_wavelet_object_get(w, i);
		fprintf(stdout, "object %d ID %d\n"
				" Total ADU %f Mean %f Sigma %f Scale %d Mag delta %f\n"
				" Position %d:%d Area %d\n", i, object->id,
				object->object_adu, object->mean_adu, object->sigma_adu,
				object->scale, object->mag_delta,
				object->pos.x, object->pos.y, object->object_area);

		/* dump 10 brightest object images */
		if (i < 10) {
			struct smbrr *image;

			smbrr_wavelet_object_get_data(w, object, &image);
			if (image)
				smbrr_image_dump(image, "%s-object-%d", ofile, i);
		}
	}

	free(bmp);
	smbrr_wavelet_free(w);
	smbrr_free(oimage);
	smbrr_free(image);
	return 0;
}
