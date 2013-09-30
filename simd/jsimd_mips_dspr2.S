/*
 * MIPS DSPr2 optimizations for libjpeg-turbo
 *
 * Copyright (C) 2013, MIPS Technologies, Inc., California.
 * All rights reserved.
 * Authors:  Teodora Novkovic (teodora.novkovic@imgtec.com)
 *           Darko Laus       (darko.laus@imgtec.com)
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

#include "jsimd_mips_dspr2_asm.h"

/*****************************************************************************/
/*
 * jsimd_extrgb_ycc_convert_mips_dspr2
 * jsimd_extbgr_ycc_convert_mips_dspr2
 * jsimd_extrgbx_ycc_convert_mips_dspr2
 * jsimd_extbgrx_ycc_convert_mips_dspr2
 * jsimd_extxbgr_ycc_convert_mips_dspr2
 * jsimd_extxrgb_ycc_convert_mips_dspr2
 *
 * Colorspace conversion RGB -> YCbCr
 */

.macro GENERATE_JSIMD_RGB_YCC_CONVERT_MIPS_DSPR2 colorid, pixel_size, r_offs, g_offs, b_offs

.macro DO_RGB_TO_YCC r,    \
                     g,    \
                     b,    \
                     inptr
    lbu     \r, \r_offs(\inptr)
    lbu     \g, \g_offs(\inptr)
    lbu     \b, \b_offs(\inptr)
    addiu   \inptr, \pixel_size
.endm

LEAF_MIPS_DSPR2(jsimd_\colorid\()_ycc_convert_mips_dspr2)
/*
 * a0     - cinfo->image_width
 * a1     - input_buf
 * a2     - output_buf
 * a3     - output_row
 * 16(sp) - num_rows
 */

    SAVE_REGS_ON_STACK 32, s0, s1, s2, s3, s4, s5, s6, s7

    lw      t7, 48(sp)        // t7 = num_rows
    li      s0, 0x4c8b        // FIX(0.29900)
    li      s1, 0x9646        // FIX(0.58700)
    li      s2, 0x1d2f        // FIX(0.11400)
    li      s3, 0xffffd4cd    // -FIX(0.16874)
    li      s4, 0xffffab33    // -FIX(0.33126)
    li      s5, 0x8000        // FIX(0.50000)
    li      s6, 0xffff94d1    // -FIX(0.41869)
    li      s7, 0xffffeb2f    // -FIX(0.08131)
    li      t8, 0x807fff      // CBCR_OFFSET + ONE_HALF-1

0:
    addiu   t7, -1            // --num_rows
    lw      t6, 0(a1)         // t6 = input_buf[0]
    lw      t0, 0(a2)
    lw      t1, 4(a2)
    lw      t2, 8(a2)
    sll     t3, a3, 2
    lwx     t0, t3(t0)        // t0 = output_buf[0][output_row]
    lwx     t1, t3(t1)        // t1 = output_buf[1][output_row]
    lwx     t2, t3(t2)        // t2 = output_buf[2][output_row]

    addu    t9, t2, a0        // t9 = end address
    addiu   a3, 1

1:
    DO_RGB_TO_YCC t3, t4, t5, t6

    mtlo    s5, $ac0
    mtlo    t8, $ac1
    mtlo    t8, $ac2
    maddu   $ac0, s2, t5
    maddu   $ac1, s5, t5
    maddu   $ac2, s5, t3
    maddu   $ac0, s0, t3
    maddu   $ac1, s3, t3
    maddu   $ac2, s6, t4
    maddu   $ac0, s1, t4
    maddu   $ac1, s4, t4
    maddu   $ac2, s7, t5
    extr.w  t3, $ac0, 16
    extr.w  t4, $ac1, 16
    extr.w  t5, $ac2, 16
    sb      t3, 0(t0)
    sb      t4, 0(t1)
    sb      t5, 0(t2)
    addiu   t0, 1
    addiu   t2, 1
    bne     t2, t9, 1b
     addiu  t1, 1
    bgtz    t7, 0b
     addiu  a1, 4

    RESTORE_REGS_FROM_STACK 32, s0, s1, s2, s3, s4, s5, s6, s7

    j ra
     nop
END(jsimd_\colorid\()_ycc_convert_mips_dspr2)

.purgem DO_RGB_TO_YCC

.endm

/*------------------------------------------id -- pix R  G  B */
GENERATE_JSIMD_RGB_YCC_CONVERT_MIPS_DSPR2 extrgb,  3, 0, 1, 2
GENERATE_JSIMD_RGB_YCC_CONVERT_MIPS_DSPR2 extbgr,  3, 2, 1, 0
GENERATE_JSIMD_RGB_YCC_CONVERT_MIPS_DSPR2 extrgbx, 4, 0, 1, 2
GENERATE_JSIMD_RGB_YCC_CONVERT_MIPS_DSPR2 extbgrx, 4, 2, 1, 0
GENERATE_JSIMD_RGB_YCC_CONVERT_MIPS_DSPR2 extxbgr, 4, 3, 2, 1
GENERATE_JSIMD_RGB_YCC_CONVERT_MIPS_DSPR2 extxrgb, 4, 1, 2, 3

/*****************************************************************************/
/*
 * jsimd_ycc_extrgb_convert_mips_dspr2
 * jsimd_ycc_extbgr_convert_mips_dspr2
 * jsimd_ycc_extrgbx_convert_mips_dspr2
 * jsimd_ycc_extbgrx_convert_mips_dspr2
 * jsimd_ycc_extxbgr_convert_mips_dspr2
 * jsimd_ycc_extxrgb_convert_mips_dspr2
 *
 * Colorspace conversion YCbCr -> RGB
 */

.macro GENERATE_JSIMD_YCC_RGB_CONVERT_MIPS_DSPR2 colorid, pixel_size, r_offs, g_offs, b_offs, a_offs

.macro STORE_YCC_TO_RGB  scratch0 \
                         scratch1 \
                         scratch2 \
                         outptr
    sb       \scratch0, \r_offs(\outptr)
    sb       \scratch1, \g_offs(\outptr)
    sb       \scratch2, \b_offs(\outptr)
.if (\pixel_size == 4)
    li       t0, 0xFF
    sb       t0, \a_offs(\outptr)
.endif
    addiu    \outptr, \pixel_size
.endm

LEAF_MIPS_DSPR2(jsimd_ycc_\colorid\()_convert_mips_dspr2)
/*
 * a0     - cinfo->image_width
 * a1     - input_buf
 * a2     - input_row
 * a3     - output_buf
 * 16(sp) - num_rows
 */

    SAVE_REGS_ON_STACK 32, s0, s1, s2, s3, s4, s5, s6, s7

    lw         s1, 48(sp)
    li         t3, 0x8000
    li         t4, 0x166e9     // FIX(1.40200)
    li         t5, 0x1c5a2     // FIX(1.77200)
    li         t6, 0xffff492e  // -FIX(0.71414)
    li         t7, 0xffffa7e6  // -FIX(0.34414)
    repl.ph    t8, 128

0:
    lw         s0, 0(a3)
    lw         t0, 0(a1)
    lw         t1, 4(a1)
    lw         t2, 8(a1)
    sll        s5, a2, 2
    addiu      s1, -1
    lwx        s2, s5(t0)
    lwx        s3, s5(t1)
    lwx        s4, s5(t2)
    addu       t9, s2, a0
    addiu      a2, 1

1:
    lbu        s7, 0(s4)       // cr
    lbu        s6, 0(s3)       // cb
    lbu        s5, 0(s2)       // y
    addiu      s2, 1
    addiu      s4, 1
    addiu      s7, -128
    addiu      s6, -128
    mul        t2, t7, s6
    mul        t0, t6, s7      // Crgtab[cr]
    sll        s7, 15
    mulq_rs.w  t1, t4, s7      // Crrtab[cr]
    sll        s6, 15
    addu       t2, t3          // Cbgtab[cb]
    addu       t2, t0

    mulq_rs.w  t0, t5, s6      // Cbbtab[cb]
    sra        t2, 16
    addu       t1, s5
    addu       t2, s5          // add y
    ins        t2, t1, 16, 16
    subu.ph    t2, t2, t8
    addu       t0, s5
    shll_s.ph  t2, t2, 8
    subu       t0, 128
    shra.ph    t2, t2, 8
    shll_s.w   t0, t0, 24
    addu.ph    t2, t2, t8      // clip & store
    sra        t0, t0, 24
    sra        t1, t2, 16
    addiu      t0, 128

    STORE_YCC_TO_RGB t1, t2, t0, s0

    bne        s2, t9, 1b
     addiu     s3, 1
    bgtz       s1, 0b
     addiu     a3, 4

    RESTORE_REGS_FROM_STACK 32, s0, s1, s2, s3, s4, s5, s6, s7

    j ra
     nop
END(jsimd_ycc_\colorid\()_convert_mips_dspr2)

.purgem STORE_YCC_TO_RGB

.endm

/*------------------------------------------id -- pix R  G  B  A */
GENERATE_JSIMD_YCC_RGB_CONVERT_MIPS_DSPR2 extrgb,  3, 0, 1, 2, 3
GENERATE_JSIMD_YCC_RGB_CONVERT_MIPS_DSPR2 extbgr,  3, 2, 1, 0, 3
GENERATE_JSIMD_YCC_RGB_CONVERT_MIPS_DSPR2 extrgbx, 4, 0, 1, 2, 3
GENERATE_JSIMD_YCC_RGB_CONVERT_MIPS_DSPR2 extbgrx, 4, 2, 1, 0, 3
GENERATE_JSIMD_YCC_RGB_CONVERT_MIPS_DSPR2 extxbgr, 4, 3, 2, 1, 0
GENERATE_JSIMD_YCC_RGB_CONVERT_MIPS_DSPR2 extxrgb, 4, 1, 2, 3, 0

/*****************************************************************************/
/*
 * jsimd_extrgb_gray_convert_mips_dspr2
 * jsimd_extbgr_gray_convert_mips_dspr2
 * jsimd_extrgbx_gray_convert_mips_dspr2
 * jsimd_extbgrx_gray_convert_mips_dspr2
 * jsimd_extxbgr_gray_convert_mips_dspr2
 * jsimd_extxrgb_gray_convert_mips_dspr2
 *
 * Colorspace conversion RGB -> GRAY
 */

.macro GENERATE_JSIMD_RGB_GRAY_CONVERT_MIPS_DSPR2 colorid, pixel_size, r_offs, g_offs, b_offs

.macro DO_RGB_TO_GRAY r,    \
                      g,    \
                      b,    \
                      inptr
    lbu     \r, \r_offs(\inptr)
    lbu     \g, \g_offs(\inptr)
    lbu     \b, \b_offs(\inptr)
    addiu   \inptr, \pixel_size
.endm

LEAF_MIPS_DSPR2(jsimd_\colorid\()_gray_convert_mips_dspr2)
/*
 * a0     - cinfo->image_width
 * a1     - input_buf
 * a2     - output_buf
 * a3     - output_row
 * 16(sp) - num_rows
 */

    SAVE_REGS_ON_STACK 32, s0, s1, s2, s3, s4, s5, s6, s7

    li      s0, 0x4c8b             // s0 = FIX(0.29900)
    li      s1, 0x9646             // s1 = FIX(0.58700)
    li      s2, 0x1d2f             // s2 = FIX(0.11400)
    li      s7, 0x8000             // s7 = FIX(0.50000)
    lw      s6, 48(sp)
    andi    t7, a0, 3

0:
    addiu   s6, -1                 // s6 = num_rows
    lw      t0, 0(a1)
    lw      t1, 0(a2)
    sll     t3, a3, 2
    lwx     t1, t3(t1)
    addiu   a3, 1
    addu    t9, t1, a0
    subu    t8, t9, t7
    beq     t1, t8, 2f
     nop

