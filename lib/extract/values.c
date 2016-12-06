/*********************************************************************/
/**	@file values.c
 *
 * Routines to extract values from fields of meteorological data.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*   v a l u e s . c                                                    *
*                                                                      *
*   Routines to extract values from fields of meteorological data      *
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

#define	VALUES_MAIN		/* To initialize defined constants and */
						/*  functions in values.h file         */

#include "equation.h"
#include "values.h"
#include "winds.h"

#include <environ/environ.h>
#include <objects/objects.h>
#include <tools/tools.h>
#include <fpa_types.h>
#include <fpa_getmem.h>

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#ifdef DEBUG_VALUES		/* Turn on/off internal debug printing */
	static	LOGICAL	DebugMode = TRUE;
#else
	static	LOGICAL	DebugMode = FALSE;
#endif /* DEBUG_VALUES */

#define dprintf (!DebugMode)? (void) 0: (void) fprintf


/* Interface functions                */
/*  ... these are defined in values.h */


/* Internal static functions (Types of Values) */
static	LOGICAL		daily_max_value(int, FLD_DESCRIPT *, LOGICAL, int, POINT *,
								float, float *, STRING *, float *, STRING *);
static	LOGICAL		daily_min_value(int, FLD_DESCRIPT *, LOGICAL, int, POINT *,
								float, float *, STRING *, float *, STRING *);

/* Internal static functions (Setting Max/Min Information) */
static	LOGICAL		compare_daily_max_value(FLD_DESCRIPT *, float,
								float, float, float);
static	LOGICAL		compare_daily_min_value(FLD_DESCRIPT *, float,
								float, float, float);
static	float		maxmin_time_difference(STRING, STRING, float);

/* Constants cross-referenced in configuration file */
static	const	STRING	Rad     = "RAD";

/* Units cross-referenced in configuration file */
static	const	STRING	VMks        = "MKS";
static	const	STRING	VHr         = "hr";
static	const	STRING	MperS       = "m/s";
static	const	STRING	DegreesTrue = "degrees_true";


/**********************************************************************
 ***                                                                ***
 *** i d e n t i f y _ v a l u e _ f u n c t i o n                  ***
 *** d i s p l a y _ v a l u e _ f u n c t i o n s                  ***
 ***																***
 **********************************************************************/

/* Define FPA value functions for search list */
static	VALUEFUNC_FUNC	ordinary_value;
static	VALUEFUNC_FUNC	vector_magnitude;
static	VALUEFUNC_FUNC	vector_direction;
static	VALUEFUNC_FUNC	daily_max_value_value;
static	VALUEFUNC_FUNC	daily_max_value_time;
static	VALUEFUNC_FUNC	daily_min_value_value;
static	VALUEFUNC_FUNC	daily_min_value_time;
static	VALUEFUNC_FUNC	get_daily_value;
static	VALUEFUNC_FUNC	fpa_wind_speed;
static	VALUEFUNC_FUNC	fpa_wind_direction;

/* Initialize FPA value function search list */
static	VALUEFUNC_TABLE	ValueFuncs[] =
	{
		{ FpaDefaultValueFunc,                  ordinary_value,            1 },
		{ "FPA_Vector_Magnitude_Func",          vector_magnitude,          1 },
		{ "FPA_Vector_Direction_Func",          vector_direction,          1 },
		{ "FPA_Daily_Max_Value_Func",           daily_max_value_value,     1 },
		{ "FPA_Daily_Max_Time_Func",            daily_max_value_time,      1 },
		{ "FPA_Daily_Min_Value_Func",           daily_min_value_value,     1 },
		{ "FPA_Daily_Min_Time_Func",            daily_min_value_time,      1 },
		{ "FPA_Get_Daily_Value_Func",           get_daily_value,           1 },
		{ "FPA_Actual_Wind_Speed_Func",         fpa_wind_speed,            1 },
		{ "FPA_Actual_Wind_Direction_Func",     fpa_wind_direction,        1 },
	};

/* Set number of FPA value functions in search list */
static	int		NumValueFuncs =
	(int) (sizeof(ValueFuncs) / sizeof(VALUEFUNC_TABLE));

/*********************************************************************/
/** Identify a config file value function name
 *
 * @param[in]	name	config file value function name
 * @param[out]	*func	pointer to function
 * @param[out]	*nreq	number of fields required by function
 * @return True if Successful.
 *********************************************************************/
LOGICAL				identify_value_function

	(
	STRING			name,
	VALUEFUNC		*func,
	int				*nreq
	)

	{
	int				inum, nrq;
	VALUEFUNC		fnc;

	/* Initialize return values */
	if ( NotNull(func) ) *func = NullValueFunc;
	if ( NotNull(nreq) ) *nreq = 0;

	/* Return FALSE if no value function name passed */
	if ( blank(name) ) return FALSE;

	/* Search user defined value functions first */
	if ( identify_user_value_function(name, &fnc, &nrq) )
		{
		if ( NotNull(func) ) *func = fnc;
		if ( NotNull(nreq) ) *nreq = nrq;
		return TRUE;
		}

	/* Search internal value functions next */
	for ( inum=0; inum<NumValueFuncs; inum++ )
		{

		if ( same(name, ValueFuncs[inum].name) )
			{
			if ( NotNull(func) ) *func = ValueFuncs[inum].func;
			if ( NotNull(nreq) ) *nreq = ValueFuncs[inum].nreq;
			return TRUE;
			}
		}

	/* Return FALSE if value function name not found */
	return FALSE;
	}

/*********************************************************************/
/** Display value function names for config files
 *********************************************************************/
void				display_value_functions

	(
	)

	{
	int				inum;

	/* Display all internal value functions */
	(void) printf(" Default Value Functions");
	(void) printf(" ... from Config \"value_function\" lines\n");
	for ( inum=0; inum<NumValueFuncs; inum++ )
		{
		(void) printf("  %2d   Value Function Name:  %s\n",
				inum+1, ValueFuncs[inum].name);
		}
	}

/**********************************************************************
 ***                                                                ***
 *** g e t _ v a l u e s _ c o n s t a n t                          ***
 ***                                                                ***
 **********************************************************************/

/* Define FpaVALUE_CONSTANTS Object - containing constants for values */
typedef struct FpaVALUE_CONSTANTS_struct
{
	STRING	cname;		/* name of saved constant */
	STRING	cunits;		/* units for saved constant */
	double	cvalue;		/* value for saved constant */
} FpaVALUE_CONSTANTS;

/* Storage locations for constants for values */
static	int					NumValueConstants = 0;
static	int					LastValueConstant = -1;
static	FpaVALUE_CONSTANTS	*ValueConstants   = NullPtr(FpaVALUE_CONSTANTS *);


/*********************************************************************/
/** Returns value of a given constant in given units for value
 * calculations
 *
 * @param[in]	name	Name of constant
 * @param[out]	units	Units for constant
 * @param[out]	*value	Value of constant
 * @return True if Successful.
 *********************************************************************/
LOGICAL				get_values_constant

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

	/* Check saved constants for values, beginning with last one used */
	if ( LastValueConstant >= 0 )
		{
		for ( inum=LastValueConstant; inum<NumValueConstants; inum++ )
			{

			/* Return value of constant, if found */
			if ( same(name, ValueConstants[inum].cname)
					&& same(units, ValueConstants[inum].cunits) )
				{
				LastValueConstant = inum;
				xval = ValueConstants[LastValueConstant].cvalue;
				if ( NotNull(value) ) *value = xval;
				return TRUE;
				}
			}
		for ( inum=0; inum<LastValueConstant; inum++ )
			{

			/* Return value of constant, if found */
			if ( same(name, ValueConstants[inum].cname)
					&& same(units, ValueConstants[inum].cunits) )
				{
				LastValueConstant = inum;
				xval = ValueConstants[LastValueConstant].cvalue;
				if ( NotNull(value) ) *value = xval;
				return TRUE;
				}
			}
		}

	/* Check saved constants for values, beginning with first one */
	else
		{
		for ( inum=0; inum<NumValueConstants; inum++ )
			{

			/* Return value of constant, if found */
			if ( same(name, ValueConstants[inum].cname)
					&& same(units, ValueConstants[inum].cunits) )
				{
				LastValueConstant = inum;
				xval = ValueConstants[LastValueConstant].cvalue;
				if ( NotNull(value) ) *value = xval;
				return TRUE;
				}
			}
		}

	/* Constant not found in saved list, so find it */
	cdef = identify_constant(name);
	if ( IsNull(cdef) )
		{
		(void) fprintf(stderr, "[get_values_constant] No constant");
		(void) fprintf(stderr, "  \"%s\"  in Config file\n", SafeStr(name));
		return FALSE;
		}

	/* Convert constant value to requested units, if possible */
	if ( !convert_value(cdef->units->name, cdef->value, units, &xval) )
		{
		(void) fprintf(stderr, "[get_values_constant] Constant  \"%s\"",
				SafeStr(name));
		(void) fprintf(stderr, "  cannot be converted to  \"%s\"\n",
				SafeStr(units));
		return FALSE;
		}
	if ( NotNull(value) ) *value = xval;

	/* Add this constant to the saved list */
	LastValueConstant = NumValueConstants++;
	ValueConstants = GETMEM(ValueConstants, FpaVALUE_CONSTANTS,
														NumValueConstants);
	ValueConstants[LastValueConstant].cname  = strdup(name);
	ValueConstants[LastValueConstant].cunits = strdup(units);
	ValueConstants[LastValueConstant].cvalue = xval;
	return TRUE;
	}

/**********************************************************************
 ***                                                                ***
 *** e x t r a c t _ s u r f a c e _ v a l u e                      ***
 *** e x t r a c t _ s u r f a c e _ v a l u e _ b y                ***
 ***                                              _ c r o s s r e f ***
 *** e x t r a c t _ s u r f a c e _ v a l u e _ b y                ***
 ***                                              _ e q u a t i o n ***
 ***                                                                ***
 **********************************************************************/

/*********************************************************************/
/** Extract value from surface fields determined from given input
 *  parameters
 *
 * @param[in]	nfds	number of field descriptors
 * @param[in]	*fdescs	list of nfds field descriptors
 * @param[in]	matched	match valid times to available fields?
 * @param[in]	npos	number of positions on fields
 * @param[in]	*ppos	list of positions on fields
 * @param[in]	clon	center longitude for fields
 * @param[out]	*values	list of values at npos positions
 * @param[out]	*units	list of units of values
 * @return True if Successful.
 *********************************************************************/
LOGICAL				extract_surface_value

	(
	int				nfds,
	FLD_DESCRIPT	*fdescs,
	LOGICAL			matched,
	int				npos,
	POINT			*ppos,
	float			clon,
	float			*values,
	STRING			*units
	)

	{
	int				nn, nreq;
	LOGICAL			valid;
	VALUEFUNC		func;

	/* Check for invalid number of field descriptors */
	if ( nfds < 1 ) return FALSE;

	/* Return FALSE if no information in field descriptors */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	for ( nn=0; nn<nfds; nn++ )
		{
		if ( IsNull(fdescs[nn].sdef) || IsNull(fdescs[nn].subdef) )
			return FALSE;
		}

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in extract_surface_value:\n");
		for ( nn=0; nn<nfds; nn++ )
			{
			dprintf(stdout,"   path: %s\n",      fdescs[nn].path);
			dprintf(stdout,"   source: %s %s\n", fdescs[nn].sdef->name,
													fdescs[nn].subdef->name);
			dprintf(stdout,"   rtime: %s\n",     fdescs[nn].rtime);
			dprintf(stdout,"   vtime: %s\n",     fdescs[nn].vtime);
			dprintf(stdout,"   function name: %s\n",
													fdescs[nn].value_func_name);
			if ( NotNull(fdescs[nn].edef) )
				dprintf(stdout,"   element: %s\n",   fdescs[nn].edef->name);
			if ( NotNull(fdescs[nn].ldef) )
				dprintf(stdout,"   level: %s\n",     fdescs[nn].ldef->name);
			}
		}

	/* Set default units */
	if ( NotNull(units) ) *units = NullString;

	/* Identify the value calculation function to use */
	if ( !identify_value_function(fdescs[0].value_func_name, &func, &nreq) )
		{
		(void) fprintf(stderr, "[extract_surface_value] Unrecognized value");
		(void) fprintf(stderr, " function: \"%s\"\n",
				SafeStr(fdescs[0].value_func_name));
		return FALSE;
		}

	/* Return FALSE if error in returned parameters */
	if ( IsNull(func) )
		{
		(void) fprintf(stderr, "[extract_surface_value] Missing function");
		(void) fprintf(stderr, " for value function: \"%s\"\n",
				SafeStr(fdescs[0].value_func_name));
		return FALSE;
		}
	if ( nfds < nreq )
		{
		(void) fprintf(stderr, "[extract_surface_value] Not enough fields");
		(void) fprintf(stderr, " for value function: \"%s\"\n",
				SafeStr(fdescs[0].value_func_name));
		return FALSE;
		}

	/* Branch to the appropriate value calculation function */
	valid = func(nfds, fdescs, matched, npos, ppos, clon, values, units);
	return valid;
	}

/*********************************************************************/
/** Extract value from surface fields determined from given input
 *  parameters which include a value cross reference
 *
 *	@param[in]	valuecref	value cross reference
 *	@param[in]	*fdesc		field descriptor
 *	@param[in]	matched		match valid times to available fields?
 *	@param[in]	npos		number of positions on field
 *	@param[in]	*ppos		list of positions on field
 *	@param[in]	clon		center longitude for field
 *	@param[out]	*values		list of values at npos positions
 *	@param[out]	*units		list of units of values
 *  @return True if Successful.
 *********************************************************************/
LOGICAL				extract_surface_value_by_crossref

	(
	STRING			valuecref,
	FLD_DESCRIPT	*fdesc,
	LOGICAL			matched,
	int				npos,
	POINT			*ppos,
	float			clon,
	float			*values,
	STRING			*units
	)

	{
	int							nn, nfds;
	LOGICAL						valid;
	FLD_DESCRIPT				*descripts;
	FpaConfigCrossRefStruct		*crdef;

	/* Return FALSE if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->sdef) || IsNull(fdesc->subdef) )
		return FALSE;
	if ( IsNull(ppos) ) return FALSE;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in extract_surface_value_by_crossref:\n");
		dprintf(stdout,"   value cross reference: %s\n", valuecref);
		dprintf(stdout,"   path: %s\n",      fdesc->path);
		dprintf(stdout,"   source: %s %s\n", fdesc->sdef->name,
												fdesc->subdef->name);
		dprintf(stdout,"   rtime: %s\n",     fdesc->rtime);
		dprintf(stdout,"   vtime: %s\n",     fdesc->vtime);
		}

	/* Set default units */
	if ( NotNull(units) ) *units = NullString;

	/* Get value cross reference information */
	crdef = identify_crossref(FpaCcRefsValues, valuecref);
	if ( IsNull(crdef) || crdef->nfld < 1 )
		{
		(void) fprintf(stderr, "[extract_surface_value_by_crossref] Unknown");
		(void) fprintf(stderr, " value cross reference type requested: \"%s\"\n",
				SafeStr(valuecref));
		return FALSE;
		}

	/* Build field descriptors for each cross reference field */
	/*  ... and one for original field descriptor             */
	nfds      = crdef->nfld + 1;
	descripts = INITMEM(FLD_DESCRIPT, nfds);
	for ( nn=0; nn<crdef->nfld; nn++ )
		{
		(void) copy_fld_descript(&descripts[nn], fdesc);
		if ( !set_fld_descript(&descripts[nn],
								FpaF_VALUE_FUNCTION_NAME, crdef->func_name,
								FpaF_ELEMENT, crdef->flds[nn]->element,
								FpaF_LEVEL, crdef->flds[nn]->level,
								FpaF_END_OF_LIST) )
			{
			FREEMEM(descripts);
			return FALSE;
			}
		}

	/* Add the original field descriptor to the list */
	/* Note that this is required by some functions! */
	(void) copy_fld_descript(&descripts[nfds-1], fdesc);

	/* Extract value using cross reference field descriptors */
	valid = extract_surface_value(nfds, descripts, matched,
			npos, ppos, clon, values, units);
	FREEMEM(descripts);
	return valid;
	}

/*********************************************************************/
/** Extract value from surface fields determined from given input
 *  parameters which include an equation
 *
 *	@param[in]	inunit	input units for equation
 *	@param[in]	inebuf	input equation for field
 *	@param[in]	*fdesc	field descriptor
 *	@param[in]	matched match valid times to available fields?
 *	@param[in]	npos	number of positions on field
 *	@param[in]	*ppos	list of positions on field
 *	@param[in]	clon	center longitude for field
 *	@param[out]	*values list of values at positions
 *	@param[out]	*units	list of units of values
 *  @return True if Successful.
 *********************************************************************/
