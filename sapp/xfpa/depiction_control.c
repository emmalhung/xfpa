/****************************************************************************
*
*  File:    depiction_control.c
*
*  Purpose: Contains the functions which handle all of the items in the
*           "Depiction" drop down menu except "Quit" and all of the
*           functionality of the sequence control and display grouping on
*           the upper right of the window.
*
*           Note that the sequence control consists of the main depiction
*           sequence selection buttons and control arrows and the selection
*           buttons for the secondary sequence bar when such is required by
*           the active field (ie. daily and static fields).
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
#include <ingred.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>
#include "resourceDefines.h"
#include "editor.h"
#include "guidance.h"
#include "depiction.h"
#include "observer.h"
#include "timelink.h"

#define RESET 999999

/* Handy for setting and clearing the depiction sequence limits */
#define SL	1
#define CL	2
#define SU	3
#define CU	4
#define T0	5


static void    add_depiction_to_sequence (String);
static void    create_limit_popup (void);
static void    depiction_button_cb (Widget, XtPointer, XtPointer);
static void    set_sequence_btn_time_display (void);
static void    sequence_display_update_observer (CAL, String*, int);
static Boolean show_secondary_sequence_bar (void);
static void    time_limit_set_cb (Widget, XtPointer, XtPointer);
static void    time_limits_popup_handler (Widget, XtPointer, XEvent*, Boolean*);

/* These variables hold some of the infomation required to define the
*  depiction sequence.  The rest are found in global.h
*/
static String     active_depict    = NULL;
static int        active_depict_no = 0;
static int        ndepictBtn       = 0;
static WidgetList depictBtn        = NULL;

/* Variables to hold the times at which we want to start and stop the
 * visible sequence selection buttons and the indicator buttons to show
 * that the sequence is limited.
 */
static Boolean    allow_seq_limit = True;
static TSTAMP     seq_start_time  = "\0";
static TSTAMP     seq_end_time    = "\0";
static Widget     seqStartBtn     = NullWidget;
static Widget     seqEndBtn       = NullWidget;

static Pixel      daily_select_fg, daily_select_bg;
static Pixel      daily_match_fg,  daily_match_bg;

static int        arrow_ndx       = -1;
static XuARROWDIR arrow_direction = XuARROW_RIGHT;

/* Secondary field selection button variables. Note that the variable
*  active_sec_time will be empty if there is no currently selected
*  secondary field displayed.
*/
static TSTAMP  active_sec_time = "\0";
static Boolean modify_sec_time = True;
static int     nsec_dates      = 0;
static String  *sec_dates      = NULL;
static int     nsec_btns       = 0;
static Widget  *sec_btns       = NULL;


/*==========================================================================*/
/*
*	InitDepictionSequence() - Read up any existing depictions which reside
*	in the "Depict" directory and add them to the sequence. If requested
*   by set_time_range put up a dialog so that the user can select the set
*   of depiction times to read.
*/
/*==========================================================================*/
void InitDepictionSequence(Boolean set_time_range )

{
	int    i, nlist;
	char   mbuf[256];
	String ptr, *time_list;
	PARM   *setup;

	daily_match_fg  = XuLoadColorResource(GW_topLevel, RNdailyMatchFg,  "foreground");
	daily_match_bg  = XuLoadColorResource(GW_topLevel, RNdailyMatchBg,  "background");
	daily_select_fg = XuLoadColorResource(GW_topLevel, RNdailySelectFg, "foreground");
	daily_select_bg = XuLoadColorResource(GW_topLevel, RNdailySelectBg, "background");

	GV_ndepict = 0;
	GV_depict  = NULL;
	GV_interp_time_delta = 60;
	SetDepictionTimeDisplay(NULL);

	(void) safe_strcpy(mbuf, "INITIALIZE");

	if(!GV_edit_mode)
	{
		(void) safe_strcat(mbuf," VIEW");
	}
	else
	{
		/* Find the time interpolation interval.
		*/
		ptr = "0";
		if((setup = GetSetupParms(INTERP_DELTA))) ptr = setup->parm[0];
		(void) safe_strcat(mbuf, " EDIT ");
		(void) safe_strcat(mbuf, ptr);
		i = interpret_hour_minute_string(ptr);
		if(i > 0) GV_interp_time_delta = i;
		if(!minutes_in_depictions() && GV_interp_time_delta < 60)
		{
			GV_interp_time_delta = 60;
			pr_warning("InitDepictionSequence","The interpolation delta value is less than one hour when no minutes are allowed in the depictions. Delta set to one hour.");
		}
	}

	if(set_time_range)
		{
		XuSetBusyCursor(OFF);
		ACTIVATE_getDepictionRangeDialog(mbuf);
		XuSetBusyCursor(ON);
		}
	else
		{
		(void) safe_strcat(mbuf, " ALL NOSAVE");
		}

	if(!IngredCommand(GE_SEQUENCE, mbuf))
	{
		XuClearBusyCursor();
		GV_pref.confirm_exit = (Boolean) False;
		XuShowMessage(GW_mainWindow, "dataLock", NULL);
		MainExitCB(NULL, NULL, NULL);
	}

	/* Ingred has now initialized and created the depiction sequence.  We
	*  now must ask for the list and create our internal list as well.
	*/
	(void) GEStatus("DEPICT TIMES", &nlist, &time_list, NULL, NULL);
	for( i = 0; i < nlist; i++ ) 
	{
		add_depiction_to_sequence(time_list[i]);
	}

	AddIngredObserver(sequence_display_update_observer);
}


/*==========================================================================*/
/*
*	InitToActiveDepiction() - Send Ingred the active depiction info.
*/
/*==========================================================================*/
void InitToActiveDepiction(void)

{
	if( GV_ndepict > 0 )
	{
		XuToggleButtonSet(depictBtn[active_depict_no], True, False);
		active_depict = GV_depict[active_depict_no];
		(void) IngredVaCommand(GE_SEQUENCE, "ACTIVE %s", active_depict);
		SetDepictionTimeDisplay(NULL);
	}
	SetT0Depiction(T0_INITIALIZE);
}



/*  Now call any functions for those dialogs which must respond to 
 *  depiction sequence changes. The changes are done in a delayed
 *  time loop to give the change processes a chance to finish.
 */
/*ARGSUSED*/
static void execute_update_functions(XtPointer client_data , XtIntervalId *id )
{
	NotifyObservers(OB_DEPICTION_NUMBER_CHANGE, NULL, 0);
}

static void call_update_functions(void)
{
	(void) XtAppAddTimeOut(GV_app_context, 0, execute_update_functions, NULL);
}


