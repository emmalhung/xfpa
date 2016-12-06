/*********************************************************************/
/**	@file line_oper.c
 *
 * Assorted operations on lines.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    l i n e _ o p e r . c                                             *
*                                                                      *
*    Assorted operations on lines.                                     *
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

#include "line.h"

#include <tools/tools.h>
#include <fpa_getmem.h>

#include <stdio.h>
#include <string.h>

#undef DEBUG_LINES
#undef DEBUG_ROUNDOFF
#undef DEBUG_ORIENTATION

/* Internal static functions */
static	LOGICAL	line_orientation(LINE, LOGICAL *);

/***********************************************************************
*                                                                      *
*      l i n e _ c l o s e d                                           *
*      c l o s e _ l i n e                                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Is the given line closed?
 *
 *	@param[in] 	line		given line
 *  @return True if line is closed.
 *********************************************************************/

LOGICAL	line_closed

	(
	LINE	line
	)

	{
	/* Return if line not there */
	if (!line)             return FALSE;
	if (line->numpts <= 1) return FALSE;

	if (line->points[0][X] != line->points[line->numpts-1][X]) return FALSE;
	if (line->points[0][Y] != line->points[line->numpts-1][Y]) return FALSE;
	return TRUE;
	}

/**********************************************************************/

/*********************************************************************/
/** Close the given line.
 *
 *	@param[in] 	line		given line
 *********************************************************************/
void	close_line

	(
	LINE	line
	)

	{
	/* Return if line not there */
	if (!line)             return;
	if (line->numpts <= 0) return;

	if (line_closed(line)) return;
	add_point_to_line(line, line->points[0]);
	}


/***********************************************************************
*                                                                      *
*      l i n e _ p r o p e r t i e s                                   *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Measure certain 2-D properties of a line.
 *
 * Properties to measure:
 * 	- open or closed
 * 	- clockwise or counter-clockwise (closed lines only)
 * 	- enclosed area (closed lines only)
 *    (area calculated using midpoint rule for integration)
 * 	- length of line
 *
 *	@param[in] 	line		given line
 *	@param[out]	*closed		is it closed?
 *	@param[out]	*clockwise	is it clockwise? (only meaningful if closed)
 *	@param[out]	*area		enclosed area (only non-zero if closed)
 *	@param[out]	*length		length of line
 *********************************************************************/

void	line_properties

	(
	LINE	line,
	LOGICAL	*closed,
	LOGICAL	*clockwise,
	float	*area,
	float	*length
	)

	{
	float	x, xprev, dx;
	float	y, yprev, dy;
	float	yfirst, ybar;
	int		i, npts;
	LOGICAL	lok, right;

	LOGICAL	NeedClosed = FALSE;		LOGICAL	Closed = FALSE;
									LOGICAL	Clock  = FALSE;
	LOGICAL	NeedArea   = FALSE;		float	Area   = 0;
	LOGICAL	NeedLength = FALSE;		float	Length = 0;

	/* Initialize to nice garbage */
	if (closed)    *closed    = Closed;
	if (clockwise) *clockwise = Clock;
	if (area)      *area      = Area;
	if (length)    *length    = Length;

	/* Return if line not there */
	if (!line)             return;
	if (line->numpts <= 0) return;

	/* Set up first point */
	x      = line->points[0][X];
	y      = line->points[0][Y];
	yfirst = y;

	/* What things do we need to compute */
	NeedClosed = (LOGICAL) (NotNull(closed) || NotNull(clockwise) || NotNull(area));
	NeedArea   = (LOGICAL) (NotNull(clockwise) || NotNull(area));
	NeedLength = (LOGICAL) (NotNull(length));

	/* Initialize certain properties */
	npts = line->numpts;
	if (NeedClosed) Closed = ( npts > 1 )
						  && ( x == line->points[npts-1][X] )
						  && ( y == line->points[npts-1][Y] );

	/* Compute area and/or length if needed */
	if (NeedArea || NeedLength)
		{
		/* Loop from 2nd to last point */
		for (i=1; i<npts; i++)
			{
			/* Set up next point */
			xprev = x;
			yprev = y;
			x     = line->points[i][X];
			y     = line->points[i][Y];

			/* Increment area and length computations */
			dx    = x - xprev;
			dy    = y - yprev;
			ybar  = 0.5*( y + yprev );
			if (NeedArea && Closed) Area   += dx*( ybar - yfirst );
			if (NeedLength)         Length += hypot((double) dx, (double) dy);
			}
		}

	/* Compute remaining properties */
	if (clockwise) Clock = ( Area > 0 );
	if (area)      Area  = fabs((double) Area);

	/* Check for consistent line properties */
	if (clockwise && fabs((double) Area) > 0.0)
		{
		lok = line_orientation(line, &right);
		if (lok && Clock && !right)
			{
			(void) pr_error("Lines",
				"Fixing line properties - should be counterclockwise!\n");
			Clock = FALSE;
			}
		else if (lok && !Clock && right)
			{
			(void) pr_error("Areas",
				"Fixing line properties - should be clockwise!\n");
			Clock = TRUE;
			}
		}

	/* Return what was requested */
	if (closed)    *closed    = Closed;
	if (clockwise) *clockwise = Clock;
	if (area)      *area      = Area;
	if (length)    *length    = Length;
	}

/***********************************************************************
*                                                                      *
*      l i n e _ t e s t _ p o i n t                                   *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Measure certain 2-D properties of a test point, relative to
 * the given line.
 *
 * Properties to measure:
 * 	- minimum (perpendicular) distance from point to line
 * 	- closest point on line (interpolated if necessary)
 * 	- index of line segment which contains closest point
 * 	- point is inside or outside (closed lines only)
 * 	- point is on left or right side of line
 *
 *	@param[in] 	line		given line
 *	@param[in] 	ptest		given test point
 *	@param[out]	*pdist		minimum (perpendicular) distance to line
 *	@param[out]	ppoint		closest (perpendicular) point on line
 *	@param[out]	*pseg		index of segment which contains closest point
 *	@param[out]	*inside		is point enclosed by line?
 *	@param[out]	*right		is point on right side of line?
 *********************************************************************/

