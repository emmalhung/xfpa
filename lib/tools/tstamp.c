/*********************************************************************/
/** @file tstamp.c
 *
 * Functions to retrieve and evaluate valid time for programs.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*     t s t a m p . c                                                  *
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

#include "tstamp.h"
#include "time.h"
#include "parse.h"
#include "solar.h"
#include "string_ext.h"

#include <fpa_math.h>
#include <fpa_types.h>
#include <fpa_getmem.h>
#include <fpa_macros.h>

#include <stdio.h>

/***********************************************************************
*                                                                      *
*     c a l c _ v a l i d _ t i m e                                    *
*     c a l c _ p r o g _ t i m e                                      *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Calculate the valid time string given a run time stamp and a
 * prog time.
 *
 * @param[in] rstamp run time stamp
 * @param[in] prog   prog time in hours added to rstamp
 * @return valid time string.
 *********************************************************************/
STRING	calc_valid_time

	(
	STRING	rstamp,
	int		prog
	)

	{
	int		year, jday, hour, min;
	LOGICAL	local, mins;

	if ( !parse_tstamp(rstamp, &year, &jday, &hour, &min, &local, &mins) )
		return NULL;

	hour += prog;
	tnorm(&year, &jday, &hour, &min, NullInt);

	return build_tstamp(year, jday, hour, min, local, mins);
	}

/**********************************************************************/

/*********************************************************************/
/** Calculate a prog time given run and valid time stamps. Indicate
 * if successful.
 * @param[in] rstamp  run time stamp
 * @param[in] vstamp  valid time stamp
 * @param[out] *status did it work?
 * @return Hours since run time.
 *********************************************************************/
int	calc_prog_time

	(
	STRING	rstamp,
	STRING	vstamp,
	LOGICAL	*status
	)

	{
	int		vyear, vjday, vhour, vmin;
	int		ryear, rjday, rhour, rmin;
	LOGICAL	rlocal, vlocal, rmins, vmins;

	if (status) *status = FALSE;
	if ( !parse_tstamp(rstamp, &ryear, &rjday, &rhour, &rmin, &rlocal, &rmins)
	  || !parse_tstamp(vstamp, &vyear, &vjday, &vhour, &vmin, &vlocal, &vmins) )
			return 0;

	if (status) *status = TRUE;
	/* Ignore minutes in this calculation */
	return hdif(ryear, rjday, rhour, vyear, vjday, vhour);
	}

/***********************************************************************
*                                                                      *
*     c a l c _ v a l i d _ t i m e _ m i n u t e s                    *
*     c a l c _ p r o g _ t i m e _ m i n u t e s                      *
*     h o u r _ m i n u t e _ s t r i n g                              *
*     i n t e r p r e t _ h o u r _ m i n u t e _ s t r i n g          *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Calculate the valid time string given a run time stamp and a
 * prog time with hours and minutes.
 *
 * @param[in] rstamp run time stamp
 * @param[in] phour  prog time (in hours)
 * @param[in] pmin   prog time (in minutes)
 * @return valid time string with minutes.
 *********************************************************************/
STRING	calc_valid_time_minutes

	(
	STRING	rstamp,
	int		phour,
	int		pmin
	)

	{
	int		year, jday, hour, min;
	LOGICAL	local;

	if ( !parse_tstamp(rstamp, &year, &jday, &hour, &min,
			&local, NullLogicalPtr) ) return NULL;

	hour += phour;
	min  += pmin;
	tnorm(&year, &jday, &hour, &min, NullInt);

	return build_tstamp(year, jday, hour, min, local, TRUE);
	}

/**********************************************************************/

/*********************************************************************/
/** Calculate a prog time given run and valid time stamps. Indicate
 * if successful.
 *
 * @param[in] rstamp   runt time stamp
 * @param[in] vstamp   valid time stamp
 * @param[out] *status did it work?
 * @return Minutes since run time.
 *********************************************************************/
int	calc_prog_time_minutes

	(
	STRING	rstamp,
	STRING	vstamp,
	LOGICAL	*status
	)

	{
	int		vyear, vjday, vhour, vmin;
	int		ryear, rjday, rhour, rmin;
	LOGICAL	rlocal, vlocal, rmins, vmins;

	if (status) *status = FALSE;
	if ( !parse_tstamp(rstamp, &ryear, &rjday, &rhour, &rmin, &rlocal, &rmins)
	  || !parse_tstamp(vstamp, &vyear, &vjday, &vhour, &vmin, &vlocal, &vmins) )
			return 0;

	if (status) *status = TRUE;
	return mdif(ryear, rjday, rhour, rmin, vyear, vjday, vhour, vmin);
	}

/**********************************************************************/

/*********************************************************************/
/** Format hours and minutes into string.
 *
 * @param[in] phour prog time (in hours)
 * @param[in] pmin prog time (in minutes)
 * @return Pointer to static variable. (Read Only!)(Will be over
 * written next time the function is called)
 *********************************************************************/
STRING	hour_minute_string

	(
	int		phour,
	int		pmin
	)

	{
	int		tmin, xhour, xmin;
	char	xsgn;

	static	TSTAMP	hmstamp = "0:00";

	/* Convert to minutes */
	tmin = phour*60 + pmin;

	/* Determine hours, minutes and sign */
	xhour = tmin / 60;
	xmin  = tmin % 60;
	xsgn  = (tmin < 0) ? '-' : '+';

	/* Return formatted string */
	(void) sprintf(hmstamp, "%c%.1d:%.2d", xsgn, abs(xhour), abs(xmin));
	return hmstamp;
	}

