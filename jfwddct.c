/*
 * jfwddct.c
 *
 * Copyright (C) 1991, Thomas G. Lane.
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


/* We assume that right shift corresponds to signed division by 2 with
 * rounding towards minus infinity.  This is correct for typical "arithmetic
 * shift" instructions that shift in copies of the sign bit.  But some
 * C compilers implement >> with an unsigned shift.  For these machines you
 * must define RIGHT_SHIFT_IS_UNSIGNED.
 * RIGHT_SHIFT provides a signed right shift of an INT32 quantity.
 * It is only applied with constant shift counts.
 */

#ifdef RIGHT_SHIFT_IS_UNSIGNED
#define SHIFT_TEMPS	INT32 shift_temp;
#define RIGHT_SHIFT(x,shft)  \
	((shift_temp = (x)) < 0 ? \
	 (shift_temp >> (shft)) | ((~0) << (32-(shft))) : \
	 (shift_temp >> (shft)))
#else
#define SHIFT_TEMPS
#define RIGHT_SHIFT(x,shft)	((x) >> (shft))
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
 * Perform a 1-dimensional DCT.
 * Note that this code is specialized to the case DCTSIZE = 8.
 */

INLINE
LOCAL void
fast_dct_8 (DCTELEM *in, int stride)
{
  /* many tmps have nonoverlapping lifetime -- flashy register colourers
   * should be able to do this lot very well
   */
  INT32 in0, in1, in2, in3, in4, in5, in6, in7;
  INT32 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  INT32 tmp10, tmp11, tmp12, tmp13;
  INT32 tmp14, tmp15, tmp16, tmp17;
  INT32 tmp25, tmp26;
  SHIFT_TEMPS
  
  in0 = in[       0];
  in1 = in[stride  ];
  in2 = in[stride*2];
  in3 = in[stride*3];
  in4 = in[stride*4];
  in5 = in[stride*5];
  in6 = in[stride*6];
  in7 = in[stride*7];
  
  tmp0 = in7 + in0;
  tmp1 = in6 + in1;
  tmp2 = in5 + in2;
  tmp3 = in4 + in3;
  tmp4 = in3 - in4;
  tmp5 = in2 - in5;
  tmp6 = in1 - in6;
  tmp7 = in0 - in7;
  
  tmp10 = tmp3 + tmp0;
  tmp11 = tmp2 + tmp1;
  tmp12 = tmp1 - tmp2;
  tmp13 = tmp0 - tmp3;
  
  in[       0] = (DCTELEM) UNFIXH((tmp10 + tmp11) * SIN_1_4);
  in[stride*4] = (DCTELEM) UNFIXH((tmp10 - tmp11) * COS_1_4);
  
  in[stride*2] = (DCTELEM) UNFIXH(tmp13*COS_1_8 + tmp12*SIN_1_8);
  in[stride*6] = (DCTELEM) UNFIXH(tmp13*SIN_1_8 - tmp12*COS_1_8);

  tmp16 = UNFIXO((tmp6 + tmp5) * SIN_1_4);
  tmp15 = UNFIXO((tmp6 - tmp5) * COS_1_4);

  OVERSHIFT(tmp4);
  OVERSHIFT(tmp7);

  /* tmp4, tmp7, tmp15, tmp16 are overscaled by OVERSCALE */

  tmp14 = tmp4 + tmp15;
  tmp25 = tmp4 - tmp15;
  tmp26 = tmp7 - tmp16;
  tmp17 = tmp7 + tmp16;
  
  in[stride  ] = (DCTELEM) UNFIXH(tmp17*OCOS_1_16 + tmp14*OSIN_1_16);
  in[stride*7] = (DCTELEM) UNFIXH(tmp17*OCOS_7_16 - tmp14*OSIN_7_16);
  in[stride*5] = (DCTELEM) UNFIXH(tmp26*OCOS_5_16 + tmp25*OSIN_5_16);
  in[stride*3] = (DCTELEM) UNFIXH(tmp26*OCOS_3_16 - tmp25*OSIN_3_16);
}


/*
 * Perform the forward DCT on one block of samples.
 *
 * A 2-D DCT can be done by 1-D DCT on each row
 * followed by 1-D DCT on each column.
 */

GLOBAL void
j_fwd_dct (DCTBLOCK data)
{
  int i;
  
  for (i = 0; i < DCTSIZE; i++)
    fast_dct_8(data+i*DCTSIZE, 1);

  for (i = 0; i < DCTSIZE; i++)
    fast_dct_8(data+i, DCTSIZE);
}
