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

/** \mainpage Sombrero
 *
 * \section intro_sec Introduction
 *
 * Sombrero is a fast wavelet data processing and object detection C library for
 * 1D and 2D data. Sombrero is named after the "Mexican Hat" shape of the
 * wavelet masks used in data convolution and is released under the GNU LGPL
 * library. Sombrero was initially developed for astronomical work but can also
 * be used with any other data that can benefit from wavelet processing.
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
 *   "with holes" data convolution is supported with either a linear or bicubic
 * mask.
 * - Data transformations, i.e. add, subtract, multiply etc. Most general
 *   operators are supported. Can be used for stacking, flats, dark frames.
 * - Detection of structures and objects within data elements. Structures and
 * objects are detected within data alongside properties like max element values
 *   brightness, average element values, position, size. Simple object
 * de-blending also performed.
 * - SIMD optimisations for element and wavelet operations using SSE, AVX, AVX2,
 *   AVX-512 and FMA on x86 CPUs.
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
 * - <b>Done</b> SIMD optimisations for Wavelet and data operations. i.e AVX,
 * NEON. A lot of the data processing is highly aligned with SIMD concepts so
 * will greatly benefit from SIMD support.
 * - <b>Done</b> OpenMP/Cilk optimisations for improved parallelism. Some
 * library APIs are good candidates to exploit the parallel capabilities of
 * modern processors. k-sigma clipping, structure detection.
 * - <b>Done</b> Convert examples to tools. Gives the example code more
 * functionality and link them to some data libraries for handling common data
 * types (e.g FITS, raw CCD output)
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

/** \file */

/**
 * \def SMBRR_MAX_SCALES
 * \brief Maximum number of scales for wavelet operations.
 */
#define SMBRR_MAX_SCALES 12

/**
** Source Data Type
* \enum smbrr_source_type
* \brief Supported source data types.
*
* Internal source data types that are supported for element and wavelet
* operations.
*/
enum smbrr_source_type {
  SMBRR_SOURCE_UINT8 = 0,  /**< 8 bits per data pixel */
  SMBRR_SOURCE_UINT16 = 1, /**< 16 bits per data pixel */
  SMBRR_SOURCE_UINT32 = 2, /**< 32 bits per data pixel */
  SMBRR_SOURCE_FLOAT = 3,  /**< 32 bits float per data pixel */
};

/**
** Functional Data Types
* \enum smbrr_data_type
* \brief Supported Internal Data types.
*
* Supported internal data types. 1D and 2D data in 32 bit float and 1D and 2D
* significant data in 32 bit unsigned int.
*/
enum smbrr_data_type {
  SMBRR_DATA_1D_UINT32 = 0, /**< 32 bit uint - used by significant 1D data */
  SMBRR_DATA_1D_FLOAT = 1,  /**< 32 bit float - used by 1D data */
  SMBRR_DATA_2D_UINT32 = 2, /**< 32 bit uint - used by significant 2D data */
  SMBRR_DATA_2D_FLOAT = 3,  /**< 32 bit float - used by 2D data */
};

/**
** Wavelet Convolution Type
* \enum smbrr_conv Wavelet Convolution Type
* \brief Supported wavelet convolution and deconvolution types.
*
* Type of wavelet convolution or deconvolution applied to 1D and 2D data
* elements.
*/
enum smbrr_conv {
  SMBRR_CONV_ATROUS = 0, /**< The A-trous "with holes" convolution */
  SMBRR_CONV_PSF = 1,    /**< The PSF Point Spread Function */
};

/** \enum smbrr_wavelet_mask
 * \brief Wavelet convolution and deconvolution mask
 *
 * Type of wavelet convolution or deconvolution mask applied to 1D and 2D data
 * elements.
 */
enum smbrr_wavelet_mask {
  SMBRR_WAVELET_MASK_LINEAR = 0,  /**< Linear wavelet convolution */
  SMBRR_WAVELET_MASK_BICUBIC = 1, /**< Bi-cubic wavelet convolution */
};

/** \enum smbrr_clip
 * \brief Wavelet data clipping strengths.
 *
 * Strength of K-sigma data background clipping.
 */
enum smbrr_clip {
  SMBRR_CLIP_VGENTLE = 0,  /**< Very gentle clipping */
  SMBRR_CLIP_GENTLE = 1,   /**< Gentle clipping */
  SMBRR_CLIP_NORMAL = 2,   /**< Normal clipping */
  SMBRR_CLIP_STRONG = 3,   /**< Strong clipping */
  SMBRR_CLIP_VSTRONG = 4,  /**< Very strong clipping */
  SMBRR_CLIP_VVSTRONG = 5, /**< Very very strong clipping */
};

