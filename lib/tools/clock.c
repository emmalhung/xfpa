/*********************************************************************/
/** @file clock.c
 *
 * This set of routines are used for time handling within the
 * text generation portion of FPA.  Each routine has an interface
 * to prolog (see clock.pl).  These routines complement those of
 * the routines in the @verbatim $(FPA)/lib/tools/time.c @endverbatim
 * library, the main difference being that the act of reading the 
 * clock is separated from interpretation of the clock.  This is 
 * important for tasks such as detecting a GMT offset.  The user 
 * wants to be careful about multiple clock sampling, eg. reading 
 * date and time on different sides of midnight (see Richard O'Keefe's
 * comments in /usr/local/q2.4.2/library/date.c).  It is also useful
 * to separate clock reading from interpretation because a bogus clock
 * value can be substituted for the real clock value.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *********************************************************************/
/************************************************************************
*                                                                       *
*     c l o c k                                                         *
*                                                                       *
*     This set of routines are used for time handling within the        *
*     text generation portion of FPA.  Each routine has an interface    *
*     to prolog (see clock.pl).  These routines complement those of     *
*     the routines in the $(FPA)/lib/tools/time.c library, the main     *
*     difference being that the act of reading the clock is separated   *
*     from interpretation of the clock.  This is important for tasks    *
*     such as detecting a GMT offset.  The user wants to be careful     *
*     about multiple clock sampling, eg. reading date and time on       *
*     different sides of midnight (see Richard O'Keefe's comments in    *
*     /usr/local/q2.4.2/library/date.c).  It is also useful to separate *
*     clock reading from interpretation because a bogus clock value     *
*     can be substituted for the real clock value.                      *
*                                                                       *
*     Reference: ctime(3C) HP-UX Reference Vol. 2, Section 3            *
*                $(FPA)/lib/tools/time.c                                *
*                /usr/local/q2.4.2/library/date.c by Richard O'Keefe    *
*                                                                       *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)             *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)             *
*     Version 7 (c) Copyright 2006 Environment Canada                   *
*     Version 8 (c) Copyright 2011 Environment Canada                   *
*                                                                       *
*   This file is part of the Forecast Production Assistant (FPA).       *
*   The FPA is free software: you can redistribute it and/or modify it  *
*   under the terms of the GNU General Public License as published by   *
*   the Free Software Foundation, either version 3 of the License, or   *
*   any later version.                                                  *
*                                                                       *
*   The FPA is distributed in the hope that it will be useful, but      *
*   WITHOUT ANY WARRANTY; without even the implied warranty of          *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                *
*   See the GNU General Public License for more details.                *
*                                                                       *
*   You should have received a copy of the GNU General Public License   *
*   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.    *
*                                                                       *
************************************************************************/

#include "clock.h"
#include "time.h"

#include <time.h>
#include <string.h>

#ifdef MACHINE_SUN
	extern	char	*tzname[2];
#endif
#ifdef MACHINE_PCLINUX
	extern	char	*tzname[2];
#endif

/************************************************************************
*                                                                       *
*     r e a d _ c l o c k                                               *
*    Make this explicit.                                                *
************************************************************************/

static	time_t current_clock = 0;	/* current clock value */

/*********************************************************************/
/** Want to be able to read the clock once and interpret the same
 * value in different ways.
 *********************************************************************/
void	read_clock(void)
	{
	current_clock = time((time_t *)0);
	}

/************************************************************************
*                                                                       *
*     s a v e _ c l o c k                                               *
*     r e s t o r e _ c l o c k                                         *
*                                                                       *
************************************************************************/

static	time_t stored_clock = 0;	/* stored clock value */

/*********************************************************************/
/** Store the current clock value.
 *
 * Want to be able to store the current clock value, call
 * bogus_clock, and then restore the clock value to what it was.
 *********************************************************************/
void	save_clock(void)
	{
	stored_clock = current_clock;
	}

/*********************************************************************/
/** Restore the saved clock value.
 *
 * Want to be able to store the current clock value, call
 * bogus_clock, and then restore the clock value to what it was.
 *********************************************************************/
void	restore_clock(void)
	{
	current_clock = stored_clock;
	}

