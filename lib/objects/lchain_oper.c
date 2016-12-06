/*********************************************************************/
/**	@file lchain_oper.c
 *
 * Assorted operations on link chains.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    l c h a i n _ o p e r . c                                         *
*                                                                      *
*    Assorted operations on link chains.                               *
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

#include <fpa_getmem.h>
#include <stdio.h>
#include <string.h>

#undef DEBUG_LCHAINS

/* Internal static functions */

/***********************************************************************
*                                                                      *
*      l c h a i n _ p r o p e r t i e s                               *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Measure certain 2-D properties of a link chain.
 *
 * Properties to measure:
 * 	- length of link chain
 *
 *	@param[in] 	lchain		given link chain
 *	@param[out]	*length		length of link chain
 *********************************************************************/

void	lchain_properties

	(
	LCHAIN	lchain,
	float	*length
	)

	{
	LINE	track;

	/* Initialize to nice garbage */
	if (length) *length = 0;

	/* Return if link chain not there */
	if (!lchain) return;

	/* Interpolate link chain track (if required) */
	if (lchain->dointerp) (void) interpolate_lchain(lchain);

	/* Return now if link chain interpolations cannot be determined */
	if (lchain->inum <= 0) return;

	/* Determine properties from link chain track */
	track = lchain->track;
	line_properties(track, NullChar, NullChar, NullFloat, length);
	}

/***********************************************************************
*                                                                      *
*      l c h a i n _ t e s t _ p o i n t                               *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Measure certain 2-D properties of a test point, relative to
 * the given link chain.
 *
 * Properties to measure:
 * 	- minimum (perpendicular) distance from point to link chain
 * 	- closest point on link chain (interpolated if necessary)
 * 	- index of interpolated segment which contains closest point
 * 	- point is on left or right side of link chain
 *
 *	@param[in] 	lchain		given link chain
 *	@param[in] 	ptest		given test point
 *	@param[out]	*pdist		minimum (perpendicular) distance to link chain
 *	@param[out]	ppoint		closest (perpendicular) point on link chain
 *	@param[out]	*pseg		index of segment which contains closest point
 *	@param[out]	*right		is point on right side of link chain?
 *********************************************************************/

void	lchain_test_point

	(
	LCHAIN	lchain,
	POINT	ptest,
	float	*pdist,
	POINT	ppoint,
	int		*pseg,
	LOGICAL	*right
	)

	{
	LINE	track;

	/* Initialize to nice garbage */
	if (ppoint) copy_point(ppoint, ptest);
	if (pdist)  *pdist  = -1;
	if (pseg)   *pseg   = -1;
	if (right)  *right  = FALSE;

	/* Return if link chain not there or no test point */
	if (!lchain) return;
	if (!ptest)  return;

	/* Interpolate link chain track (if required) */
	if (lchain->dointerp) (void) interpolate_lchain(lchain);

	/* Return now if link chain interpolations cannot be determined */
	if (lchain->inum <= 0) return;

	/* Determine properties from link chain track */
	track = lchain->track;
	line_test_point(track, ptest, pdist, ppoint, pseg, NullLogical, right);
	}

/***********************************************************************
*                                                                      *
*     i n b o x _ l c h a i n                                          *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Determine if the given link chain passes through the given box.
 *
 *	@param[in] 	lchain	given link chain
 *	@param[in] 	*box	box to test against
 * @return True if link chain passes through the box.
 *********************************************************************/

LOGICAL	inbox_lchain

	(
	LCHAIN		lchain,
	const BOX	*box
	)

	{
	int		ip;

	/* Return if link chain not there or no test box */
	if (!lchain) return FALSE;
	if (!box)    return FALSE;

	/* Interpolate link chain track (if required) */
	if (lchain->dointerp) (void) interpolate_lchain(lchain);

	/* Return now if link chain interpolations cannot be determined */
	if (lchain->inum <= 0) return FALSE;

	/* Return true if any point on the track is inside the box */
	for (ip=0; ip<lchain->inum; ip++)
		{
		if (inside_box(box, lchain->interps[ip]->node)) return TRUE;
		}

	return FALSE;
	}

/***********************************************************************
*                                                                      *
*      t r a n s l a t e _ l c h a i n                                 *
*      r o t a t e _ l c h a i n                                       *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Translate the given link chain.
 *
 *	@param[in] 	lchain	given link chain
 *	@param[in] 	dx		x offset
 *	@param[in] 	dy		y offset
 *  @return True if successful.
 *********************************************************************/
LOGICAL	translate_lchain

	(
	LCHAIN	lchain,
	float	dx,
	float	dy
	)

	{
	int		ip;

	/* Return if link chain not there or no translation parameters */
	if (!lchain)        return FALSE;
	if (dx==0 && dy==0) return TRUE;

	/* Translate the link nodes */
	for (ip=0; ip<lchain->lnum; ip++)
		{
		lchain->nodes[ip]->node[X] += dx;
		lchain->nodes[ip]->node[Y] += dy;
		}

	/* Translate the interpolated nodes */
	for (ip=0; ip<lchain->inum; ip++)
		{
		lchain->interps[ip]->node[X] += dx;
		lchain->interps[ip]->node[Y] += dy;
		}

	/* Translate the link chain track */
	(void) translate_line(lchain->track, dx, dy);

	return TRUE;
	}

