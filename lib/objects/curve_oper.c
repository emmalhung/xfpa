/*********************************************************************/
/**	@file curve_oper.c
 *
 * Assorted operations on curves and sets of curves.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    c u r v e _ o p e r . c                                           *
*                                                                      *
*    Assorted operations on curves and sets of curves.                 *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
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

#include "curve.h"

#include <fpa_math.h>
#include <stdio.h>
#include <string.h>

/***********************************************************************
*                                                                      *
*      c u r v e _ p r o p e r t i e s                                 *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Measure certain 2-D properties of a curve.
 *
 * Properties to measure:
 * 	- open or closed
 * 	- clockwise or counter-clockwise (closed curves only)
 *	- enclosed area (closed curves only)
 *	- length of curve
 *
 *	@param[in]	 	curve		given curve
 *	@param[out] 	*closed		is it closed?
 *	@param[out] 	*clockwise	is it clockwise? (only meaningful if closed)
 *	@param[out] 	*area		enclosed area (only non-zero if closed)
 *	@param[out] 	*length		length of curve
 *********************************************************************/

void	curve_properties

	(
	CURVE	curve,
	LOGICAL	*closed,
	LOGICAL	*clockwise,
	float	*area,
	float	*length
	)

	{
	LINE	line;

	line = (curve)? curve->line: NullLine;
	line_properties(line, closed, clockwise, area, length);
	}

/***********************************************************************
*                                                                      *
*      c u r v e _ t e s t _ p o i n t                                 *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Measure certain 2-D properties of a test point, relative to
 * the given curve.
 *
 * Properties to measure:
 *	- minimum (perpendicular) distance from point to curve
 *	- closest point on curve (interpolated if necessary)
 *	- index of curve segment which contains closest point
 *	- point is inside or outside (closed curves only)
 *	- point is on left or right side of curve
 *
 *	@param[in]	 	curve		given curve
 *	@param[in]	 	ptest		given test point
 *	@param[out] 	*pdist		minimum (perpendicular) distance to curve
 *	@param[out] 	ppoint		closest (perpendicular) point on curve
 *	@param[out] 	*pseg		index of segment which contains closest point
 *	@param[out] 	*inside		is point enclosed by curve?
 *	@param[out] 	*right		is point on right side of curve?
 *********************************************************************/

void	curve_test_point

	(
	CURVE	curve,
	POINT	ptest,
	float	*pdist,
	POINT	ppoint,
	int		*pseg,
	LOGICAL	*inside,
	LOGICAL	*right
	)

	{
	LINE	line;

	line = (curve)? curve->line: NullLine;
	line_test_point(line, ptest, pdist, ppoint, pseg, inside, right);
	}

/***********************************************************************
*                                                                      *
*      c u r v e _ s i g h t                                           *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find closest point on the given curve to a given test point, along
 * the line of sight defined by a second test point.  If no intersection
 * occurs, return the closest approach.
 *
 * Method:
 * 	- Parameterize a line through the test points as:
 * 	@f$ ({x_1},{y_1}) + {t}({x_2}-{x_1},{y_2}-{y_1}) @f$
 * 	- Then parameterize a line through the end points of each span
 * 	  on the curve, as:
 * 	@f$ ({x_a},{y_a}) + {s}({x_b}-{x_a},{y_b}-{y_a}) @f$
 * 	- Then solve for s, such that the two lines cross.
 * 	- We have an intersection only if s is between 0 and 1, meaning
 * 	  that the intersection lies within the span.
 * 	- Return only the closest of these intersections, if any were
 *    found.  A negative distance just means that the closest
 *    intersection was found in the opposite direction from the
 *    second point.
 *
 * @note Make sure to test the return value:
 *
 * 	- A return value of TRUE means that an intersection was found
 * 	  and the returned parameters may be interpretted as:
 * 		- dist     = distance from p1 to intersection,
 * 		- approach = (forced to zero),
 * 		- point    = the actual point of intersection,
 * 		- span     = span containing the intersection,
 * 		- between  = is intersection between p1 and p2?
 * 	- A return value of FALSE means that no intersections were
 * 	  found and the returned parameters should be interpretted as:
 * 		- dist     = distance from p1 to closest approach,
 * 		- approach = shortest distance from line to curve,
 * 		- point    = the actual point of closest approach,
 * 		- span     = point on curve that is closest,
 * 		- between  = closest approach between p1 and p2?
 * 	- If something went wrong, approach will be negative.
 *
 *	@param[in]	 	curve		given curve
 *	@param[in]	 	p1			given test point
 *	@param[in]	 	p2			second point which gives direction of sight
 *	@param[out] 	back		do we want negative distance if closer
 *	@param[out] 	*dist		distance to closest intersection/approach
 *	@param[out] 	*approach	closest approach (0 if intersection)
 *	@param[out] 	point		closest intersection/approach on curve
 *	@param[out] 	*span		index of span which contains closest point
 *	@param[out] 	*between	does curve pass between the test points
 * 	@return True if an intersection was found
 *********************************************************************/

