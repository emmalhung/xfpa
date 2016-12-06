/***********************************************************************
*                                                                      *
*   g r i b m e t a . h                                                *
*                                                                      *
*   Routines to convert decoded GRIB data to FPA Objects               *
*   (include file)                                                     *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
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
#ifndef GRIBMETA_DEFS
#define GRIBMETA_DEFS


/* We need FPA definitions */
#include <fpa.h>

/***********************************************************************
*                                                                      *
*  Initialize defined constants for gribmeta routines                  *
*                                                                      *
***********************************************************************/

#ifdef GRIBMETA_MAIN


#endif

/***********************************************************************
*                                                                      *
*  Declare external functions in gribmeta.c                            *
*                                                                      *
***********************************************************************/

METAFILE	gribfield_to_metafile(DECODEDFIELD *, FLD_DESCRIPT *, STRING);
SURFACE		gribfield_to_surface(DECODEDFIELD *, FLD_DESCRIPT *, STRING);
METAFILE	gribfield_to_metafile_by_comp(DECODEDFIELD *, FLD_DESCRIPT *, STRING,
											COMPONENT, COMPONENT);
SURFACE		gribfield_to_surface_by_comp(DECODEDFIELD *, FLD_DESCRIPT *, STRING,
											COMPONENT, COMPONENT);

/* Now it has been included */
#endif
