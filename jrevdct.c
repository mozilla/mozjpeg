/*
 * jrevdct.c
 *
 * Copyright (C) 1991, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains the basic inverse-DCT transformation subroutine.
 *
 * This implementation is based on Appendix A.2 of the book
 * "Discrete Cosine Transform---Algorithms, Advantages, Applications"
 * by K.R. Rao and P. Yip  (Academic Press, Inc, London, 1990).
 * It uses scaled fixed-point arithmetic instead of floating point.
 */

#include "jinclude.h"


/* The poop on this scaling stuff is as follows:
 *
 * Most of the numbers (after multiplication by the constants) are
 * (logically) shifted left by LG2_DCT_SCALE. This is undone by UNFIXH
 * before assignment to the output array. Note that we want an additional
 * division by 2 on the output (required by the equations).
 *
 * If right shifts are unsigned, then there is a potential problem.
 * However, shifting right by 16 and then assigning to a short
 * (assuming short = 16 bits) will keep the sign right!!
 *
 * For other shifts,
 *
 *     ((x + (1 << 30)) >> shft) - (1 << (30 - shft))
 *
 * gives a nice right shift with sign (assuming no overflow). However, all the
 * scaling is such that this isn't a problem. (Is this true?)
 */


#define ONE 1L			/* remove L if long > 32 bits */

#ifdef RIGHT_SHIFT_IS_UNSIGNED
#define LG2_DCT_SCALE 15
#define RIGHT_SHIFT(_x,_shft)   ((((_x) + (ONE << 30)) >> (_shft)) - (ONE << (30 - (_shft))))
#else
#define LG2_DCT_SCALE 16
#define RIGHT_SHIFT(_x,_shft)   ((_x) >> (_shft))
#endif

#define DCT_SCALE (ONE << LG2_DCT_SCALE)

#define LG2_OVERSCALE 2
#define OVERSCALE (ONE << LG2_OVERSCALE)

#define FIX(x)  ((INT32) ((x) * DCT_SCALE + 0.5))
#define FIXO(x)  ((INT32) ((x) * DCT_SCALE / OVERSCALE + 0.5))
#define UNFIX(x)   RIGHT_SHIFT((x) + (ONE << (LG2_DCT_SCALE-1)), LG2_DCT_SCALE)
#define UNFIXH(x)  RIGHT_SHIFT((x) + (ONE << LG2_DCT_SCALE), LG2_DCT_SCALE+1)
#define UNFIXO(x)  RIGHT_SHIFT((x) + (ONE << (LG2_DCT_SCALE-1-LG2_OVERSCALE)), LG2_DCT_SCALE-LG2_OVERSCALE)
#define OVERSH(x)   ((x) << LG2_OVERSCALE)

#define SIN_1_4 FIX(0.7071067811856476)
#define COS_1_4 SIN_1_4

#define SIN_1_8 FIX(0.3826834323650898)
#define COS_1_8 FIX(0.9238795325112870)
#define SIN_3_8 COS_1_8
#define COS_3_8 SIN_1_8

#define SIN_1_16 FIX(0.1950903220161282)
#define COS_1_16 FIX(0.9807852804032300)
#define SIN_7_16 COS_1_16
#define COS_7_16 SIN_1_16

#define SIN_3_16 FIX(0.5555702330196022)
#define COS_3_16 FIX(0.8314696123025450)
#define SIN_5_16 COS_3_16
#define COS_5_16 SIN_3_16

#define OSIN_1_4 FIXO(0.707106781185647)
#define OCOS_1_4 OSIN_1_4

#define OSIN_1_8 FIXO(0.3826834323650898)
#define OCOS_1_8 FIXO(0.9238795325112870)
#define OSIN_3_8 OCOS_1_8
#define OCOS_3_8 OSIN_1_8

#define OSIN_1_16 FIXO(0.1950903220161282)
#define OCOS_1_16 FIXO(0.9807852804032300)
#define OSIN_7_16 OCOS_1_16
#define OCOS_7_16 OSIN_1_16

#define OSIN_3_16 FIXO(0.5555702330196022)
#define OCOS_3_16 FIXO(0.8314696123025450)
#define OSIN_5_16 OCOS_3_16
#define OCOS_5_16 OSIN_3_16


INLINE
LOCAL void
fast_idct_8 (DCTELEM *in, int stride)
{
  /* tmp1x are new values of tmpx -- flashy register colourers
   * should be able to do this lot very well
   */
  INT32 tmp10, tmp11, tmp12, tmp13;
  INT32 tmp20, tmp21, tmp22, tmp23;
  INT32 tmp30, tmp31;
  INT32 tmp40, tmp41, tmp42, tmp43;
  INT32 tmp50, tmp51, tmp52, tmp53;
  INT32 in0, in1, in2, in3, in4, in5, in6, in7;
  
  in0 = in[       0];
  in1 = in[stride  ];
  in2 = in[stride*2];
  in3 = in[stride*3];
  in4 = in[stride*4];
  in5 = in[stride*5];
  in6 = in[stride*6];
  in7 = in[stride*7];
  
  tmp10 = (in0 + in4) * COS_1_4;
  tmp11 = (in0 - in4) * COS_1_4;
  tmp12 = in2 * SIN_1_8 - in6 * COS_1_8;
  tmp13 = in6 * SIN_1_8 + in2 * COS_1_8;
  
  tmp20 = tmp10 + tmp13;
  tmp21 = tmp11 + tmp12;
  tmp22 = tmp11 - tmp12;
  tmp23 = tmp10 - tmp13;
  
  tmp30 = UNFIXO((in3 + in5) * COS_1_4);
  tmp31 = UNFIXO((in3 - in5) * COS_1_4);
  
  tmp40 = OVERSH(in1) + tmp30;
  tmp41 = OVERSH(in7) + tmp31;
  tmp42 = OVERSH(in1) - tmp30;
  tmp43 = OVERSH(in7) - tmp31;
  
  tmp50 = tmp40 * OCOS_1_16 + tmp41 * OSIN_1_16;
  tmp51 = tmp40 * OSIN_1_16 - tmp41 * OCOS_1_16;
  tmp52 = tmp42 * OCOS_5_16 + tmp43 * OSIN_5_16;
  tmp53 = tmp42 * OSIN_5_16 - tmp43 * OCOS_5_16;
  
  in[       0] = UNFIXH(tmp20 + tmp50);
  in[stride  ] = UNFIXH(tmp21 + tmp53);
  in[stride*2] = UNFIXH(tmp22 + tmp52);
  in[stride*3] = UNFIXH(tmp23 + tmp51);
  in[stride*4] = UNFIXH(tmp23 - tmp51);
  in[stride*5] = UNFIXH(tmp22 - tmp52);
  in[stride*6] = UNFIXH(tmp21 - tmp53);
  in[stride*7] = UNFIXH(tmp20 - tmp50);
}


/*
 * Perform the inverse DCT on one block of coefficients.
 *
 * Note that this code is specialized to the case DCTSIZE = 8.
 */

GLOBAL void
j_rev_dct (DCTBLOCK data)
{
  int i;
  
  for (i = 0; i < DCTSIZE; i++)
    fast_idct_8(data+i*DCTSIZE, 1);
  
  for (i = 0; i < DCTSIZE; i++)
    fast_idct_8(data+i, DCTSIZE);
}
