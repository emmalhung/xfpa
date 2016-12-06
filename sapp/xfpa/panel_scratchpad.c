/****************************************************************************/
/*
*  File:	panel_scratchpad.c
*
*  Purpose:	 Provides the controlling logic for the scratchpad.
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

#include "global.h"
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>
#include <Xm/ToggleB.h>
#include <Xm/XmpSpinBox.h>
#include <Xm/Column.h>
#include <graphics.h>
#include "editor.h"
#include "menu.h"
#include "selector.h"
#include "observer.h"
#include "contextMenu.h"

#define STATE_KEY	"spp"

enum { LINE_MODE, TEXT_MODE, MEAS_MODE, DIST_MODE, SELI_MODE };

static String  drawing_modes[] = {DRAWING_MODES};

static Widget drawMode, textEntry, distanceAttrib, distanceSet;
static Widget spinBox, selectAllBtn, deleteBtn, colourSelect, stateBtns[5];

/* right mouse button context menu buttons */
static Widget panelContextMenu = NullWidget;
static Widget cm_deleteBtn, cm_selectAllBtn, cm_separator;

static int     pad_mode            = LINE_MODE;
static int     drawing_mode_ndx    = 0;
static int     preset_distance_max = 100000;
static String  text_font_type      = NULL;
static String  text_font_size      = NULL;
static String  text_font_colour    = NULL;
static String  text_string         = NULL;
static String  line_style          = NULL;
static String  line_colour         = NULL;
static String  distance_font_type  = NULL;
static String  distance_font_size  = NULL;
static String  distance_colour     = NULL;


static void send_edit_cmd     (void);
static void set_context_state (void);


/*======================== LOCAL FUNCTIONS ================================*/


static void save_sp_state(void)
{
	XuVaStateDataSave(STATE_KEY,"all",NULL, "\"%s\" %s \"%s\" \"%s\" %s \"%s\" %s \"%s\" %d %d %d",
		text_font_type, text_font_size, text_font_colour,
		line_colour, line_style,
		distance_font_type, distance_font_size, distance_colour,
		XmpSpinBoxGetValue(spinBox),
		drawing_mode_ndx,
		preset_distance_max,
		NULL);
}


static void main_menu_observer( String *parms, int nparms )
{
	if(nparms < 1) return;
	if(PTR2INT(parms[0]) != MENU_View_scratchpadShow) return;
	if(PanelIsActive(SCRATCHPAD)) return;
	DeactivateMenu();
	(void) IngredCommand(GE_SCRATCHPAD, GetDisplayState(SCRATCHPAD)? "SHOW" : "HIDE");
	ActivateMenu();
}


/* Take action depending on the action message from ingred
 */
static void ingred_message_observer( CAL cal, String *parms, int nparms )
{
	if(!same_ic(parms[0],"SCRATCHPAD")) return;

	if(same_ic(parms[1],E_BUTTON))
	{
		String  btn = parms[2];
		Boolean on  = same_ic(parms[3],E_ON);

		if(same_ic(btn,E_DELETE))
			Manage(cm_deleteBtn, on);
	}
	else if(same_ic(parms[1],E_SELECT))
	{
		int i;
		XtUnmanageChild(cm_selectAllBtn);
		XtUnmanageChild(cm_separator);
		for( i = 0; i < XtNumber(stateBtns); i++ )
		{
			char buf[10];
			(void) snprintf(buf, sizeof(buf), "cb%d", i);
			XtUnmanageChild(XtNameToWidget(panelContextMenu, buf));
		}
	}
	else
	{
		int i;
		set_context_state();
		for( i = 0; i < XtNumber(stateBtns); i++ )
		{
			char buf[10];
			(void) snprintf(buf, sizeof(buf), "cb%d", i);
			XtManageChild(XtNameToWidget(panelContextMenu, buf));
		}
	}
}


/* The following 3 functions create and control the right mouse button panel
 * context menu.
 */
/*ARGSUSED*/
static void context_menu_cb(Widget w, XtPointer client_data, XtPointer unused )
{
	int ndx = PTR2INT(client_data);

	if(ndx < XtNumber(stateBtns))
	{
		XmToggleButtonSetState(stateBtns[ndx], True, True);
	}
	else if(ndx == 100)
	{
		(void) IngredVaCommand(GE_SCRATCHPAD, "EDIT SELECT ALL");
		send_edit_cmd();
	}
	else if (ndx == 101)
	{
		(void) IngredVaCommand(GE_SCRATCHPAD, "EDIT SELECT DELETE");
		send_edit_cmd();
	}
}


