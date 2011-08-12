/*
 * ARM NEON optimizations for libjpeg-turbo
 *
 * Copyright (C) 2009-2011 Nokia Corporation and/or its subsidiary(-ies).
 * All rights reserved.
 * Contact: Alexander Bokovoy <alexander.bokovoy@nokia.com>
 *
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

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits /* mark stack as non-executable */
#endif

.text
.fpu neon
.arch armv7a
.object_arch armv4
.arm


#define RESPECT_STRICT_ALIGNMENT 1

/*****************************************************************************/

/* Supplementary macro for setting function attributes */
.macro asm_function fname
#ifdef __APPLE__
    .func _\fname
    .globl _\fname
_\fname:
#else
    .func \fname
    .global \fname
#ifdef __ELF__
    .hidden \fname
    .type \fname, %function
#endif
\fname:
#endif
.endm

/* Transpose a block of 4x4 coefficients in four 64-bit registers */
.macro transpose_4x4 x0, x1, x2, x3
    vtrn.16 \x0, \x1
    vtrn.16 \x2, \x3
    vtrn.32 \x0, \x2
    vtrn.32 \x1, \x3
.endm

/*****************************************************************************/

/*
 * jsimd_idct_ifast_neon
 *
 * This function contains a fast, not so accurate integer implementation of
 * the inverse DCT (Discrete Cosine Transform). It uses the same calculations
 * and produces exactly the same output as IJG's original 'jpeg_idct_fast'
 * function from jidctfst.c
 *
 * TODO: a bit better instructions scheduling is needed.
 */

#define XFIX_1_082392200 d0[0]
#define XFIX_1_414213562 d0[1]
#define XFIX_1_847759065 d0[2]
#define XFIX_2_613125930 d0[3]

.balign 16
jsimd_idct_ifast_neon_consts:
    .short (277 * 128 - 256 * 128) /* XFIX_1_082392200 */
    .short (362 * 128 - 256 * 128) /* XFIX_1_414213562 */
    .short (473 * 128 - 256 * 128) /* XFIX_1_847759065 */
    .short (669 * 128 - 512 * 128) /* XFIX_2_613125930 */

/* 1-D IDCT helper macro */

.macro idct_helper  x0, x1, x2, x3, x4, x5, x6, x7, \
                    t10, t11, t12, t13, t14

    vsub.s16        \t10, \x0, \x4
    vadd.s16        \x4,  \x0, \x4
    vswp.s16        \t10, \x0
    vsub.s16        \t11, \x2, \x6
    vadd.s16        \x6,  \x2, \x6
    vswp.s16        \t11, \x2
    vsub.s16        \t10, \x3, \x5
    vadd.s16        \x5,  \x3, \x5
    vswp.s16        \t10, \x3
    vsub.s16        \t11, \x1, \x7
    vadd.s16        \x7,  \x1, \x7
    vswp.s16        \t11, \x1

    vqdmulh.s16     \t13, \x2,  d0[1]
    vadd.s16        \t12, \x3,  \x3
    vadd.s16        \x2,  \x2,  \t13
    vqdmulh.s16     \t13, \x3,  d0[3]
    vsub.s16        \t10,  \x1, \x3
    vadd.s16        \t12, \t12, \t13
    vqdmulh.s16     \t13, \t10, d0[2]
    vsub.s16        \t11, \x7,  \x5
    vadd.s16        \t10, \t10, \t13
    vqdmulh.s16     \t13, \t11, d0[1]
    vadd.s16        \t11, \t11, \t13

    vqdmulh.s16     \t13, \x1,  d0[0]
    vsub.s16        \x2,  \x6,  \x2
    vsub.s16        \t14, \x0,  \x2
    vadd.s16        \x2,  \x0,  \x2
    vadd.s16        \x0,  \x4,  \x6
    vsub.s16        \x4,  \x4,  \x6
    vadd.s16        \x1,  \x1,  \t13
    vadd.s16        \t13, \x7,  \x5
    vsub.s16        \t12, \t13, \t12
    vsub.s16        \t12, \t12, \t10
    vadd.s16        \t11, \t12, \t11
    vsub.s16        \t10, \x1,  \t10
    vadd.s16        \t10, \t10, \t11

    vsub.s16        \x7,  \x0,  \t13
    vadd.s16        \x0,  \x0,  \t13
    vadd.s16        \x6,  \t14, \t12
    vsub.s16        \x1,  \t14, \t12
    vsub.s16        \x5,  \x2,  \t11
    vadd.s16        \x2,  \x2,  \t11
    vsub.s16        \x3,  \x4,  \t10
    vadd.s16        \x4,  \x4,  \t10
.endm

asm_function jsimd_idct_ifast_neon

    DCT_TABLE       .req r0
    COEF_BLOCK      .req r1
    OUTPUT_BUF      .req r2
    OUTPUT_COL      .req r3
    TMP             .req ip

    vpush           {d8-d15}

    /* Load constants */
    adr             TMP, jsimd_idct_ifast_neon_consts
    vld1.16         {d0}, [TMP, :64]

    /* Load all COEF_BLOCK into NEON registers with the following allocation:
     *       0 1 2 3 | 4 5 6 7
     *      ---------+--------
     *   0 | d4      | d5
     *   1 | d6      | d7
     *   2 | d8      | d9
     *   3 | d10     | d11
     *   4 | d12     | d13
     *   5 | d14     | d15
     *   6 | d16     | d17
     *   7 | d18     | d19
     */
    vld1.16         {d4, d5, d6, d7}, [COEF_BLOCK]!
    vld1.16         {d8, d9, d10, d11}, [COEF_BLOCK]!
    vld1.16         {d12, d13, d14, d15}, [COEF_BLOCK]!
    vld1.16         {d16, d17, d18, d19}, [COEF_BLOCK]!
    /* Dequantize */
    vld1.16         {d20, d21, d22, d23}, [DCT_TABLE]!
    vmul.s16        q2, q2, q10
    vld1.16         {d24, d25, d26, d27}, [DCT_TABLE]!
    vmul.s16        q3, q3, q11
    vmul.s16        q4, q4, q12
    vld1.16         {d28, d29, d30, d31}, [DCT_TABLE]!
    vmul.s16        q5, q5, q13
    vmul.s16        q6, q6, q14
    vld1.16         {d20, d21, d22, d23}, [DCT_TABLE]!
    vmul.s16        q7, q7, q15
    vmul.s16        q8, q8, q10
    vmul.s16        q9, q9, q11

    /* Pass 1 */
    idct_helper     q2, q3, q4, q5, q6, q7, q8, q9, q10, q11, q12, q13, q14
    /* Transpose */
    transpose_4x4   d4,  d6,  d8,  d10
    transpose_4x4   d5,  d7,  d9,  d11
    transpose_4x4   d12, d14, d16, d18
    transpose_4x4   d13, d15, d17, d19
    vswp            d12, d5
    vswp            d14, d7
    vswp            d16, d9
    vswp            d18, d11

    /* Pass 2 */
    idct_helper     q2, q3, q4, q5, q6, q7, q8, q9, q10, q11, q12, q13, q14
    /* Transpose */
    transpose_4x4   d4,  d6,  d8,  d10
    transpose_4x4   d5,  d7,  d9,  d11
    transpose_4x4   d12, d14, d16, d18
    transpose_4x4   d13, d15, d17, d19
    vswp            d12, d5
    vswp            d14, d7
    vswp            d16, d9
    vswp            d18, d11

    /* Descale and range limit */
    vmov.s16        q15, #(0x80 << 5)
    vqadd.s16       q2, q2, q15
    vqadd.s16       q3, q3, q15
    vqadd.s16       q4, q4, q15
    vqadd.s16       q5, q5, q15
    vqadd.s16       q6, q6, q15
    vqadd.s16       q7, q7, q15
    vqadd.s16       q8, q8, q15
    vqadd.s16       q9, q9, q15
    vqshrun.s16     d4, q2, #5
    vqshrun.s16     d6, q3, #5
    vqshrun.s16     d8, q4, #5
    vqshrun.s16     d10, q5, #5
    vqshrun.s16     d12, q6, #5
    vqshrun.s16     d14, q7, #5
    vqshrun.s16     d16, q8, #5
    vqshrun.s16     d18, q9, #5

    /* Store results to the output buffer */
    .irp            x, d4, d6, d8, d10, d12, d14, d16, d18
    ldr             TMP, [OUTPUT_BUF], #4
    add             TMP, TMP, OUTPUT_COL
    vst1.8          {\x}, [TMP]!
    .endr

    vpop            {d8-d15}
    bx              lr

    .unreq          DCT_TABLE
    .unreq          COEF_BLOCK
    .unreq          OUTPUT_BUF
    .unreq          OUTPUT_COL
    .unreq          TMP
