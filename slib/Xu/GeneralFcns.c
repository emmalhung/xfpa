/*================================================================*/
/*
*	GeneralFcns() - Utility functions and misc initializations.
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
*/
/*================================================================*/

#define  XULIBMAIN
#include "XuP.h"

/* Here we initialize the only variable visible to the external world.
*/
XuControl Fxu = {
	(String)0,			/* app_name */
	(String)0,			/* app_class */
	NULL,				/* app_context */
	(Widget)0,			/* top_level */
	NULL,				/* top_icon_file */
	NULL,               /* t0p_icon_mask_file */
	(String)0,			/* home_dir */
	(FILE *)0,			/* mdbfp */
	0,					/* ndd */
	(DPYPTR *)0,		/* dd */
	0,					/* pixmap_cache_len */
	0,					/* pixmap_cache */
	0,					/* bndx */
	0,					/* gndx */
	0,					/* andx */
	0,					/* nsf */
	NULL,				/* sf */
	0,					/* dlalloc */
	0,					/* ndl */
	NULL,				/* dl */
	20,                 /* tightness */
	5,	                /* margins */
	3,					/* button_margins */
	NULL                /* default_action_id */
};


/* This does nothing but hide XtFree from other functions that
 * know nothing about Xt.
 */
void XuFree( void *val )
{
	XtFree(val);
}


/* Parses a buffer containing a list of comma separated items and
 * returns them as an allocated string array.
 */
int _xu_parse_comma_separated_list( String buffer, String **list )
{
	int     n;
	String  p, s, buf, *l;

	if(list) *list = NULL;
	if(blank(buffer)) return 0;

	/* count commas to allocate the string array */
	s = buf = XtNewString(buffer);
	n = 1;
	while((p = strchr(s,',')))
	{
		s = p+1;
		n++;
	}

	/* ignore all entries consisting entirely of white space */
	l = XTCALLOC(n, String);
	s = buf;
	n = 0;
	while((p = strchr(s,',')))
	{
		*p = '\0';
		no_white(s);
		if(!blank(s)) l[n++] = XtNewString(s);
		s = p+1;
	}
	if(!blank(s))
	{
		no_white(s);
		if(!blank(s)) l[n++] = XtNewString(s);
	}
	*list = l;

	XtFree(buf);
	return n;
}


/* Is the given item in the given comma separated list buffer?
 * Leading and trailing white space are ignored
 */
Boolean _xu_in_comma_separated_list( String buffer, String item )
{
	char    *s, *p, *buf;
	Boolean rtn = False;

	if(blank(buffer) || blank(item)) return False;

	s = buf = XtNewString(buffer);
	while(!rtn && (p = strchr(s,',')))
	{
		*p = '\0';
		no_white(s);
		rtn = same(s,item);
		s = p+1;
	}
	if(!rtn)
	{
		no_white(s);
		rtn = same(s,item);
	}
	XtFree(buf);
	return rtn;
}
