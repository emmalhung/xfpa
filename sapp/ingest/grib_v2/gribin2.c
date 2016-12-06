/*******************************************************************************/
/** @file	gribin.c
 * 	GRIB Data Ingest Program
 *
 * 	This program reads the given GRIB file and separates the individual
 * 	fields into corresponding metafiles. This is used to ingest guidance
 * 	fields recieved from CMC or other sources.
 *******************************************************************************/
/***********************************************************************
*                                                                      *
*     g r i b i n . c                                                  *
*                                                                      *
*     GRIB Data Ingest Program                                         *
*                                                                      *
*     This program reads the given GRIB file and separates the         *
*     individual fields into corresponding metafiles.  This is used    *
*     to ingest guidance fields received from CMC or other sources.    *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
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

/* We need FPA definitions */
#include <fpa.h>

/* #include "gribs.h" */
#include "rgrib.h"
#include "gribmeta.h"
#include "gribdata.h"

#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>

#undef DEBUG

#ifdef DEBUG_DECODE
	static int	DebugMode = TRUE;
#else
	static int	DebugMode = FALSE;
#endif /* DEBUG_DECODE */

#define dprintf (!DebugMode)? (void) 0: (void) fprintf

/* Trap for error situations */
static	void	error_trap(int);

/* Internal static functions to extract and output metafiles from GRIB data */
static	LOGICAL	process_gribfield(DECODEDFIELD *, FLD_DESCRIPT *, STRING);
static	LOGICAL	process_gribfield_xycomp(DECODEDFIELD *, FLD_DESCRIPT *, STRING);
static	LOGICAL	process_gribfield_default(DECODEDFIELD *, FLD_DESCRIPT *, STRING);
static	LOGICAL	process_datafile(DECODEDFIELD *, FLD_DESCRIPT *, STRING, float, float);
static	LOGICAL	process_datafile_xycomp(DECODEDFIELD *, FLD_DESCRIPT *, STRING, float, float);
static	LOGICAL	process_datafile_default(DECODEDFIELD *, FLD_DESCRIPT *, STRING, float, float);
int				determine_edition_number ( STRING );
char *	print_field_detail ( FLD_DESCRIPT *, STRING, STRING, STRING, STRING, STRING, STRING, LOGICAL);

