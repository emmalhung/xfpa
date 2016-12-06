/*********************************************************************/
/** @file segment.c
 *
 * Routines to handle the SEGMENT object.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*      s e g m e n t . c                                               *
*                                                                      *
*      Routines to handle the SEGMENT object.                          *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
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

#define SEGMENT_INIT
#include "segment.h"

#include <fpa_getmem.h>
#include <fpa_math.h>
#include <string.h>

int		SegmentCount = 0;

/***********************************************************************
*                                                                      *
*      c r e a t e _ s e g m e n t                                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Create an empty segment object.
 *
 *	@return Pointer to the new segment object. You will need to
 * 			destroy this object when you are finished with it.
 *********************************************************************/

SEGMENT	create_segment(void)

	{
	SEGMENT	snew;

	/* Allocate structure */
	snew = INITMEM(struct SEGMENT_struct, 1);
	if (!snew) return NullSegment;

	/* Initialize segment buffer */
	snew->line  = NullLine;
	snew->ips   = -1;
	snew->ipe   = -1;
	snew->flags = 0;

	SegmentCount++;
	return snew;
	}

/***********************************************************************
*                                                                      *
*      d e s t r o y _ s e g m e n t                                   *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Free all allocated memory from the given segment.
 *
 *	@param[in] 	segment	segment to free
 *  @return NullSegment
 *********************************************************************/

SEGMENT	destroy_segment

	(
	SEGMENT	segment
	)

	{
	/* Do nothing if not there */
	if (!segment) return NullSegment;

	/* Destroy the actual line if exclusive */
	if (seg_exclusive(segment)) destroy_line(segment->line);
	else                        segment->line = NullLine;

	/* Free the structure itself */
	FREEMEM(segment);
	SegmentCount--;
	return NullSegment;
	}

/***********************************************************************
*                                                                      *
*      e m p t y _ s e g m e n t                                       *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Empty the point buffer of the given segment without actually
 * freeing any memory.
 *
 *	@param[in] 	segment	segment to empty
 *********************************************************************/

void	empty_segment

	(
	SEGMENT	segment
	)

	{
	/* Do nothing if not there */
	if (!segment) return;

	/* Empty the actual line if exclusive */
	if (seg_exclusive(segment)) empty_line(segment->line);
	else                        segment->line = NullLine;

	/* Reset the point indices */
	segment->ips = -1;
	segment->ipe = -1;
	}

/***********************************************************************
*                                                                      *
*      c o p y _ s e g m e n t                                         *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Make an exact copy of a segment.
 *
 *	@param[in] 	segment		segment to be copied
 *	@param[in] 	clone		copy the line or just use a pointer?
 * 	@return Pointer to copy of segment. You will need to destroy this
 * 			object when you are finished with it.
 *********************************************************************/

