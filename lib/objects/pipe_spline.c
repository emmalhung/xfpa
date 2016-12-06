/**********************************************************************/
/**	@file pipe_spline.c
 *
 *	Spline module of graphics pipe.
 *
 *	Fit the incoming stream of points with a 2-dimensional parametric 
 *	cubic spline for open or closed curves, with or without tension.
 *
 *	The cubic which interpolates any 2 adjacent input points is
 *	computed as a linear weighting of two quadratic curves:
 *	- one which interpolates the 2 points plus the one to the left
 *	- the other which interpolates the 2 points plus the one to
 *	the right, as in:
 *
 *	@f[(x,y) = W_L(s)*(A_L*s^2 + B_L*s + C_L)
 *	+ W_R(s)*(A_R*s^2 + B_R*s + C_R) @f]
 *
 *	This guarantees continuity of value and slope at the input
 *	points, and can easily be shown to be identical to the
 *	conventional solution (found by tri-diagonal matrix solution).
 *
 *	The special case of end points of an open contour is handled
 *	quite readily.  At the left end, the left-hand quadratic is
 *	allowed to degenerate to a straight line, so that a point to
 *	the left is not needed.  At the right end, the right-hand
 *	quadratic is allowed to degenerate to a straight line, so that
 *	a point to the right is not needed.  This is equivalent to the
 *	so-called "natural" boundary condition, where the second
 *	derivative is forced to zero at the end points.
 *
 *	The advantage of this algorithm is that the input points can
 *	be processed incrementally, since only a running window of 4
 *	input points is needed.  The conventional matrix solution
 *	requires the entire contour to be processed at once.
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*     p i p e _ s p l i n e . c                                        *
*                                                                      *
*     Spline module of the graphics pipe software.                     *
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
#include <fpa_math.h>
/***********************************************************************
*                                                                      *
*       e n a b l e _ s p l i n e   - turn on spline                   *
*     d i s a b l e _ s p l i n e   - turn off spline                  *
*             p u t _ s p l i n e   - add the given point to spline    *
*         f l u s h _ s p l i n e   - end the stream (close if spec)   *
*                                                                      *
*     Fit the incoming stream of points with a 2-dimensional para-     *
*     metric cubic spline for open or closed curves, with or without   *
*     tension.                                                         *
*                                                                      *
*     The cubic which interpolates any 2 adjacent input points is      *
*     computed as a linear weighting of two quadratic curves:          *
*     - one which interpolates the 2 points plus the one to the left   *
*     - the other which interpolates the 2 points plus the one to      *
*       the right, as in:                                              *
*                                                                      *
*              (x,y) = WL(s)*(AL*s*s + BL*s + CL)                      *
*                    + WR(s)*(AR*s*s + BR*s + CR)                      *
*                                                                      *
*     This guarantees continuity of value and slope at the input       *
*     points, and can easily be shown to be identical to the           *
*     conventional solution (found by tri-diagonal matrix solution).   *
*                                                                      *
*     The special case of end points of an open contour is handled     *
*     quite readily.  At the left end, the left-hand quadratic is      *
*     allowed to degenerate to a straight line, so that a point to     *
*     the left is not needed.  At the right end, the right-hand        *
*     quadratic is allowed to degenerate to a straight line, so that   *
*     a point to the right is not needed.  This is equivalent to the   *
*     so-called "natural" boundary condition, where the second         *
*     derivative is forced to zero at the end points.                  *
*                                                                      *
*     The advantage of this algorithm is that the input points can     *
*     be processed incrementally, since only a running window of 4     *
*     input points in needed.  The conventional matrix solution        *
*     requires the entire contour to be processed at once.             *
*                                                                      *
***********************************************************************/

/* Flags and shared parameters for spline module */
static	float	ds, tau, per, amp, eps;
static	LOGICAL	clsd, saved=FALSE;
static	int		np = 0;
static	float	s  = 0;
static	float	xfirst, xsecond, xthird, xclose;
static	float	yfirst, ysecond, ythird, yclose;
static	float	xa,ya,sa, txa,tya,tsa;
static	float	xb,yb,sb, txb,tyb,tsb;
static	float	xc,yc,sc, txc,tyc,tsc;
static	float	axl,bxl,cxl,dxl, ayl,byl,cyl,dyl;
static	float	axr,bxr,cxr,dxr, ayr,byr,cyr,dyr;

static	void	shift_segment(void);
static	void	spline_segment(void);
static	void	do_tension(float, float, float, float, float *, float *,
						float *);
static	void	define_scallop(float, float, float, float, float, float);
static	void	do_scallop(float *, float *, float);



