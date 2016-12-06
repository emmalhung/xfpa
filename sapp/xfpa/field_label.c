/*****************************************************************************
*
*  File:     field_label.c
*
*  Purpose:  Common editor interface for the label edit function. This
*            allows the user to select the required label type and if the
*            type has attributes which are user setable, allows the user
*            access to the entry menu.
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
*
*****************************************************************************/

#include "global.h"
#include <Xm/PushB.h>
#include <Xm/Frame.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include "editor.h"
#include "observer.h"
#include "selector.h"
#include "contextMenu.h"

/* This reduces the length of the code lines and makes reading easier */
#define LabelInfo GV_active_field->info->element->elem_detail->labelling

/* To avoid potential finger trouble */
#define ADDBTN	"addBtn"
#define MODBTN	"modBtn"
#define DELBTN	"deleteBtn"
#define SHOWBTN	"showBtn"
#define MOVEBTN	"moveBtn"

static String  action_cmd     = E_ADD;				/* current actions */
static Widget  action_btn     = NullWidget;			/* name of current action button */
static Widget  labelPanel     = NullWidget;
static Widget  actionSelect   = NULL;
static int     ntypeBtns      = 0;					/* number of label type buttons */
static Widget *typeBtns       = NullWidgetList;		/* buttons for label types */
static int     type_ndx       = 0;					/* currently selected label type */
static Widget  selectFrame    = NullWidget;
static Widget  actionFrame    = NullWidget;
static Widget  typeSelect     = NullWidget;
static Widget  addBtn         = NullWidget;
static Widget  modifyBtn      = NullWidget;
static Widget  moveBtn        = NullWidget;
static Widget  showBtn        = NullWidget;
static Widget  deleteBtn      = NullWidget;
static Widget  selectLabel    = NullWidget;
static Widget  actionLabel    = NullWidget;
static Widget  menu1Popup     = NullWidget;
static Widget  *menu1Btns     = NullWidgetList;
static Widget  menu2Popup     = NullWidget;
static Widget  menu2addBtn    = NullWidget;
static Widget  menu2deleteBtn = NullWidget;
static Widget  menu2moveBtn   = NullWidget;
static Widget  menu2modifyBtn = NullWidget;
static Widget  menu2showBtn   = NullWidget;
/*
 * Disable the send to ingred if false. This is needed as the
 * label type toggle needs to be set with a true in the notify
 * field of the set function so that the rowColumn knows about
 * the selection but we don't want the callback to send anything
 * to Ingred.
 */
static Boolean send = True;

/* Local functions */
static void action_cb            (Widget, XtPointer, XtPointer);
static void select_cb            (Widget, XtPointer, XtPointer);
static void menu_cb              (Widget, XtPointer, XtPointer);
static void set_attributes       (CAL);
static void messages_from_ingred (CAL, String*, int);