static void create_context_menu(void)
{
	int    i;
	Widget btn;

	panelContextMenu = CreatePanelContextMenu("scratchpadContextMenu");

	cm_selectAllBtn = XmVaCreatePushButton(panelContextMenu, "selectAllBtn", NULL);
	XtAddCallback(cm_selectAllBtn, XmNactivateCallback, context_menu_cb, INT2PTR(100));

	cm_deleteBtn = XmVaCreatePushButton(panelContextMenu, "deleteBtn", NULL);
	XtAddCallback(cm_deleteBtn, XmNactivateCallback, context_menu_cb, INT2PTR(101));

	cm_separator = XmCreateSeparator(panelContextMenu, "sep", NULL, 0);

	/* Doing the state buttons this way means that I don't have to worry about resource labels
	 * or changing the code if the number of buttons changes :-)
	 */
	for( i = 0; i < XtNumber(stateBtns); i++ )
	{
		char     buf[10];
		XmString label;
		(void) snprintf(buf, sizeof(buf), "cb%d", i);
		XtVaGetValues(stateBtns[i], XmNlabelString, &label, NULL);
		btn = XmVaCreateManagedPushButton(panelContextMenu, buf, XmNlabelString, label, NULL);
		XtAddCallback(btn, XmNactivateCallback, context_menu_cb, INT2PTR(i));
	}
}


static void set_context_state(void)
{
	Manage(cm_selectAllBtn, (pad_mode == SELI_MODE));
	Manage(cm_separator,    (pad_mode == SELI_MODE));
}


static void set_visual_state(void)
{
	Boolean mapDrawMode = False;
	Boolean mapColourSelect = False;
	Boolean mapTextEntry = False;
	Boolean mapDistanceAttrib = False;
	Boolean mapDistanceSet = False;
	Boolean mapSelectAll = False;

	switch(pad_mode)
	{
		case LINE_MODE:
			mapDrawMode = True;
			mapColourSelect = True;
			break;

		case TEXT_MODE:
			mapTextEntry = True;
			break;

		case MEAS_MODE:
			mapDistanceAttrib = True;
			mapColourSelect = True;
			break;

		case DIST_MODE:
			mapDistanceSet = True;
			mapDistanceAttrib = True;
			mapColourSelect = True;
			break;

		case SELI_MODE:
			mapSelectAll = True;
			break;
	}

	XtSetMappedWhenManaged(drawMode, mapDrawMode);
	XtSetMappedWhenManaged(colourSelect, mapColourSelect);
	XtSetMappedWhenManaged(textEntry, mapTextEntry);
	XtSetMappedWhenManaged(distanceAttrib, mapDistanceAttrib);
	XtSetMappedWhenManaged(distanceSet, mapDistanceSet);
	XtSetMappedWhenManaged(selectAllBtn, mapSelectAll);
	XtSetMappedWhenManaged(deleteBtn, mapSelectAll);

	set_context_state();
}


static void send_edit_cmd(void)
{
	switch(pad_mode)
	{
		case LINE_MODE:
			(void) IngredVaCommand(GE_SCRATCHPAD, "EDIT DRAW %s %s", line_colour, line_style);
			break;

		case TEXT_MODE:
			(void) IngredVaCommand(GE_SCRATCHPAD, "EDIT TEXT %s %s %s%% \'%s\'",
								text_font_colour, text_font_type, text_font_size, text_string);
			break;

		case MEAS_MODE:
			(void) IngredVaCommand(GE_SCRATCHPAD, "EDIT DISTANCE %s %s %s %s %s%%",
					line_colour, line_style, distance_colour, distance_font_type, distance_font_size);
			break;

		case DIST_MODE:
			(void) IngredVaCommand(GE_SCRATCHPAD, "EDIT PRESET_DISTANCE %s %s %s %s %s%% %d",
					line_colour, line_style, distance_colour, distance_font_type, distance_font_size,
					XmpSpinBoxGetValue(spinBox));					
			break;

		case SELI_MODE:
			(void) IngredVaCommand(GE_SCRATCHPAD, "EDIT SELECT");
			break;
	}
}