/**********************************************************************/

/*********************************************************************/
/** Parse a formated hours:minutes string.
 *
 * @param[in] hmstring  time (in hour:minute format)
 * @return Value in minutes of given string.
 *********************************************************************/
int		interpret_hour_minute_string

	(
	STRING	hmstring
	)

	{
	LOGICAL	ispos, ok;
	size_t	nc;
	int		xhours, xminutes;
	TSTAMP	xstring, hstring, mstring;

	if ( blank(hmstring) ) return 0;

	/* Strip off white space */
	(void) strcpy(xstring, hmstring);
	(void) no_white(xstring);

	/* Strip off a leading sign (if present) */
	ispos = TRUE;
	if ( xstring[0] == '+' )
		{
		(void) strcpy(xstring, xstring+1);
		}
	else if ( xstring[0] == '-' )
		{
		(void) strcpy(xstring, xstring+1);
		ispos = FALSE;
		}

	/* Extract the hours (if found) */
	nc = strcspn(xstring, ":");
	if ( nc > 0 )
		{
		(void) strncpy(hstring, xstring, nc);
		hstring[nc] = '\0';
		if (strspn(hstring, "0123456789") < strlen(hstring) )
			{
			(void) fprintf(stderr, "Error in hour/minute format: %s\n",
					hmstring);
			return 0;
			}
		xhours = int_arg(hstring, &ok);
		}
	else
		{
		xhours = 0;
		}

	/* Extract the minutes (if found) */
	if ( nc < strlen(xstring) )
		{
		(void) strcpy(mstring, xstring+nc+1);
		if ( strspn(mstring, "0123456789") < strlen(mstring) )
			{
			(void) fprintf(stderr, "Error in hour/minute format: %s\n",
					hmstring);
			return 0;
			}
		xminutes = int_arg(mstring, &ok);
		}
	else
		{
		xminutes = 0;
		}

	/* Return the number of minutes (according to the sign) */
	xminutes += xhours * 60;
	if ( ispos ) return  xminutes;
	else         return -xminutes;
	}

/***********************************************************************
*                                                                      *
*     p a r s e _ t s t a m p                                          *
*     b u i l d _ t s t a m p                                          *
*     v a l i d _ t s t a m p                                          *
*                                                                      *
*     Note:  build_tstamp creates the tstamp in recycled memory.       *
*            Make a copy of the new tstamp if you will need it after   *
*            the next call to this function.                           *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Parse a given time stamp string into its parts.
 *
 *	@param[in]	tstamp	time stamp
 *	@param[out]	*year	year
 *	@param[out]	*jday	jday
 *	@param[out]	*hour	hour
 *	@param[out]	*min	minutes
 *	@param[out]	*local	local vs GMT
 *	@param[out]	*mins	minutes are present
 *  @return
 * 	- TRUE if valid string.
 * 	- FALSE if not valid string.
 *********************************************************************/
LOGICAL	parse_tstamp

	(
	STRING	tstamp,
	int		*year,
	int		*jday,
	int		*hour,
	int		*min,
	LOGICAL	*local,
	LOGICAL	*mins
	)

	{
	int		mp, np;
	char	cc;
	int		vyear, vjday, vhour, vmin=0;
	char	xbuf[1024] = "";

	/* Initialize return parameters */
	if (year)  *year  = 0;
	if (jday)  *jday  = 0;
	if (hour)  *hour  = 0;
	if (min)   *min   = 0;
	if (local) *local = FALSE;
	if (mins)  *mins  = FALSE;

	/* Return immediately if no timestamp passed */
	if ( blank(tstamp) ) return FALSE;

	/* Try with minutes first */
	mp = 4;
	np = sscanf(tstamp,
			"%d:%d:%d:%d%c%s", &vyear, &vjday, &vhour, &vmin, &cc, xbuf);
	if (np < 4)
		{
		/* No luck - Try without minutes */
		mp = 3;
		np = sscanf(tstamp, "%d:%d:%d%c%s", &vyear, &vjday, &vhour, &cc, xbuf);
		}
	if (np < mp) return FALSE;
	if (!blank(xbuf)) return FALSE;

	if (year)  *year  = vyear;
	if (jday)  *jday  = vjday;
	if (hour)  *hour  = vhour;
	if (min)   *min   = vmin;
	if (local) *local = (LOGICAL) (np>mp && cc=='L');
	if (mins)  *mins  = (LOGICAL) (mp>3);
	return TRUE;
	}

/**********************************************************************/

/*********************************************************************/
/** Build a time stamp string from the given parts.
 *
 *  @param[in]	year	year
 *	@param[in]	jday	jday
 *	@param[in]	hour	hour
 *	@param[in]	min 	minutes
 *	@param[in]	local 	local vs GMT
 *	@param[in]	mins	include minutes
 *  @return Valid time stamp string.
 *********************************************************************/
STRING	build_tstamp

	(
	int		year,
	int		jday,
	int		hour,
	int		min,
	LOGICAL	local,
	LOGICAL	mins
	)

	{
	static	TSTAMP	tstamp = "";

	if (mins)
		(void) sprintf(tstamp, "%.4d:%.3d:%.2d:%.2d", year, jday, hour, min);
	else
		(void) sprintf(tstamp, "%.4d:%.3d:%.2d", year, jday, hour);
	if (local) (void) strcat(tstamp, "L");
	return tstamp;
	}

