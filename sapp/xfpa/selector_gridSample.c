/*****************************************************************************
*
*  File:     selector_gridSample.c
*
*  Purpose:  Provides the program interface panel for field sampling.
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
*****************************************************************************/

#include <stdarg.h>
#include "global.h"
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/XmpSpinBox.h>
#include <Xm/ToggleB.h>
#include <ingred.h>
#include "editor.h"
#include "guidance.h"
#include "selector.h"

typedef struct { Widget sync; int x; int y; } LASTVAL;

static void SpinCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	XtPointer ptr;
	LASTVAL   *last;
	XmpSpinBoxCallbackStruct *rtn = (XmpSpinBoxCallbackStruct *)call_data;

	XtVaGetValues(w, XmNuserData, &ptr, NULL);
	last = (LASTVAL*)ptr;

	if(XmToggleButtonGetState(last->sync))
	{
		XmpSpinBoxSetValue((Widget)client_data, rtn->value, False);
	}
}


/* The grid command is only issued when the button is pressed not when
*  the spacing is being adjusted.
*/
/*ARGSUSED*/
static void GridSampleCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	int         xval, yval;
	char        mbuf[128];
	SELECT_TYPE type;
	LASTVAL     *last;
	XtPointer   ptr;
	
	XtVaGetValues(w, XmNuserData, &ptr, NULL);
	last = (LASTVAL*)ptr;

	type = (SELECT_TYPE)client_data;

	xval = XmpSpinBoxGetValue(XtNameToWidget(XtParent(w), "*.gridX"));
	yval = XmpSpinBoxGetValue(XtNameToWidget(XtParent(w), "*.gridY"));

	if(xval != last->x || yval != last->y)
	{
		String key = NULL;
		last->x = xval;
		last->y = yval;
		switch(type)
		{
			case SELECT_EDIT_SAMPLE:  key = "e"; break;
			case SELECT_GUID_SAMPLE:  key = "g"; break;
			case SELECT_IMAGE_SAMPLE: key = "i"; break;
		}
		XuVaStateDataSave("grid", "gd", key, "%d %d", xval, yval, NULL);
	}

	(void) snprintf(mbuf, sizeof(mbuf), "GRID %d %d", xval, yval);

	switch(type)
	{
		case SELECT_EDIT_SAMPLE:  SendEditSampleCommand(mbuf);     break;
		case SELECT_GUID_SAMPLE:  SendGuidanceSampleCommand(mbuf); break;
		case SELECT_IMAGE_SAMPLE: SendImageSampleCommand(mbuf);    break;
	}
}


/*ARGSUSED*/
static void BeingDestroyedCB(Widget w, XtPointer client_data, XtPointer unused)
{
	FreeItem(client_data);
}



/*=======================================================================*/
/*
*	CreateGridSelector() -  Create the sampling panel and all the
*	sub-panels.
*/
/*=======================================================================*/
Widget CreateGridSelector(Widget parent, SELECT_TYPE type, ...)
{
	int     xval, yval, ac;
	String  cmd, key = NULL;
	Arg     al[20];
	Widget  grid, w, label, form, gridX, gridY;
	LASTVAL *last;
	va_list args;

	ac = 0;
    XtSetArg(al[ac], XmNshadowType, XmSHADOW_ETCHED_IN); ac++;

    va_start(args, type);
    while((cmd = va_arg(args, String)) != (String)NULL && ac < 20)
    {
        XtSetArg(al[ac], cmd, va_arg(args, XtPointer)); (ac)++;
    }
    va_end(args);
		
	last = OneMem(LASTVAL);

    grid = XmCreateFrame(parent, "sampleGrid", al, ac);
	XtAddCallback(grid, XmNdestroyCallback, BeingDestroyedCB, (XtPointer)last);

	(void) XmVaCreateManagedLabel(grid, "gridHeader",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	form = XmVaCreateManagedForm(grid, "gridForm",
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		NULL);

	gridY = XmpVaCreateManagedSpinBox(form, "gridY",
		XmNvalue, 10,
		XmNcolumns, 2,
		XmNminimum, 2,
		XmNmaximum, 50,
		XmNspinBoxUseClosestValue, True,
		XmNuserData, (XtPointer)last,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	label = XmVaCreateManagedLabel(form, "Y",
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, gridY,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, gridY,
		XmNrightOffset, 3,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomWidget, gridY,
		NULL);

	gridX = XmpVaCreateManagedSpinBox(form, "gridX",
		XmNvalue, 10,
		XmNcolumns, 2,
		XmNminimum, 2,
		XmNmaximum, 50,
		XmNspinBoxUseClosestValue, True,
		XmNuserData, (XtPointer)last,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, label,
		XmNrightOffset, 10,
		NULL);

	XtAddCallback(gridX, XmNvalueChangedCallback, SpinCB, (XtPointer)gridY);
	XtAddCallback(gridY, XmNvalueChangedCallback, SpinCB, (XtPointer)gridX);

	label = XmVaCreateManagedLabel(form, "X",
		XmNalignment, XmALIGNMENT_END,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, gridX,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, gridX,
		XmNrightOffset, 3,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomWidget, gridX,
		NULL);

	last->sync = XmVaCreateManagedToggleButton(form, "matchBtn",
		XmNborderWidth, 1,
		XmNmarginWidth, 3,
		XmNset, XmSET,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, gridY,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	w = XmVaCreateManagedPushButton(form, "gridBtn",
		XmNuserData, (XtPointer)last,
		XmNmarginWidth, 3,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, gridY,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, last->sync,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(w, XmNactivateCallback, GridSampleCB, (XtPointer)type);

	switch(type)
	{
		case SELECT_EDIT_SAMPLE:  key = "e"; break;
		case SELECT_GUID_SAMPLE:  key = "g"; break;
		case SELECT_IMAGE_SAMPLE: key = "i"; break;
	}
	if(XuVaStateDataGet("grid", "gd", key, "%d %d", &xval, &yval))
	{
		XmpSpinBoxSetValue(gridX, xval, False);
		XmpSpinBoxSetValue(gridY, yval, False);
	}

	XtManageChild(grid);
	return grid;
}
