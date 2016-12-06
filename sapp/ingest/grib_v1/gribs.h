/***********************************************************************
*                                                                      *
*   g r i b s . h                                                      *
*                                                                      *
*   Routines accessing Gribs Config file (include file)                *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
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
#ifndef GRIBS_DEFS
#define GRIBS_DEFS


/* We need FPA definitions */
#include <fpa.h>


/***********************************************************************
*                                                                      *
*  Initialize defined constants for gribs routines                     *
*                                                                      *
***********************************************************************/

#ifdef GRIBS_INIT


#endif


/***********************************************************************
*                                                                      *
*  Declare external functions in gribs.c                               *
*                                                                      *
***********************************************************************/

/* Interface functions ... reading configuration file */
LOGICAL	read_complete_gribs_file();

/* Interface functions ... replacing defaults */
LOGICAL	replace_grib_default_model(STRING *);
LOGICAL	replace_grib_default_element(STRING *, STRING *);

/* Interface functions ... fields to skip */
LOGICAL	skip_grib_field_orig(STRING, STRING);


/* Now it has been included */
#endif
