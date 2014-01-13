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
#include "local.h"
#include "bmp.h"

/*
 * Very simple bitmap handling for the examples to avoid dependences.
 */

void bmp_info(struct bitmap *bmp)
{
	fprintf(stdout, "\nType:%hd\n", bmp->Type);
	fprintf(stdout, "Size:%d\n", bmp->Size);
	fprintf(stdout, "Reserve1:%hd\n", bmp->Reserve1);
	fprintf(stdout, "Reserve2:%hd\n", bmp->Reserve2);
	fprintf(stdout, "OffBits:%d\n", bmp->OffBits);
	fprintf(stdout, "biSize:%d\n", bmp->biSize);
	fprintf(stdout, "Width:%d\n", bmp->biWidth);
	fprintf(stdout, "Height:%d\n", bmp->biHeight);
	fprintf(stdout, "biPlanes:%hd\n", bmp->biPlanes);
	fprintf(stdout, "biBitCount:%hd\n", bmp->biBitCount);
	fprintf(stdout, "biCompression:%d\n", bmp->biCompression);
	fprintf(stdout, "biSizeImage:%d\n", bmp->biSizeImage);
	fprintf(stdout, "biXPelsPerMeter:%d\n", bmp->biXPelsPerMeter);
	fprintf(stdout, "biYPelsPerMeter:%d\n", bmp->biYPelsPerMeter);
	fprintf(stdout, "biClrUsed:%d\n", bmp->biClrUsed);
	fprintf(stdout, "biClrImportant:%d\n", bmp->biClrImportant);
	fprintf(stdout, "RedMask:%d\n", bmp->RedMask);
	fprintf(stdout, "GreenMask:%d\n", bmp->GreenMask);
	fprintf(stdout, "BlueMask:%d\n", bmp->BlueMask);
	fprintf(stdout, "AlphaMask:%d\n", bmp->AlphaMask);
	fprintf(stdout, "ColorType:%d\n", bmp->ColorType);
	fprintf(stdout, "ColorEndpoint:%d\n", bmp->ColorEndpoint);
	fprintf(stdout, "RedGamma:%d\n", bmp->RedGamma);
	fprintf(stdout, "GreenGamma:%d\n", bmp->GreenGamma);
	fprintf(stdout, "BlueGamma:%d\n", bmp->BlueGamma);
	fprintf(stdout, "Intent:%d\n", bmp->Intent);
	fprintf(stdout, "ICCProfile:%d\n", bmp->ICCProfile);
	fprintf(stdout, "ICCSize:%d\n", bmp->ICCSize);
	fprintf(stdout, "Reserved:%d\n", bmp->Reserved);
}

int bmp_width(struct bitmap *bmp)
{
	return bmp->biWidth;
}

int bmp_height(struct bitmap *bmp)
{
	return bmp->biHeight;
}

int bmp_stride(struct bitmap *bmp)
{
	return bmp->biSizeImage / bmp->biHeight;
}

enum smbrr_adu bmp_depth(struct bitmap *bmp)
{
	switch (bmp->biClrUsed) {
	case 256:
		return SMBRR_ADU_8;
	default:
		fprintf(stderr, "%d colours not supported yet\n", bmp->biClrUsed);
		return -EINVAL;
	}
}

static void bmp_cmap(uint32_t *cmap, int size)
{
	int i;

	for (i = 0; i < size; i++)
		cmap[i] = (i << 16) + (i << 8) + i;
}

int bmp_save(char *file, struct bitmap *bmp, const void *data)
{
	FILE *BMPFile;
	uint32_t cmap[256];

	fprintf(stdout, "saving %s\n", file);

	BMPFile = fopen(file, "w");

	if (BMPFile == NULL) {
		fprintf(stderr, "failed to open %s\n", file);
		return -ENOENT;
	}

	//bmp_info(bmp);
	bmp_cmap(cmap, 256);
	fwrite(bmp, sizeof(*bmp), 1, BMPFile);
	fwrite(cmap, bmp->OffBits - sizeof(*bmp), 1, BMPFile);
	fwrite(data, bmp->Size - bmp->OffBits, 1, BMPFile);
	fclose(BMPFile);

	return 0;
}

int bmp_image_save(struct smbrr_image *image, struct bitmap *bmp,
	const char *file)
{
	struct smbrr_image *i;
	char filename[128];
	void *buf;
	float min, max;

	i = smbrr_image_new(SMBRR_IMAGE_FLOAT, image->width, image->height,
		0, 0, NULL);
	if (i == NULL)
		return -ENOMEM;
	smbrr_image_copy(i, image);

	buf = calloc(1, smbrr_image_bytes(i));
	if (buf == NULL)
		return -EINVAL;

	sprintf(filename, "%s.bmp", file);
	smbrr_image_convert(i, SMBRR_IMAGE_FLOAT);
	smbrr_image_find_limits(i, &min, &max);
	fprintf(stdout, "limit for %s are %f to %f\n", filename, min, max);
	smbrr_image_normalise(i, 0.0, 250.0);
	smbrr_image_get(i, SMBRR_ADU_8, &buf);
	bmp_save(filename, bmp, buf);
	free(buf);
	smbrr_image_free(i);

	return 0;
}

int bmp_load(char *file, struct bitmap **bmp, const void **data)
{
	FILE *BMPFile;
	struct bitmap *_bmp;
	int ret = -EINVAL, size;
	size_t items;

	fprintf(stdout, "loading %s\n", file);

	BMPFile = fopen(file, "rb");
	if (BMPFile == NULL) {
		fprintf(stderr, "failed to open %s\n", file);
		return -ENOENT;
	}

	_bmp = malloc(sizeof(*_bmp));
	if (_bmp == NULL) {
		fclose(BMPFile);
		return -ENOMEM;
	}
	items = fread(_bmp, sizeof(*_bmp), 1, BMPFile);
	if (items != 1)
		goto out;
	//bmp_info(_bmp);

	/* alloc buffer and colour map */
	size = _bmp->Size;
	_bmp = realloc(_bmp, size);

	fseek(BMPFile, 0, SEEK_SET);
	ret = fread(_bmp, size, 1, BMPFile);
	if (ret < 0) {
		fprintf(stderr, "failed to read image %s: %d\n", file, ret);
		free(_bmp);
	}

	*data = ((void*)_bmp) +_bmp->OffBits;
	*bmp = _bmp;

out:
	fclose(BMPFile);
	return ret;
}
