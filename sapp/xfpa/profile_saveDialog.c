/*==============================================================================*/
/*
*	   File:	profile_saveDialog.c
*
*	Purpose:	Saves profiles.
*
*	   Note:    Saving in this context means saving states related to the
*	            current profile or saving the current profile under another
*	            name and making that profile the active profile for the
*	            interface.
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
/*==============================================================================*/
#include <ctype.h>
#include <unistd.h>
#include "global.h"
#include "observer.h"
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/ToggleB.h>
#include <Xm/TextF.h>

/* For the action buttons callbacks
 */
enum { PROFILE_CANCEL, PROFILE_SAVE_AS, PROFILE_SAVE };

static Widget  dialog       = NullWidget;	/* main dialog */
static Widget  autoSave     = NullWidget;	/* toggle to auto save profile state */
static Widget  saveAsDialog = NullWidget;	/* dialog for adding a profile */
static Widget  textW        = NullWidget;	/* Text entry widget */


/*=============== Private Functions for save and save as  ======================*/


/* If the input profile name is valid, create a profile with that name and
 * set it as the active profile.
 */
/*ARGSUSED*/
static void save_as_profile_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	String p, profile = XmTextFieldGetString(textW);

	/* Replace illegal characters (comma and non-printing ones) */
	p = profile;
	while(*p)
	{
		if(*p == ',' || isprint((int)*p) == 0) *p = ' ';
		p++;
	}

	if(same_ic(profile, "default"))
	{
		XtFree(profile);
		XmTextFieldSetString(textW, "");
		return;
	}

	if(!blank(profile))
	{
		if(XuCreateProfile(profile))
		{
			XuSetProfile(profile);
			XuSaveProfileStateData(XmToggleButtonGetState(autoSave), NULL);
			XuSaveDialogProfile(XuPIN_INITIAL);
		}
		XuDestroyDialog(dialog);
		dialog = NullWidget;
	}
	XuDestroyDialog(saveAsDialog);
	XtFree(profile);
}


static void make_save_as_profile_dialog(void)
{
	Widget textLabel;

	static XuDialogActionsStruct action_items[] = {
		{"applyBtn",  save_as_profile_cb, NULL},
		{"cancelBtn", XuDestroyDialogCB,  NULL}
	};

	saveAsDialog = XuCreateFormDialog(dialog, "addProfile",
		XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL,
		XmNkeyboardFocusPolicy, XmEXPLICIT,
		XuNdefaultActionItemId, "applyBtn",
		XuNretainGeometry, XuRETAIN_NONE,
		XuNdestroyCallback, XuExitOnDestroyCallback,
		XuNdestroyData, &saveAsDialog,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XmNhorizontalSpacing, 19,
		XmNverticalSpacing, 19,
		NULL);

	textLabel = XmVaCreateManagedLabel(saveAsDialog, "textLabel",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	textW = XmVaCreateManagedTextField(saveAsDialog, "text",
		XmNhighlightOnEnter, True,
		XmNhighlightThickness, 2,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, textLabel,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	
	XtVaSetValues(XuGetActionAreaBtnByName(saveAsDialog, "applyBtn"),
			XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
			NULL);
	XtVaSetValues(XuGetActionAreaBtnByName(saveAsDialog, "cancelBtn"),
			XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
			NULL);
	XtVaSetValues(textW, XmNnavigationType,
			XmEXCLUSIVE_TAB_GROUP,
			NULL);

	XuShowDialog(saveAsDialog);
	(void) XmProcessTraversal(textW, XmTRAVERSE_CURRENT);
}



/*ARGSUSED*/
static void save_action_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int    btn_action = PTR2INT(client_data);

	if (!dialog) return;

	if(btn_action == PROFILE_SAVE_AS)
	{
		make_save_as_profile_dialog();
	}
	else if(btn_action == PROFILE_SAVE)
	{
		String notify[1];
		notify[0] = (String) ((long) XmToggleButtonGetState(autoSave));
		NotifyObservers(OB_PROFILE_SAVE, notify, 1);
		XuSaveProfileStateData(XmToggleButtonGetState(autoSave), NULL);
		XuSaveDialogProfile(XuPIN_INITIAL);
		XuDestroyDialog(dialog);
		dialog = NullWidget;
	}
	else if(btn_action == PROFILE_CANCEL)
	{
		XuDestroyDialog(dialog);
		dialog = NullWidget;
	}
}


/*================ Public Function for save and save as =====================*/


void ACTIVATE_saveProfileDialog(Widget refw)
{
	int     i, list_len;
	String  *list;
	Boolean dp;
	Widget  label, profile;

	static XuDialogActionsStruct action_items[] = {
		{"saveBtn",   save_action_cb, (XtPointer) PROFILE_SAVE   },
		{"saveAsBtn", save_action_cb, (XtPointer) PROFILE_SAVE_AS},
		{"closeBtn",  save_action_cb, (XtPointer) PROFILE_CANCEL }
	};
	
	dialog = XuCreateFormDialog(refw, "saveProfile",
		XuNactionAreaItems, action_items,
		XuNdefaultActionItemId, "saveBtn",
		XuNnumActionAreaItems, XtNumber(action_items),
		XuNrelativePosition, True,
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		NULL);

	label = XmVaCreateManagedLabel(dialog, "currentProfile",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	profile = XmVaCreateManagedLabel(dialog, "profile",
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, label,
		XmNleftOffset, 5,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	autoSave = XmVaCreateManagedToggleButton(dialog, "autoSave",
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	/* Determine the currently active profile */
	dp = True;
	XuWidgetLabel(profile, XuGetLabel("default"));
	XmToggleButtonSetState(autoSave, False, True);

	list_len = XuGetProfiles(&list);
	if(list_len)
	{
		Boolean flag;
		for(i = 0; i < list_len; i++)
		{
			(void) XuGetProfileStateData(list[i], &flag, NULL);
			if(XuIsActiveProfile(list[i]))
			{
				dp = False;
				XuWidgetLabel(profile, list[i]);
				XmToggleButtonSetState(autoSave, flag, True);
				break;
			}
		}
	}
	FreeList(list, list_len);

	/* Ghost out the save button and set the auto save to true
	 * if we are in the default profile
	 */
	if (dp)
	{
		XtSetSensitive(XuGetActionAreaBtn(dialog, action_items[0]), False);
		XmToggleButtonSetState(autoSave, True, True);
	}

	XuShowDialog(dialog);
}
