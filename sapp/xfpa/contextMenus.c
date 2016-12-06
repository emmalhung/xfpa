/****************************************************************************/
/*
*  File:    contextMenus.c
*
*  Purpose: Handles all functionality associated with creating and managing
*           the right button popup context menus.
*           
*  Menus:   There are three of these for different uses and they are used
*           and configured for different contexts by the functions in this
*           file.
*
*           Main Context Menu - This menu is associated with the editor and
*                               is the one that pops up when the editor is
*           in its ready to edit state, but no objects have been selected for
*           editing.
*
*           Selected Menu - When an object has been selected, this is the
*                           menu that is configured for the actions that can
*           be performed on the selected object. It is the responsibility of
*           the specific editor object type functions to do the configuration.
*
*           Simple Menu - A context menu used for menus with a fixed content.
*                         These include point-by-point draw, the modify menu
*           and the pan mode termination. There are no public interfaces for
*           this menu.
*
*  Notes:   The callback functions activate work procedures to make sure
*           that the event loop is empty before the actions are processed.
*           This sometimes results in a "snappier" feeling performance and
*           should avoid possible race conditions.
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
#include <ingred.h>
/* Undefine bzero and bcopy to stop compiler complaints */
#undef bzero
#undef bcopy
#include <Xm/CascadeB.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>
#include <Xm/ToggleB.h>
#include "global.h"
#include "depiction.h"
#include "editor.h"
#include "observer.h"
#include "resourceDefines.h"
#define CONTEXT_MENU_MAIN
#include "contextMenu.h"
#undef  CONTEXT_MENU_MAIN

#define SENSITIVE(w,s)	if(w)XtSetSensitive(w,s)

#define PAN_DONE	"zd"
#define CANCEL_ZOOM	"zc"

/* Private variables */

/* When the editor starts up there are various outline areas that only exist after
 * the user creates them. Once created they exist for the entire program run. The 
 * following variables are set by ingred status messages when the areas are created.
 */
static Boolean lastMovedExists = False;
static Boolean lastStompExists = False;
static Boolean lastDrawnExists = False;
static Boolean lastHoleExists  = False;

/* This is the menu that popups up when the editor is mostly in the "ready for action"
 * state - that is an edit state has been chosen but nothing is actually done yet. Not
 * all of the widgets are managed at the same time so that the menu will appear to
 * change depending on the editor state. This menu can be replaced by one specific to
 * panels other then the editor and this is reflected in the active variable. See
 * SetActiveContextMenu().
 */
static struct {
	Boolean active;		/* Is the mcm menu (the one defined by this structure) active */
	Widget  assigned;	/* The popup menu assigned to show on left mouse button activation */
	Widget  popup;		/* The default assigned popup */
	/*
	 * Several edit functions allow the user to use preset outlines for
	 * selection. This defines the context menu elements for the control.
	 * The display of the menu is controlled by commands sent from Ingred.
	 * Note that this is automatically sync'ed with buttons in the acm menu.
	 * The first two buttons always exist and are created on startup. The 
	 * rest are for areas defined in the setup file.
	 */
	struct {
		Boolean show;		/* Is the cascade button showing? */
		Widget  cascade;	/* Menu popup cascade button */
		Widget  separator;	/* Separator below the cascade btn */
		Widget  popup;		/* The cascade button popup widget */
		Widget  *btn;		/* Buttons for selecting outline area */
		String  *data;		/* Data that defines the outline area to ingred */
		int     nbtns;		/* number of buttons created in addition to required two */
	} outline;
	/*
	 * A generic button with an associated callback function and its data plus two
	 * cascade buttons whose labels and what popup menus to use are set through
	 * functions called by the field functions. The popups associated with the
	 * cascade buttons must be supplied by the calling function.
	 */
	struct {
		Widget    pushBtn;
		void      (*pushBtnFcn)(XtPointer);
		XtPointer pushBtnFcnInfo;
		Widget    cascade[2];
		Widget    separator;
	} aux;
	/*
	 * Buttons that are common to many edit functions and are
	 * mostly controlled by ingred status calls.
	 */
	Widget create;
	Widget createSeparator;
	Widget selectAll;
	Widget selectAllSeparator;
	Widget proceed;
	Widget join;
	Widget lineBreak;
	Widget rejoin;
	Widget paste;
	Widget delete; 
	Widget accept; 
	Widget cancel; 
	Widget undo; 
	Widget clear; 
	/*
	 * Buttons that switch the current edit function
	 */
	Widget cmdBtnsSeparator;
	Widget btns[NR_EDIT_BTNS];	/* buttons that reflect the edit choices */
	/*
	 * Buttons for selecting which field to switch to.
	 */
	struct {
		Widget cascade;			/* pullright cascade selection button */
		Widget popup;			/* pullright menu popup */
		Widget *btns;			/* field selection buttons */
		int    nbtns;			/* number of buttons created to date */
	} fs;						/* field selection right pulldown */
	/*
	 * The structure associated with the pullright cascade menu choice that
	 * allows the selection of the attributes to be applied. Note that this
	 * is automatically synced with the object action context menu below.
	 */
	struct {
		int    active;			/* menu active? */
		Widget separator;		/* visual separator */
		int    max_per_col;		/* maximum number of buttons per pullright column */
		Widget popup;			/* pullright menu */
		Widget cascade;			/* cascade button to popup the menu */
		Widget *btns;			/* selection buttons */
		int    nbtns;			/* number of buttons created to date */
		int    count;			/* number of buttons currently managed */
		int    ncolumns;		/* how many columns to display the data in */
		void   (*callback)(int);/* function to call when button activated */
	} ca;						/* context attributes pullright menu */
} mcm;
/*
 * For the selected object action context menu. This is the menu that
 * is made active when an object is selected for some editing procedure
 * and Ingred sends us a message telling us to go into a select state.
 */
static struct {
	Boolean active;
	Widget  popup;
	/*
	 * The labels and callbacks of the buttons are set by a call to
	 * ActivateSelectContextMenu. MAX_CONTEXT_ITEMS and CONTEXT are
	 * defined in the header file.
	 */
	struct {
		Widget   btns[MAX_CONTEXT_ITEMS];
		Widget   cascade[2];				/* for pullright menu insertion after the above buttons */
		Widget   btnsSeparator;				/* For visual grouping */
		XmString *btnLabels;				/* labels to give to btns */
		void     (*btnsFcn)(CONTEXT*, CAL);	/* callback function for the btns */
		CONTEXT  *btnsInfo;					/* context struct info for btnsFcn */
		CAL      btnsCAL;					/* cal to be send in btnsFcn */
		struct {
			Widget   btn;					/* See ActivateSelectContextMenu for use */
			Boolean  state;					/* Last copy state from Ingred */
		} copy;
	} ea;									/* selected object edit actions */
	/*
	 * The cascade button is assigned the same popup as mcm.outline.cascade
	 */
	struct {
		Widget  cascade;
		Widget  separator;
	} outline;
	/*
	 * Attribute menu subset synced with that in mcm.
	 */
	struct {
		Widget  separator;
		Widget  cascade;
		Widget  popup;
		Widget  *btns;
	} ca;
	/*
	 * Buttons controlled by Ingred messages
	 */
	Widget  selectAll;
	Widget  selectAllSeparator;
	Widget  accept; 
	Widget  cancel; 
	Widget  undo; 
} acm;
/*
 * Simple context menu. This can be configured for a variety of simple
 * things like leaving pan mode, terminating point-by-point drawing
 * and for terminating a modify.
 */
