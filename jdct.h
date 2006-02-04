/*
 * jdct.h
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * ---------------------------------------------------------------------
 * x86 SIMD extension for IJG JPEG library
 * Copyright (C) 1999-2006, MIYASAKA Masaru.
 * This file has been modified for SIMD extension.
 * Last Modified : January 5, 2006
 * ---------------------------------------------------------------------
 *
 * This include file contains common declarations for the forward and
 * inverse DCT modules.  These declarations are private to the DCT managers
 * (jcdctmgr.c, jddctmgr.c) and the individual DCT algorithms.
 * The individual DCT algorithms are kept in separate files to ease 
 * machine-dependent tuning (e.g., assembly coding).
 */


/* SIMD Ext: configuration check */

#if BITS_IN_JSAMPLE != 8
#error "Sorry, this SIMD code only copes with 8-bit sample values."
#endif


/*
 * A forward DCT routine is given a pointer to a work area of type DCTELEM[];
 * the DCT is to be performed in-place in that buffer.  Type DCTELEM is int
 * for 8-bit samples, INT32 for 12-bit samples.  (NOTE: Floating-point DCT
 * implementations use an array of type FAST_FLOAT, instead.)
 * The DCT inputs are expected to be signed (range +-CENTERJSAMPLE).
 * The DCT outputs are returned scaled up by a factor of 8; they therefore
 * have a range of +-8K for 8-bit data, +-128K for 12-bit data.  This
 * convention improves accuracy in integer implementations and saves some
 * work in floating-point ones.
 * Quantization of the output coefficients is done by jcdctmgr.c.
 */

/* SIMD Ext: To maximize parallelism, Type DCTELEM is changed to short
 * (originally, int).
 */
typedef short DCTELEM;		/* SIMD Ext: must be short */

typedef JMETHOD(void, forward_DCT_method_ptr, (DCTELEM * data));
typedef JMETHOD(void, float_DCT_method_ptr, (FAST_FLOAT * data));
typedef JMETHOD(void, convsamp_int_method_ptr,
		(JSAMPARRAY sample_data, JDIMENSION start_col,
		 DCTELEM * workspace));
typedef JMETHOD(void, convsamp_float_method_ptr,
		(JSAMPARRAY sample_data, JDIMENSION start_col,
		 FAST_FLOAT *workspace));
typedef JMETHOD(void, quantize_int_method_ptr,
		(JCOEFPTR coef_block, DCTELEM * divisors,
		 DCTELEM * workspace));
typedef JMETHOD(void, quantize_float_method_ptr,
		(JCOEFPTR coef_block, FAST_FLOAT * divisors,
		 FAST_FLOAT * workspace));


/*
 * An inverse DCT routine is given a pointer to the input JBLOCK and a pointer
 * to an output sample array.  The routine must dequantize the input data as
 * well as perform the IDCT; for dequantization, it uses the multiplier table
 * pointed to by compptr->dct_table.  The output data is to be placed into the
 * sample array starting at a specified column.  (Any row offset needed will
 * be applied to the array pointer before it is passed to the IDCT code.)
 * Note that the number of samples emitted by the IDCT routine is
 * DCT_scaled_size * DCT_scaled_size.
 */

/* typedef inverse_DCT_method_ptr is declared in jpegint.h */

/* SIMD Ext: To maximize parallelism, Type MULTIPLIER is changed to short.
 * Macro definitions of MULTIPLIER and FAST_FLOAT in jmorecfg.h are ignored.
 */
#undef MULTIPLIER
#define MULTIPLIER  short	/* SIMD Ext: must be short */
#undef FAST_FLOAT
#define FAST_FLOAT  float	/* SIMD Ext: must be float */

/*
 * Each IDCT routine has its own ideas about the best dct_table element type.
 */

typedef MULTIPLIER ISLOW_MULT_TYPE;	/* SIMD Ext: must be short */
typedef MULTIPLIER IFAST_MULT_TYPE;	/* SIMD Ext: must be short */
#define IFAST_SCALE_BITS  2	/* fractional bits in scale factors */
typedef FAST_FLOAT FLOAT_MULT_TYPE;	/* SIMD Ext: must be float */


