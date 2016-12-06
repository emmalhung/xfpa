/***********************************************************************
*                                                                      *
*   g r i b d a t a . h                                                *
*                                                                      *
*   Routines to convert decoded GRIB data to FPA Images                *
*   (include file)                                                     *
*                                                                      *
*     Version 6 (c) Copyright 2006 Environment Canada (MSC)            *
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
#ifndef GRIBDATA_DEFS
#define GRIBDATA_DEFS


/***********************************************************************
*                                                                      *
*  Initialize defined constants for gribdata routines                  *
*                                                                      *
***********************************************************************/

#ifdef GRIBDATA_MAIN

#endif

/***********************************************************************
*                                                                      *
*  Declare external functions in gribdata.c                            *
*                                                                      *
***********************************************************************/

void gribfield_to_data( DECODEDFIELD *, FLD_DESCRIPT *, STRING, float, float);

/* Now it has been included */
#endif
