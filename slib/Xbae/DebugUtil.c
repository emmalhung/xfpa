/**
 *
 * $Header: /cvsroot/xbae/Xbae/src/DebugUtil.c,v 1.19 2005/04/02 16:56:24 tobiasoed Exp $
 * 
 * Copyright (C) 1995 Free Software Foundation, Inc.
 * Copyright © 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004 LessTif Development Team 
 *
 * This file is part of the GNU LessTif Library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 **/

static const char rcsid[] =
	"$Header: /cvsroot/xbae/Xbae/src/DebugUtil.c,v 1.19 2005/04/02 16:56:24 tobiasoed Exp $";

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif
#include <unistd.h>

#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/Xresource.h>

#include <Xm/XmP.h>
#include <Xm/DrawingAP.h>
#include <Xm/Display.h>
#include <Xm/MwmUtil.h>

#include <Xbae/Utils.h>

#include "XbaeDebug.h"

#ifndef	XmUNSPECIFIED
#  define	XmUNSPECIFIED	(~0)
#endif

/* 
 * All external interfaces need to be in place for both, 
 * debug and production build. We may just reduce them
 * to empty stubs ... 
 */

/*
 *  some #defines being used here
 */
#undef DEBUGSIG					/* debug the 'signal feature' */
#undef DEBUGVALIDATE			/* debug the ValidateSource(), etc. routines */
#undef PRINT_STATE				/* add info to __XbaeDebugPrintTree() output */


/*
 * Functionality in this file is influenced at run time by these
 * environment variables :
 *
 * - XBAE_DEBUG_SOURCES : colon-separated list of files from which debug output
 *      is generated.
 *      Special values "all" and "none" have obvious meaning.
 *      You may block file from the list by adding a "-" prefix,
 *      this is obviously only useful in conjunction with "all",
 *      e.g. XBAE_DEBUG_SOURCES=all:-Vendor.c:-XmString
 *      Using an asterisk as a wildcard is also supported, e.g.
 *      XBAE_DEBUG_SOURCES=Text*
 *      The code may not be failsafe for some pathological combinations,
 *      but we don't expect anyone to debug the debugging code ... ;-)
 *
 * - XBAE_DEBUG_PRINT_WIDGETID : if this variable exists, then all widgets printed
 *      with _XbaeDebug etc. will also print their widget ID. If the variable
 *      doesn't exist, then they only print their name for identification.
 *
 * - XBAE_DEBUG_FILE : if this variable exists, then the file is used for output.
 *      If "%p" is part of the value of XBAE_DEBUG_FILE, then it is replaced by the
 *      process id.
 *      "stdout" and "stderr" are recognized and have their special
 *      obvious meaning.
 *
 * - XBAE_DEBUG_SIGNAL: define to the macro which belongs to the signal which
 *      should be accepted by the program as a switch to toggle
 *      debugging on/off. No value or "none" will turn the feature off.
 *      Valid values are compile time dependent (WRT libXm),
 *      check below what's actually supported.
 *
 *  - XBAE_DEBUG_TOGGLE: initial value whether debugging should be
 *      enabled or disabled upon program start. Valid Values are on/off.
 *      e.g. XBAE_DEBUG_TOGGLE=off
 *
 * If the C macro XBAE_PRODUCTION is defined, then _XbaeDebug etc. don't
 *      work. Note: this is a compile time option.
 * To have maximum performance, sequences of _XbaeDebug statements should be
 *      surrounded by an if (_XbaeDebugInDebug(__FILE__, w)) statement.
 *      _XbaeDebugInDebug is False when XBAE_PRODUCTION is defined.
 */

#ifndef XBAE_PRODUCTION
static Boolean _XbaeDebugFlag = True;
static Boolean _XbaeDebugPrintWidgetID = False;
static FILE *_XbaeDebugFile = NULL;
typedef void (*sighandler_t) (int);
#endif

#define	_XbaeDebugNONE			0
#define	_XbaeDebugINT			1
#define	_XbaeDebugSTRING			2
#define	_XbaeDebugXMSTRING		3
#define	_XbaeDebugCHAR			4
#define	_XbaeDebugSHORT			5
#define	_XbaeDebugATTACHMENT		6
#define	_XbaeDebugWIDGET			7
#define	_XbaeDebugBOOLEAN			8
#define	_XbaeDebugSELECTION_POLICY	9
#define	_XbaeDebugXMSTRING_LIST		10	/* bingo */
#define _XbaeDebugDIALOG_STYLE		11
#define _XbaeDebugEDIT_MODE		12
#define _XbaeDebugALIGNMENT		13
#define _XbaeDebugSB_DISPLAY_POLICY	14
#define _XbaeDebugLIST_SIZE_POLICY	15
#define _XbaeDebugSB_PLACEMENT		16
#define _XbaeDebugRESIZE_POLICY		17
#define _XbaeDebugCOMBOBOX_TYPE		18
#define _XbaeDebugSCROLLING_POLICY	19
#define _XbaeDebugDRAG_TYPE		20
#define	_XbaeDebugMWM_INPUT_MODE		21
#define	_XbaeDebugDELETE_RESPONSE		22



/* *INDENT-OFF* */

