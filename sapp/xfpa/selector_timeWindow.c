/****************************************************************************/
/*
*  File:	selector_timeWindow.c
*
*  Purpose:	 A control for setting a range of times with minimum and maximum
*            times that provide a "time window".Normally used for animation
*            purposes to allow animation over a reduced number of times.
*
*            This basically is a frame with control widgets inside. It is
*            meant to reside inside of a panel or dialog and set the size
*            of a time window in a time series.
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
/****************************************************************************/

#include <stdarg.h>
#include "global.h"
#include <Xm/ArrowB.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/XmpDoubleSS.h>
#include "selector.h"

/* 
 * Define the looping intervals when the arrows are
 * held down by the user.
 */
#define LOOP_DELAY_BEGIN			600
#define	LOOP_DELAY_DECREASE_FACTOR	1.4
#define LOOP_DELAY_MIN				25


/*
 * Time Window Widget Struct to hold the data for a given
 * instance of the widget
 */
typedef struct {
	XtAppContext app_context;
	Widget       timeWindow;
	Widget       beginLabel;
	Widget       endLabel;
	Widget       scale;
	Widget       arrow_left_left;
	Widget       arrow_left_right;
	Widget       arrow_right_left;
	Widget       arrow_right_right;
	int          area_width;
	XtIntervalId loop_id;
	long         loop_delay;
	int          loop_inc;
	void         (*callback)(TimeWindowSelectorStruct*);
	TimeWindowSelectorStruct data;
	int          ntimes;
	String       *times;
	int          begin_ndx;
	int          end_ndx;
	int          process_dirn;
} TWWS;


/*ARGSUSED*/
static void being_destroyed_cb(Widget w , XtPointer client_data , XtPointer unused )
{
	FreeItem(client_data);
}


static void set_time_label( Widget w, TSTAMP dt )
{
	int  yr, jd, hr, min, mn, dy;

	if(parse_tstamp(dt, &yr, &jd, &hr, &min, NULL, NULL))
	{
		mdate(&yr, &jd, &mn, &dy);
		XuWidgetPrint(w, "%.2d %.2d:%.2d", dy, hr, min);
	}
	else
	{
		XuWidgetLabel(w, "----");
	}
}


static void set_begin_time( TWWS *tp, int ndx )
{
	tp->begin_ndx = ndx;
	if(ndx >= 0 && tp->ntimes > 0)
	{
		set_time_label(tp->beginLabel, tp->times[tp->begin_ndx]);
	}
}


static void set_end_time( TWWS *tp, int ndx )
{
	tp->end_ndx = ndx;
	if(ndx >= 0 && tp->ntimes > 0)
	{
		set_time_label(tp->endLabel, tp->times[tp->end_ndx]);
	}
}


/* The following three functions allow the user to hold down the arrow at the
 * end of the scales that define the animation window. The times will then
 * change automatically at an increasing rate and stop when the user released
 * the arrow button.
 */

/*ARGSUSED*/
static void arrow_begin_loop( XtPointer data, XtIntervalId *id)
{
	int ndx;
	TWWS *tp = (TWWS *)data; 

	if(tp->ntimes < 1 ) return;

	ndx = tp->begin_ndx + tp->loop_inc;

	if(ndx >= tp->ntimes) return;
	if(ndx < 0)           return;

	if(tp->process_dirn == XmMAX_ON_LEFT)
	{
		if(ndx < tp->end_ndx) return;
		XmpDoubleSliderScaleSetUpperValue(tp->scale, ndx);
	}
	else
	{
		if(ndx > tp->end_ndx) return;
		XmpDoubleSliderScaleSetLowerValue(tp->scale, ndx);
	}
	set_begin_time(tp, ndx);
	set_time_label(tp->beginLabel, tp->times[ndx]);

	tp->loop_id = XtAppAddTimeOut(tp->app_context, tp->loop_delay, arrow_begin_loop, data);
	tp->loop_delay = (int)((float)tp->loop_delay / LOOP_DELAY_DECREASE_FACTOR);
	if(tp->loop_delay < LOOP_DELAY_MIN) tp->loop_delay = LOOP_DELAY_MIN;
}


