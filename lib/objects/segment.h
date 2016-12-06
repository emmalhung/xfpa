/*********************************************************************/
/** @file segment.h
 *
 * SEGMENT object definitions (include file)
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    s e g m e n t . h                                                 *
*                                                                      *
*    SEGMENT object definitions (include file)                         *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2002 Environment Canada (MSC)            *
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

/* See if already included */
#ifndef SEGMENT_DEFS
#define SEGMENT_DEFS

/* We need definitions for other objects */
#include "line.h"

/* Define SEGMENT object */
/** a contiguous segment of a line, including wrap
 * around for closed boundaries */
typedef struct SEGMENT_struct
	{
	LINE	line;	/**< which line the segment comes from */

	int		ips;	/**< starting point index */
	int		ipe;	/**< ending point index */
	long	flags;	/**< matching flags (see flag defines below) */
	} *SEGMENT;

/* Convenient definitions */
#define NullSegment        NullPtr(SEGMENT)
#define NullSegmentPtr     NullPtr(SEGMENT *)
#define NullSegmentList    NullPtr(SEGMENT *)
#define NullSegmentListPtr NullPtr(SEGMENT **)

/* Flags for individual segments */
#define	SegForward		(1L<<0)	/**< direction of segment matches point order */
#define	SegExclusive	(1L<<1)	/**< has the segment been allocated exclusively */
#define	SegShown		(1L<<2)	/**< should the segment be displayed */
#define	SegFilter		(1L<<3)	/**< can the segment be filtered */
#define	SegNormal		(SegForward | SegExclusive | SegShown | SegFilter)

/* Quick functions for segments */
#define	seg_forward(seg)	(((seg)->flags & SegForward)   != 0)
#define	seg_exclusive(seg)	(((seg)->flags & SegExclusive) != 0)
#define	seg_shown(seg)		(((seg)->flags & SegShown)     != 0)
#define	seg_filter(seg)		(((seg)->flags & SegFilter)    != 0)

/* Declare all functions in segment.c */
SEGMENT	create_segment(void);
SEGMENT	destroy_segment(SEGMENT seg);
void	empty_segment(SEGMENT seg);
SEGMENT	copy_segment(const SEGMENT seg, LOGICAL clone);
void	define_segment(SEGMENT seg, LINE line, LOGICAL clone,
						int ips, int ipe, LOGICAL fwd);
void	clone_segment_buffer(SEGMENT seg);
void	trim_segment_buffer(SEGMENT seg);
LOGICAL	segments_adjacent(SEGMENT seg1, SEGMENT seg2);
SEGMENT	join_segments(SEGMENT seg1, SEGMENT seg2);

/* Declare all functions in segment_oper.c */
float	segment_length(SEGMENT seg);
LOGICAL	before_segment_start(SEGMENT seg, int span);
LOGICAL	after_segment_end(SEGMENT seg, int span);
LOGICAL	segment_crossing(SEGMENT seg, LINE line,
						int start_span, POINT start_pos,
						POINT cross, int *xspan, int *lspan, LOGICAL *right);
LOGICAL	seglist_crossing(int nseg, SEGMENT *seglist, LOGICAL closed, LINE line,
						int start_seg, int start_span, POINT start_pos,
						POINT cross, int *xseg, int *xspan, int *lspan,
						LOGICAL *right);

/* Now it has been included */
#endif
