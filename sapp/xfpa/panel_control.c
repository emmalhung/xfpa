/****************************************************************************
*
*  File:     panel_control.c
*
*  Purpose:  Create and control the tab panels found to the right of the
*            map window that contain all of the editing functionality.
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
#include "depiction.h"
#include "editor.h"
#include "iconBar.h"
#include "menu.h"
#include "observer.h"
#include "timelink.h"
#include "imagery.h"

#include <Xm/Form.h>
#include <Xm/TabStack.h>

#undef CONNECT_DEVELOPMENT


typedef void (*DFCN)(void);

static Widget   active_panel_widget = NULL;
static PANEL_ID current_panel = ELEMENT_EDIT;
static Boolean  dialog_is_sampling = False;
static DFCN     dialog_rtn_fcn = NULL;
static String   pulldown_exclusion = "";
static Boolean  tab_select_allowed = False;


/* Name of the tab stack panel form widgets and the functions to create
 * the contents of the panels.
 */
static struct {
	String name;
	PANEL_ID id;
	Widget wid;
	void (*create_fcn)(Widget);
} panel_info[] = {
	{ "editorPanel",	 ELEMENT_EDIT, NULL, CreateEditControlPanel },
	{ "scratchpadPanel", SCRATCHPAD,   NULL, CreateScratchpadPanel  },
#ifdef CONNECT_DEVELOPMENT
	{ "connectPanel",    CONNECT,      NULL, CreateConnectPanel     },
#endif
	{ "timelinkPanel",   TIMELINK,     NULL, CreateTimelinkPanel    },
	{ "interpPanel",	 INTERPOLATE,  NULL, CreateInterpolatePanel },
	{ "animationPanel",  ANIMATION,    NULL, CreateAnimationPanel   }
};


/*
*	Set many of the elements in the main menu bar insensitive.
*/
static void set_control_sensitivity(Boolean state )
{
	int i;
	Widget btn;

	static int btnlist[] = {
		MENU_Status_depiction,
		MENU_Create_fields, MENU_Load_fields, MENU_Update_fields, MENU_Import_fields,
		MENU_Depiction_save, MENU_Depiction_saveAll,
		MENU_Depiction_delete,
		MENU_Depiction_print,
		MENU_Guidance_hide, MENU_Guidance_show,
		MENU_Image_show, MENU_Image_hide
	};

	static String items_pulldown[] = {
		"options", "products", "guidance", "imagery", "view", "actions", "help"
	};

	Manage(GW_tabFrame2, !state);

	for(i = 0; i < XtNumber(btnlist); i++)
	{
		btn = XuMenuFindButton(GW_menuBar, btnlist[i]);
		if(NotNull(btn) && XtIsWidget(btn) && XtIsRealized(btn))
			XtSetSensitive(btn, state);
	}

	/* All of the pulldown items are set insensitive with the exception of
	 * the exclusion. See sampling_notification for details.
	 */
	for(i = 0; i < XtNumber(items_pulldown); i++)
	{
		if(same(items_pulldown[i],pulldown_exclusion)) continue;
		btn = XtNameToWidget(GW_menuBar, items_pulldown[i]);
		if(NotNull(btn) && XtIsWidget(btn) && XtIsRealized(btn))
			XtSetSensitive(btn, state);
	}

	/* Set all of the icon bar buttons to the given state */
	SetIconBarButtonSensitivity(NULL, state);
}


/* Receive notification that a either the guidance or imagery dialogs is
 * starting or ending a sampling procedure. The pulldown associated with
 * the dialog must not be made insensitive. This is done through the use
 * of the pulldown_exclusion string which must be one of the entries in
 * the list of pulldowns as defined in set_control_sensitivity().
 */
