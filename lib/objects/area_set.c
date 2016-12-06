/***********************************************************************/
/**	@file area_set.c
 *
 * Assorted operations on AREA sets.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 ***********************************************************************/
/***********************************************************************
*                                                                      *
*    a r e a _ s e t . c                                               *
*                                                                      *
*    Assorted operations on AREA sets.                                 *
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
#include "set_oper.h"

#include <fpa_math.h>
#include <fpa_getmem.h>
#include <tools/tools.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static	LOGICAL	pick_better(PICK, float, float);
static	LOGICAL	pick_better
	(
	PICK	mode,
	float	oldval,
	float	newval
	)
	{
	switch (mode)
		{
		case PickFirst:		return FALSE;
		case PickLast:		return TRUE;
		case PickLargest:	return (LOGICAL) (newval>=0 && newval>oldval);
		case PickSmallest:	return (LOGICAL) (newval>=0 && newval<oldval);
		default:			return FALSE;
		}
	}

/***********************************************************************
*                                                                      *
*     e v a l _ a r e a s e t                                          *
*     e v a l _ a r e a s e t _ l i s t   (You free the lists)         *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Evaluate area set
 *
 * @param[in]	set		area set to search.
 * @param[in]	pos		point to search for.
 * @param[in]	mode	pick/order mode.
 * @param[out]	*sub	first subarea containing point.
 * @param[out]	*att	attributes for subarea containing point.
 * @return True if successful.
 ***********************************************************************/
LOGICAL	eval_areaset

	(
	SET			set,
	POINT		pos,
	PICK		mode,
	SUBAREA		*sub,
	ATTRIB_LIST	*att
	)

	{
	SUBAREA	a;
	AREA b;

	/* Set to reasonable default values */
	if (sub)  *sub = NullSubArea;
	if (att)  *att = NullAttribList;
	if (!set) return FALSE;
	if (!pos) return FALSE;

	/* Find the first enclosing subarea */
	a = enclosing_subarea(set, pos, mode, NullFloat, NullChar, NullAreaPtr);
	if (NotNull(a) && NotNull(a->attrib))
		{
		if (sub) *sub = a;
		if (att) *att = a->attrib;
		return TRUE;
		}

	/* If not found, get background value */
	b = (AREA) set->bgnd;
	if (NotNull(b) && NotNull(b->attrib))
		{
		/* Null area indicates background */
		if (sub) *sub = NullSubArea;
		if (att) *att = b->attrib;
		return TRUE;
		}

	/* No enclosing areas and no background */
	return FALSE;
	}

/**********************************************************************/

/***********************************************************************/
/**	Evaluate area set list
 *
 * @note	You need to free the memory for the lists returned here.
 *
 * @param[in]	set		setup of areas.
 * @param[in]	pos		point to check.
 * @param[in]	mode	pick/order mode.
 * @param[out]	**subs	list of subareas containing point.
 * @param[out]	**atts	list of attributes for subareas containing point.
 * @return number of areas containing point.
 ***********************************************************************/
int		eval_areaset_list

	(
	SET			set,
	POINT		pos,
	PICK		mode,
	SUBAREA		**subs,
	ATTRIB_LIST	**atts
	)

	{
	int			num, i;
	AREA		a;
	SUBAREA		*alist;
	ATTRIB_LIST	*vlist;

	/* Set to reasonable default values */
	if (subs) *subs = NullSubAreaList;
	if (atts) *atts = NullAttribListPtr;
	if (!set) return -1;
	if (!pos) return -1;

	/* Get list of enclosing subareas */
	num = enclosing_subarea_list(set, pos, mode, &alist);
	if (num < 0) return num;

	/* Add the background to the list only if empty */
	if (num == 0)
		{
		a = (AREA) set->bgnd;
		if (NotNull(a) && NotNull(a->attrib))
			{
			num++;
			alist = GETMEM(alist, SUBAREA, num);
			alist[num-1] = NullSubArea;
			}
		}

	/* Return list of subareas */
	if (subs) *subs = alist;

	/* Return list of attributes */
	if (atts)
		{
		vlist = INITMEM(ATTRIB_LIST, num);
		for (i=0; i<num; i++)
			vlist[i] = (alist[i])? alist[i]->attrib: a->attrib;
		*atts = vlist;
		}

	return num;
	}

