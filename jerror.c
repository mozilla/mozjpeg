/*
 * jerror.c
 *
 * Copyright (C) 1991, Thomas G. Lane.
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
 * the portable JPEG code doesn't use setjmp/longjmp.  However, this won't
 * release allocated memory or close temp files --- some bookkeeping would
 * need to be added to the memory manager module to make that work.
 *
 * These routines are used by both the compression and decompression code.
 */

#include "jinclude.h"
#ifdef INCLUDES_ARE_ANSI
#include <stdlib.h>		/* to declare exit() */
#endif


static external_methods_ptr methods; /* saved for access to message_parm */


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
  trace_message(msgtext);
  exit(1);
}


/*
 * The method selection routine for simple error handling.
 * The system-dependent setup routine should call this routine
 * to install the necessary method pointers in the supplied struct.
 */

GLOBAL void
jselerror (external_methods_ptr emethods)
{
  methods = emethods;		/* save struct addr for msg parm access */

  emethods->error_exit = error_exit;
  emethods->trace_message = trace_message;

  emethods->trace_level = 0;	/* default = no tracing */
}
