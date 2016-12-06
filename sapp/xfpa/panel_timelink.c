/*****************************************************************************/
/*
*  File:	 panel_timelink.c
*  
*  Purpose:  Provides the controlling logic for the time link operation.
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
/*****************************************************************************/

#include <string.h>
#include "global.h"
#include <Xm/ComboBox.h>
#include <Xm/Label.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/Separator.h>
#include <Xm/ToggleB.h>
#include <ingred.h>
#include "contextMenu.h"
#include "editor.h"
#include "productStatus.h"
#include "resourceDefines.h"
#include "guidance.h"
#include "observer.h"
#define  TL_MAIN
#include "timelink.h"

/* As the option menu button id's must be positive we just set the following
 * to some big numbers as the rest of the buttons are added starting at zero.
 */
#define SELF_ID		100001
#define MASTER_ID	100002

/* Commonly used value in SetActiveTimelinkDepiction */
#define NONE "none"

/* For the context menu buttons
 */
enum { CM_NONE, CM_NEW, CM_END, CM_CURRENT };

static Widget  acceptBtn = NULL;
static Widget  cancelBtn = NULL;
static Widget  clearBtn = NULL;
static Widget  undoBtn = NULL;
static Widget  controlButtons = NULL;		/* manager for the link command control buttons */
static Widget  selectIntermediateLink = NULL;
static Widget  mergeSubpanel = NULL;		/* Manager for the merge controls */
static Widget  mergeField = NULL;			/* Fields to merge in */
static Widget  mergeSource = NULL;			/* Sources to merge control */
static Widget  mergeFetchBtn = NULL;		/* Fetch links from selected field from source */
static Widget  mergeSelectAllBtn = NULL;	/* Select all fetched links */
static Widget  groupWindow;
static int     ngrp_last = 0;
static int     ngrpBtns = 0;
static Widget  grpBtnsMan;			/* rowColumn manager for the group toggles */
static Widget  *grpBtns = NULL;
static Widget  grpIndsMan;			/* rowColumn manager for the group indicators */
static Widget  *grpInds = NULL;
static Widget  fieldWindow;
static int     nfldBtns = 0;
static Widget  fldBtnsMan;			/* rowColumn manager for the field toggles */
static Widget  *fldBtns = NULL;
static Widget  fldIndsMan;			/* rowColumn manager for the field indicators */
static Widget  *fldInds = NULL;
static Widget  linkToLabel;			/* label for link buttons */
static Widget  lk2BtnsMan;			/* link to button manager */
static Widget  *lk2Btns = NULL;

/* right mouse button context menu buttons */
static Widget  panelContextMenu;
static Widget  cm_end;
static Widget  cm_current;
static Widget  cm_new;
static Widget  cm_selectAll;
static Widget  cm_accept;
static Widget  cm_cancel;
static Widget  cm_undo;
static Widget  cm_clear;
static Widget  cm_separator;

static Pixel   noLinkColorBg,      noLinkColorFg;		/* colour to use for no link */
static Pixel   partialColorBg,     partialColorFg;		/* colour to use for partial links */
static Pixel   almostColorBg,      almostColorFg;		/* colour to use for linked but not interpolated */
static Pixel   fieldInterpColorBg, fieldInterpColorFg;
static Pixel   allSetColorBg,      allSetColorFg;		/* colour for linked and interpolated */
static Pixel   notLinkableColorBg, notLinkableColorFg;	/* colour for not linkable field */

static Pixmap  notLinkArrow = (Pixmap)NULL;
static Pixmap  noLinkArrow  = (Pixmap)NULL;
static Pixmap  partialArrow = (Pixmap)NULL;
static Pixmap  almostArrow  = (Pixmap)NULL;
static Pixmap  allSetArrow  = (Pixmap)NULL;

static Dimension    btn_height = 0;
static Dimension    ind_height = 0;
static Dimension    ind_width  = 0;
static Boolean      panel_active = False;
static Boolean      send_edit_cmds = True;		/* Sending of edit commands allowed? */
static int          top_context_btn = CM_NONE;
static FLD_DESCRIPT *merge_fd = NULL;			/* Field descriptor of the fields available for link merge */
static int          merge_fd_len = 0;			/* allocated size of the merge_fd array */

/* Define the link commands which will be issued from the link command state
*  buttons and the define the pixmaps which will be used in those buttons.
*  The pixmap files must be named timelink.<link_btn[n].name>.<lab|sel|ins>.xpm
*  Note that the actual pixmap file must have a name that is all lower case.
*/
static struct {
	String  name;	/* Name for pixmap file */
	String  cmd;	/* Edit command to issue */
	Widget  pw;		/* Panel button widget */
	Widget  cw;		/* Context menu button widget */
} link_btn[] = {
	{"linkForward", "EDIT FORWARD",      NULL, NULL},
	{"linkBackward","EDIT BACKWARD",     NULL, NULL},
	{"moveNode",    "EDIT MOVE",         NULL, NULL},
	{"merge",       "EDIT MERGE",        NULL, NULL},
	{"deleteNode",  "EDIT DELINK NODE",  NULL, NULL},
	{"deleteChain", "EDIT DELINK CHAIN", NULL, NULL}
};
static int control_btn_no      = 0;		/* Default link_btn active */
static int last_control_btn_no = 0;		/* Must be set equal to default link_btn */

/* The button labels and commands for the context menu for the timelink merge operation
 * operation once the links to merge have been fetched and selected.
 */
static CONTEXT merge_context = {
	E_MERGE, 0, 3,
	{
		{mergeBtnId,     E_MERGE    },
		{translateBtnId, E_TRANSLATE},
		{rotateBtnId,    E_ROTATE   }
	}
};

/* Defines to improve the clarity of the code. These must be modified if
 * the link_btn structure changes.
 */
#define LINKING_FORWARD		(control_btn_no==0)
#define LINKING_BACKWARD	(control_btn_no==1)
#define MOVING				(control_btn_no==2)
#define MERGING				(control_btn_no==3)
#define IN_DELETE_MODE		(control_btn_no==4||control_btn_no==5)
#define LINKING_DIRECTION   (LINKING_FORWARD||LINKING_BACKWARD)

/* Define to make the code clearer. If the toggle that sets the link type to
 * intermediate is on and it is to be switched off, then a command will need
 * to be sent to Ingred to turn the state back to normal.
 */
#define SetLinkTypeToNormal() \
	if(XmToggleButtonGetState(selectIntermediateLink))XuToggleButtonSet(selectIntermediateLink,False,True)

static void edit_commands_cb (Widget, XtPointer, XtPointer);
static void context_menu_cb	(Widget, XtPointer, XtPointer);
static void create_context_menu (void);
static void check_button_array_size	(void);
static void action_state_cb	(Widget, XtPointer, XtPointer);
static void field_cb (Widget, XtPointer, XtPointer);
static void group_cb (Widget, XtPointer, XtPointer);
static void link_to_cb (Widget, XtPointer, XtPointer);
static void redisplay_group_timelink_state (GROUP*);
static void relabel_group_list (void);
static void relabel_field_list (void);
static void select_intermediate_link_cb	(Widget, XtPointer, XtPointer);
static void set_context_menu_visibility (void);
static void set_linked_to_option_list (Boolean);
static void timelink_clear_cb (Widget, XtPointer, XtPointer);
static void ingred_status_observer (CAL, String*, int);
static void button_state_observer (CAL, String*, int);
static void merge_cb (Widget, XtPointer, XtPointer);