/*
 * Each IDCT routine is responsible for range-limiting its results and
 * converting them to unsigned form (0..MAXJSAMPLE).  The raw outputs could
 * be quite far out of range if the input data is corrupt, so a bulletproof
 * range-limiting step is required.  We use a mask-and-table-lookup method
 * to do the combined operations quickly.  See the comments with
 * prepare_range_limit_table (in jdmaster.c) for more info.
 */

#define IDCT_range_limit(cinfo)  ((cinfo)->sample_range_limit + CENTERJSAMPLE)

#define RANGE_MASK  (MAXJSAMPLE * 4 + 3) /* 2 bits wider than legal samples */


/* Short forms of external names for systems with brain-damaged linkers. */

#ifdef NEED_SHORT_EXTERNAL_NAMES
#define jpeg_fdct_islow		jFDislow		/* jfdctint.asm */
#define jpeg_fdct_ifast		jFDifast		/* jfdctfst.asm */
#define jpeg_fdct_float		jFDfloat		/* jfdctflt.asm */
#define jpeg_fdct_islow_mmx	jFDMislow		/* jfmmxint.asm */
#define jpeg_fdct_ifast_mmx	jFDMifast		/* jfmmxfst.asm */
#define jpeg_fdct_float_3dnow	jFD3float		/* jf3dnflt.asm */
#define jpeg_fdct_islow_sse2	jFDSislow		/* jfss2int.asm */
#define jpeg_fdct_ifast_sse2	jFDSifast		/* jfss2fst.asm */
#define jpeg_fdct_float_sse	jFDSfloat		/* jfsseflt.asm */
#define jpeg_convsamp_int	jCnvInt			/* jcqntint.asm */
#define jpeg_quantize_int	jQntInt			/* jcqntint.asm */
#define jpeg_quantize_idiv	jQntIDiv		/* jcqntint.asm */
#define jpeg_convsamp_float	jCnvFloat		/* jcqntflt.asm */
#define jpeg_quantize_float	jQntFloat		/* jcqntflt.asm */
#define jpeg_convsamp_int_mmx	jCnvMmx			/* jcqntmmx.asm */
#define jpeg_quantize_int_mmx	jQntMmx			/* jcqntmmx.asm */
#define jpeg_convsamp_flt_3dnow	jCnv3dnow		/* jcqnt3dn.asm */
#define jpeg_quantize_flt_3dnow	jQnt3dnow		/* jcqnt3dn.asm */
#define jpeg_convsamp_int_sse2	jCnvISse2		/* jcqnts2i.asm */
#define jpeg_quantize_int_sse2	jQntISse2		/* jcqnts2i.asm */
#define jpeg_convsamp_flt_sse	jCnvSse			/* jcqntsse.asm */
#define jpeg_quantize_flt_sse	jQntSse			/* jcqntsse.asm */
#define jpeg_convsamp_flt_sse2	jCnvFSse2		/* jcqnts2f.asm */
#define jpeg_quantize_flt_sse2	jQntFSse2		/* jcqnts2f.asm */
#define jpeg_idct_islow		jRDislow		/* jidctint.asm */
#define jpeg_idct_ifast		jRDifast		/* jidctfst.asm */
#define jpeg_idct_float		jRDfloat		/* jidctflt.asm */
#define jpeg_idct_4x4		jRD4x4			/* jidctred.asm */
#define jpeg_idct_2x2		jRD2x2			/* jidctred.asm */
#define jpeg_idct_1x1		jRD1x1			/* jidctred.asm */
#define jpeg_idct_islow_mmx	jRDMislow		/* jimmxint.asm */
#define jpeg_idct_ifast_mmx	jRDMifast		/* jimmxfst.asm */
#define jpeg_idct_float_3dnow	jRD3float		/* ji3dnflt.asm */
#define jpeg_idct_4x4_mmx	jRDM4x4			/* jimmxred.asm */
#define jpeg_idct_2x2_mmx	jRDM2x2			/* jimmxred.asm */
#define jpeg_idct_islow_sse2	jRDSislow		/* jiss2int.asm */
#define jpeg_idct_ifast_sse2	jRDSifast		/* jiss2fst.asm */
#define jpeg_idct_float_sse	jRDSfloat		/* jisseflt.asm */
#define jpeg_idct_float_sse2	jRD2float		/* jiss2flt.asm */
#define jpeg_idct_4x4_sse2	jRDS4x4			/* jiss2red.asm */
#define jpeg_idct_2x2_sse2	jRDS2x2			/* jiss2red.asm */
#define jconst_fdct_float	jFCfloat		/* jfdctflt.asm */
#define jconst_fdct_islow_mmx	jFCMislow		/* jfmmxint.asm */
#define jconst_fdct_ifast_mmx	jFCMifast		/* jfmmxfst.asm */
#define jconst_fdct_float_3dnow	jFC3float		/* jf3dnflt.asm */
#define jconst_fdct_islow_sse2	jFCSislow		/* jfss2int.asm */
#define jconst_fdct_ifast_sse2	jFCSifast		/* jfss2fst.asm */
#define jconst_fdct_float_sse	jFCSfloat		/* jfsseflt.asm */
#define jconst_idct_float	jRCfloat		/* jidctflt.asm */
#define jconst_idct_islow_mmx	jRCMislow		/* jimmxint.asm */
#define jconst_idct_ifast_mmx	jRCMifast		/* jimmxfst.asm */
#define jconst_idct_float_3dnow	jRC3float		/* ji3dnflt.asm */
#define jconst_idct_red_mmx	jRCMred			/* jimmxred.asm */
#define jconst_idct_islow_sse2	jRCSislow		/* jiss2int.asm */
#define jconst_idct_ifast_sse2	jRCSifast		/* jiss2fst.asm */
#define jconst_idct_float_sse	jRCSfloat		/* jisseflt.asm */
#define jconst_idct_float_sse2	jRC2float		/* jiss2flt.asm */
#define jconst_idct_red_sse2	jRCSred			/* jiss2red.asm */
#endif /* NEED_SHORT_EXTERNAL_NAMES */

