/**********************************************************************/
/** @file clock.h
 *
 *  Routines to manipulate time (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*    c l o c k . h                                                     *
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

void	read_clock(void);
void	save_clock(void);
void	restore_clock(void);
void	bogus_clock(int year, int jday, int month, int day, int hour,
                    int minute, int dst_flag);

int		full_year(int abbrev, LOGICAL *converted);
void	gtime(int *year, int *jday, int *month, int *day, int *wday,
              int *hour, int *minute);
void	ltime(int *year, int *jday, int *month, int *day, int *wday,
              int *hour, int *minute);
void	g_offset(int *hoff, int *moff);
int		daylite(void);
void	convert_gmt_to_local(int gyear, int gjday, int gmonth, int gday,
                             int ghour, int gminute, int *year, int *jday,
                             int *month, int *day, int *wday, int *hour,
                             int *minute, char **tzstamp, int *dstflag);
void	convert_local_to_gmt(int lyear, int ljday, int lmonth, int lday,
                             int lhour, int lminute, int dst_flag, int *year,
                             int *jday, int *month, int *day, int *wday,
                             int *hour, int *minute);
