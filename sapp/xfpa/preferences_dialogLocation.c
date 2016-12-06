/*==============================================================================*/
/*
*	   File:	preferences_dialogLocation.c
*
*	Purpose:	Sets display location preferences for the dialogs.
*
*     Notes:	This is used in conjunction with the preferencesDialog.
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
/*==============================================================================*/

#include <ctype.h>
#include "global.h"
#include <Xm/List.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/ToggleB.h>
#include <Xm/Column.h>
#include "depiction.h"
#include "editor.h"
#include "fcstText.h"
#include "graphic.h"
#include "guidance.h"
#include "imagery.h"
#include "observer.h"
#include "pointFcst.h"
#include "preferences.h"
#include "radarSTAT.h"

#define K1 "dlg"
#define K2 "dpy"

static int        ndisplays      = 0;
static String     *displays      = NULL;
static int        *dpy_no        = NULL;
static int        default_dpy_no = 0;
static WidgetList rc             = NULL;


/* The following is the list of known dialogs that are not application
 * modal. Note that dialogs that do not have an entry in the resource
 * file will not appear in the list.
 */
static struct {
	String  name;				/* dialog name or profile name */
	Boolean wait_for_guidance;	/* bring up dialog when guidance initialized */
	void    (*activate)(Widget);/* activation function */
	Boolean exists;				/* does dialog exist in resource database? */
} dialogs[] = {
	{"attributesEntryMenu",  False, NULL                               },
	{"autoImport",           False, ACTIVATE_autoImportDialog          },
	{"fcstText",             False, ACTIVATE_fcstTextDialog            },
	{"fieldDisplayState",    False, ACTIVATE_fieldDisplayStateDialog   },
	{"fieldUpdate",          False, ACTIVATE_updateFieldsDialog        },
	{"graphicPreview",       False, NULL                               },
	{"graphicProducts",      False, ACTIVATE_graphicProductsDialog     },
	{"guidanceAvailability", True,  ACTIVATE_guidanceAvailabilityDialog},
	{"guidanceLegendDialog", True,  ACTIVATE_guidanceLegendDialog      },
	{"guidSelect",           True,  ACTIVATE_guidanceDialog            },
	{"guidanceStatus",       True,  ACTIVATE_guidanceStatusDialog      },
	{"imageryControlDialog", False, ACTIVATE_imageryControlDialog      },
	{"mapOverlayDialog",     False, ACTIVATE_mapOverlayDialog          },
	{"onlineHelp",           False, NULL                               },
	{"pointFcst",            False, ACTIVATE_pointFcstSelectDialog     },
	{"radarLegend",          False, NULL                               },
	{"radarStatDialog",      False, ACTIVATE_radarStatDialog           },
	{"satelliteLegend",      False, NULL                               },
	{"userReport",           False, NULL                               },
	{"zoomControl",          False, ACTIVATE_zoomDialog                }
}; 


/*================= Private Functions ====================*/

/* This is called because we registered as an observer to be informed when
 * the program has finished initializing and is in the event loop. Put up
 * all non-guidance related dialogs.
 */
/*ARGSUSED*/
static void activate_profiled_dialogs(String *parms, int nparms)
{
	int i;
	for(i = 0; i < XtNumber(dialogs); i++)
	{
		if(!dialogs[i].wait_for_guidance && XuIsProfiledDialog(dialogs[i].name))
			if(dialogs[i].activate) dialogs[i].activate(GW_mainWindow);
	}
	DeleteObserver(OB_GUIDANCE_READY, activate_profiled_dialogs);
}

/* This is called because we registered as an observer to be informed when
 * all of the guidance has been updated. When the guidance is done we can
 * put up all of the guidance related dialogs.
 */
/*ARGSUSED*/
static void activate_profiled_guidance_dialogs(String *parms, int nparms)
{
	int i;
	for(i = 0; i < XtNumber(dialogs); i++)
	{
		if(dialogs[i].wait_for_guidance && XuIsProfiledDialog(dialogs[i].name))
			if(dialogs[i].activate) dialogs[i].activate(GW_mainWindow);
	}
	DeleteObserver(OB_GUIDANCE_READY, activate_profiled_guidance_dialogs);
}


/*======================= Public Functions ===========================*/


