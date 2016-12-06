/*********************************************************************/
/**	@file line.c
 *
 * Routines to handle the LINE objects.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*      l i n e . c                                                     *
*                                                                      *
*      Routines to handle the LINE objects.                            *
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

#define LINE_INIT
#include "line.h"

#include <fpa_getmem.h>
#include <tools/tools.h>

#include <string.h>

int		LineCount = 0;

/***********************************************************************
*                                                                      *
*      c r e a t e _ l i n e                                           *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Create an empty line object.
 *
 * @return Pointer to a line object. You will need to destroy this
 * object when you are finished with it.
 *********************************************************************/

LINE	create_line(void)

	{
	LINE	lnew;

	/* Allocate structure */
	lnew = INITMEM(struct LINE_struct, 1);
	if (!lnew) return NullLine;

	/* Initialize point buffer */
	lnew->points = NullPointList;
	lnew->numpts = 0;
	lnew->maxpts = 0;

	LineCount++;
	return lnew;
	}

/***********************************************************************
*                                                                      *
*      d e s t r o y _ l i n e                                         *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Free all allocated memory from the given line.
 *
 *	@param[in] 	line	line to destroy
 *  @return NullLine
 *********************************************************************/

LINE	destroy_line

	(
	LINE	line
	)

	{
	/* Do nothing if not there */
	if (!line) return NullLine;

	/* Free the space used */
	if (line->maxpts > 0) FREEMEM(line->points);

	/* Free the structure itself */
	FREEMEM(line);
	LineCount--;
	return NullLine;
	}

/***********************************************************************
*                                                                      *
*      e m p t y _ l i n e                                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Empty the point buffer of the given line without actually
 * freeing any memory.
 *
 *	@param[in] 	line	line to empty
 *********************************************************************/

void	empty_line

	(
	LINE	line
	)

	{
	/* Do nothing if not there */
	if (!line) return;

	/* Zero the point counter */
	line->numpts = 0;
	}

/***********************************************************************
*                                                                      *
*      c o p y _ l i n e                                               *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Make an exact copy of a line.
 *
 *	@param[in] 	line	line to copy
 *  @return Pointer to Copy of the given line. You will need to
 *  		destroy this object when you are finished with it.
 *********************************************************************/

LINE	copy_line

	(
	const LINE	line
	)

	{
	LINE	lnew;

	if (!line) return NullLine;

	lnew = append_line(NullLine, line);
	return lnew;
	}

/***********************************************************************
*                                                                      *
*      a p p e n d _ l i n e                                           *
*      a p p e n d _ l i n e _ d i r                                   *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Append one line to the end of another.
 *
 *	@param[in] 	l1		Line to be extended
 *	@param[in] 	l2		line to append to l1
 *  @return Pointer to the extended line object.
 *********************************************************************/

LINE	append_line

	(
	LINE		l1,
	const LINE	l2
	)

	{
	/* Do nothing if second line not there */
	if (!l2) return l1;

	/* Append all the points from l2 to l1 */
	return append_line_pdir(l1, l2, 0, l2->numpts-1, TRUE);
	}

/**********************************************************************/

/*********************************************************************/
/**	Append one line to the end of another with a given direction. If
 * fwd is FALSE then l2 is appended in reverse order.
 *
 *	@param[in] 	l1		line to be extended
 *	@param[in] 	l2		line to append to l1
 *	@param[in] 	fwd		FALSE to reverse l2
 *  @return Point to the extended line object.
 *********************************************************************/
LINE	append_line_dir

	(
	LINE		l1,
	const LINE	l2,
	LOGICAL		fwd
	)

	{
	/* Do nothing if second line not there */
	if (!l2) return l1;

	/* Append all the points from l2 to l1 */
	return append_line_pdir(l1, l2, 0, l2->numpts-1, fwd);
	}

/***********************************************************************
*                                                                      *
*      a p p e n d _ l i n e _ p o r t i o n                           *
*      a p p e n d _ l i n e _ p d i r                                 *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Append part of one line to the end of another.
 *
 *	@param[in] 	l1		line to be extended
 *	@param[in] 	l2		line to append to l1
 *	@param[in] 	start	where to start on l2
 *	@param[in] 	end		where to end on l2
 *  @return Pointer to the extended line object.
 *********************************************************************/
LINE	append_line_portion

	(
	LINE		l1,
	const LINE	l2,
	int			start,
	int			end
	)

	{
	return append_line_pdir(l1, l2, start, end, TRUE);
	}

/**********************************************************************/

/*********************************************************************/
/**	Append part of one line to the end of another with a given
 * direction. If fwd is FALSE then l2 is appended in reverse order.
 *
 *	@param[in] 	l1		line to be extended
 *	@param[in] 	l2		line to be appended to l1
 *	@param[in] 	start	where to start on l2
 *	@param[in] 	end		where to end on l2
 *	@param[in] 	fwd		FALSE to reverse l2
 *  @return Point to the extended line object.
 *********************************************************************/
