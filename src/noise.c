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
 *  Copyright (C) 2026 Liam Girdwood
 */

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "local.h"
#include "mask.h"
#include "ops.h"
#include "sombrero.h"

/**
 * \param data Image
 * \return Image mean
 *
 * Calculate mean value of all data pixels.
 */
float smbrr_get_mean(struct smbrr *data) { return data->ops->get_mean(data); }

/**
 * \param data Image
 * \param sdata Significant data
 * \return mean value of significant pixels.
 *
 * Calculate the mean value of significant data pixels. i.e mean value
 * of all data(pixel) where sdata(pixel) > 0.
 */
float smbrr_significant_get_mean(struct smbrr *data, struct smbrr *sdata) {
  return data->ops->get_mean_sig(data, sdata);
}

/**
 * \param data Image
 * \param mean
 * \return Standard deviation of all data pixels.
 *
 * Calculate standard deviation (sigma) of data pixels.
 */
float smbrr_get_sigma(struct smbrr *data, float mean) {
  return data->ops->get_sigma(data, mean);
}

/**
 * \param data Image
 * \return Image norm
 *
 * Calculate norm of data pixels. i.e sqrt(sum of pixels)
 */
float smbrr_get_norm(struct smbrr *data) { return data->ops->get_norm(data); }

/**
 * \param data Image
 * \param sdata Significant data
 * \param mean_sig Mean value of significant pixels
 * \return Standard deviation of significant pixels.
 *
 * Calculate standard deviation (sigma) of significant data pixels. i.e
 * standard deviation of all data(pixel) where sdata(pixel) > 0.
 */
float smbrr_significant_get_sigma(struct smbrr *data, struct smbrr *sdata,
                                  float mean_sig) {
  return data->ops->get_sigma_sig(data, sdata, mean_sig);
}

/**
 * \param data Image
 * \param gain CCD amplifier gain in photo-electrons per ADU
 * \param bias Image bias in ADUs
 * \param readout Readout noise in RMS electrons.
 *
 * Convert an Poisson noise in an data to an data with a constant Guassian
 * deviation suitable for wavelet convolutions.
 */
void smbrr_anscombe(struct smbrr *data, float gain, float bias, float readout) {
  data->ops->anscombe(data, gain, bias, readout);
}

/**
 * \param data Image
 * \param sdata Significant data
 * \param sigma sigma
 *
 * Create new significance data from data for pixels above sigma.
 */
void smbrr_significant_new(struct smbrr *data, struct smbrr *sdata,
                           float sigma) {
  data->ops->new_significance(data, sdata, sigma);
}

/**
 * \def D1(x)
 * \brief Calculate inverse K-sigma coefficient.
 * \param x Input multiplier value.
 */
#define D1(x) (1.0 / x)

/**
 * \def M1(x)
 * \brief Calculate direct K-sigma coefficient.
 * \param x Input multiplier value.
 */
#define M1(x) (1.0 * x)

/* K sigma clip for each wavelet scale */
static const struct smbrr_clip_coeff k_sigma[6] = {
    /* very gentle */
    {
        .coeff = {M1(2.0), M1(1.0), D1(2.0), D1(4.0), D1(8.0), D1(16.0),
                  D1(32.0), D1(64.0), D1(128.0), D1(256.0), D1(512.0)},
    },
    /* gentle */
    {
        .coeff = {M1(3.0), M1(2.0), M1(1.0), D1(2.0), D1(4.0), D1(8.0),
                  D1(16.0), D1(32.0), D1(64.0), D1(128.0), D1(256.0)},
    },
    /* normal */
    {
        .coeff = {M1(4.0), M1(3.0), M1(2.0), M1(1.0), D1(2.0), D1(4.0), D1(8.0),
                  D1(16.0), D1(32.0), D1(64.0), D1(128.0)},
    },
    /* strong */
    {
        .coeff = {M1(5.0), M1(4.0), M1(3.0), M1(2.0), M1(1.0), D1(2.0), D1(4.0),
                  D1(8.0), D1(16.0), D1(32.0), D1(64.0)},
    },
    /* very strong */
    {
        .coeff = {M1(6.0), M1(5.0), M1(4.0), M1(3.0), M1(2.0), M1(1.0), D1(2.0),
                  D1(4.0), D1(8.0), D1(16.0), D1(32.0)},
    },
    /* very very strong */
    {
        .coeff = {M1(7.0), M1(6.0), M1(5.0), M1(4.0), M1(3.0), M1(2.0), M1(1.0),
                  D1(2.0), D1(4.0), D1(8.0), D1(16.0)},
    },
};

