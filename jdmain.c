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
#ifdef __STDC__
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


/*
 * If your system has getopt(3), you can use your library version by
 * defining HAVE_GETOPT.  By default, we use the PD 'egetopt'.
 */

#ifdef HAVE_GETOPT
extern int getopt PP((int argc, char **argv, char *optstring));
extern char * optarg;
extern int optind;
#else
#include "egetopt.c"
#define getopt(argc,argv,opt)	egetopt(argc,argv,opt)
#endif


typedef enum {			/* defines known output image formats */
	FMT_PPM,		/* PPM/PGM (PBMPLUS formats) */
	FMT_GIF,		/* GIF format */
	FMT_TIFF		/* TIFF format */
} IMAGE_FORMATS;

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
  default:
    ERREXIT(cinfo->emethods, "Unsupported output file format");
    break;
  }
}


/*
 * Reload the input buffer after it's been emptied, and return the next byte.
 * See the JGETC macro for calling conditions.
 *
 * This routine would need to be replaced if reading JPEG data from something
 * other than a stdio stream.
 */

METHODDEF int
read_jpeg_data (decompress_info_ptr cinfo)
{
  cinfo->bytes_in_buffer = fread(cinfo->input_buffer + MIN_UNGET,
				 1, JPEG_BUF_SIZE,
				 cinfo->input_file);
  
  cinfo->next_input_byte = cinfo->input_buffer + MIN_UNGET;
  
  if (cinfo->bytes_in_buffer <= 0)
    ERREXIT(cinfo->emethods, "Unexpected EOF in JPEG file");

  return JGETC(cinfo);
}



LOCAL void
usage (char * progname)
/* complain about bad command line */
{
  fprintf(stderr, "usage: %s ", progname);
  fprintf(stderr, "[-b] [-q colors] [-2] [-d] [-g] [-G]");
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
  dc_methods.read_jpeg_data = read_jpeg_data;

  /* Allocate memory for input buffer. */
  cinfo.input_buffer = (char *) (*cinfo.emethods->alloc_small)
					((size_t) (JPEG_BUF_SIZE + MIN_UNGET));
  cinfo.bytes_in_buffer = 0;	/* initialize buffer to empty */

  /* Set up default input and output file references. */
  /* (These may be overridden below.) */
  cinfo.input_file = stdin;
  cinfo.output_file = stdout;

  /* Set up default parameters. */
  e_methods.trace_level = 0;
  cinfo.output_gamma = 1.0;
  cinfo.quantize_colors = FALSE;
  cinfo.two_pass_quantize = FALSE;
  cinfo.use_dithering = FALSE;
  cinfo.desired_number_of_colors = 256;
  cinfo.do_block_smoothing = FALSE;
  cinfo.do_pixel_smoothing = FALSE;
  cinfo.out_color_space = CS_RGB;
  cinfo.jpeg_color_space = CS_UNKNOWN;
  /* setting any other value in jpeg_color_space overrides heuristics */
  /* in jrdjfif.c ... */
  /* You may wanta change the default output format; here's the place: */
#ifdef PPM_SUPPORTED
  requested_fmt = FMT_PPM;
#else
  requested_fmt = FMT_GIF;
#endif

  /* Scan parameters */
  
  while ((c = getopt(argc, argv, "bq:2DdgG")) != EOF)
    switch (c) {
    case 'b':			/* Enable cross-block smoothing. */
      cinfo.do_block_smoothing = TRUE;
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
    case 'D':			/* Use dithering in color quantization. */
      cinfo.use_dithering = TRUE;
      break;
    case 'd':			/* Debugging. */
      e_methods.trace_level++;
      break;
    case 'g':			/* Force grayscale output. */
      cinfo.out_color_space = CS_GRAYSCALE;
      break;
    case 'G':			/* GIF output format. */
      requested_fmt = FMT_GIF;
      break;
    case '?':
    default:
      usage(argv[0]);
      break;
    }

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
  (*cinfo.emethods->free_small) ((void *) cinfo.input_buffer);
#ifdef MEM_STATS
  if (e_methods.trace_level > 0)
    j_mem_stats();
#endif

  /* All done. */
  exit(0);
}