static struct {
	Boolean active;
	Widget  popup;
	Widget  panEndBtn;
	Widget  zoomCancel;
	Widget  done;
	Widget  confirm;
	Widget  accept;
	Widget  cancel;
	Widget  undo;
} scm;


/* Forward function declarations
 */
static void configure_outline_selection_menu(void);
static void set_outline_selection_sensitivity(void);
static void create_main_context_menu (void);
static void create_action_context_menu (void);
static void create_simple_context_menu(void);
static void edit_function_change_observer (String*, int);
static void edit_status_from_ingred_observer (CAL, String*, int);
static void manage_context_menu_appearance (void);
static void map_window_popup_cb (Widget, XtPointer, XtPointer);
static void pullright_cb (Widget, XtPointer, XtPointer);
static void zoom_mode_status (String*, int);



/*========================== Public Functions ==============================*/


/* A convienience function that creates a popup window for a panel to
 * use as a context menu. The return from this function is the widget
 * that must be passed to the SetActiveContextMenu() function below.
 */
Widget CreatePanelContextMenu(String id)
{
	Arg al[1];
	XtSetArg(al[0], XmNpopupEnabled, XmPOPUP_AUTOMATIC);
	return XmCreatePopupMenu(GW_mapWindow, id, al, 1);
}


/* Initialize all of the context menus created within this file.
 */
void CreateContextMenus(void)
{
	/* Initialize menu data structures */
	(void) memset((void *)&mcm, 0, sizeof(mcm));
	(void) memset((void *)&acm, 0, sizeof(acm));
	(void) memset((void *)&scm, 0, sizeof(scm));

	/* Attach the pre-popup callback */
	XtAddCallback(GW_mapWindow, XmNpopupHandlerCallback, map_window_popup_cb, NULL);

	/* These menus are always needed. */
	create_main_context_menu();
	create_simple_context_menu();
	ConfigureMainContextMenuForField();

	/* These are only needed in edit mode */
	if(GV_edit_mode)
	{
		create_action_context_menu();
		AddIngredObserver(edit_status_from_ingred_observer);
		AddObserver(OB_EDIT_FUNCTION_TO_CHANGE, edit_function_change_observer);
	}

	/* The menus need to know when we zoom */
	AddObserver(OB_ZOOM, zoom_mode_status);

	mcm.assigned = mcm.popup;
}



/* Set the popup menu that will be activated on a left mouse button. This can
 * be a popup menu defined by the calling routine or the predefined values
 * "FieldEditContextMenu" or "None". FieldEditContextMenu sets the mcm.popup as the
 * assigned context while None results in no menu being popped up on a left mouse
 * button click.
 */
void SetActiveContextMenu(Widget w)
{
	if(w == None)
	{
		mcm.active = False;
		ShowContextMenuAttributes(False); /* This needs to be turned off explicitly */
	}
	else if(w == FieldEditContextMenu)
	{
		mcm.active   = True;
		mcm.assigned = mcm.popup;
	}
	else if(XtIsWidget(w))
	{
		mcm.active   = True;
		mcm.assigned = w;
	}
	else
	{
		mcm.active = False;
		printf("ERROR [SetActiveContextMenu] : Input is not a widget.\n");
	}
}



/* Configure the main context menu for the currently active field type.
 */
void ConfigureMainContextMenuForField(void)
{
	if (!mcm.popup) return;

	if(!GV_edit_mode)
	{
		/* The only thing we are allowed do in viewer mode is clear samples */
		Manage(mcm.clear, True);
	}
	else if(GV_active_field)
	{
		int n;
		for(n = 0; n < NR_EDIT_BTNS; n++)
		{
			if (blank(GV_active_field->editor->btns[n].contextBtnId))
			{
				Manage(mcm.btns[n], False);
			}
			else
			{
				/* The button names are expected to be in the resource file */
				XmString label = XuVaGetXmStringResource("???", RNeditContextMenu,
									GV_active_field->editor->btns[n].contextBtnId);
				XtVaSetValues(mcm.btns[n], XmNlabelString, label, NULL);
				XmStringFree(label);
				Manage(mcm.btns[n], True);
			}
		}
	}
	manage_context_menu_appearance();
}


/* Callback for the buttons created below. Selection is done by activating
 * the appropriate entry in the field selection combo box.
 */
static Boolean field_select_wp( XtPointer data )
{
	SetActiveFieldByIndex(PTR2INT(data));
	return True;
}

/*ARGSUSED*/
static void context_menu_field_select_cb( Widget w, XtPointer client_data, XtPointer unused)
{
	(void) XtAppAddWorkProc(GV_app_context, field_select_wp, client_data);
}


/* Configure (and create) a set of pushbuttons in the field pulldown menu of the map
 * context menu that reflects the choice of fields for the current group.
 */
void ConfigureMainContextMenuFieldButtons(void)
{
	int i;

	if(!mcm.fs.popup) return;
	if(!GV_active_group) return;

	XtUnmanageChildren(mcm.fs.btns, (Cardinal) mcm.fs.nbtns);

	if(GV_active_group->nfield > mcm.fs.nbtns)
	{
		mcm.fs.btns = MoreWidgetArray(mcm.fs.btns, GV_active_group->nfield);
		for( i = mcm.fs.nbtns; i < GV_active_group->nfield; i++)
		{
			mcm.fs.btns[i] = XmCreatePushButton(mcm.fs.popup, "fpb", NULL, 0);
			XtAddCallback(mcm.fs.btns[i], XmNactivateCallback, context_menu_field_select_cb, INT2PTR(i));
		}
		mcm.fs.nbtns = GV_active_group->nfield;
	}

	for( i = 0; i < GV_active_group->nfield; i++ )
	{
		XmString label = XmStringCreateLocalized(GV_active_group->field[i]->info->sh_label);
		XtVaSetValues(mcm.fs.btns[i], XmNlabelString, label, NULL);
		XmStringFree(label);
	}
	XtManageChildren(mcm.fs.btns, (Cardinal) GV_active_group->nfield);
}


/* Configure selected object actions context menu for a particular style and use.  The button
 * labels are changed to whatever is in info.  The label is expected to be a reference to an
 * entry in the resource file. The action function will be called when any of the menu buttons
 * is activated. The supplied callback function should take an integer that corresponds to the
 * order of the labels and thus should take whatever action that button should reference. Note
 * that only one instance of this context menu will exist.
 *
 * Note that the input copyBtnId is a special case. The button is managed and unmanaged by
 * ingred action requests. The copyState variable is needed as the timing of Ingred requests is
 * command dependent and the copy on command can be received before the action configure call.
 */
