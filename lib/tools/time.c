/*********************************************************************/
/** @file time.c
 *
 * Routines to convert dates and times.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*     t i m e . c                                                      *
*                                                                      *
*     Routines to convert dates and times.                             *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
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

#include "time.h"

#include <fpa_math.h>
#include <fpa_types.h>

#include <time.h>

static	int jmonth(int, int);
static	int vnorm(int *, int, int);

/***********************************************************************
*                                                                      *
*     s y s t i m e                                                    *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** This subroutine returns the current system time in gmt, using
 * the system "time" routine.
 *	@param[out]	*year	current year
 *	@param[out]	*jday	current day of year
 *	@param[out]	*hour	current hour
 *	@param[out]	*minute	current minute
 *	@param[out]	*second	current second
 *********************************************************************/

void	systime

	(
	int	*year,
	int	*jday,
	int	*hour,
	int	*minute,
	int	*second
	)

	{
	long	cval;		/* current clock value */

	cval = (long) time((time_t *)0);
	decode_clock(cval, year, jday, hour, minute, second);
	}

/***********************************************************************
*                                                                      *
*     j d a t e                                                        *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** This subroutine converts a given conventional date (year, month
 * and day of month) to a Julian date (year and day of year).
 *	@param[in]	*year	year (normalized if necessary)
 *	@param[in]	*month	month (normalized if necessary)
 *	@param[in]	*mday	day of month (normalized if necessary)
 *	@param[out]	*jday	day of year
 *********************************************************************/

void	jdate

	(
	int	*year,
	int	*month,
	int	*mday,
	int	*jday
	)

	{
	if (!year || !month || !mday || !jday) return;

	/* Normalize given date */
	mnorm(year, month, mday);

	/* Add day of month to end of preceding month */
	*jday = *mday;
	if (*month > 1) *jday += jmonth(*year, (*month)-1);

	/* Correct for 11 days omitted from September 3 to 13 1752, */
	/* when the Gregorian calendar was adopted */
	if ((*year == 1752) && (*month == 9) && (*mday >= 14)) *jday -= 11;
	}

/***********************************************************************
*                                                                      *
*     m d a t e                                                        *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** This subroutine converts a given Julian date (year and day of
 * year) to a conventional date (year, month and day of month).
 *	@param[in]	*year	year (normalized if necessary)
 *	@param[in]	*jday	day of year (normalized if necessary)
 *	@param[out]	*month	month
 *	@param[out]	*mday	day of month
 *********************************************************************/

void	mdate

	(
	int	*year,
	int	*jday,
	int	*month,
	int	*mday
	)

	{
	if (!year || !jday || !month || !mday) return;

	/* Normalize given date */
	jnorm(year, jday);

	/* Find which month ends after the given Julian day */
	for (*month = 1; *month <= 12; (*month)++)
		if (jmonth(*year, *month) >= *jday) break;

	/* Subtract last day of previous month to get date */
	*mday = *jday;
	if (*month > 1) *mday -= jmonth(*year, (*month)-1);

	/* Correct for 11 days omitted from September 3 to 13 1752, */
	/* when the Gregorian calendar was adopted */
	if ((*year == 1752) && (*month == 9) && (*mday >= 3)) *mday += 11;
	}

/***********************************************************************
*                                                                      *
*     t n o r m                                                        *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Normalize a set of date-time values so that each element is
 * within the correct range.  If a value is out of range the value
 * above it is incremented or decremented in order to bring the
 * offending value within range.
 *	@param[in,out]	*year	year [Any year (AD)]
 *	@param[in,out]	*jday	julian day [1 to 365(366 in leap years)]
 *	@param[in,out]	*hour	hour [0 to 23]
 *	@param[in,out]	*minute	minute [0 to 59]
 *	@param[in,out]	*second	second [0 to 5]
 *********************************************************************/

