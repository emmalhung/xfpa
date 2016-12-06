/****************************************************************************
*
*  File:     time_fcns.c
*
*  Purpose:  Fucntions for the manipulation of time strings.
*
*  Note: The standard format of the time string is yyyy:jjj:hh:mm
*        where yyyy is the year, jjj the julian day, hh the hour and
*        mm the minute. If minute = 0, then the format is yyyy:jjj:hh.
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
*
****************************************************************************/

#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include "global.h"


/*   Parses string which indicate a time relative to T0. There are two
*    possible formats h:m or d/h:m.
*
*    In the first case the time is difference from T0, where the minutes
*    (:m) part is optional and the time may be + or -.
*
*    The second case specified the day relative to T0 and absolute time
*    within that day. Thus 1/12 would be the T0 day plus 1 (tomorrow)
*    at 12Z absolute on that day.
*
*    In both cases the hour must exist even if zero, ie. 0:12
*/
String ParseTimeDeltaString(String intime)
{
	int val, yr, jd, hr, min, sec = 0;

	static TSTAMP  tbuf;

	if(!parse_tstamp(GV_T0_depict, &yr, &jd, &hr, &min, NULL, NULL)) return NULL;

	if(sscanf(intime, "%d/%s", &val, tbuf) == 2)
	{
		jd += val;
		hr  = 0;
		min = interpret_hour_minute_string(tbuf);
	}
	else
	{
		min += interpret_hour_minute_string(intime);
	}
	tnorm(&yr, &jd, &hr, &min, &sec);
	strcpy(tbuf, build_tstamp(yr, jd, hr, min, False, minutes_in_depictions()));
	return tbuf;
}


/* Return true if intime is in the given time list.  If posn is not NULL
*  and the return is true, posn is the position in the list at which
*  intime was found.
*/
static DATE_FORMAT time_list_resolution = MINUTES;

void InTimeListResolution( DATE_FORMAT res )
{
	time_list_resolution = res;
}

Boolean InTimeList(String intime, String *tl, int ntl, int *posn)
{
	int     i, yr1, jd1, hr1, min1, yr2, jd2, hr2, min2;
	Boolean l1, l2;

	if (posn) *posn = 0;

	if(!tl) return False;
	if(blank(intime)) return False;
	if(!parse_tstamp(intime, &yr1, &jd1, &hr1, &min1, &l1, NULL)) return False;

	for( i = 0 ; i < ntl ; i++ )
	{
		if(!parse_tstamp(tl[i], &yr2, &jd2, &hr2, &min2, &l2, NULL)) continue;
		if((!l1 && l2) || (l1 && !l2)) continue;
		switch(time_list_resolution)
		{
			case DAYS:
				if(yr1 != yr2 || jd1 != jd2) continue;
				break;
			case HOURS:
				if(yr1 != yr2 || jd1 != jd2 || hr1 != hr2) continue;
				break;
			default:
				if(yr1 != yr2 || jd1 != jd2 || hr1 != hr2 || min1 != min2) continue;
				break;
		}
		if (posn) *posn = i;
		time_list_resolution = MINUTES;
		return True;
	}
	time_list_resolution = MINUTES;
	return False;
}


/* Return the time difference in minutes between two times.
*/
int MinuteDif(char *dt1 , char *dt2 )
{
	int yr1, jd1, h1, m1, yr2, jd2, h2, m2;

	if(!parse_tstamp(dt1, &yr1, &jd1, &h1, &m1, NULL, NULL)) return 0;
	if(!parse_tstamp(dt2, &yr2, &jd2, &h2, &m2, NULL, NULL)) return 0;
	return mdif(yr1, jd1, h1, m1, yr2, jd2, h2, m2);
}


/* Return the time difference in hours between two times.
*/
int HourDif(char *dt1 , char *dt2 )
{
	int yr1, jd1, hr1, yr2, jd2, hr2;

	if(!parse_tstamp(dt1, &yr1, &jd1, &hr1, NULL, NULL, NULL)) return 0;
	if(!parse_tstamp(dt2, &yr2, &jd2, &hr2, NULL, NULL, NULL)) return 0;
	return hdif(yr1, jd1, hr1, yr2, jd2, hr2);
}


