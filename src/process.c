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

static const struct data_ops* get_1d_ops(void)
{
	unsigned int cpu_flags = cpu_get_flags();

#if defined HAVE_FMA
	if (cpu_flags & CPU_X86_FMA)
		return &data_ops_1d_fma;
#endif
#if defined HAVE_AVX2
	if (cpu_flags & CPU_X86_AVX2)
		return &data_ops_1d_avx2;
#endif
#if defined HAVE_AVX
	if (cpu_flags & CPU_X86_AVX)
		return &data_ops_1d_avx;
#endif
#if defined HAVE_SSE42
	if (cpu_flags & CPU_X86_SSE4_2)
		return &data_ops_1d_sse42;
#endif

	/* default C implementation */
	return &data_ops_1d;
}

static const struct data_ops* get_2d_ops(void)
{
	unsigned int cpu_flags = cpu_get_flags();

#if defined HAVE_FMA
	if (cpu_flags & CPU_X86_FMA)
		return &data_ops_2d_fma;
#endif
#if defined HAVE_AVX2
	if (cpu_flags & CPU_X86_AVX2)
		return &data_ops_2d_avx2;
#endif
#if defined HAVE_AVX
	if (cpu_flags & CPU_X86_AVX)
		return &data_ops_2d_avx;
#endif
#if defined HAVE_SSE42
	if (cpu_flags & CPU_X86_SSE4_2)
		return &data_ops_2d_sse42;
#endif

	/* default C implementation */
	return &data_ops_2d;
}

/*! \fn struct smbrr *smbrr_new(enum smbrr_type type,
	unsigned int width, unsigned int height, unsigned int stride,
	enum smbrr_adu adu, const void *src_data);
* \param type New data type
* \param width Image width in pixels
* \param height Image height in pixels
* \param stride Image stride in pixels. width += %4
* \param adu Source data ADU type
* \param src_data source data raw data
* \return Pointer to new data or NULL on failure
*
* Create a new smbrr data from source raw data or a blank data if no
* source is provided.
*/
struct smbrr *smbrr_new(enum smbrr_type type,
	unsigned int width, unsigned int height, unsigned int stride,
	enum smbrr_adu adu, const void *src_data)
{
	struct smbrr *data;
	const struct data_ops* ops;
	size_t size;
	int bytes, err, elems, height_;

	if (width == 0)
		return NULL;

	switch (type) {
	case SMBRR_DATA_1D_UINT32:
		bytes = sizeof(uint32_t);
		size = width * bytes;
		ops = get_1d_ops();
		elems = width;
		height_ = 1;
		break;
	case SMBRR_DATA_2D_UINT32:
		bytes = sizeof(uint32_t);
		size = width * height * bytes;
		ops = get_2d_ops();
		elems = width * height;
		height_ = height;
		break;
	case SMBRR_DATA_1D_FLOAT:
		bytes = sizeof(float);
		size = width * bytes;
		ops = get_1d_ops();
		elems = width;
		height_ = 1;
		break;
	case SMBRR_DATA_2D_FLOAT:
		bytes = sizeof(float);
		size = width * height * bytes;
		ops = get_2d_ops();
		elems = width * height;
		height_ = height;
		break;
	default:
		return NULL;
	}

	data = calloc(1, sizeof(*data));
	if (data == NULL)
		return NULL;

	err = posix_memalign((void**)&data->adu, 32, size);
	if (err < 0) {
		free(data);
		return NULL;
	}
	bzero(data->adu, size);

	data->type = type;
	data->ops = ops;
	data->elems = elems;
	data->width = width;
	data->height = height_;

	if (stride == 0) {
		if (data->width % 4)
			data->stride = data->width + 4 - (data->width % 4);
		else
			data->stride = data->width;
	} else
		data->stride = stride;

	if (src_data == NULL)
		return data;