static const struct 
{
    const char *name;
    int t;
    const char *related;
} _XbaeDebugTypes[] = 
{
    { XmNdragInitiatorProtocolStyle, _XbaeDebugDRAG_TYPE, NULL } ,
    { XmNdragReceiverProtocolStyle, _XbaeDebugDRAG_TYPE, NULL } ,
    { XmNscrollingPolicy, _XbaeDebugSCROLLING_POLICY, NULL } ,
    { XmNmaximum, _XbaeDebugINT, NULL } ,
    { XmNminimum, _XbaeDebugINT, NULL } ,
    { XmNsliderSize, _XbaeDebugINT, NULL } ,
    { XmNpageIncrement, _XbaeDebugINT, NULL } ,
    { XmNincrement, _XbaeDebugINT, NULL } ,
    { XmNx, _XbaeDebugINT, NULL } ,
    { XmNy, _XbaeDebugINT, NULL } ,
    { XmNwidth, _XbaeDebugINT, NULL } ,
    { XmNheight, _XbaeDebugINT, NULL } ,
    { XmNlabelString, _XbaeDebugXMSTRING, NULL } ,
    { XmNmessageString, _XbaeDebugXMSTRING, NULL } ,
    { XmNrowColumnType, _XbaeDebugNONE, NULL } ,
    { XmNbuttonSet, _XbaeDebugNONE, NULL } ,
    { XmNbuttonCount, _XbaeDebugINT, NULL } ,
    { XmNoptionLabel, _XbaeDebugXMSTRING, NULL } ,
    { XmNdirectory, _XbaeDebugXMSTRING, NULL } ,
    { XmNoptionMnemonic, _XbaeDebugCHAR, NULL } ,
    { XmNrows, _XbaeDebugSHORT, NULL } ,
    { XmNcolumns, _XbaeDebugSHORT, NULL } ,
    { XmNmarginWidth, _XbaeDebugINT, NULL } ,
    { XmNmarginHeight, _XbaeDebugINT, NULL } ,
    { XmNmarginTop, _XbaeDebugINT, NULL } ,
    { XmNmarginBottom, _XbaeDebugINT, NULL } ,
    { XmNmarginLeft, _XbaeDebugINT, NULL } ,
    { XmNmarginRight, _XbaeDebugINT, NULL } ,
    { XmNselectionArrayCount, _XbaeDebugINT, NULL } ,
    { XmNshadowThickness, _XbaeDebugINT, NULL } ,
    { XmNhighlightThickness, _XbaeDebugINT, NULL } ,
    { XmNtopAttachment, _XbaeDebugATTACHMENT, NULL } ,
    { XmNbottomAttachment, _XbaeDebugATTACHMENT, NULL } ,
    { XmNleftAttachment, _XbaeDebugATTACHMENT, NULL } ,
    { XmNrightAttachment, _XbaeDebugATTACHMENT, NULL } ,
    { XmNtopOffset, _XbaeDebugINT, NULL } ,
    { XmNbottomOffset, _XbaeDebugINT, NULL } ,
    { XmNleftOffset, _XbaeDebugINT, NULL } ,
    { XmNrightOffset, _XbaeDebugINT, NULL } ,
    { XmNtopPosition, _XbaeDebugINT, NULL } ,
    { XmNbottomPosition, _XbaeDebugINT, NULL } ,
    { XmNleftPosition, _XbaeDebugINT, NULL } ,
    { XmNrightPosition, _XbaeDebugINT, NULL } ,
    { XmNdefaultButton, _XbaeDebugWIDGET, NULL } ,
    { XmNmessageWindow, _XbaeDebugWIDGET, NULL } ,
    { XmNtopWidget, _XbaeDebugWIDGET, NULL } ,
    { XmNbottomWidget, _XbaeDebugWIDGET, NULL } ,
    { XmNleftWidget, _XbaeDebugWIDGET, NULL } ,
    { XmNrightWidget, _XbaeDebugWIDGET, NULL } ,
    { XmNsensitive, _XbaeDebugBOOLEAN, NULL } ,
    { XmNresizable, _XbaeDebugBOOLEAN, NULL } ,
    { XmNmustMatch, _XbaeDebugBOOLEAN, NULL } ,
    { XmNresizeHeight, _XbaeDebugBOOLEAN, NULL } ,
    { XmNresizeWidth, _XbaeDebugBOOLEAN, NULL } ,
    { XmNfractionBase, _XbaeDebugINT, NULL } ,
    { XmNhorizontalSpacing, _XbaeDebugINT, NULL } ,
    { XmNverticalSpacing, _XbaeDebugINT, NULL } ,
    { XmNrubberPositioning, _XbaeDebugBOOLEAN, NULL } ,
    { XmNitemCount, _XbaeDebugINT, NULL } ,
    { XmNfileListItemCount, _XbaeDebugINT, NULL } ,
    { XmNtextString, _XbaeDebugXMSTRING, NULL } ,
    { XmNdirSpec, _XbaeDebugXMSTRING, NULL } ,
    { XmNdirMask, _XbaeDebugXMSTRING, NULL } ,
    { XmNitems, _XbaeDebugXMSTRING_LIST, XmNitemCount } ,                       /* bingo */
    { XmNselectionPolicy, _XbaeDebugSELECTION_POLICY, NULL } ,
    { XmNautoUnmanage, _XbaeDebugBOOLEAN, NULL } ,
    { XmNdialogStyle, _XbaeDebugDIALOG_STYLE, NULL } ,
    { XmNshowAsDefault, _XbaeDebugSHORT, NULL } ,
    { XmNeditable, _XbaeDebugBOOLEAN, NULL } ,
    { XmNmaxLength, _XbaeDebugINT, NULL } ,
    { XmNdirListItemCount, _XbaeDebugINT, NULL } ,
    { XmNfileListItemCount, _XbaeDebugINT, NULL } ,
    { XmNeditMode, _XbaeDebugEDIT_MODE, NULL } ,
    { XmNalignment, _XbaeDebugALIGNMENT, NULL } ,
    { XmNrecomputeSize, _XbaeDebugBOOLEAN, NULL } ,
    { XmNdirectoryValid, _XbaeDebugBOOLEAN, NULL } ,
    { XmNlistUpdated, _XbaeDebugBOOLEAN, NULL } ,
    { XmNhorizontalScrollBar, _XbaeDebugWIDGET, NULL } ,
    { XmNverticalScrollBar, _XbaeDebugWIDGET, NULL } ,
    { XmNworkWindow, _XbaeDebugWIDGET, NULL } ,
    { XmNmenuBar, _XbaeDebugWIDGET, NULL } ,
    { XmNcommandWindow, _XbaeDebugWIDGET, NULL } ,
    { XmNvisibleItemCount, _XbaeDebugSHORT, NULL } ,
    { XmNdefaultButtonShadowThickness, _XbaeDebugSHORT, NULL } ,
    { XmNset, _XbaeDebugBOOLEAN, NULL } ,
    { XmNtraversalOn, _XbaeDebugBOOLEAN, NULL } ,
    { XmNspacing, _XbaeDebugSHORT, NULL } ,
    { XmNscrollBarDisplayPolicy, _XbaeDebugSB_DISPLAY_POLICY, NULL } ,
    { XmNlistSizePolicy, _XbaeDebugLIST_SIZE_POLICY, NULL } ,
    { XmNscrollBarPlacement, _XbaeDebugSB_PLACEMENT, NULL } ,
    { XmNuserData, _XbaeDebugNONE, NULL } ,
    { XmNallowShellResize, _XbaeDebugBOOLEAN, NULL } ,
    { XmNresizePolicy, _XbaeDebugRESIZE_POLICY, NULL } ,
    { XmNradioBehavior, _XbaeDebugBOOLEAN, NULL } ,
    { XmNradioAlwaysOne, _XbaeDebugBOOLEAN, NULL } ,
    { XmNnumColumns, _XbaeDebugSHORT, NULL } ,
    { XmNinitialFocus, _XbaeDebugWIDGET, NULL } ,
    { XmNmwmInputMode, _XbaeDebugMWM_INPUT_MODE, NULL } ,
    { XmNmappedWhenManaged, _XbaeDebugBOOLEAN, NULL } ,
    { XmNdeleteResponse, _XbaeDebugDELETE_RESPONSE, NULL } ,
    { XmNwidthInc, _XbaeDebugSHORT, NULL } ,
    { XmNheightInc, _XbaeDebugSHORT, NULL } ,
    { XmNbaseWidth, _XbaeDebugSHORT, NULL } ,
    { XmNbaseHeight, _XbaeDebugSHORT, NULL } ,
    { XmNminWidth, _XbaeDebugSHORT, NULL } ,
    { XmNminHeight, _XbaeDebugSHORT, NULL } ,
    { XmNtitle, _XbaeDebugSTRING, NULL } ,
    { XmNiconName, _XbaeDebugSTRING, NULL } ,
    { XmNcancelLabelString, _XbaeDebugXMSTRING, NULL } ,
    { XmNcomboBoxType, _XbaeDebugCOMBOBOX_TYPE, NULL } ,
    { XmNlargeCellWidth, _XbaeDebugINT, NULL } ,
    { XmNlargeCellHeight, _XbaeDebugINT, NULL } ,
    { XmNacceleratorText,      _XbaeDebugXMSTRING,     NULL },
    { NULL, 0, NULL }
    /* the end */
};

/* *INDENT-ON* */

/**************************************************************************************************/

#ifndef XBAE_PRODUCTION
/* catches the signal defined by XBAE_DEBUG_SIGNAL and toggles the
   global debugging flag.
   Avoid calling C-runtime functions from within here if possible 
   (which in turn might also raise signals) */
static void sighandler(int signo)
{

#  ifdef DEBUGSIG
	fputs("sighandler(): signal caught\n", stderr);
#  endif

	/* switch debugging on/off */
	_XbaeDebugToggle();
	/* re-install ourselves: perhaps not always necessary, but OTOH
	   it shouldn't hurt!? */
	signal(signo, sighandler);
}
#endif /* XBAE_PRODUCTION */


#ifndef XBAE_PRODUCTION
static Boolean siginstall(void)
{
	const char *ptr;
#  define NOSIG -1
	int signo = NOSIG;

	ptr = getenv("XBAE_DEBUG_SIGNAL");
	if(ptr)
	{
#  ifdef DEBUGSIG
		fprintf(stderr, "siginstall(): trying to catch %s\n", ptr);
#  endif
		if((*ptr == '\0') || (_xbaeStrcasecmp(ptr, "none") == 0))
		{
			fprintf(stderr, "siginstall(): empty value for XBAE_DEBUG_SIGNAL\n");
		}
#  if defined(SIGBREAK)
		else if(strcmp(ptr, "SIGBREAK") == 0)
			signo = SIGBREAK;
#  endif
#  if defined(SIGUNUSED)
		else if(strcmp(ptr, "SIGUNUSED") == 0)
			signo = SIGUNUSED;
#  endif
#  if defined(SIGUSR1)
		else if(strcmp(ptr, "SIGUSR1") == 0)
			signo = SIGUSR1;
#  endif
#  if defined(SIGUSR2)
		else if(strcmp(ptr, "SIGUSR2") == 0)
			signo = SIGUSR2;
#  endif
#  if defined(SIGUSR3)
		else if(strcmp(ptr, "SIGUSR3") == 0)
			signo = SIGUSR3;
#  endif
		else
			fprintf(stderr, "siginstall(): unknown signal in XBAE_DEBUG_SIGNAL: %s\n", ptr);
	}

	if(signo == NOSIG)
	{
		return False;
	}
	else
	{
		sighandler_t sigrc;

#  ifdef DEBUGSIG
		fprintf(stderr, "siginstall(): installing %p on signal %i\n", sighandler, signo);
#  endif
		sigrc = signal(signo, sighandler);
		if(sigrc == SIG_ERR)
			return False;
		else
			return True;
	}
}
#endif /* XBAE_PRODUCTION */

/**************************************************************************************************/

#ifndef XBAE_PRODUCTION
static void _XbaeDebugOpenFile(void)
{
	const char *s;
	char *fn;

	if(_XbaeDebugFile)
	{
		/* already done */
		return;
	}

	/* The rest here is the initialization code.
	   Might be slow and long, since it's done only once */

	s = getenv("XBAE_DEBUG_FILE");
	if((s == NULL) || (*s == '\0') || ((strcmp(s, "stderr") == 0)))
		_XbaeDebugFile = stderr;	/* default/fallback value */
	else if(strcmp(s, "stdout") == 0)
		/* the user wants to mix our output with the stdout of the
		 * user application.
		 */
		_XbaeDebugFile = stdout;

	if(_XbaeDebugFile)
	{
		/* disable buffering for stdout/stderr */
		setbuf(_XbaeDebugFile, NULL);
		return;
	}
#  ifdef HAVE_GETPID
	if(strstr(s, "%p"))
	{
		char *formatstr, *p;

		fn = XtMalloc(strlen(s) + 10);
		formatstr = XtMalloc(strlen(s) + 1);
		strcpy(formatstr, s);
		p = strstr(formatstr, "%p");
		*(p + 1) = 'd';
		sprintf(fn, s, getpid());
		XtFree(formatstr);
	}
	else
#  endif
	   /* HAVE_GETPID */
	{
		fn = XtMalloc(strlen(s) + 1);
		strcpy(fn, s);
	}

	/* "a" means append, create if doesn't exist */
	_XbaeDebugFile = fopen(fn, "a");
	if(_XbaeDebugFile == NULL)
	{
		/* if fopen() fails do something reasonable */
		fprintf(stderr, "_XbaeDebugOpenFile(): Can't open file %s\n", fn);
		_XbaeDebugFile = stderr;
		/* disable buffering */
		setbuf(_XbaeDebugFile, NULL);
	}
	else
	{
		/* disable buffering for file */
		setbuf(_XbaeDebugFile, NULL);
	}
	XtFree(fn);
}
#endif /* XBAE_PRODUCTION */

