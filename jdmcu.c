/*
 * jdmcu.c
 *
 * Copyright (C) 1991, 1992, 1993, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains MCU disassembly and IDCT control routines.
 * These routines are invoked via the disassemble_MCU, reverse_DCT, and
 * disassemble_init/term methods.
 */

#include "jinclude.h"


/*
 * Fetch one MCU row from entropy_decode, build coefficient array.
 * This version is used for noninterleaved (single-component) scans.
 */

METHODDEF void
disassemble_noninterleaved_MCU (decompress_info_ptr cinfo,
				JBLOCKIMAGE image_data)
{
  JBLOCKROW MCU_data[1];
  long mcuindex;

  /* this is pretty easy since there is one component and one block per MCU */

  /* Pre-zero the target area to speed up entropy decoder */
  /* (we assume wholesale zeroing is faster than retail) */
  jzero_far((void FAR *) image_data[0][0],
	    (size_t) (cinfo->MCUs_per_row * SIZEOF(JBLOCK)));

  for (mcuindex = 0; mcuindex < cinfo->MCUs_per_row; mcuindex++) {
    /* Point to the proper spot in the image array for this MCU */
    MCU_data[0] = image_data[0][0] + mcuindex;
    /* Fetch the coefficient data */
    (*cinfo->methods->entropy_decode) (cinfo, MCU_data);
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
  JBLOCKROW MCU_data[MAX_BLOCKS_IN_MCU];
  long mcuindex;
  short blkn, ci, xpos, ypos;
  jpeg_component_info * compptr;
  JBLOCKROW image_ptr;

  /* Pre-zero the target area to speed up entropy decoder */
  /* (we assume wholesale zeroing is faster than retail) */
  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    for (ypos = 0; ypos < compptr->MCU_height; ypos++) {
      jzero_far((void FAR *) image_data[ci][ypos],
		(size_t) (cinfo->MCUs_per_row * compptr->MCU_width * SIZEOF(JBLOCK)));
    }
  }

  for (mcuindex = 0; mcuindex < cinfo->MCUs_per_row; mcuindex++) {
    /* Point to the proper spots in the image array for this MCU */
    blkn = 0;
    for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
      compptr = cinfo->cur_comp_info[ci];
      for (ypos = 0; ypos < compptr->MCU_height; ypos++) {
	image_ptr = image_data[ci][ypos] + (mcuindex * compptr->MCU_width);
	for (xpos = 0; xpos < compptr->MCU_width; xpos++) {
	  MCU_data[blkn] = image_ptr;
	  image_ptr++;
	  blkn++;
	}
      }
    }
    /* Fetch the coefficient data */
    (*cinfo->methods->entropy_decode) (cinfo, MCU_data);
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
  jpeg_component_info * compptr;
  long blocksperrow, bi;
  short numrows, ri;
  short ci;

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    /* don't bother to IDCT an uninteresting component */
    if (! compptr->component_needed)
      continue;
    /* calculate size of an MCU row in this component */
    blocksperrow = compptr->downsampled_width / DCTSIZE;
    numrows = compptr->MCU_height;
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
	  register int elem = DCTSIZE2;

	  while (--elem >= 0)
	    *localblkptr++ = (DCTELEM) *elemptr++;
	}

	j_rev_dct(block);	/* perform inverse DCT */

	/* Output the data into the sample array.
	 * Note change from signed to unsigned representation:
	 * DCT calculation works with values +-CENTERJSAMPLE,
	 * but sample arrays always hold 0..MAXJSAMPLE.
	 * We have to do range-limiting because of quantization errors in the
	 * DCT/IDCT phase.  We use the sample_range_limit[] table to do this
	 * quickly; the CENTERJSAMPLE offset is folded into table indexing.
	 */
	{ register JSAMPROW elemptr;
	  register DCTELEM *localblkptr = block;
	  register JSAMPLE *range_limit = cinfo->sample_range_limit +
						CENTERJSAMPLE;
#if DCTSIZE != 8
	  register int elemc;
#endif
	  register int elemr;

	  for (elemr = 0; elemr < DCTSIZE; elemr++) {
	    elemptr = srowptr[elemr] + (bi * DCTSIZE);
#if DCTSIZE == 8		/* unroll the inner loop */
	    *elemptr++ = range_limit[*localblkptr++];
	    *elemptr++ = range_limit[*localblkptr++];
	    *elemptr++ = range_limit[*localblkptr++];
	    *elemptr++ = range_limit[*localblkptr++];
	    *elemptr++ = range_limit[*localblkptr++];
	    *elemptr++ = range_limit[*localblkptr++];
	    *elemptr++ = range_limit[*localblkptr++];
	    *elemptr++ = range_limit[*localblkptr++];
#else
	    for (elemc = DCTSIZE; elemc > 0; elemc--) {
	      *elemptr++ = range_limit[*localblkptr++];
	    }
#endif
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
