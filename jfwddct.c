/*
 * jfwddct.c
 *
 * Copyright (C) 1991, 1992, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains the basic DCT (Discrete Cosine Transform)
 * transformation subroutine.
 *
 * This implementation is based on Appendix A.2 of the book
 * "Discrete Cosine Transform---Algorithms, Advantages, Applications"
 * by K.R. Rao and P. Yip  (Academic Press, Inc, London, 1990).
 * It uses scaled fixed-point arithmetic instead of floating point.
 */

#include "jinclude.h"

/*
 * This routine is specialized to the case DCTSIZE = 8.
 */

#if DCTSIZE != 8
  Sorry, this code only copes with 8x8 DCTs. /* deliberate syntax err */
#endif


/* The poop on this scaling stuff is as follows:
 *
 * We have to do addition and subtraction of the integer inputs, which
 * is no problem, and multiplication by fractional constants, which is
 * a problem to do in integer arithmetic.  We multiply all the constants
 * by DCT_SCALE and convert them to integer constants (thus retaining
 * LG2_DCT_SCALE bits of precision in the constants).  After doing a
 * multiplication we have to divide the product by DCT_SCALE, with proper
 * rounding, to produce the correct output.  The division can be implemented
 * cheaply as a right shift of LG2_DCT_SCALE bits.  The DCT equations also
 * specify an additional division by 2 on the final outputs; this can be
 * folded into the right-shift by shifting one more bit (see UNFIXH).
 *
 * If you are planning to recode this in assembler, you might want to set
 * LG2_DCT_SCALE to 15.  This loses a bit of precision, but then all the
 * multiplications are between 16-bit quantities (given 8-bit JSAMPLEs!)
 * so you could use a signed 16x16=>32 bit multiply instruction instead of
 * full 32x32 multiply.  Unfortunately there's no way to describe such a
 * multiply portably in C, so we've gone for the extra bit of accuracy here.
 */

#ifdef EIGHT_BIT_SAMPLES
#define LG2_DCT_SCALE 16
#else
#define LG2_DCT_SCALE 15	/* lose a little precision to avoid overflow */
#endif

#define ONE	((INT32) 1)

#define DCT_SCALE (ONE << LG2_DCT_SCALE)

/* In some places we shift the inputs left by a couple more bits, */
/* so that they can be added to fractional results without too much */
/* loss of precision. */
#define LG2_OVERSCALE 2
#define OVERSCALE  (ONE << LG2_OVERSCALE)
#define OVERSHIFT(x)  ((x) <<= LG2_OVERSCALE)

/* Scale a fractional constant by DCT_SCALE */
#define FIX(x)	((INT32) ((x) * DCT_SCALE + 0.5))

/* Scale a fractional constant by DCT_SCALE/OVERSCALE */
/* Such a constant can be multiplied with an overscaled input */
/* to produce something that's scaled by DCT_SCALE */
#define FIXO(x)  ((INT32) ((x) * DCT_SCALE / OVERSCALE + 0.5))

/* Descale and correctly round a value that's scaled by DCT_SCALE */
#define UNFIX(x)   RIGHT_SHIFT((x) + (ONE << (LG2_DCT_SCALE-1)), LG2_DCT_SCALE)

/* Same with an additional division by 2, ie, correctly rounded UNFIX(x/2) */
#define UNFIXH(x)  RIGHT_SHIFT((x) + (ONE << LG2_DCT_SCALE), LG2_DCT_SCALE+1)

/* Take a value scaled by DCT_SCALE and round to integer scaled by OVERSCALE */
#define UNFIXO(x)  RIGHT_SHIFT((x) + (ONE << (LG2_DCT_SCALE-1-LG2_OVERSCALE)),\
			       LG2_DCT_SCALE-LG2_OVERSCALE)

/* Here are the constants we need */
/* SIN_i_j is sine of i*pi/j, scaled by DCT_SCALE */
/* COS_i_j is cosine of i*pi/j, scaled by DCT_SCALE */

#define SIN_1_4 FIX(0.707106781)
#define COS_1_4 SIN_1_4

#define SIN_1_8 FIX(0.382683432)
#define COS_1_8 FIX(0.923879533)
#define SIN_3_8 COS_1_8
#define COS_3_8 SIN_1_8

#define SIN_1_16 FIX(0.195090322)
#define COS_1_16 FIX(0.980785280)
#define SIN_7_16 COS_1_16
#define COS_7_16 SIN_1_16

#define SIN_3_16 FIX(0.555570233)
#define COS_3_16 FIX(0.831469612)
#define SIN_5_16 COS_3_16
#define COS_5_16 SIN_3_16

/* OSIN_i_j is sine of i*pi/j, scaled by DCT_SCALE/OVERSCALE */
/* OCOS_i_j is cosine of i*pi/j, scaled by DCT_SCALE/OVERSCALE */