void ActivateSelectContextMenu(CONTEXT *info, void (*action_fcn)(CONTEXT*,CAL), CAL cal)
{
	int      i;
	Cardinal count = 0;

	if(!acm.popup) return;

	acm.active      = True;
	acm.ea.btnsFcn  = action_fcn;
	acm.ea.btnsCAL  = cal;
	/*
	 * 2008.09.23 BugFix: In certain circumstances (noticed especially in the divide
	 * operation) this configure function could be called while the menu was posted
	 * resulting in a redraw. Ingred would then send a new message resulting in a
	 * redraw, resulting in an ingred message, ... and on forever. To avoid this, if
	 * the input CONTEXT is the same as the context being currently displayed then
	 * the menu buttons are not updated.
	 */
	if(acm.ea.btnsInfo == info) return;
	acm.ea.btnsInfo = info;

	Manage(acm.accept, True); 
	Manage(acm.undo, True); 

	Manage(acm.ea.btnsSeparator, False);
	XtUnmanageChildren(acm.ea.btns, MAX_CONTEXT_ITEMS);

	/* No info is a legal input if we will only be going to use the cascade button
	 */
	if(!info) return;

	/* This is for the programmer in case they forget to increase the limit if the
	 * number of items increases. See contextMenu.h
	 */
	if(info->nitems > MAX_CONTEXT_ITEMS)
	{
		pr_error("ActivateSelectContextMenu",
				"The nitems value exceeds MAX_CONTEXT_ITEMS. Increase the limit.\n");
		info->nitems = MAX_CONTEXT_ITEMS;
	}

	/* 2009.02.26: The copyBtnId is made a special case */
	acm.ea.copy.btn = NullWidget;
	for( i = 0; i < info->nitems; i++ )
	{
		if(info->items[i].btn_id == undefinedBtn) continue;
		if(info->items[i].btn_id == copyBtnId)
			acm.ea.copy.btn = acm.ea.btns[i];
		XtVaSetValues(acm.ea.btns[i], XmNlabelString, acm.ea.btnLabels[info->items[i].btn_id], NULL);
		count++;
	}
	if(count > 0)
	{
		Manage(acm.ea.btnsSeparator, True);
		XtManageChildren(acm.ea.btns, count);
		Manage(acm.ea.copy.btn, acm.ea.copy.state);
	}
}


/* Configure the cascade item in the select context menu. The widget whose label will be used to
 * labe the cascade button and the popup that the cascade is to activate are the parameters. Note
 * that although configured separately, it is unmanaged by DeactivateSelectContextMenu().
 */
void ConfigureSelectContextMenuCascadeButtons( Widget labelWidget1, Widget popup1, Widget labelWidget2, Widget popup2)
{
	int      count = 0;
	XmString label;

	if(labelWidget1 && popup1)
	{
		count++;
		XtVaGetValues(labelWidget1, XmNlabelString, &label, NULL);
		XtVaSetValues(acm.ea.cascade[0],
			XmNsubMenuId, popup1,
			XmNlabelString, label,
			NULL);
		XtManageChild(acm.ea.cascade[0]);
	}
	if(labelWidget2 && popup2)
	{
		count++;
		XtVaGetValues(labelWidget2, XmNlabelString, &label, NULL);
		XtVaSetValues(acm.ea.cascade[1],
			XmNsubMenuId, popup2,
			XmNlabelString, label,
			NULL);
		XtManageChild(acm.ea.cascade[1]);
	}

	if(count > 0 && !XtIsManaged(acm.ea.btnsSeparator))
		XtManageChild(acm.ea.btnsSeparator);
}


void DeactivateSelectContextMenu(void)
{
	acm.active      = False;
	acm.ea.btnsFcn  = NULL;
	acm.ea.btnsInfo = NULL;
	acm.ea.btnsCAL  = NULL;
	acm.ea.copy.btn = NullWidget;

	if(acm.popup)
	{
		XtUnmanageChildren(acm.ea.btns, MAX_CONTEXT_ITEMS);
		XtUnmanageChildren(acm.ea.cascade, 2);
		XtUnmanageChild(acm.ea.btnsSeparator);
	}
}


/* Initialize the attribute pullright menus. This function resets the menu so that the attributes
 * can be added by the AddToContextMenuAttributes() function below. This can contain any attribute
 * information that needs to be dynamically displayed in a pullright. For example, if the current
 * field was fronts, then this would be used to configure the list of fronts (the line attribute)
 * and provide a label for the cascade button used to activate the popup.
 *
 * This function initializes the cascade buttons and lists for both the main window context menu and
 * the action context menu. Thus the supplied callback function will be called in both instances so
 * the calling prodedure will have to discriminate depending on what edit mode it is in (for instance,
 * discriminate between editing and modifying).
 *
 * parameters: cascade_label - the label to give to the cascade button that pops up the menu
 *             fcn           - the callback function for the buttons
 */
void InitContextMenuAttributes(String cascade_label, void (*fcn)(int))
{
	if(!mcm.popup) return;

	mcm.ca.callback = fcn;
	mcm.ca.count    = -1;
	mcm.ca.ncolumns = 0;

	ShowContextMenuAttributes(False);

	if(!blank(cascade_label))
	{
		XmString xms = XmStringCreateLocalized(cascade_label);
		XtVaSetValues(mcm.ca.cascade, XmNlabelString, xms, NULL);
		XtVaSetValues(acm.ca.cascade, XmNlabelString, xms, NULL);
		XmStringFree(xms);
		mcm.ca.count = 0;
	}
	XtUnmanageChildren(mcm.ca.btns, (Cardinal) mcm.ca.nbtns);
	XtUnmanageChildren(acm.ca.btns, (Cardinal) mcm.ca.nbtns);
}


/* Add the given label (button) to the attribute list in the context menu.
 * The callback for the button is the order number in the array. It is the
 * function specified to the above function.
 */
void AddToContextMenuAttributes(String label)
{
	XmString xms;

	if(mcm.ca.count < 0 || !mcm.popup || !acm.popup) return;

	if(mcm.ca.ncolumns != (mcm.ca.count/mcm.ca.max_per_col)+1)
	{
		mcm.ca.ncolumns = (mcm.ca.count/mcm.ca.max_per_col)+1;

		XtVaSetValues(mcm.ca.popup,
			XmNpacking, XmPACK_COLUMN,
			XmNnumColumns, mcm.ca.ncolumns,
			NULL);

		XtVaSetValues(acm.ca.popup,
			XmNpacking, XmPACK_COLUMN,
			XmNnumColumns, mcm.ca.ncolumns,
			NULL);
	}

	if(mcm.ca.count >= mcm.ca.nbtns)
	{
		mcm.ca.nbtns++;
		mcm.ca.btns = MoreWidgetArray(mcm.ca.btns, mcm.ca.nbtns);
		acm.ca.btns = MoreWidgetArray(acm.ca.btns, mcm.ca.nbtns);

		mcm.ca.btns[mcm.ca.count] = XmVaCreatePushButton(mcm.ca.popup, "genericBtn", NULL);
		XtAddCallback(mcm.ca.btns[mcm.ca.count], XmNactivateCallback, pullright_cb, INT2PTR(mcm.ca.count));

		acm.ca.btns[mcm.ca.count] = XmVaCreatePushButton(acm.ca.popup, "genericBtn", NULL);
		XtAddCallback(acm.ca.btns[mcm.ca.count], XmNactivateCallback, pullright_cb, INT2PTR(mcm.ca.count));
	}

	xms = XmStringCreateLocalized(label);
	XtVaSetValues(mcm.ca.btns[mcm.ca.count], XmNlabelString, xms, NULL);
	XtVaSetValues(acm.ca.btns[mcm.ca.count], XmNlabelString, xms, NULL);
	XmStringFree(xms);

	Manage(mcm.ca.btns[mcm.ca.count], True);
	Manage(acm.ca.btns[mcm.ca.count], True);

	mcm.ca.count++;
}


/* It turned out to be more convienient to have this one function with the
 * show parameter than have separate show and hide functions. This makes it
 * easy to program the logic to make the show depend on some condition.
 */
