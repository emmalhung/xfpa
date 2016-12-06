/*********************************************************************/
/**	@file winds.c
 *
 * Routines to extract wind speed and direction from fields of
 * meteorological data.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*   w i n d s . c                                                      *
*                                                                      *
*   Routines to extract wind speed and direction from fields           *
*   of meteorological data                                             *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2001 Environment Canada (AES)            *
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

#define	WINDS_MAIN			/* To initialize defined constants and */
							/*  functions in winds.h file          */

/* define or undef checking parameter to determine calculation times */
#define TIME_WINDS

/* Commonly used literal tags */
#define WindRel    "model"
#define RelDegrees "\263"
#define RelPercent "%"
#define WindAbs    "abs"
#define AbsDegrees "\263"
#define AbsKnots   "knots"

#include "equation.h"
#include "values.h"
#include "winds.h"

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

#ifdef DEBUG_WINDS			/* Turn on/off internal debug printing */
	static	LOGICAL	DebugMode = TRUE;
#else
	static	LOGICAL	DebugMode = FALSE;
#endif /* DEBUG_WINDS */

#define dprintf (!DebugMode)? (void) 0: (void) fprintf

/* Interface functions               */
/*  ... these are defined in winds.h */


/* Internal static functions (Checking Types of Wind) */
static	LOGICAL		check_fpa_wind(int, FLD_DESCRIPT *, LOGICAL, int, POINT *);
static	LOGICAL		check_other_wind(int, FLD_DESCRIPT *, LOGICAL, int, POINT *);

/* Generic winds cross-referenced in configuration file */
static	const	STRING	UgPres = "ugpres";
static	const	STRING	VgPres = "vgpres";
static	const	STRING	UgHght = "ughght";
static	const	STRING	VgHght = "vghght";

static	const	STRING	SGrMsl = "srpres";
static	const	STRING	DGrMsl = "dirgrad";
static	const	STRING	SGrad  = "srhght";
static	const	STRING	DGrad  = "dirgrad";

static	const	STRING	SCyMsl = "scpres";
static	const	STRING	DCyMsl = "dirgrad";
static	const	STRING	SCyclo = "schght";
static	const	STRING	DCyclo = "dirgrad";

static	const	STRING	UIsbar = "uisbar";
static	const	STRING	VIsbar = "visbar";

/* Constants for winds cross-referenced in configuration file */
static	const	STRING	Rad     = "RAD";
static	const	STRING	CardHgt = "CARDHGT";

/* Units for winds cross-referenced in configuration file */
static	const	STRING	MKS         = "MKS";
static	const	STRING	Meters      = "m";
static	const	STRING	MperS       = "m/s";
static	const	STRING	Knots       = "knots";
static	const	STRING	DegreesBmap = "degrees_bmap";
static	const	STRING	DegreesTrue = "degrees_true";


/**********************************************************************
 ***                                                                ***
 *** i d e n t i f y _ w i n d _ f u n c t i o n                    ***
 *** d i s p l a y _ w i n d _ f u n c t i o n s                    ***
 ***                                                                ***
 **********************************************************************/

/* Define FPA wind functions for search list */
static	WINDFUNC_FUNC	fpa_adjusted_wind;
static	WINDFUNC_FUNC	absolute_wind;
static	WINDFUNC_FUNC	geostrophic_wind;
static	WINDFUNC_FUNC	gradient_wind;
static	WINDFUNC_FUNC	cyclostrophic_wind;
static	WINDFUNC_FUNC	isallobaric_wind;
static	WINDFUNC_FUNC	component_wind;
static	WINDFUNC_FUNC	dir_spd_wind;

/* Initialize FPA wind function search list */
static	WINDFUNC_TABLE	WindFuncs[] =
	{
		{ FpaDefaultWindFunc,                 fpa_adjusted_wind,      1 },
		{ FpaAbsWindFunc,                     absolute_wind,          1 },
		{ "FPA_Geostrophic_Wind_Func",        geostrophic_wind,       1 },
		{ "FPA_Thermal_Wind_Func",            geostrophic_wind,       1 },
		{ "FPA_Gradient_Wind_Func",           gradient_wind,          1 },
		{ "FPA_Cyclostrophic_Wind_Func",      cyclostrophic_wind,     1 },
		{ "FPA_Isallobaric_Wind_Func",        isallobaric_wind,       1 },
		{ "FPA_UVcomponent_Wind_Func",        component_wind,         1 },
		{ "FPA_DirectionSpeed_Wind_Func",     dir_spd_wind,           1 },
	};

/* Set number of FPA wind functions in search list */
static	int		NumWindFuncs =
	(int) (sizeof(WindFuncs) / sizeof(WINDFUNC_TABLE));

/*********************************************************************/
/** Identify a config file wind function name.
 *
 *	@param[in]	name	config file wind function name
 *	@param[out]	*func	wind function structure
 *	@param[out]	*nreq	number of fields required
 * @return True if Successful.
 *********************************************************************/
LOGICAL				identify_wind_function

	(
	STRING			name,
	WINDFUNC		*func,
	int				*nreq
	)

	{
	int				inum, nrq;
	WINDFUNC		fnc;

	/* Initialize return values */
	if ( NotNull(func) ) *func = NullWindFunc;
	if ( NotNull(nreq) ) *nreq = 0;

	/* Return FALSE if no wind function name passed */
	if ( blank(name) ) return FALSE;

	/* Search user defined wind functions first */
	if ( identify_user_wind_function(name, &fnc, &nrq) )
		{
		if ( NotNull(func) ) *func = fnc;
		if ( NotNull(nreq) ) *nreq = nrq;
		return TRUE;
		}

	/* Search internal wind functions next */
	for ( inum=0; inum<NumWindFuncs; inum++ )
		{

		if ( same(name, WindFuncs[inum].name) )
			{
			if ( NotNull(func) ) *func = WindFuncs[inum].func;
			if ( NotNull(nreq) ) *nreq = WindFuncs[inum].nreq;
			return TRUE;
			}
		}

	/* Return FALSE if wind function name not found */
	return FALSE;
	}

/*********************************************************************/
/** display wind function names for config files.
 *********************************************************************/
void				display_wind_functions

	(
	)

	{
	int				inum;

	/* Display all internal wind functions */
	(void) printf(" Default Wind Functions");
	(void) printf(" ... from Config \"wind_function\" lines\n");
	for ( inum=0; inum<NumWindFuncs; inum++ )
		{
		(void) printf("  %2d   Wind Function Name:  %s\n",
				inum+1, WindFuncs[inum].name);
		}
	}

/**********************************************************************
 ***                                                                ***
 *** g e t _ w i n d s _ c o n s t a n t                            ***
 ***                                                                ***
 **********************************************************************/

/* Define FpaWIND_CONSTANTS Object - containing constants for winds */
typedef struct FpaWIND_CONSTANTS_struct
{
	STRING	cname;		/* name of saved constant */
	STRING	cunits;		/* units for saved constant */
	double	cvalue;		/* value for saved constant */
} FpaWIND_CONSTANTS;

/* Storage locations for constants for winds */
static	int					NumWindConstants = 0;
static	int					LastWindConstant = -1;
static	FpaWIND_CONSTANTS	*WindConstants   = NullPtr(FpaWIND_CONSTANTS *);


/*********************************************************************/
/** Get the value of a named wind constant
 *
 *	@param[in]	name	Name of constant
 *	@param[in]	units	Units for constant
 *	@param[out]	*value	Value of constant
 *  @return value of a given constant in given units for wind calculations
 *********************************************************************/
LOGICAL				get_winds_constant

	(
	STRING			name,
	STRING			units,
	double			*value
	)

	{
	int						inum;
	double					xval;
	FpaConfigConstantStruct	*cdef;

	/* Initialize return value */
	if ( NotNull(value) ) *value = 0.0;

	/* Check saved constants for winds, beginning with last one used */
	if ( LastWindConstant >= 0 )
		{
		for ( inum=LastWindConstant; inum<NumWindConstants; inum++ )
			{

			/* Return value of constant, if found */
			if ( same(name, WindConstants[inum].cname)
					&& same(units, WindConstants[inum].cunits) )
				{
				LastWindConstant = inum;
				xval = WindConstants[LastWindConstant].cvalue;
				if ( NotNull(value) ) *value = xval;
				return TRUE;
				}
			}
		for ( inum=0; inum<LastWindConstant; inum++ )
			{

			/* Return value of constant, if found */
			if ( same(name, WindConstants[inum].cname)
					&& same(units, WindConstants[inum].cunits) )
				{
				LastWindConstant = inum;
				xval = WindConstants[LastWindConstant].cvalue;
				if ( NotNull(value) ) *value = xval;
				return TRUE;
				}
			}
		}

	/* Check saved constants for winds, beginning with first one */
	else
		{
		for ( inum=0; inum<NumWindConstants; inum++ )
			{

			/* Return value of constant, if found */
			if ( same(name, WindConstants[inum].cname)
					&& same(units, WindConstants[inum].cunits) )
				{
				LastWindConstant = inum;
				xval = WindConstants[LastWindConstant].cvalue;
				if ( NotNull(value) ) *value = xval;
				return TRUE;
				}
			}
		}

	/* Constant not found in saved list, so find it */
	cdef = identify_constant(name);
	if ( IsNull(cdef) )
		{
		(void) fprintf(stderr, "[get_winds_constant] No constant");
		(void) fprintf(stderr, "  \"%s\"  in Config file\n", SafeStr(name));
		return FALSE;
		}

	/* Convert constant value to requested units, if possible */
	if ( !convert_value(cdef->units->name, cdef->value, units, &xval) )
		{
		(void) fprintf(stderr, "[get_winds_constant] Constant  \"%s\"",
				SafeStr(name));
		(void) fprintf(stderr, "  cannot be converted to  \"%s\"\n",
				SafeStr(units));
		return FALSE;
		}
	if ( NotNull(value) ) *value = xval;

	/* Add this constant to the saved list */
	LastWindConstant = NumWindConstants++;
	WindConstants = GETMEM(WindConstants, FpaWIND_CONSTANTS, NumWindConstants);
	WindConstants[LastWindConstant].cname  = strdup(name);
	WindConstants[LastWindConstant].cunits = strdup(units);
	WindConstants[LastWindConstant].cvalue = xval;
	return TRUE;
	}

/**********************************************************************
 ***                                                                ***
 *** e x t r a c t _ a r e a s e t _ w c a l c s                    ***
 ***                                                                ***
 *** e x t r a c t _ a r e a s e t _ w c a l c s                    ***
 ***                                          b y _ c r o s s r e f ***
 ***                                                                ***
 **********************************************************************/

/*********************************************************************/
/** Extract wind calc info from areaset fields determined from given
 * input parameters
 *
 *	@param[in]	nfds		number of field descriptors
 *	@param[in]	*fdescs		list of field descriptors
 *	@param[in]	matched		match valid times to available fields?
 *	@param[in]	npos		number of positions on fields
 *	@param[in]	*ppos		list of positions on fields
 *	@param[in]	clon		center longitude for fields
 *	@param[out]	**wclist	list of WIND_CALC structs
 * @return True if Successful.
 *********************************************************************/
LOGICAL				extract_areaset_wcalcs

	(
	int				nfds,
	FLD_DESCRIPT	*fdescs,
	LOGICAL			matched,
	int				npos,
	POINT			*ppos,
	float			clon,
	WIND_CALC		**wclist
	)

	{
	int			ifd, ipos, isave;
	LOGICAL		valid;
	CAL			*cals, cal;
	WIND_CALC	*wc;

	/* List of WIND_CALC structs duplicated from unique subareas */
	static	WIND_CALC	**SaveWind = (WIND_CALC **)0;
	static	CAL			*SaveCals  = NullCalPtr;
	static	int			NumSave    = 0;

	/* Check for correct number of field descriptors */
	if ( nfds != 1 )
		{
		(void) fprintf(stderr, "[extract_areaset_wcalcs] Error in");
		(void) fprintf(stderr, " number of field descriptors: %d\n", nfds);
		(void) fprintf(stderr, "   Only one field descriptor allowed (for data)\n");
		return FALSE;
		}

	/* Return FALSE if no information in field descriptors */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	for ( ifd=0; ifd<nfds; ifd++ )
		{
		if ( IsNull(fdescs[ifd].sdef) || IsNull(fdescs[ifd].subdef)
				|| IsNull(fdescs[ifd].edef) || IsNull(fdescs[ifd].ldef) )
			return FALSE;
		}

	/* Free space used by internal buffers */
	FREELIST(SaveWind, NumSave);
	FREEMEM(SaveCals);
	NumSave = 0;

	/* Initialize return parameters */
	for ( ipos=0; ipos<npos; ipos++ )
		{
		if ( NotNull(wclist) ) wclist[ipos] = (WIND_CALC *)0;
		}

	/* Retrieve list of CALs for point list */
	cals  = INITMEM(CAL, npos);
	valid = extract_areaset_attribs(nfds, fdescs, matched, npos, ppos, clon,
			cals);
	if ( !valid )
		{
		return FALSE;
		}

	/* Loop through all positions and extract wind calc info */
	for ( ipos=0; ipos<npos; ipos++ )
		{

		cal = cals[ipos];
		if ( !cal ) continue;

		/* Find if this wind calc is already in the internal list */
		/* if not, add it */
		for (isave=0; isave<NumSave; isave++)
			{
			if (cal == SaveCals[isave]) break;
			}
		if (isave >= NumSave)
			{
			NumSave++;
			SaveWind = GETMEM(SaveWind, WIND_CALC *, NumSave);
			SaveCals = GETMEM(SaveCals, CAL, NumSave);
			wc       = INITMEM(WIND_CALC, 1);
			(void) parse_wind_attribs(cal, wc);
			SaveWind[isave] = wc;
			SaveCals[isave] = cal;
			}

		/* Set pointer in return list */
		if ( NotNull(wclist) ) wclist[ipos] = SaveWind[isave];
		}

	/* Free space */
	FREEMEM(cals);

	return TRUE;
	}

/*********************************************************************/
/** Extract wind calc info from areaset fields determined from given
 * input parameters which include a value cross reference
 *
 *	@param[in]	valuecref	value cross reference
 *	@param[in]	*fdesc		field descriptor
 *	@param[in]	matched		match valid times to available fields?
 *	@param[in]	npos		number of positions on field
 *	@param[in]	*ppos		list of positions on field
 *	@param[in]	clon		center longitude for field
 *	@param[out]	**wclist	list of WIND_CALC structs
 * @return True if Successful.
 *********************************************************************/
LOGICAL				extract_areaset_wcalcs_by_crossref

	(
	STRING			valuecref,
	FLD_DESCRIPT	*fdesc,
	LOGICAL			matched,
	int				npos,
	POINT			*ppos,
	float			clon,
	WIND_CALC		**wclist
	)

	{
	int							ifd;
	LOGICAL						valid;
	FLD_DESCRIPT				*descripts;
	FpaConfigCrossRefStruct		*crdef;

	/* Return FALSE if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->sdef) || IsNull(fdesc->subdef) )
		return FALSE;
	if ( IsNull(ppos) ) return FALSE;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in extract_areaset_wcalcs_by_crossref:\n");
		dprintf(stdout,"   value cross reference: %s\n", valuecref);
		dprintf(stdout,"   path: %s\n",      fdesc->path);
		dprintf(stdout,"   source: %s %s\n", fdesc->sdef->name,
												fdesc->subdef->name);
		dprintf(stdout,"   rtime: %s\n",     fdesc->rtime);
		dprintf(stdout,"   vtime: %s\n",     fdesc->vtime);
		}

	/* Get value cross reference information */
	crdef = identify_crossref(FpaCcRefsValues, valuecref);
	if ( IsNull(crdef) || crdef->nfld < 1 )
		{
		(void) fprintf(stderr,
				"[extract_areaset_wcalcs_by_crossref] Unknown");
		(void) fprintf(stderr,
				" value cross reference type requested: \"%s\"\n",
				SafeStr(valuecref));
		return FALSE;
		}

	/* Build field descriptors for each cross reference field */
	descripts = INITMEM(FLD_DESCRIPT, crdef->nfld);
	for ( ifd=0; ifd<crdef->nfld; ifd++ )
		{
		(void) copy_fld_descript(&descripts[ifd], fdesc);
		if ( !set_fld_descript(&descripts[ifd],
								FpaF_VALUE_FUNCTION_NAME, crdef->func_name,
								FpaF_ELEMENT, crdef->flds[ifd]->element,
								FpaF_LEVEL, crdef->flds[ifd]->level,
								FpaF_END_OF_LIST) )
			{
			FREEMEM(descripts);
			return FALSE;
			}
		}

	/* Extract value using cross reference field descriptors */
	valid = extract_areaset_wcalcs(crdef->nfld, descripts, matched,
			npos, ppos, clon, wclist);
	FREEMEM(descripts);
	return valid;
	}

/**********************************************************************
 ***                                                                ***
 *** e x t r a c t _ a w i n d                                      ***
 *** e x t r a c t _ a w i n d _ b y _ c r o s s r e f              ***
 ***                                                                ***
 ***  ###  Obsolete functions ... replaced by                  ###  ***
 ***  ###   extract_awind()  and  extract_awind_by_crossref()  ###  ***
 ***                                                                ***
 *** e x t r a c t _ w i n d                                        ***
 *** e x t r a c t _ w i n d _ b y _ c r o s s r e f                ***
 ***                                                                ***
 **********************************************************************/

/*********************************************************************/
/** Extract wind speed and direction of a given type of wind from
 * data fields determined from given input parameters
 *
 *	@param[in]	nfds	number of field descriptors
 *	@param[in]	*fdescs	list of field descriptors
 *	@param[in]	matched	match valid times to available fields?
 *	@param[in]	npos	number of positions on fields
 *	@param[in]	*ppos	list of positions on fields
 *	@param[in]	clon	center longitude for fields
 *	@param[out]	*wdirs	list of wind directions
 *	@param[out]	*wspds	list of wind speeds
 *	@param[out]	*wgsts	list of wind gusts
 *	@param[out]	*units	units of wind speeds
 *  @return True if Successful.
 *********************************************************************/
