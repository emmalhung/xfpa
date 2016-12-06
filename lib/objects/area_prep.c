/***********************************************************************/
/**		@file	area_prep.c
 *
 * 	Assorted operations on visible display of AREAs
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 ***********************************************************************/
/***********************************************************************
*                                                                      *
*    a r e a _ p r e p . c                                             *
*                                                                      *
*    Assorted operations on visible display of AREAs.                  *
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

#include "area.h"
#include "item.h"

#include <tools/tools.h>
#include <fpa_getmem.h>

#include <stdio.h>
#include <string.h>

#undef DEBUG_DIV_HOLES
#undef DEBUG_BND_HOLES

/***********************************************************************
*                                                                      *
*      p r e p _ a r e a                                               *
*                                                                      *
***********************************************************************/
/***********************************************************************/
/**	Make the given area ready for display.
 *
 * Mask each subarea with holes.
 *
 * @param[in]	area	area to make ready.
 ***********************************************************************/

void	prep_area
	(
	AREA	area
	)

	{
	int		isub;
	SUBAREA	sub;

	if (IsNull(area))   return;
	if (area->visready) return;

	/* Prepare the subareas (if area has dividing lines) */
	if (area->numdiv > 0)
		{
		build_area_subareas(area);
		for (isub=0; isub<area->numdiv+1; isub++)
			{
			sub = area->subareas[isub];
			prep_subarea(area, sub);
			}
		}

	/* Set flag for visible areas ready */
	area->visready = TRUE;
	}

/***********************************************************************
*                                                                      *
*      p r e p _ s u b a r e a                                         *
*                                                                      *
***********************************************************************/
/***********************************************************************/
/**	Make the given subarea of the given area ready for display.
 *
 * Mask it with holes from its parent area.
 *
 * @param[in]	area	area with subarea.
 * @param[in]	sub		subarea to make ready.
 ***********************************************************************/

void	prep_subarea
	(
	AREA	area,
	SUBAREA	sub
	)

	{
	int		isv, iseg, ih;
	SUBVIS	svis;

	if (IsNull(area) || IsNull(area->bound)) return;
	if (IsNull(sub))  return;

	/* Free the space used by the visible area buffer */
	if (sub->nsubvis > 0)
		{
		for (isv=0; isv<sub->nsubvis; isv++)
			(void) destroy_subvis(sub->subvis[isv]);
		FREEMEM(sub->subvis);
		sub->nsubvis = 0;
		}
	if (sub->numhole > 0)
		{
		FREEMEM(sub->holes);
		sub->numhole = 0;
		}

	/* Initialize the visible area buffer */
	sub->nsubvis = 1;
	sub->subvis  = INITMEM(SUBVIS, 1);

	svis = sub->subvis[0] = create_subvis();
	svis->numvis = sub->numseg;
	svis->segvis = INITMEM(SEGMENT, svis->numvis);
	for (iseg=0; iseg<sub->numseg; iseg++)
		{
		svis->segvis[iseg] = copy_segment(sub->segments[iseg], FALSE);
		}

	/* Add all holes to each subarea */
	sub->numhole = area->bound->numhole;
	if (sub->numhole > 0)
		{
		sub->holes = GETMEM(sub->holes, LINE, sub->numhole);
		for (ih=0; ih<sub->numhole; ih++)
			{
			sub->holes[ih] = area->bound->holes[ih];
			}
		}

	/* Set flag for visible areas ready */
	sub->visready = TRUE;
	}

/***********************************************************************
*                                                                      *
*      p r e p _ a r e a _ c o m p l e x                               *
*                                                                      *
***********************************************************************/
/***********************************************************************/
/**	Make the given area ready for display.
 *
 * Mask each subarea with holes.
 *
 * @param[in]	area	area to make ready.
 ***********************************************************************/

void	prep_area_complex
	(
	AREA	area
	)

	{
	int		isub;
	SUBAREA	sub;

	if (IsNull(area))   return;
	if (area->visready) return;

	/* Prepare the subareas (even with no dividing lines) */
	build_area_subareas(area);
	for (isub=0; isub<area->numdiv+1; isub++)
		{
		sub = area->subareas[isub];
		prep_subarea_complex(area, sub);
		}

	/* Set flag for visible areas ready */
	area->visready = TRUE;
	}

/***********************************************************************
*                                                                      *
*      p r e p _ s u b a r e a _ c o m p l e x                         *
*                                                                      *
***********************************************************************/
/***********************************************************************/
/**	Make the given subarea of the given area ready for display.
 *
 * Mask it with holes from its parent area.
 *
 * @param[in]	area	area with subarea.
 * @param[in]	sub		subarea to make ready.
 ***********************************************************************/

void	prep_subarea_complex
	(
	AREA	area,
	SUBAREA	sub
	)

	{
	int		isv, iseg;
	SUBVIS	svis;

	if (IsNull(area) || IsNull(area->bound)) return;
	if (IsNull(sub))  return;

	/* Free the space used by the visible area buffer */
	if (sub->nsubvis > 0)
		{
		for (isv=0; isv<sub->nsubvis; isv++)
			(void) destroy_subvis(sub->subvis[isv]);
		FREEMEM(sub->subvis);
		sub->nsubvis = 0;
		}
	if (sub->numhole > 0)
		{
		FREEMEM(sub->holes);
		sub->numhole = 0;
		}
	sub->visready = FALSE;

	/* Initialize visible subarea if area has no dividing lines or holes */
	if (area->numdiv <= 0 && area->bound->numhole <= 0)
		{
		sub->nsubvis = 1;
		sub->subvis  = INITMEM(SUBVIS, 1);

		svis = sub->subvis[0] = create_subvis();
		svis->numvis = sub->numseg;
		svis->segvis = INITMEM(SEGMENT, svis->numvis);
		for (iseg=0; iseg<sub->numseg; iseg++)
			{
			svis->segvis[iseg] = copy_segment(sub->segments[iseg], FALSE);
			}
		sub->visready = TRUE;
		return;
		}

	/* Determine intersections of boundary, dividing lines and holes */
	(void) divide_subarea_holes(sub, area->bound);
	}

/***********************************************************************
*                                                                      *
*      d i v i d e _ s u b a r e a _ h o l e s                         *
*                                                                      *
***********************************************************************/

/* Structure for hole exit and entry information for segment lists */
typedef	enum	{ SLinitial, SLpartial, SLdone, SLskip, SLextra } SLSTAT;
typedef	struct	sl_struct
	{
	int				hexit;
	float			dexit;
	int				hentr;
	float			dentr;
	SLSTAT			slstat;
	int				lexit;
	int				lentr;
	} SLLIST;

static	void	reset_start_span(int, SEGMENT *, int, int, POINT,
									int *, int *, int *, int *, POINT);
static	int		next_hole_entry(int, SLLIST *, LOGICAL, int, float);
static	int		next_hole_exit(int, SLLIST *, LOGICAL, int, float);

/***********************************************************************/
/**	Make the subarea outline track around any holes it crosses.
 *
 * @param[in]	sub	subarea outline from boundary and divides.
 * @param[in]	bound	area boundary with all area holes.
 * @return True if successful.
 ***********************************************************************/

