/**********************************************************************/
/**	@file pipe_clip.c
 *
 *	Clip the incoming stream of points to the specified window.
 *
 *	This clipping software uses the algorithm presented in the
 *	paper "An Analysis and Algorithm for Polygon Clipping", by
 *	You-Dong Liang and Brian A. Barsky, published in 
 *	Communications of the ACM, November 1983, Volume 26, Number 11.
 *	This algorithm is reportedly more time efficient than the
 *	usual Sutherland algorithm.
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
#undef DEBUG
/***********************************************************************
*                                                                      *
*     p i p e _ c l i p . c                                            *
*                                                                      *
*     Clip module of the graphics pipe software.                       *
*                                                                      *
*     (c) Copyright 1988 Environment Canada (AES)                      *
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
#include <stdio.h>

/***********************************************************************
*                                                                      *
*       e n a b l e _ c l i p   - turn on clipping                     *
*     d i s a b l e _ c l i p   - turn off clipping                    *
*             p u t _ c l i p   - pass the given point to clipper      *
*         f l u s h _ c l i p   - end the stream (pen up)              *
*                                                                      *
*     Clip the incoming stream of points to the specified window.      *
*                                                                      *
*     This clipping software uses the algorithm presented in the       *
*     paper "An Analysis and Algorithm for Polygon Clipping", by       *
*     You-Dong Liang and Brian A. Barsky, published in Communica-      *
*     tions of the ACM, November 1983, Volume 26, Number 11.           *
*     This algorithm is reportedly more time efficient than the        *
*     usual Sutherland algorithm.                                      *
*                                                                      *
***********************************************************************/

/* Flags and shared parameters for clip module */
static LOGICAL	new   = FALSE;	/* new line */
static LOGICAL	out   = FALSE;	/* at least one point has been output */
static LOGICAL	poly  = FALSE;	/* polygon mode (include corner points) */
static LOGICAL	closd = FALSE;	/* close mode (close last to first points) */
static float	xp, yp;
static float	xq, yq;
static float	xmin, ymin;
static float	xmax, ymax;
static float	infinity = 1e10;
static LOGICAL	debug = FALSE;



/**********************************************************************/
/** Turn on clipping
 *
 * 	@param[in]	left	Left clipping edge
 * 	@param[in]	right	Right clipping edge
 * 	@param[in]	bottom	Bottom clipping edge
 * 	@param[in]	top		Top clipping edge
 * 	@param[in]	polygon	Is this a polygon?
 * 	@param[in]	closed	Is this a closed line?
***********************************************************************/
void	enable_clip

	(
	float	left,
	float	right,
	float	bottom,
	float	top,
	LOGICAL	polygon,
	LOGICAL	closed
	)

	{
	enable_module(Pclip);
	new   = TRUE;
	out   = FALSE;
	poly  = polygon;
	closd = closed;
	xmin  = MIN(left,right);
	xmax  = MAX(left,right);
	ymin  = MIN(bottom,top);
	ymax  = MAX(bottom,top);
#ifdef DEBUG
	debug = (LOGICAL) (poly && !closed);
#endif
	}



/** Turn off clipping */
void disable_clip(void)
	{
	disable_module(Pclip);
	new = FALSE;
	out = FALSE;
	}



static	void output_point(float, float);

static	void output_point(float x, float y)
	{
	/* Save first output point to join at end */
	if (!out)
		{
		out = TRUE;
		if (poly && closd)
		{
			xq  = x;
			yq  = y;
		}
		}

	/* Pass point along to next pipe operation */
	put_next(Pclip,x,y);
	}



