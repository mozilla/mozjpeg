/*
 * jconfig.h
 *
 * Copyright (C) 1991, 1992, 1993, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains preprocessor declarations that help customize
 * the JPEG software for a particular application, machine, or compiler.
 * Edit these declarations as needed (or add -D flags to the Makefile).
 */


/*
 * These symbols indicate the properties of your machine or compiler.
 * The conditional definitions given may do the right thing already,
 * but you'd best look them over closely, especially if your compiler
 * does not handle full ANSI C.  An ANSI-compliant C compiler should
 * provide all the necessary features; __STDC__ is supposed to be
 * predefined by such compilers.
 */

/*
 * HAVE_STDC is tested below to see whether ANSI features are available.
 * We avoid testing __STDC__ directly for arcane reasons of portability.
 * (On some compilers, __STDC__ is only defined if a switch is given,
 * but the switch also disables machine-specific features we need to get at.
 * In that case, -DHAVE_STDC in the Makefile is a convenient solution.)
 */

#ifdef __STDC__			/* if compiler claims to be ANSI, believe it */
#define HAVE_STDC
#endif


/* Does your compiler support function prototypes? */
/* (If not, you also need to use ansi2knr, see SETUP) */

#ifdef HAVE_STDC		/* ANSI C compilers always have prototypes */
#define PROTO
#else
#ifdef __cplusplus		/* So do C++ compilers */
#define PROTO
#endif
#endif

/* Does your compiler support the declaration "unsigned char" ? */
/* How about "unsigned short" ? */

#ifdef HAVE_STDC		/* ANSI C compilers must support both */
#define HAVE_UNSIGNED_CHAR
#define HAVE_UNSIGNED_SHORT
#endif

/* Define this if an ordinary "char" type is unsigned.
 * If you're not sure, leaving it undefined will work at some cost in speed.
 * If you defined HAVE_UNSIGNED_CHAR then it doesn't matter very much.
 */

/* #define CHAR_IS_UNSIGNED */

/* Define this if your compiler implements ">>" on signed values as a logical
 * (unsigned) shift; leave it undefined if ">>" is a signed (arithmetic) shift,
 * which is the normal and rational definition.
 */

/* #define RIGHT_SHIFT_IS_UNSIGNED */

/* Define "void" as "char" if your compiler doesn't know about type void.
 * NOTE: be sure to define void such that "void *" represents the most general
 * pointer type, e.g., that returned by malloc().
 */

/* #define void char */

/* Define const as empty if your compiler doesn't know the "const" keyword. */
/* (Even if it does, defining const as empty won't break anything.) */

#ifndef HAVE_STDC		/* ANSI C and C++ compilers should know it. */
#ifndef __cplusplus
#define const
#endif
#endif

/* For 80x86 machines, you need to define NEED_FAR_POINTERS,
 * unless you are using a large-data memory model or 80386 flat-memory mode.
 * On less brain-damaged CPUs this symbol must not be defined.
 * (Defining this symbol causes large data structures to be referenced through
 * "far" pointers and to be allocated with a special version of malloc.)
 */

#ifdef MSDOS
#define NEED_FAR_POINTERS
#endif


/* The next three symbols only affect the system-dependent user interface
 * modules (jcmain.c, jdmain.c).  You can ignore these if you are supplying
 * your own user interface code.
 */

/* Define this if you want to name both input and output files on the command
 * line, rather than using stdout and optionally stdin.  You MUST do this if
 * your system can't cope with binary I/O to stdin/stdout.  See comments at
 * head of jcmain.c or jdmain.c.
 */

#ifdef MSDOS			/* two-file style is needed for PCs */
#ifndef USE_FDOPEN		/* unless you have fdopen() or setmode() */
#ifndef USE_SETMODE
#define TWO_FILE_COMMANDLINE
#endif
#endif
#endif
#ifdef THINK_C			/* it's needed for Macintosh too */
#define TWO_FILE_COMMANDLINE
#endif

/* Define this if your system needs explicit cleanup of temporary files.
 * This is crucial under MS-DOS, where the temporary "files" may be areas
 * of extended memory; on most other systems it's not as important.
 */

#ifdef MSDOS
#define NEED_SIGNAL_CATCHER
#endif

/* By default, we open image files with fopen(...,"rb") or fopen(...,"wb").
 * This is necessary on systems that distinguish text files from binary files,
 * and is harmless on most systems that don't.  If you have one of the rare
 * systems that complains about the "b" spec, define this symbol.
 */