LOGICAL				extract_surface_value_by_equation

	(
	STRING			inunit,
	STRING			inebuf,
	FLD_DESCRIPT	*fdesc,
	LOGICAL			matched,
	int				npos,
	POINT			*ppos,
	float			clon,
	float			*values,
	STRING			*units
	)

	{
	int				ipos;
	VLIST			*vlist = NullPtr(VLIST *);

	/* Internal buffer for input units */
	static	char	eqtnunit[MAX_BCHRS];

	/* Return FALSE if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->sdef) || IsNull(fdesc->subdef) )
		return FALSE;
	if ( IsNull(ppos) ) return FALSE;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in extract_surface_value_by_equation:\n");
		dprintf(stdout,"   units: %s\n",     inunit);
		dprintf(stdout,"   equation: %s\n",  inebuf);
		dprintf(stdout,"   path: %s\n",      fdesc->path);
		dprintf(stdout,"   source: %s %s\n", fdesc->sdef->name,
												fdesc->subdef->name);
		dprintf(stdout,"   rtime: %s\n",     fdesc->rtime);
		dprintf(stdout,"   vtime: %s\n",     fdesc->vtime);
		}

	/* Set default units */
	if ( NotNull(units) ) *units = NullString;

	/* Extract data values at all positions */
	vlist = retrieve_vlist_by_equation(fdesc, npos, ppos, inunit, inebuf);

	/* Return FALSE if data cannot be found */
	if ( IsNull(vlist) )
		{
		(void) fprintf(stderr, "[extract_surface_value_by_equation] Cannot retrieve ");
		(void) fprintf(stderr, "  values for units/equation: \"%s %s\"\n",
				SafeStr(inunit), SafeStr(inebuf));
		return FALSE;
		}

	/* Loop through all positions and set data values */
	for ( ipos=0; ipos<npos; ipos++ )
		if ( NotNull(values) ) values[ipos] = vlist->val[ipos];

	/* Return MKS units since MKS is returned by retrieve_vlist_by_equation() */
	(void) safe_strcpy(eqtnunit, FpaCmksUnits);
	if ( NotNull(units) )  *units = eqtnunit;

	/* Free space used by VLIST Object and return TRUE if all went well */
	(void) free_vlist(vlist);
	FREEMEM(vlist);
	return TRUE;
	}

/**********************************************************************
 ***                                                                ***
 *** e x t r a c t _ a r e a s e t _ a t t r i b s                  ***
 *** e x t r a c t _ a r e a s e t _ a t t r i b s _ b y            ***
 ***                                              _ c r o s s r e f ***
 ***                                                                ***
 *** The following are replaced by the above and are now obsolete:  ***
 ***                                                                ***
 *** e x t r a c t _ a r e a s e t _ v a l u e                      ***
 *** e x t r a c t _ a r e a s e t _ v a l u e _ b y                ***
 ***                                              _ c r o s s r e f ***
 ***                                                                ***
 **********************************************************************/

/*********************************************************************/
/** Extract attrib lists from areaset fields determined from given
 *  input parameters
 *
 *	@param[in]	nfds	number of field descriptors
 *	@param[in]	*fdescs list of field descriptors
 *	@param[in]	matched match valid times to available fields?
 *	@param[in]	npos	number of positions on fields
 *	@param[in]	*ppos	list of positions on fields
 *	@param[in]	clon	center longitude for fields
 *	@param[out]	*cals	list of attribute lists
 *  @return True if Successful.
 *********************************************************************/
LOGICAL				extract_areaset_attribs

	(
	int				nfds,
	FLD_DESCRIPT	*fdescs,
	LOGICAL			matched,
	int				npos,
	POINT			*ppos,
	float			clon,
	CAL				*cals
	)

	{
	int						ifd, ipos, isave;
	FLD_DESCRIPT			descript;
	FpaConfigFieldStruct	*fdef;
	SET						areas;
	LOGICAL					valid;
	SUBAREA					sub;
	CAL						cal;

	/* List of CAL structs duplicated from unique subareas */
	static	CAL		*SaveCals = NullCalPtr;
	static	SUBAREA	*SaveSubs = NullSubAreaPtr;
	static	int		NumSave   = 0;

	/* Check for correct number of field descriptors */
	if ( nfds != 1 )
		{
		(void) fprintf(stderr, "[extract_areaset_attribs] Error in");
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

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in extract_areaset_attribs:\n");
		for ( ifd=0; ifd<nfds; ifd++ )
			{
			dprintf(stdout,"   path: %s\n",        fdescs[ifd].path);
			dprintf(stdout,"   source: %s %s\n",   fdescs[ifd].sdef->name,
													fdescs[ifd].subdef->name);
			dprintf(stdout,"   rtime: %s\n",       fdescs[ifd].rtime);
			dprintf(stdout,"   vtime: %s\n",       fdescs[ifd].vtime);
			dprintf(stdout,"   function name: %s\n",
													fdescs[ifd].value_func_name);
			dprintf(stdout,"   element: %s\n",     fdescs[ifd].edef->name);
			dprintf(stdout,"   level: %s\n",       fdescs[ifd].ldef->name);
			}
		}

	/* Free space used by internal buffers */
	for (isave=0; isave<NumSave; isave++)
		{
		CAL_destroy(SaveCals[isave]);
		}
	FREEMEM(SaveCals);
	FREEMEM(SaveSubs);
	NumSave = 0;

	/* Initialize return parameters */
	for ( ipos=0; ipos<npos; ipos++ )
		{
		if ( NotNull(cals) ) cals[ipos] = NullCal;
		}

	/* The first field descriptor contains the required data! */
	(void) copy_fld_descript(&descript, &fdescs[0]);

	/* Reset valid time for field (if requested) */
	if ( matched )
		{
		(void) matched_source_valid_time_reset(&descript, FpaC_TIMEDEP_ANY,
				descript.vtime);
		}

	/* Check the field info */
	fdef = get_field_info(descript.edef->name, descript.ldef->name);
	if ( IsNull(fdef) )
		{
		(void) fprintf(stderr,
				"[extract_areaset_attribs] Problem with field: \"%s %s\"\n",
				SafeStr(descript.edef->name), SafeStr(descript.ldef->name));
		return FALSE;
		}

	/* Retrieve areaset for field descriptor             */
	/*  ... and set background values if no field found! */
	areas = retrieve_areaset(&descript);
	if ( IsNull(areas) )
		{
		(void) fprintf(stderr,
				"[extract_areaset_attribs] No field: \"%s %s\"\n",
				SafeStr(descript.edef->name), SafeStr(descript.ldef->name));
		return FALSE;
		}

	/* Loop through all positions and extract areaset values */
	for ( ipos=0; ipos<npos; ipos++ )
		{

		/* Extract parameters from first enclosing subarea in areaset field */
		valid = eval_areaset(areas, ppos[ipos], PickFirst, &sub, &cal);
		if ( !valid ) continue;

		/* Find if this subarea (Null is background) is already in */
		/* the internal list - if not, add it */
		for (isave=0; isave<NumSave; isave++)
			{
			if (sub == SaveSubs[isave]) break;
			}
		if (isave >= NumSave)
			{
			NumSave++;
			SaveCals = GETMEM(SaveCals, CAL, NumSave);
			SaveSubs = GETMEM(SaveSubs, SUBAREA, NumSave);
			SaveCals[isave] = CAL_duplicate(cal);
			SaveSubs[isave] = sub;
			}

		/* Set pointer in return list */
		cals[ipos] = SaveCals[isave];
		}

	/* Free space used by SET Object */
	areas = destroy_set(areas);

	return TRUE;
	}

/*********************************************************************/
/** Extract attrib lists from areaset fields determined from given
 *  input parameters which include a value cross reference
 *
 *	@param[in]	valuecref	value cross reference
 *	@param[in]	*fdesc		field descriptor
 *	@param[in]	matched		match valid times to available fields?
 *	@param[in]	npos		number of positions on field
 *	@param[in]	*ppos		list of positions on field
 *	@param[in]	clon		center longitude for field
 *	@param[out]	*cals		list of attribute lists
 *  @return True if Successful.
 *********************************************************************/
LOGICAL				extract_areaset_attribs_by_crossref

	(
	STRING			valuecref,
	FLD_DESCRIPT	*fdesc,
	LOGICAL			matched,
	int				npos,
	POINT			*ppos,
	float			clon,
	CAL				*cals
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
		dprintf(stdout,"Here we are in extract_areaset_attribs_by_crossref:\n");
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
				"[extract_areaset_attribs_by_crossref] Unknown");
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
	valid = extract_areaset_attribs(crdef->nfld, descripts, matched,
			npos, ppos, clon, cals);
	FREEMEM(descripts);
	return valid;
	}

/*********************************************************************/
/** OBSOLETE!
 *
 * Extract value from areaset fields determined from given input
 * parameters
 *
 * @return True if Successful.
 *********************************************************************/
LOGICAL				extract_areaset_value

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on fields */
	POINT			*ppos,		/* pointer to npos positions on fields */
	float			clon,		/* center longitude for fields */
	STRING			*subelems,	/* pointer to npos areaset subelement types */
	STRING			*values,	/* pointer to npos areaset values */
	STRING			*labels		/* pointer to npos areaset labels */
	)

	{
	int						nn, ipos;
	LOGICAL					valid;
	FLD_DESCRIPT			descript;
	SET						areas;
	FpaConfigFieldStruct	*fdef;
	ATTRIB					*attrib;
	ATTRIB_LIST				attribs;

	/* Internal buffers for areaset data */
	static	STRING	*Asubs = NullStringList;
	static	STRING	*Avals = NullStringList, *Alabs = NullStringList;
	static	int		Npstns = 0;

	/* Check for correct number of field descriptors */
	if ( nfds != 1 )
		{
		(void) fprintf(stderr, "[extract_areaset_value] Error in");
		(void) fprintf(stderr, " number of field descriptors: %d\n", nfds);
		(void) fprintf(stderr, "   Only one field descriptor allowed (for data)\n");
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
		dprintf(stdout,"Here we are in extract_areaset_value:\n");
		for ( nn=0; nn<nfds; nn++ )
			{
			dprintf(stdout,"   path: %s\n",      fdescs[nn].path);
			dprintf(stdout,"   source: %s %s\n", fdescs[nn].sdef->name,
													fdescs[nn].subdef->name);
			dprintf(stdout,"   rtime: %s\n",     fdescs[nn].rtime);
			dprintf(stdout,"   vtime: %s\n",     fdescs[nn].vtime);
			dprintf(stdout,"   function name: %s\n",
													fdescs[nn].value_func_name);
			dprintf(stdout,"   element: %s\n",   fdescs[nn].edef->name);
			dprintf(stdout,"   level: %s\n",     fdescs[nn].ldef->name);
			}
		}

	/* Free space used by internal buffers */
	FREELIST(Asubs, Npstns);
	FREELIST(Avals, Npstns);
	FREELIST(Alabs, Npstns);

	/* Initialize return parameters */
	for ( ipos=0; ipos<npos; ipos++ )
		{
		if ( NotNull(subelems) ) subelems[ipos] = NullString;
		if ( NotNull(values) )   values[ipos]   = NullString;
		if ( NotNull(labels) )   labels[ipos]   = NullString;
		}

	/* The first field descriptor contains the required data! */
	(void) copy_fld_descript(&descript, &fdescs[0]);

	/* Reset valid time for field (if requested) */
	if ( matched )
		{
		(void) matched_source_valid_time_reset(&descript, FpaC_TIMEDEP_ANY,
				descript.vtime);
		}

	/* Check the field info */
	fdef = get_field_info(descript.edef->name, descript.ldef->name);
	if ( IsNull(fdef) )
		{
		(void) fprintf(stderr, "[extract_areaset_value] Problem");
		(void) fprintf(stderr, " with field: \"%s %s\"\n",
				SafeStr(descript.edef->name), SafeStr(descript.ldef->name));
		return FALSE;
		}

	/* Retrieve areaset for field descriptor             */
	/*  ... and set background values if no field found! */
	areas = retrieve_areaset(&descript);
	if ( IsNull(areas) )
		{
		(void) fprintf(stderr, "[extract_areaset_value] No field: \"%s %s\"\n",
				SafeStr(descript.edef->name), SafeStr(descript.ldef->name));
		return FALSE;
		}

	/* Allocate space for internal buffers */
	Npstns = npos;
	Asubs  = INITMEM(STRING, Npstns);
	Avals  = INITMEM(STRING, Npstns);
	Alabs  = INITMEM(STRING, Npstns);
	for ( ipos=0; ipos<Npstns; ipos++ )
		{
		Asubs[ipos] = NullString;
		Avals[ipos] = NullString;
		Alabs[ipos] = NullString;
		}

	/* Loop through all positions and extract areaset values */
	for ( ipos=0; ipos<npos; ipos++ )
		{

		/* Extract parameters from first enclosing subarea in areaset field */
		valid = eval_areaset(areas, ppos[ipos], PickFirst, NullSubAreaPtr,
					&attribs);
		if ( valid )
			{
			attrib = find_attribute(attribs, AttribCategory);
			if ( NotNull(attrib) ) Asubs[ipos] = strdup(attrib->value);
			attrib = find_attribute(attribs, AttribAutolabel);
			if ( NotNull(attrib) ) Avals[ipos] = strdup(attrib->value);
			attrib = find_attribute(attribs, AttribUserlabel);
			if ( NotNull(attrib) ) Alabs[ipos] = strdup(attrib->value);
			}
		}

	/* Free space used by SET Object, set return parameters, */
	/*  and return TRUE if all went well                     */
	areas = destroy_set(areas);
	for ( ipos=0; ipos<npos; ipos++ )
		{
		if ( NotNull(subelems) ) subelems[ipos] = Asubs[ipos];
		if ( NotNull(values) )   values[ipos]   = Avals[ipos];
		if ( NotNull(labels) )   labels[ipos]   = Alabs[ipos];
		}
	return TRUE;
	}

/*********************************************************************/
/** OBSOLETE!
 *
 * Extract value from areaset fields determined from given input
 * parameters which include a value cross reference
 *
 * @return True if Successful.
 *********************************************************************/
LOGICAL				extract_areaset_value_by_crossref

	(
	STRING			valuecref,	/* value cross reference */
	FLD_DESCRIPT	*fdesc,		/* pointer to field descriptor */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on field */
	POINT			*ppos,		/* pointer to npos positions on field */
	float			clon,		/* center longitude for field */
	STRING			*subelems,	/* pointer to npos areaset subelement types */
	STRING			*values,	/* pointer to npos areaset values */
	STRING			*labels		/* pointer to npos areaset labels */
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
		dprintf(stdout,"Here we are in extract_areaset_value_by_crossref:\n");
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
		(void) fprintf(stderr, "[extract_areaset_value_by_crossref] Unknown");
		(void) fprintf(stderr, " value cross reference type requested: \"%s\"\n",
				SafeStr(valuecref));
		return FALSE;
		}

	/* Build field descriptors for each cross reference field */
	descripts = INITMEM(FLD_DESCRIPT, crdef->nfld);
	for ( nn=0; nn<crdef->nfld; nn++ )
		{
		(void) copy_fld_descript(&descripts[nn], fdesc);
		if ( !set_fld_descript(&descripts[nn],
								FpaF_VALUE_FUNCTION_NAME, crdef->func_name,
								FpaF_ELEMENT, crdef->flds[nn]->element,
								FpaF_LEVEL, crdef->flds[nn]->level,
								FpaF_END_OF_LIST) )
			{
			FREEMEM(descripts);
			return FALSE;
			}
		}

	/* Extract value using cross reference field descriptors */
	valid = extract_areaset_value(crdef->nfld, descripts, matched,
			npos, ppos, clon, subelems, values, labels);
	FREEMEM(descripts);
	return valid;
	}

/**********************************************************************
 ***                                                                ***
 *** c h e c k _ v a l u e _ f u n c t i o n                        ***
 ***                                 _ e r r o r _ m e s s a g e s  ***
 *** c h e c k _ e x t r a c t _ v a l u e                          ***
 *** c h e c k _ e x t r a c t _ v a l u e _ b y _ c r o s s r e f  ***
 ***                                                                ***
 **********************************************************************/

/* Flag for printing error messages in check_... value functions */
static	LOGICAL	CheckValueErrorMessages = TRUE;

/*********************************************************************/
/** Set flag for printing error messages in check_... functions
 *
 *	@param[in]	errorflag	flag for printing error messages
 *********************************************************************/
void				check_value_function_error_messages

	(
	LOGICAL			errorflag
	)

	{

	/* Reset flag for printing error messages */
	if ( !errorflag ) CheckValueErrorMessages = FALSE;
	else              CheckValueErrorMessages = TRUE;
	}

/*********************************************************************/
/** Check whether a given type of value can be extracted from data
 * fields determined from given input paramters
 *
 *	@param[in]	nfds		number of field descriptors
 *	@param[in]	*fdescs		list of field descriptors
 *	@param[in]	matched		match valid times to available fields?
 *	@param[in]	npos		number of positions on fields
 *	@param[in]	*ppos		list of positions on fields
 * 	@return True if value can be extracted.
 *********************************************************************/
