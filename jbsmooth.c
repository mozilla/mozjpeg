/*
 * jbsmooth.c
 *
 * Copyright (C) 1991, 1992, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains cross-block smoothing routines.
 * These routines are invoked via the smooth_coefficients method.
 */

#include "jinclude.h"

#ifdef BLOCK_SMOOTHING_SUPPORTED


/*
 * Cross-block coefficient smoothing.
 */

METHODDEF void
smooth_coefficients (decompress_info_ptr cinfo,
		     jpeg_component_info *compptr,
		     JBLOCKROW above,
		     JBLOCKROW currow,
		     JBLOCKROW below,
		     JBLOCKROW output)
{
  QUANT_TBL_PTR Qptr = cinfo->quant_tbl_ptrs[compptr->quant_tbl_no];
  long blocks_in_row = compptr->downsampled_width / DCTSIZE;
  long col;

  /* First, copy the block row as-is.
   * This takes care of the first & last blocks in the row, the top/bottom
   * special cases, and the higher-order coefficients in each block.
   */
  jcopy_block_row(currow, output, blocks_in_row);

  /* Now apply the smoothing calculation, but not to any blocks on the
   * edges of the image.
   */

  if (above != NULL && below != NULL) {
    for (col = 1; col < blocks_in_row-1; col++) {

      /* See section K.8 of the JPEG standard.
       *
       * As I understand it, this produces approximations
       * for the low frequency AC components, based on the
       * DC values of the block and its eight neighboring blocks.
       * (Thus it can't be used for blocks on the image edges.)
       */

      /* The layout of these variables corresponds to text and figure in K.8 */
      
      JCOEF DC1, DC2, DC3;
      JCOEF DC4, DC5, DC6;
      JCOEF DC7, DC8, DC9;
      
      long       AC01, AC02;
      long AC10, AC11;
      long AC20;
      
      DC1 = above [col-1][0];
      DC2 = above [col  ][0];
      DC3 = above [col+1][0];
      DC4 = currow[col-1][0];
      DC5 = currow[col  ][0];
      DC6 = currow[col+1][0];
      DC7 = below [col-1][0];
      DC8 = below [col  ][0];
      DC9 = below [col+1][0];
      
#define DIVIDE_256(x)	x = ( (x) < 0 ? -((128-(x))/256) : ((x)+128)/256 )
      
      AC01 = (36 * (DC4 - DC6));
      DIVIDE_256(AC01);
      AC10 = (36 * (DC2 - DC8));
      DIVIDE_256(AC10);
      AC20 = (9 * (DC2 + DC8 - 2*DC5));
      DIVIDE_256(AC20);
      AC11 = (5 * ((DC1 - DC3) - (DC7 - DC9)));
      DIVIDE_256(AC11);
      AC02 = (9 * (DC4 + DC6 - 2*DC5));
      DIVIDE_256(AC02);
      
      /* I think that this checks to see if the quantisation
       * on the transmitting side would have produced this
       * answer. If so, then we use our (hopefully better)
       * estimate.
       */

#define ABS(x)	((x) < 0 ? -(x) : (x))

#define COND_ASSIGN(_ac,_n,_z)   if ((ABS(output[col][_n] - (_ac))<<1) <= Qptr[_z]) output[col][_n] = (JCOEF) (_ac)

      COND_ASSIGN(AC01,  1, 1);
      COND_ASSIGN(AC02,  2, 5);
      COND_ASSIGN(AC10,  8, 2);
      COND_ASSIGN(AC11,  9, 4);
      COND_ASSIGN(AC20, 16, 3);
    }
  }
}


/*
 * The method selection routine for cross-block smoothing.
 */

GLOBAL void
jselbsmooth (decompress_info_ptr cinfo)
{
  /* just one implementation for now */
  cinfo->methods->smooth_coefficients = smooth_coefficients;
}

#endif /* BLOCK_SMOOTHING_SUPPORTED */