/** \enum smbrr_gain
 * \brief Wavelet data gain strengths.
 *
 * Strength of K-sigma data background gain.
 */
enum smbrr_gain {
  SMBRR_GAIN_NONE = 0,   /**< No gain */
  SMBRR_GAIN_LOW = 1,    /**< Low resolution gain */
  SMBRR_GAIN_MID = 2,    /**< Mid resolution gain */
  SMBRR_GAIN_HIGH = 3,   /**< High resolution gain */
  SMBRR_GAIN_LOWMID = 4, /**< High/Mid resolution gain */
};

/** \enum smbrr_object_type
 * \brief Object classification
 *
 * Classifications of detected objects.
 */
enum smbrr_object_type {
  SMBRR_OBJECT_POINT = 0,    /**< Point like object Detected */
  SMBRR_OBJECT_EXTENDED = 1, /**< Exteded / Diffuse object detected */
};

/** \struct smbrr
 * \brief Sombrero data context.
 *
 * Context for internal 1D or 2D data elements. 1D elements can be
 * used to represent a signal whilst 2D elements are used for images.
 */
struct smbrr;

/** \struct smbrr_coord
 * \brief Coordinates.
 *
 * 1D and 2D positional coordinates than can be used to reference individual
 * data elements or detected structures/objects within data.
 */
struct smbrr_coord {
  unsigned int x; /**< 2D X coordinate / 1D position offset  */
  unsigned int y; /**< 2D Y coordinate */
};

/** \struct smbrr_object
 * \brief Object detected in 1D or 2D data elements.
 *
 * Represents detected "object" within 1D and 2D data. The object can be
 * classified and it's position within the data set is known.  The Position
 * Angle (PA), radius, anullus are also known for detected 2D objects.
 * Background noise and SNR is also calculated.
 */
struct smbrr_object {
  unsigned int id; /**< Object ID. Incrementing on brightest/largest = 0 */
  enum smbrr_object_type type; /**< Object classification */

  /* 2D object positional bounds */
  struct smbrr_coord pos;   /**< Object data coordinates for max pixel */
  struct smbrr_coord minXy; /**< Object data min X coordinate */
  struct smbrr_coord minxY; /**< Object data min Y coordinate */
  struct smbrr_coord maxXy; /**< Object data max X coordinate */
  struct smbrr_coord maxxY; /**< Object data max Y coordinate */
  float pa;                 /**< Position angle  */

  /* object aperture */
  float object_adu;         /**< Sum of all object pixels values */
  float object_radius;      /**< Object radius in pixels */
  unsigned int object_area; /**< Object area in pixels */
  float snr;                /**< Signal to Noise Ration */
  float error;              /**< Error in SNR */

  /* object anullus (background) */
  unsigned int background_area; /**< Count of background pixels in annulus */
  float background_adu;         /**< Total of background pixels in annulus */

  /* statistical data */
  float max_adu;      /**< Maximum object pixel value */
  float mean_adu;     /**< Mean value of pixels */
  float sigma_adu;    /**< Standard deviation of pixels */
  float mag_delta;    /**< Magnitude difference to brightest object */
  unsigned int scale; /**< Object wavelet scale */
};

/** \struct smbrr_clip_coeff
 * \brief Custom K-sigma cliping coefficients for each scale.
 *
 * Array of K-sigma clipping coefficients used to clip each wavelet scale.
 */
struct smbrr_clip_coeff {
  float coeff[SMBRR_MAX_SCALES - 1]; /**< clipping coefficient for scale */
};

/** \defgroup data Processing 1D and 2D data element arrays.
 *
 * 1D and 2D Data element manipulation and management.
 */

/**
 * \brief Create and allocate a new 1D or 2D data context, calculating stride
 * boundaries and optionally initializing it with source data.
 * \param type The data type format of the pixel data.
 * \param width The width of the data element in pixels.
 * \param height The height of the data element in pixels.
 * \param stride The stride size of the data elements.
 * \param adu The source type adu format.
 * \param data The pointer to the source array.
 * \return struct smbrr* A pointer to the newly allocated sombrero context.
 * \ingroup data
 */
struct smbrr *smbrr_new(enum smbrr_data_type type, unsigned int width,
                        unsigned int height, unsigned int stride,
                        enum smbrr_source_type adu, const void *data);

