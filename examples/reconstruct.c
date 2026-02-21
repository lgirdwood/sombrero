/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (C) 2012 Liam Girdwood <lgirdwood@gmail.com>
 *
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bmp.h"
#include "sombrero.h"

#define SCALES 8

static void usage(char *argv[]) {
  fprintf(stdout, "Usage:%s -i infile.bmp -o outfile\n", argv[0]);
  fprintf(stdout, " -i Input bitmap file - only greyscale supported\n");
  fprintf(stdout, " -o Output file name\n");
  exit(0);
}

int main(int argc, char *argv[]) {
  struct smbrr *image;
  struct bitmap *bmp;
  const void *data;
  int ret, width, height, stride, opt;
  enum smbrr_source_type depth;
  float mean, sigma;
  char *ifile = NULL, *ofile = NULL;

  while ((opt = getopt(argc, argv, "i:o:")) != -1) {
    switch (opt) {
    case 'i':
      ifile = optarg;
      break;
    case 'o':
      ofile = optarg;
      break;
    default: /* '?' */
      usage(argv);
    }
  }

  if (ifile == NULL || ofile == NULL)
    usage(argv);

  ret = bmp_load(ifile, &bmp, &data);
  if (ret < 0)
    return ret;

  height = bmp_height(bmp);
  width = bmp_width(bmp);
  depth = bmp_depth(bmp);
  stride = bmp_stride(bmp);
  fprintf(stdout, "Image width %d height %d stride %d\n", width, height,
          stride);

  image = smbrr_new(SMBRR_DATA_2D_FLOAT, width, height, stride, depth, data);
  if (image == NULL) {
    fprintf(stderr, "cant create new image\n");
    return -EINVAL;
  }

  mean = smbrr_get_mean(image);
  sigma = smbrr_get_sigma(image, mean);
  fprintf(stdout, "Image before mean %f sigma %f\n", mean, sigma);

  smbrr_reconstruct(image, SMBRR_WAVELET_MASK_LINEAR, 1.0e-4, 8,
                    SMBRR_CLIP_VGENTLE);

  mean = smbrr_get_mean(image);
  sigma = smbrr_get_sigma(image, mean);
  fprintf(stdout, "Image after mean %f sigma %f\n", mean, sigma);

  bmp_image_save(image, bmp, ofile);
  free(bmp);
  smbrr_free(image);

  return 0;
}
