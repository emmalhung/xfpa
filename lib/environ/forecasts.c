/**********************************************************************/
/** @file forecasts.c
 *
 * To read information related to forecast generation (FoG) from the
 * FPA config file. Thereby this information need not be hardcoded
 * in the Prolog code.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/************************************************************************
*                                                                       *
*    File:     forecasts.c                                              *
*                                                                       *
*    Purpose:  To read information related to forecast generation       *
*              (FoG) from the FPA config file.  Thereby this            *
*              information need not be hardcoded in the Prolog code.    *
*                                                                       *
*    Usage:    The FPA setup file first needs to be opened using        *
*                                                                       *
*                    define_setup (setup_file_name);                    *
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

#include "forecasts.h"    /* see $(FPA)/lib/environ/forecasts.h             */
#include "read_setup.h"   /* see $(FPA)/lib/environ/read_setup.h            */
#include "files_and_directories.h"
                          /* see $(FPA)/lib/environ/files_and_directories.h */

#include <tools/tools.h>  /* see $(FPA)/lib/tools/tools.h                   */
#include <fpa_types.h>    /* see $(FPA)/lib/include/fpa_types.h             */
#include <fpa_getmem.h>   /* see $(FPA)/lib/include/fpa_getmem.h            */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef struct
	{
	STRING  fid;            /* forecast identifier, eg. hx_22       */
	int     nkey;           /* no. of filename keys/values          */
	STRING  *f_key;         /* ptr to list of filename keys         */
	STRING  *f_val;         /* ptr to list of filenames             */
	STRING  tz;             /* time zone -- overrides current TZ    */
	int     nsit;           /* no. of summer issue times            */
	int     *sit;           /* ptr to summer issue times (in GMT)   */
	int     *sit_v;         /* ptr to regular valid periods for sit */
	int     *sit_o;         /* ptr to outlook valid periods for sit */
	int     nwit;           /* no. of winter issue times            */
	int     *wit;           /* ptr to winter issue times (in GMT)   */
	int     *wit_v;         /* ptr to regular valid periods for wit */
	int     *wit_o;         /* ptr to outlook valid periods for wit */
	} FORECAST;

/* Global variables to hold forecast config info */
static  LOGICAL  read_forecast_config(void);
static  LOGICAL  Fready = FALSE;              /* config already read in ?     */
static  int      Nfcst = 0;                   /* no. of different forecasts   */
static  FORECAST *Fcst = NullPtr(FORECAST *); /* all the info stored together */
static  FORECAST *find_forecast(STRING);
static  int      find_forecast_issue(FORECAST *, int, int, int *);

/************************************************************************
*                                                                       *
*    F o G _ f i l e                                                    *
*                                                                       *
*    This routine returns a constructed filespec depending on the       *
*    parameters provided.  All parameters are input parameters.  By     *
*    default, FoG (Forecast Generator) files are constructed as:        *
*                                                                       *
*       <path>/<id_tag>.<suffix>.<paragraph_number>                     *
*                                                                       *
*    <path> is determined by the source keyword which maps to a key   *
*                       into the directories block of the setup file    *
*    <id_tag> is the forecast identifer, eg. hx_22, eg_20, etc.         *
*    <suffix> is a single character indicating forecast language that   *
*                       is determined by the type keyword               *
*    <paragraph_number> is an integer between 1-99,                     *
*                             or  0 specifying the full text file       *
*                             or -1 specifying all files (appends '*')  *
*                                                                       *
*    Note:  FPA does a "cd" to the "home" directory of the setup file   *
*           if it exists.  If not, get_directory() returns absolute     *
*           paths given $FPA or $HOME environment variables.  Otherwise *
*           filenames are relative to the current directory.            *
*                                                                       *
************************************************************************/