/*==========================================================================*/
/*
 *   DepictionAtTime() - Is there a depiction at the specified time?
 */
/*==========================================================================*/
Boolean DepictionAtTime( String dt )
{
	if(valid_tstamp(dt))
	{
		int i;
		for( i = 0; i < GV_ndepict; i++ )
		{
			if(matching_tstamps(dt, GV_depict[i])) return True;
		}
	}
	return False;
}


/*==========================================================================*/
/*
*	CreateDepiction() - Issue the command to Ingred to create an empty
*	                    depiction at the given time and update the gui
*	controls to include the depiction. Note that the current T0 depiction
*	selection remains as is.
*/
/*==========================================================================*/
Boolean CreateDepiction(String dt )
{
	int i;

	if(!valid_tstamp(dt))
		return False;

	/* Check to see if there is already a depiction at the given time.
	*/
	for( i = 0; i < GV_ndepict; i++ )
	{
		if(matching_tstamps(dt, GV_depict[i]))
			return True;
	}

	/* Nope. Tell Ingred to create it. */
	if(!IngredVaCommand(GE_SEQUENCE, "CREATE_DEPICTION - - - - %s", dt))
		return False;

	add_depiction_to_sequence(dt);
	set_sequence_btn_time_display();
	call_update_functions();
	return True;
}


/*==========================================================================*/
/*
*	SetT0Depiction() - Set the T0 depiction time to either the active
*   depiction, the system clock time, or to the depiction closest to but
*   not greater than the system clock.
*/
/*==========================================================================*/
void SetT0Depiction(int key )
{
	PARM *parm;

	static int roundoff = -1;

	if(roundoff < 0)
	{
		/* Get the T0 roundoff parameter. Search the setup file first and if no
		*  valid roundoff is found then get it from the resource file.
		*/
		parm = GetSetupParms("T0.roundOff");
		if(IsNull(parm) || parm->nparms < 1 || sscanf(parm->parm[0], "%d", &roundoff) != 1)
			roundoff = XuGetIntResource(RNT0roundOff,12);
	}

	/* The T0_INITIALIZE_NEW_ONLY key will only initialize T0 if the existing
	*  T0 is not a valid time. Otherwise we leave it alone.
	*/
	if(key == T0_INITIALIZE_NEW_ONLY)
	{
		if(valid_tstamp(GV_T0_depict)) return;
		AppDateTime(GV_T0_depict, roundoff);
	}
	else if(key == T0_TO_ACTIVE_DEPICTION)
	{
		if(GV_ndepict > 0) (void) safe_strcpy(GV_T0_depict,active_depict);
	}
	else if(key == T0_TO_SYSTEM_CLOCK)
	{
		AppDateTime(GV_T0_depict, 1);
	}
	else if(key == T0_NEAREST_TO_SYSTEM_CLOCK)
	{
		int  i;
		TSTAMP mbuf;

		if(GV_ndepict > 0)
		{
			AppDateTime(mbuf, roundoff);
			(void) safe_strcpy(GV_T0_depict, GV_depict[0]);
			for( i = 0; i < GV_ndepict; i++ )
			{
				if(!valid_tstamp(GV_depict[i])) continue;
				if( MinuteDif(mbuf,GV_depict[i]) >= 0 ) break;
				(void) safe_strcpy(GV_T0_depict,GV_depict[i]);
			}
		}
		else
		{
			AppDateTime(GV_T0_depict, roundoff);
		}
	}
	else if(key == T0_INITIALIZE)
	{
		AppDateTime(GV_T0_depict, roundoff);
	}
	else if(key >= 0 && key < GV_ndepict)
	{
		(void) strcpy(GV_T0_depict, GV_depict[key]);
	}

	set_sequence_btn_time_display();

	(void) IngredVaCommand(GE_SEQUENCE, "TZERO %s", GV_T0_depict);
	(void) IngredCommand(GE_ACTION, "REDISPLAY");
	NotifyObservers(OB_DEPICTION_TZERO_CHANGE, NULL, 0);
}


/*==========================================================================*/
/*
*	MakeActiveDepiction() - Make the given depiction the active depiction.
*/
/*==========================================================================*/
void MakeActiveDepiction(String dt )
{
	if(GV_ndepict > 0)
	{
		if(same(dt,ACTIVE))
		{
			XuToggleButtonSet(depictBtn[active_depict_no], True, True);
		}
		else
		{
			int i;
			for( i = 0; i < GV_ndepict; i++ )
			{
				if(!matching_tstamps(dt,GV_depict[i])) continue;
				XuToggleButtonSet(depictBtn[i], True, True);
				break;
			}
		}
	}
	SetDepictionTimeDisplay(NULL);
	set_sequence_btn_time_display();
}


/*==========================================================================*/
/*
*	SaveDepiction() - This saves depictions to the backup directory and is
*	used within the callback of the depiction dropdown menu item.  It also
*	removes any depictions older than the period in days sepcified in the
*	setup file.  The default is 7 days.
*/
/*==========================================================================*/
void SaveDepiction(String cmd )
{
	int    t = 0, savetime = 7;
	char   mbuf[200];
	String dt, dir;
	PARM   *setup;

	/* Take no action if there are no depictions */
	if( GV_ndepict < 1 ) return;

	setup = GetSetupParms(DEPICT_SAVE);
	dir   = source_directory_by_name(BACKUP, NULL, NULL);

	/* Run a system call to remove all depiction files older than the specified
	 * time. We want to wait until the command finished before proceeding so we
	 * do not use the "&" on the command. A savetime value of <= 0 will not remove
	 * any files.
	 */
	if(NotNull(setup) && sscanf(setup->parm[0],"%d",&t) == 1) savetime = t;
	if(savetime > 0)
	{
		(void) snprintf(mbuf, sizeof(mbuf), "( find %s -name \'*:*:*\' -mtime +%d -exec rm {} \\; )",dir,savetime);
		(void) system(mbuf);
	}

	if(valid_tstamp(cmd))
	{
		dt = cmd;
	}
	else if(same(cmd,ACTIVE))
	{
		if(	GV_active_field->info->element->elem_tdep->time_dep != FpaC_NORMAL &&
			valid_tstamp(active_sec_time))
			dt = active_sec_time;
		else
			dt = active_depict;
	}
	else
	{
		dt = "ALL";
	}

	(void) IngredVaCommand(GE_SEQUENCE, "SAVE_DEPICTION ALL %s", dt);
	NotifyObservers(OB_DEPICTION_SAVED, NULL, 0);
}