void ShowContextMenuAttributes(Boolean show)
{
	if(show && mcm.ca.count > 0)
	{
		if(!mcm.ca.active)
		{
			mcm.ca.active = True;
			Manage(mcm.ca.cascade, True);
			Manage(mcm.ca.separator, True);
			Manage(acm.ca.cascade, True);
			Manage(acm.ca.separator, True);
		}
	}
	else
	{
		if(mcm.ca.active)
		{
			mcm.ca.active = False;
			Manage(mcm.ca.cascade, False);
			Manage(mcm.ca.separator, False);
			Manage(acm.ca.cascade, False);
			Manage(acm.ca.separator, False);
		}
	}
}


/* There is a generic push button that can be labeled and managed (normally by the field
 * specific panels). The button label is set by the buttonName string which must be an
 * entry in the resource file as *.buttonName.labelString: label. The function given in
 * the parameter list is called when the button is pressed and will be passed the value
 * of the info parameter. What exactly this button does is up to the code which uses the
 * button. The button is unmanaged and the function cleared by the release function
 * below. Note that the separator is not unmanaged on release if any one of the cascade
 * buttons is managed.
 */
void ConfigureMainContextMenuAuxPushButton(String buttonName, void (*fcn)(XtPointer), XtPointer info)
{
	Boolean show = (buttonName != NULL && fcn != NULL);
	if(show)
	{
		XmString label = XuVaGetXmStringResource("???", RNeditContextMenu, buttonName);
		mcm.aux.pushBtnFcn = fcn;
		mcm.aux.pushBtnFcnInfo = info;
		XtVaSetValues(mcm.aux.pushBtn, XmNlabelString, label, NULL);
		XmStringFree(label);
	}
	else
	{
		mcm.aux.pushBtnFcn = NULL;
		mcm.aux.pushBtnFcnInfo = NULL;
	}
	Manage(mcm.aux.pushBtn, show);
	manage_context_menu_appearance();
}


void ReleaseMainContextMenuAuxPushButton(void)
{
	ConfigureMainContextMenuAuxPushButton(NULL,NULL,NULL);
}


/* Create a pulldown menu that is a child of the mcm.popup popup menu widget.
 * This can then be used in the ConfigureMainContextMenuAuxCascade function.
 */
Widget CreateMainContextMenuAuxCascadePulldown(String id)
{
	if (!mcm.popup) return NullWidget;
	if (blank(id)) id = "mcmapp";
	return XmCreatePulldownMenu(mcm.popup, id, NULL, 0);
}


/* There are two cascade buttons followed by a separator that reside at the very top
 * of the main context popup menu. These can be used to display sub-panel specific info.
 * As such the sub-panel will be responsible for creating the pullright menu and setting
 * the labels of the cascade buttons. This function will show the context buttons and
 * assign the given labels and popup menus. The label for the cascade button pw is taken
 * from the widget lw. If lw or pw are null then the cascade button will not be managed.
 * This is useful if only one of the buttons need to be used.
 */
void ConfigureMainContextMenuAuxCascade( Widget lw1, Widget pw1, Widget lw2, Widget pw2)
{
	XmString label;

	if(lw1 && pw1)
	{
		XtVaGetValues(lw1, XmNlabelString, &label, NULL);
		XtVaSetValues(mcm.aux.cascade[0],
			XmNsubMenuId, pw1,
			XmNlabelString, label,
			NULL);
		XtManageChild(mcm.aux.cascade[0]);
	}
	else
	{
		XtUnmanageChild(mcm.aux.cascade[0]);
	}

	if(lw2 && pw2)
	{
		XtVaGetValues(lw2, XmNlabelString, &label, NULL);
		XtVaSetValues(mcm.aux.cascade[1],
			XmNsubMenuId, pw2,
			XmNlabelString, label,
			NULL);
		Manage(mcm.aux.cascade[1], True);
	}
	else
	{
		XtUnmanageChild(mcm.aux.cascade[1]);
	}
	
	manage_context_menu_appearance();
}


void ReleaseMainContextMenuAuxCascade(void)
{
	ConfigureMainContextMenuAuxCascade(NULL,NULL,NULL,NULL);
}


/* There are context buttons that parallels the copy-paste buttons in the copy-paste
 * panel. This function controls the visibility of these. Note that this panel does
 * not contain a cut button. This is supplied by the field panel function.
 */
void SetContextPasteSelectAllButtonsState(int btns_to_display )
{
	switch (btns_to_display)
	{
		case ALL:
			Manage(mcm.paste, True);
			Manage(mcm.selectAll, True);
			Manage(acm.selectAll, True);
			break;

		case SELECT_ALL_BUTTON_ONLY:
			Manage(mcm.paste, False);
			Manage(mcm.selectAll, True);
			Manage(acm.selectAll, True);
			break;

		case SELECT_ALL_BUTTON_ON:
			Manage(mcm.selectAll, True);
			Manage(acm.selectAll, True);
			break;

		case SELECT_ALL_BUTTON_OFF:
			Manage(mcm.selectAll, False);
			Manage(acm.selectAll, False);
			break;

		case SHOW_COPY_PASTE_BUTTONS_ONLY:
			Manage(mcm.paste, True);
			Manage(mcm.selectAll, False);
			Manage(acm.selectAll, False);
			break;

		default:
			Manage(mcm.paste, False);
			Manage(mcm.selectAll, False);
			Manage(acm.selectAll, False);
			break;
	}
	manage_context_menu_appearance();
}


/*============================= LOCAL FUNCTIONS ============================*/

/* In many of the functions below the callback for a button does not directly
 * do an action but calls a work procedure to carry out the work instead. This
 * is done to ensure that the context menu pops down quickly in case the action
 * takes a while. This improves the feel to the end user.
 */

/* This callback determines which menu to post on the map window. The actual menus
 * are changed by the panels and field functions.
 */
/*ARGSUSED*/
static void map_window_popup_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmPopupHandlerCallbackStruct *phcs = (XmPopupHandlerCallbackStruct *) call_data;
	XButtonPressedEvent *be = (XButtonPressedEvent *) phcs->event;

	/* Set no action default */
	phcs->menuToPost = mcm.popup;
	phcs->postIt     = False;

	/* This is to stop the popup from responding to key press events. */
	if(be->type != ButtonPress) return;

	/* The simple context menu takes priority over the active menu as
	 * both can be active at the same time.
	 */
	if(scm.active)
	{
		phcs->menuToPost = scm.popup;
		phcs->postIt     = True;
	}
	else if(acm.active)
	{
		phcs->menuToPost = acm.popup;
		phcs->postIt     = True;
	}
	else if(mcm.active)
	{
		phcs->menuToPost = mcm.assigned;
		phcs->postIt     = True;
	}
}


/* Callback function for the context menu attribute cascade buttons
 */
static Boolean pullright_wp( XtPointer data )
{
	if(mcm.ca.callback) mcm.ca.callback(PTR2INT(data));
	return True;
}

/*ARGSUSED*/
static void pullright_cb(Widget w, XtPointer data, XtPointer unused)
{
	if (mcm.ca.callback)
		(void) XtAppAddWorkProc(GV_app_context, pullright_wp, data);
}


/* Respond to edit status information from ingred that relates to
 * context menus only.
 */