/**********************************************************************/
/** Turn on spline
 * 	@param[in]	resolution	Distance between output points
 *	@param[in]	closed		Close curve
 *	@param[in]	tension		Tension parameter
 *	@param[in]	period		Period of scallop
 *	@param[in]	amplitude	Amplitude of scallop
 **********************************************************************/
void	enable_spline

	(
	float	resolution,	/* distance between output points */
	LOGICAL	closed,		/* Is curve to be closed */
	float	tension,	/* tension parameter */
	float	period,		/* period of scallop */
	float	amplitude	/* amplitude of scallop */
	)

	{
	/* Enable the module */
	enable_module(Pspline);

	/* Set appropriate parameters */
	ds    = resolution;
	clsd  = closed;
	tau   = tension;
	per   = period;
	amp   = amplitude;
	eps   = ds/10;
	np    = 0;
	s     = 0;
	saved = FALSE;
	}



/** Turn off spline */
void	disable_spline(void)
	{
	disable_module(Pspline);
	}



/** Add given point to spline */
void	put_spline(float x, float y)
	{
	/* Count points */
	np++;

	/* Save first point and do tension transformation */
	if (np == 1)
		{
		xa = x;
		ya = y;
		sa = 0;
		do_tension(tau,xa,ya,sa,&txa,&tya,&tsa);

		/* Save first point on closed contours for closure condition */
		if (clsd)
			{
			xfirst = x;
			yfirst = y;
			}
		else
			{
			s = sa;
			define_scallop(xa,ya,sa,ds,per,amp);
			}
		return;
		}

	/* Save second point and do tension transformation */
	else if (np == 2)
		{
		xb = x;
		yb = y;
		sb = sa + hypot((xb-xa),(yb-ya));
		if ((sb-sa) < eps) { np--; return; }
		do_tension(tau,xb,yb,sb,&txb,&tyb,&tsb);

		/* Compute linear-parametric coefficients on left-hand segment */
		/* dxl and dxr are needed for closed or open contours */
		dxl = (txb-txa) / (tsb-tsa);	dyl = (tyb-tya) / (tsb-tsa);
		axl = 0;				ayl = 0;
		bxl = dxl;				byl = dyl;
		cxl = txa - tsa*bxl;		cyl = tya - tsa*byl;

		/* Save second point on closed contours for closure condition */
		if (clsd)
			{
			s       = sb;
			xsecond = x;
			ysecond = y;
			define_scallop(xb,yb,sb,ds,per,amp);
			}
		return;
		}

	/* Save third and subsequent points and do tension transformation */
	xc = x;
	yc = y;
	sc = sb + hypot((xc-xb),(yc-yb));
	if ((sc-sb) < eps) { np--; return; }
	do_tension(tau,xc,yc,sc,&txc,&tyc,&tsc);

	/* Compute quadratic-parametric coefficients on right-hand segment */
	dxr = (txc-txb) / (tsc-tsb);		dyr = (tyc-tyb) / (tsc-tsb);
	axr = (dxr-dxl) / (tsc-tsa);		ayr = (dyr-dyl) / (tsc-tsa);
	bxr = dxl - axr*(tsb+tsa);		byr = dyl - ayr*(tsb+tsa);
	cxr = txa - tsa*(bxr+tsa*axr);		cyr = tya - tsa*(byr+tsa*ayr);

	/* Save third point of closed contours for closure condition */
	if ((np == 3) && clsd)
		{
		xthird = x;
		ythird = y;
		}

	/* Generate points throughout the segment overlap */
	else spline_segment();

	/* Shift right-hand segment into left for next call */
	shift_segment();
	}


static	void	spline_segment(void)
	{
	float	xp, yp, wl, wr, twl, twr, ts;

	/* Generate points throughout the segment overlap */
	while (s < sb)
		{
		do_tension(tau,1.,1.,s,&twl,&twr,&ts);
		wl = (sb-s)/(sb-sa)/twl;
		wr = (s-sa)/(sb-sa)/twr;
		xp = wl*( (axl*ts + bxl)*ts + cxl)
		   + wr*( (axr*ts + bxr)*ts + cxr);
		yp = wl*( (ayl*ts + byl)*ts + cyl)
		   + wr*( (ayr*ts + byr)*ts + cyr);
		do_scallop(&xp,&yp,s);
		put_next(Pspline,xp,yp);
		if (!saved)
			{
			xclose = xp;
			yclose = yp;
			saved  = TRUE;
			}
		s += ds;
		}
	}


