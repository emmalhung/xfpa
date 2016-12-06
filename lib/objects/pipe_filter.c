/**********************************************************************/
/**	@file pipe_filter.c
 *
 *	Filter module of graphics pipe.
 *
 *	The first and last points are always output.  Intermediate
 *	points are output only if the distance (accumulated arc-length)
 *	from the previous output point exceeds the specified limit.
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*     p i p e _ f i l t e r . c                                        *
*                                                                      *
*     Filter module of graphics pipe software.                         *
*                                                                      *
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

#include "pipeP.h"
#include <fpa_math.h>

/***********************************************************************
*                                                                      *
*       e n a b l e _ f i l t e r   - enable polyline filtering        *
*     d i s a b l e _ f i l t e r   - disable polyline filtering       *
*             p u t _ f i l t e r   - add point to filtered polyline   *
*         f l u s h _ f i l t e r   - flush filtered polyline (pen-up) *
*                                                                      *
*     The first and last points are always output.  Intermediate       *
*     points are output only if the distance (accumulated arc-length)  *
*     from the previous output point exceeds the specified limit.      *
*                                                                      *
***********************************************************************/

/* Flags and shared parameters for filter module */
static LOGICAL	new = FALSE;	/* start new point stream */
static LOGICAL	out = FALSE;	/* has a point been output yet? */
static float	res = 0;		/* filter resolution */
static float	ang = 0;		/* filter angle */
static float	s   = 0;		/* accumulated arc length from anchor point */
static float	a   = 0;		/* accumulated angle from anchor point */
static float	ao  = 0;		/* anchor angle */
static float	xo, yo;			/* anchor point */
static float	xp, yp;			/* previous point recalled */

/**********************************************************************/
/** Turn on polyline filtering 
 *
 * 	@param[in]	fres	Distance resolution
 * 	@param[in]	fang	Angle resolution
 **********************************************************************/
void enable_filter(float fres, float fang)
	{
	enable_module(Pfilter);
	new = TRUE;
	out = FALSE;
	res = fres;
	ang = fang;
	}

/** Turn off polyline filtering */
void disable_filter(void)
	{
	disable_module(Pfilter);
	new = FALSE;
	out = FALSE;
	res = 0;
	}

/** Add point to filtered polyline */
void put_filter(float x, float y)
	{
	/* Re-initialize at start of new line */
	if (new)
		{
		new = FALSE;
		out = FALSE;
		}

	/* Perform arc-length filter on subsequent points */
	else
		{
		/* Compute accumulated arc-length */
		if (x==xp && x==yp) return;
		if (res>0) s += hypot(x-xp,y-yp);
		if (out && ang>0)
			{
			a = -ao;
			if (fabs(y-yo)-fabs(x-xo) > (fabs(x)+fabs(y))/100000.0)
				a += atan2(y-yo,x-xo);
			a = fmod(a, PI);
			a = fabs(a);
			}

		/* If accumulated arc-length is less than resolution, */
		/* and accumulated angle is less than resolution, */
		/* retain anchor point and replace previous point */
		if ((ang<=0 || a<=ang) && (res<=0 || s<=res))
			{
			xp = x;
			yp = y;
			return;
			}

		/* Otherwise, pass anchor point on to buffer */
		else
			{
			out = TRUE;
			put_next(Pfilter,xo,yo);
			}
		}

	/* Reset current arc-length, reset anchor point and */
	/* save current point */
	s  = 0;
	a  = 0;
	ao = 0;
	if (out && (fabs(y-yo)-fabs(x-xo) > (fabs(x)+fabs(y))/100000.0))
		ao = atan2(y-yo,x-xo);
	xo = x;
	yo = y;
	xp = x;
	yp = y;
	}

/** Flush filtered polyline (pen up) */
void flush_filter(void)
	{
	float	ds;

	/* Pass last point on to buffer and flush line */
	if (!new)
		{
		if (!out)
			{
			ds  = 100 * hypot(xp-xo,yp-yo);
			if (ds < res)
				{
				new = TRUE;
				out = FALSE;
				flush_next(Pfilter);
				return;
				}
			put_next(Pfilter,xo,yo);
			}
		put_next(Pfilter,xp,yp);
		}
	new = TRUE;
	out = FALSE;
	flush_next(Pfilter);
	}
