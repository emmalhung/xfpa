/***********************************************************************
*                                                                      *
*     g r i b i n . c                                                  *
*                                                                      *
*     GRIB Data Ingest Program                                         *
*                                                                      *
*     This program reads the given GRIB file and separates the indi-   *
*     vidual fields into corresponding metafiles.  This is used to     *
*     ingest guidance fields received from CMC or other sources.       *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
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

#include "gribs.h"
#include "rgrib_edition1.h"
#include "rgrib_edition0.h"
#include "gribmeta.h"

#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>

static	LOGICAL	DebugMode = FALSE;

/* Trap for error situations */
static	void	error_trap(int);

/* Internal static functions to extract and output metafiles from GRIB data */
static	LOGICAL	process_gribfield(GRIBFIELD *, FLD_DESCRIPT *, STRING);
static	LOGICAL	process_gribfield_xycomp(GRIBFIELD *, FLD_DESCRIPT *, STRING);
static	LOGICAL	process_gribfield_default(GRIBFIELD *, FLD_DESCRIPT *, STRING);

/* Default message strings */
static	const	STRING	MyTitle = "FPA Ingest";
static			char	MyLabel[MAX_BCHRS];

/* Base directory shuffle and file lock information */
static	char	LockDir[MAX_BCHRS]   = "";
static	char	LockVtime[MAX_BCHRS] = "";
static	int		Locked               = FALSE;

/***********************************************************************
*                                                                      *
*     m a i n                                                          *
*                                                                      *
***********************************************************************/

