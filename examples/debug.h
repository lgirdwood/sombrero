/*
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  Copyright (C) 2012 Liam Girdwood
 */

#ifndef __SMBRR_DEBUG_H
#define __SMBRR_DEBUG_H

#include <stdio.h>
#include <stdarg.h>
#include "local.h"
#include "bmp.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

static inline void smbrr_image_dump_(struct smbrr_image *image,
	const char *fmt, ...)
{
	va_list va;
	struct bitmap bmp;
	char filename[128];

	memset(&bmp, 0, sizeof(bmp));

	bmp.Type = 19778;
	bmp.OffBits = 1146;
	bmp.biSize = 108;
	bmp.biWidth = image->width;
	bmp.biHeight = image->height;
	bmp.biPlanes = 1;
	bmp.biBitCount = 8;
	bmp.biSizeImage = image->height * image->stride;
	bmp.Size = bmp.biSizeImage + bmp.OffBits;
	bmp.biClrUsed = 256;
	bmp.biClrImportant = 256;
	bmp.biXPelsPerMeter = 2835;
	bmp.biYPelsPerMeter = 2835;

	va_start(va, fmt);
	vsprintf(filename, fmt, va);
	va_end(va);

	bmp_image_save(image, &bmp, filename);
}

static inline void smbrrerr_(const char *file, const char *func, int line,
	const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	fprintf(stderr, "%s:%s:%d:  ", file, func, line);
	vfprintf(stderr, fmt, va);
	va_end(va);
}

#define smbrr_error(format, arg...) \
	smbrrerr_(__FILE__, __func__, __LINE__, format, ## arg)

#define smbrr_image_dump(image, format, arg...) \
		smbrr_image_dump_(image, format, ## arg)

#define smbrr_wavelet_dump_scale(wavelet, scale, format, arg...) \
		smbrr_image_dump_(wavelet->c[scale], format, ## arg)

#endif
#endif