LOGICAL				check_extract_value

	(
	int				nfds,
	FLD_DESCRIPT	*fdescs,
	LOGICAL			matched,
	int				npos,
	POINT			*ppos
	)

	{
	int						nn, nreq;
	FLD_DESCRIPT			descript;
	FpaConfigElementStruct	*edef;


	/* Check for invalid number of field descriptors */
	if ( nfds < 1 ) return FALSE;

	/* Return FALSE if no information in field descriptors */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	for ( nn=0; nn<nfds; nn++ )
		{
		if ( IsNull(fdescs[nn].sdef) || IsNull(fdescs[nn].subdef) )
			return FALSE;
		}

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in check_extract_value:\n");
		for ( nn=0; nn<nfds; nn++ )
			{
			dprintf(stdout,"   path: %s\n",      fdescs[nn].path);
			dprintf(stdout,"   source: %s %s\n", fdescs[nn].sdef->name,
													fdescs[nn].subdef->name);
			dprintf(stdout,"   rtime: %s\n",     fdescs[nn].rtime);
			dprintf(stdout,"   vtime: %s\n",     fdescs[nn].vtime);
			dprintf(stdout,"   function name: %s\n",
													fdescs[nn].value_func_name);
			if ( NotNull(fdescs[nn].edef) )
				dprintf(stdout,"   element: %s\n",   fdescs[nn].edef->name);
			if ( NotNull(fdescs[nn].ldef) )
				dprintf(stdout,"   level: %s\n",     fdescs[nn].ldef->name);
			}
		}

	/* Identify the value calculation function */
	if ( !identify_value_function(fdescs[0].value_func_name,
			NullValueFuncPtr, &nreq) )
		{
		(void) fprintf(stderr, "[check_extract_value] Unrecognized value");
		(void) fprintf(stderr, " function: \"%s\"\n",
				SafeStr(fdescs[0].value_func_name));
		return FALSE;
		}

	/* Return FALSE if error in returned parameters */
	if ( nfds < nreq )
		{
		(void) fprintf(stderr, "[check_extract_value] Not enough fields");
		(void) fprintf(stderr, " for value function: \"%s\"\n",
				SafeStr(fdescs[0].value_func_name));
		return FALSE;
		}

	/* Loop to check each required field */
	for ( nn=0; nn<nreq; nn++ )
		{

		/* Check for missing element or level information */
		if ( IsNull(fdescs[nn].edef) || IsNull(fdescs[nn].ldef) )
			{
			(void) fprintf(stderr, "[check_extract_value] Error in required");
			(void) fprintf(stderr, " fields for value function: \"%s\"\n",
					SafeStr(fdescs[0].value_func_name));
			return FALSE;
			}

		/* Reset valid time for each field (if required) */
		(void) copy_fld_descript(&descript, &fdescs[nn]);
		if ( matched )
			{
			(void) matched_source_valid_time_reset(&descript, FpaC_TIMEDEP_ANY,
					descript.vtime);
			}

		/* Branch to special checks based on the type of field */
		edef = get_element_info(descript.edef->name);
		if ( IsNull(edef) ) return FALSE;
		switch ( edef->fld_type )
			{

			/* Check WIND type fields */
			case FpaC_WIND:
				(void) set_fld_descript(&descript,
						FpaF_WIND_FUNCTION_NAME, FpaDefaultWindFunc,
						FpaF_END_OF_LIST);
				if ( !check_extract_wind(1, &descript, matched, npos, ppos) )
					{
					if ( CheckValueErrorMessages )
						{
						(void) fprintf(stderr, "[check_extract_value] Missing");
						(void) fprintf(stderr, " required wind field: \"%s %s\"\n",
								SafeStr(descript.edef->name),
								SafeStr(descript.ldef->name));
						(void) fprintf(stderr, "                       for");
						(void) fprintf(stderr, " value function: \"%s\"\n",
								SafeStr(fdescs[0].value_func_name));
						}
					return FALSE;
					}
				break;

			/* Default for all other field types */
			default:
				if ( !find_retrieve_metasfc(&descript) )
					{
					if ( CheckValueErrorMessages )
						{
						(void) fprintf(stderr, "[check_extract_value] Missing");
						(void) fprintf(stderr, " required field: \"%s %s\"\n",
								SafeStr(descript.edef->name),
								SafeStr(descript.ldef->name));
						(void) fprintf(stderr, "                       for");
						(void) fprintf(stderr, " value function: \"%s\"\n",
								SafeStr(fdescs[0].value_func_name));
						}
					return FALSE;
					}
				break;
			}
		}

	/* Return TRUE if all required fields are OK */
	return TRUE;
	}

/*********************************************************************/
/** Check whether a given type of value can be extracted from data
 *  fields determined from given input paramters which include
 *  a value cross reference
 *
 *	@param[in]	valuecref	value cross reference
 *	@param[in]	*fdesc		field descriptor
 *	@param[in]	matched		match valid times to available fields?
 *	@param[in]	npos		number of positions on field
 *	@param[in]	*ppos		list of positions on field
 *  @return True if that type of value can be extracted.
 *********************************************************************/
LOGICAL				check_extract_value_by_crossref

	(
	STRING			valuecref,
	FLD_DESCRIPT	*fdesc,
	LOGICAL			matched,
	int				npos,
	POINT			*ppos
	)

	{
	int							nn, nfds;
	float						clon;
	LOGICAL						valid;
	FLD_DESCRIPT				*descripts;
	FpaConfigCrossRefStruct		*crdef;

	/* Return FALSE if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->sdef) || IsNull(fdesc->subdef) )
		return FALSE;
	if ( IsNull(ppos) ) return FALSE;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in check_extract_value_by_crossref:\n");
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
		(void) fprintf(stderr, "[check_extract_value_by_crossref] Unknown");
		(void) fprintf(stderr, " value cross reference type requested: \"%s\"\n",
				SafeStr(valuecref));
		return FALSE;
		}

	/* Build field descriptors for each cross reference field */
	/*  ... and one for original field descriptor             */
	nfds      = crdef->nfld + 1;
	descripts = INITMEM(FLD_DESCRIPT, nfds);
	for ( nn=0; nn<crdef->nfld; nn++ )
		{
		(void) copy_fld_descript(&descripts[nn], fdesc);
		if ( !set_fld_descript(&descripts[nn],
								FpaF_VALUE_FUNCTION_NAME, crdef->func_name,
								FpaF_ELEMENT, crdef->flds[nn]->element,
								FpaF_LEVEL, crdef->flds[nn]->level,
								FpaF_END_OF_LIST) )
			{
			FREEMEM(descripts);
			return FALSE;
			}
		}

	/* Add the original field descriptor to the list */
	/* Note that this is required by some functions! */
	(void) copy_fld_descript(&descripts[nfds-1], fdesc);

	/* The only way to check is to actually do the calculation! */
	/* Check value using cross reference field descriptors */
	/* >>>>>
	valid = check_extract_value(nfds, descripts, matched, npos, ppos);
	<<<<< */
	(void) grid_center(&fdesc->mproj, NullPointPtr, NullFloat, &clon);
	valid = extract_surface_value(nfds, descripts, matched, npos, ppos,
			clon, NullFloat, NullStringPtr);
	FREEMEM(descripts);
	return valid;
	}

/**********************************************************************
 ***                                                                ***
 *** e x t r a c t _ f i e l d _ b y _ v a l u e _ c r o s s r e f  ***
 *** e x t r a c t _ m e t a f i l e _ b y _ v a l u e              ***
 ***                                              _ c r o s s r e f ***
 ***                                                                ***
 **********************************************************************/

/*********************************************************************/
/** Extract FIELD Object from values extracted from data fields
 * determined from given input parameters which include a value cross
 * reference
 *
 *	@param[in]	*fdescout	field descriptor for output field
 *	@param[in]	valuecref	value cross reference
 *	@param[in]	*fdescin	field descriptor for input field
 *	@param[in]	matched		match valid times to available fields?
 * 	@return Pointer to a FIELD object.
 *********************************************************************/
FIELD				extract_field_by_value_crossref

	(
	FLD_DESCRIPT	*fdescout,
	STRING			valuecref,
	FLD_DESCRIPT	*fdescin,
	LOGICAL			matched
	)

	{
	int					Inumx, Inumy;
	POINT				**Apstns;
	float				Aglen;
	float				Clon;
	int					iix, iiy;
	STRING				units;
	FpaConfigUnitStruct	*udef;
	SURFACE				sfc;
	FIELD				fld;

	static USPEC		uspec = {NullString, 1.0, 0.0};

	/* Internal buffers for input positions and extracted values */
	/*  at each grid point                                       */
	static	int		Num     = 0;
	static	POINT	*Pstns  = NullPointList;
	static	float	*Vals   = NullPtr(float *);
	static	float	*PGvals = NullPtr(float *);
	static	float	**Gvals = NullPtr(float **);

	/* Return NullFld pointer if no information in field descriptors */
	if ( IsNull(fdescout) || IsNull(fdescin)
			|| IsNull(fdescout->edef) || IsNull(fdescout->ldef)
			|| IsNull(fdescin->sdef) || IsNull(fdescin->subdef)
			|| IsNull(fdescin->edef) || IsNull(fdescin->ldef) ) return NullFld;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in extract_field_by_value_crossref:\n");
		dprintf(stdout,"   fldelem: %s\n",       fdescout->edef->name);
		dprintf(stdout,"   fldlvl: %s\n",        fdescout->ldef->name);
		dprintf(stdout,"   value cross reference: %s\n",
													valuecref);
		dprintf(stdout,"   path: %s\n",          fdescin->path);
		dprintf(stdout,"   source: %s %s\n",     fdescin->sdef->name,
													fdescin->subdef->name);
		dprintf(stdout,"   rtime: %s\n",         fdescin->rtime);
		dprintf(stdout,"   vtime: %s\n",         fdescin->vtime);
		dprintf(stdout,"   function name: %s\n", fdescin->value_func_name);
		dprintf(stdout,"   element: %s\n",       fdescin->edef->name);
		dprintf(stdout,"   level: %s\n",         fdescin->ldef->name);
		}

	/* Check if the field descriptor contains an acceptable field */
	if ( IsNull(fdescin->fdef) ) return NullFld;

	/* Ensure that map projections match */
	if ( !same_map_projection(&fdescout->mproj, &fdescin->mproj) )
			return NullFld;

	/* Get grid positions for given map projection */
	if ( !grid_positions(&fdescout->mproj, &Inumx, &Inumy, &Aglen,
			&Apstns, NullPtr(float ***), NullPtr(float ***)) ) return NullFld;

	/* Get center longitude for given map projection */
	if ( !grid_center(&fdescout->mproj, NullPointPtr, NullFloat, &Clon) )
			return NullFld;

	/* Initialize storage for input positions for all grid points */
	Num   = Inumy * Inumx;
	Pstns = GETMEM(Pstns, POINT, Num);

	/* Set positions at each grid point */
	for ( iiy=0; iiy<Inumy; iiy++ )
		{
		for ( iix=0; iix<Inumx; iix++ )
			{
			Pstns[iiy*Inumx + iix][X] = Apstns[iiy][iix][X];
			Pstns[iiy*Inumx + iix][Y] = Apstns[iiy][iix][Y];
			}
		}

	/* Initialize storage for extracted values at all positions */
	Vals = GETMEM(Vals, float, Num);

	/* Extract values by cross reference */
	if ( !extract_surface_value_by_crossref(valuecref, fdescin, matched,
			Num, Pstns, Clon, Vals, &units) ) return NullFld;

	/* Initialize storage for extracted values at all grid points */
	PGvals = GETMEM(PGvals, float,   Num);
	Gvals  = GETMEM(Gvals,  float *, Inumy);

	/* Set pointers and values at each grid point */
	for ( iiy=0; iiy<Inumy; iiy++ )
		{
		Gvals[iiy] = PGvals + (iiy*Inumx);
		for ( iix=0; iix<Inumx; iix++ )
			{
			Gvals[iiy][iix] = Vals[iiy*Inumx + iix];
			}
		}

	/* Transfer grid data to SURFACE Object */
	sfc = create_surface();
	(void) grid_surface(sfc, Aglen, Inumx, Inumy, Gvals);

	/* Set units specs from input units for field */
	udef = identify_unit(units);
	if ( NotNull(udef) )
		{
		(void) define_uspec(&uspec, udef->name, udef->factor, udef->offset);
		(void) define_surface_units(sfc, &uspec);
		(void) change_surface_units(sfc, &MKS_UNITS);
		}

	/* Build FIELD Object to hold the SURFACE Object */
	fld = create_field("a", fdescout->edef->name, fdescout->ldef->name);
	(void) define_fld_data(fld, "surface", (POINTER) sfc);

	/* Free workspace and return pointer to FIELD Object */
	return fld;
	}

/*********************************************************************/
/** Extract METAFILE Object containing FIELD Object from values
 * extracted from data fields determined from given input parameters
 * which include a value cross reference
 *
 *	@param[in]	*fdescout	field descriptor for output field
 *	@param[in]	valuecref	value cross reference
 *	@param[in]	*fdescin	field descriptor for input field
 *	@param[in]	matched		match valid times to available fields?
 *  @return Pointer to a METAFILE object.
 *********************************************************************/
METAFILE			extract_metafile_by_value_crossref

	(
	FLD_DESCRIPT	*fdescout,
	STRING			valuecref,
	FLD_DESCRIPT	*fdescin,
	LOGICAL			matched
	)

	{
	FIELD		fld;
	METAFILE	meta;

	/* Return NullMeta pointer if no information in field descriptors */
	if ( IsNull(fdescout) || IsNull(fdescin)
			|| IsNull(fdescout->edef) || IsNull(fdescout->ldef)
			|| IsNull(fdescin->sdef) || IsNull(fdescin->subdef)
			|| IsNull(fdescin->edef) || IsNull(fdescin->ldef) ) return NullMeta;

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in extract_metafile_by_value_crossref:\n");
		dprintf(stdout,"   fldelem: %s\n",       fdescout->edef->name);
		dprintf(stdout,"   fldlvl: %s\n",        fdescout->ldef->name);
		dprintf(stdout,"   value cross reference: %s\n",
													valuecref);
		dprintf(stdout,"   path: %s\n",          fdescin->path);
		dprintf(stdout,"   source: %s %s\n",     fdescin->sdef->name,
													fdescin->subdef->name);
		dprintf(stdout,"   rtime: %s\n",         fdescin->rtime);
		dprintf(stdout,"   vtime: %s\n",         fdescin->vtime);
		dprintf(stdout,"   function name: %s\n", fdescin->value_func_name);
		dprintf(stdout,"   element: %s\n",       fdescin->edef->name);
		dprintf(stdout,"   level: %s\n",         fdescin->ldef->name);
		}

	/* Retrieve FIELD Object by value */
	fld = extract_field_by_value_crossref(fdescout, valuecref, fdescin,
			matched);
	if ( IsNull(fld) ) return NullMeta;

	/* Create METAFILE Object to hold FIELD Object */
	meta = create_metafile();
	(void) define_mf_tstamp(meta, fdescout->rtime, fdescout->vtime);
	(void) define_mf_projection(meta, &(fdescout->mproj));

	/* Add FIELD Object to METAFILE Object */
	(void) add_field_to_metafile(meta, fld);

	/* Return METAFILE Object if all went well */
	return meta;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Types of Values)                        *
*                                                                      *
*     All the routines after this point are available only within      *
*      this file.                                                      *
*                                                                      *
***********************************************************************/

/**********************************************************************
 ***                                                                ***
 *** o r d i n a r y _ v a l u e                                    ***
 ***                                                                ***
 *** determine value at a given position for a given field          ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL		ordinary_value

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on field */
	POINT			*ppos,		/* pointer to npos positions on field */
	float			clon,		/* center longitude for field */
	float			*values,	/* pointer to npos values at npos positions */
	STRING			*units		/* pointer to units of values */
	)

	{
	int				ipos, nn;
	FLD_DESCRIPT	descript;
	VLIST			*vlist = NullPtr(VLIST *);

	/* Initialize return parameters */
	if ( NotNull(values) )
		{
		for ( ipos=0; ipos<npos; ipos++ ) values[ipos] = 0.0;
		}
	if ( NotNull(units) ) *units = NullString;

	/* Check for correct number of field descriptors */
	if ( nfds < 1 )
		{
		(void) fprintf(stderr, "[ordinary_value] Error in");
		(void) fprintf(stderr, " number of field descriptors: %d\n", nfds);
		(void) fprintf(stderr, "   Only one field descriptor required (for data)\n");
		return FALSE;
		}

	/* Return FALSE if no information in field descriptors */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	for ( nn=0; nn<nfds; nn++ )
		{
		if ( IsNull(fdescs[nn].sdef) || IsNull(fdescs[nn].subdef) )
			return FALSE;
		if ( nn == 0 || nn < (nfds-1) )
			{
			if ( IsNull(fdescs[nn].edef) || IsNull(fdescs[nn].ldef) )
				return FALSE;
			}
		}

	/* The first field descriptor contains the required data! */
	(void) copy_fld_descript(&descript, &fdescs[0]);

	/* Reset valid time for field (if requested) */
	if ( matched )
		{
		(void) matched_source_valid_time_reset(&descript, FpaC_TIMEDEP_ANY,
				descript.vtime);
		}

	/* Extract data values at all positions */
	vlist = retrieve_vlist(&descript, npos, ppos);

	/* Return FALSE if data cannot be found */
	if ( IsNull(vlist) )
		{
		(void) fprintf(stderr, "[ordinary_value] Cannot retrieve value");
		(void) fprintf(stderr, "  for field: \"%s %s\"\n",
				SafeStr(descript.edef->name), SafeStr(descript.ldef->name));
		return FALSE;
		}

	/* Loop through all positions and set data values */
	if ( NotNull(values) )
		{
		for ( ipos=0; ipos<npos; ipos++ ) values[ipos] = vlist->val[ipos];
		}

	/* Set units to MKS */
	if ( NotNull(units) ) *units = VMks;

	/* Free space used by VLIST Object and return TRUE if all went well */
	(void) free_vlist(vlist);
	FREEMEM(vlist);
	return TRUE;
	}