1:
    DO_RGB_TO_GRAY t3, t4, t5, t0
    DO_RGB_TO_GRAY s3, s4, s5, t0

    mtlo    s7, $ac0
    maddu   $ac0, s2, t5
    maddu   $ac0, s1, t4
    maddu   $ac0, s0, t3
    mtlo    s7, $ac1
    maddu   $ac1, s2, s5
    maddu   $ac1, s1, s4
    maddu   $ac1, s0, s3
    extr.w  t6, $ac0, 16

    DO_RGB_TO_GRAY t3, t4, t5, t0
    DO_RGB_TO_GRAY s3, s4, s5, t0

    mtlo    s7, $ac0
    maddu   $ac0, s2, t5
    maddu   $ac0, s1, t4
    extr.w  t2, $ac1, 16
    maddu   $ac0, s0, t3
    mtlo    s7, $ac1
    maddu   $ac1, s2, s5
    maddu   $ac1, s1, s4
    maddu   $ac1, s0, s3
    extr.w  t5, $ac0, 16
    sb      t6, 0(t1)
    sb      t2, 1(t1)
    extr.w  t3, $ac1, 16
    addiu   t1, 4
    sb      t5, -2(t1)
    sb      t3, -1(t1)
    bne     t1, t8, 1b
     nop

2:
    beqz    t7, 4f
     nop

3:
    DO_RGB_TO_GRAY t3, t4, t5, t0

    mtlo    s7, $ac0
    maddu   $ac0, s2, t5
    maddu   $ac0, s1, t4
    maddu   $ac0, s0, t3
    extr.w  t6, $ac0, 16
    sb      t6, 0(t1)
    addiu   t1, 1
    bne     t1, t9, 3b
     nop

4:
    bgtz    s6, 0b
     addiu  a1, 4

    RESTORE_REGS_FROM_STACK 32, s0, s1, s2, s3, s4, s5, s6, s7

    j ra
     nop
END(jsimd_\colorid\()_gray_convert_mips_dspr2)

.purgem DO_RGB_TO_GRAY

.endm

/*------------------------------------------id --  pix R  G  B */
GENERATE_JSIMD_RGB_GRAY_CONVERT_MIPS_DSPR2 extrgb,  3, 0, 1, 2
GENERATE_JSIMD_RGB_GRAY_CONVERT_MIPS_DSPR2 extbgr,  3, 2, 1, 0
GENERATE_JSIMD_RGB_GRAY_CONVERT_MIPS_DSPR2 extrgbx, 4, 0, 1, 2
GENERATE_JSIMD_RGB_GRAY_CONVERT_MIPS_DSPR2 extbgrx, 4, 2, 1, 0
GENERATE_JSIMD_RGB_GRAY_CONVERT_MIPS_DSPR2 extxbgr, 4, 3, 2, 1
GENERATE_JSIMD_RGB_GRAY_CONVERT_MIPS_DSPR2 extxrgb, 4, 1, 2, 3
/*****************************************************************************/
/*
 * jsimd_h2v2_fancy_upsample_mips_dspr2
 *
 * Fancy processing for the common case of 2:1 horizontal and 2:1 vertical.
 */
LEAF_MIPS_DSPR2(jsimd_h2v2_fancy_upsample_mips_dspr2)
/*
 * a0     - cinfo->max_v_samp_factor
 * a1     - downsampled_width
 * a2     - input_data
 * a3     - output_data_ptr
 */

    SAVE_REGS_ON_STACK 24, s0, s1, s2, s3, s4, s5

    li             s4, 0
    lw             s2, 0(a3)       // s2 = *output_data_ptr
0:
    li             t9, 2
    lw             s1, -4(a2)      // s1 = inptr1

1:
    lw             s0, 0(a2)       // s0 = inptr0
    lwx            s3, s4(s2)
    addiu          s5, a1, -2      // s5 = downsampled_width - 2
    srl            t4, s5, 1
    sll            t4, t4, 1
    lbu            t0, 0(s0)
    lbu            t1, 1(s0)
    lbu            t2, 0(s1)
    lbu            t3, 1(s1)
    addiu          s0, 2
    addiu          s1, 2
    addu           t8, s0, t4      // t8 = end address
    andi           s5, s5, 1       // s5 = residual
    sll            t4, t0, 1
    sll            t6, t1, 1
    addu           t0, t0, t4      // t0 = (*inptr0++) * 3
    addu           t1, t1, t6      // t1 = (*inptr0++) * 3
    addu           t7, t0, t2      // t7 = thiscolsum
    addu           t6, t1, t3      // t5 = nextcolsum
    sll            t0, t7, 2       // t0 = thiscolsum * 4
    subu           t1, t0, t7      // t1 = thiscolsum * 3
    shra_r.w       t0, t0, 4
    addiu          t1, 7
    addu           t1, t1, t6
    srl            t1, t1, 4
    sb             t0, 0(s3)
    sb             t1, 1(s3)
    addiu          s3, 2
2:
    lh             t0, 0(s0)       // t0 = A3|A2
    lh             t2, 0(s1)       // t2 = B3|B2
    addiu          s0, 2
    addiu          s1, 2
    preceu.ph.qbr  t0, t0          // t0 = 0|A3|0|A2
    preceu.ph.qbr  t2, t2          // t2 = 0|B3|0|B2
    shll.ph        t1, t0, 1
    sll            t3, t6, 1
    addu.ph        t0, t1, t0      // t0 = A3*3|A2*3
    addu           t3, t3, t6      // t3 = this * 3
    addu.ph        t0, t0, t2      // t0 = next2|next1
    addu           t1, t3, t7
    andi           t7, t0, 0xFFFF  // t7 = next1
    sll            t2, t7, 1
    addu           t2, t7, t2      // t2 = next1*3
    addu           t4, t2, t6
    srl            t6, t0, 16      // t6 = next2
    shra_r.w       t1, t1, 4       // t1 = (this*3 + last + 8) >> 4
    addu           t0, t3, t7
    addiu          t0, 7
    srl            t0, t0, 4       // t0 = (this*3 + next1 + 7) >> 4
    shra_r.w       t4, t4, 4       // t3 = (next1*3 + this + 8) >> 4
    addu           t2, t2, t6
    addiu          t2, 7
    srl            t2, t2, 4       // t2 = (next1*3 + next2 + 7) >> 4
    sb             t1, 0(s3)
    sb             t0, 1(s3)
    sb             t4, 2(s3)
    sb             t2, 3(s3)
    bne            t8, s0, 2b
     addiu         s3, 4
    beqz           s5, 4f
     addu          t8, s0, s5
3:
    lbu            t0, 0(s0)
    lbu            t2, 0(s1)
    addiu          s0, 1
    addiu          s1, 1
    sll            t3, t6, 1
    sll            t1, t0, 1
    addu           t1, t0, t1      // t1 = inptr0 * 3
    addu           t3, t3, t6      // t3 = thiscolsum * 3
    addu           t5, t1, t2
    addu           t1, t3, t7
    shra_r.w       t1, t1, 4
    addu           t0, t3, t5
    addiu          t0, 7
    srl            t0, t0, 4
    sb             t1, 0(s3)
    sb             t0, 1(s3)
    addiu          s3, 2
    move           t7, t6
    bne            t8, s0, 3b
     move          t6, t5
4:
    sll            t0, t6, 2       // t0 = thiscolsum * 4
    subu           t1, t0, t6      // t1 = thiscolsum * 3
    addu           t1, t1, t7
    addiu          s4, 4
    shra_r.w       t1, t1, 4
    addiu          t0, 7
    srl            t0, t0, 4
    sb             t1, 0(s3)
    sb             t0, 1(s3)
    addiu          t9, -1
    addiu          s3, 2
    bnez           t9, 1b
     lw            s1, 4(a2)
    srl            t0, s4, 2
    subu           t0, a0, t0
    bgtz           t0, 0b
     addiu         a2, 4

    RESTORE_REGS_FROM_STACK 24, s0, s1, s2, s3, s4, s5

    j ra
     nop
END(jsimd_h2v2_fancy_upsample_mips_dspr2)

/*****************************************************************************/
LEAF_MIPS_DSPR2(jsimd_h2v1_fancy_upsample_mips_dspr2)
/*
 * a0     - cinfo->max_v_samp_factor
 * a1     - downsampled_width
 * a2     - input_data
 * a3     - output_data_ptr
 */

    SAVE_REGS_ON_STACK 16, s0, s1, s2, s3

    .set at

    beqz           a0, 3f
     sll           t0, a0, 2
    lw             s1, 0(a3)
    addu           s0, s1, t0
    li             s3, 0x10001
0:
    addiu          t8, a1, -2
    srl            t9, t8, 2
    lw             t7, 0(a2)
    lw             s2, 0(s1)
    lbu            t0, 0(t7)
    lbu            t1, 1(t7)   // t1 = inptr[1]
    sll            t2, t0, 1
    addu           t2, t2, t0  // t2 = invalue*3
    addu           t2, t2, t1
    shra_r.w       t2, t2, 2
    sb             t0, 0(s2)
    sb             t2, 1(s2)
    beqz           t9, 11f
     addiu         s2, 2
1:
    ulw            t0, 0(t7)   // t0 = |P3|P2|P1|P0|
    ulw            t1, 1(t7)
    ulh            t2, 4(t7)   // t2 = |0|0|P5|P4|
    preceu.ph.qbl  t3, t0      // t3 = |0|P3|0|P2|
    preceu.ph.qbr  t0, t0      // t0 = |0|P1|0|P0|
    preceu.ph.qbr  t2, t2      // t2 = |0|P5|0|P4|
    preceu.ph.qbl  t4, t1      // t4 = |0|P4|0|P3|
    preceu.ph.qbr  t1, t1      // t1 = |0|P2|0|P1|
    shll.ph        t5, t4, 1
    shll.ph        t6, t1, 1
    addu.ph        t5, t5, t4  // t5 = |P4*3|P3*3|
    addu.ph        t6, t6, t1  // t6 = |P2*3|P1*3|
    addu.ph        t4, t3, s3
    addu.ph        t0, t0, s3
    addu.ph        t4, t4, t5
    addu.ph        t0, t0, t6
    shrl.ph        t4, t4, 2   // t4 = |0|P3|0|P2|
    shrl.ph        t0, t0, 2   // t0 = |0|P1|0|P0|
    addu.ph        t2, t2, t5
    addu.ph        t3, t3, t6
    shra_r.ph      t2, t2, 2   // t2 = |0|P5|0|P4|
    shra_r.ph      t3, t3, 2   // t3 = |0|P3|0|P2|
    shll.ph        t2, t2, 8
    shll.ph        t3, t3, 8
    or             t2, t4, t2
    or             t3, t3, t0
    addiu          t9, -1
    usw            t3, 0(s2)
    usw            t2, 4(s2)
    addiu          s2, 8
    bgtz           t9, 1b
     addiu         t7, 4
11:
    andi           t8, 3
    beqz           t8, 3f
     addiu         t7, 1
2:
    lbu            t0, 0(t7)
    addiu          t7, 1
    sll            t1, t0, 1
    addu           t2, t0, t1  // t2 = invalue
    lbu            t3, -2(t7)
    lbu            t4, 0(t7)
    addiu          t3, 1
    addiu          t4, 2
    addu           t3, t3, t2
    addu           t4, t4, t2
    srl            t3, 2
    srl            t4, 2
    sb             t3, 0(s2)
    sb             t4, 1(s2)
    addiu          t8, -1
    bgtz           t8, 2b
     addiu         s2, 2

    lbu            t0, 0(t7)
    lbu            t2, -1(t7)
    sll            t1, t0, 1
    addu           t1, t1, t0 // t1 = invalue * 3
    addu           t1, t1, t2
    addiu          t1, 1
    srl            t1, t1, 2
    sb             t1, 0(s2)
    sb             t0, 1(s2)
    addiu          s1, 4
    bne            s1, s0, 0b
     addiu         a2, 4
3:
    RESTORE_REGS_FROM_STACK 16, s0, s1, s2, s3

    j              ra
     nop
END(jsimd_h2v1_fancy_upsample_mips_dspr2)

