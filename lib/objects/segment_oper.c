/*********************************************************************/
/** @file segment_oper.c
 *
 * Routines to handle the SEGMENT object.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*      s e g m e n t _ o p e r . c                                     *
*                                                                      *
*      Routines to handle the SEGMENT object.                          *
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

#include "segment.h"

#include <tools/tools.h>
#include <fpa_math.h>
#include <string.h>

#undef DEBUG_SEGLIST

/***********************************************************************
*                                                                      *
*     s e g m e n t _ l e n g t h                                      *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find the length of the given segment.
 *
 *	@param[in] 	seg	given segment
 * 	@return Length of the segment.
 *********************************************************************/

float	segment_length

	(
	SEGMENT	seg
	)

	{
	float	len=0;
	float	x, y, xp, yp, dx, dy;
	int		ip, jp, dp, np, ips, ipe;

	if (IsNull(seg))       return len;
	if (IsNull(seg->line)) return len;

	if (seg_forward(seg))
		{
		np  = seg->line->numpts;
		dp  = 1;
		ips = seg->ips;
		ipe = seg->ipe;
		if (ips == ipe) return len;
		if (ipe < ips) ipe += np;
		}
	else
		{
		np  = seg->line->numpts;
		dp  = -1;
		ips = seg->ipe;
		ipe = seg->ips;
		if (ips == ipe) return len;
		if (ips < ipe) ips += np;
		}

	for (ip=ips; ; ip+=dp)
		{
		jp = ip%np;

		x  = seg->line->points[jp][X];
		y  = seg->line->points[jp][Y];

		if (ip == ips)
			{
			xp = x;
			yp = y;
			continue;
			}

		dx = x - xp;
		dy = y - yp;
		len += hypot(dx, dy);

		if (ip == ipe) return len;
		xp = x;
		yp = y;
		}
	}

/***********************************************************************
*                                                                      *
*     b e f o r e _ s e g m e n t _ s t a r t                          *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Test if span is before start span of the given segment.
 *
 *	@param[in] 	seg		given segment
 *	@param[in] 	ispan	span on given segment
 *  @return True if successful.
 *********************************************************************/

LOGICAL	before_segment_start

	(
	SEGMENT	seg,
	int		ispan
	)

	{
	int		np, ips, ipe, dps, dpe;

	/* Make sure things are consistent */
	if (IsNull(seg))       return FALSE;
	if (IsNull(seg->line)) return FALSE;

	/* Test simple lines */
	if (!line_closed(seg->line))
		{
		if (seg_forward(seg))
			{
			if (ispan < seg->ips) return TRUE;
			else                  return FALSE;
			}
		else
			{
			if (ispan > seg->ipe) return TRUE;
			else                  return FALSE;
			}
		}

	/* Test closed lines */
	else
		{

		/* Set segment parameters */
		np  = seg->line->numpts;
		ips = seg->ips;
		ipe = seg->ipe;

		/* Adjust parameters if segment crosses over start of line */
		if (ipe < ips)
			{
			if (ispan <= ipe) ispan += np;
			ipe += np;
			}

		/* Test span location to closest span on segment */
		if (seg_forward(seg))
			{
			if (ispan >= ips && ispan <= ipe) return FALSE;
			else if (ispan < ips)
				{
				dps = ips - ispan;
				dpe = ispan + np - ipe;
				}
			else if (ispan > ipe)
				{
				dps = ips + np - ispan;
				dpe = ispan - ipe;
				}
			if (dps <= dpe) return TRUE;
			else            return FALSE;
			}
		else
			{
			if (ispan >= ips && ispan <= ipe) return FALSE;
			else if (ispan < ips)
				{
				dps = ips - ispan;
				dpe = ispan + np - ipe;
				}
			else if (ispan > ipe)
				{
				dps = ips + np - ispan;
				dpe = ispan - ipe;
				}
			if (dpe <= dps) return TRUE;
			else            return FALSE;
			}
		}
	}