/**********************************************************************/

/*********************************************************************/
/** Check if a time stamp string is valid.
 *
 *	@param[in]	tstamp	time stamp
 * 	@return
 * 	- TRUE if valid.
 * 	- FALSE if not valid.
 *********************************************************************/
LOGICAL	valid_tstamp

	(
	STRING	tstamp
	)

	{
	return parse_tstamp(tstamp, NullInt, NullInt, NullInt, NullInt,
				NullLogicalPtr, NullLogicalPtr);
	}

/***********************************************************************
*                                                                      *
*     Obsolescent Functions:                                           *
*                                                                      *
*     r e a d _ t s t a m p                                            *
*     m a k e _ t s t a m p                                            *
*     m a k e _ l o c a l _ t s t a m p                                *
*                                                                      *
***********************************************************************/

int	read_tstamp

	(
	STRING	tstamp	/* time stamp */ ,
	int		*year	/* date and time */ ,
	int		*jday ,
	int		*hour
	)

	{
	static	LOGICAL	noted = FALSE;
	if (!noted)
		{
		noted = TRUE;
		(void) fprintf(stderr, "================================\n");
		(void) fprintf(stderr, "Obsolescent function:\n");
		(void) fprintf(stderr, "   read_tstamp(\n");
		(void) fprintf(stderr, "              STRING tstamp,\n");
		(void) fprintf(stderr, "              int    *year,\n");
		(void) fprintf(stderr, "              int    *jday,\n");
		(void) fprintf(stderr, "              int    *hour)\n");
		(void) fprintf(stderr, "================================\n");
		(void) fprintf(stderr, "Replace with:\n");
		(void) fprintf(stderr, "   parse_tstamp(\n");
		(void) fprintf(stderr, "               STRING  tstamp,\n");
		(void) fprintf(stderr, "               int     *year,\n");
		(void) fprintf(stderr, "               int     *jday,\n");
		(void) fprintf(stderr, "               int     *hour,\n");
		(void) fprintf(stderr, "               int     *min,\n");
		(void) fprintf(stderr, "               LOGICAL *local,\n");
		(void) fprintf(stderr, "               LOGICAL *mins)\n");
		(void) fprintf(stderr, "================================\n");
		}

	return (int) parse_tstamp(tstamp, year, jday, hour, NullInt,
					NullLogicalPtr, NullLogicalPtr);
	}

/**********************************************************************/

STRING	make_tstamp

	(
	int		year	/* date and time */ ,
	int		jday ,
	int		hour
	)

	{
	static	LOGICAL	noted = FALSE;
	if (!noted)
		{
		noted = TRUE;
		(void) fprintf(stderr, "================================\n");
		(void) fprintf(stderr, "Obsolescent function:\n");
		(void) fprintf(stderr, "   make_tstamp(\n");
		(void) fprintf(stderr, "              int  year,\n");
		(void) fprintf(stderr, "              int  jday,\n");
		(void) fprintf(stderr, "              int  hour)\n");
		(void) fprintf(stderr, "================================\n");
		(void) fprintf(stderr, "Replace with:\n");
		(void) fprintf(stderr, "   build_tstamp(\n");
		(void) fprintf(stderr, "               int     year,\n");
		(void) fprintf(stderr, "               int     jday,\n");
		(void) fprintf(stderr, "               int     hour,\n");
		(void) fprintf(stderr, "               int     min,\n");
		(void) fprintf(stderr, "               LOGICAL local,\n");
		(void) fprintf(stderr, "               LOGICAL mins)\n");
		(void) fprintf(stderr, "================================\n");
		}

	return build_tstamp(year, jday, hour, 0, FALSE, FALSE);
	}

/**********************************************************************/

STRING	make_local_tstamp

	(
	int		year	/* date and time */ ,
	int		jday ,
	int		hour ,
	float	lon		/* longitude (real degrees, +=E) */
	)

	{
	static	LOGICAL	noted = FALSE;
	if (!noted)
		{
		noted = TRUE;
		(void) fprintf(stderr, "================================\n");
		(void) fprintf(stderr, "Obsolescent function:\n");
		(void) fprintf(stderr, "   make_local_tstamp(\n");
		(void) fprintf(stderr, "              int   year,\n");
		(void) fprintf(stderr, "              int   jday,\n");
		(void) fprintf(stderr, "              int   hour,\n");
		(void) fprintf(stderr, "              float lon)\n");
		(void) fprintf(stderr, "================================\n");
		(void) fprintf(stderr, "Replace with:\n");
		(void) fprintf(stderr, "   build_tstamp(\n");
		(void) fprintf(stderr, "               int     year,\n");
		(void) fprintf(stderr, "               int     jday,\n");
		(void) fprintf(stderr, "               int     hour,\n");
		(void) fprintf(stderr, "               int     min,\n");
		(void) fprintf(stderr, "               LOGICAL local,\n");
		(void) fprintf(stderr, "               LOGICAL mins)\n");
		(void) fprintf(stderr, "================================\n");
		}

	return build_tstamp(year, jday, hour, 0, TRUE, FALSE);
	}