/*ARGSUSED*/
static void edit_status_from_ingred_observer (CAL cal, String *parms, int nparms)
{
	/* All of the actions to be taken by the menus are related to editing */
	if(!same_ic(parms[0],E_EDIT)) return;

	/* If a popup is managed then it is visible and any more Ingred input
	 * is ignored. This overcomes a problem with Ingred sending extra status
	 * commands. This usually is due to the map window being obscured.
	 */
	if(XtIsManaged(mcm.popup)) return;
	if(XtIsManaged(acm.popup)) return;
	if(XtIsManaged(scm.popup)) return;

	if(same_ic(parms[1],E_DRAWING))
	{
		/* Make the point-by-point drawing termination menu active or not */
		scm.active = same(parms[2],E_ON);
		Manage(scm.done, scm.active);
		Manage(scm.cancel, scm.active);
	}
	else if(same_ic(parms[1],E_MODIFYING))
	{
		/* Make the modifying termination menu active or not */
		scm.active = same(parms[2],E_ON);
		Manage(scm.confirm, scm.active);
		Manage(scm.accept, scm.active);
		Manage(scm.cancel, scm.active);
		Manage(scm.undo, scm.active);
	}
	else if(same_ic(parms[1], E_OUTLINE))
	{
		/* The following outlines now exist and the associated buttons
		 * in the popup menus can be set sensitive.
		 */
		if(same_ic(parms[2],"MOVED"))
			lastMovedExists = True;
		else if(same_ic(parms[2],E_STOMP))
			lastStompExists = True;
		else if(same_ic(parms[2],"DRAWN"))
			lastDrawnExists = True;
		else if(same_ic(parms[2],E_HOLE))
			lastHoleExists = True;

		set_outline_selection_sensitivity();
	}
	else if(same_ic(parms[1],E_BUTTON))
	{
		String cmd = parms[2];
		Boolean on = same_ic(parms[3],E_ON);

		if(same_ic(cmd, E_UPDATE))
		{
			SENSITIVE(acm.accept, on);
			SENSITIVE(scm.accept, on);
			Manage(mcm.accept, on);
		}
		else if(same_ic(cmd, E_CANCEL))
		{
			SENSITIVE(acm.cancel, on);
			Manage(mcm.cancel, on);
		}
		else if(same_ic(cmd, E_UNDO))
		{
			SENSITIVE(acm.undo, on);
			SENSITIVE(scm.undo, on);
			Manage(mcm.undo, on);
		}
		else if(same_ic(cmd, E_CLEAR))
		{
			Manage(mcm.clear, on);
		}
		else if(same_ic(cmd, E_COPY))
		{
			acm.ea.copy.state = on;
			Manage(acm.ea.copy.btn, on);
			/* Delete along with copy only makes sense for non-continuous fields */
			if (on) on = !(IsActiveEditor(CONTINUOUS_FIELD_EDITOR) || IsActiveEditor(VECTOR_FIELD_EDITOR)); 
			Manage(mcm.delete, on);
		}
		else if(same_ic(cmd, E_PASTE))
		{
			SENSITIVE(mcm.paste, on);
		}
		else if(same_ic(cmd, E_SELECT_ALL))
		{
			Manage(mcm.selectAll, on);
			Manage(acm.selectAll, on);
		}
		else if(same_ic(cmd, E_CREATE))
		{
			Manage(mcm.create, on);
			Manage(mcm.createSeparator, on);
		}
		else if(same_ic(cmd, E_BREAK))
		{
			Manage(mcm.lineBreak, on);
		}
		else if(same_ic(cmd, E_JOIN))
		{
			Manage(mcm.join, on);
		}
		else if(same_ic(cmd, E_REJOIN))
		{
			Manage(mcm.rejoin, on);
		}
		else if(same_ic(cmd, E_PROCEED))
		{
			Manage(mcm.proceed, on);
		}
		else if(same_ic(cmd, E_PRESET_OUTLINE))
		{
			mcm.outline.show = on;
			configure_outline_selection_menu();
		}
	}
	manage_context_menu_appearance();
}


/*  Take any action that is required if the edit function is about to change.
 */
/*ARGSUSED*/
static void edit_function_change_observer(String *parms, int nparms)
{
	/* The menu function for showing outline selections is not properly
	 * cancelled by Ingred when exiting the MOVE edit operator before
	 * actually choosing something to move. This turned out to be really
	 * hard to do in Ingred, so the menu is cancelled explicitly here when
	 * any editor button is selected.
	 */
	mcm.outline.show = False;
	configure_outline_selection_menu();
}


static void zoom_mode_status( String *parms, int nparms )
{
	if(same(parms[0],E_ZOOM))
	{
		scm.active = (nparms > 1 && same(parms[1],E_ON));
		Manage(scm.zoomCancel, scm.active);
	}
	else if(same(parms[0],E_PAN_MODE))
	{
		scm.active = (nparms > 1 && same(parms[1],E_ON));
		Manage(scm.panEndBtn, scm.active);
	}
}



/* Callback for the command buttons in the context menus. Some of the commands
 * require special processing beyond just passing the command to ingred.
 */
static Boolean commands_wp(XtPointer client_data)
{
	String cmd = (String)client_data;

	if(GV_edit_mode)
	{
		/* The special undo processing only applies while in edit mode */
		if(same(cmd,E_UNDO) && PanelIsActive(ELEMENT_EDIT))
		{
			int nedit;
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
		else if(same(cmd,E_SELECT_ALL))
		{
			if(GV_active_field && GV_active_field->editor && GV_active_field->editor->active)
			{
				IngredVaEditCommand(GV_active_field->cal, NullCal, "%s %s %s",
					E_EDIT, GV_active_field->editor->active->cmd, cmd);
			}
		}
		else if(same(cmd,CANCEL_ZOOM))
		{
			scm.active = False;
			Manage(scm.zoomCancel, False);
			ZoomCommand(ZOOM_CANCEL);
		}
		else if(same(cmd,PAN_DONE))
		{
			scm.active = False;
			Manage(scm.panEndBtn, False);
			ZoomCommand(ZOOM_PAN_EXIT);
		}
		else if(same(cmd,E_CREATE))
		{
			String notify[1] = {NULL};
			DeactivateMenu();
			(void) IngredVaCommand(GE_SEQUENCE, "CREATE_FIELD %s %s %s",
					GV_active_field->info->element->name,
					GV_active_field->info->level->name,
					ActiveDepictionTime(FIELD_DEPENDENT),
					NULL);
			ActivateMenu();
			NotifyObservers(OB_FIELD_AVAILABLE, notify, 1);
		}
		else
		{
			(void) IngredCommand(GE_EDIT, cmd);
		}
	}
	else
	{
		if(same(cmd,CANCEL_ZOOM))
		{
			scm.active = False;
			Manage(scm.zoomCancel, False);
			ZoomCommand(ZOOM_CANCEL);
		}
		else if(same(cmd,PAN_DONE))
		{
			scm.active = False;
			Manage(scm.panEndBtn, False);
			ZoomCommand(ZOOM_PAN_EXIT);
		}
		else if(same(cmd,E_CLEAR))
		{
			(void) IngredCommand(GE_EDIT, cmd);
		}
	}
	return True;
}


/* ARGSUSED */
static void commands_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	(void) XtAppAddWorkProc(GV_app_context, commands_wp, client_data);
}


/* Respond to field selection buttons. */
static Boolean context_field_select_wp(XtPointer client_data)
{
	XmToggleButtonSetState(GV_active_field->editor->btns[PTR2INT(client_data)].w, True, True);
	return True;
}

/*ARGSUSED*/
static void context_field_select_cb(Widget w, XtPointer client_data, XtPointer unused)
{
	if(GV_active_field)
		(void) XtAppAddWorkProc(GV_app_context, context_field_select_wp, client_data);
}


/* Respond to the auxillary push buttons
 */
/*ARGSUSED*/
static Boolean aux_push_button_wp(XtPointer data)
{
	mcm.aux.pushBtnFcn(mcm.aux.pushBtnFcnInfo);
	return True;
}

