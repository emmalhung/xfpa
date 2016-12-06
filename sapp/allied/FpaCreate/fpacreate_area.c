/***********************************************************************
*                                                                      *
*     f p a c r e a t e _ a r e a . c                                  *
*                                                                      *
*     FPA Create Area Program                                          *
*                                                                      *
*     This program takes point, line or link chain objects from a      *
*     given field, and creates an area outline around the objects      *
*     based on one or more object attributes.                          *
*                                                                      *
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

static	LOGICAL	DebugMode = TRUE;

/* Trap for error situations */
static	void	error_trap(int);


/* Link chain field types for creating areas */
static	const	STRING	LChainTrack = "track";
static	const	STRING	LChainNodes = "nodes";

/* Allied Model aliases in Config files */
static	const	STRING	ObjectField = "object_field";
static	const	STRING	OutputField = "output_field";

/* Allied Model attribute tags in Config files */
#define	Orient		"orientation"
#define	Radius		"radius"
#define	Diameter	"diameter"
#define	DistLeft	"left"
#define	DistRight	"right"
#define	PointN		"point_n"
#define	PointNNE	"point_nne"
#define	PointNE		"point_ne"
#define	PointENE	"point_ene"
#define	PointE		"point_e"
#define	PointESE	"point_ese"
#define	PointSE		"point_se"
#define	PointSSE	"point_sse"
#define	PointS		"point_s"
#define	PointSSW	"point_ssw"
#define	PointSW		"point_sw"
#define	PointWSW	"point_wsw"
#define	PointW		"point_w"
#define	PointWNW	"point_wnw"
#define	PointNW		"point_nw"
#define	PointNNW	"point_nnw"
#define	PointStart	"point_"

/* Define structure to hold map directions for attribute tags */
typedef struct
	{
	STRING				tag;
	float				dir;
	STRING				attname;
	FpaConfigUnitStruct	*attunit;
	} TAG_INFO;

/* Define map directions for attribute tags */
static	TAG_INFO	TagDirs[] = {
	{ PointN,       0.0, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ PointNNE,    22.5, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ PointNE,     45.0, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ PointENE,    67.5, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ PointE,      90.0, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ PointESE,   112.5, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ PointSE,    135.0, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ PointSSE,   157.5, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ PointS,     180.0, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ PointSSW,   202.5, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ PointSW,    225.0, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ PointWSW,   247.5, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ PointW,     270.0, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ PointWNW,   292.5, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ PointNW,    315.0, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ PointNNW,   337.5, NullString, NullPtr(FpaConfigUnitStruct *) },
};
static	const	int		NumTagDirs = (int) (sizeof(TagDirs) / sizeof(TAG_INFO));

/* Define orientation and distances for attribute tags */
static	TAG_INFO	TagOrient =
	{ Orient,    -999.0, NullString, NullPtr(FpaConfigUnitStruct *) };
static	TAG_INFO	TagRadius =
	{ Radius,    -999.0, NullString, NullPtr(FpaConfigUnitStruct *) };
static	TAG_INFO	TagDiameter =
	{ Diameter,  -999.0, NullString, NullPtr(FpaConfigUnitStruct *) };
static	TAG_INFO	TagDistLeft =
	{ DistLeft,  -999.0, NullString, NullPtr(FpaConfigUnitStruct *) };
static	TAG_INFO	TagDistRight =
	{ DistRight, -999.0, NullString, NullPtr(FpaConfigUnitStruct *) };

/* Define storage for remaining attribute tags */
static	TAG_INFO	*TagAtts  = NullPtr(TAG_INFO *);
static	int			NumTagAtt = 0;
static	TAG_INFO	*TagNAtts  = NullPtr(TAG_INFO *);
static	int			NumTagNAtt = 0;

/* Define adjustments to line direction for start cap locations */
static	TAG_INFO	SCapDirs[] = {
	{ "",  112.5, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ "",  135.0, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ "",  157.5, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ "",  180.0, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ "",  202.5, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ "",  225.0, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ "",  247.5, NullString, NullPtr(FpaConfigUnitStruct *) },
};
static	const	int		NumSCapDirs = (int) (sizeof(SCapDirs) / sizeof(TAG_INFO));

/* Define adjustments to line direction for end cap locations */
static	TAG_INFO	ECapDirs[] = {
	{ "", -67.5, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ "", -45.0, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ "", -22.5, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ "",   0.0, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ "",  22.5, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ "",  45.0, NullString, NullPtr(FpaConfigUnitStruct *) },
	{ "",  67.5, NullString, NullPtr(FpaConfigUnitStruct *) },
};
static	const	int		NumECapDirs = (int) (sizeof(ECapDirs) / sizeof(TAG_INFO));

/* Default smoothing factor for spline fitting */
static	const	float	DefaultSmoothing = 500;

/* Default units for distances */
#define	DistanceUnitsM	"m"