/***********************************************************************
*                                                                      *
*     a f t e r _ s e g m e n t _ e n d                                *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Test if span is after end span of the given segment.
 *
 *	@param[in] 	seg		given segment
 *	@param[in] 	ispan	span on given segment
 *  @return True if successful.
 *********************************************************************/

LOGICAL	after_segment_end

	(
	SEGMENT	seg,
	int		ispan
	)

	{
	int		np, ips, ipe, dps, dpe;

	/* Make sure things are consistent */
	if (IsNull(seg))       return FALSE;
	if (IsNull(seg->line)) return FALSE;

	/* Test simple lines */
	if (!line_closed(seg->line))
		{
		if (seg_forward(seg))
			{
			if (ispan > seg->ipe) return TRUE;
			else                  return FALSE;
			}
		else
			{
			if (ispan < seg->ips) return TRUE;
			else                  return FALSE;
			}
		}

	/* Test closed lines */
	else
		{

		/* Set segment parameters */
		np  = seg->line->numpts;
		ips = seg->ips;
		ipe = seg->ipe;

		/* Adjust parameters if segment crosses over start of line */
		if (ipe < ips)
			{
			if (ispan <= ipe) ispan += np;
			ipe += np;
			}

		/* Test span location to closest span on segment */
		if (seg_forward(seg))
			{
			if (ispan >= ips && ispan <= ipe) return FALSE;
			else if (ispan < ips)
				{
				dps = ips - ispan;
				dpe = ispan + np - ipe;
				}
			else if (ispan > ipe)
				{
				dps = ips + np - ispan;
				dpe = ispan - ipe;
				}
			if (dpe < dps) return TRUE;
			else           return FALSE;
			}
		else
			{
			if (ispan >= ips && ispan <= ipe) return FALSE;
			else if (ispan < ips)
				{
				dps = ips - ispan;
				dpe = ispan + np - ipe;
				}
			else if (ispan > ipe)
				{
				dps = ips + np - ispan;
				dpe = ispan - ipe;
				}
			if (dps < dpe) return TRUE;
			else           return FALSE;
			}
		}
	}

/***********************************************************************
*                                                                      *
*     s e g m e n t _ c r o s s i n g                                  *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find the next point at which the given line crosses the given
 * segment.  It is assumed that the lines cross any number of
 * times, or not at all.
 *
 *	@param[in] 	seg		segment being crossed
 *	@param[in] 	line	line to cross
 *	@param[in] 	sspan	span of seg to start at
 *	@param[in]	spos	point on span to start at
 *	@param[out]	cross	point of intersection
 *	@param[out]	*xspan	span on seg where crossover occurs
 *	@param[out]	*lspan	span on line where crossover occurs
 *	@param[out]	*right	now on right side?
 *  @return True if successful.
 *********************************************************************/