/*****************************************************************************/
LEAF_MIPS_DSPR2(jsimd_h2v1_downsample_mips_dspr2)
/*
 * a0     - cinfo->image_width
 * a1     - cinfo->max_v_samp_factor
 * a2     - compptr->v_samp_factor
 * a3     - compptr->width_in_blocks
 * 16(sp) - input_data
 * 20(sp) - output_data
 */
    .set at

    SAVE_REGS_ON_STACK 24, s0, s1, s2, s3, s4

    beqz        a2, 7f
     lw         s1, 44(sp)  // s1 = output_data
    lw          s0, 40(sp)  // s0 = input_data
    srl         s2, a0, 2
    andi        t9, a0, 2
    srl         t7, t9, 1
    addu        s2, t7, s2
    sll         t0, a3, 3   // t0 = width_in_blocks*DCT
    srl         t7, t0, 1
    subu        s2, t7, s2
0:
    andi        t6, a0, 1   // t6 = temp_index
    addiu       t6, -1
    lw          t4, 0(s1)   // t4 = outptr
    lw          t5, 0(s0)   // t5 = inptr0
    li          s3, 0       // s3 = bias
    srl         t7, a0, 1   // t7 = image_width1
    srl         s4, t7, 2
    andi        t8, t7, 3
1:
    ulhu        t0, 0(t5)
    ulhu        t1, 2(t5)
    ulhu        t2, 4(t5)
    ulhu        t3, 6(t5)
    raddu.w.qb  t0, t0
    raddu.w.qb  t1, t1
    raddu.w.qb  t2, t2
    raddu.w.qb  t3, t3
    shra.ph     t0, t0, 1
    shra_r.ph   t1, t1, 1
    shra.ph     t2, t2, 1
    shra_r.ph   t3, t3, 1
    sb          t0, 0(t4)
    sb          t1, 1(t4)
    sb          t2, 2(t4)
    sb          t3, 3(t4)
    addiu       s4, -1
    addiu       t4, 4
    bgtz        s4, 1b
     addiu      t5, 8
    beqz        t8, 3f
     addu       s4, t4, t8
2:
    ulhu        t0, 0(t5)
    raddu.w.qb  t0, t0
    addqh.w     t0, t0, s3
    xori        s3, s3, 1
    sb          t0, 0(t4)
    addiu       t4, 1
    bne         t4, s4, 2b
     addiu      t5, 2
3:
    lbux        t1, t6(t5)
    sll         t1, 1
    addqh.w     t2, t1, s3  // t2 = pixval1
    xori        s3, s3, 1
    addqh.w     t3, t1, s3  // t3 = pixval2
    blez        s2, 5f
     append     t3, t2,  8
    addu        t5, t4, s2  // t5 = loop_end2
4:
    ush         t3, 0(t4)
    addiu       s2, -1
    bgtz        s2, 4b
     addiu      t4,  2
5:
    beqz        t9, 6f
     nop
    sb          t2, 0(t4)
6:
    addiu       s1, 4
    addiu       a2, -1
    bnez        a2, 0b
     addiu      s0, 4
7:
    RESTORE_REGS_FROM_STACK 24, s0, s1, s2, s3, s4

    j           ra
    nop
END(jsimd_h2v1_downsample_mips_dspr2)

/*****************************************************************************/
LEAF_MIPS_DSPR2(jsimd_h2v2_downsample_mips_dspr2)

/*
 * a0     - cinfo->image_width
 * a1     - cinfo->max_v_samp_factor
 * a2     - compptr->v_samp_factor
 * a3     - compptr->width_in_blocks
 * 16(sp) - input_data
 * 20(sp) - output_data
 */
    .set at
    SAVE_REGS_ON_STACK 32, s0, s1, s2, s3, s4, s5, s6, s7

    beqz         a2, 8f
     lw          s1, 52(sp)      // s1 = output_data
    lw           s0, 48(sp)      // s0 = input_data

    andi         t6, a0, 1       // t6 = temp_index
    addiu        t6, -1
    srl          t7, a0, 1       // t7 = image_width1
    srl          s4, t7, 2
    andi         t8, t7, 3
    andi         t9, a0, 2
    srl          s2, a0, 2
    srl          t7, t9, 1
    addu         s2, t7, s2
    sll          t0, a3, 3       // s2 = width_in_blocks*DCT
    srl          t7, t0, 1
    subu         s2, t7, s2
0:
    lw           t4, 0(s1)       // t4 = outptr
    lw           t5, 0(s0)       // t5 = inptr0
    lw           s7, 4(s0)       // s7 = inptr1
    li           s6, 1           // s6 = bias
2:
    ulw          t0, 0(t5)       // t0 = |P3|P2|P1|P0|
    ulw          t1, 0(s7)       // t1 = |Q3|Q2|Q1|Q0|
    ulw          t2, 4(t5)
    ulw          t3, 4(s7)
    precrq.ph.w  t7, t0, t1      // t2 = |P3|P2|Q3|Q2|
    ins          t0, t1, 16, 16  // t0 = |Q1|Q0|P1|P0|
    raddu.w.qb   t1, t7
    raddu.w.qb   t0, t0
    shra_r.w     t1, t1, 2
    addiu        t0, 1
    srl          t0, 2
    precrq.ph.w  t7, t2, t3
    ins          t2, t3, 16, 16
    raddu.w.qb   t7, t7
    raddu.w.qb   t2, t2
    shra_r.w     t7, t7, 2
    addiu        t2, 1
    srl          t2, 2
    sb           t0, 0(t4)
    sb           t1, 1(t4)
    sb           t2, 2(t4)
    sb           t7, 3(t4)
    addiu        t4, 4
    addiu        t5, 8
    addiu        s4, s4, -1
    bgtz         s4, 2b
     addiu       s7, 8
    beqz         t8, 4f
     addu        t8, t4, t8
3:
    ulhu         t0, 0(t5)
    ulhu         t1, 0(s7)
    ins          t0, t1, 16, 16
    raddu.w.qb   t0, t0
    addu         t0, t0, s6
    srl          t0, 2
    xori         s6, s6, 3
    sb           t0, 0(t4)
    addiu        t5, 2
    addiu        t4, 1
    bne          t8, t4, 3b
     addiu       s7, 2
4:
    lbux         t1, t6(t5)
    sll          t1, 1
    lbux         t0, t6(s7)
    sll          t0, 1
    addu         t1, t1, t0
    addu         t3, t1, s6
    srl          t0, t3, 2       // t2 = pixval1
    xori         s6, s6, 3
    addu         t2, t1, s6
    srl          t1, t2, 2       // t3 = pixval2
    blez         s2, 6f
     append      t1, t0, 8
5:
    ush          t1, 0(t4)
    addiu        s2, -1
    bgtz         s2, 5b
     addiu       t4, 2
6:
    beqz         t9, 7f
     nop
    sb           t0, 0(t4)
7:
    addiu        s1, 4
    addiu        a2, -1
    bnez         a2, 0b
     addiu       s0, 8
8:
    RESTORE_REGS_FROM_STACK 32, s0, s1, s2, s3, s4, s5, s6, s7

    j            ra
     nop
END(jsimd_h2v2_downsample_mips_dspr2)
/*****************************************************************************/
LEAF_MIPS_DSPR2(jsimd_h2v1_upsample_mips_dspr2)
/*
 * a0     - cinfo->max_v_samp_factor
 * a1     - cinfo->output_width
 * a2     - input_data
 * a3     - output_data_ptr
 */
    lw      t7, 0(a3)       // t7 = output_data
    andi    t8, a1, 0xf     // t8 = residual
    sll     t0, a0, 2
    blez    a0, 4f
     addu   t9, t7, t0      // t9 = output_data end address
0:
    lw      t5, 0(t7)       // t5 = outptr
    lw      t6, 0(a2)       // t6 = inptr
    addu    t3, t5, a1      // t3 = outptr + output_width (end address)
    subu    t3, t8          // t3 = end address - residual
    beq     t5, t3, 2f
     move   t4, t8
1:
    ulw     t0, 0(t6)       // t0 = |P3|P2|P1|P0|
    ulw     t2, 4(t6)       // t2 = |P7|P6|P5|P4|
    srl     t1, t0, 16      // t1 = |X|X|P3|P2|
    ins     t0, t0, 16, 16  // t0 = |P1|P0|P1|P0|
    ins     t1, t1, 16, 16  // t1 = |P3|P2|P3|P2|
    ins     t0, t0, 8, 16   // t0 = |P1|P1|P0|P0|
    ins     t1, t1, 8, 16   // t1 = |P3|P3|P2|P2|
    usw     t0, 0(t5)
    usw     t1, 4(t5)
    srl     t0, t2, 16      // t0 = |X|X|P7|P6|
    ins     t2, t2, 16, 16  // t2 = |P5|P4|P5|P4|
    ins     t0, t0, 16, 16  // t0 = |P7|P6|P7|P6|
    ins     t2, t2, 8, 16   // t2 = |P5|P5|P4|P4|
    ins     t0, t0, 8, 16   // t0 = |P7|P7|P6|P6|
    usw     t2, 8(t5)
    usw     t0, 12(t5)
    addiu   t5, 16
    bne     t5, t3, 1b
     addiu  t6, 8
    beqz    t8, 3f
     move   t4, t8
2:
    lbu     t1, 0(t6)
    sb      t1, 0(t5)
    sb      t1, 1(t5)
    addiu   t4, -2
    addiu   t6, 1
    bgtz    t4, 2b
     addiu  t5, 2
3:
    addiu   t7, 4
    bne     t9, t7, 0b
     addiu  a2, 4
4:
    j       ra
     nop
END(jsimd_h2v1_upsample_mips_dspr2)

/*****************************************************************************/
LEAF_MIPS_DSPR2(jsimd_h2v2_upsample_mips_dspr2)
/*
 * a0     - cinfo->max_v_samp_factor
 * a1     - cinfo->output_width
 * a2     - input_data
 * a3     - output_data_ptr
 */
    lw      t7, 0(a3)
    blez    a0, 7f
     andi   t9, a1, 0xf     // t9 = residual
0:
    lw      t6, 0(a2)       // t6 = inptr
    lw      t5, 0(t7)       // t5 = outptr
    addu    t8, t5, a1      // t8 = outptr end address
    subu    t8, t9          // t8 = end address - residual
    beq     t5, t8, 2f
     move   t4, t9
1:
    ulw     t0, 0(t6)
    srl     t1, t0, 16
    ins     t0, t0, 16, 16
    ins     t0, t0, 8, 16
    ins     t1, t1, 16, 16
    ins     t1, t1, 8, 16
    ulw     t2, 4(t6)
    usw     t0, 0(t5)
    usw     t1, 4(t5)
    srl     t3, t2, 16
    ins     t2, t2, 16, 16
    ins     t2, t2, 8, 16
    ins     t3, t3, 16, 16
    ins     t3, t3, 8, 16
    usw     t2, 8(t5)
    usw     t3, 12(t5)
    addiu   t5, 16
    bne     t5, t8, 1b
     addiu  t6, 8
    beqz    t9, 3f
     move   t4, t9
2:
    lbu     t0, 0(t6)
    sb      t0, 0(t5)
    sb      t0, 1(t5)
    addiu   t4, -2
    addiu   t6, 1
    bgtz    t4, 2b
     addiu  t5, 2
3:
    ulw     t6, 0(t7)       // t6 = outptr
    ulw     t5, 4(t7)       // t5 = outptr[1]
    addu    t4, t6, a1      // t4 = new end address
    subu    t8, t4, t9
    beqz    t8, 5f
     nop
4:
    ulw     t0, 0(t6)
    ulw     t1, 4(t6)
    ulw     t2, 8(t6)
    usw     t0, 0(t5)
    ulw     t0, 12(t6)
    usw     t1, 4(t5)
    usw     t2, 8(t5)
    usw     t0, 12(t5)
    addiu   t6, 16
    bne     t6, t8, 4b
     addiu  t5, 16
    beqz    t9, 6f
     nop
5:
    lbu     t0, 0(t6)
    sb      t0, 0(t5)
    addiu   t6, 1
    bne     t6, t4, 5b
     addiu  t5, 1
6:
    addiu   t7, 8
    addiu   a0, -2
    bgtz    a0, 0b
     addiu  a2, 4
7:
    j       ra
     nop