LOGICAL	divide_subarea_holes

	(
	SUBAREA	sub,
	BOUND	bound
	)

	{
	LINE	hole, xhole, xline;
	int		ih, iho, iha, jh, isv, itv;
	int		*ihorder;
	int		iseg, nseg, pseg, pspan, fseg, fspan, useg, uspan, xseg, xspan;
	int		npts, ips, ipe, ip;
	int		hentr, hlast, hexit, hspan, lentr, lexit;
	float	xarea, dentr, dlast, dexit, dspan, dist;
	SUBVIS	svis;
	SEGMENT	seg, *segs;
	SUBAREA	psub;
	LOGICAL	*outsub;
	LOGICAL	segcw, holecw, insync;
	LOGICAL	firstspan, in, rt, holemod, hin, segexcl;
	POINT	pb, px, pentr, pexit, plast;

	int		tsv, xsv, nvseg;
	SUBVIS	*tvlist, *xvlist;
	SLLIST	*sllist;
	SEGMENT	*vseg;

#	ifdef DEBUG_DIV_HOLES
	STRING	sdirection, sforward = "->", sbackward = "<-";
#	endif /* DEBUG_DIV_HOLES */

	if (!sub) return FALSE;
	if (sub->numseg < 1) return FALSE;

	if (!bound) return FALSE;

	/* Initialize visible segments on area boundary */
	if (!sub->visready)
		{
		sub->nsubvis = 1;
		sub->subvis  = INITMEM(SUBVIS, 1);

		svis = sub->subvis[0] = create_subvis();
		svis->numvis = sub->numseg;
		svis->segvis = INITMEM(SEGMENT, svis->numvis);
		for (iseg=0; iseg<sub->numseg; iseg++)
			{
			svis->segvis[iseg] = copy_segment(sub->segments[iseg], FALSE);
			}
		sub->visready = TRUE;
		}
	if (bound->numhole < 1) return TRUE;
	if (sub->nsubvis < 1)   return TRUE;

	/* Initialize subarea properties */
	subarea_properties(sub, &segcw, NullFloat, NullFloat);

#	ifdef DEBUG_DIV_HOLES
	if (segcw)
		pr_diag("divide_subarea_holes", "Clockwise subarea: [%x]\n", sub);
	else
		pr_diag("divide_subarea_holes", "CounterClockwise subarea: [%x]\n", sub);
	pr_diag("divide_subarea_holes",
			" Visible segments lists: %d\n", sub->nsubvis);
	for (isv=0; isv<sub->nsubvis; isv++)
		{
		svis = sub->subvis[isv];
		nseg = svis->numvis;
		segs = svis->segvis;
		for (iseg=0; iseg<nseg; iseg++)
			{
			seg = segs[iseg];
			sdirection = (seg_forward(seg))? sforward: sbackward;
			pr_diag("divide_subarea_holes",
				"  Segment %d [%x] - %d %s %d (%.2f/%.2f to %.2f/%.2f)\n",
				iseg, seg, seg->ips, sdirection, seg->ipe,
				seg->line->points[seg->ips][X],
				seg->line->points[seg->ips][Y],
				seg->line->points[seg->ipe][X],
				seg->line->points[seg->ipe][Y]);
			}
		}
#	endif /* DEBUG_DIV_HOLES */

	/* Create pseudo subarea for checking */
	psub = create_subarea(NULL, NULL, NULL);

	/* Initialize all connecting points */
	copy_point(pentr, ZeroPoint);
	copy_point(pexit, ZeroPoint);
	copy_point(plast, ZeroPoint);

	/* Determine order for processing holes ... enclosed holes last! */
	outsub  = INITMEM(LOGICAL, bound->numhole);
	ihorder = INITMEM(int,     bound->numhole);
	for (ih=0; ih<bound->numhole; ih++)
		{
		hole = bound->holes[ih];
		if (!hole)
			{
			outsub[ih] = TRUE;
			continue;
			}
		for (ip=0; ip<hole->numpts; ip++)
			{
			subarea_test_point(sub, hole->points[ip], &dist,
					NullPoint, NullInt, NullInt, &hin);
			if (dist > HoleTol && Not(hin))
				{
				outsub[ih] = TRUE;
				break;
				}
			}
		if (ip >= hole->numpts) outsub[ih] = FALSE;
		}

	/* First add holes outside subarea (that may intersect subarea) */
	iho = 0;
	for (ih=0; ih<bound->numhole; ih++)
		{
		if ( outsub[ih]) ihorder[iho++] = ih;
		}

	/* Next add holes that intersect these holes (recursively) */
	if (iho < bound->numhole)
		{
		do
			{
			for (iha=0, ih=0; ih<bound->numhole; ih++)
				{
				if (!outsub[ih])
					{
					hole = bound->holes[ih];
					if (!hole) continue;
					for (jh=0; jh<iho; jh++)
						{
						xhole = bound->holes[ihorder[jh]];
						if (!xhole) continue;
						if (find_line_crossing(hole, xhole, 0, hole->points[0],
												NullPoint, NullInt, NullInt,
												NullLogical))
							{
							outsub[ih]     = TRUE;
							ihorder[iho++] = ih;
							iha++;
							break;
							}
						}
					}
				}
			}
			while (iha > 0);
		}

	/* Last add holes that are completely enclosed */
	if (iho < bound->numhole)
		{
		for (ih=0; ih<bound->numhole; ih++)
			{
			if (!outsub[ih]) ihorder[iho++] = ih;
			}
		}

	/* Create visible segments intersecting with each hole boundary */
	for (ih=0; ih<bound->numhole; ih++)
		{

		/* Initialize hole properties */
		hole = bound->holes[ihorder[ih]];
		if (!hole) continue;
		line_properties(hole, NullChar, &holecw, &xarea, NullFloat);
		if (xarea <= 1.0e-10) continue;

#		ifdef DEBUG_DIV_HOLES
		if (holecw)
			pr_diag("divide_subarea_holes", "Clockwise hole: %d - %d points\n",
			ihorder[ih], hole->numpts);
		else
			pr_diag("divide_subarea_holes", "CounterClockwise hole: %d - %d points\n",
			ihorder[ih], hole->numpts);
		pr_diag("divide_subarea_holes",
				" Visible segments lists: %d\n", sub->nsubvis);
#		endif /* DEBUG_DIV_HOLES */

		/* Are the subarea and hole oriented in the same direction? */
		/* Save portions of hole inside each subarea segment list   */
		insync = (segcw && holecw) || (Not(segcw) && Not(holecw));

		/* Initialize visible segment lists */
		xsv    = 0;
		xvlist = NullSubVisList;

		/* Loop through current visible subarea lists */
		for (isv=0; isv<sub->nsubvis; isv++)
			{

			/* Initialize visible area properties */
			svis = sub->subvis[isv];
			nseg = svis->numvis;
			segs = svis->segvis;

#			ifdef DEBUG_DIV_HOLES
			pr_diag("divide_subarea_holes",
					"  List: %d  Visible segments: %d\n", isv, nseg);
			for (iseg=0; iseg<nseg; iseg++)
				{
				seg = segs[iseg];
				sdirection = (seg_forward(seg))? sforward: sbackward;
				pr_diag("divide_subarea_holes",
					"    Segment %d [%x] - %d %s %d (%.2f/%.2f to %.2f/%.2f)\n",
					iseg, seg, seg->ips, sdirection, seg->ipe,
					seg->line->points[seg->ips][X],
					seg->line->points[seg->ips][Y],
					seg->line->points[seg->ipe][X],
					seg->line->points[seg->ipe][Y]);
				}
#			endif /* DEBUG_DIV_HOLES */

			/* Set pseudo subarea to point to current visible boundary */
			psub->numseg   = nseg;
			psub->segments = segs;

			/* Initialize temporary visible segment lists */
			tsv    = 0;
			tvlist = NullSubVisList;
			sllist = NullPtr(SLLIST *);
			nvseg  = 0;
			vseg   = NullSegmentList;

			/* Set starting subarea segment */
			pseg  = 0;
			seg   = segs[pseg];
			pspan = seg_forward(seg)? seg->ips: seg->ipe;
			fseg  = pseg;
			fspan = pspan;
			useg  = pseg;
			uspan = pspan;
			copy_point(pb, seg->line->points[pspan]);

			/* Create visible segments for each hole boundary */
			hentr = -1;
			hlast = -1;
			hexit = -1;
			hspan = -1;
			dentr = 0.0;
			dlast = 0.0;
			dexit = 0.0;
			dspan = 0.0;
			firstspan = TRUE;
			holemod   = FALSE;
			while (seglist_crossing(nseg, segs, TRUE, hole, pseg, pspan, pb,
					px, &xseg, &xspan, &hspan, &rt))
				{

				/* Special case intersection in first span of first segment */
				if (firstspan)
					{
					if (xseg == fseg && xspan != fspan) firstspan = FALSE;
					else if (xseg > fseg)               firstspan = FALSE;
					}

				/* Determine distance along span to crossing point */
				dspan = point_dist(hole->points[hspan], px);

				/* Have we stepped in or out of the hole? */
				in = (holecw)? rt: Not(rt);

#				ifdef DEBUG_DIV_HOLES
				if (rt)
					{
					pr_diag("divide_subarea_holes",
						" Seglist Crossing R ... pseg/pspan: %d %d  (useg/uspan: %d %d)  pb: %.2f/%.2f\n",
						pseg, pspan, useg, uspan, pb[X], pb[Y]);
					pr_diag("divide_subarea_holes",
						"                         xseg/xspan/hspan: %d %d %d  px: %.2f/%.2f\n",
						xseg, xspan, hspan, px[X], px[Y]);
					}
				else
					{
					pr_diag("divide_subarea_holes",
						" Seglist Crossing L ... pseg/pspan: %d %d  (useg/uspan: %d %d)  pb: %.2f/%.2f\n",
						pseg, pspan, useg, uspan, pb[X], pb[Y]);
					pr_diag("divide_subarea_holes",
						"                         xseg/xspan/hspan: %d %d %d  px: %.2f/%.2f\n",
						xseg, xspan, hspan, px[X], px[Y]);
					}
				if (in)
					pr_diag("divide_subarea_holes", "  Stepped in ...\n" );
				else
					pr_diag("divide_subarea_holes", "  Stepped out ...\n" );
				if (xseg == pseg && xspan == pspan)
					if (firstspan)
						pr_diag("divide_subarea_holes",
							" Seglist Crossing on first span of first segment!\n");
					else
						pr_diag("divide_subarea_holes",
							" Seglist Crossing on same span!\n");
#				endif /* DEBUG_DIV_HOLES */

				/* Stepped in now - add all segments from previous position */
				/* (start or previous exit) up until this crossing */
				if (in)
					{

					/* Add segments before crossover */
					for (iseg=useg; iseg<xseg; iseg++)
						{

						/* Copy segments (duplicate exclusive segments) */
						nvseg++;
						vseg = GETMEM(vseg, SEGMENT, nvseg);
						segexcl = seg_exclusive(segs[iseg]);
						vseg[nvseg-1] = copy_segment(segs[iseg], segexcl);
						seg = vseg[nvseg-1];
						if (iseg == useg)
							{
							if (seg_forward(seg)) seg->ips = uspan;
							else                  seg->ipe = uspan;
							}

#						ifdef DEBUG_DIV_HOLES
						sdirection = (seg_forward(seg))? sforward: sbackward;
						pr_diag("divide_subarea_holes",
							"  Add before step in segment [%x] %d %s %d (%.2f/%.2f to %.2f/%.2f)\n",
							segs[iseg], seg->ips, sdirection, seg->ipe,
							seg->line->points[seg->ips][X],
							seg->line->points[seg->ips][Y],
							seg->line->points[seg->ipe][X],
							seg->line->points[seg->ipe][Y]);
#						endif /* DEBUG_DIV_HOLES */
						}

					/* Check for crossover before start of segment */
					seg = segs[xseg];
					if (before_segment_start(seg, xspan))
						{

#						ifdef DEBUG_DIV_HOLES
						if (seg_forward(seg))
							pr_diag("divide_subarea_holes",
								"  Entry before start crossover? xspan: %d ips: %d\n",
								xspan, seg->ips);
						else
							pr_diag("divide_subarea_holes",
								"  Entry before start crossover? xspan: %d ipe: %d\n",
								xspan, seg->ipe);
#						endif /* DEBUG_DIV_HOLES */

						/* Set entry span and hole intersection */
						hentr = hspan;
						dentr = dspan;
						copy_point(pentr, px);

#						ifdef DEBUG_DIV_HOLES
						pr_diag("divide_subarea_holes",
							"  Step in intersection pentr (%.2f/%.2f)\n",
							pentr[X], pentr[Y]);
#						endif /* DEBUG_DIV_HOLES */

						/* Hole intersected with segments */
						holemod = TRUE;

						/* Reset starting span */
						reset_start_span(nseg, segs, xseg, xspan, px,
											&pseg, &pspan, &useg, &uspan, pb);
						continue;
						}

					/* Need to avoid duplicate crossover segments    */
					/*  ... except for crossover on very first span! */
					if (!firstspan && xseg == pseg && xspan == pspan)
						{
#						ifdef DEBUG_DIV_HOLES
						pr_diag("divide_subarea_holes",
							"  Multiple crossovers for segment [%x] %d (%.2f/%.2f)\n",
							segs[xseg], xspan,
							seg->line->points[xspan][X],
							seg->line->points[xspan][Y]);
#						endif /* DEBUG_DIV_HOLES */
						}

					/* Copy crossover segment (duplicate exclusive segments) */
					else
						{
						nvseg++;
						vseg = GETMEM(vseg, SEGMENT, nvseg);
						segexcl = seg_exclusive(segs[xseg]);
						vseg[nvseg-1] = copy_segment(segs[xseg], segexcl);
						seg = vseg[nvseg-1];
						if (seg_forward(seg)) seg->ipe = xspan;
						else                  seg->ips = xspan;
						if (xseg == useg)
							{
							if (seg_forward(seg)) seg->ips = uspan;
							else                  seg->ipe = uspan;
							}

#						ifdef DEBUG_DIV_HOLES
						sdirection = (seg_forward(seg))? sforward: sbackward;
						pr_diag("divide_subarea_holes",
							"  Add step in segment [%x] %d %s %d (%.2f/%.2f to %.2f/%.2f)\n",
							segs[xseg], seg->ips, sdirection, seg->ipe,
							seg->line->points[seg->ips][X],
							seg->line->points[seg->ips][Y],
							seg->line->points[seg->ipe][X],
							seg->line->points[seg->ipe][Y]);
#						endif /* DEBUG_DIV_HOLES */
						}

					/* Check for crossover after end of last segment */
					seg = segs[xseg];
					if ((xseg == nseg-1) && after_segment_end(seg, xspan))
						{

#						ifdef DEBUG_DIV_HOLES
						if (seg_forward(seg))
							pr_diag("divide_subarea_holes",
								"  Entry after end crossover? xspan: %d ipe: %d\n",
								xspan, seg->ipe);
						else
							pr_diag("divide_subarea_holes",
								"  Entry after end crossover? xspan: %d ips: %d\n",
								xspan, seg->ips);

						pr_diag("divide_subarea_holes",
							"  Entry after end crossover? hentr: %d  hlast: %d\n",
							hspan, hlast);
#						endif /* DEBUG_DIV_HOLES */
						}

					/* Set entry span and hole intersection */
					hentr = hspan;
					dentr = dspan;
					copy_point(pentr, px);

#					ifdef DEBUG_DIV_HOLES
					pr_diag("divide_subarea_holes",
						"  Step in intersection pentr (%.2f/%.2f)\n",
						pentr[X], pentr[Y]);
#					endif /* DEBUG_DIV_HOLES */

					/* Hole intersected with segments */
					holemod = TRUE;

					/* Reset starting span */
					reset_start_span(nseg, segs, xseg, xspan, px,
										&pseg, &pspan, &useg, &uspan, pb);
					continue;
					}

				/* Stepped out now - add intersection segments and */
				/*  segments from the appropriate side of the hole */
				else
					{

					/* If we start inside the hole then we have to save the */
					/*  intersection, reset the starting span, and add the  */
					/*  remaining part of the hole at the end               */
					if (!holemod)
						{

						/* Reset starting span */
						reset_start_span(nseg, segs, xseg, xspan, px,
											&pseg, &pspan, &useg, &uspan, pb);

						/* Set last span and hole intersection */
						holemod = TRUE;
						hlast   = hspan;
						dlast   = dspan;
						copy_point(plast, px);

#						ifdef DEBUG_DIV_HOLES
						pr_diag("divide_subarea_holes",
							"  Last segment plast (%.2f/%.2f)\n",
							plast[X], plast[Y]);
#						endif /* DEBUG_DIV_HOLES */

						continue;
						}

					/* Check for crossover before start of segment */
					seg = segs[xseg];
					if (before_segment_start(seg, xspan))
						{

#						ifdef DEBUG_DIV_HOLES
						if (seg_forward(seg))
							pr_diag("divide_subarea_holes",
								"  Exit before start crossover? xspan: %d ips: %d\n",
								xspan, seg->ips);
						else
							pr_diag("divide_subarea_holes",
								"  Exit before start crossover? xspan: %d ipe: %d\n",
								xspan, seg->ipe);
#						endif /* DEBUG_DIV_HOLES */
						}

					/* Check for crossover after end of last segment */
					seg = segs[xseg];
					if ((xseg == nseg-1) && after_segment_end(seg, xspan))
						{

#						ifdef DEBUG_DIV_HOLES
						if (seg_forward(seg))
							pr_diag("divide_subarea_holes",
								"  Exit after end crossover? xspan: %d ipe: %d\n",
								xspan, seg->ipe);
						else
							pr_diag("divide_subarea_holes",
								"  Exit after end crossover? xspan: %d ips: %d\n",
								xspan, seg->ips);
#						endif /* DEBUG_DIV_HOLES */

						/* Set last span and hole intersection */
						holemod = TRUE;
						hlast   = hspan;
						dlast   = dspan;
						copy_point(plast,  px);

#						ifdef DEBUG_DIV_HOLES
						pr_diag("divide_subarea_holes",
							"  Step out intersection (after) plast (%.2f/%.2f)\n",
							plast[X], plast[Y]);
#						endif /* DEBUG_DIV_HOLES */

						break;
						}

					/* Hole intersected with segments */
					holemod = TRUE;

					/* Reset starting span */
					reset_start_span(nseg, segs, xseg, xspan, px,
											&pseg, &pspan, &useg, &uspan, pb);

					/* Check if segments enter and exit hole on same span */
					if (hentr == hspan)
						{

						/* Add exclusive segment with step in intersection */
						xline = create_line();
						add_point_to_line(xline, pentr);
						nvseg++;
						vseg = GETMEM(vseg, SEGMENT, nvseg);
						vseg[nvseg-1] = create_segment();
						seg  = vseg[nvseg-1];
						define_segment(seg, xline, TRUE, 0, 0, TRUE);

#						ifdef DEBUG_DIV_HOLES
						pr_diag("divide_subarea_holes",
							"  Add step in intersection (same span) pentr (%.2f/%.2f)\n",
							pentr[X], pentr[Y]);
#						endif /* DEBUG_DIV_HOLES */

						/* If enter/exit are not in proper order */
						/*  save hole information for later in process */
						if ((insync && (dentr < dspan))
								|| (Not(insync) && (dentr > dspan)))
							{

#							ifdef DEBUG_DIV_HOLES
							if (insync)
								{
								pr_diag("divide_subarea_holes",
									"  Out of order enter/exit on same span of hole - dentr<dspan: %.2f/%.2f\n",
									dentr, dspan);
								}
							else
								{
								pr_diag("divide_subarea_holes",
									"  Out of order enter/exit on same span of hole - dentr>dspan: %.2f/%.2f\n",
									dentr, dspan);
								}
							pr_diag("divide_subarea_holes",
								"  Saving %d segments in temporary list [%d]\n",
								nvseg, tsv);
#							endif /* DEBUG_DIV_HOLES */

							/* Create a temporary segment list */
							tsv++;
							tvlist = GETMEM(tvlist, SUBVIS, tsv);
							sllist = GETMEM(sllist, SLLIST, tsv);
							tvlist[tsv-1] = create_subvis();
							tvlist[tsv-1]->numvis = nvseg;
							tvlist[tsv-1]->segvis = vseg;
							nvseg = 0;
							vseg  = NullSegmentList;

							/* Save hole entry and previous exit information */
							/*  for creating complete segment lists */
							sllist[tsv-1].hexit  = hexit;
							sllist[tsv-1].dexit  = dexit;
							sllist[tsv-1].hentr  = hentr;
							sllist[tsv-1].dentr  = dentr;
							sllist[tsv-1].slstat = SLinitial;
							sllist[tsv-1].lexit  = -1;
							sllist[tsv-1].lentr  = -1;

							/* Save current hole exit information for */
							/*  creating subsequent segment list */
							hexit = hspan;
							dexit = dspan;
							}
						}

					/* Otherwise add step in intersection and */
					/*  save hole information for later in process */
					else
						{

						/* Set hole entry and exit parameters */
						if (insync)
							{
							npts = hole->numpts;
							ips  = hspan + 1;
							ipe  = hentr;
							if (ipe < ips) ipe += npts;
							}
						else
							{
							npts = hole->numpts;
							ips  = hentr + 1;
							ipe  = hspan;
							if (ipe < ips) ipe += npts;
							}

						/* Add exclusive segment with step in intersection */
						xline = create_line();
						add_point_to_line(xline, pentr);
						nvseg++;
						vseg = GETMEM(vseg, SEGMENT, nvseg);
						vseg[nvseg-1] = create_segment();
						seg  = vseg[nvseg-1];
						define_segment(seg, xline, TRUE, 0, 0, TRUE);

#						ifdef DEBUG_DIV_HOLES
						pr_diag("divide_subarea_holes",
							"  Add step in intersection pentr (%.2f/%.2f)\n",
							pentr[X], pentr[Y]);
#						endif /* DEBUG_DIV_HOLES */

						/* Save hole information in temporary segment list */
						tsv++;
						tvlist = GETMEM(tvlist, SUBVIS, tsv);
						sllist = GETMEM(sllist, SLLIST, tsv);
						tvlist[tsv-1] = create_subvis();
						tvlist[tsv-1]->numvis = nvseg;
						tvlist[tsv-1]->segvis = vseg;
						nvseg = 0;
						vseg  = NullSegmentList;

#						ifdef DEBUG_DIV_HOLES
						pr_diag("divide_subarea_holes",
							"  Hole segment - hexit to hentr and hspan (%d to %d and %d)\n",
							hexit, hentr, hspan);
						pr_diag("divide_subarea_holes",
							"  Saving %d segments in temporary list [%d]\n",
							tvlist[tsv-1]->numvis, tsv-1);
#						endif /* DEBUG_DIV_HOLES */

						/* Save hole entry and previous exit information */
						/*  for creating complete segment lists */
						sllist[tsv-1].hexit  = hexit;
						sllist[tsv-1].dexit  = dexit;
						sllist[tsv-1].hentr  = hentr;
						sllist[tsv-1].dentr  = dentr;
						sllist[tsv-1].slstat = SLinitial;
						sllist[tsv-1].lexit  = -1;
						sllist[tsv-1].lentr  = -1;

						/* Check for hole enter and exit on same span */
						if (hentr == hexit || hentr == hspan)
							{

#							ifdef DEBUG_DIV_HOLES
							pr_diag("divide_subarea_holes",
								"  Step out intersection on same hole span! hentr hexit hspan: %d %d %d\n",
								hentr, hexit, hspan);
#							endif /* DEBUG_DIV_HOLES */
							}

						/* Save current hole exit information for */
						/*  creating subsequent segment list */
						hexit = hspan;
						dexit = dspan;
						}

					/* Set hole intersection */
					copy_point(pexit, px);

					/* Add exclusive segment with step out intersection */
					xline = create_line();
					add_point_to_line(xline, pexit);
					nvseg++;
					vseg = GETMEM(vseg, SEGMENT, nvseg);
					vseg[nvseg-1] = create_segment();
					seg  = vseg[nvseg-1];
					define_segment(seg, xline, TRUE, 0, 0, TRUE);

#					ifdef DEBUG_DIV_HOLES
					pr_diag("divide_subarea_holes",
						"  Add step out intersection pexit (%.2f/%.2f)\n",
						pexit[X], pexit[Y]);
#					endif /* DEBUG_DIV_HOLES */

					/* Reset hole enter parameter */
					hentr = -1;
					dentr = 0.0;
					}
				}

			/* If we finish inside the hole - add intersection segments */
			/*  and the remaining segment of the hole */
			if (holemod && hlast >= 0)
				{

				/* Check if segments enter and exit hole on same span */
				if (hentr == hlast)
					{

					/* Add exclusive segment with step in intersection */
					xline = create_line();
					add_point_to_line(xline, pentr);
					nvseg++;
					vseg = GETMEM(vseg, SEGMENT, nvseg);
					vseg[nvseg-1] = create_segment();
					seg  = vseg[nvseg-1];
					define_segment(seg, xline, TRUE, 0, 0, TRUE);

#					ifdef DEBUG_DIV_HOLES
					pr_diag("divide_subarea_holes",
						"  Add step in intersection (same span) (last) pentr (%.2f/%.2f)\n",
						pentr[X], pentr[Y]);
#					endif /* DEBUG_DIV_HOLES */

					/* If enter/exit are not in proper order */
					/*  save hole information for later in process */
					if ((insync && (dentr < dlast))
							|| (Not(insync) && (dentr > dlast)))
						{

#						ifdef DEBUG_DIV_HOLES
						if (insync)
							{
							pr_diag("divide_subarea_holes",
								"  Out of order enter/exit on same span of hole - dentr<dlast: %.2f/%.2f\n",
								dentr, dlast);
							}
						else
							{
							pr_diag("divide_subarea_holes",
								"  Out of order enter/exit on same span of hole - dentr>dlast: %.2f/%.2f\n",
								dentr, dlast);
							}
						pr_diag("divide_subarea_holes",
							"  Saving %d segments in temporary list [%d]\n",
							nvseg, tsv);
#						endif /* DEBUG_DIV_HOLES */

						/* Create a temporary segment list */
						tsv++;
						tvlist = GETMEM(tvlist, SUBVIS, tsv);
						sllist = GETMEM(sllist, SLLIST, tsv);
						tvlist[tsv-1] = create_subvis();
						tvlist[tsv-1]->numvis = nvseg;
						tvlist[tsv-1]->segvis = vseg;
						nvseg = 0;
						vseg  = NullSegmentList;

						/* Save hole entry and previous exit information */
						/*  for creating complete segment lists */
						sllist[tsv-1].hexit  = hexit;
						sllist[tsv-1].dexit  = dexit;
						sllist[tsv-1].hentr  = hentr;
						sllist[tsv-1].dentr  = dentr;
						sllist[tsv-1].slstat = SLinitial;
						sllist[tsv-1].lexit  = -1;
						sllist[tsv-1].lentr  = -1;

						/* Save current hole exit information for */
						/*  creating subsequent segment list */
						hexit = hlast;
						dexit = dlast;
						}
					}

				/* Otherwise add step in intersection and */
				/*  save hole information for later in process */
				else
					{

					/* Set hole entry and exit parameters */
					if (insync)
						{
						npts = hole->numpts;
						ips  = hlast + 1;
						ipe  = hentr;
						if (ipe < ips) ipe += npts;
						}
					else
						{
						npts = hole->numpts;
						ips  = hentr + 1;
						ipe  = hlast;
						if (ipe < ips) ipe += npts;
						}

					/* Add exclusive segment with step in intersection */
					xline = create_line();
					add_point_to_line(xline, pentr);
					nvseg++;
					vseg = GETMEM(vseg, SEGMENT, nvseg);
					vseg[nvseg-1] = create_segment();
					seg  = vseg[nvseg-1];
					define_segment(seg, xline, TRUE, 0, 0, TRUE);

#					ifdef DEBUG_DIV_HOLES
					pr_diag("divide_subarea_holes",
						"  Add step in intersection (last) pentr (%.2f/%.2f)\n",
						pentr[X], pentr[Y]);
#					endif /* DEBUG_DIV_HOLES */

					/* Save hole information in temporary segment list */
					tsv++;
					tvlist = GETMEM(tvlist, SUBVIS, tsv);
					sllist = GETMEM(sllist, SLLIST, tsv);
					tvlist[tsv-1] = create_subvis();
					tvlist[tsv-1]->numvis = nvseg;
					tvlist[tsv-1]->segvis = vseg;
					nvseg = 0;
					vseg  = NullSegmentList;

#					ifdef DEBUG_DIV_HOLES
					pr_diag("divide_subarea_holes",
						"  Hole segment (last) - hexit to hentr and hlast (%d to %d and %d)\n",
						hexit, hentr, hlast);
					pr_diag("divide_subarea_holes",
						"  Saving %d segments in temporary list [%d]\n",
						tvlist[tsv-1]->numvis, tsv-1);
#					endif /* DEBUG_DIV_HOLES */

					/* Save hole entry and previous exit information */
					/*  for creating complete segment lists */
					sllist[tsv-1].hexit  = hexit;
					sllist[tsv-1].dexit  = dexit;
					sllist[tsv-1].hentr  = hentr;
					sllist[tsv-1].dentr  = dentr;
					sllist[tsv-1].slstat = SLinitial;
					sllist[tsv-1].lexit  = -1;
					sllist[tsv-1].lentr  = -1;

					/* Save current hole exit information for */
					/*  creating subsequent segment list */
					hexit = hlast;
					dexit = dlast;
					}

				/* Add exclusive segment with step out intersection */
				xline = create_line();
				add_point_to_line(xline, plast);
				nvseg++;
				vseg = GETMEM(vseg, SEGMENT, nvseg);
				vseg[nvseg-1] = create_segment();
				seg  = vseg[nvseg-1];
				define_segment(seg, xline, TRUE, 0, 0, TRUE);

#				ifdef DEBUG_DIV_HOLES
				pr_diag("divide_subarea_holes",
					"  Add step out intersection (last) plast (%.2f/%.2f)\n",
					plast[X], plast[Y]);
#				endif /* DEBUG_DIV_HOLES */

				/* Reset hole enter parameter */
				hentr = -1;
				dentr = 0.0;
				}

			/* No (further) crossings - add remainder of segment list */
			else if (holemod)
				{
				xseg = nseg - 1;
				seg  = segs[xseg];
				xspan = seg_forward(seg)? seg->ipe: seg->ips;

				/* Copy segments (duplicate exclusive segments) */
				for (iseg=useg; iseg<=xseg; iseg++)
					{
					nvseg++;
					vseg = GETMEM(vseg, SEGMENT, nvseg);
					segexcl = seg_exclusive(segs[iseg]);
					vseg[nvseg-1] = copy_segment(segs[iseg], segexcl);
					seg  = vseg[nvseg-1];
					if (iseg == useg)
						{
						if (seg_forward(seg)) seg->ips = uspan;
						else                  seg->ipe = uspan;
						}
					if (iseg == xseg)
						{
						if (seg_forward(seg)) seg->ipe = xspan;
						else                  seg->ips = xspan;
						}

#					ifdef DEBUG_DIV_HOLES
					sdirection = (seg_forward(seg))? sforward: sbackward;
					pr_diag("divide_subarea_holes",
						"  Add end segment [%x] %d %s %d (%.2f/%.2f to %.2f/%.2f)\n",
						segs[iseg], seg->ips, sdirection, seg->ipe,
						seg->line->points[seg->ips][X],
						seg->line->points[seg->ips][Y],
						seg->line->points[seg->ipe][X],
						seg->line->points[seg->ipe][Y]);
#					endif /* DEBUG_DIV_HOLES */
					}
				}

			/* Save remaining temporary visible segment list (if required) */
			if (holemod && nvseg > 0)
				{
				tsv++;
				tvlist = GETMEM(tvlist, SUBVIS, tsv);
				sllist = GETMEM(sllist, SLLIST, tsv);
				tvlist[tsv-1] = create_subvis();
				tvlist[tsv-1]->numvis = nvseg;
				tvlist[tsv-1]->segvis = vseg;
				nvseg = 0;
				vseg  = NullSegmentList;

				if (hexit >= 0 || hentr >= 0)
					{

#					ifdef DEBUG_DIV_HOLES
					pr_diag("divide_subarea_holes",
						"  Out of order hole segment (last) - hexit to hentr (%d to %d)\n",
						hexit, hentr);
					pr_diag("divide_subarea_holes",
						"  Saving %d segments in temporary list [%d]\n",
						tvlist[tsv-1]->numvis, tsv-1);
#					endif /* DEBUG_DIV_HOLES */

					/* Save hole entry and previous exit information */
					/*  for creating complete segment lists */
					sllist[tsv-1].hexit  = hexit;
					sllist[tsv-1].dexit  = dexit;
					sllist[tsv-1].hentr  = hentr;
					sllist[tsv-1].dentr  = dentr;
					sllist[tsv-1].slstat = SLinitial;
					sllist[tsv-1].lexit  = -1;
					sllist[tsv-1].lentr  = -1;
					}
				else
					{

#					ifdef DEBUG_DIV_HOLES
					pr_diag("divide_subarea_holes",
						"  Saving %d segments in temporary list [%d]\n",
						tvlist[tsv-1]->numvis, tsv-1);
#					endif /* DEBUG_DIV_HOLES */

					sllist[tsv-1].hexit  = hexit;
					sllist[tsv-1].dexit  = dexit;
					sllist[tsv-1].hentr  = hentr;
					sllist[tsv-1].dentr  = dentr;
					sllist[tsv-1].slstat = SLextra;
					sllist[tsv-1].lexit  = -1;
					sllist[tsv-1].lentr  = -1;
					}
				}

			/* Build visible segment lists from temporary lists */
			if (holemod)
				{

				/* Combine visible segment lists for all holes */
				if (tsv > 1)
					{

#					ifdef DEBUG_DIV_HOLES
					for (itv=0; itv<tsv; itv++)
						{
						pr_diag("divide_subarea_holes",
							"  Info for: %d  exit: %d %.2f  entry: %d %.2f\n",
							itv, sllist[itv].hexit, sllist[itv].dexit,
							sllist[itv].hentr, sllist[itv].dentr);
						}
#					endif /* DEBUG_DIV_HOLES */

					/* Combine visible segment lists until complete */
					hentr = -1;
					dentr = 0.0;
					hexit = -1;
					dexit = 0.0;
					while (TRUE)
						{

						/* Find the next hole entry index */
						lentr = next_hole_entry(tsv, sllist, insync,
												hexit, dexit);
						if (lentr < 0 ) break;
						hentr = sllist[lentr].hentr;
						dentr = sllist[lentr].dentr;

						/* Find the next matching hole exit index */
						lexit = next_hole_exit(tsv, sllist, insync,
												hentr, dentr);
						if (lexit < 0 ) break;
						hexit = sllist[lexit].hexit;
						dexit = sllist[lexit].dexit;

#						ifdef DEBUG_DIV_HOLES
						pr_diag("divide_subarea_holes",
							"  Matching hole exit (%d segment %d) with entry (%d segment %d)\n",
							hexit, lexit, hentr, lentr);
#						endif /* DEBUG_DIV_HOLES */

						/* Hole exit and entry are in same list      */
						/*  but exit and entry are on the same span! */
						/*  ... so hole segment not needed in list   */
						if (lentr == lexit && hentr == hexit)
							{

							/* Bail out if problem with entry/exit */
							if ((insync && (dentr < dexit))
									|| (Not(insync) && (dentr > dexit)))
								{
								pr_error("Holes",
									"Error with hole entry/exit in same list and on same span!\n");
								pr_error("Holes",
									" ... Try more points in line!\n");
								break;
								}

#							ifdef DEBUG_DIV_HOLES
							pr_diag("divide_subarea_holes",
								"  Hole segment not needed - hexit/hentr (%d %d)\n",
								hexit, hentr);
#							endif /* DEBUG_DIV_HOLES */

							/* Segment list should be complete */
							sllist[lentr].slstat = SLdone;
							}

						/* Hole exit and entry are in same list */
						/*  ... so just add hole segment to list */
						else if (lentr == lexit)
							{

#							ifdef DEBUG_DIV_HOLES
							pr_diag("divide_subarea_holes",
								"  Hole segment - hexit to hentr (%d to %d)\n",
								hexit, hentr);
#							endif /* DEBUG_DIV_HOLES */

							/* Add hole segment to the list */
							nvseg = tvlist[lentr]->numvis;
							vseg  = tvlist[lentr]->segvis;
							nvseg++;
							vseg  = GETMEM(vseg, SEGMENT, nvseg);
							vseg[nvseg-1] = create_segment();
							seg   = vseg[nvseg-1];

							if (insync) define_segment(seg, hole, FALSE,
														hexit+1, hentr, FALSE);
							else        define_segment(seg, hole, FALSE,
														hentr+1, hexit, TRUE);

#							ifdef DEBUG_DIV_HOLES
							sdirection = (seg_forward(seg))? sforward: sbackward;
							pr_diag("divide_subarea_holes",
								"  Add hole segment [%x] %d %s %d (%.2f/%.2f to %.2f/%.2f)\n",
								hole, seg->ips, sdirection, seg->ipe,
								seg->line->points[seg->ips][X],
								seg->line->points[seg->ips][Y],
								seg->line->points[seg->ipe][X],
								seg->line->points[seg->ipe][Y]);
							pr_diag("divide_subarea_holes",
								"  Add hole segment to temporary list [%d]\n",
								lentr);
#							endif /* DEBUG_DIV_HOLES */

							/* Reset segment list */
							tvlist[lentr]->numvis = nvseg;
							tvlist[lentr]->segvis = vseg;
							nvseg = 0;
							vseg  = NullSegmentList;

							/* Segment list should be complete */
							sllist[lentr].slstat = SLdone;
							}

						/* Hole exit and entry are from different lists */
						/*  ... so add the hole segment (if required)   */
						/*       and segments from the other list       */
						else
							{

							/* Reset entry for partially completed holes */
							if (sllist[lentr].slstat == SLpartial)
								{

#								ifdef DEBUG_DIV_HOLES
								pr_diag("divide_subarea_holes",
									"  Resetting hole entry (%d) to (%d)\n",
									lentr, sllist[lentr].lentr);
#								endif /* DEBUG_DIV_HOLES */

								/* Bail out if problem with entry reset */
								if (sllist[lentr].lentr == lentr)
									{
									pr_error("Holes",
										"Error with hole entry reset!\n");
									pr_error("Holes",
										" ... Try more points in line!\n");
									break;
									}

								/* Entry segment list now complete */
								sllist[lentr].slstat = SLskip;

								/* Reset entry segment list */
								lentr = sllist[lentr].lentr;
								}

							/* Hole exit and reset entry are in same list */
							/*  but exit and entry are on the same span!  */
							/*  ... so hole segment not needed in list    */
							if (lentr == lexit && hentr == hexit)
								{

								/* Bail out if problem with entry/exit */
								if ((insync && (dentr < dexit))
										|| (Not(insync) && (dentr > dexit)))
									{
									pr_error("Holes",
										"Error with hole entry/exit in same (reset) list and on same span!\n");
									pr_error("Holes",
										" ... Try more points in line!\n");
									break;
									}

#								ifdef DEBUG_DIV_HOLES
								pr_diag("divide_subarea_holes",
									"  Hole segment not needed - hexit/hentr (%d %d)\n",
									hexit, hentr);
#								endif /* DEBUG_DIV_HOLES */

								/* Segment list should be complete */
								sllist[lentr].slstat = SLdone;
								}

							/* Hole exit and reset entry are in same list */
							/*  ... so just add hole segment to list */
							else if (lentr == lexit)

								{

#								ifdef DEBUG_DIV_HOLES
								pr_diag("divide_subarea_holes",
									"  Hole segment - hexit to hentr (%d to %d)\n",
									hexit, hentr);
#								endif /* DEBUG_DIV_HOLES */

								/* Add hole segment to the list */
								nvseg = tvlist[lentr]->numvis;
								vseg  = tvlist[lentr]->segvis;
								nvseg++;
								vseg  = GETMEM(vseg, SEGMENT, nvseg);
								vseg[nvseg-1] = create_segment();
								seg   = vseg[nvseg-1];

								if (insync) define_segment(seg, hole, FALSE,
														hexit+1, hentr, FALSE);
								else        define_segment(seg, hole, FALSE,
														hentr+1, hexit, TRUE);

#								ifdef DEBUG_DIV_HOLES
								sdirection = (seg_forward(seg))? sforward: sbackward;
								pr_diag("divide_subarea_holes",
									"  Add hole segment [%x] %d %s %d (%.2f/%.2f to %.2f/%.2f)\n",
									hole, seg->ips, sdirection, seg->ipe,
									seg->line->points[seg->ips][X],
									seg->line->points[seg->ips][Y],
									seg->line->points[seg->ipe][X],
									seg->line->points[seg->ipe][Y]);
								pr_diag("divide_subarea_holes",
									"  Add hole segment to temporary list [%d]\n",
									lentr);
#								endif /* DEBUG_DIV_HOLES */

								/* Reset segment list */
								tvlist[lentr]->numvis = nvseg;
								tvlist[lentr]->segvis = vseg;
								nvseg = 0;
								vseg  = NullSegmentList;

								/* Segment list should be complete */
								sllist[lentr].slstat = SLdone;
								}

							/* Hole exit and entry are from different lists */
							/*  but exit and entry are on the same span!    */
							/*  ... so just add second list                 */
							else if (hentr == hexit)
								{

								/* Bail out if problem with entry/exit */
								if ((insync && (dentr < dexit))
										|| (Not(insync) && (dentr > dexit)))
									{
									pr_error("Holes",
										"Error with hole entry/exit in different lists but on same span!\n");
									pr_error("Holes",
										" ... Try more points in line!\n");
									break;
									}

#								ifdef DEBUG_DIV_HOLES
								pr_diag("divide_subarea_holes",
									"  Hole segment not needed - hexit/hentr (%d %d)\n",
									hexit, hentr);
								pr_diag("divide_subarea_holes",
									"  Add %d segments from temporary list [%d] to [%d]\n",
									tvlist[lexit]->numvis, lexit, lentr);
#								endif /* DEBUG_DIV_HOLES */

								/* Add remaining segments to the list */
								nvseg = tvlist[lentr]->numvis;
								vseg  = tvlist[lentr]->segvis;
								for (itv=0; itv<tvlist[lexit]->numvis; itv++)
									{
									nvseg++;
									vseg  = GETMEM(vseg, SEGMENT, nvseg);
									seg   = tvlist[lexit]->segvis[itv];
									vseg[nvseg-1] = seg;

#									ifdef DEBUG_DIV_HOLES
									sdirection = (seg_forward(seg))? sforward: sbackward;
									pr_diag("divide_subarea_holes",
										"  Add segment [%x] %d %s %d (%.2f/%.2f to %.2f/%.2f)\n",
										seg, seg->ips, sdirection, seg->ipe,
										seg->line->points[seg->ips][X],
										seg->line->points[seg->ips][Y],
										seg->line->points[seg->ipe][X],
										seg->line->points[seg->ipe][Y]);
#									endif /* DEBUG_DIV_HOLES */
									}

								/* Reset segment lists */
								tvlist[lentr]->numvis = nvseg;
								tvlist[lentr]->segvis = vseg;
								tvlist[lexit]->numvis = 0;
								tvlist[lexit]->segvis = NullSegmentList;
								nvseg = 0;
								vseg  = NullSegmentList;

								/* Combined segment list from beginning */
								/*  and end lists should now be complete */
								if (sllist[lentr].hexit < 0
										&& sllist[lexit].hentr < 0)
									{
									sllist[lentr].slstat = SLdone;
									sllist[lexit].slstat = SLskip;
									}

								/* Combined segment list not yet complete */
								/*  ... so reset parameters */
								else
									{
									sllist[lentr].slstat = SLpartial;
									sllist[lentr].lentr  = lentr;
									sllist[lentr].lexit  = lexit;
									sllist[lexit].slstat = SLpartial;
									sllist[lexit].lentr  = lentr;
									sllist[lexit].lexit  = lexit;
									}
								}

							/* Hole exit and entry are from different lists */
							/*  ... so add hole segment and second list */
							else
								{

#								ifdef DEBUG_DIV_HOLES
								pr_diag("divide_subarea_holes",
									"  Hole segment - hexit to hentr (%d to %d)\n",
									hexit, hentr);
#								endif /* DEBUG_DIV_HOLES */

								/* Add hole segment to the list */
								nvseg = tvlist[lentr]->numvis;
								vseg  = tvlist[lentr]->segvis;
								nvseg++;
								vseg  = GETMEM(vseg, SEGMENT, nvseg);
								vseg[nvseg-1] = create_segment();
								seg   = vseg[nvseg-1];

								if (insync) define_segment(seg, hole, FALSE,
														hexit+1, hentr, FALSE);
								else        define_segment(seg, hole, FALSE,
														hentr+1, hexit, TRUE);

#								ifdef DEBUG_DIV_HOLES
								sdirection = (seg_forward(seg))? sforward: sbackward;
								pr_diag("divide_subarea_holes",
									"  Add hole segment [%x] %d %s %d (%.2f/%.2f to %.2f/%.2f)\n",
									hole, seg->ips, sdirection, seg->ipe,
									seg->line->points[seg->ips][X],
									seg->line->points[seg->ips][Y],
									seg->line->points[seg->ipe][X],
									seg->line->points[seg->ipe][Y]);
								pr_diag("divide_subarea_holes",
									"  Add hole segment to temporary list [%d]\n",
									lentr);
								pr_diag("divide_subarea_holes",
									"  Add %d segments from temporary list [%d] to [%d]\n",
									tvlist[lexit]->numvis, lexit, lentr);
#								endif /* DEBUG_DIV_HOLES */

								/* Add remaining segments to the list */
								for (itv=0; itv<tvlist[lexit]->numvis; itv++)
									{
									nvseg++;
									vseg  = GETMEM(vseg, SEGMENT, nvseg);
									seg   = tvlist[lexit]->segvis[itv];
									vseg[nvseg-1] = seg;

#									ifdef DEBUG_DIV_HOLES
									sdirection = (seg_forward(seg))? sforward: sbackward;
									pr_diag("divide_subarea_holes",
										"  Add segment [%x] %d %s %d (%.2f/%.2f to %.2f/%.2f)\n",
										seg, seg->ips, sdirection, seg->ipe,
										seg->line->points[seg->ips][X],
										seg->line->points[seg->ips][Y],
										seg->line->points[seg->ipe][X],
										seg->line->points[seg->ipe][Y]);
#									endif /* DEBUG_DIV_HOLES */
									}

								/* Reset segment lists */
								tvlist[lentr]->numvis = nvseg;
								tvlist[lentr]->segvis = vseg;
								tvlist[lexit]->numvis = 0;
								tvlist[lexit]->segvis = NullSegmentList;
								nvseg = 0;
								vseg  = NullSegmentList;

								/* Combined segment list from beginning */
								/*  and end lists should now be complete */
								if (sllist[lentr].hexit < 0
										&& sllist[lexit].hentr < 0)
									{
									sllist[lentr].slstat = SLdone;
									sllist[lexit].slstat = SLskip;
									}

								/* Combined segment list not yet complete */
								/*  ... so reset parameters */
								else
									{
									sllist[lentr].slstat = SLpartial;
									sllist[lentr].lentr  = lentr;
									sllist[lentr].lexit  = lexit;
									sllist[lexit].slstat = SLpartial;
									sllist[lexit].lentr  = lentr;
									sllist[lexit].lexit  = lexit;
									}
								}
							}
						}
					}

				/* Add temporary visible segment lists to full list */
				for (itv=0; itv<tsv; itv++)
					{

					/* Skip segment lists that have been combined with others */
					if (sllist[itv].slstat != SLdone
							&& sllist[itv].slstat != SLextra) continue;

					/* Save temporary visible segment list */
					xsv++;
					xvlist = GETMEM(xvlist, SUBVIS, xsv);
					xvlist[xsv-1] = tvlist[itv];
					tvlist[itv]   = NullSubVis;
					}
				FREEMEM(tvlist);
				FREEMEM(sllist);
				tsv = 0;

#				ifdef DEBUG_DIV_HOLES
				pr_diag("divide_subarea_holes",
					"  Rebuild visible segment list with %d lists\n", xsv);
				for (itv=0; itv<xsv; itv++)
					{
					pr_diag("divide_subarea_holes",
						"    Segment list: %d  with %d segments\n",
						itv, xvlist[itv]->numvis);
					}
#				endif /* DEBUG_DIV_HOLES */
				}

			/* Hole did not intersect subarea segment list */
			/*  ... so use original visible segment list */
			else
				{

				if (tsv > 0 || NotNull(tvlist))
					{
					pr_error("Holes",
						"Have tsv (%d) or tvlist without holemod!\n", tsv);
					pr_error("Holes",
						" ... Contact system administrator!\n");
					}

				/* Hole did not intersect subarea segment list */
				/*  ... so if any point on subarea segment list is inside */
				/*       hole, the segment list must be totally enclosed */
				seg = segs[0];
				line_test_point(hole, seg->line->points[seg->ips],
								NULL, NULL, NULL, &in, NULL);

				/* Subarea segment list was not inside hole */
				if (!in)
					{

					/* Save original segment list */
					xsv++;
					xvlist = GETMEM(xvlist, SUBVIS, xsv);
					xvlist[xsv-1]    = sub->subvis[isv];
					sub->subvis[isv] = NullSubVis;

					/* Hole did not intersect the subarea */
					/*  ... so if any hole point is inside subarea, the hole */
					/*       must be totally enclosed, so add it to hole list */
					subarea_test_point(psub, hole->points[0], &dist, NullPoint,
							NullInt, NullInt, &hin);
					if (hin || dist < HoleTol)
						{
						jh = sub->numhole++;
						sub->holes = GETMEM(sub->holes, LINE, sub->numhole);
						sub->holes[jh] = hole;
						}
					}
				}
			}

		/* Replace visible segment lists */
		if (sub->nsubvis > 0)
			{
			for (isv=0; isv<sub->nsubvis; isv++)
				(void) destroy_subvis(sub->subvis[isv]);
			FREEMEM(sub->subvis);
			sub->nsubvis = 0;
			}
		if (xsv > 0)
			{
			sub->nsubvis = xsv;
			sub->subvis  = INITMEM(SUBVIS, sub->nsubvis);
			for (isv=0; isv<sub->nsubvis; isv++)
				{
				sub->subvis[isv] = xvlist[isv];
				xvlist[isv]      = NullSubVis;
				}
			FREEMEM(xvlist);
			xsv = 0;
			}
		}

	/* Destroy pseudo subarea */
	psub->numseg   = 0;
	psub->segments = NullSegmentList;
	(void) destroy_subarea(psub);

	/* Destroy hole order parameters */
	FREEMEM(outsub);
	FREEMEM(ihorder);

	/* Return when finished */
	return TRUE;
	}

