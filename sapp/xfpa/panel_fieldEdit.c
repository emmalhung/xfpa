/****************************************************************************/
/*
*  File:    panel_fieldEdit.c
*
*  Purpose: Handles all functionality associated with the editor control
*           panel of the interface and of the associated popup context
*           menus.
*
*    Notes: Fields are separated into groups.  When an field is added
*           to the list of fields it is stored according to the grouping
*           associated with the particular field.  The grouping order is
*           preset in the Groups config file, but the number of groups
*           active depends on the fields.  The order of the grouping as
*           presented to the user does depend on the group order in the
*           config file.  The type of editor associated with each field
*           is determined by the field type as the editors are generic.
*
*           The right button context menu popup logic and code is mostly in
*           this file. The main menu buttons and separators are managed and
*           unmanaged to cause the menu to appear different depending on the
*           context. The second menu is used for editing when an object has
*           been selected and there are editing choices to make. Each panel
*           that has context menus has its own popup menu, and which one is
*           active is controlled by setting the SetActiveContextMenu function
*           to the appopriate popup within the panel code.
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
#include <math.h>
#include <ingred.h>
/* Undefine bzero and bcopy to stop compiler complaints */
#undef bzero
#undef bcopy
#include <Xm/CascadeB.h>
#include <Xm/ComboBox.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>
#include <Xm/ToggleB.h>
#include "global.h"
#include "resourceDefines.h"
#include "editor.h"
#include "observer.h"
#include "timelink.h"
#include "contextMenu.h"

/* Define to shorten some code lines but to still be quite recognizable
 */
#define AFE	GV_active_field->editor

/*  Type parameter values as used by the create_radius_of_influence_btns() function.
*/
#define ROI   0		/* radius of influence */
#define PUCK  1		/* radius of the puck editor */

/* The drawing mode state for the various editor types are kept separate. Turns out that
 * WNI forecasters like to use continuous draw for areas and point-by-point for fronts.
 * The following variables keep the info and there are pointers in the editing structure
 * that point to these. The modes are set everytime a new field is made active in
 * accordance to the field type. These use the same option menus which are also set
 * every time the active field changes. The menus in return set the drawmode specified
 * by the active field.
 */
static DRAWMODE point_drawmode = { "point", NULL, NULL, NULL };
static DRAWMODE line_drawmode  = { "line",  NULL, NULL, NULL };
static DRAWMODE area_drawmode  = { "area",  NULL, NULL, NULL };
static DRAWMODE cont_drawmode  = { "cont",  NULL, NULL, NULL };
static DRAWMODE link_drawmode  = { "link",  NULL, NULL, NULL }; 
/*
 * The above values put into an array as this makes the code cleaner
 * for getting and saving values from the state store file.
 */
static DRAWMODE *drawmode_state[] = {
	&point_drawmode,
	&line_drawmode,
	&area_drawmode,
	&cont_drawmode,
	&link_drawmode
};


/* Set the editor information. The defines following are for those components found
 * in most of the editors.
 */
#define LABEL	"label"
#define SAMPLE	"sample"

static EDITOR info[] = {
	{
		POINT_FIELD_NO_EDIT, False, False, False, &point_drawmode, EM_POINT, NULL, NULL,
		{
			{SAMPLE, E_SAMPLE, SAMPLE, XuSAMPLE_CURSOR,  NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget}
		}
	},{
		POINT_FIELD_EDITOR, False, False, False, &point_drawmode, EM_POINT, NULL, NULL,
		{
			{"createPoint", E_DRAW,   "pointadd", XuPENCIL_CURSOR,  NullWidget},
			{"movePoint",   E_MOVE,   "pointmove",XuDEFAULT_CURSOR, NullWidget},
			{"modifyPoint", E_MODIFY, "modify",   XuDEFAULT_CURSOR, NullWidget},
			{"mergePoint",  E_MERGE,  "merge",    XuDEFAULT_CURSOR, NullWidget},
			{NULL,          E_NONE,   E_NONE,     XuDEFAULT_CURSOR, NullWidget},
			{SAMPLE,        E_SAMPLE, SAMPLE,     XuSAMPLE_CURSOR,  NullWidget},
			{NULL,          E_NONE,   E_NONE,     XuDEFAULT_CURSOR, NullWidget},
			{NULL,          E_NONE,   E_NONE,     XuDEFAULT_CURSOR, NullWidget}
		}
	},{
		LINE_FIELD_NO_EDIT, False, False, False, &line_drawmode, EM_LINE, NULL, NULL,
		{
			{SAMPLE, E_SAMPLE, SAMPLE, XuSAMPLE_CURSOR,  NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget}
		}
	},{
		LINE_FIELD_EDITOR, False, False, True, &line_drawmode, EM_LINE, NULL, NULL,
		{
			{"drawLine",  E_DRAW,   "draw",     XuPENCIL_CURSOR,  NullWidget},
			{"flipLine",  E_FLIP,   "flip",     XuDEFAULT_CURSOR, NullWidget},
			{"modifyLine",E_MODIFY, "modify",   XuDEFAULT_CURSOR, NullWidget},
			{"moveLine",  E_MOVE,   "linemove", XuDEFAULT_CURSOR, NullWidget},
			{"mergeLine", E_MERGE,  "merge",    XuDEFAULT_CURSOR, NullWidget},
			{"joinLine",  E_JOIN,   "join",     XuDEFAULT_CURSOR, NullWidget},
			{LABEL,       E_LABEL,  LABEL,      XuDEFAULT_CURSOR, NullWidget},
			{SAMPLE,      E_SAMPLE, SAMPLE,     XuSAMPLE_CURSOR,  NullWidget}
		}
	},{
		WIND_FIELD_NO_EDIT, True, False, False, &area_drawmode, EM_DISCRETE, NULL, NULL,
		{
			{SAMPLE, E_SAMPLE, SAMPLE, XuSAMPLE_CURSOR,  NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget}
		}
	},{
		WIND_FIELD_EDITOR, True, False, True, &area_drawmode, EM_DISCRETE, NULL, NULL,
		{
			{"drawArea",   E_DRAW,      "draw",   XuPENCIL_CURSOR,  NullWidget},
			{NULL,         E_DRAW_HOLE, "hole",   XuPENCIL_CURSOR,  NullWidget},
			{"modifyArea", E_MODIFY,    "modify", XuDEFAULT_CURSOR, NullWidget},
			{"moveArea",   E_MOVE,      "area",   XuDEFAULT_CURSOR, NullWidget},
			{"mergeArea",  E_MERGE,     "merge",  XuDEFAULT_CURSOR, NullWidget},
			{"divideArea", E_DIVIDE,    "divide", XuKNIFE_CURSOR,   NullWidget},
			{LABEL,        E_LABEL,     LABEL,    XuDEFAULT_CURSOR, NullWidget},
			{SAMPLE,       E_SAMPLE,    SAMPLE,   XuSAMPLE_CURSOR,  NullWidget}
		}
	},{
		DISCRETE_FIELD_NO_EDIT, True, False, False, &area_drawmode, EM_DISCRETE, NULL, NULL,
		{
			{SAMPLE, E_SAMPLE, SAMPLE, XuSAMPLE_CURSOR,  NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget}
		}
	},{
		DISCRETE_FIELD_EDITOR, True, False, True, &area_drawmode, EM_DISCRETE, NULL, NULL,
		{
			{"drawArea",   E_DRAW,      "draw",   XuPENCIL_CURSOR,  NullWidget},
			{NULL,         E_DRAW_HOLE, "hole",   XuPENCIL_CURSOR,  NullWidget},
			{"modifyArea", E_MODIFY,    "modify", XuDEFAULT_CURSOR, NullWidget},
			{"moveArea",   E_MOVE,      "area",   XuDEFAULT_CURSOR, NullWidget},
			{"mergeArea",  E_MERGE,     "merge",  XuDEFAULT_CURSOR, NullWidget},
			{"divideArea", E_DIVIDE,    "divide", XuKNIFE_CURSOR,   NullWidget},
			{LABEL,        E_LABEL,     LABEL,    XuDEFAULT_CURSOR, NullWidget},
			{SAMPLE,       E_SAMPLE,    SAMPLE,   XuSAMPLE_CURSOR,  NullWidget}
		}
	},{
		VECTOR_FIELD_NO_EDIT, False, True, False, &cont_drawmode, EM_VECTOR, NULL, NULL,
		{
			{SAMPLE, E_SAMPLE, SAMPLE, XuSAMPLE_CURSOR,  NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget}
		}
	},{
		VECTOR_FIELD_EDITOR, False, True, True, &cont_drawmode, EM_VECTOR, NULL, NULL,
		{
			{"moveRgn",  E_AREA,   "area",  XuDEFAULT_CURSOR, NullWidget},
			{"dragRgn",  E_DRAG,   "drag",  XuDEFAULT_CURSOR, NullWidget},
			{"pokeRgn",  E_POKE,   "poke",  XuDEFAULT_CURSOR, NullWidget},
			{"stompRgn", E_STOMP,  "stomp", XuDEFAULT_CURSOR, NullWidget},
			{"mergeRgn", E_MERGE,  "merge", XuDEFAULT_CURSOR, NullWidget},
			{"smoothRgn",E_SMOOTH, "smooth",XuDEFAULT_CURSOR, NullWidget},
			{LABEL,      E_LABEL,  LABEL,   XuDEFAULT_CURSOR, NullWidget},
			{SAMPLE,     E_SAMPLE, SAMPLE,  XuSAMPLE_CURSOR,  NullWidget}
		}
	},{
		CONTINUOUS_FIELD_NO_EDIT, False, True, False, &cont_drawmode, EM_CONTINUOUS, NULL, NULL,
		{
			{SAMPLE, E_SAMPLE, SAMPLE, XuSAMPLE_CURSOR,  NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget}
		}
	},{
		CONTINUOUS_FIELD_EDITOR, False, True, True, &cont_drawmode, EM_CONTINUOUS, NULL, NULL,
		{
			{"moveSfc",  E_AREA,   "area",  XuDEFAULT_CURSOR, NullWidget},
			{"dragSfc",  E_DRAG,   "drag",  XuDEFAULT_CURSOR, NullWidget},
			{"pokeSfc",  E_POKE,   "poke",  XuDEFAULT_CURSOR, NullWidget},
			{"stompSfc", E_STOMP,  "stomp", XuDEFAULT_CURSOR, NullWidget},
			{"mergeSfc", E_MERGE,  "merge", XuDEFAULT_CURSOR, NullWidget},
			{"smoothSfc",E_SMOOTH, "smooth",XuDEFAULT_CURSOR, NullWidget},
			{LABEL,      E_LABEL,  LABEL,   XuDEFAULT_CURSOR, NullWidget},
			{SAMPLE,     E_SAMPLE, SAMPLE,  XuSAMPLE_CURSOR,  NullWidget}
		}
	},{
		LCHAIN_FIELD_NO_EDIT, False, False, False, &link_drawmode, EM_LCHAIN, NULL, NULL,
		{
			{SAMPLE, E_SAMPLE, SAMPLE, XuSAMPLE_CURSOR,  NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget},
			{NULL,   E_NONE,   E_NONE, XuDEFAULT_CURSOR, NullWidget}
		}
	},{
		LCHAIN_FIELD_EDITOR, False, False, True, &link_drawmode, EM_LCHAIN, NULL, NULL,
		{
			{"addLink",   E_ADD,   "pointadd", XuPENCIL_CURSOR,  NullWidget},
			{"moveLink",  E_MOVE,  "linkmove", XuDEFAULT_CURSOR, NullWidget},
			{"modifyLink",E_MODIFY,"modify",   XuDEFAULT_CURSOR, NullWidget},
			{"mergeLink", E_MERGE, "merge",    XuDEFAULT_CURSOR, NullWidget},
			{"editNode",  E_NODES, "node",     XuDEFAULT_CURSOR, NullWidget},
			{SAMPLE,      E_SAMPLE,SAMPLE,     XuSAMPLE_CURSOR,  NullWidget},
			{NULL,        E_NONE,  E_NONE,     XuDEFAULT_CURSOR, NullWidget},
			{NULL,        E_NONE,  E_NONE,     XuDEFAULT_CURSOR, NullWidget}
		}
	}
};


