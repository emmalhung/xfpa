/***********************************************************************
*                                                                      *
*     c l i p p e r . h                                                *
*                                                                      *
*     Version 5 (c) Copyright 1997 Environment Canada (AES)            *
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
#ifndef CLIPPER_DEFS
#define CLIPPER_DEFS


/* We need FPA library definitions */
#include <fpa.h>


/***********************************************************************
*                                                                      *
*  Initialize defined constants for clipper routines                   *
*                                                                      *
***********************************************************************/

#ifdef CLIPPER_MAIN


#endif


/***********************************************************************
*                                                                      *
*  Declare external functions in clipper.c                             *
*                                                                      *
***********************************************************************/

BOUND		GRA_clip_boundary(BOUND);
LINE		GRA_clip_outline(LINE);
LINE		GRA_clip_outline_segment(LINE);
int			GRA_clip_line(LINE, LINE **);


/* Now it has been included */
#endif