/**
* \brief Extract a rectangular sub-region from a 2D source context and allocate
* it into a new context, preserving the original data type.
* \ingroup data
*/
struct smbrr *smbrr_new_from_area(struct smbrr *s, unsigned int x_start,
                                  unsigned int y_start, unsigned int x_end,
                                  unsigned int y_end);

/**
* \brief Extract a linear segment from a 1D source context and allocate it into
* a new context.
* \ingroup data
*/
struct smbrr *smbrr_new_from_section(struct smbrr *s, unsigned int start,
                                     unsigned int end);

/**
 * \param src Source data context.
 * \brief Perform a deep copy of an entire 1D or 2D data context into a newly
 * allocated context of identical dimensions and type.
 * \return struct smbrr* A pointer to the copied sombrero context.
 * \ingroup data
 */
struct smbrr *smbrr_new_copy(struct smbrr *src);

/**
 * \brief Safely deallocate the internal ADU pixel buffers and the context
 * structure itself.
 * \ingroup data
 */
void smbrr_free(struct smbrr *smbrr);

/*
 * Element information.
 */

/**
 * \brief Retrieve a pointer to the raw internal pixel data buffer, optionally
 * casting it to a specific ADU format.
 * \param s The active data element context.
 * \param adu The ADU format the data should be retrieved in.
 * \param data Pointer output to receive the data array.
 * \return 0 on success.
 * \ingroup data
 */
int smbrr_get_data(struct smbrr *s, enum smbrr_source_type adu, void **data);

/**
 * \brief Retrieve the total number of initialized elements (width * height) in
 * the data context.
 * \ingroup data
 */
int smbrr_get_size(struct smbrr *s);

/**
 * \brief Calculate the total memory footprint in bytes required by the internal
 * elements array.
 * \ingroup data
 */
int smbrr_get_bytes(struct smbrr *s);

/**
 * \brief Retrieve the memory alignment stride for the 2D data context to ensure
 * proper row-by-row memory access.
 * \ingroup data
 */
int smbrr_get_stride(struct smbrr *s);

/**
 * \brief Retrieve the horizontal width (in pixels) of the 2D element matrix, or
 * the length of a 1D sequence.
 * \ingroup data
 */
int smbrr_get_width(struct smbrr *s);

/**
 * \brief Retrieve the vertical height (in pixels) of the 2D element matrix.
 * \ingroup data
 */
int smbrr_get_height(struct smbrr *s);

/**
 * float *max);
 * \brief Iterate over all elements in the context to find the absolute minimum
 * and maximum floating-point pixel values.
 * \ingroup data
 */
void smbrr_find_limits(struct smbrr *s, float *min, float *max);

/**
 * \brief Compute the mathematical average (mean) across all pixels in the data
 * context.
 * \ingroup data
 */
float smbrr_get_mean(struct smbrr *s);

/**
 * \brief Compute the standard deviation (sigma) representing the statistical
 * dispersion of pixel values relative to the mean.
 * \ingroup data
 */
float smbrr_get_sigma(struct smbrr *s, float mean);

/**
 * \brief Compute the average (mean) across pixels marked as significant (non-
 * zero in the significance map).
 * \ingroup data
 */
float smbrr_significant_get_mean(struct smbrr *s, struct smbrr *sdata);

/**
 * \brief Compute the standard deviation across pixels marked as significant
 * (non-zero in the significance map).
 * \ingroup data
 */
float smbrr_significant_get_sigma(struct smbrr *s, struct smbrr *sdata,
                                  float mean);

/**
 * \brief Compute the Euclidean norm (square root of the sum of squared
 * elements) of the data context.
 * \ingroup data
 */
float smbrr_get_norm(struct smbrr *s);

/*
 * Data Transformations
 */

/**
 * float min, float max);
 * \brief Scale and shift all pixel values linearly so that they fall within the
 * specified min and max bounds.
 * \ingroup data
 */
void smbrr_normalise(struct smbrr *s, float min, float max);

/**
 * \brief Perform element-wise matrix addition: A = B + C.
 * \ingroup data
 */
void smbrr_add(struct smbrr *a, struct smbrr *b, struct smbrr *c);

/**
* \brief Add a constant scalar value exclusively to pixels that are marked as
* significant in the significance map.
* \ingroup data
*/
void smbrr_significant_add_value(struct smbrr *s, struct smbrr *sdata,
                                 float value);

/**
 * \brief Perform conditionally masked addition: add elements of C to B only
 * where C is marked significant.
 * \ingroup data
 */