static void general_date_format(String src, String fmt, String dt)
{
	int    year,jday,hour,min;
	long   t;

	strcpy(dt, "- - - - -");
	if(parse_tstamp(src,&year,&jday,&hour,&min,NULL,NULL))
	{
		t = encode_clock(year, jday, hour, min, 0);
		set_locale_from_environment(LC_TIME);
		(void) strftime(dt, 40, fmt, gmtime((time_t*)&t));
		reset_locale();
	}
}


/* Returns the strftime() style formats for dates valid to the nearest
*  minute, hour and day.
*/
void GetDateFormats(String *m, String *h, String *d)
{
	const String md = "%Y/%m/%d %H:%M", mr = ".minuteDateDisplay";
	const String hd = "%Y/%m/%d %H"   , hr = ".hourlyDateDisplay";
	const String dd = "%a, %b %d, %Y" , dr = ".dailyDateDisplay";

	static String mf = NULL, hf = NULL, df = NULL;

	if (!mf) mf = XtNewString(XuGetStringResource(mr,md));
	if (!hf) hf = XtNewString(XuGetStringResource(hr,hd));
	if (!df) df = XtNewString(XuGetStringResource(dr,dd));

	if (m) *m = mf;
	if (h) *h = hf;
	if (d) *d = df;
}


/* Convert time from any seconds to time display as specified in the
*  resource file. The returned pointer is to an internal static array.
*  Do not free!
*/
String UnixSecToMinuteDateFormat( long tm )
{
	String fmt;
	static char dt[40];

	GetDateFormats(&fmt, NULL, NULL);
	set_locale_from_environment(LC_TIME);
	(void) strftime(dt, 40, fmt, gmtime((time_t*)&tm));
	reset_locale();
	return dt;
}


/* Create a time format depending on the field type
*/
String DepictFieldDateFormat( FIELD_INFO *fld, String dt )
{
	if( fld->info->element->elem_tdep->time_dep == FpaC_DAILY )
		return DateString(dt,DAYS);

	if (minutes_in_depictions())
		return DateString(dt,MINUTES);

	return DateString(dt,HOURS);
}


String TimeDiffFormat( String dt1, String dt2, Boolean display_minutes )
{
	static char buf[20];

	if(display_minutes)
	{
		int d = MinuteDif(dt1,dt2);
		(void)  snprintf(buf,sizeof(buf),"T%+.2d:%.2d", d/60, abs(d%60));
	}
	else
	{
		(void)  snprintf(buf,sizeof(buf),"T%+.2d",HourDif(dt1,dt2));
	}
	return buf;
}


/* Create a string giving the offset of the time dt from the reference time
*  reft. If the depictions allow minutes then the format will be T+hh:mm or
*  T-hh:mm. If no minutes are allowed then we will have T+hh or T-hh.
*/
String TimeOffsetString( String reft, String dt )
{
	static TSTAMP tbuf;

	if(minutes_in_depictions())
	{
		int  mn, md, hd;
		char sn;

		if( valid_tstamp(reft) && valid_tstamp(dt) )
		{
			mn = MinuteDif(reft,dt);
			hd = abs(mn/60);
			md = abs(mn%60);
			sn = (char)((mn < 0) ? '-':'+');
			(void)  snprintf(tbuf, sizeof(tbuf), "T%c%.2d:%.2d", sn, hd, md);
		}
		else
		{
			strcpy(tbuf, "--:--");
		}
	}
	else
	{
		if( valid_tstamp(reft) && valid_tstamp(dt) )
			(void)  snprintf(tbuf, sizeof(tbuf), "T%+.2d", HourDif(reft,dt));
		else
			strcpy(tbuf, "--");
	}
	return tbuf;
}