/*ARGSUSED*/
static void arrow_end_loop( XtPointer data, XtIntervalId *id)
{
	int ndx;
	TWWS *tp = (TWWS *)data; 

	if(tp->ntimes < 1 ) return;

	ndx = tp->end_ndx + tp->loop_inc;
	if(ndx >= tp->ntimes) return;
	if(ndx < 0)           return;

	if(tp->process_dirn == XmMAX_ON_LEFT)
	{
		if(ndx > tp->begin_ndx) return;
		XmpDoubleSliderScaleSetLowerValue(tp->scale, ndx);
	}
	else
	{
		if(ndx < tp->begin_ndx) return;
		XmpDoubleSliderScaleSetUpperValue(tp->scale, ndx);
	}
	set_end_time(tp, ndx);
	set_time_label(tp->endLabel, tp->times[ndx]);

	tp->loop_id = XtAppAddTimeOut(tp->app_context, tp->loop_delay, arrow_end_loop, data);
	tp->loop_delay = (int)((float)tp->loop_delay / LOOP_DELAY_DECREASE_FACTOR);
	if(tp->loop_delay < LOOP_DELAY_MIN) tp->loop_delay = LOOP_DELAY_MIN;
}



/*ARGSUSED*/
static void arrow_arm_cb( Widget w, XtPointer client_data, XtPointer call_data )
{
	TWWS *tp = (TWWS *)client_data;

	if(tp->ntimes < 1 ) return;

	tp->loop_delay = LOOP_DELAY_BEGIN;

	if(tp->process_dirn == XmMAX_ON_LEFT)
	{
		if(w == tp->arrow_left_right || w == tp->arrow_right_right)
			tp->loop_inc = -1;
		else
			tp->loop_inc = 1;
	}
	else
	{
		if(w == tp->arrow_left_right || w == tp->arrow_right_right)
			tp->loop_inc = 1;
		else
			tp->loop_inc = -1;
	}

	if(w == tp->arrow_left_left || w == tp->arrow_left_right)
		arrow_begin_loop((XtPointer)tp, NULL);
	else
		arrow_end_loop((XtPointer)tp, NULL);
}

/* Fill in and return callback data */
static TimeWindowSelectorStruct *callback_data(TWWS *tp)
{
	tp->data.start_ndx = tp->begin_ndx;
	tp->data.end_ndx   = tp->end_ndx;
	strcpy(tp->data.start_time, tp->times[tp->begin_ndx]);
	strcpy(tp->data.end_time,   tp->times[tp->end_ndx]);
	return &tp->data;
}


/*ARGSUSED*/
static void arrow_disarm_cb( Widget w, XtPointer client_data, XtPointer call_data )
{
	TWWS *tp = (TWWS *)client_data;

	if (tp->loop_id) XtRemoveTimeOut(tp->loop_id);
	tp->loop_id = (XtIntervalId)0;
	if(tp->ntimes > 0) tp->callback(callback_data(tp));
}



static void scale_cb( Widget w, XtPointer client_data, XtPointer call_data )
{
	XmScaleCallbackStruct *rtn = (XmScaleCallbackStruct *)call_data;
	TWWS *tp = (TWWS *)client_data;

	/* There needs to be a defined time array available */
	if(tp->ntimes < 1) return;

	if(tp->process_dirn == XmMAX_ON_LEFT)
	{
		switch(rtn->reason)
		{
			case XmCR_UPPER_DRAG:
				set_time_label(tp->beginLabel, tp->times[rtn->value]);
				break;

			case XmCR_LOWER_DRAG:
				set_time_label(tp->endLabel, tp->times[rtn->value]);
				break;

			case XmCR_LOWER_VALUE_CHANGED:
				set_end_time(tp, rtn->value);
				set_time_label(tp->endLabel, tp->times[tp->end_ndx]);
				XuDelay(w, 30);
				tp->callback(callback_data(tp));
				break;

			case XmCR_UPPER_VALUE_CHANGED:
				set_begin_time(tp, rtn->value);
				set_time_label(tp->beginLabel, tp->times[tp->begin_ndx]);
				XuDelay(w, 30);
				tp->callback(callback_data(tp));
				break;
		}
	}
	else
	{
		switch(rtn->reason)
		{
			case XmCR_LOWER_DRAG:
				set_time_label(tp->beginLabel, tp->times[rtn->value]);
				break;

			case XmCR_UPPER_DRAG:
				set_time_label(tp->endLabel, tp->times[rtn->value]);
				break;

			case XmCR_LOWER_VALUE_CHANGED:
				set_begin_time(tp, rtn->value);
				set_time_label(tp->beginLabel, tp->times[tp->begin_ndx]);
				XuDelay(w, 30);
				tp->callback(callback_data(tp));
				break;

			case XmCR_UPPER_VALUE_CHANGED:
				set_end_time(tp, rtn->value);
				set_time_label(tp->endLabel, tp->times[tp->end_ndx]);
				XuDelay(w, 30);
				tp->callback(callback_data(tp));
				break;
		}
	}
}



