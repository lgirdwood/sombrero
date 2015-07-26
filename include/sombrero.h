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
 *  Copyright (C) 2012, 2013 Liam Girdwood
 */

/*! \mainpage Sombrero
 *
 * \section intro_sec Introduction
 *
 * Sombrero is a fast wavelet data processing and object detection C library for
 * 1D and 2D data. Sombrero is named after the "Mexican Hat" shape of the
 * wavelet masks used in data convolution and is released under the GNU LGPL
 * library. Sombrero was initially developed for astronomical work but can also be
 * used with any other data that can benefit from wavelet processing.
 *
 * Some of the algorithms in this library are taken from "Astronomical Image
 * and Data Analysis" by Jean-Luc Starck and Fionn Murtoch.
 *
 * \section features Current Features
 *
 * - Support for both 1D and 2D data element types.
 * - Significant element detection with k-sigma clipping. This can reduce the
 *   background noise in an data resulting in a better structure and object
 *   detection. Various K-sigma clipping levels are supported including custom
 *   user defined clipping coefficients.
 * - A'trous Wavelet convolution and deconvolution of data elements. The A'trous
 *   "with holes" data convolution is supported with either a linear or bicubic mask.
 * - Data transformations, i.e. add, subtract, multiply etc. Most general
 *   operators are supported. Can be used for stacking, flats, dark frames.
 * - Detection of structures and objects within data elements. Structures and objects
 *   are detected within data alongside properties like max element values
 *   brightness, average element values, position, size. Simple object de-blending
 *   also performed.
 * - SIMD optimisations for element and wavelet operations using SSE, AVX, AVX2
 *   and FMA on x86 CPUs.
 * - OpenMP support for parallelism within wavelet operations.
 *
 * \section examples Example Code
 *
 * Examples are provided showing the API in action for a variety of tasks and
 * are useful in their own right too for data processing and object detection.
 *
 * The examples include simple Bitmap data support for working with bitmap
 * datas. This is intentional so that Sombrero has no dependencies on data
 * libraries (for FITS, etc) and users are free to choose their own data
 * libraries.
 *
 * \section planned Planned Features
 *
 * - <b>Done</b> SIMD optimisations for Wavelet and data operations. i.e AVX, NEON. A lot
 *   of the data processing is highly aligned with SIMD concepts so will
 *   greatly benefit from SIMD support.
 * - <b>Done</b> OpenMP/Cilk optimisations for improved parallelism. Some library APIs are
 *   good candidates to exploit the parallel capabilities of modern processors.
 *   k-sigma clipping, structure detection.
 * - <b>Done</b> Convert examples to tools. Gives the example code more functionality and
 *   link them to some data libraries for handling common data types
 *   (e.g FITS, raw CCD output)
 * - Improve object detection by creating more merge/de-blend tests.
 * - Use a Neural Network like FANN to enable object classification.
 *
 * \section authors Authors
 *
 * Sombrero is maintained by
 *  <A href="mailto:lgirdwood@gmail.com">Liam Girdwood</A>
 */

#ifndef __SOMBRERO_H__
#define __SOMBRERO_H__

#include <stdint.h>

/*! \file */

/*!
 * \def SMBRR_MAX_SCALES
 * \brief Maximum number of scales for wavelet operations.
 */
#define SMBRR_MAX_SCALES	12

/*!
** Source Data Type
* \enum smbrr_source_type
* \brief Supported source data types.
*
* Internal source data types that are supported for element and wavelet operations.
*/
enum smbrr_source_type {
	SMBRR_SOURCE_UINT8		= 0,	/*!< 8 bits per data pixel */
	SMBRR_SOURCE_UINT16	= 1,	/*!< 16 bits per data pixel */
	SMBRR_SOURCE_UINT32	= 2,	/*!< 32 bits per data pixel */
	SMBRR_SOURCE_FLOAT	= 3,	/*!< 32 bits float per data pixel */
};

/*!
** Functional Data Types
* \enum smbrr_data_type
* \brief Supported Internal Data types.
*
* Supported internal data types. 1D and 2D data in 32 bit float and 1D and 2D
* significant data in 32 bit unsigned int.
*/
enum smbrr_data_type {
	SMBRR_DATA_1D_UINT32 = 0, /*!< 32 bit uint - used by significant 1D data */
	SMBRR_DATA_1D_FLOAT  = 1, /*!< 32 bit float - used by 1D data */
	SMBRR_DATA_2D_UINT32 = 2, /*!< 32 bit uint - used by significant 2D data */
	SMBRR_DATA_2D_FLOAT  = 3, /*!< 32 bit float - used by 2D data */
};

