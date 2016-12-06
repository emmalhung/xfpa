/*=========================================================================*/
/*
*      File: guidance_utilities.c
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

#include "global.h"
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/RowColumn.h>
#include <Xbae/Matrix.h>
#include "resourceDefines.h"
#include "guidance.h"


String GuidFieldDateFormat( GuidanceFieldStruct *fld, String dt )
{
	if( fld->info->element->elem_tdep->time_dep == FpaC_DAILY )
		return DateString(dt,DAYS);

	if ( fld->source->fd->sdef->minutes_rqd )
		return DateString(dt,MINUTES);

	return DateString(dt,HOURS);
}


void SelectedFieldInfo(GuidanceFieldStruct *fld, Boolean selected_only, int *active, int *nselected)
{
	int i, j, count, act;
	Boolean sel;

	act = 0;
	count = 0;
	for(i = 0; i < GVG_nguidlist; i++)
	{
		sel = (selected_only || GVG_guidlist[i] != GVG_active_guidlist);
		for(j = 0; j < GVG_guidlist[i]->nfield; j++)
		{
			if(sel && !GVG_guidlist[i]->field[j]->showing) continue;
			if(NotNull(fld) && fld == GVG_guidlist[i]->field[j]) act = count;
			count++;
		}
	}
	if (active) *active = act;
	if (nselected) *nselected = count;
}


/****************************************************************************/
/*
*   Show the legend dialog.
*/
/****************************************************************************/
/*ARGSUSED*/
void ShowGuidanceLegendCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	int    i, ndx;
	Widget rc1, rc2;

	static struct {
		String type;
		String resource;
		String colour;
		String default_colour;
		String label;
	} info[] = {
		{"fieldHeader", XmNbackground, RNguidanceFieldMissing, "IndianRed", "field2"},
		{"issueHeader", XmNforeground, RNguidanceRunOk,        "Green",     "issue1"},
		{"issueHeader", XmNforeground, RNguidanceRunCaution,   "Yellow",    "issue2"},
		{"issueHeader", XmNforeground, RNguidanceRunNA,        "Red",       "issue3"},
		{"validHeader", XmNforeground, RNguidanceRunCaution,   "Yellow",    "valid2"},
	};

	static Widget legendDialog;
	static XuDialogActionsStruct action_items[] = {
		{"closeBtn",  XuDestroyDialogCB, NULL }
	};

	if (legendDialog) return;

	ndx = -1;
	if (client_data) ndx = 4;

	legendDialog = XuCreateFormDialog(w, "guidSelectLegend",
		XmNdialogStyle, XmDIALOG_MODELESS,
		XmNnoResize, True,
		XuNdestroyCallback, XuExitOnDestroyCallback,
		XuNdestroyData, &legendDialog,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XmNhorizontalSpacing, 14,
		XmNverticalSpacing, 14,
		NULL);

	rc1 = XmVaCreateRowColumn(legendDialog, "rc1",
		XmNspacing, 11,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	rc2 = XmVaCreateRowColumn(legendDialog, "rc2",
		XmNspacing, 11,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, rc1,
		XmNleftOffset, 0,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	for(i = 0; i < XtNumber(info); i++)
	{
		if(ndx >= 0 && i != ndx) continue;

		w = XmVaCreateManagedLabel(rc1, info[i].type,
			info[i].resource,  XuLoadColorResource(legendDialog, info[i].colour, info[i].default_colour),
			NULL);
		(void) XmVaCreateManagedLabel(rc2, info[i].label, NULL);
	}
	XtManageChild(rc1);
	XtManageChild(rc2);
	XuShowDialog(legendDialog);
}