END(jsimd_h2v2_upsample_mips_dspr2)
/*****************************************************************************/
LEAF_MIPS_DSPR2(jsimd_fdct_islow_mips_dspr2)
/*
 * a0     - data
 */

    SAVE_REGS_ON_STACK 40, s0, s1, s2, s3, s4, s5, s6, s7, s8

    lui       t0, 6437
    ori       t0, 2260
    lui       t1, 9633
    ori       t1, 11363
    lui       t2, 0xd39e
    ori       t2, 0xe6dc
    lui       t3, 0xf72d
    ori       t3, 9633
    lui       t4, 2261
    ori       t4, 9633
    lui       t5, 0xd39e
    ori       t5, 6437
    lui       t6, 9633
    ori       t6, 0xd39d
    lui       t7, 0xe6dc
    ori       t7, 2260
    lui       t8, 4433
    ori       t8, 10703
    lui       t9, 0xd630
    ori       t9, 4433
    li        s8, 8
    move      a1, a0
1:
    lw        s0, 0(a1)     // tmp0 = 1|0
    lw        s1, 4(a1)     // tmp1 = 3|2
    lw        s2, 8(a1)     // tmp2 = 5|4
    lw        s3, 12(a1)    // tmp3 = 7|6
    packrl.ph s1, s1, s1    // tmp1 = 2|3
    packrl.ph s3, s3, s3    // tmp3 = 6|7
    subq.ph   s7, s1, s2    // tmp7 = 2-5|3-4 = t5|t4
    subq.ph   s5, s0, s3    // tmp5 = 1-6|0-7 = t6|t7
    mult      $0, $0        // ac0  = 0
    dpa.w.ph  $ac0, s7, t0  // ac0 += t5*  6437 + t4*  2260
    dpa.w.ph  $ac0, s5, t1  // ac0 += t6*  9633 + t7* 11363
    mult      $ac1, $0, $0  // ac1  = 0
    dpa.w.ph  $ac1, s7, t2  // ac1 += t5*-11362 + t4* -6436
    dpa.w.ph  $ac1, s5, t3  // ac1 += t6* -2259 + t7*  9633
    mult      $ac2, $0, $0  // ac2  = 0
    dpa.w.ph  $ac2, s7, t4  // ac2 += t5*  2261 + t4*  9633
    dpa.w.ph  $ac2, s5, t5  // ac2 += t6*-11362 + t7*  6437
    mult      $ac3, $0, $0  // ac3  = 0
    dpa.w.ph  $ac3, s7, t6  // ac3 += t5*  9633 + t4*-11363
    dpa.w.ph  $ac3, s5, t7  // ac3 += t6* -6436 + t7*  2260
    addq.ph   s6, s1, s2    // tmp6 = 2+5|3+4 = t2|t3
    addq.ph   s4, s0, s3    // tmp4 = 1+6|0+7 = t1|t0
    extr_r.w  s0, $ac0, 11  // tmp0 = (ac0 + 1024) >> 11
    extr_r.w  s1, $ac1, 11  // tmp1 = (ac1 + 1024) >> 11
    extr_r.w  s2, $ac2, 11  // tmp2 = (ac2 + 1024) >> 11
    extr_r.w  s3, $ac3, 11  // tmp3 = (ac3 + 1024) >> 11
    addq.ph   s5, s4, s6    // tmp5 = t1+t2|t0+t3 = t11|t10
    subq.ph   s7, s4, s6    // tmp7 = t1-t2|t0-t3 = t12|t13
    sh        s0, 2(a1)
    sh        s1, 6(a1)
    sh        s2, 10(a1)
    sh        s3, 14(a1)
    mult      $0, $0        // ac0  = 0
    dpa.w.ph  $ac0, s7, t8  // ac0 += t12*  4433 + t13* 10703
    mult      $ac1, $0, $0  // ac1  = 0
    dpa.w.ph  $ac1, s7, t9  // ac1 += t12*-10704 + t13*  4433
    sra       s4, s5, 16    // tmp4 = t11
    addiu     a1, a1, 16
    addiu     s8, s8, -1
    extr_r.w  s0, $ac0, 11  // tmp0 = (ac0 + 1024) >> 11
    extr_r.w  s1, $ac1, 11  // tmp1 = (ac1 + 1024) >> 11
    addu      s2, s5, s4    // tmp2 = t10 + t11
    subu      s3, s5, s4    // tmp3 = t10 - t11
    sll       s2, s2, 2     // tmp2 = (t10 + t11) << 2
    sll       s3, s3, 2     // tmp3 = (t10 - t11) << 2
    sh        s2, -16(a1)
    sh        s3, -8(a1)
    sh        s0, -12(a1)
    bgtz      s8, 1b
     sh       s1, -4(a1)
    li        t0, 2260
    li        t1, 11363
    li        t2, 9633
    li        t3, 6436
    li        t4, 6437
    li        t5, 2261
    li        t6, 11362
    li        t7, 2259
    li        t8, 4433
    li        t9, 10703
    li        a1, 10704
    li        s8, 8

2:
    lh        a2, 0(a0)     // 0
    lh        a3, 16(a0)    // 8
    lh        v0, 32(a0)    // 16
    lh        v1, 48(a0)    // 24
    lh        s4, 64(a0)    // 32
    lh        s5, 80(a0)    // 40
    lh        s6, 96(a0)    // 48
    lh        s7, 112(a0)   // 56
    addu      s2, v0, s5    // tmp2 = 16 + 40
    subu      s5, v0, s5    // tmp5 = 16 - 40
    addu      s3, v1, s4    // tmp3 = 24 + 32
    subu      s4, v1, s4    // tmp4 = 24 - 32
    addu      s0, a2, s7    // tmp0 =  0 + 56
    subu      s7, a2, s7    // tmp7 =  0 - 56
    addu      s1, a3, s6    // tmp1 =  8 + 48
    subu      s6, a3, s6    // tmp6 =  8 - 48
    addu      a2, s0, s3    // tmp10 = tmp0 + tmp3
    subu      v1, s0, s3    // tmp13 = tmp0 - tmp3
    addu      a3, s1, s2    // tmp11 = tmp1 + tmp2
    subu      v0, s1, s2    // tmp12 = tmp1 - tmp2
    mult      s7, t1        // ac0  = tmp7 * c1
    madd      s4, t0        // ac0 += tmp4 * c0
    madd      s5, t4        // ac0 += tmp5 * c4
    madd      s6, t2        // ac0 += tmp6 * c2
    mult      $ac1, s7, t2  // ac1  = tmp7 * c2
    msub      $ac1, s4, t3  // ac1 -= tmp4 * c3
    msub      $ac1, s5, t6  // ac1 -= tmp5 * c6
    msub      $ac1, s6, t7  // ac1 -= tmp6 * c7
    mult      $ac2, s7, t4  // ac2  = tmp7 * c4
    madd      $ac2, s4, t2  // ac2 += tmp4 * c2
    madd      $ac2, s5, t5  // ac2 += tmp5 * c5
    msub      $ac2, s6, t6  // ac2 -= tmp6 * c6
    mult      $ac3, s7, t0  // ac3  = tmp7 * c0
    msub      $ac3, s4, t1  // ac3 -= tmp4 * c1
    madd      $ac3, s5, t2  // ac3 += tmp5 * c2
    msub      $ac3, s6, t3  // ac3 -= tmp6 * c3
    extr_r.w  s0, $ac0, 15  // tmp0 = (ac0 + 16384) >> 15
    extr_r.w  s1, $ac1, 15  // tmp1 = (ac1 + 16384) >> 15
    extr_r.w  s2, $ac2, 15  // tmp2 = (ac2 + 16384) >> 15
    extr_r.w  s3, $ac3, 15  // tmp3 = (ac3 + 16384) >> 15
    addiu     s8, s8, -1
    addu      s4, a2, a3    // tmp4 = tmp10 + tmp11
    subu      s5, a2, a3    // tmp5 = tmp10 - tmp11
    sh        s0, 16(a0)
    sh        s1, 48(a0)
    sh        s2, 80(a0)
    sh        s3, 112(a0)
    mult      v0, t8        // ac0  = tmp12 * c8
    madd      v1, t9        // ac0 += tmp13 * c9
    mult      $ac1, v1, t8  // ac1  = tmp13 * c8
    msub      $ac1, v0, a1  // ac1 -= tmp12 * c10
    addiu     a0, a0, 2
    extr_r.w  s6, $ac0, 15  // tmp6 = (ac0 + 16384) >> 15
    extr_r.w  s7, $ac1, 15  // tmp7 = (ac1 + 16384) >> 15
    shra_r.w  s4, s4, 2     // tmp4 = (tmp4 + 2) >> 2
    shra_r.w  s5, s5, 2     // tmp5 = (tmp5 + 2) >> 2
    sh        s4, -2(a0)
    sh        s5, 62(a0)
    sh        s6, 30(a0)
    bgtz      s8, 2b
     sh       s7, 94(a0)

    RESTORE_REGS_FROM_STACK 40, s0, s1, s2, s3, s4, s5, s6, s7, s8

    jr       ra
     nop

END(jsimd_fdct_islow_mips_dspr2)

/*****************************************************************************/
LEAF_MIPS_DSPR2(jsimd_quantize_mips_dspr2)
/*
 * a0     - coef_block
 * a1     - divisors
 * a2     - workspace
 */

    .set at

    SAVE_REGS_ON_STACK 16, s0, s1, s2

    addiu   v0, a2, 124  // v0 = workspace_end
    lh      t0, 0(a2)
    lh      t1, 0(a1)
    lh      t2, 128(a1)
    sra     t3, t0, 15
    sll     t3, t3, 1
    addiu   t3, t3, 1
    mul     t0, t0, t3
    lh      t4, 384(a1)
    lh      t5, 130(a1)
    lh      t6, 2(a2)
    lh      t7, 2(a1)
    lh      t8, 386(a1)

1:
    andi    t1, 0xffff
    add     t9, t0, t2
    andi    t9, 0xffff
    mul     v1, t9, t1
    sra     s0, t6, 15
    sll     s0, s0, 1
    addiu   s0, s0, 1
    addiu   t9, t4, 16
    srav    v1, v1, t9
    mul     v1, v1, t3
    mul     t6, t6, s0
    andi    t7, 0xffff
    addiu   a2, a2, 4
    addiu   a1, a1, 4
    add     s1, t6, t5
    andi    s1, 0xffff
    sh      v1, 0(a0)

    mul     s2, s1, t7
    addiu   s1, t8, 16
    srav    s2, s2, s1
    mul     s2,s2, s0
    lh      t0, 0(a2)
    lh      t1, 0(a1)
    sra     t3, t0, 15
    sll     t3, t3, 1
    addiu   t3, t3, 1
    mul     t0, t0, t3
    lh      t2, 128(a1)
    lh      t4, 384(a1)
    lh      t5, 130(a1)
    lh      t8, 386(a1)
    lh      t6, 2(a2)
    lh      t7, 2(a1)
    sh      s2, 2(a0)
    lh      t0, 0(a2)
    sra     t3, t0, 15
    sll     t3, t3, 1
    addiu   t3, t3, 1
    mul     t0, t0,t3
    bne     a2, v0, 1b
     addiu  a0, a0, 4

    andi    t1, 0xffff
    add     t9, t0, t2
    andi    t9, 0xffff
    mul     v1, t9, t1
    sra     s0, t6, 15
    sll     s0, s0, 1
    addiu   s0, s0, 1
    addiu   t9, t4, 16
    srav    v1, v1, t9
    mul     v1, v1, t3
    mul     t6, t6, s0
    andi    t7, 0xffff
    sh      v1, 0(a0)
    add     s1, t6, t5
    andi    s1, 0xffff
    mul     s2, s1, t7
    addiu   s1, t8, 16
    addiu   a2, a2, 4
    addiu   a1, a1, 4
    srav    s2, s2, s1
    mul     s2, s2, s0
    sh      s2, 2(a0)

    RESTORE_REGS_FROM_STACK 16, s0, s1, s2

    j       ra
     nop

END(jsimd_quantize_mips_dspr2)