/*==========================================================================*/
/*
*	RemoveDepiction() - The cmd parameter may have 2 values: "all" or the
*	                    time of a depiction.  For "all" the user is prompted
*	for permission for the deletion. 
*/
/*==========================================================================*/
void RemoveDepiction(String cmd )
{
	int     i, nfld, posn;
	String  *elem, *level;
	Boolean set_active, all_valid;

	if( GV_ndepict < 1 ) return;
	if( !same(cmd,"all") && !InTimeList(cmd,GV_depict,GV_ndepict,&posn) ) return;

	if(same(cmd,"all")) 
	{
		int    n, response;
		String *mod_list;

		(void) GEStatus("DEPICT MODIFIED", &n, &mod_list, NULL, NULL);
		if( n > 0 )
		{
			response = XuMakeActionRequest(GW_mainWindow, XuYNC, "SomeModified", NULL);
			if( response == XuCANCEL ) return;
			if( response == XuYES ) SaveDepiction("all");
		}
		for( i = GV_ndepict - 1; i >= 0; i-- ) 
		{
			(void) IngredVaCommand(GE_SEQUENCE, "DELETE_DEPICTION ALL %s", GV_depict[i]);
		}
		GV_ndepict = 0;
		active_depict_no = 0;
		active_depict = GV_depict[0];
		SetDepictionTimeDisplay((String)NULL);
	} 
	else
	{
		set_active = matching_tstamps(active_depict, cmd);
		(void) IngredVaCommand(GE_SEQUENCE, "DELETE_DEPICTION ALL %s", cmd);
		GV_ndepict--;
		for( i = posn; i < GV_ndepict; i++ )
		{
			(void) safe_strcpy(GV_depict[i],GV_depict[i+1]);
		}
		if( GV_ndepict > 0)
		{
			if((set_active && active_depict_no > 0) || active_depict_no >= GV_ndepict)
			{
				active_depict_no--;
				active_depict = GV_depict[active_depict_no];
			}
		}
		else
		{
			active_depict_no = 0;
			active_depict = GV_depict[0];
			SetDepictionTimeDisplay((String)NULL);
		}
	}

	/* Check various things depending on the number of depictions left
	 */
	if(GV_ndepict < 1)
	{
		GV_T0_depict[0]   = '\0';
		seq_start_time[0] = '\0';
		seq_end_time[0]   = '\0';
	}
	else if(GV_ndepict < 2)
	{
		seq_start_time[0] = '\0';
		seq_end_time[0]   = '\0';
	}
	else
	{
		/* If our sequence range limiting times are no longer valid reset them to the
		* end of the time array.
		*/
		if(!InTimeList(seq_start_time, GV_depict, GV_ndepict, NULL))
			(void) safe_strcpy(seq_start_time, GV_depict[0]);
		if(!InTimeList(seq_end_time, GV_depict, GV_ndepict, NULL))
			(void) safe_strcpy(seq_end_time, GV_depict[GV_ndepict-1]);
	}

	/* We must check to see if there are any fields in the interface data
	*  structures which do not now exist in any of the remaining depictions.
	*  If so we must remove them from the interface data structures. Note
	*  that the double loop is required as the field values are modified 
	*  when the field is actually removed from the field list.
	*/
	if( GEStatus("FIELDS", &nfld, &elem, &level, NULL) == GE_INVALID) return;
	for(;;)
	{
		all_valid = True;
		for( i = 0; i < GV_nfield; i++ )
		{
			if(GV_field[i]->manditory) continue;
			if(InFieldList(GV_field[i], nfld, elem, level, NULL)) continue;
			all_valid = False;
			RemoveField(GV_field[i]);
			break;
		}
		if(all_valid) break;
	}

	set_sequence_btn_time_display();
	XuUpdateDisplay(GW_mainWindow);
	call_update_functions();
}


/*==========================================================================*/
/*
*   ResetDepictionSelection() - Issue the commands to set the field, edit
*                               functionality and the active depiction in
*   the sequence. How this is done will depend on if there are secondary
*   buttons corresponding to daily or static fields.
*/
/*==========================================================================*/
void ResetDepictionSelection(Boolean send)
{
	/* The send test must come after show_secondary_sequence_bar */
	if(!show_secondary_sequence_bar() && send)
	{
		(void) IngredVaCommand(GE_DEPICTION, "FIELD %s %s",
		 	GV_active_field->info->element->name,
		 	GV_active_field->info->level->name);
		(void) IngredVaCommand(GE_SEQUENCE, "ACTIVE %s %s", active_depict, active_sec_time);
		GV_active_field->sendEditCmdFcn(ACCEPT_MODE);
	}
}


/*==========================================================================*/
/*
 *   ResetDepictionBtnColour() - Set the depiction selection buttons to
 *                               their default colour.
 */
/*==========================================================================*/
void ResetDepictionBtnColour(void)
{
	int i;
	Pixel fg, bg;

	for(i = 0; i < GV_ndepict; i++)
	{
		GetSequenceBtnColor(GV_depict[i], &fg, &bg);
		XtVaSetValues(depictBtn[i], XmNforeground, fg, XmNbackground, bg, NULL);
	}
}


/*==========================================================================*/
/*
*	IsActiveDepiction() - Returns true if the given time is that of the 
*	active depiction.
*/
/*==========================================================================*/
Boolean IsActiveDepiction(String date_time )
{
	return matching_tstamps(date_time,active_depict);
}


/*==========================================================================*/
/*
*	ActiveDepictionTime() - Returns the time of the active depiction.
*
*	2002/06/25 Modified to return the active time depending on the field
*	           time dependency type.
*/
/*==========================================================================*/
String ActiveDepictionTime(FIELD_DEPENDENCY dep)
{
	if( dep == FIELD_INDEPENDENT ) return active_depict;
	if( blank (active_sec_time)  ) return active_depict;
	if( IsNull(GV_active_field)  ) return active_depict;

	switch(GV_active_field->info->element->elem_tdep->time_dep)
	{
		case FpaC_DAILY:
		case FpaC_STATIC:
			return active_sec_time;

		default:
			return active_depict;
	}
}

/*==========================================================================*/
/*
*	SetActiveTimelinkDepiction() - Inserts an arrow into the depiction
*	selection button at the specified depiction time.  If dirn is "forward"
*	the symbol is "->", if dirn is "backward" the symbol is "<-".  If time
*	is NULL then the buttons are reset to their normal display values.
*/
/*==========================================================================*/
void SetActiveTimelinkDepiction(String date_time , Boolean linking_backward )
{
	String dt;

	if(arrow_ndx >= 0) XuClearButtonArrow(depictBtn[arrow_ndx]);

	dt = (same_ic(date_time,"active")) ? active_depict : date_time;

	if(InTimeList(dt, GV_depict, GV_ndepict, &arrow_ndx))
	{
		XuArrowAttributes attrib;

		arrow_direction  = (linking_backward)? XuARROW_LEFT:XuARROW_RIGHT;
		attrib.flags     = XuARROW_DIRECTION | XuARROW_APPEARANCE;
		attrib.direction = arrow_direction;
		attrib.appearance = XuARROW_OUTLINED;
		XuSetButtonArrow(depictBtn[arrow_ndx], &attrib);
	}
	else
	{
		arrow_ndx = -1;
	}
}