/**********************************************************************
 ***                                                                ***
 *** v e c t o r _ m a g n i t u d e                                ***
 *** v e c t o r _ d i r e c t i o n                                ***
 ***                                                                ***
 *** extract magnitude or direction at a given position from a      ***
 ***  given vector (or u/v component) field                         ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL		vector_magnitude

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on field */
	POINT			*ppos,		/* pointer to npos positions on field */
	float			clon,		/* center longitude for field */
	float			*values,	/* pointer to npos values at npos positions */
	STRING			*units		/* pointer to units of values */
	)

	{
	int									ipos, nn, nc;
	float								wlat, wlon;
	double								degtorad, uval, vval, sval;
	FpaConfigElementStruct				*edef[2];
	FpaConfigElementComponentStruct		*cmpnt[2];
	FLD_DESCRIPT						descript;
	VLIST								*Uvlist, *Vvlist;
	LOGICAL								UseVector;

	/* Initialize return parameters */
	if ( NotNull(values) )
		{
		for ( ipos=0; ipos<npos; ipos++ ) values[ipos] = 0.0;
		}
	if ( NotNull(units) ) *units = NullString;

	/* Check for correct number of field descriptors */
	if ( nfds < 1 )
		{
		(void) fprintf(stderr, "[vector_magnitude] Error in");
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
		if ( IsNull(fdescs[nn].sdef) || IsNull(fdescs[nn].subdef) )
			return FALSE;
		if ( nn == 0 || nn < (nfds-1) )
			if ( IsNull(fdescs[nn].edef) || IsNull(fdescs[nn].ldef) )
				return FALSE;
		}

	/* Get constant for conversion of degrees to radians */
	if ( !get_values_constant(Rad, VMks, &degtorad) ) return FALSE;

	/* Ensure that the first field descriptor is from a component field */
	/*  with u and v or uv (x and y) components                         */
	if ( IsNull( edef[0] = get_element_info(fdescs[0].edef->name) ) )
		{
		(void) fprintf(stderr, "[vector_magnitude]  \"%s\"  not recognised!\n",
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
							"[vector_magnitude]  \"%s\"  does not have",
							SafeStr(fdescs[0].edef->name));
					(void) fprintf(stderr, " u and v components!\n");
					return FALSE;
					}
				break;

		default:
				(void) fprintf(stderr,
						"[vector_magnitude]  \"%s\"  does not have",
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
			(void) fprintf(stderr, "[vector_magnitude]  \"%s\"  does not have",
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
			(void) fprintf(stderr, "[vector_magnitude]  \"%s\"  is not a u or v",
					SafeStr(fdescs[1].edef->name));
			(void) fprintf(stderr, " component of  \"%s\"!\n",
					SafeStr(fdescs[0].edef->name));
			return FALSE;
			}
		}

	/* Initialize VLIST Objects for vector data at all locations */
	Uvlist = NullPtr(VLIST *);
	Vvlist = NullPtr(VLIST *);

	/* Extract u and v components at all locations   */
	/*  when a vector field is passed                */
	if ( UseVector )
		{
		/* Reset valid time for field (if requested) */
		(void) copy_fld_descript(&descript, &fdescs[0]);
		if ( matched )
			{
			(void) matched_source_valid_time_reset(&descript,
					FpaC_TIMEDEP_ANY, descript.vtime);
			}

		/* Extract both vector components at all locations */
		Uvlist = retrieve_vlist_component(&descript, X_Comp, npos, ppos);
		Vvlist = retrieve_vlist_component(&descript, Y_Comp, npos, ppos);
		}

	/* Extract u and v components at all locations           */
	/*  when both components are passed as field descriptors */
	else if ( nfds > 1 )
		{
		for ( nn=0; nn<nfds; nn++ )
			{

			/* Extract u (x) or v (y) components */
			for ( nc=0; nc<cmpnt[nn]->ncomp; nc++ )
				{

				/* U component */
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

					/* Extract u (x) component at all locations */
					Uvlist = retrieve_vlist(&descript, npos, ppos);
					}

				/* V component */
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

					/* Extract v (y) component at all locations */
					Vvlist = retrieve_vlist(&descript, npos, ppos);
					}
				}
			}
		}

	/* Extract u and v components at all locations */
	/*  when only one component is passed          */
	else
		{

		/* Reset valid time for field (if requested) */
		(void) copy_fld_descript(&descript, &fdescs[0]);
		if ( matched )
			{
			(void) matched_source_valid_time_reset(&descript,
					FpaC_TIMEDEP_ANY, descript.vtime);
			}

		/* Extract u (x) and v (y) components */
		for ( nc=0; nc<cmpnt[0]->ncomp; nc++ )
			{

			/* Reset field descriptor for u or v component */
			(void) set_fld_descript(&descript,
										FpaF_ELEMENT, cmpnt[0]->comp_edefs[nc],
										FpaF_END_OF_LIST);

			/* Extract u (x) component at all locations */
			if ( IsNull(Uvlist) && cmpnt[0]->comp_types[nc] == X_Comp )
				{
				Uvlist = retrieve_vlist(&descript, npos, ppos);
				}

			/* Extract v (y) component at all locations */
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
			(void) fprintf(stderr, "[vector_magnitude] Cannot retrieve u or v");
			(void) fprintf(stderr, " component data from  \"%s %s\"\n",
					SafeStr(fdescs[0].edef->name), SafeStr(fdescs[0].ldef->name));
			}
		else if ( nfds > 1 )
			{
			(void) fprintf(stderr, "[vector_magnitude] Cannot retrieve u or v");
			(void) fprintf(stderr, " component data from  \"%s %s\"  and  \"%s %s\"\n",
					SafeStr(fdescs[0].edef->name), SafeStr(fdescs[0].ldef->name),
					SafeStr(fdescs[1].edef->name), SafeStr(fdescs[1].ldef->name));
			}
		else
			{
			(void) fprintf(stderr, "[vector_magnitude] Cannot retrieve u or v");
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

	/* Loop through all positions and extract vector direction */
	if ( NotNull(values) )
		{
		for ( ipos=0; ipos<npos; ipos++ )
			{

			/* Set latitude and longitude for this position */
			if ( !pos_to_ll(&descript.mproj, ppos[ipos], &wlat, &wlon) )
				continue;

			/* Set vector speed (m/s) */
			uval = (double) Uvlist->val[ipos];
			vval = (double) Vvlist->val[ipos];
			sval = hypot(uval, vval);
			values[ipos] = (float) sval;
			}
		}

	/* Free space used by VLIST Objects */
	(void) free_vlist(Uvlist);
	FREEMEM(Uvlist);
	(void) free_vlist(Vvlist);
	FREEMEM(Vvlist);

	/* Set units to m/s and return TRUE if all went well */
	if ( NotNull(units) ) *units = MperS;
	return TRUE;
	}

static	LOGICAL		vector_direction

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on field */
	POINT			*ppos,		/* pointer to npos positions on field */
	float			clon,		/* center longitude for field */
	float			*values,	/* pointer to npos values at npos positions */
	STRING			*units		/* pointer to units of values */
	)

	{
	int									ipos, nn, nc;
	float								wlat, wlon;
	double								degtorad, uval, vval, dang;
	FpaConfigElementStruct				*edef[2];
	FpaConfigElementComponentStruct		*cmpnt[2];
	FLD_DESCRIPT						descript;
	VLIST								*Uvlist, *Vvlist;
	LOGICAL								UseVector;

	/* Initialize return parameters */
	if ( NotNull(values) )
		{
		for ( ipos=0; ipos<npos; ipos++ ) values[ipos] = 0.0;
		}
	if ( NotNull(units) ) *units = NullString;

	/* Check for correct number of field descriptors */
	if ( nfds < 1 )
		{
		(void) fprintf(stderr, "[vector_direction] Error in");
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
		if ( IsNull(fdescs[nn].sdef) || IsNull(fdescs[nn].subdef) )
			return FALSE;
		if ( nn == 0 || nn < (nfds-1) )
			if ( IsNull(fdescs[nn].edef) || IsNull(fdescs[nn].ldef) )
				return FALSE;
		}

	/* Get constant for conversion of degrees to radians */
	if ( !get_values_constant(Rad, VMks, &degtorad) ) return FALSE;

	/* Ensure that the first field descriptor is from a component field */
	/*  with u and v or uv (x and y) components                         */
	if ( IsNull( edef[0] = get_element_info(fdescs[0].edef->name) ) )
		{
		(void) fprintf(stderr, "[vector_direction]  \"%s\"  not recognised!\n",
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
							"[vector_direction]  \"%s\"  does not have",
							SafeStr(fdescs[0].edef->name));
					(void) fprintf(stderr, " u and v components!\n");
					return FALSE;
					}
				break;

		default:
				(void) fprintf(stderr,
						"[vector_direction]  \"%s\"  does not have",
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
			(void) fprintf(stderr, "[vector_direction]  \"%s\"  does not have",
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
			(void) fprintf(stderr, "[vector_direction]  \"%s\"  is not a u or v",
					SafeStr(fdescs[1].edef->name));
			(void) fprintf(stderr, " component of  \"%s\"!\n",
					SafeStr(fdescs[0].edef->name));
			return FALSE;
			}
		}

	/* Initialize VLIST Objects for vector data at all locations */
	Uvlist = NullPtr(VLIST *);
	Vvlist = NullPtr(VLIST *);

	/* Extract u and v components at all locations   */
	/*  when a vector field is passed                */
	if ( UseVector )
		{
		/* Reset valid time for field (if requested) */
		(void) copy_fld_descript(&descript, &fdescs[0]);
		if ( matched )
			{
			(void) matched_source_valid_time_reset(&descript,
					FpaC_TIMEDEP_ANY, descript.vtime);
			}

		/* Extract both vector components at all locations */
		Uvlist = retrieve_vlist_component(&descript, X_Comp, npos, ppos);
		Vvlist = retrieve_vlist_component(&descript, Y_Comp, npos, ppos);
		}

	/* Extract u and v components at all locations           */
	/*  when both components are passed as field descriptors */
	else if ( nfds > 1 )
		{
		for ( nn=0; nn<nfds; nn++ )
			{

			/* Extract u (x) or v (y) components */
			for ( nc=0; nc<cmpnt[nn]->ncomp; nc++ )
				{

				/* U component */
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

					/* Extract u (x) component at all locations */
					Uvlist = retrieve_vlist(&descript, npos, ppos);
					}

				/* V component */
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

					/* Extract v (y) component at all locations */
					Vvlist = retrieve_vlist(&descript, npos, ppos);
					}
				}
			}
		}

	/* Extract u and v components at all locations */
	/*  when only one component is passed          */
	else
		{

		/* Reset valid time for field (if requested) */
		(void) copy_fld_descript(&descript, &fdescs[0]);
		if ( matched )
			{
			(void) matched_source_valid_time_reset(&descript,
					FpaC_TIMEDEP_ANY, descript.vtime);
			}

		/* Extract u (x) and v (y) components */
		for ( nc=0; nc<cmpnt[0]->ncomp; nc++ )
			{

			/* Reset field descriptor for u or v component */
			(void) set_fld_descript(&descript,
										FpaF_ELEMENT, cmpnt[0]->comp_edefs[nc],
										FpaF_END_OF_LIST);

			/* Extract u (x) component at all locations */
			if ( IsNull(Uvlist) && cmpnt[0]->comp_types[nc] == X_Comp )
				{
				Uvlist = retrieve_vlist(&descript, npos, ppos);
				}

			/* Extract v (y) component at all locations */
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
			(void) fprintf(stderr, "[vector_direction] Cannot retrieve u or v");
			(void) fprintf(stderr, " component data from  \"%s %s\"\n",
					SafeStr(fdescs[0].edef->name), SafeStr(fdescs[0].ldef->name));
			}
		else if ( nfds > 1 )
			{
			(void) fprintf(stderr, "[vector_direction] Cannot retrieve u or v");
			(void) fprintf(stderr, " component data from  \"%s %s\"  and  \"%s %s\"\n",
					SafeStr(fdescs[0].edef->name), SafeStr(fdescs[0].ldef->name),
					SafeStr(fdescs[1].edef->name), SafeStr(fdescs[1].ldef->name));
			}
		else
			{
			(void) fprintf(stderr, "[vector_direction] Cannot retrieve u or v");
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

	/* Loop through all positions and extract vector direction */
	if ( NotNull(values) )
		{
		for ( ipos=0; ipos<npos; ipos++ )
			{

			/* Set latitude and longitude for this position */
			if ( !pos_to_ll(&descript.mproj, ppos[ipos], &wlat, &wlon) )
				continue;

			/* Set vector direction (degrees True) */
			uval = (double) Uvlist->val[ipos];
			vval = (double) Vvlist->val[ipos];
			/* >>>>>
			dang = (fpa_atan2(vval, uval) / degtorad) + 180.0;
			<<<<< */
			dang = fpa_atan2(vval, uval) / degtorad;
			values[ipos] = wind_dir_true(&(descript.mproj), wlat, wlon,
																(float) dang);
			}
		}

	/* Free space used by VLIST Objects */
	(void) free_vlist(Uvlist);
	FREEMEM(Uvlist);
	(void) free_vlist(Vvlist);
	FREEMEM(Vvlist);

	/* Set units to degrees_true and return TRUE if all went well */
	if ( NotNull(units) ) *units = DegreesTrue;
	return TRUE;
	}

/**********************************************************************
 ***                                                                ***
 *** d a i l y _ m a x _ v a l u e _ v a l u e                      ***
 *** d a i l y _ m a x _ v a l u e _ t i m e                        ***
 *** d a i l y _ m a x _ v a l u e                                  ***
 ***                                                                ***
 *** determine maximum value or difference from normal time for     ***
 ***  maximum value at a given position from a series of fields     ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL		daily_max_value_value

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on fields */
	POINT			*ppos,		/* pointer to npos positions on fields */
	float			clon,		/* center longitude for fields */
	float			*values,	/* pointer to npos maximum values */
								/*  at npos positions             */
	STRING			*units		/* pointer to units of values */
	)

	{
	int		ipos, nn;

	/* Initialize return parameters */
	if ( NotNull(values) )
		{
		for ( ipos=0; ipos<npos; ipos++ ) values[ipos] = 0.0;
		}
	if ( NotNull(units) ) *units = NullString;

	/* Check for correct number of field descriptors */
	if ( nfds != 2 )
		{
		(void) fprintf(stderr, "[daily_max_value_value] Error in");
		(void) fprintf(stderr, " number of field descriptors: %d\n", nfds);
		(void) fprintf(stderr, "   Two field descriptors required");
		(void) fprintf(stderr, " (for data and for output field)\n");
		return FALSE;
		}

	/* Return FALSE if no information in field descriptors */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	for ( nn=0; nn<nfds; nn++ )
		{
		if ( IsNull(fdescs[nn].sdef) || IsNull(fdescs[nn].subdef) )
			return FALSE;
		if ( nn < (nfds-1) )
			{
			if ( IsNull(fdescs[nn].edef) || IsNull(fdescs[nn].ldef) )
				return FALSE;
			}
		}

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in daily_max_value_value:\n");
		dprintf(stdout,"   path: %s\n",          fdescs[1].path);
		dprintf(stdout,"   source: %s %s\n",     fdescs[1].sdef->name,
													fdescs[1].subdef->name);
		dprintf(stdout,"   rtime: %s\n",         fdescs[1].rtime);
		dprintf(stdout,"   vtime: %s\n",         fdescs[1].vtime);
		if ( NotNull(fdescs[1].edef) )
			dprintf(stdout,"   element: %s\n",       fdescs[1].edef->name);
		if ( NotNull(fdescs[1].ldef) )
			dprintf(stdout,"   level: %s\n",         fdescs[1].ldef->name);

		dprintf(stdout,"  ... from ...\n");
		dprintf(stdout,"   source: %s %s\n",     fdescs[0].sdef->name,
													fdescs[0].subdef->name);
		dprintf(stdout,"   rtime: %s\n",         fdescs[0].rtime);
		dprintf(stdout,"   function name: %s\n", fdescs[0].value_func_name);
		dprintf(stdout,"   element: %s\n",       fdescs[0].edef->name);
		dprintf(stdout,"   level: %s\n",         fdescs[0].ldef->name);
		}

	/* Check for correct type of fields in each field descriptor */
	if ( NotNull(fdescs[1].edef) && NotNull(fdescs[1].ldef) )
		{
		if ( fdescs[1].edef->elem_tdep->time_dep != FpaC_DAILY )
			{
			(void) fprintf(stderr, "[daily_max_value_value] Output data");
			(void) fprintf(stderr, " field: %s %s  is not a Daily field!\n",
				fdescs[1].edef->name, fdescs[1].ldef->name);
			return FALSE;
			}
		}
	if ( fdescs[0].edef->elem_tdep->time_dep != FpaC_NORMAL )
		{
		(void) fprintf(stderr, "[daily_max_value_value] Input data field:");
		(void) fprintf(stderr, " %s %s  is not an Hourly field!\n",
			fdescs[0].edef->name, fdescs[0].ldef->name);
		return FALSE;
		}

	/* Return values for daily maximum values */
	return daily_max_value(nfds, fdescs, matched,
			npos, ppos, clon, values, units, NullPtr(float *), NullStringPtr);
	}

