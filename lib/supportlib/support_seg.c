/***********************************************************************
*                                                                      *
*   s u p p o r t _ s e g . c                                          *
*                                                                      *
*   Obsolescent functions to handle the SEGMENT object.                *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2002 Environment Canada (AES)            *
*     Version 7 (c) Copyright 2008 Environment Canada                  *
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

#include "support.h"

#include <string.h>
#include <stdio.h>

/***********************************************************************
*                                                                      *
*     n e x t _ s e g l i s t _ c r o s s i n g                        *
*                                                                      *
*      Obsolescent function!                                           *
*                                                                      *
*      Use equivalent seglist_crossing() function.                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** OBSOLETE!
 *
 * Find the next point at which the given line crosses the given
 * (not closed) segment list.  It is assumed that the lines cross any
 * number of times, or not at all.
 *
 *	@param[in] 	nseg		length of segment list
 *	@param[in] 	*seglist	segment list
 *	@param[in] 	line		line to cross
 *	@param[in] 	sseg		which segment to start at
 *	@param[in] 	sspan		span of sseg to start at
 *	@param[out]	cross		point of intersection
 *	@param[out]	*xseg		segment where crossover occurs
 *	@param[out]	*xspan		span on seg where crossover occurs
 *	@param[out]	*lspan		span on line where crossover occurs
 *	@param[out]	*right		now on right side?
 * 	@return True if successful.
 *********************************************************************/

