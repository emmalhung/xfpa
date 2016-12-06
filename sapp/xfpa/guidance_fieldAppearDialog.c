/*=========================================================================*/
/*
*	  File: guidance_fieldAppearDialog.c
*
*   Purpose: Dialog for adding fields to the active guidance list.
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
#include <Xm/List.h>
#include <Xm/Frame.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <Xbae/Matrix.h>
#include "help.h"
#include "guidance.h"
#include "observer.h"
#include "selector.h"
#include "resourceDefines.h"


static void appearance_cb	  (ColorSelectorStruct*);
static void exit_cb		      (Widget, XtPointer, XtPointer);
static void label_cb		  (Widget, XtPointer, XtPointer);
static void field_list_cb	  (Widget, XtPointer, XtPointer);
static void fill_selector     (void);
static void restore_cb	      (Widget, XtPointer, XtPointer);
static void send_edit_command (int, String);
static void set_field         (int);

enum {DO_LABEL, DO_CLEAR, DO_RESTORE, DO_COLOUR, DO_STYLE};

static Widget dialog = NULL;
static Widget fieldList, contour, barb;
static GuidanceFieldStruct *fld = NULL;
static String label_type = "VALUE";
static Boolean wind_available = False;
static int nselected_fields = 0;


void ACTIVATE_guidanceFieldAppearanceDialog(Widget refw )
{
	Widget   frame, rc, btn, csel;
	XmString header[3];
	String   ob_parms[] = {"G","on"};

	static XuDialogActionsStruct action_items[] = {
		{ "closeBtn", XuDestroyDialogCB, NULL },
		{ "helpBtn", HelpCB, HELP_GUIDANCE_FIELD_APPEAR }
	};

	if(dialog) return;

	dialog = XuCreateFormDialog(refw, "guidanceFieldAppearance",
		XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL,
		XuNdestroyCallback, exit_cb,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XmNhorizontalSpacing, 19,
		XmNverticalSpacing, 19,
		NULL);

	csel = CreateColorSelector(dialog, GUIDANCE, False, appearance_cb,
	    XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
	    NULL);


/*XXXXXXXXXXXXXX Not active for now XXXXXXXXXXXXXXXXX*/
	frame = XmVaCreateManagedFrame(dialog, "frame",
		XmNmappedWhenManaged, False,		/* XXXXXXXXXXX */
		XmNshadowType, XmSHADOW_ETCHED_IN,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, csel,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, csel,
		NULL);

	(void) XmVaCreateManagedLabel(frame, "labelStyle",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	rc = XmVaCreateManagedRowColumn(frame, "rc",
		XmNradioBehavior, True,
		NULL);

	contour = XmVaCreateManagedToggleButton(rc, "labelContour", NULL);
	XtAddCallback(contour, XmNvalueChangedCallback, label_cb, NULL);

	barb = XmVaCreateManagedToggleButton(rc, "labelBarb", NULL);

	btn = XmVaCreateManagedPushButton(dialog, "restore",
		XmNmarginHeight, 6,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, csel,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, csel,
		NULL);
	XtAddCallback(btn, XmNactivateCallback, restore_cb, NULL);

	header[0] = XuGetXmStringResource(RNfieldHeader, "Field Name");
	header[1] = XuGetXmStringResource(RNsourceHeader, "Source");
	header[2] = XuGetXmStringResource(RNissueHeader, "Issue Time");

	fieldList = CreateXbaeMatrix(dialog, "fieldList",
		XmNrows, 1,
		XmNvisibleRows, 6,
		XmNcolumns, 3,
		XmNxmColumnLabels, header,
		XmNallowRowResize, False,
		XmNallowColumnResize, False,
		XmNshadowThickness, 2,
		XmNcellShadowThickness, 1,
		XmNgridType, XmGRID_ROW_SHADOW,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, csel,
		NULL);
	
	XtAddCallback(fieldList, XmNselectCellCallback, field_list_cb, NULL); 

	XmStringFree(header[0]);
	XmStringFree(header[1]);
	XmStringFree(header[2]);

	fill_selector();

	XuShowDialog(dialog);
	NotifyObservers(OB_DIALOG_SAMPLING, ob_parms, 2);
	SetGuidanceDialogSensitivity(OFF);
	XbaeMatrixSelectRow(fieldList, 0);
	set_field(0);
	SelectedFieldInfo(NULL, True, NULL, &nselected_fields);
}


/* Function called from the guidanceDialog whenever any action occurs
*  which may impact on this dialog if it is active.
*/
void UpdateAppearanceDialog(void)
{
	int n, active;

	if (!dialog) return;

	SelectedFieldInfo(fld, True, &active, &n);

	if(n < 1)
	{
		nselected_fields = 0;
		XuDestroyDialog(dialog);
	}
	else if(n == nselected_fields)
	{
		send_edit_command(DO_LABEL, NULL);
	}
	else
	{
		nselected_fields = n;
		fill_selector();
		XbaeMatrixSelectRow(fieldList, active);
		set_field(active);
	}
}


/************************** LOCAL FUNCTIONS *********************************/


