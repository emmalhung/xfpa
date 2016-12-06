/*================================================================*/
/*
*	File: timelists.c
*
*   Purpose: To create and destroy lists of run and valid times.
*            The creation function returns a pointer to a time
*            structure. This saves memory if many calls point
*            to the same time structure.
*
*	Functions:
*
*		RunTimeEntry   CreateRunTimeEntry   (String *)
*       RunTimeEntry   CopyRunTimeEntry     (RunTimeEntry)
*       String         RunTime              (RunTimeEntry)
*       void           RemoveRunTimeEntry   (RunTimeEntry)
*
*       ValidTimeEntry CreateValidTimeEntry  (String *, int)
*       void           GetValidTimes         (ValidTimeEntry, String**, int*);
*       void           RemoveValidTimeEntry  (ValidTimeEntry)
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
/*================================================================*/

#include "global.h"
#include "timelists.h"


/*===================== Run Time Functions ===========================*/

static int          nrl = 0;
static RunTimeEntry *rl = (RunTimeEntry *)NULL;

RunTimeEntry CreateRunTimeEntry(String dt)
{
	int i;

	for(i = 0; i < nrl; i++)
	{
		if(matching_tstamps(rl[i]->time,dt) || (blank(dt) && IsNull(rl[i]->time)))
		{
			rl[i]->nref++;
			return rl[i];
		}
	}
	for(i = 0; i < nrl; i++)
	{
		if(rl[i]->nref > 0) continue;
		FreeItem(rl[i]->time);
		break;
	}
	if(i >= nrl)
	{
		nrl++;
		rl = MoreMem(rl, RunTimeEntry, nrl);
		rl[i] = OneMem(RunTimeStruct);
	}
	rl[i]->nref = 1;
	rl[i]->time = (String)NULL;

	if(!blank(dt))
	{
		int     year, jday, hour, minute;
		LOGICAL local, mins;
		(void) parse_tstamp(dt, &year, &jday, &hour, &minute, &local, &mins);
		rl[i]->time = XtNewString(build_tstamp(year, jday, hour, minute, local, mins));
	}
	return rl[i];
}


RunTimeEntry CopyRunTimeEntry(RunTimeEntry rte)
{
	int i;

	for(i = 0; i < nrl; i++)
	{
		if(rte == rl[i])
		{
			rl[i]->nref++;
			return rl[i];
		}
	}
	return (RunTimeEntry)NULL;
}

String RunTime(RunTimeEntry rte)
{
	if(IsNull(rte) || rte->nref < 1 || !valid_tstamp(rte->time)) return (String)NULL;
	return rte->time;
}

void RemoveRunTimeEntry(RunTimeEntry tep)
{
	if(tep && tep->nref > 0) tep->nref--;
}


/*===================== Valid Time Functions ===========================*/


static int            nvl = 0;
static ValidTimeEntry *vl = (ValidTimeEntry *)NULL;

ValidTimeEntry CreateValidTimeEntry(String *vt , int nvt)
{
	int     i, j;
	String  buf;

	/* Look for an existing series */
	for(i = 0; i < nvl; i++)
	{
		if(vl[i]->ntimes != nvt) continue;
		for(j = 0; j < nvt; j++)
		{
			/* 2005.04.18: The matching_stamps() function is not used as the
			 * valid time list can have "magic" entries as used by guidance
			 */
			if(!same(vl[i]->times[j], vt[j])) break;
		}
		/* 2005/02/23 was break but should be continue */
		if(j < nvt) continue;
		vl[i]->nref++;
		return vl[i];
	}

	/* Look for an existing position to reuse */
	for(i = 0; i < nvl; i++)
	{
		if(vl[i]->nref > 0) continue;
		if(IsNull(vl[i]->times)) break;
		FreeItem(vl[i]->times[0]);
		FreeItem(vl[i]->times);
		break;
	}

	/* More entry array needed */
	if(i >= nvl)
	{
		nvl++;
		vl = MoreMem(vl, ValidTimeEntry, nvl);
		vl[i] = OneMem(ValidTimeStruct);
	}

	/* Initialize and fill in array */
	vl[i]->nref   = 1;
	vl[i]->ntimes = nvt;
	vl[i]->times  = (String *)NULL;
	if(nvt > 0)
	{
		/* As TSTAMP is fixed just allocate one big array */
		buf = XtMalloc(nvt*sizeof(TSTAMP));
		vl[i]->times = NewStringArray(nvt);
		for(j = 0; j < nvt; j++)
		{
			vl[i]->times[j] = buf+(j*sizeof(TSTAMP));
			/* 2005.04.18: strcpy() used for same reason as previous comment */
			(void) safe_strcpy(vl[i]->times[j], vt[j]);
		}
	}
	return vl[i];
}


void GetValidTimes(ValidTimeEntry vte, String **dt, int *nt)
{
	if (dt) *dt = (String *)NULL;
	if (nt) *nt = 0;

	if(NotNull(vte) && vte->nref > 0)
	{
		if (dt) *dt = vte->times;
		if (nt) *nt = vte->ntimes;
	}
}


void RemoveValidTimeEntry(ValidTimeEntry tep)
{
	if(tep && tep->nref > 0) tep->nref--;
}
