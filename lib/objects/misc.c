/*********************************************************************/
/** @file misc.c
 *
 * Routines to handle Miscellaneous objects
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*      m i s c . c                                                     *
*                                                                      *
*      Routines to handle Miscellaneous objects.                       *
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

#define MISC_INIT
#include "misc.h"

#include <tools/tools.h>
#include <fpa_getmem.h>
#include <fpa_math.h>
#include <string.h>

/***********************************************************************
*                                                                      *
*      c o p y _ p o i n t                                             *
*      s e t _ p o i n t                                               *
*      p o i n t _ d i s t                                             *
*      p o i n t _ d i s t 2                                           *
*      m a k e _ p o i n t                                             *
*      m a k e _ p l i s t                                             *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/**	Copy the contents of p2 to p1.
 *
 *	@param[out]	p1		copy
 *	@param[in] 	p2		original
 *********************************************************************/
void	copy_point

	(
	POINT		p1,
	const POINT	p2
	)

	{
	fpalib_verify(FpaAccessRead);

	if (!p1) return;
	if (!p2) return;

	p1[X] = p2[X];
	p1[Y] = p2[Y];
	}

/*********************************************************************/
/** Assign the given x-y value to the given point.
 *
 *	@param[in] 	p	Point to set
 *	@param[in] 	x	x coord to set
 *	@param[in] 	y	y coord to set
 *********************************************************************/
void	set_point

	(
	POINT	p,
	float	x,
	float	y
	)

	{
	fpalib_verify(FpaAccessRead);

	if (!p) return;

	p[X] = x;
	p[Y] = y;
	}

/*********************************************************************/
/** Calculate the distance between two points.
 *
 *	@param[in] 	pa	first point
 *	@param[in] 	pb	second point
 *  @return Distance between @f${p_a}@f$ and @f${p_b}@f$.
 *********************************************************************/
float	point_dist

	(
	POINT	pa,
	POINT	pb
	)

	{
	float	dx, dy;

	if (!pa || !pb) return 0.0;

	dx = pb[X] - pa[X];
	dy = pb[Y] - pa[Y];
	return hypot(dx, dy);
	}

/*********************************************************************/
/** Calculate the square of the distance between two points.
 *
 *	@param[in] 	pa	first point
 *	@param[in] 	pb	second point
 *  @return Square of the distance between @f${p_a}@f$ and @f${p_b}@f$.
 *********************************************************************/
float	point_dist2

	(
	POINT	pa,
	POINT	pb
	)

	{
	float	dx, dy;

	if (!pa || !pb) return 0.0;

	dx = pb[X] - pa[X];
	dy = pb[Y] - pa[Y];
	return (dx*dx + dy*dy);
	}

/*********************************************************************/
/** Construct a point with the given x-y value.
 *
 * @note Be careful with this function. An internal static
 * variable is returned, and is overwritten upon a subsequent call.
 *
 *	@param[in] 	x	x coord
 *	@param[in] 	y	y coord
 * 	@return A pair of float values for x-y coordinates.
 *********************************************************************/
float	*make_point

	(
	float	x,
	float	y
	)

	{
	static	POINT	p;

	set_point(p, x, y);
	return (float *) p;
	}

/*********************************************************************/
/** Construct a point array with one member from the given point.
 *
 * @note Be careful with this function. An internal static
 * variable is returned, and is overwritten upon a subsequent call.
 *
 *	@param[in] 	p	point to add to list
 *  @return Pointer to a list of points. If the point p does not exist
 * 			then the list contains the zero point.
 *********************************************************************/
POINT	*make_plist

	(
	const POINT	p
	)

	{
	static	POINT	pl[1];

	if (p) copy_point(pl[0], p);
	else   copy_point(pl[0], ZeroPoint);
	return pl;
	}

/***********************************************************************
*                                                                      *
*      a d d _ c o m p o n e n t                                       *
*      n e e d _ c o m p o n e n t                                     *
*      h a v e _ c o m p o n e n t                                     *
*      r e a d y _ c o m p o n e n t s                                 *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Check if a component is needed if it is add it.
 *
 *	@param[in] 	*info	component info
 *	@param[in] 	comp	component
 *  @return True if added.
 *********************************************************************/
LOGICAL	add_component

	(
	COMP_INFO		*info,
	const COMPONENT	comp
	)

	{
	/* See if we care about this component */
	/* Can't add it if we don't need it */
	if (!need_component(info, comp)) return FALSE;

	/* Add the component */
	info->have |= (info->need & comp);
	return TRUE;
	}

