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

#include "config.h"
#include "sombrero.h"
#include "local.h"
#include "ops.h"
#include "mask.h"
#include "config.h"

static void set_signal_ops(struct smbrr_signal *signal)
{
	unsigned int cpu_flags = cpu_get_flags();

#if defined HAVE_FMA
	if (cpu_flags & CPU_X86_FMA) {
		signal->ops = &signal_ops_fma;
		return;
	}
#endif
#if defined HAVE_AVX2
	if (cpu_flags & CPU_X86_AVX2) {
		signal->ops = &signal_ops_avx2;
		return;
	}
#endif
#if defined HAVE_AVX
	if (cpu_flags & CPU_X86_AVX) {
		signal->ops = &signal_ops_avx;
		return;
	}
#endif
#if defined HAVE_SSE42
	if (cpu_flags & CPU_X86_SSE4_2) {
		signal->ops = &signal_ops_sse42;
		return;
	}
#endif

	/* default C implementation */
	signal->ops = &signal_ops;
}

/*! \fn struct smbrr_signal *smbrr_signal_new(enum smbrr_signal_type type,
	unsigned int width, unsigned int height, unsigned int stride,
	enum smbrr_adu adu, const void *src_signal);
* \param type New signal type
* \param width Image width in pixels
* \param height Image height in pixels
* \param stride Image stride in pixels. width += %4
* \param adu Source data ADU type
* \param src_signal source signal raw data
* \return Pointer to new signal or NULL on failure
*
* Create a new smbrr signal from source raw data or a blank signal if no
* source is provided.
*/
struct smbrr_signal *smbrr_signal_new(enum smbrr_image_type type,
	unsigned int length, enum smbrr_adu adu, const void *src_signal)
{
	struct smbrr_signal *signal;
	size_t size;
	int bytes, err;

	if (length == 0)
		return NULL;

	switch (type) {
	case SMBRR_IMAGE_UINT32:
		bytes = sizeof(uint32_t);
		break;
	case SMBRR_IMAGE_FLOAT:
		bytes = sizeof(float);
		break;
	default:
		return NULL;
	}

	signal = calloc(1, sizeof(*signal));
	if (signal == NULL)
		return NULL;

	/* make ADU memory aligned on 32 bytes for SIMD */
	size = length * bytes;
	size += (size % 32);
	err = posix_memalign((void**)&signal->adu, 32, size);
	if (err < 0) {
		free(signal);
		return NULL;
	}
	bzero(signal->adu, size);
	set_signal_ops(signal);

	signal->size = size;
	signal->length = length;
	signal->type = type;

	if (src_signal == NULL)
		return signal;

	switch (adu) {

	case SMBRR_ADU_8:
		switch (type) {
		case SMBRR_IMAGE_UINT32:
			signal->ops->uchar_to_uint(signal, src_signal);
			break;
		case SMBRR_IMAGE_FLOAT:
			signal->ops->uchar_to_float(signal, src_signal);
			break;
		}
		break;

	case SMBRR_ADU_16:
		switch (type) {
		case SMBRR_IMAGE_UINT32:
			signal->ops->ushort_to_uint(signal, src_signal);
			break;
		case SMBRR_IMAGE_FLOAT:
			signal->ops->ushort_to_float(signal, src_signal);
			break;
		}
		break;

	case SMBRR_ADU_32:
		switch (type) {
		case SMBRR_IMAGE_UINT32:
			signal->ops->uint_to_uint(signal, src_signal);
			break;
		case SMBRR_IMAGE_FLOAT:
			signal->ops->uint_to_float(signal, src_signal);
			break;
		}
		break;

	case SMBRR_ADU_FLOAT:
		switch (type) {
		case SMBRR_IMAGE_UINT32:
			signal->ops->float_to_uint(signal, src_signal);
			break;
		case SMBRR_IMAGE_FLOAT:
			signal->ops->float_to_float(signal, src_signal);
			break;
		}
		break;

	default:
		free(signal->adu);
		free(signal);
		return NULL;
	}

	return signal;
}