static	LOGICAL		daily_max_value_time

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on fields */
	POINT			*ppos,		/* pointer to npos positions on fields */
	float			clon,		/* center longitude for fields */
	float			*tdiffs,	/* pointer to npos differences from normal */
								/*  maximum value time at npos positions   */
	STRING			*units		/* pointer to units of time differences */
	)

	{
	int		ipos, nn;

	/* Initialize return parameters */
	if ( NotNull(tdiffs) )
		{
		for ( ipos=0; ipos<npos; ipos++ ) tdiffs[ipos] = 0.0;
		}
	if ( NotNull(units) ) *units = NullString;

	/* Check for correct number of field descriptors */
	if ( nfds != 2 )
		{
		(void) fprintf(stderr, "[daily_max_value_time] Error in");
		(void) fprintf(stderr, " number of field descriptors: %d\n", nfds);
		(void) fprintf(stderr, "   Two field descriptors required");
		(void) fprintf(stderr, " (for data and for output field)\n");
		return FALSE;
		}

	/* Return FALSE if no information in field descriptors */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	for ( nn=0; nn<nfds; nn++ )
		{
		if ( IsNull(fdescs[nn].sdef) || IsNull(fdescs[nn].subdef) )
			return FALSE;
		if ( nn < (nfds-1) )
			{
			if ( IsNull(fdescs[nn].edef) || IsNull(fdescs[nn].ldef) )
				return FALSE;
			}
		}

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in daily_max_value_time:\n");
		dprintf(stdout,"   path: %s\n",          fdescs[1].path);
		dprintf(stdout,"   source: %s %s\n",     fdescs[1].sdef->name,
													fdescs[1].subdef->name);
		dprintf(stdout,"   rtime: %s\n",         fdescs[1].rtime);
		dprintf(stdout,"   vtime: %s\n",         fdescs[1].vtime);
		if ( NotNull(fdescs[1].edef) )
			dprintf(stdout,"   element: %s\n",       fdescs[1].edef->name);
		if ( NotNull(fdescs[1].ldef) )
			dprintf(stdout,"   level: %s\n",         fdescs[1].ldef->name);

		dprintf(stdout,"  ... from ...\n");
		dprintf(stdout,"   source: %s %s\n",     fdescs[0].sdef->name,
													fdescs[0].subdef->name);
		dprintf(stdout,"   rtime: %s\n",         fdescs[0].rtime);
		dprintf(stdout,"   function name: %s\n", fdescs[0].value_func_name);
		dprintf(stdout,"   element: %s\n",       fdescs[0].edef->name);
		dprintf(stdout,"   level: %s\n",         fdescs[0].ldef->name);
		}

	/* Check for correct type of fields in each field descriptor */
	if ( NotNull(fdescs[1].edef) && NotNull(fdescs[1].ldef) )
		{
		if ( fdescs[1].edef->elem_tdep->time_dep != FpaC_DAILY )
			{
			(void) fprintf(stderr, "[daily_max_value_time] Output data");
			(void) fprintf(stderr, " field: %s %s  is not a Daily field!\n",
				fdescs[1].edef->name, fdescs[1].ldef->name);
			return FALSE;
			}
		}
	if ( fdescs[0].edef->elem_tdep->time_dep != FpaC_NORMAL )
		{
		(void) fprintf(stderr, "[daily_max_value_time] Input data field:");
		(void) fprintf(stderr, " %s %s  is not an Hourly field!\n",
			fdescs[0].edef->name, fdescs[0].ldef->name);
		return FALSE;
		}

	/* Return values for daily maximum values */
	return daily_max_value(nfds, fdescs, matched,
			npos, ppos, clon, NullPtr(float *), NullStringPtr, tdiffs, units);
	}

static	LOGICAL		daily_max_value

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on fields */
	POINT			*ppos,		/* pointer to npos positions on fields */
	float			clon,		/* center longitude for fields */
	float			*values,	/* pointer to npos maximum values */
								/*  at npos positions             */
	STRING			*vunits,	/* pointer to units of values */
	float			*tdiffs,	/* pointer to npos differences from normal */
								/*  maximum value time at npos positions   */
	STRING			*tunits		/* pointer to units of time differences */
	)

	{
	int				nn, nvts;
	FLD_DESCRIPT	descript;
	STRING			*vtlst;
	int				ipos, ivt, ivtx;
	float			xlat, xlon;
	float			maxvalue, maxtdiff, subvalue, subtdiff;
	VLIST			**vlist = NullPtr(VLIST **);

	/* Check for correct number of field descriptors */
	if ( nfds != 2 )
		{
		(void) fprintf(stderr, "[daily_max_value] Error in");
		(void) fprintf(stderr, " number of field descriptors: %d\n", nfds);
		(void) fprintf(stderr, "   Two field descriptors required");
		(void) fprintf(stderr, " (for data and for output field)\n");
		return FALSE;
		}

	/* Return FALSE if no information in field descriptors */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	for ( nn=0; nn<nfds; nn++ )
		{
		if ( IsNull(fdescs[nn].sdef) || IsNull(fdescs[nn].subdef) )
			return FALSE;
		if ( nn < (nfds-1) )
			{
			if ( IsNull(fdescs[nn].edef) || IsNull(fdescs[nn].ldef) )
				return FALSE;
			}
		}

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in daily_max_value:\n");
		dprintf(stdout,"   path: %s\n",          fdescs[1].path);
		dprintf(stdout,"   source: %s %s\n",     fdescs[1].sdef->name,
													fdescs[1].subdef->name);
		dprintf(stdout,"   rtime: %s\n",         fdescs[1].rtime);
		dprintf(stdout,"   vtime: %s\n",         fdescs[1].vtime);
		if ( NotNull(fdescs[1].edef) )
			dprintf(stdout,"   element: %s\n",       fdescs[1].edef->name);
		if ( NotNull(fdescs[1].ldef) )
			dprintf(stdout,"   level: %s\n",         fdescs[1].ldef->name);

		dprintf(stdout,"  ... from ...\n");
		dprintf(stdout,"   source: %s %s\n",     fdescs[0].sdef->name,
													fdescs[0].subdef->name);
		dprintf(stdout,"   rtime: %s\n",         fdescs[0].rtime);
		dprintf(stdout,"   function name: %s\n", fdescs[0].value_func_name);
		dprintf(stdout,"   element: %s\n",       fdescs[0].edef->name);
		dprintf(stdout,"   level: %s\n",         fdescs[0].ldef->name);
		}

	/* Check for correct type of fields in each field descriptor */
	if ( NotNull(fdescs[1].edef) && NotNull(fdescs[1].ldef) )
		{
		if ( fdescs[1].edef->elem_tdep->time_dep != FpaC_DAILY )
			{
			(void) fprintf(stderr, "[daily_max_value] Output data");
			(void) fprintf(stderr, " field: %s %s  is not a Daily field!\n",
				fdescs[1].edef->name, fdescs[1].ldef->name);
			return FALSE;
			}
		}
	if ( fdescs[0].edef->elem_tdep->time_dep != FpaC_NORMAL )
		{
		(void) fprintf(stderr, "[daily_max_value] Input data field:");
		(void) fprintf(stderr, " %s %s  is not an Hourly field!\n",
			fdescs[0].edef->name, fdescs[0].ldef->name);
		return FALSE;
		}

	/* Set default units */
	if ( NotNull(vunits) ) *vunits = NullString;
	if ( NotNull(tunits) ) *tunits = NullString;

	/* The first field descriptor contains the input data! */
	(void) copy_fld_descript(&descript, &fdescs[0]);

	/* Get range of valid times for daily maximum value */
	nvts = source_valid_range_for_daily(&fdescs[1], &descript,
			FpaC_NORMAL, clon, &vtlst);
	if ( nvts < 1 )
		{
		dprintf(stdout, "[daily_max_value] Problem with valid timestamps\n");
		dprintf(stdout, "   for field: \"%s %s\"",
				SafeStr(fdescs[1].edef->name), SafeStr(fdescs[1].ldef->name));
		dprintf(stdout, "   from fields: \"%s %s\"",
				SafeStr(descript.edef->name), SafeStr(descript.ldef->name));
		dprintf(stdout, "  for source: \"%s %s\"  at: \"%s\"\n",
				SafeStr(descript.sdef->name), SafeStr(descript.subdef->name),
				SafeStr(descript.rtime));
		return FALSE;
		}

	/* Convert valid time to local GMT time for field */
	dprintf(stdout,"   Local time: \"%s\"\n", SafeStr(fdescs[1].vtime));
	dprintf(stdout,"   Begin timestamp: \"%s\"\n", vtlst[0]);
	dprintf(stdout,"   End timestamp: \"%s\"\n", vtlst[nvts-1]);

	/* Allocate pointers for VLIST Objects at each valid time */
	vlist = GETMEM(vlist, VLIST *, nvts);

	/* Extract data values at each valid time */
	for ( ivt=0; ivt<nvts; ivt++ )
		{

		/* Reset valid time */
		(void) set_fld_descript(&descript, FpaF_VALID_TIME, vtlst[ivt],
								FpaF_END_OF_LIST);

		/* Extract data values at all positions */
		vlist[ivt] = retrieve_vlist(&descript, npos, ppos);

		/* Return FALSE if data is not found */
		if ( IsNull(vlist[ivt]) )
			{
			(void) fprintf(stderr, "[daily_max_value] Missing data");
			(void) fprintf(stderr, " for field: \"%s %s\"\n",
					SafeStr(descript.edef->name), SafeStr(descript.ldef->name));
			(void) fprintf(stderr, "   from source: \"%s %s\"",
					SafeStr(descript.sdef->name),
					SafeStr(descript.subdef->name));
			(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
					SafeStr(descript.rtime), SafeStr(descript.vtime));
			nvts = source_valid_range_for_daily_free(&vtlst, nvts);
			for ( ivtx=0; ivtx<ivt; ivtx++ )
				(void) free_vlist(vlist[ivtx]);
			FREEMEM(vlist);
			return FALSE;
			}
		}

	/* Loop through all positions and set maximum values   */
	/*  and time of maximum values from sequence of fields */
	for ( ipos=0; ipos<npos; ipos++ )
		{

		/* Initialize maximum value and time of maximum value */
		if ( NotNull(values) ) values[ipos] = 0.0;
		if ( NotNull(tdiffs) ) tdiffs[ipos] = 0.0;
		maxvalue = -FPA_FLT_MAX;
		maxtdiff = 0.0;

		/* Set latitude and longitude for this position */
		if ( !pos_to_ll(&descript.mproj, ppos[ipos], &xlat, &xlon) )
			continue;

		/* Check each valid time for new maximum value */
		for ( ivt=0; ivt<nvts; ivt++ )
			{

			/* Set maximum value and time of maximum value */
			/*  from extracted values                      */
			subvalue = vlist[ivt]->val[ipos];
			subtdiff = maxmin_time_difference(fdescs[1].vtime, vtlst[ivt],
																		xlon);
			if ( compare_daily_max_value(&(fdescs[1]),
					subvalue, subtdiff, maxvalue, maxtdiff) )
				{
				maxvalue = subvalue;
				maxtdiff = subtdiff;
				if ( NotNull(values) ) values[ipos] = maxvalue;
				if ( NotNull(tdiffs) ) tdiffs[ipos] = maxtdiff;
				}
			}
		}

	/* Set units to MKS and hr */
	if ( NotNull(vunits) ) *vunits = VMks;
	if ( NotNull(tunits) ) *tunits = VHr;

	/* Free space used by valid times and VLIST Objects */
	/*  and return TRUE if all went well                */
	nvts = source_valid_range_for_daily_free(&vtlst, nvts);
	for ( ivt=0; ivt<nvts; ivt++ )
		(void) free_vlist(vlist[ivt]);
	FREEMEM(vlist);
	return TRUE;
	}

/**********************************************************************
 ***                                                                ***
 *** d a i l y _ m i n _ v a l u e _ v a l u e                      ***
 *** d a i l y _ m i n _ v a l u e _ t i m e                        ***
 *** d a i l y _ m i n _ v a l u e                                  ***
 ***                                                                ***
 *** determine minimum value or difference from normal time for     ***
 ***  minimum value at a given position from a series of fields     ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL		daily_min_value_value

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on fields */
	POINT			*ppos,		/* pointer to npos positions on fields */
	float			clon,		/* center longitude for fields */
	float			*values,	/* pointer to npos minimum values */
								/*  at npos positions             */
	STRING			*units		/* pointer to units of values */
	)

	{
	int		ipos, nn;

	/* Initialize return parameters */
	if ( NotNull(values) )
		{
		for ( ipos=0; ipos<npos; ipos++ ) values[ipos] = 0.0;
		}
	if ( NotNull(units) ) *units = NullString;

	/* Check for correct number of field descriptors */
	if ( nfds != 2 )
		{
		(void) fprintf(stderr, "[daily_min_value_value] Error in");
		(void) fprintf(stderr, " number of field descriptors: %d\n", nfds);
		(void) fprintf(stderr, "   Two field descriptors required");
		(void) fprintf(stderr, " (for data and for output field)\n");
		return FALSE;
		}

	/* Return FALSE if no information in field descriptors */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	for ( nn=0; nn<nfds; nn++ )
		{
		if ( IsNull(fdescs[nn].sdef) || IsNull(fdescs[nn].subdef) )
			return FALSE;
		if ( nn < (nfds-1) )
			{
			if ( IsNull(fdescs[nn].edef) || IsNull(fdescs[nn].ldef) )
				return FALSE;
			}
		}

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in daily_min_value_value:\n");
		dprintf(stdout,"   path: %s\n",          fdescs[1].path);
		dprintf(stdout,"   source: %s %s\n",     fdescs[1].sdef->name,
													fdescs[1].subdef->name);
		dprintf(stdout,"   rtime: %s\n",         fdescs[1].rtime);
		dprintf(stdout,"   vtime: %s\n",         fdescs[1].vtime);
		if ( NotNull(fdescs[1].edef) )
			dprintf(stdout,"   element: %s\n",       fdescs[1].edef->name);
		if ( NotNull(fdescs[1].ldef) )
			dprintf(stdout,"   level: %s\n",         fdescs[1].ldef->name);

		dprintf(stdout,"  ... from ...\n");
		dprintf(stdout,"   source: %s %s\n",     fdescs[0].sdef->name,
													fdescs[0].subdef->name);
		dprintf(stdout,"   rtime: %s\n",         fdescs[0].rtime);
		dprintf(stdout,"   function name: %s\n", fdescs[0].value_func_name);
		dprintf(stdout,"   element: %s\n",       fdescs[0].edef->name);
		dprintf(stdout,"   level: %s\n",         fdescs[0].ldef->name);
		}

	/* Check for correct type of fields in each field descriptor */
	if ( NotNull(fdescs[1].edef) && NotNull(fdescs[1].ldef) )
		{
		if ( fdescs[1].edef->elem_tdep->time_dep != FpaC_DAILY )
			{
			(void) fprintf(stderr, "[daily_min_value_value] Output data");
			(void) fprintf(stderr, " field: %s %s  is not a Daily field!\n",
				fdescs[1].edef->name, fdescs[1].ldef->name);
			return FALSE;
			}
		}
	if ( fdescs[0].edef->elem_tdep->time_dep != FpaC_NORMAL )
		{
		(void) fprintf(stderr, "[daily_min_value_value] Input data field:");
		(void) fprintf(stderr, " %s %s  is not an Hourly field!\n",
			fdescs[0].edef->name, fdescs[0].ldef->name);
		return FALSE;
		}

	/* Return values for daily minimum values */
	return daily_min_value(nfds, fdescs, matched,
			npos, ppos, clon, values, units, NullPtr(float *), NullStringPtr);
	}