void	tnorm

	(
	int	*year,
	int	*jday,
	int	*hour,
	int	*minute,
	int	*second
	)

	{
	/* Normalize seconds */
	if (second && minute) *minute += vnorm(second, 0, 59);
	/* Normalize minutes */
	if (minute && hour)   *hour   += vnorm(minute, 0, 59);
	/* Normalize hours */
	if (hour && jday)     *jday   += vnorm(hour  , 0, 23);

	/* Normalize the date */
	if (jday && year) jnorm(year, jday);
	}

/***********************************************************************
*                                                                      *
*     j n o r m                                                        *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** This subroutine normalizes a julian date, (year and day-of-year).
 * If the day is out of range (negative, or greater than length of
 * given year), then the year is incremented or decremented accordingly.
 *	@param[in,out]	*year	year
 *	@param[in,out]	*jday	julian day
 *********************************************************************/

void	jnorm

	(
	int	*year,
	int	*jday
	)

	{
	int	yold, jold;

	if (!year || !jday) return;

	/* Repeat until date is normalized correctly */
	/* May have to repeat if enough leap days were introduced on */
	/* the first correction to push jday into another year again */
	while ( (*jday <= 0) || (*jday > ndyear(*year)) )
		{
		/* Estimate normalized date based on 365 day years */
		yold   = *year;
		jold   = *jday;
		*year += vnorm(jday, 1, 365);

		/* Force a correction for years with less than 365 days */
		/* (i.e. 1752 and the fictitious year 0) */
		if (*year == yold)
		{
		*jday -= ndyear(*year);
		(*year)++;
		}

		/* Adjust for leap years between estimated and original dates */
		*jday -= jdif(yold, jold, *year, *jday);
		}
	}

/***********************************************************************
*                                                                      *
*     m n o r m                                                        *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** This subroutine normalizes a conventional date, (year, month
 * and day-of-month).  If the day is out of range, then the month
 * is incremented or decremented accordingly. If the month is out
 * of range, then the year is incremented or decremented.
 *	@param[in,out]	*year	year
 *	@param[in,out]	*month	month
 *	@param[in,out]	*mday	day of month
 *********************************************************************/

void	mnorm

	(
	int	*year,
	int	*month,
	int	*mday
	)

	{
	int	nd;

	if (!year || !month || !mday) return;

	/* Normalize the month first */
	*year += vnorm(month, 1, 12);

	/* Remove correction for 11 days omitted from September 3 to 13 */
	/* 1752, when the Gregorian calendar was adopted */
	if ((*year == 1752) && (*month == 9) && (*mday >= 14)) *mday -= 11;

	/* Day is before start of month */
	/* Repeat until everything is normalized */
	while (*mday <= 0)
		{
		/* Decrement and re-normalize the month */
		(*month)--;
		if (*month <= 0) *year += vnorm(month, 1, 12);

		/* Increment day */
		*mday += ndmonth(*year, *month);
		}

	/* Day is after end of month */
	/* Repeat until everything is normalized */
	while (*mday > (nd=ndmonth(*year, *month)))
		{
		/* Decrement day */
		*mday -= nd;

		/* Increment and re-normalize the month */
		(*month)++;
		if (*month > 12) *year += vnorm(month, 1, 12);
		}

	/* Correct for 11 days omitted from September 3 to 13 1752, */
	/* when the Gregorian calendar was adopted */
	if ((*year == 1752) && (*month == 9) && (*mday >= 3)) *mday += 11;
	}

/***********************************************************************
*                                                                      *
*     j o r d e r                                                      *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** This subroutine returns a code to indicate which of two given
 * dates (dates must be given in Julian form) precedes the other.
 *
 *	@param[in]	year1	first year
 *	@param[in]	jday1   first jday
 *	@param[in]	year2	second year
 *	@param[in]	jday2 	second jday
 *  @return
 * 	- +1 --> first date precedes second
 * 	- 0 ---> both dates are the same
 * 	- -1 --> second date precedes first
 *********************************************************************/

int	jorder

	(
	int	year1,
	int	jday1,
	int	year2,
	int	jday2
	)

	{
	if (year1 < year2) return 1;
	if (year1 > year2) return -1;
	if (jday1 < jday2) return 1;
	if (jday1 > jday2) return -1;
	return 0;
	}

