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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "sombrero.h"

#define	SCALES		8

struct signal {
	struct smbrr *signal;
	struct smbrr *signal_orig;
	int width;
	const char *file;
	short *data;
	FILE *in_file;
	FILE *out_file;
};

static int signal_read(struct signal *s)
{
	int size, err;

	/* open input file */
	s->in_file = fopen(s->file, "r");
	if (s->in_file == NULL)
		return -errno;

	/* psse input header for sizes */
	fseek(s->in_file, 0, SEEK_END);
	size = ftell(s->in_file);
	fseek(s->in_file, 0, SEEK_SET);

	/* read input file into buffer */
	s->data = calloc(1, size);
	if (s->data == NULL)
		return -ENOMEM;
	err = fread(s->data, size, 1, s->in_file);

	s->width = size / 2;
	/* close input file */
	fclose(s->in_file);
	return err;
}

static int signal_write(struct signal *s)
{
	char file[256];
	int err;

	/* open output file */
	sprintf(file, "%s.r", s->file);
	s->out_file = fopen(file, "w");
	if (s->out_file == NULL)
		return -errno;

	/* write output file using buffer */
	err = fwrite(s->data, s->width * 2, 1, s->out_file);

	/* close output file */
	fclose(s->out_file);
	return err;
}

int main(int argc, char *argv[])
{
	struct signal s;
	int ret;
	float mean, sigma;

	memset(&s, 0, sizeof(s));

	if (argc == 2)
		s.file = argv[1];
	else
		fprintf(stderr, "usage: %s file.wav\n", argv[0]);

	ret = signal_read(&s);
	if (ret < 0)
		exit(ret);

	s.signal_orig = smbrr_new(SMBRR_DATA_1D_FLOAT, s.width, 0, 0,
		SMBRR_SOURCE_INT16, s.data);
	if (s.signal_orig == NULL) {
		fprintf(stderr, "cant create new signal\n");
		return -EINVAL;
	}

	s.signal = smbrr_new_copy(s.signal_orig);
	if (s.signal == NULL) {
		fprintf(stderr, "cant create new signal\n");
		return -EINVAL;
	}

	mean = smbrr_get_mean(s.signal);
	sigma = smbrr_get_sigma(s.signal, mean);
	fprintf(stdout, "Signal before mean %f sigma %f\n", mean, sigma);

	smbrr_reconstruct(s.signal, SMBRR_WAVELET_MASK_LINEAR, 1.0e-4, 8,
		SMBRR_CLIP_VGENTLE);

	mean = smbrr_get_mean(s.signal);
	sigma = smbrr_get_sigma(s.signal, mean);
	fprintf(stdout, "Signal after mean %f sigma %f\n", mean, sigma);

	smbrr_get_data(s.signal, SMBRR_SOURCE_UINT16, (void **)&s.data);

	/* write to file */
	signal_write(&s);

	smbrr_free(s.signal);
	smbrr_free(s.signal_orig);

	return 0;
}