static	LOGICAL		daily_min_value_time

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on fields */
	POINT			*ppos,		/* pointer to npos positions on fields */
	float			clon,		/* center longitude for fields */
	float			*tdiffs,	/* pointer to npos differences from normal */
								/*  minimum value time at npos positions   */
	STRING			*units		/* pointer to units of time differences */
	)

	{
	int		ipos, nn;

	/* Initialize return parameters */
	if ( NotNull(tdiffs) )
		{
		for ( ipos=0; ipos<npos; ipos++ ) tdiffs[ipos] = 0.0;
		}
	if ( NotNull(units) ) *units = NullString;

	/* Check for correct number of field descriptors */
	if ( nfds != 2 )
		{
		(void) fprintf(stderr, "[daily_min_value_time] Error in");
		(void) fprintf(stderr, " number of field descriptors: %d\n", nfds);
		(void) fprintf(stderr, "   Two field descriptors required");
		(void) fprintf(stderr, " (for data and for output field)\n");
		return FALSE;
		}

	/* Return FALSE if no information in field descriptors */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	for ( nn=0; nn<nfds; nn++ )
		{
		if ( IsNull(fdescs[nn].sdef) || IsNull(fdescs[nn].subdef) )
			return FALSE;
		if ( nn < (nfds-1) )
			{
			if ( IsNull(fdescs[nn].edef) || IsNull(fdescs[nn].ldef) )
				return FALSE;
			}
		}

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in daily_min_value_time:\n");
		dprintf(stdout,"   path: %s\n",          fdescs[1].path);
		dprintf(stdout,"   source: %s %s\n",     fdescs[1].sdef->name,
													fdescs[1].subdef->name);
		dprintf(stdout,"   rtime: %s\n",         fdescs[1].rtime);
		dprintf(stdout,"   vtime: %s\n",         fdescs[1].vtime);
		if ( NotNull(fdescs[1].edef) )
			dprintf(stdout,"   element: %s\n",       fdescs[1].edef->name);
		if ( NotNull(fdescs[1].ldef) )
			dprintf(stdout,"   level: %s\n",         fdescs[1].ldef->name);

		dprintf(stdout,"  ... from ...\n");
		dprintf(stdout,"   source: %s %s\n",     fdescs[0].sdef->name,
													fdescs[0].subdef->name);
		dprintf(stdout,"   rtime: %s\n",         fdescs[0].rtime);
		dprintf(stdout,"   function name: %s\n", fdescs[0].value_func_name);
		dprintf(stdout,"   element: %s\n",       fdescs[0].edef->name);
		dprintf(stdout,"   level: %s\n",         fdescs[0].ldef->name);
		}

	/* Check for correct type of fields in each field descriptor */
	if ( NotNull(fdescs[1].edef) && NotNull(fdescs[1].ldef) )
		{
		if ( fdescs[1].edef->elem_tdep->time_dep != FpaC_DAILY )
			{
			(void) fprintf(stderr, "[daily_min_value_time] Output data");
			(void) fprintf(stderr, " field: %s %s  is not a Daily field!\n",
				fdescs[1].edef->name, fdescs[1].ldef->name);
			return FALSE;
			}
		}
	if ( fdescs[0].edef->elem_tdep->time_dep != FpaC_NORMAL )
		{
		(void) fprintf(stderr, "[daily_min_value_time] Input data field:");
		(void) fprintf(stderr, " %s %s  is not an Hourly field!\n",
			fdescs[0].edef->name, fdescs[0].ldef->name);
		return FALSE;
		}

	/* Return values for daily minimum values */
	return daily_min_value(nfds, fdescs, matched,
			npos, ppos, clon, NullPtr(float *), NullStringPtr, tdiffs, units);
	}

static	LOGICAL		daily_min_value

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on fields */
	POINT			*ppos,		/* pointer to npos positions on fields */
	float			clon,		/* center longitude for fields */
	float			*values,	/* pointer to npos minimum values */
								/*  at npos positions             */
	STRING			*vunits,	/* pointer to units of values */
	float			*tdiffs,	/* pointer to npos differences from normal */
								/*  minimum value time at npos positions   */
	STRING			*tunits		/* pointer to units of time differences */
	)

	{
	int				nn, nvts;
	FLD_DESCRIPT	descript;
	STRING			*vtlst;
	int				ipos, ivt, ivtx;
	float			xlat, xlon;
	float			minvalue, mintdiff, subvalue, subtdiff;
	VLIST			**vlist = NullPtr(VLIST **);

	/* Check for correct number of field descriptors */
	if ( nfds != 2 )
		{
		(void) fprintf(stderr, "[daily_min_value] Error in");
		(void) fprintf(stderr, " number of field descriptors: %d\n", nfds);
		(void) fprintf(stderr, "   Two field descriptors required");
		(void) fprintf(stderr, " (for data and for output field)\n");
		return FALSE;
		}

	/* Return FALSE if no information in field descriptors */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	for ( nn=0; nn<nfds; nn++ )
		{
		if ( IsNull(fdescs[nn].sdef) || IsNull(fdescs[nn].subdef) )
			return FALSE;
		if ( nn < (nfds-1) )
			{
			if ( IsNull(fdescs[nn].edef) || IsNull(fdescs[nn].ldef) )
				return FALSE;
			}
		}

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in daily_min_value:\n");
		dprintf(stdout,"   path: %s\n",          fdescs[1].path);
		dprintf(stdout,"   source: %s %s\n",     fdescs[1].sdef->name,
													fdescs[1].subdef->name);
		dprintf(stdout,"   rtime: %s\n",         fdescs[1].rtime);
		dprintf(stdout,"   vtime: %s\n",         fdescs[1].vtime);
		if ( NotNull(fdescs[1].edef) )
			dprintf(stdout,"   element: %s\n",       fdescs[1].edef->name);
		if ( NotNull(fdescs[1].ldef) )
			dprintf(stdout,"   level: %s\n",         fdescs[1].ldef->name);

		dprintf(stdout,"  ... from ...\n");
		dprintf(stdout,"   source: %s %s\n",     fdescs[0].sdef->name,
													fdescs[0].subdef->name);
		dprintf(stdout,"   rtime: %s\n",         fdescs[0].rtime);
		dprintf(stdout,"   function name: %s\n", fdescs[0].value_func_name);
		dprintf(stdout,"   element: %s\n",       fdescs[0].edef->name);
		dprintf(stdout,"   level: %s\n",         fdescs[0].ldef->name);
		}

	/* Check for correct type of fields in each field descriptor */
	if ( NotNull(fdescs[1].edef) && NotNull(fdescs[1].ldef) )
		{
		if ( fdescs[1].edef->elem_tdep->time_dep != FpaC_DAILY )
			{
			(void) fprintf(stderr, "[daily_min_value] Output data");
			(void) fprintf(stderr, " field: %s %s  is not a Daily field!\n",
				fdescs[1].edef->name, fdescs[1].ldef->name);
			return FALSE;
			}
		}
	if ( fdescs[0].edef->elem_tdep->time_dep != FpaC_NORMAL )
		{
		(void) fprintf(stderr, "[daily_min_value] Input data field:");
		(void) fprintf(stderr, " %s %s  is not an Hourly field!\n",
			fdescs[0].edef->name, fdescs[0].ldef->name);
		return FALSE;
		}

	/* Set default units */
	if ( NotNull(vunits) ) *vunits = NullString;
	if ( NotNull(tunits) ) *tunits = NullString;

	/* The first field descriptor contains the input data! */
	(void) copy_fld_descript(&descript, &fdescs[0]);

	/* Get range of valid times for daily minimum value */
	nvts = source_valid_range_for_daily(&fdescs[1], &descript,
			FpaC_NORMAL, clon, &vtlst);
	if ( nvts < 1 )
		{
		dprintf(stdout, "[daily_min_value] Problem with valid timestamps\n");
		dprintf(stdout, "   for field: \"%s %s\"",
					SafeStr(fdescs[1].edef->name),
					SafeStr(fdescs[1].ldef->name));
		dprintf(stdout, "   from fields: \"%s %s\"",
				SafeStr(descript.edef->name), SafeStr(descript.ldef->name));
		dprintf(stdout, "  for source: \"%s %s\"  at: \"%s\"\n",
				SafeStr(descript.sdef->name), SafeStr(descript.subdef->name),
				SafeStr(descript.rtime));
		return FALSE;
		}

	/* Convert valid time to local GMT time for field */
	dprintf(stdout,"   Local time: \"%s\"\n", SafeStr(fdescs[1].vtime));
	dprintf(stdout,"   Begin timestamp: \"%s\"\n", vtlst[0]);
	dprintf(stdout,"   End timestamp: \"%s\"\n", vtlst[nvts-1]);

	/* Allocate pointers for VLIST Objects at each valid time */
	vlist = GETMEM(vlist, VLIST *, nvts);

	/* Extract data values at each valid time */
	for ( ivt=0; ivt<nvts; ivt++ )
		{

		/* Reset valid time */
		(void) set_fld_descript(&descript, FpaF_VALID_TIME, vtlst[ivt],
								FpaF_END_OF_LIST);

		/* Extract data values at all positions */
		vlist[ivt] = retrieve_vlist(&descript, npos, ppos);

		/* Return FALSE if data is not found */
		if ( IsNull(vlist[ivt]) )
			{
			(void) fprintf(stderr, "[daily_min_value] Missing data");
			(void) fprintf(stderr, " for field: \"%s %s\"\n",
					SafeStr(descript.edef->name), SafeStr(descript.ldef->name));
			(void) fprintf(stderr, "   from source: \"%s %s\"",
					SafeStr(descript.sdef->name),
					SafeStr(descript.subdef->name));
			(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
					SafeStr(descript.rtime), SafeStr(descript.vtime));
			nvts = source_valid_range_for_daily_free(&vtlst, nvts);
			for ( ivtx=0; ivtx<ivt; ivtx++ )
				(void) free_vlist(vlist[ivtx]);
			FREEMEM(vlist);
			return FALSE;
			}
		}

	/* Loop through all positions and set minimum values   */
	/*  and time of minimum values from sequence of fields */
	for ( ipos=0; ipos<npos; ipos++ )
		{

		/* Initialize minimum value and time of minimum value */
		if ( NotNull(values) ) values[ipos] = 0.0;
		if ( NotNull(tdiffs) ) tdiffs[ipos] = 0.0;
		minvalue = FPA_FLT_MAX;
		mintdiff = 0.0;

		/* Set latitude and longitude for this position */
		if ( !pos_to_ll(&descript.mproj, ppos[ipos], &xlat, &xlon) )
			continue;

		/* Check each valid time for new minimum value */
		for ( ivt=0; ivt<nvts; ivt++ )
			{

			/* Set minimum value and time of minimum value */
			/*  from extracted values                      */
			subvalue = vlist[ivt]->val[ipos];
			subtdiff = maxmin_time_difference(fdescs[1].vtime, vtlst[ivt],
																		xlon);
			if ( compare_daily_min_value(&(fdescs[1]),
					subvalue, subtdiff, minvalue, mintdiff) )
				{
				minvalue = subvalue;
				mintdiff = subtdiff;
				if ( NotNull(values) ) values[ipos] = minvalue;
				if ( NotNull(tdiffs) ) tdiffs[ipos] = mintdiff;
				}
			}
		}

	/* Set units to MKS and hr */
	if ( NotNull(vunits) ) *vunits = VMks;
	if ( NotNull(tunits) ) *tunits = VHr;

	/* Free space used by valid times and VLIST Objects */
	/*  and return TRUE if all went well                */
	nvts = source_valid_range_for_daily_free(&vtlst, nvts);
	for ( ivt=0; ivt<nvts; ivt++ )
		(void) free_vlist(vlist[ivt]);
	FREEMEM(vlist);
	return TRUE;
	}

/**********************************************************************
 ***                                                                ***
 *** g e t _ d a i l y _ v a l u e                                  ***
 ***                                                                ***
 *** determine daily value from field at closest normal time        ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL		get_daily_value

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on fields */
	POINT			*ppos,		/* pointer to npos positions on fields */
	float			clon,		/* center longitude for fields */
	float			*values,	/* pointer to npos daily values */
								/*  at npos positions           */
	STRING			*units		/* pointer to units of values */
	)

	{
	int				ipos, nn, ivt;
	FLD_DESCRIPT	descript;
	STRING			vdaily, vmatch;
	VLIST			*vlist = NullPtr(VLIST *);

	/* Initialize return parameters */
	if ( NotNull(values) )
		{
		for ( ipos=0; ipos<npos; ipos++ ) values[ipos] = 0.0;
		}
	if ( NotNull(units) ) *units = NullString;

	/* Check for correct number of field descriptors */
	if ( nfds != 2 )
		{
		(void) fprintf(stderr, "[get_daily_value] Error in");
		(void) fprintf(stderr, " number of field descriptors: %d\n", nfds);
		(void) fprintf(stderr, "   Two field descriptors required");
		(void) fprintf(stderr, " (for data and for output field)\n");
		return FALSE;
		}

	/* Return FALSE if no information in field descriptors */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	for ( nn=0; nn<nfds; nn++ )
		{
		if ( IsNull(fdescs[nn].sdef) || IsNull(fdescs[nn].subdef) )
			return FALSE;
		if ( nn < (nfds-1) )
			{
			if ( IsNull(fdescs[nn].edef) || IsNull(fdescs[nn].ldef) )
				return FALSE;
			}
		}

	if ( DebugMode )
		{
		dprintf(stdout,"Here we are in get_daily_value:\n");
		dprintf(stdout,"   path: %s\n",          fdescs[1].path);
		dprintf(stdout,"   source: %s %s\n",     fdescs[1].sdef->name,
													fdescs[1].subdef->name);
		dprintf(stdout,"   rtime: %s\n",         fdescs[1].rtime);
		dprintf(stdout,"   vtime: %s\n",         fdescs[1].vtime);
		if ( NotNull(fdescs[1].edef) )
			dprintf(stdout,"   element: %s\n",       fdescs[1].edef->name);
		if ( NotNull(fdescs[1].ldef) )
			dprintf(stdout,"   level: %s\n",         fdescs[1].ldef->name);

		dprintf(stdout,"  ... from ...\n");
		dprintf(stdout,"   source: %s %s\n",     fdescs[0].sdef->name,
													fdescs[0].subdef->name);
		dprintf(stdout,"   rtime: %s\n",         fdescs[0].rtime);
		dprintf(stdout,"   function name: %s\n", fdescs[0].value_func_name);
		dprintf(stdout,"   element: %s\n",       fdescs[0].edef->name);
		dprintf(stdout,"   level: %s\n",         fdescs[0].ldef->name);
		}

	/* Check for correct type of fields in each field descriptor */
	if ( NotNull(fdescs[1].edef) && NotNull(fdescs[1].ldef) )
		{
		if ( fdescs[1].edef->elem_tdep->time_dep != FpaC_DAILY )
			{
			(void) fprintf(stderr, "[get_daily_value] Output data");
			(void) fprintf(stderr, " field: %s %s  is not a Daily field!\n",
				fdescs[1].edef->name, fdescs[1].ldef->name);
			return FALSE;
			}
		}
	if ( !( fdescs[0].edef->elem_tdep->time_dep & ( FpaC_STATIC | FpaC_NORMAL ) ) )
		{
		(void) fprintf(stderr, "[get_daily_value] Input data field:");
		(void) fprintf(stderr, " %s %s  is not an Hourly field!\n",
			fdescs[0].edef->name, fdescs[0].ldef->name);
		return FALSE;
		}

	/* Set the daily time to match from the second field descriptor */
	vdaily = local_to_gmt(fdescs[1].vtime, clon);

	/* The first field descriptor contains the input data! */
	(void) copy_fld_descript(&descript, &fdescs[0]);

	/* Get the closest valid time for the input data matching the daily time */
	ivt = closest_source_valid_time(&descript,
			descript.edef->elem_tdep->time_dep, vdaily, &vmatch);
	if ( ivt < 0 )
		{
		(void) fprintf(stderr, "[get_daily_value] No matching valid timestamp\n");
		(void) fprintf(stderr, "   for field: \"%s %s\"",
				SafeStr(descript.edef->name), SafeStr(descript.ldef->name));
		(void) fprintf(stderr, "  for source: \"%s %s\"  at: \"%s\"\n",
				SafeStr(descript.sdef->name), SafeStr(descript.subdef->name),
				SafeStr(descript.rtime));
		return FALSE;
		}

	/* Extract data values at all positions for the matching valid time */
	(void) set_fld_descript(&descript, FpaF_VALID_TIME, vmatch,
							FpaF_END_OF_LIST);
	vlist = retrieve_vlist(&descript, npos, ppos);

	/* Return FALSE if data is not found */
	if ( IsNull(vlist) )
		{
		(void) fprintf(stderr, "[get_daily_value] Missing data");
		(void) fprintf(stderr, " for field: \"%s %s\"\n",
				SafeStr(descript.edef->name), SafeStr(descript.ldef->name));
		(void) fprintf(stderr, "   from source: \"%s %s\"",
				SafeStr(descript.sdef->name), SafeStr(descript.subdef->name));
		(void) fprintf(stderr, "  at runtime: \"%s\"  validtime: \"%s\"\n",
				SafeStr(descript.rtime), SafeStr(descript.vtime));
		return FALSE;
		}

	/* Loop through all positions and set values */
	if ( NotNull(values) )
		{
		for ( ipos=0; ipos<npos; ipos++ ) values[ipos] = vlist->val[ipos];
		}

	/* Set units to MKS */
	if ( NotNull(units) ) *units = VMks;

	/* Free space used by VLIST Object   */
	/*  and return TRUE if all went well */
	(void) free_vlist(vlist);
	return TRUE;
	}

