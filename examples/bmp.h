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
 * auint32_t with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (C) 2026 Liam Girdwood <lgirdwood@gmail.com>
 *
 */

#ifndef _BMP_H
#define _BMP_H

#include <sys/types.h>
#include <stdint.h>

#include <sombrero.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

/* bitmap file header */
struct bitmap {
	uint16_t Type;
	uint32_t Size;
	uint16_t Reserve1;
	uint16_t Reserve2;
	uint32_t OffBits;
	uint32_t biSize;
	uint32_t biWidth;
	uint32_t biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	uint32_t biXPelsPerMeter;
	uint32_t biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
	uint32_t RedMask;
	uint32_t GreenMask;
	uint32_t BlueMask;
	uint32_t AlphaMask;
	uint32_t ColorType;
	uint32_t ColorEndpoint;
	uint32_t RedGamma;
	uint32_t GreenGamma;
	uint32_t BlueGamma;
	uint32_t Intent;
	uint32_t ICCProfile;
	uint32_t ICCSize;
	uint32_t Reserved;
} __attribute__((packed));

int bmp_width(struct bitmap *bmp);
int bmp_height(struct bitmap *bmp);
enum smbrr_source_type bmp_depth(struct bitmap *bmp);
int bmp_stride(struct bitmap *bmp);

void bmp_info(struct bitmap *Header);
int bmp_save(const char *file, struct bitmap *bmp, const void *data);
int bmp_load(const char *file, struct bitmap **bmp, const void **data);
int bmp_image_save(struct smbrr *image, struct bitmap *bmp, const char *file);

#endif
#endif