/***********************************************************************/

static	void	reset_start_span

	(
	int			nseg,
	SEGMENT		*segs,
	int			xseg,
	int			xspan,
	POINT		px,
	int			*pseg,
	int			*pspan,
	int			*useg,
	int			*uspan,
	POINT		pb
	)

	{
	int		np, ips, ipe, ispan, tspan, jseg, jspan;
	float	*npos, sdist2, edist2;
	double	ddx, ddy, dang;
	SEGMENT	seg;
	POINT	xpos;
	LOGICAL	cfirst, clast;

	/* Initialize return values */
	if (pseg)  *pseg  = -1;
	if (pspan) *pspan = -1;
	if (useg)  *useg  = -1;
	if (uspan) *uspan = -1;
	if (pb)    copy_point(pb, ZeroPoint);

	/* Make sure things are consistent */
	if (nseg < 1)      return;
	if (!segs)         return;
	if (xseg < 0)      return;
	if (xseg > nseg-1) return;

	/* Set current segment and span */
	jseg  = xseg;
	seg   = segs[jseg];
	ispan = xspan;

	/* Set segment parameters */
	np = seg->line->numpts;
	if (seg_forward(seg))
		{
		ips = seg->ips;
		ipe = seg->ipe;
		if (line_closed(seg->line) && ipe < ips)
			{
			if (ispan <= ipe) ispan += np;
			ipe += np;
			}
		}
	else
		{
		ips = seg->ipe;
		ipe = seg->ips;
		if (line_closed(seg->line) && ips < ipe)
			{
			if (ispan >= ipe) ispan -= np;
			ipe -= np;
			}
		if (ipe < 0)
			{
			ips   += np;
			ipe   += np;
			ispan += np;
			}
		}

	/* Check for intersection before start of span */
	tspan  = ispan%np;
	cfirst = before_segment_start(seg, tspan);

	/* >>>>> This should never occur <<<<< */
	if (cfirst && xseg == 0)
		{
		pr_error("reset_start_span",
					"Reset for crossing before start of first span!\n");
		}
	/* >>>>> This should never occur <<<<< */

	/* Set end position for intersection on current segment */
	clast = FALSE;
	if (seg_forward(seg))
		{

		/* Set next position on current span */
		tspan = ispan + 1;
		jspan = tspan%np;

		/* Have we left current span? */
		if (tspan > ipe)
			{

			/* Move on to next segment (or start point of first segment) */
			jseg = xseg + 1;
			if (jseg < nseg)
				{
				seg   = segs[jseg];
				jspan = seg_forward(seg)? seg->ips: seg->ipe;
				}
			else
				{
				seg   = segs[0];
				jspan = seg_forward(seg)? seg->ips: seg->ipe;
				clast = TRUE;
				}
			}
		npos = seg->line->points[jspan];
		}
	else
		{

		/* Set next position on current span */
		tspan = ispan - 1;
		jspan = tspan%np;

		/* Have we left current span? */
		if (tspan < ipe)
			{

			/* Move on to next segment (or start point of first segment) */
			jseg = xseg + 1;
			if (jseg < nseg)
				{
				seg   = segs[jseg];
				jspan = seg_forward(seg)? seg->ips: seg->ipe;
				}
			else
				{
				seg   = segs[0];
				jspan = seg_forward(seg)? seg->ips: seg->ipe;
				clast = TRUE;
				}
			}
		npos = seg->line->points[jspan];
		}

	/* Check if intersection is very close to end position on current segment */
	edist2 = point_dist2(px, npos);
	if (edist2 < EndTol*EndTol)
		{

		/* Reset end position for intersection on current segment */
		clast = FALSE;
		if (seg_forward(seg))
			{

			/* Set next position on current span */
			tspan++;
			jspan = tspan%np;

			/* Have we left current span? */
			if (tspan > ipe)
				{

				/* Move on to next segment (or start point of first segment) */
				jseg = xseg + 1;
				if (jseg < nseg)
					{
					seg   = segs[jseg];
					jspan = seg_forward(seg)? seg->ips: seg->ipe;
					}
				else
					{
					seg   = segs[0];
					jspan = seg_forward(seg)? seg->ips: seg->ipe;
					clast = TRUE;
					}
				}
			npos = seg->line->points[jspan];
			}
		else
			{

			/* Set next position on current span */
			tspan--;
			jspan = tspan%np;

			/* Have we left current span? */
			if (tspan < ipe)
				{

				/* Move on to next segment (or start point of first segment) */
				jseg = xseg + 1;
				if (jseg < nseg)
					{
					seg   = segs[jseg];
					jspan = seg_forward(seg)? seg->ips: seg->ipe;
					}
				else
					{
					seg   = segs[0];
					jspan = seg_forward(seg)? seg->ips: seg->ipe;
					clast = TRUE;
					}
				}
			npos = seg->line->points[jspan];
			}

#		ifdef DEBUG_DIV_HOLES
		pr_diag("reset_start_span",
			"    Coincident intersection ... edist: %.4f\n", sqrt(edist2));
#		endif /* DEBUG_DIV_HOLES */
		}

	/* Reset start position based on intersection and next position */
	sdist2 = point_dist2(px, npos);

	/* Start position set to start of next span */
	if (sdist2 < SegTol*SegTol)
		{

		if (pseg)  *pseg  = clast? nseg: jseg;
		if (pspan) *pspan = jspan;
		if (useg)  *useg  = clast? nseg: jseg;
		if (uspan) *uspan = jspan;
		if (pb)    copy_point(pb, npos);

#		ifdef DEBUG_DIV_HOLES
		if (clast)
			pr_diag("reset_start_span",
				"    Resetting (last) segment pseg/pspan: %d %d to %d (%d)\n",
				xseg, xspan, nseg, jspan);
		else
			pr_diag("reset_start_span",
				"    Resetting segment pseg/pspan: %d %d to %d %d\n",
				xseg, xspan, jseg, jspan);
#		endif /* DEBUG_DIV_HOLES */
		}

	/* Start position set to same span */
	else
		{
		ddx  = (double) (npos[X] - px[X]);
		ddy  = (double) (npos[Y] - px[Y]);
		dang = atan2(ddy, ddx);
		xpos[X] = px[X] + (float) (SegTol * cos(dang));
		xpos[Y] = px[Y] + (float) (SegTol * sin(dang));
		if (pseg)  *pseg  = xseg;
		if (pspan) *pspan = xspan;
		if (useg)  *useg  = clast? nseg: jseg;
		if (uspan) *uspan = jspan;
		if (pb)    copy_point(pb, xpos);

#		ifdef DEBUG_DIV_HOLES
		if (clast)
			pr_diag("reset_start_span",
				"    No reset of (last) segment pseg/pspan: %d %d (Next: %d %d)\n",
				xseg, xspan, nseg, jspan);
		else
			pr_diag("reset_start_span",
				"    No reset of segment pseg/pspan: %d %d (Next: %d %d)\n",
				xseg, xspan, jseg, jspan);
#		endif /* DEBUG_DIV_HOLES */
		}
	}