/**********************************************************************
 ***                                                                ***
 *** f p a _ w i n d _ s p e e d                                    ***
 *** f p a _ w i n d _ d i r e c t i o n                            ***
 ***                                                                ***
 *** extract wind speed or direction at a given position from a     ***
 ***  given adjusted wind field                                     ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL		fpa_wind_speed

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on field */
	POINT			*ppos,		/* pointer to npos positions on field */
	float			clon,		/* center longitude for field */
	float			*values,	/* pointer to npos values at npos positions */
	STRING			*units		/* pointer to units of values */
	)

	{
	int						ipos, nn;
	FLD_DESCRIPT			descript;
	FpaConfigElementStruct	*edef;

	/* Initialize return parameters */
	if ( NotNull(values) )
		{
		for ( ipos=0; ipos<npos; ipos++ ) values[ipos] = 0.0;
		}
	if ( NotNull(units) ) *units = NullString;

	/* Check for correct number of field descriptors */
	if ( nfds < 1 )
		{
		(void) fprintf(stderr, "[fpa_wind_speed] Error in");
		(void) fprintf(stderr, " number of field descriptors: %d\n", nfds);
		(void) fprintf(stderr, "   Only one field descriptor required");
		(void) fprintf(stderr, " (for wind field)\n");
		return FALSE;
		}

	/* Return FALSE if no information in field descriptors */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	for ( nn=0; nn<nfds; nn++ )
		{
		if ( IsNull(fdescs[nn].sdef) || IsNull(fdescs[nn].subdef) )
			return FALSE;
		if ( nn == 0 || nn < (nfds-1) )
			if ( IsNull(fdescs[nn].edef) || IsNull(fdescs[nn].ldef) )
				return FALSE;
		}

	/* The first field descriptor must contain the adjusted wind field! */
	(void) copy_fld_descript(&descript, &fdescs[0]);
	edef = get_element_info(descript.edef->name);
	if ( IsNull(edef) || edef->fld_type != FpaC_WIND )
		{
		(void) fprintf(stderr, "[fpa_wind_speed] Error in field type for: %s %s\n",
				SafeStr(descript.edef->name), SafeStr(descript.ldef->name));
		(void) fprintf(stderr, "   Field must be an adjusted wind field!\n");
		return FALSE;
		}

	/* Set calculation type to extract winds from adjusted wind field */
	(void) set_fld_descript(&descript,
						FpaF_WIND_FUNCTION_NAME, FpaDefaultWindFunc,
						FpaF_END_OF_LIST);

	/* Reset valid time for wind field (if requested) */
	if ( matched )
		{
		(void) matched_source_valid_time_reset(&descript, FpaC_TIMEDEP_ANY,
				descript.vtime);
		}

	/* Extract wind speeds at all positions */
	if ( !extract_awind(1, &descript, matched, npos, ppos, clon,
			NullPtr(float *), values, NullPtr(float *), units) )
		{
		(void) fprintf(stderr, "[fpa_wind_speed] Cannot retrieve wind speed");
		(void) fprintf(stderr, "  from field: \"%s %s\"\n",
				SafeStr(descript.edef->name), SafeStr(descript.ldef->name));
		return FALSE;
		}

	/* Return TRUE if all wind speeds were extracted */
	return TRUE;
	}

static	LOGICAL		fpa_wind_direction

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	int				npos,		/* number of positions on field */
	POINT			*ppos,		/* pointer to npos positions on field */
	float			clon,		/* center longitude for field */
	float			*values,	/* pointer to npos values at npos positions */
	STRING			*units		/* pointer to units of values */
	)

	{
	int						ipos, nn;
	FLD_DESCRIPT			descript;
	FpaConfigElementStruct	*edef;

	/* Initialize return parameters */
	if ( NotNull(values) )
		{
		for ( ipos=0; ipos<npos; ipos++ ) values[ipos] = 0.0;
		}
	if ( NotNull(units) ) *units = NullString;

	/* Check for correct number of field descriptors */
	if ( nfds < 1 )
		{
		(void) fprintf(stderr, "[fpa_wind_direction] Error in");
		(void) fprintf(stderr, " number of field descriptors: %d\n", nfds);
		(void) fprintf(stderr, "   Only one field descriptor required");
		(void) fprintf(stderr, " (for wind field)\n");
		return FALSE;
		}

	/* Return FALSE if no information in field descriptors */
	if ( IsNull(fdescs) ) return FALSE;
	if ( IsNull(ppos) ) return FALSE;
	for ( nn=0; nn<nfds; nn++ )
		{
		if ( IsNull(fdescs[nn].sdef) || IsNull(fdescs[nn].subdef) )
			return FALSE;
		if ( nn == 0 || nn < (nfds-1) )
			if ( IsNull(fdescs[nn].edef) || IsNull(fdescs[nn].ldef) )
				return FALSE;
		}

	/* The first field descriptor must contain the adjusted wind field! */
	(void) copy_fld_descript(&descript, &fdescs[0]);
	edef = get_element_info(descript.edef->name);
	if ( IsNull(edef) || edef->fld_type != FpaC_WIND )
		{
		(void) fprintf(stderr, "[fpa_wind_direction] Error in field type for: %s %s\n",
				SafeStr(descript.edef->name), SafeStr(descript.ldef->name));
		(void) fprintf(stderr, "   Field must be an adjusted wind field!\n");
		return FALSE;
		}

	/* Set calculation type to extract winds from adjusted wind field */
	(void) set_fld_descript(&descript,
						FpaF_WIND_FUNCTION_NAME, FpaDefaultWindFunc,
						FpaF_END_OF_LIST);

	/* Reset valid time for wind field (if requested) */
	if ( matched )
		{
		(void) matched_source_valid_time_reset(&descript, FpaC_TIMEDEP_ANY,
				descript.vtime);
		}

	/* Extract wind directions at all positions */
	if ( !extract_awind(1, &descript, matched, npos, ppos, clon,
			values, NullPtr(float *), NullPtr(float *), NullStringPtr) )
		{
		(void) fprintf(stderr, "[fpa_wind_direction] Cannot retrieve wind direction");
		(void) fprintf(stderr, "  from field: \"%s %s\"\n",
				SafeStr(descript.edef->name), SafeStr(descript.ldef->name));
		return FALSE;
		}

	/* Set units to degrees_true and return TRUE if all went well */
	if ( NotNull(units) ) *units = DegreesTrue;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Setting Max/Min Information)            *
*                                                                      *
*     All the routines after this point are available only within      *
*      this file.                                                      *
*                                                                      *
***********************************************************************/

/**********************************************************************
 ***                                                                ***
 *** c o m p a r e _ d a i l y _ m a x _ v a l u e                  ***
 *** c o m p a r e _ d a i l y _ m i n _ v a l u e                  ***
 ***                                                                ***
 *** compare weighted maximum or minimum values                     ***
 ***                                                                ***
 **********************************************************************/

static	LOGICAL		compare_daily_max_value

	(
	FLD_DESCRIPT	*fdesc,		/* pointer to field descriptor for */
								/*  daily field                    */
	float			subval,		/* input maximum value */
	float			subtd,		/* input time difference for maximum value */
	float			maxval,		/* present maximum value */
	float			maxtd		/* present time difference for maximum value */
	)

	{
	double						timewgt, valuewgt;
	float						swdiff, maxwdiff;
	FpaConfigElementStruct		*edef;
	FpaConfigCrossRefStruct		*crdef;

	/* Return FALSE if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->edef) ) return FALSE;

	/* Get CrossRef structure containing time and value weights */
	edef  = get_element_info(fdesc->edef->name);
	if ( IsNull(edef) || IsNull(edef->elem_detail->valcalc) )
		{
		(void) fprintf(stderr, "[compare_daily_max_value] Missing");
		(void) fprintf(stderr, " detailed information for: \"%s\"\n",
				SafeStr(fdesc->edef->name));
		return FALSE;
		}
	crdef = edef->elem_detail->valcalc->vcalc;
	if ( IsNull(crdef) || IsNull(crdef->unit_t) || IsNull(crdef->unit_v) )
		{
		(void) fprintf(stderr, "[compare_daily_max_value] Missing");
		(void) fprintf(stderr, " CrossRef information for: \"%s\"\n",
				SafeStr(fdesc->edef->name));
		return FALSE;
		}

	/* Set weighting factors for daily maximum values */
	(void) convert_value(crdef->unit_t->name, crdef->wgtt, VHr, &timewgt);
	(void) convert_value(crdef->unit_v->name, crdef->wgtv, VMks, &valuewgt);

	/* Determine weighted differences */
	swdiff   = subval - (float) valuewgt * fabs(subtd / (float) timewgt);
	maxwdiff = maxval - (float) valuewgt * fabs(maxtd / (float) timewgt);

	/* Return TRUE if weighted input maximum value is larger */
	if ( swdiff > maxwdiff )
		{
		return TRUE;
		}

	/* Return FALSE otherwise */
	else
		{
		return FALSE;
		}
	}

static	LOGICAL		compare_daily_min_value

	(
	FLD_DESCRIPT	*fdesc,		/* pointer to field descriptor for */
								/*  daily field                    */
	float			subval,		/* input minimum value */
	float			subtd,		/* input time difference for minimum value */
	float			minval,		/* present minimum value */
	float			mintd		/* present time difference for minimum value */
	)

	{
	double						timewgt, valuewgt;
	float						swdiff, minwdiff;
	FpaConfigElementStruct		*edef;
	FpaConfigCrossRefStruct		*crdef;

	/* Return FALSE if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->edef) ) return FALSE;

	/* Get CrossRef structure containing time and value weights */
	edef  = get_element_info(fdesc->edef->name);
	if ( IsNull(edef) || IsNull(edef->elem_detail->valcalc) )
		{
		(void) fprintf(stderr, "[compare_daily_min_value] Missing");
		(void) fprintf(stderr, " detailed information for: \"%s\"\n",
				SafeStr(fdesc->edef->name));
		return FALSE;
		}
	crdef = edef->elem_detail->valcalc->vcalc;
	if ( IsNull(crdef) || IsNull(crdef->unit_t) || IsNull(crdef->unit_v) )
		{
		(void) fprintf(stderr, "[compare_daily_min_value] Missing");
		(void) fprintf(stderr, " CrossRef information for: \"%s\"\n",
				SafeStr(fdesc->edef->name));
		return FALSE;
		}

	/* Set weighting factors for daily minimum values */
	(void) convert_value(crdef->unit_t->name, crdef->wgtt, VHr, &timewgt);
	(void) convert_value(crdef->unit_v->name, crdef->wgtv, VMks, &valuewgt);

	/* Determine weighted differences */
	swdiff   = subval + (float) valuewgt * fabs(subtd / (float) timewgt);
	minwdiff = minval + (float) valuewgt * fabs(mintd / (float) timewgt);

	/* Return TRUE if weighted input minimum value is smaller */
	if ( swdiff < minwdiff )
		{
		return TRUE;
		}

	/* Return FALSE otherwise */
	else
		{
		return FALSE;
		}
	}

/**********************************************************************
 ***                                                                ***
 *** m a x m i n _ t i m e _ d i f f e r e n c e                    ***
 ***                                                                ***
 *** return time difference in hours between local and valid time   ***
 ***  for maximum or minimum values                                 ***
 ***                                                                ***
 **********************************************************************/

static	float		maxmin_time_difference

	(
	STRING			lcltime,	/* local timestamp */
	STRING			vtime,		/* valid timestamp */
	float			xlon		/* center longitude for location */
	)

	{
	LOGICAL		local;
	int			lyr, ljd, lhr, vyr, vjd, vhr;

	/* Convert local timestamp to date and time in GMT */
	if ( !parse_tstamp(local_to_gmt(lcltime, xlon), &lyr, &ljd, &lhr, NullInt,
			NullLogicalPtr, NullLogicalPtr) )
		{
		(void) fprintf(stderr, "[maxmin_time_difference] Cannot convert");
		(void) fprintf(stderr, " local time: \"%s\"\n", SafeStr(lcltime));
		return 0.0;
		}

	/* Convert valid timestamp to date and time */
	if ( !parse_tstamp(vtime, &vyr, &vjd, &vhr, NullInt, &local,
				NullLogicalPtr) || local )
		{
		(void) fprintf(stderr, "[maxmin_time_difference] Cannot read");
		(void) fprintf(stderr, " valid time: \"%s\"\n", SafeStr(vtime));
		return 0.0;
		}

	/* Return time difference */
	return (float) hdif(lyr, ljd, lhr, vyr, vjd, vhr);
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Testing static routines)                *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

#if defined VALUES_STANDALONE

/* Internal variables to test wind routines */
static	int			TestNumber       = 1;
static	POINT		TestPointArray[] = { ZERO_POINT };

/**********************************************************************
 *** routine to test extract_surface_value                          ***
 **********************************************************************/

static	void		test_extract_surface_value

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	POINT			pos,		/* input position on field */
	float			clon		/* center longitude for fields */
	)

	{
	int			nn;
	float		value;
	STRING		units;

	/* Return if no information in field descriptors */
	for ( nn=0; nn<nfds; nn++ )
		{
		if ( IsNull(fdescs[nn].sdef) || IsNull(fdescs[nn].subdef)
				|| IsNull(fdescs[nn].edef) || IsNull(fdescs[nn].ldef) ) return;
		}

	(void) fprintf(stdout, "\n Point at  xpos: %7.1f", pos[X]);
	(void) fprintf(stdout, "  ypos: %7.1f\n", pos[Y]);
	(void) fprintf(stdout, "   Source: %s %s",
			fdescs[0].sdef->name, fdescs[0].subdef->name);
	(void) fprintf(stdout, "   Runtime: %s\n", fdescs[0].rtime);
	(void) fprintf(stdout, "   Function Name: %s", fdescs[0].value_func_name);
	(void) fprintf(stdout, "   Element: %s   Level: %s\n",
			fdescs[0].edef->name, fdescs[0].ldef->name);
	if ( !inside_map_def(&fdescs[0].mproj.definition, pos)
			|| !inside_grid_def(&fdescs[0].mproj.grid, pos) )
		{
		(void) fprintf(stdout, "      Error in position\n");
		}
	(void) copy_point(TestPointArray[0], pos);
	if ( extract_surface_value(nfds, fdescs, matched,
			TestNumber, TestPointArray, clon, &value, &units) )
		{
		(void) fprintf(stdout, " Value: %f   Units: %s\n", value, units);
		}
	else
		{
		(void) fprintf(stdout, " Error in extract_surface_value\n");
		}
	}

/**********************************************************************
 *** routine to test check_extract_value                            ***
 **********************************************************************/

static	void		test_check_extract_value

	(
	int				nfds,		/* number of field descriptors */
	FLD_DESCRIPT	*fdescs,	/* pointer to nfds field descriptors */
	LOGICAL			matched,	/* match valid times to available fields? */
	POINT			pos			/* input position on field */
	)

	{
	int			nn;

	/* Return if no information in field descriptors */
	for ( nn=0; nn<nfds; nn++ )
		{
		if ( IsNull(fdescs[nn].sdef) || IsNull(fdescs[nn].subdef)
				|| IsNull(fdescs[nn].edef) || IsNull(fdescs[nn].ldef) ) return;
		}

	(void) fprintf(stdout, "\n Point at  xpos: %7.1f", pos[X]);
	(void) fprintf(stdout, "  ypos: %7.1f\n", pos[Y]);
	(void) fprintf(stdout, "   Source: %s %s",
			fdescs[0].sdef->name, fdescs[0].subdef->name);
	(void) fprintf(stdout, "   Runtime: %s\n", fdescs[0].rtime);
	(void) fprintf(stdout, "   Function Name: %s", fdescs[0].value_func_name);
	(void) fprintf(stdout, "   Element: %s   Level: %s\n",
			fdescs[0].edef->name, fdescs[0].ldef->name);
	if ( !inside_map_def(&fdescs[0].mproj.definition, pos)
			|| !inside_grid_def(&fdescs[0].mproj.grid, pos) )
		{
		(void) fprintf(stdout, "      Error in position\n");
		}
	(void) copy_point(TestPointArray[0], pos);
	if ( check_extract_value(nfds, fdescs, matched,
			TestNumber, TestPointArray) )
		{
		(void) fprintf(stdout, " Value can be extracted\n");
		}
	else
		{
		(void) fprintf(stdout, " No field for value\n");
		}
	}

/**********************************************************************
 *** routine to test check_extract_value_by_crossref                ***
 **********************************************************************/