LOGICAL				extract_awind

	(
	int				nfds,
	FLD_DESCRIPT	*fdescs,
	LOGICAL			matched,
	int				npos,
	POINT			*ppos,
	float			clon,
	float			*wdirs,
	float			*wspds,
	float			*wgsts,
	STRING			*units
	)

	{
	int				nn, nreq;
	LOGICAL			valid;
	WINDFUNC		func;

	/* Return FALSE if no information in field descriptors */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	for ( nn=0; nn<nfds; nn++ )
		{
		if ( IsNull(fdescs[nn].sdef) || IsNull(fdescs[nn].subdef)
				|| IsNull(fdescs[nn].edef) || IsNull(fdescs[nn].ldef) )
			return FALSE;
		}

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in extract_awind:\n");
		for ( nn=0; nn<nfds; nn++ )
			{
			dprintf(stdout,"   path: %s\n",       fdescs[nn].path);
			dprintf(stdout,"   source: %s %s\n",  fdescs[nn].sdef->name,
													fdescs[nn].subdef->name);
			dprintf(stdout,"   rtime: %s\n",      fdescs[nn].rtime);
			dprintf(stdout,"   vtime: %s\n",      fdescs[nn].vtime);
			dprintf(stdout,"   function name: %s\n",
													fdescs[nn].wind_func_name);
			dprintf(stdout,"   element: %s\n",    fdescs[nn].edef->name);
			dprintf(stdout,"   level: %s\n",      fdescs[nn].ldef->name);
			}
		}

	/* Set default units */
	if ( NotNull(units) ) *units = NullString;

	/* Identify the wind calculation function to use */
	if ( !identify_wind_function(fdescs[0].wind_func_name, &func, &nreq) )
		{
		(void) fprintf(stderr, "[extract_awind] Unrecognized wind");
		(void) fprintf(stderr, " function: \"%s\"\n",
				SafeStr(fdescs[0].wind_func_name));
		return FALSE;
		}

	/* Return FALSE if error in returned parameters */
	if ( IsNull(func) )
		{
		(void) fprintf(stderr, "[extract_awind] Missing function");
		(void) fprintf(stderr, " for wind function: \"%s\"\n",
				SafeStr(fdescs[0].wind_func_name));
		return FALSE;
		}
	if ( nfds < nreq )
		{
		(void) fprintf(stderr, "[extract_awind] Not enough fields");
		(void) fprintf(stderr, " for wind function: \"%s\"\n",
				SafeStr(fdescs[0].wind_func_name));
		return FALSE;
		}

	/* Branch to the appropriate wind calculation function */
	valid = func(nfds, fdescs, matched, npos, ppos, clon,
					wdirs, wspds, wgsts, units);
	return valid;
	}

/*********************************************************************/
/** Extract wind speed and direction of a given type of wind from
 * data fields determined from given input parameters which include
 * a wind cross reference
 *
 *	@param[in]	windcref wind cross reference
 *	@param[in]	*fdesc 	 field descriptor
 *	@param[in]	matched  match valid times to available fields?
 *	@param[in]	npos 	 number of positions on field
 *	@param[in]	*ppos 	 list of positions on field
 *	@param[in]	clon 	 center longitude for field
 *	@param[out]	*wdirs 	 list of wind directions
 *	@param[out]	*wspds 	 list of wind speeds
 *	@param[out]	*wgsts 	 list of wind gusts
 *	@param[out]	*units	 units of wind speeds
 * @return True if Successful.
 *********************************************************************/
LOGICAL				extract_awind_by_crossref

	(
	STRING			windcref,
	FLD_DESCRIPT	*fdesc,
	LOGICAL			matched,
	int				npos,
	POINT			*ppos,
	float			clon,
	float			*wdirs,
	float			*wspds,
	float			*wgsts,
	STRING			*units
	)

	{
	int							nn;
	LOGICAL						valid;
	FLD_DESCRIPT				*descripts;
	FpaConfigCrossRefStruct		*crdef;

	/* Return FALSE if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->sdef) || IsNull(fdesc->subdef) )
		return FALSE;
	if ( IsNull(ppos) ) return FALSE;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in extract_awind_by_crossref:\n");
		dprintf(stdout,"   path: %s\n",            fdesc->path);
		dprintf(stdout,"   source: %s %s\n",       fdesc->sdef->name,
													fdesc->subdef->name);
		dprintf(stdout,"   rtime: %s\n",           fdesc->rtime);
		dprintf(stdout,"   vtime: %s\n",           fdesc->vtime);
		dprintf(stdout,"   wind dependence: %s\n", windcref);
		}

	/* Set default units */
	if ( NotNull(units) ) *units = NullString;

	/* Get wind cross reference information */
	crdef = identify_crossref(FpaCcRefsWinds, windcref);
	if ( IsNull(crdef) || crdef->nfld <= 0 )
		{
		(void) fprintf(stderr, "[extract_awind_by_crossref] Unknown wind");
		(void) fprintf(stderr, " cross reference type requested: \"%s\"\n",
				SafeStr(windcref));
		return FALSE;
		}

	/* Build field descriptors for each cross reference field */
	descripts = INITMEM(FLD_DESCRIPT, crdef->nfld);
	for ( nn=0; nn<crdef->nfld; nn++ )
		{
		(void) copy_fld_descript(&descripts[nn], fdesc);
		if ( !set_fld_descript(&descripts[nn],
								FpaF_WIND_FUNCTION_NAME, crdef->func_name,
								FpaF_ELEMENT, crdef->flds[nn]->element,
								FpaF_LEVEL, crdef->flds[nn]->level,
								FpaF_END_OF_LIST) )
			{
			FREEMEM(descripts);
			return FALSE;
			}
		}

	/* Extract wind using cross reference fields */
	valid = extract_awind(crdef->nfld, descripts, matched,
			npos, ppos, clon, wdirs, wspds, wgsts, units);
	FREEMEM(descripts);
	return valid;
	}

/* >>> The following is obsolete in the next version <<< */
/*********************************************************************/
/** OBSOLETE!
 *
 * Extract wind speed and direction of a given type of wind from
 * data fields determined from given input parameters
 *
 * @return True if Successful.
 *********************************************************************/
LOGICAL				extract_wind

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on fields */
	POINT			*ppos,		/* pointer to npos positions on fields */
	float			clon,		/* center longitude for fields */
	float			*wdirs,		/* pointer to npos wind directions */
	float			*wspds,		/* pointer to npos wind speeds */
	STRING			*units		/* units of wind speeds */
	)

	{
	static	LOGICAL	FirstCall = TRUE;

	/* Error message for first time routine is called */
	if ( FirstCall )
		{
		(void) fprintf(stderr, "\n[extract_wind] This function");
		(void) fprintf(stderr, " is obsolete!\n");
		(void) fprintf(stderr, "  Replace ...  LOGICAL       valid, matched;\n");
		(void) fprintf(stderr, "               int           nfds, npos;\n");
		(void) fprintf(stderr, "               FLD_DESCRIPT  *fdescs;\n");
		(void) fprintf(stderr, "               POINT         *ppos;\n");
		(void) fprintf(stderr, "               float         clon;\n");
		(void) fprintf(stderr, "               float         *wdirs, *wspds;\n");
		(void) fprintf(stderr, "               STRING        *units;\n");
		(void) fprintf(stderr, "               valid = extract_wind");
		(void) fprintf(stderr, "(nfds, fdescs, matched,\n");
		(void) fprintf(stderr, "                         npos, ppos, clon,");
		(void) fprintf(stderr, " wdirs, wspds, units);\n");
		(void) fprintf(stderr, "\n  With ...     LOGICAL       valid, matched;\n");
		(void) fprintf(stderr, "               int           nfds, npos;\n");
		(void) fprintf(stderr, "               FLD_DESCRIPT  *fdescs;\n");
		(void) fprintf(stderr, "               POINT         *ppos;\n");
		(void) fprintf(stderr, "               float         clon;\n");
		(void) fprintf(stderr, "               float         *wdirs, *wspds, *wgsts;\n");
		(void) fprintf(stderr, "               STRING        *units;\n");
		(void) fprintf(stderr, "               valid = extract_awind");
		(void) fprintf(stderr, "(nfds, fdescs, matched,\n");
		(void) fprintf(stderr, "                         npos, ppos, clon,");
		(void) fprintf(stderr, " wdirs, wspds, wgsts, units);\n");
		(void) fprintf(stderr, "[extract_wind] End of replacement");
		(void) fprintf(stderr, " information.\n");
		FirstCall = FALSE;
		}

	/* Call function extract_awind() */
	return extract_awind(nfds, fdescs, matched, npos, ppos, clon,
			wdirs, wspds, NullPtr(float *), units);
	}
/* >>> The preceding is obsolete in the next version <<< */

/* >>> The following is obsolete in the next version <<< */
/*********************************************************************/
/** OBSOLETE!
 *
 * Extract wind speed and direction of a given type of wind from
 * data fields determined from given input parameters which include
 * a wind cross reference
 *
 * @return True if Successful.
 *********************************************************************/
LOGICAL				extract_wind_by_crossref

	(
	STRING			windcref,	/* wind cross reference */
	FLD_DESCRIPT	*fdesc,		/* pointer to field descriptor */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on field */
	POINT			*ppos,		/* pointer to npos positions on field */
	float			clon,		/* center longitude for field */
	float			*wdirs,		/* pointer to npos wind directions */
	float			*wspds,		/* pointer to npos wind speeds */
	STRING			*units		/* units of wind speeds */
	)

	{
	static	LOGICAL	FirstCall = TRUE;

	/* Error message for first time routine is called */
	if ( FirstCall )
		{
		(void) fprintf(stderr, "\n[extract_wind_by_crossref] This function");
		(void) fprintf(stderr, " is obsolete!\n");
		(void) fprintf(stderr, "  Replace ...  LOGICAL       valid, matched;\n");
		(void) fprintf(stderr, "               STRING        windcref;\n");
		(void) fprintf(stderr, "               FLD_DESCRIPT  *fdesc;\n");
		(void) fprintf(stderr, "               int           npos;\n");
		(void) fprintf(stderr, "               POINT         *ppos;\n");
		(void) fprintf(stderr, "               float         clon;\n");
		(void) fprintf(stderr, "               float         *wdirs, *wspds;\n");
		(void) fprintf(stderr, "               STRING        *units;\n");
		(void) fprintf(stderr, "               valid = extract_wind_by_crossref");
		(void) fprintf(stderr, "(windcref, fdesc, matched,\n");
		(void) fprintf(stderr, "                         npos, ppos, clon,");
		(void) fprintf(stderr, " wdirs, wspds, units);\n");
		(void) fprintf(stderr, "\n  With ...     LOGICAL       valid, matched;\n");
		(void) fprintf(stderr, "               STRING        windcref;\n");
		(void) fprintf(stderr, "               FLD_DESCRIPT  *fdesc;\n");
		(void) fprintf(stderr, "               int           npos;\n");
		(void) fprintf(stderr, "               POINT         *ppos;\n");
		(void) fprintf(stderr, "               float         clon;\n");
		(void) fprintf(stderr, "               float         *wdirs, *wspds, *wgsts;\n");
		(void) fprintf(stderr, "               STRING        *units;\n");
		(void) fprintf(stderr, "               valid = extract_awind_by_crossref");
		(void) fprintf(stderr, "(windcref, fdesc, matched,\n");
		(void) fprintf(stderr, "                         npos, ppos, clon,");
		(void) fprintf(stderr, " wdirs, wspds, wgsts, units);\n");
		(void) fprintf(stderr, "[extract_wind_by_crossref] End of replacement");
		(void) fprintf(stderr, " information.\n");
		FirstCall = FALSE;
		}

	/* Call function extract_awind_by_crossref() */
	return extract_awind_by_crossref(windcref, fdesc, matched, npos, ppos, clon,
			wdirs, wspds, NullPtr(float *), units);
	}
/* >>> The preceding is obsolete in the next version <<< */

/**********************************************************************
 ***                                                                ***
 *** c h e c k _ w i n d _ f u n c t i o n                          ***
 ***                                 _ e r r o r _ m e s s a g e s  ***
 *** c h e c k _ e x t r a c t _ w i n d                            ***
 *** c h e c k _ e x t r a c t _ w i n d _ b y _ c r o s s r e f    ***
 ***                                                                ***
 **********************************************************************/

/* Flag for printing error messages in check_... wind functions */
static	LOGICAL	CheckWindErrorMessages = TRUE;

/*********************************************************************/
/** Set flag for printing error messages in check_... functions.
 *	@param[in]	errorflag 	flag for printing error messages
 *********************************************************************/
void				check_wind_function_error_messages

	(
	LOGICAL			errorflag
	)

	{

	/* Reset flag for printing error messages */
	if ( !errorflag ) CheckWindErrorMessages = FALSE;
	else              CheckWindErrorMessages = TRUE;
	}

/*********************************************************************/
/** Check whether a given type of wind can be extracted from data
 * fields determined from given input paramters
 *
 *	@param[in]	nfds 		number of field descriptors
 *	@param[in]	*fdescs 	list of field descriptors
 *	@param[in]	matched 	match valid times to available fields?
 *	@param[in]	npos 		number of positions on fields
 *	@param[in]	*ppos		list of positions on fields
 * 	@return True if the given type of wind can be extracted.
 *********************************************************************/
LOGICAL				check_extract_wind

	(
	int				nfds,
	FLD_DESCRIPT	*fdescs,
	LOGICAL			matched,
	int				npos,
	POINT			*ppos
	)

	{
	int		nn;

	/* Return FALSE if no information in field descriptors */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	for ( nn=0; nn<nfds; nn++ )
		{
		if ( IsNull(fdescs[nn].sdef) || IsNull(fdescs[nn].subdef)
				|| IsNull(fdescs[nn].edef) || IsNull(fdescs[nn].ldef) )
			return FALSE;
		}

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in check_extract_wind:\n");
		for ( nn=0; nn<nfds; nn++ )
			{
			dprintf(stdout,"   path: %s\n",       fdescs[nn].path);
			dprintf(stdout,"   source: %s %s\n",  fdescs[nn].sdef->name,
													fdescs[nn].subdef->name);
			dprintf(stdout,"   rtime: %s\n",      fdescs[nn].rtime);
			dprintf(stdout,"   vtime: %s\n",      fdescs[nn].vtime);
			dprintf(stdout,"   function name: %s\n",
													fdescs[nn].wind_func_name);
			dprintf(stdout,"   element: %s\n",    fdescs[nn].edef->name);
			dprintf(stdout,"   level: %s\n",      fdescs[nn].ldef->name);
			}
		}

	/* Branch to check for wind adjustment fields */
	if ( same(fdescs[0].wind_func_name, FpaDefaultWindFunc) )
		{
		return check_fpa_wind(nfds, fdescs, matched, npos, ppos);
		}

	/* Branch to check for all other wind calculations */
	else
		{
		return check_other_wind(nfds, fdescs, matched, npos, ppos);
		}
	}

/*********************************************************************/
/** Check whether a given type of wind can be extracted from data
 * fields determined from given input paramters which include a wind
 * cross reference
 *
 *	@param[in]	windcref	wind cross reference
 *	@param[in]	*fdesc		field descriptor
 *	@param[in]	matched		match valid times to available fields?
 *	@param[in]	npos		number of positions on field
 *	@param[in]	*ppos		list of positions on field
 * 	@return True if a given type of wind can be extracted.
 *********************************************************************/
