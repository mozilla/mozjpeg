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