	switch (adu) {

	case SMBRR_ADU_8:
		switch (type) {
		case SMBRR_DATA_1D_UINT32:
		case SMBRR_DATA_2D_UINT32:
			data->ops->uchar_to_uint(data, src_data);
			break;
		case SMBRR_DATA_1D_FLOAT:
		case SMBRR_DATA_2D_FLOAT:
			data->ops->uchar_to_float(data, src_data);
			break;
		}
		break;

	case SMBRR_ADU_16:
		switch (type) {
		case SMBRR_DATA_1D_UINT32:
		case SMBRR_DATA_2D_UINT32:
			data->ops->ushort_to_uint(data, src_data);
			break;
		case SMBRR_DATA_1D_FLOAT:
		case SMBRR_DATA_2D_FLOAT:
			data->ops->ushort_to_float(data, src_data);
			break;
		}
		break;

	case SMBRR_ADU_32:
		switch (type) {
		case SMBRR_DATA_1D_UINT32:
		case SMBRR_DATA_2D_UINT32:
			data->ops->uint_to_uint(data, src_data);
			break;
		case SMBRR_DATA_1D_FLOAT:
		case SMBRR_DATA_2D_FLOAT:
			data->ops->uint_to_float(data, src_data);
			break;
		}
		break;

	case SMBRR_ADU_FLOAT:
		switch (type) {
		case SMBRR_DATA_1D_UINT32:
		case SMBRR_DATA_2D_UINT32:
			data->ops->float_to_uint(data, src_data);
			break;
		case SMBRR_DATA_1D_FLOAT:
		case SMBRR_DATA_2D_FLOAT:
			data->ops->float_to_float(data, src_data);
			break;
		}
		break;

	default:
		free(data->adu);
		free(data);
		return NULL;
	}

	return data;
}

/*! \fn struct smbrr *smbrr_new_from_area(struct smbrr *src,
	unsigned int x_start, unsigned int y_start, unsigned int x_end,
	unsigned int y_end)
* \param src Source data
* \param x_start X pixel offset for region start
* \param y_start Y pixel offset for region start
* \param x_end X pixel offset for region end
* \param y_end Y pixel offset for region end
* \return Pointer to new data or NULL on failure
*
* Create a new 2D smbrr data from a smaller source data region. This is useful
* for faster 2D processing and reconstructing smaller regions of interest.
*/
struct smbrr *smbrr_new_from_area(struct smbrr *src,
	unsigned int x_start, unsigned int y_start, unsigned int x_end,
	unsigned int y_end)
{
	struct smbrr *data;
	int bytes, width, height, i, err;
	size_t size;

	width = x_end - x_start;
	height = y_end - y_start;

	if (width <= 0 || height <= 0)
		return NULL;

	switch (src->type) {
	case SMBRR_DATA_2D_UINT32:
		bytes = sizeof(uint32_t);
		break;
	case SMBRR_DATA_2D_FLOAT:
		bytes = sizeof(float);
		break;
	default:
		return NULL;
	}

	data = calloc(1, sizeof(*data));
	if (data == NULL)
		return NULL;

	/* make ADU memory aligned on 32 bytes for SIMD */
	size = width * height * bytes;
	err = posix_memalign((void**)&data->adu, 32, size);
	if (err < 0) {
		free(data);
		return NULL;
	}
	bzero(data->adu, size);

	data->ops = get_2d_ops();
	data->elems = width * height;
	data->width = width;
	data->height = height;
	if (data->width % 4)
		data->stride = data->width + 4 - (data->width % 4);
	else
		data->stride = data->width;

	data->type = src->type;

	/* copy each row from src data to new data */
	for (i = y_start; i < y_end; i++) {
		unsigned int offset = i * data->width + x_start;
		memcpy(data->adu + i * width, src->adu + offset,
			width * sizeof(float));
	}

	return data;
}

