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
#include <immintrin.h>

#include "config.h"
#include "sombrero.h"
#include "local.h"

static void uchar_to_float(struct smbrr_image *i, const unsigned char *c)
{
	int x, y, foffset, coffset;
	float *f = i->adu;

	for (x = 0; x < i->width; x++) {
		for (y = 0; y < i->height; y++) {
			coffset = y * i->stride + x;
			foffset = y * i->width + x;
			f[foffset] = (float)c[coffset];
		}
	}
}

static void ushort_to_float(struct smbrr_image *i, const unsigned short *c)
{
	int x, y, foffset, coffset;
	float *f = i->adu;

	for (x = 0; x < i->width; x++) {
		for (y = 0; y < i->height; y++) {
			coffset = y * i->stride + x;
			foffset = y * i->width + x;
			f[foffset] = (float)c[coffset];
		}
	}
}

static void uint_to_float(struct smbrr_image *i, const unsigned int *c)
{
	int x, y, foffset, coffset;
	float *f = i->adu;

	for (x = 0; x < i->width; x++) {
		for (y = 0; y < i->height; y++) {
			coffset = y * i->stride + x;
			foffset = y * i->width + x;
			f[foffset] = (float)c[coffset];
		}
	}
}

static void float_to_uchar(struct smbrr_image *i, unsigned char *c)
{
	int x, y, coffset, foffset;
	float *f = i->adu;

	for (x = 0; x < i->width; x++) {
		for (y = 0; y < i->height; y++) {
			coffset = y * i->stride + x;
			foffset = y * i->width + x;
			c[coffset] = (unsigned char)f[foffset];
		}
	}
}

static void uint_to_uint(struct smbrr_image *i, const unsigned int *c)
{
	int x, y, foffset, coffset;
	uint32_t *f = i->s;

	for (x = 0; x < i->width; x++) {
		for (y = 0; y < i->height; y++) {
			coffset = y * i->stride + x;
			foffset = y * i->width + x;
			f[foffset] = (uint32_t)c[coffset];
		}
	}
}

static void ushort_to_uint(struct smbrr_image *i, const unsigned short *c)
{
	int x, y, foffset, coffset;
	uint32_t *f = i->s;

	for (x = 0; x < i->width; x++) {
		for (y = 0; y < i->height; y++) {
			coffset = y * i->stride + x;
			foffset = y * i->width + x;
			f[foffset] = (uint32_t)c[coffset];
		}
	}
}

static void uchar_to_uint(struct smbrr_image *i, const unsigned char *c)
{
	int x, y, foffset, coffset;
	uint32_t *f = i->s;

	for (x = 0; x < i->width; x++) {
		for (y = 0; y < i->height; y++) {
			coffset = y * i->stride + x;
			foffset = y * i->width + x;
			f[foffset] = (uint32_t)c[coffset];
		}
	}
}

