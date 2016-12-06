/*****************************************************************************
*
*  File:     field_vector.c
*
*  Purpose:  Provides the program interface panel for any field of type
*			 "vector".
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
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/Scale.h>
#include <Xm/ToggleB.h>
#include <ingred.h>
#include "editor.h"
#include "observer.h"
#include "contextMenu.h"

#define NTICKS	5
#define MEM		GV_active_field->memory

/* Local functions
*/
static void depiction_changed   (void);
static void edit_command_action	(String);
static void send_edit_command   (int);
static void entry			    (void);
static void set_select          (Widget, Widget, Boolean);
static void show_panel		    (Boolean);
static void status_from_ingred  (CAL, String*, int);
static void destroy_data		(FIELD_INFO *);

typedef struct _memory {
	struct {
		Boolean  selected;
		float    value;
		int      poke_range;
		String   units_label;
	} mag;
	struct {
		Boolean  selected;
		float    value;
		int      poke_range;
		String   units_label;
	} dir;
	float smoothing_value;
} VECMEM;


static Widget  vectorPanel = NullWidget;
static Widget  magSelect,        dirSelect;
static Widget  magLabel,         dirLabel;
static Widget  magScale,         dirScale;
static Widget  magTicks[NTICKS], dirTicks[NTICKS];
static Widget  flatness_label;
static Widget  flatness_btns;
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
*	Add a field of type vector.
*/
/*=======================================================================*/
void AddVectorField(FIELD_INFO *field )
{
	VECMEM *mem;
	FpaConfigElementEditorStruct *editor;

	mem = OneMem(VECMEM);
	mem->mag.value    = 0.0;
	mem->dir.value    = 0.0;
	mem->mag.selected = True;
	mem->dir.selected = False;
	mem->smoothing_value = 0.0;


	editor = GV_edit_mode ? field->info->element->elem_detail->editor : NULL;
	if(editor && GV_edit_mode)
	{
		mem->mag.units_label = editor->type.vector->mag_units->sh_label;
		mem->mag.poke_range  = (int)editor->type.vector->mag_poke;

		mem->dir.units_label = editor->type.vector->dir_units->sh_label;
		mem->dir.poke_range  = (int)editor->type.vector->dir_poke;
	}

	field->entryFcn       = entry;
	field->showFcn        = show_panel;
	field->sendEditCmdFcn = send_edit_command;
	field->changeEditFcn  = edit_command_action;
	field->depictFcn      = depiction_changed;
	field->setBkgndFcn    = NULL;
	field->destroyFcn     = destroy_data;
	field->memory         = mem;
	field->editor         = GetEditor((editor) ? VECTOR_FIELD_EDITOR:VECTOR_FIELD_NO_EDIT);
	field->cal            = NullCal;
	field->bkgnd_cal      = NullCal;

	SetFieldSampleInfo(field, FpaCsampleControlValueType, NULL);
}


/*=======================================================================*/
/*
*	Called when the field is activated.  Note that there seems
*	to be a bug in the vertical orentation of the scale widget such that
*	the height must be reset everytime the values are changed.
*/
/*=======================================================================*/
static void entry(void)
{
	int       i;
	float     del;
	Dimension mag_height, dir_height;

	edit_command_action(NULL);

	if(GV_active_field->editor == GetEditor(VECTOR_FIELD_NO_EDIT)) return;

	XtVaGetValues(magScale, XmNheight, &mag_height, NULL);
	set_select(magSelect, magScale, MEM->mag.selected);
	XuWidgetLabel(magLabel, MEM->mag.units_label);

	XtVaGetValues(dirScale, XmNheight, &dir_height, NULL);
	set_select(dirSelect, dirScale, MEM->dir.selected);
	XuWidgetLabel(dirLabel, MEM->dir.units_label);

	del = (float)((NTICKS-1)/2);
	for( i = 0; i < NTICKS; i++ )
	{
		XuWidgetPrint(magTicks[i], "%d", (int)((float)MEM->mag.poke_range * (del-i) / del));
		XuWidgetPrint(dirTicks[i], "%d", (int)((float)MEM->dir.poke_range * (del-i) / del));
	}

	XtVaSetValues(magScale,
		XmNmaximum, MEM->mag.poke_range*10,
		XmNminimum, -(MEM->mag.poke_range*10),
		XmNvalue, (int)(MEM->mag.value*10),
		XmNheight, mag_height,
		NULL);

	XtVaSetValues(dirScale,
		XmNmaximum, MEM->dir.poke_range*10,
		XmNminimum, -(MEM->dir.poke_range*10),
		XmNvalue, (int)(MEM->dir.value*10),
		XmNheight, dir_height,
		NULL);

	SetSmoothingValue(&MEM->smoothing_value);

	/* Spline fields do not use displayed attributes so ensure that
	 * the attribute buttons are turned off.
	 */
	ShowContextMenuAttributes(False);
}