.endfunc

.purgem idct_helper

/*****************************************************************************/

/*
 * jsimd_idct_4x4_neon
 *
 * This function contains inverse-DCT code for getting reduced-size
 * 4x4 pixels output from an 8x8 DCT block. It uses the same  calculations
 * and produces exactly the same output as IJG's original 'jpeg_idct_4x4'
 * function from jpeg-6b (jidctred.c).
 *
 * NOTE: jpeg-8 has an improved implementation of 4x4 inverse-DCT, which
 *       requires much less arithmetic operations and hence should be faster.
 *       The primary purpose of this particular NEON optimized function is
 *       bit exact compatibility with jpeg-6b.
 *
 * TODO: a bit better instructions scheduling can be achieved by expanding
 *       idct_helper/transpose_4x4 macros and reordering instructions,
 *       but readability will suffer somewhat.
 */

#define CONST_BITS  13

#define FIX_0_211164243  (1730)  /* FIX(0.211164243) */
#define FIX_0_509795579  (4176)  /* FIX(0.509795579) */
#define FIX_0_601344887  (4926)  /* FIX(0.601344887) */
#define FIX_0_720959822  (5906)  /* FIX(0.720959822) */
#define FIX_0_765366865  (6270)  /* FIX(0.765366865) */
#define FIX_0_850430095  (6967)  /* FIX(0.850430095) */
#define FIX_0_899976223  (7373)  /* FIX(0.899976223) */
#define FIX_1_061594337  (8697)  /* FIX(1.061594337) */
#define FIX_1_272758580  (10426) /* FIX(1.272758580) */
#define FIX_1_451774981  (11893) /* FIX(1.451774981) */
#define FIX_1_847759065  (15137) /* FIX(1.847759065) */
#define FIX_2_172734803  (17799) /* FIX(2.172734803) */
#define FIX_2_562915447  (20995) /* FIX(2.562915447) */
#define FIX_3_624509785  (29692) /* FIX(3.624509785) */

.balign 16
jsimd_idct_4x4_neon_consts:
    .short     FIX_1_847759065     /* d0[0] */
    .short     -FIX_0_765366865    /* d0[1] */
    .short     -FIX_0_211164243    /* d0[2] */
    .short     FIX_1_451774981     /* d0[3] */
    .short     -FIX_2_172734803    /* d1[0] */
    .short     FIX_1_061594337     /* d1[1] */
    .short     -FIX_0_509795579    /* d1[2] */
    .short     -FIX_0_601344887    /* d1[3] */
    .short     FIX_0_899976223     /* d2[0] */
    .short     FIX_2_562915447     /* d2[1] */
    .short     1 << (CONST_BITS+1) /* d2[2] */
    .short     0                   /* d2[3] */

.macro idct_helper x4, x6, x8, x10, x12, x14, x16, shift, y26, y27, y28, y29
    vmull.s16       q14, \x4,  d2[2]
    vmlal.s16       q14, \x8,  d0[0]
    vmlal.s16       q14, \x14, d0[1]

    vmull.s16       q13, \x16, d1[2]
    vmlal.s16       q13, \x12, d1[3]
    vmlal.s16       q13, \x10, d2[0]
    vmlal.s16       q13, \x6,  d2[1]

    vmull.s16       q15, \x4,  d2[2]
    vmlsl.s16       q15, \x8,  d0[0]
    vmlsl.s16       q15, \x14, d0[1]

    vmull.s16       q12, \x16, d0[2]
    vmlal.s16       q12, \x12, d0[3]
    vmlal.s16       q12, \x10, d1[0]
    vmlal.s16       q12, \x6,  d1[1]

    vadd.s32        q10, q14, q13
    vsub.s32        q14, q14, q13

.if \shift > 16
    vrshr.s32       q10,  q10, #\shift
    vrshr.s32       q14,  q14, #\shift
    vmovn.s32       \y26, q10
    vmovn.s32       \y29, q14
.else
    vrshrn.s32      \y26, q10, #\shift
    vrshrn.s32      \y29, q14, #\shift
.endif

    vadd.s32        q10, q15, q12
    vsub.s32        q15, q15, q12

.if \shift > 16
    vrshr.s32       q10,  q10, #\shift
    vrshr.s32       q15,  q15, #\shift
    vmovn.s32       \y27, q10
    vmovn.s32       \y28, q15
.else
    vrshrn.s32      \y27, q10, #\shift
    vrshrn.s32      \y28, q15, #\shift
.endif

.endm