/***********************************************************************
*                                                                      *
*     j d i f   (number of days between dates)                         *
*     h d i f   (number of hours between date-times)                   *
*     m d i f   (number of minutes between date-times)                 *
*     s d i f   (number of seconds between date-times)                 *
*                                                                      *
*     These subroutines return the number of days, hours, minutes or   *
*     seconds between two given date-times (dates must be given in     *
*     Julian form).                                                    *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Calculate the number of seconds between the given date-times.
 *
 * @param[in] year1 first date-time year
 * @param[in] jday1 first date-time julian day
 * @param[in] hour1 first date-time hour of the day
 * @param[in] min1  first date-time minute of the hour
 * @param[in] sec1  first date-time second of the minute
 * @param[in] year2 second date-time year
 * @param[in] jday2 second date-time julian day
 * @param[in] hour2 second date-time hour of the day
 * @param[in] min2  second date-time minute of the hour
 * @param[in] sec2  second date-time second of the minute
 * @return Number of seconds between date-times.
 *********************************************************************/
int	sdif

	(
	int	year1 ,
	int	jday1 ,
	int	hour1 ,
	int	min1 ,
	int	sec1 ,
	int	year2 ,
	int	jday2 ,
	int	hour2 ,
	int	min2 ,
	int	sec2
	)

	{
	return (sec2 - sec1 + 60*mdif(year1, jday1, hour1, min1,
								  year2, jday2, hour2, min2));
	}

/*********************************************************************/
/** Calculate the number of minutes between the given date-times.
 *
 * @param[in] year1 first date-time year
 * @param[in] jday1 first date-time julian day
 * @param[in] hour1 first date-time hour of the day
 * @param[in] min1  first date-time minute of the hour
 * @param[in] year2 second date-time year
 * @param[in] jday2 second date-time julian day
 * @param[in] hour2 second date-time hour of the day
 * @param[in] min2  second date-time minute of the hour
 * @return Number of minutes between date-times.
 *********************************************************************/
int	mdif

	(
	int	year1 ,
	int	jday1 ,
	int	hour1 ,
	int	min1 ,
	int	year2 ,
	int	jday2 ,
	int	hour2 ,
	int	min2
	)

	{
	return (min2 - min1 + 60*hdif(year1, jday1, hour1, year2, jday2, hour2));
	}

/*********************************************************************/
/** Calculate the number of hours between the given date-times.
 *
 * @param[in] year1 first date-time year
 * @param[in] jday1 first date-time julian day
 * @param[in] hour1 first date-time hour of the day
 * @param[in] year2 second date-time year
 * @param[in] jday2 second date-time julian day
 * @param[in] hour2 second date-time hour of the day
 * @return Number of hours between date-times.
 *********************************************************************/
int	hdif

	(
	int	year1 ,
	int	jday1 ,
	int	hour1 ,
	int	year2 ,
	int	jday2 ,
	int	hour2
	)

	{
	return (hour2 - hour1 + 24*jdif(year1, jday1, year2, jday2));
	}

/*********************************************************************/
/** Calculate the number of days between the given dates.
 *
 * @param[in] year1 first date-time year
 * @param[in] jday1 first date-time julian day
 * @param[in] year2 second date-time year
 * @param[in] jday2 second date-time julian day
 * @return Number of days between given dates.
 *********************************************************************/
int	jdif

	(
	int	year1,
	int	jday1,
	int	year2,
	int	jday2
	)

	{
	int	n, y;

	/* Compute difference between dates as if the same year */
	n = jday2 - jday1;

	/* Add in the difference between the two years */
	/* Positive if first date precedes second */
	if (year2 > year1)
		for (y = year1; y < year2; y++)
			n += ndyear(y);

	/* Negative if second date precedes first */
	else if (year1 > year2)
		for (y = year2; y < year1; y++)
			n -= ndyear(y);

	return n;
	}