/*=======================================================================*/
/*
*	Show the vector field panel.
*/
/*=======================================================================*/
static void show_panel(Boolean show )
{
	edit_command_action(show ? GV_active_field->editor->active->cmd : "");
}


/*=======================================================================*/
/*
*	Called by the edit command function whenever an edit command button
*   is pushed.
*/
/*=======================================================================*/
static void edit_command_action(String cmd )
{
	static String last_edit_cmd = NULL;

	if (!cmd)        cmd = last_edit_cmd;
	if (!blank(cmd)) last_edit_cmd = cmd;

	if(same(cmd,E_POKE))
	{
		XtUnmanageChild(flatness_btns);
		XtUnmanageChild(flatness_label);
		XtManageChild(vectorPanel);
	}
	else if(same(cmd,E_STOMP))
	{
		XtManageChild(flatness_label);
		XtManageChild(flatness_btns);
		XtManageChild(vectorPanel);
	}
	else
	{
		XtUnmanageChild(vectorPanel);
		XtUnmanageChild(flatness_btns);
		XtUnmanageChild(flatness_label);
	}
}


/*=======================================================================*/
/*
*	Called when the active depiction changes.
*/
/*=======================================================================*/
static void depiction_changed(void)
{
	edit_command_action(NULL);
}


/*=========================================================================*/
/*
*	send_edit_command() - Sends editor commands to the graphics editor.
*/
/*=========================================================================*/
/*ARGSUSED*/
static void send_edit_command(int mode)
{
	char txbuf[100];

	if(!ValidEditField()) return;

	if(InEditMode(E_SAMPLE))
	{
		SendEditSampleCommand(NULL);
	}
	else if(InEditMode(E_LABEL))
	{
		SendEditLabelCommand(NULL);
	}
	else if(InEditMode(E_POKE) || InEditMode(E_STOMP))
	{
		if (MEM->mag.selected || MEM->dir.selected)
		{
			(void) snprintf(txbuf, sizeof(txbuf), "%s %s %.1f %.1f %s", E_EDIT, GV_active_field->editor->active->cmd,
							(MEM->mag.selected)? MEM->mag.value:0.0,
							(MEM->dir.selected)? MEM->dir.value:0.0,
							InEditMode(E_STOMP)? flatness_select:"");
			(void) IngredEditCommand(txbuf, GV_active_field->cal, NullCal);
		}
	}
	else
	{
		(void) snprintf(txbuf, sizeof(txbuf), "%s %s", E_EDIT, GV_active_field->editor->active->cmd);
		(void) IngredEditCommand(txbuf, GV_active_field->cal, NullCal);
	}
}


/*=======================================================================*/
/*
*	Function to free all m assigned to the	field structure by
*   this editor.
*/
/*=======================================================================*/
static void destroy_data(FIELD_INFO *field )
{
	FreeItem(field->memory->mag.units_label);
	FreeItem(field->memory->dir.units_label);
	FreeItem(field->memory);
}


static void set_select(Widget wb, Widget ws, Boolean set)
{
	Pixel pf, pb;
	if(set)
	{
		pf = XuLoadColorResource(wb, "*.vectorFieldPanel.selectFg","black");
		pb = XuLoadColorResource(wb, "*.vectorFieldPanel.selectBg","green");
		XtVaSetValues(wb, XmNforeground, pf, XmNbackground, pb, NULL);
	}
	else
	{
		XtVaGetValues(vectorPanel, XmNforeground, &pf, XmNbackground, &pb, NULL);
		XtVaSetValues(wb, XmNforeground, pf, XmNbackground, pb, NULL);
	}
	XtSetSensitive(ws, set);
}


/*ARGSUSED*/
static void magnitude_select_cb(Widget  w , XtPointer client_data , XtPointer call_data )
{
	MEM->mag.selected = XmToggleButtonGetState(w);
	set_select(magSelect, magScale, MEM->mag.selected);
	if(!MEM->mag.selected && !MEM->dir.selected)
	{
		MEM->dir.selected = True;
		XuToggleButtonSet(dirSelect, True, False);
		set_select(dirSelect, dirScale, True);
	}
	send_edit_command(ACCEPT_MODE);
}


/*ARGSUSED*/
static void magnitude_cb(Widget  w , XtPointer client_data , XtPointer call_data )
{
	MEM->mag.value = (float)((XmScaleCallbackStruct *) call_data)->value/10.0;
	send_edit_command(ACCEPT_MODE);
}