asm_function jsimd_idct_4x4_neon

    DCT_TABLE       .req r0
    COEF_BLOCK      .req r1
    OUTPUT_BUF      .req r2
    OUTPUT_COL      .req r3
    TMP1            .req r0
    TMP2            .req r1
    TMP3            .req r2
    TMP4            .req ip

    vpush           {d8-d15}

    /* Load constants (d3 is just used for padding) */
    adr             TMP4, jsimd_idct_4x4_neon_consts
    vld1.16         {d0, d1, d2, d3}, [TMP4, :128]

    /* Load all COEF_BLOCK into NEON registers with the following allocation:
     *       0 1 2 3 | 4 5 6 7
     *      ---------+--------
     *   0 | d4      | d5
     *   1 | d6      | d7
     *   2 | d8      | d9
     *   3 | d10     | d11
     *   4 | -       | -
     *   5 | d12     | d13
     *   6 | d14     | d15
     *   7 | d16     | d17
     */
    vld1.16         {d4, d5, d6, d7}, [COEF_BLOCK, :128]!
    vld1.16         {d8, d9, d10, d11}, [COEF_BLOCK, :128]!
    add COEF_BLOCK, COEF_BLOCK, #16
    vld1.16         {d12, d13, d14, d15}, [COEF_BLOCK, :128]!
    vld1.16         {d16, d17}, [COEF_BLOCK, :128]!
    /* dequantize */
    vld1.16         {d18, d19, d20, d21}, [DCT_TABLE, :128]!
    vmul.s16        q2, q2, q9
    vld1.16         {d22, d23, d24, d25}, [DCT_TABLE, :128]!
    vmul.s16        q3, q3, q10
    vmul.s16        q4, q4, q11
    add             DCT_TABLE, DCT_TABLE, #16
    vld1.16         {d26, d27, d28, d29}, [DCT_TABLE, :128]!
    vmul.s16        q5, q5, q12
    vmul.s16        q6, q6, q13
    vld1.16         {d30, d31}, [DCT_TABLE, :128]!
    vmul.s16        q7, q7, q14
    vmul.s16        q8, q8, q15

    /* Pass 1 */
    idct_helper     d4, d6, d8, d10, d12, d14, d16, 12, d4, d6, d8, d10
    transpose_4x4   d4, d6, d8, d10
    idct_helper     d5, d7, d9, d11, d13, d15, d17, 12, d5, d7, d9, d11
    transpose_4x4   d5, d7, d9, d11

    /* Pass 2 */
    idct_helper     d4, d6, d8, d10, d7, d9, d11, 19, d26, d27, d28, d29
    transpose_4x4   d26, d27, d28, d29

    /* Range limit */
    vmov.u16        q15, #0x80
    vadd.s16        q13, q13, q15
    vadd.s16        q14, q14, q15
    vqmovun.s16     d26, q13
    vqmovun.s16     d27, q14

    /* Store results to the output buffer */
    ldmia           OUTPUT_BUF, {TMP1, TMP2, TMP3, TMP4}
    add             TMP1, TMP1, OUTPUT_COL
    add             TMP2, TMP2, OUTPUT_COL
    add             TMP3, TMP3, OUTPUT_COL
    add             TMP4, TMP4, OUTPUT_COL

#if defined(__ARMEL__) && !RESPECT_STRICT_ALIGNMENT
    /* We can use much less instructions on little endian systems if the
     * OS kernel is not configured to trap unaligned memory accesses
     */
    vst1.32         {d26[0]}, [TMP1]!
    vst1.32         {d27[0]}, [TMP3]!
    vst1.32         {d26[1]}, [TMP2]!
    vst1.32         {d27[1]}, [TMP4]!
#else
    vst1.8          {d26[0]}, [TMP1]!
    vst1.8          {d27[0]}, [TMP3]!
    vst1.8          {d26[1]}, [TMP1]!
    vst1.8          {d27[1]}, [TMP3]!
    vst1.8          {d26[2]}, [TMP1]!
    vst1.8          {d27[2]}, [TMP3]!
    vst1.8          {d26[3]}, [TMP1]!
    vst1.8          {d27[3]}, [TMP3]!

    vst1.8          {d26[4]}, [TMP2]!
    vst1.8          {d27[4]}, [TMP4]!
    vst1.8          {d26[5]}, [TMP2]!
    vst1.8          {d27[5]}, [TMP4]!
    vst1.8          {d26[6]}, [TMP2]!
    vst1.8          {d27[6]}, [TMP4]!
    vst1.8          {d26[7]}, [TMP2]!
    vst1.8          {d27[7]}, [TMP4]!
#endif

    vpop            {d8-d15}
    bx              lr

    .unreq          DCT_TABLE
    .unreq          COEF_BLOCK
    .unreq          OUTPUT_BUF
    .unreq          OUTPUT_COL
    .unreq          TMP1
    .unreq          TMP2
    .unreq          TMP3
    .unreq          TMP4
.endfunc

.purgem idct_helper

/*****************************************************************************/

/*
 * jsimd_idct_2x2_neon
 *
 * This function contains inverse-DCT code for getting reduced-size
 * 2x2 pixels output from an 8x8 DCT block. It uses the same  calculations
 * and produces exactly the same output as IJG's original 'jpeg_idct_2x2'
 * function from jpeg-6b (jidctred.c).
 *
 * NOTE: jpeg-8 has an improved implementation of 2x2 inverse-DCT, which
 *       requires much less arithmetic operations and hence should be faster.
 *       The primary purpose of this particular NEON optimized function is
 *       bit exact compatibility with jpeg-6b.
 */

.balign 8
jsimd_idct_2x2_neon_consts:
    .short     -FIX_0_720959822    /* d0[0] */
    .short     FIX_0_850430095     /* d0[1] */
    .short     -FIX_1_272758580    /* d0[2] */
    .short     FIX_3_624509785     /* d0[3] */

.macro idct_helper x4, x6, x10, x12, x16, shift, y26, y27
    vshll.s16  q14,  \x4,  #15
    vmull.s16  q13,  \x6,  d0[3]
    vmlal.s16  q13,  \x10, d0[2]
    vmlal.s16  q13,  \x12, d0[1]
    vmlal.s16  q13,  \x16, d0[0]

    vadd.s32   q10,  q14,  q13
    vsub.s32   q14,  q14,  q13

.if \shift > 16
    vrshr.s32  q10,  q10,  #\shift
    vrshr.s32  q14,  q14,  #\shift
    vmovn.s32  \y26, q10
    vmovn.s32  \y27, q14
.else
    vrshrn.s32 \y26, q10,  #\shift
    vrshrn.s32 \y27, q14,  #\shift
.endif

.endm