/************************************************************************
*                                                                       *
*     b o g u s _ c l o c k                                             *
*                                                                       *
************************************************************************/
/*********************************************************************/
/** Primarily for debugging purposes, it is desirable to run the
 * text generation software at some time other than the current
 * system time.  Bogus_clock is used in place of read_clock, thus
 * the time interpretation routines act in the usual way except
 * that their output is based on the bogus input.  Inputs are
 * expressed as local values.  A bogus timezone can be set in the
 * fpa setup file which simply replaces the TZ environment variable
 * for the current process.
 *
 * Usage:  The date can be entered either as a julian day
 * or as month-and-day.  The month-and-day method takes
 * precedence, ie. the julian day is only considered if
 * either month or day is zero.
 *
 * Routine mdate is in $(FPA)/lib/tools/tools.c.
 *
 * Unix system routine mktime does considerable time range
 * normalization, so input can be quite varied.  Remember,
 * however, that bogus_clock is inputting a *local* time
 * and usually is subject to daylight savings time changes.
 * During time changes the setting of dst_flag can be
 * critical.
 *
 * Bogus_clock is also useful in doing time conversions (see
 * convert_gmt_to_local() below).
 *
 *	@param[in]	year		years since 1900, although 19xx or 20xx o.k.
 *	@param[in]	jday		julian day [1,366] used only if month or day is zero
 *	@param[in]	month		month of year [1,12]
 *	@param[in]	day 		day of month [1,31]
 *	@param[in]	hour		hours - [0,23]
 *	@param[in]	minute		minutes after the hour - [0,59]
 *	@param[in]	dst_flag	daylight savings time flag:
 *				@li			1 = daylight savings time in effect
 *				@li			0 = standard time is in effect -
 *				@li			1 = don't know
 *********************************************************************/

void	bogus_clock

	/**************************************************************
	*                                                             *
	*  Y2000 issue:  Optional 2/3 digit year (Potential problem)  *
	*                                                             *
	*  The year argument to this function can be either the full  *
	*  year, or optionally "years since 1900" if less than 4      *
	*  digits (consistent with the UNIX tm struct).  This         *
	*  can lead to confusion about 2 and 3 digit years.  I        *
	*  recommend we drop support for an abbreviated year.         *
	*                                                             *
	**************************************************************/

	(
	int	year,
	int	jday,
	int	month,
	int	day,
	int	hour,
	int	minute,
	int	dst_flag
	)

	{
	struct	tm tbuf;		/* time structure */

	/* Calculate month and day if given the Julian day. */
	if ((month == 0 || day == 0) && jday != 0)
		mdate (&year, &jday, &month, &day);

	month--;	/* get into range [0,11] */
	while (year > 1900) year -= 1900;

	/* Set clock. */
	tbuf.tm_sec = 0;
	tbuf.tm_min = minute;
	tbuf.tm_hour = hour;
	tbuf.tm_mday = day;
	tbuf.tm_mon = month;
	tbuf.tm_year = year;
	tbuf.tm_isdst = dst_flag;
	current_clock = mktime(&tbuf);
	}

/************************************************************************
*                                                                       *
*     f u l l _ y e a r                                                 *
*                                                                       *
************************************************************************/
/*********************************************************************/
/** Convert a 2-digit year to a full year.
 *
 * This should only be necessary for interpretting external data
 * over which we have no control (ALL FPA data files should use
 * the full year).
 *
 * This is done in an intelligent (Y2000 compliant) way!
 * A 100 year sliding window is used, going 49 years back and
 * 50 years ahead of the "current" year.  For dates outside this
 * sliding window, the "demo" time stamp can be set using the
 * bogus_clock() function above.
 *
 * @param[in] abbrev abbreviated year
 * @param[out] *converted did we have to convert it?
 * @return converted year.
 *********************************************************************/

	/**************************************************************
	*                                                             *
	*  Y2000 issue:  2-digit year in external data (Compliant)    *
	*                                                             *
	*  All code which must interpret a 2-digit year from external *
	*  data, must use this function.  This function converts in   *
	*  a Y2000 compliant way, and provides a place to trap all    *
	*  instances of 2-digit years.                                *
	*                                                             *
	**************************************************************/