/***********************************************************************/

static	int		next_hole_entry

	(
	int			tsv,
	SLLIST		*sllist,
	LOGICAL		insync,
	int			hexit,
	float		dexit
	)

	{
	int		itv, hentr, hafter, lafter, hbefore, lbefore;
	float	dentr, dafter, dbefore;

	/* Set starting entry index */
	if (hexit < 0)
		{
		for (itv=0; itv<tsv; itv++)
			{
			hentr = sllist[itv].hentr;
			if (hentr < 0) continue;
			return itv;
			}
		/* >>>>> error if we get to here? <<<<< */
		return -1;
		}

	/* Search for next entry index based on last exit index */
	if (insync)
		{
		hbefore = -1;
		dbefore = 0.0;
		lbefore = -1;
		hafter  = hexit;
		dafter  = dexit;
		lafter  = -1;
		for (itv=0; itv<tsv; itv++)
			{
			if (sllist[itv].slstat == SLdone
					|| sllist[itv].slstat == SLskip) continue;
			hentr = sllist[itv].hentr;
			dentr = sllist[itv].dentr;
			if (hentr < 0) continue;

			/* Search for entry index on the same span */
			if (hentr == hexit && dentr < dexit)
				{
				if (hentr > hbefore || (hentr == hbefore && dentr > dbefore))
					{
					hbefore = hentr;
					dbefore = dentr;
					lbefore = itv;
					}
				}

			/* Search for closest index before the exit index */
			else if (hentr < hexit)
				{
				if (hentr > hbefore || (hentr == hbefore && dentr > dbefore))
					{
					hbefore = hentr;
					dbefore = dentr;
					lbefore = itv;
					}
				}

			/* Search for largest index (backwards from end of hole) */
			else if (hentr > hexit)
				{
				if (hentr > hafter || (hentr == hafter && dentr > dafter))
					{
					hafter = hentr;
					dafter = dentr;
					lafter = itv;
					}
				}
			}

		/* Return the appropriate entry index */
		if (lbefore > 0) return lbefore;
		else             return lafter;
		}
	else
		{
		hafter  = -1;
		dafter  = 0.0;
		lafter  = -1;
		hbefore = hexit;
		dbefore = hexit;
		lbefore = -1;
		for (itv=0; itv<tsv; itv++)
			{
			if (sllist[itv].slstat == SLdone
					|| sllist[itv].slstat == SLskip) continue;
			hentr = sllist[itv].hentr;
			dentr = sllist[itv].dentr;
			if (hentr < 0) continue;

			/* Search for entry index on the same span */
			if (hentr == hexit && dentr > dexit)
				{
				if (hafter < 0 || hentr < hafter
						|| (hentr == hafter && dentr < dafter))
					{
					hafter = hentr;
					dafter = dentr;
					lafter = itv;
					}
				}

			/* Search for closest index after the exit index */
			else if (hentr > hexit)
				{
				if (hafter < 0 || hentr < hafter
						|| (hentr == hafter && dentr < dafter))
					{
					hafter = hentr;
					dafter = dentr;
					lafter = itv;
					}
				}

			/* Search for smallest index (forwards from start of hole) */
			else if (hentr < hexit)
				{
				if (hentr < hbefore || (hentr == hbefore && dentr < dbefore))
					{
					hbefore = hentr;
					dbefore = dentr;
					lbefore = itv;
					}
				}
			}

		/* Return the appropriate entry index */
		if (lafter > 0) return lafter;
		else            return lbefore;
		}
	}

