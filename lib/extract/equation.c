/*********************************************************************/
/**	@file equation.c
 *
 * Routines to calculate fields or values from metorological
 * equations.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*   e q u a t i o n . c                                                *
*                                                                      *
*   Routines to calculate fields or values  from meteorological        *
*   equations                                                          *
*                                                                      *
*    Note that the SURFACE Objects (returned by retrieve_surface       *
*    and retrieve_surface_by_equation and retrieve_surface_by_attrib), *
*    the FIELD Objects (returned by retrieve_field and                 *
*    calculate_equation), and the VLIST Objects (returned by           *
*    retrieve_vlist and retrieve_vlist_by_equation and                 *
*    retrieve_vlist_by_attrib), all return values which have been      *
*    converted to standard MKS units.  The input units for the         *
*    equation or attribute modifier determine the conversion factor    *
*    used to produce values to be returned in standard MKS units.      *
*                                                                      *
*    Note that the FpaEQUATION_DEFAULTS structure "FpaEqtnDefs"        *
*    contains global information used in equation evaluation:          *
*     - source, time, and level information used in constructing       *
*       FLD_DESCRIPT structures,                                       *
*     - strings for fields/modifiers to use in generic equations       *
*     - flags and position information for point evaluations           *
*       (including map projections for input fields and for            *
*       evaluating point locations)                                    *
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

#define EQUATION_MAIN		/* To initialize defined constants and */
							/*  functions in equation.h file       */

#include "equation.h"
#include "values.h"

#include <environ/environ.h>
#include <objects/objects.h>
#include <tools/tools.h>
#include <fpa_macros.h>
#include <fpa_types.h>
#include <fpa_getmem.h>

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#ifdef DEBUG_EQUATION		/* Turn on/off internal debug printing */
	static	LOGICAL	DebugMode = TRUE;
#else
	static	LOGICAL	DebugMode = FALSE;
#endif /* DEBUG_EQUATION */

#define dprintf (!DebugMode)? (void) 0: (void) fprintf

/* Placeholder for missing information in lookup tables */
#define LookupPlaceHolder	"-"

/* Local variables for reading lookup files */
static  const   STRING  Comment      = "!#";	/* comment line character(s) */
static  const   STRING  CommentOrEnd = "!#\n";	/* comment line character(s) */
												/*  or end of line           */

/* Structures for holding parameters from lookup tables */
typedef struct
	{
	STRING			key;
	float			value;
	} LOOKUP_LINE;

typedef struct
	{
	STRING			label;
	int				numlines;
	LOOKUP_LINE		*lines;
	LOGICAL			ismiss;
	LOOKUP_LINE		mline;
	LOGICAL			isdef;
	LOOKUP_LINE		dline;
	} LOOKUP_TABLE;

/* Storage for named lookup tables */
static	int				NumLookup = 0;
static	LOOKUP_TABLE	*Lookups  = NullPtr(LOOKUP_TABLE *);

/* Interface functions                  */
/*  ... these are defined in equation.h */

/* Internal static functions (Evaluating Equation Strings) */
static FpaEQTN_DATA	*evaluate_equation(STRING);
static FpaEQTN_DATA	*evaluate_name(STRING);
static FpaEQTN_DATA	*evaluate_operator(FpaEQTN_DATA *, char *, FpaEQTN_DATA *);
static FpaEQTN_DATA	*evaluate_bracket(FpaEQTN_DATA *, FpaEQTN_DATA *);
static FpaEQTN_DATA	*evaluate_function(STRING, STRING);
static int			evaluate_function_arguments(STRING, const LOGICAL *,
													FpaEQTN_DATA **);
static int			evaluate_function_timeseries(STRING, const LOGICAL *, int *,
													int *, FpaEQTN_DATA **);
static FpaEQTN_DATA	*evaluate_unix_function(STRING, short, FpaUNAM,
													FpaEQTN_DATA **);
static FpaEQTN_DATA	*evaluate_unary(STRING);
static FpaEQTN_DATA	*evaluate_constant(STRING);
static FpaEQTN_DATA	*evaluate_field_string(STRING);

/* Internal static functions (Checking Equation Strings) */
static LOGICAL		check_evaluate_equation(STRING);
static LOGICAL		check_evaluate_name(STRING);
static LOGICAL		check_evaluate_function(STRING, STRING);
static LOGICAL		check_evaluate_unary(STRING);
static LOGICAL		check_evaluate_constant(STRING);
static LOGICAL		check_evaluate_field_string(STRING);

/* Internal static functions (Equation String Parsing) */
static void			remove_blanks(STRING);
static LOGICAL		generic_expansion(STRING, STRING);
static char			*get_fop(STRING, STRING);
static char			*get_nop(char *, STRING, STRING);
static char			*get_nsub(char *, STRING, STRING);
static LOGICAL		get_modstrng(STRING, STRING, STRING);

/* Internal static functions (Setting Field Modifiers) */
static void			init_fieldmod(STRING, STRING, STRING, STRING, STRING,
									STRING, STRING, STRING, STRING, STRING,
									float *, float *, STRING);
static LOGICAL		reset_fieldmod(STRING, STRING, STRING, STRING, STRING,
									STRING, STRING, STRING, STRING, STRING,
									STRING, float *, float *, STRING);
static LOGICAL		reset_fieldmod_lvl(STRING, STRING);
static LOGICAL		reset_fieldmod_source(STRING, STRING, STRING);
static LOGICAL		reset_fieldmod_rtime(STRING, FLD_DESCRIPT *, STRING);
static LOGICAL		reset_fieldmod_vtime(STRING, FLD_DESCRIPT *, STRING);
static LOGICAL		reset_fieldmod_attrib(STRING, STRING);
static LOGICAL		reset_fieldmod_aunits(STRING, STRING);
static LOGICAL		reset_fieldmod_xlookup(STRING, STRING);
static LOGICAL		reset_fieldmod_defval(STRING, float *);
static LOGICAL		reset_fieldmod_proximity(STRING, float *);
static LOGICAL		reset_fieldmod_punits(STRING, STRING);

/* Internal static functions (Attribute lookup tables) */
static LOOKUP_TABLE	*get_lookup_table(STRING);
static LOGICAL		match_lookup_table(STRING, STRING, float *);

/* Internal static functions (Accessing Equation Defaults) */
static LOGICAL		valid_units_and_equation(STRING, STRING);
static void			save_equation_defaults(FpaEQUATION_DEFAULTS *);
static void			restore_equation_defaults(FpaEQUATION_DEFAULTS *);

/* Internal static functions (Endless Loop in Equations) */
static void			reset_endless_loop(void);
static void			push_endless_loop(STRING);
static void			pop_endless_loop(void);
static LOGICAL		check_endless_loop(STRING);

/* Units for vector components cross-referenced in configuration file */
static	const	STRING	DegreesBmap = "degrees_bmap";
static	const	STRING	DegreesTrue = "degrees_true";

/* Use white space as normal delimiters */
#define WHITE " \t\n\r\f"

/* Define default units for proximity to features */
#define ProximityUnits "km"

/**********************************************************************
 ***                                                                ***
 *** r e t r i e v e _ s u r f a c e                                ***
 *** r e t r i e v e _ s u r f a c e _ b y _ e q u a t i o n        ***
 *** r e t r i e v e _ s u r f a c e _ b y _ a t t r i b            ***
 ***                                                                ***
 **********************************************************************/

/*********************************************************************/
/** Get a SURFACE object from a saved metafile with given default
 * values.
 *
 *	@param[in]	*fdesc		field descriptor
 * 	@return Pointer to a SURFACE object
 *********************************************************************/
SURFACE				retrieve_surface

	(
	FLD_DESCRIPT	*fdesc
	)

	{
	FIELD		fld;
	SURFACE		sfc;

	/* Return NullSfc pointer if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->sdef) || IsNull(fdesc->subdef)
			|| IsNull(fdesc->edef) || IsNull(fdesc->ldef) ) return NullSfc;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in retrieve_surface:\n");
		dprintf(stdout,"   path: %s\n",      SafeStr(fdesc->path));
		dprintf(stdout,"   source: %s %s\n", SafeStr(fdesc->sdef->name),
												SafeStr(fdesc->subdef->name));
		dprintf(stdout,"   rtime: %s\n",     SafeStr(fdesc->rtime));
		dprintf(stdout,"   vtime: %s\n",     SafeStr(fdesc->vtime));
		dprintf(stdout,"   element: %s\n",   SafeStr(fdesc->edef->name));
		dprintf(stdout,"   level: %s\n",     SafeStr(fdesc->ldef->name));
		}

	/* Search for field using input values */
	fld = retrieve_field(fdesc);

	/* Return NullSfc pointer if no field found */
	if ( IsNull(fld) ) return NullSfc;

	/* Error message if SURFACE Object not found in field */
	if ( fld->ftype != FtypeSfc || IsNull(fld->data.sfc) )
		{
		(void) fprintf(stderr, "[retrieve_surface] No SURFACE");
		(void) fprintf(stderr, " data found in FIELD Object\n");
		(void) fprintf(stderr, "   for source: \"%s %s\"",
				SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
		(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
				SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
		(void) fprintf(stderr, "   for element: \"%s\"  level: \"%s\"\n",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		fld = destroy_field(fld);
		return NullSfc;
		}

	/* Extract the SURFACE Object               */
	/*  ... and free space used by FIELD Object */
	sfc = fld->data.sfc;
	fld->data.sfc = NullSfc;
	fld = destroy_field(fld);

	/* Return pointer to SURFACE Object */
	return sfc;
	}

/**********************************************************************/

/*********************************************************************/
/** Get a SURFACE object from an equation evaluated from saved
 * metafiles.
 *
 *	@param[in]	*fdesc		field descriptor
 *	@param[in]	inunit		input units for equation
 *	@param[in]	inebuf		input equation for field
 * 	@return Pointer to a SURFACE object.
 *********************************************************************/
SURFACE				retrieve_surface_by_equation

	(
	FLD_DESCRIPT	*fdesc,
	STRING			inunit,
	STRING			inebuf
	)

	{
	FIELD		fld;
	SURFACE		sfc;

	/* Return NullSfc pointer if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->sdef) || IsNull(fdesc->subdef)
			|| IsNull(fdesc->edef) || IsNull(fdesc->ldef) ) return NullSfc;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in retrieve_surface_by_equation:\n");
		dprintf(stdout,"   path: %s\n",      SafeStr(fdesc->path));
		dprintf(stdout,"   source: %s %s\n", SafeStr(fdesc->sdef->name),
												SafeStr(fdesc->subdef->name));
		dprintf(stdout,"   rtime: %s\n",     SafeStr(fdesc->rtime));
		dprintf(stdout,"   vtime: %s\n",     SafeStr(fdesc->vtime));
		dprintf(stdout,"   element: %s\n",   SafeStr(fdesc->edef->name));
		dprintf(stdout,"   level: %s\n",     SafeStr(fdesc->ldef->name));
		dprintf(stdout,"   units: %s\n",     SafeStr(inunit));
		dprintf(stdout,"   equation: %s\n",  SafeStr(inebuf));
		}

	/* Check the accuracy of input units and equation */
	if ( !valid_units_and_equation(inunit, inebuf) ) return NullSfc;

	/* Determine field from evaluation of input equation */
	fld = calculate_equation(fdesc, inunit, inebuf);

	/* Return NullSfc pointer if no field found */
	if ( IsNull(fld) ) return NullSfc;

	/* Error message if SURFACE Object not found in field */
	if ( fld->ftype != FtypeSfc || IsNull(fld->data.sfc) )
		{
		(void) fprintf(stderr, "[retrieve_surface_by_equation] No SURFACE");
		(void) fprintf(stderr, " data found in FIELD Object\n");
		(void) fprintf(stderr, "   from equation: \"%s\"\n", SafeStr(inebuf));
		(void) fprintf(stderr, "   for source: \"%s %s\"",
				SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
		(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
				SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
		(void) fprintf(stderr, "   for element: \"%s\"  level: \"%s\"\n",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		fld = destroy_field(fld);
		return NullSfc;
		}

	/* Extract the SURFACE Object               */
	/*  ... and free space used by FIELD Object */
	sfc = fld->data.sfc;
	fld->data.sfc = NullSfc;
	fld = destroy_field(fld);

	/* Return pointer to SURFACE Object */
	return sfc;
	}

/**********************************************************************/

/*********************************************************************/
/** Get a SURFACE object evaluated from an attribute of a saved
 * metafile.
 *
 *	@param[in]	*fdesc		field descriptor
 *	@param[in]	units		input units for attribute
 *	@param[in]	attrib		attribute to be evaluated
 *	@param[in]	xlookup		lookup table for attribute
 *	@param[in]	defval		default value for attribute
 *	@param[in]	proximity	proximity to features (in km)
 * 	@return Pointer to a SURFACE object.
 *********************************************************************/
SURFACE				retrieve_surface_by_attrib

	(
	FLD_DESCRIPT	*fdesc,
	STRING			units,
	STRING			attrib,
	STRING			xlookup,
	float			defval,
	float			proximity
	)

	{
	int						inumx, inumy, npos, iix, iiy;
	float					glen, **wlats, **wlons, **grid, *gbuf;
	POINT					**pstns, *ppos;
	VLIST					*vlist;
	SURFACE					sfc;

	/* Return NullSfc pointer if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->sdef) || IsNull(fdesc->subdef)
			|| IsNull(fdesc->edef) || IsNull(fdesc->ldef) ) return NullSfc;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in retrieve_surface_by_atrrib:\n");
		dprintf(stdout,"   path: %s\n",      SafeStr(fdesc->path));
		dprintf(stdout,"   source: %s %s\n", SafeStr(fdesc->sdef->name),
												SafeStr(fdesc->subdef->name));
		dprintf(stdout,"   rtime: %s\n",     SafeStr(fdesc->rtime));
		dprintf(stdout,"   vtime: %s\n",     SafeStr(fdesc->vtime));
		dprintf(stdout,"   element: %s\n",   SafeStr(fdesc->edef->name));
		dprintf(stdout,"   level: %s\n",     SafeStr(fdesc->ldef->name));
		dprintf(stdout,"   units: %s\n",     SafeStr(units));
		dprintf(stdout,"   attrib: %s\n",    SafeStr(attrib));
		dprintf(stdout,"   xlookup: %s\n",   SafeStr(xlookup));
		dprintf(stdout,"   defval: %.2f\n",         defval);
		dprintf(stdout,"   proximity: %.2f (km)\n", proximity);
		}

	/* Set the grid sampling locations */
	if ( !grid_positions(&(fdesc->mproj), &inumx, &inumy, &glen, &pstns,
			&wlats, &wlons) ) return NullSfc;
	npos = inumx*inumy;
	ppos = INITMEM(POINT, npos);
	for ( iiy=0; iiy<inumy; iiy++ )
		for ( iix=0; iix<inumx; iix++ )
			{
			ppos[iiy*inumx + iix][X] = pstns[iiy][iix][X];
			ppos[iiy*inumx + iix][Y] = pstns[iiy][iix][Y];
			}

	/* Extract the field values based on the attribute */
	vlist = retrieve_vlist_by_attrib(fdesc, npos, ppos, units, attrib,
										xlookup, defval, proximity);

	/* Error message if problem evaluating features */
	if ( IsNull(vlist) )
		{
		(void) fprintf(stderr,
				"[retrieve_surface_by_attrib] Problem evaluating field\n");
		(void) fprintf(stderr, "   for source: \"%s %s\"",
				SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
		(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
				SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
		(void) fprintf(stderr, "   for element: \"%s\"  level: \"%s\"",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		(void) fprintf(stderr, "  attribute: \"%s\"\n",
				SafeStr(attrib));
		return NullSfc;
		}

	/* Initialize storage for field values at all grid points */
	gbuf = INITMEM(float, npos);
	grid = INITMEM(float *, inumy);
	for ( iiy=0; iiy<inumy; iiy++ )
		{
		grid[iiy] = gbuf + iiy*inumx;
		}

	/* Set grid values from VLIST Object */
	for ( iiy=0; iiy<inumy; iiy++ )
		for ( iix=0; iix<inumx; iix++ )
			grid[iiy][iix] = vlist->val[iiy*inumx + iix];

	/* Create surface from grid values */
	sfc = create_surface();
	(void) grid_surface(sfc, glen, inumx, inumy, grid);

	/* Free space used by working objects */
	FREEMEM(gbuf);
	FREEMEM(grid);
	(void) free_vlist(vlist);
	FREEMEM(vlist);
	FREEMEM(ppos);

	/* Return pointer to SURFACE Object */
	return sfc;
	}

/**********************************************************************
 ***                                                                ***
 *** r e t r i e v e _ a r e a s e t                                ***
 ***                                                                ***
 **********************************************************************/

/*********************************************************************/
/** Get a SET of AREAs from a saved metafile with given default
 * values.
 *
 *	@param[in]	*fdesc		field descriptor
 * @return Pointer to a SET object.
 *********************************************************************/
SET					retrieve_areaset

	(
	FLD_DESCRIPT	*fdesc
	)

	{
	FIELD		fld;
	SET			set;

	/* Return NullSet pointer if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->sdef) || IsNull(fdesc->subdef)
			|| IsNull(fdesc->edef) || IsNull(fdesc->ldef) ) return NullSet;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in retrieve_areaset:\n");
		dprintf(stdout,"   path: %s\n",      SafeStr(fdesc->path));
		dprintf(stdout,"   source: %s %s\n", SafeStr(fdesc->sdef->name),
												SafeStr(fdesc->subdef->name));
		dprintf(stdout,"   rtime: %s\n",     SafeStr(fdesc->rtime));
		dprintf(stdout,"   vtime: %s\n",     SafeStr(fdesc->vtime));
		dprintf(stdout,"   element: %s\n",   SafeStr(fdesc->edef->name));
		dprintf(stdout,"   level: %s\n",     SafeStr(fdesc->ldef->name));
		}

	/* Return immediately if discrete field does not exist         */
	/*  ... to avoid error messages for no equation for field!     */
	/* (Note that equations are not available for discrete fields) */
	if ( blank(find_meta_filename(fdesc)) ) return NullSet;

	/* Search for field using input values */
	fld = retrieve_field(fdesc);

	/* Return NullSet pointer if no field found */
	if ( IsNull(fld) ) return NullSet;

	/* Error message if SET Object not found in field */
	if ( fld->ftype != FtypeSet || IsNull(fld->data.set) ||
			!same(fld->data.set->type, "area"))
		{
		(void) fprintf(stderr, "[retrieve_areaset] No AREA SET");
		(void) fprintf(stderr, " data found in FIELD Object\n");
		(void) fprintf(stderr, "   for source: \"%s %s\"",
				SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
		(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
				SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
		(void) fprintf(stderr, "   for element: \"%s\"  level: \"%s\"\n",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		fld = destroy_field(fld);
		return NullSet;
		}

	/* Extract SET of AREAs                     */
	/*  ... and free space used by FIELD Object */
	set = fld->data.set;
	fld->data.set = NullSet;
	fld = destroy_field(fld);

	/* Return pointer to SET Object */
	return set;
	}

/**********************************************************************
 ***                                                                ***
 *** r e t r i e v e _ c u r v e s e t                              ***
 ***                                                                ***
 **********************************************************************/

/*********************************************************************/
/** Get a SET of CURVEs from a saved metafile with given default
 * values.
 *
 *	@param[in]	*fdesc		field descriptor
 * 	@return Pointer to a SET object.
 *********************************************************************/
SET					retrieve_curveset

	(
	FLD_DESCRIPT	*fdesc
	)

	{
	FIELD		fld;
	SET			set;

	/* Return NullSet pointer if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->sdef) || IsNull(fdesc->subdef)
			|| IsNull(fdesc->edef) || IsNull(fdesc->ldef) ) return NullSet;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in retrieve_curveset:\n");
		dprintf(stdout,"   path: %s\n",      SafeStr(fdesc->path));
		dprintf(stdout,"   source: %s %s\n", SafeStr(fdesc->sdef->name),
												SafeStr(fdesc->subdef->name));
		dprintf(stdout,"   rtime: %s\n",     SafeStr(fdesc->rtime));
		dprintf(stdout,"   vtime: %s\n",     SafeStr(fdesc->vtime));
		dprintf(stdout,"   element: %s\n",   SafeStr(fdesc->edef->name));
		dprintf(stdout,"   level: %s\n",     SafeStr(fdesc->ldef->name));
		}

	/* Return immediately if discrete field does not exist     */
	/*  ... to avoid error messages for no equation for field! */
	/* (Note that equations are not available for line fields) */
	if ( blank(find_meta_filename(fdesc)) ) return NullSet;

	/* Search for field using input values */
	fld = retrieve_field(fdesc);

	/* Return NullSet pointer if no field found */
	if ( IsNull(fld) ) return NullSet;

	/* Error message if SET Object not found in field */
	if ( fld->ftype != FtypeSet || IsNull(fld->data.set) ||
			!same(fld->data.set->type, "curve"))
		{
		(void) fprintf(stderr, "[retrieve_curveset] No CURVE SET");
		(void) fprintf(stderr, " data found in FIELD Object\n");
		(void) fprintf(stderr, "   for source: \"%s %s\"",
				SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
		(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
				SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
		(void) fprintf(stderr, "   for element: \"%s\"  level: \"%s\"\n",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		fld = destroy_field(fld);
		return NullSet;
		}

	/* Extract SET of CURVEs                    */
	/*  ... and free space used by FIELD Object */
	set = fld->data.set;
	fld->data.set = NullSet;
	fld = destroy_field(fld);

	/* Return pointer to SET Object */
	return set;
	}

/**********************************************************************
 ***                                                                ***
 *** r e t r i e v e _ s p o t s e t                                ***
 ***                                                                ***
 **********************************************************************/

/*********************************************************************/
/** Get a SET of SPOTs from a saved metafile with given default
 * values.
 *
 *	@param[in]	*fdesc		field descriptor
 * 	@return Pointer to a Set.
 *********************************************************************/
SET					retrieve_spotset

	(
	FLD_DESCRIPT	*fdesc
	)

	{
	FIELD		fld;
	SET			set;

	/* Return NullSet pointer if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->sdef) || IsNull(fdesc->subdef)
			|| IsNull(fdesc->edef) || IsNull(fdesc->ldef) ) return NullSet;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in retrieve_spotset:\n");
		dprintf(stdout,"   path: %s\n",      SafeStr(fdesc->path));
		dprintf(stdout,"   source: %s %s\n", SafeStr(fdesc->sdef->name),
												SafeStr(fdesc->subdef->name));
		dprintf(stdout,"   rtime: %s\n",     SafeStr(fdesc->rtime));
		dprintf(stdout,"   vtime: %s\n",     SafeStr(fdesc->vtime));
		dprintf(stdout,"   element: %s\n",   SafeStr(fdesc->edef->name));
		dprintf(stdout,"   level: %s\n",     SafeStr(fdesc->ldef->name));
		}

	/* Return immediately if scattered point field does not exist         */
	/*  ... to avoid error messages for no equation for field!            */
	/* (Note that equations are not available for scattered point fields) */
	if ( blank(find_meta_filename(fdesc)) ) return NullSet;

	/* Search for field using input values */
	fld = retrieve_field(fdesc);

	/* Return NullSet pointer if no field found */
	if ( IsNull(fld) ) return NullSet;

	/* Error message if SET Object not found in field */
	if ( fld->ftype != FtypeSet || IsNull(fld->data.set) ||
			!same(fld->data.set->type, "spot"))
		{
		(void) fprintf(stderr, "[retrieve_spotset] No SPOT SET");
		(void) fprintf(stderr, " data found in FIELD Object\n");
		(void) fprintf(stderr, "   for source: \"%s %s\"",
				SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
		(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
				SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
		(void) fprintf(stderr, "   for element: \"%s\"  level: \"%s\"\n",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		fld = destroy_field(fld);
		return NullSet;
		}

	/* Extract SET of SPOTs                     */
	/*  ... and free space used by FIELD Object */
	set = fld->data.set;
	fld->data.set = NullSet;
	fld = destroy_field(fld);

	/* Return pointer to SET Object */
	return set;
	}

/**********************************************************************
 ***                                                                ***
 *** r e t r i e v e _ l c h a i n s e t                            ***
 ***                                                                ***
 **********************************************************************/

/*********************************************************************/
/** Get a SET of LCHAINs from a saved metafile with given default
 * values.
 *
 *	@param[in]	*fdesc		field descriptor
 * 	@return Pointer to a SET object.
 *********************************************************************/
SET					retrieve_lchainset

	(
	FLD_DESCRIPT	*fdesc
	)

	{
	FIELD		fld;
	SET			set;

	/* Return NullSet pointer if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->sdef) || IsNull(fdesc->subdef)
			|| IsNull(fdesc->edef) || IsNull(fdesc->ldef) ) return NullSet;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in retrieve_lchainset:\n");
		dprintf(stdout,"   path: %s\n",      SafeStr(fdesc->path));
		dprintf(stdout,"   source: %s %s\n", SafeStr(fdesc->sdef->name),
												SafeStr(fdesc->subdef->name));
		dprintf(stdout,"   rtime: %s\n",     SafeStr(fdesc->rtime));
		dprintf(stdout,"   vtime: %s\n",     SafeStr(fdesc->vtime));
		dprintf(stdout,"   element: %s\n",   SafeStr(fdesc->edef->name));
		dprintf(stdout,"   level: %s\n",     SafeStr(fdesc->ldef->name));
		}

	/* Return immediately if link chain field does not exist         */
	/*  ... to avoid error messages for no equation for field!       */
	/* (Note that equations are not available for link chain fields) */
	if ( blank(find_meta_filename(fdesc)) ) return NullSet;

	/* Search for field using input values */
	fld = retrieve_field(fdesc);

	/* Return NullSet pointer if no field found */
	if ( IsNull(fld) ) return NullSet;

	/* Error message if SET Object not found in field */
	if ( fld->ftype != FtypeSet || IsNull(fld->data.set) ||
			!same(fld->data.set->type, "lchain"))
		{
		(void) fprintf(stderr, "[retrieve_lchainset] No LCHAIN SET");
		(void) fprintf(stderr, " data found in FIELD Object\n");
		(void) fprintf(stderr, "   for source: \"%s %s\"",
				SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
		(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
				SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
		(void) fprintf(stderr, "   for element: \"%s\"  level: \"%s\"\n",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		fld = destroy_field(fld);
		return NullSet;
		}

	/* Extract SET of LCHAINs                   */
	/*  ... and free space used by FIELD Object */
	set = fld->data.set;
	fld->data.set = NullSet;
	fld = destroy_field(fld);

	/* Return pointer to SET Object */
	return set;
	}

/**********************************************************************
 ***                                                                ***
 *** r e t r i e v e _ p l o t                                      ***
 ***                                                                ***
 **********************************************************************/

/*********************************************************************/
/** Get a PLOT object from a saved metafile with given default values.
 *
 *	@param[in]	*fdesc		field descriptor
 * 	@return Pointer to a PLOT object.
 *********************************************************************/
PLOT				retrieve_plot

	(
	FLD_DESCRIPT	*fdesc
	)

	{
	FIELD		fld;
	PLOT		plot;

	/* Return NullPlot pointer if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->sdef) || IsNull(fdesc->subdef)
			|| IsNull(fdesc->edef) || IsNull(fdesc->ldef) ) return NullPlot;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in retrieve_plot:\n");
		dprintf(stdout,"   path: %s\n",      SafeStr(fdesc->path));
		dprintf(stdout,"   source: %s %s\n", SafeStr(fdesc->sdef->name),
												SafeStr(fdesc->subdef->name));
		dprintf(stdout,"   rtime: %s\n",     SafeStr(fdesc->rtime));
		dprintf(stdout,"   vtime: %s\n",     SafeStr(fdesc->vtime));
		dprintf(stdout,"   element: %s\n",   SafeStr(fdesc->edef->name));
		dprintf(stdout,"   level: %s\n",     SafeStr(fdesc->ldef->name));
		}

	/* Return immediately if plot field does not exist         */
	/*  ... to avoid error messages for no equation for field! */
	/* (Note that equations are not available for plot fields) */
	if ( blank(find_meta_filename(fdesc)) ) return NullPlot;

	/* Search for field using input values */
	PreferPlot = TRUE;
	fld = retrieve_field(fdesc);

	/* Return NullPlot pointer if no field found */
	if ( IsNull(fld) ) return NullPlot;

	/* Error message if PLOT Object not found in field */
	if ( fld->ftype != FtypePlot || IsNull(fld->data.plot) )
		{
		(void) fprintf(stderr, "[retrieve_plot] No PLOT");
		(void) fprintf(stderr, " data found in FIELD Object\n");
		(void) fprintf(stderr, "   for source: \"%s %s\"",
				SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
		(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
				SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
		(void) fprintf(stderr, "   for element: \"%s\"  level: \"%s\"\n",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		fld = destroy_field(fld);
		return NullPlot;
		}

	/* Extract the PLOT Object                  */
	/*  ... and free space used by FIELD Object */
	plot = fld->data.plot;
	fld->data.plot = NullPlot;
	fld = destroy_field(fld);

	/* Return pointer to PLOT Object */
	return plot;
	}

/**********************************************************************
 ***                                                                ***
 *** r e t r i e v e _ f i e l d                                    ***
 ***                                                                ***
 **********************************************************************/

/*********************************************************************/
/** Get the FIELD object determined by searching for and reading a
 * saved metafile with given default values, or from an equation
 * evaluated from saved metafiles.
 *
 *	@param[in]	*fdesc		field descriptor
 * 	@return Pointer to a FIELD object.
 *********************************************************************/
FIELD				retrieve_field

	(
	FLD_DESCRIPT	*fdesc
	)

	{
	int								icmp, isrc;
	COMPONENT						tcomp;
	LOGICAL							UVcomp, MDcomp, valid, dirtrue;
	int								inumx, inumy, npos, iix, iiy, nn;
	float							glen, **wlats, **wlons, *vals, clon;
	POINT							**pstns, *ppos;
	float							**gridu, **gridv, *gbufu, *gbufv;
	double							dval, mval, dvalx;
	STRING							dunits = NullString;
	STRING							dirunits, direlem, dirlevel, vunits;
	FLD_DESCRIPT					descript;
	FpaConfigFieldStruct			*fdef, *fdefx;
	FpaConfigElementComponentStruct	*components;
	FpaConfigElementEquationStruct	*equation;
	FpaConfigElementValCalcStruct	*valcalc;
	FIELD							fld, fldx;
	VLIST							*vlist;
	SURFACE							uvsfc;

	FIELD							ufld   = NullFld;
	FIELD							vfld   = NullFld;
	VLIST							*dlist = NullPtr(VLIST *);
	VLIST							*mlist = NullPtr(VLIST *);

	/* Return NullFld pointer if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->sdef) || IsNull(fdesc->subdef)
			|| IsNull(fdesc->edef) || IsNull(fdesc->ldef) ) return NullFld;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in retrieve_field:\n");
		dprintf(stdout,"   path: %s\n",      SafeStr(fdesc->path));
		dprintf(stdout,"   source: %s %s\n", SafeStr(fdesc->sdef->name),
												SafeStr(fdesc->subdef->name));
		dprintf(stdout,"   rtime: %s\n",     SafeStr(fdesc->rtime));
		dprintf(stdout,"   vtime: %s\n",     SafeStr(fdesc->vtime));
		dprintf(stdout,"   element: %s\n",   SafeStr(fdesc->edef->name));
		dprintf(stdout,"   level: %s\n",     SafeStr(fdesc->ldef->name));
		}

	/* Return copy of field found in Equation Database */
	fld = field_from_equation_database(fdesc);

	/* Field was found in database */
	if ( NotNull(fld) )
		{

		/* Return pointer to field if no reprojection required */
		if ( same_map_projection(&FpaEqtnDefs.mprojRead, &fdesc->mproj) )
				return fld;

		/* Return remapped surface fields if only scales differ */
		if ( fld->ftype==FtypeSfc
				&& equivalent_map_projection(&FpaEqtnDefs.mprojRead,
						&fdesc->mproj) )
			{
			(void) remap_surface(fld->data.sfc, &FpaEqtnDefs.mprojRead,
					&fdesc->mproj);
			return fld;
			}

		/* Reproject surface fields to the evaluation projection */
		/*  using fast reprojection if evaluation is a subgrid   */
		else if ( fld->ftype==FtypeSfc && FpaEqtnDefs.subgrid
				&& equivalent_map_projection(&FpaEqtnDefs.mprojRead,
						&FpaEqtnDefs.mprojOrig) )
			{
			if ( evaluation_surface(fld->data.sfc, &FpaEqtnDefs.mprojRead,
					&fdesc->mproj) ) return fld;

			/* Error message if problem reprojecting surface field */
			/*  from subgrid of original projection                */
			else
				{
				(void) fprintf(stderr, "[retrieve_field] Error reprojecting");
				(void) fprintf(stderr, " surface field\n");
				(void) fprintf(stderr, "   for source: \"%s %s\"",
						SafeStr(fdesc->sdef->name),
						SafeStr(fdesc->subdef->name));
				(void) fprintf(stderr,
						"  at runtime: \"%s\"  validtime: \"%s\"\n",
						SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
				(void) fprintf(stderr,
						"   for element: \"%s\"  level: \"%s\"\n",
						SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
				fld = destroy_field(fld);
				return NullFld;
				}
			}

		/* Reproject surface fields to the evaluation projection */
		else if ( fld->ftype == FtypeSfc )
			{
			if ( reproject_surface(fld->data.sfc, &FpaEqtnDefs.mprojRead,
					&fdesc->mproj, &fdesc->mproj.grid) ) return fld;

			/* Error message if problem reprojecting surface field */
			else
				{
				(void) fprintf(stderr, "[retrieve_field] Error reprojecting");
				(void) fprintf(stderr, " surface field\n");
				(void) fprintf(stderr, "   for source: \"%s %s\"",
						SafeStr(fdesc->sdef->name),
						SafeStr(fdesc->subdef->name));
				(void) fprintf(stderr,
						"  at runtime: \"%s\"  validtime: \"%s\"\n",
						SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
				(void) fprintf(stderr,
						"   for element: \"%s\"  level: \"%s\"\n",
						SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
				fld = destroy_field(fld);
				return NullFld;
				}
			}

		/* Reproject set fields to the evaluation projection */
		else if ( fld->ftype == FtypeSet )
			{
			if ( reproject_set(fld->data.set, &FpaEqtnDefs.mprojRead,
					&fdesc->mproj) ) return fld;

			/* Error message if problem reprojecting set field */
			else
				{
				(void) fprintf(stderr, "[retrieve_field] Error reprojecting");
				(void) fprintf(stderr, " set field\n");
				(void) fprintf(stderr, "   for source: \"%s %s\"",
						SafeStr(fdesc->sdef->name),
								SafeStr(fdesc->subdef->name));
				(void) fprintf(stderr,
						"  at runtime: \"%s\"  validtime: \"%s\"\n",
						SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
				(void) fprintf(stderr,
						"   for element: \"%s\"  level: \"%s\"\n",
						SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
				fld = destroy_field(fld);
				return NullFld;
				}
			}

		/* Reproject plot fields to the evaluation projection */
		else if ( fld->ftype == FtypePlot )
			{
			if ( reproject_plot(fld->data.plot, &FpaEqtnDefs.mprojRead,
					&fdesc->mproj) ) return fld;

			/* Error message if problem reprojecting plot field */
			else
				{
				(void) fprintf(stderr, "[retrieve_field] Error reprojecting");
				(void) fprintf(stderr, " plot field\n");
				(void) fprintf(stderr, "   for source: \"%s %s\"",
						SafeStr(fdesc->sdef->name),
						SafeStr(fdesc->subdef->name));
				(void) fprintf(stderr,
						"  at runtime: \"%s\"  validtime: \"%s\"\n",
						SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
				(void) fprintf(stderr,
						"   for element: \"%s\"  level: \"%s\"\n",
						SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
				fld = destroy_field(fld);
				return NullFld;
				}
			}
		}

	/* Get detailed element information to search for components */
	/*  or equation or value calculation                         */
	fdef = get_field_info(fdesc->edef->name, fdesc->ldef->name);

	/* Check for components for vector fields */
	if ( NotNull(fdef) && fdef->element->fld_type == FpaC_VECTOR )
		{
		if ( DebugMode )
			{
			dprintf(stdout, "  Searching for vector components for: \"%s %s\"\n",
					SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
			}

		/* First set the component type */
		UVcomp = MDcomp = FALSE;
		components = fdef->element->elem_detail->components;
		if ( NotNull(components) && components->ncomp > 0 )
			{

			/* Check type of each component */
			for ( icmp=0; icmp<components->ncomp; icmp++ )
				{

				/* Reset field descriptor for this component */
				(void) copy_fld_descript(&descript, fdesc);
				(void) set_fld_descript(&descript,
									FpaF_ELEMENT, components->comp_edefs[icmp],
									FpaF_END_OF_LIST);

				/* Check for acceptable components */
				tcomp = components->comp_types[icmp];
				switch (tcomp)
					{
					case X_Comp:
					case Y_Comp:	UVcomp = TRUE;
									break;

					case D_Comp:
					case M_Comp:	MDcomp = TRUE;
									break;

					default:	(void) fprintf(stderr,
										"[retrieve_field] Incorrect field component type\n");
								(void) fprintf(stderr,
										"   for component: \"%s\"  level: \"%s\"\n",
										SafeStr(descript.edef->name),
										SafeStr(descript.ldef->name));
								(void) fprintf(stderr,
										"   of field: \"%s %s\"\n",
										SafeStr(fdesc->edef->name),
										SafeStr(fdesc->ldef->name));
								continue;
					}
				}
			}

		/* Extract or build field for UV components */
		if ( UVcomp )
			{

			/* Check for metafile for each component */
			for ( icmp=0; icmp<components->ncomp; icmp++ )
				{

				/* Reset field descriptor for this component */
				(void) copy_fld_descript(&descript, fdesc);
				(void) set_fld_descript(&descript,
									FpaF_ELEMENT, components->comp_edefs[icmp],
									FpaF_END_OF_LIST);

				/* Check for acceptable components */
				tcomp = components->comp_types[icmp];
				switch (tcomp)
					{
					case X_Comp:
					case Y_Comp:	break;

					default:		continue;
					}

				/* Retrieve field for this component */
				fldx = retrieve_field(&descript);
				if ( NotNull(fldx) )
					{

					/* Set the component field extracted */
					if      ( tcomp == X_Comp ) ufld = fldx;
					else if ( tcomp == Y_Comp ) vfld = fldx;
					else						(void) destroy_field(fldx);

					/* Go on to the next component */
					continue;
					}
				}

			/* Return if either component not found */
			if ( IsNull(ufld) || IsNull(vfld) )
				{
				(void) fprintf(stderr,
						"[retrieve_field] Missing field component\n");
				(void) fprintf(stderr, "   for source: \"%s %s\"",
						SafeStr(fdesc->sdef->name),
						SafeStr(fdesc->subdef->name));
				(void) fprintf(stderr,
						"  at runtime: \"%s\"  validtime: \"%s\"\n",
						SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
				(void) fprintf(stderr,
						"   for element: \"%s\"  level: \"%s\"\n",
						SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
				(void) destroy_field(ufld);
				(void) destroy_field(vfld);
				return NullFld;
				}

			/* Build a field from the two components */
			fld = build_field_2D(entity_from_field_type(descript.fmacro),
									fdesc->edef->name, fdesc->ldef->name,
									ufld, vfld, &fdesc->mproj, &fdesc->mproj);

			/* Return pointer to field from vector field components */
			(void) destroy_field(ufld);
			(void) destroy_field(vfld);
			return fld;
			}

		/* Create field for MD components */
		else if ( MDcomp )
			{

			/* Set the grid sampling locations */
			if ( !grid_positions(&(fdesc->mproj), &inumx, &inumy, &glen, &pstns,
					&wlats, &wlons) ) return NullFld;
			npos = inumx*inumy;
			ppos = INITMEM(POINT, npos);
			for ( iiy=0; iiy<inumy; iiy++ )
				for ( iix=0; iix<inumx; iix++ )
					{
					ppos[iiy*inumx + iix][X] = pstns[iiy][iix][X];
					ppos[iiy*inumx + iix][Y] = pstns[iiy][iix][Y];
					}

			/* Check for equation or value calculation for each component */
			for ( icmp=0; icmp<components->ncomp; icmp++ )
				{

				/* Reset field descriptor for this component */
				(void) copy_fld_descript(&descript, fdesc);
				(void) set_fld_descript(&descript,
									FpaF_ELEMENT, components->comp_edefs[icmp],
									FpaF_END_OF_LIST);

				/* Check for acceptable components */
				tcomp = components->comp_types[icmp];
				switch (tcomp)
					{
					case D_Comp:
					case M_Comp:	break;

					default:		continue;
					}

				/* Get detailed field information for this component */
				fdefx = get_field_info(descript.edef->name,
										descript.ldef->name);
				if ( IsNull(fdefx) )
					{
					(void) fprintf(stderr,
							"[retrieve_field] Missing detailed field information\n");
					(void) fprintf(stderr,
							"   for element: \"%s\"  level: \"%s\"\n",
							SafeStr(descript.edef->name),
							SafeStr(descript.ldef->name));
					FREEMEM(ppos);
					if ( NotNull(dlist) )
						{
						(void) free_vlist(dlist);
						FREEMEM(dlist);
						}
					if ( NotNull(mlist) )
						{
						(void) free_vlist(mlist);
						FREEMEM(mlist);
						}
					return NullFld;
					}

				/* Check for equation for this component */
				if ( DebugMode )
					{
					dprintf(stdout, "  Searching for equation for: \"%s %s\"\n",
							SafeStr(descript.edef->name),
							SafeStr(descript.ldef->name));
					}
				if ( NotNull(fdefx->element->elem_detail->equation) )
					{

					/* Extract the component values based on the equation */
					equation = fdefx->element->elem_detail->equation;
					vlist    = retrieve_vlist_by_equation(&descript, npos, ppos,
										equation->units->name, equation->eqtn);
					if ( NotNull(vlist) )
						{

						/* Set the component values extracted */
						if ( tcomp == D_Comp )
							{
							dlist    = vlist;
							dirunits = equation->units->name;
							direlem  = descript.edef->name;
							dirlevel = descript.ldef->name;
							}
						else
							{
							mlist    = vlist;
							}

						/* Go on to the next component */
						continue;
						}
					}

				/* Check for value calculation for this component */
				if ( DebugMode )
					{
					dprintf(stdout,
							"  Searching for value calculation for: \"%s %s\"\n",
							SafeStr(descript.edef->name),
							SafeStr(descript.ldef->name));
					}
				if ( NotNull(fdefx->element->elem_detail->valcalc) )
					{

					/* Check for matching value calculation source type */
					valcalc = fdefx->element->elem_detail->valcalc;
					for ( isrc=0; isrc<valcalc->nsrc_type; isrc++ )
						{
						if ( descript.sdef->src_type
								== valcalc->src_types[isrc] ) break;
						}

					/* Did not find a matching value calculation source type */
					if ( isrc >= valcalc->nsrc_type )
						{
						(void) fprintf(stderr, "[retrieve_field] No matching");
						(void) fprintf(stderr,
								" value calculation source type\n");
						(void) fprintf(stderr, "   for source: \"%s %s\"",
								SafeStr(descript.sdef->name),
								SafeStr(descript.subdef->name));
						(void) fprintf(stderr,
								"  at runtime: \"%s\"  validtime: \"%s\"\n",
								SafeStr(descript.rtime),
								SafeStr(descript.vtime));
						(void) fprintf(stderr, "   for crossref: \"%s\"",
								SafeStr(valcalc->vcalc->name));
						(void) fprintf(stderr,
								"  of element: \"%s\"  level: \"%s\"\n",
								SafeStr(descript.edef->name),
								SafeStr(descript.ldef->name));

						/* Go on to the next component */
						continue;
						}

					/* Allocate space for the values */
					vals = INITMEM(float, npos);

					/* Extract the values by value cross reference */
					(void) grid_center(&descript.mproj, NullPointPtr,
							NullFloat, &clon);
					valid = extract_surface_value_by_crossref(valcalc->vcalc->name,
							&descript, FALSE, npos, ppos, clon, vals, &vunits);

					/* Error if values cannot be extracted */
					if ( !valid )
						{
						(void) fprintf(stderr, "[retrieve_field] Error in");
						(void) fprintf(stderr,
								" extract_surface_value_by_crossref()\n");
						(void) fprintf(stderr, "   for source: \"%s %s\"",
								SafeStr(descript.sdef->name),
								SafeStr(descript.subdef->name));
						(void) fprintf(stderr,
								"  at runtime: \"%s\"  validtime: \"%s\"\n",
								SafeStr(descript.rtime),
								SafeStr(descript.vtime));
						(void) fprintf(stderr, "   for crossref: \"%s\"",
								SafeStr(valcalc->vcalc->name));
						(void) fprintf(stderr,
								"  of element: \"%s\"  level: \"%s\"\n",
								SafeStr(descript.edef->name),
								SafeStr(descript.ldef->name));
						FREEMEM(vals);

						/* Go on to the next component */
						continue;
						}

					/* Create VLIST Object to hold the values */
					vlist = INITMEM(VLIST, 1);
					(void) init_vlist(vlist);

					/* Add values to VLIST Object */
					for ( nn=0; nn<npos; nn++ )
						(void) add_point_to_vlist(vlist, ppos[nn], vals[nn]);
					FREEMEM(vals);

					/* Set the component values extracted */
					if ( tcomp == D_Comp )
						{
						FREEMEM(dunits);
						dunits   = strdup(SafeStr(vunits));
						dlist    = vlist;
						dirunits = dunits;
						direlem  = descript.edef->name;
						dirlevel = descript.ldef->name;
						}
					else
						{
						mlist    = vlist;
						}

					/* Go on to the next component */
					continue;
					}

				/* Return if no equation or value calculation found */
				/*  for this component                              */
				(void) fprintf(stderr, "[retrieve_field] No equation or");
				(void) fprintf(stderr, " value calculation found\n");
				(void) fprintf(stderr, "   for source: \"%s %s\"",
						SafeStr(descript.sdef->name),
						SafeStr(descript.subdef->name));
				(void) fprintf(stderr,
						"  at runtime: \"%s\"  validtime: \"%s\"\n",
						SafeStr(descript.rtime),
						SafeStr(descript.vtime));
				(void) fprintf(stderr,
						"   for element: \"%s\"  level: \"%s\"\n",
						SafeStr(descript.edef->name),
						SafeStr(descript.ldef->name));
				FREEMEM(dunits);
				FREEMEM(ppos);
				if ( NotNull(dlist) )
					{
					(void) free_vlist(dlist);
					FREEMEM(dlist);
					}
				if ( NotNull(mlist) )
					{
					(void) free_vlist(mlist);
					FREEMEM(mlist);
					}
				return NullFld;
				}

			/* Return if either component not found */
			if ( IsNull(dlist) || IsNull(mlist) )
				{
				(void) fprintf(stderr,
						"[retrieve_field] Missing field component\n");
				(void) fprintf(stderr, "   for source: \"%s %s\"",
						SafeStr(fdesc->sdef->name),
						SafeStr(fdesc->subdef->name));
				(void) fprintf(stderr,
						"  at runtime: \"%s\"  validtime: \"%s\"\n",
						SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
				(void) fprintf(stderr,
						"   for element: \"%s\"  level: \"%s\"\n",
						SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
				FREEMEM(dunits);
				FREEMEM(ppos);
				if ( NotNull(dlist) )
					{
					(void) free_vlist(dlist);
					FREEMEM(dlist);
					}
				if ( NotNull(mlist) )
					{
					(void) free_vlist(mlist);
					FREEMEM(mlist);
					}
				return NullFld;
				}

			/* Initialize space for UV grid point values */
			gbufu = INITMEM(float, npos);
			gbufv = INITMEM(float, npos);
			gridu = INITMEM(float *, inumy);
			gridv = INITMEM(float *, inumy);
			for ( iiy=0; iiy<inumy; iiy++ )
				{
				gridu[iiy] = gbufu + iiy*inumx;
				gridv[iiy] = gbufv + iiy*inumx;
				}

			/* Determine units for direction */
			if ( convert_value(dirunits, 0.0, DegreesTrue, NullDouble) )
				dirtrue = TRUE;
			else if ( convert_value(dirunits, 0.0, DegreesBmap, NullDouble) )
				dirtrue = FALSE;
			else
				{
				(void) fprintf(stderr,
						"[retrieve_field] Unacceptable wind units: \"%s\"\n",
						SafeStr(dirunits));
				(void) fprintf(stderr,
						"   for element: \"%s\"  level: \"%s\"\n",
						SafeStr(direlem), SafeStr(dirlevel));
				FREEMEM(dunits);
				FREEMEM(ppos);
				if ( NotNull(dlist) )
					{
					(void) free_vlist(dlist);
					FREEMEM(dlist);
					}
				if ( NotNull(mlist) )
					{
					(void) free_vlist(mlist);
					FREEMEM(mlist);
					}
				return NullFld;
				}

			/* Calculate UV grid point values from MD component values */
			for ( iiy=0; iiy<inumy; iiy++ )
				for ( iix=0; iix<inumx; iix++ )
					{

					/* Set direction and magnitude component values         */
					/* Note that values have already been converted to MKS! */
					dval = (double) dlist->val[iiy*inumx + iix];
					mval = (double) mlist->val[iiy*inumx + iix];

					/* Convert true direction to direction on the basemap */
					if ( dirtrue )
						{
						dvalx = wind_dir_xy(&(fdesc->mproj), wlats[iiy][iix],
											wlons[iiy][iix], (float) dval);
						dval  = dvalx;
						}

					/* Determine UV components from direction and magnitude */
					gridu[iiy][iix] = mval * fpa_cosdeg(dval);
					gridv[iiy][iix] = mval * fpa_sindeg(dval);
					}
			FREEMEM(dunits);
			FREEMEM(ppos);
			if ( NotNull(dlist) )
				{
				(void) free_vlist(dlist);
				FREEMEM(dlist);
				}
			if ( NotNull(mlist) )
				{
				(void) free_vlist(mlist);
				FREEMEM(mlist);
				}

			/* Create UV surface from component grid values */
			uvsfc = create_surface();
			(void) grid_surface_2D(uvsfc, glen, inumx, inumy, gridu, gridv);
			FREEMEM(gbufu);
			FREEMEM(gbufv);
			FREEMEM(gridu);
			FREEMEM(gridv);

			/* Create UV field from surface */
			fld = create_field(entity_from_field_type(descript.fmacro),
									fdesc->edef->name, fdesc->ldef->name);
			define_fld_data(fld, "surface", (POINTER) uvsfc);
			return fld;
			}
		}

	/* Check for equation to evaluate if field was not found in Database */
	if ( DebugMode )
		{
		dprintf(stdout, "  Searching for equation for: \"%s %s\"\n",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		}
	if ( NotNull(fdef) && NotNull(fdef->element->elem_detail->equation) )
		{

		/* Evaluate the equation */
		equation = fdef->element->elem_detail->equation;
		fld      = calculate_equation(fdesc,
										equation->units->name, equation->eqtn);

		/* Return pointer to field from equation evaluation */
		return fld;
		}

	/* Check for value calculation if no field or equation found */
	if ( DebugMode )
		{
		dprintf(stdout, "  Searching for value calculation for: \"%s %s\"\n",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		}
	if ( NotNull(fdef) && NotNull(fdef->element->elem_detail->valcalc) )
		{

		/* Check for matching value calculation source type */
		valcalc = fdef->element->elem_detail->valcalc;
		for ( isrc=0; isrc<valcalc->nsrc_type; isrc++ )
			{
			if ( fdesc->sdef->src_type == valcalc->src_types[isrc] )
				{

				/* Extract the field by value cross reference */
				fld = extract_field_by_value_crossref(fdesc,
						valcalc->vcalc->name, fdesc, FALSE);

				/* Return pointer to field from value cross reference */
				return fld;
				}
			}

		/* Did not find a matching value calculation source type */
		if ( isrc >= valcalc->nsrc_type )
			{
			(void) fprintf(stderr,
					"[retrieve_field] No matching value calculation source type\n");
			(void) fprintf(stderr, "   for source: \"%s %s\"",
					SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
			(void) fprintf(stderr,
					"  at runtime: \"%s\"  validtime: \"%s\"\n",
					SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
			(void) fprintf(stderr, "   for crossref: \"%s\"",
					SafeStr(valcalc->vcalc->name));
			(void) fprintf(stderr, "  of element: \"%s\"  level: \"%s\"\n",
					SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
			}
		}

	/* Error message if no field, equation, or value calculation found */
	(void) fprintf(stderr, "[retrieve_field] No field or equation found\n");
	(void) fprintf(stderr, "   for source: \"%s %s\"",
			SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
	(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
			SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
	(void) fprintf(stderr, "   for element: \"%s\"  level: \"%s\"\n",
			SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
	return NullFld;
	}

/**********************************************************************
 ***                                                                ***
 *** r e t r i e v e _ l i n k s e t                                ***
 ***                                                                ***
 **********************************************************************/

/*********************************************************************/
/** Get a SET of LCHAINs from a link file.
 *
 *	@param[in]	*fdesc		field descriptor
 * 	@return Pointer to a SET object.
 *********************************************************************/
SET					retrieve_linkset

	(
	FLD_DESCRIPT	*fdesc
	)

	{
	STRING		linkpath;
	METAFILE	meta;
	FIELD		fld;
	SET			set;

	/* Return NullSet pointer if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->sdef) || IsNull(fdesc->subdef)
			|| IsNull(fdesc->edef) || IsNull(fdesc->ldef) ) return NullSet;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in retrieve_linkset:\n");
		dprintf(stdout,"   path: %s\n",      SafeStr(fdesc->path));
		dprintf(stdout,"   source: %s %s\n", SafeStr(fdesc->sdef->name),
												SafeStr(fdesc->subdef->name));
		dprintf(stdout,"   element: %s\n",   SafeStr(fdesc->edef->name));
		dprintf(stdout,"   level: %s\n",     SafeStr(fdesc->ldef->name));
		}

	/* Return immediately if link file does not exist */
	linkpath = check_link_filename(fdesc);
	if ( blank(linkpath) ) return NullSet;

	/* Read the named link file and extract the link chain field */
	if ( DebugMode )
		{
		dprintf(stdout, "  Reading link file: \"%s\"\n", SafeStr(linkpath));
		}
	meta = read_metafile(linkpath, NullMapProj);

	/* Error message if problem reading named link file */
	if ( IsNull(meta) )
		{
		(void) fprintf(stderr, "[retrieve_linkset]");
		(void) fprintf(stderr, " Problem reading metafile: \"%s\"\n",
				linkpath);
		(void) fprintf(stderr, "   for source: \"%s %s\"",
				SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
		(void) fprintf(stderr, "  element: \"%s\"  level: \"%s\"\n",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		return NullSet;
		}

	/* Extract the link chain field */
	fld  = take_mf_field(meta, "set", "lchain", NullString,
			fdesc->edef->name, fdesc->ldef->name);
	meta = destroy_metafile(meta);

	/* Return NullSet pointer if no field found */
	if ( IsNull(fld) ) return NullSet;

	/* Error message if SET Object not found in field */
	if ( fld->ftype != FtypeSet || IsNull(fld->data.set) ||
			!same(fld->data.set->type, "lchain"))
		{
		(void) fprintf(stderr, "[retrieve_linkset] No LCHAIN SET");
		(void) fprintf(stderr, " data found in FIELD Object\n");
		(void) fprintf(stderr, "   for source: \"%s %s\"",
				SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
		(void) fprintf(stderr, "  element: \"%s\"  level: \"%s\"\n",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		fld = destroy_field(fld);
		return NullSet;
		}

	/* Extract SET of LCHAINs                   */
	/*  ... and free space used by FIELD Object */
	set = fld->data.set;
	fld->data.set = NullSet;
	fld = destroy_field(fld);

	/* Return pointer to SET Object */
	return set;
	}

/**********************************************************************
 ***                                                                ***
 *** c a l c u l a t e _ e q u a t i o n                            ***
 ***                                                                ***
 **********************************************************************/

/*********************************************************************/
/** Get a FIELD object determined by evaluationg an equation string
 * with given default values.
 *
 *	@param[in]	*fdesc		field descriptor
 *	@param[in]	inunit		input units for equation
 *	@param[in]	inebuf		input equation for field
 * 	@return Pointer to a FIELD object.
 *********************************************************************/
FIELD				calculate_equation

	(
	FLD_DESCRIPT	*fdesc,
	STRING			inunit,
	STRING			inebuf
	)

	{
	FpaEQUATION_DEFAULTS	OldEqtnDefs;
	FpaEQTN_DATA			*pfield, *pspline;
	FpaConfigUnitStruct		*udef;
	SURFACE					sfc;
	FIELD					fld;

	static USPEC			uspec = {NullString, 1.0, 0.0};

	/* Return NullFld pointer if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->sdef) || IsNull(fdesc->subdef)
			|| IsNull(fdesc->edef) || IsNull(fdesc->ldef) ) return NullFld;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in calculate_equation:\n");
		dprintf(stdout,"   path: %s\n",      SafeStr(fdesc->path));
		dprintf(stdout,"   source: %s %s\n", SafeStr(fdesc->sdef->name),
												SafeStr(fdesc->subdef->name));
		dprintf(stdout,"   rtime: %s\n",     SafeStr(fdesc->rtime));
		dprintf(stdout,"   vtime: %s\n",     SafeStr(fdesc->vtime));
		dprintf(stdout,"   element: %s\n",   SafeStr(fdesc->edef->name));
		dprintf(stdout,"   level: %s\n",     SafeStr(fdesc->ldef->name));
		dprintf(stdout,"   units: %s\n",     SafeStr(inunit));
		dprintf(stdout,"   equation: %s\n",  SafeStr(inebuf));
		}

	/* Check the accuracy of input units and equation */
	if ( !valid_units_and_equation(inunit, inebuf) ) return NullFld;

	/* Ensure that equation handler is not trapped in an endless loop */
	if ( check_endless_loop(inebuf) )
		{
		(void) reset_endless_loop();
		return NullFld;
		}

	/* Store old Equation Defaults */
	(void) save_equation_defaults(&OldEqtnDefs);

	/* Reset default path name and source */
	(void) safe_strcpy(FpaEqtnDefs.path,      fdesc->path);
	(void) safe_strcpy(FpaEqtnDefs.source,    fdesc->sdef->name);
	(void) safe_strcpy(FpaEqtnDefs.subsource, fdesc->subdef->name);

	/* Reset default run and valid timestamps */
	(void) safe_strcpy(FpaEqtnDefs.rtime, fdesc->rtime);
	(void) safe_strcpy(FpaEqtnDefs.vtime, fdesc->vtime);

	/* Reset default levels based on enumerated level type */
	switch ( fdesc->ldef->lvl_type )
		{

		/* Reset all default levels from single pressure level */
		case FpaC_MSL:
		case FpaC_SURFACE:
		case FpaC_LEVEL:
		case FpaC_GEOGRAPHY:
		case FpaC_ANNOTATION:
			(void) safe_strcpy(FpaEqtnDefs.lvl,    fdesc->ldef->lev_lvls->lvl);
			(void) safe_strcpy(FpaEqtnDefs.uprlvl, fdesc->ldef->lev_lvls->lvl);
			(void) safe_strcpy(FpaEqtnDefs.lwrlvl, fdesc->ldef->lev_lvls->lvl);
			break;

		/* Reset default levels from upper and lower pressure levels */
		case FpaC_LAYER:
			(void) safe_strcpy(FpaEqtnDefs.lvl,    fdesc->ldef->lev_lvls->uprlvl);
			(void) safe_strcpy(FpaEqtnDefs.uprlvl, fdesc->ldef->lev_lvls->uprlvl);
			(void) safe_strcpy(FpaEqtnDefs.lwrlvl, fdesc->ldef->lev_lvls->lwrlvl);
			break;

		/* Do not reset levels for other cases */
		default:
			break;
		}

	/* Reset default map projection for evaluation                     */
	/* (Note that FpaEqtnDefs.mprojOrig, if it exists, is not changed) */
	(void) copy_map_projection(&FpaEqtnDefs.mprojEval, &fdesc->mproj);

	/* Evaluate the equation string                       */
	/*  ... and convert evaluated Object to SPLINE Object */
	pfield  = evaluate_equation(inebuf);
	pspline = convert_eqtn_data(FpaEQT_Spline, pfield);

	/* Error message if problem evaluating equation string */
	if ( IsNull(pfield) || IsNull(pspline) )
		{
		(void) fprintf(stderr, "[calculate_equation] Problem evaluating");
		(void) fprintf(stderr, " equation: \"%s\"\n", SafeStr(inebuf));
		(void) fprintf(stderr, "   for source: \"%s %s\"",
				SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
		(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
				SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
		(void) fprintf(stderr, "   for element: \"%s\"  level: \"%s\"\n",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		/* Free space used by work structures */
		(void) free_eqtn_data(pfield);
		(void) free_eqtn_data(pspline);
		(void) restore_equation_defaults(&OldEqtnDefs);
		(void) pop_endless_loop();
		return NullFld;
		}

	/* Build FIELD Object to hold evaluation */
	sfc = create_surface();
	fld = create_field("a", fdesc->edef->name, fdesc->ldef->name);
	define_fld_data(fld, "surface", (POINTER) sfc);

	/* Transfer data to SURFACE Object in FIELD Object */
	(void) define_surface_spline(sfc, pspline->Data.splne.m,
			pspline->Data.splne.n, &pspline->Data.splne.mp,
			pspline->Data.splne.origin, pspline->Data.splne.orient,
			pspline->Data.splne.gridlen, *pspline->Data.splne.cvs,
			pspline->Data.splne.n);

	/* Set units specs from input units for equation */
	udef = identify_unit(inunit);
	if ( NotNull(udef) )
		{
		(void) define_uspec(&uspec, udef->name, udef->factor, udef->offset);
		(void) define_surface_units(sfc, &uspec);
		(void) change_surface_units(sfc, &MKS_UNITS);
		}

	/* Free space used by work structures */
	(void) free_eqtn_data(pfield);
	(void) free_eqtn_data(pspline);

	/* Finally, restore old Equation Defaults and */
	/*  return pointer to FIELD Object            */
	(void) restore_equation_defaults(&OldEqtnDefs);
	(void) pop_endless_loop();
	return fld;
	}

/**********************************************************************
 ***                                                                ***
 *** r e t r i e v e _ v l i s t                                    ***
 *** r e t r i e v e _ v l i s t _ c o m p o n e n t                ***
 *** r e t r i e v e _ v l i s t _ b y _ e q u a t i o n            ***
 *** r e t r i e v e _ v l i s t _ b y _ a t t r i b                ***
 ***                                                                ***
 **********************************************************************/

/*********************************************************************/
/** Get a VLIST from a saved metafile with given default values.
 *
 *	@param[in]	*fdesc		field descriptor
 *	@param[in]	npos		number of positions on field
 *	@param[in]	*pos		positions on field for evaluation
 * 	@return Pointer to a VLIST object.
 *********************************************************************/
VLIST				*retrieve_vlist

	(
	FLD_DESCRIPT	*fdesc,
	int				npos,
	POINT			*pos
	)

	{
	int								nn, isrc;
	LOGICAL							valid, evalid, force;
	POINT							poseval;
	double							value;
	FpaConfigFieldStruct			*fdef;
	FpaConfigElementEquationStruct	*equation;
	FpaConfigElementValCalcStruct	*valcalc;
	float							*vals, clon;
	FIELD							fld;
	VLIST							*vlist;

	/* Return Null pointer if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->sdef) || IsNull(fdesc->subdef)
			|| IsNull(fdesc->edef) || IsNull(fdesc->ldef) )
		return NullPtr(VLIST *);
	if ( IsNull(pos) ) return NullPtr(VLIST *);

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in retrieve_vlist:\n");
		dprintf(stdout,"   path: %s\n",      SafeStr(fdesc->path));
		dprintf(stdout,"   source: %s %s\n", SafeStr(fdesc->sdef->name),
												SafeStr(fdesc->subdef->name));
		dprintf(stdout,"   rtime: %s\n",     SafeStr(fdesc->rtime));
		dprintf(stdout,"   vtime: %s\n",     SafeStr(fdesc->vtime));
		dprintf(stdout,"   element: %s\n",   SafeStr(fdesc->edef->name));
		dprintf(stdout,"   level: %s\n",     SafeStr(fdesc->ldef->name));
		}

	/* Get detailed element information to search for equation */
	/*  or value calculation                                   */
	fdef = get_field_info(fdesc->edef->name, fdesc->ldef->name);

	/* Re-evaluate if "force_calculation" is set */
	force = FALSE;
	if ( NotNull(fdef) && NotNull(fdef->element->elem_detail->equation) )
		{
		force = fdef->element->elem_detail->equation->force;
		}

	/* Re-evaluate value calculation (if "force_calculation" is set) */
	else if ( NotNull(fdef) && NotNull(fdef->element->elem_detail->valcalc) )
		{
		force = fdef->element->elem_detail->valcalc->force;
		}

	/* Return copy of field found in Equation Database                          */
	/* Note that VECTOR fields stored in Equation Database are not remapped     */
	/*  from their original projections, and no remapping is required by this   */
	/*  routine since only the magnitude of the VECTOR field is returned        */
	/* Note that CONTINUOUS fields stored in Equation Database are not remapped */
	/*  from their original projections ... except for components of VECTOR     */
	/*  fields, which are remapped before being stored in Equation Database     */
	if ( !force ) fld = field_from_equation_database(fdesc);
	else          fld = NullFld;

	/* Extract required values if field found in Equation Database */
	if ( NotNull(fld) )
		{

		/* Error if field is not a SURFACE! */
		if ( fld->ftype != FtypeSfc || IsNull(fld->data.sfc) )
			{
			(void) fprintf(stderr, "[retrieve_vlist] No SURFACE");
			(void) fprintf(stderr, " data found in FIELD Object\n");
			(void) fprintf(stderr, "   for source: \"%s %s\"",
					SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
			(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
					SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
			(void) fprintf(stderr, "   for element: \"%s\"  level: \"%s\"\n",
					SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
			fld = destroy_field(fld);
			return NullPtr(VLIST *);
			}

		/* Compare map projections in debug mode */
		if ( DebugMode &&
				!same_map_projection(&FpaEqtnDefs.mprojRead, &fdesc->mproj) )
			{
			dprintf(stderr, "[retrieve_vlist] Surface map: olat/olon/lref: %.2f %.2f %.2f\n",
					FpaEqtnDefs.mprojRead.definition.olat, FpaEqtnDefs.mprojRead.definition.olon,
					FpaEqtnDefs.mprojRead.definition.lref);
			dprintf(stderr, "[retrieve_vlist] Surface map: xorg/yorg/xlen/ylen: %.2f %.2f %.2f %2f\n",
					FpaEqtnDefs.mprojRead.definition.xorg, FpaEqtnDefs.mprojRead.definition.yorg,
					FpaEqtnDefs.mprojRead.definition.xlen, FpaEqtnDefs.mprojRead.definition.ylen);
			dprintf(stderr, "[retrieve_vlist] Projection map: olat/olon/lref: %.2f %.2f %.2f\n",
					fdesc->mproj.definition.olat, fdesc->mproj.definition.olon,
					fdesc->mproj.definition.lref);
			dprintf(stderr, "[retrieve_vlist] Projection map: xorg/yorg/xlen/ylen: %.2f %.2f %.2f %2f\n",
					fdesc->mproj.definition.xorg, fdesc->mproj.definition.yorg,
					fdesc->mproj.definition.xlen, fdesc->mproj.definition.ylen);
			}
		else if ( DebugMode )
			{
			dprintf(stderr, "[retrieve_vlist] Same map projections!\n");
			dprintf(stderr, "[retrieve_vlist] Projection map: olat/olon/lref: %.2f %.2f %.2f\n",
					fdesc->mproj.definition.olat, fdesc->mproj.definition.olon,
					fdesc->mproj.definition.lref);
			dprintf(stderr, "[retrieve_vlist] Projection map: xorg/yorg/xlen/ylen: %.2f %.2f %.2f %2f\n",
					fdesc->mproj.definition.xorg, fdesc->mproj.definition.yorg,
					fdesc->mproj.definition.xlen, fdesc->mproj.definition.ylen);
			}

		/* Create VLIST Object to hold the extracted values */
		vlist = INITMEM(VLIST, 1);
		(void) init_vlist(vlist);

		/* Extract values at each position */
		for ( nn=0; nn<npos; nn++ )
			{

			/* Set position on the input grid for extraction */
			valid = pos_to_pos(&fdesc->mproj, pos[nn],
					&FpaEqtnDefs.mprojRead, poseval);

			/* Extract the value at the requested position */
			evalid = eval_sfc(fld->data.sfc, poseval, &value);

			/* Error if value cannot be extracted */
			if ( !valid || !evalid )
				{
				(void) fprintf(stderr, "[retrieve_vlist] Error in eval_sfc()");
				(void) fprintf(stderr, " at position: %f  %f\n",
						pos[nn][X], pos[nn][Y]);
				(void) fprintf(stderr, "   for source: \"%s %s\"",
						SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
				(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
						SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
				(void) fprintf(stderr, "   for element: \"%s\"  level: \"%s\"\n",
						SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
				fld = destroy_field(fld);
				return NullPtr(VLIST *);
				}

			/* Add value to VLIST Object */
			(void) add_point_to_vlist(vlist, pos[nn], (float)value);
			}

		/* Free space used by FIELD Object     */
		/*  and return pointer to VLIST Object */
		fld = destroy_field(fld);
		return vlist;
		}

	/* Check for equation to evaluate if field was not found in Database */
	if ( DebugMode )
		{
		dprintf(stdout, "  Searching for equation for: \"%s %s\"\n",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		}
	if ( NotNull(fdef) && NotNull(fdef->element->elem_detail->equation) )
		{

		/* Evaluate the equation */
		equation = fdef->element->elem_detail->equation;
		vlist    = retrieve_vlist_by_equation(fdesc, npos, pos,
										equation->units->name, equation->eqtn);

		/* Return pointer to VLIST Object from equation evaluation */
		return vlist;
		}

	/* Check for value calculation if no field or equation found */
	if ( DebugMode )
		{
		dprintf(stdout, "  Searching for value calculation for: \"%s %s\"\n",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		}
	if ( NotNull(fdef) && NotNull(fdef->element->elem_detail->valcalc) )
		{

		/* Check for matching value calculation source type */
		valcalc = fdef->element->elem_detail->valcalc;
		for ( isrc=0; isrc<valcalc->nsrc_type; isrc++ )
			{
			if ( fdesc->sdef->src_type == valcalc->src_types[isrc] )
				{

				/* Allocate space for the values */
				vals = INITMEM(float, npos);

				/* Extract the values by value cross reference */
				(void) grid_center(&fdesc->mproj, NullPointPtr, NullFloat,
						&clon);
				valid = extract_surface_value_by_crossref(valcalc->vcalc->name,
						fdesc, FALSE, npos, pos, clon, vals, NullStringPtr);

				/* Error if values cannot be extracted */
				if ( !valid )
					{
					(void) fprintf(stderr, "[retrieve_vlist] Error in");
					(void) fprintf(stderr, " extract_surface_value_by_crossref()\n");
					(void) fprintf(stderr, "   for source: \"%s %s\"",
							SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
					(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
							SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
					(void) fprintf(stderr, "   for crossref: \"%s\"",
							SafeStr(valcalc->vcalc->name));
					(void) fprintf(stderr, "  of element: \"%s\"",
							SafeStr(fdesc->edef->name));
					(void) fprintf(stderr, "  level: \"%s\"\n",
							SafeStr(fdesc->ldef->name));
					(void) fprintf(stderr, "   for element: \"%s\"  level: \"%s\"\n",
							SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
					FREEMEM(vals);
					return NullPtr(VLIST *);
					}

				/* Create VLIST Object to hold the values */
				vlist = INITMEM(VLIST, 1);
				(void) init_vlist(vlist);

				/* Add values to VLIST Object */
				for ( nn=0; nn<npos; nn++ )
					(void) add_point_to_vlist(vlist, pos[nn], vals[nn]);

				/* Free space used by values and return pointer to            */
				/*  VLIST Object containing values from value cross reference */
				FREEMEM(vals);
				return vlist;
				}
			}

		/* Did not find a matching value calculation source type */
		if ( isrc >= valcalc->nsrc_type )
			{
			(void) fprintf(stderr, "[retrieve_vlist] No matching");
			(void) fprintf(stderr, " value calculation source type\n");
			(void) fprintf(stderr, "   for source: \"%s %s\"",
					SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
			(void) fprintf(stderr,
					"  at runtime: \"%s\"  validtime: \"%s\"\n",
					SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
			(void) fprintf(stderr, "   for crossref: \"%s\"",
					SafeStr(valcalc->vcalc->name));
			(void) fprintf(stderr, "  of element: \"%s\"  level: \"%s\"\n",
					SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
			}
		}

	/* Error message if no field, equation, or value calculation found */
	(void) fprintf(stderr, "[retrieve_vlist] No field or equation found\n");
	(void) fprintf(stderr, "   for source: \"%s %s\"",
			SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
	(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
			SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
	(void) fprintf(stderr, "   for element: \"%s\"  level: \"%s\"\n",
			SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
	return NullPtr(VLIST *);
	}

/**********************************************************************/

/*********************************************************************/
/** Get a VLIST from a saved metafile with given default values. Only
 * get the requested component.
 *
 *	@param[in]	*fdesc		field descriptor
 *	@param[in]	which		which component of vector field
 *	@param[in]	npos		number of positions on field
 *	@param[in]	*pos		positions on field for evaluation
 * 	@return Pointer to a VLIST object.
 *********************************************************************/
VLIST				*retrieve_vlist_component

	(
	FLD_DESCRIPT	*fdesc,
	int				which,
	int				npos,
	POINT			*pos
	)

	{
	int								nn, icmp;
	LOGICAL							valid, force, UVcomp, MDcomp;
	float							wlat, wlon, rval;
	double							value, dang;
	FLD_DESCRIPT					descript;
	COMPONENT						tcomp;
	FpaConfigFieldStruct			*fdef, *cfdef;
	FpaConfigElementComponentStruct	*components;
	FIELD							fld;
	VLIST							*vlist;
	SURFACE							xsfc   = NullSfc;
	SURFACE							ysfc   = NullSfc;
	SURFACE							xysfc  = NullSfc;
	VLIST							*mlist = NullPtr(VLIST *);
	VLIST							*dlist = NullPtr(VLIST *);

	/* Return Null pointer if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->sdef) || IsNull(fdesc->subdef)
			|| IsNull(fdesc->edef) || IsNull(fdesc->ldef) )
		return NullPtr(VLIST *);
	if ( IsNull(pos) ) return NullPtr(VLIST *);

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in retrieve_vlist_component:\n");
		dprintf(stdout,"   path: %s\n",      SafeStr(fdesc->path));
		dprintf(stdout,"   source: %s %s\n", SafeStr(fdesc->sdef->name),
												SafeStr(fdesc->subdef->name));
		dprintf(stdout,"   rtime: %s\n",     SafeStr(fdesc->rtime));
		dprintf(stdout,"   vtime: %s\n",     SafeStr(fdesc->vtime));
		dprintf(stdout,"   element: %s\n",   SafeStr(fdesc->edef->name));
		dprintf(stdout,"   level: %s\n",     SafeStr(fdesc->ldef->name));
		}

	/* Check for acceptable components */
	switch (which)
		{
		case X_Comp:
		case Y_Comp:
		case M_Comp:
		case D_Comp:
			break;

		default:
			(void) fprintf(stderr, "[retrieve_vlist_component] Cannot sample");
			(void) fprintf(stderr, " component: \"%d\"\n", which);
			(void) fprintf(stderr, "   for element: \"%s\"  level: \"%s\"\n",
					SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
			return NullPtr(VLIST *);
		}

	/* Get detailed element information for field or component */
	fdef = get_field_info(fdesc->edef->name, fdesc->ldef->name);

	/* Force re-evaluation of both components if one component passed */
	force = TRUE;

	/* Check components of vector fields to see if re-evaluation required */
	if ( NotNull(fdef) && fdef->element->fld_type == FpaC_VECTOR )
		{
		force      = FALSE;
		components = fdef->element->elem_detail->components;
		if ( NotNull(components) && components->ncomp > 0 )
			{

			/* Check for requested component */
			for ( icmp=0; icmp<components->ncomp; icmp++ )
				{

				/* Reset field descriptor for this component */
				(void) copy_fld_descript(&descript, fdesc);
				(void) set_fld_descript(&descript,
									FpaF_ELEMENT, components->comp_edefs[icmp],
									FpaF_END_OF_LIST);

				/* Get detailed element information for this component */
				cfdef = get_field_info(descript.edef->name, descript.ldef->name);

				/* Re-evaluate equation (if "force_calculation" is set) */
				if ( NotNull(cfdef)
						&& NotNull(cfdef->element->elem_detail->equation) )
					{
					if ( !force )
						force = cfdef->element->elem_detail->equation->force;
					}

				/* Re-evaluate value calculation (if "force_calculation" is set) */
				else if ( NotNull(cfdef)
						&& NotNull(cfdef->element->elem_detail->valcalc) )
					{
					if ( !force )
						force = cfdef->element->elem_detail->valcalc->force;
					}
				}
			}
		}

	/* Return copy of field found in Equation Database                        */
	/* Note that VECTOR fields stored in Equation Database are not remapped   */
	/*  from their original projections, and must be remapped in this routine */
	/* Note that CONTINUOUS fields that are components of VECTOR fields are   */
	/*  automatically remapped before being stored in Equation Database       */
	if ( !force ) fld = field_from_equation_database(fdesc);
	else          fld = NullFld;

	/* Extract required values if field found in Equation Database */
	if ( NotNull(fld) )
		{
		if ( DebugMode )
			{
			dprintf(stdout,
					"  Extracting vector component from field: \"%s %s\"\n",
					SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
			}

		/* Error if field is not a SURFACE! */
		if ( fld->ftype != FtypeSfc || IsNull(fld->data.sfc) )
			{
			(void) fprintf(stderr, "[retrieve_vlist_component] No SURFACE");
			(void) fprintf(stderr, " data found in FIELD Object\n");
			(void) fprintf(stderr, "   for source: \"%s %s\"",
					SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
			(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
					SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
			(void) fprintf(stderr, "   for element: \"%s\"  level: \"%s\"\n",
					SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
			fld = destroy_field(fld);
			return NullPtr(VLIST *);
			}

		/* Compare map projections in debug mode */
		if ( DebugMode &&
				!same_map_projection(&FpaEqtnDefs.mprojRead, &fdesc->mproj) )
			{
			dprintf(stderr, "[retrieve_vlist_component] Surface map: olat/olon/lref: %.2f %.2f %.2f\n",
					FpaEqtnDefs.mprojRead.definition.olat, FpaEqtnDefs.mprojRead.definition.olon,
					FpaEqtnDefs.mprojRead.definition.lref);
			dprintf(stderr, "[retrieve_vlist_component] Surface map: xorg/yorg/xlen/ylen: %.2f %.2f %.2f %2f\n",
					FpaEqtnDefs.mprojRead.definition.xorg, FpaEqtnDefs.mprojRead.definition.yorg,
					FpaEqtnDefs.mprojRead.definition.xlen, FpaEqtnDefs.mprojRead.definition.ylen);
			dprintf(stderr, "[retrieve_vlist_component] Projection map: olat/olon/lref: %.2f %.2f %.2f\n",
					fdesc->mproj.definition.olat, fdesc->mproj.definition.olon,
					fdesc->mproj.definition.lref);
			dprintf(stderr, "[retrieve_vlist_component] Projection map: xorg/yorg/xlen/ylen: %.2f %.2f %.2f %2f\n",
					fdesc->mproj.definition.xorg, fdesc->mproj.definition.yorg,
					fdesc->mproj.definition.xlen, fdesc->mproj.definition.ylen);
			}
		else if ( DebugMode )
			{
			dprintf(stderr, "[retrieve_vlist_component] Same map projections!\n");
			dprintf(stderr, "[retrieve_vlist_component] Projection map: olat/olon/lref: %.2f %.2f %.2f\n",
					fdesc->mproj.definition.olat, fdesc->mproj.definition.olon,
					fdesc->mproj.definition.lref);
			dprintf(stderr, "[retrieve_vlist_component] Projection map: xorg/yorg/xlen/ylen: %.2f %.2f %.2f %2f\n",
					fdesc->mproj.definition.xorg, fdesc->mproj.definition.yorg,
					fdesc->mproj.definition.xlen, fdesc->mproj.definition.ylen);
			}

		/* Create VLIST Object to hold the extracted values */
		vlist = INITMEM(VLIST, 1);
		(void) init_vlist(vlist);

		/* Remap surface (if required) */
		if ( !same_map_projection(&FpaEqtnDefs.mprojRead, &fdesc->mproj) )
			remap_surface(fld->data.sfc, &FpaEqtnDefs.mprojRead, &fdesc->mproj);

		/* Extract values at each position */
		for ( nn=0; nn<npos; nn++ )
			{

			/* Extract the requested value at the requested position */
			switch (which)
				{
				case X_Comp:
					valid = eval_sfc_UV(fld->data.sfc, pos[nn],
								&value, NullDouble);
					break;

				case Y_Comp:
					valid = eval_sfc_UV(fld->data.sfc, pos[nn],
								NullDouble, &value);
					break;

				case M_Comp:
					valid = eval_sfc_MD(fld->data.sfc, pos[nn],
								&value, NullDouble);
					break;

				case D_Comp:
					valid = eval_sfc_MD(fld->data.sfc, pos[nn],
								NullDouble, &value);
					break;
				}

			/* Error if value cannot be extracted */
			if ( !valid )
				{
				(void) fprintf(stderr,
						"[retrieve_vlist_component] Error in eval_sfc_..()");
				(void) fprintf(stderr, " at position: %f  %f\n",
						pos[nn][X], pos[nn][Y]);
				(void) fprintf(stderr, "   for source: \"%s %s\"",
						SafeStr(fdesc->sdef->name),
						SafeStr(fdesc->subdef->name));
				(void) fprintf(stderr,
						"  at runtime: \"%s\"  validtime: \"%s\"\n",
						SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
				(void) fprintf(stderr,
						"   for element: \"%s\"  level: \"%s\"\n",
						SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
				fld = destroy_field(fld);
				return NullPtr(VLIST *);
				}

			/* Add value to VLIST Object */
			(void) add_point_to_vlist(vlist, pos[nn], (float) value);
			}

		/* Free space used by FIELD Object     */
		/*  and return pointer to VLIST Object */
		fld = destroy_field(fld);
		return vlist;
		}

	/* Evaluate component from vector components contained in field */
	if ( NotNull(fdef)
			&& (fdef->element->fld_type == FpaC_VECTOR
					|| fdef->element->fld_type == FpaC_CONTINUOUS) )
		{
		if ( DebugMode )
			{
			dprintf(stdout,
					"  Evaluating vector component for field: \"%s %s\"\n",
					SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
			}

		/* Create VLIST object to hold the evaluated values */
		vlist = INITMEM(VLIST, 1);
		(void) init_vlist(vlist);

		/* Extract values for each component in field */
		UVcomp = MDcomp = FALSE;
		components = fdef->element->elem_detail->components;
		if ( NotNull(components) && components->ncomp > 0 )
			{
			for ( icmp=0; icmp<components->ncomp; icmp++ )
				{

				/* Reset field descriptor for this component */
				(void) copy_fld_descript(&descript, fdesc);
				(void) set_fld_descript(&descript,
									FpaF_ELEMENT, components->comp_edefs[icmp],
									FpaF_END_OF_LIST);

				/* Extract values based on component type */
				tcomp = components->comp_types[icmp];
				switch ( tcomp )
					{
					case X_Comp:
						if ( NotNull(xsfc) ) break;
						UVcomp = TRUE;
						xsfc   = retrieve_surface(&descript);
						break;

					case Y_Comp:
						if ( NotNull(ysfc) ) break;
						UVcomp = TRUE;
						ysfc  = retrieve_surface(&descript);
						break;

					case M_Comp:
						if ( NotNull(mlist) ) break;
						MDcomp = TRUE;
						mlist  = retrieve_vlist(&descript, npos, pos);
						break;

					case D_Comp:
						if ( NotNull(dlist) ) break;
						MDcomp = TRUE;
						dlist  = retrieve_vlist(&descript, npos, pos);
						break;
					}
				}
			}

		/* Evaluate values based on x/y component surfaces */
		if ( UVcomp && NotNull(xsfc) && NotNull(ysfc) )
			{

			/* Compare surface projections */
			if ( !same_map_projection(&xsfc->sp.mp, &ysfc->sp.mp) )
				{
				(void) fprintf(stderr,
						"[retrieve_vlist_component] Unmatched map projections!\n");
				(void) fprintf(stderr,
						"   for source: \"%s %s\"",
						SafeStr(fdesc->sdef->name),
						SafeStr(fdesc->subdef->name));
				(void) fprintf(stderr,
						"  at runtime: \"%s\"  validtime: \"%s\"\n",
						SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
				(void) fprintf(stderr,
						"   for element: \"%s\"  level: \"%s\"\n",
						SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
				xsfc = destroy_surface(xsfc);
				ysfc = destroy_surface(ysfc);
				return NullPtr(VLIST *);
				}

			/* Compare map projections in debug mode */
			if ( DebugMode && !same_map_projection(&xsfc->sp.mp, &fdesc->mproj) )
				{
				dprintf(stderr, "[retrieve_vlist_component] Surface map: olat/olon/lref: %.2f %.2f %.2f\n",
						xsfc->sp.mp.definition.olat, xsfc->sp.mp.definition.olon,
						xsfc->sp.mp.definition.lref);
				dprintf(stderr, "[retrieve_vlist_component] Surface map: xorg/yorg/xlen/ylen: %.2f %.2f %.2f %2f\n",
						xsfc->sp.mp.definition.xorg, xsfc->sp.mp.definition.yorg,
						xsfc->sp.mp.definition.xlen, xsfc->sp.mp.definition.ylen);
				dprintf(stderr, "[retrieve_vlist_component] Projection map: olat/olon/lref: %.2f %.2f %.2f\n",
						fdesc->mproj.definition.olat, fdesc->mproj.definition.olon,
						fdesc->mproj.definition.lref);
				dprintf(stderr, "[retrieve_vlist_component] Projection map: xorg/yorg/xlen/ylen: %.2f %.2f %.2f %2f\n",
						fdesc->mproj.definition.xorg, fdesc->mproj.definition.yorg,
						fdesc->mproj.definition.xlen, fdesc->mproj.definition.ylen);
				}
			else if ( DebugMode )
				{
				dprintf(stderr, "[retrieve_vlist_component] Same map projections!\n");
				dprintf(stderr, "[retrieve_vlist_component] Projection map: olat/olon/lref: %.2f %.2f %.2f\n",
						fdesc->mproj.definition.olat, fdesc->mproj.definition.olon,
						fdesc->mproj.definition.lref);
				dprintf(stderr, "[retrieve_vlist_component] Projection map: xorg/yorg/xlen/ylen: %.2f %.2f %.2f %2f\n",
						fdesc->mproj.definition.xorg, fdesc->mproj.definition.yorg,
						fdesc->mproj.definition.xlen, fdesc->mproj.definition.ylen);
				}

			/* Build a 2D surface from the x/y component surfaces */
			xysfc = build_surface_2D(xsfc, ysfc, &xsfc->sp.mp, &fdesc->mproj);

			/* Evaluate values at each position */
			for ( nn=0; nn<npos; nn++ )
				{

				/* Extract the requested value at the requested position */
				switch (which)
					{
					case X_Comp:
						valid = eval_sfc_UV(xysfc, pos[nn],
									&value, NullDouble);
						break;

					case Y_Comp:
						valid = eval_sfc_UV(xysfc, pos[nn],
									NullDouble, &value);
						break;

					case M_Comp:
						valid = eval_sfc_MD(xysfc, pos[nn],
									&value, NullDouble);
						break;

					case D_Comp:
						valid = eval_sfc_MD(xysfc, pos[nn],
									NullDouble, &value);
						break;
					}

				/* Error if value cannot be extracted */
				if ( !valid )
					{
					(void) fprintf(stderr,
							"[retrieve_vlist_component] Error in eval_sfc_..()");
					(void) fprintf(stderr, " at position: %f  %f\n",
							pos[nn][X], pos[nn][Y]);
					(void) fprintf(stderr, "   for source: \"%s %s\"",
							SafeStr(fdesc->sdef->name),
							SafeStr(fdesc->subdef->name));
					(void) fprintf(stderr,
							"  at runtime: \"%s\"  validtime: \"%s\"\n",
							SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
					(void) fprintf(stderr,
							"   for element: \"%s\"  level: \"%s\"\n",
							SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
					xysfc = destroy_surface(xysfc);
					return NullPtr(VLIST *);
					}

				/* Add value to VLIST Object */
				(void) add_point_to_vlist(vlist, pos[nn], (float) value);
				}
			}

		/* Evaluate values based on m/d component values */
		else if ( MDcomp && NotNull(mlist) && NotNull(dlist) )
			{

			/* Evaluate values at each position */
			for ( nn=0; nn<npos; nn++ )
				{

				/* Set latitude and longitude for this position */
				if ( !pos_to_ll(&fdesc->mproj, pos[nn], &wlat, &wlon) )
					continue;

				/* Evaluate values for requested component */
				switch ( which )
					{

					/* Evaluate x values based on m/d component values */
					case X_Comp:

						/* Evaluate x value */
						rval  = wind_dir_xy(&fdesc->mproj, wlat, wlon,
																dlist->val[nn]);
						value = (double) (mlist->val[nn] * fpa_cosdeg(rval));

						/* Add x value to VLIST Object */
						(void) add_point_to_vlist(vlist, pos[nn], (float) value);
						break;

					/* Evaluate y values based on m/d component values */
					case Y_Comp:

						/* Evaluate y value */
						rval  = wind_dir_xy(&fdesc->mproj, wlat, wlon,
																dlist->val[nn]);
						value = (double) (mlist->val[nn] * fpa_sindeg(rval));

						/* Add y value to VLIST Object */
						(void) add_point_to_vlist(vlist, pos[nn], (float) value);
						break;

					/* Set magnitude based on magnitude component */
					case M_Comp:
						(void) add_point_to_vlist(vlist, pos[nn], mlist->val[nn]);
						break;

					/* Set direction based on direction component */
					case D_Comp:
						(void) add_point_to_vlist(vlist, pos[nn], dlist->val[nn]);
						break;
					}
				}
			}

		/* Error if missing component values */
		else
			{
			(void) fprintf(stderr, "[retrieve_vlist_component] Missing data to");
			(void) fprintf(stderr, " sample component: \"%d\"\n", which);
			(void) fprintf(stderr, "   for element: \"%s\"  level: \"%s\"\n",
					SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
			(void) free_vlist(vlist);
			FREEMEM(vlist);
			}

		/* Free the component surfaces */
		if ( IsNull(xysfc) && NotNull(xsfc) ) xsfc  = destroy_surface(xsfc);
		if ( IsNull(xysfc) && NotNull(ysfc) ) ysfc  = destroy_surface(ysfc);
		if ( NotNull(xysfc) )                 xysfc = destroy_surface(xysfc);

		/* Free the component values */
		if ( NotNull(mlist) )
			{
			(void) free_vlist(mlist);
			FREEMEM(mlist);
			}
		if ( NotNull(dlist) )
			{
			(void) free_vlist(dlist);
			FREEMEM(dlist);
			}

		/* Return pointer to VLIST Object */
		return vlist;
		}

	/* Error message if no field or component found */
	(void) fprintf(stderr,
			"[retrieve_vlist_component] No field or component found\n");
	(void) fprintf(stderr, "   for source: \"%s %s\"",
			SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
	(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
			SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
	(void) fprintf(stderr, "   for element: \"%s\"  level: \"%s\"\n",
			SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
	return NullPtr(VLIST *);
	}

/**********************************************************************/

/*********************************************************************/
/** Get a VLIST from a an equation evaluated from saved metafiles.
 *
 *	@param[in]	*fdesc		field descriptor
 *	@param[in]	npos		number of positions on field
 *	@param[in]	*pos		positions on field for evaluation
 *	@param[in]	inunit		input units for equation
 *	@param[in]	inebuf		input equation for field
 * 	@return Pointer to a VLIST object.
 *********************************************************************/
VLIST				*retrieve_vlist_by_equation

	(
	FLD_DESCRIPT	*fdesc,
	int				npos,
	POINT			*pos,
	STRING			inunit,
	STRING			inebuf
	)

	{
	FpaEQUATION_DEFAULTS	OldEqtnDefs;
	MAP_PROJ				mprojeval;
	POINT					*poseval;
	int						nn;
	double					dval;
	FpaEQTN_DATA			*pfield, *pvlist;
	VLIST					*vlist;

	/* Return Null pointer if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->sdef) || IsNull(fdesc->subdef)
			|| IsNull(fdesc->edef) || IsNull(fdesc->ldef) )
		return NullPtr(VLIST *);
	if ( IsNull(pos) ) return NullPtr(VLIST *);

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in retrieve_vlist_by_equation:\n");
		dprintf(stdout,"   path: %s\n",      SafeStr(fdesc->path));
		dprintf(stdout,"   source: %s %s\n", SafeStr(fdesc->sdef->name),
												SafeStr(fdesc->subdef->name));
		dprintf(stdout,"   rtime: %s\n",     SafeStr(fdesc->rtime));
		dprintf(stdout,"   vtime: %s\n",     SafeStr(fdesc->vtime));
		dprintf(stdout,"   element: %s\n",   SafeStr(fdesc->edef->name));
		dprintf(stdout,"   level: %s\n",     SafeStr(fdesc->ldef->name));
		dprintf(stdout,"   units: %s\n",     SafeStr(inunit));
		dprintf(stdout,"   equation: %s\n",  SafeStr(inebuf));
		}

	/* Check the accuracy of input units and equation */
	if ( !valid_units_and_equation(inunit, inebuf) ) return NullPtr(VLIST *);

	/* Ensure that equation handler is not trapped in an endless loop */
	if ( check_endless_loop(inebuf) )
		{
		(void) reset_endless_loop();
		return NullPtr(VLIST *);
		}

	/* Store old Equation Defaults */
	(void) save_equation_defaults(&OldEqtnDefs);

	/* Reset default path name and source */
	(void) safe_strcpy(FpaEqtnDefs.path,      fdesc->path);
	(void) safe_strcpy(FpaEqtnDefs.source,    fdesc->sdef->name);
	(void) safe_strcpy(FpaEqtnDefs.subsource, fdesc->subdef->name);

	/* Reset default run and valid timestamps */
	(void) safe_strcpy(FpaEqtnDefs.rtime, fdesc->rtime);
	(void) safe_strcpy(FpaEqtnDefs.vtime, fdesc->vtime);

	/* Reset default levels based on enumerated level type */
	switch ( fdesc->ldef->lvl_type )
		{

		/* Reset all default levels from single pressure level */
		case FpaC_MSL:
		case FpaC_SURFACE:
		case FpaC_LEVEL:
		case FpaC_GEOGRAPHY:
		case FpaC_ANNOTATION:
			(void) safe_strcpy(FpaEqtnDefs.lvl,    fdesc->ldef->lev_lvls->lvl);
			(void) safe_strcpy(FpaEqtnDefs.uprlvl, fdesc->ldef->lev_lvls->lvl);
			(void) safe_strcpy(FpaEqtnDefs.lwrlvl, fdesc->ldef->lev_lvls->lvl);
			break;

		/* Reset default levels from upper and lower pressure levels */
		case FpaC_LAYER:
			(void) safe_strcpy(FpaEqtnDefs.lvl,    fdesc->ldef->lev_lvls->uprlvl);
			(void) safe_strcpy(FpaEqtnDefs.uprlvl, fdesc->ldef->lev_lvls->uprlvl);
			(void) safe_strcpy(FpaEqtnDefs.lwrlvl, fdesc->ldef->lev_lvls->lwrlvl);
			break;

		/* Do not reset levels for other cases */
		default:
			break;
		}

	/* Reset default point evaluation information */
	FpaEqtnDefs.pointeval = TRUE;

	/* Determine an evaluation map projection                      */
	/*  ... which will be a subgrid of the original map projection */
	/* Note that this is done only once for each equation          */
	if ( !FpaEqtnDefs.subgrid )
		{

		/* Determine the evalutation map projection */
		if ( !evaluation_map_projection(&fdesc->mproj, npos, pos,
				&mprojeval, &poseval) )
			{
			(void) pop_endless_loop();
			return NullPtr(VLIST *);
			}

		/* Set default original and evaluation map projections */
		/*  and positions on evaluation map projection         */
		FpaEqtnDefs.subgrid = TRUE;
		(void) copy_map_projection(&FpaEqtnDefs.mprojOrig, &fdesc->mproj);
		(void) copy_map_projection(&FpaEqtnDefs.mprojEval, &mprojeval);
		FpaEqtnDefs.numposEval = npos;
		FpaEqtnDefs.posEval = GETMEM(FpaEqtnDefs.posEval, POINT,
				FpaEqtnDefs.numposEval);
		for ( nn=0; nn<FpaEqtnDefs.numposEval; nn++ )
			(void) copy_point(FpaEqtnDefs.posEval[nn], poseval[nn]);
		}

	/* Evaluate the equation string                      */
	/*  ... and convert evaluated Object to VLIST Object */
	pfield = evaluate_equation(inebuf);
	pvlist = convert_eqtn_data(FpaEQT_Vlist, pfield);

	/* Error message if problem evaluating equation string */
	if ( IsNull(pfield) || IsNull(pvlist) )
		{
		(void) fprintf(stderr, "[retrieve_vlist_by_equation] Problem evaluating");
		(void) fprintf(stderr, " equation: \"%s\"\n", SafeStr(inebuf));
		(void) fprintf(stderr, "   for source: \"%s %s\"",
				SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
		(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
				SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
		(void) fprintf(stderr, "   for element: \"%s\"  level: \"%s\"\n",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		/* Free space used by work structures */
		(void) free_eqtn_data(pfield);
		(void) free_eqtn_data(pvlist);
		(void) restore_equation_defaults(&OldEqtnDefs);
		(void) pop_endless_loop();
		return NullPtr(VLIST *);
		}

	/* Build VLIST Object to hold evaluation */
	vlist = INITMEM(VLIST, 1);

	/* Transfer data to VLIST Object */
	(void) copy_vlist(vlist, &pvlist->Data.vlist);

	/* Convert VLIST values to MKS */
	for ( nn=0; nn<vlist->numpts; nn++ )
		{
		if (convert_value(inunit, (double) vlist->val[nn],
						FpaCmksUnits, &dval)) vlist->val[nn] = (float) dval;
		}

	/* Free space used by work structures */
	(void) free_eqtn_data(pfield);
	(void) free_eqtn_data(pvlist);

	/* Finally, restore old Equation Defaults and */
	/*  return pointer to VLIST Object            */
	(void) restore_equation_defaults(&OldEqtnDefs);
	(void) pop_endless_loop();
	return vlist;
	}

/**********************************************************************/

/*********************************************************************/
/** Get a VLIST evaluated from an attribute of a saved metafile.
 *
 *	@param[in]	*fdesc		field descriptor
 *	@param[in]	npos		number of positions on field
 *	@param[in]	*pos		positions on field for evaluation
 *	@param[in]	units		input units for attribute
 *	@param[in]	attrib		attribute to be evaluated
 *	@param[in]	xlookup		lookup table for attribute
 *	@param[in]	defval		default value for attribute
 *	@param[in]	proximity	proximity to features (in km)
 * 	@return Pointer to a VLIST object.
 *********************************************************************/
VLIST				*retrieve_vlist_by_attrib

	(
	FLD_DESCRIPT	*fdesc,
	int				npos,
	POINT			*pos,
	STRING			units,
	STRING			attrib,
	STRING			xlookup,
	float			defval,
	float			proximity
	)

	{
	FpaEQUATION_DEFAULTS	OldEqtnDefs;
	MAP_PROJ				mprojeval;
	POINT					*poseval;
	int						nn, iarea, isub;
	STRING					val, pval;
	LOGICAL					inside;
	float					*vals, xval, fdist;
	double					dval;
	SET						set, copyset;
	AREA					area, carea;
	SUBAREA					subarea;
	CURVE					curve;
	SPOT					spot;
	POINT					fpos;
	CAL						cal;
	FpaConfigFieldStruct	*fdef;
	VLIST					*vlist;

	/* Return Null pointer if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->sdef) || IsNull(fdesc->subdef)
			|| IsNull(fdesc->edef) || IsNull(fdesc->ldef) )
		return NullPtr(VLIST *);
	if ( IsNull(pos) ) return NullPtr(VLIST *);

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in retrieve_vlist_by_attrib:\n");
		dprintf(stdout,"   path: %s\n",      SafeStr(fdesc->path));
		dprintf(stdout,"   source: %s %s\n", SafeStr(fdesc->sdef->name),
												SafeStr(fdesc->subdef->name));
		dprintf(stdout,"   rtime: %s\n",     SafeStr(fdesc->rtime));
		dprintf(stdout,"   vtime: %s\n",     SafeStr(fdesc->vtime));
		dprintf(stdout,"   element: %s\n",   SafeStr(fdesc->edef->name));
		dprintf(stdout,"   level: %s\n",     SafeStr(fdesc->ldef->name));
		dprintf(stdout,"   units: %s\n",     SafeStr(units));
		dprintf(stdout,"   attrib: %s\n",    SafeStr(attrib));
		dprintf(stdout,"   xlookup: %s\n",   SafeStr(xlookup));
		dprintf(stdout,"   defval: %.2f\n",         defval);
		dprintf(stdout,"   proximity: %.2f (km)\n", proximity);
		}

	/* Get detailed field information */
	fdef = get_field_info(fdesc->edef->name, fdesc->ldef->name);
	if ( IsNull(fdef) ) return NullPtr(VLIST *);

	/* Retrieve SET of features based on field type */
	switch ( fdef->element->fld_type )
		{

		/* Retrieve SET of areas */
		case FpaC_DISCRETE:
		case FpaC_WIND:
			set = retrieve_areaset(fdesc);
			break;

		/* Retrieve SET of curves */
		case FpaC_LINE:
			set = retrieve_curveset(fdesc);
			break;

		/* Retrieve SET of spots */
		case FpaC_SCATTERED:
			set = retrieve_spotset(fdesc);
			break;

		default:
			set = NullSet;
			break;
		}

	/* Error message if problem evaluating features */
	if ( IsNull(set) )
		{
		(void) fprintf(stderr,
				"[retrieve_vlist_by_attrib] Problem evaluating field\n");
		(void) fprintf(stderr, "   for source: \"%s %s\"",
				SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
		(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
				SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
		(void) fprintf(stderr, "   for element: \"%s\"  level: \"%s\"\n",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		return NullPtr(VLIST *);
		}

	/* Store old Equation Defaults */
	(void) save_equation_defaults(&OldEqtnDefs);

	/* Reset default path name and source */
	(void) safe_strcpy(FpaEqtnDefs.path,      fdesc->path);
	(void) safe_strcpy(FpaEqtnDefs.source,    fdesc->sdef->name);
	(void) safe_strcpy(FpaEqtnDefs.subsource, fdesc->subdef->name);

	/* Reset default run and valid timestamps */
	(void) safe_strcpy(FpaEqtnDefs.rtime, fdesc->rtime);
	(void) safe_strcpy(FpaEqtnDefs.vtime, fdesc->vtime);

	/* Reset default levels based on enumerated level type */
	switch ( fdesc->ldef->lvl_type )
		{

		/* Reset all default levels from single pressure level */
		case FpaC_MSL:
		case FpaC_SURFACE:
		case FpaC_LEVEL:
		case FpaC_GEOGRAPHY:
		case FpaC_ANNOTATION:
			(void) safe_strcpy(FpaEqtnDefs.lvl,    fdesc->ldef->lev_lvls->lvl);
			(void) safe_strcpy(FpaEqtnDefs.uprlvl, fdesc->ldef->lev_lvls->lvl);
			(void) safe_strcpy(FpaEqtnDefs.lwrlvl, fdesc->ldef->lev_lvls->lvl);
			break;

		/* Reset default levels from upper and lower pressure levels */
		case FpaC_LAYER:
			(void) safe_strcpy(FpaEqtnDefs.lvl,    fdesc->ldef->lev_lvls->uprlvl);
			(void) safe_strcpy(FpaEqtnDefs.uprlvl, fdesc->ldef->lev_lvls->uprlvl);
			(void) safe_strcpy(FpaEqtnDefs.lwrlvl, fdesc->ldef->lev_lvls->lwrlvl);
			break;

		/* Do not reset levels for other cases */
		default:
			break;
		}

	/* Reset default point evaluation information */
	FpaEqtnDefs.pointeval = TRUE;

	/* Determine an evaluation map projection                      */
	/*  ... which will be a subgrid of the original map projection */
	/* Note that this is done only once for each evaluation        */
	if ( !FpaEqtnDefs.subgrid )
		{

		/* Determine the evalutation map projection */
		if ( !evaluation_map_projection(&fdesc->mproj, npos, pos,
				&mprojeval, &poseval) )
			{
			return NullPtr(VLIST *);
			}

		/* Set default original and evaluation map projections */
		/*  and positions on evaluation map projection         */
		FpaEqtnDefs.subgrid = TRUE;
		(void) copy_map_projection(&FpaEqtnDefs.mprojOrig, &fdesc->mproj);
		(void) copy_map_projection(&FpaEqtnDefs.mprojEval, &mprojeval);
		FpaEqtnDefs.numposEval = npos;
		FpaEqtnDefs.posEval = GETMEM(FpaEqtnDefs.posEval, POINT,
				FpaEqtnDefs.numposEval);
		for ( nn=0; nn<FpaEqtnDefs.numposEval; nn++ )
			(void) copy_point(FpaEqtnDefs.posEval[nn], poseval[nn]);
		}

	/* Allocate space for the attribute values */
	vals = INITMEM(float, FpaEqtnDefs.numposEval);

	/* Evaluate the attribute at each point on the field */
	for ( nn=0; nn<npos; nn++ )
		{

		/* Evaluate the attribute based on field type */
		switch ( fdef->element->fld_type )
			{

			/* Evaluate the attribute from SET of areas */
			case FpaC_DISCRETE:

				/* Evaluate areas based on proximity */
				if ( proximity > 0.0 )
					{

					/* Make a copy of the area or subareas in the area */
					copyset = create_set("areas");
					for ( iarea=0; iarea<set->num; iarea++ )
						{
						area = (AREA) set->list[iarea];

						/* Ensure that area is within proximity */
						(void) area_test_point(area, FpaEqtnDefs.posEval[nn],
												NULL, fpos, NULL, NULL, NULL,
												&inside);
						if ( !inside )
							{
							fdist  = great_circle_distance(&FpaEqtnDefs.mprojEval,
												FpaEqtnDefs.posEval[nn], fpos);
							fdist /= 1000.0;
							if ( fdist > proximity ) continue;
							}

						/* Copy the area if no subareas */
						if ( area->numdiv == 0)
							{
							carea = copy_area(area, FALSE);
							if ( NotNull(carea) )
								{
								(void) add_item_to_set(copyset, (ITEM) carea);
								}
							}

						/* Create an area from each subarea */
						else
							{
							for ( isub=0; isub<=area->numdiv; isub++ )
								{
								subarea = area->subareas[isub];
								carea   = area_from_subarea(subarea);
								if ( NotNull(carea) )
									{
									(void) add_item_to_set(copyset,
															(ITEM) carea);
									}
								}
							}
						}

					/* Search for closest area within proximity */
					/*  that matches the required attribute     */
					while ( copyset->num > 0 )
						{

						/* Find an enclosing area */
						area = enclosing_area(copyset, FpaEqtnDefs.posEval[nn],
											PickFirst, NullFloat, NullChar);

						/* Set sampling point and proximity (if inside) */
						if ( NotNull(area) )
							{

							/* Check for attribute */
							val = CAL_get_attribute(area->attrib, attrib);

							/* Match attribute value with lookup table */
							if ( !blank(val) && !blank(xlookup) )
								{
								if ( match_lookup_table(xlookup, val, &xval) )
									{
									vals[nn] = xval;

									/* Destroy what is left of the copy */
									(void) destroy_set(copyset);
									continue;
									}
								}

							/* Extract attribute value directly */
							else if ( !blank(val) )
								{
								dval = strtod(val, &pval);
								if ( !same(val, pval) )
									{
									vals[nn] = (float) dval;

									/* Destroy what is left of the copy */
									(void) destroy_set(copyset);
									continue;
									}
								}
							}

						/* Find the closest area within proximity */
						else
							{
							area   = closest_area(copyset,
											FpaEqtnDefs.posEval[nn],
											NULL, fpos, NULL, NULL, NULL);
							fdist  = great_circle_distance(&FpaEqtnDefs.mprojEval,
											FpaEqtnDefs.posEval[nn], fpos);
							fdist /= 1000.0;

							/* Exit loop if no more areas close enough */
							if ( fdist > proximity ) break;

							/* Check for attribute */
							val = CAL_get_attribute(area->attrib, attrib);

							/* Match attribute value with lookup table */
							if ( !blank(val) && !blank(xlookup) )
								{
								if ( match_lookup_table(xlookup, val, &xval) )
									{
									vals[nn] = xval;

									/* Destroy what is left of the copy */
									(void) destroy_set(copyset);
									continue;
									}
								}

							/* Extract attribute value directly */
							else if ( !blank(val) )
								{
								dval = strtod(val, &pval);
								if ( !same(val, pval) )
									{
									vals[nn] = (float) dval;

									/* Destroy what is left of the copy */
									(void) destroy_set(copyset);
									continue;
									}
								}
							}

						/* Attribute did not match               */
						/*  ... so remove this area from the set */
						(void) remove_item_from_set(copyset, (ITEM) area);
						}

					/* If no areas left in set ... use default value */
					vals[nn] = defval;

					/* Destroy what is left of the copy */
					(void) destroy_set(copyset);
					continue;
					}

				/* Sample areas directly */
				else
					{

					/* Find area enclosing point                       */
					/* Note that this should return a background value */
					/*  with no subarea if no enclosing area is found  */
					(void) eval_areaset(set, FpaEqtnDefs.posEval[nn],
							PickFirst, &subarea, &cal);

					/* Check for attribute */
					val = CAL_get_attribute(cal, attrib);

					/* Match attribute value with lookup table */
					if ( !blank(val) && !blank(xlookup) )
						{
						if ( match_lookup_table(xlookup, val, &xval) )
							{
							vals[nn] = xval;
							continue;
							}
						}

					/* Extract attribute value directly */
					else if ( !blank(val) )
						{
						dval = strtod(val, &pval);
						if ( !same(val, pval) )
							{
							vals[nn] = (float) dval;
							continue;
							}
						}

					/* Use default value if no attribute value found */
					vals[nn] = defval;
					continue;
					}

			/* Evaluate the attribute from SET of curves */
			case FpaC_LINE:

				/* Make a copy of the line curves */
				copyset = copy_set(set);

				/* Search for closest curve within proximity */
				/*  that matches the required attribute      */
				while ( copyset->num > 0 )
					{

					/* Find the closest curve */
					curve = closest_curve(copyset, FpaEqtnDefs.posEval[nn],
											NullFloat, fpos, NullInt);
					if ( IsNull(curve) || IsNull(curve->attrib) )
						{
						(void) remove_item_from_set(copyset, (ITEM) curve);
						continue;
						}

					/* Check the proximity */
					fdist  = great_circle_distance(&FpaEqtnDefs.mprojEval,
											FpaEqtnDefs.posEval[nn], fpos);
					fdist /= 1000.0;

					/* Exit loop if no more curves are close enough */
					if ( fdist > proximity ) break;

					/* Check for attribute */
					val = CAL_get_attribute(curve->attrib, attrib);

					/* Match attribute value with lookup table */
					if ( !blank(val) && !blank(xlookup) )
						{
						if ( match_lookup_table(xlookup, val, &xval) )
							{
							vals[nn] = xval;

							/* Destroy what is left of the copy */
							(void) destroy_set(copyset);
							continue;
							}
						}

					/* Extract attribute value directly */
					else if ( !blank(val) )
						{
						dval = strtod(val, &pval);
						if ( !same(val, pval) )
							{
							vals[nn] = (float) dval;

							/* Destroy what is left of the copy */
							(void) destroy_set(copyset);
							continue;
							}
						}

					/* Attribute did not match                */
					/*  ... so remove this curve from the set */
					(void) remove_item_from_set(copyset, (ITEM) curve);
					}

				/* If no curves left in set ... use default value */
				vals[nn] = defval;

				/* Destroy what is left of the copy */
				(void) destroy_set(copyset);
				continue;

			/* Evaluate the attribute from SET of spots */
			case FpaC_SCATTERED:

				/* Make a copy of the scattered spots */
				copyset = copy_set(set);

				/* Search for closest spot within proximity */
				/*  that matches the required attribute     */
				while ( copyset->num > 0 )
					{

					/* Find the closest spot */
					spot = closest_spot(copyset, FpaEqtnDefs.posEval[nn], NULL,
											NULL, NullFloat, fpos);
					if ( IsNull(spot) || IsNull(spot->attrib) )
						{
						(void) remove_item_from_set(copyset, (ITEM) spot);
						continue;
						}

					/* Check the proximity */
					fdist  = great_circle_distance(&FpaEqtnDefs.mprojEval,
											FpaEqtnDefs.posEval[nn], fpos);
					fdist /= 1000.0;
					
					/* Exit loop if no more spots are close enough */
					if ( fdist > proximity ) break;

					/* Check for attribute */
					val = CAL_get_attribute(spot->attrib, attrib);

					/* Match attribute value with lookup table */
					if ( !blank(val) && !blank(xlookup) )
						{
						if ( match_lookup_table(xlookup, val, &xval) )
							{
							xval = defval;
							vals[nn] = xval;

							/* Destroy what is left of the copy */
							(void) destroy_set(copyset);
							continue;
							}
						}

					/* Extract attribute value directly */
					else if ( !blank(val) )
						{
						dval = strtod(val, &pval);
						if ( !same(val, pval) )
							{
							vals[nn] = (float) dval;

							/* Destroy what is left of the copy */
							(void) destroy_set(copyset);
							continue;
							}
						}

					/* Attribute did not match               */
					/*  ... so remove this spot from the set */
					(void) remove_item_from_set(copyset, (ITEM) spot);
					}

				/* If no spots left in set ... use default value */
				vals[nn] = defval;

				/* Destroy what is left of the copy */
				(void) destroy_set(copyset);
				continue;
			}
		}

	/* Build VLIST Object to hold evaluation */
	vlist = INITMEM(VLIST, 1);
	(void) init_vlist(vlist);

	/* Convert values to MKS and transfer to VLIST Object */
	for ( nn=0; nn<npos; nn++ )
		{
		if ( convert_value(units, vals[nn], FpaCmksUnits, &dval) )
			{
			vals[nn] = (float) dval;
			}
		(void) add_point_to_vlist(vlist, pos[nn], vals[nn]);
		}

	/* Free space used by work structures */
	FREEMEM(vals);

	/* Finally, restore old Equation Defaults and */
	/*  return pointer to VLIST Object            */
	(void) restore_equation_defaults(&OldEqtnDefs);
	return vlist;
	}

/**********************************************************************
 ***                                                                ***
 *** c h e c k _ r e t r i e v e _ m e t a s f c                    ***
 *** f i n d _ r e t r i e v e _ m e t a s f c                      ***
 *** f i n d _ r e t r i e v e _ m e t a s f c _ b y _ a t t r i b  ***
 ***                                                                ***
 **********************************************************************/

/*********************************************************************/
/** Check whether metafile containing data for given default
 *  values exists, or whether an equation for given default
 *  values can be evaluated from existing metafiles
 *
 *  NOTE: This routine looks for an exact match in valid time!
 *
 *	@param[in]	*fdesc		field descriptor
 *  @return True if such a file exists or if the value can be
 *  		evaluated using an equation.
 *********************************************************************/
LOGICAL				check_retrieve_metasfc

	(
	FLD_DESCRIPT	*fdesc
	)

	{
	STRING							metapath;
	int								icmp, isrc;
	FLD_DESCRIPT					descript;
	FpaConfigFieldStruct			*fdef, *fdefx;
	FpaConfigElementComponentStruct	*components;
	FpaConfigElementEquationStruct	*equation;
	FpaConfigElementValCalcStruct	*valcalc;

	/* Internal variables to test value calculations */
	static	int			TestNumber       = 1;
	static	POINT		TestPointArray[] = { ZERO_POINT };

	/* Return FALSE if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->sdef) || IsNull(fdesc->subdef)
			|| IsNull(fdesc->edef) || IsNull(fdesc->ldef) ) return FALSE;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in check_retrieve_metasfc:\n");
		dprintf(stdout,"   path: %s\n",      SafeStr(fdesc->path));
		dprintf(stdout,"   source: %s %s\n", SafeStr(fdesc->sdef->name),
												SafeStr(fdesc->subdef->name));
		dprintf(stdout,"   rtime: %s\n",     SafeStr(fdesc->rtime));
		dprintf(stdout,"   vtime: %s\n",     SafeStr(fdesc->vtime));
		dprintf(stdout,"   element: %s\n",   SafeStr(fdesc->edef->name));
		dprintf(stdout,"   level: %s\n",     SafeStr(fdesc->ldef->name));
		}

	/* Get detailed field information */
	fdef = get_field_info(fdesc->edef->name, fdesc->ldef->name);
	if ( IsNull(fdef) ) return FALSE;

	/* Check for metafile containing data for field descriptor */
	metapath = check_meta_filename(fdesc);
	if ( !blank(metapath) ) return TRUE;

	/* Check for components for vector fields */
	if ( fdef->element->fld_type == FpaC_VECTOR )
		{
		if ( DebugMode )
			{
			dprintf(stdout, "  Searching for vector components for: \"%s %s\"\n",
					SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
			}
		components = fdef->element->elem_detail->components;
		if ( NotNull(components) && components->ncomp > 0 )
			{

			/* Check for metafile for each component */
			for ( icmp=0; icmp<components->ncomp; icmp++ )
				{

				/* Reset field descriptor for this component */
				(void) copy_fld_descript(&descript, fdesc);
				(void) set_fld_descript(&descript,
									FpaF_ELEMENT, components->comp_edefs[icmp],
									FpaF_END_OF_LIST);

				/* Get detailed field information for this component */
				fdefx = get_field_info(descript.edef->name,
										descript.ldef->name);
				if ( IsNull(fdefx) ) return FALSE;

				/* Check for metafile for this component */
				metapath = check_meta_filename(&descript);
				if ( !blank(metapath) ) continue;

				/* Check for equation for this component */
				if ( DebugMode )
					{
					dprintf(stdout, "  Searching for equation for: \"%s %s\"\n",
							SafeStr(descript.edef->name),
							SafeStr(descript.ldef->name));
					}
				if ( NotNull(fdefx->element->elem_detail->equation) )
					{

					/* Component OK if force calculation is set! */
					equation = fdefx->element->elem_detail->equation;
					if ( equation->force ) continue;

					/* Check result of equation for this component */
					if ( check_calculate_equation(&descript,
								equation->units->name, equation->eqtn) )
						continue;
					}

				/* Check for value calculation for this component */
				if ( DebugMode )
					{
					dprintf(stdout,
							"  Searching for value calculation for: \"%s %s\"\n",
							SafeStr(descript.edef->name),
							SafeStr(descript.ldef->name));
					}
				if ( NotNull(fdefx->element->elem_detail->valcalc) )
					{

					/* Check for matching value calculation source type */
					valcalc = fdefx->element->elem_detail->valcalc;
					for ( isrc=0; isrc<valcalc->nsrc_type; isrc++ )
						{
						if ( descript.sdef->src_type ==
													valcalc->src_types[isrc] )
							{

							/* Component OK if force calculation is set! */
							if ( valcalc->force ) break;

							/* Check result of value cross reference */
							/*  for this component                   */
							if ( check_extract_value_by_crossref(valcalc->vcalc->name,
									fdesc, FALSE, TestNumber, TestPointArray) )
								break;
							}
						}

					/* Found a value calculation for this component */
					if ( isrc < valcalc->nsrc_type ) continue;

					/* Did not find a matching value calculation source type */
					/*  for this component                                   */
					else if ( DebugMode )
						{
						dprintf(stderr, "[check_retrieve_metasfc] No matching");
						dprintf(stderr, " value calculation source type\n");
						dprintf(stderr, "   for source: \"%s %s\"",
								SafeStr(descript.sdef->name),
								SafeStr(descript.subdef->name));
						dprintf(stderr,
								"  at runtime: \"%s\"  validtime: \"%s\"\n",
								SafeStr(descript.rtime),
								SafeStr(descript.vtime));
						dprintf(stderr, "   for crossref: \"%s\"",
								SafeStr(valcalc->vcalc->name));
						dprintf(stderr, "  of element: \"%s\"  level: \"%s\"\n",
								SafeStr(descript.edef->name),
								SafeStr(descript.ldef->name));
						}
					}

				/* Return FALSE if no metafile, equation, or value */
				/*  calculation found for this component           */
				if ( DebugMode )
					{
					dprintf(stderr, "[check_retrieve_metasfc] No field");
					dprintf(stderr, " or equation found\n");
					dprintf(stderr, "   for source: %s %s",
							SafeStr(descript.sdef->name),
							SafeStr(descript.subdef->name));
					dprintf(stderr, "  at runtime: %s  validtime: %s\n",
							SafeStr(descript.rtime), SafeStr(descript.vtime));
					dprintf(stderr, "   for element: %s  level: %s\n",
							SafeStr(descript.edef->name),
							SafeStr(descript.ldef->name));
					}
				return FALSE;
				}

			/* Return TRUE if all components found */
			return TRUE;
			}
		}

	/* Check for equation to check */
	if ( DebugMode )
		{
		dprintf(stdout, "  Searching for equation for: \"%s %s\"\n",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		}
	if ( NotNull(fdef->element->elem_detail->equation) )
		{

		/* Return TRUE if force calculation is set! */
		equation = fdef->element->elem_detail->equation;
		if ( equation->force ) return TRUE;

		/* Return result of checking equation */
		return check_calculate_equation(fdesc,
										equation->units->name, equation->eqtn);
		}

	/* Check for value calculation if no field or equation found */
	if ( DebugMode )
		{
		dprintf(stdout, "  Searching for value calculation for: \"%s %s\"\n",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		}
	if ( NotNull(fdef->element->elem_detail->valcalc) )
		{

		/* Check for matching value calculation source type */
		valcalc = fdef->element->elem_detail->valcalc;
		for ( isrc=0; isrc<valcalc->nsrc_type; isrc++ )
			{
			if ( fdesc->sdef->src_type == valcalc->src_types[isrc] )
				{

				/* Return TRUE if force calculation is set! */
				if ( valcalc->force ) return TRUE;

				/* Check the field using the value cross reference */
				return check_extract_value_by_crossref(valcalc->vcalc->name,
						fdesc, FALSE, TestNumber, TestPointArray);
				}
			}

		/* Did not find a matching value calculation source type */
		if ( isrc >= valcalc->nsrc_type && DebugMode )
			{
			dprintf(stderr, "[check_retrieve_metasfc] No matching");
			dprintf(stderr, " value calculation source type\n");
			dprintf(stderr, "   for source: \"%s %s\"",
					SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
			dprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
					SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
			dprintf(stderr, "   for crossref: \"%s\"",
					SafeStr(valcalc->vcalc->name));
			dprintf(stderr, "  of element: \"%s\"  level: \"%s\"\n",
					SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
			}
		}

	/* Return FALSE if no field, equation, or value calculation found */
	if ( DebugMode )
		{
		dprintf(stderr, "[check_retrieve_metasfc] No field");
		dprintf(stderr, " or equation found\n");
		dprintf(stderr, "   for source: %s %s",
				SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
		dprintf(stderr, "  at runtime: %s  validtime: %s\n",
				SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
		dprintf(stderr, "   for element: %s  level: %s\n",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		}
	return FALSE;
	}

/**********************************************************************/

/*********************************************************************/
/** Check whether data for given default values can be extracted
 * from existing metafiles, or whether an equation for given
 * default values can be evaluated from existing metafiles
 *
 * NOTE: This routine also checks for "matching" metafiles for daily
 *  or static fields!
 *
 *	@param[in]	*fdesc		field descriptor
 *  @return True if data can be extracted.
 *********************************************************************/
LOGICAL				find_retrieve_metasfc

	(
	FLD_DESCRIPT	*fdesc
	)

	{
	STRING							metapath;
	int								icmp, isrc;
	FLD_DESCRIPT					descript;
	FpaConfigFieldStruct			*fdef, *fdefx;
	FpaConfigElementComponentStruct	*components;
	FpaConfigElementEquationStruct	*equation;
	FpaConfigElementValCalcStruct	*valcalc;

	/* Internal variables to test value calculations */
	static	int			TestNumber       = 1;
	static	POINT		TestPointArray[] = { ZERO_POINT };

	/* Return FALSE if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->sdef) || IsNull(fdesc->subdef)
			|| IsNull(fdesc->edef) || IsNull(fdesc->ldef) ) return FALSE;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in find_retrieve_metasfc:\n");
		dprintf(stdout,"   path: %s\n",      SafeStr(fdesc->path));
		dprintf(stdout,"   source: %s %s\n", SafeStr(fdesc->sdef->name),
												SafeStr(fdesc->subdef->name));
		dprintf(stdout,"   rtime: %s\n",     SafeStr(fdesc->rtime));
		dprintf(stdout,"   vtime: %s\n",     SafeStr(fdesc->vtime));
		dprintf(stdout,"   element: %s\n",   SafeStr(fdesc->edef->name));
		dprintf(stdout,"   level: %s\n",     SafeStr(fdesc->ldef->name));
		}

	/* Get detailed field information */
	fdef = get_field_info(fdesc->edef->name, fdesc->ldef->name);
	if ( IsNull(fdef) ) return FALSE;

	/* Check for metafile containing data for field descriptor */
	metapath = find_meta_filename(fdesc);
	if ( !blank(metapath) ) return TRUE;

	/* Check for components for vector fields */
	if ( fdef->element->fld_type == FpaC_VECTOR )
		{
		if ( DebugMode )
			{
			dprintf(stdout, "  Searching for vector components for: \"%s %s\"\n",
					SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
			}
		components = fdef->element->elem_detail->components;
		if ( NotNull(components) && components->ncomp > 0 )
			{

			/* Check for metafile for each component */
			for ( icmp=0; icmp<components->ncomp; icmp++ )
				{

				/* Reset field descriptor for this component */
				(void) copy_fld_descript(&descript, fdesc);
				(void) set_fld_descript(&descript,
									FpaF_ELEMENT, components->comp_edefs[icmp],
									FpaF_END_OF_LIST);

				/* Get detailed field information for this component */
				fdefx = get_field_info(descript.edef->name,
										descript.ldef->name);
				if ( IsNull(fdefx) ) return FALSE;

				/* Check for metafile for this component */
				metapath = find_meta_filename(&descript);
				if ( !blank(metapath) ) continue;

				/* Check for equation for this component */
				if ( DebugMode )
					{
					dprintf(stdout, "  Searching for equation for: \"%s %s\"\n",
							SafeStr(descript.edef->name),
							SafeStr(descript.ldef->name));
					}
				if ( NotNull(fdefx->element->elem_detail->equation) )
					{

					/* Component OK if force calculation is set! */
					equation = fdefx->element->elem_detail->equation;
					if ( equation->force ) continue;

					/* Check result of equation for this component */
					if ( check_calculate_equation(&descript,
								equation->units->name, equation->eqtn) )
						continue;
					}

				/* Check for value calculation for this component */
				if ( DebugMode )
					{
					dprintf(stdout,
							"  Searching for value calculation for: \"%s %s\"\n",
							SafeStr(descript.edef->name),
							SafeStr(descript.ldef->name));
					}
				if ( NotNull(fdefx->element->elem_detail->valcalc) )
					{

					/* Check for matching value calculation source type */
					valcalc = fdefx->element->elem_detail->valcalc;
					for ( isrc=0; isrc<valcalc->nsrc_type; isrc++ )
						{
						if ( descript.sdef->src_type ==
													valcalc->src_types[isrc] )
							{

							/* Component OK if force calculation is set! */
							if ( valcalc->force ) break;

							/* Check result of value cross reference */
							/*  for this component                   */
							if ( check_extract_value_by_crossref(valcalc->vcalc->name,
									fdesc, FALSE, TestNumber, TestPointArray) )
								break;
							}
						}

					/* Found a value calculation for this component */
					if ( isrc < valcalc->nsrc_type ) continue;

					/* Did not find a matching value calculation source type */
					else if ( DebugMode )
						{
						dprintf(stderr, "[find_retrieve_metasfc] No matching");
						dprintf(stderr, " value calculation source type\n");
						dprintf(stderr, "   for source: \"%s %s\"",
								SafeStr(descript.sdef->name),
								SafeStr(descript.subdef->name));
						dprintf(stderr,
								"  at runtime: \"%s\"  validtime: \"%s\"\n",
								SafeStr(descript.rtime),
								SafeStr(descript.vtime));
						dprintf(stderr, "   for crossref: \"%s\"",
								SafeStr(valcalc->vcalc->name));
						dprintf(stderr, "  of element: \"%s\"  level: \"%s\"\n",
								SafeStr(descript.edef->name),
								SafeStr(descript.ldef->name));
						}
					}

				/* Return FALSE if no metafile, equation, or value */
				/*  calculation found for this component           */
				if ( DebugMode )
					{
					dprintf(stderr, "[find_retrieve_metasfc] No field");
					dprintf(stderr, " or equation found\n");
					dprintf(stderr, "   for source: %s %s",
							SafeStr(descript.sdef->name),
							SafeStr(descript.subdef->name));
					dprintf(stderr, "  at runtime: %s  validtime: %s\n",
							SafeStr(descript.rtime), SafeStr(descript.vtime));
					dprintf(stderr, "   for element: %s  level: %s\n",
							SafeStr(descript.edef->name),
							SafeStr(descript.ldef->name));
					}
				return FALSE;
				}

			/* Return TRUE if all components found */
			return TRUE;
			}
		}

	/* Check for equation to check */
	if ( DebugMode )
		{
		dprintf(stdout, "  Searching for equation for: \"%s %s\"\n",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		}
	if ( NotNull(fdef->element->elem_detail->equation) )
		{

		/* Return TRUE if force calculation is set! */
		equation = fdef->element->elem_detail->equation;
		if ( equation->force ) return TRUE;

		/* Return result of checking equation */
		return check_calculate_equation(fdesc,
										equation->units->name, equation->eqtn);
		}

	/* Check for value calculation if no field or equation found */
	if ( DebugMode )
		{
		dprintf(stdout, "  Searching for value calculation for: \"%s %s\"\n",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		}
	if ( NotNull(fdef->element->elem_detail->valcalc) )
		{

		/* Check for matching value calculation source type */
		valcalc = fdef->element->elem_detail->valcalc;
		for ( isrc=0; isrc<valcalc->nsrc_type; isrc++ )
			{
			if ( fdesc->sdef->src_type == valcalc->src_types[isrc] )
				{

				/* Return TRUE if force calculation is set! */
				if ( valcalc->force ) return TRUE;

				/* Check the field using the value cross reference */
				return check_extract_value_by_crossref(valcalc->vcalc->name,
						fdesc, FALSE, TestNumber, TestPointArray);
				}
			}

		/* Did not find a matching value calculation source type */
		if ( isrc >= valcalc->nsrc_type && DebugMode )
			{
			dprintf(stderr, "[find_retrieve_metasfc] No matching");
			dprintf(stderr, " value calculation source type\n");
			dprintf(stderr, "   for source: \"%s %s\"",
					SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
			dprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
					SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
			dprintf(stderr, "   for crossref: \"%s\"",
					SafeStr(valcalc->vcalc->name));
			dprintf(stderr, "  of element: \"%s\"  level: \"%s\"\n",
					SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
			}
		}

	/* Return FALSE if no field, equation, or value calculation found */
	if ( DebugMode )
		{
		dprintf(stderr, "[find_retrieve_metasfc] No field");
		dprintf(stderr, " or equation found\n");
		dprintf(stderr, "   for source: \"%s %s\"",
				SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
		dprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
				SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
		dprintf(stderr, "   for element: \"%s\"  level: \"%s\"\n",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		}
	return FALSE;
	}

/**********************************************************************/

/*********************************************************************/
/** Check whether data for given default values can be extracted
 * from existing metafiles with a given attribute
 *
 * NOTE: This routine also checks for "matching" metafiles for daily
 *  or static fields!
 *
 *	@param[in]	*fdesc		field descriptor
 *	@param[in]	attrib		field attribute
 *  @return True if data can be extracted.
 *********************************************************************/
LOGICAL				find_retrieve_metasfc_by_attrib

	(
	FLD_DESCRIPT	*fdesc,
	STRING			attrib
	)

	{
	int								nn;
	STRING							metapath;
	FpaConfigFieldStruct			*fdef;
	FpaConfigElementAttribStruct	*attribs;

	/* Return FALSE if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->sdef) || IsNull(fdesc->subdef)
			|| IsNull(fdesc->edef) || IsNull(fdesc->ldef) ) return FALSE;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in find_retrieve_metasfc_by_attrib:\n");
		dprintf(stdout,"   path: %s\n",      SafeStr(fdesc->path));
		dprintf(stdout,"   source: %s %s\n", SafeStr(fdesc->sdef->name),
												SafeStr(fdesc->subdef->name));
		dprintf(stdout,"   rtime: %s\n",     SafeStr(fdesc->rtime));
		dprintf(stdout,"   vtime: %s\n",     SafeStr(fdesc->vtime));
		dprintf(stdout,"   element: %s\n",   SafeStr(fdesc->edef->name));
		dprintf(stdout,"   level: %s\n",     SafeStr(fdesc->ldef->name));
		dprintf(stdout,"   attrib: %s\n",    SafeStr(attrib));
		}

	/* Check for metafile containing data for field descriptor */
	metapath = find_meta_filename(fdesc);
	if ( !blank(metapath) )
		{

		/* Get detailed field information */
		fdef = get_field_info(fdesc->edef->name, fdesc->ldef->name);
		if ( NotNull(fdef) )
			{

			/* Check that field contains this attribute */
			attribs = fdef->element->elem_detail->attributes;
			for (nn=0; nn<attribs->nattribs; nn++)
				{
				if (same(attribs->attrib_names[nn], attrib)) return TRUE;
				}
			}
		}

	/* Return FALSE if no field or attribute */
	if ( DebugMode )
		{
		dprintf(stderr, "[find_retrieve_metasfc_by_attrib] No field");
		dprintf(stderr, " or attribute found\n");
		dprintf(stderr, "   for source: \"%s %s\"",
				SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
		dprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
				SafeStr(fdesc->rtime), SafeStr(fdesc->vtime));
		dprintf(stderr, "   for element: \"%s\"  level: \"%s\"",
				SafeStr(fdesc->edef->name), SafeStr(fdesc->ldef->name));
		dprintf(stderr, "  attrib: \"%s\"\n",
				SafeStr(attrib));
		}
	return FALSE;
	}

/**********************************************************************
 ***                                                                ***
 *** c h e c k _ c a l c u l a t e _ e q u a t i o n                ***
 ***                                                                ***
 **********************************************************************/

/*********************************************************************/
/** check whether equation string with given default values can
 * be calculated
 *
 *	@param[in]	*fdesc		field descriptor
 *	@param[in]	inunit		input units for equation
 *	@param[in]	inebuf		input equation for field
 * 	@return True if equation string can be calculated.
 *********************************************************************/
LOGICAL				check_calculate_equation

	(
	FLD_DESCRIPT	*fdesc,
	STRING			inunit,
	STRING			inebuf
	)

	{
	FpaEQUATION_DEFAULTS	OldEqtnDefs;

	/* Return FALSE if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->sdef) || IsNull(fdesc->subdef)
			|| IsNull(fdesc->edef) || IsNull(fdesc->ldef) ) return FALSE;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in check_calculate_equation:\n");
		dprintf(stdout,"   path: %s\n",      SafeStr(fdesc->path));
		dprintf(stdout,"   source: %s %s\n", SafeStr(fdesc->sdef->name),
												SafeStr(fdesc->subdef->name));
		dprintf(stdout,"   rtime: %s\n",     SafeStr(fdesc->rtime));
		dprintf(stdout,"   vtime: %s\n",     SafeStr(fdesc->vtime));
		dprintf(stdout,"   element: %s\n",   SafeStr(fdesc->edef->name));
		dprintf(stdout,"   level: %s\n",     SafeStr(fdesc->ldef->name));
		dprintf(stdout,"   units: %s\n",     SafeStr(inunit));
		dprintf(stdout,"   equation: %s\n",  SafeStr(inebuf));
		}

	/* Check the accuracy of input units and equation */
	if ( !valid_units_and_equation(inunit, inebuf) ) return FALSE;

	/* Ensure that equation handler is not trapped in an endless loop */
	if ( check_endless_loop(inebuf) )
		{
		(void) reset_endless_loop();
		return FALSE;
		}

	/* Store old Equation Defaults */
	(void) save_equation_defaults(&OldEqtnDefs);

	/* Reset default path name and source */
	(void) safe_strcpy(FpaEqtnDefs.path,      fdesc->path);
	(void) safe_strcpy(FpaEqtnDefs.source,    fdesc->sdef->name);
	(void) safe_strcpy(FpaEqtnDefs.subsource, fdesc->subdef->name);

	/* Reset default run and valid timestamps */
	(void) safe_strcpy(FpaEqtnDefs.rtime, fdesc->rtime);
	(void) safe_strcpy(FpaEqtnDefs.vtime, fdesc->vtime);

	/* Reset default levels based on enumerated level type */
	switch ( fdesc->ldef->lvl_type )
		{

		/* Reset all default levels from single pressure level */
		case FpaC_MSL:
		case FpaC_SURFACE:
		case FpaC_LEVEL:
		case FpaC_GEOGRAPHY:
		case FpaC_ANNOTATION:
			(void) safe_strcpy(FpaEqtnDefs.lvl,    fdesc->ldef->lev_lvls->lvl);
			(void) safe_strcpy(FpaEqtnDefs.uprlvl, fdesc->ldef->lev_lvls->lvl);
			(void) safe_strcpy(FpaEqtnDefs.lwrlvl, fdesc->ldef->lev_lvls->lvl);
			break;

		/* Reset default levels from upper and lower pressure levels */
		case FpaC_LAYER:
			(void) safe_strcpy(FpaEqtnDefs.lvl,    fdesc->ldef->lev_lvls->uprlvl);
			(void) safe_strcpy(FpaEqtnDefs.uprlvl, fdesc->ldef->lev_lvls->uprlvl);
			(void) safe_strcpy(FpaEqtnDefs.lwrlvl, fdesc->ldef->lev_lvls->lwrlvl);
			break;

		/* Do not reset levels for other cases */
		default:
			break;
		}

	/* Reset default map projection for evaluation */
	(void) copy_map_projection(&FpaEqtnDefs.mprojEval, &fdesc->mproj);

	/* Check the equation string */
	if ( !check_evaluate_equation(inebuf) )
		{
		if ( DebugMode )
			{
			dprintf(stderr, "[check_calculate_equation] Problem");
			dprintf(stderr, " checking equation: \"%s\"\n", SafeStr(inebuf));
			}
		(void) restore_equation_defaults(&OldEqtnDefs);
		(void) pop_endless_loop();
		return FALSE;
		}

	/* Finally, restore old Equation Defaults */
	/*  and return TRUE if all went well      */
	(void) restore_equation_defaults(&OldEqtnDefs);
	(void) pop_endless_loop();
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*   c h e c k e d _ v a l i d _ t i m e _ l i s t                      *
*   c h e c k e d _ v a l i d _ t i m e _ l i s t _ f r e e            *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Return valid timestamps for a given source and run time.
 *
 * NOTE: that the valid times are matched with element, level, or
 * valid time if these are set in the field descriptor!
 *
 * NOTE: that the function checks for calculated fields too!
 *
 * @param[in]	*fdesc		field descriptor
 * @param[in]	macro		enumerated time dependence to match
 * @param[out]	**list		list of valid times for given source and run time
 * @return The size of the list.
 *********************************************************************/
int					checked_valid_time_list

	(
	FLD_DESCRIPT	*fdesc,
	int				macro,
	STRING			**list
	)

	{
	int						ifl;
	float					clon;
	FLD_DESCRIPT			descript;
	STRING					srcdir;
	struct stat				statbuf;
	FpaConfigFieldStruct	*fdef;

	/* Variables to store valid time list */
	int		NumTimes  = 0;
	STRING	*TimeList = NullStringList;

	/* Internal variables to store list of all valid times */
	/*  for a given source and run time                    */
	static	int				AllTimes     = 0;
	static	STRING			*AllTimeList = NullStringList;
	static	FLD_DESCRIPT	AllFdesc     = FpaNO_FDESC;
	static	int				AllMacro     = FpaCnoMacro;
	static	time_t			AllModTime   = 0;

	/* Initialize return parameter */
	if ( NotNull(list) ) *list = NullStringList;

	/* Return immediately if no structure passed */
	if ( IsNull(fdesc) ) return NumTimes;

	/* Get detailed field information (if required) */
	if ( NotNull(fdesc->edef) && NotNull(fdesc->ldef) )
		{
		fdef = get_field_info(fdesc->edef->name, fdesc->ldef->name);
		if ( IsNull(fdef) ) return NumTimes;
		}
	else
		{
		fdef = NullPtr(FpaConfigFieldStruct *);
		}

	/* Return list of valid times for fields found in data base */
	(void) copy_fld_descript(&descript, fdesc);
	(void) set_fld_descript(&descript,
							FpaF_VALID_TIME, FpaCblank,
							FpaF_END_OF_LIST);
	NumTimes = source_valid_time_list(&descript, macro, &TimeList);
	if ( NumTimes > 0 )
		{
		if ( NotNull(list) )
			{
			*list = TimeList;
			}
		else
			{
			(void) source_valid_time_list_free(&TimeList, NumTimes);
			}
		return NumTimes;
		}

	/* Return if no field information */
	if ( IsNull(fdef) ) return NumTimes;

	/* Return immediately ... unless the field is a vector type field, */
	/*  or has an equation or value calculation                        */
	if ( fdef->element->fld_type != FpaC_VECTOR
			&& IsNull(fdef->element->elem_detail->equation)
			&& IsNull(fdef->element->elem_detail->valcalc) ) return NumTimes;
	if ( DebugMode && fdef->element->fld_type == FpaC_VECTOR )
		{
		dprintf(stderr, "[checked_valid_time_list] Checking for vector components\n");
		dprintf(stderr, "[checked_valid_time_list]   for field %s %s from %s\n",
			fdesc->edef->name, fdesc->ldef->name, fdesc->sdef->name);
		}
	else if ( DebugMode && IsNull(fdef->element->elem_detail->equation) )
		{
		dprintf(stderr, "[checked_valid_time_list] Checking equation evaluation\n");
		dprintf(stderr, "[checked_valid_time_list]   for field %s %s from %s\n",
			fdesc->edef->name, fdesc->ldef->name, fdesc->sdef->name);
		}
	else if ( DebugMode && IsNull(fdef->element->elem_detail->valcalc) )
		{
		dprintf(stderr, "[checked_valid_time_list] Checking value calculation\n");
		dprintf(stderr, "[checked_valid_time_list]   for field %s %s from %s\n",
			fdesc->edef->name, fdesc->ldef->name, fdesc->sdef->name);
		}

	/* Set field descriptor to set list of all valid times */
	/*  for given source and run time                      */
	(void) copy_fld_descript(&descript, fdesc);
	(void) set_fld_descript(&descript,
							FpaF_ELEMENT_NAME, FpaCblank,
							FpaF_LEVEL_NAME,   FpaCblank,
							FpaF_VALID_TIME,   FpaCblank,
							FpaF_END_OF_LIST);

	/* Set directory information for given source and run time */
	/*  with call to system subroutine  stat()                 */
	srcdir = source_directory(&descript);
	if ( blank(srcdir) || stat(srcdir, &statbuf) != 0 ) return NumTimes;

	/* Set list of all valid times based on time dependence */
	switch ( fdesc->edef->elem_tdep->time_dep )
		{

		/* Set list of all valid times for FpaC_NORMAL or FpaC_STATIC fields */
		case FpaC_NORMAL:
		case FpaC_STATIC:

			/* Reset list of all valid times if not already saved! */
			if ( !same_fld_descript(&descript, &AllFdesc)
					|| (FpaC_NORMAL | FpaC_STATIC) != AllMacro
					|| statbuf.st_mtime != AllModTime )
				{

				/* Save field descriptor, macro and modify time for next call */
				(void) copy_fld_descript(&AllFdesc, &descript);
				AllMacro   = (FpaC_NORMAL | FpaC_STATIC);
				AllModTime = statbuf.st_mtime;

				/* Free list of all valid times (if required) */
				AllTimes = source_valid_time_list_free(&AllTimeList, AllTimes);

				/* Set list of all valid times */
				AllTimes = source_valid_time_list(&AllFdesc, AllMacro,
																&AllTimeList);
				if ( AllTimes <= 0 ) return NumTimes;
				}

			/* List of all valid times now set and saved */
			break;

		/* Set list of valid times for FpaC_DAILY fields */
		case FpaC_DAILY:

			/* Reset list of all valid times if not already saved! */
			if ( !same_fld_descript(fdesc, &AllFdesc)
					|| FpaC_TIMEDEP_ANY != AllMacro
					|| statbuf.st_mtime != AllModTime )
				{

				/* Save field descriptor, macro and modify time for next call */
				(void) copy_fld_descript(&AllFdesc, fdesc);
				AllMacro   = FpaC_TIMEDEP_ANY;
				AllModTime = statbuf.st_mtime;

				/* Free list of all valid times (if required)       */
				/* Note that routine daily_field_local_times_free() */
				/*  calls routine source_valid_time_list_free()     */
				AllTimes = daily_field_local_times_free(&AllTimeList, AllTimes);

				/* Set center longitude for field (or for default map) */
				if ( !same_map_projection(&fdesc->mproj, &NoMapProj) )
					(void) grid_center(&fdesc->mproj, NullPointPtr, NullFloat,
							&clon);
				else
					(void) grid_center(get_target_map(), NullPointPtr,
							NullFloat, &clon);

				/* Set list of all valid times */
				AllTimes = daily_field_local_times(&AllFdesc, &descript,
												AllMacro, clon, &AllTimeList);
				if ( AllTimes <= 0 ) return NumTimes;
				}

			/* List of all valid times now set and saved */
			break;

		/* Error return for unrecognized time dependence */
		default:
			return NumTimes;
		}

	/* Build list of valid timestamps for calculated fields */
	for ( ifl=0; ifl<AllTimes; ifl++ )
		{

		/* Set the field descriptor for this valid time */
		(void) copy_fld_descript(&descript, fdesc);
		(void) set_fld_descript(&descript,
								FpaF_VALID_TIME, AllTimeList[ifl],
								FpaF_END_OF_LIST);

		/* Add valid timestamp to list if field can be calculated */
		/*  ... but only if the list is returned!                 */
		if ( check_retrieve_metasfc(&descript) )
			{
			NumTimes++;
			if ( NotNull(list) )
				{
				TimeList = GETMEM(TimeList, STRING, NumTimes);
				TimeList[NumTimes-1] = strdup(AllTimeList[ifl]);
				}
			}
		}

	/* Return the list of valid timestamps */
	if ( NotNull(list) ) *list = TimeList;
	return NumTimes;
	}

/**********************************************************************/

/*********************************************************************/
/** Free valid timestamps list.
 *
 *	@param[in]	**list		list of valid times
 *	@param[in]	num			number of valid times in list
 * 	@return The size of the list (0).
 *********************************************************************/
int					checked_valid_time_list_free

	(
	STRING			**list,
	int				num
	)

	{

	/* Free the list of valid times */
	FREELIST(*list, num);

	/* Reset the number of valid times and return it */
	num = 0;
	return num;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Evaluating Equation Strings)            *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/**********************************************************************
 ***                                                                ***
 *** e v a l u a t e _ e q u a t i o n                              ***
 ***                                                                ***
 *** return pointer to structure determined by parsing an equation  ***
 ***  into structures to evaluate, two structures and an operator,  ***
 ***  or a function applied to zero or more arguments               ***
 ***                                                                ***
 **********************************************************************/

static	FpaEQTN_DATA	*evaluate_equation

	(
	STRING			buf			/* string containing equation */
	)

	{
	char			xbuf[MAX_BCHRS];
	char			*pfop, *pnop;
	char			lbuf[MAX_BCHRS], rbuf[MAX_BCHRS];
	FpaEQTN_DATA	*plfld, *prfld, *pfield;

	/* Return Null pointer if no string to evaluate */
	if ( blank(buf) ) return NullPtr(FpaEQTN_DATA *);

	/* Expand equation by generic substitution of fields (if required) */
	if ( !generic_expansion(buf, xbuf) ) return NullPtr(FpaEQTN_DATA *);

	/* Initialize pointers to structures */
	plfld = NullPtr(FpaEQTN_DATA *);
	prfld = NullPtr(FpaEQTN_DATA *);

	/* Identify location of first operator */
	pfop = get_fop(xbuf, lbuf);

	/* Evaluate left side of equation ... except for function */
	/*  operators, where left side is function name           */
	if (*pfop != '[')
		{
		if ( DebugMode )
			{
			dprintf(stdout, "  Evaluation of:  >%s<\n", SafeStr(lbuf));
			}
		plfld = evaluate_name(lbuf);

		/* Return Null pointer if error in name evaluation */
		if ( !blank(lbuf) && IsNull(plfld) ) return NullPtr(FpaEQTN_DATA *);
		if ( DebugMode )
			{
			if ( NotNull(plfld) )
				{
				dprintf(stdout, "    Result Object type: %d", plfld->Type);
				dprintf(stdout, "   from:  >%s<\n", SafeStr(lbuf));
				dprintf(stdout, "         at: <%lx>\n", (unsigned long) plfld);
				}
			else
				{
				dprintf(stdout, "    Result Object type: 0");
				dprintf(stdout, "   from:  >%s<\n", SafeStr(lbuf));
				dprintf(stdout, "         at: <%lx>\n", (unsigned long) plfld);
				}
			}
		}

	/* Return now if no operator was found */
	if (*pfop == '\0') return plfld;

	/* If operators found, continue until no more found */
	do
		{
		/* Identify location of next operator */
		pnop = get_nop(pfop, xbuf, rbuf);

		/* Calculation for all operators except function operators */
		if (*pfop != '[')
			{
			prfld = evaluate_equation(rbuf);
			if ( DebugMode )
				{
				dprintf(stdout, "  Calculation:  <%lx>  %c", (unsigned long) plfld, *pfop);
				dprintf(stdout, "  <%lx>\n", (unsigned long) prfld);
				}
			pfield = evaluate_operator(plfld, pfop, prfld);

			/* Free space used by work structures */
			(void) free_eqtn_data(plfld);
			(void) free_eqtn_data(prfld);

			/* Return Null pointer if error in operator calculation */
			if ( IsNull(pfield) ) return NullPtr(FpaEQTN_DATA *);
			if ( DebugMode )
				{
				dprintf(stdout, "    Result Object type: %d",
						pfield->Type);
				dprintf(stdout, "   from:  >%s<  %c  >%s<\n",
						SafeStr(lbuf), *pfop, SafeStr(rbuf));
				dprintf(stdout, "         at: <%lx>\n", (unsigned long) pfield);
				}
			}

		/* Calculation for function operators */
		else
			{
			pfield = evaluate_function(lbuf, rbuf);

			/* Return Null pointer if error in function calculation */
			if ( IsNull(pfield) )
				{
				(void) free_eqtn_data(plfld);
				return NullPtr(FpaEQTN_DATA *);
				}
			if ( DebugMode )
				{
				dprintf(stdout, "    Result Object type: %d", pfield->Type);
				dprintf(stdout, "   from:  >%s<  of  >%s<\n",
						SafeStr(lbuf), SafeStr(rbuf));
				dprintf(stdout, "         at: <%lx>\n", (unsigned long) pfield);
				}
			}

		/* Go on to next operator ... until no more found */
		pfop = pnop;
		plfld = pfield;
		} while (*pfop != '\0');

	/* Return pointer to evaluated structure */
	return pfield;
	}

/**********************************************************************
 ***                                                                ***
 *** e v a l u a t e _ n a m e                                      ***
 ***                                                                ***
 *** return pointer to structure for evaluation of a single string  ***
 ***  name identifying a constant or field name                     ***
 ***                                                                ***
 **********************************************************************/

static	FpaEQTN_DATA	*evaluate_name

	(
	STRING			buf			/* string containing constant or field name */
	)

	{
	char			tbuf[MAX_BCHRS];
	FpaEQTN_DATA	*plmn, *pfld, *pfldmn;

	/* Return Null pointer if no string name to evaluate */
	if ( blank(buf) ) return NullPtr(FpaEQTN_DATA *);

	/* Check for unary + or - ... and return structure containing */
	/*  unary value if string name contains only unary operator   */
	(void) safe_strcpy(tbuf, buf);
	plmn = evaluate_unary(tbuf);
	if ( blank(tbuf) ) return plmn;

	/* Check for constant or named constant */
	pfld = evaluate_constant(tbuf);

	/* Check for field name if not a constant or named constant */
	if ( IsNull(pfld) ) pfld = evaluate_field_string(tbuf);

	/* Error message if string name cannot be evalutated */
	if ( IsNull(pfld) )
		{
		(void) fprintf(stderr, "[evaluate_name]  No named constant or");
		(void) fprintf(stderr, " valid field for: \"%s\"\n", SafeStr(tbuf));

		/* Error message if name is a badly formatted function name */
		if ( identify_function(tbuf) )
			{
			(void) fprintf(stderr, "[evaluate_name]  \"%s\" ", SafeStr(tbuf));
			(void) fprintf(stderr, " is a function name!!\n");
			(void) fprintf(stderr, "  It must be written  %s[ ... ]",
					SafeStr(tbuf));
			(void) fprintf(stderr, "  with function arguments within [ ]\n");
			}

		/* Free space used by unary structure */
		(void) free_eqtn_data(plmn);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Return pointer to evaluated structure if no unary + or - */
	if ( IsNull(plmn) )
		{
		return pfld;
		}

	/* Otherwise, multiply unary value and evaluated structure, */
	/*  free space used by work structures, and return pointer  */
	else
		{
		pfldmn = oper_mult(plmn, pfld);
		(void) free_eqtn_data(plmn);
		(void) free_eqtn_data(pfld);
		return pfldmn;
		}
	}

/**********************************************************************
 ***                                                                ***
 *** e v a l u a t e _ o p e r a t o r                              ***
 ***                                                                ***
 *** return pointer to structure calculated from two structures     ***
 ***  and an operator                                               ***
 ***                                                                ***
 **********************************************************************/

static	FpaEQTN_DATA	*evaluate_operator

	(
	FpaEQTN_DATA	*plfld,		/* pointer to structure to left of operator */
	char			*pop,		/* pointer to operator */
	FpaEQTN_DATA	*prfld		/* pointer to structure to right of operator */
	)

	{
	FpaEQTN_DATA	*pfield;

	if (*pop == '+')
		{
		pfield = oper_plus(plfld, prfld);
		}
	else if (*pop == '-')
		{
		pfield = oper_minus(plfld, prfld);
		}
	else if (*pop == '*')
		{
		pfield = oper_mult(plfld, prfld);
		}
	else if (*pop == '/')
		{
		pfield = oper_divn(plfld, prfld);
		}
	else if (*pop == '^')
		{
		pfield = oper_power(plfld, prfld);
		}
	else if (*pop == '(')
		{
		pfield = evaluate_bracket(plfld, prfld);
		}
	else
		{
	/* Error message for unrecognizable operator */
		(void) fprintf(stderr, "[evaluate_operator]");
		(void) fprintf(stderr, " Unrecognizable operator: %c\n", *pop);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Return pointer to evaluated structure */
	return pfield;
	}

/**********************************************************************
 ***                                                                ***
 *** e v a l u a t e _ b r a c k e t                                ***
 ***                                                                ***
 *** return pointer to second structure in brackets (if first       ***
 ***  structure is Null) or to first structure multiplied by second ***
 ***  structure (if first structure is unary + or -)                ***
 *** Note that first structure must be Null or unary + or -         ***
 ***                                                                ***
 **********************************************************************/

static	FpaEQTN_DATA	*evaluate_bracket

	(
	FpaEQTN_DATA	*plfld,		/* pointer to first structure */
	FpaEQTN_DATA	*prfld		/* pointer to second structure */
	)

	{
	FpaEQTN_DATA	*pfield;

	/* Return Null if missing structure to evaluate */
	if ( IsNull(prfld) ) return NullPtr(FpaEQTN_DATA *);

	/* Error message for non-unary structure to left of brackets */
	if ( plfld )
		{
		if ( plfld->Type != FpaEQT_Scalar
				|| (plfld->Data.scalr.sval != 1.0
					&& plfld->Data.scalr.sval != -1.0) )
			{
			(void) fprintf(stderr, "[evaluate_bracket] Non unary structure to");
			(void) fprintf(stderr, " left of (\n");
			(void) fprintf(stderr, "  Object type: %d\n", plfld->Type);
			(void) fprintf(stderr, "     at: <%lx>\n", (unsigned long) plfld);
			(void) debug_eqtn_data(plfld);
			return NullPtr(FpaEQTN_DATA *);
			}
		}

	/* Make copy of structure within brackets if no unary + or - */
	if ( IsNull(plfld) )
		pfield = copy_eqtn_data(prfld);

	/* Otherwise, multiply unary value and structure within brackets */
	else
		pfield = oper_mult(plfld, prfld);

	/* Return pointer to calculated structure */
	return pfield;
	}

/**********************************************************************
 ***                                                                ***
 *** e v a l u a t e _ f u n c t i o n                              ***
 ***                                                                ***
 *** return pointer to structure calculated from a function applied ***
 ***  to zero or more arguments                                     ***
 *** Note that a maximum of FpaEQFF_MaxFlds (5) arguments are       ***
 ***  allowed.  This is hard-coded in  (*pfunc->Enam) function      ***
 ***  call, and in function  evaluate_unix_function                 ***
 ***                                                                ***
 **********************************************************************/

static	FpaEQTN_DATA	*evaluate_function

	(
	STRING			lbuf,		/* string containing function name */
	STRING			rbuf		/* string containing function arguments */
	)

	{
	int					jj, jargs, jflds, ptimes[FpaEQFF_MaxFlds];
	char				tlbuf[MAX_BCHRS];
	const FpaEQTN_FUNC	*pfunc;
	FpaEQTN_DATA		*plmn, *pfflds[FpaEQFF_MaxFlds], *pfield, *pfieldmn;

	/* Error message if no function name */
	if ( blank(lbuf) )
		{
		(void) fprintf(stderr, "[evaluate_function] No function name left of [\n");
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Initialize pointers to function argument structures */
	for ( jj=0; jj<FpaEQFF_MaxFlds; jj++ ) pfflds[jj] = NullPtr(FpaEQTN_DATA *);

	/* Check for unary + or - before function name */
	(void) safe_strcpy(tlbuf, lbuf);
	plmn = evaluate_unary(tlbuf);

	/* Error message if function name contains only unary + or - */
	if ( blank(tlbuf) )
		{
		(void) fprintf(stderr, "[evaluate_function] Only unary operator");
		(void) fprintf(stderr, " to left of [\n");
		/* Free space used by unary structure */
		(void) free_eqtn_data(plmn);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Identify function by name */
	pfunc = identify_function(tlbuf);

	/* Error message if function name not found */
	if ( IsNull(pfunc) )
		{
		(void) fprintf(stderr, "[evaluate_function] Unrecognizable");
		(void) fprintf(stderr, " function name: \"%s\"\n", SafeStr(tlbuf));
		/* Free space used by unary structure */
		(void) free_eqtn_data(plmn);
		return NullPtr(FpaEQTN_DATA *);
		}

	if ( DebugMode )
		{
		dprintf(stdout, "  Argument evaluation for");
		dprintf(stdout, " function:  %s[]\n", SafeStr(tlbuf));
		}

	/* Branch to evaluations based on function type */
	switch ( pfunc->Ftype )
		{

		/* Evaluate function argument structures for UNIX */
		/*  functions and ordinary non-UNIX functions     */
		case FpaEQF_UNIX:
		case FpaEQF_ORDINARY:
			jargs = evaluate_function_arguments(rbuf, pfunc->PointEvalOK,
					pfflds);
			jflds = 0;
			break;

		/* Evaluate function argument structures for */
		/*  timeseries non-UNIX functions            */
		case FpaEQF_TIMESERIES:
			jargs = evaluate_function_timeseries(rbuf, pfunc->PointEvalOK,
					&jflds, ptimes, pfflds);
			break;

		/* Error message for unacceptable function type */
		default:
			(void) fprintf(stderr, "[evaluate_function]");
			(void) fprintf(stderr, " Unacceptable function type: %d", pfunc->Ftype);
			(void) fprintf(stderr, "   for function: \"%s\"\n", SafeStr(tlbuf));
			/* Free space used by unary structure */
			(void) free_eqtn_data(plmn);
			return NullPtr(FpaEQTN_DATA *);
		}

	/* Debug output for function argument structures */
	if ( DebugMode )
		{
		dprintf(stdout, "  Function:  %s[]  of ...\n", SafeStr(tlbuf));
		if ( jargs>0 )
			{
			for ( jj=0; jj<jargs; jj++ )
				{
				dprintf(stdout, "    Structure  (%d)", jj);
				dprintf(stdout, "   at: <%lx>\n", (unsigned long) pfflds[jj]);
				}
			}
		}

	/* Error message if incorrect number of arguments */
	if ( (pfunc->Fargs != jargs) )
		{
		(void) fprintf(stderr, "[evaluate_function] Incorrect number");
		(void) fprintf(stderr, " of argument structures for");
		(void) fprintf(stderr, " function: \"%s\"\n", SafeStr(tlbuf));
		(void) fprintf(stderr, "  Structures passed: %d", jargs);
		(void) fprintf(stderr, "   (Maximum: %d)", FpaEQFF_MaxFlds);
		(void) fprintf(stderr, "   Required: %d\n", pfunc->Fargs);
		/* Free space used by work structures */
		(void) free_eqtn_data(plmn);
		for ( jj=0; jj<FpaEQFF_MaxFlds; jj++ )
			(void) free_eqtn_data(pfflds[jj]);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Branch to evaluations based on function type */
	switch ( pfunc->Ftype )
		{

		/* Evaluate UNIX functions */
		case FpaEQF_UNIX:
			pfield = evaluate_unix_function(pfunc->Name, pfunc->Fargs,
					pfunc->Unam, pfflds);
			break;

		/* Evaluate ordinary non-UNIX functions by function call */
		/*  passing pointers to each argument structure          */
		case FpaEQF_ORDINARY:
			pfield = (*pfunc->Enam) (pfflds[0], pfflds[1], pfflds[2],
					pfflds[3], pfflds[4]);
			break;

		/* Evaluate timeseries non-UNIX functions by function    */
		/*  call passing pointer to all structures in timeseries */
		case FpaEQF_TIMESERIES:
			pfield = (*pfunc->Enam) (jflds, ptimes, pfflds);
			break;

		/* Error message for unacceptable function type */
		default:
			(void) fprintf(stderr, "[evaluate_function]");
			(void) fprintf(stderr, " Unacceptable function type: %d", pfunc->Ftype);
			(void) fprintf(stderr, "   for function: \"%s\"\n", SafeStr(tlbuf));
			/* Free space used by work structures */
			(void) free_eqtn_data(plmn);
			for ( jj=0; jj<FpaEQFF_MaxFlds; jj++ )
				(void) free_eqtn_data(pfflds[jj]);
			return NullPtr(FpaEQTN_DATA *);
		}

	/* Free space used by function argument structures */
	for ( jj=0; jj<FpaEQFF_MaxFlds; jj++ )
		(void) free_eqtn_data(pfflds[jj]);

	/* Return pointer to evaluated structure if no unary + or - */
	if ( IsNull(plmn) )
		{
		return pfield;
		}

	/* Otherwise, multiply unary value and evaluated */
	/*  structure, free space used by structures,    */
	/*  and return pointer                           */
	else
		{
		pfieldmn = oper_mult(plmn, pfield);
		(void) free_eqtn_data(plmn);
		(void) free_eqtn_data(pfield);
		return pfieldmn;
		}
}

/**********************************************************************
 ***                                                                ***
 *** e v a l u a t e _ f u n c t i o n _ a r g u m e n t s          ***
 ***                                                                ***
 *** return pointer to structures determined by parsing a string    ***
 ***  containing zero or more function arguments                    ***
 *** Note that a maximum of FpaEQFF_MaxFlds (5) arguments are       ***
 ***  allowed                                                       ***
 ***                                                                ***
 **********************************************************************/

static	int				evaluate_function_arguments

	(
	STRING			rbuf,		/* string containing function arguments */
	const LOGICAL	pointOK[],	/* flags for point evaluation of arguments */
	FpaEQTN_DATA	*pfflds[]	/* pointers to function argument structures */
	)

	{
	FpaEQUATION_DEFAULTS	OldEqtnDefs;
	int						jargs;
	char					*pbgn, *pnsub;
	char					rbufl[MAX_BCHRS];

	/* Return now if no function arguments to evaluate */
	if ( blank(rbuf) ) return 0;

	/* Evaluate function argument structures */
	for ( pbgn=&rbuf[0], jargs=0; *pbgn; jargs++ )
		{

		/* Error message if too many arguments */
		if ( jargs >= FpaEQFF_MaxFlds )
			{
			(void) fprintf(stderr, "[evaluate_function_arguments] More than");
			(void) fprintf(stderr, " max number (%d) of arguments", FpaEQFF_MaxFlds);
			(void) fprintf(stderr, "   from: \"%s\"\n", SafeStr(rbuf));
			return 0;
			}

		/* Store old Equation Defaults */
		(void) save_equation_defaults(&OldEqtnDefs);

		/* Reset flag for point evaluation of argument structure */
		if ( FpaEqtnDefs.pointeval) FpaEqtnDefs.pointeval = pointOK[jargs];

		/* Identify argument and evaluate argument structure */
		pnsub = get_nsub(pbgn, rbuf, rbufl);
		pfflds[jargs] = evaluate_equation(rbufl);

		/* Error message if error in argument evaluation */
		if ( IsNull(pfflds[jargs]) )
			{
			(void) fprintf(stderr, "[evaluate_function_arguments]");
			(void) fprintf(stderr, " Unacceptable argument (%d)", jargs+1);
			(void) fprintf(stderr, "   from: \"%s\"\n", SafeStr(rbuf));
			return 0;
			}

		/* Set pointer to next argument */
		pbgn = pnsub;

		/* Restore old Equation Defaults */
		(void) restore_equation_defaults(&OldEqtnDefs);
		}

	/* Return number of function argument structures */
	return jargs;
	}

/**********************************************************************
 ***                                                                ***
 *** e v a l u a t e _ f u n c t i o n _ t i m e s e r i e s        ***
 ***                                                                ***
 *** return pointer to timeseries of structures determined by       ***
 ***  evaluating an argument string at a series of valid times      ***
 *** Note that a maximum of FpaEQFF_MaxFlds (5) timeseries          ***
 ***  structures are allowed and that a minimum of                  ***
 ***  FpaEQFF_MinFlds (2) timeseries structures are required        ***
 ***                                                                ***
 **********************************************************************/

static	int				evaluate_function_timeseries

	(
	STRING			rbuf,		/* string containing function arguments */
	const LOGICAL	pointOK[],	/* flags for point evaluation of arguments */
	int				*jflds,		/* pointer to number of structures in */
								/*  timeseries                        */
	int				ptimes[],	/* pointer to times in timeseries */
	FpaEQTN_DATA	*pfflds[]	/* pointers to structures in timeseries */
	)

	{
	FpaEQUATION_DEFAULTS	OldEqtnDefs;
	LOGICAL					local, status, *usevts;
	int						jargs, macro, nn, maxvts, ncheck, nfound, nvts;
	char					*pbgn, *pnsub;
	char					rbufl[MAX_BCHRS];
	FLD_DESCRIPT			descript;
	STRING					*vtlist;

	/* Initialize number of structures in timeseries */
	*jflds = 0;

	/* Return now if no function arguments to evaluate */
	if ( blank(rbuf) ) return 0;

	/* Ensure that only one function argument exists */
	pbgn = &rbuf[0];
	pnsub = get_nsub(pbgn, rbuf, rbufl);
	if (*pnsub != '\0')
		{
		(void) fprintf(stderr, "[evaluate_function_timeseries]");
		(void) fprintf(stderr, " More than one argument in: \"%s\"\n",
				SafeStr(rbuf));
		return 0;
		}
	jargs = 1;

	/* Initialize a field descriptor using default field values */
	(void) init_fld_descript(&descript);
	(void) set_fld_descript(&descript,
							FpaF_DIRECTORY_PATH, FpaEqtnDefs.path,
							FpaF_SOURCE_NAME,    FpaEqtnDefs.source,
							FpaF_SUBSOURCE_NAME, FpaEqtnDefs.subsource,
							FpaF_RUN_TIME,       FpaEqtnDefs.rtime,
							FpaF_END_OF_LIST);

	/* Set the type of fields from the valid time (normal or local) */
	(void) parse_tstamp(FpaEqtnDefs.vtime,
							NullInt, NullInt, NullInt, NullInt, &local,
							NullLogicalPtr);
	if ( local ) macro = FpaC_DAILY;
	else         macro = (FpaC_NORMAL | FpaC_STATIC);

	/* Determine maximum number of valid times available */
	maxvts = source_valid_time_list(&descript, macro, NullPtr(STRING **));
	if ( maxvts < 1 )
		{
		if ( local )
			(void) fprintf(stderr, "[evaluate_function_timeseries] No daily fields");
		else
			(void) fprintf(stderr, "[evaluate_function_timeseries] No normal fields");
		(void) fprintf(stderr, " for source: \"%s %s\"",
				SafeStr(FpaEqtnDefs.source), SafeStr(FpaEqtnDefs.subsource));
		(void) fprintf(stderr, "   rtime: \"%s\"\n",
				SafeStr(FpaEqtnDefs.rtime));
		return 0;
		}

	/* Determine series of valid times around default valid time       */
	/* Note that the loop increments the range of valid times to check */
	/*  until the the required number of fields is found               */
	ncheck = FpaEQFF_MaxFlds;
	while ( TRUE )
		{

		/* Debug output for timeseries evaluation */
		if ( DebugMode )
			{
			dprintf(stderr, "[evaluate_function_timeseries]");
			dprintf(stderr, " Checking  %d  times for field \"%s\"\n",
					ncheck, SafeStr(rbuf));
			}

		/* Determine series of ncheck valid times around default valid time */
		nvts = source_valid_time_sublist(&descript, macro, ncheck,
				FpaCblank, FpaEqtnDefs.vtime, FpaCblank, &vtlist);
		usevts = INITMEM(LOGICAL, nvts);

		/* Determine number of fields found for ncheck valid times */
		for ( nfound=0, nn=0; nn<nvts; nn++ )
			{

			/* Store old Equation Defaults */
			(void) save_equation_defaults(&OldEqtnDefs);

			/* Reset default valid time for evaluation of structure */
			(void) safe_strcpy(FpaEqtnDefs.vtime, vtlist[nn]);

			/* Evaluate timeseries structure based on new valid time */
			if ( check_evaluate_equation(rbuf) )
				{
				nfound++;
				usevts[nn] = TRUE;
				}
			else
				{
				usevts[nn] = FALSE;
				}

			/* Restore old Equation Defaults */
			(void) restore_equation_defaults(&OldEqtnDefs);
			}

		/* Debug output for timeseries evaluation */
		if ( DebugMode )
			{
			dprintf(stderr, "[evaluate_function_timeseries]");
			dprintf(stderr, "  %d  fields for \"%s\" contain required data\n",
					nfound, SafeStr(rbuf));
			}

		/* Check if required number of fields found */
		if ( nfound >= FpaEQFF_MaxFlds ) break;

		/* Check if maximum number of valid times have been checked */
		if ( ncheck >= maxvts ) break;

		/* Free valid timestamps and check a larger list of valid times */
		nvts = source_valid_time_sublist_free(&vtlist, nvts);
		FREEMEM(usevts);
		ncheck++;
		}

	/* Evaluate structures at each valid time                             */
	/* Note that valid times and fields are saved only if a field exists! */
	for ( nn=0; nn<nvts; nn++ )
		{

		/* Check for valid fields only */
		if ( !usevts[nn] ) continue;

		/* Store old Equation Defaults */
		(void) save_equation_defaults(&OldEqtnDefs);

		/* Set time wrt default valid time for this structure */
		ptimes[*jflds] = calc_prog_time_minutes(FpaEqtnDefs.vtime,
														vtlist[nn], &status);

		/* Reset default valid time for evaluation of structure */
		(void) safe_strcpy(FpaEqtnDefs.vtime, vtlist[nn]);

		/* Reset flag for point evaluation of structure */
		if ( FpaEqtnDefs.pointeval) FpaEqtnDefs.pointeval = pointOK[jargs-1];

		/* Evaluate timeseries structure based on new valid time */
		pfflds[*jflds] = evaluate_equation(rbuf);

		/* Increment number of structures in timeseries if the field exists! */
		/* Note that data for missing fields will be overwritten!            */
		if ( NotNull(pfflds[*jflds]) ) (*jflds)++;

		/* Restore old Equation Defaults */
		(void) restore_equation_defaults(&OldEqtnDefs);
		}

	/* Ensure that enough timeseries structures have been found */
	if ( *jflds < FpaEQFF_MinFlds )
		{
		(void) fprintf(stderr, "[evaluate_function_timeseries]");
		(void) fprintf(stderr, " Not enough timeseries structures: %d", *jflds);
		(void) fprintf(stderr, "   (Min required: %d)\n", FpaEQFF_MinFlds);
		nvts = source_valid_time_sublist_free(&vtlist, nvts);
		FREEMEM(usevts);
		return 0;
		}

	/* Free valid timestamps and return number of */
	/*  function argument structures (set to 1)   */
	nvts = source_valid_time_sublist_free(&vtlist, nvts);
	FREEMEM(usevts);
	return jargs;
	}

/**********************************************************************
 ***                                                                ***
 *** e v a l u a t e _ u n i x _ f u n c t i o n                    ***
 ***                                                                ***
 *** return pointer to structure calculated from a UNIX function    ***
 ***  applied to zero or more argument structures                   ***
 *** Note that a maximum of FpaEQFF_MaxFlds (5) argument structures ***
 ***  are allowed.  This is hard-coded in  (*unam)  function call   ***
 ***                                                                ***
 **********************************************************************/

static	FpaEQTN_DATA	*evaluate_unix_function

	(
	const STRING	name,		/* string containing UNIX function name */
	const short		fargs,		/* number of function arguments */
	const FpaUNAM	unam,		/* pointer to UNIX function name */
	FpaEQTN_DATA	*pfflds[]	/* pointers to function argument structures */
	)

	{
	LOGICAL			allscalar, anygrid, anyspline, anyvlist;
	int				jj, nn, iix, iiy;
	double			vals[FpaEQFF_MaxFlds];
	FpaEQTN_DATA	*ptemp[FpaEQFF_MaxFlds], *ptempx, *pfield;

	/* Error message if too many function arguments */
	if ( fargs > FpaEQFF_MaxFlds )
		{
		(void) fprintf(stderr, "[evaluate_unix_function] Too many function");
		(void) fprintf(stderr, " arguments for UNIX function: \"%s\"\n",
				SafeStr(name));
		(void) fprintf(stderr, "  Arguments passed: %d", fargs);
		(void) fprintf(stderr, "   (Maximum: %d)\n", FpaEQFF_MaxFlds);
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Determine Objects in argument structures that are not SCALAR */
	allscalar = TRUE;
	anygrid = anyspline = anyvlist = FALSE;
	for ( jj=0; jj<fargs; jj++ )
		{
		if ( pfflds[jj]->Type == FpaEQT_Scalar ) continue;
		else if ( pfflds[jj]->Type == FpaEQT_Grid )
			{
			allscalar = FALSE;
			anygrid   = TRUE;
			}
		else if ( pfflds[jj]->Type == FpaEQT_Spline )
			{
			allscalar = FALSE;
			anyspline = TRUE;
			}
		else if ( pfflds[jj]->Type == FpaEQT_Vlist )
			{
			allscalar = FALSE;
			anyvlist  = TRUE;
			}

		/* Error message if any Objects are of unacceptable type */
		else
			{
			(void) fprintf(stderr, "[evaluate_unix_function] Unacceptable");
			(void) fprintf(stderr, " type of Object for function: \"%s\"\n",
					SafeStr(name));
			(void) fprintf(stderr, "  Structure pfflds[%d]", jj);
			(void) fprintf(stderr, "    Object type: %d\n", pfflds[jj]->Type);
			return NullPtr(FpaEQTN_DATA *);
			}
		}

	/* Error message if any VLIST Objects are mixed with GRID or SPLINE */
	if ( anyvlist && (anygrid || anyspline) )
		{
		(void) fprintf(stderr, "[evaluate_unix_function] Error mixing");
		(void) fprintf(stderr, " VLIST Objects with GRID or SPLINE Objects\n");
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Calculate UNIX function directly if all Objects are SCALAR */
	if ( allscalar )
		{

		/* Initialize structure from first argument structure */
		pfield = copy_eqtn_data(pfflds[0]);

		/* Set values from each SCALAR Object */
		for ( jj=0; jj<fargs; jj++ )
			vals[jj] = (double) pfflds[jj]->Data.scalr.sval;

		/* Overwrite structure data with UNIX function value */
		if ( fargs == 0 )
			pfield->Data.scalr.sval = (float) (*unam)
					();
		else if ( fargs == 1 )
			pfield->Data.scalr.sval = (float) (*unam)
					(vals[0]);
		else if ( fargs == 2 )
			pfield->Data.scalr.sval = (float) (*unam)
					(vals[0], vals[1]);
		else if ( fargs == 3 )
			pfield->Data.scalr.sval = (float) (*unam)
					(vals[0], vals[1], vals[2]);
		else if ( fargs == 4 )
			pfield->Data.scalr.sval = (float) (*unam)
					(vals[0], vals[1], vals[2], vals[3]);
		else if ( fargs == 5 )
			pfield->Data.scalr.sval = (float) (*unam)
					(vals[0], vals[1], vals[2], vals[3], vals[4]);
		}

	/* Calculate UNIX function by looping on VLIST points if any */
	/*  Objects are VLIST Objects                                */
	else if ( anyvlist )
		{

		/* Compare point lists of all VLIST argument structures */
		for ( pfield=NullPtr(FpaEQTN_DATA *), jj=0; jj<fargs; jj++ )
			{
			if ( pfflds[jj]->Type == FpaEQT_Vlist )
				{

				/* Initialize structure from first VLIST */
				/*  argument structure                   */
				if ( IsNull(pfield) ) pfield = copy_eqtn_data(pfflds[jj]);

				/* Compare point lists of all other VLIST */
				/*  argument structures                   */
				else if ( !same_vlist_points(&pfield->Data.vlist,
								&pfflds[jj]->Data.vlist) )
					{
					(void) fprintf(stderr, "[evaluate_unix_function] Points in");
					(void) fprintf(stderr, " VLIST Objects do not match\n");
					(void) free_eqtn_data(pfield);
					return NullPtr(FpaEQTN_DATA *);
					}
				}
			}

		/* Loop on all points in VLIST Object */
		for ( nn=0; nn< pfield->Data.vlist.numpts; nn++ )
			{

			/* Set values from each VLIST or SCALAR Object */
			for ( jj=0; jj<fargs; jj++ )
				{
				if ( pfflds[jj]->Type == FpaEQT_Vlist )
					vals[jj] = (double) pfflds[jj]->Data.vlist.val[nn];
				else
					vals[jj] = (double) pfflds[jj]->Data.scalr.sval;
				}

			/* Overwrite structure data with UNIX function value */
			if ( fargs == 0 )
				pfield->Data.vlist.val[nn] = (float) (*unam)
						();
			else if ( fargs == 1 )
				pfield->Data.vlist.val[nn] = (float) (*unam)
						(vals[0]);
			else if ( fargs == 2 )
				pfield->Data.vlist.val[nn] = (float) (*unam)
						(vals[0], vals[1]);
			else if ( fargs == 3 )
				pfield->Data.vlist.val[nn] = (float) (*unam)
						(vals[0], vals[1], vals[2]);
			else if ( fargs == 4 )
				pfield->Data.vlist.val[nn] = (float) (*unam)
						(vals[0], vals[1], vals[2], vals[3]);
			else if ( fargs == 5 )
				pfield->Data.vlist.val[nn] = (float) (*unam)
						(vals[0], vals[1], vals[2], vals[3], vals[4]);
			}
		}

	/* Calculate UNIX function by looping on grid values if any */
	/*  Objects are GRID or SPLINE Objects                      */
	else
		{

		/* Initialize pointers to work structures */
		for ( jj=0; jj<fargs; jj++ ) ptemp[jj] = NullPtr(FpaEQTN_DATA *);

		/* Convert all Objects to GRID work Objects at the default */
		/*  grid dimensions                                        */
		for ( jj=0; jj<fargs; jj++ )
			{

			if ( pfflds[jj]->Type == FpaEQT_Scalar )
				{
				ptemp[jj] = convert_eqtn_data(FpaEQT_Grid, pfflds[jj]);
				}

			else if ( pfflds[jj]->Type == FpaEQT_Grid )
				{
				/* Ensure that GRID Object has default grid dimensions */
				if ( pfflds[jj]->Data.gridd.nx
										!= FpaEqtnDefs.mprojEval.grid.nx
						|| pfflds[jj]->Data.gridd.ny
										!= FpaEqtnDefs.mprojEval.grid.ny
						|| pfflds[jj]->Data.gridd.gridlen
										!= FpaEqtnDefs.mprojEval.grid.gridlen )
					{
					(void) fprintf(stderr, "[evaluate_unix_function] Grid");
					(void) fprintf(stderr, " dimensions do not match default\n");
					/* Convert GRID Object to default grid dimensions */
					ptempx = convert_eqtn_data(FpaEQT_Spline, pfflds[jj]);
					ptemp[jj] = convert_eqtn_data(FpaEQT_Grid, ptempx);
					(void) free_eqtn_data(ptempx);
					}

				else
					{
					ptemp[jj] = copy_eqtn_data(pfflds[jj]);
					}
				}

			else if ( pfflds[jj]->Type == FpaEQT_Spline )
				{
				ptemp[jj] = convert_eqtn_data(FpaEQT_Grid, pfflds[jj]);
				}
			}

		/* Initialize structure from first argument structure */
		pfield = copy_eqtn_data(ptemp[0]);

		/* Loop on all grid points */
		for ( iiy=0; iiy<pfield->Data.gridd.ny; iiy++ )
			for ( iix=0; iix<pfield->Data.gridd.nx; iix++ )
				{

				/* Set values from each GRID work Object */
				for ( jj=0; jj<fargs; jj++ )
					vals[jj] = (double) ptemp[jj]->Data.gridd.gval[iiy][iix];

				/* Overwrite structure data with UNIX function value */
				/*  at each grid point                               */
				if ( fargs == 0 )
					pfield->Data.gridd.gval[iiy][iix] = (float) (*unam)
							();
				else if ( fargs == 1 )
					pfield->Data.gridd.gval[iiy][iix] = (float) (*unam)
							(vals[0]);
				else if ( fargs == 2 )
					pfield->Data.gridd.gval[iiy][iix] = (float) (*unam)
							(vals[0], vals[1]);
				else if ( fargs == 3 )
					pfield->Data.gridd.gval[iiy][iix] = (float) (*unam)
							(vals[0], vals[1], vals[2]);
				else if ( fargs == 4 )
					pfield->Data.gridd.gval[iiy][iix] = (float) (*unam)
							(vals[0], vals[1], vals[2], vals[3]);
				else if ( fargs == 5 )
					pfield->Data.gridd.gval[iiy][iix] = (float) (*unam)
							(vals[0], vals[1], vals[2], vals[3], vals[4]);
				}

		/* Free space used by work structures */
		for ( jj=0; jj<fargs; jj++ ) (void) free_eqtn_data(ptemp[jj]);
		}

	/* Return pointer to structure containing calculations */
	return pfield;
	}

/**********************************************************************
 ***                                                                ***
 *** e v a l u a t e _ u n a r y                                    ***
 ***                                                                ***
 *** return pointer to structure for evaluation of unary + or -     ***
 ***  at beginning of a string                                      ***
 ***                                                                ***
 **********************************************************************/

static	FpaEQTN_DATA	*evaluate_unary

	(
	STRING			buf			/* string which may contain unary + or - */
	)

	{
	FpaEQTN_DATA	*plmn;

	/* Return Null pointer if no string to evaluate */
	if ( blank(buf) ) return NullPtr(FpaEQTN_DATA *);

	/* Return Null pointer if beginning of string is not unary + or - */
	if ( !(buf[0] == '+' || buf[0] == '-') ) return NullPtr(FpaEQTN_DATA *);

	/* Create structure to hold unary + or - evaluation */
	plmn = init_eqtn_data(FpaEQT_Scalar);

	/* Fill structure with unary + or - information */
	plmn->Type = FpaEQT_Scalar;
	if ( buf[0] == '+' )
		plmn->Data.scalr.sval = 1.0;
	else if ( buf[0] == '-' )
		plmn->Data.scalr.sval = -1.0;

	/* Remove unary + or - from beginning of string */
	memmove(buf, buf+1, strlen(buf));

	/* Return pointer to evaluated structure */
	return plmn;
	}

/**********************************************************************
 ***                                                                ***
 *** e v a l u a t e _ c o n s t a n t                              ***
 ***                                                                ***
 *** return pointer to structure for evaluation of a constant       ***
 ***  string or named constant                                      ***
 ***                                                                ***
 **********************************************************************/

static	FpaEQTN_DATA	*evaluate_constant

	(
	STRING			buf			/* string which may contain constant string */
								/*  or named constant                       */
	)

	{
	double					value;
	char					*endbuf;
	FpaConfigConstantStruct	*cdef;
	FpaEQTN_DATA			*pfld;

	/* Return Null pointer if no string to evaluate */
	if ( blank(buf) ) return NullPtr(FpaEQTN_DATA *);

	/* Check for constant strings */
	value = (double) strtod(buf, &endbuf);
	if ( endbuf == (buf + strlen(buf)) )
		{
		/* Return value of constant string in a SCALAR Object */
		pfld = init_eqtn_data(FpaEQT_Scalar);
		pfld->Data.scalr.sval = value;
		return pfld;
		}

	/* Check for named constants */
	cdef = identify_constant(buf);
	if ( NotNull(cdef) )
		{
		/* Return value of named constant (converted to MKS units) */
		/*  in a SCALAR Object                                     */
		if ( convert_value(cdef->units->name, cdef->value,
				cdef->units->MKS, &value) )
			{
			pfld = init_eqtn_data(FpaEQT_Scalar);
			pfld->Data.scalr.sval = value;
			return pfld;
			}
		}

	/* Return Null pointer if string not a constant or named constant */
	return NullPtr(FpaEQTN_DATA *);
	}

/**********************************************************************
 ***                                                                ***
 *** e v a l u a t e _ f i e l d _ s t r i n g                      ***
 ***                                                                ***
 *** return pointer to structure for evaluation of a named field    ***
 ***  with modified parameters                                      ***
 ***                                                                ***
 **********************************************************************/

static	FpaEQTN_DATA	*evaluate_field_string

	(
	STRING			buf			/* string which may contain field name */
								/*  and field modifiers                */
	)

	{
	FpaEQUATION_DEFAULTS	OldEqtnDefs;
	char					*pbgn, *pnmod;
	char					lvl[MAX_NCHRS];
	char					uprlvl[MAX_NCHRS], lwrlvl[MAX_NCHRS];
	char					source[MAX_NCHRS], subsource[MAX_NCHRS];
	char					rtime[MAX_NCHRS], vtime[MAX_NCHRS];
	char					attrib[MAX_BCHRS], aunits[MAX_BCHRS];
	char					xlookup[MAX_BCHRS], punits[MAX_BCHRS];
	float					defval, proximity;
	double					dval;
	char					fldbuf[MAX_BCHRS];
	char					modbuf[MAX_BCHRS], modbufl[MAX_BCHRS];
	FpaConfigElementStruct	*edef;
	FpaConfigLevelStruct	*ldef;
	FpaConfigFieldStruct	*fdef;
	LOGICAL					valid, validupr, validlwr;
	FLD_DESCRIPT			descript;
	VLIST					*vlist;
	SURFACE					sfc;
	FpaEQTN_DATA			*pfld;

	/* Return Null pointer if no string to evaluate */
	if ( blank(buf) ) return NullPtr(FpaEQTN_DATA *);

	/* Initialize field modifiers from default values */
	(void) init_fieldmod(lvl, uprlvl, lwrlvl, source, subsource,
			rtime, vtime, attrib, aunits, xlookup, &defval, &proximity, punits);

	/* Divide field string into field name and field modifiers */
	if ( !get_modstrng(buf, fldbuf, modbuf) ) return NullPtr(FpaEQTN_DATA *);

	/* Check if field name is an allowed element identifier */
	edef = identify_element(fldbuf);
	if ( NotNull(edef) )
		{
		}

	/* Check if field name is a field file identifier   */
	/*  (containing both element and level information) */
	/*  ... and if so, reset level modifiers            */
	else if ( parse_file_identifier(fldbuf, &edef, &ldef, NullStringPtr) )
		{

		/* Reset level based on enumerated level type */
		valid    = TRUE;
		validupr = TRUE;
		validlwr = TRUE;
		switch ( ldef->lvl_type )
			{

			/* Reset single pressure level */
			case FpaC_MSL:
			case FpaC_SURFACE:
			case FpaC_LEVEL:
			case FpaC_GEOGRAPHY:
			case FpaC_ANNOTATION:
				valid = reset_fieldmod_lvl(ldef->lev_lvls->lvl, lvl);
				break;

			/* Reset upper and lower pressure levels */
			case FpaC_LAYER:
				validupr = reset_fieldmod_lvl(ldef->lev_lvls->uprlvl, uprlvl);
				validlwr = reset_fieldmod_lvl(ldef->lev_lvls->lwrlvl, lwrlvl);
				break;

			/* Do not reset levels for other cases */
			default:
				break;
			}

		/* Error message if level modifier cannot be reset */
		if ( !valid || !validupr || !validlwr )
			{
			(void) fprintf(stderr, "[evaluate_field_string] Unacceptable levels");
			(void) fprintf(stderr, " in field identifier: \"%s\"\n",
					SafeStr(fldbuf));
			return NullPtr(FpaEQTN_DATA *);
			}
		}

	/* Return Null pointer if field name not found */
	else
		{
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Reset field modifiers */
	for ( pbgn=&modbuf[0]; *pbgn; )
		{
		pnmod = get_nsub(pbgn, modbuf, modbufl);
		/* Return Null pointer if problem resetting any modifier */
		if ( !reset_fieldmod(modbufl, lvl, uprlvl, lwrlvl, source, subsource,
				rtime, vtime, attrib, aunits, xlookup, &defval, &proximity,
					punits) ) return NullPtr(FpaEQTN_DATA *);
		pbgn = pnmod;
		}

	/* Build new level identifier using modified levels */
	ldef = identify_level_from_levels(edef->lvl_type, lvl, uprlvl, lwrlvl);

	/* Error message if element and modified levels are not consistent */
	if ( IsNull(ldef) )
		{
		(void) fprintf(stderr, "[evaluate_field_string] Inconsistent element");
		(void) fprintf(stderr, " and modified levels:\n");
		(void) fprintf(stderr, "   element: \"%s\"   level type: \"%d\"",
				SafeStr(edef->name), edef->lvl_type);
		(void) fprintf(stderr, "   lvl: \"%s\"   uprlvl: \"%s\"   lwrlvl: %s\n",
				SafeStr(lvl), SafeStr(uprlvl), SafeStr(lwrlvl));
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Convert proximity to km (if required) */
	if ( proximity > 0.0 )
		{
		(void) convert_value(punits, (double) proximity, ProximityUnits, &dval);
		proximity = (float) dval;
		}

	/* Store Equation Defaults for GENERIC level type "Any" */
	(void) save_equation_defaults(&OldEqtnDefs);

	/* Replace all default levels for GENERIC level type "Any" */
	/*  ... these will be defaults used in retrieve_surface    */
	if ( edef->lvl_type == FpaC_LVL_ANY )
		{
		(void) safe_strcpy(FpaEqtnDefs.lvl,    lvl);
		(void) safe_strcpy(FpaEqtnDefs.uprlvl, uprlvl);
		(void) safe_strcpy(FpaEqtnDefs.lwrlvl, lwrlvl);
		}

	/* Initialize a field descriptor using the modified field values */
	(void) init_fld_descript(&descript);
	if ( !set_fld_descript(&descript,
							FpaF_MAP_PROJECTION, &FpaEqtnDefs.mprojEval,
							FpaF_DIRECTORY_PATH, FpaEqtnDefs.path,
							FpaF_SOURCE_NAME,    source,
							FpaF_SUBSOURCE_NAME, subsource,
							FpaF_RUN_TIME,       rtime,
							FpaF_ELEMENT,        edef,
							FpaF_LEVEL,          ldef,
							FpaF_VALID_TIME,     vtime,
							FpaF_END_OF_LIST) )
		{
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Get detailed field information */
	fdef = get_field_info(edef->name, ldef->name);
	if ( IsNull(fdef) )
		{
		return NullPtr(FpaEQTN_DATA *);
		}

	/* Check for point evaluations */
	if ( FpaEqtnDefs.pointeval )
		{

		/* Evaluate VLIST Object using modified field values */
		switch ( fdef->element->fld_type )
			{

			/* Evaluate VLIST Object based on features and attributes */
			case FpaC_DISCRETE:
			case FpaC_WIND:
			case FpaC_LINE:
			case FpaC_SCATTERED:

				vlist = retrieve_vlist_by_attrib(&descript,
						FpaEqtnDefs.numposEval, FpaEqtnDefs.posEval,
						aunits, attrib, xlookup, defval, proximity);

				/* Error message if VLIST Object not found */
				if ( IsNull(vlist) )
					{
					(void) fprintf(stderr,
						"[evaluate_field_string] Data cannot be extracted");
					(void) fprintf(stderr,
						" for element: \"%s\"", SafeStr(edef->name));
					(void) fprintf(stderr,
						" and level: \"%s\"", SafeStr(ldef->name));
					(void) fprintf(stderr,
						" and attribute: \"%s\"\n", SafeStr(attrib));
					(void) restore_equation_defaults(&OldEqtnDefs);
					return NullPtr(FpaEQTN_DATA *);
					}

				break;

			/* Evaluate VLIST Object based on field values */
			case FpaC_CONTINUOUS:
			case FpaC_VECTOR:
			default:

				vlist = retrieve_vlist(&descript, FpaEqtnDefs.numposEval,
						FpaEqtnDefs.posEval);

				/* Error message if VLIST Object not found */
				if ( IsNull(vlist) )
					{
					(void) fprintf(stderr,
						"[evaluate_field_string] No data found");
					(void) fprintf(stderr,
						" for element: \"%s\"", SafeStr(edef->name));
					(void) fprintf(stderr,
						" and level: \"%s\"\n", SafeStr(ldef->name));
					(void) restore_equation_defaults(&OldEqtnDefs);
					return NullPtr(FpaEQTN_DATA *);
					}

				break;
			}

		/* Copy information from VLIST Object */
		pfld = init_eqtn_data(FpaEQT_Vlist);
		(void) copy_vlist(&pfld->Data.vlist, vlist);

		/* Free space used by VLIST Object */
		(void) free_vlist(vlist);
		FREEMEM(vlist);
		}

	/* Otherwise, evalutation is over a field */
	else
		{

		/* Evaluate SURFACE Object using modified field values */
		switch ( fdef->element->fld_type )
			{

			/* Evaluate SURFACE Object based on features and attributes */
			case FpaC_DISCRETE:
			case FpaC_WIND:
			case FpaC_LINE:
			case FpaC_SCATTERED:

				sfc = retrieve_surface_by_attrib(&descript,
						aunits, attrib, xlookup, defval, proximity);

				/* Error message if SURFACE Object not found */
				if ( IsNull(sfc) )
					{
					(void) fprintf(stderr,
						"[evaluate_field_string] SURFACE cannot be created");
					(void) fprintf(stderr,
						" for element: \"%s\"", SafeStr(edef->name));
					(void) fprintf(stderr,
						" and level: \"%s\"", SafeStr(ldef->name));
					(void) fprintf(stderr,
						" and attribute: \"%s\"\n", SafeStr(attrib));
					(void) restore_equation_defaults(&OldEqtnDefs);
					return NullPtr(FpaEQTN_DATA *);
					}

				break;

			/* Evaluate SURFACE Object based on field values */
			case FpaC_CONTINUOUS:
			case FpaC_VECTOR:
			default:

				sfc = retrieve_surface(&descript);

				/* Error message if SURFACE Object not found */
				if ( IsNull(sfc) )
					{
					(void) fprintf(stderr,
						"[evaluate_field_string] No SURFACE found");
					(void) fprintf(stderr,
						" for element: \"%s\"", SafeStr(edef->name));
					(void) fprintf(stderr,
						" and level: \"%s\"\n", SafeStr(ldef->name));
					(void) restore_equation_defaults(&OldEqtnDefs);
					return NullPtr(FpaEQTN_DATA *);
					}

				break;
			}

		/* Copy information from SURFACE Object into SPLINE Object */
		pfld = init_eqtn_data(FpaEQT_Spline);
		(void) copy_spline(&pfld->Data.splne, &sfc->sp);

		/* Free space used by SURFACE Object */
		sfc = destroy_surface(sfc);
		}

	/* Restore Equation Defaults (changed for GENERIC level type "Any") */
	/*  and return pointer to structure containing surface information  */
	(void) restore_equation_defaults(&OldEqtnDefs);
	return pfld;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Checking Equation Strings)              *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/**********************************************************************
 ***                                                                ***
 *** c h e c k _ e v a l u a t e _ e q u a t i o n                  ***
 ***                                                                ***
 *** check equation string by parsing equation into components and  ***
 ***  checking if components can be evaluated                       ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL			check_evaluate_equation

	(
	STRING			buf			/* string containing equation */
	)

	{
	char		xbuf[MAX_BCHRS];
	char		*pfop, *pnop;
	char		lbuf[MAX_BCHRS], rbuf[MAX_BCHRS];

	/* Return FALSE if no string to evaluate */
	if ( blank(buf) ) return FALSE;

	/* Expand equation by generic substitution of fields (if required) */
	if ( !generic_expansion(buf, xbuf) ) return FALSE;

	/* Identify location of first operator */
	pfop = get_fop(xbuf, lbuf);

	/* Check left side of equation ... except for function */
	/*  operators, where left side is function name        */
	if (*pfop != '[')
		{
		if ( DebugMode )
			{
			dprintf(stdout, "  Checking:  >%s<\n", SafeStr(lbuf));
			}
		if ( !blank(lbuf) && !check_evaluate_name(lbuf) ) return FALSE;
		}

	/* Return now if no operator was found */
	if (*pfop == '\0') return TRUE;

	/* If operators found, continue until no more found */
	do
		{
		/* Identify location of next operator */
		pnop = get_nop(pfop, xbuf, rbuf);

		/* Checking for all operators except function operators */
		if (*pfop != '[')
			{
			if ( !check_evaluate_equation(rbuf) ) return FALSE;
			}

		/* Checking for function operators */
		else
			{
			if ( !check_evaluate_function(lbuf, rbuf) ) return FALSE;
			}

		/* Go on to next operator ... until no more found */
		pfop = pnop;
		} while (*pfop != '\0');

	/* Return TRUE if all went well */
	return TRUE;
	}

/**********************************************************************
 ***                                                                ***
 *** c h e c k _ e v a l u a t e _ n a m e                          ***
 ***                                                                ***
 *** check a single string name identifying a constant or field     ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL			check_evaluate_name

	(
	STRING			buf			/* string containing constant or field name */
	)

	{
	char		tbuf[MAX_BCHRS];

	/* Return FALSE if no string name to evaluate */
	if ( blank(buf) ) return FALSE;

	/* Check for unary + or - ... and return TRUE   */
	/*  if string name contains only unary operator */
	(void) safe_strcpy(tbuf, buf);
	if ( check_evaluate_unary(tbuf) && blank(tbuf) ) return TRUE;

	/* Check for constant or named constant */
	if ( check_evaluate_constant(tbuf) ) return TRUE;

	/* Check for field name if not a constant or named constant */
	if ( check_evaluate_field_string(tbuf) ) return TRUE;

	/* Debug error message if string name cannot be evalutated */
	if ( DebugMode )
		{
		dprintf(stderr, "[check_evaluate_name]  No named constant or");
		dprintf(stderr, " valid field for: \"%s\"\n", SafeStr(tbuf));
		}

	/* Debug error message if name is a badly formatted function name */
	if ( DebugMode && NotNull(identify_function(tbuf)) )
		{
		dprintf(stderr, "[check_evaluate_name]  \"%s\" ", SafeStr(tbuf));
		dprintf(stderr, " is a function name!!\n");
		dprintf(stderr, "  It must be written  %s[ ... ]", SafeStr(tbuf));
		dprintf(stderr, "  with function arguments within [ ]\n");
		}

	/* Return FALSE since string name was not found */
	return FALSE;
	}

/**********************************************************************
 ***                                                                ***
 *** c h e c k _ e v a l u a t e _ f u n c t i o n                  ***
 ***                                                                ***
 *** check function applied to zero or more arguments               ***
 *** Note that a maximum of FpaEQFF_MaxFlds (5) arguments are       ***
 ***  allowed                                                       ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL			check_evaluate_function

	(
	STRING			lbuf,		/* string containing function name */
	STRING			rbuf		/* string containing function arguments */
	)

	{
	int					jargs=0;
	char				tlbuf[MAX_BCHRS], rbufl[MAX_BCHRS];
	char				*pbgn, *pnsub;
	const FpaEQTN_FUNC	*pfunc;

	/* Debug error message if no function name */
	if ( blank(lbuf) )
		{
		if ( DebugMode )
			{
			dprintf(stderr, "[check_evaluate_function]");
			dprintf(stderr, " No function name left of [\n");
			}
		return FALSE;
		}

	/* Debug error message if function name contains only unary + or - */
	(void) safe_strcpy(tlbuf, lbuf);
	if ( check_evaluate_unary(tlbuf) && blank(tlbuf) )
		{
		if ( DebugMode )
			{
			dprintf(stderr, "[check_evaluate_function] Only unary");
			dprintf(stderr, " operator to left of [\n");
			}
		return FALSE;
		}

	/* Identify function by name */
	pfunc = identify_function(tlbuf);

	/* Debug error message if function name not found */
	if ( IsNull(pfunc) )
		{
		if ( DebugMode )
			{
			dprintf(stderr, "[check_evaluate_function] Unrecognizable");
			dprintf(stderr, " function name: %s\n", SafeStr(tlbuf));
			}
		return FALSE;
		}

	if ( DebugMode )
		{
		dprintf(stdout, "  Argument checking for");
		dprintf(stdout, " function:  %s[]\n", SafeStr(tlbuf));
		}

	/* Check function arguments */
	for ( pbgn=&rbuf[0], jargs=0; *pbgn; jargs++ )
		{

		/* Identify and check function arguments */
		if ( jargs < FpaEQFF_MaxFlds )
			{
			pnsub = get_nsub(pbgn, rbuf, rbufl);
			if ( !check_evaluate_equation(rbufl) )
				{
				if ( DebugMode )
					{
					dprintf(stderr, "[check_evaluate_function]");
					dprintf(stderr, " Unacceptable");
					dprintf(stderr, " argument (%d)", jargs+1);
					dprintf(stderr, "   for function: %s\n", SafeStr(tlbuf));
					}
				return FALSE;
				}

			pbgn = pnsub;
			}

		/* Debug error message if too many arguments */
		else
			{
			if ( DebugMode )
				{
				dprintf(stderr, "[check_evaluate_function] More");
				dprintf(stderr, " than max number (%d)",
						FpaEQFF_MaxFlds);
				dprintf(stderr, " of arguments for");
				dprintf(stderr, " function: %s\n", SafeStr(tlbuf));
				}
			return FALSE;
			}
		}

	/* Debug error message if incorrect number of arguments */
	if ( (pfunc->Fargs != jargs) )
		{
		if ( DebugMode )
			{
			dprintf(stderr, "[check_evaluate_function]");
			dprintf(stderr, " Incorrect number of arguments");
			dprintf(stderr, " for function: %s\n", SafeStr(tlbuf));
			dprintf(stderr, "  Arguments: %d", jargs);
			dprintf(stderr, "   (Maximum: %d)", FpaEQFF_MaxFlds);
			dprintf(stderr, "   Required: %d\n", pfunc->Fargs);
			}
		return FALSE;
		}

	/* Branch to evaluations based on function type */
	switch ( pfunc->Ftype )
		{

		/* Return TRUE for UNIX functions */
		case FpaEQF_UNIX:
			return TRUE;

		/* Return TRUE for non-UNIX functions */
		case FpaEQF_ORDINARY:
		case FpaEQF_TIMESERIES:
			return TRUE;

		/* Debug error message for unacceptable function type */
		default:
			if ( DebugMode )
				{
				dprintf(stderr, "[check_evaluate_function]");
				dprintf(stderr, " Unacceptable function type: %d",
						pfunc->Ftype);
				dprintf(stderr, "   for function: %s\n", SafeStr(tlbuf));
				}
			return FALSE;
		}
	}

/**********************************************************************
 ***                                                                ***
 *** c h e c k _ e v a l u a t e _ u n a r y                        ***
 ***                                                                ***
 *** check for unary + or - at beginning of a string                ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL			check_evaluate_unary

	(
	STRING			buf			/* string which may contain unary + or - */
	)

	{

	/* Return FALSE if no string to evaluate */
	if ( blank(buf) ) return FALSE;

	/* Return FALSE if beginning of string is not unary + or - */
	if ( !(buf[0] == '+' || buf[0] == '-') ) return FALSE;

	/* Remove unary + or - from beginning of string and return TRUE */
	memmove(buf, buf+1, strlen(buf));
	return TRUE;
	}

/**********************************************************************
 ***                                                                ***
 *** c h e c k _ e v a l u a t e _ c o n s t a n t                  ***
 ***                                                                ***
 *** check if a string is a constant or a named constant            ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL			check_evaluate_constant

	(
	STRING			buf			/* string which may be a constant string */
								/*  or a named constant                  */
	)

	{
	char					*endbuf;
	FpaConfigConstantStruct	*cdef;

	/* Return FALSE if no string to evaluate */
	if ( blank(buf) ) return FALSE;

	/* Check for constant strings */
	(void) strtod(buf, &endbuf);
	if ( endbuf == (buf + strlen(buf)) ) return TRUE;

	/* Check for named constants */
	cdef = identify_constant(buf);
	if ( NotNull(cdef) ) return TRUE;

	/* Return FALSE if string is not a constant or a named constant */
	return FALSE;
	}

/**********************************************************************
 ***                                                                ***
 *** c h e c k _ e v a l u a t e _ f i e l d _ s t r i n g          ***
 ***                                                                ***
 *** check if a string identifies a named field with modified       ***
 ***  parameters                                                    ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL			check_evaluate_field_string

	(
	STRING			buf			/* string which may contain field name */
								/*  and field modifiers                */
	)

	{
	FpaEQUATION_DEFAULTS	OldEqtnDefs;
	char					*pbgn, *pnmod;
	char					lvl[MAX_NCHRS];
	char					uprlvl[MAX_NCHRS], lwrlvl[MAX_NCHRS];
	char					source[MAX_NCHRS], subsource[MAX_NCHRS];
	char					rtime[MAX_NCHRS], vtime[MAX_NCHRS];
	char					attrib[MAX_BCHRS], aunits[MAX_BCHRS];
	char					xlookup[MAX_BCHRS], punits[MAX_BCHRS];
	float					defval, proximity;
	char					fldbuf[MAX_BCHRS];
	char					modbuf[MAX_BCHRS], modbufl[MAX_BCHRS];
	FpaConfigElementStruct	*edef;
	FpaConfigLevelStruct	*ldef;
	FpaConfigFieldStruct	*fdef;
	LOGICAL					valid, validupr, validlwr;
	FLD_DESCRIPT			descript;

	/* Return FALSE if no string to evaluate */
	if ( blank(buf) ) return FALSE;

	/* Initialize field modifiers from default values */
	(void) init_fieldmod(lvl, uprlvl, lwrlvl, source, subsource,
			rtime, vtime, attrib, aunits, xlookup, &defval, &proximity, punits);

	/* Divide field string into field name and field modifiers */
	if ( !get_modstrng(buf, fldbuf, modbuf) ) return FALSE;

	/* Check if field name is an allowed element identifier */
	edef = identify_element(fldbuf);
	if ( NotNull(edef) )
		{
		}

	/* Check if field name is a field file identifier   */
	/*  (containing both element and level information) */
	/*  ... and if so, reset level modifiers            */
	else if ( parse_file_identifier(fldbuf, &edef, &ldef, NullStringPtr) )
		{

		/* Reset level based on enumerated level type */
		valid    = TRUE;
		validupr = TRUE;
		validlwr = TRUE;
		switch ( ldef->lvl_type )
			{

			/* Reset single pressure level */
			case FpaC_MSL:
			case FpaC_SURFACE:
			case FpaC_LEVEL:
			case FpaC_GEOGRAPHY:
			case FpaC_ANNOTATION:
				valid = reset_fieldmod_lvl(ldef->lev_lvls->lvl, lvl);
				break;

			/* Reset upper and lower pressure levels */
			case FpaC_LAYER:
				validupr = reset_fieldmod_lvl(ldef->lev_lvls->uprlvl, uprlvl);
				validlwr = reset_fieldmod_lvl(ldef->lev_lvls->lwrlvl, lwrlvl);
				break;

			/* Do not reset levels for other cases */
			default:
				break;
			}

		/* Debug error message if level modifier cannot be reset */
		if ( !valid || !validupr || !validlwr )
			{
			if ( DebugMode )
				{
				dprintf(stderr, "[check_evaluate_field_string] Unacceptable");
				dprintf(stderr, " levels in field identifier: %s\n",
						SafeStr(fldbuf));
				}
			return FALSE;
			}
		}

	/* Return FALSE if field name not found */
	else
		{
		return FALSE;
		}

	/* Reset field modifiers */
	for ( pbgn=&modbuf[0]; *pbgn; )
		{
		pnmod = get_nsub(pbgn, modbuf, modbufl);
		/* Return FALSE if problem resetting any modifier */
		if ( !reset_fieldmod(modbufl, lvl, uprlvl, lwrlvl, source, subsource,
				rtime, vtime, attrib, aunits, xlookup, &defval, &proximity,
					punits) ) return FALSE;
		pbgn = pnmod;
		}

	/* Build new level identifier using modified levels */
	ldef = identify_level_from_levels(edef->lvl_type, lvl, uprlvl, lwrlvl);

	/* Debug error message if element and modified levels are not consistent */
	if ( IsNull(ldef) )
		{
		if ( DebugMode )
			{
			dprintf(stderr, "[check_evaluate_field_string] Inconsistent");
			dprintf(stderr, " element and modified levels:\n");
			dprintf(stderr, "   element: %s   level type: %d",
					SafeStr(edef->name), edef->lvl_type);
			dprintf(stderr, "   lvl: %s   uprlvl: %s",
					SafeStr(lvl), SafeStr(uprlvl));
			dprintf(stderr, "   lwrlvl: %s\n", SafeStr(lwrlvl));
			}
		return FALSE;
		}

	/* Store Equation Defaults for GENERIC level type "Any" */
	(void) save_equation_defaults(&OldEqtnDefs);

	/* Replace all default levels for GENERIC level type "Any"   */
	/*  ... these will be defaults used in find_retrieve_metasfc */
	if ( edef->lvl_type == FpaC_LVL_ANY )
		{
		(void) safe_strcpy(FpaEqtnDefs.lvl,    lvl);
		(void) safe_strcpy(FpaEqtnDefs.uprlvl, uprlvl);
		(void) safe_strcpy(FpaEqtnDefs.lwrlvl, lwrlvl);
		}

	/* Initialize a field descriptor using the modified field values */
	(void) init_fld_descript(&descript);
	if ( !set_fld_descript(&descript,
							FpaF_MAP_PROJECTION, &FpaEqtnDefs.mprojEval,
							FpaF_DIRECTORY_PATH, FpaEqtnDefs.path,
							FpaF_SOURCE_NAME,    source,
							FpaF_SUBSOURCE_NAME, subsource,
							FpaF_RUN_TIME,       rtime,
							FpaF_ELEMENT,        edef,
							FpaF_LEVEL,          ldef,
							FpaF_VALID_TIME,     vtime,
							FpaF_END_OF_LIST) )
		{
		(void) restore_equation_defaults(&OldEqtnDefs);
		return FALSE;
		}

	/* Get detailed field information */
	fdef = get_field_info(edef->name, ldef->name);
	if ( IsNull(fdef) )
		{
		(void) restore_equation_defaults(&OldEqtnDefs);
		return FALSE;
		}

	/* Check for data in metafile identified by modified field values */
	switch ( fdef->element->fld_type )
		{

		/* Check for data based on features and attributes */
		case FpaC_DISCRETE:
		case FpaC_WIND:
		case FpaC_LINE:
		case FpaC_SCATTERED:
			
			/* Return if data for field and attribute can be found */
			if ( find_retrieve_metasfc_by_attrib(&descript, attrib) )
				{
				(void) restore_equation_defaults(&OldEqtnDefs);
				return TRUE;
				}

			/* Debug error message if data not found */
			else
				{
				if ( DebugMode )
					{
					dprintf(stderr, "[check_evaluate_field_string] Data");
					dprintf(stderr, " cannot be extracted for element: %s",
							SafeStr(edef->name));
					dprintf(stderr, " and level: %s", SafeStr(ldef->name));
					dprintf(stderr, " and attribute: %s\n", SafeStr(attrib));
					}
				(void) restore_equation_defaults(&OldEqtnDefs);
				return FALSE;
				}

		/* Check for data based on field values */
		case FpaC_CONTINUOUS:
		case FpaC_VECTOR:
		default:

			/* Return if data can be evaluated */
			if ( find_retrieve_metasfc(&descript) )
				{
				(void) restore_equation_defaults(&OldEqtnDefs);
				return TRUE;
				}

			/* Debug error message if data not found */
			else
				{
				if ( DebugMode )
					{
					dprintf(stderr, "[check_evaluate_field_string] No SURFACE");
					dprintf(stderr, " Object found for element: %s",
							SafeStr(edef->name));
					dprintf(stderr, " and level: %s\n", SafeStr(ldef->name));
					}
				(void) restore_equation_defaults(&OldEqtnDefs);
				return FALSE;
				}
		}
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Equation String Parsing)                *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/**********************************************************************
 ***                                                                ***
 *** r e m o v e _ b l a n k s                                      ***
 ***                                                                ***
 *** remove blanks from a equation string                           ***
 ***                                                                ***
 **********************************************************************/

static	void			remove_blanks

	(
	STRING			buf			/* string containing equation */
	)

	{
	int		j;
	char	bufnb[MAX_BCHRS];

	/* Initialize work string */
	bufnb[0] = '\0';

	/* Copy all non-blank characters to work string */
	for ( j=0; j<strlen(buf); j++ )
		if ( !strchr(WHITE, buf[j]) ) (void) strncat(bufnb, buf+j, 1);
		/* if ( buf[j] != ' ' ) (void) strncat(bufnb, buf+j, 1); */

	/* Overwrite equation string with work string */
	(void) safe_strcpy(buf, bufnb);
	}

/**********************************************************************
 ***                                                                ***
 *** g e n e r i c _ e x p a n s i o n                              ***
 ***                                                                ***
 *** expand equation string by replacing @ with default field buffer***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL			generic_expansion

	(
	STRING			buf,		/* string containing equation to be expanded */
	STRING			xbuf		/* string containing expanded equation */
	)

	{
	char		cbuf[MAX_BCHRS];
	int			ilen;
	char		*pfop, *pnop, *pbgn, *pnsub;
	char		lbuf[MAX_BCHRS], rbuf[MAX_BCHRS], rbufl[MAX_BCHRS];
	char		tbuf[MAX_BCHRS];

	/* Return FALSE if no string to expand */
	if ( blank(buf) ) return FALSE;

	/* Make a copy of the original equation string */
	(void) safe_strcpy(cbuf, buf);

	/* Remove any blanks from equation string to be expanded */
	remove_blanks(cbuf);

	/* Return immediately if no generic expansion of fields required */
	if ( !strchr(cbuf, '@') )
		{
		(void) safe_strcpy(xbuf, cbuf);
		return TRUE;
		}

	/* Initialize and set expected length of expanded equation */
	xbuf[0] = '\0';
	ilen = strlen(cbuf);

	/* Identify location of first operator */
	pfop = get_fop(cbuf, lbuf);

	/* Expand left side of equation if required */
	if ( lbuf[0] == '@' )
		{
		if ( DebugMode )
			{
			dprintf(stdout, "  Expanding:  >%s<", SafeStr(lbuf));
			dprintf(stdout, "     with:  >%s<\n", SafeStr(FpaEqtnDefs.genfld));
			}
		ilen += (strlen(FpaEqtnDefs.genfld) -1);
		if ( ilen >= MAX_BCHRS ) return FALSE;
		(void) safe_strcat(xbuf, FpaEqtnDefs.genfld);
		(void) safe_strcat(xbuf, lbuf+1);
		}
	else
		(void) safe_strcat(xbuf, lbuf);

	/* Return now if no operator was found */
	if (*pfop == '\0') return TRUE;

	/* If operators found, continue until no more found */
	do
		{
		/* Identify location of next operator */
		pnop = get_nop(pfop, cbuf, rbuf);

		/* Expansion for function operators */
		if (*pfop == '[')
			{
			(void) strcat(xbuf, "[");
			if ( DebugMode )
				{
				dprintf(stdout, "  Argument expansion for");
				dprintf(stdout, " function:  %s[]\n", SafeStr(lbuf));
				}

			/* Expand function arguments */
			for ( pbgn=&rbuf[0]; *pbgn; )
				{
				pnsub = get_nsub(pbgn, rbuf, rbufl);
				if ( !generic_expansion(rbufl, tbuf) ) return FALSE;
				ilen += (strlen(tbuf) - strlen(rbufl));
				if ( ilen >= MAX_BCHRS ) return FALSE;
				(void) safe_strcat(xbuf, tbuf);
				pbgn = pnsub;
				if (*pbgn != '\0') (void) strcat(xbuf, ",");
				}

			(void) strcat(xbuf, "]");
			}

		/* Expansion for bracket operators */
		else if (*pfop == '(')
			{
			(void) strcat(xbuf, "(");
			if ( !generic_expansion(rbuf, tbuf) ) return FALSE;
			ilen += (strlen(tbuf) - strlen(rbuf));
			if ( ilen >= MAX_BCHRS ) return FALSE;
			(void) safe_strcat(xbuf, tbuf);
			(void) strcat(xbuf, ")");
			}

		/* Expansion for all other operators */
		else
			{
			(void) strncat(xbuf, pfop, 1);
			if ( !generic_expansion(rbuf, tbuf) ) return FALSE;
			ilen += (strlen(tbuf) - strlen(rbuf));
			if ( ilen >= MAX_BCHRS ) return FALSE;
			(void) safe_strcat(xbuf, tbuf);
			}

		/* Go on to next operator ... until no more found */
		pfop = pnop;
		} while (*pfop != '\0');

	/* Return TRUE if all went well */
	return TRUE;
	}

/**********************************************************************
 ***                                                                ***
 *** g e t _ f o p                                                  ***
 ***                                                                ***
 *** return pointer to first operator in equation string            ***
 ***                                                                ***
 **********************************************************************/

static	char			*get_fop

	(
	STRING			buf,		/* string containing equation */
	STRING			lbuf		/* string containing left hand side  */
								/*  of equation up to first operator */
	)

	{
	int		jbgn, j, nestm;
	LOGICAL	unary, exponent;
	char	*pfop;

	/* Initialize string for left hand side of equation */
	lbuf[0] = '\0';

	/* Set start of nested field modifiers */
	nestm = 0;

	/* Identify location of first operator in string */
	for ( j=jbgn=0; j<strlen(buf); j++ )
		{

		/* Keep track of nested field modifiers */
		if ( buf[j] == '<' ) nestm++;
		if ( buf[j] == '>' ) nestm--;

		/* Exit immediately for unbalanced field modifiers */
		if ( nestm < 0 ) break;

		/* Set check for possible unary + or - at beginning of string */
		if ( j == jbgn )
			unary = TRUE;
		else
			unary = FALSE;

		/* Set check for possible exponent + or - */
		if ( j >= 2 && (buf[j-1] == 'e' || buf[j-1] == 'E')
				&& isdigit(buf[j-2]) )
			exponent = TRUE;
		else if ( j >= 3 && (buf[j-1] == 'e' || buf[j-1] == 'E')
				&& buf[j-2] == '.' && isdigit(buf[j-3]) )
			exponent = TRUE;
		else
			exponent = FALSE;

		/* Check for operators if no nested field modifiers         */
		/* Note that unary or exponent + or - is ignored            */
		if ( nestm == 0 )
			{
			if ( (buf[j] == '+' || buf[j] == '-')
				&& !unary && !exponent ) break;
			if ( buf[j] == '*' || buf[j] == '/' ) break;
			if ( buf[j] == '^' ) break;
			if ( buf[j] == '(' ) break;
			if ( buf[j] == '[' ) break;

			/* Error messages for unbalanced brackets, functions */
			if ( buf[j] == ')' )
				{
				(void) fprintf(stderr, "[get_fop] Unbalanced bracket )\n");
				(void) fprintf(stderr, "  in string: \"%s\"\n", SafeStr(buf));
				return '\0';
				}
			if ( buf[j] == ']' )
				{
				(void) fprintf(stderr, "[get_fop] Unbalanced function ]\n");
				(void) fprintf(stderr, "  in string: \"%s\"\n", SafeStr(buf));
				return '\0';
				}
			if ( buf[j] == ',' )
				{
				(void) fprintf(stderr, "[get_fop] Unattached function ,\n");
				(void) fprintf(stderr, "  in string: \"%s\"\n", SafeStr(buf));
				return '\0';
				}
			}
		}

	/* Error message for unbalanced field modifiers */
	if ( nestm != 0 )
		{
		(void) fprintf(stderr, "[get_fop] Unbalanced field modifiers < or >\n");
		(void) fprintf(stderr, "  in string: \"%s\"\n", SafeStr(buf));
		return '\0';
		}

	/* Set pointer to first operator in equation string */
	pfop = &buf[j];

	/* Set left side of equation */
	(void) safe_strcpy(lbuf, buf);
	if (*pfop != '\0')
		lbuf[j] = '\0';

	/* Return pointer to first operator in equation string */
	return pfop;
	}

/**********************************************************************
 ***                                                                ***
 *** g e t _ n o p                                                  ***
 ***                                                                ***
 *** return pointer to next operator in equation string             ***
 ***                                                                ***
 **********************************************************************/

static	char			*get_nop

	(
	char			*pfop,		/* pointer to present operator in equation */
	STRING			buf,		/* string containing equation */
	STRING			rbuf		/* string containing right hand side of equation */
								/*  ... from present operator to next operator   */
	)

	{
	int		jbgn, j, nestb, nestf, nestm;
	LOGICAL	unary, exponent;
	char	*pnop;

	/* Initialize string for right hand side of equation */
	rbuf[0] = '\0';

	/* Set start of nested brackets, functions, field modifiers */
	nestb = 0;
	if (*pfop == '(') nestb++;
	nestf = 0;
	if (*pfop == '[') nestf++;
	nestm = 0;

	/* Identify location of next operator       */
	/*  ... depending on type of first operator */
	for ( j=jbgn=(pfop-buf+1); j<strlen(buf); j++ )
		{

		/* Keep track of nested brackets */
		if ( buf[j] == '(' ) nestb++;
		if ( buf[j] == ')' ) nestb--;

		/* Keep track of nested functions */
		if ( buf[j] == '[' ) nestf++;
		if ( buf[j] == ']' ) nestf--;

		/* Keep track of nested field modifiers */
		if ( buf[j] == '<' ) nestm++;
		if ( buf[j] == '>' ) nestm--;

		/* Exit immediately for unbalanced brackets, functions, */
		/*  field modifiers                                     */
		if ( nestb < 0 || nestf < 0 || nestm < 0 ) break;

		/* Set check for possible unary + or - at beginning of string */
		/*  or immediately following another operator                 */
		if ( j == jbgn || buf[j-1] == '+' || buf[j-1] == '-'
				|| buf[j-1] == '*' || buf[j-1] == '/' || buf[j-1] == '^' )
			unary = TRUE;
		else
			unary = FALSE;

		/* Set check for possible exponent + or - */
		if ( j >= 2 && (buf[j-1] == 'e' || buf[j-1] == 'E')
				&& isdigit(buf[j-2]) )
			exponent = TRUE;
		else
			exponent = FALSE;

		/* Check for operators if no nested brackets, functions, */
		/*  field modifiers                                      */
		/* Note that unary or exponent + or - is ignored         */
		if ( nestb == 0 && nestf == 0 && nestm == 0 )
			{

			/* + or - operators end at + or - */
			if (*pfop == '+' || *pfop == '-')
				{
				if ( (buf[j] == '+' || buf[j] == '-')
						&& !unary && !exponent ) break;
				}

			/* * or / operators end at * or / or + or - */
			else if (*pfop == '*' || *pfop == '/')
				{
				if ( (buf[j] == '*' || buf[j] == '/')
						|| ((buf[j] == '+' || buf[j] == '-')
						&& !unary && !exponent) ) break;
				}

			/* ^ operator ends at * or / or + or - */
			else if (*pfop == '^')
				{
				if ( (buf[j] == '*' || buf[j] == '/')
						|| ((buf[j] == '+' || buf[j] == '-')
						&& !unary && !exponent) ) break;
				}

			/* ( or [ operators end just after nesting ends */
			else if (*pfop == '(' || *pfop == '[')
				{
				j++;
				break;
				}
			}
		}

	/* Error messages for unbalanced brackets, functions, modifiers */
	if ( nestb != 0 )
		{
		(void) fprintf(stderr, "[get_nop] Unbalanced brackets ( or )\n");
		(void) fprintf(stderr, "  in string: \"%s\"\n", SafeStr(buf));
		return '\0';
		}
	if ( nestf != 0 )
		{
		(void) fprintf(stderr, "[get_nop] Unbalanced function [ or ]\n");
		(void) fprintf(stderr, "  in string: \"%s\"\n", SafeStr(buf));
		return '\0';
		}
	if ( nestm != 0 )
		{
		(void) fprintf(stderr, "[get_nop] Unbalanced field modifiers < or >\n");
		(void) fprintf(stderr, "  in string: \"%s\"\n", SafeStr(buf));
		return '\0';
		}

	/* Set pointer to next operator in equation string      */
	/*  ... or to Null character if no more operators found */
	pnop = &buf[j];

	/* Set right side of equation based on operator */
	(void) safe_strcpy(rbuf, buf+jbgn);
	if (*pfop == '(' || *pfop == '[')
		rbuf[j-jbgn-1] = '\0';
	else
		rbuf[j-jbgn] = '\0';

	/* Return pointer to next operator in equation string */
	return pnop;
	}

/**********************************************************************
 ***                                                                ***
 *** g e t _ n s u b                                                ***
 ***                                                                ***
 *** return pointer to next substring in a function/modifier string ***
 ***                                                                ***
 **********************************************************************/

static	char			*get_nsub

	(
	char			*pbgn,		/* pointer to beginning of a substring in a */
								/*  function/modifier string                */
	STRING			buf,		/* string containing function/modifier */
								/*  substrings separated by commas     */
	STRING			bufl		/* string containing function/modifier  */
								/*  substring beginning at pointer pbgn */
	)

	{
	int		j, nestb, nestf, nestm;
	char	*pnsub;

	/* Initialize string for function/modifier substring */
	bufl[0] = '\0';

	/* Set start of nested brackets, functions, field modifiers */
	nestb = 0;
	nestf = 0;
	nestm = 0;

	/* Build substring and identify location of following substring */
	for ( j=(pbgn-buf); j<strlen(buf); j++ )
		{

		/* Keep track of nested brackets */
		if ( buf[j] == '(' ) nestb++;
		if ( buf[j] == ')' ) nestb--;

		/* Keep track of nested functions */
		if ( buf[j] == '[' ) nestf++;
		if ( buf[j] == ']' ) nestf--;

		/* Keep track of nested field modifiers */
		if ( buf[j] == '<' ) nestm++;
		if ( buf[j] == '>' ) nestm--;

		/* Exit immediately for unbalanced brackets, functions, */
		/*  field modifiers                                     */
		if ( nestb < 0 || nestf < 0 || nestm < 0 ) break;

		/* Check for comma ending substring if no nested brackets, */
		/*  functions, field modifiers                             */
		if ( nestb == 0 && nestf == 0 && nestm == 0 )
			{

			/* Set location for start of next function/modifier */
			/*  substring to character just after comma         */
			if ( buf[j] == ',' && !blank(bufl) )
				{
				j++;
				break;
				}

			/* Skip blank function/modifier substrings  */
			/*  ... and keep looking for next substring */
			else if ( buf[j] == ',' && blank(bufl) ) continue;

			}

		/* Add character to substring */
		(void) strncat(bufl, buf+j, 1);
		}

	/* Error messages for unbalanced brackets, functions, modifiers */
	if ( nestb != 0 )
		{
		(void) fprintf(stderr, "[get_nsub] Unbalanced brackets ( or )\n");
		(void) fprintf(stderr, "  in string: \"%s\"\n", SafeStr(buf));
		bufl[0] = '\0';
		return '\0';
		}
	if ( nestf != 0 )
		{
		(void) fprintf(stderr, "[get_nsub] Unbalanced function [ or ]\n");
		(void) fprintf(stderr, "  in string: \"%s\"\n", SafeStr(buf));
		bufl[0] = '\0';
		return '\0';
		}
	if ( nestm != 0 )
		{
		(void) fprintf(stderr, "[get_nsub] Unbalanced field modifiers < or >\n");
		(void) fprintf(stderr, "  in string: \"%s\"\n", SafeStr(buf));
		bufl[0] = '\0';
		return '\0';
		}

	/* Set pointer to start of next function/modifier substring */
	/*  ... or to Null character if no more commas found        */
	pnsub = &buf[j];

	/* Return pointer to start of next function/modifier substring */
	return pnsub;
	}

/**********************************************************************
 ***                                                                ***
 *** g e t _ m o d s t r n g                                        ***
 ***                                                                ***
 *** divide a field string into a field name and a string of field  ***
 ***  modifiers                                                     ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL			get_modstrng

	(
	STRING			buf,		/* string containing field name and modifiers */
	STRING			lbuf,		/* string containing field name */
	STRING			rbuf		/* string containing field modifiers */
	)

	{
	int		j, nestb, nestf, nestm, modpair;

	/* Initialize strings for field name and field modifiers */
	lbuf[0] = '\0';
	rbuf[0] = '\0';

	/* Build field name and identify location of field modifier string */
	for ( j=0; j<strlen(buf); j++ )
		{

		/* Error message if unacceptable character precedes */
		/*  field modifier string                           */
		if ( buf[j] == '+' || buf[j] == '-' || buf[j] == '*'
				|| buf[j] == '/' || buf[j] == '^' || buf[j] == '('
				|| buf[j] == ')' || buf[j] == '[' || buf[j] == ']'
				|| buf[j] == '>' )
			{
			(void) fprintf(stderr, "[get_modstrng] Unacceptable character");
			(void) fprintf(stderr, " precedes field modifer string\n");
			(void) fprintf(stderr, "  in field string: \"%s\"\n", SafeStr(buf));
			lbuf[0] = '\0';
			return FALSE;
			}

		/* Set location for start of field modifier string */
		/*  to character just after angle bracket          */
		if ( buf[j] == '<' )
			{
			j++;
			break;
			}

		/* Add character to field name string */
		(void) strncat(lbuf, buf+j, 1);
		}

	/* Error message if blank field name string */
	if ( blank(lbuf) )
		{
		(void) fprintf(stderr, "[get_modstrng] No field name\n");
		(void) fprintf(stderr, "  in field string: \"%s\"\n", SafeStr(buf));
		return FALSE;
		}

	/* Return now if no field modifier string */
	if ( j == strlen(buf) ) return TRUE;

	/* Error message if unacceptable character at end of field string */
	if ( buf[strlen(buf) - 1] != '>' )
		{
		(void) fprintf(stderr, "[get_modstrng] Unacceptable character at end\n");
		(void) fprintf(stderr, "  of field string: \"%s\"\n", SafeStr(buf));
		return FALSE;
		}

	/* Set start of nested brackets, functions, field modifiers */
	/*  ... and set check for field modifier pair               */
	nestb = 0;
	nestf = 0;
	nestm = 0;
	modpair = 0;

	/* Build field modifier string */
	for ( ; j<(strlen(buf) - 1); j++)
		{

		/* Check for second part of field modifier pair  ><  */
		/*  ... and replace it with a comma                  */
		if ( modpair && buf[j] == '<' )
			{
			(void) strncat(rbuf, ",", 1);
			nestm++;
			modpair = 0;
			continue;
			}

		/* Error message for unacceptable character following > */
		else if ( modpair )
			{
			(void) fprintf(stderr, "[get_modstrng] Unacceptable character");
			(void) fprintf(stderr, " follows >\n");
			(void) fprintf(stderr, "  in field string: \"%s\"\n", SafeStr(buf));
			lbuf[0] = '\0';
			rbuf[0] = '\0';
			return FALSE;
			}

		/* Keep track of nested brackets */
		if ( buf[j] == '(' ) nestb++;
		if ( buf[j] == ')' ) nestb--;

		/* Keep track of nested functions */
		if ( buf[j] == '[' ) nestf++;
		if ( buf[j] == ']' ) nestf--;

		/* Keep track of nested field modifiers */
		if ( buf[j] == '<' ) nestm++;
		if ( buf[j] == '>' ) nestm--;

		/* Exit immediately for unbalanced brackets, functions */
		if ( nestb < 0 || nestf < 0 ) break;

		/* Check for first part of field modifier pair  ><  */
		if ( nestb == 0 && nestf == 0 && nestm < 0 )
			{
			modpair = 1;
			continue;
			}

		/* Add character to field modifier string */
		(void) strncat(rbuf, buf+j, 1);
		}

	/* Error messages for unbalanced brackets, functions, modifiers */
	if ( nestb != 0 )
		{
		(void) fprintf(stderr, "[get_modstrng] Unbalanced brackets ( or )\n");
		(void) fprintf(stderr, "  in field string: \"%s\"\n", SafeStr(buf));
		lbuf[0] = '\0';
		rbuf[0] = '\0';
		return FALSE;
		}
	if ( nestf != 0 )
		{
		(void) fprintf(stderr, "[get_modstrng] Unbalanced function [ or ]\n");
		(void) fprintf(stderr, "  in field string: \"%s\"\n", SafeStr(buf));
		lbuf[0] = '\0';
		rbuf[0] = '\0';
		return FALSE;
		}
	if ( nestm != 0 )
		{
		(void) fprintf(stderr, "[get_modstrng] Unbalanced field modifiers");
		(void) fprintf(stderr, " < or >\n  in field string: \"%s\"\n", SafeStr(buf));
		lbuf[0] = '\0';
		rbuf[0] = '\0';
		return FALSE;
		}

	/* Return TRUE if all went well */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Setting Field Modifiers)                *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/**********************************************************************
 ***                                                                ***
 *** i n i t _ f i e l d m o d                                      ***
 ***                                                                ***
 *** initialize field modifiers from default modifiers              ***
 ***                                                                ***
 **********************************************************************/

static	void			init_fieldmod

	(
	STRING			lvl,		/* string containing default pressure level */
	STRING			uprlvl,		/* string containing default upper pressure */
								/*  level                                   */
	STRING			lwrlvl,		/* string containing default lower pressure */
								/*  level                                   */
	STRING			source,		/* string containing default source */
	STRING			subsource,	/* string containing default subsource */
	STRING			rtime,		/* string containing default run timestamp */
	STRING			vtime,		/* string containing default valid timestamp */
	STRING			attrib,		/* string containing default attribute */
	STRING			aunits,		/* string containing units for attribute */
	STRING			xlookup,	/* string containing default attribute lookup */
	float			*defval,	/* default value for attribute */
	float			*proximity,	/* default proximity to features */
	STRING			punits		/* string containing units for proximity */
	)

	{

	/* Initialize field modifiers from default parameters */
	(void) safe_strcpy(lvl,       FpaEqtnDefs.lvl);
	(void) safe_strcpy(uprlvl,    FpaEqtnDefs.uprlvl);
	(void) safe_strcpy(lwrlvl,    FpaEqtnDefs.lwrlvl);
	(void) safe_strcpy(source,    FpaEqtnDefs.source);
	(void) safe_strcpy(subsource, FpaEqtnDefs.subsource);
	(void) safe_strcpy(rtime,     FpaEqtnDefs.rtime);
	(void) safe_strcpy(vtime,     FpaEqtnDefs.vtime);

	/* Initialize field attribute modifiers from default parameters */
	(void) safe_strcpy(attrib,    FpaCblank);
	(void) safe_strcpy(aunits,    FpaCmksUnits);
	(void) safe_strcpy(xlookup,   FpaCblank);
	(void) safe_strcpy(punits,    ProximityUnits);
	if (NotNull(defval))    *defval    = 0.0;
	if (NotNull(proximity)) *proximity = 0.0;
	}

/**********************************************************************
 ***                                                                ***
 *** r e s e t _ f i e l d m o d                                    ***
 ***                                                                ***
 *** reset field modifiers                                          ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL			reset_fieldmod

	(
	STRING			buf,		/* string containing field modifier */
	STRING			lvl,		/* string containing pressure level */
	STRING			uprlvl,		/* string containing upper pressure level */
	STRING			lwrlvl,		/* string containing lower pressure level */
	STRING			source,		/* string containing source */
	STRING			subsrc,		/* string containing subsource */
	STRING			rtime,		/* string containing run timestamp */
	STRING			vtime,		/* string containing valid timestamp */
	STRING			attrib,		/* string containing attribute */
	STRING			aunits,		/* string containing units for attribute */
	STRING			xlookup,	/* string containing attribute lookup */
	float			*defval,	/* default value for attribute */
	float			*proximity,	/* proximity to features */
	STRING			punits		/* string containing units for proximity */
	)

	{
	char					cbuf[MAX_BCHRS];
	FLD_DESCRIPT			descript;
	FpaConfigLevelStruct	*ldef;
	LOGICAL					valid, validupr, validlwr;

	/* Make a copy of the original field modifier */
	(void) safe_strcpy(cbuf, buf);

	/* First check for generic replacement of field buffer */
	if ( cbuf[0] == '@' )
		{
		/* Set generic replacement of field buffer */
		if ( cbuf[1] != '\0' )
			{
			(void) safe_strcpy(FpaEqtnDefs.genfld, cbuf+1);
			return TRUE;
			}
		/* Return if generic replacement of field buffer */
		/*  will be at a deeper level                    */
		else
			{
			return TRUE;
			}
		}

	/* Next check for generic replacement of modifier buffer */
	else if ( cbuf[0] == '!' )
		{
		/* Set generic replacement of modifier buffer */
		if ( cbuf[1] != '\0' )
			{
			(void) safe_strcpy(FpaEqtnDefs.genmod, cbuf+1);
			return TRUE;
			}
		/* Retrieve generic replacement of modifier buffer */
		else
			{
			(void) safe_strcpy(cbuf, FpaEqtnDefs.genmod);
			}
		}

	/* Next check for generic replacement of default level */
	/*  by upper level pressure or lower level pressure    */
	if ( same(cbuf, "$upper") )
		{
		(void) safe_strcpy(lvl, uprlvl);
		return TRUE;
		}
	else if ( same(cbuf, "$lower") )
		{
		(void) safe_strcpy(lvl, lwrlvl);
		return TRUE;
		}

	/* Next check for pressure level modifiers */
	ldef = identify_level(cbuf);
	if ( NotNull(ldef) )
		{

		/* Reset level based on enumerated level type */
		switch ( ldef->lvl_type )
			{

			/* Reset single pressure level */
			case FpaC_MSL:
			case FpaC_SURFACE:
			case FpaC_LEVEL:
			case FpaC_GEOGRAPHY:
			case FpaC_ANNOTATION:
				valid = reset_fieldmod_lvl(ldef->lev_lvls->lvl, lvl);
				return valid;

			/* Reset upper and lower pressure levels */
			case FpaC_LAYER:
				validupr = reset_fieldmod_lvl(ldef->lev_lvls->uprlvl, uprlvl);
				validlwr = reset_fieldmod_lvl(ldef->lev_lvls->lwrlvl, lwrlvl);
				return (validupr && validlwr);

			/* Error will occur for other cases */
			default:
				(void) fprintf(stderr, "[reset_fieldmod] Unacceptable level");
				(void) fprintf(stderr, " type for field modifier: \"%s\"\n",
					SafeStr(buf));
				return FALSE;
			}
		}

	/* Set a field descriptor for use with field modifiers for */
	/*  run time stamp or valid time stamp                     */
	if ( cbuf[0] == 'r' || cbuf[0] == 'v' )
		{
		(void) init_fld_descript(&descript);
		(void) set_fld_descript(&descript,
								FpaF_DIRECTORY_PATH, FpaEqtnDefs.path,
								FpaF_SOURCE_NAME,    source,
								FpaF_SUBSOURCE_NAME, subsrc,
								FpaF_RUN_TIME,       rtime,
								FpaF_END_OF_LIST);
		}

	/* Next check for field modifiers starting with  u  l  m  r  v  */

	/*  ... reset upper level pressure */
	if ( cbuf[0] == 'u' && reset_fieldmod_lvl(cbuf+1, uprlvl) )
			return TRUE;

	/*  ... reset lower level pressure */
	else if ( cbuf[0] == 'l' && reset_fieldmod_lvl(cbuf+1, lwrlvl) )
			return TRUE;

	/*  ... reset source and subsource */
	else if ( cbuf[0] == 'm' && reset_fieldmod_source(cbuf+1, source, subsrc) )
			return TRUE;

	/*  ... reset run time stamp */
	else if ( cbuf[0] == 'r' && reset_fieldmod_rtime(cbuf+1, &descript, rtime) )
			return TRUE;

	/*  ... reset valid time stamp */
	else if ( cbuf[0] == 'v' && reset_fieldmod_vtime(cbuf+1, &descript, vtime) )
			return TRUE;

	/* Next check for field modifiers starting with  a  b  x  d  p  q  */

	/*  ... reset attribute */
	else if ( cbuf[0] == 'a' && reset_fieldmod_attrib(cbuf+1, attrib) )
			return TRUE;

	/*  ... reset attribute */
	else if ( cbuf[0] == 'b' && reset_fieldmod_aunits(cbuf+1, aunits) )
			return TRUE;

	/*  ... reset attribute lookup */
	else if ( cbuf[0] == 'x' && reset_fieldmod_xlookup(cbuf+1, xlookup) )
			return TRUE;

	/*  ... reset default value for attribute */
	else if ( cbuf[0] == 'd' && reset_fieldmod_defval(cbuf+1, defval) )
			return TRUE;

	/*  ... reset proximity to features */
	else if ( cbuf[0] == 'p' && reset_fieldmod_proximity(cbuf+1, proximity) )
			return TRUE;

	/*  ... reset proximity units */
	else if ( cbuf[0] == 'q' && reset_fieldmod_punits(cbuf+1, punits) )
			return TRUE;

	/* Error message for unrecognizable field modifiers */
	(void) fprintf(stderr, "[reset_fieldmod] Unrecognizable");
	(void) fprintf(stderr, " field modifier: \"%s\"\n", SafeStr(buf));
	return FALSE;
	}

/**********************************************************************
 ***                                                                ***
 *** r e s e t _ f i e l d m o d _ l v l                            ***
 ***                                                                ***
 *** return TRUE/FALSE from resetting pressure level from pressure  ***
 ***  level modifier                                                ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL			reset_fieldmod_lvl

	(
	STRING			buf,		/* string containing pressure level modifier */
	STRING			lvl			/* string containing modified pressure level */
	)

	{
	FpaConfigLevelStruct	*ldef;

	/* Identify level from modifier */
	ldef = identify_level(buf);
	if ( NotNull(ldef) )
		{

		/* Reset level based on enumerated level type */
		switch ( ldef->lvl_type )
			{

			/* Reset level from single pressure level */
			case FpaC_MSL:
			case FpaC_SURFACE:
			case FpaC_LEVEL:
			case FpaC_GEOGRAPHY:
			case FpaC_ANNOTATION:
				(void) safe_strcpy(lvl, ldef->lev_lvls->lvl);
				return TRUE;

			/* Error for more than one level in level modifier */
			case FpaC_LAYER:
				(void) fprintf(stderr, "[reset_fieldmod_lvl] More than one level");
				(void) fprintf(stderr, " in level modifier: \"%s\"\n", SafeStr(buf));
				return FALSE;

			/* Error will occur for other cases */
			default:
				break;
			}
		}

	/* Error message for unrecognizable level modifier */
	(void) fprintf(stderr, "[reset_fieldmod_lvl] Unrecognizable");
	(void) fprintf(stderr, " level modifier: \"%s\"\n", SafeStr(buf));
	return FALSE;
	}

/**********************************************************************
 ***                                                                ***
 *** r e s e t _ f i e l d m o d _ s o u r c e                      ***
 ***                                                                ***
 *** return TRUE/FALSE from resetting source (and subsource)        ***
 ***  from source modifier                                          ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL			reset_fieldmod_source

	(
	STRING			buf,		/* string containing source modifier */
	STRING			source,		/* string containing modified source */
	STRING			subsource	/* string containing modified subsource */
	)

	{
	STRING					src, subsrc;
	FpaConfigSourceStruct	*sdef;

	/* Parse source modifier into source and subsource strings */
	if ( parse_source_name(buf, &src, &subsrc) )
		{

		/* Reset source (and subsource) modifier for valid sources */
		sdef = identify_source(src, subsrc);
		if ( NotNull(sdef) )
			{
			(void) safe_strcpy(source,    sdef->name);
			(void) safe_strcpy(subsource, sdef->src_sub->name);
			return TRUE;
			}
		}

	/* Error message for unrecognizable source modifier */
	(void) fprintf(stderr, "[reset_fieldmod_source] Unrecognizable");
	(void) fprintf(stderr, " source modifier: \"%s\"\n", SafeStr(buf));
	return FALSE;
	}

/**********************************************************************
 ***                                                                ***
 *** r e s e t _ f i e l d m o d _ r t i m e                        ***
 ***                                                                ***
 *** return TRUE/FALSE from resetting run timestamp from run        ***
 ***  timestamp modifier                                            ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL			reset_fieldmod_rtime

	(
	STRING			buf,		/* string containing run time modifier */
	FLD_DESCRIPT	*fdesc,		/* pointer to field descriptor       */
								/*  containing directory information */
	STRING			rtime		/* string containing modified run timestamp */
	)

	{
	LOGICAL	local;
	int		ii, year, jday, hour;
	int		minute = 0, second = 0;
	STRING	xtime;
	int		inum;
	STRING	*rlist;

	/* Copy run time modifier if it looks like a timestamp */
	if ( parse_tstamp(buf, NullInt, NullInt, NullInt, NullInt, NullLogicalPtr,
						NullLogicalPtr) )
		{
		(void) strcpy(rtime, buf);
		return TRUE;
		}

	/* Adjust time from run time modifier if adjustment is + or - */
	if ( buf[0] == '+' || buf[0] == '-' )
		{

		/* Error message if any remaining character is non-numeric */
		for ( ii=1; ii<strlen(buf); ii++ )
			{
			if ( !isdigit(buf[ii]) )
				{
				(void) fprintf(stderr, "[reset_fieldmod_rtime] Unrecognizable");
				(void) fprintf(stderr, " run time modifier: \"%s\"\n", SafeStr(buf));
				return FALSE;
				}
			}

		/* Get date from run timestamp */
		if ( parse_tstamp(rtime, &year, &jday, &hour, &minute, &local,
						NullLogicalPtr) )
			{

			/* Adjust hour from run time modifier */
			if ( buf[0] == '+' )      hour = hour + atoi(buf+1);
			else if ( buf[0] == '-' ) hour = hour - atoi(buf+1);
			(void) tnorm(&year, &jday, &hour, &minute, &second);

			/* Remake run timestamp */
			xtime = build_tstamp(year, jday, hour, minute, local, FALSE);
			if ( !blank(xtime) )
				{
				(void) safe_strcpy(rtime, xtime);
				return TRUE;
				}
			}
		}

	/* Set run time modifier from top-most directory for "current" */
	if ( same_ic(buf, "current") )
		{
		/* Get list of run times */
		inum = source_run_time_list(fdesc, &rlist);
		if ( inum < 1 )
			{
			(void) fprintf(stderr, "[reset_fieldmod_rtime] No data in directories");
			(void) fprintf(stderr, " for: \"%s %s\"\n",
					SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
			return FALSE;
			}
		/* Set run time from top-most directory */
		(void) safe_strcpy(rtime, rlist[0]);
		inum = source_run_time_list_free(&rlist, inum);
		return TRUE;
		}

	/* Set run time modifier from previous directory for "previous" */
	else if ( same_ic(buf, "previous") )
		{
		/* Get list of run times */
		inum = source_run_time_list(fdesc, &rlist);
		if ( inum < 1 )
			{
			(void) fprintf(stderr, "[reset_fieldmod_rtime] No data in directories");
			(void) fprintf(stderr, " for: \"%s %s\"\n",
					SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
			return FALSE;
			}
		/* Check for next directory to current run time */
		for ( ii=0; ii<inum; ii++ )
			{
			if ( blank(rtime) || same(rtime, rlist[ii]) ) break;
			}
		if ( ++ii >= inum )
			{
			(void) fprintf(stderr, "[reset_fieldmod_rtime] No previous directory");
			(void) fprintf(stderr, " for: \"%s %s  %s\"\n",
					SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name),
					SafeStr(rtime));
			inum = source_run_time_list_free(&rlist, inum);
			return FALSE;
			}
		/* Set run time from next directory to current run time */
		(void) safe_strcpy(rtime, rlist[ii]);
		inum = source_run_time_list_free(&rlist, inum);
		return TRUE;
		}

	/* Error message for unrecognizable run time modifier */
	(void) fprintf(stderr, "[reset_fieldmod_rtime] Unrecognizable");
	(void) fprintf(stderr, " run time modifier: \"%s\"\n", SafeStr(buf));
	return FALSE;
	}

/**********************************************************************
 ***                                                                ***
 *** r e s e t _ f i e l d m o d _ v t i m e                        ***
 ***                                                                ***
 *** return TRUE/FALSE from resetting valid timestamp from run      ***
 ***  timestamp and valid time modifier                             ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL			reset_fieldmod_vtime

	(
	STRING			buf,		/* string containing valid time modifier */
	FLD_DESCRIPT	*fdesc,		/* pointer to field descriptor       */
								/*  containing directory information */
	STRING			vtime		/* string containing modified valid timestamp */
	)

	{

	LOGICAL	local;
	int		ii, year, jday, hour, macro;
	int		minute = 0, second = 0;
	STRING	xtime;
	int		inum;
	STRING	*vlist;

	/* Copy valid time modifier if it looks like a timestamp */
	if ( parse_tstamp(buf, NullInt, NullInt, NullInt, NullInt, NullLogicalPtr,
					NullLogicalPtr) )
		{
		(void) strcpy(vtime, buf);
		return TRUE;
		}

	/* Adjust time from valid time modifier if adjustment is + or - */
	if ( buf[0] == '+' || buf[0] == '-' )
		{

		/* Error message if any remaining character is non-numeric */
		for ( ii=1; ii<strlen(buf); ii++ )
			{
			if ( !isdigit(buf[ii]) )
				{
				(void) fprintf(stderr, "[reset_fieldmod_vtime] Unrecognizable");
				(void) fprintf(stderr, " valid time modifier: \"%s\"\n", SafeStr(buf));
				return FALSE;
				}
			}

		/* Get date from valid timestamp */
		if ( parse_tstamp(vtime, &year, &jday, &hour, &minute, &local,
						NullLogicalPtr) )
			{

			/* Adjust hour from valid time modifier */
			if ( buf[0] == '+' )      hour = hour + atoi(buf+1);
			else if ( buf[0] == '-' ) hour = hour - atoi(buf+1);
			(void) tnorm(&year, &jday, &hour, &minute, &second);

			/* Remake valid timestamp */
			xtime = build_tstamp(year, jday, hour, minute, local, FALSE);
			if ( !blank(xtime) )
				{
				(void) strcpy(vtime, xtime);
				return TRUE;
				}
			}
		}

	/* Adjust time from run timestamp and valid time modifier */
	/*  if adjustment is all numeric                          */
	else if ( isdigit(buf[0]) )
		{

		/* Error message if any character is non-numeric */
		for ( ii=0; ii<strlen(buf); ii++ )
			{
			if ( !isdigit(buf[ii]) )
				{
				(void) fprintf(stderr, "[reset_fieldmod_vtime] Unrecognizable");
				(void) fprintf(stderr, " valid time modifier: \"%s\"\n", SafeStr(buf));
				return FALSE;
				}
			}

		/* Get date from run timestamp */
		if ( parse_tstamp(fdesc->rtime, &year, &jday, &hour, &minute, &local,
						NullLogicalPtr) )
			{

			/* Adjust hour from valid time modifier */
			hour = hour + atoi(buf);
			(void) tnorm(&year, &jday, &hour, &minute, &second);

			/* Remake valid timestamp */
			xtime = build_tstamp(year, jday, hour, minute, local, FALSE);
			if ( !blank(xtime) )
				{
				(void) strcpy(vtime, xtime);
				return TRUE;
				}
			}
		}

	/* Set valid time modifier from previous valid time for "previous" */
	if ( same_ic(buf, "previous") )
		{

		/* Set the type of fields from the valid time (normal or local) */
		(void) parse_tstamp(FpaEqtnDefs.vtime,
								NullInt, NullInt, NullInt, NullInt, &local,
								NullLogicalPtr);
		if ( local ) macro = FpaC_DAILY;
		else         macro = (FpaC_NORMAL | FpaC_STATIC);

		/* Get list of valid times */
		inum = source_valid_time_list(fdesc, macro, &vlist);
		if ( inum < 1 )
			{
			if ( local )
				(void) fprintf(stderr, "[reset_fieldmod_vtime] No daily fields");
			else
				(void) fprintf(stderr, "[reset_fieldmod_vtime] No normal fields");
			(void) fprintf(stderr, " for source: \"%s %s\"",
					SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name));
			(void) fprintf(stderr, "   rtime: \"%s\"\n",
					SafeStr(fdesc->rtime));
			return FALSE;
			}
		/* Check for current valid time */
		for ( ii=0; ii<inum; ii++ )
			{
			if ( same(vtime, vlist[ii]) ) break;
			}
		if ( ii >= inum || --ii < 0 )
			{
			(void) fprintf(stderr, "[reset_fieldmod_rtime] No previous valid time");
			(void) fprintf(stderr, " for: \"%s %s  %s  %s\"\n",
					SafeStr(fdesc->sdef->name), SafeStr(fdesc->subdef->name),
					SafeStr(fdesc->rtime), SafeStr(vtime));
			inum = source_valid_time_list_free(&vlist, inum);
			return FALSE;
			}
		/* Set valid time from previous valid time */
		(void) safe_strcpy(vtime, vlist[ii]);
		inum = source_valid_time_list_free(&vlist, inum);
		return TRUE;
		}

	/* Error message for unrecognizable valid time modifier */
	(void) fprintf(stderr, "[reset_fieldmod_vtime] Unrecognizable");
	(void) fprintf(stderr, " valid time modifier: \"%s\"\n", SafeStr(buf));
	return FALSE;
	}

/**********************************************************************
 ***                                                                ***
 *** r e s e t _ f i e l d m o d _ a t t r i b                      ***
 ***                                                                ***
 *** return TRUE/FALSE from resetting attribute modifier            ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL			reset_fieldmod_attrib

	(
	STRING		buf,		/* string containing attribute modifier */
	STRING		attrib		/* string containing modified attribute */
	)

	{

	/* Identify attribute modifier */
	if ( !blank(buf) )
		{
		(void) safe_strcpy(attrib, buf);
		return TRUE;
		}

	/* Error message for missing attribute modifier */
	(void) fprintf(stderr,
		"[reset_fieldmod_attrib] Missing attribute modifier!\n");
	return FALSE;
	}

/**********************************************************************
 ***                                                                ***
 *** r e s e t _ f i e l d m o d _ a u n i t s                      ***
 ***                                                                ***
 *** return TRUE/FALSE from resetting attribute units modifier      ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL			reset_fieldmod_aunits

	(
	STRING		buf,		/* string containing attribute units modifier */
	STRING		aunits		/* string containing modified attribute units */
	)

	{
	FpaConfigUnitStruct	*udef;

	/* Identify attribute units from modifier */
	udef = identify_unit(buf);
	if ( NotNull(udef) )
		{
		(void) safe_strcpy(aunits, udef->name);
		return TRUE;
		}

	/* Error message for unacceptable attribute units modifier */
	(void) fprintf(stderr, "[reset_fieldmod_aunits] Unacceptable");
	(void) fprintf(stderr, " attribute units modifier: \"%s\"\n", SafeStr(buf));
	return FALSE;
	}

/**********************************************************************
 ***                                                                ***
 *** r e s e t _ f i e l d m o d _ x l o o k u p                    ***
 ***                                                                ***
 *** return TRUE/FALSE from resetting cross reference lookup        ***
 ***  modifier                                                      ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL			reset_fieldmod_xlookup

	(
	STRING		buf,		/* string containing attribute lookup modifier */
	STRING		xlookup		/* string containing modified attribute lookup */
	)

	{

	/* Identify attribute lookup modifier */
	if ( !blank(buf) )
		{
		(void) safe_strcpy(xlookup, buf);
		return TRUE;
		}

	/* Error message for unknown attribute lookup file */
	(void) fprintf(stderr,
		"[reset_fieldmod_xlookup] Missing lookup modifier!\n");
	return FALSE;
	}

/**********************************************************************
 ***                                                                ***
 *** r e s e t _ f i e l d m o d _ d e f v a l                      ***
 ***                                                                ***
 *** return TRUE/FALSE from resetting default attribute value       ***
 ***  modifier                                                      ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL			reset_fieldmod_defval

	(
	STRING		buf,		/* string containing default value modifier */
	float		*defval		/* modified default value */
	)

	{
	double	value;
	STRING	pbuf;

	/* Set return value */
	if ( NotNull(defval) ) *defval = 0.0;

	/* Extract default value from modifier */
	if ( !blank(buf) && NotNull(defval) )
		{
		value = strtod(buf, &pbuf);
		if ( blank(pbuf) )
			{
			*defval = (float) value;
			return TRUE;
			}
		}

	/* Error message for problems extracting default value from modifier */
	(void) fprintf(stderr, "[reset_fieldmod_defval] Problems extracting");
	(void) fprintf(stderr, " default value from modifier: \"%s\"\n",
		SafeStr(buf));
	return FALSE;
	}

/**********************************************************************
 ***                                                                ***
 *** r e s e t _ f i e l d m o d _ p r o x i m i t y                ***
 ***                                                                ***
 *** return TRUE/FALSE from resetting proximity to features         ***
 ***  modifier                                                      ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL			reset_fieldmod_proximity

	(
	STRING		buf,		/* string containing proximity modifier */
	float		*proximity	/* modified proximity to features */
	)

	{
	double	value;
	STRING	pbuf;

	/* Set return value */
	if ( NotNull(proximity) ) *proximity = 0.0;

	/* Extract proximity to features from modifier */
	if ( !blank(buf) && NotNull(proximity) )
		{
		value = strtod(buf, &pbuf);
		if ( blank(pbuf) )
			{
			*proximity = (float) value;
			return TRUE;
			}
		}

	/* Error message for problems extracting proximity from modifier */
	(void) fprintf(stderr, "[reset_fieldmod_proximity] Problems extracting");
	(void) fprintf(stderr, " proximity from modifier: \"%s\"\n", SafeStr(buf));
	return FALSE;
	}

/**********************************************************************
 ***                                                                ***
 *** r e s e t _ f i e l d m o d _ p u n i t s                      ***
 ***                                                                ***
 *** return TRUE/FALSE from resetting proximity units modifier      ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL			reset_fieldmod_punits

	(
	STRING		buf,		/* string containing proximity units modifier */
	STRING		punits		/* string containing modified proximity units */
	)

	{
	FpaConfigUnitStruct	*udef;

	/* Identify proximity units from modifier       */
	/*  and ensure units can be converted correctly */
	udef = identify_unit(buf);
	if ( NotNull(udef)
			&& convert_value(udef->name, 0.0, ProximityUnits, NullDouble) )
		{
		(void) safe_strcpy(punits, udef->name);
		return TRUE;
		}

	/* Error message for unacceptable proximity units modifier */
	(void) fprintf(stderr, "[reset_fieldmod_punits] Unacceptable");
	(void) fprintf(stderr, " proximity units modifier: \"%s\"\n", SafeStr(buf));
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Attribute lookup tables)                *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/**********************************************************************
 ***                                                                ***
 *** g e t _ l o o k u p _ t a b l e                                ***
 ***                                                                ***
 *** return pointer to LOOKUP_TABLE structure from reading lookup   ***
 ***  table of given name.                                          ***
 ***                                                                ***
 **********************************************************************/

static	LOOKUP_TABLE	*get_lookup_table

	(
	STRING		xlookup		/* lookup table name */
	)

	{
	int				ilook, iline;
	STRING			table_file;
	char			tbuf[MAX_BCHRS];
	char			*token1, *token2, *endtoken;
	double			dval;
	FILE			*table_fp;
	LOOKUP_TABLE	*LookTable;
	LOOKUP_LINE		*LookLine;

	/* First find the full path of the lookup file */
	table_file = get_file("lookups", xlookup);
	if ( blank(table_file) )
		{
		(void) fprintf(stderr,
			"[get_lookup_table] Cannot find attribute lookup file: \"%s\"\n",
			SafeStr(xlookup));
		return NullPtr(LOOKUP_TABLE *);
		}

	/* Search the list for the lookup file name */
	for ( ilook=0; ilook<NumLookup; ilook++ )
		{

		/* Return the lookup table from the list */
		if ( same(table_file, Lookups[ilook].label) )
			{
			return &Lookups[ilook];
			}
		}

	/* Otherwise, add another lookup table to the list */
	ilook     = NumLookup++;
	Lookups   = GETMEM(Lookups, LOOKUP_TABLE, NumLookup);
	LookTable = &Lookups[ilook];

	/* Initialize the new lookup table */
	LookTable->label       = strdup(table_file);
	LookTable->numlines    = 0;
	LookTable->lines       = NullPtr(LOOKUP_LINE *);
	LookTable->ismiss      = FALSE;
	LookTable->mline.key   = NullString;
	LookTable->mline.value = 0.0;
	LookTable->isdef       = FALSE;
	LookTable->dline.key   = NullString;
	LookTable->dline.value = 0.0;

	/* Open the lookup file */
	if ( IsNull(table_fp = fopen(table_file, "r")) )
		{
		(void) fprintf(stderr,
			"[get_lookup_table] Cannot open attribute lookup file: \"%s\"\n",
			SafeStr(table_file));
		return &Lookups[ilook];
		}

	/* Read the lookup file line by line */
	while ( NotNull( getvalidline(table_fp, tbuf, (size_t) MAX_BCHRS,
																Comment) ) )
		{

		/* Extract the lookup table parameters */
		token1 = strtok(tbuf, ":");
		(void) no_white(token1);

		token2 = strtok('\0', CommentOrEnd);
		(void) no_white(token2);

		/* Skip lines that contain no parameters ... comment lines! */
		if ( blank(token2) ) continue;

		/* Convert the value parameter */
		dval = strtod(token2, &endtoken);
		if ( endtoken != (token2 + strlen(token2)) )
			{
			(void) fprintf(stderr,
				"[get_lookup_table] Error with value \"%s\" in lookup file: \"%s\"\n",
				SafeStr(token2), SafeStr(table_file));
			continue;
			}

		/* Add the lookup table parameters to the default list */
		if ( same(token1, "*missing*") )
			{
			LookTable->ismiss = TRUE;
			LookLine          = &LookTable->mline;
			LookLine->key     = safe_strdup(FpaCblank);
			LookLine->value   = (float) dval;
			}

		/* Add the lookup table parameters to the default list */
		else if ( same(token1, "*default*") )
			{
			LookTable->isdef  = TRUE;
			LookLine          = &LookTable->dline;
			LookLine->key     = safe_strdup(FpaCblank);
			LookLine->value   = (float) dval;
			}

		/* Add the lookup table parameters to the table list */
		else
			{
			iline             = LookTable->numlines++;
			LookTable->lines  = GETMEM(LookTable->lines,
											LOOKUP_LINE, LookTable->numlines);
			LookLine          = &LookTable->lines[iline];
			LookLine->key     = safe_strdup(token1);
			LookLine->value   = (float) dval;
			}
		}

	/* Close the lookup table and return the current pointer */
	(void) fclose(table_fp);
	return &Lookups[ilook];
	}

/**********************************************************************
 ***                                                                ***
 *** m a t c h _ l o o k u p _ t a b l e                            ***
 ***                                                                ***
 *** return TRUE/FALSE from matching attribute value to entries in  ***
 ***  lookup table.                                                 ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL			match_lookup_table

	(
	STRING		xlookup,	/* lookup table name */
	STRING		string,		/* attribute value to match */
	float		*value		/* matching value from lookup table */
	)

	{
	int				ii;
	LOOKUP_TABLE	*xtable;

	/* Initialize output value */
	if ( NotNull(value) ) *value = 0.0;

	/* Get lookup table */
	xtable = get_lookup_table(xlookup);
	if ( IsNull(xtable) ) return FALSE;

	/* No string to match ... so check for special missing parameters */
	if ( blank(string) && xtable->ismiss )
		{

		/* Set the missing parameters */
		if ( NotNull(value) ) *value = xtable->mline.value;
		return TRUE;
		}

	/* Return if no string to match */
	else if ( blank(string) )
		{
		return FALSE;
		}

	/* Search each line of lookup table for exact match */
	for ( ii=0; ii<xtable->numlines; ii++ )
		{

		/* Match the "key" against the attribute "string" */
		if ( strstr(string, xtable->lines[ii].key) )
			{

			/* Set the matched parameters */
			if ( NotNull(value) ) *value = xtable->lines[ii].value;
			return TRUE;
			}
		}

	/* No match found ... so check for special default parameters */
	if ( xtable->isdef )
		{

		/* Set the default parameters */
		if ( NotNull(value) ) *value = xtable->dline.value;
		return TRUE;
		}

	/* Return FALSE if no match found ... and no default */
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Accessing Equation Defaults)            *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/**********************************************************************
 ***                                                                ***
 *** v a l i d _ u n i t s _ a n d _ e q u a t i o n                ***
 ***                                                                ***
 *** check accuracy of input units and equation                     ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL			valid_units_and_equation

	(
	STRING			inunit,		/* input equation units */
	STRING			inebuf		/* input equation buffer */
	)

	{
	FpaConfigUnitStruct		*udef;

	/* Error message for missing units or equation */
	if ( blank(inunit) || blank(inebuf) )
		{
		(void) fprintf(stderr, "[valid_units_and_equation] Missing");
		(void) fprintf(stderr, " units: \"%s\"", SafeStr(inunit));
		(void) fprintf(stderr, "   or equation: \"%s\"\n", SafeStr(inebuf));
		return FALSE;
		}

	/* Check that units can be identified */
	udef = identify_unit(inunit);
	if ( NotNull(udef) )
		{
		return TRUE;
		}

	/* Error message for unrecognizable units */
	else
		{
		(void) fprintf(stderr, "[valid_units_and_equation] Unrecognizable");
		(void) fprintf(stderr, " units: \"%s\"\n", SafeStr(inunit));
		return FALSE;
		}
	}

/**********************************************************************
 ***                                                                ***
 *** s a v e _ e q u a t i o n _ d e f a u l t s                    ***
 ***                                                                ***
 *** save global Equation Defaults to FpaEQUATION_DEFAULTS Object   ***
 ***                                                                ***
 **********************************************************************/

static	void			save_equation_defaults

	(
	FpaEQUATION_DEFAULTS	*olddef		/* local copy of original defaults */
	)

	{
	int			nn;

	/* Copy global Equation Defaults to FpaEQUATION_DEFAULTS Object */

	/* Copy information used in FLD_DESCRIPT structures */
	(void) safe_strcpy(olddef->path,      FpaEqtnDefs.path);
	(void) safe_strcpy(olddef->source,    FpaEqtnDefs.source);
	(void) safe_strcpy(olddef->subsource, FpaEqtnDefs.subsource);
	(void) safe_strcpy(olddef->rtime,     FpaEqtnDefs.rtime);
	(void) safe_strcpy(olddef->vtime,     FpaEqtnDefs.vtime);
	(void) safe_strcpy(olddef->lvl,       FpaEqtnDefs.lvl);
	(void) safe_strcpy(olddef->uprlvl,    FpaEqtnDefs.uprlvl);
	(void) safe_strcpy(olddef->lwrlvl,    FpaEqtnDefs.lwrlvl);

	/* Copy strings to use in generic equations */
	(void) safe_strcpy(olddef->genfld,    FpaEqtnDefs.genfld);
	(void) safe_strcpy(olddef->genmod,    FpaEqtnDefs.genmod);

	/* Copy information for point evaluations */
	olddef->pointeval = FpaEqtnDefs.pointeval;
	olddef->subgrid   = FpaEqtnDefs.subgrid;
	(void) copy_map_projection(&olddef->mprojRead, &FpaEqtnDefs.mprojRead);
	(void) copy_map_projection(&olddef->mprojOrig, &FpaEqtnDefs.mprojOrig);
	(void) copy_map_projection(&olddef->mprojEval, &FpaEqtnDefs.mprojEval);

	/* Allocate space and copy positions for evaluation */
	olddef->numposEval = FpaEqtnDefs.numposEval;
	olddef->posEval    = NullPointPtr;
	if ( olddef->numposEval > 0 )
		{
		olddef->posEval = GETMEM(olddef->posEval, POINT, olddef->numposEval);
		for ( nn=0; nn<olddef->numposEval; nn++ )
			(void) copy_point(olddef->posEval[nn], FpaEqtnDefs.posEval[nn]);
		}
	}

/**********************************************************************
 ***                                                                ***
 *** r e s t o r e _ e q u a t i o n _ d e f a u l t s              ***
 ***                                                                ***
 *** restore global Equation Defaults from FpaEQUATION_DEFAULTS     ***
 ***  Object                                                        ***
 ***                                                                ***
 **********************************************************************/

static	void			restore_equation_defaults

	(
	FpaEQUATION_DEFAULTS	*olddef		/* local copy of original defaults */
	)

	{

	/* Copy global Equation Defaults from FpaEQUATION_DEFAULTS Object */

	/* Copy information used in FLD_DESCRIPT structures */
	(void) safe_strcpy(FpaEqtnDefs.path,      olddef->path);
	(void) safe_strcpy(FpaEqtnDefs.source,    olddef->source);
	(void) safe_strcpy(FpaEqtnDefs.subsource, olddef->subsource);
	(void) safe_strcpy(FpaEqtnDefs.rtime,     olddef->rtime);
	(void) safe_strcpy(FpaEqtnDefs.vtime,     olddef->vtime);
	(void) safe_strcpy(FpaEqtnDefs.lvl,       olddef->lvl);
	(void) safe_strcpy(FpaEqtnDefs.uprlvl,    olddef->uprlvl);
	(void) safe_strcpy(FpaEqtnDefs.lwrlvl,    olddef->lwrlvl);

	/* Copy strings to use in generic equations */
	(void) safe_strcpy(FpaEqtnDefs.genfld,    olddef->genfld);
	(void) safe_strcpy(FpaEqtnDefs.genmod,    olddef->genmod);

	/* Copy information for point evaluations */
	FpaEqtnDefs.pointeval = olddef->pointeval;
	FpaEqtnDefs.subgrid   = olddef->subgrid;
	(void) copy_map_projection(&FpaEqtnDefs.mprojRead, &olddef->mprojRead);
	(void) copy_map_projection(&FpaEqtnDefs.mprojOrig, &olddef->mprojOrig);
	(void) copy_map_projection(&FpaEqtnDefs.mprojEval, &olddef->mprojEval);

	/* Deallocate space and reset pointer to positions for evaluation */
	if ( FpaEqtnDefs.posEval ) FREEMEM(FpaEqtnDefs.posEval);
	FpaEqtnDefs.numposEval = olddef->numposEval;
	FpaEqtnDefs.posEval    = olddef->posEval;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Endless Loop in Equations)              *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/**********************************************************************
 ***                                                                ***
 *** r e s e t _ e n d l e s s _ l o o p                            ***
 *** p u s h _ e n d l e s s _ l o o p                              ***
 *** p o p _ e n d l e s s _ l o o p                                ***
 *** c h e c k _ e n d l e s s _ l o o p                            ***
 ***                                                                ***
 *** set and check number of embedded equations                     ***
 ***                                                                ***
 **********************************************************************/

/* Storage location for number of embedded equations */
static	int		EndlessLoop  = 0;
static	STRING	*EndlessEqtn = NullStringList;

static	void			reset_endless_loop

	(
	)

	{

	/* Free list of equations */
	FREELIST(EndlessEqtn, EndlessLoop);

	/* Reset global number of embedded equations */
	EndlessLoop = 0;
	}

static	void			push_endless_loop

	(
	STRING			inebuf		/* input equation for field */
	)

	{

	/* Add to number of embedded equations */
	EndlessLoop++;

	/* Allocate space for another equation and save it */
	EndlessEqtn = GETMEM(EndlessEqtn, STRING, EndlessLoop);
	EndlessEqtn[EndlessLoop-1] = strdup(inebuf);
	}

static	void			pop_endless_loop

	(
	)

	{

	/* Return immediately if no list of equations */
	if ( !EndlessEqtn || EndlessLoop < 1 ) return;

	/* Free space used by present equation */
	FREEMEM(EndlessEqtn[EndlessLoop-1]);

	/* Subtract from number of embedded equations */
	EndlessLoop--;
	}

static	LOGICAL			check_endless_loop

	(
	STRING			inebuf		/* input equation for field */
	)

	{
	int		nn;

	/* Return TRUE if equation handler is trapped in an endless loop */
	for ( nn=0; nn<EndlessLoop; nn++ )
		{
		if ( same(inebuf, EndlessEqtn[nn]) )
			{
			(void) fprintf(stderr, "[check_endless_loop] Equation handler");
			(void) fprintf(stderr, " is trapped in an endless loop!\n");
			(void) fprintf(stderr, "                     Equation \"%s\"",
					SafeStr(inebuf));
			(void) fprintf(stderr, " has been called more than once!\n");
			return TRUE;
			}
		}

	/* Otherwise, add this equation to list and return FALSE */
	(void) push_endless_loop(inebuf);
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Testing static routines)                *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

#ifdef EQUATION_STANDALONE

/**********************************************************************
 *** routine to test remove_blanks                                  ***
 **********************************************************************/

static	void	test_remove_blanks

	(
	STRING		buf		/* string containing equation */
	)

	{
	(void) fprintf(stderr, "\nOriginal buffer: >%s<\n", buf);
	remove_blanks(buf);
	(void) fprintf(stderr, "Final buffer:    >%s<\n", buf);
	}

/**********************************************************************
 *** routine to test generic_expansion                              ***
 **********************************************************************/

static	void	test_generic_expansion

	(
	STRING		buf		/* string containing equation */
	)

	{
	char		xbuf[MAX_BCHRS];

	(void) fprintf(stderr, "\nOriginal buffer: >%s<\n", buf);
	if ( generic_expansion(buf, xbuf) )
		(void) fprintf(stderr, "Final buffer:    >%s<\n", xbuf);
	else
		(void) fprintf(stderr, "Original buffer cannot be expanded\n");
	}

/**********************************************************************
 *** routine to test get_fop and get_nop                            ***
 **********************************************************************/

static	void	test_get_fopnop

	(
	STRING		buf		/* string containing equation */
	)

	{
	char	rbuf[MAX_BCHRS], lbuf[MAX_BCHRS];
	char	*pfop, *pnop;

	(void) fprintf(stderr, "\nOriginal buffer: >%s<\n", buf);
	(void) fprintf(stderr, "     at: <%x>\n", buf);
	pfop = get_fop(buf, lbuf);
	(void) fprintf(stderr, "  Left side: >%s<\n", lbuf);
	(void) fprintf(stderr, "     at: <%x>\n", lbuf);
	if (*pfop != '\0')
		{
		(void) fprintf(stderr, "  First operator: %c\n", *pfop);
		(void) fprintf(stderr, "     at: <%x>\n", pfop);
		do
			{
			pnop = get_nop(pfop, buf, rbuf);
			(void) fprintf(stderr, "  Right side: >%s<\n", rbuf);
			(void) fprintf(stderr, "     at: <%x>\n", rbuf);
			if (*pnop != '\0')
				{
				(void) fprintf(stderr, "  Next operator: %c\n", *pnop);
				(void) fprintf(stderr, "     at: <%x>\n", pnop);
				}
			else
				{
				(void) fprintf(stderr, "  Next operator: Null\n");
				(void) fprintf(stderr, "     at: <%x>\n", pnop);
				}
			pfop = pnop;
			} while (*pfop != '\0');
		}
	else
		{
		(void) fprintf(stderr, "  First operator: Null\n");
		(void) fprintf(stderr, "     at: <%x>\n", pfop);
		}
	}

/**********************************************************************
 *** routine to test get_nsub                                       ***
 **********************************************************************/

static	void	test_get_nsub

	(
	STRING		buf		/* string containing equation */
	)

	{
	char	lbuf[MAX_BCHRS];
	char	*pbgn, *pnsub;

	(void) fprintf(stderr, "\nOriginal buffer: >%s<\n", buf);
	(void) fprintf(stderr, "     at: <%x>\n", buf);
	for ( pbgn=&buf[0]; *pbgn; )
		{
		pnsub = get_nsub(pbgn, buf, lbuf);
		(void) fprintf(stderr, "  Modifier string: >%s<\n", lbuf);
		(void) fprintf(stderr, "     at: <%x>\n", lbuf);
		if (*pnsub != '\0')
			{
			(void) fprintf(stderr, "  Next modifier character: %c\n", *pnsub);
			(void) fprintf(stderr, "     at: <%x>\n", pnsub);
			}
		else
			{
			(void) fprintf(stderr, "  Next modifier character: Null\n");
			(void) fprintf(stderr, "     at: <%x>\n", pnsub);
			}
		pbgn = pnsub;
		}
	}

/**********************************************************************
 *** routine to test get_modstrng                                   ***
 **********************************************************************/

static	void	test_get_modstrng

	(
	STRING		buf		/* string containing field name and modifiers */
	)

	{
	char	lbuf[MAX_BCHRS], rbuf[MAX_BCHRS];

	(void) fprintf(stderr, "\nOriginal buffer: >%s<\n", buf);
	(void) fprintf(stderr, "     at: <%x>\n", buf);
	if ( get_modstrng(buf, lbuf, rbuf) )
		{
		(void) fprintf(stderr, "  Field name: >%s<\n", lbuf);
		(void) fprintf(stderr, "     at: <%x>\n", lbuf);
		(void) fprintf(stderr, "  Field modifiers: >%s<\n", rbuf);
		(void) fprintf(stderr, "     at: <%x>\n", rbuf);
		}
	else
		(void) fprintf(stderr, "  Error in buffer containing field string\n");
	}

/**********************************************************************
 *** routine to test reset_fieldmod                                 ***
 **********************************************************************/

static	void	test_reset_fieldmod

	(
	STRING		buf		/* string containing field modifier */
	)

	{
	char	lvl[MAX_NCHRS], uprlvl[MAX_NCHRS];
	char	lwrlvl[MAX_NCHRS];
	char	source[MAX_NCHRS], subsource[MAX_NCHRS];
	char	rtime[MAX_NCHRS], vtime[MAX_NCHRS];
	char	attrib[MAX_BCHRS], aunits[MAX_BCHRS];
	char	xlookup[MAX_BCHRS], punits[MAX_BCHRS];
	float	defval, proximity;

	(void) fprintf(stderr, "\nModifier string: >%s<\n", buf);
	(void) fprintf(stderr, "     at: <%x>\n", buf);
	(void) init_fieldmod(lvl, uprlvl, lwrlvl, source, subsource,
			rtime, vtime, attrib, aunits, xlookup, &defval, &proximity, punits);
	(void) fprintf(stderr, "  lvl: %s  uprlvl: %s", lvl, uprlvl);
	(void) fprintf(stderr, "  lwrlvl: %s\n    source: %s %s",
			lwrlvl, source, subsource);
	(void) fprintf(stderr, "  rtime: %s  vtime: %s\n", rtime, vtime);
	(void) fprintf(stderr, "    attrib: %s  aunits: %s  xlookup: %s",
			attrib, aunits, xlookup);
	(void) fprintf(stderr, "  defval: %.2f  proximity: %.2f  punits: %s\n",
			defval, proximity, punits);
	if ( reset_fieldmod(buf, lvl, uprlvl, lwrlvl, source, subsource,
			rtime, vtime, attrib, aunits, xlookup, &defval, &proximity, punits) )
		{
		(void) fprintf(stderr, "  lvl: %s  uprlvl: %s", lvl, uprlvl);
		(void) fprintf(stderr, "  lwrlvl: %s\n    source: %s %s",
				lwrlvl, source, subsource);
		(void) fprintf(stderr, "  rtime: %s  vtime: %s\n", rtime, vtime);
		(void) fprintf(stderr, "    attrib: %s  aunits: %s  xlookup: %s",
				attrib, aunits, xlookup);
		(void) fprintf(stderr, "  defval: %.2f  proximity: %.2f  punits: %s\n",
				defval, proximity, punits);
		}
	}

/**********************************************************************
 *** routine to test evaluate_unary                                 ***
 **********************************************************************/

static	void	test_evaluate_unary

	(
	STRING		buf		/* string containing equation */
	)

	{
	FpaEQTN_DATA	*pfield;

	(void) fprintf(stderr, "\nOriginal buffer: >%s<\n", buf);
	(void) fprintf(stderr, "     at: <%x>\n", buf);
	pfield = evaluate_unary(buf);
	(void) fprintf(stderr, "Final buffer:    >%s<\n", buf);
	(void) fprintf(stderr, "     at: <%x>\n", buf);

	if ( NotNull(pfield) )
		{
		(void) fprintf(stderr, "  Type of unary Object: %d\n", pfield->Type);
		(void) fprintf(stderr, "     at: <%x>\n", pfield);
		(void) debug_scalar(&pfield->Data.scalr);
		}
	else
		{
		(void) fprintf(stderr, "  Type of unary Object: 0\n");
		(void) fprintf(stderr, "     at: <%x>\n\n", pfield);
		}
	(void) free_eqtn_data(pfield);
	}

/**********************************************************************
 *** routine to test evaluate_name                                  ***
 **********************************************************************/

static	void	test_evaluate_name

	(
	STRING		buf		/* string containing constant or field name */
	)

	{
	FpaEQTN_DATA	*pfield, *pgrid, *pspline;

	(void) fprintf(stderr, "\nOriginal buffer: >%s<\n", buf);
	(void) fprintf(stderr, "     at: <%x>\n", buf);

	pfield = evaluate_name(buf);
	if ( NotNull(pfield) )
		{
		(void) fprintf(stderr, "  Data from Object type: %d\n", pfield->Type);
		(void) fprintf(stderr, "     at: <%x>\n", pfield);
		(void) debug_eqtn_data(pfield);

		/* Also convert SPLINE Objects to GRID Objects for testing */
		if ( pfield->Type == FpaEQT_Spline )
			{
			pgrid = convert_eqtn_data(FpaEQT_Grid, pfield);
			(void) fprintf(stderr, "  Same data from");
			(void) fprintf(stderr, " Object type: %d\n", pgrid->Type);
			(void) fprintf(stderr, "     at: <%x>\n", pgrid);
			(void) debug_eqtn_data(pgrid);
			pspline = convert_eqtn_data(FpaEQT_Spline, pgrid);
			(void) fprintf(stderr, "  Original data\n");
			(void) fprintf(stderr, "     at: <%x>\n", pspline);
			(void) debug_eqtn_data(pspline);
			(void) free_eqtn_data(pgrid);
			(void) free_eqtn_data(pspline);
			}
		}
	else
		{
		(void) fprintf(stderr, "  Data from Object type: 0\n");
		(void) fprintf(stderr, "     at: <%x>\n", pfield);
		}
	(void) free_eqtn_data(pfield);
	}

/**********************************************************************
 *** routine to test evaluate_operator                              ***
 **********************************************************************/

static	void	test_evaluate_operator

	(
	STRING		lbuf,	/* string containing first constant or field name */
	STRING		oper,	/* string containing operator */
	STRING		rbuf	/* string containing second constant or field name */
	)

	{
	FpaEQTN_DATA	*pfield, *plfld, *prfld;

	(void) fprintf(stderr, "\nEvaluation of:  >%s<  %s  >%s<\n",
			lbuf, oper, rbuf);

	plfld = evaluate_name(lbuf);
	if ( NotNull(pfield) )
		{
		(void) fprintf(stderr, "  Left side Object type: %d\n", plfld->Type);
		(void) fprintf(stderr, "     at: <%x>\n", plfld);
		}
	else
		{
		(void) fprintf(stderr, "  Left side Object type: 0\n");
		(void) fprintf(stderr, "     at: <%x>\n", plfld);
		}

	prfld = evaluate_name(rbuf);
	if ( NotNull(prfld) )
		{
		(void) fprintf(stderr, "  Right side Object type: %d\n", prfld->Type);
		(void) fprintf(stderr, "     at: <%x>\n", prfld);
		}
	else
		{
		(void) fprintf(stderr, "  Right side Object type: 0\n");
		(void) fprintf(stderr, "     at: <%x>\n", prfld);
		}

	pfield = evaluate_operator(plfld, oper, prfld);
	if ( NotNull(pfield) )
		{
		(void) fprintf(stderr, "  Result Object type: %d\n", pfield->Type);
		(void) fprintf(stderr, "     at: <%x>\n", pfield);
		(void) debug_eqtn_data(pfield);
		}
	else
		{
		(void) fprintf(stderr, "  Result Object type: 0\n");
		(void) fprintf(stderr, "     at: <%x>\n", pfield);
		}
	(void) free_eqtn_data(plfld);
	(void) free_eqtn_data(prfld);
	(void) free_eqtn_data(pfield);
	}

/**********************************************************************
 *** routine to test evaluate_function                              ***
 **********************************************************************/

static	void	test_evaluate_function

	(
	STRING		lbuf,	/* string containing function */
	STRING		rbuf	/* string containing constants or field names */
						/*  separated by commas                       */
	)

	{
	FpaEQTN_DATA	*pfield;

	(void) fprintf(stderr, "\nFunction:  %s[]  of  >%s<\n", lbuf, rbuf);
	pfield = evaluate_function(lbuf, rbuf);
	if ( NotNull(pfield) )
		{
		(void) fprintf(stderr, "  Result Object type: %d\n", pfield->Type);
		(void) fprintf(stderr, "     at: <%x>\n", pfield);
		(void) debug_eqtn_data(pfield);
		}
	else
		{
		(void) fprintf(stderr, "  Result Object type: 0\n");
		(void) fprintf(stderr, "     at: <%x>\n", pfield);
		}
	(void) free_eqtn_data(pfield);
	}

/**********************************************************************
 *** routine to test evaluate_equation                              ***
 **********************************************************************/

static	void	test_evaluate_equation

	(
	STRING		buf		/* string containing equation */
	)

	{
	FpaEQTN_DATA	*pfield;

	(void) fprintf(stderr, "\nOriginal buffer: >%s<\n", buf);
	(void) fprintf(stderr, "     at: <%x>\n", buf);
	pfield = evaluate_equation(buf);
	if ( NotNull(pfield) )
		{
		(void) fprintf(stderr, "  Result Object type: %d\n", pfield->Type);
		(void) fprintf(stderr, "     at: <%x>\n", pfield);
		(void) debug_eqtn_data(pfield);
		}
	else
		{
		(void) fprintf(stderr, "  Result Object type: 0\n");
		(void) fprintf(stderr, "     at: <%x>\n", pfield);
		}
	(void) free_eqtn_data(pfield);
	}

/**********************************************************************
 *** main routine to test all static routines                       ***
 **********************************************************************/

int		main

(
int			argc,
STRING		*argv
)

{
int			nsetup;
STRING		setupfile, *setuplist, pathname;
MAP_PROJ	*mproj;
STRING		rtime, vtime;
POINT		pos;
char		buf[MAX_BCHRS];
char		lbuf[MAX_BCHRS], rbuf[MAX_BCHRS], oper[MAX_NCHRS];

/* Set Defaults for EQUATION_STANDALONE */

/*  ... First set the default output units */
(void) setvbuf(stdout, NullString, _IOLBF, 0);
(void) setvbuf(stderr, NullString, _IOLBF, 0);

/*  ... Next do the Setup for FPA applications */
(void) fpalib_license(FpaAccessLib);
setupfile = strdup(argv[1]);
(void) fprintf(stdout, "Setup File: %s\n", setupfile);
nsetup = setup_files(setupfile, &setuplist);
if ( !define_setup(nsetup, setuplist) )
	{
	(void) fprintf(stderr, " Fatal problem with Setup File: %s\n", setupfile);
	return -1;
	}

/* ... Next set Default Map Projection */
mproj = get_target_map();
if ( IsNull(mproj) )
	{
	(void) fprintf(stderr, "Fatal problem with Default Map Projection\n");
	return -1;
	}

dprintf(stdout, "\n\nBasemap  olat: %f", mproj->definition.olat);
dprintf(stdout, "  olon: %f", mproj->definition.olon);
dprintf(stdout, "  lref: %f\n", mproj->definition.lref);
dprintf(stdout, "         xorg: %f", mproj->definition.xorg);
dprintf(stdout, "  yorg: %f\n", mproj->definition.yorg);
dprintf(stdout, "         xlen: %f", mproj->definition.xlen);
dprintf(stdout, "  ylen: %f", mproj->definition.ylen);
dprintf(stdout, "  units: %f\n", mproj->definition.units);

dprintf(stdout, "\nGrid definition  nx: %d", mproj->grid.nx);
dprintf(stdout, "  ny: %d", mproj->grid.ny);
dprintf(stdout, "  gridlen: %f", mproj->grid.gridlen);
dprintf(stdout, "  units: %f\n", mproj->grid.units);

FpaEqtnDefs.mprojEval = *mproj;

/*  ... Next set Default Pathname (set to the Home directory) */
pathname = home_directory();
(void) strcpy(FpaEqtnDefs.path, pathname);

/*  ... Next set Default Source, Run Time, and Valid Time */
(void) strcpy(FpaEqtnDefs.source,    "fem");
(void) strcpy(FpaEqtnDefs.subsource, "");
rtime = strdup(build_tstamp(1991, 238, 12, 0, FALSE, FALSE));
(void) strcpy(FpaEqtnDefs.rtime, rtime);
vtime = strdup(build_tstamp(1991, 238, 12, 0, FALSE, FALSE));
(void) strcpy(FpaEqtnDefs.vtime, vtime);

/*  ... Next set Default Levels */
(void) strcpy(FpaEqtnDefs.lvl, "sfc");
(void) strcpy(FpaEqtnDefs.uprlvl, "500");
(void) strcpy(FpaEqtnDefs.lwrlvl, "700");

/*  ... Next set buffers for Default generic field/modifier strings */
(void) strcpy(FpaEqtnDefs.genfld, "ugp");
(void) strcpy(FpaEqtnDefs.genmod, "mspectral");

/*  ... Next set Default position evaluation parameters */
FpaEqtnDefs.pointeval = FALSE;
FpaEqtnDefs.numposEval = 2;
FpaEqtnDefs.posEval = GETMEM(FpaEqtnDefs.posEval, POINT, FpaEqtnDefs.numposEval);
pos[X] = 0.0;			pos[Y] = 0.0;
(void) copy_point(FpaEqtnDefs.posEval[0], pos);
pos[X] = 1000.0;		pos[Y] = 1000.0;
(void) copy_point(FpaEqtnDefs.posEval[1], pos);

/* Testing for remove_blanks */
(void) fprintf(stderr, "\n\n ***Testing for remove_blanks***\n");
(void) strcpy(buf, " abc");						test_remove_blanks(buf);
(void) strcpy(buf, " a bc ");					test_remove_blanks(buf);
(void) strcpy(buf, " abc<700, t-12, xxx>  ");	test_remove_blanks(buf);

/* Testing for generic_expansion */
(void) fprintf(stderr, "\n\n ***Testing for generic_expansion***\n");
(void) strcpy(buf, "@ - @<r-12, v-12>");		test_generic_expansion(buf);
(void) strcpy(buf, "@<r-12, v-12>");			test_generic_expansion(buf);
(void) strcpy(buf, "@ - @<@diff>");				test_generic_expansion(buf);
(void) strcpy(buf, "@ - @*@<r-12, v-12>");		test_generic_expansion(buf);
(void) strcpy(buf, "@ - @*@-@[@]");				test_generic_expansion(buf);
(void) strcpy(buf, "@*@-@[@,@<r-12>,tt<850>]");	test_generic_expansion(buf);
(void) strcpy(buf, "@*@+@<r-12, v-12>");		test_generic_expansion(buf);
(void) strcpy(buf, "@*(@+@<r-12, v-12>)");		test_generic_expansion(buf);

/* Testing for get_fop and get_nop */
(void) fprintf(stderr, "\n\n ***Testing for get_fop and get_nop***\n");
(void) strcpy(buf, "abc");						test_get_fopnop(buf);
(void) strcpy(buf, "abc+xyz");					test_get_fopnop(buf);
(void) strcpy(buf, "(abc+xyz)");				test_get_fopnop(buf);
(void) strcpy(buf, "abc+xyz-lmn");				test_get_fopnop(buf);
(void) strcpy(buf, "abc+xyz*lmn");				test_get_fopnop(buf);
(void) strcpy(buf, "a*b^2+c");					test_get_fopnop(buf);
(void) strcpy(buf, "a*b + c");					test_get_fopnop(buf);
(void) strcpy(buf, "ddd[rem,abc]*c");			test_get_fopnop(buf);
(void) strcpy(buf, "-abc");						test_get_fopnop(buf);
(void) strcpy(buf, "abc+-xyz");					test_get_fopnop(buf);
(void) strcpy(buf, "abc+xyz--lmn");				test_get_fopnop(buf);
(void) strcpy(buf, "abc+xyz*-lmn");				test_get_fopnop(buf);
(void) strcpy(buf, "xyz*-lmn");					test_get_fopnop(buf);
(void) strcpy(buf, "-(abc+xyz)");				test_get_fopnop(buf);
(void) strcpy(buf, "a*b^-5+c");					test_get_fopnop(buf);
(void) strcpy(buf, "b^-5+c");					test_get_fopnop(buf);
(void) strcpy(buf, "a^b^-5+c");					test_get_fopnop(buf);
(void) strcpy(buf, "a+5.3e-3");					test_get_fopnop(buf);
(void) strcpy(buf, "3.0e-3");					test_get_fopnop(buf);
(void) strcpy(buf, "3.e-3");					test_get_fopnop(buf);
(void) strcpy(buf, "abc+xyz-*-lmn");			test_get_fopnop(buf);

/* Testing for get_nsub */
(void) fprintf(stderr, "\n\n ***Testing for get_nsub***\n");
(void) strcpy(buf, "abc,tt500,xxx");			test_get_nsub(buf);
(void) strcpy(buf, "abc,,tt500,xxx,");			test_get_nsub(buf);
(void) strcpy(buf, " abc, tt500, xxx");			test_get_nsub(buf);
(void) strcpy(buf, "d*e,f+ g,-pow[xx,Pi^2.0],hr850");
												test_get_nsub(buf);

/* Testing for get_modstrng */
(void) fprintf(stderr, "\n\n ***Testing for get_modstrng***\n");
(void) strcpy(buf, "tt<500,<xxx>");				test_get_modstrng(buf);
(void) strcpy(buf, "tt<500,xxx>>");				test_get_modstrng(buf);
(void) strcpy(buf, "tt><500,xxx>");				test_get_modstrng(buf);
(void) strcpy(buf, "tt<500, xxx>");				test_get_modstrng(buf);
(void) strcpy(buf, "tt500");					test_get_modstrng(buf);
(void) strcpy(buf, "<500, xxx>");				test_get_modstrng(buf);
(void) strcpy(buf, "<500, xxx><yyy, 250>");		test_get_modstrng(buf);
(void) strcpy(buf, "tt<500, xxx><yyy, 250>");	test_get_modstrng(buf);

/* Testing for reset_fieldmod */
(void) fprintf(stderr, "\n\n ***Testing for reset_fieldmod***\n");
(void) strcpy(buf, "850");						test_reset_fieldmod(buf);
(void) strcpy(buf, "850-1000");					test_reset_fieldmod(buf);
(void) strcpy(buf, "851K");						test_reset_fieldmod(buf);
(void) strcpy(buf, "850-700");					test_reset_fieldmod(buf);
(void) strcpy(buf, "800");						test_reset_fieldmod(buf);
(void) strcpy(buf, "u850");						test_reset_fieldmod(buf);
(void) strcpy(buf, "u851K");					test_reset_fieldmod(buf);
(void) strcpy(buf, "l850");						test_reset_fieldmod(buf);
(void) strcpy(buf, "l851K");					test_reset_fieldmod(buf);
(void) strcpy(buf, "mspectral");				test_reset_fieldmod(buf);
(void) strcpy(buf, "msv9");						test_reset_fieldmod(buf);
(void) strcpy(buf, "mSpectrl");					test_reset_fieldmod(buf);
(void) strcpy(buf, "mtest");					test_reset_fieldmod(buf);
(void) strcpy(buf, "r06");						test_reset_fieldmod(buf);
(void) strcpy(buf, "r+06");						test_reset_fieldmod(buf);
(void) strcpy(buf, "r-06");						test_reset_fieldmod(buf);
(void) strcpy(buf, "r-n6");						test_reset_fieldmod(buf);
(void) strcpy(buf, "r1992:236:00");				test_reset_fieldmod(buf);
(void) strcpy(buf, "v06");						test_reset_fieldmod(buf);
(void) strcpy(buf, "v+12");						test_reset_fieldmod(buf);
(void) strcpy(buf, "v-12");						test_reset_fieldmod(buf);
(void) strcpy(buf, "v-x2");						test_reset_fieldmod(buf);
(void) strcpy(buf, "v*12");						test_reset_fieldmod(buf);
(void) strcpy(buf, "v1992:234:18");				test_reset_fieldmod(buf);
(void) strcpy(buf, "$upper");					test_reset_fieldmod(buf);
(void) strcpy(buf, "$lower");					test_reset_fieldmod(buf);
(void) strcpy(buf, "$upr");						test_reset_fieldmod(buf);
(void) strcpy(buf, "ANY");						test_reset_fieldmod(buf);
(void) strcpy(buf, "msl");						test_reset_fieldmod(buf);
(void) strcpy(buf, "mdonelan");					test_reset_fieldmod(buf);
(void) strcpy(buf, "mdonelan:");				test_reset_fieldmod(buf);
(void) strcpy(buf, "mdonelan:lake_ontario");	test_reset_fieldmod(buf);
(void) strcpy(buf, "mdoneln:lake_ontario");		test_reset_fieldmod(buf);
(void) strcpy(buf, "!");						test_reset_fieldmod(buf);
(void) strcpy(buf, "rcurrent");					test_reset_fieldmod(buf);
(void) strcpy(buf, "rprevious");				test_reset_fieldmod(buf);
rtime = strdup(build_tstamp(1991, 222, 00, 0, FALSE, FALSE));
(void) strcpy(FpaEqtnDefs.rtime, rtime);
(void) strcpy(buf, "rcurrent");					test_reset_fieldmod(buf);
(void) strcpy(buf, "rprevious");				test_reset_fieldmod(buf);
(void) strcpy(FpaEqtnDefs.rtime, "");
(void) strcpy(buf, "rcurrent");					test_reset_fieldmod(buf);
(void) strcpy(buf, "rprevious");				test_reset_fieldmod(buf);
rtime = strdup(build_tstamp(1991, 238, 12, 0, FALSE, FALSE));
(void) strcpy(FpaEqtnDefs.rtime, rtime);
(void) strcpy(buf, "vprevious");				test_reset_fieldmod(buf);
vtime = strdup(build_tstamp(1991, 239, 12, 0, FALSE, FALSE));
(void) strcpy(FpaEqtnDefs.vtime, vtime);
(void) strcpy(buf, "vprevious");				test_reset_fieldmod(buf);
vtime = strdup(build_tstamp(1991, 239, 12, 0, TRUE, FALSE));
(void) strcpy(FpaEqtnDefs.vtime, vtime);
(void) strcpy(buf, "vprevious");				test_reset_fieldmod(buf);
vtime = strdup(build_tstamp(1991, 238, 12, 0, FALSE, FALSE));
(void) strcpy(FpaEqtnDefs.vtime, vtime);
(void) strcpy(buf, "acloud_top");				test_reset_fieldmod(buf);
(void) strcpy(buf, "xcloud_top_xref");			test_reset_fieldmod(buf);
(void) strcpy(buf, "d-57.3");					test_reset_fieldmod(buf);
(void) strcpy(buf, "d	");						test_reset_fieldmod(buf);
(void) strcpy(buf, "dunknown");					test_reset_fieldmod(buf);
(void) strcpy(buf, "p50");						test_reset_fieldmod(buf);
(void) strcpy(buf, "p50miles");					test_reset_fieldmod(buf);
(void) strcpy(buf, "p5.0e2");					test_reset_fieldmod(buf);
(void) strcpy(buf, "qmi");						test_reset_fieldmod(buf);
(void) strcpy(buf, "qm");						test_reset_fieldmod(buf);

/* Testing for evaluate_unary */
(void) fprintf(stderr, "\n\n ***Testing for evaluate_unary***\n");
(void) strcpy(buf, "abc");						test_evaluate_unary(buf);
(void) strcpy(buf, "+abc");						test_evaluate_unary(buf);
(void) strcpy(buf, "-abc");						test_evaluate_unary(buf);

/* Testing for evaluate_name */
(void) fprintf(stderr, "\n\n ***Testing for evaluate_name***\n");
(void) strcpy(buf, "-2.5");						test_evaluate_name(buf);
(void) strcpy(buf, "PI");						test_evaluate_name(buf);
(void) strcpy(buf, "?PI");						test_evaluate_name(buf);
(void) strcpy(buf, "tz850");					test_evaluate_name(buf);
(void) strcpy(buf, "tt975");					test_evaluate_name(buf);
(void) strcpy(buf, "tt850");					test_evaluate_name(buf);
(void) strcpy(buf, "ttsfc");					test_evaluate_name(buf);
(void) strcpy(buf, "ww700");					test_evaluate_name(buf);
(void) strcpy(buf, "lat");						test_evaluate_name(buf);
(void) strcpy(buf, "sin");						test_evaluate_name(buf);
(void) strcpy(buf, "gz850");					test_evaluate_name(buf);
(void) strcpy(buf, "ug850");					test_evaluate_name(buf);
(void) strcpy(buf, "vg850");					test_evaluate_name(buf);
(void) strcpy(buf, "sflux");					test_evaluate_name(buf);
(void) strcpy(buf, "maxtmp");					test_evaluate_name(buf);
vtime = strdup(build_tstamp(1991, 238, 14, 0, TRUE, FALSE));
(void) strcpy(FpaEqtnDefs.vtime, vtime);
(void) strcpy(buf, "maxtmp");					test_evaluate_name(buf);
vtime = strdup(build_tstamp(1991, 238, 12, 0, FALSE, FALSE));
(void) strcpy(FpaEqtnDefs.vtime, vtime);

/* Testing for evaluate_operator */
(void) fprintf(stderr, "\n\n ***Testing for evaluate_operator***\n");
(void) strcpy(lbuf, "2.5");		(void) strcpy(rbuf, "-2.5");
(void) strcpy(oper, "*");			test_evaluate_operator(lbuf, oper, rbuf);
(void) strcpy(lbuf, "-2.5");	(void) strcpy(rbuf, "ww700");
(void) strcpy(oper, "*");			test_evaluate_operator(lbuf, oper, rbuf);
(void) strcpy(lbuf, "tt850");	(void) strcpy(rbuf, "ww700");
(void) strcpy(oper, "*");			test_evaluate_operator(lbuf, oper, rbuf);
(void) strcpy(lbuf, "tt850");	(void) strcpy(rbuf, "2.5");
(void) strcpy(oper, "*");			test_evaluate_operator(lbuf, oper, rbuf);
(void) strcpy(lbuf, "2.5");		(void) strcpy(rbuf, "-2.5");
(void) strcpy(oper, "/");			test_evaluate_operator(lbuf, oper, rbuf);
(void) strcpy(lbuf, "-2.5");	(void) strcpy(rbuf, "ww700");
(void) strcpy(oper, "/");			test_evaluate_operator(lbuf, oper, rbuf);
(void) strcpy(lbuf, "tt850");	(void) strcpy(rbuf, "ww700");
(void) strcpy(oper, "/");			test_evaluate_operator(lbuf, oper, rbuf);
(void) strcpy(lbuf, "tt850");	(void) strcpy(rbuf, "2.5");
(void) strcpy(oper, "/");			test_evaluate_operator(lbuf, oper, rbuf);
(void) strcpy(lbuf, "2.5");		(void) strcpy(rbuf, "-2.5");
(void) strcpy(oper, "^");			test_evaluate_operator(lbuf, oper, rbuf);
(void) strcpy(lbuf, "2.5");		(void) strcpy(rbuf, "ww700");
(void) strcpy(oper, "^");			test_evaluate_operator(lbuf, oper, rbuf);
(void) strcpy(lbuf, "tt850");	(void) strcpy(rbuf, "ww700");
(void) strcpy(oper, "^");			test_evaluate_operator(lbuf, oper, rbuf);
(void) strcpy(lbuf, "tt850");	(void) strcpy(rbuf, "2.5");
(void) strcpy(oper, "^");			test_evaluate_operator(lbuf, oper, rbuf);
(void) strcpy(lbuf, "2.5");		(void) strcpy(rbuf, "-2.5");
(void) strcpy(oper, "+");			test_evaluate_operator(lbuf, oper, rbuf);
(void) strcpy(lbuf, "-2.5");	(void) strcpy(rbuf, "ww700");
(void) strcpy(oper, "+");			test_evaluate_operator(lbuf, oper, rbuf);
(void) strcpy(lbuf, "tt850");	(void) strcpy(rbuf, "ww700");
(void) strcpy(oper, "+");			test_evaluate_operator(lbuf, oper, rbuf);
(void) strcpy(lbuf, "tt850");	(void) strcpy(rbuf, "2.5");
(void) strcpy(oper, "+");			test_evaluate_operator(lbuf, oper, rbuf);
(void) strcpy(lbuf, "2.5");		(void) strcpy(rbuf, "-2.5");
(void) strcpy(oper, "-");			test_evaluate_operator(lbuf, oper, rbuf);
(void) strcpy(lbuf, "-2.5");	(void) strcpy(rbuf, "ww700");
(void) strcpy(oper, "-");			test_evaluate_operator(lbuf, oper, rbuf);
(void) strcpy(lbuf, "tt850");	(void) strcpy(rbuf, "ww700");
(void) strcpy(oper, "-");			test_evaluate_operator(lbuf, oper, rbuf);
(void) strcpy(lbuf, "tt850");	(void) strcpy(rbuf, "2.5");
(void) strcpy(oper, "-");			test_evaluate_operator(lbuf, oper, rbuf);

/* Testing for evaluate_function */
(void) fprintf(stderr, "\n\n ***Testing for evaluate_function***\n");
(void) strcpy(lbuf, "lat");		(void) strcpy(rbuf, "");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "lat");		(void) strcpy(rbuf, "2.5");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "sin");		(void) strcpy(rbuf, "PI");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "sin");		(void) strcpy(rbuf, "lat[]*RAD");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "sin");		(void) strcpy(rbuf, "lon[]*RAD");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "sqrt");	(void) strcpy(rbuf, "");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "sqrt");	(void) strcpy(rbuf, "gz850");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "pow");		(void) strcpy(rbuf, "gz850,0.5");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "ddx");		(void) strcpy(rbuf, "ttsfc");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "ddy");		(void) strcpy(rbuf, "gz850");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "ddx");		(void) strcpy(rbuf, "ug850");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "ddy");		(void) strcpy(rbuf, "vg850");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "laplc");	(void) strcpy(rbuf, "ttsfc");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "advct");	(void) strcpy(rbuf, "tt850,ug850,vg850");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "divrg");	(void) strcpy(rbuf, "ug850,vg850");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "svprs");	(void) strcpy(rbuf, "tt850");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "max");		(void) strcpy(rbuf, "ww700,0");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "min");		(void) strcpy(rbuf, "ww700,0");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "max");		(void) strcpy(rbuf, "ww700,ttsfc-300");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "min");		(void) strcpy(rbuf, "ww700,ttsfc-300");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "min");		(void) strcpy(rbuf, "ww700,,ttsfc-300,");
test_evaluate_function(lbuf, rbuf);

/* Testing for evaluate_equation */
(void) fprintf(stderr, "\n\n ***Testing for evaluate_equation***\n");
(void) strcpy(buf, "(lat[]*RAD)");
test_evaluate_equation(buf);
(void) strcpy(buf, "lat[]*RAD+tt850*ww700");
test_evaluate_equation(buf);
(void) strcpy(buf, "-2.5*ddx(ttsfc)");
test_evaluate_equation(buf);
(void) strcpy(buf, "-2.5*ddx[ttsfc]");
test_evaluate_equation(buf);
(void) strcpy(buf, "-2.5*ddx[tt<sfc>*PI]");
test_evaluate_equation(buf);
(void) strcpy(buf, "ttsfc<u850,l1000,,mFEM><r-396,v36>");
test_evaluate_equation(buf);
(void) strcpy(buf, "ttsfc<mFEM,r-396,v36>+ttsfc");
test_evaluate_equation(buf);
(void) strcpy(buf, "12hrdiff<@ughght<@gz850,v+12>>");
test_evaluate_equation(buf);
(void) strcpy(buf, "12hrdiff<@ughght<@gz850>><v+12>");
test_evaluate_equation(buf);
(void) strcpy(buf, "12hrdiff<@ughght<@gz850>,v+12>");
test_evaluate_equation(buf);
(void) strcpy(buf, "12hrdiff<@ughght<@th851K>,v+12>");
test_evaluate_equation(buf);
(void) strcpy(buf, "12hrdiff<@ughght<@th>,v+12,851K>+th");
test_evaluate_equation(buf);
(void) strcpy(buf, "tgrd");
test_evaluate_equation(buf);
(void) strcpy(buf, "tgrd<!>");
test_evaluate_equation(buf);
(void) strcpy(buf, "lvlprs[]");
test_evaluate_equation(buf);
(void) strcpy(buf, "uprprs[]");
test_evaluate_equation(buf);
(void) strcpy(buf, "lwrprs[]");
test_evaluate_equation(buf);

/* Special testing for timeseries */
(void) strcpy(buf, "dz850<v+12>");
test_evaluate_equation(buf);
(void) strcpy(FpaEqtnDefs.source,    "interp");
(void) strcpy(FpaEqtnDefs.subsource, "");
vtime = strdup(build_tstamp(1991, 239, 01, 00, FALSE, FALSE));
(void) strcpy(FpaEqtnDefs.vtime, vtime);
(void) strcpy(buf, "ddt[p<msl,v+12>*temp<sfc>]");
test_evaluate_equation(buf);
vtime = strdup(build_tstamp(1991, 239, 00, 00, FALSE, FALSE));
(void) strcpy(FpaEqtnDefs.vtime, vtime);
(void) strcpy(buf, "dp<msl>");
test_evaluate_equation(buf);
vtime = strdup(build_tstamp(1991, 240, 12, 00, FALSE, FALSE));
(void) strcpy(FpaEqtnDefs.vtime, vtime);
(void) strcpy(buf, "dp<msl>");
test_evaluate_equation(buf);

/* Reset source and times for testing scalar routines */
(void) strcpy(FpaEqtnDefs.source,    "fem");
(void) strcpy(FpaEqtnDefs.subsource, "");
rtime = strdup(build_tstamp(1991, 238, 12, 00, FALSE, FALSE));
(void) strcpy(FpaEqtnDefs.rtime, rtime);
vtime = strdup(build_tstamp(1991, 238, 12, 00, FALSE, FALSE));
(void) strcpy(FpaEqtnDefs.vtime, vtime);

/* Reset equation database for testing scalar routines */
(void) clear_equation_database();

/* Testing for scalar evaluate_name */
(void) fprintf(stderr, "\n\n ***Testing for scalar evaluate_name***\n");
FpaEqtnDefs.pointeval = TRUE;
(void) strcpy(buf, "ttsfc");					test_evaluate_name(buf);
(void) strcpy(buf, "lat");						test_evaluate_name(buf);
(void) strcpy(buf, "maxtmp");					test_evaluate_name(buf);
vtime = strdup(build_tstamp(1991, 238, 14, 00, TRUE, FALSE));
(void) strcpy(FpaEqtnDefs.vtime, vtime);
(void) strcpy(buf, "maxtmp");					test_evaluate_name(buf);
vtime = strdup(build_tstamp(1991, 238, 12, 00, FALSE, FALSE));
(void) strcpy(FpaEqtnDefs.vtime, vtime);
(void) strcpy(buf, "spdg<850>");				test_evaluate_name(buf);
(void) strcpy(buf, "dirg<850>");				test_evaluate_name(buf);
(void) strcpy(buf, "spdc<850>");				test_evaluate_name(buf);
(void) strcpy(buf, "dirc<850>");				test_evaluate_name(buf);
(void) strcpy(buf, "spdgp<msl>");				test_evaluate_name(buf);
(void) strcpy(buf, "dirgp<msl>");				test_evaluate_name(buf);
(void) strcpy(buf, "spdcp<msl>");				test_evaluate_name(buf);
(void) strcpy(buf, "dircp<msl>");				test_evaluate_name(buf);
(void) strcpy(buf, "spdrp<msl>");				test_evaluate_name(buf);
(void) strcpy(buf, "dirrp<msl>");				test_evaluate_name(buf);
(void) strcpy(buf, "theta_e<sfc>");				test_evaluate_name(buf);
(void) strcpy(buf, "theta_e<500>");				test_evaluate_name(buf);

/* Testing for scalar evaluate_function */
(void) fprintf(stderr, "\n\n ***Testing for scalar evaluate_function***\n");
FpaEqtnDefs.pointeval = TRUE;
(void) strcpy(lbuf, "lat");		(void) strcpy(rbuf, "");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "lon");		(void) strcpy(rbuf, "");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "ddx");		(void) strcpy(rbuf, "ttsfc");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "ddy");		(void) strcpy(rbuf, "gz850");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "ddx");		(void) strcpy(rbuf, "ug850");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "max");		(void) strcpy(rbuf, "ww700,ttsfc-300");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "advct");	(void) strcpy(rbuf, "tt850,ug850,vg850");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "curv");	(void) strcpy(rbuf, "gz850");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "ddx");		(void) strcpy(rbuf, "gz850");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "ddy");		(void) strcpy(rbuf, "gz850");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "curv");	(void) strcpy(rbuf, "pnmsl");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "ddx");		(void) strcpy(rbuf, "pnmsl");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "ddy");		(void) strcpy(rbuf, "pnmsl");
test_evaluate_function(lbuf, rbuf);
(void) strcpy(lbuf, "ddz");		(void) strcpy(rbuf, "pnmsl");
test_evaluate_function(lbuf, rbuf);

/* Testing for endless loop escape */
(void) fprintf(stderr, "\n\n ***Testing for endless loop escape***\n");
(void) strcpy(buf, "rhsfc");					test_evaluate_name(buf);

/* Testing for ... */

return 0;
}

#endif /* EQUATION_STANDALONE */