static void fill_selector(void)
{
	int    i, j;
	int    nrows;
	String cell_data[3];
	char   mbuf[256];

	nrows = XbaeMatrixNumRows(fieldList);
	XbaeMatrixDeleteRows(fieldList, 0, nrows);

	for(nrows = 0, i = 0; i < GVG_nguidlist; i++)
	{
		for(j = 0; j < GVG_guidlist[i]->nfield; j++)
		{
			if(!GVG_guidlist[i]->field[j]->showing) continue;

			if(GVG_option_full_display)
				cell_data[0] = GVG_guidlist[i]->field[j]->info->label;
			else
				cell_data[0] = GVG_guidlist[i]->field[j]->info->sh_label;
			cell_data[1] = SrcShortLabel(GVG_guidlist[i]->field[j]->source);
			switch(GVG_guidlist[i]->field[j]->rtype)
			{
				case GUID_CURRENT:
					(void)strcpy(mbuf, XuGetLabel(CURRENT_STRING));
					break;
				case GUID_PREVIOUS:
					(void)strcpy(mbuf, XuGetLabel(PREVIOUS_STRING));
					break;
				case GUID_ABSOLUTE:
					(void)strcpy(mbuf, DateString(GVG_guidlist[i]->field[j]->run->time,HOURS));
					break;
				default:
					(void)strcpy(mbuf, XuGetLabel("na-short"));
					break;
			}
			cell_data[2] = mbuf;
			XbaeMatrixAddRows(fieldList, nrows, cell_data, NULL, NULL, 1);
			nrows++;
		}
	}

	if(nrows < 1)
		XbaeMatrixAddRows(fieldList, 0, NULL, NULL, NULL, 1);

	XbaeMatrixResizeColumnsToCells(fieldList, True);
}


static void set_field( int list_ndx )
{
	int i, j, n;

	list_ndx++;
	n = 0;
	for(i = 0; i < GVG_nguidlist; i++)
	{
		for(j = 0; j < GVG_guidlist[i]->nfield; j++)
		{
			fld = GVG_guidlist[i]->field[j];
			if(!fld->showing) continue;
			n++;
			if(list_ndx != n) continue;
			wind_available =
				NotNull(fld->info->element->elem_detail) ?
					(fld->info->element->elem_detail->wd_class != FpaC_NOWIND) :
					False;
			XtSetSensitive(barb, wind_available);
			XuToggleButtonSet(contour, True, False);
			XuToggleButtonSet(barb, False, False);
			if(wind_available && same(label_type,"WIND"))
			{
				XuToggleButtonSet(barb, True, False);
				XuToggleButtonSet(contour, False, False);
			}
			send_edit_command(DO_LABEL, NULL);
			break;
		}
		if(list_ndx == n) break;
	}

}


/*ARGSUSED*/
static void field_list_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i, nrows;
	XbaeMatrixSelectCellCallbackStruct *rtn = (XbaeMatrixSelectCellCallbackStruct *)call_data;

	if( rtn->row < 0 || rtn->column < 0 || rtn->row >= nselected_fields ) return;

	XtVaGetValues(w, XmNrows, &nrows, NULL);
	for( i = 0; i < nrows; i++ )
		XbaeMatrixDeselectRow(w, i);
	XbaeMatrixSelectRow(w, rtn->row);
	set_field(rtn->row);
}


/*ARGSUSED*/
static void label_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	label_type = "VALUE";
	if(XmToggleButtonGetState(barb)) label_type = "WIND";
	send_edit_command(DO_LABEL, NULL);
}


static void send_edit_command(int command , String value )
{
	char cmd[128];

	cmd[0] = '\0';
	switch(command)
	{
		case DO_CLEAR:     
			(void) strcpy(cmd, "CLEAR");   
			break;

		case DO_RESTORE:
			(void) strcpy(cmd, "RESTORE"); 
			fld->legend_colour = fld->legend_colour_default; 
			break;

		case DO_COLOUR:
			(void) snprintf(cmd, sizeof(cmd), "COLOR %s", value); 
		    fld->legend_colour = glSetColor(value); 
			break;

		case DO_STYLE:
			(void) snprintf(cmd, sizeof(cmd), "STYLE %s", value);
			break;
	}
	if(cmd[0])
		(void) IngredVaCommand(GE_GUIDANCE, "EDIT FIELD %s %s", fld->id, cmd);

	strcpy(cmd, wind_available ? label_type:"VALUE");
	(void) IngredVaCommand(GE_GUIDANCE, "LABEL ADD %s %s", fld->id, cmd);
	ACTIVATE_guidanceLegendDialog(dialog);
}


/*ARGSUSED*/
static void appearance_cb(ColorSelectorStruct *cbs )
{
    switch(cbs->reason)
    {
        case SELECT_COLOUR: send_edit_command(DO_COLOUR, cbs->colour); break;
        case SELECT_STYLE:  send_edit_command(DO_STYLE, cbs->style);   break;
    }
}


/*ARGSUSED*/
static void restore_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	send_edit_command(DO_RESTORE, NULL);
}


/*ARGSUSED*/
static void exit_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	String ob_parms[] = {"G","off"};
	dialog = NULL;
	NotifyObservers(OB_DIALOG_SAMPLING, ob_parms, 2);
	SetGuidanceDialogSensitivity(ON);
}
