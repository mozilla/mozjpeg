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

/* CHROMA UPSAMPLING */

#include "jsimd_altivec.h"


void
jsimd_h2v1_fancy_upsample_altivec (int max_v_samp_factor,
                                   JDIMENSION downsampled_width,
                                   JSAMPARRAY input_data,
                                   JSAMPARRAY *output_data_ptr)
{
  JSAMPARRAY output_data = *output_data_ptr;
  JSAMPROW inptr, outptr;
  int inrow, col;

  __vector unsigned char block, last, next, lastblock, nextblock = {0}, out;
  __vector short blocke, blocko, blockl, blockh, lastl, lasth, nextl, nexth,
    outle, outhe, outlo, outho;

  /* Constants */
  __vector unsigned char pb_three = { __16X(3) }, pb_zero = { __16X(0) },
    last_index_col0 = {0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14},
    last_index = {15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30},
    next_index = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16},
    next_index_lastcol = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,15},
    merge_pack_index = {1,17,3,19,5,21,7,23,9,25,11,27,13,29,15,31};
  __vector short pw_one = { __8X(1) }, pw_two = { __8X(2) };

  for (inrow = 0; inrow < max_v_samp_factor; inrow++) {
    inptr = input_data[inrow];
    outptr = output_data[inrow];

    block = vec_ld(0, inptr);
    last = vec_perm(block, block, last_index_col0);
    lastblock = block;

    for (col = 0; col < downsampled_width;
         col += 16, inptr += 16, outptr += 32) {

      if (col > 0) {
        last = vec_perm(lastblock, block, last_index);
        lastblock = block;
      }

      if (downsampled_width - col <= 16)
        next = vec_perm(block, block, next_index_lastcol);
      else {
        nextblock = vec_ld(16, inptr);
        next = vec_perm(block, nextblock, next_index);
      }

      blocke = (__vector short)vec_mule(block, pb_three);
      blocko = (__vector short)vec_mulo(block, pb_three);
      blockl = vec_mergeh(blocke, blocko);
      blockh = vec_mergel(blocke, blocko);

      lastl = (__vector short)vec_mergeh(pb_zero, last);
      lasth = (__vector short)vec_mergel(pb_zero, last);
      lastl = vec_add(lastl, pw_one);
      lasth = vec_add(lasth, pw_one);

      nextl = (__vector short)vec_mergeh(pb_zero, next);
      nexth = (__vector short)vec_mergel(pb_zero, next);
      nextl = vec_add(nextl, pw_two);
      nexth = vec_add(nexth, pw_two);

      outle = vec_add(blockl, lastl);
      outhe = vec_add(blockh, lasth);
      outlo = vec_add(blockl, nextl);
      outho = vec_add(blockh, nexth);
      outle = vec_sr(outle, (__vector unsigned short)pw_two);
      outhe = vec_sr(outhe, (__vector unsigned short)pw_two);
      outlo = vec_sr(outlo, (__vector unsigned short)pw_two);
      outho = vec_sr(outho, (__vector unsigned short)pw_two);

      out = vec_perm((__vector unsigned char)outle,
                     (__vector unsigned char)outlo, merge_pack_index);
      vec_st(out, 0, outptr);
      out = vec_perm((__vector unsigned char)outhe,
                     (__vector unsigned char)outho, merge_pack_index);
      vec_st(out, 16, outptr);

      block = nextblock;
    }
  }
}