/*! \fn struct smbrr_signal *smbrr_signal_new_from_region(struct smbrr_signal *src,
	unsigned int x_start, unsigned int y_start, unsigned int x_end,
	unsigned int y_end)
* \param src Source signal
* \param start X pixel offset for region start
* \param end X pixel offset for region end
* \return Pointer to new signal or NULL on failure
*
* Create a new smbrr signal from a smaller source signal region. This is useful
* for faster processing and reconstructing smaller regions of interest.
*/
struct smbrr_signal *smbrr_signal_new_from_region(struct smbrr_signal *src,
	unsigned int start, unsigned int end)
{
	struct smbrr_signal *signal;
	int bytes, err, length;
	size_t size;

	length = end - start;

	if (length <= 0)
		return NULL;

	switch (src->type) {
	case SMBRR_IMAGE_UINT32:
		bytes = sizeof(uint32_t);
		break;
	case SMBRR_IMAGE_FLOAT:
		bytes = sizeof(float);
		break;
	default:
		return NULL;
	}

	signal = calloc(1, sizeof(*signal));
	if (signal == NULL)
		return NULL;

	/* make ADU memory aligned on 32 bytes for SIMD */
	size = length * bytes;
	size += (size % 32);
	err = posix_memalign((void**)&signal->adu, 32, size);
	if (err < 0) {
		free(signal);
		return NULL;
	}
	bzero(signal->adu, size);
	set_signal_ops(signal);

	signal->size = size;
	signal->length = length;
	signal->type = src->type;

	/* copy data from src signal to new signal */
	memcpy(signal->adu, src->adu + start, length * sizeof(float));

	return signal;
}

/*! \fn struct smbrr_signal *smbrr_signal_new_copy(struct smbrr_signal *signal)
* \param signal Source signal.
* \return Pointer to new signal or NULL on failure
*
* Create a new smbrr signal from source signal.
*/
struct smbrr_signal *smbrr_signal_new_copy(struct smbrr_signal *src)
{
	return smbrr_signal_new_from_region(src, 0, src->length);
}

/*! \fn void smbrr_signal_free(struct smbrr_signal *signal)
* \param signal Image to be freed
*
* Free signal resources.
*/
void smbrr_signal_free(struct smbrr_signal *signal)
{
	if (signal == NULL)
		return;

	free(signal->adu);
	free(signal);
}

/*! \fn int smbrr_signal_get(struct smbrr_signal *signal, enum smbrr_adu adu,
	void **buf)
* \param signal Image
* \param adu ADU type of raw data
* \param buf Pointer to raw data buffer.
* \return 0 on success.
*
* Copy signal pixel data to buffer buf and convert it to adu format.
*/
int smbrr_signal_get(struct smbrr_signal *signal, enum smbrr_adu adu,
	void **buf)
{
	return signal->ops->get(signal, adu, buf);
}

/*! \fn int smbrr_signal_copy(struct smbrr_signal *dest, struct smbrr_signal *src)
* \param src Source signal
* \param dest Destination signal.
* \return 0 on success.
*
* Copy the source signal to destination
*/
int smbrr_signal_copy(struct smbrr_signal *dest, struct smbrr_signal *src)
{
	if (dest->length != src->length)
		return -EINVAL;

	memcpy(dest->adu, src->adu, sizeof(float) * dest->size);
	return 0;
}

/*! \fn int smbrr_signal_convert(struct smbrr_signal *signal,
	enum smbrr_signal_type type)
* \param signal Image
* \param type target destination type
* \return 0 on success
*
* Convert an signal from one type to another.
*/
int smbrr_signal_convert(struct smbrr_signal *signal,
	enum smbrr_image_type type)
{
	return signal->ops->convert(signal, type);
}