SEGMENT	copy_segment

	(
	const SEGMENT	segment,
	LOGICAL			clone
	)

	{
	SEGMENT	snew;

	if (!segment) return NullSegment;

	/* Create an empty one */
	snew = create_segment();

	/* Make a non-exclusive copy */
	snew->line  = segment->line;
	snew->ips   = segment->ips;
	snew->ipe   = segment->ipe;
	snew->flags = segment->flags & ~SegExclusive;

	/* Make it exclusive if requested */
	if (clone) trim_segment_buffer(snew);
	return snew;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ s e g m e n t                                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Set or reset the given segment to be the given portion of
 * the given line.
 *
 * If Clone = FALSE then only a point is used to store the given
 * line. Be careful when you destroy these items.
 *
 *	@param[in] 	segment	given segment
 *	@param[in] 	line	line to be used
 *	@param[in] 	clone	do we want to copy the line or just use a pointer?
 *	@param[in] 	ips		line portion
 *	@param[in] 	ipe		line portion
 *	@param[in] 	fwd		count forward or backward from ips to ipe?
 *********************************************************************/

void	define_segment

	(
	SEGMENT	segment,
	LINE	line,
	LOGICAL	clone,
	int		ips,
	int		ipe,
	LOGICAL	fwd
	)

	{
	/* Do nothing if segment not there */
	if (!segment) return;

	/* Get rid of existing contents */
	if (seg_exclusive(segment)) destroy_line(segment->line);
	segment->line  = NullLine;
	segment->ips   = -1;
	segment->ipe   = -1;
	segment->flags = 0;

	/* Do no more if line not given */
	if (!line) return;

	/* Make a non-exclusive copy */
	segment->line  = line;
	segment->ips   = ips;
	segment->ipe   = ipe;
	segment->flags = (fwd)? SegForward: 0;

	/* Make it exclusive if requested */
	if (clone) trim_segment_buffer(segment);
	}

/***********************************************************************
*                                                                      *
*      c l o n e _ s e g m e n t _ b u f f e r                         *
*      t r i m _ s e g m e n t _ b u f f e r                           *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Change segment to use an exclusive copy of the line.
 *
 *	@param[in] 	segment	given segment
 *********************************************************************/

void	clone_segment_buffer

	(
	SEGMENT	segment
	)

	{
	/* Do nothing if segment not there */
	if (!segment) return;

	/* Do nothing if segment already exclusive */
	if (seg_exclusive(segment)) return;

	/* Duplicate the line */
	segment->line  = copy_line(segment->line);
	segment->flags |= SegExclusive;
	}

/**********************************************************************/

/*********************************************************************/
/** Discard the unused portions.
 *
 *	@param[in] 	segment	given segment
 *********************************************************************/
void	trim_segment_buffer

	(
	SEGMENT	segment
	)

	{
	int		ips, ipe;
	LINE	line;

	/* Do nothing if segment not there */
	if (!segment) return;

	/* Check if line is Null */
	if (!segment->line)
		{
		segment->flags |= SegExclusive;
		return;
		}

	/* Check if range is empty */
	ips = segment->ips;
	ipe = segment->ipe;
	if (ips < 0 || ipe < 0)
		{
		segment->line = NullLine;
		segment->flags |= SegExclusive;
		return;
		}

	/* Check if already trimmed */
	if (seg_exclusive(segment))
		{
		if (seg_forward(segment))
			{
			if (ips == 0 && ipe == segment->line->numpts-1) return;
			}
		else
			{
			if (ipe == 0 && ips == segment->line->numpts-1) return;
			}
		}

	/* Initialize to empty */
	line = segment->line;
	segment->line = create_line();

	/* Add the appropriate pieces */
	if (seg_forward(segment))
		{
		if (ips <= ipe)
			{
			/* Forward without wrap */
			append_line_portion(segment->line, line, ips, ipe);
			}
		else
			{
			/* Forward with wrap */
			append_line_portion(segment->line, line, ips, line->numpts-1);
			append_line_portion(segment->line, line, 0,   ipe);
			}
		segment->ips = 0;
		segment->ipe = segment->line->numpts - 1;
		}
	else
		{
		if (ips >= ipe)
			{
			/* Backward without wrap */
			append_line_portion(segment->line, line, ipe, ips);
			}
		else
			{
			/* Backward with wrap */
			append_line_portion(segment->line, line, ipe, line->numpts-1);
			append_line_portion(segment->line, line, 0,   ips);
			}
		segment->ips = segment->line->numpts - 1;
		segment->ipe = 0;
		}

	/* Set exclusive mode */
	if (seg_exclusive(segment)) destroy_line(line);
	segment->flags |= SegExclusive;
	}

/***********************************************************************
*                                                                      *
*      s e g m e n t s _ a d j a c e n t                               *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Determine if 2 segments are adjacent (in the order given).
 *
 *	@param[in] 	seg1	first segment
 *	@param[in] 	seg2	second segment
 *  @return True if segments are adjacent.
 *********************************************************************/

LOGICAL	segments_adjacent

	(
	SEGMENT	seg1,
	SEGMENT	seg2
	)

	{
	int		lp;
	LOGICAL	fwd1, fwd2;

	if (!seg1)       return FALSE;
	if (!seg2)       return FALSE;
	if (!seg1->line) return FALSE;
	if (!seg2->line) return FALSE;

	/* Segments must use the same line */
	if (seg1->line != seg2->line) return FALSE;
	lp   = seg1->line->numpts - 1;

	/* Segments must use the same direction */
	fwd1 = seg_forward(seg1);
	fwd2 = seg_forward(seg2);
	if (fwd1 && !fwd2) return FALSE;
	if (!fwd1 && fwd2) return FALSE;

	/* Appropriate end-points must line up */
	if (fwd1)
		{
		if (seg1->ipe+1 == seg2->ips)      return TRUE;
		if (seg1->ipe==lp && seg2->ips==0) return TRUE;
		}
	else
		{
		if (seg2->ipe+1 == seg1->ips)      return TRUE;
		if (seg2->ipe==lp && seg1->ips==0) return TRUE;
		}

	return FALSE;
	}

/***********************************************************************
*                                                                      *
*      j o i n _ s e g m e n t s                                       *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Join 2 segments if they are adjacent.
 *
 *	@param[in] 	seg1	first segment
 *	@param[in] 	seg2	second segment
 * 	@return pointer to extended segment. NullSegment if something
 * 			went wrong. The returned segment shares memory with seg1.
 *********************************************************************/

SEGMENT	join_segments

	(
	SEGMENT	seg1,
	SEGMENT	seg2
	)

	{
	int		lp;
	LOGICAL	fwd;
	SEGMENT	seg;

	if (!segments_adjacent(seg1, seg2)) return NullSegment;

	/* Create a copy of the first segment */
	seg = copy_segment(seg1, FALSE);
	lp  = seg->line->numpts - 1;
	fwd = seg_forward(seg);

	/* Join the second segment */
	if (fwd) seg->ipe = seg2->ipe;
	else     seg->ips = seg2->ips;

	/* If segment wraps all the way around, anchor at 0 */
	if (seg->ipe+1 == seg->ips)
		{
		seg->ips = 0;
		seg->ipe = lp;
		}

	return seg;
	}
