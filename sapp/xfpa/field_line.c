/*****************************************************************************
*
*  File:    field_line.c
*
*  Purpose: Provides the program interface for any line type field. Lines
*           are displayed as pixmap labels in a set of toggle buttons.
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
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>
#include <ingred.h>
#include "editor.h"
#include "metafileToPixmap.h"
#include "observer.h"
#include "contextMenu.h"

/* These specify the particulars of the line pixmap patterns used for
*  display to the users.
*/
#define PATTERN_WIDTH			130
#define PATTERN_HEIGHT			22
#define PATTERN_MARGIN_HEIGHT	3
#define PATTERN_MARGIN_WIDTH	0
#define PATTERN_HEIGHT_PCT		50
#define PATTERN_REPEAT_COUNT	3

/* Useful defines for clearer code
 */
#define MEM					GV_active_field->memory
#define ALLOW_ATTRIB_EDIT	GV_active_field->memory->attrib_editing


static void set_as_active_cal      (CAL);
static void destroy_data		   (FIELD_INFO*);
static void edit_cmd_action	       (String);
static void entry			       (void);
static void draw_select_cb	       (Widget, XtPointer, XtPointer);
static void launch_entry_dialog_cb (Widget, XtPointer, XtPointer);
static void modify_select_cb	   (Widget, XtPointer, XtPointer);
static void move_indicator         (Widget);
static void set_cal_value          (CAL);
static void send_edit_cmd          (int);
static void show_panel		       (Boolean);
static void show_value_cb          (Widget, XtPointer, XtPointer);
static void status_from_ingred     (CAL, String*, int);


/* Define the memory structure for the line menus. There will be one
*  of these for every line type field added to the system.
*/
typedef struct _memory {
	int     nlines;			/* Number of lines in this field */
	int     sel;			/* Currently selected draw line number */
	CAL     set_cal;		/* CAL to use when the setBtn is activated */
	CAL     *cal;           /* CAL structure array */
	Pixmap  *label_px;		/* pixmap to display for line */
	Pixmap  *insen_px;		/* insensitive pixmap */
	Boolean attrib_editing;	/* Is there an attribute editing definition file? */
} FL_MEM;

static Widget panel = NullWidget;
static Widget attributeDisplay, labelDisplay, setBtn;
static Widget drawSelectWindow, drawSelectList;
static Widget modifySelectWindow, modifySelectList, modifyIndicator;

static int         nbtns      = 0;		/* Number of line selection buttons created */
static WidgetList  drawBtns   = NULL;
static WidgetList  modifyBtns = NULL;

/* The button labels and command for the selected object context menus. These are used to set
 * the popup button labels and to provide the commands to be sent to ingred associated with
 * the buttons.
 */
