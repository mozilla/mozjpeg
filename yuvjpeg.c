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

/* Expects 4:2:0 YCbCr */

/* gcc -std=c99 yuvjpeg.c -I/opt/local/include/ -L/opt/local/lib/ -ljpeg -o yuvjpeg */

#include <errno.h>
#include <stdio.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

#include "jpeglib.h"

int main(int argc, char *argv[]) {
  long quality;
  const char *size;
  char *x;
  long width;
  long height;
  const char *yuv_path;
  const char *jpg_path;
  FILE *yuv_fd;
  size_t yuv_size;
  JSAMPLE *image_buffer;
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  FILE *jpg_fd;
  JSAMPROW yrow_pointer[16];
  JSAMPROW cbrow_pointer[8];
  JSAMPROW crrow_pointer[8];
  JSAMPROW *plane_pointer[3];
  int y;

  if (argc != 5) {
    fprintf(stderr, "Required arguments:\n");
    fprintf(stderr, "1. JPEG quality value, 0-100\n");
    fprintf(stderr, "2. Image size (e.g. '512x512'\n");
    fprintf(stderr, "3. Path to YUV input file\n");
    fprintf(stderr, "4. Path to JPG output file\n");
    return 1;
  }

  errno = 0;

  quality = strtol(argv[1], NULL, 10);
  if (errno != 0 || quality < 0 || quality > 100) {
    fprintf(stderr, "Invalid JPEG quality value!\n");
    return 1;
  }

  size = argv[2];
  x = strchr(size, 'x');
  if (!x && x != size && x != (x + strlen(x) - 1)) {
    fprintf(stderr, "Invalid image size input!\n");
    return 1;
  }
  width = strtol(size, NULL, 10);
  if (errno != 0) {
    fprintf(stderr, "Invalid image size input!\n");
    return 1;
  }
  height = strtol(x + 1, NULL, 10);
  if (errno != 0) {
    fprintf(stderr, "Invalid image size input!\n");
    return 1;
  }
  /* Right now we only support dimensions that are multiples of 16. */
  if ((width % 16) != 0 || (height % 16) != 0) {
    fprintf(stderr, "Image dimensions must be multiples of 16!\n");
    return 1;
  }

  /* Will check these for validity when opening via 'fopen'. */
  yuv_path = argv[3];
  jpg_path = argv[4];

  yuv_fd = fopen(yuv_path, "r");
  if (!yuv_fd) {
    fprintf(stderr, "Invalid path to YUV file!\n");
    return 1;
  }

  fseek(yuv_fd, 0, SEEK_END);
  yuv_size = ftell(yuv_fd);
  fseek(yuv_fd, 0, SEEK_SET);
  image_buffer = malloc(yuv_size);
  if (fread(image_buffer, yuv_size, 1, yuv_fd)!=1) {
    fprintf(stderr, "Error reading yuv file\n");
  };

  fclose(yuv_fd);

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);

  jpg_fd = fopen(jpg_path, "wb");
  if (!jpg_fd) {
    fprintf(stderr, "Invalid path to JPEG file!\n");
    return 1;
  }

  jpeg_stdio_dest(&cinfo, jpg_fd);

  cinfo.image_width = width;
  cinfo.image_height = height;
  cinfo.input_components = 3;

  cinfo.in_color_space = JCS_YCbCr;
  cinfo.use_moz_defaults = TRUE;
  jpeg_set_defaults(&cinfo);

  cinfo.raw_data_in = TRUE;
#if JPEG_LIB_VERSION >= 70
  cinfo.do_fancy_downsampling = FALSE;
#endif

  cinfo.comp_info[0].h_samp_factor = 2;
  cinfo.comp_info[0].v_samp_factor = 2;
  cinfo.comp_info[0].dc_tbl_no = 0;
  cinfo.comp_info[0].ac_tbl_no = 0;
  cinfo.comp_info[0].quant_tbl_no = 0;

  cinfo.comp_info[1].h_samp_factor = 1;
  cinfo.comp_info[1].v_samp_factor = 1;
  cinfo.comp_info[1].dc_tbl_no = 1;
  cinfo.comp_info[1].ac_tbl_no = 1;
  cinfo.comp_info[1].quant_tbl_no = 1;

  cinfo.comp_info[2].h_samp_factor = 1;
  cinfo.comp_info[2].v_samp_factor = 1;
  cinfo.comp_info[2].dc_tbl_no = 1;
  cinfo.comp_info[2].ac_tbl_no = 1;
  cinfo.comp_info[2].quant_tbl_no = 1;

  jpeg_set_quality(&cinfo, quality, TRUE);
  cinfo.optimize_coding = TRUE;

  jpeg_start_compress(&cinfo, TRUE);

  plane_pointer[0] = yrow_pointer;
  plane_pointer[1] = cbrow_pointer;
  plane_pointer[2] = crrow_pointer;
  while (cinfo.next_scanline < cinfo.image_height) {
    for (y = 0; y < 16; y++) {
      yrow_pointer[y] = image_buffer + cinfo.image_width * (cinfo.next_scanline + y);
    }
    for (y = 0; y < 8; y++) {
      cbrow_pointer[y] = image_buffer + width * height +
                         ((width + 1) >> 1) * ((cinfo.next_scanline >> 1) + y);
      crrow_pointer[y] = image_buffer + width * height +
                         ((width + 1) >> 1) * ((height + 1) >> 1) +
                         ((width + 1) >> 1) * ((cinfo.next_scanline >> 1) + y);
    }
    jpeg_write_raw_data(&cinfo, plane_pointer, 16);
  }

  jpeg_finish_compress(&cinfo);

  jpeg_destroy_compress(&cinfo);

  free(image_buffer);

  fclose(jpg_fd);

  return 0;
}
