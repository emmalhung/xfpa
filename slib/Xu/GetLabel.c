/*=========================================================================*/
/*
*	XuGetLabel() - Provides a mechanism for the retrevial and storage of
*                  short "label" strings from the <app>Mdb resource file
*   database.  The string must be stored in the format as:
*
*       app.key.label: Label string wanted
*
*   where app is the application name in the resource file, key is the
*   key associated with the label and label is a literal which must be
*   the literal string label.
*
*   There are three functions contained in this file:
*
*       XuAssignLabel - get label and keep a permanent copy.
*       XuGetLabel    - get label straight from the database.
*       XuGetLabelUc  - get and convert to uppercase
*       XuGetLabelLc  - get and convert to lowercase
*       XuGetXmLabel  - get label from the database and return in
*                       XmString format.
*
*   Note: Up to 20 labels and their keys are stored as a "history".
*   This may seem redundant, but this function may be called several times
*   in one operation and we do not want the storage buffer overwritten.
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
/*=========================================================================*/

#include <limits.h>
#include <ctype.h>
#include "XuP.h"

/*
*   Returns a label from the message database with the given types.
*
*               'l' = lower case
*               'u' = upper case
*               'f' = lower case with first leter upper case
*               'a' = return "as is" from the message database
*/
static String get_modified_label(String key , const char type )
{
	int i, n;
	String p;

	static int ns = 0;
	static String s = (String)NULL;

	p = XuFindKeyLine(key, "label", key);
	if((n = (int) safe_strlen(p)) > ns)
	{
		ns = n;
		s = (String)XtRealloc(s, (Cardinal)(ns+1));
	}
	(void) safe_strcpy(s, p);

	switch(type)
	{
		case 'u':
		case 'U':
			for(i = 0; i < n; i++) s[i] = (char) toupper((int)s[i]);
			break;
		case 'l':
		case 'L':
			for(i = 0; i < n; i++) s[i] = (char) tolower((int)s[i]);
			break;
		case 'f':
		case 'F':
			s[0] = (char) toupper((int)s[0]);
			for(i = 1; i < n; i++) s[i] = (char) tolower((int)s[i]);
			break;
	}
	return s;
}


String XuGetLabel(String key )
{
	return get_modified_label(key, 'a');
}

String XuGetLabelUc(String key )
{
	return get_modified_label(key, 'u');
}

String XuGetLabelLc(String key )
{
	return get_modified_label(key, 'l');
}

XmString XuGetXmLabel(String key )
{
	return XuNewXmString(get_modified_label(key, 'a'));
}

String XuAssignLabel(String key)
{
	int n;

	static int    nlist = 0;
	static String *keys = (String*)NULL;
	static String *list = (String*)NULL;

	for(n = 0; n < nlist; n++)
	{
		if(same(key, keys[n])) return list[n];
	}

	nlist++;
	keys = (String*)XtRealloc((void*)keys, (Cardinal)(nlist*sizeof(String)));
	list = (String*)XtRealloc((void*)list, (Cardinal)(nlist*sizeof(String)));

	keys[n] = XtNewString(key);
	list[n] = XtNewString(XuGetLabel(key));
	return list[n];
}
