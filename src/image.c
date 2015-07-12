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

static void set_image_ops(struct smbrr_image *image)
{
	unsigned int cpu_flags = cpu_get_flags();

#if defined HAVE_FMA
	if (cpu_flags & CPU_X86_FMA) {
		image->ops = &image_ops_fma;
		return;
	}
#endif
#if defined HAVE_AVX2
	if (cpu_flags & CPU_X86_AVX2) {
		image->ops = &image_ops_avx2;
		return;
	}
#endif
#if defined HAVE_AVX
	if (cpu_flags & CPU_X86_AVX) {
		image->ops = &image_ops_avx;
		return;
	}
#endif
#if defined HAVE_SSE42
	if (cpu_flags & CPU_X86_SSE4_2) {
		image->ops = &image_ops_sse42;
		return;
	}
#endif

	/* default C implementation */
	image->ops = &image_ops;
}

/*! \fn struct smbrr_image *smbrr_image_new(enum smbrr_image_type type,
	unsigned int width, unsigned int height, unsigned int stride,
	enum smbrr_adu adu, const void *src_image);
* \param type New image type
* \param width Image width in pixels
* \param height Image height in pixels
* \param stride Image stride in pixels. width += %4
* \param adu Source data ADU type
* \param src_image source image raw data
* \return Pointer to new image or NULL on failure
*
* Create a new smbrr image from source raw data or a blank image if no
* source is provided.
*/
struct smbrr_image *smbrr_image_new(enum smbrr_image_type type,
	unsigned int width, unsigned int height, unsigned int stride,
	enum smbrr_adu adu, const void *src_image)
{
	struct smbrr_image *image;
	size_t size;
	int bytes, err;

	if (width == 0 || height == 0)
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

	image = calloc(1, sizeof(*image));
	if (image == NULL)
		return NULL;

	/* make ADU memory aligned on 32 bytes for SIMD */
	size = width * height * bytes;
	size += (size % 32);
	err = posix_memalign((void**)&image->adu, 32, size);
	if (err < 0) {
		free(image);
		return NULL;
	}
	bzero(image->adu, size);
	set_image_ops(image);

	image->size = width * height;
	image->width = width;
	image->height = height;
	if (stride == 0) {
		if (image->width % 4)
			image->stride = image->width + 4 - (image->width % 4);
		else
			image->stride = image->width;
	} else
		image->stride = stride;

	image->type = type;

	if (src_image == NULL)
		return image;

	switch (adu) {

	case SMBRR_ADU_8:
		switch (type) {
		case SMBRR_IMAGE_UINT32:
			image->ops->uchar_to_uint(image, src_image);
			break;
		case SMBRR_IMAGE_FLOAT:
			image->ops->uchar_to_float(image, src_image);
			break;
		}
		break;

	case SMBRR_ADU_16:
		switch (type) {
		case SMBRR_IMAGE_UINT32:
			image->ops->ushort_to_uint(image, src_image);
			break;
		case SMBRR_IMAGE_FLOAT:
			image->ops->ushort_to_float(image, src_image);
			break;
		}
		break;

	case SMBRR_ADU_32:
		switch (type) {
		case SMBRR_IMAGE_UINT32:
			image->ops->uint_to_uint(image, src_image);
			break;
		case SMBRR_IMAGE_FLOAT:
			image->ops->uint_to_float(image, src_image);
			break;
		}
		break;

	case SMBRR_ADU_FLOAT:
		switch (type) {
		case SMBRR_IMAGE_UINT32:
			image->ops->float_to_uint(image, src_image);
			break;
		case SMBRR_IMAGE_FLOAT:
			image->ops->float_to_float(image, src_image);
			break;
		}
		break;

	default:
		free(image->adu);
		free(image);
		return NULL;
	}

	return image;
}

