/*********************************************************************/
/**	@file label_set.c
 *
 * Assorted operations on labels and sets of labels.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    l a b e l _ s e t . c                                             *
*                                                                      *
*    Assorted operations on labels and sets of labels.                 *
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

#include "label.h"
#include "set_oper.h"

#include <tools/tools.h>
#include <fpa_math.h>

#include <stdio.h>
#include <string.h>

/***********************************************************************
*                                                                      *
*      c l o s e s t _ l a b e l                                       *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find closest label in the given label set to the given point.
 *
 *	@param[in] 	set			label set to be examined
 *	@param[in] 	p			reference point
 *	@param[in] 	category	desired category
 *	@param[out]	*pdist		distance to closest label
 *	@param[out]	ppoint		actual label location
 *  @return Pointer to a label in the set.
 *********************************************************************/

LABEL	closest_label

	(
	SET		set,
	POINT	p,
	STRING	category,
	float	*pdist,
	POINT	ppoint
	)

	{
	int		i;
	LABEL	label, blabel;
	POINT	q;
	float	dist, bdist, label_distance();
	int		found = FALSE;
	STRING	cat;
	LOGICAL	rev=FALSE;

	blabel = NullLabel;
	bdist  = -1;

	/* Set to reasonable default values */
	if (pdist) *pdist = bdist;
	if (ppoint) copy_point(ppoint, ZeroPoint);

	/* Return if label set doesn't exist */
	if (!p)                        return NullLabel;
	if (!set)                      return NullLabel;
	if (!same(set->type, "label")) return NullLabel;

	if (NotNull(category))
		{
		if (category[0] == '!')
			{
			category++;
			rev = TRUE;
			}
		}

	/* Examine all labels in the label set */
	for (i=0; i<set->num; i++)
		{
		label = (LABEL) set->list[i];
		if (NotNull(category))
			{
			(void) get_attribute(label->attrib, AttribCategory, &cat);
			if (!rev && !same(cat, category)) continue;
			if ( rev &&  same(cat, category)) continue;
			}
		dist = label_distance(label, p, q);
		if ((!found) || ((dist >= 0) && (dist < bdist)))
			{
			blabel = label;
			bdist  = dist;
			found  = TRUE;
			}
		}

	/* Return the closest label */
	if (pdist) *pdist = bdist;
	if (blabel && ppoint) copy_point(ppoint, blabel->anchor);
	return blabel;
	}

/***********************************************************************
*                                                                      *
*      l a b e l _ c o u n t                                           *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Count number of labels in the given set, with the given category.
 *
 *	@param[in] 	set			label set to be examined
 *	@param[in] 	category	desired category
 *  @return The number of labels in the set, with the category.
 *********************************************************************/

int		label_count

	(
	SET		set,
	STRING	category
	)

	{
	int		i, count;
	LABEL	label;
	STRING	cat;
	LOGICAL	rev=FALSE;

	/* Return if label set doesn't exist */
	if (!set) return 0;
	if (!same(set->type, "label")) return 0;

	if (NotNull(category))
		{
		if (category[0] == '!')
			{
			category++;
			rev = TRUE;
			}
		}

	/* Examine all labels in the label set */
	count = 0;
	for (i=0; i<set->num; i++)
		{
		label = (LABEL) set->list[i];
		if (NotNull(category))
			{
			(void) get_attribute(label->attrib, AttribCategory, &cat);
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
*      l a b e l _ d i s t a n c e                                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find distance between given point and given label.
 *
 *	@param[in] 	label		given label
 *	@param[in] 	p			Point to compare
 *	@param[out]	q			return location of label
 *  @return The distance between the point and label.
 *********************************************************************/

float	label_distance

	(
	LABEL	label,
	POINT	p,
	POINT	q
	)

	{
	float	dist, xp, yp;

	if (q) copy_point(q, ZeroPoint);
	if (!p) return -1.0;
	if (!label) return -1.0;

	xp = label->anchor[X];
	yp = label->anchor[Y];
	if (q) set_point(q, xp, yp);
	dist = hypot(p[X]-xp, p[Y]-yp);
	return dist;
	}
