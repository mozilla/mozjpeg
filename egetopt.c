/*
 * egetopt.c -- Extended 'getopt'.
 *
 * A while back, a public-domain version of getopt() was posted to the
 * net.  A bit later, a gentleman by the name of Keith Bostic made some
 * enhancements and reposted it.
 *
 * In recent weeks (i.e., early-to-mid 1988) there's been some
 * heated discussion in comp.lang.c about the merits and drawbacks
 * of getopt(), especially with regard to its handling of '?'.
 *
 * In light of this, I have taken Mr. Bostic's public-domain getopt()
 * and have made some changes that I hope will be considered to be
 * improvements.  I call this routine 'egetopt' ("Extended getopt").
 * The default behavior of this routine is the same as that of getopt(),
 * but it has some optional features that make it more useful.  These
 * options are controlled by the settings of some global variables.
 * By not setting any of these extra global variables, you will have
 * the same functionality as getopt(), which should satisfy those
 * purists who believe getopt() is perfect and can never be improved.
 * If, on the other hand, you are someone who isn't satisfied with the
 * status quo, egetopt() may very well give you the added capabilities
 * you want.
 *
 * Look at the enclosed README file for a description of egetopt()'s
 * new features.
 *
 * The code was originally posted to the net as getopt.c by ...
 *
 *	Keith Bostic
 *	ARPA: keith@seismo 
 *	UUCP: seismo!keith
 *
 * Current version: added enhancements and comments, reformatted code.
 *
 *	Lloyd Zusman
 *	Master Byte Software
 *	Los Gatos, California
 *	Internet:	ljz@fx.com
 *	UUCP:		...!ames!fxgrp!ljz
 *
 *    	May, 1988
 *
 * Modified for use in free JPEG code:
 *
 *	Ed Hanway
 *	UUCP:	uunet!sisd!jeh
 *
 *	October, 1991
 */

/* The original egetopt.c was written not to need stdio.h.
 * For the JPEG code this is an unnecessary and unportable assumption.
 * Also, we make all the variables and routines "static" to avoid
 * possible conflicts with a system-library version of getopt.
 *
 * In the JPEG code, this file is compiled by #including it in jcmain.c
 * or jdmain.c.  Since ANSI2KNR does not process include files, we can't
 * rely on it to convert function definitions to K&R style.  Hence we
 * provide both styles of function header with an explicit #ifdef PROTO (ick).
 */

#define GVAR static		/* make empty to export these variables */

/*
 * None of these constants are referenced in the executable portion of
 * the code ... their sole purpose is to initialize global variables.
 */
#define BADCH		(int)'?'
#define NEEDSEP		(int)':'
#define MAYBESEP	(int)'\0'
#define EMSG		""
#define START		"-"

/*
 * Here are all the pertinent global variables.
 */
GVAR int opterr = 1;		/* if true, output error message */
GVAR int optind = 1;		/* index into parent argv vector */
GVAR int optopt;		/* character checked for validity */
GVAR int optbad = BADCH;	/* character returned on error */
GVAR int optchar = 0;		/* character that begins returned option */
GVAR int optneed = NEEDSEP;	/* flag for mandatory argument */
GVAR int optmaybe = MAYBESEP;	/* flag for optional argument */
GVAR const char *optarg;	/* argument associated with option */
GVAR const char *optstart = START; /* list of characters that start options */


/*
 * Macros.
 */

/*
 * Conditionally print out an error message and return (depends on the
 * setting of 'opterr').
 */
#define TELL(S)	{ \
	if (opterr) \
		fprintf(stderr, "%s%s%c\n", *nargv, (S), optopt); \
	return (optbad); \
}

/*
 * This works similarly to index() and strchr().  I include it so that you
 * don't need to be concerned as to which one your system has.
 */

#ifdef PROTO
LOCAL const char *
_sindex (const char *string, int ch)
#else
LOCAL const char *
_sindex (string, ch)
     const char *string;
     int ch;
