/*
 * jcmain.c
 *
 * Copyright (C) 1991, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains a trivial test user interface for the JPEG compressor.
 * It should work on any system with Unix- or MS-DOS-style command lines.
 *
 * Two different command line styles are permitted, depending on the
 * compile-time switch TWO_FILE_COMMANDLINE:
 *	cjpeg [options]  inputfile outputfile
 *	cjpeg [options]  [inputfile]
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


/*
 * This routine determines what format the input file is,
 * and selects the appropriate input-reading module.
 *
 * To determine which family of input formats the file belongs to,
 * we look only at the first byte of the file, since C does not
 * guarantee that more than one character can be pushed back with ungetc.
 * This is sufficient for the currently envisioned set of input formats.
 *
 * If you need to look at more than one character to select an input module,
 * you can either
 *     1) assume you can fseek() the input file (may fail for piped input);
 *     2) assume you can push back more than one character (works in
 *        some C implementations, but unportable);
 * or  3) don't put back the data, and modify the various input_init
 *        methods to assume they start reading after the start of file.
 */

LOCAL void
select_file_type (compress_info_ptr cinfo)
{
  int c;

  if ((c = getc(cinfo->input_file)) == EOF)
    ERREXIT(cinfo->emethods, "Empty input file");

  switch (c) {
#ifdef GIF_SUPPORTED
  case 'G':
    jselrgif(cinfo);
    break;
#endif
#ifdef PPM_SUPPORTED
  case 'P':
    jselrppm(cinfo);
    break;
#endif
  default:
    ERREXIT(cinfo->emethods, "Unsupported input file format");
    break;
  }

  if (ungetc(c, cinfo->input_file) == EOF)
    ERREXIT(cinfo->emethods, "ungetc failed");
}


/*
 * This routine gets control after the input file header has been read.
 * It must determine what output JPEG file format is to be written,
 * and make any other compression parameter changes that are desirable.
 */

METHODDEF void
c_ui_method_selection (compress_info_ptr cinfo)
{
  /* If the input is gray scale, generate a monochrome JPEG file. */
  if (cinfo->in_color_space == CS_GRAYSCALE)
    j_monochrome_default(cinfo);
  /* For now, always select JFIF output format. */
#ifdef JFIF_SUPPORTED
  jselwjfif(cinfo);
#else
  You shoulda defined JFIF_SUPPORTED.   /* deliberate syntax error */
#endif
}


LOCAL void
usage (char * progname)
/* complain about bad command line */
{
  fprintf(stderr, "usage: %s ", progname);
  fprintf(stderr, "[-I] [-Q quality 0..100] [-a] [-o] [-d]");
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
  struct compress_info_struct cinfo;
  struct compress_methods_struct c_methods;
  struct external_methods_struct e_methods;
  int c;

  /* On Mac, fetch a command line. */
#ifdef THINK_C
  argc = ccommand(&argv);
#endif

  /* Initialize the system-dependent method pointers. */
  cinfo.methods = &c_methods;
  cinfo.emethods = &e_methods;
  jselerror(&e_methods);	/* error/trace message routines */
  jselvirtmem(&e_methods);	/* memory allocation routines */
  c_methods.c_ui_method_selection = c_ui_method_selection;

  /* Set up default input and output file references. */
  /* (These may be overridden below.) */
  cinfo.input_file = stdin;
  cinfo.output_file = stdout;

  /* Set up default parameters. */
  e_methods.trace_level = 0;
  j_default_compression(&cinfo, 75); /* default quality level */

  /* Scan parameters */
  
  while ((c = getopt(argc, argv, "IQ:aod")) != EOF)
    switch (c) {
    case 'I':			/* Create noninterleaved file. */
#ifdef MULTISCAN_FILES_SUPPORTED
      cinfo.interleave = FALSE;
#else
      fprintf(stderr, "%s: sorry, multiple-scan support was not compiled\n",
	      argv[0]);
      exit(2);
#endif
      break;
    case 'Q':			/* Quality factor. */
      { int val;
	if (optarg == NULL)
	  usage(argv[0]);
	if (sscanf(optarg, "%d", &val) != 1)
	  usage(argv[0]);
	/* Note: for now, we leave force_baseline FALSE.
	 * In a production user interface, probably should make it TRUE
	 * unless overridden by a separate switch.
	 */
	j_set_quality(&cinfo, val, FALSE);
      }
      break;
    case 'a':			/* Use arithmetic coding. */
#ifdef ARITH_CODING_SUPPORTED
      cinfo.arith_code = TRUE;
#else
      fprintf(stderr, "%s: sorry, arithmetic coding not supported\n",
	      argv[0]);
      exit(2);
#endif
      break;
    case 'o':			/* Enable entropy parm optimization. */
#ifdef ENTROPY_OPT_SUPPORTED
      cinfo.optimize_coding = TRUE;
#else
      fprintf(stderr, "%s: sorry, entropy optimization was not compiled\n",
	      argv[0]);
      exit(2);
#endif
      break;
    case 'd':			/* Debugging. */
      e_methods.trace_level++;
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

  /* Figure out the input file format, and set up to read it. */
  select_file_type(&cinfo);

  /* Do it to it! */
  jpeg_compress(&cinfo);

  /* Release memory. */
  j_free_defaults(&cinfo);
#ifdef MEM_STATS
  if (e_methods.trace_level > 0)
    j_mem_stats();
#endif

  /* All done. */
  exit(0);
}
