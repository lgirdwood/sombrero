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
 * Sombrero is a fast wavelet image processing and object detection C library for
 * astronomical images. Sombrero is named after the "Mexican Hat" shape of the
 * wavelet masks used in image convolution and is released under the GNU LGPL
 * library.
 *
 * Some of the algorithms in this library are taken from "Astronomical Image
 * and Data Analysis" by Jean-Luc Starck and Fionn Murtoch.
 *
 * \section features Current Features
 *
 * - Significant pixel detection with k-sigma clipping. This can reduce the
 *   background noise in an image resulting in a better structure and object
 *   detection. Various K-sigma clipping levels are supported including custom
 *   user defined clipping coefficients.
 * - A'trous Wavelet convolution and deconvolution of images. The A'trous
 *   "with holes" image convolution is supported with either a linear or
 *   bicubic mask.
 * - Image transformations, i.e. add, subtract, multiply etc. Most general
 *   operators are supported. Can be used for stacking, flats, dark frames.
 * - Detection of structures and objects within images. Structures and objects
 *   are detected within images alongside properties like max pixel, brightness,
 *   average pixel brightness, position, size. Simple object de-blending
 *   also performed.
 *
 * \section examples Example Code
 *
 * Examples are provided showing the API in action for a variety of tasks and
 * are useful in their own right too for image processing and object detection.
 *
 * The examples include simple Bitmap image support for working with bitmap
 * images. This is intentional so that Sombrero has no dependencies on image
 * libraries (for FITS, etc) and users are free to choose their own image
 * libraries.
 *
 * \section planned Planned Features
 *
 * - SIMD optimisations for Wavelet and image operations. i.e AVX, NEON. A lot
 *   of the image processing is highly aligned with SIMD concepts so will
 *   greatly benefit from SIMD support.
 * - OpenMP/Cilk optimisations for improved parallelism. Some library APIs are
 *   good candidates to exploit the parallel capabilities of modern processors.
 *   k-sigma clipping, structure detection.
 * - Convert examples to tools. Gives the example code more functionality and
 *   link them to some image libraries for handling common image types
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
	SMBRR_ADU_8		= 0,	/*!< 8 bits per image pixel */
	SMBRR_ADU_16	= 1,	/*!< 16 bits per image pixel */
	SMBRR_ADU_32	= 2,	/*!< 32 bits per image pixel */
	SMBRR_ADU_FLOAT	= 3,	/*!< 32 bits float per image pixel */
};

/*!
** Image Type
* \enum smbrr_image_type
* \brief Supported Image types.
*
* Internal representation of image pixels.
*/
enum smbrr_image_type {
	SMBRR_IMAGE_UINT32 = 0, /*!< uint 32 - used by significant images */
	SMBRR_IMAGE_FLOAT  = 1, /*!< float - used by image pixels */
};

/*!
** Wavelet Convolution Type
* \enum smbrr_conv
* \brief Supported wavelet convolution types.
*
* Type of wavelet convolution or deconvolution applied to an image.
*/
enum smbrr_conv {
	SMBRR_CONV_ATROUS	= 0, /*!< The A-trous "with holes" convolution */
};

/*! \enum smbrr_wavelet_mask
* \brief Wavelet convolution mask
*
* Type of wavelet convolution or deconvolution mask applied to an image.
*/
enum smbrr_wavelet_mask {
	SMBRR_WAVELET_MASK_LINEAR	= 0, /*!< Linear wavelet convolution */
	SMBRR_WAVELET_MASK_BICUBIC	= 1, /*!< Bi-cubic wavelet convolution */
};

/*! \enum smbrr_clip
* \brief Wavelet image clipping strengths.
*
* Strength of K-sigma image background clipping.
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
* \brief Wavelet image gain strengths.
*
* Strength of K-sigma image background gain.
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
	SMBRR_OBJECT_STAR			= 0,	/*!< Stellar object Detected */
	SMBRR_OBJECT_EXTENDED		= 1,	/*!< Non stellar object detected */
};

/*! \struct smbrr_image
* \brief Image.
*
* Image structure containing image representation and runtime data.
*/
struct smbrr_image;

