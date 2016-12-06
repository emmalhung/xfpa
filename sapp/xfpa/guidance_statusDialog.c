/*=========================================================================*/
/*
*      File: guidance_statusDialog.c
*
*   Purpose: Displays the current ingest status of all guidance fields.
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
/*=========================================================================*/
#include "global.h"
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>
#include "iconBar.h"
#include "resourceDefines.h"
#include "guidance.h"
#include "observer.h"
#include "help.h"


static void    display_status (XtPointer, XtIntervalId*);
static void    exit_cb (Widget, XtPointer, XtPointer);
static void    activate_new_source_arrived_icon (XtPointer, XtIntervalId*);
static Boolean update_guidance_status (Boolean);

static Widget     dialog         = NullWidget;
static WidgetList timew          = (WidgetList)NULL;
static time_t    *last_stat_time = (time_t*)NULL;
static Boolean    flash_stop      = False;


/*===================== PUBLIC FUNCTIONS ============================*/

void ACTIVATE_guidanceStatusDialog(Widget reference_widget )
{
	int        i, nsrc;
	SourceList src;
	Widget     label, sep, rc1, rc2;

	static XuDialogActionsStruct action_items[] = {
		{ "closeBtn", XuDestroyDialogCB, NULL },
		{ "helpBtn",  HelpCB,  HELP_GUIDANCE_STATUS }
	};

	if (dialog) return;

	dialog = XuCreateFormDialog(reference_widget, "guidanceStatus",
		XmNdialogStyle, XmDIALOG_MODELESS,
		XuNdestroyCallback, exit_cb,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		NULL);

	SourceListByType(SRC_FPA|SRC_NWP|SRC_ALLIED, FpaC_TIMEDEP_ANY, &src, &nsrc);

	timew = NewMem(Widget, nsrc);

	label = XmVaCreateManagedLabel(dialog, "sourceHeader",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	sep = XmVaCreateManagedSeparator(dialog, "sep",
		XmNseparatorType, XmSINGLE_LINE,
		XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, label, XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET, XmNleftWidget, label, XmNleftOffset, 0,
		XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET, XmNrightWidget, label, XmNrightOffset, 0,
		NULL);

	rc1 = XmVaCreateRowColumn(dialog, "rc1",
		XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, sep,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	label = XmVaCreateManagedLabel(dialog, "lastModTime",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, rc1,
		NULL);

	sep = XmVaCreateManagedSeparator(dialog, "sep",
		XmNseparatorType, XmSINGLE_LINE,
		XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, label, XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET, XmNleftWidget, label, XmNleftOffset, 0,
		XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET, XmNrightWidget, label, XmNrightOffset, 0,
		NULL);

	rc2 = XmVaCreateRowColumn(dialog, "rc2",
		XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, sep,
		XmNleftAttachment, XmATTACH_WIDGET, XmNleftWidget, rc1,
		NULL);

	for(i = 0; i < nsrc; i++)
	{
		(void)XmVaCreateManagedLabel(rc1, SrcLabel(src[i]), NULL);
		timew[i] = XmVaCreateManagedLabel(rc2, "time", NULL);
	}

	XtManageChild(rc1);
	XtManageChild(rc2);

	display_status(NULL,NULL);
	activate_new_source_arrived_icon(NULL,NULL);
	XuShowDialog(dialog);
}


void InitGuidanceStatusSystem(void)
{
	int        i, nsrc;
	SourceList src;

	SourceListByType(SRC_FPA|SRC_NWP|SRC_ALLIED, FpaC_TIMEDEP_ANY, &src, &nsrc);
	last_stat_time = NewMem(time_t, nsrc);
	for(i = 0; i < nsrc; i++)
		last_stat_time[i] = src[i]->last_mod_time;
	AddSourceObserver(update_guidance_status,"GuidanceStatus");
}


void StopGuidanceArrivedIndicator(void)
{
	flash_stop = True;
}


/*================  LOCAL FUNCTIONS =================*/


/* Function called when a source update notify is received. The first
 * parameter is a boolean value indicating if we are to notify the user
 * of the update by flashing the book shelf indicator.
 */
static Boolean update_guidance_status(Boolean changed)
{
	display_status(NULL,NULL);
	if (changed) activate_new_source_arrived_icon(NULL,NULL);
	return True;
}


/* Function to control the flashing of the new source arrived indicator on the button bar.
*/
/*ARGSUSED*/
static void activate_new_source_arrived_icon(XtPointer cd, XtIntervalId *id)
{
	int     i;
	Boolean status_check   = False;
	Boolean called_by_loop = (id != (XtIntervalId*)0);

	static Pixel        bkgnd_pix   = 0;
	static Pixel        enhance_pix = 0;
	static Pixmap       books[7]    = {0,0,0,0,0,0,0};
	static XtIntervalId interval_id = (XtIntervalId)NULL;
	static int          loop_count  = 0;
	static int          icon_index  = 6;
	static Boolean      flash       = False;
	static Boolean      enhance     = False;
	static time_t       start_time  = 0;
	static Widget       update_btn  = NullWidget;

	/* Get the icon bar button widget that is to be animated.
	 */
	if(!update_btn) update_btn = GetIconBarWidget(GUIDANCE_STATUS_ICON);
	if(!update_btn) return;

	/* The last icon, books[6], is set to the default full bookshelf icon and is never freed. The others
	 * are assigned when the flashing starts and freed after it is finished.
	 */
	if (!books[6])
	{
		enhance_pix = XuLoadColorResource(update_btn, RNsourceUpdatingFlashColour,"green");
		XtVaGetValues(update_btn,
			XmNbackground, &bkgnd_pix,
			XmNlabelPixmap, &books[6],
			NULL);
	}

	/* Determine if a source status check is required. If the flashing has been going on for
	 * more than a minute force a check anyway.
	 */
	if(!called_by_loop && !flash)
	{
		status_check = True;
		start_time   = time((time_t*)0);
	}
	else if(!called_by_loop)
	{
		start_time = time((time_t*)0);
	}
	else if(loop_count > 120)
	{
		status_check = True;
	}

	/* The bookshelf should not flash if the dialog is active or if a
	 * stop command has been issued.
	 */
	if(dialog || flash_stop)
	{
		flash = False;
	}
	else if(status_check)
	{
		int        nsrc;
		SourceList src;
		time_t     sys_time = time((time_t*)0);

		/* Note that only certain sources activate the bookshelf */
		flash = False;
		loop_count = 0;
		SourceListByType(SRC_FPA|SRC_NWP|SRC_ALLIED, FpaC_TIMEDEP_ANY, &src, &nsrc);
		for(i = 0; i < nsrc; i++)
		{
			if(src[i]->last_mod_time - last_stat_time[i] >= GV_pref.updating_flash_delay)
			{
				if(sys_time - start_time <= GV_pref.updating_flash_time) flash = True;
			}
		}
	}

	if(flash)
	{
		/* Create the bookshelf pixmaps if they do not exist already
		 */
		if(!books[0])
		{
			for(i = 0; i < 6; i++)
			{
				char mbuf[30];
				snprintf(mbuf, 30, "bookshelf-%d", i);
				books[i] = XuGetPixmap(update_btn, mbuf);
			}
		}
		/*
		 * This logic ensures that the bookshelf is refreshed only within the time out
		 * loop and not when activated from functions within this file.
		 */
		if(called_by_loop)
		{
			loop_count++;
			enhance = !enhance;
			if(enhance) icon_index = (icon_index+1)%7;
			XtVaSetValues(update_btn,
				XmNbackground, enhance? enhance_pix:bkgnd_pix,
				XmNlabelPixmap, books[icon_index],
				NULL);
			interval_id = XtAppAddTimeOut(GV_app_context, 500, activate_new_source_arrived_icon, NULL);
		}
		else if(!interval_id)
		{
			interval_id = XtAppAddTimeOut(GV_app_context, 500, activate_new_source_arrived_icon, NULL);
		}
	}
	else
	{
		if (interval_id)
		{
			XtRemoveTimeOut(interval_id);
			XtVaSetValues(update_btn,
				XmNbackground, bkgnd_pix,
				XmNlabelPixmap, books[6],
				NULL);
		}
		interval_id = (XtIntervalId)NULL;
		icon_index  = 6;
		enhance     = False;
		flash_stop  = False;
		if(books[0])
		{
			for(i = 0; i < 6; i++)
			{
				XuFreePixmap(GW_mainWindow, books[i]);
				books[i] = 0;
			}
		}
	}
}



/* Display the status of the guidance sources in the dialog. This function is
 * a timeout so that if the status state was one of the special ones it does
 * an automatic recheck in 30 seconds to update the status as the source will
 * not retrigger it. Done because some users tend to leave this dialog up on
 * the screen for long periods of time.
 */
static void display_status(XtPointer unused, XtIntervalId *id)
{
	int        i, nsrc;
	time_t     sys_time;
	String     label;
	XmString   xmlabel;
	Pixel      fg, bg, efg, ebg;
	SourceList src;
	Boolean    recheck = False;

	static XtIntervalId sid = 0;

	if(!dialog) return;

	if( !id && sid ) XtRemoveTimeOut(sid);
	sid = 0;

	efg = XuLoadColorResource(dialog, RNguidanceArrivedFg,"black");
	ebg = XuLoadColorResource(dialog, RNguidanceArrivedBg,"green");

	sys_time = time((time_t*)0);
	SourceListByType(SRC_FPA|SRC_NWP|SRC_ALLIED, FpaC_TIMEDEP_ANY, &src, &nsrc);
	/*
	 * Cycle through all of the sources and display their status.
	 */
	for(i = 0; i < nsrc; i++)
	{
		XtVaGetValues(dialog,
			XmNbackground, &bg,
			XmNforeground, &fg,
			NULL);
		/*
		 * If the mod time was less than 80 seconds ago mark it as updating.
		 * 80 sec is used as the source checking code uses 60 seconds as a
		 * collection and notify limit.
		 */
		if((sys_time - src[i]->last_mod_time) < 80)
		{
			recheck = True;
			last_stat_time[i] = src[i]->last_mod_time;
			if(src[i]->isdata)
			{
				label = XuGetLabel("updating");
				fg = efg;
				bg = ebg;
			}
			else
			{
				label = XuGetLabel("na");
			}
		}
		/*
		 * If less then 300 seconds then show the time but outline it in
		 * the enhancement colours.
		 */
		else if((sys_time - src[i]->last_mod_time) < 300)
		{
			recheck = True;
			if(src[i]->isdata)
			{
				label = UnixSecToMinuteDateFormat(src[i]->last_mod_time);
				fg = efg;
				bg = ebg;
			}
			else
			{
				label = XuGetLabel("na");
			}
		}
		/*
		 * Display things normally.
		 */
		else
		{
			if(src[i]->isdata)
				label = UnixSecToMinuteDateFormat(src[i]->last_mod_time);
			else
				label = XuGetLabel("na");
		}

		xmlabel = XuNewXmString(label);
		XtVaSetValues(timew[i],
			XmNlabelString, xmlabel,
			XmNbackground, bg,
			XmNforeground, fg,
			NULL);
		XmStringFree(xmlabel);
	}
	if (recheck)
		sid = XtAppAddTimeOut(GV_app_context, 30000, display_status, NULL);
}


/*ARGSUSED*/
static void exit_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	FreeItem(timew);
	dialog = NullWidget;
}