/*ARGSUSED*/
static void draw_mode_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	/* Note that the smoothing is defaulted to 50 */
	drawing_mode_ndx = PTR2INT(client_data);
	(void) IngredVaCommand(GE_ACTION, "STATE DRAW_MODE %s 50", drawing_modes[drawing_mode_ndx]);
	send_edit_cmd();
	save_sp_state();
}


/*ARGSUSED*/
static void text_cb(TextSelectorStruct *cbs )
{
	if(cbs->reason == SELECT_TEXT_FOCUS)
	{
		save_sp_state();
	}
	else
	{
		text_font_type   = cbs->font;
		text_font_size   = cbs->size;
		text_font_colour = cbs->colour;
		text_string      = cbs->text;
		send_edit_cmd();
	}
}


/*ARGSUSED*/
static void colour_cb(ColorSelectorStruct *cbs )
{
	line_style  = cbs->style;
	line_colour = cbs->colour;
	if(cbs->reason != SELECT_NONE)
	{
		send_edit_cmd();
		save_sp_state();
	}
}


/*  Note that this function is set up so as to operate as a work procedure function
 *  which must have XtPointer as a parameter and return True when actions complete.
 *  This sets the increment of the distance spinner to something related to the size
 *  of the current map projection. Set it to something that feels "good". This is
 *  done by setting the minimum value to 2% of maxval then selecting from the list
 *  of predefined increments. Also ensure that the minimum is the increment and that
 *  the value is a multiple of the increment. The new value is set to a value that
 *  has the same ratio to the limits as the old value so that the initial value is
 *  something besides the minimum.
 */

/*  The following variable is used as an interlock to avoid this proc from being
 *  called multiple times if the zoom_observer function is called multiple times
 *  within the same event sequence.
 */
static Boolean setting_increment = False;


/* ARGSUSED*/
static Boolean set_spinbox_increment(XtPointer unused)
{
	int n, xlen, ylen, val, maxval, inc, incval;
	MAP_PROJ *proj;

	/* Standard increment values */
	const int inc_vals[]  = { 100, 50, 40, 30, 25, 20, 15, 10, 5, 1 };

	/* The current active zoomed projection should be returned by
	 * gxGetZoomMproj(), but in case there is a problem (or if the
	 * display has not yet been zoomed) default to the base projection.
	 */
	proj = gxGetZoomMproj();
	if (!proj || equivalent_map_projection(proj, &NoMapProj))
		proj = get_target_map();

	/* Set the size of the window */
	xlen = (int)(proj->definition.xlen * proj->definition.units / 1000.0 + 0.5);
	ylen = (int)(proj->definition.ylen * proj->definition.units / 1000.0 + 0.5);

	/* Find the standard increment to use */
	incval = MIN(xlen,ylen)/50;
	inc = inc_vals[0];
	for( n = 1; n < XtNumber(inc_vals); n++)
	{
		if(incval <= inc_vals[n-1]) inc = inc_vals[n];
	}

	/* This will find the value in the new range that has the same ratio within
	 * the new range as the old range value (in multiples of the increment).
	 */
	maxval = (MAX(xlen,ylen)/inc)*inc;
	val = (int)(((float)(XmpSpinBoxGetValue(spinBox)*maxval)/(float)preset_distance_max) + 0.5);
	val = ((val+inc/2)/inc)*inc;
	if(val > maxval || val < inc) val = inc;

	/* When changing the limits we must make sure that the value is within the new
	 * limits or the widget will not accept the set.
	 */
	if(preset_distance_max > maxval) preset_distance_max = maxval;
	XmpSpinBoxSetValue(spinBox, preset_distance_max, False);
	preset_distance_max = maxval;
	XtVaSetValues(spinBox, XmNincrement, inc, XmNminimum, inc, XmNmaximum, maxval, NULL);
	XmpSpinBoxSetValue(spinBox, val, False);
	send_edit_cmd();
	setting_increment = False;
	return True;
}


/* Notify observers called function for zooming operation. The zoomed projection may not
 * have been set at the time this function is called so the set function is called as a
 * work procedure to ensure that all activity associated with the event sequence involved
 * with setting the projection has finished before getting the projection information.
 */
