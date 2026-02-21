#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Added for strstr

#include "../examples/bmp.h"
#include "../examples/fits.h" // Changed to point to ../examples/
#include "sombrero.h"

int main(int argc, char *argv[]) {
  struct smbrr *image, *simage, *oimage;
  struct smbrr_wavelet *w;
  struct bitmap *bmp = NULL; // Initialize bmp to NULL
  const void *data;
  int ret, width, height, stride, scales = 9;
  enum smbrr_source_type depth;
  char *ifile = NULL, *ofile = NULL;
  char outfile[64];
  int use_fits = 0; // Added as per instruction
  int structures;

  /* Expected values from examples/structures on wiz-ha-x.bmp */
  int expected_structures[] = {891, 703, 787, 933, 841, 267, 56, 12};

  if (argc != 3) {
    fprintf(stderr, "Usage: %s <input.bmp> <output_prefix>\n", argv[0]);
    return -EINVAL;
  }

  ifile = argv[1];
  ofile = argv[2];

  if (strstr(ifile, ".fit") != NULL) {
    use_fits = 1;
    ret = fits_load(ifile, &data, &width, &height, &depth, &stride);
    if (ret < 0)
      return ret;
  } else {
    ret = bmp_load(ifile, &bmp, &data);
    if (ret < 0)
      return ret;

    height = bmp_height(bmp);
    width = bmp_width(bmp);
    depth = bmp_depth(bmp);
    stride = bmp_stride(bmp);
  }

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

  // The following block is replaced as per instruction
  /*
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

    if (!use_fits && structures != expected_structures[i]) {
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
  */

  // New code block from instruction starts here
  struct smbrr *tsimage =
      smbrr_new(SMBRR_DATA_2D_FLOAT, width, height, stride, depth, NULL);
  if (tsimage == NULL) {
    return -EINVAL;
  }

  /* save image with significant structures on it */
  sprintf(outfile, "%s-ksigma", ofile);
  fprintf(stdout, "saving %s\n", outfile);
  if (use_fits)
    fits_image_save(image, outfile); // Changed simage to image as per context
  else
    bmp_image_save(image, bmp,
                   outfile); // Changed simage to image as per context

  for (int i = 0; i < scales - 1; i++) { // Changed loop variable to 'int i'
    struct smbrr *wimage = smbrr_wavelet_get_wavelet(w, i);

    /* get significant coefficients, if we clip the unecessary BG
     * we can better reveal structures */
    struct smbrr *wksigma = smbrr_wavelet_get_significant(w, i);

    /* we now use another copy of the source image to add only the
            significant wavelet coefficients back to it */
    smbrr_significant_add_value(oimage, wksigma, 1.0);

    /* dump each wavelet layer for visualisation */
    smbrr_set_value(tsimage, 0.0);
    smbrr_add_value(tsimage, 127.0); /* this brings out the detail */
    smbrr_add(tsimage, tsimage, wimage);

    sprintf(outfile, "%s-sig-%d", ofile, i);
    float dmin = 0, dmax = 0;
    smbrr_find_limits(tsimage, &dmin, &dmax);
    fprintf(stdout, "limit for %s are %f to %f\n", outfile, dmin, dmax);
    fprintf(stdout, "saving %s\n", outfile);
    if (use_fits)
      fits_image_save(tsimage, outfile);
    else
      bmp_image_save(tsimage, bmp, outfile);
  }
  // New code block from instruction ends here

  if (bmp) { // Only free bmp if it was allocated
    free(bmp);
  }
  smbrr_wavelet_free(w);
  smbrr_free(oimage);
  smbrr_free(image);
  smbrr_free(tsimage); // Free tsimage

  return 0;
}
