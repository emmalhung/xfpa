/*********************************************************************/
/** @file line.h
 *
 * LINE object definitions (include file)
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    l i n e . h                                                       *
*                                                                      *
*    LINE object definitions (include file)                            *
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
#ifndef LINE_DEFS
#define LINE_DEFS

/* We need definitions for other objects */
#include "projection.h"

/* Parameters */
#define DELTA_POINTS 1

/* Set parameters for checking crossings        */
/*  ... minimum distance between line crossings */
/*  ... minimum distance between hole crossings */
/*  ... closest distance to end of a span       */
#define SegTol  0.15
#define HoleTol 0.03
#define EndTol  0.03

/* Define LINE object */
/** an ordered list of points */
typedef struct LINE_struct
	{
	POINT	*points;	/**< head of point buffer */
	int		numpts;		/**< number of points */
	int		maxpts;		/**< max allocated points so far */
	} *LINE;

/* Convenient definitions */
#define NullLine        NullPtr(LINE)
#define NullLinePtr     NullPtr(LINE *)
#define NullLineList    NullPtr(LINE *)
#define NullLineListPtr NullPtr(LINE **)

/* Declare all functions in line.c */
LINE	create_line(void);
LINE	destroy_line(LINE line);
void	empty_line(LINE line);
LINE	copy_line(const LINE line);
LINE	append_line(LINE line1, const LINE line2);
LINE	append_line_dir(LINE line1, const LINE line2, LOGICAL fwd);
LINE	append_line_portion(LINE line1, const LINE line2,
						int start, int end);
LINE	append_line_pdir(LINE line1, const LINE line2,
						int start, int end, LOGICAL fwd);
void	condense_line(LINE line);
void	reverse_line(LINE line);
void	get_line_points(LINE line, POINT **points, int *numpts, int *maxpts);
void	get_line_plist(LINE line, POINT *pbuf, int *numpts, int *bufpts);
void	save_line_plist(LINE line, POINT *points, int numpts);
void	add_point_to_line(LINE line, POINT p);

/* Declare all functions in line_oper.c */
LOGICAL	line_closed(LINE line);
void	close_line(LINE line);
void	line_properties(LINE line, LOGICAL *closed, LOGICAL *clockwise,
						float *area, float *length);
void	line_test_point(LINE line, POINT ptest,
						float *pdist, POINT ppoint, int *pseg,
						LOGICAL * inside, LOGICAL * right);
LOGICAL	line_sight(LINE line, POINT po, POINT pv, LOGICAL back,
						float *dist, float *approach, POINT point,
						int *ispan, LOGICAL *between);
int		line_closest_point(LINE line, POINT ptest, float *dist, POINT point);
int		line_intersect(LINE line, LINE divl, int *ip1, int *ip2);
LOGICAL	find_line_crossing(LINE line1, LINE line2, int start, POINT spos,
						POINT cross, int *span1, int *span2, LOGICAL *right);
LOGICAL	looped_line_crossing(LINE line, POINT cross, int *seg1, int *seg2);
LOGICAL	line_approach(LINE line, LINE tline, float *dist, float *approach,
						POINT point, int *span, int *tspan);
LOGICAL	inbox_line(LINE line, const BOX *box);
LOGICAL	contiguous_line(LINE line, POINT ptest, float maxang);
int		contiguous_line_segments(LINE line, POINT ptest, float maxang,
						LINE **segments);
LOGICAL	scale_line(LINE line, float sx, float sy);
LOGICAL	translate_line(LINE line, float dx, float dy);
LOGICAL	rotate_line(LINE line, POINT ref, float angle);
LOGICAL	line_too_short(LINE line, float tol);
float	line_index(LINE line, int ispan, float dspan);
float	*line_pos(LINE line, float idx, int *ispan, float *dspan);
float	line_span_info(LINE line, int ispan, POINT spos, POINT epos, POINT dp);
float	line_slen(LINE line, float idx1, float idx2);
float	line_walk(LINE line, float idx, float dist);
float	great_circle_line_length(const MAP_PROJ *mproj, LINE line);
LOGICAL	matching_lines(LINE line1, LINE line2);

/* Now it has been included */
#endif