LOGICAL				check_extract_wind_by_crossref

	(
	STRING			windcref,
	FLD_DESCRIPT	*fdesc,
	LOGICAL			matched,
	int				npos,
	POINT			*ppos
	)

	{
	int							nn;
	LOGICAL						valid;
	FLD_DESCRIPT				*descripts;
	FpaConfigCrossRefStruct		*crdef;

	/* Return FALSE if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->sdef) || IsNull(fdesc->subdef) )
		return FALSE;
	if ( IsNull(ppos) ) return FALSE;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in check_extract_wind_by_crossref:\n");
		dprintf(stdout,"   path: %s\n",          fdesc->path);
		dprintf(stdout,"   source: %s %s\n",     fdesc->sdef->name,
													fdesc->subdef->name);
		dprintf(stdout,"   rtime: %s\n",         fdesc->rtime);
		dprintf(stdout,"   vtime: %s\n",         fdesc->vtime);
		dprintf(stdout,"   wind crossref: %s\n", windcref);
		}

	/* Get wind cross reference information */
	crdef = identify_crossref(FpaCcRefsWinds, windcref);
	if ( IsNull(crdef) || crdef->nfld <= 0 )
		{
		(void) fprintf(stderr, "[check_extract_wind_by_crossref] Unknown wind");
		(void) fprintf(stderr, " cross reference type requested: \"%s\"\n",
				SafeStr(windcref));
		return FALSE;
		}

	/* Build field descriptors for each cross reference field */
	descripts = INITMEM(FLD_DESCRIPT, crdef->nfld);
	for ( nn=0; nn<crdef->nfld; nn++ )
		{
		(void) copy_fld_descript(&descripts[nn], fdesc);
		if ( !set_fld_descript(&descripts[nn],
								FpaF_WIND_FUNCTION_NAME, crdef->func_name,
								FpaF_ELEMENT, crdef->flds[nn]->element,
								FpaF_LEVEL, crdef->flds[nn]->level,
								FpaF_END_OF_LIST) )
			{
			FREEMEM(descripts);
			return FALSE;
			}
		}

	/* Check wind using cross reference fields */
	valid = check_extract_wind(crdef->nfld, descripts, matched, npos, ppos);
	FREEMEM(descripts);
	return valid;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Types of Wind)                          *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/**********************************************************************
 ***                                                                ***
 *** f p a _ a d j u s t e d _ w i n d                              ***
 ***                                                                ***
 *** determine wind speed and direction from the type of wind and   ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL		fpa_adjusted_wind

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on field */
	POINT			*ppos,		/* pointer to npos positions on field */
	float			clon,		/* center longitude for field */
	float			*wdirs,		/* pointer to npos wind directions */
	float			*wspds,		/* pointer to npos wind speeds */
	float			*wgsts,		/* pointer to npos wind gusts */
	STRING			*units		/* units of wind speeds */
	)

	{
	FLD_DESCRIPT	descript;
	int				ipos, ityp, inum, icnt;
	LOGICAL			unique;
	STRING			eunits;
	WIND_CALC		*wcalc;
	float			wlat, wlon, dval, dadj, sval, sadj, gval, gadj;
	double			dspd, dgst;

	/* Internal buffers for wind areaset data and unique wind models */
	static	WIND_CALC	**WCalcs    = NullPtr(WIND_CALC **);
	static	int			NumTypes    = 0;
	static	int			MaxNumTypes = 0;
	static	STRING		*WindTypes  = NullStringList;

	/* Internal buffers for wind data for each unique wind model */
	static	int		*NumOfType    = NullPtr(int *);
	static	int		*MaxNumOfType = NullPtr(int *);
	static	int		**CntOfType   = NullPtr(int **);
	static	POINT	**PosOfType   = NullPtr(POINT **);
	static	float	**DirOfType   = NullPtr(float **);
	static	float	**SpdOfType   = NullPtr(float **);
	static	float	**GstOfType   = NullPtr(float **);

	/* Check for correct number of field descriptors */
	if ( nfds != 1 )
		{
		(void) fprintf(stderr, "[fpa_adjusted_wind] Error in");
		(void) fprintf(stderr, " number of field descriptors: %d\n", nfds);
		(void) fprintf(stderr, "   Only one field descriptor allowed (for data)\n");
		return FALSE;
		}

	/* Return FALSE if no information in field descriptor */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	if ( IsNull(fdescs[0].sdef) || IsNull(fdescs[0].subdef)
			|| IsNull(fdescs[0].edef) || IsNull(fdescs[0].ldef) ) return FALSE;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in fpa_adjusted_wind:\n");
		dprintf(stdout,"   path: %s\n",       fdescs[0].path);
		dprintf(stdout,"   source: %s %s\n",  fdescs[0].sdef->name,
												fdescs[0].subdef->name);
		dprintf(stdout,"   rtime: %s\n",      fdescs[0].rtime);
		dprintf(stdout,"   vtime: %s\n",      fdescs[0].vtime);
		dprintf(stdout,"   function name: %s\n",
												fdescs[0].wind_func_name);
		dprintf(stdout,"   element: %s\n",    fdescs[0].edef->name);
		dprintf(stdout,"   level: %s\n",      fdescs[0].ldef->name);
		}

	/* Initialize return parameters */
	for ( ipos=0; ipos<npos; ipos++ )
		{
		if ( NotNull(wdirs) ) wdirs[ipos] = 0.0;
		if ( NotNull(wspds) ) wspds[ipos] = 0.0;
		if ( NotNull(wgsts) ) wgsts[ipos] = 0.0;
		}
	if ( NotNull(units) ) *units = NullString;

	/* Reset valid time for field (if requested) */
	(void) copy_fld_descript(&descript, &fdescs[0]);
	if ( matched )
		{
		(void) matched_source_valid_time_reset(&descript,
				FpaC_TIMEDEP_ANY, descript.vtime);
		}

	/* Allocate space for wind area structures */
	WCalcs = GETMEM(WCalcs, WIND_CALC *, npos);

	/* Return data from wind areas (if found) or background wind */
	if ( !extract_areaset_wcalcs(1, &descript, matched,
			npos, ppos, clon, WCalcs) )
		{
		(void) fprintf(stderr, "[fpa_adjusted_wind] Cannot");
		(void) fprintf(stderr, " extract a wind or background wind\n");
		(void) fprintf(stderr, "   for field: \"%s %s\"\n",
				SafeStr(descript.edef->name), SafeStr(descript.ldef->name));
		(void) fprintf(stderr, "   from source: \"%s %s\"",
				SafeStr(descript.sdef->name), SafeStr(descript.subdef->name));
		(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
				SafeStr(descript.rtime), SafeStr(descript.vtime));
		return FALSE;
		}

	/* Loop through all positions and copy positions of each */
	/*  unique wind type to a separate array                 */
	for ( NumTypes=0, ipos=0; ipos<npos; ipos++ )
		{

		/* Identify unique wind types */
		for ( unique=TRUE, ityp=0; ityp<NumTypes; ityp++ )
			{
			if ( same(WCalcs[ipos]->model, WindTypes[ityp]) )
				{

				/* Copy another position for this wind type */
				inum = ++(NumOfType[ityp]);
				if ( (NumTypes > MaxNumTypes) || IsNull(MaxNumOfType)
						|| (NotNull(MaxNumOfType)
								&& (inum > MaxNumOfType[ityp])) )
					{
					CntOfType[ityp] = GETMEM(CntOfType[ityp], int,   inum);
					PosOfType[ityp] = GETMEM(PosOfType[ityp], POINT, inum);
					}
				CntOfType[ityp][inum-1] = ipos;
				(void) copy_point(PosOfType[ityp][inum-1], ppos[ipos]);
				unique = FALSE;
				break;
				}
			}
		if ( unique )
			{

			/* Save this wind type */
			NumTypes++;
			if ( NumTypes > MaxNumTypes )
				WindTypes = GETMEM(WindTypes,  STRING, NumTypes);
			WindTypes[NumTypes-1] = strdup(WCalcs[ipos]->model);

			/* Allocate space for all positions of this wind type */
			/*  and initialize the position array                 */
			if ( NumTypes > MaxNumTypes )
				{
				NumOfType = GETMEM(NumOfType, int,     NumTypes);
				CntOfType = GETMEM(CntOfType, int *,   NumTypes);
				PosOfType = GETMEM(PosOfType, POINT *, NumTypes);
				CntOfType[NumTypes-1] = NullPtr(int *);
				PosOfType[NumTypes-1] = NullPointList;
				}

			/* Copy position for this wind type */
			inum = 1;
			NumOfType[NumTypes-1] = inum;
			if ( (NumTypes > MaxNumTypes) || IsNull(MaxNumOfType)
					|| (NotNull(MaxNumOfType)
							&& (inum > MaxNumOfType[NumTypes-1])) )
				{
				CntOfType[NumTypes-1] = GETMEM(CntOfType[NumTypes-1],
						int,   inum);
				PosOfType[NumTypes-1] = GETMEM(PosOfType[NumTypes-1],
						POINT, inum);
				}
			CntOfType[NumTypes-1][inum-1] = ipos;
			(void) copy_point(PosOfType[NumTypes-1][inum-1], ppos[ipos]);
			}
		}

	/* Allocate space for wind direction and speed of each unique wind type */
	if ( NumTypes > MaxNumTypes )
		{
		DirOfType = GETMEM(DirOfType, float *, NumTypes);
		SpdOfType = GETMEM(SpdOfType, float *, NumTypes);
		GstOfType = GETMEM(GstOfType, float *, NumTypes);
		}

	/* Initialize and allocate the arrays */
	for ( ityp=0; ityp<NumTypes; ityp++ )
		{
		if ( ityp >= MaxNumTypes )
			{
			DirOfType[ityp] = NullPtr(float *);
			SpdOfType[ityp] = NullPtr(float *);
			GstOfType[ityp] = NullPtr(float *);
			}
		if ( ityp >= MaxNumTypes || IsNull(MaxNumOfType)
				|| (NotNull(MaxNumOfType)
						&& (NumOfType[ityp] > MaxNumOfType[ityp])) )
			{
			DirOfType[ityp] = GETMEM(DirOfType[ityp], float, NumOfType[ityp]);
			SpdOfType[ityp] = GETMEM(SpdOfType[ityp], float, NumOfType[ityp]);
			GstOfType[ityp] = GETMEM(GstOfType[ityp], float, NumOfType[ityp]);
			}
		}

	/* Finished allocating ... so set MaxNumTypes and MaxNumOfType[] */
	/*  for next call to routine                                     */
	if ( NumTypes > MaxNumTypes )
		MaxNumOfType = GETMEM(MaxNumOfType, int, NumTypes);
	MaxNumTypes = NumTypes;
	for ( ityp=0; ityp<NumTypes; ityp++ )
		MaxNumOfType[ityp] = NumOfType[ityp];

	/* Now extract wind direction and speed for each unique wind type */
	for ( ityp=0; ityp<NumTypes; ityp++ )
		{

		/* Extract wind direction and speed by dependence type */
		if ( !extract_awind_by_crossref(WindTypes[ityp], &descript,
				matched, NumOfType[ityp], PosOfType[ityp], clon,
				DirOfType[ityp], SpdOfType[ityp], GstOfType[ityp],
				&eunits) )
			{
			(void) fprintf(stderr, "[fpa_adjusted_wind] Cannot");
			(void) fprintf(stderr, " extract wind type: \"%s\"",
					SafeStr(WindTypes[ityp]));
			(void) fprintf(stderr, " for field: \"%s %s\"\n",
					SafeStr(descript.edef->name),
					SafeStr(descript.ldef->name));
			(void) fprintf(stderr, "   from source: \"%s %s\"",
					SafeStr(descript.sdef->name),
					SafeStr(descript.subdef->name));
			(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
					SafeStr(descript.rtime),
					SafeStr(descript.vtime));
			return FALSE;
			}

		/* Ensure that units can be converted to m/s */
		if ( !convert_value(eunits, 1.0, MperS, NullDouble) )
			{
			(void) fprintf(stderr, "[fpa_adjusted_wind] Unacceptable");
			(void) fprintf(stderr, " units: \"%s\"", eunits);
			(void) fprintf(stderr, " for wind type: \"%s\"\n",
					SafeStr(WindTypes[ityp]));
			(void) fprintf(stderr, "   for field: \"%s %s\"\n",
					SafeStr(descript.edef->name),
					SafeStr(descript.ldef->name));
			(void) fprintf(stderr, "   from source: \"%s %s\"",
					SafeStr(descript.sdef->name),
					SafeStr(descript.subdef->name));
			(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
					SafeStr(descript.rtime),
					SafeStr(descript.vtime));
			return FALSE;
			}

		/* Now loop through all winds of this type to set */
		/*  final wind direction and speed                */
		for ( inum=0; inum<NumOfType[ityp]; inum++ )
			{

			/* Set counter and wind area structure for this wind */
			icnt  = CntOfType[ityp][inum];
			wcalc = WCalcs[CntOfType[ityp][inum]];

			/* Set latitude and longitude for this wind */
			if ( !pos_to_ll(&descript.mproj, PosOfType[ityp][inum],
					&wlat, &wlon) ) continue;

			if ( DebugMode )
				{
				dprintf(stdout,"     Wind Model: \"%s\"", wcalc->model);
				if ( wcalc->rel_dir )
					{
					dprintf(stdout,"  Direction: \"%s %d%s\"",
							WindRel, NINT(wcalc->dir), RelDegrees);
					}
				else
					{
					dprintf(stdout,"  Direction: \"%s %d%s\"",
							WindAbs, NINT(wcalc->dir), AbsDegrees);
					}
				if ( wcalc->rel_speed )
					{
					dprintf(stdout,"  Speed/Gust: \"%s %d%s / %d%s\"\n",
							WindRel, NINT(wcalc->speed), RelPercent,
							NINT(wcalc->gust), RelPercent);
					}
				else
					{
					dprintf(stdout,"  Speed/Gust: \"%s %d%s / %d%s\"\n",
							WindAbs, NINT(wcalc->speed), AbsKnots,
							NINT(wcalc->gust), AbsKnots);
					}
				}

			/* Adjust the calculated wind direction */
			if ( wcalc->rel_dir )
				{

				/* Set the calculated wind direction and adjustment */
				dval = DirOfType[ityp][inum];
				dadj = wcalc->dir;

				if ( DebugMode )
					dprintf(stdout,"     dval: %f  dadj: %f", dval, dadj);

				/* Change the sign of the adjustment in Southern Hemisphere! */
				if ( wlat >= 0.0 ) dval += dadj;
				else               dval -= dadj;

				/* Set the final wind direction */
				dval = fmod(dval, 360.0);
				if ( dval < 0.0 ) dval += 360.0;
				if ( NotNull(wdirs) ) wdirs[icnt] = dval;

				if ( DebugMode )
					dprintf(stdout,"  final dval: %f\n", dval);
				}

			/* Set an absolute wind direction */
			else
				{

				/* Set the absolute wind direction */
				dval = wcalc->dir;

				if ( DebugMode )
					dprintf(stdout,"     dval: %f", dval);

				/* Set the final wind direction */
				dval = fmod(dval, 360.0);
				if ( dval < 0.0 ) dval += 360.0;
				if ( NotNull(wdirs) ) wdirs[icnt] = dval;

				if ( DebugMode )
					dprintf(stdout,"  final dval: %f\n", dval);
				}

			/* Adjust the calculated wind speed and gust */
			if ( wcalc->rel_speed )
				{

				/* Set the calculated wind speed and adjustment */
				sval = SpdOfType[ityp][inum];
				sadj = wcalc->speed;

				if ( DebugMode )
					dprintf(stdout,"     sval: %f  sadj: %f", sval, sadj);

				/* Convert final wind speed to m/s */
				sval *= sadj/100.0;
				(void) convert_value(eunits, (double) sval, MperS, &dspd);
				sval  = (float) dspd;
				if ( NotNull(wspds) ) wspds[icnt] = sval;

				if ( DebugMode )
					dprintf(stdout,"  final sval: %f\n", sval);

				/* Set the calculated wind gust and adjustment */
				gval = GstOfType[ityp][inum];
				gadj = wcalc->gust;

				if ( DebugMode )
					dprintf(stdout,"     gval: %f  gadj: %f", gval, gadj);

				/* Convert final wind gust to m/s */
				gval *= gadj/100.0;
				(void) convert_value(eunits, (double) gval, MperS, &dgst);
				gval  = (float) dgst;
				if ( NotNull(wgsts) ) wgsts[icnt] = gval;

				if ( DebugMode )
					dprintf(stdout,"  final gval: %f\n", gval);
				}

			/* Set an absolute wind speed and gust */
			else
				{

				/* Set the absolute wind speed */
				sval = wcalc->speed;

				if ( DebugMode )
					dprintf(stdout,"     sval: %f", sval);

				/* Convert final wind speed from knots to m/s */
				(void) convert_value(Knots, (double) sval, MperS, &dspd);
				sval  = (float) dspd;
				if ( NotNull(wspds) ) wspds[icnt] = sval;

				if ( DebugMode )
					dprintf(stdout,"  final sval: %f\n", sval);

				/* Set the absolute wind gust */
				gval = wcalc->gust;

				if ( DebugMode )
					dprintf(stdout,"     gval: %f", gval);

				/* Convert final wind gust from knots to m/s */
				(void) convert_value(Knots, (double) gval, MperS, &dgst);
				gval  = (float) dgst;
				if ( NotNull(wgsts) ) wgsts[icnt] = gval;

				if ( DebugMode )
					dprintf(stdout,"  final gval: %f\n", gval);
				}
			}
		}

	/* Set units to m/s and return TRUE if all went well */
	if ( NotNull(units) ) *units = MperS;
	return TRUE;
	}

/**********************************************************************
 ***                                                                ***
 *** a b s o l u t e _ w i n d                                      ***
 ***                                                                ***
 *** set absolute wind speed and direction to zero                  ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL		absolute_wind

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on field */
	POINT			*ppos,		/* pointer to npos positions on field */
	float			clon,		/* center longitude for field */
	float			*wdirs,		/* pointer to npos wind directions */
	float			*wspds,		/* pointer to npos wind speeds */
	float			*wgsts,		/* pointer to npos wind gusts */
	STRING			*units		/* units of wind speeds */
	)

	{
	int							ipos;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in absolute_wind:\n");
		}
	if ( IsNull(ppos) ) return FALSE;

	/* Initialize return parameters */
	for ( ipos=0; ipos<npos; ipos++ )
		{
		if ( NotNull(wdirs) ) wdirs[ipos] = 0.0;
		if ( NotNull(wspds) ) wspds[ipos] = 0.0;
		if ( NotNull(wgsts) ) wgsts[ipos] = 0.0;
		}

	/* Set units to m/s and return TRUE if all went well */
	if ( NotNull(units) ) *units = MperS;
	return TRUE;
	}

/**********************************************************************
 ***                                                                ***
 *** g e o s t r o p h i c _ w i n d                                ***
 ***                                                                ***
 *** determine geostrophic wind speed and direction from a GENERIC  ***
 ***  equation based on a pressure, height, or thickness field      ***
 ***                                                                ***
 **********************************************************************/

/* Global variables to hold GENERIC geostrophic wind information */
static	LOGICAL					GeoSet   = FALSE;
static	LOGICAL					GeoValid = FALSE;
static	FpaConfigElementStruct	*ugpres  = NullPtr(FpaConfigElementStruct *);
static	FpaConfigElementStruct	*vgpres  = NullPtr(FpaConfigElementStruct *);
static	FpaConfigElementStruct	*ughght  = NullPtr(FpaConfigElementStruct *);
static	FpaConfigElementStruct	*vghght  = NullPtr(FpaConfigElementStruct *);