/*! \fn void smbrr_signal_find_limits(struct smbrr_signal *signal,
    float *min, float *max)
* \param signal Image
* \param min pointer to store signal min pixel value
* \param max pointer to store signal max pixel value
*
* Find min and max pixel value limits for signal.
*/
void smbrr_signal_find_limits(struct smbrr_signal *signal, float *min, float *max)
{
	signal->ops->find_limits(signal, min, max);
}

/*! \fn void smbrr_signal_normalise(struct smbrr_signal *signal, float min,
	float max)
* \param signal Image
* \param min minimum pixel value
* \param max maximum pixel value
*
* Normalise signal within a defined minimum value and range factor. This
* subtracts minimum from value and then multiplies by factor.
*/
void smbrr_signal_normalise(struct smbrr_signal *signal, float min, float max)
{
	signal->ops->normalise(signal, min, max);
}

/*! \fn void smbrr_signal_add(struct smbrr_signal *a, struct smbrr_signal *b,
	struct smbrr_signal *c)
* \param a Image A
* \param b signal B
* \param c Image C
*
* Add pixels in signal C from signal B and store in signal A.
*/
void smbrr_signal_add(struct smbrr_signal *a, struct smbrr_signal *b,
	struct smbrr_signal *c)
{
	a->ops->add(a, b, c);
}

/*! \fn void smbrr_signal_add_sig(struct smbrr_signal *a, struct smbrr_signal *b,
	struct smbrr_signal *c, struct smbrr_signal *s)
* \param a Image A
* \param b signal B
* \param c Image C
* \param s Significant signal.
*
* Add significant pixels in signal C to signal B and store in signal A.
*/
void smbrr_signal_add_sig(struct smbrr_signal *a, struct smbrr_signal *b,
	struct smbrr_signal *c, struct smbrr_signal *s)
{
	a->ops->add_sig(a, b, c, s);
}

/*! \fn void smbrr_signal_fma(struct smbrr_signal *dest, struct smbrr_signal *a,
	struct smbrr_signal *b, float c)
* \param dest Destination Image
* \param a Image A
* \param b signal B
* \param c value C
*
* Add pixels in signal B multiplied by value from signal A and store in
* destination signal.
*/
void smbrr_signal_fma(struct smbrr_signal *dest, struct smbrr_signal *a,
	struct smbrr_signal *b, float c)
{
	a->ops->fma(dest, a, b, c);
}

/*! \fn void smbrr_signal_fms(struct smbrr_signal *dest, struct smbrr_signal *a,
	struct smbrr_signal *b, float c)
* \param dest Destination Image
* \param a Image A
* \param b signal B
* \param c value C
*
* Subtract pixels in signal B multiplied by value from signal A and store in
* destination signal.
*/
void smbrr_signal_fms(struct smbrr_signal *dest, struct smbrr_signal *a,
	struct smbrr_signal *b, float c)
{
	dest->ops->fms(dest, a, b, c);
}

/*! \fn void smbrr_signal_subtract(struct smbrr_signal *a, struct smbrr_signal *b,
	struct smbrr_signal *c)
* \param a Image A
* \param b signal B
* \param c Image C
*
* Subtract pixels in signal C from signal B and store in signal A.
*/
void smbrr_signal_subtract(struct smbrr_signal *a, struct smbrr_signal *b,
	struct smbrr_signal *c)
{
	a->ops->subtract(a, b, c);
}

/*! \fn void smbrr_signal_subtract_sig(struct smbrr_signal *a, struct smbrr_signal *b,
	struct smbrr_signal *c, struct smbrr_signal *s)
* \param a Image A
* \param b signal B
* \param c Image C
* \param s Significant signal.
*
* Subtract significant pixels in signal C from signal B and store in signal A.
*/
void smbrr_signal_subtract_sig(struct smbrr_signal *a, struct smbrr_signal *b,
	struct smbrr_signal *c, struct smbrr_signal *s)
{
	a->ops->subtract_sig(a, b, c, s);
}