#define OSIN_1_4 FIXO(0.707106781)
#define OCOS_1_4 OSIN_1_4

#define OSIN_1_8 FIXO(0.382683432)
#define OCOS_1_8 FIXO(0.923879533)
#define OSIN_3_8 OCOS_1_8
#define OCOS_3_8 OSIN_1_8

#define OSIN_1_16 FIXO(0.195090322)
#define OCOS_1_16 FIXO(0.980785280)
#define OSIN_7_16 OCOS_1_16
#define OCOS_7_16 OSIN_1_16

#define OSIN_3_16 FIXO(0.555570233)
#define OCOS_3_16 FIXO(0.831469612)
#define OSIN_5_16 OCOS_3_16
#define OCOS_5_16 OSIN_3_16


/*
 * Perform the forward DCT on one block of samples.
 *
 * A 2-D DCT can be done by 1-D DCT on each row
 * followed by 1-D DCT on each column.
 */

GLOBAL void
j_fwd_dct (DCTBLOCK data)
{
  int pass, rowctr;
  register DCTELEM *inptr, *outptr;
  DCTBLOCK workspace;

  /* Each iteration of the inner loop performs one 8-point 1-D DCT.
   * It reads from a *row* of the input matrix and stores into a *column*
   * of the output matrix.  In the first pass, we read from the data[] array
   * and store into the local workspace[].  In the second pass, we read from
   * the workspace[] array and store into data[], thus performing the
   * equivalent of a columnar DCT pass with no variable array indexing.
   */

  inptr = data;			/* initialize pointers for first pass */
  outptr = workspace;
  for (pass = 1; pass >= 0; pass--) {
    for (rowctr = DCTSIZE-1; rowctr >= 0; rowctr--) {
      /* many tmps have nonoverlapping lifetime -- flashy register colourers
       * should be able to do this lot very well
       */
      INT32 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
      INT32 tmp10, tmp11, tmp12, tmp13;
      INT32 tmp14, tmp15, tmp16, tmp17;
      INT32 tmp25, tmp26;
      SHIFT_TEMPS

      tmp0 = inptr[7] + inptr[0];
      tmp1 = inptr[6] + inptr[1];
      tmp2 = inptr[5] + inptr[2];
      tmp3 = inptr[4] + inptr[3];
      tmp4 = inptr[3] - inptr[4];
      tmp5 = inptr[2] - inptr[5];
      tmp6 = inptr[1] - inptr[6];
      tmp7 = inptr[0] - inptr[7];
      
      tmp10 = tmp3 + tmp0;
      tmp11 = tmp2 + tmp1;
      tmp12 = tmp1 - tmp2;
      tmp13 = tmp0 - tmp3;
      
      outptr[        0] = (DCTELEM) UNFIXH((tmp10 + tmp11) * SIN_1_4);
      outptr[DCTSIZE*4] = (DCTELEM) UNFIXH((tmp10 - tmp11) * COS_1_4);
      
      outptr[DCTSIZE*2] = (DCTELEM) UNFIXH(tmp13*COS_1_8 + tmp12*SIN_1_8);
      outptr[DCTSIZE*6] = (DCTELEM) UNFIXH(tmp13*SIN_1_8 - tmp12*COS_1_8);
      
      tmp16 = UNFIXO((tmp6 + tmp5) * SIN_1_4);
      tmp15 = UNFIXO((tmp6 - tmp5) * COS_1_4);
      
      OVERSHIFT(tmp4);
      OVERSHIFT(tmp7);
      
      /* tmp4, tmp7, tmp15, tmp16 are overscaled by OVERSCALE */
      
      tmp14 = tmp4 + tmp15;
      tmp25 = tmp4 - tmp15;
      tmp26 = tmp7 - tmp16;
      tmp17 = tmp7 + tmp16;
      
      outptr[DCTSIZE  ] = (DCTELEM) UNFIXH(tmp17*OCOS_1_16 + tmp14*OSIN_1_16);
      outptr[DCTSIZE*7] = (DCTELEM) UNFIXH(tmp17*OCOS_7_16 - tmp14*OSIN_7_16);
      outptr[DCTSIZE*5] = (DCTELEM) UNFIXH(tmp26*OCOS_5_16 + tmp25*OSIN_5_16);
      outptr[DCTSIZE*3] = (DCTELEM) UNFIXH(tmp26*OCOS_3_16 - tmp25*OSIN_3_16);

      inptr += DCTSIZE;		/* advance inptr to next row */
      outptr++;			/* advance outptr to next column */
    }
    /* end of pass; in case it was pass 1, set up for pass 2 */
    inptr = workspace;
    outptr = data;
  }
}
