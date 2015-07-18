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


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "sombrero.h"
#include "local.h"
#include "ops.h"
#include "mask.h"
#include "config.h"

/* function suffixes for SIMD ops */
#ifdef OPS_SSE42
	#define OPS(a) a ## _sse42
#elif OPS_AVX
	#define OPS(a) a ## _avx
#elif OPS_AVX2
	#define OPS(a) a ## _avx2
#elif OPS_FMA
	#define OPS(a) a ## _fma
#else
	#define OPS(a) a
#endif

/* create Wi and Ci from C0 */
static void atrous_conv(struct smbrr_wavelet_1d *wavelet)
{
	int scale, scale2, length;
	float *c, *_c;

	/* scale loop */
	for (scale = 1; scale < wavelet->num_scales; scale++) {

		/* clear each scale */
		smbrr_signal_reset_value(wavelet->c[scale], 0.0);

		c = wavelet->c[scale]->adu;
		_c = wavelet->c[scale - 1]->adu;
		scale2 = 1 << (scale - 1);

		/* signal loop */
#pragma omp parallel for \
		firstprivate(c, _c, scale, scale2, wavelet) \
		schedule(static, 50)

		for (length = 0; length < wavelet->length; length++) {

			int offx, x,  xc;

			xc = wavelet->mask.width >> 1;

			/* mask x loop */
			for (x = 0; x < wavelet->mask.length; x++) {

				offx = x_boundary(wavelet->length,
					length + ((x - xc) * scale2));

				c[length] += _c[offx] * wavelet->mask.data[x];
			}
		}
	}

	/* create wavelet */
#pragma omp parallel for schedule(static, 1)
	for (scale = 1; scale < wavelet->num_scales; scale++)
		smbrr_signal_subtract(wavelet->w[scale - 1], wavelet->c[scale - 1],
				wavelet->c[scale]);

}

/* create Wi and Ci from C0 if S */
static void atrous_conv_sig(struct smbrr_wavelet_1d *wavelet)
{
	int scale,  scale2, length;
	float *c, *_c;

	/* scale loop */
	for (scale = 1; scale < wavelet->num_scales; scale++) {

		/* clear each scale */
		smbrr_signal_reset_value(wavelet->c[scale], 0.0);

		/* dont run loop if there are no sig pixels at this scale */
		if (wavelet->s[scale - 1]->sig_pixels == 0)
			continue;

		c = wavelet->c[scale]->adu;
		_c = wavelet->c[scale - 1]->adu;
		scale2 = 1 << (scale - 1);

		/* signal length loop */
#pragma omp parallel for \
		firstprivate(c, _c, scale, scale2, wavelet) \
		schedule(static, 50)

		for (length = 0; length < wavelet->length; length++) {

			int offx, x, xc;

			xc = wavelet->mask.width >> 1;

			/* mask x loop */
			for (x = 0; x < wavelet->mask.width; x++) {

				offx = x_boundary(wavelet->length,
					length + ((x - xc) * scale2));

				/* only apply wavelet if sig */
				if (!wavelet->s[scale - 1]->s[offx])
					continue;

				c[length] += _c[offx] * wavelet->mask.data[x];
			}
		}
	}
	/* create wavelet */
#pragma omp parallel for schedule(static, 1)
	for (scale = 1; scale < wavelet->num_scales; scale++)
		smbrr_signal_subtract(wavelet->w[scale - 1], wavelet->c[scale - 1],
				wavelet->c[scale]);

}

static void insert_object(struct smbrr_wavelet_1d *w,
		struct object *object, unsigned int pixel)
{
	/* insert object if none or current object at higher scale */
	if (w->object_map[pixel]  == NULL || w->object_map[pixel] == object )
		w->object_map[pixel] = object;
	else {
		/* other objects exists */
		struct object *o = w->object_map[pixel];

		/* add if new object starts at lower scale */
		if (object->start_scale < o->start_scale)
			w->object_map[pixel] = object;
	}
}

#if 0
/* C0 = C(scale - 1) + sum of wavelets; */
static void atrous_deconv_object(struct smbrr_wavelet_1d *w,
	struct smbrr_object *object)
{
	struct structure *s;
	struct object *o = (struct object *)object;
	struct smbrr_signal *signal = o->signal, *ssignal, *wsignal;
	int scale, x, y, id, ix, iy, pixel, ipixel, start, end;

	ix = object->minXy.x;
	iy = object->minxY.y;
	start = o->start_scale;
	end = o->end_scale;

	for (scale = end; scale >= start; scale --) {
		s = w->structure[object->scale] +
			o->structure[object->scale];
		id = s->id + 2;
		ssignal = w->s[scale];
		wsignal = w->w[scale];

		for (x = s->minXy.x; x <= s->maxXy.x; x++) {
			for (y = s->minxY.y; y <= s->maxxY.y; y++) {

				pixel = ssignal->width * y + x;

				if (ssignal->s[pixel] == id) {
					ipixel = signal->width * (y - iy) + (x - ix);
					signal->adu[ipixel] += wsignal->adu[pixel];
					insert_object(w, o, pixel);
				}
			}
		}
	}
}

#endif

const struct convolution1d_ops OPS(conv1d_ops) = {
	.atrous_conv = atrous_conv,
	.atrous_conv_sig = atrous_conv_sig,
//	.atrous_deconv_object = atrous_deconv_object,
};