/*=========================================================================*/
/*
*	DepictionStep() - Called to step through the depictions. The increment
*                     parameter determines the direction.
*/
/*=========================================================================*/
/* ARGSUSED */
void DepictionStep(int increment, int increment_delta)
{
	if( GV_ndepict < 1 ) return;

	if(PanelIsActive(TIMELINK))
	{
		/* If we are in timelink the stepping process is somewhat different. If
		 * there is no valid field at the next stepped time we want to skip it.
		 * We find out the status of the field at a particular time by examining
		 * the depiction button colour. The count variable just ensures that we
		 * do not get into an endless loop if there are no valid times for the
		 * active field (should not happen). One could create a GEStatus call to
		 * return the list of times, increment on this list and then relate this
		 * back to the list of depiction times, but as this seems to work ok why
		 * bother (unless there are unforseen problems).
		 */
		int   count = 0, ndx;
		Pixel fg, bg, bfg, bbg;

		/* ndx is used in case we need to bail */
		GetSequenceBtnColor( NULL, &fg, &bg );

		/* First we find the depiction that meets our minumum time step requirement.
		 */
		if(increment_delta > 0)
		{
			TSTAMP min_time;
			(void) safe_strcpy(min_time, calc_valid_time_minutes(active_depict, 0, increment_delta*increment));
			if(increment > 0)
			{
				while(active_depict_no + increment < GV_ndepict)
				{
					active_depict_no += increment;
					if(MinuteDif(min_time, GV_depict[active_depict_no]) >= 0) break;
				}
			}
			else
			{
				while(active_depict_no + increment >= 0)
				{
					active_depict_no += increment;
					if(MinuteDif(min_time, GV_depict[active_depict_no]) <= 0) break;
				}
			}
			/* back up one for the next loop
			 */
			active_depict_no -= increment;
		}

		/* No we find first valid field after this
		 */
		ndx = active_depict_no;
		do
		{
			if(count > GV_ndepict) return; /* bail - we have a problem */
			count++;
			ndx += increment;
			if( ndx < 0 ) ndx = 0;
			if( ndx > GV_ndepict-1 ) ndx = GV_ndepict-1;
			XtVaGetValues(depictBtn[ndx], XmNforeground, &bfg, XmNbackground, &bbg, NULL);
		} while(bfg == fg && bbg == bg);
		active_depict_no = ndx;
	}
	else
	{
		int pos;
		int bgn_pos = 0;
		int end_pos = GV_ndepict - 1;
		
		/* The effect of the depiction increment must be limited to the limits imposed
		 * by the sequence start and end times.
		 */
		if(allow_seq_limit)
		{
			if(InTimeList(seq_start_time, GV_depict, GV_ndepict, &pos)) bgn_pos = pos;
			if(InTimeList(seq_end_time,   GV_depict, GV_ndepict, &pos)) end_pos = pos;
		}

		if(increment_delta > 0)
		{
			TSTAMP min_time;
			(void) safe_strcpy(min_time, calc_valid_time_minutes(active_depict, 0, increment_delta*increment));
			if(increment > 0)
			{
				while(active_depict_no + increment <= end_pos)
				{
					active_depict_no += increment;
					if(MinuteDif(min_time, GV_depict[active_depict_no]) >= 0) break;
				}
			}
			else
			{
				while(active_depict_no + increment >= bgn_pos)
				{
					active_depict_no += increment;
					if(MinuteDif(min_time, GV_depict[active_depict_no]) <= 0) break;
				}
			}
		}
		else
		{
			active_depict_no += increment;
			if( active_depict_no < bgn_pos ) active_depict_no = bgn_pos;
			if( active_depict_no > end_pos ) active_depict_no = end_pos;
		}
	}
	active_depict = GV_depict[active_depict_no];
	XuToggleButtonSet(depictBtn[active_depict_no], True, True);
}


/*=========================================================================*/
/*
*	HaveFieldToEdit() - Returns True if we actually have a field to edit.
*/
/*=========================================================================*/
Boolean HaveFieldToEdit(void)
{
	if(GV_active_field->info->element->elem_tdep->time_dep == FpaC_NORMAL) return True;
	if(!blank(active_sec_time))  return True;
	return False;
}


/*=========================================================================*/
/*
*	HideSecondarySequenceBar() - Called to hide the secondary sequence bar.
*	                             We also need to reset the sequence in
*   Ingred as the secondary time is blanked (set to none).
*/
/*=========================================================================*/
void HideSecondarySequenceBar(Boolean force_hide)
{
	if( force_hide ||
		IsNull(GV_active_field) || 
		GV_active_field->info->element->elem_tdep->time_dep == FpaC_NORMAL)
	{
		XtVaSetValues(XtParent(GW_mainMessageBar),
			XmNrightWidget, GW_tabFrame,
			NULL);
		XtUnmapWidget(GW_secSeqManager);
		(void) safe_strcpy(active_sec_time, "");
		(void) IngredVaCommand(GE_SEQUENCE, "ACTIVE %s", active_depict);
		XuUpdateDisplay(GW_mainWindow);
	}
	modify_sec_time = True;
}


/* ARGSUSED */
/*=========================================================================*/
/*
*   Called by the question mark button "?" in the secondary sequence bar
*   to reset the sequence selection. We do not want to send the secondary
*   time for this operation just the active depiction time.
*/
/*=========================================================================*/
void SecondaryBtnResetCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i, ndates;
	char mbuf[300];
	String *active;

	(void) snprintf(mbuf,  sizeof(mbuf), "FIELD TIMES %s %s",
		GV_active_field->info->element->name,
		GV_active_field->info->level->name);
	(void) GEStatus(mbuf, &ndates, NULL, &active, NULL);

	(void) safe_strcpy(active_sec_time,"");
	modify_sec_time = True;

	for(i = 0; i < nsec_dates; i++)
	{
		XuToggleButtonSet(sec_btns[i], False,  False);
		if(matching_tstamps(sec_dates[i],active[0]))
		{
			XuToggleButtonSet(sec_btns[i], True,  False);
			(void) safe_strcpy(active_sec_time,sec_dates[i]);
		}
	}
	(void) IngredVaCommand(GE_SEQUENCE, "ACTIVE %s", active_depict);
	ActivateMenu();
}


