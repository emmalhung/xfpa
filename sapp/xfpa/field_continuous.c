/*****************************************************************************
*
*  File:     field_continuous.c
*
*  Purpose:  Provides the program interface panel for any field of type
*			 "continuous".
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

#include <string.h>
#include "global.h"
#include <ingred.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/Scale.h>
#include <Xm/ToggleB.h>
#include "editor.h"
#include "observer.h"
#include "contextMenu.h"

#define NTICKS	5
#define MEM		GV_active_field->memory

/* Local functions */
static void create_flatness_btns (String*, int);
static void depiction_changed    (void);
static void destroy_data         (FIELD_INFO *);
static void edit_command_action  (String);
static void entry                (void);
static void flatness_cb          (Widget, XtPointer, XtPointer);
static void send_edit_command    (int);
static void show_panel           (Boolean);
static void status_from_ingred   (CAL, String*, int);
static void value_cb             (Widget, XtPointer, XtPointer);

typedef struct _memory {
	float    value;
	int      poke_range;
	XmString ticks[NTICKS];
	XmString delta_label;
	float    smoothing_value;
} CONTMEM;

static Widget  scalePanel = NullWidget;
static Widget  deltaScale = NullWidget;
static Widget  scaleTitle = NullWidget;
static Widget  ticks[NTICKS];
static Widget  flatness_label = NullWidget;
static Widget  flatness_btns  = NullWidget;
static String  flatness_values[] = { "100","75","50","25" };
static String  flatness_select;

static CONTEXT move_context = {
	E_AREA, 0, 3,
	{
		{translateBtnId, E_TRANSLATE},
		{rotateBtnId, E_ROTATE},
		{copyBtnId, E_COPY}
	}
};

static CONTEXT merge_context = {
	E_MERGE, 0, 3,
	{
		{mergeBtnId, E_MERGE},
		{translateBtnId, E_TRANSLATE},
		{rotateBtnId, E_ROTATE}
	}
};