void
jsimd_h2v2_fancy_upsample_altivec (int max_v_samp_factor,
                                   JDIMENSION downsampled_width,
                                   JSAMPARRAY input_data,
                                   JSAMPARRAY *output_data_ptr)
{
  JSAMPARRAY output_data = *output_data_ptr;
  JSAMPROW inptr_1, inptr0, inptr1, outptr0, outptr1;
  int inrow, outrow, col;

  __vector unsigned char block_1, block0, block1, out;
  __vector short block_1l, block_1h, block0l, block0h, block1l, block1h,
    lastcolsum_1h, lastcolsum1h,
    p_lastcolsum_1l, p_lastcolsum_1h, p_lastcolsum1l, p_lastcolsum1h,
    thiscolsum_1l, thiscolsum_1h, thiscolsum1l, thiscolsum1h,
    nextcolsum_1l = {0}, nextcolsum_1h = {0},
    nextcolsum1l = {0}, nextcolsum1h = {0},
    p_nextcolsum_1l, p_nextcolsum_1h, p_nextcolsum1l, p_nextcolsum1h,
    tmpl, tmph, outle, outhe, outlo, outho;

  /* Constants */
  __vector unsigned char pb_zero = { __16X(0) },
    last_index_col0 = {0,1,0,1,2,3,4,5,6,7,8,9,10,11,12,13},
    last_index={14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29},
    next_index = {2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17},
    next_index_lastcol = {2,3,4,5,6,7,8,9,10,11,12,13,14,15,14,15},
    merge_pack_index = {1,17,3,19,5,21,7,23,9,25,11,27,13,29,15,31};
  __vector short pw_zero = { __8X(0) }, pw_three = { __8X(3) },
    pw_seven = { __8X(7) }, pw_eight = { __8X(8) };
  __vector unsigned short pw_four = { __8X(4) };

  for (inrow = 0, outrow = 0; outrow < max_v_samp_factor; inrow++) {

    inptr_1 = input_data[inrow - 1];
    inptr0 = input_data[inrow];
    inptr1 = input_data[inrow + 1];
    outptr0 = output_data[outrow++];
    outptr1 = output_data[outrow++];

    block0 = vec_ld(0, inptr0);
    block0l = (__vector short)vec_mergeh(pb_zero, block0);
    block0h = (__vector short)vec_mergel(pb_zero, block0);
    block0l = vec_mladd(block0l, pw_three, pw_zero);
    block0h = vec_mladd(block0h, pw_three, pw_zero);

    block_1 = vec_ld(0, inptr_1);
    block_1l = (__vector short)vec_mergeh(pb_zero, block_1);
    block_1h = (__vector short)vec_mergel(pb_zero, block_1);
    thiscolsum_1l = vec_add(block0l, block_1l);
    thiscolsum_1h = vec_add(block0h, block_1h);
    lastcolsum_1h = thiscolsum_1h;
    p_lastcolsum_1l = vec_perm(thiscolsum_1l, thiscolsum_1l, last_index_col0);
    p_lastcolsum_1h = vec_perm(thiscolsum_1l, thiscolsum_1h, last_index);

    block1 = vec_ld(0, inptr1);
    block1l = (__vector short)vec_mergeh(pb_zero, block1);
    block1h = (__vector short)vec_mergel(pb_zero, block1);
    thiscolsum1l = vec_add(block0l, block1l);
    thiscolsum1h = vec_add(block0h, block1h);
    lastcolsum1h = thiscolsum1h;
    p_lastcolsum1l = vec_perm(thiscolsum1l, thiscolsum1l, last_index_col0);
    p_lastcolsum1h = vec_perm(thiscolsum1l, thiscolsum1h, last_index);

    for (col = 0; col < downsampled_width;
         col += 16, inptr_1 += 16, inptr0 += 16, inptr1 += 16,
         outptr0 += 32, outptr1 += 32) {

      if (col > 0) {
        p_lastcolsum_1l = vec_perm(lastcolsum_1h, thiscolsum_1l, last_index);
        p_lastcolsum_1h = vec_perm(thiscolsum_1l, thiscolsum_1h, last_index);
        p_lastcolsum1l = vec_perm(lastcolsum1h, thiscolsum1l, last_index);
        p_lastcolsum1h = vec_perm(thiscolsum1l, thiscolsum1h, last_index);
        lastcolsum_1h = thiscolsum_1h;  lastcolsum1h = thiscolsum1h;
      }

      if (downsampled_width - col <= 16) {
        p_nextcolsum_1l = vec_perm(thiscolsum_1l, thiscolsum_1h, next_index);
        p_nextcolsum_1h = vec_perm(thiscolsum_1h, thiscolsum_1h,
                                   next_index_lastcol);
        p_nextcolsum1l = vec_perm(thiscolsum1l, thiscolsum1h, next_index);
        p_nextcolsum1h = vec_perm(thiscolsum1h, thiscolsum1h,
                                  next_index_lastcol);
      } else {
        block0 = vec_ld(16, inptr0);
        block0l = (__vector short)vec_mergeh(pb_zero, block0);
        block0h = (__vector short)vec_mergel(pb_zero, block0);
        block0l = vec_mladd(block0l, pw_three, pw_zero);
        block0h = vec_mladd(block0h, pw_three, pw_zero);

        block_1 = vec_ld(16, inptr_1);
        block_1l = (__vector short)vec_mergeh(pb_zero, block_1);
        block_1h = (__vector short)vec_mergel(pb_zero, block_1);
        nextcolsum_1l = vec_add(block0l, block_1l);
        nextcolsum_1h = vec_add(block0h, block_1h);
        p_nextcolsum_1l = vec_perm(thiscolsum_1l, thiscolsum_1h, next_index);
        p_nextcolsum_1h = vec_perm(thiscolsum_1h, nextcolsum_1l, next_index);

        block1 = vec_ld(16, inptr1);
        block1l = (__vector short)vec_mergeh(pb_zero, block1);
        block1h = (__vector short)vec_mergel(pb_zero, block1);
        nextcolsum1l = vec_add(block0l, block1l);
        nextcolsum1h = vec_add(block0h, block1h);
        p_nextcolsum1l = vec_perm(thiscolsum1l, thiscolsum1h, next_index);
        p_nextcolsum1h = vec_perm(thiscolsum1h, nextcolsum1l, next_index);
      }

      /* Process the upper row */

      tmpl = vec_mladd(thiscolsum_1l, pw_three, pw_zero);
      tmph = vec_mladd(thiscolsum_1h, pw_three, pw_zero);
      outle = vec_add(tmpl, p_lastcolsum_1l);
      outhe = vec_add(tmph, p_lastcolsum_1h);
      outle = vec_add(outle, pw_eight);
      outhe = vec_add(outhe, pw_eight);
      outle = vec_sr(outle, pw_four);
      outhe = vec_sr(outhe, pw_four);

      outlo = vec_add(tmpl, p_nextcolsum_1l);
      outho = vec_add(tmph, p_nextcolsum_1h);
      outlo = vec_add(outlo, pw_seven);
      outho = vec_add(outho, pw_seven);
      outlo = vec_sr(outlo, pw_four);
      outho = vec_sr(outho, pw_four);

      out = vec_perm((__vector unsigned char)outle,
                     (__vector unsigned char)outlo, merge_pack_index);
      vec_st(out, 0, outptr0);

      out = vec_perm((__vector unsigned char)outhe,
                     (__vector unsigned char)outho, merge_pack_index);
      vec_st(out, 16, outptr0);

      /* Process the lower row */

      tmpl = vec_mladd(thiscolsum1l, pw_three, pw_zero);
      tmph = vec_mladd(thiscolsum1h, pw_three, pw_zero);
      outle = vec_add(tmpl, p_lastcolsum1l);
      outhe = vec_add(tmph, p_lastcolsum1h);
      outle = vec_add(outle, pw_eight);
      outhe = vec_add(outhe, pw_eight);
      outle = vec_sr(outle, pw_four);
      outhe = vec_sr(outhe, pw_four);

      outlo = vec_add(tmpl, p_nextcolsum1l);
      outho = vec_add(tmph, p_nextcolsum1h);
      outlo = vec_add(outlo, pw_seven);
      outho = vec_add(outho, pw_seven);
      outlo = vec_sr(outlo, pw_four);
      outho = vec_sr(outho, pw_four);

      out = vec_perm((__vector unsigned char)outle,
                     (__vector unsigned char)outlo, merge_pack_index);
      vec_st(out, 0, outptr1);
      out = vec_perm((__vector unsigned char)outhe,
                     (__vector unsigned char)outho, merge_pack_index);
      vec_st(out, 16, outptr1);

      thiscolsum_1l = nextcolsum_1l;  thiscolsum_1h = nextcolsum_1h;
      thiscolsum1l = nextcolsum1l;  thiscolsum1h = nextcolsum1h;
    }
  }
}