/**********************************************************************/

/*********************************************************************/
/** Check if a component is needed.
 *
 *	@param[in] 	*info	component info
 *	@param[in] 	comp	component
 *********************************************************************/
LOGICAL	need_component

	(
	const COMP_INFO	*info,
	const COMPONENT	comp
	)

	{
	if (!info) return FALSE;

	/* Make sure we have a legal component */
	switch (comp)
		{
		case X_Comp:	break;
		case Y_Comp:	break;
		default:		return FALSE;
		}

	/* See if we care about this component */
	return  (info->need & comp)? TRUE: FALSE;
	}

/**********************************************************************/

/*********************************************************************/
/** Check if a component is present.
 *
 *	@param[in] 	*info	component info
 *	@param[in] 	comp	component
 *  @return True if present.
 *********************************************************************/
LOGICAL	have_component

	(
	const COMP_INFO	*info,
	const COMPONENT	comp
	)

	{
	/* See if we care about this component */
	/* Can't have it if we don't need it */
	if (!need_component(info, comp)) return FALSE;

	/* See if we have this component */
	return  (info->have & comp)? TRUE: FALSE;
	}

/**********************************************************************/

/*********************************************************************/
/** check if components are ready.
 *
 *	@param[in] 	*info	component info
 *  @return True if ready.
 *********************************************************************/
LOGICAL	ready_components

	(
	const COMP_INFO	*info
	)

	{
	if (!info) return FALSE;

	return ((info->need & info->have) == info->need)? TRUE: FALSE;
	}

/***********************************************************************
*                                                                      *
*      c o p y _ b o x                                                 *
*      i n s i d e _ b o x                                             *
*      i n s i d e _ b o x _ x y                                       *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Make a copy of a box object.
 *
 *	@param[in] 	*b1	copy
 *	@param[in] 	*b2 original
 *********************************************************************/
void	copy_box

	(
	BOX			*b1,
	const BOX	*b2
	)

	{
	if (!b1) return;
	if (!b2) return;

	b1->left   = b2->left;
	b1->right  = b2->right;
	b1->top    = b2->top;
	b1->bottom = b2->bottom;
	}

/*********************************************************************/
/** Check if a point is inside the given box
 *
 *	@param[in] 	*b	given box
 *	@param[in] 	p	given point
 *  @return True if point is inside the box.
 *********************************************************************/
LOGICAL	inside_box

	(
	const BOX	*b,
	const POINT	p
	)

	{
	if (!b) return FALSE;
	if (!p) return FALSE;

	return inside_box_xy(b, p[X], p[Y]);
	}

/*********************************************************************/
/** Check if an x-y pair is inside the given box.
 *
 *	@param[in]  *b	given box
 *	@param[in] 	x	x coord
 *	@param[in] 	y	y coord
 *  @return True if x-y pair is inside box.
 *********************************************************************/
