/*========================================================================*/
/*
*	File:		depiction_zoom.c
*
*	Purpose:	Contains functions responsible for commanding Ingred in
*               zooming operations and for receiving status information
*               from Ingred after a zoom operation.
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
/*========================================================================*/

#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <ingred.h>
#include "global.h"
#include "iconBar.h"
#include "depiction.h"
#include "editor.h"
#include "menu.h"
#include "observer.h"
#include "contextMenu.h"


static Widget dialog = NullWidget;
static Widget dialogInBtn, dialogOutBtn, dialogPanBtn, dialogExitBtn;

typedef struct {
	Boolean active;
	float   x;
	float   y;
	float   width;
	float   height;
} ZLEV;

static float   h_max, v_max;		/* scrollbar sizing */
static Boolean in_pan_mode = False; /* pan mode state variable */
static Boolean zooming_in  = False;	/* interlock during zoom process */
static Boolean my_command  = False;	/* these functions are issuing the commands to Ingred */
static int     maxzlev     = 0;		/* size of zoom state array */
static int     nzlev       = 0;		/* number of levels of zoom applied */
static ZLEV    *zlev       = NULL;	/* zoom level state data */
static String  parms[2];
static Boolean in_sensitive      = True;
static Boolean out_sensitive     = False;
static Boolean pan_sensitive     = False;
static Boolean exit_sensitive    = False;
static Boolean magnify_cursor_on = False;
static Boolean busy_cursor_on    = False;


/*======================= PRIVATE FUNCTIONS ============================*/

static void set_button_state(Boolean by_states)
{
	if(by_states)
	{
		XtSetSensitive(XuMenuFindButton(GW_menuBar, MENU_Actions_Zoom_in), in_sensitive);
		XtSetSensitive(XuMenuFindButton(GW_menuBar, MENU_Actions_Zoom_out), out_sensitive);
		XtSetSensitive(XuMenuFindButton(GW_menuBar, MENU_Actions_Zoom_pan), pan_sensitive);
		XtSetSensitive(XuMenuFindButton(GW_menuBar, MENU_Actions_Zoom_exit), exit_sensitive);

		SetIconBarButtonSensitivity(ZOOM_IN_ICON,  in_sensitive );
		SetIconBarButtonSensitivity(ZOOM_OUT_ICON, out_sensitive);
		SetIconBarButtonSensitivity(ZOOM_PAN_ICON, pan_sensitive);

		if (dialog)
		{
			XtSetSensitive(dialogInBtn, in_sensitive);
			XtSetSensitive(dialogOutBtn, out_sensitive);
			XtSetSensitive(dialogPanBtn, pan_sensitive);
			XtSetSensitive(dialogExitBtn, exit_sensitive);
		}
	}
	else
	{
		XtSetSensitive(XuMenuFindButton(GW_menuBar, MENU_Actions_Zoom_in), False);
		XtSetSensitive(XuMenuFindButton(GW_menuBar, MENU_Actions_Zoom_out), False);
		XtSetSensitive(XuMenuFindButton(GW_menuBar, MENU_Actions_Zoom_pan), False);
		XtSetSensitive(XuMenuFindButton(GW_menuBar, MENU_Actions_Zoom_exit), False);

		SetIconBarButtonSensitivity(ZOOM_IN_ICON,  False );
		SetIconBarButtonSensitivity(ZOOM_OUT_ICON, False);
		SetIconBarButtonSensitivity(ZOOM_PAN_ICON, False);

		if (dialog)
		{
			XtSetSensitive(dialogInBtn, False);
			XtSetSensitive(dialogOutBtn, False);
			XtSetSensitive(dialogPanBtn, False);
			XtSetSensitive(dialogExitBtn, False);
		}
	}
}


static void area_command(String cbuf )
{
	Boolean my_command_not_set = !my_command;

	if (my_command_not_set) my_command = True;
	if (in_pan_mode)
	{
		(void) IngredCommand(GE_ZOOM, cbuf);
		(void) IngredCommand(GE_ZOOM, E_PAN);
	}
	else
	{
		DeactivateMenuForZoom();
		(void) IngredCommand(GE_ZOOM, cbuf);
		ActivateMenu();
	}
	if (my_command_not_set) my_command = False;
}