void smbrr_significant_add(struct smbrr *a, struct smbrr *b, struct smbrr *c,
                           struct smbrr *s);

/**
 * \brief Perform element-wise matrix subtraction: A = B - C.
 * \ingroup data
 */
void smbrr_subtract(struct smbrr *a, struct smbrr *b, struct smbrr *c);

/**
 * \brief Perform conditionally masked subtraction: subtract elements of C from
 * B only where C is marked significant.
 * \ingroup data
 */
void smbrr_significant_subtract(struct smbrr *a, struct smbrr *b,
                                struct smbrr *c, struct smbrr *s);

/**
 * \brief Add a constant scalar value to every pixel in the data context.
 * \ingroup data
 */
void smbrr_add_value(struct smbrr *a, float value);

/**
 * \brief Subtract a constant scalar value from every pixel in the data context.
 * \ingroup data
 */
void smbrr_subtract_value(struct smbrr *a, float value);

/**
 * \brief Multiply every pixel in the data context by a constant scalar value.
 * \ingroup data
 */
void smbrr_mult_value(struct smbrr *a, float value);

/**
 * \brief Overwrite every pixel in the data context with a constant scalar
 * value.
 * \ingroup data
 */
void smbrr_set_value(struct smbrr *a, float value);

/**
 * \brief Overwrite pixels in the data context with a constant scalar value only
 * where marked significant.
 * \ingroup data
 */
void smbrr_significant_set_value(struct smbrr *a, struct smbrr *s, float value);

/**
 * \brief Assign a fixed threshold value to all elements in a significance map.
 * \ingroup data
 */
void smbrr_significant_set_svalue(struct smbrr *a, uint32_t value);

/**
 * \brief Cast the data context between different underlying numerical
 * representations (e.g. from UINT32 to FLOAT).
 * \param a The active data element context.
 * \param type The target data type to cast to.
 * \return 0 on success.
 * \ingroup data
 */
int smbrr_convert(struct smbrr *a, enum smbrr_data_type type);

/**
 * \brief Perform a mathematical ReLU-like operation, clamping all negative
 * pixel values to zero.
 * \ingroup data
 */
void smbrr_zero_negative(struct smbrr *a);

/**
 * \brief Convert all pixel values in the context to their absolute magnitudes.
 * \ingroup data
 */
void smbrr_abs(struct smbrr *a);

/**
 * \brief Transfer the mathematical sign bit from elements in context N to
 * corresponding elements in context S.
 * \ingroup data
 */
int smbrr_signed(struct smbrr *s, struct smbrr *n);

/**
 * \brief Perform a block memory copy of pixel data between two identical
 * contexts.
 * \ingroup data
 */
int smbrr_copy(struct smbrr *dest, struct smbrr *src);

/**
 * \brief Copy pixels from the source context to the destination context
 * exclusively where the significance map is non-zero.
 * \ingroup data
 */
int smbrr_significant_copy(struct smbrr *dest, struct smbrr *src,
                           struct smbrr *sig);

/**
 * \brief Perform a fused multiply-add operation: add the product of context B
 * and scalar C to context A.
 * \ingroup data
 */
void smbrr_mult_add(struct smbrr *dest, struct smbrr *a, struct smbrr *b,
                    float c);

/**
 * \brief Perform a fused multiply-subtract operation: subtract the product of
 * context B and scalar C from context A.
 * \ingroup data
 */
void smbrr_mult_subtract(struct smbrr *dest, struct smbrr *a, struct smbrr *b,
                         float c);

/**
 * float bias, float readout);
 * \brief Apply an Anscombe variance-stabilizing transformation to convert
 * Poisson noise into approximately Gaussian noise.
 * \ingroup data
 */
void smbrr_anscombe(struct smbrr *s, float gain, float bias, float readout);

/**
 * \brief Generate a boolean significance map S by thresholding context A
 * against a given sigma value.
 * \ingroup data
 */
void smbrr_significant_new(struct smbrr *a, struct smbrr *s, float sigma);

/**
 * \brief Apply a Point Spread Function (PSF) convolution to the source data to
 * simulate or correct optical blurring.
 * \ingroup data
 */
int smbrr_psf(struct smbrr *src, struct smbrr *dest,
              enum smbrr_wavelet_mask mask);

/**
 * \brief Extract the pixel value computationally at the specific 2D (x, y)
 * Cartesian coordinate.
 * \ingroup data
 */