/*! \fn struct smbrr_image *smbrr_image_new_from_region(struct smbrr_image *src,
	unsigned int x_start, unsigned int y_start, unsigned int x_end,
	unsigned int y_end)
* \param src Source image
* \param x_start X pixel offset for region start
* \param y_start Y pixel offset for region start
* \param x_end X pixel offset for region end
* \param y_end Y pixel offset for region end
* \return Pointer to new image or NULL on failure
*
* Create a new smbrr image from a smaller source image region. This is useful
* for faster processing and reconstructing smaller regions of interest.
*/
struct smbrr_image *smbrr_image_new_from_region(struct smbrr_image *src,
	unsigned int x_start, unsigned int y_start, unsigned int x_end,
	unsigned int y_end)
{
	struct smbrr_image *image;
	int bytes, width, height, i, err;
	size_t size;

	width = x_end - x_start;
	height = y_end - y_start;

	if (width <= 0 || height <= 0)
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

	image = calloc(1, sizeof(*image));
	if (image == NULL)
		return NULL;

	/* make ADU memory aligned on 32 bytes for SIMD */
	size = width * height * bytes;
	size += (size % 32);
	err = posix_memalign((void**)&image->adu, 32, size);
	if (err < 0) {
		free(image);
		return NULL;
	}
	bzero(image->adu, size);
	set_image_ops(image);

	image->size = width * height;
	image->width = width;
	image->height = height;
	if (image->width % 4)
		image->stride = image->width + 4 - (image->width % 4);
	else
		image->stride = image->width;

	image->type = src->type;

	/* copy each row from src image to new image */
	for (i = y_start; i < y_end; i++) {
		unsigned int offset = i * image->width + x_start;
		memcpy(image->adu + i * width, src->adu + offset,
			width * sizeof(float));
	}

	return image;
}

/*! \fn struct smbrr_image *smbrr_image_new_copy(struct smbrr_image *image)
* \param image Source image.
* \return Pointer to new image or NULL on failure
*
* Create a new smbrr image from source image.
*/
struct smbrr_image *smbrr_image_new_copy(struct smbrr_image *src)
{
	return smbrr_image_new_from_region(src, 0, 0, src->width, src->height);
}

/*! \fn void smbrr_image_free(struct smbrr_image *image)
* \param image Image to be freed
*
* Free image resources.
*/
void smbrr_image_free(struct smbrr_image *image)
{
	if (image == NULL)
		return;

	free(image->adu);
	free(image);
}

/*! \fn int smbrr_image_get(struct smbrr_image *image, enum smbrr_adu adu,
	void **buf)
* \param image Image
* \param adu ADU type of raw data
* \param buf Pointer to raw data buffer.
* \return 0 on success.
*
* Copy image pixel data to buffer buf and convert it to adu format.
*/
int smbrr_image_get(struct smbrr_image *image, enum smbrr_adu adu,
	void **buf)
{
	return image->ops->get(image, adu, buf);
}

/*! \fn int smbrr_image_copy(struct smbrr_image *dest, struct smbrr_image *src)
* \param src Source image
* \param dest Destination image.
* \return 0 on success.
*
* Copy the source image to destination
*/
int smbrr_image_copy(struct smbrr_image *dest, struct smbrr_image *src)
{
	if (dest->width != src->width)
		return -EINVAL;
	if (dest->height != src->height)
		return -EINVAL;

	memcpy(dest->adu, src->adu, sizeof(float) * dest->size);
	return 0;
}

/*! \fn int smbrr_image_convert(struct smbrr_image *image,
	enum smbrr_image_type type)
* \param image Image
* \param type target destination type
* \return 0 on success
*
* Convert an image from one type to another.
*/
int smbrr_image_convert(struct smbrr_image *image,
	enum smbrr_image_type type)
{
	return image->ops->convert(image, type);
}

/*! \fn void smbrr_image_find_limits(struct smbrr_image *image,
    float *min, float *max)
* \param image Image
* \param min pointer to store image min pixel value
* \param max pointer to store image max pixel value
*
* Find min and max pixel value limits for image.
*/
void smbrr_image_find_limits(struct smbrr_image *image, float *min, float *max)
{
	image->ops->find_limits(image, min, max);
}