/*! \fn void smbrr_signal_add_value(struct smbrr_signal *signal, float value)
* \param signal Image
* \param value Value
*
* Add value to all signal pixels.
*/
void smbrr_signal_add_value(struct smbrr_signal *signal, float value)
{
	signal->ops->add_value(signal, value);
}

/*! \fn void smbrr_signal_add_value_sig(struct smbrr_signal *signal,
	struct smbrr_signal *signal, float value)
* \param signal Image
* \param value Value
*
* Add value to all significant signal pixels.
*/
void smbrr_signal_add_value_sig(struct smbrr_signal *signal,
	struct smbrr_signal *ssignal, float value)
{
	signal->ops->add_value_sig(signal, ssignal, value);
}

/*! \fn void smbrr_signal_subtract_value(struct smbrr_signal *signal, float value)
* \param signal Image
* \param value Value
*
* Subtract value from all signal pixels.
*/
void smbrr_signal_subtract_value(struct smbrr_signal *signal, float value)
{
	signal->ops->subtract_value(signal, value);
}

/*! \fn void smbrr_signal_mult_value(struct smbrr_signal *signal, float value)
* \param signal Image
* \param value Value
*
* Multiply all signal pixels by value.
*/
void smbrr_signal_mult_value(struct smbrr_signal *signal, float value)
{
	signal->ops->mult_value(signal, value);
}

/*! \fn void smbrr_signal_reset_value(struct smbrr_signal *signal, float value)
* \param signal Image
* \param value Value
*
* Reset all values in signal to value
*/
void smbrr_signal_reset_value(struct smbrr_signal *signal, float value)
{
	signal->ops->reset_value(signal, value);
}

/*! \fn void smbrr_signal_set_sig_value(struct smbrr_signal *signal,
	uint32_t value)
* \param signal Image
* \param value reset value
*
* Reset all values in significant signal to value
*/
void smbrr_signal_set_sig_value(struct smbrr_signal *signal, uint32_t value)
{
	signal->ops->set_sig_value(signal, value);
}

/*! \fn void smbrr_signal_set_value_sig(struct smbrr_signal *signal,
	struct smbrr_signal *ssignal, float sig_value)
* \param signal Image
* \param ssignal Significant Image
* \param sig_value Significant value.
*
* Set signal pixels to value if pixel is significant.
*/
void smbrr_signal_set_value_sig(struct smbrr_signal *signal,
	struct smbrr_signal *ssignal, float sig_value)
{
	signal->ops->set_value_sig(signal, ssignal, sig_value);
}

/*! \fn void smbrr_signal_clear_negative(struct smbrr_signal *signal)
* \param signal Image
*
* Clear any signal pixels with negative values to zero..
*/
void smbrr_signal_clear_negative(struct smbrr_signal *signal)
{
	signal->ops->clear_negative(signal);
}

/*! \fn int smbrr_signal_psf(struct smbrr_signal *src, struct smbrr_signal *dest,
	enum smbrr_wavelet_mask mask)
* \param src Source signal
* \param dest Destination Image
* \param mask PSF convolution mask
* \return 0 for success.
*
* Perform Point Spread Function on source signal and store in destination signal
* using wavelet masks to blur signal.
*/
int smbrr_signal_psf(struct smbrr_signal *src, struct smbrr_signal *dest,
	enum smbrr_wavelet_mask mask)
{
	return src->ops->psf(src, dest, mask);
}

/*! \fn int smbrr_signal_width(struct smbrr_signal *signal)
* \param signal Image
* \return Image stride.
*
* Return the number of pixels in signal width.
*/
int smbrr_signal_length(struct smbrr_signal *signal)
{
	return signal->length;
}

/*! \fn float smbrr_signal_get_adu_at(struct smbrr_signal *signal, int x, int y);
 * \param signal Image
 * \param x X coordinate
 * \return Image ADU
 *
 * Get signal ADU value at (x)
 */
float smbrr_signal_get_adu_at(struct smbrr_signal *signal, int x)
{
	if (x < 0 || x >= signal->length)
		return -1.0;

	return signal->adu[x];
}
