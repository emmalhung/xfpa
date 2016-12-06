/*********************************************************************/
/**	@file bound_oper.c
 *
 * Assorted operations on bounds.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    b o u n d _ o p e r . c                                           *
*                                                                      *
*    Assorted operations on bounds.                                    *
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

#include "bound.h"

#include <fpa_math.h>
#include <stdio.h>
#include <string.h>

/***********************************************************************
*                                                                      *
*      b o u n d _ p r o p e r t i e s                                 *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Measure certain 2-D properties of a bound.
 *
 * Properties to measure:
 *	- clockwise or counter-clockwise
 *	- enclosed area (excluding area of holes)
 *	- length of boundary (including hole boundaries)
 *
 * @note If you want the area or length of the boundary only, use
 * line_properties() with bound->boundary.
 *
 *	@param[in] 	bound		given bound
 *	@param[out]	*clockwise	is it clockwise?
 *	@param[out]	*area		enclosed area
 *	@param[out]	*length		length of boundary
 *********************************************************************/
void	bound_properties

	(
	BOUND	bound,
	LOGICAL	*clockwise,
	float	*area,
	float	*length
	)

	{
	float	ahole, lhole;
	float	*aptr, *lptr;
	int		i;

	/* Initialize return parameters */
	if (clockwise) *clockwise = FALSE;
	if (area)      *area      = 0;
	if (length)    *length    = 0;
	if (!bound) return;

	/* Get properties of boundary first */
	line_properties(bound->boundary, NULL, clockwise, area, length);

	/* Subtract area of any holes if we want area */
	/* and add length of any holes if we want length */
	if (!area && !length) return;
	aptr = (area)?   &ahole: NULL;
	lptr = (length)? &lhole: NULL;
	for (i=0; i<bound->numhole; i++)
		{
		line_properties(bound->holes[i], NULL, NULL, aptr, lptr);
		if (aptr) *area   -= ahole;
		if (lptr) *length += lhole;
		}
	}

/***********************************************************************
*                                                                      *
*      b o u n d _ t e s t _ p o i n t                                 *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Measure certain 2-D properties of a test point, relative to     *
 * the given bound.
 *
 * Properties to measure:
 * 	- minimum (perpendicular) distance from point to boundary
 *    or hole edge
 * 	- closest point on boundary or hole edge (interpolated if
 *    necessary)
 * 	- index of boundary (-1) or hole (0...N-1) which contains
 *    closest point
 * 	- index of span in boundary or hole which contains closest
 *    point
 * 	- point is inside or outside (in hole = outside)
 *
 *	@param[in] 	bound	given bound
 *	@param[in] 	ptest	given test point
 *	@param[out]	*pdist	minimum (perpendicular) distance to line
 *	@param[out]	ppoint	closest (perpendicular) point on line
 *	@param[out]	*phole	index of hole which contains closest point
 *	@param[out]	*pspan	index of span which contains closest point
 *	@param[out]	*inside	is point enclosed by bound?
 *********************************************************************/
void	bound_test_point

	(
	BOUND	bound,
	POINT	ptest,
	float	*pdist,
	POINT	ppoint,
	int		*phole,
	int		*pspan,
	LOGICAL	*inside
	)

	{
	int		i;
	float	bdist, hdist;
	POINT	bpoint;
	int		bhole, bspan;
	LOGICAL	in;

	/* Initialize return parameters */
	if (ppoint) copy_point(ppoint, ZeroPoint);
	if (pdist)  *pdist  = -1;
	if (phole)  *phole  = -1;
	if (pspan)  *pspan  = -1;
	if (inside) *inside = FALSE;
	if (!bound || !ptest) return;

	/* Test w.r.t. boundary first */
	bhole = -1;
	line_test_point(bound->boundary, ptest, &bdist, bpoint, &bspan, &in, NULL);
	if (ppoint) copy_point(ppoint, bpoint);
	if (pdist)  *pdist  = bdist;
	if (phole)  *phole  = bhole;
	if (pspan)  *pspan  = bspan;
	if (inside) *inside = in;

	/* Test w.r.t. each hole */
	for (i=0; i<bound->numhole; i++)
		{
		bhole = i;
		line_test_point(bound->holes[i], ptest, &hdist, bpoint, &bspan, &in,
				NULL);
		if (hdist < bdist)
			{
			bdist = hdist;
			if (ppoint) copy_point(ppoint, bpoint);
			if (pdist)  *pdist  = bdist;
			if (phole)  *phole  = bhole;
			if (pspan)  *pspan  = bspan;
			}
		if (in && inside)
			{
			*inside = FALSE;
			}
		}
	}