/***********************************************************************
*                                                                      *
*     i n t e r p r e t _ t i m e s t r i n g                          *
*     m e t a f i l e _ t i m e s t r i n g                            *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Reads a time string in any one of the accepted formats and
 *    reformats into the fpa standard format. yyyy:jjj:hh((:MM)L)
 *
 * Valid input formats are:
 * -	yyyy-jjj-hh-MM-SS,
 * -	yyyy-jjj-hh-MM,
 * -	yyyy-jjj-hh,
 * -	yyyy/mm/dd/hh:MM,
 * -	yyyy/mm/dd/hh,
 * -	yyyy/jjj/hh:MM,
 * -	yyyy/jjj/hh,
 * -    yyyy:jjj:hh:MM,
 * -    yyyy:jjj:hh,
 * -    rr/hh:MM,
 * -    rr/hh,
 * -    xx:MM,
 * -    xx
 *
 * Where:
 *  -    yyyy = 4 digit year
 *  -    mm   = 2 digit month
 *  -    dd   = 2 digit day of month
 *  -    hh   = 2 digit hour of day
 *  -    MM   = 2 digit minute of hour
 *  -    SS   = 2 digit second of minute
 *  -    jjj  = 3 digit Julian day
 *  -    rr   = 2 digit day ( 0 for the "T0" day, 1 for the next day, ...)
 *  -    xx   = 2 digit number of hours after "T0" (0 to 23)
 *  -    L    = indicates time is local.
 *
 * Note that all input formats can end with "L" to indicate local time
 * Note that "xx" and "xx:MM" formats can be negative (time before "T0")
 *
 * @param[in] tstring time string to interpret
 * @param[in] rstamp  run time stamp
 * @param[in] lon     longitude (real degrees, +=E)
 * @return
 * 		The reformated string, or Null if there was a failure.
 *
 *********************************************************************/
STRING	interpret_timestring

	(
	STRING	tstring,
	STRING	rstamp,
	float	lon
	)

	{
	LOGICAL	local;
	int		ilast, ryear, rjday;
	int		vyear, vmonth, vday, vjday, vrday;
	int		vhour, vrhour, vmin, vrmin, vsec;
	char	xbuf[1024] = "";

	static	STRING	tbuf = NullString;

	/* Return immediately if no valid time string passed */
	if ( blank(tstring) ) return NullString;

	/* Make a working copy */
	if (NotNull(tbuf)) FREEMEM(tbuf);
	tbuf = strdup(tstring);

	/* Set flag for local time in time string */
	ilast = strlen(tbuf) - 1;
	if (tbuf[ilast] == 'L')
		{
		local = TRUE;
		tbuf[ilast] = '\0';
		}
	else
		{
		local = FALSE;
		}

	/* Build the valid timestamp from  yyyy-jjj-hh-MM-SS  format time string */
	if ( sscanf(tbuf, "%d-%d-%d-%d-%d%s",
				&vyear, &vjday, &vhour, &vmin, &vsec, xbuf) == 5 )
		{
		(void) tnorm(&vyear, &vjday, &vhour, &vmin, &vsec);
		return build_tstamp(vyear, vjday, vhour, vmin, local, TRUE);
		}

	/* Build the valid timestamp from  yyyy-jjj-hh-MM  format time string */
	else if ( sscanf(tbuf, "%d-%d-%d-%d%s",
				&vyear, &vjday, &vhour, &vmin, xbuf) == 4 )
		{
		vsec = 0;
		(void) tnorm(&vyear, &vjday, &vhour, &vmin, &vsec);
		return build_tstamp(vyear, vjday, vhour, vmin, local, TRUE);
		}

	/* Build the valid timestamp from  yyyy-jjj-hh  format time string */
	else if ( sscanf(tbuf, "%d-%d-%d%s", &vyear, &vjday, &vhour, xbuf) == 3 )
		{
		vmin = 0;
		vsec = 0;
		(void) tnorm(&vyear, &vjday, &vhour, &vmin, &vsec);
		return build_tstamp(vyear, vjday, vhour, vmin, local, FALSE);
		}

	/* Build the valid timestamp from  yyyy/mm/dd/hh:MM  format time string */
	else if ( sscanf(tbuf, "%d/%d/%d/%d:%d%s",
			&vyear, &vmonth, &vday, &vhour, &vmin, xbuf) == 5 )
		{
		vsec = 0;
		(void) jdate(&vyear, &vmonth, &vday, &vjday);
		(void) tnorm(&vyear, &vjday, &vhour, &vmin, &vsec);
		return build_tstamp(vyear, vjday, vhour, vmin, local, TRUE);
		}

	/* Build the valid timestamp from  yyyy/mm/dd/hh  format time string */
	else if ( sscanf(tbuf, "%d/%d/%d/%d%s",
				&vyear, &vmonth, &vday, &vhour, xbuf) == 4 )
		{
		vmin = 0;
		vsec = 0;
		(void) jdate(&vyear, &vmonth, &vday, &vjday);
		(void) tnorm(&vyear, &vjday, &vhour, &vmin, &vsec);
		return build_tstamp(vyear, vjday, vhour, vmin, local, FALSE);
		}

	/* Build the valid timestamp from  yyyy/jjj/hh:MM  format time string */
	else if ( sscanf(tbuf, "%d/%d/%d:%d%s",
				&vyear, &vjday, &vhour, &vmin, xbuf) == 4 )
		{
		vsec = 0;
		(void) tnorm(&vyear, &vjday, &vhour, &vmin, &vsec);
		return build_tstamp(vyear, vjday, vhour, vmin, local, TRUE);
		}

	/* Build the valid timestamp from  yyyy/jjj/hh  format time string */
	else if ( sscanf(tbuf, "%d/%d/%d%s", &vyear, &vjday, &vhour, xbuf) == 3 )
		{
		vmin = 0;
		vsec = 0;
		(void) tnorm(&vyear, &vjday, &vhour, &vmin, &vsec);
		return build_tstamp(vyear, vjday, vhour, vmin, local, FALSE);
		}

	/* Build the valid timestamp from  yyyy:jjj:hh:MM  format time string */
	else if ( sscanf(tbuf, "%d:%d:%d:%d%s",
				&vyear, &vjday, &vhour, &vmin, xbuf) == 4 )
		{
		vsec = 0;
		(void) tnorm(&vyear, &vjday, &vhour, &vmin, &vsec);
		return build_tstamp(vyear, vjday, vhour, vmin, local, TRUE);
		}

	/* Build the valid timestamp from  yyyy:jjj:hh  format time string */
	else if ( sscanf(tbuf, "%d:%d:%d%s", &vyear, &vjday, &vhour, xbuf) == 3 )
		{
		vmin = 0;
		vsec = 0;
		(void) tnorm(&vyear, &vjday, &vhour, &vmin, &vsec);
		return build_tstamp(vyear, vjday, vhour, vmin, local, FALSE);
		}

	/* Build the valid timestamp from  rr/hh:MM  format time string */
	else if ( sscanf(tbuf, "%d/%d:%d%s", &vrday, &vhour, &vmin, xbuf) == 3 )
		{
		vsec = 0;
		if ( !parse_tstamp(rstamp, &ryear, &rjday, NullInt, NullInt,
				NullLogicalPtr, NullLogicalPtr) )
				return NullString;
		rjday += vrday;
		(void) tnorm(&ryear, &rjday, &vhour, &vmin, &vsec);
		return build_tstamp(ryear, rjday, vhour, vmin, local, TRUE);
		}

	/* Build the valid timestamp from  rr/hh  format time string */
	else if ( sscanf(tbuf, "%d/%d%s", &vrday, &vhour, xbuf) == 2 )
		{
		vmin = 0;
		vsec = 0;
		if ( !parse_tstamp(rstamp, &ryear, &rjday, NullInt, NullInt,
				NullLogicalPtr, NullLogicalPtr) )
				return NullString;
		rjday += vrday;
		(void) tnorm(&ryear, &rjday, &vhour, &vmin, &vsec);
		return build_tstamp(ryear, rjday, vhour, vmin, local, FALSE);
		}

	/* Build the valid timestamp from  xx:MM  format time string */
	else if ( sscanf(tbuf, "%d:%d%s", &vrhour, &vrmin, xbuf) == 2 )
		{
		if ( same_start(tbuf, "-") )
			return calc_valid_time_minutes(rstamp, vrhour, -vrmin);
		else
			return calc_valid_time_minutes(rstamp, vrhour,  vrmin);
		}

	/* Build the valid timestamp from  xx  format time string */
	else if ( sscanf(tbuf, "%d%s", &vrhour, xbuf) == 1 )
		{
		return calc_valid_time(rstamp, vrhour);
		}

	/* Unrecognized format for time string */
	else
		{
		return NullString;
		}
	}