/* #define DONT_USE_B_MODE */


/* If you're getting bored, that's the end of the symbols you HAVE to
 * worry about.  Go fix the makefile and compile.
 */


/* If your compiler supports inline functions, define INLINE
 * as the inline keyword; otherwise define it as empty.
 */

#ifdef __GNUC__			/* for instance, GNU C knows about inline */
#define INLINE __inline__
#endif
#ifndef INLINE			/* default is to define it as empty */
#define INLINE
#endif

/* On a few systems, type boolean and/or macros FALSE, TRUE may appear
 * in standard header files.  Or you may have conflicts with application-
 * specific header files that you want to include together with these files.
 * In that case you need only comment out these definitions.
 */

typedef int boolean;
#undef FALSE			/* in case these macros already exist */
#undef TRUE
#define FALSE	0		/* values of boolean */
#define TRUE	1

/* This defines the size of the I/O buffers for entropy compression
 * and decompression; you could reduce it if memory is tight.
 */

#define JPEG_BUF_SIZE	4096 /* bytes */



/* These symbols determine the JPEG functionality supported. */

/*
 * These defines indicate whether to include various optional functions.
 * Undefining some of these symbols will produce a smaller but less capable
 * program file.  Note that you can leave certain source files out of the
 * compilation/linking process if you've #undef'd the corresponding symbols.
 * (You may HAVE to do that if your compiler doesn't like null source files.)
 */

/* Arithmetic coding is unsupported for legal reasons.  Complaints to IBM. */

/* Encoder capability options: */
#undef  C_ARITH_CODING_SUPPORTED    /* Arithmetic coding back end? */
#undef  C_MULTISCAN_FILES_SUPPORTED /* Multiple-scan JPEG files?  (NYI) */
#define ENTROPY_OPT_SUPPORTED	    /* Optimization of entropy coding parms? */
#define INPUT_SMOOTHING_SUPPORTED   /* Input image smoothing option? */
/* Decoder capability options: */
#undef  D_ARITH_CODING_SUPPORTED    /* Arithmetic coding back end? */
#define D_MULTISCAN_FILES_SUPPORTED /* Multiple-scan JPEG files? */
#define BLOCK_SMOOTHING_SUPPORTED   /* Block smoothing during decoding? */
#define QUANT_1PASS_SUPPORTED	/* 1-pass color quantization? */
#define QUANT_2PASS_SUPPORTED	/* 2-pass color quantization? */
/* these defines indicate which JPEG file formats are allowed */
#define JFIF_SUPPORTED		/* JFIF or "raw JPEG" files */
#undef  JTIFF_SUPPORTED		/* JPEG-in-TIFF (not yet implemented) */
/* these defines indicate which image (non-JPEG) file formats are allowed */
#define GIF_SUPPORTED		/* GIF image file format */
/* #define RLE_SUPPORTED */	/* RLE image file format (by default, no) */
#define PPM_SUPPORTED		/* PPM/PGM image file format */
#define TARGA_SUPPORTED		/* Targa image file format */
#undef  TIFF_SUPPORTED		/* TIFF image file format (not yet impl.) */

/* more capability options later, no doubt */


/*
 * Define exactly one of these three symbols to indicate whether you want
 * 8-bit, 12-bit, or 16-bit sample (pixel component) values.  8-bit is the
 * default and is nearly always the right thing to use.  You can use 12-bit if
 * you need to support image formats with more than 8 bits of resolution in a
 * color value.  16-bit should only be used for the lossless JPEG mode (not
 * currently supported).  Note that 12- and 16-bit values take up twice as
 * much memory as 8-bit!
 * Note: if you select 12- or 16-bit precision, it is dangerous to turn off
 * ENTROPY_OPT_SUPPORTED.  The standard Huffman tables are only good for 8-bit
 * precision, so jchuff.c normally uses entropy optimization to compute
 * usable tables for higher precision.  If you don't want to do optimization,
 * you'll have to supply different default Huffman tables.
 */

#define EIGHT_BIT_SAMPLES
#undef  TWELVE_BIT_SAMPLES
#undef  SIXTEEN_BIT_SAMPLES



/*
 * The remaining definitions don't need to be hand-edited in most cases.
 * You may need to change these if you have a machine with unusual data
 * types; for example, "char" not 8 bits, "short" not 16 bits,
 * or "long" not 32 bits.  We don't care whether "int" is 16 or 32 bits,
 * but it had better be at least 16.
 */

