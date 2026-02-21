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
 *  Copyright (C) 2015 Liam Girdwood
 */

#ifndef _OPS_H
#define _OPS_H

/** \cond */
struct smbrr;
struct smbrr_wavelet;
/** \endcond */

/**
 * \struct data_ops
 * \brief Function pointers for SIMD optimized signal/image processing atomic
 * operations.
 */
struct data_ops {

  float (*get_mean)(struct smbrr *data); /**< Get mean. */
  float (*get_sigma)(struct smbrr *data,
                     float mean); /**< Get standard deviation. */
  float (*get_mean_sig)(
      struct smbrr *data,
      struct smbrr *sdata); /**< Get mean of significant pixels. */
  int (*sign)(struct smbrr *s, struct smbrr *n); /**< Get sign. */
  float (*get_sigma_sig)(struct smbrr *data, struct smbrr *sdata,
                         float mean); /**< Get sigma of significant pixels. */
  float (*get_norm)(struct smbrr *data); /**< Get L2 norm. */
  void (*normalise)(struct smbrr *data, float min,
                    float max); /**< Normalize data range. */
  void (*add)(struct smbrr *a, struct smbrr *b,
              struct smbrr *c); /**< Vector addition. */
  void (*add_value_sig)(struct smbrr *data, struct smbrr *sdata,
                        float value); /**< Add value to significant pixels. */
  void (*add_sig)(
      struct smbrr *a, struct smbrr *b, struct smbrr *c,
      struct smbrr *s); /**< Vector addition with significance mask. */
  void (*subtract)(struct smbrr *a, struct smbrr *b,
                   struct smbrr *c); /**< Vector subtraction. */
  void (*subtract_sig)(
      struct smbrr *a, struct smbrr *b, struct smbrr *c,
      struct smbrr *s); /**< Vector subtraction with significance mask. */
  void (*add_value)(struct smbrr *a, float value); /**< Add scalar value. */
  void (*subtract_value)(struct smbrr *a,
                         float value); /**< Subtract scalar value. */
  void (*mult_value)(struct smbrr *a,
                     float value); /**< Multiply scalar value. */
  void (*reset_value)(struct smbrr *a,
                      float value); /**< Reset all to scalar value. */
  void (*set_value_sig)(struct smbrr *a, struct smbrr *s,
                        float value); /**< Set significant pixels to value. */
  int (*convert)(struct smbrr *a,
                 enum smbrr_data_type type); /**< Convert core data type. */
  void (*set_sig_value)(struct smbrr *a,
                        uint32_t value);   /**< Set significance map value. */
  void (*clear_negative)(struct smbrr *a); /**< Clear negative values. */
  void (*abs)(struct smbrr *a);            /**< Compute absolute values. */
  void (*copy_sig)(struct smbrr *dest, struct smbrr *src,
                   struct smbrr *sig); /**< Copy masked significant data. */
  void (*mult_add)(struct smbrr *dest, struct smbrr *a, struct smbrr *b,
                   float c); /**< Multiply and add vector elements. */
  void (*mult_subtract)(struct smbrr *dest, struct smbrr *a, struct smbrr *b,
                        float c); /**< Multiply and subtract vector elements. */
  void (*anscombe)(
      struct smbrr *data, float gain, float bias,
      float readout); /**< Anscombe variance stabilization transform. */
  void (*new_significance)(struct smbrr *a, struct smbrr *s,
                           float sigma); /**< Generate new significance map. */
  void (*find_limits)(struct smbrr *data, float *min,
                      float *max); /**< Find min/max bounds. */
  int (*get)(struct smbrr *data, enum smbrr_source_type adu,
             void **buf); /**< Extract backing array pointer. */
  int (*psf)(struct smbrr *src, struct smbrr *dest,
             enum smbrr_wavelet_mask mask); /**< Generate PSF mapping. */

  /* conversion */
  void (*uchar_to_float)(
      struct smbrr *i, const unsigned char *c); /**< Unsigned char to float. */
  void (*ushort_to_float)(
      struct smbrr *i,
      const unsigned short *c); /**< Unsigned short to float. */
  void (*uint_to_float)(struct smbrr *i,
                        const unsigned int *c); /**< Unsigned int to float. */
  void (*float_to_uchar)(struct smbrr *i,
                         unsigned char *c); /**< Float to unsigned char. */
  void (*uint_to_uint)(
      struct smbrr *i,
      const unsigned int *c); /**< Unsigned int to unsigned int. */
  void (*ushort_to_uint)(
      struct smbrr *i,
      const unsigned short *c); /**< Unsigned short to unsigned int. */
  void (*uchar_to_uint)(
      struct smbrr *i,
      const unsigned char *c); /**< Unsigned char to unsigned int. */
  void (*float_to_uint)(struct smbrr *i,
                        const float *c); /**< Float to unsigned int. */
  void (*float_to_float)(struct smbrr *i,
                         const float *c); /**< Float to float copy. */
  void (*uint_to_uchar)(
      struct smbrr *i, unsigned char *c); /**< Unsigned int to unsigned char. */
};

/**
 * \struct convolution_ops
 * \brief Function pointers for wavelet convolution and deconvolution
 * operations.
 */
struct convolution_ops {
  void (*atrous_conv)(
      struct smbrr_wavelet *wavelet); /**< Execute Atrous convolution. */
  void (*atrous_conv_sig)(
      struct smbrr_wavelet *wavelet); /**< Execute Atrous convolution strictly
                                         on significant pixels. */
  void (*atrous_deconv_object)(
      struct smbrr_wavelet *w,
      struct smbrr_object
          *object); /**< Deconvolve object back to original scale. */
};

extern const struct data_ops data_ops_1d;
extern const struct data_ops data_ops_1d_sse42;
extern const struct data_ops data_ops_1d_avx;
extern const struct data_ops data_ops_1d_avx2;
extern const struct data_ops data_ops_1d_fma;
extern const struct data_ops data_ops_1d_avx512;

extern const struct data_ops data_ops_2d;
extern const struct data_ops data_ops_2d_sse42;
extern const struct data_ops data_ops_2d_avx;
extern const struct data_ops data_ops_2d_avx2;
extern const struct data_ops data_ops_2d_fma;
extern const struct data_ops data_ops_2d_avx512;

extern const struct convolution_ops conv_ops_1d;
extern const struct convolution_ops conv_ops_1d_sse42;
extern const struct convolution_ops conv_ops_1d_avx;
extern const struct convolution_ops conv_ops_1d_avx2;
extern const struct convolution_ops conv_ops_1d_fma;
extern const struct convolution_ops conv_ops_1d_avx512;

extern const struct convolution_ops conv_ops_2d;
extern const struct convolution_ops conv_ops_2d_sse42;
extern const struct convolution_ops conv_ops_2d_avx;
extern const struct convolution_ops conv_ops_2d_avx2;
extern const struct convolution_ops conv_ops_2d_fma;
extern const struct convolution_ops conv_ops_2d_avx512;

#endif