/*ARGSUSED*/
static void aux_push_button_cb(Widget w, XtPointer client_data, XtPointer unused)
{
	if(mcm.aux.pushBtnFcn)
	{
		(void) XtAppAddWorkProc(GV_app_context, aux_push_button_wp, client_data);
	}
}


/* Callback function for the buttons in the outlines cascade menu. Any index
 * number < 0 is for the predefined buttons and any 0 or above are for the
 * values obtained from the setup file.
 */
/*ARGSUSED*/
static void outline_cb(Widget w, XtPointer client_data, XtPointer unused)
{
	int ndx = PTR2INT(client_data);
	String editor = GV_active_field->editor->active->cmd;

	if (mcm.outline.data[ndx]) {
		(void) IngredVaCommand(GE_DEPICTION, "EDIT %s PRESET_OUTLINE %s", editor, mcm.outline.data[ndx]);
	} else {
		(void) IngredVaCommand(GE_DEPICTION, "EDIT %s DRAW_OUTLINE", editor);
	}
}


/* The preset outline "button" is a cascade on the main context menu and on the 
 * selection menu that provides several options for selecting outlines that are
 * used to define areas of selection for merge and move operations. 
 */
static void configure_outline_selection_menu(void)
{
	SETUP *setup;
	String  cascadeLabel, setupFileOutlines = NULL;

	Manage(mcm.outline.cascade, mcm.outline.show);
	Manage(acm.outline.cascade, mcm.outline.show);

	if(!mcm.outline.show) return;

	/* Note that in the following setupFileOutlines is the same for every field type. Things
	 * were coded this way so that in future this could be different for the various
	 * field types.
	 */
	if(IsActiveEditor(CONTINUOUS_FIELD_EDITOR) || IsActiveEditor(VECTOR_FIELD_EDITOR))
	{
		if(InEditMode(E_STOMP))
		{
			cascadeLabel = XuGetLabelResource("presetBoundary","Boundary From");
			XuWidgetLabel(mcm.outline.btn[0], XuGetLabelResource("lastStompOutlineBtn","Stomp Outline"));
			mcm.outline.data[0] = FpaEditorLastStompBoundary;
			XtUnmanageChild(mcm.outline.btn[1]);
			setupFileOutlines = MAP_EDITOR;
		}
		else
		{
			cascadeLabel = XuGetLabelResource("presetBoundary","Boundary From");
			XuWidgetLabel(mcm.outline.btn[0], XuGetLabelResource("lastDrawnAreaBtn","Last Drawn Outline"));
			mcm.outline.data[0] = FpaEditorLastDrawnBoundary;
			XtManageChild(mcm.outline.btn[1]);
			XuWidgetLabel(mcm.outline.btn[1], XuGetLabelResource("lastMovedAreaBtn","Last Moved Outline"));
			mcm.outline.data[1] = FpaEditorLastMovedBoundary;
			setupFileOutlines = MAP_EDITOR;
		}
	}
	else
	{
		if(InEditMode(E_DRAW_HOLE))
		{
			cascadeLabel = XuGetLabelResource("holesFrom","Holes From");
			XuWidgetLabel(mcm.outline.btn[0], XuGetLabelResource("lastDrawnHole","Last Drawn Hole"));
			mcm.outline.data[0] = FpaEditorLastDrawnHole;
			XtUnmanageChild(mcm.outline.btn[1]);
			setupFileOutlines = MAP_HOLES;
		}
		else
		{
			cascadeLabel = XuGetLabelResource("presetSelect","Select By");
			XuWidgetLabel(mcm.outline.btn[0], XuGetLabelResource("drawOutlineBtn","Draw Outline"));
			mcm.outline.data[0] = NULL;
			XtManageChild(mcm.outline.btn[1]);
			XuWidgetLabel(mcm.outline.btn[1], XuGetLabelResource("lastDrawnAreaBtn","Last Drawn Outline"));
			mcm.outline.data[1] = FpaEditorLastDrawnBoundary;
			setupFileOutlines = MAP_EDITOR;
		}
	}

	XuWidgetLabel(mcm.outline.cascade, cascadeLabel);
	XuWidgetLabel(acm.outline.cascade, cascadeLabel);

	/* Create, manage and label the predefined outlines as specified in the setup file.
	 * These buttons come after the two manditory buttons in the list which have already
	 * been defined. Note that if setupFileOutlines is not valid or NULL then GetSetup
	 * will return NULL.
	 */
	setup = GetSetup(setupFileOutlines);
	if (setup)
	{
		int n;
		if(mcm.outline.nbtns < setup->nentry)
		{
			mcm.outline.btn  = MoreWidgetArray(mcm.outline.btn, setup->nentry+2);
			mcm.outline.data = MoreStringArray(mcm.outline.data, setup->nentry+2);
			for(n = mcm.outline.nbtns+2; n < setup->nentry+2; n++)
			{
				mcm.outline.btn[n] = XmCreatePushButton(mcm.outline.popup, "setBtn", NULL, 0);
				XtAddCallback(mcm.outline.btn[n], XmNactivateCallback, outline_cb, INT2PTR(n));
			}
			mcm.outline.nbtns = setup->nentry;
		}
		XtUnmanageChildren(mcm.outline.btn+2, mcm.outline.nbtns);
		for(n = 0; n < setup->nentry; n++)
		{
			XuWidgetLabel(mcm.outline.btn[n+2], SetupParm(setup,n,0));
			mcm.outline.data[n+2] = SetupParm(setup,n,1);
		}
		XtManageChildren(mcm.outline.btn+2, setup->nentry);
	}
	else if(mcm.outline.nbtns > 0)
	{
		XtUnmanageChildren(mcm.outline.btn+2, mcm.outline.nbtns);
	}

	set_outline_selection_sensitivity();
}


/* Change the outline selection button sensitivity depending on if a
 * previous outline exists or not. Ingred tells us this.
 */
static void set_outline_selection_sensitivity(void)
{
	if(IsActiveEditor(CONTINUOUS_FIELD_EDITOR) || IsActiveEditor(VECTOR_FIELD_EDITOR))
	{
		if(InEditMode(E_STOMP))
		{
			XtSetSensitive(mcm.outline.btn[0], lastStompExists);
			XtSetSensitive(mcm.outline.btn[1], False);
		}
		else
		{
			XtSetSensitive(mcm.outline.btn[0], lastDrawnExists);
			XtSetSensitive(mcm.outline.btn[1], lastMovedExists);
		}
	}
	else
	{
		if(InEditMode(E_DRAW_HOLE))
		{
			XtSetSensitive(mcm.outline.btn[0], lastHoleExists);
			XtSetSensitive(mcm.outline.btn[1], False);
		}
		else
		{
			XtSetSensitive(mcm.outline.btn[0], True);
			XtSetSensitive(mcm.outline.btn[1], lastDrawnExists);
		}
	}
}


/*   Create the main context menu that will appear whenever the right mouse button
 *   is selected on the map window. Note that the actual menu is created in main.c
 *   right after the mapWindow as some other functions need it in order to interact
 *   with the menu (add pullrights). The order that the widgets are created in this
 *   function is important! Also note that the menus required for outline selection
 *   are created at the bottom of this function as they are part of the main menu.
 */