/*! \fn void smbrr_image_normalise(struct smbrr_image *image, float min,
	float max)
* \param image Image
* \param min minimum pixel value
* \param max maximum pixel value
*
* Normalise image within a defined minimum value and range factor. This
* subtracts minimum from value and then multiplies by factor.
*/
void smbrr_image_normalise(struct smbrr_image *image, float min, float max)
{
	image->ops->normalise(image, min, max);
}

/*! \fn void smbrr_image_add(struct smbrr_image *a, struct smbrr_image *b,
	struct smbrr_image *c)
* \param a Image A
* \param b image B
* \param c Image C
*
* Add pixels in image C from image B and store in image A.
*/
void smbrr_image_add(struct smbrr_image *a, struct smbrr_image *b,
	struct smbrr_image *c)
{
	a->ops->add(a, b, c);
}

/*! \fn void smbrr_image_add_sig(struct smbrr_image *a, struct smbrr_image *b,
	struct smbrr_image *c, struct smbrr_image *s)
* \param a Image A
* \param b image B
* \param c Image C
* \param s Significant image.
*
* Add significant pixels in image C to image B and store in image A.
*/
void smbrr_image_add_sig(struct smbrr_image *a, struct smbrr_image *b,
	struct smbrr_image *c, struct smbrr_image *s)
{
	a->ops->add_sig(a, b, c, s);
}

/*! \fn void smbrr_image_fma(struct smbrr_image *dest, struct smbrr_image *a,
	struct smbrr_image *b, float c)
* \param dest Destination Image
* \param a Image A
* \param b image B
* \param c value C
*
* Add pixels in image B multiplied by value from image A and store in
* destination image.
*/
void smbrr_image_fma(struct smbrr_image *dest, struct smbrr_image *a,
	struct smbrr_image *b, float c)
{
	a->ops->fma(dest, a, b, c);
}

/*! \fn void smbrr_image_fms(struct smbrr_image *dest, struct smbrr_image *a,
	struct smbrr_image *b, float c)
* \param dest Destination Image
* \param a Image A
* \param b image B
* \param c value C
*
* Subtract pixels in image B multiplied by value from image A and store in
* destination image.
*/
void smbrr_image_fms(struct smbrr_image *dest, struct smbrr_image *a,
	struct smbrr_image *b, float c)
{
	dest->ops->fms(dest, a, b, c);
}

/*! \fn void smbrr_image_subtract(struct smbrr_image *a, struct smbrr_image *b,
	struct smbrr_image *c)
* \param a Image A
* \param b image B
* \param c Image C
*
* Subtract pixels in image C from image B and store in image A.
*/
void smbrr_image_subtract(struct smbrr_image *a, struct smbrr_image *b,
	struct smbrr_image *c)
{
	a->ops->subtract(a, b, c);
}

/*! \fn void smbrr_image_subtract_sig(struct smbrr_image *a, struct smbrr_image *b,
	struct smbrr_image *c, struct smbrr_image *s)
* \param a Image A
* \param b image B
* \param c Image C
* \param s Significant image.
*
* Subtract significant pixels in image C from image B and store in image A.
*/
void smbrr_image_subtract_sig(struct smbrr_image *a, struct smbrr_image *b,
	struct smbrr_image *c, struct smbrr_image *s)
{
	a->ops->subtract_sig(a, b, c, s);
}

/*! \fn void smbrr_image_add_value(struct smbrr_image *image, float value)
* \param image Image
* \param value Value
*
* Add value to all image pixels.
*/
void smbrr_image_add_value(struct smbrr_image *image, float value)
{
	image->ops->add_value(image, value);
}

/*! \fn void smbrr_image_add_value_sig(struct smbrr_image *image,
	struct smbrr_image *image, float value)
* \param image Image
* \param value Value
*
* Add value to all significant image pixels.
*/
void smbrr_image_add_value_sig(struct smbrr_image *image,
	struct smbrr_image *simage, float value)
{
	image->ops->add_value_sig(image, simage, value);
}

/*! \fn void smbrr_image_subtract_value(struct smbrr_image *image, float value)
* \param image Image
* \param value Value
*
* Subtract value from all image pixels.
*/
void smbrr_image_subtract_value(struct smbrr_image *image, float value)
{
	image->ops->subtract_value(image, value);
}