static	void		test_check_extract_value_by_crossref

	(
	STRING			valuecref,	/* value cross reference */
	FLD_DESCRIPT	*fdesc,		/* pointer to field descriptor */
	LOGICAL			matched,	/* match valid times to available fields? */
	POINT			pos			/* input position on field */
	)

	{

	/* Return if no information in field descriptor */
	if ( IsNull(fdesc) || IsNull(fdesc->sdef) || IsNull(fdesc->subdef) ) return;

	(void) fprintf(stdout, "\n Point at  xpos: %7.1f", pos[X]);
	(void) fprintf(stdout, "  ypos: %7.1f\n", pos[Y]);
	(void) fprintf(stdout, "   Value cross reference: %s\n", valuecref);
	(void) fprintf(stdout, "   Source: %s %s",
			fdesc->sdef->name, fdesc->subdef->name);
	(void) fprintf(stdout, "   Runtime: %s\n", fdesc->rtime);
	if ( !inside_map_def(&fdesc->mproj.definition, pos)
			|| !inside_grid_def(&fdesc->mproj.grid, pos) )
		{
		(void) fprintf(stdout, "      Error in position\n");
		}
	(void) copy_point(TestPointArray[0], pos);
	if ( check_extract_value_by_crossref(valuecref, fdesc, matched,
			TestNumber, TestPointArray) )
		{
		(void) fprintf(stdout, " Value can be extracted\n");
		}
	else
		{
		(void) fprintf(stdout, " No field for value\n");
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
float			clon;
STRING			valuefunc, crossref;
STRING			source, subsource, rtime, vtime, element, level;
POINT			pos;
FLD_DESCRIPT	fdescs[2];

/* Set Defaults for VALUES_STANDALONE */

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

/* ... Next set Default Map Projection and center longitude for projection */
mproj = get_target_map();
if ( IsNull(mproj) || ! grid_center(mproj, NullPointPtr, NullFloat, &clon) )
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

/* Testing for extract_surface_value */
(void) fprintf(stdout, "\n\n ***Testing for extract_surface_value***\n");
(void) clear_equation_database();
(void) init_fld_descript(&fdescs[0]);
(void) set_fld_descript(&fdescs[0], FpaF_MAP_PROJECTION, mproj,
						FpaF_END_OF_LIST);
(void) init_fld_descript(&fdescs[1]);
(void) set_fld_descript(&fdescs[1], FpaF_MAP_PROJECTION, mproj,
						FpaF_END_OF_LIST);


(void) fprintf(stdout, "\n    *** ordinary values ***\n");

source    = "depict";	subsource = "";
rtime     = strdup(build_tstamp(1992, 329, 00, 00, FALSE, FALSE));
valuefunc = "FPA_Value_Func";
element   = "temp";		level     = "sfc";
vtime     = strdup(build_tstamp(1992, 329, 00, 00, FALSE, FALSE));
(void) set_fld_descript(&fdescs[0], FpaF_DIRECTORY_PATH, path,
						FpaF_SOURCE_NAME, source,
						FpaF_SUBSOURCE_NAME, subsource,
						FpaF_RUN_TIME, rtime,
						FpaF_VALUE_FUNCTION_NAME, valuefunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
						FpaF_END_OF_LIST);
pos[X] = 0.0;			pos[Y] = 0.0;
(void) test_extract_surface_value(1, fdescs, TRUE, pos, clon);

vtime = strdup(build_tstamp(1992, 329, 12, 00, FALSE, FALSE));
(void) set_fld_descript(&fdescs[0], FpaF_VALID_TIME, vtime, FpaF_END_OF_LIST);
(void) test_extract_surface_value(1, fdescs, TRUE, pos, clon);

vtime = strdup(build_tstamp(1992, 330, 00, 00, FALSE, FALSE));
(void) set_fld_descript(&fdescs[0], FpaF_VALID_TIME, vtime, FpaF_END_OF_LIST);
(void) test_extract_surface_value(1, fdescs, TRUE, pos, clon);

vtime = strdup(build_tstamp(1992, 330, 12, 00, FALSE, FALSE));
(void) set_fld_descript(&fdescs[0], FpaF_VALID_TIME, vtime, FpaF_END_OF_LIST);
(void) test_extract_surface_value(1, fdescs, TRUE, pos, clon);

vtime = strdup(build_tstamp(1992, 331, 00, 00, FALSE, FALSE));
(void) set_fld_descript(&fdescs[0], FpaF_VALID_TIME, vtime, FpaF_END_OF_LIST);
(void) test_extract_surface_value(1, fdescs, TRUE, pos, clon);

vtime = strdup(build_tstamp(1992, 329, 00, 00, FALSE, FALSE));
(void) set_fld_descript(&fdescs[0], FpaF_VALID_TIME, vtime, FpaF_END_OF_LIST);
pos[X] = 500.0;			pos[Y] = 500.0;
(void) test_extract_surface_value(1, fdescs, TRUE, pos, clon);

vtime = strdup(build_tstamp(1992, 329, 12, 00, FALSE, FALSE));
(void) set_fld_descript(&fdescs[0], FpaF_VALID_TIME, vtime, FpaF_END_OF_LIST);
(void) test_extract_surface_value(1, fdescs, TRUE, pos, clon);

vtime = strdup(build_tstamp(1992, 330, 00, 00, FALSE, FALSE));
(void) set_fld_descript(&fdescs[0], FpaF_VALID_TIME, vtime, FpaF_END_OF_LIST);
(void) test_extract_surface_value(1, fdescs, TRUE, pos, clon);

vtime = strdup(build_tstamp(1992, 330, 12, 00, FALSE, FALSE));
(void) set_fld_descript(&fdescs[0], FpaF_VALID_TIME, vtime, FpaF_END_OF_LIST);
(void) test_extract_surface_value(1, fdescs, TRUE, pos, clon);

vtime = strdup(build_tstamp(1992, 331, 00, 00, FALSE, FALSE));
(void) set_fld_descript(&fdescs[0], FpaF_VALID_TIME, vtime, FpaF_END_OF_LIST);
(void) test_extract_surface_value(1, fdescs, TRUE, pos, clon);

element = "max_temp";	level = "sfc";
vtime   = strdup(build_tstamp(1992, 330, 12, 00, FALSE, FALSE));
(void) set_fld_descript(&fdescs[0], FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
						FpaF_END_OF_LIST);
(void) test_extract_surface_value(1, fdescs, TRUE, pos, clon);

(void) fprintf(stdout, "\n    *** daily maximum value ***\n");

source    = "depict";	subsource = "";
rtime     = strdup(build_tstamp(1992, 329, 00, 00, FALSE, FALSE));
valuefunc = "FPA_Daily_Max_Value_Func";
element   = "temp";		level = "sfc";
vtime = strdup(build_tstamp(1992, 329, 14, 00, TRUE, FALSE));
(void) set_fld_descript(&fdescs[0], FpaF_DIRECTORY_PATH, path,
						FpaF_SOURCE_NAME, source,
						FpaF_SUBSOURCE_NAME, subsource,
						FpaF_RUN_TIME, rtime,
						FpaF_VALUE_FUNCTION_NAME, valuefunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
						FpaF_END_OF_LIST);
valuefunc = "FPA_Value_Func";
element   = "max_temp";		level = "sfc";
(void) set_fld_descript(&fdescs[1], FpaF_DIRECTORY_PATH, path,
						FpaF_SOURCE_NAME, source,
						FpaF_SUBSOURCE_NAME, subsource,
						FpaF_RUN_TIME, rtime,
						FpaF_VALUE_FUNCTION_NAME, valuefunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
						FpaF_END_OF_LIST);
pos[X] = 0.0;			pos[Y] = 0.0;
(void) test_extract_surface_value(2, fdescs, TRUE, pos, clon);

vtime = strdup(build_tstamp(1992, 330, 14, 00, TRUE, FALSE));
(void) set_fld_descript(&fdescs[0], FpaF_VALID_TIME, vtime, FpaF_END_OF_LIST);
(void) set_fld_descript(&fdescs[1], FpaF_VALID_TIME, vtime, FpaF_END_OF_LIST);
pos[X] = 500.0;			pos[Y] = 500.0;
(void) test_extract_surface_value(2, fdescs, TRUE, pos, clon);


(void) fprintf(stdout, "\n    *** daily minimum value ***\n");

valuefunc = "FPA_Daily_Min_Value_Func";
element   = "temp";		level = "sfc";
vtime = strdup(build_tstamp(1992, 330, 05, 00, TRUE, FALSE));
(void) set_fld_descript(&fdescs[0],
						FpaF_VALUE_FUNCTION_NAME, valuefunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
						FpaF_END_OF_LIST);
valuefunc = "FPA_Value_Func";
element   = "min_temp";		level = "sfc";
(void) set_fld_descript(&fdescs[1],
						FpaF_VALUE_FUNCTION_NAME, valuefunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
						FpaF_END_OF_LIST);
pos[X] = 0.0;			pos[Y] = 0.0;
(void) test_extract_surface_value(2, fdescs, TRUE, pos, clon);

vtime = strdup(build_tstamp(1992, 331, 05, 00, TRUE, FALSE));
(void) set_fld_descript(&fdescs[0], FpaF_VALID_TIME, vtime, FpaF_END_OF_LIST);
(void) set_fld_descript(&fdescs[1], FpaF_VALID_TIME, vtime, FpaF_END_OF_LIST);
(void) test_extract_surface_value(2, fdescs, TRUE, pos, clon);

vtime = strdup(build_tstamp(1992, 330, 05, 00, TRUE, FALSE));
(void) set_fld_descript(&fdescs[0], FpaF_VALID_TIME, vtime, FpaF_END_OF_LIST);
(void) set_fld_descript(&fdescs[1], FpaF_VALID_TIME, vtime, FpaF_END_OF_LIST);
pos[X] = 500.0;			pos[Y] = 500.0;
(void) test_extract_surface_value(2, fdescs, TRUE, pos, clon);

vtime = strdup(build_tstamp(1992, 331, 05, 00, TRUE, FALSE));
(void) set_fld_descript(&fdescs[0], FpaF_VALID_TIME, vtime, FpaF_END_OF_LIST);
(void) set_fld_descript(&fdescs[1], FpaF_VALID_TIME, vtime, FpaF_END_OF_LIST);
(void) test_extract_surface_value(2, fdescs, TRUE, pos, clon);


(void) fprintf(stdout, "\n    *** daily maximum value time ***\n");

valuefunc = "FPA_Daily_Max_Time_Func";
element   = "temp";		level = "sfc";
vtime = strdup(build_tstamp(1992, 329, 14, 00, TRUE, FALSE));
(void) set_fld_descript(&fdescs[0],
						FpaF_VALUE_FUNCTION_NAME, valuefunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
						FpaF_END_OF_LIST);
valuefunc = "FPA_Value_Func";
element   = "max_temp_time";		level = "sfc";
(void) set_fld_descript(&fdescs[1],
						FpaF_VALUE_FUNCTION_NAME, valuefunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
						FpaF_END_OF_LIST);
pos[X] = 0.0;			pos[Y] = 0.0;
(void) test_extract_surface_value(2, fdescs, TRUE, pos, clon);

vtime = strdup(build_tstamp(1992, 329, 14, 00, TRUE, FALSE));
(void) set_fld_descript(&fdescs[0], FpaF_VALID_TIME, vtime, FpaF_END_OF_LIST);
(void) set_fld_descript(&fdescs[1], FpaF_VALID_TIME, vtime, FpaF_END_OF_LIST);
pos[X] = 500.0;			pos[Y] = 500.0;
(void) test_extract_surface_value(2, fdescs, TRUE, pos, clon);


(void) fprintf(stdout, "\n    *** daily minimum value time ***\n");

valuefunc = "FPA_Daily_Min_Time_Func";
element   = "temp";		level = "sfc";
vtime = strdup(build_tstamp(1992, 330, 05, 00, TRUE, FALSE));
(void) set_fld_descript(&fdescs[0],
						FpaF_VALUE_FUNCTION_NAME, valuefunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
						FpaF_END_OF_LIST);
valuefunc = "FPA_Value_Func";
element   = "min_temp_time";		level = "sfc";
(void) set_fld_descript(&fdescs[1],
						FpaF_VALUE_FUNCTION_NAME, valuefunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
						FpaF_END_OF_LIST);
pos[X] = 0.0;			pos[Y] = 0.0;
(void) test_extract_surface_value(2, fdescs, TRUE, pos, clon);

vtime = strdup(build_tstamp(1992, 330, 05, 00, TRUE, FALSE));
(void) set_fld_descript(&fdescs[0], FpaF_VALID_TIME, vtime, FpaF_END_OF_LIST);
(void) set_fld_descript(&fdescs[1], FpaF_VALID_TIME, vtime, FpaF_END_OF_LIST);
pos[X] = 500.0;			pos[Y] = 500.0;
(void) test_extract_surface_value(2, fdescs, TRUE, pos, clon);

/* Testing for check_extract_value */
(void) fprintf(stdout, "\n\n ***Testing for check_extract_value***\n");

source    = "fem";		subsource = "";
valuefunc = "FPA_Value_Func";
element   = "gz";		level     = "850";
rtime = strdup(build_tstamp(1992, 329, 00, 00, FALSE, FALSE));
vtime = strdup(build_tstamp(1992, 330, 05, 00, TRUE, FALSE));
(void) set_fld_descript(&fdescs[0], FpaF_DIRECTORY_PATH, path,
						FpaF_SOURCE_NAME, source,
						FpaF_SUBSOURCE_NAME, subsource,
						FpaF_RUN_TIME, rtime,
						FpaF_VALUE_FUNCTION_NAME, valuefunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
						FpaF_END_OF_LIST);
pos[X] = 0.0;			pos[Y] = 0.0;
(void) test_check_extract_value(1, fdescs, TRUE, pos);

element = "rh";			level = "850";
(void) set_fld_descript(&fdescs[0],
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_END_OF_LIST);
(void) test_check_extract_value(1, fdescs, TRUE, pos);

source  = "depict";		subsource = "";
(void) set_fld_descript(&fdescs[0], FpaF_DIRECTORY_PATH, path,
						FpaF_SOURCE_NAME, source,
						FpaF_SUBSOURCE_NAME, subsource,
						FpaF_RUN_TIME, rtime,
						FpaF_END_OF_LIST);
(void) test_check_extract_value(1, fdescs, TRUE, pos);

valuefunc = "FPA_Daily_Max_Value_Func";
element   = "temp";		level = "sfc";
(void) set_fld_descript(&fdescs[0],
						FpaF_VALUE_FUNCTION_NAME, valuefunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_END_OF_LIST);
valuefunc = "FPA_Value_Func";
element   = "max_temp";		level = "sfc";
(void) set_fld_descript(&fdescs[1],
						FpaF_VALUE_FUNCTION_NAME, valuefunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_END_OF_LIST);
(void) test_check_extract_value(2, fdescs, TRUE, pos);

source  = "fem";		subsource = "";
element = "maxtmp";		level     = "sfc";
(void) set_fld_descript(&fdescs[0], FpaF_DIRECTORY_PATH, path,
						FpaF_SOURCE_NAME, source,
						FpaF_SUBSOURCE_NAME, subsource,
						FpaF_RUN_TIME, rtime,
						FpaF_VALUE_FUNCTION_NAME, valuefunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
						FpaF_END_OF_LIST);
(void) test_check_extract_value(1, fdescs, TRUE, pos);

/* Testing for check_extract_value_by_crossref */
(void) fprintf(stdout, "\n\n ***Testing for");
(void) fprintf(stdout, " check_extract_value_by_crossref***\n");

crossref  = "Max_Temp";
source    = "fem";		subsource = "";
valuefunc = "";
element   = "";			level     = "";
(void) set_fld_descript(&fdescs[0], FpaF_DIRECTORY_PATH, path,
						FpaF_SOURCE_NAME, source,
						FpaF_SUBSOURCE_NAME, subsource,
						FpaF_RUN_TIME, rtime,
						FpaF_VALUE_FUNCTION_NAME, valuefunc,
						FpaF_ELEMENT_NAME, element,
						FpaF_LEVEL_NAME, level,
						FpaF_VALID_TIME, vtime,
						FpaF_END_OF_LIST);
(void) test_check_extract_value_by_crossref(crossref, &fdescs[0], TRUE, pos);

source = "depict";		subsource = "";
(void) set_fld_descript(&fdescs[0], FpaF_DIRECTORY_PATH, path,
						FpaF_SOURCE_NAME, source,
						FpaF_SUBSOURCE_NAME, subsource,
						FpaF_RUN_TIME, rtime,
						FpaF_END_OF_LIST);
(void) test_check_extract_value_by_crossref(crossref, &fdescs[0], TRUE, pos);

crossref = "Min_Temp_Time";
pos[X] = 1000.0;		pos[Y] = 1000.0;
(void) test_check_extract_value_by_crossref(crossref, &fdescs[0], TRUE, pos);

pos[X] = 10000.0;		pos[Y] = 10000.0;
(void) test_check_extract_value_by_crossref(crossref, &fdescs[0], TRUE, pos);

/* Testing for ... */

return 0;
}

#endif /* VALUES_STANDALONE */
