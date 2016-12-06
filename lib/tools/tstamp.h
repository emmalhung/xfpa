/**********************************************************************/
/** @file tstamp.h
 *
 * Functions to retrieve and evaluate valid time for programs.(include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*    t s t a m p . h                                                   *
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

#include <fpa_types.h>

/* See if already included */
#ifndef TSTAMP_INCLUDED

#undef USE_MINUTES

/* Define TSTAMP structure to store time stamps */
typedef char	TSTAMP[20];

STRING	calc_valid_time (STRING rstamp, int prog);
int		calc_prog_time (STRING rstamp, STRING vstamp, LOGICAL *status);

STRING	calc_valid_time_minutes (STRING rstamp, int phour, int pmin);
int		calc_prog_time_minutes (STRING rstamp, STRING vstamp, LOGICAL *status);
STRING	hour_minute_string (int phour, int pmin);
int		interpret_hour_minute_string (STRING hmstring);

LOGICAL	parse_tstamp (STRING tstamp, int *year, int *jday, int *hour,
						int *min, LOGICAL *local, LOGICAL *mins);
STRING	build_tstamp (int year, int jday , int hour, int min, LOGICAL local,
						LOGICAL mins);
LOGICAL	valid_tstamp (STRING tstamp);

STRING	interpret_timestring (STRING tstring, STRING rstamp, float lon);
STRING	metafile_timestring (int year, int jday , int hour, int min, int sec,
						LOGICAL local, LOGICAL mins, LOGICAL secs);

STRING	local_to_gmt (STRING tstamp, float lon);
STRING	gmt_to_local (STRING tstamp, float lon);

int		compare_tstamps (STRING ts1, STRING ts2, float lon);
LOGICAL	matching_tstamps (STRING ts1, STRING ts2);
int		closest_tstamp (int nvt, STRING *vts, STRING vtime, float lon,
						STRING before, STRING after, int *first, int *last);
STRING	tstamp_to_minutes (STRING tstamp, int *cmp);
STRING	tstamp_to_hours (STRING tstamp, LOGICAL round, int *cmp);

/* Obsolescent functions */
int		read_tstamp (STRING tstamp, int *year, int *jday , int *hour);
STRING	make_tstamp (int year, int jday , int hour);
STRING	make_local_tstamp (int year, int jday , int hour, float lon);

/* Obsolescent functions */
STRING	lmt_to_lmt (STRING tstamp, float lon1, float lon2);
STRING	lmt_to_gmt (STRING tstamp, float lon);
STRING	gmt_to_lmt (STRING tstamp, float lon);

/* Now it has been included */
#	define TSTAMP_INCLUDED
#endif
