/**********************************************************************/
/** @file ingest.c
 *
 * Routines to interpret the "ingest" setup block
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   i n g e s t . c                                                    *
*                                                                      *
*   Routines to interpret the "ingest" setup block                     *
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

#define INGEST_INIT		/* To initialize declarations in ingest.h */

#include "read_setup.h"
#include "ingest.h"

#include <objects/objects.h>
#include <tools/tools.h>
#include <fpa_types.h>
#include <fpa_getmem.h>

#include <stdio.h>
#include <string.h>

/* Internal static function to read "ingest" setup block */
static	int			read_ingest_setup(void);

/* Global variables to hold ingest information */
static	int			IngestBlockReady   = FALSE;		/* Has it been read yet */
static	char		RemoteHost[256]    = "";		/* Name of remote host */
static	char		RemoteDir[256]     = "";		/* Data dir on remote */
static	int			MonitorListSize    = 0;			/* No. of files to check */
static	STRING		*MonitorList = NullStringList;	/* List of files to check */
static	STRING		*MonitorDir  = NullStringList;	/* Corresponding dir */
static	STRING		*MonitorType = NullStringList;	/* Corresponding ingest */
static	UNSIGN		SchedulerWaitTime  = 300;		/* Time between checking */
static	char		StatusFile[256]    = "";		/* Scheduler status file */
static	char		ActiveLogFile[256] = "";		/* Scheduler log file */
static	char		BackupLogFile[256] = "";		/* Previous log file */
static	int			LogFileMoveHour    = 0;			/* Hour and              */
static	int			LogFileMoveMin     = 0;			/*  min to move log file */

/***********************************************************************
*                                                                      *
*   g e t _ i n g e s t _ r e m o t e                                  *
*   g e t _ i n g e s t _ m o n i t o r                                *
*   g e t _ i n g e s t _ w a i t                                      *
*   g e t _ i n g e s t _ s t a t                                      *
*   g e t _ i n g e s t _ l o g                                        *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Determine location of data to be ingested and point to
 * directory with dir?
 *
 * @note returned values are stored in a static variable within function
 * if you are not going to use them immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[out]	*dir the remote data directory
 *  @return The Remote Host name, "" if not present.
 **********************************************************************/

STRING		get_ingest_remote

	(
	STRING		*dir
	)

	{
	if (dir) *dir = NullString;
	if (!read_ingest_setup()) return NullString;

	if (dir) *dir = RemoteDir;
	return RemoteHost;
	}

/**********************************************************************/
/** Read and return a list of ingest monitors, directories
 * and types.
 *
 *	@param[out]	**list	the list of ingest monitors
 *	@param[out]	**dir 	the list of ingest directories
 *	@param[out]	**type 	the list of ingest types
 * @return The size of the lists.
 **********************************************************************/
int			get_ingest_monitor

	(
	STRING		**list,
	STRING		**dir,
	STRING		**type
	)

	{
	if (list) *list = NullStringList;
	if (dir)  *dir  = NullStringList;
	if (type) *type = NullStringList;
	if (!read_ingest_setup()) return 0;

	if (list) *list = MonitorList;
	if (dir)  *dir  = MonitorDir;
	if (type) *type = MonitorType;
	return MonitorListSize;
	}

/**********************************************************************/
/** Read the ingest wait time from the setup file.
 *
 * @return Number of seconds to wait between checking data file.
 **********************************************************************/
UNSIGN		get_ingest_wait

	(
	)

	{
	if (!read_ingest_setup()) return 0;

	return SchedulerWaitTime;
	}

/**********************************************************************/
/** Get the name of the ingest status file.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 * @return The name of the ingest status file.
 **********************************************************************/
STRING		get_ingest_stat

	(
	)

	{

	if (!read_ingest_setup()) return NullString;

	if (blank(StatusFile))
		{
		(void) safe_strcpy(StatusFile, get_path("ingest.stat", "ingest.stat"));
		}

	return StatusFile;
	}

/**********************************************************************/
/** Get the name of the ingest log file.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[out]	*lprev	the name of the old log file
 *	@param[out]	*mvhour	the time at which the log file should be shuffled
 *	@param[out]	*mvmin	the minute at which the log file should be shuffled
 * 	@return The name of the ingest log file.
 **********************************************************************/