/*! \fn void smbrr_image_mult_value(struct smbrr_image *image, float value)
* \param image Image
* \param value Value
*
* Multiply all image pixels by value.
*/
void smbrr_image_mult_value(struct smbrr_image *image, float value)
{
	image->ops->mult_value(image, value);
}

/*! \fn void smbrr_image_reset_value(struct smbrr_image *image, float value)
* \param image Image
* \param value Value
*
* Reset all values in image to value
*/
void smbrr_image_reset_value(struct smbrr_image *image, float value)
{
	image->ops->reset_value(image, value);
}

/*! \fn void smbrr_image_set_sig_value(struct smbrr_image *image,
	uint32_t value)
* \param image Image
* \param value reset value
*
* Reset all values in significant image to value
*/
void smbrr_image_set_sig_value(struct smbrr_image *image, uint32_t value)
{
	image->ops->set_sig_value(image, value);
}

/*! \fn void smbrr_image_set_value_sig(struct smbrr_image *image,
	struct smbrr_image *simage, float sig_value)
* \param image Image
* \param simage Significant Image
* \param sig_value Significant value.
*
* Set image pixels to value if pixel is significant.
*/
void smbrr_image_set_value_sig(struct smbrr_image *image,
	struct smbrr_image *simage, float sig_value)
{
	image->ops->set_value_sig(image, simage, sig_value);
}

/*! \fn void smbrr_image_clear_negative(struct smbrr_image *image)
* \param image Image
*
* Clear any image pixels with negative values to zero..
*/
void smbrr_image_clear_negative(struct smbrr_image *image)
{
	image->ops->clear_negative(image);
}

/*! \fn int smbrr_image_psf(struct smbrr_image *src, struct smbrr_image *dest,
	enum smbrr_wavelet_mask mask)
* \param src Source image
* \param dest Destination Image
* \param mask PSF convolution mask
* \return 0 for success.
*
* Perform Point Spread Function on source image and store in destination image
* using wavelet masks to blur image.
*/
int smbrr_image_psf(struct smbrr_image *src, struct smbrr_image *dest,
	enum smbrr_wavelet_mask mask)
{
	return src->ops->psf(src, dest, mask);
}

/*! \fn int smbrr_image_pixels(struct smbrr_image *image)
* \param image Image
* \return Number of image pixels.
*
* Return the number of valid pixels used by image. i.e. width * height.
*/
int smbrr_image_pixels(struct smbrr_image *image)
{
	return image->width * image->height;
}

/*! \fn int smbrr_image_bytes(struct smbrr_image *image)
* \param image Image
* \return Number of image bytes.
*
* Return the number of raw pixels used by image. i.e. stride * height.
*/
int smbrr_image_bytes(struct smbrr_image *image)
{
	return image->stride * image->height;
}

/*! \fn int smbrr_image_stride(struct smbrr_image *image)
* \param image Image
* \return Image stride.
*
* Return the number of pixels in image stride.
*/
int smbrr_image_stride(struct smbrr_image *image)
{
	return image->stride;
}

/*! \fn int smbrr_image_width(struct smbrr_image *image)
* \param image Image
* \return Image stride.
*
* Return the number of pixels in image width.
*/
int smbrr_image_width(struct smbrr_image *image)
{
	return image->width;
}

/*! \fn int smbrr_image_height(struct smbrr_image *image)
* \param image Image
* \return Image height.
*
* Return the number of pixels in image height.
*/
int smbrr_image_height(struct smbrr_image *image)
{
	return image->height;
}

/*! \fn float smbrr_image_get_adu_at(struct smbrr_image *image, int x, int y);
 * \param image Image
 * \param x X coordinate
 * \param y Y coordinate
 * \return Image ADU
 *
 * Get image ADU value at (x,y)
 */
float smbrr_image_get_adu_at(struct smbrr_image *image, int x, int y)
{
	int pixel;

	if (x < 0 || x >= image->width)
		return -1.0;
	if (y < 0 || y >= image->height)
		return -1.0;

	pixel = y * image->width + x;
	return image->adu[pixel];
}
