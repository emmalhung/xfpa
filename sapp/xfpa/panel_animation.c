/*****************************************************************************/
/*
*  File:     panel_animation.c
*  
*  Purpose:  Controls the animation of the depiction sequence and the 
*            interpolated depictions.
*
*  Notes:    This control panel sits to the side of the map display and is
*            visible continuously while active. A scale widget is placed at
*            the bottom of the manager widget of the drawing window and used
*            to show the relative position of the visible depiction in the
*            sequence. 
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

#include "global.h"
#include <ingred.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Scale.h>
#include <Xm/ScrolledW.h>
#include <Xm/ToggleB.h>
#include "depiction.h"
#include "editor.h"
#include "observer.h"
#include "resourceDefines.h"
#include "selector.h"

#define MAX_TIME_LABELS	17		/* max number of animation scale labels (set for appearance) */
#define STATE_ID		"apsd"	/* animation panel state data for XuVaStateDataSave and Get */
#define LS				"ls"	/* animation panel loop speed for XuVaStateDataSave and Get */

static void animation_speed_cb  (Widget, XtPointer, XtPointer);
static void animation_status_observer (CAL, String*, int);
static void elem_select_cb		(Widget, XtPointer, XtPointer);
static void position_display_cb	(Widget, XtPointer, XtPointer);
static void source_cb			(Widget, XtPointer, XtPointer);
static void start_stop_cb		(Widget, XtPointer, XtPointer);
static void time_window_cb		(TimeWindowSelectorStruct*);

static int     scale_max = 0, scale_min = 0;
static int     nbtns = 15;
static int     animation_loop_speed = 1;
static TSTAMP  display_time;
static Widget  *btns = NULL;
static Widget  animationScale = NULL;
static TSTAMP  scale_set_time;
static TSTAMP  actual_set_time;
static Boolean scaleRespond = True;
static Boolean animation_active = False;
static Widget  sourceSelect;
static Widget  fieldSelect;
static Widget  srcDepict;
static Widget  srcInterp;
static Widget  startBtn;
static Widget  scaleLabels[MAX_TIME_LABELS];

/* Static variables for the animation window limit object */
static Widget  timeWindow;
static int     time_window_depict_count        = 0;
static int     time_window_depict_start        = 0;
static int     time_window_depict_end          = 0;
static int     time_window_interp_start        = 0;
static int     time_window_interp_end          = 0;
static TSTAMP  time_window_sequence_start_time = "";
static TSTAMP  time_window_sequence_end_time   = "";