/***********************************************************************
*                                                                      *
*      c l o s e s t _ a r e a                                         *
*                                                                      *
***********************************************************************/
/***********************************************************************/
/**	Find closest area in the given set to the given point.
 *
 * 	@param[in]	set		set to be examined.
 * 	@param[in]	p		reference point.
 * 	@param[out] *dist	distance to closest area.
 * 	@param[out] pclosest closest point on boundary or hole
 * 	@param[out]	*mtype	boundary, hole or divide?
 * 	@param[out]	*mem	which boundary, hole or divide?
 * 	@param[out]	*span	span of hole, divide or boundary with closest point.
 * 	@return closest area to reference point.
 ***********************************************************************/

AREA	closest_area

	(
	SET		set,		/* set to be examined */
	POINT	p,			/* reference point */
	float	*dist,		/* distance to closest area */
	POINT	pclosest,	/* closest point on boundary or hole */
	AMEMBER	*mtype,		/* boundary, hole or divide? */
	int		*mem,		/* which boundary, hole or divide? */
	int		*span		/* span of hole divide or boundary with closest point */
	)

	{
	int		iarea;
	AREA	area;
	POINT	q;

	AREA	BestArea = NullArea;
	float	BestDist = -1;
	float	Dist;
	AMEMBER	MemTyp;
	int		Member, Span;

	/* Set to reasonable default values */
	if (pclosest) copy_point(pclosest, p);
	if (dist)  *dist  = BestDist;
	if (mtype) *mtype = AreaNone;
	if (mem)   *mem   = -1;
	if (span)  *span  = -1;

	/* Return if set doesn't exist */
	if (!p)            return NullArea;
	if (!set)          return NullArea;
	if (set->num <= 0) return NullArea;

	/* Examine all areas in the mosaic */
	for (iarea=0; iarea<set->num; iarea++)
		{
		area = (AREA) set->list[iarea];
		MemTyp = area_closest_feature(area, p, &Dist, q, &Member, &Span);
		if ((!BestArea) || ((Dist >= 0) && (Dist < BestDist)))
			{
			BestArea = area;
			BestDist = Dist;
			if (pclosest) copy_point(pclosest, q);
			if (dist)  *dist  = Dist;
			if (mtype) *mtype = MemTyp;
			if (mem)   *mem   = Member;
			if (span)  *span  = Span;
			}
		}

	/* Return the closest area */
	return BestArea;
	}

/***********************************************************************
*                                                                      *
*      e n c l o s i n g _ a r e a                                     *
*                                                                      *
*      e n c l o s i n g _ a r e a _ l i s t                           *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Find the first area in the given set, that encloses the given
 * point, according to the given pick mode.
 *
 * @param[in]	set		set to be examined
 * @param[in]	p		test point.
 * @param[in]	mode	pick/order mode.
 * @param[out]	*size	size of enclosing area.
 * @param[out]	*cwise	is it clockwise or otherwise?
 * @return first area found to contain test point.
 ***********************************************************************/
AREA	enclosing_area

	(
	SET		set,	/* set to be examined */
	POINT	p,		/* test point */
	PICK	mode,	/* pick/order mode */
	float	*size,	/* size of enclosing area */
	LOGICAL	*cwise	/* is it clockwise or otherwise */
	)

	{
	int		iarea;
	float	dist;
	LOGICAL	inside;
	AREA	area;

	AREA	BestArea = NullArea;
	float	BestSize = -1;
	float	Size;
	LOGICAL	Clock;

	/* Set to reasonable default values */
	if (size)  *size  = -1;
	if (cwise) *cwise = FALSE;

	/* Return if mosaic doesn't exist */
	if (!p)            return NullArea;
	if (!set)          return NullArea;
	if (set->num <= 0) return NullArea;

	/* Examine all areas in the set until one is found */
	for (iarea=0; iarea<set->num; iarea++)
		{
		area = (AREA) set->list[iarea];
		area_test_point(area, p, &dist, NullPoint, NULL, NullInt,
						NullInt, &inside);
		if (!inside && dist != 0.0) continue;

		area_properties(area, &Clock, &Size, NullFloat);
		if ((!BestArea) || pick_better(mode, BestSize, Size))
			{
			BestArea = area;
			BestSize = Size;
			if (size)  *size  = Size;
			if (cwise) *cwise = Clock;
			if (mode == PickFirst) return BestArea;
			}
		}

	/* Return the smallest enclosing area */
	return BestArea;
	}

/**********************************************************************/

/***********************************************************************/
/**	Return the list of areas in the given set, that enclose the
 * given point, ordered according to the given mode.
 *
 * You free the list when done - just don't free the areas!
 *
 * @param[in]	set		set to be examined.
 * @param[in]	p		test point.
 * @param[in]	mode	pick/order mode.
 * @param[out]	**list	list of areas that enclose the point.
 * @return number of areas in the list.
 ***********************************************************************/