/*!
** Wavelet Convolution Type
* \enum smbrr_conv Wavelet Convolution Type
* \brief Supported wavelet convolution and deconvolution types.
*
* Type of wavelet convolution or deconvolution applied to 1D and 2D data
* elements.
*/
enum smbrr_conv {
	SMBRR_CONV_ATROUS	 = 0, /*!< The A-trous "with holes" convolution */
	SMBRR_CONV_PSF = 1, /*!< The PSF Point Spread Function */
};

/*! \enum smbrr_wavelet_mask
* \brief Wavelet convolution and deconvolution mask
*
* Type of wavelet convolution or deconvolution mask applied to 1D and 2D data
* elements.
*/
enum smbrr_wavelet_mask {
	SMBRR_WAVELET_MASK_LINEAR	= 0, /*!< Linear wavelet convolution */
	SMBRR_WAVELET_MASK_BICUBIC	= 1, /*!< Bi-cubic wavelet convolution */
};

/*! \enum smbrr_clip
* \brief Wavelet data clipping strengths.
*
* Strength of K-sigma data background clipping.
*/
enum smbrr_clip {
	SMBRR_CLIP_VGENTLE	= 0, /*!< Very gentle clipping */
	SMBRR_CLIP_GENTLE	= 1, /*!< Gentle clipping */
	SMBRR_CLIP_NORMAL	= 2, /*!< Normal clipping */
	SMBRR_CLIP_STRONG	= 3, /*!< Strong clipping */
	SMBRR_CLIP_VSTRONG	= 4, /*!< Very strong clipping */
	SMBRR_CLIP_VVSTRONG	= 5, /*!< Very very strong clipping */
};

/*! \enum smbrr_gain
* \brief Wavelet data gain strengths.
*
* Strength of K-sigma data background gain.
*/
enum smbrr_gain {
	SMBRR_GAIN_NONE		= 0,	/*!< No gain */
	SMBRR_GAIN_LOW		= 1,	/*!< Low resolution gain */
	SMBRR_GAIN_MID		= 2,	/*!< Mid resolution gain */
	SMBRR_GAIN_HIGH		= 3,	/*!< High resolution gain */
	SMBRR_GAIN_LOWMID	= 4,	/*!< High/Mid resolution gain */
};

/*! \enum smbrr_object_type
* \brief Object classification
*
* Classifications of detected objects.
*/
enum smbrr_object_type {
	SMBRR_OBJECT_POINT			= 0,	/*!< Point like object Detected */
	SMBRR_OBJECT_EXTENDED		= 1,	/*!< Exteded / Diffuse object detected */
};

/*! \struct smbrr
* \brief Sombrero data context.
*
* Context for internal 1D or 2D data elements. 1D elements can be
* used to represent a signal whilst 2D elements are used for images.
*/
struct smbrr;


/*! \struct smbrr_coord
* \brief Coordinates.
*
* 1D and 2D positional coordinates than can be used to reference individual data
* elements or detected structures/objects within data.
*/
struct smbrr_coord {
	unsigned int x;		/*!< 2D X coordinate / 1D position offset  */
	unsigned int y;		/*!< 2D Y coordinate */
};

/*! \struct smbrr_object
* \brief Object detected in 1D or 2D data elements.
*
* Represents detected "object" within 1D and 2D data. The object can be classified
* and it's position within the data set is known.  The Position Angle (PA), radius,
* anullus are also known for detected 2D objects. Background noise and SNR is
* also calculated.
*/
struct smbrr_object {
	unsigned int id;	/*!< Object ID. Incrementing on brightest/largest = 0 */
	enum smbrr_object_type type;	/*!< Object classification */

	/* 2D object positional bounds */
	struct smbrr_coord pos;	/*!< Object data coordinates for max pixel */
	struct smbrr_coord minXy;	/*!< Object data min X coordinate */
	struct smbrr_coord minxY;	/*!< Object data min Y coordinate */
	struct smbrr_coord maxXy;	/*!< Object data max X coordinate */
	struct smbrr_coord maxxY;	/*!< Object data max Y coordinate */
	float pa;				/*!< Position angle  */