static	void	shift_segment(void)
	{
	/* Shift right-hand segment into left for next call */
	xa  = xb;	xb  = xc;
	ya  = yb;	yb  = yc;
	sa  = sb;	sb  = sc;
	txa = txb;	txb = txc;
	tya = tyb;	tyb = tyc;
	tsa = tsb;	tsb = tsc;

	axl = axr;	ayl = ayr;
	bxl = bxr;	byl = byr;
	cxl = cxr;	cyl = cyr;
	dxl = dxr;	dyl = dyr;
	}



/** End the stream (close if spec) */
void	flush_spline(void)
	{
	/* For closed contours simulate put_spline calls for the first  */
	/* three points to accomplish the closure condition */
	if (clsd)
		{
		if (np < 3) goto reset;
		put_spline(xfirst,yfirst);
		put_spline(xsecond,ysecond);
		put_spline(xthird,ythird);
		}

	/* For open contours compute linear-parametric coefficients in */
	/* remaining segment */
	else
		{
		if (np < 2) goto reset;
		axr = 0;				ayr = 0;
		bxr = dxl;				byr = dyl;
		cxr = txb - tsb*bxr;	cyr = tyb - tsb*byr;

		spline_segment();
		shift_segment();
		}

	/* Now output last point and flush */
	if (clsd && saved) put_next(Pspline,xclose,yclose);
	else               put_next(Pspline,xa,ya);
	reset:
	flush_next(Pspline);
	np    = 0;
	s     = 0;
	saved = FALSE;
	}


/***********************************************************************
*                                                                      *
*     d o _ t e n s i o n   - Perform parametric co-ordinate trans-    *
*                             formation to account for splines with    *
*                             or without tension.                      *
*                                                                      *
*     Without tension (tau <= 0) the spline solutions are in the       *
*     form of 2 linear weighted quadratics, as in:                     *
*                                                                      *
*              (x,y) = WL(s)*(AL*s*s + BL*s + CL)                      *
*                    + WR(s)*(AR*s*s + BR*s + CR)                      *
*                                                                      *
*     With tension (tau > 0) the spline solutions can be expressed     *
*     in the form:                                                     *
*                                                                      *
*              (x,y) = WL(s)*(AL*exp(tau*s) + BL + CL*exp(-tau*s))     *
*                    + WR(s)*(AR*exp(tau*s) + BR + CR*exp(-tau*s))     *
*                                                                      *
*     Which can be transformed to resemble the familiar quadratic      *
*     solutions by multiplying both sides by exp(tau*s), thus:         *
*                                                                      *
*            (tx,ty) = WL(s)*(AL*ts*ts + BL*ts + CL)                   *
*                    + WR(s)*(AR*ts*ts + BR*ts + CR)                   *
*                                                                      *
*                    where ts = exp(tau*s)                             *
*                          tx = ts*x                                   *
*                          ty = ts*y                                   *
*                                                                      *
***********************************************************************/

static	void	do_tension

	(
	float	tension,
	float	px,
	float	py,
	float	ps,
	float	*tx,
	float	*ty,
	float	*ts
	)

	{
	/* No tension - use given values */
	if (tension <= 0)
		{
		*tx = px;
		*ty = py;
		*ts = ps;
		}

	/* Tension - transform given values */
	else
		{
		*ts = exp(tension*ps);
		*tx = *ts*px;
		*ty = *ts*py;
		}
	}

/***********************************************************************
*                                                                      *
*     d e f i n e _ s c a l l o p   - Define scallop parameters.       *
*                                                                      *
*             d o _ s c a l l o p   - Transform splined points to      *
*                                     add a wave or scallop.           *
*                                                                      *
***********************************************************************/

/* Common parameters */
static	LOGICAL	enabled = FALSE;
static	LOGICAL	scallop = FALSE;
static	float	xo, yo, so, freq, ampl;

static	void	define_scallop

	(
	float	px,
	float	py,
	float	ps,
	float	resolution,
	float	period,
	float	amplitude
	)

	{
	/* Exit if not enabled */
	enabled = (period != 0) && (amplitude != 0);
	if (!enabled) return;

	/* Save anchor point and parameters */
	xo = px;
	yo = py;
	so = ps;
	ampl  = amplitude / resolution;
	freq  = PI / fabs(period);
	scallop = period < 0;
	}


static	void	do_scallop

	(
	float	*px,
	float	*py,
	float	ps
	)

	{
	float	t, dx, dy;

	/* Skip if disabled */
	if (!enabled) return;

	/* Compute the scallop as a sinusoid */
	t = sin(freq*(ps-so));
	if (scallop) t = fabs(t);
	t *= ampl;
	dx = t*(*py-yo);
	dy = t*(*px-xo);

	/* Modify the point and save unmodified point for next time */
	xo   = *px;
	yo   = *py;
	*px -= dx;
	*py += dy;
	}
