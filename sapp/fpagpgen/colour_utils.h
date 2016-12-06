/***********************************************************************
*                                                                      *
*     c o l o u r _ u t i l s . h                                      *
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
#ifndef COLOUR_UTILS_DEFS
#define COLOUR_UTILS_DEFS


/* We need FPA library definitions */
#include <fpa.h>


/***********************************************************************
*                                                                      *
*  Initialize defined constants for colour_utils routines              *
*                                                                      *
***********************************************************************/

#ifdef COLOUR_UTILS_MAIN


#endif


/***********************************************************************
*                                                                      *
*  Declare external functions in colour_utils.c                        *
*                                                                      *
***********************************************************************/

LOGICAL		convert_cmyk_for_psmet(STRING);
LOGICAL		convert_rgb_for_psmet(STRING);
LOGICAL		convert_x11_for_psmet(STRING);
LOGICAL		convert_cmyk_for_svgmet(STRING);
LOGICAL		convert_rgb_for_svgmet(STRING);
LOGICAL		convert_x11_for_svgmet(STRING);
LOGICAL		convert_cmyk_for_cormet(STRING);
LOGICAL		convert_rgb_for_cormet(STRING);
LOGICAL		convert_x11_for_cormet(STRING);
LOGICAL		convert_cmyk_for_radar(STRING);
LOGICAL		convert_rgb_for_radar(STRING);
LOGICAL		convert_x11_for_radar(STRING);
LOGICAL		x11name_to_cmyk(STRING, int *, int *, int *, int *);
LOGICAL		x11name_to_rgb(STRING, int *, int *, int *);
void		cmyk_to_rgb(int, int, int, int, int *, int *, int *);
void		rgb_to_cmyk(int, int, int, int*, int *, int *, int *);


/* Now it has been included */
#endif
