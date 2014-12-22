/*
 * AltiVec optimizations for libjpeg-turbo
 *
 * Copyright (C) 2014, D. R. Commander.
 * Copyright (C) 2014, Jay Foad.
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

/* This file is included by jccolor-altivec.c */


void jsimd_rgb_ycc_convert_altivec (JDIMENSION img_width, JSAMPARRAY input_buf,
                                    JSAMPIMAGE output_buf,
                                    JDIMENSION output_row, int num_rows)
{
  JSAMPROW inptr;
  JSAMPROW outptr0, outptr1, outptr2;
  int pitch;
  __vector unsigned char rgb0, rgb1 = {0}, rgb2 = {0}, rgb3 = {0}, rgbg0,
    rgbg1, rgbg2, rgbg3, y, cb, cr;
#if RGB_PIXELSIZE == 4
  __vector unsigned char rgb4;
#endif
  __vector short rg0, rg1, rg2, rg3, bg0, bg1, bg2, bg3;
  __vector unsigned short y01, y23, cr01, cr23, cb01, cb23;
  __vector int y0, y1, y2, y3, cr0, cr1, cr2, cr3, cb0, cb1, cb2, cb3;

  /* Constants */
  __vector short pw_f0299_f0337 = { __4X2(F_0_299, F_0_337) },
    pw_f0114_f0250 = { __4X2(F_0_114, F_0_250) },
    pw_mf016_mf033 = { __4X2(-F_0_168, -F_0_331) },
    pw_mf008_mf041 = { __4X2(-F_0_081, -F_0_418) };
  __vector unsigned short pw_f050_f000 = { __4X2(F_0_500, 0) };
  __vector int pd_onehalf = { __4X(ONE_HALF) },
    pd_onehalfm1_cj = { __4X(ONE_HALF - 1 + (CENTERJSAMPLE << SCALEBITS)) };
  __vector unsigned char zero = { __16X(0) },
    shift_pack_index =
      { 0, 1, 4, 5, 8, 9, 12, 13, 16, 17, 20, 21, 24, 25, 28, 29};

  while (--num_rows >= 0) {
    inptr = *input_buf++;
    outptr0 = output_buf[0][output_row];
    outptr1 = output_buf[1][output_row];
    outptr2 = output_buf[2][output_row];
    output_row++;

    for (pitch = img_width * RGB_PIXELSIZE; pitch > 0;
         pitch -= RGB_PIXELSIZE * 16, inptr += RGB_PIXELSIZE * 16,
         outptr0 += 16, outptr1 += 16, outptr2 += 16) {

#if RGB_PIXELSIZE == 3
      /* Load 16 pixels == 48 bytes */
      if ((size_t)inptr & 15) {
        __vector unsigned char unaligned_shift_index;
        rgb0 = vec_ld(0, inptr);
        if (pitch > 16)
          rgb1 = vec_ld(16, inptr);
        else
          rgb1 = vec_ld(-1, inptr + pitch);
        if (pitch > 32)
          rgb2 = vec_ld(32, inptr);
        else
          rgb2 = vec_ld(-1, inptr + pitch);
        if (pitch > 48)
          rgb3 = vec_ld(48, inptr);
        else
          rgb3 = vec_ld(-1, inptr + pitch);
        unaligned_shift_index = vec_lvsl(0, inptr);
        rgb0 = vec_perm(rgb0, rgb1, unaligned_shift_index);
        rgb1 = vec_perm(rgb1, rgb2, unaligned_shift_index);
        rgb2 = vec_perm(rgb2, rgb3, unaligned_shift_index);
      } else {
        rgb0 = vec_ld(0, inptr);
        if (pitch > 16)
          rgb1 = vec_ld(16, inptr);
        if (pitch > 32)
          rgb2 = vec_ld(32, inptr);
      }

      /* rgb0 = R0 G0 B0 R1 G1 B1 R2 G2 B2 R3 G3 B3 R4 G4 B4 R5
       * rgb1 = G5 B5 R6 G6 B6 R7 G7 B7 R8 G8 B8 R9 G9 B9 Ra Ga
       * rgb2 = Ba Rb Gb Bb Rc Gc Bc Rd Gd Bd Re Ge Be Rf Gf Bf
       *
       * rgbg0 = R0 G0 R1 G1 R2 G2 R3 G3 B0 G0 B1 G1 B2 G2 B3 G3
       * rgbg1 = R4 G4 R5 G5 R6 G6 R7 G7 B4 G4 B5 G5 B6 G6 B7 G7
       * rgbg2 = R8 G8 R9 G9 Ra Ga Rb Gb B8 G8 B9 G9 Ba Ga Bb Gb
       * rgbg3 = Rc Gc Rd Gd Re Ge Rf Gf Bc Gc Bd Gd Be Ge Bf Gf
       */
      rgbg0 = vec_perm(rgb0, rgb0, (__vector unsigned char)RGBG_INDEX0);
      rgbg1 = vec_perm(rgb0, rgb1, (__vector unsigned char)RGBG_INDEX1);
      rgbg2 = vec_perm(rgb1, rgb2, (__vector unsigned char)RGBG_INDEX2);
      rgbg3 = vec_perm(rgb2, rgb2, (__vector unsigned char)RGBG_INDEX3);
#else
      /* Load 16 pixels == 64 bytes */
      if ((size_t)inptr & 15) {
        __vector unsigned char unaligned_shift_index;
        rgb0 = vec_ld(0, inptr);
        if (pitch > 16)
          rgb1 = vec_ld(16, inptr);
        else
          rgb1 = vec_ld(-1, inptr + pitch);
        if (pitch > 32)
          rgb2 = vec_ld(32, inptr);
        else
          rgb2 = vec_ld(-1, inptr + pitch);
        if (pitch > 48)
          rgb3 = vec_ld(48, inptr);
        else
          rgb3 = vec_ld(-1, inptr + pitch);
        if (pitch > 64)
          rgb4 = vec_ld(64, inptr);
        else
          rgb4 = vec_ld(-1, inptr + pitch);
        unaligned_shift_index = vec_lvsl(0, inptr);
        rgb0 = vec_perm(rgb0, rgb1, unaligned_shift_index);
        rgb1 = vec_perm(rgb1, rgb2, unaligned_shift_index);
        rgb2 = vec_perm(rgb2, rgb3, unaligned_shift_index);
        rgb3 = vec_perm(rgb3, rgb4, unaligned_shift_index);
      } else {
        rgb0 = vec_ld(0, inptr);
        if (pitch > 16)
          rgb1 = vec_ld(16, inptr);
        if (pitch > 32)
          rgb2 = vec_ld(32, inptr);
        if (pitch > 48)
          rgb3 = vec_ld(48, inptr);
      }

      /* rgb0 = R0 G0 B0 X0 R1 G1 B1 X1 R2 G2 B2 X2 R3 G3 B3 X3
       * rgb0 = R4 G4 B4 X4 R5 G5 B5 X5 R6 G6 B6 X6 R7 G7 B7 X7
       * rgb0 = R8 G8 B8 X8 R9 G9 B9 X9 Ra Ga Ba Xa Rb Gb Bb Xb
       * rgb0 = Rc Gc Bc Xc Rd Gd Bd Xd Re Ge Be Xe Rf Gf Bf Xf
       *
       * rgbg0 = R0 G0 R1 G1 R2 G2 R3 G3 B0 G0 B1 G1 B2 G2 B3 G3
       * rgbg1 = R4 G4 R5 G5 R6 G6 R7 G7 B4 G4 B5 G5 B6 G6 B7 G7
       * rgbg2 = R8 G8 R9 G9 Ra Ga Rb Gb B8 G8 B9 G9 Ba Ga Bb Gb
       * rgbg3 = Rc Gc Rd Gd Re Ge Rf Gf Bc Gc Bd Gd Be Ge Bf Gf
       */
      rgbg0 = vec_perm(rgb0, rgb0, (__vector unsigned char)RGBG_INDEX);
      rgbg1 = vec_perm(rgb1, rgb1, (__vector unsigned char)RGBG_INDEX);
      rgbg2 = vec_perm(rgb2, rgb2, (__vector unsigned char)RGBG_INDEX);
      rgbg3 = vec_perm(rgb3, rgb3, (__vector unsigned char)RGBG_INDEX);
#endif

      /* rg0 = R0 G0 R1 G1 R2 G2 R3 G3
       * bg0 = B0 G0 B1 G1 B2 G2 B3 G3
       * ...
       *
       * NOTE: We have to use vec_merge*() here because vec_unpack*() doesn't
       * support unsigned vectors.
       */
      rg0 = (__vector signed short)vec_mergeh(zero, rgbg0);
      bg0 = (__vector signed short)vec_mergel(zero, rgbg0);
      rg1 = (__vector signed short)vec_mergeh(zero, rgbg1);
      bg1 = (__vector signed short)vec_mergel(zero, rgbg1);
      rg2 = (__vector signed short)vec_mergeh(zero, rgbg2);
      bg2 = (__vector signed short)vec_mergel(zero, rgbg2);
      rg3 = (__vector signed short)vec_mergeh(zero, rgbg3);
      bg3 = (__vector signed short)vec_mergel(zero, rgbg3);

      /* (Original)
       * Y  =  0.29900 * R + 0.58700 * G + 0.11400 * B
       * Cb = -0.16874 * R - 0.33126 * G + 0.50000 * B + CENTERJSAMPLE
       * Cr =  0.50000 * R - 0.41869 * G - 0.08131 * B + CENTERJSAMPLE
       *
       * (This implementation)
       * Y  =  0.29900 * R + 0.33700 * G + 0.11400 * B + 0.25000 * G
       * Cb = -0.16874 * R - 0.33126 * G + 0.50000 * B + CENTERJSAMPLE
       * Cr =  0.50000 * R - 0.41869 * G - 0.08131 * B + CENTERJSAMPLE
       */

      /* Calculate Y values */

      y0 = vec_msums(rg0, pw_f0299_f0337, pd_onehalf);
      y1 = vec_msums(rg1, pw_f0299_f0337, pd_onehalf);
      y2 = vec_msums(rg2, pw_f0299_f0337, pd_onehalf);
      y3 = vec_msums(rg3, pw_f0299_f0337, pd_onehalf);
      y0 = vec_msums(bg0, pw_f0114_f0250, y0);
      y1 = vec_msums(bg1, pw_f0114_f0250, y1);
      y2 = vec_msums(bg2, pw_f0114_f0250, y2);
      y3 = vec_msums(bg3, pw_f0114_f0250, y3);
      /* Clever way to avoid 4 shifts + 2 packs.  This packs the high word from
       * each dword into a new 16-bit vector, which is the equivalent of
       * descaling the 32-bit results (right-shifting by 16 bits) and then
       * packing them.
       */
      y01 = vec_perm((__vector unsigned short)y0, (__vector unsigned short)y1,
                     shift_pack_index);
      y23 = vec_perm((__vector unsigned short)y2, (__vector unsigned short)y3,
                     shift_pack_index);
      y = vec_pack(y01, y23);
      vec_st(y, 0, outptr0);

      /* Calculate Cb values */
      cb0 = vec_msums(rg0, pw_mf016_mf033, pd_onehalfm1_cj);
      cb1 = vec_msums(rg1, pw_mf016_mf033, pd_onehalfm1_cj);
      cb2 = vec_msums(rg2, pw_mf016_mf033, pd_onehalfm1_cj);
      cb3 = vec_msums(rg3, pw_mf016_mf033, pd_onehalfm1_cj);
      cb0 = (__vector int)vec_msum((__vector unsigned short)bg0, pw_f050_f000,
                                   (__vector unsigned int)cb0);
      cb1 = (__vector int)vec_msum((__vector unsigned short)bg1, pw_f050_f000,
                                   (__vector unsigned int)cb1);
      cb2 = (__vector int)vec_msum((__vector unsigned short)bg2, pw_f050_f000,
                                   (__vector unsigned int)cb2);
      cb3 = (__vector int)vec_msum((__vector unsigned short)bg3, pw_f050_f000,
                                   (__vector unsigned int)cb3);
      cb01 = vec_perm((__vector unsigned short)cb0,
                      (__vector unsigned short)cb1, shift_pack_index);
      cb23 = vec_perm((__vector unsigned short)cb2,
                      (__vector unsigned short)cb3, shift_pack_index);
      cb = vec_pack(cb01, cb23);
      vec_st(cb, 0, outptr1);

      /* Calculate Cr values */
      cr0 = vec_msums(bg0, pw_mf008_mf041, pd_onehalfm1_cj);
      cr1 = vec_msums(bg1, pw_mf008_mf041, pd_onehalfm1_cj);
      cr2 = vec_msums(bg2, pw_mf008_mf041, pd_onehalfm1_cj);
      cr3 = vec_msums(bg3, pw_mf008_mf041, pd_onehalfm1_cj);
      cr0 = (__vector int)vec_msum((__vector unsigned short)rg0, pw_f050_f000,
                                   (__vector unsigned int)cr0);
      cr1 = (__vector int)vec_msum((__vector unsigned short)rg1, pw_f050_f000,
                                   (__vector unsigned int)cr1);
      cr2 = (__vector int)vec_msum((__vector unsigned short)rg2, pw_f050_f000,
                                   (__vector unsigned int)cr2);
      cr3 = (__vector int)vec_msum((__vector unsigned short)rg3, pw_f050_f000,
                                   (__vector unsigned int)cr3);
      cr01 = vec_perm((__vector unsigned short)cr0,
                      (__vector unsigned short)cr1, shift_pack_index);
      cr23 = vec_perm((__vector unsigned short)cr2,
                      (__vector unsigned short)cr3, shift_pack_index);
      cr = vec_pack(cr01, cr23);
      vec_st(cr, 0, outptr2);
    }
  }
}
