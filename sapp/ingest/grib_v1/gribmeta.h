/***********************************************************************
*                                                                      *
*   g r i b m e t a . h                                                *
*                                                                      *
*   Routines to convert decoded GRIB data to FPA Objects               *
*   (include file)                                                     *
*                                                                      *
*   Version 4 (c) Copyright 1996 Environment Canada (AES)              *
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

/* We need definitions for GRIB data structures */
#include "rgrib_edition1.h"


/***********************************************************************
*                                                                      *
*  Initialize defined constants for gribmeta routines                  *
*                                                                      *
***********************************************************************/

#ifdef GRIBMETA_MAIN


#endif

/* Set definition for number of significant digits in writing metafiles */
#define MaxDigits	4


/***********************************************************************
*                                                                      *
*  Declare external functions in gribmeta.c                            *
*                                                                      *
***********************************************************************/

LOGICAL		gribfield_mapproj(GRIBFIELD *, MAP_PROJ *);
METAFILE	gribfield_to_metafile(GRIBFIELD *, FLD_DESCRIPT *, STRING);
SURFACE		gribfield_to_surface(GRIBFIELD *, FLD_DESCRIPT *, STRING);
METAFILE	gribfield_to_metafile_by_comp(GRIBFIELD *, FLD_DESCRIPT *, STRING,
											COMPONENT, COMPONENT);
SURFACE		gribfield_to_surface_by_comp(GRIBFIELD *, FLD_DESCRIPT *, STRING,
											COMPONENT, COMPONENT);


/* Now it has been included */
#endif