/**********************************************************************/

/*********************************************************************/
/** Construct a timestring for a new format FPA metafile name
 *    from the given parts.  The format is yyyy-jjj-hh((-MM(-SS))L)
 *
 * Where:
 *  -    yyyy = 4 digit year
 *  -    jjj  = 3 digit Julian day
 *  -    hh   = 2 digit hour of day
 *  -    MM   = 2 digit minute of hour (optional)
 *  -    SS   = 2 digit second of minute (optional)
 *  -    L    = indicates time is local.
 *
 *  @param[in]	year	year
 *	@param[in]	jday	jday
 *	@param[in]	hour	hour
 *	@param[in]	min 	minutes
 *	@param[in]	sec 	seconds
 *	@param[in]	local 	local vs GMT
 *	@param[in]	mins	include minutes
 *	@param[in]	secs	include seconds
 *  @return new format FPA metafile time string.
 *********************************************************************/
STRING	metafile_timestring

	(
	int		year,
	int		jday,
	int		hour,
	int		min,
	int		sec,
	LOGICAL	local,
	LOGICAL	mins,
	LOGICAL	secs
	)

	{
	static	TSTAMP	tstamp = "";

	if (mins && secs)
		(void) sprintf(tstamp, "%.4d-%.3d-%.2d-%.2d-%.2d",
											year, jday, hour, min, sec);
	else if (mins)
		(void) sprintf(tstamp, "%.4d-%.3d-%.2d-%.2d", year, jday, hour, min);
	else
		(void) sprintf(tstamp, "%.4d-%.3d-%.2d", year, jday, hour);
	if (local) (void) strcat(tstamp, "L");
	return tstamp;
	}

/***********************************************************************
*                                                                      *
*     l m t _ t o _ l m t                                              *
*     l m t _ t o _ g m t                                              *
*     g m t _ t o _ l m t                                              *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Convert a local time at lon1 to the local time at lon2
 *
 * @param[in] tstamp local mean time (at lon1)
 * @param[in] lon1   from longitude (real degrees, +=E)
 * @param[in] lon2   to longitude (real degrees, +=E)
 * @return a time stamp string in yyyy:jjj:hh:MML format.
 **********************************************************************/