STRING  FoG_file

	(
	STRING  id_tag,
	STRING  source,    /* "work", "concepts", "released", or "merged" */
	STRING  language,  /* "english", "french", "meteo", "status", or "all" */
	int     npar       /* paragraph no.:  1-99, 0=full, -1=all */
	)

	{
	static  STRING  file = NullString;   /* constructed filename */

	int		nc;
	STRING  p;                           /* basename associated with file_key */
	STRING  dir, subdir, tmp;
	STRING  suffix;
	char    num[4];

	if ( npar < -1 || npar > 99 ) npar = 0;  /* default to full text */
	num[0] = '.';
	if ( npar == -1 ) (void) strcpy(&num[1],"*");
	else if ( npar == 0 ) num[0] = '\0';
	else
		(void) sprintf(&num[1], "%d", npar);

	if      (same_start(language,"e")) suffix = ".e";       /* english */
	else if (same_start(language,"f")) suffix = ".f";       /* french  */
	else if (same_start(language,"c")) suffix = ".c";       /* meteocode */
	else if (same_start(language,"s")) suffix = ".s";       /* status */
	else if (same_start(language,"a")) suffix = ".?";       /* all */
	else return NullString;

	if      (same_start(source,"w"))
		{
		dir = get_directory("fcst.work");
		subdir = NullString;
		}
	else if (same_start(source,"c"))
		{
		dir = get_directory("fcst.concept");
		subdir = NullString;
		}
	else if (same_start(source,"r"))
		{
		dir = get_directory("fcst.release");
		subdir = NullString;
		}
	else if (same_start(source,"m"))
		{
		dir = get_directory("fcst.release");
		subdir = ".merged/";
		if      (same_start(language,"e")) suffix = ".me"; /* english */
		else if (same_start(language,"f")) suffix = ".mf"; /* french  */
		else if (same_start(language,"c")) suffix = ".c";  /* meteocode */
		else if (same_start(language,"a")) suffix = ".*"; /* all */
		else return NullString;
		}
	else
		return NullString;

	nc  = 1;
	nc += safe_strlen(subdir);
	nc += safe_strlen(id_tag);
	nc += safe_strlen(suffix);
	nc += safe_strlen(num);
	p   = INITMEM(char, nc);
	(void) safe_strcpy(p,subdir);
	(void) safe_strcat(p,id_tag);
	(void) safe_strcat(p,suffix);
	(void) safe_strcat(p,num);

	/* Now add a path. */
	/* Note: pathname returns a pointer to an internal static string     */
	/*       or to one of its parameters.  Insulate the user of FoG_file */
	/*       from this by making a copy. */
	FREEMEM(file);
	tmp = pathname(dir,p);  /* Note tmp points to internal static var. */
	file = INITMEM(char,safe_strlen(tmp)+1);
	(void) safe_strcpy(file,tmp);
	FREEMEM(p);

	return file;
	}

/************************************************************************
*                                                                       *
*    F o G _ s e t u p _ f i l e                                        *
*                                                                       *
*    Prepends a path onto a FoG setup parameter given a key into the    *
*    directories block of the setup file.                               *
*                                                                       *
************************************************************************/

STRING  FoG_setup_file

	(
	STRING  id_tag,
	STRING  dir_key,
	STRING  file_key
	)

	{
	STRING  p;                      /* basename associated with file_key */

	if (!read_forecast_config() ) return NullString;

	/* First get whatever was found in the config file. */
	p = FoG_setup_parm (id_tag,file_key);
	if ( !p ) return NullString;
	if ( !dir_key ) return p;
	if ( same(dir_key, "maps") ) dir_key = "Maps";
	return source_path_by_name(dir_key, NullString, NullString, p);
	}

/************************************************************************
*                                                                       *
*    F o G _ s e t u p _ p a r m                                        *
*                                                                       *
************************************************************************/

STRING  FoG_setup_parm

	(
	STRING  id_tag,
	STRING	key
	)

	{
	FORECAST        *fcst;
	int     i;

	if ( !read_forecast_config() ) return NullString;

	fcst = find_forecast(id_tag);
	if ( !fcst ) return NullString;

	/* Get whatever was found in the config file. */
	for (i = 0; i < fcst->nkey; i++)
		{
		if (same(fcst->f_key[i],key))
			return fcst->f_val[i];
		}

	return NullString;
	}

/************************************************************************
*                                                                       *
*    F o G _ c o p y _ f i l e s                                        *
*                                                                       *
*    When FoG first runs, it places its output (consisting of full and  *
*    single paragraph concepts and english and/or french text files)    *
*    into the "fcst.work" directory.  This directory is defined in      *
*    the FPA setup file.                                                *
*                                                                       *
*    The version 2 interface allows the user to start up the Bulletin   *
*    Editor.  When this happens, work files are copied -to- the         *
*    "fcst.concept" directory.  After the user is finished with the     *
*    Bulletin Editor, work files are copied -from- the "fcst.concept"   *
*    directory back to the "fcst.work" directory.                       *
*                                                                       *
*    When the user is fully satisfied with the forecast, a button is    *
*    pressed on the interface that copies only full text english or     *
*    french forecasts -to- the "fcst.release" directory.                *
*                                                                       *
************************************************************************/