static	int		next_hole_exit

	(
	int			tsv,
	SLLIST		*sllist,
	LOGICAL		insync,
	int			hentr,
	float		dentr
	)

	{
	int		itv, hexit, hafter, lafter, hbefore, lbefore;
	float	dexit, dafter, dbefore;

	/* Search for next exit index based on last entry index */
	if (insync)
		{
		hbefore = -1;
		dbefore = 0.0;
		lbefore = -1;
		hafter  = hentr;
		dafter  = dentr;
		lafter  = -1;
		for (itv=0; itv<tsv; itv++)
			{
			if (sllist[itv].slstat == SLdone
					|| sllist[itv].slstat == SLskip) continue;
			hexit = sllist[itv].hexit;
			dexit = sllist[itv].dexit;
			if (hexit < 0) continue;

			/* Search for exit index on the same span */
			if (hexit == hentr && dexit < dentr)
				{
				if (hexit > hbefore || (hexit == hbefore && dexit > dbefore))
					{
					hbefore = hexit;
					dbefore = dexit;
					lbefore = itv;
					}
				}

			/* Search for closest index before the entry index */
			else if (hexit < hentr)
				{
				if (hexit > hbefore || (hexit == hbefore && dexit > dbefore))
					{
					hbefore = hexit;
					dbefore = dexit;
					lbefore = itv;
					}
				}

			/* Search for largest index (backwards from end of hole) */
			else if (hexit > hentr)
				{
				if (hexit > hafter || (hexit == hafter && dexit > dafter))
					{
					hafter = hexit;
					dafter = dexit;
					lafter = itv;
					}
				}
			}

		/* Return the appropriate exit index */
		if (lbefore > 0) return lbefore;
		else             return lafter;
		}
	else
		{
		hafter  = -1;
		dafter  = 0.0;
		lafter  = -1;
		hbefore = hentr;
		dbefore = hentr;
		lbefore = -1;
		for (itv=0; itv<tsv; itv++)
			{
			if (sllist[itv].slstat == SLdone
					|| sllist[itv].slstat == SLskip) continue;
			hexit = sllist[itv].hexit;
			dexit = sllist[itv].dexit;
			if (hexit < 0) continue;

			/* Search for exit index on the same span */
			if (hexit == hentr && dexit > dentr)
				{
				if (hafter < 0 || hexit < hafter
						|| (hexit == hafter && dexit < dafter))
					{
					hafter = hexit;
					dafter = dexit;
					lafter = itv;
					}
				}

			/* Search for closest index after the entry index */
			else if (hexit > hentr)
				{
				if (hafter < 0 || hexit < hafter
						|| (hexit == hafter && dexit < dafter))
					{
					hafter = hexit;
					dafter = dexit;
					lafter = itv;
					}
				}

			/* Search for smallest index (forwards from start of hole) */
			else if (hexit < hentr)
				{
				if (hexit < hbefore || (hexit == hbefore && dexit < dbefore))
					{
					hbefore = hexit;
					dbefore = dexit;
					lbefore = itv;
					}
				}
			}

		/* Return the appropriate exit index */
		if (lafter > 0) return lafter;
		else            return lbefore;
		}
	}

/***********************************************************************
*                                                                      *
*     c l i p _ l i n e _ b y _ a r e a _ h o l e s                    *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Clip a line into segments based on area holes
 *
 * @param[in]	line	line.
 * @param[in]	area	area containing holes.
 * @param[out]	lsegs	line segments not within area holes.
 * @return	number of line segments not within area holes.
 ***********************************************************************/
int		clip_line_by_area_holes

	(
	LINE	line,		/* line */
	AREA	area,		/* area containing holes */
	LINE	**lsegs		/* line segments not within area holes */
	)

	{
	LOGICAL	closed, start, inside, intrsct, between;
	int		nseg, tnseg, nh, ns, np;
	LINE	hole, seg, segl, *segs, *tsegs;
	POINT	spos, epos, xcross;
	float	xdist, sdist2;
	double	ddx, ddy, dang;

	/* Setup default return parameters */
	nseg = 0;
	segs = NullLineList;
	if (NotNull(lsegs)) *lsegs = NullLineList;
	if (IsNull(line))        return nseg;
	if (line->numpts < 2)    return nseg;
	if (IsNull(area))        return nseg;
	if (IsNull(area->bound)) return nseg;

	/* Set initial line parameters */
	nseg = 1;
	segs = INITMEM(LINE, nseg);
	segs[0] = copy_line(line);
	tnseg = 0;
	tsegs = NullLineList;
	closed = line_closed(line);
	start  = TRUE;

	/* Check for intersections with area holes */
	for (nh=0; nh<area->bound->numhole; nh++)
		{
		hole = area->bound->holes[nh];
		if (IsNull(hole))     continue;
		if (hole->numpts < 3) continue;

		/* Check each line segment for intersection with hole */
		for (ns=0; ns<nseg; ns++)
			{
			seg = segs[ns];

			/* Check if first point of line segment is inside hole */
			copy_point(spos, seg->points[0]);
			line_test_point(hole, spos, NullFloat, NullPoint,
								NullInt, &inside, NullChar);

			/* Check if first point of first segment of closed line is inside */
			if (closed && start && ns == 0 && inside) start = FALSE;

			/* If first point is outside hole, begin a line segment */
			if (!inside)
				{
				tnseg++;
				tsegs = GETMEM(tsegs, LINE, tnseg);
				tsegs[tnseg-1] = create_line();
				add_point_to_line(tsegs[tnseg-1], spos);
				}

			/* Set end point to next point on span */
			np = 1;
			copy_point(epos, seg->points[np]);

			/* Check for intersections on each span */
			while ( np < seg->numpts)
				{

				/* If the points coincide, try the next point */
				if (spos[X] == epos[X] && spos[Y] == epos[Y])
					{
					np++;
					copy_point(epos, seg->points[np]);
					continue;
					}

				/* Look for a crossing between these points */
				intrsct = line_sight(hole, spos, epos, FALSE, &xdist,
										NullFloat, xcross, NullInt, &between);

				/* The line_sight() function allows a certain tolerance in   */
				/*  determining intersections to account for round off error */
				/*  ... so check for potential problem cases below           */

				/* An intersection that occurs just after the end of a span  */
				/*  may be mistakenly identified as occurring in the current */
				/*  span ... so ensure xdist is not greater than the length  */
				/*  of current span                                          */
				/* (Note that since we always check in a forward direction,  */
				/*  we can never find an intersection before start of span!) */
				if (intrsct && between && xdist > point_dist(spos, epos))
					{
					between = FALSE;

#					ifdef DEBUG_DIV_HOLES
					pr_diag("clip_line_by_area_holes",
						"Beyond end of span crossing ... xdist/dist: %.4f/%.4f\n",
						xdist, point_dist(spos, epos));
#					endif /* DEBUG_DIV_HOLES */
					}

				/* Found an intersection point */
				if (intrsct && between)
					{

					/* If we started inside the hole, intersection point */
					/*  will be the first point of the next line segment */
					if (inside)
						{
						tnseg++;
						tsegs = GETMEM(tsegs, LINE, tnseg);
						tsegs[tnseg-1] = create_line();
						add_point_to_line(tsegs[tnseg-1], xcross);
						inside = FALSE;
						}

					/* If we started outside the hole, intersection point */
					/*  will be the last point of this line segment       */
					else
						{
						add_point_to_line(tsegs[tnseg-1], xcross);
						inside = TRUE;
						}

					/* Reset the start point to the next span if close to end */
					sdist2 = point_dist2(xcross, epos);
					if (sdist2 < EndTol*EndTol)
						{
						copy_point(spos, epos);
						np++;
						copy_point(epos, seg->points[np]);
						}

					/* Reset the start point to just beyond the intersection */
					else
						{
						ddx  = (double) (epos[X] - xcross[X]);
						ddy  = (double) (epos[Y] - xcross[Y]);
						dang = atan2(ddy, ddx);
						spos[X] = xcross[X] + (float) (SegTol * cos(dang));
						spos[Y] = xcross[Y] + (float) (SegTol * sin(dang));
						}
					}

				/* No intersection point */
				else
					{

					/* If we started outside the hole,         */
					/*  add the end point to this line segment */
					if (!inside) add_point_to_line(tsegs[tnseg-1], epos);

					/* Reset the start point and end point and keep looking */
					copy_point(spos, epos);
					np++;
					copy_point(epos, seg->points[np]);
					}
				}
			}

		/* Reset line segment parameters */
		for (ns=0; ns<nseg; ns++) destroy_line(segs[ns]);
		FREEMEM(segs);
		nseg = tnseg;
		segs = tsegs;
		tnseg = 0;
		tsegs = NullLineList;
		}

	/* For closed lines that start outside all holes       */
	/*  connect the first and last segments (if necessary) */
	if (closed && start && nseg > 1)
		{

		/* Last segment becomes start of first segment */
		seg  = segs[0];
		segl = segs[nseg-1];
		segl = append_line(segl, seg);
		destroy_line(seg);
		seg  = segl;
		nseg--;
		}

	/* Return the line segments */
	if (NotNull(lsegs)) *lsegs = segs;
	return nseg;
	}

/***********************************************************************
*                                                                      *
*      p r e p _ a r e a _ b o u n d _ h o l e s                       *
*                                                                      *
***********************************************************************/

static	void	reset_hole_span(SEGMENT, int, POINT, int *, int *, POINT);

/***********************************************************************/
/**	Merge bound holes that cross over each other.
 *
 * @param[in]	bound	boundary with holes.
 * @return True if successful.
 ***********************************************************************/

