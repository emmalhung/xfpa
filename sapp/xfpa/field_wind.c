/****************************************************************************
*
*  File:    field_wind.c
*
*  Purpose: Provides the program interface for any wind field.
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
****************************************************************************/

#include <string.h>
#include <ingred.h>
/* Undefine bzero and bcopy to stopy compiler complaints */
#undef bzero
#undef bcopy
#include "global.h"
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include "editor.h"
#include "observer.h"
#include "wind.h"
#include "contextMenu.h"

#define NMEMKEY      "nmem"
#define MAINWIND     "mainwind"
#define FLDNAMES(f)  f->info->element->name,f->info->level->name


static Boolean add_to_memory                (FIELD_INFO *, WIND_CALC *);
static void    change_background            (WIND_CALC *);
static void    context_cb                   (int);
static void    delete_active_data_cb        (Widget, XtPointer, XtPointer);
static void    destroy_data                 (FIELD_INFO *);
static void    edit_cmd_action              (String);
static void    entry                        (void);
static Boolean get_state_store_wind         (String, WIND_CALC *);
static void    memory_select_cb             (Widget, XtPointer, XtPointer);
static void    launch_entry_dialog_cb       (Widget, XtPointer, XtPointer);
static void    launch_bngnd_entry_dialog_cb (Widget, XtPointer, XtPointer);
static void    send_edit_cmb                (int);
static void    set_bkgnd_from_ingred_cmd    (FIELD_INFO*, CAL);
static void    send_direct                  (WIND_CALC *);
static void    store_in_memory              (WIND_CALC *);
static void    show_panel                   (Boolean);
static void    status_from_ingred           (CAL, String*, int);
static void    update_sample_list           (void);

static WINDMEM  *memory;
static int      nmemBtn = 0;
static Widget   *memBtn = NULL;
static Widget   mainPanel;
static Widget   bkgndDisplay;
static Widget   valueDisplay;
static Widget   memoryManager;
static Widget   setBtn, setBkgndBtn, deleteBtn;
static Widget   deleteMemDialog = NullWidget;

static CONTEXT modify_context = {
	E_MODIFY, 0, 2,
	{
		{setBtnId,E_SET},
		{deleteAreaBtnId,E_DELETE}
	}
};
static CONTEXT delete_context = {
	E_MODIFY, 0, 1,
	{
		{deleteAreaBtnId,E_DELETE}
	}
};
static CONTEXT delete_hole_context = {
	E_MODIFY, 0, 2,
	{
		{deleteHoleBtnId,E_DELETE_HOLE},
		{deleteAreaBtnId,E_DELETE}
	}
};
static CONTEXT move_context = {
	E_MOVE, 0, 4,
	{
		{translateBtnId,E_TRANSLATE},
		{rotateBtnId,E_ROTATE},
		{cutBtnId,E_CUT},
		{copyBtnId,E_COPY}
	}
};
static CONTEXT divide_context = {
	E_DIVIDE, 0, 2,
	{
		{setBtnId,E_SET},
		{leaveAsIsBtnId,E_SET}
	}
};
static CONTEXT merge_context  = {
	E_MERGE, 0, 3,
	{
		{mergeBtnId,E_MERGE},
		{translateBtnId,E_TRANSLATE},
		{rotateBtnId,E_ROTATE}
	}
};