STRING	lmt_to_lmt

	(
	STRING	tstamp,
	float	lon1,
	float	lon2
	)

	{
	int		year, jday, hour, min, sec;

	if ( !read_tstamp(tstamp, &year, &jday, &hour) ) return NULL;

	/* Round up to nearest hour */
	min = 30;
	sec = seconds_from_gmt(lon2-lon1);
	tnorm(&year, &jday, &hour, &min, &sec);

	return make_tstamp(year, jday, hour);
	}

/**********************************************************************/

/**********************************************************************/
/** Convert a local time at lon to GMT
 *
 * Function is obsolete. Use local_to_gmt instead.
 *
 *	@param[in]	tstamp	local mean time
 *	@param[in]	lon		longitude (real degrees, +=E)
 * 	@return a time stamp string in yyyy:jjj:hh:MML format.
 **********************************************************************/
STRING	lmt_to_gmt

	(
	STRING	tstamp,
	float	lon
	)

	{
	static	LOGICAL	noted = FALSE;
	if (!noted)
		{
		noted = TRUE;
		(void) fprintf(stderr, "================================\n");
		(void) fprintf(stderr, "Obsolescent function:\n");
		(void) fprintf(stderr, "   lmt_to_gmt(\n");
		(void) fprintf(stderr, "              STRING tstamp,\n");
		(void) fprintf(stderr, "              float  lon)\n");
		(void) fprintf(stderr, "================================\n");
		(void) fprintf(stderr, "Replace with:\n");
		(void) fprintf(stderr, "   local_to_gmt(\n");
		(void) fprintf(stderr, "               STRING tstamp,\n");
		(void) fprintf(stderr, "               float  lon)\n");
		(void) fprintf(stderr, "================================\n");
		}

	return lmt_to_lmt(tstamp, lon, 0.0);
	}

/**********************************************************************/
/**********************************************************************/
/** Convert a GMT to local time at lon
 *
 * Function is obsolete. Use gmt_to_local instead.
 *
 *	@param[in]	tstamp	Greenwich mean time
 *	@param[in]	lon		longitude (real degrees, +=E)
 * @return a time stamp string in yyyy:jjj:hh:MML format.
 **********************************************************************/
STRING	gmt_to_lmt

	(
	STRING	tstamp,
	float	lon
	)

	{
	static	LOGICAL	noted = FALSE;
	if (!noted)
		{
		noted = TRUE;
		(void) fprintf(stderr, "================================\n");
		(void) fprintf(stderr, "Obsolescent function:\n");
		(void) fprintf(stderr, "   gmt_to_lmt(\n");
		(void) fprintf(stderr, "              STRING tstamp,\n");
		(void) fprintf(stderr, "              float  lon)\n");
		(void) fprintf(stderr, "================================\n");
		(void) fprintf(stderr, "Replace with:\n");
		(void) fprintf(stderr, "   gmt_to_local(\n");
		(void) fprintf(stderr, "               STRING tstamp,\n");
		(void) fprintf(stderr, "               float  lon)\n");
		(void) fprintf(stderr, "================================\n");
		}

	return lmt_to_lmt(tstamp, 0.0, lon);
	}

/***********************************************************************
*                                                                      *
*     l o c a l _ t o _ g m t                                          *
*     g m t _ t o _ l o c a l                                          *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Convert a local time at long to GTM
 *
 * @param[in] tstamp local mean time
 * @param[in] lon    longitude (real degrees, +=E)
 * @return a time stamp string in yyyy:jjj:hh:MML format.
 **********************************************************************/
STRING	local_to_gmt

	(
	STRING	tstamp,
	float	lon
	)

	{
	int		year, jday, hour, min;
	LOGICAL	local, mins;

	if ( !parse_tstamp(tstamp, &year, &jday, &hour, &min, &local, &mins) )
			return NULL;
	if ( !local ) return tstamp;

	/* Shift hour to GMT */
	hour -= hours_from_gmt(lon);
	tnorm(&year, &jday, &hour, &min, NullInt);

	return build_tstamp(year, jday, hour, min, FALSE, mins);
	}

/**********************************************************************/

/**********************************************************************/
/** Convert a GMT time to local time at lon
 *
 * @param[in]  tstamp  Greenwich mean time
 * @param[in]  lon     longitude (real degrees, +=E)
 * @return a time stamp string in yyyy:jjj:hh:MML format.
 **********************************************************************/
STRING	gmt_to_local

	(
	STRING	tstamp,
	float	lon
	)

	{
	int		year, jday, hour, min;
	LOGICAL	local, mins;

	if ( !parse_tstamp(tstamp, &year, &jday, &hour, &min, &local, &mins) )
			return NULL;
	if ( local ) return tstamp;

	/* Shift hour from GMT */
	hour += hours_from_gmt(lon);
	tnorm(&year, &jday, &hour, &min, NullInt);

	return build_tstamp(year, jday, hour, min, TRUE, mins);
	}