/*
 * See if 'fn' refers to a source file which is allowed to produce
 * debugging output.
 * The ".c" suffix for sources is not required (i.e. Form is equivalent
 * to Form.c).
 */
#ifndef XBAE_PRODUCTION
static Boolean ValidateSource(const char *fn)
{
	static Boolean init = False;

	typedef struct {
		char *fn;
		Boolean shortmatch;
		size_t len;
	} list_entry_t;

	static list_entry_t *poslist = NULL;
	static list_entry_t *neglist = NULL;
	static int positems = 0;
	static int negitems = 0;
	static Boolean flag_all = False;
	static Boolean flag_none = False;

	if(!init)
	{
		const char *sourcelist;

		/* Do initialization once and for all.
		   Might be a long, slow procedure, but should speed all
		   upcoming calls to this routine! */
		sourcelist = getenv("XBAE_DEBUG_SOURCES");
		if(sourcelist == NULL)
		{
			/* for compatibility with earlier versions */
			sourcelist = getenv("XBAE_DEBUGSOURCES");
		}

		/* set some flags to indicate situations where no explicit
		   search is later on required */

		/* list does not exist, is empty or set to "none" */
		if(sourcelist == NULL || sourcelist[0] == '\0' || (_xbaeStrcasecmp(sourcelist, "none") == 0))
		{
			flag_none = True;
		}
		else if(_xbaeStrcasecmp(sourcelist, "all") == 0)
		{
			flag_all = True;
		}
		else
		{
			const char *s, *p;

			s = sourcelist;
			while(s && *s)
			{
				char *dotptr, *asteriskptr;
				list_entry_t *newitem;
				Cardinal len;

				p = strchr(s, ':');
				if(p)
					len = (p - s);
				else
					len = strlen(s);

				if(*s == '-')
				{
					/* cut the '-' out: */
					len--;
					s++;

					neglist =
						(list_entry_t *) XtRealloc((char *) neglist, sizeof(list_entry_t) * (negitems + 1));
					newitem = neglist + negitems;
					negitems++;
				}
				else
				{
					poslist =
						(list_entry_t *) XtRealloc((char *) poslist, sizeof(list_entry_t) * (positems + 1));
					newitem = poslist + positems;
					positems++;
				}

				newitem->fn = XtMalloc(len + 1);
				strncpy(newitem->fn, s, len);
				newitem->fn[len] = '\0';

				/* Cut the file extensions */
				if((dotptr = strrchr(newitem->fn, '.')))
					*dotptr = '\0';

				/* Check for shortmatch, asterisk */
				if((asteriskptr = strchr(newitem->fn, '*')))
				{
					*asteriskptr = '\0';
					newitem->shortmatch = True;
					newitem->len = strlen(newitem->fn);
				}
				else
				{
					newitem->shortmatch = False;
					newitem->len = 0;
				}

				/* proceed to next entry */
				if(p)
					s = p + 1;
				else
					s = p;
			}					/* while() */
		}

		init = True;

#  ifdef DEBUGVALIDATE
		{
			int i;
			fprintf(stderr, "VS() init\n");
			for(i = 0; i < positems; i++)
			{
				fprintf(stderr, "positem[%i]=%s # Short=%i\n", i, poslist[i].fn, poslist[i].shortmatch);
			}
			for(i = 0; i < negitems; i++)
			{
				fprintf(stderr, "negitem[%i]=%s # Short=%i\n", i, neglist[i].fn, neglist[i].fn.shortmatch);
			}
		}
#  endif
	}

	/* the most simple cases: */
	if(flag_none)
	{
		return False;
	}
	else if(flag_all)
	{
		return True;
	}
	else
	{
		/* OK, we need to check explicitly ... */
		const char *lastslash;
		char *lastdot;
		char shortfn[256];		/* dynamic memory would be too 'expensive' */

		/* First we have to prepare the file name as passed to this routine:
		   the __FILE__ macro as inserted from CPP may contain an optional path
		   and certainly features a file extension (".c").
		   Our XBAE_DEBUG_SOURCES shouldn't have this, so we have to strip that. */

		lastslash = strrchr(fn, '/');
		if(lastslash && *(lastslash + 1) != '\0')
			strncpy(shortfn, lastslash + 1, sizeof(shortfn) - 1);
		else
			strncpy(shortfn, fn, sizeof(shortfn) - 1);
		lastdot = strrchr(shortfn, '.');
		if(lastdot)
			*lastdot = '\0';

		if(negitems > 0)
		{
			/* we have a negative list -> return true unless the file is on the list */
			int i;
			for(i = 0; i < negitems; i++)
			{
				if((neglist[i].shortmatch && strncmp(shortfn, neglist[i].fn, neglist[i].len) == 0)
				   || strcmp(shortfn, neglist[i].fn) == 0)
				{
					return False;
				}
			}
			return True;
		}
		else
		{
			/* we have a positive list -> return false unless the file is on the list */
			int i;
			for(i = 0; i < positems; i++)
			{
				if((poslist[i].shortmatch && strncmp(shortfn, poslist[i].fn, poslist[i].len) == 0)
				   || strcmp(shortfn, poslist[i].fn) == 0)
				{
					return True;
				}
			}
			return False;		/* no matching entry found */
		}
	}
}
#endif /* XBAE_PRODUCTION */

/**************************************************************************************************/

/* some initialization.
   Does never fail; return value indicates whether debugging 
   is en- or disabled currently */
extern Boolean _XbaeDebugInit(void)
{
#ifdef XBAE_PRODUCTION
	return False;
#else
	static Boolean init = False;

	if(!init)
	{
		const char *ptr;

		ptr = getenv("XBAE_DEBUG_TOGGLE");
		if(ptr && (strcmp(ptr, "off") == 0))
			_XbaeDebugFlag = False;

		ptr = getenv("XBAE_DEBUG_PRINT_WIDGETID");
		if(ptr)
			_XbaeDebugPrintWidgetID = True;

		_XbaeDebugOpenFile();
		siginstall();
		init = True;
	}
	return _XbaeDebugFlag;
#endif
}

/**************************************************************************************************/

/* amai: why does it take a widget here as an argument; any good
         reason for it?? */

#ifdef _XbaeDebugInDebug
#  undef _XbaeDebugInDebug
#endif
extern Boolean _XbaeDebugInDebug(const char *fn, Widget w)
{
#ifdef XBAE_PRODUCTION
	return False;
#else
	return ValidateSource(fn);
#endif
}


/* Allow user to query the state of Debugging System */
extern Boolean _XbaeDebugQueryState(void)
{
#ifndef XBAE_PRODUCTION
	return _XbaeDebugFlag;
#else
	return False;
#endif
}


/* extern interface to turn on/off debugging output */
extern void _XbaeDebugSet(Boolean flag)
{
#ifndef XBAE_PRODUCTION
	if(flag)
		_XbaeDebugFlag = True;
	else
		_XbaeDebugFlag = False;
#endif
}


/* In rare circumstances when one can't afford to code in
   arguments to a function call for _LtSetDebug() here's
   an alternative call */
extern void _XbaeDebugToggle(void)
{
#ifndef XBAE_PRODUCTION
	_XbaeDebugFlag = !_XbaeDebugFlag;
#endif
}

/**************************************************************************************************/

#ifndef XBAE_PRODUCTION
extern void _XbaeDebug(const char *fn, Widget w, const char *fmt, ...)
{
	va_list ap;

#  ifdef DEBUGVALIDATE
	fprintf(stderr, "ValidateSource(%s)=%s\n", fn, _XbaeDebugBoolean2String(ValidateSource(fn)));
#  endif
	if(_XbaeDebugInit() && ValidateSource(fn))
	{
		if(w)
		{
			if(_XbaeDebugPrintWidgetID)
			{
				fprintf(_XbaeDebugFile, "%s %s [%p]: ",
						w->core.widget_class->core_class.class_name, XtName(w), w);
			}
			else
			{
				fprintf(_XbaeDebugFile, "%s %s: ", w->core.widget_class->core_class.class_name, XtName(w));
			}
		}
		else
		{
			fprintf(_XbaeDebugFile, "(null widget): ");
		}

		va_start(ap, fmt);
		vfprintf(_XbaeDebugFile, fmt, ap);
		va_end(ap);

		fflush(_XbaeDebugFile);
	}
}
#endif /* !XBAE_PRODUCTION */


#ifndef XBAE_PRODUCTION
extern void _XbaeDebug2(const char *fn, Widget w, Widget c, const char *fmt, ...)
{
	va_list ap;

	if(_XbaeDebugInit() && ValidateSource(fn))
	{
		if(w && c)
		{
			if(_XbaeDebugPrintWidgetID)
			{
				fprintf(_XbaeDebugFile, "%s %s [%p] (child %s [%p]): ",
						w->core.widget_class->core_class.class_name, XtName(w), w, XtName(c), c);
			}
			else
			{
				fprintf(_XbaeDebugFile, "%s %s (child %s): ",
						w->core.widget_class->core_class.class_name, XtName(w), XtName(c));
			}
		}
		else if(w)
		{
			if(_XbaeDebugPrintWidgetID)
			{
				fprintf(_XbaeDebugFile, "%s %s [%p] (child NULL): ",
						w->core.widget_class->core_class.class_name, XtName(w), w);
			}
			else
			{
				fprintf(_XbaeDebugFile, "%s %s (child NULL): ",
						w->core.widget_class->core_class.class_name, XtName(w));
			}
		}
		else
		{
			fprintf(_XbaeDebugFile, "(null widget): ");
		}

		va_start(ap, fmt);
		vfprintf(_XbaeDebugFile, fmt, ap);
		va_end(ap);
	}
}
#endif /* !XBAE_PRODUCTION */