/*! \struct smbrr_wavelet_2d
 * \brief Wavelet
 *
 * Wavelet structure containing wavelet, scale and significant image
 * representations and runtime data.
 */
struct smbrr_wavelet_2d;

/*! \struct smbrr_coord
* \brief Coordinates.
*
* Structure and object image coordinates.
*/
struct smbrr_coord {
	unsigned int x;		/*!< Image X coordinate */
	unsigned int y;		/*!< Image Y coordinate */
};

/*! \struct smbrr_object
* \brief Detected Object.
*
* Object detected in image.
*/
struct smbrr_object {
	unsigned int id;	/*!< Object ID. Brightest = 0 */
	enum smbrr_object_type type;	/*!< Object classification */

	/* positional bounds */
	struct smbrr_coord pos;	/*!< Object image coordinates for max pixel */
	struct smbrr_coord minXy;	/*!< Object image min X coordinate */
	struct smbrr_coord minxY;	/*!< Object image min Y coordinate */
	struct smbrr_coord maxXy;	/*!< Object image max X coordinate */
	struct smbrr_coord maxxY;	/*!< Object image max Y coordinate */
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

/*! \defgroup image Images
*
* Image manipulation and management.
*/

/*
 * Image Construction and destruction.
 */

/*! \fn struct smbrr_image *smbrr_image_new(enum smbrr_image_type type,
	unsigned int width, unsigned int height, unsigned int stride,
	enum smbrr_adu adu, const void *img);
* \brief Create a new image.
* \ingroup image
*/
struct smbrr_image *smbrr_image_new(enum smbrr_image_type type,
	unsigned int width, unsigned int height, unsigned int stride,
	enum smbrr_adu adu, const void *img);

/*! \fn struct smbrr_image *smbrr_image_new_from_region(struct smbrr_image *image,
	unsigned int x_start, unsigned int y_start, unsigned int x_end,
	unsigned int y_end);
* \brief Create a new image from another image region.
* \ingroup image
*/
struct smbrr_image *smbrr_image_new_from_region(struct smbrr_image *image,
	unsigned int x_start, unsigned int y_start, unsigned int x_end,
	unsigned int y_end);

/*! \fn struct smbrr_image *smbrr_image_new_copy(struct smbrr_image *image)
* \param image Source image.
* \brief Create a new smbrr image from source image.
* \ingroup image
*/
struct smbrr_image *smbrr_image_new_copy(struct smbrr_image *src);

/*! \fn void smbrr_image_free(struct smbrr_image *image);
 * \brief Free an image.
 * \ingroup image
 */
void smbrr_image_free(struct smbrr_image *image);

/*
 * Image information.
 */

/*! \fn int smbrr_image_get(struct smbrr_image *image, enum smbrr_adu adu,
	void **img);
 * \brief Get raw data from an image.
 * \ingroup image
 */
int smbrr_image_get(struct smbrr_image *image, enum smbrr_adu adu,
	void **img);

/*! \fn int smbrr_image_pixels(struct smbrr_image *image);
 * \brief Get number of pixels in image.
 * \ingroup image
 */
int smbrr_image_pixels(struct smbrr_image *image);

/*! \fn int smbrr_image_bytes(struct smbrr_image *image);
 * \brief Get number of bytes for raw image.
 * \ingroup image
 */
int smbrr_image_bytes(struct smbrr_image *image);

/*! \fn int smbrr_image_stride(struct smbrr_image *image)
* \brief Return the number of pixels in image stride.
* \ingroup image
*/
int smbrr_image_stride(struct smbrr_image *image);

/*! \fn int smbrr_image_width(struct smbrr_image *image)
* \brief Return the number of pixels in image width.
* \ingroup image
*/
int smbrr_image_width(struct smbrr_image *image);

/*! \fn int smbrr_image_height(struct smbrr_image *image)
* \brief Return the number of pixels in image height.
* \ingroup image
*/
int smbrr_image_height(struct smbrr_image *image);

/*! \fn void smbrr_image_find_limits(struct smbrr_image *image, float *min,
 * float *max);
 * \brief Find image limits.
 * \ingroup image
 */
void smbrr_image_find_limits(struct smbrr_image *image, float *min, float *max);

/*! \fn float smbrr_image_get_mean(struct smbrr_image *image);
 * \brief Get mean pixel value of image.
 * \ingroup image
 */
float smbrr_image_get_mean(struct smbrr_image *image);

/*! \fn float smbrr_image_get_sigma(struct smbrr_image *image, float mean);
 * \brief Get pixel standard deviation of image.
 * \ingroup image
 */
float smbrr_image_get_sigma(struct smbrr_image *image, float mean);

/*! \fn float smbrr_image_get_mean_sig(struct smbrr_image *image,
	struct smbrr_image *simage);
 * \brief Get image mean for significant pixels.
 * \ingroup image
 */
float smbrr_image_get_mean_sig(struct smbrr_image *image,
	struct smbrr_image *simage);

/*! \fn float smbrr_image_get_sigma_sig(struct smbrr_image *image,
	struct smbrr_image *simage, float mean);
 * \brief Get image standard deviation for significant pixels.
 * \ingroup image
 */
float smbrr_image_get_sigma_sig(struct smbrr_image *image,
	struct smbrr_image *simage, float mean);

/*! \fn float smbrr_image_get_norm(struct smbrr_image *image);
 * \brief Get image norm.
 * \ingroup image
 */
float smbrr_image_get_norm(struct smbrr_image *image);

/*
 * Image Transformations
 */

/*! \fn float void smbrr_image_normalise(struct smbrr_image *image,
 * float min, float max);
 * \brief Normalise image pixels between new min and max values.
 * \ingroup image
 */
void smbrr_image_normalise(struct smbrr_image *image, float min, float max);

/*! \fn void smbrr_image_add(struct smbrr_image *a, struct smbrr_image *b,
	struct smbrr_image *c);
 * \brief Image A = B + C
 * \ingroup image
 */
void smbrr_image_add(struct smbrr_image *a, struct smbrr_image *b,
	struct smbrr_image *c);

/*! \fn void smbrr_image_add_value_sig(struct smbrr_image *image,
	struct smbrr_image *image, float value)
* \brief If pixel significant then Image A += value
* \ingroup image
*/
void smbrr_image_add_value_sig(struct smbrr_image *image,
	struct smbrr_image *simage, float value);

/*! \fn void smbrr_image_add_sig(struct smbrr_image *a, struct smbrr_image *b,
	struct smbrr_image *c, struct smbrr_image *s);
 * \brief Image A = B + (significant) C
 * \ingroup image
 */
void smbrr_image_add_sig(struct smbrr_image *a, struct smbrr_image *b,
	struct smbrr_image *c, struct smbrr_image *s);

/*! \fn void smbrr_image_subtract(struct smbrr_image *a, struct smbrr_image *b,
	struct smbrr_image *c);
 * \brief Image A = B - C
 * \ingroup image
 */
void smbrr_image_subtract(struct smbrr_image *a, struct smbrr_image *b,
	struct smbrr_image *c);

/*! \fn void smbrr_image_subtract_sig(struct smbrr_image *a,
 *  struct smbrr_image *b, struct smbrr_image *c, struct smbrr_image *s);
 * \brief Image A = B - (significant) C
 * \ingroup image
 */
void smbrr_image_subtract_sig(struct smbrr_image *a, struct smbrr_image *b,
	struct smbrr_image *c, struct smbrr_image *s);

/*! \fn void smbrr_image_add_value(struct smbrr_image *a, float value);
 * \brief Image A = A + real value
 * \ingroup image
 */
void smbrr_image_add_value(struct smbrr_image *a, float value);

/*! \fn void smbrr_image_subtract_value(struct smbrr_image *a, float value);
 * \brief Image A = A - real value
 * \ingroup image
 */
void smbrr_image_subtract_value(struct smbrr_image *a, float value);

/*! \fn void smbrr_image_mult_value(struct smbrr_image *a, float value);
 * \brief Image A = A * real value
 * \ingroup image
 */
void smbrr_image_mult_value(struct smbrr_image *a, float value);

/*! \fn void smbrr_image_reset_value(struct smbrr_image *a, float value);
 * \brief Image A = real value
 * \ingroup image
 */
void smbrr_image_reset_value(struct smbrr_image *a, float value);

/*! \fn void smbrr_image_set_value_sig(struct smbrr_image *a,
	struct smbrr_image *s, float sig_value);
 * \brief Image (significant) A = value
 * \ingroup image
 */
void smbrr_image_set_value_sig(struct smbrr_image *a,
	struct smbrr_image *s, float value);

/*! \fn int smbrr_image_convert(struct smbrr_image *a,
 * enum smbrr_image_type type);
 * \brief Convert image A to new type.
 * \ingroup image
 */
int smbrr_image_convert(struct smbrr_image *a, enum smbrr_image_type type);

/*! \fn void smbrr_image_set_sig_value(struct smbrr_image *a, uint32_t value);
 * \brief Image (significant) A = value
 * \ingroup image
 */
void smbrr_image_set_sig_value(struct smbrr_image *a, uint32_t value);

/*! \fn void smbrr_image_clear_negative(struct smbrr_image *a);
 * \brief Set Image A negative pixels to zero.
 * \ingroup image
 */
void smbrr_image_clear_negative(struct smbrr_image *a);

/*! \fn int smbrr_image_copy(struct smbrr_image *dest, struct smbrr_image *src);
 * \brief Image dest = src.
 * \ingroup image
 */
int smbrr_image_copy(struct smbrr_image *dest, struct smbrr_image *src);

/*! \fn void smbrr_image_fma(struct smbrr_image *dest, struct smbrr_image *a,
	struct smbrr_image *b, float c);
 * \brief Image A = A + B * value C
 * \ingroup image
 */
void smbrr_image_fma(struct smbrr_image *dest, struct smbrr_image *a,
	struct smbrr_image *b, float c);

/*! \fn void smbrr_image_fms(struct smbrr_image *dest, struct smbrr_image *a,
	struct smbrr_image *b, float c);
 * \brief Image A = A - B * value C
 * \ingroup image
 */
void smbrr_image_fms(struct smbrr_image *dest, struct smbrr_image *a,
	struct smbrr_image *b, float c);

/*! \fn void smbrr_image_anscombe(struct smbrr_image *image, float gain,
 * float bias, float readout);
 * \brief Perform Anscombe noise reduction on Image.
 * \ingroup image
 */
void smbrr_image_anscombe(struct smbrr_image *image, float gain, float bias,
	float readout);

/*! \fn void smbrr_image_new_significance(struct smbrr_image *a,
	struct smbrr_image *s, float sigma);
 * \brief Set S(pixel) if A(pixel) > sigma
 * \ingroup image
 */
void smbrr_image_new_significance(struct smbrr_image *a,
	struct smbrr_image *s, float sigma);

/*! \fn int smbrr_image_psf(struct smbrr_image *src, struct smbrr_image *dest,
	enum smbrr_wavelet_mask mask);
 * \brief Perform PSF on source image
 * \ingroup image
 */
int smbrr_image_psf(struct smbrr_image *src, struct smbrr_image *dest,
	enum smbrr_wavelet_mask mask);

/*! \fn float smbrr_image_get_adu_at(struct smbrr_image *image, int x, int y);
 * \brief Get image ADU value at (x,y)
 * \ingroup image
 */
float smbrr_image_get_adu_at(struct smbrr_image *image, int x, int y);

/*! \fn int smbrr_image_reconstruct(struct smbrr_image *O,
	enum smbrr_wavelet_mask mask, float threshold, int scales,
	enum smbrr_clip sigma_clip);
 * \brief Reconstruct an image without noise.
 * \ingroup image
 */
int smbrr_image_reconstruct(struct smbrr_image *O,
	enum smbrr_wavelet_mask mask, float threshold, int scales,
	enum smbrr_clip sigma_clip);

/*! \defgroup wavelet Wavelet
*
* Wavelet manipulation and management.
*/


/*! \fn struct smbrr_wavelet_2d *smbrr_wavelet_new(struct smbrr_image *image,
	unsigned int num_scales);
 * \brief Create new wavelet from image.
 * \ingroup wavelet
 */
struct smbrr_wavelet_2d *smbrr_wavelet_new(struct smbrr_image *image,
	unsigned int num_scales);

/*! \fn struct smbrr_wavelet_2d *smbrr_wavelet_new_from_object(
	 struct smbrr_object *object);
 * \brief Create new wavelet from object.
 * \ingroup wavelet
 */
struct smbrr_wavelet_2d *smbrr_wavelet_new_from_object(struct smbrr_object *object);

/*! \fn void smbrr_wavelet_free(struct smbrr_wavelet_2d *w);
 * \brief Free wavelet.
 * \ingroup wavelet
 */
void smbrr_wavelet_free(struct smbrr_wavelet_2d *w);

/*! \fn int smbrr_wavelet_convolution(struct smbrr_wavelet_2d *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask);
 * \brief Convolve wavelet.
 * \ingroup wavelet
 */
int smbrr_wavelet_convolution(struct smbrr_wavelet_2d *w, enum smbrr_conv conv,
	enum smbrr_wavelet_mask mask);

/*! \fn int smbrr_wavelet_convolution_sig(struct smbrr_wavelet_2d *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask);
 * \brief Convolve wavelet using significant pixels only
 * \ingroup wavelet
 */
int smbrr_wavelet_convolution_sig(struct smbrr_wavelet_2d *w, enum smbrr_conv conv,
	enum smbrr_wavelet_mask mask);

/*! \fn int smbrr_wavelet_deconvolution(struct smbrr_wavelet_2d *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask);
 * \brief Deconvolve wavelet.
 * \ingroup wavelet
 */
int smbrr_wavelet_deconvolution(struct smbrr_wavelet_2d *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask);

/*! \fn int smbrr_wavelet_deconvolution_sig(struct smbrr_wavelet_2d *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask, enum smbrr_gain gain);
 * \brief Deconvolve wavelet using significant pixels.
 * \ingroup wavelet
 */
int smbrr_wavelet_deconvolution_sig(struct smbrr_wavelet_2d *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask, enum smbrr_gain gain);

/*! \fn int smbrr_wavelet_deconvolution_object(struct smbrr_wavelet_2d *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask,
	struct smbrr_object *object);
 * \brief Deconvolve wavelet object using significant pixels.
 * \ingroup wavelet
 */
int smbrr_wavelet_deconvolution_object(struct smbrr_wavelet_2d *w,
	enum smbrr_conv conv, enum smbrr_wavelet_mask mask,
	struct smbrr_object *object);

/*! \fn struct smbrr_image *smbrr_wavelet_image_get_scale(
	struct smbrr_wavelet_2d *w, unsigned int scale);
 * \brief Get scale image from wavelet at scale.
 * \ingroup wavelet
 */
struct smbrr_image *smbrr_wavelet_image_get_scale(struct smbrr_wavelet_2d *w,
	unsigned int scale);

/*! \fn struct smbrr_image *smbrr_wavelet_image_get_wavelet(
	struct smbrr_wavelet_2d *w, unsigned int scale);
 * \brief Get wavelet image from wavelet at scale.
 * \ingroup wavelet
 */
struct smbrr_image *smbrr_wavelet_image_get_wavelet(struct smbrr_wavelet_2d *w,
	unsigned int scale);

/*! \fn struct smbrr_image *smbrr_wavelet_image_get_significant(
	struct smbrr_wavelet_2d *w, unsigned int scale);
 * \brief Get significant image from wavelet at scale.
 * \ingroup wavelet
 */
struct smbrr_image *smbrr_wavelet_image_get_significant(struct smbrr_wavelet_2d *w,
	unsigned int scale);

/*! \fn void smbrr_wavelet_add(struct smbrr_wavelet_2d *a,
    struct smbrr_wavelet_2d *b, struct smbrr_wavelet_2d *c);
 * \brief Wavelet A = B + C.
 * \ingroup wavelet
 */
void smbrr_wavelet_add(struct smbrr_wavelet_2d *a, struct smbrr_wavelet_2d *b,
	struct smbrr_wavelet_2d *c);

/*! \fn void smbrr_wavelet_subtract(struct smbrr_wavelet_2d *a,
	struct smbrr_wavelet_2d *b, struct smbrr_wavelet_2d *c);
 * \brief Wavelet A = B - C.
 * \ingroup wavelet
 */
void smbrr_wavelet_subtract(struct smbrr_wavelet_2d *a, struct smbrr_wavelet_2d *b,
	struct smbrr_wavelet_2d *c);

/*! \fn int smbrr_wavelet_new_significance(struct smbrr_wavelet_2d *w,
	enum smbrr_clip sigma_clip);
 * \brief Create new significance scales for wavlet.
 * \ingroup wavelet
 */
int smbrr_wavelet_new_significance(struct smbrr_wavelet_2d *w,
	enum smbrr_clip sigma_clip);

/*! \fn int smbrr_wavelet_ksigma_clip(struct smbrr_wavelet_2d *w,
    enum smbrr_clip clip, float sig_delta);
 * \brief K sigma clip each wavelet scale.
 * \ingroup wavelet
 */
int smbrr_wavelet_ksigma_clip(struct smbrr_wavelet_2d *w, enum smbrr_clip clip,
	float sig_delta);

/*! \fn int smbrr_wavelet_ksigma_clip_custom(struct smbrr_wavelet_2d *w,
	struct smbrr_clip_coeff *coeff, float sig_delta);
 * \brief K sigma clip each wavelet scale with custom coefficients.
 * \ingroup wavelet
 */
int smbrr_wavelet_ksigma_clip_custom(struct smbrr_wavelet_2d *w,
	struct smbrr_clip_coeff *coeff, float sig_delta);

/*! \fn int smbrr_wavelet_structure_find(struct smbrr_wavelet_2d *w,
	unsigned int scale);
 * \brief Find structures in wavelet scale.
 * \ingroup wavelet
 */
int smbrr_wavelet_structure_find(struct smbrr_wavelet_2d *w, unsigned int scale);

/*! \fn int smbrr_wavelet_structure_connect(struct smbrr_wavelet_2d *w,
		unsigned int start_scale, unsigned int end_scale);
 * \brief Connect structures between wavelet scales.
 * \ingroup wavelet
 */
int smbrr_wavelet_structure_connect(struct smbrr_wavelet_2d *w,
		unsigned int start_scale, unsigned int end_scale);

/*! \fn struct smbrr_object *smbrr_wavelet_object_get(struct smbrr_wavelet_2d *w,
	unsigned int object_id);
* \brief Get wavelet object.
* \ingroup wavelet
*/
struct smbrr_object *smbrr_wavelet_object_get(struct smbrr_wavelet_2d *w,
	unsigned int object_id);

/*! \fn void smbrr_wavelet_object_free_all(struct smbrr_wavelet_2d *w)
* \brief Free all wavelet structures and objects.
* \ingroup wavelet
*/
void smbrr_wavelet_object_free_all(struct smbrr_wavelet_2d *w);

/*! \fn int smbrr_wavelet_object_get_image(struct smbrr_wavelet_2d *w,
		struct smbrr_object *object, struct smbrr_image **image);
* \brief Get wavelet object image.
* \ingroup wavelet
*/
int smbrr_wavelet_object_get_image(struct smbrr_wavelet_2d *w,
		struct smbrr_object *object, struct smbrr_image **image);

/*! \fn struct smbrr_object *smbrr_wavelet_get_object_at(struct smbrr_wavelet_2d *w,
 * 	int x, int y)
* \brief Get object at position (x,y).
* \ingroup wavelet
*/
struct smbrr_object *smbrr_wavelet_get_object_at(struct smbrr_wavelet_2d *w,
		int x, int y);

/*! \fn int smbrr_wavelet_set_dark_mean(struct smbrr_wavelet_2d *w,
 * 	float dark);
* \brief Set average background value.
* \ingroup wavelet
*/
int smbrr_wavelet_set_dark_mean(struct smbrr_wavelet_2d *w,
		float dark);

/*! \fnvoid smbrr_wavelet_set_ccd(struct smbrr_wavelet_2d *w, float gain, float bias,
	float readout)
* \brief Set CCD configuration
* \ingroup wavelet
*/
void smbrr_wavelet_set_ccd(struct smbrr_wavelet_2d *w, float gain, float bias,
	float readout);

#endif
