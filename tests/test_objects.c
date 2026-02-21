#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // for strstr
#include <unistd.h>

#include "../examples/bmp.h"
#include "../examples/debug.h"
#include "../examples/fits.h"
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
  int opt, structures;
  int use_fits = 0;

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

    /* save each structure scale for visualisation */
    struct smbrr *tsimage;
    tsimage = smbrr_wavelet_get_significant(w, i);
    smbrr_set_value(oimage, 0.0);
    smbrr_significant_add_value(oimage, tsimage, 1.0);
    sprintf(outfile, "%s-struct-%d.bmp", ofile, i);
    float dmin = 0, dmax = 0;
    smbrr_find_limits(tsimage, &dmin, &dmax);
    fprintf(stdout, "limit for %s are %f to %f\n", outfile, dmin, dmax);
    fprintf(stdout, "saving %s\n", outfile);
    if (use_fits)
      fits_image_save(tsimage, outfile);
    else
      bmp_image_save(tsimage, bmp, outfile);
  }

  int objects = smbrr_wavelet_structure_connect(w, 0, scales - 2);
  if (objects < 0) {
    return -EINVAL;
  }

  fprintf(stdout, "Found %d objects\n", objects);
  if (!use_fits && objects != expected_objects) {
    fprintf(stderr, "Objects validation failed: Expected %d, got %d\n",
            expected_objects, objects);
    return -EINVAL;
  }

  for (int i = 0; i < objects; i++) {
    struct smbrr_object *object = smbrr_wavelet_object_get(w, i);
    if (i < 10) {
      struct smbrr *oimage_data;
      smbrr_wavelet_object_get_data(w, object, &oimage_data);
      if (oimage_data) {
        float omin = 0, omax = 0;
        smbrr_find_limits(oimage_data, &omin, &omax);
        sprintf(outfile, "%s-object-%d.bmp", ofile, i);
        fprintf(stdout, "limit for %s are %f to %f\n", outfile, omin, omax);
        fprintf(stdout, "saving %s\n", outfile);
        if (use_fits)
          fits_image_save(oimage_data, outfile);
        else
          bmp_image_save(oimage_data, bmp, outfile);
      }
    }
  }

  if (!use_fits)
    free(bmp);
  smbrr_wavelet_free(w);
  smbrr_free(oimage);
  smbrr_free(image);

  return 0;
}