static	LOGICAL		geostrophic_wind

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on field */
	POINT			*ppos,		/* pointer to npos positions on field */
	float			clon,		/* center longitude for field */
	float			*wdirs,		/* pointer to npos wind directions */
	float			*wspds,		/* pointer to npos wind speeds */
	float			*wgsts,		/* pointer to npos wind gusts */
	STRING			*units		/* units of wind speeds */
	)

	{
	int							ipos;
	float						wlat, wlon;
	double						degtorad, uval, vval, dang, sval;
	STRING						Uunits, Vunits;
	char						Ueqtnbuf[MAX_BCHRS], Veqtnbuf[MAX_BCHRS];
	FpaConfigElementStruct		*edef, *ucomp, *vcomp;
	FLD_DESCRIPT				descript;
	VLIST						*Uvlist, *Vvlist;

	/* Check for correct number of field descriptors */
	if ( nfds != 1 )
		{
		(void) fprintf(stderr, "[geostrophic_wind] Error in");
		(void) fprintf(stderr, " number of field descriptors: %d\n", nfds);
		(void) fprintf(stderr, "   Only one field descriptor allowed (for data)\n");
		return FALSE;
		}

	/* Return FALSE if no information in field descriptor */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	if ( IsNull(fdescs[0].sdef) || IsNull(fdescs[0].subdef)
			|| IsNull(fdescs[0].edef) || IsNull(fdescs[0].ldef) ) return FALSE;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in geostrophic_wind:\n");
		dprintf(stdout,"   path: %s\n",       fdescs[0].path);
		dprintf(stdout,"   source: %s %s\n",  fdescs[0].sdef->name,
												fdescs[0].subdef->name);
		dprintf(stdout,"   rtime: %s\n",      fdescs[0].rtime);
		dprintf(stdout,"   vtime: %s\n",      fdescs[0].vtime);
		dprintf(stdout,"   function name: %s\n",
												fdescs[0].wind_func_name);
		dprintf(stdout,"   element: %s\n",    fdescs[0].edef->name);
		dprintf(stdout,"   level: %s\n",      fdescs[0].ldef->name);
		}

#	ifdef TIME_WINDS
	if ( npos>1 )
		{
		set_stopwatch(TRUE);
		}
#	endif /* TIME_WINDS */

	/* Initialize return parameters */
	for ( ipos=0; ipos<npos; ipos++ )
		{
		if ( NotNull(wdirs) ) wdirs[ipos] = 0.0;
		if ( NotNull(wspds) ) wspds[ipos] = 0.0;
		if ( NotNull(wgsts) ) wgsts[ipos] = 0.0;
		}
	if ( NotNull(units) ) *units = NullString;

	/* Set pointers to GENERIC equations only once! */
	if ( !GeoSet )
		{
		GeoValid = TRUE;

		/* Generic equations for geostrophic winds based on pressure */
		ugpres = get_element_info(UgPres);
		if ( IsNull(ugpres) || IsNull(ugpres->elem_detail->equation)
				|| blank(ugpres->elem_detail->equation->eqtn)
				|| IsNull(ugpres->elem_detail->equation->units) )
			{
			(void) fprintf(stderr, "[geostrophic_wind] Error in GENERIC equation");
			(void) fprintf(stderr, "for  \"%s\"  in config file\n", UgPres);
			GeoValid = FALSE;
			}
		vgpres = get_element_info(VgPres);
		if ( IsNull(vgpres) || IsNull(vgpres->elem_detail->equation)
				|| blank(vgpres->elem_detail->equation->eqtn)
				|| IsNull(vgpres->elem_detail->equation->units) )
			{
			(void) fprintf(stderr, "[geostrophic_wind] Error in GENERIC equation");
			(void) fprintf(stderr, "for  \"%s\"  in config file\n", VgPres);
			GeoValid = FALSE;
			}

		/* Generic equations for geostrophic winds based on height/thickness */
		ughght = get_element_info(UgHght);
		if ( IsNull(ughght) || IsNull(ughght->elem_detail->equation)
				|| blank(ughght->elem_detail->equation->eqtn)
				|| IsNull(ughght->elem_detail->equation->units) )
			{
			(void) fprintf(stderr, "[geostrophic_wind] Error in GENERIC equation");
			(void) fprintf(stderr, "for  \"%s\"  in config file\n", UgHght);
			GeoValid = FALSE;
			}
		vghght = get_element_info(VgHght);
		if ( IsNull(vghght) || IsNull(vghght->elem_detail->equation)
				|| blank(vghght->elem_detail->equation->eqtn)
				|| IsNull(vghght->elem_detail->equation->units) )
			{
			(void) fprintf(stderr, "[geostrophic_wind] Error in GENERIC equation");
			(void) fprintf(stderr, "for  \"%s\"  in config file\n", VgHght);
			GeoValid = FALSE;
			}
		GeoSet = TRUE;
		}

	/* Check for valid GENERIC equations */
	if ( !GeoValid )
		{
		(void) fprintf(stderr, "[geostrophic_wind] Error in GENERIC equations\n");
		return FALSE;
		}

	/* Get constant for conversion of degrees to radians */
	if ( !get_winds_constant(Rad, MKS, &degtorad) ) return FALSE;

	/* Reset valid time for field (if requested) */
	(void) copy_fld_descript(&descript, &fdescs[0]);
	if ( matched )
		{
		(void) matched_source_valid_time_reset(&descript,
				FpaC_TIMEDEP_ANY, descript.vtime);
		}

	/* Get detailed element information */
	edef = get_element_info(descript.edef->name);
	if ( IsNull(edef) )
		{
		(void) fprintf(stderr, "[geostrophic_wind] Error in detailed");
		(void) fprintf(stderr, " informtion for element: \"%s\"\n",
				SafeStr(descript.edef->name));
		return FALSE;
		}

	/* Set pointers to u and v components of wind based on wind class */
	switch ( edef->elem_detail->wd_class )
		{

		/* "Pressure" type fields */
		case FpaC_PRESSURE:
			ucomp = ugpres;
			vcomp = vgpres;
			break;

		/* "Height" or "Thickness" type fields */
		case FpaC_HEIGHT:
		case FpaC_THICKNESS:
			ucomp = ughght;
			vcomp = vghght;
			break;

		/* Default for all other type fields */
		default:
			(void) fprintf(stderr, "[geostrophic_wind] Missing wind_class");
			(void) fprintf(stderr, " for element: \"%s\"\n",
					SafeStr(descript.edef->name));
			return FALSE;
		}

	/* Set units and build equation for u component of winds */
	Uunits = ucomp->elem_detail->equation->units->name;
	(void) sprintf(Ueqtnbuf, "%s<@%s>", SafeStr(ucomp->name),
			SafeStr(descript.edef->name));

	/* Set units and build equation for v component of winds */
	Vunits = vcomp->elem_detail->equation->units->name;
	(void) sprintf(Veqtnbuf, "%s<@%s>", SafeStr(vcomp->name),
			SafeStr(descript.edef->name));

	/* Extract u and v components of winds at all locations */
	Uvlist = retrieve_vlist_by_equation(&descript, npos, ppos,
			Uunits, Ueqtnbuf);
	Vvlist = retrieve_vlist_by_equation(&descript, npos, ppos,
			Vunits, Veqtnbuf);

	/* Return FALSE if u or v components cannot be found */
	if ( IsNull(Uvlist) || IsNull(Vvlist) )
		{
		(void) fprintf(stderr, "[geostrophic_wind] Cannot");
		(void) fprintf(stderr, " retrieve data for  \"%s\"  or  \"%s\"\n",
				SafeStr(ucomp->name), SafeStr(vcomp->name));
		if ( NotNull(Uvlist) )
			{
			(void) free_vlist(Uvlist);
			FREEMEM(Uvlist);
			}
		if ( NotNull(Vvlist) )
			{
			(void) free_vlist(Vvlist);
			FREEMEM(Vvlist);
			}
		return FALSE;
		}

	/* Loop through all positions and set wind parameters */
	for ( ipos=0; ipos<npos; ipos++ )
		{

		/* Set latitude and longitude for this position */
		if ( !pos_to_ll(&descript.mproj, ppos[ipos], &wlat, &wlon) )
			continue;

		/* Set wind speed and direction (degrees True ) */
		uval = (double) Uvlist->val[ipos];
		vval = (double) Vvlist->val[ipos];
		if ( NotNull(wdirs) )
			{
			dang = (fpa_atan2(vval, uval) / degtorad) + 180.0;
			wdirs[ipos] = wind_dir_true(&(descript.mproj), wlat, wlon,
											(float) dang);
			}
		if ( NotNull(wspds) || NotNull(wgsts) )
			{
			sval = hypot(uval, vval);
			if ( NotNull(wspds) )
				{
				wspds[ipos] = (float) sval;
				}
			if ( NotNull(wgsts) )
				{
				wgsts[ipos] = (float) sval;
				}
			}
		}

	/* Free space used by VLIST Objects  */
	(void) free_vlist(Uvlist);
	FREEMEM(Uvlist);
	(void) free_vlist(Vvlist);
	FREEMEM(Vvlist);

#	ifdef TIME_WINDS
	if ( npos>1 )
		{
		long	nsec, nusec;
		get_stopwatch(&nsec, &nusec, NULL, NULL);
		(void) fprintf(stdout, "geostrophic wind: %d points %ld.%.6ld sec\n",
				npos, nsec, nusec);
		}
#	endif /* TIME_WINDS */

	/* Set units to m/s and return TRUE if all went well */
	if ( NotNull(units) ) *units = MperS;
	return TRUE;
	}

/**********************************************************************
 ***                                                                ***
 *** g r a d i e n t _ w i n d                                      ***
 ***                                                                ***
 *** determine gradient wind speed and direction from a GENERIC     ***
 ***  equation based on a pressure or height field                  ***
 ***                                                                ***
 **********************************************************************/

/* Global variables to hold GENERIC gradient wind information */
static	LOGICAL					GradSet   = FALSE;
static	LOGICAL					GradValid = FALSE;
static	FpaConfigElementStruct	*sgrmsl   = NullPtr(FpaConfigElementStruct *);
static	FpaConfigElementStruct	*dgrmsl   = NullPtr(FpaConfigElementStruct *);
static	FpaConfigElementStruct	*sgrad    = NullPtr(FpaConfigElementStruct *);
static	FpaConfigElementStruct	*dgrad    = NullPtr(FpaConfigElementStruct *);

static	LOGICAL		gradient_wind

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on field */
	POINT			*ppos,		/* pointer to npos positions on field */
	float			clon,		/* center longitude for field */
	float			*wdirs,		/* pointer to npos wind directions */
	float			*wspds,		/* pointer to npos wind speeds */
	float			*wgsts,		/* pointer to npos wind gusts */
	STRING			*units		/* units of wind speeds */
	)

	{
	int							ipos;
	float						wlat, wlon, spd, dir;
	STRING						Sunits, Dunits;
	char						Seqtnbuf[MAX_BCHRS], Deqtnbuf[MAX_BCHRS];
	FpaConfigElementStruct		*edef, *scomp, *dcomp;
	FLD_DESCRIPT				descript;
	VLIST						*Svlist, *Dvlist;

	/* Check for correct number of field descriptors */
	if ( nfds != 1 )
		{
		(void) fprintf(stderr, "[gradient_wind] Error in");
		(void) fprintf(stderr, " number of field descriptors: %d\n", nfds);
		(void) fprintf(stderr, "   Only one field descriptor allowed (for data)\n");
		return FALSE;
		}

	/* Return FALSE if no information in field descriptor */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	if ( IsNull(fdescs[0].sdef) || IsNull(fdescs[0].subdef)
			|| IsNull(fdescs[0].edef) || IsNull(fdescs[0].ldef) ) return FALSE;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in gradient_wind:\n");
		dprintf(stdout,"   path: %s\n",       fdescs[0].path);
		dprintf(stdout,"   source: %s %s\n",  fdescs[0].sdef->name,
												fdescs[0].subdef->name);
		dprintf(stdout,"   rtime: %s\n",      fdescs[0].rtime);
		dprintf(stdout,"   vtime: %s\n",      fdescs[0].vtime);
		dprintf(stdout,"   function name: %s\n",
												fdescs[0].wind_func_name);
		dprintf(stdout,"   element: %s\n",    fdescs[0].edef->name);
		dprintf(stdout,"   level: %s\n",      fdescs[0].ldef->name);
		}

#	ifdef TIME_WINDS
	if ( npos>1 )
		{
		set_stopwatch(TRUE);
		}
#	endif /* TIME_WINDS */

	/* Initialize return parameters */
	for ( ipos=0; ipos<npos; ipos++ )
		{
		if ( NotNull(wdirs) ) wdirs[ipos] = 0.0;
		if ( NotNull(wspds) ) wspds[ipos] = 0.0;
		if ( NotNull(wgsts) ) wgsts[ipos] = 0.0;
		}
	if ( NotNull(units) ) *units = NullString;

	/* Set pointers to GENERIC equations only once! */
	if ( !GradSet )
		{
		GradValid = TRUE;

		/* Generic equations for gradient winds based on pressure */
		sgrmsl = get_element_info(SGrMsl);
		if ( IsNull(sgrmsl) || IsNull(sgrmsl->elem_detail->equation)
				|| blank(sgrmsl->elem_detail->equation->eqtn)
				|| IsNull(sgrmsl->elem_detail->equation->units) )
			{
			(void) fprintf(stderr, "[gradient_wind] Error in GENERIC equation");
			(void) fprintf(stderr, "for  \"%s\"  in config file\n", SGrMsl);
			GradValid = FALSE;
			}
		dgrmsl = get_element_info(DGrMsl);
		if ( IsNull(dgrmsl) || IsNull(dgrmsl->elem_detail->equation)
				|| blank(dgrmsl->elem_detail->equation->eqtn)
				|| IsNull(dgrmsl->elem_detail->equation->units) )
			{
			(void) fprintf(stderr, "[gradient_wind] Error in GENERIC equation");
			(void) fprintf(stderr, "for  \"%s\"  in config file\n", DGrMsl);
			GradValid = FALSE;
			}

		/* Generic equations for gradient winds based on height */
		sgrad = get_element_info(SGrad);
		if ( IsNull(sgrad) || IsNull(sgrad->elem_detail->equation)
				|| blank(sgrad->elem_detail->equation->eqtn)
				|| IsNull(sgrad->elem_detail->equation->units) )
			{
			(void) fprintf(stderr, "[gradient_wind] Error in GENERIC equation");
			(void) fprintf(stderr, "for  \"%s\"  in config file\n", SGrad);
			GradValid = FALSE;
			}
		dgrad = get_element_info(DGrad);
		if ( IsNull(dgrad) || IsNull(dgrad->elem_detail->equation)
				|| blank(dgrad->elem_detail->equation->eqtn)
				|| IsNull(dgrad->elem_detail->equation->units) )
			{
			(void) fprintf(stderr, "[gradient_wind] Error in GENERIC equation");
			(void) fprintf(stderr, "for  \"%s\"  in config file\n", DGrad);
			GradValid = FALSE;
			}
		GradSet = TRUE;
		}

	/* Check for valid GENERIC equations */
	if ( !GradValid )
		{
		(void) fprintf(stderr, "[gradient_wind] Error in GENERIC equations\n");
		return FALSE;
		}

	/* Reset valid time for field (if requested) */
	(void) copy_fld_descript(&descript, &fdescs[0]);
	if ( matched )
		{
		(void) matched_source_valid_time_reset(&descript,
				FpaC_TIMEDEP_ANY, descript.vtime);
		}

	/* Get detailed element information */
	edef = get_element_info(descript.edef->name);
	if ( IsNull(edef) )
		{
		(void) fprintf(stderr, "[gradient_wind] Error in detailed");
		(void) fprintf(stderr, " informtion for element: \"%s\"\n",
				SafeStr(descript.edef->name));
		return FALSE;
		}

	/* Set pointers to speed and direction of wind based on wind class */
	switch ( edef->elem_detail->wd_class )
		{

		/* "Pressure" type fields */
		case FpaC_PRESSURE:
			scomp = sgrmsl;
			dcomp = dgrmsl;
			break;

		/* "Height" type fields */
		case FpaC_HEIGHT:
			scomp = sgrad;
			dcomp = dgrad;
			break;

		/* Default for all other type fields */
		default:
			(void) fprintf(stderr, "[gradient_wind] Missing wind_class");
			(void) fprintf(stderr, " for element: \"%s\"\n",
					SafeStr(descript.edef->name));
			return FALSE;
		}

	/* Set units and build equation for wind speed */
	Sunits = scomp->elem_detail->equation->units->name;
	(void) sprintf(Seqtnbuf, "%s<@%s>", SafeStr(scomp->name),
			SafeStr(descript.edef->name));

	/* Set units and build equation for wind direction */
	Dunits = dcomp->elem_detail->equation->units->name;
	(void) sprintf(Deqtnbuf, "%s<@%s>", SafeStr(dcomp->name),
			SafeStr(descript.edef->name));

	/* Extract wind speed and direction at all locations */
	Svlist = retrieve_vlist_by_equation(&descript, npos, ppos,
			Sunits, Seqtnbuf);
	Dvlist = retrieve_vlist_by_equation(&descript, npos, ppos,
			Dunits, Deqtnbuf);

	/* Return FALSE if wind speed or direction cannot be found */
	if ( IsNull(Svlist) || IsNull(Dvlist) )
		{
		(void) fprintf(stderr, "[gradient_wind] Cannot");
		(void) fprintf(stderr, " retrieve data for  \"%s\"  or  \"%s\"\n",
				SafeStr(scomp->name), SafeStr(dcomp->name));
		if ( NotNull(Svlist) )
			{
			(void) free_vlist(Svlist);
			FREEMEM(Svlist);
			}
		if ( NotNull(Dvlist) )
			{
			(void) free_vlist(Dvlist);
			FREEMEM(Dvlist);
			}
		return FALSE;
		}

	/* Loop through all positions and set wind parameters */
	for ( ipos=0; ipos<npos; ipos++ )
		{

		/* Set latitude and longitude for this position */
		if ( !pos_to_ll(&descript.mproj, ppos[ipos], &wlat, &wlon) )
			continue;

		/* Set wind speed and direction (degrees True ) */
		spd = Svlist->val[ipos];
		dir = Dvlist->val[ipos];
		if ( NotNull(wdirs) )
			{
			wdirs[ipos] = wind_dir_true(&(descript.mproj), wlat, wlon, dir);
			}
		if ( NotNull(wspds) )
			{
			wspds[ipos] = spd;
			}
		if ( NotNull(wgsts) )
			{
			wgsts[ipos] = spd;
			}
		}

	/* Free space used by VLIST Objects  */
	(void) free_vlist(Svlist);
	FREEMEM(Svlist);
	(void) free_vlist(Dvlist);
	FREEMEM(Dvlist);

