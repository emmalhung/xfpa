/**********************************************************************/
/** @file time.h
 *
 * Routines to convert dates and times. (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*    t i m e . h                                                       *
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

void	systime (int *year, int *jday, int *hour, int *minute, int *second);

void	jdate (int *year, int *month, int *mday, int *jday);
void	mdate (int *year, int *jday, int *month, int *mday);

void	tnorm (int *year, int *jday, int *hour, int *minute, int *second);
void	jnorm (int *year, int *jday);
void	mnorm (int *year, int *month, int *mday);

int		jorder (int year1, int jday1, int year2, int jday2);
int		sdif (int year1, int jday1, int hour1, int min1, int sec1,
			int year2, int jday2, int hour2, int min2, int sec2);
int		mdif (int year1, int jday1, int hour1, int min1,
			int year2, int jday2, int hour2, int min2);
int		hdif (int year1, int jday1, int hour1, int year2, int jday2, int hour2);
int		jdif (int year1, int jday1, int year2, int jday2);

STRING	weekday (int year, int jday);
int		wkday (int year, int jday);

int		ndmonth (int year, int month);
int		ndyear (int year);
LOGICAL	leap (int year);

void	decode_clock (long clockval, int *year, int *jday, int *hour,
			int *minute, int *second);
long	encode_clock (int year, int jday, int hour, int minute, int second);

