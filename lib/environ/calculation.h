/**********************************************************************/
/** @file calculation.h
 *
 *  Routines to handle various calculations, formatting and encoding
 *   for winds and values (include file).
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   c a l c u l a t i o n . h                                          *
*                                                                      *
*   Routines to handle various calculations, formatting and encoding   *
*   for winds and values (include file).                               *
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
#ifndef CALC_DEFS
#define CALC_DEFS


/* We need definitions for low level types and other Objects */
#include <fpa_types.h>

#include "cal.h"


/***********************************************************************
*                                                                      *
*  Initialize defined constants                                        *
*                                                                      *
***********************************************************************/

/** @{ */
/** Macros for preset value and wind function names */
#define	FpaDefaultValueFunc "FPA_Value_Func"
#define	FpaDefaultWindFunc  "FPA_Adjusted_Wind_Func"
#define	FpaAbsWindModel     "FPA_Absolute_Wind_Model"
#define	FpaAbsWindFunc      "FPA_Absolute_Wind_Func"
/** @} */

/** Wind component structure */
typedef	struct	wind_struct
	{
	float	dir;		/**< direction */
	STRING	dunit;		/**< direction units */
	float	speed;		/**< speed */
	float	gust;		/**< gust */
	STRING	sunit;		/**< speed units */
	} WIND_VAL;
/** Wind calculation structure */
typedef	struct	wcalc_struct
	{
	STRING	model;		/**< model to use */
	LOGICAL	rel_dir;	/**< use relative direction? */
	LOGICAL	rel_speed;	/**< use relative speed? */
	float	dir;		/**< direction */
	float	speed;		/**< speed */
	float	gust;		/**< gust */
	} WIND_CALC;

#ifdef CALC_INIT

#endif


/***********************************************************************
*                                                                      *
*  Declare external functions in calculation.c                         *
*                                                                      *
***********************************************************************/
/**********************************************************************/
STRING		build_wind_value_string(WIND_VAL *wv);
LOGICAL		parse_wind_value_string(STRING value, WIND_VAL *wv);
LOGICAL		consistent_wind_attribs(CAL cal);
LOGICAL		build_wind_attribs(CAL cal, WIND_CALC *wc);
LOGICAL		parse_wind_attribs(CAL cal, WIND_CALC *wc);
STRING		build_wind_attrib_string(STRING att, STRING cfgval);
LOGICAL		consistent_wind_attrib_strings(STRING wmodel, STRING wdir,
						STRING wspeed, STRING wgust, STRING *cmodel,
						STRING *cdir, STRING *cspeed, STRING *cgust,
						STRING *ccat, STRING *cval, STRING *clab);
STRING		build_wind_label_attrib(STRING att, WIND_CALC *wc);

/* Now it has been included */
#endif
