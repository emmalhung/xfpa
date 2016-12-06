/*****************************************************************************
*
*  File:    field_point.c
*
*  Purpose: Provides the program interface for any point type field.
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
#include <Xm/Frame.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <ingred.h>
#include "editor.h"
#include "observer.h"
#include "contextMenu.h"

/* To shorten code statements */
#define MEM	GV_active_field->memory

/* This structure is not strictly necessary for this field type, but is used
 * for future consideration when more information may be required to be
 * stored.
 */
typedef struct _memory {
	int                                 select;
	FpaConfigElementScatteredTypeStruct *info;
} MEMORY;

static void destroy_data       (FIELD_INFO*);
static void edit_cmd_action    (String);
static void entry              (void);
static void send_edit_cmd      (int);
static void set_cb             (Widget, XtPointer, XtPointer);
static void show_panel         (Boolean);
static void status_from_ingred (CAL, String*, int);

static Widget     panel      = NullWidget;
static Widget     displayFrame, manager, setBtn;
static WidgetList type_btns  = (WidgetList)0;
static int        ntype_btns = 0;

/* The button labels and command for the selected object context menus. These are used to set
 * the popup button labels and to provide the commands to be sent to ingred associated with
 * the buttons.
 */
static CONTEXT move_context = {
	E_MOVE, 0, 3,
	{
		{translateBtnId,E_TRANSLATE},
		{cutBtnId,E_CUT},
		{copyBtnId,E_COPY}
	}
};
static CONTEXT modify_context = {
	E_MODIFY, 0, 2,
	{
		{setBtnId, E_SET},
		{deletePointBtnId,E_DELETE}
	}
};
static CONTEXT merge_context = {
	E_MERGE, 0, 2,
	{
		{mergeBtnId,E_MERGE},
		{translateBtnId,E_TRANSLATE}
	}
};


