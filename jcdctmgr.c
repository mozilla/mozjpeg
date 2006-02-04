/*
 * jcdctmgr.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * ---------------------------------------------------------------------
 * x86 SIMD extension for IJG JPEG library
 * Copyright (C) 1999-2006, MIYASAKA Masaru.
 * This file has been modified for SIMD extension.
 * Last Modified : December 24, 2005
 * ---------------------------------------------------------------------
 *
 * This file contains the forward-DCT management logic.
 * This code selects a particular DCT implementation to be used,
 * and it performs related housekeeping chores including coefficient
 * quantization.
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdct.h"		/* Private declarations for DCT subsystem */


/* Private subobject for this module */

typedef struct {
  struct jpeg_forward_dct pub;	/* public fields */

  /* Pointer to the DCT routine actually in use */
  forward_DCT_method_ptr do_dct;
  convsamp_int_method_ptr convsamp;
  quantize_int_method_ptr quantize;

  /* The actual post-DCT divisors --- not identical to the quant table
   * entries, because of scaling (especially for an unnormalized DCT).
   * Each table is given in normal array order.
   */
  DCTELEM * divisors[NUM_QUANT_TBLS];

#ifdef DCT_FLOAT_SUPPORTED
  /* Same as above for the floating-point case. */
  float_DCT_method_ptr do_float_dct;
  convsamp_float_method_ptr float_convsamp;
  quantize_float_method_ptr float_quantize;
  FAST_FLOAT * float_divisors[NUM_QUANT_TBLS];
#endif
} my_fdct_controller;

typedef my_fdct_controller * my_fdct_ptr;

/*
 * SIMD Ext: Most of SSE/SSE2 instructions require that the memory address
 * is aligned to a 16-byte boundary; if not, a general-protection exception
 * (#GP) is generated.
 */

#define ALIGN_SIZE	16		/* sizeof SSE/SSE2 register */
#define ALIGN_MEM(p,a)	((void *) (((size_t) (p) + (a) - 1) & -(a)))

#ifdef JFDCT_INT_QUANTIZE_WITH_DIVISION
#undef jpeg_quantize_int
#undef jpeg_quantize_int_mmx
#undef jpeg_quantize_int_sse2
#define jpeg_quantize_int       jpeg_quantize_idiv
#define jpeg_quantize_int_mmx   jpeg_quantize_idiv
#define jpeg_quantize_int_sse2  jpeg_quantize_idiv
#endif


#ifndef JFDCT_INT_QUANTIZE_WITH_DIVISION

/*
 * SIMD Ext: compute the reciprocal of the divisor
 *
 * This implementation is based on an algorithm described in
 *   "How to optimize for the Pentium family of microprocessors"
 *   (http://www.agner.org/assem/).
 */

LOCAL(void)
compute_reciprocal (DCTELEM divisor, DCTELEM * dtbl)
{
  unsigned long d = ((unsigned long) divisor) & 0x0000FFFF;
  unsigned long fq, fr;
  int b, r, c;

  for (b = 0; (1UL << b) <= d; b++) ;

  r  = 16 + (--b);
  fq = (1UL << r) / d;
  fr = (1UL << r) % d;
  r -= 16;
  c  = 0;

  if (fr == 0) {
    fq >>= 1;
    r--;
  } else if (fr <= (d / 2)) {
    c++;
  } else {
    fq++;
  }

  dtbl[DCTSIZE2 * 0] = (DCTELEM) fq;		/* reciprocal */
  dtbl[DCTSIZE2 * 1] = (DCTELEM) (c + (d / 2));	/* correction + roundfactor */
  dtbl[DCTSIZE2 * 2] = (DCTELEM) (1 << (16 - (r + 1 + 1)));	/* scale */
  dtbl[DCTSIZE2 * 3] = (DCTELEM) (r + 1);			/* shift */
}

#endif /* JFDCT_INT_QUANTIZE_WITH_DIVISION */


/*
 * Initialize for a processing pass.
 * Verify that all referenced Q-tables are present, and set up
 * the divisor table for each one.
 * In the current implementation, DCT of all components is done during
 * the first pass, even if only some components will be output in the
 * first scan.  Hence all components should be examined here.
 */