/***********************************************************************
*                                                                      *
*     w e e k d a y                                                    *
*                                                                      *

*                                                                      *
*     w k d a y                                                        *
*                                                                      *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Find the weekday name of the given Julian date.
 *
 * @param[in] year year to check
 * @param[in] jday Julian day to check
 * @return Pointer into a static array within the function. One of:
 * 	- sun = sunday
 * 	- mon = monday
 * 	- tue = tuesday
 * 	- wed = wednesday
 * 	- thu = thursday
 * 	- fri = friday
 * 	- sat = saturday
 *********************************************************************/
STRING	weekday

	(
	int	year,
	int	jday
	)

	{
	static	STRING	dname[]
					= { "sun", "mon", "tue", "wed", "thu", "fri", "sat" };

	return dname[wkday(year, jday) - 1];
	}

/*********************************************************************/
/** This subroutine returns a weekday code for the given Julian date.
 *
 * @param[in] year year to check
 * @param[in] jday Julian day to check
 * @return One of:
 * 	- 1 = sunday
 * 	- 2 = monday
 * 	- 3 = tuesday
 * 	- 4 = wednesday
 * 	- 5 = thursday
 * 	- 6 = friday
 * 	- 7 = saturday
 *********************************************************************/
int	wkday

	(
	int	year,
	int	jday
	)

	{
	int	nd, yy;

	/* The imaginary year 0 would have begun on a thursday */
	nd = 5;

	/* Add 365 mod 7 (=1) for each year since year 0 */
	nd += year;

	/* Add an extra day for each leap year prior to 1752 */
	/* (every 4th year) */
	yy  = (year>1752)? 1751: year-1;
	nd += yy/4 + 1;

	if (year > 1752)
		{
		/* Take off the 11 days that were removed in 1752 */
		nd -= 11;

		/* Add an extra day for each leap year since 1752 */
		/* (every 4th year except 3 out of 4 centuries) */
		yy  = year -1;
		nd += 1 + (yy-1752)/4 - (yy-1700)/100 + (yy-1600)/400;
		}

	/* Add day of year less 1 */
	nd += jday - 1;

	/* Return the result modulo 7 */
	return ( (nd-1)%7 + 1);
	}

/***********************************************************************
*                                                                      *
*     n d m o n t h                                                    *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Lookup the number of days in the given month.
 *
 * @param[in] year year to check
 * @param[in] month month to check
 * @return the number of days in the month.
 *********************************************************************/

int	ndmonth

	(
	int	year,
	int	month
	)

	{
	/* Normalize month */
	year += vnorm(&month, 1, 12);
	if (year == 0) year++;

	if (month == 1) return (jmonth(year, month));
	else            return (jmonth(year, month) - jmonth(year, month-1));
	}

/***********************************************************************
*                                                                      *
*     n d y e a r                                                      *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Lookup the number of days in the given year and indicate whether
 * it is a leap year.
 *
 * @param[in] year year to check
 * @return Number of days in the given year.
 *********************************************************************/

int	ndyear

	(
	int	year
	)

	{
	if (year == 0) return 0;
	return jmonth(year, 12);
	}

/***********************************************************************
*                                                                      *
*     l e a p                                                          *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** This subroutine determines whether the given year is a leap year.
 *
 * @param[in] year year to test
 * @return
 * 	- TRUE  --> given year is a leap year
 *	- FALSE --> given year is not a leap year
 *********************************************************************/

LOGICAL	leap

	(
	int	year
	)

	{
	LOGICAL	isleap;

	/* Handle BC years (no year zero) */
	if (year == 0) return FALSE;
	if (year < 0)
		{
		isleap = (LOGICAL) Divisible(-1-year, 4);
		return isleap;
		}

	/* First calculate based on Julian calendar */
	isleap = Divisible(year, 4);
	if (year <= 1752) return isleap;

	/* Then make correction to Gregorian calendar after 1752 */
	if (!isleap) return FALSE;
	isleap = (LOGICAL) ( !Divisible(year, 100) || Divisible(year, 400) );
	return isleap;
	}

