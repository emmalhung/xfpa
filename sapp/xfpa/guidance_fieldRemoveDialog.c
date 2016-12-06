/*=========================================================================*/
/*
*      File: guidance_fieldRemoveDialog.c
*
*   Purpose: Dialog for removing fields from the active list.
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
#include <Xbae/Matrix.h>
#include "resourceDefines.h"
#include "help.h"
#include "guidance.h"


static Widget dialog = NULL;
static Widget fieldList;
static int    list_len = 0;

/*ARGSUSED*/
static void field_list_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	XbaeMatrixSelectCellCallbackStruct *rtn = (XbaeMatrixSelectCellCallbackStruct *)call_data;

	if( rtn->row < 0 || rtn->column < 0 || rtn->row >= list_len ) return;

	if(XbaeMatrixIsRowSelected(w, rtn->row))
		XbaeMatrixDeselectRow(w, rtn->row);
	else
		XbaeMatrixSelectRow(w, rtn->row);
}


/*ARGSUSED*/
static void remove_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i, nrows, *sl, nsl;

	XtVaGetValues(fieldList, XmNrows, &nrows, NULL);
	sl = NewIntArray(nrows);
	for( nsl = 0, i = 0; i < MIN(list_len, nrows); i++ )
	{
		if(XbaeMatrixIsRowSelected(fieldList, i))
		{
			sl[nsl] = i;
			nsl++;
		}
	}
	GuidanceListRemoveFields(sl, nsl);
	FreeItem(sl);
	XuDestroyDialog(dialog);
	dialog = NullWidget;
}



void ACTIVATE_guidanceFieldRemoveDialog(Widget refw )
{
	int        i, j;
	String     cell_data[3];
	char       mbuf[256];
	XmString   header[3];
	String     title;
	XmString   label;

	static XuDialogActionsStruct action_items[] = {
        { "removeBtn", remove_cb, NULL },
        { "closeBtn", XuDestroyDialogCB, NULL        },
        { "helpBtn", HelpCB, HELP_GUIDANCE_FIELD_DEL }
    };

	if(dialog) return;

	dialog = XuCreateFormDialog(refw, "guidanceFieldRemove",
		XuNdestroyCallback, XuExitOnDestroyCallback,
		XuNdestroyData, &dialog,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XmNhorizontalSpacing, 19,
		XmNverticalSpacing, 19,
		NULL);

	/* Put the list label into the dialog title.
	*/
	title = XuGetStringResource(RNguidanceFieldRemoveDialogTitle, NULL);
	label = XuNewXmStringFmt(title, GVG_active_guidlist->label);
	XtVaSetValues(XtParent(dialog), XmNdialogTitle, label, NULL);
	XmStringFree(label);

	header[0] = XuGetXmStringResource(RNfieldHeader, "Field Name");
	header[1] = XuGetXmStringResource(RNsourceHeader, "Source");
	header[2] = XuGetXmStringResource(RNissueHeader, "Issue Time");

	fieldList = CreateXbaeMatrix(dialog, "fieldList",
		XmNrows, 0,
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
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	
	XtAddCallback(fieldList, XmNselectCellCallback, field_list_cb, NULL); 

	XmStringFree(header[0]);
	XmStringFree(header[1]);
	XmStringFree(header[2]);

	for(list_len = 0, i = 0; i < GVG_nguidlist; i++)
	{
		if(GVG_guidlist[i] != GVG_active_guidlist) continue;
		for(j = 0; j < GVG_guidlist[i]->nfield; j++)
		{
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
			XbaeMatrixAddRows(fieldList, list_len, cell_data, NULL, NULL, 1);
			list_len++;
		}
	}
	if(list_len < 1) XbaeMatrixAddRows(fieldList, 0, NULL, NULL, NULL, 1);
	XbaeMatrixResizeColumnsToCells(fieldList, True);
	XuShowDialog(dialog);
}
