/********************************************************************/
/*
*     File:  motif.c
*
*     Purpose: Contains functions which allow the library to
*              interface with motif.
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
/********************************************************************/

/* These are to stop complaints from the compiler
 */
#undef bzero
#undef bcopy
#include <Xm/Xm.h>

#include "FpaXglP.h"


int glInitWidgetWindow(Widget w)
{
	static char *MyName = "glInitWidgetWindow";
	glInit();
	if(!XtIsRealized(w))
	{
		pr_error(MyName, "Widget \"%s\" is not realized.\n", XtName(w));
		return 0;
	}
	else
	{
		return glInitWindow(XtDisplay(w), XtWindow(w));
	}
}
