/*================================================================================*/
/*
*    File: selector_targetTime.c
*
*    Purpose: Provides functions which can be called to create and manipulate
*             a target time object with time increment and decrement arrows.
*
*    Functions: void      CreateTargetTimeControl(parent, type, callback, ...)
*               void      TargetTimeSetStrTime(Widget, String, Boolean)
*               String    TargetTimeGetStrTime(Widget)
*               void      TargetTimeSetFormatType(type)
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
/*================================================================================*/
#include <stdarg.h>
#include "global.h"
#include <Xm/Column.h>
#include <Xm/Form.h>
#include <Xm/XmpSpinBox.h>
#include "selector.h"

#define SPINBOX "targetTimeSpinbox"
typedef void (*CBF)(Widget);


/*ARGSUSED*/
static void SpinboxCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	CBF rtnfcn;
	rtnfcn = (CBF)client_data;
	if(rtnfcn) rtnfcn(XtParent(w));
}


Widget CreateTargetTimeControl( Widget parent, DATE_DISPLAY type, CBF cbf, ...)
{	
	int     ac;
	long    incr;
	String  cmd, fmt, mfmt, hfmt, dfmt;
	Arg     al[30];
	Widget  w, form;
	va_list args;

	GetDateFormats(&mfmt, &hfmt, &dfmt);

	/* Note that incr must be in seconds, thus the 60 factor */
	switch(type)
	{
		case DATE_TO_MINUTE:          fmt = mfmt; incr = 60 * GV_interp_time_delta; break;
		case DATE_TO_MINUTE_NO_LABEL: fmt = mfmt; incr = 60 * GV_interp_time_delta; break;
		case DATE_TO_HOUR:            fmt = hfmt; incr = 60 * GV_interp_time_delta; break;
		case DATE_TO_HOUR_NO_LABEL:   fmt = hfmt; incr = 60 * GV_interp_time_delta; break;
		case DATE_TO_DAY:             fmt = dfmt; incr = 86400;                     break;
		default:                      fmt = hfmt; incr = 60 * GV_interp_time_delta; break;
	}

	ac = 0;

    va_start(args, cbf);
    while(NotNull(cmd = va_arg(args, String)))
    {
		if(ac < 22)
		{
			XtSetArg(al[ac], cmd, va_arg(args, XtPointer)); ac++;
		}
		else
		{
			pr_error(SPINBOX,"XmN settings exceeded the maximum. Increase al count.\n");
		}
    }
    va_end(args);

	XtSetArg(al[ac], XmNspinBoxType, XmSPINBOX_DATE); ac++;
	XtSetArg(al[ac], XmNdateFormat, fmt); ac++;
	XtSetArg(al[ac], XmNeditable, False); ac++;
	XtSetArg(al[ac], XmNmaximum, INT_MAX); ac++;
	XtSetArg(al[ac], XmNminimum, INT_MIN); ac++;
	XtSetArg(al[ac], XmNincrement, incr); ac++;
	XtSetArg(al[ac], XmNvalue, 0); ac++;

	if(!DATE_TO_MINUTE_NO_LABEL && !DATE_TO_HOUR_NO_LABEL)
	{
		form = XmCreateColumn(parent, "targetTime", al, ac);	
		w = XmpCreateSpinBox(form, SPINBOX, al, ac);
		XtAddCallback(w, XmNvalueChangedCallback, SpinboxCB, (XtPointer)cbf);
		XtManageChild(w);
		XtManageChild(form);
		return form;
	}
	else
	{
		w = XmpCreateSpinBox(parent, SPINBOX, al, ac);
		XtAddCallback(w, XmNvalueChangedCallback, SpinboxCB, (XtPointer)cbf);
		XtManageChild(w);
		return w;
	}
}


void TargetTimeSetStrTime( Widget w, String dt, Boolean make_callback)
{
	int    year, jday, hour, min;
	Widget sb;

	if(parse_tstamp(dt, &year, &jday, &hour, &min, NULL, NULL))
	{
		long sec = encode_clock(year, jday, hour, min, 0);

		sb = XtNameToWidget(w,SPINBOX);
		if (!sb) sb = w;
		/*
		 * 2007.10.24: If the time is the same as that already in the spinbox the
		 * callback will not be done. Setting the time and doing the set value
		 * again ensures that the callback is executed.
		 */
		if(make_callback)
			XmpSpinBoxSetValue(sb, sec+1, False);
		XmpSpinBoxSetValue(sb, sec, make_callback);
	}
}


String TargetTimeGetStrTime(Widget w)
{
	int  year, jday, hour, min, sec;
	long t;
	Widget sb;
	static TSTAMP mbuf;

	sb = XtNameToWidget(w,SPINBOX);
	if (!sb) sb = w;

	t = XmpSpinBoxGetValue(sb);
	decode_clock(t, &year, &jday, &hour, &min, &sec);
	strcpy(mbuf, build_tstamp(year, jday, hour, min, False, minutes_in_depictions()));
	return mbuf;
}


void TargetTimeSetFormatType( Widget w, DATE_DISPLAY type )
{
	long   incr, value;
	String fmt, mfmt, hfmt, dfmt;
	Widget sb;

	sb = XtNameToWidget(w,SPINBOX);
	if (!sb) sb = w;

	GetDateFormats(&mfmt, &hfmt, &dfmt);

	/* Note that incr must be in seconds, thus the 60 factor */
	switch(type)
	{
		case DATE_TO_MINUTE:          fmt = mfmt; incr = 60 * GV_interp_time_delta; break;
		case DATE_TO_MINUTE_NO_LABEL: fmt = mfmt; incr = 60 * GV_interp_time_delta; break;
		case DATE_TO_HOUR:            fmt = hfmt; incr = 60 * GV_interp_time_delta; break;
		case DATE_TO_HOUR_NO_LABEL:   fmt = hfmt; incr = 60 * GV_interp_time_delta; break;
		case DATE_TO_DAY:             fmt = dfmt; incr = 86400;                     break;
		default:                      fmt = hfmt; incr = 60 * GV_interp_time_delta; break;
	}

	value = XmpSpinBoxGetValue(sb);

	XtVaSetValues(sb,
		XmNvalue, 0,
		XmNdateFormat, fmt,
		XmNincrement, incr,
		NULL);

	XmpSpinBoxSetValue(sb, value, False);
}
