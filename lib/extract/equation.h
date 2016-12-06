/*********************************************************************/
/**	@file equation.h
 * 
 * Routines to calculate fields or values for meteorological
 * equations 
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*   e q u a t i o n . h                                                *
*                                                                      *
*   Routines to calculate fields or values from meteorological         *
*   equations (include file)                                           *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
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

/* See if already included */
#ifndef EQUATION_DEFS
#define EQUATION_DEFS


/* We need definitions for low level types and macros */
#include <fpa_types.h>

/* We need definitions for other Objects and Environ parameters */
#include <objects/objects.h>
#include <environ/environ.h>

/* Temporary fix for plot/spot fields */
#ifdef EQUATION_DATA
	LOGICAL PreferPlot = FALSE;
#else
	extern LOGICAL	PreferPlot;
#endif

/***********************************************************************
*                                                                      *
*  Initialize defined constants for equation routines                  *
*                                                                      *
************************************************************************/

/* Define FpaEQTN_DATA Object                           */
/*  a union of SCALAR, GRID, SPLINE, or VLIST Objects */

/** A catch all data holder that can point to SCALAR, GRID,
 * SPLINE or VLIST objects. 
 **/
typedef struct FpaEQTN_DATA_struct
{
	short	Type;			/**< type of data */
	union	{
			SCALAR		scalr;	/**< data for SCALAR Object */
			GRID		gridd;	/**< data for GRID Object */
			SPLINE		splne;	/**< data for SPLINE Object */
			VLIST		vlist;	/**< data for VLIST Object */
			} Data;				/**< Data Object */
} FpaEQTN_DATA;

/* Set Default representations for Objects in FpaEQTN_DATA Object */
#define FpaEQT_Scalar	1
#define FpaEQT_Grid		2
#define FpaEQT_Spline	3
#define FpaEQT_Vlist	4


/* Set Default number of function argument fields */
#define FpaEQFF_MaxFlds	5
#define FpaEQFF_MinFlds	2

/** Define enumerated types for FPA functions */
typedef enum { FpaEQF_UNIX,			/**< Unix type functions */
			   FpaEQF_ORDINARY,		/**< Ordinary type functions */
			   FpaEQF_TIMESERIES	/**< Time series type functions */
			 } FpaEQFtypeOptions;

/* Typedef UNIX functions and non-UNIX functions */
typedef double			(*FpaUNAM)();
typedef FpaEQTN_DATA	*(*FpaENAM)();

/* Define FpaEQTN_FUNC Object */
/** 
 * For converting function string to function call using typedef
 * of UNIX functions and non-UNIX functions
 **/
typedef struct FpaEQTN_FUNC_struct
	{
	const	STRING		Name;	/**< string for function name */
	const	short		Fargs;	/**< number of function arguments */
	const	LOGICAL		PointEvalOK[FpaEQFF_MaxFlds];
								/**< flags for point evaluation of arguments */
	const	int			Ftype;	/**< enumerated function type */
	const	FpaUNAM		Unam;	/**< pointer to UNIX function name */
	const	FpaENAM		Enam;	/**< pointer to non-UNIX function name */
	} FpaEQTN_FUNC;


#if defined(EQUATION_MAIN) || defined(EQUATION_OPER) || defined(EQUATION_DATA)


/* Define FpaEQUATION_DEFAULTS Object - */
/** 
 * Containing Equation Defaults.
 * Note that MAX_BCHRS and MAX_NCHRS are defined in environ.h 
 **/
typedef struct FpaEQUATION_DEFAULTS_struct
{
	/* Field descriptor information */
	char		path[MAX_BCHRS];		/**< Default path name string */
	char		source[MAX_NCHRS];		/**< Default source identifier string */
	char		subsource[MAX_NCHRS];	/**< Default sub source identifier string */
	char		rtime[MAX_NCHRS];		/**< Default run time string */
	char		vtime[MAX_NCHRS];		/**< Default valid time string */
	char		lvl[MAX_NCHRS];			/**< Default level string */
	char		uprlvl[MAX_NCHRS];		/**< Default upper level string */
	char		lwrlvl[MAX_NCHRS];		/**< Default lower level string */

	/* String for fields and modifiers in generic equations */
	char		genfld[MAX_BCHRS];		/**< Default generic field string */
	char		genmod[MAX_NCHRS];		/**< Default generic modifier string */

	/* Point evaluation information */
	LOGICAL		pointeval;				/**< Default point evaluation flag  */
	LOGICAL		subgrid;				/**< Default subgrid flag      
										  (if TRUE, mprojEval is a 
										  subgrid of mprojOrig)    */
	MAP_PROJ	mprojRead;				/**< Default map projection read from 
										  metafile containing field       */
	MAP_PROJ	mprojOrig;				/**< Default equation map projection */
	MAP_PROJ	mprojEval;				/**< Default map projection for     
										  evaluating field or equation  
										  (may be subgrid of mprojOrig) */
	int			numposEval;				/**< Default number of positions */
	POINT		*posEval;				/**< Default positions for evaluation 
										  on mprojEval map projection     */
} FpaEQUATION_DEFAULTS;

#define FpaNO_EQUATION_DEFAULTS { FpaCblank, FpaCblank, FpaCblank,      \
									FpaCblank, FpaCblank,               \
									FpaCblank, FpaCblank, FpaCblank,    \
									FpaCblank, FpaCblank,               \
									FALSE, FALSE,                       \
									NO_MAPPROJ, NO_MAPPROJ, NO_MAPPROJ, \
									0, NullPointList }