#	ifdef TIME_WINDS
	if ( npos>1 )
		{
		long	nsec, nusec;
		get_stopwatch(&nsec, &nusec, NULL, NULL);
		(void) fprintf(stdout, "gradient wind: %d points %ld.%.6ld sec\n",
				npos, nsec, nusec);
		}
#	endif /* TIME_WINDS */

	/* Set units to m/s and return TRUE if all went well */
	if ( NotNull(units) ) *units = MperS;
	return TRUE;
	}

/**********************************************************************
 ***                                                                ***
 *** c y c l o s t r o p h i c _ w i n d                            ***
 ***                                                                ***
 *** determine cyclostrophic wind speed and direction from a        ***
 ***  GENERIC equation based on a pressure or height field          ***
 ***                                                                ***
 **********************************************************************/

/* Global variables to hold GENERIC cyclostrophic wind information */
static	LOGICAL					CycloSet   = FALSE;
static	LOGICAL					CycloValid = FALSE;
static	FpaConfigElementStruct	*scymsl    = NullPtr(FpaConfigElementStruct *);
static	FpaConfigElementStruct	*dcymsl    = NullPtr(FpaConfigElementStruct *);
static	FpaConfigElementStruct	*scyclo    = NullPtr(FpaConfigElementStruct *);
static	FpaConfigElementStruct	*dcyclo    = NullPtr(FpaConfigElementStruct *);

static	LOGICAL		cyclostrophic_wind

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on field */
	POINT			*ppos,		/* pointer to npos positions on field */
	float			clon,		/* center longitude for field */
	float			*wdirs,		/* pointer to npos wind directions */
	float			*wspds,		/* pointer to npos wind speeds */
	float			*wgsts,		/* pointer to npos wind gusts */
	STRING			*units		/* units of wind speeds */
	)

	{
	int							ipos;
	float						wlat, wlon, spd, dir;
	STRING						Sunits, Dunits;
	char						Seqtnbuf[MAX_BCHRS], Deqtnbuf[MAX_BCHRS];
	FpaConfigElementStruct		*edef, *scomp, *dcomp;
	FLD_DESCRIPT				descript;
	VLIST						*Svlist, *Dvlist;

	/* Check for correct number of field descriptors */
	if ( nfds != 1 )
		{
		(void) fprintf(stderr, "[cyclostrophic_wind] Error in");
		(void) fprintf(stderr, " number of field descriptors: %d\n", nfds);
		(void) fprintf(stderr, "   Only one field descriptor allowed (for data)\n");
		return FALSE;
		}

	/* Return FALSE if no information in field descriptor */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	if ( IsNull(fdescs[0].sdef) || IsNull(fdescs[0].subdef)
			|| IsNull(fdescs[0].edef) || IsNull(fdescs[0].ldef) ) return FALSE;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in cyclostrophic_wind:\n");
		dprintf(stdout,"   path: %s\n",       fdescs[0].path);
		dprintf(stdout,"   source: %s %s\n",  fdescs[0].sdef->name,
												fdescs[0].subdef->name);
		dprintf(stdout,"   rtime: %s\n",      fdescs[0].rtime);
		dprintf(stdout,"   vtime: %s\n",      fdescs[0].vtime);
		dprintf(stdout,"   function name: %s\n",
												fdescs[0].wind_func_name);
		dprintf(stdout,"   element: %s\n",    fdescs[0].edef->name);
		dprintf(stdout,"   level: %s\n",      fdescs[0].ldef->name);
		}

#	ifdef TIME_WINDS
	if ( npos>1 )
		{
		set_stopwatch(TRUE);
		}
#	endif /* TIME_WINDS */

	/* Initialize return parameters */
	for ( ipos=0; ipos<npos; ipos++ )
		{
		if ( NotNull(wdirs) ) wdirs[ipos] = 0.0;
		if ( NotNull(wspds) ) wspds[ipos] = 0.0;
		if ( NotNull(wgsts) ) wgsts[ipos] = 0.0;
		}
	if ( NotNull(units) ) *units = NullString;

	/* Set pointers to GENERIC equations only once! */
	if ( !CycloSet )
		{
		CycloValid = TRUE;

		/* Generic equations for cyclostrophic winds based on pressure */
		scymsl = get_element_info(SCyMsl);
		if ( IsNull(scymsl) || IsNull(scymsl->elem_detail->equation)
				|| blank(scymsl->elem_detail->equation->eqtn)
				|| IsNull(scymsl->elem_detail->equation->units) )
			{
			(void) fprintf(stderr, "[cyclostophic_wind] Error in GENERIC equation");
			(void) fprintf(stderr, "for  \"%s\"  in config file\n", SCyMsl);
			CycloValid = FALSE;
			}
		dcymsl = get_element_info(DCyMsl);
		if ( IsNull(dcymsl) || IsNull(dcymsl->elem_detail->equation)
				|| blank(dcymsl->elem_detail->equation->eqtn)
				|| IsNull(dcymsl->elem_detail->equation->units) )
			{
			(void) fprintf(stderr, "[cyclostophic_wind] Error in GENERIC equation");
			(void) fprintf(stderr, "for  \"%s\"  in config file\n", DCyMsl);
			CycloValid = FALSE;
			}

		/* Generic equations for cyclostophic winds based on height */
		scyclo = get_element_info(SCyclo);
		if ( IsNull(scyclo) || IsNull(scyclo->elem_detail->equation)
				|| blank(scyclo->elem_detail->equation->eqtn)
				|| IsNull(scyclo->elem_detail->equation->units) )
			{
			(void) fprintf(stderr, "[cyclostophic_wind] Error in GENERIC equation");
			(void) fprintf(stderr, "for  \"%s\"  in config file\n", SCyclo);
			CycloValid = FALSE;
			}
		dcyclo = get_element_info(DCyclo);
		if ( IsNull(dcyclo) || IsNull(dcyclo->elem_detail->equation)
				|| blank(dcyclo->elem_detail->equation->eqtn)
				|| IsNull(dcyclo->elem_detail->equation->units) )
			{
			(void) fprintf(stderr, "[cyclostophic_wind] Error in GENERIC equation");
			(void) fprintf(stderr, "for  \"%s\"  in config file\n", DCyclo);
			CycloValid = FALSE;
			}
		CycloSet = TRUE;
		}

	/* Check for valid GENERIC equations */
	if ( !CycloValid )
		{
		(void) fprintf(stderr, "[cyclostrophic_wind] Error in GENERIC equations\n");
		return FALSE;
		}

	/* Reset valid time for field (if requested) */
	(void) copy_fld_descript(&descript, &fdescs[0]);
	if ( matched )
		{
		(void) matched_source_valid_time_reset(&descript,
				FpaC_TIMEDEP_ANY, descript.vtime);
		}

	/* Get detailed element information */
	edef = get_element_info(descript.edef->name);
	if ( IsNull(edef) )
		{
		(void) fprintf(stderr, "[cyclostophic_wind] Error in detailed");
		(void) fprintf(stderr, " informtion for element: \"%s\"\n",
				SafeStr(descript.edef->name));
		return FALSE;
		}

	/* Set pointers to speed and direction of wind based on wind class */
	switch ( edef->elem_detail->wd_class )
		{

		/* "Pressure" type fields */
		case FpaC_PRESSURE:
			scomp = scymsl;
			dcomp = dcymsl;
			break;

		/* "Height" type fields */
		case FpaC_HEIGHT:
			scomp = scyclo;
			dcomp = dcyclo;
			break;

		/* Default for all other type fields */
		default:
			(void) fprintf(stderr, "[cyclostrophic_wind] Missing wind_class");
			(void) fprintf(stderr, " for element: \"%s\"\n",
					SafeStr(descript.edef->name));
			return FALSE;
		}

	/* Set units and build equation for wind speed */
	Sunits = scomp->elem_detail->equation->units->name;
	(void) sprintf(Seqtnbuf, "%s<@%s>", SafeStr(scomp->name),
			SafeStr(descript.edef->name));

	/* Set units and build equation for wind direction */
	Dunits = dcomp->elem_detail->equation->units->name;
	(void) sprintf(Deqtnbuf, "%s<@%s>", SafeStr(dcomp->name),
			SafeStr(descript.edef->name));

	/* Extract wind speed and direction at all locations */
	Svlist = retrieve_vlist_by_equation(&descript, npos, ppos,
			Sunits, Seqtnbuf);
	Dvlist = retrieve_vlist_by_equation(&descript, npos, ppos,
			Dunits, Deqtnbuf);

	/* Return FALSE if wind speed or direction cannot be found */
	if ( IsNull(Svlist) || IsNull(Dvlist) )
		{
		(void) fprintf(stderr, "[cyclostrophic_wind] Cannot");
		(void) fprintf(stderr, " retrieve data for  \"%s\"  or  \"%s\"\n",
				SafeStr(scomp->name), SafeStr(dcomp->name));
		if ( NotNull(Svlist) )
			{
			(void) free_vlist(Svlist);
			FREEMEM(Svlist);
			}
		if ( NotNull(Dvlist) )
			{
			(void) free_vlist(Dvlist);
			FREEMEM(Dvlist);
			}
		return FALSE;
		}

	/* Loop through all positions and set wind parameters */
	for ( ipos=0; ipos<npos; ipos++ )
		{

		/* Set latitude and longitude for this position */
		if ( !pos_to_ll(&descript.mproj, ppos[ipos], &wlat, &wlon) )
			continue;

		/* Set wind speed and direction (degrees True ) */
		spd = Svlist->val[ipos];
		dir = Dvlist->val[ipos];
		if ( NotNull(wdirs) )
			{
			wdirs[ipos] = wind_dir_true(&(descript.mproj), wlat, wlon, dir);
			}
		if ( NotNull(wspds) )
			{
			wspds[ipos] = spd;
			}
		if ( NotNull(wgsts) )
			{
			wgsts[ipos] = spd;
			}
		}

	/* Free space used by VLIST Objects  */
	(void) free_vlist(Svlist);
	FREEMEM(Svlist);
	(void) free_vlist(Dvlist);
	FREEMEM(Dvlist);

#	ifdef TIME_WINDS
	if ( npos>1 )
		{
		long	nsec, nusec;
		get_stopwatch(&nsec, &nusec, NULL, NULL);
		(void) fprintf(stdout, "cyclostrophic wind: %d points %ld.%.6ld sec\n",
				npos, nsec, nusec);
		}
#	endif /* TIME_WINDS */

	/* Set units to m/s and return TRUE if all went well */
	if ( NotNull(units) ) *units = MperS;
	return TRUE;
	}

/**********************************************************************
 ***                                                                ***
 *** i s a l l o b a r i c _ w i n d                                ***
 ***                                                                ***
 *** determine isallobaric wind speed and direction from a GENERIC  ***
 ***  equation based on a pressure difference field                 ***
 ***                                                                ***
 **********************************************************************/

/* Global variables to hold GENERIC isallobaric wind information */
static	LOGICAL					IsalloSet   = FALSE;
static	LOGICAL					IsalloValid = FALSE;
static	FpaConfigElementStruct	*uisbar     = NullPtr(FpaConfigElementStruct *);
static	FpaConfigElementStruct	*visbar     = NullPtr(FpaConfigElementStruct *);

static	LOGICAL		isallobaric_wind

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on field */
	POINT			*ppos,		/* pointer to npos positions on field */
	float			clon,		/* center longitude for field */
	float			*wdirs,		/* pointer to npos wind directions */
	float			*wspds,		/* pointer to npos wind speeds */
	float			*wgsts,		/* pointer to npos wind gusts */
	STRING			*units		/* units of wind speeds */
	)

	{
	int							ipos;
	float						wlat, wlon;
	double						degtorad, uval, vval, dang, sval;
	STRING						Uunits, Vunits;
	char						Ueqtnbuf[MAX_BCHRS], Veqtnbuf[MAX_BCHRS];
	FpaConfigElementStruct		*edef, *ucomp, *vcomp;
	FLD_DESCRIPT				descript;
	VLIST						*Uvlist, *Vvlist;

	/* Check for correct number of field descriptors */
	if ( nfds != 1 )
		{
		(void) fprintf(stderr, "[isallobaric_wind] Error in");
		(void) fprintf(stderr, " number of field descriptors: %d\n", nfds);
		(void) fprintf(stderr, "   Only one field descriptor allowed (for data)\n");
		return FALSE;
		}

	/* Return FALSE if no information in field descriptor */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	if ( IsNull(fdescs[0].sdef) || IsNull(fdescs[0].subdef)
			|| IsNull(fdescs[0].edef) || IsNull(fdescs[0].ldef) ) return FALSE;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in isallobaric_wind:\n");
		dprintf(stdout,"   path: %s\n",       fdescs[0].path);
		dprintf(stdout,"   source: %s %s\n",  fdescs[0].sdef->name,
												fdescs[0].subdef->name);
		dprintf(stdout,"   rtime: %s\n",      fdescs[0].rtime);
		dprintf(stdout,"   vtime: %s\n",      fdescs[0].vtime);
		dprintf(stdout,"   function name: %s\n",
												fdescs[0].wind_func_name);
		dprintf(stdout,"   element: %s\n",    fdescs[0].edef->name);
		dprintf(stdout,"   level: %s\n",      fdescs[0].ldef->name);
		}

#	ifdef TIME_WINDS
	if ( npos>1 )
		{
		set_stopwatch(TRUE);
		}
#	endif /* TIME_WINDS */

	/* Initialize return parameters */
	for ( ipos=0; ipos<npos; ipos++ )
		{
		if ( NotNull(wdirs) ) wdirs[ipos] = 0.0;
		if ( NotNull(wspds) ) wspds[ipos] = 0.0;
		if ( NotNull(wgsts) ) wgsts[ipos] = 0.0;
		}
	if ( NotNull(units) ) *units = NullString;

	/* Set pointers to GENERIC equations only once! */
	if ( !IsalloSet )
		{
		IsalloValid = TRUE;

		/* Generic equations for isallobaric winds */
		uisbar = get_element_info(UIsbar);
		if ( IsNull(uisbar) || IsNull(uisbar->elem_detail->equation)
				|| blank(uisbar->elem_detail->equation->eqtn)
				|| IsNull(uisbar->elem_detail->equation->units) )
			{
			(void) fprintf(stderr, "[isallobaric_wind] Error in GENERIC equation");
			(void) fprintf(stderr, "for  \"%s\"  in config file\n", UIsbar);
			IsalloValid = FALSE;
			}
		visbar = get_element_info(VIsbar);
		if ( IsNull(visbar) || IsNull(visbar->elem_detail->equation)
				|| blank(visbar->elem_detail->equation->eqtn)
				|| IsNull(visbar->elem_detail->equation->units) )
			{
			(void) fprintf(stderr, "[isallobaric_wind] Error in GENERIC equation");
			(void) fprintf(stderr, "for  \"%s\"  in config file\n", VIsbar);
			IsalloValid = FALSE;
			}
		IsalloSet = TRUE;
		}

	/* Check for valid GENERIC equations */
	if ( !IsalloValid )
		{
		(void) fprintf(stderr, "[isallobaric_wind] Error in GENERIC equations\n");
		return FALSE;
		}

	/* Get constant for conversion of degrees to radians */
	if ( !get_winds_constant(Rad, MKS, &degtorad) ) return FALSE;

	/* Reset valid time for field (if requested) */
	(void) copy_fld_descript(&descript, &fdescs[0]);
	if ( matched )
		{
		(void) matched_source_valid_time_reset(&descript,
				FpaC_TIMEDEP_ANY, descript.vtime);
		}

	/* Get detailed element information */
	edef = get_element_info(descript.edef->name);
	if ( IsNull(edef) )
		{
		(void) fprintf(stderr, "[isallobaric_wind] Error in detailed");
		(void) fprintf(stderr, " informtion for element: \"%s\"\n",
				SafeStr(descript.edef->name));
		return FALSE;
		}

	/* Set pointers to u and v components of wind based on wind class */
	switch ( edef->elem_detail->wd_class )
		{

		/* "Pressure" type fields */
		case FpaC_PRESSURE:
			ucomp = uisbar;
			vcomp = visbar;
			break;

		/* Default for all other type fields */
		default:
			(void) fprintf(stderr, "[isallobaric_wind] Missing wind_class");
			(void) fprintf(stderr, " for element: \"%s\"\n",
					SafeStr(descript.edef->name));
			return FALSE;
		}

	/* Set units and build equation for u component of winds */
	Uunits = ucomp->elem_detail->equation->units->name;
	(void) sprintf(Ueqtnbuf, "%s<@%s>", SafeStr(ucomp->name),
			SafeStr(descript.edef->name));

	/* Set units and build equation for v component of winds */
	Vunits = vcomp->elem_detail->equation->units->name;
	(void) sprintf(Veqtnbuf, "%s<@%s>", SafeStr(vcomp->name),
			SafeStr(descript.edef->name));

	/* Extract u and v components of winds at all locations */
	Uvlist = retrieve_vlist_by_equation(&descript, npos, ppos,
			Uunits, Ueqtnbuf);
	Vvlist = retrieve_vlist_by_equation(&descript, npos, ppos,
			Vunits, Veqtnbuf);

	/* Return FALSE if u or v components cannot be found */
	if ( IsNull(Uvlist) || IsNull(Vvlist) )
		{
		(void) fprintf(stderr, "[isallobaric_wind] Cannot");
		(void) fprintf(stderr, " retrieve data for  \"%s\"  or  \"%s\"\n",
				SafeStr(ucomp->name), SafeStr(vcomp->name));
		if ( NotNull(Uvlist) )
			{
			(void) free_vlist(Uvlist);
			FREEMEM(Uvlist);
			}
		if ( NotNull(Vvlist) )
			{
			(void) free_vlist(Vvlist);
			FREEMEM(Vvlist);
			}
		return FALSE;
		}

	/* Loop through all positions and extract wind parameters */
	for ( ipos=0; ipos<npos; ipos++ )
		{

		/* Set latitude and longitude for this position */
		if ( !pos_to_ll(&descript.mproj, ppos[ipos], &wlat, &wlon) )
			continue;

		/* Set wind speed and direction (degrees True ) */
		uval = (double) Uvlist->val[ipos];
		vval = (double) Vvlist->val[ipos];
		if ( NotNull(wdirs) )
			{
			dang = (fpa_atan2(vval, uval) / degtorad) + 180.0;
			wdirs[ipos] = wind_dir_true(&(descript.mproj), wlat, wlon,
											(float) dang);
			}
		if ( NotNull(wspds) || NotNull(wgsts) )
			{
			sval = hypot(uval, vval);
			if ( NotNull(wspds) )
				{
				wspds[ipos] = (float) sval;
				}
			if ( NotNull(wgsts) )
				{
				wgsts[ipos] = (float) sval;
				}
			}
		}

	/* Free space used by VLIST Objects */
	(void) free_vlist(Uvlist);
	FREEMEM(Uvlist);
	(void) free_vlist(Vvlist);
	FREEMEM(Vvlist);

