/*=========================================================================*/
/*
*      File: preferences_dialog.c
*
*   Purpose: Contains sections for setting user preferences in the 
*            operation of the interface. For the moment the sections are:
*
*            - Set the cursor style and colour.
*            - Set the map palette.
*            - Set the fields for be auto-imported from allied models.
*            - Set operational preferences.
*
*            Note that allied models are not included if fpa is running
*            in view only mode.
*
*            This dialog sets up the framework into which the individual
*            sections are placed. The sections themselves are in other
*            files. The files are:
*
*            - preferences_alliedModel.c
*            - preferences_cursor.c
*            - preferences_dialogLocation.c
*            - preferences_map.c
*            - preferences_general.c
*
*      Note: Due to the cursor setting procedure the cursor used in this
*            dialog is set and controlled by that procedure.
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
#include <Xu.h>
#include <Xm/Form.h>
#include <Xm/TabStack.h>
#include "help.h"
#include "alliedModel.h"
#include "preferences.h"


static Widget dialog    = NullWidget;
static Widget prefPage[5];
static int    nscreens  = 0;
static int    tab_posn  = 0;
static int    nuse_tabs = 0;
static String *use_tabs  = NULL;

static void ActionPrefsCB	(Widget, XtPointer, XtPointer);
static void ExitCB          (Widget, XtPointer, XtPointer);
static void LocalHelpCB		(Widget, XtPointer, XtPointer);
static void TabsCB			(Widget, XtPointer, XtPointer);
/*
 * Note that AlliedModel must be the last item in the list for the reomval
 * code below to work. If the order changes then the code will need to be
 * modified as well.
 */
static String edit_tab_ids[] = {"General","GUI","DialogLocation","AlliedModel"};
static String view_tab_ids[] = {"General","GUI","DialogLocation"};
static String one_screen_edit_tab_ids[] = {"General","GUI","AlliedModel"};
static String one_screen_view_tab_ids[] = {"General","GUI"};


void InitPreferences(void)
{
	InitAlliedModelOptions();
	InitGeneralOptions();
	InitMainGui();
	nscreens = InitDialogLocationOptions();
}


void ACTIVATE_preferencesDialog(Widget refw)
{
	int i;
	Widget tabs;

	static XuDialogActionsStruct action_items[] = {
		{ "applyBtn", ActionPrefsCB, NULL },
		{ "closeBtn", ExitCB,        NULL },
		{ "helpBtn",  LocalHelpCB,   NULL }
	 };

	if (dialog) return;

	tab_posn = 0;

	/* If there are no allied models defined then we can remove
	 * the tab by subtraction as it is always the last in the
	 * list.
	 */
	if(nscreens > 1)
	{
		if( GV_edit_mode)
		{
			nuse_tabs = XtNumber(edit_tab_ids);
			use_tabs  = edit_tab_ids;
			if(GV_nallied_model < 1) nuse_tabs--;
		}
		else
		{
			nuse_tabs = XtNumber(view_tab_ids);
			use_tabs  = view_tab_ids;
		}
	}
	else
	{
		if( GV_edit_mode)
		{
			nuse_tabs = XtNumber(one_screen_edit_tab_ids);
			use_tabs  = one_screen_edit_tab_ids;
			if(GV_nallied_model < 1) nuse_tabs--;
		}
		else
		{
			nuse_tabs = XtNumber(one_screen_view_tab_ids);
			use_tabs  = one_screen_view_tab_ids;
		}
	}

	dialog = XuCreateFormDialog(refw, "preferences",
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XmNhorizontalSpacing, 19,
		XmNverticalSpacing, 19,
		NULL);

	tabs = XmVaCreateTabStack(dialog, "tabs",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(tabs, XmNtabSelectedCallback, TabsCB, NULL);

	for(i = 0; i < nuse_tabs; i++)
	{
		prefPage[i] = XmVaCreateForm(tabs, use_tabs[i],
			XmNhorizontalSpacing, 19,
			XmNverticalSpacing, 19,
			NULL);
		
		switch(use_tabs[i][1])
		{
			case 'e': GeneralOptions(prefPage[i]);        break;
			case 'U': GuiOptions(prefPage[i]);            break;
			case 'l': AlliedModelOptions(prefPage[i]);    break;
			case 'i': DialogLocationOptions(prefPage[i]); break;
		}
		XtManageChild(prefPage[i]);
	}
	XtManageChild(tabs);
	XmTabStackSelectTab(prefPage[tab_posn], False);

	XuShowDialog(dialog);
}


/*ARGSUSED*/
static void TabsCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	int     i;
	XmTabStackCallbackStruct *rtn = (XmTabStackCallbackStruct *)call_data;

	for(i = 0; i < nuse_tabs; i++)
	{
		if(rtn->selected_child != prefPage[i]) continue;
		tab_posn = i;
		break;
	}
}


/* The help that is shown depends on the tab setting so we call the global
*  help function with this info.
*/
/*ARGSUSED*/
static void LocalHelpCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	switch(use_tabs[tab_posn][1])
	{
		case 'e':  Help(HELP_GENERAL_OPTIONS    ); break;
		case 'U':  Help(HELP_CHANGE_CURSOR      ); break;
		case 'l':  Help(HELP_ALLIED_MODEL_IMPORT); break;
		case 'i':  Help(HELP_DIALOG_LOCATION    ); break;
	}
}


/*ARGSUSED*/
static void ActionPrefsCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i;

	XuSetBusyCursor(ON);
	DeactivateMenu();
	for(i = 0; i < nuse_tabs; i++)
	{
		switch(use_tabs[i][1])
		{
			case 'e': SetGeneralOptions();        break;
			case 'U': SetGuiOptions();            break;
			case 'l': SetAlliedModelOptions();    break;
			case 'i': SetDialogLocationOptions(); break;
		}
	}
	ActivateMenu();
	XuSetBusyCursor(OFF);
}


/*ARGSUSED*/
static void ExitCB( Widget w , XtPointer client_data , XtPointer call_data )
{
	ExitGeneralOptions();
	ExitDialogLocationOptions();
	XuDestroyDialog(dialog);
	dialog = NULL;
}
