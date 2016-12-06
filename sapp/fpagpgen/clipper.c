/***********************************************************************
*                                                                      *
*     c l i p p e r . c                                                *
*                                                                      *
*     Routines to clip patterns for line drawing                       *
*                                                                      *
*     Version 5 (c) Copyright 1997 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2001 Environment Canada (MSC)            *
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

#include "clipper.h"
#include "fpagpgen_structs.h"

/* We need FPA library definitions */
#include <fpa.h>

/***********************************************************************
*                                                                      *
*    G R A _ c l i p _ b o u n d a r y                                 *
*                                                                      *
***********************************************************************/

BOUND			GRA_clip_boundary

	(
	BOUND		pbound		/* boundary (including holes) to clip */
	)

	{
	LINE	pline, *lines;
	int		nline, ihole;

	/* Static BOUND object to return */
	static	BOUND	Bound = NullBound;

	/* Set up clipping to the current portion of the pattern     */
	/* Clip the current pattern component and display the pieces */

	/* First clip the boundary */
	pline = pbound->boundary;

	/* Ensure that the boundary is a closed curve */
	if ( pline->points[0][X] != pline->points[pline->numpts-1][X]
			|| pline->points[0][Y] != pline->points[pline->numpts-1][Y] )
		(void) add_point_to_line(pline, pline->points[0]);

	/* Clip the boundary outline */
	(void) reset_pipe();
	(void) enable_clip(0.0, BaseMap.definition.xlen,
						0.0, BaseMap.definition.ylen, TRUE, TRUE);
	(void) enable_save();
	(void) line_pipe(pline);

	/* Return if the entire boundary has been clipped */
	nline = recall_save(&lines);
	if ( nline == 0 ) return NullBound;

	/* Reallocate the static BOUND Object */
	(void) destroy_bound(Bound);
	Bound = create_bound();

	/* Add the clipped boundary to the BOUND object */
	(void) define_bound_boundary(Bound, copy_line(lines[0]));

	/* Now clip the boundary holes */
	for ( ihole=0; ihole<pbound->numhole; ihole++ )
		{

		/* Ensure that the hole is a closed curve */
		pline = pbound->holes[ihole];
		if ( pline->points[0][X] != pline->points[pline->numpts-1][X]
				|| pline->points[0][Y] != pline->points[pline->numpts-1][Y] )
			(void) add_point_to_line(pline, pline->points[0]);

		/* Clip the hole outline */
		(void) reset_pipe();
		(void) enable_clip(0.0, BaseMap.definition.xlen,
							0.0, BaseMap.definition.ylen, TRUE, TRUE);
		(void) enable_save();
		(void) line_pipe(pline);

		/* Continue if the entire hole has been clipped */
		nline = recall_save(&lines);
		if ( nline == 0 ) continue;

		/* Add the clipped hole to the BOUND object */
		(void) add_bound_hole(Bound, copy_line(lines[0]));
		}

	/* Return the remaining boundary */
	return Bound;
	}

/***********************************************************************
*                                                                      *
*    G R A _ c l i p _ o u t l i n e                                   *
*                                                                      *
***********************************************************************/

LINE			GRA_clip_outline

	(
	LINE		pline		/* outline to clip */
	)

	{
	LINE	*lines;
	int		nline;

	/* Static LINE object to return */
	static	LINE	Line = NullLine;

	/* Set up clipping to the current portion of the pattern     */
	/* Clip the current pattern component and display the pieces */

	/* Ensure that this is a closed curve */
	if ( pline->points[0][X] != pline->points[pline->numpts-1][X]
			|| pline->points[0][Y] != pline->points[pline->numpts-1][Y] )
		(void) add_point_to_line(pline, pline->points[0]);

	/* Clip the outline */
	(void) reset_pipe();
	if ( AnchorToCrossSection )
		{
		/* Clip the outline for a cross section */
		(void) enable_clip((float) XSect_ULpoint[X], (float) XSect_LRpoint[X],
							(float) XSect_LRpoint[Y], (float) XSect_ULpoint[Y],
							TRUE, TRUE);
		}
	else
		{
		/* Clip the outline for a map */
		(void) enable_clip(0.0, BaseMap.definition.xlen,
							0.0, BaseMap.definition.ylen, TRUE, TRUE);
		}
	(void) enable_save();
	(void) line_pipe(pline);

	/* Return if the entire outline has been clipped */
	nline = recall_save(&lines);
	if ( nline == 0 ) return NullLine;

	/* Reallocate the static LINE Object */
	(void) destroy_line(Line);

	/* Return a copy of the remaining outline */
	Line = copy_line(lines[0]);
	return Line;
	}

/***********************************************************************
*                                                                      *
*    G R A _ c l i p _ o u t l i n e _ s e g m e n t                   *
*                                                                      *
***********************************************************************/

LINE			GRA_clip_outline_segment

	(
	LINE		pline		/* outline to clip */
	)

	{
	LINE	*lines;
	int		nline;

	/* Static LINE object to return */
	static	LINE	Line = NullLine;

	/* Set up clipping to the current portion of the pattern     */
	/* Clip the current pattern component and display the pieces */

	/* Clip the outline */
	(void) reset_pipe();
	(void) enable_clip(0.0, BaseMap.definition.xlen,
						0.0, BaseMap.definition.ylen, TRUE, FALSE);
	(void) enable_save();
	(void) line_pipe(pline);

	/* Return if the entire outline has been clipped */
	nline = recall_save(&lines);
	if ( nline == 0 ) return NullLine;

	/* Reallocate the static LINE Object */
	(void) destroy_line(Line);

	/* Return a copy of the remaining outline */
	Line = copy_line(lines[0]);
	return Line;
	}

/***********************************************************************
*                                                                      *
*    G R A _ c l i p _ l i n e                                         *
*                                                                      *
***********************************************************************/

int				GRA_clip_line

	(
	LINE		pline,		/* line to clip */
	LINE		**lines		/* output line segments */
	)

	{
	int		nline;

	/* Set up clipping to the current portion of the pattern     */
	/* Clip the current pattern component and display the pieces */

	/* Clip the line */
	(void) reset_pipe();
	if ( AnchorToCrossSection )
		{
		/* Clip the line for a cross section */
		(void) enable_clip((float) XSect_ULpoint[X], (float) XSect_LRpoint[X],
							(float) XSect_LRpoint[Y], (float) XSect_ULpoint[Y],
							FALSE, FALSE);
		}
	else
		{
		/* Clip the line for a map */
		(void) enable_clip(0.0, BaseMap.definition.xlen,
							0.0, BaseMap.definition.ylen, FALSE, FALSE);
		}
	(void) enable_save();
	(void) line_pipe(pline);

	/* Return the number of clipped lines */
	nline = recall_save(lines);
	return nline;
	}
