#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fits.h"
#include "sombrero.h"
#include <fitsio.h>

int fits_load(const char *file, const void **data, int *width, int *height,
              enum smbrr_source_type *depth, int *stride) {
  fitsfile *fptr;
  int status = 0;
  int naxis;
  long naxes[2] = {1, 1};
  long npixels;
  float *float_data;
  int anynul;

  if (fits_open_file(&fptr, file, READONLY, &status)) {
    fits_report_error(stderr, status);
    return -EINVAL;
  }

  if (fits_get_img_dim(fptr, &naxis, &status)) {
    fits_report_error(stderr, status);
    fits_close_file(fptr, &status);
    return -EINVAL;
  }

  if (fits_get_img_size(fptr, 2, naxes, &status)) {
    fits_report_error(stderr, status);
    fits_close_file(fptr, &status);
    return -EINVAL;
  }

  if (naxis == 1) {
    *width = naxes[0];
    *height = 1;
  } else {
    *width = naxes[0];
    *height = naxes[1];
  }

  npixels = (*width) * (*height);
  *data = calloc(*width * *height * 4, 1);
  float_data = (float *)*data;
  if (!float_data) {
    fits_close_file(fptr, &status);
    return -ENOMEM;
  }

  if (fits_read_img(fptr, TFLOAT, 1, npixels, NULL, float_data, &anynul,
                    &status)) {
    fits_report_error(stderr, status);
    free(float_data);
    fits_close_file(fptr, &status);
    return -EINVAL;
  }

  fits_close_file(fptr, &status);

  *data = float_data;
  *depth = SMBRR_SOURCE_FLOAT;
  *stride = *width;

  return 0;
}

int fits_image_save(struct smbrr *image, const char *file) {
  fitsfile *fptr;
  int status = 0;
  long naxes[2];
  long npixels;
  float *data;
  int width, height;

  remove(file); // remove pre-existing file if any

  if (fits_create_file(&fptr, file, &status)) {
    fits_report_error(stderr, status);
    return -EINVAL;
  }

  width = smbrr_get_width(image);
  height = smbrr_get_height(image);

  naxes[0] = width;
  naxes[1] = height;
  npixels = width * height;

  if (fits_create_img(fptr, FLOAT_IMG, 2, naxes, &status)) {
    fits_report_error(stderr, status);
    fits_close_file(fptr, &status);
    return -EINVAL;
  }

  data = calloc(npixels, sizeof(float));
  if (!data) {
    fits_close_file(fptr, &status);
    return -ENOMEM;
  }

  smbrr_get_data(image, SMBRR_SOURCE_FLOAT, (void **)&data);

  if (fits_write_img(fptr, TFLOAT, 1, npixels, data, &status)) {
    fits_report_error(stderr, status);
    free(data);
    fits_close_file(fptr, &status);
    return -EINVAL;
  }

  free(data);

  fits_close_file(fptr, &status);
  return 0;
}