/*=======================================================================*/
/*
*  Create the label panel. The mapContextMenu parameter is the popup menu
*  to use to add the label context information to.
*/
/*=======================================================================*/
void CreateLabelFieldPanel(Widget parent, Widget topAttach)
{
	labelPanel = XmVaCreateForm(parent, "labelPanel",
		XmNverticalSpacing, 19,
		XmNhorizontalSpacing, 3,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, topAttach,
		NULL);

	selectFrame = XmVaCreateManagedFrame(labelPanel, "frame",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	selectLabel = XmVaCreateManagedLabel(selectFrame, "labelSelectTitle",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	typeSelect = XmVaCreateManagedRowColumn(selectFrame, "labelRC",
		XmNradioBehavior, True,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	actionFrame = XmVaCreateManagedFrame(labelPanel, "frame",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, selectFrame,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	actionLabel = XmVaCreateManagedLabel(actionFrame, "labelActionTitle",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	actionSelect = XmVaCreateManagedRowColumn(actionFrame, "selectRC",
		XmNradioBehavior, True,
		XmNpacking, XmPACK_COLUMN,
		XmNnumColumns, 2,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	addBtn = XmVaCreateManagedToggleButton(actionSelect, ADDBTN, NULL);
	XtAddCallback(addBtn, XmNvalueChangedCallback, action_cb, (XtPointer)E_ADD);
	action_btn = addBtn;

	modifyBtn = XmVaCreateManagedToggleButton(actionSelect, MODBTN, NULL);
	XtAddCallback(modifyBtn, XmNvalueChangedCallback, action_cb, (XtPointer)E_MODIFY);

	deleteBtn = XmVaCreateManagedToggleButton(actionSelect, DELBTN, NULL);
	XtAddCallback(deleteBtn, XmNvalueChangedCallback, action_cb, (XtPointer)E_DELETE);

	moveBtn = XmVaCreateManagedToggleButton(actionSelect, MOVEBTN, NULL);
	XtAddCallback(moveBtn, XmNvalueChangedCallback, action_cb, (XtPointer)E_MOVE);

	showBtn = XmVaCreateManagedToggleButton(actionSelect, SHOWBTN, NULL);
	XtAddCallback(showBtn, XmNvalueChangedCallback, action_cb, (XtPointer)E_SHOW);

	/* Create the main context menu pullright menus. These will duplicate the
	 * toggle button selection in the context menu so that the user can change
	 * the label type and action from within the map area.
	 */
	menu1Popup = CreateMainContextMenuAuxCascadePulldown("cmpr");
	menu2Popup = CreateMainContextMenuAuxCascadePulldown("cmpr");

	menu2addBtn = XmVaCreateManagedPushButton(menu2Popup, ADDBTN, NULL);
	XtAddCallback(menu2addBtn, XmNactivateCallback, menu_cb, (XtPointer) addBtn);

	menu2modifyBtn = XmVaCreateManagedPushButton(menu2Popup, MODBTN, NULL);
	XtAddCallback(menu2modifyBtn, XmNactivateCallback, menu_cb, (XtPointer) modifyBtn);

	menu2moveBtn = XmVaCreateManagedPushButton(menu2Popup, MOVEBTN, NULL);
	XtAddCallback(menu2moveBtn, XmNactivateCallback, menu_cb, (XtPointer) moveBtn);

	menu2showBtn = XmVaCreateManagedPushButton(menu2Popup, SHOWBTN, NULL);
	XtAddCallback(menu2showBtn, XmNactivateCallback, menu_cb, (XtPointer) showBtn);

	menu2deleteBtn = XmVaCreateManagedPushButton(menu2Popup, DELBTN, NULL);
	XtAddCallback(menu2deleteBtn, XmNactivateCallback, menu_cb, (XtPointer) deleteBtn);

	/* Last add ourselves to receive messages from ingred
	 */
	AddIngredObserver(messages_from_ingred);
}


/*=======================================================================*/
/*
*   Creates the list of sources available for label selection and sets
*   the appearance of the widgets for the applicable field type.
*/
/*=======================================================================*/
void ConfigureLabelFieldPanel(void)
{
	int      n;
	Boolean  no_mod_file = True;

	if(LabelInfo->ntypes > ntypeBtns)
	{
		/* The panel buttons */
		typeBtns = MoreWidgetArray(typeBtns, LabelInfo->ntypes);
		for(n = ntypeBtns; n < LabelInfo->ntypes; n++)
		{
			typeBtns[n] = XmCreateToggleButton(typeSelect, "btn", NULL, 0);
			XtAddCallback(typeBtns[n], XmNvalueChangedCallback, select_cb, INT2PTR(n));
		}
		/* The context menu buttons */
		menu1Btns = MoreWidgetArray(menu1Btns, LabelInfo->ntypes);
		for(n = ntypeBtns; n < LabelInfo->ntypes; n++)
		{
			menu1Btns[n] = XmCreatePushButton(menu1Popup, "btn", NULL, 0);
			XtAddCallback(menu1Btns[n], XmNactivateCallback, menu_cb, (XtPointer) typeBtns[n]);
		}
		ntypeBtns = LabelInfo->ntypes;
	}
	/*
	 * No modify files means that you are not allowed to modify.
	 */
	if(LabelInfo->type_modify_files)
	{
		for(n = 0; n < LabelInfo->ntypes; n++)
		{
			if(blank(LabelInfo->type_modify_files[n])) continue;
			no_mod_file = False;
			break;
		}
	}
	Manage(modifyBtn, !no_mod_file);
	Manage(menu2modifyBtn, !no_mod_file);
	/*
	 * Change the button labels to those of the current field.
	 */
	XtUnmanageChildren(typeBtns, (Cardinal) ntypeBtns);
	XtUnmanageChildren(menu1Btns, (Cardinal) ntypeBtns);
	for(n = 0; n < LabelInfo->ntypes; n++)
	{
		XuWidgetLabel(typeBtns[n], LabelInfo->type_labels[n]);
		XuWidgetLabel(menu1Btns[n], LabelInfo->type_labels[n]);
	}
	XtManageChildren(typeBtns, (Cardinal) LabelInfo->ntypes);
	XtManageChildren(menu1Btns, (Cardinal) LabelInfo->ntypes);
	/*
	 * Initialize for the new field
	 */
	type_ndx   = 0;
	action_cmd = E_ADD;
	action_btn = addBtn;
	send = False;
	XuToggleButtonSet(addBtn, True, True);
	if( ntypeBtns > 0)
		XuToggleButtonSet(typeBtns[0], True, True);
	send = True;
	/*
	 * For those cases where the show panel may not be called.
	 */
	if(XtIsManaged(labelPanel))
		ConfigureMainContextMenuAuxCascade(selectLabel, menu1Popup, actionLabel, menu2Popup);
}


/*=======================================================================*/
/*
*    Send the current label selections as a label command.
*/
/*=======================================================================*/
void SendEditLabelCommand(String process)
{
	if( send && ValidEditField() )
		(void) IngredVaCommand(GE_DEPICTION, "LABEL %s %s", blank(process)? action_cmd : process,
			LabelInfo->type_names[type_ndx]);
}


/*=======================================================================*/
/*
*   Show and hide the panel.
*/
/*=======================================================================*/
void ShowLabelFieldPanel()
{
	if (!labelPanel) return;
	Manage(labelPanel, True);
	ConfigureMainContextMenuAuxCascade(selectLabel, menu1Popup, actionLabel, menu2Popup);
}

void HideLabelFieldPanel()
{
	if (!labelPanel || !XtIsManaged(labelPanel)) return;

	XtUnmanageChild(labelPanel);
	ReleaseMainContextMenuAuxCascade();
	DestroyAttributesEntryDialog();
}


/*======================== LOCAL FUNCTIONS ==============================*/


/*  Function called when Ingred requests that an entry menu be posted so that
 *  the user can enter label attributes. The label type of the selected label
 *  is passed back by Ingred and the interface must adjust typeBtns accordingly.
 *  Ingred will also let us know then the modification is finished so that we
 *  can reset things.
 */
/*ARGSUSED*/
static void messages_from_ingred( CAL cal, String *parms, int nparms)
{
	int     n, i;
	String  menu_file = NULL;

	static int saved_type_ndx = 0;

	if(!same_ic(parms[0], E_LABEL)) return;

	if(same_ic(parms[1], E_OFF))
	{
		/* Label modification finished so reset the type buttons */
		type_ndx = saved_type_ndx;
		for(n = 0; n < LabelInfo->ntypes; n++)
			XtSetSensitive(typeBtns[n], True);
		send = False;
		if(ntypeBtns > 0 && type_ndx < ntypeBtns)
			XuToggleButtonSet(typeBtns[type_ndx], True, True);
		send = True;
		DestroyAttributesEntryDialog();
		return;
	}
	else if(!blank(parms[1]))
	{
		/* Modifying a label. Set the appropriate type button on and make
		 * all of the others insensitive. This to imply that we can not
		 * change the selection - which we are not allowed to do.
		 */
		saved_type_ndx = type_ndx;
		for(n = 0; n < LabelInfo->ntypes; n++)
		{
			if(!same(parms[1],LabelInfo->type_names[n])) continue;
			type_ndx = n;
			send = False;
			XuToggleButtonSet(typeBtns[n], True, True);
			send = True;
			for(i = 0; i < LabelInfo->ntypes; i++)
				XtSetSensitive(typeBtns[i], (i==n));
			break;
		}
		
		/* If this occurs we had a major problem */
		if(n == LabelInfo->ntypes) return;
	}

	if(same(action_cmd,E_MODIFY))
		menu_file = LabelInfo->type_modify_files[type_ndx];
	else if(same(action_cmd,E_ADD))
		menu_file = LabelInfo->type_entry_files[type_ndx];

	if(blank(menu_file))
	{
		DestroyAttributesEntryDialog();
		set_attributes(cal);
	}
	else
	{
		ACTIVATE_labelAttributesDialog(GW_mainWindow, cal, menu_file, set_attributes);
	}
}


/* 2008.04.03 - Send Ingred an update command so that any label types created or
 * modified before the selection of the new type are saved.
 */
/*ARGSUSED*/
static void select_cb (Widget w, XtPointer client_data, XtPointer call_data)
{
	if(!XmToggleButtonGetState(w)) return;

	(void) IngredCommand(GE_EDIT, E_UPDATE);
	DestroyAttributesEntryDialog();
	type_ndx = PTR2INT(client_data);
	SendEditLabelCommand(NULL);
}


/*ARGSUSED*/
static void action_cb (Widget w, XtPointer client_data, XtPointer unused)
{
	if(!XmToggleButtonGetState(w)) return;

	action_cmd = (String)client_data;
	action_btn = w;
	if(!same(action_cmd,E_ADD) && !same(action_cmd,E_MODIFY))
		DestroyAttributesEntryDialog();
	SendEditLabelCommand(NULL);
	DEACTIVATE_attributeDisplayDialogs();
}


/* The callback for the context menu activates the corresponding toggle
 * button from the panel.
 */
/*ARGSUSED*/
static void menu_cb (Widget w, XtPointer client_data, XtPointer unused)
{
	XmToggleButtonSetState((Widget) client_data, True, True);
}


static void set_attributes(CAL cal)
{
	if (cal) IngredVaEditCommand(cal, NullCal, "LABEL %s %s SET", action_cmd, LabelInfo->type_names[type_ndx]);
}
