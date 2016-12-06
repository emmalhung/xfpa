/**********************************************************************/
/** @file presentation.h
 *
 *  Routines to handle the presentation config file (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   p r e s e n t a t i o n . h                                        *
*                                                                      *
*   Routines to handle the presentation config file (include file)     *
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
#ifndef PRESENTATION_DEFS
#define PRESENTATION_DEFS


/* We need definitions for low level types and other Objects */
#include <objects/objects.h>
#include <fpa_types.h>


/***********************************************************************
*                                                                      *
*  Initialize defined constants for presentation config routines       *
*                                                                      *
***********************************************************************/

#ifdef PRESENTATION_INIT


#endif


/***********************************************************************
*                                                                      *
*  Declare external functions in presentation.c and units.c            *
*                                                                      *
***********************************************************************/

LOGICAL	setup_metafile_presentation(METAFILE meta, STRING source);
LOGICAL	setup_fld_presentation(FIELD fld, STRING source);
LOGICAL	setup_sfc_presentation(SURFACE sfc, STRING elem, STRING level,
						STRING source);
LOGICAL	setup_set_presentation(SET set, STRING elem, STRING level,
						STRING source);
LOGICAL	setup_plot_presentation(PLOT plot, STRING elem, STRING level,
						STRING source);
int		get_conspecs(STRING elem, STRING level, STRING source, STRING mtype,
						STRING mname, CONSPEC **conspecs);
int		get_catspecs(STRING elem, STRING level, STRING source, STRING mtype,
						STRING mname, CATSPEC **catspecs);
int		get_pltspecs(STRING elem, STRING level, STRING source, STRING mtype,
						STRING mname, PLTSPEC **pltspecs);

LOGICAL	setup_metafile_units(METAFILE meta, STRING source);
LOGICAL	setup_fld_units(FIELD fld, STRING source);
LOGICAL	setup_sfc_units(SURFACE sfc, STRING elem, STRING level, STRING source);
int		get_unitspec(STRING elem, STRING level, STRING source, STRING mtype,
						STRING mname, USPEC **uspecs);


/* Now it has been included */
#endif