/*****************************************************************************/
LEAF_MIPS_DSPR2(jsimd_idct_2x2_mips_dspr2)
/*
 * a0     - compptr->dct_table
 * a1     - coef_block
 * a2     - output_buf
 * a3     - output_col
 */
    .set at

    SAVE_REGS_ON_STACK 24, s0, s1, s2, s3, s4, s5

    addiu     sp, sp, -40
    move      v0, sp
    addiu     s2, zero, 29692
    addiu     s3, zero, -10426
    addiu     s4, zero, 6967
    addiu     s5, zero, -5906
    lh        t0, 0(a1)         // t0 = inptr[DCTSIZE*0]
    lh        t5, 0(a0)         // t5 = quantptr[DCTSIZE*0]
    lh        t1, 48(a1)        // t1 = inptr[DCTSIZE*3]
    lh        t6, 48(a0)        // t6 = quantptr[DCTSIZE*3]
    mul       t4, t5, t0
    lh        t0, 16(a1)        // t0 = inptr[DCTSIZE*1]
    lh        t5, 16(a0)        // t5 = quantptr[DCTSIZE*1]
    mul       t6, t6, t1
    mul       t5, t5, t0
    lh        t2, 80(a1)        // t2 = inptr[DCTSIZE*5]
    lh        t7, 80(a0)        // t7 = quantptr[DCTSIZE*5]
    lh        t3, 112(a1)       // t3 = inptr[DCTSIZE*7]
    lh        t8, 112(a0)       // t8 = quantptr[DCTSIZE*7]
    mul       t7, t7, t2
    mult      zero, zero
    mul       t8, t8, t3
    li        s0, 0x73FCD746    // s0 = (29692 << 16) | (-10426 & 0xffff)
    li        s1, 0x1B37E8EE    // s1 = (6967 << 16) | (-5906 & 0xffff)
    ins       t6, t5, 16, 16    // t6 = t5|t6
    sll       t4, t4, 15
    dpa.w.ph  $ac0, t6, s0
    lh        t1, 2(a1)
    lh        t6, 2(a0)
    ins       t8, t7, 16, 16    // t8 = t7|t8
    dpa.w.ph  $ac0, t8, s1
    mflo      t0, $ac0
    mul       t5, t6, t1
    lh        t1, 18(a1)
    lh        t6, 18(a0)
    lh        t2, 50(a1)
    lh        t7, 50(a0)
    mul       t6, t6, t1
    subu      t8, t4, t0
    mul       t7, t7, t2
    addu      t0, t4, t0
    shra_r.w  t0, t0, 13
    lh        t1, 82(a1)
    lh        t2, 82(a0)
    lh        t3, 114(a1)
    lh        t4, 114(a0)
    shra_r.w  t8, t8, 13
    mul       t1, t1, t2
    mul       t3, t3, t4
    sw        t0, 0(v0)
    sw        t8, 20(v0)
    sll       t4, t5, 15
    ins       t7, t6, 16, 16
    mult      zero, zero
    dpa.w.ph  $ac0, t7, s0
    ins       t3, t1, 16, 16
    lh        t1, 6(a1)
    lh        t6, 6(a0)
    dpa.w.ph  $ac0, t3, s1
    mflo      t0, $ac0
    mul       t5, t6, t1
    lh        t1, 22(a1)
    lh        t6, 22(a0)
    lh        t2, 54(a1)
    lh        t7, 54(a0)
    mul       t6, t6, t1
    subu      t8, t4, t0
    mul       t7, t7, t2
    addu      t0, t4, t0
    shra_r.w  t0, t0, 13
    lh        t1, 86(a1)
    lh        t2, 86(a0)
    lh        t3, 118(a1)
    lh        t4, 118(a0)
    shra_r.w  t8, t8, 13
    mul       t1, t1, t2
    mul       t3, t3, t4
    sw        t0, 4(v0)
    sw        t8, 24(v0)
    sll       t4, t5, 15
    ins       t7, t6, 16, 16
    mult      zero, zero
    dpa.w.ph  $ac0, t7, s0
    ins       t3, t1, 16, 16
    lh        t1, 10(a1)
    lh        t6, 10(a0)
    dpa.w.ph  $ac0, t3, s1
    mflo      t0, $ac0
    mul       t5, t6, t1
    lh        t1, 26(a1)
    lh        t6, 26(a0)
    lh        t2, 58(a1)
    lh        t7, 58(a0)
    mul       t6, t6, t1
    subu      t8, t4, t0
    mul       t7, t7, t2
    addu      t0, t4, t0
    shra_r.w  t0, t0, 13
    lh        t1, 90(a1)
    lh        t2, 90(a0)
    lh        t3, 122(a1)
    lh        t4, 122(a0)
    shra_r.w  t8, t8, 13
    mul       t1, t1, t2
    mul       t3, t3, t4
    sw        t0, 8(v0)
    sw        t8, 28(v0)
    sll       t4, t5, 15
    ins       t7, t6, 16, 16
    mult      zero, zero
    dpa.w.ph  $ac0, t7, s0
    ins       t3, t1, 16, 16
    lh        t1, 14(a1)
    lh        t6, 14(a0)
    dpa.w.ph  $ac0, t3, s1
    mflo      t0, $ac0
    mul       t5, t6, t1
    lh        t1, 30(a1)
    lh        t6, 30(a0)
    lh        t2, 62(a1)
    lh        t7, 62(a0)
    mul       t6, t6, t1
    subu      t8, t4, t0
    mul       t7, t7, t2
    addu      t0, t4, t0
    shra_r.w  t0, t0, 13
    lh        t1, 94(a1)
    lh        t2, 94(a0)
    lh        t3, 126(a1)
    lh        t4, 126(a0)
    shra_r.w  t8, t8, 13
    mul       t1, t1, t2
    mul       t3, t3, t4
    sw        t0, 12(v0)
    sw        t8, 32(v0)
    sll       t4, t5, 15
    ins       t7, t6, 16, 16
    mult      zero, zero
    dpa.w.ph  $ac0, t7, s0
    ins       t3, t1, 16, 16
    dpa.w.ph  $ac0, t3, s1
    mflo      t0, $ac0
    lw        t9, 0(a2)
    lw        t3, 0(v0)
    lw        t7, 4(v0)
    lw        t1, 8(v0)
    addu      t9, t9, a3
    sll       t3, t3, 15
    subu      t8, t4, t0
    addu      t0, t4, t0
    shra_r.w  t0, t0, 13
    shra_r.w  t8, t8, 13
    sw        t0, 16(v0)
    sw        t8, 36(v0)
    lw        t5, 12(v0)
    lw        t6, 16(v0)
    mult      t7, s2
    madd      t1, s3
    madd      t5, s4
    madd      t6, s5
    lw        t5, 24(v0)
    lw        t7, 28(v0)
    mflo      t0, $ac0
    lw        t8, 32(v0)
    lw        t2, 36(v0)
    mult      $ac1, t5, s2
    madd      $ac1, t7, s3
    madd      $ac1, t8, s4
    madd      $ac1, t2, s5
    addu      t1, t3, t0
    subu      t6, t3, t0
    shra_r.w  t1, t1, 20
    shra_r.w  t6, t6, 20
    mflo      t4, $ac1
    shll_s.w  t1, t1, 24
    shll_s.w  t6, t6, 24
    sra       t1, t1, 24
    sra       t6, t6, 24
    addiu     t1, t1, 128
    addiu     t6, t6, 128
    lw        t0, 20(v0)
    sb        t1, 0(t9)
    sb        t6, 1(t9)
    sll       t0, t0, 15
    lw        t9, 4(a2)
    addu      t1, t0, t4
    subu      t6, t0, t4
    addu      t9, t9, a3
    shra_r.w  t1, t1, 20
    shra_r.w  t6, t6, 20
    shll_s.w  t1, t1, 24
    shll_s.w  t6, t6, 24
    sra       t1, t1, 24
    sra       t6, t6, 24
    addiu     t1, t1, 128
    addiu     t6, t6, 128
    sb        t1, 0(t9)
    sb        t6, 1(t9)
    addiu     sp, sp, 40

    RESTORE_REGS_FROM_STACK 24, s0, s1, s2, s3, s4, s5

    j         ra
     nop

END(jsimd_idct_2x2_mips_dspr2)

/*****************************************************************************/
LEAF_MIPS_DSPR2(jsimd_idct_4x4_mips_dspr2)
/*
 * a0     - compptr->dct_table
 * a1     - coef_block
 * a2     - output_buf
 * a3     - output_col
 * 16(sp) - workspace[DCTSIZE*4];  // buffers data between passes
 */

    .set at
    SAVE_REGS_ON_STACK 32, s0, s1, s2, s3, s4, s5, s6, s7

    lw        v1, 48(sp)
    move      t0, a1
    move      t1, v1
    li        t9, 4
    li        s0, 0x2e75f93e
    li        s1, 0x21f9ba79
    li        s2, 0xecc2efb0
    li        s3, 0x52031ccd

0:
    lh        s6, 32(t0)        // inptr[DCTSIZE*2]
    lh        t6, 32(a0)        // quantptr[DCTSIZE*2]
    lh        s7, 96(t0)        // inptr[DCTSIZE*6]
    lh        t7, 96(a0)        // quantptr[DCTSIZE*6]
    mul       t6, s6, t6        // z2 = (inptr[DCTSIZE*2] * quantptr[DCTSIZE*2])
    lh        s4, 0(t0)         // inptr[DCTSIZE*0]
    mul       t7, s7, t7        // z3 = (inptr[DCTSIZE*6] * quantptr[DCTSIZE*6])
    lh        s5, 0(a0)         // quantptr[0]
    li        s6, 15137
    li        s7, 6270
    mul       t2, s4, s5        // tmp0 = (inptr[0] * quantptr[0])
    mul       t6, s6, t6        // z2 = (inptr[DCTSIZE*2] * quantptr[DCTSIZE*2])
    lh        t5, 112(t0)       // inptr[DCTSIZE*7]
    mul       t7, s7, t7        // z3 = (inptr[DCTSIZE*6] * quantptr[DCTSIZE*6])
    lh        s4, 112(a0)       // quantptr[DCTSIZE*7]
    lh        v0, 80(t0)        // inptr[DCTSIZE*5]
    lh        s5, 80(a0)        // quantptr[DCTSIZE*5]
    lh        s6, 48(a0)        // quantptr[DCTSIZE*3]
    sll       t2, t2, 14        // tmp0 <<= (CONST_BITS+1)
    lh        s7, 16(a0)        // quantptr[DCTSIZE*1]
    lh        t8, 16(t0)        // inptr[DCTSIZE*1]
    subu      t6, t6, t7        // tmp2 = MULTIPLY(z2, t5) - MULTIPLY(z3, t6)
    lh        t7, 48(t0)        // inptr[DCTSIZE*3]
    mul       t5, s4, t5        // z1 = (inptr[DCTSIZE*7] * quantptr[DCTSIZE*7])
    mul       v0, s5, v0        // z2 = (inptr[DCTSIZE*5] * quantptr[DCTSIZE*5])
    mul       t7, s6, t7        // z3 = (inptr[DCTSIZE*3] * quantptr[DCTSIZE*3])
    mul       t8, s7, t8        // z4 = (inptr[DCTSIZE*1] * quantptr[DCTSIZE*1])
    addu      t3, t2, t6        // tmp10 = tmp0 + z2
    subu      t4, t2, t6        // tmp10 = tmp0 - z2
    mult      $ac0, zero, zero
    mult      $ac1, zero, zero
    ins       t5, v0, 16, 16
    ins       t7, t8, 16, 16
    addiu     t9, t9, -1
    dpa.w.ph  $ac0, t5, s0
    dpa.w.ph  $ac0, t7, s1
    dpa.w.ph  $ac1, t5, s2
    dpa.w.ph  $ac1, t7, s3
    mflo      s4, $ac0
    mflo      s5, $ac1
    addiu     a0, a0, 2
    addiu     t1, t1, 4
    addiu     t0, t0, 2
    addu      t6, t4, s4
    subu      t5, t4, s4
    addu      s6, t3, s5
    subu      s7, t3, s5
    shra_r.w  t6, t6, 12        // DESCALE(tmp12 + temp1, 12)
    shra_r.w  t5, t5, 12        // DESCALE(tmp12 - temp1, 12)
    shra_r.w  s6, s6, 12        // DESCALE(tmp10 + temp2, 12)
    shra_r.w  s7, s7, 12        // DESCALE(tmp10 - temp2, 12)
    sw        t6, 28(t1)
    sw        t5, 60(t1)
    sw        s6, -4(t1)
    bgtz      t9, 0b
     sw       s7, 92(t1)
    // second loop three pass
    li        t9, 3