static void reset_zoom_controls(void)
{
	String parms[2];
	zooming_in = False;
	if (magnify_cursor_on)
	{
		XuSetCursor(GW_mapWindow, XuMAGNIFY_CURSOR, OFF);
		magnify_cursor_on = False;
	}
	if (busy_cursor_on)
	{
		XuSetCursor(GW_mapWindow, XuBUSY_CURSOR, OFF);
		busy_cursor_on = False;
	}
	if(nzlev > 0 && !zlev[nzlev-1].active) nzlev--;
	in_sensitive   = True;
	out_sensitive  = (nzlev > 0);
	pan_sensitive  = (nzlev > 0);
	exit_sensitive = (nzlev > 0);
	set_button_state(True);

	parms[0] = E_ZOOM;
	parms[1] = E_OFF;
	NotifyObservers(OB_ZOOM, parms, 2);

	/* Bug 20090716: Reissue the in pan mode notification. */
	if(in_pan_mode)
	{
		parms[0] = E_PAN_MODE;
		parms[1] = E_ON;
		NotifyObservers(OB_ZOOM, parms, 2);
	}
}


static void exit_pan_mode(void)
{
	in_pan_mode    = False;
	in_sensitive   = True;
	out_sensitive  = (nzlev > 0);
	pan_sensitive  = (nzlev > 0);
	exit_sensitive = (nzlev > 0);
	set_button_state(True);
	(void) IngredCommand(GE_ZOOM, E_PAN_DONE);
	XuSetCursor(GW_mapWindow, XuPAN_CURSOR, OFF);
	parms[0] = E_PAN_MODE;
	parms[1] = E_OFF;
	NotifyObservers(OB_ZOOM, parms, 2);
}


/* Called by Ingred to pass parameters back to the interface while
 * in zoom mode.
 */
/*ARGSUSED*/
static void zoom_data_from_ingred(CAL cal, String *parms, int nparms)
{
	int    pc   = 0;
	String item = parms[pc++];

	if(same_ic(item, E_EDIT))
	{
		if(same_ic(parms[pc++], E_DRAWING))
			set_button_state(!same_ic(parms[pc++],E_ON));
	}
	else if(same_ic(item, E_ZOOM))
	{
		int     n;
		Widget  s[2];
	
		item = parms[pc++];

		if(same_ic(item,E_AREA))
		{
			in_sensitive = True;
			set_button_state(True);

			if( nzlev < 1 ) return;  /* paranoia */

			XtVaGetValues(GW_drawingWindow,
				XmNhorizontalScrollBar, &s[0],
				XmNverticalScrollBar, &s[1],
				NULL);

			n = nzlev-1;
			zlev[n].active = True;
			zlev[n].x      = atof(parms[pc++]);
			zlev[n].y      = atof(parms[pc++]);
			zlev[n].width  = atof(parms[pc++]);
			zlev[n].height = atof(parms[pc++]);

			v_max  = (100.0/(float)ZOOM_LINE_SIZE)*100.0/zlev[n].height;
			h_max  = (100.0/(float)ZOOM_LINE_SIZE)*100.0/zlev[n].width;

			XtVaSetValues(s[0],
				XmNmaximum, NINT(h_max),
				XmNvalue, NINT(zlev[n].x*h_max/100.0),
				NULL);

			XtVaSetValues(s[1],
				XmNmaximum, NINT(v_max),
				XmNvalue, NINT(zlev[n].y*v_max/100.0),
				NULL);

			if(zooming_in)
			{
				reset_zoom_controls();
				if(in_pan_mode)
				{
					XuSetCursor(GW_mapWindow, XuPAN_CURSOR, ON);
					(void) IngredCommand(GE_ZOOM, E_PAN);
				}
				else
				{
					ActivateMenu();
					XtManageChildren(s, 2);
				}
			}
		}
		else if(same_ic(item,E_PAN))
		{
			if(same_ic(parms[pc++],"END") && in_pan_mode)
			{
				exit_pan_mode();
				ActivateMenu();
			}
		}
		else if(same_ic(item,E_START))
		{
			String prm[] = {E_ZOOM, E_OFF};
			NotifyObservers(OB_ZOOM, prm, 2);
			XuSetCursor(GW_mapWindow, XuMAGNIFY_CURSOR, OFF);
			magnify_cursor_on = False;
			XuSetCursor(GW_mapWindow, XuBUSY_CURSOR, ON);
			busy_cursor_on = True;
		}
	}
}


