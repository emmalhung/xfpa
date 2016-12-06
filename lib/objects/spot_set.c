/*********************************************************************/
/** @file spot_set.c
 *
 * Assorted operations on spots and sets of spots.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    s p o t _ s e t . c                                               *
*                                                                      *
*    Assorted operations on spots and sets of spots.                   *
*                                                                      *
*     Version 4 (c) Copyright 1997 Environment Canada (AES)            *
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

#include "spot.h"
#include "set_oper.h"

#include <tools/tools.h>
#include <fpa_math.h>

#include <stdio.h>
#include <string.h>

/***********************************************************************
*                                                                      *
*      c l o s e s t _ s p o t                                         *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find closest spot in the given spot set to the given point.
 *
 *	@param[in] 	set			spot set to be examined
 *	@param[in] 	p			reference point
 *	@param[in] 	class		desired class
 *	@param[in] 	category	desired category
 *	@param[out]	*pdist		distance to closest spot
 *	@param[out]	ppoint		actual spot location
 * 	@return Pointer to closest spot in the set to the point.
 *********************************************************************/

SPOT	closest_spot

	(
	SET		set,
	POINT	p,
	STRING	class,
	STRING	category,
	float	*pdist,
	POINT	ppoint
	)

	{
	int		i;
	SPOT	spot, bspot;
	POINT	q;
	float	dist, bdist;
	int		found = FALSE;
	STRING	cat;
	LOGICAL	clsrev=FALSE;
	LOGICAL	catrev=FALSE;

	bspot = NullSpot;
	bdist = -1;

	/* Set to reasonable default values */
	if (pdist) *pdist = bdist;
	if (ppoint) copy_point(ppoint, ZeroPoint);

	/* Return if spot set doesn't exist */
	if (!p)                       return NullSpot;
	if (!set)                     return NullSpot;
	if (!same(set->type, "spot")) return NullSpot;

	if (NotNull(class))
		{
		if (class[0] == '!')
			{
			class++;
			clsrev = TRUE;
			}
		}
	if (NotNull(category))
		{
		if (category[0] == '!')
			{
			category++;
			catrev = TRUE;
			}
		}

	/* Examine all spots in the spot set */
	for (i=0; i<set->num; i++)
		{
		spot = (SPOT) set->list[i];
		if (NotNull(class))
			{
			if (!clsrev && !same(class, spot->mclass)) continue;
			if ( clsrev &&  same(class, spot->mclass)) continue;
			}
		if (NotNull(category))
			{
			(void) get_attribute(spot->attrib, AttribCategory, &cat);
			if (!catrev && !same(cat, category)) continue;
			if ( catrev &&  same(cat, category)) continue;
			}
		dist = spot_distance(spot, p, q);
		if ((!found) || ((dist >= 0) && (dist < bdist)))
			{
			bspot = spot;
			bdist = dist;
			found = TRUE;
			}
		}

	/* Return the closest spot */
	if (pdist) *pdist = bdist;
	if (bspot) copy_point(ppoint, bspot->anchor);
	return bspot;
	}

/***********************************************************************
*                                                                      *
*      s p o t _ c o u n t                                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Count the number of spots in the given set, with the given category.
 *
 *	@param[in] 	set			spot set to be examined
 *	@param[in] 	class		desired class
 *	@param[in] 	category	desired category
 * 	@return number of spots in the set, with the category.
 *********************************************************************/

int		spot_count

	(
	SET		set,
	STRING	class,
	STRING	category
	)

	{
	int		i, count;
	SPOT	spot;
	STRING	cat;
	LOGICAL	clsrev=FALSE;
	LOGICAL	catrev=FALSE;

	/* Return if spot set doesn't exist */
	if (!set) return 0;
	if (!same(set->type, "spot")) return 0;

	if (NotNull(class))
		{
		if (class[0] == '!')
			{
			class++;
			clsrev = TRUE;
			}
		}
	if (NotNull(category))
		{
		if (category[0] == '!')
			{
			category++;
			catrev = TRUE;
			}
		}

	/* Examine all spots in the spot set */
	count = 0;
	for (i=0; i<set->num; i++)
		{
		spot = (SPOT) set->list[i];
		if (NotNull(class))
			{
			if (!clsrev && !same(class, spot->mclass)) continue;
			if ( clsrev &&  same(class, spot->mclass)) continue;
			}
		if (NotNull(category))
			{
			(void) get_attribute(spot->attrib, AttribCategory, &cat);
			if (!catrev && !same(cat, category)) continue;
			if ( catrev &&  same(cat, category)) continue;
			}
		count++;
		}

	/* Return the count */
	return count;
	}

/***********************************************************************
*                                                                      *
*      s p o t _ d i s t a n c e                                       *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find distance between given point and given spot.
 *
 *	@param[in] 	spot	given spot
 *	@param[in] 	p		given point
 *	@param[out]	q		distance broken down into x-y components
 * 	@return The distance between the point and spot.
 *********************************************************************/

float	spot_distance

	(
	SPOT	spot,
	POINT	p,
	POINT	q
	)

	{
	float	dist, xp, yp;

	if (q) copy_point(q, ZeroPoint);
	if (!p) return -1.0;
	if (!spot) return -1.0;

	xp = spot->anchor[X];
	yp = spot->anchor[Y];
	if (q) set_point(q, xp, yp);
	dist = hypot(p[X]-xp, p[Y]-yp);
	return dist;
	}

/***********************************************************************
*                                                                      *
*      p r e p a r e _ s p o t _ s e t                                 *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Build spot members according to presentation specs.
 *
 *	@param[in] 	set		set to prepare
 *********************************************************************/

void	prepare_spot_set

	(
	SET		set
	)

	{
	int		i;
	SPOT	spot;

	if (IsNull(set))              return;
	if (!same(set->type, "spot")) return;
	if (set->num <= 0)            return;

	for (i=0; i<set->num; i++)
		{
		spot = (SPOT) set->list[i];
		(void) build_spot_members(spot, set->ncspec, set->cspecs);
		}
	}