void	line_test_point

	(
	LINE	line,
	POINT	ptest,
	float	*pdist,
	POINT	ppoint,
	int		*pseg,
	LOGICAL	*inside,
	LOGICAL	*right
	)

	{
	float	xtest, ytest;
	float	x, y;
	float	xprev, yprev;
	float	xnext, ynext;
	float	xscld, yscld, scale;
	int		npts, ii;
	float	dxa, dya, da2;	/* dist from point to start of segment */
	float	dxb, dyb, db2;	/* dist from point to end of segment */
	float	dxp, dyp, dp2;	/* perp distance to current segment */
	float	dxs, dys, ds2;	/* length of current segment */
	float	dxn, dyn, dn2;	/* dist from point to end of next segment */
	float	dxt, dyt, dt2;	/* length of next segment */
	float	dxz, dyz, dz2;	/* dist from point to scaled next segment point */
	float	dtol=1e-5;
	float	a;
	float	dmin2;
	LOGICAL	closed, xperp;
	float	anga, angb, angn, tang, dang;

	float	dist = -1;
	int		seg  = -1;
	LOGICAL	perp = FALSE;
	LOGICAL	in   = FALSE;
	LOGICAL	rt   = FALSE;

	/* Initialize to nice garbage */
	if (ppoint) copy_point(ppoint, ptest);
	if (pdist)  *pdist  = dist;
	if (pseg)   *pseg   = seg;
	if (inside) *inside = in;
	if (right)  *right  = rt;

	/* Return if line not there */
	if (!ptest)            return;
	if (!line)             return;
	if (line->numpts <= 0) return;

	/* Parameters associated with the 1st point */
	xtest = ptest[X];
	ytest = ptest[Y];
	x     = line->points[0][X];
	y     = line->points[0][Y];
	dxa   = x - xtest;
	dya   = y - ytest;
	da2   = dxa*dxa + dya*dya;
	if (da2 == 0)
		{
		dist = 0;
		seg  = 0;
		if (ppoint) set_point(ppoint, x, y);
		if (pdist)  *pdist  = dist;
		if (pseg)   *pseg   = seg;
		if (inside) *inside = in;
		if (right)  *right  = rt;
		return;
		}
	anga = atan2((double) dya, (double) dxa);

	/* Initialize a few properties */
	npts    = line->numpts;
	tang	= 0;
	dmin2   = da2;
	closed	= ( npts > 1 )
		   && ( x == line->points[npts-1][X] )
		   && ( y == line->points[npts-1][Y] );
	if (ppoint) set_point(ppoint, x, y);
	seg  = (closed) ? npts-1 : 0;
	rt   = FALSE;

	/* Scan through all the segments on the line */
	for (ii=1; ii<npts; ii++)
		{
		/* Set up next point */
		xprev = x;
		yprev = y;
		x     = line->points[ii][X];
		y     = line->points[ii][Y];

		/* Parameters associated with current point */
		dxb = x - xtest;
		dyb = y - ytest;
		db2 = dxb*dxb + dyb*dyb;
		if (db2 == 0)
			{
			dist = 0;
			seg  = ii - 1;
			if (ppoint) set_point(ppoint, x, y);
			if (pdist)  *pdist  = dist;
			if (pseg)   *pseg   = seg;
			if (inside) *inside = in;
			if (right)  *right  = rt;
			return;
			}

		/* Distance between current and previous points */
		dxs = x - xprev;
		dys = y - yprev;
		ds2 = dxs*dxs + dys*dys;

		/* Ignore very short spans ... check the next one instead! */
		if ((fabs((double) dxs)<dtol) && (fabs((double) dys)<dtol)) continue;

		/* Increment cumulative angle */
		if (db2 > 0) angb = atan2((double) dyb, (double) dxb);
		else         angb = 0;
		dang = angb - anga;
		if (dang < -PI) dang += (PI + PI);
		if (dang >  PI) dang -= (PI + PI);
		tang += dang;

		/* If segment contains perpendicular, compute perpendicular distance */
		if (ds2 > fabs((double) (db2-da2)))
			{
			xperp = TRUE;

			/* Almost horizontal case ... y = f(x) */
			if (fabs((double) (dxb-dxa)) >= fabs((double) (dyb-dya)))
				{
				a   = (dyb-dya)/(dxb-dxa);
				dxp = a*(a*dxa - dya) / (1 + a*a);
				dyp = a*(dxp-dxa) + dya;
				dp2 = dxp*dxp + dyp*dyp;
				}

			/* Almost vertical case ... x = g(y) */
			else
				{
				a   = (dxb-dxa)/(dyb-dya);
				dyp = a*(a*dya - dxa) / (1 + a*a);
				dxp = a*(dyp-dya) + dxa;
				dp2 = dxp*dxp + dyp*dyp;
				}
			}

		/* Otherwise segment does not contain perpendicular, so use end point */
		else
			{
			xperp = FALSE;
			dxp   = dxb;
			dyp   = dyb;
			dp2   = db2;
			}

		/* If closer than best so far, remember it */
		if (dp2 < dmin2)
			{
			dmin2 = dp2;
			perp  = xperp;
			seg   = ii - 1;
			rt    = (dang < 0);
			if (ppoint) set_point(ppoint, xtest+dxp, ytest+dyp);
			}

		/* Set initial side of line from first span, regardless of closeness */
		else if (ii == 1)
			{
			rt    = (dang < 0);
			}

		/* Update parameters on previous point */
		dxa  = dxb;
		dya  = dyb;
		da2  = db2;
		anga = angb;
		}

	/* Check for which side of line (if required) for non perpendicular cases   */
	/* This will occur for lines that turn sharply, when the test point lies    */
	/*  between the perpendicular to this span and the reverse extension of the */
	/*  next span, or between the extension of this span and the perpendicular  */
	/*  to the next span and the intersection is the end point of this span     */
	/* Compare the distance from the start point of this span to the test point */
	/*  with the distance from the end point of the next span (scaled to be the */
	/*  same length as this span) to the test point to determine which to use   */
	/* Cases where the test point lies between the extension of this span and   */
	/*  the perpendicular to the next span will need to be adjusted             */
	if ( right && !perp &&
			(closed || (!closed && seg >= 0 && seg < npts-2)) )
		{

		/* Recalculate distance from start point of closest span */
		xprev = line->points[seg][X];
		yprev = line->points[seg][Y];
		dxa   = xprev - xtest;
		dya   = yprev - ytest;
		da2   = dxa*dxa + dya*dya;
		anga = atan2((double) dya, (double) dxa);

		/* Recalculate distance and angle from end point of closest span */
		ii = seg + 1;
		if (closed && ii >= npts) ii -= (npts - 1);
		x    = line->points[ii][X];
		y    = line->points[ii][Y];
		dxb  = x - xtest;
		dyb  = y - ytest;
		db2  = dxb*dxb + dyb*dyb;
		angb = atan2((double) dyb, (double) dxb);

		/* Recalculate distance between current and previous points */
		dxs = x - xprev;
		dys = y - yprev;
		ds2 = dxs*dxs + dys*dys;

		/* Calculate distance between next and current points */
		ii = seg + 2;
		if (closed && ii >= npts) ii -= (npts - 1);
		xnext = line->points[ii][X];
		ynext = line->points[ii][Y];
		dxt   = xnext - x;
		dyt   = ynext - y;
		dt2   = dxt*dxt + dyt*dyt;

		/* Determine scaled point on next span for testing */
		scale = sqrt((double) ds2) / sqrt((double) dt2);
		xscld = x + scale * dxt;
		yscld = y + scale * dyt;

		/* Calculate distance from scaled point on next span */
		dxz = xscld - xtest;
		dyz = yscld - ytest;
		dz2 = dxz*dxz + dyz*dyz;

		/* If scaled point is closer use next span for determining which side */
		if (dz2 < da2)
			{

			/* Calculate angle from end point of next span */
			dxn  = xnext - xtest;
			dyn  = ynext - ytest;
			dn2  = dxn*dxn + dyn*dyn;
			angn = atan2((double) dyn, (double) dxn);

			/* Determine difference in angles */
			dang = angn - angb;
			if (dang < -PI) dang += (PI + PI);
			if (dang >  PI) dang -= (PI + PI);

			/* Change which side (if required) */
			if ( (rt && !(dang < 0)) || (!rt && (dang < 0)) )
				{

				/* Change to which side of line has been found */
				rt = !rt;

#				ifdef DEBUG_LINES
				(void) fprintf(stderr,
					"[line_test_point] Change to side of line encountered!\n");
				(void) fprintf(stderr,
					"[line_test_point]  xtest/ytest: %.3f %.3f\n",
					xtest, ytest);
				(void) fprintf(stderr,
					"[line_test_point]  x/y:         %.3f %.3f  dxb/dyb: %.3f %.3f\n",
					x, y, dxb, dyb);
				(void) fprintf(stderr,
					"[line_test_point]   db2: %.3f  angb: %.5f\n", db2, angb);
				(void) fprintf(stderr,
					"[line_test_point]  xprev/yprev: %.3f %.3f  dxa/dya: %.3f %.3f\n",
					xprev, yprev, dxa, dya);
				(void) fprintf(stderr,
					"[line_test_point]   da2: %.3f  anga: %.5f\n", da2, anga);
				(void) fprintf(stderr,
					"[line_test_point]  dxs/dys: %.3f %.3f  ds2: %.3f\n",
					dxs, dys, ds2);
				(void) fprintf(stderr,
					"[line_test_point]  xnext/ynext: %.3f %.3f  dxn/dyn: %.3f %.3f\n",
					xnext, ynext, dxn, dyn);
				(void) fprintf(stderr,
					"[line_test_point]   dn2: %.3f  angn: %.5f\n", dn2, angn);
				(void) fprintf(stderr,
					"[line_test_point]  dxt/dyt: %.3f %.3f  dt2: %.3f\n",
					dxs, dys, ds2);
				(void) fprintf(stderr,
					"[line_test_point]  xscld/yscld: %.3f %.3f  dxz/dyz: %.3f %.3f\n",
					xscld, yscld, dxz, dyz);
				(void) fprintf(stderr,
					"[line_test_point]   dz2: %.3f  scale: %.3f\n", dz2, scale);
				if (rt)
					(void) fprintf(stderr,
						"[line_test_point]  dang: %.3f  Change side to: R\n",
						dang);
				else
					(void) fprintf(stderr,
						"[line_test_point]  dang: %.3f  Change side to: L\n",
						dang);
#				endif /* DEBUG_LINES */
				}
			}
		}

	/* Compute remaining properties */
	dist = sqrt((double) dmin2);
	in   = (LOGICAL) (closed && ( fabs((double) tang) > PI ));

	/* Return what was requested */
	if (pdist)  *pdist  = dist;
	if (pseg)   *pseg   = seg;
	if (inside) *inside = in;
	if (right)  *right  = rt;
	}

/***********************************************************************
*                                                                      *
*      l i n e _ s i g h t                                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find closest point on the given line to a given test point,
 * along the line of sight defined by a second test point.  If no
 * intersection occurs, return the closest approach.
 *
 * Method:
 * 	- Parameterize a line through the test points as:
 * @f$ ({x_o}, {y_o}) + {t}({x_v}-{x_o}, {y_v}-{y_o}) @f$
 * 	- Then parameterize a line through the end points of each span on
 * 	  the curve, as:
 * @f$ ({x_a}, {y_a}) + {s}({x_b}-{x_a}, {y_b}-{y_a}) @f$
 * 	- Then solve for s, such that the two lines cross.
 * 	- We have an intersection only if s is between 0 and 1, meaning
 * 	  that the intersection lies within the span.
 * 	- Return only the closest of these intersections, if any were
 * 	  found.  A negative distance just means that the closest
 * 	  intersection was found in the opposite direction from the
 * 	  second point.
 *
 * @note Make sure to test the return value:
 *
 * 	- A return value of TRUE means that an intersection was found and
 * 	  the returned parameters may be interpretted as:
 * 		- dist     = distance from po to intersection,
 * 		- approach = (forced to zero),
 * 		- point    = the actual point of intersection on the target
 * 					 line,
 * 		- span     = span containing the intersection,
 * 		- between  = is intersection between po and pv?
 * 	- A return value of FALSE means that no intersections were found
 * 	  and the returned parameters should be interpretted as:
 * 		- dist     = distance from po to closest approach along sight
 * 		             line,
 * 		- approach = shortest distance from sight line to the target
 * 		             line,
 * 		- point    = the actual point of closest approach, on the
 * 		             target line,
 * 		- span     = span on target line that is closest,
 * 		- between  = is closest approach between po and pv?
 * 	- If something went wrong, approach will be negative.
 *
 *	@param[in] 	line		given line
 *	@param[in] 	po			given test point
 *	@param[in] 	pv			second point which gives direction of sight
 *	@param[in] 	back		do we want negative distance if closer
 *	@param[out]	*dist		distance to closest intersection/approach
 *	@param[out]	*approach	closest approach (0 if intersection)
 *	@param[out]	point		closest intersection/approach on line
 *	@param[out]	*span		index of span which contains closest point
 *	@param[out]	*between	is intersection/approach between the test points
 * 	@return True if an intersection was found.
 *********************************************************************/