static void clip_scale(struct smbrr_wavelet *w, int scale,
                       const struct smbrr_clip_coeff *c, float sig_delta) {
  struct smbrr *data, *sdata;
  float mean, sigma;
  float mean_sig, sigma_sig, sigma_sig_old = 0.0;

  data = smbrr_wavelet_get_wavelet(w, scale);
  sdata = smbrr_wavelet_get_significant(w, scale);

  mean = smbrr_get_mean(data);
  sigma = smbrr_get_sigma(data, mean);

  sigma_sig = sigma;

  /* clip scale until sigma is less than delta */
  do {
    sigma_sig_old = sigma_sig;

    /* create new sig pixel data for scale */
    smbrr_significant_new(data, sdata, c->coeff[scale] * sigma_sig_old);

    /* calculate new mean and sigma for sig pixels */
    mean_sig = smbrr_significant_get_mean(data, sdata);
    smbrr_significant_get_sigma(data, sdata, mean_sig);

    /* keep going if sigma difference is above delta */
  } while (fabsf(sigma_sig - sigma_sig_old) > sig_delta);
}

/**
 * \param w wavelet
 * \param clip clipping strength
 * \param sig_delta clipping sigma delta
 *
 * Clip each wavelet scale based on strength until sigma for each scale is less
 * than sig_delta.
 */
int smbrr_wavelet_ksigma_clip(struct smbrr_wavelet *w, enum smbrr_clip clip,
                              float sig_delta) {
  const struct smbrr_clip_coeff *coeff;
  int i;

  if (clip < SMBRR_CLIP_VGENTLE || clip > SMBRR_CLIP_VVSTRONG)
    return -EINVAL;

  coeff = &k_sigma[clip];
#pragma omp parallel for firstprivate(w, clip, sig_delta, coeff)               \
    schedule(dynamic, 1)
  for (i = 0; i < w->num_scales - 1; i++)
    clip_scale(w, i, coeff, sig_delta);

  return 0;
}

/*
 * \param w wavelet
 * \param clip clipping strength
 * \param sig_delta clipping sigma delta
 *
 * Clip each wavelet scale based on strength until sigma for each scale is less
 * than sig_delta.
 */
int smbrr_wavelet_ksigma_clip_custom(struct smbrr_wavelet *w,
                                     struct smbrr_clip_coeff *coeff,
                                     float sig_delta) {
  int i;

#pragma omp parallel for firstprivate(w, sig_delta, coeff) schedule(dynamic, 1)
  for (i = 0; i < w->num_scales - 1; i++)
    clip_scale(w, i, coeff, sig_delta);

  return 0;
}

/**
 * \param w wavelet
 * \param sigma_clip clipping strength
 *
 * Create new significance datas at each scale for wavelet using clip strength
 * sigma_clip.
 */
int smbrr_wavelet_new_significant(struct smbrr_wavelet *w,
                                  enum smbrr_clip sigma_clip) {
  struct smbrr *W, *S;
  const struct smbrr_clip_coeff *c;
  float mean, sigma;
  int i;

  if (sigma_clip < SMBRR_CLIP_VGENTLE || sigma_clip > SMBRR_CLIP_VVSTRONG)
    return -EINVAL;

  c = &k_sigma[sigma_clip];

  for (i = 0; i < w->num_scales - 1; i++) {
    W = smbrr_wavelet_get_wavelet(w, i);
    S = smbrr_wavelet_get_significant(w, i);

    mean = smbrr_get_mean(W);
    sigma = smbrr_get_sigma(W, mean);

    smbrr_significant_new(W, S, c->coeff[i] * sigma);
  }

  return 0;
}

/**
 * \param w wavelet.
 * \param dark Mean dark ADU
 *
 * Set the mean dark ADU level.
 */
int smbrr_wavelet_set_dark_mean(struct smbrr_wavelet *w, float dark) {
  w->dark = dark;
  return 0;
}

/**
 * \param w wavelet.
 * \param gain CCD amplifier gain in photo-electrons per ADU
 * \param bias Image bias in ADUs
 * \param readout Readout noise in RMS electrons.
 *
 * Set the CCD device configuration for noise calculations.
 */
void smbrr_wavelet_set_ccd(struct smbrr_wavelet *w, float gain, float bias,
                           float readout) {
  w->gain = gain;
  w->bias = bias;
  w->readout = readout;
}