/***********************************************************************
*                                                                      *
*      b o u n d _ s i g h t                                           *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find closest point on the given bound to a given test point,
 * along the line of sight defined by a second test point.  If no
 * intersection occurs, return the closest approach.
 *
 * Method:
 * 	- Parameterize a line through the test points as:
 * @f$ ({x_1},{y_1}) + {t}({x_2}-{x_1},{y_2}-{y_1}) @f$
 * 	- Then parameterize a line through the end points of each span
 * 	  on the curve, as:
 * @f$ ({x_a},{y_a}) + {s}({x_b}-{x_a},{y_b}-{y_a}) @f$
 * 	- Then solve for s, such that the two lines cross.
 * 	- We have an intersection only if s is between 0 and 1, meaning
 * 	  that the intersection lies within the span.
 * 	- Return only the closest of these intersections, if any were
 * 	  found.  A negative distance just means that the closest
 * 	  intersection was found in the opposite direction from the
 * 	  second point.
 *
 * @note Make sure to test the return value:
 *
 * 	- A return value of TRUE means that an intersection was found
 * 	  and the returned parameters may be interpretted as:
 * 		- dist     = distance from p1 to intersection,
 * 		- approach = (forced to zero),
 * 		- point    = the actual point of intersection on the target
 * 					 bound,
 * 		- hole     = boundary (-1) or hole (1...N-1) containing
 * 		             the intersection,
 * 		- span     = span on boundary or hole containing the
 * 					 intersection,
 * 		- between  = is intersection between p1 and p2?
 * 	- A return value of FALSE means that no intersections were found
 * 	  and the returned parameters should be interpretted as:
 * 		- dist     = distance from p1 to closest approach along sight
 * 					 line,
 * 		- approach = shortest distance from sight line to the target
 * 			    	 bound,
 * 		- point    = the actual point of closest approach, on the target
 * 					 bound,
 * 		- hole     = boundary (-1) or hole (1...N-1) that is closest
 * 				     (should never be a hole),
 * 		- span     = span on boundary or hole that is closest,
 * 		- between  = is closest approach between p1 and p2?
 * 	- If something went wrong, approach will be negative.
 *
 *	@param[in] 	bound		given bound
 *	@param[in] 	p1			given test point
 *	@param[in] 	p2			second point which gives direction of sight
 *	@param[in] 	back		do we want negative distance if closer
 *	@param[out]	*dist		distance to closest intersection/approach
 *	@param[out]	*approach	closest approach (0 if intersection)
 *	@param[out]	point		closest intersection/approach on bound
 *	@param[out]	*hole		index of hole which contains closest point
 *	@param[out]	*span		index of span which contains closest point
 *	@param[out]	*between	is intersection/approach between the test points
 * 	@return True if there is an intersection.
 *********************************************************************/

LOGICAL	bound_sight

	(
	BOUND	bound,
	POINT	p1,
	POINT	p2,
	LOGICAL	back,
	float	*dist,
	float	*approach,
	POINT	point,
	int		*hole,
	int		*span,
	LOGICAL	*between
	)

	{
	int		i;
	float	bdist, hdist, bappr;
	POINT	bpoint;
	int		bhole, bspan;
	LOGICAL	hfound, found, tween;

	/* Initialize return parameters */
	if (point) copy_point(point, ZeroPoint);
	if (dist)     *dist     = -1;
	if (approach) *approach = -1;
	if (hole)     *hole     = -1;
	if (span)     *span     = -1;
	if (between)  *between  = FALSE;
	if (!bound || !p1 || !p2) return FALSE;

	/* Test w.r.t. boundary first */
	bhole = -1;
	found = line_sight(bound->boundary, p1, p2, back, &bdist, &bappr, bpoint,
					   &bspan, &tween);
	if (point)    copy_point(point, bpoint);
	if (dist)     *dist     = bdist;
	if (approach) *approach = bappr;
	if (hole)     *hole     = bhole;
	if (span)     *span     = bspan;
	if (between)  *between  = tween;

	/* Test w.r.t. each hole, unless it doesn't even intersect the boundary */
	if (!found) return FALSE;
	for (i=0; i<bound->numhole; i++)
		{
		bhole  = i;
		hfound = line_sight(bound->holes[i], p1, p2, back, &hdist, &bappr,
							bpoint, &bspan, &tween);

		/* May encounter closer approaches, but want best intersection */
		if (hfound && hdist < bdist)
			{
			bdist = hdist;
			if (point)    copy_point(point, bpoint);
			if (dist)     *dist     = bdist;
			if (approach) *approach = bappr;
			if (hole)     *hole     = bhole;
			if (span)     *span     = bspan;
			if (between)  *between  = tween;
			}
		}
	return found;
	}

/***********************************************************************
*                                                                      *
*      b o u n d _ c l o s e s t _ p o i n t                           *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find closest point in the given bound to the given point.
 *
 * @note If you want the closest perpendicular point on the  bound,
 *       use bound_test_point().
 *
 * 	@param[in] 	bound	given bound
 * 	@param[in] 	ptest	given test point
 * 	@param[out]	*dist	distance to closest point
 * 	@param[out]	point	closest point
 * 	@return
 *********************************************************************/
