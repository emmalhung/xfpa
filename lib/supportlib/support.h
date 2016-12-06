/**********************************************************************/
/** @file support.h
 *  Support for obsolescent functions (include file)
 *
 *  Version 7 (c) Copyright 2006 Environment Canada
 *  Version 8 (c) Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   s u p p o r t . h                                                  *
*                                                                      *
*   Support for obsolescent functions (include file)                   *
*                                                                      *
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
#ifndef SUPPORT_DEFS
#define SUPPORT_DEFS

/* We need definitions for various objects */
#include <fpa.h>


/* Declare obsolete functions in support_area.c */
void	redefine_area_boundary(AREA, LINE);
void	redefine_area_divide(AREA, int, LINE);


/* Declare obsolete functions in support_curv.c */
LOGICAL	curve_crossing(CURVE curve1, CURVE curve2, POINT cross,
						int *seg1, int *seg2);


/* Declare obsolete functions in support_line.c */
LOGICAL	line_crossing(LINE line1, LINE line2,
						POINT cross, int *seg1, int *seg2);
LOGICAL	next_line_crossing(LINE line1, LINE line2, int start,
						POINT cross, int *span1, int *span2);


/* Declare obsolete functions in support_seg.c */
LOGICAL	next_segment_crossing(SEGMENT seg, LINE line,
						int start_span, POINT cross,
						int *xspan, int *lspan, LOGICAL *right);
LOGICAL	next_seglist_crossing(int nseg, SEGMENT *seglist, LINE line,
						int start_seg, int start_span, POINT cross,
						int *xseg, int *xspan, int *lspan, LOGICAL *right);
LOGICAL	next_closed_seglist_crossing(int nseg, SEGMENT *seglist, LINE line,
						int start_seg, int start_span, POINT cross,
						int *xseg, int *xspan, int *lspan, LOGICAL *right);


/* Declare obsolete functions in support_sfc.c */
float	eval_surface(SURFACE, POINT, float *, float *, LOGICAL *);
LOGICAL	eval_surface_1st_deriv(SURFACE, POINT, float *, float *);
LOGICAL	eval_surface_2nd_deriv(SURFACE, POINT, float *, float *, float *);
LOGICAL	eval_surface_curvature(SURFACE, POINT, float *, POINT);
float	eval_surface_unmapped(SURFACE, POINT, LOGICAL *);
STRING	eval_surface_feature(SURFACE, POINT, STRING, POINT, char *, ITEM *,
						LOGICAL *);

/* Now it has been included */
#endif
