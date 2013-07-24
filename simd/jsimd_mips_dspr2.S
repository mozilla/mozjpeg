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