int	bound_closest_point

	(
	BOUND	bound,
	POINT	ptest,
	float	*dist,
	POINT	point
	)

	{
	int		i, ip, ipmin;
	float	bdist, hdist;
	POINT	bpoint;

	/* Initialize return parameters */
	if (point) copy_point(point, ZeroPoint);
	if (dist)  *dist = -1;
	if (!bound || !ptest) return -1;

	/* Test w.r.t. boundary first */
	ipmin = line_closest_point(bound->boundary, ptest, &bdist, bpoint);
	if (point) copy_point(point, bpoint);
	if (dist)  *dist = bdist;

	/* Test w.r.t. each hole */
	for (i=0; i<bound->numhole; i++)
		{
		ip = line_closest_point(bound->holes[i], ptest, &hdist, bpoint);
		if (ip >= 0 && hdist < bdist)
			{
			ipmin = ip;
			bdist = hdist;
			if (point) copy_point(point, bpoint);
			if (dist)  *dist = bdist;
			}
		}
	return ipmin;
	}

/***********************************************************************
*                                                                      *
*      b o u n d _ c l o s e s t _ h o l e                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find closest hole in the given bound to the given point.
 *
 *	@param[in] 		bound	given bound
 *	@param[in]	 	ptest	given test point
 *	@param[out] 	*dist	distance to closest point
 *	@param[out] 	point	closest point
 *  @return Pointer to the boundary line closest to the given hole.
 *********************************************************************/
LINE	bound_closest_hole

	(
	BOUND	bound,
	POINT	ptest,
	float	*dist,
	POINT	point
	)

	{
	LINE	bhole;
	int		i, ip;
	float	bdist, hdist;
	POINT	bpoint;

	/* Initialize return parameters */
	if (point) copy_point(point, ZeroPoint);
	if (dist)  *dist = 0.;
	if (!bound || !ptest) return NullLine;

	/* Test w.r.t. each hole */
	bhole = NullLine;
	bdist = -1;
	for (i=0; i<bound->numhole; i++)
		{
		ip = line_closest_point(bound->holes[i], ptest, &hdist, bpoint);
		if (ip >= 0 && (bdist < 0 || hdist < bdist))
			{
			bhole = bound->holes[i];
			bdist = hdist;
			if (point) copy_point(point, bpoint);
			if (dist)  *dist = bdist;
			}
		}
	return bhole;
	}

/***********************************************************************
*                                                                      *
*     i n b o x _ b o u n d                                            *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Determine if the given bound passes through the given box.
 *
 *	@param[in] 	bound	given bound
 *	@param[in] 	*box	box to test against
 * @return True if the bound passes through the box.
 *********************************************************************/
LOGICAL	inbox_bound

	(
	BOUND		bound,
	const BOX	*box
	)

	{
	int		i;
	POINT	pos;
	LOGICAL	inside, allin, in;

	if (!bound) return FALSE;
	if (!box)   return FALSE;

	/* First see if the boundary or any holes pass through the box */
	if (inbox_line(bound->boundary, box)) return TRUE;
	for (i=0; i<bound->numhole; i++)
		{
		if (inbox_line(bound->holes[i], box)) return TRUE;
		}

	/* Next see if the box is partially inside the area */
	set_point(pos, box->left, box->bottom);
	line_test_point(bound->boundary, pos, NULL, NULL, NULL, &in, NULL);
	inside = in;
	allin  = in;

	set_point(pos, box->left, box->top);
	line_test_point(bound->boundary, pos, NULL, NULL, NULL, &in, NULL);
	inside |= in;
	allin  &= in;

	set_point(pos, box->right, box->bottom);
	line_test_point(bound->boundary, pos, NULL, NULL, NULL, &in, NULL);
	inside |= in;
	allin  &= in;

	set_point(pos, box->right, box->top);
	line_test_point(bound->boundary, pos, NULL, NULL, NULL, &in, NULL);
	inside |= in;
	allin  &= in;

	/* Next make sure the box is not totally inside a hole */
	if (!inside) return FALSE;
	if (!allin)  return TRUE;
	for (i=0; i<bound->numhole; i++)
		{
		set_point(pos, box->left, box->bottom);
		line_test_point(bound->holes[i], pos, NULL, NULL, NULL, &in, NULL);
		if (!in) continue;

		set_point(pos, box->left, box->top);
		line_test_point(bound->holes[i], pos, NULL, NULL, NULL, &in, NULL);
		if (!in) continue;

		set_point(pos, box->right, box->bottom);
		line_test_point(bound->holes[i], pos, NULL, NULL, NULL, &in, NULL);
		if (!in) continue;

		set_point(pos, box->right, box->top);
		line_test_point(bound->holes[i], pos, NULL, NULL, NULL, &in, NULL);
		if (!in) continue;

		return FALSE;
		}
	return TRUE;
	}
