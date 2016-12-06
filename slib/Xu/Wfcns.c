/*   Small Widget utility functions
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
#include "XuP.h"
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>
#include <stdarg.h>


/*
 * Allocates a string array and prints the given parameters into it. The return
 * is an allocated array which must be free'd using XtFree after use. Note that
 * this uses the glibc function vsnprintf to do the printing and to determine
 * if the allocated buffer needs to be increased in size.
 */
static String print_buf(String fmt, va_list ap)
{
	int     n, size = 250; /* arbitrary but should be ok in most cases */
	String  p, np;

	if ((p = XtMalloc(size)) == NULL) return NULL;

	while (1)
	{
		n = vsnprintf (p, size, fmt, ap);
		if (n > -1 && n < size) break;
		if (n > -1)			/* glibc 2.1 */
			size = n+1;		/* precisely what is needed */
		else				/* glibc 2.0 and HPUX 11 */
			size *= 2;		/* twice the old size */

		if ((np = XtRealloc (p, size)) == NULL) {
			XtFree(p);
			return NULL;
		} else {
			p = np;
		}
	}
	return p;

}



/* A convenient function that displays text in a label or button,
 * providing the same syntax as printf()
 */
void XuWidgetPrint(Widget w, String format, ...)
{
	if(!w)
	{
        (void) fprintf(stderr, "XuWidgetPrint() requires a non-null widget");
	}
	else if(!XtIsSubclass(w, xmLabelWidgetClass) && !XtIsSubclass(w, xmLabelGadgetClass)) 
	{
        (void) fprintf(stderr, "XuWidgetPrint() requires a Label Subclass Widget");
	}
	else
	{
		String    p;
		va_list   ap;
		va_start(ap, format);
		if((p = print_buf(format, ap)))
		{
			XmString xms;
			xms = _xu_xmstring_create(w, p, NULL);
			XtVaSetValues(w, XmNlabelString, xms, NULL);
			XmStringFree(xms);
			XtFree(p);
		}
		va_end(ap);
	}
}


/* Sets the label of the given widget to the given string.
 */
void XuWidgetLabel(Widget w, String str)
{
	XmString xms = _xu_xmstring_create(w, str, NULL);
	XtVaSetValues(w, XmNlabelString, xms, NULL);
	XmStringFree(xms);
}


/*   If a toggle button needs to be set and notification sent, but is
 *   already on (or off), the toggle will not action unless it is first
 *   set to the opposite state.
 *
 *   2005.04.15: First line in couple set to !state instead of False.
 */
void XuToggleButtonSet(Widget w, const Boolean state, const Boolean notify )

{
	if(!w) return;

	if(XtIsSubclass(w, xmToggleButtonGadgetClass))
	{
		XmToggleButtonGadgetSetState(w, !state, False );
		XmToggleButtonGadgetSetState(w, state, notify);
	}
	else
	{
		XmToggleButtonSetState(w, !state, False );
		XmToggleButtonSetState(w, state, notify);
	}
}
