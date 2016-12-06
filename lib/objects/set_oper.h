/*********************************************************************/
/** @file set_oper.h
 *
 * definitions for functions that operate on sets of items.
 * (include file)
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    s e t _ o p e r . h                                               *
*                                                                      *
*    defintitions for functions that operate on sets of items.         *
*    (include file)                                                    *
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
#ifndef SET_OPER_DEFS
#define SET_OPER_DEFS

/* Include things that can belong to a set */
#include "set.h"

/** Define modes for picking/sorting enclosing areas and curves */
typedef	enum	{
  PickFirst,	/**< Pick the first match */
  PickLast,		/**< Pick the last match */
  PickSmallest,		/**< Pick the smallest match */
  PickLargest		/**< Pick the largest match */
} PICK;

/* Declare functions in lchain_set.c */
LCHAIN	closest_lchain(SET set, POINT ptest,
						float *dist, POINT point, int *iseg);
LCHAIN	closest_lchain_node(SET set, POINT ptest,
						float *dist, POINT point, LMEMBER *itype, int *inode);

/* Declare functions in curve_set.c */
CURVE	closest_curve(SET set, POINT ptest,
						float *dist, POINT point, int *iseg);
CURVE	enclosing_curve(SET set, POINT ptest, PICK mode,
						float *size, LOGICAL *clockwise);
int		sort_enclosing_curves(SET set, POINT ptest, PICK mode, CURVE **curves);
LOGICAL	reorder_curves(SET set, PICK mode);

/* Declare functions in area_set.c */
LOGICAL	eval_areaset(SET set, POINT pos, PICK mode,
						SUBAREA *sub, ATTRIB_LIST *attribs);
int		eval_areaset_list(SET set, POINT pos, PICK mode,
						SUBAREA **slist, ATTRIB_LIST **attlist);
AREA	closest_area(SET set, POINT ptest, float *dist, POINT point,
						AMEMBER *mtype, int *mem, int *ispan);
AREA	enclosing_area(SET set, POINT ptest, PICK mode,
						float *size, LOGICAL *clockwise);
int		enclosing_area_list(SET set, POINT ptest, PICK mode, AREA **alist);
SUBAREA	closest_subarea(SET set, POINT ptest, float *dist, POINT point,
						int *iseg, int *ispan, AREA *parent);
SUBAREA	enclosing_subarea(SET set, POINT ptest, PICK mode,
						float *size, LOGICAL *clockwise, AREA *parent);
int		enclosing_subarea_list(SET set, POINT ptest, PICK mode,
						SUBAREA **slist);
LOGICAL	reorder_areas(SET set, PICK mode);

/* Declare functions in label_set.c */
LABEL	closest_label(SET set, POINT ptest, STRING category,
						float *dist, POINT point);
int		label_count(SET set, STRING category);
float	label_distance(LABEL, POINT ptest, POINT point);

/* Declare functions in mark_set.c */
MARK	closest_mark(SET set, POINT ptest, STRING category,
						float *dist, POINT point);
int		mark_count(SET set, STRING category);
float	mark_distance(MARK, POINT ptest, POINT point);

/* Declare functions in barb_set.c */
BARB	closest_barb(SET set, POINT ptest, STRING category,
						float *dist, POINT point);
int		barb_count(SET set, STRING category);
float	barb_distance(BARB, POINT ptest, POINT point);

/* Declare functions in spot_set.c */
SPOT	closest_spot(SET set, POINT ptest, STRING class, STRING category,
						float *dist, POINT point);
int		spot_count(SET set, STRING class, STRING category);
float	spot_distance(SPOT set, POINT ptest, POINT point);
void	prepare_spot_set(SET set);

/* Declare functions in button_set.c */
BUTTON	pick_set_button(SET set, POINT ptest);

/* Now it has been included */
#endif