#define YES  TRUE
#define NO   FALSE

LOGICAL	FoG_copy_files

	(
	STRING  id_tag,
	STRING  cmd     /* "to_concept_edit",
					   "from_concept_edit",
					   "amd_from_concept_edit",
					   "cor_from_concept_edit",
					   "nor_from_concept_edit",
					   "to_release",
					or "from_release" */
	)

	{
	STRING  english, french;
	int     status;
	STRING	*slist, setup;
	int		nsetup;
	char	buf[1024];

	nsetup = current_setup_list(&slist);
	if (nsetup <= 0)
		return FALSE;
	setup = slist[0];

	if (same(cmd,"to_concept_edit"))
		{
		(void) sprintf(buf,"wtoc.sh %s %s english", setup, id_tag);

		status = shrun(buf,YES);
		}
	else if (same(cmd,"from_concept_edit"))
		{
		(void) sprintf(buf,"ctow.sh %s %s normal", setup, id_tag);

		status = shrun(buf,YES);
		}
	else if (same(cmd,"amd_from_concept_edit"))
		{
		(void) sprintf(buf,"ctow.sh %s %s amend", setup, id_tag);

		status = shrun(buf,YES);
		}
	else if (same(cmd,"cor_from_concept_edit"))
		{
		(void) sprintf(buf,"ctow.sh %s %s correct", setup, id_tag);

		status = shrun(buf,YES);
		}
	else if (same(cmd,"nor_from_concept_edit"))
		{
		(void) sprintf(buf,"ctow.sh %s %s normal", setup, id_tag);

		status = shrun(buf,YES);
		}
	else if (same(cmd,"from_release"))
		{
		(void) sprintf(buf,"rtow.sh %s %s", setup, id_tag);

		status = shrun(buf,YES);
		}
	else if (same(cmd,"to_release"))
		{
		(void) sprintf(buf,"wtor.sh %s %s", setup, id_tag);

		status = shrun(buf,YES);
		if (status != 0) return FALSE;

		/* Once FPA has delivered the forecast files to the release
		   directory, other systems may want access to these files.
		   Run a script that can be modified by end-users. */
		english = strtok_arg( FoG_setup_parm(id_tag,"english") );
		if (english == NullString) english = "null";
		french = strtok_arg( FoG_setup_parm(id_tag,"french") );
		if (french == NullString) french = "null";
		(void) sprintf(buf,"fpa_text_out.sh %s %s %s %s", setup, id_tag, english, french);

		status = shrun(buf,NO);	/* don't wait for child to return */
		}
	else
		return FALSE;

	if (status != 0) return FALSE;
	return TRUE;
	}

/************************************************************************
*                                                                       *
*       F o G _ o v e r r i d e _ t z                                   *
*                                                                       *
************************************************************************/

LOGICAL	FoG_override_tz

	(
	STRING  id_tag
	)

	{
	int     found;                  /* flag set TRUE if timezone provided */
									/* is found in TZTAB       */
	char    line[129];              /* line of text from file TZTAB    */
	int     ncl = 128;              /* number of characters in line    */
	STRING  tz, val;
	FILE    *fp;
	FORECAST        *fcst;

	/* When putenv is called, strng becomes part of the environment */
	/* so it has to stick around. */
	static  STRING  strng = NullString;
	const   static  STRING  TZTAB = "/usr/lib/tztab";

	if (!read_forecast_config() ) return FALSE;

	/* Find the requested forecast. */
	fcst = find_forecast(id_tag);
	if ( !fcst ) return FALSE;

	/* Get the timezone. */
	tz = fcst->tz;
	if ( !tz ) return FALSE;

	/* Check the validity of the timezone in the UNIX tztab file. */
	fp = fopen(TZTAB,"r");
	if ( !fp )
		{
		(void) fprintf(stderr,"Unable to open file %s.\n",TZTAB);
		(void) fprintf(stderr,"Therefore unable to confirm validity of %s.\n\n",tz);
		}
	else
		{
		found = FALSE;
		line[ncl] = '\0';
		while ( getfileline(fp,line,ncl) )
			{
			val = string_arg(line);
			if ( same(val,tz) )
				{
				found = TRUE;
				break;
				}
			}
		(void) fclose(fp);
		if (!found)
			{
			(void) fprintf(stderr,"Specified timezone not in %s.\n",TZTAB);
			(void) fprintf(stderr,"Keeping current timezone.\n");
			return FALSE;
			}
		}

	/* Now set the TZ environment variable. */
	FREEMEM(strng);
	strng = INITMEM(char,safe_strlen(tz)+4);
	strng = safe_strcpy(strng,"TZ=");
	strng = safe_strcat(strng,tz);
	(void) putenv(strng);
	return TRUE;
	}