int		full_year

	(
	int		abbrev,
	LOGICAL	*converted
	)

	{
	int	syear, scent, year;
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

	if (converted) *converted = FALSE;
	if (abbrev >= 100) return abbrev;
	if (converted) *converted = TRUE;

	/* Find 49 years before the current year (start of 100 year window) */
	if (current_clock == 0) read_clock();
	tbuf  = gmtime(&current_clock);
	syear = tbuf->tm_year + 1900 - 49;
	scent = syear / 100;

	year  = abbrev + scent*100;
	if (year < syear) year += 100;
	return year;
	}

/************************************************************************
*                                                                       *
*     g t i m e                                                         *
*                                                                       *
************************************************************************/
/*********************************************************************/
/** Interpret the saved clock value into Greenwich Mean Time (GMT).
 *
 * @param[out] *year     GMT year
 * @param[out] *jday     GMT day of year       - [1,366]
 * @param[out] *month    GMT month             - [1,12]
 * @param[out] *day      GMT day of month      - [1,31]
 * @param[out] *wday     GMT days since Sunday - [0,6]
 * @param[out] *hour     GMT hour              - [0,23]
 * @param[out] *minute   GMT minute            - [0,59]
 *********************************************************************/

void  gtime

	(
	int	*year,
	int	*jday,
	int	*month,
	int	*day,
	int	*wday,
	int	*hour,
	int	*minute
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

	if (current_clock == 0) read_clock();
	tbuf	= gmtime(&current_clock);
	if ( year )		*year	= tbuf->tm_year + 1900;
	if ( jday )		*jday	= tbuf->tm_yday + 1;
	if ( month )	*month	= tbuf->tm_mon + 1;
	if ( day )		*day	= tbuf->tm_mday;
	if ( wday )		*wday	= tbuf->tm_wday;
	if ( hour )		*hour	= tbuf->tm_hour;
	if ( minute )	*minute	= tbuf->tm_min;
	}

/************************************************************************
*                                                                       *
*     l t i m e                                                         *
*                                                                       *
************************************************************************/
/*********************************************************************/
/** Interpret the saved clock value into local time according to
 * the TZ variable set within the current Unix shell.
 *
 *	@param[out]	*year		year - 19xx or 20xx
 *	@param[out]	*jday		day of year - [1,366]
 *	@param[out]	*month		month - [1,12]
 *	@param[out]	*day		day of month - [1,31]
 *	@param[out]	*wday		days since Sunday - [0,6]
 *	@param[out]	*hour		hour - [0,23]
 *	@param[out]	*minute		minute - [0,59]
 *********************************************************************/

void  ltime

	(
	int	*year,
	int	*jday,
	int	*month,
	int	*day,
	int	*wday,
	int	*hour,
	int	*minute
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

	if (current_clock == 0) read_clock();
	tbuf	= localtime(&current_clock);
	if ( year )		*year	= tbuf->tm_year + 1900;
	if ( jday )		*jday	= tbuf->tm_yday + 1;
	if ( month )	*month	= tbuf->tm_mon + 1;
	if ( day )		*day	= tbuf->tm_mday;
	if ( wday )		*wday	= tbuf->tm_wday;
	if ( hour )		*hour	= tbuf->tm_hour;
	if ( minute )	*minute	= tbuf->tm_min;
	}

/************************************************************************
*                                                                       *
*     g _ o f f s e t                                                   *
*                                                                       *
************************************************************************/
/*********************************************************************/
/** Calculate the number of hours and minutes between local and GMT
 * time.  Note that this may be different than the contents of
 * external variable timezone which is the difference between GM
 * and local standard(!) time.  For the same reason, it is not
 * sufficient to simply parse the TZ environment variable.
 *
 *	@param[out]	*hoff	number of hours  between local and gmt time
 *	@param[out]	*moff	minutes to be added to hoff
 *********************************************************************/