#	ifdef TIME_WINDS
	if ( npos>1 )
		{
		long	nsec, nusec;
		get_stopwatch(&nsec, &nusec, NULL, NULL);
		(void) fprintf(stdout, "isallobaric wind: %d points %ld.%.6ld sec\n",
				npos, nsec, nusec);
		}
#	endif /* TIME_WINDS */

	/* Set units to m/s and return TRUE if all went well */
	if ( NotNull(units) ) *units = MperS;
	return TRUE;
	}

/**********************************************************************
 ***                                                                ***
 *** c o m p o n e n t _ w i n d                                    ***
 ***                                                                ***
 *** determine wind speed and direction from the u and v component  ***
 ***  fields of the given wind                                      ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL		component_wind

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on fields */
	POINT			*ppos,		/* pointer to npos positions on fields */
	float			clon,		/* center longitude for fields */
	float			*wdirs,		/* pointer to npos wind directions */
	float			*wspds,		/* pointer to npos wind speeds */
	float			*wgsts,		/* pointer to npos wind gusts */
	STRING			*units		/* units of wind speeds */
	)

	{
	int									nn, ipos, nc;
	float								wlat, wlon;
	double								degtorad, uval, vval, dang, sval;
	FpaConfigElementStruct				*edef[2];
	FpaConfigElementComponentStruct		*cmpnt[2];
	FLD_DESCRIPT						descript;
	VLIST								*Uvlist, *Vvlist;
	LOGICAL								UseVector;

	/* Check for correct number of field descriptors */
	if ( nfds < 1 )
		{
		(void) fprintf(stderr, "[component_wind] Error in");
		(void) fprintf(stderr, " number of field descriptors: %d\n", nfds);
		(void) fprintf(stderr, "   At least one field descriptor required");
		(void) fprintf(stderr, " (for u or v or uv component data)\n");
		return FALSE;
		}

	/* Return FALSE if no information in field descriptors */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	for ( nn=0; nn<nfds; nn++ )
		{
		if ( IsNull(fdescs[nn].sdef) || IsNull(fdescs[nn].subdef)
				|| IsNull(fdescs[nn].edef) || IsNull(fdescs[nn].ldef) )
			return FALSE;
		}

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in component_wind:\n");
		for ( nn=0; nn<nfds; nn++ )
			{
			dprintf(stdout,"   path: %s\n",       fdescs[nn].path);
			dprintf(stdout,"   source: %s %s\n",  fdescs[nn].sdef->name,
													fdescs[nn].subdef->name);
			dprintf(stdout,"   rtime: %s\n",      fdescs[nn].rtime);
			dprintf(stdout,"   vtime: %s\n",      fdescs[nn].vtime);
			dprintf(stdout,"   function name: %s\n",
													fdescs[nn].wind_func_name);
			dprintf(stdout,"   element: %s\n",    fdescs[nn].edef->name);
			dprintf(stdout,"   level: %s\n",      fdescs[nn].ldef->name);
			}
		}

#	ifdef TIME_WINDS
	if ( npos>1 )
		{
		set_stopwatch(TRUE);
		}
#	endif /* TIME_WINDS */

	/* Initialize return parameters */
	for ( ipos=0; ipos<npos; ipos++ )
		{
		if ( NotNull(wdirs) ) wdirs[ipos] = 0.0;
		if ( NotNull(wspds) ) wspds[ipos] = 0.0;
		if ( NotNull(wgsts) ) wgsts[ipos] = 0.0;
		}
	if ( NotNull(units) ) *units = NullString;

	/* Get constant for conversion of degrees to radians */
	if ( !get_winds_constant(Rad, MKS, &degtorad) ) return FALSE;

	/* Ensure that the first field descriptor is from a component field */
	/*  with u and v or uv (x and y) components                         */
	if ( IsNull( edef[0] = get_element_info(fdescs[0].edef->name) ) )
		{
		(void) fprintf(stderr, "[component_wind]  \"%s\"  not recognised!\n",
				SafeStr(fdescs[0].edef->name));
		return FALSE;
		}

	/* See if the first field descriptor refers to a vector or scalar field */
	switch ( edef[0]->fld_type )
		{
		case FpaC_VECTOR:
				UseVector = TRUE;
				break;

		case FpaC_CONTINUOUS:
				UseVector = FALSE;
				if ( IsNull( cmpnt[0] = edef[0]->elem_detail->components )
						|| cmpnt[0]->cinfo->need != XY_Comp )
					{
					(void) fprintf(stderr,
							"[component_wind]  \"%s\"  does not have",
							SafeStr(fdescs[0].edef->name));
					(void) fprintf(stderr, " u and v components!\n");
					return FALSE;
					}
				break;

		default:
				(void) fprintf(stderr,
						"[component_wind]  \"%s\"  does not have",
						SafeStr(fdescs[0].edef->name));
				(void) fprintf(stderr, " u and v components!\n");
				return FALSE;
		}


	/* Ensure that the second field descriptor (if it is given and needed) */
	/*  is also from a component field with u and v (x and y) components   */
	if ( !UseVector && nfds > 1 )
		{
		if ( IsNull( edef[1] = get_element_info(fdescs[1].edef->name) )
				|| IsNull( cmpnt[1] = edef[1]->elem_detail->components )
				|| cmpnt[1]->cinfo->need != XY_Comp )
			{
			(void) fprintf(stderr, "[component_wind]  \"%s\"  does not have",
					SafeStr(fdescs[1].edef->name));
			(void) fprintf(stderr, " u and v components!\n");
			return FALSE;
			}

		/* Ensure that the second field descriptor is a component field */
		/*  of the first field descriptor                               */
		for ( nc=0; nc<cmpnt[0]->ncomp; nc++ )
			{
			if ( edef[1] == cmpnt[0]->comp_edefs[nc] ) break;
			}
		if ( nc >= cmpnt[0]->ncomp )
			{
			(void) fprintf(stderr, "[component_wind]  \"%s\"  is not a u or v",
					SafeStr(fdescs[1].edef->name));
			(void) fprintf(stderr, " component of  \"%s\"!\n",
					SafeStr(fdescs[0].edef->name));
			return FALSE;
			}
		}

	/* Initialize VLIST Objects for wind data at all locations */
	Uvlist = NullPtr(VLIST *);
	Vvlist = NullPtr(VLIST *);

	/* Extract u and v components of wind at all locations   */
	/*  when a vector field is passed                        */
	if ( UseVector )
		{
		/* Reset valid time for field (if requested) */
		(void) copy_fld_descript(&descript, &fdescs[0]);
		if ( matched )
			{
			(void) matched_source_valid_time_reset(&descript,
					FpaC_TIMEDEP_ANY, descript.vtime);
			}

		/* Extract both components of wind at all locations */
		Uvlist = retrieve_vlist_component(&descript, X_Comp, npos, ppos);
		Vvlist = retrieve_vlist_component(&descript, Y_Comp, npos, ppos);
		}

	/* Extract u and v components of wind at all locations   */
	/*  when both components are passed as field descriptors */
	else if ( nfds > 1 )
		{
		for ( nn=0; nn<nfds; nn++ )
			{

			/* Extract u (x) or v (y) components of wind */
			for ( nc=0; nc<cmpnt[nn]->ncomp; nc++ )
				{

				/* U component of wind */
				if ( IsNull(Uvlist) && edef[nn] == cmpnt[nn]->comp_edefs[nc]
						&& cmpnt[nn]->comp_types[nc] == X_Comp )
					{

					/* Reset valid time for field (if requested) */
					(void) copy_fld_descript(&descript, &fdescs[nn]);
					if ( matched )
						{
						(void) matched_source_valid_time_reset(&descript,
								FpaC_TIMEDEP_ANY, descript.vtime);
						}

					/* Extract u (x) component of wind at all locations */
					Uvlist = retrieve_vlist(&descript, npos, ppos);
					}

				/* V component of wind */
				else if ( IsNull(Vvlist) && edef[nn] == cmpnt[nn]->comp_edefs[nc]
						&& cmpnt[nn]->comp_types[nc] == Y_Comp )
					{

					/* Reset valid time for field (if requested) */
					(void) copy_fld_descript(&descript, &fdescs[nn]);
					if ( matched )
						{
						(void) matched_source_valid_time_reset(&descript,
								FpaC_TIMEDEP_ANY, descript.vtime);
						}

					/* Extract v (y) component of wind at all locations */
					Vvlist = retrieve_vlist(&descript, npos, ppos);
					}
				}
			}
		}

	/* Extract u and v components of wind at all locations */
	/*  when only one component is passed                  */
	else
		{

		/* Reset valid time for field (if requested) */
		(void) copy_fld_descript(&descript, &fdescs[0]);
		if ( matched )
			{
			(void) matched_source_valid_time_reset(&descript,
					FpaC_TIMEDEP_ANY, descript.vtime);
			}

		/* Extract u (x) and v (y) components of wind */
		for ( nc=0; nc<cmpnt[0]->ncomp; nc++ )
			{

			/* Reset field descriptor for u or v component of wind */
			(void) set_fld_descript(&descript,
										FpaF_ELEMENT, cmpnt[0]->comp_edefs[nc],
										FpaF_END_OF_LIST);

			/* Extract u (x) component of wind at all locations */
			if ( IsNull(Uvlist) && cmpnt[0]->comp_types[nc] == X_Comp )
				{
				Uvlist = retrieve_vlist(&descript, npos, ppos);
				}

			/* Extract v (y) component of wind at all locations */
			else if ( IsNull(Vvlist) && cmpnt[0]->comp_types[nc] == Y_Comp )
				{
				Vvlist = retrieve_vlist(&descript, npos, ppos);
				}
			}
		}

	/* Return FALSE if u or v components cannot be found */
	if ( IsNull(Uvlist) || IsNull(Vvlist) )
		{
		if ( UseVector )
			{
			(void) fprintf(stderr, "[component_wind] Cannot retrieve u or v");
			(void) fprintf(stderr, " component data from  \"%s %s\"\n",
					SafeStr(fdescs[0].edef->name), SafeStr(fdescs[0].ldef->name));
			}
		else if ( nfds > 1 )
			{
			(void) fprintf(stderr, "[component_wind] Cannot retrieve u or v");
			(void) fprintf(stderr, " component data from  \"%s %s\"  and  \"%s %s\"\n",
					SafeStr(fdescs[0].edef->name), SafeStr(fdescs[0].ldef->name),
					SafeStr(fdescs[1].edef->name), SafeStr(fdescs[1].ldef->name));
			}
		else
			{
			(void) fprintf(stderr, "[component_wind] Cannot retrieve u or v");
			(void) fprintf(stderr, " component data from  \"%s %s\"\n",
					SafeStr(fdescs[0].edef->name), SafeStr(fdescs[0].ldef->name));
			}
		if ( NotNull(Uvlist) )
			{
			(void) free_vlist(Uvlist);
			FREEMEM(Uvlist);
			}
		if ( NotNull(Vvlist) )
			{
			(void) free_vlist(Vvlist);
			FREEMEM(Vvlist);
			}
		return FALSE;
		}

	/* Loop through all positions and extract wind parameters */
	for ( ipos=0; ipos<npos; ipos++ )
		{

		/* Set latitude and longitude for this position */
		if ( !pos_to_ll(&descript.mproj, ppos[ipos], &wlat, &wlon) )
			continue;

		/* Set wind speed and direction (degrees True ) */
		uval = (double) Uvlist->val[ipos];
		vval = (double) Vvlist->val[ipos];
		if ( NotNull(wdirs) )
			{
			dang = (fpa_atan2(vval, uval) / degtorad) + 180.0;
			wdirs[ipos] = wind_dir_true(&(descript.mproj), wlat, wlon,
											(float) dang);
			}
		if ( NotNull(wspds) || NotNull(wgsts) )
			{
			sval = hypot(uval, vval);
			if ( NotNull(wspds) )
				{
				wspds[ipos] = (float) sval;
				}
			if ( NotNull(wgsts) )
				{
				wgsts[ipos] = (float) sval;
				}
			}
		}

	/* Free space used by VLIST Objects */
	(void) free_vlist(Uvlist);
	FREEMEM(Uvlist);
	(void) free_vlist(Vvlist);
	FREEMEM(Vvlist);

#	ifdef TIME_WINDS
	if ( npos>1 )
		{
		long	nsec, nusec;
		get_stopwatch(&nsec, &nusec, NULL, NULL);
		(void) fprintf(stdout, "component wind: %d points %ld.%.6ld sec\n",
				npos, nsec, nusec);
		}
#	endif /* TIME_WINDS */

	/* Set units to m/s and return TRUE if all went well */
	if ( NotNull(units) ) *units = MperS;
	return TRUE;
	}

/**********************************************************************
 ***                                                                ***
 *** d i r _ s p d _ w i n d                                        ***
 ***                                                                ***
 *** determine wind speed and direction from the direction and      ***
 ***  speed component fields of the given wind                      ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL		dir_spd_wind

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on fields */
	POINT			*ppos,		/* pointer to npos positions on fields */
	float			clon,		/* center longitude for fields */
	float			*wdirs,		/* pointer to npos wind directions */
	float			*wspds,		/* pointer to npos wind speeds */
	float			*wgsts,		/* pointer to npos wind gusts */
	STRING			*units		/* units of wind speeds */
	)

	{
	int									nn, ipos, nc;
	float								wlat, wlon;
	double								dang, xdang;
	STRING								dirunits, direlem, dirlevel;
	LOGICAL								dirtrue;
	FpaConfigElementStruct				*edef[2];
	FpaConfigElementComponentStruct		*cmpnt[2];
	FpaConfigUnitStruct					*udef;
	FLD_DESCRIPT						descript;
	VLIST								*Dvlist, *Mvlist;

	/* Static buffers to speed up multiple calls */
	static	LOGICAL					DegSet = FALSE;
	static	FpaConfigUnitStruct		*Umks, *Ubmap, *Utrue;

	/* Check for correct number of field descriptors */
	if ( nfds < 1 )
		{
		(void) fprintf(stderr, "[dir_spd_wind] Error in");
		(void) fprintf(stderr, " number of field descriptors: %d\n", nfds);
		(void) fprintf(stderr, "   At least one field descriptor required");
		(void) fprintf(stderr, " (for direction or magnitude data)\n");
		return FALSE;
		}

	/* Return FALSE if no information in field descriptors */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	for ( nn=0; nn<nfds; nn++ )
		{
		if ( IsNull(fdescs[nn].sdef) || IsNull(fdescs[nn].subdef)
				|| IsNull(fdescs[nn].edef) || IsNull(fdescs[nn].ldef) )
			return FALSE;
		}

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in dir_spd_wind:\n");
		for ( nn=0; nn<nfds; nn++ )
			{
			dprintf(stdout,"   path: %s\n",       fdescs[nn].path);
			dprintf(stdout,"   source: %s %s\n",  fdescs[nn].sdef->name,
													fdescs[nn].subdef->name);
			dprintf(stdout,"   rtime: %s\n",      fdescs[nn].rtime);
			dprintf(stdout,"   vtime: %s\n",      fdescs[nn].vtime);
			dprintf(stdout,"   function name: %s\n",
													fdescs[nn].wind_func_name);
			dprintf(stdout,"   element: %s\n",    fdescs[nn].edef->name);
			dprintf(stdout,"   level: %s\n",      fdescs[nn].ldef->name);
			}
		}

	/* Get pointers for units identifiers only once! */
	if ( !DegSet )
		{
		Umks  = identify_unit(FpaCmksUnits);
		Ubmap = identify_unit(DegreesBmap);
		Utrue = identify_unit(DegreesTrue);
		DegSet   = TRUE;
		}

#	ifdef TIME_WINDS
	if ( npos>1 )
		{
		set_stopwatch(TRUE);
		}