/* Returns the current system time in standard format or if the [demo.data]
*  key is in the setup file that date is returned and put into the environment
*  as FPA_DEMO_DATE. If this is not the case the environment is checked for
*  the FPA_DEMO_DATE variable and if set it  returns that time as the "current"
*  system time. The resolution determins the "grainess" of the intervals in hours.
*/
void AppDateTime(String dt , int resolution )
{
	int      n, year,jday,hour,month,day,min,sec;
	char     mbuf[128];
	String   dd, tm;
	Boolean  ok;
	PARM    *p;

	static int     demo_year     = 0; 
	static int     demo_jday     = 0;
	static int     demo_hour     = 0;
	static Boolean first_time_in = True;
	static String  demo_key      = "FPA_DEMO_DATE";

	if(first_time_in)
	{
		ok = False;
		first_time_in = False;
		p = GetSetupParms(DEMO_DATE);
		if(p && p->nparms > 0)
		{
			/* This allows for the possibility that the hour is separated from the
			*  day by a space and not be a slash.
			*/
			strcpy(mbuf, p->parm[0]);
			for(n = 1; n < p->nparms; n++) { strcat(mbuf,"/"); strcat(mbuf,p->parm[n]); }

			tm = interpret_timestring(mbuf, GV_T0_depict, 0);
			ok = parse_tstamp(tm, &year, &jday, &hour, &min, NULL, NULL);
			if(ok)
			{
				mdate(&year, &jday, &month, &day);
				(void)  snprintf(mbuf, sizeof(mbuf), "%s=%d/%.2d/%.2d/%.2d", demo_key, year, month, day, hour);
				(void) putenv(XtNewString(mbuf));
			}
		}
		else if((dd = getenv(demo_key)) != NULL)
		{
			tm = interpret_timestring(dd, GV_T0_depict, 0);
			ok = parse_tstamp(tm, &year, &jday, &hour, &min, NULL, NULL);
		}
		if (ok)
		{
			demo_year = year;
			demo_jday = jday;
			demo_hour = hour;
			dd = DateString(build_tstamp(demo_year,demo_jday,demo_hour,0,False,False),HOURS);
			(void) printf("[FPA Date/Time] %s %s\n", XuGetLabel("demoDate"), dd);
		}
	}

	min = sec = 0;
	if(demo_year)
	{
		hour = demo_hour - demo_hour%resolution;
		tnorm(&demo_year, &demo_jday, &hour, &min, &sec);
		strcpy(dt,build_tstamp(demo_year,demo_jday,hour,min,False,minutes_in_depictions()));
	}
	else
	{
		/* minutes and seconds must not be retrieved as we want hours only */
		systime(&year, &jday, &hour, NULL, NULL);
		hour = hour - hour%resolution;
		tnorm(&year, &jday, &hour, &min, &sec);
		strcpy(dt, build_tstamp(year, jday, hour, min, False, minutes_in_depictions()));
	}
}


/* Returns the current system local date-time as a string in standard format.
*/
String sysClockFmt(void)
{
	int year,jday,hour,min,sec;
	static TSTAMP dt;

	systime(&year,&jday,&hour,&min,&sec);
	strcpy(dt, build_tstamp(year, jday, hour, min, False, minutes_in_depictions()));
	return dt;
}


/* Return the date as a string according to the specified format key.
 * If the input is not a valid date just return the string.
 */