void	g_offset

	(
	int	*hoff,
	int	*moff
	)

	{
	int			lhour,lmin,ghour,gmin,tmin1,tmin2,tdif;
	struct tm	*tbuf;		/* time structure pointer */

	if (current_clock == 0) read_clock();
	tbuf	= localtime(&current_clock);
	lhour	= tbuf->tm_hour;
	lmin	= tbuf->tm_min;
	tbuf	= gmtime(&current_clock);
	ghour	= tbuf->tm_hour;
	gmin	= tbuf->tm_min;

	/* Convert to minutes and return an offset between +/-12 hours */
	/* (1440 is 24*60 and 720 is 12*60) */

	tmin1 = ghour*60 + gmin + 1440;
	tmin2 = lhour*60 + lmin;
	tdif = tmin1 - tmin2;
	while (tdif > 720) tdif -= 1440;
	if (hoff) *hoff = tdif/60;
	if (moff) *moff = tdif%60;
	}

/************************************************************************
*                                                                       *
*     d a y l i t e                                                     *
*                                                                       *
************************************************************************/
/*********************************************************************/
/** This function determines if Daylight Savings Time is in effect
 * by calling the system "time" routine and "localtime".  It
 * returns:
 *
 * Note that the routine is not named "daylight" to avoid
 * conflicting with the external variable with the same name
 * declared in <time.h>.  "localtime" calls "tzset" which uses
 * this external variable.
 *
 * @return @li 1 if Daylight Savings Time is in effect;
 *         @li 0 if Daylight Savings Time is not in effect;
 *        @li -1 if the information is not available.
 *********************************************************************/
int   daylite(void)

	{
	struct tm *tbuf;	/* time structure pointer */

	if (current_clock == 0) read_clock();
	tbuf = localtime(&current_clock);
	return (tbuf->tm_isdst);
	}

/************************************************************************
*                                                                       *
*     c o n v e r t _ g m t _ t o _ l o c a l                           *
*                                                                       *
************************************************************************/
/*********************************************************************/
/**  Convert from GMT time to local time.
 *
 * Unlike ltime, this routine converts from a user provided GMT
 * time to a local time.  It uses bogus_clock, so the value of
 * current_clock is temporarily altered.  It also returns a timezone
 * stamp (eg. EDT).  Setting the local TZ environment variable
 * must be done before calling this routine.
 *
 * Usage:  The GMT date can be entered either as a julian day
 * or as month-and-day.  The month-and-day method takes
 * precedence, ie. the julian day is only considered if
 * either month or day is zero.
 *
 * The algorithm used is to temporarily modify the TZ environment
 * variable to equal GMT.  The input time is registered via
 * bogus_clock, the TZ variable is returned to what it was, and
 * the time is interpreted via ltime.  This is the preferred method
 * because all details such as change to and from daylight savings
 * time are handled by existing Unix system calls.
 *
 *	@param[in]		gyear		GMT year - 19xx or 20xx
 *	@param[in]		gjday		GMT julian day [1,366]
 *								(ignored if gmonth and gday non-zero)
 *	@param[in]		gmonth		GMT month - [1,12] (or zero if using gjday)
 *	@param[in]		gday		GMT day of month - [1,31] (or zero if using gjday)
 *	@param[in]		ghour		GMT hour - [0,23]
 *	@param[in]		gminute		GMT minute - [0,59]
 *	@param[out]		*year		local year - 19xx or 20xx
 *	@param[out]		*jday		local day of year - [1,366]
 *	@param[out]		*month		local month - [1,12]
 *	@param[out]		*day		local day of month - [1,31]
 *	@param[out]		*wday		local days since Sunday - [0,6]
 *	@param[out]		*hour		local hour - [0,23]
 *	@param[out]		*minute		local minute - [0,59]
 *	@param[out]  	**tzstamp	current timezone stamp accounting
 *								for daylight savings time, eg. est or edt
 *	@param[out]		*dstflag	0=standard time; 1=daylight; time -1=don't know
 *********************************************************************/