	/* object aperture */
	float object_adu;		/*!< Sum of all object pixels values */
	float object_radius;	/*!< Object radius in pixels */
	unsigned int object_area;		/*!< Object area in pixels */
	float snr;		/*!< Signal to Noise Ration */
	float error;		/*!< Error in SNR */

	/* object anullus (background) */
	unsigned int background_area; /*!< Count of background pixels in annulus */
	float background_adu;	/*!< Total of background pixels in annulus */

	/* statistical data */
	float max_adu;			/*!< Maximum object pixel value */
	float mean_adu;			/*!< Mean value of pixels */
	float sigma_adu;		/*!< Standard deviation of pixels */
	float mag_delta;		/*!< Magnitude difference to brightest object */
	unsigned int scale;		/*!< Object wavelet scale */
};

/*! \struct smbrr_clip_coeff
* \brief Custom K-sigma cliping coefficients for each scale.
*
* Array of K-sigma clipping coefficients used to clip each wavelet scale.
*/
struct smbrr_clip_coeff {
	float coeff[SMBRR_MAX_SCALES - 1];	/*!< clipping coefficient for scale */
};


/*! \defgroup data Processing 1D and 2D data element arrays.
*
* 1D and 2D Data element manipulation and management.
*/

/*
 * Data Construction and destruction.
 */

/*! \fn struct smbrr *smbrr_new(enum smbrr_data_type type,
	unsigned int width, unsigned int height, unsigned int stride,
	enum smbrr_source_type adu, const void *data);
* \brief Create a new data element context for use with element operations.
* \ingroup data
*/
struct smbrr *smbrr_new(enum smbrr_data_type type,
	unsigned int width, unsigned int height, unsigned int stride,
	enum smbrr_source_type adu, const void *data);

/*! \fn struct smbrr *smbrr_new_from_area(struct smbrr *s,
	unsigned int x_start, unsigned int y_start, unsigned int x_end,
	unsigned int y_end);
* \brief Create a new data element context from an area within an existing 2D data
* element context.
* \ingroup data
*/
struct smbrr *smbrr_new_from_area(struct smbrr *s,
	unsigned int x_start, unsigned int y_start, unsigned int x_end,
	unsigned int y_end);

/*! \fn struct smbrr *smbrr_new_from_section(struct smbrr *s,
	unsigned int start,  unsigned int end)
* \brief Create a new data element context from a section within an existing 1D data
* element context.
* \ingroup data
*/
struct smbrr *smbrr_new_from_section(struct smbrr *s,
	unsigned int start,  unsigned int end);

/*! \fn struct smbrr *smbrr_new_copy(struct smbrr *s)
* \param s Source data context.
* \brief Create a new data element context by copying an existing context and data.
* \ingroup data
*/
struct smbrr *smbrr_new_copy(struct smbrr *src);

/*! \fn void smbrr_free(struct smbrr *s);
 * \brief Frees data element context and all data elements.
 * \ingroup data
 */
void smbrr_free(struct smbrr *smbrr);

/*
 * Element information.
 */

/*! \fn int smbrr_get_data(struct smbrr *s, enum smbrr_source_type adu,
	void **data);
 * \brief Get the raw data elements from element context.
 * \ingroup data
 */
int smbrr_get_data(struct smbrr *s, enum smbrr_source_type adu,
	void **data);

/*! \fn int smbrr_get_size(struct smbrr *s);
 * \brief Get number of data elements in the context.
 * \ingroup data
 */
int smbrr_get_size(struct smbrr *s);

/*! \fn int smbrr_get_bytes(struct smbrr *s);
 * \brief Get number of bytes used by the data elements within this context.
 * \ingroup data
 */
int smbrr_get_bytes(struct smbrr *s);

/*! \fn int smbrr_get_stride(struct smbrr *s)
* \brief Get the data stride for the original 2D data that was used to create the
* elements within this data context.
* \ingroup data
*/
int smbrr_get_stride(struct smbrr *s);

/*! \fn int smbrr_get_width(struct smbrr *s);
* \brief Get the 2D width of the data elements or 1D size of the elements.
* \ingroup data
*/
int smbrr_get_width(struct smbrr *s);

/*! \fn int smbrr_get_height(struct smbrr *s)
* \brief Get the 2D height of the data elements.
* \ingroup data
*/
int smbrr_get_height(struct smbrr *s);

/*! \fn void smbrr_find_limits(struct smbrr *s, float *min,
 * float *max);
 * \brief Find the upper and lower values within the data elements.
 * \ingroup data
 */
