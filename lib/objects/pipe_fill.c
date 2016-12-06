/**********************************************************************/
/**	@file pipe_fill.c
 * 
 *	Fill module of graphics pipe software.
 *
 *	All points input are passed to the output.  Intermediate points
 *	are inserted in the output if the distance between adjacent
 *	input points exceeds the specified resolution.
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*     p i p e _ f i l l . c                                            *
*                                                                      *
*     Fill module of graphics pipe software.                           *
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
*       e n a b l e _ f i l l   - enable polyline filling              *
*     d i s a b l e _ f i l l   - disable polyline filling             *
*             p u t _ f i l l   - add point to filled polyline         *
*         f l u s h _ f i l l   - flush filled polyline (pen-up)       *
*                                                                      *
*     All points input are passed to the output.  Intermediate points  *
*     are inserted in the output if the distance between adjacent      *
*     input points exceeds the specified resolution.                   *
*                                                                      *
***********************************************************************/

/* Flags and shared parameters for fill module */
static LOGICAL	new = FALSE;	/* start new point stream */
static float	res = 0;		/* fill resolution */
static float	xo, yo;			/* anchor point */

/**********************************************************************/
/** Turn on polyline filling
 *
 * 	@param[in]	fres	Resolution
 **********************************************************************/
void enable_fill(float fres)
	{
	enable_module(Pfill);
	new = TRUE;
	res = fres;
	}

/** Turn off polyline filling */
void disable_fill(void)
	{
	disable_module(Pfill);
	new = FALSE;
	res = 0;
	}

/** Add point to filled polyline */
void put_fill(float x, float y)
	{
	float	s, ds ,dx, dy, r;

	/* Re-initialize at start of new line */
	if (new) new = !new;

	/* Perform intermediate output on subsequent point */
	else	{
		/* Compute arc-length */
		s  = hypot(x-xo,y-yo);

		/* Output intermediate points, if any */
		if (s > res)
			{
			r  = res/s;
			dx = (x-xo)*r;
			dy = (y-yo)*r;
			for (ds=res; ds<s; ds+=res)
				{
				xo += dx;
				yo += dy;
				put_next(Pfill,xo,yo);
				}
			}
		}

	/* Output current point and save it for the next time */
	put_next(Pfill,x,y);
	xo = x;
	yo = y;
	}

/** Flush filled polyline (pen up) */
void flush_fill(void)
	{
	new = TRUE;
	flush_next(Pfill);
	}
