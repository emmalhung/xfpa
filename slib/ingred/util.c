/***********************************************************************
*                                                                      *
*     u t i l . c                                                      *
*                                                                      *
*     Miscellaneous operations for the edit/link/interp processes.     *
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

#include "ingred_private.h"

#undef DEBUG_UTIL

/***********************************************************************
*                                                                      *
*     t r u n c _ l i n e   - Chop the end(s) of the given line.       *
*                                                                      *
***********************************************************************/

LOGICAL	trunc_line

	(
	LINE	line,
	float	dist,
	LOGICAL	front,
	LOGICAL	back
	)

	{
	float	ds;
	int		np, ips, ipe, ixs, ixe, ip, jp;

	if (IsNull(line)) return FALSE;
	if (dist <= 0)    return FALSE;

	np = line->numpts;
	if (np < 1) return FALSE;
	if (np < 2) return TRUE;

	if (!front && !back) return TRUE;
	ips = 0;
	ipe = np-1;
	ixs = (back)?  (np-1)/2: np-2;
	ixe = (front)? (np+1)/2: 1;

	/* Truncate from the beginning */
	if (front)
		{
		/* Walk forward from the beginning */
		ds  = line_walk(line, 0.0, dist);
		ips = (int)(ds+1);
		ips = MIN(ips, ixs);
		}

	/* Truncate from the end */
	if (back)
		{
		/* Walk backward from the end */
		ds  = line_walk(line, (float)(np-1), -dist);
		ipe = (int)(ds);
		ipe = MAX(ipe, ixe);
		}

	/* Now modify the point buffer */
	np = ipe - ips + 1;
	if (ips > 0)
		{
		for (ip=0; ip<np; ip++)
			{
			jp = ip + ips;
			copy_point(line->points[ip], line->points[jp]);
			}
		}
	line->numpts = np;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     s m o o t h _ l i n e     - Smooth the given line.               *
*                                 (The given line is modified)         *
*                                                                      *
*     s m j o i n _ l i n e s   - Smoothly join two given lines.       *
*                                 (The 1st given line is modified)     *
*                                                                      *
*     s m c l o s e _ l i n e   - Smoothly close the given line.       *
*                                 (The given line is modified)         *
*                                                                      *
*     b r i d g e _ l i n e s   - Build the smooth join between 2      *
*                                 given lines.                         *
*                                                                      *
***********************************************************************/

LINE	smooth_line

	(
	LINE	line,
	float	fres,
	float	sres
	)

	{
	LOGICAL	closed;
	int		nl;
	LINE	*lines;

	if (IsNull(line) || line->numpts < 1) return line;

	/* Check for closed lines */
	closed = line_closed(line);

	/* Set up spline operation */
	reset_pipe();
	enable_filter(fres, 0.0);
	enable_spline(sres, closed, 0.0, 0.0, 0.0);
	enable_save();

	/* Re-spline line to desired resolution */
	line_pipe(line);
	nl = recall_save(&lines);
	if (nl <= 0)
		{
		reset_pipe();
		return line;
		}

	/* Replace the line with the re-splined line */
	empty_line(line);
	append_line(line, lines[0]);
	reset_pipe();

	return line;
	}

/**********************************************************************/

LINE	smjoin_lines

	(
	LINE	line1,
	LINE	line2,
	float	fres,
	float	sres
	)

	{
	LINE	join;
	int		ip, np;

#	ifdef DEBUG_UTIL
	int		ix;
#	endif /* DEBUG_UTIL */

	if (IsNull(line1) || line1->numpts < 1) return append_line(line1, line2);
	if (IsNull(line2) || line2->numpts < 1) return line1;

	join = bridge_lines(line1, line2, fres, sres);
	if (IsNull(join)) return append_line(line1, line2);

#	ifdef DEBUG_UTIL
	pr_diag("smjoin_lines", "  ----- Join line with %d points\n",
		join->numpts);
		for (ix=0; ix<join->numpts; ix++)
			pr_diag("smjoin_lines", "         %.2f %.2f\n",
				join->points[ix][X], join->points[ix][Y]);
#	endif /* DEBUG_UTIL */

	np = line1->numpts;
	line1->numpts -= (np>1)? 2: 1;

#	ifdef DEBUG_UTIL
	if (line1->numpts > 0)
		{
		pr_diag("smjoin_lines", "  ----- Start line 0-%d of %d points\n",
			line1->numpts, np);
			for (ix=0; ix<line1->numpts; ix++)
				pr_diag("smjoin_lines", "         %.2f %.2f\n",
					line1->points[ix][X], line1->points[ix][Y]);
		}
	else
		pr_diag("smjoin_lines", "  ----- Start line of %d points not used!\n",
			np);
#	endif /* DEBUG_UTIL */

	append_line(line1, join);
	join = destroy_line(join);

	np = line2->numpts;
	ip = (np>1)? 2: 1;

#	ifdef DEBUG_UTIL
	if (ip < np)
		{
		pr_diag("smjoin_lines", "  ----- End line %d-%d of %d points\n",
			ip, np-1, np);
		for (ix=ip; ix<np; ix++)
			pr_diag("smjoin_lines", "         %.2f %.2f\n",
				line2->points[ix][X], line2->points[ix][Y]);
		}
	else
		pr_diag("smjoin_lines", "  ----- End line of %d points not used!\n",
			np);
#	endif /* DEBUG_UTIL */

	append_line_portion(line1, line2, ip, np-1);
	return line1;
	}

/**********************************************************************/

LINE	smclose_line

	(
	LINE	line,
	float	fres,
	float	sres
	)

	{
	LINE	join;
	int		np;

	if (IsNull(line) || line->numpts < 1) return line;
	if (line_closed(line))                return line;

	if (line->numpts < 3)
		{
		close_line(line);
		return line;
		}

	join = bridge_lines(line, line, fres, sres);
	if (IsNull(join))
		{
		close_line(line);
		return line;
		}

	np = line->numpts;
	line->numpts -= (np>1)? 2: 1;

	append_line(line, join);
	join = destroy_line(join);

	if (np>1)
		{
		copy_point(line->points[0], line->points[1]);
		condense_line(line);
		}
	close_line(line);
	return line;
	}

/**********************************************************************/

LINE	bridge_lines

	(
	LINE	line1,
	LINE	line2,
	float	fres,
	float	sres
	)

	{
	LINE	join;
	int		np;
	int		nl;
	LINE	*lines;

	if (IsNull(line1))     return NullLine;
	if (IsNull(line2))     return NullLine;
	if (line1->numpts < 1) return NullLine;
	if (line2->numpts < 1) return NullLine;

	reset_pipe();
	enable_filter(fres, 0.0);
	enable_spline(sres, FALSE, 0.0, 0.0, 0.0);
	enable_save();

	/* Start with end of first line */
	np = line1->numpts;
	if (np > 1) put_pipe(line1->points[np-2][X], line1->points[np-2][Y]);
	put_pipe(line1->points[np-1][X], line1->points[np-1][Y]);

	/* Finish with start of second line */
	np = line2->numpts;
	put_pipe(line2->points[0][X], line2->points[0][Y]);
	if (np > 1) put_pipe(line2->points[1][X], line2->points[1][Y]);

	flush_pipe();
	nl = recall_save(&lines);
	if (nl <= 0)
		{
		reset_pipe();
		return NullLine;
		}
	join = copy_line(lines[0]);
	reset_pipe();
	return join;
	}

/***********************************************************************
*                                                                      *
*     s p o t _ l i s t _ t r a n s l a t e   - Reposition list of     *
*                                               spots by translation.  *
*                                                                      *
*     s p o t _ l i s t _ r o t a t e   - Reposition list of spots by  *
*                                         rotation about a location.   *
*                                                                      *
***********************************************************************/

LOGICAL	spot_list_translate

	(
	int		nsp,
	SPOT	*spots,
	float	dx,
	float	dy
	)

	{
	int		ip;
	SPOT	spot;

	if (IsNull(spots)) return FALSE;
	if (nsp <= 0)      return TRUE;

	/* Translate anchor position for each spot */
	for (ip=0; ip<nsp; ip++)
		{
		spot = spots[ip];
		if (IsNull(spot)) continue;
		spot->anchor[X] += dx;
		spot->anchor[Y] += dy;
		}
	return TRUE;
	}

/**********************************************************************/

LOGICAL	spot_list_rotate

	(
	int		nsp,
	SPOT	*spots,
	POINT	ref,
	float	angle
	)

	{
	int		ip;
	float	ca, sa, xx, yy;
	SPOT	spot;

	if (IsNull(spots)) return FALSE;
	if (nsp <= 0)      return TRUE;
	if (angle == 0)    return TRUE;

	/* Turn angle into cos and sin */
	ca = cosdeg(angle);
	sa = sindeg(angle);

	/* Rotate anchor position for each spot around reference location */
	for (ip=0; ip<nsp; ip++)
		{
		spot = spots[ip];
		if (IsNull(spot)) continue;
		xx = spot->anchor[X] - ref[X];
		yy = spot->anchor[Y] - ref[Y];
		spot->anchor[X] = xx*ca - yy*sa + ref[X];
		spot->anchor[Y] = xx*sa + yy*ca + ref[Y];
		}
	return TRUE;
	}