#	endif /* TIME_WINDS */

	/* Initialize return parameters */
	for ( ipos=0; ipos<npos; ipos++ )
		{
		if ( NotNull(wdirs) ) wdirs[ipos] = 0.0;
		if ( NotNull(wspds) ) wspds[ipos] = 0.0;
		if ( NotNull(wgsts) ) wgsts[ipos] = 0.0;
		}
	if ( NotNull(units) ) *units = NullString;

	/* Ensure that the first field descriptor is from a component field */
	/*  with direction and magnitude components                         */
	if ( IsNull( edef[0] = get_element_info(fdescs[0].edef->name) )
			|| IsNull( cmpnt[0] = edef[0]->elem_detail->components )
			|| cmpnt[0]->cinfo->need != DM_Comp )
		{
		(void) fprintf(stderr, "[dir_spd_wind]  \"%s\"  does not have",
				SafeStr(fdescs[0].edef->name));
		(void) fprintf(stderr, " direction and magnitude components!\n");
		return FALSE;
		}

	/* Ensure that the second field descriptor (if it is given) is also */
	/*  from a component field with direction and magnitude components  */
	if ( nfds > 1 )
		{
		if ( IsNull( edef[1] = get_element_info(fdescs[1].edef->name) )
				|| IsNull( cmpnt[1] = edef[1]->elem_detail->components )
				|| cmpnt[1]->cinfo->need != DM_Comp )
			{
			(void) fprintf(stderr, "[dir_spd_wind]  \"%s\"  does not have",
					SafeStr(fdescs[1].edef->name));
			(void) fprintf(stderr, " direction and magnitude components!\n");
			return FALSE;
			}

		/* Ensure that the second field descriptor is a component field */
		/*  of the first field descriptor                               */
		for ( nc=0; nc<cmpnt[0]->ncomp; nc++ )
			{
			if ( edef[1] == cmpnt[0]->comp_edefs[nc] ) break;
			}
		if ( nc >= cmpnt[0]->ncomp )
			{
			(void) fprintf(stderr, "[dir_spd_wind]  \"%s\"  is not a direction or",
					SafeStr(fdescs[1].edef->name));
			(void) fprintf(stderr, " magnitude component of  \"%s\"!\n",
					SafeStr(fdescs[0].edef->name));
			return FALSE;
			}
		}

	/* Initialize VLIST Objects for wind data at all locations */
	Dvlist = NullPtr(VLIST *);
	Mvlist = NullPtr(VLIST *);

	/* Extract direction and magnitude components of wind at all locations */
	/*  when both components are passed as field descriptors               */
	if ( nfds > 1 )
		{
		for ( nn=0; nn<nfds; nn++ )
			{

			/* Extract direction or magnitude components of wind */
			for ( nc=0; nc<cmpnt[nn]->ncomp; nc++ )
				{

				/* Direction component of wind */
				if ( IsNull(Dvlist) && edef[nn] == cmpnt[nn]->comp_edefs[nc]
						&& cmpnt[nn]->comp_types[nc] == D_Comp )
					{

					/* Reset valid time for field (if requested) */
					(void) copy_fld_descript(&descript, &fdescs[nn]);
					if ( matched )
						{
						(void) matched_source_valid_time_reset(&descript,
								FpaC_TIMEDEP_ANY, descript.vtime);
						}

					/* Extract direction component of wind at all locations */
					/*  and set units for direction component of wind       */
					Dvlist   = retrieve_vlist(&descript, npos, ppos);
					dirunits = cmpnt[nn]->comp_edefs[nc]->elem_io->units->name;
					direlem  = descript.edef->name;
					dirlevel = descript.ldef->name;
					}

				/* Magnitude component of wind */
				else if ( IsNull(Mvlist) && edef[nn] == cmpnt[nn]->comp_edefs[nc]
						&& cmpnt[nn]->comp_types[nc] == M_Comp )
					{

					/* Reset valid time for field (if requested) */
					(void) copy_fld_descript(&descript, &fdescs[nn]);
					if ( matched )
						{
						(void) matched_source_valid_time_reset(&descript,
								FpaC_TIMEDEP_ANY, descript.vtime);
						}

					/* Extract magnitude component of wind at all locations */
					Mvlist = retrieve_vlist(&descript, npos, ppos);
					}
				}
			}
		}

	/* Extract direction and magnitude components of wind at all locations */
	/*  when only one component is passed                                  */
	else
		{

		/* Reset valid time for field (if requested) */
		(void) copy_fld_descript(&descript, &fdescs[0]);
		if ( matched )
			{
			(void) matched_source_valid_time_reset(&descript,
					FpaC_TIMEDEP_ANY, descript.vtime);
			}

		/* Extract direction and magnitude components of wind */
		for ( nc=0; nc<cmpnt[0]->ncomp; nc++ )
			{

			/* Reset field descriptor for direction or magnitude component */
			(void) set_fld_descript(&descript,
										FpaF_ELEMENT, cmpnt[0]->comp_edefs[nc],
										FpaF_END_OF_LIST);

			/* Extract direction component of wind at all locations */
			/*  and set units for direction component of wind       */
			if ( IsNull(Dvlist) && cmpnt[0]->comp_types[nc] == D_Comp )
				{
				Dvlist   = retrieve_vlist(&descript, npos, ppos);
				dirunits = cmpnt[0]->comp_edefs[nc]->elem_io->units->name;
				direlem  = descript.edef->name;
				dirlevel = descript.ldef->name;
				}

			/* Extract magnitude component of wind at all locations */
			else if ( IsNull(Mvlist) && cmpnt[0]->comp_types[nc] == M_Comp )
				{
				Mvlist = retrieve_vlist(&descript, npos, ppos);
				}
			}
		}

	/* Return FALSE if direction or magnitude components cannot be found */
	if ( IsNull(Dvlist) || IsNull(Mvlist) )
		{
		if ( nfds > 1 )
			{
			(void) fprintf(stderr, "[dir_spd_wind] Cannot retrieve direction or");
			(void) fprintf(stderr, " magnitude data from  \"%s %s\"  and  \"%s %s\"\n",
					SafeStr(fdescs[0].edef->name), SafeStr(fdescs[0].ldef->name),
					SafeStr(fdescs[1].edef->name), SafeStr(fdescs[1].ldef->name));
			}
		else
			{
			(void) fprintf(stderr, "[dir_spd_wind] Cannot retrieve direction or");
			(void) fprintf(stderr, " magnitude data from  \"%s %s\"\n",
					SafeStr(fdescs[0].edef->name), SafeStr(fdescs[0].ldef->name));
			}
		if ( NotNull(Dvlist) )
			{
			(void) free_vlist(Dvlist);
			FREEMEM(Dvlist);
			}
		if ( NotNull(Mvlist) )
			{
			(void) free_vlist(Mvlist);
			FREEMEM(Mvlist);
			}
		return FALSE;
		}

	/* Determine whether wind direction is in units of DegreesTrue */
	/*  or in units of  DegreesBmap                                */
	udef = identify_unit(dirunits);
	if ( IsNull(udef) || !udef->valid )
		{
		(void) fprintf(stderr,
				"[dir_spd_wind] Unknown wind units  \"%s\"",
				SafeStr(dirunits));
		(void) fprintf(stderr, "  from  \"%s %s\"\n",
				SafeStr(direlem), SafeStr(dirlevel));
		if ( NotNull(Dvlist) )
			{
			(void) free_vlist(Dvlist);
			FREEMEM(Dvlist);
			}
		if ( NotNull(Mvlist) )
			{
			(void) free_vlist(Mvlist);
			FREEMEM(Mvlist);
			}
		return FALSE;
		}
	else if ( NotNull(Umks) && Umks->valid && same(udef->MKS, Umks->MKS) )
		{
		dirtrue = TRUE;
		}
	else if ( NotNull(Ubmap) && Ubmap->valid && same(udef->MKS, Ubmap->MKS) )
		{
		dirtrue = FALSE;
		}
	else if ( NotNull(Utrue) && Utrue->valid && same(udef->MKS, Utrue->MKS) )
		{
		dirtrue = TRUE;
		}
	else
		{
		(void) fprintf(stderr,
				"[dir_spd_wind] Unacceptable wind units  \"%s\"",
				SafeStr(dirunits));
		(void) fprintf(stderr, "  from  \"%s %s\"\n",
				SafeStr(direlem), SafeStr(dirlevel));
		if ( NotNull(Dvlist) )
			{
			(void) free_vlist(Dvlist);
			FREEMEM(Dvlist);
			}
		if ( NotNull(Mvlist) )
			{
			(void) free_vlist(Mvlist);
			FREEMEM(Mvlist);
			}
		return FALSE;
		}

	/* Loop through all positions and extract wind parameters */
	for ( ipos=0; ipos<npos; ipos++ )
		{

		/* Set latitude and longitude for this position */
		if ( !pos_to_ll(&descript.mproj, ppos[ipos], &wlat, &wlon) )
			continue;

		/* Set wind speed and direction (degrees True ) */
		if ( NotNull(wdirs) )
			{

			/* Set wind direction if direction is in units of DegreesTrue */
			if ( dirtrue )
				{
				dang = (double) Dvlist->val[ipos];
				(void) convert_value(dirunits, dang, DegreesTrue, &xdang);
				wdirs[ipos] = (float) xdang;
				}

			/* Set wind direction if direction is in units of DegreesBmap */
			else
				{
				dang = (double) Dvlist->val[ipos];
				(void) convert_value(dirunits, dang, DegreesBmap, &xdang);
				wdirs[ipos] = wind_dir_true(&(descript.mproj), wlat, wlon,
												(float) xdang);
				}
			}
		if ( NotNull(wspds) )
			{
			wspds[ipos] = Mvlist->val[ipos];
			}
		if ( NotNull(wgsts) )
			{
			wgsts[ipos] = Mvlist->val[ipos];
			}
		}

	/* Free space used by VLIST Objects */
	(void) free_vlist(Dvlist);
	FREEMEM(Dvlist);
	(void) free_vlist(Mvlist);
	FREEMEM(Mvlist);

#	ifdef TIME_WINDS
	if ( npos>1 )
		{
		long	nsec, nusec;
		get_stopwatch(&nsec, &nusec, NULL, NULL);
		(void) fprintf(stdout, "direction/speed wind: %d points %ld.%.6ld sec\n",
				npos, nsec, nusec);
		}
#	endif /* TIME_WINDS */

	/* Set units to m/s and return TRUE if all went well */
	if ( NotNull(units) ) *units = MperS;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Checking Types of Wind)                 *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/**********************************************************************
 ***                                                                ***
 *** c h e c k _ f p a _ w i n d                                    ***
 ***                                                                ***
 *** check whether the types of wind from a wind areaset field can  ***
 ***  be extracted                                                  ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL		check_fpa_wind

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on field */
	POINT			*ppos		/* pointer to npos positions on field */
	)

	{
	FLD_DESCRIPT	descript;
	int				ipos, ityp;
	LOGICAL			unique;

	/* Internal buffers for wind areaset data */
	static	STRING	*WTypes  = NullStringList;
	static	STRING	*WValues = NullStringList;
	static	int		NumTypes = 0;
	static	STRING	*WindTypes = NullStringList;

	/* Check for correct number of field descriptors */
	if ( nfds != 1 )
		{
		(void) fprintf(stderr, "[check_fpa_wind] Error in");
		(void) fprintf(stderr, " number of field descriptors: %d\n", nfds);
		(void) fprintf(stderr, "   Only one field descriptor allowed (for data)\n");
		return FALSE;
		}

	/* Return FALSE if no information in field descriptor */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	if ( IsNull(fdescs[0].sdef) || IsNull(fdescs[0].subdef)
			|| IsNull(fdescs[0].edef) || IsNull(fdescs[0].ldef) ) return FALSE;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in check_fpa_wind:\n");
		dprintf(stdout,"   path: %s\n",       fdescs[0].path);
		dprintf(stdout,"   source: %s %s\n",  fdescs[0].sdef->name,
												fdescs[0].subdef->name);
		dprintf(stdout,"   rtime: %s\n",      fdescs[0].rtime);
		dprintf(stdout,"   vtime: %s\n",      fdescs[0].vtime);
		dprintf(stdout,"   function name: %s\n",
												fdescs[0].wind_func_name);
		dprintf(stdout,"   element: %s\n",    fdescs[0].edef->name);
		dprintf(stdout,"   level: %s\n",      fdescs[0].ldef->name);
		}

	/* Reset valid time for field (if requested) */
	(void) copy_fld_descript(&descript, &fdescs[0]);
	if ( matched )
		{
		(void) matched_source_valid_time_reset(&descript,
				FpaC_TIMEDEP_ANY, descript.vtime);
		}

	/* Allocate space for wind areaset data */
	WTypes  = GETMEM(WTypes,  STRING, npos);
	WValues = GETMEM(WValues, STRING, npos);

	/* Return data from wind areaset (if found) or background wind */
	if ( !extract_areaset_value(1, &descript, matched,
			npos, ppos, 0.0, WTypes, WValues, NullStringList) )
		{
		if ( CheckWindErrorMessages )
			{
			(void) fprintf(stderr, "[check_fpa_wind] Cannot");
			(void) fprintf(stderr, " extract a wind or background wind\n");
			(void) fprintf(stderr, "   for field: \"%s %s\"\n",
					SafeStr(descript.edef->name), SafeStr(descript.ldef->name));
			(void) fprintf(stderr, "   from source: \"%s %s\"",
					SafeStr(descript.sdef->name), SafeStr(descript.subdef->name));
			(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
					SafeStr(descript.rtime), SafeStr(descript.vtime));
			}
		return FALSE;
		}

	/* Loop through all positions and save unique wind types */
	for ( NumTypes=0, ipos=0; ipos<npos; ipos++ )
		{

		/* Identify and save unique wind types */
		for ( unique=TRUE, ityp=0; ityp<NumTypes; ityp++ )
			{
			if ( same(WTypes[ipos], WindTypes[ityp]) )
				{
				unique = FALSE;
				break;
				}
			}
		if ( unique )
			{
			WindTypes = GETMEM(WindTypes,  STRING, ++NumTypes);
			WindTypes[NumTypes-1] = strdup(WTypes[ipos]);
			}
		}

	/* Check that all unique wind types can be extracted */
	/* (Note that only the first position is passed!)    */
	for ( ityp=0; ityp<NumTypes; ityp++ )
		{
		if ( same(WindTypes[ityp], FpaAbsWindModel) ) continue;
		if ( !check_extract_wind_by_crossref(WindTypes[ityp], &descript,
				matched, 1, ppos) ) return FALSE;
		}

	/* Return TRUE if all unique wind types can be extracted */
	return TRUE;
	}

/**********************************************************************
 ***                                                                ***
 *** c h e c k _ o t h e r _ w i n d                                ***
 ***                                                                ***
 *** check if the fields required to extract a given type of wind   ***
 ***  can be found                                                  ***
 ***                                                                ***
 *** Note that individual locations are not usually checked!        ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL		check_other_wind

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on field */
	POINT			*ppos		/* pointer to npos positions on field */
	)

	{
	int						nn;
	FLD_DESCRIPT			descript;
	FpaConfigElementStruct	*edef;

	/* Return FALSE if no information in field descriptors */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	for ( nn=0; nn<nfds; nn++ )
		{
		if ( IsNull(fdescs[nn].sdef) || IsNull(fdescs[nn].subdef)
				|| IsNull(fdescs[nn].edef) || IsNull(fdescs[nn].ldef) )
			return FALSE;
		}

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in check_other_wind:\n");
		for ( nn=0; nn<nfds; nn++ )
			{
			dprintf(stdout,"   path: %s\n",       fdescs[nn].path);
			dprintf(stdout,"   source: %s %s\n",  fdescs[nn].sdef->name,
													fdescs[nn].subdef->name);
			dprintf(stdout,"   rtime: %s\n",      fdescs[nn].rtime);
			dprintf(stdout,"   vtime: %s\n",      fdescs[nn].vtime);
			dprintf(stdout,"   function name: %s\n",
													fdescs[nn].wind_func_name);
			dprintf(stdout,"   element: %s\n",    fdescs[nn].edef->name);
			dprintf(stdout,"   level: %s\n",      fdescs[nn].ldef->name);
			}
		}

	/* Check for fields for all field descriptors */
	for ( nn=0; nn<nfds; nn++ )
		{

		/* Reset valid time for field (if requested) */
		(void) copy_fld_descript(&descript, &fdescs[nn]);
		if ( matched )
			{
			(void) matched_source_valid_time_reset(&descript,
					FpaC_TIMEDEP_ANY, descript.vtime);
			}

		/* Return FALSE if the field cannot be found */
		if ( !find_retrieve_metasfc(&descript) )
			{
			if ( CheckWindErrorMessages )
				{
				(void) fprintf(stderr, "[check_other_wind] Missing");
				(void) fprintf(stderr, " required field: \"%s %s\"\n",
						SafeStr(descript.edef->name),
						SafeStr(descript.ldef->name));
				(void) fprintf(stderr, "   from source: \"%s %s\"",
						SafeStr(descript.sdef->name),
						SafeStr(descript.subdef->name));
				(void) fprintf(stderr, "  at runtime: \"%s\"",
						SafeStr(descript.rtime));
				(void) fprintf(stderr, "  validtime: \"%s\"\n",
						SafeStr(descript.vtime));
				(void) fprintf(stderr, "                       for");
				(void) fprintf(stderr, " wind function: \"%s\"\n",
						SafeStr(descript.wind_func_name));
				}
			return FALSE;
			}

		/* Get detailed element information */
		edef = get_element_info(descript.edef->name);
		if ( IsNull(edef) ) return FALSE;

		/* Branch to special checks based on the type of field */
		switch ( edef->fld_type )
			{

			/* Check that WIND type fields can be evaluated */
			case FpaC_WIND:
				if ( !check_fpa_wind(1, &descript, matched, npos, ppos) )
						return FALSE;
				break;
			}
		}

	/* Return TRUE if all fields were found */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Testing static routines)                *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

#if defined WINDS_STANDALONE

/* Internal variables to test wind routines */
static	int			TestNumber       = 1;
static	POINT		TestPointArray[] = { ZERO_POINT };

/**********************************************************************
 *** routine to test extract_awind                                  ***
 **********************************************************************/

static	void		test_extract_awind

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	POINT			pos			/* input position on field */
	)

	{
	float		wspd, wgst, wdir;
	STRING		units;

	(void) fprintf(stdout, "\n Point at  xpos: %7.1f", pos[X]);
	(void) fprintf(stdout, "  ypos: %7.1f\n", pos[Y]);
	if ( !inside_map_def(&fdescs[0].mproj.definition, pos)
			|| !inside_grid_def(&fdescs[0].mproj.grid, pos) )
		{
		(void) fprintf(stdout, "      Error in position\n");
		}
	(void) copy_point(TestPointArray[0], pos);
	if ( extract_awind(nfds, fdescs, matched,
			TestNumber, TestPointArray, 0.0, &wdir, &wspd, &wgst, &units) )
		{
		(void) fprintf(stdout, " Speed/Gust: %7.2f %7.2f", wspd, wgst);
		(void) fprintf(stdout, "   Units: %s", units);
		(void) fprintf(stdout, "   Direction (degrees): %7.2f\n", wdir);
		}
	else
		{
		(void) fprintf(stdout, " Error in wind function: %s\n",
				fdescs[0].wind_func_name);
		}
	}

/**********************************************************************
 *** routine to test extract_awind_by_crossref                      ***
 **********************************************************************/

