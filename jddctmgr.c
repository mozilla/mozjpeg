/*
 * jddctmgr.c
 *
 * Copyright (C) 1994, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains the inverse-DCT management logic.
 * This code selects a particular IDCT implementation to be used,
 * and it performs related housekeeping chores.  No code in this file
 * is executed per IDCT step, only during pass setup.
 *
 * Note that the IDCT routines are responsible for performing coefficient
 * dequantization as well as the IDCT proper.  This module sets up the
 * dequantization multiplier table needed by the IDCT routine.
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdct.h"		/* Private declarations for DCT subsystem */


/* Private subobject for this module */

typedef struct {
  struct jpeg_inverse_dct pub;	/* public fields */

  /* Record the IDCT method type actually selected for each component */
  J_DCT_METHOD real_method[MAX_COMPONENTS];
} my_idct_controller;

typedef my_idct_controller * my_idct_ptr;


/* ZIG[i] is the zigzag-order position of the i'th element of a DCT block */
/* read in natural order (left to right, top to bottom). */
static const int ZIG[DCTSIZE2] = {
     0,  1,  5,  6, 14, 15, 27, 28,
     2,  4,  7, 13, 16, 26, 29, 42,
     3,  8, 12, 17, 25, 30, 41, 43,
     9, 11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63
};


/* The current scaled-IDCT routines require ISLOW-style multiplier tables,
 * so be sure to compile that code if either ISLOW or SCALING is requested.
 */
#ifdef DCT_ISLOW_SUPPORTED
#define PROVIDE_ISLOW_TABLES
#else
#ifdef IDCT_SCALING_SUPPORTED
#define PROVIDE_ISLOW_TABLES
#endif
#endif


/*
 * Initialize for an input scan.
 *
 * Verify that all referenced Q-tables are present, and set up
 * the multiplier table for each one.
 * With a multiple-scan JPEG file, this is called during each input scan,
 * NOT during the final output pass where the IDCT is actually done.
 * The purpose is to save away the current Q-table contents just in case
 * the encoder changes tables between scans.  This decoder will dequantize
 * any component using the Q-table which was current at the start of the
 * first scan using that component.
 */

METHODDEF void
start_input_pass (j_decompress_ptr cinfo)
{
  my_idct_ptr idct = (my_idct_ptr) cinfo->idct;
  int ci, qtblno, i;
  jpeg_component_info *compptr;
  JQUANT_TBL * qtbl;

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    qtblno = compptr->quant_tbl_no;
    /* Make sure specified quantization table is present */
    if (qtblno < 0 || qtblno >= NUM_QUANT_TBLS ||
	cinfo->quant_tbl_ptrs[qtblno] == NULL)
      ERREXIT1(cinfo, JERR_NO_QUANT_TABLE, qtblno);
    qtbl = cinfo->quant_tbl_ptrs[qtblno];
    /* Create multiplier table from quant table, unless we already did so. */
    if (compptr->dct_table != NULL)
      continue;
    switch (idct->real_method[compptr->component_index]) {
#ifdef PROVIDE_ISLOW_TABLES
    case JDCT_ISLOW:
      {
	/* For LL&M IDCT method, multipliers are equal to raw quantization
	 * coefficients, but are stored in natural order as ints.
	 */
	ISLOW_MULT_TYPE * ismtbl;
	compptr->dct_table =
	  (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				      DCTSIZE2 * SIZEOF(ISLOW_MULT_TYPE));
	ismtbl = (ISLOW_MULT_TYPE *) compptr->dct_table;
	for (i = 0; i < DCTSIZE2; i++) {
	  ismtbl[i] = (ISLOW_MULT_TYPE) qtbl->quantval[ZIG[i]];
	}
      }
      break;
#endif
#ifdef DCT_IFAST_SUPPORTED
    case JDCT_IFAST:
      {
	/* For AA&N IDCT method, multipliers are equal to quantization
	 * coefficients scaled by scalefactor[row]*scalefactor[col], where
	 *   scalefactor[0] = 1
	 *   scalefactor[k] = cos(k*PI/16) * sqrt(2)    for k=1..7
	 * For integer operation, the multiplier table is to be scaled by
	 * IFAST_SCALE_BITS.  The multipliers are stored in natural order.
	 */
	IFAST_MULT_TYPE * ifmtbl;
#define CONST_BITS 14
	static const INT16 aanscales[DCTSIZE2] = {
	  /* precomputed values scaled up by 14 bits */
	  16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
	  22725, 31521, 29692, 26722, 22725, 17855, 12299,  6270,
	  21407, 29692, 27969, 25172, 21407, 16819, 11585,  5906,
	  19266, 26722, 25172, 22654, 19266, 15137, 10426,  5315,
	  16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
	  12873, 17855, 16819, 15137, 12873, 10114,  6967,  3552,
	   8867, 12299, 11585, 10426,  8867,  6967,  4799,  2446,
	   4520,  6270,  5906,  5315,  4520,  3552,  2446,  1247
	};
	SHIFT_TEMPS

	compptr->dct_table =
	  (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				      DCTSIZE2 * SIZEOF(IFAST_MULT_TYPE));
	ifmtbl = (IFAST_MULT_TYPE *) compptr->dct_table;
	for (i = 0; i < DCTSIZE2; i++) {
	  ifmtbl[i] = (IFAST_MULT_TYPE)
	    DESCALE(MULTIPLY16V16((INT32) qtbl->quantval[ZIG[i]],
				  (INT32) aanscales[i]),
		    CONST_BITS-IFAST_SCALE_BITS);
	}
      }
      break;
#endif
#ifdef DCT_FLOAT_SUPPORTED
    case JDCT_FLOAT:
      {
	/* For float AA&N IDCT method, multipliers are equal to quantization
	 * coefficients scaled by scalefactor[row]*scalefactor[col], where
	 *   scalefactor[0] = 1
	 *   scalefactor[k] = cos(k*PI/16) * sqrt(2)    for k=1..7
	 * The multipliers are stored in natural order.
	 */
	FLOAT_MULT_TYPE * fmtbl;
	int row, col;
	static const double aanscalefactor[DCTSIZE] = {
	  1.0, 1.387039845, 1.306562965, 1.175875602,
	  1.0, 0.785694958, 0.541196100, 0.275899379
	};

	compptr->dct_table =
	  (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				      DCTSIZE2 * SIZEOF(FLOAT_MULT_TYPE));
	fmtbl = (FLOAT_MULT_TYPE *) compptr->dct_table;
	i = 0;
	for (row = 0; row < DCTSIZE; row++) {
	  for (col = 0; col < DCTSIZE; col++) {
	    fmtbl[i] = (FLOAT_MULT_TYPE)
	      ((double) qtbl->quantval[ZIG[i]] *
	       aanscalefactor[row] * aanscalefactor[col]);
	    i++;
	  }
	}
      }
      break;
#endif
    default:
      ERREXIT(cinfo, JERR_NOT_COMPILED);
      break;
    }
  }
}