/* Default message strings */
static	const	STRING	MyTitle = "FPA GRIB 2 Ingest";
static			char	MyLabel[MAX_BCHRS];
static			char	MyPid[MAX_BCHRS];

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
	int				nslist, iarg, nflds;
	LOGICAL			minutes_rqd;
	STRING			sfile, *slist, dir;
	char			home[MAX_BCHRS], work[MAX_BCHRS], gribname[MAX_BCHRS];
	MAP_PROJ		*mproj;
	FLD_DESCRIPT	fdesc, fdescin;
	DECODEDFIELD	*gribfld;
	STRING			model, rtime, btime, etime, element, level, units;
	char			xrtime[GRIB_LABEL_LEN];
	char			xbtime[GRIB_LABEL_LEN];
	char			xetime[GRIB_LABEL_LEN];
	int				edition;
	LOGICAL			iret;

	/* Datafile scale & offset */
	float	precision, offset;

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
		(void) fprintf(stderr, "   gribin2 <setup_file> <file> [<file> ...]\n");
		return (-1);
		}

	/* Obtain a licence */
	(void) app_license("ingest");

	/* Trap all signals that would abort the process by default */
	(void) set_error_trap(error_trap);

	/* Startup message */
	(void) sprintf(MyPid, "[%d] ", getpid());
	(void) sprintf(MyLabel, "%s%s:", MyPid, MyTitle);
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

	/* Read the Ingest configuration files */
	if ( !read_complete_ingest_file() )
		{
		(void) fprintf(stderr, "%s Problem with Ingest Files\n", MyLabel);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Retrieve the target projection definition */
	mproj = get_target_map();
	if ( !mproj )
		{
		(void) fprintf(stderr,
				"%s Target map not defined in setup file \"%s\"\n",
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
	(void) init_fld_descript(&fdesc);
	if ( !set_fld_descript(&fdesc,
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

		/* find edition number */
		edition = determine_edition_number(gribname);

		/* Initialize number of decoded GRIB fields */
		nflds = 0;


		/* Open Edition 0,1, or 2 GRIB file */
		switch (edition)
			{
				case 1: iret = open_gribfile_edition1(gribname); break;
				case 2: iret = open_gribfile_edition2(gribname); break;
				default: /* If no edition number assume edition 0 */
						iret = open_gribfile_edition0(gribname);
						edition = 0;
						break;
			}
		if (!iret)
			{
			(void) fprintf(stderr, "%s Cannot access GRIB file \"%s\"\n",
				MyLabel, gribname);
			continue;
			}

		(void) fprintf(stdout, "\n%s GRIB Edition %d decode\n",
				MyLabel, edition);

		/* Edition 0,1 or 2 decode of each field in GRIB file */
		while ( 1 )
			{
			switch ( edition )
				{
				case 0: iret = next_gribfield_edition0(&gribfld); break;
				case 1: iret = next_gribfield_edition1(&gribfld); break;
				case 2: iret = next_gribfield_edition2(&gribfld); break;
				default: iret = FALSE; break;
				}
			if ( !iret ) break;	/* End of file reached */

			/* Reset number of decoded GRIB fields */
			nflds++;

			/* Get information about this gribfield */
			switch ( edition )
				{
				case 0:
					iret = gribfield_identifiers_edition0(&model, &rtime, &btime,
							&etime, &element, &level, &units);
					break;
				case 1:
					iret = gribfield_identifiers_edition1(&model, &rtime, &btime,
							&etime, &element, &level, &units);
					break;
				case 2:
					iret = gribfield_identifiers_edition2(&model, &rtime, &btime,
							&etime, &element, &level, &units);
					break;
				default: iret = FALSE; break;
				}
			if ( !iret )
				{
				(void) fprintf(stdout, "%s Skipping unrecognized field\n",
						MyPid);
				continue;
				}

			/* reset field descriptor */
			(void) copy_fld_descript(&fdescin, &fdesc);

			/* Check for unwanted datafiles */
			if ( skip_grib_datafile(edition, model, element, level) )
				{

				/* Don't turn these fields into datafiles */
				(void) fprintf(stdout, "%s Skipping datafile: %s", MyPid,
							   print_field_detail(&fdescin, model, rtime, btime, etime,
								   element, level, TRUE));
				}
			else	/* Process Field into Datafile */
				{

				/* Set the field descriptor directory info */
				/* Redirect Source if necessary */
				if ( !set_fld_descript(&fdescin,
							FpaF_SOURCE_NAME,    redirect_datafile(edition, model),
							FpaF_END_OF_LIST) )
					{
					(void) fprintf(stderr, "%s Skipping non-FPA model \"%s\"\n",
							MyPid, redirect_datafile(edition, model));
					continue;
					}

				/* Make copies of the timestamps (depending on minutes required) */
				minutes_rqd = fdescin.sdef->minutes_rqd;
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
				if ( !set_fld_descript(&fdescin,
										FpaF_RUN_TIME,     xrtime,
										FpaF_VALID_TIME,   xetime,
										FpaF_ELEMENT_NAME, element,
										FpaF_LEVEL_NAME,   level,
										FpaF_END_OF_LIST) )
					{
					/* Skip unrecognized fields */
					(void) fprintf(stdout, "%s Skipping non-FPA field: %s", MyPid,
								   print_field_detail(&fdescin, model, rtime, btime,
									   etime, element, level, FALSE));
					continue;
					}

				/* Check for daily fields ... which should not be in GRIB files! */
				if ( fdescin.edef->elem_tdep->time_dep == FpaC_DAILY )
					{

					/* Skip daily fields */
					(void) fprintf(stdout, "%s Skipping Daily field: %s", MyPid,
								   print_field_detail(&fdescin, model, rtime, btime,
									   etime, element, level, FALSE));
					continue;
					}

				/* Prepare data directory for GRIB field */
				if ( blank(prepare_source_directory(&fdescin)) )
					{

					/* Skip fields if directory cannot be created */
					(void) fprintf(stdout, "%s Cannot create directory for: %s",
							MyPid, print_field_detail(&fdescin, model, rtime, btime,
								etime, element, level, FALSE));
					continue;
					}

				/* Check that metafile name can be created for field */
				if ( blank(construct_meta_filename(&fdescin))
						&& blank(build_meta_filename(&fdescin)) )
					{

					/* Skip fields if metafile name cannot be created */
					(void) fprintf(stdout,
							"%s Cannot create metafile name for: %s",
							MyPid, print_field_detail(&fdescin, model, rtime, btime,
								etime, element, level, FALSE));
					continue;
					}

				/* There were no objections so go ahead and process datafile */
				(void) fprintf(stdout, "%s Extracting datafile: %s",
							   MyPid, print_field_detail(&fdescin, model, rtime, btime,
								   etime, element, level, TRUE));

				/* Lookup precision and scale factor from Ingest config */
				rescale_datafile(edition, element, level, &precision, &offset);
				/* Extract and output datafile from GRIB field */
				if ( !process_datafile(gribfld, &fdescin, units, precision, offset) )
					{
					/* Error processing field */
					(void) fprintf(stdout,
							"%s Error while processing gribfield to datafile: %s",
							MyPid, print_field_detail(&fdescin, model, rtime, btime,
								etime, element, level, FALSE));
					continue;
					}
				}

			/* reset field descriptor */
			(void) copy_fld_descript(&fdescin, &fdesc);

			/* Check for unwanted fields */
			if ( skip_grib_field(edition, model, element, level) )
				{

				/* Skip unwanted fields */
				(void) fprintf(stdout, "%s Skipping field: %s",
							   MyPid, print_field_detail(&fdescin, model, rtime, btime,
								   etime, element, level, TRUE));
				}
			else	/* Process Field into Metafile */
				{
				/* Set the field descriptor directory info */
				/* Redirect Source if necessary */
				if ( !set_fld_descript(&fdescin,
										FpaF_SOURCE_NAME,    redirect_field(edition, model),
										FpaF_END_OF_LIST) )
					{
					(void) fprintf(stderr, "%s Skipping non-FPA model \"%s\"\n",
							MyPid, redirect_field(edition, model));
					continue;
					}


				/* Make copies of the timestamps (depending on minutes required) */
				minutes_rqd = fdescin.sdef->minutes_rqd;
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
				if ( !set_fld_descript(&fdescin,
										FpaF_RUN_TIME,     xrtime,
										FpaF_VALID_TIME,   xetime,
										FpaF_ELEMENT_NAME, element,
										FpaF_LEVEL_NAME,   level,
										FpaF_END_OF_LIST) )
					{
					/* Skip unrecognized fields */
					(void) fprintf(stdout, "%s Skipping non-FPA field: %s", MyPid,
								   print_field_detail(&fdescin, model, rtime, btime,
									   etime, element, level, FALSE));
					continue;
					}

				/* Check for daily fields ... which should not be in GRIB files! */
				if ( fdescin.edef->elem_tdep->time_dep == FpaC_DAILY )
					{

					/* Skip daily fields */
					(void) fprintf(stdout, "%s Skipping Daily field: %s", MyPid,
								   print_field_detail(&fdescin, model, rtime, btime,
									   etime, element, level, FALSE));
					continue;
					}

				/* Prepare data directory for GRIB field */
				if ( blank(prepare_source_directory(&fdescin)) )
					{

					/* Skip fields if directory cannot be created */
					(void) fprintf(stdout, "%s Cannot create directory for: %s",
							MyPid, print_field_detail(&fdescin, model, rtime, btime,
								etime, element, level, FALSE));
					continue;
					}

				/* Check that metafile name can be created for field */
				if ( blank(construct_meta_filename(&fdescin))
						&& blank(build_meta_filename(&fdescin)) )
					{

					/* Skip fields if metafile name cannot be created */
					(void) fprintf(stdout,
							"%s Cannot create datafile name for: %s",
							MyPid, print_field_detail(&fdescin, model, rtime, btime,
								etime, element, level, FALSE));
					continue;
					}

				/* There were no objections so go ahead and process field */
				(void) fprintf(stdout, "%s Extracting field:  %s", MyPid,
							   print_field_detail(&fdescin, model, rtime, btime, etime,
								   element, level, FALSE));

				/* Extract and output metafile from GRIB field */
				if ( !process_gribfield(gribfld, &fdescin, units) )
					{
					/* Error processing field */
					(void) fprintf(stdout,
							"%s Error while processing gribfield to Metafile: %s",
							MyPid, print_field_detail(&fdescin, model, rtime, btime,
								etime, element, level, FALSE));
					continue;
					}
				}

			/* Process next field in current GRIB file */
			}

		/* Close GRIB file and process next one */
		switch ( edition )
			{
			case 0: (void) close_gribfile_edition0();
			case 1: (void) close_gribfile_edition1();
			case 2: (void) close_gribfile_edition2();
			}
		if ( nflds > 0 )
			{
			(void) fprintf(stdout, "\n%s   %d GRIB fields processed\n",
					MyLabel, nflds);
			continue;
			}

		/* Error message if no GRIB fields processed */
		(void) fprintf(stderr, "%s No fields in GRIB file \"%s\"\n",
				MyLabel, gribname);

		}	/* Process next GRIB file */

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

/*******************************************************************************/
/** Choose processing method depending on whether a field has x/y components
 * or not.
 *
 * @param[in]	*gribfld	DECODEDFIELD object with decoded GRIB data
 * @param[in]	*fdesc		pointer to field descriptor for field
 * @param[in]	units		field units label.
 *******************************************************************************/
static	LOGICAL	process_gribfield

	(
	DECODEDFIELD	*gribfld,	/* GRIBFIELD Object with decoded GRIB data */
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

/*******************************************************************************/
/** Choose processing method depending on whether a field has x/y components
 * or not. For now you cannot turn an x/y component field into a datafile.
 *
 * @param[in]	*gribfld	DECODEDFIELD object with decoded GRIB data
 * @param[in]	*fdesc		pointer to field descriptor for field
 * @param[in]	units		field units label.
 * @param[in]	precision	amount to scale output values by.
 * @param[in]	offset		amount to offset output values by.
 *******************************************************************************/
static	LOGICAL	process_datafile

	(
	DECODEDFIELD	*gribfld,	/* GRIBFIELD Object with decoded GRIB data */
	FLD_DESCRIPT	*fdesc,		/* pointer to field descriptor for field */
	STRING			units,		/* field units label */
	float			precision,
	float			offset
	)

	{

	/* Process GRIB fields with x/y components */
	if ( xy_component_field(fdesc->edef->name) )
		return process_datafile_xycomp(gribfld, fdesc, units, precision, offset);

	/* Process all other types of GRIB fields */
	else
		return process_datafile_default(gribfld, fdesc, units, precision, offset);
	}

/*******************************************************************************/
/**  Process gribfield that has both x and y components.
 *
 * @param[in]	*gribfld	DECODEDFIELD object with decoded GRIB data
 * @param[in]	*fdesc		pointer to field descriptor for field
 * @param[in]	units		field units label.
 *******************************************************************************/
static	LOGICAL	process_gribfield_xycomp

	(
	DECODEDFIELD	*gribfld,	/* GRIBFIELD Object with decoded GRIB data */
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
			(void) fprintf(stderr, "%s   Unable to reset component!\n", MyPid);
			return FALSE;
			}

		/* Convert GRIB field to component metafile */
		meta[0] = gribfield_to_metafile_by_comp(gribfld, &fdescout, units,
						compin, compout);

		/* Error message if no metafile could be created */
		if ( !meta[0] )
			{
			(void) fprintf(stderr, "%s   Unable to extract target field!\n",
					MyPid);
			return FALSE;
			}

		/* Set file lock in base directory while processing field */
		dir = source_directory_by_name(fdescin.sdef->name, fdescin.subdef->name,
				FpaCblank);
		(void) strcpy(LockDir,   dir);
		(void) strcpy(LockVtime, fdescin.vtime);
		if ( !set_file_lock(LockDir, LockVtime) )
			{
			(void) fprintf(stderr, "%s   Unable to establish file lock!\n",
					MyPid);
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
		(void) fprintf(stdout, "%s   Current coverage: %.0f%%\n",
				MyPid, coverage_mf_source_proj(metanew));

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

/*******************************************************************************/
/**  Process datafile that has both x and y components.
 *
 * 	At present we don't do this
 *
 * @param[in]	*gribfld	DECODEDFIELD object with decoded GRIB data
 * @param[in]	*fdesc		pointer to field descriptor for field
 * @param[in]	units		field units label.
 * @param[in]	precision	amount to scale output values by.
 * @param[in]	offset		amount to offset output values by.
 *******************************************************************************/
static	LOGICAL	process_datafile_xycomp

	(
	DECODEDFIELD	*gribfld,	/* GRIBFIELD Object with decoded GRIB data */
	FLD_DESCRIPT	*fdesc,		/* pointer to field descriptor for field */
	STRING			units,		/* field units label */
	float			precision,
	float			offset
	)

	{

	(void) pr_error("[process_datafile_xycom]", "Operation not supported\n");
	/* Return when all components have been processed */
	return TRUE;
	}

/*******************************************************************************/
/**  Process single component gribfield
 *
 * @param[in]	*gribfld	DECODEDFIELD object with decoded GRIB data
 * @param[in]	*fdesc		pointer to field descriptor for field
 * @param[in]	units		field units label.
 *******************************************************************************/
static	LOGICAL	process_gribfield_default

	(
	DECODEDFIELD	*gribfld,	/* GRIBFIELD Object with decoded GRIB data */
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
		(void) fprintf(stderr, "%s   Unable to extract target field!\n", MyPid);
		return FALSE;
		}

	/* Set file lock in base directory while processing field */
	dir = source_directory_by_name(fdesc->sdef->name, fdesc->subdef->name,
			FpaCblank);
	(void) strcpy(LockDir,   dir);
	(void) strcpy(LockVtime, fdesc->vtime);
	if ( !set_file_lock(LockDir, LockVtime) )
		{
		(void) fprintf(stderr, "%s   Unable to establish file lock!\n", MyPid);
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
	(void) fprintf(stdout, "%s   Current coverage: %.0f%%\n",
			MyPid, coverage_mf_source_proj(metanew));

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

/*******************************************************************************/
/**  Process single component datafile
 *
 * @param[in]	*gribfld	DECODEDFIELD object with decoded GRIB data
 * @param[in]	*fdesc		pointer to field descriptor for field
 * @param[in]	units		field units label.
 * @param[in]	precision	amount to scale output values by.
 * @param[in]	offset		amount to offset output values by.
 *******************************************************************************/
static	LOGICAL	process_datafile_default

	(
	DECODEDFIELD	*gribfld,	/* GRIBFIELD Object with decoded GRIB data */
	FLD_DESCRIPT	*fdesc,		/* pointer to field descriptor for field */
	STRING			units,		/* field units label */
	float			precision,
	float			offset
	)

	{
	STRING					dir;

	/* Redirect source if requested */
	/* Set file lock in base directory while processing field */
	dir = source_directory_by_name(fdesc->sdef->name, fdesc->subdef->name,
			FpaCblank);
	(void) strcpy(LockDir,   dir);
	(void) strcpy(LockVtime, fdesc->vtime);
	if ( !set_file_lock(LockDir, LockVtime) )
		{
		(void) fprintf(stderr, "%s   Unable to establish file lock!\n",
				MyPid);
		return FALSE;
		}
	Locked = TRUE;

	/* Lookup */
	/* Format and output datafile */
	(void) gribfield_to_data(gribfld, fdesc, units, precision, offset);

	/* Remove the current lock in the base directory */
	(void) release_file_lock(LockDir, LockVtime);
	Locked = FALSE;

	return TRUE;
	}

/**************************************************
* d e t e r m i n e _ e d i t i o n _ n u m b e r *
***************************************************/
/*******************************************************************************/
/** Opens file and determines the grib edition.
 *
 * At the conclusion of this function the given file is closed.
 *
 * @param[in]	name	GRIB file name.
 *******************************************************************************/
int	determine_edition_number
	(
	STRING	name
	)
	{
	const char	*HEADER = "GRIB";
	const int	HEADER_LEN = 4;
	char	ip_char, *required_char, edition;
	int		nb_found=0, totalchar=0;

	FILE *file	= NullPtr(FILE *);

	/* Do nothing if GRIB file name not given */
	if (blank(name))
		{
		dprintf(stderr, "[determine_edition_number] GRIB file name not given\n");
		return -1;
		}
	/* See if the GRIB file exists */
	if (!find_file(name))
		{
		dprintf(stderr, "[determine_edition_number] GRIB file not found: %s\n", name);
		return -1;
		}
	/* Open the GRIB file */
	file = fopen(name, "r");
	if ( IsNull(file) )
		{
		dprintf(stderr, "[determine_edition_number] GRIB file unreadable: %s\n", name);
		return -1;
		}

	required_char = HEADER;
	while(nb_found < HEADER_LEN)
		{
		ip_char = fgetc(file);
		if ( feof(file) > 0 )
			{
			if ( totalchar <= 0 )
				dprintf(stderr, "\n End of GRIB file\n");
			else
				(void) fprintf(stderr,"\n End-of-file in Section 0 header\n");
			return -1;
			}
		if ( ferror(file) )
			{
			(void) fprintf(stderr, "\n Error in Section 0 hearder\n");
			return -1;
			}
		if ( ++totalchar > 80 )
			{
			totalchar = 1;
			dprintf(stderr, "\n");
			}
		dprintf(stderr, "%c", ip_char);

		if ( ip_char == *required_char )
			{
			++nb_found;
			++required_char;
			}
		else
			{
			nb_found = 0;
			required_char = HEADER;
			if (ip_char == *required_char )
				{
					++nb_found;
					++required_char;
				}
			}
		}
		fget3c(file);			/* Skip Octet #5-7 */
		edition = fgetc(file);	/* Return Octet #8 as edition# */
		(void) fclose (file);			/* Close the file */
		return edition;
	}

/*******************************************************************************/
/** Opens file and determines the grib edition.
 *
 * At the conclusion of this function the given file is closed.
 *
 * @param[in]	*fdesc	field descriptor causing the problem.
 * @param[in]	model
 * @param[in]	rtime
 * @param[in]	btime
 * @param[in]	etime
 * @param[in]	element
 * @param[in]	level
 * @param[in]	skip
 **** ***************************************************************************/
char	*print_field_detail
	(
	FLD_DESCRIPT	*fdesc,		/* pointer to field descriptor for field */
	STRING			model,
	STRING			rtime,
	STRING			btime,
	STRING			etime,
	STRING			element,
	STRING			level,
	LOGICAL			skip
	)
	{
	int				mplus;
	LOGICAL			ok, minutes_rqd=FALSE;
	char			xrtime[GRIB_LABEL_LEN];
	char			xbtime[GRIB_LABEL_LEN];
	char			xetime[GRIB_LABEL_LEN];
	static char		bufout[MAX_BCHRS];
	char			buftmp[MAX_BCHRS];

	safe_strcpy(bufout, "");
	safe_strcpy(buftmp, "");

	/* Set the prog time from run time and end valid time */
	mplus = calc_prog_time_minutes(rtime, etime, &ok);

	/* Make copies of the timestamps (depending on minutes required) */
	if ( NotNull(fdesc->sdef) )
		minutes_rqd = fdesc->sdef->minutes_rqd;
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
	/* Print field detail */
	if ( NotNull(fdesc->sdef) && !blank(fdesc->sdef->name) )
		{
		(void) safe_strcat(bufout, fdesc->sdef->name);
		}
	else if (skip)
		{
		(void) safe_strcat(bufout, model);
		}
	else
		{
		(void) sprintf(buftmp, "(%s)", model);
		(void) safe_strcat(bufout, buftmp);
		}

	if ( NotNull(fdesc->subdef) && !blank(fdesc->subdef->name) )
		{
		(void) sprintf(buftmp, ":%s", fdesc->subdef->name);
		(void) safe_strcat(bufout, buftmp);
		}

	if ( NotNull(fdesc->rtime) && !blank(fdesc->rtime) )
		{
		(void) sprintf(buftmp, " %s", fdesc->rtime);
		(void) safe_strcat(bufout, buftmp);
		}
	else if (skip)
		{
		(void) sprintf(buftmp, " %s", xrtime);
		(void) safe_strcat(bufout, buftmp);
		}
	else
		{
		(void) sprintf(buftmp, " (%s)", xrtime);
		(void) safe_strcat(bufout, buftmp);
		}

	if ( minutes_rqd )
		{
		(void) sprintf(buftmp, " T%s", hour_minute_string(0, mplus));
		(void) safe_strcat(bufout, buftmp);
		}
	else
		{
		(void) sprintf(buftmp, " T%+d", mplus/60);
		(void) safe_strcat(bufout, buftmp);
		}

	if ( NotNull(fdesc->edef) && !blank(fdesc->edef->name) )
		{
		(void) sprintf(buftmp, " %s", fdesc->edef->name);
		(void) safe_strcat(bufout, buftmp);
		}
	else if (skip)
		{
		(void) sprintf(buftmp, " %s", element);
		(void) safe_strcat(bufout, buftmp);
		}
	else
		{
		(void) sprintf(buftmp, " (%s)", element);
		(void) safe_strcat(bufout, buftmp);
		}

	if ( NotNull(fdesc->ldef) && !blank(fdesc->ldef->name) )
		{
		(void) sprintf(buftmp, " %s", fdesc->ldef->name);
		(void) safe_strcat(bufout, buftmp);
		}
	else if (skip)
		{
		(void) sprintf(buftmp, " %s", level);
		(void) safe_strcat(bufout, buftmp);
		}
	else
		{
		(void) sprintf(buftmp, " (%s)", level);
		(void) safe_strcat(bufout, buftmp);
		}

		(void) safe_strcat(bufout, "\n");
		return bufout;
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
		(void) fprintf(stdout, "%s Removing lock in Guidance Directory\n",
				MyPid);
		(void) release_file_lock(LockDir, LockVtime);
		}
	(void) exit(1);
	}
