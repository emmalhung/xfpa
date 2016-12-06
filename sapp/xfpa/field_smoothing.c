/*****************************************************************************
*
*  File:     field_smoothing.c
*
*  Purpose:  Provides the program interface panel for any field that needs
*            a smoothing control.
*
*  Notes:    The only function that the field panel needs to call is
*            SetSmoothingValue() with a pointer to the memory location of
*            the existing smoothing value for that field. This is normally
*            done on entry to the appropriate field editing panel.
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

#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/Scale.h>
#include "global.h"
#include "editor.h"

/* Local functions */
static void    smoothing_cb (Widget, XtPointer, XtPointer);

/* Local static variables */
static Widget  smoothingPanel = NullWidget;
static Widget  smoothingScale = NullWidget;
static float   max_value      = 5.0;		/* upper scale limit */



void CreateSmoothingFieldPanel(Widget parent, Widget topAttach )
{
	char   buf[20];
	Widget title;
	PARM   *parm;

	smoothingPanel = XmVaCreateForm(parent, "smoothingPanel",
		XmNhorizontalSpacing, 10,
		XmNverticalSpacing, 10,
		XmNborderWidth, 0,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNbottomOffset, 3,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 3,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 3,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, topAttach,
		XmNtopOffset, 3,
		NULL);

	title = XmVaCreateManagedLabel(smoothingPanel, "smoothingScaleTitle",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);
	/*
	 * Get max_value from setup file. There is only one entry in the line
	 * that specifies the smoothing factor so nothing fancy here.
	 */
	parm = GetSetupParms(FIELD_SMOOTHING);
	if(parm != NULL && parm->nparms > 0)
	{
		float val = atof(parm->parm[0]);
		if(val > 0) max_value = val;
	}

	smoothingScale = XmVaCreateManagedScale(smoothingPanel, "smoothingScale",
		XmNorientation, XmHORIZONTAL,
		XmNminimum, 10,
		XmNmaximum, (int) (max_value * 10.0),
		XmNscaleMultiple, 5,
		XmNborderWidth, 0,
		XmNdecimalPoints, 1,
		XmNshowValue, XmNEAR_SLIDER,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, title,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	XtAddCallback(smoothingScale, XmNvalueChangedCallback, smoothing_cb, NULL);

	snprintf(buf, sizeof(buf), "%.1f", max_value);
	(void) XmVaCreateManagedLabel(smoothingScale, "1.0", NULL);
	(void) XmVaCreateManagedLabel(smoothingScale, buf, NULL);
}


void ShowSmoothingPanel(void)
{
	if (smoothingPanel != NullWidget && !XtIsManaged(smoothingPanel))
		XtManageChild(smoothingPanel);
}


void HideSmoothingPanel(void)
{
	if (smoothingPanel != NullWidget && XtIsManaged(smoothingPanel))
		XtUnmanageChild(smoothingPanel);
}


/* Call to set the scale values. Note that the value input is the address
 * of the value so that it can be modified by the scale callback. The
 * userData scale resource is used to store the address.
 */
void SetSmoothingValue(float *value)
{
	if(smoothingPanel == NullWidget) return;

	if(*value < 1.0 || *value > max_value)
		*value = 1.0;

	XtVaSetValues(smoothingScale,
		XmNvalue, (int)((*value)*10),
		XmNuserData, (XtPointer) value,
		NULL);

	(void) IngredVaCommand(GE_ACTION, "STATE SMOOTHING_AMOUNT %.1f", *value);	
}


/* Transmits a change in smoothing amount to Ingred and changes the value
*   associated with the value address passed in above.
*/
/*ARGSUSED*/
static void smoothing_cb(Widget  w , XtPointer client_data , XtPointer call_data )
{
	float *value;
	XtPointer rtn;
	
	XtVaGetValues(smoothingScale, XmNuserData, &rtn, NULL);
	value = (float*) rtn;
	*value = ((float)((XmScaleCallbackStruct*) call_data)->value)/10.0;
	(void) IngredVaCommand(GE_ACTION, "STATE SMOOTHING_AMOUNT %.1f", *value);
}