/*=========================================================================*/
/*
*	CreateWindFieldPanel() - Create the actual widgets used in the
*	panel.  In the case of wind fields this must include a special
*	element dependent sub-panel for wind labels.  Users require that wind
*	labels be both in offset values and wind barbs.
*
*	Note that the initialize function can be called any number of times
*	for different wind element types, but that the widgets in the edit
*	panel are created only once.
*/
/*=========================================================================*/
void CreateWindFieldPanel(Widget parent , Widget topAttach)
{
	Widget label, memoryManagerWindow, valueFrame, bkgndFrame;
	Pixel displayBkgnd = XuLoadColor(parent,
			XuGetStringResource("*XmTextField.background","RGB:6A/6A/8D"));

	/* First create the main panel used by all the wind fields.
	*/
	mainPanel = XmVaCreateForm(parent, "windFieldPanel",
		XmNborderWidth, 0,
		XmNhorizontalSpacing, 3,
		XmNverticalSpacing, 3,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNbottomOffset, 3,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 3,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 3,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, topAttach,
		XmNtopOffset, 3,
		NULL);

	label = XmVaCreateManagedLabel(mainPanel, "bkgndLabel",
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 9,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	bkgndFrame = XmVaCreateManagedFrame(mainPanel, "bkgndFrame",
		XmNshadowType, XmSHADOW_IN,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	bkgndDisplay = XmVaCreateManagedLabel(bkgndFrame, "bkgndDisplay",
		XmNmarginHeight, 5,
		XmNbackground, displayBkgnd,
		NULL);

	setBkgndBtn = XmVaCreateManagedPushButton(mainPanel, "setBkgndBtn",
		XmNmarginHeight, 2,
		XmNmarginWidth, 5,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, bkgndFrame,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	XtAddCallback(setBkgndBtn, XmNactivateCallback, launch_bngnd_entry_dialog_cb, NULL);

	label = XmVaCreateManagedLabel(mainPanel, "valueLabel",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, setBkgndBtn,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	valueFrame = XmVaCreateManagedFrame(mainPanel, "valueFrame",
		XmNshadowType, XmSHADOW_IN,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	valueDisplay = XmVaCreateManagedLabel(valueFrame, "valueDisplay",
		XmNmarginHeight, 5,
		XmNbackground, displayBkgnd,
		NULL);

	setBtn = XmVaCreateManagedPushButton(mainPanel, "setBtn",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, valueFrame,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNrightPosition, 48,
		NULL);

	XtAddCallback(setBtn, XmNactivateCallback, launch_entry_dialog_cb, NULL);

	deleteBtn = XmVaCreateManagedPushButton(mainPanel, "deleteBtn",
		XmNsensitive, False,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, valueFrame,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 52,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	XtAddCallback(deleteBtn, XmNactivateCallback, delete_active_data_cb, (XtPointer)0);

	label = XmVaCreateManagedLabel(mainPanel, "memoryManagerLabel",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, setBtn,
		XmNtopOffset, 20,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	memoryManagerWindow = XmVaCreateManagedScrolledWindow(mainPanel, "memoryManagerWindow",
		XmNborderWidth, 1,
		XmNshadowThickness, 0,
		XmNscrollingPolicy, XmAUTOMATIC,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, 0,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	memoryManager = XmVaCreateManagedRowColumn(memoryManagerWindow, "memoryManager",
		XmNwidth, 170,
		XmNborderWidth, 0,
		XmNresizeWidth, False,
		XmNspacing, 0,
		NULL);

	AddIngredObserver(status_from_ingred);
}


/*=========================================================================*/
/*
*	AddWindField() - Initialize. Get the last stored state of the memory
*                    buttons and restore. Note that the depicton sequence,
*   if any, must have been read in before this function is called.
*/
/*=========================================================================*/
void AddWindField(FIELD_INFO  *field )
{
	int       i, data_total;
	Boolean   ok;
	char      mbuf[256];
	String    data, dpy;
	CAL       cal;
	WIND_CALC wind;
	FpaConfigElementEditorStruct *editor;

	memory = OneMem(WINDMEM);
	editor = GV_edit_mode ? field->info->element->elem_detail->editor : NULL;

	(void) snprintf(mbuf, sizeof(mbuf), "FIELD BACKGROUND %s %s", FLDNAMES(field));
	(void) GEStatus(mbuf, NULL, NULL, NULL, &cal);

	field->entryFcn       = entry;
	field->showFcn        = show_panel;
	field->sendEditCmdFcn = send_edit_cmb;
	field->changeEditFcn  = edit_cmd_action;
	field->depictFcn      = update_sample_list;
	field->setBkgndFcn    = set_bkgnd_from_ingred_cmd;
	field->destroyFcn     = destroy_data;
	field->memory         = memory;
	field->editor         = GetEditor((editor)? WIND_FIELD_EDITOR:WIND_FIELD_NO_EDIT);
	field->cal            = CAL_create_by_edef(field->info->element);
	field->bkgnd_cal      = CAL_duplicate(cal);

	SetFieldSampleInfo(field, FpaCsampleControlWindType, NULL);

	/* If there is an initial wind sample type specified set it but check for null's in
	 * the path to it.
	 */
	if(	field->info->element->elem_detail->sampling->type.wind &&
		field->info->element->elem_detail->sampling->type.wind->windsample  )
		field->sample.item = field->info->element->elem_detail->sampling->type.wind->windsample->name;

	if(editor)
	{
		XtSetSensitive(setBtn, True);
		XtSetSensitive(memoryManager, True);
		XtSetSensitive(setBkgndBtn, True);

		/* Load up the memory cells from the state store file.
		*/
		data_total = 0;
		XuVaStateDataGet(FLDNAMES(field), NMEMKEY, "%d", &data_total);
		for( i = 0; i < data_total; i++ )
		{
			(void) snprintf(mbuf, sizeof(mbuf), "mem%d", i);
			if(!XuStateDataGet(FLDNAMES(field), mbuf, &data)) continue;
			if( get_state_store_wind(data, &wind) ) (void)add_to_memory(field, &wind);
			FreeItem(data);
		}

		/* Now set the active wind value from state and if not found use memory.
		*/
		dpy = " ";
		ok = XuStateDataGet(FLDNAMES(field), MAINWIND, &data);
		if (ok) ok = get_state_store_wind(data, &wind);
		if (ok) ok = build_wind_attribs(field->cal, &wind);
		if (ok)
		{
			dpy = CAL_get_attribute(field->cal, CALuserlabel);
		}
		else if(memory->ncal > 0)
		{
			(void) CAL_destroy(field->cal);
			field->cal = CAL_duplicate(memory->cal[0]);
			dpy = CAL_get_attribute(field->cal, CALuserlabel);
		}
		XuWidgetLabel(valueDisplay, dpy);
		XuWidgetLabel(bkgndDisplay, CAL_get_attribute(field->bkgnd_cal, CALuserlabel));
		FreeItem(data);
	}
	else if(GV_edit_mode)
	{
		XtSetSensitive(setBtn, False);
		XtSetSensitive(memoryManager, False);
		XtSetSensitive(setBkgndBtn, False);
	}
}



/*====================== LOCAL FUNCTIONS ================================*/


/* Right mouse button context menu function. See the ActivateSelectContextMenu
 * function in panel_fieldEdit.c
 */
static void action_command (CONTEXT *info, CAL cal)
{
	if(info->selected_item->btn_id == setBtnId)
	{
		ACTIVATE_windEntryDialog(mainPanel, cal, send_direct, store_in_memory);
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


/*   Responds to the edit status commands sent by Ingred. For now the command
 *   string is EDIT WIND SELECT|DESELECT [SET]
 */
/*ARGSUSED*/
static void status_from_ingred( CAL cal, String *parms, int nparms )
{
	if(!GV_edit_mode || !same_ic(parms[0],E_EDIT) || !same_ic(parms[1],E_WIND)) return;

	memory->set_modify_cal = NULL;

	if(same_ic(parms[2], E_SELECT))
	{
		if(InEditMode(E_MOVE))
		{
			ActivateSelectContextMenu(&move_context, action_command, NULL);
		}
		else if(InEditMode(E_MERGE))
		{
			ActivateSelectContextMenu(&merge_context, action_command, NULL);
		}
		else if(InEditMode(E_DIVIDE))
		{
			if(same_ic(parms[3],E_SET))
			{
				XtSetSensitive(setBtn, True);
				XtSetSensitive(memoryManager, True);
				ActivateSelectContextMenu(&divide_context, action_command, cal);
				ShowContextMenuAttributes(True);
				memory->set_modify_cal = cal;
			}
			else
			{
				XtSetSensitive(setBtn, False);
				XtSetSensitive(memoryManager, False);
				DeactivateSelectContextMenu();
				ShowContextMenuAttributes(False);
			}
		}
		else if(InEditMode(E_MODIFY))
		{
			if(same_ic(parms[3],E_SET))
			{
				XtSetSensitive(setBtn, True);
				XtSetSensitive(memoryManager, True);
				ShowContextMenuAttributes(True);
				ActivateSelectContextMenu(&modify_context, action_command, cal);
				if (cal) memory->set_modify_cal = cal;
			}
			else if(same_ic(parms[3], E_HOLE))
			{
				XtSetSensitive(setBtn, False);
				XtSetSensitive(memoryManager, False);
				ShowContextMenuAttributes(False);
				ActivateSelectContextMenu(&delete_hole_context, action_command, cal);
				if (cal) memory->set_modify_cal = cal;
			}
			else
			{
				XtSetSensitive(setBtn, False);
				XtSetSensitive(memoryManager, False);
				ShowContextMenuAttributes(False);
				ActivateSelectContextMenu(&delete_context, action_command, NULL);
			}
		}
		else
		{
			ShowContextMenuAttributes(InEditMode(E_DRAW));
			DeactivateSelectContextMenu();
		}
	}
	else
	{
		DestroyWindEntryDialog();
		DestroyBkgndWindEntryDialog();
		ShowContextMenuAttributes(InEditMode(E_DRAW));
		DeactivateSelectContextMenu();
		if(InEditMode(E_MODIFY) || InEditMode(E_DIVIDE))
		{
			XtSetSensitive(setBtn, same_ic(parms[3], E_SET));
			XtSetSensitive(memoryManager, same_ic(parms[3], E_SET));
		}
		else
		{
			XtSetSensitive(setBtn, InEditMode(E_DRAW));
			XtSetSensitive(memoryManager, InEditMode(E_DRAW));
		}
		if(NotNull(cal) && !InEditMode(E_DRAW))
		{
			(void) CAL_destroy(GV_active_field->cal);
			GV_active_field->cal = CAL_duplicate(cal);
			XuWidgetLabel(valueDisplay, CAL_get_attribute(GV_active_field->cal, CALuserlabel));
			UpdateWindEntryDialog(GV_active_field->cal);
		}
	}
}


static Boolean get_state_store_wind(String data, WIND_CALC *wind)
{
	String   model;
	Boolean  ok;
	FpaConfigCrossRefStruct *xref;

	model = string_arg(data);

	if(same_ic(model,FpaAbsWindModel))
		wind->model = FpaAbsWindModel;
	else if((xref = identify_crossref(FpaCcRefsWinds, model)))
		wind->model = xref->name;
	else
		return False;

			wind->dir       = float_arg(data, &ok);
	if (ok) wind->rel_dir   = (int_arg(data,&ok) == 0);
	if (ok) wind->speed     = float_arg(data, &ok);
	if (ok) wind->rel_speed = (int_arg(data,&ok) == 0);
	if (ok) wind->gust      = float_arg(data, &ok);

	return (ok);
}


/*
*	Called by the field editor when any wind field is activated.  The panel 
*	information is retrieved from the field data structure and the wind 
*	data for that panel is inserted into the display text and into the 
*	selection buttons.
*/
static void entry(void)
{
	int      i;

	memory = GV_active_field->memory;

	InitContextMenuAttributes(GV_active_field->info->sh_label, context_cb);
	XtUnmanageChildren(memBtn, (Cardinal) nmemBtn);
	for( i = 0; i < memory->ncal; i++ )
	{
		String label = CAL_get_attribute(memory->cal[i],CALuserlabel);
		XuWidgetLabel(memBtn[i], label);
		AddToContextMenuAttributes(label);
	}
	XtManageChildren(memBtn, (Cardinal) memory->ncal);
	XuWidgetLabel(valueDisplay, CAL_get_attribute(GV_active_field->cal,CALuserlabel));
	XuWidgetLabel(bkgndDisplay, CAL_get_attribute(GV_active_field->bkgnd_cal,CALuserlabel));
	XtSetSensitive(deleteBtn, (memory->ncal > 0));

	show_panel(True);
}



/* Callback for ConfigureMainContextMenuAuxPushButton */
/*ARGSUSED*/
static void aux_push_button_cb(XtPointer unused)
{
	ACTIVATE_windEntryDialog(mainPanel, NULL, send_direct, store_in_memory);
}


/*	Called by the edit command function whenever an edit command button is pushed. */
static void edit_cmd_action(String cmd )
{
	Manage(mainPanel, same(cmd,E_DRAW) || same(cmd,E_MODIFY) || same(cmd,E_DIVIDE));
	DeactivateSelectContextMenu();

	if(same_ic(cmd,E_DRAW))
	{
		XtSetSensitive(setBtn,  True);
		XtSetSensitive(memoryManager,  True);
		ConfigureMainContextMenuAuxPushButton("setBtn", aux_push_button_cb, NULL);
		ShowContextMenuAttributes(True);
	}
	else
	{
		XtSetSensitive(setBtn, False);
		XtSetSensitive(memoryManager, False);
		ReleaseMainContextMenuAuxPushButton();
		ShowContextMenuAttributes(False);
	}
}


/* Sends editor commands to the graphics editor.
*
* Commands:	The following is a list of the recoginzed commands and a
*			description of what action is taken upon a call to this routine.
*
*			ACCEPT_MODE - Post edit.
*			SET_MODE    - Inserts the SET option into the appropriate command.
*           STACK_MODE  - Inserts the STACK option.
*/
static void send_edit_cmb(int cmd)
{
	char txbuf[256];

	if( !ValidEditField() ) return;

	if(InEditMode(E_SAMPLE))
	{
		SendEditSampleCommand(NULL);
	}
	else if(InEditMode(E_LABEL))
	{
		SendEditLabelCommand(NULL);
	}
	else
	{
		if( InEditMode(E_DIVIDE) )
		{
			(void) snprintf(txbuf, sizeof(txbuf), "EDIT %s %s", GV_active_field->editor->active->cmd,
																(cmd == SET_MODE)? "SET":"");
		}
		else if( InEditMode(E_MODIFY) )
		{
			if( cmd == STACK_MODE )
            {
				(void) snprintf(txbuf, sizeof(txbuf), "EDIT %s STACK %s",
								GV_active_field->editor->active->cmd, GV_stacking_order);
            }
			else
			{
				(void) snprintf(txbuf, sizeof(txbuf), "EDIT %s %s", GV_active_field->editor->active->cmd,
																(cmd == SET_MODE)? "SET":"");
			}
		}
		else if( InEditMode(E_LABEL) )
		{
			(void) snprintf(txbuf, sizeof(txbuf), "EDIT %s WIND", GV_active_field->editor->active->cmd);
		}
		else
		{
			(void) snprintf(txbuf, sizeof(txbuf), "EDIT %s", GV_active_field->editor->active->cmd);
		}
		(void) IngredEditCommand(txbuf, GV_active_field->cal, NullCal);
	}
}


/* Change the "available for sample" list to reflect the actual availability
 * of fields to allow sampling the winds as listed in the configuration file.
 *  Note that this function is set up to be called every time the user
 *  moves to a new depiction as well.
 */
static void update_sample_list(void)
{
	ConfigureSamplingPanel();
}


/* Frees any resources assigned to the field by this editor.
*/
static void destroy_data(FIELD_INFO *field )
{
	int i;
	WINDMEM *mem = field->memory;

	for( i = 0; i < mem->ncal; i++ ) (void) CAL_destroy(mem->cal[i]);
	FreeItem(mem->cal);
	FreeItem(mem);
}


static void show_panel(Boolean show )
{
	if (show)
	{
		edit_cmd_action(GV_active_field->editor->active->cmd);
	}
	else
	{
		edit_cmd_action(NULL);
		DestroyWindEntryDialog();
		DestroyBkgndWindEntryDialog();
	}
}


/*ARGSUSED*/
static void launch_bngnd_entry_dialog_cb( Widget w, XtPointer client_data, XtPointer call_data )
{
	ACTIVATE_bkgndWindEntryDialog(mainPanel, change_background);
}


/*ARGSUSED*/
static void launch_entry_dialog_cb( Widget w, XtPointer client_data, XtPointer call_data )
{
	ACTIVATE_windEntryDialog(mainPanel, memory->set_modify_cal, send_direct, store_in_memory);
}


static void change_background(WIND_CALC *wind)
{
	char mbuf[256];
	CAL  cal = CAL_create_by_edef(GV_active_field->info->element);

	if(build_wind_attribs(cal, wind))
	{
		(void) CAL_destroy(GV_active_field->bkgnd_cal);
		GV_active_field->bkgnd_cal = CAL_duplicate(cal);
		if(GV_edit_mode)
			XuWidgetLabel(bkgndDisplay, CAL_get_attribute(cal, CALuserlabel));

		(void) snprintf(mbuf, sizeof(mbuf), "BACKGROUND %s %s", FLDNAMES(GV_active_field));
		(void) IngredEditCommand(mbuf, cal, NullCal);
	}
	(void) CAL_destroy(cal);
}


/*  Function called from main.c when Ingred sends a status report on the background.
 *  If cal is null then Ingred is asking for the background to be set by the interface.
 *   Any other value means that the background must be set in the interface to agree
 *   with what Ingred has sent.
 */
static void set_bkgnd_from_ingred_cmd(FIELD_INFO *fld , CAL cal)
{
	if(IsNull(cal))
	{
		SetFieldBackground(fld);
	}
	else
	{
		(void) CAL_destroy(fld->bkgnd_cal);
		fld->bkgnd_cal = CAL_duplicate(cal);
	}

	if(fld == GV_active_field)
	{
		XuWidgetLabel(bkgndDisplay, CAL_get_attribute(fld->bkgnd_cal,CALuserlabel));
	}
}


/* 	Sends the information directly to Ingred without saving it in memory.
*/
static void send_direct(WIND_CALC *wind)
{
	if(!build_wind_attribs(GV_active_field->cal, wind)) return;

	XuWidgetLabel(valueDisplay, CAL_get_attribute(GV_active_field->cal, CALuserlabel));
	send_edit_cmb(SET_MODE);

	XuVaStateDataSave(FLDNAMES(GV_active_field), MAINWIND,
		"%s %d %d %d %d %d",
		wind->model,
		NINT(wind->dir),   wind->rel_dir   ? 0:1,
		NINT(wind->speed), wind->rel_speed ? 0:1,
		NINT(wind->gust),
		NULL);
}


/* Calls add_to_memory() and stores the data in the interface state file.
*/
static void store_in_memory(WIND_CALC *wind)
{
	if(add_to_memory(GV_active_field, wind))
	{
		char mbuf[256];
		XuVaStateDataSave(FLDNAMES(GV_active_field), NMEMKEY, "%d", memory->ncal);
		(void) snprintf(mbuf, sizeof(mbuf), "mem%d",memory->ncal-1);
		XuVaStateDataSave(FLDNAMES(GV_active_field), mbuf,
			"%s %d %d %d %d %d",
			wind->model,
			NINT(wind->dir),   wind->rel_dir   ? 0:1,
			NINT(wind->speed), wind->rel_speed ? 0:1,
			NINT(wind->gust),
			NULL);
	}
}


/* Stores the given display string, value string and sub-element into a memory button.
 */
static Boolean add_to_memory(FIELD_INFO *fld, WIND_CALC *wind)
{
	char   mbuf[256];
	String label;
	CAL    cal;

	/* Create another memory button if needed
	*/
	if( memory->ncal >= nmemBtn )
	{
		memBtn = MoreWidgetArray(memBtn, nmemBtn+1);
		(void) snprintf(mbuf, sizeof(mbuf), "wxMemBtn%d",nmemBtn);
		memBtn[nmemBtn] = XmCreatePushButton(memoryManager, mbuf, NULL, 0);
		XtAddCallback(memBtn[nmemBtn], XmNactivateCallback,
			memory_select_cb, INT2PTR(nmemBtn));
		nmemBtn++;
	}

	/* Make sure that we have a valid entry before incrementing CAL array.
	*/
	cal = CAL_create_by_edef(fld->info->element);
	if(NotNull(cal) && build_wind_attribs(cal, wind))
	{
		memory->cal = MoreMem(memory->cal, CAL, memory->ncal+1);
		memory->cal[memory->ncal] = cal;
		label = CAL_get_attribute(cal, CALuserlabel);
		XuWidgetLabel(memBtn[memory->ncal], label);
		XtManageChild(memBtn[memory->ncal]);
		AddToContextMenuAttributes(label);
		memory->ncal++;
		XtSetSensitive(deleteBtn, (memory->ncal > 0));
		return True;
	}
	else
	{
		(void) CAL_destroy(cal);
		return False;
	}
}


/* Callback function for the memory buttons.
*/
/*ARGSUSED*/
static void memory_select_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	WIND_CALC wind;
	int       ndx = PTR2INT(client_data);

	(void) CAL_destroy(GV_active_field->cal);
	GV_active_field->cal = CAL_duplicate(memory->cal[ndx]);
	UpdateWindEntryDialog(GV_active_field->cal);
	XuWidgetLabel(valueDisplay, CAL_get_attribute(GV_active_field->cal, CALuserlabel));
	send_edit_cmb(SET_MODE);

	if(parse_wind_attribs(GV_active_field->cal, &wind))
	{
		XuVaStateDataSave(FLDNAMES(GV_active_field), MAINWIND,
			"%s %d %d %d %d %d",
			wind.model,
			NINT(wind.dir),   wind.rel_dir   ? 0:1,
			NINT(wind.speed), wind.rel_speed ? 0:1,
			NINT(wind.gust),
			NULL);
	}
}


/* This is the callback from the context menu attribute setting procedure, so we
 * just need to call memory_select_cb with the index.
 */
static void context_cb( int ndx )
{
	memory_select_cb(NULL, INT2PTR(ndx), NULL);
}


/* Deletes the selected memory locations.
*/
/* ARGSUSED */
static void DeleteMemoryCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	int    *poslist, lenlist;

	w = *((Widget*)client_data);
	if(XmListGetSelectedPos(w, &poslist, &lenlist))
	{
		int    i, k, n, ndx;
		char   mbuf[256];
		String data, label;
		CAL    *callist;

		InitContextMenuAttributes(GV_active_field->info->sh_label, context_cb);
		XtUnmanageChildren(memBtn, (Cardinal) nmemBtn);
		callist = NewMem(CAL, memory->ncal - lenlist);
		i = k = 0;
		ndx = poslist[0] - 1;
		for( n = 0; n < memory->ncal; n++ )
		{
			(void) snprintf(mbuf, sizeof(mbuf), "mem%d", n);
			(void) XuStateDataGet(FLDNAMES(GV_active_field), mbuf, &data);
			XuStateDataRemove(FLDNAMES(GV_active_field), mbuf);
			
			if( n == ndx )
			{
				i++;
				ndx = (i < lenlist) ? poslist[i]-1 : -1;
			}
			else
			{
				(void) snprintf(mbuf, sizeof(mbuf), "mem%d", k);
				XuStateDataSave(FLDNAMES(GV_active_field), mbuf, data);
				callist[k] = memory->cal[n];
				label = CAL_get_attribute(callist[k],CALuserlabel);
				XuWidgetLabel(memBtn[k], label);
				AddToContextMenuAttributes(label);
				k++;
			}
			FreeItem(data);
		}
		memory->ncal -= lenlist;
		XtManageChildren(memBtn, (Cardinal) memory->ncal);
		FreeItem(poslist);
		FreeItem(memory->cal);
		memory->cal = callist;
	}
	XtSetSensitive(deleteBtn, (memory->ncal > 0));
	XuVaStateDataSave(FLDNAMES(GV_active_field), NMEMKEY, "%d", memory->ncal);
	XuDestroyDialog(deleteMemDialog);
	deleteMemDialog = NullWidget;
}


/* Creates and launches the dialog for the selection and deletion of memory locations.
 */
/*ARGSUSED*/
static void delete_active_data_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int      i;
	String   attrib;
	XmString item;

	static Widget list;  /* needs to be static to be passed in struct below */

	static XuDialogActionsStruct action_items[] = {
		{ "deleteBtn", DeleteMemoryCB,    &list },
		{ "cancelBtn", XuDestroyDialogCB, NULL  }
	};

	if(NotNull(deleteMemDialog)) return;

	deleteMemDialog = XuCreateFormDialog(w, "deleteMemDialog",
		XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
        XuNdestroyCallback, XuExitOnDestroyCallback,
		XuNdestroyData, &deleteMemDialog,
		NULL);

	list = XmVaCreateManagedScrolledList(deleteMemDialog, "deleteList",
		XmNselectionPolicy, XmEXTENDED_SELECT,
		XmNlistSizePolicy, XmCONSTANT,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNlistMarginHeight, 3,
		XmNlistMarginWidth, 5,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	for(i = 0; i < memory->ncal; i++)
	{
		attrib = CAL_get_attribute(memory->cal[i], CALuserlabel);
		item = XuNewXmString(blank(attrib) ? "-?-":attrib);
		XmListAddItem(list, item, 0);
		XmStringFree(item);
	}

	XuShowDialog(deleteMemDialog);
}