#endif
{
	if (string != NULL) {
		for (; *string != '\0'; ++string) {
			if (*string == (char)ch) {
				return (string);
			}
		}
	}

	return (NULL);
}

/*
 * Here it is:
 */

#ifdef PROTO
LOCAL int
egetopt (int nargc, char **nargv, const char *ostr)
#else
LOCAL int
egetopt (nargc, nargv, ostr)
     int nargc;
     char **nargv;
     const char *ostr;
#endif
{
	static const char *place = EMSG; /* option letter processing */
	register const char *oli;	 /* option letter list index */
	register const char *osi = NULL; /* option start list index */

	if (nargv == (char **)NULL) {
		return (EOF);
	}

	if (nargc <= optind || nargv[optind] == NULL) {
		return (EOF);
	}

	if (place == NULL) {
		place = EMSG;
	}

	/*
	 * Update scanning pointer.
	 */
	if (*place == '\0') {
		place = nargv[optind];
		if (place == NULL) {
			return (EOF);
		}
		osi = _sindex(optstart, *place);
		if (osi != NULL) {
			optchar = (int)*osi;
		}
		if (optind >= nargc || osi == NULL || *++place == '\0') {
		    	return (EOF);
		}

		/*
		 * Two adjacent, identical flag characters were found.
		 * This takes care of "--", for example.
		 */
		if (*place == place[-1]) {
			++optind;
			return (EOF);
		}
	}

	/*
	 * If the option is a separator or the option isn't in the list,
	 * we've got an error.
	 */
	optopt = (int)*place++;
	oli = _sindex(ostr, optopt);
	if (optopt == optneed || optopt == optmaybe || oli == NULL) {
		/*
		 * If we're at the end of the current argument, bump the
		 * argument index.
		 */
		if (*place == '\0') {
			++optind;
		}
		TELL(": illegal option -- ");	/* byebye */
	}

	/*
	 * If there is no argument indicator, then we don't even try to
	 * return an argument.
	 */
	++oli;
	if (*oli == '\0' || (*oli != optneed && *oli != optmaybe)) {
		/*
		 * If we're at the end of the current argument, bump the
		 * argument index.
		 */
		if (*place == '\0') {
			++optind;
		}
		optarg = NULL;
	}
	/*
	 * If we're here, there's an argument indicator.  It's handled
	 * differently depending on whether it's a mandatory or an
	 * optional argument.
	 */
	else {
		/*
		 * If there's no white space, use the rest of the
		 * string as the argument.  In this case, it doesn't
		 * matter if the argument is mandatory or optional.
		 */
		if (*place != '\0') {
			optarg = place;
		}
		/*
		 * If we're here, there's whitespace after the option.
		 *
		 * Is it a mandatory argument?  If so, return the
		 * next command-line argument if there is one.
		 */
		else if (*oli == optneed) {
			/*
			 * If we're at the end of the argument list, there
			 * isn't an argument and hence we have an error.
			 * Otherwise, make 'optarg' point to the argument.
			 */
			if (nargc <= ++optind) {
				place = EMSG;
				TELL(": option requires an argument -- ");
			}
			else {
				optarg = nargv[optind];
			}
		}
		/*
		 * If we're here it must have been an optional argument.
		 */
		else {
			if (nargc <= ++optind) {
				place = EMSG;
				optarg = NULL;
			}
			else {
				optarg = nargv[optind];
				if (optarg == NULL) {
					place = EMSG;
				}
				/*
				 * If the next item begins with a flag
				 * character, we treat it like a new
				 * argument.  This is accomplished by
				 * decrementing 'optind' and returning
				 * a null argument.
				 */
				else if (_sindex(optstart, *optarg) != NULL) {
					--optind;
					optarg = NULL;
				}
			}
		}
		place = EMSG;
		++optind;
	}

	/*
	 * Return option letter.
	 */
	return (optopt);
}