int		enclosing_area_list

	(
	SET		set,	/* set to be examined */
	POINT	p,		/* test point */
	PICK	mode,	/* pick/order mode */
	AREA	**list	/* list of areas that enclose the point */
	)

	{
	int		iarea;
	float	dist;
	LOGICAL	inside;
	AREA	area;

	AREA	*List = NullAreaList;
	int		Num   = 0;

	/* Set to reasonable default values */
	if (list) *list = NullAreaList;

	/* Return if set doesn't exist */
	if (!p)   return -1;
	if (!set) return -1;

	/* Examine all areas in the set */
	for (iarea=0; iarea<set->num; iarea++)
		{
		area = (AREA) set->list[iarea];
		area_test_point(area, p, &dist, NullPoint, NULL, NullInt,
						NullInt, &inside);
		if (!inside && dist != 0.0) continue;

		Num++;
		if (list)
			{
			List = GETMEM(List, AREA, Num);
			List[Num-1] = area;
			}
		}

	/* Reorder according to given mode */
	if (list && Num>0 && mode!=PickFirst)
		{
		SITEM	*sortlist;
		float	size;
		int		(*cmp)(const void *, const void *);
		LOGICAL	bysize;
		int		i;

		switch (mode)
			{
			case PickLast:		bysize = FALSE; cmp = itemrev; break;
			case PickSmallest:	bysize = TRUE;  cmp = itemcmp; break;
			case PickLargest:	bysize = TRUE;  cmp = itemrev; break;
			}

		/* Allocate and build the list to be sorted */
		sortlist = INITMEM(SITEM, Num);
		for (i=0; i<Num; i++)
			{
			area = List[i];
			if (bysize) area_properties(area, NullChar, &size, NullFloat);
			else        size = (float) i;
			sortlist[i].item  = (ITEM) area;
			sortlist[i].value = size;
			}

		/* Sort by size */
		qsort((POINTER)sortlist, (size_t) Num, sizeof(SITEM), cmp);

		/* Put sorted list back into the list */
		for (i=0; i<Num; i++)
			{
			List[i] = (AREA) sortlist[i].item;
			}
		FREEMEM(sortlist);
		}

	/* Return the list */
	if (list) *list = List;
	return Num;
	}

/***********************************************************************
*                                                                      *
*      c l o s e s t _ s u b a r e a                                   *
*                                                                      *
***********************************************************************/
/***********************************************************************/
/**	Find closest subarea in the given set of areas to the given point.
 *
 * @param[in]	set		set to be examined.
 * @param[in]	p		reference point.
 * @param[out]	*dist	distance to closest subarea
 * @param[out]	pclosest closest point on boundary.
 * @param[out]	*seg	segment containing closest point.
 * @param[out]	*span	span of segment containing closest point.
 * @param[out]	*parent	parent area.
 * @return closest subarea to reference point.
 ***********************************************************************/

SUBAREA	closest_subarea

	(
	SET		set,		/* set to be examined */
	POINT	p,			/* reference point */
	float	*dist,		/* distance to closest subarea */
	POINT	pclosest,	/* closest point on boundary */
	int		*seg,		/* segment containing closest point */
	int		*span,		/* span of segment containing closest point */
	AREA	*parent		/* parent area */
	)

	{
	int		isub, nsub;
	AREA	area;
	SUBAREA	sub;
	POINT	q;
	LOGICAL	inside;

	AREA	BestArea = NullArea;
	SUBAREA	BestSub  = NullSubArea;
	float	BestDist = -1;
	float	Dist;
	int		Seg, Span;

	/* Set to reasonable default values */
	if (pclosest) copy_point(pclosest, p);
	if (dist)     *dist   = BestDist;
	if (seg)      *seg    = -1;
	if (span)     *span   = -1;
	if (parent)   *parent = BestArea;

	/* Return if set doesn't exist */
	if (!p)            return NullSubArea;
	if (!set)          return NullSubArea;
	if (set->num <= 0) return NullSubArea;

	/* Start with the closest area */
	area = closest_area(set, p, NullFloat, NullPoint, NULL, NullInt, NullInt);
	if (IsNull(area))  return NullSubArea;

	/* See if we're inside or not */
	area_test_point(area, p, NullFloat, NullPoint, NULL, NullInt, NullInt,
				&inside);
	build_area_subareas(area);
	nsub = area->numdiv + 1;

	/* If we're inside - find the enclosing subarea */
	if (inside)
		{
		for (isub=0; isub<nsub; isub++)
			{
			sub = area->subareas[isub];
			subarea_test_point(sub, p, &Dist, q, &Seg, &Span, &inside);
			if (inside)
				{
				BestArea = area;
				BestSub  = sub;
				BestDist = Dist;
				if (pclosest) copy_point(pclosest, q);
				if (dist)     *dist   = Dist;
				if (seg)      *seg    = Seg;
				if (span)     *span   = Span;
				if (parent)   *parent = BestArea;

				/* Should only be inside one subarea */
				break;
				}
			}
		}

	/* If we're outside - find the closest subarea */
	else
		{
		for (isub=0; isub<nsub; isub++)
			{
			sub = area->subareas[isub];
			subarea_test_point(sub, p, &Dist, q, &Seg, &Span, NullChar);
			if ((!BestSub) || ((Dist >= 0) && (Dist < BestDist)))
				{
				BestArea = area;
				BestSub  = sub;
				BestDist = Dist;
				if (pclosest) copy_point(pclosest, q);
				if (dist)     *dist   = Dist;
				if (seg)      *seg    = Seg;
				if (span)     *span   = Span;
				if (parent)   *parent = BestArea;
				}
			}
		}

	/* Return the closest subarea */
	return BestSub;
	}