/************************************************************************
*                                                                       *
*       F o G _ p r e v _ i s s u e _ t i m e                           *
*       F o G _ n e x t _ i s s u e _ t i m e                           *
*                                                                       *
*       A time is specified (GMT for the former, local for the latter)  *
*       and the previous/next issue_time is returned.                   *
*                                                                       *
************************************************************************/

LOGICAL	FoG_prev_issue_time

	(
	STRING  id_tag,
	int     gyear,		/* Target time (input). */
	int		gjday,
	int		gmonth,
	int		gday,
	int		ghour,
	int		gminute,
	int     dst,		/* daylight savings time flag:
							1 = daylight savings time is in effect,
							0 = standard time is in effect */
	int     *iyear,		/* Issue time (input). */
	int		*ijday,
	int		*imonth,
	int		*iday,
	int		*itime,
	int     *ivalid,
	int		*ioutlook
	)

	{
	FORECAST	*fcst;
	int			gtm, day, j;

	if ( !read_forecast_config() ) return FALSE;

	if ( !iyear || !ijday || !imonth || !iday ) return FALSE;
	if ( !itime || !ivalid || !ioutlook ) return FALSE;

	/* Find the requested forecast. */
	fcst = find_forecast(id_tag);
	if ( !fcst ) return FALSE;

	gtm = ghour*100 + gminute;
	day = 0;

	/* Search (ordered) list of issue times depending on whether */
	/* daylight savings time is in effect or not.  Note:  If the */
	/* current time happens to equal an issue time, it's too late */
	/* to issue for that time, so use it as the last scheduled time. */
	j = find_forecast_issue(fcst,gtm,dst,&day);
	j--;

	if (dst)
		{
		/* If start of list, issue time is yesterday. */
		if (j < 0)
			{
			j = fcst->nsit-1;
			day--;
			}
		*itime = fcst->sit[j];
		*ivalid = fcst->sit_v[j];
		*ioutlook = fcst->sit_o[j];
		}
	else
		{
		/* If start of list, issue time is yesterday. */
		if (j < 0)
			{
			j = fcst->nwit-1;
			day--;
			}
		*itime = fcst->wit[j];
		*ivalid = fcst->wit_v[j];
		*ioutlook = fcst->wit_o[j];
		}

	/* Set date.  See $(FPA)/lib/tools/time.c for mdate.  */
	*iyear = gyear;
	*ijday = gjday + day;
	mdate (iyear,ijday,imonth,iday);

	return TRUE;
	}

LOGICAL	FoG_next_issue_time

	(
	STRING  id_tag,
	int     lyear,		/* Local reference time (input). */
	int     ljday,
	int     lmonth,
	int     lday,
	int     lhour,
	int     lminute,
	int     dst_flag,	/* daylight savings time flag:
							 1 = daylight savings time is in effect,
							 0 = standard time is in effect,
							-1 = don't know    */
	int     *iyear,		/* Issue time (output). */
	int     *ijday,
	int     *imonth,
	int     *iday,
	int     *itime,
	int     *ivalid,
	int     *ioutlook
	)

	{
	int     fyear,fjday,fmonth,fday,ftime,fvalid,foutlook;

	if ( !read_forecast_config() ) return FALSE;

	/* Store the current clock value to be on the safe side. */
	save_clock();

	/* Input a date-time value that the time interpretation routines
	   can use. */
	bogus_clock (lyear,ljday,lmonth,lday,lhour,lminute,dst_flag);

	/* Look up what the next issue time is from the config file.
	   Values for the following issue time are not used. */
	(void) FoG_issue_times(id_tag,iyear,ijday,imonth,iday,itime,ivalid,ioutlook,
			&fyear,&fjday,&fmonth,&fday,&ftime,&fvalid,&foutlook);

	/* Restore the clock value to whatever it was. */
	restore_clock();

	return TRUE;
	}