/* Set the stacking control information structure. This is required
*  for the area fields.
*/
static struct {
	String order;
	long   arrow_type;
	int    arrow_dirn;
} stack_mode[] =
{
	{"TOP",    XuARROW_PLAIN|XuARROW_BAR, XuARROW_UP   },
	{"BOTTOM", XuARROW_PLAIN|XuARROW_BAR, XuARROW_DOWN }
}, stack_transient[] =
{
	{"TOP",    XuARROW_PLAIN|XuARROW_BAR, XuARROW_UP   },
	{"UP",     XuARROW_PLAIN,             XuARROW_UP   },
	{"DOWN",   XuARROW_PLAIN,             XuARROW_DOWN },
	{"BOTTOM", XuARROW_PLAIN|XuARROW_BAR, XuARROW_DOWN }
};

/* Widgets for the general editing functions
 */
static Widget groupSelect = NullWidget;
static Widget fieldSelect = NullWidget;
static Widget clearBtn;
static Widget drawModeOption;
static Widget modifyModeOption;
static Widget lineSmoothOption;
static Widget stackModeOption;
static Widget stackTransientOption;
static Widget spreadAmountOption;


/* At the moment there are two drawing modes, continuous (CONT) and point
 * to point select and close. These are defined only here and other panels
 * like scratchpad get the settings by the use of functions.
 */
static String  drawing_modes[] = {DRAWING_MODES};
static String  spread_amount   = NULL;
static Boolean send_to_ingred  = True;


/* Forward function declarations
 */
static void   action_ingred_messages (CAL, String*, int);
static void   commands_cb (Widget, XtPointer, XtPointer);
static String create_radius_of_influence_btns (Widget, int);
static void   create_line_smoothing_btns (Widget);
static void   draw_mode_cb (Widget, XtPointer, XtPointer);
static void   edit_fcn_select_cb (Widget, XtPointer, XtPointer);
static void   field_select_cb (Widget, XtPointer, XtPointer);
static GROUP  *get_field_group (FpaConfigFieldStruct *);
static void   group_select_cb (Widget, XtPointer, XtPointer);
static void   modify_mode_cb (Widget, XtPointer, XtPointer);
static void   set_active_field (const Boolean);
static void   save_edit_state (void);
static void   line_smoothing_cb (Widget, XtPointer, XtPointer);
static void   spread_amount_cb (Widget, XtPointer, XtPointer);
static void   stack_mode_cb (Widget, XtPointer, XtPointer);
static void   stack_transient_cb (Widget, XtPointer, XtPointer);
static String get_saved_option (Widget, String, String, String);
static void   set_edit_state_specific_controls(void);



/*========================== Public Functions ==============================*/


/*
*	Return a pointer to the relevant editor structure.
*/
EDITOR *GetEditor(int key )
{
	return &info[key];
}


Boolean IsActiveEditor(int key)
{
	return (AFE == &info[key]);
}


/*
*	Initialize the field and group structures.
*/
void InitFields(void)
{
	int    i, nelm;
	FpaConfigFieldStruct **fptr;

	/* Initialize the state of the editor.
	*/
	(void) IngredVaCommand(GE_ACTION, "STATE SPREAD %s", spread_amount);

	/* The GV_groups variable actually points to the second member of the
	*  set of groups as given by the GV_tlgrps variable. This is done for
	*  simplicity as it makes keeping track of code simpler (at least for
	*  me).  This first group is the global master time link group and is
	*  only used by the timelink panel.
	*/
	GV_ntlgrps = 1;
	GV_tlgrps = OneMem(GROUP*);
	GV_tlgrps[0] = OneMem(GROUP);
	GV_tlgrps[0]->name = "GLOBAL";
	GV_tlgrps[0]->label = XtNewString(XuGetLabel("globalMaster"));
	GV_tlgrps[0]->fields_link_status = NOLINKS;

	GV_active_group = NULL;
	GV_nfield = 0;
	GV_field = NULL;
	GV_active_field = NULL;

	/* Now create the manditory fields.
	*/
	nelm = depict_field_list(&fptr);
	for( i = 0; i < nelm; i++ )
	{
		AddField(fptr[i], True);
	}
}


/*
*	Used on initialization to select the active group and by default the active field.
*/
void InitToActiveGroup(void)
{
	int    group_ndx = 0;
	String buf       = NULL;

	GV_active_group = NULL;
	GV_active_field = NULL;

	if(GV_ngroups < 1) return;

	/* If permitted restore the previous editor state. Note that it is important
	 * that the GV_active_group and GV_active_field variables remain NULL as this
	 * is used in initialization code later on.
	 */
	if(GV_pref.restore_edit_state && XuStateDataGet(EDITOR_SAVE_STATE_KEY, NULL, NULL, &buf))
	{
		int    i, n, k;
		String grp_name, elm_name, elm_level, edit_cmd;

		grp_name  = strtok_arg(buf);
		elm_name  = strtok_arg(NULL);
		elm_level = strtok_arg(NULL);

		/* Find the specified group. */
		for( i = 0; i < GV_ngroups; i++ )
		{
			if(!same(grp_name, GV_groups[i]->name)) continue;
			group_ndx = i;

			/* Find the field */
			for( n = 0; n < GV_groups[i]->nfield; n++ )
			{
				if(!same(elm_name,  GV_groups[i]->field[n]->info->element->name)) continue;
				if(!same(elm_level, GV_groups[i]->field[n]->info->level->name)  ) continue;
				GV_groups[i]->afield = GV_groups[i]->field[n];
				edit_cmd  = string_arg(buf);

				/* Find the edit command */
				for( k = 0; k < NR_EDIT_BTNS; k++ )
				{
					if(!same(edit_cmd, GV_groups[i]->afield->editor->btns[k].cmd)) continue;
					/* Toggle off the previously set active edit button. This is needed
					 * here as the initialization procedure does not do this\
					 */
					XuToggleButtonSet(GV_groups[i]->afield->editor->active->w, False, False);
					GV_groups[i]->afield->editor->active = &GV_groups[i]->afield->editor->btns[k];
					break;
				}
				break;
			}
			break;
		}
	}
	FreeItem(buf);

	XuComboBoxSelectPos(groupSelect, group_ndx+1, True);
}


/*
*	Creates the group selection when fpa is running in viewer mode.
*/
void CreateViewerModeControlPanel(Widget parent)
{
	groupSelect = XmVaCreateManagedComboBox(parent, "groupSelect",
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 5,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 5,
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 15,
		NULL);

	XtAddCallback(groupSelect, XmNselectionCallback, group_select_cb, NULL);

	fieldSelect = XmVaCreateManagedComboBox(parent, "fieldSelect",
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 5,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 5,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, groupSelect,
		XmNtopOffset, 9,
		NULL);

	XtAddCallback(fieldSelect, XmNselectionCallback, field_select_cb, NULL);

	CreateSampleFieldPanel(parent, fieldSelect);
	ConfigureMainContextMenuForField();
	ShowSamplingPanel();
}