/*! \fn struct smbrr *smbrr_new_from_section(struct smbrr *src,
	unsigned int start,  unsigned int end)
* \param src Source data
* \param start data offset for region start
* \param end  data offset for region end
* \return Pointer to new data or NULL on failure
*
* Create a new1D smbrr data from a smaller source data region. This is useful
* for faster 1D processing and reconstructing smaller regions of interest.
*/
struct smbrr *smbrr_new_from_section(struct smbrr *src,
	unsigned int start,  unsigned int end)
{
	struct smbrr *data;
	int bytes, width, err;
	size_t size;

	width = end - start;

	if (width <= 0 || width + start >= src->width)
		return NULL;

	switch (src->type) {
	case SMBRR_DATA_1D_UINT32:
		bytes = sizeof(uint32_t);
		break;
	case SMBRR_DATA_1D_FLOAT:
		bytes = sizeof(float);
		break;
	default:
		return NULL;
	}

	data = calloc(1, sizeof(*data));
	if (data == NULL)
		return NULL;

	/* make ADU memory aligned on 32 bytes for SIMD */
	size = width * bytes;
	err = posix_memalign((void**)&data->adu, 32, size);
	if (err < 0) {
		free(data);
		return NULL;
	}
	bzero(data->adu, size);

	data->ops = get_1d_ops();
	data->elems = width;
	data->width = width;
	data->height = 1;
	data->type = src->type;

	if (data->width % 4)
		data->stride = data->width + 4 - (data->width % 4);
	else
		data->stride = data->width;

	/* copy from src data to new data */
	memcpy(data->adu, src->adu + start, width * sizeof(float));

	return data;
}

/*! \fn struct smbrr *smbrr_new_copy(struct smbrr *data)
* \param data Source data.
* \return Pointer to new data or NULL on failure
*
* Create a new smbrr data from source data.
*/
struct smbrr *smbrr_new_copy(struct smbrr *src)
{
	switch (src->type) {
	case SMBRR_DATA_1D_UINT32:
	case SMBRR_DATA_1D_FLOAT:
		return smbrr_new_from_section(src, 0, src->width);
	case SMBRR_DATA_2D_UINT32:
	case SMBRR_DATA_2D_FLOAT:
		return smbrr_new_from_area(src, 0, 0, src->width, src->height);
	default:
		return NULL;
	}
}

/*! \fn void smbrr_free(struct smbrr *data)
* \param data Image to be freed
*
* Free data resources.
*/
void smbrr_free(struct smbrr *data)
{
	if (data == NULL)
		return;

	free(data->adu);
	free(data);
}

/*! \fn int smbrr_get_data(struct smbrr *data, enum smbrr_adu adu,
	void **buf)
* \param data Image
* \param adu ADU type of raw data
* \param buf Pointer to raw data buffer.
* \return 0 on success.
*
* Copy data pixel data to buffer buf and convert it to adu format.
*/
int smbrr_get_data(struct smbrr *data, enum smbrr_adu adu,
	void **buf)
{
	return data->ops->get(data, adu, buf);
}

/*! \fn int smbrr_copy(struct smbrr *dest, struct smbrr *src)
* \param src Source data
* \param dest Destination data.
* \return 0 on success.
*
* Copy the source data to destination
*/
int smbrr_copy(struct smbrr *dest, struct smbrr *src)
{
	if (dest->width != src->width)
		return -EINVAL;
	if (dest->height != src->height)
		return -EINVAL;

	memcpy(dest->adu, src->adu, sizeof(float) * dest->elems);
	return 0;
}

/*! \fn int smbrr_convert(struct smbrr *data,
	enum smbrr_type type)
* \param data Image
* \param type target destination type
* \return 0 on success
*
* Convert an data from one type to another.
*/
int smbrr_convert(struct smbrr *data,
	enum smbrr_type type)
{
	return data->ops->convert(data, type);
}

/*! \fn void smbrr_find_limits(struct smbrr *data,
    float *min, float *max)
* \param data Image
* \param min pointer to store data min pixel value
* \param max pointer to store data max pixel value
*
* Find min and max pixel value limits for data.
*/
void smbrr_find_limits(struct smbrr *data, float *min, float *max)
{
	data->ops->find_limits(data, min, max);
}

/*! \fn void smbrr_normalise(struct smbrr *data, float min,
	float max)
* \param data Image
* \param min minimum pixel value
* \param max maximum pixel value
*
* Normalise data within a defined minimum value and range factor. This
* subtracts minimum from value and then multiplies by factor.
*/
void smbrr_normalise(struct smbrr *data, float min, float max)
{
	data->ops->normalise(data, min, max);
}

