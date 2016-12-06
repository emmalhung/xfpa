/*=========================================================================*/
/*
 *      File: guidance_legendDialog.c
 *
 *   Purpose: Provides a window separate from the map area in which the
 *            currently active guidance fields are listed. Note that once
 *            activated this dialog always stays around unless the user
 *            takes action to remove it.
 *
 *      Note: It turned out to be easier to put the actual dialog in a
 *            time delay loop to in case of mulitple quick calls to this
 *            function than to try and put the function into the code in
 *            all the places where it might be accessed only once. Doing
 *            it this way ensures that the dialog activates only once if
 *            multiple calls are made within the time window.
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

#include <string.h>
#include "global.h"
#include <Xbae/Matrix.h>
#include "resourceDefines.h"
#include "observer.h"
#include "guidance.h"

/* Our static variables */
static Widget       dialog           = NullWidget;
static Widget       display          = NullWidget;
static Boolean      allow_activation = True;
static XtIntervalId oldId            = (XtIntervalId) NULL;
static int          nrows            = 1;

/* Forward function declaration */
static void activate_legend(XtPointer, XtIntervalId*);


/* Show the dialog, but only move it to the front of the window stack
 * if the dialog does not already exist. This way it will not pop to
 * the front if the user has put it underneath another window. This is
 * important for users with only one monitor.
 */
void ACTIVATE_guidanceLegendDialog(Widget w)
{
	if (allow_activation || !dialog)
		activate_legend( INT2PTR(False), NULL );
}


/* Show the dialog and move it to the front of the window stack even
 * if the dialog already exists.
 */
void ACTIVATE_showGuidanceLegendDialog(Widget w)
{
	if (allow_activation || !dialog)
		activate_legend( INT2PTR(True), NULL );
}


/*  Especially with animation the above activation function above must be
 *  disabled so if it is called by some other code it will not activate.
 *  This function will only activate if the dialog is rendered already and
 *  not put the display up on its own. Return the activation state before
 *  the requested change.
 */
Boolean GuidanceLegendDialogActivationState(Boolean state)
{
	Boolean past_state = allow_activation;
	allow_activation = state;
	return past_state;
}

/* This function changes the valid time only and was written for guidance
 * animation so the the valid time would change as the fields animated but
 * not have the entire dialog "flash" when everything is changed.
 */
void GuidanceLegendDialogSetValidTime( String dt )
{
	int i, n, nr;

	if (!dialog) return;

	for(nr = 0, i = 0; i < GVG_nguidlist; i++)
	{
		for( n = 0; n < GVG_guidlist[i]->nfield; n++)
		{
			GuidanceFieldStruct *fld = GVG_guidlist[i]->field[n];
			if(fld->show)
			{
				String rt = (fld->rtype == GUID_DEPICT)? GV_T0_depict:fld->run->time;
				XbaeMatrixSetCell(display, nr, 4, TimeDiffFormat(rt, dt, SHOW_MINUTES(fld)));
				nr++;
			}
		}
	}
}


/*================= Private functions ======================*/


static void exit_cb( Widget w, XtPointer unused, XtPointer notused)
{
	XuDestroyDialog(dialog);
	dialog = NullWidget;
}



/* This is the callback for the XtAppAddTimeOut function and will add itself
 * to the timeout loop if the id is input as null.
 */