/*
 * Prepare for an output pass that will actually perform IDCTs.
 *
 * start_input_pass should already have been done for all components
 * of interest; we need only verify that this is true.
 * Note that uninteresting components are not required to have loaded tables.
 * This allows the master controller to stop before reading the whole file
 * if it has obtained the data for the interesting component(s).
 */

METHODDEF void
start_output_pass (j_decompress_ptr cinfo)
{
  jpeg_component_info *compptr;
  int ci;

  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    if (! compptr->component_needed)
      continue;
    if (compptr->dct_table == NULL)
      ERREXIT1(cinfo, JERR_NO_QUANT_TABLE, compptr->quant_tbl_no);
  }
}


/*
 * Initialize IDCT manager.
 */

GLOBAL void
jinit_inverse_dct (j_decompress_ptr cinfo)
{
  my_idct_ptr idct;
  int ci;
  jpeg_component_info *compptr;

  idct = (my_idct_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(my_idct_controller));
  cinfo->idct = (struct jpeg_inverse_dct *) idct;
  idct->pub.start_input_pass = start_input_pass;
  idct->pub.start_output_pass = start_output_pass;

  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    compptr->dct_table = NULL;	/* initialize tables to "not prepared" */
    switch (compptr->DCT_scaled_size) {
#ifdef IDCT_SCALING_SUPPORTED
    case 1:
      idct->pub.inverse_DCT[ci] = jpeg_idct_1x1;
      idct->real_method[ci] = JDCT_ISLOW; /* jidctred uses islow-style table */
      break;
    case 2:
      idct->pub.inverse_DCT[ci] = jpeg_idct_2x2;
      idct->real_method[ci] = JDCT_ISLOW; /* jidctred uses islow-style table */
      break;
    case 4:
      idct->pub.inverse_DCT[ci] = jpeg_idct_4x4;
      idct->real_method[ci] = JDCT_ISLOW; /* jidctred uses islow-style table */
      break;
#endif
    case DCTSIZE:
      switch (cinfo->dct_method) {
#ifdef DCT_ISLOW_SUPPORTED
      case JDCT_ISLOW:
	idct->pub.inverse_DCT[ci] = jpeg_idct_islow;
	idct->real_method[ci] = JDCT_ISLOW;
	break;
#endif
#ifdef DCT_IFAST_SUPPORTED
      case JDCT_IFAST:
	idct->pub.inverse_DCT[ci] = jpeg_idct_ifast;
	idct->real_method[ci] = JDCT_IFAST;
	break;
#endif
#ifdef DCT_FLOAT_SUPPORTED
      case JDCT_FLOAT:
	idct->pub.inverse_DCT[ci] = jpeg_idct_float;
	idct->real_method[ci] = JDCT_FLOAT;
	break;
#endif
      default:
	ERREXIT(cinfo, JERR_NOT_COMPILED);
	break;
      }
      break;
    default:
      ERREXIT1(cinfo, JERR_BAD_DCTSIZE, compptr->DCT_scaled_size);
      break;
    }
  }
}
