/*=========================================================================*/
/*
*      File: guidance_animationTab.c
*
*   Purpose: Tab contents for animating fields from the active list.
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
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Scale.h>
#include <Xm/ToggleB.h>
#include "observer.h"
#include "resourceDefines.h"
#include "guidance.h"
#include "selector.h"

#define MIN_DELAY	1	/* in miliseconds */
#define ALL_FIELDS	-1
#define ALL_TIMES	-2

#define STATE_ID	"gld"
#define GALS		"gals"	/* guidance animation loop speed */

/* Data to create time spacing buttons. */
struct TI {
	int      delta;	/* Mod of data time with this value must equal 0 */
	XmString label;	/* Label to display to the user */
	String   name;	/* Widget name */
};

/* Private variables */
static int        ntime_ndx        = 0;			/* number of elements in time index array */
static int       *time_ndx         = NULL;		/* Index into the GVG_valid_seq time array */
static Boolean    animating        = False;		/* Is animation active? */
static int        loop_delay       = MIN_DELAY; /* what the actual delay between frames is */
static int        loop_ndx         = 0;			/* current element in the loop */
static int        loop_start_ndx   = 0;			/* where in time_ndx to start animating */
static int        loop_end_ndx     = 0;			/* where in time_ndx to stop animation */
static int        ntime_intervals  = 2;			/* number of intervals in list */
static struct TI *time_intervals   = NULL;		/* list of time intervals between animation frames */
static int        time_interval    = ALL_FIELDS;/* What is the time interval between frames */
static int        in_select_btn    = -1;		/* Which sequence button was selected at first */
static Widget     speedControl     = NullWidget;/* The speed control widget */
static Widget     startBtn         = NullWidget;
static Widget     frameCount       = NullWidget;/* Displays how many frames available for animation */
static Widget     loopRange        = NullWidget;/* Set the animation range */
static int        nfld             = 0;			/* how many active fields are there */
static GuidanceFieldStruct **fld   = NULL;	/* the list of active fields */
/*
 * This data is for the first two selection buttons for the time intervals. These
 * are always present and the widget label is in the resource file.
 */
static struct TI time_intervals_default[] = {
	{ ALL_FIELDS, NULL, "allFields"},
	{ ALL_TIMES,  NULL, "allTimes" }
};


/* Forward function declarations */
static void animation_lockout_observer(String*, int);
static void animation_speed_cb        (Widget, XtPointer, XtPointer);
static void set_animate_interval      (void);
static void set_animation_time_window (TimeWindowSelectorStruct*);
static void start_stop_cb             (Widget, XtPointer, XtPointer);
static void time_interval_cb          (Widget, XtPointer, XtPointer);
static void T0_change_observer        (String*, int);


/* --------------------- Public Functions --------------------------*/