/***********************************************************************
*                                                                      *
*      e n c l o s i n g _ s u b a r e a                               *
*                                                                      *
*      e n c l o s i n g _ s u b a r e a _ l i s t                     *
*                                                                      *
***********************************************************************/
/***********************************************************************/
/**	Find the first subarea in the given set of areas, that
 * encloses the given point, according to the given pick mode.
 *
 * @param[in]	set		set to be examined.
 * @param[in]	p		test point.
 * @param[in]	mode	pick/order mode.
 * @param[out]	*size	size of enclosing subarea.
 * @param[out]	*cwise	is it clockwise or otherwise?
 * @param[out]	*parent parent area.
 * @return first subarea found to contain the test point.
 ***********************************************************************/

SUBAREA	enclosing_subarea

	(
	SET		set,	/* set to be examined */
	POINT	p,		/* test point */
	PICK	mode,	/* pick/order mode */
	float	*size,	/* size of enclosing subarea */
	LOGICAL	*cwise,	/* is it clockwise or otherwise */
	AREA	*parent	/* parent area */
	)

	{
	int		iarea, isub, nsub;
	float	dist;
	LOGICAL	inside;
	AREA	area;
	SUBAREA	sub;

	AREA	BestArea = NullArea;
	SUBAREA	BestSub  = NullSubArea;
	float	BestSize = -1;
	float	Size;
	LOGICAL	Clock;

	/* Set to reasonable default values */
	if (size)   *size   = -1;
	if (cwise)  *cwise  = FALSE;
	if (parent) *parent = BestArea;

	/* Return if mosaic doesn't exist */
	if (!p)            return NullSubArea;
	if (!set)          return NullSubArea;
	if (set->num <= 0) return NullSubArea;

	/* Examine all areas in the set until one is found */
	for (iarea=0; iarea<set->num; iarea++)
		{
		area = (AREA) set->list[iarea];
		area_test_point(area, p, &dist, NullPoint, NULL, NullInt,
						NullInt, &inside);
		if (!inside && dist != 0.0) continue;

		build_area_subareas(area);
		nsub = area->numdiv + 1;
		for (isub=0; isub<nsub; isub++)
			{
			sub = area->subareas[isub];
			subarea_test_point(sub, p, &dist, NullPoint, NullInt, NullInt,
								&inside);
			if (!inside && dist != 0.0) continue;

			subarea_properties(sub, &Clock, &Size, NullFloat);
			if ((!BestSub) || pick_better(mode, BestSize, Size))
				{
				BestArea = area;
				BestSub  = sub;
				BestSize = Size;
				if (size)   *size   = Size;
				if (cwise)  *cwise  = Clock;
				if (parent) *parent = BestArea;
				if (mode == PickFirst) return BestSub;
				}
			}
		}

	/* Return smallest subarea */
	return BestSub;
	}

/**********************************************************************/

/***********************************************************************/
/**	Return the list of subareas in the given set of areas, that enclose
 * the given point, ordered according to the given mode.
 *
 * You free the list when done - just don't free the areas!
 *
 * @param[in]	set		set to be examined.
 * @param[in]	p		test point.
 * @param[in]	mode	pick/order mode.
 * @param[out]	**list	list of subareas that enclose the point
 * @return the number of subareas that enclose the point.
 ***********************************************************************/