/* Extern declarations for the forward and inverse DCT routines. */

EXTERN(void) jpeg_fdct_islow JPP((DCTELEM * data));
EXTERN(void) jpeg_fdct_ifast JPP((DCTELEM * data));
EXTERN(void) jpeg_fdct_float JPP((FAST_FLOAT * data));

EXTERN(void) jpeg_fdct_islow_mmx JPP((DCTELEM * data));
EXTERN(void) jpeg_fdct_ifast_mmx JPP((DCTELEM * data));
EXTERN(void) jpeg_fdct_float_3dnow JPP((FAST_FLOAT * data));

EXTERN(void) jpeg_fdct_islow_sse2 JPP((DCTELEM * data));
EXTERN(void) jpeg_fdct_ifast_sse2 JPP((DCTELEM * data));
EXTERN(void) jpeg_fdct_float_sse JPP((FAST_FLOAT * data));

EXTERN(void) jpeg_convsamp_int
    JPP((JSAMPARRAY sample_data, JDIMENSION start_col, DCTELEM * workspace));
EXTERN(void) jpeg_quantize_int
    JPP((JCOEFPTR coef_block, DCTELEM * divisors, DCTELEM * workspace));
EXTERN(void) jpeg_quantize_idiv
    JPP((JCOEFPTR coef_block, DCTELEM * divisors, DCTELEM * workspace));
EXTERN(void) jpeg_convsamp_float
    JPP((JSAMPARRAY sample_data, JDIMENSION start_col, FAST_FLOAT *workspace));
EXTERN(void) jpeg_quantize_float
    JPP((JCOEFPTR coef_block, FAST_FLOAT * divisors, FAST_FLOAT * workspace));

EXTERN(void) jpeg_convsamp_int_mmx
    JPP((JSAMPARRAY sample_data, JDIMENSION start_col, DCTELEM * workspace));
EXTERN(void) jpeg_quantize_int_mmx
    JPP((JCOEFPTR coef_block, DCTELEM * divisors, DCTELEM * workspace));
EXTERN(void) jpeg_convsamp_flt_3dnow
    JPP((JSAMPARRAY sample_data, JDIMENSION start_col, FAST_FLOAT *workspace));
EXTERN(void) jpeg_quantize_flt_3dnow
    JPP((JCOEFPTR coef_block, FAST_FLOAT * divisors, FAST_FLOAT * workspace));

EXTERN(void) jpeg_convsamp_int_sse2
    JPP((JSAMPARRAY sample_data, JDIMENSION start_col, DCTELEM * workspace));
EXTERN(void) jpeg_quantize_int_sse2
    JPP((JCOEFPTR coef_block, DCTELEM * divisors, DCTELEM * workspace));
EXTERN(void) jpeg_convsamp_flt_sse
    JPP((JSAMPARRAY sample_data, JDIMENSION start_col, FAST_FLOAT *workspace));