/*============ Public Functions =================*/


/*
 *   Create an instance of the widget construct that sets the time within
 *   the limits of some time window defined by a start and end time.
 *
 *   Parameters;
 *
 *   parent               - the object parent widget
 *   processing_direction - either XmMAX_ON_LEFT or XmMAX_ON_RIGHT. This refers
 *                          to the index values used to access the time array
 *                          and not to the time values stored in the array.
 *   cbf(start_ndx, end_ndx, start_time, end_time)      - function called when the user changes the time
 *                          limits. It returns two arguments: the beginning time
 *                          and the end time of the  window.
 *   ...                  - variable argument list used to set widget constraints.
 */
Widget CreateTimeWindowSelector(Widget parent, int processing_direction, void (*cbf)(), ...)
{
	int      ac;
	String   cmd;
	Widget   form;
	Arg      al[30];
	TWWS     *tp;
	va_list  args;
	
	/* Initialize the instance specific data structure */
	tp = OneMem(TWWS);

	tp->app_context  = XtWidgetToApplicationContext(parent);
	tp->callback     = cbf;
	tp->process_dirn = processing_direction;

	/* Set the frame constraints */
	ac = 0;
	XtSetArg(al[ac], XmNshadowType, XmSHADOW_ETCHED_IN); ac++;
	XtSetArg(al[ac], XmNuserData, (XtPointer)tp); ac++;
	va_start(args, cbf);
	while((cmd = va_arg(args, String)))
	{
		XtPointer ptr = va_arg(args, XtPointer);
		XtSetArg(al[ac], cmd, ptr); ac++;
	}
	va_end(args);

	/* Create the surrounding frame */
	tp->timeWindow = XmCreateFrame(parent, "timeWindow", al, ac);
	XtAddCallback(tp->timeWindow, XmNdestroyCallback, being_destroyed_cb, (XtPointer)tp);

	(void)XmVaCreateManagedLabel(tp->timeWindow, "timeWindowLabel",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	form = XmVaCreateManagedForm(tp->timeWindow, "form",
		XmNhorizontalSpacing, 3,
		XmNverticalSpacing, 3,
		NULL);

	tp->beginLabel = XmVaCreateManagedLabel(form, "asl",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	tp->endLabel = XmVaCreateManagedLabel(form, "ael",
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	tp->scale = XmpVaCreateManagedDoubleSliderScale(form, "da",
		XmNprocessingDirection, processing_direction,
		XmNshowValues, False,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, tp->beginLabel,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(tp->scale, XmNlowerDragCallback,         scale_cb, (XtPointer)tp);
	XtAddCallback(tp->scale, XmNupperDragCallback,         scale_cb, (XtPointer)tp);
	XtAddCallback(tp->scale, XmNlowerValueChangedCallback, scale_cb, (XtPointer)tp);
	XtAddCallback(tp->scale, XmNupperValueChangedCallback, scale_cb, (XtPointer)tp);

	tp->arrow_left_left = XmVaCreateManagedArrowButton(form, "all",
		XmNarrowDirection, XmARROW_LEFT,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, tp->scale,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(tp->arrow_left_left, XmNarmCallback,    arrow_arm_cb,    (XtPointer)tp);
	XtAddCallback(tp->arrow_left_left, XmNdisarmCallback, arrow_disarm_cb, (XtPointer)tp);

	tp->arrow_left_right = XmVaCreateManagedArrowButton(form, "alr",
		XmNarrowDirection, XmARROW_RIGHT,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, tp->scale,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, tp->arrow_left_left,
		XmNleftOffset, 0,
		NULL);
	XtAddCallback(tp->arrow_left_right, XmNarmCallback,    arrow_arm_cb,    (XtPointer)tp);
	XtAddCallback(tp->arrow_left_right, XmNdisarmCallback, arrow_disarm_cb, (XtPointer)tp);

	tp->arrow_right_right = XmVaCreateManagedArrowButton(form, "arr",
		XmNarrowDirection, XmARROW_RIGHT,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, tp->scale,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(tp->arrow_right_right, XmNarmCallback,    arrow_arm_cb,    (XtPointer)tp);
	XtAddCallback(tp->arrow_right_right, XmNdisarmCallback, arrow_disarm_cb, (XtPointer)tp);

	tp->arrow_right_left = XmVaCreateManagedArrowButton(form, "arl",
		XmNarrowDirection, XmARROW_LEFT,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, tp->scale,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, tp->arrow_right_right,
		XmNrightOffset, 0,
		NULL);
	XtAddCallback(tp->arrow_right_left, XmNarmCallback,    arrow_arm_cb,    (XtPointer)tp);
	XtAddCallback(tp->arrow_right_left, XmNdisarmCallback, arrow_disarm_cb, (XtPointer)tp);

	set_time_label(tp->beginLabel, NULL);
	set_time_label(tp->endLabel,   NULL);

	XtManageChild(tp->timeWindow);
	return tp->timeWindow;
}


/*
 *   After the above widget has been created, this function sets the time list and
 *   limiting times for the widget. Note that the times array is referred to by
 *   reference. This array must not then be changed while the time window widget
 *   is functioning.
 *
 *   Parameters:
 *
 *   w      - the widget
 *   times  - the array of times
 *   ntimes - number of elements in the array
 *   start  - the start time index of the time array.
 *   end    - the end time index of the time array.
 */
void SetTimeWindowLimits( Widget w, String *times, int ntimes, int lower_value, int upper_value )
{
	XtPointer xtp;
	TWWS      *tp;

	static String Module = "SetTimeWindowLimits";

	XtVaGetValues(w, XmNuserData, &xtp, NULL);
	tp = (TWWS *)xtp;

	if (!tp)
	{
		pr_error(Module,"Widget data structure returned as null\n");
		return;
	}
	if(w != tp->timeWindow)
	{
		pr_error(Module,"Given widget and data structure do not agree\n");
		return;
	}

	if(IsNull(times) || ntimes <= 0)
	{
		FREELIST( tp->times, tp->ntimes);
		tp->ntimes = 0;
		tp->times  = (String *)0;
		set_time_label(tp->beginLabel, NULL);
		set_time_label(tp->endLabel,   NULL);
	}
	else
	{
		FREELIST( tp->times, tp->ntimes);
		tp->ntimes = ntimes;
		tp->times  = strlistdup(ntimes, times);

		XtVaSetValues(tp->scale,
			XmNlowerValue, 0,		/* needed to avoid range complaints */
			XmNupperValue, 1,		/* ditto */
			XmNminimum, 0,
			XmNmaximum, ntimes-1,
			NULL);

		if(lower_value < 0 || lower_value >= ntimes) lower_value = 0;
		if(upper_value < 0 || upper_value >= ntimes) upper_value = ntimes - 1;

		if( lower_value > upper_value )
		{
			int val     = lower_value;
			lower_value = upper_value;
			upper_value = val;
		}

		XmpDoubleSliderScaleSetUpperValue(tp->scale, upper_value);
		XmpDoubleSliderScaleSetLowerValue(tp->scale, lower_value);

		if(tp->process_dirn == XmMAX_ON_LEFT)
		{
			set_begin_time(tp, upper_value);
			set_end_time(tp, lower_value);
		}
		else
		{
			set_begin_time(tp, lower_value);
			set_end_time(tp, upper_value);
		}
	}
}
