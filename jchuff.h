/*
 * jchuff.h
 *
 * This file was part of the Independent JPEG Group's software:
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * It was modified by The libjpeg-turbo Project to include only code relevant
 * to libjpeg-turbo.
 * mozjpeg Modifications:
 * Copyright (C) 2014, Mozilla Corporation.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains declarations for Huffman entropy encoding routines
 * that are shared between the sequential encoder (jchuff.c) and the
 * progressive encoder (jcphuff.c).  No other modules need to see these.
 */

/* The legal range of a DCT coefficient is
 *  -1024 .. +1023  for 8-bit data;
 * -16384 .. +16383 for 12-bit data.
 * Hence the magnitude should always fit in 10 or 14 bits respectively.
 */

#if BITS_IN_JSAMPLE == 8
#define MAX_COEF_BITS 10
#else
#define MAX_COEF_BITS 14
#endif

/* Derived data constructed for each Huffman table */

typedef struct {
  unsigned int ehufco[256];     /* code for each symbol */
  char ehufsi[256];             /* length of code for each symbol */
  /* If no code has been allocated for a symbol S, ehufsi[S] contains 0 */
} c_derived_tbl;

/* Expand a Huffman table definition into the derived format */
EXTERN(void) jpeg_make_c_derived_tbl
        (j_compress_ptr cinfo, boolean isDC, int tblno,
         c_derived_tbl ** pdtbl);

/* Generate an optimal table definition given the specified counts */
EXTERN(void) jpeg_gen_optimal_table
        (j_compress_ptr cinfo, JHUFF_TBL *htbl, long freq[]);

EXTERN(void) quantize_trellis
        (j_compress_ptr cinfo, jpeg_component_info *compptr, c_derived_tbl *dctbl, c_derived_tbl *actbl, JBLOCKROW coef_blocks, JBLOCKROW src, JDIMENSION num_blocks, JDIMENSION row_num,
                 JQUANT_TBL * qtbl, double *norm_src, double *norm_coef, JCOEF *last_dc_val,
         JBLOCKROW coef_blocks_above, JBLOCKROW src_above);