LOGICAL	inside_box_xy

	(
	const BOX	*b,
	float		x,
	float		y
	)

	{
	if (!b) return FALSE;

	if (x < b->left)   return FALSE;
	if (x > b->right)  return FALSE;
	if (y < b->bottom) return FALSE;
	if (y > b->top)    return FALSE;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*                c o p y _ x f o r m                                   *
*              b u i l d _ x f o r m                                   *
*              b l o c k _ x f o r m                                   *
*      t r a n s l a t e _ x f o r m                                   *
*            r o t a t e _ x f o r m                                   *
*              s c a l e _ x f o r m                                   *
*                                                                      *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Copy a 2-D transform
 *
 *	@param[out]	x1	Transform copy
 *	@param[in] 	x2	Transform original
 *********************************************************************/
void	copy_xform

	(
	XFORM		x1,
	const XFORM	x2
	)

	{
	if (!x1) return;
	if (!x2) return;

	x1[X][X] = x2[X][X];
	x1[X][Y] = x2[X][Y];
	x1[Y][X] = x2[Y][X];
	x1[Y][Y] = x2[Y][Y];
	x1[H][X] = x2[H][X];
	x1[H][Y] = x2[H][Y];
	}

/*********************************************************************/
/** Construct a 2-D transform in terms of the given scale, translation
 * and rotation.
 *
 *	@param[out]	xform	transform to compute
 *	@param[in] 	sx		scale x factor
 *	@param[in] 	sy		scale y factor
 *	@param[in] 	tx		x translation
 *	@param[in] 	ty		y translation
 *	@param[in] 	angle	rotation
 *********************************************************************/
void	build_xform

	(
	XFORM	xform,
	float	sx,
	float	sy,
	float	tx,
	float	ty,
	float	angle
	)

	{
	float	cang, sang;

	if (!xform) return;

	cang = cos(RAD*angle);
	sang = sin(RAD*angle);
	xform[X][X] = sx*cang;
	xform[X][Y] = sx*sang;
	xform[Y][X] = -sy*sang;
	xform[Y][Y] = sy*cang;
	xform[H][X] = tx;
	xform[H][Y] = ty;
	}

/*********************************************************************/
/** Transform point by multiplying it by a transformation matrix
 *
 *	@param[in] 	xform	Transform to compute
 *	@param[in] 	pin		Input point
 *	@param[out] pout	Output point
 *********************************************************************/
void	transform_point

	(
	XFORM	xform,
	POINT	pin,
	POINT	pout
	)
	{
	pout[X] = xform[X][X] * pin[X] + xform[Y][X] * pin[Y] + xform[H][X];
	pout[Y] = xform[X][Y] * pin[X] + xform[Y][Y] * pin[Y] + xform[H][Y];
	}
/*********************************************************************/
/** Premultiply XFORM with translation matix return answer in same
 *	matrix.
 *
 *	@param[in/out] 	xform	Transform to compute
 *	@param[in]  	tx		x translation
 *	@param[in] 		ty 		y translation
 *********************************************************************/
void	translate_xform

	(
	XFORM	xform,
	float	tx,
	float	ty
	)
	{
	xform[H][X] += tx;
	xform[H][Y] += ty;
	}

/*********************************************************************/
/** Premultiply XFORM with rotation matix return answer in same
 *	matrix.
 *
 *	@param[in/out] 	xform	Transform to compute
 *	@param[in]  	pivot	centre of rotation
 *	@param[in] 		angle 	rotation
 *********************************************************************/
void	rotate_xform

	(
	XFORM	xform,
	POINT	pivot,
	float	angle
	)
	{
	float 	sang, cang;
	XFORM	m;
	XFORM  	tmp;

	copy_xform(m, IdentXform);
	copy_xform(tmp, IdentXform);

	cang = cos(RAD*angle);
	sang = sin(RAD*angle);

	/* Transformation for rotation about a pivot point */
	m[X][X] = cang;
	m[X][Y] = sang;
	m[Y][X] = -sang;
	m[Y][Y] = cang;
	m[H][X] = pivot[X] * (1-cang) + pivot[Y]*sang;
	m[H][Y] = pivot[Y] * (1-cang) - pivot[X]*sang;

	tmp[X][X] = m[X][X] * xform[X][X] + m[Y][X] * xform[X][Y];
	tmp[X][Y] = m[X][Y] * xform[X][X] + m[Y][Y] * xform[X][Y];
	tmp[Y][X] = m[X][X] * xform[Y][X] + m[Y][X] * xform[Y][Y];
	tmp[Y][Y] = m[X][Y] * xform[Y][X] + m[Y][Y] * xform[Y][Y];
	tmp[H][X] = m[X][X] * xform[H][X] + m[Y][X] * xform[H][Y] + m[H][X];
	tmp[H][Y] = m[X][Y] * xform[H][X] + m[Y][Y] * xform[H][Y] + m[H][Y];

	copy_xform(xform, tmp);
	}
/*********************************************************************/
/** Premultiply XFORM with scale matix return answer in same
 *	matrix.
 *
 *	@param[in/out] 	xform	Transform to compute
 *	@param[in]  	point	fixed-point to scale from scale
 *	@param[in] 		sx	 	x scale
 *	@param[in] 		sy	 	y scale 
 *********************************************************************/
void	scale_xform

	(
	XFORM	xform,
	POINT	point,
	float	sx,
	float	sy
	)
	{
	XFORM	m;
	XFORM  	tmp;

	copy_xform(m, IdentXform);
	copy_xform(tmp, IdentXform);

	/* Transformation for rotation about a point point */
	m[X][X] = sx;
	m[Y][Y] = sy;
	m[H][X] = point[X] * ( 1 - sx );
	m[H][Y] = point[Y] * ( 1 - sy );

	tmp[X][X] = m[X][X] * xform[X][X];
	tmp[X][Y] = m[Y][Y] * xform[X][Y];
	tmp[Y][X] = m[X][X] * xform[Y][X];
	tmp[Y][Y] = m[Y][Y] * xform[Y][Y];
	tmp[H][X] = m[X][X] * xform[H][X] + m[H][X];
	tmp[H][Y] = m[Y][Y] * xform[H][Y] + m[H][Y];

	copy_xform(xform, tmp);
	}

/*********************************************************************/
/** Construct a 2-D transform based on the relationship between the
 * given viewport and window.
 *
 *	@param[in] 	xform		Transform to compute
 *	@param[in] 	*viewport	Viewport definition
 *	@param[in] 	*window 	Window definition
 *********************************************************************/
void	block_xform

	(
	XFORM			xform,
	const BOX		*viewport,
	const BOX		*window
	)

	{
	float	dwx, dwy, dvx, dvy;
	float	sx, sy, s, gapx, gapy, tx, ty;

	if (!xform)    return;
	if (!viewport) return;
	if (!window)   return;

	/* Compute lengths */
	dwx = window->right   - window->left;
	dwy = window->top     - window->bottom;
	dvx = viewport->right - viewport->left;
	dvy = viewport->top   - viewport->bottom;

	/* Compute scale factor (isotropic) */
	sx = dvx/dwx;
	sy = dvy/dwy;
	s  = MIN(sx, sy);

	/* Compute translation including half of gap */
	gapx = dvx - s*dwx;
	gapy = dvy - s*dwy;
	tx   = 0.5*gapx - s*window->left   + viewport->left;
	ty   = 0.5*gapy - s*window->bottom + viewport->bottom;

	/* Construct the transform */
	build_xform(xform, s, s, tx, ty, 0.0);
	}

/***********************************************************************
*                                                                      *
*   f i n d _ i t b l _ e n t r y                                      *
*   f i n d _ i t b l _ s t r i n g                                    *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Find a given entry in an indexed table structure by name.
 *
 *	@param[in] 	*tbl	Table to search
 *	@param[in] 	num	 	Number of elements in Tabel
 *	@param[in] 	name	name of element to search for
 *  @return Index of entry in table. <0 if not there.
 *********************************************************************/
int		find_itbl_entry

	(
	const ITBL		(*tbl),
	const int		num,
	const STRING	name
	)

	{
	int	i;

	if (!tbl) return -1;

	for (i=0; i<num; i++)
		{
		if (same_ic(name, tbl[i].name))
			return tbl[i].ival;
		}

	return -1;
	}

/**********************************************************************/

/*********************************************************************/
/** Find a given entry in an indexed table structure by value.
 *
 *	@param[in] 	*tbl	Table to search
 *	@param[in]   num		number of entries in the table
 *	@param[in] 	ival	value to search for
 * 	@return Pointer to the name element in the entry that matches the
 * 			given value.
 *********************************************************************/
STRING	find_itbl_string

	(
	const ITBL      (*tbl),
	const int       num,
	const int		ival
	)

	{
	int	i;

	if (!tbl) return NULL;

	for (i=0; i<num; i++)
		{
		if (ival == tbl[i].ival)
			return tbl[i].name;
		}

	return NULL;
	}

/***********************************************************************
*
*   r e a d _ r a n g e
*   t e s t _ r a n g e
*   c o p y _ r a n g e
*   s a m e _ r a n g e
*   s u b _ r a n g e
*
***********************************************************************/

/*********************************************************************/
/** Read a numerical range from a string.
 *
 * Accepted formats for read_range:
 * 	- [min,max]   ...   min <= x <= max
 * 	- (min,max]   ...   min <  x <= max
 * 	- [min,max)   ...   min <= x <  max
 * 	- (min,max)   ...   min <  x <  max
 *
 * White space may surround the brackets and the comma.  A colon (:)
 * may be used instead of the comma.  An asterisk in place of max or
 * min is interpreted as plus or minus infinity.
 *
 *	@param[in] 	string	String to parse
 *	@param[out]	*range	Return range in string
 *  @return True if successful.
 *********************************************************************/
LOGICAL	read_range
	(
	STRING	string,
	RANGE	*range
	)

	{
	double	val;
	STRING	cp, ap;
	LOGICAL	unlim;

	if (IsNull(range))
		{
		return FALSE;
		}
	copy_range(range, &UnlimitedRange);
	if (blank(string))
		{
		return FALSE;
		}

	/* Skip leading whitespace */
	cp = string + strspn(string, " \t\n\r\f");

	/* First character must be an opening round or square bracket */
	switch (*cp)
		{
		case '[':	range->mincon = RangeIncl;
					break;
		case '(':	range->mincon = RangeExcl;
					break;
		default:	return FALSE;
		}
	cp++;

	/* Skip whitespace */
	cp += strspn(cp, " \t\n\r\f");

	/* Now we must either have an asterisk or a number */
	if (*cp == '*')
		{
		unlim = TRUE;
		cp++;
		}
	else
		{
		/* Now we must have a number (see strtod) */
		unlim = FALSE;
		val = strtod(cp, &ap);
		if (ap == cp)
			{
			return FALSE;
			}
		range->minval = val;
		cp = ap;
		}
	if (unlim) range->mincon = RangeUnlim;

	/* Skip whitespace */
	cp += strspn(cp, " \t\n\r\f");

	/* Now we must have a comma or colon */
	switch (*cp)
		{
		case ',':	break;
		case ':':	break;
		default:	return FALSE;
		}
	cp++;

	/* Skip whitespace */
	cp += strspn(cp, " \t\n\r\f");

	/* Now we must either have an asterisk or a number */
	if (*cp == '*')
		{
		unlim = TRUE;
		cp++;
		}
	else
		{
		/* Now we must have a number (see strtod) */
		unlim = FALSE;
		val = strtod(cp, &ap);
		if (ap == cp)
			{
			return FALSE;
			}
		range->maxval = val;
		cp = ap;
		}

	/* Skip whitespace */
	cp += strspn(cp, " \t\n\r\f");

	/* Next character must be a closing round or square bracket */
	switch (*cp)
		{
		case ']':	range->maxcon = RangeIncl;
					break;
		case ')':	range->maxcon = RangeExcl;
					break;
		default:	return FALSE;
		}
	cp++;
	if (unlim) range->maxcon = RangeUnlim;

	/* Now there must be nothing left but whitespace */
	if (!blank(cp))
		{
		return FALSE;
		}

	/* What do you know? It worked. */
	return TRUE;
	}

/**********************************************************************/

/*********************************************************************/
/** Determine if a number is in a range.
 *
 *	@param[in] 	val		number to check
 *	@param[in] 	*range	range to check it against
 *  @return True if the number is in the range.
 *********************************************************************/
LOGICAL	test_range
	(
	double	val,
	RANGE	*range
	)

	{
	if (IsNull(range)) return FALSE;

	switch (range->mincon)
		{
		case RangeUnlim:	break;

		case RangeIncl:		if (val < range->minval) return FALSE;
							break;

		case RangeExcl:		if (val <= range->minval) return FALSE;
							break;

		default:			return FALSE;
		}

	switch (range->maxcon)
		{
		case RangeUnlim:	break;

		case RangeIncl:		if (val > range->maxval) return FALSE;
							break;

		case RangeExcl:		if (val >= range->maxval) return FALSE;
							break;

		default:			return FALSE;
		}

	return TRUE;
	}

/**********************************************************************/

/*********************************************************************/
/** Copy a range.
 *
 *	@param[out]	*r1	copy of range
 *	@param[in] 	*r2	original range
 *********************************************************************/
void	copy_range

	(
	RANGE		*r1,
	const RANGE	*r2
	)

	{
	if (!r1) return;
	if (!r2) return;

	r1->minval = r2->minval;
	r1->mincon = r2->mincon;
	r1->maxval = r2->maxval;
	r1->maxcon = r2->maxcon;
	}

/**********************************************************************/

/*********************************************************************/
/** Test if two ranges match.
 *
 *	@param[in] 	*r1	First range to compare
 *	@param[in] 	*r2	Second range to compare
 *  @return True if the ranges are the same.
 *********************************************************************/
LOGICAL	same_range

	(
	const RANGE	*r1,
	const RANGE	*r2
	)

	{
	if (!r1 && !r2) return TRUE;
	if (!r1)        return FALSE;
	if (!r2)        return FALSE;

	if (r1->mincon != r2->mincon) return FALSE;
	if (r1->maxcon != r2->maxcon) return FALSE;

	switch (r1->mincon)
		{
		case RangeUnlim:	break;

		case RangeIncl:
		case RangeExcl:		if (r1->minval != r2->minval) return FALSE;
							break;

		default:			return FALSE;
		}

	switch (r1->maxcon)
		{
		case RangeUnlim:	break;

		case RangeIncl:
		case RangeExcl:		if (r1->maxval != r2->maxval) return FALSE;
							break;

		default:			return FALSE;
		}

	return TRUE;
	}

/**********************************************************************/

/*********************************************************************/
/** Test if the 1st range contains the 2nd.
 *
 *	@param[in] 	*r1	Range
 *	@param[in] 	*r2	possible sub-range
 *  @return True if r1 contains r2
 *********************************************************************/
LOGICAL	sub_range

	(
	const RANGE	*r1,
	const RANGE	*r2
	/* Does r1 contain r2? */
	)

	{
	if (!r1 && !r2) return TRUE;
	if (!r1)        return FALSE;
	if (!r2)        return FALSE;

	switch (r1->mincon)
		{
		case RangeUnlim:	break;

		case RangeIncl:
			switch (r2->mincon)
				{
				case RangeUnlim:	return FALSE;
				case RangeIncl:		if (r2->minval <  r1->minval) return FALSE;
									break;
				case RangeExcl:		if (r2->minval <  r1->minval) return FALSE;
									break;
				default:			return FALSE;
				}
			break;

		case RangeExcl:
			switch (r2->mincon)
				{
				case RangeUnlim:	return FALSE;
				case RangeIncl:		if (r2->minval <= r1->minval) return FALSE;
									break;
				case RangeExcl:		if (r2->minval <  r1->minval) return FALSE;
									break;
				default:			return FALSE;
				}
			break;

		default:			return FALSE;
		}

	switch (r1->maxcon)
		{
		case RangeUnlim:	break;

		case RangeIncl:
			switch (r2->maxcon)
				{
				case RangeUnlim:	return FALSE;
				case RangeIncl:		if (r2->maxval >  r1->maxval) return FALSE;
									break;
				case RangeExcl:		if (r2->maxval >  r1->maxval) return FALSE;
									break;
				default:			return FALSE;
				}
			break;

		case RangeExcl:
			switch (r2->maxcon)
				{
				case RangeUnlim:	return FALSE;
				case RangeIncl:		if (r2->maxval >= r1->maxval) return FALSE;
									break;
				case RangeExcl:		if (r2->maxval >  r1->maxval) return FALSE;
									break;
				default:			return FALSE;
				}
			break;

		default:			return FALSE;
		}

	return TRUE;
	}


#ifdef STANDALONE

/***********************************************************************
*                                                                      *
*     Stand-alone test program.                                        *
*                                                                      *
***********************************************************************/

void	main(void)
	{
	char	buf[50];
	RANGE	range;
	double	value;
	LOGICAL	valid;

	while (TRUE)
		{
		(void) printf("\n");
		(void) printf("Range:\t");
		getfileline(stdin, buf, sizeof(buf));
		if (blank(buf)) break;

		valid = read_range(buf, &range);
		if (!valid)
			{
			(void) printf("\tINVALID!\n");
			continue;
			}

		(void) printf("\n");
		(void) printf("\tMin:\t%g", range.minval);
		switch(range.mincon)
			{
			case RangeUnlim:	(void) printf(" Unlimited\n"); break;
			case RangeIncl:		(void) printf(" Inclusive\n"); break;
			case RangeExcl:		(void) printf(" Exclusive\n"); break;
			}

		(void) printf("\tMax:\t%g", range.maxval);
		switch(range.maxcon)
			{
			case RangeUnlim:	(void) printf(" Unlimited\n"); break;
			case RangeIncl:		(void) printf(" Inclusive\n"); break;
			case RangeExcl:		(void) printf(" Exclusive\n"); break;
			}

		while (TRUE)
			{
			(void) printf("\n");
			(void) printf("\tValue:\t");
			getfileline(stdin, buf, sizeof(buf));
			if (blank(buf)) break;

			value = double_arg(buf, &valid);
			if (!valid)
				{
				(void) printf("\t\tMust be a number!\n");
				continue;
				}

			valid = test_range(value, &range);
			if (valid) (void) printf("\t\t%g is Inside\n", value);
			else       (void) printf("\t\t%g is Outside\n", value);
			}
		}

	}
#endif /* STANDALONE */
