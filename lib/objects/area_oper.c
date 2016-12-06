/***********************************************************************/
/**	@file	area_oper.c
 *
 * Assorted operations on Areas.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 ***********************************************************************/
/***********************************************************************
*                                                                      *
*    a r e a _ o p e r . c                                             *
*                                                                      *
*    Assorted operations on AREAs.                                     *
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

#include "area.h"

#include <tools/tools.h>
#include <fpa_getmem.h>

#include <stdio.h>
#include <string.h>

#undef DEBUG_CLIP
#undef DEBUG_DIVIDE
#undef DEBUG_PROPERTIES
#undef DEBUG_TESTPOINT
#undef DEBUG_SUBIDS
#undef DEBUG_ADJACENT
#undef DEBUG_ORIENTATION

/* Define internal constant ratio for near edge criteria */
#define	NearEdgeRatio 0.20

/* Internal static functions */
static	DIVSTAT	area_wrt_line(LINE, AREA);
static	DIVSTAT	subarea_wrt_line(LINE, SUBAREA);
static	LOGICAL	subarea_orientation(SUBAREA, LOGICAL *);
static	void	remove_area_subid(int, int *, int *, int, int);

/***********************************************************************
*                                                                      *
*      Routines specific to a given AREA                               *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*      a r e a _ p r o p e r t i e s                                   *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Measure certain 2-D properties of a area:
 *
 *        - clockwise or counter-clockwise
 *        - enclosed area (excluding area of holes)
 *        - length of boundary (including hole boundaries)
 *
 *      @note If you want the area or length of the boundary only, use
 *            line_properties() with area->bound->boundary.
 *
 * @param[in]	area		area to measure.
 * @param[out]	*clockwise	is it clockwise?
 * @param[out]	*size		size of enclosed area.
 * @param[out]	*length		length of boundary.
 ***********************************************************************/
void	area_properties

	(
	AREA	area,		/* given area */
	LOGICAL	*clockwise,	/* is it clockwise? */
	float	*size,		/* enclosed area */
	float	*length		/* length of boundary */
	)

	{
	if (clockwise) *clockwise = FALSE;
	if (size)      *size      = 0;
	if (length)    *length    = 0;
	if (!area) return;

	/* Get properties of main boundary with holes */
	bound_properties(area->bound, clockwise, size, length);
	}

/***********************************************************************
*                                                                      *
*      a r e a _ t e s t _ p o i n t                                   *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Measure certain 2-D properties of a test point, relative to
 * the given area:
 *
 *        - minimum (perpendicular) distance from point to boundary
 *          or hole edge
 *        - closest point on boundary or hole edge (interpolated if
 *          necessary)
 *        - type of feature (boundary/hole/divide) which contains
 *          closest point
 *        - index of boundary/hole/divide containing closest point
 *        - index of span in boundary/hole/divide containing closest
 *          point
 *        - point is inside or outside (in hole = outside)
 *
 * @param[in]	area	area to test.
 * @param[in]	ptest	point to test.
 * @param[out]	*pdist	minimum (perpendicular) distance to line.
 * @param[out]	ppoint	closest (perpendicular) point on line.
 * @param[out]	*mtype	boundary, hole or divide?
 * @param[out]	*imem	which boundary, hole or divide? (index in list).
 * @param[out]	*span	index of span which contains closest point.
 * @param[out]	inside	is point enclosed by area?
 ***********************************************************************/
void	area_test_point

	(
	AREA	area,	/* given area */
	POINT	ptest,	/* given test point */
	float	*pdist,	/* minimum (perpendicular) distance to line */
	POINT	ppoint,	/* closest (perpendicular) point on line */
	AMEMBER	*mtype,	/* boundary, hole or divide? */
	int		*imem,	/* which boundary, hole or divide? */
	int		*span,	/* index of span which contains closest point */
	LOGICAL	*inside	/* is point enclosed by area? */
	)

	{
	int		Hole, Span;

	/* Initialize return parameters */
	if (ppoint) copy_point(ppoint, ZeroPoint);
	if (pdist)  *pdist  = -1;
	if (mtype)  *mtype  = AreaNone;
	if (imem)   *imem   = -1;
	if (span)   *span   = -1;
	if (inside) *inside = FALSE;
	if (!area || !ptest) return;

	/* Test w.r.t. main boundary with holes */
	bound_test_point(area->bound, ptest, pdist, ppoint, &Hole, &Span, inside);
	if (Span < 0)
		{
		if (mtype) *mtype = AreaNone;
		if (imem)  *imem  = -1;
		if (span)  *span  = -1;
		}
	else if (Hole < 0)
		{
		if (mtype) *mtype = AreaBound;
		if (imem)  *imem  = 0;
		if (span)  *span  = Span;
		}
	else
		{
		if (mtype) *mtype = AreaHole;
		if (imem)  *imem  = Hole;
		if (span)  *span  = Span;
		}

	/* Should improve to test w.r.t. dividing lines */
	}

/***********************************************************************
*                                                                      *
*      a r e a _ s i g h t                                             *
*                                                                      *
************************************************************************/
/***********************************************************************/
/**	Find closest point on the given area to a given test point,
*      along the line of sight defined by a second test point.  If no
*      intersection occurs, return the closest approach.
*
*      Method:
*	- Parameterize a line through the test points as:
*		@f$ ({x_1}, {y_1}) + {t}({x_2}-{x_1}, {y_2}-{y_1}) @f$
*
*	- Then parameterize a line through the end points of each span on the curve, as:
*		@f$ ({x_a}, {y_a}) + {s}({x_b}-{x_a}, {y_b}-{y_a}) @f$
*
*	- Then solve for s, such that the two lines cross.
*
*	- We have an intersection only if s is between 0 and
*		1, meaning that the intersection lies within the span.
*
*	- Return only the closest of these intersections, if
*		any were found.  A negative distance just means that
*		the closest intersection was found in the opposite
*		direction from the second point.
*
*      @note Make sure to test the return value:
*
*	- A return value of TRUE means that an intersection was
*		found and the returned parameters may be interpretted as:
*		- dist     = distance from p1 to intersection,
*		- approach = (forced to zero),
*		- point    = the actual point of intersection on the target area,
*		- mtype    = boundary, hole or divide,
*		- imem     = index of boundary, hole or divide containing the intersection,
*		- span     = span on boundary, hole or divide containing the intersection,
*		- between  = is intersection between p1 and p2?
*
*	- A return value of FALSE means that no intersections
*		were found and the returned parameters should be interpretted as:
*		- dist     = distance from p1 to closest approach along sight line,
*		- approach = shortest distance from sight line to the target area,
*		- point    = the actual point of closest approach, on the target area,
*		- mtype    = boundary, hole or divide (will never be hole or divide),
*		- imem     = index of boundary, hole or divide that is closest,
*		- span     = span on boundary, hole or divide that is closest,
* 		- between  = is closest approach between p1 and p2?
*
*	- If something went wrong, approach will be negative.
*	@param[in]	area	area to test.
*	@param[in]	p1		first test point.
*	@param[in]	p2		second test point gives direction of sight.
*	@param[in]	back	do we want negative distance if closer?
*	@param[out]	*dist	distance to closest intersection/approach.
*	@param[out]	*approach	closest approach ( O if intersection).
*	@param[out]	point	closest intersection/approach on area.
*	@param[out]	*mtype	boundary, hole or divide?
*	@param[out] *imem	which boundary, hole or divide? (index)
*	@param[out]	*span	index of span which contains closest point.
*	@param[out]	*between	ins intersection/approach between the test points?
*	@return True if successful.
***********************************************************************/

LOGICAL	area_sight

	(
	AREA	area,		/* given area */
	POINT	p1,			/* given test point */
	POINT	p2,			/* second point which gives direction of sight */
	LOGICAL	back,		/* do we want negative distance if closer */
	float	*dist,		/* distance to closest intersection/approach */
	float	*approach,	/* closest approach (0 if intersection) */
	POINT	point,		/* closest intersection/approach on area */
	AMEMBER	*mtype,		/* boundary, hole or divide? */
	int		*imem,		/* which boundary, hole or divide? */
	int		*span,		/* index of span which contains closest point */
	LOGICAL	*between	/* is intersection/approach between the test points */
	)

	{
	int		Hole, Span;
	LOGICAL	Sight;

	/* Initialize return parameters */
	if (point) copy_point(point, ZeroPoint);
	if (dist)     *dist     = -1;
	if (approach) *approach = -1;
	if (mtype)    *mtype    = AreaNone;
	if (imem)     *imem     = -1;
	if (span)     *span     = -1;
	if (between)  *between  = FALSE;
	if (!area || !p1 || !p2) return FALSE;

	/* Test w.r.t. main boundary with holes */
	Sight = bound_sight(area->bound, p1, p2, back, dist, approach, point, &Hole,
						&Span, between);
	if (Span < 0)
		{
		if (mtype) *mtype = AreaNone;
		if (imem)  *imem  = -1;
		if (span)  *span  = -1;
		}
	else if (Hole < 0)
		{
		if (mtype) *mtype = AreaBound;
		if (imem)  *imem  = 0;
		if (span)  *span  = Span;
		}
	else
		{
		if (mtype) *mtype = AreaHole;
		if (imem)  *imem  = Hole;
		if (span)  *span  = Span;
		}

	/* Should improve to test w.r.t. dividing lines */

	return Sight;
	}

/***********************************************************************
*                                                                      *
*      a r e a _ c l o s e s t _ f e a t u r e                         *
************************************************************************/
/***********************************************************************/
/** Find closest line (boundary, hole or dividing line) in the given
 * area to the given point.
 *
 *	The type of feature is provided as the return value.
 *
 *	If the return value is AreaNone (0), then nothing was found.
 *	In this case imem and span are both irrelevant and are both
 *	set to -1.
 *
 *	If the return value is AreaBound (1), then the boundary
 *	contains the closest point.  In this case imem is irrelevant
 *	and is set to 0.
 *
 *	If the return value is AreaHole (2), then a hole contains the
 *	closest point.  In this case imem indicates which hole.
 *
 *	If the return value is AreaDiv (3), then a dividing line
 *	contains the closest point.  In this case imem indicates
 *	which dividing line.
 *
 *	If the given point is outside, or the area is not divided and
 *	has no holes, then the area boundary is automatically found.
 *
 * @param[in]	area	area to test.
 * @param[in]	ptest	point to test.
 * @param[out]	*pdist	minimum (perpendicular) distance to line.
 * @param[out]	ppoint	closest (perpendicular) point on line.
 * @param[out]	*imem	which boundary, hole or dividing line contains
 * 						the closest point.
 * @param[out]	*span	span of boundary, hole or dividing line with
 * 						closest point.
 * @return type of feature found.
 **********************************************************************/

AMEMBER	area_closest_feature

	(
	AREA	area,	/* given area */
	POINT	ptest,	/* given test point */
	float	*pdist,	/* minimum (perpendicular) distance to line */
	POINT	ppoint,	/* closest (perpendicular) point on line */
	int		*imem,	/* which boundary/hole/divide contains closest point */
	int		*span	/* span of boundary/hole/divide with closest point */
	)

	{
	int		id;
	LINE	divl;

	float	ddist, bdist=-1;
	POINT	bpoint;
	LOGICAL	inside, inbnd;
	AMEMBER	MemTyp;
	int		Member, Span;

	/* Initialize return parameters */
	if (ppoint) copy_point(ppoint, ZeroPoint);
	if (pdist)  *pdist  = -1;
	if (imem)   *imem   = -1;
	if (span)   *span   = -1;
	if (!area || !ptest) return AreaNone;

	/* Let's see if the point is inside */
	area_test_point(area, ptest, &bdist, bpoint,
					&MemTyp, &Member, &Span, &inside);
	if (pdist)  *pdist = bdist;
	if (ppoint) copy_point(ppoint, bpoint);
	if (imem)   *imem  = Member;
	if (span)   *span  = Span;

	/* No divides? */
	if (area->numdiv <= 0) return MemTyp;

	/* Not inside (and not inside boundary either)? */
	if (!inside)
		{

		/* Cannot be inside boundary if no holes */
		if (!area->bound) return MemTyp;
		if (area->bound->numhole <= 0) return MemTyp;

		/* Check if inside boundary (inside a hole) */
		line_test_point(area->bound->boundary, ptest, NullFloat, NullPoint,
						NullInt, &inbnd, NullChar);
		if (!inbnd) return MemTyp;
		}

	/* Now let's check each dividing line */
	for (id=0; id<area->numdiv; id++)
		{
		divl = area->divlines[id];
		line_test_point(divl, ptest, &ddist, bpoint, &Span, NullChar, NullChar);
		if (ddist < bdist)
			{
			bdist  = ddist;
			MemTyp = AreaDiv;
			Member = id;
			if (pdist)  *pdist = bdist;
			if (ppoint) copy_point(ppoint, bpoint);
			if (imem)   *imem  = Member;
			if (span)   *span  = Span;
			}
		}

	return MemTyp;
	}

/***********************************************************************
*                                                                      *
*      a r e a _ c l o s e s t _ p o i n t                             *
*                                                                      *
***********************************************************************/
/***********************************************************************/
/**	Find closest point in the given area to the given point.
 *
 *	@note If you want the closest perpendicular point on the area,
 *		  use area_test_point().
 *
 * @param[in] 	area	area to test against.
 * @param[in]	ptest	test point.
 * @param[out]	*dist	distance to closest point.
 * @param[out]	point	closest point.
 * @return	index of closest point.
 ***********************************************************************/

int		area_closest_point

	(
	AREA	area,	/* given area */
	POINT	ptest,	/* given test point */
	float	*dist,	/* distance to closest point */
	POINT	point	/* closest point */
	)

	{
	/* Initialize return parameters */
	if (point) copy_point(point, ZeroPoint);
	if (dist)  *dist = -1;
	if (!area || !ptest) return -1;

	/* Test w.r.t. main boundary with holes */
	return bound_closest_point(area->bound, ptest, dist, point);

	/* Should improve to test w.r.t. dividing lines */
	}

/***********************************************************************
*                                                                      *
*      a r e a _ c l o s e s t _ h o l e                               *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Find closest hole in the given area to the given point.
 *
 * @param[in]	area	area to test against.
 * @param[in]	ptest	test point.
 * @param[out]	*dist	distance to closest point.
 * @param[out]	point	closest point
 * @return	pointer to hole boundary.
 ***********************************************************************/
LINE	area_closest_hole

	(
	AREA	area,	/* given area */
	POINT	ptest,	/* given test point */
	float	*dist,	/* distance to closest point */
	POINT	point	/* closest point */
	)

	{
	/* Initialize return parameters */
	if (point) copy_point(point, ZeroPoint);
	if (dist)  *dist = -1;
	if (!area || !ptest) return NullLine;

	return bound_closest_hole(area->bound, ptest, dist, point);
	}

/***********************************************************************
*                                                                      *
*     h o l e _ i n s i d e _ a r e a                                  *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Determine if part of the given hole is inside the given area.
 *
 * @param[in]	area	area to test.
 * @param[in]	hole	hole to test against.
 * @return	True if successful.
 ***********************************************************************/
