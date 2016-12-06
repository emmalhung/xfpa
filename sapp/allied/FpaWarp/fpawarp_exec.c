/***********************************************************************
*                                                                      *
*     f p a w a r p _ e x e c . c                                      *
*                                                                      *
*     FPA Field Warp Program                                           *
*                                                                      *
*     This program contains a function that will take a given field    *
*     (first guess) and one or more scattered files and return a field *
*     that is warped to provide a reasonable (if not exact) fit of the *
*     scattered point information to the first guess field.            *
*                                                                      *
*     Version 5 (c) Copyright 1997 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2003 Environment Canada (MSC)            *
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

/* We need FPA library definitions */
#include <fpa.h>

#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>

static	LOGICAL	DebugMode = FALSE;

/* Trap for error situations */
static	void	error_trap(int);

/* Internal static functions */
static	PLOT	extract_plot_data(PLOT, SET, STRING, STRING);


/* Allied Model aliases in Config files */
static	const	STRING	GuessField   = "guess_field";
static	const	STRING	PlotField    = "plot_field";

/* Label and units for Allied Model data in internal plot file */
static	const	STRING	PlotData  = "plot_data";
static	const	STRING	PlotUnits = "MKS";

/* Default weighting factors for sfit_surface() funtion */
static	const	float	WgtFactor = 1.20;


