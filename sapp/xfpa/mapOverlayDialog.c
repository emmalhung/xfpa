/****************************************************************************
*
*  File:     mapOverlayDialog.c
*
*  Purpose:  Holds code for setting the active map overlays.
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

#include "global.h"
#include <sys/stat.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/ToggleB.h>
#include <Xm/Column.h>
#include "help.h"
#include "observer.h"

#define MAP	"map"
#define KEY	"ol"

typedef struct {
	String  label;
	String  fname;
	Boolean selected;
	Boolean new;
	Boolean reread;
	Boolean pad;
	time_t  mtime;
	Widget  indicator;
} OVINFO;
	
	

static Widget   dialog = NullWidget;
static int      noverlays = 0;
static OVINFO   *overlays;
static Pixmap   red_flag = XmUNSPECIFIED_PIXMAP;


static Boolean check_overlays_for_changes( Boolean );


/* Send map initialization sequence to Ingred.
*/
void InitMap(void)
{
    int    i;
    char   mbuf[500];
	String stored;
    String base_map;
    SETUP  *setup;
    PARM   *setup_parm;

	/* Get overlay information */
    setup = GetSetup(MAP_OVERLAY);
    noverlays = setup->nentry;

    /* Send the base map and overlay state to Ingred */
    setup_parm = GetSetupParms(MAP_BASE);
    base_map = (setup_parm) ? setup_parm->parm[1]:NULL;
    snprintf(mbuf, sizeof(mbuf), "MAP BASE %s %d", base_map, noverlays);
    (void) IngredCommand(GE_ACTION, mbuf);

    if(!noverlays) return;

	/* Create overlay state data array */
	overlays = NewMem(OVINFO, noverlays);

	for( i = 0; i < noverlays; i++)
	{
		struct stat sb;
		String path_name;

		overlays[i].label = SetupParm(setup,i,0);
		overlays[i].fname = SetupParm(setup,i,1);
		/*
		 * Get the time stamp of the overlay files.
		 */
		path_name = get_path(MAPS, overlays[i].fname);
		if( stat(path_name, &sb) == 0 )
			overlays[i].mtime = sb.st_mtime;

		if(XuStateDataGet(MAP,KEY,overlays[i].fname, &stored))
		{
			if(same_ic(stored, "ON"))
			{
				overlays[i].selected = True;
				snprintf(mbuf, sizeof(mbuf), "MAP OVERLAY %d %s", i+1, overlays[i].fname);
				(void) IngredCommand(GE_ACTION, mbuf);
			}
			XtFree(stored);
		}
	}
	AddSourceObserver(check_overlays_for_changes,"OverlayChange");
}


/* Check for an overlay change. If found we put an indicator beside the overlay.
 */
/*ARGSUSED*/
static Boolean check_overlays_for_changes(Boolean unused)
{
	Source src = FindSourceByName(MAPS, NULL);
	if( src && src->modified )
	{
		int  i;
		for(i = 0; i < noverlays; i++)
		{
			struct stat sb;
			String fname = get_path(MAPS, overlays[i].fname);
			if( fname != NULL && stat(fname, &sb) == 0 && sb.st_mtime > overlays[i].mtime )
			{
				overlays[i].reread = True;
				overlays[i].mtime  = sb.st_mtime;
				if (dialog)
				{
					XtVaSetValues(overlays[i].indicator, XmNentryLabelPixmap, red_flag, NULL);
				}
			}
		}
	}
	return True;
}



/*ARGSUSED*/
static void overlay_cb(Widget  w , XtPointer client_data , XtPointer call_data )
{
	overlays[PTR2INT(client_data)].new = XmToggleButtonGetState(w);
}


/*ARGSUSED*/
static void set_overlays_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int i;

	if(!client_data)	/* SET */
	{
		for(i = 0; i < noverlays; i++)
		{
			if(overlays[i].new)
			{
				if(overlays[i].reread)
				{
					XuStateDataSave(MAP,KEY, overlays[i].fname, "ON");
					(void) IngredVaCommand(GE_ACTION, "MAP OVERLAY %d %s REREAD", i+1, overlays[i].fname);
					XtVaSetValues(overlays[i].indicator, XmNentryLabelPixmap, XmUNSPECIFIED_PIXMAP, NULL);
					overlays[i].reread = False;
				}
				else if(!overlays[i].selected)
				{
					XuStateDataSave(MAP,KEY, overlays[i].fname, "ON");
					(void) IngredVaCommand(GE_ACTION, "MAP OVERLAY %d %s", i+1, overlays[i].fname);
				}
			}
			else if(overlays[i].selected)
			{
				XuStateDataSave(MAP,KEY, overlays[i].fname, "OFF");
				(void) IngredVaCommand(GE_ACTION, "MAP OVERLAY %d OFF", i+1);
			}
			overlays[i].selected = overlays[i].new;
		}
	}
	else	/* UPDATE */
	{
		for(i = 0; i < noverlays; i++)
		{
			if(overlays[i].reread && overlays[i].selected)
			{
				(void) IngredVaCommand(GE_ACTION, "MAP OVERLAY %d %s REREAD", i+1, overlays[i].fname);
				overlays[i].reread = False;
			}
			XtVaSetValues(overlays[i].indicator, XmNentryLabelPixmap, XmUNSPECIFIED_PIXMAP, NULL);
		}
	}
}


/*ARGSUSED*/
static void exit_cb( Widget w, XtPointer cd, XtPointer unused )
{
	XuFreePixmap(dialog, red_flag);
	red_flag = XmUNSPECIFIED_PIXMAP;
	dialog = NullWidget;
}


void ACTIVATE_mapOverlayDialog(Widget parent )
{
	int    i;

	static XuDialogActionsStruct action_items[] = {
		{ "setBtn",    set_overlays_cb,   (XtPointer) 0    },
		{ "updateBtn", set_overlays_cb,   (XtPointer) 1    },
		{ "closeBtn",  XuDestroyDialogCB, NULL             },
		{ "helpBtn",   HelpCB,            HELP_MAP_OVERLAYS}
	};

	if(noverlays < 1) return;

	if (!dialog)
	{
		dialog = XuCreateToplevelDialog(GW_mainWindow, xmColumnWidgetClass, "mapOverlayDialog",
			XmNorientation, XmVERTICAL,
			XuNretainGeometry, XuRETAIN_ALL,
			XuNactionAreaItems, action_items,
			XuNnumActionAreaItems, XtNumber(action_items),
			XuNdestroyCallback, exit_cb,
			XmNminWidth, 100,
			XmNminHeight, 100,
			NULL);

		red_flag = XuGetPixmap(dialog, "redflag");

		for(i = 0; i < noverlays; i++)
		{
			overlays[i].new = overlays[i].selected;

			overlays[i].indicator = XmVaCreateManagedToggleButton(dialog, overlays[i].label,
				XmNset, (overlays[i].selected) ? XmSET:XmUNSET,
				XmNentryLabelType, XmPIXMAP,
				XmNentryLabelPixmap, (overlays[i].reread)? red_flag:XmUNSPECIFIED_PIXMAP,
				NULL);
			XtAddCallback(overlays[i].indicator, XmNvalueChangedCallback, overlay_cb, INT2PTR(i));
		}
	}
	XuShowDialog(dialog);
}