LINE	append_line_pdir

	(
	LINE		l1,
	const LINE	l2,
	int			start,
	int			end,
	LOGICAL		fwd
	)

	{
	int		i1, i2, nnew, nn, nadd;
	POINT	*pp1, *pp2;

	/* Do nothing if second line not there */
	if (!l2) return l1;

	/* If l1 not there, create an empty copy of l2 */
	if (!l1)
		{
		l1 = create_line();
		if (!l1) return NullLine;
		}
	if (l2->numpts <= 0) return l1;

	/* Make sure range is correct */
	if (start >= l2->numpts) return l1;
	if (end < 0)             return l1;
	if (start > end)         return l1;

	/* Determine extent of range */
	if (start < 0)         start = 0;
	if (end >= l2->numpts) end   = l2->numpts - 1;
	nadd = end - start + 1;

	/* Allocate more room in target if necessary */
	nnew = l1->numpts + nadd;
	if (nnew > l1->maxpts)
		{
		nn = (nnew-1)%DELTA_POINTS + 1;
		l1->maxpts = nnew + DELTA_POINTS - nn;
		l1->points = GETMEM(l1->points, POINT, l1->maxpts);
		}

	/* Add the points to the end of the target in desired direction */
	nn  = (fwd)? start: end;
	pp1 = l1->points + l1->numpts;
	pp2 = l2->points + nn;
	for (i1=0, i2=0; i1<nadd; i1++)
		{
		copy_point(pp1[i1], pp2[i2]);
		l1->numpts++;
		(fwd)? i2++: i2--;
		}

	return l1;
	}

/***********************************************************************
*                                                                      *
*     c o n d e n s e _ l i n e                                        *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Remove repeated points in the given line.
 *
 *	@param[in] 	line	line to be condensed
 *********************************************************************/

void	condense_line

	(
	LINE	line
	)

	{
	POINT	*p;
	float	px, py, qx, qy;
	int		ip, tp;

	/* Do nothing if not there */
	if (!line)             return;
	if (line->numpts <= 1) return;

	p  = line->points;
	px = p[0][X];
	py = p[0][Y];
	for (ip=1, tp=1; ip<line->numpts; ip++)
		{
		qx = p[ip][X];
		qy = p[ip][Y];
		if (px==qx && py==qy) continue;

		if (tp < ip) copy_point(p[tp], p[ip]);
		px = qx;
		py = qy;
		tp++;
		}

	line->numpts = tp;
	}

/***********************************************************************
*                                                                      *
*     r e v e r s e _ l i n e                                          *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Reverse the order of the points in the given line.
 *
 *	@param[in] 	line	line to be reversed
 *********************************************************************/

void	reverse_line

	(
	LINE	line
	)

	{
	POINT	*p, ptemp;
	int		ip, rp;

	/* Do nothing if not there */
	if (!line)             return;
	if (line->numpts <= 0) return;

	p = line->points;
	for (ip=0, rp=line->numpts-1; ip<rp; ip++, rp--)
		{
		copy_point(ptemp, p[rp]);
		copy_point(p[rp], p[ip]);
		copy_point(p[ip], ptemp);
		}
	}

/***********************************************************************
*                                                                      *
*      g e t _ l i n e _ p o i n t s                                   *
*      g e t _ l i n e _ p l i s t                                     *
*      s a v e _ l i n e _ p l i s t                                   *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Retrieve the point buffer information from the given line.
 *
 *	@param[in] 	line		given line pointer
 *	@param[out]	**points	returned start of point buffer
 *	@param[out]	*numpts		returned current number of points
 *	@param[out]	*maxpts		returned current reserve of points
 *********************************************************************/
void	get_line_points

	(
	LINE	line,
	POINT	**points,
	int		*numpts,
	int		*maxpts
	)

	{
	/* Retrieve the necessary info */
	if (points) *points = (line) ? line->points : NullPointList;
	if (numpts) *numpts = (line) ? line->numpts : 0;
	if (maxpts) *maxpts = (line) ? line->maxpts : 0;
	}

/**********************************************************************/

/*********************************************************************/
/** Copy the point buffer from the given line into a supplied buffer.
 *
 * @note useful for FORTRAN applications
 *
 *	@param[in] 	line	given line pointer
 *	@param[out]	*pbuf	buffer of points to be returned
 *	@param[out]	*numpts	number of points in curve
 *	@param[out]	*bufpts	size of buffer and actual number returned
 *********************************************************************/
void	get_line_plist

	(
	LINE	line,
	POINT	*pbuf,
	int		*numpts,
	int		*bufpts
	)

	{
	int	i;

	if (!numpts || !bufpts) return;

	/* Retrieve the actual number of points */
	*numpts = (line) ? line->numpts : 0;
	*bufpts = MIN(*bufpts, *numpts);
	if (*bufpts <= 0) return;

	/* Copy the points into the supplied buffer */
	if (pbuf)
		{
		for (i=0; i<*bufpts; i++)
			copy_point(pbuf[i], line->points[i]);
		}
	}

/**********************************************************************/

/*********************************************************************/
/** Place the given list of points in the given line.
 *
 *	@param[in] 	line	line to save points in
 *	@param[in] 	*points	buffer of points
 *	@param[in] 	numpts	number of points
 *********************************************************************/
void	save_line_plist

	(
	LINE	line,
	POINT	*points,
	int		numpts
	)

	{
	int	i;

	/* Do nothing if not there */
	if (!line || !points) return;

	/* Copy the points to the line */
	empty_line(line);
	for (i=0; i<numpts; i++)
		add_point_to_line(line, points[i]);
	}

/***********************************************************************
*                                                                      *
*      a d d _ p o i n t _ t o _ l i n e                               *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Build a line point-by-point.
 *
 *	@param[in] 	line	line to add point to
 *	@param[in] 	p		point to add to line
 *********************************************************************/

void	add_point_to_line

	(
	LINE	line,
	POINT	p
	)

	{
	int		last;
	POINT	pp;

	/* Do nothing if not there */
	if (!p) return;
	if (!line) return;

	/* Check if we need more space */
	last = line->numpts++;
	if (line->numpts > line->maxpts)
		{
		/* Protect point in case it is from the same buffer */
		copy_point(pp, p);
		line->maxpts += DELTA_POINTS;
		line->points  = GETMEM(line->points, POINT, line->maxpts);
		copy_point(line->points[last], pp);
		}
	else
		{
		copy_point(line->points[last], p);
		}

	return;
	}
