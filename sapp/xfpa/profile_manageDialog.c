/*==============================================================================*/
/*
*	   File:	profile_manageDialog.c
*
*	Purpose:	Manages profiles by allowing copying and deleting. Note that
*	            the active profile cannot be deleted.
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
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/TextF.h>


/* For the action buttons callbacks
 */
enum { PROFILE_CANCEL, PROFILE_DELETE, PROFILE_COPY };

static Widget  dialog     = NullWidget;	/* main dialog */
static Widget  copyDialog = NullWidget;	/* dialog for adding a profile */
static Widget  listW      = NullWidget;	/* profile list widget */
static Widget  textW      = NullWidget;	/* Text entry widget */
static int     list_pos   = -1;			/* currently selected profile */
static int     list_len   = 0;			/* number of profiles */
static String  *list      = NULL;		/* list of profile names */
static int     active     = -1;			/* active profile when dialog entered */


/*==================== Private Functions ===========================*/


/*ARGSUSED*/
static void copy_profile_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int    pos;
	String p, profile = XmTextFieldGetString(textW);

	/* Replace illegal characters (comma and non-printing ones) */
	p = profile;
	while(*p)
	{
		if(*p == ',' || isprint((int)*p) == 0) *p = ' ';
		p++;
	}

	if(!blank(profile))
	{
		XuListAddItem(listW, profile);
		pos = list_len;
		list_len++;
		list = MoreStringArray(list, list_len);
		list[pos] = XtNewString(profile);

		/* Create our new profile.
		 * 1. Set the active profile to the list position.
		 * 2. Create the new profile (it copies the active profile)
		 * 3. Reset the profile back to the original one
		 */
		XuSetProfile((list_pos < 0)? NULL : list[list_pos]);
		if(XuCreateProfile(list[pos]))
		{
			XmListSelectPos(listW, pos+1, True);
		}
		XuSetProfile((active < 0)? NULL : list[active]);
	}
	XuDestroyDialog(copyDialog);
}


static void make_copy_profile_dialog(void)
{
	Widget textLabel;

	static XuDialogActionsStruct action_items[] = {
		{"applyBtn",  copy_profile_cb,    NULL},
		{"cancelBtn", XuDestroyDialogCB, NULL}
	};

	copyDialog = XuCreateFormDialog(dialog, "addProfile",
		XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL,
		XmNkeyboardFocusPolicy, XmEXPLICIT,
		XuNretainGeometry, XuRETAIN_NONE,
		XuNdestroyCallback, XuExitOnDestroyCallback,
		XuNdestroyData, &copyDialog,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XmNhorizontalSpacing, 19,
		XmNverticalSpacing, 19,
		NULL);

	textLabel = XmVaCreateManagedLabel(copyDialog, "textLabel",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	textW = XmVaCreateManagedTextField(copyDialog, "text",
		XmNhighlightOnEnter, True,
		XmNhighlightThickness, 2,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, textLabel,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	XuShowDialog(copyDialog);
}



/* Called from the action buttons of the selection box */
/*ARGSUSED*/
static void action_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int    btn_action = PTR2INT(client_data);

	if (!dialog) return;

	if(btn_action == PROFILE_COPY)
	{
		make_copy_profile_dialog();
	}
	else if(btn_action == PROFILE_DELETE)
	{
		if(list_pos >= 0)
		{
			if(active == list_pos)
			{
				XuShowMessage(dialog, "ActiveProfileDelete", list[list_pos], NULL);
			}
			else if(XuAskUser(dialog, "ProfileDelete", list[list_pos], NULL) == XuYES)
			{
				int i, n;
				for(n = 0, i = 0; i < list_len; i++)
				{
					if(i == list_pos)
					{
						XuDestroyProfile(list[i]);
						XtFree(list[i]);
					}
					else
					{
						list[n++] = list[i];
					}
				}
				/* If our active depiction is above our deletion point
				 * decrement it to maintain position.
				 */
				if(active >= list_pos) active--;
				list_len--;
				list_pos = -1;
				XuListLoad(listW, list, list_len, 0);
				if(active >= 0)
					XmListSelectPos(listW, active+1, True);
			}
		}
	}
	else if(btn_action == PROFILE_CANCEL)
	{
		XuSetProfile((active < 0)? NULL:list[active]);
		FreeList(list, list_len);
		list_len = 0;
		XuDestroyDialog(dialog);
		dialog = NullWidget;
	}
}


/*ARGSUSED*/
static void selection_list_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmListCallbackStruct *lcb = (XmListCallbackStruct *) call_data;
	list_pos = lcb->item_position - 1;
}


/*===================== Public Functions ==========================*/


/* This is the profile selection dialog
 */
void ACTIVATE_manageProfileDialog(Widget refw)
{
	int       i;
	XmString *items = NULL;

	static XuDialogActionsStruct action_items[] = {
		{"copyBtn",   action_cb, (XtPointer) PROFILE_COPY  },
		{"deleteBtn", action_cb, (XtPointer) PROFILE_DELETE},
		{"closeBtn",  action_cb, (XtPointer) PROFILE_CANCEL}
	};
	
	dialog = XuCreateFormDialog(refw, "manageProfile",
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XuNrelativePosition, True,
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		NULL);

	/* Create the list and determine the currently active profile */
	list_len = XuGetProfiles(&list);
	if(list_len)
	{
		items  = NewXmStringArray(list_len);
		for(i = 0; i < list_len; i++)
		{
			(void) XuGetProfileStateData(list[i], NULL, NULL);
			if(XuIsActiveProfile(list[i])) active = i;
			items[i] = XmStringCreateLocalized(list[i]);
		}
	}

	listW = XmVaCreateManagedScrolledList(dialog, "sl",
		XmNitems, items,
		XmNitemCount, list_len,
		XmNselectionPolicy, XmSINGLE_SELECT,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	XtAddCallback(listW, XmNsingleSelectionCallback, selection_list_cb, NULL);
	XmStringArrayFree(items, list_len);

	if(active >= 0)
	{
		list_pos = active;
		XmListSelectPos(listW, active+1, True);
	}

	XuShowDialog(dialog);
}
