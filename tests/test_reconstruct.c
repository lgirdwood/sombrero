#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../examples/bmp.h"
#include "sombrero.h"

int main(int argc, char *argv[]) {
  struct smbrr *image;
  struct bitmap *bmp;
  const void *data;
  int ret, width, height, stride, opt;
  enum smbrr_source_type depth;
  char *ifile = NULL, *ofile = NULL;
  float mean, sigma;

  /* Expected values from examples/reconstruct on wiz-ha-x.bmp */
  float initial_mean = 0.570462;
  float initial_sigma = 1.654864;
  float final_mean = 1.196682;
  float final_sigma = 1.554609;

  while ((opt = getopt(argc, argv, "i:o:")) != -1) {
    switch (opt) {
    case 'i':
      ifile = optarg;
      break;
    case 'o':
      ofile = optarg;
      break;
    }
  }

  if (ifile == NULL || ofile == NULL) {
    fprintf(stderr, "Usage: %s -i input.bmp -o output.bmp\n", argv[0]);
    return -EINVAL;
  }

  ret = bmp_load(ifile, &bmp, &data);
  if (ret < 0)
    return ret;

  height = bmp_height(bmp);
  width = bmp_width(bmp);
  depth = bmp_depth(bmp);
  stride = bmp_stride(bmp);

  image = smbrr_new(SMBRR_DATA_2D_FLOAT, width, height, stride, depth, data);
  if (image == NULL) {
    return -EINVAL;
  }

  mean = smbrr_get_mean(image);
  sigma = smbrr_get_sigma(image, mean);
  fprintf(stdout, "Image before mean %f sigma %f\n", mean, sigma);

  if (fabs(mean - initial_mean) > 0.0001 ||
      fabs(sigma - initial_sigma) > 0.0001) {
    fprintf(stderr, "Initial image validation failed\n");
    return -EINVAL;
  }

  ret = smbrr_reconstruct(image, SMBRR_WAVELET_MASK_LINEAR, 1.0e-4, 8,
                          SMBRR_CLIP_VGENTLE);
  if (ret < 0) {
    return ret;
  }

  mean = smbrr_get_mean(image);
  sigma = smbrr_get_sigma(image, mean);
  fprintf(stdout, "Image after mean %f sigma %f\n", mean, sigma);

  if (fabs(mean - final_mean) > 0.0001 || fabs(sigma - final_sigma) > 0.0001) {
    fprintf(stderr, "Final image validation failed\n");
    return -EINVAL;
  }

  bmp_image_save(image, bmp, ofile);

  free(bmp);
  smbrr_free(image);

  return 0;
}