/************************************************************************
*                                                                       *
*       F o G _ i s s u e _ t i m e s                                   *
*       F o G _ i s s u e _ l a s t                                     *
*                                                                       *
*       The system clock is read and the next two scheduled issue times *
*       or the preceding issue time are returned.                       *
*                                                                       *
************************************************************************/

LOGICAL	FoG_issue_times

	(
	STRING  id_tag,
					/* Issue time (output). */
	int     *iyear,
	int     *ijday,
	int     *imonth,
	int     *iday,
	int     *itime,
	int     *ivalid,
	int     *ioutlook,
					/* Following issue time (output). */
	int     *fyear,
	int     *fjday,
	int     *fmonth,
	int     *fday,
	int     *ftime,
	int     *fvalid,
	int     *foutlook
	)

	{
	int     j, k;
	int     gyear,gjday,gmonth,gday,gwday,ghour,gminute,gjday2;
	int	lyear,ljday,lmonth,lday,lwday,lhour,lminute;
	char	*tzstamp;
	int	t1;			/* clock sample time		*/
	int     dstflag, issuedst, nextdst;
	int     itcode, fitcode;        /* 0 = today; 1 = tomorrow      */
	int	max_it;			/* max. no. of issue times	*/
	FORECAST        *fcst;

	if (!read_forecast_config() ) return FALSE;

	/* Find the requested forecast. */
	fcst = find_forecast(id_tag);
	if ( !fcst ) return FALSE;

	if ( !iyear || !ijday || !imonth || !iday ) return FALSE;
	if ( !itime || !ivalid || !ioutlook ) return FALSE;
	if ( !fyear || !fjday || !fmonth || !fday ) return FALSE;
	if ( !ftime || !fvalid || !foutlook ) return FALSE;

	/* Get current time in GMT. */
	/* Encode to same format as in config file.      */
	gtime (&gyear,&gjday,&gmonth,&gday,&gwday,&ghour,&gminute);
	t1 = ghour*100 + gminute;

	/* Search (ordered) list of issue times depending on whether */
	/* daylight savings time is in effect or not.  Note:  If the */
	/* current time happens to equal an issue time, it's too late */
	/* to issue for that time, so go to the next time in the list.*/
	itcode = 0;
	fitcode = 0;
	convert_gmt_to_local( gyear,gjday,gmonth,gday,ghour,gminute,
		&lyear,&ljday,&lmonth,&lday,&lwday,&lhour,&lminute,
		&tzstamp,&dstflag);	/* get dstflag */
	j = find_forecast_issue(fcst,t1,dstflag,&itcode);

	/* We now have to worry about the situation where we have sampled
	   the time in standard time but the issue time is in daylight
	   savings time (or vice versa). */
	if (dstflag)
		*itime = fcst->sit[j];
	else
		*itime = fcst->wit[j];

	ghour = *itime / 100;
	gminute = *itime % 100;
	gjday2 = gjday + itcode;
	convert_gmt_to_local( gyear,gjday2,0,0,ghour,gminute,
		&lyear,&ljday,&lmonth,&lday,&lwday,&lhour,&lminute,
		&tzstamp,&issuedst);	/* get dst flag for issue time */

	j = find_forecast_issue(fcst,t1,issuedst,&itcode);

	if (issuedst)
		{
		*itime = fcst->sit[j];
		*ivalid = fcst->sit_v[j];
		*ioutlook = fcst->sit_o[j];
		}
	else
		{
		*itime = fcst->wit[j];
		*ivalid = fcst->wit_v[j];
		*ioutlook = fcst->wit_o[j];
		}

	/* If end of list, issue time is tomorrow. */
	/* If current issue time is tomorrow, then so is following
	   issue time (we assume there are at least two issues per day). */
	if (itcode) fitcode = 1;

	k = j + 1;
	if (issuedst) max_it = fcst->nsit;
	else	max_it = fcst->nwit;
	if (k >= max_it)
		{
		k = 0;
		fitcode = 1;
		}

	if (issuedst) *ftime = fcst->sit[k];
	else	 *ftime = fcst->wit[k];

	/* Again we have to check that we haven't changed daylight savings
	   time regimes between the issue time and the next issue time. */
	ghour = *ftime / 100;
	gminute = *ftime % 100;
	gjday2 = gjday + fitcode;
	convert_gmt_to_local( gyear,gjday2,0,0,ghour,gminute,
		&lyear,&ljday,&lmonth,&lday,&lwday,&lhour,&lminute,
		&tzstamp,&nextdst);	/* get next dst flag */

	if (nextdst)
		{
		*ftime = fcst->sit[k];
		*fvalid = fcst->sit_v[k];
		*foutlook = fcst->sit_o[k];
		}
	else
		{
		*ftime = fcst->wit[k];
		*fvalid = fcst->wit_v[k];
		*foutlook = fcst->wit_o[k];
		}

	/* Set date.  See $(FPA)/lib/tools/time.c for mdate.  */
	*iyear = gyear;
	*ijday = gjday + itcode;
	mdate (iyear,ijday,imonth,iday);

	*fyear = gyear;
	*fjday = gjday + fitcode;
	mdate (fyear,fjday,fmonth,fday);

	return TRUE;
	}