LOGICAL	hole_inside_area

	(
	AREA		area,	/* given area */
	LINE		hole	/* hole to test against */
	)

	{
	float	*ppos, *npos;
	int		ip;
	LOGICAL	inside, intrsct, between;

	if (!area)                             return FALSE;
	if (!area->bound)                      return FALSE;
	if (!area->bound->boundary)            return FALSE;
	if (area->bound->boundary->numpts < 1) return FALSE;
	if (!hole)                             return FALSE;
	if (hole->numpts < 1)                  return FALSE;

	/* Check if a point on the hole is inside the area */
	for (ip=0; ip<hole->numpts; ip++)
		{
		npos = hole->points[ip];
		area_test_point(area, npos, NullFloat, NullPoint, (AMEMBER *) 0,
							NullInt, NullInt, &inside);
		if (inside) return TRUE;
		}

	/* Next check if a span on the hole intersects the area */
	ppos = hole->points[0];
	for (ip=1; ip<hole->numpts; ip++)
		{

		/* Skip coincident points */
		npos = hole->points[ip];
		if (npos[X] == ppos[X] && npos[Y] == ppos[Y]) continue;

		/* Look for an intersection with area between these points */
		intrsct = area_sight(area, ppos, npos, FALSE, NullFloat, NullFloat,
						NullPoint, (AMEMBER *) 0, NullInt, NullInt, &between);
		if (intrsct && between) return TRUE;

		/* Reset start point if no intersection */
		ppos = npos;
		}

	/* Last check the area is totally inside the hole */
	ppos = area->bound->boundary->points[0];
	line_test_point(hole, ppos, NullFloat, NullPoint,
			NullInt, &inside, NullLogical);
	if (inside) return TRUE;

	/* Return FALSE if hole does not intersect the area */
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*     i n b o x _ a r e a                                              *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Determine if the given area passes through the given box.
 *
 * @param[in]	area	area to test.
 * @param[in]	*box	box to test against.
 * @return	True if successful.
 ***********************************************************************/
LOGICAL	inbox_area

	(
	AREA		area,	/* given area */
	const BOX	*box	/* box to test against */
	)

	{
	if (!area) return FALSE;
	if (!box)  return FALSE;

	/* Test w.r.t. main boundary with holes */
	return inbox_bound(area->bound, box);
	}

/***********************************************************************
*                                                                      *
*      t r a n s l a t e _ a r e a                                     *
*      r o t a t e _ a r e a                                           *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Translate the given area.
 *
 *	@param[in] 	area	given area
 *	@param[in] 	dx		x offset
 *	@param[in] 	dy		y offset
 *  @return True if successful.
 *********************************************************************/
LOGICAL	translate_area

	(
	AREA	area,
	float	dx,
	float	dy
	)

	{
	int		ih, id, isub, isv, ivis;
	SUBAREA	sub;
	SUBVIS	svis;
	SEGMENT	seg;

	if (!area)                  return FALSE;
	if (!area->bound)           return FALSE;
	if (!area->bound->boundary) return FALSE;

	/* Translate area boundary/holes/dividing lines */
	translate_line(area->bound->boundary, dx, dy);

	/* Translate area holes */
	for (ih=0; ih<area->bound->numhole; ih++)
		translate_line(area->bound->holes[ih], dx, dy);

	/* Translate area dividing lines */
	for (id=0; id<area->numdiv; id++)
		translate_line(area->divlines[id], dx, dy);

	/* Also translate exclusive subarea segments */
	if (area->numdiv > 0 || NotNull(area->subareas))
		{
		for (isub=0; isub<area->numdiv+1; isub++)
			{
			sub = area->subareas[isub];
			if (!sub)           continue;
			if (!sub->visready) continue;
			for (isv=0; isv<sub->nsubvis; isv++)
				{
				svis = sub->subvis[isv];
				for (ivis=0; ivis<svis->numvis; ivis++)
					{
					seg = svis->segvis[ivis];
					if (seg_exclusive(seg))
						translate_line(seg->line, dx, dy);
					}
				}
			}
		}
	return TRUE;
	}

/*********************************************************************/
/** Rotate the given area.
 *
 *	@param[in] 	area	given area
 *	@param[in] 	ref		centre of rotation
 *	@param[in] 	angle	angle of rotation
 *  @return True if successful.
 *********************************************************************/
LOGICAL	rotate_area

	(
	AREA	area,
	POINT	ref,
	float	angle
	)

	{
	int		ih, id, isub, isv, ivis;
	SUBAREA	sub;
	SUBVIS	svis;
	SEGMENT	seg;

	if (!area)                  return FALSE;
	if (!area->bound)           return FALSE;
	if (!area->bound->boundary) return FALSE;

	/* Rotate area boundary/holes/dividing lines */
	rotate_line(area->bound->boundary, ref, angle);

	/* Rotate area holes */
	for (ih=0; ih<area->bound->numhole; ih++)
		rotate_line(area->bound->holes[ih], ref, angle);

	/* Rotate area dividing lines */
	for (id=0; id<area->numdiv; id++)
		rotate_line(area->divlines[id], ref, angle);

	/* Also rotate exclusive subarea segments */
	if (area->numdiv > 0 || NotNull(area->subareas))
		{
		for (isub=0; isub<area->numdiv+1; isub++)
			{
			sub = area->subareas[isub];
			if (!sub)           continue;
			if (!sub->visready) continue;
			for (isv=0; isv<sub->nsubvis; isv++)
				{
				svis = sub->subvis[isv];
				for (ivis=0; ivis<svis->numvis; ivis++)
					{
					seg = svis->segvis[ivis];
					if (seg_exclusive(seg))
						rotate_line(seg->line, ref, angle);
					}
				}
			}
		}
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     c l i p _ d i v l i n e _ t o _ a r e a                          *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Clip a dividing line to lie within bounds of a given area
 *
 * @param[in]	area	area to be divided.
 * @param[in]	divl	dividing line.
 * @param[in]	fwd		begin at start of line?
 * @param[in]	nearedge extend line to boundary if near edge?
 * @param[out]	status	what went wrong?
 * @return	pointer to clipped dividing line.
 ***********************************************************************/
LINE	clip_divline_to_area

	(
	AREA	area,		/* area to be divided */
	LINE	divl,		/* dividing line */
	LOGICAL	fwd,		/* begin at start of line? */
	LOGICAL	nearedge,	/* extend line to boundary if near edge? */
	DIVSTAT	*status		/* What went wrong? */
	)

	{
	LOGICAL	inside, between;
	int		npd, ip, ipbgn, ipend, jpbgn, jpend, nextra;
	LINE	newdivl;
	POINT	*dpts, spos, epos, mpos;
	float	adist, sdist, edist, tdist, dx, dy;

	/* Setup default returns */
	if (status) *status = DivNoInfo;

	/* Return if no area or dividing line */
	if (!area) return NullLine;
	if (!divl) return NullLine;

	/* Initialize parameters */
	newdivl = copy_line(divl);
	condense_line(newdivl);
	dpts = newdivl->points;
	npd  = newdivl->numpts;

	/* Check for dividing lines that are too short */
	if (newdivl->numpts < 2)
		{
		if (status) *status = DivTooShort;
		destroy_line(newdivl);
		return NullLine;
		}

	/* Set average length of line spans (for extending dividing line) */
	line_properties(newdivl, NullChar, NullChar, NullFloat, &adist);
	adist /= (float) (npd-1);

	/* Check forwards from start of line */
	if (fwd)
		{

		/* Find first point inside the area */
		for (ip=0; ip<npd; ip++)
			{
			(void) area_test_point(area, dpts[ip], NullFloat, NullPoint,
									(AMEMBER *) 0, NullInt, NullInt, &inside);
			if (inside) break;
			}
		ipbgn = ip;

		/* Check if dividing line does not pass through the area! */
		if (ip >= npd)
			{
			if (status) *status = area_wrt_line(newdivl, area);
			destroy_line(newdivl);
			return NullLine;
			}

		/* Find last point inside the area */
		for (ip=ipbgn+1; ip<npd; ip++)
			{
			(void) area_test_point(area, dpts[ip], NullFloat, NullPoint,
									(AMEMBER *) 0, NullInt, NullInt, &inside);
			if (!inside) break;
			}
		ipend = ip-1;

		/* Check for suspiciously short dividing lines */
		if (ipend-ipbgn+1 < npd/2)
			{

#			ifdef DEBUG_CLIP
			pr_diag("clip_divline_to_area()",
				"Too few points from %d point line?  Begin/end: %d %d\n",
				npd, ipbgn, ipend);
#			endif /* DEBUG_CLIP */

			/* Find next point inside the area */
			for (ip=ipend+1; ip<npd; ip++)
				{
				(void) area_test_point(area, dpts[ip], NullFloat, NullPoint,
										(AMEMBER *) 0, NullInt, NullInt,
										&inside);
				if (inside) break;
				}

			/* Reset first point and last point inside area */
			if (ip < npd)
				{
				jpbgn = ip;
				for (ip=jpbgn+1; ip<npd; ip++)
					{
					(void) area_test_point(area, dpts[ip], NullFloat, NullPoint,
											(AMEMBER *) 0, NullInt, NullInt,
											&inside);
					if (!inside) break;
					}
				jpend = ip-1;
				if (jpend-jpbgn > ipend-ipbgn)
					{
					ipbgn = jpbgn;
					ipend = jpend;
					}
				}
			}
		}

	/* Check backwards from end of line */
	else
		{

		/* Find first point inside the area */
		for (ip=npd-1; ip>=0; ip--)
			{
			(void) area_test_point(area, dpts[ip], NullFloat, NullPoint,
									(AMEMBER *) 0, NullInt, NullInt, &inside);
			if (inside) break;
			}
		ipend = ip;

		/* Check if dividing line does not pass through the area! */
		if (ip < 0)
			{
			if (status) *status = area_wrt_line(newdivl, area);
			destroy_line(newdivl);
			return NullLine;
			}

		/* Find last point inside the area */
		for (ip=ipend-1; ip>=0; ip--)
			{
			(void) area_test_point(area, dpts[ip], NullFloat, NullPoint,
									(AMEMBER *) 0, NullInt, NullInt, &inside);
			if (!inside) break;
			}
		ipbgn = ip+1;

		/* Check for suspiciously short dividing lines */
		if (ipend-ipbgn+1 < npd/2)
			{
#			ifdef DEBUG_CLIP
			pr_diag("clip_divline_to_area()",
				"Too few points from %d point line?  Begin/end: %d %d\n",
				npd, ipbgn, ipend);
#			endif /* DEBUG_CLIP */

			/* Find next point inside the area */
			for (ip=ipbgn-1; ip>=0; ip--)
				{
				(void) area_test_point(area, dpts[ip], NullFloat, NullPoint,
										(AMEMBER *) 0, NullInt, NullInt,
										&inside);
				if (inside) break;
				}

			/* Reset last point and first point inside area */
			if (ip >= 0)
				{
				jpend = ip;
				for (ip=jpend-1; ip>=0; ip--)
					{
					(void) area_test_point(area, dpts[ip], NullFloat, NullPoint,
											(AMEMBER *) 0, NullInt, NullInt,
											&inside);
					if (!inside) break;
					}
				jpbgn = ip+1;
				if (jpend-jpbgn > ipend-ipbgn)
					{
					ipbgn = jpbgn;
					ipend = jpend;
					}
				}
			}
		}

	/* Extend the dividing line to the area boundaries */
	/*  where only the start point is inside the area  */
	if (ipbgn == 0 && ipend == ipbgn)
		{
		ip = ipbgn;
		(void) area_sight(area, dpts[ip], dpts[ip+1], FALSE, NullFloat,
					NullFloat, epos, (AMEMBER *) 0, NullInt, NullInt, NullChar);
		mpos[X] = (dpts[ip][X] + epos[X]) / 2.0;
		mpos[Y] = (dpts[ip][Y] + epos[Y]) / 2.0;
		(void) area_sight(area, mpos, dpts[ip], FALSE, NullFloat,
					NullFloat, spos, (AMEMBER *) 0, NullInt, NullInt, NullChar);
		}

	/* Extend the dividing line to the area boundaries */
	/*  where only the end point is inside the area    */
	else if (ipbgn == npd-1 && ipend == ipbgn)
		{
		ip = ipbgn;
		(void) area_sight(area, dpts[ip], dpts[ip-1], FALSE, NullFloat,
					NullFloat, spos, (AMEMBER *) 0, NullInt, NullInt, NullChar);
		mpos[X] = (dpts[ip][X] + spos[X]) / 2.0;
		mpos[Y] = (dpts[ip][Y] + spos[Y]) / 2.0;
		(void) area_sight(area, mpos, dpts[ip], FALSE, NullFloat,
					NullFloat, epos, (AMEMBER *) 0, NullInt, NullInt, NullChar);
		}

	/* Extend the dividing line to the area boundaries */
	else
		{

		/* Find the intersection for the start point */
		ip = ipbgn;
		if (ip == 0) ip++;
		(void) area_sight(area, dpts[ip], dpts[ip-1], FALSE, &sdist,
					NullFloat, spos, (AMEMBER *) 0, NullInt, NullInt, &between);

		/* Reset the start point intersection for a dividing line */
		/*  that is already very close to the area boundary       */
		if (!between && nearedge)
			{
			(void) area_test_point(area, dpts[ipbgn], &tdist, mpos,
									(AMEMBER *) 0, NullInt, NullInt, NullChar);
			if (tdist < sdist*NearEdgeRatio) copy_point(spos, mpos);
			}

		/* Find the intersection for the end point */
		ip = ipend;
		if (ip == npd-1) ip--;
		(void) area_sight(area, dpts[ip], dpts[ip+1], FALSE, &edist,
					NullFloat, epos, (AMEMBER *) 0, NullInt, NullInt, &between);

		/* Reset the end point intersection for a dividing line */
		/*  that is already very close to the area boundary     */
		if (!between && nearedge)
			{
			(void) area_test_point(area, dpts[ipend], &tdist, mpos,
									(AMEMBER *) 0, NullInt, NullInt, NullChar);
			if (tdist < edist*NearEdgeRatio) copy_point(epos, mpos);
			}
		}

#	ifdef DEBUG_CLIP
	pr_diag("clip_divline_to_area()", "Begin/end: %d %d\n", ipbgn, ipend);
	pr_diag("clip_divline_to_area()",
		"Start/End points: %.2f %.2f   %.2f %.2f\n",
		spos[X], spos[Y], epos[X], epos[Y]);
#	endif /* DEBUG_CLIP */

	/* Truncate the start of the dividing line */
	if (ipbgn > 0)
		{
		reverse_line(newdivl);
		newdivl->numpts -= ipbgn;
		reverse_line(newdivl);
		}

	/* Truncate the end of the dividing line */
	if (ipend < npd-1)
		{
		newdivl->numpts -= (npd-1-ipend);
		}

	/* Check whether extra points needed for extending start span */
	dpts   = newdivl->points;
	copy_point(mpos, dpts[0]);
	sdist  = point_dist(spos, mpos);
	nextra = NINT(sdist/adist);
#	ifdef DEBUG_CLIP
	if (nextra > 1)
		{
		pr_diag("clip_divline_to_area()",
			"Add %d extra points at start\n", nextra-1);
		pr_diag("clip_divline_to_area()",
			"Extend start from point: %.2f %.2f\n", mpos[X], mpos[Y]);
		}
#	endif /* DEBUG_CLIP */

	/* Now add the new start point (and extra points if start span too long) */
	reverse_line(newdivl);
	if (nextra > 1)
		{
		dx = (spos[X] - mpos[X]) / (float) nextra;
		dy = (spos[Y] - mpos[Y]) / (float) nextra;

		for (ip=1; ip<nextra; ip++)
			{
			mpos[X] += dx;
			mpos[Y] += dy;
			add_point_to_line(newdivl, mpos);

#			ifdef DEBUG_CLIP
			pr_diag("clip_divline_to_area()",
				"Extra start point: %.2f %.2f\n", mpos[X], mpos[Y]);
#			endif /* DEBUG_CLIP */
			}
		}
	add_point_to_line(newdivl, spos);
	reverse_line(newdivl);

	/* Check whether extra points needed for extending end span */
	dpts   = newdivl->points;
	npd    = newdivl->numpts;
	copy_point(mpos, dpts[npd-1]);
	edist  = point_dist(epos, mpos);
	nextra = NINT(edist/adist);
#	ifdef DEBUG_CLIP
	if (nextra > 1)
		{
		pr_diag("clip_divline_to_area()",
			"Add %d extra points at end\n", nextra-1);
		pr_diag("clip_divline_to_area()",
			"Extend end from point: %.2f %.2f\n", mpos[X], mpos[Y]);
		}
#	endif /* DEBUG_CLIP */

	/* Now add the new end point (and extra points if end span too long) */
	if (nextra > 1)
		{
		dx = (epos[X] - mpos[X]) / (float) nextra;
		dy = (epos[Y] - mpos[Y]) / (float) nextra;

		for (ip=1; ip<nextra; ip++)
			{
			mpos[X] += dx;
			mpos[Y] += dy;
			add_point_to_line(newdivl, mpos);

#			ifdef DEBUG_CLIP
			pr_diag("clip_divline_to_area()",
				"Extra end point: %.2f %.2f\n", mpos[X], mpos[Y]);
#			endif /* DEBUG_CLIP */
			}
		}
	add_point_to_line(newdivl, epos);
	condense_line(newdivl);

	/* Return the dividing line */
	if (status) *status = DivOK;
	return newdivl;
	}

/***********************************************************************
*                                                                      *
*      Routines specific to a given SUBAREA                            *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*      s u b a r e a _ p r o p e r t i e s                             *
************************************************************************/
/***********************************************************************/
/**	Measure certain 2-D properties of a subarea:
 *
 *        - clockwise or counter-clockwise
 *        - enclosed area (using midpoint rule for integration)
 *        - length of boundary
 *
 * @param[in]	sub		subarea to measure.
 * @param[out]	*clockwise is it clockwise?
 * @param[out]	*size	size of enclosed area.
 * @param[out]	*length	length of boundary.
 ***********************************************************************/

void	subarea_properties

	(
	SUBAREA	sub,		/* given subarea */
	LOGICAL	*clockwise,	/* is it clockwise? */
	float	*size,		/* enclosed area */
	float	*length		/* length of boundary */
	)

	{
	float	x, xprev, dx;
	float	y, yprev, dy;
	float	yfirst, ybar;
	int		iseg, nseg, ips, ipe, ip, jp, np, dp;
	SEGMENT	seg;
	LINE	lseg;
	LOGICAL	sok, right;

	LOGICAL	First = TRUE;

	LOGICAL	NeedClock  = FALSE;		LOGICAL	Clock  = FALSE;
	LOGICAL	NeedSize   = FALSE;		float	Size   = 0;
	LOGICAL	NeedLength = FALSE;		float	Length = 0;

	/* Initialize to nice garbage */
	if (clockwise) *clockwise = Clock;
	if (size)      *size      = Size;
	if (length)    *length    = Length;

	/* Return if sub not there */
	if (!sub)             return;
	if (sub->numseg <= 0) return;

	/* What things do we need to compute */
	NeedClock  = (LOGICAL) (NotNull(clockwise));
	NeedSize   = (LOGICAL) (NeedClock || NotNull(size));
	NeedLength = (LOGICAL) (NotNull(length));

	/* Loop through all boundary segments */
	nseg = sub->numseg;

#	ifdef DEBUG_PROPERTIES
	pr_diag("Properties", "Subarea segments: %d\n", nseg);
#	endif /* DEBUG_PROPERTIES */

	for (iseg=0; iseg<nseg; iseg++)
		{
		seg  = sub->segments[iseg];
		lseg = seg->line;
		if (seg_forward(seg))
			{
			np  = lseg->numpts;
			dp  = 1;
			ips = seg->ips;
			ipe = seg->ipe;
			if (ipe < ips) ipe += np;
			}
		else
			{
			np  = lseg->numpts;
			dp  = -1;
			ips = seg->ipe;
			ipe = seg->ips;
			if (ips < ipe) ips += np;
			}

#		ifdef DEBUG_PROPERTIES
		pr_diag("Properties", "  Segment: %d\n", iseg);
		if (NeedSize && NeedLength)
			pr_diag("", "  Size/Length: ");
		else if (NeedSize)
			pr_diag("", "  Size: ");
		else if (NeedLength)
			pr_diag("", "  Length: ");
#		endif /* DEBUG_PROPERTIES */

		/* Loop through points in current boundary segment in proper order */
		for (ip=ips; ; ip+=dp)
			{
			if ( seg_forward(seg) && ip>ipe) break;
			if (!seg_forward(seg) && ip<ipe) break;

			/* Extract point co-ordinates */
			jp = ip%np;
			x  = lseg->points[jp][X];
			y  = lseg->points[jp][Y];

			/* Set up first point */
			/* Need initial y value for area calculation */
			if (First)
				{
				First  = FALSE;
				yfirst = y;
				xprev  = x;
				yprev  = y;
				continue;
				}

			/* Increment area and length computations */
			dx = x - xprev;
			dy = y - yprev;
			if ((dx==0) && (dy==0)) continue;
			ybar = 0.5*( y + yprev );
			if (NeedSize)   Size   += dx*( ybar - yfirst );
			if (NeedLength) Length += hypot((double) dx, (double) dy);

#			ifdef DEBUG_PROPERTIES
			if (NeedSize && NeedLength)
				{
				pr_diag("", " %.0f/%.0f ", Size, Length);
				if ((ip-ips)%4 == 0)
					{
					pr_diag("", "\n");
					pr_diag("", "               ");
					}
				}
			else if (NeedSize)
				{
				pr_diag("", " %.0f ", Size);
				if ((ip-ips)%8 == 0)
					{
					pr_diag("", "\n");
					pr_diag("", "        ");
					}
				}
			else if (NeedLength)
				{
				pr_diag("", " %.0f ", Length);
				if ((ip-ips)%8 == 0)
					{
					pr_diag("", "\n");
					pr_diag("", "          ");
					}
				}
#			endif /* DEBUG_PROPERTIES */

			/* Update previous point */
			xprev = x;
			yprev = y;
			}

#		ifdef DEBUG_PROPERTIES
		if (NeedSize || NeedLength)
			pr_diag("", "\n");
#		endif /* DEBUG_PROPERTIES */

		}

	/* Make sure sub gets closed */

	/* Compute remaining properties */
	if (clockwise) Clock = (LOGICAL) ( Size > 0 );
	if (size)      Size  = fabs((double) Size);

	/* Check for consistent subarea properties */
	if (clockwise && fabs((double) Size) > 0.0)
		{
		sok = subarea_orientation(sub, &right);
		if (sok && Clock && !right)
			{
			(void) pr_error("Areas",
				"Fixing subarea properties - should be counterclockwise!\n");
			Clock = FALSE;
			}
		else if (sok && !Clock && right)
			{
			(void) pr_error("Areas",
				"Fixing subarea properties - should be clockwise!\n");
			Clock = TRUE;
			}
		}

	/* Return what was requested */
	if (clockwise) *clockwise = Clock;
	if (size)      *size      = Size;
	if (length)    *length    = Length;
	}

/***********************************************************************
*                                                                      *
*      s u b a r e a _ t e s t _ p o i n t                             *
*                                                                      *
************************************************************************/
/***********************************************************************/
/**	Measure certain 2-D properties of a test point, relative to
 * the given subarea:
 *
 *        - minimum (perpendicular) distance from point to boundary
 *        - closest point on boundary (interpolated if necessary)
 *        - index of segment which contains closest point
 *        - index of span in segment which contains closest point
 *        - point is inside or outside
 *
 * @param[in]	sub		subarea to measure.
 * @param[in]	ptest	test point.
 * @param[out]	*pdist	minimum (perpendicular) distance to line.
 * @param[out]	ppoint	closest (perpendicular) point on line.
 * @param[out]	*seg	index of segment which contains closest point.
 * @param[out]	*span	index of span which contains closest point.
 * @param[out]	*inside is point enclosed by subarea?
 ***********************************************************************/

void	subarea_test_point

	(
	SUBAREA	sub,	/* given subarea */
	POINT	ptest,	/* given test point */
	float	*pdist,	/* minimum (perpendicular) distance to line */
	POINT	ppoint,	/* closest (perpendicular) point on line */
	int		*seg,	/* index of segment which contains closest point */
	int		*span,	/* index of span which contains closest point */
	LOGICAL	*inside	/* is point enclosed by subarea? */
	)

	{
	float	xtest, ytest;
	float	x, y, xfirst, yfirst;
	float	xprev, yprev;
	int		nseg, iseg, lastseg;
	int		np, ips, ipe, ip, jp, dp, lastjp;
	float	dxa, dya, da2;	/* dist from point to start of span */
	float	dxb, dyb, db2;	/* dist from point to end of span */
	float	dxp, dyp, dp2;	/* perp distance to current span */
	float	dxs, dys, ds2;	/* length of current span */
	float	dtol=1e-5;
	float	a, dmin2;
	float	anga, angb, tang, dang;
	SEGMENT	sseg;
	LINE	lseg;

	LOGICAL	First = TRUE;

	float	Dist    = -1;
	int		Segment = -1;
	int		Segspan = -1;
	LOGICAL	Inside  = FALSE;

	/* Initialize to nice garbage */
	if (ppoint) copy_point(ppoint, ptest);
	if (pdist)  *pdist  = Dist;
	if (seg)    *seg    = Segment;
	if (span)   *span   = Segspan;
	if (inside) *inside = Inside;

	/* Return if sub not there */
	if (!sub)             return;
	if (sub->numseg <= 0) return;
	if (!ptest)           return;

	/* Initialize a few things */
	xtest = ptest[X];
	ytest = ptest[Y];
	tang  = 0;

	lastjp  = -1;
	lastseg = -1;

	/* Loop through all boundary segments */
	nseg = sub->numseg;
	for (iseg=0; iseg<nseg; iseg++)
		{
		sseg = sub->segments[iseg];
		if (IsNull(sseg)) continue;
		lseg = sseg->line;
		if (IsNull(lseg) || lseg->numpts <= 0) continue;
		if (seg_forward(sseg))
			{
			np  = lseg->numpts;
			dp  = 1;
			ips = sseg->ips;
			ipe = sseg->ipe;
			if (ipe < ips) ipe += np;
			}
		else
			{
			np  = lseg->numpts;
			dp  = -1;
			ips = sseg->ipe;
			ipe = sseg->ips;
			if (ips < ipe) ips += np;
			}

		/* Loop through points in current boundary segment in proper order */
		lastseg = iseg;
		for (ip=ips; ; ip+=dp)
			{
			if ( seg_forward(sseg) && ip>ipe) break;
			if (!seg_forward(sseg) && ip<ipe) break;

			/* Extract point co-ordinates */
			jp = ip%np;
			lastjp = jp;
			x  = lseg->points[jp][X];
			y  = lseg->points[jp][Y];

			/* Parameters associated with first point */
			if (First)
				{
				First = FALSE;
				dxa = x - xtest;
				dya = y - ytest;
				da2 = dxa*dxa + dya*dya;
				if (da2 == 0)
					{
					/* Found an exact match! */
					Dist    = 0;
					Segment = iseg;
					Segspan = jp;

#					ifdef DEBUG_TESTPOINT
					if (seg_forward(sseg))
						pr_diag("subarea_test_point()",
							"Exact match at point: %d on segment: %d (%d-%d)\n",
							jp, iseg, sseg->ips, sseg->ipe);
					else
						pr_diag("subarea_test_point()",
							"Exact match at point: %d on segment: %d (%d-%d)\n",
							jp, iseg, sseg->ipe, sseg->ips);
#					endif /* DEBUG_TESTPOINT */

					if (ppoint) set_point(ppoint, x, y);
					if (pdist)  *pdist  = Dist;
					if (seg)    *seg    = Segment;
					if (span)   *span   = Segspan;
					if (inside) *inside = Inside;
					return;
					}
				anga    = atan2(dya, dxa);
				dmin2   = da2;
				Segment = iseg;
				Segspan = jp;
				if (ppoint) set_point(ppoint, x, y);
				xfirst  = x;
				yfirst  = y;
				xprev   = x;
				yprev   = y;

#				ifdef DEBUG_TESTPOINT
				if (seg_forward(sseg))
					pr_diag("subarea_test_point()",
						"Initial point: %d on segment: %d (%d-%d)\n",
						jp, iseg, sseg->ips, sseg->ipe);
				else
					pr_diag("subarea_test_point()",
						"Initial point: %d on segment: %d (%d-%d)\n",
						jp, iseg, sseg->ipe, sseg->ips);
#				endif /* DEBUG_TESTPOINT */

				continue;
				}

			/* Parameters associated with a subsequent point */
			dxb = x - xtest;
			dyb = y - ytest;
			db2 = dxb*dxb + dyb*dyb;
			if (db2 == 0)
				{

				/* Found an exact match! */
				Dist    = 0;
				Segment = iseg;
				Segspan = jp;
				if (ppoint) set_point(ppoint, x, y);

#				ifdef DEBUG_TESTPOINT
				if (seg_forward(sseg))
					pr_diag("subarea_test_point()",
						"Exact match at point: %d on segment: %d (%d-%d)\n",
						jp, iseg, sseg->ips, sseg->ipe);
				else
					pr_diag("subarea_test_point()",
						"Exact match at point: %d on segment: %d (%d-%d)\n",
						jp, iseg, sseg->ipe, sseg->ips);
#				endif /* DEBUG_TESTPOINT */

				if (pdist)  *pdist  = Dist;
				if (seg)    *seg    = Segment;
				if (span)   *span   = Segspan;
				if (inside) *inside = Inside;
				return;
				}

			/* Distance between current and previous points */
			dxs = x - xprev;
			dys = y - yprev;
			ds2 = dxs*dxs + dys*dys;

			/* Ignore very short spans ... check the next one instead! */
			if ((fabs((double) dxs)<dtol) && (fabs((double) dys)<dtol)) continue;

			/* Increment cumulative angle */
			if (db2 > 0) angb = atan2(dyb, dxb);
			else         angb = 0;
			dang = angb - anga;
			if (dang < -PI) dang += (PI + PI);
			if (dang >  PI) dang -= (PI + PI);
			tang += dang;

			/* If segment contains perpendicular, compute the  */
			/* perpendicular distance */
			if (ds2 > fabs((double)(db2-da2)))
				{
				/* Almost horizontal case ... y = f(x) */
				/* if (fabs((double) (dxb-dxa)) >= fabs((double) (dyb-dya))) */
				if (fabs((double) dxs) >= fabs((double) dys))
					{
					a   = (dys)/(dxs);
					dxp = a*(a*dxa - dya) / (1 + a*a);
					dyp = a*(dxp-dxa) + dya;
					dp2 = dxp*dxp + dyp*dyp;
					}
				/* Almost vertical case ... x = g(y) */
				else
					{
					a   = (dxs)/(dys);
					dyp = a*(a*dya - dxa) / (1 + a*a);
					dxp = a*(dyp-dya) + dxa;
					dp2 = dxp*dxp + dyp*dyp;
					}
				}

			/* Otherwise (segment does not contain perpendicular) */
			/* use end point */
			else
				{
				dxp = dxb;
				dyp = dyb;
				dp2 = db2;
				}

			/* If closer than best so far, remember it */
			if (dp2 < dmin2)
				{
				dmin2   = dp2;
				Segment = iseg;
				Segspan = (seg_forward(sseg))? jp-1: jp;
				if (ppoint)set_point(ppoint, xtest+dxp, ytest+dyp);

#				ifdef DEBUG_TESTPOINT
				if (seg_forward(sseg))
					pr_diag("subarea_test_point()",
						"Closer span: %d on segment: %d (%d-%d)\n",
						jp-1, iseg, sseg->ips, sseg->ipe);
				else
					pr_diag("subarea_test_point()",
						"Closer span: %d on segment: %d (%d-%d)\n",
						jp, iseg, sseg->ipe, sseg->ips);
#				endif /* DEBUG_TESTPOINT */
				}

			/* Update parameters for previous point */
			dxa   = dxb;
			dya   = dyb;
			da2   = db2;
			anga  = angb;
			xprev = x;
			yprev = y;
			}

		/* Compute (or re-compute) remaining properties */
		Dist   = sqrt(dmin2);
		Inside = (LOGICAL) ( fabs((double) tang) > PI );
		}

	/* Determine parameters from last point of last segment */
	/* to first point of first segment */
	if (!First)
		{
		dxs = xfirst - xprev;
		dys = yfirst - yprev;
		if ((fabs((double) dxs)>=dtol) || (fabs((double) dys)>=dtol))
			{

			/* Distance between first and previous point */
			ds2 = dxs*dxs + dys*dys;

			/* Parameters associated with the first point */
			dxb = xfirst - xtest;
			dyb = yfirst - ytest;
			db2 = dxb*dxb + dyb*dyb;

			/* Increment cumulative angle */
			if (db2 > 0) angb = atan2(dyb, dxb);
			else         angb = 0;
			dang = angb - anga;
			if (dang < -PI) dang += (PI + PI);
			if (dang >  PI) dang -= (PI + PI);
			tang += dang;

			/* If segment contains perpendicular, compute the  */
			/* perpendicular distance */
			if (ds2 > fabs((double) (db2-da2)))
				{
				/* Almost horizontal case ... y = f(x) */
				/* if (fabs((double) (dxb-dxa)) >= fabs((double) (dyb-dya))) */
				if (fabs((double)dxs) >= fabs((double)dys))
					{
					a   = (dys)/(dxs);
					dxp = a*(a*dxa - dya) / (1 + a*a);
					dyp = a*(dxp-dxa) + dya;
					dp2 = dxp*dxp + dyp*dyp;
					}
				/* Almost vertical case ... x = g(y) */
				else
					{
					a   = (dxs)/(dys);
					dyp = a*(a*dya - dxa) / (1 + a*a);
					dxp = a*(dyp-dya) + dxa;
					dp2 = dxp*dxp + dyp*dyp;
					}
				}

			/* Otherwise (segment does not contain perpendicular) */
			/* use end point */
			else
				{
				dxp = dxb;
				dyp = dyb;
				dp2 = db2;
				}

			/* If closer than best so far, remember it */
			if (dp2 < dmin2)
				{
				dmin2   = dp2;
				sseg    = sub->segments[lastseg];
				Segment = lastseg;
				Segspan = (seg_forward(sseg))? lastjp: lastjp-1;
				if (ppoint)set_point(ppoint, xtest+dxp, ytest+dyp);

#				ifdef DEBUG_TESTPOINT
				if (seg_forward(sseg))
					pr_diag("subarea_test_point()",
						"Last span: %d on segment: %d (%d-%d)\n",
						lastjp, lastseg, sseg->ips, sseg->ipe);
				else
					pr_diag("subarea_test_point()",
						"Last span: %d on segment: %d (%d-%d)\n",
						lastjp-1, lastseg, sseg->ipe, sseg->ips);
#				endif /* DEBUG_TESTPOINT */
				}

			/* Re-compute remaining properties */
			Dist   = sqrt(dmin2);
			Inside = (LOGICAL) ( fabs((double) tang) > PI );
			}
		}

#	ifdef DEBUG_TESTPOINT
	pr_diag("subarea_test_point()",
		"Returning span: %d on segment: %d\n", Segspan, Segment);
#	endif /* DEBUG_TESTPOINT */

	/* Return what was requested */
	if (pdist)  *pdist  = Dist;
	if (seg)    *seg    = Segment;
	if (span)   *span   = Segspan;
	if (inside) *inside = Inside;
	}

/***********************************************************************
*                                                                      *
*      s u b a r e a _ s i g h t                                       *
*                                                                      *
************************************************************************/
/***********************************************************************/
/**	Find closest point on the given subarea to a given test point,
 * along the line of sight defined by a second test point.  If no
 * intersection occurs, return the closest approach.
 *
 *	Method:
 *	- Parameterize a line through the test points as:
 *      @f$({x_1}, {y_1}) + {t}({x_2} - {x_1}, {y_2} - {y_1})@f$
 *	- Then parameterize a line through the end points of each span on
 *	the curve, as:
 *	@f$ ({x_a}, {y_a}) + {s}({x_b} - {x_a}, {y_b} - {y_a})@f$
 *	- Then solve for s, such that the two lines cross.
 *	- We have an intersection only if s is between 0 and
 *  	1, meaning that the intersection lies within the
 *  	span.
 *	- Return only the closest of these intersections, if
 *  	any were found.  A negative distance just means that
 *  	the closest intersection was found in the opposite
 *  	direction from the second point.
 *
 *      @note Make sure to test the return value:
 *
 *	- A return value of TRUE means that an intersection was
 *	found and the returned parameters may be interpretted as:
 *		-	dist     = distance from p1 to intersection,
 *		-	approach = (forced to zero),
 *		-	point    = the actual point of intersection on the target area,
 *		-	seg      = segment of subarea boundary containing the intersection,
 *		-	span     = span on segment containing the intersection,
 *		-	between  = is intersection between p1 and p2?
 *	- A return value of FALSE means that no intersections were found
 *	and the returned parameters should be interpretted as:
 *		-	dist     = distance from p1 to closest approach along sight line,
 *		-	approach = shortest distance from sight line to the target area,
 *		-	point    = the actual point of closest approach, on the target area,
 *		-	seg      = segment of subarea boundary that is closest,
 *		-	span     = span on segment that is closest,
 *		-	between  = is closest approach between p1 and p2?
 *	- If something went wrong, approach will be negative.
 *
 * @param[in]	sub	subarea to test.
 * @param[in]	p1			first test point.
 * @param[in]	p2			second test point gives direction of sight.
 * @param[in]	back		do we want negative distance if closer?
 * @param[out]	*dist		distance to closest intersection/approach.
 * @param[out]	*approach	closest approach (0 if intersection).
 * @param[out]	point		closest intersection/approach on area.
 * @param[out]	*seg		index of segment which contains closest point.
 * @param[out]	*span		index of span which contains closest point.
 * @param[out]	*between	is intersection/approach between the test points?
 * @return True if successful.
 ***********************************************************************/

LOGICAL	subarea_sight

	(
	SUBAREA	sub,		/* given subarea */
	POINT	p1,			/* given test point */
	POINT	p2,			/* second point which gives direction of sight */
	LOGICAL	back,		/* do we want negative distance if closer */
	float	*dist,		/* distance to closest intersection/approach */
	float	*approach,	/* closest approach (0 if intersection) */
	POINT	point,		/* closest intersection/approach on area */
	int		*seg,		/* index of segment which contains closest point */
	int		*span,		/* index of span which contains closest point */
	LOGICAL	*between	/* is intersection/approach between the test points */
	)

	{
	int		i;
	float	bdist, bappr;
	POINT	bpoint;
	int		bseg, xspan, bspan;
	int		ips, ipe, ipl, npts;
	SEGMENT	sseg;
	LOGICAL	found, tween, fwd;

	static	LINE	lseg = NullLine;

	/* Set reasonable return values */
	if (point)    copy_point(point, p1);
	if (dist)     *dist     = -1;
	if (approach) *approach = -1;
	if (seg)      *seg      = -1;
	if (span)     *span     = -1;
	if (between)  *between  = FALSE;
	if (!sub || !p1 || !p2) return FALSE;

	if (!lseg) lseg = create_line();
	empty_line(lseg);

	/* Create closed line with all subarea segments */
	for (i=0; i<sub->numseg; i++)
		{
		sseg = sub->segments[i];
		fwd  = (LOGICAL) seg_forward(sseg);
		ips = sseg->ips;
		ipe = sseg->ipe;
		ipl = sseg->line->numpts - 1;
		if (fwd)
            {
            if (ips <= ipe)
                {
                lseg = append_line_pdir(lseg, sseg->line, ips, ipe, fwd);
                }
            else
                {
                lseg = append_line_pdir(lseg, sseg->line, ips, ipl, fwd);
                lseg = append_line_pdir(lseg, sseg->line, 0, ipe, fwd);
                }
            }
        else
            {
            if (ips <= ipe)
                {
                lseg = append_line_pdir(lseg, sseg->line, ips, ipe, fwd);
                }
            else
                {
                lseg = append_line_pdir(lseg, sseg->line, 0, ipe, fwd);
                lseg = append_line_pdir(lseg, sseg->line, ips, ipl, fwd);
                }
            }
        }

	/* Close the subarea outline after the last span */
	close_line(lseg);

	/* Find the intersection or closest approach */
	found = line_sight(lseg, p1, p2, back, &bdist, &bappr, bpoint,
						&xspan, &tween);

	/* Determine the span of the intersection or closest approach */
	if (xspan >= 0 && bappr >= 0.0)
		{

		/* Check each segment for span location */
		for (i=0; i<sub->numseg; i++)
			{
			bseg = i;
			sseg = sub->segments[i];
			fwd  = (LOGICAL) seg_forward(sseg);
			npts = sseg->line->numpts;
			ips  = sseg->ips;
			ipe  = sseg->ipe;

			/* Reset span count for checking next segment */
			if (xspan >= npts)
				{
				xspan -= npts;
				continue;
				}

			/* Determine span based on segment parameters */
			if (fwd) bspan = ips + xspan;
			else     bspan = ipe - xspan - 1;

			/* Set return parameters */
			if (point)    copy_point(point, bpoint);
			if (dist)     *dist     = bdist;
			if (approach) *approach = bappr;
			if (seg)      *seg      = bseg;
			if (span)     *span     = bspan;
			if (between)  *between  = tween;
			}
		}

	/* Return results */
	empty_line(lseg);
	return found;
	}

/***********************************************************************
*                                                                      *
*     f i n d _ s u b a r e a _ c r o s s o v e r                      *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Zero in on the intersection between the given line and the
 * boundary of the given subarea.
 *
 * @param[in]	sub		subarea being divided.
 * @param[in]	line	dividing line.
 * @param[in]	ipin	index of point that is known to be inside.
 * @param[in]	ipout	index of point that is known to be outside.
 * @param[out]	px		point of intersection.
 * @param[out]	*seg	segment containing intersection.
 * @param[out]	*span	span containing intersection.
 * @return index of intersection point.
 ***********************************************************************/
int		find_subarea_crossover

	(
	SUBAREA	sub,	/* subarea being divided */
	LINE	line,	/* dividing line */
	int		ipin,	/* index of a point that is known to be inside */
	int		ipout,	/* index of a point that is known to be outside */
	POINT	px,
	int		*seg,
	int		*span
	)

	{
	LOGICAL	inside;
	int		ipmid;
	POINT	*p;

	/* Initialize return parameters */
	if (px) copy_point(px, ZeroPoint);
	if (seg)  *seg  = -1;
	if (span) *span = -1;
	if (!sub || !line)         return -1;
	if (ipin < 0 )             return -1;
	if (ipin >= line->numpts)  return -1;
	if (ipout < 0 )            return -1;
	if (ipout >= line->numpts) return -1;

	/* Do a binary search for the intersection */
	/* We assume here that ipin is inside and ipout is outside */
	while (abs(ipout - ipin) > 1)
		{
		ipmid = (ipin + ipout) / 2;
		p     = line->points + ipmid;
		subarea_test_point(sub, *p, NullFloat, px, seg, span, &inside);
		if (inside) ipin  = ipmid;
		else        ipout = ipmid;
		}

	/* Return closest segment */
	return ipout;
	}


/***********************************************************************
*                                                                      *
*      Routines specific to the SUBAREAs of a given AREA               *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*      b u i l d _ a r e a _ s u b a r e a s                           *
*                                                                      *
***********************************************************************/
/***********************************************************************/
/**	Build a subarea for an area without divides, that matches the
 * original boundary.
 *
 * @param[in]	area	area to affect.
 ***********************************************************************/

void	build_area_subareas

	(
	AREA	area	/* area to check */
	)

	{
	SUBAREA	sub;
	SEGMENT	seg;

	/* Do nothing if no info */
	if (!area || !area->bound || !area->bound->boundary) return;

	/* Do nothing if it already has subareas */
	if (area->numdiv > 0) return;
	if (area->subareas)   return;

	/* Construct the single subarea equalling the whole area */
	area->subareas = INITMEM(SUBAREA, 1);
	sub = create_subarea(NULL, NULL, NULL);
	define_subarea_attribs(sub, area->attrib);
	area->subareas[0] = sub;

	/* Duplicate presentation specs */
	copy_lspec(&sub->lspec, &area->lspec);
	copy_fspec(&sub->fspec, &area->fspec);

	/* Construct a single segment */
	sub->numseg = 1;
	sub->segments = INITMEM(SEGMENT, 1);
	sub->segments[0] = seg = create_segment();
	define_segment(seg, area->bound->boundary, FALSE, 0,
				   area->bound->boundary->numpts-1, TRUE);
	}

/***********************************************************************
*                                                                      *
*      a r e a _ f r o m _ s u b a r e a                               *
*                                                                      *
***********************************************************************/
/***********************************************************************/
/**	Build an area from a subarea.
 *
 * @param[in]	sub		SUBAREA structure.
 * @return	AREA structure created from SUBAREA structure.
 ***********************************************************************/

AREA	area_from_subarea

	(
	SUBAREA		sub		/* SUBAREA structure */
	)

	{
	AREA		area = NullArea;
	LINE		outline;

	/* Return immediately if no subarea */
	if ( IsNull(sub) ) return NullArea;

	/* Create area boundary from outline of subarea */
	outline = outline_from_subarea(sub);
	if ( IsNull(outline) )     return NullArea;
	if ( outline->numpts < 3 ) return NullArea;

	/* Create an area from the subarea parameters */
	area = create_area(NullString, NullString, NullString);
	define_area_attribs(area, sub->attrib);
	define_area_boundary(area, outline);

	/* Return the area */
	return area;
	}

/***********************************************************************
*                                                                      *
*      o u t l i n e _ f r o m _ s u b a r e a                         *
*                                                                      *
***********************************************************************/
/***********************************************************************/
/**	Build a line from the outline of a subarea.
 *
 * @param[in]	sub		SUBAREA structure.
 * @return	LINE structure created from outline of a SUBAREA structure.
 ***********************************************************************/

LINE	outline_from_subarea

	(
	SUBAREA		sub		/* SUBAREA structure */
	)

	{
	int			ii, ips, ipe, ipl;
	LOGICAL		fwd;
	SEGMENT		seg;
	POINT		pos;
	LINE		poly = NullLine;

	/* Return immediately if no subarea or no lines in subarea */
	if ( IsNull(sub) )      return NullLine;
	if ( sub->numseg <= 0 ) return NullLine;

	/* Build a single polygon */
	for ( ii=0; ii<sub->numseg; ii++ )
		{
		seg  = sub->segments[ii];
		fwd  = (LOGICAL) seg_forward(seg);
		ips  = seg->ips;
		ipe  = seg->ipe;
		ipl  = seg->line->numpts - 1;
		if ( fwd )
			{
			if ( ips <= ipe )
				{
				poly = append_line_pdir(poly, seg->line, ips, ipe, fwd);
				}
			else
				{
				poly = append_line_pdir(poly, seg->line, ips, ipl, fwd);
				poly = append_line_pdir(poly, seg->line, 0,   ipe, fwd);
				}
			}
		else
			{
			if ( ips <= ipe )
				{
				poly = append_line_pdir(poly, seg->line, ips, ipe, fwd);
				}
			else
				{
				poly = append_line_pdir(poly, seg->line, 0,   ipe, fwd);
				poly = append_line_pdir(poly, seg->line, ips, ipl, fwd);
				}
			}
		}

	/* Close the line */
	if ( NotNull(poly) )
		{
		/* (void) reverse_line(poly); */
		pos[X] = poly->points[0][X];
		pos[Y] = poly->points[0][Y];
		(void) add_point_to_line(poly, pos);
		}

	/* Return the line */
	return poly;
	}

/***********************************************************************
*                                                                      *
*      o u t l i n e _ f r o m _ s u b v i s                           *
*                                                                      *
***********************************************************************/
/***********************************************************************/
/**	Build a line from the visible outline of a subarea.
 *
 * @param[in]	sub		SUBAREA structure.
 * @return	LINE structure created from visible outline of a SUBAREA.
 ***********************************************************************/

LINE	outline_from_subvis

	(
	SUBVIS		subvis		/* Visible outline for SUBAREA structure */
	)

	{
	int			ii, ips, ipe, ipl;
	LOGICAL		fwd;
	SEGMENT		seg;
	POINT		pos;
	LINE		poly = NullLine;

	/* Return immediately if no subvis or no lines in subvis */
	if ( IsNull(subvis) )      return NullLine;
	if ( subvis->numvis <= 0 ) return NullLine;

	/* Build a single polygon */
	for ( ii=0; ii<subvis->numvis; ii++ )
		{
		seg  = subvis->segvis[ii];
		fwd  = (LOGICAL) seg_forward(seg);
		ips  = seg->ips;
		ipe  = seg->ipe;
		ipl  = seg->line->numpts - 1;
		if ( fwd )
			{
			if ( ips <= ipe )
				{
				poly = append_line_pdir(poly, seg->line, ips, ipe, fwd);
				}
			else
				{
				poly = append_line_pdir(poly, seg->line, ips, ipl, fwd);
				poly = append_line_pdir(poly, seg->line, 0,   ipe, fwd);
				}
			}
		else
			{
			if ( ips <= ipe )
				{
				poly = append_line_pdir(poly, seg->line, ips, ipe, fwd);
				}
			else
				{
				poly = append_line_pdir(poly, seg->line, 0,   ipe, fwd);
				poly = append_line_pdir(poly, seg->line, ips, ipl, fwd);
				}
			}
		}

	/* Close the line */
	if ( NotNull(poly) )
		{
		/* (void) reverse_line(poly); */
		pos[X] = poly->points[0][X];
		pos[Y] = poly->points[0][Y];
		(void) add_point_to_line(poly, pos);
		}

	/* Return the line */
	return poly;
	}

/***********************************************************************
*                                                                      *
*      r e p l a c e _ a r e a _ b o u n d a r y                       *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Replace the boundary of the given area, while retaining any divides
 * and/or holes that are still consistent with the new boundary.
 *
 * @param[in]	area		area to affect.
 * @param[in]	boundary	line to define boundary.
 * @param[out]	*status		What went wrong?
 * @return True if successful.
 ***********************************************************************/
LOGICAL	replace_area_boundary

	(
	AREA	area,		/* given area */
	LINE	boundary,	/* line to define boundary */
	DIVSTAT	*status		/* What went wrong? */
	)

	{
	LOGICAL	ok=TRUE;
	int		numhole, numdiv, *subids, ihole, idiv, isub;
	LINE	*holes, *divlines, olddiv, newdiv;
	LOGICAL	cwarea, cwline, resetnext;
	SUBAREA	*subareas, dsub, lsub, rsub, lnew, rnew;
	DIVSTAT	dstat;

	SUBAREA	*llist=NullSubAreaList, *rlist=NullSubAreaList;

#	ifdef DEBUG_SUBIDS
	char	xbuf[10], tbuf[240];
#	endif /* DEBUG_SUBIDS */

	/* Setup default returns */
	if (status) *status = DivOK;

	/* Do nothing if area not there */
	if (!area) return FALSE;

	/* Retain hole information from the area */
	numhole = 0;
	holes   = NullLineList;
	if (area->bound)
		{
		numhole = area->bound->numhole;
		holes   = area->bound->holes;
		}

	/* Remove hole information from the area */
	if (numhole > 0)
		{
		area->bound->numhole = 0;
		area->bound->holes   = NullLineList;
		}

	/* Retain division information from the area */
	numdiv   = area->numdiv;
	divlines = area->divlines;
	subids   = area->subids;
	subareas = area->subareas;

#	ifdef DEBUG_SUBIDS
	if (numdiv > 0)
		{
		(void) strcpy(tbuf, "");
		for (idiv=0; idiv<numdiv; idiv++)
			{
			(void) sprintf(xbuf, " %d", subids[idiv]);
			(void) strcat(tbuf, xbuf);
			}
		pr_diag("replace_area_boundary()", "Start subids: %d - %s\n",
				numdiv, tbuf);
		}
#	endif /* DEBUG_SUBIDS */

	/* Determine parameters for dividing lines (if required) */
	if (numdiv > 0)
		{

		/* Figure out relationship between dividing lines and subareas */
		/* Note that these will be pointers to area->subareas */
		llist = INITMEM(SUBAREA, numdiv);
		rlist = INITMEM(SUBAREA, numdiv);
		for (idiv=0; idiv<numdiv; idiv++)
			{
			(void) adjacent_subareas(area, idiv, llist+idiv, rlist+idiv);
			}

		/* Remove division information from the area */
		area->numdiv   = 0;
		area->divlines = NullLineList;
		area->subids   = NullInt;
		area->subareas = NullSubAreaList;
		}

	/* Determine the direction of the original boundary */
	(void) area_properties(area, &cwarea, NullFloat, NullFloat);

	/* Clean up the area and create an empty bound if necessary */
	empty_area(area);
	if (!area->bound) area->bound = create_bound();

	/* Define the boundary line ... in the same direction as the original! */
	(void) close_line(boundary);
	(void) line_properties(boundary, NullLogical, &cwline,
			NullFloat, NullFloat);
	if ((cwarea && !cwline) || (!cwarea && cwline))
		{
		(void) pr_error("Areas", "Reversing area boundary!\n");
		reverse_line(boundary);
		}
	define_bound_boundary(area->bound, boundary);

	/* Add back all holes that are inside the area */
	if (numhole > 0)
		{
		for (ihole=0; ihole<numhole; ihole++)
			{
			if (hole_inside_area(area, holes[ihole]))
				{
				add_bound_hole(area->bound, holes[ihole]);
				holes[ihole] = NullLine;
				}
			}
		}

	/* Now redivide the area in the same order (if required) */
	if (numdiv > 0)
		{
		build_area_subareas(area);
		for (idiv=0; idiv<numdiv; idiv++)
			{

			/* Divide the same subarea as in the original */
			isub   = subids[idiv];
			olddiv = divlines[idiv];

#			ifdef DEBUG_SUBIDS
			pr_diag("replace_area_boundary()", "Now on subid: %d - %d\n",
					idiv, isub);
#			endif /* DEBUG_SUBIDS */

			/* Skip missing subareas */
			if (isub < 0) continue;
			if (isub > area->numdiv+1)
				{
				ok = FALSE;
				if (status && *status == DivOK) *status = DivNoSub;
				continue;
				}

			/* Set subarea to divide */
			dsub = area->subareas[isub];

			/* Prepare the dividing line for this subarea */
			/* (Extend and/or crop to subarea boundary) */
			newdiv = prepare_area_divline(area, dsub, olddiv, &dstat);

			/* This dividing line is outside its subarea */
			if (IsNull(newdiv))
				{

				/* Need to reset the subarea attributes for the subaraa */
				switch(dstat)
					{
					case DivAreaLeft:
						lsub = llist[idiv];
						define_subarea_value(dsub,
								lsub->subelem, lsub->value, lsub->label);
						define_subarea_attribs(dsub, lsub->attrib);
						resetnext = !cwarea;
						if (status && *status == DivOK) *status = DivNoSub;
						break;
					case DivAreaRight:
						rsub = rlist[idiv];
						define_subarea_value(dsub,
								rsub->subelem, rsub->value, rsub->label);
						define_subarea_attribs(dsub, rsub->attrib);
						resetnext = cwarea;
						if (status && *status == DivOK) *status = DivNoSub;
						break;
					default:
						if (status && *status == DivOK) *status = DivNoSub;
						continue;
					}

				/* Also need to shuffle the dividing lines and subareas  */
				/*  to avoid additional divides that can no longer exist */
				(void) reset_area_subids(numdiv, subids, idiv, area->numdiv+1,
										 resetnext);

				ok = FALSE;
				continue;
				}

			/* Divide the area */
			if (!divide_area(area, dsub, newdiv, &lnew, &rnew, &dstat))
				{
				destroy_line(newdiv);
				ok = FALSE;
				if (status && *status == DivOK) *status = DivNoSub;
				continue;
				}

			/* Set the attributes for the new subareas */
			lsub = llist[idiv];
			rsub = rlist[idiv];
			define_subarea_value(lnew, lsub->subelem, lsub->value, lsub->label);
			define_subarea_attribs(lnew, lsub->attrib);
			define_subarea_value(rnew, rsub->subelem, rsub->value, rsub->label);
			define_subarea_attribs(rnew, rsub->attrib);
			}

#		ifdef DEBUG_SUBIDS
		(void) strcpy(tbuf, "");
		for (idiv=0; idiv<numdiv; idiv++)
			{
			(void) sprintf(xbuf, " %d", subids[idiv]);
			(void) strcat(tbuf, xbuf);
			}
		pr_diag("replace_area_boundary()", "End subids: %d - %s\n",
				numdiv, tbuf);
#		endif /* DEBUG_SUBIDS */

		/* Clean up holes */
		for (ihole=0; ihole<numhole; ihole++)
			(void) destroy_line(holes[ihole]);
		FREEMEM(holes);

		/* Clean up dividing lines and subids */
		FREEMEM(llist);
		FREEMEM(rlist);
		for (isub=0; isub<numdiv+1; isub++)
			(void) destroy_subarea(subareas[isub]);
		FREEMEM(subareas);
		FREEMEM(subids);
		for (idiv=0; idiv<numdiv; idiv++)
			(void) destroy_line(divlines[idiv]);
		FREEMEM(divlines);
		}

	/* Reset default area attributes from first subarea */
	if (numdiv > 0)
		{
		pr_diag("replace_area_boundary()", "Reset area attribs\n");
		if (pr_level(NULL, 5))
			{
			(void) debug_attrib_list("replace_area_boundary()",
				area->subareas[0]->attrib);
			}
		define_area_attribs(area, area->subareas[0]->attrib);
		}

	/* Redefine visible portions of area */
	area->visready = FALSE;
	return ok;
	}

/***********************************************************************
*                                                                      *
*      r e p l a c e _ a r e a _ h o l e                               *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Replace the specified hole of the given area, while retaining all
 * other holes that are still consistent with the new hole.
 *
 * @param[in]	area	area to affect.
 * @param[in]	chole	which hole to change.
 * @param[out]	modhole	new hole.
 * @param[out]	*status	what went wrong?
 * @return True if successful.
 ***********************************************************************/
LOGICAL	replace_area_hole

	(
	AREA	area,		/* given area */
	int		chole,		/* which hole to change */
	LINE	modhole,	/* new hole */
	DIVSTAT	*status		/* What went wrong? */
	)

	{
	LOGICAL	ok=TRUE;
	int		numhole, nsub, isub, isv;
	LINE	*holes, hole, xhole;
	SUBAREA	sub;
	AREA	xarea;

	/* Setup default returns */
	if (status) *status = DivOK;

	/* Do nothing if area not there */
	if (!area) return FALSE;

	/* Do nothing if hole is not in the list */
	numhole = area->bound->numhole;
	holes   = area->bound->holes;
	if (chole < 0)        return FALSE;
	if (chole >= numhole) return FALSE;

	/* Hole will be replaced                                          */
	/*  ... so remove visible portions of area that might refer to it */
	if (NotNull(area->subareas))
		{
		nsub = area->numdiv + 1;
		for (isub=0; isub<nsub; isub++)
			{
			sub = area->subareas[isub];
			if (IsNull(sub)) continue;
			if (sub->nsubvis > 0)
				{
				for (isv=0; isv<sub->nsubvis; isv++)
					(void) destroy_subvis(sub->subvis[isv]);
				FREEMEM(sub->subvis);
				sub->nsubvis = 0;
				}
			if (sub->numhole > 0)
				{
				FREEMEM(sub->holes);
				sub->numhole = 0;
				}
			sub->visready = FALSE;
			}
		}

	/* Remove hole if nothing to replace */
	hole = holes[chole];
	if (IsNull(modhole) || modhole->numpts < 3)
		{
		remove_area_hole(area, hole);
		if (status) *status = DivNoInfo;
		return FALSE;
		}

	/* Remove the hole if not inside area */
	/* Note that we must check a copy of the area without the original hole! */
	xarea = copy_area(area, FALSE);
	xhole = xarea->bound->holes[chole];
	remove_area_hole(xarea, xhole);
	if (!hole_inside_area(xarea, modhole))
		{
		xarea = destroy_area(xarea);
		remove_area_hole(area, hole);
		if (status) *status = DivNoInfo;
		return FALSE;
		}
	xarea = destroy_area(xarea);

	/* Replace the desired hole */
	empty_line(hole);
	hole = append_line(hole, modhole);
	close_line(hole);

	/* Redefine visible portions of area */
	area->visready = FALSE;
	return ok;
	}

/***********************************************************************
*                                                                      *
*      r e p l a c e _ a r e a _ d i v i d e                           *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Replace the specified dividing line of the given area, while
 * retaining all other divides and/or holes that are still consistent
 * with the new dividing line.
 *
 * @param[in]	area	area to affect.
 * @param[in]	cdiv	which dividing line to change.
 * @param[out]	moddiv	new dividing line.
 * @param[out]	*status	what went wrong?
 * @return True if successful.
 ***********************************************************************/
LOGICAL	replace_area_divide

	(
	AREA	area,		/* given area */
	int		cdiv,		/* which dividing line to change */
	LINE	moddiv,		/* new dividing line */
	DIVSTAT	*status		/* What went wrong? */
	)

	{
	LOGICAL	ok=TRUE;
	int		numdiv, *subids, idiv, isub;
	LINE	*divlines, olddiv, newdiv;
	LOGICAL	cwarea, resetnext;
	SUBAREA	*subareas, dsub, lsub, rsub, lnew, rnew;
	DIVSTAT	dstat;

	SUBAREA	*llist=NullSubAreaList, *rlist=NullSubAreaList;

#	ifdef DEBUG_SUBIDS
	char	xbuf[10], tbuf[240];
#	endif /* DEBUG_SUBIDS */

	/* Setup default returns */
	if (status) *status = DivOK;

	/* Do nothing if area not there */
	if (!area) return FALSE;

	/* Retain division information from the area */
	numdiv   = area->numdiv;
	divlines = area->divlines;
	subids   = area->subids;
	subareas = area->subareas;
	if (cdiv < 0)       return FALSE;
	if (cdiv >= numdiv) return FALSE;
	if (numdiv <= 0)    return FALSE;

#	ifdef DEBUG_SUBIDS
	(void) strcpy(tbuf, "");
	for (idiv=0; idiv<numdiv; idiv++)
		{
		(void) sprintf(xbuf, " %d", subids[idiv]);
		(void) strcat(tbuf, xbuf);
		}
	pr_diag("replace_area_divide()", "For cdiv: %d  Start subids: %d - %s\n",
			cdiv, numdiv, tbuf);
#	endif /* DEBUG_SUBIDS */

	/* Figure out relationship between dividing lines and subareas */
	/* Note that these will be pointers to area->subareas */
	llist = INITMEM(SUBAREA, numdiv);
	rlist = INITMEM(SUBAREA, numdiv);
	for (idiv=0; idiv<numdiv; idiv++)
		{
		(void) adjacent_subareas(area, idiv, llist+idiv, rlist+idiv);
		}

	/* Remove division information from the area */
	area->numdiv   = 0;
	area->divlines = NullLineList;
	area->subids   = NullInt;
	area->subareas = NullSubAreaList;

	/* Determine the direction of the original boundary */
	(void) area_properties(area, &cwarea, NullFloat, NullFloat);

	/* Now redivide the area in the same order */
	build_area_subareas(area);
	for (idiv=0; idiv<numdiv; idiv++)
		{

		/* Divide the same subarea as in the original */
		isub = subids[idiv];
		if (idiv == cdiv) olddiv = moddiv;
		else              olddiv = divlines[idiv];

#		ifdef DEBUG_SUBIDS
		pr_diag("replace_area_divide()", "Now on subid: %d - %d\n",
				idiv, isub);
#		endif /* DEBUG_SUBIDS */

		/* Skip missing subareas */
		if (isub < 0) continue;
		if (isub > area->numdiv+1)
			{
			ok = FALSE;
			if (status && *status == DivOK) *status = DivNoSub;
			continue;
			}

		/* Set subarea to divide */
		dsub = area->subareas[isub];

		/* Prepare the dividing line for this subarea */
		/* (Extend and/or crop to subarea boundary) */
		newdiv = prepare_area_divline(area, dsub, olddiv, &dstat);

		/* This dividing line is outside its subarea */
		if (IsNull(newdiv))
			{

			/* Need to reset the subarea attributes for the subaraa */
			switch(dstat)
				{
				case DivAreaLeft:
					lsub = llist[idiv];
					define_subarea_value(dsub,
							lsub->subelem, lsub->value, lsub->label);
					define_subarea_attribs(dsub, lsub->attrib);
					resetnext = !cwarea;
					break;
				case DivAreaRight:
					rsub = rlist[idiv];
					define_subarea_value(dsub,
							rsub->subelem, rsub->value, rsub->label);
					define_subarea_attribs(dsub, rsub->attrib);
					resetnext = cwarea;
					break;
				default:
					if (status && *status == DivOK) *status = DivNoSub;
					continue;
				}

			/* Also need to shuffle the dividing lines and subareas  */
			/*  to avoid additional divides that can no longer exist */
			(void) reset_area_subids(numdiv, subids, idiv, area->numdiv+1,
									 resetnext);

			/* Reset the status if this is the input dividing line */
			if (status && idiv == cdiv)          *status = dstat;
			else if (status && *status == DivOK) *status = DivNoSub;

			ok = FALSE;
			continue;
			}

		/* Divide the area */
		if (!divide_area(area, dsub, newdiv, &lnew, &rnew, &dstat))
			{
			destroy_line(newdiv);
			ok = FALSE;
			if (status && *status == DivOK) *status = DivNoSub;
			continue;
			}

		/* Set the attributes for the new subareas */
		lsub = llist[idiv];
		rsub = rlist[idiv];
		define_subarea_value(lnew, lsub->subelem, lsub->value, lsub->label);
		define_subarea_attribs(lnew, lsub->attrib);
		define_subarea_value(rnew, rsub->subelem, rsub->value, rsub->label);
		define_subarea_attribs(rnew, rsub->attrib);
		}

#	ifdef DEBUG_SUBIDS
	(void) strcpy(tbuf, "");
	for (idiv=0; idiv<numdiv; idiv++)
		{
		(void) sprintf(xbuf, " %d", subids[idiv]);
		(void) strcat(tbuf, xbuf);
		}
	pr_diag("replace_area_divide()", "For cdiv: %d  End subids: %d - %s\n",
			cdiv, numdiv, tbuf);
#	endif /* DEBUG_SUBIDS */

	/* Clean up dividing lines and subareas */
	FREEMEM(llist);
	FREEMEM(rlist);
	for (isub=0; isub<numdiv+1; isub++)
		(void) destroy_subarea(subareas[isub]);
	FREEMEM(subareas);
	FREEMEM(subids);
	for (idiv=0; idiv<numdiv; idiv++)
		(void) destroy_line(divlines[idiv]);
	FREEMEM(divlines);

	/* Reset default area attributes from first subarea */
	if (numdiv > 0)
		{
		pr_diag("replace_area_divide()", "Reset area attribs\n");
		if (pr_level(NULL, 5))
			{
			(void) debug_attrib_list("replace_area_divide()",
				area->subareas[0]->attrib);
			}
		define_area_attribs(area, area->subareas[0]->attrib);
		}

	/* Redefine visible portions of area */
	area->visready = FALSE;
	return ok;
	}

/***********************************************************************
*                                                                      *
*      r e m o v e _ a r e a _ d i v i d e                             *
*                                                                      *
***********************************************************************/
/***********************************************************************/
/**	Remove the specified dividing line of the given area, while
 * retaining all other divides and/or holes that are still consistent
 * with the new divided area.
 *
 * @param[in]	area	area to remove from.
 * @param[in]	cdiv	index of dividing line to remove.
 * @param[in]	right	which side remains?
 * @param[out]	*status	what went wrong.
 * @return True if successful.
 ***********************************************************************/

LOGICAL	remove_area_divide

	(
	AREA	area,		/* given area */
	int		cdiv,		/* which dividing line to remove */
	LOGICAL	right,		/* which side remains? */
	DIVSTAT	*status		/* What went wrong? */
	)

	{
	LOGICAL	ok=TRUE;
	int		numdiv, *subids, idiv, isub;
	LINE	*divlines, olddiv, newdiv;
	LOGICAL	cwarea, nextreset, resetnext;
	SUBAREA	*subareas, dsub, lsub, rsub, lnew, rnew;
	DIVSTAT	dstat;

	SUBAREA	*llist=NullSubAreaList, *rlist=NullSubAreaList;

#	ifdef DEBUG_SUBIDS
	char	xbuf[10], tbuf[240];
#	endif /* DEBUG_SUBIDS */

	/* Setup default returns */
	if (status) *status = DivOK;

	/* Do nothing if area not there */
	if (!area) return FALSE;

	/* Retain division information from the area */
	numdiv   = area->numdiv;
	divlines = area->divlines;
	subids   = area->subids;
	subareas = area->subareas;
	if (cdiv < 0)       return FALSE;
	if (cdiv >= numdiv) return FALSE;
	if (numdiv <= 0)    return FALSE;

#	ifdef DEBUG_SUBIDS
	(void) strcpy(tbuf, "");
	for (idiv=0; idiv<numdiv; idiv++)
		{
		(void) sprintf(xbuf, " %d", subids[idiv]);
		(void) strcat(tbuf, xbuf);
		}
	pr_diag("remove_area_divide()", "For cdiv: %d  Start subids: %d - %s\n",
			cdiv, numdiv, tbuf);
#	endif /* DEBUG_SUBIDS */

	/* Figure out relationship between dividing lines and subareas */
	/* Note that these will be pointers to area->subareas */
	llist = INITMEM(SUBAREA, numdiv);
	rlist = INITMEM(SUBAREA, numdiv);
	for (idiv=0; idiv<numdiv; idiv++)
		{
		(void) adjacent_subareas(area, idiv, llist+idiv, rlist+idiv);
		}

	/* Remove division information from the area */
	area->numdiv   = 0;
	area->divlines = NullLineList;
	area->subids   = NullInt;
	area->subareas = NullSubAreaList;

	/* Determine the direction of the original boundary */
	(void) area_properties(area, &cwarea, NullFloat, NullFloat);

	/* Now redivide the area in the same order */
	build_area_subareas(area);
	for (idiv=0; idiv<numdiv; idiv++)
		{

		/* Divide the same subarea as in the original */
		isub   = subids[idiv];
		olddiv = divlines[idiv];

#		ifdef DEBUG_SUBIDS
		pr_diag("remove_area_divide()", "Now on subid: %d - %d\n",
				idiv, isub);
#		endif /* DEBUG_SUBIDS */

		/* Skip missing subareas */
		if (isub < 0) continue;
		if (isub > area->numdiv+1)
			{
			ok = FALSE;
			if (status && *status == DivOK) *status = DivNoSub;
			continue;
			}

		/* Set subarea to divide */
		dsub = area->subareas[isub];

		/* Special case for dividing line to be removed */
		if (idiv == cdiv)
			{

			/* Need to reset the subarea attributes for the subaraa */
			if (right)
				{
				dstat = DivAreaRight;
				rsub  = rlist[idiv];
				define_subarea_value(dsub,
						rsub->subelem, rsub->value, rsub->label);
				define_subarea_attribs(dsub, rsub->attrib);
				nextreset = cwarea;
				}
			else
				{
				dstat = DivAreaLeft;
				lsub  = llist[idiv];
				define_subarea_value(dsub,
						lsub->subelem, lsub->value, lsub->label);
				define_subarea_attribs(dsub, lsub->attrib);
				nextreset = !cwarea;
				}

			/* Also need to shuffle the dividing lines and subareas  */
			/*  to avoid additional divides that can no longer exist */
			(void) reset_area_subids(numdiv, subids, idiv, area->numdiv+1,
									 nextreset);

			/* Reset the status if this is the input dividing line */
			if (status) *status = dstat;

			ok = FALSE;
			continue;
			}

		/* Prepare the dividing line for this subarea */
		/* (Extend and/or crop to subarea boundary) */
		else
			{
			newdiv = prepare_area_divline(area, dsub, olddiv, &dstat);

			/* This dividing line is outside its subarea */
			if (IsNull(newdiv))
				{

				/* Need to reset the subarea attributes for the subaraa */
				switch(dstat)
					{
					case DivAreaLeft:
						lsub = llist[idiv];
						define_subarea_value(dsub,
								lsub->subelem, lsub->value, lsub->label);
						define_subarea_attribs(dsub, lsub->attrib);
						resetnext = cwarea;
						break;
					case DivAreaRight:
						rsub = rlist[idiv];
						define_subarea_value(dsub,
								rsub->subelem, rsub->value, rsub->label);
						define_subarea_attribs(dsub, rsub->attrib);
						resetnext = !cwarea;
						break;
					default:
						if (status && *status == DivOK) *status = DivNoSub;
						continue;
					}

				/* Also need to shuffle the dividing lines and subareas  */
				/*  to avoid additional divides that can no longer exist */
				(void) reset_area_subids(numdiv, subids, idiv, area->numdiv+1,
										 resetnext);

				/* Reset the status if this is the input dividing line */
				if (status && idiv == cdiv)          *status = dstat;
				else if (status && *status == DivOK) *status = DivNoSub;

				ok = FALSE;
				continue;
				}
			}

		/* Divide the area */
		if (!divide_area(area, dsub, newdiv, &lnew, &rnew, &dstat))
			{
			destroy_line(newdiv);
			ok = FALSE;
			if (status && *status == DivOK) *status = DivNoSub;
			continue;
			}

		/* Set the attributes for the new subareas */
		lsub = llist[idiv];
		rsub = rlist[idiv];
		define_subarea_value(lnew, lsub->subelem, lsub->value, lsub->label);
		define_subarea_attribs(lnew, lsub->attrib);
		define_subarea_value(rnew, rsub->subelem, rsub->value, rsub->label);
		define_subarea_attribs(rnew, rsub->attrib);
		}

#	ifdef DEBUG_SUBIDS
	(void) strcpy(tbuf, "");
	for (idiv=0; idiv<numdiv; idiv++)
		{
		(void) sprintf(xbuf, " %d", subids[idiv]);
		(void) strcat(tbuf, xbuf);
		}
	pr_diag("remove_area_divide()", "For cdiv: %d  End subids: %d - %s\n",
			cdiv, numdiv, tbuf);
#	endif /* DEBUG_SUBIDS */

	/* Clean up dividing lines and subareas */
	FREEMEM(llist);
	FREEMEM(rlist);
	for (isub=0; isub<numdiv+1; isub++)
		(void) destroy_subarea(subareas[isub]);
	FREEMEM(subareas);
	FREEMEM(subids);
	for (idiv=0; idiv<numdiv; idiv++)
		(void) destroy_line(divlines[idiv]);
	FREEMEM(divlines);

	/* Reset default area attributes from first subarea */
	if (numdiv > 0)
		{
		pr_diag("remove_area_divide()", "Reset area attribs\n");
		if (pr_level(NULL, 5))
			{
			(void) debug_attrib_list("remove_area_divide()",
				area->subareas[0]->attrib);
			}
		define_area_attribs(area, area->subareas[0]->attrib);
		}

	/* Redefine visible portions of area */
	area->visready = FALSE;
	return ok;
	}

/***********************************************************************
*                                                                      *
*      p a r t i a l _ a r e a _ r e d i v i d e                       *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Redivide the given area up to the index of the next dividing line.
 *
 * @param[in]	area	area to redivide.
 * @param[in]	cdiv	index of dividing line.
 * @param[out]	*status	what went wrong.
 * @return True if successful.
 ***********************************************************************/
LOGICAL	partial_area_redivide

	(
	AREA	area,		/* given area */
	int		cdiv,		/* index of dividing line */
	DIVSTAT	*status		/* What went wrong? */
	)

	{
	LOGICAL	ok=TRUE;
	int		numdiv, *subids, idiv, isub;
	LINE	*divlines, olddiv, newdiv;
	LOGICAL	cwarea;
	SUBAREA	*subareas, dsub, lsub, rsub, lnew, rnew;
	DIVSTAT	dstat;

	SUBAREA	*llist=NullSubAreaList, *rlist=NullSubAreaList;

#	ifdef DEBUG_SUBIDS
	char	xbuf[10], tbuf[240];
#	endif /* DEBUG_SUBIDS */

	/* Setup default returns */
	if (status) *status = DivOK;

	/* Do nothing if area not there */
	if (!area) return FALSE;

	/* Retain division information from the area */
	numdiv   = area->numdiv;
	divlines = area->divlines;
	subids   = area->subids;
	subareas = area->subareas;
	if (cdiv < 0)       return FALSE;
	if (cdiv >= numdiv) return FALSE;
	if (numdiv <= 0)    return FALSE;

#	ifdef DEBUG_SUBIDS
	(void) strcpy(tbuf, "");
	for (idiv=0; idiv<numdiv; idiv++)
		{
		(void) sprintf(xbuf, " %d", subids[idiv]);
		(void) strcat(tbuf, xbuf);
		}
	pr_diag("partial_area_redivide()", "For cdiv: %d  Subids: %d - %s\n",
			cdiv, numdiv, tbuf);
#	endif /* DEBUG_SUBIDS */

	/* Figure out relationship between dividing lines and subareas */
	/* Note that these will be pointers to area->subareas */
	llist = INITMEM(SUBAREA, numdiv);
	rlist = INITMEM(SUBAREA, numdiv);
	for (idiv=0; idiv<numdiv; idiv++)
		{
		(void) adjacent_subareas(area, idiv, llist+idiv, rlist+idiv);
		}

	/* Remove division information from the area */
	area->numdiv   = 0;
	area->divlines = NullLineList;
	area->subids   = NullInt;
	area->subareas = NullSubAreaList;

	/* Determine the direction of the original boundary */
	(void) area_properties(area, &cwarea, NullFloat, NullFloat);

	/* Now redivide the area in the same order */
	build_area_subareas(area);
	for (idiv=0; idiv<cdiv; idiv++)
		{

		/* Divide the same subarea as in the original */
		isub   = subids[idiv];
		olddiv = divlines[idiv];

		/* Skip missing subareas */
		if (isub < 0) continue;
		if (isub > area->numdiv+1)
			{
			ok = FALSE;
			if (status && *status == DivOK) *status = DivNoSub;
			continue;
			}

		/* Set subarea to divide */
		dsub = area->subareas[isub];

		/* Prepare the dividing line for this subarea */
		/* (Extend and/or crop to subarea boundary) */
		newdiv = prepare_area_divline(area, dsub, olddiv, &dstat);
		if (IsNull(newdiv))
			{
			ok = FALSE;
			if (status && *status == DivOK) *status = DivNoSub;
			continue;
			}

		/* Divide the area */
		if (!divide_area(area, dsub, newdiv, &lnew, &rnew, &dstat))
			{
			destroy_line(newdiv);
			ok = FALSE;
			if (status && *status == DivOK) *status = DivNoSub;
			continue;
			}

		/* Set the attributes for the new subareas */
		lsub = llist[idiv];
		rsub = rlist[idiv];
		define_subarea_value(lnew, lsub->subelem, lsub->value, lsub->label);
		define_subarea_attribs(lnew, lsub->attrib);
		define_subarea_value(rnew, rsub->subelem, rsub->value, rsub->label);
		define_subarea_attribs(rnew, rsub->attrib);
		}

	/* Clean up dividing lines and subareas */
	FREEMEM(llist);
	FREEMEM(rlist);
	for (isub=0; isub<numdiv+1; isub++)
		(void) destroy_subarea(subareas[isub]);
	FREEMEM(subareas);
	FREEMEM(subids);
	for (idiv=0; idiv<numdiv; idiv++)
		(void) destroy_line(divlines[idiv]);
	FREEMEM(divlines);

	/* Redefine visible portions of area */
	area->visready = FALSE;
	return ok;
	}

/***********************************************************************
*                                                                      *
*      d i v i d e _ a r e a                                           *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Divide the given subarea of the given area using the given line.
 *
 * @param[in]	area	area containing subarea.
 * @param[in]	sub		subarea to be divided.
 * @param[in]	divl	dividing line
 * @param[out]	lsub	subarea created to left of dividing line.
 * @param[out]	rsub	subarea created to right of dividing line.
 * @param[out]	*status	what went wrong.
 * @return True when successful.
 ***********************************************************************/
LOGICAL	divide_area

	(
	AREA	area,	/* area containing subarea */
	SUBAREA	sub,	/* subarea to be divided */
	LINE	divl,	/* dividing line */
	SUBAREA	*lsub,	/* subarea created to left of dividing line */
	SUBAREA	*rsub,	/* subarea created to right of dividing line */
	DIVSTAT	*status	/* What went wrong? */
	)

	{
	SUBAREA	fsnew, bsnew;
	SEGMENT	seg, sseg, sfseg, sbseg, eseg, efseg, ebseg, xseg;
	POINT	*dpts;
	int		ndiv, isub;
	int		npd, ipds, ipde;
	int		isseg, isspan, nspts, psips, xsspan;
	int		ieseg, iespan, nepts, peips, xespan;
	int		iseg, ixseg, ixspan;
	int		numf, numb, totf, totb, num, ii;
	LOGICAL	clockwise, sfwd, scls, efwd, ecls, xfwd, cwfs, cwbs;

	/* Setup default returns */
	if (lsub)   *lsub   = NullSubArea;
	if (rsub)   *rsub   = NullSubArea;
	if (status) *status = DivNoInfo;

	/* Do nothing if no info */
	if (!area) return FALSE;
	if (!divl) return FALSE;

	/* Check for dividing lines that are too short */
	if (divl->numpts < 2)
		{
		if (status) *status = DivTooShort;
		return FALSE;
		}

	/* Make sure the subarea is part of the given area */
	if (NotNull(sub))
		{
		isub = which_area_subarea(area, sub);
		if (isub < 0)
			{
			if (status) *status = DivNotMySub;
			return FALSE;
			}
		}

	/* If no subarea, only permit an area with no divides yet */
	/* >>> later just find which one contains the point */
	else
		{
		if (area->numdiv > 0)
			{
			if (status) *status = DivNoSub;
			return FALSE;
			}
		build_area_subareas(area);
		isub = 0;
		sub  = area->subareas[isub];
		}

	/* Make sure we have an acceptable subarea */
	if (IsNull(sub) || sub->numseg < 1)
		{
		if (status) *status = DivNoSub;
		return FALSE;
		}

#	ifdef DEBUG_DIVIDE
	pr_diag("divide_area()", "Original subarea\n");
	for (iseg=0; iseg<sub->numseg; iseg++)
		{
		seg = sub->segments[iseg];
		if (seg_forward(seg))
			pr_diag("divide_area()",
				"  Segment %d: %X %d to %d (0-%d)\n",
				iseg, seg->line, seg->ips, seg->ipe, seg->line->numpts-1);
		else
			pr_diag("divide_area()",
				"  Segment %d: %X %d to %d (0-%d)\n",
				iseg, seg->line, seg->ipe, seg->ips, seg->line->numpts-1);
		}
#	endif /* DEBUG_DIVIDE */

	/* Get ready */
	dpts = divl->points;
	npd  = divl->numpts;
	ipds = 0;
	ipde = npd - 1;
	subarea_properties(sub, &clockwise, NullFloat, NullFloat);

	/* Find out where the start of dividing line intersects the subarea */
	subarea_test_point(sub, dpts[ipds], NullFloat, NullPoint, &isseg, &isspan,
					   NullChar);
	sseg  = sub->segments[isseg];
	sfwd  = seg_forward(sseg);
	scls  = line_closed(sseg->line);
	nspts  = sseg->line->numpts;
	psips  = sseg->ips - 1; if (scls && psips < 0)       psips  += (nspts-1);
	xsspan = isspan + 1;    if (scls && xsspan >= nspts) xsspan -= (nspts-1);

	/* If the intersection is at the endpoint of a segment  */
	/*  ... find the endpoint of the adjoining segment too! */
	ixseg  = isseg;
	ixspan = isspan;
	if ((sfwd && isspan == sseg->ipe) || (!sfwd && isspan == sseg->ips))
		{
#		ifdef DEBUG_DIVIDE
		if (sub->numseg > 1)
			pr_diag("divide_area()",
				"Intersection of start segment at endpoint of next segment!\n");
#		endif /* DEBUG_DIVIDE */
		ixseg++;
		if (ixseg >= sub->numseg) ixseg = 0;
		xseg = sub->segments[ixseg];
		xfwd = seg_forward(xseg);
		if (xfwd) ixspan = xseg->ips;
		else      ixspan = xseg->ipe;
		}
	else if ((sfwd && isspan == sseg->ips) || (!sfwd && isspan == sseg->ipe))
		{
#		ifdef DEBUG_DIVIDE
		if (sub->numseg > 1)
			pr_diag("divide_area()",
				"Intersection of start segment at endpoint of previous segment!\n");
#		endif /* DEBUG_DIVIDE */
		ixseg--;
		if (ixseg < 0) ixseg = sub->numseg-1;
		xseg = sub->segments[ixseg];
		xfwd = seg_forward(xseg);
		if (xfwd) ixspan = xseg->ipe;
		else      ixspan = xseg->ips;
		}

	/* Find out where the end of dividing line intersects the subarea */
	subarea_test_point(sub, dpts[ipde], NullFloat, NullPoint, &ieseg, &iespan,
					   NullChar);
	eseg = sub->segments[ieseg];
	efwd = seg_forward(eseg);
	ecls  = line_closed(eseg->line);
	nepts  = eseg->line->numpts;
	peips  = eseg->ips - 1; if (ecls && peips < 0)       peips  += (nepts-1);
	xespan = iespan + 1;    if (ecls && xespan >= nepts) xespan -= (nepts-1);

	/* Bail out when both intersections are in the same span of the */
	/* same segment */
	if ((isseg == ieseg && isspan == iespan)
			|| (ixseg == ieseg && ixspan == iespan))
		{
		if (status) *status = subarea_wrt_line(divl, sub);
		return FALSE;
		}

	/* Determine number of segments on the forward and backward sides */
	numf = isseg - ieseg - 1;
	if (numf < 0) numf += sub->numseg;
	numb = ieseg - isseg - 1;
	if (numb < 0) numb += sub->numseg;

	/* Create front and back pieces of the 'start' segment */
	if (sfwd)
		{
		if (isspan == sseg->ipe) sfseg = NullSegment;
		else
			{
			sfseg = copy_segment(sseg, FALSE);
			sfseg->ips = xsspan;
			}

		if (isspan == psips) sbseg = NullSegment;
		else
			{
			sbseg = copy_segment(sseg, FALSE);
			sbseg->ipe = isspan;
			}
		}
	else
		{
		if (isspan == psips) sfseg = NullSegment;
		else
			{
			sfseg = copy_segment(sseg, FALSE);
			sfseg->ipe = isspan;
			}

		if (isspan == sseg->ipe) sbseg = NullSegment;
		else
			{
			sbseg = copy_segment(sseg, FALSE);
			sbseg->ips = xsspan;
			}
		}

	/* Create front and back pieces of the 'end' segment */
	if (isseg != ieseg)
		{
		if (efwd)
			{
			if (iespan == eseg->ipe) efseg = NullSegment;
			else
				{
				efseg = copy_segment(eseg, FALSE);
				efseg->ips = xespan;
				}

			if (iespan == peips) ebseg = NullSegment;
			else
				{
				ebseg = copy_segment(eseg, FALSE);
				ebseg->ipe = iespan;
				}
			}
		else
			{
			if (iespan == peips) efseg = NullSegment;
			else
				{
				efseg = copy_segment(eseg, FALSE);
				efseg->ipe = iespan;
				}

			if (iespan == eseg->ipe) ebseg = NullSegment;
			else
				{
				ebseg = copy_segment(eseg, FALSE);
				ebseg->ips = xespan;
				}
			}
		}

	/* Complications when 'start' and 'end' are the same segment */
	/* Note that dividing lines that intersect the same span  */
	/*  in the same segment have already been rejected above! */
	else
		{
		/* Determine ordering of dividing line intersections */
		if (sfwd)
			{
			if (sseg->ips > sseg->ipe
				&& isspan >= sseg->ips
					&& iespan <= sseg->ipe)
				{
				xfwd = TRUE;

#				ifdef DEBUG_DIVIDE
				pr_diag("divide_area()",
					"Forward divide of forward crossover of 0!\n");
#				endif /* DEBUG_DIVIDE */
				}
			else if (sseg->ips > sseg->ipe
				&& isspan <= sseg->ipe
					&& iespan >= sseg->ips)
				{
				xfwd = FALSE;

#				ifdef DEBUG_DIVIDE
				pr_diag("divide_area()",
					"Reverse divide of forward crossover of 0!\n");
#				endif /* DEBUG_DIVIDE */
				}
			else
				xfwd = (isspan < iespan)? TRUE: FALSE;
			}
		else
			{
			if (sseg->ips < sseg->ipe
				&& isspan <= sseg->ips
					&& iespan >= sseg->ipe)
				{
				xfwd = TRUE;

#				ifdef DEBUG_DIVIDE
				pr_diag("divide_area()",
					"Forward divide of reverse crossover of 0!\n");
#				endif /* DEBUG_DIVIDE */
				}
			else if (sseg->ips < sseg->ipe
				&& isspan >= sseg->ipe
					&& iespan <= sseg->ips)
				{
				xfwd = FALSE;

#				ifdef DEBUG_DIVIDE
				pr_diag("divide_area()",
					"Reverse divide of reverse crossover of 0!\n");
#				endif /* DEBUG_DIVIDE */
				}
			else
				xfwd = (isspan > iespan)? TRUE: FALSE;
			}

		/* Reset front and back pieces of 'start' and 'end' segments */
		if (sfwd && xfwd)
			{
#			ifdef DEBUG_DIVIDE
			pr_diag("divide_area()",
				"In sfwd && xfwd with iespan: %d  sseg->ipe: %d  xespan: %d\n",
				iespan, sseg->ipe, xespan);
			pr_diag("divide_area()",
				"In sfwd && xfwd with sfseg: %X  peips: %d  isspan: %d  xsspan: %d\n",
				sfseg, peips, isspan, xsspan);
#			endif /* DEBUG_DIVIDE */

			if (iespan == sseg->ipe) efseg = NullSegment;
			else
				{
				efseg = copy_segment(sseg, FALSE);
				efseg->ips = xespan;
				}

			numb  = 0;
			ebseg = NullSegment;
			if (sfseg) sfseg->ipe = iespan;

			/* >>>>>
			if (sfseg)
				{
				if (iespan == peips) sfseg = destroy_segment(sfseg);
				else                 sfseg->ipe = iespan;
				}
			<<<<< */

			/* Later: could join ef+sb if only 1 segment */
			}

		else if (sfwd)
			{
#			ifdef DEBUG_DIVIDE
			pr_diag("divide_area()",
				"In sfwd && !xfwd with sbseg: %X  iespan: %d  sseg->ipe: %d\n",
				sbseg, iespan, sseg->ipe);
			pr_diag("divide_area()",
				"In sfwd && !xfwd with xespan: %d  peips: %d  isspan: %d  xsspan: %d\n",
				xespan, peips, isspan, xsspan);
#			endif /* DEBUG_DIVIDE */

			numf  = 0;
			if (sbseg)
				{
				efseg = sbseg;
				efseg->ips = xespan;
				sbseg = NullSegment;
				}
			else
				{
				efseg = copy_segment(sseg, FALSE);
				efseg->ips = xespan;
				efseg->ipe = isspan;
				}

			/* >>>>>
			efseg = sbseg;
			sbseg = NullSegment;
			if (efseg)
				{
				if (iespan == sseg->ipe) efseg = destroy_segment(efseg);
				else                     efseg->ips = xespan;
				}
			<<<<< */

			if (iespan == peips) ebseg = NullSegment;
			else
				{
				ebseg = copy_segment(sseg, FALSE);
				ebseg->ipe = iespan;
				}

			/* Later: could join eb+sf if only 1 segment */
			}

		else if (!sfwd && xfwd)
			{
#			ifdef DEBUG_DIVIDE
			pr_diag("divide_area()",
				"In !sfwd && xfwd with iespan: %d  peips: %d  sfseg: %X\n",
				iespan, peips, sfseg);
			pr_diag("divide_area()",
				"In !sfwd && xfwd sseg->ipe: %d  xespan: %d  isspan: %d  xsspan: %d\n",
				sseg->ipe, xespan, isspan, xsspan);
#			endif /* DEBUG_DIVIDE */

			if (iespan == peips) efseg = NullSegment;
			else
				{
				efseg = copy_segment(sseg, FALSE);
				efseg->ipe = iespan;
				}

			numb  = 0;
			ebseg = NullSegment;
			if (sfseg) sfseg->ips = xespan;

			/* >>>>>
			if (sfseg)
				{
				if (iespan == sseg->ipe) sfseg = destroy_segment(sfseg);
				else                     sfseg->ips = xespan;
				}
			<<<<< */

			/* Later: could join ef+sb if only 1 segment */
			}

		else
			{
#			ifdef DEBUG_DIVIDE
			pr_diag("divide_area()",
				"In !sfwd && !xfwd with sbseg: %X  iespan: %d  peips: %d\n",
				sbseg, iespan, peips);
			pr_diag("divide_area()",
				"In !sfwd && !xfwd with sseg->ipe: %d  xespan: %d  isspan: %d  xsspan: %d\n",
				sseg->ipe, xespan, isspan, xsspan);
#			endif /* DEBUG_DIVIDE */

			numf  = 0;
			if (sbseg)
				{
				efseg = sbseg;
				efseg->ipe = iespan;
				sbseg = NullSegment;
				}
			else
				{
				efseg = copy_segment(sseg, FALSE);
				efseg->ipe = iespan;
				efseg->ips = xsspan;
				}

			/* >>>>>
			efseg = sbseg;
			sbseg = NullSegment;
			if (efseg)
				{
				if (iespan == peips) efseg = destroy_segment(efseg);
				else                 efseg->ipe = iespan;
				}
			<<<<< */

			if (iespan == sseg->ipe) ebseg = NullSegment;
			else
				{
				ebseg = copy_segment(sseg, FALSE);
				ebseg->ips = xespan;
				}

			/* Later: could join eb+sf if only 1 segment */
			}
		}

#	ifdef DEBUG_DIVIDE
	if (sfwd)
		pr_diag("divide_area()",
			"Start at: %d on segment: %X %d to %d (0-%d)\n",
			isspan, sseg->line, sseg->ips, sseg->ipe, sseg->line->numpts-1);
	else
		pr_diag("divide_area()",
			"Start at: %d on segment: %X %d to %d (0-%d)\n",
			isspan, sseg->line, sseg->ipe, sseg->ips, sseg->line->numpts-1);
	if (efwd)
		pr_diag("divide_area()",
			"End at: %d on segment: %X %d to %d (0-%d)\n",
			iespan, eseg->line, eseg->ips, eseg->ipe, eseg->line->numpts-1);
	else
		pr_diag("divide_area()",
			"End at: %d on segment: %X %d to %d (0-%d)\n",
			iespan, eseg->line, eseg->ipe, eseg->ips, eseg->line->numpts-1);
	if (sfseg)
		{
		if (seg_forward(sfseg))
			pr_diag("divide_area()",
				"Segment sfseg: %X %d to %d (0-%d)\n",
				sfseg->line, sfseg->ips, sfseg->ipe, sfseg->line->numpts-1);
		else
			pr_diag("divide_area()",
				"Segment sfseg: %X %d to %d of (%d-0)\n",
				sfseg->line, sfseg->ipe, sfseg->ips, sfseg->line->numpts-1);
		}
	if (sbseg)
		{
		if (seg_forward(sbseg))
			pr_diag("divide_area()",
				"Segment sbseg: %X %d to %d (0-%d)\n",
				sbseg->line, sbseg->ips, sbseg->ipe, sbseg->line->numpts-1);
		else
			pr_diag("divide_area()",
				"Segment sbseg: %X %d to %d of (%d-0)\n",
				sbseg->line, sbseg->ipe, sbseg->ips, sbseg->line->numpts-1);
		}
	if (efseg)
		{
		if (seg_forward(efseg))
			pr_diag("divide_area()",
				"Segment efseg: %X %d to %d (0-%d)\n",
				efseg->line, efseg->ips, efseg->ipe, efseg->line->numpts-1);
		else
			pr_diag("divide_area()",
				"Segment efseg: %X %d to %d of (%d-0)\n",
				efseg->line, efseg->ipe, efseg->ips, efseg->line->numpts-1);
		}
	if (ebseg)
		{
		if (seg_forward(ebseg))
			pr_diag("divide_area()",
				"Segment ebseg: %X %d to %d (0-%d)\n",
				ebseg->line, ebseg->ips, ebseg->ipe, ebseg->line->numpts-1);
		else
			pr_diag("divide_area()",
				"Segment ebseg: %X %d to %d of (%d-0)\n",
				ebseg->line, ebseg->ipe, ebseg->ips, ebseg->line->numpts-1);
		}
#	endif /* DEBUG_DIVIDE */

	/* Determine total number of segments on the forward/backward sides */
	totf = numf + 1;
	if (efseg) totf++;
	if (sbseg) totf++;
	totb = numb + 1;
	if (sfseg) totb++;
	if (ebseg) totb++;

	/* Bail out if not enough segments on either side! */
	if (totf < 2 || totb < 2)
		{

#		ifdef DEBUG_DIVIDE
		if (totf < 2)
			pr_diag("divide_area()",
				"Not enough segments - totf: %d (efseg/sbseg: %X/%X)\n",
				totf, efseg, sbseg);
		if (totb < 2)
			pr_diag("divide_area()",
				"Not enough segments - totb: %d (sfseg/ebseg: %X/%X)\n",
				totb, sfseg, ebseg);
#		endif /* DEBUG_DIVIDE */

		if (sfseg) sfseg = destroy_segment(sfseg);
		if (sbseg) sbseg = destroy_segment(sbseg);
		if (efseg) efseg = destroy_segment(efseg);
		if (ebseg) ebseg = destroy_segment(ebseg);
		if (status) *status = DivNoSub;
		(void) pr_error("Area.Divide", "Error with segments in area divide!\n");
		return FALSE;
		}

	/* Build subarea on the forward side (extending from start of dividing */
	/* line in the same direction as the subarea segment list) */
	fsnew = copy_subarea(sub, FALSE);
	empty_subarea(fsnew);
	fsnew->numseg = totf;
	fsnew->segments = GETMEM(fsnew->segments, SEGMENT, fsnew->numseg);

	/* Start with dividing line in forward sense */
	num = 0;
	fsnew->segments[num++] = seg = create_segment();
	define_segment(seg, divl, FALSE, ipds, ipde, TRUE);

	/* Add front part of 'end' segment */
	/* Then add segments between 'end' and 'start' */
	/* Then add back part of 'start' segment */
	if (efseg) fsnew->segments[num++] = efseg;
	for (iseg=ieseg, ii=0; ii<numf; ii++)
		{
		iseg = (iseg+1) % sub->numseg;
		fsnew->segments[num++] = copy_segment(sub->segments[iseg], FALSE);
		}
	if (sbseg) fsnew->segments[num++] = sbseg;

	/* Check to make sure all the segments were copied! */
	if ( num != totf )
		{
		(void) pr_error("Area.Divide",
					"Error with number of segments in forward divide!\n");
		}

	/* Build subarea on the backward side (extending from end of dividing */
	/* line in the same direction as the subarea segment list) */
	bsnew = copy_subarea(sub, FALSE);
	empty_subarea(bsnew);
	bsnew->numseg = totb;
	bsnew->segments = GETMEM(bsnew->segments, SEGMENT, bsnew->numseg);

	/* Start with dividing line in backward sense */
	num = 0;
	bsnew->segments[num++] = seg = create_segment();
	define_segment(seg, divl, FALSE, ipds, ipde, FALSE);

	/* Add front part of 'start' segment */
	/* Then add segments between 'start' and 'end' */
	/* Then add back part of 'end' segment */
	if (sfseg) bsnew->segments[num++] = sfseg;
	for (iseg=isseg, ii=0; ii<numb; ii++)
		{
		iseg = (iseg+1) % sub->numseg;
		bsnew->segments[num++] = copy_segment(sub->segments[iseg], FALSE);
		}
	if (ebseg) bsnew->segments[num++] = ebseg;

	/* Check to make sure all the segments were copied! */
	if ( num != totb )
		{
		(void) pr_error("Area.Divide",
					"Error with number of segments in backward divide!\n");
		}

	/* Check the subarea properties for consistency */
	/* Because of the way they were created, they should both be drawn */
	/*  either clockwise or counterclockwise                           */
	subarea_properties(fsnew, &cwfs, NullFloat, NullFloat);
	subarea_properties(bsnew, &cwbs, NullFloat, NullFloat);
	if ( (cwfs && !cwbs) || (!cwfs && cwbs) )
		{
		(void) destroy_subarea(fsnew);
		(void) destroy_subarea(bsnew);
		if (status) *status = DivNoSub;
		(void) pr_error("Area.Divide", "Inconsistent subareas!\n");
		return FALSE;
		}

	/* Now add the dividing line to the area */
	ndiv = ++(area->numdiv);
	area->divlines = GETMEM(area->divlines, LINE, ndiv);
	area->divlines[ndiv-1] = divl;

	/* Identify the subarea that has been divided */
	area->subids = GETMEM(area->subids, int, ndiv);
	area->subids[ndiv-1] = isub;

	/* Now replace the original subarea with the two new ones */
	(void) destroy_subarea(sub);
	area->subareas = GETMEM(area->subareas, SUBAREA, ndiv+1);
	area->subareas[isub] = fsnew;
	area->subareas[ndiv] = bsnew;

#	ifdef DEBUG_DIVIDE
	pr_diag("divide_area()", "Forward subarea\n");
	for (iseg=0; iseg<fsnew->numseg; iseg++)
		{
		seg = fsnew->segments[iseg];
		if (seg_forward(seg))
			pr_diag("divide_area()",
				"  Segment %d: %X %d to %d (0-%d)\n",
				iseg, seg->line, seg->ips, seg->ipe, seg->line->numpts-1);
		else
			pr_diag("divide_area()",
				"  Segment %d: %X %d to %d (0-%d)\n",
				iseg, seg->line, seg->ipe, seg->ips, seg->line->numpts-1);
		}
	pr_diag("divide_area()", "Backward subarea\n");
	for (iseg=0; iseg<bsnew->numseg; iseg++)
		{
		seg = bsnew->segments[iseg];
		if (seg_forward(seg))
			pr_diag("divide_area()",
				"  Segment %d: %X %d to %d (0-%d)\n",
				iseg, seg->line, seg->ips, seg->ipe, seg->line->numpts-1);
		else
			pr_diag("divide_area()",
				"  Segment %d: %X %d to %d (0-%d)\n",
				iseg, seg->line, seg->ipe, seg->ips, seg->line->numpts-1);
		}
#	endif /* DEBUG_DIVIDE */

	/* Redefine visible portions of area */
	area->visready = FALSE;

	/* Now return the new subareas as left and right */
	if (lsub)   *lsub   = (clockwise)? bsnew: fsnew;
	if (rsub)   *rsub   = (clockwise)? fsnew: bsnew;
	if (status) *status = DivOK;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*      p r e p a r e _ a r e a _ d i v l i n e                         *
*                                                                      *
***********************************************************************/
/***********************************************************************/
/**	Prepare the given dividing line so that it can be used to
 * divide the given subarea of the given area.
 *
 * @param[in]	area	area containing subarea.
 * @param[in]	sub		subarea to be divided.
 * @param[in]	divl	dividing line.
 * @param[out]	*status what went wrong?
 * @return	prepared dividing line.
 ***********************************************************************/

LINE	prepare_area_divline

	(
	AREA	area,	/* area containing subarea */
	SUBAREA	sub,	/* subarea to be divided */
	LINE	divl,	/* dividing line */
	DIVSTAT	*status	/* What went wrong? */
	)

	{
	LINE	newdivl;
	int		isub;

	/* Setup default returns */
	if (status) *status = DivNoInfo;

	/* Do nothing if no info */
	if (!area) return NullLine;
	if (!divl) return NullLine;

	/* Make sure the subarea is part of the given area */
	if (sub)
		{
		isub = which_area_subarea(area, sub);
		if (isub < 0)
			{
			if (status) *status = DivNotMySub;
			return NullLine;
			}
		}

	/* If no subarea, only permit an area with no divides yet */
	/* >>> later just find which one contains the point */
	else
		{
		if (area->numdiv > 0)
			{
			if (status) *status = DivNoSub;
			return NullLine;
			}
		build_area_subareas(area);
		isub = 0;
		sub  = area->subareas[isub];
		}

	/* Clip the dividing line to the boundary of the subarea */
	newdivl = clip_divline_to_subarea(sub, divl, TRUE, TRUE, status);
	if (IsNull(newdivl)) return NullLine;

	/* Ensure that dividing line is long enough */
	if (newdivl->numpts < 2)
		{
		(void) destroy_line(newdivl);
		if (status) *status = DivTooShort;
		return NullLine;
		}

	/* A valid dividing line has been drawn */
	if (status) *status = DivOK;
	return newdivl;
	}

/***********************************************************************
*                                                                      *
*     c l i p _ d i v l i n e _ t o _ s u b a r e a                    *
*                                                                      *
***********************************************************************/
/***********************************************************************/
/**	Clip a dividing line to lie within bounds of a given subarea
 *
 * 	@param[in]	sub		subarea to be divided.
 * 	@param[in]	divl	dividing line.
 * 	@param[in]	fwd		begin at start of line.
 * 	@param[in]	nearedge extend line to boundary if near edge?
 * 	@param[out]	*status	what went wrong?
 * 	@return clipped dividing line.
 ***********************************************************************/

LINE	clip_divline_to_subarea

	(
	SUBAREA	sub,		/* subarea to be divided */
	LINE	divl,		/* dividing line */
	LOGICAL	fwd,		/* begin at start of line? */
	LOGICAL	nearedge,	/* extend line to boundary if near edge? */
	DIVSTAT	*status		/* What went wrong? */
	)

	{
	LOGICAL	inside, between;
	int		npd, ip, ipbgn, ipend, jpbgn, jpend, nextra;
	LINE	newdivl;
	POINT	*dpts, spos, epos, mpos;
	float	adist, sdist, edist, tdist, dx, dy;

	/* Setup default returns */
	if (status) *status = DivNoInfo;

	/* Return if no subarea or dividing line */
	if (!sub)  return NullLine;
	if (!divl) return NullLine;

	/* Initialize parameters */
	newdivl = copy_line(divl);
	condense_line(newdivl);
	dpts = newdivl->points;
	npd  = newdivl->numpts;

	/* Check for dividing lines that are too short */
	if (newdivl->numpts < 2)
		{
		if (status) *status = DivTooShort;
		destroy_line(newdivl);
		return NullLine;
		}

	/* Set average length of line spans (for extending dividing line) */
	line_properties(newdivl, NullChar, NullChar, NullFloat, &adist);
	adist /= (float) (npd-1);

	/* Check forwards from start of line */
	if (fwd)
		{

		/* Find first point inside the subarea */
		for (ip=0; ip<npd; ip++)
			{
			(void) subarea_test_point(sub, dpts[ip], NullFloat, NullPoint,
										NullInt, NullInt, &inside);
			if (inside) break;
			}
		ipbgn = ip;

		/* Check if dividing line does not pass through the subarea! */
		if (ip >= npd)
			{
			if (status) *status = subarea_wrt_line(newdivl, sub);
			destroy_line(newdivl);
			return NullLine;
			}

		/* Find last point inside the subarea */
		for (ip=ipbgn+1; ip<npd; ip++)
			{
			(void) subarea_test_point(sub, dpts[ip], NullFloat, NullPoint,
										NullInt, NullInt, &inside);
			if (!inside) break;
			}
		ipend = ip-1;

		/* Check for suspiciously short dividing lines */
		if (ipend-ipbgn+1 < npd/2)
			{
#			ifdef DEBUG_CLIP
			pr_diag("clip_divline_to_subarea()",
				"Too few points from %d point line?  Begin/end: %d %d\n",
				npd, ipbgn, ipend);
#			endif /* DEBUG_CLIP */

			/* Find next point inside the subarea */
			for (ip=ipend+1; ip<npd; ip++)
				{
				(void) subarea_test_point(sub, dpts[ip], NullFloat, NullPoint,
											NullInt, NullInt, &inside);
				if (inside) break;
				}

			/* Reset first point and last point inside subarea */
			if (ip < npd)
				{
				jpbgn = ip;
				for (ip=jpbgn+1; ip<npd; ip++)
					{
					(void) subarea_test_point(sub, dpts[ip], NullFloat,
												NullPoint, NullInt, NullInt,
												&inside);
					if (!inside) break;
					}
				jpend = ip-1;
				if (jpend-jpbgn > ipend-ipbgn)
					{
					ipbgn = jpbgn;
					ipend = jpend;
					}
				}
			}
		}

	/* Check backwards from end of line */
	else
		{

		/* Find first point inside the subarea */
		for (ip=npd-1; ip>=0; ip--)
			{
			(void) subarea_test_point(sub, dpts[ip], NullFloat, NullPoint,
										NullInt, NullInt, &inside);
			if (inside) break;
			}
		ipend = ip;

		/* Check if dividing line does not pass through the subarea! */
		if (ip < 0)
			{
			if (status) *status = subarea_wrt_line(newdivl, sub);
			destroy_line(newdivl);
			return NullLine;
			}

		/* Find last point inside the subarea */
		for (ip=ipend-1; ip>=0; ip--)
			{
			(void) subarea_test_point(sub, dpts[ip], NullFloat, NullPoint,
										NullInt, NullInt, &inside);
			if (!inside) break;
			}
		ipbgn = ip+1;

		/* Check for suspiciously short dividing lines */
		if (ipend-ipbgn+1 < npd/2)
			{
#			ifdef DEBUG_CLIP
			pr_diag("clip_divline_to_subarea()",
				"Too few points from %d point line?  Begin/end: %d %d\n",
				npd, ipbgn, ipend);
#			endif /* DEBUG_CLIP */

			/* Find next point inside the subarea */
			for (ip=ipbgn-1; ip>=0; ip--)
				{
				(void) subarea_test_point(sub, dpts[ip], NullFloat, NullPoint,
											NullInt, NullInt, &inside);
				if (inside) break;
				}

			/* Reset last point and first point inside subarea */
			if (ip >= 0)
				{
				jpend = ip;
				for (ip=jpend-1; ip>=0; ip--)
					{
					(void) subarea_test_point(sub, dpts[ip], NullFloat,
												NullPoint, NullInt, NullInt,
												&inside);
					if (!inside) break;
					}
				jpbgn = ip+1;
				if (jpend-jpbgn > ipend-ipbgn)
					{
					ipbgn = jpbgn;
					ipend = jpend;
					}
				}
			}
		}

	/* Extend the dividing line to the subarea boundaries */
	/*  where only the start point is inside the subarea  */
	if (ipbgn == 0 && ipend == ipbgn)
		{
		ip = ipbgn;
		(void) subarea_sight(sub, dpts[ip], dpts[ip+1], FALSE, NullFloat,
							NullFloat, epos, NullInt, NullInt, NullChar);
		mpos[X] = (dpts[ip][X] + epos[X]) / 2.0;
		mpos[Y] = (dpts[ip][Y] + epos[Y]) / 2.0;
		(void) subarea_sight(sub, mpos, dpts[ip], FALSE, NullFloat,
							NullFloat, spos, NullInt, NullInt, NullChar);
		}

	/* Extend the dividing line to the subarea boundaries */
	/*  where only the end point is inside the subarea    */
	else if (ipbgn == npd-1 && ipend == ipbgn)
		{
		ip = ipbgn;
		(void) subarea_sight(sub, dpts[ip], dpts[ip-1], FALSE, NullFloat,
							NullFloat, spos, NullInt, NullInt, NullChar);
		mpos[X] = (dpts[ip][X] + spos[X]) / 2.0;
		mpos[Y] = (dpts[ip][Y] + spos[Y]) / 2.0;
		(void) subarea_sight(sub, mpos, dpts[ip], FALSE, NullFloat,
							NullFloat, epos, NullInt, NullInt, NullChar);
		}

	/* Extend the dividing line to the subarea boundaries */
	else
		{

		/* Find the intersection for the start point */
		ip = ipbgn;
		if (ip == 0) ip++;
		(void) subarea_sight(sub, dpts[ip], dpts[ip-1], FALSE, &sdist,
							NullFloat, spos, NullInt, NullInt, &between);

		/* Reset the start point intersection for a dividing line */
		/*  that is already very close to the subarea boundary    */
		if (!between && nearedge)
			{
			(void) subarea_test_point(sub, dpts[ipbgn], &tdist, mpos,
										NullInt, NullInt, NullChar);
			if (tdist < sdist*NearEdgeRatio) copy_point(spos, mpos);
			}

		/* Find the intersection for the end point */
		ip = ipend;
		if (ip == npd-1) ip--;
		(void) subarea_sight(sub, dpts[ip], dpts[ip+1], FALSE, &edist,
							NullFloat, epos, NullInt, NullInt, &between);

		/* Reset the end point intersection for a dividing line */
		/*  that is already very close to the subarea boundary  */
		if (!between && nearedge)
			{
			(void) subarea_test_point(sub, dpts[ipend], &tdist, mpos,
										NullInt, NullInt, NullChar);
			if (tdist < edist*NearEdgeRatio) copy_point(epos, mpos);
			}
		}

#	ifdef DEBUG_CLIP
	pr_diag("clip_divline_to_subarea()", "Begin/end: %d %d\n", ipbgn, ipend);
	pr_diag("clip_divline_to_subarea()",
		"Start/End points: %.2f %.2f   %.2f %.2f\n",
		spos[X], spos[Y], epos[X], epos[Y]);
#	endif /* DEBUG_CLIP */

	/* Truncate the start of the dividing line */
	if (ipbgn > 0)
		{
		reverse_line(newdivl);
		newdivl->numpts -= ipbgn;
		reverse_line(newdivl);
		}

	/* Truncate the end of the dividing line */
	if (ipend < npd-1)
		{
		newdivl->numpts -= (npd-1-ipend);
		}

	/* Check whether extra points needed for extending start span */
	dpts   = newdivl->points;
	copy_point(mpos, dpts[0]);
	sdist  = point_dist(spos, mpos);
	nextra = NINT(sdist/adist);
#	ifdef DEBUG_CLIP
	if (nextra > 1)
		{
		pr_diag("clip_divline_to_subarea()",
			"Add %d extra points at start\n", nextra-1);
		pr_diag("clip_divline_to_subarea()",
			"Extend start from point: %.2f %.2f\n", mpos[X], mpos[Y]);
		}
#	endif /* DEBUG_CLIP */

	/* Now add the new start point (and extra points if start span too long) */
	reverse_line(newdivl);
	if (nextra > 1)
		{
		dx = (spos[X] - mpos[X]) / (float) nextra;
		dy = (spos[Y] - mpos[Y]) / (float) nextra;

		for (ip=1; ip<nextra; ip++)
			{
			mpos[X] += dx;
			mpos[Y] += dy;
			add_point_to_line(newdivl, mpos);

#			ifdef DEBUG_CLIP
			pr_diag("clip_divline_to_subarea()",
				"Extra start point: %.2f %.2f\n", mpos[X], mpos[Y]);
#			endif /* DEBUG_CLIP */
			}
		}
	add_point_to_line(newdivl, spos);
	reverse_line(newdivl);

	/* Check whether extra points needed for extending end span */
	dpts   = newdivl->points;
	npd    = newdivl->numpts;
	copy_point(mpos, dpts[npd-1]);
	edist  = point_dist(epos, mpos);
	nextra = NINT(edist/adist);
#	ifdef DEBUG_CLIP
	if (nextra > 1)
		{
		pr_diag("clip_divline_to_subarea()",
			"Add %d extra points at end\n", nextra-1);
		pr_diag("clip_divline_to_subarea()",
			"Extend end from point: %.2f %.2f\n", mpos[X], mpos[Y]);
		}
#	endif /* DEBUG_CLIP */

	/* Now add the new end point (and extra points if end span too long) */
	if (nextra > 1)
		{
		dx = (epos[X] - mpos[X]) / (float) nextra;
		dy = (epos[Y] - mpos[Y]) / (float) nextra;

		for (ip=1; ip<nextra; ip++)
			{
			mpos[X] += dx;
			mpos[Y] += dy;
			add_point_to_line(newdivl, mpos);

#			ifdef DEBUG_CLIP
			pr_diag("clip_divline_to_subarea()",
				"Extra end point: %.2f %.2f\n", mpos[X], mpos[Y]);
#			endif /* DEBUG_CLIP */
			}
		}
	add_point_to_line(newdivl, epos);
	condense_line(newdivl);

	/* Return the dividing line */
	if (status) *status = DivOK;
	return newdivl;
	}

/***********************************************************************
*                                                                      *
*      u n d i v i d e _ a r e a                                       *
*                                                                      *
***********************************************************************/
/***********************************************************************/
/**	Remove the dividing line between the two given subareas.
 *
 * The value of the first subarea is retained.
 *
 * 	@param[in]	area	area that contains the subareas.
 * 	@param[in]	sub1	first subarea.
 * 	@param[in]	sub2	second subarea.
 * 	@return True if successful.
 ***********************************************************************/

LOGICAL	undivide_area

	(
	AREA	area,
	SUBAREA	sub1,
	SUBAREA	sub2
	)

	{
	int		isub1, isub2, idiv, ndiv, nsub, nseg, nseg1, nseg2, iseg, jseg;
	int		dseg1, dseg2;
	int		iseg1s, iseg1e, iseg2s, iseg2e;
	LOGICAL	found, fwd1, fwd2;
	SUBAREA	lsub, rsub, newsub;
	LINE	dline;
	SEGMENT	seg, pseg, newseg;

	if (!area)        return FALSE;
	if (!sub1)        return FALSE;
	if (!sub2)        return FALSE;
	if (sub1 == sub2) return FALSE;

	/* Make sure the subareas belong to the given area */
	isub1 = which_area_subarea(area, sub1);
	isub2 = which_area_subarea(area, sub2);
	if (isub1 < 0) return FALSE;
	if (isub2 < 0) return FALSE;

	/* Make sure the subareas are adjacent */
	ndiv  = area->numdiv;
	found = FALSE;
	for (idiv=ndiv-1; idiv>=0; idiv--)
		{
		(void) adjacent_subareas(area, idiv, &lsub, &rsub);
		if ((lsub==sub1 && rsub==sub2) || (lsub==sub2 && rsub==sub1))
			{
			found = TRUE;
			break;
			}
		}
	if (!found) return FALSE;

	/* For now, can only remove last divide */
	/*  ... to be consistent with function divide_area_draw() */
	if (idiv  < ndiv-1) return FALSE;

	/* Remove the dividing line and the last subarea */
	dline  = area->divlines[ndiv-1];
	nsub   = (isub1 < isub2)? isub1: isub2;
	newsub = copy_subarea(area->subareas[isub1], FALSE);
	empty_subarea(newsub);
	area->divlines[ndiv-1] = NullLine;
	area->subids[ndiv-1]   = -1;
	area->subareas[ndiv]   = NullSubArea;
	area->subareas[nsub]   = newsub;
	area->numdiv = --ndiv;

	/* If this was the last dividing line ... reset the area attributes! */
	/* This will ensure that any subarea attribute changes are retained! */
	if (area->numdiv <= 0) define_area_attribs(area, newsub->attrib);

	/* Find where the dividing line is used in the first subarea */
	/* Note: As long as we are removing the last divide, the dividing line */
	/*       will appear as only one segment in each subarea */
	dseg1 = -1;
	nseg1 = sub1->numseg;
	for (iseg=0; iseg<nseg1; iseg++)
		{
		seg = sub1->segments[iseg];
		if (seg->line == dline)
			{
			fwd1  = seg_forward(seg);
			dseg1 = iseg;
			break;
			}
		}
	if (dseg1 < 0) return FALSE;
	iseg1s = dseg1 + 1;
	iseg1e = dseg1 + nseg1 - 1;

	/* Find where the dividing line is used in the second subarea */
	/* Note: As long as we are removing the last divide, the dividing line */
	/*       will appear as only one segment in each subarea */
	dseg2 = -1;
	nseg2 = sub2->numseg;
	for (iseg=0; iseg<nseg2; iseg++)
		{
		seg = sub2->segments[iseg];
		if (seg->line == dline)
			{
			fwd2  = seg_forward(seg);
			dseg2 = iseg;
			break;
			}
		}
	if (dseg2 < 0) return FALSE;
	iseg2s = dseg2 + 1;
	iseg2e = dseg2 + nseg2 - 1;

	/* Reallocate segment list in joined subarea */
	nseg = nseg1 + nseg2 - 2;
	newsub->numseg   = 0;
	newsub->segments = INITMEM(SEGMENT, nseg);
	for (iseg=0; iseg<nseg; iseg++)
		{
		newsub->segments[iseg] = NullSegment;
		}

	/* Build segment list in joined subarea */
	/* Add segments from first subarea in given order */
	nseg = 0;
	pseg = NullSegment;
	for (iseg=iseg1s; iseg<=iseg1e; iseg++)
		{
		jseg = iseg % nseg1;
		seg  = sub1->segments[jseg];
		sub1->segments[jseg] = NullSegment;

		/* Join contiguous segments */
		if (newseg = join_segments(pseg, seg))
			{
			/* Replace previous segment with joined one */
			newsub->segments[nseg-1] = newseg;
			(void) destroy_segment(pseg);
			(void) destroy_segment(seg);
			pseg = newseg;
			}
		else
			{
			newsub->segments[nseg++] = seg;
			pseg = seg;
			}
		}

	/* Build segment list in joined subarea */
	if ((fwd1&&!fwd2) || (!fwd1&&fwd2))
		{
		/* Add segments from second subarea in given order */
		for (iseg=iseg2s; iseg<=iseg2e; iseg++)
			{
			jseg = iseg % nseg2;
			seg  = sub2->segments[jseg];
			sub2->segments[jseg] = NullSegment;

			/* Join contiguous segments */
			if (newseg = join_segments(pseg, seg))
				{
				/* Replace previous segment with joined one */
				newsub->segments[nseg-1] = newseg;
				(void) destroy_segment(pseg);
				(void) destroy_segment(seg);
				pseg = newseg;
				}
			else
				{
				newsub->segments[nseg++] = seg;
				pseg = seg;
				}
			}
		}
	else
		{
		/* Add segments from second subarea in reverse order */
		for (iseg=iseg2e; iseg<=iseg2s; iseg--)
			{
			jseg = iseg % nseg2;
			seg  = sub2->segments[jseg];
			sub2->segments[jseg] = NullSegment;

			/* Switch direction of segment */
			seg->flags ^= SegForward;

			/* Join contiguous segments */
			if (newseg = join_segments(pseg, seg))
				{
				/* Replace previous segment with joined one */
				newsub->segments[nseg-1] = newseg;
				(void) destroy_segment(pseg);
				(void) destroy_segment(seg);
				pseg = newseg;
				}
			else
				{
				newsub->segments[nseg++] = seg;
				pseg = seg;
				}
			}
		}

	/* Join first and last segment if necessary/possible */
	if (nseg > 1)
		{
		seg = newsub->segments[0];

		/* Join last and first segments if contiguous */
		if (newseg = join_segments(pseg, seg))
			{
			/* Replace first segment with joined one */
			newsub->segments[0] = newseg;
			(void) destroy_segment(pseg);
			(void) destroy_segment(seg);

			/* Get rid of last segment */
			newsub->segments[--nseg] = NullSegment;
			}
		}
	newsub->numseg = nseg;

	/* Redefine visible portions of area */
	area->visready = FALSE;

	/* Destroy dividing line and subareas */
	(void) destroy_line(dline);
	(void) destroy_subarea(sub1);
	(void) destroy_subarea(sub2);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*      a d j a c e n t _ s u b a r e a s                               *
*                                                                      *
***********************************************************************/
/***********************************************************************/
/**	Return the first subarea on either side of the given divide of
 * the given area.
 *
 * 	@param[in]	area	area to check.
 * 	@param[in]	idiv	index of dividing line.
 * 	@param[out]	lsub	area to the left of dividing line.
 * 	@param[out]	rsub	area to the right of dividing line.
 * 	@return True if successful.
 ***********************************************************************/

LOGICAL	adjacent_subareas

	(
	AREA	area,
	int		idiv,
	SUBAREA	*lsub,
	SUBAREA	*rsub
	)

	{
	int		nsub, isub, nseg, iseg, ips, ipe;
	LINE	divl;
	SUBAREA	sub;
	SEGMENT	seg;
	LOGICAL	clockwise, fwd, right;
	SUBAREA	lfound = NullSubArea;
	SUBAREA	rfound = NullSubArea;

	SUBAREA	lextra = NullSubArea;
	SUBAREA	rextra = NullSubArea;

#	ifdef DEBUG_ADJACENT
	STRING	sclock,       scw      = "CW",    sccw      = "CCW";
	STRING	sdirection,   sforward = "Fwd",   sbackward = "Bkwd";
	STRING	sorientation, sright   = "Right", sleft     = "Left";
#	endif /* DEBUG_ADJACENT */

	if (lsub) *lsub = lfound;
	if (rsub) *rsub = rfound;

	if (!area)                return FALSE;
	if (idiv < 0)             return FALSE;
	if (idiv >= area->numdiv) return FALSE;
	if (!lsub && !rsub)       return TRUE;

	divl = area->divlines[idiv];

	/* Find the first two subareas that use this dividing line */
	/* Note that portions of dividing line used must overlap!  */
	nsub = area->numdiv + 1;
	for (isub=0; isub<nsub; isub++)
		{
		sub = area->subareas[isub];
		subarea_properties(sub, &clockwise, NullFloat, NullFloat);

		/* See if this subarea uses the given dividing line */
		nseg = sub->numseg;
		for (iseg=0; iseg<nseg; iseg++)
			{
			seg = sub->segments[iseg];
			if (seg->line != divl) continue;

			/* Yes it does!  See if it's on the right or left */
			fwd   = (LOGICAL) seg_forward(seg);
			right = (LOGICAL) ((clockwise && fwd) || (!clockwise && !fwd));

#			ifdef DEBUG_ADJACENT
			sclock       = (clockwise)? scw:      sccw;
			sdirection   = (fwd)?       sforward: sbackward;
			sorientation = (right)?     sright:   sleft;
			(void) fprintf(stderr,
				"[adjacent_subareas] Subarea (%d): %s  Divide (%d) (%d - %x): %s  Subarea on: %s\n",
					isub, sclock, idiv, iseg, sub->segments[iseg]->line,
					sdirection, sorientation);
#			endif /* DEBUG_ADJACENT */

			/* Keep track */
			if (right)
				{
				if (!rfound)
					{
					rfound = sub;
					ips = seg->ips;
					ipe = seg->ipe;
					}
				else
					{
					/* Ensure that dividing line segments overlap! */
					if ( (ips<seg->ips && ipe<seg->ips)
							|| (ips>seg->ipe && ipe>seg->ipe) )
						{

#						ifdef DEBUG_ADJACENT
						(void) fprintf(stderr,
							"[adjacent_subareas]   No overlap ... ips/ipe: %d %d  ips/ipe: %d %d\n",
								ips, ipe, seg->ips, seg->ipe);
#						endif /* DEBUG_ADJACENT */

						continue;
						}

					/* Found two subareas to right of dividing line! */
					(void) pr_error("Area.Divide",
							"Two subareas to right of dividing line?\n");
					rextra = sub;
					}
				}
			else
				{
				if (!lfound)
					{
					lfound = sub;
					ips = seg->ips;
					ipe = seg->ipe;
					}
				else
					{
					/* Ensure that dividing line segments overlap! */
					if ( (ips<seg->ips && ipe<seg->ips)
							|| (ips>seg->ipe && ipe>seg->ipe) )
						{

#						ifdef DEBUG_ADJACENT
						(void) fprintf(stderr,
							"[adjacent_subareas]   No overlap ... ips/ipe: %d %d  ips/ipe: %d %d\n",
								ips, ipe, seg->ips, seg->ipe);
#						endif /* DEBUG_ADJACENT */

						continue;
						}

					/* Found two subareas to left of dividing line! */
					(void) pr_error("Area.Divide",
							"Two subareas to left of dividing line?\n");
					lextra = sub;
					}
				}
			break;
			}

		/* Done if we have the first two */
		if ((lfound && rfound) || (lfound && lextra) || (rfound && rextra))
			break;
		}

	/* Try to fix possible problems with dividing line */
	if (rextra)
		{
		if (subarea_orientation(rextra, &right) && right)
			{
			if (subarea_orientation(rfound, &right) && right)
				{
				(void) pr_error("Area.Divide",
						"Both subareas seem to be to the right!\n");
				}
			(void) pr_error("Area.Divide",
					"Setting first subarea to left of dividing line!\n");
			lfound = rfound;
			rfound = rextra;
			}
		else
			{
			(void) pr_error("Area.Divide",
					"Setting second subarea to left of dividing line!\n");
			lfound = rextra;
			}
		}
	if (lextra)
		{
		if (subarea_orientation(lextra, &right) && !right)
			{
			if (subarea_orientation(lfound, &right) && right)
				{
				(void) pr_error("Area.Divide",
						"Both subareas seem to be to the left!\n");
				}
			(void) pr_error("Area.Divide",
					"Setting first subarea to right of dividing line!\n");
			rfound = lfound;
			lfound = lextra;
			}
		else
			{
			(void) pr_error("Area.Divide",
					"Setting second subarea to right of dividing line!\n");
			rfound = lextra;
			}
		}

	if (lsub) *lsub = lfound;
	if (rsub) *rsub = rfound;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     r e s e t _ a r e a _ s u b i d s                                *
*                                                                      *
***********************************************************************/
/***********************************************************************/
/**	reset subarea ids for an area with a missing dividing line
 *
 * 	@param[in]	numdiv	number of subarea ids.
 * 	@param[in]	*subids	list of subarea ids.
 * 	@param[in]	mdiv	location of missing dividing line.
 * 	@param[in]	next	next subarea that would be created.
 * 	@param[in]	setnext	reset index for the next subarea?
 ***********************************************************************/

void	reset_area_subids

	(
	int				numdiv,		/* Number of subarea ids */
	int				*subids,	/* Array of subarea ids */
	int				mdiv,		/* Location of missing dividing line */
	int				next,		/* Next subarea that would be created */
	LOGICAL			setnext		/* Reset index for the next subarea? */
	)

	{
	int		msub, idiv, isub, jdiv, ndiv;
	int		*xrem;

#	ifdef DEBUG_SUBIDS
	char	xbuf[10], tbuf[240];
#	endif /* DEBUG_SUBIDS */

	/* Reset the index for the missing dividing line */
	msub = subids[mdiv];
	subids[mdiv] = -1;

	/* Create array for number of missing subareas */
	xrem = INITMEM(int, numdiv);
	for (idiv=0; idiv<numdiv; idiv++) xrem[idiv] = 1;

#	ifdef DEBUG_SUBIDS
	(void) strcpy(tbuf, "");
	for (idiv=0; idiv<numdiv; idiv++)
		{
		(void) sprintf(xbuf, " %d", subids[idiv]);
		(void) strcat(tbuf, xbuf);
		}
	if (setnext)
		pr_diag("reset_area_subids()",
				"For mdiv/msub/next: %d %d >%d< T  Start subids: %d - %s\n",
				mdiv, msub, next, numdiv, tbuf);
	else
		pr_diag("reset_area_subids()",
				"For mdiv/msub/next: %d >%d< %d F  Start subids: %d - %s\n",
				mdiv, msub, next, numdiv, tbuf);
	(void) strcpy(tbuf, "");
	for (idiv=0; idiv<numdiv; idiv++)
		{
		(void) sprintf(xbuf, " %d", xrem[idiv]);
		(void) strcat(tbuf, xbuf);
		}
	pr_diag("reset_area_subids()",
			"                                 Start xrem: %d - %s\n",
			numdiv, tbuf);
#	endif /* DEBUG_SUBIDS */

	/* Adjust subids if the next subarea is missing */
	if (setnext)
		{
		for (idiv=mdiv+1; idiv<numdiv; idiv++)
			{
			isub = subids[idiv];

			/* Subarea already removed */
			if (isub < 0) continue;

			/* Attempt to divide a missing subarea */
			if (isub == next)
				{

				/* Reset index for this subarea */
				subids[idiv] = -1;

				/* All divides for subsequent subareas must also be reset */
				remove_area_subid(numdiv, subids, xrem, idiv+1, idiv+1);

				/* Reset the index for the number of missing subareas */
				for (jdiv=idiv+1; jdiv<numdiv; jdiv++)
					if (subids[jdiv] > idiv) xrem[jdiv]++;
				}
			}

		/* Reset subids to account for missing subarea */
		for (idiv=mdiv+1; idiv<numdiv; idiv++)
			{
			if (subids[idiv] > next) subids[idiv] -= xrem[idiv];
			}
		}

	/* Adjust subids if the present subarea is missing */
	else
		{
		for (idiv=mdiv+1; idiv<numdiv; idiv++)
			{
			isub = subids[idiv];

			/* Subarea already removed */
			if (isub < 0) continue;

			/* Attempt to divide a missing subarea */
			if (isub == msub)
				{

				/* Reset index for this subarea */
				subids[idiv] = -1;

				/* All divides for subsequent subareas must also be reset */
				remove_area_subid(numdiv, subids, xrem, idiv+1, idiv+1);

				/* Reset the index for the number of missing subareas */
				for (jdiv=idiv+1; jdiv<numdiv; jdiv++)
					if (subids[jdiv] > idiv) xrem[jdiv]++;
				}
			}

		/* Reset subids to account for missing subarea */
		for (idiv=mdiv+1; idiv<numdiv; idiv++)
			{
			if (subids[idiv] == next)     subids[idiv]  = msub;
			else if (subids[idiv] > next) subids[idiv] -= xrem[idiv];
			}
		}

#	ifdef DEBUG_SUBIDS
	(void) strcpy(tbuf, "");
	for (idiv=0; idiv<numdiv; idiv++)
		{
		(void) sprintf(xbuf, " %d", subids[idiv]);
		(void) strcat(tbuf, xbuf);
		}
	if (setnext)
		pr_diag("reset_area_subids()",
				"For mdiv/msub/next: %d %d >%d< T  End subids: %d - %s\n",
				mdiv, msub, next, numdiv, tbuf);
	else
		pr_diag("reset_area_subids()",
				"For mdiv/msub/next: %d >%d< %d F  End subids: %d - %s\n",
				mdiv, msub, next, numdiv, tbuf);
	(void) strcpy(tbuf, "");
	for (idiv=0; idiv<numdiv; idiv++)
		{
		(void) sprintf(xbuf, " %d", xrem[idiv]);
		(void) strcat(tbuf, xbuf);
		}
	pr_diag("reset_area_subids()",
			"                                 End xrem: %d - %s\n",
			numdiv, tbuf);
#	endif /* DEBUG_SUBIDS */

	/* Clean up missing subarea array */
	FREEMEM(xrem);

	/* Check that area subids are still valid */
	for (ndiv=0, idiv=0; idiv<numdiv; idiv++)
		if (subids[idiv] >= 0) ndiv++;
	for (idiv=0; idiv<numdiv; idiv++)
		if (subids[idiv] >= ndiv)
			(void) pr_error("Area.Divide",
						"Error in subarea ids for divided area\n");
	}

/***********************************************************************
*                                                                      *
*     l i n e _ i n s i d e _ s u b a r e a                            *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Determine if part of the given line is inside the given subarea.
 *
 * @param[in]	sub		subarea to test.
 * @param[in]	line	line to test against.
 * @return	True if successful.
 ***********************************************************************/
LOGICAL	line_inside_subarea

	(
	SUBAREA		sub,	/* given subarea */
	LINE		line	/* line to test against */
	)

	{
	POINT	*lpts;
	int		ip;
	LOGICAL	in;

	if (!sub)  return FALSE;
	if (!line) return FALSE;

	/* Check that at least one point on the line is inside the subarea */
	lpts = line->points;
	for (ip=0; ip<line->numpts; ip++)
		{
		subarea_test_point(sub, lpts[ip], NULL, NULL, NULL, NULL, &in);
		if (in) return TRUE;
		}

	/* Return FALSE if no line points were inside the subarea */
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*   STATIC Functions                                                   *
*                                                                      *
*   The following functions can only be used internally.               *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*     a r e a _ w r t _ l i n e                                        *
*     s u b a r e a _ w r t _ l i n e                                  *
*                                                                      *
*     determine location of area/subarea with respect to a line        *
*     which does not pass through the area/subarea                     *
*                                                                      *
***********************************************************************/

static	DIVSTAT	area_wrt_line

	(
	LINE			line,
	AREA			area
	)

	{
	int		dpt, ipt;
	float	ddist, bdist=-1;
	LOGICAL	right, bright;
	LINE	boundary;

	if (!line)            return DivNoInfo;
	if (line->numpts < 2) return DivNoInfo;

	if (!area)                             return DivNoInfo;
	if (!area->bound)                      return DivNoInfo;
	if (!area->bound->boundary)            return DivNoInfo;
	if (area->bound->boundary->numpts < 3) return DivNoInfo;

	/* Check every 10th point or so on the area boundary */
	boundary = area->bound->boundary;
	dpt      = boundary->numpts/10;
	if (dpt < 2) dpt = 2;

	/* Find furthest point on area boundary from the line */
	for (ipt=0; ipt<boundary->numpts; ipt+=dpt)
		{
		line_test_point(line, boundary->points[ipt], &ddist,
							NullPoint, NullInt, NullChar, &right);
		if (ddist > bdist)
			{
			bdist  = ddist;
			bright = right;
			}
		}

	/* Return the location of the area with respect to the line based */
	/*  on the point on the area boundary furthest away from the line */
	return (bright)? DivAreaRight: DivAreaLeft;
	}

static	DIVSTAT	subarea_wrt_line

	(
	LINE			line,
	SUBAREA			sub
	)

	{
	int		nseg, iseg, npts, ips, ipe, dpt, ipt, jpt;
	float	ddist, bdist=-1;
	LOGICAL	right, bright;
	SEGMENT	seg;
	LINE	lseg;

	if (!line)            return DivNoInfo;
	if (line->numpts < 2) return DivNoInfo;

	if (!sub)            return DivNoInfo;
	if (sub->numseg < 1) return DivNoInfo;

	/* Find furthest point on subarea segments from the line */
	nseg = sub->numseg;
	for (iseg=0; iseg<nseg; iseg++)
		{

		seg = sub->segments[iseg];
		if (IsNull(seg)) continue;
		lseg = seg->line;
		if (IsNull(lseg) || lseg->numpts <= 0) continue;

		/* Set segment parameters to check every 5th point or so */
		if (seg_forward(seg))
			{
			npts = lseg->numpts;
			ips  = seg->ips;
			ipe  = seg->ipe;
			if (ipe < ips) ipe += npts;
			dpt  = (ipe-ips)/5;
			if (dpt < 2) dpt = 2;
			}
		else
			{
			npts = lseg->numpts;
			ips  = seg->ipe;
			ipe  = seg->ips;
			if (ips < ipe) ips += npts;
			dpt  = (ipe-ips)/5;
			if (dpt > -2) dpt = -2;
			}

		/* Loop through points in current boundary segment in proper order */
		for (ipt=ips; ; ipt+=dpt)
			{
			if ( seg_forward(seg) && ipt>ipe) break;
			if (!seg_forward(seg) && ipt<ipe) break;

			jpt = ipt%npts;
			line_test_point(line, lseg->points[jpt], &ddist,
								NullPoint, NullInt, NullChar, &right);
			if (ddist > bdist)
				{
				bdist  = ddist;
				bright = right;
				}
			}
		}

	/* Return the location of the subarea with respect to the line based */
	/*  on the point on the subarea segments furthest away from the line */
	return (bright)? DivAreaRight: DivAreaLeft;
	}

/***********************************************************************
*                                                                      *
*     s u b a r e a _ o r i e n t a t i o n                            *
*                                                                      *
*     determine orientation of subarea wrt subarea boundary            *
*                                                                      *
***********************************************************************/

static	LOGICAL	subarea_orientation

	(
	SUBAREA	sub,		/* given subarea */
	LOGICAL	*right		/* is the subarea to right of boundary? */
	)

	{
	float	x, xprev, xtest, xnext, dx, dxn, dxp;
	float	y, yprev, ytest, ynext, dy, dyn, dyp;
	float	xmid, ymid, dz, dzn, dzp, ang;
	int		iseg, nseg, ips, ipe, ip, jp, np, dp;
	SEGMENT	seg;
	LINE	lseg;
	POINT	rpos, lpos;
	LOGICAL	rin, lin;

	LOGICAL	First = TRUE;
	LOGICAL	Test  = TRUE;
	LOGICAL	AngOK = FALSE;

#	ifdef DEBUG_ORIENTATION
	STRING	sdirection, sforward = "->", sbackward = "<-";
#	endif /* DEBUG_ORIENTATION */

	/* Initialize to nice garbage */
	if (right) *right = TRUE;

	/* Return if sub not there */
	if (!sub)             return FALSE;
	if (sub->numseg <= 0) return FALSE;

#	ifdef DEBUG_ORIENTATION
	(void) fprintf(stderr,
		"[subarea_orientation] Checking subarea with: %d segments\n",
		sub->numseg);
#	endif /* DEBUG_ORIENTATION */

	/* Loop through all boundary segments */
	nseg = sub->numseg;
	for (iseg=0; iseg<nseg; iseg++)
		{
		seg  = sub->segments[iseg];
		lseg = seg->line;
		if (seg_forward(seg))
			{
			np  = lseg->numpts;
			dp  = 1;
			ips = seg->ips;
			ipe = seg->ipe;
			if (ipe < ips) ipe += np;
			}
		else
			{
			np  = lseg->numpts;
			dp  = -1;
			ips = seg->ipe;
			ipe = seg->ips;
			if (ips < ipe) ips += np;
			}

#		ifdef DEBUG_ORIENTATION
		sdirection = (seg_forward(seg))? sforward: sbackward;
		(void) fprintf(stderr,
			"[subarea_orientation]  Checking segment with points: %d %s %d\n",
			ips, sdirection, ipe);
#		endif /* DEBUG_ORIENTATION */

		/* Loop through points in current boundary segment in proper order */
		for (ip=ips; ; ip+=dp)
			{
			if ( seg_forward(seg) && ip>ipe) break;
			if (!seg_forward(seg) && ip<ipe) break;

			/* Extract point co-ordinates */
			jp = ip%np;
			x  = lseg->points[jp][X];
			y  = lseg->points[jp][Y];

			/* Set first point */
			if (First)
				{
				xprev = x;
				yprev = y;
				First = FALSE;
				continue;
				}

			/* Set test point */
			if (Test)
				{
				xtest = x;
				ytest = y;
				dx = xtest - xprev;
				dy = ytest - yprev;
				if ((dx==0) && (dy==0)) continue;

				/* Found test point */
				dz = (float) hypot((double) dx, (double) dy);
				Test = FALSE;
				continue;
				}

			/* Set next point */
			xnext = x;
			ynext = y;
			dxn = xnext - xtest;
			dyn = ynext - ytest;
			if ((dxn==0) && (dyn==0)) continue;

			/* Found next point */
			dzn = (float) hypot((double) dxn, (double) dyn);

			/* Determine cosine of angle about xtest/ytest */
			dxp = xnext - xprev;
			dyp = ynext - yprev;
			dzp = (float) hypot((double) dxp, (double) dyp);
			ang = (dz*dz + dzn*dzn - dzp*dzp) / (2.0 * dz * dzn);

			/* Angle must be greater than 20 degrees or so */
			if (ang > 0.94)
				{
#				ifdef DEBUG_ORIENTATION
				(void) fprintf(stderr,
					"[subarea_orientation]   Bad angle: %.3f  to point: %d\n",
					ang, jp);
#				endif /* DEBUG_ORIENTATION */

				/* Reset parameters and look for next point */
				AngOK = FALSE;
				xprev = xtest;
				yprev = ytest;
				xtest = xnext;
				ytest = ynext;
				dx = dxn;
				dy = dyn;
				dz = dzn;
				continue;
				}

#			ifdef DEBUG_ORIENTATION
			(void) fprintf(stderr,
				"[subarea_orientation]   Found good angle: %.3f  to point: %d\n",
				ang, jp);
#			endif /* DEBUG_ORIENTATION */

			/* Look for two good angles in a row */
			if (!AngOK)
				{
				AngOK = TRUE;

				/* Reset parameters and look for next point */
				xprev = xtest;
				yprev = ytest;
				xtest = xnext;
				ytest = ynext;
				dx = dxn;
				dy = dyn;
				dz = dzn;
				continue;
				}

			/* Set points just to right and left of xtest/ytest span */
			dz /= 100.0;
			xmid = (xtest + xprev) / 2.0;
			ymid = (ytest + yprev) / 2.0;
			(void) set_point(rpos, xmid, ymid);
			(void) set_point(lpos, xmid, ymid);

#			ifdef DEBUG_ORIENTATION
			(void) fprintf(stderr,
				"[subarea_orientation]   Test point: %.1f %.1f  to: %.1f %.1f \n",
				xprev, yprev, xtest, ytest);
			(void) fprintf(stderr,
				"[subarea_orientation]   Test dx/dy/dz: %.1f %.1f %.1f\n",
				dx, dy, dz);
			(void) fprintf(stderr,
				"[subarea_orientation]   Initial rpos: %.1f %.1f  lpos: %.1f %.1f\n",
				rpos[X], rpos[Y], lpos[X], lpos[Y]);
#			endif /* DEBUG_ORIENTATION */

			/* Near vertical cases */
			if (fabs((double) dx) < dz && dy > 0)
				{
				rpos[X] += dz; lpos[X] -=dz;
				}
			else if (fabs((double) dx) < dz)
				{
				rpos[X] -= dz; lpos[X] +=dz;
				}

			/* Near horizontal cases */
			else if (fabs((double) dy) < dz && dx > 0)
				{
				rpos[Y] -= dz; lpos[Y] +=dz;
				}
			else if (fabs((double) dy) < dz)
				{
				rpos[Y] += dz; lpos[Y] -=dz;
				}

			/* Remaining cases */
			else if (dx > 0.0 && dy > 0.0)
				{
				rpos[X] += dz; rpos[Y] -= dz; lpos[X] -=dz; lpos[Y] +=dz;
				}
			else if (dx > 0.0)
				{
				rpos[X] -= dz; rpos[Y] -= dz; lpos[X] +=dz; lpos[Y] +=dz;
				}
			else if (dx < 0.0 && dy > 0.0)
				{
				rpos[X] += dz; rpos[Y] += dz; lpos[X] -=dz; lpos[Y] -=dz;
				}
			else
				{
				rpos[X] -= dz; rpos[Y] += dz; lpos[X] +=dz; lpos[Y] -=dz;
				}

#			ifdef DEBUG_ORIENTATION
			(void) fprintf(stderr,
				"[subarea_orientation]   Final   rpos: %.1f %.1f  lpos: %.1f %.1f\n",
				rpos[X], rpos[Y], lpos[X], lpos[Y]);
#			endif /* DEBUG_ORIENTATION */

			/* Check which one is inside */
			(void) subarea_test_point(sub, rpos, NullFloat, NullPoint,
					NullInt, NullInt, &rin);
			(void) subarea_test_point(sub, lpos, NullFloat, NullPoint,
					NullInt, NullInt, &lin);
			if ((lin && rin) || (!lin && !rin))
				{
				(void) pr_error("Areas", "Problem with subarea orientation!\n");
				return FALSE;
				}

			/* Return orientation */
			if (right) *right = rin;
			return TRUE;
			}
		}

	/* Problem with points */
	(void) pr_error("Areas", "Cannot determine subarea orientation!\n");
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*     r e m o v e _ a r e a _ s u b i d                                *
*                                                                      *
*     remove subarea ids for a deleted subarea and all subsequent      *
*     subareas of the deleted subarea                                  *
*                                                                      *
***********************************************************************/

static	void	remove_area_subid

	(
	int				numdiv,		/* Number of subarea ids */
	int				*subids,	/* Array of subarea ids */
	int				*xrem,		/* Array of number of missing subareas */
	int				sdiv,		/* Location to start checking */
	int				missing		/* Index of missing subarea */
	)

	{
	int		idiv, isub, jdiv;

#	ifdef DEBUG_SUBIDS
	char	xbuf[10], tbuf[240];
#	endif /* DEBUG_SUBIDS */

#	ifdef DEBUG_SUBIDS
	(void) strcpy(tbuf, "");
	for (idiv=0; idiv<numdiv; idiv++)
		{
		(void) sprintf(xbuf, " %d", subids[idiv]);
		(void) strcat(tbuf, xbuf);
		}
	pr_diag("remove_area_subid()",
			"For sdiv/missing: %d %d  Start subids: %d - %s\n",
			sdiv, missing, numdiv, tbuf);
	(void) strcpy(tbuf, "");
	for (idiv=0; idiv<numdiv; idiv++)
		{
		(void) sprintf(xbuf, " %d", xrem[idiv]);
		(void) strcat(tbuf, xbuf);
		}
	pr_diag("remove_area_subid()",
			"                         Start xrem: %d - %s\n",
			numdiv, tbuf);
#	endif /* DEBUG_SUBIDS */

	/* Check subids for attempts to divide a missing subarea */
	for (idiv=sdiv; idiv<numdiv; idiv++)
		{
		isub = subids[idiv];

		/* Subarea already removed */
		if (isub < 0) continue;

		/* Attempt to divide a missing subarea */
		if (isub == missing)
			{

			/* Reset index for this subarea */
			subids[idiv] = -1;

			/* All divides for the next subarea must also be reset */
			remove_area_subid(numdiv, subids, xrem, idiv+1, idiv+1);

			/* Reset the index for the number of missing subareas */
			for (jdiv=idiv+1; jdiv<numdiv; jdiv++)
				if (subids[jdiv] > idiv) xrem[jdiv]++;
			}
		}

#	ifdef DEBUG_SUBIDS
	(void) strcpy(tbuf, "");
	for (idiv=0; idiv<numdiv; idiv++)
		{
		(void) sprintf(xbuf, " %d", subids[idiv]);
		(void) strcat(tbuf, xbuf);
		}
	pr_diag("remove_area_subid()",
			"For sdiv/missing: %d %d  End subids: %d - %s\n",
			sdiv, missing, numdiv, tbuf);
	(void) strcpy(tbuf, "");
	for (idiv=0; idiv<numdiv; idiv++)
		{
		(void) sprintf(xbuf, " %d", xrem[idiv]);
		(void) strcat(tbuf, xbuf);
		}
	pr_diag("remove_area_subid()",
			"                         End xrem: %d - %s\n",
			numdiv, tbuf);
#	endif /* DEBUG_SUBIDS */
	}