#ifndef XBAE_PRODUCTION
extern void _XbaeDebug0(const char *fn, Widget w, const char *fmt, ...)
{
	va_list ap;

	if(_XbaeDebugInit() && ValidateSource(fn))
	{
		va_start(ap, fmt);
		vfprintf(_XbaeDebugFile, fmt, ap);
		va_end(ap);
	}
}
#endif /* !XBAE_PRODUCTION */


#ifndef XBAE_PRODUCTION
extern void _XbaeDebugPrintArgList(const char *fn, Widget w, ArgList al, int n, Boolean Get)
{
	int i;
	unsigned num;

	if(_XbaeDebugInit() && ValidateSource(fn))
	{

		for(i = 0; i < n; i++)
		{
			int at;

			for(at = 0; _XbaeDebugTypes[at].name; at++)
			{
				if(strcmp(al[i].name, _XbaeDebugTypes[at].name) == 0)
				{
					break;
				}
			}

			if(_XbaeDebugTypes[at].name == NULL)
			{
				fprintf(_XbaeDebugFile, "Arg[%d] : %s (not handled FIX ME)\n", i, al[i].name);
				continue;
			}

			switch (_XbaeDebugTypes[at].t)
			{
				case _XbaeDebugNONE:
					fprintf(_XbaeDebugFile, "Arg[%d] : %s\n", i, al[i].name);
					break;

				case _XbaeDebugINT:
					fprintf(_XbaeDebugFile, "Arg[%d] : %s %d\n", i, al[i].name,
							(Get) ? *(int *) al[i].value : (int) al[i].value);
					break;

				case _XbaeDebugSHORT:
					fprintf(_XbaeDebugFile, "Arg[%d] : %s %d\n", i, al[i].name,
							(Get) ? *(short *) al[i].value : (short) al[i].value);
					break;

				case _XbaeDebugSTRING:
					fprintf(_XbaeDebugFile, "Arg[%d] : %s %s\n", i, al[i].name, (char *) al[i].value);
					break;

				case _XbaeDebugXMSTRING:
					fprintf(_XbaeDebugFile, "Arg[%d] : %s %s\n", i, al[i].name,
							_XbaeDebugXmString2String((XmString) al[i].value));
					break;

				case _XbaeDebugCHAR:
					fprintf(_XbaeDebugFile, "Arg[%d] : %s %c\n", i, al[i].name, (char) al[i].value);
					break;

				case _XbaeDebugALIGNMENT:
					fprintf(_XbaeDebugFile, "Arg[%d] : %s %s\n", i, al[i].name,
							(Get) ? _XbaeDebugAlignment2String(*(unsigned char *) al[i].value)
							: _XbaeDebugAlignment2String((unsigned char) al[i].value));
					break;

				case _XbaeDebugRESIZE_POLICY:
					fprintf(_XbaeDebugFile, "Arg[%d] : %s %s\n", i, al[i].name,
							(Get) ? _XbaeDebugResizePolicy2String(*(unsigned char *) al[i].value)
							: _XbaeDebugResizePolicy2String((unsigned char) al[i].value));
					break;

				case _XbaeDebugSB_DISPLAY_POLICY:
					fprintf(_XbaeDebugFile, "Arg[%d] : %s %s\n", i, al[i].name,
							(Get) ? _XbaeDebugSBDisplayPolicy2String(*(unsigned char *) al[i].value)
							: _XbaeDebugSBDisplayPolicy2String((unsigned char) al[i].value));
					break;

				case _XbaeDebugDRAG_TYPE:
					fprintf(_XbaeDebugFile, "Arg[%d] : %s %s\n", i, al[i].name,
							(Get) ? _XbaeDebugDragType2String(*(unsigned char *) al[i].value)
							: _XbaeDebugDragType2String((unsigned char) al[i].value));
					break;

				case _XbaeDebugSCROLLING_POLICY:
					fprintf(_XbaeDebugFile, "Arg[%d] : %s %s\n", i, al[i].name,
							(Get) ? _XbaeDebugScrollingPolicy2String(*(unsigned char *) al[i].value)
							: _XbaeDebugScrollingPolicy2String((unsigned char) al[i].value));
					break;

				case _XbaeDebugSB_PLACEMENT:
					fprintf(_XbaeDebugFile, "Arg[%d] : %s %s\n", i, al[i].name,
							(Get) ? _XbaeDebugSBPlacement2String(*(unsigned char *) al[i].value)
							: _XbaeDebugSBPlacement2String((unsigned char) al[i].value));
					break;

				case _XbaeDebugLIST_SIZE_POLICY:
					fprintf(_XbaeDebugFile, "Arg[%d] : %s %s\n", i, al[i].name,
							(Get) ? _XbaeDebugListSizePolicy2String(*(unsigned char *) al[i].value)
							: _XbaeDebugListSizePolicy2String((unsigned char) al[i].value));
					break;

				case _XbaeDebugATTACHMENT:
					fprintf(_XbaeDebugFile, "Arg[%d] : %s %s\n", i, al[i].name,
							(Get) ? _XbaeDebugAttachment2String(*(unsigned char *) al[i].value)
							: _XbaeDebugAttachment2String((unsigned char) al[i].value));
					break;

				case _XbaeDebugDIALOG_STYLE:
					fprintf(_XbaeDebugFile, "Arg[%d] : %s %s\n", i, al[i].name,
							(Get) ? _XbaeDebugDialogStyle2String(*(unsigned char *) al[i].value)
							: _XbaeDebugDialogStyle2String((unsigned char) al[i].value));
					break;

				case _XbaeDebugWIDGET:
					fprintf(_XbaeDebugFile, "Arg[%d] : %s %s\n", i, al[i].name, (Get)
							? ((al[i].value && *(Widget *) al[i].value)
							   ? XtName(*(Widget *) al[i].value)
							   : "(null)") : (al[i].value ? XtName((Widget) al[i].value) : "(null)"));
					break;

				case _XbaeDebugEDIT_MODE:
					fprintf(_XbaeDebugFile, "Arg[%d] : %s %s\n", i, al[i].name,
							(Get) ? _XbaeDebugEditMode2String(*(Boolean *) al[i].value)
							: _XbaeDebugEditMode2String((Boolean) al[i].value));
					break;

				case _XbaeDebugBOOLEAN:
					fprintf(_XbaeDebugFile, "Arg[%d] : %s %s\n", i, al[i].name,
							(Get) ? _XbaeDebugBoolean2String(*(Boolean *) al[i].value)
							: _XbaeDebugBoolean2String((Boolean) al[i].value));
					break;

				case _XbaeDebugSELECTION_POLICY:
					fprintf(_XbaeDebugFile, "Arg[%d] : %s %s\n", i, al[i].name,
							_XbaeDebugSelectionPolicy2String(al[i].value));
					break;

				case _XbaeDebugXMSTRING_LIST:	/* Need related info !! */
					num = 0xdeadbeef;

					XtVaGetValues(w, _XbaeDebugTypes[at].related, &num, NULL);

					if(num != 0xdeadbeef)
					{
						int j;

						fprintf(_XbaeDebugFile, "Arg[%d] : %s(%d):\n", i, al[i].name, num);
						for(j = 0; j < (int) num; j++)
						{
							fprintf(_XbaeDebugFile, "\tItem %d '%s'\n", j,
									_XbaeDebugXmString2String(((XmString *) (al[i].value))[j]));
						}
					}

					break;

				case _XbaeDebugMWM_INPUT_MODE:
					fprintf(_XbaeDebugFile, "Arg[%d] : %s %s\n", i, al[i].name,
							(Get) ? _XbaeDebugMwmInput2String(*(int *) al[i].value)
							: _XbaeDebugMwmInput2String((int) al[i].value));
					break;

				case _XbaeDebugDELETE_RESPONSE:
					fprintf(_XbaeDebugFile, "Arg[%d] : %s %s\n", i, al[i].name,
							(Get) ? _XbaeDebugDeleteResponse2String(*(int *) al[i].value) :
							_XbaeDebugDeleteResponse2String((int) al[i].value));
					break;

				case _XbaeDebugCOMBOBOX_TYPE:
					fprintf(_XbaeDebugFile, "Arg[%d] : %s %s\n", i, al[i].name,
							_XbaeDebugComboBoxType2String(al[i].value));
					break;

			}
		}
	}
}
#endif /* !XBAE_PRODUCTION */


#ifndef XBAE_PRODUCTION
extern void
_XbaeDebugAction(const char *fn, Widget w, const String action,
				 const String * params, const Cardinal * num_params)
{
	if(_XbaeDebugInit() && ValidateSource(fn))
	{
		int i;

		if(w)
		{
			if(_XbaeDebugPrintWidgetID)
			{
				fprintf(_XbaeDebugFile, "%s %s [%p]: ",
						w->core.widget_class->core_class.class_name, XtName(w), w);
			}
			else
			{
				fprintf(_XbaeDebugFile, "%s %s: ", w->core.widget_class->core_class.class_name, XtName(w));
			}
		}
		else
		{
			fprintf(_XbaeDebugFile, "(null widget): ");
		}

		fprintf(_XbaeDebugFile, "Action %s(", action);
		if(*num_params)
		{
			fprintf(_XbaeDebugFile, "%s", params[0]);
		}
		for(i = 1; i < (int) *num_params; i++)
		{
			fprintf(_XbaeDebugFile, ", %s", params[i]);
		}
		fprintf(_XbaeDebugFile, ")\n");

		fflush(_XbaeDebugFile);
	}
}
#endif /* !XBAE_PRODUCTION */