String DateString(String dt , DATE_FORMAT format )
{
	int    year, jday, month, day;
	char   *fmt, names[128];
	static char strfmt[30];

	strcpy(strfmt,"");

	if(!valid_tstamp(dt)) return dt;

	switch(format)
	{
		case DEFAULT_FORMAT:
			if(minutes_in_depictions())
				GetDateFormats(&fmt, NULL, NULL);
			else
				GetDateFormats(NULL, &fmt, NULL);
			general_date_format(dt, fmt, strfmt);
			break;

		case MINUTES:
			GetDateFormats(&fmt, NULL, NULL);
			general_date_format(dt, fmt, strfmt);
			break;

		case HOURS:
			GetDateFormats(NULL, &fmt, NULL);
			general_date_format(dt, fmt, strfmt);
			break;

		case DAYS:
			GetDateFormats(NULL, NULL, &fmt);
			general_date_format(dt, fmt, strfmt);
			break;

		case DAY_NR:
			(void)parse_tstamp(dt, &year, &jday, NULL, NULL, NULL, NULL);
			mdate(&year, &jday, &month, &day);
			(void)  snprintf(strfmt, sizeof(strfmt), "%d", day);
			break;

		case SHORT_DAY_NAME:
			(void)parse_tstamp(dt, &year, &jday, NULL, NULL, NULL, NULL);
			strcpy(names, XuFindKeyLine(weekday(year,jday), "day", NULL));
			(void)string_arg(names);
			(void)  snprintf(strfmt, sizeof(strfmt), "%s", string_arg(names));
			break;

		case SHORT_DAY_NAME_NR_OF_MONTH:
			(void)parse_tstamp(dt, &year, &jday, NULL, NULL, NULL, NULL);
			mdate(&year, &jday, &month, &day);
			strcpy(names, XuFindKeyLine(weekday(year,jday), "day", NULL));
			(void)string_arg(names);
			(void)  snprintf(strfmt, sizeof(strfmt), "%s %d", string_arg(names), day);
			break;

		case LONG_DAY_NAME:
			(void)parse_tstamp(dt, &year, &jday, NULL, NULL, NULL, NULL);
			strcpy(names, XuFindKeyLine(weekday(year,jday), "day", NULL));
			(void)  snprintf(strfmt, sizeof(strfmt), "%s", string_arg(names));
			break;

		case LONG_DAY_NAME_NR_OF_MONTH:
			(void)parse_tstamp(dt, &year, &jday, NULL, NULL, NULL, NULL);
			mdate(&year, &jday, &month, &day);
			strcpy(names, XuFindKeyLine(weekday(year,jday), "day", NULL));
			(void)  snprintf(strfmt, sizeof(strfmt), "%s %d", string_arg(names), day);
			break;

		default:
			(void) printf("[FPA Date/Time] Unrecognized date format type.\n");
			break;
	}
	return strfmt;
}


/*  Returns the time of the depiction given a delta time from T0. If the
*   given time delta does not exist and match is EXACT then NULL is returned.
*   If match is NEXT or PREV and a depiction at the specified delta does not
*   exist, then the depiction next in the sequence after/before the delta
*   time is chosen.
*
*   The delta may be in the format of offset from T0 in hours and
*   minutes or in the form del_day/time where del_day if the day delta from
*   T0 and time is an absolute time.
*/
Boolean GetDepictionTimeFromOffset(Source src, String dt, DATE_MATCH dm, String *sdt)
{
	int     i, nlist, yr1, yr2, jd1, jd2, h1, h2, m1, m2;
	TSTAMP  intime;
	String  *list;
	Boolean found;
	static  TSTAMP tm;

	strcpy(tm, "");
	if (sdt) *sdt = tm;

	(void) strncpy(intime, dt, sizeof(TSTAMP));

	if(!valid_tstamp(GV_T0_depict)) return False;

	strcpy(tm, ParseTimeDeltaString(intime));
	if(IsNull(src)) return False;
	if(!parse_tstamp(tm, &yr1, &jd1, &h1, &m1, NULL, NULL)) return False;

	nlist = FilteredValidTimeList(src->fd, FpaC_NORMAL, &list);
	found = InTimeList(tm, list, nlist, NULL);

	if(!found && dm != EXACT)
	{
		if(dm == PREV)
		{
			for(i = nlist - 1; i >= 0; i--)
			{
				if(!parse_tstamp(list[i], &yr2, &jd2, &h2, &m2, NULL, NULL)) continue;
				if(mdif(yr1,jd1,h1,m1, yr2,jd2,h2,m2) > 0) continue;
				strcpy(tm, list[i]);
				found = True;
				break;
			}
		}
		else
		{
			for(i = 0; i < nlist; i++)
			{
				if(!parse_tstamp(list[i], &yr2, &jd2, &h2, &m2, NULL, NULL)) continue;
				if(mdif(yr1,jd1,h1,m1, yr2,jd2,h2,m2) < 0) continue;
				strcpy(tm, list[i]);
				found = True;
				break;
			}
		}
	}
	nlist = FilteredValidTimeListFree(&list, nlist);
	return found;
}