void AllowLimitedDepictionSequence(Boolean state)
{
	allow_seq_limit = state;
	set_sequence_btn_time_display();
}


/*====================== LOCAL (STATIC) FUNCTIONS ==========================*/

static Widget timeLimitsPopup = NullWidget;
static Widget setLowerLimit, setUpperLimit, clearLowerLimit, clearUpperLimit;
static int    menu_seq_btn = 0;



/*  Respond to the status information sent to the interface from ingred with
 *  regards field status. For the FIELD status info there are tw0 keys:
 *  STATUS which updates the depiction sequence for a specific time and
 *  TIMES which gives a list of depiction times at which the active field exists.
 */
/*ARGSUSED*/
static void sequence_display_update_observer( CAL cal, String *parms, int nparms )
{
	int i, ntimes, rtn, pos, pc = 0;
	String key, elm, lev, dt, status, *tlist, *slist;
	Boolean in_timelink;
	Pixel bg, fg, bfg, bbg;
	FIELD_INFO *fld = 0;

	/* Note that parms[0] contains a CAL pointer */
	if(!same_ic(parms[pc++], "FIELD")) return;

	key = parms[pc++];
	elm = parms[pc++];
	lev = parms[pc++];

	in_timelink = PanelIsActive(TIMELINK);

	/* An element of MASTER_LINK is a special case for timelinking and
	 * must be handled separately. Level is just a dummy.
	 */
	if(in_timelink && same_ic(elm, "MASTER_LINK"))
	{
		if(GV_atlgrp->atlfldno != 0) return;
	}
	else
	{
		FIELD_INFO *afld = (in_timelink) ? GV_atlgrp->atlfld:GV_active_field;
		fld = FindField(elm, lev);
		if(IsNull(fld) || fld != afld) return;
	}

	if(same_ic(key, "STATUS"))
	{
		dt     = parms[pc++];
		status = parms[pc++];

		if(InTimeList(dt, GV_depict, GV_ndepict, &pos))
		{
			if(same_ic(status, "DELETED"))
				GetSequenceBtnColor( NULL, &fg, &bg );
			else if(in_timelink)
				LinkStatusStringToColour(status, &fg, &bg);
			else
				GetSequenceBtnColor( dt, &fg, &bg );

			/* Only apply the color if there is a change then update the display so
			 * that the button changes immediately.
			 */
			XtVaGetValues(depictBtn[pos], XmNforeground, &bfg, XmNbackground, &bbg, NULL);
			if(bfg != fg || bbg != bg)
			{
				XtVaSetValues(depictBtn[pos], XmNforeground, fg, XmNbackground, bg, NULL);
				XuUpdateDisplay(depictBtn[pos]);
			}
		}
	}
	else if(same_ic(key, "TIMES"))
	{
		/* We must do this if we change the button colours otherwise
		*  the arrow does not change colour in sync with the buttons!
		*/
		ntimes = atoi(parms[pc++]);
		if (fld) fld->exists = (ntimes > 0);

		/* There is a status given after every time.
		*/
		tlist = NewStringArray(ntimes);
		slist = NewStringArray(ntimes);

		for(i = 0; i < ntimes; i++)
		{
			tlist[i] = parms[pc++];
			slist[i] = parms[pc++];
		}

		/* If the field is not in the sequence at a particular time we set
		*  the sequence button to the no field colour.
		*/
		for(i = 0; i < GV_ndepict; i++)
		{
			if(InTimeList(GV_depict[i], tlist, ntimes, &pos))
			{
				if(same_ic(slist[pos], "DELETED"))
					GetSequenceBtnColor( NULL, &fg, &bg );
				else if(in_timelink)
					LinkStatusStringToColour(slist[pos], &fg, &bg);
				else
					GetSequenceBtnColor( GV_depict[i], &fg, &bg );
			}
			else
			{
				GetSequenceBtnColor( NULL, &fg, &bg );
			}

			/* Only apply the color if there is a change then update the display so
			 * that the button changes immediately.
			 */
			XtVaGetValues(depictBtn[i], XmNforeground, &bfg, XmNbackground, &bbg, NULL);
			if(bfg != fg || bbg != bg)
			{
				XtVaSetValues(depictBtn[i], XmNforeground, fg, XmNbackground, bg, NULL);
				XuUpdateDisplay(depictBtn[i]);
			}

			if(arrow_ndx == i)
			{
				XuArrowAttributes attrib;

				attrib.flags      = XuARROW_DIRECTION|XuARROW_APPEARANCE;
				attrib.direction  = arrow_direction;
				attrib.appearance = XuARROW_OUTLINED;
				XuSetButtonArrow(depictBtn[i], &attrib);
			}
		}
		FreeItem(tlist);
		FreeItem(slist);
	}
}


/* ARGSUSED */
static void secondary_btn_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	if(!XmToggleButtonGetState(w)) return;

	/* Once we have an active secondary sequence by selecting its button we
	 * do not want the primary depiction selection buttons to reset it. This
	 * is only done by activating the reset callback. Trace this variable to
	 * see where this is done.
	 */
	modify_sec_time = False;
	(void) safe_strcpy(active_sec_time, sec_dates[PTR2INT(client_data)]);
	(void) show_secondary_sequence_bar();
}



/* Check to see if the active field requires a secondary sequence time bar.
 * If it does the bar is created and shown. Returns True if a secondary
 * field is found and selected, False if not.
 */