1:
    lh        s6, 34(t0)        // inptr[DCTSIZE*2]
    lh        t6, 34(a0)        // quantptr[DCTSIZE*2]
    lh        s7, 98(t0)        // inptr[DCTSIZE*6]
    lh        t7, 98(a0)        // quantptr[DCTSIZE*6]
    mul       t6, s6, t6        // z2 = (inptr[DCTSIZE*2] * quantptr[DCTSIZE*2])
    lh        s4, 2(t0)         // inptr[DCTSIZE*0]
    mul       t7, s7, t7        // z3 = (inptr[DCTSIZE*6] * quantptr[DCTSIZE*6])
    lh        s5, 2(a0)         // quantptr[DCTSIZE*0]
    li        s6, 15137
    li        s7, 6270
    mul       t2, s4, s5        // tmp0 = (inptr[0] * quantptr[0])
    mul       v0, s6, t6        // z2 = (inptr[DCTSIZE*2] * quantptr[DCTSIZE*2])
    lh        t5, 114(t0)       // inptr[DCTSIZE*7]
    mul       t7, s7, t7        // z3 = (inptr[DCTSIZE*6] * quantptr[DCTSIZE*6])
    lh        s4, 114(a0)       // quantptr[DCTSIZE*7]
    lh        s5, 82(a0)        // quantptr[DCTSIZE*5]
    lh        t6, 82(t0)        // inptr[DCTSIZE*5]
    sll       t2, t2, 14        // tmp0 <<= (CONST_BITS+1)
    lh        s6, 50(a0)        // quantptr[DCTSIZE*3]
    lh        t8, 18(t0)        // inptr[DCTSIZE*1]
    subu      v0, v0, t7        // tmp2 = MULTIPLY(z2, t5) - MULTIPLY(z3, t6)
    lh        t7, 50(t0)        // inptr[DCTSIZE*3]
    lh        s7, 18(a0)        // quantptr[DCTSIZE*1]
    mul       t5, s4, t5        // z1 = (inptr[DCTSIZE*7] * quantptr[DCTSIZE*7])
    mul       t6, s5, t6        // z2 = (inptr[DCTSIZE*5] * quantptr[DCTSIZE*5])
    mul       t7, s6, t7        // z3 = (inptr[DCTSIZE*3] * quantptr[DCTSIZE*3])
    mul       t8, s7, t8        // z4 = (inptr[DCTSIZE*1] * quantptr[DCTSIZE*1])
    addu      t3, t2, v0        // tmp10 = tmp0 + z2
    subu      t4, t2, v0        // tmp10 = tmp0 - z2
    mult      $ac0, zero, zero
    mult      $ac1, zero, zero
    ins       t5, t6, 16, 16
    ins       t7, t8, 16, 16
    dpa.w.ph  $ac0, t5, s0
    dpa.w.ph  $ac0, t7, s1
    dpa.w.ph  $ac1, t5, s2
    dpa.w.ph  $ac1, t7, s3
    mflo      t5, $ac0
    mflo      t6, $ac1
    addiu     t9, t9, -1
    addiu     t0, t0, 2
    addiu     a0, a0, 2
    addiu     t1, t1, 4
    addu      s5, t4, t5
    subu      s4, t4, t5
    addu      s6, t3, t6
    subu      s7, t3, t6
    shra_r.w  s5, s5, 12        // DESCALE(tmp12 + temp1, 12)
    shra_r.w  s4, s4, 12        // DESCALE(tmp12 - temp1, 12)
    shra_r.w  s6, s6, 12        // DESCALE(tmp10 + temp2, 12)
    shra_r.w  s7, s7, 12        // DESCALE(tmp10 - temp2, 12)
    sw        s5, 32(t1)
    sw        s4, 64(t1)
    sw        s6, 0(t1)
    bgtz      t9, 1b
     sw       s7, 96(t1)
    move      t1, v1
    li        s4, 15137
    lw        s6, 8(t1)         // wsptr[2]
    li        s5, 6270
    lw        s7, 24(t1)        // wsptr[6]
    mul       s4, s4, s6        // MULTIPLY((INT32) wsptr[2], FIX_1_847759065)
    lw        t2, 0(t1)         // wsptr[0]
    mul       s5, s5, s7        // MULTIPLY((INT32) wsptr[6], - FIX_0_765366865)
    lh        t5, 28(t1)        // wsptr[7]
    lh        t6, 20(t1)        // wsptr[5]
    lh        t7, 12(t1)        // wsptr[3]
    lh        t8, 4(t1)         // wsptr[1]
    ins       t5, t6, 16, 16
    ins       t7, t8, 16, 16
    mult      $ac0, zero, zero
    dpa.w.ph  $ac0, t5, s0
    dpa.w.ph  $ac0, t7, s1
    mult      $ac1, zero, zero
    dpa.w.ph  $ac1, t5, s2
    dpa.w.ph  $ac1, t7, s3
    sll       t2, t2, 14        // tmp0 = ((INT32) wsptr[0]) << (CONST_BITS+1)
    mflo      s6, $ac0
    // MULTIPLY(wsptr[2], FIX_1_847759065 + MULTIPLY(wsptr[6], -FIX_0_765366865)
    subu      s4, s4, s5
    addu      t3, t2, s4        // tmp10 = tmp0 + z2
    mflo      s7, $ac1
    subu      t4, t2, s4        // tmp10 = tmp0 - z2
    addu      t7, t4, s6
    subu      t8, t4, s6
    addu      t5, t3, s7
    subu      t6, t3, s7
    shra_r.w  t5, t5, 19        // DESCALE(tmp10 + temp2, 19)
    shra_r.w  t6, t6, 19        // DESCALE(tmp10 - temp2, 19)
    shra_r.w  t7, t7, 19        // DESCALE(tmp12 + temp1, 19)
    shra_r.w  t8, t8, 19        // DESCALE(tmp12 - temp1, 19)
    sll       s4, t9, 2
    lw        v0, 0(a2)         // output_buf[ctr]
    shll_s.w  t5, t5, 24
    shll_s.w  t6, t6, 24
    shll_s.w  t7, t7, 24
    shll_s.w  t8, t8, 24
    sra       t5, t5, 24
    sra       t6, t6, 24
    sra       t7, t7, 24
    sra       t8, t8, 24
    addu      v0, v0, a3        // outptr = output_buf[ctr] + output_col
    addiu     t5, t5, 128
    addiu     t6, t6, 128
    addiu     t7, t7, 128
    addiu     t8, t8, 128
    sb        t5, 0(v0)
    sb        t7, 1(v0)
    sb        t8, 2(v0)
    sb        t6, 3(v0)
    // 2
    li        s4, 15137
    lw        s6, 40(t1)        // wsptr[2]
    li        s5, 6270
    lw        s7, 56(t1)        // wsptr[6]
    mul       s4, s4, s6        // MULTIPLY((INT32) wsptr[2], FIX_1_847759065)
    lw        t2, 32(t1)        // wsptr[0]
    mul       s5, s5, s7        // MULTIPLY((INT32) wsptr[6], - FIX_0_765366865)
    lh        t5, 60(t1)        // wsptr[7]
    lh        t6, 52(t1)        // wsptr[5]
    lh        t7, 44(t1)        // wsptr[3]
    lh        t8, 36(t1)        // wsptr[1]
    ins       t5, t6, 16, 16
    ins       t7, t8, 16, 16
    mult      $ac0, zero, zero
    dpa.w.ph  $ac0, t5, s0
    dpa.w.ph  $ac0, t7, s1
    mult      $ac1, zero, zero
    dpa.w.ph  $ac1, t5, s2
    dpa.w.ph  $ac1, t7, s3
    sll       t2, t2, 14        // tmp0 = ((INT32) wsptr[0]) << (CONST_BITS+1)
    mflo      s6, $ac0
    // MULTIPLY(wsptr[2], FIX_1_847759065 + MULTIPLY(wsptr[6], -FIX_0_765366865)
    subu      s4, s4, s5
    addu      t3, t2, s4        // tmp10 = tmp0 + z2
    mflo      s7, $ac1
    subu      t4, t2, s4        // tmp10 = tmp0 - z2
    addu      t7, t4, s6
    subu      t8, t4, s6
    addu      t5, t3, s7
    subu      t6, t3, s7
    shra_r.w  t5, t5, 19        // DESCALE(tmp10 + temp2, CONST_BITS-PASS1_BITS+1)
    shra_r.w  t6, t6, 19        // DESCALE(tmp10 - temp2, CONST_BITS-PASS1_BITS+1)
    shra_r.w  t7, t7, 19        // DESCALE(tmp12 + temp1, CONST_BITS-PASS1_BITS+1)
    shra_r.w  t8, t8, 19        // DESCALE(tmp12 - temp1, CONST_BITS-PASS1_BITS+1)
    sll       s4, t9, 2
    lw        v0, 4(a2)         // output_buf[ctr]
    shll_s.w  t5, t5, 24
    shll_s.w  t6, t6, 24
    shll_s.w  t7, t7, 24
    shll_s.w  t8, t8, 24
    sra       t5, t5, 24
    sra       t6, t6, 24
    sra       t7, t7, 24
    sra       t8, t8, 24
    addu      v0, v0, a3        // outptr = output_buf[ctr] + output_col
    addiu     t5, t5, 128
    addiu     t6, t6, 128
    addiu     t7, t7, 128
    addiu     t8, t8, 128
    sb        t5, 0(v0)
    sb        t7, 1(v0)
    sb        t8, 2(v0)
    sb        t6, 3(v0)
    // 3
    li        s4, 15137
    lw        s6, 72(t1)        // wsptr[2]
    li        s5, 6270
    lw        s7, 88(t1)        // wsptr[6]
    mul       s4, s4, s6        // MULTIPLY((INT32) wsptr[2], FIX_1_847759065)
    lw        t2, 64(t1)        // wsptr[0]
    mul       s5, s5, s7        // MULTIPLY((INT32) wsptr[6], - FIX_0_765366865)
    lh        t5, 92(t1)        // wsptr[7]
    lh        t6, 84(t1)        // wsptr[5]
    lh        t7, 76(t1)        // wsptr[3]
    lh        t8, 68(t1)        // wsptr[1]
    ins       t5, t6, 16, 16
    ins       t7, t8, 16, 16
    mult      $ac0, zero, zero
    dpa.w.ph  $ac0, t5, s0
    dpa.w.ph  $ac0, t7, s1
    mult      $ac1, zero, zero
    dpa.w.ph  $ac1, t5, s2
    dpa.w.ph  $ac1, t7, s3
    sll       t2, t2, 14        // tmp0 = ((INT32) wsptr[0]) << (CONST_BITS+1)
    mflo      s6, $ac0
    // MULTIPLY(wsptr[2], FIX_1_847759065 + MULTIPLY(wsptr[6], -FIX_0_765366865)
    subu      s4, s4, s5
    addu      t3, t2, s4        // tmp10 = tmp0 + z2
    mflo      s7, $ac1
    subu      t4, t2, s4        // tmp10 = tmp0 - z2
    addu      t7, t4, s6
    subu      t8, t4, s6
    addu      t5, t3, s7
    subu      t6, t3, s7
    shra_r.w  t5, t5, 19        // DESCALE(tmp10 + temp2, 19)
    shra_r.w  t6, t6, 19        // DESCALE(tmp10 - temp2, 19)
    shra_r.w  t7, t7, 19        // DESCALE(tmp12 + temp1, 19)
    shra_r.w  t8, t8, 19        // DESCALE(tmp12 - temp1, 19)
    sll       s4, t9, 2
    lw        v0, 8(a2)         // output_buf[ctr]
    shll_s.w  t5, t5, 24
    shll_s.w  t6, t6, 24
    shll_s.w  t7, t7, 24
    shll_s.w  t8, t8, 24
    sra       t5, t5, 24
    sra       t6, t6, 24
    sra       t7, t7, 24
    sra       t8, t8, 24
    addu      v0, v0, a3        // outptr = output_buf[ctr] + output_col
    addiu     t5, t5, 128
    addiu     t6, t6, 128
    addiu     t7, t7, 128
    addiu     t8, t8, 128
    sb        t5, 0(v0)
    sb        t7, 1(v0)
    sb        t8, 2(v0)
    sb        t6, 3(v0)
    li        s4, 15137
    lw        s6, 104(t1)       // wsptr[2]
    li        s5, 6270
    lw        s7, 120(t1)       // wsptr[6]
    mul       s4, s4, s6        // MULTIPLY((INT32) wsptr[2], FIX_1_847759065)
    lw        t2, 96(t1)        // wsptr[0]
    mul       s5, s5, s7        // MULTIPLY((INT32) wsptr[6], -FIX_0_765366865)
    lh        t5, 124(t1)       // wsptr[7]
    lh        t6, 116(t1)       // wsptr[5]
    lh        t7, 108(t1)       // wsptr[3]
    lh        t8, 100(t1)       // wsptr[1]
    ins       t5, t6, 16, 16
    ins       t7, t8, 16, 16
    mult      $ac0, zero, zero
    dpa.w.ph  $ac0, t5, s0
    dpa.w.ph  $ac0, t7, s1
    mult      $ac1, zero, zero
    dpa.w.ph  $ac1, t5, s2
    dpa.w.ph  $ac1, t7, s3
    sll       t2, t2, 14        // tmp0 = ((INT32) wsptr[0]) << (CONST_BITS+1)
    mflo      s6, $ac0
    // MULTIPLY(wsptr[2], FIX_1_847759065 + MULTIPLY(wsptr[6], -FIX_0_765366865)
    subu      s4, s4, s5
    addu      t3, t2, s4        // tmp10 = tmp0 + z2;
    mflo      s7, $ac1
    subu      t4, t2, s4        // tmp10 = tmp0 - z2;
    addu      t7, t4, s6
    subu      t8, t4, s6
    addu      t5, t3, s7
    subu      t6, t3, s7
    shra_r.w  t5, t5, 19        // DESCALE(tmp10 + temp2, 19)
    shra_r.w  t6, t6, 19        // DESCALE(tmp10 - temp2, 19)
    shra_r.w  t7, t7, 19        // DESCALE(tmp12 + temp1, 19)
    shra_r.w  t8, t8, 19        // DESCALE(tmp12 - temp1, 19)
    sll       s4, t9, 2
    lw        v0, 12(a2)        // output_buf[ctr]
    shll_s.w  t5, t5, 24
    shll_s.w  t6, t6, 24
    shll_s.w  t7, t7, 24
    shll_s.w  t8, t8, 24
    sra       t5, t5, 24
    sra       t6, t6, 24
    sra       t7, t7, 24
    sra       t8, t8, 24
    addu      v0, v0, a3        // outptr = output_buf[ctr] + output_col
    addiu     t5, t5, 128
    addiu     t6, t6, 128
    addiu     t7, t7, 128
    addiu     t8, t8, 128
    sb        t5, 0(v0)
    sb        t7, 1(v0)
    sb        t8, 2(v0)
    sb        t6, 3(v0)

    RESTORE_REGS_FROM_STACK 32, s0, s1, s2, s3, s4, s5, s6, s7

    j         ra
     nop
