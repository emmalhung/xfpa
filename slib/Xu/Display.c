/*==========================================================================*/
/**
 *
 * \file	Display.c
 *
 * \brief	Display information functions
 *
 *     Version 8 (c) Copyright 2011 Environment Canada
 *
 *   This file is part of the Forecast Production Assistant (FPA).
 *   The FPA is free software: you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   any later version.
 *
 *   The FPA is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *   See the GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.
 *			
*==========================================================================*/
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <Xm/Xm.h>
#include <Xm/Display.h>
#include "XuP.h"

static void standard_display_string( String, size_t, String );


/**
 * \brief	Update of all displays.
 *
 * Update all displays opened by the Xu library
 *
 * \param[in]	w	Reference widget of a display
 *
 * \note The widget is not actually used but is included for compatability
 *       with XmUpdateDisplay, so the argument can be NULL.
 */
/*ARGSUSED*/
void XuUpdateDisplay( Widget w )
{
	int n;
	for(n=0; n<Fxu.ndd; n++)
		XmUpdateDisplay(XmGetXmDisplay(Fxu.dd[n]->display));
}


/**
 * \brief Returns a display pointer if the given string is a valid display.
 *
 *  \param[in] display_string	A string representing a display (host:display.screen)
 *
 *  \return The display pointer or NULL if not a valid display.
 */
Display *XuIsValidDisplayString( String display_string )
{
	DPYPTR dp = _xu_open_display(display_string, NULL, 0, NULL, NULL);
	if (dp) return dp->display;
	return (Display *)0;
}


/* Put the display string entered as a parameter into standard format. This
*  is defined as host:display.screen. The returned string is allocated memory
*  and it is the responsibility of the calling program to free this. 
*/
/**
 * \brief Put the given display string into a standard format.
 *
 * \param[in] display_string A display string that may be a partial
 *                           representation of the display such as
 *                           ":0"
 *
 * \return	A display string in the standard format
 *
 * \attention	The returned string is allocated memory which must be freed
 *				by the calling application.
 */
String XuStandardDisplayString( String display_string )
{
	char  mbuf[200];
	standard_display_string(mbuf, sizeof(mbuf), display_string);
	return XtNewString(mbuf);
}


/*============= Internal Library Functions =====================*/


/* Put the display string entered as a parameter into standard format. This
*  is defined as host:display.screen. The returned string is allocated memory
*  and it is the responsibility of the calling program to free this. 
*/
static void standard_display_string( String buffer, size_t len, String display_string )
{
	char  *dn, *p, *b, *e;

	(void) memset(buffer, 0, len);

	b  = buffer;
	dn = XDisplayName(display_string);

	/*  First check for the existance of a ':'. If there is not one make it the same
	 *  as the default display or just add a colon.
	 */
	if( (strchr(dn,(int)':')) == NULL )
	{
		String nam = (Fxu.ndd > 0)? XDisplayString(DefaultAppDisplayInfo->display) : XDisplayName(NULL);
		if((e = strchr(nam,(int)':')))
		{
			while(nam != e) *b++ = *nam++;
		}
		*b++ = ':';
	}

	/* Add the input display string and make sure there is an explicit display.
	 */
	p = dn;
	while(*p)
	{
		if(isprint((int)(*p)))
		{
			if(*p == '.' && *(b-1) == ':') *b++ = '0';
			*b++ = *p;
		}
		p++;
	}

	/* The following makes sure we have an explicit screen reference
	 */
	if(*(b-1) == '.') *b++ = '0';
	if((strrchr(strchr(buffer,(int)':'),(int)'.')) == NULL) (void) strcat(buffer, ".0");
}




/* The functions in this block allow for the display on which any given
*  top level dialog is to be activate on to be specified by a display
*  string from either the resource file or the dialog argument list.
*  This uses the resource id XuNdialogDisplay or the resource file entry
*  dialogDisplay. Both take as input the form host:display.screen
*/

static void nullTimerProc()
{
	/* This empty timer proc is used to work around a bug which prevents
	*  the main loop from seeing events from a new display the first
	*  time around. This was true with Motif1.2 and I don't know if this
	*  is the case with Motif2.x, but keeping it here does no harm.
	*/
}


/* Open the specified display and add the information to our display information array. If the
 * display has already been opened we can just return a pointer to the info struct.
 */
DPYPTR _xu_open_display( String display_string, XrmOptionDescRec *options, Cardinal num_options,
							String *argv, int *argc)
{
	int     n;
	char    dpyname[200];
	String  p, s;
	Display *dpy;
	DPYPTR  dd;

	standard_display_string(dpyname, sizeof(dpyname), display_string);

	/* Do we already have this display in our list?
	*/
	for(n = 0; n < Fxu.ndd; n++ )
		if(same(Fxu.dd[n]->name, dpyname)) return Fxu.dd[n];

	/* If not doing initialization the argc parameter will normally be NULL and we need a variable */
	n = (argc)? *argc:0;
	dpy = XtOpenDisplay(Fxu.app_context, dpyname, Fxu.app_name, Fxu.app_class, options, num_options, &n, argv);
	if (argc) *argc = n;
	if(!dpy)
	{
		/* No display_string means we are doing initialization and the error is fatal */
		(void) fprintf(stderr, "%s: Unable to open display \"%s\".\n", Fxu.app_name, dpyname);
		if(!display_string) exit(1);
		return NULL;
	}

	Fxu.dd = (DPYPTR *)XtRealloc((char*)Fxu.dd, (Cardinal)(Fxu.ndd+1) * sizeof(DPYPTR));
	dd = Fxu.dd[Fxu.ndd] = XTCALLOC(1,DPYDATA);
	Fxu.ndd++;

	dd->display = dpy;
	dd->name    = XtNewString(dpyname);
	dd->id      = XTCALLOC(strlen(dpyname), char);
	dd->width   = XDisplayWidth(dpy, DefaultScreen(dpy));
	dd->height  = XDisplayHeight(dpy, DefaultScreen(dpy));

	_xu_set_visual(dpy, DefaultScreen(dpy), &dd->depth, &dd->visual, &dd->cmap);

	/* Create the display id */
	for(s = dd->name, p = dd->id; *s; s++)
		if(isalnum((int)*s)) *p++ = *s;

	/* See nullTimerProc above for the reason for this */
	(void) XtAppAddTimeOut(Fxu.app_context, 1, (XtTimerCallbackProc)nullTimerProc, NULL);

	return dd;
}


DPYPTR _xu_find_display_info_from_widget(Widget wid)
{
	int n;

	if(!wid) return DefaultAppDisplayInfo;

	for( n = 0; n < Fxu.ndd; n++ )
	{
		if(XtDisplay(wid) == Fxu.dd[n]->display) return Fxu.dd[n];
	}
	return DefaultAppDisplayInfo;
}
