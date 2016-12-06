/*********************************************************************/
/** @file bound.h
 *
 * BOUND object definitions (include file)
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    b o u n d . h                                                     *
*                                                                      *
*    BOUND object definitions (include file)                           *
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
#ifndef BOUND_DEFS
#define BOUND_DEFS

/* We need definitions for other objects */
#include "line.h"


/* Define BOUND object */
/** defines the outer boundary and hole edges of a single given area */
typedef struct BOUND_struct
	{
	LINE	boundary;	/**< closed line representing the boundary */
	int		numhole;	/**< number of holes */
	LINE	*holes;		/**< list of holes */
	} *BOUND;


/* Convenient definitions */
#define NullBound        NullPtr(BOUND)
#define NullBoundPtr     NullPtr(BOUND *)
#define NullBoundList    NullPtr(BOUND *)
#define NullBoundListPtr NullPtr(BOUND **)

/* Declare all functions in bound.c */
BOUND	create_bound(void);
BOUND	destroy_bound(BOUND bound);
void	empty_bound(BOUND bound);
BOUND	copy_bound(const BOUND bound);
void	define_bound(BOUND bound,
						LINE boundary, int numhole, const LINE *holes);
void	define_bound_boundary(BOUND bound, LINE boundary);
void	define_bound_holes(BOUND bound, int numhole, const LINE *holes);
void	add_bound_hole(BOUND bound, LINE hole);
void	remove_bound_hole(BOUND bound, LINE hole);
int		which_bound_hole(BOUND bound, LINE hole);

/* Declare all functions in bound_oper.c */
void	bound_properties(BOUND bound,
						LOGICAL *clockwise, float *area, float *length);
void	bound_test_point(BOUND bound, POINT ptest,
						float *pdist, POINT ppoint,
						int *phole, int *pspan, LOGICAL *inside);
LOGICAL	bound_sight(BOUND bound, POINT pos1, POINT pos2, LOGICAL back,
						float *dist, float *approach, POINT point,
						int *hole, int *ispan, LOGICAL *between);
int		bound_closest_point(BOUND bound, POINT ptest, float *dist, POINT point);
LINE	bound_closest_hole(BOUND bound, POINT ptest, float *dist, POINT point);
LOGICAL	inbox_bound(BOUND bound, const BOX *box);

/* Now it has been included */
#endif