/***********************************************************************
*                                                                      *
*     c o m p a r e _ t s t a m p s                                    *
*                                                                      *
*     Return values:                                                   *
*          -2 --> error with ts1                                       *
*          -1 --> ts1 precedes ts2                                     *
*           0 --> ts1 is equivalent to ts2                             *
*          +1 --> ts1 follows ts2                                      *
*          +2 --> error with ts2                                       *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Compare two timestamps to determine <,>,= status
 *
 * @param[in]  ts1 First timestamp to compare
 * @param[in]  ts2 Second timestamp to compare
 * @param[in]  lon Longitude of comparison.
 * @return - 	-2 --> error with ts1,
 * 		 -	-1 --> ts1 precedes ts2,
 * 		 -	 0 --> ts1 is equivalent to ts2,
 * 		 -	+1 --> ts1 follows ts2,
 * 		 -	+2 --> error with ts2
 **********************************************************************/
int		compare_tstamps

	(
	STRING	ts1,
	STRING	ts2,
	float	lon
	)

	{
	int		y1, j1, h1, m1;
	int		y2, j2, h2, m2;
	LOGICAL	lf1, mf1, lf2, mf2;
	int		dm;

	if ( !parse_tstamp(ts1, &y1, &j1, &h1, &m1, &lf1, &mf1) ) return -2;
	if ( !parse_tstamp(ts2, &y2, &j2, &h2, &m2, &lf2, &mf2) ) return  2;

	if (lf1) h1 += hours_from_gmt(lon);
	if (lf2) h2 += hours_from_gmt(lon);

	dm = mdif(y1, j1, h1, m1, y2, j2, h2, m2);
	if (dm > 0) return -1;
	if (dm < 0) return  1;
	return 0;
	}

/***********************************************************************
*                                                                      *
*     m a t c h i n g _ t s t a m p s                                  *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Determine if two timestamps are equivalent.
 *
 * @param[in] ts1 	First timestamp to compare
 * @param[in] ts2 	Second timestamp to compare
 * @return TRUE if ts1 == ts2, FALSE otherwise.
 **********************************************************************/
