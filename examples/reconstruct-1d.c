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

struct wav_hdr {
	char buf[44];		/* tmp hack for wav header */
};

struct audio_recon {
	struct smbrr *signal;
	struct smbrr *signal_orig;
	int width;
	const char *file;
	struct wav_hdr wav_hdr;
	short *data;
	FILE *in_file;
	FILE *out_file;
};

static int wav_read(struct audio_recon *ar)
{
	int size, err;

	/* open input file */
	ar->in_file = fopen(ar->file, "r");
	if (ar->in_file == NULL)
		return -errno;

	/* parse input header for sizes */
	fseek(ar->in_file, 0, SEEK_END);
	size = ftell(ar->in_file);
	fseek(ar->in_file, 0, SEEK_SET);

	err = fread(&ar->wav_hdr, 44, 1, ar->in_file);

	/* read input file into buffer */
	ar->data = calloc(1, size);
	if (ar->data == NULL)
		return -ENOMEM;
	err = fread(ar->data, size - 44, 1, ar->in_file);

	ar->width = size / 2;
	/* close input file */
	fclose(ar->in_file);
	return err;
}

static int wav_write(struct audio_recon *ar)
{
	char file[256];
	int err;

	/* open output file */
	sprintf(file, "%s.r", ar->file);
	ar->out_file = fopen(file, "w");
	if (ar->out_file == NULL)
		return -errno;

	/* reuse input file wav header */
	err = fwrite(&ar->wav_hdr, 44, 1, ar->out_file);

	/* write output file using buffer */
	err = fwrite(ar->data, ar->width * 2, 1, ar->out_file);

	/* close output file */
	fclose(ar->out_file);
	return err;
}

int main(int argc, char *argv[])
{
	struct audio_recon ar;
	int ret;
	float mean, sigma;

	memset(&ar, 0, sizeof(ar));

	if (argc == 2)
		ar.file = argv[1];
	else
		fprintf(stderr, "usage: %s file.wav\n", argv[0]);

	ret = wav_read(&ar);
	if (ret < 0)
		exit(ret);

	ar.signal_orig = smbrr_new(SMBRR_DATA_1D_FLOAT, ar.width, 0, 0,
		SMBRR_SOURCE_UINT16, ar.data);
	if (ar.signal_orig == NULL) {
		fprintf(stderr, "cant create new signal\n");
		return -EINVAL;
	}

	ar.signal = smbrr_new_copy(ar.signal_orig);
	if (ar.signal == NULL) {
		fprintf(stderr, "cant create new signal\n");
		return -EINVAL;
	}

	/* convert audio signal to absolute values */
	smbrr_abs(ar.signal);

	mean = smbrr_get_mean(ar.signal);
	sigma = smbrr_get_sigma(ar.signal, mean);
	fprintf(stdout, "Signal before mean %f sigma %f\n", mean, sigma);

	smbrr_reconstruct(ar.signal, SMBRR_WAVELET_MASK_LINEAR, 1.0e-4, 8,
		SMBRR_CLIP_VGENTLE);

	mean = smbrr_get_mean(ar.signal);
	sigma = smbrr_get_sigma(ar.signal, mean);
	fprintf(stdout, "Signal after mean %f sigma %f\n", mean, sigma);

	/* covert reconstructed signal back to signed values */
	smbrr_signed(ar.signal, ar.signal_orig);

	smbrr_get_data(ar.signal, SMBRR_SOURCE_UINT16, (void **)&ar.data);

	/* write to file */
	wav_write(&ar);

	smbrr_free(ar.signal);
	smbrr_free(ar.signal_orig);

	return 0;
}