int		main

	(
	int		argc,
	STRING	argv[]
	)

	{
	int				status;
	int				cyear, cjday, cmonth, cmday, chour, cmin, csec;
	int				nslist, iarg, nflds, mplus;
	LOGICAL			ok, minutes_rqd;
	STRING			sfile, *slist, dir;
	char			home[MAX_BCHRS], work[MAX_BCHRS], gribname[MAX_BCHRS];
	MAP_PROJ		*mproj;
	FLD_DESCRIPT	fdescinit, fdesc;
	GRIBFIELD		*gribfld;
	STRING			model, rtime, btime, etime, element, level, units;
	char			xrtime[GRIB_LABEL_LEN];
	char			xbtime[GRIB_LABEL_LEN];
	char			xetime[GRIB_LABEL_LEN];

	/* Ignore hangup, interrupt and quit signals so we can survive after */
	/* logging off */
	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	status = setvbuf(stdout, NullString, _IOLBF, 0);
	status = setvbuf(stderr, NullString, _IOLBF, 0);

	/* Set debug mode (if requested) */
	if ( DebugMode ) (void) pr_control(NULL, 5, 1);
	else             (void) pr_control(NULL, 3, 1);

	/* Validate run string parameters */
	if ( argc < 3 )
		{
		(void) fprintf(stderr, "Usage:\n");
		(void) fprintf(stderr, "   gribin <setup_file> <file> [<file> ...]\n");
		return (-1);
		}

	/* Obtain a licence */
	(void) app_license("ingest");

	/* Trap all signals that would abort the process by default */
	(void) set_error_trap(error_trap);

	/* Startup message */
	(void) sprintf(MyLabel, "[%d] %s:", getpid(), MyTitle);
	(void) systime(&cyear, &cjday, &chour, &cmin, &csec);
	(void) mdate(&cyear, &cjday, &cmonth, &cmday);
	(void) fprintf(stdout, "%s Beginning: %d/%.2d/%.2d %.2d:%.2d:%.2d GMT\n",
			MyLabel, cyear, cmonth, cmday, chour, cmin, csec);

	/* Read the setup file */
	/* This moves to standard FPA directory */
	sfile  = strdup(argv[1]);
	nslist = setup_files(sfile, &slist);
	if ( !define_setup(nslist, slist) )
		{
		(void) fprintf(stderr, "%s Problem with setup file \"%s\"\n",
				MyLabel, sfile);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Read the Config files */
	if ( !read_complete_config_file() )
		{
		(void) fprintf(stderr, "%s Problem with Config Files\n", MyLabel);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Read the Gribs configuration files */
	if ( !read_complete_gribs_file() )
		{
		(void) fprintf(stderr, "%s Problem with Gribs Files\n", MyLabel);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Retrieve the target projection definition */
	mproj = get_target_map();
	if ( !mproj )
		{
		(void) fprintf(stderr, "%s Target map not defined in setup file \"%s\"\n",
				MyLabel, sfile);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Retrieve the operating directory */
	dir = home_directory();
	(void) strcpy(home, dir);
	dir = get_directory("ingest.src");
	if ( blank(dir) )
		dir = getenv("FPA_LOCAL_GRIB");
	if ( !blank(dir) ) (void) strcpy(work, dir);
	else               (void) strcpy(work, home);

	/* Initialize the field descriptor for files */
	(void) init_fld_descript(&fdescinit);
	if ( !set_fld_descript(&fdescinit,
							FpaF_MAP_PROJECTION, mproj,
							FpaF_DIRECTORY_PATH, home,
							FpaF_END_OF_LIST) )
		{
		(void) fprintf(stderr, "%s Problem initializing field descriptor\n",
				MyLabel);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Process each GRIB file in order */
	for ( iarg=2; iarg<argc; iarg++ )
		{
		(void) strcpy(gribname, pathname(work, argv[iarg]));
		(void) fprintf(stdout, "\n%s Processing GRIB file \"%s\"\n",
				MyLabel, gribname);

		/* Initialize number of decoded GRIB fields */
		nflds = 0;

		/* First try Edition 1 decode */
		(void) fprintf(stdout, "\n%s GRIB Edition 1 decode\n", MyLabel);

		/* Open Edition 1 GRIB file */
		if ( !open_gribfile_edition1(gribname) )
			{
			(void) fprintf(stderr, "%s Cannot access GRIB file \"%s\"\n",
					MyLabel, gribname);
			continue;
			}

		/* Edition 1 decode of each field in the GRIB file */
		while ( next_gribfield_edition1(&gribfld) )
			{

			/* Reset number of decoded GRIB fields */
			nflds++;

			/* Get information about this gribfield */
			if ( !gribfield_identifiers_edition1(&model, &rtime, &btime,
					&etime, &element, &level, &units) )
				{
				(void) fprintf(stdout, "    Skipping unrecognized field\n");
				continue;
				}

			/* Replace default GRIB model (if required) */
			(void) replace_grib_default_model(&model);

			/* Replace default GRIB element and units (if required) */
			(void) replace_grib_default_element(&element, &units);

			/* Re-initialize the field descriptor */
			(void) copy_fld_descript(&fdesc, &fdescinit);

			/* Set the field descriptor directory info */
			if ( !set_fld_descript(&fdesc,
									FpaF_SOURCE_NAME,    model,
									FpaF_END_OF_LIST) )
				{
				(void) fprintf(stderr, "    Skipping non-FPA model \"%s\"\n",
						model);
				continue;
				}

			/* Set the prog time from run time and end valid time */
			mplus = calc_prog_time_minutes(rtime, etime, &ok);

			/* Make copies of the timestamps (depending on minutes required) */
			minutes_rqd = fdesc.sdef->minutes_rqd;
			if ( minutes_rqd )
				{
				(void) strcpy(xrtime, rtime);
				(void) strcpy(xbtime, btime);
				(void) strcpy(xetime, etime);
				}
			else
				{
				(void) strcpy(xrtime, tstamp_to_hours(rtime, TRUE, NullInt));
				(void) strcpy(xbtime, tstamp_to_hours(btime, TRUE, NullInt));
				(void) strcpy(xetime, tstamp_to_hours(etime, TRUE, NullInt));
				}

			/* Set the field descriptor file info with GRIB parameters */
			if ( !set_fld_descript(&fdesc,
									FpaF_RUN_TIME,     xrtime,
									FpaF_VALID_TIME,   xetime,
									FpaF_ELEMENT_NAME, element,
									FpaF_LEVEL_NAME,   level,
									FpaF_END_OF_LIST) )
				{

				/* Skip unrecognized fields */
				(void) fprintf(stdout, "    Skipping non-FPA field:");
				if ( NotNull(fdesc.sdef) && !blank(fdesc.sdef->name) )
					(void) fprintf(stdout, " %s", fdesc.sdef->name);
				else
					(void) fprintf(stdout, " (%s)", model);
				if ( NotNull(fdesc.subdef) && !blank(fdesc.subdef->name) )
					(void) fprintf(stdout, ":%s", fdesc.subdef->name);
				if ( NotNull(fdesc.rtime) && !blank(fdesc.rtime) )
					(void) fprintf(stdout, " %s", fdesc.rtime);
				else
					(void) fprintf(stdout, " (%s)", xrtime);
				if ( minutes_rqd )
					(void) fprintf(stdout, " T%s",
											hour_minute_string(0, mplus));
				else
					(void) fprintf(stdout, " T%+d", mplus/60);
				if ( NotNull(fdesc.edef) && !blank(fdesc.edef->name) )
					(void) fprintf(stdout, " %s", fdesc.edef->name);
				else
					(void) fprintf(stdout, " (%s)", element);
				if ( NotNull(fdesc.ldef) && !blank(fdesc.ldef->name) )
					(void) fprintf(stdout, " %s", fdesc.ldef->name);
				else
					(void) fprintf(stdout, " (%s)", level);
				(void) fprintf(stdout, "\n");
				continue;
				}

			/* Check for daily fields ... which should not be in GRIB files! */
			if ( fdesc.edef->elem_tdep->time_dep == FpaC_DAILY )
				{

				/* Skip daily fields */
				(void) fprintf(stdout, "    Skipping Daily field:");
				if ( NotNull(fdesc.sdef) && !blank(fdesc.sdef->name) )
					(void) fprintf(stdout, " %s", fdesc.sdef->name);
				else
					(void) fprintf(stdout, " (%s)", model);
				if ( NotNull(fdesc.subdef) && !blank(fdesc.subdef->name) )
					(void) fprintf(stdout, ":%s", fdesc.subdef->name);
				if ( NotNull(fdesc.rtime) && !blank(fdesc.rtime) )
					(void) fprintf(stdout, " %s", fdesc.rtime);
				else
					(void) fprintf(stdout, " (%s)", xrtime);
				if ( minutes_rqd )
					(void) fprintf(stdout, " T%s",
											hour_minute_string(0, mplus));
				else
					(void) fprintf(stdout, " T%+d", mplus/60);
				if ( NotNull(fdesc.edef) && !blank(fdesc.edef->name) )
					(void) fprintf(stdout, " %s", fdesc.edef->name);
				else
					(void) fprintf(stdout, " (%s)", element);
				if ( NotNull(fdesc.ldef) && !blank(fdesc.ldef->name) )
					(void) fprintf(stdout, " %s", fdesc.ldef->name);
				else
					(void) fprintf(stdout, " (%s)", level);
				(void) fprintf(stdout, "\n");
				continue;
				}

			/* Prepare data directory for GRIB field */
			if ( blank(prepare_source_directory(&fdesc)) )
				{

				/* Skip fields if directory cannot be created */
				(void) fprintf(stdout, "    Cannot create directory for:");
				if ( NotNull(fdesc.sdef) && !blank(fdesc.sdef->name) )
					(void) fprintf(stdout, " %s", fdesc.sdef->name);
				else
					(void) fprintf(stdout, " (%s)", model);
				if ( NotNull(fdesc.subdef) && !blank(fdesc.subdef->name) )
					(void) fprintf(stdout, ":%s", fdesc.subdef->name);
				if ( NotNull(fdesc.rtime) && !blank(fdesc.rtime) )
					(void) fprintf(stdout, " %s", fdesc.rtime);
				else
					(void) fprintf(stdout, " (%s)", xrtime);
				(void) fprintf(stdout, "\n");
				continue;
				}

			/* Check that metafile name can be created for field */
			if ( blank(construct_meta_filename(&fdesc))
					&& blank(build_meta_filename(&fdesc)) )
				{

				/* Skip fields if metafile name cannot be created */
				(void) fprintf(stdout, "    Cannot create metafile name for:");
				if ( NotNull(fdesc.sdef) && !blank(fdesc.sdef->name) )
					(void) fprintf(stdout, " %s", fdesc.sdef->name);
				else
					(void) fprintf(stdout, " (%s)", model);
				if ( NotNull(fdesc.subdef) && !blank(fdesc.subdef->name) )
					(void) fprintf(stdout, ":%s", fdesc.subdef->name);
				if ( NotNull(fdesc.rtime) && !blank(fdesc.rtime) )
					(void) fprintf(stdout, " %s", fdesc.rtime);
				else
					(void) fprintf(stdout, " (%s)", xrtime);
				if ( minutes_rqd )
					(void) fprintf(stdout, " T%s",
											hour_minute_string(0, mplus));
				else
					(void) fprintf(stdout, " T%+d", mplus/60);
				if ( NotNull(fdesc.edef) && !blank(fdesc.edef->name) )
					(void) fprintf(stdout, " %s", fdesc.edef->name);
				else
					(void) fprintf(stdout, " (%s)", element);
				if ( NotNull(fdesc.ldef) && !blank(fdesc.ldef->name) )
					(void) fprintf(stdout, " %s", fdesc.ldef->name);
				else
					(void) fprintf(stdout, " (%s)", level);
				(void) fprintf(stdout, "\n");
				continue;
				}

			/* Check for unwanted fields */
			if ( skip_grib_field_orig(fdesc.edef->name, fdesc.ldef->name) )
				{

				/* Skip unwanted fields */
				(void) fprintf(stdout, "    Skipping field:");
				if ( NotNull(fdesc.sdef) && !blank(fdesc.sdef->name) )
					(void) fprintf(stdout, " %s", fdesc.sdef->name);
				else
					(void) fprintf(stdout, " (%s)", model);
				if ( NotNull(fdesc.subdef) && !blank(fdesc.subdef->name) )
					(void) fprintf(stdout, ":%s", fdesc.subdef->name);
				if ( NotNull(fdesc.rtime) && !blank(fdesc.rtime) )
					(void) fprintf(stdout, " %s", fdesc.rtime);
				else
					(void) fprintf(stdout, " (%s)", xrtime);
				if ( minutes_rqd )
					(void) fprintf(stdout, " T%s",
											hour_minute_string(0, mplus));
				else
					(void) fprintf(stdout, " T%+d", mplus/60);
				if ( NotNull(fdesc.edef) && !blank(fdesc.edef->name) )
					(void) fprintf(stdout, " %s", fdesc.edef->name);
				else
					(void) fprintf(stdout, " (%s)", element);
				if ( NotNull(fdesc.ldef) && !blank(fdesc.ldef->name) )
					(void) fprintf(stdout, " %s", fdesc.ldef->name);
				else
					(void) fprintf(stdout, " (%s)", level);
				(void) fprintf(stdout, "\n");
				continue;
				}

			(void) fprintf(stdout, "    Extracting field:");
			if ( NotNull(fdesc.sdef) && !blank(fdesc.sdef->name) )
				(void) fprintf(stdout, " %s", fdesc.sdef->name);
			else
				(void) fprintf(stdout, " (%s)", model);
			if ( NotNull(fdesc.subdef) && !blank(fdesc.subdef->name) )
				(void) fprintf(stdout, ":%s", fdesc.subdef->name);
			if ( NotNull(fdesc.rtime) && !blank(fdesc.rtime) )
				(void) fprintf(stdout, " %s", fdesc.rtime);
			else
				(void) fprintf(stdout, " (%s)", xrtime);
			if ( minutes_rqd )
				(void) fprintf(stdout, " T%s", hour_minute_string(0, mplus));
			else
				(void) fprintf(stdout, " T%+d", mplus/60);
			if ( NotNull(fdesc.edef) && !blank(fdesc.edef->name) )
				(void) fprintf(stdout, " %s", fdesc.edef->name);
			else
				(void) fprintf(stdout, " (%s)", element);
			if ( NotNull(fdesc.ldef) && !blank(fdesc.ldef->name) )
				(void) fprintf(stdout, " %s", fdesc.ldef->name);
			else
				(void) fprintf(stdout, " (%s)", level);
			(void) fprintf(stdout, "\n");

			/* Extract and output metafile from GRIB field */
			if ( !process_gribfield(gribfld, &fdesc, units) ) continue;

			/* Process next field in current GRIB file */
			}

		/* Close GRIB file and process next one if Edition 1 decode */
		/* processed one or more fields                             */
		(void) close_gribfile_edition1();
		if ( nflds > 0 )
			{
			(void) fprintf(stdout, "\n%s   %d GRIB fields processed\n",
					MyLabel, nflds);
			continue;
			}

		/* Next try Edition 0 decode */
		(void) fprintf(stdout, "\n%s GRIB Edition 0 decode\n", MyLabel);

		/* Open Edition 0 GRIB file */
		if ( !open_gribfile_edition0(gribname) )
			{
			(void) fprintf(stderr, "%s Cannot access GRIB file \"%s\"\n",
					MyLabel, gribname);
			continue;
			}

		/* Edition 0 decode of each field in the GRIB file */
		while ( next_gribfield_edition0(&gribfld) )
			{

			/* Reset number of decoded GRIB fields */
			nflds++;

			/* Get information about this gribfield */
			if ( !gribfield_identifiers_edition0(&model, &rtime, &btime,
					&etime, &element, &level, &units) )
				{
				(void) fprintf(stdout, "    Skipping unrecognized field\n");
				continue;
				}

			/* Set the field descriptor directory info */
			if ( !set_fld_descript(&fdesc,
									FpaF_SOURCE_NAME,    model,
									FpaF_END_OF_LIST) )
				{
				(void) fprintf(stderr, "    Skipping non-FPA model \"%s\"\n",
						model);
				continue;
				}

			/* Set the prog time from run time and end valid time */
			mplus  = calc_prog_time_minutes(rtime, etime, &ok);

			/* Make copies of the timestamps (depending on minutes required) */
			minutes_rqd = fdesc.sdef->minutes_rqd;
			if ( minutes_rqd )
				{
				(void) strcpy(xrtime, rtime);
				(void) strcpy(xbtime, btime);
				(void) strcpy(xetime, etime);
				}
			else
				{
				(void) strcpy(xrtime, tstamp_to_hours(rtime, TRUE, NullInt));
				(void) strcpy(xbtime, tstamp_to_hours(btime, TRUE, NullInt));
				(void) strcpy(xetime, tstamp_to_hours(etime, TRUE, NullInt));
				}

			/* Set the field descriptor for files with GRIB parameters */
			if ( !set_fld_descript(&fdesc,
									FpaF_RUN_TIME,     xrtime,
									FpaF_VALID_TIME,   xetime,
									FpaF_ELEMENT_NAME, element,
									FpaF_LEVEL_NAME,   level,
									FpaF_END_OF_LIST) )
				{

				/* Skip unrecognized fields */
				(void) fprintf(stdout, "    Skipping non-FPA field:");
				if ( NotNull(fdesc.sdef) && !blank(fdesc.sdef->name) )
					(void) fprintf(stdout, " %s", fdesc.sdef->name);
				else
					(void) fprintf(stdout, " (%s)", model);
				if ( NotNull(fdesc.subdef) && !blank(fdesc.subdef->name) )
					(void) fprintf(stdout, ":%s", fdesc.subdef->name);
				if ( NotNull(fdesc.rtime) && !blank(fdesc.rtime) )
					(void) fprintf(stdout, " %s", fdesc.rtime);
				else
					(void) fprintf(stdout, " (%s)", xrtime);
				if ( minutes_rqd )
					(void) fprintf(stdout, " T%s",
											hour_minute_string(0, mplus));
				else
					(void) fprintf(stdout, " T%+d", mplus/60);
				if ( NotNull(fdesc.edef) && !blank(fdesc.edef->name) )
					(void) fprintf(stdout, " %s", fdesc.edef->name);
				else
					(void) fprintf(stdout, " (%s)", element);
				if ( NotNull(fdesc.ldef) && !blank(fdesc.ldef->name) )
					(void) fprintf(stdout, " %s", fdesc.ldef->name);
				else
					(void) fprintf(stdout, " (%s)", level);
				(void) fprintf(stdout, "\n");
				continue;
				}

			/* Check for daily fields ... which should not be in GRIB files! */
			if ( fdesc.edef->elem_tdep->time_dep == FpaC_DAILY )
				{

				/* Skip daily fields */
				(void) fprintf(stdout, "    Skipping Daily field:");
				if ( NotNull(fdesc.sdef) && !blank(fdesc.sdef->name) )
					(void) fprintf(stdout, " %s", fdesc.sdef->name);
				else
					(void) fprintf(stdout, " (%s)", model);
				if ( NotNull(fdesc.subdef) && !blank(fdesc.subdef->name) )
					(void) fprintf(stdout, ":%s", fdesc.subdef->name);
				if ( NotNull(fdesc.rtime) && !blank(fdesc.rtime) )
					(void) fprintf(stdout, " %s", fdesc.rtime);
				else
					(void) fprintf(stdout, " (%s)", xrtime);
				if ( minutes_rqd )
					(void) fprintf(stdout, " T%s",
											hour_minute_string(0, mplus));
				else
					(void) fprintf(stdout, " T%+d", mplus/60);
				if ( NotNull(fdesc.edef) && !blank(fdesc.edef->name) )
					(void) fprintf(stdout, " %s", fdesc.edef->name);
				else
					(void) fprintf(stdout, " (%s)", element);
				if ( NotNull(fdesc.ldef) && !blank(fdesc.ldef->name) )
					(void) fprintf(stdout, " %s", fdesc.ldef->name);
				else
					(void) fprintf(stdout, " (%s)", level);
				(void) fprintf(stdout, "\n");
				continue;
				}

			/* Prepare data directory for GRIB field */
			if ( blank(prepare_source_directory(&fdesc)) )
				{

				/* Skip fields if directory cannot be created */
				(void) fprintf(stdout, "    Cannot create directory for:");
				if ( NotNull(fdesc.sdef) && !blank(fdesc.sdef->name) )
					(void) fprintf(stdout, " %s", fdesc.sdef->name);
				else
					(void) fprintf(stdout, " (%s)", model);
				if ( NotNull(fdesc.subdef) && !blank(fdesc.subdef->name) )
					(void) fprintf(stdout, ":%s", fdesc.subdef->name);
				if ( NotNull(fdesc.rtime) && !blank(fdesc.rtime) )
					(void) fprintf(stdout, " %s", fdesc.rtime);
				else
					(void) fprintf(stdout, " (%s)", xrtime);
				(void) fprintf(stdout, "\n");
				continue;
				}

			/* Check that metafile name can be created for field */
			if ( blank(construct_meta_filename(&fdesc))
					&& blank(build_meta_filename(&fdesc)) )
				{

				/* Skip fields if metafile name cannot be created */
				(void) fprintf(stdout, "    Cannot create metafile name for:");
				if ( NotNull(fdesc.sdef) && !blank(fdesc.sdef->name) )
					(void) fprintf(stdout, " %s", fdesc.sdef->name);
				else
					(void) fprintf(stdout, " (%s)", model);
				if ( NotNull(fdesc.subdef) && !blank(fdesc.subdef->name) )
					(void) fprintf(stdout, ":%s", fdesc.subdef->name);
				if ( NotNull(fdesc.rtime) && !blank(fdesc.rtime) )
					(void) fprintf(stdout, " %s", fdesc.rtime);
				else
					(void) fprintf(stdout, " (%s)", xrtime);
				if ( minutes_rqd )
					(void) fprintf(stdout, " T%s",
											hour_minute_string(0, mplus));
				else
					(void) fprintf(stdout, " T%+d", mplus/60);
				if ( NotNull(fdesc.edef) && !blank(fdesc.edef->name) )
					(void) fprintf(stdout, " %s", fdesc.edef->name);
				else
					(void) fprintf(stdout, " (%s)", element);
				if ( NotNull(fdesc.ldef) && !blank(fdesc.ldef->name) )
					(void) fprintf(stdout, " %s", fdesc.ldef->name);
				else
					(void) fprintf(stdout, " (%s)", level);
				(void) fprintf(stdout, "\n");
				continue;
				}

			/* Check for unwanted fields */
			if ( skip_grib_field_orig(fdesc.edef->name, fdesc.ldef->name) )
				{

				/* Skip unwanted fields */
				(void) fprintf(stdout, "    Skipping field:");
				if ( NotNull(fdesc.sdef) && !blank(fdesc.sdef->name) )
					(void) fprintf(stdout, " %s", fdesc.sdef->name);
				else
					(void) fprintf(stdout, " (%s)", model);
				if ( NotNull(fdesc.subdef) && !blank(fdesc.subdef->name) )
					(void) fprintf(stdout, ":%s", fdesc.subdef->name);
				if ( NotNull(fdesc.rtime) && !blank(fdesc.rtime) )
					(void) fprintf(stdout, " %s", fdesc.rtime);
				else
					(void) fprintf(stdout, " (%s)", xrtime);
				if ( minutes_rqd )
					(void) fprintf(stdout, " T%s",
											hour_minute_string(0, mplus));
				else
					(void) fprintf(stdout, " T%+d", mplus/60);
				if ( NotNull(fdesc.edef) && !blank(fdesc.edef->name) )
					(void) fprintf(stdout, " %s", fdesc.edef->name);
				else
					(void) fprintf(stdout, " (%s)", element);
				if ( NotNull(fdesc.ldef) && !blank(fdesc.ldef->name) )
					(void) fprintf(stdout, " %s", fdesc.ldef->name);
				else
					(void) fprintf(stdout, " (%s)", level);
				(void) fprintf(stdout, "\n");
				continue;
				}

			(void) fprintf(stdout, "    Extracting field:");
			if ( NotNull(fdesc.sdef) && !blank(fdesc.sdef->name) )
				(void) fprintf(stdout, " %s", fdesc.sdef->name);
			else
				(void) fprintf(stdout, " (%s)", model);
			if ( NotNull(fdesc.subdef) && !blank(fdesc.subdef->name) )
				(void) fprintf(stdout, ":%s", fdesc.subdef->name);
			if ( NotNull(fdesc.rtime) && !blank(fdesc.rtime) )
				(void) fprintf(stdout, " %s", fdesc.rtime);
			else
				(void) fprintf(stdout, " (%s)", xrtime);
			if ( minutes_rqd )
				(void) fprintf(stdout, " T%s", hour_minute_string(0, mplus));
			else
				(void) fprintf(stdout, " T%+d", mplus/60);
			if ( NotNull(fdesc.edef) && !blank(fdesc.edef->name) )
				(void) fprintf(stdout, " %s", fdesc.edef->name);
			else
				(void) fprintf(stdout, " (%s)", element);
			if ( NotNull(fdesc.ldef) && !blank(fdesc.ldef->name) )
				(void) fprintf(stdout, " %s", fdesc.ldef->name);
			else
				(void) fprintf(stdout, " (%s)", level);
			(void) fprintf(stdout, "\n");

			/* Extract and output metafile from GRIB field */
			if ( !process_gribfield(gribfld, &fdesc, units) ) continue;

			/* Process next field in current GRIB file */
			}

		/* Close GRIB file and process next one if Edition 0 decode */
		/* processed one or more fields                             */
		(void) close_gribfile_edition0();
		if ( nflds > 0 )
			{
			(void) fprintf(stdout, "\n%s   %d GRIB fields processed\n",
					MyLabel, nflds);
			continue;
			}

		/* Error message if no GRIB fields processed */
		(void) fprintf(stderr, "%s No fields in GRIB file \"%s\"\n",
				MyLabel, gribname);

		/* Process next GRIB file */
		}

	/* Shutdown message */
	(void) systime(&cyear, &cjday, &chour, &cmin, &csec);
	(void) mdate(&cyear, &cjday, &cmonth, &cmday);
	(void) fprintf(stdout, "\n%s Finished: %d/%.2d/%.2d %.2d:%.2d:%.2d GMT\n",
			MyLabel, cyear, cmonth, cmday, chour, cmin, csec);

	return 0;
	}

/***********************************************************************
*                                                                      *
*     p r o c e s s _ g r i b f i e l d                                *
*     p r o c e s s _ g r i b f i e l d _ x y c o m p                  *
*     p r o c e s s _ g r i b f i e l d _ d e f a u l t                *
*                                                                      *
***********************************************************************/

static	LOGICAL	process_gribfield

	(
	GRIBFIELD		*gribfld,	/* GRIBFIELD Object with decoded GRIB data */
	FLD_DESCRIPT	*fdesc,		/* pointer to field descriptor for field */
	STRING			units		/* field units label */
	)

	{

	/* Process GRIB fields with x/y components */
	if ( xy_component_field(fdesc->edef->name) )
		return process_gribfield_xycomp(gribfld, fdesc, units);

	/* Process all other types of GRIB fields */
	else
		return process_gribfield_default(gribfld, fdesc, units);
	}

static	LOGICAL	process_gribfield_xycomp

	(
	GRIBFIELD		*gribfld,	/* GRIBFIELD Object with decoded GRIB data */
	FLD_DESCRIPT	*fdesc,		/* pointer to field descriptor for field */
	STRING			units		/* field units label */
	)

	{
	int							iout, maxout;
	FpaConfigFieldStruct		*fdef;
	FpaConfigElementStruct		*edefout;
	COMPONENT					compin, compout;
	STRING						dir, fname;
	FLD_DESCRIPT				fdescin, fdescout;
	METAFILE					meta[2], metanew;

	/* Ensure that detailed field information has been read */
	/*  and is entered back into field descriptor           */
	fdef = get_field_info(fdesc->edef->name, fdesc->ldef->name);
	if ( IsNull(fdef) ) return FALSE;
	(void) copy_fld_descript(&fdescin, fdesc);
	(void) set_fld_descript(&fdescin,
								FpaF_ELEMENT, fdef->element,
								FpaF_LEVEL,   fdef->level,
								FpaF_END_OF_LIST);

	/* Set parameters for processing by x/y components */
	maxout = fdef->element->elem_detail->components->ncomp;
	compin = which_components(fdef->element->name,
								NullStringPtr, NullPtr(COMPONENT *));

	/* Loop to process x/y components of field */
	for ( iout=0; iout<maxout; iout++ )
		{

		/* Set output element and component type */
		edefout = fdef->element->elem_detail->components->comp_edefs[iout];
		compout = fdef->element->elem_detail->components->comp_types[iout];

		/* Reset field descriptor for output */
		(void) copy_fld_descript(&fdescout, &fdescin);
		if ( !set_fld_descript(&fdescout, FpaF_ELEMENT, edefout,
											FpaF_END_OF_LIST) )
			{
			(void) fprintf(stderr, "       Unable to reset component!\n");
			return FALSE;
			}

		/* Convert GRIB field to component metafile */
		meta[0] = gribfield_to_metafile_by_comp(gribfld, &fdescout, units,
						compin, compout);

		/* Error message if no metafile could be created */
		if ( !meta[0] )
			{
			(void) fprintf(stderr, "       Unable to extract target field!\n");
			return FALSE;
			}

		/* Set file lock in base directory while processing field */
		dir = source_directory_by_name(fdescin.sdef->name, fdescin.subdef->name,
				FpaCblank);
		(void) strcpy(LockDir,   dir);
		(void) strcpy(LockVtime, fdescin.vtime);
		if ( !set_file_lock(LockDir, LockVtime) )
			{
			(void) fprintf(stderr, "       Unable to establish file lock!\n");
			meta[0] = destroy_metafile(meta[0]);
			return FALSE;
			}
		Locked = TRUE;

		/* Construct new format (or old format) metafile name */
		fname = construct_meta_filename(&fdescout);
		if ( blank(fname) ) fname = build_meta_filename(&fdescout);

		/* Read an existing metafile from the directory */
		meta[1] = read_metafile(fname, &fdescout.mproj);

		/* Now merge the new metafile with the existing metafile */
		/*  ... and output the result to the output directory    */
		metanew = merge_metafiles(2, meta);
		(void) write_metafile(fname, metanew, MaxDigits);

		/* Determine coverage for extracted field */
		(void) fprintf(stdout, "       Current coverage: %.0f%%\n",
				coverage_mf_source_proj(metanew));

		/* Free space used by METAFILE Objects */
		meta[0] = destroy_metafile(meta[0]);
		meta[1] = destroy_metafile(meta[1]);
		metanew = destroy_metafile(metanew);

		/* Remove the current lock in the base directory */
		(void) release_file_lock(LockDir, LockVtime);
		Locked = FALSE;
		}

	/* Return when all components have been processed */
	return TRUE;
	}

static	LOGICAL	process_gribfield_default

	(
	GRIBFIELD		*gribfld,	/* GRIBFIELD Object with decoded GRIB data */
	FLD_DESCRIPT	*fdesc,		/* pointer to field descriptor for field */
	STRING			units		/* field units label */
	)

	{
	STRING					dir, fname;
	FLD_DESCRIPT			fdescout;
	METAFILE				meta[2], metanew;

	/* Initialize field descriptor for output */
	(void) copy_fld_descript(&fdescout, fdesc);

	/* Convert GRIB field to metafile */
	meta[0] = gribfield_to_metafile(gribfld, &fdescout, units);

	/* Error message if no metafile could be created */
	if ( !meta[0] )
		{
		(void) fprintf(stderr, "       Unable to extract target field!\n");
		return FALSE;
		}

	/* Set file lock in base directory while processing field */
	dir = source_directory_by_name(fdesc->sdef->name, fdesc->subdef->name,
			FpaCblank);
	(void) strcpy(LockDir,   dir);
	(void) strcpy(LockVtime, fdesc->vtime);
	if ( !set_file_lock(LockDir, LockVtime) )
		{
		(void) fprintf(stderr, "       Unable to establish file lock!\n");
		meta[0] = destroy_metafile(meta[0]);
		return FALSE;
		}
	Locked = TRUE;

	/* Construct new format (or old format) metafile name */
	fname = construct_meta_filename(&fdescout);
	if ( blank(fname) ) fname = build_meta_filename(&fdescout);

	/* Read an existing metafile from the directory */
	meta[1] = read_metafile(fname, &fdescout.mproj);

	/* Now merge the new metafile with the existing metafile */
	/*  ... and output the result to the output directory    */
	metanew = merge_metafiles(2, meta);
	(void) write_metafile(fname, metanew, MaxDigits);

	/* Determine coverage for extracted field */
	(void) fprintf(stdout, "       Current coverage: %.0f%%\n",
			coverage_mf_source_proj(metanew));

	/* Free space used by METAFILE Objects */
	meta[0] = destroy_metafile(meta[0]);
	meta[1] = destroy_metafile(meta[1]);
	metanew = destroy_metafile(metanew);

	/* Remove the current lock in the base directory */
	(void) release_file_lock(LockDir, LockVtime);
	Locked = FALSE;

	/* Return when all components have been processed */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     e r r o r _ t r a p                                              *
*                                                                      *
***********************************************************************/

static	void	error_trap

	(
	int		sig
	)

	{
	char	*sname;

	/* Ignore all further signals */
	(void) set_error_trap(SIG_IGN);
	(void) signal(sig, SIG_IGN);

	/* Get the signal name if possible */
	sname = signal_name(sig);

	/* Provide a message */
	(void) fprintf(stdout, "%s !!! %s Has Occurred - Terminating\n",
			MyLabel, sname);

	/* Die gracefully */
	if ( Locked )
		{
		(void) fprintf(stdout, "    Removing lock in Guidance Directory\n");
		(void) release_file_lock(LockDir, LockVtime);
		}
	(void) exit(1);
	}
