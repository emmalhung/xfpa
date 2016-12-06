/*========================================================================*/
/*
*   File:	  load_fieldsDialog.c
*
*   Purpose:  Provides the management mechanism for the field loading
*             selections.
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
/*========================================================================*/

#include "global.h"
#include <Xm/TabStack.h>
#include "resourceDefines.h"
#include "depiction.h"
#include "editor.h"
#include "help.h"
#define  LOAD_FIELD_MAIN
#include "loadFields.h"


/* Local functions and variables
 */
static void tabs_cb       (Widget, XtPointer, XtPointer);
static void load_cb       (Widget, XtPointer, XtPointer);
static void local_help_cb (Widget, XtPointer, XtPointer);
static void exit_cb       (Widget, XtPointer, XtPointer);

static Widget    dialog       = NullWidget;
static Widget    loadBtn      = NullWidget;
static Widget    pages[3];
static int       page_number  = 0;
static XmString  btn_label[3];
static Boolean   action_taken = False;


void ACTIVATE_loadFieldsDialog(Widget parent )
{
	int    n;
	Widget tabs;

	static XuDialogActionsStruct action_items[] = {
		{ "loadBtn",  load_cb,           NULL },
		{ "closeBtn", XuDestroyDialogCB, NULL },
		{ "helpBtn",  local_help_cb,     NULL }
	};

	if(dialog) return;

	action_taken = False;
	set_locale_from_environment(LC_TIME);

	for(n = 0; n < 3; n++)
		btn_label[n] = XuGetXmStringResource(RNloadBtnLabel,"Load");

    dialog = XuCreateFormDialog( parent, "loadFields",
		XmNresizePolicy, XmRESIZE_NONE,
		XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
		XuNdestroyCallback, exit_cb,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		NULL);

	loadBtn = XtNameToWidget(XtParent(dialog), "*.loadBtn");

	tabs = XmVaCreateTabStack(dialog, "tabs",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(tabs, XmNtabSelectedCallback, tabs_cb, NULL);

	/* We do not care about error messages when creating the available lists!
    */
    (void) check_value_function_error_messages(False);
    (void) check_wind_function_error_messages(False);

	pages[0] = CreateLoadHourlyFields (tabs);
	pages[1] = CreateLoadDailyFields  (tabs);
	pages[2] = CreateLoadStaticFields (tabs);

    (void) check_value_function_error_messages(True);
    (void) check_wind_function_error_messages(True);

	XtManageChild(tabs);
	XuShowDialog(dialog);

	/* We must activate these after the dialog has been managed
	*  and shown so as to avoid layout problems associated with the list
	*  widget which leads to 10,000 iteration problems and crashes.
	*/
	InitLoadHourlyFields();
	InitLoadDailyFields();
	InitLoadStaticFields();
}


void SetLoadBtnLabel(int fld, String label)
{
	XmStringFree(btn_label[fld]);
	btn_label[fld] = XuNewXmString(label);
	XtVaSetValues(loadBtn, XmNlabelString, btn_label[page_number], NULL);
}


/*ARGSUSED*/
static void tabs_cb( Widget w, XtPointer client_data, XtPointer call_data)
{
	int i;
	XmTabStackCallbackStruct *rtn = (XmTabStackCallbackStruct *)call_data;

	for(i = 0; i < 3; i++)
	{
		if(rtn->selected_child != pages[i]) continue;
		page_number = i;
		XtVaSetValues(loadBtn, XmNlabelString, btn_label[page_number], NULL);
		break;
	}
}


/*ARGSUSED*/
static void load_cb( Widget w, XtPointer client_data, XtPointer call_data)
{
	XuSetBusyCursor(ON);
	DeactivateMenu();
	switch(page_number)
	{
		case 0: ImportHourlyFields(w); break;
		case 1: ImportDailyFields(w);  break;
		case 2: ImportStaticFields(w); break;
	}
	ActivateMenu();
	XuSetBusyCursor(OFF);
	action_taken = True;
}


/*ARGSUSED*/
static void local_help_cb( Widget w, XtPointer client_data, XtPointer call_data)
{
	switch(page_number)
	{
		case 0: Help(HELP_COPY_HOURLY_FIELD); break;
		case 1: Help(HELP_COPY_DAILY       ); break;
		case 2: Help(HELP_COPY_STATIC      ); break;
	}
}


/*ARGSUSED*/
static void exit_cb( Widget w, XtPointer client_data, XtPointer call_data)
{
	int n;

	reset_locale();

	LoadHourlyFieldsExit();
	LoadDailyFieldsExit();
	LoadStaticFieldsExit();

	for(n = 0; n < GV_nfield; n++)
	{
		SetFieldExistance(GV_field[n]);
	}

	for(n = 0; n < 3; n++)
	{
		XmStringFree(btn_label[n]);
	}

	dialog = NullWidget;

	if (action_taken)
	{
		SetT0Depiction(T0_INITIALIZE_NEW_ONLY);
		MakeActiveDepiction(ACTIVE);
		ResetActiveField();
	}
}