/*======================= PUBLIC FUNCTIONS ========================*/


/* For things that need initialization from the mainline.
 */
void InitZoomFunctions(void)
{
	AddIngredObserver(zoom_data_from_ingred);
}


/* Send zoom commands to Ingred and set the state of all of the
 * zoom control buttons, the cursor and keep track of zoom level
 * information.
 */
void ZoomCommand(ZOOM_CMDS cmd)
{
	int    n;
	char   mbuf[200];
	Widget s[2];

	my_command = True;

	switch(cmd)
	{
		case ZOOM_IN:
			in_sensitive   = False;
			out_sensitive  = False;
			pan_sensitive  = False;
			exit_sensitive = False;
			set_button_state(True);
			zooming_in = True;
			if(in_pan_mode)
			{
				XuSetCursor(GW_mapWindow, XuPAN_CURSOR, OFF);
			}
			else
			{
				DeactivateMenuForZoom();
			}
			XuSetCursor(GW_mapWindow, XuMAGNIFY_CURSOR, ON);
			magnify_cursor_on = True;
			nzlev++;
			if(nzlev >= maxzlev)
			{
				maxzlev += 10;
				zlev = MoreMem(zlev, ZLEV, maxzlev);
			}
			zlev[nzlev-1].active = False;
			(void) IngredCommand(GE_ZOOM, "IN");
			parms[0] = E_ZOOM;
			parms[1] = E_ON;
			NotifyObservers(OB_ZOOM, parms, 2);
			break;

		case ZOOM_OUT:
			if(nzlev == 0) break;
			nzlev--;
			if(nzlev < 1)
			{
				nzlev = 0; /* paranoia */
				ZoomCommand(ZOOM_EXIT);
			}
			else
			{
				n = nzlev-1;
				(void) snprintf(mbuf, sizeof(mbuf), "AREA %f %f %f %f", zlev[n].x, zlev[n].y, zlev[n].width, zlev[n].height);
				area_command(mbuf);
			}
			break;

		case ZOOM_PAN:
			/* If we are already in pan mode or not zoomed then ignore this */
			if(in_pan_mode) break;
			if(nzlev == 0) break;
			in_pan_mode    = True;
			pan_sensitive  = False;
			in_sensitive   = True;
			out_sensitive  = True;
			exit_sensitive = True;
			set_button_state(True);
			XuSetCursor(GW_mapWindow, XuPAN_CURSOR, ON);
			/* Use zooming_in to lock out the ZoomStateCheck function activation */
			zooming_in = True;
			DeactivateMenuForZoom();
			zooming_in = False;
			parms[0] = E_PAN_MODE;
			parms[1] = E_ON;
			NotifyObservers(OB_ZOOM, parms, 2);
			(void) IngredCommand(GE_ZOOM, E_PAN);
			break;

		case ZOOM_PAN_EXIT:
			if(!in_pan_mode) break;
			exit_pan_mode();
			ActivateMenu();
			break;

		case ZOOM_EXIT:
			XtVaGetValues(GW_drawingWindow,
				XmNhorizontalScrollBar, &s[0],
				XmNverticalScrollBar, &s[1],
				NULL);
			XtUnmanageChildren(s, 2);
			if(in_pan_mode)
			{
				exit_pan_mode();
			}
			else
			{
				DeactivateMenuForZoom();
			}
			out_sensitive  = False;
			pan_sensitive  = False;
			exit_sensitive = False;
			set_button_state(True);
			(void) IngredCommand(GE_ZOOM, "OUT");
			ActivateMenu();
			FreeItem(zlev);
			nzlev = maxzlev = 0;
			parms[0] = E_PAN_MODE;
			parms[1] = E_CANCEL;
			NotifyObservers(OB_ZOOM, parms, 2);
			break;

		case ZOOM_CANCEL:
			if(!zooming_in) break;
			reset_zoom_controls();
			if(in_pan_mode)
			{
				XuSetCursor(GW_mapWindow, XuPAN_CURSOR, ON);
				(void) IngredCommand(GE_ZOOM, E_PAN);
			}
			else
			{
				ActivateMenu();
			}
			break;
	}
	my_command = False;
}