LOGICAL	line_sight

	(
	LINE	line,
	POINT	po,
	POINT	pv,
	LOGICAL	back,
	float	*dist,
	float	*approach,
	POINT	point,
	int		*span,
	LOGICAL	*between
	)

	{
	float	xo, xv, xa, xb, xint, cxint;
	float	yo, yv, ya, yb, yint, cyint;
	double	dsx, dsy, dcx, dcy, dss;
	double	s, top, bottom, dtest, dtol, d, cd;
	double	dmin, dxmin=-1, dcmin=-1, a, amin;
	int		i, cspan;
	LOGICAL	found, bfound, cfound, inside;

	/* Initialize to nice garbage */
	if (point)    copy_point(point, po);
	if (dist)     *dist     = 0;
	if (approach) *approach = -1;
	if (span)     *span     = -1;
	if (between)  *between  = FALSE;

	/* Return if curve does not exist */
	if (!po)               return FALSE;
	if (!pv)               return FALSE;
	if (!line)             return FALSE;
	if (line->numpts <= 0) return FALSE;

	/* Compute spans between test points and construct unit vector */
	xo  = po[X];	yo  = po[Y];
	xv  = pv[X];	yv  = pv[Y];
	dsx = xv - xo;	dsy = yv - yo;
	if ( (dsx == 0) && (dsy == 0) ) return FALSE;
	dtest = hypot(dsx, dsy);
	dtol  = dtest * .01;
	dsx  /= dtest;
	dsy  /= dtest;

	/* Compute approach to first point on curve */
	xa   = line->points[0][X];
	ya   = line->points[0][Y];
	s    = hypot((double) (xa-xo), (double) (ya-yo));
	d    = (xa-xo)*dsx + (ya-yo)*dsy;
	dmin = fabs(d);
	amin = (s > dmin)? sqrt( s*s - dmin*dmin ): 0;
	if (point)    set_point(point, xa, ya);
	if (dist)     *dist     = dmin;
	if (approach) *approach = amin;
	if (span)     *span     = 0;
	if (between)  *between  = (LOGICAL) ( (d > -dtol) && (d < dtest+dtol) );

	/* Scan through all the spans on the curve ... numbered by i-1 */
	found  = FALSE;
	bfound = FALSE;
	cfound = FALSE;
	for (i=1; i<line->numpts; i++)
		{
		/* Set up current span */
		xa  = line->points[i-1][X];	ya  = line->points[i-1][Y];
		xb  = line->points[i][X];	yb  = line->points[i][Y];
		dcx = xb - xa;		dcy = yb - ya;
		if ( (dcx == 0) && (dcy == 0) ) continue;

		/* If no intersection so far, keep looking for closest approach */
		if (!found && !cfound)
			{
			s = hypot((double) (xb-xo), (double) (yb-yo));
			d = (xb-xo)*dsx + (yb-yo)*dsy;
			a = (s > fabs(d))? sqrt( s*s - d*d ): 0;
			if ((a < amin) || ((a == amin) && (fabs(d)<dmin)))
				{
				/* Set to last point on this span or first point on next span */
				dmin = fabs(d);
				amin = a;
				if (point)    set_point(point, xb, yb);
				if (dist)     *dist     = dmin;
				if (approach) *approach = amin;
				if (span)     *span     = i;
				if (between)  *between  = (LOGICAL)
										  ( (d > -dtol) && (d < dtest+dtol) );
				}
			}

		/* Compute operands for generating parameter s */
		/* Diagonal to vertical case */
		if (fabs(dsx) <= fabs(dsy))
			{
			dss    = dsx / dsy;
			top    = (xo-xa) - (yo-ya)*dss;
			bottom =     dcx -     dcy*dss;
			}

		/* Diagonal to horizontal case */
		else
			{
			dss    = dsy / dsx;
			top    = (xo-xa)*dss - (yo-ya);
			bottom =     dcx*dss -     dcy;
			}

		/* Compute parameter s */
		if (bottom == 0)
			{
			/* The span is parallel to the test points. */
			/* An intersection is only possible if they are colinear. */
			/* Use po itself if it is within the span. */
			/* Otherwise use the closest end of the span. */
			if (top != 0) continue;
			if (fabs(dsx) <= fabs(dsy)) s = (yo-ya)/dcy;
			else                        s = (xo-xa)/dcx;
			}
		else
			{
			/* The span is not parallel to the test points. */
			/* An intersection must occur somewhere */
			/* Only consider intersections within the span */
			s = top/bottom;
			if (s < -.01) continue;
			if (s > 1.01) continue;
#			ifdef DEBUG_ROUNDOFF
			if ( (s < 0) || (s > 1) )
				(void) fprintf(stderr,
					"[line_sight] *** Would have died!!! (A) s: %.6f\n",
					s);
#			endif /* DEBUG_ROUNDOFF */
			}

		/* Check for intersection on span or close to span */
		if (s >= 0.0 && s <= 1.0) inside = TRUE;
		else                      inside = FALSE;

		/* Round off if close but not inside */
		if (!inside)
			{
#			ifdef DEBUG_ROUNDOFF
			xint = xa + s*dcx;
			yint = ya + s*dcy;
			d    = (xint-xo)*dsx + (yint-yo)*dsy;
			(void) fprintf(stderr,
				"[line_sight] Close ... span/d: %d/%.6f  xint/yint: %.6f/%.6f\n",
				i-1, d, xint, yint);
#			endif /* DEBUG_ROUNDOFF */

			s = MAX(s, 0);
			s = MIN(s, 1);
			}

		/* We have an intersection */
		/* Compute actual intersection point and distance from test point */
		xint = xa + s*dcx;
		yint = ya + s*dcy;
		d    = (xint-xo)*dsx + (yint-yo)*dsy;

#		ifdef DEBUG_ROUNDOFF
		if (inside)
			(void) fprintf(stderr,
				"[line_sight] Found ... span/d: %d/%.6f  xint/yint: %.6f/%.6f\n",
				i-1, d, xint, yint);
#		endif /* DEBUG_ROUNDOFF */

		/* Continue if not looking backwards! */
		if (inside && !back && d<0) bfound = TRUE;
		if (!back && d<0) continue;

		/* Reset intersection point */
		if (inside && (!found || (fabs(d)<dxmin)))
			{
			/* Set to point on this span */
			found = TRUE;
			dxmin = fabs(d);
			if (point)    set_point(point, xint, yint);
			if (dist)     *dist     = d;
			if (approach) *approach = 0;
			if (span)     *span     = i-1;
			if (between)  *between  = (LOGICAL)
									  ( (d > -dtol) && (d < dtest+dtol) );
#			ifdef DEBUG_ROUNDOFF
			if ( (d > -dtol) && (d < dtest+dtol) )
				if ( (d < 0) || (d > dtest) )
					(void) fprintf(stderr,
						"[line_sight] *** Would have died!!! (B) d/dtest: %.6f/%.6f\n",
						d, dtest);
#			endif /* DEBUG_ROUNDOFF */
			}

		/* Keep track of close intersections if nothing better found */
		else if (!inside && !found && !bfound && (!cfound || (fabs(d)<dcmin)))
			{
			/* Set to point on this span */
			cfound = TRUE;
			dcmin  = fabs(d);
			cxint  = xint;
			cyint  = yint;
			cd     = d;
			cspan  = i-1;

#			ifdef DEBUG_ROUNDOFF
			if ( (cd > -dtol) && (cd < dtest+dtol) )
				if ( (cd < 0) || (cd > dtest) )
					(void) fprintf(stderr,
						"[line_sight] *** Would have died!!! (B) cd/dtest: %.6f/%.6f\n",
						cd, dtest);
#			endif /* DEBUG_ROUNDOFF */
			}
		}

	/* Return if intersection found */
	if (found)
		{

#		ifdef DEBUG_ROUNDOFF
		if (span && dist && point)
			(void) fprintf(stderr,
				"[line_sight] Return Found ... span/d: %d/%.6f  xint/yint: %.6f/%.6f\n",
				*span, *dist, point[X], point[Y]);
		else
			(void) fprintf(stderr,
				"[line_sight] Return Found ...\n");
#		endif /* DEBUG_ROUNDOFF */

		return found;
		}

	/* Do not return close intersection if backwards intersection found */
	else if (bfound && cfound)
		{

#		ifdef DEBUG_ROUNDOFF
		(void) fprintf(stderr,
			"[line_sight] Do not return Close ... span/d: %d/%.6f  xint/yint: %.6f/%.6f\n",
			cspan, cd, cxint, cyint);
#		endif /* DEBUG_ROUNDOFF */

		return found;
		}

	/* Do not return backwards intersection (if not requested) */
	else if (bfound)
		{

#		ifdef DEBUG_ROUNDOFF
		(void) fprintf(stderr, "[line_sight] Do not return Backwards\n");
#		endif /* DEBUG_ROUNDOFF */

		return found;
		}

	/* Return close intersection if nothing better found */
	else if (!bfound && cfound)
		{
		if (point)    set_point(point, cxint, cyint);
		if (dist)     *dist     = cd;
		if (approach) *approach = 0;
		if (span)     *span     = cspan;
		if (between)  *between  = (LOGICAL)
								  ( (cd > -dtol) && (cd < dtest+dtol) );

#		ifdef DEBUG_ROUNDOFF
		(void) fprintf(stderr,
			"[line_sight] Return Close ... span/d: %d/%.6f  xint/yint: %.6f/%.6f\n",
			cspan, cd, cxint, cyint);
#		endif /* DEBUG_ROUNDOFF */

		return cfound;
		}

	/* Return if no intersection found */
	else
		return found;
	}

/***********************************************************************
*                                                                      *
*      l i n e _ c l o s e s t _ p o i n t                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find closest point in the given line to the given point.
 *
 * @note If you want the closest perpendicular point on the
 * line, use line_test_point().
 *
 *	@param[in] 	line	given line
 *	@param[in] 	ptest	given test point
 *	@param[out]	*dist	distance to closest point
 *	@param[out]	point	closest point
 * @return Array index of closest point in the line.
 *********************************************************************/

int		line_closest_point

	(
	LINE	line,
	POINT	ptest,
	float	*dist,
	POINT	point
	)

	{
	float	x, xtest, dx;
	float	y, ytest, dy;
	float	dist2, dmin2;
	int		ip, ipmin;

	/* Initialize to nice garbage */
	ipmin = -1;
	if (point) copy_point(point, ptest);
	if (dist)  *dist = 0;

	/* Return nice garbage if curve does not exist */
	if (!ptest)            return ipmin;
	if (!line)             return ipmin;
	if (line->numpts <= 0) return ipmin;

	/* Loop from 1st to last point */
	xtest = ptest[X];
	ytest = ptest[Y];
	dmin2 = 0;
	for (ip=0; ip<line->numpts; ip++)
		{
		/* Set up next point */
		x = line->points[ip][X];
		y = line->points[ip][Y];

		/* Parameters associated with current point */
		dx    = x - xtest;
		dy    = y - ytest;
		dist2 = dx*dx + dy*dy;

		/* If closer than best so far, remember it */
		if (ip==0 || dist2<dmin2)
			{
			ipmin = ip;
			dmin2 = dist2;
			if (point) set_point(point, x, y);
			if (dist2 == 0) return ipmin;
			}
		}

	/* Compute actual distance */
	if (dist) *dist = sqrt((double) dmin2);
	return ipmin;
	}

