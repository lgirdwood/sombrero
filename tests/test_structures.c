#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "../examples/bmp.h"
#include "sombrero.h"

int main(int argc, char *argv[]) {
  struct smbrr *image, *simage, *oimage;
  struct smbrr_wavelet *w;
  struct bitmap *bmp;
  const void *data;
  int ret, width, height, stride, scales = 9;
  enum smbrr_source_type depth;
  char *ifile = NULL, *ofile = NULL;
  char outfile[64];
  int structures;

  /* Expected values from examples/structures on wiz-ha-x.bmp */
  int expected_structures[] = {891, 703, 787, 933, 841, 267, 56, 12};

  if (argc != 3) {
    fprintf(stderr, "Usage: %s <input.bmp> <output_prefix>\n", argv[0]);
    return -EINVAL;
  }

  ifile = argv[1];
  ofile = argv[2];

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

  oimage = smbrr_new(SMBRR_DATA_2D_FLOAT, width, height, stride, depth, NULL);
  if (oimage == NULL) {
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

  ret = smbrr_wavelet_significant_deconvolution(w, SMBRR_CONV_ATROUS,
                                                SMBRR_WAVELET_MASK_LINEAR, 0);
  if (ret < 0) {
    return ret;
  }

  sprintf(outfile, "%s-ksigma", ofile);
  bmp_image_save(image, bmp, outfile);

  for (int i = 0; i < scales - 1; i++) {
    simage = smbrr_wavelet_get_significant(w, i);
    smbrr_significant_add_value(oimage, simage, 16 + (1 << ((scales - 1) - i)));
  }

  sprintf(outfile, "%s-sigall", ofile);
  bmp_image_save(oimage, bmp, outfile);

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

    simage = smbrr_wavelet_get_significant(w, i);
    smbrr_set_value(oimage, 0.0);
    smbrr_significant_set_value(oimage, simage, 127);
    sprintf(outfile, "%s-sig-%d", ofile, i);
    bmp_image_save(oimage, bmp, outfile);
  }

  free(bmp);
  smbrr_wavelet_free(w);
  smbrr_free(oimage);
  smbrr_free(image);

  return 0;
}