/*=========================================================================*/
/*
*	CreateAnimationPanel() - Creats the widgets in the animation control
*	panel.
*/
/*=========================================================================*/
void CreateAnimationPanel(Widget parent)
{
	int	   i, val;
	Widget w, fieldSelectSW, form, frame, speedControl;

	/* The max animation delay in milliseconds is in the resource file. It is set
	 * here but used in every animation loop system in the program.
	*/
	GV_animation_max_delay  = XuGetIntResource(RNanimationMax,GV_animation_max_delay);
	GV_animation_loop_delay = XuGetIntResource(RNanimationLoopDelay, GV_animation_loop_delay);

	/* Make sure that the animation delays are set to something reasonable.
	 */
	GV_animation_max_delay = MAX(GV_animation_max_delay, 250);
	if(GV_animation_loop_delay <= GV_animation_max_delay)
		GV_animation_loop_delay = GV_animation_max_delay * 2;

	animation_loop_speed = GV_animation_max_delay/2 + 1;
	if(XuVaStateDataGet(STATE_ID,LS,NULL,"%d",&val) && val > 0 && val <= GV_animation_max_delay)
		animation_loop_speed = val;

	GW_animationPanel = parent;

	frame = XmVaCreateFrame(GW_animationPanel, "frame",
		XmNresizable, False,
		XmNtopAttachment, XmATTACH_FORM, XmNtopOffset, 5,
		XmNleftAttachment, XmATTACH_FORM, XmNleftOffset, 5,
		XmNrightAttachment, XmATTACH_FORM, XmNrightOffset, 5,
		NULL);

	(void) XmVaCreateManagedLabel(frame, "sourceLabel",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	sourceSelect = XmVaCreateManagedRowColumn(frame, "sourceSelect",
		XmNborderWidth, 0,
		XmNradioBehavior, True,
		NULL);

	srcDepict = XmVaCreateManagedToggleButton(sourceSelect, "depictBtn",
		XmNset, XmSET,
		NULL);
	XtAddCallback(srcDepict, XmNvalueChangedCallback, source_cb, NULL);

	srcInterp = XmVaCreateManagedToggleButton(sourceSelect, "interpBtn", NULL);
	XtAddCallback(srcInterp, XmNvalueChangedCallback, source_cb, NULL);

	XtManageChild(frame);

	startBtn = XmVaCreateManagedPushButton(GW_animationPanel, "startBtn",
		XmNmarginHeight, 6,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 9,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 9,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNbottomOffset, 9,
		NULL);
	XtAddCallback(startBtn, XmNactivateCallback, start_stop_cb, NULL);

	w = XmVaCreateManagedLabel(GW_animationPanel, "minLabel",
		XmNresizable, False,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 9,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, startBtn,
		XmNbottomOffset, 15,
		NULL);

	speedControl = XmVaCreateManagedScale(GW_animationPanel, "speedControl",
		XmNorientation, XmHORIZONTAL,
		XmNprocessingDirection, XmMAX_ON_LEFT,
		XmNresizable, False,
		XmNminimum, 1,
		XmNmaximum, GV_animation_max_delay,
		XmNvalue, animation_loop_speed,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 9,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 9,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, w,
		XmNbottomOffset, 0,
		NULL);
	XtAddCallback(speedControl,XmNvalueChangedCallback,animation_speed_cb,NULL);

	(void) XmVaCreateManagedLabel(GW_animationPanel, "maxLabel",
		XmNresizable, False,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, speedControl,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 9,
		NULL);

	w = XmVaCreateManagedLabel(GW_animationPanel, "speedLabel",
		XmNresizable, False,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 9,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, speedControl,
		XmNbottomOffset, 0,
		NULL);

	timeWindow = CreateTimeWindowSelector(GW_animationPanel, XmMAX_ON_RIGHT, time_window_cb,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 5,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 5,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, w,
		XmNbottomOffset, 15,
		NULL);

	fieldSelectSW = XmVaCreateManagedScrolledWindow(GW_animationPanel, "fieldSelectSW",
		XmNscrollBarDisplayPolicy, XmAS_NEEDED,
		XmNscrollingPolicy, XmAUTOMATIC,
		XmNresizable, False,
		XmNtopAttachment, XmATTACH_WIDGET, XmNtopOffset, 20, XmNtopWidget, frame,
		XmNleftAttachment, XmATTACH_FORM, XmNleftOffset, 5,
		XmNrightAttachment, XmATTACH_FORM, XmNrightOffset, 5,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, timeWindow,
		XmNbottomOffset, 15,
		NULL);

	fieldSelect = XmVaCreateManagedRowColumn(fieldSelectSW, "fieldSelect",
		XmNpacking, XmPACK_COLUMN,
		NULL);

	btns = NewWidgetArray(nbtns);
	for(i = 0; i < nbtns; i++)
	{
		btns[i] = XmCreateToggleButton(fieldSelect, "btn", NULL, 0);
		XtAddCallback(btns[i], XmNvalueChangedCallback, elem_select_cb, INT2PTR(i));
	}

	XtManageChild(GW_animationPanel);

	AddIngredObserver(animation_status_observer);
	
	/* Create the scale above the main map that displays the animation process with
	 * time labels. Attaching the animation scale directly to a frame type like the
	 * GW_animationScaleManager results in some strange looking output because of
	 * height expansion. Thus the form within the frame.
	 */
	form = XmCreateForm(GW_animationScaleManager, "animationForm", NULL, 0);

	animationScale = XmVaCreateManagedScale(form, "animationScale",
		XmNorientation,     XmHORIZONTAL,
		XmNtopAttachment,   XmATTACH_FORM, XmNtopOffset,   0,
		XmNrightAttachment, XmATTACH_FORM, XmNrightOffset, 0,
		XmNleftAttachment,  XmATTACH_FORM, XmNleftOffset,  0,
		NULL);

	XtAddCallback(animationScale, XmNvalueChangedCallback, position_display_cb, NULL);
	XtAddCallback(animationScale, XmNdragCallback, position_display_cb, NULL);

	for(i = 0; i < MAX_TIME_LABELS; i++)
	{
		char buf[16];
		(void) snprintf(buf, 16, "sl%d", i);
		scaleLabels[i] = XmVaCreateManagedLabel(animationScale, buf,
			XmNmarginHeight, 0,
			XmNmarginWidth, 0,
			NULL);
	}

	XtManageChild(form);
}

/*=========================================================================*/
/*
*	Starts up the animation panel for the current depiction set and time
*	settings.
*/
/*=========================================================================*/
Boolean AnimationStartup(void)
{
	int       i, nfield, nstep, nhour, start_min, del, scale_val, scale_del;
	int       slider_display;
	Dimension margin_bottom;
	char      mbuf[300];
	Boolean   depict_selected;
	XmString  label;

	static Boolean first = True;

	(void) strcpy(scale_set_time, "");
	(void) strcpy(actual_set_time, "");

	/* These can be insensitive if the exit was done by one of the
	 * zoom type controls.
	 */
	XtSetSensitive(sourceSelect, True);
	XtSetSensitive(fieldSelect, True);

	(void) IngredCommand(GE_ANIMATE, "ENTER");

	/* Make sure there are at least 2 depictions available.
	*/
	(void) GEStatus("DEPICT TIMES", &i, NULL, NULL, NULL);
	if( i < 2 )
	{
		XuShowError(GW_mainWindow, "AnimationMinNr", NULL);
		return False;
	}

	/* Update the animation progress indicator depending on the database type. If
	 * the database is in minutes we do not want to display a value above the slider.
	 * In this case the bottom margin of the labels needs to be adjusted to compensate
	 * for the loss of the slider value label.
	 */
	if(minutes_in_depictions())
	{
		scale_min = MinuteDif(GV_T0_depict, GV_depict[0]);
		scale_max = MinuteDif(GV_T0_depict, GV_depict[GV_ndepict-1]);
		scale_val = MinuteDif(GV_T0_depict, ActiveDepictionTime(FIELD_INDEPENDENT));
		scale_del = GV_interp_time_delta;
		slider_display = XmNONE;
		margin_bottom = 9;
	}
	else
	{
		scale_min = HourDif(GV_T0_depict, GV_depict[0]);
		scale_max = HourDif(GV_T0_depict, GV_depict[GV_ndepict-1]);
		scale_val = HourDif(GV_T0_depict, ActiveDepictionTime(FIELD_INDEPENDENT));
		scale_del = GV_interp_time_delta/60;
		slider_display = XmNEAR_SLIDER;
		margin_bottom = 0;
	}

	XtVaSetValues(animationScale,
		XmNminimum,       scale_min,
		XmNmaximum,       scale_max,
		XmNvalue,         scale_val,
		XmNscaleMultiple, scale_del,
		XmNshowValue,     slider_display,
		NULL);

	/* The labels across the top of the animation scale are in hours. Determine
	 * how many labels to show. If more hours than MAX_TIME_LABELS get a division
	 * that is within 10 minutes.
	 */
	nstep = nhour = HourDif(GV_depict[0], GV_depict[GV_ndepict-1]);
	if(nstep > MAX_TIME_LABELS)
	{
		for(nstep = MAX_TIME_LABELS; nstep > 1; nstep--)
			if( fabsf(((float)(nhour/(nstep-1)))-((float)nhour/(float)(nstep-1))) < 0.17 ) break;
	}
	/*
	 * The labels are evenly spaced across the widget, thus the times need to
	 * rounded to the nearest hour.
	 */
	XtUnmanageChildren(scaleLabels, MAX_TIME_LABELS);
	start_min = MinuteDif(GV_T0_depict, GV_depict[0]);
	del = MinuteDif(GV_depict[0],GV_depict[GV_ndepict-1]);
	for(i = 0; i < nstep; i++)
	{
		int val = start_min + del * i / (nstep-1);
		(void) snprintf(mbuf, sizeof(mbuf), "%d", ((val<0)?(val-30):(val+30))/60); 
		label = XmStringCreateLocalized(mbuf);
		XtVaSetValues(scaleLabels[i],
			XmNlabelString, label,
			XmNmarginBottom, margin_bottom,
			NULL);
		XmStringFree(label);
	}
	XtManageChildren(scaleLabels, nstep);
	XtManageChild(GW_animationScaleManager);

	/* Label the element selection buttons and create them as required.
	*/
	XtUnmanageChildren(btns, (Cardinal) nbtns);
	for( i = 0; i < GV_nfield; i++ )
	{
		if(i >= nbtns)
		{
			nbtns++;
			btns = MoreWidgetArray(btns,nbtns);
			btns[i] = XmCreateToggleButton(fieldSelect, "btn", NULL, 0);
			XtAddCallback(btns[i], XmNvalueChangedCallback, elem_select_cb, INT2PTR(i));
		}

		if(first)
		{
			/* The initial visibility state will be that of the depiction fields.
			*/
			GV_field[i]->animate_vis =	( GV_field[i]->visible == VIS_ALWAYS_ON ||
					  						( GV_field[i]->visible == VIS_ON &&
						  						(	GV_field[i]->group->visible == VIS_ON ||
													GV_field[i]->group == GV_active_group
						  					 	)
					  					 	)
										);
		}

		/* Set animation visibility state
		 */
		(void) snprintf(mbuf, sizeof(mbuf), "FIELD_VISIBILITY %s %s %s",
			GV_field[i]->info->element->name,
			GV_field[i]->info->level->name,
			GV_field[i]->animate_vis ? "ON" : "OFF");
		(void) IngredCommand(GE_ANIMATE, mbuf);

		label = XuNewXmString(GV_field[i]->info->sh_label);
		XtVaSetValues(btns[i],
			XmNset, GV_field[i]->animate_vis,
			XmNlabelString, label,
			NULL);
		XmStringFree(label);
	}

	/* Determine if the time window limits should be reset */
	if( !matching_tstamps(time_window_sequence_start_time, GV_depict[0]           ) ||
		!matching_tstamps(time_window_sequence_end_time,   GV_depict[GV_ndepict-1]) ||
		time_window_depict_count != GV_ndepict                                        )
	{
		time_window_depict_count  = GV_ndepict;
		time_window_depict_start  = 0;
		time_window_depict_end    = GV_ndepict - 1;
		time_window_interp_start  = 0;
		time_window_interp_end    = 1000000;

		strcpy(time_window_sequence_start_time, GV_depict[0]);
		strcpy(time_window_sequence_end_time,   GV_depict[GV_ndepict-1]);
	}

	/* Toggle the active source button to set up for animation.
	*  If there are no interpolations ghost out the interp button.
	*/
	(void) GEStatus("FIELDS INTERPOLATED", &nfield, NULL, NULL, NULL);
	depict_selected = (XmToggleButtonGetState(srcDepict) || nfield < 1);

	if(depict_selected)
		XuToggleButtonSet(srcDepict, True, True );
	else
		XuToggleButtonSet(srcInterp, True, True );

	XtSetSensitive(srcInterp, (nfield > 0));

	/* Set the between depiction time delay */
    (void) IngredVaCommand(GE_ANIMATE, "DELAY %d", animation_loop_speed);

	first = False;
	return True;
}


/*=========================================================================*/
/*
*	AnimationStep() - Called to step through the animation sequence. The
*                     incrememt indicates the direction of travel.
*/
/*=========================================================================*/
void AnimationStep(int increment, int increment_delta)
{
	if(animation_active) start_stop_cb(NullWidget, NULL, NULL);

	if(increment_delta > 0)
	{
		int    del = increment_delta;
		TSTAMP dt;

		/* the increment delta can not be less than the interp delta for
		 * the following logic to work.
		 */
		if( del < GV_interp_time_delta) del = GV_interp_time_delta;
		strcpy(dt, calc_valid_time_minutes(display_time, 0, del * increment));
		(void) IngredVaCommand(GE_ANIMATE, "SHOW %s", dt);
	}
	else
	{
		(void) IngredCommand(GE_ANIMATE, (increment >= 0)? "SHOW NEXT":"SHOW PREV");
	}
}


/*=========================================================================*/
/*
*	AnimationExit() - Exit the animation panel. If the cmd parameter is
*	                  E_ZOOM then this function has been called by the zoom
*   functions and we know that we will be re-entering the animation panel
*   once the user has selected the area or terminates the pan. Thus restrict
*   what things we do so as to preserve the current state of the panel.
*/
/*=========================================================================*/
void AnimationExit(String cmd)
{
	if(animation_active)
		start_stop_cb(NullWidget, NULL, NULL);

	if(same(cmd,E_ZOOM))
	{
		XtSetSensitive(sourceSelect, False);
		XtSetSensitive(fieldSelect, False);
	}
	else
	{
		XtUnmanageChild(GW_animationScaleManager);
		SetDepictionTimeDisplay(ActiveDepictionTime(FIELD_INDEPENDENT));
		SetTimeWindowLimits(timeWindow, NULL, 0, 0, 0);
		(void) IngredCommand(GE_ANIMATE, cmd);
	}
}


/*===================== LOCAL FUNCTIONS FOLLOW ============================*/



/* Process calls from Ingred that give the time of the of the depiction
 * currently visible in the animation.
*/
static void animation_status_observer(CAL cal, String *parms, int nparms)
{
	if(!same_ic(parms[0], "ANIMATION")) return;

	if(scaleRespond && same_ic(parms[1],"SHOWING") && NotNull(parms[2]))
	{
		int deltime;
		if(valid_tstamp(parms[2]))
		{
			(void) strcpy(display_time, parms[2]);	
			if(minutes_in_depictions())
				deltime = MinuteDif(GV_T0_depict, display_time);
			else
				deltime = HourDif(GV_T0_depict, display_time);
			if(deltime >= scale_min && deltime <= scale_max)
			{
				XmScaleSetValue(animationScale, deltime);
				SetDepictionTimeDisplay(display_time);
				(void) strcpy(scale_set_time, display_time);
			}
		}
	}
}


/*=========================================================================*/
/*
*	source_cb() - Select the source of the animation.  This will be either
*	the depiction sequence of the interpolated depictions if available.
*	Note that the list of fields shown on the animation panel may be 
*	different between the sequence and the interpolations.
*/
/*=========================================================================*/
/*ARGSUSED*/
static void source_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
    int      i, nfields, nlist;
    String   *elem, *level;
	Widget   *list;
	Boolean  ok;


	if(!XmToggleButtonGetState(w)) return;

	XtUnmanageChildren(btns, (Cardinal) nbtns);

    if(w == srcDepict)
    {
		if(valid_tstamp(scale_set_time))
		{
			ok = ((i = closest_tstamp(GV_ndepict, GV_depict, scale_set_time, 0, NULL, NULL, NULL, NULL)) >=0);
			(void) strcpy(actual_set_time, ok? GV_depict[i] : "");
		}
        (void) GEStatus("FIELDS", &nfields, &elem, &level, NULL);
		SetTimeWindowLimits(timeWindow, GV_depict, GV_ndepict, time_window_depict_start, time_window_depict_end);
        (void) IngredVaCommand(GE_ANIMATE, "MODE DEPICT %s %s",
				GV_depict[time_window_depict_start], GV_depict[time_window_depict_end]);
		if(ok) (void) IngredVaCommand(GE_ANIMATE, "SHOW %s", actual_set_time);
    }
    else
    {
		int     ntimes = 0;
		String  *times = NULL;
		TSTAMP  start, end;

		(void) GEStatus("INTERP TIMES", &ntimes, &times, NULL, NULL);
		if(time_window_interp_end >= ntimes)
		{
			time_window_interp_start = 0;
			time_window_interp_end   = ntimes - 1;
		}
		SetTimeWindowLimits(timeWindow, times, ntimes, time_window_interp_start, time_window_interp_end);
		strcpy(start, times[time_window_interp_start]);
		strcpy(end,   times[time_window_interp_end]  );
		if(valid_tstamp(scale_set_time))
		{
			ok = ((i = closest_tstamp(ntimes, times, scale_set_time, 0, NULL, NULL, NULL, NULL)) >= 0);
			(void) strcpy(actual_set_time, ok? times[i] : "");
		}
        (void) GEStatus("FIELDS INTERPOLATED", &nfields, &elem, &level, NULL);
        (void) IngredVaCommand(GE_ANIMATE, "MODE INTERP %s %s", start, end);
		if(ok) (void) IngredVaCommand(GE_ANIMATE, "SHOW %s", actual_set_time);
    }

	list = NewWidgetArray(GV_nfield);
	nlist = 0;
    for(i = 0; i < GV_nfield; i++)
    {
        if(!InFieldList(GV_field[i], nfields, elem, level, NULL)) continue;
		list[nlist] = btns[i];
		nlist++;
    }
	XtManageChildren(list, (Cardinal) nlist);
	FreeItem(list);
}