LOGICAL	curve_sight

	(
	CURVE	curve,
	POINT	p1,
	POINT	p2,
	LOGICAL	back,
	float	*dist,
	float	*approach,
	POINT	point,
	int		*span,
	LOGICAL	*between
	)

	{
	LINE	line;

	line = (curve)? curve->line: NullLine;
	return line_sight(line, p1, p2, back, dist, approach, point, span, between);
	}

/***********************************************************************
*                                                                      *
*      c u r v e _ c l o s e s t _ p o i n t                           *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find closest point in the given curve to the given point.
 *
 * @note If you want the closest perpendicular point on the
 *       curve, use curve_test_point().
 *
 *	@param[in]	 	curve	given curve
 *	@param[in]	 	ptest	given test point
 *	@param[out] 	*dist	distance to closest point
 *	@param[out] 	point	closest point
 *  @return Non-negative if closest point found.
 *********************************************************************/
int		curve_closest_point

	(
	CURVE	curve,
	POINT	ptest,
	float	*dist,
	POINT	point
	)

	{
	LINE	line;

	line = (curve)? curve->line: NullLine;
	return line_closest_point(line, ptest, dist, point);
	}

/***********************************************************************
*                                                                      *
*     f i n d _ c u r v e _ c r o s s i n g                            *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find the next point at which two curves cross.  It is assumed that
 * the curves cross any number of times, or not at all.
 *
 *	@param[in] 	curv1	first curve to be examined
 *	@param[in] 	curv2	second curve to be examined
 *	@param[in] 	start	span of curv1 to start at
 *	@param[in] 	spos	point on span of curv1 to start at
 *	@param[out]	cross	point of intersection
 *	@param[out]	*span1	span on curv1 where crossover occurs
 *	@param[out]	*span2	span on curv2 where crossover occurs
 *	@param[out]	*right	now on right side?
 *  @return True if successful.
 *********************************************************************/

LOGICAL	find_curve_crossing

	(
	CURVE	curv1,
	CURVE	curv2,
	int		start,
	POINT	spos,
	POINT	cross,
	int		*span1,
	int		*span2,
	LOGICAL	*right
	)

	{
	/* Set up reasonable return values */
	if (cross) copy_point(cross, ZeroPoint);
	if (span1) *span1 = -1;
	if (span2) *span2 = -1;
	if (right) *right = FALSE;

	/* Make sure things are consistent */
	if (!curv1) return FALSE;
	if (!curv2) return FALSE;

	return find_line_crossing(curv1->line, curv2->line, start, spos,
								cross, span1, span2, right);
	}

/***********************************************************************
*                                                                      *
*     i n b o x _ c u r v e                                            *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Determine if the given curve passes through th given box.
 *
 *	@param[in]	 	curve	given curve
 *	@param[in]	  	*box	box to test against
 * 	@return True if curve passed through box.
 *********************************************************************/

LOGICAL	inbox_curve

	(
	CURVE		curve,
	const BOX	*box
	)

	{
	if (!curve) return FALSE;
	if (!box)   return FALSE;

	return inbox_line(curve->line, box);
	}

/***********************************************************************
*                                                                      *
*      t r a n s l a t e _ c u r v e                                   *
*      r o t a t e _ c u r v e                                         *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Translate the given curve.
 *
 *	@param[in] 	curve	given curve
 *	@param[in] 	dx		x offset
 *	@param[in] 	dy		y offset
 *  @return True if successful.
 *********************************************************************/
LOGICAL	translate_curve

	(
	CURVE	curve,
	float	dx,
	float	dy
	)

	{
	LINE	line;

	line = (curve)? curve->line: NullLine;
	return translate_line(line, dx, dy);
	}

/*********************************************************************/
/** Rotate the given curve.
 *
 *	@param[in] 	curve	given curve
 *	@param[in] 	ref		centre of rotation
 *	@param[in] 	angle	angle of rotation
 *  @return True if successful.
 *********************************************************************/
LOGICAL	rotate_curve

	(
	CURVE	curve,
	POINT	ref,
	float	angle
	)

	{
	LINE	line;

	line = (curve)? curve->line: NullLine;
	return rotate_line(line, ref, angle);
	}