void LayoutGuidanceAnimationTab(void)
{
	int    n;
	Widget w, form, form1, form2, frame, rc, btn;

	static String module = "GuidanceAnimation";

	/* The following is done only once and held on to.
	 */
	if(!time_intervals)
	{
		int    val;
		char   buf[1000];
		String p1, p2;
		SETUP  *s;
		String intdef = {"1 Hour,60,3 Hours,180"};

		loop_delay = val = GV_animation_max_delay/2 + 1;
		if(XuVaStateDataGet(STATE_ID,GALS,NULL,"%d",&val) && val > 0 && val <= GV_animation_max_delay)
				loop_delay = val;

		/* Set the list of animation time steps. First set the defaults and then look for the
		 * rest in the setup file and the resource file.
		 */
		time_intervals  = NewMem(struct TI, ntime_intervals);
		(void) memcpy((void*) time_intervals, (void*) time_intervals_default, ntime_intervals*sizeof(struct TI));

		if((s = GetSetup(GUID_TIME_STEPS)) && s->nentry > 0)
		{
			for(n = 0; n < s->nentry; n++)
			{
				val = atoi(SetupParm(s,n,1));
				/* An interval > 10 days must be a parsing error! */
				if(val > 15000)
				{
					pr_error(module, "Parse error in %s block of setup file.\n", GUID_TIME_STEPS);
				}
				else
				{
					time_intervals = MoreMem(time_intervals, struct TI, ntime_intervals+1);
					time_intervals[ntime_intervals].delta = val;
					time_intervals[ntime_intervals].label = XmStringCreateLocalized(SetupParm(s,n,0));
					time_intervals[ntime_intervals].name  = "tlb";
					ntime_intervals++;
				}
			}
		}
		else
		{
			(void) strncpy(buf, XuGetStringResource(RNguidanceAnimationSteps,intdef), 1000);
			p1 = strtok(buf,",");
			while(p1)
			{
				/* An interval > 10 days must be a parsing error! */
				if(((p2 = strtok(NULL,",")) != NULL) && ((n = atoi(p2)) > 0) && (n < 15000))
				{
					time_intervals = MoreMem(time_intervals, struct TI, ntime_intervals+1);
					time_intervals[ntime_intervals].delta = n;
					time_intervals[ntime_intervals].label = XmStringCreateLocalized(p1);
					time_intervals[ntime_intervals].name  = "tlb";
					ntime_intervals++;
				}
				else
				{
					pr_error(module, "Parse error in %s line in resource file: %s\n",
							RNguidanceAnimationSteps, XuGetStringResource(RNguidanceAnimationSteps,NULL));
					break;
				}
				p1 = strtok(NULL,",");
			}
		}

		/* For animation lockout */
		AddObserver(OB_ANIMATION_RUNNING, animation_lockout_observer);
	}

	form1 = XmVaCreateForm(GVG_animateTab, "timeForm",
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 30,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 19,
		NULL);

	form2 = XmVaCreateForm(GVG_animateTab, "timeForm",
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 30,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, form1,
		NULL);

	frame = XmVaCreateManagedFrame(form1, "timeFrame",
		XmNshadowType, XmSHADOW_ETCHED_IN,
		XmNmarginWidth, 9,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	(void)XmVaCreateManagedLabel(frame, "timeInterval",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	rc = XmVaCreateManagedRowColumn(frame, "rc",
		XmNradioBehavior, True,
		NULL);

	for(n = 0; n < ntime_intervals; n++)
	{
		int ac = 0;
		Arg al[2];
		XtSetArg(al[ac], XmNlabelString, time_intervals[n].label);
		if (time_intervals[n].label) ac++;
		XtSetArg(al[ac], XmNset, (time_intervals[n].delta == time_interval)); ac++;
		btn = XmCreateToggleButton(rc, time_intervals[n].name, al, ac);
		XtAddCallback(btn, XmNvalueChangedCallback, time_interval_cb, INT2PTR(time_intervals[n].delta));
		XtManageChild(btn);
	}

	frameCount = XmVaCreateManagedLabel(form1, "frameCount",
		XmNborderWidth, 1,
		XmNmarginHeight, 9,
		XmNmarginWidth, 9,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, frame,
		XmNtopOffset, 20,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	loopRange = CreateTimeWindowSelector(form2,
		XmMAX_ON_RIGHT, set_animation_time_window,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	frame = XmVaCreateManagedFrame(form2, "timeFrame",
		XmNshadowType, XmSHADOW_ETCHED_IN,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, loopRange,
		XmNtopOffset, 16,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	(void)XmVaCreateManagedLabel(frame, "speedLabel",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	form = XmVaCreateManagedForm(frame, "form",
		XmNhorizontalSpacing, 5,
		XmNverticalSpacing, 5,
		NULL);

	speedControl = XmVaCreateManagedScale(form, "speedControl",
		XmNorientation, XmHORIZONTAL,
		XmNprocessingDirection, XmMAX_ON_LEFT,
		XmNresizable, False,
		XmNminimum, MIN_DELAY,
		XmNmaximum, GV_animation_max_delay,
		XmNvalue, loop_delay,
		XmNwidth, 200,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(speedControl, XmNvalueChangedCallback, animation_speed_cb, NULL);

	w = XmVaCreateManagedLabel(form, "minLabel",
		XmNresizable, False,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, speedControl,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	(void) XmVaCreateManagedLabel(form, "maxLabel",
		XmNresizable, False,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, speedControl,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	startBtn = XmVaCreateManagedPushButton(GVG_animateTab, "startBtn",
		XmNmarginHeight, 6,
		XmNwidth, 150,
		XmNresizable, False,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopOffset, 30,
		XmNtopWidget, form1,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, form1,
		XmNleftOffset, -75,
		NULL);
	XtAddCallback(startBtn, XmNactivateCallback, start_stop_cb, NULL);

	XtManageChild(form1);
	XtManageChild(form2);
}


void ActivateGuidanceAnimationTab(void)
{
	int i, n;

	/* Create a list of the selected fields */
	for(n = 0; n < GVG_nguidlist; n++)
	{
		for(i = 0; i < GVG_guidlist[n]->nfield; i++)
		{
			if(GVG_guidlist[n]->field[i]->show)
			{
				fld = MoreMem(fld, GuidanceFieldStruct*, nfld+1);
				fld[nfld++] = GVG_guidlist[n]->field[i];
			}
		}
	}

	/* Create the animation valid time list */
	set_animate_interval();
	
	/* We need to know if the T0 depiction time is modified */
	AddObserver(OB_DEPICTION_TZERO_CHANGE, T0_change_observer);
}


void DeactivateGuidanceAnimationTab(void)
{
	DeleteObserver(OB_DEPICTION_TZERO_CHANGE, T0_change_observer);
	if (animating) start_stop_cb(NullWidget, NULL, NULL);
	ntime_ndx = 0;
	FreeItem(time_ndx);
	nfld = 0;
	FreeItem(fld);
}



/************************** LOCAL FUNCTIONS ************************/


/*ARGSUSED*/
static void T0_change_observer(String *parms, int nparms)
{
	set_animate_interval();
}


/* Fill in the animation time sequence. The fill will depend on the
 * time interval chosen. If all fields then the interval will be set
 * such that every selected fields must be available for each frame.
 * If all times then all of the times are shown regardless of the
 * field availability. The rest are set at a set time interval based
 * on the current T0 - every 10 minutes would mean that the mod of
 * the time in minutes with 10 must be 0.
 */
static void set_animate_interval(void)
{
	int      n;
	XmString xmlabel;
	Pixel    fg;
	char     buf[100];
	String  *times;

	ntime_ndx = 0;
	if (!time_ndx) time_ndx = NewMem(int, GVG_nvalid_seq);

	if(time_interval == ALL_FIELDS)
	{
		int i;
		for(n = 0; n < GVG_nvalid_seq; n++)
		{
			Boolean all_in = True;
			for(i = 0; i < nfld; i++)
			{
				if(InTimeList(GVG_valid_seq[n], fld[i]->valid->times, fld[i]->valid->ntimes, NULL)) continue;
				all_in = False;
				break;
			}
			if(all_in) time_ndx[ntime_ndx++] = n;
		}
	}
	else if(time_interval == ALL_TIMES)
	{
		ntime_ndx = GVG_nvalid_seq;
		for(n = 0; n < GVG_nvalid_seq; n++)
			time_ndx[n] = n;
	}
	else
	{
		for(n = 0; n < GVG_nvalid_seq; n++)
		{
			int delta = MinuteDif(GV_T0_depict, GVG_valid_seq[n]);
			if(delta%time_interval == 0)
				time_ndx[ntime_ndx++] = n;
		}
	}

	(void) snprintf(buf, 100, "%3d %s", ntime_ndx, XuGetStringResource(".animationFrameCountLabel",NULL));
	xmlabel = XmStringCreateLocalized(buf);
	XtVaGetValues(GVG_animateTab, XmNforeground, &fg, NULL);
	if(ntime_ndx < 1)
		fg = XuLoadColor(GVG_animateTab, "red");
	XtVaSetValues(frameCount,
		XmNlabelString, xmlabel,
		XmNborderColor, fg,
		NULL);
	XmStringFree(xmlabel);

	loop_start_ndx = 0;
	loop_end_ndx   = ntime_ndx-1;

	times = NewMem(String, ntime_ndx);
	for(n = 0; n < ntime_ndx; n++)
		times[n] = GVG_valid_seq[time_ndx[n]];

	SetTimeWindowLimits(loopRange, times, ntime_ndx, loop_start_ndx, loop_end_ndx);

	FreeItem(times);
}


static void set_animation_time_window( TimeWindowSelectorStruct *tw )
{
	if(tw->start_ndx >= 0 && tw->start_ndx < ntime_ndx)
		loop_start_ndx = tw->start_ndx;

	if(tw->end_ndx >= 0 && tw->end_ndx < ntime_ndx)
		loop_end_ndx = tw->end_ndx;
}



/*	The slider returns a value from MIN_DELAY to GV_animation_max_delay.
 */
/*ARGSUSED*/
static void animation_speed_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	loop_delay = ((XmScaleCallbackStruct *) call_data)->value;
	XuVaStateDataSave(STATE_ID,GALS,NULL,"%d", loop_delay);
}


/* This is where the commands are sent to Ingred that create the animation, toggle
 * the buttons and change the time in the legend.
 */
static void animation_loop(XtPointer data, XtIntervalId *id)
{
	int i, last;

	if (!animating) return;

	/*
	 * Show the fields.
	 */
	for(i = 0; i < nfld; i++)
	{
		(void) IngredVaCommand(GE_GUIDANCE, "FIELD_VISIBILITY %s %s ON", fld[i]->id, GVG_valid_seq[time_ndx[loop_ndx]]);
	}
	(void) IngredCommand(GE_GUIDANCE, "SHOW");
	/*
	 * Toggle the sequence buttons so that they display the time of the fields being displayed.
	 */
	last = loop_ndx - 1;
	if(last < loop_start_ndx) last = loop_end_ndx;
	XmToggleButtonSetState(GVG_btnBarBtns[time_ndx[last]], False, False);
	XmToggleButtonSetState(GVG_btnBarBtns[time_ndx[loop_ndx]], True, False);
	/*
	 * Same for the guidance legend box.
	 */
	GuidanceLegendDialogSetValidTime(GVG_valid_seq[time_ndx[loop_ndx]]);
	/*
	 * Increment the loop and put ourselves back into the time queue.
	 */
	if(loop_ndx >= loop_end_ndx)
	{
		loop_ndx = loop_start_ndx;
		(void) XtAppAddTimeOut(GV_app_context, (unsigned long) GV_animation_loop_delay, animation_loop, NULL);
	}
	else
	{
		loop_ndx++;
		(void) XtAppAddTimeOut(GV_app_context, (unsigned long) loop_delay, animation_loop, NULL);
	}
}



/* Callback for the start stop button.
 */
/*ARGSUSED*/
static void start_stop_cb(Widget w, XtPointer client_data, XtPointer unused)
{
	int n;
	XmString xmlabel = NULL;
	Pixel fg, bg;
	String parms[2];

	if(!animating)
	{
		loop_ndx = loop_start_ndx;
		if(ntime_ndx > 1)
		{
			DeactivatePanels();

			/* Set it so no other parts of the program can change the legend */
			(void) GuidanceLegendDialogActivationState(False);

			animating = True;
			AllowIngredBusyCursor(False);
			xmlabel = XmStringCreateLocalized(XuGetStringResource(RNstopBtn,"Stop"));
			bg = XuLoadColorResource(GVG_animateTab, RNselectBgColor, "ForestGreen");
			fg = XuLoadColorResource(GVG_animateTab, RNselectFgColor, "black");
			XtVaSetValues(startBtn, XmNlabelString, xmlabel, XmNbackground, bg, XmNforeground, fg, NULL);

			/* Save the currently selected sequence button */
			in_select_btn = -1;
			for(n = 0; n < GVG_nvalid_seq; n++)
			{
				if(!XmToggleButtonGetState(GVG_btnBarBtns[n])) continue;
				in_select_btn = n;
				break;
			}
			if(in_select_btn >= 0)
				XmToggleButtonSetState(GVG_btnBarBtns[in_select_btn], False, False);

			/* Start loop */
			animation_loop(NULL, NULL);
		}
	}
	else
	{
		ActivatePanels();
		animating = False;
		AllowIngredBusyCursor(True);
		xmlabel = XmStringCreateLocalized(XuGetStringResource(RNstopBtn,"Start"));
		XtVaGetValues(GVG_animateTab, XmNbackground, &bg, XmNforeground, &fg, NULL);
		XtVaSetValues(startBtn, XmNlabelString, xmlabel, XmNbackground, bg, XmNforeground, fg, NULL);

		/* Restore the sequence button state */
		for(n = 0; n < GVG_nvalid_seq; n++)
			XmToggleButtonSetState(GVG_btnBarBtns[n], False, False);

		(void) GuidanceLegendDialogActivationState(True);
		if(in_select_btn >= 0)
		{
			/* A sequence button was selected on entry so reset. */
			XuToggleButtonSet(GVG_btnBarBtns[in_select_btn], True, True);
		}
		else
		{
			/* If any fields did not have a valid field time on entry turn them off now. */
			String dt;
			int last = loop_ndx - 1;
			if(last < loop_start_ndx) last = loop_end_ndx;
			dt = GVG_valid_seq[time_ndx[last]];
			SetGuidanceDisplayState(True);
			for(n = 0; n < nfld; n++)
			{
				if(fld[n]->vsel < 0)
					(void) IngredVaCommand(GE_GUIDANCE, "FIELD_VISIBILITY %s %s OFF", fld[n]->id, dt);
			}
		}
		(void) IngredCommand(GE_GUIDANCE, "SHOW");
		ACTIVATE_guidanceLegendDialog(GW_mainWindow);
	}
	if (xmlabel) XmStringFree(xmlabel);

	parms[0] = OB_KEY_GUIDANCE;
	parms[1] = (animating)? OB_KEY_ON:OB_KEY_OFF;
	NotifyObservers(OB_ANIMATION_RUNNING, parms, 2);
}


static void animation_lockout_observer(String *parms, int nparms)
{
	if(GVG_selectDialog != NULL && nparms == 2 && !same(parms[0],OB_KEY_GUIDANCE))
		XtSetSensitive(startBtn, !same(parms[1],OB_KEY_ON));
}



/* Callback for the animation interval row column toggle buttons.
 */
/*ARGSUSED*/
static void time_interval_cb(Widget w, XtPointer client_data, XtPointer unused)
{
	if(XmToggleButtonGetState(w))
	{
		time_interval = PTR2INT(client_data);
		set_animate_interval();
	}
}