LOGICAL	prep_area_bound_holes

	(
	BOUND	bound
	)

	{
	LINE	hole, xhole, thole, xline;
	BOUND	xbnd;
	int		ih, iho, iha, jh, isv, itv;
	int		*ihorder;
	LOGICAL	*ihdone;
	SITEM	*sortlist;
	LOGICAL	holecw, xholecw, insd, insync, fwd, boundcw;
	int		pspan, fspan, uspan, xspan;
	int		npts, ips, ipe, ipl;
	int		hentr, hlast, hexit, hspan, lentr, lexit;
	float	harea, dentr, dlast, dexit, dspan;
	SEGMENT	hseg, seg;

	LOGICAL	firstspan, in, rt, anymod, holemod, segexcl;
	POINT	pb, px, pentr, pexit, plast;

	int		tnhole, nhole, tsv, xsv, nvseg;
	LINE	*tholes, *holes;
	LOGICAL	*hinsd;
	SUBVIS	*tvlist, *xvlist;
	SLLIST	*sllist;
	SEGMENT	*vseg;

#	ifdef DEBUG_BND_HOLES
	STRING	sdirection, sforward = "->", sbackward = "<-";
#	endif /* DEBUG_BND_HOLES */

	if (!bound) return FALSE;
	if (bound->numhole < 1) return TRUE;

	/* Make a copy of the boundary to rebuild holes */
	xbnd = create_bound();
	define_bound_boundary(xbnd, copy_line(bound->boundary));

	/* Add holes to copy of the boundary */
	for (ih=0; ih<bound->numhole; ih++)
		{

		/* Do not copy missing holes */
		hole = bound->holes[ih];
		if (IsNull(hole)) continue;

		/* Do not copy very small holes */
		line_properties(hole, NullChar, &holecw, &harea, NullFloat);
		if (harea <= 1.0e-10) continue;

		/* Add the hole to the copy of the boundary */
		add_bound_hole(xbnd, copy_line(hole));
		}

	/* Messages if holes removed from set */
	if (xbnd->numhole < bound->numhole)
		{

#		ifdef DEBUG_BND_HOLES
		pr_diag("prep_area_bound_holes", "Removing %d small holes from boundary!\n",
			(bound->numhole - xbnd->numhole));
#		endif /* DEBUG_BND_HOLES */

		/* Replace the boundary for display if no holes left in set */
		if (xbnd->numhole < 1)
			{

#			ifdef DEBUG_BND_HOLES
			pr_diag("prep_area_bound_holes", "No holes left in boundary to display!\n");
#			endif /* DEBUG_BND_HOLES */

			/* Replace the boundary for display */
			define_bound(bound, xbnd->boundary, xbnd->numhole, xbnd->holes);
			return TRUE;
			}
		}

	/* Determine order for processing holes */
	ihorder = INITMEM(int,     xbnd->numhole);
	ihdone  = INITMEM(LOGICAL, xbnd->numhole);

	/* Add the first hole to the list */
	iho = 0;
	ihdone[iho]    = TRUE;
	ihorder[iho++] = 0;

	/* Initialize checking for remaining holes */
	for (ih=1; ih<xbnd->numhole; ih++) ihdone[ih] = FALSE;

	/* Then add holes that intersect holes in list (recursively) */
	while (iho < xbnd->numhole)
		{
		do
			{
			for (iha=1, ih=1; ih<xbnd->numhole; ih++)
				{
				if (!ihdone[ih])
					{
					hole = xbnd->holes[ih];
					for (jh=0; jh<iho; jh++)
						{
						xhole = xbnd->holes[ihorder[jh]];
						if (find_line_crossing(hole, xhole, 0, hole->points[0],
												NullPoint, NullInt, NullInt,
												NullLogical))
							{
							ihdone[ih]     = TRUE;
							ihorder[iho++] = ih;
							iha++;
							break;
							}
						}
					}
				}
			}
			while (iha > 1);

			/* Add the next non-intersecting hole to the list */
			for (ih=1; ih<xbnd->numhole; ih++)
				{
				if (!ihdone[ih])
					{
					hole = xbnd->holes[ih];
					ihdone[ih]     = TRUE;
					ihorder[iho++] = ih;
					break;
					}
				}
		}

	/* Initialize hole list from first bound hole */
	ih    = 0;
	nhole = 1;
	holes = INITMEM(LINE,    nhole);
	hinsd = INITMEM(LOGICAL, nhole);
	holes[nhole-1] = copy_line(xbnd->holes[ihorder[ih++]]);
	hinsd[nhole-1] = FALSE;

	/* Initialize all connecting points */
	copy_point(pentr, ZeroPoint);
	copy_point(pexit, ZeroPoint);
	copy_point(plast, ZeroPoint);

	/* Merge successive boundary holes with intersecting hole list segments */
	for (; ih<xbnd->numhole; ih++)
		{

		/* Initialize next hole properties */
		xhole = xbnd->holes[ihorder[ih]];
		line_properties(xhole, NullChar, &xholecw, NullFloat, NullFloat);

#		ifdef DEBUG_BND_HOLES
		if (xholecw)
			pr_diag("prep_area_bound_holes",
			"Intersections with Clockwise hole: [%d] - %d points\n",
			ihorder[ih], xhole->numpts);
		else
			pr_diag("prep_area_bound_holes",
			"Intersections with CounterClockwise hole: [%d] - %d points\n",
			ihorder[ih], xhole->numpts);
#		endif /* DEBUG_BND_HOLES */

		/* Initialize temporary hole list */
		tnhole = 0;
		tholes = NullLineList;

#		ifdef DEBUG_BND_HOLES
		pr_diag("prep_area_bound_holes",
			"Processing %d working holes\n", nhole);
#		endif /* DEBUG_BND_HOLES */

		/* Loop through current hole list */
		anymod = FALSE;
		for (iho=0; iho<nhole; iho++)
			{

			/* Initialize working hole properties */
			hole = holes[iho];
			insd = hinsd[iho];
			line_properties(hole, NullChar, &holecw, NullFloat, NullFloat);

			/* Create a segment from the working hole */
			hseg = create_segment();
			define_segment(hseg, hole, FALSE, 0, hole->numpts-1, TRUE);
			segexcl = seg_exclusive(hseg);

#			ifdef DEBUG_BND_HOLES
			if (holecw)
				pr_diag("prep_area_bound_holes",
					"Clockwise working hole: [%d] - %d points\n",
					iho, hole->numpts);
			else
				pr_diag("prep_area_bound_holes",
					"CounterClockwise working hole: [%d] - %d points\n",
					iho, hole->numpts);
#			endif /* DEBUG_BND_HOLES */

			/* Are the hole and working hole oriented in the same direction? */
			/* Save portions of boundary hole outside each hole segment list */
			insync = (holecw && xholecw) || (Not(holecw) && Not(xholecw));

			/* Reverse orientation check if "hole" is inside another hole   */
			/*  ... since the "inside hole" will be part of the area!       */
			/* Save portions of boundary hole inside each hole segment list */
			if (insd) insync = Not(insync);

			/* Initialize segment lists */
			xsv    = 0;
			xvlist = NullSubVisList;

			/* Initialize temporary segment lists */
			tsv    = 0;
			tvlist = NullSubVisList;
			sllist = NullPtr(SLLIST *);
			nvseg  = 0;
			vseg   = NullSegmentList;

			/* Set starting position on working hole segment */
			pspan = hseg->ips;
			fspan = pspan;
			uspan = pspan;
			copy_point(pb, hseg->line->points[pspan]);

			/* Create segments for each hole intersection */
			hentr = -1;
			hlast = -1;
			hexit = -1;
			hspan = -1;
			dentr = 0.0;
			dlast = 0.0;
			dexit = 0.0;
			dspan = 0.0;
			firstspan = TRUE;
			holemod   = FALSE;
			while (segment_crossing(hseg, xhole, pspan, pb, px, &xspan, &hspan,
																		&rt))
				{

				/* Check for special case intersection in first span of hole */
				if (firstspan && xspan != fspan) firstspan = FALSE;

				/* Determine distance along span to crossing point */
				dspan = point_dist(xhole->points[hspan], px);

				/* Have we stepped in or out of the hole? */
				in = (xholecw)? rt: Not(rt);

#				ifdef DEBUG_BND_HOLES
				if (rt)
					{
					pr_diag("prep_area_bound_holes",
						" Segment Crossing R ... pspan: %d  (uspan: %d)  pb: %.2f/%.2f\n",
						pspan, uspan, pb[X], pb[Y]);
					pr_diag("prep_area_bound_holes",
						"                         xspan/hspan: %d %d  px: %.2f/%.2f\n",
						xspan, hspan, px[X], px[Y]);
					}
				else
					{
					pr_diag("prep_area_bound_holes",
						" Segment Crossing L ... pspan: %d  (uspan: %d)  pb: %.2f/%.2f\n",
						pspan, uspan, pb[X], pb[Y]);
					pr_diag("prep_area_bound_holes",
						"                         xspan/hspan: %d %d  px: %.2f/%.2f\n",
						xspan, hspan, px[X], px[Y]);
					}
				if (in)
					pr_diag("prep_area_bound_holes", "  Stepped in ...\n" );
				else
					pr_diag("prep_area_bound_holes", "  Stepped out ...\n" );
				if (xspan == pspan)
					pr_diag("prep_area_bound_holes",
						" Segment Crossing on same span!\n");
#				endif /* DEBUG_BND_HOLES */

				/* Stepped in now - add all segments from previous position */
				/* (start or previous exit) up until this crossing */
				if (in)
					{

					/* Need to avoid duplicate crossover segments    */
					/*  ... except for crossover on very first span! */
					if (!firstspan && xspan == pspan)
						{
#						ifdef DEBUG_BND_HOLES
						pr_diag("prep_area_bound_holes",
							"  Multiple crossovers for span %d (%.2f/%.2f)\n",
							xspan, hseg->line->points[xspan][X],
							hseg->line->points[xspan][Y]);
#						endif /* DEBUG_BND_HOLES */
						}

					/* Copy crossover segment (duplicate exclusive segments) */
					else
						{
						nvseg++;
						vseg = GETMEM(vseg, SEGMENT, nvseg);
						vseg[nvseg-1] = copy_segment(hseg, segexcl);
						seg = vseg[nvseg-1];
						seg->ips = uspan;
						seg->ipe = xspan;

#						ifdef DEBUG_BND_HOLES
						pr_diag("prep_area_bound_holes",
							"  Add step in span %d -> %d (%.2f/%.2f to %.2f/%.2f)\n",
							seg->ips, seg->ipe,
							seg->line->points[seg->ips][X],
							seg->line->points[seg->ips][Y],
							seg->line->points[seg->ipe][X],
							seg->line->points[seg->ipe][Y]);
#						endif /* DEBUG_BND_HOLES */
						}

					/* Set entry span and hole intersection */
					hentr = hspan;
					dentr = dspan;
					copy_point(pentr, px);

#					ifdef DEBUG_BND_HOLES
					pr_diag("prep_area_bound_holes",
						"  Step in intersection pentr (%.2f/%.2f)\n",
						pentr[X], pentr[Y]);
#					endif /* DEBUG_BND_HOLES */

					/* Hole intersected with working hole */
					anymod  = TRUE;
					holemod = TRUE;

					/* Reset starting span on working hole */
					reset_hole_span(hseg, xspan, px, &pspan, &uspan, pb);
					continue;
					}

				/* Stepped out now - add intersection segments and */
				/*  segments from the appropriate side of the hole */
				else
					{

					/* If we start inside the hole then we have to save the */
					/*  intersection, reset the starting span, and add the  */
					/*  remaining part of the hole at the end               */
					if (!holemod)
						{

						/* Reset starting span on working hole */
						reset_hole_span(hseg, xspan, px, &pspan, &uspan, pb);

						/* Set last span and hole intersection */
						anymod  = TRUE;
						holemod = TRUE;
						hlast   = hspan;
						dlast   = dspan;
						copy_point(plast, px);

#						ifdef DEBUG_BND_HOLES
						pr_diag("prep_area_bound_holes",
							"  Last segment plast (%.2f/%.2f)\n",
							plast[X], plast[Y]);
#						endif /* DEBUG_BND_HOLES */

						continue;
						}

					/* Hole intersected with working hole */
					anymod  = TRUE;
					holemod = TRUE;

					/* Reset starting span on working hole */
					reset_hole_span(hseg, xspan, px, &pspan, &uspan, pb);

					/* Check if segments enter and exit hole on same span */
					if (hentr == hspan)
						{

						/* Add exclusive segment with step in intersection */
						xline = create_line();
						add_point_to_line(xline, pentr);
						nvseg++;
						vseg = GETMEM(vseg, SEGMENT, nvseg);
						vseg[nvseg-1] = create_segment();
						seg  = vseg[nvseg-1];
						define_segment(seg, xline, TRUE, 0, 0, TRUE);

#						ifdef DEBUG_BND_HOLES
						pr_diag("prep_area_bound_holes",
							"  Add step in intersection (same span) pentr (%.2f/%.2f)\n",
							pentr[X], pentr[Y]);
#						endif /* DEBUG_BND_HOLES */

						/* If enter/exit are not in proper order */
						/*  save hole information for later in process */
						if ((insync && (dentr > dspan))
								|| (Not(insync) && (dentr < dspan)))
							{

#							ifdef DEBUG_BND_HOLES
							if (insync)
								{
								pr_diag("prep_area_bound_holes",
									"  Out of order enter/exit on same span of hole - dentr<dspan: %.2f/%.2f\n",
									dentr, dspan);
								}
							else
								{
								pr_diag("prep_area_bound_holes",
								"  Out of order enter/exit on same span of hole - dentr>dspan: %.2f/%.2f\n",
								dentr, dspan);
								}
							pr_diag("prep_area_bound_holes",
								"  Saving %d segments in temporary list [%d]\n",
								nvseg, tsv);
#							endif /* DEBUG_BND_HOLES */

							/* Create a temporary segment list */
							tsv++;
							tvlist = GETMEM(tvlist, SUBVIS, tsv);
							sllist = GETMEM(sllist, SLLIST, tsv);
							tvlist[tsv-1] = create_subvis();
							tvlist[tsv-1]->numvis = nvseg;
							tvlist[tsv-1]->segvis = vseg;
							nvseg = 0;
							vseg  = NullSegmentList;

							/* Save hole entry and previous exit information */
							/*  for creating complete segment lists */
							sllist[tsv-1].hexit  = hexit;
							sllist[tsv-1].dexit  = dexit;
							sllist[tsv-1].hentr  = hentr;
							sllist[tsv-1].dentr  = dentr;
							sllist[tsv-1].slstat = SLinitial;
							sllist[tsv-1].lexit  = -1;
							sllist[tsv-1].lentr  = -1;

							/* Save current hole exit information for */
							/*  creating subsequent segment list */
							hexit = hspan;
							dexit = dspan;
							}

						/* Save current hole exit information for */
						/*  creating subsequent segment list */
						hexit = hspan;
						dexit = dspan;
						}

					/* Otherwise add step in intersection and */
					/*  save hole information for later in process */
					else
						{

						/* Set hole entry and exit parameters */
						if (insync)
							{
							npts = xhole->numpts;
							ips  = hentr + 1;
							ipe  = hspan;
							if (ipe < ips) ipe += npts;
							}
						else
							{
							npts = xhole->numpts;
							ips  = hspan + 1;
							ipe  = hentr;
							if (ipe < ips) ipe += npts;
							}

						/* Add exclusive segment with step in intersection */
						xline = create_line();
						add_point_to_line(xline, pentr);
						nvseg++;
						vseg = GETMEM(vseg, SEGMENT, nvseg);
						vseg[nvseg-1] = create_segment();
						seg  = vseg[nvseg-1];
						define_segment(seg, xline, TRUE, 0, 0, TRUE);

#						ifdef DEBUG_BND_HOLES
						pr_diag("prep_area_bound_holes",
							"  Add step in intersection pentr (%.2f/%.2f)\n",
							pentr[X], pentr[Y]);
#						endif /* DEBUG_BND_HOLES */

						/* Save hole information in temporary segment list */
						tsv++;
						tvlist = GETMEM(tvlist, SUBVIS, tsv);
						sllist = GETMEM(sllist, SLLIST, tsv);
						tvlist[tsv-1] = create_subvis();
						tvlist[tsv-1]->numvis = nvseg;
						tvlist[tsv-1]->segvis = vseg;
						nvseg = 0;
						vseg  = NullSegmentList;

#						ifdef DEBUG_BND_HOLES
						pr_diag("prep_area_bound_holes",
							"  Hole span - hexit to hentr and hspan (%d to %d and %d)\n",
							hexit, hentr, hspan);
						pr_diag("prep_area_bound_holes",
							"  Saving %d segments in temporary list [%d]\n",
							tvlist[tsv-1]->numvis, tsv-1);
#						endif /* DEBUG_BND_HOLES */

						/* Save hole entry and previous exit information */
						/*  for creating complete segment lists */
						sllist[tsv-1].hexit  = hexit;
						sllist[tsv-1].dexit  = dexit;
						sllist[tsv-1].hentr  = hentr;
						sllist[tsv-1].dentr  = dentr;
						sllist[tsv-1].slstat = SLinitial;
						sllist[tsv-1].lexit  = -1;
						sllist[tsv-1].lentr  = -1;

						/* Check for hole enter and exit on same span */
						if (hentr == hexit || hentr == hspan)
							{

#							ifdef DEBUG_BND_HOLES
							pr_diag("prep_area_bound_holes",
								"  Step out intersection on same hole span! hentr hexit hspan: %d %d %d\n",
								hentr, hexit, hspan);
#							endif /* DEBUG_BND_HOLES */
							}

						/* Save current hole exit information for */
						/*  creating subsequent segment list */
						hexit = hspan;
						dexit = dspan;
						}

					/* Set hole intersection */
					copy_point(pexit, px);

					/* Add exclusive segment with step out intersection */
					xline = create_line();
					add_point_to_line(xline, pexit);
					nvseg++;
					vseg = GETMEM(vseg, SEGMENT, nvseg);
					vseg[nvseg-1] = create_segment();
					seg  = vseg[nvseg-1];
					define_segment(seg, xline, TRUE, 0, 0, TRUE);

#					ifdef DEBUG_BND_HOLES
					pr_diag("prep_area_bound_holes",
						"  Add step out intersection pexit (%.2f/%.2f)\n",
						pexit[X], pexit[Y]);
#					endif /* DEBUG_BND_HOLES */

					/* Reset hole enter parameter */
					hentr = -1;
					dentr = 0.0;
					}
				}

			/* If we finish inside the hole - add intersection segments */
			/*  and the remaining segment of the hole */
			if (holemod && hlast >= 0)
				{

				/* Check if segments enter and exit hole on same span */
				if (hentr == hlast)
					{

					/* Add exclusive segment with step in intersection */
					xline = create_line();
					add_point_to_line(xline, pentr);
					nvseg++;
					vseg = GETMEM(vseg, SEGMENT, nvseg);
					vseg[nvseg-1] = create_segment();
					seg  = vseg[nvseg-1];
					define_segment(seg, xline, TRUE, 0, 0, TRUE);

#					ifdef DEBUG_BND_HOLES
					pr_diag("prep_area_bound_holes",
						"  Add step in intersection (same span) (last) pentr (%.2f/%.2f)\n",
						pentr[X], pentr[Y]);
#					endif /* DEBUG_BND_HOLES */

					/* If enter/exit are not in proper order */
					/*  save hole information for later in process */
					if ((insync && (dentr > dlast))
							|| (Not(insync) && (dentr < dlast)))
						{

#						ifdef DEBUG_BND_HOLES
						if (insync)
							{
							pr_diag("prep_area_bound_holes",
								"  Out of order enter/exit on same span of hole - dentr<dlast: %.2f/%.2f\n",
								dentr, dlast);
							}
						else
							{
							pr_diag("prep_area_bound_holes",
								"  Out of order enter/exit on same span of hole - dentr>dlast: %.2f/%.2f\n",
								dentr, dlast);
							}
						pr_diag("prep_area_bound_holes",
							"  Saving %d segments in temporary list [%d]\n",
							nvseg, tsv);
#						endif /* DEBUG_BND_HOLES */

						/* Create a temporary segment list */
						tsv++;
						tvlist = GETMEM(tvlist, SUBVIS, tsv);
						sllist = GETMEM(sllist, SLLIST, tsv);
						tvlist[tsv-1] = create_subvis();
						tvlist[tsv-1]->numvis = nvseg;
						tvlist[tsv-1]->segvis = vseg;
						nvseg = 0;
						vseg  = NullSegmentList;

						/* Save hole entry and previous exit information */
						/*  for creating complete segment lists */
						sllist[tsv-1].hexit  = hexit;
						sllist[tsv-1].dexit  = dexit;
						sllist[tsv-1].hentr  = hentr;
						sllist[tsv-1].dentr  = dentr;
						sllist[tsv-1].slstat = SLinitial;
						sllist[tsv-1].lexit  = -1;
						sllist[tsv-1].lentr  = -1;

						/* Save current hole exit information for */
						/*  creating subsequent segment list */
						hexit = hlast;
						dexit = dlast;
						}
					}

				/* Otherwise add step in intersection and */
				/*  save hole information for later in process */
				else
					{

					/* Set hole entry and exit parameters */
					if (insync)
						{
						npts = xhole->numpts;
						ips  = hentr + 1;
						ipe  = hlast;
						if (ipe < ips) ipe += npts;
						}
					else
						{
						npts = xhole->numpts;
						ips  = hlast + 1;
						ipe  = hentr;
						if (ipe < ips) ipe += npts;
						}

					/* Add exclusive segment with step in intersection */
					xline = create_line();
					add_point_to_line(xline, pentr);
					nvseg++;
					vseg = GETMEM(vseg, SEGMENT, nvseg);
					vseg[nvseg-1] = create_segment();
					seg  = vseg[nvseg-1];
					define_segment(seg, xline, TRUE, 0, 0, TRUE);

#					ifdef DEBUG_BND_HOLES
					pr_diag("prep_area_bound_holes",
						"  Add step in intersection (last) pentr (%.2f/%.2f)\n",
						pentr[X], pentr[Y]);
#					endif /* DEBUG_BND_HOLES */

					/* Save hole information in temporary segment list */
					tsv++;
					tvlist = GETMEM(tvlist, SUBVIS, tsv);
					sllist = GETMEM(sllist, SLLIST, tsv);
					tvlist[tsv-1] = create_subvis();
					tvlist[tsv-1]->numvis = nvseg;
					tvlist[tsv-1]->segvis = vseg;
					nvseg = 0;
					vseg  = NullSegmentList;

#					ifdef DEBUG_BND_HOLES
					pr_diag("prep_area_bound_holes",
						"  Hole span (last) - hexit to hentr and hlast (%d to %d and %d)\n",
						hexit, hentr, hlast);
					pr_diag("prep_area_bound_holes",
						"  Saving %d segments in temporary list [%d]\n",
						tvlist[tsv-1]->numvis, tsv-1);
#					endif /* DEBUG_BND_HOLES */

					/* Save hole entry and previous exit information */
					/*  for creating complete segment lists */
					sllist[tsv-1].hexit  = hexit;
					sllist[tsv-1].dexit  = dexit;
					sllist[tsv-1].hentr  = hentr;
					sllist[tsv-1].dentr  = dentr;
					sllist[tsv-1].slstat = SLinitial;
					sllist[tsv-1].lexit  = -1;
					sllist[tsv-1].lentr  = -1;

					/* Save current hole exit information for */
					/*  creating subsequent segment list */
					hexit = hlast;
					dexit = dlast;
					}

				/* Add exclusive segment with step out intersection */
				xline = create_line();
				add_point_to_line(xline, plast);
				nvseg++;
				vseg = GETMEM(vseg, SEGMENT, nvseg);
				vseg[nvseg-1] = create_segment();
				seg  = vseg[nvseg-1];
				define_segment(seg, xline, TRUE, 0, 0, TRUE);

#				ifdef DEBUG_BND_HOLES
				pr_diag("prep_area_bound_holes",
					"  Add step out intersection (last) plast (%.2f/%.2f)\n",
					plast[X], plast[Y]);
#				endif /* DEBUG_BND_HOLES */

				/* Reset hole enter parameter */
				hentr = -1;
				dentr = 0.0;
				}

			/* No (further) crossings - add remainder of working hole segment */
			else if (holemod)
				{

				/* Copy segments (duplicate exclusive segments) */
				nvseg++;
				vseg = GETMEM(vseg, SEGMENT, nvseg);
				vseg[nvseg-1] = copy_segment(hseg, segexcl);
				seg  = vseg[nvseg-1];
				seg->ips = uspan;

#				ifdef DEBUG_BND_HOLES
				sdirection = (seg_forward(seg))? sforward: sbackward;
				pr_diag("prep_area_bound_holes",
					"  Add end span %d -> %d (%.2f/%.2f to %.2f/%.2f)\n",
					seg->ips, seg->ipe,
					seg->line->points[seg->ips][X],
					seg->line->points[seg->ips][Y],
					seg->line->points[seg->ipe][X],
					seg->line->points[seg->ipe][Y]);
#				endif /* DEBUG_BND_HOLES */
				}

			/* Save remaining temporary segment list (if required) */
			if (holemod && nvseg > 0)
				{
				tsv++;
				tvlist = GETMEM(tvlist, SUBVIS, tsv);
				sllist = GETMEM(sllist, SLLIST, tsv);
				tvlist[tsv-1] = create_subvis();
				tvlist[tsv-1]->numvis = nvseg;
				tvlist[tsv-1]->segvis = vseg;
				nvseg = 0;
				vseg  = NullSegmentList;

				if (hexit >= 0 || hentr >= 0)
					{

#					ifdef DEBUG_BND_HOLES
					pr_diag("prep_area_bound_holes",
						"  Hole span (last) - hexit to hentr (%d to %d)\n",
						hexit, hentr);
					pr_diag("prep_area_bound_holes",
						"  Saving %d segments in temporary list [%d]\n",
						tvlist[tsv-1]->numvis, tsv-1);
#					endif /* DEBUG_BND_HOLES */

					/* Save hole entry and previous exit information */
					/*  for creating complete segment lists */
					sllist[tsv-1].hexit  = hexit;
					sllist[tsv-1].dexit  = dexit;
					sllist[tsv-1].hentr  = hentr;
					sllist[tsv-1].dentr  = dentr;
					sllist[tsv-1].slstat = SLinitial;
					sllist[tsv-1].lexit  = -1;
					sllist[tsv-1].lentr  = -1;
					}
				else
					{

#					ifdef DEBUG_BND_HOLES
					pr_diag("prep_area_bound_holes",
						"  Saving %d segments in temporary list [%d]\n",
						tvlist[tsv-1]->numvis, tsv-1);
#					endif /* DEBUG_BND_HOLES */

					sllist[tsv-1].hexit  = hexit;
					sllist[tsv-1].dexit  = dexit;
					sllist[tsv-1].hentr  = hentr;
					sllist[tsv-1].dentr  = dentr;
					sllist[tsv-1].slstat = SLextra;
					sllist[tsv-1].lexit  = -1;
					sllist[tsv-1].lentr  = -1;
					}
				}

			/* Build temporary holes from temporary lists */
			if (holemod)
				{

				/* Combine segment lists for intersecting holes */
				if (tsv > 1)
					{

#					ifdef DEBUG_BND_HOLES
					for (itv=0; itv<tsv; itv++)
						{
						pr_diag("prep_area_bound_holes",
							"  Info for: %d  exit: %d %.2f  entry: %d %.2f\n",
							itv, sllist[itv].hexit, sllist[itv].dexit,
							sllist[itv].hentr, sllist[itv].dentr);
						}
#					endif /* DEBUG_BND_HOLES */

					/* Combine segment lists until complete */
					hentr = -1;
					dentr = 0.0;
					hexit = -1;
					dexit = 0.0;
					while (TRUE)
						{

						/* Find the next hole entry index */
						lentr = next_hole_entry(tsv, sllist, !insync,
												hexit, dexit);
						if (lentr < 0 ) break;
						hentr = sllist[lentr].hentr;
						dentr = sllist[lentr].dentr;

						/* Find the next matching hole exit index */
						lexit = next_hole_exit(tsv, sllist, !insync,
												hentr, dentr);
						if (lexit < 0 ) break;
						hexit = sllist[lexit].hexit;
						dexit = sllist[lexit].dexit;

#						ifdef DEBUG_BND_HOLES
						pr_diag("prep_area_bound_holes",
							"  Matching hole exit (%d span %d) with entry (%d span %d)\n",
							hexit, lexit, hentr, lentr);
#						endif /* DEBUG_BND_HOLES */

						/* Hole exit and entry are in same list      */
						/*  but exit and entry are on the same span! */
						/*  ... so hole span not needed in list      */
						if (lentr == lexit && hentr == hexit)
							{

							/* Bail out if problem with entry/exit */
							if ((insync && (dentr > dexit))
									|| (Not(insync) && (dentr < dexit)))
								{
								pr_error("Holes",
									"Error with hole entry/exit in same list and on same span!\n");
								pr_error("Holes",
									" ... Try more points in line!\n");
								break;
								}

#							ifdef DEBUG_BND_HOLES
							pr_diag("prep_area_bound_holes",
								"  Hole span not needed - hexit/hentr (%d %d)\n",
								hexit, hentr);
#							endif /* DEBUG_BND_HOLES */

							/* Segment list should be complete */
							sllist[lentr].slstat = SLdone;
							}

						/* Hole exit and entry are in same list */
						/*  ... so just add hole span to list   */
						else if (lentr == lexit)
							{

#							ifdef DEBUG_BND_HOLES
							pr_diag("prep_area_bound_holes",
								"  Hole span - hexit to hentr (%d to %d)\n",
								hexit, hentr);
#							endif /* DEBUG_BND_HOLES */

							/* Add hole span to the list */
							nvseg = tvlist[lentr]->numvis;
							vseg  = tvlist[lentr]->segvis;
							nvseg++;
							vseg  = GETMEM(vseg, SEGMENT, nvseg);
							vseg[nvseg-1] = create_segment();
							seg   = vseg[nvseg-1];

							if (insync) define_segment(seg, xhole, FALSE,
														hentr+1, hexit, TRUE);
							else        define_segment(seg, xhole, FALSE,
														hexit+1, hentr, FALSE);

#							ifdef DEBUG_BND_HOLES
							sdirection = (seg_forward(seg))? sforward: sbackward;
							pr_diag("prep_area_bound_holes",
								"  Add hole [%d] span %d %s %d (%.2f/%.2f to %.2f/%.2f)\n",
								ihorder[ih], seg->ips, sdirection, seg->ipe,
								seg->line->points[seg->ips][X],
								seg->line->points[seg->ips][Y],
								seg->line->points[seg->ipe][X],
								seg->line->points[seg->ipe][Y]);
							pr_diag("prep_area_bound_holes",
								"  Add hole span to temporary list [%d]\n",
								lentr);
#							endif /* DEBUG_BND_HOLES */

							/* Reset segment list */
							tvlist[lentr]->numvis = nvseg;
							tvlist[lentr]->segvis = vseg;
							nvseg = 0;
							vseg  = NullSegmentList;

							/* Segment list should be complete */
							sllist[lentr].slstat = SLdone;
							}

						/* Hole exit and entry are from different lists */
						/*  ... so add the hole span (if required)      */
						/*       and segments from the other list       */
						else
							{

							/* Reset entry for partially completed holes */
							if (sllist[lentr].slstat == SLpartial)
								{

#								ifdef DEBUG_BND_HOLES
								pr_diag("prep_area_bound_holes",
									"  Resetting hole entry (%d) to (%d)\n",
									lentr, sllist[lentr].lentr);
#								endif /* DEBUG_BND_HOLES */

								/* Bail out if problem with entry reset */
								if (sllist[lentr].lentr == lentr)
									{
									pr_error("Holes",
										"Error with hole entry reset!\n");
									pr_error("Holes",
										" ... Try more points in line!\n");
									break;
									}

								/* Entry segment list now complete */
								sllist[lentr].slstat = SLskip;

								/* Reset entry segment list */
								lentr = sllist[lentr].lentr;
								}

							/* Hole exit and reset entry are in same list */
							/*  but exit and entry are on the same span!  */
							/*  ... so hole span not needed in list       */
							if (lentr == lexit && hentr == hexit)
								{

								/* Bail out if problem with entry/exit */
								if ((insync && (dentr > dexit))
										|| (Not(insync) && (dentr < dexit)))
									{
									pr_error("Holes",
										"Error with hole entry/exit in same (reset) list and on same span!\n");
									pr_error("Holes",
										" ... Try more points in line!\n");
									break;
									}

#								ifdef DEBUG_BND_HOLES
								pr_diag("prep_area_bound_holes",
									"  Hole span not needed - hexit/hentr (%d %d)\n",
									hexit, hentr);
#								endif /* DEBUG_BND_HOLES */

								/* Segment list should be complete */
								sllist[lentr].slstat = SLdone;
								}

							/* Hole exit and reset entry are in same list */
							/*  ... so just add hole span to list */
							else if (lentr == lexit)

								{

#								ifdef DEBUG_BND_HOLES
								pr_diag("prep_area_bound_holes",
									"  Hole span - hexit to hentr (%d to %d)\n",
									hexit, hentr);
#								endif /* DEBUG_BND_HOLES */

								/* Add hole span to the list */
								nvseg = tvlist[lentr]->numvis;
								vseg  = tvlist[lentr]->segvis;
								nvseg++;
								vseg  = GETMEM(vseg, SEGMENT, nvseg);
								vseg[nvseg-1] = create_segment();
								seg   = vseg[nvseg-1];

								if (insync) define_segment(seg, xhole, FALSE,
														hentr+1, hexit, TRUE);
								else        define_segment(seg, xhole, FALSE,
														hexit+1, hentr, FALSE);

#								ifdef DEBUG_BND_HOLES
								sdirection = (seg_forward(seg))? sforward: sbackward;
								pr_diag("prep_area_bound_holes",
									"  Add hole [%d] span %d %s %d (%.2f/%.2f to %.2f/%.2f)\n",
									ihorder[ih], seg->ips, sdirection, seg->ipe,
									seg->line->points[seg->ips][X],
									seg->line->points[seg->ips][Y],
									seg->line->points[seg->ipe][X],
									seg->line->points[seg->ipe][Y]);
								pr_diag("prep_area_bound_holes",
									"  Add hole span to temporary list [%d]\n",
									lentr);
#								endif /* DEBUG_BND_HOLES */

								/* Reset segment list */
								tvlist[lentr]->numvis = nvseg;
								tvlist[lentr]->segvis = vseg;
								nvseg = 0;
								vseg  = NullSegmentList;

								/* Segment list should be complete */
								sllist[lentr].slstat = SLdone;
								}

							/* Hole exit and entry are from different lists */
							/*  but exit and entry are on the same span!    */
							/*  ... so just add second list                 */
							else if (hentr == hexit)
								{

								/* Bail out if problem with entry/exit */
								if ((insync && (dentr > dexit))
										|| (Not(insync) && (dentr < dexit)))
									{
									pr_error("Holes",
										"Error with hole entry/exit in different lists but on same span!\n");
									pr_error("Holes",
										" ... Try more points in line!\n");
									break;
									}

#								ifdef DEBUG_BND_HOLES
								pr_diag("prep_area_bound_holes",
									"  Hole span not needed - hexit/hentr (%d %d)\n",
									hexit, hentr);
								pr_diag("prep_area_bound_holes",
									"  Add %d segments from temporary list [%d] to [%d]\n",
									tvlist[lexit]->numvis, lexit, lentr);
#								endif /* DEBUG_BND_HOLES */

								/* Add remaining segments to the list */
								nvseg = tvlist[lentr]->numvis;
								vseg  = tvlist[lentr]->segvis;
								for (itv=0; itv<tvlist[lexit]->numvis; itv++)
									{
									nvseg++;
									vseg  = GETMEM(vseg, SEGMENT, nvseg);
									seg   = tvlist[lexit]->segvis[itv];
									vseg[nvseg-1] = seg;

#									ifdef DEBUG_BND_HOLES
									sdirection = (seg_forward(seg))? sforward: sbackward;
									pr_diag("prep_area_bound_holes",
										"  Add segment [%x] %d %s %d (%.2f/%.2f to %.2f/%.2f)\n",
										seg, seg->ips, sdirection, seg->ipe,
										seg->line->points[seg->ips][X],
										seg->line->points[seg->ips][Y],
										seg->line->points[seg->ipe][X],
										seg->line->points[seg->ipe][Y]);
#									endif /* DEBUG_BND_HOLES */
									}

								/* Reset segment lists */
								tvlist[lentr]->numvis = nvseg;
								tvlist[lentr]->segvis = vseg;
								tvlist[lexit]->numvis = 0;
								tvlist[lexit]->segvis = NullSegmentList;
								nvseg = 0;
								vseg  = NullSegmentList;

								/* Combined segment list from beginning */
								/*  and end lists should now be complete */
								if (sllist[lentr].hexit < 0
										&& sllist[lexit].hentr < 0)
									{
									sllist[lentr].slstat = SLdone;
									sllist[lexit].slstat = SLskip;
									}

								/* Combined segment list not yet complete */
								/*  ... so reset parameters */
								else
									{
									sllist[lentr].slstat = SLpartial;
									sllist[lentr].lentr  = lentr;
									sllist[lentr].lexit  = lexit;
									sllist[lexit].slstat = SLpartial;
									sllist[lexit].lentr  = lentr;
									sllist[lexit].lexit  = lexit;
									}
								}

							/* Hole exit and entry are from different lists */
							/*  ... so add hole span and second list */
							else
								{

#								ifdef DEBUG_BND_HOLES
								pr_diag("prep_area_bound_holes",
									"  Hole span - hexit to hentr (%d to %d)\n",
									hexit, hentr);
#								endif /* DEBUG_BND_HOLES */

								/* Add hole span to the list */
								nvseg = tvlist[lentr]->numvis;
								vseg  = tvlist[lentr]->segvis;
								nvseg++;
								vseg  = GETMEM(vseg, SEGMENT, nvseg);
								vseg[nvseg-1] = create_segment();
								seg   = vseg[nvseg-1];

								if (insync) define_segment(seg, xhole, FALSE,
														hentr+1, hexit, TRUE);
								else        define_segment(seg, xhole, FALSE,
														hexit+1, hentr, FALSE);

#								ifdef DEBUG_BND_HOLES
								sdirection = (seg_forward(seg))? sforward: sbackward;
								pr_diag("prep_area_bound_holes",
									"  Add hole [%d] span %d %s %d (%.2f/%.2f to %.2f/%.2f)\n",
									ihorder[ih], seg->ips, sdirection, seg->ipe,
									seg->line->points[seg->ips][X],
									seg->line->points[seg->ips][Y],
									seg->line->points[seg->ipe][X],
									seg->line->points[seg->ipe][Y]);
								pr_diag("prep_area_bound_holes",
									"  Add hole span to temporary list [%d]\n",
									lentr);
								pr_diag("prep_area_bound_holes",
									"  Add %d segments from temporary list [%d] to [%d]\n",
									tvlist[lexit]->numvis, lexit, lentr);
#								endif /* DEBUG_BND_HOLES */

								/* Add remaining segments to the list */
								for (itv=0; itv<tvlist[lexit]->numvis; itv++)
									{
									nvseg++;
									vseg  = GETMEM(vseg, SEGMENT, nvseg);
									seg   = tvlist[lexit]->segvis[itv];
									vseg[nvseg-1] = seg;

#									ifdef DEBUG_BND_HOLES
									sdirection = (seg_forward(seg))? sforward: sbackward;
									pr_diag("prep_area_bound_holes",
										"  Add segment [%x] %d %s %d (%.2f/%.2f to %.2f/%.2f)\n",
										seg, seg->ips, sdirection, seg->ipe,
										seg->line->points[seg->ips][X],
										seg->line->points[seg->ips][Y],
										seg->line->points[seg->ipe][X],
										seg->line->points[seg->ipe][Y]);
#									endif /* DEBUG_BND_HOLES */
									}

								/* Reset segment lists */
								tvlist[lentr]->numvis = nvseg;
								tvlist[lentr]->segvis = vseg;
								tvlist[lexit]->numvis = 0;
								tvlist[lexit]->segvis = NullSegmentList;
								nvseg = 0;
								vseg  = NullSegmentList;

								/* Combined segment list from beginning */
								/*  and end lists should now be complete */
								if (sllist[lentr].hexit < 0
										&& sllist[lexit].hentr < 0)
									{
									sllist[lentr].slstat = SLdone;
									sllist[lexit].slstat = SLskip;
									}

								/* Combined segment list not yet complete */
								/*  ... so reset parameters */
								else
									{
									sllist[lentr].slstat = SLpartial;
									sllist[lentr].lentr  = lentr;
									sllist[lentr].lexit  = lexit;
									sllist[lexit].slstat = SLpartial;
									sllist[lexit].lentr  = lentr;
									sllist[lexit].lexit  = lexit;
									}
								}
							}
						}
					}

				/* Add temporary segment lists to full list */
				for (itv=0; itv<tsv; itv++)
					{

					/* Skip segment lists that have been combined with others */
					if (sllist[itv].slstat != SLdone
							&& sllist[itv].slstat != SLextra) continue;

					/* Save temporary segment list */
					xsv++;
					xvlist = GETMEM(xvlist, SUBVIS, xsv);
					xvlist[xsv-1] = tvlist[itv];
					tvlist[itv]   = NullSubVis;
					}
				FREEMEM(tvlist);
				FREEMEM(sllist);
				tsv = 0;

				/* Build holes from the segment lists */
				if (xsv > 0)
					{

					/* Add each hole to the temporary hole list */
					for (isv=0; isv<xsv; isv++)
						{
						tnhole++;
						tholes = GETMEM(tholes, LINE, tnhole);
						thole  = create_line();
						tholes[tnhole-1] = thole;

#						ifdef DEBUG_BND_HOLES
						pr_diag("prep_area_bound_holes",
							"  Build hole: %d from %d segments\n",
							tnhole-1, xvlist[isv]->numvis);
#						endif /* DEBUG_BND_HOLES */

						/* Build a hole from the visible segments */
						for (itv=0; itv<xvlist[isv]->numvis; itv++)
							{
							seg  = xvlist[isv]->segvis[itv];
							fwd  = (LOGICAL) seg_forward(seg);
							ips  = seg->ips;
							ipe  = seg->ipe;
							ipl  = seg->line->numpts - 1;

#							ifdef DEBUG_BND_HOLES
							sdirection = (fwd)? sforward: sbackward;
							pr_diag("prep_area_bound_holes",
								"  Segment [%x] %d %s %d (%.2f/%.2f to %.2f/%.2f)\n",
								seg, ips, sdirection, ipe,
								seg->line->points[ips][X],
								seg->line->points[ips][Y],
								seg->line->points[ipe][X],
								seg->line->points[ipe][Y]);
#							endif /* DEBUG_BND_HOLES */

							if (fwd)
								{
								if (ips <= ipe)
									{
									thole = append_line_pdir(thole, seg->line,
																ips, ipe, fwd);
									}
								else
									{
									thole = append_line_pdir(thole, seg->line,
																ips, ipl, fwd);
									thole = append_line_pdir(thole, seg->line,
																0, ipe, fwd);
									}
								}
							else
								{
								if (ips <= ipe)
									{
									thole = append_line_pdir(thole, seg->line,
																ips, ipe, fwd);
									}
								else
									{
									thole = append_line_pdir(thole, seg->line,
																0, ipe, fwd);
									thole = append_line_pdir(thole, seg->line,
																ips, ipl, fwd);
									}
								}
							}
						add_point_to_line(thole, thole->points[0]);

#						ifdef DEBUG_BND_HOLES
						pr_diag("prep_area_bound_holes",
							"    Hole: [%x] with %d points\n",
							thole, thole->numpts);
#						endif /* DEBUG_BND_HOLES */

						(void) destroy_subvis(xvlist[isv]);
						}
					FREEMEM(xvlist);
					xsv = 0;
					}
				}

			/* Hole did not intersect working hole             */
			/*  ... check if working hole is completely inside */
			else
				{

#				ifdef DEBUG_BND_HOLES
				pr_diag("prep_area_bound_holes", "No intersections found!\n");
#				endif /* DEBUG_BND_HOLES */

				/* Skip working hole if completely inside */
				line_test_point(xhole, hole->points[0],
								NULL, NULL, NULL, &in, NULL);
				if (in)
					{

#					ifdef DEBUG_BND_HOLES
					pr_diag("prep_area_bound_holes",
						"  Skip inside working hole with no intersections!\n");
#					endif /* DEBUG_BND_HOLES */
					}

				/* Make copy of working hole if not inside */
				else
					{

#					ifdef DEBUG_BND_HOLES
					pr_diag("prep_area_bound_holes",
						"  Add working hole with no intersection: %d\n",
						tnhole);
#					endif /* DEBUG_BND_HOLES */

					tnhole++;
					tholes = GETMEM(tholes, LINE, tnhole);
					tholes[tnhole-1] = copy_line(hole);
					}
				}
			}

		/* Current boundary hole did not intersect any holes in list */
		/*  ... so add it to the list                                */
		if (!anymod)
			{

#			ifdef DEBUG_BND_HOLES
			pr_diag("prep_area_bound_holes",
				"  Add boundary hole with no intersection: %d\n", tnhole);
#			endif /* DEBUG_BND_HOLES */

			tnhole++;
			tholes = GETMEM(tholes, LINE, tnhole);
			tholes[tnhole-1] = copy_line(xhole);
			}

		/* Sort the holes by size ... largest to smallest */
		sortlist = INITMEM(SITEM, tnhole);
		for (iho=0; iho<tnhole; iho++)
			{
			hole = tholes[iho];
			line_properties(hole, NullChar, NullChar, &harea, NullFloat);
			sortlist[iho].item  = (ITEM) hole;
			sortlist[iho].value = harea;
			}

#		ifdef DEBUG_BND_HOLES
		pr_diag("prep_area_bound_holes", "List of %d holes\n", tnhole);
		for (iho=0; iho<tnhole; iho++)
			{
			hole  = (LINE) sortlist[iho].item;
			harea = sortlist[iho].value;
			pr_diag("prep_area_bound_holes",
				"  Hole: [%x] with %d points and size: %.2f\n",
				hole, hole->numpts, harea);
			}
#		endif /* DEBUG_BND_HOLES */

		/* Sort the holes */
		if (tnhole > 0)
			qsort((POINTER)sortlist, (size_t) tnhole, sizeof(SITEM), itemrev);

#		ifdef DEBUG_BND_HOLES
		pr_diag("prep_area_bound_holes", "Sorted list of %d holes\n", tnhole);
#		endif /* DEBUG_BND_HOLES */

		/* Reset the current hole list                    */
		/*  ... and identify any holes inside other holes */
		for (iho=0; iho<nhole; iho++) destroy_line(holes[iho]);
		nhole = tnhole;
		holes = GETMEM(holes, LINE,    nhole);
		hinsd = GETMEM(hinsd, LOGICAL, nhole);
		for (iho=0; iho<nhole; iho++)
			{
			hole  = (LINE) sortlist[iho].item;
			harea = sortlist[iho].value;
			holes[iho] = hole;
			hinsd[iho] = FALSE;

			/* A "hole" inside another hole will be part of the area! */
			for (iha=iho-1; iha>=0; iha--)
				{
				xhole = holes[iha];
				line_test_point(xhole, hole->points[0],
								NULL, NULL, NULL, &in, NULL);
				if (in)
					{
					hinsd[iho] = TRUE;
					break;
					}
				}

#			ifdef DEBUG_BND_HOLES
			if (hinsd[iho])
				pr_diag("prep_area_bound_holes",
					"  Hole: [%x] with %d points and size: %.2f - Inside!\n",
					hole, hole->numpts, harea);
			else
				pr_diag("prep_area_bound_holes",
					"  Hole: [%x] with %d points and size: %.2f\n",
					hole, hole->numpts, harea);
#			endif /* DEBUG_BND_HOLES */

			}
		}

	/* Redefine the original boundary with the new holes */
	empty_bound(bound);
	define_bound_boundary(bound, copy_line(xbnd->boundary));

	/* Loop through sorted list of holes */
	/* Check each subsequent hole for inside holes already in boundary    */
	/*  ... if inside a hole, orientation is reverse of that hole         */
	/*  ... if not inside, orientation is reverse of boundary orientation */
	line_properties(bound->boundary, NullChar, &boundcw, NullFloat, NullFloat);
	for (ih=0; ih<nhole; ih++)
		{
		hole = holes[ih];
		line_properties(hole, NullChar, &holecw, NullFloat, NullFloat);

		/* Compare orientation to boundary hole if inside */
		for (iho=bound->numhole-1; iho>=0; iho--)
			{
			xhole = bound->holes[iho];
			line_test_point(xhole, hole->points[0],
							NULL, NULL, NULL, &in, NULL);
			if (in)
				{
				line_properties(xhole, NullChar, &xholecw,
								NullFloat, NullFloat);
				if ((holecw && xholecw) || (Not(holecw) && Not(xholecw)))
					reverse_line(hole);
				break;
				}
			}

		/* Compare orientation to boundary if not inside any hole */
		if (iho < 0)
			{
			if ((holecw && boundcw) || (Not(holecw) && Not(boundcw)))
				reverse_line(hole);
			}

		/* Add the hole to the boundary */
		add_bound_hole(bound, hole);
		}

	/* Destroy the working boundary */
	destroy_bound(xbnd);

	/* Destroy hole order parameters */
	FREEMEM(ihdone);
	FREEMEM(ihorder);

	/* Return when finished */
	return TRUE;
	}

