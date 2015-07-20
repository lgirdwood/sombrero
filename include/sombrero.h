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
 * astronomical datas. Sombrero is named after the "Mexican Hat" shape of the
 * wavelet masks used in data convolution and is released under the GNU LGPL
 * library.
 *
 * Some of the algorithms in this library are taken from "Astronomical Image
 * and Data Analysis" by Jean-Luc Starck and Fionn Murtoch.
 *
 * \section features Current Features
 *
 * - Significant pixel detection with k-sigma clipping. This can reduce the
 *   background noise in an data resulting in a better structure and object
 *   detection. Various K-sigma clipping levels are supported including custom
 *   user defined clipping coefficients.
 * - A'trous Wavelet convolution and deconvolution of datas. The A'trous
 *   "with holes" data convolution is supported with either a linear or
 *   bicubic mask.
 * - Image transformations, i.e. add, subtract, multiply etc. Most general
 *   operators are supported. Can be used for stacking, flats, dark frames.
 * - Detection of structures and objects within datas. Structures and objects
 *   are detected within datas alongside properties like max pixel, brightness,
 *   average pixel brightness, position, size. Simple object de-blending
 *   also performed.
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
 * - SIMD optimisations for Wavelet and data operations. i.e AVX, NEON. A lot
 *   of the data processing is highly aligned with SIMD concepts so will
 *   greatly benefit from SIMD support.
 * - OpenMP/Cilk optimisations for improved parallelism. Some library APIs are
 *   good candidates to exploit the parallel capabilities of modern processors.
 *   k-sigma clipping, structure detection.
 * - Convert examples to tools. Gives the example code more functionality and
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
 * \brief Max number of wavelet scales.
 */
#define SMBRR_MAX_SCALES	12

/*!
** ADU Type
* \enum smbrr_adu
* \brief Supported CCD ADU types.
*
* Number of CCD digitisation bits per pixel.
*/
enum smbrr_adu {
	SMBRR_ADU_8		= 0,	/*!< 8 bits per data pixel */
	SMBRR_ADU_16	= 1,	/*!< 16 bits per data pixel */
	SMBRR_ADU_32	= 2,	/*!< 32 bits per data pixel */
	SMBRR_ADU_FLOAT	= 3,	/*!< 32 bits float per data pixel */
};

/*!
** Data Type
* \enum smbrr_type
* \brief Supported Data types.
*
* Type of data and tranforms carried out by sombrero.
*/
enum smbrr_type {
	SMBRR_DATA_1D_UINT32 = 0, /*!< uint 32 - used by significant signal data */
	SMBRR_DATA_1D_FLOAT  = 1, /*!< float - used by signal */
	SMBRR_DATA_2D_UINT32 = 2, /*!< uint 32 - used by significant data pixels */
	SMBRR_DATA_2D_FLOAT  = 3, /*!< float - used by data pixels */
};

/*!
** Wavelet Convolution Type
* \enum smbrr_conv
* \brief Supported wavelet convolution types.
*
* Type of wavelet convolution or deconvolution applied to an data.
*/
enum smbrr_conv {
	SMBRR_CONV_ATROUS	= 0, /*!< The A-trous "with holes" convolution */
};

/*! \enum smbrr_wavelet_mask
* \brief Wavelet convolution mask
*
* Type of wavelet convolution or deconvolution mask applied to an data.
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
* \brief data.
*
* Sombrero data structure containing data representation and internal runtime data.
*/
struct smbrr;


/*! \struct smbrr_coord
* \brief Coordinates.
*
* Structure and object data coordinates.
*/
struct smbrr_coord {
	unsigned int x;		/*!< Signal / Image X coordinate  */
	unsigned int y;		/*!< Image Y coordinate */
};

/*! \struct smbrr_object
* \brief Detected Object.
*
* Object detected in data.
*/
struct smbrr_object {
	unsigned int id;	/*!< Object ID. Brightest = 0 */
	enum smbrr_object_type type;	/*!< Object classification */