void smbrr_find_limits(struct smbrr *s, float *min, float *max);

/*! \fn float smbrr_get_mean(struct smbrr *s);
 * \brief Calculate the mean value of all elements.
 * \ingroup data
 */
float smbrr_get_mean(struct smbrr *s);

/*! \fn float smbrr_get_sigma(struct smbrr *s, float mean);
 * \brief Calculate the standard deviation (sigma) of the data elements.
 * \ingroup data
 */
float smbrr_get_sigma(struct smbrr *s, float mean);

/*! \fn float smbrr_significant_get_mean(struct smbrr *s,
	struct smbrr *sdata);
 * \brief Calculate the mean value for only the significant data elements.
 * \ingroup data
 */
float smbrr_significant_get_mean(struct smbrr *s,
	struct smbrr *sdata);

/*! \fn float smbrr_significant_get_sigma(struct smbrr *s,
	struct smbrr *sdata, float mean);
 * \brief Calculate the standard deviation (sigma) for only the significant data
 * elements.
 * \ingroup data
 */
float smbrr_significant_get_sigma(struct smbrr *s,
	struct smbrr *sdata, float mean);

/*! \fn float smbrr_get_norm(struct smbrr *s);
 * \brief Calculate the norm value of the data elements.
 * \ingroup data
 */
float smbrr_get_norm(struct smbrr *s);

/*
 * Data Transformations
 */

/*! \fn float void smbrr_normalise(struct smbrr *s,
 * float min, float max);
 * \brief Normalise data elements between new min and max values.
 * \ingroup data
 */
void smbrr_normalise(struct smbrr *s, float min, float max);

/*! \fn void smbrr_add(struct smbrr *a, struct smbrr *b,
	struct smbrr *c);
 * \brief Add all data elements from several contexts.  A = B + C
 * \ingroup data
 */
void smbrr_add(struct smbrr *a, struct smbrr *b,
	struct smbrr *c);

/*! \fn void smbrr_significant_add_value(struct smbrr *s,
	struct smbrr *sdata, float value);
* \brief Add a scalar value to only significant data elements.
* \ingroup data
*/
void smbrr_significant_add_value(struct smbrr *s,
	struct smbrr *sdata, float value);

/*! \fn void smbrr_significant_add(struct smbrr *a, struct smbrr *b,
	struct smbrr *c, struct smbrr *s);
 * \brief Add significant data elements.  A = B + (significant) C
 * \ingroup data
 */
void smbrr_significant_add(struct smbrr *a, struct smbrr *b,
	struct smbrr *c, struct smbrr *s);

/*! \fn void smbrr_subtract(struct smbrr *a, struct smbrr *b,
	struct smbrr *c);
 * \brief Subtract all data elements from several contexts. A = B - C
 * \ingroup data
 */
void smbrr_subtract(struct smbrr *a, struct smbrr *b,
	struct smbrr *c);

/*! \fn void smbrr_significant_subtract(struct smbrr *a, struct smbrr *b,
	struct smbrr *c, struct smbrr *s);
 * \brief Add significant data elements from several contexts. A = B - (significant) C
 * \ingroup data
 */
void smbrr_significant_subtract(struct smbrr *a, struct smbrr *b,
	struct smbrr *c, struct smbrr *s);

/*! \fn void smbrr_add_value(struct smbrr *a, float value);
 * \brief Add scalar value to data elements. A = A + real value
 * \ingroup data
 */
void smbrr_add_value(struct smbrr *a, float value);

/*! \fn void smbrr_subtract_value(struct smbrr *a, float value);
 * \brief Subtract scaler value from all data elements. A = A - real value
 * \ingroup data
 */
void smbrr_subtract_value(struct smbrr *a, float value);

/*! \fn void smbrr_mult_value(struct smbrr *a, float value);
 * \brief Multiply scalar all data elements by scalar value. A = A * real value
 * \ingroup data
 */
void smbrr_mult_value(struct smbrr *a, float value);

/*! \fn void smbrr_set_value(struct smbrr *a, float value);
 * \brief Set all data elements to scalar value. A = real value
 * \ingroup data
 */
void smbrr_set_value(struct smbrr *a, float value);

/*! \fn void smbrr_significant_set_value(struct smbrr *a,
	struct smbrr *s, float value);
 * \brief Set only significant elements to scalar value. (significant) A = value
 * \ingroup data
 */