/*=======================================================================*/
/*
*	Create the panel.
*/
/*=======================================================================*/
void CreateContinuousFieldPanel(Widget parent, Widget topAttach )
{
	int	     i;
	Widget   w;
	XmString label = XuNewXmString("");

	/* Create the panel that allows us to set the amount of any poke
	 * type of command and to set the effective shape of the stomp
	 * command.
	 */
	scalePanel = XmVaCreateForm(parent, "continuousFieldPanel",
		XmNhorizontalSpacing, 10,
		XmNverticalSpacing, 10,
		XmNborderWidth, 0,
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

	scaleTitle = XmVaCreateManagedLabel(scalePanel, "scaleTitle",
		XmNlabelString, label,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	deltaScale = XmVaCreateManagedScale(scalePanel, "deltaScale",
		XmNtitleString, label,
		XmNborderWidth, 0,
		XmNdecimalPoints, 1,
		XmNshowValue, True,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 40,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, scaleTitle,
		XmNtopOffset, 0,
		NULL);
	XtAddCallback(deltaScale, XmNvalueChangedCallback, value_cb, NULL);

	for( i = 0; i < NTICKS; i++ )
	{
		ticks[i] = XmVaCreateManagedLabel( deltaScale, "tick", NULL);
	}
	XmStringFree(label);

	flatness_label = XmVaCreateManagedLabel(scalePanel, "flatnessTitle",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, deltaScale,
		XmNtopOffset, 20,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	flatness_select = flatness_values[0];

	AddObserver(OB_MAIN_INITIALIZED, create_flatness_btns);
	AddIngredObserver(status_from_ingred);
}


/*=======================================================================*/
/*
*	Add a field of type continuous.
*/
/*=======================================================================*/
void AddContinuousField(FIELD_INFO *field )
{
	int    i, range;
	float  del;
	CONTMEM *memory;
	FpaConfigUnitStruct *units;
	FpaConfigElementEditorStruct *editor;

	memory = OneMem(CONTMEM);
	memory->value = 0.0;
	memory->smoothing_value = 0.0;

	editor = GV_edit_mode ? field->info->element->elem_detail->editor : NULL;
	if(editor && GV_edit_mode)
	{
		memory->poke_range = (int)editor->type.continuous->poke;
		units = editor->type.continuous->units;
		del = (float)((NTICKS-1)/2);
		for( i = 0; i < NTICKS; i++ )
		{
			range = (int)((float)memory->poke_range * (del-i) / del);
			memory->ticks[i] = XuNewXmStringFmt("%d", range);
		}
		memory->delta_label = XuNewXmStringFmt("%s  (%s)",XuGetLabel("delta"),units->name);
	}

	field->entryFcn       = entry;
	field->showFcn        = show_panel;
	field->sendEditCmdFcn = send_edit_command;
	field->changeEditFcn  = edit_command_action;
	field->depictFcn      = depiction_changed;
	field->setBkgndFcn    = NULL;
	field->destroyFcn     = destroy_data;
	field->memory         = memory;
	field->editor         = GetEditor((editor) ? CONTINUOUS_FIELD_EDITOR:CONTINUOUS_FIELD_NO_EDIT);
	field->cal            = NullCal;
	field->bkgnd_cal      = NullCal;

	SetFieldSampleInfo(field, FpaCsampleControlValueType, NULL);
}


/* ================= Local Functions ========================*/


/*
*	Called when the field is activated.  Note that there seems
*	to be a bug in the vertical orentation of the scale widget such that
*	the height must be reset everytime the values are changed.
*/
static void entry(void)
{
	int       i;
	Dimension height;

	edit_command_action(NULL);

	if(!IsActiveEditor(CONTINUOUS_FIELD_EDITOR)) return;

	/* Set the range delta elements
	 */
	XtVaGetValues(deltaScale, XmNheight, &height, NULL);
	XtVaSetValues(scaleTitle, XmNlabelString, MEM->delta_label, NULL);

	for( i = 0; i < NTICKS; i++ )
	{
		XtVaSetValues(ticks[i],
			XmNlabelString, MEM->ticks[i],
			NULL);
	}

	XtVaSetValues(deltaScale,
		XmNmaximum, MEM->poke_range*10,
		XmNminimum, -(MEM->poke_range*10),
		XmNvalue, (int)(MEM->value*10),
		XmNheight, height,
		NULL);

	SetSmoothingValue(&MEM->smoothing_value);

	/* Spline fields do not use displayed attributes so ensure that
	 * the attribute buttons are turned off.
	 */
	ShowContextMenuAttributes(False);
}


/*
*	Show the continuous field panel.
*/
static void show_panel(Boolean show )
{
	edit_command_action(show ? GV_active_field->editor->active->cmd : "");
}


/*
*	Called by the edit command function whenever an edit command button
*   is pushed.
*/
static void edit_command_action(String cmd )
{
	static String last_edit_cmd = NULL;

	if (!cmd)        cmd = last_edit_cmd;
	if (!blank(cmd)) last_edit_cmd = cmd;

	Manage(flatness_label, same(cmd,E_STOMP));
	Manage(flatness_btns,  same(cmd,E_STOMP));
	Manage(scalePanel, same(cmd,E_POKE) || same(cmd,E_STOMP));
}


/*
*	Called when the active depiction changes.
*/
static void depiction_changed(void)
{
	edit_command_action(NULL);
}


/*
*	send_edit_command() - Sends editor commands to the graphics editor.
*/
/*ARGSUSED*/
static void send_edit_command(int mode)
{
	char  txbuf[100];

	if(!ValidEditField()) return;

	
	if(InEditMode(E_SAMPLE))
	{
		SendEditSampleCommand(NULL);
	}
	else if(InEditMode(E_LABEL))
	{
		SendEditLabelCommand(NULL);
	}
	else if(InEditMode(E_POKE))
	{
		(void) snprintf(txbuf, sizeof(txbuf), "EDIT %s %.1f",
						GV_active_field->editor->active->cmd, MEM->value);
		(void) IngredEditCommand(txbuf, GV_active_field->cal, NullCal);
	}
	else if(InEditMode(E_STOMP))
	{
		(void) snprintf(txbuf, sizeof(txbuf), "EDIT %s %.1f %s",
						GV_active_field->editor->active->cmd, MEM->value, flatness_select);
		(void) IngredEditCommand(txbuf, GV_active_field->cal, NullCal);
	}
	else
	{
		(void) snprintf(txbuf, sizeof(txbuf), "EDIT %s", GV_active_field->editor->active->cmd);
		(void) IngredEditCommand(txbuf, GV_active_field->cal, NullCal);
	}
}


/*
*	Function to free all memory assigned to the	field structure by
*   this editor.
*/
static void destroy_data(FIELD_INFO *field )
{
	int i;
	CONTMEM *mem = field->memory;
	for( i = 0; i < 4; i++ )
		XmStringFree(mem->ticks[i]);
	XmStringFree(mem->delta_label);
	FreeItem(mem);
}


/*
*   Transmits a change in scale to Ingred.
*/
/*ARGSUSED*/
static void value_cb(Widget  w , XtPointer client_data , XtPointer call_data )
{
	MEM->value = (float)((XmScaleCallbackStruct *) call_data)->value/10.0;
	send_edit_command(ACCEPT_MODE);
}


/*
*   Transmits a change in flatness to Ingred.
*/
/*ARGSUSED*/
static void flatness_cb(Widget  w , XtPointer client_data , XtPointer call_data )
{
	if(!XmToggleButtonGetState(w)) return;
	flatness_select = (String)client_data;
	send_edit_command(ACCEPT_MODE);
}


/* The pixmaps for the flatness buttons must be created after the windows
 * come into existance. Thus the button creation is delayed until after
 * main initialization.
 */
/*ARGSUSED*/
static void create_flatness_btns(String *unused, int notused)
{
	int i;

	flatness_btns = XmVaCreateRowColumn(scalePanel, "fb",
		XmNorientation, XmHORIZONTAL,
		XmNradioBehavior, True,
		XmNspacing, 3,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, flatness_label,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	for(i = 0; i < (int) XtNumber(flatness_values); i++)
	{
		Widget w;
		Pixmap lpx, spx, ipx;

		MakeTaperPixmaps(scalePanel, 30, 30, flatness_values[i], &lpx, &spx, &ipx);

		w = XmVaCreateManagedToggleButton(flatness_btns, flatness_values[i],
			XmNshadowThickness, 2,
			XmNindicatorOn, False,
			XmNlabelType, XmPIXMAP,
			XmNlabelPixmap, lpx,
			XmNselectPixmap, spx,
			XmNselectInsensitivePixmap, ipx,
			XmNmarginHeight, 0,
			XmNmarginWidth, 0,
			XmNset, same(flatness_select, flatness_values[i])? XmSET:XmUNSET,
			NULL);

		XtAddCallback(w, XmNvalueChangedCallback, flatness_cb, (XtPointer)flatness_values[i]);
	}
	XtManageChild(flatness_btns);

	DeleteObserver(OB_MAIN_INITIALIZED, create_flatness_btns);
}


/* Right mouse button context menu functions. See the
 * ActivateSelectContextMenu function in panel_fieldEdit.c
 */
static void action_command (CONTEXT *info, CAL cal)
{
	IngredVaEditCommand(cal, NullCal, "%s %s %s", E_EDIT, info->edit_mode, info->selected_item->edit_cmd);
}



/*ARGSUSED*/
static void status_from_ingred(CAL cal, String *parms, int nparms)
{
	if(!GV_edit_mode) return;
	if(!same_ic(parms[0],E_EDIT)) return;
	if(!same_ic(parms[1],E_CONTOUR)) return;
	if(!IsActiveEditor(CONTINUOUS_FIELD_EDITOR)) return;

	if(same_ic(parms[2],E_SELECT))
	{
		if(InEditMode(E_AREA))
		{
			ActivateSelectContextMenu(&move_context, action_command, NULL);
		}
		else if(InEditMode(E_MERGE))
		{
			ActivateSelectContextMenu(&merge_context, action_command, NULL);
		}
		SetContextPasteSelectAllButtonsState(SELECT_ALL_BUTTON_OFF);
	}
	else
	{
		DeactivateSelectContextMenu();
	}
}