static void create_main_context_menu(void)
{
	int n;
	SETUP  *setup;
	Widget btn;

	mcm.popup = CreatePanelContextMenu("editContextMenu");

	/* Get maximum number of items per attribute pullright menu from resource file */
	mcm.ca.max_per_col = XuGetIntResource(RNmaxContextMenuColumnLength, 15);

	mcm.create = XmCreatePushButton(mcm.popup, "createBtn", NULL, 0);
	XtAddCallback(mcm.create, XmNactivateCallback, commands_cb, (XtPointer) E_CREATE);
		
	mcm.createSeparator = XmCreateSeparator(mcm.popup, "sep", NULL, 0);
	
	/* Create the outline selector. The function and labels of these are changed
	 * depending on the active field type. See configure_outline_selection_menu.
	 */
	mcm.outline.popup = XmCreatePulldownMenu(mcm.popup, "outlineSelect", NULL, 0);

	/* The first two selection buttons always exist and any found in the setup
	 * file are added later.
	 */
	mcm.outline.btn  = NewWidgetArray(2);
	mcm.outline.data = NewStringArray(2);
	for(n = 0; n < 2; n++)
	{
		mcm.outline.btn[n] = XmVaCreateManagedPushButton(mcm.outline.popup, "btn1", NULL);
		XtAddCallback(mcm.outline.btn[n], XmNactivateCallback, outline_cb, INT2PTR(n));
	}

	mcm.outline.cascade = XmVaCreateCascadeButton(mcm.popup, "cascade",
		XmNsubMenuId, mcm.outline.popup,
		NULL);

	mcm.outline.separator = XmCreateSeparator(mcm.popup, "sep", NULL, 0);

	/* Create a generic push button and cascade buttons for auxillary pullright popups
	 * that can be used by the field specific functions to put up actions when required.
	 * Note that no pulldown menu is specified with the cascade buttons here as it must
	 * be supplied by the function that uses these.
	 */
	mcm.aux.pushBtn = XmCreatePushButton(mcm.popup, "apb", NULL, 0);
	XtAddCallback(mcm.aux.pushBtn, XmNactivateCallback, aux_push_button_cb, NULL);
	
	mcm.aux.cascade[0] = XmCreateCascadeButton(mcm.popup, "genericBtn", NULL, 0);
	mcm.aux.cascade[1] = XmCreateCascadeButton(mcm.popup, "genericBtn", NULL, 0);
	mcm.aux.separator  = XmCreateSeparator(mcm.popup, "sep", NULL, 0);

	/* Popup for selecting the object attributes. This information must also be filled
	 * in by the field specific functions.
	 */
	mcm.ca.popup = XmCreatePulldownMenu(mcm.popup, "cmpr", NULL, 0);

	mcm.ca.cascade = XmVaCreateCascadeButton(mcm.popup, "otc",
		XmNsubMenuId, mcm.ca.popup,
		NULL);
		
	mcm.ca.separator = XmCreateSeparator(mcm.popup, "sep", NULL, 0);

	/* Action push buttons. These are managed or not by responding to status information
	 * from Ingred.
	 */
	mcm.paste = XmVaCreatePushButton(mcm.popup, "pasteBtn",
		XmNsensitive, False,
		NULL);
	XtAddCallback(mcm.paste, XmNactivateCallback, commands_cb, (XtPointer) E_PASTE);

	mcm.selectAll = XmCreatePushButton(mcm.popup, "selectAllBtn", NULL, 0);
	XtAddCallback(mcm.selectAll, XmNactivateCallback, commands_cb, (XtPointer) E_SELECT_ALL);

	mcm.selectAllSeparator = XmCreateSeparator(mcm.popup, "sep", NULL, 0);

	mcm.proceed = XmCreatePushButton(mcm.popup, "proceedBtn", NULL, 0);
	XtAddCallback(mcm.proceed, XmNactivateCallback, commands_cb, (XtPointer) E_PROCEED);

	mcm.join = XmCreatePushButton(mcm.popup, "joinBtn", NULL, 0);
	XtAddCallback(mcm.join, XmNactivateCallback, commands_cb, (XtPointer) E_JOIN);

	mcm.rejoin = XmCreatePushButton(mcm.popup, "rejoinBtn", NULL, 0);
	XtAddCallback(mcm.rejoin, XmNactivateCallback, commands_cb, (XtPointer) E_REJOIN);

	mcm.lineBreak = XmCreatePushButton(mcm.popup, "breakBtn", NULL, 0);
	XtAddCallback(mcm.lineBreak, XmNactivateCallback, commands_cb, (XtPointer) E_BREAK);

	mcm.delete = XmCreatePushButton(mcm.popup, "deleteBtn", NULL, 0);
	XtAddCallback(mcm.delete, XmNactivateCallback, commands_cb, (XtPointer) E_DELETE);

	mcm.accept = XmCreatePushButton(mcm.popup, "acceptBtn", NULL, 0);
	XtAddCallback(mcm.accept, XmNactivateCallback, commands_cb, (XtPointer) E_UPDATE);

	mcm.cancel = XmCreatePushButton(mcm.popup, "cancelBtn", NULL, 0);
	XtAddCallback(mcm.cancel, XmNactivateCallback, commands_cb, (XtPointer) E_CANCEL);

	mcm.undo = XmCreatePushButton(mcm.popup, "undoBtn", NULL, 0);
	XtAddCallback(mcm.undo, XmNactivateCallback, commands_cb, (XtPointer) E_UNDO);

	mcm.clear = XmCreatePushButton(mcm.popup, "clearBtn", NULL, 0);
	XtAddCallback(mcm.clear, XmNactivateCallback, commands_cb, (XtPointer) E_CLEAR);

	mcm.cmdBtnsSeparator = XmCreateSeparator(mcm.popup, "sep", NULL, 0);

	/* The list of edit operations for the currently active field type.
	 */
	for(n = 0; n < NR_EDIT_BTNS; n++)
	{
		mcm.btns[n] = XmCreatePushButton(mcm.popup, "genericBtn", NULL, 0);
		XtAddCallback(mcm.btns[n], XmNactivateCallback, context_field_select_cb, INT2PTR(n));
	}

	(void) XmVaCreateManagedSeparator(mcm.popup, "sep", NULL);

	/* We need a popup menu for the active field selection cascade button.
	 */
	mcm.fs.popup = XmCreatePulldownMenu(mcm.popup, "fieldSelectPopup", NULL, 0);

	mcm.fs.cascade = XmVaCreateManagedCascadeButton(mcm.popup, "fieldLabel",
		XmNsubMenuId, mcm.fs.popup,
		NULL);
}


/* This callback simply calls the current action function with the order number of
 * the activated push button.
 */
static Boolean action_context_select_wp(XtPointer client_data)
{
	acm.ea.btnsInfo->selected_item = acm.ea.btnsInfo->items + PTR2INT(client_data);
	acm.ea.btnsFcn(acm.ea.btnsInfo, acm.ea.btnsCAL);
	return True;
}

/*ARGSUSED*/
static void action_context_select_cb(Widget w, XtPointer client_data, XtPointer unused)
{
	if (acm.ea.btnsFcn && acm.ea.btnsInfo)
	{
		(void) XtAppAddWorkProc(GV_app_context, action_context_select_wp, client_data);
	}
}


/* Create the menu that will be configured to respond to requests from ingred for actions
 * on selected objects. See the ActivateSelectContextMenu() function for details. The
 * order of widget creation is important.
 */