/***********************************************************************
*                                                                      *
*     l i n e _ i n t e r s e c t                                      *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find the point at which two curves cross.
 *
 *	@param[in] 	line	line being crossed
 *	@param[in] 	divl	crossing line
 *	@param[out]	*ip1	indice of point in divl that is known to
 *						be on the opposite side of line to ip2
 *	@param[out]	*ip2	indice of point in divl that is known to
 *						be on the opposite side of line to ip1
 * 	@return Array index of point closest to the intersection point in
 * 			the line. -1 if something went wrong.
 *********************************************************************/

int		line_intersect

	(
	LINE	line,
	LINE	divl,
	int		*ip1,
	int		*ip2
	)

	{
	LOGICAL	side, side1, side2;
	int		ipmid;
	POINT	cpos;
	float	*p;

	/* Make sure everything is consistent */
	if (!line)                return -1;
	if (line->numpts <= 0)    return -1;
	if (!divl)                return -1;
	if (divl->numpts <= 0)    return -1;
	if (!ip1)                 return -1;
	if (*ip1 < 0)             return -1;
	if (*ip1 >= divl->numpts) return -1;
	if (!ip2)                 return -1;
	if (*ip2 < 0)             return -1;
	if (*ip2 >= divl->numpts) return -1;

	/* Let's find out what side is which */
	p = divl->points[*ip1];
	line_test_point(line, p, NullFloat, cpos, NullInt, NullChar, &side1);
	p = divl->points[*ip2];
	line_test_point(line, p, NullFloat, cpos, NullInt, NullChar, &side2);
	if (side1 == side2) return -1;

	/* Do a binary search for the intersection */
	while (abs(*ip2 - *ip1) > 1)
		{
		ipmid = (*ip1 + *ip2) / 2;
		p     = divl->points[ipmid];
		line_test_point(line, p, NullFloat, cpos, NullInt, NullChar, &side);
		if (side == side1) *ip1  = ipmid;
		else               *ip2 = ipmid;
		}

	/* Return closest span */
	return (int) MIN(*ip1, *ip2);
	}

/***********************************************************************
*                                                                      *
*     f i n d _ l i n e _ c r o s s i n g                              *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find the next point at which two lines cross.  It is assumed that
 * the lines cross any number of times, or not at all.
 *
 *	@param[in] 	line1	first line to be examined
 *	@param[in] 	line2	second line to be examined
 *	@param[in] 	start	span of line1 to start at
 *	@param[in] 	spos	point on span of line1 to start at
 *	@param[out]	cross	point of intersection
 *	@param[out]	*span1	span on line1 where crossover occurs
 *	@param[out]	*span2	span on line2 where crossover occurs
 *	@param[out]	*right	now on right side?
 *  @return True if successful.
 *********************************************************************/