/* This function is used in IngredCommand() to allow us to check the zoom
 * state when commands are issued to Ingred. If we are in the process of
 * zooming or panning, then we need to reset the state if anything besides
 * these functions issue the command and thus change the state of Ingred.
 * In the code the my_command variable is used in addition to checking the
 * type as part of zooming is to issue other commands to set Ingred into a
 * neutral state before we issue the zoom command.
 */
/*ARGSUSED*/
void ZoomStateCheck(GE_CMD type , String cmd )
{
	if(type == GE_ZOOM || my_command) return;
	
	if(zooming_in)
	{
		reset_zoom_controls();
		ActivateMenu();
	}
	else if(in_pan_mode)
	{
		exit_pan_mode();
		ActivateMenu();
	}
}


/*ARGSUSED*/
void HorizontalZoomScrollBarCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	int max_slider_value;
	char mbuf[128];
	XmScrollBarCallbackStruct *cbs = (XmScrollBarCallbackStruct *)call_data;

	if(nzlev < 1) return;

	/* Handle the max slider value case specially to avoid round off error.
	*/
	max_slider_value = NINT(h_max) - 100/ZOOM_LINE_SIZE;
	(void) snprintf(mbuf, sizeof(mbuf), "AREA %f -", (cbs->value == max_slider_value) ?
		100.0 - zlev[nzlev-1].width : (float)cbs->value*100.0/h_max);
	area_command(mbuf);
}


/*ARGSUSED*/
void VerticalZoomScrollBarCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	int max_slider_value;
	char mbuf[128];
	XmScrollBarCallbackStruct *cbs = (XmScrollBarCallbackStruct *)call_data;

	if(nzlev < 1) return;

	/* Handle the max slider value case specially to avoid round off error.
	*/
	max_slider_value = NINT(v_max) - 100/ZOOM_LINE_SIZE;
	(void) snprintf(mbuf, sizeof(mbuf), "AREA - %f", (cbs->value == max_slider_value) ?
		100.0 - zlev[nzlev-1].height : (float)cbs->value*100.0/v_max);
	area_command(mbuf);
}


/*ARGSUSED*/
static void zoom_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	ZoomCommand((ZOOM_CMDS)client_data);
}


/* The normal tear-off functionality of Motif does not allow us to initiate a tear-off
 * by a function call. A requirement was for the state and location of the zoom tear-off
 * to be saved and have it re-appear on program startup if required. The only way to do
 * this was to emulate a tear-off with my own function.
 */
void ACTIVATE_zoomDialog(Widget parent)
{
	if (!dialog)
	{
		dialog = XuCreateToplevelDialog(parent, xmRowColumnWidgetClass, "zoomControl",
			XmNnoResize, True,
			XmNresizePolicy, XmRESIZE_NONE,
			XuNretainGeometry, XuRETAIN_POSN_ONLY,
			XmNminWidth, 10,
			XmNminHeight, 10,
			XuNdestroyCallback, XuExitOnDestroyCallback,
			XuNdestroyData, &dialog,
			NULL);

		dialogInBtn = XtVaCreateManagedWidget("zoomIn", xmPushButtonWidgetClass, dialog, NULL); 
		XtAddCallback(dialogInBtn, XmNactivateCallback, zoom_cb, (XtPointer)ZOOM_IN);
		
		dialogOutBtn = XtVaCreateManagedWidget("zoomOut", xmPushButtonWidgetClass, dialog, NULL); 
		XtAddCallback(dialogOutBtn, XmNactivateCallback, zoom_cb, (XtPointer)ZOOM_OUT);
		
		dialogPanBtn = XtVaCreateManagedWidget("zoomPan", xmPushButtonWidgetClass, dialog, NULL); 
		XtAddCallback(dialogPanBtn, XmNactivateCallback, zoom_cb, (XtPointer)ZOOM_PAN);

		dialogExitBtn = XtVaCreateManagedWidget("zoomExit", xmPushButtonWidgetClass, dialog, NULL);
		XtAddCallback(dialogExitBtn, XmNactivateCallback, zoom_cb, (XtPointer)ZOOM_EXIT);
	}
	set_button_state(True);
	XuShowDialog(dialog);
}
