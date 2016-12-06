/**********************************************************************/
/** @file forecasts.h
 *
 *  Routines to interpret the "Forecasts" config file (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   f o r e c a s t s . h                                              *
*                                                                      *
*   Routines to interpret the "Forecsats" config file (include file)   *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2002 Environment Canada (MSC)            *
*     Version 7 (c) Copyright 2006 Environment Canada                  *
*     Version 8 (c) Copyright 2011 Environment Canada                  *
*                                                                      *
*   This file is part of the Forecast Production Assistant (FPA).      *
*   The FPA is free software: you can redistribute it and/or modify it *
*   under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation, either version 3 of the License, or  *
*   any later version.                                                 *
*                                                                      *
*   The FPA is distributed in the hope that it will be useful, but     *
*   WITHOUT ANY WARRANTY; without even the implied warranty of         *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.               *
*   See the GNU General Public License for more details.               *
*                                                                      *
*   You should have received a copy of the GNU General Public License  *
*   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.   *
*                                                                      *
***********************************************************************/

/* See if already included */
#ifndef FORECASTS_DEFS
#define FORECASTS_DEFS


/* We need definitions for low level types and other Objects */
#include <fpa_types.h>


/***********************************************************************
*                                                                      *
*  Initialize defined constants                                        *
*                                                                      *
***********************************************************************/

#ifdef FORECASTS_INIT


#endif


/***********************************************************************
*                                                                      *
*  Declare external functions in forecasts.c                           *
*                                                                      *
***********************************************************************/

STRING	FoG_file(STRING, STRING, STRING, int);
STRING	FoG_setup_file(STRING, STRING, STRING);
STRING	FoG_setup_parm(STRING, STRING);
LOGICAL	FoG_copy_files(STRING, STRING);
LOGICAL	FoG_override_tz(STRING);
LOGICAL	FoG_prev_issue_time(STRING, int, int, int, int, int, int, int,
						int *, int *, int *, int *, int *, int *, int *);
LOGICAL	FoG_next_issue_time(STRING, int, int, int, int, int, int, int,
						int *, int *, int *, int *, int *, int *, int *);
LOGICAL	FoG_issue_times(STRING, int *, int *, int *, int *, int *, int *,
						int *, int *, int *, int *, int *, int *, int *, int *);
LOGICAL	FoG_issue_last(STRING, int *, int *, int *, int *, int *, int *, int *);
LOGICAL	FoG_WeatherEntryValid(STRING, STRING *, STRING *, int *, int *);


/* Now it has been included */
#endif
