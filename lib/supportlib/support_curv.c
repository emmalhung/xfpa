/***********************************************************************
*                                                                      *
*   s u p p o r t _ c u r v . c                                        *
*                                                                      *
*   Obsolescent functions to handle the CURVE object.                  *
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
*     c u r v e _ c r o s s i n g                                      *
*                                                                      *
*      Obsolescent function!                                           *
*                                                                      *
*      Use equivalent find_curve_crossing() function.                  *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** OBSOLETE!
 * Find the point at which two curves cross.
 *
 * It is assumed that the curves cross only once, or not at all.
 *
 *	@param[in]	 	curv1	Curves to be examined
 *	@param[in]	 	curv2	Curves to be examined
 *	@param[out] 	cross	Point of intersection
 *	@param[out] 	*seg1	Segment on curv1 where crossover occurs
 *	@param[out] 	*seg2	Segment on curv2 where crossover occurs
 *  @return True if Successful.
 *********************************************************************/

LOGICAL	curve_crossing

	(
	CURVE	curv1,
	CURVE	curv2,
	POINT	cross,
	int		*seg1,
	int		*seg2
	)

	{
	static	LOGICAL	done = FALSE;
	if (!done)
		{
		done = TRUE;
		(void) printf("*** Obsolete function temoprarily supported:\n");
		(void) printf("Replace:\n");
		(void) printf("    CURVE  curv1;\n");
		(void) printf("    CURVE  curv2;\n");
		(void) printf("    POINT  cross;\n");
		(void) printf("    int    seg1;\n");
		(void) printf("    int    seg2;\n");
		(void) printf("    curve_crossing(curv1, curv2, cross, &seg1, &seg2);\n");
		(void) printf("With:\n");
		(void) printf("    CURVE  curv1;\n");
		(void) printf("    CURVE  curv2;\n");
		(void) printf("    POINT  cross;\n");
		(void) printf("    int    seg1;\n");
		(void) printf("    int    seg2;\n");
		(void) printf("    if (curv1 && curv1->line && curv1->line->numpts > 0)\n");
		(void) printf("        find_curve_crossing(curv1, curv2, 0, curv1->line->points[0],\n");
		(void) printf("                             cross, &seg1, &seg2, NullLogical);\n");
		(void) printf("*** End\n");
		}

	/* Set up reasonable return values */
	if (cross) copy_point(cross, ZeroPoint);
	if (seg1) *seg1 = -1;
	if (seg2) *seg2 = -1;
	if (!curv1) return FALSE;
	if (!curv2) return FALSE;

	return line_crossing(curv1->line, curv2->line, cross, seg1, seg2);
	}