static Boolean show_secondary_sequence_bar(void)
{
	int     i, ndx;
	char    mbuf[300];
	String  *dt, *ac;
	Pixel   fg, bg;
	Boolean rtn = False;

	/* If the number of secondary field dates changes then we need to
	 * redisplay the selection buttons even if the modify_sec_time flag
	 * is False - thus this variable to keep track of the numer of dates.
	 */
	static int nsec_dates_last = 0;

	if(IsNull(GV_active_field)                          ) return False;
	if(IsNull(GV_active_field->info)                    ) return False;
	if(IsNull(GV_active_field->info->element)           ) return False;
	if(IsNull(GV_active_field->info->element->elem_tdep)) return False;

	/* Show the buttons only if the field is of type static or daily.  */
	if(GV_active_field->info->element->elem_tdep->time_dep != FpaC_DAILY  &&
	   GV_active_field->info->element->elem_tdep->time_dep != FpaC_STATIC   )
	   	return False;

	/* We only show the secondary buttons while editing */
	if(!PanelIsActive(ELEMENT_EDIT)) return False;

	/* Ask Ingred for the list of depictions for this field. The return in the
	*  first element of ac is the time of the depiction that corresponds to the
	*  currently selected normal type depiction. We need to send the current
	*  field and sequence command first so that ingred knows whatof we speak.
    */
    (void) check_value_function_error_messages(False);
    (void) check_wind_function_error_messages(False);

	(void) IngredVaCommand(GE_DEPICTION, "FIELD %s %s",
		GV_active_field->info->element->name,
		GV_active_field->info->level->name);
	(void) IngredVaCommand(GE_SEQUENCE, "ACTIVE %s %s", active_depict, active_sec_time);
	GV_active_field->sendEditCmdFcn(ACCEPT_MODE);

	(void) snprintf(mbuf,  sizeof(mbuf), "FIELD TIMES %s %s",
		GV_active_field->info->element->name,
		GV_active_field->info->level->name);
	(void) GEStatus(mbuf, &nsec_dates, &dt, &ac, NULL);

	/* If active_sec_time was not set by the user specifically by pressing one
	 * of the secondary time buttons we set it to the time corresponding to the
	 * currently selected depiction.
	 */
	if (modify_sec_time) (void) safe_strcpy(active_sec_time, ac[0]);

	if(nsec_dates > nsec_btns)
	{
		sec_dates = MoreStringArray(sec_dates, nsec_dates);
		sec_btns  = MoreWidgetArray(sec_btns,  nsec_dates);
		for(i = nsec_btns; i < nsec_dates; i++)
		{
			sec_dates[i] = XtMalloc(sizeof(TSTAMP));
			sec_btns[i]  = XmVaCreateToggleButton( GW_secSeqBtns, "secBtn",
				XmNborderWidth,     0,
				XmNshadowThickness, 2,
				XmNindicatorOn,     False,
				XmNset,             XmUNSET,
				NULL);
			XtAddCallback(sec_btns[i],XmNvalueChangedCallback,secondary_btn_cb,INT2PTR(i));
		}
		nsec_btns = nsec_dates;
	}

	(void) check_value_function_error_messages(True);
	(void) check_wind_function_error_messages(True);

	/* If the modify_sec_time is not set and the number of dates is the same
	 * as last time we are only permitted to change the colour of the buttons
	 * and not their settings. This is to avoid the obvious flashing that occurs
	 */
	if(modify_sec_time || nsec_dates_last != nsec_dates)
	{
		/* Here we unmanage the selection buttons and change the attachment of the
		*  manager widget. The attachment is then reset below. This is necessary to
		*  ensure that the selection structure behaves properly on display. Why? I
		*  suspect that some internal state of the manager requires a "jiggle" in
		*  order to respond to the changes properly.
		*/
		ndx = -1;

		XtUnmanageChildren(sec_btns, nsec_btns);
		XtVaSetValues(XtParent(GW_mainMessageBar), XmNrightWidget, GW_tabFrame, NULL);
		XtUnmapWidget(GW_secSeqManager);
		XtVaSetValues(GW_secSeqManager, XmNleftAttachment, XmATTACH_FORM, NULL);

		for(i = 0; i < nsec_dates; i++)
		{
			XtVaSetValues(sec_btns[i], XmNset, False, NULL);
			(void) safe_strcpy(sec_dates[i], dt[i]);
			if(matching_tstamps(active_sec_time,sec_dates[i])) ndx = i;

			if( GV_active_field->info->element->elem_tdep->time_dep == FpaC_DAILY )
				XuWidgetPrint(sec_btns[i], "%s", DateString(sec_dates[i], GV_pref.daily_date_format));
			else
				XuWidgetPrint(sec_btns[i], "%.2d", HourDif(GV_T0_depict, sec_dates[i]));
		}
		XtManageChildren(sec_btns, nsec_dates);
		XuWidgetLabel(GW_secSeqLabel, GV_active_field->info->sh_label);
		XtVaSetValues(GW_secSeqManager, XmNleftAttachment, XmATTACH_NONE, NULL);
		XtVaSetValues(XtParent(GW_mainMessageBar), XmNrightWidget, GW_secSeqManager, NULL);
		XtMapWidget(GW_secSeqManager);

		/* Clear active_sec_time in case there was no time match above. The active
		 * field may have been deleted.
		 */
		(void) safe_strcpy(active_sec_time, "");

		/* This is done here and not in the loop above to reduce widget flashing when the
		 * ingred commands take a considerable time.
		 */
		if (ndx != -1)
		{
			/* order of these statements important */
			(void) safe_strcpy(active_sec_time, sec_dates[ndx]);
			XtVaSetValues(sec_btns[ndx], XmNset, True, NULL);
			(void) IngredVaCommand(GE_SEQUENCE, "ACTIVE %s %s", active_depict, active_sec_time);
			GV_active_field->sendEditCmdFcn(ACCEPT_MODE);
			rtn = True;
		}
	}

	XtVaGetValues(GW_secSeqBtns, XmNforeground, &fg, XmNbackground, &bg, NULL);

	/* Set the colour of the buttons depending on the time match.
	 */
	for(i = 0; i < nsec_dates; i++)
	{
		if(matching_tstamps(ac[0],sec_dates[i]))
		{
			XtVaSetValues(sec_btns[i],
				XmNforeground, daily_match_fg,
				XmNbackground, daily_match_bg,
				NULL);
		}
		else if(matching_tstamps(active_sec_time, sec_dates[i]) && XmToggleButtonGetState(sec_btns[i]))
		{
			XtVaSetValues(sec_btns[i],
				XmNforeground, daily_select_fg,
				XmNbackground, daily_select_bg,
				NULL);
		}
		else
		{
			XtVaSetValues(sec_btns[i],
				XmNforeground, fg,
				XmNbackground, bg,
				NULL);
		}
	}
	SetDepictionTimeDisplay(NULL);
	XmUpdateDisplay(GW_secSeqManager);
	nsec_dates_last = nsec_dates;
	return rtn;
}