/*=========================================================================*/
/*
*	elem_select_cb() - Responds to the display state requests for the 
*	fields.
*/
/*=========================================================================*/
/*ARGSUSED*/
static void elem_select_cb(Widget w, XtPointer client_data, XtPointer unused)
{
	int  elem_no;
	char mbuf[300];

	elem_no = PTR2INT(client_data);
	GV_field[elem_no]->animate_vis = XmToggleButtonGetState(w);

	(void) snprintf(mbuf, sizeof(mbuf), "FIELD_VISIBILITY %s %s %s",
		GV_field[elem_no]->info->element->name,
		GV_field[elem_no]->info->level->name,
		GV_field[elem_no]->animate_vis ? "ON" : "OFF");
	(void) IngredCommand(GE_ANIMATE, mbuf);
}

/*=========================================================================*/
/*
*	animation_speed_cb() - The slider returns a value from 0 to 100.  This is
*	converted into the delay time, in milliseconds, between depictions.
*	Note that 0 corresponds to 1 and 100 to GV_animation_max_delay.
*/
/*=========================================================================*/
/*ARGSUSED*/
static void animation_speed_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	animation_loop_speed = ((XmScaleCallbackStruct *) call_data)->value;
    (void) IngredVaCommand(GE_ANIMATE, "DELAY %d", animation_loop_speed);
	XuVaStateDataSave(STATE_ID,LS,NULL,"%d",animation_loop_speed);
}