static void sampling_notification( String *parms, int nparms )
{
	if(nparms < 2) return;

	/* As an "on" command can be followed by another "on" and not
	 * necessarly an "off" set the controls all on.
	 */
	set_control_sensitivity(ON);

	/* Note that the order of the calls below is important */
	if(same_ic(parms[1],"on"))
	{
		if(same(parms[0],OB_KEY_GUIDANCE)) pulldown_exclusion = "guidance";
		if(same(parms[0],OB_KEY_IMAGERY) ) pulldown_exclusion = "imagery";
		if(nparms >= 3) dialog_rtn_fcn = (DFCN) parms[2];
		DeactivateMenu();
		dialog_is_sampling = True;
		set_control_sensitivity(OFF);
	}
	else
	{
		dialog_rtn_fcn = NULL;
		pulldown_exclusion = "";
		dialog_is_sampling = False;
		ActivateMenu();
	}
}


/* Once the main has finished initializing everything this will be called.*/
/*ARGSUSED*/
static void initialization_notification( String *parms, int nparms )
{
	tab_select_allowed = True;
}


/* Change the tab panels */
static Boolean switch_panel(PANEL_ID panel )
{
	int i;
	Widget btn;

	static int timelink_btn_list[] = {
		MENU_Create_fields, MENU_Load_fields, MENU_Update_fields, MENU_Import_fields,
		MENU_Depiction_save, MENU_Depiction_saveAll, MENU_Depiction_delete,
		MENU_Products_regularText, MENU_Products_graphics, MENU_Products_pointFcst,
		MENU_Products_alliedModels
	};
	static String timelink_icon_list[] = {
		LOAD_FIELDS_ICON, UPDATE_FIELDS_ICON, IMPORT_FIELDS_ICON, DELETE_FIELDS_ICON,
		GRAPHIC_PRODUCTION_ICON, ALLIED_MODELS_ICON
	};

	/* Set the cursor back to the default. If any other style is required then the
	 * panel startup function must take care of this.
	 */
	XuSetCursor(GW_mapWindow, XuDEFAULT_CURSOR, ON);

	if(!active_panel_widget) active_panel_widget = EDITOR_PANEL;

	ShowMessages(False);
	/* 
	 * 20070508: Just in case someone issued a zoom command but did not actually
	 *           do anything cancel the zoom. No harm if this is not the case.
	 * 20090109: If in pan mode exit before proceeding.
	 */
	ZoomCommand(ZOOM_CANCEL);
	ZoomCommand(ZOOM_PAN_EXIT);

	/* Exit out of the currently active menu.
	*/
	switch(current_panel)
	{
		case SCRATCHPAD:
			ScratchpadExit(EXIT);
			break;

		case ELEMENT_EDIT:
			FieldEditorExit(EXIT);
			break;

		case TIMELINK:
			TimelinkExit(EXIT);
			(void) IngredCommand(GE_DEPICTION, "FIELD NONE NONE");
			for(i = 0; i < XtNumber(timelink_btn_list); i++)
			{
				btn = XuMenuFindButton(GW_menuBar, timelink_btn_list[i]);
				if(NotNull(btn)) XtSetSensitive(btn, True);
			}
			for(i = 0; i < XtNumber(timelink_icon_list); i++)
				SetIconBarButtonSensitivity(timelink_icon_list[i], True);
			break;

		case INTERPOLATE:
			InterpolateExit();
			for(i = 0; i < XtNumber(timelink_btn_list); i++)
			{
				btn = XuMenuFindButton(GW_menuBar, timelink_btn_list[i]);
				if(NotNull(btn)) XtSetSensitive(btn, True);
			}
			for(i = 0; i < XtNumber(timelink_icon_list); i++)
				SetIconBarButtonSensitivity(timelink_icon_list[i], True);
			break;

		case ANIMATION:
			AnimationExit(EXIT);
			break;

		case CONNECT:
			ConnectExit(EXIT);
			break;
	}
	
	current_panel = panel;

	/* Now enter the new menu for control application.
	*/
	switch(panel)
	{
		case ANIMATION:

			ResetDepictionBtnColour();
			active_panel_widget = GW_animationPanel;
			AllowLimitedDepictionSequence(True);
			if(!AnimationStartup())
			{
				ShowMessages(True);
				return False;
			}
			break;

		case ELEMENT_EDIT:

			active_panel_widget = EDITOR_PANEL;
			AllowLimitedDepictionSequence(True);
			FieldEditorStartup();
			break;

		case SCRATCHPAD:

			ResetDepictionBtnColour();
			active_panel_widget = GW_scratchPanel;
			AllowLimitedDepictionSequence(True);
			ScratchpadStartup();
			break;

		case TIMELINK:

			active_panel_widget = GW_timelinkPanel;
			AllowLimitedDepictionSequence(False);
			if(!TimelinkStartup(True))
			{
				ShowMessages(True);
				return False;
			}
			for(i = 0; i < XtNumber(timelink_btn_list); i++)
			{
				btn = XuMenuFindButton(GW_menuBar, timelink_btn_list[i]);
				if(NotNull(btn)) XtSetSensitive(btn, False);
			}
			for(i = 0; i < XtNumber(timelink_icon_list); i++)
				SetIconBarButtonSensitivity(timelink_icon_list[i], False);
			break;

		case INTERPOLATE:

			ResetDepictionBtnColour();
			active_panel_widget = GW_interpolatePanel;
			AllowLimitedDepictionSequence(False);
			InterpolateStartup();
			for(i = 0; i < XtNumber(timelink_btn_list); i++)
			{
				btn = XuMenuFindButton(GW_menuBar, timelink_btn_list[i]);
				if(NotNull(btn)) XtSetSensitive(btn, False);
			}
			for(i = 0; i < XtNumber(timelink_icon_list); i++)
				SetIconBarButtonSensitivity(timelink_icon_list[i], False);
			break;

		case CONNECT:

			ResetDepictionBtnColour();
			active_panel_widget = GW_connectPanel;
			AllowLimitedDepictionSequence(False);
			ConnectStartup();
			break;
	}
	SetDrawingWindowAttachment();
	ShowMessages(True);
	return True;
}