METHODDEF(void)
start_pass_fdctmgr (j_compress_ptr cinfo)
{
  my_fdct_ptr fdct = (my_fdct_ptr) cinfo->fdct;
  int ci, qtblno, i;
  jpeg_component_info *compptr;
  JQUANT_TBL * qtbl;
  DCTELEM * dtbl;

  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    qtblno = compptr->quant_tbl_no;
    /* Make sure specified quantization table is present */
    if (qtblno < 0 || qtblno >= NUM_QUANT_TBLS ||
	cinfo->quant_tbl_ptrs[qtblno] == NULL)
      ERREXIT1(cinfo, JERR_NO_QUANT_TABLE, qtblno);
    qtbl = cinfo->quant_tbl_ptrs[qtblno];
    /* Compute divisors for this quant table */
    /* We may do this more than once for same table, but it's not a big deal */
    switch (cinfo->dct_method) {
#ifdef DCT_ISLOW_SUPPORTED
    case JDCT_ISLOW:
      /* For LL&M IDCT method, divisors are equal to raw quantization
       * coefficients multiplied by 8 (to counteract scaling).
       */
#ifndef JFDCT_INT_QUANTIZE_WITH_DIVISION
      if (fdct->divisors[qtblno] == NULL) {
	fdct->divisors[qtblno] = (DCTELEM *)
	  (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				      (DCTSIZE2 * 4) * SIZEOF(DCTELEM));
      }
      dtbl = fdct->divisors[qtblno];
      for (i = 0; i < DCTSIZE2; i++) {
	compute_reciprocal ((DCTELEM) (qtbl->quantval[i] << 3), &dtbl[i]);
      }
      break;
#else  /* JFDCT_INT_QUANTIZE_WITH_DIVISION */
      if (fdct->divisors[qtblno] == NULL) {
	fdct->divisors[qtblno] = (DCTELEM *)
	  (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				      DCTSIZE2 * SIZEOF(DCTELEM));
      }
      dtbl = fdct->divisors[qtblno];
      for (i = 0; i < DCTSIZE2; i++) {
	dtbl[i] = ((DCTELEM) qtbl->quantval[i]) << 3;
      }
      break;
#endif /* JFDCT_INT_QUANTIZE_WITH_DIVISION */
#endif /* DCT_ISLOW_SUPPORTED */
#ifdef DCT_IFAST_SUPPORTED
    case JDCT_IFAST:
      {
	/* For AA&N IDCT method, divisors are equal to quantization
	 * coefficients scaled by scalefactor[row]*scalefactor[col], where
	 *   scalefactor[0] = 1
	 *   scalefactor[k] = cos(k*PI/16) * sqrt(2)    for k=1..7
	 * We apply a further scale factor of 8.
	 */
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

#ifndef JFDCT_INT_QUANTIZE_WITH_DIVISION
	if (fdct->divisors[qtblno] == NULL) {
	  fdct->divisors[qtblno] = (DCTELEM *)
	    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
					(DCTSIZE2 * 4) * SIZEOF(DCTELEM));
	}
	dtbl = fdct->divisors[qtblno];
	for (i = 0; i < DCTSIZE2; i++) {
	  compute_reciprocal ((DCTELEM)
			       DESCALE(MULTIPLY16V16((INT32) qtbl->quantval[i],
						     (INT32) aanscales[i]),
				       CONST_BITS-3),
			      &dtbl[i]);
	}
#else  /* JFDCT_INT_QUANTIZE_WITH_DIVISION */
	if (fdct->divisors[qtblno] == NULL) {
	  fdct->divisors[qtblno] = (DCTELEM *)
	    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
					DCTSIZE2 * SIZEOF(DCTELEM));
	}
	dtbl = fdct->divisors[qtblno];
	for (i = 0; i < DCTSIZE2; i++) {
	  dtbl[i] = (DCTELEM)
	    DESCALE(MULTIPLY16V16((INT32) qtbl->quantval[i],
				  (INT32) aanscales[i]),
		    CONST_BITS-3);
	}
#endif /* JFDCT_INT_QUANTIZE_WITH_DIVISION */
      }
      break;