/* Add the a depiction to the internal data structures of the interface.
*/
static void add_depiction_to_sequence(String dt )
{
	int     n, ndx;
	TSTAMP  ts;
	Boolean reset_start_limit = False;
	Boolean reset_end_limit   = False;

	static  XmString space = NULL;


	/* This code is only executed once so use it to create the popup time limit
	 * setting menu as well as the space string. I could not use the automatic
	 * posting mechanism as I needed to know which button had been used to post
	 * the menu.
	 */
	if (!space)
	{
		space = XmStringCreateLocalized(" ");

		/* The indicator showing that the visible sequence does not start at the
		 * first depiction must be the first widget in the row column parent. The
		 * end indicator is created here but its position index resource will be
		 * reset to the end as new depiction buttons are added.
		 */
		seqStartBtn = XmVaCreateToggleButton(GW_sequenceSelector, "seqStartLimitIndicator",
			XmNmarginWidth, 2,
			XmNshadowThickness, 2,
			XmNindicatorOn, False,
			NULL);
		XtAddCallback(seqStartBtn, XmNvalueChangedCallback, time_limit_set_cb, (XtPointer) CL);

		seqEndBtn = XmVaCreateToggleButton(GW_sequenceSelector, "seqEndLimitIndicator",
			XmNmarginWidth, 2,
			XmNshadowThickness, 2,
			XmNindicatorOn, False,
			NULL);
		XtAddCallback(seqEndBtn, XmNvalueChangedCallback, time_limit_set_cb, (XtPointer) CU);

		create_limit_popup();
	}

	/* If the depiction is already in our timelist there is nothing to do */
	if( InTimeList(dt, GV_depict, GV_ndepict, &ndx) ) return;

	/* assign memory for the sequence arrays if required. These buttons are
	 * never deallocated, just unmanaged.
	 */
	if( GV_ndepict >= ndepictBtn )
	{
		ndepictBtn++;
		GV_depict = MoreStringArray(GV_depict, ndepictBtn);
		GV_depict[GV_ndepict] = XtMalloc(sizeof(TSTAMP));
		depictBtn = MoreWidgetArray(depictBtn,ndepictBtn);
		depictBtn[GV_ndepict] = XmVaCreateToggleButton(GW_sequenceSelector, "dpBtn",
			XmNlabelString, space,
			XmNmarginWidth, 5,
			XmNshadowThickness, 2,
			XmNindicatorOn, False,
			NULL);

		XtAddCallback(depictBtn[GV_ndepict], XmNvalueChangedCallback,
			depiction_button_cb, INT2PTR(GV_ndepict));

		/* This event handler on the button will popup a context menu that allows
		 * setting the button to T0 or setting limits on the depiction sequence
		 * that reduce the number of displayed buttons. Useful for those who have
		 * a lot of depictions. See depiction_button_cb function.
		 */
		XtAddEventHandler(depictBtn[GV_ndepict], ButtonPressMask, False,
			time_limits_popup_handler, INT2PTR(GV_ndepict));

		/* The end of sequence control button needs to be repositioned
		 * so that it is always at the end
		 */
		XtVaSetValues(seqEndBtn, XmNpositionIndex, XmLAST_POSITION, NULL);
	}

	/* Check to see if we will need to adjust our sequence limits */
	if(GV_ndepict > 0)
	{
		reset_start_limit = (same(seq_start_time,GV_depict[0]) && MinuteDif(seq_start_time, dt) < 0);
		reset_end_limit   = (same(seq_end_time,GV_depict[GV_ndepict-1]) && MinuteDif(seq_end_time, dt) > 0);
	}

	/* Save the current active depiction time so we can restore the pointer
	 * to the active depiction
	 */
	(void) safe_strcpy(ts, active_depict);

	/* insert in the proper time order
	*/
	for( n = GV_ndepict-1; n >= 0; n-- )
	{
		if( MinuteDif(GV_depict[n],dt) > 0 ) break;
		(void) safe_strcpy(GV_depict[n+1],GV_depict[n]);
	}
	ndx = n + 1;
	(void) safe_strcpy(GV_depict[ndx], dt);

	/* Restore the active depiction pointer to the appropriate array element */
	for( n = 0; n <= GV_ndepict; n++ )
	{
		if(!matching_tstamps(GV_depict[n],ts)) continue;
		active_depict = GV_depict[n];
		break;
	}

	if(reset_start_limit) (void) safe_strcpy(seq_start_time, GV_depict[0]);
	if(reset_end_limit  ) (void) safe_strcpy(seq_end_time,   GV_depict[GV_ndepict]);

	GV_ndepict++;
	XtManageChildren(depictBtn, GV_ndepict);
}

/*  Callback to select the active depiction.  The number of the depiction
 *  is passed in client_data. 
*
*	Note: Due to problems in Linux the toggle control is handled directly
*	      by this function and not by the RowColumn widget.
*/
/* ARGSUSED */
static void depiction_button_cb(Widget w , XtPointer client_data , XtPointer unused )
{
	if(GV_ndepict < 1) return;

	XuSetDialogCursor(GW_mainWindow, XuBUSY_CURSOR, ON);
	if(!XmToggleButtonGetState(w))
	{
		/* If this is for the active depiction reselect it.
		 */
		if( w == depictBtn[active_depict_no] )
			XuToggleButtonSet(w, True, False);
	}
	else
	{
		int n;

		/* Set all other toggles to the off position
		 */
		for( n = 0; n < GV_ndepict; n++ )
		{
			if(XmToggleButtonGetState(depictBtn[n]) && w != depictBtn[n])
				XuToggleButtonSet(depictBtn[n], False, False);
		}

		active_depict_no = PTR2INT(client_data);
		active_depict = GV_depict[active_depict_no];

		if(GV_active_field && GV_active_field->depictFcn)
		{
			GV_active_field->depictFcn();
		}

		NotifyObservers(OB_DEPICTION_ABOUT_TO_CHANGE, NULL, 0);
		if(show_secondary_sequence_bar())
		{
			NotifyObservers(OB_DEPICTION_CHANGE, NULL, 0);
		}
		else
		{
			/* Issue the depiction command to ingred */
			(void) IngredVaCommand(GE_SEQUENCE, "ACTIVE %s %s", active_depict, active_sec_time);
			SetDepictionTimeDisplay(NULL);
			NotifyObservers(OB_DEPICTION_CHANGE, NULL, 0);
			ActivateMenu();
		}
	}
	XuSetDialogCursor(GW_mainWindow, XuBUSY_CURSOR, OFF);
}


/* Sets the times into the sequence selection buttons and controls the visibility
 * when the sequence start and end times differ from the depiction start and end.
 */
