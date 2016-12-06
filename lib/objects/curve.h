/*********************************************************************/
/** @file curve.h
 *
 * CURVE object definitions (include file)
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    c u r v e . h                                                     *
*                                                                      *
*    CURVE object definitions (include file)                           *
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
#ifndef CURVE_DEFS
#define CURVE_DEFS

/* We need definitions for other objects */
#include "line.h"
#include "pspec.h"
#include "attrib.h"

/* Define CURVE object */
/** a collection of points with various attributes */
typedef struct CURVE_struct
	{
	LINE		line;		/**< point buffer */
	HAND		sense;		/**< curve handedness (matters for patterns etc.) */
	LSPEC		lspec;		/**< how to display the curve */
	ATTRIB_LIST	attrib;		/**< list of attributes */
	/* following will become obsolete */
	STRING		subelem;	/**< curve subelement */
	STRING		value;		/**< curve value */
	STRING		label;		/**< curve label */
	} *CURVE;

/* Convenient definitions */
#define NullCurve        NullPtr(CURVE)
#define NullCurvePtr     NullPtr(CURVE *)
#define NullCurveList    NullPtr(CURVE *)
#define NullCurveListPtr NullPtr(CURVE **)

/** Curve member identifier */
typedef	enum { CurveNone, CurveLine, CurveLabel } CMEMBER;

/* Declare all functions in curve.c */
CURVE	create_curve(STRING subelem, STRING value, STRING label);
CURVE	destroy_curve(CURVE curve);
CURVE	copy_curve(const CURVE curve);
void	define_curve_sense(CURVE curve, HAND sense);
void	recall_curve_sense(CURVE curve, HAND *sense);
void	define_curve_value(CURVE curve,
						STRING subelem, STRING value, STRING label);
void	recall_curve_value(CURVE curve,
						STRING *subelem, STRING *value, STRING *label);
void	define_curve_attribs(CURVE curve, ATTRIB_LIST attribs);
void	recall_curve_attribs(CURVE curve, ATTRIB_LIST *attribs);
void	define_curve_pspec(CURVE curve, PPARAM param, POINTER value);
void	recall_curve_pspec(CURVE curve, PPARAM param, POINTER value);
void	highlight_curve(CURVE curve, HILITE code);
void	widen_curve(CURVE curve, float delta);
void	empty_curve(CURVE curve);
CURVE	append_curve(CURVE curve1, const CURVE curve2);
void	add_line_to_curve(CURVE curve, const LINE line);
void	add_point_to_curve(CURVE curve, POINT p);
void	reverse_curve(CURVE curve);
void	flip_curve(CURVE curve);

/* Declare all functions in curve_oper.c */
void	curve_properties(CURVE curve, LOGICAL *closed, LOGICAL *clockwise,
						float *area, float *length);
void	curve_test_point(CURVE curve, POINT ptest, float *pdist, POINT ppoint,
						int *pseg, LOGICAL *inside, LOGICAL *right);
LOGICAL	curve_sight(CURVE curve, POINT pos1, POINT pos2, LOGICAL back,
						float *dist, float *approach, POINT point,
						int *ispan, LOGICAL *between);
int		curve_closest_point(CURVE curve, POINT ptest, float *dist, POINT point);
LOGICAL	find_curve_crossing(CURVE curv1, CURVE curv2, int start, POINT spos,
						POINT cross, int *span1, int *span2, LOGICAL *right);
LOGICAL	inbox_curve(CURVE curve, const BOX *box);
LOGICAL	translate_curve(CURVE curve, float dx, float dy);
LOGICAL	rotate_curve(CURVE curve, POINT ref, float angle);

/* Functions in curve_set.c are declared in set_oper.h */

/* Now it has been included */
#endif
