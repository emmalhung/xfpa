/***********************************************************************
*                                                                      *
*     e x a m p l e _ e x t r a c t . c                                *
*                                                                      *
*     Template for creating ASCII data files from the FPA database     *
*     by running an "Allied Model".                                    *
*                                                                      *
*   Usage:  ex_extract  <setup_file>  (<optional_config_info>)         *
*                                                                      *
*     where  <setup_file>            is the local setup file name      *
*            <optional_config_info>  is information regarding fields   *
*                                     directories, and times           *
*                                                                      *
*     The <optional_config_info> can consist of any of the following:  *
*       allied name          - the FPA Allied Model name               *
*       subarea name         - the FPA Allied Model subarea name       *
*       run (or issue) time  - the date/time of the FPA directory      *
*       valid time(s)        - the valid date/time(s) for the field    *
*       source               - the FPA source name for input data      *
*       element(s)           - the FPA element name(s)                 *
*       level(s)             - the FPA level name(s)                   *
*       crossreference(s)    - the FPA Wind or Value crossreference    *
*                               name(s)                                *
*                                                                      *
*   Note that the "allied name", "subarea name", "run time",           *
*   "valid time", "source", "element", "level" and "crossreference"    *
*   information can be hard-coded (in a "static const STRING"          *
*   declaration), soft-coded as run string parameter (as argv()), or   *
*   set from an environment variable in the .fparc file.               *
*                                                                      *
*   In this example, the "allied name", "subarea name", "run time" and *
*   "valid time(s)" are soft-coded as run string parameters, while the *
*   "source", "element", "level" and "crossreference" names are passed *
*   through the "Allied Model" parameters.                             *
*                                                                      *
*   Note that the "source", "element", "level" and "crossreference"    *
*   names must correspond to names in the FPA configuration files.     *
*   If the data routine uses different names, you may have to set up   *
*   a table to identify the corresponding FPA names.                   *
*                                                                      *
*     Version 5 (c) Copyright 2000 Environment Canada (MSC)            *
*     Version 6 (c) Copyright 2001 Environment Canada (MSC)            *
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

/* FPA library definitions */
#include <fpa.h>

/* Standard library definitions */
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>

/********************************************************/
/********************************************************/
/** Set debug mode for local development (if required) **/
/********************************************************/
/********************************************************/

static	LOGICAL	DebugMode = FALSE;

/********************************************/
/********************************************/
/** Set program title (change as required) **/
/********************************************/
/********************************************/

static	const	STRING	MyTitle = "FPA ASCII Data Extraction";
static			char	MyLabel[MAX_BCHRS];


/* Trap for error situations */
static	void	error_trap();


/* Base directory shuffle and file lock information */
static	char	LockDir[MAX_BCHRS]   = "";
static	char	LockVtime[MAX_BCHRS] = "";
static	int		Locked               = FALSE;

/*************************************************************************/
/*************************************************************************/
/** Declarations for information from (or added to) configuration files **/
/*************************************************************************/
/*************************************************************************/

/* Allied Model name and subname from the "Sources" block of local Config */
static  const  STRING	AlliedName    = "EX_EXTRACT";
static  const  STRING	AlliedSubName = "";

/* Allied Model "required_fields" aliases from the "Sources" block */
static  const  STRING	SfcTemp       = "SfcTemp";

/* Allied Model "required_wind_crossrefs" aliases from the "Sources" block */
static  const  STRING	FpaWind       = "FpaWind";

/* Allied Model "files" aliases from the "Sources" block */
static  const  STRING	OutputSfcTemp = "OutputSfcTemp";
static  const  STRING	OutputWind    = "OutputWind";

/* Units names from the "Units" block of local Config */
static  const  STRING	DegreesF      = "degreesF";
static  const  STRING	Knots         = "knots";


/******************************************************************/
/******************************************************************/
/** Declarations for information used in sampling map projection **/
/******************************************************************/
/******************************************************************/

/* Sampling Projection information */
static  const  STRING	ProjName = "lambert_conformal";
static  const  STRING	Ref1     = "39:00N";
static  const  STRING	Ref2     = "39:00N";
static  const  STRING	Ref3     = "";
static  const  STRING	Ref4     = "";
static  const  STRING	Ref5     = "";

/* Sampling Map Definition information */
static  const  STRING	Olat     = "39:00N";
static  const  STRING	Olon     = "96:00W";
static  const  STRING	Rlon     = "96:00W";
static  const  float	Xmin     = -2400.0;
static  const  float	Ymin     = -1500.0;
static  const  float	Xmax     =  2400.0;
static  const  float	Ymax     =  1500.0;
static  const  float	MapUnits =  1000.0;

/* Sampling Grid Definition information */
static  const  float	Glen     = 100.0;
static  const  float	XGlen    = 100.0;
static  const  float	YGlen    = 100.0;

/***********************************************************************
*                                                                      *
*     m a i n                                                          *
*                                                                      *
***********************************************************************/

int		main(argc, argv)

