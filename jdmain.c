/*
 * jdmain.c
 *
 * Copyright (C) 1991, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains a trivial test user interface for the JPEG decompressor.
 * It should work on any system with Unix- or MS-DOS-style command lines.
 *
 * Two different command line styles are permitted, depending on the
 * compile-time switch TWO_FILE_COMMANDLINE:
 *	djpeg [options]  inputfile outputfile
 *	djpeg [options]  [inputfile]
 * In the second style, output is always to standard output, which you'd
 * normally redirect to a file or pipe to some other program.  Input is
 * either from a named file or from standard input (typically redirected).
 * The second style is convenient on Unix but is unhelpful on systems that
 * don't support pipes.  Also, you MUST use the first style if your system
 * doesn't do binary I/O to stdin/stdout.
 */

#include "jinclude.h"
#ifdef INCLUDES_ARE_ANSI
#include <stdlib.h>		/* to declare exit() */
#endif

#ifdef THINK_C
#include <console.h>		/* command-line reader for Macintosh */
#endif

#ifdef DONT_USE_B_MODE		/* define mode parameters for fopen() */
#define READ_BINARY	"r"
#define WRITE_BINARY	"w"
#else
#define READ_BINARY	"rb"
#define WRITE_BINARY	"wb"
#endif

#include "jversion.h"		/* for version message */


/*
 * PD version of getopt(3).
 */

#include "egetopt.c"


/*
 * This list defines the known output image formats
 * (not all of which need be supported by a given version).
 * You can change the default output format by defining DEFAULT_FMT;
 * indeed, you had better do so if you undefine PPM_SUPPORTED.
 */

typedef enum {
	FMT_GIF,		/* GIF format */
	FMT_PPM,		/* PPM/PGM (PBMPLUS formats) */
	FMT_RLE,		/* RLE format */
	FMT_TARGA,		/* Targa format */
	FMT_TIFF		/* TIFF format */
} IMAGE_FORMATS;

#ifndef DEFAULT_FMT		/* so can override from CFLAGS in Makefile */
#define DEFAULT_FMT	FMT_PPM
#endif

static IMAGE_FORMATS requested_fmt;


/*
 * This routine gets control after the input file header has been read.
 * It must determine what output file format is to be written,
 * and make any other decompression parameter changes that are desirable.
 */

METHODDEF void
d_ui_method_selection (decompress_info_ptr cinfo)
{
  /* if grayscale or CMYK input, force similar output; */
  /* else leave the output colorspace as set by options. */
  if (cinfo->jpeg_color_space == CS_GRAYSCALE)
    cinfo->out_color_space = CS_GRAYSCALE;
  else if (cinfo->jpeg_color_space == CS_CMYK)
    cinfo->out_color_space = CS_CMYK;

  /* select output file format */
  /* Note: jselwxxx routine may make additional parameter changes,
   * such as forcing color quantization if it's a colormapped format.
   */
  switch (requested_fmt) {
#ifdef GIF_SUPPORTED
  case FMT_GIF:
    jselwgif(cinfo);
    break;
#endif
#ifdef PPM_SUPPORTED
  case FMT_PPM:
    jselwppm(cinfo);
    break;
#endif
#ifdef RLE_SUPPORTED
  case FMT_RLE:
    jselwrle(cinfo);
    break;
#endif
#ifdef TARGA_SUPPORTED
  case FMT_TARGA:
    jselwtarga(cinfo);
    break;
#endif
  default:
    ERREXIT(cinfo->emethods, "Unsupported output file format");
    break;
  }
}


LOCAL void
usage (char * progname)
/* complain about bad command line */
{
  fprintf(stderr, "usage: %s ", progname);
  fprintf(stderr, "[-G] [-P] [-R] [-T] [-b] [-g] [-q colors] [-2] [-D] [-d]");
#ifdef TWO_FILE_COMMANDLINE
  fprintf(stderr, " inputfile outputfile\n");
#else
  fprintf(stderr, " [inputfile]\n");
#endif
  exit(2);
}


/*
 * The main program.
 */