float smbrr_get_adu_at_posn(struct smbrr *s, int x, int y);

/**
 * \brief Extract the pixel value computationally at a specific 1D linear
 * offset.
 * \ingroup data
 */
float smbrr_get_adu_at_offset(struct smbrr *s, int offset);

/**
 * \brief Iteratively rebuild the data context using wavelet convolutions
 * targeting noise-free threshold limits.
 * \ingroup data
 */
int smbrr_reconstruct(struct smbrr *O, enum smbrr_wavelet_mask mask,
                      float threshold, int scales, enum smbrr_clip sigma_clip);

/** \defgroup wavelet Wavelet 1D and 2D Convolutions
 *
 * Wavelet manipulation and management.
 */

/**
 * \brief Allocate a hierarchical wavelet context comprising multiple resolution
 * scales decomposed from the original data.
 * \ingroup wavelet
 */
struct smbrr_wavelet *smbrr_wavelet_new(struct smbrr *s,
                                        unsigned int num_scales);

/**
 * \brief Construct a bounding sub-region wavelet scale hierarchy focused exclusively on a pre-detected astronomical object.
 * \param object The localized object parameters used as extraction bounds.
 * \return struct smbrr_wavelet* New Wavelet scale representation.
 * \ingroup wavelet
 */
struct smbrr_wavelet *
smbrr_wavelet_new_from_object(struct smbrr_object *object);

/**
 * \brief Safely deallocate the wavelet context hierarchy and all its
 * dynamically linked scale objects.
 * \ingroup wavelet
 */
void smbrr_wavelet_free(struct smbrr_wavelet *w);

/**
 * \brief Execute an A'trous or PSF smoothing convolution recursively across the
 * wavelet scales to separate detail frequencies.
 * \ingroup wavelet
 */
int smbrr_wavelet_convolution(struct smbrr_wavelet *w, enum smbrr_conv conv,
                              enum smbrr_wavelet_mask mask);

/**
 * \brief Execute smoothing convolution recursively across scales, masking
 * operations strictly within significant pixels to isolate signal from noise.
 * \ingroup wavelet
 */
int smbrr_wavelet_significant_convolution(struct smbrr_wavelet *w,
                                          enum smbrr_conv conv,
                                          enum smbrr_wavelet_mask mask);

/**
 * \brief Recombine detail coefficients across the wavelet scales to reconstruct
 * the original merged signal.
 * \ingroup wavelet
 */
int smbrr_wavelet_deconvolution(struct smbrr_wavelet *w, enum smbrr_conv conv,
                                enum smbrr_wavelet_mask mask);

/**
 * \brief Recombine detail coefficients across scales to reconstruct a merged
 * signal, filtering out components not marked as significant.
 * \ingroup wavelet
 */
int smbrr_wavelet_significant_deconvolution(struct smbrr_wavelet *w,
                                            enum smbrr_conv conv,
                                            enum smbrr_wavelet_mask mask,
                                            enum smbrr_gain gain);

/**
 * \brief Recombine coefficients spatially restricted to a segmented object's
 * coordinates, retaining only significant values.
 * \ingroup wavelet
 */
int smbrr_wavelet_deconvolution_object(struct smbrr_wavelet *w,
                                       enum smbrr_conv conv,
                                       enum smbrr_wavelet_mask mask,
                                       struct smbrr_object *object);

/**
 * \brief Retrieve the raw data context representing the specific smoothed
 * resolution level (scale) within the wavelet hierarchy.
 * \ingroup wavelet
 */
struct smbrr *smbrr_wavelet_get_scale(struct smbrr_wavelet *w,
                                      unsigned int scale);

/**
 * \brief Retrieve the data context representing the detail coefficients
 * (wavelet difference) at a specific hierarchical scale.
 * \ingroup wavelet
 */
struct smbrr *smbrr_wavelet_get_wavelet(struct smbrr_wavelet *w,
                                        unsigned int scale);

/**
 * \brief Retrieve the binary significance map distinguishing real signal from
 * background noise at a specific wavelet scale.
 * \ingroup wavelet
 */
struct smbrr *smbrr_wavelet_get_significant(struct smbrr_wavelet *w,
                                            unsigned int scale);

/**
 * \brief Perform element-wise addition recursively across all corresponding
 * scales of the wavelet structures.
 * \ingroup wavelet
 */
void smbrr_wavelet_add(struct smbrr_wavelet *a, struct smbrr_wavelet *b,
                       struct smbrr_wavelet *c);

