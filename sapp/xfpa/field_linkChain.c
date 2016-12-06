/*****************************************************************************
*
*  File:    field_linkChain.c
*
*  Purpose: Provides the program user interface for a lchain (link chain)
*           type field.
*
*  Notes:   There are two possible items in this field to manipulate, link
*           chains and link nodes. Only link chains can be created while nodes
*           can only be changed as to their type and properties, but both can
*           be moved or modified.
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
#include <Xm/ComboBox.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/TextF.h>
#include <Xm/ToggleB.h>
#include <Xm/XmpSpinBox.h>
#include <ingred.h>
#include "depiction.h"
#include "editor.h"
#include "observer.h"
#include "contextMenu.h"
#include "selector.h"

/* To shorten code statements for the active field memory */
#define AFM					GV_active_field->memory
#define ALLOW_ATTRIB_EDIT	GV_active_field->memory->add.attrib_editing
#define ALLOW_NODE_EDIT		GV_active_field->memory->node.node_editing

/* Sets the number of multiples of the interpolation interval that defines
 * the maximum and minimum limit for the link chain time delta.
 */
#define TIME_DELTA_LIMIT_RATIO	20

/* For use in callback client data */
enum { ADD_CHAIN = 1, ADD_NODE, MOD_CHAIN, EDIT_NODE };

/* For the node class type option menus.
 */
enum { normalClassId = 1, controlClassId, floatingClassId, interpClassId };

/* Memory structure for instance specific states. Note that the CAL
 * structure for the chains is stored in the normal field->cal variable.
 * In the following code the CAL_merge() function is normally used for the
 * cal's as the information that is in the cal Ingred has usually contains
 * much more information than our preset stuff.
 */
struct _memory {
	struct {
		TSTAMP  ref_time;
		CAL     node_cal;
		int     node_type_id;
		String  node_type_val;
		int     preset_selected;
		int     preset_ncal;
		CAL    *preset_chain_cal;
		CAL    *preset_node_cal;
		CAL     preset_default_cal;
		Boolean attrib_editing;		/* Is there an attribute editing file? */
	} add;
	struct {
		CAL cal;
	} modify;
	struct {
		int     time_delta;
		int     type_id;
		String  type_val;
		Widget  edit_btn;
		String  edit_mode;
		CAL     cal;
		int     preset_ncal;
		CAL    *preset_cal;
		Boolean node_editing;	/* Is there a node attribute editing file? */
	} node;
};

/* Private functions
 */
static void create_lchainSubpanel   (void);
static void depict_change_observer  (String*, int);
static void destroy_data            (FIELD_INFO*);
static void add_preset_cb           (Widget, XtPointer, XtPointer);
static void add_node_type_cb        (Widget, XtPointer, XtPointer);
static void edit_cmd_action         (String);
static void entry                   (void);
static void ingred_observer         (CAL, String*, int);
static void launch_entry_dialog_cb  (Widget, XtPointer, XtPointer);
static void mod_time_cb             (Widget, XtPointer, XtPointer);
static void modify_node_popup_btn_cb(Widget, XtPointer, XtPointer);
static void modify_time_settings_cb (Widget, XtPointer, XtPointer);
static void node_edit_command_cb    (Widget, XtPointer, XtPointer);
static void node_edit_popup_cb      (Widget, XtPointer, XtPointer);
static void modify_node_type_cb     (Widget, XtPointer, XtPointer);
static void send_edit_cmd           (int);
static void set_cal_value           (CAL cal);
static void set_add_node_cal_value  (CAL cal);
static void set_node_cal_value      (CAL cal);
static void show_panel              (Boolean);
static void show_value_cb           (Widget, XtPointer, XtPointer);
static void configure_memory        (FIELD_INFO*);
static void ref_time_cb             (Widget, XtPointer, XtPointer);
static void time_delta_cb           (Widget, XtPointer, XtPointer);

/* Private data
 */
static Boolean initializing            = False;
static Widget  setBtn                  = NullWidget;
static Widget  addSetBtn               = NullWidget;
static Widget  modifySetBtn            = NullWidget;
static Widget  nodeAddSetBtn           = NullWidget;
static Widget  nodeModifySetBtn        = NullWidget;
static Widget  nodePresetLabel         = NullWidget;
static Widget  nodePresetList          = NullWidget;
static int     nnodePresetBtns         = 0;
static Widget  nodePresets             = NullWidget;
static Widget *nodePresetPanelBtns     = NullWidgetList;
static Widget  nodePresetPopup         = NullWidget;
static Widget *nodePresetPopupBtns     = NullWidgetList;
static Widget  lchainSubpanel          = NullWidget;
static Widget  parent                  = NullWidget;
static Widget  topAttach               = NullWidget;
static Widget  addControls             = NullWidget;
static Widget  modifyControls          = NullWidget;
static int     nmodPresetBtns          = 0;
static Widget *modPresetBtns           = NullWidgetList;
static Widget  nodeControls            = NullWidget;
static Widget  pasteBtn                = NullWidget;
static Widget  addPreset               = NullWidget;
static Widget  addNodeTypeOption       = NullWidget;
static Widget  addLchainCalDisplay     = NullWidget;
static Widget  addLnodeCalDisplay      = NullWidget;
static Widget  addRefTime              = NullWidget;
static Widget  addNodeTimeDelta        = NullWidget;
static Widget  modPreset               = NullWidget;
static Widget  modLchainCalDisplay     = NullWidget;
static Widget  modRefTime              = NullWidget;
static Widget  modStartTime            = NullWidget;
static Widget  modEndTime              = NullWidget;
static Widget  modifyNodeTypeLabel     = NullWidget;
static Widget  modifyNodeTypeOption    = NullWidget;
static Widget  modifyNodeTypePopup     = NullWidget;
static Widget *modifyNodeTypePopupBtns = NullWidgetList;
static Widget  nodeModControls         = NullWidget;
static Widget  nodeCalDisplay          = NullWidget;
static Widget  nodeMoveBtn             = NullWidget;
static Widget  nodeEditLabel           = NullWidget;
static Widget  nodeEdit                = NullWidget;
static Widget  nodeEditPopup           = NullWidget;

/* The button labels and command for the selected object context menus. These are used to set
 * the popup button labels and to provide the commands to be sent to ingred associated with
 * the buttons.
 */
static CONTEXT add_context = {
	E_ADD, 0, 1,
	{
		{endChainBtnId, E_END_CHAIN},
	}
};
static CONTEXT add_new_context = {
	E_ADD, 0, 1,
	{
		{newChainBtnId, E_NEW_CHAIN},
	}
};
static CONTEXT move_context = {
	E_MOVE, 0, 4,
	{
		{translateBtnId, E_TRANSLATE},
		{rotateBtnId,    E_ROTATE},
		{cutBtnId,       E_CUT},
		{copyBtnId,      E_COPY}
	}
};
static CONTEXT modify_context = {
	E_MODIFY, 0, 2,
	{
		{setBtnId,        E_SET},
		{deleteLineBtnId, E_DELETE}
	}
};
static CONTEXT merge_context = {
	E_MERGE, 0, 3,
	{
		{mergeBtnId,     E_MERGE},
		{translateBtnId, E_TRANSLATE},
		{rotateBtnId,    E_ROTATE}
	}
};
static CONTEXT delete_context = {
	E_MODIFY, 0, 1,
	{
		{deleteLineBtnId, E_DELETE}
	}
};
static CONTEXT node_edit_context = {
	E_NODES, 0, 1,
	{
		{setBtnId, E_SET}
	}
};


/* The create function actually just sets the parent and top attachment
 * widgets. The creation is not done until a field of the appropriate
 * type is actually added.
 */
void CreateLinkChainFieldPanel(Widget p, Widget t)
{
	parent    = p;
	topAttach = t;
}