/* Default message strings */
static	const	STRING	MyTitle = "FPA Create Area";
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
	STRING							setup, amodel, subarea, rtime, vtime;
	LOGICAL							use_track, use_interp, startcap;
	STRING							stime, etime, ltype, ntype;
	float							smoothing, sres;
	int								nslist;
	STRING							*slist;
	int								cyear, cjday, cmonth, cday, chr, cmin, csec;
	MAP_PROJ						*mproj;
	int								Inumx, Inumy;
	POINT							**Apstns;
	float							Aglen, Clon;
	STRING							datadir, basedir, fname;
	FLD_DESCRIPT					fdesc, descript, descriptin;
	FpaConfigSourceStruct			*sdef;
	FpaConfigSourceAlliedStruct		*allied;
	FpaConfigAlliedFieldsStruct		*fields;
	FpaConfigAlliedMetafilesStruct	*metafiles;
	FpaConfigFieldStruct			*fdef;
	FpaConfigAlliedAttribStruct		*attrib_info, *node_info, *attrib_meta;
	FpaConfigAlliedDefAttribStruct	*def_info;
	LOGICAL							first, pok, attok;
	int								fkind, natt, nn, ntx, nset, ips, ipe, inode;
	int								splus, eplus;
	float							xrad, xleft, xright, ldir, xdir, xavg, xdist;
	double							dval;
	STRING							val, lval, rval, svt, evt, nvt;
	POINT							spos, epos, xpos;
	SET								set = NullSet;
	SPOT							spot;
	CURVE							curve;
	LCHAIN							lchain;
	LINTERP							linterp;
	LNODE							lnode;
	METAFILE						meta;
	SET								aset;
	AREA							area;
	LINE							bdry, xline, tline;
	CAL								cal, acal, ncal;
	int								nlines;
	LINE							*lines;

	/* Working lines */
	static	LINE	lline = NullLine;
	static	LINE	rline = NullLine;
	static	LINE	scap  = NullLine;
	static	LINE	ecap  = NullLine;
	static	LINE	span  = NullLine;
	static	LINE	lspan = NullLine;
	static	LINE	rspan = NullLine;

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
	if ( argc < 7 )
		{
		(void) fprintf(stderr, "Usage:\n");
		(void) fprintf(stderr, "   fpacreate_area  <setup_file>");
		(void) fprintf(stderr, "  <allied_model_name>  <subarea_name>\n");
		(void) fprintf(stderr, "                    <run_time>  <valid_time>");
		(void) fprintf(stderr, "  <smoothing>\n");
		(void) fprintf(stderr, "                     (<start_time>  <end_time>");
		(void) fprintf(stderr, "  <lchain_type>  <node_type>)\n\n");
		(void) fprintf(stderr, "   Note that  <smoothing>  produces smoother");
		(void) fprintf(stderr, " outlines with larger values (default 500)\n");
		(void) fprintf(stderr, "   Note that  <start_time>  and  <end_time>  are");
		(void) fprintf(stderr, " optional (for link chain objects)\n");
		(void) fprintf(stderr, "     with format  hh  or  hh:mm  where  hh/mm ");
		(void) fprintf(stderr, " are hours/minutes after current time\n");
		(void) fprintf(stderr, "   Note that  <lchain_type>  is optional");
		(void) fprintf(stderr, " (for link chain objects)\n");
		(void) fprintf(stderr, "     with  \"track\"  used to create an area");
		(void) fprintf(stderr, " based on all nodes of a link chain (default)\n");
		(void) fprintf(stderr, "      and  \"nodes\"  used to create one area");
		(void) fprintf(stderr, " for each node of a link chain\n");
		(void) fprintf(stderr, "   Note that  <node_type>  is optional");
		(void) fprintf(stderr, " (for link chain objects)\n");
		(void) fprintf(stderr, "     with  \"normal\"  used to create an area");
		(void) fprintf(stderr, " based on normal link chain nodes (default)\n");
		(void) fprintf(stderr, "      and  \"interp\"  used to create an area");
		(void) fprintf(stderr, " based on interpolated link chain nodes\n");
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
	vtime         = argv[5];
	if ( !same(argv[6], "-") ) smoothing = (float) atof(argv[6]);
	else                       smoothing = DefaultSmoothing;
	if ( argc > 7 && !same(argv[7], "-") ) stime = argv[7];
	else                                   stime = "";
	if ( argc > 8 && !same(argv[8], "-") ) etime = argv[8];
	else                                   etime = "";
	if ( argc > 9 && !same(argv[9], "-") ) ltype = argv[9];
	else                                   ltype = "";
	if ( argc > 10 && !same(argv[10], "-") ) ntype = argv[10];
	else                                     ntype = "";
	if ( blank(ltype) )
		{
		use_track = TRUE;
		}
	else
		{
		if      ( same_ic(ltype, LChainTrack) ) use_track = TRUE;
		else if ( same_ic(ltype, LChainNodes) ) use_track = FALSE;
		else
			{
			(void) fprintf(stderr,
					"In fpacreate_area <lchain_type> must be \"%s\" or \"%s\"\n",
					LChainTrack, LChainNodes);
			return (-1);
			}
		}
	if ( blank(ntype) )
		{
		use_interp = FALSE;
		}
	else
		{
		if      ( same_ic(ntype, FpaNodeClass_Normal) ) use_interp = FALSE;
		else if ( same_ic(ntype, FpaNodeClass_Interp) ) use_interp = TRUE;
		else
			{
			(void) fprintf(stderr,
					"In fpacreate_area <node_type> must be \"%s\" or \"%s\"\n",
					FpaNodeClass_Normal, FpaNodeClass_Interp);
			return (-1);
			}
		}

	/* Reset smoothing if too small! */
	if ( smoothing < 10.0 ) smoothing = 10.0;

	/* Startup message */
	(void) sprintf(MyLabel, "[%d] %s:", getpid(), MyTitle);
	(void) systime(&cyear, &cjday, &chr, &cmin, &csec);
	(void) mdate(&cyear, &cjday, &cmonth, &cday);
	(void) fprintf(stdout, "\n%s Beginning: %d/%.2d/%.2d %.2d:%.2d:%.2d GMT\n",
			MyLabel, cyear, cmonth, cday, chr, cmin, csec);
	(void) fprintf(stdout, "%s      for Allied Model \"%s %s\" at \"%s\" \"%s\"\n",
			MyLabel, amodel, subarea, rtime, vtime);
	(void) fprintf(stdout, "%s      with smoothing: %f", MyLabel, smoothing);
	if ( !blank(stime) ) (void) fprintf(stdout, "  stime: %s", stime);
	if ( !blank(etime) ) (void) fprintf(stdout, "  etime: %s", etime);
	if ( !blank(ltype) ) (void) fprintf(stdout, "  lchain_type: %s", ltype);
	if ( !blank(ntype) ) (void) fprintf(stdout, "  node_type: %s", ntype);
	(void) fprintf(stdout, "\n");

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
		(void) printf("   Map %g X %g\n", mproj->definition.xlen,
					mproj->definition.ylen);
		(void) printf("   Directory %s\n", datadir);
		(void) printf("   Source %s\n", sdef->name);
		(void) printf("   Run time %s\n", rtime);
		(void) printf("   Valid time %s\n", vtime);
		}
	init_fld_descript(&fdesc);
	if ( !set_fld_descript(&fdesc,
							FpaF_MAP_PROJECTION, mproj,
							FpaF_DIRECTORY_PATH, datadir,
							FpaF_SOURCE,         sdef,
							FpaF_RUN_TIME,       rtime,
							FpaF_VALID_TIME,     vtime,
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
		(void) printf("   Map %g X %g\n", mproj->definition.xlen,
					mproj->definition.ylen);
		(void) printf("   Directory %s\n", datadir);
		(void) printf("   Source %s\n", allied->src_def->name);
		(void) printf("   Subsource %s\n", allied->sub_def->name);
		(void) printf("   Run time %s\n", rtime);
		(void) printf("   Valid time %s\n", vtime);
		}
	init_fld_descript(&descript);
	if ( !set_fld_descript(&descript,
							FpaF_MAP_PROJECTION, mproj,
							FpaF_DIRECTORY_PATH, datadir,
							FpaF_SOURCE,         allied->src_def,
							FpaF_SUBSOURCE,      allied->sub_def,
							FpaF_RUN_TIME,       rtime,
							FpaF_VALID_TIME,     vtime,
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
			(void) printf("Setting descript\n");
			(void) printf("   Run time %s\n", FpaCblank);
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

	/* Set the field type from the required fields block */
	fdef  = fields->flds[0];
	fkind = fdef->element->fld_type;

	/* Reset the field type for extracting features (if required) */
	if ( fields->ftypes[0] != FpaCnoMacro ) fkind = fields->ftypes[0];

	/* Initialize a field descriptor for required fields file */
	/* Note that there will be only one required fields file! */
	if (DebugMode)
		{
		(void) printf("Initializing descriptin\n");
		(void) printf("   Map %g X %g\n", mproj->definition.xlen,
					mproj->definition.ylen);
		(void) printf("   Directory %s\n", datadir);
		(void) printf("   Source %s\n", fields->src_defs[0]->name);
		if ( NotNull(fields->sub_defs[0]) )
			(void) printf("   Subsource %s\n", fields->sub_defs[0]->name);
		(void) printf("   Run time %s\n", rtime);
		(void) printf("   Element %s\n", fdef->element->name);
		(void) printf("   Level %s\n", fdef->level->name);
		(void) printf("   Valid time %s\n", vtime);
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
					FpaF_FIELD_MACRO,    fkind,
					FpaF_VALID_TIME,     vtime,
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
			(void) printf("   Run time %s\n", FpaCblank);
			}
		(void) set_fld_descript(&descriptin,
									FpaF_RUN_TIME, FpaCblank,
									FpaF_END_OF_LIST);
		}

	/* Extract a set of objects based on type of required field */
	switch (fkind)
		{

		/* Retrieve a set of scattered points */
		case FpaC_SCATTERED:
			set = retrieve_spotset(&descriptin);
			break;

		/* Retrieve a set of curves */
		case FpaC_LINE:
			set = retrieve_curveset(&descriptin);
			break;

		/* Retrieve a set of link chains */
		case FpaC_LCHAIN:
			set = retrieve_lchainset(&descriptin);
			break;

		/* Cannot retrieve objects from other types of field */
		case FpaC_CONTINUOUS:
		case FpaC_VECTOR:
		case FpaC_DISCRETE:
		case FpaC_WIND:
		default:
			(void) fprintf(stderr,
					"%s Cannot create area for required field \"%s\" \"%s\"\n",
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
				"%s No file for required field \"%s\" \"%s\" at \"%s\"\n",
				MyLabel, fdef->element->name, fdef->level->name, vtime);
		(void) fprintf(stderr,
				"%s      for Allied Model \"%s %s\"\n",
				MyLabel, amodel, subarea);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}
	if ( set->num <= 0 )
		{
		(void) fprintf(stderr,
				"%s No objects in required field \"%s\" \"%s\" at \"%s\"\n",
				MyLabel, fdef->element->name, fdef->level->name, vtime);
		(void) fprintf(stderr,
				"%s      for Allied Model \"%s %s\"\n",
				MyLabel, amodel, subarea);
		(void) fprintf(stdout, "%s Aborted\n", MyLabel);
		return (-1);
		}
	if (DebugMode)
		{
		switch (fkind)
			{
			case FpaC_SCATTERED:
				(void) fprintf(stderr,
						"%s Creating areas for %d SPOT objects\n",
						MyLabel, set->num);
				break;
			case FpaC_LINE:
				(void) fprintf(stderr,
						"%s Creating areas for %d CURVE objects\n",
						MyLabel, set->num);
				break;
			case FpaC_LCHAIN:
				(void) fprintf(stderr,
						"%s Creating areas for %d LCHAIN objects\n",
						MyLabel, set->num);
				break;
			}
		}

	/* Identify attribute tags based on type of required field */
	switch (fkind)
		{
		case FpaC_SCATTERED:

			/* Identify the attribute tags and save them into structures */
			attrib_info = fields->attinfo[0];
			attrib_meta = metafiles->attinfo[0];
			if (DebugMode)
				{
				(void) fprintf(stderr,
						"%s Identifying %d attributes for SPOT objects\n",
						MyLabel, attrib_info->nattribs);
				}
			for ( natt=0; natt<attrib_info->nattribs; natt++ )
				{
				attok = FALSE;
				if (DebugMode)
					{
					(void) fprintf(stderr, "%s   Comparing attribute tag: %s\n",
							MyLabel, attrib_info->tag[natt]);
					}

				/* First save direction tags into the TagDirs structure */
				for ( nn=0; nn<NumTagDirs; nn++ )
					{
					if ( !same_ic(attrib_info->tag[natt], TagDirs[nn].tag) )
						continue;
					TagDirs[nn].attname = safe_strdup(attrib_info->attname[natt]);
					TagDirs[nn].attunit = attrib_info->attunit[natt];
					break;
					}
				if ( nn < NumTagDirs ) attok = TRUE;

				/* Next save radius tag into the TagRadius structure */
				if ( same_ic(attrib_info->tag[natt], TagRadius.tag) )
					{
					TagRadius.attname = safe_strdup(attrib_info->attname[natt]);
					TagRadius.attunit = attrib_info->attunit[natt];
					attok = TRUE;
					}

				/* Next save diameter tag into the TagDiameter structure */
				else if ( same_ic(attrib_info->tag[natt], TagDiameter.tag) )
					{
					TagDiameter.attname = safe_strdup(attrib_info->attname[natt]);
					TagDiameter.attunit = attrib_info->attunit[natt];
					attok = TRUE;
					}

				/* Structure for attributes output to metafile */
				for ( nn=0; nn<attrib_meta->nattribs; nn++ )
					{
					if ( !same_ic(attrib_info->tag[natt], attrib_meta->tag[nn]) )
						continue;

					/* Ensure that attribute units match (exactly!) */
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
			if ( NumTagAtt < attrib_meta->nattribs )
				{
				for ( natt=0; natt<attrib_meta->nattribs; natt++ )
					{
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

		case FpaC_LINE:

			/* Identify the attribute tags and save them into structures */
			attrib_info = fields->attinfo[0];
			attrib_meta = metafiles->attinfo[0];
			if (DebugMode)
				{
				(void) fprintf(stderr,
						"%s Identifying %d attributes for LINE objects\n",
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

				/* Save radius tag into the TagRadius structure */
				if ( same_ic(attrib_info->tag[natt], TagRadius.tag) )
					{
					TagRadius.attname = safe_strdup(attrib_info->attname[natt]);
					TagRadius.attunit = attrib_info->attunit[natt];
					attok = TRUE;
					}

				/* Next save diameter tag into the TagDiameter structure */
				else if ( same_ic(attrib_info->tag[natt], TagDiameter.tag) )
					{
					TagDiameter.attname = safe_strdup(attrib_info->attname[natt]);
					TagDiameter.attunit = attrib_info->attunit[natt];
					attok = TRUE;
					}

				/* Next save distance left tag into the TagDistLeft structure */
				else if ( same_ic(attrib_info->tag[natt], TagDistLeft.tag) )
					{
					TagDistLeft.attname = safe_strdup(attrib_info->attname[natt]);
					TagDistLeft.attunit = attrib_info->attunit[natt];
					attok = TRUE;
					}

				/* Next save distance right tag into the TagDistRight structure */
				else if ( same_ic(attrib_info->tag[natt], TagDistRight.tag) )
					{
					TagDistRight.attname = safe_strdup(attrib_info->attname[natt]);
					TagDistRight.attunit = attrib_info->attunit[natt];
					attok = TRUE;
					}

				/* Structure for attributes output to metafile */
				for ( nn=0; nn<attrib_meta->nattribs; nn++ )
					{
					if ( !same_ic(attrib_info->tag[natt], attrib_meta->tag[nn]) )
						continue;

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
			if ( NumTagAtt < attrib_meta->nattribs )
				{
				for ( natt=0; natt<attrib_meta->nattribs; natt++ )
					{
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

		case FpaC_LCHAIN:

			/* Identify the attribute tags and save them into structures */
			attrib_info = fields->attinfo[0];
			attrib_meta = metafiles->attinfo[0];
			if (DebugMode)
				{
				(void) fprintf(stderr,
						"%s Identifying %d attributes for LCHAIN objects\n",
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
					if ( !same_ic(attrib_info->tag[natt], attrib_meta->tag[nn]) )
						continue;

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

			/* Identify the node attribute tags and save them into structures */
			node_info = fields->nodeinfo[0];
			if (DebugMode)
				{
				(void) fprintf(stderr,
						"%s Identifying %d node attributes for LCHAIN objects\n",
						MyLabel, node_info->nattribs);
				}
			for ( natt=0; natt<node_info->nattribs; natt++ )
				{
				attok = FALSE;
				if (DebugMode)
					{
					(void) fprintf(stderr,
							"%s   Comparing node attribute tag: %s\n",
							MyLabel, node_info->tag[natt]);
					}

				/* First save direction tags into the TagDirs structure */
				for ( nn=0; nn<NumTagDirs; nn++ )
					{
					if ( !same_ic(node_info->tag[natt], TagDirs[nn].tag) )
						continue;
					TagDirs[nn].attname = safe_strdup(node_info->attname[natt]);
					TagDirs[nn].attunit = node_info->attunit[natt];
					break;
					}
				if ( nn < NumTagDirs ) attok = TRUE;

				/* Save orientation tag into the TagOrient structure */
				if ( same_ic(node_info->tag[natt], TagOrient.tag) )
					{
					TagOrient.attname = safe_strdup(node_info->attname[natt]);
					TagOrient.attunit = node_info->attunit[natt];
					attok = TRUE;
					}

				/* Save radius tag into the TagRadius structure */
				else if ( same_ic(node_info->tag[natt], TagRadius.tag) )
					{
					TagRadius.attname = safe_strdup(node_info->attname[natt]);
					TagRadius.attunit = node_info->attunit[natt];
					attok = TRUE;
					}

				/* Next save diameter tag into the TagDiameter structure */
				else if ( same_ic(node_info->tag[natt], TagDiameter.tag) )
					{
					TagDiameter.attname = safe_strdup(node_info->attname[natt]);
					TagDiameter.attunit = node_info->attunit[natt];
					attok = TRUE;
					}

				/* Next save distance left tag into the TagDistLeft structure */
				else if ( same_ic(node_info->tag[natt], TagDistLeft.tag) )
					{
					TagDistLeft.attname = safe_strdup(node_info->attname[natt]);
					TagDistLeft.attunit = node_info->attunit[natt];
					attok = TRUE;
					}

				/* Next save distance right tag into the TagDistRight structure */
				else if ( same_ic(node_info->tag[natt], TagDistRight.tag) )
					{
					TagDistRight.attname = safe_strdup(node_info->attname[natt]);
					TagDistRight.attunit = node_info->attunit[natt];
					attok = TRUE;
					}

				/* Structure for attributes output to metafile */
				for ( nn=0; nn<attrib_meta->nattribs; nn++ )
					{
					if ( !same_ic(node_info->tag[natt], attrib_meta->tag[nn]) )
						continue;

					/* Check that units match exactly! */
					if ( !same_ic(node_info->attunit[natt]->name,
									attrib_meta->attunit[nn]->name) )
						{
						(void) fprintf(stderr,
								"%s Units do not match for required_fields / metafiles tag: %s\n",
								MyLabel, attrib_meta->tag[natt]);
						(void) fprintf(stderr,
								"%s   Units for required_fields: %s  for metafiles: %s\n",
								MyLabel, node_info->attunit[natt]->name,
								attrib_meta->attunit[nn]->name);
						continue;
						}

					/* Save the information in the TagNAtts structure */
					ntx = NumTagNAtt++;
					TagNAtts = GETMEM(TagNAtts, TAG_INFO, NumTagNAtt);
					TagNAtts[ntx].tag     = safe_strdup(node_info->tag[natt]);
					TagNAtts[ntx].attname = safe_strdup(node_info->attname[natt]);
					TagNAtts[ntx].attunit = node_info->attunit[natt];
					break;
					}
				if ( nn < attrib_meta->nattribs ) attok = TRUE;

				/* Error if node attribute not recognized */
				if ( !attok )
					{
					(void) fprintf(stderr,
							"%s     Unacceptable required_fields node_attribute tag: %s\n",
							MyLabel, node_info->tag[natt]);
					}
				}

			/* Check that all metafile attribute tags have been entered */
			if ( NumTagAtt + NumTagNAtt < attrib_meta->nattribs )
				{
				for ( natt=0; natt<attrib_meta->nattribs; natt++ )
					{
					for ( nn=0; nn<NumTagAtt; nn++ )
						{
						if ( same_ic(attrib_meta->tag[natt], TagAtts[nn].tag) )
							break;
						}
					if ( nn < NumTagAtt ) continue;
					for ( nn=0; nn<NumTagNAtt; nn++ )
						{
						if ( same_ic(attrib_meta->tag[natt], TagNAtts[nn].tag) )
							break;
						}
					if ( nn < NumTagNAtt ) continue;

					/* Error if attribute not recognized */
					(void) fprintf(stderr,
							"%s     Unacceptable metafiles attribute tag: %s\n",
							MyLabel, attrib_meta->tag[natt]);
					}
				}
			break;
		}

	/* Check for necessary attributes based on required field type */
	switch (fkind)
		{

		/* Check for necessary attributes for scattered points */
		case FpaC_SCATTERED:

			/* Need to have Radius or Diameter or 3 or more TagDirs */
			if ( !blank(TagRadius.attname) )   break;
			if ( !blank(TagDiameter.attname) ) break;
			for ( natt=0, nn=0; nn<NumTagDirs; nn++ )
				if ( !blank(TagDirs[nn].attname) ) natt++;
			if ( natt >= 3 )                   break;

			/* Error if necessary attributes not found */
			(void) fprintf(stderr,
					"%s No attributes %s or %s or (starting with) %s\n",
					MyLabel, Radius, Diameter, PointStart);
			(void) fprintf(stderr,
					"%s      for Allied Model \"%s %s\"\n",
					MyLabel, amodel, subarea);
			(void) fprintf(stdout, "%s Aborted\n", MyLabel);
			return (-1);

		/* Check for necessary attributes for curves */
		case FpaC_LINE:

			/* Need to have Radius or Diameter or DistLeft/DistRight */
			if ( !blank(TagRadius.attname) )          break;
			if ( !blank(TagDiameter.attname) )        break;
			if ( !blank(TagDistLeft.attname)
					&& !blank(TagDistRight.attname) ) break;

			/* Error if necessary attributes not found */
			(void) fprintf(stderr,
					"%s No attributes %s or %s or %s/%s\n",
					MyLabel, Radius, Diameter, DistLeft, DistRight);
			(void) fprintf(stderr,
					"%s      for Allied Model \"%s %s\"\n",
					MyLabel, amodel, subarea);
			(void) fprintf(stdout, "%s Aborted\n", MyLabel);
			return (-1);

		/* Check for necessary attributes for link chains */
		case FpaC_LCHAIN:

			/* Need to have Radius or Diameter or DistLeft/DistRight */
			/*  or 3 or more TagDirs                                 */
			if ( !blank(TagRadius.attname) )          break;
			if ( !blank(TagDiameter.attname) )        break;
			if ( !blank(TagDistLeft.attname)
					&& !blank(TagDistRight.attname) ) break;
			for ( natt=0, nn=0; nn<NumTagDirs; nn++ )
				if ( !blank(TagDirs[nn].attname) ) natt++;
			if ( natt >= 3 )                          break;

			/* Error if necessary attributes not found */
			(void) fprintf(stderr,
					"%s No attributes %s or %s or %s/%s or (starting with) %s\n",
					MyLabel, Radius, Diameter, DistLeft, DistRight, PointStart);
			(void) fprintf(stderr,
					"%s      for Allied Model \"%s %s\"\n",
					MyLabel, amodel, subarea);
			(void) fprintf(stdout, "%s Aborted\n", MyLabel);
			return (-1);
		}

	if (DebugMode)
		{
		for ( nn=0; nn<NumTagDirs; nn++ )
			{
			if ( blank(TagDirs[nn].attname) ) continue;
			(void) fprintf(stderr,
					"%s   Info for direction tag: %s  attname: %s  attunits: %s\n",
					MyLabel, TagDirs[nn].tag, TagDirs[nn].attname,
					TagDirs[nn].attunit->name);
			}
		if ( !blank(TagOrient.attname) )
			{
			(void) fprintf(stderr,
					"%s   Info for orientation tag: %s  attname: %s  attunits: %s\n",
					MyLabel, TagOrient.tag, TagOrient.attname,
					TagOrient.attunit->name);
			}
		if ( !blank(TagRadius.attname) )
			{
			(void) fprintf(stderr,
					"%s   Info for radius tag: %s  attname: %s  attunits: %s\n",
					MyLabel, TagRadius.tag, TagRadius.attname,
					TagRadius.attunit->name);
			}
		if ( !blank(TagDiameter.attname) )
			{
			(void) fprintf(stderr,
					"%s   Info for diameter tag: %s  attname: %s  attunits: %s\n",
					MyLabel, TagDiameter.tag, TagDiameter.attname,
					TagDiameter.attunit->name);
			}
		if ( !blank(TagDistLeft.attname) )
			{
			(void) fprintf(stderr,
					"%s   Info for distance left tag: %s  attname: %s  attunits: %s\n",
					MyLabel, TagDistLeft.tag, TagDistLeft.attname,
					TagDistLeft.attunit->name);
			}
		if ( !blank(TagDistRight.attname) )
			{
			(void) fprintf(stderr,
					"%s   Info for distance right tag: %s  attname: %s  attunits: %s\n",
					MyLabel, TagDistRight.tag, TagDistRight.attname,
					TagDistRight.attunit->name);
			}
		for ( nn=0; nn<NumTagAtt; nn++ )
			{
			(void) fprintf(stderr,
					"%s   Info for attribute tag: %s  attname: %s  attunits: %s\n",
					MyLabel, TagAtts[nn].tag, TagAtts[nn].attname,
					TagAtts[nn].attunit->name);
			}
		for ( nn=0; nn<NumTagNAtt; nn++ )
			{
			(void) fprintf(stderr,
					"%s   Info for node attribute tag: %s  attname: %s  attunits: %s\n",
					MyLabel, TagNAtts[nn].tag, TagNAtts[nn].attname,
					TagNAtts[nn].attunit->name);
			}
		}

	/* Create an SET Object to hold the created areas */
	aset = create_set("area");

	/* Create working lines for portions of the area boundaries */
	lline = create_line();
	rline = create_line();
	scap  = create_line();
	ecap  = create_line();
	span  = create_line();
	lspan = create_line();
	rspan = create_line();

	/* Create areas based on points in SCATTERED type fields */
	if ( fkind == FpaC_SCATTERED )
		{

		/* Create an area for each object in the field */
		first = TRUE;
		for ( nset=0; nset<set->num; nset++ )
			{
			spot = (SPOT) set->list[nset];
			if ( IsNull(spot) ) continue;
			cal = CAL_duplicate((CAL) spot->attrib);
			CAL_add_location(cal, mproj, spot->anchor);

			/* Save the latitude/longitude and valid time with special names */
			val = CAL_get_attribute(cal, CALlatitude);
			CAL_add_attribute(cal, AttribReferenceLatitude, val);
			val = CAL_get_attribute(cal, CALlongitude);
			CAL_add_attribute(cal, AttribReferenceLongitude, val);
			CAL_add_attribute(cal, AttribReferenceTime, vtime);

			area = create_area("", "", "");
			bdry = create_line();

			/* Create boundary based on radius or diameter at spot location */
			if ( !blank(TagRadius.attname) || !blank(TagDiameter.attname) )
				{

				/* Determine distance about spot location */
				if ( !blank(TagRadius.attname) )
					{
					val = CAL_get_attribute(cal, TagRadius.attname);
					(void) convert_value(TagRadius.attunit->name, atof(val),
											DistanceUnitsM, &dval);
					xrad = (float) dval;
					}
				else
					{
					val = CAL_get_attribute(cal, TagDiameter.attname);
					(void) convert_value(TagDiameter.attunit->name, atof(val),
											DistanceUnitsM, &dval);
					xrad = (float) dval / 2.0;
					}

				/* Loop through directions to determine boundary locations */
				for ( nn=0; nn<NumTagDirs; nn++ )
					{
					ldir = TagDirs[nn].dir;
					(void) great_circle_span(mproj, spot->anchor, ldir, xrad,
																		epos);
					add_point_to_line(bdry, epos);
					}
				}

			/* Create boundary based on distances for set directions */
			else
				{

				/* Loop through directions to extract boundary locations */
				for ( nn=0; nn<NumTagDirs; nn++ )
					{

					/* Skip directions which do not have attribute values */
					if ( blank(TagDirs[nn].attname) ) continue;

					/* Extract the attribute value at set directions */
					ldir = TagDirs[nn].dir;
					val  = CAL_get_attribute(cal, TagDirs[nn].attname);
					(void) convert_value(TagDirs[nn].attunit->name, atof(val),
											DistanceUnitsM, &dval);
					xrad = (float) dval;
					(void) great_circle_span(mproj, spot->anchor, ldir, xrad,
																		epos);
					add_point_to_line(bdry, epos);
					}
				}

			/* Close boundary and skip duplicate locations */
			close_line(bdry);
			condense_line(bdry);

			/* Error if not enough points added to boundary */
			if ( bdry->numpts < 3 )
				{
				(void) fprintf(stderr,
						"%s No area created for spot %d in field \"%s\" \"%s\" at \"%s\"\n",
						MyLabel, nset,
						fdef->element->name, fdef->level->name, vtime);
				continue;
				}

			/* Ensure that area boundary does not cross over itself */
			while ( looped_line_crossing(bdry, spos, &ips, &ipe) )
				{
				if (DebugMode)
					{
					(void) fprintf(stderr,
							"%s Area crossover between segments: %d and %d (0 to %d)\n",
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
			empty_line(bdry);
			append_line(bdry, lines[0]);

			/* Add the area boundary to the AREA Object */
			define_area_boundary(area, bdry);

			/* Add the metafile attributes to the AREA Object */
			acal = CAL_create_by_name(fdesc.edef->name, fdesc.ldef->name);
			for ( natt=0; natt<attrib_meta->nattribs; natt++ )
				{
				if (DebugMode)
					{
					(void) fprintf(stderr,
							"%s Find matching attribute for metafile tag: %s\n",
							MyLabel, attrib_meta->tag[natt]);
					}

				/* Find the matching attribute tag */
				for ( nn=0; nn<NumTagAtt; nn++ )
					{
					if ( same_ic(attrib_meta->tag[natt], TagAtts[nn].tag) ) break;
					}
				if ( nn >= NumTagAtt ) continue;

				/* Extract attribute value from object attributes */
				val = CAL_get_attribute(cal, TagAtts[nn].attname);

				/* Add attribute value to metafile attributes */
				CAL_add_attribute(acal, attrib_meta->attname[natt], val);
				if (DebugMode)
					{
					(void) fprintf(stderr,
							"%s Adding metafile attribute: %s  with value: %s\n",
							MyLabel, attrib_meta->attname[natt], val);
					}
				}

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

			/* Add the attributes to the area */
			define_area_attribs(area, acal);

			/* Add the AREA Object to the set */
			(void) add_item_to_set(aset, (ITEM) area);
			(void) fprintf(stderr,
					"%s Adding area for object: %d  Boundary points: %d\n",
					MyLabel, nset, area->bound->boundary->numpts);
			first = FALSE;
			}
		}

	/* Create areas based on lines in LINE type fields */
	else if ( fkind == FpaC_LINE )
		{

		/* Create an area for each object in the field */
		first = TRUE;
		for ( nset=0; nset<set->num; nset++ )
			{
			curve = (CURVE) set->list[nset];
			if ( IsNull(curve) )           continue;
			if ( IsNull(curve->line) )     continue;
			if ( curve->line->numpts < 2 ) continue;
			cal = CAL_duplicate((CAL) curve->attrib);
			CAL_add_line_len(cal, mproj, curve->line);

			/* Save the valid time with a special name */
			CAL_add_attribute(cal, AttribReferenceTime, vtime);

			area = create_area("", "", "");
			bdry = create_line();

			/* Determine distances left/right of line based on radius */
			if ( !blank(TagRadius.attname) )
				{
				val = CAL_get_attribute(cal, TagRadius.attname);
				(void) convert_value(TagRadius.attunit->name, atof(val),
										DistanceUnitsM, &dval);
				xleft = xright = (float) dval;

				if (DebugMode)
					{
					(void) fprintf(stderr,
							"%s Extract: %s from attribute: %s %s\n",
							MyLabel, val, TagRadius.attname,
							TagRadius.attunit->name);
					(void) fprintf(stderr,
							"%s   Radius: %.2f %s\n",
							MyLabel, xleft, DistanceUnitsM);
					}
				}

			/* Determine distances left/right of line based on diameter */
			if ( !blank(TagDiameter.attname) )
				{
				val = CAL_get_attribute(cal, TagDiameter.attname);
				(void) convert_value(TagDiameter.attunit->name, atof(val),
										DistanceUnitsM, &dval);
				xleft = xright = (float) dval / 2.0;

				if (DebugMode)
					{
					(void) fprintf(stderr,
							"%s Extract: %s from attribute: %s %s\n",
							MyLabel, val, TagDiameter.attname,
							TagDiameter.attunit->name);
					(void) fprintf(stderr,
							"%s   Radius: %.2f %s\n",
							MyLabel, xleft, DistanceUnitsM);
					}
				}

			/* Determine distances left/right of line */
			if ( !blank(TagDistLeft.attname) && !blank(TagDistRight.attname) )
				{
				lval = CAL_get_attribute(cal, TagDistLeft.attname);
				(void) convert_value(TagDistLeft.attunit->name, atof(lval),
										DistanceUnitsM, &dval);
				xleft  = (float) dval;
				rval = CAL_get_attribute(cal, TagDistRight.attname);
				(void) convert_value(TagDistRight.attunit->name, atof(rval),
										DistanceUnitsM, &dval);
				xright = (float) dval;

				if (DebugMode)
					{
					(void) fprintf(stderr,
							"%s Extract: %s from attribute: %s %s\n",
							MyLabel, lval, TagDistLeft.attname,
							TagDistLeft.attunit->name);
					(void) fprintf(stderr,
							"%s Extract: %s from attribute: %s %s\n",
							MyLabel, rval, TagDistRight.attname,
							TagDistRight.attunit->name);
					(void) fprintf(stderr,
							"%s   DistLeft/DistRight: %.2f/%.2f %s\n",
							MyLabel, xleft, xright, DistanceUnitsM);
					}
				}

			/* Make a condensed copy of the line */
			xline = copy_line(curve->line);
			condense_line(xline);

			/* Prepare work objects */
			(void) empty_line(lline);
			(void) empty_line(rline);
			(void) empty_line(scap);
			(void) empty_line(ecap);
			(void) empty_line(lspan);
			(void) empty_line(rspan);

			/* Loop through points on curve to determine boundary locations */
			for ( nn=0; nn<xline->numpts; nn++ )
				{
				copy_point(spos, xline->points[nn]);

				/* Determine line direction at this point */
				if ( nn < xline->numpts-1 )
					{
					copy_point(xpos, xline->points[nn]);
					copy_point(epos, xline->points[nn+1]);
					}
				else
					{
					copy_point(xpos, xline->points[nn-1]);
					copy_point(epos, xline->points[nn]);
					}
				ldir = great_circle_bearing(mproj, xpos, epos);

				/* Add locations for start cap */
				if ( nn == 0 )
					{
					for ( natt=0; natt<NumSCapDirs; natt++ )
						{
						xdir = SCapDirs[natt].dir;
						xavg = (xleft + xright) / 2.0;
						if      ( xdir < 180.0 && xright <= 0.0 ) continue;
						else if ( xdir < 180.0 )
							{
							xdist = hypot((xright * fpa_cosdeg(xdir - 90.0)),
											(xavg * fpa_sindeg(xdir - 90.0)));
							}
						else if ( xdir > 180.0 && xleft <= 0.0 ) continue;
						else if ( xdir > 180.0 )
							{
							xdist = hypot((xleft * fpa_cosdeg(270.0 - xdir)),
											(xavg * fpa_sindeg(270.0 - xdir)));
							}
						else
							{
							xdist = xavg;
							}
						(void) great_circle_span(mproj, spos,
													ldir + xdir, xdist, epos);
						add_point_to_line(scap, epos);
						if (DebugMode)
							{
							(void) fprintf(stderr,
									"%s Start cap point: %d  ldir/xdir: %.2f/%.2f  epos: %.2f/%.2f\n",
									MyLabel, natt, ldir, xdir, epos[X], epos[Y]);
							}
						}
					}

				/* Add locations to left of line                      */
				/*  ... as long as they do not cross over themselves! */
				(void) great_circle_span(mproj, spos, (ldir-90), xleft, epos);
				empty_line(span);
				add_point_to_line(span, spos);
				add_point_to_line(span, epos);
				if ( !find_line_crossing(span, lspan, 0, spos,
						NullPoint, NullInt, NullInt, NullLogical) )
					{
					add_point_to_line(lline, epos);
					}

				/* Save this span to check the next one */
				empty_line(lspan);
				append_line(lspan, span);

				/* Add locations to right of line                     */
				/*  ... as long as they do not cross over themselves! */
				(void) great_circle_span(mproj, spos, (ldir+90), xright, epos);
				empty_line(span);
				add_point_to_line(span, spos);
				add_point_to_line(span, epos);
				if ( !find_line_crossing(span, rspan, 0, spos,
						NullPoint, NullInt, NullInt, NullLogical) )
					{
					add_point_to_line(rline, epos);
					}

				/* Save this span to check the next one */
				empty_line(rspan);
				append_line(rspan, span);

				/* Add locations for end cap */
				if ( nn == xline->numpts-1 )
					{
					for ( natt=0; natt<NumECapDirs; natt++ )
						{
						xdir = ECapDirs[natt].dir;
						xavg = (xleft + xright) / 2.0;
						if      ( xdir < 0.0 && xleft <= 0.0 ) continue;
						else if ( xdir < 0.0 )
							{
							xdist = hypot((-xleft * fpa_sindeg(xdir)),
											(xavg * fpa_cosdeg(xdir)));
							}
						else if ( xdir > 0.0 && xright <= 0.0 ) continue;
						else if ( xdir > 0.0 )
							{
							xdist = hypot((xright * fpa_sindeg(xdir)),
											(xavg * fpa_cosdeg(xdir)));
							}
						else
							{
							xdist = xavg;
							}
						(void) great_circle_span(mproj, spos,
													ldir + xdir, xdist, epos);
						add_point_to_line(ecap, epos);
						if (DebugMode)
							{
							(void) fprintf(stderr,
									"%s Start cap point: %d  ldir/xdir: %.2f/%.2f  epos: %.2f/%.2f\n",
									MyLabel, natt, ldir, xdir, epos[X], epos[Y]);
							}
						}
					}
				}

			/* Destroy the condensed copy of the line */
			destroy_line(xline);

			/* Build the boundary from the pieces */
			bdry = append_line_dir(bdry, scap,  TRUE);
			bdry = append_line_dir(bdry, lline, TRUE);
			bdry = append_line_dir(bdry, ecap,  TRUE);
			bdry = append_line_dir(bdry, rline, FALSE);
			close_line(bdry);
			condense_line(bdry);

			/* Error if not enough points added to boundary */
			if ( bdry->numpts < 3 )
				{
				(void) fprintf(stderr,
						"%s No area created for line %d in field \"%s\" \"%s\" at \"%s\"\n",
						MyLabel, nset,
						fdef->element->name, fdef->level->name, vtime);
				continue;
				}

			/* Ensure that area boundary does not cross over itself */
			while ( looped_line_crossing(bdry, spos, &ips, &ipe) )
				{
				if (DebugMode)
					{
					(void) fprintf(stderr,
							"%s Area crossover between segments: %d and %d (0 to %d)\n",
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
			empty_line(bdry);
			append_line(bdry, lines[0]);

			/* Add the area boundary to the AREA Object */
			define_area_boundary(area, bdry);

			/* Add the metafile attributes to the AREA Object */
			acal = CAL_create_by_name(fdesc.edef->name, fdesc.ldef->name);
			for ( natt=0; natt<attrib_meta->nattribs; natt++ )
				{
				if (DebugMode)
					{
					(void) fprintf(stderr,
							"%s Find matching attribute for metafile tag: %s\n",
							MyLabel, attrib_meta->tag[natt]);
					}

				/* Find the matching attribute tag */
				for ( nn=0; nn<NumTagAtt; nn++ )
					{
					if ( same_ic(attrib_meta->tag[natt], TagAtts[nn].tag) ) break;
					}
				if ( nn >= NumTagAtt ) continue;

				/* Extract attribute value from object attributes */
				val = CAL_get_attribute(cal, TagAtts[nn].attname);

				/* Add attribute value to metafile attributes */
				CAL_add_attribute(acal, attrib_meta->attname[natt], val);
				if (DebugMode)
					{
					(void) fprintf(stderr,
							"%s Adding metafile attribute: %s  with value: %s\n",
							MyLabel, attrib_meta->attname[natt], val);
					}
				}

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

			/* Add the attributes to the area */
			define_area_attribs(area, acal);

			/* Add the AREA Object to the set */
			(void) add_item_to_set(aset, (ITEM) area);
			(void) fprintf(stderr,
					"%s Adding area for object: %d  Boundary points: %d\n",
					MyLabel, nset, area->bound->boundary->numpts);
			first = FALSE;
			}
		}

	/* Create areas based on tracks in LCHAIN type fields */
	else if ( fkind == FpaC_LCHAIN && use_track )
		{

		/* Create an area for each object in the field */
		first = TRUE;
		for ( nset=0; nset<set->num; nset++ )
			{
			lchain = (LCHAIN) set->list[nset];
			if ( IsNull(lchain) )   continue;
			if ( lchain->dointerp ) (void) interpolate_lchain(lchain);
			if ( lchain->inum < 1 ) continue;
			cal = CAL_duplicate((CAL) lchain->attrib);

			/* Save the reference time with a special name */
			CAL_add_attribute(cal, AttribReferenceTime, lchain->xtime);

			area = create_area("", "", "");
			bdry = create_line();

			/* Convert start and end times for testing */
			if ( !blank(stime) )
				{
				svt   = interpret_timestring(stime, lchain->xtime, Clon);
				splus = calc_prog_time_minutes(lchain->xtime, svt, &pok);
				}
			else
				{
				splus = lchain->splus;
				}
			if ( !blank(etime) )
				{
				evt   = interpret_timestring(etime, lchain->xtime, Clon);
				eplus = calc_prog_time_minutes(lchain->xtime, evt, &pok);
				}
			else
				{
				eplus = lchain->eplus;
				}

			/* Prepare work objects */
			(void) empty_line(lline);
			(void) empty_line(rline);
			(void) empty_line(scap);
			(void) empty_line(ecap);
			(void) empty_line(lspan);
			(void) empty_line(rspan);

			/* Loop through interpolated nodes on link chain */
			if ( use_interp )
				{
				startcap = TRUE;
				for ( inode=0; inode<lchain->inum; inode++ )
					{
					linterp = lchain->interps[inode];

					if (DebugMode)
						{
						if ( linterp->mplus >= splus && linterp->mplus <= eplus )
							{
							(void) fprintf(stderr, "%s Node: %d at time: %d\n",
									MyLabel, inode, linterp->mplus);
							debug_attrib_list("LchainNodes", linterp->attrib);
							}
						else
							{
							(void) fprintf(stderr, "%s Skip node: %d at time: %d (%d to %d)\n",
									MyLabel, inode, linterp->mplus, splus, eplus);
							}
						}

					/* Skip nodes outside time range */
					if ( linterp->mplus < splus ) continue;
					if ( linterp->mplus > eplus ) continue;

					/* Set node parameters */
					copy_point(spos, linterp->node);
					ncal = CAL_duplicate((CAL) linterp->attrib);
					CAL_add_lchain_node_motion(ncal, mproj, lchain,
																linterp->mplus);

					/* Determine orientation at node from attribute */
					if ( !blank(TagOrient.attname) )
						{
						val = CAL_get_attribute(ncal, TagOrient.attname);
						ldir = (float) atof(val);

						if (DebugMode)
							{
							(void) fprintf(stderr,
									"%s Extract: %s from attribute: %s\n",
									MyLabel, val, TagOrient.attname);
							(void) fprintf(stderr,
									"%s   Orientation: %.2f degrees True\n",
									MyLabel, ldir);
							}
						}

					/* Determine orientation at node from track direction */
					else
						{
						val  = CAL_get_attribute(ncal, AttribLnodeDirection);
						ldir = (float) atof(val);

						if (DebugMode)
							{
							(void) fprintf(stderr,
									"%s Extract: %s from attribute: %s\n",
									MyLabel, val, AttribLnodeDirection);
							(void) fprintf(stderr,
									"%s   Orientation: %.2f degrees True\n",
									MyLabel, ldir);
							}
						}

					/* Determine distances left/right of node based on radius */
					if ( !blank(TagRadius.attname) )
						{
						val = CAL_get_attribute(ncal, TagRadius.attname);
						(void) convert_value(TagRadius.attunit->name, atof(val),
												DistanceUnitsM, &dval);
						xleft = xright = (float) dval;

						if (DebugMode)
							{
							(void) fprintf(stderr,
									"%s Extract: %s from attribute: %s %s\n",
									MyLabel, val, TagRadius.attname,
									TagRadius.attunit->name);
							(void) fprintf(stderr,
									"%s   Radius: %.2f %s\n",
									MyLabel, xleft, DistanceUnitsM);
							}
						}

					/* Determine distances left/right of node based on diameter */
					if ( !blank(TagDiameter.attname) )
						{
						val = CAL_get_attribute(ncal, TagDiameter.attname);
						(void) convert_value(TagDiameter.attunit->name, atof(val),
												DistanceUnitsM, &dval);
						xleft = xright = (float) dval / 2.0;

						if (DebugMode)
							{
							(void) fprintf(stderr,
									"%s Extract: %s from attribute: %s %s\n",
									MyLabel, val, TagDiameter.attname,
									TagDiameter.attunit->name);
							(void) fprintf(stderr,
									"%s   Radius: %.2f %s\n",
									MyLabel, xleft, DistanceUnitsM);
							}
						}

					/* Determine distances left/right of track */
					if ( !blank(TagDistLeft.attname)
							&& !blank(TagDistRight.attname) )
						{
						lval = CAL_get_attribute(ncal, TagDistLeft.attname);
						(void) convert_value(TagDistLeft.attunit->name,
										atof(lval), DistanceUnitsM, &dval);
						xleft  = (float) dval;
						rval = CAL_get_attribute(ncal, TagDistRight.attname);
						(void) convert_value(TagDistRight.attunit->name,
										atof(rval), DistanceUnitsM, &dval);
						xright = (float) dval;

						if (DebugMode)
							{
							(void) fprintf(stderr,
									"%s Extract: %s from attribute: %s %s\n",
									MyLabel, lval, TagDistLeft.attname,
									TagDistLeft.attunit->name);
							(void) fprintf(stderr,
									"%s Extract: %s from attribute: %s %s\n",
									MyLabel, rval, TagDistRight.attname,
									TagDistRight.attunit->name);
							(void) fprintf(stderr,
									"%s   DistLeft/DistRight: %.2f/%.2f %s\n",
									MyLabel, xleft, xright, DistanceUnitsM);
							}
						}

					/* Add locations for start cap (using first point) */
					if ( startcap )
						{
						for ( natt=0; natt<NumSCapDirs; natt++ )
							{
							xdir = SCapDirs[natt].dir;
							xavg = (xleft + xright) / 2.0;
							if      ( xdir < 180.0 && xright <= 0.0 ) continue;
							else if ( xdir < 180.0 )
								{
								xdist = hypot((xright * fpa_cosdeg(xdir - 90.0)),
												(xavg * fpa_sindeg(xdir - 90.0)));
								}
							else if ( xdir > 180.0 && xleft <= 0.0 ) continue;
							else if ( xdir > 180.0 )
								{
								xdist = hypot((xleft * fpa_cosdeg(270.0 - xdir)),
												(xavg * fpa_sindeg(270.0 - xdir)));
								}
							else
								{
								xdist = xavg;
								}
							(void) great_circle_span(mproj, spos,
														ldir + xdir, xdist, epos);
							add_point_to_line(scap, epos);
							}
						startcap = FALSE;
						}

					/* Add locations to left of line                      */
					/*  ... as long as they do not cross over themselves! */
					(void) great_circle_span(mproj, spos, (ldir-90), xleft, epos);
					empty_line(span);
					add_point_to_line(span, spos);
					add_point_to_line(span, epos);
					if ( !find_line_crossing(span, lspan, 0, spos,
							NullPoint, NullInt, NullInt, NullLogical) )
						{
						add_point_to_line(lline, epos);
						}

					/* Save this span to check the next one */
					empty_line(lspan);
					append_line(lspan, span);

					/* Add locations to right of line                     */
					/*  ... as long as they do not cross over themselves! */
					(void) great_circle_span(mproj, spos, (ldir+90), xright, epos);
					empty_line(span);
					add_point_to_line(span, spos);
					add_point_to_line(span, epos);
					if ( !find_line_crossing(span, rspan, 0, spos,
							NullPoint, NullInt, NullInt, NullLogical) )
						{
						add_point_to_line(rline, epos);
						}

					/* Save this span to check the next one */
					empty_line(rspan);
					append_line(rspan, span);

					/* Free node parameters */
					(void) CAL_destroy(ncal);
					}

				/* Add locations for end cap (only if start cap was set) */
				if ( !startcap )
					{
					for ( natt=0; natt<NumECapDirs; natt++ )
						{
						xdir = ECapDirs[natt].dir;
						xavg = (xleft + xright) / 2.0;
						if      ( xdir < 0.0 && xleft <= 0.0 ) continue;
						else if ( xdir < 0.0 )
							{
							xdist = hypot((-xleft * fpa_sindeg(xdir)),
											(xavg * fpa_cosdeg(xdir)));
							}
						else if ( xdir > 0.0 && xright <= 0.0 ) continue;
						else if ( xdir > 0.0 )
							{
							xdist = hypot((xright * fpa_sindeg(xdir)),
											(xavg * fpa_cosdeg(xdir)));
							}
						else
							{
							xdist = xavg;
							}
						(void) great_circle_span(mproj, spos,
													ldir + xdir, xdist, epos);
						add_point_to_line(ecap, epos);
						}
					}
				}

			/* Loop through normal/floating nodes on link chain */
			else
				{
				startcap = TRUE;
				for ( inode=0; inode<lchain->lnum; inode++ )
					{
					lnode = lchain->nodes[inode];

					if (DebugMode)
						{
						if ( lnode->ltype == LchainUnknown )
							{
							(void) fprintf(stderr, "%s Skip unknown node\n",
									MyLabel);
							}
						else if ( lnode->ltype == LchainNode )
							{
							(void) fprintf(stderr, "%s Check normal node\n",
									MyLabel);
							}
						else if ( lnode->ltype == LchainControl )
							{
							(void) fprintf(stderr, "%s Skip control node\n",
									MyLabel);
							}
						else if ( lnode->ltype == LchainFloating )
							{
							(void) fprintf(stderr, "%s Check floating node\n",
									MyLabel);
							}
						else if ( lnode->ltype == LchainInterp )
							{
							(void) fprintf(stderr, "%s Skip interpolated node\n",
									MyLabel);
							}
						}

					/* Skip nodes that are not normal/floating */
					if ( lnode->ltype != LchainNode
							&& lnode->ltype != LchainFloating ) continue;

					if (DebugMode)
						{
						if ( lnode->mplus >= splus && lnode->mplus <= eplus )
							{
							(void) fprintf(stderr, "%s Node: %d at time: %d\n",
									MyLabel, inode, lnode->mplus);
							debug_attrib_list("LchainNodes", lnode->attrib);
							}
						else
							{
							(void) fprintf(stderr, "%s Skip node: %d at time: %d (%d to %d)\n",
									MyLabel, inode, lnode->mplus, splus, eplus);
							}
						}

					/* Skip nodes outside time range */
					if ( lnode->mplus < splus ) continue;
					if ( lnode->mplus > eplus ) continue;

					/* Set node parameters */
					copy_point(spos, lnode->node);
					ncal = CAL_duplicate((CAL) lnode->attrib);
					CAL_add_lchain_node_motion(ncal, mproj, lchain, lnode->mplus);

					/* Determine orientation at node from attribute */
					if ( !blank(TagOrient.attname) )
						{
						val = CAL_get_attribute(ncal, TagOrient.attname);
						ldir = (float) atof(val);

						if (DebugMode)
							{
							(void) fprintf(stderr,
									"%s Extract: %s from attribute: %s\n",
									MyLabel, val, TagOrient.attname);
							(void) fprintf(stderr,
									"%s   Orientation: %.2f degrees True\n",
									MyLabel, ldir);
							}
						}

					/* Determine orientation at node from track direction */
					else
						{
						val  = CAL_get_attribute(ncal, AttribLnodeDirection);
						ldir = (float) atof(val);

						if (DebugMode)
							{
							(void) fprintf(stderr,
									"%s Extract: %s from attribute: %s\n",
									MyLabel, val, AttribLnodeDirection);
							(void) fprintf(stderr,
									"%s   Orientation: %.2f degrees True\n",
									MyLabel, ldir);
							}
						}

					/* Determine distances left/right of node based on radius */
					if ( !blank(TagRadius.attname) )
						{
						val = CAL_get_attribute(ncal, TagRadius.attname);
						(void) convert_value(TagRadius.attunit->name, atof(val),
												DistanceUnitsM, &dval);
						xleft = xright = (float) dval;

						if (DebugMode)
							{
							(void) fprintf(stderr,
									"%s Extract: %s from attribute: %s %s\n",
									MyLabel, val, TagRadius.attname,
									TagRadius.attunit->name);
							(void) fprintf(stderr,
									"%s   Radius: %.2f %s\n",
									MyLabel, xleft, DistanceUnitsM);
							}
						}

					/* Determine distances left/right of node based on diameter */
					if ( !blank(TagDiameter.attname) )
						{
						val = CAL_get_attribute(ncal, TagDiameter.attname);
						(void) convert_value(TagDiameter.attunit->name, atof(val),
												DistanceUnitsM, &dval);
						xleft = xright = (float) dval / 2.0;

						if (DebugMode)
							{
							(void) fprintf(stderr,
									"%s Extract: %s from attribute: %s %s\n",
									MyLabel, val, TagDiameter.attname,
									TagDiameter.attunit->name);
							(void) fprintf(stderr,
									"%s   Radius: %.2f %s\n",
									MyLabel, xleft, DistanceUnitsM);
							}
						}

					/* Determine distances left/right of track */
					if ( !blank(TagDistLeft.attname)
							&& !blank(TagDistRight.attname) )
						{
						lval = CAL_get_attribute(ncal, TagDistLeft.attname);
						(void) convert_value(TagDistLeft.attunit->name,
										atof(lval), DistanceUnitsM, &dval);
						xleft  = (float) dval;
						rval = CAL_get_attribute(ncal, TagDistRight.attname);
						(void) convert_value(TagDistRight.attunit->name,
										atof(rval), DistanceUnitsM, &dval);
						xright = (float) dval;

						if (DebugMode)
							{
							(void) fprintf(stderr,
									"%s Extract: %s from attribute: %s %s\n",
									MyLabel, lval, TagDistLeft.attname,
									TagDistLeft.attunit->name);
							(void) fprintf(stderr,
									"%s Extract: %s from attribute: %s %s\n",
									MyLabel, rval, TagDistRight.attname,
									TagDistRight.attunit->name);
							(void) fprintf(stderr,
									"%s   DistLeft/DistRight: %.2f/%.2f %s\n",
									MyLabel, xleft, xright, DistanceUnitsM);
							}
						}

					/* Add locations for start cap (using first point) */
					if ( startcap )
						{
						for ( natt=0; natt<NumSCapDirs; natt++ )
							{
							xdir = SCapDirs[natt].dir;
							xavg = (xleft + xright) / 2.0;
							if      ( xdir < 180.0 && xright <= 0.0 ) continue;
							else if ( xdir < 180.0 )
								{
								xdist = hypot((xright * fpa_cosdeg(xdir - 90.0)),
												(xavg * fpa_sindeg(xdir - 90.0)));
								}
							else if ( xdir > 180.0 && xleft <= 0.0 ) continue;
							else if ( xdir > 180.0 )
								{
								xdist = hypot((xleft * fpa_cosdeg(270.0 - xdir)),
												(xavg * fpa_sindeg(270.0 - xdir)));
								}
							else
								{
								xdist = xavg;
								}
							(void) great_circle_span(mproj, spos,
														ldir + xdir, xdist, epos);
							add_point_to_line(scap, epos);
							}
						startcap = FALSE;
						}

					/* Add locations to left of line                      */
					/*  ... as long as they do not cross over themselves! */
					(void) great_circle_span(mproj, spos, (ldir-90), xleft, epos);
					empty_line(span);
					add_point_to_line(span, spos);
					add_point_to_line(span, epos);
					if ( !find_line_crossing(span, lspan, 0, spos,
							NullPoint, NullInt, NullInt, NullLogical) )
						{
						add_point_to_line(lline, epos);
						}

					/* Save this span to check the next one */
					empty_line(lspan);
					append_line(lspan, span);

					/* Add locations to right of line                     */
					/*  ... as long as they do not cross over themselves! */
					(void) great_circle_span(mproj, spos, (ldir+90), xright, epos);
					empty_line(span);
					add_point_to_line(span, spos);
					add_point_to_line(span, epos);
					if ( !find_line_crossing(span, rspan, 0, spos,
							NullPoint, NullInt, NullInt, NullLogical) )
						{
						add_point_to_line(rline, epos);
						}

					/* Save this span to check the next one */
					empty_line(rspan);
					append_line(rspan, span);

					/* Free node parameters */
					(void) CAL_destroy(ncal);
					}

				/* Add locations for end cap (only if start cap was set) */
				if ( !startcap )
					{
					for ( natt=0; natt<NumECapDirs; natt++ )
						{
						xdir = ECapDirs[natt].dir;
						xavg = (xleft + xright) / 2.0;
						if      ( xdir < 0.0 && xleft <= 0.0 ) continue;
						else if ( xdir < 0.0 )
							{
							xdist = hypot((-xleft * fpa_sindeg(xdir)),
											(xavg * fpa_cosdeg(xdir)));
							}
						else if ( xdir > 0.0 && xright <= 0.0 ) continue;
						else if ( xdir > 0.0 )
							{
							xdist = hypot((xright * fpa_sindeg(xdir)),
											(xavg * fpa_cosdeg(xdir)));
							}
						else
							{
							xdist = xavg;
							}
						(void) great_circle_span(mproj, spos,
													ldir + xdir, xdist, epos);
						add_point_to_line(ecap, epos);
						}
					}
				}

			/* Build the boundary from the pieces */
			bdry = append_line_dir(bdry, scap,  TRUE);
			bdry = append_line_dir(bdry, lline, TRUE);
			bdry = append_line_dir(bdry, ecap,  TRUE);
			bdry = append_line_dir(bdry, rline, FALSE);
			close_line(bdry);
			condense_line(bdry);

			/* Error if not enough points added to boundary */
			if ( bdry->numpts < 3 )
				{
				(void) fprintf(stderr,
						"%s No area created for link chain %d in field \"%s\" \"%s\" at \"%s\"\n",
						MyLabel, nset,
						fdef->element->name, fdef->level->name, vtime);
				continue;
				}

			/* Ensure that area boundary does not cross over itself */
			while ( looped_line_crossing(bdry, spos, &ips, &ipe) )
				{
				if (DebugMode)
					{
					(void) fprintf(stderr,
							"%s Area crossover between segments: %d and %d (0 to %d)\n",
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
			empty_line(bdry);
			append_line(bdry, lines[0]);

			/* Add the area boundary to the AREA Object */
			define_area_boundary(area, bdry);

			/* Add the metafile attributes to the AREA Object */
			acal = CAL_create_by_name(fdesc.edef->name, fdesc.ldef->name);
			for ( natt=0; natt<attrib_meta->nattribs; natt++ )
				{
				if (DebugMode)
					{
					(void) fprintf(stderr,
							"%s Find matching attribute for metafile tag: %s\n",
							MyLabel, attrib_meta->tag[natt]);
					}

				/* Find the matching attribute tag */
				for ( nn=0; nn<NumTagAtt; nn++ )
					{
					if ( same_ic(attrib_meta->tag[natt], TagAtts[nn].tag) ) break;
					}
				if ( nn >= NumTagAtt ) continue;

				/* Extract attribute value from object attributes */
				val = CAL_get_attribute(cal, TagAtts[nn].attname);

				/* Add attribute value to metafile attributes */
				CAL_add_attribute(acal, attrib_meta->attname[natt], val);
				if (DebugMode)
					{
					(void) fprintf(stderr,
							"%s Adding metafile attribute: %s  with value: %s\n",
							MyLabel, attrib_meta->attname[natt], val);
					}
				}

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

			/* Add the attributes to the area */
			define_area_attribs(area, acal);

			/* Add the AREA Object to the set */
			(void) add_item_to_set(aset, (ITEM) area);
			(void) fprintf(stderr,
					"%s Adding area for object: %d  Boundary points: %d\n",
					MyLabel, nset, area->bound->boundary->numpts);
			first = FALSE;
			}
		}

	/* Create areas based on nodes in LCHAIN type fields */
	else if ( fkind == FpaC_LCHAIN && !use_track )
		{

		/* Create an area for each node of each object in the field */
		first = TRUE;
		for ( nset=0; nset<set->num; nset++ )
			{
			lchain = (LCHAIN) set->list[nset];
			if ( IsNull(lchain) )   continue;
			if ( lchain->dointerp ) (void) interpolate_lchain(lchain);
			if ( lchain->inum < 1 ) continue;
			cal = CAL_duplicate((CAL) lchain->attrib);

			/* Convert start and end times for testing */
			if ( !blank(stime) )
				{
				svt   = interpret_timestring(stime, lchain->xtime, Clon);
				splus = calc_prog_time_minutes(lchain->xtime, svt, &pok);
				}
			else
				{
				splus = lchain->splus;
				}
			if ( !blank(etime) )
				{
				evt   = interpret_timestring(etime, lchain->xtime, Clon);
				eplus = calc_prog_time_minutes(lchain->xtime, evt, &pok);
				}
			else
				{
				eplus = lchain->eplus;
				}

			/* Loop through interpolated nodes on link chain */
			if ( use_interp )
				{
				for ( inode=0; inode<lchain->inum; inode++ )
					{
					linterp = lchain->interps[inode];

					if (DebugMode)
						{
						if ( linterp->mplus >= splus && linterp->mplus <= eplus )
							{
							(void) fprintf(stderr, "%s Node: %d at time: %d\n",
									MyLabel, inode, linterp->mplus);
							debug_attrib_list("LchainNodes", linterp->attrib);
							}
						else
							{
							(void) fprintf(stderr, "%s Skip node: %d at time: %d (%d to %d)\n",
									MyLabel, inode, linterp->mplus, splus, eplus);
							}
						}

					/* Skip nodes outside time range */
					if ( linterp->mplus < splus ) continue;
					if ( linterp->mplus > eplus ) continue;

					/* Set node parameters */
					copy_point(spos, linterp->node);
					ncal = CAL_duplicate((CAL) linterp->attrib);
					CAL_add_lchain_node_motion(ncal, mproj, lchain,
																linterp->mplus);
					CAL_add_location(ncal, mproj, linterp->node);

					/* Save the latitude/longitude with special names */
					val = CAL_get_attribute(ncal, CALlatitude);
					CAL_add_attribute(ncal, AttribReferenceLatitude, val);
					val = CAL_get_attribute(ncal, CALlongitude);
					CAL_add_attribute(ncal, AttribReferenceLongitude, val);

					/* Save the node valid time with a special name */
					nvt = calc_valid_time_minutes(lchain->xtime, 0,
																linterp->mplus);
					CAL_add_attribute(ncal, AttribReferenceTime, nvt);

					area = create_area("", "", "");
					bdry = create_line();

					/* Create boundary based on radius or diameter at spot location */
					if ( !blank(TagRadius.attname) || !blank(TagDiameter.attname) )
						{

						/* Determine distance about spot location */
						if ( !blank(TagRadius.attname) )
							{
							val = CAL_get_attribute(ncal, TagRadius.attname);
							(void) convert_value(TagRadius.attunit->name, atof(val),
													DistanceUnitsM, &dval);
							xrad = (float) dval;
							}
						else
							{
							val = CAL_get_attribute(ncal, TagDiameter.attname);
							(void) convert_value(TagDiameter.attunit->name, atof(val),
													DistanceUnitsM, &dval);
							xrad = (float) dval / 2.0;
							}

						/* Loop through directions to determine boundary locations */
						for ( nn=0; nn<NumTagDirs; nn++ )
							{
							ldir = TagDirs[nn].dir;
							(void) great_circle_span(mproj, spos, ldir, xrad, epos);
							add_point_to_line(bdry, epos);
							}
						}

					/* Create boundary based on distances for set directions */
					else
						{

						/* Loop through directions to extract boundary locations */
						for ( nn=0; nn<NumTagDirs; nn++ )
							{

							/* Skip directions which do not have attribute values */
							if ( blank(TagDirs[nn].attname) ) continue;

							/* Extract the attribute value at set directions */
							ldir = TagDirs[nn].dir;
							val  = CAL_get_attribute(ncal, TagDirs[nn].attname);
							(void) convert_value(TagDirs[nn].attunit->name, atof(val),
													DistanceUnitsM, &dval);
							xrad = (float) dval;
							(void) great_circle_span(mproj, spos, ldir, xrad, epos);
							add_point_to_line(bdry, epos);
							}
						}

					/* Close boundary and skip duplicate locations */
					close_line(bdry);
					condense_line(bdry);

					/* Error if not enough points added to boundary */
					if ( bdry->numpts < 3 )
						{
						(void) fprintf(stderr,
								"%s No area created for node %d of link chain %d in field \"%s\" \"%s\" at \"%s\"\n",
								MyLabel, inode, nset,
								fdef->element->name, fdef->level->name, vtime);
						continue;
						}

					/* Ensure that area boundary does not cross over itself */
					while ( looped_line_crossing(bdry, spos, &ips, &ipe) )
						{
						if (DebugMode)
							{
							(void) fprintf(stderr,
									"%s Area crossover between segments: %d and %d (0 to %d)\n",
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
					empty_line(bdry);
					append_line(bdry, lines[0]);

					/* Add the area boundary to the AREA Object */
					define_area_boundary(area, bdry);

					/* Add the metafile attributes to the AREA Object */
					acal = CAL_create_by_name(fdesc.edef->name, fdesc.ldef->name);
					for ( natt=0; natt<attrib_meta->nattribs; natt++ )
						{
						if (DebugMode)
							{
							(void) fprintf(stderr,
									"%s Find matching attribute for metafile tag: %s\n",
									MyLabel, attrib_meta->tag[natt]);
							}

						/* Find the matching attribute tag */
						for ( nn=0; nn<NumTagAtt; nn++ )
							{
							if ( same_ic(attrib_meta->tag[natt], TagAtts[nn].tag) ) break;
							}
						for ( ntx=0; ntx<NumTagNAtt; ntx++ )
							{
							if ( same_ic(attrib_meta->tag[natt], TagNAtts[ntx].tag) ) break;
							}

						/* Extract attribute value from object attributes */
						if ( nn < NumTagAtt )
							{
							val = CAL_get_attribute(cal, TagAtts[nn].attname);
							}
						else if ( ntx < NumTagNAtt )
							{
							val = CAL_get_attribute(ncal, TagNAtts[ntx].attname);
							}
						else
							{
							continue;
							}

						/* Add attribute value to metafile attributes */
						CAL_add_attribute(acal, attrib_meta->attname[natt], val);
						if (DebugMode)
							{
							(void) fprintf(stderr,
									"%s Adding metafile attribute: %s  with value: %s\n",
									MyLabel, attrib_meta->attname[natt], val);
							}
						}

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

					/* Add the attributes to the area */
					define_area_attribs(area, acal);

					/* Add the AREA Object to the set */
					(void) add_item_to_set(aset, (ITEM) area);
					(void) fprintf(stderr,
							"%s Adding area for node: %d of object: %d  Boundary points: %d\n",
							MyLabel, inode, nset, area->bound->boundary->numpts);
					first = FALSE;
					}
				}

			/* Loop through normal/floating nodes on link chain */
			else
				{
				for ( inode=0; inode<lchain->lnum; inode++ )
					{
					lnode = lchain->nodes[inode];

					if (DebugMode)
						{
						if ( lnode->ltype == LchainUnknown )
							{
							(void) fprintf(stderr, "%s Skip unknown node\n",
									MyLabel);
							}
						else if ( lnode->ltype == LchainNode )
							{
							(void) fprintf(stderr, "%s Check normal node\n",
									MyLabel);
							}
						else if ( lnode->ltype == LchainControl )
							{
							(void) fprintf(stderr, "%s Skip control node\n",
									MyLabel);
							}
						else if ( lnode->ltype == LchainFloating )
							{
							(void) fprintf(stderr, "%s Check floating node\n",
									MyLabel);
							}
						else if ( lnode->ltype == LchainInterp )
							{
							(void) fprintf(stderr, "%s Skip interpolated node\n",
									MyLabel);
							}
						}

					/* Skip nodes that are not normal/floating */
					if ( lnode->ltype != LchainNode
							&& lnode->ltype != LchainFloating ) continue;

					if (DebugMode)
						{
						if ( lnode->mplus >= splus && lnode->mplus <= eplus )
							{
							(void) fprintf(stderr, "%s Node: %d at time: %d\n",
									MyLabel, inode, lnode->mplus);
							debug_attrib_list("LchainNodes", lnode->attrib);
							}
						else
							{
							(void) fprintf(stderr, "%s Skip node: %d at time: %d (%d to %d)\n",
									MyLabel, inode, lnode->mplus, splus, eplus);
							}
						}

					/* Skip nodes outside time range */
					if ( lnode->mplus < splus ) continue;
					if ( lnode->mplus > eplus ) continue;

					/* Set node parameters */
					copy_point(spos, lnode->node);
					ncal = CAL_duplicate((CAL) lnode->attrib);
					CAL_add_lchain_node_motion(ncal, mproj, lchain, lnode->mplus);
					CAL_add_location(ncal, mproj, lnode->node);

					/* Save the latitude/longitude with special names */
					val = CAL_get_attribute(ncal, CALlatitude);
					CAL_add_attribute(ncal, AttribReferenceLatitude, val);
					val = CAL_get_attribute(ncal, CALlongitude);
					CAL_add_attribute(ncal, AttribReferenceLongitude, val);

					/* Save the node valid time with a special name */
					nvt = calc_valid_time_minutes(lchain->xtime, 0, lnode->mplus);
					CAL_add_attribute(ncal, AttribReferenceTime, nvt);

					area = create_area("", "", "");
					bdry = create_line();

					/* Create boundary based on radius or diameter at spot location */
					if ( !blank(TagRadius.attname) || !blank(TagDiameter.attname) )
						{

						/* Determine distance about spot location */
						if ( !blank(TagRadius.attname) )
							{
							val = CAL_get_attribute(ncal, TagRadius.attname);
							(void) convert_value(TagRadius.attunit->name, atof(val),
													DistanceUnitsM, &dval);
							xrad = (float) dval;
							}
						else
							{
							val = CAL_get_attribute(ncal, TagDiameter.attname);
							(void) convert_value(TagDiameter.attunit->name, atof(val),
													DistanceUnitsM, &dval);
							xrad = (float) dval / 2.0;
							}

						/* Loop through directions to determine boundary locations */
						for ( nn=0; nn<NumTagDirs; nn++ )
							{
							ldir = TagDirs[nn].dir;
							(void) great_circle_span(mproj, spos, ldir, xrad, epos);
							add_point_to_line(bdry, epos);
							}
						}

					/* Create boundary based on distances for set directions */
					else
						{

						/* Loop through directions to extract boundary locations */
						for ( nn=0; nn<NumTagDirs; nn++ )
							{

							/* Skip directions which do not have attribute values */
							if ( blank(TagDirs[nn].attname) ) continue;

							/* Extract the attribute value at set directions */
							ldir = TagDirs[nn].dir;
							val  = CAL_get_attribute(ncal, TagDirs[nn].attname);
							(void) convert_value(TagDirs[nn].attunit->name, atof(val),
													DistanceUnitsM, &dval);
							xrad = (float) dval;
							(void) great_circle_span(mproj, spos, ldir, xrad, epos);
							add_point_to_line(bdry, epos);
							}
						}

					/* Close boundary and skip duplicate locations */
					close_line(bdry);
					condense_line(bdry);

					/* Error if not enough points added to boundary */
					if ( bdry->numpts < 3 )
						{
						(void) fprintf(stderr,
								"%s No area created for node %d of link chain %d in field \"%s\" \"%s\" at \"%s\"\n",
								MyLabel, inode, nset,
								fdef->element->name, fdef->level->name, vtime);
						continue;
						}

					/* Ensure that area boundary does not cross over itself */
					while ( looped_line_crossing(bdry, spos, &ips, &ipe) )
						{
						if (DebugMode)
							{
							(void) fprintf(stderr,
									"%s Area crossover between segments: %d and %d (0 to %d)\n",
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
					empty_line(bdry);
					append_line(bdry, lines[0]);

					/* Add the area boundary to the AREA Object */
					define_area_boundary(area, bdry);

					/* Add the metafile attributes to the AREA Object */
					acal = CAL_create_by_name(fdesc.edef->name, fdesc.ldef->name);
					for ( natt=0; natt<attrib_meta->nattribs; natt++ )
						{
						if (DebugMode)
							{
							(void) fprintf(stderr,
									"%s Find matching attribute for metafile tag: %s\n",
									MyLabel, attrib_meta->tag[natt]);
							}

						/* Find the matching attribute tag */
						for ( nn=0; nn<NumTagAtt; nn++ )
							{
							if ( same_ic(attrib_meta->tag[natt], TagAtts[nn].tag) ) break;
							}
						for ( ntx=0; ntx<NumTagNAtt; ntx++ )
							{
							if ( same_ic(attrib_meta->tag[natt], TagNAtts[ntx].tag) ) break;
							}

						/* Extract attribute value from object attributes */
						if ( nn < NumTagAtt )
							{
							val = CAL_get_attribute(cal, TagAtts[nn].attname);
							}
						else if ( ntx < NumTagNAtt )
							{
							val = CAL_get_attribute(ncal, TagNAtts[ntx].attname);
							}
						else
							{
							continue;
							}

						/* Add attribute value to metafile attributes */
						CAL_add_attribute(acal, attrib_meta->attname[natt], val);
						if (DebugMode)
							{
							(void) fprintf(stderr,
									"%s Adding metafile attribute: %s  with value: %s\n",
									MyLabel, attrib_meta->attname[natt], val);
							}
						}

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

					/* Add the attributes to the area */
					define_area_attribs(area, acal);

					/* Add the AREA Object to the set */
					(void) add_item_to_set(aset, (ITEM) area);
					(void) fprintf(stderr,
							"%s Adding area for node: %d of object: %d  Boundary points: %d\n",
							MyLabel, inode, nset, area->bound->boundary->numpts);
					first = FALSE;
					}
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
	(void) remove_file(fname, NULL);
	fname = construct_meta_filename(&fdesc);
	(void) remove_file(fname, NULL);

	/* Construct old format metafile name if new format name not found */
	if ( blank(fname) ) fname = build_meta_filename(&fdesc);

	/* Output the METAFILE Object containing the SET Object */
	(void) write_metafile(fname, meta, 4);

	/* Remove the current file lock in the base directory */
	(void) release_file_lock(LockDir, LockVtime);
	Locked = FALSE;

	/* Remove space used by METAFILE Object */
	meta = destroy_metafile(meta);

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
