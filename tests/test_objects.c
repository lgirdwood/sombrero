#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../examples/bmp.h"
#include "sombrero.h"

int main(int argc, char *argv[]) {
  struct smbrr *image;
  struct smbrr_wavelet *w;
  struct bitmap *bmp;
  const void *data;
  int ret, width, height, stride, scales = 9;
  enum smbrr_source_type depth;
  char *ifile = NULL, *ofile = NULL;
  int opt, structures;

  /* Expected values from examples/objects on wiz-ha-x.bmp */
  int expected_structures[] = {891, 703, 787, 933, 841, 267, 56, 12};
  int expected_objects = 741;

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
    fprintf(stderr, "Usage: %s -i <input.bmp> -o <output_prefix>\n", argv[0]);
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

  w = smbrr_wavelet_new(image, scales);
  if (w == NULL) {
    return -EINVAL;
  }

  ret = smbrr_wavelet_convolution(w, SMBRR_CONV_ATROUS,
                                  SMBRR_WAVELET_MASK_LINEAR);
  if (ret < 0) {
    return ret;
  }

  fprintf(stdout, "Using K sigma strength %d delta %f\n", 1, 0.001000);
  smbrr_wavelet_ksigma_clip(w, 1, 0.001);

  for (int i = 0; i < scales - 1; i++) {
    structures = smbrr_wavelet_structure_find(w, i);
    if (structures < 0) {
      return -EINVAL;
    }

    fprintf(stdout, "Found %d structures at scale %d\n", structures, i);

    if (structures != expected_structures[i]) {
      fprintf(stderr,
              "Structures at scale %d validation failed: Expected %d, got %d\n",
              i, expected_structures[i], structures);
      return -EINVAL;
    }
  }

  int objects = smbrr_wavelet_structure_connect(w, 0, scales - 2);
  if (objects < 0) {
    return -EINVAL;
  }

  fprintf(stdout, "Found %d objects\n", objects);
  if (objects != expected_objects) {
    fprintf(stderr, "Objects validation failed: Expected %d, got %d\n",
            expected_objects, objects);
    return -EINVAL;
  }

  free(bmp);
  smbrr_wavelet_free(w);
  smbrr_free(image);

  return 0;
}