/*=========================================================================*/
/*
 *   time_window_cb() - Set the time limits of the animation.
 */
/*=========================================================================*/
static void time_window_cb( TimeWindowSelectorStruct *tw )
{
    if(XmToggleButtonGetState(srcDepict))
    {
		time_window_depict_start = tw->start_ndx;
		time_window_depict_end   = tw->end_ndx;
        (void) IngredVaCommand(GE_ANIMATE, "MODE DEPICT %s %s", tw->start_time, tw->end_time);
    }
    else
    {
		time_window_interp_start = tw->start_ndx;
		time_window_interp_end   = tw->end_ndx;
        (void) IngredVaCommand(GE_ANIMATE, "MODE INTERP %s %s", tw->start_time, tw->end_time);
    }
}


/*=========================================================================*/
/*
*	start_stop_cb() - The slider returns a value from 0 to 100.  This is
*	converted into the delay time, in milliseconds, between depictions.
*	Note that 0 corresponds to 1 and 100 to GV_animation_max_delay.
*/
/*=========================================================================*/
/*ARGSUSED*/
static void start_stop_cb(Widget w, XtPointer client_data, XtPointer unused)
{
	String parms[2];

	if(!animation_active)
	{
		animation_active = True;
		XuWidgetLabel(startBtn, XuGetStringResource(RNstopBtn,"Stop"));
		(void) IngredCommand(GE_ANIMATE, "START");
	}
	else
	{
		animation_active = False;
		XuWidgetLabel(startBtn, XuGetStringResource(RNstartBtn,"Start"));
		(void) IngredCommand(GE_ANIMATE, "STOP");
		if(valid_tstamp(actual_set_time))
			(void) IngredVaCommand(GE_ANIMATE, "SHOW %s", actual_set_time);
	}

	parms[0] = OB_KEY_DEPICT;
	parms[1] = (animation_active)? OB_KEY_ON:OB_KEY_OFF;
	NotifyObservers(OB_ANIMATION_RUNNING, parms, 2);
}


/*=========================================================================*/
/*
*    Respond to a user grabbing the display scale and setting it to
*    a specific time delta.
*/
/*=========================================================================*/
/*ARGSUSED*/
static void position_display_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int     year, jday, hour, min;
	String  dt;
	XmScaleCallbackStruct *rtn = (XmScaleCallbackStruct *)call_data;

	if(animation_active) start_stop_cb(NullWidget, NULL, NULL);

	if(rtn->reason == XmCR_DRAG) return;

	scaleRespond = True;
	if( parse_tstamp(GV_T0_depict, &year, &jday, &hour, &min, NULL, NULL) )
	{
		if(minutes_in_depictions())
		{
			min += rtn->value;
			tnorm(&year, &jday, &hour, &min, NullInt);
			(void) strcpy(scale_set_time, build_tstamp(year, jday, hour, min, False, True));
		}
		else
		{
			hour += rtn->value;
			tnorm(&year, &jday, &hour, &min, NullInt);
			(void) strcpy(scale_set_time, build_tstamp(year, jday, hour, min, False, False));
		}
		(void) strcpy(actual_set_time, scale_set_time);
		(void) IngredVaCommand(GE_ANIMATE, "SHOW %s", actual_set_time);
	}
}