/*
*	Creates the editor control panel and the field manager panel then initializes 
*	the field control panels. This panel is what contains the various field type
*	controls - the edit action selection buttons, the command buttons, the stacking
*	order controls, line drawing action and sharpness.
*
*	NOTE: The link type selector also resides here, underneath the command buttons,
*	      but it is created and managed by the functions in field_linkChain.c. It
*   occupies the same space as the stacking order option menu, but should not appear
*	at the same time as nodes are really a type of line.
*/
void CreateEditControlPanel(Widget editorPanel)
{
	int     i, j, nbtns;
	String  mode;
	Boolean btn_defined[NR_EDIT_BTNS];
	Pixmap  lpx, spx, ipx;
	Pixel   fg, bg, ifg;
	Widget  list;
	Widget  cmdBtnManager;
	Widget  fieldEditor;
	Widget  editFcnManager;
	XuArrowAttributes attrib;

	fieldEditor = XmVaCreateForm(editorPanel, "fieldEditor",
		XmNborderWidth, 0,
		XmNmarginHeight, 0,
		XmNmarginWidth, 0,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		NULL);

	groupSelect = XmVaCreateManagedComboBox(fieldEditor, "groupSelect",
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 5,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 5,
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 15,
		NULL);

	XtAddCallback(groupSelect, XmNselectionCallback, group_select_cb, NULL);

	fieldSelect = XmVaCreateManagedComboBox(fieldEditor, "fieldSelect",
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 5,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 5,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, groupSelect,
		XmNtopOffset, 9,
		NULL);

	XtAddCallback(fieldSelect, XmNselectionCallback, field_select_cb, NULL);

	editFcnManager = XmVaCreateForm(fieldEditor, "editFcnManager",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopOffset, 14,
		XmNtopWidget, fieldSelect,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 7,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 7,
		NULL);

	/* Create the edit command select button sets.  NOTE: The button labels are
	*  specified in the resource file so the manager and button names are important
	*  and must be changed only if the resource file is modified also.
	*/

	for( i = 0; i < (int) XtNumber(info); i++ )
	{
		for(nbtns = 0, j = 0; j < NR_EDIT_BTNS; j++)
		{
			btn_defined[j] = !same(info[i].btns[j].cmd, E_NONE);
			if(btn_defined[j]) nbtns = j+1;
		}
		nbtns += nbtns%2;

		info[i].buttonManager = XmVaCreateRowColumn(editFcnManager,
			info[i].manager_name,
			XmNmappedWhenManaged, False,
			XmNborderWidth, 1,
			XmNmarginHeight, 0,
			XmNmarginWidth, 0,
			XmNradioBehavior, True,
			XmNpacking, XmPACK_COLUMN,
			XmNorientation, XmHORIZONTAL,
			XmNnumColumns, (short)(nbtns/2),
			XmNtopAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_FORM,
			NULL);

		for(j = 0; j < nbtns; j++)
		{
			lpx = spx = ipx = XmUNSPECIFIED_PIXMAP;
			if(btn_defined[j])
				ToggleButtonPixmaps(editorPanel, "edit", info[i].btns[j].pixmapBaseName, &lpx, &spx, &ipx);

			info[i].btns[j].w = XmVaCreateManagedToggleButton(info[i].buttonManager,
				info[i].btns[j].cmd,
				XmNshadowThickness, 2,
				XmNindicatorOn, False,
				XmNmarginHeight, 0,
				XmNmarginWidth, 0,
				XmNlabelType, XmPIXMAP,
				XmNlabelPixmap, lpx,
				XmNselectPixmap, spx,
				XmNselectInsensitivePixmap, ipx,
				XmNsensitive, btn_defined[j],
				NULL);

			XtAddCallback(info[i].btns[j].w, XmNvalueChangedCallback,
				edit_fcn_select_cb, (XtPointer)&info[i].btns[j]);
		}
		XtManageChild(info[i].buttonManager);
		info[i].active = info[i].btns;
		XuToggleButtonSet(info[i].active->w, True, False);
	}

	cmdBtnManager = XmVaCreateManagedRowColumn(editFcnManager, "cmdBtnManager",
		XmNmarginHeight, 0,
		XmNmarginWidth, 0,
		XmNresizeWidth, False,
		XmNentryAlignment, XmALIGNMENT_CENTER,
		XmNpacking, XmPACK_TIGHT,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	GW_editorAcceptBtn = XmVaCreateManagedPushButton(cmdBtnManager, "acceptBtn",
		XmNsensitive, False,
		NULL);
	XtAddCallback(GW_editorAcceptBtn, XmNactivateCallback, commands_cb, (XtPointer)E_UPDATE);

	GW_editorCancelBtn = XmVaCreateManagedPushButton(cmdBtnManager, "cancelBtn",
		XmNsensitive, False,
		NULL);
	XtAddCallback(GW_editorCancelBtn, XmNactivateCallback, commands_cb, (XtPointer)E_CANCEL);

	GW_editorUndoBtn = XmVaCreateManagedPushButton(cmdBtnManager, "undoBtn",
		XmNsensitive, False,
		NULL);
	XtAddCallback(GW_editorUndoBtn, XmNactivateCallback, commands_cb, (XtPointer)E_UNDO);

	clearBtn = XmVaCreateManagedPushButton(cmdBtnManager, "clearBtn",
		XmNsensitive, False,
		NULL);
	XtAddCallback(clearBtn, XmNactivateCallback, commands_cb, (XtPointer)E_CLEAR);


	/* Create the edit radius of influence (spread) selector.
	*/
	spreadAmountOption = XuVaMenuBuildOption(editFcnManager, "spreadSelect", NULL,
		XmNmappedWhenManaged, False,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, cmdBtnManager,
		XmNtopOffset, 10,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, -2,
		NULL);

	mode = create_radius_of_influence_btns(spreadAmountOption, ROI);
	spread_amount = get_saved_option(spreadAmountOption, RADIUS_OF_INFLUENCE_STATE_KEY, NULL, mode);
	XuMenuSelectItemByName(spreadAmountOption, spread_amount);


	/* This creates the stacking order controls. There are two. One to set the
	*  mode and one for transient operations like modifying an area.
	*/
	GV_stacking_order = stack_mode[0].order;

	attrib.flags         =  XuARROW_APPEARANCE|XuARROW_DIRECTION|XuARROW_WIDTH|XuARROW_HEIGHT|
							XuARROW_MARGIN_WIDTH|XuARROW_MARGIN_HEIGHT| XuARROW_FOREGROUND;
	attrib.width         = 20;
	attrib.height        = 20;
	attrib.margin_width  = 5;
	attrib.margin_height = 2;

	fg  = XuLoadColor(fieldEditor, "black");
	bg  = XuLoadColor(fieldEditor, "green");
	ifg = XuLoadColor(fieldEditor, "Gray88");

	stackModeOption = XuVaMenuBuildOption(editFcnManager, "stackMode", NULL,
		XmNsensitive, False,
		XmNmappedWhenManaged, False,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, cmdBtnManager,
		XmNtopOffset, 10,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, -2,
		NULL);

	list = XuMenuFind(stackModeOption, NoId);

	for(i = 0; i < (int) XtNumber(stack_mode); i++)
	{
		attrib.appearance = stack_mode[i].arrow_type;
		attrib.direction  = stack_mode[i].arrow_dirn;

		attrib.flags = attrib.flags | XuARROW_BACKGROUND;
		attrib.foreground = fg;
		attrib.background = bg;
		lpx  = XuCreateArrowPixmap(list, &attrib);

		attrib.flags = attrib.flags & ~XuARROW_BACKGROUND;
		attrib.foreground = ifg;
		ipx  = XuCreateArrowPixmap(list, &attrib);

		(void) XuMenuAddPixmapButton(stackModeOption, stack_mode[i].order, NoId, lpx, ipx, stack_mode_cb, INT2PTR(i));
	}

	stackTransientOption = XuVaMenuBuildOption(editFcnManager, "stackTransient", NULL,
		XmNsensitive, False,
		XmNmappedWhenManaged, False,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, cmdBtnManager,
		XmNtopOffset, 10,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, -2,
		NULL);

	list = XuMenuFind(stackTransientOption, NoId);

	for(i = 0; i < (int) XtNumber(stack_transient); i++)
	{
		attrib.appearance = stack_transient[i].arrow_type;
		attrib.direction  = stack_transient[i].arrow_dirn;

		attrib.flags = attrib.flags | XuARROW_BACKGROUND;
		attrib.foreground = fg;
		attrib.background = bg;
		lpx  = XuCreateArrowPixmap(list, &attrib);

		attrib.flags = attrib.flags & ~XuARROW_BACKGROUND;
		attrib.foreground = ifg;
		ipx  = XuCreateArrowPixmap(list, &attrib);

		(void) XuMenuAddPixmapButton(stackTransientOption, stack_transient[i].order, NoId, lpx, ipx,
									 stack_transient_cb, INT2PTR(i));
	}

	XtManageChild(editFcnManager);

	/* Create the drawing mode option menu.
	*/
	drawModeOption = XuVaMenuBuildOption(fieldEditor, "drawModeOption", NULL,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, editFcnManager,
		XmNtopOffset, 9,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 5,
		NULL);

	for(i = 0; i < (int) XtNumber(drawing_modes); i++)
	{
		PushButtonPixmaps(editorPanel, "draw", drawing_modes[i], &lpx, &ipx);
		(void) XuMenuAddPixmapButton(drawModeOption, drawing_modes[i], NoId, lpx, ipx,
									 draw_mode_cb, (XtPointer)drawing_modes[i]);
	}

	for(i = 0; i < XtNumber(drawmode_state); i++)
		drawmode_state[i]->draw = get_saved_option(drawModeOption, DRAWING_MODE_STATE_KEY,
								drawmode_state[i]->type, drawing_modes[0]);

	/* Create the modify mode option menu. Similar to drawing mode but
	*  with the addition of the "puck" size.
	*/
	modifyModeOption = XuVaMenuBuildOption(fieldEditor, "modifyModeOption", NULL,
		XmNmappedWhenManaged, False,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, editFcnManager,
		XmNtopOffset, 9,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 5,
		NULL);

	for(i = 0; i < (int) XtNumber(drawing_modes); i++)
	{
		PushButtonPixmaps(editorPanel, "draw", drawing_modes[i], &lpx, &ipx);
		(void) XuMenuAddPixmapButton(modifyModeOption, drawing_modes[i], i, lpx, ipx,
									    modify_mode_cb, (XtPointer)drawing_modes[i]);
	}

	(void)create_radius_of_influence_btns(modifyModeOption, PUCK);

	for(i = 0; i < XtNumber(drawmode_state); i++)
		drawmode_state[i]->modify = get_saved_option(modifyModeOption, MODIFY_MODE_STATE_KEY,
									drawmode_state[i]->type, drawing_modes[0]);

	/* Create the drawing mode smoothing amount option menu.
	*/
	lineSmoothOption = XuVaMenuBuildOption(fieldEditor, "smoothModeOption", NULL,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, editFcnManager,
		XmNtopOffset, 9,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 5,
		NULL);
	create_line_smoothing_btns(lineSmoothOption);

	(void) XmVaCreateManagedSeparator(fieldEditor, "sep",
		XmNseparatorType, XmSHADOW_ETCHED_IN,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, drawModeOption,
		XmNtopOffset, 9,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	XtManageChild(fieldEditor);


	/* Create the field edit command panels
	*/
	CreateContinuousFieldPanel (editorPanel, fieldEditor);
	CreateVectorFieldPanel     (editorPanel, fieldEditor);
	CreateDiscreteFieldPanel   (editorPanel, fieldEditor);
	CreateWindFieldPanel       (editorPanel, fieldEditor);
	CreateLinkChainFieldPanel  (editorPanel, fieldEditor);
	CreateLineFieldPanel       (editorPanel, fieldEditor);
	CreatePointFieldPanel      (editorPanel, fieldEditor);

	/* Create the general use special panels
	 */
	CreateCopyPasteSubpanel    (editorPanel, fieldEditor);
	CreateSampleFieldPanel     (editorPanel, fieldEditor);
	CreateSmoothingFieldPanel  (editorPanel, fieldEditor);
	CreateMergeFieldPanel      (editorPanel, fieldEditor);
	CreateLabelFieldPanel      (editorPanel, fieldEditor);

	XtManageChild(editorPanel);

	ConfigureMainContextMenuForField();

	/* Set the context default. This will be assigned by every panel and it is
	 * important to be set to NULL when leaving a panel.
	 */
	SetActiveContextMenu(FieldEditContextMenu);

	AddIngredObserver(action_ingred_messages);
}


/*
*	Adds a field to the list of editable fields in the depiction.  First the
*	edit panel for the type of field given in the parameter list is updated
*	to include the field then a	field selection button is created. The
*	selection button is part of a pulldown menu list in the editor Panel.
*	Note that when selected the	client data passed back will be the pointer
*	to the field structure.
*/
void AddField( FpaConfigFieldStruct *fs , const Boolean manditory )
{
	int        i, j;
	GROUP      *group;
	FIELD_INFO *fptr;
	static char module[] = "AddField";

	/* Check to make sure that we do not already have the requested
	*  field before we add it to our list.
	*/
	if(IsNull(fs) || IsNull(fs->element) || IsNull(fs->level)) return;
	if(IsNull(get_field_info(fs->element->name, fs->level->name))) return;
	if(NotNull(FindField(fs->element->name, fs->level->name))) return;

	/* Find group that field belongs to. There must be a group.
	*/
	group = get_field_group(fs);
	if(!group) return;

	fptr = OneMem(FIELD_INFO);
	fptr->info        = get_field_info(fs->element->name, fs->level->name);
	fptr->manditory   = manditory;
	fptr->group       = group;
	fptr->visible     = VIS_OFF;
	fptr->animate_vis = True;
	fptr->link_state  = LINKED_TO_SELF;
	fptr->link_status = NOT_LINKABLE;
	fptr->link_fld    = NULL;

	SetFieldExistance(fptr);
	SetFieldTimelinkState(fptr);

	switch(fs->element->fld_type)
	{
		case FpaC_CONTINUOUS: AddContinuousField(fptr);	break;
		case FpaC_VECTOR:     AddVectorField(fptr);     break;
		case FpaC_DISCRETE:   AddDiscreteField(fptr);	break;
		case FpaC_WIND:       AddWindField(fptr);		break;
		case FpaC_LINE:       AddLineField(fptr);		break;
		case FpaC_SCATTERED:  AddPointField(fptr);      break;
		case FpaC_LCHAIN:     AddLinkChainField(fptr);  break;
		default:
			Warning(module, "ErrorInField", fs->label);
			Warning(module, "UnknownFieldType", NULL);
			FreeItem(fptr);
			return;
	}

	/* Add the field to the group
	*/
	group->field = MoreMem(group->field, FIELD_INFO*, group->nfield+1);
	group->field[group->nfield] = fptr;
	group->nfield++;

	/* Set the group active field if necessary.
	*/
	if (!group->afield) group->afield = group->field[0];

	/* Add the field to the all fields vector.
	*/
	GV_field = MoreMem(GV_field, FIELD_INFO*, GV_nfield+1);
	GV_nfield = 0;
	for( i = 0; i < GV_ngroups; i++ )
	{
		for( j = 0; j < GV_groups[i]->nfield; j++ )
		{
			GV_field[GV_nfield] = GV_groups[i]->field[j];
			GV_nfield++;
		}
	}

	/* Add the field to the timelinkable fields list.
	*/
	if(	fptr->info->element->elem_tdep->time_dep == FpaC_NORMAL
		&& fptr->link_status != NOT_LINKABLE )
	{
		group->tlfld = MoreMem(group->tlfld, FIELD_INFO*, group->ntlfld+1);
		group->tlfld[group->ntlfld] = fptr;
		group->ntlfld++;
		if(!group->atlfld)
		{
			group->atlfld = group->tlfld[1];
			group->atlfldno = 1;
		}
		/* Set the fields_link_status to the lowest link status from all fields.
		*/
		group->fields_link_status = INTERPOLATED;
		for(i = 0; i < group->nfield; i++)
		{
			if(group->field[i]->link_status != NOT_LINKABLE)
			{
				group->fields_link_status = MIN(group->fields_link_status,
												group->field[i]->link_status);
			}
		}
	}

	/* This needs to be at the end as it does an element search and
	 * needs the field to have been fully inserted into the structures.
	 */
	SetFieldVisibility(fptr);
}


/*
*	Removes a field from the list of editable fields.
*/
void RemoveField(FIELD_INFO *field )
{
	int i, j, k;
	GROUP **grps;
	FIELD_INFO **flds;

	if (!field) return;

	/* Since we are to remove it the following must be true.
	*/
	field->exists = False;

	/* We are not allowed to remove manditory fields from the interface.
	*/
	if(field->manditory) return;

	/* Remove from the group.  If this removes all of the fields from the 
	*  group then we must destroy the group as well.
	*/
	field->group->nfield--;
	if(field->group->nfield > 0)
	{
		/* Remove from the full list of fields first.
		*/
		flds = NewMem(FIELD_INFO*, field->group->nfield);
		j = 0;
		for( i = 0; i < field->group->nfield; i++ )
		{
			if(field->group->field[j] == field) j++;
			flds[i] = field->group->field[j];
			j++;
		}
		FreeItem(field->group->field);
		field->group->field = flds;
		if(field->group->afield == field) field->group->afield = field->group->field[0];

		/* Set the fields_link_status to the lowest link status from all fields.
		*/
		field->group->fields_link_status = INTERPOLATED;
		for(i = 0; i < field->group->nfield; i++)
		{
			if(field->group->field[i]->link_status != NOT_LINKABLE)
			{
				field->group->fields_link_status = MIN(field->group->fields_link_status,
														field->group->field[i]->link_status);
			}
		}

		/* Now remove from the timelinkable field list.
		*/
		for(k = 1; k < field->group->ntlfld; k++)
		{
			if(field != field->group->tlfld[k]) continue;
			field->group->ntlfld--;
			flds = NULL;
			if(field->group->ntlfld > 1)
			{
				flds = NewMem(FIELD_INFO*, field->group->ntlfld);
				j = 1;
				for( i = 0; i < field->group->ntlfld; i++ )
				{
					if(field->group->field[j] == field) j++;
					flds[i] = field->group->tlfld[j];
					j++;
				}
			}
			FreeItem(field->group->tlfld);
			field->group->tlfld = flds;
			if(field->group->atlfld == field)
			{
				j = (field->group->ntlfld > 0) ? 1:0;
				field->group->atlfld = field->group->field[j];
				field->group->atlfldno = j;
			}
			break;
		}
	}
	else
	{
		GV_ngroups--;
		grps = NewMem(GROUP*, GV_ngroups);
		j = 0;
		for(i = 0; i < GV_ngroups; i++)
		{
			if(GV_groups[j] == field->group) j++;
			grps[i] = GV_groups[j];
			j++;
		}
		FreeItem(GV_groups);
		GV_groups = grps;
		if(GV_active_group == field->group) GV_active_group = GV_groups[0];
		FreeItem(field->group->field);
		FreeItem(field->group->tlfld);
		FreeItem(field->group);

		/* This requires explanation. When the visibleItemCount is set the
		*  widget does not behave properly and expands beyond the border of
		*  the parent. This process enforces proper behaviour.
		*/
		XuComboBoxSetString(groupSelect, GV_active_group->label);
		XuComboBoxDeleteAllItems(groupSelect);
		XtVaSetValues(groupSelect,
			XmNvisibleItemCount, MIN(GV_ngroups,20),
			XmNrightOffset, 4,
			NULL);
		for( i = 0; i < GV_ngroups; i++ )
		{
			XuComboBoxAddItem(groupSelect, GV_groups[i]->label, 0);
		}
		XtVaSetValues(groupSelect, XmNrightOffset, 5, NULL);
	}

	/* Reassign the all fields vector.
	*/
	GV_nfield = 0;
	for( i = 0; i < GV_ngroups; i++ )
	{
		for( j = 0; j < GV_groups[i]->nfield; j++ )
		{
			GV_field[GV_nfield] = GV_groups[i]->field[j];
			GV_nfield++;
		}
	}

	/* Lastly destroy the element data;
	*/
	field->destroyFcn(field);
	FreeItem(field);
}


void SetActiveFieldByIndex(int ndx)
{
	/* Note that the ComboBox has a 1 origin for selection */
	if( ndx >= 0 && ndx < GV_active_field->group->nfield)
		XuComboBoxSelectPos(fieldSelect, ndx+1, True);
}


/*
*   Resets the active field pointer. If no active field is found the
*   side bar panel manager is set  insensitive so that none of the
*   controls will work.
*/
void ResetActiveField(void)
{
	int i, nfld;
	String *elem, *level;

	if (!GV_edit_mode) return;

	/* Find out from Ingred what fields there are.
	*/
	(void) GEStatus("FIELDS", &nfld, &elem, &level, NULL);

	if (nfld < 1)
	{
		GV_active_group = NULL;
		GV_active_field = NULL;
	}
	else if(!InFieldList(GV_active_field, nfld, elem, level, NULL))
	{
		/* Try and keep the active field in the same group.
		*/
		GV_active_field = NULL;
		if(NotNull(GV_active_group))
		{
			for(i = 0; i < GV_active_group->nfield; i++)
			{
				if(GV_active_group->field[i]->info->element->elem_tdep->time_dep != FpaC_NORMAL) continue;
				if(!InFieldList(GV_active_group->field[i], nfld, elem, level, NULL)) continue;
				GV_active_group->afield = GV_active_group->field[i];
				GV_active_field = GV_active_group->afield;
				break;
			}
		}
		/* If we did not find it in the active group find the first valid one
		*  and reset the active group pointer as well.
		*/
		if(IsNull(GV_active_field))
		{
			for(i = 0; i < GV_nfield; i++)
			{
				if(GV_field[i]->info->element->elem_tdep->time_dep != FpaC_NORMAL) continue;
				if(!InFieldList(GV_field[i], nfld, elem, level, NULL)) continue;
				GV_active_group = GV_field[i]->group;
				GV_active_group->afield = GV_field[i];
				GV_active_field = GV_field[i];
				break;
			}
		}
	}

	if(NotNull(GV_active_field))
	{
		send_to_ingred = False;
		XuComboBoxSelectItem(groupSelect, GV_active_group->name, True);
		XtUnmanageChild(GW_tabFrame2);
	}
	else
	{
		XtManageChild(GW_tabFrame2);
	}

	ConfigureMainContextMenuForField();
}


/*
*	Starts up the editor and sets it to the	proper edit state for
*	whatever menu we are in.
*/
void FieldEditorStartup(void)
{
	if (!GV_active_field) return;

	if(GV_edit_mode)
	{
		XuSetCursor(GW_mapWindow, AFE->active->cursor, ON);
		/*
		 * It is possible that another panel has changed these so we must
		 * set them to the state required by the current editor. Note that
		 * this must be done before the depiction reset call below.
		 */
		if(AFE->draw_modes_used)
		{
			XuMenuSelectItemByName(drawModeOption,   AFE->draw_mode->draw);
			XuMenuSelectItemByName(modifyModeOption, AFE->draw_mode->modify);
			XuMenuSelectItemByName(lineSmoothOption, AFE->draw_mode->smoothing);
			(void) IngredVaCommand(GE_ACTION, "STATE DRAW_MODE %s %s",
					AFE->draw_mode->draw, AFE->draw_mode->smoothing);
			(void) IngredVaCommand(GE_ACTION, "STATE MODIFY_MODE %s %s",
					AFE->draw_mode->modify, AFE->draw_mode->smoothing);
		}
		set_edit_state_specific_controls();
		ResetDepictionSelection(True);
	}
	else
	{
		(void) IngredVaCommand(GE_DEPICTION, "FIELD %s %s",
			GV_active_field->info->element->name,
			GV_active_field->info->level->name);
		set_active_field(True);
	}

	SetActiveContextMenu(FieldEditContextMenu);
	ConfigureMainContextMenuForField();
}


/*
*	Called upon exit from the field editor.
*/
/*ARGSUSED*/
void FieldEditorExit(String key)
{
	(void) IngredCommand(GE_DEPICTION, "FIELD NONE NONE");

	if( GV_edit_mode )
	{
		if( NotNull(GV_active_field) &&
			NotNull(AFE) &&
			NotNull(AFE->active))
		{
			XuSetCursor(GW_mapWindow, AFE->active->cursor, OFF);
		}
		HideSecondarySequenceBar(True);
	}
	else
	{
		XuSetCursor(GW_mapWindow, XuSAMPLE_CURSOR, OFF);
	}

	SetActiveContextMenu(None);
}


/*
*    Checks to see if there is a field to actually edit. If not then
*    the editor controls are set insensitive.
*/
Boolean ValidEditField(void)
{
	Boolean state;
	
	if (!GV_edit_mode) return False;

	state = HaveFieldToEdit();
	XuToggleButtonSet(AFE->active->w, state, False);
	XtSetSensitive(AFE->buttonManager, state);
	return state;
}


/*============================= LOCAL FUNCTIONS ============================*/



/* Respond to status information from Ingred. We expect:
 *
 *     prams[0] = E_MODE or E_EDIT
 *     prams[1] = E_BUTTON
 *     parms[2] = specific button
 *     parms[3] = "ON" or "OFF"
 */
static void action_ingred_messages (CAL cal, String *parms, int nparms)
{
	if(!GV_edit_mode) return;
	
	if(same_ic(parms[0],E_MODE) && same_ic(parms[1],E_BUTTON))
	{
		String  btn = parms[2];
		Boolean on  = same_ic(parms[3],E_ON);

		if(same_ic(btn, E_DRAW) || same_ic(btn,E_ADD))
		{
			XtSetSensitive(drawModeOption,   on);
			XtSetSensitive(modifyModeOption, on);
		}
	}
	else if(same_ic(parms[0],E_EDIT) && same_ic(parms[1],E_BUTTON))
	{
		String  btn = parms[2];
		Boolean on  = same_ic(parms[3],E_ON);

		if(same_ic(btn, E_UPDATE))
		{
			XtSetSensitive(GW_editorAcceptBtn, on);
		}
		else if(same_ic(btn, E_CANCEL))
		{
			XtSetSensitive(GW_editorCancelBtn, on);
			SetTimelinkCancel(on);
			set_edit_state_specific_controls();
		}
		else if(same_ic(btn, E_UNDO))
		{
			XtSetSensitive(GW_editorUndoBtn, on);
		}
		else if(same_ic(btn, E_CLEAR))
		{
			XtSetSensitive(clearBtn, on);
		}
		UpdateAttributesEntryDialog(NULL); 
	}
}


/* The sub-panels that appear depend on the particular edit function
 * currently in effect. This function sets the appropriate panel and
 * this can be done by just keying on the active edit command.
 */
static void set_field_sub_panel(void)
{
	HideMergeFieldPanel();
	HideSamplingPanel();
	HideSmoothingPanel();
	HideLabelFieldPanel();
	HideCopyPasteSubpanel();

	if(same(AFE->active->cmd,E_AREA))
	{
		ShowCopyPasteSubpanel(SHOW_COPY_PASTE_BUTTONS_ONLY);
	}
	else if(same(AFE->active->cmd,E_MERGE))
	{
		ShowMergeFieldPanel();
	}
	else if(same(AFE->active->cmd,E_SAMPLE))
	{
		ShowSamplingPanel();
	}
	else if(same(AFE->active->cmd,E_SMOOTH))
	{
		ShowSmoothingPanel();
	}
	else if(same(AFE->active->cmd,E_LABEL))
	{
		ShowLabelFieldPanel();
	}
	else if(same(AFE->active->cmd,E_MOVE))
	{
		ShowCopyPasteSubpanel(ALL);
	}
	else if(same(AFE->active->cmd,E_ROTATE))
	{
		ShowCopyPasteSubpanel(SELECT_ALL_BUTTON_ONLY);
	}
}


/*
*  1. Controls the sensitivity of and the visibility of the stacking order
*     The modify button is only sensitive if the mode is "modify" and the
*     cancel button is active.
*
*  2. Controls the visibility of the drawing mode option menu. If in modify
*     edit mode then the puck selections are visible.
*/
static void set_edit_state_specific_controls(void)
{
	Boolean mapModifyMode     = False;
	Boolean mapStackMode      = False;
	Boolean mapStackTransient = False;

	if (!GV_edit_mode) return;

	if( IsActiveEditor(DISCRETE_FIELD_EDITOR) || IsActiveEditor(WIND_FIELD_EDITOR) )
	{
		if (InEditMode(E_DRAW) || InEditMode(E_ADD) || InEditMode(E_MOVE) || InEditMode(E_MERGE))
		{
			XtSetSensitive(stackModeOption, True);
			mapStackMode = True;
		}
		else if (InEditMode(E_MODIFY))
		{
			XtSetSensitive(stackTransientOption, XtIsSensitive(GW_editorCancelBtn));
			mapStackTransient = True;
			mapModifyMode = True;
		}
		else
		{
			XtSetSensitive(stackModeOption, False);
			XtSetSensitive(stackTransientOption, False);
		}
	}
	else if( IsActiveEditor(LINE_FIELD_EDITOR) || IsActiveEditor(LCHAIN_FIELD_EDITOR) )
	{
		mapModifyMode = InEditMode(E_MODIFY);
	}

	/* Does the editor use the stack mode?
	*/
	if(AFE->stack_order_used)
	{
		XtSetMappedWhenManaged(stackModeOption, mapStackMode);
		XtSetMappedWhenManaged(stackTransientOption, mapStackTransient);
	}
	else
	{
		XtSetMappedWhenManaged(stackModeOption, False);
		XtSetMappedWhenManaged(stackTransientOption, False);
	}

	XtSetMappedWhenManaged(drawModeOption, !mapModifyMode);
	XtSetMappedWhenManaged(modifyModeOption, mapModifyMode);
}


/*  Callback for the command buttons situated on the field command widget and in
*	the context menus. Some of the commands require special processing.
*/
/* ARGSUSED */
static void commands_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int nedit;
	String cmd = (String)client_data;

	if(same(cmd,E_UNDO))
	{
		/* An undo while in label mode needs to remove any entry dialogs */
		if(InEditMode(E_LABEL))
			DestroyAttributesEntryDialog();
		(void) GEStatus("FIELDS EDIT_POSTED", &nedit, NULL, NULL, NULL);
		if(nedit > 0) (void) IngredCommand(GE_EDIT, cmd);
	}
	else if(same(cmd,E_CLEAR))
	{
		ClearAttributeDisplayDialogs();
		DEACTIVATE_attributeDisplayDialogs();
		(void) IngredCommand(GE_EDIT, cmd);
	}
	else
	{
		(void) IngredCommand(GE_EDIT, cmd);
	}
}


/*
*   Finds the value of an option selection as found in the state store
*   file and returns this value only if it is found in the given option
*   menu widget. If not the default value is returned.
*
*   Parameters: option - The option menu
*               key1   - the first state store key for this menu.
*               key2   - The second state key
*               val    - the default selection
*
*   NOTE: The returned value is an internal static. Do NOT free.
*/
static String get_saved_option( Widget option, String key1, String key2, String val )
{
	String data;
	if( XuStateDataGet(key1, key2, NULL, &data) )
	{
		int    i, nchild;
		Widget list, *childs;

		XtVaGetValues(option, XmNsubMenuId, &list, NULL);
		XtVaGetValues(list, XtNnumChildren, &nchild, XtNchildren, &childs, NULL);

		for( i = 0; i < nchild; i++ )
		{
			if(same_ic(data,XtName(childs[i]))) val = XtName(childs[i]);
		}
		FreeItem(data);
	}
	return (val);
}


static void save_edit_state(void)
{
	if(IsNull(GV_active_group)                        ) return;
	if(IsNull(GV_active_group->afield)                ) return;
	if(IsNull(GV_active_group->afield->info)          ) return;
	if(IsNull(GV_active_group->afield->editor)        ) return;
	if(IsNull(GV_active_group->afield->editor->active)) return;

	XuVaStateDataSave(EDITOR_SAVE_STATE_KEY, NULL, NULL, "%s %s %s %s",
		GV_active_group->name,
		GV_active_group->afield->info->element->name,
		GV_active_group->afield->info->level->name,
		GV_active_group->afield->editor->active->cmd   );
}



/*
*  Given the radius of influence from 0 - 100 create a button with a
*  pixmap which displays a circle with the number next to it. Both label
*  and insensitive pixmaps are created. Note that there is a minimum
*  circle size of 2 pixels.
*/
static String create_radius_of_influence_btns( Widget parent, int type )
{
	#define UINT unsigned int

	int      depth, posn, size;
	UINT     sz;
	Position label_y;
	char     mbuf[100];
	String   d, s, rtn;
	XmString xms;
	Pixmap   pl, pi;
	Pixel    fgb, fgg, bgg, bkgnd;
	Display  *dpy;
	Window   win;
	Widget   list, tempw;
	GC       gc, xmgc;
	XtGCMask value_mask;
	XGCValues values;
	XmRenderTable rendertable;

	/* Structure to hold data required to generate the pixmap labels for the
	*  radius of influence and the modify puck option selectors. The order of
	*  these entries must correspond to the values of the defines used as
	*  input to the type parameter (ROI and PUCK).
	*/
	static struct {
		String  name;			/* resource name */
		String  default_value;	/* resource default value name */
		String  prefix;
		UINT    width, height;	/* pixmap size */
		int     xoffset;		/* arc offset */
		Boolean fill;			/* fill the circle? */
		int     label_x;		/* where label starts in pixmap */
		void    (*cbf)();		/* callback function to use */
	} pixinfo[] = {
	{ RNradiusOIList, RNradiusOIDefault, "",      44, 20, 0, False, 23, spread_amount_cb },
	{ RNpuckSizeList, RNpuckSizeDefault, "PUCK ", 50, 20, 3, True,  26, modify_mode_cb }
	};

	/* It is best to use the XmString functions here as this gives us access to XFT
	 * fonts and any render tags defined in the resource file. To do string drawing
	 * with XmStringDraw we need a render table and that is only supplied by a widget
	 * with the render table trait. This label is for temporary use in this function.
	 */
	tempw = XmCreateLabel(GW_mainWindow, "lll", NULL, 0);
	XtVaGetValues(tempw,
		XmNrenderTable, &rendertable,
		XmNbackground, &bkgnd,
		XmNdepth, &depth,
		NULL);

	list = XuMenuFind(parent, NoId);

	dpy  = XtDisplay(list);
	win  = XtWindow(list);
	gc   = XCreateGC(dpy, win, None, NULL);
	fgb  = XuLoadColor(list, "black");
	bgg  = XuLoadColor(list, "green");
	fgg  = XuLoadColor(list, "Gray78");

	/* graphics context to use with XmStringDraw */
	value_mask = GCForeground;
	values.foreground = fgb;
	xmgc = XtAllocateGC(tempw, 0, value_mask, &values, 0, 0);

	d    = XtNewString(XuGetStringResource(pixinfo[type].name,"0 25 50 75 100"));
	rtn  = XuGetStringResource(pixinfo[type].default_value,"50");

	while(NotNull(s = string_arg(d)))
	{
		if( sscanf(s, "%d", &size) != 1 ) continue;
		xms = XmStringCreate(s, VERY_SMALL_FONT);
		size = MIN(100,MAX(0,size));

		sz = (UINT)(7.0*(float)size/100.0 + 0.5) + 2;
		posn = 10 - sz;
		sz = sz*2 - 1;   /* See X manual for reason for -1 */

		/* First create the normal pixmap
		*/
		pl = XCreatePixmap(dpy, win, pixinfo[type].width, pixinfo[type].height, (UINT) depth);
		XSetForeground(dpy, gc, bgg);
		XFillRectangle(dpy, pl, gc, 0, 0, pixinfo[type].width, 20);
		XSetForeground(dpy, gc, fgb);
		XDrawArc(dpy, pl, gc, posn + pixinfo[type].xoffset, posn, sz, sz, 0, 23040);
		if(pixinfo[type].fill)
			XFillArc(dpy, pl, gc, posn + pixinfo[type].xoffset, posn, sz, sz, 0, 23040);

		label_y = ((Position) pixinfo[type].height - XmStringHeight(rendertable, xms) + 1)/2; 
		XmStringDraw(dpy, pl, rendertable, xms, xmgc,
			(Position) pixinfo[type].label_x, label_y,
			(Dimension) pixinfo[type].width, XmALIGNMENT_BEGINNING, XmSTRING_DIRECTION_L_TO_R, NULL);

		/* Now create the insensitive pixmap.
		*/
		pi = XCreatePixmap(dpy, win, pixinfo[type].width, pixinfo[type].height, (UINT) depth);
		XSetForeground(dpy, gc, bkgnd);
		XFillRectangle(dpy, pi, gc, 0, 0, pixinfo[type].width, 20);
		XSetForeground(dpy, gc, fgg);
		XDrawArc(dpy, pi, gc, posn + pixinfo[type].xoffset, posn, sz, sz, 0, 23040);
		if(pixinfo[type].fill)
			XFillArc(dpy, pi, gc, posn + pixinfo[type].xoffset, posn, sz, sz, 0, 23040);

		XmStringDraw(dpy, pl, rendertable, xms, xmgc,
			(Position) pixinfo[type].label_x, label_y,
			(Dimension) pixinfo[type].width, XmALIGNMENT_BEGINNING, XmSTRING_DIRECTION_L_TO_R, NULL);

		(void)snprintf(mbuf, 100, "%s%d", pixinfo[type].prefix, size);
		s = XtNewString(mbuf);
		(void) XuMenuAddPixmapButton(parent, s, NoId, pl, pi, pixinfo[type].cbf, (XtPointer)s);
		XmStringFree(xms);
	}

	XtReleaseGC(tempw, xmgc);
	XtDestroyWidget(tempw);
	XtFree(d);
	XFreeGC(dpy, gc);
	#undef UINT
	return rtn;
}



/*
*  Given the smoothing amount as a percentage, this creates a pixmap with a
*  normal curve as the display. The width of the curve corresponds to the
*  smoothing amount. The pixmap is hardcoded at a size of 44x20 pixels.
*/
static void create_line_smoothing_btns( Widget parent )
{
	int      depth, size, x, y, n;
	char     mbuf[100];
	double   con, dx;
	String   d, s, mode;
	Pixmap   pl, pi;
	Pixel    fgb, fgg, bgg, bkgnd;
	Display  *dpy;
	Window   win;
	Widget   list;
	GC       gc;
	XPoint   pts[40];

	list = XuMenuFind(parent, NoId);

	dpy  = XtDisplay(list);
	win  = XtWindow(list);
	gc   = XCreateGC(dpy, win, None, NULL);
	fgb  = XuLoadColor(list, "black");
	bgg  = XuLoadColor(list, "green");
	fgg  = XuLoadColor(list, "Gray78");

	XtVaGetValues(list, XmNbackground, &bkgnd, XmNdepth, &depth, NULL);

	d    = XtNewString(XuGetStringResource(RNsmoothingList,"0 25 50 75 100"));
	mode = XuGetStringResource(RNsmoothingDefault,"50");
	con  = sqrt(2 * M_PI);
	XSetLineAttributes(dpy, gc, 2, LineSolid, JoinRound, FillSolid);

	while(NotNull(s = string_arg(d)))
	{
		/* Calculate the points that will constitute the bell shaped curve
		*  that will represent our smoothing amount in the pixmap. We do
		*  double lines to increase our thickness.
		*/
		if( sscanf(s, "%d", &size) != 1 ) continue;

		size = MIN(MAX(1,size),100);
		for( n = 0, x = 3; x < 41; x++, n++ )
		{
			dx = (double)(x - 21);
			y = (int)(36.0 * (exp(-.05 * dx * dx * 20.0 / size) / con + .5));
			pts[n].x = (short) x;
			pts[n].y = (short) (35-y);
		}

		(void)snprintf(mbuf, 100, "%d", size);
		s = XtNewString(mbuf);

		/* First create the normal pixmap
		*/
		pl = XCreatePixmap(dpy, win, 44, 20, (unsigned int) depth);
		XSetForeground(dpy, gc, bgg);
		XFillRectangle(dpy, pl, gc, 0, 0, 44, 20);
		XSetForeground(dpy, gc, fgb);
		XDrawLines(dpy, pl, gc, pts, n, CoordModeOrigin);

		/* Now create the insensitive pixmap.
		*/
		pi = XCreatePixmap(dpy, win, 44, 20, (unsigned int) depth);
		XSetForeground(dpy, gc, bkgnd);
		XFillRectangle(dpy, pi, gc, 0, 0, 44, 20);
		XSetForeground(dpy, gc, fgg);
		XDrawLines(dpy, pi, gc, pts, n, CoordModeOrigin);

		(void) XuMenuAddPixmapButton(parent, s, NoId, pl, pi, line_smoothing_cb, (XtPointer)s);
	}
	XtFree(d);
	XFreeGC(dpy, gc);

	for(n = 0; n < XtNumber(drawmode_state); n++)
		drawmode_state[n]->smoothing = get_saved_option(lineSmoothOption, SMOOTHING_MODE_STATE_KEY,
										drawmode_state[n]->type, mode);
}


/*
*	Returns a pointer to the named group.  If the group does not already
*	exist one is created and initialized.
*/
static GROUP *get_field_group(FpaConfigFieldStruct *fs )
{
	int      i, j, k, n;
	char     mbuf[256];
	String   *l;
	GROUP    *nptr;
	FpaConfigGroupStruct *gdef, **list;

	static String module = "GetFieldGroup";
	static FpaConfigFieldStruct *master_field = (FpaConfigFieldStruct *)NULL;

	/* We need to create a dummy config file field entry to identify the
	*  master link field which is the first entry in the timelink fields
	*  list..
	*/
	if(!master_field)
	{
		master_field = OneMem(FpaConfigFieldStruct);
		master_field->label = XtNewString(XuGetLabel("masterLink"));
		master_field->sh_label = master_field->label;
		master_field->element = OneMem(FpaConfigElementStruct);
		master_field->element->name = "MASTER_LINK";
	}

	if(!fs->group)
	{
		Warning(module, "NoFieldGroup", NULL);
		return NULL;
	}

	/* Does the group already exist?
	*/
	for( i = 0; i < GV_ngroups; i++ )
	{
		if(!same(fs->group->name,GV_groups[i]->name)) continue;
		return (GV_groups[i]);
	}

	/* Create a new group.
	*/
	gdef = identify_group(FpaCblockFields, fs->group->name);
	if(!gdef) Warning(module, "UnknownGroup", fs->group->name);

	GV_tlgrps = MoreMem(GV_tlgrps, GROUP*, GV_ntlgrps+1);
	nptr = GV_tlgrps[GV_ntlgrps] = OneMem(GROUP);
	GV_ntlgrps++;
	GV_groups = GV_tlgrps + 1;
	GV_ngroups++;

	nptr->name = fs->group->name;
	nptr->label = (gdef)? gdef->sh_label:fs->group->name;
	nptr->visible = False;
	nptr->nfield = 0;
	nptr->field = NULL;
	nptr->afield = NULL;
	nptr->ntlfld = 1;
	nptr->tlfld  = OneMem(FIELD_INFO*);
	nptr->tlfld[0] = OneMem(FIELD_INFO);
	nptr->tlfld[0]->info = master_field;
	nptr->tlfld[0]->exists = True;
	nptr->tlfld[0]->link_state = LINKED_TO_SELF;
	nptr->tlfld[0]->link_status = NOLINKS;
	nptr->atlfld = NULL;
	nptr->atlfldno = 0;
	nptr->fields_link_status = NOLINKS;
	(void)strcpy(mbuf, "GROUP MASTER_LINK_STATUS ");
	(void)strcat(mbuf, nptr->name);
	if(GEStatus(mbuf, &i, &l, NULL, NULL) == GE_VALID)
		nptr->fields_link_status = LinkStatusStringToIndex(l[0], True);

	SetGroupVisibility(nptr);

	/* Sort the list in config file order.
	*/
	n = identify_groups_for_fields(&list);
	k = 0;
	for( i = 0; i < n; i++ )
	{
		for( j = 0; j < GV_ngroups; j++ )
		{
			if(!same(GV_groups[j]->name,list[i]->name)) continue;
			nptr = GV_groups[k];
			GV_groups[k] = GV_groups[j];
			GV_groups[j] = nptr;
			k++;
		}
	}
	(void) identify_groups_for_fields_free(&list,n);

	/* This requires explanation. When the visibleItemCount is set the
	*  widget does not behave properly and expands beyond the border of
	*  the parent. This process enforces proper behaviour.
	*/
	nptr = (GROUP *)NULL;
	XtVaSetValues(groupSelect,
		XmNvisibleItemCount, MIN(GV_ngroups,20),
		XmNrightOffset, 4,
		NULL);
	XuComboBoxDeleteAllItems(groupSelect);
	for( i = 0; i < GV_ngroups; i++ )
	{
		XuComboBoxAddItem(groupSelect, GV_groups[i]->label, 0);
		if(same(fs->group->name,GV_groups[i]->name)) nptr = GV_groups[i];
	}
	XtVaSetValues(groupSelect, XmNrightOffset, 5, NULL);
	return nptr;
}


static Boolean group_select_wp(XtPointer data)
{
	int ndx = PTR2INT(data);
	if( ndx >= 0 && GV_active_group != GV_groups[ndx] )
	{
		GV_active_group = GV_groups[ndx];
		if (!GV_active_group->afield)
			GV_active_group->afield = GV_active_group->field[0];

		/* Put the fields for the group into the field selection widget.
		*  This requires explanation. When the visibleItemCount is set the
		*  widget does not behave properly and expands beyond the border of
		*  the parent. This process enforces proper behaviour.
		*/
		XtVaSetValues(fieldSelect,
			XmNvisibleItemCount, MIN(GV_active_group->nfield,30),
			XmNrightOffset, 4,
			NULL);

		XuComboBoxSetString(fieldSelect, GV_active_group->afield->info->sh_label);
		XuComboBoxDeleteAllItems(fieldSelect);
		for( ndx = 0; ndx < GV_active_group->nfield; ndx++ )
		{
			XuComboBoxAddItem(fieldSelect, GV_active_group->field[ndx]->info->sh_label, 0);
		}
		XtVaSetValues(fieldSelect, XmNrightOffset, 5, NULL);

		/* Set the buttons in the context menu for field selection.
		 */
		ConfigureMainContextMenuFieldButtons();

		if(!send_to_ingred)
		{
			set_active_field(False);
		}
		else if(!GV_edit_mode)
		{
			set_active_field(True);
		}
		else if(PanelIsActive(ELEMENT_EDIT))
		{
			set_active_field(True);
		}
		else if(!(PanelIsActive(TIMELINK) || PanelIsActive(ANIMATION)))
		{
			set_active_field(False);
			(void) IngredVaCommand(GE_DEPICTION, "GROUP %s", GV_active_group->name);
		}
		else
		{
			set_active_field(False);
		}
		send_to_ingred = True;

		NotifyObservers(OB_GROUP_CHANGE, NULL, 0);
	}
	else
	{
		send_to_ingred = True;
	}
	save_edit_state();
	XuSetDialogCursor(GW_mainWindow, XuBUSY_CURSOR, OFF);
	return True;
}


/*
*	Used as the callback function for the group select buttons. The work
*	is farmed out to a work procedure as this gives the interface the
*	feel of a faster response.
*/
/*ARGSUSED*/
static void group_select_cb(Widget  w , XtPointer client_data, XtPointer call_data )
{
	XmComboBoxCallbackStruct *rtn = (XmComboBoxCallbackStruct *) call_data;
	int ndx = rtn->item_position - 1;
	XuSetDialogCursor(GW_mainWindow, XuBUSY_CURSOR, ON);
	(void) XtAppAddWorkProc(GV_app_context, group_select_wp, INT2PTR(ndx));
}


/*
*	Set the currently active field to the active field in the currently active
*	group.  If the send parameter is True send the data to the graphic editor.
*	If False do not send.
*/
static void set_active_field(const Boolean send )
{
	if (GV_edit_mode)
	{
		/* Remove any attribute display panels showing
		 */
		DEACTIVATE_attributeDisplayDialogs();

		/* Unmap the edit selector for the last editor
		*/
		if(GV_active_field)
		{
			GV_active_field->showFcn(False);
			XuSetCursor(GW_mapWindow, AFE->active->cursor, OFF);
			XtSetMappedWhenManaged(AFE->buttonManager, False);
		}

		GV_active_field = GV_active_group->afield;
		HideSecondarySequenceBar(False);
		XuComboBoxSetString(fieldSelect, GV_active_field->info->sh_label);
		XtSetMappedWhenManaged(AFE->buttonManager, True);

		if(AFE->draw_modes_used)
		{
			XuMenuSelectItemByName(drawModeOption,   AFE->draw_mode->draw);
			XuMenuSelectItemByName(modifyModeOption, AFE->draw_mode->modify);
			XuMenuSelectItemByName(lineSmoothOption, AFE->draw_mode->smoothing);
			(void) IngredVaCommand(GE_ACTION, "STATE DRAW_MODE %s %s",
					AFE->draw_mode->draw, AFE->draw_mode->smoothing);
			(void) IngredVaCommand(GE_ACTION, "STATE MODIFY_MODE %s %s",
					AFE->draw_mode->modify, AFE->draw_mode->smoothing);
		}

		ConfigureSamplingPanel();
		ConfigureMergeFieldPanel();
		ConfigureLabelFieldPanel();
		ConfigureCopyPastePanel();

		GV_active_field->entryFcn();
		GV_active_field->showFcn(True);
		set_edit_state_specific_controls();
		set_field_sub_panel();

		XuSetCursor(GW_mapWindow, AFE->active->cursor, ON);

		/* Set selectability of the spread mode selector depending on editor.
		*/
		XtSetMappedWhenManaged(spreadAmountOption, AFE->spread_amount_used);

		ResetDepictionSelection(send);
		ConfigureMainContextMenuForField();

		/* Let all interested parties know that the active field has changed */
		NotifyObservers(OB_FIELD_CHANGE, NULL, 0);
	}
	else
	{
		GV_active_field = GV_active_group->afield;
		NotifyObservers(OB_FIELD_CHANGE, NULL, 0);
		HideSecondarySequenceBar(False);
		XuComboBoxSetString(fieldSelect, GV_active_field->info->sh_label);
		ConfigureSamplingPanel();
		XuSetCursor(GW_mapWindow, XuSAMPLE_CURSOR, ON);
		ResetDepictionSelection(send);
		if (send) SendEditSampleCommand(NULL);
	}
}


static Boolean field_select_wp(XtPointer data)
{
	int ndx = PTR2INT(data);
	if(ndx >= 0 && GV_active_field != GV_active_group->field[ndx])
	{
		GV_active_group->afield = GV_active_group->field[ndx];
		set_active_field(True);
	}
	save_edit_state();
	XuSetDialogCursor(GW_mainWindow, XuBUSY_CURSOR, OFF);
	return True;
}

/*
*	Used as the callback function for the field select ComboBox. The work
*	is farmed out to a work procedure as this gives the interface the
*	feel of quicker response.
*/
/* ARGSUSED */
static void field_select_cb(Widget w , XtPointer unused, XtPointer call_data )
{
	XmComboBoxCallbackStruct *rtn = (XmComboBoxCallbackStruct *) call_data;
	int ndx = rtn->item_position - 1;
	XuSetDialogCursor(GW_mainWindow, XuBUSY_CURSOR, ON);
	(void) XtAppAddWorkProc(GV_app_context, field_select_wp, INT2PTR(ndx));
}

/*
*	The callback function for the edit function	selection toggle buttons.
*/
/* ARGSUSED */
static void edit_fcn_select_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	if(!((XmToggleButtonCallbackStruct *)call_data)->set) return;

	/* Filter out any push of an already active button.  This is due to
	*  Motif sending 2 events showing a set state if set button pushed.
	*/
	if( AFE->active->w == w ) return;

	NotifyObservers(OB_EDIT_FUNCTION_TO_CHANGE, NULL, 0);
	XuSetCursor(GW_mapWindow, AFE->active->cursor, OFF);
	AFE->active = (EDITBTN *)client_data;
	XuSetCursor(GW_mapWindow, AFE->active->cursor, ON);
	GV_active_field->changeEditFcn(AFE->active->cmd);
	GV_active_field->showFcn(True);
	set_edit_state_specific_controls();
	set_field_sub_panel();
	GV_active_field->sendEditCmdFcn(ACCEPT_MODE);
	save_edit_state();
}