void smbrr_significant_set_value(struct smbrr *a,
	struct smbrr *s, float value);

/*! \fn void smbrr_significant_set_svalue(struct smbrr *a, uint32_t value);
 * \brief Set value of significant elements.
 * \ingroup data
 */
void smbrr_significant_set_svalue(struct smbrr *a, uint32_t value);

/*! \fn int smbrr_convert(struct smbrr *a, enum smbrr_data_type type);;
 * \brief Convert data elements A to new type.
 * \ingroup data
 */
int smbrr_convert(struct smbrr *a, enum smbrr_data_type type);

/*! \fn void smbrr_zero_negative(struct smbrr *a)
 * \brief Set all elements with negative values to zero.
 * \ingroup data
 */
void smbrr_zero_negative(struct smbrr *a);

/*! \fn void smbrr_abs(struct smbrr *a);
 * \brief Set all data elements to absolute values
 * \ingroup data
 */
void smbrr_abs(struct smbrr *a);

/*! \fn int smbrr_copy(struct smbrr *dest, struct smbrr *src);
 * \brief Copy data elements from one context to another.
 * \ingroup data
 */
int smbrr_copy(struct smbrr *dest, struct smbrr *src);

/*! \fn int smsmbrr_significant_copy(struct smbrr *dest, struct smbrr *src,
	struct smbrr *sig);
 * \brief Copy data significant elements from one context to another.
 * \ingroup data
 */
int smbrr_significant_copy(struct smbrr *dest, struct smbrr *src,
	struct smbrr *sig);

/*! \fn void smbrr_mult_add(struct smbrr *dest, struct smbrr *a,
	struct smbrr *b, float c)
 * \brief Image A = A + B * value C
 * \ingroup data
 */
void smbrr_mult_add(struct smbrr *dest, struct smbrr *a,
	struct smbrr *b, float c);

/*! \fn void smbrr_mult_subtract(struct smbrr *dest, struct smbrr *a,
	struct smbrr *b, float c);
 * \brief Multiple data elements by scalar value and subtract from another context.
 *  A = A - B * value C
 * \ingroup data
 */
void smbrr_mult_subtract(struct smbrr *dest, struct smbrr *a,
	struct smbrr *b, float c);

/*! \fn void smbrr_anscombe(struct smbrr *s, float gain,
 * float bias, float readout);
 * \brief Perform Anscombe noise reduction on data.
 * \ingroup data
 */
void smbrr_anscombe(struct smbrr *s, float gain, float bias,
	float readout);

/*! \fn void smbrr_significant_new(struct smbrr *a,
	struct smbrr *s, float sigma);
 * \brief Set significant elements in s if corresponding elements in a > sigma.
 * \ingroup data
 */
void smbrr_significant_new(struct smbrr *a,
	struct smbrr *s, float sigma);

/*! \fn int smbrr_psf(struct smbrr *src, struct smbrr *dest,
	enum smbrr_wavelet_mask mask);
 * \brief Perform PSF on source data
 * \ingroup data
 */
int smbrr_psf(struct smbrr *src, struct smbrr *dest,
	enum smbrr_wavelet_mask mask);

/*! \fn float smbrr_get_adu_at_posn(struct smbrr *s, int x, int y);
 * \brief Get data element value at coordinate (x,y) in 2D data.
 * \ingroup data
 */
float smbrr_get_adu_at_posn(struct smbrr *s, int x, int y);

/*! \fn float smbrr_get_adu_at_offset(struct smbrr *s, int offset);
 * \brief Get data element value at offset in 1D data.
 * \ingroup data
 */
float smbrr_get_adu_at_offset(struct smbrr *s, int offset);

/*! \fn int smbrr_reconstruct(struct smbrr *O,
	enum smbrr_wavelet_mask mask, float threshold, int scales,
	enum smbrr_clip sigma_clip);
 * \brief Reconstruct an data without noise.
 * \ingroup data
 */
int smbrr_reconstruct(struct smbrr *O,
	enum smbrr_wavelet_mask mask, float threshold, int scales,
	enum smbrr_clip sigma_clip);

/*! \defgroup wavelet Wavelet 1D and 2D Convolutions
*
* Wavelet manipulation and management.
*/

/*! \fn struct smbrr_wavelet *smbrr_wavelet_new(struct smbrr *s,
	unsigned int num_scales);
 * \brief Create new wavelet context and load elements from data context.
 * \ingroup wavelet
 */