/***********************************************************************/

static	void	reset_hole_span

	(
	SEGMENT		hseg,
	int			xspan,
	POINT		px,
	int			*pspan,
	int			*uspan,
	POINT		pb
	)

	{
	int		ispan, tspan;
	float	*npos, sdist2, edist2;
	double	ddx, ddy, dang;
	POINT	xpos;
	LOGICAL	clast;

	/* Initialize return values */
	if (pspan) *pspan = -1;
	if (uspan) *uspan = -1;
	if (pb)    copy_point(pb, ZeroPoint);

	/* Set current span */
	ispan = xspan;

	/* Set end position for next intersection */
	clast = FALSE;

	/* Set next position on current span */
	tspan = ispan + 1;

	/* Have we left current span? */
	if (tspan > hseg->ipe)
		{
		tspan = hseg->ipe;
		clast = TRUE;
		}
	npos = hseg->line->points[tspan];

	/* Check if intersection is very close to end position on hole */
	edist2 = point_dist2(px, npos);
	if (edist2 < EndTol*EndTol)
		{

		/* Reset end position for intersection on current segment */
		clast = FALSE;

		/* Set next position on current span */
		tspan++;

		/* Have we left current span? */
		if (tspan > hseg->ipe)
			{
			tspan = hseg->ipe;
			clast = TRUE;
			}
		npos = hseg->line->points[tspan];

#		ifdef DEBUG_BND_HOLES
		pr_diag("reset_hole_span",
			"    Coincident intersection ... edist: %.4f\n", sqrt(edist2));
#		endif /* DEBUG_BND_HOLES */
		}

	/* Reset start position based on intersection and next position */
	sdist2 = point_dist2(px, npos);

	/* Start position set to start of next span */
	if (sdist2 < SegTol*SegTol)
		{

		if (pspan) *pspan = tspan;
		if (uspan) *uspan = tspan;
		if (pb)    copy_point(pb, npos);

#		ifdef DEBUG_BND_HOLES
		if (clast)
			pr_diag("reset_hole_span",
				"    Resetting (last) pspan: %d to (%d)\n", xspan, tspan);
		else
			pr_diag("reset_hole_span",
				"    Resetting pspan: %d %d to %d %d\n", xspan, tspan);
#		endif /* DEBUG_BND_HOLES */
		}

	/* Start position set to same span */
	else
		{
		ddx  = (double) (npos[X] - px[X]);
		ddy  = (double) (npos[Y] - px[Y]);
		dang = atan2(ddy, ddx);
		xpos[X] = px[X] + (float) (SegTol * cos(dang));
		xpos[Y] = px[Y] + (float) (SegTol * sin(dang));
		if (pspan) *pspan = xspan;
		if (uspan) *uspan = tspan;
		if (pb)    copy_point(pb, xpos);

#		ifdef DEBUG_BND_HOLES
		if (clast)
			pr_diag("reset_hole_span",
				"    No reset of (last) pspan: %d (Next: %d)\n", xspan, tspan);
		else
			pr_diag("reset_hole_span",
				"    No reset of pspan: %d (Next: %d)\n", xspan, tspan);
#		endif /* DEBUG_BND_HOLES */
		}
	}