/*********************************************************************/
/** Rotate the given link chain.
 *
 *	@param[in] 	lchain	given link chain
 *	@param[in] 	ref		centre of rotation
 *	@param[in] 	angle	angle of rotation
 *  @return True if successful.
 *********************************************************************/
LOGICAL	rotate_lchain

	(
	LCHAIN	lchain,
	POINT	ref,
	float	angle
	)

	{
	int		ip;
	float	ca, sa, x, y;

	/* Return if link chain not there or no rotation parameter */
	if (!lchain)    return FALSE;
	if (angle == 0) return TRUE;

	/* Turn angle into cos and sin */
	ca = cosdeg(angle);
	sa = sindeg(angle);

	/* Rotate the link nodes */
	for (ip=0; ip<lchain->lnum; ip++)
		{
		x = lchain->nodes[ip]->node[X] - ref[X];
		y = lchain->nodes[ip]->node[Y] - ref[Y];

		lchain->nodes[ip]->node[X] = x*ca - y*sa + ref[X];
		lchain->nodes[ip]->node[Y] = x*sa + y*ca + ref[Y];
		}

	/* Rotate the interpolated nodes */
	for (ip=0; ip<lchain->inum; ip++)
		{
		x = lchain->interps[ip]->node[X] - ref[X];
		y = lchain->interps[ip]->node[Y] - ref[Y];

		lchain->interps[ip]->node[X] = x*ca - y*sa + ref[X];
		lchain->interps[ip]->node[Y] = x*sa + y*ca + ref[Y];
		}

	/* Rotate the link chain track */
	(void) rotate_line(lchain->track, ref, angle);

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*      i n t e r p o l a t e _ l c h a i n                             *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Determine interpolated nodes for the given link chain.
 *
 *	@param[in] 	lchain	given link chain
 *  @return True if successful.
 *********************************************************************/
LOGICAL	interpolate_lchain

	(
	LCHAIN	lchain
	)

	{
	int		mplus, nplus, fmplus;
	int		stime, etime, tdiff, numtween, start, end, ntween;
	int		inode, xnode, nkey, akey, itween, ikey, nnum, nbgn, nend;
	double	dx, dy;
	LOGICAL	isnode, isdepict;
	POINT	pos;
	LNODE	lnode;
	LINTERP	linterp;

	static	int		maxkey = 0;
	static	double	*keytimes  = NullDouble;
	static	LOGICAL	*keynormal = NullLogicalPtr;
	static	double	*keyx = NullDouble, *keyy = NullDouble;
	static	int		maxatt = 0;
	static	double	*atttimes  = NullDouble;
	static	LOGICAL	*attnormal = NullLogicalPtr;
	static	int		*attnodes  = NullInt;
	static	int		maxtween = 0;
	static	double	*tweentimes = NullDouble;
	static	double	*tweenx = NullDouble, *tweeny = NullDouble;

	/* Return if link chain not there or no link nodes to interpolate */
	if (!lchain)           return FALSE;
	if (lchain->lnum <= 0) return FALSE;

	/* Return if invalid interpolation delta */
	if (lchain->minterp <= 0)
		{
		(void) pr_error("Lchains",
				"Invalid interpolation delta: %d\n", lchain->minterp);
		return FALSE;
		}

#	ifdef DEBUG_LCHAINS
	if (!lchain->dointerp)
		(void) pr_diag("LchainInterp",
			"In interpolate_lchain() without needing to be?\n");
#	endif /* DEBUG_LCHAINS */

	/* Remove existing interpolated nodes */
	if (lchain->inum > 0)
		{
		for (inode=0; inode<lchain->inum; inode++)
			lchain->interps[inode] = destroy_linterp(lchain->interps[inode]);
		FREEMEM(lchain->interps);
		lchain->inum = 0;
		}

	/* Initialize the link chain track */
	if (!lchain->track)
		lchain->track = create_line();
	else                
		(void) empty_line(lchain->track);

	/* Allocate working space for track nodes */
	if (lchain->lnum > maxkey)
		{
		maxkey    = lchain->lnum;
		keytimes  = GETMEM(keytimes,  double,  maxkey);
		keynormal = GETMEM(keynormal, LOGICAL, maxkey);
		keyx      = GETMEM(keyx,      double,  maxkey);
		keyy      = GETMEM(keyy,      double,  maxkey);
		}

	/* Allocate working space for attribute nodes */
	if (lchain->lnum > maxatt)
		{
		maxatt    = lchain->lnum;
		atttimes  = GETMEM(atttimes,  double,  maxatt);
		attnormal = GETMEM(attnormal, LOGICAL, maxatt);
		attnodes  = GETMEM(attnodes,  int,     maxatt);
		}

	/* Build track and attribute buffers from link/control/floating nodes */
	for (inode=0, nkey=0, akey=0; inode<lchain->lnum; inode++)
		{

		/* Use only existing link/control/floating nodes */
		lnode = lchain->nodes[inode];
		if (!lnode->there)                  continue;
		if (lnode->ltype == LchainUnknown)  continue;
		if (lnode->ltype == LchainInterp)   continue;

		/* Set the link node time */
		mplus = lnode->mplus;

		/* Save the link node information for tracking */
		if (lnode->ltype == LchainNode)
			{
			keytimes[nkey]  = (double) mplus;
			keynormal[nkey] = TRUE;
			keyx[nkey]      = lnode->node[X];
			keyy[nkey]      = lnode->node[Y];
			nkey++;
			}

		/* Save the control node information for tracking */
		else if (lnode->ltype == LchainControl)
			{
			keytimes[nkey]  = (double) mplus;
			keynormal[nkey] = FALSE;
			keyx[nkey]      = lnode->node[X];
			keyy[nkey]      = lnode->node[Y];
			nkey++;
			}

		/* Save the link node information for attributes */
		if (lnode->ltype == LchainNode)
			{
			atttimes[akey]  = (double) mplus;
			attnormal[akey] = TRUE;
			attnodes[akey]  = inode;
			akey++;
			}

		/* Save the floating node information for attributes */
		else if (lnode->ltype == LchainFloating)
			{
			atttimes[akey]  = (double) mplus;
			attnormal[akey] = FALSE;
			attnodes[akey]  = inode;
			akey++;
			}
		}

	/* Return now if no active nodes for tracking */
	if (nkey == 0) return TRUE;

#	ifdef DEBUG_LCHAINS
	(void) pr_diag("LchainInterp",
		"Interpolating from %d (of %d) key frames\n", nkey, lchain->lnum);
	for (ikey=0; ikey<nkey; ikey++)
		{
		if (keynormal[ikey])
			(void) pr_diag("LchainInterp",
				"  Keyframe %3d at% 5d  %6.2f/%6.2f N\n",
				ikey, NINT(keytimes[ikey]), keyx[ikey], keyy[ikey]);
		else
			(void) pr_diag("LchainInterp",
				"  Keyframe %3d at% 5d  %6.2f/%6.2f C\n",
				ikey, NINT(keytimes[ikey]), keyx[ikey], keyy[ikey]);
		}
#	endif /* DEBUG_LCHAINS */

	/* Set start and end times for interpolations */
	stime = MIN(NINT(keytimes[0]),      lchain->splus);
	etime = MAX(NINT(keytimes[nkey-1]), lchain->eplus);

#	ifdef DEBUG_LCHAINS
	(void) pr_diag("LchainInterp",
		"Interpolating from %d to %d for chain from %d to %d\n",
		stime, etime, lchain->splus, lchain->eplus);
#	endif /* DEBUG_LCHAINS */

	/* Determine number of interpolated nodes */
	tdiff = etime - stime;
	numtween = tdiff / lchain->minterp;
	if (numtween*lchain->minterp != tdiff)
		{
		(void) pr_warning("Lchains",
				"Problem with interpolation delta: %d  for chain: %d to %d\n",
				lchain->minterp, lchain->splus, lchain->eplus);
		}
	numtween++;

	/* Determine extent of link nodes based on start and end times */
	tdiff = NINT(keytimes[0] - (double) stime);
	start = tdiff / lchain->minterp;
	if (start*lchain->minterp != tdiff)
		{
		(void) pr_warning("Lchains",
				"Problem with interpolation delta: %d  for chain start: %d  and first node: %d\n",
				lchain->minterp, lchain->splus, NINT(keytimes[0]));
		}
	tdiff = NINT(keytimes[nkey-1] - (double) stime);
	end   = tdiff / lchain->minterp;
	if (end*lchain->minterp != tdiff)
		{
		(void) pr_warning("Lchains",
				"Problem with interpolation delta: %d  for chain start: %d  and last node: %d\n",
				lchain->minterp, lchain->splus, NINT(keytimes[nkey-1]));
		}
	ntween = end - start + 1;

	/* Allocate space for interpolation of nodes */
	if (numtween > maxtween)
		{
		maxtween   = numtween;
		tweentimes = GETMEM(tweentimes, double, maxtween);
		tweenx     = GETMEM(tweenx,     double, maxtween);
		tweeny     = GETMEM(tweeny,     double, maxtween);
		}

#	ifdef DEBUG_LCHAINS
	(void) pr_diag("LchainInterp",
		"Interpolating %d of %d frames from nodes between %d and %d\n",
		ntween, numtween, start, end);
#	endif /* DEBUG_LCHAINS */

	/* Build the interpolation time/node buffer */
	for (itween=0; itween<numtween; itween++)
		{
		tweentimes[itween] = (double) (stime + itween*lchain->minterp);
		}

	/* Handle locations between link nodes */
	if (ntween == 1)
		{
		tweenx[start] = keyx[0];
		tweeny[start] = keyy[0];
		}
	else
		{
		/* Interpolate the link node locations */
		QuasiLinear_Tween(nkey,   keytimes,         keyx,         keyy,
						  ntween, tweentimes+start, tweenx+start, tweeny+start);
		}

	/* Handle early start or late end times */
	if (start > 0)
		{

#		ifdef DEBUG_LCHAINS
		(void) pr_diag("LchainInterp",
				"Extrapolating frames 0 to %d for early start time %d (first node %d)\n",
				start-1, lchain->splus, NINT(keytimes[0]));
#		endif /* DEBUG_LCHAINS */

		dx = 0;
		dy = 0;
		if (ntween > 1)
			{
			dx = tweenx[start+1] - tweenx[start];
			dy = tweeny[start+1] - tweeny[start];
			}
		for (itween=start-1; itween>=0; itween--)
			{
			tweenx[itween] = tweenx[itween+1] - dx;
			tweeny[itween] = tweeny[itween+1] - dy;
			}
		}
	if (end < numtween-1)
		{

#		ifdef DEBUG_LCHAINS
		(void) pr_diag("LchainInterp",
				"Extrapolating frames %d to %d for late end time %d (last node %d)\n",
				end+1, numtween-1, lchain->eplus, NINT(keytimes[nkey-1]));
#		endif /* DEBUG_LCHAINS */

		dx = 0;
		dy = 0;
		if (ntween > 1)
			{
			dx = tweenx[end] - tweenx[end-1];
			dy = tweeny[end] - tweeny[end-1];
			}
		for (itween=end+1; itween<numtween; itween++)
			{
			tweenx[itween] = tweenx[itween-1] + dx;
			tweeny[itween] = tweeny[itween-1] + dy;
			}
		}

	/* Set parameters for first node */
	fmplus = (akey > 0)? NINT(atttimes[0]):  0;
	xnode  = (akey > 0)? attnodes[0]:       -1;

	/* Construct point list for interpolated nodes */
	for (itween=0, ikey=0; itween<numtween; itween++)
		{

		/* Check link chain start and end time for interpolated nodes */
		mplus = NINT(tweentimes[itween]);
		if (mplus < lchain->splus) continue;
		if (mplus > lchain->eplus) continue;

		/* Get space for interpolated link node */
		lchain->inum++;
		lchain->interps = GETMEM(lchain->interps, LINTERP, lchain->inum);
		inode = lchain->inum - 1;
		lchain->interps[inode] = create_linterp(mplus);

		/* Check if the interpolated node is at a link node time */
		/*  and reset the matching node for attributes           */
		isnode   = FALSE;
		isdepict = FALSE;

		/* Set default parameters for no attributes */
		if (akey == 0)
			{
			}

		/* Check for early start times (before first node) */
		else if (ikey == 0 && mplus < fmplus)
			{
			}

		/* Check if the time matches the first node */
		else if (ikey == 0 && mplus == fmplus)
			{
			isnode   = TRUE;
			isdepict = attnormal[0];
			}

		/* Check if the time matches the next node */
		else if (ikey+1 < akey)
			{
			nplus = NINT(atttimes[ikey+1]);
			if (mplus == nplus)
				{
				ikey++;
				isnode   = TRUE;
				isdepict = attnormal[ikey];
				xnode    = attnodes[ikey];
				}
			else if (mplus > nplus)
				{
				ikey++;
				xnode    = attnodes[ikey];
				}
			}

		/* Save the interpolated link node locations */
		set_point(pos, (float) tweenx[itween], (float) tweeny[itween]);
		define_linterp_node(lchain->interps[inode], TRUE, isdepict, pos);

		/* Set the attributes for interpolated node (if available) */
		if (xnode >= 0)
			define_linterp_attribs(lchain->interps[inode],
									lchain->nodes[xnode]->attrib);

#		ifdef DEBUG_LCHAINS
		if (isnode && isdepict)
			(void) pr_diag("LchainInterp",
					"  Interpolated point: %3d at:% 5d  %6.2f/%6.2f  node: %d N\n",
					inode, NINT(tweentimes[itween]),
					lchain->interps[inode]->node[X],
					lchain->interps[inode]->node[Y], xnode);
		else if (isnode)
			(void) pr_diag("LchainInterp",
					"  Interpolated point: %3d at:% 5d  %6.2f/%6.2f  node: %d F\n",
					inode, NINT(tweentimes[itween]),
					lchain->interps[inode]->node[X],
					lchain->interps[inode]->node[Y], xnode);
		else
			(void) pr_diag("LchainInterp",
					"  Interpolated point: %3d at:% 5d  %6.2f/%6.2f  node: %d\n",
					inode, NINT(tweentimes[itween]),
					lchain->interps[inode]->node[X],
					lchain->interps[inode]->node[Y], xnode);
#		endif /* DEBUG_LCHAINS */
		}

	/* Reset position of floating nodes to interpolated position */
	for (xnode=0; xnode<lchain->lnum; xnode++)
		{

		/* Check only active floating nodes */
		lnode = lchain->nodes[xnode];
		if (!lnode->there || lnode->ltype != LchainFloating) continue;
		mplus = lnode->mplus;

		/* Check for matching interpolated node */
		for (inode=0; inode<lchain->inum; inode++)
			{
			linterp = lchain->interps[inode];
			if (linterp->mplus == mplus)
				{
				if (point_dist2(lnode->node, linterp->node) > 0.0)
					{

#					ifdef DEBUG_LCHAINS
					(void) pr_diag("LchainInterp",
						"Reset position of floating node: %d  at: %d\n",
						xnode, mplus);
#					endif /* DEBUG_LCHAINS */

					(void) copy_point(lnode->node, linterp->node);
					}
				break;
				}
			}
		}

	/* Reset attributes of control nodes from interpolated nodes */
	for (xnode=0; xnode<lchain->lnum; xnode++)
		{

		/* Check only active control nodes */
		lnode = lchain->nodes[xnode];
		if (!lnode->there || lnode->ltype != LchainControl) continue;
		mplus = lnode->mplus;

		/* Check for matching interpolated node */
		for (inode=0; inode<lchain->inum; inode++)
			{
			linterp = lchain->interps[inode];
			if (linterp->mplus == mplus)
				{

				/* Set the attributes from the interpolated node */
				lnode->attrib = destroy_attrib_list(lnode->attrib);
				if (NotNull(linterp->attrib))
					lnode->attrib = copy_attrib_list(linterp->attrib);

				/* Reset the default control node attributes */
				define_lnode_default_attribs(lnode);

#				ifdef DEBUG_LCHAINS
				(void) pr_diag("LchainInterp",
					"Reset attributes of control node: %d  at: %d\n",
					xnode, mplus);
#				endif /* DEBUG_LCHAINS */

				break;
				}
			}
		}

	/* Build the link chain track from the interpolated nodes */
	for (inode=0; inode<lchain->inum; inode++)
		(void) add_point_to_line(lchain->track, lchain->interps[inode]->node);

	/* Return interpolated link chain */
	lchain->dointerp = FALSE;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*      l c h a i n _ c l o s e s t _ n o d e                           *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Determine closest node in a link chain to a given test point.
 *
 *	@param[in] 	lchain		given link chain
 *	@param[in] 	dtol		tolerance for checking for closest node
 *	@param[in] 	atol		tolerance for checking for ambiguous nodes
 *	@param[in] 	ptest		reference point
 *	@param[out]	*pdist		distance to closest node
 *	@param[out]	ppos		location of closest node
 *	@param[out]	*ltype		type of closest node
 *	@param[out]	*inum		number of interpolated nodes near reference point
 *	@param[out]	**inodes	index of interpolated nodes near reference point
 *  @return index of closest node if successful.
 *********************************************************************/
int		lchain_closest_node

	(
	LCHAIN	lchain,
	float	dtol,
	float	atol,
	POINT	ptest,
	float	*pdist,
	POINT	ppos,
	LMEMBER *ltype,
	int		*inum,
	int		**inodes
	)

	{
	LOGICAL	first;
	int		ii, inode, il, mplus;
	float	dist, bdist;
	LNODE	lnode;
	LINTERP	linterp;

	static	int		NumNode = 0, MaxNode = 0;
	static	int		*INodes = NullInt;

	/* Initialize to nice garbage */
	if (ppos)   copy_point(ppos, ZeroPoint);
	if (pdist)  *pdist  = -1;
	if (ltype)  *ltype  = LchainUnknown;
	if (inum)   *inum   =  0;
	if (inodes) *inodes = NullInt;

	/* Return if no link chain or no link nodes */
	if (!lchain)           return -1;
	if (lchain->lnum <= 0) return -1;

	/* Interpolate link chain (if required) */
	if (lchain->dointerp) (void) interpolate_lchain(lchain);

	/* Determine closest interpolated node to reference point */
	if (dtol > 0.0)
		{
		first = FALSE;
		bdist = dtol;
		}
	else
		{
		first = TRUE;
		bdist = 0.0;
		}
	for (inode=-1, ii=0; ii<lchain->inum; ii++)
		{

		/* Check only active interpolated nodes */
		linterp = lchain->interps[ii];
		if (!linterp->there) continue;

		/* Determine distance to interpolated node */
		dist = point_dist(ptest, linterp->node);
		if (dtol > 0.0 && dist > dtol) continue;
		if (!first && dist >= bdist)   continue;

		/* Closest interpolated node so far */
		bdist = dist;
		inode = ii;
		first = FALSE;
		}

	/* Return if no interpolated node is close enough */
	if (inode < 0) return inode;
	mplus = lchain->interps[inode]->mplus;

	/* Determine interpolated nodes very close to closest node (if required) */
	NumNode = 0;
	if (atol > 0.0)
		{
		for (ii=0; ii<lchain->inum; ii++)
			{

			/* Check only active interpolated nodes */
			linterp = lchain->interps[ii];
			if (!linterp->there) continue;

			/* Determine distance to closest interpolated node */
			dist = point_dist(lchain->interps[inode]->node, linterp->node);
			if (dist > atol) continue;

			/* Save all interpolated nodes very close to closest node */
			NumNode++;
			if (NumNode > MaxNode)
				{
				MaxNode = NumNode;
				INodes  = GETMEM(INodes, int, MaxNode);
				}
			INodes[NumNode-1] = ii;
			}
		}

	/* Set return parameters for ambiguous interpolated nodes */
	if (NumNode > 1)
		{
		if (ppos)   copy_point(ppos, lchain->interps[inode]->node);
		if (pdist)  *pdist  = bdist;
		if (ltype)  *ltype  = LchainInterp;
		if (inum)   *inum   = NumNode;
		if (inodes) *inodes = INodes;

		/* Return closest interpolated node */
		return inode;
		}

	/* Set return parameters for link node */
	/*  if one exists at the matching time */
	il = which_lchain_node(lchain, LchainNode, mplus);
	if (il >= 0 && lchain->nodes[il]->there)
		{
		if (ppos)   copy_point(ppos, lchain->nodes[il]->node);
		if (pdist)  *pdist  = bdist;
		if (ltype)  *ltype  = LchainNode;
		if (inum)   *inum   = NumNode;
		if (inodes) *inodes = INodes;

		/* Return closest link node */
		return il;
		}

	/* Set return parameters for control node */
	/*  if one exists at the matching time    */
	il = which_lchain_node(lchain, LchainControl, mplus);
	if (il >= 0 && lchain->nodes[il]->there)
		{
		if (ppos)   copy_point(ppos, lchain->nodes[il]->node);
		if (pdist)  *pdist  = bdist;
		if (ltype)  *ltype  = LchainControl;
		if (inum)   *inum   = NumNode;
		if (inodes) *inodes = INodes;

		/* Return closest control node */
		return il;
		}

	/* Set return parameters for floating node */
	/*  if one exists at the matching time     */
	il = which_lchain_node(lchain, LchainFloating, mplus);
	if (il >= 0 && lchain->nodes[il]->there)
		{
		if (ppos)   copy_point(ppos, lchain->nodes[il]->node);
		if (pdist)  *pdist  = bdist;
		if (ltype)  *ltype  = LchainFloating;
		if (inum)   *inum   = NumNode;
		if (inodes) *inodes = INodes;

		/* Return closest control node */
		return il;
		}

	/* Set return parameters for interpolated node */
	if (ppos)   copy_point(ppos, lchain->interps[inode]->node);
	if (pdist)  *pdist  = bdist;
	if (ltype)  *ltype  = LchainInterp;
	if (inum)   *inum   = NumNode;
	if (inodes) *inodes = INodes;

	/* Return closest interpolated node */
	return inode;
	}

/***********************************************************************
*                                                                      *
*      p r o m o t e _ l c h a i n _ n o d e                           *
*                                                                      *
*      Promote a link node to another type.                            *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Promote a link node that matches the given time to the given type.
 *
 * @param[in]	lchain	link chain containing node to promote.
 * @param[in] 	ptype	type of node to promote to.
 * @param[in] 	mplus	time to match.
 ***********************************************************************/

void	promote_lchain_node

	(
	LCHAIN	lchain,
	LMEMBER	ptype,
	int		mplus
	)

	{
	int			inode, jnode;
	LMEMBER		ltype;
	ATTRIB_LIST	attrib;
	POINT		pos;
	LNODE		lnode;
	LINTERP		linterp;

	/* Return if no link chain or incorrect type for promotion */
	if (!lchain)                return;
	if (ptype == LchainUnknown) return;
	if (ptype == LchainInterp)  return;

	/* Interpolate link chain (if required) */
	if (lchain->dointerp) (void) interpolate_lchain(lchain);

	/* Find a link node at this time */
	inode = which_lchain_node(lchain, LchainNode, mplus);
	if (inode >= 0 && lchain->nodes[inode]->there)
		{
		ltype  = LchainNode;
		attrib = lchain->nodes[inode]->attrib;
		copy_point(pos, lchain->nodes[inode]->node);
		}

	/* Not found ... keep looking */
	else
		{

		/* Find a control node at this time */
		/* Note that attributes from control nodes are extracted */
		/*  from the interpolated node at the matching time!     */
		inode = which_lchain_node(lchain, LchainControl, mplus);
		if (inode >= 0 && lchain->nodes[inode]->there)
			{
			ltype  = LchainControl;
			jnode  = which_lchain_node(lchain, LchainInterp, mplus);
			if (jnode >= 0)
				attrib = lchain->interps[jnode]->attrib;
			else
				attrib = lchain->nodes[inode]->attrib;
			copy_point(pos, lchain->nodes[inode]->node);
			}

		/* Not found ... keep looking */
		else
			{

			/* Find a floating node at this time */
			inode = which_lchain_node(lchain, LchainFloating, mplus);
			if (inode >= 0 && lchain->nodes[inode]->there)
				{
				ltype  = LchainFloating;
				attrib = lchain->nodes[inode]->attrib;
				copy_point(pos, lchain->nodes[inode]->node);
				}

			/* Not found ... keep looking */
			else
				{

				/* Find an interpolated node at this time */
				inode = which_lchain_node(lchain, LchainInterp, mplus);
				if (inode >= 0)
					{
					ltype  = LchainInterp;
					attrib = lchain->interps[inode]->attrib;
					copy_point(pos, lchain->interps[inode]->node);
					}

				/* Return if no node found at this time */
				else
					{
					return;
					}
				}
			}
		}

	/* Check for promotion of interpolated node to link node */
	if (ltype == LchainInterp && ptype == LchainNode)
		{

		/* Create a link node and add it to the link chain */
		lnode = create_lnode(mplus);
		define_lnode_type(lnode, TRUE, FALSE, LchainNode);
		define_lnode_node(lnode, pos);
		define_lnode_attribs(lnode, attrib);
		(void) add_lchain_lnode(lchain, lnode);
		}

	/* Check for promotion of interpolated node to control node */
	else if (ltype == LchainInterp && ptype == LchainControl)
		{

		/* Create a link node and add it to the link chain */
		lnode = create_lnode(mplus);
		define_lnode_type(lnode, TRUE, FALSE, LchainControl);
		define_lnode_node(lnode, pos);
		define_lnode_attribs(lnode, attrib);
		(void) add_lchain_lnode(lchain, lnode);
		}

	/* Check for promotion of interpolated node to floating node */
	else if (ltype == LchainInterp && ptype == LchainFloating)
		{

		/* Create a link node and add it to the link chain */
		lnode = create_lnode(mplus);
		define_lnode_type(lnode, TRUE, FALSE, LchainFloating);
		define_lnode_node(lnode, pos);
		define_lnode_attribs(lnode, attrib);
		(void) add_lchain_lnode(lchain, lnode);
		}

	/* Check for promotion of floating node to link node */
	else if (ltype == LchainFloating && ptype == LchainNode)
		{

		/* Change the node type */
		lnode = lchain->nodes[inode];
		define_lnode_type(lnode, TRUE, FALSE, LchainNode);
		lchain->dointerp = TRUE;
		}

	/* Check for promotion of control node to link node */
	else if (ltype == LchainControl && ptype == LchainNode)
		{

		/* Change the node type and set the attributes */
		lnode = lchain->nodes[inode];
		define_lnode_type(lnode, TRUE, FALSE, LchainNode);
		define_lnode_attribs(lnode, attrib);
		lchain->dointerp = TRUE;
		}

	/* Check for promotion of guess control node to control node */
	else if (ltype == LchainControl && ptype == LchainControl)
		{
		lnode = lchain->nodes[inode];
		lnode->guess = FALSE;
		}

	/* Check for promotion of guess link node to link node */
	else if (ltype == LchainNode && ptype == LchainNode)
		{
		lnode = lchain->nodes[inode];
		lnode->guess = FALSE;
		}

	/* Re-interpolate link chain (if required) */
	if (lchain->dointerp) (void) interpolate_lchain(lchain);
	}

/***********************************************************************
*                                                                      *
*      n e a r e s t _ l c h a i n _ l n o d e                         *
*      n e a r e s t _ l c h a i n _ l i n t e r p                     *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Determine closest link node to a given test point.
 *
 *	@param[in] 	lchain		given link chain
 *	@param[in] 	dtol		tolerance for checking nodes
 *	@param[in] 	ptest		reference point
 *	@param[out]	*pdist		distance to nearest link node
 *	@param[out]	ppos		location of nearest link node
 *	@param[out]	*lnum		number of link nodes near reference point
 *	@param[out]	**lnodes	index of link nodes near reference point
 *  @return index of closest link node if successful.
 *********************************************************************/
int		nearest_lchain_lnode

	(
	LCHAIN	lchain,
	float	dtol,
	POINT	ptest,
	float	*pdist,
	POINT	ppos,
	int		*lnum,
	int		**lnodes
	)

	{
	int		ii, inode;
	float	xtol, dist, bdist;
	LNODE	lnode;

	static	int		NumLNode = 0, MaxLNode = 0;
	static	int		*LNodes = NullInt;

	/* Initialize to nice garbage */
	if (ppos)   copy_point(ppos, ZeroPoint);
	if (pdist)  *pdist  = -1;
	if (lnum)   *lnum   =  0;
	if (lnodes) *lnodes = NullInt;

	/* Return if no link chain or no link nodes */
	if (!lchain)           return -1;
	if (lchain->lnum <= 0) return -1;

	/* Determine closest link node to reference point */
	inode = -1;
	bdist = dtol;
	for (ii=0; ii<lchain->lnum; ii++)
		{

		/* Check only active link nodes */
		lnode = lchain->nodes[ii];
		if (!lnode->there) continue;

		/* Determine distance to link node */
		dist = point_dist(ptest, lnode->node);
		if (dist > dtol)  continue;
		if (dist > bdist) continue;

		/* Closest link node so far */
		if (dist < bdist)
			{
			bdist = dist;
			inode = ii;
			}
		}

	/* Return if no link node is close enough */
	if (inode < 0) return inode;

	/* Set very close tolerance for ambiguous link nodes */
	xtol = dtol/100.0;

	/* Determine link nodes very near to closest node */
	NumLNode = 0;
	for (ii=0; ii<lchain->lnum; ii++)
		{

		/* Check only active link nodes */
		lnode = lchain->nodes[ii];
		if (!lnode->there) continue;

		/* Determine distance to closest node */
		dist = point_dist(lchain->nodes[inode]->node, lnode->node);
		if (dist > xtol) continue;

		/* Save all link nodes very close to closest node */
		NumLNode++;
		if (NumLNode > MaxLNode)
			{
			MaxLNode = NumLNode;
			LNodes   = GETMEM(LNodes, int, MaxLNode);
			}
		LNodes[NumLNode-1] = ii;
		}

	/* Set return parameters */
	if (ppos)   copy_point(ppos, lchain->nodes[inode]->node);
	if (pdist)  *pdist  = bdist;
	if (lnum)   *lnum   = NumLNode;
	if (lnodes) *lnodes = LNodes;

	/* Return closest link node */
	return inode;
	}

/*********************************************************************/
/** Determine closest interpolated node to a given test point.
 *
 *	@param[in] 	lchain		given link chain
 *	@param[in] 	dtol		tolerance for checking nodes
 *	@param[in] 	ptest		reference point
 *	@param[out]	*pdist		distance to nearest interpolated node
 *	@param[out]	ppos		location of nearest interpolated node
 *	@param[out]	*inum		number of interpolated nodes near reference point
 *	@param[out]	**inodes	index of interpolated nodes near reference point
 *  @return index of closest interpolated node if successful.
 *********************************************************************/
int		nearest_lchain_linterp

	(
	LCHAIN	lchain,
	float	dtol,
	POINT	ptest,
	float	*pdist,
	POINT	ppos,
	int		*inum,
	int		**inodes
	)

	{
	int		ii, inode;
	float	xtol, dist, bdist;
	LINTERP	linterp;

	static	int		NumINode = 0, MaxINode = 0;
	static	int		*INodes = NullInt;

	/* Initialize to nice garbage */
	if (ppos)   copy_point(ppos, ZeroPoint);
	if (pdist)  *pdist  = -1;
	if (inum)   *inum   =  0;
	if (inodes) *inodes = NullInt;

	/* Return if no link chain or no interpolated nodes */
	if (!lchain)           return -1;
	if (lchain->inum <= 0) return -1;

	/* Determine closest interpolated node to reference point */
	inode = -1;
	bdist = dtol;
	for (ii=0; ii<lchain->inum; ii++)
		{

		/* Check only active interpolated nodes */
		linterp = lchain->interps[ii];
		if (!linterp->there) continue;

		/* Determine distance to interpolated node */
		dist = point_dist(ptest, linterp->node);
		if (dist > dtol)  continue;
		if (dist > bdist) continue;

		/* Closest interpolated node so far */
		if (dist < bdist)
			{
			bdist = dist;
			inode = ii;
			}
		}

	/* Return if no interpolated node is close enough */
	if (inode < 0) return inode;

	/* Set very close tolerance for ambiguous interpolated nodes */
	xtol = dtol/100.0;

	/* Determine interpolated nodes very near to closest node */
	NumINode = 0;
	for (ii=0; ii<lchain->inum; ii++)
		{

		/* Check only active interpolated nodes */
		linterp = lchain->interps[ii];
		if (!linterp->there) continue;

		/* Determine distance to closest node */
		dist = point_dist(lchain->interps[inode]->node, linterp->node);
		if (dist > xtol) continue;

		/* Save all interpolated nodes very close to closest node */
		NumINode++;
		if (NumINode > MaxINode)
			{
			MaxINode = NumINode;
			INodes   = GETMEM(INodes, int, MaxINode);
			}
		INodes[NumINode-1] = ii;
		}

	/* Set return parameters */
	if (ppos)   copy_point(ppos, lchain->interps[inode]->node);
	if (pdist)  *pdist  = bdist;
	if (inum)   *inum   = NumINode;
	if (inodes) *inodes = INodes;

	/* Return closest interpolated node */
	return inode;
	}