LOGICAL	FoG_issue_last

	(
	STRING  id_tag,
					/* Issue time (output). */
	int     *iyear,
	int     *ijday,
	int     *imonth,
	int     *iday,
	int     *itime,
	int     *ivalid,
	int     *ioutlook
	)

	{
	int     gyear,gjday,gmonth,gday,gwday,ghour,gminute,dst;

	/* Get current time in GMT. */
	gtime (&gyear,&gjday,&gmonth,&gday,&gwday,&ghour,&gminute);
	dst = daylite();

	/* Get issue time before this */
	return FoG_prev_issue_time(id_tag,
		gyear,gjday,gmonth,gday,ghour,gminute,dst,
		iyear,ijday,imonth,iday,itime,ivalid,ioutlook);
	}

/************************************************************************
*                                                                       *
*       STATIC (LOCAL) ROUTINES                                         *
*                                                                       *
*       All the routines after this point are available only within     *
*       this file.                                                      *
*                                                                       *
************************************************************************/

/************************************************************************
*                                                                       *
*       r e a d _ f o r e c a s t _ c o n f i g                         *
*                                                                       *
************************************************************************/

static  LOGICAL read_forecast_config(void)
	{
	STRING  line, cmd;
	/* temporary variables for improved readability -- init to Null     */
	/*                                                 for GETMEM       */
	int     *sp = NullInt;          /* ptr to summer issue times        */
	int     *wp = NullInt;          /* ptr to winter issue times        */
	int     i;                      /* index value for above            */

	int     j, tmp, mark;           /* variables used for sorting       */
	int     ival;
	LOGICAL	status;

	/* Have we already read the config? */
	if (Fready) return TRUE;

	/* Read the "forecasts" config file. */
	if ( !open_config_file("forecasts") ) return FALSE;

	while (line = config_file_line() )
		{
		cmd = string_arg(line);
		if ( !cmd ) continue;

		if (same(cmd,"forecast"))
			{
			Nfcst++;
			Fcst = GETMEM(Fcst,FORECAST,Nfcst);
			Fcst[Nfcst-1].fid = strdup_arg(line);
			Fcst[Nfcst-1].nkey = 0;
			Fcst[Nfcst-1].f_key = NullStringList;
			Fcst[Nfcst-1].f_val = NullStringList;
			Fcst[Nfcst-1].tz  = NullString;
			Fcst[Nfcst-1].nsit = 0;
			Fcst[Nfcst-1].sit = NullInt;
			Fcst[Nfcst-1].sit_v = NullInt;
			Fcst[Nfcst-1].sit_o = NullInt;
			Fcst[Nfcst-1].nwit = 0;
			Fcst[Nfcst-1].wit = NullInt;
			Fcst[Nfcst-1].wit_v = NullInt;
			Fcst[Nfcst-1].wit_o = NullInt;
			}
		else if (same(cmd,"timezone"))
			{
			Fcst[Nfcst-1].tz = strdup_arg(line);
			}
		else if (same(cmd,"summer_issue"))
			{
			ival = int_arg(line,&status);
			if ( !status ) continue;

			i = Fcst[Nfcst-1].nsit + 1;
			sp = Fcst[Nfcst-1].sit;
			sp = GETMEM(sp,int,i);
			sp[i-1] = ival;
			/* Sort list, and mark new insertion. */
			mark = 0;
			for (j = i-1; j > 0; j--)
				{
				if (sp[j-1] > sp[j])
					{
					tmp = sp[j-1];
					sp[j-1] = sp[j];
					sp[j] = tmp;
					}
				else
					{
					mark = j;
					break;
					}
				}
			Fcst[Nfcst-1].nsit = i;
			Fcst[Nfcst-1].sit = sp;

			/* Now add valid time at mark. */
			sp = Fcst[Nfcst-1].sit_v;
			sp = GETMEM(sp,int,i);
			sp[i-1] = int_arg(line,&status);
			if (!status) sp[i-1] = 33;      /* some default value */
			for (j = i-1; j > mark; j--)
				{
				tmp = sp[j-1];
				sp[j-1] = sp[j];
				sp[j] = tmp;
				}
			Fcst[Nfcst-1].sit_v = sp;

			/* Now add outlook flag at mark. */
			sp = Fcst[Nfcst-1].sit_o;
			sp = GETMEM(sp,int,i);
			sp[i-1] = int_arg(line,&status);
			if (!status) sp[i-1] = 0;      /* default value: no outlook */
			for (j = i-1; j > mark; j--)
				{
				tmp = sp[j-1];
				sp[j-1] = sp[j];
				sp[j] = tmp;
				}
			Fcst[Nfcst-1].sit_o = sp;
			}

		else if (same(cmd,"winter_issue"))
			{
			ival = int_arg(line,&status);
			if ( !status ) continue;

			i = Fcst[Nfcst-1].nwit + 1;
			wp = Fcst[Nfcst-1].wit;
			wp = GETMEM(wp,int,i);
			wp[i-1] = ival;
			/* Sort list, and mark new insertion. */
			mark = 0;
			for (j = i-1; j > 0; j--)
				{
				if (wp[j-1] > wp[j])
					{
					tmp = wp[j-1];
					wp[j-1] = wp[j];
					wp[j] = tmp;
					}
				else
					{
					mark = j;
					break;
					}
				}
			Fcst[Nfcst-1].nwit = i;
			Fcst[Nfcst-1].wit = wp;

			/* Now add valid range at mark. */
			wp = Fcst[Nfcst-1].wit_v;
			wp = GETMEM(wp,int,i);
			wp[i-1] = int_arg(line,&status);
			if (!status) wp[i-1] = 24;      /* some default value */
			for (j = i-1; j > mark; j--)
				{
				tmp = wp[j-1];
				wp[j-1] = wp[j];
				wp[j] = tmp;
				}
			Fcst[Nfcst-1].wit_v = wp;

			/* Now add outlook flag at mark. */
			wp = Fcst[Nfcst-1].wit_o;
			wp = GETMEM(wp,int,i);
			wp[i-1] = 0;    /* some default value */
			wp[i-1] = int_arg(line,&status);
			if (!status) wp[i-1] = 0;      /* default value: no outlook */
			for (j = i-1; j > mark; j--)
				{
				tmp = wp[j-1];
				wp[j-1] = wp[j];
				wp[j] = tmp;
				}
			Fcst[Nfcst-1].wit_o = wp;
			}
		else
			/* Anything not recognized as a keyword above is */
			/* assumed to be a "soft" keyword. Strip leading */
			/* blanks and save.                              */
			{
			i = Fcst[Nfcst-1].nkey + 1;
			Fcst[Nfcst-1].f_key =
							 GETMEM(Fcst[Nfcst-1].f_key,char *,i);
			Fcst[Nfcst-1].f_key[i-1] = strdup(cmd);

			Fcst[Nfcst-1].f_val =
							 GETMEM(Fcst[Nfcst-1].f_val,char *,i);
			Fcst[Nfcst-1].f_val[i-1] = strdup(strrem_arg(line));
			Fcst[Nfcst-1].nkey = i;
			}
		}

	close_config_file();
	Fready = TRUE;
	return TRUE;
	}