/* First define the representation of a single pixel element value. */

#ifdef EIGHT_BIT_SAMPLES
/* JSAMPLE should be the smallest type that will hold the values 0..255.
 * You can use a signed char by having GETJSAMPLE mask it with 0xFF.
 * If you have only signed chars, and you are more worried about speed than
 * memory usage, it might be a win to make JSAMPLE be short.
 */

#ifdef HAVE_UNSIGNED_CHAR

typedef unsigned char JSAMPLE;
#define GETJSAMPLE(value)  (value)

#else /* not HAVE_UNSIGNED_CHAR */
#ifdef CHAR_IS_UNSIGNED

typedef char JSAMPLE;
#define GETJSAMPLE(value)  (value)

#else /* not CHAR_IS_UNSIGNED */

typedef char JSAMPLE;
#define GETJSAMPLE(value)  ((value) & 0xFF)

#endif /* CHAR_IS_UNSIGNED */
#endif /* HAVE_UNSIGNED_CHAR */

#define BITS_IN_JSAMPLE   8
#define MAXJSAMPLE	255
#define CENTERJSAMPLE	128

#endif /* EIGHT_BIT_SAMPLES */


#ifdef TWELVE_BIT_SAMPLES
/* JSAMPLE should be the smallest type that will hold the values 0..4095. */
/* On nearly all machines "short" will do nicely. */

typedef short JSAMPLE;
#define GETJSAMPLE(value)  (value)

#define BITS_IN_JSAMPLE   12
#define MAXJSAMPLE	4095
#define CENTERJSAMPLE	2048

#endif /* TWELVE_BIT_SAMPLES */


#ifdef SIXTEEN_BIT_SAMPLES
/* JSAMPLE should be the smallest type that will hold the values 0..65535. */

#ifdef HAVE_UNSIGNED_SHORT

typedef unsigned short JSAMPLE;
#define GETJSAMPLE(value)  (value)

#else /* not HAVE_UNSIGNED_SHORT */

/* If int is 32 bits this'll be horrendously inefficient storage-wise.
 * But since we don't actually support 16-bit samples (ie lossless coding) yet,
 * I'm not going to worry about making a smarter definition ...
 */
typedef unsigned int JSAMPLE;
#define GETJSAMPLE(value)  (value)

#endif /* HAVE_UNSIGNED_SHORT */

#define BITS_IN_JSAMPLE    16
#define MAXJSAMPLE	65535
#define CENTERJSAMPLE	32768

#endif /* SIXTEEN_BIT_SAMPLES */


/* Here we define the representation of a DCT frequency coefficient.
 * This should be a signed 16-bit value; "short" is usually right.
 * It's important that this be exactly 16 bits, no more and no less;
 * more will cost you a BIG hit of memory, less will give wrong answers.
 */

typedef short JCOEF;


/* The remaining typedefs are used for various table entries and so forth.
 * They must be at least as wide as specified; but making them too big
 * won't cost a huge amount of memory, so we don't provide special
 * extraction code like we did for JSAMPLE.  (In other words, these
 * typedefs live at a different point on the speed/space tradeoff curve.)
 */

/* UINT8 must hold at least the values 0..255. */

#ifdef HAVE_UNSIGNED_CHAR
typedef unsigned char UINT8;
#else /* not HAVE_UNSIGNED_CHAR */
#ifdef CHAR_IS_UNSIGNED
typedef char UINT8;
#else /* not CHAR_IS_UNSIGNED */
typedef short UINT8;
#endif /* CHAR_IS_UNSIGNED */
#endif /* HAVE_UNSIGNED_CHAR */

/* UINT16 must hold at least the values 0..65535. */

#ifdef HAVE_UNSIGNED_SHORT
typedef unsigned short UINT16;
#else /* not HAVE_UNSIGNED_SHORT */
typedef unsigned int UINT16;
#endif /* HAVE_UNSIGNED_SHORT */

/* INT16 must hold at least the values -32768..32767. */

#ifndef XMD_H			/* X11/xmd.h correctly defines INT16 */
typedef short INT16;
#endif

/* INT32 must hold signed 32-bit values; if your machine happens */
/* to have 64-bit longs, you might want to change this. */

#ifndef XMD_H			/* X11/xmd.h correctly defines INT32 */
typedef long INT32;
#endif