static CONTEXT flip_context = {
	E_FLIP, 0, 3,
	{
		{flipBtnId,E_FLIP},
		{reverseBtnId,E_REVERSE},
		{flipReverseBtnId,E_FLIP_REVERSE}
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
static CONTEXT merge_context = {
	E_MERGE, 0, 3,
	{
		{mergeBtnId,E_MERGE},
		{translateBtnId,E_TRANSLATE},
		{rotateBtnId,E_ROTATE}
	}
};
static CONTEXT modify_context = {
	E_MODIFY, 0, 2,
	{
		{setBtnId,E_SET},
		{deleteLineBtnId,E_DELETE}
	}
};
static CONTEXT delete_context = {
	E_MODIFY, 0, 1,
	{
		{deleteLineBtnId,E_DELETE}
	}
};

/* Create edit panel for line type fields.
*/
void CreateLineFieldPanel(Widget parent , Widget topAttach)
{
	Widget   label, showBtn, form;
	XmString xmlabel;

	panel = XmVaCreateForm(parent, "linePanel",
		XmNverticalSpacing, 3,
		XmNhorizontalSpacing, 3,
		XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, topAttach,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	/* start attribute display objects */

	attributeDisplay = XmVaCreateManagedForm(panel, "attribDisplay",
		XmNverticalSpacing, 3,
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 19,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	label = XmVaCreateManagedLabel(attributeDisplay, "lineLabel",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	labelDisplay = XmVaCreateManagedText(attributeDisplay, "labelDisplay",
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

	showBtn = XmVaCreateManagedPushButton(attributeDisplay, "showBtn",
		XmNmarginWidth, 3,
		XmNmarginHeight, 3,
		XmNmarginWidth, 9,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, labelDisplay,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	XtAddCallback(showBtn, XmNarmCallback,    show_value_cb, (XtPointer)False);
	XtAddCallback(showBtn, XmNdisarmCallback, show_value_cb, NULL);

	setBtn = XmVaCreateManagedPushButton(attributeDisplay, "setBtn",
		XmNmarginHeight, 3,
		XmNmarginWidth, 19,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, labelDisplay,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(setBtn, XmNactivateCallback, launch_entry_dialog_cb, NULL);

	(void) XmVaCreateManagedLabel(attributeDisplay, "lineTypeLabel",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, showBtn,
		XmNtopOffset, 19,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	/* end attribute display objects */

	drawSelectWindow = XmVaCreateScrolledWindow(panel, "drawSelectWindow",
		XmNscrollingPolicy, XmAUTOMATIC,
		XmNshadowThickness, 0,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, attributeDisplay,
		XmNtopOffset, 5,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	drawSelectList = XmVaCreateManagedRowColumn(drawSelectWindow, "drawSelectList",
		XmNradioBehavior, True,
		XmNspacing, 3,
		XmNmarginWidth, 0,
		XmNmarginHeight, 0,
		NULL);

	modifySelectWindow = XmVaCreateScrolledWindow(panel, "modifySelectWindow",
		XmNmanageChild, False,
		XmNsensitive, False,
		XmNscrollingPolicy, XmAUTOMATIC,
		XmNscrolledWindowMarginWidth, 0,
		XmNshadowThickness, 0,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, attributeDisplay,
		XmNtopOffset, 5,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 3,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	/* There is an indicator that will appear beside the appropriate line type
	 * when a line is selected for modify so that there is more of an indication
	 * of the selection. This is implemented as one label that is moved to the
	 * proper location by changing the topOffset resource of the form attachment.
	 * Note that the indicator height settings are the same macros as used to
	 * create the line pixmaps so that the indicator will be centered.
	 */
	form = XmVaCreateManagedForm(modifySelectWindow, "form",
		XmNverticalSpacing, 0,
		XmNhorizontalSpacing, 0,
		XmNmarginWidth, 0,
		XmNmarginHeight, 0,
		NULL);

	xmlabel = XmStringCreate(">>", LARGE_BOLD_FIXED_FONT);

	modifyIndicator = XmVaCreateManagedLabel(form, "mi",
		XmNmappedWhenManaged, False,
		XmNlabelString, xmlabel,
		XmNheight, PATTERN_HEIGHT,
		XmNmarginHeight, PATTERN_MARGIN_HEIGHT,
		XmNmarginWidth, 0,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	XmStringFree(xmlabel);

	modifySelectList = XmVaCreateManagedRowColumn(form, "modifySelectList",
		XmNspacing, 1,
		XmNmarginWidth, 0,
		XmNmarginHeight, 0,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftOffset, 3,
		XmNleftWidget, modifyIndicator,
		NULL);

	AddIngredObserver(status_from_ingred);
}


/* The given field is to be added as a line type elment.  Line fields are
*  displayed as a series of pixmaps or labels in a set of toggle buttons,
*  only one of which is active at a time.
*/
void AddLineField(FIELD_INFO  *field )
{
	int	 i;
	FpaConfigElementEditorStruct *editor;

	static char module[] = "AddLineField";

	editor = GV_edit_mode ? field->info->element->elem_detail->editor : NULL;

	field->entryFcn       = entry;
	field->showFcn		  = show_panel;
	field->sendEditCmdFcn = send_edit_cmd;
	field->changeEditFcn  = edit_cmd_action;
	field->depictFcn      = NULL;
	field->setBkgndFcn    = NULL;
	field->destroyFcn	  = destroy_data;
	field->memory		  = OneMem(FL_MEM);
	field->editor		  = GetEditor((editor) ? LINE_FIELD_EDITOR:LINE_FIELD_NO_EDIT);
	field->cal            = CAL_create_by_edef(field->info->element);
	field->bkgnd_cal      = NullCal;

	SetFieldSampleInfo(field, FpaCsampleControlAttribType, AttribAll);


	if(editor && GV_edit_mode)
	{
		FpaConfigElementLineTypeStruct *ls = field->info->element->elem_detail->line_types;
		FL_MEM                         *fm = field->memory;

		fm->nlines         = 0;
		fm->sel            = 0;
		fm->attrib_editing = MenuFileExists(ENTRY_ATTRIBUTES|MODIFY_ATTRIBUTES, field);

		/* Check to make sure that line_types are defined */
		if(ls != NULL && ls->ntypes > 0)
		{
			fm->cal       = NewMem(CAL,    ls->ntypes);
			fm->label_px  = NewMem(Pixmap, ls->ntypes);
			fm->insen_px  = NewMem(Pixmap, ls->ntypes);

			for( i = 0; i < ls->ntypes; i++ ) 
			{
				fm->label_px[fm->nlines] = (Pixmap)0;
				fm->insen_px[fm->nlines] = (Pixmap)0;
				fm->cal[fm->nlines] = CAL_create_by_edef(field->info->element);
				CAL_set_attribute(fm->cal[fm->nlines], CALlinetype,  ls->type_names[i]);
				CAL_set_attribute(fm->cal[fm->nlines], CALcategory,  ls->type_names[i]);
				CAL_set_attribute(fm->cal[fm->nlines], CALuserlabel, ls->type_labels[i]);
				CAL_set_attribute(fm->cal[fm->nlines], CALautolabel, ls->type_labels[i]);
				if(blank(ls->patterns[i]))
				{
					fm->nlines++;
				}
				else if(MetafilePatternToPixmap( panel, ls->patterns[i],
							PATTERN_WIDTH, PATTERN_HEIGHT,
							PATTERN_MARGIN_WIDTH, PATTERN_MARGIN_HEIGHT,
							PATTERN_HEIGHT_PCT, PATTERN_REPEAT_COUNT,
							NULL, "*linePanel.lineBackground",
							&fm->label_px[fm->nlines], NULL, &fm->insen_px[fm->nlines]))
				{
					fm->nlines++;
				}
				else
				{
					Warning(module, "NoBitmapFile", ls->type_labels[i]);
				}
			}
		}
		else
		{
			/* Create one cal structure to avoid crashes */
			fm->cal = OneMem(CAL);
			fm->cal[0] = CAL_create_by_edef(field->info->element);
		}

		if( fm->nlines > 0 )
		{
			(void) CAL_destroy(field->cal);
			field->cal = CAL_duplicate(fm->cal[fm->sel]);
		}
	}
}


/*==================== LOCAL STATIC FUNCTIONS ===========================*/


/* Right mouse button context menu functions. See the
 * ActivateSelectContextMenu function in panel_fieldEdit.c
 */
static void action_command (CONTEXT *info, CAL cal)
{
	if(same(info->selected_item->edit_cmd,E_SET))
	{
		ACTIVATE_lineAttributesDialog(panel, (cal)? cal : GV_active_field->cal, set_cal_value);
	}
	else
	{
		IngredVaEditCommand(cal, NullCal, "%s %s %s", E_EDIT, info->edit_mode, info->selected_item->edit_cmd);
		if(same(info->edit_mode,E_MERGE))
		{
			String notify[2];
			notify[0] = E_MERGE;
			notify[1] = info->selected_item->edit_cmd;
			NotifyObservers(OB_MENU_ACTIVATION, notify, 2);
		}
	}
}

/* Handles status function calles from Ingred.
 */
static void status_from_ingred( CAL cal, String *parms, int nparms )
{
	int     n, pc = 0;
	String  key;

	if(!GV_edit_mode) return;
	if(!same_ic(parms[pc++],E_EDIT)) return;
	if(!same_ic(parms[pc++],E_LINE)) return;
	if(!IsActiveEditor(LINE_FIELD_EDITOR)) return;

	key = parms[pc++];
	MEM->set_cal = NULL;

	if(same_ic(key,E_SELECT))
	{
		XtSetSensitive(panel, True);

		if(InEditMode(E_FLIP))
		{
			ActivateSelectContextMenu(&flip_context, action_command, NULL);
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
			ActivateSelectContextMenu(&delete_context, action_command, NULL);
			if(cal)
			{	/*
				* Set the indicator pointer to the appropriate pixmap
				*/
				char name[200];
				(void) safe_strcpy(name, CAL_get_attribute(cal, CALuserlabel));
				for( n = 0; n < MEM->nlines; n++ )
				{
					if(!same(name, CAL_get_attribute(MEM->cal[n],CALuserlabel))) continue;
					move_indicator(modifyBtns[n]);
					break;
				}
				XmTextSetString(labelDisplay, name);
				/*
				* The default is "SET", so a blank is taken as a set
				*/
				key = parms[pc++];
				if(blank(key) || same_ic(key,E_SET))
				{
					XtSetSensitive(modifySelectWindow, True);
					XtSetSensitive(setBtn, ALLOW_ATTRIB_EDIT);
					MEM->set_cal = cal;
					if(ALLOW_ATTRIB_EDIT)
						ActivateSelectContextMenu(&modify_context, action_command, cal);
				}
			}
			ShowContextMenuAttributes(True);
		}
		else
		{
			ShowContextMenuAttributes(InEditMode(E_DRAW));
			DeactivateSelectContextMenu();
		}
	}
	else
	{
		move_indicator(NULL);
		XtSetSensitive(modifySelectWindow, False);
		DestroyAttributesEntryDialog();
		XtSetSensitive(panel, False);
		/*
		 * This is check is required because of timing issues. It is possible
		 * for ingred to send a deselect command after the editor state has
		 * been switched from modify. In this case we do not want to blank out
		 * the display.
		 */
		if(InEditMode(E_MODIFY))
		{
			XmTextSetString(labelDisplay, " ");
			XtSetSensitive(setBtn, False);
		}
		/* Do not hide the attributes if in drawing mode */
		ShowContextMenuAttributes(InEditMode(E_DRAW));
		DeactivateSelectContextMenu();
	}

	if (ALLOW_ATTRIB_EDIT)
		UpdateAttributesEntryDialog(GV_active_field->cal);
}


static void set_as_active_cal(CAL cal)
{
	(void) CAL_destroy(GV_active_field->cal);
	GV_active_field->cal = CAL_duplicate(cal);
}


static void move_indicator(Widget w)
{
	Boolean map = False;
	if(w)
	{
		Position bx, by, fx, fy;
		map = True;
		XtTranslateCoords(w, 0, 0, &bx, &by);
		XtTranslateCoords(XtParent(modifySelectList), 0, 0, &fx, &fy);
		XtVaSetValues(modifyIndicator, XmNtopOffset, (int)(by-fy), NULL);
	}
	XtSetMappedWhenManaged(modifyIndicator, map);
}


/*ARGSUSED*/
static void context_cb( int ndx )
{
	if(InEditMode(E_DRAW))
	{
		XmToggleButtonSetState(drawBtns[ndx], True, True);
	}
	else if(InEditMode(E_MODIFY))
	{
		modify_select_cb(NullWidget, INT2PTR(ndx), NULL);
	}
}


/* Called when a line field is activated.  This sets the data memory
*  pointer and sets the button labels.
*/
static void entry(void)
{
	int i;

	if(GV_active_field->editor == GetEditor(LINE_FIELD_NO_EDIT)) return;

	/* Check to see if we need more buttons.
	*/
	if(MEM->nlines > nbtns)
	{
		drawBtns   = MoreWidgetArray(drawBtns, MEM->nlines);
		modifyBtns = MoreWidgetArray(modifyBtns, MEM->nlines);
		for(i = nbtns; i < MEM->nlines; i++)
		{
			drawBtns[i] = XmVaCreateToggleButton(drawSelectList, "lineDrawBtn",
				XmNhighlightOnEnter, True,
				XmNhighlightThickness, 2,
				XmNmarginHeight, 0,
				XmNmarginWidth, 0,
#ifdef INDICATOR_SIZE
				XmNindicatorSize, INDICATOR_SIZE,
#endif
				NULL);
			XtAddCallback(drawBtns[i], XmNvalueChangedCallback, draw_select_cb, INT2PTR(i));

			modifyBtns[i] = XmVaCreatePushButton(modifySelectList, "lineModBtn",
				NULL);
			XtAddCallback(modifyBtns[i], XmNactivateCallback, modify_select_cb, INT2PTR(i));
		}
		nbtns = MEM->nlines;
	}

	XtUnmanageChildren(drawBtns,   (Cardinal) nbtns);
	XtUnmanageChildren(modifyBtns, (Cardinal) nbtns);

	/* Initialize the line selection in the context menu */
	InitContextMenuAttributes(GV_active_field->info->sh_label, context_cb);

	for( i = 0; i < MEM->nlines; i++ )
	{
		if(MEM->label_px[i])
		{
			XtVaSetValues(drawBtns[i],
				XmNlabelType, XmPIXMAP,
				XmNlabelPixmap, MEM->label_px[i],
				XmNselectInsensitivePixmap, MEM->insen_px[i],
				XmNset, False,
				NULL);

			XtVaSetValues(modifyBtns[i],
				XmNlabelType, XmPIXMAP,
				XmNlabelPixmap, MEM->label_px[i],
				XmNlabelInsensitivePixmap, MEM->insen_px[i],
				NULL);
		}
		else
		{
			XmString xmlabel = XmStringCreateLocalized(CAL_get_attribute(MEM->cal[i], CALuserlabel));
			XtVaSetValues(drawBtns[i],
				XmNlabelType, XmSTRING,
				XmNlabelString, xmlabel,
				XmNset, False,
				NULL);

			XtVaSetValues(modifyBtns[i],
				XmNlabelType, XmSTRING,
				XmNlabelString, xmlabel,
				NULL);
			XmStringFree(xmlabel);
		}
		AddToContextMenuAttributes(CAL_get_attribute(MEM->cal[i], CALuserlabel));
	}

	set_as_active_cal(MEM->cal[MEM->sel]);

	/* Bug 20051020: Make sure there are buttons */
	if (nbtns > 0) XuToggleButtonSet(drawBtns[MEM->sel], True, False);
	XtManageChildren(drawBtns,   (Cardinal) MEM->nlines);
	XtManageChildren(modifyBtns, (Cardinal) MEM->nlines);

	XmTextSetString(labelDisplay, CAL_get_attribute(GV_active_field->cal, CALuserlabel));
}

/* Callback for ConfigureMainContextMenuAuxPushButton */
/*ARGSUSED*/
static void aux_push_button_cb(XtPointer unused)
{
	ACTIVATE_lineAttributesDialog(panel, GV_active_field->cal, set_cal_value);
}


/* To be called by the edit command function whenever an edit command
*  button is pushed.
*/
static void edit_cmd_action(String cmd )
{
	DeactivateSelectContextMenu();
	if(same_ic(cmd,E_DRAW))
	{
		set_as_active_cal(MEM->cal[MEM->sel]);
		XtManageChild(attributeDisplay);
		XtManageChild(drawSelectWindow);
		XtUnmanageChild(modifySelectWindow);
		XmTextSetString(labelDisplay, CAL_get_attribute(GV_active_field->cal, CALuserlabel));
		XtSetSensitive(setBtn, ALLOW_ATTRIB_EDIT);
		if (ALLOW_ATTRIB_EDIT)
			ConfigureMainContextMenuAuxPushButton("setBtn", aux_push_button_cb, NULL);
		ShowContextMenuAttributes(True);
		XtManageChild(panel);
	}
	else if(same_ic(cmd,E_MODIFY))
	{
		XtManageChild(attributeDisplay);
		XtManageChild(modifySelectWindow);
		XtUnmanageChild(drawSelectWindow);
		XmTextSetString(labelDisplay, " ");
		XtSetSensitive(setBtn, False);
		ReleaseMainContextMenuAuxPushButton();
		ShowContextMenuAttributes(False);
		XtManageChild(panel);
	}
	else
	{
		XtUnmanageChild(panel);
		XtUnmanageChild(drawSelectWindow);
		XtUnmanageChild(modifySelectWindow);
		XtUnmanageChild(attributeDisplay);
		ShowContextMenuAttributes(False);
		XtSetSensitive(setBtn, False);
		ReleaseMainContextMenuAuxPushButton();
	}
	XmUpdateDisplay(panel);
}


/* Frees up any resources assigned by this editor when the field is
*  removed from the system.
*/
static void destroy_data(FIELD_INFO *field )
{
	int i;

	for( i = 0; i < field->memory->nlines; i++ )
	{
		(void) XmDestroyPixmap(XtScreen(panel),field->memory->label_px[i]);
		(void) XmDestroyPixmap(XtScreen(panel),field->memory->insen_px[i]);
		(void) CAL_destroy(field->memory->cal[i]);
	}

	FreeItem(field->memory->cal);
	FreeItem(field->memory->label_px);
	FreeItem(field->memory->insen_px);
	FreeItem(field->memory);
	(void) CAL_destroy(field->cal);
}


/* Called when we wish to show or hide the panel.
*/
static void show_panel(Boolean show )
{
	if (show)
	{
		edit_cmd_action(GV_active_field->editor->active->cmd);
	}
	else
	{
		XtUnmanageChild(panel);
		HideSamplingPanel();
		HideMergeFieldPanel();
		HideLabelFieldPanel();
	}
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
*/
/*=========================================================================*/
static void send_edit_cmd(int	cmd )
{
	char txbuf[100];

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
		(void) snprintf(txbuf, sizeof(txbuf), "%s %s %s", E_EDIT, GV_active_field->editor->active->cmd,
						(InEditMode(E_MODIFY) && cmd == SET_MODE)? E_SET:"");
		(void) IngredEditCommand(txbuf, GV_active_field->cal, NullCal);
	}
}


/* The callback function for the line field selection toggle buttons.
*/
/*ARGSUSED*/
static void draw_select_cb(Widget w , XtPointer client_data , XtPointer unused )
{
	if(!XmToggleButtonGetState(w)) return;
	MEM->sel = PTR2INT(client_data);
	set_as_active_cal(MEM->cal[MEM->sel]);
	XmTextSetString(labelDisplay, CAL_get_attribute(GV_active_field->cal, CALuserlabel));
	send_edit_cmd(ACCEPT_MODE);
}


/*ARGSUSED*/
static void modify_select_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int ndx = PTR2INT(client_data);

	move_indicator(modifyBtns[ndx]);
	set_as_active_cal(MEM->cal[ndx]);
	XmTextSetString(labelDisplay, CAL_get_attribute(GV_active_field->cal, CALuserlabel));
	send_edit_cmd(SET_MODE);
}


static void set_cal_value(CAL cal)
{
	if (IsNull(cal)) return;
	set_as_active_cal(cal);
	XmTextSetString(labelDisplay, CAL_get_attribute(cal, CALuserlabel));
	/* 2007.09.05 Only send an edit command if in draw mode */
	if (InEditMode(E_DRAW))
		send_edit_cmd(SET_MODE);

}


/*ARGSUSED*/
static void launch_entry_dialog_cb (Widget w, XtPointer client_data, XtPointer call_data)
{
	ACTIVATE_lineAttributesDialog(panel, (MEM->set_cal)? MEM->set_cal : GV_active_field->cal, set_cal_value);
}


/*ARGSUSED*/
static void show_value_cb (Widget w, XtPointer client_data, XtPointer call_data)
{
	Boolean show = (((XmPushButtonCallbackStruct *)call_data)->reason == XmCR_ARM);
	DisplayAttributesPopup(panel, show, GV_active_field->cal);
}
