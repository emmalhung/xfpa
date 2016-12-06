/*********************************************************************/
/**	@file values.h
 *
 * Routines to extract values from fields of meteorological data.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*   v a l u e s . h                                                    *
*                                                                      *
*   Routines to extract values from fields of meteorological data      *
*   (include file)                                                     *
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
#ifndef VALUES_DEFS
#define VALUES_DEFS


/* We need definitions for low level types */
#include <fpa_types.h>

/* We need definitions for other Objects and Environ parameters */
#include <objects/objects.h>
#include <environ/environ.h>


/* Define VALUEFUNC object - FPA value function pointer */
typedef	LOGICAL			(VALUEFUNC_FUNC)(int nfds, FLD_DESCRIPT *fdescs,
							LOGICAL matched, int npos, POINT *ppos, float clon,
							float *values, STRING *vunits);
typedef	VALUEFUNC_FUNC	(*VALUEFUNC);

/* Define FPA value function search list structure */
typedef struct
	{
	STRING		name;		/* config file value function name */
	VALUEFUNC	func;		/* function */
	int			nreq;		/* number of fields required by function */
	} VALUEFUNC_TABLE;

/* These are the pre-defined config file value function names */
/*  "FPA_Value_Func" (or FpaDefaultValueFunc in code)         */
/*  "FPA_Vector_Magnitude_Func"                               */
/*  "FPA_Vector_Direction_Func"                               */
/*  "FPA_Daily_Max_Value_Func"                                */
/*  "FPA_Daily_Max_Time_Func"                                 */
/*  "FPA_Daily_Min_Value_Func"                                */
/*  "FPA_Daily_Min_Time_Func"                                 */
/*  "FPA_Get_Daily_Value_Func"                                */
/*  "FPA_Actual_Wind_Speed_Func"                              */
/*  "FPA_Actual_Wind_Direction_Func"                          */

/* Convenient definitions */
#define	NullValueFunc		NullPtr(VALUEFUNC)
#define	NullValueFuncPtr	NullPtr(VALUEFUNC *)
#define	NullValueFuncList	NullPtr(VALUEFUNC *)


/***********************************************************************
*                                                                      *
*  Initialize defined constants for values routines                    *
*                                                                      *
***********************************************************************/

#ifdef VALUES_MAIN

#endif


/***********************************************************************
*                                                                      *
*  Declare external functions in values.c                              *
*                                                                      *
***********************************************************************/

LOGICAL		identify_value_function(STRING name, VALUEFUNC *func, int *nflds);
void		display_value_functions(void);
LOGICAL		get_values_constant(STRING name, STRING units, double *value);
LOGICAL		extract_surface_value(int nfds, FLD_DESCRIPT *fdescs,
						LOGICAL matched, int npos, POINT *ppos, float clon,
						float *values, STRING *vunits);
LOGICAL		extract_surface_value_by_crossref(STRING valuecref,
						FLD_DESCRIPT *fdesc, LOGICAL matched,
						int npos, POINT *ppos, float clon,
						float *values, STRING *vunits);
LOGICAL		extract_surface_value_by_equation(STRING units, STRING equation,
						FLD_DESCRIPT *fdesc, LOGICAL matched,
						int npos, POINT *ppos, float clon,
						float *values, STRING *vunits);
LOGICAL		extract_areaset_attribs(int nfds, FLD_DESCRIPT *fdescs,
						LOGICAL matched, int npos, POINT *ppos, float clon,
						CAL *cals);
LOGICAL		extract_areaset_attribs_by_crossref(STRING valuecref,
						FLD_DESCRIPT *fdesc, LOGICAL matched,
						int npos, POINT *ppos, float clon, CAL *cals);
LOGICAL		extract_areaset_value(int nfds, FLD_DESCRIPT *fdescs,
						LOGICAL matched, int npos, POINT *ppos, float clon,
						STRING *subelems, STRING *values, STRING *labels);
LOGICAL		extract_areaset_value_by_crossref(STRING valuecref,
						FLD_DESCRIPT *fdesc, LOGICAL matched,
						int npos, POINT *ppos, float clon,
						STRING *subelems, STRING *values, STRING *labels);
void		check_value_function_error_messages(LOGICAL errorflag);
LOGICAL		check_extract_value(int nfds, FLD_DESCRIPT *fdescs,
						LOGICAL matched, int npos, POINT *ppos);
LOGICAL		check_extract_value_by_crossref(STRING valuecref,
						FLD_DESCRIPT *fdesc, LOGICAL matched,
						int npos, POINT *ppos);
FIELD		extract_field_by_value_crossref(FLD_DESCRIPT *fdescout,
						STRING valuecref, FLD_DESCRIPT *fdescin,
						LOGICAL matched);
METAFILE	extract_metafile_by_value_crossref(FLD_DESCRIPT *fdescout,
						STRING valuecref, FLD_DESCRIPT *fdescin,
						LOGICAL matched);


/***********************************************************************
*                                                                      *
*  Declare external functions in lib/userlib/user_values.c             *
*                                                                      *
***********************************************************************/

LOGICAL		identify_user_value_function(STRING name, VALUEFUNC * func,
						int *nflds);
void		display_user_value_functions(void);


/* Now it has been included */
#endif