/**********************************************************************/
/*
*   Create the timelink control panel widgets.
*/
/**********************************************************************/
void CreateTimelinkPanel(Widget parent)
{
	int i, n;
	Dimension height, spacing;
	XmString blank_label;
	Widget btn, label, rc, controlBtnRC;
	Widget groupLabel, groupForm;
	Widget fieldLabel, fieldForm;
	Widget linkControls;
	Pixmap lpx, spx, ipx;

	static XuMenuItemStruct link_option_menu[] = {
		{"self",       &xmPushButtonWidgetClass, 0, None, SELF_ID, link_to_cb, (XtPointer)LINKED_TO_SELF, NULL },
		{"masterLink", &xmPushButtonWidgetClass, 0, None, MASTER_ID, link_to_cb, (XtPointer)LINKED_TO_MASTER, NULL },
		{"linkSep",    &xmSeparatorWidgetClass,  0, None, NoId, NULL, (XtPointer)NULL, NULL },
		NULL
	};

	/* Create the panel itself.
	*/
	blank_label = XuNewXmString("");

	GW_timelinkPanel = parent;

	groupLabel = XmVaCreateManagedLabel(GW_timelinkPanel, "group",
		XmNresizable, False,
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 20,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 9,
		NULL);

	groupWindow = XmVaCreateManagedScrolledWindow(GW_timelinkPanel, "groupWindow",
		XmNresizable, False,
		XmNscrollingPolicy, XmAUTOMATIC,
		XmNspacing, 2,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, groupLabel,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 4,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 4,
		NULL);

	groupForm = XmVaCreateManagedForm(groupWindow, "groupForm", NULL);

	grpIndsMan = XmVaCreateManagedRowColumn(groupForm, "indsMan",
		XmNpacking, XmPACK_COLUMN,
		XmNadjustLast, False,
		XmNadjustMargin, False,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	grpBtnsMan = XmVaCreateManagedRowColumn(groupForm, "grpBtnsMan",
		XmNradioBehavior, True,
		XmNmarginWidth, 0,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, grpIndsMan,
		NULL);

	/* Set the spacing of the row column managers used by the indicator
	*  widgets depending on the size of the selector buttons.
	*/
	btn = XmVaCreateManagedToggleButton(grpBtnsMan, "tmp", NULL);
	XtVaGetValues(btn, XmNheight, &btn_height, NULL);
	ind_height = btn_height - INDICATOR_DIFF + INDICATOR_MARGIN*2;
	ind_width = (btn_height - INDICATOR_DIFF)/2 + INDICATOR_MARGIN*2;
	height = btn_height - ind_height;
	XtVaSetValues(grpIndsMan, XmNspacing, height, XmNmarginHeight, height-2, NULL);
	XtDestroyWidget(btn);
	
	/* Create the subpanel that appears when the merge button is selected. It is not managed
	 * on creation as this is controlled in the field selection callback.
	 */
	mergeSubpanel = XmVaCreateForm(GW_timelinkPanel, "mergeSubpanel",
		XmNmappedWhenManaged, False,
		XmNborderWidth, 1,
		XmNverticalSpacing, 4,
		XmNhorizontalSpacing, 4,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 6,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 6,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNbottomOffset, 6,
		NULL);

	label = XmVaCreateManagedLabel(mergeSubpanel, "fieldHeader",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	mergeField = XmVaCreateManagedComboBox(mergeSubpanel, "mergeField",
		XmNvalue, "",
		XmNvisibleItemCount, 5,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(mergeField, XmNselectionCallback, merge_cb, NULL);

	label = XmVaCreateManagedLabel(mergeSubpanel, "sourceHeader",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, mergeField,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	mergeSource = XmVaCreateManagedComboBox(mergeSubpanel, "mergeSource",
		XmNvalue, "",
		XmNvisibleItemCount, 5,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(mergeSource, XmNselectionCallback, merge_cb, NULL);

	mergeFetchBtn = XmVaCreateManagedPushButton(mergeSubpanel, "fetchBtn",
		XmNsensitive, False,
		XmNmarginWidth, 7,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, mergeSource,
		XmNtopOffset, 9,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(mergeFetchBtn, XmNactivateCallback, merge_cb, NULL);

	mergeSelectAllBtn = XmVaCreateManagedPushButton(mergeSubpanel, "selectAll",
		XmNsensitive, False,
		XmNmarginWidth, 7,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, mergeSource, 
		XmNtopOffset, 9,
		XmNrightAttachment, XmATTACH_FORM, 
		NULL);
	XtAddCallback(mergeSelectAllBtn, XmNactivateCallback, merge_cb, (XtPointer)E_SELECT_ALL);

	XtManageChild(mergeSubpanel);

	/* The intermediate link toggle is only valid with the move command
	 * and thus is only managed if the move button is selected.
	 */
	selectIntermediateLink = XmVaCreateManagedToggleButton(GW_timelinkPanel, "selectInterLink",
		XmNmappedWhenManaged, False,
		XmNset, XmUNSET,
		XmNsensitive, False,
		XmNborderWidth, 1,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, mergeSubpanel,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 6,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 6,
		NULL);
	XtAddCallback(selectIntermediateLink, XmNvalueChangedCallback, select_intermediate_link_cb, NULL);

	controlButtons = XmVaCreateManagedFrame(GW_timelinkPanel, "controlFrame",
		XmNshadowType, XmSHADOW_ETCHED_IN,
		XmNmarginHeight, 4,
		XmNmarginWidth, 4,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 6,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 6,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, mergeSubpanel,
		XmNbottomOffset, 5,
		NULL);

	(void) XmVaCreateManagedLabel(controlButtons, "controlLabel",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	linkControls = XmVaCreateManagedForm(controlButtons, "linkControls", NULL);

	controlBtnRC = XmVaCreateManagedRowColumn(linkControls, "controlButtons",
		XmNmarginWidth, 0,
		XmNmarginHeight, 0,
		XmNradioBehavior, True,
		XmNorientation, XmHORIZONTAL,
		XmNpacking, XmPACK_COLUMN,
		XmNnumColumns, 3,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	for(i = 0; i < (int) XtNumber(link_btn); i++)
	{
		ToggleButtonPixmaps(GW_timelinkPanel, "timelink", link_btn[i].name, &lpx, &spx, &ipx);

		link_btn[i].pw = XmVaCreateManagedToggleButton(controlBtnRC, link_btn[i].name,
			XmNshadowThickness, 2,
			XmNindicatorOn, False,
			XmNmarginHeight, 0,
			XmNmarginWidth, 0,
			XmNlabelType, XmPIXMAP,
			XmNlabelPixmap, lpx,
			XmNselectPixmap, spx,
			XmNselectInsensitivePixmap, ipx,
			NULL);
		XtAddCallback(link_btn[i].pw, XmNvalueChangedCallback, action_state_cb, INT2PTR(i));
	}

	rc = XmVaCreateManagedRowColumn(linkControls, "cmdBtns",
		XmNmarginWidth, 0,
		XmNmarginHeight, 0,
		XmNentryAlignment, XmALIGNMENT_CENTER,
		XmNpacking, XmPACK_COLUMN,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, controlBtnRC,
		XmNleftOffset, 6,
		NULL);

	acceptBtn = XmVaCreateManagedPushButton(rc, "acceptBtn",
		XmNsensitive, False,
		NULL);
	XtAddCallback(acceptBtn, XmNactivateCallback, edit_commands_cb, E_UPDATE);

	cancelBtn = XmVaCreateManagedPushButton(rc, "cancelBtn",
		XmNsensitive, False,
		NULL);
	XtAddCallback(cancelBtn, XmNactivateCallback, edit_commands_cb, E_CANCEL);

	undoBtn = XmVaCreateManagedPushButton(rc, "undoBtn",
		XmNsensitive, False,
		NULL);
	XtAddCallback(undoBtn, XmNactivateCallback, edit_commands_cb, E_UNDO);

	clearBtn = XmVaCreateManagedPushButton(rc, "clearBtn",
		XmNsensitive, False,
		NULL);
	XtAddCallback(clearBtn, XmNactivateCallback, timelink_clear_cb, NULL);


	/* Create the option menu along with the first two button which are
	*  fixed and do not change with changing field lists.
	*/
	lk2BtnsMan = XuVaMenuBuildOption(GW_timelinkPanel, "lk2BtnsMan", link_option_menu,
		XmNresizable, False,
		XmNlabelString, blank_label,
        XmNmarginWidth, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 3,
        XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 3,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, controlButtons,
		XmNbottomOffset, 9,
		NULL);
	
	linkToLabel = XmVaCreateManagedLabel(GW_timelinkPanel, "linkToLabel",
		XmNresizable, False,
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 9,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, lk2BtnsMan,
		XmNbottomOffset, 0,
		NULL);

	btn = XmVaCreateManagedPushButton(GW_timelinkPanel, "legendBtn",
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 9,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 9,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, linkToLabel,
		XmNbottomOffset, 9,
		NULL);
	XtAddCallback(btn, XmNactivateCallback,  ShowFieldStatusLegendCB, NULL);

	fieldLabel = XmVaCreateManagedLabel(GW_timelinkPanel, "fieldLabel",
		XmNresizable, False,
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, groupWindow,
		XmNtopOffset, 10,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 9,
		NULL);

	fieldWindow = XmVaCreateManagedScrolledWindow(GW_timelinkPanel, "fieldWindow",
		XmNresizable, False,
		XmNscrollingPolicy, XmAUTOMATIC,
		XmNspacing, 2,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, fieldLabel,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 4,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 4,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, btn,
		XmNbottomOffset, 5,
		NULL);

	fieldForm = XmVaCreateManagedForm(fieldWindow, "fieldForm", NULL);

	fldIndsMan = XmVaCreateManagedRowColumn(fieldForm, "indsMan",
		XmNspacing, height,
		XmNmarginHeight, height-2,
		XmNadjustLast, False,
		XmNadjustMargin, False,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	fldBtnsMan = XmVaCreateManagedRowColumn(fieldForm, "fldBtnsMan",
		XmNmarginWidth, 0,
		XmNradioBehavior, True,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, fldIndsMan,
		NULL);

	/* Get the indicator colours from the resource database.
	*/
	notLinkableColorBg = XuLoadColorResource(parent, RNnotLinkableColorBg,"IndianRed");
	notLinkableColorFg = XuLoadColorResource(parent, RNnotLinkableColorFg,"black");
	noLinkColorBg      = XuLoadColorResource(parent, RNnoLinkColorBg,"CadetBlue1");
	noLinkColorFg      = XuLoadColorResource(parent, RNnoLinkColorFg,"black");
	partialColorBg     = XuLoadColorResource(parent, RNpartialLinkColorBg,"RoyalBlue");
	partialColorFg     = XuLoadColorResource(parent, RNpartialLinkColorFg,"black");
	almostColorBg      = XuLoadColorResource(parent, RNalmostColorBg,"GoldenRod");
	almostColorFg      = XuLoadColorResource(parent, RNalmostColorFg,"black");
	fieldInterpColorBg = XuLoadColorResource(parent, RNfieldInterpColorBg,"green");
	fieldInterpColorFg = XuLoadColorResource(parent, RNfieldInterpColorFg,"black");
	allSetColorBg      = XuLoadColorResource(parent, RNallSetColorBg,"ForestGreen");
	allSetColorFg      = XuLoadColorResource(parent, RNallSetColorFg,"black");

	XmStringFree(blank_label);

	/* Make the group window fit the size as found in the resource file or
	 * the actual number of groups, which ever is less. 
	*/
	XtVaGetValues(grpBtnsMan,
		XmNspacing, &spacing,
		XmNmarginHeight, &height,
		NULL);

	n = XuGetIntResource(RNtimelinkPanelMaxGroupBtns, 4);
	XtVaSetValues(groupWindow,
		XmNheight, (Dimension)((int)(btn_height+spacing)*n+(int)height*4),
		NULL);

	XtManageChild(GW_timelinkPanel);

	create_context_menu();

	AddIngredObserver(ingred_status_observer);
	AddIngredObserver(button_state_observer);
}



/**********************************************************************/
/* 
 * Initialize the timelink panel. Create the arrow pixmaps and set the
 * initial state of the timelink groups and of the active field in the
 * groups. Note that this function must be called after the create
 * function above and after all of the fields and groups have been
 * initialized.
 */
/**********************************************************************/
void InitTimelinkPanel()
{
	int  i, n, nlist;
	char **list;
	XuArrowAttributes attrib;
	/*
	 * Create all of the arrow pixmaps
	 */
	attrib.flags = 	XuARROW_APPEARANCE|XuARROW_WIDTH|XuARROW_HEIGHT|XuARROW_MARGIN_WIDTH|
					XuARROW_MARGIN_HEIGHT|XuARROW_FOREGROUND|XuARROW_BACKGROUND;
	attrib.appearance    = XuARROW_BARBED|XuARROW_STEM;
	attrib.width         = (int)ind_width;
	attrib.height        = (int)ind_height;
	attrib.margin_width  = INDICATOR_MARGIN;
	attrib.margin_height = INDICATOR_MARGIN;

	attrib.foreground = notLinkableColorFg;
	attrib.background = notLinkableColorBg;
	notLinkArrow  = XuCreateArrowPixmap(GW_timelinkPanel, &attrib);

	attrib.foreground = noLinkColorFg;
	attrib.background = noLinkColorBg;
	noLinkArrow  = XuCreateArrowPixmap(GW_timelinkPanel, &attrib);

	attrib.foreground = partialColorFg;
	attrib.background = partialColorBg;
	partialArrow = XuCreateArrowPixmap(GW_timelinkPanel, &attrib);

	attrib.foreground = almostColorFg;
	attrib.background = almostColorBg;
	almostArrow  = XuCreateArrowPixmap(GW_timelinkPanel, &attrib);

	attrib.foreground = allSetColorFg;
	attrib.background = allSetColorBg;
	allSetArrow  = XuCreateArrowPixmap(GW_timelinkPanel, &attrib);
	/*
	 * Initialize the master link group. This is used only within this timelinking
	 * panel. The GV_tlgrps array is created and extended in panel_fieldEdit.c
	 * (see InitFields and get_field_group functions in that file) and not by the
	 * functions in this file.
	 */
	if(GEStatus("GROUP MASTER_LINK_STATUS GLOBAL", &nlist, &list, NULL, NULL) == GE_VALID)
		GV_tlgrps[0]->fields_link_status = LinkStatusStringToIndex(list[0], True);
	/*
	 * Initialize the group master status. If there are any changes after this ingred
	 * will post a notification that will be caught by ingred_status_observer()
	 */
	for(i = 0; i < GV_ntlgrps; i++)
	{
		char mbuf[256];
		if(!GV_tlgrps[i]->tlfld) continue;
		(void) snprintf(mbuf, sizeof(mbuf), "GROUP MASTER_LINK_STATUS %s", GV_tlgrps[i]->name);
		if(GEStatus(mbuf, &nlist, &list, NULL, NULL) == GE_VALID)
			GV_tlgrps[i]->tlfld[0]->link_status = LinkStatusStringToIndex(list[0], True);
		SetGroupLinkStatus(GV_tlgrps[i]);
	}
	/*
	 * Set the default active field for the group for timelink selection. 
	 * Set it to the first field that exists and is not a group master.
	 * If none default to the groupmaster.
	 */
	for(i = 1; i < GV_ntlgrps; i++)
	{
		if(!GV_tlgrps[i]->tlfld) continue;
		GV_tlgrps[i]->atlfld = GV_tlgrps[i]->tlfld[0];
		GV_tlgrps[i]->atlfldno = 0;
		for(n = 1; n < GV_tlgrps[i]->ntlfld; n++)
		{
			if(!GV_tlgrps[i]->tlfld[n]->exists) continue;
			GV_tlgrps[i]->atlfld = GV_tlgrps[i]->tlfld[n];
			GV_tlgrps[i]->atlfldno = n;
			break;
		}
	}
}



/**********************************************************************/
/*
*   Set the timelink state variables for a field.
*/
/**********************************************************************/
void SetFieldTimelinkState(FIELD_INFO *eptr )
{
	int nfld;
	char mbuf[300];
	String *list1, *list2;


	snprintf(mbuf, sizeof(mbuf), "FIELD LINK_INFO %s %s",
		eptr->info->element->name, eptr->info->level->name);
	(void) GEStatus(mbuf, &nfld, &list1, &list2, NULL);

	if(same_ic(list2[0], "MASTER_LINK"))
	{
		eptr->link_state  = LINKED_TO_MASTER;
		eptr->link_status = LinkStatusStringToIndex(list1[0], False);
		eptr->link_fld    = NULL;
	}
	else if(same_ic(list2[0], "FIELD"))
	{
		eptr->link_state  = LINKED_TO_FIELD;
		eptr->link_status = LinkStatusStringToIndex(list1[0], False);
		eptr->link_fld    = FindField(list1[1], list2[1]);
	}
	else
	{
		eptr->link_state  = LINKED_TO_SELF;
		eptr->link_status = LinkStatusStringToIndex(list1[0], False);
		eptr->link_fld    = NULL;
	}
}


/**********************************************************************/
/*
*  Controls the startup of the timelink panel. If an error is
*  detected the return is False.
*
*  The full_initialize parameter specifies if the panel should be fully
*  initialized (True), as when changing panels or partially initialized
*  (False) as when being called from the ActivateMenu() function.
*/
/**********************************************************************/
Boolean TimelinkStartup(Boolean full_initialize)
{
	int i;
	char mbuf[300];

	/* We need at least two depictions to do any linking at all!
	*/
	if(GV_ndepict < 2)
	{
		XuShowError(GW_timelinkPanel, "TimelinkDepictNr", NULL);
		return False;
	}

	XtSetSensitive(grpBtnsMan, True);
	XtSetSensitive(fldBtnsMan, True);

	/* If the variable panel_active is true then this menu is active
	*  (we have not left by the exit function below) and all we need
	*  to do is issue the active field and timelink edit commands.
	*/
	if( panel_active )
	{
		if(GV_atlgrp == GV_tlgrps[0])
		{
			(void) IngredCommand(GE_TIMELINK, "MASTER_LINK GLOBAL");
		}
		else if(GV_atlgrp->atlfldno == 0)
		{
			strcpy(mbuf, "MASTER_LINK ");
			strcat(mbuf, GV_atlgrp->name);
			(void) IngredCommand(GE_TIMELINK, mbuf);
		}
		else
		{
			snprintf(mbuf, sizeof(mbuf), "FIELD %s %s",
				GV_atlgrp->atlfld->info->element->name,
				GV_atlgrp->atlfld->info->level->name);
			(void) IngredCommand(GE_DEPICTION, mbuf);
		}

		if(send_edit_cmds)
			(void) IngredCommand(GE_TIMELINK, link_btn[control_btn_no].cmd);

		SetActiveTimelinkDepiction(LINKING_DIRECTION? ACTIVE:NONE, LINKING_BACKWARD);
	}
	else
	{
		/* Check for fields with a state of linked to field. If the field
		*  was added before the wanted field was added to the interface
		*  the field pointer will be NULL and we update it now.
		*/
		for(i = 1; i < GV_nfield; i++)
		{
			if(GV_field[i]->link_state != LINKED_TO_FIELD) continue;
			if(NotNull(GV_field[i]->link_fld)) continue;
			SetFieldTimelinkState(GV_field[i]);

			/* If we still do not have a valid 'link to field' set the link state
			*  to 'linked to self'.
			*/
			if(IsNull(GV_field[i]->link_fld)) GV_field[i]->link_state = LINKED_TO_SELF;
		}

		if (full_initialize)
		{
			/* Set the field to be linked the same as the active field.
			*/
			GV_atlgrp = GV_active_field->group;
			GV_atlgrp->atlfld = GV_active_field;
			GV_atlgrp->atlfldno = 1;
			for(i = 0; i < GV_atlgrp->nfield; i++)
			{
				if(GV_atlgrp->atlfld != GV_atlgrp->tlfld[i]) continue;
				GV_atlgrp->atlfldno = i;
				break;
			}
		}

		/* If the active field cannot be timelinked then set the active field
		*  to the first group and field which are not master links and which
		*  can be linked.
		*/
		if(GV_atlgrp->ntlfld < 2)
		{
			for(i = 1; i < GV_ntlgrps; i++)
			{
				if(GV_tlgrps[i]->ntlfld < 2) continue;
				GV_atlgrp = GV_tlgrps[i];
				GV_atlgrp->atlfld = GV_atlgrp->tlfld[1];
				GV_atlgrp->atlfldno = 1;
				break;
			}
			if(i >= GV_ntlgrps)
			{
				XuShowError(GW_timelinkPanel, "TimelinkNoGrps", NULL);
				return False;
			}
		}

		/* We have a valid group, not check the field and select a valid one.
		*/
		if(!GV_atlgrp->atlfld->exists)
		{
			for(i = 1; i < GV_atlgrp->ntlfld; i++)
			{
				if(!GV_atlgrp->tlfld[i]->exists) continue;
				GV_atlgrp->atlfld = GV_atlgrp->tlfld[i];
				GV_atlgrp->atlfldno = i;
				break;
			}
		}

		panel_active = True;
		(void) IngredCommand(GE_TIMELINK, "ENTER");
		redisplay_group_timelink_state(GV_atlgrp);
	}

	SetActiveContextMenu(panelContextMenu);
	SetActiveTimelinkDepiction(LINKING_DIRECTION? ACTIVE:NONE, LINKING_BACKWARD);
	set_context_menu_visibility();

	return True;
}


void SetTimelinkCancel( Boolean state )
{
	if (!state) SetLinkTypeToNormal();
	XtSetSensitive(cancelBtn, state);
	set_context_menu_visibility();
}


/**********************************************************************/
/*
*   Handle exiting from the panel. Note that the indicators at least
*   must be unmanaged so that when we come back into this panel the
*   server does not try and redisplay the widgets with non-existant
*   pixmaps in them. The key parameter is not used at this time.
*/
/**********************************************************************/
/*ARGSUSED*/
void TimelinkExit(String key)
{
	SetActiveTimelinkDepiction(NONE,LINKING_BACKWARD);
	(void) IngredCommand(GE_TIMELINK, EXIT);
	SetLinkTypeToNormal();
	NotifyObservers(OB_TIMELINK_EXIT, NULL, 0);
	panel_active = False;
	XtSetSensitive(grpBtnsMan, False);
	XtSetSensitive(fldBtnsMan, False);
	SetActiveContextMenu(None);
}


/*  Return the status variable given the status as a string.
*/
int LinkStatusStringToIndex(String status_string , Boolean is_master )
{
	if(is_master)
	{
		if(same_ic(status_string,"PARTIAL"))      return (char)SOME_LINKS;
		else if(same_ic(status_string,"LINKED"))  return (char)INTERPOLATED;
		else if(same_ic(status_string,"FIELD"))   return (char)INTERPOLATED;
		else if(same_ic(status_string,"INTERP"))  return (char)INTERPOLATED;
		else                                      return (char)NOLINKS;
	}
	else
	{
		if(same_ic(status_string,"NONE"))         return (char)NOLINKS;
		else if(same_ic(status_string,"PARTIAL")) return (char)SOME_LINKS;
		else if(same_ic(status_string,"LINKED"))  return (char)LINKED;
		else if(same_ic(status_string,"FIELD"))   return (char)FIELD_INTERP;
		else if(same_ic(status_string,"INTERP"))  return (char)INTERPOLATED;
		else                                      return (char)NOT_LINKABLE;
	}
}


/* Return the indicator colour given the field link status.
*/
void LinkStatusStringToColour(String status_string , Pixel *fg, Pixel *bg)
{
	switch(LinkStatusStringToIndex(status_string, False))
	{
		case NOLINKS:      *bg = noLinkColorBg;       *fg = noLinkColorFg;       break;
		case SOME_LINKS:   *bg = partialColorBg;      *fg = partialColorFg;      break;
		case LINKED:       *bg = almostColorBg;       *fg = almostColorFg;       break;
		case FIELD_INTERP: *bg = fieldInterpColorBg;  *fg = fieldInterpColorFg;  break;
		case INTERPOLATED: *bg = allSetColorBg;       *fg = allSetColorFg;       break;
		case NOT_LINKABLE: *bg = notLinkableColorBg;  *fg = notLinkableColorFg;  break;
	}
}


/* Set the link status of the group which depends on the minimum link
*  status of all of the fields in the group.
*/
void SetGroupLinkStatus(GROUP *grp )
{
	int i;

	if(grp->nfield < 1) return;

	grp->fields_link_status = INTERPOLATED;
	for(i = 0; i < grp->nfield; i++)
	{
		if(!grp->field[i]->exists) continue;
		if(grp->field[i]->link_status == NOT_LINKABLE) continue;
		grp->fields_link_status =
			MIN(grp->fields_link_status,grp->field[i]->link_status);
	}
}


/*===================== LOCAL FUNCTIONS FOLLOW ============================*/


/* Action status information from ingred that results in the accept,
 * undo and cancel buttons being made active or inactive.
 */
static void button_state_observer(CAL cal, String *parms, int nparms)
{
	String  cmd;
	Boolean state;

	if(!same_ic(parms[0],E_EDIT))   return;
	if(!same_ic(parms[1],E_BUTTON)) return;

	cmd = parms[2];
	state = same_ic(parms[3],E_ON);
	
	if(same_ic(cmd,E_UPDATE))
	{
		XtSetSensitive(acceptBtn, state);
	}
	if(same_ic(cmd,E_CANCEL))
	{
		if(!MERGING && !state) SetLinkTypeToNormal();
		XtSetSensitive(cancelBtn, state);
		XtSetSensitive(mergeSelectAllBtn, state && MERGING);
	}
	else if(same_ic(cmd,E_UNDO))
	{
		XtSetSensitive(undoBtn, state);
	}

	set_context_menu_visibility();
}


/* Right mouse button context menu function callback that returns a pointer to the
 * CONTEXT structure passed in to the context menu configure function.
 */
static void context_menu_action (CONTEXT *info, CAL cal)
{
	(void) IngredVaCommand(GE_TIMELINK, "EDIT %s %s", info->edit_mode, info->selected_item->edit_cmd);
}



/* This function responds to any timelink status information passed
 * to the interface by Ingred.
 */
/*ARGSUSED*/
static void ingred_status_observer(CAL cal, String *parms, int nparms)
{
	int i, pc = 0;
	char status;
	String key, element, level, group, state;
	Pixel bg;
	FIELD_INFO *field;

	if(!GV_edit_mode) return;

	if(!same_ic(parms[pc++], "TIMELINK")) return;

	top_context_btn = CM_NONE;
	key = parms[pc++];

	/*
	 * Many of the items set by a status call from Ingred result in
	 * a command being sent to Ingred when they are set. The commands
	 * are thus blocked unless specifically allowed in the following.
	 */
	SendIngredCommands(False);

	if(same_ic(key,"LINKING"))
	{
		if(same_ic(parms[pc],E_NEW))
		{
			top_context_btn = CM_NEW;
		}
		else if(same_ic(parms[pc],"EXTRAP"))
		{
			top_context_btn = CM_CURRENT;
		}
		else if(valid_tstamp(parms[pc]))
		{
			SetActiveTimelinkDepiction(parms[pc], LINKING_BACKWARD);
			SendIngredCommands(True);   /* OK to send guidance commands */
			DisplayGuidanceAtTime(parms[pc++]);
			if(nparms < 5 || (!same_ic(parms[pc],"NO_FIELD") && !same_ic(parms[pc],"NO_DATA")))
			{
				/* The end chain can only appear once we have started linking. In
				 * this case a request to put up the cancel button will arrive
				 */
				if(XtIsSensitive(cancelBtn)) top_context_btn = CM_END;
			}
		}
		else
		{
			SetActiveTimelinkDepiction(NULL, LINKING_BACKWARD);
			SendIngredCommands(True);   /* OK to send guidance commands */
			DisplayGuidanceAtTime(NULL);
		}
	}
	else if(same_ic(key,"MASTER_LINK_STATUS_UPDATE"))
	{
		group  = parms[pc++];
		status = LinkStatusStringToIndex(parms[pc++], True);
		switch(status)
		{
			case SOME_LINKS:   bg = partialColorBg;     break;
			case FIELD_INTERP: bg = fieldInterpColorBg; break;
			case INTERPOLATED: bg = allSetColorBg;      break;
			case NOT_LINKABLE: bg = notLinkableColorBg; break;
			default:           bg = noLinkColorBg;      break;
		}
		if(NotNull(GV_tlgrps))
		{
			if(same_ic(group,"GLOBAL"))
			{
				if(NotNull(GV_tlgrps[0]))
				{
					GV_tlgrps[0]->fields_link_status = status;
					if(panel_active)
					{
						XtVaSetValues(grpInds[0], XmNbackground, bg, NULL);
						redisplay_group_timelink_state(GV_tlgrps[0]);
					}
				}
			}
			else
			{
				for(i = 1; i < GV_ntlgrps; i++)
				{
					if(IsNull(GV_tlgrps[i]) || !same_ic(group,GV_tlgrps[i]->name))
						continue;
					GV_tlgrps[i]->tlfld[0]->link_status = status;
					if(panel_active)
					{
						XtVaSetValues(fldInds[0],
							XmNlabelType, XmSTRING,
							XmNbackground, bg,
							NULL);
						redisplay_group_timelink_state(GV_tlgrps[i]);
					}
					break;
				}
			}
		}
	}
	else if(same_ic(key,"STATUS_UPDATE"))
	{
		element = parms[pc++];
		level   = parms[pc++];
		state   = parms[pc++];

		if((field = FindField(element, level)) == NULL) return;

		field->link_status = LinkStatusStringToIndex(state, False);
		if(panel_active)
		{
			set_linked_to_option_list(True);
			redisplay_group_timelink_state(field->group);
		}
	}
	else if(same_ic(key,"ACTION"))
	{
		key = parms[pc++];
		if(same_ic(key,"INTERMEDIATE_DONE"))
		{
			SendIngredCommands(True);   /* OK to reset link type */
			SetLinkTypeToNormal();
		}
		else if(same_ic(key,E_SELECT))
		{
			XtSetSensitive(mergeFetchBtn, False);
			ActivateSelectContextMenu(&merge_context, context_menu_action, NULL);
		}
		else if(same_ic(key,E_DESELECT))
		{
			XtSetSensitive(mergeFetchBtn, True);
			DeactivateSelectContextMenu();
		}
	}
	SendIngredCommands(True);
	set_context_menu_visibility();
}


/* Callback for the group selection toggles.
 */
/*ARGSUSED*/
static void group_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	Boolean mapFldBtnsMan = False;
	Boolean mapFldIndsMan = False;

	if(!XmToggleButtonGetState(w)) return;

	GV_atlgrp = GV_tlgrps[PTR2INT(client_data)];
	if(GV_atlgrp == GV_tlgrps[0])
	{
		send_edit_cmds = True;
		XtSetSensitive(controlButtons, True);
		XuToggleButtonSet(link_btn[control_btn_no].pw, True, False);
		GV_atlgrp = NULL;
		set_linked_to_option_list(False);
		(void) IngredCommand(GE_TIMELINK, "MASTER_LINK GLOBAL");
		(void) IngredCommand(GE_TIMELINK, link_btn[control_btn_no].cmd);
	}
	else if(GV_atlgrp->ntlfld > 1)
	{
		mapFldBtnsMan = True;
		mapFldIndsMan = True;
		XtSetSensitive(controlButtons, True);
		set_linked_to_option_list(True);
		relabel_field_list();
	}
	else
	{
		set_linked_to_option_list(False);
		XtSetSensitive(controlButtons, False);
		(void) IngredCommand(GE_TIMELINK, "MASTER_LINK GLOBAL");
	}
	XtSetMappedWhenManaged(fldBtnsMan, mapFldBtnsMan);
	XtSetMappedWhenManaged(fldIndsMan, mapFldIndsMan);
	set_context_menu_visibility();
}


/* Control setting button sensitivity and managing the subpanel controls.
 */
static void manage_link_controls(void)
{
	XtSetSensitive(clearBtn, IN_DELETE_MODE);

	if(MERGING)
	{
		XtUnmapWidget(selectIntermediateLink);
		XtMapWidget(mergeSubpanel);
		XtSetSensitive(mergeSubpanel, GV_atlgrp->atlfldno > 0);
	}
	else if(MOVING)
	{
		XtUnmapWidget(mergeSubpanel);
		XtMapWidget(selectIntermediateLink);
		/* Ingred can not handle continuous field intermediate links */
		XtSetSensitive(selectIntermediateLink, 
			GV_atlgrp->atlfld->info->element->fld_type != FpaC_CONTINUOUS &&
			GV_atlgrp->atlfld->info->element->fld_type != FpaC_VECTOR);

	}
	else
	{
		XtUnmapWidget(mergeSubpanel);
		XtUnmapWidget(selectIntermediateLink);
	}
}


/*ARGSUSED*/
static void field_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int n;
	char mbuf[300];
	int ndx = PTR2INT(client_data);

	if(!XmToggleButtonGetState(w)) return;

	GV_atlgrp->atlfld = GV_atlgrp->tlfld[ndx];
	GV_atlgrp->atlfldno = ndx;
	if(ndx == 0)
	{
		send_edit_cmds = True;
		set_linked_to_option_list(False);
		strcpy(mbuf, "MASTER_LINK ");
		strcat(mbuf, GV_atlgrp->name);
		(void) IngredCommand(GE_TIMELINK, mbuf);
		(void) IngredCommand(GE_TIMELINK, link_btn[control_btn_no].cmd);
	}
	else
	{
		set_linked_to_option_list(True);
		send_edit_cmds = !(GV_atlgrp->atlfld->link_state == LINKED_TO_MASTER ||
							GV_atlgrp->atlfld->link_state == LINKED_TO_FIELD);
		snprintf(mbuf, sizeof(mbuf), "FIELD %s %s", GV_atlgrp->atlfld->info->element->name,
			GV_atlgrp->atlfld->info->level->name);
		if(panel_active)
		{
			(void) IngredCommand(GE_DEPICTION, mbuf);
			if(send_edit_cmds)
				(void) IngredCommand(GE_TIMELINK, link_btn[control_btn_no].cmd);
		}
	}
	XtSetSensitive(controlButtons, send_edit_cmds);
	XuToggleButtonSet(link_btn[control_btn_no].pw, send_edit_cmds, False);

	/* Ingred can not handle link nodes for continuous fields.  */
	XtSetSensitive(selectIntermediateLink, (MOVING &&
									GV_atlgrp->atlfld->info->element->fld_type != FpaC_CONTINUOUS &&
									GV_atlgrp->atlfld->info->element->fld_type != FpaC_VECTOR)       );

	/* Configure the merge sub-panel for the active field.
	 */
	XuComboBoxDeleteAllItems(mergeField);
	XuComboBoxDeleteAllItems(mergeSource);
	if(GV_atlgrp->atlfldno > 0)
	{
		int nlink = GV_atlgrp->atlfld->info->element->elem_detail->linking->nlink;

		if(nlink+1 >= merge_fd_len)
			merge_fd = MoreMem(merge_fd, FLD_DESCRIPT, (merge_fd_len=nlink+1));

		/* The active field is always a choice along with any linking fields associated with the element.
		 */
		init_fld_descript(&merge_fd[0]);
		(void) set_fld_descript(&merge_fd[0],
				FpaF_ELEMENT, GV_atlgrp->atlfld->info->element,
				FpaF_LEVEL, GV_atlgrp->atlfld->info->level,
				FpaF_END_OF_LIST);
		XuComboBoxAddItem(mergeField, merge_fd[0].fdef->sh_label, 0);

		for(ndx = 1, n = 0; n < nlink; n++)
		{
			FpaConfigElementStruct *elem;
			FpaConfigLevelStruct   *level;

			elem  = GV_atlgrp->atlfld->info->element->elem_detail->linking->link_elems[n];
			level = GV_atlgrp->atlfld->info->element->elem_detail->linking->link_levels[n];
			if (!level) level = GV_atlgrp->atlfld->info->level;

			/* The active field must not be added again even if it is in the list. */
			if(elem == GV_atlgrp->atlfld->info->element && level == GV_atlgrp->atlfld->info->level)
				continue;
			
			init_fld_descript(&merge_fd[ndx]);
			if(set_fld_descript(&merge_fd[ndx], FpaF_ELEMENT, elem, FpaF_LEVEL, level, FpaF_END_OF_LIST))
			{
				XuComboBoxAddItem(mergeField, merge_fd[ndx].fdef->sh_label, 0);
				ndx++;
			}
		}
		XuComboBoxSelectPos(mergeField, 1, True);
	}

	manage_link_controls();
	set_context_menu_visibility();
}


/* If the field to to be linked to the master links or to another field then
*  we want to turn on the master link or the other field selection button.
*  If there are any other fields linked to the active field then these will
*  have to relinked to the given field.
*/
/*ARGSUSED*/
static void link_to_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i;
	char mbuf[300];
	Boolean others;
	int ndx = PTR2INT(client_data);

	if(!GV_atlgrp->atlfld) return;
	
	GV_atlgrp->atlfld->link_status = NOLINKS;
	others = False;

	switch(ndx)
	{
		case LINKED_TO_MASTER:
			for(i = 1; i < GV_atlgrp->ntlfld; i++)
			{
				if(GV_atlgrp->tlfld[i]->link_state != LINKED_TO_FIELD) continue;
				if(GV_atlgrp->tlfld[i]->link_fld != GV_atlgrp->atlfld) continue;
				others = True;
				GV_atlgrp->tlfld[i]->link_state = LINKED_TO_MASTER;
				GV_atlgrp->tlfld[i]->link_status = NOLINKS;
				GV_atlgrp->tlfld[i]->link_fld = NULL;
				snprintf(mbuf, sizeof(mbuf), "FIELD %s %s",
					GV_atlgrp->tlfld[i]->info->element->name,
					GV_atlgrp->tlfld[i]->info->level->name);
				(void) IngredCommand(GE_DEPICTION, mbuf);
				(void) IngredCommand(GE_TIMELINK, "LINK_TO MASTER_LINK");
			}
			if(others)
			{
				snprintf(mbuf, sizeof(mbuf), "FIELD %s %s",
					GV_atlgrp->atlfld->info->element->name,
					GV_atlgrp->atlfld->info->level->name);
				(void) IngredCommand(GE_DEPICTION, mbuf);
			}
			strcpy(mbuf, "LINK_TO MASTER_LINK");
			GV_atlgrp->atlfld->link_state = LINKED_TO_MASTER;
			GV_atlgrp->atlfld->link_status = NOLINKS;
			GV_atlgrp->atlfld->link_fld = NULL;
			redisplay_group_timelink_state(GV_atlgrp);
			XuUpdateDisplay(GW_timelinkPanel);
			if(panel_active) (void) IngredCommand(GE_TIMELINK, mbuf);
			break;

		case LINKED_TO_SELF:
			strcpy(mbuf, "LINK_TO");
			GV_atlgrp->atlfld->link_state = LINKED_TO_SELF;
			GV_atlgrp->atlfld->link_status = NOLINKS;
			GV_atlgrp->atlfld->link_fld = NULL;
			redisplay_group_timelink_state(GV_atlgrp);
			XuUpdateDisplay(GW_timelinkPanel);
			if(panel_active)
			{
				(void) IngredCommand(GE_TIMELINK, mbuf);
				if(send_edit_cmds)
					(void) IngredCommand(GE_TIMELINK, link_btn[control_btn_no].cmd);
			}
			break;

		default:
			for(i = 1; i < GV_atlgrp->ntlfld; i++)
			{
				if(GV_atlgrp->tlfld[i]->link_state != LINKED_TO_FIELD) continue;
				if(GV_atlgrp->tlfld[i]->link_fld != GV_atlgrp->atlfld) continue;
				others = True;
				GV_atlgrp->tlfld[i]->link_status = NOLINKS;
				GV_atlgrp->tlfld[i]->link_fld = GV_atlgrp->tlfld[ndx];;
				snprintf(mbuf, sizeof(mbuf), "FIELD %s %s",
					GV_atlgrp->tlfld[i]->info->element->name,
					GV_atlgrp->tlfld[i]->info->level->name);
				(void) IngredCommand(GE_DEPICTION, mbuf);
				(void) IngredCommand(GE_TIMELINK, "LINK_TO MASTER_LINK");
			}
			if(others)
			{
				snprintf(mbuf, sizeof(mbuf), "FIELD %s %s",
					GV_atlgrp->atlfld->info->element->name,
					GV_atlgrp->atlfld->info->level->name);
				(void) IngredCommand(GE_DEPICTION, mbuf);
			}
			GV_atlgrp->atlfld->link_state = LINKED_TO_FIELD;
			GV_atlgrp->atlfld->link_status = NOLINKS;
			GV_atlgrp->atlfld->link_fld = GV_atlgrp->tlfld[ndx];
			snprintf(mbuf, sizeof(mbuf), "LINK_TO %s %s",
				GV_atlgrp->atlfld->link_fld->info->element->name,
				GV_atlgrp->atlfld->link_fld->info->level->name);
			redisplay_group_timelink_state(GV_atlgrp);
			XuUpdateDisplay(GW_timelinkPanel);
			if(panel_active) (void) IngredCommand(GE_TIMELINK, mbuf);
			break;
	}
}


/*  Display the timelink state variables for a group. This also
*   arranges the order of the fields so that any field that is linked
*   to another comes directly under the other field in the order.
*/
static void redisplay_group_timelink_state(GROUP *tlg )
{
	int i, j, k, n, nfl;
	Boolean found;
	FIELD_INFO *field, **fl;

	if(!GV_edit_mode) return;
	if(!tlg) return;

	if(!GV_atlgrp) GV_atlgrp = tlg;

	/* Turn off the currently active field button in case we end up
	*  moving its position in the list.
	*/
	if(GV_atlgrp->atlfldno < nfldBtns)
	{
		XuToggleButtonSet(fldBtns[GV_atlgrp->atlfldno], False, False);
	}

	SetGroupLinkStatus(tlg);

	/* Now reorder the fields within the group depending on their link_to
	*  status. We want fields which are linked to other fields to be directly
	*  "under" the linked to field. First put in original order.
	*/
	n = 1;
	for(i = 0; i < GV_nfield; i++)
	{
		for( j = 1; j < tlg->ntlfld; j++)
		{
			if(tlg->tlfld[j] != GV_field[i]) continue;
			field = tlg->tlfld[j];
			tlg->tlfld[j] = tlg->tlfld[n];
			tlg->tlfld[n] = field;
			n++;
		}
	}

	/* Now do fields linked to other fields. First we remove all fields
	*  which are linked to other fields then we insert them back into 
	*  the list in order. If the field is linked to another field but
	*  that field does not exist we must reset it to the master link;
	*/
	fl = NewMem(FIELD_INFO*, tlg->ntlfld);

	n = nfl = 1;
	for(i = 1; i < tlg->ntlfld; i++)
	{
		if(tlg->tlfld[i]->link_state != LINKED_TO_FIELD)
		{
			tlg->tlfld[n] = tlg->tlfld[i];
			n++;
		}
		else
		{
			found = False;
			for(j = 1; j < tlg->ntlfld; j++)
			{
				if(tlg->tlfld[i]->link_fld == tlg->tlfld[j])
				{
					found = True;
					fl[nfl] = tlg->tlfld[i];
					nfl++;
					break;
				}
			}
			if(!found)
			{
				tlg->tlfld[i]->link_state = LINKED_TO_MASTER;
				tlg->tlfld[n] = tlg->tlfld[i];
				n++;
			}
		}
	}

	/* If there are some linked to field entries we insert them.
	*/
	if(nfl > 1)
	{
		for(i = nfl-1; i > 0; i--)
		{
			for(j = 1; j < n; j++)
			{
				if(fl[i]->link_fld != tlg->tlfld[j]) continue;
				j++;
				for(k = n; k > j; k--)
				{
					tlg->tlfld[k] = tlg->tlfld[k-1];
				}
				tlg->tlfld[j] = fl[i];
				n++;
				break;
			}
		}
	}
	FreeItem(fl);

	/* Now do the master links.
	*/
	n = 1;
	for(i = 1; i < tlg->ntlfld; i++)
	{
		if(tlg->tlfld[i]->link_state != LINKED_TO_MASTER) continue;
		field = tlg->tlfld[i];
		for(j = i; j > n; j--)
		{
			tlg->tlfld[j] = tlg->tlfld[j-1];
		}
		tlg->tlfld[n] = field;
		n++;
	}

	/* Reset the active field number.
	*/
	for(i = 0; i < tlg->ntlfld; i++)
	{
		if(tlg->atlfld != tlg->tlfld[i]) continue;
		tlg->atlfldno = i;
		break;
	}

	/* The following can only be executed if the panel is active (showing)
	*/
	if(panel_active)
	{
		relabel_group_list();
		relabel_field_list();
	}
}


/* Set the linked to option list for the currently active group and field
*  within that group. If the display parameter is False, set the label to
*  "Self" and set the option list insensitive. If True decide which fields
*  are to appear in the list and set it sensitive.
*/
static void set_linked_to_option_list(Boolean display )
{
	int i;
	Widget w, btn;
	XmString label;

	XtUnmanageChildren(lk2Btns, (Cardinal) nfldBtns);
	check_button_array_size();
/*
*  NOTE: This following line must change if the field type which can be linked
*        to another changes.
*/
	/*
	if(display && GV_atlgrp->atlfld->editor == GetEditor(CONTINUOUS_FIELD_EDITOR))
	*/
	if(display &&
		(  GV_atlgrp->atlfld->info->element->fld_type == FpaC_CONTINUOUS
		|| GV_atlgrp->atlfld->info->element->fld_type == FpaC_VECTOR) )
	{
		XtSetSensitive(linkToLabel, True);
		XtSetSensitive(lk2BtnsMan, True);
		for(i = 1; i < GV_atlgrp->ntlfld; i++)
		{
			if(i != GV_atlgrp->atlfldno)
			{
				if( GV_atlgrp->tlfld[i]->exists &&
					GV_atlgrp->tlfld[i]->link_state == LINKED_TO_SELF &&
					(  GV_atlgrp->tlfld[i]->info->element->fld_type == FpaC_CONTINUOUS
					|| GV_atlgrp->tlfld[i]->info->element->fld_type == FpaC_VECTOR) )
					XtManageChild(lk2Btns[i]);
				else
					XtUnmanageChild(lk2Btns[i]);
			}
		}
		if(GV_atlgrp->atlfld->link_state == LINKED_TO_MASTER)
		{
			btn = XuMenuFindButton(lk2BtnsMan, MASTER_ID);
		}
		else if(GV_atlgrp->atlfld->link_state == LINKED_TO_FIELD)
		{
			for(i = 1; i < GV_atlgrp->ntlfld; i++)
			{
				if(GV_atlgrp->atlfld->link_fld != GV_atlgrp->tlfld[i]) continue;
				btn = lk2Btns[i];
				break;
			}
		}
		else
		{
			btn = XuMenuFindButton(lk2BtnsMan, SELF_ID);
		}
		XtVaSetValues(lk2BtnsMan, XmNmenuHistory, btn, NULL);
		XtVaGetValues(btn, XmNlabelString, &label, NULL);
		w = XmOptionButtonGadget(lk2BtnsMan);
		XtVaSetValues(w, XmNlabelString, label, NULL);
	}
	else
	{
		btn = XuMenuFindButton(lk2BtnsMan, SELF_ID);
		XtVaSetValues(lk2BtnsMan, XmNmenuHistory, btn, NULL);
		XtSetSensitive(linkToLabel, False);
		XtSetSensitive(lk2BtnsMan, False);
	}
}


/* Responds to the user selection of linking direction and other state
*  commands such as remove link.
*/
/*ARGSUSED*/
static void action_state_cb(Widget	w , XtPointer client_data , XtPointer user_data )
{
	if(!XmToggleButtonGetState(w)) return;

	control_btn_no = PTR2INT(client_data);
	if(last_control_btn_no != control_btn_no)
	{
		last_control_btn_no = control_btn_no;
		SetLinkTypeToNormal();
		SetActiveTimelinkDepiction(LINKING_DIRECTION? ACTIVE:NONE, LINKING_BACKWARD);
		manage_link_controls();
		if(send_edit_cmds)
			(void) IngredCommand(GE_TIMELINK, link_btn[control_btn_no].cmd);
		top_context_btn = CM_NONE;
		set_context_menu_visibility();
	}
}


/* Responds to the user selection of the special linking in the move commend.
*/
/*ARGSUSED*/
static void select_intermediate_link_cb(Widget w ,XtPointer client_data, XtPointer user_data )
{
	if(XmToggleButtonGetState(w))
		(void) IngredCommand(GE_ACTION, "STATE LINK_MODE INTERMEDIATE");
	else
		(void) IngredCommand(GE_ACTION, "STATE LINK_MODE NORMAL");
}


/*ARGSUSED*/
static void timelink_clear_cb(Widget w , XtPointer client_data , XtPointer user_data )
{
	/* This is its own callcack as when clearing we need to send the
	 * current linking state after the clear
	 */
	(void) IngredCommand(GE_TIMELINK, "EDIT DELINK CLEAR");
	SetLinkTypeToNormal();
	(void) IngredCommand(GE_TIMELINK, link_btn[control_btn_no].cmd);
}


/*ARGSUSED*/
static void edit_commands_cb(Widget	w , XtPointer client_data , XtPointer user_data )
{
	String cmd = (String) client_data;
	(void) IngredCommand(GE_EDIT, cmd);
	if(same(cmd,E_UNDO) && MERGING)
		SetLinkTypeToNormal();
}


/* Relabels and resets the indicator state for the groups.
*/
static void relabel_group_list(void)
{
	int i, nbtns, ngrp_btns, extra, grp_count;
	Dimension height, spacing, grp_height, fld_height, scroll_spacing;
	Widget scroll;
	XmString label;
	Pixel color;
	XtWidgetGeometry size;

	if(!GV_atlgrp) return;

	XtUnmanageChildren(grpBtns, (Cardinal) ngrpBtns);
	XtUnmanageChildren(grpInds, (Cardinal) ngrpBtns);

	if(GV_ntlgrps > ngrpBtns)
	{
		label = XuNewXmString("");
		grpBtns = MoreWidgetArray(grpBtns, GV_ntlgrps);
		grpInds = MoreWidgetArray(grpInds, GV_ntlgrps);
		for(i = ngrpBtns; i < GV_ntlgrps; i++)
		{
			grpBtns[i] = XmCreateToggleButton(grpBtnsMan, "gb", NULL, 0);
			XtAddCallback(grpBtns[i], XmNvalueChangedCallback, group_cb, INT2PTR(i));
			grpInds[i] = XmVaCreateLabel(grpIndsMan, "gb",
				XmNlabelString, label,
				XmNrecomputeSize, False,
				XmNheight, ind_height,
				XmNwidth, ind_width,
				XmNmarginWidth, 0,
				XmNmarginHeight, 0,
				NULL);
		}
		ngrpBtns = GV_ntlgrps;
		XmStringFree(label);
	}

	for(grp_count = 0, i = 0; i < GV_ntlgrps; i++)
	{
		/* We don't want groups with no linkable fields to show up
		*  in the group list.
		*/
		if(GV_tlgrps[i]->ntlfld < 2) continue;
		grp_count++;

		label = XuNewXmString(GV_tlgrps[i]->label);
		XtVaSetValues(grpBtns[i],
			XmNlabelString, label,
			XmNset, (GV_atlgrp == GV_tlgrps[i]),
			NULL);
		XmStringFree(label);
		SetGroupLinkStatus(GV_tlgrps[i]);
		switch(GV_tlgrps[i]->fields_link_status)
		{
			case NOLINKS:      color = noLinkColorBg;      break;
			case SOME_LINKS:   color = partialColorBg;     break;
			case LINKED:       color = almostColorBg;      break;
			case FIELD_INTERP: color = fieldInterpColorBg; break;
			case INTERPOLATED: color = allSetColorBg;      break;
			case NOT_LINKABLE: color = notLinkableColorBg; break;
		}
		XtVaSetValues(grpInds[i], XmNbackground, color, NULL);
		XtManageChild(grpInds[i]);
		XtManageChild(grpBtns[i]);
	}

	/* Set the height of the group window to fit the number of buttons
	 * specified for each in the resource file. Note that the group
	*  window is only allowed to shrink.
	*/
	if(grp_count != ngrp_last)
	{
		ngrp_last = grp_count;
		ngrp_btns = XuGetIntResource(RNtimelinkPanelMaxGroupBtns, 4);
		nbtns = MIN(ngrp_btns, grp_count);
		XtVaGetValues(grpBtnsMan,
			XmNspacing, &spacing,
			XmNmarginHeight, &height,
			NULL);
		height = 4*height;
		XtVaGetValues(groupWindow,
			XmNhorizontalScrollBar, &scroll,
			XmNspacing, &scroll_spacing,
			NULL);
		extra = 0;
		if(XtIsManaged(scroll) && nbtns > ngrp_btns)
		{
			size.request_mode = CWHeight;
			(void) XtQueryGeometry(scroll, NULL, &size);
			extra = (int)(size.height + scroll_spacing);
		}
		grp_height = (Dimension)((int)(btn_height+spacing)*nbtns+(int)height+extra);
		XtVaSetValues(groupWindow, XmNheight, grp_height, NULL);
	}
	XuUpdateDisplay(GW_timelinkPanel);
}


/* Relabels the fields and the link to option selector and resets
*  the field state indicators.
*/
static void relabel_field_list(void)
{
	int     i, n, active;
	Pixel   color;
	Pixmap  pixmap;
	Widget  *indList, *btnList, *lnkList;

	if(!GV_atlgrp || GV_atlgrp->ntlfld < 1) return;

	XtUnmanageChildren(fldBtns, (Cardinal) nfldBtns);
	XtUnmanageChildren(fldInds, (Cardinal) nfldBtns);
	XtUnmanageChildren(lk2Btns, (Cardinal) nfldBtns);

	check_button_array_size();

	indList = NewWidgetArray(GV_atlgrp->ntlfld);
	btnList = NewWidgetArray(GV_atlgrp->ntlfld);
	lnkList = NewWidgetArray(GV_atlgrp->ntlfld);

	active = 1;
	n = 0;
	for(i = 0; i < GV_atlgrp->ntlfld; i++)
	{
		if(!GV_atlgrp->tlfld[i]->exists) continue;
		XuWidgetLabel(fldBtns[i], GV_atlgrp->tlfld[i]->info->sh_label);
		XuWidgetLabel(lk2Btns[i], GV_atlgrp->tlfld[i]->info->sh_label);
		switch(GV_atlgrp->tlfld[i]->link_status)
		{
			case NOT_LINKABLE: pixmap = notLinkArrow; color = notLinkableColorBg; break;
			case NOLINKS:      pixmap = noLinkArrow;  color = noLinkColorBg;      break;
			case SOME_LINKS:   pixmap = partialArrow; color = partialColorBg;     break;
			case LINKED:       pixmap = almostArrow;  color = almostColorBg;      break;
			case FIELD_INTERP: pixmap = almostArrow;  color = fieldInterpColorBg; break;
			case INTERPOLATED: pixmap = allSetArrow;  color = allSetColorBg;      break;
		}
		XtVaSetValues(fldInds[i],
			XmNlabelType, (GV_atlgrp->tlfld[i]->link_state == LINKED_TO_SELF) ? XmSTRING:XmPIXMAP,
			XmNbackground, color,
			XmNlabelPixmap, pixmap,
			NULL);
		indList[n] = fldInds[i];
		btnList[n] = fldBtns[i];
		lnkList[n] = lk2Btns[i];
		n++;
		if(i == GV_atlgrp->atlfldno) active = i;
	}
	XtManageChildren(indList, (Cardinal) n);
	XtManageChildren(btnList, (Cardinal) n);
	XtManageChildren(lnkList, (Cardinal) n);

	FreeItem(indList);
	FreeItem(btnList);
	FreeItem(lnkList);

	XuToggleButtonSet(fldBtns[active], True, True);
	XuUpdateDisplay(GW_timelinkPanel);
	set_linked_to_option_list(True);
}


/* Check to see if the button arrays have to be increased in size
*  due to an increase in the field list.
*/
static void check_button_array_size(void)
{
	int      i;
	XmString label;
	Widget   w;

	if(nfldBtns >= GV_atlgrp->ntlfld) return;

	label = XuNewXmString("");
	fldBtns = MoreWidgetArray(fldBtns, GV_atlgrp->ntlfld);
	fldInds = MoreWidgetArray(fldInds, GV_atlgrp->ntlfld);
	lk2Btns = MoreWidgetArray(lk2Btns, GV_atlgrp->ntlfld);

	w = XuMenuFind(lk2BtnsMan, NoId);

	for(i = nfldBtns; i < GV_atlgrp->ntlfld; i++)
	{
		char buf[16];
		(void) snprintf(buf, 16, "fb%d", i);
		fldBtns[i] = XmCreateToggleButton(fldBtnsMan, buf, NULL, 0);
		XtAddCallback(fldBtns[i], XmNvalueChangedCallback, field_cb, INT2PTR(i));
		fldInds[i] = XmVaCreateLabel(fldIndsMan, "gb",
			XmNlabelString, label,
			XmNrecomputeSize, False,
			XmNheight, ind_height,
			XmNwidth, ind_width,
			XmNmarginWidth, 0,
			XmNmarginHeight, 0,
			NULL);
		(void) snprintf(buf, 16, "lb%d", i);
		lk2Btns[i] = XmCreatePushButton(w, buf, NULL, 0);
		XtAddCallback(lk2Btns[i], XmNactivateCallback, link_to_cb, INT2PTR(i));
	}
	nfldBtns = GV_atlgrp->ntlfld;
	XmStringFree(label);
}


static void merge_cb (Widget w, XtPointer client_data, XtPointer call_data)
{
	static int        fld_ndx = 0;		/* selected merge field */
	static int        src_ndx = 0;		/* selected merge source */
	static SourceList src_list = NULL;	/* list of available merge sources */
	static int        src_list_size = 0;/* current size of the list */

	int    n, len, nsrcs;
	Source *srcs;

	if(w == mergeField)
	{
		if(!((XmComboBoxCallbackStruct*)call_data)->item_position > 0) return;

		fld_ndx = ((XmComboBoxCallbackStruct*)call_data)->item_position-1;
		XuComboBoxDeleteAllItems(mergeSource);

		SourceListByType(SRC_DEPICT|SRC_FPA, FpaC_TIMEDEP_ANY, &srcs, &nsrcs);
		if(nsrcs > src_list_size)
			src_list = MoreMem(src_list, Source, (src_list_size = nsrcs));

		for(len = 0, n = 0; n < nsrcs; n++)
		{
			(void) set_fld_descript(&merge_fd[fld_ndx],
					FpaF_SOURCE, SrcDef(srcs[n]), 
					FpaF_SUBSOURCE, SrcSubDef(srcs[n]),
					FpaF_END_OF_LIST);
			if(check_link_filename(&merge_fd[fld_ndx]))
			{
				src_list[len++] = srcs[n];
				XuComboBoxAddItem(mergeSource, SrcLabel(srcs[n]), 0);
			}
		}

		if(len > 0) XuComboBoxSelectPos(mergeSource, 1, True);
		XtSetSensitive(mergeFetchBtn, (len > 0));
		XtSetSensitive(mergeSelectAllBtn, False);
	}
	else if(w == mergeSource)
	{
		if(((XmComboBoxCallbackStruct*)call_data)->item_position > 0)
			src_ndx = ((XmComboBoxCallbackStruct*)call_data)->item_position - 1;
	}
	else if(w == mergeFetchBtn)
	{
		(void) IngredVaCommand(GE_TIMELINK, "EDIT MERGE FETCH  %s %s %s %s",
			SrcName(src_list[src_ndx]), SrcSubDashName(src_list[src_ndx]),
			merge_fd[fld_ndx].edef->name, merge_fd[fld_ndx].ldef->name);
		XtSetSensitive(mergeSelectAllBtn, True);
	}
	/* The select all can come from the panel button or the context button */
	else if(same((String) client_data, E_SELECT_ALL))
	{
		(void) IngredCommand(GE_TIMELINK, "EDIT MERGE SELECT_ALL");
	}

	set_context_menu_visibility();
}


/* The following three functions are for the right button context menu. The callback
 * just activates the approproate toggle button and the sensitivity function does
 * what it says.
 */
/*ARGSUSED*/
static void context_menu_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmToggleButtonSetState((Widget) client_data, True, True);
}


/* Create the right mouse button context menu for the panel.
 */
static void create_context_menu(void)
{
	int i;

	panelContextMenu = CreatePanelContextMenu("timelinkContextMenu");

	/* This button is for merge only and has a different callback function from the rest */
	cm_selectAll = XmVaCreatePushButton(panelContextMenu, "selectAll", NULL);
	XtAddCallback(cm_selectAll, XmNactivateCallback, merge_cb, (XtPointer) E_SELECT_ALL);

	cm_new = XmVaCreatePushButton(panelContextMenu, "newChain", NULL);
	XtAddCallback(cm_new, XmNactivateCallback, edit_commands_cb, (XtPointer) E_NEW_CHAIN);

	cm_current = XmVaCreatePushButton(panelContextMenu, "currentTime", NULL);
	XtAddCallback(cm_current, XmNactivateCallback, edit_commands_cb, (XtPointer) "NOEXTRAP");

	cm_end = XmVaCreatePushButton(panelContextMenu, "endChain", NULL);
	XtAddCallback(cm_end, XmNactivateCallback, edit_commands_cb, (XtPointer) E_END_CHAIN);

	cm_accept = XmVaCreatePushButton(panelContextMenu, "acceptBtn", NULL);
	XtAddCallback(cm_accept, XmNactivateCallback, edit_commands_cb, (XtPointer) E_UPDATE);

	cm_cancel = XmVaCreatePushButton(panelContextMenu, "cancelBtn", NULL);
	XtAddCallback(cm_cancel, XmNactivateCallback, edit_commands_cb, (XtPointer) E_CANCEL);

	cm_undo = XmVaCreatePushButton(panelContextMenu, "undoBtn", NULL);
	XtAddCallback(cm_undo, XmNactivateCallback, edit_commands_cb, (XtPointer) E_UNDO);

	cm_clear = XmVaCreatePushButton(panelContextMenu, "clearBtn", NULL);
	XtAddCallback(cm_clear, XmNactivateCallback, timelink_clear_cb, NULL);

	cm_separator = XmVaCreateSeparator(panelContextMenu, "sep", NULL);

	for(i = 0; i < (int) XtNumber(link_btn); i++)
	{
		if(link_btn[i].name)
		{
			link_btn[i].cw = XmVaCreateManagedPushButton(panelContextMenu, link_btn[i].name, NULL);
			XtAddCallback(link_btn[i].cw, XmNactivateCallback, context_menu_cb, (XtPointer) link_btn[i].pw);
		}
	}
}


/* Note that the select all button should only be available during the merge operation,
 * thus the check to ensure that we are in merge mode for this context button.
 */
static void set_context_menu_visibility(void)
{
	Boolean show_separator;

	Manage(cm_new, top_context_btn == CM_NEW);
	Manage(cm_end, top_context_btn == CM_END);
	Manage(cm_current, top_context_btn == CM_CURRENT);
	Manage(cm_selectAll, XtIsSensitive(mergeSelectAllBtn) && MERGING);
	Manage(cm_accept, XtIsSensitive(acceptBtn));
	Manage(cm_cancel, XtIsSensitive(cancelBtn));
	Manage(cm_undo, XtIsSensitive(undoBtn));
	Manage(cm_clear, XtIsSensitive(clearBtn));

	show_separator = (	XtIsManaged(cm_end)       ||
						XtIsManaged(cm_new)       ||
						XtIsManaged(cm_current)   ||
						XtIsManaged(cm_cancel)    ||
						XtIsManaged(cm_undo)      ||
						XtIsManaged(cm_clear)     ||
						XtIsManaged(cm_selectAll) ||
						XtIsManaged(cm_accept)
					);
	Manage(cm_separator, show_separator);
}