static void create_action_context_menu(void)
{
	int n;
	String labels[] = {CONTEXT_BUTTON_LABELS};

	if (!GV_edit_mode) return;

	/* Create the set of XmString labels that are associated with the various button id's */
	acm.ea.btnLabels = NewMem(XmString, XtNumber(labels));
	for(n = 0; n < XtNumber(labels); n++)
		acm.ea.btnLabels[n] = XuVaGetXmStringResource("???", RNeditContextMenu, labels[n]);

	/* Create the actual popup widget */
	acm.popup = CreatePanelContextMenu("actionContextMenu");

	/* The edit action buttons that are set by the active sub-panel */
	for(n = 0; n < MAX_CONTEXT_ITEMS; n++)
	{
		char mbuf[16];
		(void) snprintf(mbuf, 16, "eaBtn%d", n);
		acm.ea.btns[n] = XmCreatePushButton(acm.popup, mbuf, NULL, 0);
		XtAddCallback(acm.ea.btns[n], XmNactivateCallback, action_context_select_cb, INT2PTR(n));
	}

	acm.ea.cascade[0] = XmCreateCascadeButton(acm.popup, "prm0", NULL, 0);
	acm.ea.cascade[1] = XmCreateCascadeButton(acm.popup, "prm1", NULL, 0);
	acm.ea.btnsSeparator =  XmCreateSeparator(acm.popup, "sep", NULL, 0);

	/* Cascade buttons for the outline selection. These mirror the onec in the mcm menu
	 * and are controlled at the same time and use the same popup menu.
	 */
	acm.outline.cascade = XmVaCreateCascadeButton(acm.popup, "outlineCascade",
		XmNsubMenuId, mcm.outline.popup,
		NULL);

	acm.outline.separator = XmCreateSeparator(acm.popup, "sep", NULL, 0);

	acm.selectAll = XmCreatePushButton(acm.popup, "selectAllBtn", NULL, 0);
	XtAddCallback(acm.selectAll, XmNactivateCallback, commands_cb, (XtPointer) E_SELECT_ALL);

	acm.selectAllSeparator = XmCreateSeparator(acm.popup, "sep", NULL, 0);

	/* Popup for selecting the object attributes to be applied */
	acm.ca.popup = XmCreatePulldownMenu(acm.popup, "cmpr", NULL, 0);

	acm.ca.cascade = XmVaCreateCascadeButton(acm.popup, "genericBtn",
		XmNsubMenuId, acm.ca.popup,
		NULL);

	acm.ca.separator = XmCreateSeparator(acm.popup, "sep", NULL, 0);

	/* The action buttons whose existance is controlled by Ingred
	 */
	acm.accept = XmVaCreateManagedPushButton(acm.popup, "acceptBtn", XmNsensitive, False, NULL);
	XtAddCallback(acm.accept, XmNactivateCallback, commands_cb, (XtPointer) E_UPDATE);

	acm.cancel = XmVaCreateManagedPushButton(acm.popup, "cancelBtn", XmNsensitive, False, NULL);
	XtAddCallback(acm.cancel, XmNactivateCallback, commands_cb, (XtPointer) E_CANCEL);

	acm.undo = XmVaCreateManagedPushButton(acm.popup, "undoBtn", XmNsensitive, False, NULL);
	XtAddCallback(acm.undo, XmNactivateCallback, commands_cb, (XtPointer) E_UNDO);
}


/* Sets some of the visual states in the context menus, mostly some of the separators that
 * are used under some of the buttons. Some special rules for what buttons are allowed to
 * appear together are also applied here.
 */
static void manage_context_menu_appearance(void)
{
	int     n;
	Boolean state;

	if (!mcm.popup) return;

	state = XtIsManaged(mcm.aux.pushBtn)    ||
		    XtIsManaged(mcm.aux.cascade[0]) ||
			XtIsManaged(mcm.aux.cascade[1]);

	Manage(mcm.aux.separator, state);

	/* Manage the main context menu visible state
	 */
	state = XtIsManaged(mcm.delete)    ||
			XtIsManaged(mcm.accept)    ||
			XtIsManaged(mcm.cancel)    ||
			XtIsManaged(mcm.undo)      ||
			XtIsManaged(mcm.clear)     ||
			XtIsManaged(mcm.join)      ||
			XtIsManaged(mcm.lineBreak) ||
			XtIsManaged(mcm.rejoin)    ||
			XtIsManaged(mcm.proceed);

	Manage(mcm.cmdBtnsSeparator, state);

	state =	XtIsManaged(mcm.paste)	||
			XtIsManaged(mcm.selectAll);

	/* I do not want the outline and select all buttons visually separated */
	Manage(mcm.selectAllSeparator, state);
	Manage(mcm.outline.separator, !state && mcm.outline.show);

	if(GV_edit_mode && GV_active_field)
	{
		state = XtIsSensitive(GV_active_field->editor->buttonManager);
		for(n = 0; n < NR_EDIT_BTNS; n++)
		{
			if(GV_active_field->editor->btns[n].contextBtnId)
				SENSITIVE(mcm.btns[n], state);
		}
	}

	/* Manage the selected object action context menu visible states.
	 * We do not want the cancel and undo buttons to be available at
	 * the same time as this confuses the user. It is easier to do
	 * this here rather than in Ingred or in direct response to Ingred
	 * commands
	 */
	state = (acm.selectAll)? XtIsManaged(acm.selectAll):False;
	Manage(acm.selectAllSeparator, state);
	Manage(acm.outline.separator, !state && mcm.outline.show);

	/* Manage the simple context menu button visible states. If confirm is
	 * active we do not want accept and if cancel is active we do not want
	 * undo as these can confuse the user.
	 */
	if(scm.confirm && scm.accept)
	{
		if( XtIsManaged(scm.confirm) && XtIsManaged(scm.accept) )
			XtUnmanageChild(scm.accept);
	}
	if(scm.cancel && scm.undo)
	{
		if( XtIsManaged(scm.cancel) && XtIsManaged(scm.undo) )
			XtUnmanageChild(scm.undo);
	}
}


/* The simple menu. The order of the button creation is important */
static void create_simple_context_menu(void)
{
	scm.popup = CreatePanelContextMenu("simpleContextMenu");

	scm.panEndBtn = XmCreatePushButton(scm.popup, "zoomPanEnd", NULL, 0);
	XtAddCallback(scm.panEndBtn, XmNactivateCallback, commands_cb, (XtPointer) PAN_DONE );

	scm.zoomCancel = XmCreatePushButton(scm.popup, "cancelBtn", NULL, 0);
	XtAddCallback(scm.zoomCancel, XmNactivateCallback, commands_cb, (XtPointer) CANCEL_ZOOM );

	scm.done = XmCreatePushButton(scm.popup, "doneBtn", NULL, 0);
	XtAddCallback(scm.done, XmNactivateCallback, commands_cb, (XtPointer) E_DRAW_DONE);

	scm.confirm = XmCreatePushButton(scm.popup, "confirmBtn", NULL, 0);
	XtAddCallback(scm.confirm, XmNactivateCallback, commands_cb, (XtPointer) E_MODIFY_CONFIRM);

	scm.accept = XmVaCreatePushButton(scm.popup, "acceptBtn", XmNsensitive, False, NULL);
	XtAddCallback(scm.accept, XmNactivateCallback, commands_cb, (XtPointer) E_UPDATE);

	scm.cancel = XmCreatePushButton(scm.popup, "cancelBtn", NULL, 0);
	XtAddCallback(scm.cancel, XmNactivateCallback, commands_cb, (XtPointer) E_CANCEL);

	scm.undo = XmVaCreatePushButton(scm.popup, "undoBtn", XmNsensitive, False, NULL);
	XtAddCallback(scm.undo, XmNactivateCallback, commands_cb, (XtPointer) E_UNDO);
}
