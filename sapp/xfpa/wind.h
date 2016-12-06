/*========================================================================*/
/*
*	File:		wind.h
*
*   Purpose:    Header for wind functions.
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
#ifndef _MEMORY_H
#define _MEMORY_H

typedef struct _memory {
	CAL set_modify_cal;	/* holds CAL to use during modify  */
	int ncal;			/* number of active memory arrays  */
	CAL *cal;			/* wind information as CAL structs */
} WINDMEM;

typedef void(*WINDFCN)(WIND_CALC*);

extern void ACTIVATE_bkgndWindEntryDialog (Widget, WINDFCN);
extern void ACTIVATE_windEntryDialog      (Widget, CAL, WINDFCN, WINDFCN);
extern void UpdateWindEntryDialog         (CAL);
extern void DestroyWindEntryDialog        (void);
extern void DestroyBkgndWindEntryDialog   (void);

#endif	/* _MEMORY_H */