/* Create time difference display string. The time is shown as a difference between the
 * T0 depiction time and the given time. If the difference is an even hour then the hours
 * are returned using the hour_font_tag. If the difference is not an even hour, then the
 * minutes of the hour difference are returned using the minute_font_tag.
 */
XmString XmStringSequenceTime( String timestr, String hour_font_tag, String minute_font_tag )
{
	int      dh, dm, dt, min;
	char     mbuf[20];
	String   tag;
	XmString label;

	if(valid_tstamp(timestr))
	{
		/* If we don't have a T0 depiction use the current system time */
		if(valid_tstamp(GV_T0_depict))
			min = MinuteDif(GV_T0_depict, timestr);
		else
			min = MinuteDif(sysClockFmt(), timestr);

		dm  = min % 60;
		dh  = min / 60;

		if (dm)
		{
			dt  = dm;
			tag = minute_font_tag;
		}
		else
		{
			dt  = dh;
			tag = hour_font_tag;
		}
		(void) snprintf(mbuf, sizeof(mbuf), "%.2d", dt);
		label = XmStringCreate(mbuf,tag);
	}
	else
	{
		label = XmStringCreate("--", hour_font_tag);
	}
	return label;
}


/*  Function to display the time in a sequence selection button according to the
*   time containing minutes or not. Hours and minutes are shown as is but the
*   font used for minutes is different than that used for the even hours. The
*   hour and minutes font are different depending the SEQUENCE_TYPE. The fonts
*   for the main sequence should be part of the fontList for the main sequence
*   selection buttons and the normal and very small fonts should be part of the
*   overall system font list. See global.h for the font macro values.
*/
void SetSequenceBtnTime( Widget w, String timestr, SEQUENCE_TYPE key )
{
	XmString label;

	if( key == MAIN_SEQUENCE )
		label = XmStringSequenceTime(timestr, SEQUENCE_HOUR_FONT, SEQUENCE_MINUTE_FONT);
	else
		label = XmStringSequenceTime(timestr, NORMAL_FONT, VERY_SMALL_FONT);

	XtVaSetValues(w, XmNlabelString, label, NULL);
	XmStringFree(label);
}


/* This is a direct replacement for the checked_valid_time_list() function found in the
 * Ingred library. If the depiction database is in minutes then any valid time can be 
 * imported into the depiction set, so the valid time list is returned as is. If the
 * database is in hours, then only input times that are flagged as not having minutes
 * or having minutes but with a minute of 0 are copied into the output array. Note
 * that it is the responsibility of the calling process to free the returned memory with
 * a call to checked_valid_time_list_free(). This function is coded to be compatable
 * with the memory allocation routines used in checked_valid_time_list.
 */
int FilteredValidTimeList( FLD_DESCRIPT *fd, int macro, STRING **list )
{
	int    nvl;
	STRING *vl;

	nvl = checked_valid_time_list(fd, macro, &vl);
	if(!minutes_in_depictions())
	{
		int n;
		for(n = 0; n < nvl; n++)
		{
			int     min;
			LOGICAL mins;
			if(!parse_tstamp(vl[n],NULL,NULL,NULL,&min,NULL,&mins) || (mins && min != 0))
			{
				int    i, nv = 0;
				STRING *v = INITMEM(STRING,nvl);
				for(i = 0; i < nvl; i++)
				{
					v[i] = NULL;
					if(!parse_tstamp(vl[i],NULL,NULL,NULL,&min,NULL,&mins)) continue;
					if(mins && min != 0) continue;
					v[nv++] = safe_strdup(vl[i]);
				}
				nvl = checked_valid_time_list_free(&vl, nvl);
				vl  = v;
				nvl = nv;
				break;
			}
		}
	}
	if (list) {
		*list = vl;
	} else {
		FREELIST(vl,nvl);
	}
	return nvl;
}


int FilteredValidTimeListFree(STRING **list, int nlist)
{
	return checked_valid_time_list_free(list, nlist);
}
