/*
 * jdmcu.c
 *
 * Copyright (C) 1991, 1992, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains MCU disassembly routines and quantization descaling.
 * These routines are invoked via the disassemble_MCU, reverse_DCT, and
 * disassemble_init/term methods.
 */

#include "jinclude.h"


/*
 * Quantization descaling and zigzag reordering
 */


/* ZAG[i] is the natural-order position of the i'th element of zigzag order. */

static const short ZAG[DCTSIZE2] = {
  0,  1,  8, 16,  9,  2,  3, 10,
 17, 24, 32, 25, 18, 11,  4,  5,
 12, 19, 26, 33, 40, 48, 41, 34,
 27, 20, 13,  6,  7, 14, 21, 28,
 35, 42, 49, 56, 57, 50, 43, 36,
 29, 22, 15, 23, 30, 37, 44, 51,
 58, 59, 52, 45, 38, 31, 39, 46,
 53, 60, 61, 54, 47, 55, 62, 63
};


LOCAL void
qdescale_zig (JBLOCK input, JBLOCKROW outputptr, QUANT_TBL_PTR quanttbl)
{
  const short * zagptr = ZAG;
  short i;

  for (i = DCTSIZE2-1; i >= 0; i--) {
    (*outputptr)[*zagptr++] = (*input++) * (*quanttbl++);
  }
}



/*
 * Fetch one MCU row from entropy_decode, build coefficient array.
 * This version is used for noninterleaved (single-component) scans.
 */

METHODDEF void
disassemble_noninterleaved_MCU (decompress_info_ptr cinfo,
				JBLOCKIMAGE image_data)
{
  JBLOCK MCU_data[1];
  long mcuindex;
  jpeg_component_info * compptr;
  QUANT_TBL_PTR quant_ptr;

  /* this is pretty easy since there is one component and one block per MCU */
  compptr = cinfo->cur_comp_info[0];
  quant_ptr = cinfo->quant_tbl_ptrs[compptr->quant_tbl_no];
  for (mcuindex = 0; mcuindex < cinfo->MCUs_per_row; mcuindex++) {
    /* Fetch the coefficient data */
    (*cinfo->methods->entropy_decode) (cinfo, MCU_data);
    /* Descale, reorder, and distribute it into the image array */
    qdescale_zig(MCU_data[0], image_data[0][0] + mcuindex, quant_ptr);
  }
}


/*
 * Fetch one MCU row from entropy_decode, build coefficient array.
 * This version is used for interleaved (multi-component) scans.
 */

METHODDEF void
disassemble_interleaved_MCU (decompress_info_ptr cinfo,
			     JBLOCKIMAGE image_data)
{
  JBLOCK MCU_data[MAX_BLOCKS_IN_MCU];
  long mcuindex;
  short blkn, ci, xpos, ypos;
  jpeg_component_info * compptr;
  QUANT_TBL_PTR quant_ptr;
  JBLOCKROW image_ptr;

  for (mcuindex = 0; mcuindex < cinfo->MCUs_per_row; mcuindex++) {
    /* Fetch the coefficient data */
    (*cinfo->methods->entropy_decode) (cinfo, MCU_data);
    /* Descale, reorder, and distribute it into the image array */
    blkn = 0;
    for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
      compptr = cinfo->cur_comp_info[ci];
      quant_ptr = cinfo->quant_tbl_ptrs[compptr->quant_tbl_no];
      for (ypos = 0; ypos < compptr->MCU_height; ypos++) {
	image_ptr = image_data[ci][ypos] + (mcuindex * compptr->MCU_width);
	for (xpos = 0; xpos < compptr->MCU_width; xpos++) {
	  qdescale_zig(MCU_data[blkn], image_ptr, quant_ptr);
	  image_ptr++;
	  blkn++;
	}
      }
    }
  }
}


/*
 * Perform inverse DCT on each block in an MCU row's worth of data;
 * output the results into a sample array starting at row start_row.
 * NB: start_row can only be nonzero when dealing with a single-component
 * scan; otherwise we'd have to pass different offsets for different
 * components, since the heights of interleaved MCU rows can vary.
 * But the pipeline controller logic is such that this is not necessary.
 */

METHODDEF void
reverse_DCT (decompress_info_ptr cinfo,
	     JBLOCKIMAGE coeff_data, JSAMPIMAGE output_data, int start_row)
{
  DCTBLOCK block;
  JBLOCKROW browptr;
  JSAMPARRAY srowptr;
  long blocksperrow, bi;
  short numrows, ri;
  short ci;

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    /* calculate size of an MCU row in this component */
    blocksperrow = cinfo->cur_comp_info[ci]->subsampled_width / DCTSIZE;
    numrows = cinfo->cur_comp_info[ci]->MCU_height;
    /* iterate through all blocks in MCU row */
    for (ri = 0; ri < numrows; ri++) {
      browptr = coeff_data[ci][ri];
      srowptr = output_data[ci] + (ri * DCTSIZE + start_row);
      for (bi = 0; bi < blocksperrow; bi++) {
	/* copy the data into a local DCTBLOCK.  This allows for change of
	 * representation (if DCTELEM != JCOEF).  On 80x86 machines it also
	 * brings the data back from FAR storage to NEAR storage.
	 */
	{ register JCOEFPTR elemptr = browptr[bi];
	  register DCTELEM *localblkptr = block;
	  register short elem = DCTSIZE2;

	  while (--elem >= 0)
	    *localblkptr++ = (DCTELEM) *elemptr++;
	}

	j_rev_dct(block);	/* perform inverse DCT */

	/* output the data into the sample array.
	 * Note change from signed to unsigned representation:
	 * DCT calculation works with values +-CENTERJSAMPLE,
	 * but sample arrays always hold 0..MAXJSAMPLE.
	 * Have to do explicit range-limiting because of quantization errors
	 * and so forth in the DCT/IDCT phase.
	 */
	{ register JSAMPROW elemptr;
	  register DCTELEM *localblkptr = block;
	  register short elemr, elemc;
	  register DCTELEM temp;

	  for (elemr = 0; elemr < DCTSIZE; elemr++) {
	    elemptr = srowptr[elemr] + (bi * DCTSIZE);
	    for (elemc = 0; elemc < DCTSIZE; elemc++) {
	      temp = (*localblkptr++) + CENTERJSAMPLE;
	      if (temp < 0) temp = 0;
	      else if (temp > MAXJSAMPLE) temp = MAXJSAMPLE;
	      *elemptr++ = (JSAMPLE) temp;
	    }
	  }
	}
      }
    }
  }
}


/*
 * Initialize for processing a scan.
 */

METHODDEF void
disassemble_init (decompress_info_ptr cinfo)
{
  /* no work for now */
}


/*
 * Clean up after a scan.
 */

METHODDEF void
disassemble_term (decompress_info_ptr cinfo)
{
  /* no work for now */
}



/*
 * The method selection routine for MCU disassembly.
 */

GLOBAL void
jseldmcu (decompress_info_ptr cinfo)
{
  if (cinfo->comps_in_scan == 1)
    cinfo->methods->disassemble_MCU = disassemble_noninterleaved_MCU;
  else
    cinfo->methods->disassemble_MCU = disassemble_interleaved_MCU;
  cinfo->methods->reverse_DCT = reverse_DCT;
  cinfo->methods->disassemble_init = disassemble_init;
  cinfo->methods->disassemble_term = disassemble_term;
}