LOGICAL	matching_tstamps

	(
	STRING	ts1,
	STRING	ts2
	)

	{
	int		y1, j1, h1, m1;
	int		y2, j2, h2, m2;
	LOGICAL	lf1, mf1, lf2, mf2;
	int		dm;

	if ( blank(ts1) && blank(ts2) ) return TRUE;

	if ( !parse_tstamp(ts1, &y1, &j1, &h1, &m1, &lf1, &mf1) ) return FALSE;
	if ( !parse_tstamp(ts2, &y2, &j2, &h2, &m2, &lf2, &mf2) ) return FALSE;

	if ((lf1 && !lf2) || (lf2 && !lf1)) return FALSE;

	dm = mdif(y1, j1, h1, m1, y2, j2, h2, m2);
	if (dm != 0) return FALSE;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     c l o s e s t _ t s t a m p                                      *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Search a list of timestamps and return the index of the one
 *     closest to a given timestamp.  The search range may be limited
 *     by a time before and a time after the given timestamp.
 *     The function sets the index of timestamps within the range
 *     given by the before and after times.
 *     The function returns -1 if no timestamp is within the range.
 *	@param[in]	nvt 	number of valid times in valid time list
 *	@param[in]	*vts 	valid times list to search
 *	@param[in]	vtime 	valid time to match
 *	@param[in]	lon 	reference longitude for local times
 *	@param[in]	before 	lower limit for matching window in hh:MM format
 *	@param[in]	after 	upper limit for matching window in hh:MM format
 *	@param[out]	*first 	pointer to the first valid time in the window
 *	@param[out]	*last	pointer to last valid time in the window
 *  @return
 *  -  0 to nvt-1  --> index of closest timestamp,
 *  -    -1        --> error with timestamps,
 *  -    -2        --> all timestamps are before range,
 *  -    -3        --> all timestamps are after range,
 *  -    -4        --> no timestamps within range
 *
 **********************************************************************/
int		closest_tstamp

	(
	int		nvt,
	STRING	*vts,
	STRING	vtime,
	float	lon,
	STRING	before,
	STRING	after,
	int		*first,
	int		*last
	)

	{
	LOGICAL	ascend, status;
	int		nfirst, nlast, ninc, nf, nl, nc, tmin, xmin, nn;
	TSTAMP	fvt, lvt;

	/* Initialize return parameters */
	if ( NotNull(first) ) *first = -1;
	if ( NotNull(last) )  *last  = -1;

	/* Check for parameters */
	if ( nvt < 1 || IsNull(vts) ) return -1;

	/* Determine if timestamps are ascending or descending in time */
	if ( compare_tstamps(vts[nvt-1], vts[0], lon) >= 0)
		{
		ascend = TRUE;
		nfirst =  0;
		nlast  = nvt-1;
		ninc   =  1;
		}
	else
		{
		ascend = FALSE;
		nfirst = nvt-1;
		nlast  =  0;
		ninc   = -1;
		}

	/* Find index of first timestamp */
	if ( blank(before) )
		nf = nfirst;
	else
		{
		tmin = interpret_hour_minute_string(before);
		(void) safe_strcpy(fvt, calc_valid_time_minutes(vtime, 0, -tmin));
		for ( nf=nfirst; ; nf+=ninc )
			{
			if (  ascend && nf>nlast ) return -2;
			if ( !ascend && nf<nlast ) return -2;
			if ( compare_tstamps(vts[nf], fvt, lon) >= 0) break;
			}
		}

	/* Find index of last timestamp */
	if ( blank(after) )
		nl = nlast;
	else
		{
		tmin = interpret_hour_minute_string(after);
		(void) safe_strcpy(lvt, calc_valid_time_minutes(vtime, 0, tmin));
		for ( nl=nlast; ; nl-=ninc )
			{
			if (  ascend && nl<nfirst ) return -3;
			if ( !ascend && nl>nfirst ) return -3;
			if ( compare_tstamps(vts[nl], lvt, lon) <= 0) break;
			}
		}

	/* No timestamps found within range */
	if (  ascend && nf>nl ) return -4;
	if ( !ascend && nf<nl ) return -4;

	/* Find closest timestamp within range */
	nc   = -1;
	xmin = MAXINT;
	for ( nn=nf; ; nn+=ninc )
		{
		if (  ascend && nn>nl ) break;
		if ( !ascend && nn<nl ) break;
		tmin = calc_prog_time_minutes(vtime, vts[nn], &status);
		if ( !status ) return -1;
		if ( abs(tmin) < abs(xmin) )
			{
			nc   = nn;
			xmin = tmin;
			}
		}

	/* Set return paramters */
	if ( NotNull(first) ) *first = nf;
	if ( NotNull(last) )  *last  = nl;
	return nc;
	}

/***********************************************************************
*                                                                      *
*     t s t a m p _ t o _ m i n u t e s                                *
*     t s t a m p _ t o _ h o u r s                                    *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Convert from hours-only to minutes format.
 *
 *     tstamp_to_minutes will add :00 if minutes are not already
 *     present.
 *
 *     Both functions return a flag, similar to the return value from
 *     compare_tstamps, to indicate if the modified tstamp precedes,
 *     is equivalent to, or follows the original one:
 *
 *     -    -2 --> error with original
 *     -    -1 --> original precedes modified
 *     -     0 --> original is equivalent to modified
 *     -    +1 --> original follows modified
 *     -    +2 --> (will never be generated)
 *
 *     In the case of tstamp_to_minutes, this will always be zero
 *     unless there is an error in the original tstamp.
 *
 *	@param[in] tstamp Timestamp to convert
 *	@param[out] *cmp  flag indicates how new time relates to given time.
 *  @return (possibly modified) tstamp uses recycled memory,
 *    and may even point to the original if no change is required.
 *    Make a copy of the new tstamp if you will need it after the
 *    next call to this function or build_tstamp.
 *
 **********************************************************************/

 STRING	tstamp_to_minutes

    (
	STRING	tstamp,
	int    *cmp
	)

	{
	int		year, jday, hour, min;
	LOGICAL	local, mins;

	if ( !parse_tstamp(tstamp, &year, &jday, &hour, &min, &local, &mins) )
		{
		if (NotNull(cmp)) *cmp = -2;
		return NullString;
		}

	/* Already in minutes??? */
	if (mins)
		{
		if (NotNull(cmp)) *cmp = 0;
		return tstamp;
		}

	/* Build the new tstamp */
	return build_tstamp(year, jday, hour, 0, local, TRUE);
	}

/**********************************************************************/
/**********************************************************************/
/** Convert to hours-only from minutes format.
 *
 *     tstamp_to_hours will truncate or round to hours, according to
 *     the round flag:
 *     - set to TRUE to round to nearest hour
 *     - set to FALSE to truncate minutes
 *
 *     Both functions return a flag, similar to the return value from
 *     compare_tstamps, to indicate if the modified tstamp precedes,
 *     is equivalent to, or follows the original one:
 *
 *     -    -2 --> error with original
 *     -    -1 --> original precedes modified
 *     -     0 --> original is equivalent to modified
 *     -    +1 --> original follows modified
 *     -    +2 --> (will never be generated)
 *
 *  @param[in] tstamp Timestamp to convert
 *  @param[in] round  round up (TRUE), or truncate (FALSE)
 *  @param[out] *cmp   flag indicates how new time relates to given time
 *  @return (possibly modified) tstamp uses recycled memory,
 *     and may even point to the original if no change is required.
 *     Make a copy of the new tstamp if you will need it after the
 *     next call to this function or build_tstamp.
 *
 **********************************************************************/
STRING	tstamp_to_hours

	(
	STRING	tstamp,
	LOGICAL	round,
	int    *cmp
	)

	{
	int		year, jday, hour, min;
	LOGICAL	local, mins;

	if ( !parse_tstamp(tstamp, &year, &jday, &hour, &min, &local, &mins) )
		{
		if (NotNull(cmp)) *cmp = -2;
		return NullString;
		}

	/* Already in hours-only??? */
	if (!mins)
		{
		if (NotNull(cmp)) *cmp = 0;
		return tstamp;
		}

	if (min==0)
		{
		/* Minutes are zero - no need to round */
		if (NotNull(cmp)) *cmp = 0;
		}
	else if (round)
		{
		/* Round to nearest hour */
		if (min >= 30)
			{
			/* Round up */
			/* Modified time will follow original */
			hour++;
			if (NotNull(cmp)) *cmp = -1;
			}
		else
			{
			/* Round down */
			/* Modified time will precede original */
			if (NotNull(cmp)) *cmp = 1;
			}
		}
	else
		{
		/* Truncate */
		/* Modified time will precede original */
		if (NotNull(cmp)) *cmp = 1;
		}

	/* Build the new tstamp */
	return build_tstamp(year, jday, hour, 0, local, FALSE);
	}