/***********************************************************************
*                                                                      *
*     d e c o d e _ c l o c k
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** This subroutine returns the julian date and gmt time which
 * correspond to the given clock value.
 *	@param[in]	cval	clock value to be decoded
 *	@param[out]	*year	current year
 *	@param[out]	*jday	current day of year
 *	@param[out]	*hour	current hour
 *	@param[out]	*minute	current minute
 *	@param[out]	*second	current second
 *********************************************************************/

void	decode_clock

	(
	long	cval,
	int		*year,
	int		*jday,
	int		*hour,
	int		*minute,
	int		*second
	)

	{
	struct tm *tbuf;	/* time structure pointer */

	/**************************************************************
	*                                                             *
	*  Y2000 issue:  UNIX tm struct (Compliant)                   *
	*                                                             *
	*  The UNIX tm struct represents the year as "years since     *
	*  1900".  This does not create a problem at 2000, since its  *
	*  value is not limited to 2 digits.                          *
	*                                                             *
	**************************************************************/

	tbuf    = gmtime((time_t *)&cval);
	if (year)   *year   = tbuf->tm_year + 1900;
	if (jday)   *jday   = tbuf->tm_yday + 1;
	if (hour)   *hour   = tbuf->tm_hour;
	if (minute) *minute = tbuf->tm_min;
	if (second) *second = tbuf->tm_sec;
	}

/***********************************************************************
*                                                                      *
*     e n c o d e _ c l o c k                                          *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Convert a given Julian date (year and day of year) to a long
 * integer value, representing the number of seconds since
 * 00:00:00 GMT 1 January 1970.
 *
 *	@param[in]	year	year
 *	@param[in]	jday	day of year
 *	@param[in]	hour	hour
 *	@param[in]	minute	minute
 *	@param[in]	second	second
 * @return Number of seconds since 00:00:00: GMT 1 January 1970.
 *********************************************************************/

long	encode_clock

	(
	int	year,
	int	jday,
	int	hour,
	int	minute,
	int	second
	)

	{
	long	cval;

	/* Define the dawn of time */
	static	int	Y0 = 1970;
	static	int	J0 = 1;
	static	int	H0 = 0;
	static	int	M0 = 0;
	static	int	S0 = 0;

	/* Normalize date-time */
	tnorm(&year, &jday, &hour, &minute, &second);

	/* Get number of days since dawn of time */
	cval = jday - J0;
	while (--year >= Y0) cval += ndyear(year);
	cval *= 86400;

	/* Add in hours, minutes and seconds */
	cval += (hour   - H0) * 3600;
	cval += (minute - M0) * 60;
	cval += (second - S0);

	return cval;
	}

/***********************************************************************
*                                                                      *
*     j m o n t h   (static)                                           *
*                                                                      *
*     This subroutine returns the day of year of the last day of the   *
*     given month.                                                     *
*                                                                      *
***********************************************************************/

static	int	jmonth

	(
	int	year	/* given year */ ,
	int	month	/* given month */
	)

	{
	static int	ldmonth[]
			= {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};

#	define jmnorm(i) (ldmonth[(i)])
#	define jmleap(i) ((i<2) ? jmnorm(i) : jmnorm(i)+1)
#	define jm1752(i) ((i<9) ? jmleap(i) : jmleap(i)-11)

	/* Normalize month */
	year += vnorm(&month, 1, 12);
	if (year == 0) year++;

	/* Special short year 1752, when Gregorian calendar was adopted */
	if (year == 1752) return jm1752(month);

	/* Leap year */
	else if (leap(year)) return jmleap(month);

	/* Normal year */
	else return jmnorm(month);
	}