	/* positional bounds */
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
	float snr;
	float error;

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


/*! \defgroup data Data
*
* Data manipulation and management.
*/

/*
 * Data Construction and destruction.
 */

/*! \fn struct smbrr *smbrr_new(enum smbrr_type type,
	unsigned int width, unsigned int height, unsigned int stride,
	enum smbrr_adu adu, const void *img);
* \brief Create a new data.
* \ingroup data
*/
struct smbrr *smbrr_new(enum smbrr_type type,
	unsigned int width, unsigned int height, unsigned int stride,
	enum smbrr_adu adu, const void *data);

/*! \fn struct smbrr *smbrr_new_from_area(struct smbrr *data,
	unsigned int x_start, unsigned int y_start, unsigned int x_end,
	unsigned int y_end);
* \brief Create a new data from another data region.
* \ingroup data
*/
struct smbrr *smbrr_new_from_area(struct smbrr *data,
	unsigned int x_start, unsigned int y_start, unsigned int x_end,
	unsigned int y_end);

/*! \fn struct smbrr *smbrr_new_from_section(struct smbrr *data,
	unsigned int start,  unsigned int end)
* \brief Create a new data from another data region.
* \ingroup data
*/
struct smbrr *smbrr_new_from_section(struct smbrr *data,
	unsigned int start,  unsigned int end);

/*! \fn struct smbrr *smbrr_new_copy(struct smbrr *data)
* \param data Source data.
* \brief Create a new smbrr data from source data.
* \ingroup data
*/
struct smbrr *smbrr_new_copy(struct smbrr *src);

/*! \fn void smbrr_free(struct smbrr *data);
 * \brief Free an data.
 * \ingroup data
 */
void smbrr_free(struct smbrr *smbrr);

/*
 * Image information.
 */

/*! \fn int smbrr_get_data(struct smbrr *data, enum smbrr_adu adu,
	void **img);
 * \brief Get raw data from an data.
 * \ingroup data
 */
int smbrr_get_data(struct smbrr *data, enum smbrr_adu adu,
	void **img);

/*! \fn int smbrr_get_size(struct smbrr *data);
 * \brief Get number of pixels in data.
 * \ingroup data
 */
int smbrr_get_size(struct smbrr *data);

/*! \fn int smbrr_get_bytes(struct smbrr *data);
 * \brief Get number of bytes for raw data.
 * \ingroup data
 */
int smbrr_get_bytes(struct smbrr *data);

/*! \fn int smbrr_get_stride(struct smbrr *data)
* \brief Return the number of pixels in data stride.
* \ingroup data
*/
int smbrr_get_stride(struct smbrr *data);

/*! \fn int smbrr_get_width(struct smbrr *data);
* \brief Return the number of pixels in data width.
* \ingroup data
*/
int smbrr_get_width(struct smbrr *data);

/*! \fn int smbrr_get_height(struct smbrr *data)
* \brief Return the number of pixels in data height.
* \ingroup data
*/
int smbrr_get_height(struct smbrr *data);

/*! \fn void smbrr_find_limits(struct smbrr *data, float *min,
 * float *max);
 * \brief Find data limits.
 * \ingroup data
 */
void smbrr_find_limits(struct smbrr *data, float *min, float *max);

/*! \fn float smbrr_get_mean(struct smbrr *data);
 * \brief Get mean pixel value of data.
 * \ingroup data
 */
float smbrr_get_mean(struct smbrr *data);

/*! \fn float smbrr_get_sigma(struct smbrr *data, float mean);
 * \brief Get pixel standard deviation of data.
 * \ingroup data
 */
float smbrr_get_sigma(struct smbrr *data, float mean);

/*! \fn float smbrr_significant_get_mean(struct smbrr *data,
	struct smbrr *sdata);
 * \brief Get data mean for significant pixels.
 * \ingroup data
 */
float smbrr_significant_get_mean(struct smbrr *data,
	struct smbrr *sdata);

/*! \fn float smbrr_significant_get_sigma(struct smbrr *data,
	struct smbrr *sdata, float mean);
 * \brief Get data standard deviation for significant pixels.
 * \ingroup data
 */
float smbrr_significant_get_sigma(struct smbrr *data,
	struct smbrr *sdata, float mean);

/*! \fn float smbrr_get_norm(struct smbrr *data);
 * \brief Get data norm.
 * \ingroup data
 */
float smbrr_get_norm(struct smbrr *data);

/*
 * Image Transformations
 */

/*! \fn float void smbrr_normalise(struct smbrr *data,
 * float min, float max);
 * \brief Normalise data pixels between new min and max values.
 * \ingroup data
 */
void smbrr_normalise(struct smbrr *data, float min, float max);

/*! \fn void smbrr_add(struct smbrr *a, struct smbrr *b,
	struct smbrr *c);
 * \brief Image A = B + C
 * \ingroup data
 */
void smbrr_add(struct smbrr *a, struct smbrr *b,
	struct smbrr *c);

/*! \fn void smbrr_significant_add_value(struct smbrr *data,
	struct smbrr *sdata, float value);
* \brief If pixel significant then Image A += value
* \ingroup data
*/
void smbrr_significant_add_value(struct smbrr *data,
	struct smbrr *sdata, float value);

/*! \fn void smbrr_significant_add(struct smbrr *a, struct smbrr *b,
	struct smbrr *c, struct smbrr *s);
 * \brief Image A = B + (significant) C
 * \ingroup data
 */
void smbrr_significant_add(struct smbrr *a, struct smbrr *b,
	struct smbrr *c, struct smbrr *s);

/*! \fn void smbrr_subtract(struct smbrr *a, struct smbrr *b,
	struct smbrr *c);
 * \brief Image A = B - C
 * \ingroup data
 */
void smbrr_subtract(struct smbrr *a, struct smbrr *b,
	struct smbrr *c);

/*! \fn void smbrr_significant_subtract(struct smbrr *a, struct smbrr *b,
	struct smbrr *c, struct smbrr *s);
 * \brief Image A = B - (significant) C
 * \ingroup data
 */
void smbrr_significant_subtract(struct smbrr *a, struct smbrr *b,
	struct smbrr *c, struct smbrr *s);

/*! \fn void smbrr_add_value(struct smbrr *a, float value);
 * \brief Image A = A + real value
 * \ingroup data
 */
void smbrr_add_value(struct smbrr *a, float value);

/*! \fn void smbrr_subtract_value(struct smbrr *a, float value);
 * \brief Image A = A - real value
 * \ingroup data
 */
void smbrr_subtract_value(struct smbrr *a, float value);

/*! \fn void smbrr_mult_value(struct smbrr *a, float value);
 * \brief Image A = A * real value
 * \ingroup data
 */
void smbrr_mult_value(struct smbrr *a, float value);

/*! \fn void smbrr_reset_value(struct smbrr *a, float value);
 * \brief Image A = real value
 * \ingroup data
 */
void smbrr_reset_value(struct smbrr *a, float value);

/*! \fn void smbrr_significant_set_value(struct smbrr *a,
	struct smbrr *s, float value);
 * \brief Image (significant) A = value
 * \ingroup data
 */
void smbrr_significant_set_value(struct smbrr *a,
	struct smbrr *s, float value);

/*! \fn int smbrr_convert(struct smbrr *a,
 * enum smbrr_type type);
 * \brief Convert data A to new type.
 * \ingroup data
 */
int smbrr_convert(struct smbrr *a, enum smbrr_type type);

/*! \fn void smbrr_significant_set_sig_value(struct smbrr *a, uint32_t value);
 * \brief Image (significant) A = value
 * \ingroup data
 */
void smbrr_significant_set_sig_value(struct smbrr *a, uint32_t value);

/*! \fn void smbrr_zero_negative(struct smbrr *a)
 * \brief Set Image A negative pixels to zero.
 * \ingroup data
 */
void smbrr_zero_negative(struct smbrr *a);

/*! \fn void smbrr_abs(struct smbrr *a);
 * \brief Set data elements to absolute values
 * \ingroup data
 */
void smbrr_abs(struct smbrr *a);

/*! \fn int smbrr_copy(struct smbrr *dest, struct smbrr *src);
 * \brief Image dest = src.
 * \ingroup data
 */
int smbrr_copy(struct smbrr *dest, struct smbrr *src);

/*! \fn void smbrr_mult_add(struct smbrr *dest, struct smbrr *a,
	struct smbrr *b, float c)
 * \brief Image A = A + B * value C
 * \ingroup data
 */
void smbrr_mult_add(struct smbrr *dest, struct smbrr *a,
	struct smbrr *b, float c);

/*! \fn void smbrr_mult_subtract(struct smbrr *dest, struct smbrr *a,
	struct smbrr *b, float c);
 * \brief Image A = A - B * value C
 * \ingroup data
 */
void smbrr_mult_subtract(struct smbrr *dest, struct smbrr *a,
	struct smbrr *b, float c);

/*! \fn void smbrr_anscombe(struct smbrr *data, float gain,
 * float bias, float readout);
 * \brief Perform Anscombe noise reduction on Image.
 * \ingroup data
 */
void smbrr_anscombe(struct smbrr *data, float gain, float bias,
	float readout);

/*! \fn void smbrr_significant_new(struct smbrr *a,
	struct smbrr *s, float sigma);
 * \brief Set S(pixel) if A(pixel) > sigma
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

/*! \fn float smbrr_get_adu_at_posn(struct smbrr *data, int x, int y);
 * \brief Get data ADU value at (x,y)
 * \ingroup data
 */
float smbrr_get_adu_at_posn(struct smbrr *data, int x, int y);

/*! \fn float smbrr_get_adu_at_offset(struct smbrr *data, int offset);
 * \brief Get data ADU value at (x,y)
 * \ingroup data
 */
float smbrr_get_adu_at_offset(struct smbrr *data, int offset);

/*! \fn int smbrr_reconstruct(struct smbrr *O,
	enum smbrr_wavelet_mask mask, float threshold, int scales,
	enum smbrr_clip sigma_clip);
 * \brief Reconstruct an data without noise.
 * \ingroup data
 */
int smbrr_reconstruct(struct smbrr *O,
	enum smbrr_wavelet_mask mask, float threshold, int scales,
	enum smbrr_clip sigma_clip);

/*! \defgroup wavelet Wavelet
*
* Wavelet manipulation and management.
*/

/*! \fn struct smbrr_wavelet *smbrr_wavelet_new(struct smbrr *data,
	unsigned int num_scales);
 * \brief Create new wavelet from data.
 * \ingroup wavelet
 */
struct smbrr_wavelet *smbrr_wavelet_new(struct smbrr *data,
	unsigned int num_scales);

/*! \fn struct smbrr_wavelet *smbrr_wavelet_new_from_object(
	 struct smbrr_object *object);
 * \brief Create new wavelet from object.
 * \ingroup wavelet
 */
struct smbrr_wavelet *smbrr_wavelet_new_from_object(struct smbrr_object *object);

/*! \fn void smbrr_wavelet_free(struct smbrr_wavelet *w);
 * \brief Free wavelet.
 * \ingroup wavelet
 */
void smbrr_wavelet_free(struct smbrr_wavelet *w);

/*! \fn int smbrr_wavelet_convolution(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask);
 * \brief Convolve wavelet.
 * \ingroup wavelet
 */
int smbrr_wavelet_convolution(struct smbrr_wavelet *w, enum smbrr_conv conv,
	enum smbrr_wavelet_mask mask);

/*! \fn int smbrr_wavelet_significant_convolution(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask);
 * \brief Convolve wavelet using significant pixels only
 * \ingroup wavelet
 */
int smbrr_wavelet_significant_convolution(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask);

/*! \fn int smbrr_wavelet_deconvolution(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask);
 * \brief Deconvolve wavelet.
 * \ingroup wavelet
 */
int smbrr_wavelet_deconvolution(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask);

/*! \fn int smbrr_wavelet_significant_deconvolution(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask, enum smbrr_gain gain);
 * \brief Deconvolve wavelet using significant pixels.
 * \ingroup wavelet
 */
int smbrr_wavelet_significant_deconvolution(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask, enum smbrr_gain gain);

/*! \fn int smbrr_wavelet_deconvolution_object(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask,
	struct smbrr_object *object);
 * \brief Deconvolve wavelet object using significant pixels.
 * \ingroup wavelet
 */
int smbrr_wavelet_deconvolution_object(struct smbrr_wavelet *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask,
	struct smbrr_object *object);

/*! \fn struct smbrr *smbrr_wavelet_get_scale(
	struct smbrr_wavelet *w, unsigned int scale);
 * \brief Get scale data from wavelet at scale.
 * \ingroup wavelet
 */
struct smbrr *smbrr_wavelet_get_scale(struct smbrr_wavelet *w,
	unsigned int scale);

/*! \fn struct smbrr *smbrr_wavelet_get_wavelet(
	struct smbrr_wavelet *w, unsigned int scale);
 * \brief Get wavelet data from wavelet at scale.
 * \ingroup wavelet
 */
struct smbrr *smbrr_wavelet_get_wavelet(struct smbrr_wavelet *w,
	unsigned int scale);

/*! \fn struct smbrr *smbrr_wavelet_get_significant(
	struct smbrr_wavelet *w, unsigned int scale);
 * \brief Get significant data from wavelet at scale.
 * \ingroup wavelet
 */
struct smbrr *smbrr_wavelet_get_significant(struct smbrr_wavelet *w,
	unsigned int scale);

/*! \fn void smbrr_wavelet_add(struct smbrr_wavelet *a,
    struct smbrr_wavelet *b, struct smbrr_wavelet *c);
 * \brief Wavelet A = B + C.
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

/*! \fnvoid smbrr_wavelet_set_ccd(struct smbrr_wavelet *w, float gain, float bias,
	float readout)
* \brief Set CCD configuration
* \ingroup wavelet
*/
void smbrr_wavelet_set_ccd(struct smbrr_wavelet *w, float gain, float bias,
	float readout);

#endif