static void set_sequence_btn_time_display(void)
{
	if( GV_ndepict > 0 )
	{
		int    i;
		int    ndx = 0;
		TSTAMP dt;

		/* Unmanage the buttons while we are doing the relabel. The first one is left
		 * managed as this prevents the button manager itself resizing and causing
		 * display flashing when the buttons are managed.
		 */
		if( ndepictBtn > 1 ) XtUnmanageChildren(depictBtn+1, ndepictBtn-1);

		/* Preserve the active button selection when resetting.
		 */
		(void) safe_strcpy(dt, active_depict);
		for(i = 0; i < GV_ndepict; i++ )
		{
			XmToggleButtonSetState(depictBtn[i], False, False);
			SetSequenceBtnTime(depictBtn[i], GV_depict[i], MAIN_SEQUENCE);
			if(matching_tstamps(dt, GV_depict[i])) ndx = i;
		}
		XmToggleButtonSetState(depictBtn[ndx], True, False);
		XtVaSetValues(XtParent(depictBtn[ndx]), XmNmenuHistory, depictBtn[ndx], NULL);
		active_depict_no = ndx;
		XtManageChildren(depictBtn, GV_ndepict);

		if(valid_tstamp(seq_start_time) && valid_tstamp(seq_end_time))
		{
			int      ll  = 0;
			int      ul  = GV_ndepict;
			Cardinal mwc = 0;
			Cardinal uwc = 0;

			/* Restrict the range of the visible sequence buttons to those
			 * between the set time limits.
			 */
			if(allow_seq_limit)
			{
				WidgetList mwl = NewWidgetArray(GV_ndepict);
				WidgetList uwl = NewWidgetArray(GV_ndepict);
				for(i = 0; i < GV_ndepict; i++)
				{
					if(matching_tstamps(GV_depict[i], seq_start_time)) ll = i;
					else if(matching_tstamps(GV_depict[i], seq_end_time)) ul = i;
				}
				for(i = 0; i < GV_ndepict; i++)
				{
					if (i >= ll && i <= ul)
						mwl[mwc++] = depictBtn[i];
					else
						uwl[uwc++] = depictBtn[i];
				}
				XtManageChildren(mwl, mwc);
				XtUnmanageChildren(uwl, uwc);
				FreeItem(mwl);
				FreeItem(uwl);
			}
			else
			{
				XtManageChildren(depictBtn, GV_ndepict);
			}

			/* Display the sequence limited buttons.
			 */
			if(ll > 0) XtManageChild(seqStartBtn);
			else       XtUnmanageChild(seqStartBtn);

			if(ul < GV_ndepict-1) XtManageChild(seqEndBtn);
			else                  XtUnmanageChild(seqEndBtn);
		}
	}
	else
	{
		XtUnmanageChildren(depictBtn, ndepictBtn);
	}
	/*
	 * This function checks to see if the time buttons take up more space than is
	 * available and thus need to be scrolled. The relable could do this.
	 */
	CheckTimeButtonLayout();
}


/*====================== Time limit popup handler functions =========================*/


static void create_limit_popup(void)
{
	Arg    arg[1];
	Widget w;

	XtSetArg(arg[0], XmNpopupEnabled, XmPOPUP_DISABLED);
	timeLimitsPopup = XmCreatePopupMenu(GW_sequenceSelector, "timePopup", arg, 1);

	w = XmVaCreateManagedPushButton(timeLimitsPopup, "setToT0", NULL);
	XtAddCallback(w, XmNactivateCallback, time_limit_set_cb, (XtPointer) T0);

	(void) XmVaCreateManagedSeparator(timeLimitsPopup, "sep", NULL);

	setLowerLimit = XmVaCreateManagedPushButton(timeLimitsPopup, "seqLimitStart", NULL);
	XtAddCallback(setLowerLimit, XmNactivateCallback, time_limit_set_cb, (XtPointer) SL);

	setUpperLimit = XmVaCreateManagedPushButton(timeLimitsPopup, "seqLimitEnd", NULL);
	XtAddCallback(setUpperLimit, XmNactivateCallback, time_limit_set_cb, (XtPointer) SU);

	(void) XmVaCreateManagedSeparator(timeLimitsPopup, "sep", NULL);

	clearLowerLimit = XmVaCreateManagedPushButton(timeLimitsPopup, "clearLimitStart", NULL);
	XtAddCallback(clearLowerLimit, XmNactivateCallback, time_limit_set_cb, (XtPointer) CL);

	clearUpperLimit = XmVaCreateManagedPushButton(timeLimitsPopup, "clearLimitEnd", NULL);
	XtAddCallback(clearUpperLimit, XmNactivateCallback, time_limit_set_cb, (XtPointer) CU);

}

/* This handler, popped up from a "right" mouse button click, is for setting the upper and
 * lower time limits on which sequence buttons will be displayed on the time bar. One
 * can clear the setting or set the button as the upper or lower limit.
 */
/* ARGSUSED */
static void time_limits_popup_handler(Widget w, XtPointer data, XEvent *event, Boolean *doit)
{
	if(event->type != ButtonPress) return;
	if(event->xbutton.button != Button3) return;
	
	if(GV_ndepict < 1) return;

	menu_seq_btn = PTR2INT(data);

	if (!valid_tstamp(seq_start_time)) (void) safe_strcpy(seq_start_time, GV_depict[0]);
	if (!valid_tstamp(seq_end_time)  ) (void) safe_strcpy(seq_end_time,   GV_depict[GV_ndepict-1]);
	
	XmMenuPosition(timeLimitsPopup, (XButtonEvent *) event);
	XtManageChild(timeLimitsPopup);
}


/* This callback responds to the selection made in the time limit popup.
 */
/* ARGSUSED */
static void time_limit_set_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int  i, ll, ul;

	/* We are cheating in the case of these buttons and using them as push buttons and
	 * not toggle buttons although they are in the sequence row column manager, thus we
	 * must reset the state of the toggles.
	 */
	if(w == seqStartBtn || w == seqEndBtn)
	{
		XuToggleButtonSet(w, False, False);
		XuToggleButtonSet(depictBtn[active_depict_no], True, False);
	}

	switch(PTR2INT(client_data))
	{
		case CL: (void) safe_strcpy(seq_start_time, GV_depict[0]           ); break; /* clear lower limit */
		case SL: (void) safe_strcpy(seq_start_time, GV_depict[menu_seq_btn]); break; /* set lower limit */
		case CU: (void) safe_strcpy(seq_end_time,   GV_depict[GV_ndepict-1]); break; /* clear upper limit */
		case SU: (void) safe_strcpy(seq_end_time,   GV_depict[menu_seq_btn]); break; /* set upper limit */
		case T0: SetT0Depiction(menu_seq_btn);                                break; /* set to T0 */
	}

	/* If the active depiction is now not visible we need to set it
	 * to one in the visible range.
	 */
	for(ll = ul = GV_ndepict, i = 0; i < GV_ndepict; i++)
	{
		if(matching_tstamps(GV_depict[i], seq_start_time)) ll = i;
		else if(matching_tstamps(GV_depict[i], seq_end_time)) ul = i;
	}
	if(active_depict_no < ll)
		XuToggleButtonSet(depictBtn[ll], True, True);
	else if(active_depict_no > ul)
		XuToggleButtonSet(depictBtn[ul], True, True);

	set_sequence_btn_time_display();
}