/**************************************************************************************************/

#ifndef XBAE_PRODUCTION
static void __XbaeDebugPrintTree(Widget w, int level)
{
	int i;
	Cardinal c;
	CompositeWidget cw = (CompositeWidget) w;

	if(w == NULL)
	{
		return;
	}

	for(i = 0; i < level; i++)
	{
		fprintf(_XbaeDebugFile, "\t");
	}

	fprintf(_XbaeDebugFile, "%s : %p/%ld", XtName(w), w, XtWindow(w));
	fprintf(_XbaeDebugFile, "(%s) geo %d %d %d %d",
			w->core.widget_class->core_class.class_name, XtX(w), XtY(w), XtWidth(w), XtHeight(w));
#  ifdef PRINT_STATE
	fprintf(_XbaeDebugFile, " state: %s %s", _XbaeDebugState(w), w->core.mapped_when_managed ? "mwm" : "");
#  endif
	fprintf(_XbaeDebugFile, "\n");
	if(XtIsSubclass(w, compositeWidgetClass))
	{
		for(c = 0; c < cw->composite.num_children; c++)
		{
			__XbaeDebugPrintTree(cw->composite.children[c], level + 1);
		}
	}

	for(c = 0; c < cw->core.num_popups; c++)
	{
		__XbaeDebugPrintTree(cw->core.popup_list[c], level + 1);
	}
}
#endif


extern void _XbaeDebugPrintTree(Widget w)
{
#ifndef XBAE_PRODUCTION
	if(_XbaeDebugInit())
	{
		__XbaeDebugPrintTree(w, 0);
	}
#endif
}


extern void _XbaeDebugPrintCompleteTree(Widget w)
{
#ifndef XBAE_PRODUCTION
	if(_XbaeDebugInit())
	{
		Widget p;
		for(p = XtParent(w); p != NULL; w = p, p = XtParent(w))
			;

		__XbaeDebugPrintTree(w, 0);
	}
#endif
}

/**************************************************************************************************/

/*
 * Allow unconditional printing to the _XbaeDebugFile
 */
extern void _XbaeDebugPrintString(const char *s)
{
#ifndef XBAE_PRODUCTION
	_XbaeDebugInit();
	fprintf(_XbaeDebugFile, "%s", s);
#endif
}

/**************************************************************************************************/

/* The following calls shouldn't depend on the complete debugging 
   subsystem, i.e. things like _XbaeDebugFile, etc. */

extern const char *_XbaeDebugState(Widget w)
{
	if(XtIsRealized(w))
	{
		if(XtIsManaged(w))
		{
			return "realized, managed";
		}
		else
		{
			return "realized, not managed";
		}
	}
	else
	{
		if(XtIsManaged(w))
		{
			return "not realized, managed";
		}
		else
		{
			return "not realized, not managed";
		}
	}
}

extern const char *_XbaeDebugDeleteResponse2String(int d)
{
	switch (d)
	{
		case XmDESTROY:
			return "XmDESTROY";
		case XmUNMAP:
			return "XmUNMAP";
		case XmDO_NOTHING:
			return "XmDO_NOTHING";
		default:
			return "??";
	}
}

extern const char *_XbaeDebugComboBoxType2String(unsigned char type)
{
	switch (type)
	{
		case XmDROP_DOWN_LIST:
			return ("XmDROP_DOWN_LIST");
		case XmDROP_DOWN_COMBO_BOX:
			return ("XmDROP_DOWN_COMBO_BOX");
		case XmCOMBO_BOX:
			return ("XmCOMBO_BOX");
		default:
			return ("UNKNOWN");
	}
}

extern const char *_XbaeDebugGeoAction2String(int action)
{
	switch (action)
	{
		case XmGET_ACTUAL_SIZE:
			return ("XmGET_ACTUAL_SIZE");
		case XmGET_PREFERRED_SIZE:
			return ("XmGET_PREFERRED_SIZE");
		case XmGEO_PRE_SET:
			return ("XmGEO_PRE_SET");
		case XmGEO_POST_SET:
			return ("XmGEO_POST_SET");
		default:
			return ("Unknown geo action");
	}
}

extern const char *_XbaeDebugGeometryResult2String(XtGeometryResult r)
{
	switch (r)
	{
		case XtGeometryYes:
			return "Yes";

		case XtGeometryNo:
			return "No";

		case XtGeometryAlmost:
			return "Almost";

		case XtGeometryDone:
			return "Done";

		default:
			return "(invalid geometry result)";
	}
}

extern const char *_XbaeDebugDragAndDropMessageType2String(unsigned char r)
{
	switch (r)
	{
		case XmTOP_LEVEL_ENTER:
			return "TOP_LEVEL_ENTER";

		case XmTOP_LEVEL_LEAVE:
			return "TOP_LEVEL_LEAVE";

		case XmDRAG_MOTION:
			return "DRAG_MOTION";

		case XmDROP_SITE_ENTER:
			return "DROP_SITE_ENTER";

		case XmDROP_SITE_LEAVE:
			return "DROP_SITE_LEAVE";

		case XmDROP_START:
			return "DROP_START";

		case XmDROP_FINISH:
			return "DROP_FINISH";

		case XmDRAG_DROP_FINISH:
			return "DRAG_DROP_FINISH";

		case XmOPERATION_CHANGED:
			return "OPERATION_CHANGED";

		default:
			return "UNKNOWN";
	}
}

extern const char *_XbaeDebugDragType2String(unsigned char r)
{
	switch (r)
	{
		case XmDRAG_NONE:
			return "XmDRAG_NONE";

		case XmDRAG_DROP_ONLY:
			return "XmDRAG_DROP_ONLY";

		case XmDRAG_PREFER_PREREGISTER:
			return "XmDRAG_PREFER_PREREGISTER";

		case XmDRAG_PREREGISTER:
			return "XmDRAG_PREREGISTER";

		case XmDRAG_PREFER_DYNAMIC:
			return "XmDRAG_PREFER_DYNAMIC";

		case XmDRAG_PREFER_RECEIVER:
			return "XmDRAG_PREFER_RECEIVER";

		case XmDRAG_DYNAMIC:
			return "XmDRAG_DYNAMIC";

		default:
			return "UNKNOWN";
	}
}

extern const char *_XbaeDebugScrollingPolicy2String(unsigned char r)
{
	switch (r)
	{
		case XmAUTOMATIC:
			return "XmAUTOMATIC";

		case XmCONSTANT:
			return "XmCONSTANT";

		default:
			return "UNKNOWN";
	}
}

extern const char *_XbaeDebugMwmInput2String(int a)
{
	switch (a)
	{
		case MWM_INPUT_MODELESS:
			return "MWM_INPUT_MODELESS";

		case MWM_INPUT_PRIMARY_APPLICATION_MODAL:
			return "MWM_INPUT_PRIMARY_APPLICATION_MODAL or MWM_INPUT_APPLICATION_MODAL";

		case MWM_INPUT_FULL_APPLICATION_MODAL:
			return "MWM_INPUT_FULL_APPLICATION_MODAL";

		case MWM_INPUT_SYSTEM_MODAL:
			return "MWM_INPUT_SYSTEM_MODAL";
#if 0
		case MWM_INPUT_APPLICATION_MODAL:
			return "MWM_INPUT_APPLICATION_MODAL";
#endif
		default:
			return "(invalid input style)";
	}
}

extern const char *_XbaeDebugDialogStyle2String(int a)
{
	switch (a)
	{
		case XmDIALOG_WORK_AREA:
			return "XmDIALOG_WORK_AREA or XmDIALOG_MODELESS";

			/*
			   case XmDIALOG_MODELESS:
			   return "XmDIALOG_MODELESS";
			 */

		case XmDIALOG_PRIMARY_APPLICATION_MODAL:
			return "XmDIALOG_PRIMARY_APPLICATION_MODAL or XmDIALOG_APPLICATION_MODAL";

		case XmDIALOG_FULL_APPLICATION_MODAL:
			return "XmDIALOG_FULL_APPLICATION_MODAL";

		case XmDIALOG_SYSTEM_MODAL:
			return "XmDIALOG_SYSTEM_MODAL";

			/*
			   case XmDIALOG_APPLICATION_MODAL:
			   return "XmDIALOG_APPLICATION_MODAL";
			 */

		default:
			return "(invalid dialog style)";
	}
}

extern const char *_XbaeDebugAttachment2String(int a)
{
	switch (a)
	{
		case XmATTACH_FORM:
			return "XmATTACH_FORM";

		case XmATTACH_OPPOSITE_FORM:
			return "XmATTACH_OPPOSITE_FORM";

		case XmATTACH_WIDGET:
			return "XmATTACH_WIDGET";

		case XmATTACH_OPPOSITE_WIDGET:
			return "XmATTACH_OPPOSITE_WIDGET";

		case XmATTACH_NONE:
			return "XmATTACH_NONE";

		case XmATTACH_POSITION:
			return "XmATTACH_POSITION";

		case XmATTACH_SELF:
			return "XmATTACH_SELF";

		default:
			return "(invalid attachment)";
	}
}

