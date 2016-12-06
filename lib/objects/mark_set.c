/*********************************************************************/
/**	@file mark_set.c
 *
 * Assorted operations on marks and sets of marks.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    m a r k _ s e t . c                                               *
*                                                                      *
*    Assorted operations on marks and sets of marks.                   *
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

#include "mark.h"
#include "set_oper.h"

#include <tools/tools.h>
#include <fpa_math.h>

#include <stdio.h>
#include <string.h>

/***********************************************************************
*                                                                      *
*      c l o s e s t _ m a r k                                         *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find closest mark in the given mark set to the given point.
 *
 *	@param[in] 	set			mark set to be examined
 *	@param[in] 	p			reference point
 *	@param[in] 	category	desired category
 *	@param[out]	*pdist		distance to closest mark
 *	@param[out]	ppoint		actual mark location
 *  @return Pointer to the closest mark in the set.
 *********************************************************************/

MARK	closest_mark

	(
	SET		set,
	POINT	p,
	STRING	category,
	float	*pdist,
	POINT	ppoint
	)

	{
	int		i;
	MARK	mark, bmark;
	POINT	q;
	float	dist, bdist, mark_distance();
	int		found = FALSE;
	STRING	cat;
	LOGICAL	rev=FALSE;

	bmark = NullMark;
	bdist = -1;

	/* Set to reasonable default values */
	if (pdist) *pdist = bdist;
	if (ppoint) copy_point(ppoint, ZeroPoint);

	/* Return if mark set doesn't exist */
	if (!p)                       return NullMark;
	if (!set)                     return NullMark;
	if (!same(set->type, "mark")) return NullMark;

	if (NotNull(category))
		{
		if (category[0] == '!')
			{
			category++;
			rev = TRUE;
			}
		}

	/* Examine all marks in the mark set */
	for (i=0; i<set->num; i++)
		{
		mark = (MARK) set->list[i];
		if (NotNull(category))
			{
			(void) get_attribute(mark->attrib, AttribCategory, &cat);
			if (!rev && !same(cat, category)) continue;
			if ( rev &&  same(cat, category)) continue;
			}
		dist = mark_distance(mark, p, q);
		if ((!found) || ((dist >= 0) && (dist < bdist)))
			{
			bmark = mark;
			bdist = dist;
			found = TRUE;
			}
		}

	/* Return the closest mark */
	if (pdist) *pdist = bdist;
	if (bmark && ppoint) copy_point(ppoint, bmark->anchor);
	return bmark;
	}

/***********************************************************************
*                                                                      *
*      m a r k _ c o u n t                                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Return number of marks in the given set, with the given category.
 *
 *	@param[in] 	set			mark set to be examined
 *	@param[in] 	category	desired category
 *  @return The number of marks in the set with the category.
 *********************************************************************/

int		mark_count

	(
	SET		set,
	STRING	category
	)

	{
	int		i, count;
	MARK	mark;
	STRING	cat;
	LOGICAL	rev=FALSE;

	/* Return if mark set doesn't exist */
	if (!set) return 0;
	if (!same(set->type, "mark")) return 0;

	if (NotNull(category))
		{
		if (category[0] == '!')
			{
			category++;
			rev = TRUE;
			}
		}

	/* Examine all marks in the mark set */
	count = 0;
	for (i=0; i<set->num; i++)
		{
		mark = (MARK) set->list[i];
		if (NotNull(category))
			{
			(void) get_attribute(mark->attrib, AttribCategory, &cat);
			if (!rev && !same(cat, category)) continue;
			if ( rev &&  same(cat, category)) continue;
			}
		count++;
		}

	/* Return the count */
	return count;
	}

/***********************************************************************
*                                                                      *
*      m a r k _ d i s t a n c e                                       *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find distance between given point and given mark.
 *
 * @return The distance between the point and the mark.
 *********************************************************************/

float	mark_distance

	(
	MARK	mark,
	POINT	p,
	POINT	q
	)

	{
	float	dist, xp, yp;

	if (q) copy_point(q, ZeroPoint);
	if (!p) return -1.0;
	if (!mark) return -1.0;

	xp = mark->anchor[X];
	yp = mark->anchor[Y];
	if (q) set_point(q, xp, yp);
	dist = hypot(p[X]-xp, p[Y]-yp);
	return dist;
	}