STRING		get_ingest_log

	(
	STRING		*lprev,
	int			*mvhour,
	int			*mvmin
	)

	{

	if (lprev)  *lprev  = NullString;
	if (mvhour) *mvhour = 0;
	if (mvmin)  *mvmin  = 0;
	if (!read_ingest_setup()) return NullString;

	if (blank(ActiveLogFile))
		{
		(void) safe_strcpy(ActiveLogFile, get_path("ingest.log", "ingest.log"));
		}

	if (lprev)
		{
		if (blank(BackupLogFile))
			{
			(void) safe_strcpy(BackupLogFile, get_path("ingest.log", "ingest.old"));
			}
		*lprev = BackupLogFile;
		}

	if (mvhour) *mvhour = LogFileMoveHour;
	if (mvmin)  *mvmin  = LogFileMoveMin;

	return ActiveLogFile;
	}

/***********************************************************************
*                                                                      *
*   r e a d _ i n g e s t _ s e t u p                                  *
*                                                                      *
***********************************************************************/

static	int		read_ingest_setup

	(
	)

	{
	int			itime;
	LOGICAL		argOK;
	STRING		line, key, arg, type, dp, dir, patt;

	if (IngestBlockReady)                    return TRUE;
	if ( !find_setup_block("ingest", TRUE) ) return FALSE;

	/* Store the ingest block */
	MonitorListSize = 0;
	while ( line = setup_block_line() )
		{
		/* Read the keyword */
		key = string_arg(line);

		if (same(key, "wait"))
			{
			/* Read the wait time */
			itime = int_arg(line, &argOK);	if (!argOK) continue;
			SchedulerWaitTime = itime;
			}

		else if (same(key, "log"))
			{
			/* Read the log file name */
			arg = string_arg(line);			if (blank(arg)) continue;
			(void) safe_strcpy(ActiveLogFile, get_path("ingest.log", arg) );

			/* Read the old log file name */
			arg = string_arg(line);			if (blank(arg)) continue;
			(void) safe_strcpy(BackupLogFile, get_path("ingest.log", arg) );

			/* Read the log file changeover time */
			itime = int_arg(line, &argOK);	if (!argOK) continue;
			if (itime < 0)       continue;
			if (itime >= 2400)   continue;
			if (itime%100 >= 60) continue;
			LogFileMoveHour = itime/100;
			LogFileMoveMin  = itime%100;
			}

		else if (same(key, "status"))
			{
			/* Read the status file name */
			arg = string_arg(line);			if (blank(arg)) continue;
			(void) safe_strcpy(StatusFile, get_path("ingest.stat", arg) );
			}

		else if (same(key, "remote"))
			{
			/* Read the remote host name */
			arg = string_arg(line);			if (blank(arg)) continue;
			(void) safe_strcpy(RemoteHost, arg);

			/* Read the remote data directory */
			arg = string_arg(line);			if (blank(arg)) continue;
			(void) safe_strcpy(RemoteDir, arg);
			}

		else if (same(key, "monitor"))
			{
			/* Read the file spec and           */
			/* add the entry in the ingest list */
			dir  = strdup(".");
			arg  = string_arg(line);		if (blank(arg)) continue;
			type = strdup(arg);
			arg  = string_arg(line);		if (blank(arg)) continue;
			patt = strdup(arg);
			MonitorListSize++;
			MonitorList = GETMEM(MonitorList, STRING, MonitorListSize);
			MonitorDir  = GETMEM(MonitorDir,  STRING, MonitorListSize);
			MonitorType = GETMEM(MonitorType, STRING, MonitorListSize);
			MonitorList[MonitorListSize-1] = patt;
			MonitorDir[MonitorListSize-1]  = dir;
			MonitorType[MonitorListSize-1] = type;
			}

		else if (same(key, "monitor_in"))
			{
			/* Read the file spec and           */
			/* add the entry in the ingest list */
			arg  = string_arg(line);		if (blank(arg)) continue;
			dp   = get_directory(arg);		if (IsNull(dp)) continue;
			dir  = strdup(dp);
			arg  = string_arg(line);		if (blank(arg)) continue;
			type = strdup(arg);
			arg  = string_arg(line);		if (blank(arg)) continue;
			patt = strdup(arg);
			MonitorListSize++;
			MonitorList = GETMEM(MonitorList, STRING, MonitorListSize);
			MonitorDir  = GETMEM(MonitorDir,  STRING, MonitorListSize);
			MonitorType = GETMEM(MonitorType, STRING, MonitorListSize);
			MonitorList[MonitorListSize-1] = patt;
			MonitorDir[MonitorListSize-1]  = dir;
			MonitorType[MonitorListSize-1] = type;
			}

		}

	IngestBlockReady = TRUE;
	return TRUE;
	}