int		enclosing_subarea_list

	(
	SET		set,	/* set to be examined */
	POINT	p,		/* test point */
	PICK	mode,	/* pick/order mode */
	SUBAREA	**list	/* list of subareas that enclose the point */
	)

	{
	int		iarea, isub, nsub;
	float	dist;
	LOGICAL	inside;
	AREA	area;
	SUBAREA	sub;

	SUBAREA	*List = NullSubAreaList;
	int		Num   = 0;

	/* Set to reasonable default values */
	if (list) *list = NullSubAreaList;

	/* Return if set doesn't exist */
	if (!p)   return -1;
	if (!set) return -1;

	/* Examine all areas in the set */
	for (iarea=0; iarea<set->num; iarea++)
		{
		area = (AREA) set->list[iarea];
		area_test_point(area, p, &dist, NullPoint, NULL, NullInt,
						NullInt, &inside);
		if (!inside && dist != 0.0) continue;

		build_area_subareas(area);
		nsub = area->numdiv + 1;
		for (isub=0; isub<nsub; isub++)
			{
			sub = area->subareas[isub];
			subarea_test_point(sub, p, &dist, NullPoint, NullInt, NullInt,
								&inside);
			if (!inside && dist != 0.0) continue;

			Num++;
			if (list)
				{
				List = GETMEM(List, SUBAREA, Num);
				List[Num-1] = sub;
				}
			}
		}

	/* Reorder according to given mode */
	if (list && Num>0 && mode!=PickFirst)
		{
		SITEM	*sortlist;
		float	size;
		int		(*cmp)(const void *, const void *);
		LOGICAL	bysize;
		int		i;

		switch (mode)
			{
			case PickLast:		bysize = FALSE; cmp = itemrev; break;
			case PickSmallest:	bysize = TRUE;  cmp = itemcmp; break;
			case PickLargest:	bysize = TRUE;  cmp = itemrev; break;
			}

		/* Allocate and build the list to be sorted */
		sortlist = INITMEM(SITEM, Num);
		for (i=0; i<Num; i++)
			{
			sub = List[i];
			if (bysize) subarea_properties(sub, NullChar, &size, NullFloat);
			else        size = (float) i;
			sortlist[i].item  = (ITEM) sub;
			sortlist[i].value = size;
			}

		/* Sort by size */
		qsort((POINTER)sortlist, (size_t) Num, sizeof(SITEM), cmp);

		/* Put sorted list back into the list */
		for (i=0; i<Num; i++)
			{
			List[i] = (SUBAREA) sortlist[i].item;
			}
		FREEMEM(sortlist);
		}

	/* Return the list */
	if (list) *list = List;
	return Num;
	}

/***********************************************************************
*                                                                      *
*      r e o r d e r _ a r e a s                                       *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Sort the areas in the given set according to the given mode.
 *
 * @param[in]	set		area set to be reordered.
 * @param[in]	mode	pick/order mode.
 * @return True if successful.
 ***********************************************************************/
LOGICAL	reorder_areas

	(
	SET		set,	/* area set to be reordered */
	PICK	mode	/* pick/order mode */
	)

	{
	int		iarea;
	AREA	area;
	SITEM	*sortlist;
	float	size;
	int		(*cmp)(const void *, const void *);
	LOGICAL	bysize;

	/* Return if set doesn't exist */
	if (!set)          return FALSE;
	if (set->num <= 0) return TRUE;

	switch (mode)
		{
		case PickFirst:		return TRUE;
		case PickLast:		bysize = FALSE; cmp = itemrev; break;
		case PickSmallest:	bysize = TRUE;  cmp = itemcmp; break;
		case PickLargest:	bysize = TRUE;  cmp = itemrev; break;
		default:			return FALSE;
		}

	/* Allocate and build the list to be sorted */
	sortlist = INITMEM(SITEM, set->num);
	for (iarea=0; iarea<set->num; iarea++)
		{
		area = (AREA) set->list[iarea];
		if (bysize) area_properties(area, NullChar, &size, NullFloat);
		else        size = (float) iarea;
		sortlist[iarea].item  = (ITEM) area;
		sortlist[iarea].value = size;
		}

	/* Sort by size */
	qsort((POINTER)sortlist, (size_t) set->num, sizeof(SITEM), cmp);

	/* Put sorted list back into the set */
	for (iarea=0; iarea<set->num; iarea++)
		{
		set->list[iarea] = sortlist[iarea].item;
		}
	FREEMEM(sortlist);
	return TRUE;
	}