/***********************************************************************
*                                                                      *
*     v n o r m   (static)                                             *
*                                                                      *
*     This subroutine normalizes a given variable to within a          *
*     specified range (much like the mod function).  In addition a     *
*     value is returned which represents the number of range widths    *
*     by which the original variable was outside the specified range.  *
*     This should be used to increment (or decrement) a "carry"        *
*     variable, as in:                                                 *
*                                                                      *
*          carry += vnorm(&value, vmin, vmax);                         *
*                                                                      *
*     note: if the specified range is reversed (upper limit precedes   *
*           lower limit) nothing is done.                              *
*                                                                      *
***********************************************************************/

static	int	vnorm

	(
	int	*value	/* modified - specified value --> normalized result */ ,
	int	vmin	/* given - lower limit on value range */ ,
	int	vmax	/* given - upper limit on value range */
	)

	{
	int	width, carry;

	if (!value) return 0;

	/* Compute range from specified limits */
	carry = 0;
	width = vmax - vmin + 1;
	if (width <= 0) return carry;

	/* Value is below range */
	if (*value < vmin)
		{
		carry   = (1-vmin+*value)/width - 1;
		*value -= carry*width;
		}

	/* Value is above range */
	else if (*value > vmax)
		{
		carry   = 1 - (1-*value+vmax)/width;
		*value -= carry*width;
		}

	/* Return the carry value */
	return carry;
	}

/***********************************************************************
*                                                                      *
*   Test program for codeword handling functions.                      *
*                                                                      *
***********************************************************************/

#ifdef STANDALONE

#include <stdio.h>

void	jdate_test(void);
void	mdate_test(void);
void	cal_test(void);

void	main(void)
	{
	char    buf[25];

	while (1)
		{
		(void) printf("\n");
		(void) printf("Test modes:\n");
		(void) printf("   1 - Month/date -> Julian date\n");
		(void) printf("   2 - Julian date -> month/date\n");
		(void) printf("   3 - Calendar\n");
		(void) printf("Enter test mode: ");
		getfileline(stdin, buf, sizeof(buf));

		if (blank(buf)) break;
		else if (same(buf, "1")) jdate_test();
		else if (same(buf, "2")) mdate_test();
		else if (same(buf, "3")) cal_test();
		}
	}

void	jdate_test(void)
	{
	char	buf[25];
	int		year, month, ndm, mday, jday;

	while (1)
		{
		(void) printf("\n");
		(void) printf("Enter year: ");
		getfileline(stdin, buf, sizeof(buf));
		if (blank(buf)) break;
		if (sscanf(buf, "%d", &year) < 1) continue;

		(void) printf("Enter month (1-12): ");
		getfileline(stdin, buf, sizeof(buf));
		if (blank(buf)) continue;
		if (sscanf(buf, "%d", &month) < 1) continue;
		if (month < 1)  continue;
		if (month > 12) continue;

		ndm = ndmonth(year, month);
		(void) printf("Enter day of month (1-%d): ", ndm);
		getfileline(stdin, buf, sizeof(buf));
		if (blank(buf)) continue;
		if (sscanf(buf, "%d", &mday) < 1) continue;
		if (mday < 1)   continue;
		if (mday > ndm) continue;

		jdate(&year, &month, &mday, &jday);
		(void) printf("Day of year: %d\n", jday);
		}
	}

void	mdate_test(void)
	{
	char	buf[25];
	int		year, month, ndy, mday, jday;

	while (1)
		{
		(void) printf("\n");
		(void) printf("Enter year: ");
		getfileline(stdin, buf, sizeof(buf));
		if (blank(buf)) break;
		if (sscanf(buf, "%d", &year) < 1) continue;

		ndy = ndyear(year);
		(void) printf("Enter day of year (1-%d): ", ndy);
		getfileline(stdin, buf, sizeof(buf));
		if (blank(buf)) continue;
		if (sscanf(buf, "%d", &jday) < 1) continue;
		if (jday < 1)   continue;
		if (jday > ndy) continue;

		mdate(&year, &jday, &month, &mday);
		(void) printf("Month: %d\n", month);
		(void) printf("Day of month: %d\n", mday);
		}
	}

void	cal_test(void)
	{
	}
#endif /* STANDALONE */