END(jsimd_idct_4x4_mips_dspr2)

/*****************************************************************************/
LEAF_MIPS_DSPR2(jsimd_idct_6x6_mips_dspr2)
/*
 * a0     - compptr->dct_table
 * a1     - coef_block
 * a2     - output_buf
 * a3     - output_col
 */
    .set at

    SAVE_REGS_ON_STACK 32, s0, s1, s2, s3, s4, s5, s6, s7

    addiu     sp, sp, -144
    move      v0, sp
    addiu     v1, v0, 24
    addiu     t9, zero, 5793
    addiu     s0, zero, 10033
    addiu     s1, zero, 2998

1:
    lh        s2, 0(a0)   // q0 = quantptr[ 0]
    lh        s3, 32(a0)  // q1 = quantptr[16]
    lh        s4, 64(a0)  // q2 = quantptr[32]
    lh        t2, 64(a1)  // tmp2 = inptr[32]
    lh        t1, 32(a1)  // tmp1 = inptr[16]
    lh        t0, 0(a1)   // tmp0 = inptr[ 0]
    mul       t2, t2, s4  // tmp2 = tmp2 * q2
    mul       t1, t1, s3  // tmp1 = tmp1 * q1
    mul       t0, t0, s2  // tmp0 = tmp0 * q0
    lh        t6, 16(a1)  // z1 = inptr[ 8]
    lh        t8, 80(a1)  // z3 = inptr[40]
    lh        t7, 48(a1)  // z2 = inptr[24]
    lh        s2, 16(a0)  // q0 = quantptr[ 8]
    lh        s4, 80(a0)  // q2 = quantptr[40]
    lh        s3, 48(a0)  // q1 = quantptr[24]
    mul       t2, t2, t9  // tmp2 = tmp2 * 5793
    mul       t1, t1, s0  // tmp1 = tmp1 * 10033
    sll       t0, t0, 13  // tmp0 = tmp0 << 13
    mul       t6, t6, s2  // z1 = z1 * q0
    mul       t8, t8, s4  // z3 = z3 * q2
    mul       t7, t7, s3  // z2 = z2 * q1
    addu      t3, t0, t2  // tmp10 = tmp0 + tmp2
    sll       t2, t2, 1   // tmp2 = tmp2 << 2
    subu      t4, t0, t2  // tmp11 = tmp0 - tmp2;
    subu      t5, t3, t1  // tmp12 = tmp10 - tmp1
    addu      t3, t3, t1  // tmp10 = tmp10 + tmp1
    addu      t1, t6, t8  // tmp1 = z1 + z3
    mul       t1, t1, s1  // tmp1 = tmp1 * 2998
    shra_r.w  t4, t4, 11  // tmp11 = (tmp11 + 1024) >> 11
    subu      t2, t6, t8  // tmp2 = z1 - z3
    subu      t2, t2, t7  // tmp2 = tmp2 - z2
    sll       t2, t2, 2   // tmp2 = tmp2 << 2
    addu      t0, t6, t7  // tmp0 = z1 + z2
    sll       t0, t0, 13  // tmp0 = tmp0 << 13
    subu      s2, t8, t7  // q0 = z3 - z2
    sll       s2, s2, 13  // q0 = q0 << 13
    addu      t0, t0, t1  // tmp0 = tmp0 + tmp1
    addu      t1, s2, t1  // tmp1 = q0 + tmp1
    addu      s2, t4, t2  // q0 = tmp11 + tmp2
    subu      s3, t4, t2  // q1 = tmp11 - tmp2
    addu      t6, t3, t0  // z1 = tmp10 + tmp0
    subu      t7, t3, t0  // z2 = tmp10 - tmp0
    addu      t4, t5, t1  // tmp11 = tmp12 + tmp1
    subu      t5, t5, t1  // tmp12 = tmp12 - tmp1
    shra_r.w  t6, t6, 11  // z1 = (z1 + 1024) >> 11
    shra_r.w  t7, t7, 11  // z2 = (z2 + 1024) >> 11
    shra_r.w  t4, t4, 11  // tmp11 = (tmp11 + 1024) >> 11
    shra_r.w  t5, t5, 11  // tmp12 = (tmp12 + 1024) >> 11
    sw        s2, 24(v0)
    sw        s3, 96(v0)
    sw        t6, 0(v0)
    sw        t7, 120(v0)
    sw        t4, 48(v0)
    sw        t5, 72(v0)
    addiu     v0, v0, 4
    addiu     a1, a1, 2
    bne       v0, v1, 1b
     addiu    a0, a0, 2

    /* Pass 2: process 6 rows from work array, store into output array. */
    move      v0, sp
    addiu     v1, v0, 144

2:
    lw        t0, 0(v0)
    lw        t2, 16(v0)
    lw        s5, 0(a2)
    addiu     t0, t0, 16
    sll       t0, t0, 13
    mul       t3, t2, t9
    lw        t6, 4(v0)
    lw        t8, 20(v0)
    lw        t7, 12(v0)
    addu      s5, s5, a3
    addu      s6, t6, t8
    mul       s6, s6, s1
    addu      t1, t0, t3
    subu      t4, t0, t3
    subu      t4, t4, t3
    lw        t3, 8(v0)
    mul       t0, t3, s0
    addu      s7, t6, t7
    sll       s7, s7, 13
    addu      s7, s6, s7
    subu      t2, t8, t7
    sll       t2, t2, 13
    addu      t2, s6, t2
    subu      s6, t6, t7
    subu      s6, s6, t8
    sll       s6, s6, 13
    addu      t3, t1, t0
    subu      t5, t1, t0
    addu      t6, t3, s7
    subu      t3, t3, s7
    addu      t7, t4, s6
    subu      t4, t4, s6
    addu      t8, t5, t2
    subu      t5, t5, t2
    shll_s.w  t6, t6, 6
    shll_s.w  t3, t3, 6
    shll_s.w  t7, t7, 6
    shll_s.w  t4, t4, 6
    shll_s.w  t8, t8, 6
    shll_s.w  t5, t5, 6
    sra       t6, t6, 24
    addiu     t6, t6, 128
    sra       t3, t3, 24
    addiu     t3, t3, 128
    sb        t6, 0(s5)
    sra       t7, t7, 24
    addiu     t7, t7, 128
    sb        t3, 5(s5)
    sra       t4, t4, 24
    addiu     t4, t4, 128
    sb        t7, 1(s5)
    sra       t8, t8, 24
    addiu     t8, t8, 128
    sb        t4, 4(s5)
    addiu     v0, v0, 24
    sra       t5, t5, 24
    addiu     t5, t5, 128
    sb        t8, 2(s5)
    addiu     a2, a2,  4
    bne       v0, v1, 2b
     sb       t5, 3(s5)

    addiu     sp, sp, 144

    RESTORE_REGS_FROM_STACK 32, s0, s1, s2, s3, s4, s5, s6, s7

    j         ra
     nop

END(jsimd_idct_6x6_mips_dspr2)

/*****************************************************************************/
LEAF_MIPS_DSPR2(jsimd_idct_12x12_pass1_mips_dspr2)
/*
 * a0     - compptr->dct_table
 * a1     - coef_block
 * a2     - workspace
 */

    SAVE_REGS_ON_STACK 16, s0, s1, s2, s3

    li         a3, 8