static	void		test_extract_awind_by_crossref

	(
	STRING			windcref,	/* wind cross reference */
	FLD_DESCRIPT	*fdesc,		/* pointer to field descriptor */
	LOGICAL			matched,	/* match valid times to available fields? */
	POINT			pos			/* input position on field */
	)

	{
	float		wspd, wgst, wdir;
	STRING		units;

	(void) fprintf(stdout, "\n Point at  xpos: %7.1f", pos[X]);
	(void) fprintf(stdout, "  ypos: %7.1f\n", pos[Y]);
	if ( !inside_map_def(&fdesc->mproj.definition, pos)
			|| !inside_grid_def(&fdesc->mproj.grid, pos) )
		{
		(void) fprintf(stdout, "      Error in position\n");
		}
	(void) copy_point(TestPointArray[0], pos);
	if ( extract_awind_by_crossref(windcref, fdesc, matched,
			TestNumber, TestPointArray, 0.0, &wdir, &wspd, &wgst, &units) )
		{
		(void) fprintf(stdout, " Speed/Gust: %7.2f %7.2f", wspd, wgst);
		(void) fprintf(stdout, "   Units: %s", units);
		(void) fprintf(stdout, "   Direction (degrees): %7.2f\n", wdir);
		}
	else
		{
		(void) fprintf(stdout, " Error in wind function: %s\n",
				fdesc->wind_func_name);
		}
	}

/**********************************************************************
 *** routine to test check_extract_wind                             ***
 **********************************************************************/

static	void		test_check_extract_wind

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	POINT			pos			/* input position on field */
	)

	{

	(void) fprintf(stdout, "\n Point at  xpos: %7.1f", pos[X]);
	(void) fprintf(stdout, "  ypos: %7.1f\n", pos[Y]);
	(void) fprintf(stdout, "   Source: %s %s",
			fdescs[0].sdef->name, fdescs[0].subdef->name);
	(void) fprintf(stdout, "   Runtime: %s\n", fdescs[0].rtime);
	(void) fprintf(stdout, "   Function name: %s", fdescs[0].wind_func_name);
	(void) fprintf(stdout, "   Element: %s   Level: %s\n",
			fdescs[0].edef->name, fdescs[0].ldef->name);
	if ( !inside_map_def(&fdescs[0].mproj.definition, pos)
			|| !inside_grid_def(&fdescs[0].mproj.grid, pos) )
		{
		(void) fprintf(stdout, "      Error in position\n");
		}
	(void) copy_point(TestPointArray[0], pos);
	if ( check_extract_wind(nfds, fdescs, matched, TestNumber, TestPointArray) )
		{
		(void) fprintf(stdout, " Wind can be extracted\n");
		}
	else
		{
		(void) fprintf(stdout, " No fields for wind\n");
		}
	}

/**********************************************************************
 *** routine to test check_extract_wind_by_crossref                 ***
 **********************************************************************/

static	void		test_check_extract_wind_by_crossref

	(
	STRING			windcref,	/* wind cross reference */
	FLD_DESCRIPT	*fdesc,		/* pointer to field descriptor */
	LOGICAL			matched,	/* match valid times to available fields? */
	POINT			pos			/* input position on field */
	)

	{

	(void) fprintf(stdout, "\n Point at  xpos: %7.1f", pos[X]);
	(void) fprintf(stdout, "  ypos: %7.1f\n", pos[Y]);
	(void) fprintf(stdout, "   Source: %s %s",
			fdesc->sdef->name, fdesc->subdef->name);
	(void) fprintf(stdout, "   Runtime: %s\n", fdesc->rtime);
	(void) fprintf(stdout, "   Wind crossref: %s", windcref);
	(void) fprintf(stdout, "   Element: %s   Level: %s\n",
			fdesc->edef->name, fdesc->ldef->name);
	if ( !inside_map_def(&fdesc->mproj.definition, pos)
			|| !inside_grid_def(&fdesc->mproj.grid, pos) )
		{
		(void) fprintf(stdout, "      Error in position\n");
		}
	(void) copy_point(TestPointArray[0], pos);
	if ( check_extract_wind_by_crossref(windcref, fdesc, matched,
			TestNumber, TestPointArray) )
		{
		(void) fprintf(stdout, " Wind can be extracted\n");
		}
	else
		{
		(void) fprintf(stdout, " No fields for wind\n");
		}
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
int				nsetup;
STRING			setupfile, *setuplist, path;
MAP_PROJ		*mproj;
STRING			windfunc, crossref;
STRING			source, subsource, rtime, vtime, element, level;
FLD_DESCRIPT	fdesc;
FLD_DESCRIPT	cfdescs[3];
POINT			pos;

/* Set Defaults for WINDS_STANDALONE */

/* ... First set the default output units */
(void) setvbuf(stdout, NullString, _IOLBF, 0);
(void) setvbuf(stderr, NullString, _IOLBF, 0);

/* ... Next get setup file for testing */
(void) fpalib_license(FpaAccessLib);
setupfile = strdup(argv[1]);
(void) fprintf(stdout, "Setup File: %s\n", setupfile);
nsetup = setup_files(setupfile, &setuplist);
if ( !define_setup(nsetup, setuplist) )
	{
	(void) fprintf(stderr, "Fatal problem with Setup File: %s\n", setupfile);
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

/* ... Next set Default Pathname (set to the Home directory) */
path = home_directory();

/* Testing for geostrophic_wind */
(void) fprintf(stdout, "\n\n *** Testing for geostrophic_wind ***\n");
(void) clear_equation_database();
(void) init_fld_descript(&fdesc);
(void) set_fld_descript(&fdesc, FpaF_MAP_PROJECTION, mproj,
						FpaF_END_OF_LIST);

source   = "fem";			subsource = "";
windfunc = "FPA_Geostrophic_Wind_Func";
element  = "p";				level     = "msl";
rtime    = strdup(build_tstamp(1991, 238, 12, 00, FALSE, FALSE));
vtime    = strdup(build_tstamp(1991, 239, 00, 00, FALSE, FALSE));
pos[X] = 0.0;				pos[Y] = 0.0;
(void) set_fld_descript(&fdesc, FpaF_DIRECTORY_PATH, path,
						FpaF_SOURCE_NAME, source,
						FpaF_SUBSOURCE_NAME, subsource,
						FpaF_RUN_TIME, rtime,
						FpaF_WIND_FUNCTION_NAME, windfunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
						FpaF_END_OF_LIST);
(void) test_extract_awind(1, &fdesc, TRUE, pos);

pos[X] = 100.0;				pos[Y] = 100.0;
(void) test_extract_awind(1, &fdesc, TRUE, pos);

source   = "Spectral";		subsource = "";
(void) set_fld_descript(&fdesc, FpaF_DIRECTORY_PATH, path,
						FpaF_SOURCE_NAME, source,
						FpaF_SUBSOURCE_NAME, subsource,
						FpaF_RUN_TIME, rtime,
						FpaF_END_OF_LIST);
(void) test_extract_awind(1, &fdesc, TRUE, pos);

source   = "fem";			subsource = "";
element  = "gz";			level     = "850";
pos[X] = 0.0;				pos[Y] = 0.0;
(void) set_fld_descript(&fdesc, FpaF_DIRECTORY_PATH, path,
						FpaF_SOURCE_NAME, source,
						FpaF_SUBSOURCE_NAME, subsource,
						FpaF_RUN_TIME, rtime,
						FpaF_WIND_FUNCTION_NAME, windfunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
						FpaF_END_OF_LIST);
(void) test_extract_awind(1, &fdesc, TRUE, pos);

pos[X] = 100.0;				pos[Y] = 100.0;
(void) test_extract_awind(1, &fdesc, TRUE, pos);

source   = "Spectral";		subsource = "";
(void) set_fld_descript(&fdesc, FpaF_DIRECTORY_PATH, path,
						FpaF_SOURCE_NAME, source,
						FpaF_SUBSOURCE_NAME, subsource,
						FpaF_RUN_TIME, rtime,
						FpaF_END_OF_LIST);
(void) test_extract_awind(1, &fdesc, TRUE, pos);

windfunc = "FPA_Thermal_Wind_Func";
element  = "thick";			level     = "851K";
(void) set_fld_descript(&fdesc,
						FpaF_WIND_FUNCTION_NAME, windfunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_END_OF_LIST);
(void) test_extract_awind(1, &fdesc, TRUE, pos);

/* Testing for isallobaric_wind */
(void) fprintf(stdout, "\n\n *** Testing for isallobaric_wind ***\n");
(void) clear_equation_database();

source   = "fem";			subsource = "";
windfunc = "FPA_Isallobaric_Wind_Func";
element  = "dp";			level     = "msl";
pos[X] = 0.0;				pos[Y] = 0.0;
(void) set_fld_descript(&fdesc, FpaF_DIRECTORY_PATH, path,
						FpaF_SOURCE_NAME, source,
						FpaF_SUBSOURCE_NAME, subsource,
						FpaF_RUN_TIME, rtime,
						FpaF_WIND_FUNCTION_NAME, windfunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
						FpaF_END_OF_LIST);
(void) test_extract_awind(1, &fdesc, TRUE, pos);

pos[X] = 100.0;				pos[Y] = 100.0;
(void) test_extract_awind(1, &fdesc, TRUE, pos);

source   = "Spectral";		subsource = "";
(void) set_fld_descript(&fdesc, FpaF_DIRECTORY_PATH, path,
						FpaF_SOURCE_NAME, source,
						FpaF_SUBSOURCE_NAME, subsource,
						FpaF_RUN_TIME, rtime,
						FpaF_END_OF_LIST);
(void) test_extract_awind(1, &fdesc, TRUE, pos);

pos[X] = 1000.0;			pos[Y] = 1000.0;
(void) test_extract_awind(1, &fdesc, TRUE, pos);

/* Testing for gradient_wind */
(void) fprintf(stdout, "\n\n ***Testing for gradient_wind***\n");
(void) clear_equation_database();

source   = "fem";			subsource = "";
windfunc = "FPA_Gradient_Wind_Func";
element  = "p";				level     = "msl";
pos[X] = 0.0;				pos[Y] = 0.0;
(void) set_fld_descript(&fdesc, FpaF_DIRECTORY_PATH, path,
						FpaF_SOURCE_NAME, source,
						FpaF_SUBSOURCE_NAME, subsource,
						FpaF_RUN_TIME, rtime,
						FpaF_WIND_FUNCTION_NAME, windfunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
						FpaF_END_OF_LIST);
(void) test_extract_awind(1, &fdesc, TRUE, pos);

pos[X] = 100.0;				pos[Y] = 100.0;
(void) test_extract_awind(1, &fdesc, TRUE, pos);

source   = "Spectral";		subsource = "";
(void) set_fld_descript(&fdesc, FpaF_DIRECTORY_PATH, path,
						FpaF_SOURCE_NAME, source,
						FpaF_SUBSOURCE_NAME, subsource,
						FpaF_RUN_TIME, rtime,
						FpaF_END_OF_LIST);
(void) test_extract_awind(1, &fdesc, TRUE, pos);

source   = "fem";			subsource = "";
element  = "gz";			level     = "850";
pos[X] = 0.0;				pos[Y] = 0.0;
(void) set_fld_descript(&fdesc, FpaF_DIRECTORY_PATH, path,
						FpaF_SOURCE_NAME, source,
						FpaF_SUBSOURCE_NAME, subsource,
						FpaF_RUN_TIME, rtime,
						FpaF_WIND_FUNCTION_NAME, windfunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
						FpaF_END_OF_LIST);
(void) test_extract_awind(1, &fdesc, TRUE, pos);

pos[X] = 100.0;				pos[Y] = 100.0;
(void) test_extract_awind(1, &fdesc, TRUE, pos);

source   = "Spectral";		subsource = "";
(void) set_fld_descript(&fdesc, FpaF_DIRECTORY_PATH, path,
						FpaF_SOURCE_NAME, source,
						FpaF_SUBSOURCE_NAME, subsource,
						FpaF_RUN_TIME, rtime,
						FpaF_END_OF_LIST);
(void) test_extract_awind(1, &fdesc, TRUE, pos);

/* Testing for fpa_adjusted_wind */
(void) fprintf(stdout, "\n\n *** Testing for fpa_adjusted_wind ***\n");
(void) clear_equation_database();

source   = "depict";		subsource = "";
windfunc = "FPA_Adjusted_Wind_Func";
element  = "wind";			level     = "sfc";
pos[X] = 0.0;				pos[Y] = 0.0;
(void) set_fld_descript(&fdesc, FpaF_DIRECTORY_PATH, path,
						FpaF_SOURCE_NAME, source,
						FpaF_SUBSOURCE_NAME, subsource,
						FpaF_RUN_TIME, rtime,
						FpaF_WIND_FUNCTION_NAME, windfunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
						FpaF_END_OF_LIST);
(void) test_extract_awind(1, &fdesc, TRUE, pos);

pos[X] = 100.0;				pos[Y] = 100.0;
(void) test_extract_awind(1, &fdesc, TRUE, pos);

crossref = "Fpa_Wind";
windfunc = "";
element  = "";				level     = "";
(void) set_fld_descript(&fdesc,
						FpaF_WIND_FUNCTION_NAME, windfunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
						FpaF_END_OF_LIST);
(void) test_extract_awind_by_crossref(crossref, &fdesc, TRUE, pos);

pos[X] = 1000.0;			pos[Y] = 1000.0;
(void) test_extract_awind_by_crossref(crossref, &fdesc, TRUE, pos);

/* Testing for component_wind */
(void) fprintf(stdout, "\n\n *** Testing for component_wind ***\n");
(void) clear_equation_database();

source   = "JMA";			subsource = "";
windfunc = "FPA_UVcomponent_Wind_Func";
rtime    = strdup(build_tstamp(1993, 247, 00, 00, FALSE, FALSE));
vtime    = strdup(build_tstamp(1993, 248, 00, 00, FALSE, FALSE));
(void) set_fld_descript(&fdesc, FpaF_DIRECTORY_PATH, path,
						FpaF_SOURCE_NAME, source,
						FpaF_SUBSOURCE_NAME, subsource,
						FpaF_RUN_TIME, rtime,
						FpaF_WIND_FUNCTION_NAME, windfunc,
						FpaF_VALID_TIME, vtime,
						FpaF_END_OF_LIST);
element  = "temperature";	level     = "500mb";
(void) copy_fld_descript(&cfdescs[0], &fdesc);
(void) set_fld_descript(&cfdescs[0],
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_END_OF_LIST);
element  = "u_wind";		level     = "500mb";
(void) copy_fld_descript(&cfdescs[1], &fdesc);
(void) set_fld_descript(&cfdescs[1],
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_END_OF_LIST);
(void) test_extract_awind(2, cfdescs, TRUE, pos);

element  = "u_wind";		level     = "500mb";
(void) copy_fld_descript(&cfdescs[0], &fdesc);
(void) set_fld_descript(&cfdescs[0],
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_END_OF_LIST);
(void) test_extract_awind(2, cfdescs, TRUE, pos);

element  = "v_wind";		level     = "500mb";
(void) copy_fld_descript(&cfdescs[0], &fdesc);
(void) set_fld_descript(&cfdescs[0],
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_END_OF_LIST);
(void) test_extract_awind(2, cfdescs, TRUE, pos);

pos[X] = 100.0;				pos[Y] = 100.0;
(void) test_extract_awind(2, cfdescs, TRUE, pos);

crossref = "Vx_Msl";
windfunc = "";
element  = "";				level     = "";
(void) set_fld_descript(&fdesc,
						FpaF_WIND_FUNCTION_NAME, windfunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
						FpaF_END_OF_LIST);
(void) test_extract_awind_by_crossref(crossref, &fdesc, TRUE, pos);

pos[X] = 1000.0;			pos[Y] = 1000.0;
(void) test_extract_awind_by_crossref(crossref, &fdesc, TRUE, pos);

/* Testing for check_extract_wind */
(void) fprintf(stdout, "\n\n *** Testing for check_extract_wind ***\n");
(void) clear_equation_database();

source   = "fem";			subsource = "";
windfunc = "FPA_Geostrophic_Wind_Func";
element  = "gz";			level     = "850";
rtime    = strdup(build_tstamp(1991, 238, 12, 00, FALSE, FALSE));
vtime    = strdup(build_tstamp(1991, 239, 00, 00, FALSE, FALSE));
pos[X] = 0.0;				pos[Y] = 0.0;
(void) set_fld_descript(&fdesc, FpaF_DIRECTORY_PATH, path,
						FpaF_SOURCE_NAME, source,
						FpaF_SUBSOURCE_NAME, subsource,
						FpaF_RUN_TIME, rtime,
						FpaF_WIND_FUNCTION_NAME, windfunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
						FpaF_END_OF_LIST);
(void) test_check_extract_wind(1, &fdesc, TRUE, pos);

source   = "depict";		subsource = "";
(void) set_fld_descript(&fdesc, FpaF_DIRECTORY_PATH, path,
						FpaF_SOURCE_NAME, source,
						FpaF_SUBSOURCE_NAME, subsource,
						FpaF_RUN_TIME, rtime,
						FpaF_END_OF_LIST);
(void) test_check_extract_wind(1, &fdesc, TRUE, pos);

windfunc = "FPA_Adjusted_Wind_Func";
element  = "wind";			level     = "sfc";
(void) set_fld_descript(&fdesc,
						FpaF_WIND_FUNCTION_NAME, windfunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
						FpaF_END_OF_LIST);
(void) test_check_extract_wind(1, &fdesc, TRUE, pos);

/* Testing for check_extract_wind_by_crossref */
(void) fprintf(stdout, "\n\n *** Testing for check_extract_wind_by_crossref ***\n");
(void) clear_equation_database();

crossref = "Fpa_Wind";
pos[X] = 1000.0;			pos[Y] = 1000.0;
(void) test_check_extract_wind_by_crossref(crossref, &fdesc, TRUE, pos);

pos[X] = 10000.0;			pos[Y] = 10000.0;
(void) test_check_extract_wind_by_crossref(crossref, &fdesc, TRUE, pos);

return 0;
}

#endif /* WINDS_STANDALONE */