static void zoom_observer(String *parms, int nparms)
{
	if(!PanelIsActive(SCRATCHPAD)) return;
	if(setting_increment) return;
	if(nparms < 2) return;
	if(!(same(parms[1],E_OFF) || same(parms[1],E_CANCEL))) return;

	setting_increment = True;
	(void) XtAppAddWorkProc(GV_app_context, set_spinbox_increment, NULL);
}


/* Callback function for the preset distance spin box widget. */
/*ARGSUSED*/
static void distance_cb(Widget w , XtPointer client_data , XtPointer call_data)
{
	XmpSpinBoxCallbackStruct *rtn = (XmpSpinBoxCallbackStruct *)call_data;
	if(rtn->reason == XmCR_OK)
	{
		send_edit_cmd();
		save_sp_state();
	}
}


/*ARGSUSED*/
static void mode_cb(Widget	w , XtPointer client_data , XtPointer call_data )
{
	if(!XmToggleButtonGetState(w)) return;
	pad_mode = PTR2INT(client_data);
	set_visual_state();
	send_edit_cmd();
}

/*ARGSUSED*/
static void distance_font_cb(FontSelectorStruct *font)
{
	distance_font_type = font->type;
	distance_font_size = font->size;
	distance_colour    = font->colour;

	if(font->reason != SELECT_NONE)
	{
		send_edit_cmd();
		save_sp_state();
	}
}


/*================ Public Functions ===================*/


/*=========================================================================*/
/*
*	ScratchpadStartup() - Maps the scratchpad editor and sends Ingred
*	commands.
*/
/*=========================================================================*/
void ScratchpadStartup(void)
{
	(void) set_spinbox_increment(NULL);

	if (!GetDisplayState(SCRATCHPAD))
		(void) IngredCommand(GE_SCRATCHPAD, "SHOW");

	set_visual_state();
	XuMenuSelectItemByName(drawMode, drawing_modes[drawing_mode_ndx]);
	draw_mode_cb(NULL, INT2PTR(drawing_mode_ndx), NULL);

	SetActiveContextMenu(panelContextMenu);
}


/*=========================================================================*/
/*
*  ScratchpadExit() - Send the appropriate sequence of commands uppon exit.
*/
/*=========================================================================*/
/*ARGSUSED*/
void ScratchpadExit(String key)
{
	if( !GetDisplayState(SCRATCHPAD) )
		(void) IngredCommand(GE_SCRATCHPAD, "HIDE");
	SetActiveContextMenu(None);
}


/*=========================================================================*/
/*
 *  CreateScratchpadPandl()
 */