int		argc;
STRING	argv[];
	{
	int			status, nslist, numvts, nvt, ntemps, nwinds;
	int			cyear, cjday, cmonth, cmday, chour, cmin, csec;
	LOGICAL		local;
	int			iyear, ijday, imonth, imday, ihour, imin, iplushr;
	STRING		setupfile, *slist;
	MAP_PROJ	*tmproj;
	float		clon;
	STRING		homedir, odir;
	STRING		allied_model, sub_area;
	STRING		timestring, rtime, *vtimes;

	FpaConfigSourceStruct	*sdef;
	FpaConfigSourceAlliedStruct	*allied;
	FpaConfigAlliedFilesStruct	*files;
	FpaConfigAlliedFieldsStruct	*fields;
	FpaConfigAlliedWindsStruct	*winds;
	FpaConfigAlliedValuesStruct	*values;

	int						nx, ny;
	MAP_PROJ				smproj;
	PROJ_DEF				projection;
	MAP_DEF					mapdef;
	GRID_DEF				grid;
	FLD_DESCRIPT			fdescin, fdescout;

	int						ii, iix, iiy;
	float					mapx, mapy, *lats, *lons;
	POINT					pos, *ppos;

	int						loct, locw;
	FLD_DESCRIPT			descript;
	float					*tmps, *dirs, *spds, *gsts;
	STRING					tunits, sunits;
	double					dval;

	STRING					fname;
	char					filename[MAX_BCHRS];
	char					basedir[MAX_BCHRS];
	LOGICAL					created;
	FILE					*FP_Out;

	/* Ignore hangup, interrupt and quit signals */
	/*  ... so we can survive after logging off  */
	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	status = setvbuf(stdout, NullString, _IOLBF, 0);
	status = setvbuf(stderr, NullString, _IOLBF, 0);

	/* Set debug mode (if requested) */
	if ( DebugMode ) (void) pr_control(NULL, 5, 1);
	else             (void) pr_control(NULL, 1, 1);

	/**************************************************************************/
	/**************************************************************************/
	/** Validate the run string parameters                                   **/
	/**                                                                      **/
	/** Note that this example contains at least five "optional_config_info" **/
	/**  parameters, or a total of six or more run string parameters         **/
	/**************************************************************************/
	/**************************************************************************/

	/* Check for correct number of run string parameters */
	if ( argc < 6 )
		{
		(void) fprintf(stderr, "Usage:\n");
		(void) fprintf(stderr, "  example_extract  <setup_file>");
		(void) fprintf(stderr, "  <allied_model>  <sub_area>");
		(void) fprintf(stderr, "  <run_time>  <valid_time(s)>\n\n");
		return (-1);
		}

	/**************************************************/
	/**************************************************/
	/** Initial startup (license and error messages) **/
	/**************************************************/
	/**************************************************/

	/* Obtain a licence */
	(void) fpalib_license(FpaAccessLib);

	/* Trap all signals that would abort the process by default */
	(void) set_error_trap(error_trap);

	/* Startup message */
	(void) sprintf(MyLabel, "[%d] %s:", getpid(), MyTitle);
	(void) systime(&cyear, &cjday, &chour, &cmin, &csec);
	(void) mdate(&cyear, &cjday, &cmonth, &cmday);
	(void) fprintf(stdout, "%s Beginning: %d/%.2d/%.2d %.2d:%.2d:%.2d GMT\n",
			MyLabel, cyear, cmonth, cmday, chour, cmin, csec);

	/********************************************************************/
	/********************************************************************/
	/** Get setup file from run string and initialize FPA setup/config **/
	/********************************************************************/
	/********************************************************************/

	/* Set run string parameter for setup file */
	setupfile = strdup(argv[1]);

	/* Read the setup file */
	/* This moves to standard FPA directory */
	nslist = setup_files(setupfile, &slist);
	if ( !define_setup(nslist, slist) )
		{
		(void) fprintf(stderr, "%s Problem with setup file \"%s\"\n",
				MyLabel, setupfile);
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

	/* Retrieve the target projection definition */
	tmproj = get_target_map();
	if ( !tmproj )
		{
		(void) fprintf(stderr, "%s Target map not defined", MyLabel);
		(void) fprintf(stderr, " in setup file \"%s\"\n", setupfile);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Set center longitude from target projection definition */
	if ( !grid_center(tmproj, NULL, NULL, &clon) )
		{
		(void) fprintf(stderr, "%s Problem with center lat/long\n", MyLabel);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Set base directory for this FPA database */
	homedir = strdup(home_directory());

	/*************************************************************************/
	/*************************************************************************/
	/** Get the Allied Model information here!                              **/
	/**                                                                     **/
	/** Information for the Allied Model is returned by a call to           **/
	/**                                                                     **/
	/**    STRING  allied_model, sub_area;                                  **/
	/**    FpaConfigSourceStruct  *sdef;                                    **/
	/**                                                                     **/
	/**    sdef = get_source_info(allied_model, sub_area);                  **/
	/**                                                                     **/
	/**  where the "sub_area" is for an Allied Model that can be used over  **/
	/**  a number of different geographic areas.                            **/
	/**                                                                     **/
	/** The get_source_info() routine returns an FpaConfigSourceStruct      **/
	/**  structure (defined in $FPA/lib/environ/config_structs.h) which     **/
	/**  contains the Allied Model information from the local Config file.  **/
	/**                                                                     **/
	/** Like all FPA "Sources", the Allied Model identifies a particular    **/
	/**  directory, and this directory is the default directory for the     **/
	/**  Allied Model output files.  The "directory_tag", "directory_path", **/
	/**  and "sub_directory_path" keywords in the local configuration file  **/
	/**  identify the location of this directory.  (Note that output files  **/
	/**  can be sent to another directory by setting the "directory_tag"    **/
	/**  in the "files" block of the "allied_model" block to a tag          **/
	/**  identified in the "directories" block of the local setup file.)    **/
	/**                                                                     **/
	/** The default directory for the Allied Model input files is set       **/
	/**  by the "source_info" keyword in the "allied_model" block.  (Note   **/
	/**  that input files from another directory are identified by setting  **/
	/**  the "source_info" keyword in the "required_fields" or              **/
	/**  "required_wind_crossrefs" or "required_value_crossrefs" blocks.)   **/
	/*************************************************************************/
	/*************************************************************************/

	/********************************************************/
	/********************************************************/
	/** For this example, "allied name" and "subarea name" **/
	/**  are run string parameters.                        **/
	/********************************************************/
	/********************************************************/

	/* Set run string parameter for Allied Model */
	allied_model = strdup(argv[2]);
	sub_area     = strdup(argv[3]);

	/* Get Allied Model information */
	sdef = get_source_info(allied_model, sub_area);
	if ( IsNull(sdef) )
		{
		(void) fprintf(stderr, "%s Error finding Allied Model \"%s %s\"\n",
				MyLabel, allied_model, sub_area);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}
	allied = sdef->allied;
	if ( IsNull(allied) )
		{
		(void) fprintf(stderr, "%s Missing Allied Model data for \"%s %s\"\n",
				MyLabel, allied_model, sub_area);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}
	files  = sdef->allied->files;
	fields = sdef->allied->fields;
	winds  = sdef->allied->winds;
	values = sdef->allied->values;

	/*************************************************************************/
	/*************************************************************************/
	/** Set the run (or issue) time and valid time(s) here!                 **/
	/**                                                                     **/
	/** The format of a run time ("rtime") or valid time ("vtime") can be   **/
	/**  checked by a call to                                               **/
	/**                                                                     **/
	/**    STRING  rtime, vtime;                                            **/
	/**    float   clon;                                                    **/
	/**                                                                     **/
	/**    rtime = interpret_timestring(<input string>, NullString, clon);  **/
	/**    vtime = interpret_timestring(<input string>, rtime, clon);       **/
	/**                                                                     **/
	/**  where "clon" is the center longitude (used for local times)        **/
	/**                                                                     **/
	/** The run time (or valid time) can be created by a call to            **/
	/**                                                                     **/
	/**    rtime = build_tstamp(iyear, ijday, ihour, imin, local, mins);    **/
	/**                                                                     **/
	/**  where the year ("iyear"), julian day ("ijday"), hour of day        **/
	/**  ("ihour"), and minute ("imin") are integers, and the local time    **/
	/**  parameter ("local") is usually FALSE, that is, a GMT based time.   **/
	/**  Also, the use minutes flag (mins) is usually set to FALSE.         **/
	/**                                                                     **/
	/** A copy of the run time (or valid time) can be saved by a call to    **/
	/**                                                                     **/
	/**    rtime = strdup(build_tstamp(iyear, ijday, ihour, imin, local,    **/
	/**                                mins));                              **/
	/**                                                                     **/
	/** The valid time can also be determined from the run time and an      **/
	/**  additional number of hours ("iplushr") by a call to                **/
	/**                                                                     **/
	/**    vtime = calc_valid_time(rtime, iplushr);                         **/
	/**                                                                     **/
	/** The julian day ("ijday") can be created from the month and day      **/
	/**  ("imonth" and "imday") by a call to                                **/
	/**                                                                     **/
	/**    (void) jdate(&iyear, &imonth, &imday, &ijday);                   **/
	/**                                                                     **/
	/**  or vice versa, by a call to                                        **/
	/**                                                                     **/
	/**    (void) mdate(&iyear, &ijday, &imonth, &imday);                   **/
	/**                                                                     **/
	/** The year and julian day (or month and day) can be normalized (if    **/
	/**  one adds or subtracts from the day, for example) by a call to      **/
	/**                                                                     **/
	/**    (void) jnorm(&iyear, &ijday);                                    **/
	/**    (void) mnorm(&iyear, &imonth, &imday);                           **/
	/**                                                                     **/
	/** The hour ("ihour") can also be normalized by a call to              **/
	/**                                                                     **/
	/**    (void) tnorm(&iyear, &ijday, &ihour, NullInt, NullInt);          **/
	/**                                                                     **/
	/*************************************************************************/
	/*************************************************************************/

	/**********************************************************/
	/**********************************************************/
	/** For this example, "run time" and "valid time(s)" are **/
	/**  run string parameters.                              **/
	/**********************************************************/
	/**********************************************************/

	/* Set run time from a run string parmater */
	timestring = interpret_timestring(argv[4], NullString, clon);
	if ( IsNull(timestring) )
		{
		(void) fprintf(stderr, "%s Problem with run time: \"%s\"\n",
				MyLabel, argv[4]);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}
	rtime = strdup(timestring);

	/* Determine number of valid times */
	numvts = argc - 5;
	vtimes = INITMEM(STRING, numvts);

	/* Set valid time(s) from run string parmaters */
	for ( nvt=0; nvt<numvts; nvt++ )
		{
		timestring = interpret_timestring(argv[5+nvt], rtime, clon);
		if ( IsNull(timestring) )
			{
			(void) fprintf(stderr, "%s Problem with valid time: \"%s\"\n",
					MyLabel, argv[5+nvt]);
			(void) fprintf(stdout, "%s Aborted\n", MyLabel);
			return (-1);
			}
		vtimes[nvt] = strdup(timestring);
		}

	/**********************************************************************/
	/**********************************************************************/
	/** Initialize FLD_DESCRIPT Objects for input and output files here! **/
	/**                                                                  **/
	/** The FLD_DESCRIPT Object contains information used to identify a  **/
	/**  file in an FPA database.  (The FLD_DESCRIPT Object is defined   **/
	/**  in $FPA/lib/environ/files_and_directories.h.)                   **/
	/**                                                                  **/
	/** The FLD_DESCRIPT Object is initialized and set by calls to       **/
	/**                                                                  **/
	/**    LOGICAL       valid;                                          **/
	/**    FLD_DESCRIPT  fdesc, fdesc_orig;                              **/
	/**                                                                  **/
	/**    (void) init_fld_descript(&fdesc);                             **/
	/**      or                                                          **/
	/**    (void) copy_fld_descript(&fdesc, &fdesc_orig);                **/
	/**                                                                  **/
	/**    valid = set_fld_descript(&fdesc, <type>, <value>,             **/
	/**                                       FpaF_END_OF_LIST);         **/
	/**                                                                  **/
	/**  where the <type> and <value> describe various aspects of the    **/
	/**  Object structure.  For example, the <type> FpaF_SOURCE refers   **/
	/**  to a <value> FpaConfigSourceStruct structure, wheras the        **/
	/**  <type> FpaF_SOURCE_NAME refers to a <value> STRING (a name).    **/
	/**                                                                  **/
	/** A full file description requires an FpaF_SOURCE (and SUB_SOURCE) **/
	/**  and an FpaF_RUN_TIME to identify the data directory, an         **/
	/**  FpaF_ELEMENT, FpaF_LEVEL, and FpaF_VALID_TIME to identify the   **/
	/**  filename, an FpaF_MAP_PROJECTION to define the map projection   **/
	/**  to sample from, and an FpaF_WIND_FUNCTION_NAME or an            **/
	/**  FpaF_VALUE_FUNCTION_NAME to define what to sample.              **/
	/**                                                                  **/
	/** The FLD_DESCRIPT Object set with the Allied Model name can be    **/
	/**  used to initialize the default Allied Model output directory    **/
	/**  by a call to                                                    **/
	/**                                                                  **/
	/**    STRING        dirname;                                        **/
	/**    FLD_DESCRIPT  fdesc;                                          **/
	/**                                                                  **/
	/**    dirname = prepare_source_directory(&fdesc);                   **/
	/**                                                                  **/
	/**  which returns the name of the default output directory, as well **/
	/**  as handling any file shuffling that may be required.            **/
	/**********************************************************************/
	/**********************************************************************/

	/* Initialize the field descriptor for input files */
	/*  using the default directory for input files    */
	(void) init_fld_descript(&fdescin);
	if ( !set_fld_descript(&fdescin,
							FpaF_MAP_PROJECTION, tmproj,
							FpaF_DIRECTORY_PATH, homedir,
							FpaF_SOURCE,         allied->src_def,
							FpaF_SUBSOURCE,      allied->sub_def,
							FpaF_END_OF_LIST) )
		{
		(void) fprintf(stderr, "%s Problem initializing field descriptor",
				MyLabel);
		(void) fprintf(stderr, " for \"%s %s\"\n",
				allied->src_def->name, allied->sub_def->name);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Initialize the field descriptor for output files */
	(void) init_fld_descript(&fdescout);
	if ( !set_fld_descript(&fdescout,
							FpaF_MAP_PROJECTION, tmproj,
							FpaF_DIRECTORY_PATH, homedir,
							FpaF_SOURCE_NAME,    AlliedName,
							FpaF_SUBSOURCE_NAME, AlliedSubName,
							FpaF_RUN_TIME,       rtime,
							FpaF_END_OF_LIST) )
		{
		(void) fprintf(stderr, "%s Problem initializing field descriptor",
				MyLabel);
		(void) fprintf(stderr, " for \"%s %s\"  at \"%s\"\n",
				AlliedName, AlliedSubName, rtime);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Prepare data directory for output */
	odir = prepare_source_directory(&fdescout);
	if ( blank(odir) )
		{
		(void) fprintf(stderr, "%s Problem preparing data directory", MyLabel);
		(void) fprintf(stderr, " for \"%s %s\"  at \"%s\"\n",
				AlliedName, AlliedSubName, rtime);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}
	(void) strcpy(basedir, odir);

	/***********************************************************************/
	/***********************************************************************/
	/** Set the new map projection for sampling here!                     **/
	/**                                                                   **/
	/** The default map projection information is defined by              **/
	/**                                                                   **/
	/**    MAP_PROJ  *tmproj;                                             **/
	/**                                                                   **/
	/**  which in turn contains                                           **/
	/**                                                                   **/
	/**    PROJ_DEF  projection                                           **/
	/**    MAP_DEF   mapdef                                               **/
	/**    GRID_DEF  grid                                                 **/
	/**                                                                   **/
	/** The projection can be modified by a call to                       **/
	/**                                                                   **/
	/**    LOGICAL  valid;                                                **/
	/**    STRING   type, ref1, ref2, ref3, ref4, ref5;                   **/
	/**                                                                   **/
	/**    valid = define_projection_by_name(&projection, type, ref1,     **/
	/**                                        ref2, ref3, ref4, ref5);   **/
	/**                                                                   **/
	/**  where the parameters are described in the "projection" section   **/
	/**  of the FPA Graphics Metafile Standard                            **/
	/**                                                                   **/
	/** The map definition can be modified by a call to                   **/
	/**                                                                   **/
	/**    LOGICAL  valid;                                                **/
	/**    STRING  olat, olon, rlon;                                      **/
	/**    float   xmin, ymin, xmax, ymax, map_units;                     **/
	/**                                                                   **/
	/**    valid = define_map_def(&mapdef, olat, olon, rlon,              **/
	/**                             xmin, ymin, xmax, ymax, map_units);   **/
	/**                                                                   **/
	/**  where the parameters are described in the "mapdef" section       **/
	/**  of the FPA Graphics Metafile Standard                            **/
	/**                                                                   **/
	/** The sampling grid can be modified by a call to                    **/
	/**                                                                   **/
	/**    LOGICAL  valid;                                                **/
	/**    int    nx, ny;                                                 **/
	/**    float  glen, xglen, yglen, map_units;                          **/
	/**                                                                   **/
	/**    valid = define_grid_def(&grid, nx, ny, glen, xglen, yglen,     **/
	/**                              map_units);                          **/
	/**                                                                   **/
	/**  where "glen", "xglen", "yglen" are the grid spacing (usually     **/
	/**  the same value), "map_units" is the same as in define_map_def(), **/
	/**  and "nx" and "ny" are based on "xmin", "xmax", "xglen" and       **/
	/**  "ymin", "ymax", "yglen" respectively.                            **/
	/**                                                                   **/
	/** The new map projection is then respecified by a call to           **/
	/**                                                                   **/
	/**    LOGICAL   valid;                                               **/
	/**    MAP_PROJ  smproj;                                              **/
	/**                                                                   **/
	/**    valid = define_map_projection(&smproj,                         **/
	/**                                    &projection, &mapdef, &grid);  **/
	/***********************************************************************/
	/***********************************************************************/

	/******************************************************************/
	/******************************************************************/
	/** For this example, the projection is set to Lambert Conformal **/
	/**  and the map is centered on the United States of America.    **/
	/******************************************************************/
	/******************************************************************/

	/* Define the projection */
	if ( !define_projection_by_name(&projection, ProjName, Ref1, Ref2,
			Ref3, Ref4, Ref5) )
		{
		(void) fprintf(stderr, "%s Unrecognized projection type ... %s",
				MyLabel, ProjName);
		(void) fprintf(stderr, "  or parameters ... %s %s %s %s %s",
				Ref1, Ref2, Ref3, Ref4, Ref5);
		(void) fprintf(stderr, " for \"%s %s\"  at \"%s\"\n",
				AlliedName, AlliedSubName, rtime);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Define the map definition */
	if ( !define_map_def(&mapdef, Olat, Olon, Rlon,
			Xmin, Ymin, Xmax, Ymax, MapUnits) )
		{
		(void) fprintf(stderr, "%s Unrecognized olat/olon/rlon ... %s %s %s",
				MyLabel, Olat, Olon, Rlon);
		(void) fprintf(stderr, "  or xmin/ymin/xmax/ymax ... %6.0f %6.0f %6.0f %6.0f",
				Xmin, Ymin, Xmax, Ymax);
		(void) fprintf(stderr, "  or map_units ... %6.0f", MapUnits);
		(void) fprintf(stderr, " for \"%s %s\"  at \"%s\"\n",
				AlliedName, AlliedSubName, rtime);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Define the sampling grid */
	nx = NINT((Xmax - Xmin) / XGlen) + 1;
	ny = NINT((Ymax - Ymin) / YGlen) + 1;
	if ( !define_grid_def(&grid, nx, ny, Glen, XGlen, YGlen, MapUnits) )
		{
		(void) fprintf(stderr, "%s Unrecognized nx/ny ... %d %d",
				MyLabel, nx, ny);
		(void) fprintf(stderr, "  or glen/xglen/yglen ... %6.0f %6.0f %6.0f",
				Glen, XGlen, YGlen);
		(void) fprintf(stderr, "  or map_units ... %6.0f", MapUnits);
		(void) fprintf(stderr, " for \"%s %s\"  at \"%s\"\n",
				AlliedName, AlliedSubName, rtime);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Define the map projection */
	(void) define_map_projection(&smproj, &projection, &mapdef, &grid);

	/********************************************************************/
	/********************************************************************/
	/** Set the sampling points here!                                  **/
	/**                                                                **/
	/** Note that we convert the sampling points to positions on the   **/
	/**  FPA map projection, to avoid reprojection of the FPA fields.  **/
	/**                                                                **/
	/** Sampling points in latitude/longitude are set by a call to     **/
	/**                                                                **/
	/**    LOGICAL   valid;                                            **/
	/**    MAP_PROJ  *mproj;                                           **/
	/**    float     lat, lon;                                         **/
	/**    POINT     pos;                                              **/
	/**                                                                **/
	/**    valid = ll_to_pos(mproj, lat, lon, pos);                    **/
	/**                                                                **/
	/** Sampling points in map coordinates on a new projection (and    **/
	/**  the corresponding latitude/longitude) are set by calls to     **/
	/**                                                                **/
	/**    LOGICAL   valid;                                            **/
	/**    MAP_PROJ  *mproj;                                           **/
	/**    MAP_PROJ  new_mproj;                                        **/
	/**    POINT     pos_on_new, pos;                                  **/
	/**    float     lat, lon;                                         **/
	/**                                                                **/
	/**    valid = pos_to_pos(&new_mproj, pos_on_new, mproj, pos);     **/
	/**    valid = pos_to_ll(&new_mproj, pos_on_new, &lat, &lon);      **/
	/**                                                                **/
	/** Note that map coordinates within the FPA are always calculated **/
	/**  from the lower left corner of the FPA map projection (0,0).   **/
	/********************************************************************/
	/********************************************************************/

	/* Get memory to hold the sampling locations */
	ppos = INITMEM(POINT, ny*nx);
	lats = INITMEM(float, ny*nx);
	lons = INITMEM(float, ny*nx);

	/* Loop to set sampling locations ... based on map locations       */
	/* The FPA map coordinates are 0 to (Xmax-Xmin) from left to right */
	/*  and 0 to (Ymax-Ymin) from bottom to top                        */
	/* The sampling is done top to bottom for easier ASCII output!     */
	mapy = Ymax - Ymin;
	for ( iiy=0; iiy<ny; iiy++ )
		{
		mapx = 0.0;
		for ( iix=0; iix<nx; iix++ )
			{
			ii = iiy*nx + iix;
			ppos[ii][X] = mapx;
			ppos[ii][Y] = mapy;
			mapx += XGlen;
			}
		mapy -= YGlen;
		}

	/* Loop to convert sampling locations on the sampling map projection */
	/*  to locations on the default FPA map projection                   */
	/* Get latitudes and longitudes at the same time!                    */
	for ( ii=0; ii<ny*nx; ii++ )
		{
		(void) copy_point(pos, ppos[ii]);
		(void) pos_to_pos(&smproj, pos, tmproj, ppos[ii]);
		(void) pos_to_ll(&smproj, pos, &lats[ii], &lons[ii]);
		}

	/************************************************************************/
	/************************************************************************/
	/** Extract the FPA data and output the ASCII files here!              **/
	/**                                                                    **/
	/** Note that we make use of the Allied Model structures for           **/
	/**  "required_fields", "required_wind_crossrefs" or                   **/
	/**  "required_value_crossrefs" to identify the FPA fields that must   **/
	/**  be sampled to extract the required information.  The Allied       **/
	/**  Model structures contain "source" information, and either         **/
	/**  "element" and "level" information, or the name of a wind or value **/
	/**  crossreference (from the "CrossRefs" block of a config file.)     **/
	/**                                                                    **/
	/** Values from continuous fields can be sampled by a call to          **/
	/**                                                                    **/
	/**    LOGICAL       valid;                                            **/
	/**    FLD_DESCRIPT  fdesc;                                            **/
	/**    int           npos;                                             **/
	/**    POINT         *ppos;                                            **/
	/**    float         clon, *vals;                                      **/
	/**    STRING        vunits, inunits, equation;                        **/
	/**                                                                    **/
	/**    vals = INITMEM(float, npos);                                    **/
	/**                                                                    **/
	/**    valid = extract_surface_value(1, &fdesc,                        **/
	/**                 <time matching>, npos, ppos, clon, vals, &vunits)  **/
	/**      or                                                            **/
	/**    valid = extract_surface_value_by_equation(inunits, equation,    **/
	/**                 &fdesc, <time matching>, npos, ppos, clon,         **/
	/**                 vals, &vunits)                                     **/
	/**      or                                                            **/
	/**    valid = extract_surface_value_by_crossref(<valuecref>, &fdesc,  **/
	/**                 <time matching>, npos, ppos, clon, vals, &vunits)  **/
	/**                                                                    **/
	/**  where <time matching> is set from the Allied Model ("True" will   **/
	/**  find the closest time to the valid time requested), the field is  **/
	/**  sampled at "npos" positions (given by the "ppos" array), and the  **/
	/**  values returned in the "vals" array with units of "vunits".       **/
	/**  (Note that the second routine allows sampling using an "equation" **/
	/**  string expressed in units of "inunits", and the third routine     **/
	/**  allows sampling based on <valuecref>, a value crossreference      **/
	/**  from the "CrossRefs" block of a configuration file.)              **/
	/**                                                                    **/
	/** Values from wind fields can be sampled by a call to                **/
	/**                                                                    **/
	/**    LOGICAL       valid;                                            **/
	/**    FLD_DESCRIPT  fdesc;                                            **/
	/**    int           npos;                                             **/
	/**    POINT         *ppos;                                            **/
	/**    float         clon, *dirs, *spds, *gsts;                        **/
	/**    STRING        sunits;                                           **/
	/**                                                                    **/
	/**    dirs = INITMEM(float, npos);                                    **/
	/**    spds = INITMEM(float, npos);                                    **/
	/**    gsts = INITMEM(float, npos);                                    **/
	/**                                                                    **/
	/**    valid = extract_awind(1, &fdesc, <time_matching>,               **/
	/**                     npos, ppos, clon, dirs, spds, gsts, &sunits)   **/
	/**      or                                                            **/
	/**    valid = extract_awind_by_crossref(<windcref>, &fdesc,           **/
	/**                     <time_matching>, npos, ppos, clon,             **/
	/**                     dirs, spds, gsts, &sunits)                     **/
	/**                                                                    **/
	/**  where <time matching> is set from the Allied Model ("True" will   **/
	/**  find the closest time to the valid time requested), the field is  **/
	/**  sampled at "npos" positions (given by the "ppos" array), the      **/
	/**  directions returned in the "dirs" array (directions are always    **/
	/**  returned in degrees true), and the speeds and gusts returned in   **/
	/**  the "spds" and "gsts" arrays with units of "sunits".              **/
	/**  (Note that the second routine allows sampling based on            **/
	/**  <windcref>, a wind crossreference from the "CrossRefs" block of   **/
	/**  a configuration file.)                                            **/
	/**                                                                    **/
	/** Note that values extracted from continuous fields or wind speeds   **/
	/**  and gusts extracted from wind fields can be converted from the    **/
	/**  default units to another related units by a call to               **/
	/**                                                                    **/
	/**    LOGICAL  valid;                                                 **/
	/**    STRING   units, new_units;                                      **/
	/**    float    value;                                                 **/
	/**    double   dval;                                                  **/
	/**                                                                    **/
	/**    valid = convert_value(units, (double) value, new_units, &dval); **/
	/**    value = (float) dval;                                           **/
	/**                                                                    **/
	/** Values from discrete (area) fields can be sampled by a call to     **/
	/**                                                                    **/
	/**    LOGICAL       valid;                                            **/
	/**    FLD_DESCRIPT  fdesc;                                            **/
	/**    int           npos;                                             **/
	/**    POINT         *ppos;                                            **/
	/**    CAL           *cals;                                            **/
	/**                                                                    **/
	/**    cals = INITMEM(CAL, npos);                                      **/
	/**                                                                    **/
	/**    valid = extract_areaset_attribs(1, &fdesc,                      **/
	/**                          <time_matching>, npos, ppos, clon, cals)  **/
	/**      or                                                            **/
	/**    valid = extract_areaset_attribs_by_crossref(valuecref, &fdesc,  **/
	/**                          <time_matching>, npos, ppos, clon, cals)  **/
	/**                                                                    **/
	/**  where <time matching> is set from the Allied Model ("True" will   **/
	/**  find the closest time to the valid time requested), the field     **/
	/**  is sampled at "npos" positions (given by the "ppos" array), and   **/
	/**  and the discrete area values returned in the "cals" array (an     **/
	/**  array of CAL structures.                                          **/
	/**  (Note that the second routine allows sampling based on            **/
	/**  <valuecref>, a value crossreference from the "CrossRefs" block    **/
	/**  of a configuration file.)                                         **/
	/**                                                                    **/
	/** Each CAL structure contains a set of named attributes, which can   **/
	/**  be checked for or extracted by calls to                           **/
	/**                                                                    **/
	/**    LOGICAL  valid;                                                 **/
	/**    CAL      cal;                                                   **/
	/**    STRING   value;                                                 **/
	/**                                                                    **/
	/**    valid = CAL_has_attribute(cal, <attribute_name>);               **/
	/**    value = CAL_get_attribute(cal, <attribute_name>);               **/
	/**                                                                    **/
	/**  where <attribute_name> is the name of the desired attribute.      **/
	/**                                                                    **/
	/** The full pathname for the output files are created by a call to    **/
	/**                                                                    **/
	/**    STRING        filename;                                         **/
	/**    FLD_DESCRIPT  fdesc;                                            **/
	/**                                                                    **/
	/**    filename = build_allied_filename(&fdesc, FpaC_ALLIED_FILES,     **/
	/**                                                    <file_alias>);  **/
	/**                                                                    **/
	/**  where the <file_alias> is from the "files" block of the Allied    **/
	/**  Model.  This will generate a filename in the default Allied Model **/
	/**  directory (which will have been created previously with a call    **/
	/**  to the prepare_source_directory() routine), or to another         **/
	/**  directory identified by a "directory_tag" keyword in the "files"  **/
	/**  block of the Allied Model in the local configuration file.        **/
	/**                                                                    **/
	/** The full pathname for output files can also be created directly,   **/
	/**  or by appending to the directory created by the call to the       **/
	/**  prepare_source_directory() routine.                               **/
	/**                                                                    **/
	/** In any case, a data directory that does not already exist will be  **/
	/**  automatically created by the routine create_directory();          **/
	/************************************************************************/
	/************************************************************************/

	/**********************************************/
	/**********************************************/
	/** Loop to extract surface temperature data **/
	/**********************************************/
	/**********************************************/

	/* Loop on number of valid times for data requested */
	for ( ntemps=0, nvt=0; nvt<numvts; nvt++ )
		{

		/********************************************/
		/********************************************/
		/** Extract surface temperature data here! **/
		/********************************************/
		/********************************************/

		/* Check for surface temperature in the "required_fields" block */
		loct = source_allied_data_location(sdef, FpaC_ALLIED_FIELDS, SfcTemp);
		if ( loct < 0 )
			{
			(void) fprintf(stdout, "%s Unknown", MyLabel);
			(void) fprintf(stdout, " \"required_fields\" file \"%s\"\n",
					SfcTemp);
			continue;
			}

		/* Set field descriptor for extracting the surface temperature data */
		/* Note that the "element", "level", and "source" information is    */
		/*  taken from the Allied Model "fields" structure!                 */
		(void) copy_fld_descript(&descript, &fdescin);
		if ( !set_fld_descript(&descript,
						FpaF_VALUE_FUNCTION_NAME, FpaDefaultValueFunc,
						FpaF_ELEMENT,             fields->flds[loct]->element,
						FpaF_LEVEL,               fields->flds[loct]->level,
						FpaF_SOURCE,              fields->src_defs[loct],
						FpaF_SUBSOURCE,           fields->sub_defs[loct],
						FpaF_RUN_TIME,            rtime,
						FpaF_VALID_TIME,          vtimes[nvt],
						FpaF_END_OF_LIST) )
			{
			(void) fprintf(stdout, "%s Problem setting field descriptor",
					MyLabel);
			(void) fprintf(stdout, " for \"%s\" at \"%s\"",
					fields->flds[loct]->element->name,
					fields->flds[loct]->level->name);
			(void) fprintf(stdout, "  from \"%s %s\" at \"%s %s\"\n",
					fields->src_defs[loct]->name, fields->sub_defs[loct]->name,
					rtime, vtimes[nvt]);
			continue;
			}

		/* Extract the surface temperature data */
		tmps = INITMEM(float, ny*nx);
		if ( !extract_surface_value(1, &descript, allied->time_match,
				ny*nx, ppos, clon, tmps, &tunits) )
			{
			(void) fprintf(stdout, "%s Cannot extract data", MyLabel);
			(void) fprintf(stdout, " for \"%s\" at \"%s\"",
					fields->flds[loct]->element->name,
					fields->flds[loct]->level->name);
			(void) fprintf(stdout, "  from \"%s %s\" at \"%s %s\"\n",
					fields->src_defs[loct]->name, fields->sub_defs[loct]->name,
					rtime, vtimes[nvt]);
			continue;
			}

		/* Check that surface temperature can be converted to output units */
		if ( !convert_value(tunits, 0.0, DegreesF, NullDouble) )
			{
			(void) fprintf(stdout, "%s Cannot convert \"%s\" data",
					MyLabel, fields->flds[loct]->element->name);
			(void) fprintf(stdout, " from \"%s\" to \"%s\"\n",
					tunits, DegreesF);
			continue;
			}

		/* Convert the surface temperature data for output */
		for ( ii=0; ii<ny*nx; ii++ )
			{
			(void) convert_value(tunits, (double) tmps[ii], DegreesF, &dval);
			tmps[ii] = (float) dval;
			}

		/******************************************************************/
		/******************************************************************/
		/** Ouput file containing sampled surface temperature data here! **/
		/******************************************************************/
		/******************************************************************/

		/* Get the output filename for the surface temperature data */
		fname = build_allied_filename(&fdescout, FpaC_ALLIED_FILES,
																OutputSfcTemp);
		if ( blank(fname) )
			{
			(void) fprintf(stdout, "%s Cannot build pathname for", MyLabel);
			(void) fprintf(stdout, " \"files\" file \"%s\"\n", OutputSfcTemp);
			continue;
			}

		/* Append the valid time to the filename */
		(void) strcpy(filename, fname);
		(void) strcat(filename, "_");
		(void) strcat(filename, vtimes[nvt]);

		/* Make sure that the output directory exists                   */
		/* Note that the default output directory for this Allied Model */
		/*  will have been created by prepare_source_directory() above! */
		(void) strcpy(basedir, dir_name(filename));
		if ( !create_directory(basedir, S_IRWXU|S_IRWXG|S_IRWXO, &created) )
			{
			(void) fprintf(stdout, "%s Cannot create directory \"%s\"\n",
					MyLabel, basedir);
			continue;
			}
		else if ( created )
			{
			(void) fprintf(stdout, " Creating directory ... \"%s\"\n", basedir);
			}

		/* Now try to open the output file */
		if ( IsNull( FP_Out = fopen(filename, "w") ) )
			{
			(void) fprintf(stdout, "%s Cannot open output file \"%s\"\n",
					MyLabel, filename);
			continue;
			}
		(void) fprintf(stdout, " Outputting to file ... %s\n", filename);
		ntemps++;

		/* Set base directory for shuffle and file locks */
		(void) strcpy(LockDir, basedir);

		/* Set a file lock in the base directory while processing field */
		(void) strcpy(LockVtime, vtimes[nvt]);
		if ( !set_file_lock(LockDir, LockVtime) )
			{
			(void) fprintf(stderr, "%s Cannot establish file lock!\n", MyLabel);
			continue;
			}
		Locked = TRUE;

		/* Output ASCII file containing surface temperature data */
		for ( iiy=0; iiy<ny; iiy++ )
			{
			for ( iix=0; iix<nx; iix++ )
				{
				(void) fprintf(FP_Out, "%6.1f,", tmps[iiy*nx + iix]);
				}
			(void) fprintf(FP_Out, "\n");
			}

		/* Close the output file */
		(void) fclose(FP_Out);

		/* Remove the current lock in the base directory */
		(void) release_file_lock(LockDir, LockVtime);
		Locked = FALSE;
		}

	/**********************************************/
	/**********************************************/
	/** Loop to extract wind direction and speed **/
	/**********************************************/
	/**********************************************/

	/* Loop on number of valid times for data requested */
	for ( nwinds=0, nvt=0; nvt<numvts; nvt++ )
		{

		/********************************************/
		/********************************************/
		/** Extract wind direction and speed here! **/
		/********************************************/
		/********************************************/

		/* Check for FPA wind in the "required_wind_crossrefs" block */
		locw = source_allied_data_location(sdef, FpaC_ALLIED_WINDS, FpaWind);
		if ( locw < 0 )
			{
			(void) fprintf(stdout, "%s Unknown", MyLabel);
			(void) fprintf(stdout, " \"required_wind_crossrefs\" file \"%s\"\n",
					FpaWind);
			continue;
			}

		/* Set field descriptor for extracting the wind direction and speed  */
		/* Note that the "source" information is taken from the Allied Model */
		/*  "winds" structure, but that the "element" and "level" are not    */
		/*  set since the wind will be extracted by a crossreference!        */
		(void) copy_fld_descript(&descript, &fdescin);
		if ( !set_fld_descript(&descript,
								FpaF_ELEMENT_NAME, FpaCblank,
								FpaF_LEVEL_NAME,   FpaCblank,
								FpaF_SOURCE,       winds->src_defs[locw],
								FpaF_SUBSOURCE,    winds->sub_defs[locw],
								FpaF_RUN_TIME,     rtime,
								FpaF_VALID_TIME,   vtimes[nvt],
								FpaF_END_OF_LIST) )
			{
			(void) fprintf(stdout, "%s Problem setting field descriptor",
					MyLabel);
			(void) fprintf(stdout, " for \"%s %s\" at \"%s %s\"\n",
					winds->src_defs[locw]->name, winds->sub_defs[locw]->name,
					rtime, vtimes[nvt]);
			continue;
			}

		/* Extract the wind direction and speed */
		dirs = INITMEM(float, ny*nx);
		spds = INITMEM(float, ny*nx);
		gsts = INITMEM(float, ny*nx);
		if ( !extract_awind_by_crossref(winds->wcrefs[locw]->name,
				&descript, allied->time_match, ny*nx, ppos, clon,
				dirs, spds, gsts, &sunits) )
			{
			(void) fprintf(stdout, "%s Cannot extract data", MyLabel);
			(void) fprintf(stdout, " for \"%s\"", winds->wcrefs[locw]->name);
			(void) fprintf(stdout, "  from \"%s %s\" at \"%s %s\"\n",
					winds->src_defs[locw]->name, winds->sub_defs[locw]->name,
					rtime, vtimes[nvt]);
			continue;
			}

		/* Check that wind speeds and gusts can be converted to output units */
		if ( !convert_value(sunits, 0.0, Knots, NullDouble) )
			{
			(void) fprintf(stdout, "%s Cannot convert \"%s\" data",
					MyLabel, winds->wcrefs[locw]->name);
			(void) fprintf(stdout, " from \"%s\" to \"%s\"\n",
					sunits, Knots);
			continue;
			}

		/* Convert the wind speed and gusts for output */
		for ( ii=0; ii<ny*nx; ii++ )
			{
			(void) convert_value(sunits, (double) spds[ii], Knots, &dval);
			spds[ii] = (float) dval;
			(void) convert_value(sunits, (double) gsts[ii], Knots, &dval);
			gsts[ii] = (float) dval;
			}

		/******************************************************************/
		/******************************************************************/
		/** Ouput file containing sampled wind direction and speed here! **/
		/******************************************************************/
		/******************************************************************/

		/* Get the output filename for the wind direction and speed */
		fname = build_allied_filename(&fdescout, FpaC_ALLIED_FILES, OutputWind);
		if ( blank(fname) )
			{
			(void) fprintf(stdout, "%s Cannot build pathname for", MyLabel);
			(void) fprintf(stdout, " \"files\" file \"%s\"\n", OutputWind);
			continue;
			}

		/* Append the valid time to the filename */
		(void) strcpy(filename, fname);
		(void) strcat(filename, "_");
		(void) strcat(filename, vtimes[nvt]);

		/* Make sure that the output directory exists                   */
		/* Note that the default output directory for this Allied Model */
		/*  will have been created by prepare_source_directory() above! */
		(void) strcpy(basedir, dir_name(filename));
		if ( !create_directory(basedir, S_IRWXU|S_IRWXG|S_IRWXO, &created) )
			{
			(void) fprintf(stdout, "%s Cannot create directory \"%s\"\n",
					MyLabel, basedir);
			continue;
			}
		else if ( created )
			{
			(void) fprintf(stdout, " Creating directory ... \"%s\"\n", basedir);
			}

		/* Now try to open the output file */
		if ( IsNull( FP_Out = fopen(filename, "w") ) )
			{
			(void) fprintf(stdout, "%s Cannot open output file \"%s\"\n",
					MyLabel, filename);
			continue;
			}
		(void) fprintf(stdout, " Outputting to file ... %s\n", filename);
		nwinds++;

		/* Set base directory for shuffle and file locks */
		(void) strcpy(LockDir, basedir);

		/* Set a file lock in the base directory while processing field */
		(void) strcpy(LockVtime, vtimes[nvt]);
		if ( !set_file_lock(LockDir, LockVtime) )
			{
			(void) fprintf(stderr, "%s Cannot establish file lock!\n", MyLabel);
			continue;
			}
		Locked = TRUE;

		/* Output ASCII file containing wind direction and speed */
		for ( iiy=0; iiy<ny; iiy++ )
			{
			for ( iix=0; iix<nx; iix++ )
				{
				(void) fprintf(FP_Out, "%6.1f,", spds[iiy*nx + iix]);
				}
			(void) fprintf(FP_Out, "\n");
			}

		/* Close the output file */
		(void) fclose(FP_Out);

		/* Remove the current lock in the base directory */
		(void) release_file_lock(LockDir, LockVtime);
		Locked = FALSE;
		}

	/****************************/
	/****************************/
	/** Final shutdown message **/
	/****************************/
	/****************************/

	/* Shutdown message */
	(void) systime(&cyear, &cjday, &chour, &cmin, &csec);
	(void) mdate(&cyear, &cjday, &cmonth, &cmday);
	(void) fprintf(stdout, "\n%s Finished: %d/%.2d/%.2d %.2d:%.2d:%.2d GMT\n\n",
			MyLabel, cyear, cmonth, cmday, chour, cmin, csec);
	(void) fprintf(stdout, "%s   %d by %d (%d) data points extracted\n",
			MyLabel, nx, ny, ny*nx);
	(void) fprintf(stdout, "%s  for  %d  %s fields and  %d  %s fields\n",
			MyLabel, ntemps, SfcTemp, nwinds, FpaWind);

	return 0;
	}

/***********************************************************************
*                                                                      *
*     e r r o r _ t r a p                                              *
*                                                                      *
***********************************************************************/

static	void	error_trap(sig)
int		sig;
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
	exit(1);
	}
