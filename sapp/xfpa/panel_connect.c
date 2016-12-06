/****************************************************************************/
/*
*  File:	panel_conenct.c
*
*  Purpose:	 Provides the controlling logic for the object connection
*            tab panel.
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
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>
#include <graphics.h>
#include "editor.h"
#include "menu.h"
#include "selector.h"
#include "observer.h"
#include "contextMenu.h"

/* right mouse button context menu buttons */
static Widget panelContextMenu = NullWidget;
static Widget cm_deleteBtn, cm_selectAllBtn, cm_separator;

/*======================== LOCAL FUNCTIONS ================================*/


/* Take action depending on the action message from ingred */
static void ingred_message_observer( CAL cal, String *parms, int nparms )
{
	if(!same_ic(parms[0],"CONNECT")) return;
}


/* The following 2 functions create and control the right mouse button panel
 * context menu. If a context menu is not needed for the final panel they must
 * be deleted.
 */
/*ARGSUSED*/
static void context_menu_cb(Widget w, XtPointer client_data, XtPointer unused )
{
	int ndx = PTR2INT(client_data);
}


static void create_context_menu(void)
{
	int    i;
	Widget btn;

	panelContextMenu = CreatePanelContextMenu("connectContextMenu");

	/* These buttons are just place holders to show what must be done. The actual
	 * buttons must be decided when the panel is designed.
	 */
	cm_selectAllBtn = XmVaCreatePushButton(panelContextMenu, "selectAllBtn", NULL);
	XtAddCallback(cm_selectAllBtn, XmNactivateCallback, context_menu_cb, INT2PTR(100));

	cm_deleteBtn = XmVaCreatePushButton(panelContextMenu, "deleteBtn", NULL);
	XtAddCallback(cm_deleteBtn, XmNactivateCallback, context_menu_cb, INT2PTR(101));

	cm_separator = XmCreateSeparator(panelContextMenu, "sep", NULL, 0);

	/* Temporary just to have the buttons show up. Normally they will be managed
	 * and unmanaged depending on some state panel. Use the Manage(Widget, Boolean)
	 * function.
	 */
	XtManageChild(cm_selectAllBtn);
	XtManageChild(cm_deleteBtn);
}



/*================ Public Functions ===================*/


/*
 *  Create the panel.
 */
void CreateConnectPanel(Widget parent)
{
	GW_connectPanel = parent;

	XtVaSetValues(GW_connectPanel, XmNhorizontalSpacing, 6, XmNverticalSpacing, 20, NULL);

	XtManageChild(GW_connectPanel);

	create_context_menu();
	AddIngredObserver(ingred_message_observer);
}


/*
*	Activates and configures the panel.
*/
void ConnectStartup(void)
{
	SetActiveContextMenu(panelContextMenu);
}


/*
*  Exit the panel setting whatever states are required.
*/
/*ARGSUSED*/
void ConnectExit(String key)
{
	SetActiveContextMenu(None);
}