void	convert_gmt_to_local

	(
	int		gyear,
	int		gjday,
	int		gmonth,
	int		gday,
	int		ghour,
	int		gminute,
	int		*year,
	int		*jday,
	int		*month,
	int		*day,
	int		*wday,
	int		*hour,
	int		*minute,
	char	**tzstamp,
	int		*dstflag
	)

	{
	int		i, idst;

	char	*p;
	static	char	strng[] = "TZ=GMT";
	extern	char	**environ;

	/* Store the current clock value so that we can use the
	   usual date-time interpretation routines. */
	save_clock();

	/* Save the current timezone and switch to GMT, turning the
	   input GMT date into a "local" date.  Then call bogus_clock
	   to correctly encode the date into current_clock.  Restore the
	   original timezone and interpret the clock.

	   The original version of this function used getenv() and putenv()
	   to save and restore the current timezone environment variable
	   TZ.  However, putenv actually puts the string passed to it into
	   the environment.  In order to scope beyond the current file, it
	   would have been necessary to create an external variable,
	   to be stored in some include file somewhere.  The following
	   code accesses the environ array directly (which is exactly what
	   getenv would do -- see environ(5)), and avoids the baggage of
	   putenv().
	*/

	for (i = 0; *environ[i] != '\0'; i++)
		if (strncmp(environ[i],"TZ=",3) == 0)
		break;

	p = environ[i];
	if (*environ[i] != '\0') environ[i] = strng;
	bogus_clock (gyear,gjday,gmonth,gday,ghour,gminute,-1);
	environ[i] = p;
	ltime (year,jday,month,day,wday,hour,minute);

	/* Now get the timezone stamp directly from the external */
	/* variable tzname (defined in time.h).	*/
	idst = daylite();
	if ( dstflag ) *dstflag = idst;
	if ( idst )
		{ if ( tzstamp ) *tzstamp = tzname[1]; }
	else
		{ if ( tzstamp ) *tzstamp = tzname[0]; }

	/* Restore the original clock value. */
	restore_clock();
	}

/************************************************************************
*                                                                       *
*     c o n v e r t _ l o c a l _ t o _ g m t                           *
*                                                                       *
************************************************************************/
/*********************************************************************/
/** Convert local time to GMT time.
 *
 * Unlike its opposite, this routine is relatively straightforward.
 * It uses bogus_clock, so the value of save_clock is again
 * temporarily altered.  It also uses the same assumption that the
 * local TZ environment variable must be set before calling this
 * routine.
 *
 * Usage:  The local date can be entered either as a julian day
 * or as month-and-day.  The month-and-day method takes
 * precedence, ie. the julian day is only considered if
 * either month or day is zero.
 *
 * The algorithm is to set a local date and time via bogus_clock
 * and to interpret it using gtime.  The only complication is in
 * specifying the local time during a time change between daylight
 * and standard time.  In going to daylight savings time, the hour
 * between 2 and 3 a.m. local time does not exist.  When switching
 * back to standard time, the hour between 1 and 2 a.m. local time
 * is repeated.
 *	@param[in]	lyear		local year - 19xx or 20xx
 *	@param[in]	ljday		local julian day [1,366]
 *							(ignored if gmonth and gday non-zero)
 *	@param[in]	lmonth		local month - [1,12] (or zero if using gjday)
 *	@param[in]	lday		local day of month - [1,31] (or zero if using gjday)
 *	@param[in]	lhour		local hour - [0,23]
 *	@param[in]	lminute		local minute - [0,59]
 *	@param[in]	dst_flag	daylight savings time flag:
 *				@li	         1 = daylight savings time in effect
 *				@li	         0 = standard time is in effect
 *				@li	        -1 = don't know			
 *	@param[out]	*year		GMT year - 19xx or 20xx
 *	@param[out]	*jday		GMT day of year - [1,366]
 *	@param[out]	*month		GMT month - [1,12]
 *	@param[out]	*day		GMT day of month - [1,31]
 *	@param[out]	*wday		GMT days since Sunday - [0,6]
 *	@param[out]	*hour		GMT hour - [0,23]
 *	@param[out]	*minute		GMT minute - [0,59]
 *********************************************************************/

void	convert_local_to_gmt

	(
	int	lyear,
	int	ljday,
	int	lmonth,
	int	lday,
	int	lhour,
	int	lminute,
	int	dst_flag,
	int	*year,
	int	*jday,
	int	*month,
	int	*day,
	int	*wday,
	int	*hour,
	int	*minute
	)

	{
	/* Store the current clock value so that we can use the
	   usual date-time interpretation routines. */
	save_clock();

	bogus_clock (lyear,ljday,lmonth,lday,lhour,lminute,dst_flag);
	gtime (year,jday,month,day,wday,hour,minute);

	/* Restore the original clock value. */
	restore_clock();
	}
