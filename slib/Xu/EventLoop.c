#include "XuP.h"
/*============================================================================*/
/**
 * \file EventLoop.c
 *
 * \brief Functions that go into a local event processing loop until a given
 *        condition is met.
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
/*============================================================================*/


static Boolean wait_here;


/*ARGSUSED*/
static void delay(XtPointer client_data , XtIntervalId id )
{
	wait_here = False;
}

/**
 * \brief Pauses processing for the interval (in milliseconds) specified.
 *
 * param[in] w			The reference widget.
 * param[in] interval	The time in milliseconds to pause for.
 */
void XuDelay(Widget w, long interval)
{
	wait_here = True;
	XtAppAddTimeOut(Fxu.app_context, interval, (XtTimerCallbackProc)delay, (XtPointer)NULL);
	while(wait_here || XtAppPending(Fxu.app_context)) XtAppProcessEvent(Fxu.app_context, XtIMAll);
}

/**
 * \brief Goes into a local event processing loop until either the given number of
 *   milliseconds has passed or until the flag is false. If the interval is 0 then
 *   the loop will check for the flag state only. If flag is NULL then the loop
 *   will only delay for interval milliseconds.
 *
 *   param[in] w        Reference widget
 *   param[in] interval Interval to wait in milliseconds
 *   param[in] *flag    Do while true, Note that the address of the flag must be passed in.
 */
void XuProcessEventLoopWhile( Widget w, long interval, Boolean *flag )
{
	if(interval > 0)
	{
		wait_here = True;
		XtAppAddTimeOut(Fxu.app_context, interval,
			(XtTimerCallbackProc)delay, (XtPointer)NULL);
		if(flag)
		{
			while((wait_here && *flag) ||  XtAppPending(Fxu.app_context))
				XtAppProcessEvent(Fxu.app_context, XtIMAll);
		}
		else
		{
			while(wait_here || XtAppPending(Fxu.app_context))
				XtAppProcessEvent(Fxu.app_context, XtIMAll);
		}
	}
	else if(flag)
	{
		while(*flag ||  XtAppPending(Fxu.app_context))
			XtAppProcessEvent(Fxu.app_context, XtIMAll);
	}
}