static void uint_to_uchar(struct smbrr_image *i, unsigned char *c)
{
	int x, y, coffset, foffset;
	uint32_t *f = i->s;

	for (x = 0; x < i->width; x++) {
		for (y = 0; y < i->height; y++) {
			coffset = y * i->stride + x;
			foffset = y * i->width + x;
			c[coffset] = (unsigned char)f[foffset];
		}
	}
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
			uchar_to_uint(image, src_image);
			break;
		case SMBRR_IMAGE_FLOAT:
			uchar_to_float(image, src_image);
			break;
		}
		break;
	case SMBRR_ADU_16:

		switch (type) {
		case SMBRR_IMAGE_UINT32:
			ushort_to_uint(image, src_image);
			break;
		case SMBRR_IMAGE_FLOAT:
			ushort_to_float(image, src_image);
			break;
		}
		break;
	case SMBRR_ADU_32:

		switch (type) {
		case SMBRR_IMAGE_UINT32:
			uint_to_uint(image, src_image);
			break;
		case SMBRR_IMAGE_FLOAT:
			uint_to_float(image, src_image);
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

	image->size = width * height;
	image->width = width;
	image->height = height;
	if (image->width % 4)
		image->stride = image->width + 4 - (image->width % 4);
	else
		image->stride = image->width;

	image->type = src->type;

	/* copy each row from src image to new image */
	for (i = y_start; i <= y_end; i++) {
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
	if (buf == NULL)
		return -EINVAL;

	switch (adu) {
	case SMBRR_ADU_8:

		switch (image->type) {
		case SMBRR_IMAGE_UINT32:
			uint_to_uchar(image, *buf);
			break;
		case SMBRR_IMAGE_FLOAT:
			float_to_uchar(image, *buf);
			break;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
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
	int offset;

	if (type == image->type)
		return 0;

	switch (type) {
	case SMBRR_IMAGE_UINT32:
		switch (image->type) {
		case SMBRR_IMAGE_FLOAT:
			for (offset = 0; offset < image->size; offset++)
				image->s[offset] = (uint32_t)image->adu[offset];
			break;
		case SMBRR_IMAGE_UINT32:
			break;
		}
		break;
	case SMBRR_IMAGE_FLOAT:
		switch (image->type) {
		case SMBRR_IMAGE_UINT32:
			for (offset = 0; offset < image->size; offset++)
				image->adu[offset] = (float)image->s[offset];
			break;
		case SMBRR_IMAGE_FLOAT:
			break;
		}
		break;
	default:
		return -EINVAL;
	}

	image->type = type;
	return 0;
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
	float *adu = image->adu, _min, _max;
	int offset;

	_min = 1.0e6;
	_max = -1.0e6;

	for (offset = 0; offset < image->size; offset++) {

		if (adu[offset] > _max)
			_max = adu[offset];

		if (adu[offset] < _min)
			_min = adu[offset];
	}

	*max = _max;
	*min = _min;
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
	float _min, _max, _range, range, factor;
	size_t offset;

	smbrr_image_find_limits(image, &_min, &_max);

	_range = _max - _min;
	range = max - min;
	factor = range / _range;

#if HAVE_AVX
	float * __restrict__ I = __builtin_assume_aligned(image->adu, 32);
	__m256 mi, mm, mf, mr, mim;

	for (offset = 0; offset < image->size; offset += 8) {
		mi = _mm256_load_ps(&I[offset]);
		mf = _mm256_broadcast_ss(&factor);
		mm = _mm256_broadcast_ss(&_min);
		mim = _mm256_sub_ps(mi, mm);
		mr = _mm256_mul_ps(mim, mf);
		_mm256_store_ps(&I[offset], mr);
	}
#else
	float *adu = image->adu;

	for (offset = 0; offset < image->size; offset++)
		adu[offset] = (adu[offset] - _min) * factor;
#endif
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
	float * __restrict__ A = __builtin_assume_aligned(a->adu, 32);
	float * __restrict__ B = __builtin_assume_aligned(b->adu, 32);
	float * __restrict__ C = __builtin_assume_aligned(c->adu, 32);
	size_t offset;

#if HAVE_AVX
	__m256 ma, mb, mc;

	for (offset = 0; offset < a->size; offset += 8) {
		mb = _mm256_load_ps(&B[offset]);
		mc = _mm256_load_ps(&C[offset]);
		ma = _mm256_add_ps(mb, mc);
		_mm256_store_ps(&A[offset], ma);
	}
#else
	/* A = B + C */
	for (offset = 0; offset < a->size; offset++)
		A[offset] = B[offset] + C[offset];
#endif
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
	float *A = a->adu, *B = b->adu, *C = c->adu;
	uint32_t *S = s->s;
	int offset;

	/* iff S then A = B + C */
	for (offset = 0; offset < a->size; offset++) {
		if (S[offset])
			A[offset] = B[offset] + C[offset];
	}
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
	float * __restrict__ D = __builtin_assume_aligned(dest->adu, 32);
	float * __restrict__ A = __builtin_assume_aligned(a->adu, 32);
	float * __restrict__ B = __builtin_assume_aligned(b->adu, 32);
	size_t offset;

#if HAVE_AVX
	__m256 ma, mb, mv, mr, mbv;

	for (offset = 0; offset < dest->size; offset += 8) {
		ma = _mm256_load_ps(&A[offset]);
		mb = _mm256_load_ps(&B[offset]);
		mv = _mm256_broadcast_ss(&c);
		mbv = _mm256_mul_ps(mb, mv);
		mr = _mm256_add_ps(ma, mbv);
		_mm256_store_ps(&D[offset], mr);
	}
#else

	/* dest = a + b * c */
	for (offset = 0; offset < dest->size; offset++)
		D[offset] = A[offset] + B[offset] * c;
#endif
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
	float * __restrict__ D = __builtin_assume_aligned(dest->adu, 32);
	float * __restrict__ A = __builtin_assume_aligned(a->adu, 32);
	float * __restrict__ B = __builtin_assume_aligned(b->adu, 32);
	size_t offset;

#if HAVE_AVX
	__m256 ma, mb, mv, mr, mbv;

	for (offset = 0; offset < dest->size; offset += 8) {
		ma = _mm256_load_ps(&A[offset]);
		mb = _mm256_load_ps(&B[offset]);
		mv = _mm256_broadcast_ss(&c);
		mbv = _mm256_mul_ps(mb, mv);
		mr = _mm256_sub_ps(ma, mbv);
		_mm256_store_ps(&D[offset], mr);
	}
#else
	/* dest = a - b * c */
	for (offset = 0; offset < dest->size; offset++)
		D[offset] = A[offset] - B[offset] * c;
#endif
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
	float * __restrict__ A = __builtin_assume_aligned(a->adu, 32);
	float * __restrict__ B = __builtin_assume_aligned(b->adu, 32);
	float * __restrict__ C = __builtin_assume_aligned(c->adu, 32);
	size_t offset;

#if HAVE_AVX
	__m256 ma, mb, mc;

	for (offset = 0; offset < a->size; offset += 8) {
		mb = _mm256_load_ps(&B[offset]);
		mc = _mm256_load_ps(&C[offset]);
		ma = _mm256_sub_ps(mb, mc);
		_mm256_store_ps(&A[offset], ma);
	}
#else
	/* A = B - C */
	for (offset = 0; offset < a->size; offset++)
		A[offset] = B[offset] + C[offset];
#endif

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
	float *A = a->adu, *B = b->adu, *C = c->adu;
	uint32_t *S = s->s;
	int offset;

	/* iff S then A = B - C */
	for (offset = 0; offset < a->size; offset++) {
		if (S[offset])
			A[offset] = B[offset] - C[offset];
	}
}

/*! \fn void smbrr_image_add_value(struct smbrr_image *image, float value)
* \param image Image
* \param value Value
*
* Add value to all image pixels.
*/
void smbrr_image_add_value(struct smbrr_image *image, float value)
{
	size_t offset;

#if HAVE_AVX
	float * __restrict__ I = __builtin_assume_aligned(image->adu, 32);
	__m256 mi, mv, mr;

	for (offset = 0; offset < image->size; offset += 8) {
		mi = _mm256_load_ps(&I[offset]);
		mv = _mm256_broadcast_ss(&value);
		mr = _mm256_add_ps(mi, mv);
		_mm256_store_ps(&I[offset], mr);
	}
#else
	float *adu = image->adu;

	for (offset = 0; offset < image->size; offset++)
		adu[offset] += value;
#endif
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
	float *adu = image->adu;
	int offset;

	for (offset = 0; offset < image->size; offset++) {
		if (simage->s[offset])
			adu[offset] += value;
	}
}

/*! \fn void smbrr_image_subtract_value(struct smbrr_image *image, float value)
* \param image Image
* \param value Value
*
* Subtract value from all image pixels.
*/
void smbrr_image_subtract_value(struct smbrr_image *image, float value)
{
	size_t offset;

#if HAVE_AVX
	float * __restrict__ I = __builtin_assume_aligned(image->adu, 32);
	__m256 mi, mv, mr;

	for (offset = 0; offset < image->size; offset += 8) {
		mi = _mm256_load_ps(&I[offset]);
		mv = _mm256_broadcast_ss(&value);
		mr = _mm256_sub_ps(mi, mv);
		_mm256_store_ps(&I[offset], mr);
	}
#else
	float *adu = image->adu;

	for (offset = 0; offset < image->size; offset++)
		adu[offset] -= value;
#endif
}

/*! \fn void smbrr_image_mult_value(struct smbrr_image *image, float value)
* \param image Image
* \param value Value
*
* Multiply all image pixels by value.
*/
void smbrr_image_mult_value(struct smbrr_image *image, float value)
{
	size_t offset;

#if HAVE_AVX
	float * __restrict__ I = __builtin_assume_aligned(image->adu, 32);
	__m256 mi, mv, mr;

	for (offset = 0; offset < image->size; offset += 8) {
		mi = _mm256_load_ps(&I[offset]);
		mv = _mm256_broadcast_ss(&value);
		mr = _mm256_mul_ps(mi, mv);
		_mm256_store_ps(&I[offset], mr);
	}
#else
	float *adu = image->adu;

	for (offset = 0; offset < image->size; offset++)
		adu[offset] *= value;
#endif
}

/*! \fn void smbrr_image_reset_value(struct smbrr_image *image, float value)
* \param image Image
* \param value Value
*
* Reset all values in image to value
*/
void smbrr_image_reset_value(struct smbrr_image *image, float value)
{
	float *adu = image->adu;
	int offset;

	for (offset = 0; offset < image->size; offset++)
		adu[offset] = value;
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
	uint32_t *adu = image->s;
	int offset;

	for (offset = 0; offset < image->size; offset++)
		adu[offset] = value;

	if (value == 0)
		image->sig_pixels = 0;
	else
		image->sig_pixels = image->size;
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
	float *adu = image->adu;
	uint32_t *sig = simage->s;
	int offset;

	for (offset = 0; offset < image->size; offset++) {
		if (sig[offset])
			adu[offset] = sig_value;
	}
}

/*! \fn void smbrr_image_clear_negative(struct smbrr_image *image)
* \param image Image
*
* Clear any image pixels with negative values to zero..
*/
void smbrr_image_clear_negative(struct smbrr_image *image)
{
	float *i = image->adu;
	int offset;

	for (offset = 0; offset < image->size; offset++)
		if (i[offset] < 0.0)
			i[offset] = 0.0;
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