#endif /* DCT_IFAST_SUPPORTED */
#ifdef DCT_FLOAT_SUPPORTED
    case JDCT_FLOAT:
      {
	/* For float AA&N IDCT method, divisors are equal to quantization
	 * coefficients scaled by scalefactor[row]*scalefactor[col], where
	 *   scalefactor[0] = 1
	 *   scalefactor[k] = cos(k*PI/16) * sqrt(2)    for k=1..7
	 * We apply a further scale factor of 8.
	 * What's actually stored is 1/divisor so that the inner loop can
	 * use a multiplication rather than a division.
	 */
	FAST_FLOAT * fdtbl;
	int row, col;
	static const double aanscalefactor[DCTSIZE] = {
	  1.0, 1.387039845, 1.306562965, 1.175875602,
	  1.0, 0.785694958, 0.541196100, 0.275899379
	};

	if (fdct->float_divisors[qtblno] == NULL) {
	  fdct->float_divisors[qtblno] = (FAST_FLOAT *)
	    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
					DCTSIZE2 * SIZEOF(FAST_FLOAT));
	}
	fdtbl = fdct->float_divisors[qtblno];
	i = 0;
	for (row = 0; row < DCTSIZE; row++) {
	  for (col = 0; col < DCTSIZE; col++) {
	    fdtbl[i] = (FAST_FLOAT)
	      (1.0 / (((double) qtbl->quantval[i] *
		       aanscalefactor[row] * aanscalefactor[col] * 8.0)));
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
 * Perform forward DCT on one or more blocks of a component.
 *
 * The input samples are taken from the sample_data[] array starting at
 * position start_row/start_col, and moving to the right for any additional
 * blocks. The quantized coefficients are returned in coef_blocks[].
 */

METHODDEF(void)
forward_DCT (j_compress_ptr cinfo, jpeg_component_info * compptr,
	     JSAMPARRAY sample_data, JBLOCKROW coef_blocks,
	     JDIMENSION start_row, JDIMENSION start_col,
	     JDIMENSION num_blocks)
/* This version is used for integer DCT implementations. */
{
  my_fdct_ptr fdct = (my_fdct_ptr) cinfo->fdct;
  DCTELEM * divisors = fdct->divisors[compptr->quant_tbl_no];
  DCTELEM workspace[DCTSIZE2 + ALIGN_SIZE/sizeof(DCTELEM)];
  DCTELEM * wkptr = (DCTELEM *) ALIGN_MEM(workspace, ALIGN_SIZE);
  JDIMENSION bi;

  sample_data += start_row;	/* fold in the vertical offset once */

  for (bi = 0; bi < num_blocks; bi++, start_col += DCTSIZE) {
    /* Load data into workspace, applying unsigned->signed conversion */
    (*fdct->convsamp) (sample_data, start_col, wkptr);

    /* Perform the DCT */
    (*fdct->do_dct) (wkptr);

    /* Quantize/descale the coefficients, and store into coef_blocks[] */
    (*fdct->quantize) (coef_blocks[bi], divisors, wkptr);
  }
}


#ifdef DCT_FLOAT_SUPPORTED

METHODDEF(void)
forward_DCT_float (j_compress_ptr cinfo, jpeg_component_info * compptr,
		   JSAMPARRAY sample_data, JBLOCKROW coef_blocks,
		   JDIMENSION start_row, JDIMENSION start_col,
		   JDIMENSION num_blocks)
/* This version is used for floating-point DCT implementations. */
{
  my_fdct_ptr fdct = (my_fdct_ptr) cinfo->fdct;
  FAST_FLOAT * divisors = fdct->float_divisors[compptr->quant_tbl_no];
  FAST_FLOAT workspace[DCTSIZE2 + ALIGN_SIZE/sizeof(FAST_FLOAT)];
  FAST_FLOAT * wkptr = (FAST_FLOAT *) ALIGN_MEM(workspace, ALIGN_SIZE);
  JDIMENSION bi;

  sample_data += start_row;	/* fold in the vertical offset once */

  for (bi = 0; bi < num_blocks; bi++, start_col += DCTSIZE) {
    /* Load data into workspace, applying unsigned->signed conversion */
    (*fdct->float_convsamp) (sample_data, start_col, wkptr);

    /* Perform the DCT */
    (*fdct->do_float_dct) (wkptr);

    /* Quantize/descale the coefficients, and store into coef_blocks[] */
    (*fdct->float_quantize) (coef_blocks[bi], divisors, wkptr);
  }
}

#endif /* DCT_FLOAT_SUPPORTED */


/*
 * Initialize FDCT manager.
 */

GLOBAL(void)
jinit_forward_dct (j_compress_ptr cinfo)
{
  my_fdct_ptr fdct;
  int i;
  unsigned int simd = jpeg_simd_support((j_common_ptr) cinfo);

  fdct = (my_fdct_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(my_fdct_controller));
  cinfo->fdct = (struct jpeg_forward_dct *) fdct;
  fdct->pub.start_pass = start_pass_fdctmgr;

  switch (cinfo->dct_method) {
#ifdef DCT_ISLOW_SUPPORTED
  case JDCT_ISLOW:
    fdct->pub.forward_DCT = forward_DCT;
#ifdef JFDCT_INT_SSE2_SUPPORTED
    if (simd & JSIMD_SSE2 &&
        IS_CONST_ALIGNED_16(jconst_fdct_islow_sse2)) {
      fdct->do_dct = jpeg_fdct_islow_sse2;
      fdct->convsamp = jpeg_convsamp_int_sse2;
      fdct->quantize = jpeg_quantize_int_sse2;
    } else
#endif
#ifdef JFDCT_INT_MMX_SUPPORTED
    if (simd & JSIMD_MMX) {
      fdct->do_dct = jpeg_fdct_islow_mmx;
      fdct->convsamp = jpeg_convsamp_int_mmx;
      fdct->quantize = jpeg_quantize_int_mmx;
    } else
#endif
    {
      fdct->do_dct = jpeg_fdct_islow;
      fdct->convsamp = jpeg_convsamp_int;
      fdct->quantize = jpeg_quantize_int;
    }
    break;
#endif /* DCT_ISLOW_SUPPORTED */
#ifdef DCT_IFAST_SUPPORTED
  case JDCT_IFAST:
    fdct->pub.forward_DCT = forward_DCT;
#ifdef JFDCT_INT_SSE2_SUPPORTED
    if (simd & JSIMD_SSE2 &&
        IS_CONST_ALIGNED_16(jconst_fdct_ifast_sse2)) {
      fdct->do_dct = jpeg_fdct_ifast_sse2;
      fdct->convsamp = jpeg_convsamp_int_sse2;
      fdct->quantize = jpeg_quantize_int_sse2;
    } else
#endif
#ifdef JFDCT_INT_MMX_SUPPORTED
    if (simd & JSIMD_MMX) {
      fdct->do_dct = jpeg_fdct_ifast_mmx;
      fdct->convsamp = jpeg_convsamp_int_mmx;
      fdct->quantize = jpeg_quantize_int_mmx;
    } else
#endif
    {
      fdct->do_dct = jpeg_fdct_ifast;
      fdct->convsamp = jpeg_convsamp_int;
      fdct->quantize = jpeg_quantize_int;
    }
    break;
#endif /* DCT_IFAST_SUPPORTED */
#ifdef DCT_FLOAT_SUPPORTED
  case JDCT_FLOAT:
    fdct->pub.forward_DCT = forward_DCT_float;
#ifdef JFDCT_FLT_SSE_SSE2_SUPPORTED
    if (simd & JSIMD_SSE && simd & JSIMD_SSE2 &&
        IS_CONST_ALIGNED_16(jconst_fdct_float_sse)) {
      fdct->do_float_dct = jpeg_fdct_float_sse;
      fdct->float_convsamp = jpeg_convsamp_flt_sse2;
      fdct->float_quantize = jpeg_quantize_flt_sse2;
    } else
#endif
#ifdef JFDCT_FLT_SSE_MMX_SUPPORTED
    if (simd & JSIMD_SSE &&
        IS_CONST_ALIGNED_16(jconst_fdct_float_sse)) {
      fdct->do_float_dct = jpeg_fdct_float_sse;
      fdct->float_convsamp = jpeg_convsamp_flt_sse;
      fdct->float_quantize = jpeg_quantize_flt_sse;
    } else
#endif
#ifdef JFDCT_FLT_3DNOW_MMX_SUPPORTED
    if (simd & JSIMD_3DNOW) {
      fdct->do_float_dct = jpeg_fdct_float_3dnow;
      fdct->float_convsamp = jpeg_convsamp_flt_3dnow;
      fdct->float_quantize = jpeg_quantize_flt_3dnow;
    } else
#endif
    {
      fdct->do_float_dct = jpeg_fdct_float;
      fdct->float_convsamp = jpeg_convsamp_float;
      fdct->float_quantize = jpeg_quantize_float;
    }
    break;
#endif /* DCT_FLOAT_SUPPORTED */
  default:
    ERREXIT(cinfo, JERR_NOT_COMPILED);
    break;
  }

  /* Mark divisor tables unallocated */
  for (i = 0; i < NUM_QUANT_TBLS; i++) {
    fdct->divisors[i] = NULL;
#ifdef DCT_FLOAT_SUPPORTED
    fdct->float_divisors[i] = NULL;
#endif
  }
}


#ifndef JSIMD_MODEINFO_NOT_SUPPORTED

GLOBAL(unsigned int)
jpeg_simd_forward_dct (j_compress_ptr cinfo, int method)
{
  unsigned int simd = jpeg_simd_support((j_common_ptr) cinfo);

  switch (method) {
#ifdef DCT_ISLOW_SUPPORTED
  case JDCT_ISLOW:
#ifdef JFDCT_INT_SSE2_SUPPORTED
    if (simd & JSIMD_SSE2 &&
        IS_CONST_ALIGNED_16(jconst_fdct_islow_sse2))
      return JSIMD_SSE2;
#endif
#ifdef JFDCT_INT_MMX_SUPPORTED
    if (simd & JSIMD_MMX)
      return JSIMD_MMX;
#endif
    return JSIMD_NONE;
#endif /* DCT_ISLOW_SUPPORTED */
#ifdef DCT_IFAST_SUPPORTED
  case JDCT_IFAST:
#ifdef JFDCT_INT_SSE2_SUPPORTED
    if (simd & JSIMD_SSE2 &&
        IS_CONST_ALIGNED_16(jconst_fdct_ifast_sse2))
      return JSIMD_SSE2;
#endif
#ifdef JFDCT_INT_MMX_SUPPORTED
    if (simd & JSIMD_MMX)
      return JSIMD_MMX;
#endif
    return JSIMD_NONE;
#endif /* DCT_IFAST_SUPPORTED */
#ifdef DCT_FLOAT_SUPPORTED
  case JDCT_FLOAT:
#ifdef JFDCT_FLT_SSE_SSE2_SUPPORTED
    if (simd & JSIMD_SSE && simd & JSIMD_SSE2 &&
        IS_CONST_ALIGNED_16(jconst_fdct_float_sse))
      return JSIMD_SSE;		/* (JSIMD_SSE | JSIMD_SSE2); */
#endif
#ifdef JFDCT_FLT_SSE_MMX_SUPPORTED
    if (simd & JSIMD_SSE &&
        IS_CONST_ALIGNED_16(jconst_fdct_float_sse))
      return JSIMD_SSE;		/* (JSIMD_SSE | JSIMD_MMX); */
#endif
#ifdef JFDCT_FLT_3DNOW_MMX_SUPPORTED
    if (simd & JSIMD_3DNOW)
      return JSIMD_3DNOW;	/* (JSIMD_3DNOW | JSIMD_MMX); */
#endif
    return JSIMD_NONE;
#endif /* DCT_FLOAT_SUPPORTED */
  default:
    ;
  }

  return JSIMD_NONE;	/* not compiled */
}

#endif /* !JSIMD_MODEINFO_NOT_SUPPORTED */
