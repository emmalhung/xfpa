/****************************************************************************
*
*  File:    field_discrete.c
*
*  Purpose: Provides the program interface for any field of type
*           "discrete".  The field elem is used to determine which data
*           entry dialog to use for a particular field.
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
#include "global.h"
#include <ingred.h>
#include <X11/Shell.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/ToggleB.h>
#include <Xm/Text.h>
#include "editor.h"
#include "observer.h"
#include "contextMenu.h"

#define  BREAK        18,2            /* chars/line,max lines */
#define  SAVE_ACTIVE  -999
#define  NMEMKEY      "nmem"         /* number of memories stored */
#define  ACTIVEKEY    "mema"         /* state store key for active selection */
#define  FLDNAMES(f)  f->info->element->name,f->info->level->name


static void set_active_cal_value         (CAL);
static void set_bkgnd_cal_value          (CAL);
static void set_bkgnd_from_ingred_cmd    (FIELD_INFO*, CAL);
static void store_in_memory              (CAL);
static void entry                        (void);
static void edit_cmd_action              (String);
static void show_panel                   (Boolean);
static void memory_select_cb             (Widget, XtPointer, XtPointer);
static void delete_active_data_cb        (Widget, XtPointer, XtPointer);
static void launch_entry_dialog_cb       (Widget, XtPointer, XtPointer);
static void launch_bkgnd_entry_dialog_cb (Widget, XtPointer, XtPointer);
static void destroy_data                 (FIELD_INFO *);
static void save_cal                     (FIELD_INFO*, int);
static void send_edit_cmd                (int);
static void show_value_cb                (Widget, XtPointer, XtPointer);
static void status_from_ingred           (CAL, String*, int);

typedef struct _memory {
	int     total;				/* total number of memory arrays created */
	int     nfixed;				/* number of arrays not modifyable */
	int     num;				/* number of memory arrays in use now */
	CAL     *cal;				/* CAL data structures array */
	CAL     active_cal;			/* currently active cal data structure */
	Boolean attrib_editing;     /* can the field attributes be edited? */
	Boolean bkgnd_editing;      /* can the field background be modified? */
} FD_MEM;

static Boolean  initialized = False;
static FD_MEM   *mem;
static int      nmemBtns = 0;
static Widget   *memBtns = NULL;
static Widget   panel;
static Widget   bkgndDisplay;
static Widget   labelDisplay;
static Widget   memoryManager;
static Widget   showBtn;
static Widget   setBtn;
static Widget   deleteBtn;
static Widget   setBkgnd;
static Boolean  memoryManagerState = ON;
static Boolean  setBtnState = ON;
static Boolean  deleteBtnState = ON;
static CAL      modify_cal = NULL;

/* Button and command information for the action popup context menus
 */
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
static CONTEXT divide_set_context = {
	E_DIVIDE, 0, 2,
	{
		{setBtnId,E_SET},
		{leaveAsIsBtnId,E_SET}
	}
};
static CONTEXT divide_noset_context = {
	E_DIVIDE, 0, 1,
	{
		{leaveAsIsBtnId,E_SET}
	}
};
static CONTEXT merge_context = {
	E_MERGE, 0, 3,
	{
		{mergeBtnId,E_MERGE},
		{translateBtnId,E_TRANSLATE},
		{rotateBtnId,E_ROTATE}
	}
};


