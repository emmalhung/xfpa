/*================================================================*/
/*
*	ShellControl() - Contains utility functions to create and set
*	                 various shell parameters.
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

#include <stdarg.h>
#include <X11/IntrinsicP.h>
#include <X11/CoreP.h>
#include <X11/Shell.h>
#include "XuP.h"


/* In Motif1.x the popup shell did not inherit the depth, visual and colormap of the
 * parent widget. If the parent did not have the default values for these, then the
 * popup would cause an abort due to non-matching values. This code propogates these
 * values. This problem does not exist in Motif2.x (I think), but this provides us
 * with backwards compatability. I used a fixed buffer for Arg, as there should never
 * be more than 30 arguments passed into the shell creation.
 */
Widget XuCreatePopupShell(String _name, WidgetClass _wc, Widget _parent, ArgList _arg, int _narg)
{
	int      argc, i, depth;
	Arg      args[30];
	Widget   parentShell;
	Visual   *visual;
	Colormap colormap;

	parentShell = XuGetShell(_parent);
	if (!parentShell) return NullWidget;

	argc = 0;
	XtSetArg(args[argc], XtNdepth, &depth); argc++;
	XtSetArg(args[argc], XtNvisual, &visual); argc++;
	XtSetArg(args[argc], XtNcolormap, &colormap); argc++;
	XtGetValues(parentShell, args, argc);

	argc = 0;
	XtSetArg(args[argc], XtNdepth, depth); argc++;
	XtSetArg(args[argc], XtNvisual, visual); argc++;
	XtSetArg(args[argc], XtNcolormap, colormap); argc++;
	
	for( i = 0; i < _narg, argc < 30; i++, argc++ )
	{
		args[argc].name  = _arg[i].name;
		args[argc].value = _arg[i].value;
	}

	return XtCreatePopupShell(_name, _wc, _parent, args, argc);
}


Widget XuVaCreatePopupShell(String _name, WidgetClass _wc, Widget _parent, ...)
{
	int      argc, depth;
	String   argname;
	Arg      args[30];
	Widget   parentShell;
	Visual   *visual;
	Colormap colormap;
	va_list  ap;

	parentShell = XuGetShell(_parent);
	if (!parentShell) return NullWidget;

	argc = 0;
	XtSetArg(args[argc], XtNdepth, &depth); argc++;
	XtSetArg(args[argc], XtNvisual, &visual); argc++;
	XtSetArg(args[argc], XtNcolormap, &colormap); argc++;
	XtGetValues(parentShell, args, argc);

	argc = 0;
	XtSetArg(args[argc], XtNdepth, depth); argc++;
	XtSetArg(args[argc], XtNvisual, visual); argc++;
	XtSetArg(args[argc], XtNcolormap, colormap); argc++;
	
	va_start(ap,_parent);
	while(NotNull(argname = va_arg(ap,String)) && argc < 30)
	{
		XtSetArg(args[argc], argname, va_arg(ap,XtArgVal)); argc++;
	}
	va_end(ap);

	return XtCreatePopupShell(_name, _wc, _parent, args, argc);
}


/* Get the shell of any widget
 */
Widget XuGetShell (Widget _w)
{
	Widget temp;

	if (IsNull(_w)) return NullWidget;

	temp = _w;
	while(NotNull(temp) && !XtIsSubclass(temp, shellWidgetClass))
		temp = XtParent(temp);
	return temp;
}


Widget XuGetShellChild (Widget _w)
{
	Widget     w;
	Cardinal   numKids;
	WidgetList kids;

	if(IsNull(w = XuGetShell(_w))) return NullWidget;

	XtVaGetValues(w, XtNnumChildren, &numKids, XtNchildren, &kids, NULL);
	if(numKids == 0) return NullWidget;
	return kids[0];
}


Boolean XuShellVisible (Widget _w)
{
	XWindowAttributes wa;

	if(IsNull(_w) || !XtIsRealized(_w)) return False;

	_w = XuGetShell(_w);
	XGetWindowAttributes(XtDisplay(_w), XtWindow(_w), &wa);
	return (wa.map_state == IsViewable);
}


void XuShellAllowResize(Widget _shell , int _flags )
{
    Arg warg[4];
    int n;

	if(IsNull(_shell = XuGetShell(_shell))) return;

    n = 0;
    if (_flags & CWWidth)
    {
        XtSetArg(warg[n], XtNminWidth, 0); n++;
        XtSetArg(warg[n], XtNmaxWidth, 32767); n++;
    }
    if (_flags & CWHeight)
    {
        XtSetArg(warg[n], XtNminHeight, 0); n++;
        XtSetArg(warg[n], XtNmaxHeight, 32767); n++;
    }
    if (n) XtSetValues(_shell, warg, n);
}


void XuShellRestrictResize(Widget _shell , int _flags )
{
    Widget child;
    Arg warg[4];
    int n;

	if(IsNull(_shell = XuGetShell(_shell))) return;
    if(IsNull(child = XuGetShellChild(_shell))) return;

    n = 0;
    if (_flags & CWWidth)
    {
        XtSetArg(warg[n], XtNminWidth, child->core.width); n++;
        XtSetArg(warg[n], XtNmaxWidth, child->core.width); n++;
    }
    if (_flags & CWHeight)
    {
        XtSetArg(warg[n], XtNminHeight, child->core.height); n++;
        XtSetArg(warg[n], XtNmaxHeight, child->core.height); n++;
    }
    if (n) XtSetValues(_shell, warg, n);
}


void XuShellCenter(Widget _shell)
{
    Position x, y;
    Widget   child;
	Display  *dpy;

	if(IsNull(_shell = XuGetShell(_shell))) return;

	dpy   = XtDisplay(_shell);
    child = XuGetShellChild(_shell);

    if (IsNull(child)) child = _shell;

    x = (Position)(XDisplayWidth(dpy, XDefaultScreen(dpy)) - child->core.width)/2;
    y = (Position)(XDisplayHeight(dpy, XDefaultScreen(dpy)) - child->core.height)/2;

    XtVaSetValues(_shell, XtNx, x, XtNy, y, NULL);
}