int InitDialogLocationOptions(void)
{
	int     n, k;
	String  s, p, b;

	if (GV_edit_mode)
	{
		AddObserver(OB_MAIN_INITIALIZED, activate_profiled_dialogs);
		AddObserver(OB_GUIDANCE_READY,   activate_profiled_guidance_dialogs);
	}

	/* Get the list of displays available. This we keep around
	*  as it is not all that much information. The resource file
	*  specifies the order of the displays. Note that if multiple
	*  X severs are used and not multiple screens (like SUN) then
	*  the displays must be specified in the resource file as this
	*  code will not auto detect them (tried it but the delay
	*  to program startup was significant).
	*/
	if(NotNull(s = XuGetStringResource(".screenList",NULL)))
	{
		p = XtNewString(s);
		while(NotNull(s = string_arg(p)))
		{
			if(XuIsValidDisplayString(p))
			{
				displays = MoreStringArray(displays, ndisplays+1);
				displays[ndisplays] = XuStandardDisplayString(s);
				ndisplays++;
			}
		}
		XtFree(p);
	}

	/* This ensures that we have all possible screens even if they
	 * were not specified above. The assumption here is that we will
	 * never have more than 10 screens.
	 */
	for(n = 0; n < XScreenCount(XtDisplay(GW_mainWindow)) && n < 10; n++)
	{
		p = XuStandardDisplayString(XDisplayString(XtDisplay(GW_mainWindow)));
		s = strrchr(p, '.') + 1;
		(void) sprintf(s, "%d", n);
		for(k=0; k < ndisplays; k++)
			if(same(p,displays[k])) break;
		if( k >= ndisplays )
		{
			displays = MoreStringArray(displays, ndisplays+1);
			displays[ndisplays] = p;
			ndisplays++;
		}
		else
		{
			XtFree(p);
		}
	}

	/* Put any settings into the environment. Make sure that the
	*  display stored in the database is valid before acceptance
	*  and that the dialog name is recognized.
	*/
	for(n = 0; n < XtNumber(dialogs); n++)
	{
		char mbuf[200];
		dialogs[n].exists = True;
		(void) snprintf(mbuf, sizeof(mbuf), "*.%s.dialogTitle", dialogs[n].name);
		if((s = XuGetStringResource(mbuf,NULL)) == NULL)
		{
			dialogs[n].exists = False;
		}
		else
		{
			if(!XuStateDataGet(K1, K2, dialogs[n].name, &b)) continue;
			if(InList(b, ndisplays, displays, NULL))
			{
				(void) snprintf(mbuf, sizeof(mbuf), "*.%s.%s", dialogs[n].name, XuNdialogDisplay);
				XuPutStringResource(mbuf, b);
			}
			XtFree(b);
		}
	}
	return XScreenCount(XtDisplay(GW_mainWindow));
}


void DialogLocationOptions(Widget parent )
{
	int      i, j;
	char     mbuf[200];
	String   p;
	Widget   sw1, w;
	XmString xmlabel;
	XmString xl = XuNewXmString("");

	/* Our active display reference list. We get any previously set
	*  values from the state file and set the list.
	*/
	p = XuStandardDisplayString(XDisplayString(XtDisplay(parent)));
	(void)InList(p, ndisplays, displays, &default_dpy_no);
	dpy_no = (int *)XtCalloc(XtNumber(dialogs), sizeof(int));
	for(i = 0; i < XtNumber(dialogs); i++)
	{
		if(!dialogs[i].exists) continue;
		dpy_no[i] = default_dpy_no;
		if(XuStateDataGet(K1, K2, dialogs[i].name, &p))
		{
			(void)InList(p, ndisplays, displays, &dpy_no[i]);
			XtFree(p);
		}
	}

	sw1 = XmVaCreateScrolledWindow(parent, "alliedModelList",
		XmNscrollingPolicy, XmAUTOMATIC,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	w = XmVaCreateColumn(sw1, "sel",
		XmNdefaultEntryLabelAlignment, XmALIGNMENT_END,
		XmNlabelSpacing, 9,
		XmNitemSpacing,  0,
		XmNmarginWidth,  9,
		XmNmarginHeight, 9,
		NULL);

	(void)XmVaCreateManagedLabel(w, "location", NULL);
	xmlabel = XmStringCreateLocalized(" ");
	(void)XmVaCreateManagedLabel(w, " ",
		XmNentryLabelString, xmlabel,
		NULL);
	XmStringFree(xmlabel);

	rc = NewWidgetArray(XtNumber(dialogs));
	for( i = 0; i < XtNumber(dialogs); i++ )
	{
		if(!dialogs[i].exists) continue;

		/* Get the dialog title from the resource environment as this can
		 * vary by language.
		 */
		(void) snprintf(mbuf, sizeof(mbuf), "*.%s.dialogTitle", dialogs[i].name);
		xmlabel = XuGetXmStringResource(mbuf,"???");

		rc[i] = XmVaCreateManagedRowColumn(w, "rcb",
			XmNentryLabelString, xmlabel,
			XmNorientation, XmHORIZONTAL,
			XmNradioBehavior, True,
			XmNspacing, 0,
			NULL);
		for(j = 0; j < ndisplays; j++)
		{
			(void) snprintf(mbuf, sizeof(mbuf), "%d", j);
			(void) XmVaCreateManagedToggleButton(rc[i], mbuf,
				XmNset, (dpy_no[i] == j) ? XmSET:XmUNSET,
				XmNlabelString, xl,
				XmNsensitive, (ndisplays > 1),
				NULL);
		}
		XmStringFree(xmlabel);
	}
	XmStringFree(xl);
	XtManageChild(w);
	XtManageChild(sw1);
}


void SetDialogLocationOptions(void)
{
	int    i, j, n;
	char   mbuf[100];
	Widget w;

	for(i = 0; i < XtNumber(dialogs); i++)
	{
		if(!dialogs[i].exists) continue;

		for( n = 0, j = 0; j < ndisplays; j++)
		{
			(void) snprintf(mbuf, sizeof(mbuf), "%d", j);
			w = XtNameToWidget(rc[i], mbuf);
			if(XmToggleButtonGetState(w)) n = j;
		}
		if( dpy_no[i] != n )
		{
			XuStateDataSave(K1, K2, dialogs[i].name, displays[n]);
			(void) snprintf(mbuf, sizeof(mbuf), "*.%s.%s", dialogs[i].name, XuNdialogDisplay);
			XuPutStringResource(mbuf, displays[n]);
			dpy_no[i] = n;
		}
	}
}


void ExitDialogLocationOptions(void)
{
	FreeItem(rc);
	FreeItem(dpy_no);
	default_dpy_no = 0;
}