/*! \fn void smbrr_add(struct smbrr *a, struct smbrr *b,
	struct smbrr *c)
* \param a Image A
* \param b data B
* \param c Image C
*
* Add pixels in data C from data B and store in data A.
*/
void smbrr_add(struct smbrr *a, struct smbrr *b,
	struct smbrr *c)
{
	a->ops->add(a, b, c);
}

/*! \fn void smbrr_significant_add(struct smbrr *a, struct smbrr *b,
	struct smbrr *c, struct smbrr *s)
* \param a Image A
* \param b data B
* \param c Image C
* \param s Significant data.
*
* Add significant pixels in data C to data B and store in data A.
*/
void smbrr_significant_add(struct smbrr *a, struct smbrr *b,
	struct smbrr *c, struct smbrr *s)
{
	a->ops->add_sig(a, b, c, s);
}

/*! \fn void smbrr_mult_add(struct smbrr *dest, struct smbrr *a,
	struct smbrr *b, float c)
* \param dest Destination Image
* \param a Image A
* \param b data B
* \param c value C
*
* Add pixels in data B multiplied by value from data A and store in
* destination data.
*/
void smbrr_mult_add(struct smbrr *dest, struct smbrr *a,
	struct smbrr *b, float c)
{
	a->ops->mult_add(dest, a, b, c);
}

/*! \fn void smbrr_mult_subtract(struct smbrr *dest, struct smbrr *a,
	struct smbrr *b, float c)
* \param dest Destination Image
* \param a Image A
* \param b data B
* \param c value C
*
* Subtract pixels in data B multiplied by value from data A and store in
* destination data.
*/
void smbrr_mult_subtract(struct smbrr *dest, struct smbrr *a,
	struct smbrr *b, float c)
{
	dest->ops->mult_subtract(dest, a, b, c);
}

/*! \fn void smbrr_subtract(struct smbrr *a, struct smbrr *b,
	struct smbrr *c)
* \param a Image A
* \param b data B
* \param c Image C
*
* Subtract pixels in data C from data B and store in data A.
*/
void smbrr_subtract(struct smbrr *a, struct smbrr *b,
	struct smbrr *c)
{
	a->ops->subtract(a, b, c);
}

/*! \fn void smbrr_significant_subtract(struct smbrr *a, struct smbrr *b,
	struct smbrr *c, struct smbrr *s)
* \param a Image A
* \param b data B
* \param c Image C
* \param s Significant data.
*
* Subtract significant pixels in data C from data B and store in data A.
*/
void smbrr_significant_subtract(struct smbrr *a, struct smbrr *b,
	struct smbrr *c, struct smbrr *s)
{
	a->ops->subtract_sig(a, b, c, s);
}

/*! \fn void smbrr_add_value(struct smbrr *data, float value)
* \param data Image
* \param value Value
*
* Add value to all data pixels.
*/
void smbrr_add_value(struct smbrr *data, float value)
{
	data->ops->add_value(data, value);
}

/*! \fn void smbrr_significant_add_value(struct smbrr *data,
	struct smbrr *sdata, float value)
* \param data Image
* \param sdata Image
* \param value Value
*
* Add value to all significant data pixels.
*/
void smbrr_significant_add_value(struct smbrr *data,
	struct smbrr *sdata, float value)
{
	data->ops->add_value_sig(data, sdata, value);
}

/*! \fn void smbrr_subtract_value(struct smbrr *data, float value)
* \param data Image
* \param value Value
*
* Subtract value from all data pixels.
*/
void smbrr_subtract_value(struct smbrr *data, float value)
{
	data->ops->subtract_value(data, value);
}

/*! \fn void smbrr_mult_value(struct smbrr *data, float value)
* \param data Image
* \param value Value
*
* Multiply all data pixels by value.
*/
void smbrr_mult_value(struct smbrr *data, float value)
{
	data->ops->mult_value(data, value);
}