/* Default message strings */
static	const	STRING	MyTitle = "FPA Field Warp";
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
	STRING							setup, amodel, subarea, rtime;
	LOGICAL							create_anyway, need_input, have_input;
	float							tension, influence, sparse_dist;
	int								numvts, nslist;
	STRING							*slist;
	int								cyear, cjday, cmonth, cday, chr, cmin, csec;
	MAP_PROJ						*mproj;
	int								Inumx, Inumy;
	POINT							**Apstns;
	float							Aglen, Clon;
	STRING							datadir, basedir, fname;
	FLD_DESCRIPT					fdesc, descript, descriptin;
	int								nummeta, nvt, nfld;
	STRING							*vguess, *vplot, *vmeta, *vlist;
	int								ndata, iplt, nn, iix, iiy;
	float							*wdata, mean_error, last_mean_error;
	float							avg_error, delta_error, dist;
	double							sfcval;
	POINT							*wpstn, pos;
	PSUB							*psub;
	LOGICAL							valid;
	FpaConfigSourceStruct			*sdef;
	FpaConfigSourceAlliedStruct		*allied;
	FpaConfigAlliedFieldsStruct		*fields;
	FpaConfigAlliedMetafilesStruct	*metafiles;
	FpaConfigFieldStruct			*fdef, *fdefp;
	FpaCtimeDepTypeOption			time_dep;
	METAFILE						meta    = NullMeta;
	SURFACE							sfc     = NullSfc;
	SET								spotset = NullSet;
	PLOT							plot    = NullPlot;

	/* Ignore hangup, interrupt and quit signals so we can survive after */
	/* logging off */
	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	status = setvbuf(stdout, NullString, _IOLBF, 0);
	status = setvbuf(stderr, NullString, _IOLBF, 0);

	/* Set debug mode (if requested) */
	if (getenv("FPA_DEBUG_WARP") != NULL) DebugMode = TRUE;
	if ( DebugMode ) (void) pr_control(NULL, 5, 1);
	else             (void) pr_control(NULL, 1, 1);

	/* Validate run string parameters */
	if ( argc < 7 )
		{
		(void) fprintf(stderr, "Usage:\n");
		(void) fprintf(stderr, "   fpawarp_exec  <setup_file>");
		(void) fprintf(stderr, "  <allied_model_name>");
		(void) fprintf(stderr, "  <subarea_name>  <run_time>\n");
		(void) fprintf(stderr, "                  <tension (50)>");
		(void) fprintf(stderr, "  <influence (1)>");
		(void) fprintf(stderr, "  <create_with_no_data (Y/N)>\n");
		(void) fprintf(stderr, "                   (<guess_file_time>");
		(void) fprintf(stderr, "  <plot_file_time>  <output_file_time>)\n\n");
		(void) fprintf(stderr, "   Note that  <..._time>  variables are");
		(void) fprintf(stderr, " optional with format  xx  or  rr/hh(L)\n");
		(void) fprintf(stderr, "     where  xx  is hours after current");
		(void) fprintf(stderr, " time,  rr  is day (0 for today,\n");
		(void) fprintf(stderr, "       1 for tomorrow, ...), and  hh(L)");
		(void) fprintf(stderr, "  is GMT (or local) hour of day (0-23)\n");
		return (-1);
		}
	if ( argc < 8 )
		{
		(void) fprintf(stderr, "Change usage to:\n");
		(void) fprintf(stderr, "   fpawarp_exec  <setup_file>");
		(void) fprintf(stderr, "  <allied_model_name>");
		(void) fprintf(stderr, "  <subarea_name>  <run_time>\n");
		(void) fprintf(stderr, "                  <tension (50)>");
		(void) fprintf(stderr, "  <influence (1)>");
		(void) fprintf(stderr, "  <create_with_no_data (Y/N)>\n");
		(void) fprintf(stderr, "                   (<guess_file_time>");
		(void) fprintf(stderr, "  <plot_file_time>  <output_file_time>)\n\n");
		(void) fprintf(stderr, "   Note that  <..._time>  variables are");
		(void) fprintf(stderr, " optional with format  xx  or  rr/hh(L)\n");
		(void) fprintf(stderr, "     where  xx  is hours after current");
		(void) fprintf(stderr, " time,  rr  is day (0 for today,\n");
		(void) fprintf(stderr, "       1 for tomorrow, ...), and  hh(L)");
		(void) fprintf(stderr, "  is GMT (or local) hour of day (0-23)\n");
		}

	/* Obtain a licence */
	(void) app_license("model.fpawarp");

	/* Trap all signals that would abort the process by default */
	(void) set_error_trap(error_trap);

	/* Set run string parameters */
	setup         = argv[1];
	amodel        = argv[2];
	subarea       = argv[3];
	rtime         = argv[4];
	tension       = (float) atof(argv[5]);
	influence     = (float) atof(argv[6]);
	create_anyway = TRUE;
	if ( argc >= 8 )
		{
		if ( same_ic(argv[7], "F") || same_ic(argv[7], "FALSE")
				|| same_ic(argv[7], "N") || same_ic(argv[7], "NO") )
			{
			create_anyway = FALSE;
			}
		}

	/* Set number of <..._time> variables passed */
	numvts = 0;
	if ( argc > 8 )
		{
		numvts = argc - 8;
		if ( (numvts / 3) * 3 != numvts )
			{
			(void) fprintf(stderr, "%s Not enough times in run string!\n",
							MyLabel);
			(void) fprintf(stdout, "%s Aborted\n", MyLabel);
			return (-1);
			}
		numvts /= 3;
		}

	/* Reset tension or influence if too small! */
	if ( tension   < 10.0 ) tension   = 10.0;
	if ( influence <  1.0 ) influence =  1.0;

	/* Startup message */
	(void) sprintf(MyLabel, "[%d] %s:", getpid(), MyTitle);
	(void) systime(&cyear, &cjday, &chr, &cmin, &csec);
	(void) mdate(&cyear, &cjday, &cmonth, &cday);
	(void) fprintf(stdout, "\n%s Beginning: %d/%.2d/%.2d %.2d:%.2d:%.2d GMT\n",
					MyLabel, cyear, cmonth, cday, chr, cmin, csec);
	(void) fprintf(stdout, "%s      for Allied Model \"%s %s\" at \"%s\"\n",
					MyLabel, amodel, subarea, rtime);
	(void) fprintf(stdout, "%s      with tension: %f  and influence: %f\n",
					MyLabel, tension, influence);

	/* Read the setup file */
	/* This moves to standard FPA directory */
	nslist = setup_files(setup, &slist);
	if ( !define_setup(nslist, slist) )
		{
		(void) fprintf(stderr, "%s Problem with setup file \"%s\"\n",
						MyLabel, setup);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Retrieve the target projection definition */
	mproj = get_target_map();
	if ( IsNull(mproj) )
		{
		(void) fprintf(stderr, "%s Target map not defined in setup file \"%s\"\n",
						MyLabel, setup);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Set grid positions and center longitude from target projection */
	if ( !grid_positions(mproj, &Inumx, &Inumy, &Aglen, &Apstns,
					NullPtr(float ***), NullPtr(float ***))
			|| !grid_center(mproj, NullPointPtr, NullFloat, &Clon) )
		{
		(void) fprintf(stderr, "%s Problem with grid positions or center longitude\n",
						MyLabel);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Set critical distance for data sparse areas */
	sparse_dist = influence * Aglen;

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
		(void) fprintf(stderr, "%s Missing allied_model information\n",
						MyLabel);
		(void) fprintf(stderr, "%s      for Allied Model \"%s %s\"\n",
						MyLabel, amodel, subarea);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Must be at least one required field and exactly one output metafile */
	fields    = allied->fields;
	metafiles = allied->metafiles;
	if ( fields->nfields < 1 )
		{
		(void) fprintf(stderr, "%s Need at least 1 member of required_fields block\n",
						MyLabel);
		(void) fprintf(stderr, "%s      for Allied Model \"%s %s\"\n",
						MyLabel, amodel, subarea);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}
	if ( metafiles->nfiles != 1 )
		{
		(void) fprintf(stderr, "%s Need exactly 1 member of metafiles block\n",
						MyLabel);
		(void) fprintf(stderr, "%s      for Allied Model \"%s %s\"\n",
						MyLabel, amodel, subarea);
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
		(void) printf("   Map %g X %g\n", mproj->definition.xlen,
					mproj->definition.ylen);
		(void) printf("   Directory %s\n", datadir);
		(void) printf("   Source %s\n", sdef->name);
		(void) printf("   Run time %s\n", rtime);
		}
	(void) init_fld_descript(&fdesc);
	if ( !set_fld_descript(&fdesc,
							FpaF_MAP_PROJECTION, mproj,
							FpaF_DIRECTORY_PATH, datadir,
							FpaF_SOURCE,         sdef,
							FpaF_RUN_TIME,       rtime,
							FpaF_END_OF_LIST) )
		{
		(void) fprintf(stderr, "%s Problem initializing field descriptor\n",
						MyLabel);
		(void) fprintf(stderr, "%s      for Allied Model \"%s %s\"\n",
						MyLabel, amodel, subarea);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Prepare the data directory for the output metafile */
	basedir = prepare_source_directory(&fdesc);
	if ( blank(basedir) )
		{
		(void) fprintf(stderr, "%s Problem preparing base directory\n",
						MyLabel);
		(void) fprintf(stderr, "%s      for Allied Model \"%s %s\"\n",
						MyLabel, amodel, subarea);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Initialize a field descriptor for the source data directory */
	if (DebugMode)
		{
		(void) printf("Initializing descript\n");
		(void) printf("   Map %g X %g\n", mproj->definition.xlen,
					mproj->definition.ylen);
		(void) printf("   Directory %s\n", datadir);
		(void) printf("   Source %s\n", allied->src_def->name);
		(void) printf("   Subsource %s\n", allied->sub_def->name);
		(void) printf("   Run time %s\n", rtime);
		}
	(void) init_fld_descript(&descript);
	if ( !set_fld_descript(&descript,
							FpaF_MAP_PROJECTION, mproj,
							FpaF_DIRECTORY_PATH, datadir,
							FpaF_SOURCE,         allied->src_def,
							FpaF_SUBSOURCE,      allied->sub_def,
							FpaF_RUN_TIME,       rtime,
							FpaF_END_OF_LIST) )
		{
		(void) fprintf(stderr, "%s Error setting source data directory \"%s %s\"\n",
						MyLabel, allied->src_def->name, allied->sub_def->name);
		(void) fprintf(stderr, "%s      for Allied Model \"%s %s\"\n",
						MyLabel, amodel, subarea);
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
			(void) printf("Setting descript\n");
			(void) printf("   Run time %s\n", FpaCblank);
			}
		(void) set_fld_descript(&descript,
									FpaF_RUN_TIME, FpaCblank,
									FpaF_END_OF_LIST);
		}

	/* Initialize a field descriptor for the output metafile */
	/* Note that there should be only one metafile!          */
	if (DebugMode)
		{
		(void) printf("Setting fdesc\n");
		(void) printf("   Element %s\n", metafiles->flds[0]->element->name);
		(void) printf("   Level %s\n", metafiles->flds[0]->level->name);
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

	/* Set the lists of valid times from the run string parameters */
	if ( numvts > 0 )
		{
		vguess = INITMEM(STRING, numvts);
		vplot  = INITMEM(STRING, numvts);
		vmeta  = INITMEM(STRING, numvts);
		for ( nvt=0; nvt<numvts; nvt++ )
			{
			vguess[nvt] = strdup(interpret_timestring(argv[8 + nvt*3 + 0],
														rtime, Clon));
			vplot[nvt]  = strdup(interpret_timestring(argv[8 + nvt*3 + 1],
														rtime, Clon));
			vmeta[nvt]  = strdup(interpret_timestring(argv[8 + nvt*3 + 2],
														rtime, Clon));
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
		vguess = INITMEM(STRING, numvts);
		vplot  = INITMEM(STRING, numvts);
		vmeta  = INITMEM(STRING, numvts);
		for ( nvt=0; nvt<numvts; nvt++ )
			{
			vguess[nvt] = strdup(vlist[nvt]);
			vplot[nvt]  = strdup(vlist[nvt]);
			vmeta[nvt]  = strdup(vlist[nvt]);
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

		/* Set the field descriptor for the output metafile */
		if (DebugMode)
			{
			(void) printf("Setting fdesc\n");
			(void) printf("   Valid time %s\n", vmeta[nvt]);
			}
		(void) set_fld_descript(&fdesc,
								FpaF_VALID_TIME, vmeta[nvt],
								FpaF_END_OF_LIST);

		/* Extract the guess field as a SURFACE Object */
		for ( nfld=0; nfld<fields->nfields; nfld++ )
			{

			/* Check for required field alias of GuessField */
			if ( same_start(fields->aliases[nfld], GuessField) )
				{

				/* Set field descriptor for GuessField */
				fdef = fields->flds[nfld];
				if (DebugMode)
					{
					(void) printf("Initializing descriptin\n");
					(void) printf("   Map %g X %g\n",
								mproj->definition.xlen,
								mproj->definition.ylen);
					(void) printf("   Directory %s\n", datadir);
					(void) printf("   Source %s\n",
								fields->src_defs[nfld]->name);
					(void) printf("   Subsource %s\n",
								fields->sub_defs[nfld]->name);
					(void) printf("   Run time %s\n", rtime);
					(void) printf("   Element %s\n",
								fdef->element->name);
					(void) printf("   Level %s\n", fdef->level->name);
					(void) printf("   Valid time %s\n", vguess[nvt]);
					}
				(void) init_fld_descript(&descriptin);
				if ( !set_fld_descript(&descriptin,
								FpaF_MAP_PROJECTION, mproj,
								FpaF_DIRECTORY_PATH, datadir,
								FpaF_SOURCE,         fields->src_defs[nfld],
								FpaF_SUBSOURCE,      fields->sub_defs[nfld],
								FpaF_RUN_TIME,       rtime,
								FpaF_ELEMENT,        fdef->element,
								FpaF_LEVEL,          fdef->level,
								FpaF_VALID_TIME,     vguess[nvt],
								FpaF_END_OF_LIST) ) continue;

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
							"%s  Will try to use most recent data!\n",
							MyLabel);
					if (DebugMode)
						{
						(void) printf("Setting descriptin\n");
						(void) printf("   Run time %s\n", FpaCblank);
						}
					(void) set_fld_descript(&descriptin,
												FpaF_RUN_TIME, FpaCblank,
												FpaF_END_OF_LIST);
					}

				/* Extract the SURFACE Object for GuessField */
				sfc = retrieve_surface(&descriptin);
				break;
				}
			}

		/* Error message if no SURFACE Object for guess field found */
		if ( IsNull(sfc) )
			{
			(void) fprintf(stderr, "%s No guess field \"%s\" at \"%s\"\n",
							MyLabel, GuessField, vguess[nvt]);
			(void) fprintf(stderr, "%s      found for Allied Model \"%s %s\"\n",
							MyLabel, amodel, subarea);
			continue;
			}

		/* Create a PLOT Object to hold the warping data */
		plot = create_plot();
		add_subfld_to_plot(plot, PlotData, "float", NullItem);

		/* Initialize checks for input data */
		need_input = FALSE;
		have_input = FALSE;

		/* Extract the input data from each scattered point field */
		for ( nfld=0; nfld<fields->nfields; nfld++ )
			{

			/* Check for required field alias beginning with PlotField */
			/*  ... and also ensure that a sub-field name is given!    */
			if ( same_start(fields->aliases[nfld], PlotField)
					&& !blank(fields->sub_fields[nfld]) )
				{

				/* Set check for input data required */
				need_input = TRUE;

				/* Extract the SET Object for each PlotField */
				fdefp = fields->flds[nfld];
				if (DebugMode)
					{
					(void) printf("Initializing descriptin\n");
					(void) printf("   Map %g X %g\n",
								mproj->definition.xlen,
								mproj->definition.ylen);
					(void) printf("   Directory %s\n", datadir);
					(void) printf("   Source %s\n",
								fields->src_defs[nfld]->name);
					(void) printf("   Subsource %s\n",
								fields->sub_defs[nfld]->name);
					(void) printf("   Run time %s\n", rtime);
					(void) printf("   Element %s\n",
								fdefp->element->name);
					(void) printf("   Level %s\n", fdefp->level->name);
					(void) printf("   Valid time %s\n", vplot[nvt]);
					}
				(void) init_fld_descript(&descriptin);
				if ( !set_fld_descript(&descriptin,
								FpaF_MAP_PROJECTION, mproj,
								FpaF_DIRECTORY_PATH, datadir,
								FpaF_SOURCE,         fields->src_defs[nfld],
								FpaF_SUBSOURCE,      fields->sub_defs[nfld],
								FpaF_RUN_TIME,       rtime,
								FpaF_ELEMENT,        fdefp->element,
								FpaF_LEVEL,          fdefp->level,
								FpaF_VALID_TIME,     vplot[nvt],
								FpaF_END_OF_LIST) ) continue;

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
							"%s  Will try to use most recent data!\n",
							MyLabel);
					if (DebugMode)
						{
						(void) printf("Setting descriptin\n");
						(void) printf("   Run time %s\n", FpaCblank);
						}
					(void) set_fld_descript(&descriptin,
												FpaF_RUN_TIME, FpaCblank,
												FpaF_END_OF_LIST);
					}

				/* Extract the SPOT data from the metafile */
				spotset = retrieve_spotset(&descriptin);
				if ( IsNull(spotset) ) continue;

				/* Ensure that the units for each PlotField can be converted */
				/*  to units used by GuessField!                             */
				if ( !convert_value(fields->sub_units[nfld]->name, 1.0,
						fdef->element->elem_io->units->name, NullDouble) )
					{
					(void) fprintf(stderr, "%s Unmatched units \"%s\" and \"%s\"\n",
									MyLabel, fields->sub_units[nfld]->name,
									fdef->element->elem_io->units->name);
					spotset = destroy_set(spotset);
					continue;
					}

				/* Set check for input data available */
				have_input = TRUE;

				/* Extract the plot data from this SET Object */
				plot  = extract_plot_data(plot, spotset,
											fields->sub_fields[nfld],
											fields->sub_units[nfld]->name);
				spotset = destroy_set(spotset);
				}
			}

		/* Use the guess field if no input data required */
		if ( !need_input )
			{
			(void) fprintf(stdout,
							"%s Using guess field to create \"%s %s %s\"\n",
							MyLabel, fdesc.edef->name, fdesc.ldef->name,
							fdesc.vtime);
			}

		/* Do not output field if required input data is missing */
		else if ( need_input && !have_input )
			{
			(void) fprintf(stdout,
							"%s No input data to create \"%s %s %s\"\n",
							MyLabel, fdesc.edef->name, fdesc.ldef->name,
							fdesc.vtime);
			sfc  = destroy_surface(sfc);
			plot = destroy_plot(plot);
			continue;
			}

		/* Do not output field if no input data was found */
		else if ( need_input && have_input
					&& plot->numpts <= 0 && !create_anyway )
			{
			(void) fprintf(stdout,
							"%s No input data to create \"%s %s %s\"\n",
							MyLabel, fdesc.edef->name, fdesc.ldef->name,
							fdesc.vtime);
			sfc  = destroy_surface(sfc);
			plot = destroy_plot(plot);
			continue;
			}

		/* Use the guess field if no input data was found */
		/*  (and request made to create field anyways)    */
		else if ( need_input && have_input
					&& plot->numpts <= 0 && create_anyway )
			{
			(void) fprintf(stdout,
							"%s Using unwarped guess field to create \"%s %s %s\"\n",
							MyLabel, fdesc.edef->name, fdesc.ldef->name,
							fdesc.vtime);
			}

		/* Warp the guess field with PLOT Object data */
		else
			{

			(void) fprintf(stdout, "%s Warping field \"%s %s %s\" ",
							MyLabel, fdesc.edef->name, fdesc.ldef->name,
							fdesc.vtime);

			/* Initialize error parameters */
			last_mean_error = FPA_FLT_MAX;

			/* Initialize data arrays to hold all warping data */
			ndata = plot->numpts;
			wdata = INITMEM(float, plot->numpts);
			wpstn = INITMEM(POINT, plot->numpts);

			/* Set data values from PLOT Object data */
			iplt = which_plot_subfld(plot, PlotData);
			if ( iplt < 0 ) break;
			psub = plot->subs + iplt;
			for ( nn=0; nn<plot->numpts; nn++ )
				{
				wdata[nn] = psub->fval1[nn];
				copy_point(wpstn[nn], plot->pts[nn]);
				}

			/* Set data values for sparse data (to fix to guess field) */
			for ( iiy=0; iiy<Inumy; iiy++ )
				{
				for ( iix=0; iix<Inumx; iix++ )
					{
					if ( closest_plot_point(plot, Apstns[iiy][iix], &dist, pos)
							< 0 ) continue;
					if ( dist >= sparse_dist )
						{
						ndata++;
						wdata  = GETMEM(wdata, float, ndata);
						wpstn  = GETMEM(wpstn, POINT, ndata);
						valid  = eval_sfc(sfc, Apstns[iiy][iix], &sfcval);
						if ( valid )
							{
							wdata[ndata-1] = (float) sfcval;
							copy_point(wpstn[ndata-1], Apstns[iiy][iix]);
							}
						}
					}
				}

			/* Loop to warp guess field with data until field and data match */
			while ( TRUE )
				{

				/* Determine error parameters from PLOT Object data */
				mean_error = 0.0;
				for ( nn=0; nn<plot->numpts; nn++ )
					{
					valid = eval_sfc(sfc, wpstn[nn], &sfcval);
					if ( valid )
						{
						mean_error += fabs(wdata[nn] - (float) sfcval);
						}
					}

				/* Set error parameters */
				mean_error /= (float) ndata;
				avg_error   = (fabs(last_mean_error) + fabs(mean_error)) / 2.0;
				delta_error = fabs(last_mean_error) - fabs(mean_error);

				/* End when mean error is no longer decreasing, or when */
				/*  errors are within value defined by tension          */
				if ( delta_error <= 0.0
						|| fabs(delta_error) <= (avg_error / tension) ) break;

				/* Warp the guess field with the plot data */
				(void) sfit_surface(sfc, ndata, wpstn, wdata,
										influence, WgtFactor, TRUE, FALSE);
				last_mean_error = mean_error;
				(void) fprintf(stdout, ".");
				}

			(void) fprintf(stdout, " finished!\n");
			}

		/* Create a METAFILE Object to hold the field */
		meta = create_metafile();
		(void) define_mf_tstamp(meta, fdesc.rtime, fdesc.vtime);
		(void) define_mf_projection(meta, mproj);

		/* Move the warped SURFACE Object into the METAFILE Object */
		(void) add_sfc_to_metafile(meta, "a", fdesc.edef->name,
				fdesc.ldef->name, sfc);

		/* Set shuffle lock in base directory while outputting warped field */
		(void) strcpy(LockVtime, fdesc.vtime);
		if ( !set_file_lock(LockDir, LockVtime) )
			{
			(void) fprintf(stderr, "%s Unable to establish file lock\n",
							MyLabel);
			(void) fprintf(stderr, "%s      for Allied Model \"%s %s\"\n",
							MyLabel, amodel, subarea);
			(void) fprintf(stdout, "%s Aborted\n", MyLabel);
			return (-1);
			}
		Locked = TRUE;

		/* Construct old and new metafile names for METAFILE Object */
		/*  ... and remove any existing metafiles with these names! */
		fname = build_meta_filename(&fdesc);
		(void) remove_file(fname, NULL);
		fname = construct_meta_filename(&fdesc);
		(void) remove_file(fname, NULL);

		/* Construct old format metafile name if new format name not found */
		if ( blank(fname) ) fname = build_meta_filename(&fdesc);

		/* Output the METAFILE Object containing the warped SURFACE Object */
		(void) write_metafile(fname, meta, 4);
		nummeta++;

		/* Remove the current file lock in the base directory */
		(void) release_file_lock(LockDir, LockVtime);
		Locked = FALSE;

		/* Remove space used by METAFILE and PLOT Objects */
		meta = destroy_metafile(meta);
		plot = destroy_plot(plot);
		}

	/* Error message if no output fields created */
	if ( nummeta < 1 )
		{
		(void) fprintf(stderr, "%s No metafiles created\n", MyLabel);
		(void) fprintf(stderr, "%s      for Allied Model \"%s %s\"\n",
						MyLabel, amodel, subarea);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
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
*     e x t r a c t _ p l o t _ d a t a                                *
*                                                                      *
*     Extract data from SPOT Objects and store it in a PLOT Object.    *
*                                                                      *
***********************************************************************/

static	PLOT	extract_plot_data

	(
	PLOT		plot,		/* PLOT Object to add data to */
	SET			spotset,	/* SET Object containing SPOT Object input data */
	STRING		plotsub,	/* label for input data field */
	STRING		subunits	/* units for input data field */
	)

	{
	LOGICAL	valid;
	int		nn, npt;
	STRING	sval;
	double	val, valc;
	SPOT	sp;
	ATTRIB	*att;

	/* Return NullPlot if no PLOT Object passed */
	if ( IsNull(plot) ) return NullPlot;

	/* Return immediately if no input data */
	if ( IsNull(spotset) )                 return plot;
	if ( !same_ic(spotset->type, "spot") ) return plot;
	if ( spotset->num <= 0 )               return plot;

	/* Extract data from input SET Object                            */
	/* Note that the data is saved as type "float" in field PlotData */
	/*  and that the data is converted to units of PlotUnits!        */
	for ( nn=0; nn<spotset->num; nn++ )
		{

		/* Extract SPOT Object at this point */
		sp = (SPOT) spotset->list[nn];
		if ( IsNull(sp) ) continue;

		/* Extract the attribute value */
		att = get_attribute(sp->attrib, plotsub, &sval);
		if ( IsNull(att) ) continue;

		/* Check for numerical attribute values */
		if ( same_ic(sval, "N/A") ) continue;
		val = double_arg(sval, &valid);
		if ( !valid ) continue;

		/* Convert value to PlotUnits */
		(void) convert_value(subunits, val, PlotUnits, &valc);

		/* Add value to PLOT Object */
		npt = plot->numpts;
		add_point_to_plot(plot, sp->anchor);
		define_subfld_value(plot, PlotData, npt, NullString, NullString, 0, 0,
				(float) valc, (float) valc);
		}

	/* Return PLOT Object containing data */
	return plot;
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
		(void) fprintf(stdout, "    Removing file lock in directory \"%s\"\n",
						LockDir);
		(void) release_file_lock(LockDir, LockVtime);
		}
	exit(1);
	}
