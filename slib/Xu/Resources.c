/*================================================================*/
/*
*	File:    Resources.c
*
*	Purpose: Get and Put resources from and to the database.
*
*  Get Fcns: Extract a resource from the database given the
*            resource name.
*
*             Boolean  XuGetBooleanResource()
*             Boolean  XuVaGetBooleanResource()
*             int      XuGetIntResource()
*             int      XuVaGetIntResource()
*             String   XuGetStringResource()
*             String   XuVaGetStringResource()
*             String   XuGetLabelResource()
*             XmString XuGetXmStringResource()
*             XmString XuVaGetXmStringResource()
*
*  Put Fcns: Put a resource value into the database.
*
*             void XuPutStringResource()
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

#include <ctype.h>
#include <stdarg.h>
#include <Xm/Display.h>
#include "XuP.h"

#define BUFLEN 256
const String noPrefix = "";
const String noSuffix = "";


static String get_resource(String prefix, String res, String suffix)
{
    char   class[BUFLEN], name[BUFLEN];
	String cp, rtype;
    XrmValue  value;

	if(blank(res)) return NULL;

	(void) snprintf(name, BUFLEN, "%s%s%s%s", Fxu.app_name, prefix, res, suffix);
	(void) snprintf(class, BUFLEN, "%s%s%s%s", Fxu.app_class, prefix, res, suffix);

    /* Raise case of all chars after "." or "*" in class buffer. */
	cp = class;
	while ((cp = strpbrk(cp, "*.")) != NULL)
	{
	   	cp++;
		*cp = (char) toupper((int)*cp);
	}

	(void) memset((void*)&value, 0, sizeof(value));
	if(XrmGetResource(XtDatabase(DefaultAppDisplayInfo->display), name, class, &rtype, &value))
		return (String)value.addr;

	return NULL;
}


/*============= Public Functions =================*/


/* Assume that the given resource name is a part of a longer actual resource
 * name like "*.resName.labelString". This results in easier to read code
 * when looking for a label in the resource file.
 */
String XuGetLabelResource(String resName, String defaultLabel)
{
	char buf[BUFLEN];
	String ptr;
	const String ls = ".labelString";

	if(blank(resName)) return defaultLabel;

	(void) memset(buf, 0, BUFLEN);
	(void) strncpy(buf, resName, BUFLEN-1);
	no_white(buf);
	/* 
	 * Does the resource name start with a wild card?
	 */
	if (strspn(buf,"*.") > 0)
	{
		/* Yes so try it with and without labelString */
		ptr = get_resource(noPrefix, buf, ls);
		if (!ptr) ptr = get_resource(noPrefix, buf, noSuffix);
	}
	else
	{
		/* No so try various prefix values with and without labelString */
		ptr = get_resource(".", buf, ls);
		if (!ptr) ptr = get_resource("*.", buf, ls);
		if (!ptr) ptr = get_resource(".", buf, noSuffix);
		if (!ptr) ptr = get_resource("*.", buf, noSuffix);
	}

	return((ptr)? ptr : defaultLabel);
}


/* Search for the resource name exactly as provided in the parameter list.
 */
String XuGetStringResource(String resName, String defaultValue)
{
	String ptr = get_resource(noPrefix, resName, noSuffix);
	return((ptr)? ptr : defaultValue);
}


String XuVaGetStringResource(String defaultValue, String fmt, ...)
{
	char    mbuf[BUFLEN];
	va_list args;

	va_start(args, fmt);
	(void) vsnprintf(mbuf, BUFLEN, fmt, args);
	va_end(args);
	return XuGetStringResource(mbuf, defaultValue);
}


XmString XuGetXmStringResource(String resName, String defaultValue)
{
	String s = XuGetStringResource(resName, defaultValue);
	return XuNewXmString(s);
}


XmString XuVaGetXmStringResource(String defaultValue, String fmt, ...)
{
	char    mbuf[BUFLEN];
	va_list args;

	va_start(args, fmt);
	(void) vsnprintf(mbuf, BUFLEN, fmt, args);
	va_end(args);
	return XuGetXmStringResource(mbuf, defaultValue);
}


int XuGetIntResource(String resName, int defaultValue)
{
	String sval = XuGetStringResource(resName,NULL);
	if (!blank(sval))
	{
		String endptr;
		long int val = strtol(sval, &endptr, 10);
		if(*endptr == '\0') return (int) val;
	}
	return defaultValue;
}


int XuVaGetIntResource(int defaultValue, String fmt, ...)
{
	char mbuf[BUFLEN];
	va_list args;

	va_start(args, fmt);
	(void) vsnprintf(mbuf, BUFLEN, fmt, args);
	va_end(args);
	return XuGetIntResource(mbuf, defaultValue);
}


Boolean XuGetBooleanResource(String resName, Boolean defaultValue)
{
	String res = XuGetStringResource(resName,NULL);
	if (res) return (strchr("TtYy", *res) != NULL);
	return defaultValue;
}


Boolean XuVaGetBooleanResource(Boolean defaultValue, String fmt, ...)
{
	char mbuf[BUFLEN];
	va_list args;

	va_start(args, fmt);
	(void) vsnprintf(mbuf, BUFLEN, fmt, args);
	va_end(args);
	return XuGetBooleanResource(mbuf, defaultValue);
}



void XuPutStringResource(String specifier , String value )
{
	XrmDatabase appDB, XrmGetDatabase();
	appDB = XrmGetDatabase(DefaultAppDisplayInfo->display);
	XrmPutStringResource(&appDB, specifier, value);
}