LOGICAL	segment_crossing

	(
	SEGMENT	seg,
	LINE	line,
	int		sspan,
	POINT	spos,
	POINT	cross,
	int		*xspan,
	int		*lspan,
	LOGICAL	*right
	)

	{
	int		ips, ipe, ip, jp, np, dp, xlspan, ipt;
	float	*ppos, *npos, xdist, sdist2, edist2, dx, dy;
	double	dang;
	LINE	lseg, tline;
	POINT	xcross, xpos;
	LOGICAL	intrsct, between, nside;

	/* Set up reasonable return values */
	if (cross) copy_point(cross, ZeroPoint);
	if (xspan) *xspan = -1;
	if (lspan) *lspan = -1;
	if (right) *right = FALSE;

	/* Make sure things are consistent */
	if (!seg)                return FALSE;
	if (!(lseg = seg->line)) return FALSE;
	if (lseg->numpts < 2)    return FALSE;
	if (!line)               return FALSE;
	if (line->numpts < 2)    return FALSE;

	/* Set segment parameters */
	np = lseg->numpts;
	if (seg_forward(seg))
		{
		dp  = 1;
		ips = seg->ips;
		ipe = seg->ipe;
		if (line_closed(lseg) && ipe < ips)
			{
			if (sspan <= ipe) sspan += np;
			ipe += np;
			}
		if (sspan < ips) return FALSE;
		if (sspan > ipe) return FALSE;
		}
	else
		{
		dp  = -1;
		ips = seg->ipe;
		ipe = seg->ips;
		if (line_closed(lseg) && ips < ipe)
			{
			if (sspan <= ips) sspan += np;
			ips += np;
			}
		if (sspan > ips) return FALSE;
		if (sspan < ipe) return FALSE;
		}

	/* Set start point on segment */
	ppos = spos;

	/* Scan rest of the segment to find crossings */
	for (ip=sspan+dp; ; ip+=dp)
		{
		if ( seg_forward(seg) && ip>ipe) break;
		if (!seg_forward(seg) && ip<ipe) break;

		/* Set next point of segment */
		jp   = ip%np;
		npos = lseg->points[jp];

		/* If the points coincide, try the next point */
		if (npos[X] == ppos[X] && npos[Y] == ppos[Y]) continue;

		/* Look for crossing between these points */
		intrsct = line_sight(line, ppos, npos, FALSE, &xdist, NullFloat,
								xcross, &xlspan, &between);

		/* The line_sight() function allows a certain tolerance in   */
		/*  determining intersections to account for round off error */
		/*  ... so check for potential problem cases below           */

		/* An intersection that occurs just after the end of a span may be */
		/*  mistakenly identified as occurring in the current span ... so  */
		/*  ensure xdist is not greater than the length of current span    */
		/* (Note that since we always check in a forward direction, we can */
		/*  never find an intersection before the start of a span!)        */
		if (intrsct && between && xdist > point_dist(ppos, npos))
			{
			between = FALSE;

#			ifdef DEBUG_SEGLIST
			pr_diag("segment_crossing",
				"Beyond end of span crossing ... xdist/dist: %.4f/%.4f\n",
				xdist, point_dist(ppos, npos));
#			endif /* DEBUG_SEGLIST */
			}

		/* Found an intersection point */
		if (intrsct && between)
			{

			/* If intersection is very close to end point, move back */
			/*  towards start point to find which side we started on */
			/*  (reverse will be which side we ended on!)            */
			edist2 = point_dist2(npos, xcross);
			if (edist2 < EndTol*EndTol)
				{

				/* Determine side we ended on only if necessary */
				if (right)
					{
					sdist2 = point_dist2(ppos, xcross);
					if (sdist2 < SegTol*SegTol)
						{
						copy_point(xpos, ppos);
						}
					else
						{
						dx   = xcross[X] - ppos[X];
						dy   = xcross[Y] - ppos[Y];
						dang = atan2((double) dy, (double) dx);
						xpos[X] = xcross[X] - SegTol * (float) cos(dang);
						xpos[Y] = xcross[Y] - SegTol * (float) sin(dang);
						}

					/* Check intersection span to determine side we started */
					/*  on ... and then reverse                             */
					tline = create_line();
					if (xlspan < 0)                      ipt = 0;
					else if (xlspan >= line->numpts - 1) ipt = line->numpts - 2;
					else                                 ipt = xlspan;
					add_point_to_line(tline, line->points[ipt]);
					add_point_to_line(tline, line->points[ipt+1]);
					line_test_point(tline, xpos, NullFloat, NullPoint,
										NullInt, NullChar, &nside);
					nside = !nside;
					tline = destroy_line(tline);
					}

#				ifdef DEBUG_SEGLIST
				pr_diag("segment_crossing",
					"Close to end of span crossing! edist: %.4f\n",
					sqrt(edist2));
#				endif /* DEBUG_SEGLIST */

				/* Set the intersection point */
				if (cross) copy_point(cross, xcross);
				if (xspan) *xspan = jp;
				if (lspan) *lspan = xlspan;
				if (right) *right = nside;
				return TRUE;
				}

			/* Otherwise, move towards end point to determine */
			/*  which side we ended on                        */
			else
				{

				/* Determine side we ended on only if necessary */
				if (right)
					{
					sdist2 = point_dist2(xcross, npos);
					if (sdist2 < SegTol*SegTol)
						{
						copy_point(xpos, npos);
						}
					else
						{
						dx   = npos[X] - xcross[X];
						dy   = npos[Y] - xcross[Y];
						dang = atan2((double) dy, (double) dx);
						xpos[X] = xcross[X] + SegTol * (float) cos(dang);
						xpos[Y] = xcross[Y] + SegTol * (float) sin(dang);
						}

					/* Check intersection span to determine side we ended on */
					tline = create_line();
					if (xlspan < 0)                      ipt = 0;
					else if (xlspan >= line->numpts - 1) ipt = line->numpts - 2;
					else                                 ipt = xlspan;
					add_point_to_line(tline, line->points[ipt]);
					add_point_to_line(tline, line->points[ipt+1]);
					line_test_point(tline, xpos, NullFloat, NullPoint,
										NullInt, NullChar, &nside);
					tline = destroy_line(tline);
					}

#				ifdef DEBUG_SEGLIST
				pr_diag("segment_crossing", "Normal crossing!\n");
#				endif /* DEBUG_SEGLIST */

				/* Set the intersection point */
				if (cross) copy_point(cross, xcross);
				if (xspan) *xspan = jp - dp;
				if (lspan) *lspan = xlspan;
				if (right) *right = nside;
				return TRUE;
				}
			}

		/* Keep looking */
		ppos = npos;
		continue;
		}

	/* No intersections found */
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*     s e g l i s t _ c r o s s i n g                                  *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Find the next point at which the given line crosses the given
 * segment list.  It is assumed that the lines cross any number
 * of times, or not at all.
 *
 *	@param[in] 	nseg		length of segment list
 *	@param[in] 	*seglist	segment list
 *	@param[in] 	closed		is the segment list closed?
 *	@param[in] 	line		line to cross
 *	@param[in] 	sseg		which segment to start at
 *	@param[in] 	sspan		span of sseg to start at
 *	@param[in]	spos		point on span of sseg to start at
 *	@param[out]	cross		point of intersection
 *	@param[out]	*xseg		segment where crossover occurs
 *	@param[out]	*xspan		span on seg where crossover occurs
 *	@param[out]	*lspan		span on line where crossover occurs
 *	@param[out]	*right		now on right side?
 * 	@return True if successful.
 *********************************************************************/

LOGICAL	seglist_crossing

	(
	int		nseg,
	SEGMENT	*seglist,
	LOGICAL	closed,
	LINE	line,
	int		sseg,
	int		sspan,
	POINT	spos,
	POINT	cross,
	int		*xseg,
	int		*xspan,
	int		*lspan,
	LOGICAL	*right
	)

	{
	int		iseg, ispan, ispanx, xlspan, ipt;
	float	*ppos, *npos, xdist, sdist2, edist2, dx, dy;
	double	dang;
	SEGMENT	seg, segx;
	LINE	tline;
	POINT	xcross, xpos;
	LOGICAL	cfirst, clast;
	LOGICAL	intrsct, between, nside;

	/* Set up reasonable return values */
	if (cross) copy_point(cross, ZeroPoint);
	if (xseg)  *xseg  = -1;
	if (xspan) *xspan = -1;
	if (lspan) *lspan = -1;
	if (right) *right = FALSE;

	/* Make sure things are consistent */
	if (nseg < 1)         return FALSE;
	if (!seglist)         return FALSE;
	if (!line)            return FALSE;
	if (line->numpts < 2) return FALSE;
	if (sseg >= nseg)     return FALSE;
	if (sseg < 0)         return FALSE;

	/* Are we checking before start of this segment? */
	seg    = seglist[sseg];
	cfirst = before_segment_start(seg, sspan);

	/* Are we already checking beyond last segment? */
	if (sseg == nseg-1)
		{
		seg   = seglist[sseg];
		clast = ((seg_forward(seg) && sspan == seg->ipe)
					|| (!seg_forward(seg) && sspan == seg->ips)
					|| after_segment_end(seg, sspan));
		}
	else
		{
		clast = FALSE;
		}

	/* Set start position to previous intersection */
	ppos = spos;

	/* Should never be checking before start of initial segment! */
	if (cfirst && sseg == 0)
		{
		pr_error("seglist_crossing",
			"Checking for crossing before start of initial segment!\n");
		return FALSE;
		}

	/* Check for crossover before start of first segment */
	else if (cfirst)
		{
		seg   = seglist[sseg];
		ispan = sspan;

		/* Set start point from this segment */
		if (seg_forward(seg)) ispanx = seg->ips;
		else                  ispanx = seg->ipe;
		npos = seg->line->points[ispanx];

		/* Look for an intersection between previous intersection */
		/*  and start point from this segment                     */
		intrsct = line_sight(line, ppos, npos, FALSE, &xdist,
								NullFloat, xcross, &xlspan, &between);

		/* The line_sight() function allows a certain tolerance in   */
		/*  determining intersections to account for round off error */
		/*  ... so check for potential problem cases below           */

		/* An intersection that occurs just after the end of a span may be */
		/*  mistakenly identified as occurring in the current span ... so  */
		/*  ensure xdist is not greater than the length of current span    */
		/* (Note that since we always check in a forward direction, we can */
		/*  never find an intersection before the start of a span!)        */
		if (intrsct && between && xdist > point_dist(ppos, npos))
			{
			between = FALSE;

#			ifdef DEBUG_SEGLIST
			pr_diag("seglist_crossing",
				"  Before start line_sight problem ... xdist/dist: %.4f/%.4f\n",
				xdist, point_dist(ppos, npos));
#			endif /* DEBUG_SEGLIST */
			}

		/* Found an intersection point */
		if (intrsct && between)
			{

#			ifdef DEBUG_SEGLIST
			pr_diag("seglist_crossing",
				"Found a crossing between two spans (first)!\n");
#			endif /* DEBUG_SEGLIST */

			/* If intersection coincides with end point, move back   */
			/*  towards start point to find which side we started on */
			/*  (reverse will be which side we ended on!)            */
			if (xcross[X] == npos[X] && xcross[Y] == npos[Y])
				{

				/* Determine side we ended on only if necessary */
				if (right)
					{
					sdist2 = point_dist2(ppos, xcross);
					if (sdist2 < SegTol*SegTol)
						{
						copy_point(xpos, ppos);
						}
					else
						{
						dx   = npos[X] - ppos[X];
						dy   = npos[Y] - ppos[Y];
						dang = atan2((double) dy, (double) dx);
						xpos[X] = xcross[X] - SegTol * (float) cos(dang);
						xpos[Y] = xcross[Y] - SegTol * (float) sin(dang);
						}

					/* Check intersection span to determine side we started */
					/*  on ... and then reverse                             */
					tline = create_line();
					if (xlspan < 0)                      ipt = 0;
					else if (xlspan >= line->numpts - 1) ipt = line->numpts - 2;
					else                                 ipt = xlspan;
					add_point_to_line(tline, line->points[ipt]);
					add_point_to_line(tline, line->points[ipt+1]);
					line_test_point(tline, xpos, NullFloat, NullPoint,
										NullInt, NullChar, &nside);
					nside = !nside;
					tline = destroy_line(tline);
					}

				/* Set return values */
				if (cross) copy_point(cross, xcross);
				if (xseg)  *xseg  = sseg;
				if (xspan) *xspan = ispanx;
				if (lspan) *lspan = xlspan;
				if (right) *right = nside;
				return TRUE;
				}

			/* Otherwise, move towards end point to determine */
			/*  which side we ended on                        */
			else
				{

				/* Determine side we ended on only if necessary */
				if (right)
					{
					edist2 = point_dist2(xcross, npos);
					if (edist2 < SegTol*SegTol)
						{
						copy_point(xpos, npos);
						}
					else
						{
						dx   = npos[X] - ppos[X];
						dy   = npos[Y] - ppos[Y];
						dang = atan2((double) dy, (double) dx);
						xpos[X] = xcross[X] + SegTol * (float) cos(dang);
						xpos[Y] = xcross[Y] + SegTol * (float) sin(dang);
						}

					/* Check intersection span to determine side we ended on */
					tline = create_line();
					if (xlspan < 0)                      ipt = 0;
					else if (xlspan >= line->numpts - 1) ipt = line->numpts - 2;
					else                                 ipt = xlspan;
					add_point_to_line(tline, line->points[ipt]);
					add_point_to_line(tline, line->points[ipt+1]);
					line_test_point(tline, xpos, NullFloat, NullPoint,
										NullInt, NullChar, &nside);
					tline = destroy_line(tline);
					}

				/* Set return values */
				if (cross) copy_point(cross, xcross);
				if (xseg)  *xseg  = sseg;
				if (xspan) *xspan = sspan;
				if (lspan) *lspan = xlspan;
				if (right) *right = nside;
				return TRUE;
				}
			}

		/* Reset start point on segment if no intersection */
		ppos = npos;
		}

	/* Check for crossovers on each segment */
	if (!clast)
		{
		for (iseg=sseg; iseg<nseg; iseg++)
			{
			seg = seglist[iseg];
			if (iseg == sseg && !cfirst) ispan = sspan;
			else if (seg_forward(seg))   ispan = seg->ips;
			else                         ispan = seg->ipe;

			/* Check for crossover between end of previous segment */
			/*  and start of this segment                          */
			if (iseg > sseg)
				{

				/* Set previous point from previous segment */
				segx = seglist[iseg-1];
				if (seg_forward(segx)) ispanx = segx->ipe;
				else                   ispanx = segx->ips;
				ppos = segx->line->points[ispanx];

				/* Set start point from this segment */
				npos = seg->line->points[ispan];

				/* Look for an intersection between these points */
				intrsct = line_sight(line, ppos, npos, FALSE, &xdist,
										NullFloat, xcross, &xlspan, &between);

				/* The line_sight() function allows a certain tolerance in   */
				/*  determining intersections to account for round off error */
				/*  ... so check for potential problem cases below           */

				/* An intersection that occurs just after end of a span may   */
				/*  be mistakenly identified as occurring in current span ... */
				/*  so ensure xdist not greater than length of current span   */
				/* (Note that since we always check in a forward direction,   */
				/*  we can never find an intersection before start of a span! */
				if (intrsct && between && xdist > point_dist(ppos, npos))
					{
					between = FALSE;

#					ifdef DEBUG_SEGLIST
					pr_diag("seglist_crossing",
						"  Within segment line_sight problem ... xdist/dist: %.4f/%.4f\n",
						xdist, point_dist(ppos, npos));
#					endif /* DEBUG_SEGLIST */
					}

				/* Found an intersection point */
				if (intrsct && between)
					{

#					ifdef DEBUG_SEGLIST
					pr_diag("seglist_crossing",
						"Found a crossing between two spans!\n");
#					endif /* DEBUG_SEGLIST */

					/* If intersection coincides with end point, move back   */
					/*  towards start point to find which side we started on */
					/*  (reverse will be which side we ended on!)            */
					if (xcross[X] == npos[X] && xcross[Y] == npos[Y])
						{

						/* Determine side we ended on only if necessary */
						if (right)
							{
							sdist2 = point_dist2(ppos, xcross);
							if (sdist2 < SegTol*SegTol)
								{
								copy_point(xpos, ppos);
								}
							else
								{
								dx   = npos[X] - ppos[X];
								dy   = npos[Y] - ppos[Y];
								dang = atan2((double) dy, (double) dx);
								xpos[X] = xcross[X] - SegTol * (float) cos(dang);
								xpos[Y] = xcross[Y] - SegTol * (float) sin(dang);
								}

							/* Check intersection span to determine side we */
							/*  started on ... and then reverse             */
							tline = create_line();
							if (xlspan < 0)
								ipt = 0;
							else if (xlspan >= line->numpts - 1)
								ipt = line->numpts - 2;
							else
								ipt = xlspan;
							add_point_to_line(tline, line->points[ipt]);
							add_point_to_line(tline, line->points[ipt+1]);
							line_test_point(tline, xpos, NullFloat, NullPoint,
												NullInt, NullChar, &nside);
							nside = !nside;
							tline = destroy_line(tline);
							}

						/* Set return values */
						if (cross) copy_point(cross, xcross);
						if (xseg)  *xseg  = iseg;
						if (xspan) *xspan = ispan;
						if (lspan) *lspan = xlspan;
						if (right) *right = nside;
						return TRUE;
						}

					/* Otherwise, move towards end point to determine */
					/*  which side we ended on                        */
					else
						{

						/* Determine side we ended on only if necessary */
						if (right)
							{
							edist2 = point_dist2(xcross, npos);
							if (edist2 < SegTol*SegTol)
								{
								copy_point(xpos, npos);
								}
							else
								{
								dx   = npos[X] - ppos[X];
								dy   = npos[Y] - ppos[Y];
								dang = atan2((double) dy, (double) dx);
								xpos[X] = xcross[X] + SegTol * (float) cos(dang);
								xpos[Y] = xcross[Y] + SegTol * (float) sin(dang);
								}

							/* Check intersection span to determine side we */
							/*  ended on                                    */
							tline = create_line();
							if (xlspan < 0)
								ipt = 0;
							else if (xlspan >= line->numpts - 1)
								ipt = line->numpts - 2;
							else
								ipt = xlspan;
							add_point_to_line(tline, line->points[ipt]);
							add_point_to_line(tline, line->points[ipt+1]);
							line_test_point(tline, xpos, NullFloat, NullPoint,
												NullInt, NullChar, &nside);
							tline = destroy_line(tline);
							}

						/* Set previous span on segment */
						ispanx = (seg_forward(seg))? ispan-1: ispan+1;
						if (ispanx < 0 && line_closed(seg->line))
							ispanx = seg->line->numpts - 2;
						if (ispanx >= seg->line->numpts
								&& line_closed(seg->line))
							ispanx = 1;

						/* Set return values */
						if (cross) copy_point(cross, xcross);
						if (xseg)  *xseg  = iseg;
						if (xspan) *xspan = ispanx;
						if (lspan) *lspan = xlspan;
						if (right) *right = nside;
						return TRUE;
						}
					}

				/* Reset start point on segment if no intersection */
				ppos   = npos;
				}

			/* Check for crossing within this segment */
			if (segment_crossing(seg, line, ispan, ppos,
									cross, xspan, lspan, right))
				{
				if (xseg) *xseg = iseg;
				return TRUE;
				}
			}

		/* Reset start position to after last span */
		seg  = seglist[nseg-1];
		if (seg_forward(seg)) ispan = seg->ipe;
		else                  ispan = seg->ips;
		ppos = seg->line->points[ispan];
		}

	/* Set start position if checking after last span */
	else
		{
		ppos  = spos;
		ispan = sspan;
		}

	/* Check for crossing after last segment on a closed segment list   */
	/*  ... between last point on last segment and first point on first */
	if (closed)
		{
		seg  = seglist[0];
		if (seg_forward(seg)) ispanx = seg->ips;
		else                  ispanx = seg->ipe;
		npos = seg->line->points[ispanx];

		/* If the points coincide, there cannot be any more crossings */
		if (npos[X] == ppos[X] && npos[Y] == ppos[Y]) return FALSE;

		/* Look for an intersection between these points */
		intrsct = line_sight(line, ppos, npos, FALSE, &xdist, NullFloat,
								xcross, &xlspan, &between);

		/* The line_sight() function allows a certain tolerance in   */
		/*  determining intersections to account for round off error */
		/*  ... so check for potential problem cases below           */

		/* An intersection that occurs just after the end of a span may be */
		/*  mistakenly identified as occurring in the current span ... so  */
		/*  ensure xdist is not greater than the length of current span    */
		/* (Note that since we always check in a forward direction, we can */
		/*  never find an intersection before the start of a span!)        */
		if (intrsct && between && xdist > point_dist(ppos, npos))
			{
			between = FALSE;

#			ifdef DEBUG_SEGLIST
			pr_diag("seglist_crossing",
				"  After last line_sight problem ... xdist/dist: %.4f/%.4f\n",
				xdist, point_dist(ppos, npos));
#			endif /* DEBUG_SEGLIST */
			}

		/* Found an intersection point */
		if (intrsct && between)
			{

#			ifdef DEBUG_SEGLIST
			pr_diag("seglist_crossing",
				"Found a crossing after end of last span!\n");
#			endif /* DEBUG_SEGLIST */

			/* If intersection coincides with end point, move back   */
			/*  towards start point to find which side we started on */
			/*  (reverse will be which side we ended on!)            */
			if (xcross[X] == npos[X] && xcross[Y] == npos[Y])
				{

				/* Determine side we ended on only if necessary */
				if (right)
					{
					sdist2 = point_dist2(ppos, xcross);
					if (sdist2 < SegTol*SegTol)
						{
						copy_point(xpos, ppos);
						}
					else
						{
						dx   = npos[X] - ppos[X];
						dy   = npos[Y] - ppos[Y];
						dang = atan2((double) dy, (double) dx);
						xpos[X] = xcross[X] - SegTol * (float) cos(dang);
						xpos[Y] = xcross[Y] - SegTol * (float) sin(dang);
						}

					/* Check intersection span to determine side we started */
					/*  on ... and then reverse                             */
					tline = create_line();
					if (xlspan < 0)                      ipt = 0;
					else if (xlspan >= line->numpts - 1) ipt = line->numpts - 2;
					else                                 ipt = xlspan;
					add_point_to_line(tline, line->points[ipt]);
					add_point_to_line(tline, line->points[ipt+1]);
					line_test_point(tline, xpos, NullFloat, NullPoint,
										NullInt, NullChar, &nside);
					nside = !nside;
					tline = destroy_line(tline);
					}
				}

			/* Otherwise, move towards end point to determine */
			/*  which side we ended on                        */
			else
				{

				/* Determine side we ended on only if necessary */
				if (right)
					{
					edist2 = point_dist2(xcross, npos);
					if (edist2 < SegTol*SegTol)
						{
						copy_point(xpos, npos);
						}
					else
						{
						dx   = npos[X] - ppos[X];
						dy   = npos[Y] - ppos[Y];
						dang = atan2((double) dy, (double) dx);
						xpos[X] = xcross[X] + SegTol * (float) cos(dang);
						xpos[Y] = xcross[Y] + SegTol * (float) sin(dang);
						}

					/* Check intersection span to determine side we ended on */
					tline = create_line();
					if (xlspan < 0)                      ipt = 0;
					else if (xlspan >= line->numpts - 1) ipt = line->numpts - 2;
					else                                 ipt = xlspan;
					add_point_to_line(tline, line->points[ipt]);
					add_point_to_line(tline, line->points[ipt+1]);
					line_test_point(tline, xpos, NullFloat, NullPoint,
										NullInt, NullChar, &nside);
					tline = destroy_line(tline);
					}
				}

			/* Set return values */
			if (cross) copy_point(cross, xcross);
			if (xseg)  *xseg  = nseg - 1;
			if (xspan) *xspan = ispan;
			if (lspan) *lspan = xlspan;
			if (right) *right = nside;
			return TRUE;
			}
		}

	/* No crossover found */
	return FALSE;
	}