/* The given field is to be added as a link chain element.
*/
void AddLinkChainField(FIELD_INFO  *field )
{
	int editor = (GV_edit_mode && field->info->element->elem_detail->editor)? LCHAIN_FIELD_EDITOR:LCHAIN_FIELD_NO_EDIT;

	create_lchainSubpanel();

	field->entryFcn		  = entry;
	field->showFcn		  = show_panel;
	field->sendEditCmdFcn = send_edit_cmd;
	field->changeEditFcn  = edit_cmd_action;
	field->destroyFcn	  = destroy_data;
	field->editor		  = GetEditor(editor);
	field->cal            = CAL_create_by_edef(field->info->element);

	configure_memory(field);
	SetFieldSampleInfo(field, FpaCsampleControlAttribType, AttribAll);
	/*
	 * These are the default values for the start and end times.
	 */
	CAL_set_attribute(field->cal, AttribLchainStartTime, "0");
	CAL_set_attribute(field->cal, AttribLchainEndTime,   "0");
}



/*==================== LOCAL STATIC FUNCTIONS ===========================*/



/* Create the panel and the controls for the panel for the link chain which
 * resides under the general editor controls created in panel_fieldEdit.c.
 */
static void create_lchainSubpanel()
{
	int    n;
	Widget btn, label, rc, frame, form, sw;
	Widget modBtn, deleteBtn, copyBtn, showBtn;

	static XuMenuItemStruct add_node_type_options[] = {
		{"classNormal",  &xmPushButtonWidgetClass,0,None,normalClassId,  add_node_type_cb,INT2PTR(normalClassId),  NULL},
		{"classControl", &xmPushButtonWidgetClass,0,None,controlClassId, add_node_type_cb,INT2PTR(controlClassId), NULL},
		NULL
	};

	static XuMenuItemStruct modify_node_type_options[] = {
		{"classNormal",  &xmPushButtonWidgetClass,0,None,normalClassId,  modify_node_type_cb,INT2PTR(normalClassId),  NULL},
		{"classControl", &xmPushButtonWidgetClass,0,None,controlClassId, modify_node_type_cb,INT2PTR(controlClassId), NULL},
		{"classFloating",&xmPushButtonWidgetClass,0,None,floatingClassId,modify_node_type_cb,INT2PTR(floatingClassId),NULL},
		{"classInterp",  &xmPushButtonWidgetClass,0,None,interpClassId,  modify_node_type_cb,INT2PTR(interpClassId),  NULL},
		NULL
	};

	if (lchainSubpanel) return;

	/* This is the parent panel for all of the controls. It could have
	 * been coded without this, but it makes some actions like making
	 * any visible panel insensitive easier to code.
	 */
	lchainSubpanel = XmVaCreateForm(parent, "linkChainPanel",
		XmNverticalSpacing, 0,
		XmNhorizontalSpacing, 0,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, topAttach,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	/* -----------------------------------------------------------
	 * The panel for the controls associated with the selection of
	 * the "add" edit command button.
	 */
	addControls = XmVaCreateForm(lchainSubpanel, "lchainAdd",
		XmNverticalSpacing, 3,
		XmNhorizontalSpacing, 6,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	
	label = XmVaCreateManagedLabel(addControls, "memory",
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 6,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);
	
	addPreset = XmVaCreateManagedComboBox(addControls, "addPreset",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	XtAddCallback(addPreset, XmNselectionCallback, add_preset_cb, NULL);
	
	label = XmVaCreateManagedLabel(addControls, "lchainCalDisplay",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, addPreset,
		XmNtopOffset, 15,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	addLchainCalDisplay = XmVaCreateManagedTextField(addControls, "lchainText",
		XmNcursorPositionVisible, False,
		XmNeditable, False,
		XmNmarginWidth, 4,
		XmNvalue, "",
		XmNpendingDelete, False,
		XmNcursorPositionVisible, False,
		XmNwordWrap, True,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	addSetBtn = XmVaCreateManagedPushButton(addControls, "setBtn",
		XmNmarginHeight, 2,
		XmNmarginWidth, 5,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, addLchainCalDisplay,
		NULL);

	XtAddCallback(addSetBtn, XmNactivateCallback, launch_entry_dialog_cb, INT2PTR(ADD_CHAIN));

	showBtn = XmVaCreateManagedPushButton(addControls, "showBtn",
		XmNmarginHeight, 2,
		XmNmarginWidth, 5,
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, addLchainCalDisplay,
		NULL);

	XtAddCallback(showBtn, XmNarmCallback,    show_value_cb, INT2PTR(ADD_CHAIN));
	XtAddCallback(showBtn, XmNdisarmCallback, show_value_cb, INT2PTR(ADD_CHAIN));
		
	label = XmVaCreateManagedLabel(addControls, "lnodeCalDisplay",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, showBtn,
		XmNtopOffset, 15,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	addLnodeCalDisplay = XmVaCreateManagedTextField(addControls, "lnodeText",
		XmNcursorPositionVisible, False,
		XmNeditable, False,
		XmNmarginWidth, 4,
		XmNvalue, "",
		XmNpendingDelete, False,
		XmNcursorPositionVisible, False,
		XmNwordWrap, True,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	nodeAddSetBtn = XmVaCreateManagedPushButton(addControls, "setBtn",
		XmNmarginHeight, 2,
		XmNmarginWidth, 5,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, addLnodeCalDisplay,
		NULL);

	XtAddCallback(nodeAddSetBtn, XmNactivateCallback, launch_entry_dialog_cb, INT2PTR(ADD_NODE));

	showBtn = XmVaCreateManagedPushButton(addControls, "showBtn",
		XmNmarginHeight, 2,
		XmNmarginWidth, 5,
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, addLnodeCalDisplay,
		NULL);

	XtAddCallback(showBtn, XmNarmCallback,    show_value_cb, INT2PTR(ADD_NODE));
	XtAddCallback(showBtn, XmNdisarmCallback, show_value_cb, INT2PTR(ADD_NODE));
		
	label = XmVaCreateManagedLabel(addControls, "refTime",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, showBtn,
		XmNtopOffset, 15,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	addRefTime = CreateTargetTimeControl(addControls, DATE_TO_MINUTE_NO_LABEL, ref_time_cb,			
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, -30,
		NULL);

	label = XmVaCreateManagedLabel(addControls, "nodeType",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, addRefTime,
		XmNtopOffset, 15,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	addNodeTypeOption = XuVaMenuBuildOption(addControls, "dnto", add_node_type_options,
		XmNresizable, False,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
		
	label = XmVaCreateManagedLabel(addControls, "timeDelta",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, addNodeTypeOption,
		XmNtopOffset, 15,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	addNodeTimeDelta = XmpVaCreateManagedSpinBox(addControls, "td",
		XmNcolumns, 5,
		XmNeditable, False,
		XmNminimum, GV_interp_time_delta * -TIME_DELTA_LIMIT_RATIO,
		XmNmaximum, GV_interp_time_delta * TIME_DELTA_LIMIT_RATIO,
		XmNincrement, GV_interp_time_delta,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(addNodeTimeDelta, XmNvalueChangedCallback, time_delta_cb, NULL);

	/* -----------------------------------------------------------
	 * The panel for the controls associated with the selection of
	 * the "modify" edit command button.
	 */
	modifyControls = XmVaCreateForm(lchainSubpanel, "lchainMod",
		XmNsensitive, False,
		XmNverticalSpacing, 3,
		XmNhorizontalSpacing, 6,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
		
	label = XmVaCreateManagedLabel(modifyControls, "lchainCalDisplay",
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 6,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	modLchainCalDisplay = XmVaCreateManagedTextField(modifyControls, "lchainText",
		XmNcursorPositionVisible, False,
		XmNeditable, False,
		XmNmarginWidth, 4,
		XmNvalue, "",
		XmNpendingDelete, False,
		XmNcursorPositionVisible, False,
		XmNwordWrap, True,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	modifySetBtn = XmVaCreateManagedPushButton(modifyControls, "setBtn",
		XmNmarginHeight, 2,
		XmNmarginWidth, 5,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, modLchainCalDisplay,
		NULL);

	XtAddCallback(modifySetBtn, XmNactivateCallback, launch_entry_dialog_cb, INT2PTR(MOD_CHAIN));

	showBtn = XmVaCreateManagedPushButton(modifyControls, "showBtn",
		XmNmarginHeight, 2,
		XmNmarginWidth, 5,
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, modLchainCalDisplay,
		NULL);

	XtAddCallback(showBtn, XmNarmCallback,    show_value_cb, INT2PTR(MOD_CHAIN));
	XtAddCallback(showBtn, XmNdisarmCallback, show_value_cb, INT2PTR(MOD_CHAIN));

	frame = XmVaCreateManagedFrame(modifyControls, "timeSetFrame",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, showBtn,
		XmNtopOffset, 15,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	(void) XmVaCreateManagedLabel(frame, "timeSettings",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	form = XmVaCreateForm(frame, "timeSetForm",
		XmNverticalSpacing, 5,
		XmNhorizontalSpacing, 5,
		NULL);
		
	label = XmVaCreateManagedLabel(form, "reference",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, showBtn,
		XmNtopOffset, 5,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	modRefTime = CreateTargetTimeControl(form, DATE_TO_MINUTE_NO_LABEL, NULL,			
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, -40,
		NULL);
		
	label = XmVaCreateManagedLabel(form, "startTime",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, modRefTime,
		XmNtopOffset, 15,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	modStartTime = XmpVaCreateManagedSpinBox(form, "st",
		XmNcolumns, 5,
		XmNeditable, False,
		XmNminimum, INT_MIN,
		XmNmaximum, INT_MAX,
		XmNincrement, GV_interp_time_delta,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(modStartTime, XmNvalueChangedCallback, mod_time_cb, NULL);

	modEndTime = XmpVaCreateManagedSpinBox(form, "et",
		XmNcolumns, 5,
		XmNeditable, False,
		XmNminimum, INT_MIN,
		XmNmaximum, INT_MAX,
		XmNincrement, GV_interp_time_delta,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, 0,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 0,
		NULL);
	XtAddCallback(modEndTime, XmNvalueChangedCallback, mod_time_cb, NULL);
		
	(void) XmVaCreateManagedLabel(form, "endTime",
		XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, modEndTime,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, modEndTime,
		XmNbottomOffset, 0,
		NULL);

	setBtn = XmVaCreateManagedPushButton(form, "setBtn",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, modEndTime,
		XmNtopOffset, 15,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 35,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 35,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNbottomOffset, 6,
		NULL);
	XtAddCallback(setBtn, XmNactivateCallback, modify_time_settings_cb, NULL);

	label = XmVaCreateManagedLabel(modifyControls, "memory",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, frame,
		XmNtopOffset, 15,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	sw = XmVaCreateManagedScrolledWindow(modifyControls, "sw",
		XmNborderWidth, 1,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	modPreset = XmVaCreateManagedRowColumn(sw, "mpc", NULL);

	XtManageChild(form);

	/* -----------------------------------------------------------
	 * The panel for the controls associated with the selection of
	 * the "link node" edit command button.
	 */
	nodeControls = XmVaCreateForm(lchainSubpanel, "lnodeCmd",
		XmNverticalSpacing, 3,
		XmNhorizontalSpacing, 6,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	
	nodeEdit = XmVaCreateManagedFrame(nodeControls, "timeSetFrame",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
		
	nodeEditLabel = XmVaCreateManagedLabel(nodeEdit, "nodeEdit",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	rc = XmVaCreateManagedRowColumn(nodeEdit, "rc",
		XmNspacing, 5,
		XmNradioBehavior, True,
		XmNpacking, XmPACK_COLUMN,
		XmNnumColumns, 2,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	nodeMoveBtn = XmVaCreateManagedToggleButton(rc, "moveBtn", XmNmarginHeight, 0, NULL);
	XtAddCallback(nodeMoveBtn, XmNvalueChangedCallback, node_edit_command_cb, (XtPointer)E_MOVE);

	modBtn = XmVaCreateManagedToggleButton(rc, "modBtn", XmNmarginHeight, 0, NULL);
	XtAddCallback(modBtn, XmNvalueChangedCallback, node_edit_command_cb, (XtPointer)E_MODIFY);

	deleteBtn = XmVaCreateManagedToggleButton(rc, "deleteBtn", XmNmarginHeight, 0, NULL);
	XtAddCallback(deleteBtn, XmNvalueChangedCallback, node_edit_command_cb, (XtPointer)E_DELETE);

	copyBtn = XmVaCreateManagedToggleButton(rc, "copyBtn", XmNmarginHeight, 0, XmNsensitive, False, NULL);
	XtAddCallback(copyBtn, XmNvalueChangedCallback, node_edit_command_cb, (XtPointer)E_COPY);

	pasteBtn = XmVaCreateManagedToggleButton(rc, "pasteBtn", XmNmarginHeight, 0, XmNsensitive, False, NULL);
	XtAddCallback(pasteBtn, XmNvalueChangedCallback, node_edit_command_cb, (XtPointer)E_PASTE);

	showBtn = XmVaCreateManagedToggleButton(rc, "showBtn", XmNmarginHeight, 0, NULL);
	XtAddCallback(showBtn, XmNvalueChangedCallback, node_edit_command_cb, (XtPointer)E_SHOW);

	/* Create the main context menu pullright menu. THis duplicates the action selections
	 * above in the context menu so that the user can change the action from within the
	 * pullright context menu.
	 */
	nodeEditPopup = CreateMainContextMenuAuxCascadePulldown("nap");

	btn = XmVaCreateManagedPushButton(nodeEditPopup, "moveBtn", NULL);
	XtAddCallback(btn, XmNactivateCallback, node_edit_popup_cb, (XtPointer) nodeMoveBtn);

	btn = XmVaCreateManagedPushButton(nodeEditPopup, "modBtn", NULL);
	XtAddCallback(btn, XmNactivateCallback, node_edit_popup_cb, (XtPointer) modBtn);

	btn = XmVaCreateManagedPushButton(nodeEditPopup, "copyBtn", NULL);
	XtAddCallback(btn, XmNactivateCallback, node_edit_popup_cb, (XtPointer) copyBtn);

	btn = XmVaCreateManagedPushButton(nodeEditPopup, "pasteBtn", NULL);
	XtAddCallback(btn, XmNactivateCallback, node_edit_popup_cb, (XtPointer) pasteBtn);

	btn = XmVaCreateManagedPushButton(nodeEditPopup, "showBtn", NULL);
	XtAddCallback(btn, XmNactivateCallback, node_edit_popup_cb, (XtPointer) showBtn);

	btn = XmVaCreateManagedPushButton(nodeEditPopup, "deleteBtn", NULL);
	XtAddCallback(btn, XmNactivateCallback, node_edit_popup_cb, (XtPointer) deleteBtn);

	/* The node type and attribute controls are placed within a form so that they and their
	 * labels can easily be set sensitive or insensitive as these are only active when in
	 * modify mode and ingred has notified us that a chain has been selected.
	 */
	nodeModControls = XmVaCreateForm(nodeControls, "lnodeClassForm",
		XmNsensitive, False,
		XmNverticalSpacing, 3,
		XmNhorizontalSpacing, 0,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, nodeEdit,
		XmNtopOffset, 15,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	modifyNodeTypeLabel = XmVaCreateManagedLabel(nodeModControls, "nodeType",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, addRefTime,
		XmNtopOffset, 15,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	modifyNodeTypeOption = XuVaMenuBuildOption(nodeModControls, "dnto", modify_node_type_options,
		XmNresizable, False,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, modifyNodeTypeLabel,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	label = XmVaCreateManagedLabel(nodeModControls, "lnodeCalDisplay",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, modifyNodeTypeOption,
		XmNtopOffset, 15,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	nodeCalDisplay = XmVaCreateManagedTextField(nodeModControls, "lnodeText",
		XmNcursorPositionVisible, False,
		XmNeditable, False,
		XmNmarginWidth, 4,
		XmNvalue, "",
		XmNpendingDelete, False,
		XmNcursorPositionVisible, False,
		XmNwordWrap, True,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	nodeModifySetBtn = XmVaCreateManagedPushButton(nodeModControls, "setBtn",
		XmNmarginHeight, 2,
		XmNmarginWidth, 5,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, nodeCalDisplay,
		NULL);

	XtAddCallback(nodeModifySetBtn, XmNactivateCallback, launch_entry_dialog_cb, INT2PTR(EDIT_NODE));

	showBtn = XmVaCreateManagedPushButton(nodeModControls, "showBtn",
		XmNmarginHeight, 2,
		XmNmarginWidth, 5,
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, nodeCalDisplay,
		NULL);

	XtAddCallback(showBtn, XmNarmCallback,    show_value_cb, INT2PTR(EDIT_NODE));
	XtAddCallback(showBtn, XmNdisarmCallback, show_value_cb, INT2PTR(EDIT_NODE));

	nodePresetLabel = XmVaCreateManagedLabel(nodeModControls, "memory",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, showBtn,
		XmNtopOffset, 15,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	nodePresetList = XmVaCreateManagedScrolledWindow(nodeModControls, "sw",
		XmNborderWidth, 1,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, nodePresetLabel,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	nodePresets = XmVaCreateManagedRowColumn(nodePresetList, "dnto", NULL);

	XtManageChild(nodeModControls);
	
	/* Create the popup menus that are used in the action context menu to mirror the
	 * predefined node cal values and node types that will be in nodePresets. These
	 * will only become visible when a node has been selected for modification.
	 */
	nodePresetPopup = CreateMainContextMenuAuxCascadePulldown("ntp");
	modifyNodeTypePopup = CreateMainContextMenuAuxCascadePulldown("ntp");

	/* Create the buttons in the context menu popup that mirror the ones in the
	 * node type option menu on the control panel. Note that callbacks are needed
	 * to set the option menu display and to set the selected node as the menu
	 * set call does not have the option to force a callback when it is changed.
	 */
	modifyNodeTypePopupBtns = NewWidgetArray(XtNumber(modify_node_type_options));
	for(n = 0; n < XtNumber(modify_node_type_options); n++)
	{
		modifyNodeTypePopupBtns[n] =
			XmVaCreateManagedPushButton(modifyNodeTypePopup, modify_node_type_options[n].name, NULL);
		XtAddCallback(modifyNodeTypePopupBtns[n], XmNactivateCallback,
			modify_node_popup_btn_cb, modify_node_type_options[n].client_data);
		XtAddCallback(modifyNodeTypePopupBtns[n], XmNactivateCallback,
			modify_node_type_cb, modify_node_type_options[n].client_data);
	}

	AddObserver(OB_DEPICTION_CHANGE, depict_change_observer);
	AddIngredObserver(ingred_observer);
}


/* This is a separate function so that the add field function above stays clear of
 * the complication the code here introduces.
 */
static void configure_memory( FIELD_INFO *field)
{
	int      i, n;
	String   fname         = NULL;
	String   path          = NULL;
	String   ptr           = NULL;
	String  *node_block_id = NULL;
	char     node_ref[32];
	FILE    *fp            = NULL;
	CAL      cal           = NullCal;
	CAL      node_cal      = NullCal;
	Boolean  valid         = False;
	Boolean  is_default    = False;
	String   element_label  = field->info->element->label;
	/*
	 * The m variable us used to make the variable references shorter and less confusing.
	 */
	struct _memory *m = field->memory = OneMem(struct _memory);

	m->add.node_cal       = CAL_create_default(); /* XXX Temporary until Brian thinks of something else !!! */
	m->add.node_type_id   = normalClassId;
	m->add.node_type_val  = FpaNodeClass_Normal;
	m->add.attrib_editing = MenuFileExists(ENTRY_ATTRIBUTES|MODIFY_ATTRIBUTES, field);
	m->modify.cal         = NullCal;
	m->node.cal           = CAL_create_default(); /* XXX Temporary until Brian thinks of something else !!! */
	m->node.type_id       = normalClassId;
	m->node.type_val      = FpaNodeClass_Unknown;
	m->node.edit_btn      = nodeMoveBtn;
	m->node.edit_mode     = E_MOVE;
	m->node.time_delta    = GV_interp_time_delta;
	m->node.node_editing  = MenuFileExists(NODE_ENTRY_ATTRIBUTES, field);

	if(!GV_edit_mode || !field->info->element->elem_detail->editor) return;

	fname = field->info->element->elem_detail->editor->type.lchain->memory_file;
	if(blank(fname)) return;

	path = get_file(MEMORY_CFG, fname);
	if(blank(path))
	{
		pr_error(element_label,"Unable to find predefined attribute memory file \"%s\"\n", fname);
		return;
	}

	fp = fopen(path,"r");
	if(!fp)
	{
		pr_error(element_label,"Unable to open predefined attribute memory file \"%s\"\n", path);
		return;
	}

	/* Scan through the file reading all of the predefined node attribute blocks. This
	 * needs to be done first as the link chain predefined blocks refer to the node
	 * block identifier.
	 */
	while((ptr = ReadLine(fp)))
	{
		String  s, key, val;

		if((s = strchr(ptr,'='))) *s = ' ';
		key = strtok_arg(ptr);
		val = strtok_arg(NULL);

		if(same_ic(key, "memory"))
		{
			valid = False;
			if(cal)
			{
				node_block_id = MoreStringArray(node_block_id, m->node.preset_ncal+1);
				node_block_id[m->node.preset_ncal] = XtNewString(node_ref);
				m->node.preset_cal = MoreCalArray(m->node.preset_cal, m->node.preset_ncal+1);
				m->node.preset_cal[m->node.preset_ncal] = cal;
				m->node.preset_ncal++;
				cal = NullCal;
			}
			if(same_ic(val, "link_node"))
			{
				(void) safe_strcpy(node_ref, strtok_arg(NULL));
				valid = True;
			}
		}
		/*
		else if(valid && valid_edef_attribute(field->info->element, key))
		*/
		else if(valid)
		{
			if(!cal) cal = CAL_create_default(); /* XXX temporary XXX */
			CAL_add_attribute(cal, key, val);
		}
	}
	if(cal)
	{
		node_block_id = MoreStringArray(node_block_id, m->node.preset_ncal+1);
		node_block_id[m->node.preset_ncal] = XtNewString(node_ref);
		m->node.preset_cal = MoreCalArray(m->node.preset_cal, m->node.preset_ncal+1);
		m->node.preset_cal[m->node.preset_ncal] = cal;
		m->node.preset_ncal++;
	}
	/*
	 * Check for duplicate node ids
	 */
	for(i = 0; i < m->node.preset_ncal; i++)
	{
		for(n = 0; n < m->node.preset_ncal; n++)
		{
			if( i == n ) continue;
			if(same(node_block_id[i],node_block_id[n]))
			{
				pr_error(element_label,"Duplicate node block identifier \"%s\" in memory file %s\n",
						node_block_id[i], path);
			}
		}
	}

	/*
	 * Scan through the file again reading the link chain predefined attribute blocks.
	 */
	cal = NullCal;
	rewind(fp);
	while((ptr = ReadLine(fp)))
	{
		String  s, key, val;

		if((s = strchr(ptr,'='))) *s = ' ';
		key = strtok_arg(ptr);
		val = strtok_arg(NULL);

		if(same_ic(key, "memory"))
		{
			valid = False;
			if(cal)
			{
				if(!node_cal)
					pr_warning(element_label,"No node is associated with predefined link chain %d in memory file %s.\n",
						m->add.preset_ncal+1, path);
				m->add.preset_chain_cal = MoreCalArray(m->add.preset_chain_cal, m->add.preset_ncal+1);
				m->add.preset_node_cal = MoreCalArray(m->add.preset_node_cal, m->add.preset_ncal+1);
				m->add.preset_chain_cal[m->add.preset_ncal] = cal;
				if(is_default)
					m->add.preset_default_cal = m->add.preset_chain_cal[m->add.preset_ncal];
				m->add.preset_node_cal[m->add.preset_ncal] = node_cal;
				m->add.preset_ncal++;
				cal = NullCal;
				node_cal = NullCal;
				is_default = False;
			}
			if(same_ic(val, "link_chain"))
			{
				is_default = same_ic(strtok_arg(NULL),"default");
				valid = True;
			}
		}
		else if(valid)
		{
			if(same_ic(key,"link_node"))
			{
				int n;
				for(node_cal = NullCal, n = 0; n < m->node.preset_ncal && node_cal == NullCal; n++)
				{
					if(same_ic(val,node_block_id[n])) node_cal = m->node.preset_cal[n];
				}
			}
			else if(valid_edef_attribute(field->info->element, key))
			{
				if(!cal) cal = CAL_create_by_edef(field->info->element);
				CAL_add_attribute(cal, key, val);
			}
		}
	}
	if(cal)
	{
		m->add.preset_chain_cal = MoreCalArray(m->add.preset_chain_cal, m->add.preset_ncal+1);
		m->add.preset_node_cal = MoreCalArray(m->add.preset_node_cal, m->add.preset_ncal+1);
		m->add.preset_chain_cal[m->add.preset_ncal] = cal;
		if(is_default)
			m->add.preset_default_cal = m->add.preset_chain_cal[m->add.preset_ncal];
		m->add.preset_node_cal[m->add.preset_ncal] = node_cal;
		m->add.preset_ncal++;
	}
	(void) fclose(fp);
	FreeList(node_block_id, m->node.preset_ncal);
}


/* Function activated by the ingred observer registration when the active
 * depiction changes.
 */
/*ARGSUSED*/
static void depict_change_observer( String *parms, int nparms )
{
	if(!IsActiveEditor(LCHAIN_FIELD_EDITOR)) return;
	(void) safe_strcpy(AFM->add.ref_time, ActiveDepictionTime(FIELD_INDEPENDENT));
	TargetTimeSetStrTime(addRefTime, AFM->add.ref_time, True);
}



/* Frees up any resources assigned by this editor when the field is
*  removed from the system.
*/
/*ARGSUSED*/
static void destroy_data(FIELD_INFO *field )
{
	int n;
	(void) CAL_destroy(field->memory->add.node_cal);
	(void) CAL_destroy(field->memory->modify.cal);
	(void) CAL_destroy(field->memory->node.cal);
	for(n = 0; n < field->memory->add.preset_ncal; n++)
	{
		(void) CAL_destroy(field->memory->add.preset_chain_cal[n]);
	}
	FreeItem(field->memory->add.preset_chain_cal);
	FreeItem(field->memory->add.preset_node_cal);
	for(n = 0; n < field->memory->node.preset_ncal; n++)
	{
		(void) CAL_destroy(field->memory->node.preset_cal[n]);
	}
	FreeItem(field->memory->node.preset_cal);
	FreeItem(field->memory);
}


/*	The following two function show the attributes associated with the chains
 *	and the nodes while the user is pushing the button.
 */
/*ARGSUSED*/
static void show_value_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	if( ((XmPushButtonCallbackStruct *)call_data)->reason == XmCR_ARM )
	{
		CAL cal;
		switch(PTR2INT(client_data))
		{
			case ADD_CHAIN: cal = GV_active_field->cal; break;
			case ADD_NODE:  cal = AFM->add.node_cal;   break;
			case MOD_CHAIN: cal = AFM->modify.cal;     break;
			case EDIT_NODE: cal = AFM->node.cal;       break;
		}
		DisplayAttributesPopup(lchainSubpanel, True, cal);
	}
	else
	{
		DisplayAttributesPopup(lchainSubpanel, False, NULL);
	}
}


/*ARGSUSED*/
static void launch_entry_dialog_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	switch(PTR2INT(client_data))
	{
		case ADD_CHAIN: ACTIVATE_linkChainAttributesDialog(lchainSubpanel, GV_active_field->cal, set_cal_value);       break;
		case ADD_NODE:  ACTIVATE_linkNodeAttributesDialog (lchainSubpanel, AFM->add.node_cal, set_add_node_cal_value); break;
		case MOD_CHAIN: ACTIVATE_linkChainAttributesDialog(lchainSubpanel, AFM->modify.cal, set_cal_value);            break;
		case EDIT_NODE: ACTIVATE_linkNodeAttributesDialog (lchainSubpanel, AFM->node.cal, set_node_cal_value);         break;
	}
}


/* Callback to set the reference time of the link chain.
 */
/*ARGSUSED*/
static void ref_time_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	(void) safe_strcpy(AFM->add.ref_time, TargetTimeGetStrTime(w));
	CAL_set_attribute(GV_active_field->cal, AttribLchainReference, AFM->add.ref_time);
	if(InEditMode(E_ADD)) send_edit_cmd(ACCEPT_MODE);
}


/* Put the user label into the given text widget, or if not defined output
 * a message so that we know what the problem is.
 */
static void set_cal_text_label(Widget text, CAL cal)
{
	String label = CAL_get_attribute(cal, AttribUserlabel);
	if(CAL_no_value(label)) label = CAL_get_attribute(cal, AttribAutolabel);
	if(CAL_no_value(label)) label = "No Label Defined";
	XmTextSetString(text, label);
}


/* Parses the cal structure to set the id and class for a node.
 */
static void get_node_type_from_cal(CAL cal, int *id, String *val)
{
	String node_type;

	if (!cal) return;

	node_type = CAL_get_attribute(cal, AttribLnodeType);

	if(same(node_type,FpaNodeClass_Normal))
	{
		*id  = normalClassId;
		*val = FpaNodeClass_Normal;
	}
	else if(same(node_type,FpaNodeClass_Control))
	{
		*id  = controlClassId;
		*val = FpaNodeClass_Control;
	}
	else if(same(node_type,FpaNodeClass_Floating))
	{
		*id  = floatingClassId;
		*val = FpaNodeClass_Floating;
	}
	else if(same(node_type,FpaNodeClass_Interp))
	{
		*id  = interpClassId;
		*val = FpaNodeClass_Interp;
	}
	else
	{
		*id  = normalClassId;
		*val = FpaNodeClass_Normal;
	}
}


/* Callback function for the link chain preset attributes selector.
 */
/*ARGSUSED*/
static void add_preset_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmComboBoxCallbackStruct *rtn = (XmComboBoxCallbackStruct *) call_data;
	if(rtn->item_position < 1) return;
	AFM->add.preset_selected = rtn->item_position;
	CAL_merge(GV_active_field->cal, AFM->add.preset_chain_cal[AFM->add.preset_selected-1], True);
	set_cal_text_label(addLchainCalDisplay, GV_active_field->cal);
	CAL_merge(AFM->add.node_cal, AFM->add.preset_node_cal[AFM->add.preset_selected-1], True);
	get_node_type_from_cal(AFM->add.node_cal, &AFM->add.node_type_id, &AFM->add.node_type_val);
	set_cal_text_label(addLnodeCalDisplay, AFM->add.node_cal);
	XuMenuSelectItem(addNodeTypeOption, AFM->add.node_type_id);
}


/* Callback function for the link chain default node attributes.
 */
/*ARGSUSED*/
static void add_node_type_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	AFM->add.node_type_id = PTR2INT(client_data);
	switch(AFM->add.node_type_id)
	{
		case normalClassId:   AFM->add.node_type_val = FpaNodeClass_Normal;   break;
		case controlClassId:  AFM->add.node_type_val = FpaNodeClass_Control;  break;
		case floatingClassId: AFM->add.node_type_val = FpaNodeClass_Floating; break;
	}
	send_edit_cmd(ACCEPT_MODE);
}


/* Callback to set the node time delta
 */
/*ARGSUSED*/
static void time_delta_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmpSpinBoxCallbackStruct *rtn = (XmpSpinBoxCallbackStruct *)call_data;
	if(rtn->reason == XmCR_OK)
	{
		AFM->node.time_delta = XmpSpinBoxGetValue(w);
		send_edit_cmd(ACCEPT_MODE);
	}
}


/* The value of the end time of a chain can not be less that the start time,
 * so this callback ensures that this is the case.
 */
/*ARGSUSED*/
static void mod_time_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmpSpinBoxCallbackStruct *rtn = (XmpSpinBoxCallbackStruct *)call_data;
	if(rtn->reason == XmCR_OK)
	{
		int st = XmpSpinBoxGetValue(modStartTime);
		int et = XmpSpinBoxGetValue(modEndTime);
		if(et < st) XmpSpinBoxSetValue(modEndTime, st, False);
	}
	
}


/*ARGSUSED*/
static void modify_time_settings_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	(void) IngredVaCommand(GE_DEPICTION, "EDIT MODIFY SET_TIMES %s %d %d",
				TargetTimeGetStrTime(modRefTime),
				XmpSpinBoxGetValue(modStartTime),
				XmpSpinBoxGetValue(modEndTime));
}


/*ARGSUSED*/
static void node_edit_command_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	if(!XmToggleButtonGetState(w)) return;
	AFM->node.edit_btn  = w;
	AFM->node.edit_mode = (String) client_data;
	send_edit_cmd(ACCEPT_MODE);
}


/*ARGSUSED*/
static void node_edit_popup_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	XuToggleButtonSet((Widget) client_data, True, True);
}


/*ARGSUSED*/
static void modify_node_type_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	AFM->node.type_id = PTR2INT(client_data);
	switch(AFM->node.type_id)
	{
		case normalClassId:   AFM->node.type_val = FpaNodeClass_Normal;   break;
		case controlClassId:  AFM->node.type_val = FpaNodeClass_Control;  break;
		case floatingClassId: AFM->node.type_val = FpaNodeClass_Floating; break;
		case interpClassId:   AFM->node.type_val = FpaNodeClass_Interp;   break;
	}
	send_edit_cmd(ACCEPT_MODE);
}


/* Callback for the modify node context menu popup menu buttons.
 */
/*ARGSUSED*/
static void modify_node_popup_btn_cb(Widget w, XtPointer client_data, XtPointer unused)
{
	XuMenuSelectItem(modifyNodeTypeOption, PTR2INT(client_data));
}


/* Callback to apply the preset cal values to a selected chain. */
/*ARGSUSED*/
static void preset_mod_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int ndx = PTR2INT(client_data);
	CAL_merge(AFM->modify.cal, AFM->add.preset_chain_cal[ndx], True);
	set_cal_text_label(modLchainCalDisplay, AFM->modify.cal);
	send_edit_cmd(SET_MODE);
}


/* The context menu equivalent of the above.
 */
static void context_memory_cb(int ndx)
{
	preset_mod_cb(NULL, INT2PTR(ndx), NULL);
}


/* Callback to apply the preset node cal values to a selected node. */
/*ARGSUSED*/
static void preset_node_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int ndx = PTR2INT(client_data);
	CAL_merge(AFM->node.cal, AFM->node.preset_cal[ndx], True);
	get_node_type_from_cal(AFM->node.cal, &AFM->node.type_id, &AFM->node.type_val);
	set_cal_text_label(nodeCalDisplay, AFM->node.cal);
	XuMenuSelectItem(modifyNodeTypeOption, AFM->node.type_id);
	send_edit_cmd(SET_MODE);
}



/*  This is the function called by the attributes entry dialog when 
 *  link chain data is to be sent to Ingred.
*/
static void set_cal_value(CAL cal)
{
    if (!cal) return;
	if(InEditMode(E_MODIFY))
	{
		CAL_merge(AFM->modify.cal, cal, True);
		set_cal_text_label(modLchainCalDisplay, AFM->modify.cal);
	}
	else
	{
		CAL_merge(GV_active_field->cal, cal, True);
		set_cal_text_label(addLchainCalDisplay, GV_active_field->cal);
	}
	send_edit_cmd(SET_MODE);
}


/* Only the link chain cal can be changed when in the normal edit field modify mode
 */
static void set_add_node_cal_value(CAL cal)
{
    if (!cal) return;
	CAL_merge(AFM->add.node_cal, cal, True);
	set_cal_text_label(addLnodeCalDisplay,  AFM->add.node_cal);
	send_edit_cmd(SET_MODE);
}


/* Same thing for the nodes when in the edit node modify state.
 */
static void set_node_cal_value(CAL cal)
{
    if (!cal) return;
	CAL_merge(AFM->node.cal, cal, True);
	set_cal_text_label(nodeCalDisplay,  AFM->node.cal);
	send_edit_cmd(SET_MODE);
}


/* Right mouse button context menu functions. See the
 * ActivateSelectContextMenu function in panel_fieldEdit.c
 */
static void action_command_cb (CONTEXT *info, CAL cal)
{
	if(InEditMode(E_NODES))
	{
		if(same(info->selected_item->edit_cmd,E_SET))
		{
			ACTIVATE_linkNodeAttributesDialog (lchainSubpanel, (cal)? cal:AFM->node.cal, set_node_cal_value);
		}
		else
		{
			IngredVaEditCommand(cal, NullCal, "%s %s %s", E_EDIT, E_NODES, info->selected_item->edit_cmd);
		}
	}
	else if(same(info->selected_item->edit_cmd,E_SET))
	{
		ACTIVATE_linkChainAttributesDialog(lchainSubpanel, (cal)?cal:GV_active_field->cal, set_cal_value);
	}
	else if(same(info->selected_item->edit_cmd,E_NEW_CHAIN))
	{
		IngredVaEditCommand(GV_active_field->cal, AFM->add.node_cal, "%s %s %s %s %d %s",
			E_EDIT, E_ADD, E_NEW_CHAIN, AFM->add.ref_time, AFM->node.time_delta, AFM->add.node_type_val);
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


/* Handles status calls from Ingred that enable the context menus to be changed in
 * response to selected and deselected objects.
 */
static void ingred_observer( CAL cal, String *parms, int nparms )
{
	if(!GV_edit_mode) return;
	if(!same_ic(parms[0],E_EDIT)) return;

	if(same_ic(parms[1], E_BUTTON))
	{
		/* If a field does not exist at the current depiction time ingred sends
		* "EDIT BUTTON CREATE ON" and if it does "EDIT BUTTON CREATE OFF".
		* This is used to set the node edit controls insensitive or not.
		*/
		if(same_ic(parms[2], E_CREATE))
		{
			XtSetSensitive(nodeEdit, !same_ic(parms[3],E_ON));
		}
		else if(same_ic(parms[2], E_PASTE) && InEditMode(E_NODES))
		{
			XtSetSensitive(pasteBtn, same_ic(parms[3],E_ON));
		}
	}
	else if(same_ic(parms[1], E_ADDING))
	{
		/* If the NEW parameter is sent it means that the user has clicked on an
		 * existing node, so Ingred wants to know if the user wants to start a new
		 * chain or extend the existing one. Thus we must put up a context menu
		 * item that will say "New Chain" if that is what is wanted.
		 */
		if(same_ic(parms[2],E_NEW))
			ActivateSelectContextMenu(&add_new_context, action_command_cb, NULL);
	}
	else if(same_ic(parms[1],E_LCHAIN))
	{
		if(same_ic(parms[2], E_SELECT))
		{
			/* Respond to an "EDIT LCHAIN SELECT NODE [SET]" status
			 */
			if(same_ic(parms[3], "NODE"))
			{
				Boolean set = same_ic(parms[4],E_SET);

				/* Only the node modify edit mode has specific context menus for
				 * the selected item.
				 */
				(void) CAL_destroy(AFM->node.cal);
				AFM->node.cal = CAL_duplicate(cal);
				get_node_type_from_cal(AFM->node.cal, &AFM->node.type_id, &AFM->node.type_val);
				set_cal_text_label(nodeCalDisplay, AFM->node.cal);
				XuMenuSelectItem(modifyNodeTypeOption, AFM->node.type_id);

				XtSetSensitive(nodeModifySetBtn, (set && ALLOW_NODE_EDIT));
				XtSetSensitive(nodePresetLabel,  set);
				XtSetSensitive(nodePresetList,   set);

				if(same(AFM->node.edit_mode,E_MODIFY))
				{
					if(set && ALLOW_NODE_EDIT)
					{
						ActivateSelectContextMenu(&node_edit_context, action_command_cb, cal);
						ConfigureSelectContextMenuCascadeButtons(nodePresetLabel, nodePresetPopup, modifyNodeTypeLabel, modifyNodeTypePopup);
					}
					else
					{
						ActivateSelectContextMenu(NULL, action_command_cb, cal);
						ConfigureSelectContextMenuCascadeButtons(NULL, NULL, modifyNodeTypeLabel, modifyNodeTypePopup);
					}
					XtSetSensitive(nodeModControls, True);
				}
				else
				{
					DeactivateSelectContextMenu();
					XtSetSensitive(nodeModControls, False);
				}
			}
			else if(InEditMode(E_ADD))
			{
				XtSetSensitive(addSetBtn, ALLOW_ATTRIB_EDIT);
				XtSetSensitive(nodeAddSetBtn, ALLOW_NODE_EDIT);
				ShowContextMenuAttributes(True);
				ActivateSelectContextMenu(&add_context, action_command_cb, NULL);
			}
			else if(InEditMode(E_MOVE))
			{
				ActivateSelectContextMenu(&move_context, action_command_cb, NULL);
			}
			else if(InEditMode(E_MERGE))
			{
				ActivateSelectContextMenu(&merge_context, action_command_cb, NULL);
			}
			else if(InEditMode(E_MODIFY))
			{
				if(cal)
				{
					if(same_ic(parms[3],E_SET))
					{
						/* In this case the attribute is being sent by
						 * Ingred so we duplicate the entire thing.
						 */
						(void) CAL_destroy(AFM->modify.cal);
						AFM->modify.cal = CAL_duplicate(cal);
					}
					XtSetSensitive(modifySetBtn, ALLOW_ATTRIB_EDIT);
					if(ALLOW_ATTRIB_EDIT)
						ActivateSelectContextMenu(&modify_context, action_command_cb, cal);
					else
						ActivateSelectContextMenu(&delete_context, action_command_cb, cal);
					set_cal_text_label(modLchainCalDisplay, cal);
					(void) safe_strcpy(AFM->add.ref_time, CAL_get_attribute(cal, AttribLchainReference));
					TargetTimeSetStrTime(modRefTime, AFM->add.ref_time, False);
					XtVaSetValues(modStartTime, XmNvalue, atoi(CAL_get_attribute(cal, AttribLchainStartTime)), NULL);
					XtVaSetValues(modEndTime, XmNvalue, atoi(CAL_get_attribute(cal, AttribLchainEndTime)), NULL);
					XtSetSensitive(modifyControls, True);
					ShowContextMenuAttributes(True);
				}
				else
				{
					XtSetSensitive(modifySetBtn, False);
					ActivateSelectContextMenu(&delete_context, action_command_cb, cal);
					ShowContextMenuAttributes(False);
				}
			}
			else
			{
				XtSetSensitive(modifySetBtn, False);
				ShowContextMenuAttributes(False);
				DeactivateSelectContextMenu();
			}
		}
		else
		{
			/* Here the command was probably a DESELECT and we need to return the context
			 * menu to its no object selected state.
			 */
			XtSetSensitive(modifySetBtn, ALLOW_ATTRIB_EDIT);
			XtSetSensitive(nodeModifySetBtn, ALLOW_NODE_EDIT);
			XtSetSensitive(nodeModControls, False);
			XtSetSensitive(modifyControls, False);
			DestroyAttributesEntryDialog();
			DeactivateSelectContextMenu();
			ShowContextMenuAttributes(InEditMode(E_ADD));
			/*
			 * Particular to this field type, once an add has been completed by issuing an
			 * "end chain" command the add command must be reissued with all the time
			 * parameters so Ingred gets the time information again.
			 */
			if(InEditMode(E_ADD))
				send_edit_cmd(ACCEPT_MODE);
		}

		/* Just in case there is an entry menu active.
		 */
		if(cal)
		{
			if(InEditMode(E_NODES))
			{
				if(ALLOW_NODE_EDIT)
					UpdateAttributesEntryDialog(AFM->node.cal);
			}
			else
			{
				if(ALLOW_ATTRIB_EDIT)
					UpdateAttributesEntryDialog(GV_active_field->cal);
			}
		}
	}
}


/* Called when a link chain field is activated from panel_fieldEdit.c. The various
 * controls on the subpanel are initialized to those of the active field. We do not
 * want any edit commands to be sent while doing the initialization, and since the
 * SpinBox is especially bad at sending stuff, the "initializing" interlock variable
 * is used.
 */
static void entry(void)
{
	int n;

	initializing = True;

	set_cal_text_label(addLchainCalDisplay, GV_active_field->cal);
	set_cal_text_label(addLnodeCalDisplay,  AFM->add.node_cal);

	(void) safe_strcpy(AFM->add.ref_time, ActiveDepictionTime(FIELD_INDEPENDENT));
	TargetTimeSetStrTime(addRefTime, AFM->add.ref_time, True);

	AFM->add.node_type_id  = normalClassId;
	AFM->add.node_type_val = FpaNodeClass_Normal;
	XuMenuSelectItem(addNodeTypeOption, AFM->add.node_type_id);

	XtVaSetValues(addNodeTimeDelta, XmNvalue, AFM->node.time_delta, NULL);
	XtVaSetValues(modStartTime, XmNvalue, 0, NULL);
	XtVaSetValues(modEndTime, XmNvalue, 0, NULL);

	Manage(addControls,    InEditMode(E_ADD));
	Manage(modifyControls, InEditMode(E_MODIFY));
	Manage(nodeControls,   InEditMode(E_NODES));
	Manage(lchainSubpanel, InEditMode(E_ADD) || InEditMode(E_MODIFY) || InEditMode(E_NODES));

	XuMenuSelectItem(modifyNodeTypeOption, AFM->node.type_id);
	XuToggleButtonSet(AFM->node.edit_btn, True, False);
	set_cal_text_label(nodeCalDisplay, AFM->node.cal);

	XtSetSensitive(addSetBtn, ALLOW_ATTRIB_EDIT);
	XtSetSensitive(modifySetBtn, ALLOW_ATTRIB_EDIT);
	XtSetSensitive(nodeAddSetBtn, ALLOW_NODE_EDIT);

	/* ComboBox in add editor and pushbuttons in the modify editor that allow for
	 * the selection of predefined link chain attributes.
	 */
	XuComboBoxDeleteAllItems(addPreset);
	XtUnmanageChildren(modPresetBtns, nmodPresetBtns);
	InitContextMenuAttributes(GV_active_field->info->sh_label, context_memory_cb);
	if(AFM->add.preset_ncal > 0)
	{
		if(AFM->add.preset_ncal > nmodPresetBtns)
		{
			modPresetBtns = MoreWidgetArray(modPresetBtns, AFM->add.preset_ncal);
			for(n = nmodPresetBtns; n < AFM->add.preset_ncal; n++)
			{
				modPresetBtns[n] = XmVaCreateManagedPushButton(modPreset, "npb", NULL);
				XtAddCallback(modPresetBtns[n], XmNactivateCallback, preset_mod_cb, INT2PTR(n));
			}
			nmodPresetBtns = AFM->add.preset_ncal;
		}
		for(n = 0; n < AFM->add.preset_ncal; n++)
		{
			String label = CAL_get_attribute(AFM->add.preset_chain_cal[n], AttribUserlabel);
			XuComboBoxAddItem(addPreset, label, 0);
			XuWidgetLabel(modPresetBtns[n], label);
			AddToContextMenuAttributes(label);
		}

		if(AFM->add.preset_selected > 0)
			XuComboBoxSelectPos(addPreset, AFM->add.preset_selected, True);
		else if(AFM->add.preset_default_cal)
			XuComboBoxSelectItem(addPreset, CAL_get_attribute(AFM->add.preset_default_cal,AttribUserlabel), True);
	}
	XtManageChildren(modPresetBtns, AFM->add.preset_ncal);

	/* The buttons that allow selection of preset node attributes. There are the buttons on
	 * the subpanel here as well as those in the context menu popup.
	 */
	XtUnmanageChildren(nodePresetPanelBtns, nnodePresetBtns);
	XtUnmanageChildren(nodePresetPopupBtns, nnodePresetBtns);
	if(AFM->node.preset_ncal > nnodePresetBtns)
	{
		nodePresetPanelBtns = MoreWidgetArray(nodePresetPanelBtns, AFM->node.preset_ncal);
		nodePresetPopupBtns = MoreWidgetArray(nodePresetPopupBtns, AFM->node.preset_ncal);
		for(n = nnodePresetBtns; n < AFM->node.preset_ncal; n++)
		{
			nodePresetPanelBtns[n] = XmVaCreateManagedPushButton(nodePresets, "npb", NULL);
			XtAddCallback(nodePresetPanelBtns[n], XmNactivateCallback, preset_node_cb, INT2PTR(n));
			nodePresetPopupBtns[n] = XmVaCreateManagedPushButton(nodePresetPopup, "npb", NULL);
			XtAddCallback(nodePresetPopupBtns[n], XmNactivateCallback, preset_node_cb, INT2PTR(n));
		}
		nnodePresetBtns = AFM->node.preset_ncal;
	}
	for(n = 0; n < AFM->node.preset_ncal; n++)
	{
		String label = CAL_get_attribute(AFM->node.preset_cal[n], AttribUserlabel);
		XuWidgetLabel(nodePresetPanelBtns[n], label);
		XuWidgetLabel(nodePresetPopupBtns[n], label);
	}
	XtManageChildren(nodePresetPanelBtns, AFM->node.preset_ncal);
	XtManageChildren(nodePresetPopupBtns, AFM->node.preset_ncal);

	initializing = False;
}


/* To be called by the edit command function whenever an edit command
*  button is pushed in the editor panel in panel_fieldEdit.c
*/
/*ARGSUSED*/
static void edit_cmd_action(String cmd )
{
	DeactivateSelectContextMenu();

	Manage(addControls,    InEditMode(E_ADD));
	Manage(modifyControls, InEditMode(E_MODIFY));
	Manage(nodeControls,   InEditMode(E_NODES));
	Manage(lchainSubpanel, InEditMode(E_ADD) || InEditMode(E_MODIFY) || InEditMode(E_NODES));

	ShowContextMenuAttributes(InEditMode(E_ADD));
}


/* Called when the panel is to be shown or not from within panel_fieldEdit.c
 */
static void show_panel(Boolean state )
{
	if(state && InEditMode(E_NODES))
	{
		ConfigureMainContextMenuAuxCascade(nodeEditLabel, nodeEditPopup, NullWidget, NullWidget);
	}
	else
	{
		ReleaseMainContextMenuAuxCascade();
	}

	if(state && (InEditMode(E_ADD) || InEditMode(E_MODIFY) || InEditMode(E_NODES)))
	{
		XtManageChild(lchainSubpanel);
	}
	else
	{
		XtUnmanageChild(lchainSubpanel);
		DestroyAttributesEntryDialog();
	}
}


/* Sends editor commands to the graphics editor. This function is called
 * from the functions in this file as well as from panel_fieldEdit.c.The
 * recognized cmd parameters are:
 *
 *	ACCEPT_MODE - Post edit.
 *	SET_MODE    - Inserts the SET option into the appropriate command.
 *
 *	Note that in ADD mode the CAL structure for the nodes needs to be
 *	sent as well as that for the chains. This is done through a special
 *	"EDIT ADD NODE_CAL SET" edit command and is repeated any time the
 *	link chain CAL is set.
 */
static void send_edit_cmd(int cmd)
{
	String set_str = (cmd == SET_MODE)? E_SET:"";

	if(initializing) return;
	if(!ValidEditField()) return;
	if(!IsActiveEditor(LCHAIN_FIELD_EDITOR)) return;

	if(InEditMode(E_SAMPLE))
	{
		SendEditSampleCommand(NULL);
	}
	else if(InEditMode(E_ADD))
	{
		if(blank(set_str))
		{
			IngredVaEditCommand(GV_active_field->cal, AFM->add.node_cal, "%s %s %s %d %s",
				E_EDIT, E_ADD, AFM->add.ref_time, AFM->node.time_delta, AFM->add.node_type_val);
		}
		else
		{
			IngredVaEditCommand(GV_active_field->cal, AFM->add.node_cal, "%s %s %s %d %s %s",
				E_EDIT, E_ADD, AFM->add.ref_time, AFM->node.time_delta, AFM->add.node_type_val, E_SET);
		}
	}
	else if(InEditMode(E_MODIFY))
	{
		IngredVaEditCommand(AFM->modify.cal, NullCal, "%s %s %s", E_EDIT, E_MODIFY, set_str);
	}
	else if(InEditMode(E_NODES))
	{
		/* If a node has not been selected the node type is not sent along with the modify
		 * edit command. Once selected the type is sent. This is determined by checking the
		 * sensitivity of the nodeModeControls as this panel switches sensitivity state
		 * depending on just this selection process.
		 */
		if(same(AFM->node.edit_mode,E_MODIFY) && XtIsSensitive(nodeModControls))
		{
			IngredVaEditCommand(AFM->node.cal, NullCal, "%s %s %s %s %s",
				E_EDIT, E_NODES, AFM->node.edit_mode, AFM->node.type_val, set_str);
		}
		/* Send the command without the type even if the mod controls are sensitive so that Ingred
		 * changes things only once (thus an undo at this point will not confuse things).
		 */
		IngredVaEditCommand(AFM->node.cal, NullCal, "%s %s %s", E_EDIT, E_NODES, AFM->node.edit_mode);
	}
	else
	{
		IngredVaEditCommand(GV_active_field->cal, NullCal, "%s %s", E_EDIT, GV_active_field->editor->active->cmd);
	}
}
