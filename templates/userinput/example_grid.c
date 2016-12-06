/***********************************************************************
*                                                                      *
*     e x a m p l e _ g r i d . c                                      *
*                                                                      *
*     Template for creating "Continuous" type metafiles from grid      *
*      point data.                                                     *
*                                                                      *
*   Usage:  example_grid  <setup_file>  (<optional_config_info>)       *
*                           <input_file>  (<input_file>  ...)          *
*                                                                      *
*     where  <setup_file>            is the usual local setup file name*
*            <optional_config_info>  is information regarding fields,  *
*                                     directories, and times           *
*            <input_file>            is the input data file (or files) *
*                                     containing grid point data       *
*                                                                      *
*     The <optional_config_info> can consist of any of the following:  *
*       source               - the FPA directory name                  *
*       run (or issue) time  - the date/time of the FPA directory      *
*       valid time           - the valid date/time for the field       *
*       element              - the FPA element name                    *
*       level                - the FPA level name                      *
*       inunits              - the FPA units of the data               *
*                                                                      *
*   Note that the "source", "run time", "valid time", "element",       *
*   "level" and "inunits"  information can be hard-coded (in a         *
*   "static const STRING" declaration), soft-coded as run string       *
*   parameter (as argv()), or contained in the data file name or as    *
*   part of the data file.                                             *
*                                                                      *
*   Note that the "source", "element", "level" and "inunits" names     *
*   must correspond to names in the FPA configuration files.  If the   *
*   data routine uses different names, you may have to set up a        *
*   cross-reference table to convert these to FPA names.               *
*                                                                      *
*   In this example, the "source" is hard-coded as a declaration,      *
*   the "run time" is soft-coded as a run string parameter,            *
*   the "valid time" is contained in the data file name,               *
*   and the "element", "level" and "inunits" are read from the data    *
*   file itself.                                                       *
*                                                                      *
*   This example routine also requires information regarding the       *
*   grid projection and spacing of the grid points.  This information  *
*   can be hard-coded (in a "static const STRING" declaration), or     *
*   included as part of the data file.  A complete description of the  *
*   parameters can be found in the "FPA Graphics Metafile Standard"    *
*   for the "projection", "mapdef", and "grid" commands.               *
*                                                                      *
*     Parameters from "projection" command:                            *
*       type                - a projection keyword                     *
*       ref1 ref2 ... ref5  - projection dependent reference points    *
*                                                                      *
*     Parameters from "mapdef" command:                                *
*       olat   - latitude of the grid data "origin"                    *
*       olon   - longitude of the grid data "origin"                   *
*       rlon   - reference longitude of the grid data                  *
*       xmin   - start of the x-axis of the grid data (in "units")     *
*       ymin   - start of the y-axis of the grid data (in "units")     *
*       xmax   - end of the x-axis of the grid data (in "units")       *
*       ymax   - end of the y-axis of the grid data (in "units")       *
*       units  - number of metres per map unit (or degrees per map     *
*                 for "latitude_longitude" or "rotated_lat_lon"        *
*                 projections)                                         *
*                                                                      *
*     Parameters from "grid" command:                                  *
*       nx     - number of grid points along x-axis                    *
*       ny     - number of grid points along y-axis                    *
*       grid   - grid spacing along both axes                          *
*        or                                                            *
*       xgrid  - grid spacing along x-axis                             *
*       ygrid  - grid spacing along y-axis                             *
*                                                                      *
*   Note that  (nx-1) * xgrid  should equal  (xmax - xmin)             *
*   and that   (ny-1) * ygrid  should equal  (ymax - ymin)             *
*                                                                      *
*   In this example, the "projection" and "mapdef" information is      *
*   hard-coded as a declaration, and the "grid" information is read    *
*   from the data file itself.                                         *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
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

static	LOGICAL	DebugMode = FALSE;


static	const	STRING	MyTitle = "FPA Grid Point Data Input";
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
static  const  STRING	ExampleSource = "EXAMPLES";

/********************************************************************/
/********************************************************************/
/** Declarations for "projection", "mapdef", or "grid" information **/
/********************************************************************/
/********************************************************************/
static  const  STRING	Projection = "polar_stereographic";
static  const  STRING	Ref1       = "north";
static  const  STRING	Ref2       = "60.0";
static  const  STRING	Ref3       = "0.0";
static  const  STRING	Ref4       = "0.0";
static  const  STRING	Ref5       = "0.0";