extern const char *_XbaeDebugMenuEnum2String(int f)
{
	switch (f)
	{
		case XmMENU_POPDOWN:
			return "XmMENU_POPDOWN";

		case XmMENU_PROCESS_TREE:
			return "XmMENU_PROCESS_TREE";

		case XmMENU_TRAVERSAL:
			return "XmMENU_TRAVERSAL";

		case XmMENU_SHELL_POPDOWN:
			return "XmMENU_SHELL_POPDOWN";

		case XmMENU_CALLBACK:
			return "XmMENU_CALLBACK";

		case XmMENU_BUTTON:
			return "XmMENU_BUTTON";

		case XmMENU_CASCADING:
			return "XmMENU_CASCADING";

		case XmMENU_SUBMENU:
			return "XmMENU_SUBMENU";

		case XmMENU_ARM:
			return "XmMENU_ARM";

		case XmMENU_DISARM:
			return "XmMENU_DISARM";

		case XmMENU_BAR_CLEANUP:
			return "XmMENU_BAR_CLEANUP";

		case XmMENU_STATUS:
			return "XmMENU_STATUS";

		case XmMENU_MEMWIDGET_UPDATE:
			return "XmMENU_MEMWIDGET_UPDATE";

		case XmMENU_BUTTON_POPDOWN:
			return "XmMENU_BUTTON_POPDOWN";

		case XmMENU_RESTORE_EXCLUDED_TEAROFF_TO_TOPLEVEL_SHELL:
			return "XmMENU_RESTORE_EXCLUDED_TEAROFF_TO_TOPLEVEL_SHELL";

		case XmMENU_RESTORE_TEAROFF_TO_TOPLEVEL_SHELL:
			return "XmMENU_RESTORE_TEAROFF_TO_TOPLEVEL_SHELL";

		case XmMENU_RESTORE_TEAROFF_TO_MENUSHELL:
			return "XmMENU_RESTORE_TEAROFF_TO_MENUSHELL";

		case XmMENU_GET_LAST_SELECT_TOPLEVEL:
			return "XmMENU_GET_LAST_SELECT_TOPLEVEL";

		case XmMENU_TEAR_OFF_ARM:
			return "XmMENU_TEAR_OFF_ARM";

		default:
			return "??";
	}
}

extern const char *_XbaeDebugBoolean2String(Boolean b)
{
	if(b)
		return "True";
	else
		return "False";
}

extern const char *_XbaeDebugXmString2String(XmString xms)
{
	static char *s = NULL;

	if(s)
	{
		XtFree(s);
		s = NULL;
	}
	if(xms == (XmString) XmUNSPECIFIED)
	{
		return "XmUNSPECIFIED";
	}
	if(!XmStringGetLtoR(xms, XmFONTLIST_DEFAULT_TAG, &s) || s == NULL)
	{
		return "(null)";
	}

	return s;
}

extern const char *_XbaeDebugPacking2String(unsigned char p)
{
	static char res[40];

	switch (p)
	{
		case XmPACK_COLUMN:
			return "XmPACK_COLUMN";

		case XmPACK_TIGHT:
			return "XmPACK_TIGHT";

		case XmPACK_NONE:
			return "XmPACK_NONE";

		default:
			sprintf(res, "Invalid packing %d", p);
			return res;
	}
}

extern const char *_XbaeDebugRcType2String(unsigned char t)
{
	static char res[40];

	switch (t)
	{
		case XmWORK_AREA:
			return "XmWORK_AREA";

		case XmMENU_BAR:
			return "XmMENU_BAR";

		case XmMENU_PULLDOWN:
			return "XmMENU_PULLDOWN";

		case XmMENU_POPUP:
			return "XmMENU_POPUP";

		case XmMENU_OPTION:
			return "XmMENU_OPTION";

		default:
			sprintf(res, "Invalid RC Type %d", t);
			return res;
	}
}

extern const char *_XbaeDebugWidgetGeometry2String(XtWidgetGeometry * g)
{
	static char o1[128], o2[128], b[20];
	static char *out = NULL;
	int i;

	if(g == NULL)
	{
		return "NULL_GEOMETRY";
	}

	if(g->request_mode == 0)
	{
		return "GEOMETRY_NO_FIELDS";
	}

/* Some magic to ensure you can call this sucker twice in one C function call */
	if(out == o1)
	{
		out = o2;
	}
	else
	{
		out = o1;
	}

	out[0] = '\0';

	if(g->request_mode & CWX)
	{
		sprintf(b, "x %d ", g->x);
		strcat(out, b);
	}
	if(g->request_mode & CWY)
	{
		sprintf(b, "y %d ", g->y);
		strcat(out, b);
	}
	if(g->request_mode & CWWidth)
	{
		sprintf(b, "w %d ", g->width);
		strcat(out, b);
	}
	if(g->request_mode & CWHeight)
	{
		sprintf(b, "h %d ", g->height);
		strcat(out, b);
	}
	if(g->request_mode & CWBorderWidth)
	{
		sprintf(b, "bw %d ", g->border_width);
		strcat(out, b);
	}

	for(i = 0; out[i]; i++)
	{
	}

	if(i > 0 && out[i - 1] == ' ')
	{
		out[i - 1] = '\0';
	}

	return out;
}

extern const char *_XbaeDebugEditMode2String(int n)
{
	switch (n)
	{
		case XmMULTI_LINE_EDIT:
			return "XmMULTI_LINE_EDIT";

		case XmSINGLE_LINE_EDIT:
			return "XmSINGLE_LINE_EDIT";

		default:
			return "???";
	}
}

extern const char *_XbaeDebugSelectionPolicy2String(int n)
{
	switch (n)
	{
		case XmSINGLE_SELECT:
			return "XmSINGLE_SELECT";

		case XmBROWSE_SELECT:
			return "XmBROWSE_SELECT";

		case XmMULTIPLE_SELECT:
			return "XmMULTIPLE_SELECT";

		case XmEXTENDED_SELECT:
			return "XmEXTENDED_SELECT";

		default:
			return "???";
	}
}

extern const char *_XbaeDebugResizePolicy2String(int n)
{
	switch (n)
	{
		case XmRESIZE_NONE:
			return "XmRESIZE_NONE";

		case XmRESIZE_GROW:
			return "XmRESIZE_GROW";

		case XmRESIZE_ANY:
			return "XmRESIZE_ANY";

		case XmRESIZE_SWINDOW:
			return "XmRESIZE_SWINDOW";

		default:
			return "XmNscrollBarDisplayPolicy - illegal";
	}
}

extern const char *_XbaeDebugSBDisplayPolicy2String(int n)
{
	switch (n)
	{
		case XmSTATIC:
			return "XmSTATIC";

		case XmAS_NEEDED:
			return "XmAS_NEEDED";

		default:
			return "XmNscrollBarDisplayPolicy - illegal";
	}
}

extern const char *_XbaeDebugSBPlacement2String(int n)
{
	switch (n)
	{
		case XmTOP_LEFT:
			return "XmTOP_LEFT";

		case XmBOTTOM_LEFT:
			return "XmBOTTOM_LEFT";

		case XmTOP_RIGHT:
			return "XmTOP_RIGHT";

		case XmBOTTOM_RIGHT:
			return "XmBOTTOM_RIGHT";

		default:
			return "XmNscrollBarPlacement - illegal";
	}
}

extern const char *_XbaeDebugListSizePolicy2String(int n)
{
	switch (n)
	{
		case XmVARIABLE:
			return "XmVARIABLE";

		case XmCONSTANT:
			return "XmCONSTANT";

		case XmRESIZE_IF_POSSIBLE:
			return "XmRESIZE_IF_POSSIBLE";

		default:
			return "XmNlistSizePolicy - illegal";
	}
}

extern const char *_XbaeDebugAlignment2String(int n)
{
	switch (n)
	{
		case XmALIGNMENT_BEGINNING:
			return "XmALIGNMENT_BEGINNING";

		case XmALIGNMENT_CENTER:
			return "XmALIGNMENT_CENTER";

		case XmALIGNMENT_END:
			return "XmALIGNMENT_END";

		default:
			return "XmALIGNMENT - illegal";
	}
}

extern const char *_XbaeDebugMenuType2String(int n)
{
	switch (n)
	{
		case XmMENU_OPTION:
			return "XmMENU_OPTION";

		case XmMENU_POPUP:
			return "XmMENU_POPUP";

		case XmMENU_PULLDOWN:
			return "XmMENU_PULLDOWN";

		default:
			return "???";
	}
}

extern const char *_XbaeDebugNavigability2String(unsigned char n)
{
	switch (n)
	{
		case XmDESCENDANTS_NAVIGABLE:
			return "XmDESCENDANTS_NAVIGABLE";

		case XmDESCENDANTS_TAB_NAVIGABLE:
			return "XmDESCENDANTS_TAB_NAVIGABLE";

		case XmCONTROL_NAVIGABLE:
			return "XmCONTROL_NAVIGABLE";

		case XmNOT_NAVIGABLE:
			return "XmNOT_NAVIGABLE";

		case XmTAB_NAVIGABLE:
			return "XmTAB_NAVIGABLE";

		default:
			return "???";
	}
}

extern const char *_XbaeDebugHighlightMode2String(int mode)
{
	switch (mode)
	{
		case XmHIGHLIGHT_NORMAL:
			return "NORMAL";

		case XmHIGHLIGHT_SELECTED:
			return "SELECTED";

		case XmHIGHLIGHT_SECONDARY_SELECTED:
			return "SECONDARY_SELECTED";

		default:
			return "???";
	}
}