/*ARGSUSED*/
static void draw_mode_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	AFE->draw_mode->draw = (String)client_data;
	(void) IngredVaCommand(GE_ACTION, "STATE DRAW_MODE %s %s",
			AFE->draw_mode->draw, AFE->draw_mode->smoothing);
	XuStateDataSave(DRAWING_MODE_STATE_KEY, AFE->draw_mode->type, NULL,
			AFE->draw_mode->draw);
	GV_active_field->sendEditCmdFcn(ACCEPT_MODE);
}


/*ARGSUSED*/
static void modify_mode_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	AFE->draw_mode->modify = (String)client_data;
	/*
	 * If the modification mode popup was not posted from the option menu then it
	 * must have been posted from the context menu. In this case we have a bunch
	 * of work to do before changing the mode including changing the editor type
	 * and setting the option menu to the proper value.
	 *
	 * XXXX For future reference if needed
	 *
	if(XmGetPostedFromWidget(XtParent(w)) != modifyModeOption)
	{
		int n;
		for(n = 0; n < NR_EDIT_BTNS; n++)
		{
			if (!AFE->btns[n].contextBtnId) continue;
			if (!same(AFE->btns[n].cmd,E_MODIFY)) continue;
			XmToggleButtonSetState(AFE->btns[n].w, True, True);
			break;
		}
		XuMenuSelectItemByName(modifyModeOption, AFE->draw_mode->modify);
	}
	 */
	(void) IngredVaCommand(GE_ACTION, "STATE MODIFY_MODE %s %s",
			AFE->draw_mode->modify, AFE->draw_mode->smoothing);
	XuStateDataSave(MODIFY_MODE_STATE_KEY, AFE->draw_mode->type, NULL,
			AFE->draw_mode->modify);
	GV_active_field->sendEditCmdFcn(ACCEPT_MODE);
}


