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
fast_dct_8 (DCTELEM *in, int stride)
{
  /* tmp1x are new values of tmpx -- flashy register colourers
   * should be able to do this lot very well
   */
  INT32 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  INT32 tmp10, tmp11, tmp12, tmp13, tmp14, tmp15, tmp16, tmp17;
  INT32 tmp25, tmp26;
  INT32 in0, in1, in2, in3, in4, in5, in6, in7;
  
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
  
  tmp10 = tmp3 + tmp0 ;
  tmp11 = tmp2 + tmp1 ;
  tmp12 = tmp1 - tmp2 ;
  tmp13 = tmp0 - tmp3 ;
  
  /* Now using tmp10, tmp11, tmp12, tmp13 */
  
  in[       0] = UNFIXH((tmp10 + tmp11) * SIN_1_4);
  in[stride*4] = UNFIXH((tmp10 - tmp11) * COS_1_4);
  
  in[stride*2] = UNFIXH(tmp13*COS_1_8 + tmp12*SIN_1_8);
  in[stride*6] = UNFIXH(tmp13*SIN_1_8 - tmp12*COS_1_8);
  
  tmp16 = UNFIXO((tmp6 + tmp5) * SIN_1_4);
  tmp15 = UNFIXO((tmp6 - tmp5) * COS_1_4);
  
  /* Now using tmp10, tmp11, tmp13, tmp14, tmp15, tmp16 */
  
  tmp14 = OVERSH(tmp4) + tmp15;
  tmp25 = OVERSH(tmp4) - tmp15;
  tmp26 = OVERSH(tmp7) - tmp16;
  tmp17 = OVERSH(tmp7) + tmp16;
  
  /* These are now overscaled by OVERSCALE */
  
  /* tmp10, tmp11, tmp12, tmp13, tmp14, tmp25, tmp26, tmp17 */
  
  in[stride  ] = UNFIXH(tmp17*OCOS_1_16 + tmp14*OSIN_1_16);
  in[stride*7] = UNFIXH(tmp17*OCOS_7_16 - tmp14*OSIN_7_16);
  in[stride*5] = UNFIXH(tmp26*OCOS_5_16 + tmp25*OSIN_5_16);
  in[stride*3] = UNFIXH(tmp26*OCOS_3_16 - tmp25*OSIN_3_16);
}


/*
 * Perform the forward DCT on one block of samples.
 *
 * Note that this code is specialized to the case DCTSIZE = 8.
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
