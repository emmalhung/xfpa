/*=========================================================================*/
/*
*      File: guidance_listAddItemDialog.c
*
*   Purpose: Dialog for adding to the guidance lists.
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
#include <Xm/TextF.h>
#include "help.h"
#include "guidance.h"

static Widget dialog = NULL;
static Widget textW;
static Boolean copy_active;


/*ARGSUSED*/
static void UpdateListCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	String label = XmTextFieldGetString(textW);
	if(!blank(label))
	{
		if(GuidanceListAddItem(label, copy_active))
			XuDestroyDialog(dialog);
		else
			XuShowMessage(w, "ListExists", label);
	}
	XtFree(label);
}


void ACTIVATE_guidanceListAddItemDialog(Widget refw , Boolean copy )
{

	static XuDialogActionsStruct action_items[] = {
        { "applyBtn", UpdateListCB, NULL },
        { "closeBtn", XuDestroyDialogCB, NULL },
        { "helpBtn", HelpCB, HELP_GUIDANCE_LIST_ADD }
    };

	if(dialog) return;

	copy_active = copy;

	dialog = XuCreateFormDialog(refw, "guidanceListAddItem",
		XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL,
		XmNnoResize, True,
		XmNkeyboardFocusPolicy, XmEXPLICIT,
		XuNdestroyCallback, XuExitOnDestroyCallback,
		XuNdestroyData, &dialog,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XmNhorizontalSpacing, 19,
		XmNverticalSpacing, 19,
		NULL);

	textW = XmVaCreateManagedTextField(dialog, "text",
		xmTextFieldWidgetClass, dialog,
		XmNhighlightOnEnter, True,
		XmNhighlightThickness, 2,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(textW, XmNactivateCallback, UpdateListCB, NULL);

	(void) XmVaCreateManagedLabel(dialog, "textLabel",
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, textW,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, textW,
		XmNrightOffset, 6,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomWidget, textW,
		XmNbottomOffset, 0,
		NULL);

	XuShowDialog(dialog);
}