/** Pass given point to clipper */
void	put_clip(float x, float y)
	{
	float	xx, dx, xin, xout, axin, axout;
	float	yy, dy, yin, yout, ayin, ayout;
	float	ain1, ain2, aout1;

	if (debug) (void) printf("\npoint: %f %f\n",x,y);

	/* Process first point individually */
	/* Remainder of algorithm depends on a previous point */
	if (new)
		{
		new = !new;
		if (x < xmin) goto done;
		if (x > xmax) goto done;
		if (y < ymin) goto done;
		if (y > ymax) goto done;
		output_point(x,y);
		goto done;
		}

	/* Find the x entry point */
	dx = x - xp;
	if (dx > 0)		/* line going right */
		{
		xin  = xmin;
		axin = (xin-xp)/dx;
		}
	else if (dx < 0)	/* line going left */
		{
		xin  = xmax;
		axin = (xin-xp)/dx;
		}
	else			/* vertical line */
		{
		xin  = xmax;
		axin = -infinity;
		if (xp > xmax) xin = xmin;
		}

	/* Find the y entry point */
	dy = y - yp;
	if (dy > 0)		/* line going up */
		{
		yin  = ymin;
		ayin = (yin-yp)/dy;
		}
	else if (dy < 0)	/* line going down */
		{
		yin  = ymax;
		ayin = (yin-yp)/dy;
		}
	else			/* horizontal line */
		{
		yin  = ymax;
		ayin = -infinity;
		if (yp > ymax) yin = ymin;
		}

	/* Order the two entry points */
	ain1 = MIN(axin,ayin);
	ain2 = MAX(axin,ayin);

	/* Case 1:  (1 < ain1) */
	/* No contribution whatsoever */
	if (1 < ain1)
		{
		if (debug) (void) printf("case-1\n");
		goto done;
		}

	/* Case 5:  (0 < ain1 <= 1) */
	/* Add a turning vertex due to 1st point */
	if (0 < ain1)
		{
		if (debug) (void) printf("case-5: %f %f\n",xin,yin);
		if (poly)  output_point(xin,yin);
		}

	/* Case 2:  (0 >= ain1)  and  (1 < ain2) */
	/* No contribution */
	if (1 < ain2)
		{
		if (debug) (void) printf("case-2\n");
		goto done;
		}

	/* Find the x exit point */
	if (dx > 0)
		{
		xout  = xmax;
		axout = (xout-xp)/dx;
		}
	else if (dx < 0)
		{
		xout  = xmin;
		axout = (xout-xp)/dx;
		}
	else
		{
		xout  = xmin;
		axout = infinity;
		if (xp < xmin) axout = -infinity;
		if (xp > xmax) axout = -infinity;
		if (xp > xmax) xout  = xmax;
		}

	/* Find the y exit point */
	if (dy > 0)
		{
		yout  = ymax;
		ayout = (yout-yp)/dy;
		}
	else if (dy < 0)
		{
		yout  = ymin;
		ayout = (yout-yp)/dy;
		}
	else
		{
		yout  = ymin;
		ayout = infinity;
		if (yp < ymin) ayout = -infinity;
		if (yp > ymax) ayout = -infinity;
		if (yp > ymax) yout  = ymax;
		}

	/* Order the two exit points */
	aout1 = MIN(axout,ayout);

	/* Case 3:  (0 >= ain2) and (0 >= aout1) */
	/* No contribution */
	if ((0 >= ain2) && (0 >= aout1))
		{
		if (debug) (void) printf("case-3\n");
		if (0 == aout1)
			{
			/* Terminate line segment if line mode */
			if (debug) (void) printf("going out: %f %f\n",xx,yy);
			if (!poly) flush_next(Pclip);
			}
		goto done;
		}

	/* Case 4:  (ain2 <= aout1) and (0 < aout1) and (1 >= ain1) */
	/* Visible segment - may be clipped on either end */
	if (ain2 <= aout1)
		{
		if (debug) (void) printf("case-4\n");

		/* 1st point invisible - coming in */
		if (0 < ain2)
		{
			if (axin > ayin)	/* clip to vertical */
			{
			xx = xin;
			yy = yp + axin*dy;
			}
			else			/* clip to horizontal */
			{
			xx = xp + ayin*dx;
			yy = yin;
			}
		if (debug) (void) printf("coming in: %f %f\n",xx,yy);
			output_point(xx,yy);
		}

		/* 2nd point invisible - going out */
		if (1 > aout1)
		{
		if (axout < ayout)	/* clip to vertical */
			{
			xx = xout;
			yy = yp + axout*dy;
			}
		else			/* clip to horizontal */
			{
			xx = xp + ayout*dx;
			yy = yout;
			}
		if (debug) (void) printf("going out: %f %f\n",xx,yy);
		output_point(xx,yy);

		/* Terminate line segment if line mode */
		if (!poly) flush_next(Pclip);
		}

		/* 2nd point is visible - don't clip */
		else
		{
		if (debug) (void) printf("end visible: %f %f\n",x,y);
		output_point(x,y);
		}
		}

	/* Case 6:  (0 < ain2 <= 1) and (aout1 < ain2) */
	/* Add turning vertex due to 2nd point */
	else if (poly)
		{
		if (axin > ayin)
		{
		if (debug) (void) printf("case-6: %f %f\n",xin,yout);
			output_point(xin,yout);
		}
		else
		{
		if (debug) (void) printf("case-6: %f %f\n",xout,yin);
			output_point(xout,yin);
		}
		}

	/* Save current point (un-clipped) */
	done:
	xp  = x;
	yp  = y;
	}



/** End the stream (Pen up) */
void flush_clip(void)
	{
	/* Force polygon to join if needed */
	if (poly && closd)
		{
		if (out) put_next(Pclip,xq,yq);
		}

	/* Reset new-line flag and flush buffer */
	flush_next(Pclip);
	new = TRUE;
	out = FALSE;
	}