/*=========================================================================*/
void CreateScratchpadPanel(Widget parent)
{
	int    i;
	String buf;
	Pixmap lpx, ipx;
	Widget w, frame, form, editManager;

	GW_scratchPanel = parent;

	XtVaSetValues(GW_scratchPanel, XmNhorizontalSpacing, 6, XmNverticalSpacing, 20, NULL);

	frame = XmVaCreateManagedFrame(GW_scratchPanel, "frame",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	(void)XmVaCreateManagedLabel(frame, "editLabel",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	editManager = XmVaCreateManagedRowColumn(frame, "editManager",
		XmNradioBehavior, True,
		NULL);

	stateBtns[0] = XmVaCreateManagedToggleButton(editManager, "drawBtn", XmNset, XmSET, NULL);
	XtAddCallback(stateBtns[0], XmNvalueChangedCallback, mode_cb, INT2PTR(LINE_MODE));

	stateBtns[1] = XmVaCreateManagedToggleButton(editManager, "textBtn", NULL);
	XtAddCallback(stateBtns[1], XmNvalueChangedCallback, mode_cb, INT2PTR(TEXT_MODE));

	stateBtns[2] = XmVaCreateManagedToggleButton(editManager, "measureBtn", NULL);
	XtAddCallback(stateBtns[2], XmNvalueChangedCallback, mode_cb, INT2PTR(MEAS_MODE));

	stateBtns[3] = XmVaCreateManagedToggleButton(editManager, "distanceBtn", NULL);
	XtAddCallback(stateBtns[3], XmNvalueChangedCallback, mode_cb, INT2PTR(DIST_MODE));

	stateBtns[4] = XmVaCreateManagedToggleButton(editManager, "selectItemsBtn", NULL);
	XtAddCallback(stateBtns[4], XmNvalueChangedCallback, mode_cb, INT2PTR(SELI_MODE));

	form = XmVaCreateManagedForm(GW_scratchPanel, "form",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, frame,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	selectAllBtn = XmVaCreateManagedPushButton(form, "selectAllBtn",
		XmNmappedWhenManaged, False,
		XmNmarginHeight, 3,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 19,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 19,
		NULL);

	XtAddCallback(selectAllBtn, XmNactivateCallback, context_menu_cb, INT2PTR(100));

	deleteBtn = XmVaCreateManagedPushButton(form, "deleteBtn",
		XmNmappedWhenManaged, False,
		XmNmarginHeight, 3,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, selectAllBtn,
		XmNtopOffset, 9,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 19,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 19,
		NULL);

	XtAddCallback(deleteBtn, XmNactivateCallback, context_menu_cb, INT2PTR(101));

	drawMode = XuVaMenuBuildOption(form, "drawMode", NULL,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	for(i = 0; i < XtNumber(drawing_modes); i++)
	{
		PushButtonPixmaps(GW_scratchPanel, "draw", drawing_modes[i], &lpx, &ipx);
		w = XuMenuAddPixmapButton(drawMode, drawing_modes[i], NoId, lpx, ipx, draw_mode_cb, INT2PTR(i));
	}

	textEntry = CreateTextSelector(form, SCRATCHPAD, text_cb,
		XmNmappedWhenManaged, False,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	distanceSet = XmVaCreateManagedColumn(form, "align",
		XmNmappedWhenManaged, False,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	spinBox = XmpVaCreateManagedSpinBox(distanceSet, "setDist",
		XmNvalue, 1,
		XmNincrement, 1,
		XmNminimum, 0,
		XmNmaximum, preset_distance_max,
		XmNcolumns, 4,
		NULL);
	XtAddCallback(spinBox, XmNvalueChangedCallback, distance_cb, NULL);

	distanceAttrib = CreateFontSelector(form, distance_font_cb,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	distance_font_type = GetFontSelectorValue(distanceAttrib, SELECT_FONT_TYPE);
	distance_font_size = GetFontSelectorValue(distanceAttrib, SELECT_FONT_SIZE);
	distance_colour    = GetFontSelectorValue(distanceAttrib, SELECT_COLOUR);

	colourSelect = CreateColorSelector(GW_scratchPanel, SCRATCHPAD, True, colour_cb,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, form,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	line_style  = GetColorSelectorValue(colourSelect, SELECT_STYLE);
	line_colour = GetColorSelectorValue(colourSelect, SELECT_COLOUR);

	XtManageChild(GW_scratchPanel);

	if(XuStateDataGet(STATE_KEY,"all",NULL, &buf))
	{
		int     val, preset_val = 1;
		String  font, size, color, style, type;
		Boolean ok;

		font  = strtok_arg(buf);
		size  = strtok_arg(NULL);
		color = strtok_arg(NULL);
		SetTextSelector(textEntry, font, size, color);

		color = strtok_arg(NULL);
		style = strtok_arg(NULL);
		SetColorSelector(colourSelect, style, color);

		type  = strtok_arg(NULL);
		size  = strtok_arg(NULL);
		color = strtok_arg(NULL);
		SetFontSelector(distanceAttrib, type, size, color);

		val = inttok_arg(NULL, &ok);
		if (ok) preset_val = val;

		val = inttok_arg(NULL, &ok);
		if(ok && val >= 0 && val < XtNumber(drawing_modes))
			drawing_mode_ndx = val;

		/* The preset max is last for backwards compatability with stored state. */
		val = inttok_arg(NULL, &ok);
		if(ok && val > 1)
		{
			preset_distance_max = val;
			if(preset_val > val || preset_val < 1) preset_val = 1;
			XtVaSetValues(spinBox, XmNmaximum, val, XmNvalue, preset_val, NULL);
		}
	}
	create_context_menu();
	AddIngredObserver(ingred_message_observer);
	AddObserver(OB_ZOOM, zoom_observer);
	AddObserver(OB_MAIN_MENU_ACTIVATION, main_menu_observer);
}
