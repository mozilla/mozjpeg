/*
 * Written by Josh Aas and Tim Terriberry
 * Copyright (c) 2013, Mozilla Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Mozilla Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* Input: JPEG YUV 4:2:0 */
/* Output: YUV 4:2:0 */

/* gcc -std=c99 jpegyuv.c -I/opt/local/include/ -L/opt/local/lib/ -ljpeg -o jpegyuv */

#include <errno.h>
#include <stdio.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

#include "jpeglib.h"

int main(int argc, char *argv[]) {
  const char *jpg_path;
  const char *yuv_path;
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  FILE *jpg_fd;
  int width;
  int height;
  int yuv_size;
  JSAMPLE *image_buffer;
  JSAMPROW yrow_pointer[16];
  JSAMPROW cbrow_pointer[8];
  JSAMPROW crrow_pointer[8];
  JSAMPROW *plane_pointer[3];
  int y;
  FILE *yuv_fd;

  if (argc != 3) {
    fprintf(stderr, "Required arguments:\n");
    fprintf(stderr, "1. Path to JPG input file\n");
    fprintf(stderr, "2. Path to YUV output file\n");
    return 1;
  }

  errno = 0;

  /* Will check these for validity when opening via 'fopen'. */
  jpg_path = argv[1];
  yuv_path = argv[2];

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);

  jpg_fd = fopen(jpg_path, "rb");
  if (!jpg_fd) {
    fprintf(stderr, "Invalid path to JPEG file!\n");
    return 1;
  }

  jpeg_stdio_src(&cinfo, jpg_fd);

  jpeg_read_header(&cinfo, TRUE);

  cinfo.raw_data_out = TRUE;
  cinfo.do_fancy_upsampling = FALSE;

  jpeg_start_decompress(&cinfo);

  width = cinfo.output_width;
  height = cinfo.output_height;
  /* Right now we only support dimensions that are multiples of 16. */
  if ((width % 16) != 0 || (height % 16) != 0) {
    fprintf(stderr, "Image dimensions must be multiples of 16!\n");
    return 1;
  }

  yuv_size = (width * height) + (((width >> 1) * (height >> 1)) * 2);
  image_buffer = malloc(yuv_size);

  plane_pointer[0] = yrow_pointer;
  plane_pointer[1] = cbrow_pointer;
  plane_pointer[2] = crrow_pointer;
  while (cinfo.output_scanline < cinfo.output_height) {
    for (y = 0; y < 16; y++) {
      yrow_pointer[y] = image_buffer + cinfo.image_width * (cinfo.output_scanline + y);
    }
    for (y = 0; y < 8; y++) {
      cbrow_pointer[y] = image_buffer + width * height +
                         ((width + 1) >> 1) * ((cinfo.output_scanline >> 1) + y);
      crrow_pointer[y] = image_buffer + width * height +
                         ((width + 1) >> 1) * ((height + 1) >> 1) +
                         ((width + 1) >> 1) * ((cinfo.output_scanline >> 1) + y);
    }
    jpeg_read_raw_data(&cinfo, plane_pointer, 16);
  }

  jpeg_finish_decompress(&cinfo);

  jpeg_destroy_decompress(&cinfo);

  fclose(jpg_fd);

  yuv_fd = fopen(yuv_path, "wb");
  if (!yuv_fd) {
    fprintf(stderr, "Invalid path to YUV file!");
    return 1;
  }
  if (fwrite(image_buffer, yuv_size, 1, yuv_fd)!=1) {
    fprintf(stderr, "Error writing yuv file\n");
  }
  fclose(yuv_fd);

  return 0;
}