struct smbrr_wavelet *smbrr_wavelet_new(struct smbrr *s,
	unsigned int num_scales);

/*! \fn struct smbrr_wavelet *smbrr_wavelet_new_from_object(
	 struct smbrr_object *object);
 * \brief Create new wavelet context and load elements from detected object.
 * \ingroup wavelet
 */
struct smbrr_wavelet *smbrr_wavelet_new_from_object(struct smbrr_object *object);

/*! \fn void smbrr_wavelet_free(struct smbrr_wavelet *w);
 * \brief Free wavelet context.
 * \ingroup wavelet
 */
void smbrr_wavelet_free(struct smbrr_wavelet *w);

/*! \fn int smbrr_wavelet_convolution(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask);
 * \brief Perform wavelet convolution.
 * \ingroup wavelet
 */
int smbrr_wavelet_convolution(struct smbrr_wavelet *w, enum smbrr_conv conv,
	enum smbrr_wavelet_mask mask);

/*! \fn int smbrr_wavelet_significant_convolution(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask);
 * \brief Perform wavelet convolution on significant pixels only.
 * \ingroup wavelet
 */
int smbrr_wavelet_significant_convolution(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask);

/*! \fn int smbrr_wavelet_deconvolution(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask);
 * \brief Perform wavelet deconvolution.
 * \ingroup wavelet
 */
int smbrr_wavelet_deconvolution(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask);

/*! \fn int smbrr_wavelet_significant_deconvolution(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask, enum smbrr_gain gain);
 * \brief Perform wavelet deconvolution using significant pixels only.
 * \ingroup wavelet
 */
int smbrr_wavelet_significant_deconvolution(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask, enum smbrr_gain gain);

/*! \fn int smbrr_wavelet_deconvolution_object(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask,
	struct smbrr_object *object);
 * \brief Perform wavelet deconvolution on object using significant pixels only.
 * \ingroup wavelet
 */
int smbrr_wavelet_deconvolution_object(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask,
	struct smbrr_object *object);

/*! \fn struct smbrr *smbrr_wavelet_get_scale(
	struct smbrr_wavelet *w, unsigned int scale);
 * \brief Get scale data elements from wavelet at scale.
 * \ingroup wavelet
 */
struct smbrr *smbrr_wavelet_get_scale(struct smbrr_wavelet *w,
	unsigned int scale);

/*! \fn struct smbrr *smbrr_wavelet_get_wavelet(
	struct smbrr_wavelet *w, unsigned int scale);
 * \brief Get wavelet data elements from wavelet at scale.
 * \ingroup wavelet
 */
struct smbrr *smbrr_wavelet_get_wavelet(struct smbrr_wavelet *w,
	unsigned int scale);

/*! \fn struct smbrr *smbrr_wavelet_get_significant(
	struct smbrr_wavelet *w, unsigned int scale);
 * \brief Get significant data elements from wavelet at scale.
 * \ingroup wavelet
 */
struct smbrr *smbrr_wavelet_get_significant(struct smbrr_wavelet *w,
	unsigned int scale);

/*! \fn void smbrr_wavelet_add(struct smbrr_wavelet *a,
    struct smbrr_wavelet *b, struct smbrr_wavelet *c);
 * \brief Add wavelet contexts.  A = B + C.
 * \ingroup wavelet
 */
void smbrr_wavelet_add(struct smbrr_wavelet *a, struct smbrr_wavelet *b,
	struct smbrr_wavelet *c);

/*! \fn void smbrr_wavelet_subtract(struct smbrr_wavelet *a,
	struct smbrr_wavelet *b, struct smbrr_wavelet *c);
 * \brief Wavelet A = B - C.
 * \ingroup wavelet
 */
void smbrr_wavelet_subtract(struct smbrr_wavelet *a, struct smbrr_wavelet *b,
	struct smbrr_wavelet *c);

/*! \fn void smbrr_wavelet_significant_subtract(struct smbrr_wavelet *a,
	struct smbrr_wavelet *b, struct smbrr_wavelet *c);
 * \brief Wavelet A = B - sig(C).
 * \ingroup wavelet
 */
void smbrr_wavelet_significant_subtract(struct smbrr_wavelet *a,
	struct smbrr_wavelet *b, struct smbrr_wavelet *c);