/*ARGSUSED*/
static void line_smoothing_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	AFE->draw_mode->smoothing = (String)client_data;
	/*
	 * We only have one line_smoothing state for everything, but Ingred stores them
	 * independently for draw and modify so we need to set both.
	 */
	(void) IngredVaCommand(GE_ACTION, "STATE DRAW_MODE %s %s",
			AFE->draw_mode->draw, AFE->draw_mode->smoothing);
	(void) IngredVaCommand(GE_ACTION, "STATE MODIFY_MODE %s %s",
			AFE->draw_mode->modify, AFE->draw_mode->smoothing);
	XuStateDataSave(SMOOTHING_MODE_STATE_KEY, AFE->draw_mode->type, NULL,
			AFE->draw_mode->smoothing);
	GV_active_field->sendEditCmdFcn(ACCEPT_MODE);
}


/*
 *   React to the spread amount control.
 */
/*ARGSUSED*/
static void spread_amount_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	spread_amount = (String)client_data;
	(void) IngredVaCommand(GE_ACTION, "STATE SPREAD %s", spread_amount);
	GV_active_field->sendEditCmdFcn(ACCEPT_MODE);
	XuStateDataSave(RADIUS_OF_INFLUENCE_STATE_KEY, NULL, NULL, spread_amount);
}


/*
 *   React to the activation of the stack order buttons.
 */
/*ARGSUSED*/
static void stack_mode_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	GV_stacking_order = stack_mode[PTR2INT(client_data)].order;
	(void) IngredVaCommand(GE_ACTION, "STATE STACK_ORDER %s", GV_stacking_order);
	GV_active_field->sendEditCmdFcn(ACCEPT_MODE);
}


/*
 *   React to the activation of the stack order buttons.
 */
/*ARGSUSED*/
static void stack_transient_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	String order = GV_stacking_order;
	GV_stacking_order = stack_transient[PTR2INT(client_data)].order;
	GV_active_field->sendEditCmdFcn(STACK_MODE);
	GV_stacking_order = order;
}

