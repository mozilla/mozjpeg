/*
 * wrppm.h
 *
 * This file was part of the Independent JPEG Group's software:
 * Copyright (C) 1994, Thomas G. Lane.
 * For conditions of distribution and use, see the accompanying README.ijg
 * file.
 */

#ifdef PPM_SUPPORTED

/* Private version of data destination object */

typedef struct {
  struct djpeg_dest_struct pub; /* public fields */

  /* Usually these two pointers point to the same place: */
  char *iobuffer;               /* fwrite's I/O buffer */
  JSAMPROW pixrow;              /* decompressor output buffer */
  size_t buffer_width;          /* width of I/O buffer */
  JDIMENSION samples_per_row;   /* JSAMPLEs per output row */
} ppm_dest_struct;

typedef ppm_dest_struct *ppm_dest_ptr;

#endif
