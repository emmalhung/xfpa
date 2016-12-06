/*********************************************************************/
/**	@file curve_set.c
 *
 * Assorted operations on curves and sets of curves.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    c u r v e _ s e t . c                                             *
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
#include "set_oper.h"

#include <tools/tools.h>
#include <fpa_macros.h>
#include <fpa_getmem.h>
#include <fpa_math.h>

#include <stdio.h>
#include <string.h>


/***********************************************************************
*                                                                      *
*      c l o s e s t _ c u r v e                                       *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find closest curve in the given curve set to the given point.
 *
 *	@param[in]	 	set			curve set to be examined
 *	@param[in]	 	p			reference point
 *	@param[out] 	*dist		distance to closest curve
 *	@param[out] 	pclosest	closest point on curve
 *	@param[out] 	*segment	line segment containing closest point
 *  @return Pointer to the curve object closes to the given point.
 *********************************************************************/

CURVE	closest_curve

	(
	SET		set,
	POINT	p,
	float	*dist,
	POINT	pclosest,
	int		*segment
	)

	{
	int		icurv;
	CURVE	curv;
	POINT	q;

	CURVE	BestCurv = NullCurve;
	float	BestDist = -1;
	float	Dist;
	int		Segment;

	/* Set to reasonable default values */
	if (pclosest) copy_point(pclosest,p);
	if (dist)     *dist    = BestDist;
	if (segment)  *segment = -1;

	/* Return if set doesn't contain a set of curves */
	if (!p)                       return NullCurve;
	if (!set)                     return NullCurve;
	if (set->num <= 0)            return NullCurve;
	if (!same(set->type,"curve")) return NullCurve;

	/* Examine all curves in the set */
	for (icurv=0; icurv<set->num; icurv++)
		{
		curv = (CURVE) set->list[icurv];
		if (!curv) continue;
		line_test_point(curv->line,p,&Dist,q,&Segment,NullChar,NullChar);
		if ((!BestCurv) || ((Dist >= 0) && (Dist < BestDist)))
			{
			BestCurv = curv;
			BestDist = Dist;
			if (pclosest) copy_point(pclosest,q);
			if (dist)     *dist    = Dist;
			if (segment)  *segment = Segment;
			}
		}

	/* Return the closest curve */
	return BestCurv;
	}

/***********************************************************************
*                                                                      *
*      e n c l o s i n g _ c u r v e                                   *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find the smallest closed curve in the given curve set, that
 * encloses the given point.
 *
 *	@param[in]	 	set		set of curves to be examined
 *	@param[in]	 	p		test point
 *	@param[in]	 	mode	pick/order mode
 *	@param[out] 	*size	size of enclosing area
 *	@param[out] 	*cwise	is it clockwise or otherwise
 *  @return Pointer to smallest closed curve in the set that encloses
 * 			the point.
 *********************************************************************/

/*ARGSUSED*/
CURVE	enclosing_curve

	(
	SET		set,
	POINT	p,
	PICK	mode,
	float	*size,
	LOGICAL	*cwise
	)

	{
	int		icurv;
	LOGICAL	inside;
	CURVE	curv;

	CURVE	BestCurv = NullCurve;
	float	BestSize = -1;
	float	Size;
	LOGICAL	Clock;

	/* Set to reasonable default values */
	if (size)  *size  = -1;
	if (cwise) *cwise = FALSE;

	/* Return if set doesn't contain a set of curves */
	if (!p)                       return NullCurve;
	if (!set)                     return NullCurve;
	if (set->num <= 0)            return NullCurve;
	if (!same(set->type,"curve")) return NullCurve;

	/* Examine all curves in the set */
	for (icurv=0; icurv<set->num; icurv++)
		{
		curv = (CURVE) set->list[icurv];
		line_test_point(curv->line,p,NullFloat,NullPoint,NullInt,&inside,
						NullChar);
		if (inside)
			{
			line_properties(curv->line,NullChar,&Clock,&Size,NullFloat);
			if ((!BestCurv) || ((Size >= 0) && (Size < BestSize)))
				{
				BestCurv = curv;
				BestSize = Size;
				if (size)  *size  = Size;
				if (cwise) *cwise = Clock;
				}
			}
		}

	/* Return the smallest enclosing curve */
	return BestCurv;
	}

