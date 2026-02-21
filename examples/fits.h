#ifndef _FITS_H
#define _FITS_H

#include "sombrero.h"

int fits_load(const char *file, const void **data, int *width, int *height,
              enum smbrr_source_type *depth, int *stride);
int fits_image_save(struct smbrr *image, const char *file);

#endif