static void activate_legend(XtPointer client_data, XtIntervalId *id)
{
	int      i, n, nr;
	String   rt, element, level;
	char     rtime[50];
	char     vtime[50];
	Pixel    colours[1];
	Boolean  show_dialog;
	GuidanceFieldStruct *fld;

	/* This will prevent the dialog updating if there are several calls
	 * in quick succession. Things look messy if this happens.
	 */
	if (!id)
	{
		if (oldId) XtRemoveTimeOut(oldId);
		oldId = XtAppAddTimeOut(GV_app_context, 250, activate_legend, client_data);
		return;
	}
	oldId = (XtIntervalId) NULL;

	/* The show_dialog flag means that the XuShowDialog() function will only be
	 * be called if the dialog is not already realized or if the flag forcing a
	 * display is set. This will prevent it from being popped to the front on a
	 * refresh if the user has put it away unless specifically requested to do so.
	 */
	show_dialog = (dialog == NullWidget || PTR2BOOL(client_data));
	if(!show_dialog && !dialog) return;

	if(!dialog)
	{
		dialog = XuCreateToplevelFormDialog(GW_mainWindow, "guidanceLegendDialog",
			XuNretainGeometry, XuRETAIN_ALL,
			XuNallowIconify, True,
			XuNmwmDeleteOverride, exit_cb,
			XmNdialogStyle, XmDIALOG_MODELESS,
			XmNminHeight, 100,
			XmNminWidth, 100,
			XmNverticalSpacing, 9,
			XmNhorizontalSpacing, 9,
			NULL);

		display = CreateXbaeMatrix(dialog, "display",
			XmNrows, nrows,
			XmNcolumns, 5,
			XmNvisibleRows, 4,
			XmNallowRowResize, False,
			XmNallowColumnResize, False,
			XmNshadowThickness, 2,
			XmNcellShadowThickness, 1,
			XmNgridType, XmGRID_ROW_SHADOW,
			XmNtopAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			XmNrightOffset, 0,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);

		XuShowDialog(dialog);
	}
	else if (show_dialog)
	{
		XuShowDialog(dialog);
	}

	/* Determine how many fields are showing */
	for(nr = 0, i = 0; i < GVG_nguidlist; i++)
		for( n = 0; n < GVG_guidlist[i]->nfield; n++)
			if(GVG_guidlist[i]->field[n]->show) nr++;

	XbaeMatrixDisableRedisplay(display);

	if(!GetGuidanceDisplayState())
	{
		/* The guidance fields are hidden so display a row with "hidden" message */
		XbaeMatrixDeleteRows(display, 1, nrows - 1);
		nrows = 1;
		XbaeMatrixSetCell(display, 0, 0, XuGetLabel("fields"));
		XbaeMatrixSetCell(display, 0, 1, XuGetLabel("hidden"));
		for(i = 2; i < 5; i++)
			XbaeMatrixSetCell(display, 0, i, " ");
		colours[0] = XuLoadColor(dialog, "black");
		XbaeMatrixSetRowColors(display, nr, colours, 1);
	}
	else if(nr == 0)
	{
		/* No fields showing so display a single row of dashes */
		XbaeMatrixDeleteRows(display, 1, nrows - 1);
		nrows = 1;
		for(i = 0; i < 5; i++)
			XbaeMatrixSetCell(display, 0, i, "---");
		colours[0] = XuLoadColor(dialog, "black");
		XbaeMatrixSetRowColors(display, nr, colours, 1);
	}
	else
	{
		/* Display the actual field information */
		if(nr > nrows)
			XbaeMatrixAddRows(display, nrows, NULL, NULL, NULL, nr - nrows);
		else if(nr < nrows)
			XbaeMatrixDeleteRows(display, nr, nrows - nr);
		nrows = nr;

		for(nr = 0, i = 0; i < GVG_nguidlist; i++)
		{
			for( n = 0; n < GVG_guidlist[i]->nfield; n++)
			{
				fld = GVG_guidlist[i]->field[n];
				if(!fld->show) continue;
				if(GVG_option_full_display)
				{
					level   = fld->info->level->label;
					element = fld->info->element->label;
				}
				else
				{
					level   = fld->info->level->sh_label;
					element = fld->info->element->sh_label;
				}

				rt = (fld->rtype == GUID_DEPICT)? "-":fld->run->time;
				(void) safe_strcpy(rtime, DateString(rt, DEFAULT_FORMAT));

				if(fld->vsel >= 0)
				{
					rt = (fld->rtype == GUID_DEPICT)? GV_T0_depict:fld->run->time;
					(void) safe_strcpy(vtime, TimeDiffFormat(rt, fld->valid->times[fld->vsel], SHOW_MINUTES(fld)));
				}
				else
				{
					(void) strcpy(vtime,"--");
				}

				XbaeMatrixSetCell(display, nr, 0, level);
				XbaeMatrixSetCell(display, nr, 1, element);
				XbaeMatrixSetCell(display, nr, 2, SrcShortLabel(fld->source));
				XbaeMatrixSetCell(display, nr, 3, rtime);
				XbaeMatrixSetCell(display, nr, 4, vtime);

				/* Obtain the field display colour */
				colours[0] = glGetPixelFromColorIndex(fld->legend_colour);
				XbaeMatrixSetRowColors(display, nr, colours, 1);
				nr++;
			}
		}
	}
	XbaeMatrixEnableRedisplay(display, True);
	XbaeMatrixResizeColumnsToCells(display, True);
}