asm_function jsimd_idct_2x2_neon

    DCT_TABLE       .req r0
    COEF_BLOCK      .req r1
    OUTPUT_BUF      .req r2
    OUTPUT_COL      .req r3
    TMP1            .req r0
    TMP2            .req ip

    vpush           {d8-d15}

    /* Load constants */
    adr             TMP2, jsimd_idct_2x2_neon_consts
    vld1.16         {d0}, [TMP2, :64]

    /* Load all COEF_BLOCK into NEON registers with the following allocation:
     *       0 1 2 3 | 4 5 6 7
     *      ---------+--------
     *   0 | d4      | d5
     *   1 | d6      | d7
     *   2 | -       | -
     *   3 | d10     | d11
     *   4 | -       | -
     *   5 | d12     | d13
     *   6 | -       | -
     *   7 | d16     | d17
     */
    vld1.16         {d4, d5, d6, d7}, [COEF_BLOCK, :128]!
    add             COEF_BLOCK, COEF_BLOCK, #16
    vld1.16         {d10, d11}, [COEF_BLOCK, :128]!
    add             COEF_BLOCK, COEF_BLOCK, #16
    vld1.16         {d12, d13}, [COEF_BLOCK, :128]!
    add             COEF_BLOCK, COEF_BLOCK, #16
    vld1.16         {d16, d17}, [COEF_BLOCK, :128]!
    /* Dequantize */
    vld1.16         {d18, d19, d20, d21}, [DCT_TABLE, :128]!
    vmul.s16        q2, q2, q9
    vmul.s16        q3, q3, q10
    add             DCT_TABLE, DCT_TABLE, #16
    vld1.16         {d24, d25}, [DCT_TABLE, :128]!
    vmul.s16        q5, q5, q12
    add             DCT_TABLE, DCT_TABLE, #16
    vld1.16         {d26, d27}, [DCT_TABLE, :128]!
    vmul.s16        q6, q6, q13
    add             DCT_TABLE, DCT_TABLE, #16
    vld1.16         {d30, d31}, [DCT_TABLE, :128]!
    vmul.s16        q8, q8, q15

    /* Pass 1 */
#if 0
    idct_helper     d4, d6, d10, d12, d16, 13, d4, d6
    transpose_4x4   d4, d6, d8,  d10
    idct_helper     d5, d7, d11, d13, d17, 13, d5, d7
    transpose_4x4   d5, d7, d9,  d11
#else
    vmull.s16       q13, d6,  d0[3]
    vmlal.s16       q13, d10, d0[2]
    vmlal.s16       q13, d12, d0[1]
    vmlal.s16       q13, d16, d0[0]
    vmull.s16       q12, d7,  d0[3]
    vmlal.s16       q12, d11, d0[2]
    vmlal.s16       q12, d13, d0[1]
    vmlal.s16       q12, d17, d0[0]
    vshll.s16       q14, d4,  #15
    vshll.s16       q15, d5,  #15
    vadd.s32        q10, q14, q13
    vsub.s32        q14, q14, q13
    vrshrn.s32      d4,  q10, #13
    vrshrn.s32      d6,  q14, #13
    vadd.s32        q10, q15, q12
    vsub.s32        q14, q15, q12
    vrshrn.s32      d5,  q10, #13
    vrshrn.s32      d7,  q14, #13
    vtrn.16         q2,  q3
    vtrn.32         q3,  q5
#endif

    /* Pass 2 */
    idct_helper     d4, d6, d10, d7, d11, 20, d26, d27

    /* Range limit */
    vmov.u16        q15, #0x80
    vadd.s16        q13, q13, q15
    vqmovun.s16     d26, q13
    vqmovun.s16     d27, q13

    /* Store results to the output buffer */
    ldmia           OUTPUT_BUF, {TMP1, TMP2}
    add             TMP1, TMP1, OUTPUT_COL
    add             TMP2, TMP2, OUTPUT_COL

    vst1.8          {d26[0]}, [TMP1]!
    vst1.8          {d27[4]}, [TMP1]!
    vst1.8          {d26[1]}, [TMP2]!
    vst1.8          {d27[5]}, [TMP2]!

    vpop            {d8-d15}
    bx              lr

    .unreq          DCT_TABLE
    .unreq          COEF_BLOCK
    .unreq          OUTPUT_BUF
    .unreq          OUTPUT_COL
    .unreq          TMP1
    .unreq          TMP2
.endfunc

.purgem idct_helper

/*****************************************************************************/

/*
 * jsimd_ycc_extrgb_convert_neon
 * jsimd_ycc_extbgr_convert_neon
 * jsimd_ycc_extrgbx_convert_neon
 * jsimd_ycc_extbgrx_convert_neon
 * jsimd_ycc_extxbgr_convert_neon
 * jsimd_ycc_extxrgb_convert_neon
 *
 * Colorspace conversion YCbCr -> RGB
 */