/**
 * \brief Perform element-wise subtraction recursively across all corresponding
 * scales of the wavelet structures.
 * \ingroup wavelet
 */
void smbrr_wavelet_subtract(struct smbrr_wavelet *a, struct smbrr_wavelet *b,
                            struct smbrr_wavelet *c);

/**
 * \brief Subtract coefficients of wavelet C from B across all scales, strictly
 * masked by the significance mapping of C.
 * \ingroup wavelet
 */
void smbrr_wavelet_significant_subtract(struct smbrr_wavelet *a,
                                        struct smbrr_wavelet *b,
                                        struct smbrr_wavelet *c);

/**
 * \brief Add coefficients of wavelet C to B across all scales, strictly masked
 * by the significance mapping of C.
 * \ingroup wavelet
 */
void smbrr_wavelet_significant_add(struct smbrr_wavelet *a,
                                   struct smbrr_wavelet *b,
                                   struct smbrr_wavelet *c);

/**
 * \brief Apply K-sigma thresholding across all wavelet scales to dynamically
 * map statistically significant structures.
 * \ingroup wavelet
 */
int smbrr_wavelet_new_significant(struct smbrr_wavelet *w,
                                  enum smbrr_clip sigma_clip);

/**
 * \brief Iteratively threshold each wavelet scale using standard pre-calculated
 * K-sigma deviation coefficients until convergence.
 * \ingroup wavelet
 */
int smbrr_wavelet_ksigma_clip(struct smbrr_wavelet *w, enum smbrr_clip clip,
                              float sig_delta);

/**
 * \brief Iteratively threshold each wavelet scale using user-supplied deviation
 * coefficients until convergence.
 * \param w A pointer to the initialized wavelet context representation.
 * \param coeff A struct of coefficients representing thresholds for scaling.
 * \param sig_delta Signal delta parameters that is added.
 * \return 0 on success.
 * \ingroup wavelet
 */
int smbrr_wavelet_ksigma_clip_custom(struct smbrr_wavelet *w,
                                     struct smbrr_clip_coeff *coeff,
                                     float sig_delta);

/**
 * \brief Perform a connected-component analysis on the significance map at a
 * specific scale to logically group contiguous structural pixels.
 * \ingroup wavelet
 */
int smbrr_wavelet_structure_find(struct smbrr_wavelet *w, unsigned int scale);

/**
 * \brief Build a relational tree matching overlapping structures between
 * consecutive wavelet layers to classify multi-scale objects.
 * \ingroup wavelet
 */
int smbrr_wavelet_structure_connect(struct smbrr_wavelet *w,
                                    unsigned int start_scale,
                                    unsigned int end_scale);

/**
* \brief Access the classification and boundary parameter data for a globally
* identified structural object by its ID.
* \ingroup wavelet
*/
struct smbrr_object *smbrr_wavelet_object_get(struct smbrr_wavelet *w,
                                              unsigned int object_id);

/**
 * \brief Recursively deallocate memory assigned to structural clusters and
 * mathematical objects mapped within the wavelet context.
 * \ingroup wavelet
 */
void smbrr_wavelet_object_free_all(struct smbrr_wavelet *w);

/**
* \brief Extract the reconstructed pixel values isolated specifically within the
* confines of the detected object.
* \ingroup wavelet
*/
int smbrr_wavelet_object_get_data(struct smbrr_wavelet *w,
                                  struct smbrr_object *object,
                                  struct smbrr **data);

/**
* \brief Identify and return the highest-priority object whose bounding
* constraints overlap the given 2D coordinates.
* \ingroup wavelet
*/
struct smbrr_object *smbrr_wavelet_get_object_at_posn(struct smbrr_wavelet *w,
                                                      int x, int y);

/**
 * 	float dark);
 * \brief Manually inject a static background dark noise mean to calibrate
 * object SNR logic.
 * \ingroup wavelet
 */
int smbrr_wavelet_set_dark_mean(struct smbrr_wavelet *w, float dark);

/**
* \brief Define device-specific physical parameters (gain, bias, readout
* constraints) for precise Anscombe variance transformations.
* \ingroup wavelet
*/
void smbrr_wavelet_set_ccd(struct smbrr_wavelet *w, float gain, float bias,
                           float readout);

/**
 * \brief Seed the foundational layer (Scale 0) of the wavelet hierarchy with
 * raw input signal data.
 * \ingroup wavelet
 */
int smbrr_wavelet_set_elems(struct smbrr_wavelet *w, struct smbrr *s);

#endif