1:
    // odd part
    lh         t0, 48(a1)
    lh         t1, 48(a0)
    lh         t2, 16(a1)
    lh         t3, 16(a0)
    lh         t4, 80(a1)
    lh         t5, 80(a0)
    lh         t6, 112(a1)
    lh         t7, 112(a0)
    mul        t0, t0, t1    // z2
    mul        t1, t2, t3    // z1
    mul        t2, t4, t5    // z3
    mul        t3, t6, t7    // z4
    li         t4, 10703     // FIX(1.306562965)
    li         t5, 4433      // FIX_0_541196100
    li         t6, 7053      // FIX(0.860918669)
    mul        t4, t0,t4     // tmp11
    mul        t5, t0,t5     // -tmp14
    addu       t7, t1,t2     // tmp10
    addu       t8, t7,t3     // tmp10 + z4
    mul        t6, t6, t8    // tmp15
    li         t8, 2139      // FIX(0.261052384)
    mul        t8, t7, t8    // MULTIPLY(tmp10, FIX(0.261052384))
    li         t7, 2295      // FIX(0.280143716)
    mul        t7, t1, t7    // MULTIPLY(z1, FIX(0.280143716))
    addu       t9, t2, t3    // z3 + z4
    li         s0, 8565      // FIX(1.045510580)
    mul        t9, t9, s0    // -tmp13
    li         s0, 12112     // FIX(1.478575242)
    mul        s0, t2, s0    // MULTIPLY(z3, FIX(1.478575242)
    li         s1, 12998     // FIX(1.586706681)
    mul        s1, t3, s1    // MULTIPLY(z4, FIX(1.586706681))
    li         s2, 5540      // FIX(0.676326758)
    mul        s2, t1, s2    // MULTIPLY(z1, FIX(0.676326758))
    li         s3, 16244     // FIX(1.982889723)
    mul        s3, t3, s3    // MULTIPLY(z4, FIX(1.982889723))
    subu       t1, t1, t3    // z1-=z4
    subu       t0, t0, t2    // z2-=z3
    addu       t2, t0, t1    // z1+z2
    li         t3, 4433      // FIX_0_541196100
    mul        t2, t2, t3    // z3
    li         t3, 6270      // FIX_0_765366865
    mul        t1, t1, t3    // MULTIPLY(z1, FIX_0_765366865)
    li         t3, 15137     // FIX_0_765366865
    mul        t0, t0, t3    // MULTIPLY(z2, FIX_1_847759065)
    addu       t8, t6, t8    // tmp12
    addu       t3, t8, t4    // tmp12 + tmp11
    addu       t3, t3, t7    // tmp10
    subu       t8, t8, t9    // tmp12 + tmp13
    addu       s0, t5, s0
    subu       t8, t8, s0    // tmp12
    subu       t9, t6, t9
    subu       s1, s1, t4
    addu       t9, t9, s1    // tmp13
    subu       t6, t6, t5
    subu       t6, t6, s2
    subu       t6, t6, s3    // tmp15
    // even part start
    lh         t4, 64(a1)
    lh         t5, 64(a0)
    lh         t7, 32(a1)
    lh         s0, 32(a0)
    lh         s1, 0(a1)
    lh         s2, 0(a0)
    lh         s3, 96(a1)
    lh         v0, 96(a0)
    mul        t4, t4, t5    // DEQUANTIZE(inptr[DCTSIZE*4],quantptr[DCTSIZE*4])
    mul        t5, t7, s0    // DEQUANTIZE(inptr[DCTSIZE*2],quantptr[DCTSIZE*2])
    mul        t7, s1, s2    // DEQUANTIZE(inptr[DCTSIZE*0],quantptr[DCTSIZE*0])
    mul        s0, s3, v0    // DEQUANTIZE(inptr[DCTSIZE*6],quantptr[DCTSIZE*6])
    // odd part end
    addu       t1, t2, t1    // tmp11
    subu       t0, t2, t0    // tmp14
    // update counter and pointers
    addiu      a3, a3, -1
    addiu      a0, a0, 2
    addiu      a1, a1, 2
    // even part rest
    li         s1, 10033
    li         s2, 11190
    mul        t4, t4, s1    // z4
    mul        s1, t5, s2    // z4
    sll        t5, t5, 13    // z1
    sll        t7, t7, 13
    addiu      t7, t7, 1024  // z3
    sll        s0, s0, 13    // z2
    addu       s2, t7, t4    // tmp10
    subu       t4, t7, t4    // tmp11
    subu       s3, t5, s0    // tmp12
    addu       t2, t7, s3    // tmp21
    subu       s3, t7, s3    // tmp24
    addu       t7, s1, s0    // tmp12
    addu       v0, s2, t7    // tmp20
    subu       s2, s2, t7    // tmp25
    subu       s1, s1, t5    // z4 - z1
    subu       s1, s1, s0    // tmp12
    addu       s0, t4, s1    // tmp22
    subu       t4, t4, s1    // tmp23
    // final output stage
    addu       t5, v0, t3
    subu       v0, v0, t3
    addu       t3, t2, t1
    subu       t2, t2, t1
    addu       t1, s0, t8
    subu       s0, s0, t8
    addu       t8, t4, t9
    subu       t4, t4, t9
    addu       t9, s3, t0
    subu       s3, s3, t0
    addu       t0, s2, t6
    subu       s2, s2, t6
    sra        t5, t5, 11
    sra        t3, t3, 11
    sra        t1, t1, 11
    sra        t8, t8, 11
    sra        t9, t9, 11
    sra        t0, t0, 11
    sra        s2, s2, 11
    sra        s3, s3, 11
    sra        t4, t4, 11
    sra        s0, s0, 11
    sra        t2, t2, 11
    sra        v0, v0, 11
    sw         t5, 0(a2)
    sw         t3, 32(a2)
    sw         t1, 64(a2)
    sw         t8, 96(a2)
    sw         t9, 128(a2)
    sw         t0, 160(a2)
    sw         s2, 192(a2)
    sw         s3, 224(a2)
    sw         t4, 256(a2)
    sw         s0, 288(a2)
    sw         t2, 320(a2)
    sw         v0, 352(a2)
    bgtz       a3, 1b
     addiu     a2, a2, 4

    RESTORE_REGS_FROM_STACK 16, s0, s1, s2, s3

    j          ra
     nop

END(jsimd_idct_12x12_pass1_mips_dspr2)

/*****************************************************************************/
LEAF_MIPS_DSPR2(jsimd_idct_12x12_pass2_mips_dspr2)
/*
 * a0     - workspace
 * a1     - output
 */

    SAVE_REGS_ON_STACK 16, s0, s1, s2, s3

    li        a3, 12

1:
    // Odd part
    lw        t0, 12(a0)
    lw        t1, 4(a0)
    lw        t2, 20(a0)
    lw        t3, 28(a0)
    li        t4, 10703     // FIX(1.306562965)
    li        t5, 4433      // FIX_0_541196100
    mul       t4, t0, t4    // tmp11
    mul       t5, t0, t5    // -tmp14
    addu      t6, t1, t2    // tmp10
    li        t7, 2139      // FIX(0.261052384)
    mul       t7, t6, t7    // MULTIPLY(tmp10, FIX(0.261052384))
    addu      t6, t6, t3    // tmp10 + z4
    li        t8, 7053      // FIX(0.860918669)
    mul       t6, t6, t8    // tmp15
    li        t8, 2295      // FIX(0.280143716)
    mul       t8, t1, t8    // MULTIPLY(z1, FIX(0.280143716))
    addu      t9, t2, t3    // z3 + z4
    li        s0, 8565      // FIX(1.045510580)
    mul       t9, t9, s0    // -tmp13
    li        s0, 12112     // FIX(1.478575242)
    mul       s0, t2, s0    // MULTIPLY(z3, FIX(1.478575242))
    li        s1, 12998     // FIX(1.586706681)
    mul       s1, t3, s1    // MULTIPLY(z4, FIX(1.586706681))
    li        s2, 5540      // FIX(0.676326758)
    mul       s2, t1, s2    // MULTIPLY(z1, FIX(0.676326758))
    li        s3, 16244     // FIX(1.982889723)
    mul       s3, t3, s3    // MULTIPLY(z4, FIX(1.982889723))
    subu      t1, t1, t3    // z1 -= z4
    subu      t0, t0, t2    // z2 -= z3
    addu      t2, t1, t0    // z1 + z2
    li        t3, 4433      // FIX_0_541196100
    mul       t2, t2, t3    // z3
    li        t3, 6270      // FIX_0_765366865
    mul       t1, t1, t3    // MULTIPLY(z1, FIX_0_765366865)
    li        t3, 15137     // FIX_1_847759065
    mul       t0, t0, t3    // MULTIPLY(z2, FIX_1_847759065)
    addu      t3, t6, t7    // tmp12
    addu      t7, t3, t4
    addu      t7, t7, t8    // tmp10
    subu      t3, t3, t9
    subu      t3, t3, t5
    subu      t3, t3, s0    // tmp12
    subu      t9, t6, t9
    subu      t9, t9, t4
    addu      t9, t9, s1    // tmp13
    subu      t6, t6, t5
    subu      t6, t6, s2
    subu      t6, t6, s3    // tmp15
    addu      t1, t2, t1    // tmp11
    subu      t0, t2, t0    // tmp14
    // even part
    lw        t2, 16(a0)    // z4
    lw        t4, 8(a0)     // z1
    lw        t5, 0(a0)     // z3
    lw        t8, 24(a0)    // z2
    li        s0, 10033     // FIX(1.224744871)
    li        s1, 11190     // FIX(1.366025404)
    mul       t2, t2, s0    // z4
    mul       s0, t4, s1    // z4
    addiu     t5, t5, 0x10
    sll       t5, t5, 13    // z3
    sll       t4, t4, 13    // z1
    sll       t8, t8, 13    // z2
    subu      s1, t4, t8    // tmp12
    addu      s2, t5, t2    // tmp10
    subu      t2, t5, t2    // tmp11
    addu      s3, t5, s1    // tmp21
    subu      s1, t5, s1    // tmp24
    addu      t5, s0, t8    // tmp12
    addu      v0, s2, t5    // tmp20
    subu      t5, s2, t5    // tmp25
    subu      t4, s0, t4
    subu      t4, t4, t8    // tmp12
    addu      t8, t2, t4    // tmp22
    subu      t2, t2, t4    // tmp23
    // increment counter and pointers
    addiu     a3, a3, -1
    addiu     a0, a0, 32
    // Final stage
    addu      t4, v0, t7
    subu      v0, v0, t7
    addu      t7, s3, t1
    subu      s3, s3, t1
    addu      t1, t8, t3
    subu      t8, t8, t3
    addu      t3, t2, t9
    subu      t2, t2, t9
    addu      t9, s1, t0
    subu      s1, s1, t0
    addu      t0, t5, t6
    subu      t5, t5, t6
    sll       t4, t4, 4
    sll       t7, t7, 4
    sll       t1, t1, 4
    sll       t3, t3, 4
    sll       t9, t9, 4
    sll       t0, t0, 4
    sll       t5, t5, 4
    sll       s1, s1, 4
    sll       t2, t2, 4
    sll       t8, t8, 4
    sll       s3, s3, 4
    sll       v0, v0, 4
    shll_s.w  t4, t4, 2
    shll_s.w  t7, t7, 2
    shll_s.w  t1, t1, 2
    shll_s.w  t3, t3, 2
    shll_s.w  t9, t9, 2
    shll_s.w  t0, t0, 2
    shll_s.w  t5, t5, 2
    shll_s.w  s1, s1, 2
    shll_s.w  t2, t2, 2
    shll_s.w  t8, t8, 2
    shll_s.w  s3, s3, 2
    shll_s.w  v0, v0, 2
    srl       t4, t4, 24
    srl       t7, t7, 24
    srl       t1, t1, 24
    srl       t3, t3, 24
    srl       t9, t9, 24
    srl       t0, t0, 24
    srl       t5, t5, 24
    srl       s1, s1, 24
    srl       t2, t2, 24
    srl       t8, t8, 24
    srl       s3, s3, 24
    srl       v0, v0, 24
    lw        t6, 0(a1)
    addiu     t4, t4, 0x80
    addiu     t7, t7, 0x80
    addiu     t1, t1, 0x80
    addiu     t3, t3, 0x80
    addiu     t9, t9, 0x80
    addiu     t0, t0, 0x80
    addiu     t5, t5, 0x80
    addiu     s1, s1, 0x80
    addiu     t2, t2, 0x80
    addiu     t8, t8, 0x80
    addiu     s3, s3, 0x80
    addiu     v0, v0, 0x80
    sb        t4, 0(t6)
    sb        t7, 1(t6)
    sb        t1, 2(t6)
    sb        t3, 3(t6)
    sb        t9, 4(t6)
    sb        t0, 5(t6)
    sb        t5, 6(t6)
    sb        s1, 7(t6)
    sb        t2, 8(t6)
    sb        t8, 9(t6)
    sb        s3, 10(t6)
    sb        v0, 11(t6)
    bgtz      a3, 1b
     addiu    a1, a1, 4

    RESTORE_REGS_FROM_STACK 16, s0, s1, s2, s3

    jr        ra
     nop

END(jsimd_idct_12x12_pass2_mips_dspr2)

/*****************************************************************************/