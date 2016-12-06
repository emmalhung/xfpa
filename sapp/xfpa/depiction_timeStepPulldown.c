/****************************************************************************/
/*
*  File:     depiction_timeStepPulldown.c 
*
*  Purpose:  Creates the list of time steps which appears in the options
*            pulldown menu item "Minimum Time Step"
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

#include <stdlib.h>
#include <Xm/ToggleB.h>
#include "global.h"
#include "menu.h"
#include "resourceDefines.h"


/*ARGSUSED*/
static void step_change_cb( Widget w, XtPointer client_data, XtPointer call_data )
{
	int ndx = PTR2INT(client_data);
	GV_increment_step = GV_seq_delta[ndx].value;
	XuVaStateDataSave(SEQ_DELTA_STATE_KEYS, "%d", GV_increment_step, NULL);
	XtVaSetValues(GW_depictMinTimeStepLabel, XmNlabelString, GV_seq_delta[ndx].step_label, NULL);
}


/* The XmString parameter returned is the label for the currently active
 * time step setting. This is used to set the label of the indicator as
 * it is created after this function is called
 */
void CreateTimeStepPulldown(XmString *step_label)
{
	int    n, ndx, delmin;
	char   buf[500];
	String ptr, labels[30];
	Widget btn, pulldown;
	SETUP  *s;

	/* Get the sequence increment list from the setup or resource file. The first
	 * entry is preset to step through all of the depictions and is preset
	 * and immutable. Note that the time entry for the normal increment
	 * is always forced to 0.
	 */
	(void) strncpy(buf, XuGetStringResource(RNnormalIncrementLabel, "Sequence,S"), 500);
	ptr = strtok(buf,",");
	no_white(ptr);
	labels[0] = XtNewString(ptr);
	ptr = strtok(NULL,",");
	no_white(ptr);
	GV_nseq_delta = 1;
	GV_seq_delta = OneMem(SEQDELTA);
	GV_seq_delta[0].step_label = XmStringCreateLocalized(ptr);
	GV_seq_delta[0].value = 0;

	if((s = GetSetup(DEPICT_TIME_STEPS)) && s->nentry > 0)
	{
		for(n = 0; n < s->nentry; n++)
		{
			if(s->entry[n].nparms < 3) continue;
			GV_seq_delta = MoreMem(GV_seq_delta, SEQDELTA, GV_nseq_delta+1);
			labels[GV_nseq_delta] = XtNewString(SetupParm(s,n,0));
			GV_seq_delta[GV_nseq_delta].step_label = XmStringCreateLocalized(SetupParm(s,n,1));
			GV_seq_delta[GV_nseq_delta].value = atoi(SetupParm(s,n,2));
			GV_nseq_delta++;
		}
	}
	else
	{
		(void) strncpy(buf, XuGetStringResource(RNsequenceIncrements, "1 Hour,1H,60"), 500);
		ptr = strtok(buf,",");
		while(ptr)
		{
			GV_seq_delta = MoreMem(GV_seq_delta, SEQDELTA, GV_nseq_delta+1);
			no_white(ptr);
			labels[GV_nseq_delta] = XtNewString(ptr);
			if(!(ptr = strtok(NULL,","))) break;
			no_white(ptr);
			GV_seq_delta[GV_nseq_delta].step_label = XmStringCreateLocalized(ptr);
			if(!(ptr = strtok(NULL,","))) break;
			(void) sscanf(ptr, "%d", &GV_seq_delta[GV_nseq_delta].value);
			GV_nseq_delta++;
			ptr = strtok(NULL,",");
		}
	}

	/* Do we have a current setting in the resource file
	 */
	ndx = 0;
	*step_label = GV_seq_delta[0].step_label;

	if(XuVaStateDataGet(SEQ_DELTA_STATE_KEYS, "%d", &delmin, NULL))
	{
		for( n = 0; n < GV_nseq_delta; n++ )
		{
			if(delmin != GV_seq_delta[n].value) continue;
			ndx = n;
			GV_increment_step = delmin;
			*step_label = GV_seq_delta[n].step_label;
			break;
		}
	}

	/* Create the pulldown toggle list used in the main menu bar
	 */
	pulldown = XuMenuFind(GW_menuBar, MENU_Option_timeStep);

	for( n = 0; n < GV_nseq_delta; n++ )
	{
		btn = XmVaCreateManagedToggleButton(pulldown, labels[n],
			XmNset, (ndx == n),
#ifdef INDICATOR_SIZE
			XmNindicatorSize, INDICATOR_SIZE,
#endif
			NULL);
		XtAddCallback(btn, XmNvalueChangedCallback, step_change_cb, INT2PTR(n));
		FreeItem(labels[n]);
	}
}