EXTERN(void) jpeg_quantize_flt_sse
    JPP((JCOEFPTR coef_block, FAST_FLOAT * divisors, FAST_FLOAT * workspace));
EXTERN(void) jpeg_convsamp_flt_sse2
    JPP((JSAMPARRAY sample_data, JDIMENSION start_col, FAST_FLOAT *workspace));
EXTERN(void) jpeg_quantize_flt_sse2
    JPP((JCOEFPTR coef_block, FAST_FLOAT * divisors, FAST_FLOAT * workspace));

EXTERN(void) jpeg_idct_islow
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_ifast
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_float
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_4x4
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_2x2
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_1x1
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));

EXTERN(void) jpeg_idct_islow_mmx
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_ifast_mmx
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_4x4_mmx
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_2x2_mmx
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));

EXTERN(void) jpeg_idct_float_3dnow
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_float_sse
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_float_sse2
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));

EXTERN(void) jpeg_idct_islow_sse2
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_ifast_sse2
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_4x4_sse2
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_2x2_sse2
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));

extern const int jconst_fdct_float[];
extern const int jconst_fdct_islow_mmx[];
extern const int jconst_fdct_ifast_mmx[];
extern const int jconst_fdct_float_3dnow[];
extern const int jconst_fdct_islow_sse2[];
extern const int jconst_fdct_ifast_sse2[];
extern const int jconst_fdct_float_sse[];
extern const int jconst_idct_float[];
extern const int jconst_idct_islow_mmx[];
extern const int jconst_idct_ifast_mmx[];
extern const int jconst_idct_float_3dnow[];
extern const int jconst_idct_red_mmx[];
extern const int jconst_idct_islow_sse2[];
extern const int jconst_idct_ifast_sse2[];
extern const int jconst_idct_float_sse[];
extern const int jconst_idct_float_sse2[];
extern const int jconst_idct_red_sse2[];


/*
 * Macros for handling fixed-point arithmetic; these are used by many
 * but not all of the DCT/IDCT modules.
 *
 * All values are expected to be of type INT32.
 * Fractional constants are scaled left by CONST_BITS bits.
 * CONST_BITS is defined within each module using these macros,
 * and may differ from one module to the next.
 */

#define ONE	((INT32) 1)
#define CONST_SCALE (ONE << CONST_BITS)

/* Convert a positive real constant to an integer scaled by CONST_SCALE.
 * Caution: some C compilers fail to reduce "FIX(constant)" at compile time,
 * thus causing a lot of useless floating-point operations at run time.
 */

#define FIX(x)	((INT32) ((x) * CONST_SCALE + 0.5))

/* Descale and correctly round an INT32 value that's scaled by N bits.
 * We assume RIGHT_SHIFT rounds towards minus infinity, so adding
 * the fudge factor is correct for either sign of X.
 */

#define DESCALE(x,n)  RIGHT_SHIFT((x) + (ONE << ((n)-1)), n)

/* Multiply an INT32 variable by an INT32 constant to yield an INT32 result.
 * This macro is used only when the two inputs will actually be no more than
 * 16 bits wide, so that a 16x16->32 bit multiply can be used instead of a
 * full 32x32 multiply.  This provides a useful speedup on many machines.
 * Unfortunately there is no way to specify a 16x16->32 multiply portably
 * in C, but some C compilers will do the right thing if you provide the
 * correct combination of casts.
 */

#ifdef SHORTxSHORT_32		/* may work if 'int' is 32 bits */
#define MULTIPLY16C16(var,const)  (((INT16) (var)) * ((INT16) (const)))
#endif
#ifdef SHORTxLCONST_32		/* known to work with Microsoft C 6.0 */
#define MULTIPLY16C16(var,const)  (((INT16) (var)) * ((INT32) (const)))
#endif

#ifndef MULTIPLY16C16		/* default definition */
#define MULTIPLY16C16(var,const)  ((var) * (const))
#endif

/* Same except both inputs are variables. */

#ifdef SHORTxSHORT_32		/* may work if 'int' is 32 bits */
#define MULTIPLY16V16(var1,var2)  (((INT16) (var1)) * ((INT16) (var2)))
#endif

#ifndef MULTIPLY16V16		/* default definition */
#define MULTIPLY16V16(var1,var2)  ((var1) * (var2))
#endif