/* Storage locations for global Equation Defaults */
#ifdef EQUATION_MAIN
			FpaEQUATION_DEFAULTS		FpaEqtnDefs = FpaNO_EQUATION_DEFAULTS;
#else /* EQUATION_OPER || EQUATION_DATA */
	extern	FpaEQUATION_DEFAULTS		FpaEqtnDefs;
#endif /* EQUATION_MAIN */


#endif /* EQUATION_MAIN || EQUATION_OPER || EQUATION_DATA */


#ifdef EQUATION_DATA


/* Define FpaEQTN_FLDS Object -*/
/** Containing fields in Equation Database */
typedef struct FpaEQTN_FLDS_struct
{
	FLD_DESCRIPT	descript;	/**< field descriptor for field search */
	MAP_PROJ		mproj;		/**< map projection for saved field */
	FIELD			fld;		/**< saved field of data */
} FpaEQTN_FLDS;


#endif /* EQUATION_DATA */


/***********************************************************************
*                                                                      *
*  Declare external functions in equation.c                            *
*                                                                      *
***********************************************************************/

SURFACE		retrieve_surface(FLD_DESCRIPT *fdesc);
SURFACE		retrieve_surface_by_equation(FLD_DESCRIPT *fdesc,
						STRING units, STRING equation);
SURFACE		retrieve_surface_by_attrib(FLD_DESCRIPT *fdesc,
						STRING units, STRING attrib, STRING xlookup,
						float defval, float proximity);
SET			retrieve_areaset(FLD_DESCRIPT *fdesc);
SET			retrieve_curveset(FLD_DESCRIPT *fdesc);
SET			retrieve_spotset(FLD_DESCRIPT *fdesc);
SET			retrieve_lchainset(FLD_DESCRIPT *fdesc);
PLOT		retrieve_plot(FLD_DESCRIPT *fdesc);
FIELD		retrieve_field(FLD_DESCRIPT *fdesc);
SET			retrieve_linkset(FLD_DESCRIPT *fdesc);
FIELD		calculate_equation(FLD_DESCRIPT *fdesc,
						STRING units, STRING equation);
VLIST		*retrieve_vlist(FLD_DESCRIPT *fdesc, int npos, POINT *pos);
VLIST		*retrieve_vlist_component(FLD_DESCRIPT *fdesc, int which,
						int npos, POINT *pos);
VLIST		*retrieve_vlist_by_equation(FLD_DESCRIPT *fdesc,
						int npos, POINT *pos, STRING units, STRING equation);
VLIST		*retrieve_vlist_by_attrib(FLD_DESCRIPT *fdesc, int npos, POINT *pos,
						STRING units, STRING attrib, STRING xlookup,
						float defval, float proximity);
LOGICAL		check_retrieve_metasfc(FLD_DESCRIPT *fdesc);
LOGICAL		find_retrieve_metasfc(FLD_DESCRIPT *fdesc);
LOGICAL		find_retrieve_metasfc_by_attrib(FLD_DESCRIPT *fdesc, STRING attrib);
LOGICAL		check_calculate_equation(FLD_DESCRIPT *fdesc,
						STRING units, STRING equation);
int			checked_valid_time_list(FLD_DESCRIPT *fdesc,
						int timedep, STRING **vlist);
int			checked_valid_time_list_free(STRING **, int num);


/***********************************************************************
*                                                                      *
*  Declare external functions in equationData.c                        *
*                                                                      *
***********************************************************************/

void			clear_equation_database(void);
FIELD			field_from_equation_database(FLD_DESCRIPT *fdesc);
void			replace_field_in_equation_database(FLD_DESCRIPT *fdesc,
						MAP_PROJ *mproj, FIELD fld);
void			delete_field_in_equation_database(FLD_DESCRIPT *fdesc);
FpaEQTN_DATA	*init_eqtn_data(short type);
void			free_eqtn_data(FpaEQTN_DATA *pfld);
FpaEQTN_DATA	*copy_eqtn_data(FpaEQTN_DATA *pfld);
FpaEQTN_DATA	*convert_eqtn_data(short type, FpaEQTN_DATA *pfld);
void			debug_eqtn_data(FpaEQTN_DATA *pfld);


/***********************************************************************
*                                                                      *
*  Declare external functions in equationOper.c                        *
*                                                                      *
***********************************************************************/

/* Declare interface functions (Function Identification) */
const FpaEQTN_FUNC	*identify_function(STRING buf);

/* Declare interface functions (Mathematical Symbols) in */
FpaEQTN_DATA		*oper_power(FpaEQTN_DATA *plfld, FpaEQTN_DATA *prfld);
FpaEQTN_DATA		*oper_plus(FpaEQTN_DATA *plfld, FpaEQTN_DATA *prfld);
FpaEQTN_DATA		*oper_minus(FpaEQTN_DATA *plfld, FpaEQTN_DATA *prfld);
FpaEQTN_DATA		*oper_mult(FpaEQTN_DATA *plfld, FpaEQTN_DATA *prfld);
FpaEQTN_DATA		*oper_divn(FpaEQTN_DATA *plfld, FpaEQTN_DATA *prfld);


/* Now it has been included */
#endif
