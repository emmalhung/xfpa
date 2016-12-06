/*========================================================================*/
/*
*	File:		timelists.h
*
*   Purpose:    Header for timelist functions.
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
#ifndef TIMELISTS_H
#define TIMELISTS_H

typedef struct _RunTimeStruct {
	int nref;
	String time;
} RunTimeStruct, *RunTimeEntry;

typedef struct _ValidTimeStruct {
	int nref;
	int ntimes;
	String *times;
} ValidTimeStruct, *ValidTimeEntry;

/* These are some handy defines to make the code clearer but where
 * a separate function is not needed,
 */
#define CreateEmptyRunTimeEntry()	CreateRunTimeEntry((String)NULL)
#define CreateEmptyValidTimeEntry()	CreateValidTimeEntry((String*)NULL,0)
#define InValidTimeEntry(a,b,c)		InTimeList(a,b->times,b->ntimes,c)

extern RunTimeEntry   CreateRunTimeEntry  (String);
extern RunTimeEntry   CopyRunTimeEntry    (RunTimeEntry);
extern String         RunTime             (RunTimeEntry);
extern void           RemoveRunTimeEntry  (RunTimeEntry);

extern ValidTimeEntry CreateValidTimeEntry  (String *, int);
extern void           GetValidTimes         (ValidTimeEntry, String**, int*);
extern void           RemoveValidTimeEntry  (ValidTimeEntry);

#endif /* TIMELISTS_H */