LOGICAL	find_line_crossing

	(
	LINE	line1,
	LINE	line2,
	int		start,
	POINT	spos,
	POINT	cross,
	int		*span1,
	int		*span2,
	LOGICAL	*right
	)

	{
	int		ip, cspan, ipt;
	float	*ppos, *npos, xdist, sdist2, edist2, dx, dy;
	double	dang;
	LINE	tline;
	POINT	xcross, xpos;
	LOGICAL	intrsct, between, nside;

	/* Set up reasonable return values */
	if (cross) copy_point(cross, ZeroPoint);
	if (span1) *span1 = -1;
	if (span2) *span2 = -1;
	if (right) *right = FALSE;

	/* Make sure things are consistent */
	if (!line1)                 return FALSE;
	if (line1->numpts <= 0)     return FALSE;
	if (!line2)                 return FALSE;
	if (line2->numpts <= 0)     return FALSE;
	if (start >= line1->numpts) return FALSE;

	/* Set start point on line */
	ppos = spos;

	/* Scan rest of the line to find crossings */
	for (ip=start+1; ip<line1->numpts; ip++)
		{

		/* Set next point on line */
		npos = line1->points[ip];

		/* If the points coincide, try the next point */
		if (npos[X] == ppos[X] && npos[Y] == ppos[Y]) continue;

		/* Look for crossing between these points */
		intrsct = line_sight(line2, ppos, npos, FALSE, &xdist, NullFloat,
								xcross, &cspan, &between);

		/* The line_sight() function allows a certain tolerance in   */
		/*  determining intersections to account for round off error */
		/*  ... so check for potential problem cases below           */

		/* An intersection that occurs just after the end of a span may be */
		/*  mistakenly identified as occurring in the current span ... so  */
		/*  ensure xdist is not greater than the length of current span    */
		/* (Note that since we always check in a forward direction, we can */
		/*  never find an intersection before the start of a span!)        */
		if (intrsct && between && xdist > point_dist(ppos, npos))
			{
			between = FALSE;

#			ifdef DEBUG_LINES
			pr_diag("find_line_crossing",
				"Beyond end of span crossing ... xdist/dist: %.4f/%.4f\n",
				xdist, point_dist(ppos, npos));
#			endif /* DEBUG_LINES */
			}

		/* Found an intersection point */
		if (intrsct && between)
			{

			/* If intersection is very close to end point, move back */
			/*  towards start point to find which side we started on */
			/*  (reverse will be which side we ended on!)            */
			edist2 = point_dist2(npos, xcross);
			if (edist2 < EndTol*EndTol)
				{

				/* Determine side we ended on only if necessary */
				if (right)
					{
					sdist2 = point_dist2(ppos, xcross);
					if (sdist2 < SegTol*SegTol)
						{
						copy_point(xpos, ppos);
						}
					else
						{
						dx   = xcross[X] - ppos[X];
						dy   = xcross[Y] - ppos[Y];
						dang = atan2((double) dy, (double) dx);
						xpos[X] = xcross[X] - SegTol * (float) cos(dang);
						xpos[Y] = xcross[Y] - SegTol * (float) sin(dang);
						}

					/* Check intersection span to determine side we started */
					/*  on ... and then reverse                             */
					tline = create_line();
					if (cspan < 0)                     ipt = 0;
					else if (cspan >= line2->numpts-1) ipt = line2->numpts - 2;
					else                               ipt = cspan;
					add_point_to_line(tline, line2->points[ipt]);
					add_point_to_line(tline, line2->points[ipt+1]);
					line_test_point(tline, xpos, NullFloat, NullPoint,
										NullInt, NullChar, &nside);
					nside = !nside;
					tline = destroy_line(tline);
					}

#				ifdef DEBUG_LINES
				pr_diag("find_line_crossing",
					"Close to end of span crossing! edist: %.4f  span1: %d (0-%d)  span2: %d (0-%d)\n",
					sqrt(edist2), ip-1, line1->numpts-1, cspan, line2->numpts-1);
#				endif /* DEBUG_LINES */

				/* Set the intersection point */
				if (cross) copy_point(cross, xcross);
				if (span1) *span1 = ip - 1;
				if (span2) *span2 = cspan;
				if (right) *right = nside;
				return TRUE;
				}

			/* Otherwise, move towards end point to determine */
			/*  which side we ended on                        */
			else
				{

				/* Determine side we ended on only if necessary */
				if (right)
					{
					sdist2 = point_dist2(xcross, npos);
					if (sdist2 < SegTol*SegTol)
						{
						copy_point(xpos, npos);
						}
					else
						{
						dx   = npos[X] - xcross[X];
						dy   = npos[Y] - xcross[Y];
						dang = atan2((double) dy, (double) dx);
						xpos[X] = xcross[X] + SegTol * (float) cos(dang);
						xpos[Y] = xcross[Y] + SegTol * (float) sin(dang);
						}

					/* Check intersection span to determine side we ended on */
					tline = create_line();
					if (cspan < 0)                     ipt = 0;
					else if (cspan >= line2->numpts-1) ipt = line2->numpts - 2;
					else                               ipt = cspan;
					add_point_to_line(tline, line2->points[ipt]);
					add_point_to_line(tline, line2->points[ipt+1]);
					line_test_point(tline, xpos, NullFloat, NullPoint,
										NullInt, NullChar, &nside);
					tline = destroy_line(tline);
					}

#				ifdef DEBUG_LINES
				pr_diag("find_line_crossing", "Normal crossing!\n");
#				endif /* DEBUG_LINES */

				/* Set the intersection point */
				if (cross) copy_point(cross, xcross);
				if (span1) *span1 = ip - 1;
				if (span2) *span2 = cspan;
				if (right) *right = nside;
				return TRUE;
				}
			}

		/* Keep looking */
		ppos = npos;
		continue;
		}

	/* No intersections found */
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*     l o o p e d _ l i n e _ c r o s s i n g                          *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Find the point at which a line crosses over itself.
 * It is assumed that the line may cross itself any number of
 * times, or not at all.
 *
 *	@param[in] 	line	line to be examined
 *	@param[out]	cross	point of crossover
 *	@param[out]	*seg1	first segment on line where crossover occurs
 *	@param[out]	*seg2	second segment on line where crossover occurs
 *  @return TRUE if a crossing was found.
 ***********************************************************************/

LOGICAL	looped_line_crossing

	(
	LINE	line,
	POINT	cross,
	int		*seg1,
	int		*seg2
	)

	{
	int		ii, iseg1, iseg2;
	float	xmin, xmax, ymin, ymax;
	LOGICAL	closed, between;
	POINT	p1, p2, xcross;
	LINE    xline;

	/* Set up reasonable return values */
	if (cross) copy_point(cross, ZeroPoint);
	if (seg1) *seg1 = -1;
	if (seg2) *seg2 = -1;

	/* Make sure things are consistent */
	/* Note that line with 3 or fewer points cannot cross over itself! */
	if (!line)             return FALSE;
	condense_line(line);
	if (line->numpts <= 3) return FALSE;
	closed = line_closed(line);

	/* Create line object for testing */
	xline = create_line();

	/* Loop to check each segment of line for crossovers */
	for (ii=2; ii<line->numpts-1; ii++)
		{

		/* Set points to check from preceding segment */
		iseg1 = ii - 2;
		(void) copy_point(p1, line->points[iseg1]);
		(void) copy_point(p2, line->points[iseg1+1]);

		/* Special case for first point of closed lines               */
		/*  ... to avoid identifying first/last point as a crossover! */
		if (closed && iseg1 == 0)
			{
			p1[X] = 0.999*p1[X] + 0.001*p2[X];
			p1[Y] = 0.999*p1[Y] + 0.001*p2[Y];
			}

		/* Make a copy of the remainder of the line */
		(void) empty_line(xline);
		xline = append_line_portion(xline, line, ii, line->numpts-1);

		/* Check for crossing on each line segment */
		if ( line_sight(xline, p1, p2, TRUE,
				NullFloat, NullFloat, xcross, &iseg2, &between) && between )
			{

			/* Ensure crossing is within test segment */
			xmin = MIN(p1[0], p2[0]);
			xmax = MAX(p1[0], p2[0]);
			ymin = MIN(p1[1], p2[1]);
			ymax = MAX(p1[1], p2[1]);
			if ( xcross[0] < xmin || xcross[0] > xmax
					|| xcross[1] < ymin || xcross[1] > ymax ) continue;

#			ifdef DEBUG_LINES
			(void) fprintf(stderr,
				"[looped_line_crossing] cross: %.2f %.2f\n",
				xcross[0], xcross[1]);
			(void) fprintf(stderr,
				"[looped_line_crossing] check: %.2f %.2f to %.2f %.2f (%d-%d of 0-%d)\n",
				p1[0], p1[1], p2[0], p2[1], iseg1, iseg1+1, line->numpts-1);
			(void) fprintf(stderr,
				"[looped_line_crossing] xline: %.2f %.2f to %.2f %.2f (%d-%d of 0-%d)\n",
				xline->points[iseg2][0],   xline->points[iseg2][1],
				xline->points[iseg2+1][0], xline->points[iseg2+1][1],
				iseg2, iseg2+1, xline->numpts-1);
#			endif /* DEBUG_LINES */

			/* Set first and second segments */
			/* Second segment is adjusted to account for missing first part */
			iseg2 += ii;
			if (cross) copy_point(cross, xcross);
			if (seg1) *seg1 = iseg1;
			if (seg2) *seg2 = iseg2;

			pr_error("Lines",
				"Line crosses itself at span %d and %d! (0 to %d)\n",
				iseg1, iseg2, xline->numpts-1);

			/* Return for crossover found */
			(void) destroy_line(xline);
			return TRUE;
			}
		}

	/* No crossover found */
	(void) destroy_line(xline);
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*      l i n e _ a p p r o a c h                                       *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find an intersection on the given line by a given test line.
 * If no intersection occurs, return the closest approach.
 *
 *	@param[in] 	line		given line
 *	@param[in] 	tline		given test line
 *	@param[out]	*dist		distance along test line to intersection/approach
 *	@param[out]	*approach	distance of closest approach (0 if intersection)
 *	@param[out]	point		closest intersection/approach on lines
 *	@param[out]	*span		index of span on line which contains point
 *	@param[out]	*tspan		index of span on test line which contains point
 * 	@return True if an intersection was found.
 *********************************************************************/

LOGICAL	line_approach

	(
	LINE	line,
	LINE	tline,
	float	*dist,
	float	*approach,
	POINT	point,
	int		*span,
	int		*tspan
	)

	{
	LOGICAL	first, intrsct, between;
	int		ip, aip, isp, aisp;
	float	adist, appr, amin, dx, dy;
	POINT	ppos, npos, xpos, apos;

	/* Initialize to nice garbage */
	if (point)    copy_point(point, ZeroPoint);
	if (dist)     *dist     = 0;
	if (approach) *approach = -1;
	if (span)     *span     = -1;
	if (tspan)    *tspan    = -1;

	/* Return if lines do not exist */
	if (!line)              return FALSE;
	if (line->numpts <= 1)  return FALSE;
	if (!tline)             return FALSE;
	if (tline->numpts <= 1) return FALSE;

	/* Set start point on test line */
	copy_point(ppos, tline->points[0]);

	/* Compute closest approach for each successive span on test line */
	first = TRUE;
	for (ip=0; ip<tline->numpts; ip++)
		{
		copy_point(npos, tline->points[ip]);
		if (npos[X] == ppos[X] && npos[Y] == ppos[Y]) continue;

		/* Determine closest approach to this span */
		intrsct = line_sight(line, ppos, npos, TRUE, NullFloat, &appr,
								xpos, &isp, &between);

		/* Return if we found an intersection */
		if (intrsct && between)
			{

			/* Determine along test line to intersection (if required) */
			if (dist)
				{
				adist  = line_slen(tline, 0.0, (float) (ip-1));
				dx     = xpos[X] - ppos[X];
				dy     = xpos[Y] - ppos[Y];
				adist += hypot(dx, dy);
				}

			/* Return distance to intersection */
			if (point)    copy_point(point, xpos);
			if (dist)     *dist     = adist;
			if (approach) *approach = appr;
			if (span)     *span     = isp;
			if (tspan)    *tspan    = ip-1;
			return TRUE;
			}

		/* Is this the closest so far? */
		if (first)
			{
			copy_point(apos, xpos);
			amin  = appr;
			aisp  = isp;
			aip   = ip-1;
			first = FALSE;
			}
		else if (appr < amin)
			{
			copy_point(apos, xpos);
			amin  = appr;
			aisp  = isp;
			aip   = ip-1;
			}

		/* Reset start point to check next span */
		copy_point(ppos, npos);
		}

	/* Error if no usable spans found */
	if (first) return FALSE;

	/* Determine along test line to closest approach (if required) */
	if (dist)
		{
		adist  = line_slen(tline, 0.0, (float) aip);
		dx     = apos[X] - tline->points[aip][X];
		dy     = apos[Y] - tline->points[aip][Y];
		adist += hypot(dx, dy);
		}

	/* Return closest approach if no intersection found */
	if (point)    copy_point(point, apos);
	if (dist)     *dist     = adist;
	if (approach) *approach = amin;
	if (span)     *span     = aisp;
	if (tspan)    *tspan    = aip;
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*     i n b o x _ l i n e                                              *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Determine if the given line passes through the given box.
 *
 *	@param[in] 	line	given line
 *	@param[in] 	*box	box to test against
 * @return True if line passes through the box.
 *********************************************************************/

LOGICAL	inbox_line

	(
	LINE		line,
	const BOX	*box
	)

	{
	int		ip;

	if (!line) return FALSE;
	if (!box)   return FALSE;

	for (ip=0; ip<line->numpts; ip++)
		{
		if (inside_box(box, line->points[ip])) return TRUE;
		}

	return FALSE;
	}

/***********************************************************************
*                                                                      *
*     c o n t i g u o u s _ l i n e                                    *
*     c o n t i g u o u s _ l i n e _ s e g m e n t s                  *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Determine if the given line is acceptable by checking the angle
 * from the test point to consecutive points on the line.
 *
 *	@param[in] 	line		given line
 *	@param[in] 	ptest		given test point
 *	@param[in] 	maxang		angle from test point to line
 *  @return True if line is acceptable.
 *********************************************************************/

LOGICAL	contiguous_line

	(
	LINE	line,
	POINT	ptest,
	float	maxang
	)

	{
	int		ip;
	double	dx, dy, ang0, ang, angp, angm;

	if (!ptest)           return FALSE;
	if (!line)            return FALSE;
	if (line->numpts < 2) return FALSE;
	if (maxang <= 0.0)    return FALSE;
	if (maxang >= 180.0)  return FALSE;

	/* Check consecutive points for big jumps */
	dx = (double) (line->points[0][X] - ptest[X]);
	dy = (double) (line->points[0][Y] - ptest[Y]);
	ang0 = atan2(dy, dx) / RAD;
	for (ip=1; ip<line->numpts; ip++)
		{
		dx = (double) (line->points[ip][X] - ptest[X]);
		dy = (double) (line->points[ip][Y] - ptest[Y]);
		ang  = atan2(dy, dx) / RAD;
		angp = ang + 360.0;
		angm = ang - 360.0;
		if ((fabs(ang-ang0) > (double) maxang)
				&& (fabs(angp-ang0) > (double) maxang)
				&& (fabs(angm-ang0) > (double) maxang)) return FALSE;
		ang0 = ang;
		}

	/* Made it through with no big jumps */
	return TRUE;
	}

/**********************************************************************/

/*********************************************************************/
/** Return acceptable segments from the given line by checking the
 * angle from the test point to consecutive points on the line.
 *
 *	@param[in] 	line		line
 *	@param[in] 	ptest		test point
 *	@param[in] 	maxang		angle from test point to line
 *	@param[out]	**segments	contiguous line segments from line
 *  @return The size of the segment list.
 *********************************************************************/
int		contiguous_line_segments

	(
	LINE	line,
	POINT	ptest,
	float	maxang,
	LINE	**segments
	)

	{
	int		nsegs, ip, isg;
	double	dx, dy, ang0, ang, angp, angm;
	LINE	*lsegs = NullLineList;

	if (!ptest)           return 0;
	if (!line)            return 0;
	if (line->numpts < 2) return 0;
	if (maxang <= 0.0)    return 0;
	if (maxang >= 180.0)  return 0;

	nsegs = 0;
	if (segments) *segments = NullLineList;

	/* Create first line segment */
	isg = nsegs++;
	lsegs = GETMEM(lsegs, LINE, nsegs);
	lsegs[isg] = create_line();
	(void) add_point_to_line(lsegs[isg], line->points[0]);

	/* Check consecutive points for big jumps */
	dx = (double) (line->points[0][X] - ptest[X]);
	dy = (double) (line->points[0][Y] - ptest[Y]);
	ang0 = atan2(dy, dx) / RAD;
	for (ip=1; ip<line->numpts; ip++)
		{
		dx = (double) (line->points[ip][X] - ptest[X]);
		dy = (double) (line->points[ip][Y] - ptest[Y]);
		ang  = atan2(dy, dx) / RAD;
		angp = ang + 360.0;
		angm = ang - 360.0;
		/* Create new line segment if big jump is found */
		if ((fabs(ang-ang0) > (double) maxang)
				&& (fabs(angp-ang0) > (double) maxang)
				&& (fabs(angm-ang0) > (double) maxang))
			{
			/* Re-use current line segment if too few points */
			if (lsegs[isg]->numpts < 2)
				{
				(void) empty_line(lsegs[isg]);
				}
			/* Create new line segment */
			else
				{
				isg = nsegs++;
				lsegs = GETMEM(lsegs, LINE, nsegs);
				lsegs[isg] = create_line();
				}
			}
		(void) add_point_to_line(lsegs[isg], line->points[ip]);
		ang0 = ang;
		}

	/* Check last line segment for too few points */
	if (lsegs[isg]->numpts < 2)
		{
		(void) destroy_line(lsegs[isg]);
		nsegs--;
		}

	/* Return segments and number of segments */
	if (segments) *segments = lsegs;
	return nsegs;
	}

/***********************************************************************
*                                                                      *
*      s c a l e _ l i n e                                             *
*      t r a n s l a t e _ l i n e                                     *
*      r o t a t e _ l i n e                                           *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Scale the given line.
 *
 *	@param[in] 	line	given line
 *	@param[in] 	sx		value to multiply each x coordinate (x scale)
 *	@param[in] 	sy		value to multiply eacy y coordinate (y scale)
 * @return True if successful.
 *********************************************************************/
LOGICAL	scale_line

	(
	LINE	line,
	float	sx,
	float	sy
	)

	{
	int		ip;

	if (!line)             return FALSE;
	if (line->numpts <= 0) return TRUE;
	if (sx==1 && sy==1)    return TRUE;

	for (ip=0; ip<line->numpts; ip++)
		{
		line->points[ip][X] *= sx;
		line->points[ip][Y] *= sy;
		}

	return TRUE;
	}

/*********************************************************************/
/** Translate the given line.
 *
 *	@param[in] 	line	given line
 *	@param[in] 	dx		x offset
 *	@param[in] 	dy		y offset
 *  @return True if successful.
 *********************************************************************/
LOGICAL	translate_line

	(
	LINE	line,
	float	dx,
	float	dy
	)

	{
	int		ip;

	if (!line)             return FALSE;
	if (line->numpts <= 0) return TRUE;
	if (dx==0 && dy==0)    return TRUE;

	for (ip=0; ip<line->numpts; ip++)
		{
		line->points[ip][X] += dx;
		line->points[ip][Y] += dy;
		}

	return TRUE;
	}

/*********************************************************************/
/** Rotate the given line.
 *
 *	@param[in] 	line	given line
 *	@param[in] 	ref		centre of rotation
 *	@param[in] 	angle	angle of rotation
 *  @return True if successful.
 *********************************************************************/
LOGICAL	rotate_line

	(
	LINE	line,
	POINT	ref,
	float	angle
	)

	{
	int		ip;
	float	ca, sa, x, y;

	if (!line)             return FALSE;
	if (line->numpts <= 0) return TRUE;
	if (angle == 0)        return TRUE;

	/* Turn angle into cos and sin */
	ca = cosdeg(angle);
	sa = sindeg(angle);

	for (ip=0; ip<line->numpts; ip++)
		{
		x = line->points[ip][X] - ref[X];
		y = line->points[ip][Y] - ref[Y];

		line->points[ip][X] = x*ca - y*sa + ref[X];
		line->points[ip][Y] = x*sa + y*ca + ref[Y];
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*      l i n e _ t o o _ s h o r t                                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Determine whether the given line is shorter than a given tolerance.
 *
 *	@param[in] 	line		given line
 *	@param[in] 	tol			minimum line length
 *  @return True if line is too short.
 *********************************************************************/

LOGICAL	line_too_short

	(
	LINE	line,
	float	tol
	)

	{
	float	length;

	if (!line)             return TRUE;
	if (line->numpts <= 1) return TRUE;
	if (tol <= 0.0)        return TRUE;

	line_properties(line, NullChar, NullChar, NullFloat, &length);
	return (length < tol)? TRUE: FALSE;
	}

/***********************************************************************
*                                                                      *
*      l i n e _ i n d e x                                             *
*      l i n e _ p o s                                                 *
*      l i n e _ s p a n _ i n f o                                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Convert between line position index, span and point.
 *
 *	@param[in] 	line		given line
 *	@param[in] 	ispan		line span to start from
 *	@param[in] 	dspan		distance from start of line span
 *  @return Index of point a distance at least dspan from the starting
 * 			point.
 *********************************************************************/

float	line_index

	(
	LINE	line,
	int		ispan,
	float	dspan
	)

	{
	float	idx, ds;

	if (IsNull(line))          return 0.0;
	if (ispan < 0)             return 0.0;
	if (ispan >= line->numpts) return 0.0;

	idx = (float) ispan;
	if (dspan == 0.0) return idx;

	/* Walk backwards if dspan is negative */
	if (dspan < 0) return line_walk(line, idx, dspan);

	/* Get length of given span - walk if dspan is longer */
	ds = line_span_info(line, ispan, NullPoint, NullPoint, NullPoint);
	if (dspan >= ds) return line_walk(line, idx, dspan);

	/* Compute index */
	idx += dspan/ds;
	return idx;
	}

/**********************************************************************/

/*********************************************************************/
/** Interpolate a point on the line that produces the given span from
 * the given starting point.
 *
 *	@param[in] 	line		given line
 *	@param[in] 	idx			given line index
 *	@param[out]	*ispan		line span containing position
 *	@param[out]	*dspan		distance from start of line span
 * @return Pointer to a coordinate pair of the interpolated point.
 *********************************************************************/
float	*line_pos

	(
	LINE	line,
	float	idx,
	int		*ispan,
	float	*dspan
	)

	{
	int		isp;
	float	a, ds, dsp;
	POINT	ppos, dp;

	static	POINT	pos;

	if (IsNull(line))        return (float *)ZeroPoint;
	isp = (int)idx;
	if (isp < 0)             return (float *)ZeroPoint;
	if (isp >= line->numpts) return (float *)ZeroPoint;

	/* Get length of given span and compute partial length */
	a   = idx - (float)isp;
	ds  = line_span_info(line, isp, ppos, NullPoint, dp);
	dsp = a*ds;

	/* Return description */
	if (NotNull(ispan)) *ispan = isp;
	if (NotNull(dspan)) *dspan = dsp;

	/* Interpolate point */
	pos[X] = ppos[X] + a*dp[X];
	pos[Y] = ppos[Y] + a*dp[Y];
	return (float *)pos;
	}

/**********************************************************************/

/*********************************************************************/
/**	Lookup information on a given line span.
 *
 *	@param[in] 	line		given line
 *	@param[in] 	ispan		given line span
 *	@param[out]	spos		starting point on line span
 *	@param[out]	epos		ending point on line span
 *	@param[out]	dp			x and y distance from start to end point
 *  @return x/y distance between the start and end of a line.
 *********************************************************************/
float	line_span_info

	(
	LINE	line,
	int		ispan,
	POINT	spos,
	POINT	epos,
	POINT	dp
	)

	{
	float	xs, ys, xe, ye, dx, dy;

	if (spos) copy_point(spos, ZeroPoint);
	if (epos) copy_point(epos, ZeroPoint);
	if (dp)   copy_point(dp,   ZeroPoint);

	if (IsNull(line))          return 0.0;
	if (ispan < 0)             return 0.0;
	if (ispan >= line->numpts) return 0.0;

	/* Get point at start of span */
	xs = line->points[ispan][X];
	ys = line->points[ispan][Y];
	if (spos) set_point(spos, xs, ys);

	if (ispan == line->numpts-1) return 0.0;

	/* Get point at end of span */
	xe = line->points[ispan+1][X];
	ye = line->points[ispan+1][Y];
	if (epos) set_point(epos, xe, ye);

	if (xs==xe && ys==ye) return 0.0;

	/* Compute deltas */
	dx = xe - xs;
	dy = ye - ys;
	if (dp) set_point(dp, dx, dy);

	/* Compute length */
	return hypot((double) dx, (double) dy);
	}

/***********************************************************************
*                                                                      *
*      l i n e _ s l e n                                               *
*      l i n e _ w a l k                                               *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Compute length of given segment.
 *
 *	@param[in] 	line		given line
 *	@param[in] 	idx1		line index to start
 *	@param[in] 	idx2		line index to end
 *  @return Length of segment.
 *********************************************************************/

float	line_slen

	(
	LINE	line,
	float	idx1,
	float	idx2
	)

	{
	int		isp, isp1, isp2;
	float	a1, a2, ds1, ds2, ds;

	if (IsNull(line))         return 0;

	/* Compute span and ratio for first index */
	isp1 = (int)idx1;
	a1   = idx1 - (float)isp1;
	if (isp1 < 0)             return 0;
	if (isp1 >= line->numpts) return 0;

	/* Compute span and ratio for second index */
	isp2 = (int)idx2;
	a2   = idx2 - (float)isp2;
	if (isp2 < 0)             return 0;
	if (isp2 >= line->numpts) return 0;

	/* Compute partial length of first span */
	/* Handle special case if it ends in the same span */
	ds1 = line_span_info(line, isp1, NullPoint, NullPoint, NullPoint);
	if (isp1 == isp2)
		{
		ds = (a2-a1)*ds1;
		return ds;
		}
	ds = (1-a1)*ds1;

	/* Accumulate lengths of intervening spans */
	for (isp=isp1+1; isp<isp2; isp++)
		{
		ds += line_span_info(line, isp, NullPoint, NullPoint, NullPoint);
		}

	/* Accumulate partial length of final span */
	ds2 = line_span_info(line, isp2, NullPoint, NullPoint, NullPoint);
	ds += a2*ds2;
	return ds;
	}

/**********************************************************************/

/*********************************************************************/
/** Move by given length along the line segment.
 *
 *	@param[in] 	line	given line
 *	@param[in] 	idx		line index to start
 *	@param[in] 	dist	distance to walk
 *  @return	The index in the line object at the end of the span.
 *********************************************************************/
float	line_walk

	(
	LINE	line,
	float	idx,
	float	dist
	)

	{
	LOGICAL	closed;
	int		isp, lp;
	float	a, b, dsp, ds, tds;

	/* Return if no line */
	if (IsNull(line))      return 0;
	if (line->numpts <= 1) return 0;

	/* Set line parameters */
	lp     = line->numpts - 1;
	closed = line_closed(line);

	/* Reset line index (if required) */
	if (idx < 0.0)
		{
		pr_warning("Line.Walk",
			"Index (%.4f) before start of line (0)!\n", idx);
		if (closed) while (idx < 0.0) idx += (float) lp;
		else                          idx  = 0.0;
		}
	else if (idx > (float) lp)
		{
		pr_warning("Line.Walk",
			"Index (%.4f) after end of line (%d)!\n", idx, lp);
		if (closed) while (idx > (float) lp) idx -= (float) lp;
		else                                 idx  = (float) lp;
		}

	/* Return index if no distance to walk */
	if (dist == 0.0) return idx;

	/* Compute span and ratio for given index */
	isp = (int) idx;
	a   = idx - (float)isp;

	/* Get length of initial span */
	ds = line_span_info(line, isp, NullPoint, NullPoint, NullPoint);

	/* Walk forward! */
	if (dist > 0)
		{

		/* Compute partial length of first span */
		/* If longer than dist compute resulting index */
		dsp = (1-a)*ds;
		if (dsp > dist)
			{
			b = dist/ds + a;
			return ((float) isp + b);
			}

		/* Accumulate distance walked */
		tds = dsp;

		/* Walk through following spans */
		while (TRUE)
			{

			/* Reset span index */
			if (isp < lp-1) isp++;

			/* Walk past end of line! */
			else
				{
				if (closed)
					{
					pr_diag("Line.Walk",
						"Walking from (%.4f) past end of closed line (%d)!\n",
							idx, lp);
					pr_diag("Line.Walk",
						"  Distance walked %.4f of %.4f \n", tds, dist);
					isp = 0;
					}
				else
					{
					if (!fcompare(tds, dist, dist, 1e-5))
						{
						pr_warning("Line.Walk",
							"Walked from (%.4f) to end of line (%d)!\n",
								idx, lp);
						pr_warning("Line.Walk",
							"  Distance walked %.4f of %.4f \n", tds, dist);
						}
					return (float) lp;
					}
				}

			/* Compute length of next span */
			/* If walked length exceeds dist compute resulting index */
			ds = line_span_info(line, isp, NullPoint, NullPoint, NullPoint);
			if (ds == 0.0) continue;
			if (tds+ds > dist)
				{
				b = (dist-tds)/ds;
				return ((float) isp + b);
				}

			/* Accumulate distance walked */
			tds +=ds;
			}
		}

	/* Walk backward! */
	else
		{
		dist = -dist;

		/* Compute partial length of first span */
		/* If longer than dist compute resulting index */
		dsp = a*ds;
		if (dsp > dist)
			{
			b = -dist/ds + a;
			return ((float) isp + b);
			}

		/* Accumulate distance walked */
		tds = dsp;

		/* Walk through preceding spans */
		while (TRUE)
			{

			/* Reset span index */
			if (isp > 0) isp--;

			/* Walk past start of line! */
			else
				{
				if (closed)
					{
					pr_diag("Line.Walk",
						"Walking from (%.4f) past start of closed line (0)!\n",
							idx);
					pr_diag("Line.Walk",
						"  Distance walked %.4f of %.4f \n", tds, dist);
					isp = lp-1;
					}
				else
					{
					if (!fcompare(tds, dist, dist, 1e-5))
						{
						pr_warning("Line.Walk",
							"Walked from (%.4f) to start of line (0)!\n", idx);
						pr_warning("Line.Walk",
							"  Distance walked %.4f of %.4f \n", tds, dist);
						}
					return (float) 0;
					}
				}

			/* Compute length of next span */
			/* If walked length exceeds dist compute resulting index */
			ds = line_span_info(line, isp, NullPoint, NullPoint, NullPoint);
			if (ds == 0.0) continue;
			if (tds+ds > dist)
				{
				b = 1 - (dist-tds)/ds;
				return ((float) isp + b);
				}

			/* Accumulate distance walked */
			tds +=ds;
			}
		}
	}

/***********************************************************************
*                                                                      *
*      g r e a t _ c i r c l e _ l i n e _ l e n g t h                 *
*                                                                      *
*      Compute length of line by summing great circle segments         *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/**	Compute length of line by summing great circle segments
 *
 * @param	*mproj	Map Projection
 * @param	line	Line feature
 * @return	great circle line length of line feature.
***********************************************************************/
float	great_circle_line_length

	(
	const MAP_PROJ	*mproj,		/* Map projection */
	LINE			line		/* Line feature */
	)

	{
	int			ii;
	float		x, y, xprev, yprev;
	POINT		spos, epos;
	float		flen;

	if (IsNull(mproj))    return 0;
	if (IsNull(line))     return 0;
	if (line->numpts < 2) return 0;

	/* Set first point parameters */
	x = line->points[0][X];
	y = line->points[0][Y];

	/* Sum the great circle distances from 2nd to last point */
	flen = 0.0;
	for (ii=1; ii<line->numpts; ii++)
		{
		xprev = x;
		yprev = y;
		x     = line->points[ii][X];
		y     = line->points[ii][Y];
		if (x == xprev && y == yprev) continue;

		(void) set_point(spos, xprev, yprev);
		(void) set_point(epos, x, y);
		flen += great_circle_distance(mproj, spos, epos);
		}

	/* Return the total line length */
	return flen;
	}

/***********************************************************************
*                                                                      *
*      m a t c h i n g _ l i n e s                                     *
*                                                                      *
*      Determine if two lines are copies of each other.                *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/**	Determine if two lines are copies of each other.
 *
 * @param	line1	First line to compare
 * @param	line2	Second line to compare
 * @return	True if all points in lines match.
***********************************************************************/
LOGICAL		matching_lines

	(
	LINE	l1,
	LINE	l2
	)

	{
	LOGICAL	fwd;
	int		np, ii, ix;
	float	x1, y1, x2, y2, xr, yr;

	if (!l1)            return FALSE;
	if (l1->numpts < 2) return FALSE;
	if (!l2)            return FALSE;
	if (l2->numpts < 2) return FALSE;

	if (l1->numpts != l2->numpts) return FALSE;

	/* Compare first and last points to check for line reversal */
	np = l1->numpts;
	x1 = l1->points[0][X]; x2 = l2->points[0][X]; xr = l2->points[np-1][X];
	y1 = l1->points[0][Y]; y2 = l2->points[0][Y]; yr = l2->points[np-1][Y];
	if ((x1 != x2 || y1 != y2) && (x1 != xr || y1 != yr)) return FALSE;
	if      (x1 == x2 && y1 == y2) fwd = TRUE;
	else if (x1 == xr && y1 == yr) fwd = FALSE;

	/* Compare remaining points in each line */
	for (ii=1; ii<np; ii++)
		{
		if (fwd) ix = ii;
		else     ix = np-ii-1;
		x1 = l1->points[ii][X]; x2 = l2->points[ix][X];
		y1 = l1->points[ii][Y]; y2 = l2->points[ix][Y];
		if (x1 != x2 || y1 != y2) return FALSE;
		}

	/* Return TRUE if all points match */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*   STATIC Functions                                                   *
*                                                                      *
*   The following functions can only be used internally.               *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*     l i n e _ o r i e n t a t i o n                                  *
*                                                                      *
*     determine orientation of closed line wrt itself                  *
*                                                                      *
***********************************************************************/

static	LOGICAL	line_orientation

	(
	LINE	line,	/* given line */
	LOGICAL	*right	/* is enclosed area to right? (only meaningful if closed) */
	)

	{
	float	x, xprev, xtest, xnext, dx, dxn, dxp;
	float	y, yprev, ytest, ynext, dy, dyn, dyp;
	float	xmid, ymid, dz, dzn, dzp, dzt, dzz, ang;
	int		iprev, itest, inext, i, npts;
	POINT	rpos, lpos;
	LOGICAL	rin, lin;

	LOGICAL	First = TRUE;
	LOGICAL	Test  = TRUE;
	LOGICAL	AngOK = FALSE;
	LOGICAL	Fgood = TRUE;

	static LINE	xline = NullLine;

	/* Initialize to nice garbage */
	if (right) *right = TRUE;

	/* Return if line not there or not closed */
	if (IsNull(line))       return FALSE;
	if (!line_closed(line)) return FALSE;

	/* Make copy of line for testing */
	if (NotNull(xline)) xline = destroy_line(xline);
	xline = copy_line(line);
	(void) condense_line(xline);
	if (xline->numpts <= 3) return FALSE;

#	ifdef DEBUG_ORIENTATION
	(void) fprintf(stderr,
		"[line_orientation] Checking line with: %d points\n", line->numpts);
#	endif /* DEBUG_ORIENTATION */

	/* Loop through points in line */
	iprev = itest = inext = -1;
	npts  = xline->numpts;
	for (i=0; i<npts; i++)
		{

		/* Extract point coordinates */
		x = xline->points[i][X];
		y = xline->points[i][Y];

		/* Set first point */
		if (First)
			{
			iprev = i;
			xprev = x;
			yprev = y;
			First = FALSE;
			continue;
			}

		/* Set test point */
		if (Test)
			{
			itest = i;
			xtest = x;
			ytest = y;
			dx = xtest - xprev;
			dy = ytest - yprev;
			if ((dx==0) && (dy==0)) continue;

			/* Found test point */
			dz = (float) hypot((double) dx, (double) dy);
			Test = FALSE;
			continue;
			}

		/* Set next point */
		if (Fgood) inext = i;
		xnext = x;
		ynext = y;
		dxn = xnext - xtest;
		dyn = ynext - ytest;
		if ((dxn==0) && (dyn==0)) continue;

		/* Found next point */
		dzn = (float) hypot((double) dxn, (double) dyn);

		/* Determine cosine of angle about xtest/ytest */
		dxp = xnext - xprev;
		dyp = ynext - yprev;
		dzp = (float) hypot((double) dxp, (double) dyp);
		ang = (dz*dz + dzn*dzn - dzp*dzp) / (2.0 * dz * dzn);

		/* Angle must be greater than 20 degrees or so */
		if (ang > 0.94)
			{
#			ifdef DEBUG_ORIENTATION
			(void) fprintf(stderr,
				"[line_orientation]  Bad angle: %.3f  at point: %d\n",
				ang, i-1);
#			endif /* DEBUG_ORIENTATION */

			/* Reset parameters and look for next point */
			AngOK = FALSE;
			if (Fgood) iprev = itest;
			xprev = xtest;
			yprev = ytest;
			if (Fgood) itest = inext;
			xtest = xnext;
			ytest = ynext;
			dx = dxn;
			dy = dyn;
			dz = dzn;
			continue;
			}

#		ifdef DEBUG_ORIENTATION
		(void) fprintf(stderr,
			"[line_orientation]  Found good angle: %.3f  at point: %d\n",
			ang, i-1);
#		endif /* DEBUG_ORIENTATION */

		/* Fix location of span for first good angle */
		if (Fgood) Fgood = FALSE;

		/* Look for two good angles in a row */
		if (!AngOK)
			{
			AngOK = TRUE;

			/* Reset parameters and look for next point */
			xprev = xtest;
			yprev = ytest;
			xtest = xnext;
			ytest = ynext;
			dx = dxn;
			dy = dyn;
			dz = dzn;
			continue;
			}

		/* Set points just to right and left of xtest/ytest span */
		dzt  = dz / 10.0;
		dzz  = dz / 50.0;
		xmid = (xtest + xprev) / 2.0;
		ymid = (ytest + yprev) / 2.0;
		(void) set_point(rpos, xmid, ymid);
		(void) set_point(lpos, xmid, ymid);

		/* Near vertical cases */
		if (fabs((double) dx) < dzt && dy > 0)
			{
			rpos[X] += dzz; lpos[X] -=dzz;
			}
		else if (fabs((double) dx) < dzt)
			{
			rpos[X] -= dzz; lpos[X] +=dzz;
			}

		/* Near horizontal cases */
		else if (fabs((double) dy) < dzt && dx > 0)
			{
			rpos[Y] -= dzz; lpos[Y] +=dzz;
			}
		else if (fabs((double) dy) < dzt)
			{
			rpos[Y] += dzz; lpos[Y] -=dzz;
			}

		/* Remaining cases */
		else if (dx > 0.0 && dy > 0.0)
			{
			rpos[X] += dzz; rpos[Y] -= dzz; lpos[X] -=dzz; lpos[Y] +=dzz;
			}
		else if (dx > 0.0)
			{
			rpos[X] -= dzz; rpos[Y] -= dzz; lpos[X] +=dzz; lpos[Y] +=dzz;
			}
		else if (dx < 0.0 && dy > 0.0)
			{
			rpos[X] += dzz; rpos[Y] += dzz; lpos[X] -=dzz; lpos[Y] -=dzz;
			}
		else
			{
			rpos[X] -= dzz; rpos[Y] += dzz; lpos[X] +=dzz; lpos[Y] -=dzz;
			}

		/* Check which one is inside */
		(void) line_test_point(xline, rpos, NullFloat, NullPoint,
				NullInt, &rin, NullChar);
		(void) line_test_point(xline, lpos, NullFloat, NullPoint,
				NullInt, &lin, NullChar);

		/* Problem determining orientation on this span     */
		/*  ... so reset parameters and look for next point */
		if ((lin && rin) || (!lin && !rin))
			{
			xprev = xtest;
			yprev = ytest;
			xtest = xnext;
			ytest = ynext;
			dx = dxn;
			dy = dyn;
			dz = dzn;
			continue;
			}

		/* Return orientation */
		if (right) *right = rin;
		return TRUE;
		}

	/* Problem with points */
	(void) pr_error("Lines",
		"Problem with line orientation! Trying best guess!\n");
	(void) pr_error("Lines",
		"  ... iprev: %d  itest: %d  inext: %d\n", iprev, itest, inext);

	/* Determine orientation from first good angle found */
	if (iprev >= 0 && itest >= 1 && inext >= 2)
		{

#		ifdef DEBUG_ORIENTATION
		(void) fprintf(stderr,
			"[line_orientation]  Test before good angle at point: %d\n",
			itest);
#		endif /* DEBUG_ORIENTATION */

		/* Set first point */
		xprev = xline->points[iprev][X];
		yprev = xline->points[iprev][Y];

		/* Set test point */
		xtest = xline->points[itest][X];
		ytest = xline->points[itest][Y];
		dx = xtest - xprev;
		dy = ytest - yprev;
		dz = (float) hypot((double) dx, (double) dy);

		/* Set points just to right and left of xtest/ytest span */
		/*  ... but test close to the good angle */
		dzt  = dz /  10.0;
		dzz  = dz / 150.0;
		xmid = xtest*0.9 + xprev*0.1;
		ymid = ytest*0.9 + yprev*0.1;
		(void) set_point(rpos, xmid, ymid);
		(void) set_point(lpos, xmid, ymid);

		/* Near vertical cases */
		if (fabs((double) dx) < dzt && dy > 0)
			{
			rpos[X] += dzz; lpos[X] -=dzz;
			}
		else if (fabs((double) dx) < dzt)
			{
			rpos[X] -= dzz; lpos[X] +=dzz;
			}

		/* Near horizontal cases */
		else if (fabs((double) dy) < dzt && dx > 0)
			{
			rpos[Y] -= dzz; lpos[Y] +=dzz;
			}
		else if (fabs((double) dy) < dzt)
			{
			rpos[Y] += dzz; lpos[Y] -=dzz;
			}

		/* Remaining cases */
		else if (dx > 0.0 && dy > 0.0)
			{
			rpos[X] += dzz; rpos[Y] -= dzz; lpos[X] -=dzz; lpos[Y] +=dzz;
			}
		else if (dx > 0.0)
			{
			rpos[X] -= dzz; rpos[Y] -= dzz; lpos[X] +=dzz; lpos[Y] +=dzz;
			}
		else if (dx < 0.0 && dy > 0.0)
			{
			rpos[X] += dzz; rpos[Y] += dzz; lpos[X] -=dzz; lpos[Y] -=dzz;
			}
		else
			{
			rpos[X] -= dzz; rpos[Y] += dzz; lpos[X] +=dzz; lpos[Y] -=dzz;
			}

		/* Check which one is inside */
		(void) line_test_point(xline, rpos, NullFloat, NullPoint,
				NullInt, &rin, NullChar);
		(void) line_test_point(xline, lpos, NullFloat, NullPoint,
				NullInt, &lin, NullChar);

		/* Return orientation if one is inside */
		if ((lin && !rin) || (!lin && rin))
			{
			if (right) *right = rin;
			return TRUE;
			}

#		ifdef DEBUG_ORIENTATION
		(void) fprintf(stderr,
			"[line_orientation]  Test after good angle at point: %d\n",
			itest);
#		endif /* DEBUG_ORIENTATION */

		/* Set test point */
		xtest = xline->points[itest][X];
		ytest = xline->points[itest][Y];

		/* Set next point */
		xnext = xline->points[inext][X];
		ynext = xline->points[inext][Y];
		dx = xnext - xtest;
		dy = ynext - ytest;
		dz = (float) hypot((double) dx, (double) dy);

		/* Set points just to right and left of xnext/ynext span */
		/*  ... but test close to the good angle */
		dzt  = dz /  10.0;
		dzz  = dz / 150.0;
		xmid = xtest*0.9 + xnext*0.1;
		ymid = ytest*0.9 + ynext*0.1;
		(void) set_point(rpos, xmid, ymid);
		(void) set_point(lpos, xmid, ymid);

		/* Near vertical cases */
		if (fabs((double) dx) < dzt && dy > 0)
			{
			rpos[X] += dzz; lpos[X] -=dzz;
			}
		else if (fabs((double) dx) < dzt)
			{
			rpos[X] -= dzz; lpos[X] +=dzz;
			}

		/* Near horizontal cases */
		else if (fabs((double) dy) < dzt && dx > 0)
			{
			rpos[Y] -= dzz; lpos[Y] +=dzz;
			}
		else if (fabs((double) dy) < dzt)
			{
			rpos[Y] += dzz; lpos[Y] -=dzz;
			}

		/* Remaining cases */
		else if (dx > 0.0 && dy > 0.0)
			{
			rpos[X] += dzz; rpos[Y] -= dzz; lpos[X] -=dzz; lpos[Y] +=dzz;
			}
		else if (dx > 0.0)
			{
			rpos[X] -= dzz; rpos[Y] -= dzz; lpos[X] +=dzz; lpos[Y] +=dzz;
			}
		else if (dx < 0.0 && dy > 0.0)
			{
			rpos[X] += dzz; rpos[Y] += dzz; lpos[X] -=dzz; lpos[Y] -=dzz;
			}
		else
			{
			rpos[X] -= dzz; rpos[Y] += dzz; lpos[X] +=dzz; lpos[Y] -=dzz;
			}

		/* Check which one is inside */
		(void) line_test_point(xline, rpos, NullFloat, NullPoint,
				NullInt, &rin, NullChar);
		(void) line_test_point(xline, lpos, NullFloat, NullPoint,
				NullInt, &lin, NullChar);

		/* Return orientation if one is inside */
		if ((lin && !rin) || (!lin && rin))
			{
			if (right) *right = rin;
			return TRUE;
			}
		}

	/* Problem with points */
	(void) pr_error("Lines", "Cannot determine line orientation!\n");
	return FALSE;
	}