extern const char *_XbaeDebugReason2String(int reason)
{
	switch (reason)
	{
		case XmCR_NONE:
			return "XmCR_NONE";
		case XmCR_HELP:
			return "XmCR_HELP";
		case XmCR_VALUE_CHANGED:
			return "XmCR_VALUE_CHANGED";
		case XmCR_INCREMENT:
			return "XmCR_INCREMENT";
		case XmCR_DECREMENT:
			return "XmCR_DECREMENT";
		case XmCR_PAGE_INCREMENT:
			return "XmCR_PAGE_INCREMENT";
		case XmCR_PAGE_DECREMENT:
			return "XmCR_PAGE_DECREMENT";
		case XmCR_TO_TOP:
			return "XmCR_TO_TOP";
		case XmCR_TO_BOTTOM:
			return "XmCR_TO_BOTTOM";
		case XmCR_DRAG:
			return "XmCR_DRAG";
		case XmCR_ACTIVATE:
			return "XmCR_ACTIVATE";
		case XmCR_ARM:
			return "XmCR_ARM";
		case XmCR_DISARM:
			return "XmCR_DISARM";

/*	case XmCR_DUMMY13:                 return "XmCR_DUMMY13";	*/

/*	case XmCR_DUMMY14:                 return "XmCR_DUMMY14";	*/

/*	case XmCR_DUMMY15:                 return "XmCR_DUMMY15";	*/
		case XmCR_MAP:
			return "XmCR_MAP";
		case XmCR_UNMAP:
			return "XmCR_UNMAP";
		case XmCR_FOCUS:
			return "XmCR_FOCUS";
		case XmCR_LOSING_FOCUS:
			return "XmCR_LOSING_FOCUS";
		case XmCR_MODIFYING_TEXT_VALUE:
			return "XmCR_MODIFYING_TEXT_VALUE";
		case XmCR_MOVING_INSERT_CURSOR:
			return "XmCR_MOVING_INSERT_CURSOR";
		case XmCR_EXECUTE:
			return "XmCR_EXECUTE";
		case XmCR_SINGLE_SELECT:
			return "XmCR_SINGLE_SELECT";
		case XmCR_MULTIPLE_SELECT:
			return "XmCR_MULTIPLE_SELECT";
		case XmCR_EXTENDED_SELECT:
			return "XmCR_EXTENDED_SELECT";
		case XmCR_BROWSE_SELECT:
			return "XmCR_BROWSE_SELECT";
		case XmCR_DEFAULT_ACTION:
			return "XmCR_DEFAULT_ACTION";
		case XmCR_CLIPBOARD_DATA_REQUEST:
			return "XmCR_CLIPBOARD_DATA_REQUEST";
		case XmCR_CLIPBOARD_DATA_DELETE:
			return "XmCR_CLIPBOARD_DATA_DELETE";
		case XmCR_CASCADING:
			return "XmCR_CASCADING";
		case XmCR_OK:
			return "XmCR_OK";
		case XmCR_CANCEL:
			return "XmCR_CANCEL";

/*	case XmCR_DUMMY33:                 return "XmCR_DUMMY33";	*/
		case XmCR_APPLY:
			return "XmCR_APPLY";
		case XmCR_NO_MATCH:
			return "XmCR_NO_MATCH";
		case XmCR_COMMAND_ENTERED:
			return "XmCR_COMMAND_ENTERED";
		case XmCR_COMMAND_CHANGED:
			return "XmCR_COMMAND_CHANGED";
		case XmCR_EXPOSE:
			return "XmCR_EXPOSE";
		case XmCR_RESIZE:
			return "XmCR_RESIZE";
		case XmCR_INPUT:
			return "XmCR_INPUT";
		case XmCR_GAIN_PRIMARY:
			return "XmCR_GAIN_PRIMARY";
		case XmCR_LOSE_PRIMARY:
			return "XmCR_LOSE_PRIMARY";
		case XmCR_CREATE:
			return "XmCR_CREATE";
		case XmCR_TEAR_OFF_ACTIVATE:
			return "XmCR_TEAR_OFF_ACTIVATE";
		case XmCR_TEAR_OFF_DEACTIVATE:
			return "XmCR_TEAR_OFF_DEACTIVATE";
		case XmCR_OBSCURED_TRAVERSAL:
			return "XmCR_OBSCURED_TRAVERSAL";
		case XmCR_FOCUS_MOVED:
			return "XmCR_FOCUS_MOVED";

/*	case XmCR_DUMMY48:                 return "XmCR_DUMMY48";	*/

/*	case XmCR_DUMMY49:                 return "XmCR_DUMMY49";	*/

/*	case XmCR_DUMMY50:                 return "XmCR_DUMMY50";	*/

/*	case XmCR_DUMMY51:                 return "XmCR_DUMMY51";	*/

/*	case XmCR_DUMMY52:                 return "XmCR_DUMMY52";	*/

/*	case XmCR_DUMMY53:                 return "XmCR_DUMMY53";	*/
		case XmCR_REPOST:
			return "XmCR_REPOST";
		case XmCR_COLLAPSED:
			return "XmCR_COLLAPSED";
		case XmCR_EXPANDED:
			return "XmCR_EXPANDED";
		case XmCR_SELECT:
			return "XmCR_SELECT";
		case XmCR_DRAG_START:
			return "XmCR_DRAG_START";
		case XmCR_NO_FONT:
			return "XmCR_NO_FONT";
		case XmCR_NO_RENDITION:
			return "XmCR_NO_RENDITION";
		case XmCR_POST:
			return "XmCR_POST";
		case XmCR_SPIN_NEXT:
			return "XmCR_SPIN_NEXT";
		case XmCR_SPIN_PRIOR:
			return "XmCR_SPIN_PRIOR";
		case XmCR_SPIN_FIRST:
			return "XmCR_SPIN_FIRST";
		case XmCR_SPIN_LAST:
			return "XmCR_SPIN_LAST";
		case XmCR_PAGE_SCROLLER_INCREMENT:
			return "XmCR_PAGE_SCROLLER_INCREMENT";
		case XmCR_PAGE_SCROLLER_DECREMENT:
			return "XmCR_PAGE_SCROLLER_DECREMENT";
		case XmCR_MAJOR_TAB:
			return "XmCR_MAJOR_TAB";
		case XmCR_MINOR_TAB:
			return "XmCR_MINOR_TAB";
		case XmCR_PDM_NONE:
			return "XmCR_PDM_NONE";
		case XmCR_PDM_START_VXAUTH:
			return "XmCR_PDM_START_VXAUTH";
		case XmCR_PDM_START_PXAUTH:
			return "XmCR_PDM_START_PXAUTH";
		case XmCR_PDM_UP:
			return "XmCR_PDM_UP";
		case XmCR_PDM_OK:
			return "XmCR_PDM_OK";
		case XmCR_PDM_CANCEL:
			return "XmCR_PDM_CANCEL";
		case XmCR_PDM_START_ERROR:
			return "XmCR_PDM_START_ERROR";
		case XmCR_PDM_EXIT_ERROR:
			return "XmCR_PDM_EXIT_ERROR";
		case XmCR_PROTOCOLS:
			return "XmCR_PROTOCOLS";
		default:
			return "???";
	}
}

extern const char *_XbaeDebugFocusChange2String(XmFocusChange c)
{
	switch (c)
	{
		case XmFOCUS_IN:
			return "XmFOCUS_IN";
		case XmFOCUS_OUT:
			return "XmFOCUS_OUT";
		case XmENTER:
			return "XmENTER";
		case XmLEAVE:
			return "XmLEAVE";
		default:
			return "???";
	}
}

extern const char *_XbaeDebugNavigationType2String(XmNavigationType nt)
{
	switch (nt)
	{
		case XmNONE:
			return "XmNONE";
		case XmTAB_GROUP:
			return "XmTAB_GROUP";
		case XmSTICKY_TAB_GROUP:
			return "XmSTICKY_TAB_GROUP";
		case XmEXCLUSIVE_TAB_GROUP:
			return "XmEXCLUSIVE_TAB_GROUP";
		default:
			return "???";
	}
}

extern const char *_XbaeDebugEventType2String(int type)
{
	switch (type)
	{
		case KeyPress:
			return ("KeyPress");
		case KeyRelease:
			return ("KeyRelease");
		case ButtonPress:
			return ("ButtonPress");
		case ButtonRelease:
			return ("ButtonRelease");
		case KeymapNotify:
			return ("KeymapNotify");
		case MotionNotify:
			return ("MotionNotify");
		case EnterNotify:
			return ("EnterNotify");
		case LeaveNotify:
			return ("LeaveNotify");
		case FocusIn:
			return ("FocusIn");
		case FocusOut:
			return ("FocusOut");
		case Expose:
			return ("Expose");
		case GraphicsExpose:
			return ("GraphicsExpose");
		case NoExpose:
			return ("NoExpose");
		case ColormapNotify:
			return ("ColormapNotify");
		case PropertyNotify:
			return ("PropertyNotify");
		case VisibilityNotify:
			return ("VisibilityNotify");
		case ResizeRequest:
			return ("ResizeRequest");
		case CirculateNotify:
			return ("CirculateNotify");
		case ConfigureNotify:
			return ("ConfigureNotify");
		case DestroyNotify:
			return ("DestroyNotify");
		case GravityNotify:
			return ("GravityNotify");
		case MapNotify:
			return ("MapNotify");
		case ReparentNotify:
			return ("ReparentNotify");
		case UnmapNotify:
			return ("UnmapNotify");
		case CreateNotify:
			return ("CreateNotify");
		case CirculateRequest:
			return ("CirculateRequest");
		case ConfigureRequest:
			return ("ConfigureRequest");
		case MapRequest:
			return ("MapRequest");
		case MappingNotify:
			return ("MappingNotify");
		case ClientMessage:
			return ("ClientMessage");
		case SelectionClear:
			return ("SelectionClear");
		case SelectionNotify:
			return ("SelectionNotify");
		case SelectionRequest:
			return ("SelectionRequest");
		default:
			return ("UNKNOWN");
	}
}