/* Create edit panel for line type fields.
*/
void CreatePointFieldPanel(Widget parent , Widget topAttach)
{
	panel = XmVaCreateForm(parent, "pointPanel",
		XmNverticalSpacing, 3,
		XmNhorizontalSpacing, 3,
		XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, topAttach,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	setBtn = XmVaCreateManagedPushButton(panel, "setBtn",
		XmNsensitive, False,
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 19,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 19,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 19,
		NULL);

	XtAddCallback(setBtn, XmNactivateCallback, set_cb, NULL);

	displayFrame = XmVaCreateFrame(panel, "pointList",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, setBtn,
		XmNtopOffset, 19,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	(void) XmVaCreateManagedLabel(displayFrame, "typeList",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	manager = XmVaCreateManagedRowColumn(displayFrame, "manager",
		XmNradioBehavior, True,
		NULL);

	AddIngredObserver(status_from_ingred);
}


/* The given field is to be added as a point (scattered) type element.
*/
void AddPointField(FIELD_INFO  *field )
{
	FpaConfigElementEditorStruct *editor;

	editor = GV_edit_mode ? field->info->element->elem_detail->editor : NULL;

	field->entryFcn		  = entry;
	field->showFcn		  = show_panel;
	field->sendEditCmdFcn = send_edit_cmd;
	field->changeEditFcn  = edit_cmd_action;
	field->depictFcn      = NULL;
	field->setBkgndFcn    = NULL;
	field->destroyFcn	  = destroy_data;
	field->memory		  = OneMem(MEMORY);
	field->editor		  = GetEditor((editor)?POINT_FIELD_EDITOR:POINT_FIELD_NO_EDIT);
	field->cal            = CAL_create_by_edef(field->info->element);
	field->bkgnd_cal      = NullCal;

	SetFieldSampleInfo(field, FpaCsampleControlAttribType, AttribAll);

	if(editor && GV_edit_mode)
	{
		field->memory->select = 0;
		field->memory->info   = field->info->element->elem_detail->scattered_types;

		CAL_set_attribute(field->cal, CALscatteredtype, field->memory->info->type_names[0]);
		CAL_set_attribute(field->cal, CALcategory,      field->memory->info->type_names[0]);
		CAL_set_attribute(field->cal, CALuserlabel,     field->memory->info->type_labels[0]);
		CAL_set_attribute(field->cal, CALautolabel,     field->memory->info->type_labels[0]);
	}
}


/*==================== LOCAL STATIC FUNCTIONS ===========================*/


/*  This is the function called by the attributes entry dialog when data is to
 *  be sent to Ingred.
*/
static void set_active_cal_value(CAL cal)
{
    if (IsNull(cal)) return;
	(void) CAL_destroy(GV_active_field->cal);
	GV_active_field->cal = CAL_duplicate(cal);
	send_edit_cmd(SET_MODE);
}


/*ARGSUSED*/
static void set_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	ACTIVATE_pointAttributesDialog(panel, GV_active_field->cal, set_active_cal_value);
}


/* Right mouse button context menu functions. See the
 * ActivateSelectContextMenu function in panel_fieldEdit.c
 */
static void action_command (CONTEXT *info, CAL cal)
{
	if(same(info->edit_mode,E_DRAW))
	{
		IngredVaEditCommand(NULL, NullCal, "%s %s %s %s",
			E_EDIT, E_DRAW,
			(cal)? CAL_get_attribute(cal, CALscatteredtype) : MEM->info->type_names[0],
			info->selected_item->edit_cmd );
	}
	else if(same(info->edit_mode,E_MODIFY))
	{
		if(same(info->selected_item->edit_cmd,E_SET))
			ACTIVATE_pointAttributesDialog(panel, cal, set_active_cal_value);
		else
			IngredVaEditCommand(NULL, NullCal, "%s %s %s %s",
				E_EDIT, E_MODIFY,
				(cal)? CAL_get_attribute(cal, CALscatteredtype) : MEM->info->type_names[0],
				info->selected_item->edit_cmd );
	}
	else
	{
		IngredVaEditCommand(NULL, NullCal, "%s %s %s", E_EDIT, info->edit_mode, info->selected_item->edit_cmd);
		if(same(info->edit_mode,E_MERGE))
		{
			String notify[2];
			notify[0] = E_MERGE;
			notify[1] = info->selected_item->edit_cmd;
			NotifyObservers(OB_MENU_ACTIVATION, notify, 2);
		}
	}
}



/* Handles status function calles from Ingred. This at the moment are the
*  selection and deselection of lines from within modify.
*/
static void status_from_ingred( CAL cal, String *parms, int nparms )
{
	if(!GV_edit_mode) return;
	if(!same_ic(parms[0],E_EDIT)) return;
	if(!same_ic(parms[1],E_POINT)) return;
	if(!IsActiveEditor(POINT_FIELD_EDITOR)) return;

	XtSetSensitive(setBtn, False);

	if(same_ic(parms[2], E_SELECT))
	{
		XtSetSensitive(panel, True);

		if(same_ic(parms[3],E_SET) && cal)
		{
			(void) CAL_destroy(GV_active_field->cal);
			GV_active_field->cal = CAL_duplicate(cal);
		}

		if(InEditMode(E_DRAW))
		{
			DeactivateSelectContextMenu();
			ACTIVATE_pointAttributesDialog(panel, GV_active_field->cal, set_active_cal_value);
		}
		else if(InEditMode(E_MOVE))
		{
			ActivateSelectContextMenu(&move_context, action_command, NULL);
		}
		else if(InEditMode(E_MERGE))
		{
			ActivateSelectContextMenu(&merge_context, action_command, NULL);
		}
		else if(InEditMode(E_MODIFY))
		{
			XtSetSensitive(setBtn, same_ic(parms[3],E_SET));
			ActivateSelectContextMenu(&modify_context, action_command, cal);
		}
		else
		{
			DeactivateSelectContextMenu();
		}
	}
	else
	{
		XtSetSensitive(panel, False);
		DestroyAttributesEntryDialog();
		DeactivateSelectContextMenu();

		if(same_ic(parms[3],"SET") && cal)
		{
			(void) CAL_destroy(GV_active_field->cal);
			GV_active_field->cal = CAL_duplicate(cal);
		}
	}
}


/* Callback function for the point types toggle button.
 */
/*ARGSUSED*/
static void type_cb( Widget w, XtPointer client_data, XtPointer call_data )
{
	if(!XmToggleButtonGetState(w)) return;

	MEM->select = PTR2INT(client_data);

	CAL_set_attribute(GV_active_field->cal, CALscatteredtype, MEM->info->type_names[MEM->select]);
	CAL_set_attribute(GV_active_field->cal, CALcategory,      MEM->info->type_names[MEM->select]);
	CAL_set_attribute(GV_active_field->cal, CALuserlabel,     MEM->info->type_labels[MEM->select]);
	CAL_set_attribute(GV_active_field->cal, CALautolabel,     MEM->info->type_labels[MEM->select]);

	send_edit_cmd(ACCEPT_MODE);
}


/* Called when a point field is activated.
*/
static void entry(void)
{
	int      n;
	XmString xmlabel;

	if(GV_active_field->editor == GetEditor(POINT_FIELD_NO_EDIT)) return;

	InitContextMenuAttributes(NULL, NULL);

	/* If we have only one point type there is no point in putting up
	 * the point type selector as this would only confuse the user.
	 */
	if( MEM->info->ntypes < 2 )
	{
		XtUnmanageChild(displayFrame);
	}
	else
	{
		/* Do we need more selection buttons?
		 */
		if( MEM->info->ntypes > ntype_btns )
		{
			type_btns = MoreWidgetArray(type_btns, MEM->info->ntypes);
			for(n = ntype_btns; n < MEM->info->ntypes; n++ )
			{
				type_btns[n] = XmVaCreateManagedToggleButton(manager, "btn", NULL);
				XtAddCallback(type_btns[n], XmNvalueChangedCallback, type_cb, INT2PTR(n));
			}
			ntype_btns = MEM->info->ntypes;
		}

		/* Put the labels for the current field in the buttons and set to the
		 * active selection for this field.
		 */
		for(n=0; n < MEM->info->ntypes; n++)
		{
			xmlabel = XuNewXmString(MEM->info->type_labels[n]);
			XtVaSetValues(type_btns[n],
				XmNlabelString, xmlabel,
				XmNset, (n == MEM->select),
				NULL);
			XmStringFree(xmlabel);
		}
		XtManageChild(displayFrame);
	}
}


/* To be called by the edit command function whenever an edit command
*  button is pushed.
*/
static void edit_cmd_action(String cmd )
{
	Manage(panel, same(cmd,E_DRAW) || same(cmd,E_MODIFY));
}


/* Frees up any resources assigned by this editor when the field is
*  removed from the system.
*/
/*ARGSUSED*/
static void destroy_data(FIELD_INFO *field )
{
	FreeItem(field->memory);
	(void) CAL_destroy(field->cal);
	field->cal = (CAL) NULL;
}


/* Called when we wish to show or hide the panel.
*/
static void show_panel(Boolean show )
{
	edit_cmd_action((show)? GV_active_field->editor->active->cmd : NULL);
	if (!show) DestroyAttributesEntryDialog();
}


/* Sends editor commands to the graphics editor. The recognized cmd
 * parameters are:
 *
 *	ACCEPT_MODE - Post edit.
 *	SET_MODE    - Inserts the SET option into the appropriate command.
 */
static void send_edit_cmd(int	cmd )
{
	char txbuf[200];

	if( !ValidEditField() ) return;

	if(InEditMode(E_SAMPLE))
	{
		SendEditSampleCommand(NULL);
	}
	else
	{
		String type = NULL;
		String set  = NULL;
		if( InEditMode(E_MODIFY) || InEditMode(E_DRAW) )
		{
			type = MEM->info->type_names[MEM->select];
		}
		if( cmd == SET_MODE && (InEditMode(E_MODIFY) || InEditMode(E_DRAW) || InEditMode(E_MOVE)) )
		{
			set = E_SET;
		}
		(void) snprintf(txbuf, sizeof(txbuf), "%s %s %s %s", E_EDIT,  GV_active_field->editor->active->cmd, type, set);
		(void) IngredEditCommand(txbuf, GV_active_field->cal, NullCal);
	}
}
