/*
 * AltiVec optimizations for libjpeg-turbo
 *
 * Copyright (C) 2015, D. R. Commander.
 * All rights reserved.
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

/* CHROMA DOWNSAMPLING */

#include "jsimd_altivec.h"
#include "jcsample.h"


void
jsimd_h2v1_downsample_altivec (JDIMENSION image_width, int max_v_samp_factor,
                               JDIMENSION v_samp_factor,
                               JDIMENSION width_blocks,
                               JSAMPARRAY input_data, JSAMPARRAY output_data)
{
  int outrow, outcol;
  JDIMENSION output_cols = width_blocks * DCTSIZE;
  JSAMPROW inptr, outptr;
  __vector unsigned char tmpa, tmpb, out;
  __vector unsigned short tmpae, tmpao, tmpbe, tmpbo, outl, outh;

  /* Constants */
  __vector unsigned short bias = { __4X2(0, 1) },
    one = { __8X(1) };
  __vector unsigned char even_odd_index =
    { 0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15 },
    zero = { __16X(0) };

  expand_right_edge(input_data, max_v_samp_factor, image_width,
                    output_cols * 2);

  for (outrow = 0; outrow < v_samp_factor; outrow++) {
    outptr = output_data[outrow];
    inptr = input_data[outrow];

    for (outcol = output_cols; outcol > 0;
         outcol -= 16, inptr += 32, outptr += 16) {

      tmpa = vec_ld(0, inptr);
      tmpa = vec_perm(tmpa, tmpa, even_odd_index);
      tmpae = (__vector unsigned short)vec_mergeh(zero, tmpa);
      tmpao = (__vector unsigned short)vec_mergel(zero, tmpa);
      outl = vec_add(tmpae, tmpao);
      outl = vec_add(outl, bias);
      outl = vec_sr(outl, one);

      if (outcol > 16) {
        tmpb = vec_ld(16, inptr);
        tmpb = vec_perm(tmpb, tmpb, even_odd_index);
        tmpbe = (__vector unsigned short)vec_mergeh(zero, tmpb);
        tmpbo = (__vector unsigned short)vec_mergel(zero, tmpb);
        outh = vec_add(tmpbe, tmpbo);
        outh = vec_add(outh, bias);
        outh = vec_sr(outh, one);
      } else
        outh = vec_splat_u16(0);

      out = vec_pack(outl, outh);
      vec_st(out, 0, outptr);
    }
  }
}


void
jsimd_h2v2_downsample_altivec (JDIMENSION image_width, int max_v_samp_factor,
                               JDIMENSION v_samp_factor,
                               JDIMENSION width_blocks,
                               JSAMPARRAY input_data, JSAMPARRAY output_data)
{
  int inrow, outrow, outcol;
  JDIMENSION output_cols = width_blocks * DCTSIZE;
  JSAMPROW inptr0, inptr1, outptr;
  __vector unsigned char tmp0a, tmp0b, tmp1a, tmp1b, out;
  __vector unsigned short tmp0ae, tmp0ao, tmp0be, tmp0bo, tmp1ae, tmp1ao,
    tmp1be, tmp1bo, out0l, out0h, out1l, out1h, outl, outh;

  /* Constants */
  __vector unsigned short bias = { __4X2(1, 2) },
    two = { __8X(2) };
  __vector unsigned char even_odd_index =
    { 0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15 },
    zero = { __16X(0) };

  expand_right_edge(input_data, max_v_samp_factor, image_width,
                    output_cols * 2);

  for (inrow = 0, outrow = 0; outrow < v_samp_factor;
       inrow += 2, outrow++) {

    outptr = output_data[outrow];
    inptr0 = input_data[inrow];
    inptr1 = input_data[inrow + 1];

    for (outcol = output_cols; outcol > 0;
         outcol -= 16, inptr0 += 32, inptr1 += 32, outptr += 16) {

      tmp0a = vec_ld(0, inptr0);
      tmp0a = vec_perm(tmp0a, tmp0a, even_odd_index);
      tmp0ae = (__vector unsigned short)vec_mergeh(zero, tmp0a);
      tmp0ao = (__vector unsigned short)vec_mergel(zero, tmp0a);
      out0l = vec_add(tmp0ae, tmp0ao);

      tmp1a = vec_ld(0, inptr1);
      tmp1a = vec_perm(tmp1a, tmp1a, even_odd_index);
      tmp1ae = (__vector unsigned short)vec_mergeh(zero, tmp1a);
      tmp1ao = (__vector unsigned short)vec_mergel(zero, tmp1a);
      out1l = vec_add(tmp1ae, tmp1ao);

      outl = vec_add(out0l, out1l);
      outl = vec_add(outl, bias);
      outl = vec_sr(outl, two);

      if (outcol > 16) {
        tmp0b = vec_ld(16, inptr0);
        tmp0b = vec_perm(tmp0b, tmp0b, even_odd_index);
        tmp0be = (__vector unsigned short)vec_mergeh(zero, tmp0b);
        tmp0bo = (__vector unsigned short)vec_mergel(zero, tmp0b);
        out0h = vec_add(tmp0be, tmp0bo);

        tmp1b = vec_ld(16, inptr1);
        tmp1b = vec_perm(tmp1b, tmp1b, even_odd_index);
        tmp1be = (__vector unsigned short)vec_mergeh(zero, tmp1b);
        tmp1bo = (__vector unsigned short)vec_mergel(zero, tmp1b);
        out1h = vec_add(tmp1be, tmp1bo);

        outh = vec_add(out0h, out1h);
        outh = vec_add(outh, bias);
        outh = vec_sr(outh, two);
      } else
        outh = vec_splat_u16(0);

      out = vec_pack(outl, outh);
      vec_st(out, 0, outptr);
    }
  }
}