/*! \fn void smbrr_wavelet_significant_add(struct smbrr_wavelet *a,
    struct smbrr_wavelet *b, struct smbrr_wavelet *c);
 * \brief Add wavelet contexts.  A = B + (sig)C.
 * \ingroup wavelet
 */
void smbrr_wavelet_significant_add(struct smbrr_wavelet *a,
	struct smbrr_wavelet *b, struct smbrr_wavelet *c);

/*! \fn int smbrr_wavelet_new_significant(struct smbrr_wavelet *w,
	enum smbrr_clip sigma_clip);
 * \brief Create new significance scales for wavlet.
 * \ingroup wavelet
 */
int smbrr_wavelet_new_significant(struct smbrr_wavelet *w,
	enum smbrr_clip sigma_clip);

/*! \fn int smbrr_wavelet_ksigma_clip(struct smbrr_wavelet *w,
    enum smbrr_clip clip, float sig_delta);
 * \brief K sigma clip each wavelet scale.
 * \ingroup wavelet
 */
int smbrr_wavelet_ksigma_clip(struct smbrr_wavelet *w, enum smbrr_clip clip,
	float sig_delta);

/*! \fn int smbrr_wavelet_ksigma_clip_custom(struct smbrr_wavelet *w,
	struct smbrr_clip_coeff *coeff, float sig_delta);
 * \brief K sigma clip each wavelet scale with custom coefficients.
 * \ingroup wavelet
 */
int smbrr_wavelet_ksigma_clip_custom(struct smbrr_wavelet *w,
	struct smbrr_clip_coeff *coeff, float sig_delta);

/*! \fn int smbrr_wavelet_structure_find(struct smbrr_wavelet *w,
	unsigned int scale);
 * \brief Find structures in wavelet scale.
 * \ingroup wavelet
 */
int smbrr_wavelet_structure_find(struct smbrr_wavelet *w, unsigned int scale);

/*! \fn int smbrr_wavelet_structure_connect(struct smbrr_wavelet *w,
		unsigned int start_scale, unsigned int end_scale);
 * \brief Connect structures between wavelet scales.
 * \ingroup wavelet
 */
int smbrr_wavelet_structure_connect(struct smbrr_wavelet *w,
		unsigned int start_scale, unsigned int end_scale);

/*! \fn struct smbrr_object *smbrr_wavelet_object_get(struct smbrr_wavelet *w,
	unsigned int object_id);
* \brief Get wavelet object.
* \ingroup wavelet
*/
struct smbrr_object *smbrr_wavelet_object_get(struct smbrr_wavelet *w,
	unsigned int object_id);

/*! \fn void smbrr_wavelet_object_free_all(struct smbrr_wavelet *w)
* \brief Free all wavelet structures and objects.
* \ingroup wavelet
*/
void smbrr_wavelet_object_free_all(struct smbrr_wavelet *w);

/*! \fn int smbrr_wavelet_object_get_data(struct smbrr_wavelet *w,
		struct smbrr_object *object, struct smbrr **data);
* \brief Get wavelet object data.
* \ingroup wavelet
*/
int smbrr_wavelet_object_get_data(struct smbrr_wavelet *w,
		struct smbrr_object *object, struct smbrr **data);

/*! \fn struct smbrr_object *smbrr_wavelet_get_object_at_posn(struct smbrr_wavelet *w,
		int x, int y)
* \brief Get object at position (x,y).
* \ingroup wavelet
*/
struct smbrr_object *smbrr_wavelet_get_object_at_posn(struct smbrr_wavelet *w,
		int x, int y);

/*! \fn int smbrr_wavelet_set_dark_mean(struct smbrr_wavelet *w,
 * 	float dark);
* \brief Set average background value.
* \ingroup wavelet
*/
int smbrr_wavelet_set_dark_mean(struct smbrr_wavelet *w,
		float dark);

/*! \fn void smbrr_wavelet_set_ccd(struct smbrr_wavelet *w, float gain, float bias,
	float readout)
* \brief Set CCD configuration
* \ingroup wavelet
*/
void smbrr_wavelet_set_ccd(struct smbrr_wavelet *w, float gain, float bias,
	float readout);

/*! \fn int smbrr_wavelet_set_elems(struct smbrr_wavelet *w, struct smbrr *s);
* \brief Set wavelent data elements
* \ingroup wavelet
*/
int smbrr_wavelet_set_elems(struct smbrr_wavelet *w, struct smbrr *s);

#endif
