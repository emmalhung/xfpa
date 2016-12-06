/***********************************************************************
*                                                                      *
*   s u p p o r t _ l i n e . c                                        *
*                                                                      *
*   Obsolescent functions to handle the LINE object.                   *
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
*     l i n e _ c r o s s i n g                                        *
*                                                                      *
*      Obsolescent function!                                           *
*                                                                      *
*      Use equivalent find_line_crossing() function.                   *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** OBSOLETE!
 * Find the point at which two lines cross.
 * It is assumed that the lines cross only once, or not at all.
 *
 *	@param[in] 	line1	first line to be examined
 *	@param[in] 	line2	second line to be examined
 *	@param[out]	cross	point of intersection
 *	@param[out]	*seg1	segment on line1 where crossover occurs
 *	@param[out]	*seg2	segment on line2 where crossover occurs
 *  @return True if successful.
 *********************************************************************/

LOGICAL	line_crossing

	(
	LINE	line1,
	LINE	line2,
	POINT	cross,
	int		*seg1,
	int		*seg2
	)

	{
	int		seg, ip, ipr, ipl;
	LOGICAL	right;
	float	*p;

	static	LOGICAL	done = FALSE;
	if (!done)
		{
		done = TRUE;
		(void) printf("*** Obsolete function temoprarily supported:\n");
		(void) printf("Replace:\n");
		(void) printf("    LINE   line1;\n");
		(void) printf("    LINE   line2;\n");
		(void) printf("    POINT  cross;\n");
		(void) printf("    int    seg1;\n");
		(void) printf("    int    seg2;\n");
		(void) printf("    line_crossing(line1, line2, cross, &seg1, &seg2);\n");
		(void) printf("With:\n");
		(void) printf("    LINE   line1;\n");
		(void) printf("    LINE   line2;\n");
		(void) printf("    POINT  cross;\n");
		(void) printf("    int    seg1;\n");
		(void) printf("    int    seg2;\n");
		(void) printf("    if (line1 && line1->numpts > 0)\n");
		(void) printf("        find_line_crossing(line1, line2, 0, line1->points[0],\n");
		(void) printf("                            cross, &seg1, &seg2, NullLogical);\n");
		(void) printf("*** End\n");
		}

	/* Set up reasonable return values */
	ipr = -1;
	ipl = -1;
	if (cross) copy_point(cross, ZeroPoint);
	if (seg1) *seg1 = -1;
	if (seg2) *seg2 = -1;

	/* Make sure things are consistent */
	if (!line1)               return FALSE;
	if (line1->numpts <= 0)   return FALSE;
	if (!line2)               return FALSE;
	if (line2->numpts <= 0)   return FALSE;

	/* See what side of second line the first line begins on */
	ip = 0;
	p  = line1->points[ip];
	line_test_point(line2, p, NullFloat, cross, &seg, NullChar, &right);
	if (right) ipr = ip;
	else       ipl = ip;

	/* See what side of second line the first line ends on */
	ip = line1->numpts -1;
	p  = line1->points[ip];
	line_test_point(line2, p, NullFloat, cross, &seg, NullChar, &right);
	if (right) ipr = ip;
	else       ipl = ip;

	/* Give up if both on the same side */
	if (ipr < 0) return FALSE;
	if (ipl < 0) return FALSE;

	/* Do a binary search for the intersection */
	/* We know here that ipr is on the right and ipl is on the left */
	while (abs(ipr-ipl) > 1)
		{
		ip = (ipr+ipl) / 2;
		p  = line1->points[ip];
		line_test_point(line2, p, NullFloat, cross, &seg, NullChar, &right);
		if (right) ipr = ip;
		else       ipl = ip;
		}

	/* Return segments in which crossover occurs */
	if (seg1) *seg1 = MIN(ipr, ipl);
	if (seg2) *seg2 = seg;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     n e x t _ l i n e _ c r o s s i n g                              *
*                                                                      *
*      Obsolescent function!                                           *
*                                                                      *
*      Use equivalent find_line_crossing() function.                   *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** OBSOLETE!
 * Find the next point at which two lines cross.  It is assumed that
 * the lines cross any number of times, or not at all.
 *
 *	@param[in] 	line1	first line to be examined
 *	@param[in] 	line2	second line to be examined
 *	@param[in] 	start	span of line1 to start at
 *	@param[out]	cross	point of intersection
 *	@param[out]	*span1	span on line1 where crossover occurs
 *	@param[out]	*span2	span on line2 where crossover occurs
 *  @return True if successful in finding a crossing point.
 *********************************************************************/

LOGICAL	next_line_crossing

	(
	LINE	line1,
	LINE	line2,
	int		start,
	POINT	cross,
	int		*span1,
	int		*span2
	)

	{
	int		ip, ips;
	LOGICAL	pside, nside, between;
	float	*ppos, *npos;
	float	pdist, ndist;

	static	LOGICAL	done = FALSE;
	if (!done)
		{
		done = TRUE;
		(void) printf("*** Obsolete function temoprarily supported:\n");
		(void) printf("Replace:\n");
		(void) printf("    LINE   line1;\n");
		(void) printf("    LINE   line2;\n");
		(void) printf("    int    start;\n");
		(void) printf("    POINT  cross;\n");
		(void) printf("    int    span1;\n");
		(void) printf("    int    span2;\n");
		(void) printf("    next_line_crossing(line1, line2, start, cross, &seg1, &seg2);\n");
		(void) printf("With:\n");
		(void) printf("    LINE   line1;\n");
		(void) printf("    LINE   line2;\n");
		(void) printf("    int    start;\n");
		(void) printf("    POINT  cross;\n");
		(void) printf("    int    span1;\n");
		(void) printf("    int    span2;\n");
		(void) printf("    if (line1 && line1->numpts > 0 && start < line1->numpts)\n");
		(void) printf("        find_line_crossing(line1, line2, start, line1->points[start],\n");
		(void) printf("                            cross, &span1, &span2, NullLogical);\n");
		(void) printf("*** End\n");
		}

	/* Set up reasonable return values */
	if (cross) copy_point(cross, ZeroPoint);
	if (span1) *span1  = -1;
	if (span2) *span2  = -1;

	/* Make sure things are consistent */
	if (!line1)                 return FALSE;
	if (line1->numpts <= 0)     return FALSE;
	if (!line2)                 return FALSE;
	if (line2->numpts <= 0)     return FALSE;
	if (start >= line1->numpts) return FALSE;

	/* See what side of second line the first line begins on */
	/* Note that the side for an exact match cannot be determined! */
	for (ip=start; ip<line1->numpts; ip++)
		{
		ppos = line1->points[ip];
		line_test_point(line2, ppos, &pdist, NullPoint, NullInt, NullChar,
				&pside);
		if (pdist > 0.0) break;
		pr_diag("next_line_crossing",
			"Start of line 1 is an exact match!\n");
		}
	ips = ip;

	/* Scan rest of first line to find crossings */
	ppos = line1->points[start];
	for (ip=ips+1; ip<line1->numpts; ip++)
		{
		/* See what side of second line the next point is on */
		/* Note that the side for an exact match cannot be determined! */
		npos = line1->points[ip];
		line_test_point(line2, npos, &ndist, NullPoint, NullInt, NullChar,
				&nside);
		if (ndist == 0.0)
			{
			/* Keep looking */
			pr_error("next_line_crossing",
				"Point %d (%.0f/%.0f) on line 1 is an exact match!\n",
				ip, npos[X], npos[Y]);
			ppos = npos;
			continue;
			}
		if (nside == pside)
			{
			/* Keep looking */
			ppos = npos;
			continue;
			}

		/* May have crossed over - find the intersection point */
		if (span1) *span1 = ip - 1;
		if ( line_sight(line2, ppos, npos, FALSE, NullFloat, NullFloat,
				cross, span2, &between) && between )
			{
			if (cross)
				pr_diag("next_line_crossing",
					"Crossover at span %d (%.0f/%.0f)\n",
					ip-1, cross[X], cross[Y]);
			return TRUE;
			}

		else
			{
			/* Keep looking */
			ppos = npos;
			continue;
			}
		}

	/* No intersections found */
	return FALSE;
	}