/*=========================================================================*/
/*
*	CreateDiscreteFieldPanel() - Create the actual widgets used in the panel.
*
*	Note that the initialize function can be called any number of times
*	for different discrete element types, but that the widgets in the edit
*	panel are created only once.
*/
/*=========================================================================*/
void CreateDiscreteFieldPanel(Widget parent , Widget topAttach)
{
	Widget w, label, memoryManagerWindow, valueBtn;

	panel = XmVaCreateForm(parent, "discreteFieldPanel",
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

	label = XmVaCreateManagedLabel(panel, "bkgndLabel",
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 6,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	bkgndDisplay = XmVaCreateManagedText(panel, "bkgndDisplay",
		XmNautoShowCursorPosition, False,
		XmNeditable, False,
		XmNeditMode, XmMULTI_LINE_EDIT,
		XmNmarginWidth, 4,
		XmNvalue, "",
		XmNpendingDelete, False,
		XmNcursorPositionVisible, False,
		XmNwordWrap, True,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	setBkgnd = XmVaCreateManagedPushButton(panel, "setBkgndBtn",
		XmNsensitive, False,
		XmNmarginHeight, 2,
		XmNmarginWidth, 5,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, bkgndDisplay,
		NULL);

	XtAddCallback(setBkgnd, XmNactivateCallback, launch_bkgnd_entry_dialog_cb, NULL);

	showBtn = XmVaCreateManagedPushButton(panel, "showBtn",
		XmNmarginHeight, 2,
		XmNmarginWidth, 5,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, setBkgnd,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, bkgndDisplay,
		NULL);

	XtAddCallback(showBtn, XmNarmCallback,    show_value_cb, (XtPointer)True);
	XtAddCallback(showBtn, XmNdisarmCallback, show_value_cb, NULL);

	label = XmVaCreateManagedLabel(panel, "valueLabel",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, bkgndDisplay,
		XmNtopOffset, 9,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	labelDisplay = XmVaCreateManagedText(panel, "labelDisplay",
		XmNautoShowCursorPosition, False,
		XmNeditable, False,
		XmNeditMode, XmMULTI_LINE_EDIT,
		XmNmarginWidth, 4,
		XmNvalue, "",
		XmNpendingDelete, False,
		XmNselectionArray, "select_position",
		XmNselectionArrayCount, 1,
		XmNcolumns, 18,
		XmNcursorPositionVisible, False,
		XmNwordWrap, True,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	valueBtn = XmVaCreateManagedPushButton(panel, "valueBtn",
		XmNmarginWidth, 3,
		XmNmarginHeight, 3,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, labelDisplay,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	XtAddCallback(valueBtn, XmNarmCallback,    show_value_cb, (XtPointer)False);
	XtAddCallback(valueBtn, XmNdisarmCallback, show_value_cb, NULL);

	deleteBtn = XmVaCreateManagedPushButton(panel, "deleteBtn",
		XmNmarginWidth, 3,
		XmNmarginHeight, 3,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, labelDisplay,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	XtAddCallback(deleteBtn, XmNactivateCallback, delete_active_data_cb, 0);

	setBtn = XmVaCreateManagedPushButton(panel, "setBtn",
		XmNmarginHeight, 3,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, labelDisplay,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, valueBtn,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, deleteBtn,
		NULL);

	XtAddCallback(setBtn, XmNactivateCallback, launch_entry_dialog_cb, NULL);

	w = XmVaCreateManagedLabel(panel, "memoryManagerLabel",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, setBtn,
		XmNtopOffset, 20,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	memoryManagerWindow = XmVaCreateManagedScrolledWindow(panel, "memoryManagerWindow",
		XmNborderWidth, 1,
		XmNshadowThickness, 0,
		XmNscrollingPolicy, XmAUTOMATIC,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, w,
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

	/* For some reason if the fpa is created with a height of less than about 550
	*  pixels, we get a BadValue X error on realization of this widget if it is
	*  managed.  Since we normally will be set to such sizes only in viewer mode
	*  we manage this one only if we are not in viewer mode.  Reason? Who knows!
	*/
	if (GV_edit_mode) XtManageChild(memoryManager);

	AddIngredObserver(status_from_ingred);
}


/*=========================================================================*/
/*
*	AddDiscreteField() - Initialize. Get the last stored state of the
*                        memory buttons and restore. Note that the data
*   entry menu used is field specific.
*/
/*=========================================================================*/
void AddDiscreteField(FIELD_INFO  *field )
{
	int i, data_total;
	char mbuf[300];
	String str, data, label, value;
	FpaConfigElementEditorStruct *editor;
	FILE *fp;
	CAL cal;

	mem = OneMem(FD_MEM);
	mem->active_cal = CAL_create_by_edef(field->info->element);

	editor = GV_edit_mode ? field->info->element->elem_detail->editor : NULL;

	if(editor && GV_edit_mode)
	{
		FpaConfigDiscreteEditorStruct *ds = editor->type.discrete;
		initialized = False;

		mem->attrib_editing = MenuFileExists(ENTRY_ATTRIBUTES|MODIFY_ATTRIBUTES, field);
		mem->bkgnd_editing  = MenuFileExists(BACKGROUND_ATTRIBUTES, field);

		/* First initialize the memory from the predefined memory file.
		*/
		if(NotNull(str=get_file(MEMORY_CFG,ds->memory_file)) && NotNull(fp=fopen(str,"r")))
		{
			String buf;
			String s, key, val;
			Boolean have_cal = False;
			cal = CAL_create_by_edef(field->info->element);

			while((buf = ReadLine(fp)))
			{
				if((s = strchr(buf,'='))) *s = ' ';
				key = strtok_arg(buf);
				val = strtok_arg(NULL);

				if(same_ic(key, "memory"))
				{
					if (have_cal)
					{
						CAL_invoke_rules(cal, ds->nrules, ds->entry_funcs);
						CAL_invoke_python_rules(cal, ds->py_nrules, ds->py_entry_rules);
						store_in_memory(cal);
						mem->nfixed++;
						(void) CAL_destroy(cal);
						cal = CAL_create_by_edef(field->info->element);
						have_cal = False;
					}
				}
				else if(valid_edef_attribute(field->info->element, key))
				{
					have_cal = True;
					CAL_add_attribute(cal, key, val);
				}
			}

			if (have_cal)
			{
				CAL_invoke_rules(cal, ds->nrules, ds->entry_funcs);
				CAL_invoke_python_rules(cal, ds->py_nrules, ds->py_entry_rules);
				store_in_memory(cal);
				mem->nfixed++;
			}
			(void) fclose(fp);
			(void) CAL_destroy(cal);
		}

		/* Now get memory from the state store file.
		*/
		data_total = 0;
		XuVaStateDataGet(FLDNAMES(field), NMEMKEY, "%d", &data_total);

		for( i = 0; i < data_total; i++ )
		{
			(void) snprintf(mbuf, sizeof(mbuf), "mem%d",i);
			if(!XuStateDataGet(FLDNAMES(field), mbuf, &data)) continue;
			cal = CAL_create_by_edef(field->info->element);
			while(NotNull(opt_arg(data, &label, &value)))
			{
				CAL_set_attribute(cal, label, value);
			}
			CAL_invoke_rules(cal, ds->nrules, ds->entry_funcs);
			CAL_invoke_python_rules(cal, ds->py_nrules, ds->py_entry_rules);
			store_in_memory(cal);
			(void) CAL_destroy(cal);
			FreeItem(data);
		}

		/* Finally the last active cal setting.
		*/
		if(XuStateDataGet(FLDNAMES(field), ACTIVEKEY, &data))
		{
			while(NotNull(opt_arg(data, &label, &value)))
			{
				CAL_set_attribute(mem->active_cal, label, value);
			}
			CAL_invoke_rules(mem->active_cal, ds->nrules, ds->entry_funcs);
			CAL_invoke_python_rules(mem->active_cal, ds->py_nrules, ds->py_entry_rules);
			FreeItem(data);
		}

		/* Now save back out to the state store file so that we have the newest
		*  calculated fields in case the attributes shift under our feet.
		*/
		XuStateDataRemove( FLDNAMES(field), "mem*" );
		save_cal(field, SAVE_ACTIVE);
		for(i = mem->nfixed; i < mem->num; i++) save_cal(field, i);
		XuVaStateDataSave(FLDNAMES(field), NMEMKEY, "%d", mem->num - mem->nfixed);
	}

	initialized = True;

	(void) snprintf(mbuf, sizeof(mbuf), "FIELD BACKGROUND %s %s", FLDNAMES(field));
	(void) GEStatus(mbuf, NULL, NULL, NULL, &cal);

	field->entryFcn       = entry;
	field->showFcn        = show_panel;
	field->sendEditCmdFcn = send_edit_cmd;
	field->changeEditFcn  = edit_cmd_action;
	field->depictFcn      = NULL;
	field->setBkgndFcn    = set_bkgnd_from_ingred_cmd;
	field->destroyFcn     = destroy_data;
	field->memory         = mem;
	field->editor         = GetEditor((editor) ? DISCRETE_FIELD_EDITOR : DISCRETE_FIELD_NO_EDIT);
	field->cal            = mem->active_cal;
	field->bkgnd_cal      = CAL_create_by_edef(field->info->element);

	SetFieldSampleInfo(field, FpaCsampleControlAttribType, AttribAll);
}


/*====================== LOCAL FUNCTIONS ================================*/


/* If state is OFF, then all of the buttons are set insensitive. If
 * ON, then the buttons are set to the state of their corresponding
 * state variable.
 */
static void set_button_sensitivity(Boolean state)
{
	if (state)
	{
		XtSetSensitive(memoryManager, memoryManagerState);
		XtSetSensitive(setBtn, setBtnState);
		XtSetSensitive(deleteBtn, deleteBtnState);
	}
	else
	{
		XtSetSensitive(memoryManager, False);
		XtSetSensitive(setBtn, False);
		XtSetSensitive(deleteBtn, False);
	}
}



/* Right mouse button context menu function. See the ActivateSelectContextMenu
 * function in panel_fieldEdit.c
 */
static void action_command (CONTEXT *info, CAL cal)
{
	if(info->selected_item->btn_id == setBtnId)
	{
		ACTIVATE_areaAttributesDialog(panel, cal? cal:mem->active_cal, set_active_cal_value, store_in_memory);
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



/*  Responds to the edit status commands sent by Ingred.  The command
 *  string is EDIT AREA SELECT|DESELECT [SET].
 */
static void status_from_ingred( CAL cal, String *parms, int nparms )
{
	String key;

	if(!GV_edit_mode) return;
	if(nparms < 3) return;
	if(!same_ic(parms[0],E_EDIT)) return;
	if(!same_ic(parms[1],E_AREA)) return;
	if(!IsActiveEditor(DISCRETE_FIELD_EDITOR)) return;

	key = parms[2];
	modify_cal = NULL;

	if(same_ic(key, E_SELECT))
	{
		if(InEditMode(E_DRAW))
		{
			memoryManagerState = ON;
		}
		else if(InEditMode(E_MOVE))
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
				if(GV_active_field->memory->attrib_editing)
				{
					setBtnState = ON;
					ActivateSelectContextMenu(&divide_set_context, action_command, cal);
					if(IsActiveEditor(DISCRETE_FIELD_EDITOR))
						UpdateAttributesEntryDialog(cal);
				}
				else
				{
					setBtnState = OFF;
					ActivateSelectContextMenu(&divide_noset_context, action_command, cal);
				}
				memoryManagerState = ON;
				modify_cal = cal;
			}
			else
			{
				setBtnState = OFF;
				memoryManagerState = OFF;
				DeactivateSelectContextMenu();
			}
			ShowContextMenuAttributes(True);
		}
		else if(InEditMode(E_MODIFY))
		{
			if(same_ic(parms[3],E_SET))
			{
				if(GV_active_field->memory->attrib_editing)
				{
					modify_cal = cal;
					ActivateSelectContextMenu(&modify_context, action_command, cal);
					if(IsActiveEditor(DISCRETE_FIELD_EDITOR))
						UpdateAttributesEntryDialog(cal);
					setBtnState = ON;
				}
				else
				{
					UpdateAttributesEntryDialog(GV_active_field->memory->active_cal);
					ActivateSelectContextMenu(&delete_context, action_command, NULL);
					setBtnState = OFF;
				}
				memoryManagerState = ON;
				ShowContextMenuAttributes(True);
			}
			else if(same_ic(parms[3], E_HOLE))
			{
				UpdateAttributesEntryDialog(GV_active_field->memory->active_cal);
				ActivateSelectContextMenu(&delete_hole_context, action_command, NULL);
				setBtnState = OFF;
				memoryManagerState = OFF;
				ShowContextMenuAttributes(False);

			}
			else
			{
				setBtnState = OFF;
				memoryManagerState = OFF;
				ActivateSelectContextMenu(&delete_context, action_command, NULL);
				ShowContextMenuAttributes(False);
			}
		}
		else
		{
			ShowContextMenuAttributes(InEditMode(E_DRAW));
			DeactivateSelectContextMenu();
		}
		set_button_sensitivity(ON);
	}
	else
	{
		set_button_sensitivity(OFF);
		DestroyAttributesEntryDialog();
		ShowContextMenuAttributes(InEditMode(E_DRAW));
		DeactivateSelectContextMenu();
		if(InEditMode(E_MODIFY) || InEditMode(E_DIVIDE))
		{
			setBtnState =  same_ic(parms[3],E_SET);
			memoryManagerState =  same_ic(parms[3],E_SET);
		}
		else
		{
			setBtnState = (InEditMode(E_DRAW) && GV_active_field->memory->attrib_editing);
			memoryManagerState = InEditMode(E_DRAW);
		}
	}
}


/*ARGSUSED*/
static void context_cb( int ndx )
{
	set_active_cal_value(mem->cal[ndx]);
	/* 
	 * 2007.09.05 The above function will not send anything if we are not in
	 * draw mode so the memory button selection needs to send a command in
	 * this case.
	 */
	if (!InEditMode(E_DRAW)) send_edit_cmd(SET_MODE);
	if(mem->attrib_editing) UpdateAttributesEntryDialog(mem->active_cal);
}


/*=======================================================================*/
/*
*	entry() - Called by the field editor when any discrete field is
*	activated.  The panel information is retrieved from the field data
*	structure and the discrete data for that panel is inserted into the
*	display text and into the selection buttons.
*/
/*=======================================================================*/
static void entry(void)
{
	int      i;
	String   display;
	Boolean  draw;

	if(!IsActiveEditor(DISCRETE_FIELD_EDITOR)) return;

	mem = GV_active_field->memory;

	InitContextMenuAttributes(GV_active_field->info->sh_label, context_cb);
	XtUnmanageChildren(memBtns, (Cardinal) nmemBtns);
	for(i = 0; i < mem->num; i++)
	{
		String label = CAL_get_attribute(mem->cal[i], CALuserlabel);
		display = LineBreak(label, BREAK);
		XuWidgetLabel(memBtns[i], display);
		FreeItem(display);
		AddToContextMenuAttributes(label);
	}
	XtManageChildren(memBtns, (Cardinal) mem->num);

	XmTextSetString(labelDisplay, CAL_get_attribute(mem->active_cal, CALuserlabel));
	XmTextSetString(bkgndDisplay, CAL_get_attribute(GV_active_field->bkgnd_cal, CALuserlabel));

	draw = InEditMode(E_DRAW);
	setBtnState = (mem->attrib_editing && draw);
	memoryManagerState = (mem->attrib_editing && draw);
	deleteBtnState = (mem->attrib_editing && mem->num > mem->nfixed);
	XtSetSensitive(setBkgnd, mem->bkgnd_editing);

	set_button_sensitivity(ON);

	show_panel(True);
}


/* Callback for ConfigureMainContextMenuAuxPushButton */
/*ARGSUSED*/
static void aux_push_button_cb(XtPointer unused)
{
	ACTIVATE_areaAttributesDialog(panel, mem->active_cal, set_active_cal_value, store_in_memory);
}


/*=======================================================================*/
/*
*	edit_cmd_action() - To be called by the edit command function whenever
*   an edit command button is pushed.
*/
/*=======================================================================*/
static void edit_cmd_action(String cmd )
{
	Manage(panel, same(cmd,E_DRAW) || same(cmd,E_MODIFY) || same(cmd,E_DIVIDE));
	DeactivateSelectContextMenu();

	if(same_ic(cmd,E_DRAW))
	{
		setBtnState =  mem->attrib_editing;
		memoryManagerState =  mem->attrib_editing;
		if (mem->attrib_editing)
			ConfigureMainContextMenuAuxPushButton("setBtn", aux_push_button_cb, NULL);
		ShowContextMenuAttributes(True);
	}
	else
	{
		setBtnState = False;
		memoryManagerState = False;
		ReleaseMainContextMenuAuxPushButton();
		ShowContextMenuAttributes(False);
	}
	set_button_sensitivity(ON);
}


/*=========================================================================*/
/*
*	send_edit_cmd() - Sends editor commands to the graphics editor.
*
* Commands:	The following is a list of the recoginzed commands and a
*			description of what action is taken upon a call to this routine.
*
*			ACCEPT_MODE - Post edit.
*			SET_MODE    - Inserts the SET option into the appropriate command.
*           STACK_MODE  - Inserts the STACK option.
*/
/*=========================================================================*/
static void send_edit_cmd(int cmd)
{
	char  txbuf[100];

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
		strcpy(txbuf, "EDIT ");
		strcat(txbuf, GV_active_field->editor->active->cmd);

		if( InEditMode(E_DIVIDE) )
		{
			if( cmd == SET_MODE ) strcat(txbuf, " SET");
		}
		else if( InEditMode(E_MODIFY) )
		{
			if( cmd == SET_MODE )
			{
				strcat(txbuf, " SET");
			}
			else if( cmd == STACK_MODE )
			{
				strcat(txbuf, " STACK ");
				strcat(txbuf, GV_stacking_order);
			}
		}
		else if( InEditMode(E_LABEL) )
		{
			strcat(txbuf, " LABEL");
		}
		(void) IngredEditCommand(txbuf, GV_active_field->cal, NullCal);
	}
}


/*=======================================================================*/
/*
*	destroy_data() - Frees any resources assigned to the field by this
*	editor.
*/
/*=======================================================================*/
static void destroy_data(FIELD_INFO *field )
{
	int i;
	FD_MEM *m = field->memory;
	for(i = 0; i < m->num; i++)
	{
		(void) CAL_destroy(m->cal[i]);
	}
	FreeItem(m->cal);
	FreeItem(m);
}


/*=======================================================================*/
/*
*	show_panel() - Called when a different field has been selected.
*/
/*=======================================================================*/
static void show_panel(Boolean show )
{
	edit_cmd_action(show ? GV_active_field->editor->active->cmd : NULL);

	/* If there is an entry panel for this editor showing we want
	*  to remove it.
	*/
	if(!show && mem->attrib_editing) DestroyAttributesEntryDialog();
}


/*=========================================================================*/
/*
*	show_value_cb() - Shows the value associated with the active memory call
*	while the user is "pushing down" on the button.
*/
/*=========================================================================*/
/*ARGSUSED*/
static void show_value_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	if( ((XmPushButtonCallbackStruct *)call_data)->reason == XmCR_ARM )
	{
		CAL cal = (client_data) ? GV_active_field->bkgnd_cal : mem->active_cal;
		DisplayAttributesPopup(panel, True, cal);
	}
	else
	{
		DisplayAttributesPopup(panel, False, NULL);
	}
}


/*=========================================================================*/
/*
*	launch_bkgnd_entry_dialog_cb() - Activates the background entry dialog.
*/
/*=========================================================================*/
/* ARGSUSED */
static void launch_bkgnd_entry_dialog_cb(Widget w, XtPointer client, XtPointer call)
{
	ACTIVATE_areaBkgndAttributesDialog(panel, GV_active_field->bkgnd_cal, set_bkgnd_cal_value);
}


/*=========================================================================*/
/*
*	launch_entry_dialog_cb() - Activates the entry dialog.
*/
/*=========================================================================*/
/* ARGSUSED */
static void launch_entry_dialog_cb(Widget w, XtPointer client, XtPointer call)
{
	ACTIVATE_areaAttributesDialog(panel, (modify_cal)? modify_cal : mem->active_cal,
			set_active_cal_value, store_in_memory);
}


/*=========================================================================*/
/*
*	memory_select_cb() - Callback function for the memory buttons.
*/
/*=========================================================================*/
/* ARGSUSED */
static void memory_select_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	set_active_cal_value(mem->cal[PTR2INT(client_data)]);
	/* 
	 * 2007.09.05 The above function will not send anything if we are not in
	 * draw mode so the memory button selection needs to send a command in
	 * this case.
	 */
	if (!InEditMode(E_DRAW)) send_edit_cmd(SET_MODE);
	if(mem->attrib_editing) UpdateAttributesEntryDialog(mem->active_cal);
}


static void save_cal(FIELD_INFO *field, int ndx)
{
	int    i, nids, size;
	char   mbuf[20];
	String *ids, key, value, nbuf;
	CAL    cal;

	if(IsNull(field)) return;
	if(blank(field->info->element->name)) return;
	if(blank(field->info->level->name)  ) return;

	if(ndx == SAVE_ACTIVE)
	{
		key = ACTIVEKEY;
		cal = mem->active_cal;
	}
	else
	{
		i = ndx - mem->nfixed;
		if(i < 0) return;
		(void) snprintf(mbuf, sizeof(mbuf), "mem%d", i);
		key = mbuf;
		cal = mem->cal[ndx];
	}
	CAL_get_attribute_names(cal, &ids, &nids);
	if(nids < 1) return;

	/* Assume about 30 characters per attribute */
	size = nids * 30;
	nbuf = NewMem(char,size);
	for(i = 0; i < nids; i++)
	{
		value = CAL_get_attribute(cal, ids[i]);
		if(CAL_is_value(value))
		{
			char *np;
			int  len = (int) safe_strlen(nbuf);
			int  nl  = size - len;
			int  n   = snprintf(&nbuf[len], (size_t) nl, " %s='%s'", ids[i], value);
			/*
			 * if true then our write was successful
			 */
			if(n > -1 && n < nl) continue;
			/* 
			 * print attempt failed so we allocate more memory. Since we are in a
			 * loop allocate an arbitrary increase.
			 */
			if(n > -1) size += (n+1)*(nids-i);		/* glibc 2.1 */
			else       size += 200;					/* glibc 2.0 */
			if((np = MoreMem(nbuf, char, size)) == NULL) {
				free(nbuf);
				return;
			} else {
				nbuf = np;
			}
		}
	}
	if(!blank(nbuf)) XuStateDataSave(FLDNAMES(field), key, nbuf);

	free(ids);
	FreeItem(nbuf);
}


/*=========================================================================*/
/*
*	set_active_cal_value() - Send the entry menu data direct to Ingred and
*                          do not store. After we are done reset the active
*                          field pointers.
*/
/*=========================================================================*/
static void set_active_cal_value(CAL cal)
{
	if (!cal) return;

	(void) CAL_destroy(mem->active_cal);
	mem->active_cal = CAL_duplicate(cal);
	GV_active_field->cal = mem->active_cal;
	save_cal(GV_active_field, SAVE_ACTIVE);
	XmTextSetString(labelDisplay, CAL_get_attribute(mem->active_cal, CALuserlabel));
	/* 2007.09.05 Do not set the area if in modify or divide mode */
	if (InEditMode(E_DRAW))
		send_edit_cmd(SET_MODE);
}


/*=========================================================================*/
/*
*	set_bkgnd_cal_value() - Set the background value of the field.
*/
/*=========================================================================*/
static void set_bkgnd_cal_value(CAL cal)
{
	char mbuf[300];

	if (IsNull(cal)) return;

	(void) CAL_destroy(GV_active_field->bkgnd_cal);
	GV_active_field->bkgnd_cal = CAL_duplicate(cal);
	XmTextSetString(bkgndDisplay, CAL_get_attribute(GV_active_field->bkgnd_cal, CALuserlabel));

	(void) snprintf(mbuf, sizeof(mbuf), "BACKGROUND %s %s", FLDNAMES(GV_active_field));
	(void) IngredEditCommand(mbuf, GV_active_field->bkgnd_cal, NullCal);
}


/*=========================================================================*/
/*
*	set_bkgnd_from_ingred_cmd() - Function called from main.c when Ingred
*   sends a status report on the background. If cal is null
*   then Ingred is asking for the background to be set by the interface.
*   Any other value means that the background must be set in the interface
*   to agree with what Ingred has sent.
*/
/*=========================================================================*/
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
		XmTextSetString(bkgndDisplay, CAL_get_attribute(fld->bkgnd_cal, CALuserlabel));
	}
}


/*=========================================================================*/
/*
*	store_in_memory() - Stores the given display string, value string and
*	sub-element into a memory button and stores the data in the interface
*	state file. This is done only while in draw mode.
*/
/*=========================================================================*/
static void store_in_memory(CAL cal)
{
	String store, line;

	if (IsNull(cal)) return;

	/* Create another memory button if needed */
	if( mem->num >= nmemBtns )
	{
		nmemBtns++;
		memBtns = MoreWidgetArray(memBtns, nmemBtns);
		memBtns[mem->num] = XmCreatePushButton(memoryManager, "wxMem", NULL, 0);
		XtAddCallback(memBtns[mem->num], XmNactivateCallback, memory_select_cb, INT2PTR(mem->num));
	}
	XtManageChild(memBtns[mem->num]);

	/* Assign more memory to the discrete memory array if required */
	if( mem->num >= mem->total )
	{
		mem->total++;
		mem->cal = MoreMem(mem->cal, CAL, mem->total);
	}
	mem->cal[mem->num] = CAL_duplicate(cal);

	/* Put the memory information into the button */
	line = CAL_get_attribute(cal, CALuserlabel);
	store = LineBreak(blank(line) ? " ":line, BREAK);
	XuWidgetLabel(memBtns[mem->num], store);
	FreeItem(store);

	/* Add it to our context attribute list */
	AddToContextMenuAttributes(line);

	if(initialized)
	{
		save_cal(GV_active_field, mem->num);
		XuVaStateDataSave(FLDNAMES(GV_active_field), NMEMKEY, "%d",mem->num+1-mem->nfixed);
	}

	mem->num++;
	deleteBtnState = (mem->attrib_editing && mem->num > mem->nfixed);
	set_button_sensitivity(ON);
}


static Widget deleteMemDialog = NullWidget;


/*=========================================================================*/
/*
*	DeleteMemoryCB() - Deletes the selected memory locations.
*/
/*=========================================================================*/
/* ARGSUSED */
static void DeleteMemoryCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	int      i, n, *poslist, lenlist, ndx;
	char     mbuf[10];
	XmString label;

	w = *((Widget*)client_data);
	if(XmListGetSelectedPos(w, &poslist, &lenlist))
	{
		for(n = lenlist-1; n >= 0; n--)
		{
			ndx = poslist[n] - 1 + mem->nfixed;
			(void) CAL_destroy(mem->cal[ndx]);
			for( i = ndx; i < mem->num - 1; i++ ) 
			{
				mem->cal[i] = mem->cal[i+1];
				XtVaGetValues( memBtns[i+1], XmNlabelString, &label, NULL );
				XtVaSetValues( memBtns[i],   XmNlabelString, label,  NULL );
			}
			mem->num--;
			XtUnmanageChild(memBtns[mem->num]);
		}
		FreeItem(poslist);
		InitContextMenuAttributes(GV_active_field->info->sh_label, context_cb);
		for(i = 0; i < mem->num; i++)
		{
			save_cal(GV_active_field, i);
			AddToContextMenuAttributes(CAL_get_attribute(mem->cal[i], CALuserlabel));
		}
		(void) snprintf(mbuf, sizeof(mbuf), "mem%d",mem->num - mem->nfixed);
		XuStateDataRemove(FLDNAMES(GV_active_field), mbuf);
	}
	XuVaStateDataSave(FLDNAMES(GV_active_field), NMEMKEY, "%d",mem->num-mem->nfixed);
	XuDestroyDialog(deleteMemDialog);
	deleteMemDialog = NullWidget;
	deleteBtnState =  (mem->attrib_editing && mem->num > mem->nfixed);
	set_button_sensitivity(ON);
}


/*=========================================================================*/
/*
*	LaunchDeleteMemoryDialog() - Creates and launches the dialog for the
*                                selection and deletion of memory locations.
*/
/*=========================================================================*/
static void LaunchDeleteMemoryDialog(Widget w)
{
	int      i;
	String   attrib;
	XmString item;
	static   Widget   list;

	static XuDialogActionsStruct action_items[] = {
		{ "deleteBtn", DeleteMemoryCB,    &list },
		{ "cancelBtn", XuDestroyDialogCB, NULL  }
	};

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

	for(i = mem->nfixed; i < mem->num; i++)
	{
		attrib = CAL_get_attribute(mem->cal[i], CALuserlabel);
		item = XuNewXmString(blank(attrib) ? "-?-":attrib);
		XmListAddItem(list, item, 0);
		XmStringFree(item);
	}
	XuShowDialog(deleteMemDialog);
}


/*=========================================================================*/
/*
*	delete_active_data_cb() - Located within the delete button callback,
*	this deletes memory locations.
*/
/*=========================================================================*/
/* ARGSUSED */
static void delete_active_data_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	if( IsNull(deleteMemDialog) && mem->num > mem->nfixed )
		LaunchDeleteMemoryDialog(w);
}
