/*********************************************************************/
/**	@file barb_set.c
 *
 * Assorted operations on barbs and sets of barbs.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    b a r b _ s e t . c                                               *
*                                                                      *
*    Assorted operations on barbs and sets of barbs.                   *
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

#include "barb.h"
#include "set_oper.h"

#include <tools/tools.h>
#include <fpa_math.h>

#include <stdio.h>
#include <string.h>

/***********************************************************************
*                                                                      *
*      c l o s e s t _ b a r b                                         *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find closest barb in the given barb set to the given point.
 *
 *	@param[in] 	set			barb set to be examined
 *	@param[in] 	p			reference point
 *	@param[in] 	category	desired category
 *	@param[out]	*pdist		distance to closest barb
 *	@param[out]	ppoint		actual barb location
 *  @return Pointer to the closest barb to a given point.
 *********************************************************************/

BARB	closest_barb

	(
	SET		set,
	POINT	p,
	STRING	category,
	float	*pdist,
	POINT	ppoint
	)

	{
	int		i;
	BARB	barb, bbarb;
	POINT	q;
	float	dist, bdist, barb_distance();
	int		found = FALSE;
	STRING	cat;
	LOGICAL	rev=FALSE;

	bbarb = NullBarb;
	bdist = -1;

	/* Set to reasonable default values */
	if (pdist) *pdist = bdist;
	if (ppoint) copy_point(ppoint, ZeroPoint);

	/* Return if barb set doesn't exist */
	if (!p)                      return NullBarb;
	if (!set)                    return NullBarb;
	if (!same(set->type,"barb")) return NullBarb;

	if (NotNull(category))
		{
		if (category[0] == '!')
			{
			category++;
			rev = TRUE;
			}
		}

	/* Examine all barbs in the barb set */
	for (i=0; i<set->num; i++)
		{
		barb = (BARB) set->list[i];
		if (NotNull(category))
			{
			(void) get_attribute(barb->attrib, AttribCategory, &cat);
			if (!rev && !same(cat, category)) continue;
			if ( rev &&  same(cat, category)) continue;
			}
		dist = barb_distance(barb,p,q);
		if ((!found) || ((dist >= 0) && (dist < bdist)))
			{
			bbarb = barb;
			bdist = dist;
			found = TRUE;
			}
		}

	/* Return the closest barb */
	if (pdist) *pdist = bdist;
	if (ppoint && bbarb) copy_point(ppoint, bbarb->anchor);
	return bbarb;
	}

/***********************************************************************
*                                                                      *
*      b a r b _ c o u n t                                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Return number of barbs in the given set, with the given category.
 *
 *	@param[in] 	set			barb set to be examined
 *	@param[in] 	category	desired category
 *  @return The number of barbs that match the given category.
 *********************************************************************/

int		barb_count

	(
	SET		set,
	STRING	category
	)

	{
	int		i, count;
	BARB	barb;
	STRING	cat;
	LOGICAL	rev=FALSE;

	/* Return if barb set doesn't exist */
	if (!set) return 0;
	if (!same(set->type,"barb")) return 0;

	if (NotNull(category))
		{
		if (category[0] == '!')
			{
			category++;
			rev = TRUE;
			}
		}

	/* Examine all barbs in the barb set */
	count = 0;
	for (i=0; i<set->num; i++)
		{
		barb = (BARB) set->list[i];
		if (NotNull(category))
			{
			(void) get_attribute(barb->attrib, AttribCategory, &cat);
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
*      b a r b _ d i s t a n c e                                       *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find distance between given point and given barb.
 *
 *	@param[in] 	barb	given barb
 *	@param[in] 	p		location of point
 *	@param[out]	q		location of barb
 *  @return Distance between the point and barb.
 *********************************************************************/
float	barb_distance

	(
	BARB	barb,
	POINT	p,
	POINT	q
	)

	{
	float	dist, xp, yp;

	if (q) copy_point(q, ZeroPoint);
	if (!p) return -1.0;
	if (!barb) return -1.0;

	xp = barb->anchor[X];
	yp = barb->anchor[Y];
	if (q) set_point(q, xp, yp);
	dist = hypot(p[X]-xp, p[Y]-yp);
	return dist;
	}