/*ARGSUSED*/
static void direction_select_cb(Widget  w , XtPointer client_data , XtPointer call_data )
{
	MEM->dir.selected = XmToggleButtonGetState(w);
	set_select(dirSelect, dirScale, MEM->dir.selected);
	if(!MEM->dir.selected && !MEM->mag.selected)
	{
		MEM->mag.selected = True;
		XuToggleButtonSet(magSelect, True, False);
		set_select(magSelect, magScale, True);
	}
	send_edit_command(ACCEPT_MODE);
}


/*ARGSUSED*/
static void direction_cb(Widget  w , XtPointer client_data , XtPointer call_data )
{
	MEM->dir.value = (float)((XmScaleCallbackStruct *) call_data)->value/10.0;
	send_edit_command(ACCEPT_MODE);
}


/*=======================================================================*/
/*
*   Transmits a change in flatness to Ingred.
*/
/*=======================================================================*/
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

	flatness_btns = XmVaCreateRowColumn(vectorPanel, "fb",
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

		MakeTaperPixmaps(vectorPanel, 30, 30, flatness_values[i], &lpx, &spx, &ipx);

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


/*=======================================================================*/
/*
*	Create the vectorPanel.
*/
/*=======================================================================*/
void CreateVectorFieldPanel(Widget parent, Widget topAttach )
{
	int	 i;
	Widget w, mag, dir;
	XmString label = XuNewXmString("");

	vectorPanel = XmVaCreateForm(parent, "vectorFieldPanel",
		XmNhorizontalSpacing, 3,
		XmNverticalSpacing, 5,
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

	mag = XmVaCreateManagedForm(vectorPanel, "magForm",
		XmNhorizontalSpacing, 5,
		XmNverticalSpacing, 5,
		XmNborderWidth, 1,
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		NULL);

	magSelect = XmVaCreateManagedToggleButton(mag, "magSelect",
		XmNshadowThickness, 2,
		XmNindicatorOn, False,
		XmNmarginWidth, 3,
		XmNset, XmSET,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(magSelect, XmNvalueChangedCallback, magnitude_select_cb, NULL);

	magLabel = XmVaCreateManagedLabel(mag, "ml",
		XmNlabelString, label,
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, magSelect,
		XmNtopOffset, 15,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	magScale = XmVaCreateManagedScale(mag, "deltaScale",
		XmNtitleString, label,
		XmNdecimalPoints, 1,
		XmNshowValue, True,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, magLabel,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(magScale, XmNvalueChangedCallback, magnitude_cb, NULL);

	dir = XmVaCreateManagedForm(vectorPanel, "dirForm",
		XmNhorizontalSpacing, 5,
		XmNverticalSpacing, 5,
		XmNborderWidth, 1,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		NULL);

	dirSelect = XmVaCreateManagedToggleButton(dir, "dirSelect",
		XmNshadowThickness, 2,
		XmNindicatorOn, False,
		XmNmarginWidth, 3,
		XmNset, XmUNSET,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(dirSelect, XmNvalueChangedCallback, direction_select_cb, NULL);

	dirLabel = XmVaCreateManagedLabel(dir, "ml",
		XmNlabelString, label,
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, dirSelect,
		XmNtopOffset, 15,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	dirScale = XmVaCreateManagedScale(dir, "deltaScale",
		XmNtitleString, label,
		XmNdecimalPoints, 1,
		XmNshowValue, True,
		XmNsensitive, False,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, dirLabel,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(dirScale, XmNvalueChangedCallback, direction_cb, NULL);

	for( i = 0; i < NTICKS; i++ )
	{
		magTicks[i] = XmVaCreateManagedLabel(magScale, "scaleTicks", XmNlabelString, label, NULL);
		dirTicks[i] = XmVaCreateManagedLabel(dirScale, "scaleTicks", XmNlabelString, label, NULL);
	}
	XmStringFree(label);

	flatness_label = XmVaCreateManagedLabel(vectorPanel, "flatnessTitle",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, mag,
		XmNtopOffset, 20,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	flatness_select = flatness_values[0];

	AddObserver(OB_MAIN_INITIALIZED, create_flatness_btns);
	AddIngredObserver(status_from_ingred);
}



/* Right mouse button context menu functions. See the
 * ActivateSelectContextMenu function in panel_fieldEdit.c
 */
static void action_command (CONTEXT *info, CAL cal)
{
	IngredVaEditCommand(cal, NullCal, "%s %s %s", E_EDIT, info->edit_mode, info->selected_item->edit_cmd);
}



static void status_from_ingred(CAL cal, String *parms, int nparms)
{
	if(!GV_edit_mode || !same_ic(parms[0],E_EDIT) || !same_ic(parms[1],E_VECTOR)) return;

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
