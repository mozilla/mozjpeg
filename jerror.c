/*
 * jerror.c
 *
 * Copyright (C) 1991, 1992, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains simple error-reporting and trace-message routines.
 * These are suitable for Unix-like systems and others where writing to
 * stderr is the right thing to do.  If the JPEG software is integrated
 * into a larger application, you may well need to replace these.
 *
 * The error_exit() routine should not return to its caller.  Within a
 * larger application, you might want to have it do a longjmp() to return
 * control to the outer user interface routine.  This should work since
 * the portable JPEG code doesn't use setjmp/longjmp.  You should make sure
 * that free_all is called either within error_exit or after the return to
 * the outer-level routine.
 *
 * These routines are used by both the compression and decompression code.
 */

#include "jinclude.h"
#ifdef INCLUDES_ARE_ANSI
#include <stdlib.h>		/* to declare exit() */
#endif

#ifndef EXIT_FAILURE		/* define exit() codes if not provided */
#define EXIT_FAILURE  1
#endif


static external_methods_ptr methods; /* saved for access to message_parm, free_all */


METHODDEF void
trace_message (const char *msgtext)
{
  fprintf(stderr, msgtext,
	  methods->message_parm[0], methods->message_parm[1],
	  methods->message_parm[2], methods->message_parm[3],
	  methods->message_parm[4], methods->message_parm[5],
	  methods->message_parm[6], methods->message_parm[7]);
  fprintf(stderr, "\n");
}


METHODDEF void
error_exit (const char *msgtext)
{
  (*methods->trace_message) (msgtext);
  (*methods->free_all) ();	/* clean up memory allocation */
  exit(EXIT_FAILURE);
}


/*
 * The method selection routine for simple error handling.
 * The system-dependent setup routine should call this routine
 * to install the necessary method pointers in the supplied struct.
 */

GLOBAL void
jselerror (external_methods_ptr emethods)
{
  methods = emethods;		/* save struct addr for later access */

  emethods->error_exit = error_exit;
  emethods->trace_message = trace_message;

  emethods->trace_level = 0;	/* default = no tracing */

  emethods->num_warnings = 0;	/* no warnings emitted yet */
  /* By default, the first corrupt-data warning will be displayed,
   * but additional ones will appear only if trace level is at least 3.
   * A corrupt data file could generate many warnings, so it's a good idea
   * to suppress additional messages except at high tracing levels.
   */
  emethods->first_warning_level = 0;
  emethods->more_warning_level = 3;
}