/***********************************************************************
*                                                                      *
*      s o r t _ e n c l o s i n g _ c u r v e s                       *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Sort the list of enclosing areas in ascending order of size.
 *
 *	@param[in]	 	set			curve set to be examined
 *	@param[in]	 	p			test point
 *	@param[in]	 	mode		pick/order mode
 *	@param[out] 	**curves	sorted list of curves
 *  @return The number elements in the list. 0 if something went
 * 			wrong.
 *********************************************************************/

/*ARGSUSED*/
int		sort_enclosing_curves

	(
	SET		set,
	POINT	p,
	PICK	mode,
	CURVE	**curves
	)

	{
	CURVE	curv;
	float	size, dist;
	int		i, seg;
	LOGICAL	inside, right;
	POINT	q;
	SITEM	*sortlist;

	/* Static storage for sorted list of curves */
	/* NOTE: This structure is overwritten each time this routine is called */
	static	CURVE       *sorted = NullCurveList;
	static	unsigned    nsort   = 0;

	/* Set to reasonable default values */
	if (curves) *curves = NullCurveList;

	/* Return if curve set doesn't exist */
	if (!p)                       return 0;
	if (!set)                     return 0;
	if (set->num <= 0)            return 0;
	if (!same(set->type,"curve")) return 0;

	/* Allocate list to be sorted */
	sortlist = INITMEM(SITEM,set->num);

	/* Examine all curves in the curve set */
	(void) printf("Unsorted:\n");
	nsort = 0;
	for (i=0; i<set->num; i++)
		{
		curv = (CURVE) set->list[i];
		if (!curv) continue;
		line_test_point(curv->line,p,&dist,q,&seg,&inside,&right);
		if (inside)
			{
			line_properties(curv->line,NullChar,NullChar,&size,NullFloat);
			sortlist[nsort].item  = (ITEM) curv;
			sortlist[nsort].value = size;
			nsort++;
			}
		}

	/* Sort in terms of size */
	if (nsort <= 0)
		{
		FREEMEM(sortlist);
		(void) printf("    None\n");
		return 0;
		}
	qsort((POINTER)sortlist,nsort,sizeof(SITEM),itemcmp);

	/* Construct and return the sorted list of enclosing curves */
	(void) printf("Sorted:\n");
	sorted = GETMEM(sorted,CURVE,nsort);
	for (i=0; i<nsort; i++)
		{
		sorted[i] = (CURVE) sortlist[i].item;
		}
	FREEMEM(sortlist);
	*curves = sorted;
	return nsort;
	}

/***********************************************************************
*                                                                      *
*      r e o r d e r _ c u r v e s                                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Sort the curves in the given set according to the given mode.
 *
 *	@param[in]	 	set		curve set to be reordered
 *	@param[in]	 	mode	pick/order mode
 * 	@return True if Successful.
 *********************************************************************/

LOGICAL	reorder_curves

	(
	SET		set,
	PICK	mode
	)

	{
	int		icurve;
	CURVE	curve;
	SITEM	*sortlist;
	void	qsort();
	float	size;
	int		(*cmp)(const void *, const void *);
	LOGICAL	bysize;

	/* Return if set doesn't exist */
	if (!set)          return FALSE;
	if (set->num <= 0) return TRUE;

	switch (mode)
		{
		case PickFirst:     return TRUE;
		case PickLast:      bysize = FALSE; cmp = itemrev; break;
		case PickSmallest:  bysize = TRUE;  cmp = itemcmp; break;
		case PickLargest:   bysize = TRUE;  cmp = itemrev; break;
		default:            return FALSE;
		}

	/* Allocate and build the list to be sorted */
	sortlist = INITMEM(SITEM, set->num);
	for (icurve=0; icurve<set->num; icurve++)
		{
		curve = (CURVE) set->list[icurve];
		if (bysize) curve_properties(curve, NullChar,NullChar,&size,NullFloat);
		else        size = (float) icurve;
		sortlist[icurve].item  = (ITEM) curve;
		sortlist[icurve].value = size;
		}

	/* Sort by size */
	qsort((POINTER)sortlist, set->num, sizeof(SITEM), cmp);

	/* Put sorted list back into the set */
	for (icurve=0; icurve<set->num; icurve++)
		{
		set->list[icurve] = sortlist[icurve].item;
		}
	FREEMEM(sortlist);
	return TRUE;
	}