.macro do_load size
    .if \size == 8
        vld1.8  {d4}, [U]!
        vld1.8  {d5}, [V]!
        vld1.8  {d0}, [Y]!
        pld     [Y, #64]
        pld     [U, #64]
        pld     [V, #64]
    .elseif \size == 4
        vld1.8  {d4[0]}, [U]!
        vld1.8  {d4[1]}, [U]!
        vld1.8  {d4[2]}, [U]!
        vld1.8  {d4[3]}, [U]!
        vld1.8  {d5[0]}, [V]!
        vld1.8  {d5[1]}, [V]!
        vld1.8  {d5[2]}, [V]!
        vld1.8  {d5[3]}, [V]!
        vld1.8  {d0[0]}, [Y]!
        vld1.8  {d0[1]}, [Y]!
        vld1.8  {d0[2]}, [Y]!
        vld1.8  {d0[3]}, [Y]!
    .elseif \size == 2
        vld1.8  {d4[4]}, [U]!
        vld1.8  {d4[5]}, [U]!
        vld1.8  {d5[4]}, [V]!
        vld1.8  {d5[5]}, [V]!
        vld1.8  {d0[4]}, [Y]!
        vld1.8  {d0[5]}, [Y]!
    .elseif \size == 1
        vld1.8  {d4[6]}, [U]!
        vld1.8  {d5[6]}, [V]!
        vld1.8  {d0[6]}, [Y]!
    .else
        .error unsupported macroblock size
    .endif
.endm

.macro do_store bpp, size
    .if \bpp == 24
        .if \size == 8
            vst3.8  {d10, d11, d12}, [RGB]!
        .elseif \size == 4
            vst3.8  {d10[0], d11[0], d12[0]}, [RGB]!
            vst3.8  {d10[1], d11[1], d12[1]}, [RGB]!
            vst3.8  {d10[2], d11[2], d12[2]}, [RGB]!
            vst3.8  {d10[3], d11[3], d12[3]}, [RGB]!
        .elseif \size == 2
            vst3.8  {d10[4], d11[4], d12[4]}, [RGB]!
            vst3.8  {d10[5], d11[5], d12[5]}, [RGB]!
        .elseif \size == 1
            vst3.8  {d10[6], d11[6], d12[6]}, [RGB]!
        .else
            .error unsupported macroblock size
        .endif
    .elseif \bpp == 32
        .if \size == 8
            vst4.8  {d10, d11, d12, d13}, [RGB]!
        .elseif \size == 4
            vst4.8  {d10[0], d11[0], d12[0], d13[0]}, [RGB]!
            vst4.8  {d10[1], d11[1], d12[1], d13[1]}, [RGB]!
            vst4.8  {d10[2], d11[2], d12[2], d13[2]}, [RGB]!
            vst4.8  {d10[3], d11[3], d12[3], d13[3]}, [RGB]!
        .elseif \size == 2
            vst4.8  {d10[4], d11[4], d12[4], d13[4]}, [RGB]!
            vst4.8  {d10[5], d11[5], d12[5], d13[5]}, [RGB]!
        .elseif \size == 1
            vst4.8  {d10[6], d11[6], d12[6], d13[6]}, [RGB]!
        .else
            .error unsupported macroblock size
        .endif
    .else
        .error unsupported bpp
    .endif
.endm

.macro generate_jsimd_ycc_rgb_convert_neon colorid, bpp, r_offs, g_offs, b_offs

.macro do_yuv_to_rgb
    vaddw.u8        q3, q1, d4     /* q3 = u - 128 */
    vaddw.u8        q4, q1, d5     /* q2 = v - 128 */
    vmull.s16       q10, d6, d1[1] /* multiply by -11277 */
    vmlal.s16       q10, d8, d1[2] /* multiply by -23401 */
    vmull.s16       q11, d7, d1[1] /* multiply by -11277 */
    vmlal.s16       q11, d9, d1[2] /* multiply by -23401 */
    vmull.s16       q12, d8, d1[0] /* multiply by 22971 */
    vmull.s16       q13, d9, d1[0] /* multiply by 22971 */
    vmull.s16       q14, d6, d1[3] /* multiply by 29033 */
    vmull.s16       q15, d7, d1[3] /* multiply by 29033 */
    vrshrn.s32      d20, q10, #15
    vrshrn.s32      d21, q11, #15
    vrshrn.s32      d24, q12, #14
    vrshrn.s32      d25, q13, #14
    vrshrn.s32      d28, q14, #14
    vrshrn.s32      d29, q15, #14
    vaddw.u8        q10, q10, d0
    vaddw.u8        q12, q12, d0
    vaddw.u8        q14, q14, d0
    vqmovun.s16     d1\g_offs, q10
    vqmovun.s16     d1\r_offs, q12
    vqmovun.s16     d1\b_offs, q14
.endm

/* Apple gas crashes on adrl, work around that by using adr.
 * But this requires a copy of these constants for each function.
 */

.balign 16
jsimd_ycc_\colorid\()_neon_consts:
    .short          0,      0,     0,      0
    .short          22971, -11277, -23401, 29033
    .short          -128,  -128,   -128,   -128
    .short          -128,  -128,   -128,   -128

asm_function jsimd_ycc_\colorid\()_convert_neon
    OUTPUT_WIDTH    .req r0
    INPUT_BUF       .req r1
    INPUT_ROW       .req r2
    OUTPUT_BUF      .req r3
    NUM_ROWS        .req r4

    INPUT_BUF0      .req r5
    INPUT_BUF1      .req r6
    INPUT_BUF2      .req INPUT_BUF

    RGB             .req r7
    Y               .req r8
    U               .req r9
    V               .req r10
    N               .req ip

    /* Load constants to d1, d2, d3 (d0 is just used for padding) */
    adr             ip, jsimd_ycc_\colorid\()_neon_consts
    vld1.16         {d0, d1, d2, d3}, [ip, :128]

    /* Save ARM registers and handle input arguments */
    push            {r4, r5, r6, r7, r8, r9, r10, lr}
    ldr             NUM_ROWS, [sp, #(4 * 8)]
    ldr             INPUT_BUF0, [INPUT_BUF]
    ldr             INPUT_BUF1, [INPUT_BUF, #4]
    ldr             INPUT_BUF2, [INPUT_BUF, #8]
    .unreq          INPUT_BUF

    /* Save NEON registers */
    vpush           {d8-d15}

    /* Initially set d10, d11, d12, d13 to 0xFF */
    vmov.u8         q5, #255
    vmov.u8         q6, #255

    /* Outer loop over scanlines */
    cmp             NUM_ROWS, #1
    blt             9f
0:
    ldr             Y, [INPUT_BUF0, INPUT_ROW, lsl #2]
    ldr             U, [INPUT_BUF1, INPUT_ROW, lsl #2]
    mov             N, OUTPUT_WIDTH
    ldr             V, [INPUT_BUF2, INPUT_ROW, lsl #2]
    add             INPUT_ROW, INPUT_ROW, #1
    ldr             RGB, [OUTPUT_BUF], #4

    /* Inner loop over pixels */
    subs            N, N, #8
    blt             2f
1:
    do_load         8
    do_yuv_to_rgb
    do_store        \bpp, 8
    subs            N, N, #8
    bge             1b
    tst             N, #7
    beq             8f
2:
    tst             N, #4
    beq             3f
    do_load         4
3:
    tst             N, #2
    beq             4f
    do_load         2
4:
    tst             N, #1
    beq             5f
    do_load         1
5:
    do_yuv_to_rgb
    tst             N, #4
    beq             6f
    do_store        \bpp, 4
6:
    tst             N, #2
    beq             7f
    do_store        \bpp, 2
7:
    tst             N, #1
    beq             8f
    do_store        \bpp, 1
8:
    subs            NUM_ROWS, NUM_ROWS, #1
    bgt             0b
9:
    /* Restore all registers and return */
    vpop            {d8-d15}
    pop             {r4, r5, r6, r7, r8, r9, r10, pc}

    .unreq          OUTPUT_WIDTH
    .unreq          INPUT_ROW
    .unreq          OUTPUT_BUF
    .unreq          NUM_ROWS
    .unreq          INPUT_BUF0
    .unreq          INPUT_BUF1
    .unreq          INPUT_BUF2
    .unreq          RGB
    .unreq          Y
    .unreq          U
    .unreq          V
    .unreq          N
.endfunc

.purgem do_yuv_to_rgb

.endm

/*--------------------------------- id ----- bpp R  G  B */
generate_jsimd_ycc_rgb_convert_neon extrgb,  24, 0, 1, 2
generate_jsimd_ycc_rgb_convert_neon extbgr,  24, 2, 1, 0
generate_jsimd_ycc_rgb_convert_neon extrgbx, 32, 0, 1, 2
generate_jsimd_ycc_rgb_convert_neon extbgrx, 32, 2, 1, 0
generate_jsimd_ycc_rgb_convert_neon extxbgr, 32, 3, 2, 1
generate_jsimd_ycc_rgb_convert_neon extxrgb, 32, 1, 2, 3

.purgem do_load
.purgem do_store

/*****************************************************************************/

/*
 * jsimd_extrgb_ycc_convert_neon
 * jsimd_extbgr_ycc_convert_neon
 * jsimd_extrgbx_ycc_convert_neon
 * jsimd_extbgrx_ycc_convert_neon
 * jsimd_extxbgr_ycc_convert_neon
 * jsimd_extxrgb_ycc_convert_neon
 *
 * Colorspace conversion RGB -> YCbCr
 */

.macro do_store size
    .if \size == 8
        vst1.8  {d20}, [Y]!
        vst1.8  {d21}, [U]!
        vst1.8  {d22}, [V]!
    .elseif \size == 4
        vst1.8  {d20[0]}, [Y]!
        vst1.8  {d20[1]}, [Y]!
        vst1.8  {d20[2]}, [Y]!
        vst1.8  {d20[3]}, [Y]!
        vst1.8  {d21[0]}, [U]!
        vst1.8  {d21[1]}, [U]!
        vst1.8  {d21[2]}, [U]!
        vst1.8  {d21[3]}, [U]!
        vst1.8  {d22[0]}, [V]!
        vst1.8  {d22[1]}, [V]!
        vst1.8  {d22[2]}, [V]!
        vst1.8  {d22[3]}, [V]!
    .elseif \size == 2
        vst1.8  {d20[4]}, [Y]!
        vst1.8  {d20[5]}, [Y]!
        vst1.8  {d21[4]}, [U]!
        vst1.8  {d21[5]}, [U]!
        vst1.8  {d22[4]}, [V]!
        vst1.8  {d22[5]}, [V]!
    .elseif \size == 1
        vst1.8  {d20[6]}, [Y]!
        vst1.8  {d21[6]}, [U]!
        vst1.8  {d22[6]}, [V]!
    .else
        .error unsupported macroblock size
    .endif
.endm

.macro do_load bpp, size
    .if \bpp == 24
        .if \size == 8
            vld3.8  {d10, d11, d12}, [RGB]!
            pld     [RGB, #128]
        .elseif \size == 4
            vld3.8  {d10[0], d11[0], d12[0]}, [RGB]!
            vld3.8  {d10[1], d11[1], d12[1]}, [RGB]!
            vld3.8  {d10[2], d11[2], d12[2]}, [RGB]!
            vld3.8  {d10[3], d11[3], d12[3]}, [RGB]!
        .elseif \size == 2
            vld3.8  {d10[4], d11[4], d12[4]}, [RGB]!
            vld3.8  {d10[5], d11[5], d12[5]}, [RGB]!
        .elseif \size == 1
            vld3.8  {d10[6], d11[6], d12[6]}, [RGB]!
        .else
            .error unsupported macroblock size
        .endif
    .elseif \bpp == 32
        .if \size == 8
            vld4.8  {d10, d11, d12, d13}, [RGB]!
            pld     [RGB, #128]
        .elseif \size == 4
            vld4.8  {d10[0], d11[0], d12[0], d13[0]}, [RGB]!
            vld4.8  {d10[1], d11[1], d12[1], d13[1]}, [RGB]!
            vld4.8  {d10[2], d11[2], d12[2], d13[2]}, [RGB]!
            vld4.8  {d10[3], d11[3], d12[3], d13[3]}, [RGB]!
        .elseif \size == 2
            vld4.8  {d10[4], d11[4], d12[4], d13[4]}, [RGB]!
            vld4.8  {d10[5], d11[5], d12[5], d13[5]}, [RGB]!
        .elseif \size == 1
            vld4.8  {d10[6], d11[6], d12[6], d13[6]}, [RGB]!
        .else
            .error unsupported macroblock size
        .endif
    .else
        .error unsupported bpp
    .endif
.endm

.macro generate_jsimd_rgb_ycc_convert_neon colorid, bpp, r_offs, g_offs, b_offs

/*
 * 2 stage pipelined RGB->YCbCr conversion
 */

.macro do_rgb_to_yuv_stage1
    vmovl.u8    q2, d1\r_offs /* r = { d4, d5 } */
    vmovl.u8    q3, d1\g_offs /* g = { d6, d7 } */
    vmovl.u8    q4, d1\b_offs /* b = { d8, d9 } */
    vmull.u16   q7, d4, d0[0]
    vmlal.u16   q7, d6, d0[1]
    vmlal.u16   q7, d8, d0[2]
    vmull.u16   q8, d5, d0[0]
    vmlal.u16   q8, d7, d0[1]
    vmlal.u16   q8, d9, d0[2]
    vrev64.32   q9,  q1
    vrev64.32   q13, q1
    vmlsl.u16   q9,  d4, d0[3]
    vmlsl.u16   q9,  d6, d1[0]
    vmlal.u16   q9,  d8, d1[1]
    vmlsl.u16   q13, d5, d0[3]
    vmlsl.u16   q13, d7, d1[0]
    vmlal.u16   q13, d9, d1[1]
    vrev64.32   q14, q1
    vrev64.32   q15, q1
    vmlal.u16   q14, d4, d1[1]
    vmlsl.u16   q14, d6, d1[2]
    vmlsl.u16   q14, d8, d1[3]
    vmlal.u16   q15, d5, d1[1]
    vmlsl.u16   q15, d7, d1[2]
    vmlsl.u16   q15, d9, d1[3]
.endm

.macro do_rgb_to_yuv_stage2
    vrshrn.u32  d20, q7,  #16
    vrshrn.u32  d21, q8,  #16
    vshrn.u32   d22, q9,  #16
    vshrn.u32   d23, q13, #16
    vshrn.u32   d24, q14, #16
    vshrn.u32   d25, q15, #16
    vmovn.u16   d20, q10      /* d20 = y */
    vmovn.u16   d21, q11      /* d21 = u */
    vmovn.u16   d22, q12      /* d22 = v */
.endm

.macro do_rgb_to_yuv
    do_rgb_to_yuv_stage1
    do_rgb_to_yuv_stage2
.endm

.macro do_rgb_to_yuv_stage2_store_load_stage1
      vrshrn.u32  d20, q7,  #16
      vrshrn.u32  d21, q8,  #16
      vshrn.u32   d22, q9,  #16
    vrev64.32   q9,  q1
      vshrn.u32   d23, q13, #16
    vrev64.32   q13, q1
      vshrn.u32   d24, q14, #16
      vshrn.u32   d25, q15, #16
    do_load     \bpp, 8
      vmovn.u16   d20, q10      /* d20 = y */
    vmovl.u8    q2, d1\r_offs   /* r = { d4, d5 } */
      vmovn.u16   d21, q11      /* d21 = u */
    vmovl.u8    q3, d1\g_offs   /* g = { d6, d7 } */
      vmovn.u16   d22, q12      /* d22 = v */
    vmovl.u8    q4, d1\b_offs   /* b = { d8, d9 } */
    vmull.u16   q7, d4, d0[0]
    vmlal.u16   q7, d6, d0[1]
    vmlal.u16   q7, d8, d0[2]
      vst1.8      {d20}, [Y]!
    vmull.u16   q8, d5, d0[0]
    vmlal.u16   q8, d7, d0[1]
    vmlal.u16   q8, d9, d0[2]
    vmlsl.u16   q9,  d4, d0[3]
    vmlsl.u16   q9,  d6, d1[0]
    vmlal.u16   q9,  d8, d1[1]
      vst1.8      {d21}, [U]!
    vmlsl.u16   q13, d5, d0[3]
    vmlsl.u16   q13, d7, d1[0]
    vmlal.u16   q13, d9, d1[1]
    vrev64.32   q14, q1
    vrev64.32   q15, q1
    vmlal.u16   q14, d4, d1[1]
    vmlsl.u16   q14, d6, d1[2]
    vmlsl.u16   q14, d8, d1[3]
      vst1.8      {d22}, [V]!
    vmlal.u16   q15, d5, d1[1]
    vmlsl.u16   q15, d7, d1[2]
    vmlsl.u16   q15, d9, d1[3]
.endm

.balign 16
jsimd_\colorid\()_ycc_neon_consts:
    .short          19595, 38470, 7471,  11059
    .short          21709, 32768, 27439, 5329
    .short          32767, 128,   32767, 128
    .short          32767, 128,   32767, 128

asm_function jsimd_\colorid\()_ycc_convert_neon
    OUTPUT_WIDTH    .req r0
    INPUT_BUF       .req r1
    OUTPUT_BUF      .req r2
    OUTPUT_ROW      .req r3
    NUM_ROWS        .req r4

    OUTPUT_BUF0     .req r5
    OUTPUT_BUF1     .req r6
    OUTPUT_BUF2     .req OUTPUT_BUF

    RGB             .req r7
    Y               .req r8
    U               .req r9
    V               .req r10
    N               .req ip

    /* Load constants to d0, d1, d2, d3 */
    adr             ip, jsimd_\colorid\()_ycc_neon_consts
    vld1.16         {d0, d1, d2, d3}, [ip, :128]

    /* Save ARM registers and handle input arguments */
    push            {r4, r5, r6, r7, r8, r9, r10, lr}
    ldr             NUM_ROWS, [sp, #(4 * 8)]
    ldr             OUTPUT_BUF0, [OUTPUT_BUF]
    ldr             OUTPUT_BUF1, [OUTPUT_BUF, #4]
    ldr             OUTPUT_BUF2, [OUTPUT_BUF, #8]
    .unreq          OUTPUT_BUF

    /* Save NEON registers */
    vpush           {d8-d15}

    /* Outer loop over scanlines */
    cmp             NUM_ROWS, #1
    blt             9f
0:
    ldr             Y, [OUTPUT_BUF0, OUTPUT_ROW, lsl #2]
    ldr             U, [OUTPUT_BUF1, OUTPUT_ROW, lsl #2]
    mov             N, OUTPUT_WIDTH
    ldr             V, [OUTPUT_BUF2, OUTPUT_ROW, lsl #2]
    add             OUTPUT_ROW, OUTPUT_ROW, #1
    ldr             RGB, [INPUT_BUF], #4

    /* Inner loop over pixels */
    subs            N, N, #8
    blt             3f
    do_load         \bpp, 8
    do_rgb_to_yuv_stage1
    subs            N, N, #8
    blt             2f
1:
    do_rgb_to_yuv_stage2_store_load_stage1
    subs            N, N, #8
    bge             1b
2:
    do_rgb_to_yuv_stage2
    do_store        8
    tst             N, #7
    beq             8f
3:
    tst             N, #4
    beq             3f
    do_load         \bpp, 4
3:
    tst             N, #2
    beq             4f
    do_load         \bpp, 2
4:
    tst             N, #1
    beq             5f
    do_load         \bpp, 1
5:
    do_rgb_to_yuv
    tst             N, #4
    beq             6f
    do_store        4
6:
    tst             N, #2
    beq             7f
    do_store        2
7:
    tst             N, #1
    beq             8f
    do_store        1
8:
    subs            NUM_ROWS, NUM_ROWS, #1
    bgt             0b
9:
    /* Restore all registers and return */
    vpop            {d8-d15}
    pop             {r4, r5, r6, r7, r8, r9, r10, pc}

    .unreq          OUTPUT_WIDTH
    .unreq          OUTPUT_ROW
    .unreq          INPUT_BUF
    .unreq          NUM_ROWS
    .unreq          OUTPUT_BUF0
    .unreq          OUTPUT_BUF1
    .unreq          OUTPUT_BUF2
    .unreq          RGB
    .unreq          Y
    .unreq          U
    .unreq          V
    .unreq          N
.endfunc

.purgem do_rgb_to_yuv
.purgem do_rgb_to_yuv_stage1
.purgem do_rgb_to_yuv_stage2
.purgem do_rgb_to_yuv_stage2_store_load_stage1

.endm

/*--------------------------------- id ----- bpp R  G  B */
generate_jsimd_rgb_ycc_convert_neon extrgb,  24, 0, 1, 2
generate_jsimd_rgb_ycc_convert_neon extbgr,  24, 2, 1, 0
generate_jsimd_rgb_ycc_convert_neon extrgbx, 32, 0, 1, 2
generate_jsimd_rgb_ycc_convert_neon extbgrx, 32, 2, 1, 0
generate_jsimd_rgb_ycc_convert_neon extxbgr, 32, 3, 2, 1
generate_jsimd_rgb_ycc_convert_neon extxrgb, 32, 1, 2, 3

.purgem do_load
.purgem do_store

/*****************************************************************************/

/*
 * Load data into workspace, applying unsigned->signed conversion
 *
 * TODO: can be combined with 'jsimd_fdct_ifast_neon' to get
 *       rid of VST1.16 instructions
 */

asm_function jsimd_convsamp_neon
    SAMPLE_DATA     .req r0
    START_COL       .req r1
    WORKSPACE       .req r2
    TMP1            .req r3
    TMP2            .req r4
    TMP3            .req r5
    TMP4            .req ip

    push            {r4, r5}
    vmov.u8         d0, #128

    ldmia           SAMPLE_DATA!, {TMP1, TMP2, TMP3, TMP4}
    add             TMP1, TMP1, START_COL
    add             TMP2, TMP2, START_COL
    add             TMP3, TMP3, START_COL
    add             TMP4, TMP4, START_COL
    vld1.8          {d16}, [TMP1]
    vsubl.u8        q8, d16, d0
    vld1.8          {d18}, [TMP2]
    vsubl.u8        q9, d18, d0
    vld1.8          {d20}, [TMP3]
    vsubl.u8        q10, d20, d0
    vld1.8          {d22}, [TMP4]
    ldmia           SAMPLE_DATA!, {TMP1, TMP2, TMP3, TMP4}
    vsubl.u8        q11, d22, d0
    vst1.16         {d16, d17, d18, d19}, [WORKSPACE, :128]!
    add             TMP1, TMP1, START_COL
    add             TMP2, TMP2, START_COL
    vst1.16         {d20, d21, d22, d23}, [WORKSPACE, :128]!
    add             TMP3, TMP3, START_COL
    add             TMP4, TMP4, START_COL
    vld1.8          {d24}, [TMP1]
    vsubl.u8        q12, d24, d0
    vld1.8          {d26}, [TMP2]
    vsubl.u8        q13, d26, d0
    vld1.8          {d28}, [TMP3]
    vsubl.u8        q14, d28, d0
    vld1.8          {d30}, [TMP4]
    vsubl.u8        q15, d30, d0
    vst1.16         {d24, d25, d26, d27}, [WORKSPACE, :128]!
    vst1.16         {d28, d29, d30, d31}, [WORKSPACE, :128]!
    pop             {r4, r5}
    bx              lr

    .unreq          SAMPLE_DATA
    .unreq          START_COL
    .unreq          WORKSPACE
    .unreq          TMP1
    .unreq          TMP2
    .unreq          TMP3
    .unreq          TMP4
.endfunc

/*****************************************************************************/

/*
 * jsimd_fdct_ifast_neon
 *
 * This function contains a fast, not so accurate integer implementation of
 * the forward DCT (Discrete Cosine Transform). It uses the same calculations
 * and produces exactly the same output as IJG's original 'jpeg_fdct_ifast'
 * function from jfdctfst.c
 *
 * TODO: can be combined with 'jsimd_convsamp_neon' to get
 *       rid of a bunch of VLD1.16 instructions
 */

#define XFIX_0_382683433 d0[0]
#define XFIX_0_541196100 d0[1]
#define XFIX_0_707106781 d0[2]
#define XFIX_1_306562965 d0[3]

.balign 16
jsimd_fdct_ifast_neon_consts:
    .short (98 * 128)              /* XFIX_0_382683433 */
    .short (139 * 128)             /* XFIX_0_541196100 */
    .short (181 * 128)             /* XFIX_0_707106781 */
    .short (334 * 128 - 256 * 128) /* XFIX_1_306562965 */

asm_function jsimd_fdct_ifast_neon

    DATA            .req r0
    TMP             .req ip

    vpush           {d8-d15}

    /* Load constants */
    adr             TMP, jsimd_fdct_ifast_neon_consts
    vld1.16         {d0}, [TMP, :64]

    /* Load all DATA into NEON registers with the following allocation:
     *       0 1 2 3 | 4 5 6 7
     *      ---------+--------
     *   0 | d16     | d17    | q8
     *   1 | d18     | d19    | q9
     *   2 | d20     | d21    | q10
     *   3 | d22     | d23    | q11
     *   4 | d24     | d25    | q12
     *   5 | d26     | d27    | q13
     *   6 | d28     | d29    | q14
     *   7 | d30     | d31    | q15
     */

    vld1.16         {d16, d17, d18, d19}, [DATA, :128]!
    vld1.16         {d20, d21, d22, d23}, [DATA, :128]!
    vld1.16         {d24, d25, d26, d27}, [DATA, :128]!
    vld1.16         {d28, d29, d30, d31}, [DATA, :128]
    sub             DATA, DATA, #(128 - 32)

    mov             TMP, #2
1:
    /* Transpose */
    vtrn.16         q12, q13
    vtrn.16         q10, q11
    vtrn.16         q8,  q9
    vtrn.16         q14, q15
    vtrn.32         q9,  q11
    vtrn.32         q13, q15
    vtrn.32         q8,  q10
    vtrn.32         q12, q14
    vswp            d30, d23
    vswp            d24, d17
    vswp            d26, d19
      /* 1-D FDCT */
      vadd.s16        q2,  q11, q12
    vswp            d28, d21
      vsub.s16        q12, q11, q12
      vsub.s16        q6,  q10, q13
      vadd.s16        q10, q10, q13
      vsub.s16        q7,  q9,  q14
      vadd.s16        q9,  q9,  q14
      vsub.s16        q1,  q8,  q15
      vadd.s16        q8,  q8,  q15
      vsub.s16        q4,  q9,  q10
      vsub.s16        q5,  q8,  q2
      vadd.s16        q3,  q9,  q10
      vadd.s16        q4,  q4,  q5
      vadd.s16        q2,  q8,  q2
      vqdmulh.s16     q4,  q4,  XFIX_0_707106781
      vadd.s16        q11, q12, q6
      vadd.s16        q8,  q2,  q3
      vsub.s16        q12, q2,  q3
      vadd.s16        q3,  q6,  q7
      vadd.s16        q7,  q7,  q1
      vqdmulh.s16     q3,  q3,  XFIX_0_707106781
      vsub.s16        q6,  q11, q7
      vadd.s16        q10, q5,  q4
      vqdmulh.s16     q6,  q6,  XFIX_0_382683433
      vsub.s16        q14, q5,  q4
      vqdmulh.s16     q11, q11, XFIX_0_541196100
      vqdmulh.s16     q5,  q7,  XFIX_1_306562965
      vadd.s16        q4,  q1,  q3
      vsub.s16        q3,  q1,  q3
      vadd.s16        q7,  q7,  q6
      vadd.s16        q11, q11, q6
      vadd.s16        q7,  q7,  q5
      vadd.s16        q13, q3,  q11
      vsub.s16        q11, q3,  q11
      vadd.s16        q9,  q4,  q7
      vsub.s16        q15, q4,  q7
    subs            TMP, TMP, #1
    bne             1b

    /* store results */
    vst1.16         {d16, d17, d18, d19}, [DATA, :128]!
    vst1.16         {d20, d21, d22, d23}, [DATA, :128]!
    vst1.16         {d24, d25, d26, d27}, [DATA, :128]!
    vst1.16         {d28, d29, d30, d31}, [DATA, :128]

    vpop            {d8-d15}
    bx              lr

    .unreq          DATA
    .unreq          TMP
.endfunc

/*****************************************************************************/
