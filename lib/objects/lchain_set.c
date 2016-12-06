/*********************************************************************/
/**	@file lchain_set.c
 *
 * Assorted operations on link chains and sets of link chains.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    l c h a i n _ s e t . c                                           *
*                                                                      *
*    Assorted operations on link chains and sets of link chains.       *
*                                                                      *
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

#include "lchain.h"
#include "set_oper.h"

#include <stdio.h>
#include <string.h>

#undef DEBUG_CLOSEST

/***********************************************************************
*                                                                      *
*      c l o s e s t _ l c h a i n                                     *
*      c l o s e s t _ l c h a i n _ n o d e                           *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find closest link chain in the given link chain set to the given point.
 *
 *	@param[in]	 	set			link chain set to be examined
 *	@param[in]	 	p			reference point
 *	@param[out] 	*dist		distance to closest link chain
 *	@param[out] 	pclosest	closest point on link chain
 *	@param[out] 	*segment	line segment containing closest point
 *  @return Pointer to the link chain object closest to the given point.
 *********************************************************************/

LCHAIN	closest_lchain

	(
	SET		set,
	POINT	p,
	float	*dist,
	POINT	pclosest,
	int		*segment
	)

	{
	int		ichain;
	LCHAIN	chain;
	POINT	q;

	LCHAIN	BestChain = NullLchain;
	float	BestDist  = -1;
	float	Dist;
	int		Segment;

	/* Set to reasonable default values */
	if (pclosest) copy_point(pclosest, p);
	if (dist)     *dist    = BestDist;
	if (segment)  *segment = -1;

	/* Return if set doesn't contain a set of link chains */
	if (!p)                        return NullLchain;
	if (!set)                      return NullLchain;
	if (set->num <= 0)             return NullLchain;
	if (!same(set->type,"lchain")) return NullLchain;

	/* Examine all link chains in the set */
	for (ichain=0; ichain<set->num; ichain++)
		{
		chain = (LCHAIN) set->list[ichain];
		if (!chain) continue;
		lchain_test_point(chain, p, &Dist, q, &Segment, NullChar);
		if ((!BestChain) || ((Dist >= 0) && (Dist < BestDist)))
			{
			BestChain = chain;
			BestDist = Dist;
			if (pclosest) copy_point(pclosest, q);
			if (dist)     *dist    = Dist;
			if (segment)  *segment = Segment;
			}
		}

	/* Return the closest link chain */
	return BestChain;
	}

/*********************************************************************/
/** Find closest link node in the given link chain set to the given point.
 *
 *	@param[in]	 	set			link chain set to be examined
 *	@param[in]	 	p			reference point
 *	@param[out] 	*dist		distance to closest link node
 *	@param[out] 	pclosest	closest node on link chain
 *	@param[out] 	*ltype		type of closest node
 *	@param[out] 	*inode		index of closest node
 *  @return Pointer to link chain object with node closest to given point.
 *********************************************************************/

LCHAIN	closest_lchain_node

	(
	SET		set,
	POINT	p,
	float	*dist,
	POINT	pclosest,
	LMEMBER	*ltype,
	int		*inode
	)

	{
	int		ichain, ii;
	LCHAIN	chain;
	LNODE	lnode;
	LINTERP	linterp;
	POINT	q;

	LCHAIN	BestChain = NullLchain;
	float	BestDist  = -1;
	float	Dist;
	int		Segment;

	/* Set to reasonable default values */
	if (pclosest) copy_point(pclosest, p);
	if (dist)  *dist  = BestDist;
	if (ltype) *ltype = LchainUnknown;
	if (inode) *inode = -1;

	/* Return if set doesn't contain a set of link chains */
	if (!p)                        return NullLchain;
	if (!set)                      return NullLchain;
	if (set->num <= 0)             return NullLchain;
	if (!same(set->type,"lchain")) return NullLchain;

	/* Examine all link chains in the set */
	for (ichain=0; ichain<set->num; ichain++)
		{
		chain = (LCHAIN) set->list[ichain];
		if (!chain) continue;
		
#		ifdef DEBUG_CLOSEST
		(void) pr_status("Lnodes",
			"closest_lchain_node[] chain: %d\n", ichain);
#		endif /* DEBUG_CLOSEST */

		/* First test link nodes */
		for (ii=0; ii<chain->lnum; ii++)
			{
			lnode = chain->nodes[ii];
			Dist  = point_dist(p, lnode->node);
		
#			ifdef DEBUG_CLOSEST
			(void) pr_status("Lnodes",
				"  lnode: %d  Dist: %.2f\n", ii, Dist);
#			endif /* DEBUG_CLOSEST */

			if ((!BestChain) || ((Dist >= 0) && (Dist < BestDist)))
				{
				BestChain = chain;
				BestDist  = Dist;
				if (pclosest) copy_point(pclosest, lnode->node);
				if (dist)  *dist  = BestDist;
				if (ltype) *ltype = lnode->ltype;
				if (inode) *inode = ii;
				}
			}

		/* Last test interpolated nodes */
		for (ii=0; ii<chain->inum; ii++)
			{
			linterp = chain->interps[ii];
			Dist  = point_dist(p, linterp->node);
		
#			ifdef DEBUG_CLOSEST
			(void) pr_status("Lnodes",
				"  linterp: %d  Dist: %.2f\n", ii, Dist);
#			endif /* DEBUG_CLOSEST */

			if ((!BestChain) || ((Dist >= 0) && (Dist < BestDist)))
				{
				BestChain = chain;
				BestDist  = Dist;
				if (pclosest) copy_point(pclosest, linterp->node);
				if (dist)  *dist  = BestDist;
				if (ltype) *ltype = LchainInterp;
				if (inode) *inode = ii;
				}
			}
		}

	/* Return the link chain containing the closest node */
	return BestChain;
	}