static  const  STRING	Olat       = "31.5N";
static  const  STRING	Olon       = "139.5W";
static  const  STRING	Rlon       = "111W";
static  const  float	Xmin       = 0;
static  const  float	Ymin       = 0;
static  const  float	Xmax       = 6500;
static  const  float	Ymax       = 5900;
static  const  float	Units      = 1000;

/***********************************************************************
*                                                                      *
*     m a i n                                                          *
*                                                                      *
***********************************************************************/

int		main(argc, argv)

int		argc;
STRING	argv[];
	{
	int			status, numargs, nslist, iarg, nfiles;
	int			cyear, cjday, cmonth, cmday, chour, cmin, csec;
	STRING		setupfile, runtime, *slist, dir, fname;
	MAP_PROJ	*mproj;
	float		clon;
	char		homedir[MAX_BCHRS], inputdir[MAX_BCHRS];
	char		dataname[MAX_BCHRS];

	LOGICAL		local;
	int			iyear, ijday, imonth, imday, ihour, imin, iplushr;
	STRING		source  = NullString;
	STRING		rtime   = NullString;
	STRING		vtime   = NullString;
	STRING		vlocal  = NullString;
	STRING		element = NullString;
	STRING		level   = NullString;
	STRING		inunits = NullString;

	PROJ_DEF	sproj;
	MAP_DEF		smdef;
	GRID_DEF	sgdef, odef;
	MAP_PROJ	smproj;
	int			nx, ny, iix, iiy;
	float		grid1, grid2, grid, xgrid, ygrid;

	FLD_DESCRIPT		fdesc;
	FpaConfigUnitStruct	*udef;
	float				*vbuf, **gbuf;
	METAFILE			meta = NullMeta;
	SURFACE				sfc  = NullSfc;
	GRID				gridd;

	static USPEC		uspec = {NULL, 1.0, 0.0};

	/******************************/
	/******************************/
	/** File reading information **/
	/******************************/
	/******************************/
	static			char		Line[1024];			/* Line reading buffer */
	static	const	size_t		Ncl = 1023;			/* Size of buffer - 1 */
	static			LOGICAL		argOK;
	static			FILE		*DataFile = NULL;

	/* Ignore hangup, interrupt and quit signals so we can survive after */
	/* logging off */
	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	status = setvbuf(stdout, NULL, _IOLBF, 0);
	status = setvbuf(stderr, NULL, _IOLBF, 0);

	/* Set debug mode (if requested) */
	if ( DebugMode ) (void) pr_control(NULL, 5, 1);
	else             (void) pr_control(NULL, 1, 1);

	/*****************************************************************/
	/*****************************************************************/
	/** Validate run string parameters                              **/
	/** (Note that this example contains one "optional_config_info" **/
	/**   parameter, or a total of three run string parameters)     **/
	/*****************************************************************/
	/*****************************************************************/
	numargs = 3;
	if ( argc < (numargs + 1) )
		{
		(void) fprintf(stderr, "Usage:\n");
		(void) fprintf(stderr, "  example_grid  <setup_file>");
		(void) fprintf(stderr, "  [<optional_config_info>]");
		(void) fprintf(stderr, "  <input_file>  [<input_file> ...]\n\n");
		(void) fprintf(stderr, "      <input_file> is an input data file\n\n");
		return (-1);
		}

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

	/* Set run string parameters */
	setupfile = strdup(argv[1]);
	runtime   = strdup(argv[2]);

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
	mproj = get_target_map();
	if ( !mproj )
		{
		(void) fprintf(stderr, "%s Target map not defined", MyLabel);
		(void) fprintf(stderr, " in setup file \"%s\"\n", setupfile);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Set center longitude from target projection definition */
	if ( !grid_center(mproj, NULL, NULL, &clon) )
		{
		(void) fprintf(stderr, "%s Problem with center lat/long\n", MyLabel);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/**************************************************************************/
	/**************************************************************************/
	/** Retrieve the "homedir" and "inputdir" directories                    **/
	/** Note that the "homedir" directory is the base data directory for FPA **/
	/** Note that the "inputdir" directory is the directory in which the     **/
	/**  input data files will be located                                    **/
	/** The get_directory() command identifies a directory from a directory  **/
	/**  key in the "directories" block of the local setup file (if found)   **/
	/** The getenv() command identifies a directory from an environment      **/
	/**  variable set in the .fparc file (if found)                          **/
	/**************************************************************************/
	/**************************************************************************/
	dir = home_directory();
	(void) strcpy(homedir, dir);
	dir = get_directory("Examples");
	if ( !blank(dir) ) (void) strcpy(inputdir, dir);
	else               (void) strcpy(inputdir, homedir);
	dir = getenv("FPA_EXAMPLES");
	if ( !blank(dir) ) (void) strcpy(inputdir, dir);

	/************************************************************************/
	/************************************************************************/
	/** Need the source directory and run (or issue) time here!            **/
	/** The source directory name is a STRING of characters                **/
	/** The run time can be created by a call to                           **/
	/**    rtime = build_tstamp(iyear, ijday, ihour, imin, local, mins);   **/
	/**  where the year (iyear), julian day (ijday), hour of day (ihour),  **/
	/**  and minute of hour (imin) are integers, and the local time of day **/
	/**  parameter (local) is usually FALSE, that is, a GMT based time.    **/
	/**  Also, the use minutes flag (mins) is usually set to FALSE.        **/
	/** Or you can save a copy of the run time by calling                  **/
	/**    rtime = strdup(build_tstamp(iyear, ijday, ihour, imin, local,   **/
	/**                                mins));                             **/
	/** The julian day (ijday) can be created from the month               **/
	/**  and day (imonth and imday) by a call to                           **/
	/**    (void) jdate(&iyear, &imonth, &imday, &ijday);                  **/
	/** The year and julian day can be normalized (if you add              **/
	/**  or subtract from the day, for example) by a call to               **/
	/**    (void) jnorm(&iyear, &ijday);                                   **/
	/************************************************************************/
	/************************************************************************/

	/*********************************************************/
	/*********************************************************/
	/** For this example, "source" is hard-coded,           **/
	/**  and "run time" is passed as a run string parameter **/
	/*********************************************************/
	/*********************************************************/
	source = ExampleSource;
	rtime  = runtime;

	/* Initialize the field descriptor for files */
	(void) init_fld_descript(&fdesc);
	if ( !set_fld_descript(&fdesc,
							FpaF_MAP_PROJECTION, mproj,
							FpaF_DIRECTORY_PATH, homedir,
							FpaF_SOURCE_NAME,    source,
							FpaF_RUN_TIME,       rtime,
							FpaF_END_OF_LIST) )
		{
		(void) fprintf(stderr, "%s Problem initializing field descriptor\n",
				MyLabel);
		(void) fprintf(stderr, "  for \"%s\"  at \"%s\"\n", source, rtime);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Set base directory for shuffle and file locks */
	dir = source_directory_by_name(fdesc.sdef->name,
			fdesc.subdef->name, FpaCblank);
	(void) strcpy(LockDir, dir);

	/* Prepare data directory for output */
	if ( blank(prepare_source_directory(&fdesc)) )
		{
		(void) fprintf(stderr, "%s Problem preparing data directory", MyLabel);
		(void) fprintf(stderr, " for \"%s %s\"  at \"%s\"\n",
				fdesc.sdef->name, fdesc.subdef->name, fdesc.rtime);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/***************************************************************/
	/***************************************************************/
	/** Read all the data files.                                  **/
	/** Note that each data file will contain grid point data for **/
	/**  a separate field!                                        **/
	/***************************************************************/
	/***************************************************************/
	nfiles  = 0;
	for ( iarg=numargs; iarg<argc; iarg++ )
		{

		/************************************************/
		/************************************************/
		/** Extract valid timestamp from data filename **/
		/************************************************/
		/************************************************/
		FREEMEM(vtime);
		vtime = strdup((strpbrk(argv[iarg], "_") + 1));

		/* Set data filename */
		(void) strcpy(dataname, pathname(inputdir, argv[iarg]));
		(void) fprintf(stdout, "\n%s Processing data file \"%s\"\n",
				MyLabel, dataname);

		/* Open the data file */
		DataFile = fopen(dataname, "r");
		if ( !DataFile )
			{
			(void) fprintf(stderr, "%s Cannot access data file \"%s\"\n",
					MyLabel, dataname);
			return (-1);
			}

		/******************************************************************/
		/******************************************************************/
		/** Need the valid time and element/level names here!            **/
		/** The element/level names are STRINGs of characters.           **/
		/** The valid time can be created by a call to                   **/
		/**    vtime = build_tstamp(iyear, ijday, ihour, imin, local,    **/
		/**                                mins);                        **/
		/**  where the local parameter is set to TRUE to create a daily  **/
		/**  type field, or to FALSE to create a normal field.           **/
		/**  Also, the use minutes flag (mins) is usually set to FALSE.  **/
		/** Or you can calculate the valid time from the run time and an **/
		/**  additional number of hours (iplushr) by a call to           **/
		/**    vtime = calc_valid_time(rtime, iplushr);                  **/
		/** You can convert a time from GMT to local by a call to        **/
		/**    vlocal = gmt_to_local(vtime, clon);                       **/
		/**  where clon is the center longitude of the current basemap   **/
		/** Or you can convert a time from local to GMT by a call to     **/
		/**    vtime = local_to_gmt(vlocal, clon);                       **/
		/******************************************************************/
		/******************************************************************/

		/**********************************************************/
		/**********************************************************/
		/** For this example, "valid time" is extracted from     **/
		/**  the name of the data file, and "element", "level"   **/
		/**  and "inunits" are read from the data file itself.   **/
		/** Note that the config file information for "inunits"  **/
		/**  is used to define the SURFACE Object "uspec" below. **/
		/**********************************************************/
		/**********************************************************/
		(void) getfileline(DataFile, Line, Ncl);
		FREEMEM(element);
		FREEMEM(level);
		FREEMEM(inunits);
		element = strdup_arg(Line);
		level   = strdup_arg(Line);
		inunits = strdup_arg(Line);

		/* Set field descriptor for grid point data */
		if ( !set_fld_descript(&fdesc,
								FpaF_VALID_TIME,   vtime,
								FpaF_ELEMENT_NAME, element,
								FpaF_LEVEL_NAME,   level,
								FpaF_END_OF_LIST) )
			{
			(void) fprintf(stderr, "%s Problem setting field descriptor",
					MyLabel);
			(void) fprintf(stderr, " for \"%s %s\"", element, level);
			(void) fprintf(stderr, "  at \"%s\"\n", vtime);
			(void) fprintf(stdout, "%s Aborted\n", MyLabel);
			return (-1);
			}

		/* Check the units for the grid point data */
		udef = identify_unit(inunits);
		if ( !udef )
			{
			(void) fprintf(stderr, "%s Unrecognizable units \"%s\"",
					MyLabel, inunits);
			(void) fprintf(stderr, " for \"%s %s\"", element, level);
			(void) fprintf(stderr, " at \"%s\"\n", vtime);
			(void) fprintf(stdout, "%s Aborted\n", MyLabel);
			return (-1);
			}

		/******************************************************************/
		/******************************************************************/
		/** Need the "projection", "mapdef" and "grid" information here! **/
		/******************************************************************/
		/******************************************************************/

		/*****************************************************************/
		/*****************************************************************/
		/** For this example, the "projection" and "mapdef" information **/
		/**  is hard-coded and the "grid" information is read from the  **/
		/**  data file itself.                                          **/
		/*****************************************************************/
		/*****************************************************************/

		/* Define the projection */
		if ( !define_projection_by_name(&sproj, Projection,
				Ref1, Ref2, Ref3, Ref4, Ref5) )
			{
			(void) fprintf(stderr, "%s Problem setting \"projection\" with",
					MyLabel);
			(void) fprintf(stderr, "  type: \"%s\"  ref(1-5): \"%s\" \"%s\"",
					Projection, Ref1, Ref2);
			(void) fprintf(stderr, " \"%s\" \"%s\" \"%s\"\n", Ref3, Ref4, Ref5);
			(void) fprintf(stdout, "%s Aborted\n", MyLabel);
			return (-1);
			}

		/* Define the map definition */
		if ( !define_map_def(&smdef, Olat, Olon, Rlon, Xmin, Ymin,
				Xmax, Ymax, Units) )
			{
			(void) fprintf(stderr, "%s Problem setting \"mapdef\" with",
					MyLabel);
			(void) fprintf(stderr, "  olat/olon: \"%s %s\"  rlon: \"%s\"\n",
					Olat, Olon, Rlon);
			(void) fprintf(stderr, "%s                              ",
					MyLabel);
			(void) fprintf(stderr, "   xmin/ymin: \"%f %f\"", Xmin, Ymin);
			(void) fprintf(stderr, "  xmax/ymax: \"%f %f\"", Xmax, Ymax);
			(void) fprintf(stderr, "  units: \"%s\"\n", Units);
			(void) fprintf(stdout, "%s Aborted\n", MyLabel);
			return (-1);
			}

		/* Define the grid definition */
		(void) getfileline(DataFile, Line, Ncl);
		nx    = int_arg(Line, &argOK);
		ny    = int_arg(Line, &argOK);
		grid1 = float_arg(Line, &argOK);
		grid2 = float_arg(Line, &argOK);
		if ( !argOK )
			{
			grid  = grid1;
			xgrid = 0.0;
			ygrid = 0.0;
			}
		else
			{
			grid  = 0.0;
			xgrid = grid1;
			ygrid = grid2;
			}
		if ( !define_grid_def(&sgdef, nx, ny, grid, xgrid, ygrid, Units) )
			{
			(void) fprintf(stderr, "%s Problem setting \"grid\" with",
					MyLabel);
			(void) fprintf(stderr, "  nx/ny: \"%d %d\"  grid: \"%f\"",
					nx, ny, grid);
			(void) fprintf(stderr, "  xgrid/ygrid: \"%f %f\"  units: \"%s\"\n",
					grid, xgrid, ygrid, Units);
			(void) fprintf(stdout, "%s Aborted\n", MyLabel);
			return (-1);
			}

		/* Define the full map projection */
		(void) define_map_projection(&smproj, &sproj, &smdef, &sgdef);

		/****************************************************/
		/****************************************************/
		/** Read in the grid data from the data file here! **/
		/****************************************************/
		/****************************************************/
		vbuf = INITMEM(float, nx*ny);
		gbuf = INITMEM(float *, ny);
		for ( iiy=0; iiy<ny; iiy++ )
			{
			gbuf[iiy] = vbuf + iiy*nx;
			for ( iix=0; iix<nx; iix++ )
				{
				if ( blank(Line) ) (void) getfileline(DataFile, Line, Ncl);
				gbuf[iiy][iix] = float_arg(Line, &argOK);
				}
			}

		/* Create METAFILE Object to hold grid point data */
		meta = create_metafile();
		(void) define_mf_tstamp(meta, fdesc.rtime, fdesc.vtime);
		(void) define_mf_projection(meta, mproj);

		/* Create a GRID object to hold the grid point data */
		(void) init_grid(&gridd);
		(void) define_grid(&gridd, nx, ny, ZeroPoint, 0.0, grid, vbuf, nx);

		/* Remap the grid point data to the target map projection */
		(void) remap_grid(&gridd, &smproj, mproj);
		odef = mproj->grid;

		/* Convert GRID Object to SURFACE Object */
		sfc = create_surface();
		(void) grid_surface(sfc, odef.gridlen, odef.nx, odef.ny, gridd.gval);

		/* Set units for data in SURFACE Object */
		(void) define_uspec(&uspec, udef->name, udef->factor, udef->offset);
		(void) define_surface_units(sfc, &uspec);

		/* Move SURFACE Object to METAFILE Object */
		(void) add_sfc_to_metafile(meta, "a",
				fdesc.edef->name, fdesc.ldef->name, sfc);

		/* Set a file lock in the base directory while processing field */
		(void) strcpy(LockVtime, vtime);
		if ( !set_file_lock(LockDir, LockVtime) )
			{
			(void) fprintf(stderr, "%s Cannot establish file lock!\n", MyLabel);
			return (-1);
			}
		Locked = TRUE;

		/* Construct new format (or old format) metafile name */
		fname = construct_meta_filename(&fdesc);
		if ( blank(fname) ) fname = build_meta_filename(&fdesc);

		/* Output METAFILE Object containing SURFACE Object */
		(void) write_metafile(fname, meta, 0);

		/* Free space used by work Objects */
		meta = destroy_metafile(meta);
		(void) free_grid(&gridd);
		FREEMEM(gbuf);
		FREEMEM(vbuf);

		/* Remove the current lock in the base directory */
		(void) release_file_lock(LockDir, LockVtime);
		Locked = FALSE;

		/* Keep count of the number of data files processed */
		(void) fclose(DataFile);
		nfiles++;
		}

	/* Shutdown message */
	(void) systime(&cyear, &cjday, &chour, &cmin, &csec);
	(void) mdate(&cyear, &cjday, &cmonth, &cmday);
	(void) fprintf(stdout, "\n%s Finished: %d/%.2d/%.2d %.2d:%.2d:%.2d GMT\n",
			MyLabel, cyear, cmonth, cmday, chour, cmin, csec);
	(void) fprintf(stdout, "\n%s   %d data file(s) processed\n",
			MyLabel, nfiles);

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
