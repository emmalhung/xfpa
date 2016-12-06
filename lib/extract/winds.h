/*********************************************************************/
/**	@file winds.h
 *
 * Routines to extract wind speed and direction from fields of
 * meteorological data.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*   w i n d s . h                                                      *
*                                                                      *
*   Routines to extract wind speed and direction from fields of        *
*   meteorological data (include file)                                 *
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
#ifndef WINDS_DEFS
#define WINDS_DEFS


/* We need definitions for low level types */
#include <fpa_types.h>

/* We need definitions for other Objects and Environ parameters */
#include <objects/objects.h>
#include <environ/environ.h>


/* Define WINDFUNC object - FPA wind function pointer */
typedef	LOGICAL			(WINDFUNC_FUNC)(int nfds, FLD_DESCRIPT *fdescs,
							LOGICAL matched, int npos, POINT *ppos, float clon,
							float *wdirs, float *wspds, float *wgsts,
							STRING *wunits);
typedef	WINDFUNC_FUNC	(*WINDFUNC);

/* Define FPA wind function search list structure */
typedef struct
	{
	STRING		name;		/* config file wind function name */
	WINDFUNC	func;		/* function */
	int			nreq;		/* number of fields required by function */
	} WINDFUNC_TABLE;

/* These are the pre-defined config file wind function names   */
/*  "FPA_Adjusted_Wind_Func" (or FpaDefaultWindFunc in code)   */
/*  "FPA_Absolute_Wind_Func" (or FpaAbsWindFunc in code)       */
/*  "FPA_Geostrophic_Wind_Func"                                */
/*  "FPA_Thermal_Wind_Func"                                    */
/*  "FPA_Gradient_Wind_Func"                                   */
/*  "FPA_Cyclostrophic_Wind_Func"                              */
/*  "FPA_Isallobaric_Wind_Func"                                */
/*  "FPA_UVcomponent_Wind_Func" (or FpaVectorWindFunc in code) */
/*  "FPA_DirectionSpeed_Wind_Func"                             */

/* Convenient definitions */
#define	NullWindFunc		NullPtr(WINDFUNC)
#define	NullWindFuncPtr		NullPtr(WINDFUNC *)
#define	NullWindFuncList	NullPtr(WINDFUNC *)


/***********************************************************************
*                                                                      *
*  Initialize defined constants for winds routines                     *
*                                                                      *
***********************************************************************/

#ifdef WINDS_MAIN

#endif


/***********************************************************************
*                                                                      *
*  Declare external functions in winds.c                               *
*                                                                      *
***********************************************************************/

LOGICAL		identify_wind_function(STRING name, WINDFUNC *func, int *nflds);
void		display_wind_functions(void);
LOGICAL		get_winds_constant(STRING name, STRING units, double *value);
LOGICAL		extract_areaset_wcalcs(int nfds, FLD_DESCRIPT *fdescs,
						LOGICAL matched, int npos, POINT *ppos, float clon,
						WIND_CALC **wclist);
LOGICAL		extract_areaset_wcalcs_by_crossref(STRING valuecref,
						FLD_DESCRIPT *fdesc, LOGICAL matched,
						int npos, POINT *ppos, float clon, WIND_CALC **wclist);
LOGICAL		extract_awind(int nfds, FLD_DESCRIPT *fdescs, LOGICAL matched,
						int npos, POINT *ppos, float clon,
						float *wdirs, float *wspds, float *wgsts,
						STRING *wunits);
LOGICAL		extract_awind_by_crossref(STRING windcref, FLD_DESCRIPT *fdesc,
						LOGICAL matched, int npos, POINT *ppos, float clon,
						float *wdirs, float *wspds, float *wgsts,
						STRING *wunits);

/* >>> the following are obsolete in next version <<< */
LOGICAL		extract_wind(int nfds, FLD_DESCRIPT *fdescs, LOGICAL matched,
						int npos, POINT *ppos, float clon,
						float *wdirs, float *wspds, STRING *wunits);
LOGICAL		extract_wind_by_crossref(STRING windcref, FLD_DESCRIPT *fdesc,
						LOGICAL matched, int npos, POINT *ppos, float clon,
						float *wdirs, float *wspds, STRING *wunits);
/* >>> the preceding are obsolete in next version <<< */

void		check_wind_function_error_messages(LOGICAL errorflag);
LOGICAL		check_extract_wind(int nfds, FLD_DESCRIPT *fdescs, LOGICAL matched,
						int npos, POINT *ppos);
LOGICAL		check_extract_wind_by_crossref(STRING windcref, FLD_DESCRIPT *fdesc,
						LOGICAL matched, int npos, POINT *ppos);


/***********************************************************************
*                                                                      *
*  Declare external functions in lib/userlib/user_winds.c              *
*                                                                      *
***********************************************************************/

LOGICAL		identify_user_wind_function(STRING name, WINDFUNC *func,
						int *nflds);
void		display_user_wind_functions(void);


/* Now it has been included */
#endif
