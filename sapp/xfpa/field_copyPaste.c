/****************************************************************************/
/*
*  File:    field_copyPaste.c
*
*  Purpose: Creates and controls the copy and paste edit sub-panel. This
*           will appear for any edit control that involves moving things.
*           Although called copy and paste, this panel actually cuts, copies,
*           pasts and selects all as well as setting the move mode state.
*
*  Note:    The button moveLabelsBtn is for setting the MOVE_MODE state.
*           This decides if labels will be moved (or pasted) along with any
*           selected objects in the field.
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
/****************************************************************************/

#include <string.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include "global.h"
#include "editor.h"
#include "observer.h"
#include "contextMenu.h"


static Widget  panel             = NullWidget;
static Widget  cutBtn            = NullWidget;
static Widget  copyBtn           = NullWidget;
static Widget  pasteBtn          = NullWidget;
static Widget  selectAllBtn      = NullWidget;
static Widget  moveLabelsBtn     = NullWidget;
static Boolean move_labels_state = True;



/* Respond to status information from Ingred. We expect:
 *
 *     prams[0] = E_MODE or E_EDIT
 *     prams[1] = E_BUTTON
 *     parms[2] = specific button command is for
 *     parms[3] = "ON" or "OFF"
 */
/*ARGSUSED*/
static void action_ingred_messages (CAL cal, String *parms, int nparms)
{
	String btn;
	Boolean on;

	if(!GV_edit_mode) return;
	if(!same_ic(parms[0],E_EDIT)) return;
	if(!same_ic(parms[1],E_BUTTON)) return;

	btn = parms[2];
	on  = same_ic(parms[3],E_ON);

	if(same_ic(btn, E_COPY))
	{
		/* Cut and copy go lock step here so only operate on copy status */
		XtSetSensitive(copyBtn, on);
		XtSetSensitive(cutBtn, on);
	}
	else if(same_ic(btn, E_PASTE))
	{
		XtSetSensitive(pasteBtn, on);
	}
	else if(same_ic(btn, E_SELECT_ALL))
	{
		XtSetSensitive(selectAllBtn, on);
		SetContextPasteSelectAllButtonsState(on? SELECT_ALL_BUTTON_ON:SELECT_ALL_BUTTON_OFF);
	}
}



/*  Callback for the command buttons
*/
/* ARGSUSED */
static void commands_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	String cmd = (String)client_data;

	if(same(cmd, E_SELECT_ALL))
	{
		if(GV_active_field && GV_active_field->editor && GV_active_field->editor->active)
		{
			IngredVaEditCommand(GV_active_field->cal, NullCal, "%s %s %s", E_EDIT, GV_active_field->editor->active->cmd, cmd);
		}
	}
	else
	{
		(void) IngredCommand(GE_EDIT, cmd);
	}
}



/*ARGSUSED*/
static void move_labels_state_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	move_labels_state = XmToggleButtonGetState(w);
	(void) IngredVaCommand(GE_ACTION, "STATE MOVE_MODE %s", move_labels_state? "FIELD_AND_LABELS":"FIELD");
	XuVaStateDataSave(MOVE_MODE_STATE_KEY, FieldTypeID(GV_active_field), NULL, "%d", move_labels_state);
}



/* This creates the sub-panel and inserts itself as an observer for
 * messages from Ingred.
 */
void CreateCopyPasteSubpanel(Widget parent, Widget top_attach)
{
	Widget rc;

	panel = XmVaCreateForm(parent, "copyPanel",
		XmNresizable, False,
		XmNverticalSpacing, 30,
		XmNhorizontalSpacing, 20,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, top_attach,
		NULL);

	rc = XmVaCreateManagedRowColumn(panel, "cmdBtnManager",
		XmNentryAlignment, XmALIGNMENT_CENTER,
		XmNpacking, XmPACK_COLUMN,
		XmNspacing, 5,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	cutBtn = XmVaCreateManagedPushButton(rc, "cutBtn",
		XmNsensitive, False,
		NULL);
	XtAddCallback(cutBtn, XmNactivateCallback, commands_cb, (XtPointer) E_DELETE);

	copyBtn = XmVaCreateManagedPushButton(rc, "copyBtn",
		XmNsensitive, False,
		NULL);
	XtAddCallback(copyBtn, XmNactivateCallback, commands_cb, (XtPointer) E_COPY);

	pasteBtn = XmVaCreateManagedPushButton(rc, "pasteBtn",
		XmNsensitive, False,
		NULL);
	XtAddCallback(pasteBtn, XmNactivateCallback, commands_cb, (XtPointer) E_PASTE);

	selectAllBtn = XmVaCreateManagedPushButton(rc, "selectAll",
		XmNsensitive, True,
		NULL);
	XtAddCallback(selectAllBtn, XmNactivateCallback, commands_cb, (XtPointer) E_SELECT_ALL);

	moveLabelsBtn = XmVaCreateManagedToggleButton(panel, "moveLabels",
		XmNborderWidth, 1,
		XmNset, move_labels_state,
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, rc,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(moveLabelsBtn, XmNvalueChangedCallback, move_labels_state_cb, NULL);

	AddIngredObserver(action_ingred_messages);
}


/* Configures the copy paste panel for the currently active field type.
 */
void ConfigureCopyPastePanel(void)
{
	switch(GV_active_field->info->element->fld_type)
	{
		case FpaC_LCHAIN:
			Manage(moveLabelsBtn, False);
			break;

		default:
			Manage(moveLabelsBtn, True);
			break;
	}
}


/*
 * The parameter btns_to_display can be either ALL or SELECT_ALL_BUTTON_ONLY.
 * This allows the same selection object to be used for copy/paste and select
 * all or for the select all function only.
 */
void ShowCopyPasteSubpanel(int btns_to_display)
{
	if(IsNull(panel)) return;

	switch(btns_to_display)
	{
		case SELECT_ALL_BUTTON_ONLY:
			XtManageChild(selectAllBtn);
			XtUnmanageChild(cutBtn);
			XtUnmanageChild(copyBtn);
			XtUnmanageChild(pasteBtn);
			break;

		case SELECT_ALL_BUTTON_OFF:
			XtManageChild(cutBtn);
			XtManageChild(copyBtn);
			XtManageChild(pasteBtn);
			XtUnmanageChild(selectAllBtn);
			break;

		case SHOW_COPY_PASTE_BUTTONS_ONLY:
			XtManageChild(copyBtn);
			XtManageChild(pasteBtn);
			XtUnmanageChild(cutBtn);
			XtUnmanageChild(selectAllBtn);
			break;

		default:
			XtManageChild(cutBtn);
			XtManageChild(copyBtn);
			XtManageChild(pasteBtn);
			XtManageChild(selectAllBtn);
			break;
	}

	if(XtIsManaged(moveLabelsBtn))
	{
		int val = 1;
		(void) XuVaStateDataGet(MOVE_MODE_STATE_KEY, FieldTypeID(GV_active_field), NULL, "%d", &val);
		move_labels_state = (Boolean) val;
		XuToggleButtonSet(moveLabelsBtn, move_labels_state, True);
	}

	if(!XtIsManaged(panel))
		XtManageChild(panel);

	SetContextPasteSelectAllButtonsState(btns_to_display);
}


void HideCopyPasteSubpanel(void)
{
	if(NotNull(panel) && XtIsManaged(panel))
		XtUnmanageChild(panel);

	SetContextPasteSelectAllButtonsState(NO_BUTTON_SELECT);
}
