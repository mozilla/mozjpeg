/*
 * jcmain.c
 *
 * Copyright (C) 1991, 1992, 1993, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains a command-line user interface for the JPEG compressor.
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
 * To simplify script writing, the "-outfile" switch is provided.  The syntax
 *	cjpeg [options]  -outfile outputfile  inputfile
 * works regardless of which command line style is used.
 */

#include "jinclude.h"
#ifdef INCLUDES_ARE_ANSI
#include <stdlib.h>		/* to declare exit() */
#endif
#include <ctype.h>		/* to declare isupper(), tolower() */
#ifdef NEED_SIGNAL_CATCHER
#include <signal.h>		/* to declare signal() */
#endif
#ifdef USE_SETMODE
#include <fcntl.h>		/* to declare setmode() */
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

#ifndef EXIT_FAILURE		/* define exit() codes if not provided */
#define EXIT_FAILURE  1
#endif
#ifndef EXIT_SUCCESS
#ifdef VMS
#define EXIT_SUCCESS  1		/* VMS is very nonstandard */
#else
#define EXIT_SUCCESS  0
#endif
#endif


#include "jversion.h"		/* for version message */


/*
 * This routine determines what format the input file is,
 * and selects the appropriate input-reading module.
 *
 * To determine which family of input formats the file belongs to,
 * we may look only at the first byte of the file, since C does not
 * guarantee that more than one character can be pushed back with ungetc.
 * Looking at additional bytes would require one of these approaches:
 *     1) assume we can fseek() the input file (fails for piped input);
 *     2) assume we can push back more than one character (works in
 *        some C implementations, but unportable);
 *     3) provide our own buffering as is done in djpeg (breaks input readers
 *        that want to use stdio directly, such as the RLE library);
 * or  4) don't put back the data, and modify the input_init methods to assume
 *        they start reading after the start of file (also breaks RLE library).
 * #1 is attractive for MS-DOS but is untenable on Unix.
 *
 * The most portable solution for file types that can't be identified by their
 * first byte is to make the user tell us what they are.  This is also the
 * only approach for "raw" file types that contain only arbitrary values.
 * We presently apply this method for Targa files.  Most of the time Targa
 * files start with 0x00, so we recognize that case.  Potentially, however,
 * a Targa file could start with any byte value (byte 0 is the length of the
 * seldom-used ID field), so we provide a switch to force Targa input mode.
 */

static boolean is_targa;	/* records user -targa switch */