/************************************************************************
*                                                                       *
*       f i n d _ f o r e c a s t                                       *
*       f i n d _ f o r e c a s t _ i s s u e                           *
*                                                                       *
************************************************************************/

static  FORECAST        *find_forecast

	(
	STRING  id_tag
	)

	{
	FORECAST        *fcst;
	int     i;

	for ( i=0; i<Nfcst; i++ )
		{
		fcst = Fcst + i;
		if ( same(id_tag,fcst->fid) ) return fcst;
		}

	return NullPtr(FORECAST *);
	}

static	int	find_forecast_issue

	(
	FORECAST	*fcst,
	int			tm,
	int			dst,
	int			*tomorrow
	)

	{
	int		j;

	if (tomorrow) *tomorrow = 0;
	if (!fcst) return -1;

	if (dst)
		{
		for (j = 0; j < fcst->nsit; j++)
			{
			if ( fcst->sit[j] > tm ) return j;
			}
		if (tomorrow) *tomorrow = 1;
		return 0;
		}

	else
		{
		for (j = 0; j < fcst->nwit; j++)
			{
			if ( fcst->wit[j] > tm ) return j;
			}
		if (tomorrow) *tomorrow = 1;
		return 0;
		}
	}

#define MAX_PARSE_STRING	128

LOGICAL FoG_WeatherEntryValid

	(
	STRING	in_string,		/* string containing weather entry */
	STRING	*type,			/* returns "fog","cloud","precip",or "error" */
	STRING	*error_code,	/* returns the ascii error key (see below) */
	int		*start_pos,		/* first character of error in string (>=0) */
	int		*end_pos		/* last character of error in string */
	)