/* Action the callback generated when a tab is selected.
 */
/*ARGSUSED*/
static void tabs_cb( Widget w, XtPointer client_data, XtPointer call_data)
{
	int n;
	XmTabStackCallbackStruct *rtn = (XmTabStackCallbackStruct *)call_data;

	/* When the tabs stack is managed this callback will be activated but
	 * we can not allow this to happen as the switch_panel function will
	 * break. Thus we must wait until the main has finished initializing
	 * before any callbacks are responded to.
	 */
	if(!tab_select_allowed) return;

	for(n = 0; n < XtNumber(panel_info); n++)
	{
		if(rtn->selected_child != panel_info[n].wid) continue;
		(void) switch_panel(panel_info[n].id);
		break;
	}
}


/*=========================================================================*/
/*
 * Create the tab panels found to the right of the map window and call
 * the panel content creation functions as well.
 */
/*=========================================================================*/
void CreateTabPanels(void)
{
	int    n;
	Widget tabManager;

	/* Add ourseleves to the observer call list for both sampling and
	 * mainline initialization completed notifications.
	 */
	AddObserver(OB_DIALOG_SAMPLING, sampling_notification);
	AddObserver(OB_MAIN_INITIALIZED, initialization_notification);

	/* Create the tabs */
	tabManager = XmVaCreateTabStack(GW_tabFrame, "tabs",
		XmNtabSide, XmTABS_ON_LEFT,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(tabManager, XmNtabSelectedCallback, tabs_cb, NULL);

	for(n = 0; n < XtNumber(panel_info); n++)
	{
		panel_info[n].wid = XmVaCreateForm(tabManager, panel_info[n].name,
			XmNresizePolicy, XmRESIZE_NONE,
			XmNwidth, PANEL_WIDTH,
			NULL);

		panel_info[n].create_fcn(panel_info[n].wid);
	}
	XtManageChild(tabManager);
}



/*=========================================================================*/
/*
*	Put the editor into a neutral state, that is not in any particular
*   "waiting for input" state. This is to be used in conjunction with the
*   activate function below when there is no possibility of event input
*   between the two function calls (in same code sequence). If this is not
*   the case use DeactivatePanels function.
*/
/*=========================================================================*/
void DeactivateMenu(void)
{
	if(dialog_is_sampling) return;

	switch(current_panel)
	{
		case ANIMATION:    AnimationExit(EXIT);   break;
		case CONNECT:      ConnectExit(EXIT);     break;
		case ELEMENT_EDIT: FieldEditorExit(EXIT); break;
		case SCRATCHPAD:   ScratchpadExit(EXIT);  break;
		case TIMELINK:     TimelinkExit(EXIT);    break;
	}
	XuUpdateDisplay(GW_mainWindow);
}


/*=========================================================================*/
/*
 *  This is similar to the above, except that the exit functions are given
 *  the key E_ZOOM instead of EXIT to feed to the IngredCommand function.
 *  This ensures that some display information is left on the screen while
 *  the user selects the zoom area instead of the visual info disappearing.
 */
/*=========================================================================*/
void DeactivateMenuForZoom(void)
{
	if(dialog_is_sampling) return;

	switch(current_panel)
	{
		case ANIMATION:    AnimationExit(E_ZOOM);   break;
		case CONNECT:      ConnectExit(E_ZOOM);     break;
		case ELEMENT_EDIT: FieldEditorExit(E_ZOOM); break;
		case SCRATCHPAD:   ScratchpadExit(E_ZOOM);  break;
		case TIMELINK:     TimelinkExit(E_ZOOM);    break;
	}
	XuUpdateDisplay(GW_mainWindow);
}


/*=========================================================================*/
/*
*  Put the editor back into its previous edit state (if applicable).
*/
/*=========================================================================*/
void ActivateMenu(void)
{
	if(!GV_active_field) return;

	ShowMessages(False);

	if(dialog_is_sampling)
	{
		if (dialog_rtn_fcn) dialog_rtn_fcn();
	}
	else
	{
		switch(current_panel)
		{
			case ANIMATION:    (void) AnimationStartup();     break;
			case CONNECT:      ConnectStartup();              break;
			case ELEMENT_EDIT: FieldEditorStartup();		  break;
			case SCRATCHPAD:   ScratchpadStartup();			  break;
			case TIMELINK:     (void) TimelinkStartup(False); break;
		}
	}
	XuUpdateDisplay(GW_mainWindow);
	ShowMessages(True);
	NotifyObservers(OB_MENU_ACTIVATION, NULL, 0);
}


/*=========================================================================*/
/*
*    Set editor state into neutral and set the panels and controls
*    insensitive. The primary use of this as apposed to the DeactivateMenu()
*    function is when the deactivation is for a considerable length of time
*    (such as during image animation) when the MainLoop is active and other
*    button actions could occur.
*/
/*=========================================================================*/
/*ARGSUSED*/
void DeactivatePanels(void)
{
	DeactivateMenu();
	set_control_sensitivity(OFF);
}


/*
*    Undo the action of DeactivatePanels()
*/
void ActivatePanels(void)
{
	ActivateMenu();
	set_control_sensitivity(ON);
}


/*=========================================================================*/
/*
*	Returns the state of the given panel.
*/
/*=========================================================================*/
Boolean PanelIsActive(PANEL_ID panel )
{
	return (panel == current_panel);
}


/*ARGSUSED*/
static void private_vis_cb(XtPointer client_data, XtIntervalId *id)
{
	Boolean state = PTR2BOOL(client_data);

	XtSetSensitive(GW_animationPanel,   state);
	XtSetSensitive(GW_connectPanel,     state);
	XtSetSensitive(EDITOR_PANEL,        state);
	XtSetSensitive(GW_timelinkPanel,    state);
	XtSetSensitive(GW_interpolatePanel, state);

	if(state)
	{
		(void) IngredCommand(GE_DEPICTION, "SHOW");
		MakeActiveDepiction(ACTIVE);
	}
	else
	{
		(void) IngredCommand(GE_DEPICTION, "HIDE");
	}
}


/*=========================================================================*/
/*
 * Sets the state of the depictions to be visible or not. The actual work
 * is carried out in a timeout procedure to ensure that all events currently
 * in the event que are processed before this is done. There were cases of a
 * race condition occuring and this cured it.
 */
/*=========================================================================*/
void SetDepictionVisibility(Boolean state)
{
	(void) XtAppAddTimeOut(GV_app_context, 0, private_vis_cb, INT2PTR(state));
}