LOCAL void
select_file_type (compress_info_ptr cinfo)
{
  int c;

  if (is_targa) {
#ifdef TARGA_SUPPORTED
    jselrtarga(cinfo);
#else
    ERREXIT(cinfo->emethods, "Targa support was not compiled");
#endif
    return;
  }

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
#ifdef RLE_SUPPORTED
  case 'R':
    jselrrle(cinfo);
    break;
#endif
#ifdef TARGA_SUPPORTED
  case 0x00:
    jselrtarga(cinfo);
    break;
#endif
  default:
#ifdef TARGA_SUPPORTED
    ERREXIT(cinfo->emethods, "Unrecognized input file format --- perhaps you need -targa");
#else
    ERREXIT(cinfo->emethods, "Unrecognized input file format");
#endif
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


/*
 * Signal catcher to ensure that temporary files are removed before aborting.
 * NB: for Amiga Manx C this is actually a global routine named _abort();
 * see -Dsignal_catcher=_abort in CFLAGS.  Talk about bogus...
 */

#ifdef NEED_SIGNAL_CATCHER

static external_methods_ptr emethods; /* for access to free_all */

GLOBAL void
signal_catcher (int signum)
{
  if (emethods != NULL) {
    emethods->trace_level = 0;	/* turn off trace output */
    (*emethods->free_all) ();	/* clean up memory allocation & temp files */
  }
  exit(EXIT_FAILURE);
}

#endif


/*
 * Optional routine to display a percent-done figure on stderr.
 * See jcdeflts.c for explanation of the information used.
 */

#ifdef PROGRESS_REPORT

METHODDEF void
progress_monitor (compress_info_ptr cinfo, long loopcounter, long looplimit)
{
  if (cinfo->total_passes > 1) {
    fprintf(stderr, "\rPass %d/%d: %3d%% ",
	    cinfo->completed_passes+1, cinfo->total_passes,
	    (int) (loopcounter*100L/looplimit));
  } else {
    fprintf(stderr, "\r %3d%% ",
	    (int) (loopcounter*100L/looplimit));
  }
  fflush(stderr);
}

#endif


/*
 * Argument-parsing code.
 * The switch parser is designed to be useful with DOS-style command line
 * syntax, ie, intermixed switches and file names, where only the switches
 * to the left of a given file name affect processing of that file.
 * The main program in this file doesn't actually use this capability...
 */


static char * progname;		/* program name for error messages */
static char * outfilename;	/* for -outfile switch */


LOCAL void
usage (void)
/* complain about bad command line */
{
  fprintf(stderr, "usage: %s [switches] ", progname);
#ifdef TWO_FILE_COMMANDLINE
  fprintf(stderr, "inputfile outputfile\n");
#else
  fprintf(stderr, "[inputfile]\n");
#endif

  fprintf(stderr, "Switches (names may be abbreviated):\n");
  fprintf(stderr, "  -quality N     Compression quality (0..100; 5-95 is useful range)\n");
  fprintf(stderr, "  -grayscale     Create monochrome JPEG file\n");
#ifdef ENTROPY_OPT_SUPPORTED
  fprintf(stderr, "  -optimize      Optimize Huffman table (smaller file, but slow compression)\n");
#endif
#ifdef TARGA_SUPPORTED
  fprintf(stderr, "  -targa         Input file is Targa format (usually not needed)\n");
#endif
  fprintf(stderr, "Switches for advanced users:\n");
  fprintf(stderr, "  -restart N     Set restart interval in rows, or in blocks with B\n");
#ifdef INPUT_SMOOTHING_SUPPORTED
  fprintf(stderr, "  -smooth N      Smooth dithered input (N=1..100 is strength)\n");
#endif
  fprintf(stderr, "  -maxmemory N   Maximum memory to use (in kbytes)\n");
  fprintf(stderr, "  -verbose  or  -debug   Emit debug output\n");
  fprintf(stderr, "Switches for wizards:\n");
#ifdef C_ARITH_CODING_SUPPORTED
  fprintf(stderr, "  -arithmetic    Use arithmetic coding\n");
#endif
#ifdef C_MULTISCAN_FILES_SUPPORTED
  fprintf(stderr, "  -nointerleave  Create noninterleaved JPEG file\n");
#endif
  fprintf(stderr, "  -qtables file  Use quantization tables given in file\n");
  fprintf(stderr, "  -sample HxV[,...]  Set JPEG sampling factors\n");
  exit(EXIT_FAILURE);
}


LOCAL boolean
keymatch (char * arg, const char * keyword, int minchars)
/* Case-insensitive matching of (possibly abbreviated) keyword switches. */
/* keyword is the constant keyword (must be lower case already), */
/* minchars is length of minimum legal abbreviation. */
{
  register int ca, ck;
  register int nmatched = 0;

  while ((ca = *arg++) != '\0') {
    if ((ck = *keyword++) == '\0')
      return FALSE;		/* arg longer than keyword, no good */
    if (isupper(ca))		/* force arg to lcase (assume ck is already) */
      ca = tolower(ca);
    if (ca != ck)
      return FALSE;		/* no good */
    nmatched++;			/* count matched characters */
  }
  /* reached end of argument; fail if it's too short for unique abbrev */
  if (nmatched < minchars)
    return FALSE;
  return TRUE;			/* A-OK */
}


LOCAL int
qt_getc (FILE * file)
/* Read next char, skipping over any comments (# to end of line) */
/* A comment/newline sequence is returned as a newline */
{
  register int ch;
  
  ch = getc(file);
  if (ch == '#') {
    do {
      ch = getc(file);
    } while (ch != '\n' && ch != EOF);
  }
  return ch;
}


LOCAL long
read_qt_integer (FILE * file)
/* Read an unsigned decimal integer from a quantization-table file */
/* Swallows one trailing character after the integer */
{
  register int ch;
  register long val;
  
  /* Skip any leading whitespace, detect EOF */
  do {
    ch = qt_getc(file);
    if (ch == EOF)
      return EOF;
  } while (isspace(ch));
  
  if (! isdigit(ch)) {
    fprintf(stderr, "%s: bogus data in quantization file\n", progname);
    exit(EXIT_FAILURE);
  }

  val = ch - '0';
  while (ch = qt_getc(file), isdigit(ch)) {
    val *= 10;
    val += ch - '0';
  }
  return val;
}


LOCAL void
read_quant_tables (compress_info_ptr cinfo, char * filename, int scale_factor)
/* Read a set of quantization tables from the specified file.
 * The file is plain ASCII text: decimal numbers with whitespace between.
 * Comments preceded by '#' may be included in the file.
 * There may be one to NUM_QUANT_TBLS tables in the file, each of 64 values.
 * The tables are implicitly numbered 0,1,etc.
 */
{
  /* ZIG[i] is the zigzag-order position of the i'th element of a DCT block */
  /* read in natural order (left to right, top to bottom). */
  static const short ZIG[DCTSIZE2] = {
     0,  1,  5,  6, 14, 15, 27, 28,
     2,  4,  7, 13, 16, 26, 29, 42,
     3,  8, 12, 17, 25, 30, 41, 43,
     9, 11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63
    };
  FILE * fp;
  int tblno, i;
  long val;
  QUANT_TBL table;

  if ((fp = fopen(filename, "r")) == NULL) {
    fprintf(stderr, "%s: can't open %s\n", progname, filename);
    exit(EXIT_FAILURE);
  }
  tblno = 0;

  while ((val = read_qt_integer(fp)) != EOF) { /* read 1st element of table */
    if (tblno >= NUM_QUANT_TBLS) {
      fprintf(stderr, "%s: too many tables in file %s\n", progname, filename);
      exit(EXIT_FAILURE);
    }
    table[0] = (QUANT_VAL) val;
    for (i = 1; i < DCTSIZE2; i++) {
      if ((val = read_qt_integer(fp)) == EOF) {
	fprintf(stderr, "%s: incomplete table in file %s\n", progname, filename);
	exit(EXIT_FAILURE);
      }
      table[ZIG[i]] = (QUANT_VAL) val;
    }
    j_add_quant_table(cinfo, tblno, table, scale_factor, FALSE);
    tblno++;
  }

  fclose(fp);
}


LOCAL void
set_sample_factors (compress_info_ptr cinfo, char *arg)
/* Process a sample-factors parameter string, of the form */
/*     HxV[,HxV,...]    */
{
#define MAX_COMPONENTS 4	/* # of comp_info slots made by jcdeflts.c */
  int ci, val1, val2;
  char ch1, ch2;

  for (ci = 0; ci < MAX_COMPONENTS; ci++) {
    if (*arg) {
      ch2 = ',';		/* if not set by sscanf, will be ',' */
      if (sscanf(arg, "%d%c%d%c", &val1, &ch1, &val2, &ch2) < 3)
	usage();
      if ((ch1 != 'x' && ch1 != 'X') || ch2 != ',')
	usage();		/* syntax check */
      if (val1 <= 0 || val1 > 4 || val2 <= 0 || val2 > 4) {
	fprintf(stderr, "JPEG sampling factors must be 1..4\n");
	exit(EXIT_FAILURE);
      }
      cinfo->comp_info[ci].h_samp_factor = val1;
      cinfo->comp_info[ci].v_samp_factor = val2;
      while (*arg && *arg++ != ',') /* advance to next segment of arg string */
	;
    } else {
      /* reached end of parameter, set remaining components to 1x1 sampling */
      cinfo->comp_info[ci].h_samp_factor = 1;
      cinfo->comp_info[ci].v_samp_factor = 1;
    }
  }
}


LOCAL int
parse_switches (compress_info_ptr cinfo, int last_file_arg_seen,
		int argc, char **argv)
/* Initialize cinfo with default switch settings, then parse option switches.
 * Returns argv[] index of first file-name argument (== argc if none).
 * Any file names with indexes <= last_file_arg_seen are ignored;
 * they have presumably been processed in a previous iteration.
 * (Pass 0 for last_file_arg_seen on the first or only iteration.)
 */
{
  int argn;
  char * arg;
  char * qtablefile = NULL;	/* saves -qtables filename if any */
  int q_scale_factor = 100;	/* default to no scaling for -qtables */

  /* (Re-)initialize the system-dependent error and memory managers. */
  jselerror(cinfo->emethods);	/* error/trace message routines */
  jselmemmgr(cinfo->emethods);	/* memory allocation routines */
  cinfo->methods->c_ui_method_selection = c_ui_method_selection;

  /* Now OK to enable signal catcher. */
#ifdef NEED_SIGNAL_CATCHER
  emethods = cinfo->emethods;
#endif

  /* Set up default JPEG parameters. */
  /* Note that default -quality level here need not, and does not,
   * match the default scaling for an explicit -qtables argument.
   */
  j_c_defaults(cinfo, 75, FALSE); /* default quality level = 75 */
  is_targa = FALSE;
  outfilename = NULL;

  /* Scan command line options, adjust parameters */

  for (argn = 1; argn < argc; argn++) {
    arg = argv[argn];
    if (*arg != '-') {
      /* Not a switch, must be a file name argument */
      if (argn <= last_file_arg_seen) {
	outfilename = NULL;	/* -outfile applies to just one input file */
	continue;		/* ignore this name if previously processed */
      }
      break;			/* else done parsing switches */
    }
    arg++;			/* advance past switch marker character */

    if (keymatch(arg, "arithmetic", 1)) {
      /* Use arithmetic coding. */
#ifdef C_ARITH_CODING_SUPPORTED
      cinfo->arith_code = TRUE;
#else
      fprintf(stderr, "%s: sorry, arithmetic coding not supported\n",
	      progname);
      exit(EXIT_FAILURE);
#endif

    } else if (keymatch(arg, "debug", 1) || keymatch(arg, "verbose", 1)) {
      /* Enable debug printouts. */
      /* On first -d, print version identification */
      if (last_file_arg_seen == 0 && cinfo->emethods->trace_level == 0)
	fprintf(stderr, "Independent JPEG Group's CJPEG, version %s\n%s\n",
		JVERSION, JCOPYRIGHT);
      cinfo->emethods->trace_level++;

    } else if (keymatch(arg, "grayscale", 2) || keymatch(arg, "greyscale",2)) {
      /* Force a monochrome JPEG file to be generated. */
      j_monochrome_default(cinfo);

    } else if (keymatch(arg, "maxmemory", 1)) {
      /* Maximum memory in Kb (or Mb with 'm'). */
      long lval;
      char ch = 'x';

      if (++argn >= argc)	/* advance to next argument */
	usage();
      if (sscanf(argv[argn], "%ld%c", &lval, &ch) < 1)
	usage();
      if (ch == 'm' || ch == 'M')
	lval *= 1000L;
      cinfo->emethods->max_memory_to_use = lval * 1000L;

    } else if (keymatch(arg, "nointerleave", 3)) {
      /* Create noninterleaved file. */
#ifdef C_MULTISCAN_FILES_SUPPORTED
      cinfo->interleave = FALSE;
#else
      fprintf(stderr, "%s: sorry, multiple-scan support was not compiled\n",
	      progname);
      exit(EXIT_FAILURE);
#endif

    } else if (keymatch(arg, "optimize", 1) || keymatch(arg, "optimise", 1)) {
      /* Enable entropy parm optimization. */
#ifdef ENTROPY_OPT_SUPPORTED
      cinfo->optimize_coding = TRUE;
#else
      fprintf(stderr, "%s: sorry, entropy optimization was not compiled\n",
	      progname);
      exit(EXIT_FAILURE);
#endif

    } else if (keymatch(arg, "outfile", 3)) {
      /* Set output file name. */
      if (++argn >= argc)	/* advance to next argument */
	usage();
      outfilename = argv[argn];	/* save it away for later use */

    } else if (keymatch(arg, "quality", 1)) {
      /* Quality factor (quantization table scaling factor). */
      int val;

      if (++argn >= argc)	/* advance to next argument */
	usage();
      if (sscanf(argv[argn], "%d", &val) != 1)
	usage();
      /* Set quantization tables (will be overridden if -qtables also given).
       * Note: we make force_baseline FALSE.
       * This means non-baseline JPEG files can be created with low Q values.
       * To ensure only baseline files are generated, pass TRUE instead.
       */
      j_set_quality(cinfo, val, FALSE);
      /* Change scale factor in case -qtables is present. */
      q_scale_factor = j_quality_scaling(val);

    } else if (keymatch(arg, "qtables", 2)) {
      /* Quantization tables fetched from file. */
      if (++argn >= argc)	/* advance to next argument */
	usage();
      qtablefile = argv[argn];
      /* we postpone actually reading the file in case -quality comes later */

    } else if (keymatch(arg, "restart", 1)) {
      /* Restart interval in MCU rows (or in MCUs with 'b'). */
      long lval;
      char ch = 'x';

      if (++argn >= argc)	/* advance to next argument */
	usage();
      if (sscanf(argv[argn], "%ld%c", &lval, &ch) < 1)
	usage();
      if (lval < 0 || lval > 65535L)
	usage();
      if (ch == 'b' || ch == 'B')
	cinfo->restart_interval = (UINT16) lval;
      else
	cinfo->restart_in_rows = (int) lval;

    } else if (keymatch(arg, "sample", 2)) {
      /* Set sampling factors. */
      if (++argn >= argc)	/* advance to next argument */
	usage();
      set_sample_factors(cinfo, argv[argn]);

    } else if (keymatch(arg, "smooth", 2)) {
      /* Set input smoothing factor. */
      int val;

      if (++argn >= argc)	/* advance to next argument */
	usage();
      if (sscanf(argv[argn], "%d", &val) != 1)
	usage();
      if (val < 0 || val > 100)
	usage();
      cinfo->smoothing_factor = val;

    } else if (keymatch(arg, "targa", 1)) {
      /* Input file is Targa format. */
      is_targa = TRUE;

    } else {
      usage();			/* bogus switch */
    }
  }

  /* Post-switch-scanning cleanup */

  if (qtablefile != NULL)	/* process -qtables if it was present */
    read_quant_tables(cinfo, qtablefile, q_scale_factor);

  return argn;			/* return index of next arg (file name) */
}


/*
 * The main program.
 */

GLOBAL int
main (int argc, char **argv)
{
  struct Compress_info_struct cinfo;
  struct Compress_methods_struct c_methods;
  struct External_methods_struct e_methods;
  int file_index;

  /* On Mac, fetch a command line. */
#ifdef THINK_C
  argc = ccommand(&argv);
#endif

  progname = argv[0];

  /* Set up links to method structures. */
  cinfo.methods = &c_methods;
  cinfo.emethods = &e_methods;

  /* Install, but don't yet enable signal catcher. */
#ifdef NEED_SIGNAL_CATCHER
  emethods = NULL;
  signal(SIGINT, signal_catcher);
#ifdef SIGTERM			/* not all systems have SIGTERM */
  signal(SIGTERM, signal_catcher);
#endif
#endif

  /* Scan command line: set up compression parameters, find file names. */

  file_index = parse_switches(&cinfo, 0, argc, argv);

#ifdef TWO_FILE_COMMANDLINE
  /* Must have either -outfile switch or explicit output file name */
  if (outfilename == NULL) {
    if (file_index != argc-2) {
      fprintf(stderr, "%s: must name one input and one output file\n",
	      progname);
      usage();
    }
    outfilename = argv[file_index+1];
  } else {
    if (file_index != argc-1) {
      fprintf(stderr, "%s: must name one input and one output file\n",
	      progname);
      usage();
    }
  }
#else
  /* Unix style: expect zero or one file name */
  if (file_index < argc-1) {
    fprintf(stderr, "%s: only one input file\n", progname);
    usage();
  }
#endif /* TWO_FILE_COMMANDLINE */

  /* Open the input file. */
  if (file_index < argc) {
    if ((cinfo.input_file = fopen(argv[file_index], READ_BINARY)) == NULL) {
      fprintf(stderr, "%s: can't open %s\n", progname, argv[file_index]);
      exit(EXIT_FAILURE);
    }
  } else {
    /* default input file is stdin */
#ifdef USE_SETMODE		/* need to hack file mode? */
    setmode(fileno(stdin), O_BINARY);
#endif
#ifdef USE_FDOPEN		/* need to re-open in binary mode? */
    if ((cinfo.input_file = fdopen(fileno(stdin), READ_BINARY)) == NULL) {
      fprintf(stderr, "%s: can't open stdin\n", progname);
      exit(EXIT_FAILURE);
    }
#else
    cinfo.input_file = stdin;
#endif
  }

  /* Open the output file. */
  if (outfilename != NULL) {
    if ((cinfo.output_file = fopen(outfilename, WRITE_BINARY)) == NULL) {
      fprintf(stderr, "%s: can't open %s\n", progname, outfilename);
      exit(EXIT_FAILURE);
    }
  } else {
    /* default output file is stdout */
#ifdef USE_SETMODE		/* need to hack file mode? */
    setmode(fileno(stdout), O_BINARY);
#endif
#ifdef USE_FDOPEN		/* need to re-open in binary mode? */
    if ((cinfo.output_file = fdopen(fileno(stdout), WRITE_BINARY)) == NULL) {
      fprintf(stderr, "%s: can't open stdout\n", progname);
      exit(EXIT_FAILURE);
    }
#else
    cinfo.output_file = stdout;
#endif
  }

  /* Figure out the input file format, and set up to read it. */
  select_file_type(&cinfo);

#ifdef PROGRESS_REPORT
  /* Start up progress display, unless trace output is on */
  if (e_methods.trace_level == 0)
    c_methods.progress_monitor = progress_monitor;
#endif

  /* Do it to it! */
  jpeg_compress(&cinfo);

#ifdef PROGRESS_REPORT
  /* Clear away progress display */
  if (e_methods.trace_level == 0) {
    fprintf(stderr, "\r                \r");
    fflush(stderr);
  }
#endif

  /* All done. */
  exit(EXIT_SUCCESS);
  return 0;			/* suppress no-return-value warnings */
}