/*
	This is the interface to "hotparse", a Prolog executable based
	on FoG's weather parser in wxdcg.pl.  "hotparse" writes to standard
	output and FoG_WeatherEntryValid reads this via a Unix pipe.

	Error_code values:	ERROR MESSAGE TO USER
	==================	=====================
	bad_input		Not a legal FPA code.
	hot_parse		Unable to run hot parse program.
	modification		Only one modification is permitted.
	none    		< This is not an error condition >
	no_visby_cause		Reduced visibility must have a cause.
	parsing 		Illegal sequence.
	trash   		This input has been discarded.
	visibility		Visibilities must be from 0 to 8.
	weather			At most 3 weather varieties are allowed.
*/

{
	FILE	*pfd;
	int	len;
	char	cmd[MAX_PARSE_STRING+10];	/* 10 is strlen("hotparse")+2 */
	static	char	buft[7];	/* 7 is strlen("precip")+1 */
	static	char	buf[15];	/* 15 is strlen("no_visby_cause")+1   */
					/* ie. the maximum error_code length  */

	len = safe_strlen( in_string );
	if ( len > MAX_PARSE_STRING ) {
		in_string[MAX_PARSE_STRING] = '\0';
		len = MAX_PARSE_STRING;
		}

	*start_pos = 0;
	*end_pos = len-1;
	(void) strcpy(buft,"error");		/* default is 'guilty until   */
	*type = buft;						/* proven innocent'           */
	(void) strcpy(buf,"hot_parse");
	*error_code = buf;

	(void) sprintf( cmd, "hotparse '%s'", in_string );
	pfd = popen( cmd, "r" );
	if (!pfd) {
		perror("no pipe to hotparse");
		return FALSE;
		}

	if (!fscanf(pfd,"%s",buft)) {
		perror("hotparse not found");
		}
	else {
		*type = buft;
		(void) fscanf(pfd,"%s",buf);
		*error_code = buf;
		(void) fscanf(pfd,"%d",start_pos);
		(void) fscanf(pfd,"%d",end_pos);
		}

	(void) pclose( pfd );

	if ( strcmp( *error_code , "none" ) != 0 ) return FALSE;
	return TRUE;
}

#ifdef FORECAST_STANDALONE

main()
{
	STRING	s;

	s = FoG_file("eg_22","work","english",0);
	printf( "working english file is %s\n", s );
	s = FoG_file("eg_22","release","status",0);
	printf( "release status file is %s\n", s );
	s = FoG_file("eg_22","concepts","concepts",1);  /* not used */
	printf( "concepts meteocode file is %s\n", s );
	s = FoG_file("eg_22","merged","concepts",0);
	printf( "concepts meteocode file is %s\n", s );
}

#endif /* FORECAST_STANDALONE */