GLOBAL void
main (int argc, char **argv)
{
  struct decompress_info_struct cinfo;
  struct decompress_methods_struct dc_methods;
  struct external_methods_struct e_methods;
  int c;

  /* On Mac, fetch a command line. */
#ifdef THINK_C
  argc = ccommand(&argv);
#endif

  /* Initialize the system-dependent method pointers. */
  cinfo.methods = &dc_methods;
  cinfo.emethods = &e_methods;
  jselerror(&e_methods);	/* error/trace message routines */
  jselvirtmem(&e_methods);	/* memory allocation routines */
  dc_methods.d_ui_method_selection = d_ui_method_selection;

  /* Set up default JPEG parameters. */
  j_d_defaults(&cinfo, TRUE);
  requested_fmt = DEFAULT_FMT;	/* set default output file format */

  /* Scan command line options, adjust parameters */
  
  while ((c = egetopt(argc, argv, "GPRTbdgq:2D")) != EOF)
    switch (c) {
    case 'G':			/* GIF output format. */
      requested_fmt = FMT_GIF;
      break;
    case 'P':			/* PPM output format. */
      requested_fmt = FMT_PPM;
      break;
    case 'R':			/* RLE output format. */
      requested_fmt = FMT_RLE;
      break;
    case 'T':			/* Targa output format. */
      requested_fmt = FMT_TARGA;
      break;
    case 'b':			/* Enable cross-block smoothing. */
      cinfo.do_block_smoothing = TRUE;
      break;
    case 'd':			/* Debugging. */
      e_methods.trace_level++;
      break;
    case 'g':			/* Force grayscale output. */
      cinfo.out_color_space = CS_GRAYSCALE;
      break;
    case 'q':			/* Do color quantization. */
      { int val;
	if (optarg == NULL)
	  usage(argv[0]);
	if (sscanf(optarg, "%d", &val) != 1)
	  usage(argv[0]);
	cinfo.desired_number_of_colors = val;
      }
      cinfo.quantize_colors = TRUE;
      break;
    case '2':			/* Use two-pass quantization. */
      cinfo.two_pass_quantize = TRUE;
      break;
    case 'D':			/* Suppress dithering in color quantization. */
      cinfo.use_dithering = FALSE;
      break;
    case '?':
    default:
      usage(argv[0]);
      break;
    }

  /* If -d appeared, print version identification */
  if (e_methods.trace_level > 0)
    fprintf(stderr, "Independent JPEG Group's DJPEG, version %s\n%s\n",
	    JVERSION, JCOPYRIGHT);

  /* Select the input and output files */

#ifdef TWO_FILE_COMMANDLINE

  if (optind != argc-2) {
    fprintf(stderr, "%s: must name one input and one output file\n", argv[0]);
    usage(argv[0]);
  }
  if ((cinfo.input_file = fopen(argv[optind], READ_BINARY)) == NULL) {
    fprintf(stderr, "%s: can't open %s\n", argv[0], argv[optind]);
    exit(2);
  }
  if ((cinfo.output_file = fopen(argv[optind+1], WRITE_BINARY)) == NULL) {
    fprintf(stderr, "%s: can't open %s\n", argv[0], argv[optind+1]);
    exit(2);
  }

#else /* not TWO_FILE_COMMANDLINE -- use Unix style */

  cinfo.input_file = stdin;	/* default input file */
  cinfo.output_file = stdout;	/* always the output file */

  if (optind < argc-1) {
    fprintf(stderr, "%s: only one input file\n", argv[0]);
    usage(argv[0]);
  }
  if (optind < argc) {
    if ((cinfo.input_file = fopen(argv[optind], READ_BINARY)) == NULL) {
      fprintf(stderr, "%s: can't open %s\n", argv[0], argv[optind]);
      exit(2);
    }
  }

#endif /* TWO_FILE_COMMANDLINE */
  
  /* Set up to read a JFIF or baseline-JPEG file. */
  /* A smarter UI would inspect the first few bytes of the input file */
  /* to determine its type. */
#ifdef JFIF_SUPPORTED
  jselrjfif(&cinfo);
#else
  You shoulda defined JFIF_SUPPORTED.   /* deliberate syntax error */
#endif

  /* Do it to it! */
  jpeg_decompress(&cinfo);

  /* Release memory. */
  j_d_free_defaults(&cinfo, TRUE);
#ifdef MEM_STATS
  if (e_methods.trace_level > 0) /* Optional memory-usage statistics */
    j_mem_stats();
#endif

  /* All done. */
  exit(0);
}