LOGICAL	next_seglist_crossing

	(
	int		nseg,
	SEGMENT	*seglist,
	LINE	line,
	int		sseg,
	int		sspan,
	POINT	cross,
	int		*xseg,
	int		*xspan,
	int		*lspan,
	LOGICAL	*right
	)

	{
	int		iseg, ispan, ispanx;
	float	*ppos, *npos;
	SEGMENT	seg, segx;
	LOGICAL	pside, nside;

	static	LOGICAL	done = FALSE;
	if (!done)
		{
		done = TRUE;
		(void) printf("*** Obsolete function temoprarily supported:\n");
		(void) printf("Replace:\n");
		(void) printf("    int      nseg;\n");
		(void) printf("    SEGMENT  *seglist;\n");
		(void) printf("    LINE     line;\n");
		(void) printf("    int      sseg;\n");
		(void) printf("    int      sspan;\n");
		(void) printf("    POINT    cross;\n");
		(void) printf("    int      xseg;\n");
		(void) printf("    int      xspan;\n");
		(void) printf("    int      lspan;\n");
		(void) printf("    LOGICAL  right;\n");
		(void) printf("    next_seglist_crossing(nseg, seglist, line, sseg, sspan,\n");
		(void) printf("                           cross, &xseg, &xspan, &lspan, &right);\n");
		(void) printf("With:\n");
		(void) printf("    int      nseg;\n");
		(void) printf("    SEGMENT  *seglist;\n");
		(void) printf("    LINE     line;\n");
		(void) printf("    int      sseg;\n");
		(void) printf("    int      sspan;\n");
		(void) printf("    POINT    spos;\n");
		(void) printf("    POINT    cross;\n");
		(void) printf("    int      xseg;\n");
		(void) printf("    int      xspan;\n");
		(void) printf("    int      lspan;\n");
		(void) printf("    LOGICAL  right;\n");
		(void) printf("    seglist_crossing(nseg, seglist, FALSE, line, sseg, sspan, spos,\n");
		(void) printf("                      cross, &xseg, &xspan, &lspan, &right);\n");
		(void) printf("*** End\n");
		}

	/* Set up reasonable return values */
	if (cross) copy_point(cross, ZeroPoint);
	if (xseg)  *xseg  = -1;
	if (xspan) *xspan = -1;
	if (lspan) *lspan = -1;
	if (right) *right = FALSE;

	/* Make sure things are consistent */
	if (nseg < 1)          return FALSE;
	if (!seglist)          return FALSE;
	if (!line)             return FALSE;
	if (line->numpts <= 0) return FALSE;
	if (sseg >= nseg)      return FALSE;

	/* Check for crossovers on each segment */
	for (iseg=sseg; iseg<nseg; iseg++)
		{
		seg = seglist[iseg];
		if (iseg == sseg)          ispan = sspan;
		else if (seg_forward(seg)) ispan = seg->ips;
		else                       ispan = seg->ipe;

		/* Test which side of the line the first segment starts on */
		if (iseg == sseg)
			{
			line_test_point(line, seg->line->points[ispan], NullFloat,
					NullPoint, NullInt, NullChar, &pside);
			}

		/* Test which side of the line subsequent segment starts on */
		else
			{
			line_test_point(line, seg->line->points[ispan], NullFloat,
					NullPoint, NullInt, NullChar, &nside);

			/* A crossover must occur between end of previous segment */
			/*  and start of this segment!                            */
			if ((nside && !pside) || (!nside && pside))
				{

#				ifdef DEBUG_SEGLIST
				pr_diag("next_seglist_crossing",
					"Crossing between two spans!\n");
#				endif /* DEBUG_SEGLIST */

				segx = seglist[iseg-1];
				if (seg_forward(segx)) ispanx = segx->ipe;
				else                   ispanx = segx->ips;
				ppos = segx->line->points[ispanx];

				npos = seg->line->points[ispan];

				if (seg_forward(seg)) ispan--;
				if (ispan < 0 && line_closed(seg->line))
					ispan = seg->line->numpts - 2;

				/* Find the intersection point */
				if (xseg)  *xseg  = iseg;
				if (xspan) *xspan = ispan;
				if (right) *right = nside;
				(void) line_sight(line, ppos, npos, FALSE, NullFloat, NullFloat,
					cross, lspan, NullChar);
				return TRUE;
				}
			}

		/* Check for crossing within this segment */
		if (next_segment_crossing(seg, line, ispan, cross, xspan, lspan, right))
			{
			if (xseg) *xseg = iseg;
			return TRUE;
			}
		}

	/* No crossover found */
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*     n e x t _ c l o s e d _ s e g l i s t _ c r o s s i n g          *
*                                                                      *
*      Obsolescent function!                                           *
*                                                                      *
*      Use equivalent seglist_crossing() function.                     *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** OBSOLETE!
 *
 * Find the next point at which the given line crosses the given
 * closed segment list.  It is assumed that the lines cross any number
 * of times, or not at all.
 *
 *	@param[in] 	nseg		length of segment list
 *	@param[in] 	*seglist	closed segment list
 *	@param[in] 	line		line to cross
 *	@param[in] 	sseg		which segment to start at
 *	@param[in] 	sspan		span of sseg to start at
 *	@param[out]	cross		point of intersection
 *	@param[out]	*xseg		segment where crossover occurs
 *	@param[out]	*xspan		span on seg where crossover occurs
 *	@param[out]	*lspan		span on line where crossover occurs
 *	@param[out]	*right		now on right side?
 * 	@return True if successful.
 *********************************************************************/

LOGICAL	next_closed_seglist_crossing

	(
	int		nseg,
	SEGMENT	*seglist,
	LINE	line,
	int		sseg,
	int		sspan,
	POINT	cross,
	int		*xseg,
	int		*xspan,
	int		*lspan,
	LOGICAL	*right
	)

	{
	int		iseg, ispan, ispanx;
	float	*ppos, *npos;
	SEGMENT	seg, segx;
	LOGICAL	pside, nside;

	static	LOGICAL	done = FALSE;
	if (!done)
		{
		done = TRUE;
		(void) printf("*** Obsolete function temoprarily supported:\n");
		(void) printf("Replace:\n");
		(void) printf("    int      nseg;\n");
		(void) printf("    SEGMENT  *seglist;\n");
		(void) printf("    LINE     line;\n");
		(void) printf("    int      sseg;\n");
		(void) printf("    int      sspan;\n");
		(void) printf("    POINT    cross;\n");
		(void) printf("    int      xseg;\n");
		(void) printf("    int      xspan;\n");
		(void) printf("    int      lspan;\n");
		(void) printf("    LOGICAL  right;\n");
		(void) printf("    next_closed_seglist_crossing(nseg, seglist, line, sseg, sspan,\n");
		(void) printf("                                  cross, &xseg, &xspan, &lspan, &right);\n");
		(void) printf("With:\n");
		(void) printf("    int      nseg;\n");
		(void) printf("    SEGMENT  *seglist;\n");
		(void) printf("    LINE     line;\n");
		(void) printf("    int      sseg;\n");
		(void) printf("    int      sspan;\n");
		(void) printf("    POINT    spos;\n");
		(void) printf("    POINT    cross;\n");
		(void) printf("    int      xseg;\n");
		(void) printf("    int      xspan;\n");
		(void) printf("    int      lspan;\n");
		(void) printf("    LOGICAL  right;\n");
		(void) printf("    seglist_crossing(nseg, seglist, TRUE, line, sseg, sspan, spos,\n");
		(void) printf("                      cross, &xseg, &xspan, &lspan, &right);\n");
		(void) printf("*** End\n");
		}

	/* Set up reasonable return values */
	if (cross) copy_point(cross, ZeroPoint);
	if (xseg)  *xseg  = -1;
	if (xspan) *xspan = -1;
	if (lspan) *lspan = -1;
	if (right) *right = FALSE;

	/* Make sure things are consistent */
	if (nseg < 1)          return FALSE;
	if (!seglist)          return FALSE;
	if (!line)             return FALSE;
	if (line->numpts <= 0) return FALSE;
	if (sseg >= nseg)      return FALSE;

	/* Check for crossovers on each segment */
	for (iseg=sseg; iseg<nseg; iseg++)
		{
		seg = seglist[iseg];
		if (iseg == sseg)          ispan = sspan;
		else if (seg_forward(seg)) ispan = seg->ips;
		else                       ispan = seg->ipe;

		/* Test which side of the line the first segment starts on */
		if (iseg == sseg)
			{
			line_test_point(line, seg->line->points[ispan], NullFloat,
					NullPoint, NullInt, NullChar, &pside);
			}

		/* Test which side of the line subsequent segment starts on */
		else
			{
			line_test_point(line, seg->line->points[ispan], NullFloat,
					NullPoint, NullInt, NullChar, &nside);

			/* A crossover must occur between end of previous segment */
			/*  and start of this segment!                            */
			if ((nside && !pside) || (!nside && pside))
				{

#				ifdef DEBUG_SEGLIST
				pr_diag("next_closed_seglist_crossing",
					"Crossing between two spans!\n");
#				endif /* DEBUG_SEGLIST */

				segx = seglist[iseg-1];
				if (seg_forward(segx)) ispanx = segx->ipe;
				else                   ispanx = segx->ips;
				ppos = segx->line->points[ispanx];

				npos = seg->line->points[ispan];

				if (seg_forward(seg)) ispan--;
				if (ispan < 0 && line_closed(seg->line))
					ispan = seg->line->numpts - 2;

				/* Find the intersection point */
				if (xseg)  *xseg  = iseg;
				if (xspan) *xspan = ispan;
				if (right) *right = nside;
				(void) line_sight(line, ppos, npos, FALSE, NullFloat, NullFloat,
					cross, lspan, NullChar);
				return TRUE;
				}
			}

		/* Check for crossing within this segment */
		if (next_segment_crossing(seg, line, ispan, cross, xspan, lspan, right))
			{
			if (xseg) *xseg = iseg;
			return TRUE;
			}
		}

	/* Check for crossing after last segment on a closed segment list   */
	/*  ... between last point on last segment and first point on first */
	seg = seglist[0];
	if (seg_forward(seg)) ispan = seg->ips;
	else                  ispan = seg->ipe;
	ppos = seg->line->points[ispan];
	line_test_point(line, ppos, NullFloat, NullPoint, NullInt,
					NullChar, &pside);

	seg = seglist[nseg-1];
	if (seg_forward(seg)) ispan = seg->ipe;
	else                  ispan = seg->ips;
	npos = seg->line->points[ispan];
	line_test_point(line, npos, NullFloat, NullPoint, NullInt,
					NullChar, &nside);

	if (seg_forward(seg)) ispanx = seg->ipe;
	else                  ispanx = seg->ips - 1;
	if (ispanx < 0 && line_closed(seg->line)) ispanx = seg->line->numpts - 2;

	/* If the points coincide, there cannot be any more crossings */
	if (npos[X] == ppos[X] && npos[Y] == ppos[Y]) return FALSE;

	/* Check for a crossing after the last span of the last segment */
	if ((nside && !pside) || (!nside && pside))
		{

#		ifdef DEBUG_SEGLIST
		pr_diag("next_closed_seglist_crossing",
			"Crossing after end of last span!\n");
#		endif /* DEBUG_SEGLIST */

		/* Must have crossed over - find the intersection point */
		if (xseg)  *xseg  = nseg - 1;
		if (xspan) *xspan = ispanx;
		if (right) *right = pside;
		(void) line_sight(line, npos, ppos, FALSE, NullFloat, NullFloat,
			cross, lspan, NullChar);
		return TRUE;
		}

	/* No crossover found */
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*     n e x t _ s e g m e n t _ c r o s s i n g                        *
*                                                                      *
*      Obsolescent function!                                           *
*                                                                      *
*      Use equivalent segment_crossing() function.                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** OBSOLETE!
 *
 * Find the next point at which the given line crosses the given
 * segment.  It is assumed that the lines cross any number of
 * times, or not at all.
 *
 *	@param[in] 	seg		segment being crossed
 *	@param[in] 	line	line to cross
 *	@param[in] 	sspan	span of seg to start at
 *	@param[out]	cross	point of intersection
 *	@param[out]	*xspan	span on seg where crossover occurs
 *	@param[out]	*lspan	span on line where crossover occurs
 *	@param[out]	*right	now on right side?
 *  @return True if successful.
 *********************************************************************/

LOGICAL	next_segment_crossing

	(
	SEGMENT	seg,
	LINE	line,
	int		sspan,
	POINT	cross,
	int		*xspan,
	int		*lspan,
	LOGICAL	*right
	)

	{
	int		ip, dp;
	LINE	lseg;
	LOGICAL	pside, nside;
	float	*ppos, *npos;

	static	LOGICAL	done = FALSE;
	if (!done)
		{
		done = TRUE;
		(void) printf("*** Obsolete function temoprarily supported:\n");
		(void) printf("Replace:\n");
		(void) printf("    SEGMENT  seg;\n");
		(void) printf("    LINE     line;\n");
		(void) printf("    int      sspan;\n");
		(void) printf("    POINT    cross;\n");
		(void) printf("    int      xspan;\n");
		(void) printf("    int      lspan;\n");
		(void) printf("    LOGICAL  right;\n");
		(void) printf("    next_segment_crossing(seg, line, sspan,\n");
		(void) printf("                           cross, &xspan, &lspan, &right);\n");
		(void) printf("With:\n");
		(void) printf("    SEGMENT  seg;\n");
		(void) printf("    LINE     line;\n");
		(void) printf("    int      sspan;\n");
		(void) printf("    POINT    spos;\n");
		(void) printf("    POINT    cross;\n");
		(void) printf("    int      xspan;\n");
		(void) printf("    int      lspan;\n");
		(void) printf("    LOGICAL  right;\n");
		(void) printf("    segment_crossing(seg, line, sspan, spos,\n");
		(void) printf("                      cross, &xspan, &lspan, &right);\n");
		(void) printf("*** End\n");
		}

	/* Set up reasonable return values */
	if (cross) copy_point(cross, ZeroPoint);
	if (xspan) *xspan = -1;
	if (lspan) *lspan = -1;
	if (right) *right = FALSE;

	/* Make sure things are consistent */
	if (!seg)                  return FALSE;
	if (!(lseg = seg->line))   return FALSE;
	if (lseg->numpts <= 0)     return FALSE;
	if (!line)                 return FALSE;
	if (line->numpts <= 0)     return FALSE;
	if (sspan >= lseg->numpts) return FALSE;
	if (sspan < 0)             return FALSE;

	/* See what side of line the segment begins on */
	ip   = sspan;
	ppos = lseg->points[ip];
	line_test_point(line, ppos, NullFloat, NullPoint, NullInt, NullChar,
			&pside);

	/* Scan rest of the segment to find crossings */
	dp = (seg_forward(seg))? 1: -1;
	for (ip=sspan+dp; (ip>=seg->ips && ip<=seg->ipe); ip+=dp)
		{
		/* See what side of line the next point is on */
		npos = lseg->points[ip];
		line_test_point(line, npos, NullFloat, NullPoint, NullInt, NullChar,
				&nside);
		if ((nside && pside) || (!nside && !pside))
			{
			/* Keep looking */
			ppos = npos;
			continue;
			}

		/* Must have crossed over - find the intersection point */
		if (xspan) *xspan = ip - dp;
		if (right) *right = nside;
		(void) line_sight(line, ppos, npos, FALSE, NullFloat, NullFloat,
				cross, lspan, NullChar);
		return TRUE;
		}

	/* No intersections found */
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*     s e g l i s t _ u n i o n                                        *
*     s e g l i s t _ i n t e r s e c t                                *
*     s e g l i s t _ e x c l u d e                                    *
*                                                                      *
*     Logical operations (Venn Diagram) on pairs of closed segment     *
*     lists.                                                           *
*                                                                      *
*     These functions have never been implemented!                     *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Segment list exclude. Logical operations (Venn Diagram) on pairs
 * or closed segment lists
 *
 *	@param[in] 	num1,	Size of list1
 *	@param[in] 	*list1, First segment list
 *	@param[in] 	num2,	Size of list 2
 *	@param[in] 	*list2,	Second segment list
 *	@param[out]	*num,	Size of exclusion list
 *	@param[out]	**list	List of segements from list1 and list2 not
 *						common to both lists
 * 	@return True if successful.
 *********************************************************************/
#ifdef LATER
LOGICAL	seglist_exclude
	(
	int		num1,
	SEGMENT	*list1,
	int		num2,
	SEGMENT	*list2,
	int		*num,
	SEGMENT	**list
	)

	{
	LINE	hole;
	int		ih, nseg, pseg, pspan, xseg, xspan, hentr, hspan;
	SEGMENT	seg, *segs;
	LOGICAL	segcw, holecw, in, rt, segmod, holemod;
	POINT	px;

	int		nvseg = 0;
	SEGMENT	*vseg = NullSegmentList;

	if (!sub) return FALSE;
	if (sub->numseg < 1) return FALSE;

	if (!bound) return FALSE;
	if (bound->numhole < 1) return TRUE;

	/* >>>>> Change to SUBAREA structure as of Aug 2007 <<<<< */
	if (!sub->visready)
		{
		sub->numvis = sub->numseg;
		sub->segvis = GETMEM(sub->segvis, SEGMENT, sub->numvis);
		for (iseg=0; iseg<sub->numseg; iseg++)
			{
			sub->segvis[iseg] = copy_segment(sub->segments[iseg], FALSE);
			}
		sub->visready = TRUE;
		}
	if (sub->numvis < 1) return TRUE;
	nseg = sub->numvis;
	segs = sub->segvis;
	subarea_properties(sub, &segcw, NullFloat, NullFloat);

	segmod = FALSE;
	for (ih=0; ih<bound->numhole; ih++)
		{
		hole  = bound->holes[ih];
		line_properties(hole, NullChar, &holecw, NullFloat, NullFloat);

		pseg  = 0;
		seg   = segs[pseg];
		pspan = seg_forward(seg)? seg->ips: seg->ipe;

		hentr = -1;
		hexit = -1;
		holemod = FALSE;
		while (next_seglist_crossing(nseg, segs, bound, pseg, pspan, px,
					&xseg, &xspan, &hspan, &rt))
			{
			/* Have we stepped in or out of the hole? */
			in = (holecw)? rt: Not(rt);

			/* Stepped in now - add all segments from previous position */
			/* (start or previous exit) up until this crossing */
			if (in)
				{
				for (iseg=pseg; iseg<=xseg; iseg++)
					{
					nvseg++;
					vseg = GETMEM(vseg, SEGMENT, nvseg);
					vseg[nvseg-1] = copy_segment(segs[iseg], FALSE);
					seg  = vseg[nvseg-1];
					if (iseg == pseg)
						{
						if (seg_forward(seg)) seg->ips = pspan;
						else                  seg->ipe = pspan;
						}
					if (iseg == xseg)
						{
						if (seg_forward(seg)) seg->ipe = xspan;
						else                  seg->ips = xspan;
						}
					}

				/* Add intersection >>> */

				hentr = hspan;
				}

			/* Stepped out now - add segments from the appropriate side */
			/* of the hole */
			else
				{
				/* If we started inside the hole then we have to add this */
				/* part of the hole at the end */
				if (!holemod)
					{
					hexit   = hspan;
					holemod = TRUE;
					segmod  = TRUE;
					continue;
					}

				/* Add intersection >>> */

				nvseg++;
				vseg = GETMEM(vseg, SEGMENT, nvseg);
				vseg[nvseg-1] = create_segment();
				seg  = vseg[nvseg-1];

				if ( (holecw && Not(segcw)) || (Not(holecw) && segcw) )
					define_segment(seg, hole, FALSE, hentr+1, hspan, TRUE);
				else
					define_segment(seg, hole, FALSE, hentr, hspan+1, FALSE);

				pseg  = xseg;
				seg   = segs[pseg];
				pspan = xspan;
				hentr = -1;
				}

			holemod = TRUE;
			segmod  = TRUE;
			}

		/* If we finished inside the hole - add the first part of the hole */
		if (hexit >= 0)
			{
			nvseg++;
			vseg = GETMEM(vseg, SEGMENT, nvseg);
			vseg[nvseg-1] = create_segment();
			seg  = vseg[nvseg-1];

			if ( (holecw && Not(segcw)) || (Not(holecw) && segcw) )
				define_segment(seg, hole, FALSE, hentr+1, hexit, TRUE);
			else
				define_segment(seg, hole, FALSE, hentr, hexit+1, FALSE);
			}

		/* No (further) crossings - add remainder of segment list */
		else
			{
			xseg = nseg-1;
			xspan = ???;
			for (iseg=pseg; iseg<=xseg; iseg++)
				{
				nvseg++;
				vseg = GETMEM(vseg, SEGMENT, nvseg);
				vseg[nvseg-1] = copy_segment(segs[iseg], FALSE);
				seg  = vseg[nvseg-1];
				if (iseg == pseg)
					{
					if (seg_forward(seg)) seg->ips = pspan;
					else                  seg->ipe = pspan;
					}
				if (iseg == xseg)
					{
					if (seg_forward(seg)) seg->ipe = xspan;
					else                  seg->ips = xspan;
					}
				}
			}

		/* Replace visible boundary if altered */
		if (holemod)
			{
			for (iseg=0; iseg<sub->numvis; iseg++)
				{
				destroy_segment(sub->segvis[iseg]);
				}
			FREEMEM(sub->segvis);
			sub->numvis = nvseg;
			sub->segvis = vseg;

			nseg = sub->numvis;
			segs = sub->segvis;
			nvseg = 0;
			vseg  = NullSegmentList;
			}
		}

	return TRUE;
	}
#endif
