/***********************************************************************
*                                                                      *
*     f p a c r e a t e _ c o n t . c                                  *
*                                                                      *
*     FPA Create from Contoured Fields Program                         *
*                                                                      *
*     This program extracts countours from a given continuous field    *
*     and creates an area outline from the minimum to the maximum      *
*     contour values in a range.                                       *
*                                                                      *
*     Version 8 (c) Copyright 2013 Environment Canada                  *
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

/* We need FPA library definitions */
#include <fpa.h>

#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>

static	LOGICAL	DebugMode = TRUE;

/* Trap for error situations */
static	void	error_trap(int);


/* Define structure to hold attribute tags */
typedef struct
	{
	STRING				tag;
	float				val;
	STRING				attname;
	FpaConfigUnitStruct	*attunit;
	} TAG_INFO;

/* Define storage for attribute tags */
static	TAG_INFO	*TagAtts  = NullPtr(TAG_INFO *);
static	int			NumTagAtt = 0;

/* Default smoothing factor for spline fitting */
static	const	float	DefaultSmoothing = 500;

/* Default minimum number of points for area boundaries */
static	const	int		DefaultMinPoints = 10;

/* Default units for contour values */
static	const	STRING	MKS = "MKS";


/* Default message strings */
static	const	STRING	MyTitle = "FPA Create from Contours";
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
	int								status;
	STRING							setup, amodel, subarea, rtime, units;
	float							smoothing, sres, minval, maxval;
	int								minpoints, numvts, nslist;
	STRING							*slist;
	int								cyear, cjday, cmonth, cday, chr, cmin, csec;
	MAP_PROJ						*mproj;
	int								Inumx, Inumy;
	POINT							**Apstns;
	float							Aglen, Clon;
	STRING							datadir, basedir, fname;
	FLD_DESCRIPT					fdesc, descript, descriptin;
	int								nummeta, nvt;
	STRING							*vtimes, *vlist;
	FpaConfigSourceStruct			*sdef;
	FpaConfigSourceAlliedStruct		*allied;
	FpaConfigAlliedFieldsStruct		*fields;
	FpaConfigAlliedMetafilesStruct	*metafiles;
	FpaConfigFieldStruct			*fdef;
	FpaConfigAlliedAttribStruct		*attrib_info, *attrib_meta;
	FpaConfigAlliedDefAttribStruct	*def_info;
	FpaCtimeDepTypeOption			time_dep;
	LOGICAL							usemin, usemax, attok;
	int								fkind, natt, nn, ntx, nset, ips, ipe, nh;
	double							dminval, dmaxval;
	STRING							val;
	POINT							spos;
	SET								set = NullSet;
	SURFACE							sfc;
	METAFILE						meta;
	SET								aset;
	AREA							area;
	LINE							bdry, tline;
	DIVSTAT							dstat;
	CAL								cal, acal;
	int								nlines;
	LINE							*lines;

	/* Ignore hangup, interrupt and quit signals so we can survive after */
	/* logging off */
	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	status = setvbuf(stdout, NullString, _IOLBF, 0);
	status = setvbuf(stderr, NullString, _IOLBF, 0);

	/* Set debug mode (if requested) */
	if (getenv("FPA_DEBUG_CREATE") != NULL) DebugMode = TRUE;
	if ( DebugMode ) (void) pr_control(NULL, 5, 1);
	else             (void) pr_control(NULL, 1, 1);

	/* Validate run string parameters */
	if ( argc < 10 )
		{
		(void) fprintf(stderr, "Usage:\n");
		(void) fprintf(stderr, "   fpacreate_cont  <setup_file>");
		(void) fprintf(stderr, "  <allied_model_name>  <subarea_name>");
		(void) fprintf(stderr, "  <run_time>\n");
		(void) fprintf(stderr, "                    <smoothing>  <min_points>");
		(void) fprintf(stderr, "  <min_value>  <max_value>  <units>\n");
		(void) fprintf(stderr, "                     (<valid_times>)\n\n");
		(void) fprintf(stderr, "   Note that  <smoothing>  produces smoother");
		(void) fprintf(stderr, " outlines with larger values (default 500)\n");
		(void) fprintf(stderr, "   Note that  <min_points>  sets the minimum");
		(void) fprintf(stderr, " number of points in outlines (default 10)\n");
		(void) fprintf(stderr, "   Note that  <min_value>  <max_value>  <units>");
		(void) fprintf(stderr, " set the contour range for outlines\n");
		(void) fprintf(stderr, "   Note that  <valid_times>  variables are");
		(void) fprintf(stderr, " optional with format  xx  or  rr/hh(L)\n");
		(void) fprintf(stderr, "     where  xx  is hours after current");
		(void) fprintf(stderr, " time,  rr  is day (0 for today,\n");
		(void) fprintf(stderr, "       1 for tomorrow, ...), and  hh(L)");
		(void) fprintf(stderr, "  is GMT (or local) hour of day (0-23)\n");
		return (-1);
		}

	/* Obtain a licence */
	(void) app_license("model.fpacreate");

	/* Trap all signals that would abort the process by default */
	(void) set_error_trap(error_trap);

	/* Set run string parameters */
	setup         = argv[1];
	amodel        = argv[2];
	subarea       = argv[3];
	rtime         = argv[4];
	if ( !same(argv[5], "-") ) smoothing = (float) atof(argv[5]);
	else                       smoothing = DefaultSmoothing;
	if ( !same(argv[6], "-") ) minpoints = (int) atoi(argv[6]);
	else                       minpoints = DefaultMinPoints;
	if ( !same(argv[7], "-") )
		{
		minval = (float) atof(argv[7]);
		usemin = TRUE;
		}
	else 
		{
		minval    = -FPA_FLT_MAX;
		usemin = FALSE;
		}
	if ( !same(argv[8], "-") )
		{
		maxval = (float) atof(argv[8]);
		usemax = TRUE;
		}
	else
		{
		maxval = FPA_FLT_MAX;
		usemax = FALSE;
		}
	if ( !same(argv[9], "-") ) units     = argv[9];
	else                       units     = MKS;

	/* Set number of <valid_time> variables passed */
	numvts = 0;
	if ( argc > 10 ) numvts = argc - 10;

	/* Reset smoothing if too small! */
	if ( smoothing < 10.0 ) smoothing = 10.0;

	/* Startup message */
	(void) sprintf(MyLabel, "[%d] %s:", getpid(), MyTitle);
	(void) systime(&cyear, &cjday, &chr, &cmin, &csec);
	(void) mdate(&cyear, &cjday, &cmonth, &cday);
	(void) fprintf(stdout, "\n%s Beginning: %d/%.2d/%.2d %.2d:%.2d:%.2d GMT\n",
			MyLabel, cyear, cmonth, cday, chr, cmin, csec);
	(void) fprintf(stdout, "%s      for Allied Model \"%s %s\" at \"%s\"\n",
			MyLabel, amodel, subarea, rtime);
	(void) fprintf(stdout, "%s      with smoothing: %f\n", MyLabel, smoothing);
	if ( usemin && usemax )
		(void) fprintf(stdout, "%s      from contour values from: %f to: %f %s\n",
				MyLabel, minval, maxval, units);
	else if ( usemin )
		(void) fprintf(stdout, "%s      from contour values above: %f %s\n",
				MyLabel, minval, units);
	else if ( usemax )
		(void) fprintf(stdout, "%s      from contour values below: %f %s\n",
				MyLabel, maxval, units);

	/* Read the setup file */
	/* This moves to standard FPA directory */
	nslist = setup_files(setup, &slist);
	if ( !define_setup(nslist, slist) )
		{
		(void) fprintf(stderr,
				"%s Problem with setup file \"%s\"\n", MyLabel, setup);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Retrieve the target projection definition */
	mproj = get_target_map();
	if ( IsNull(mproj) )
		{
		(void) fprintf(stderr,
				"%s Target map not defined in setup file \"%s\"\n", MyLabel, setup);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Set center longitude from target projection */
	if ( !grid_positions(mproj, &Inumx, &Inumy, &Aglen, &Apstns,
					NullPtr(float ***), NullPtr(float ***))
			|| !grid_center(mproj, NullPointPtr, NullFloat, &Clon) )
		{
		(void) fprintf(stderr,
				"%s Problem with grid positions or center longitude\n", MyLabel);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Set smoothing resolution from map projection and smoothing */
	sres = (mproj->definition.xlen + mproj->definition.ylen) / smoothing;

	/* Retrieve Allied Model information */
	sdef = get_source_info(amodel, subarea);
	if ( IsNull(sdef) )
		{
		(void) fprintf(stderr, "%s Unrecognized Allied Model \"%s %s\"\n",
				MyLabel, amodel, subarea);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}
	allied = sdef->allied;
	if ( IsNull(allied) )
		{
		(void) fprintf(stderr, "%s Missing allied_model information\n", MyLabel);
		(void) fprintf(stderr, "%s      for Allied Model \"%s %s\"\n",
				MyLabel, amodel, subarea);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Must be one required field and one output metafile */
	fields    = allied->fields;
	metafiles = allied->metafiles;
	if ( fields->nfields != 1 )
		{
		(void) fprintf(stderr,
				"%s Need exactly 1 member of required_fields block\n", MyLabel);
		(void) fprintf(stderr,
				"%s      for Allied Model \"%s %s\"\n", MyLabel, amodel, subarea);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}
	if ( metafiles->nfiles != 1 )
		{
		(void) fprintf(stderr,
				"%s Need exactly 1 member of metafiles block\n", MyLabel);
		(void) fprintf(stderr,
				"%s      for Allied Model \"%s %s\"\n", MyLabel, amodel, subarea);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Set the data directory and base directory for this Allied Model */
	datadir = get_directory(sdef->src_io->src_tag);
	basedir = source_directory_by_name(amodel, subarea, FpaCblank);

	/* Create the base directory (if required) */
	if ( blank(basedir) )
		{
		basedir = prepare_source_directory_by_name(amodel, subarea, FpaCblank);
		}
	if ( blank(basedir) )
		{
		(void) fprintf(stderr, "%s Problem setting base directory\n", MyLabel);
		(void) fprintf(stderr, "%s      for Allied Model \"%s %s\"\n",
				MyLabel, amodel, subarea);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}
	(void) strcpy(LockDir, basedir);

	/* Initialize field descriptor for this Allied Model */
	if (DebugMode)
		{
		(void) printf("Initializing fdesc\n");
		(void) printf("   Map: %g X %g\n", mproj->definition.xlen,
					mproj->definition.ylen);
		(void) printf("   Directory: %s\n", datadir);
		(void) printf("   Source: %s\n", sdef->name);
		(void) printf("   Run time: %s\n", rtime);
		}
	init_fld_descript(&fdesc);
	if ( !set_fld_descript(&fdesc,
							FpaF_MAP_PROJECTION, mproj,
							FpaF_DIRECTORY_PATH, datadir,
							FpaF_SOURCE,         sdef,
							FpaF_RUN_TIME,       rtime,
							FpaF_END_OF_LIST) )
		{
		(void) fprintf(stderr,
				"%s Problem initializing field descriptor\n", MyLabel);
		(void) fprintf(stderr,
				"%s      for Allied Model \"%s %s\"\n", MyLabel, amodel, subarea);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Prepare the data directory for the output metafile */
	basedir = prepare_source_directory(&fdesc);
	if ( blank(basedir) )
		{
		(void) fprintf(stderr,
				"%s Problem preparing base directory\n", MyLabel);
		(void) fprintf(stderr,
				"%s      for Allied Model \"%s %s\"\n", MyLabel, amodel, subarea);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Initialize a field descriptor for the source data directory */
	if (DebugMode)
		{
		(void) printf("Initializing descript\n");
		(void) printf("   Map: %g X %g\n", mproj->definition.xlen,
					mproj->definition.ylen);
		(void) printf("   Directory: %s\n", datadir);
		(void) printf("   Source: %s\n", allied->src_def->name);
		(void) printf("   Subsource: %s\n", allied->sub_def->name);
		(void) printf("   Run time: %s\n", rtime);
		}
	init_fld_descript(&descript);
	if ( !set_fld_descript(&descript,
							FpaF_MAP_PROJECTION, mproj,
							FpaF_DIRECTORY_PATH, datadir,
							FpaF_SOURCE,         allied->src_def,
							FpaF_SUBSOURCE,      allied->sub_def,
							FpaF_RUN_TIME,       rtime,
							FpaF_END_OF_LIST) )
		{
		(void) fprintf(stderr,
				"%s Error setting source data directory \"%s %s\"\n",
				MyLabel, allied->src_def->name, allied->sub_def->name);
		(void) fprintf(stderr,
				"%s      for Allied Model \"%s %s\"\n", MyLabel, amodel, subarea);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Check for the source data directory             */
	/*  ... and reset the field descriptor to use most */
	/*       recent data (if required)                 */
	if ( blank(source_directory(&descript)) )
		{
		if (DebugMode)
			{
			(void) printf("Setting descript (to use most recent run time)\n");
			(void) printf("   Run time: %s\n", FpaCblank);
			}
		(void) set_fld_descript(&descript,
									FpaF_RUN_TIME, FpaCblank,
									FpaF_END_OF_LIST);
		}

	/* Check to ensure that the output file is an AREA type field */
	if ( metafiles->flds[0]->element->fld_type != FpaC_DISCRETE )
		{
		(void) fprintf(stderr, "%s Output metafile \"%s %s\"\n",
				MyLabel, metafiles->flds[0]->element->name,
				metafiles->flds[0]->level->name);
		(void) fprintf(stderr, "%s      for Allied Model \"%s %s\"\n",
				MyLabel, amodel, subarea);
		(void) fprintf(stderr, "%s      must be a \"Discrete\" type field!\n",
				MyLabel);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Initialize a field descriptor for the output metafile */
	/* Note that there will be only one metafile!            */
	if (DebugMode)
		{
		(void) printf("Setting fdesc\n");
		(void) printf("   Element: %s\n", metafiles->flds[0]->element->name);
		(void) printf("   Level: %s\n", metafiles->flds[0]->level->name);
		}
	if ( !set_fld_descript(&fdesc,
							FpaF_ELEMENT, metafiles->flds[0]->element,
							FpaF_LEVEL,   metafiles->flds[0]->level,
							FpaF_END_OF_LIST) )
		{
		(void) fprintf(stderr, "%s Error setting output metafile \"%s %s\"\n",
				MyLabel, metafiles->flds[0]->element->name,
				metafiles->flds[0]->level->name);
		(void) fprintf(stderr, "%s      for Allied Model \"%s %s\"\n",
				MyLabel, amodel, subarea);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Set the field type from the required fields block */
	fdef  = fields->flds[0];
	fkind = fdef->element->fld_type;

	/* Initialize a field descriptor for required fields file */
	/* Note that there will be only one required fields file! */
	if (DebugMode)
		{
		(void) printf("Initializing descriptin\n");
		(void) printf("   Map: %g X %g\n", mproj->definition.xlen,
					mproj->definition.ylen);
		(void) printf("   Directory: %s\n", datadir);
		(void) printf("   Source: %s\n", fields->src_defs[0]->name);
		if ( NotNull(fields->sub_defs[0]) )
			(void) printf("   Subsource: %s\n", fields->sub_defs[0]->name);
		(void) printf("   Run time: %s\n", rtime);
		(void) printf("   Element: %s\n", fdef->element->name);
		(void) printf("   Level: %s\n", fdef->level->name);
		}
	init_fld_descript(&descriptin);
	if ( !set_fld_descript(&descriptin,
					FpaF_MAP_PROJECTION, mproj,
					FpaF_DIRECTORY_PATH, datadir,
					FpaF_SOURCE,         fields->src_defs[0],
					FpaF_SUBSOURCE,      fields->sub_defs[0],
					FpaF_RUN_TIME,       rtime,
					FpaF_ELEMENT,        fdef->element,
					FpaF_LEVEL,          fdef->level,
					FpaF_END_OF_LIST) )
		{
		(void) fprintf(stderr, "%s Error setting required field \"%s %s\"\n",
				MyLabel, fdef->element->name, fdef->level->name);
		(void) fprintf(stderr, "%s      for Allied Model \"%s %s\"\n",
				MyLabel, amodel, subarea);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Check for the source data directory             */
	/*  ... and reset the field descriptor to use most */
	/*       recent data (if required)                 */
	if ( blank(source_directory(&descriptin)) )
		{
		(void) fprintf(stdout,
				"%s Cannot find directory \"%s %s\" at \"%s\"\n",
				MyLabel,
				((descriptin.sdef)?   descriptin.sdef->name:   ""),
				((descriptin.subdef)? descriptin.subdef->name: ""),
				SafeStr(descriptin.rtime));
		(void) fprintf(stdout,
				"%s  Will try to use most recent data!\n", MyLabel);
		if (DebugMode)
			{
			(void) printf("Setting descriptin\n");
			(void) printf("   Run time: %s\n", FpaCblank);
			}
		(void) set_fld_descript(&descriptin,
									FpaF_RUN_TIME, FpaCblank,
									FpaF_END_OF_LIST);
		}

	/* Set the list of valid times from the run string parameters */
	if ( numvts > 0 )
		{
		(void) fprintf(stderr,
					"%s Setting run time list of valid times ... numvts: %d\n",
					MyLabel, numvts);
		vtimes = INITMEM(STRING, numvts);
		for ( nvt=0; nvt<numvts; nvt++ )
			{
			vtimes[nvt] = strdup(interpret_timestring(argv[10+nvt], rtime, Clon));
			}
		}

	/* Otherwise create lists of valid times based on the  */
	/*  time dependence type and the source data directory */
	else
		{

		/* Get a list of valid times from the depiction sequence */
		time_dep = fdesc.edef->elem_tdep->time_dep;
		switch ( time_dep )
			{

			/* Branch to FpaC_DAILY fields */
			case FpaC_DAILY:
				numvts = daily_field_local_times(&fdesc, &descript,
												FpaC_TIMEDEP_ANY, Clon, &vlist);
				break;

			/* Branch to all other types of fields */
			default:
				numvts = source_valid_time_list(&descript, FpaC_NORMAL, &vlist);
				break;
			}

		/* Create copies of valid times */
		(void) fprintf(stderr,
					"%s Setting retrieved list of valid times ... numvts: %d\n",
					MyLabel, numvts);
		vtimes = INITMEM(STRING, numvts);
		for ( nvt=0; nvt<numvts; nvt++ )
			{
			vtimes[nvt] = strdup(vlist[nvt]);
			}

		/* Free space for valid times */
		switch ( time_dep )
			{

			/* Branch to FpaC_DAILY fields */
			case FpaC_DAILY:
				(void) daily_field_local_times_free(&vlist, numvts);
				break;

			/* Branch to all other types of fields */
			default:
				(void) source_valid_time_list_free(&vlist, numvts);
				break;
			}
		}

	/* Create a metafile at each valid time */
	for ( nummeta=0, nvt=0; nvt<numvts; nvt++ )
		{

		/* Set the field descriptor for the required fields file */
		if (DebugMode)
			{
			(void) printf("Setting descriptin\n");
			(void) printf("   Valid time: %s\n", vtimes[nvt]);
			}
		(void) set_fld_descript(&descriptin,
								FpaF_VALID_TIME, vtimes[nvt],
								FpaF_END_OF_LIST);

		/* Set the field descriptor for the output metafile */
		if (DebugMode)
			{
			(void) printf("Setting fdesc\n");
			(void) printf("   Valid time: %s\n", vtimes[nvt]);
			}
		(void) set_fld_descript(&fdesc,
								FpaF_VALID_TIME, vtimes[nvt],
								FpaF_END_OF_LIST);

		/* Extract a set of objects based on type of required field */
		switch (fkind)
			{

			/* Retrieve a set of areas from contours of a surface type field */
			case FpaC_CONTINUOUS:
			case FpaC_VECTOR:

				/* First retrieve the surface */
				sfc = retrieve_surface(&descriptin);

				/* Continue with next time if field is missing */
				if ( IsNull(sfc) )
					{
					(void) fprintf(stderr,
							"%s No surface for required field \"%s\" \"%s\" at \"%s\"\n",
							MyLabel, fdef->element->name, fdef->level->name,
							vtimes[nvt]);
					(void) fprintf(stderr,
							"%s      for Allied Model \"%s %s\"\n",
							MyLabel, amodel, subarea);
					continue;
					}
				
				/* Convert the maximum and minimum values (if required) */
				if ( usemin )
					(void) convert_value(units, (double) minval,
											sfc->units.name, &dminval);
				else
					dminval = -FPA_FLT_MAX;
				if ( usemax )
					(void) convert_value(units, (double) maxval,
											sfc->units.name, &dmaxval);
				else
					dmaxval = FPA_FLT_MAX;
		
				if (DebugMode)
					{
					(void) fprintf(stderr,
							"%s Extracting areas from contours: %f to %f %s (%f to %f %s)\n",
							MyLabel, minval, maxval, units, dminval, dmaxval,
							sfc->units.name);
					}

				/* Extract a set of contour areas from the surface */
				set = contour_areaset(sfc, (float) dminval, (float) dmaxval,
						NullPtr(USPEC *), NullPtr(BOX *));
				break;

			/* Cannot retrieve areas from other types of field */
			case FpaC_DISCRETE:
			case FpaC_WIND:
			case FpaC_LINE:
			case FpaC_SCATTERED:
			case FpaC_LCHAIN:
			default:
				(void) fprintf(stderr,
						"%s Cannot create contour areas for required field \"%s\" \"%s\"\n",
						MyLabel, fdef->element->name, fdef->level->name);
				(void) fprintf(stderr,
						"%s      for Allied Model \"%s %s\"\n",
						MyLabel, amodel, subarea);
				(void) fprintf(stdout, "%s Aborted\n", MyLabel);
				return (-1);
			}

		/* Error message if no SET or no objects in SET */
		if ( IsNull(set) )
			{
			(void) fprintf(stderr,
					"%s No contours for required field \"%s\" \"%s\" at \"%s\"\n",
					MyLabel, fdef->element->name, fdef->level->name, vtimes[nvt]);
			(void) fprintf(stderr,
					"%s      for Allied Model \"%s %s\"\n",
					MyLabel, amodel, subarea);
			continue;
			}
		if ( set->num <= 0 )
			{
			if ( usemin && usemax )
				(void) fprintf(stderr, "%s No contours between %f and %f %s",
						MyLabel, minval, maxval, units);
			else if ( usemin )
				(void) fprintf(stderr, "%s No contours above %f %s",
						MyLabel, minval, units);
			else if ( usemax )
				(void) fprintf(stderr, "%s No contours below %f %s",
						MyLabel, maxval, units);
			(void) fprintf(stderr,
						" in required field \"%s\" \"%s\" at \"%s\"\n",
						fdef->element->name, fdef->level->name, vtimes[nvt]);
			(void) fprintf(stderr,
					"%s      for Allied Model \"%s %s\"\n",
					MyLabel, amodel, subarea);
			continue;
			}

		/* Identify attribute tags based on type of required field */
		switch (fkind)
			{
			case FpaC_CONTINUOUS:
			case FpaC_VECTOR:

				/* Identify the required field attribute tags */
				attrib_info = fields->attinfo[0];
				attrib_meta = metafiles->attinfo[0];
				if (DebugMode)
					{
					(void) fprintf(stderr,
							"%s Identifying %d required field attributes for SURFACE objects\n",
							MyLabel, attrib_info->nattribs);
					}
				for ( natt=0; natt<attrib_info->nattribs; natt++ )
					{
					attok = FALSE;
					if (DebugMode)
						{
						(void) fprintf(stderr,
								"%s   Comparing attribute tag: %s\n",
								MyLabel, attrib_info->tag[natt]);
						}

					/* Structure for attributes output to metafile */
					for ( nn=0; nn<attrib_meta->nattribs; nn++ )
						{
						if ( !same_ic(attrib_info->tag[natt],
									attrib_meta->tag[nn]) ) continue;

						/* Check that units match exactly! */
						if ( !same_ic(attrib_info->attunit[natt]->name,
										attrib_meta->attunit[nn]->name) )
							{
							(void) fprintf(stderr,
									"%s Units do not match for required_fields / metafiles tag: %s\n",
									MyLabel, attrib_meta->tag[natt]);
							(void) fprintf(stderr,
									"%s   Units for required_fields: %s  for metafiles: %s\n",
									MyLabel, attrib_meta->attunit[natt]->name,
									attrib_meta->attunit[nn]->name);
							continue;
							}

						/* Save the information in the TagAtts structure */
						ntx = NumTagAtt++;
						TagAtts = GETMEM(TagAtts, TAG_INFO, NumTagAtt);
						TagAtts[ntx].tag     = safe_strdup(attrib_info->tag[natt]);
						TagAtts[ntx].attname = safe_strdup(attrib_info->attname[natt]);
						TagAtts[ntx].attunit = attrib_info->attunit[natt];
						break;
						}
					if ( nn < attrib_meta->nattribs ) attok = TRUE;

					/* Error if attribute not recognized */
					if ( !attok )
						{
						(void) fprintf(stderr,
								"%s     Unacceptable required_fields attribute tag: %s\n",
								MyLabel, attrib_info->tag[natt]);
						}
					}

				/* Check that all metafile attribute tags have been entered */
				/*  or are one of the default ones!                         */
				if (DebugMode)
					{
					(void) fprintf(stderr,
							"%s Checking %d metafile attributes for SURFACE objects\n",
							MyLabel, attrib_meta->nattribs);
					(void) fprintf(stderr,
							"%s  (%d attributes saved)\n",
							MyLabel, NumTagAtt);
					}
				if ( NumTagAtt < attrib_meta->nattribs )
					{
					for ( natt=0; natt<attrib_meta->nattribs; natt++ )
						{

						/* First check attributes from required fields */
						for ( nn=0; nn<NumTagAtt; nn++ )
							{
							if ( same_ic(attrib_meta->tag[natt], TagAtts[nn].tag) )
								break;
							}
						if ( nn < NumTagAtt ) continue;

						/* Error if attribute not recognized */
						(void) fprintf(stderr,
								"%s     Unacceptable metafiles attribute tag: %s\n",
								MyLabel, attrib_meta->tag[natt]);
						}
					}
				break;
			}

		if (DebugMode)
			{
			(void) fprintf(stderr, "%s Creating %d areas from contours\n",
					MyLabel, set->num);
			}

		/* Create an SET Object to hold the created areas */
		aset = create_set("area");

		/* Create areas with smoothed boundaries, holes and attributes */
		for ( nset=0; nset<set->num; nset++ )
			{
			area = copy_area((AREA) set->list[nset], TRUE);
			if ( IsNull(area) ) continue;
			if ( IsNull(area->bound) ) continue;
			if ( IsNull(area->attrib) ) continue;

			if (DebugMode)
				{
				if ( area->bound->numhole <= 0 )
					{
					(void) fprintf(stderr,
							"%s Creating area %d from contours - %d points (no holes)\n",
							MyLabel, nset, area->bound->boundary->numpts);
					}
				else
					{
					(void) fprintf(stderr,
							"%s Creating area %d from contours - %d points and %d holes (",
							MyLabel, nset, area->bound->boundary->numpts,
							area->bound->numhole);
					for ( nh=area->bound->numhole-1; nh>0; nh-- )
						(void) fprintf(stderr, "%d-",
								area->bound->holes[nh]->numpts);
					(void) fprintf(stderr, "%d points)\n", 
							area->bound->holes[nh]->numpts);
					}
				}

			/* Ensure that area boundary does not cross over itself */
			bdry = copy_line(area->bound->boundary);
			while ( looped_line_crossing(bdry, spos, &ips, &ipe) )
				{
				if (DebugMode)
					{
					(void) fprintf(stderr,
							"%s  Area crossover between segments: %d and %d (0 to %d)\n",
							MyLabel, ips, ipe, bdry->numpts-1);
					(void) fprintf(stderr,
							"%s   Points: %.2f/%.2f  %.2f/%.2f  %.2f/%.2f\n",
							MyLabel, bdry->points[ips][X], bdry->points[ips][Y],
							spos[X], spos[Y],
							bdry->points[ipe+1][X], bdry->points[ipe+1][Y]);
					}

				/* Truncate the line to remove the crossover */
				tline = append_line_portion(NullLine, bdry, 0, ips);
				add_point_to_line(tline, spos);
				tline = append_line_portion(tline, bdry, ipe+1, bdry->numpts-1);
				empty_line(bdry);
				append_line(bdry, tline);
				(void) destroy_line(tline);
				}

			/* Smooth the area boundary */
			reset_pipe();
			enable_filter(sres, 0.0);
			enable_spline(sres, TRUE, 0.0, 0.0, 0.0);
			enable_save();
			for ( nn=0; nn<bdry->numpts; nn++ )
				put_pipe(bdry->points[nn][X], bdry->points[nn][Y]);
			flush_pipe();
			nlines = recall_save(&lines);
			if ( nlines <= 0 )
				{
				if (DebugMode)
					{
					(void) fprintf(stderr,
							"%s  No area boundary after smoothing!\n", MyLabel);
					}
				destroy_line(bdry);
				destroy_area(area);
				continue;
				}
			empty_line(bdry);
			append_line(bdry, lines[0]);

			/* Remove the area if too few points in boundary */
			if ( bdry->numpts < minpoints )
				{
				(void) fprintf(stderr,
						"%s  Skipping area boundary with %d points\n",
						MyLabel, bdry->numpts);
				destroy_line(bdry);
				destroy_area(area);
				continue;
				}

			/* Replace the boundary in the AREA Object */
			if ( !replace_area_boundary(area, bdry, &dstat) )
				{
				(void) fprintf(stderr,
						"%s  Error replacing area boundary with %d points\n",
						MyLabel, bdry->numpts);
				destroy_line(bdry);
				destroy_area(area);
				continue;
				}
			if (DebugMode)
				{
				(void) fprintf(stderr,
						"%s  Replacing area boundary with %d points\n",
						MyLabel, bdry->numpts);
				}

			/* Replace boundary holes in the Area Object with smoothed holes */
			for ( nh=area->bound->numhole-1; nh>=0; nh-- )
				{

				/* Make a copy of each hole */
				tline = copy_line(area->bound->holes[nh]);

				/* Smooth the hole */
				reset_pipe();
				enable_filter(sres, 0.0);
				enable_spline(sres, TRUE, 0.0, 0.0, 0.0);
				enable_save();
				for ( nn=0; nn<tline->numpts; nn++ )
					put_pipe(tline->points[nn][X], tline->points[nn][Y]);
				flush_pipe();
				nlines = recall_save(&lines);
				if ( nlines <= 0 )
					{
					if (DebugMode)
						{
						(void) fprintf(stderr,
								"%s  No area hole after smoothing!\n", MyLabel);
						}
					remove_area_hole(area, area->bound->holes[nh]);
					destroy_line(tline);
					continue;
					}
				empty_line(tline);
				append_line(tline, lines[0]);

				/* Remove the hole if too few points */
				if ( tline->numpts < minpoints )
					{
					(void) fprintf(stderr,
							"%s  Skipping area hole with %d points\n",
							MyLabel, tline->numpts);
					remove_area_hole(area, area->bound->holes[nh]);
					destroy_line(tline);
					continue;
					}

				/* Replace the hole in the AREA Object */
				if ( !replace_area_hole(area, nh, tline, &dstat) )
					{
					(void) fprintf(stderr,
							"%s  Error replacing area hole with %d points\n",
							MyLabel, tline->numpts);
					remove_area_hole(area, area->bound->holes[nh]);
					destroy_line(tline);
					continue;
					}
				if (DebugMode)
					{
					(void) fprintf(stderr,
							"%s  Replacing area hole with %d points\n",
							MyLabel, tline->numpts);
					}
				}

			/* Create an attribute structure for the AREA Object */
			acal = CAL_create_by_name(fdesc.edef->name, fdesc.ldef->name);

			/* Add the metafile default attributes to the AREA Object */
			def_info = metafiles->definfo[0];
			for ( natt=0; natt<def_info->natt_defs; natt++ )
				{

				/* Add the default attribute and value */
				CAL_add_attribute(acal, def_info->attname_defs[natt],
										def_info->attval_defs[natt]);
				if (DebugMode)
					{
					(void) fprintf(stderr,
							"%s  Adding metafile default attribute: %s  with value: %s\n",
							MyLabel, def_info->attname_defs[natt],
							def_info->attval_defs[natt]);
					}
				}

			/* Add the metafile attributes to the AREA Object */
			for ( natt=0; natt<attrib_meta->nattribs; natt++ )
				{

				if (DebugMode)
					{
					(void) fprintf(stderr,
							"%s  Find matching attribute for metafile tag: %s\n",
							MyLabel, attrib_meta->tag[natt]);
					}

				/* Find the matching required field attribute tag */
				for ( nn=0; nn<NumTagAtt; nn++ )
					{
					if ( same_ic(attrib_meta->tag[natt], TagAtts[nn].tag) ) break;
					}
				if ( nn >= NumTagAtt ) continue;

				/* Extract attribute value from object attributes */
				cal = area->attrib;
				val = CAL_get_attribute(cal, TagAtts[nn].attname);

				/* Add attribute value to metafile attributes */
				CAL_add_attribute(acal, attrib_meta->attname[natt], val);
				if (DebugMode)
					{
					(void) fprintf(stderr,
							"%s  Adding metafile attribute: %s  with value: %s\n",
							MyLabel, attrib_meta->attname[natt], val);
					}
				}
			define_area_attribs(area, acal);
			acal = NullCal;

			/* Add the AREA Object to the set */
			(void) add_item_to_set(aset, (ITEM) area);
			if (DebugMode)
				{
				if ( area->bound->numhole <= 0 )
					{
					(void) fprintf(stderr,
							"%s Adding area %d from smoothed contours - %d points (no holes)\n",
							MyLabel, nset, area->bound->boundary->numpts);
					}
				else
					{
					(void) fprintf(stderr,
							"%s Adding area %d from smoothed contours - %d points and %d holes (",
							MyLabel, nset, area->bound->boundary->numpts,
							area->bound->numhole);
					for ( nh=area->bound->numhole-1; nh>0; nh-- )
						(void) fprintf(stderr, "%d-",
								area->bound->holes[nh]->numpts);
					(void) fprintf(stderr, "%d points)\n", 
							area->bound->holes[nh]->numpts);
					}
				}
			}

		/* Create a METAFILE Object to hold the SET Object of created areas */
		meta = create_metafile();
		define_mf_tstamp(meta, fdesc.rtime, fdesc.vtime);
		define_mf_projection(meta, mproj);

		/* Move the SET Object containing the areas into the METAFILE Object */
		add_set_to_metafile(meta, "b", fdesc.edef->name, fdesc.ldef->name, aset);

		/* Set shuffle lock in base directory while outputting SET field */
		(void) strcpy(LockVtime, fdesc.vtime);
		if ( !set_file_lock(LockDir, LockVtime) )
			{
			(void) fprintf(stderr, "%s Unable to establish file lock\n", MyLabel);
			(void) fprintf(stderr, "%s      for Allied Model \"%s %s\"\n",
					MyLabel, amodel, subarea);
			(void) fprintf(stdout, "%s Aborted\n", MyLabel);
			return (-1);
			}
		Locked = TRUE;

		/* Construct old and new metafile names for METAFILE Object */
		/*  ... and remove any existing metafiles with these names! */
		fname = build_meta_filename(&fdesc);
		if ( !blank(fname) ) (void) remove_file(fname, NULL);
		fname = construct_meta_filename(&fdesc);
		if ( !blank(fname) ) (void) remove_file(fname, NULL);

		/* Construct old format metafile name if new format name not found */
		if ( blank(fname) ) fname = build_meta_filename(&fdesc);

		/* Output the METAFILE Object containing the SET Object */
		(void) write_metafile(fname, meta, 4);
		nummeta++;

		/* Remove the current file lock in the base directory */
		(void) release_file_lock(LockDir, LockVtime);
		Locked = FALSE;

		/* Remove space used by METAFILE Object */
		meta = destroy_metafile(meta);
		}

	/* Error message if no output fields created */
	if ( nummeta < 1 )
		{
		(void) fprintf(stderr, "%s No metafiles created\n", MyLabel);
		(void) fprintf(stderr, "%s      for Allied Model \"%s %s\"\n",
						MyLabel, amodel, subarea);
		}

	/* Shutdown message */
	(void) systime(&cyear, &cjday, &chr, &cmin, &csec);
	(void) mdate(&cyear, &cjday, &cmonth, &cday);
	(void) fprintf(stdout, "\n%s Finished: %d/%.2d/%.2d %.2d:%.2d:%.2d GMT\n",
			MyLabel, cyear, cmonth, cday, chr, cmin, csec);

	return 0;
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
	(void) fprintf(stdout,
			"%s !!! %s Has Occurred - Terminating\n", MyLabel, sname);

	/* Die gracefully */
	if ( Locked )
		{
		(void) fprintf(stdout,
				"    Removing file lock in directory \"%s\"\n", LockDir);
		(void) release_file_lock(LockDir, LockVtime);
		}
	exit(1);
	}