/*! \fn void smbrr_reset_value(struct smbrr *data, float value)
* \param data Image
* \param value Value
*
* Reset all values in data to value
*/
void smbrr_reset_value(struct smbrr *data, float value)
{
	data->ops->reset_value(data, value);
}

/*! \fn void smbrr_significant_set_sig_value(struct smbrr *data, uint32_t value)
* \param data Image
* \param value reset value
*
* Reset all values in significant data to value
*/
void smbrr_significant_set_sig_value(struct smbrr *data, uint32_t value)
{
	data->ops->set_sig_value(data, value);
}

/*! \fn void smbrr_significant_set_value(struct smbrr *data,
	struct smbrr *sdata, float sig_value)
* \param data Image
* \param sdata Significant Image
* \param sig_value Significant value.
*
* Set data pixels to value if pixel is significant.
*/
void smbrr_significant_set_value(struct smbrr *data,
	struct smbrr *sdata, float sig_value)
{
	data->ops->set_value_sig(data, sdata, sig_value);
}

/*! \fn void smbrr_zero_negative(struct smbrr *data)
* \param data Image
*
* Clear any data pixels with negative values to zero..
*/
void smbrr_zero_negative(struct smbrr *data)
{
	data->ops->clear_negative(data);
}

/*! \fn int smbrr_psf(struct smbrr *src, struct smbrr *dest,
	enum smbrr_wavelet_mask mask)
* \param src Source data
* \param dest Destination Image
* \param mask PSF convolution mask
* \return 0 for success.
*
* Perform Point Spread Function on source data and store in destination data
* using wavelet masks to blur data.
*/
int smbrr_psf(struct smbrr *src, struct smbrr *dest,
	enum smbrr_wavelet_mask mask)
{
	return src->ops->psf(src, dest, mask);
}

/*! \fn int smbrr_get_size(struct smbrr *data)
* \param data Image
* \return Number of data pixels.
*
* Return the number of valid pixels used by data. i.e. width * height.
*/
int smbrr_get_size(struct smbrr *data)
{
	return data->width * data->height;
}

/*! \fn int smbrr_get_bytes(struct smbrr *data)
* \param data Image
* \return Number of data bytes.
*
* Return the number of raw pixels used by data. i.e. stride * height.
*/
int smbrr_get_bytes(struct smbrr *data)
{
	return data->stride * data->height;
}

/*! \fn int smbrr_get_stride(struct smbrr *data)
* \param data Image
* \return Image stride.
*
* Return the number of pixels in data stride.
*/
int smbrr_get_stride(struct smbrr *data)
{
	return data->stride;
}

/*! \fn int smbrr_get_width(struct smbrr *data)
* \param data Image
* \return Image stride.
*
* Return the number of pixels in data width.
*/
int smbrr_get_width(struct smbrr *data)
{
	return data->width;
}

/*! \fn int smbrr_get_height(struct smbrr *data)
* \param data Image
* \return Image height.
*
* Return the number of pixels in data height.
*/
int smbrr_get_height(struct smbrr *data)
{
	return data->height;
}

/*! \fn float smbrr_get_adu_at_posn(struct smbrr *data, int x, int y);
 * \param data Image
 * \param x X coordinate
 * \param y Y coordinate
 * \return Image ADU
 *
 * Get data ADU value at (x,y)
 */
float smbrr_get_adu_at_posn(struct smbrr *data, int x, int y)
{
	int pixel;

	if (x < 0 || x >= data->width)
		return -1.0;
	if (y < 0 || y >= data->height)
		return -1.0;

	pixel = y * data->width + x;
	return data->adu[pixel];
}

/*! \fn float smbrr_get_adu_at_offset(struct smbrr *data, int offset);
 * \param data Image
 * \param offset daat offset
 * \return data ADU
 *
 * Get data ADU value at (offset)
 */
float smbrr_get_adu_at_offset(struct smbrr *data, int offset)
{
	if (offset < 0 || offset >= data->width)
		return -1.0;

	return data->adu[offset];
}