extern const char *_XbaeDebugFocusMode2String(int type)
{
	switch (type)
	{
		case NotifyNormal:
			return ("NotifyNormal");
		case NotifyGrab:
			return ("NotifyGrab");
		case NotifyUngrab:
			return ("NotifyUngrab");
		case NotifyWhileGrabbed:
			return ("NotifyWhileGrabbed");
		default:
			return ("UNKNOWN");
	}
}

extern const char *_XbaeDebugFocusDetail2String(int type)
{
	switch (type)
	{
		case NotifyAncestor:
			return ("NotifyAncestor");
		case NotifyDetailNone:
			return ("NotifyDetailNone");
		case NotifyInferior:
			return ("NotifyInferior");
		case NotifyNonlinear:
			return ("NotifyNonlinear");
		case NotifyNonlinearVirtual:
			return ("NotifyNonlinearVirtual");
		case NotifyPointer:
			return ("NotifyPointer");
		case NotifyPointerRoot:
			return ("NotifyPointerRoot");
		case NotifyVirtual:
			return ("NotifyVirtual");
		default:
			return ("UNKNOWN");
	}
}

extern const char *_LtDebugFrameChildType2String(int action)
{
	switch (action)
	{
		case XmFRAME_GENERIC_CHILD:
			return ("XmFRAME_GENERIC_CHILD");
		case XmFRAME_WORKAREA_CHILD:
			return ("XmFRAME_WORKAREA_CHILD");
		case XmFRAME_TITLE_CHILD:
			return ("XmFRAME_TITLE_CHILD");
		default:
			return ("Unknown frame childtype");
	}
}

extern const char *_XbaeDebugShadowTypeToString(const char s)
{
	switch (s)
	{
		case XmSHADOW_IN:
			return "XmSHADOW_IN";
		case XmSHADOW_OUT:
			return "XmSHADOW_OUT";
		case BAD_SHADOW:
			return "BAD_SHADOW";
		case XmSINGLE_LINE:
			return "XmSINGLE_LINE";
		case XmDOUBLE_LINE:
			return "XmDOUBLE_LINE";
		case XmSINGLE_DASHED_LINE:
			return "XmSINGLE_DASHED_LINE";
		case XmDOUBLE_DASHED_LINE:
			return "XmDOUBLE_DASHED_LINE";
		case XmSHADOW_ETCHED_IN:
			return "XmSHADOW_ETCHED_IN";
		case XmSHADOW_ETCHED_OUT:
			return "XmSHADOW_ETCHED_OUT";
			/* Same as XmSHADOW_IN case XmSHADOW_ETCHED_IN_DASH:    return "XmSHADOW_ETCHED_IN_DASH"; */
			/* Same as XmSHADOW_OUT case XmSHADOW_ETCHED_OUT_DASH:  return "XmSHADOW_ETCHED_OUT_DASH"; */
		case XmINVALID_SEPARATOR_TYPE:
			return "XmINVALID_SEPARATOR_TYPE";
		default:
			return "??";
	}
}

/**************************************************************************************************/

#ifdef WITH_DMALLOC

static void XbaeAllocError(const char *name)
{
	(void) write(2, "Xt Error: ", 10);
	(void) write(2, name, strlen(name));
	(void) write(2, "\n", 1);
	exit(1);
}

#  define DMALLOC_DISABLE
#  define	DMALLOC_DEFAULT_LINE	0

#  define DMALLOC_FUNC_MALLOC     10/* malloc function called */
#  define DMALLOC_FUNC_CALLOC     11/* calloc function called */
#  define DMALLOC_FUNC_REALLOC    12/* realloc function called */
#  define DMALLOC_FUNC_RECALLOC   13/* recalloc called */
#  define DMALLOC_FUNC_MEMALIGN   14/* memalign function called */
#  define DMALLOC_FUNC_VALLOC     15/* valloc function called */
#  define DMALLOC_FUNC_STRDUP     16/* strdup function called */
#  define DMALLOC_FUNC_FREE       17/* free function called */
#  define DMALLOC_FUNC_CFREE      18/* cfree function called */

/*
 * For use with dmalloc, a malloc debugging package.
 * Mimick Xt behaviour ...
 * NEVER call them directly!
 */

extern XtPointer _XbaeDebugMalloc(const char *f, int l, Cardinal size)
{
	XtPointer r = NULL;

	if(size == 0)
	{
		size = 1;
	}

#  if DMALLOC_VERSION_MAJOR < 5
	r = _malloc_leap(f, l, size);
#  else
	r = dmalloc_malloc(f, l, size, DMALLOC_FUNC_MALLOC, 0, 0);
#  endif

	if(r == NULL)
	{
		XbaeAllocError("malloc");
	}

	return r;
}

extern XtPointer _XbaeDebugCalloc(const char *f, int l, Cardinal count, Cardinal size)
{
	XtPointer r = NULL;

	if(size == 0 || count == 0)
	{
		count = size = 1;
	}

#  if DMALLOC_VERSION_MAJOR < 5
	r = _calloc_leap(f, l, count, size);
#  else
	r = dmalloc_malloc(f, l, count * size, DMALLOC_FUNC_CALLOC, 0, 0);
#  endif

	if(r == NULL)
	{
		XbaeAllocError("calloc");
	}

	return r;
}

extern XtPointer _XbaeDebugRealloc(const char *f, int l, XtPointer p, Cardinal size)
{
	XtPointer r = NULL;

	if(p == NULL)
	{
		if(size == 0)
		{
			size = 1;
		}
#  if DMALLOC_VERSION_MAJOR < 5
		r = _malloc_leap(f, l, size);
#  else
		r = dmalloc_malloc(f, l, size, DMALLOC_FUNC_REALLOC, 0, 0);
#  endif
	}
	else if(size == 0)
	{
#  if DMALLOC_VERSION_MAJOR < 5
		_free_leap(f, l, p);
#  else
		dmalloc_free(f, l, p, DMALLOC_FUNC_FREE);
#  endif
		return NULL;
	}
	else
	{
#  if DMALLOC_VERSION_MAJOR < 5
		r = _realloc_leap(f, l, p, size);
#  else
		r = dmalloc_realloc(f, l, p, size, DMALLOC_FUNC_REALLOC, 0);
#  endif
	}

	if(r == NULL)
	{
		XbaeAllocError("realloc");
	}

	return r;
}

extern void _XbaeDebugFree(const char *f, int l, XtPointer p)
{
	if(p)
	{
#  if DMALLOC_VERSION_MAJOR < 5
		_free_leap(f, l, p);
#  else
		dmalloc_free(f, l, p, DMALLOC_FUNC_FREE);
#  endif
	}
}

#  define DO_XT_ENTRY_POINTS 1
#  if DO_XT_ENTRY_POINTS
/*
 * Provide malloc library replacements for X Window System heap routines
 * 	XtCalloc
 * 	XtFree
 * 	XtMalloc
 * 	XtRealloc
 * so that we can get accurate caller data.
 *
 * David Hill
 */

#    if 0
#      include "/home/danny/src/dmalloc/dmalloc-5.0.2/return.h"
#    endif

#    ifndef GET_RET_ADDR
#      define GET_RET_ADDR(file) do { file = "unknown";} while (0)
#    endif

#    ifdef	XtMalloc
#      undef	XtMalloc
#    endif
char *XtMalloc(unsigned size)
{
	char *file;

	GET_RET_ADDR(file);

	return _XbaeDebugMalloc(file, DMALLOC_DEFAULT_LINE, size);
}

#    ifdef	XtCalloc
#      undef	XtCalloc
#    endif
char *XtCalloc(unsigned num_elements, unsigned size)
{
	char *file;

	GET_RET_ADDR(file);

	return _XbaeDebugCalloc(file, DMALLOC_DEFAULT_LINE, num_elements, size);
}

#    ifdef	XtRealloc
#      undef	XtRealloc
#    endif
char *XtRealloc(char *ptr, unsigned size)
{
	char *file;

	GET_RET_ADDR(file);

	return _XbaeDebugRealloc(file, DMALLOC_DEFAULT_LINE, ptr, size);
}

#    ifdef	XtFree
#      undef	XtFree
#    endif
void XtFree(char *ptr)
{
	char *file;

	GET_RET_ADDR(file);

	_XbaeDebugFree(file, DMALLOC_DEFAULT_LINE, ptr);
}

#  endif
	   /* DO_XT_ENTRY_POINTS */

#else

static void XbaeAllocError(const char *name)
{
	fprintf(stderr, "Xbae Error: Xbae was not compiled with memory allocation"
			"debugging but %s was called. Your linking is broken\n", name);
	exit(1);
}

extern XtPointer _XbaeDebugMalloc(const char *f, int l, Cardinal size)
{
	XbaeAllocError("_XbaeDebugMalloc");
	return NULL;
}

extern XtPointer _XbaeDebugCalloc(const char *f, int l, Cardinal count, Cardinal size)
{
	XbaeAllocError("_XbaeDebugCalloc");
	return NULL;
}

extern XtPointer _XbaeDebugRealloc(const char *f, int l, XtPointer p, Cardinal size)
{
	XbaeAllocError("_XbaeDebugRealloc");
	return NULL;
}

extern void _XbaeDebugFree(const char *f, int l, XtPointer p)
{
	XbaeAllocError("_XbaeDebugFree");
}

#endif /* WITH_DMALLOC */
