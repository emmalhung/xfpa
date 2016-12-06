/***************************************************************************/
/*
 *     File: draw.c
 *
 *  Purpose: Contains most of the public functions used for drawing.
 *
*     Version 8 (c) Copyright 2011 Environment Canada
*
*   This file is part of the Forecast Production Assistant (FPA).
*   The FPA is free software: you can redistribute it and/or modify it
*   under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   any later version.
*
*   The FPA is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*   See the GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.
 */
/***************************************************************************/

#include <limits.h>
#include <fpa_math.h>
#include "FpaXglP.h"

#define DWLGC	D,WX->draw,WX->linegc		/* For drawing lines */
#define DWFGC	D,WX->draw,WX->fillgc		/* For fills */
#define MASK	pp.have_holes||W->fs>1000

/* Set cursor positions. Ex.: SCP(=x,=y), SCP(+=x,+=y); */
#define SCP(gx,gy) { W->xp gx;W->yp gy; }
#define IFLOOR(x)  (((int)(x)) - ((x) < 0 && (x) != ((float)((int)(x))) ? 1 : 0))


/***************************************************************************/
/*
 *  Queue handling function. The input points are clipped to a window
 *  just slightly bigger than the viewport so that the many points clipped
 *  to the border will not be displayed. The lines are clipped using an
 *  algorithm based on the Liang-Barsky technique for fast clipping to
 *  non-rotated (vertical) rectangles. A search on the web will turn up
 *  many references to this techinque and its advantages.
*/
/***************************************************************************/

static int     PMode  = Complex;
static int     q_len  = 0;
static int     q_max  = 0;
static XPoint *queue  = NULL;


static void q_add(int x, int y)
{
	if(q_max <= q_len) queue = GETMEM(queue, XPoint, q_max+=100);

	/* Add point and make sure it does not overflow the short data type
	*/
	queue[q_len].x = (short) MIN(MAX(x,SHRT_MIN),SHRT_MAX);
	queue[q_len].y = (short) MIN(MAX(y,SHRT_MIN),SHRT_MAX);

	/* Accept into the queue only if different from the previous point.
	*/
	if(	q_len == 0 ||
		queue[q_len].x != queue[q_len-1].x ||
		queue[q_len].y != queue[q_len-1].y    ) q_len++;
}



/* --------------- Begin Liang-Barsky line clipping algorithm --------------- */

static LOGICAL cliptest(float p, float q, float *u1, float *u2)
{
	float  r;
	LOGICAL result = TRUE;

	if (p < 0)
	{
		r = q/p;
		if (r > *u2)
			result = FALSE;
		else if (r > *u1)
			*u1 = r;
	}
	else if (p > 0)
	{
		r = q/p;
		if (r < *u1)
			result = FALSE;
		else if (r < *u2)
			*u2 = r;
	}
	else if (q < 0)
	{
		result = FALSE;
	}
	return result;
}


/* Line clipping returns TRUE if the line is within the clipping
 * window and FALSE if it is not or if the line is exiting the
 * clipping window. The pp argument is the previous point in the
 * line array.
 */
static LOGICAL clipline(int x, int y, POINT pp)
{
	LOGICAL reject  = TRUE;
	LOGICAL leaving = FALSE;

	float dx = (float) x;
	float dy = (float) y;
	float u1 = 0.0;
	float u2 = 1.0;
	float Dx = dx - pp[X];

	if(q_max <= q_len+1) queue = GETMEM(queue, XPoint, q_max+=100);

	/* Check for line crossing and clip to the window
	 */
	if( cliptest(-Dx, pp[X], &u1, &u2) )
	{
		if( cliptest(Dx, ((float) W->xm - pp[X]), &u1, &u2) )
		{
			float Dy = dy - pp[Y];
			if( cliptest(-Dy, pp[Y], &u1, &u2) )
			{
				if( cliptest(Dy, ((float) W->ym - pp[Y]), &u1, &u2) )
				{
					if( u2 < 1.0 )
					{
						leaving = TRUE;
						dx = pp[X] + u2*Dx;
						dy = pp[Y] + u2*Dy;
					}
					if( u1 > 0.0 )
					{
						pp[X] = pp[X] + u1*Dx;
						pp[Y] = pp[Y] + u1*Dy;
					}
					reject = FALSE;
				}
			}
		}
	}

	if (reject)
	{
		pp[X] = (float) x;
		pp[Y] = (float) y;
		return FALSE;
	}

	/* If the queue is empty add in the first point */
	if(q_len == 0)
		q_add((int) pp[X], (int) pp[Y]);

	/* Add in the current point */
	q_add((int) dx, (int) dy);

	pp[X] = (float) x;
	pp[Y] = (float) y;

	return (!leaving);
}

/* --------------- End Liang-Barsky line clipping algorithm --------------- */



/* ---------- Begin Sutherland-Hodgman Polygon Clipping -------------- */

#define CP_LEFT 0
#define CP_RIGHT 1
#define CP_TOP 2
#define CP_BOTTOM 3

struct clipwin {
	float t; /* Top */
	float b; /* Bottom */
	float l; /* Left */
	float r; /* Right */
};

struct point {
	float x;
	float y;
};


/* Returns true if the point is inside of the clipping rectangle.
 */
static int inside( struct point p, struct clipwin r, int side )
{
	switch( side )
	{
		case CP_LEFT:   return (p.x >= r.l);
		case CP_RIGHT:  return (p.x <= r.r);
		case CP_TOP:    return (p.y <= r.t);
		case CP_BOTTOM: return (p.y >= r.b);
	}
}


/* Finds the intersection of the line defined by the two points on the clipping rectangle.
 */
static struct point intersect( struct point p, struct point q, struct clipwin r, int side )
{
	struct point t;
	float a, b;

	/* find slope and intercept of segment pq */
	a = (q.x != p.x)? (( q.y - p.y ) / ( q.x - p.x )) : 0.0;
	b = p.y - p.x * a;

	switch( side )
	{
		case CP_LEFT:
			t.x = r.l;
			t.y = t.x * a + b;
			break;
		case CP_RIGHT:
			t.x = r.r;
			t.y = t.x * a + b;
			break;
		case CP_TOP:
			t.y = r.t;
			if( a != 0 )
				t.x = ( t.y - b ) / a;
			else
				t.x = p.x;
			break;
		case CP_BOTTOM:
			t.y = r.b;
			if( a != 0 )
				t.x = ( t.y - b ) / a;
			else
				t.x = p.x;
			break;
	}

	return t;
}	


/* Clip polygon points to given side.
 */
static void clipplane( int *np, struct point *in, struct point *out, struct clipwin r, int side )
{
	int i, j=0;
	struct point s, p;

	s = in[*np-1];
	for( i = 0 ; i < *np ; i++ )
	{
		p = in[i];

		if( inside( p, r, side ) )
		{
			/* point p is "inside" */
			if( !inside( s, r, side ) )
			{
				/* p is "inside" and s is "outside" */
				out[j] = intersect( p, s, r, side );
				j++;
			}
			out[j] = p;
		       	j++;
		}
		else if( inside( s, r, side ) )
		{
			/* s is "inside" and p is "outside" */
			out[j] = intersect( s, p, r, side );
			j++;
		}

		s = p;
	}

	/* set return values */
	*np = j;
	for( i = 0 ; i < *np ; i++ )
	{
		in[i] = out[i];
	}
}


/* Clip the given polygon to a rectangle just slightly larger than the
 * rendering window and put the polygon into the X queue. As the
 * clipping function will leave parts of polygons running along the
 * border this ensures that these will be clipped by X on display.
 */
static void clippoly( int np, Coord (*vp)[2])
{
	int i;
	struct clipwin r;
	struct point *o;

	static int maxp = 0;
	static struct point *p = NULL;

	q_len = 0;

	/* A polygon needs at least 3 points */
	if( np < 3 ) return;

	/* Allocate the array and keep it around as this function gets
	 * called very often.
	 */
	i = (np+1)*2;
	if( maxp <= i )
	{
		maxp = i + 100;
		p = GETMEM(p, struct point, maxp);
	}
	o = p + np + 1;

	/* Map input to screen */
	for(i = 0; i < np; i++)
	{
		p[i].x = (float) XS(vp[i][X],vp[i][Y]);
		p[i].y = (float) YS(vp[i][X],vp[i][Y]);
	}

	/* Check to make sure the polygon is closed */
	if(p[0].x != p[np-1].x || p[0].y != p[np-1].y)
	{
		p[np] = p[0];
		np++;
	}

	r.b = -1.0;
	r.t = (float) (W->ym + 1);
	r.l = -1;
	r.r = (float) (W->xm + 1);
	
	           clipplane( &np, p, o, r, CP_LEFT );
	if(np > 0) clipplane( &np, p, o, r, CP_RIGHT );
	if(np > 0) clipplane( &np, p, o, r, CP_TOP );
	if(np > 0) clipplane( &np, p, o, r, CP_BOTTOM );

	for(i = 0; i < np; i++)
		q_add((int) p[i].x, (int) p[i].y);
}

/* ---------- End Sutherland-Hodgman Polygon Clipping -------------- */


/* Rectangle clipping. If the input rectangle is not within the
 * rendering window FALSE is returned. If some part is within the
 * window the rectangle is clipped and TRUE returned. The clip
 * size is bigger than the actual window to allow for thick lines.
 */
static LOGICAL cliprect( int *x, int *y, int *width, int *height)
{
	int x1 = *x;
	int y1 = *y;
	int x2 = x1 + *width;
	int y2 = y1 + *height;
	int xu = (int) (W->xm + 10);
	int yu = (int) (W->ym + 10);

	if( x1 >= xu ) return FALSE;
	if( y1 >= yu ) return FALSE;
	if( x2 <= 0  ) return FALSE;
	if( y2 <= 0  ) return FALSE;

	if( x1 < -10 || y1 < -10 || x2 > xu || y2 > yu )
	{
		x1 = MAX(-10,x1);
		y1 = MAX(-10,y1);
		x2 = MIN(xu,x2);
		y2 = MIN(yu,y2);

		*x = x1;
		*y = y1;
		*width  = x2 - x1;
		*height = y2 - y1;
	}
	return TRUE;
}



/***************************************************************************/
/*
*   Local partial polygon functions.
*/
/***************************************************************************/

/* Structure to hold partial polygon data arrays.
*/
static struct {
	LOGICAL have_holes;
	int     na;
	int    *np;
	XPoint **p;
} pp = { FALSE, 0, NULL, NULL };


/* The data for the partial polygon function is cached until needed
*  by calls to the function which finish up the process.
*/
static void partial_poly_add()
{
	if(q_len < 3) return;

	pp.np = GETMEM(pp.np, int, pp.na+1);
	pp.p  = GETMEM(pp.p, XPoint*, pp.na+1);
	pp.np[pp.na] = q_len;
	pp.p[pp.na] = MEM(XPoint, q_len);
	(void) memcpy((void*)pp.p[pp.na], (void*)queue, (size_t)q_len*sizeof(XPoint));
	pp.na++;
	pp.have_holes = TRUE;
}


static void free_mask(void)
{
	if(pp.have_holes)
	{
		int i;
		for(i = 0; i < pp.na; i++) FREEMEM(pp.p[i]);
		FREEMEM(pp.np);
		FREEMEM(pp.p );
		pp.na = 0;
		pp.have_holes = FALSE;
	}
	XSetClipMask(D, WX->fillgc, None);
}


/***************************************************************************/
/*
 *   Calculate the pattern of hatch lines which would fill the given box.
 *   The pattern is calculated wrt to the origin of the window but returned
 *   wrt to the origin of the given recatngle. This way multiple areas which
 *   are hatched will seem to be over the same hatching pattern.
 *
 *   Writtn by: R.Trafford, 1996/09/20
 */
/***************************************************************************/
static void hatch_box ( float angle, float space, int xorg, int yorg,
						int xlen, int ylen, int *nseg, XSegment	**seglist)
{
	float	ax, ay, s, smin, smax, x, y, xr, yr, xl, yl, xt, yt, xb, yb;
	int		nmin, nmax, n, i;

	/* Segment list to be returned */
	int			num    = 0;
	XSegment	*slist = (XSegment *)0;

	while(angle < 0.0  ) angle += 360.0;
	while(angle > 360.0) angle -= 360.0;

	/* Equation of family of lines is:  x.sin(a) - y.cos(a) = n.d */
	ax =  SINDEG(angle) / space;
	ay = -COSDEG(angle) / space;

	/* Figure out range of parameter n by checking all 4 corners */
	s    = xorg*ax + yorg*ay;
	smin = s;
	smax = s;
	s    = (xorg+xlen)*ax + yorg*ay;
	smin = MIN(smin, s);
	smax = MAX(smax, s);
	s    = xorg*ax + (yorg+ylen)*ay;
	smin = MIN(smin, s);
	smax = MAX(smax, s);
	s    = (xorg+xlen)*ax + (yorg+ylen)*ay;
	smin = MIN(smin, s);
	smax = MAX(smax, s);

	/* Determine how many lines need to be drawn */
	nmin = (int)ceilf(smin);
	nmax = (int)floorf(smax);
	num  = nmax - nmin + 1;
	if (num <= 0)
	{
		if (nseg)    *nseg    = 0;
		if (seglist) *seglist = (XSegment *)0;
		return;
	}

	/* Allocate the segment list */
	slist = INITMEM(XSegment, num);

	/* Determine start and end point on each segment */
	for (n=nmin, i = 0; n<=nmax; n++, i++)
	{
		/* If parallel to the x-axis, only 2 intersections will occur */
		if (ax == 0)
		{
			y = n/ay;
			slist[i].x1 = (short) xorg;
			slist[i].y1 = (short) y;
			slist[i].x2 = (short) (xorg + xlen);
			slist[i].y2 = (short) y;
		}

		/* If parallel to the y-axis, only 2 intersections will occur */
		else if (ay == 0)
		{
			x = n/ax;
			slist[i].x1 = (short) x;
			slist[i].y1 = (short) yorg;
			slist[i].x2 = (short) x;
			slist[i].y2 = (short) (yorg + ylen);
		}

		/* Otherwise check intersections with all 4 edges. Only 2 will be in */
		else
		{
			/* Left edge */
			xl = (float) xorg;
			yl = ((float)n - ax*xl) / ay;

			/* Right edge */
			xr = (float) (xorg + xlen);
			yr = ((float)n - ax*xr) / ay;

			/* Top edge */
			yt = (float) yorg;
			xt = ((float)n - ay*yt) / ax;

			/* Bottom edge */
			yb = (float) (yorg + ylen);
			xb = ((float)n - ay*yb) / ax;

			/* Use left, top, or bottom intersection */
			if (yl < (float)yorg)
			{
				slist[i].x1 = (short)xt;
				slist[i].y1 = (short)yt;
			}
			else if (yl > (float)(yorg+ylen))
			{
				slist[i].x1 = (short)xb;
				slist[i].y1 = (short)yb;
			}
			else
			{
				slist[i].x1 = (short)xl;
				slist[i].y1 = (short)yl;
			}

			/* Use right, top, or bottom intersection */
			if (yr < (float)yorg)
			{
				slist[i].x2 = (short)xt;
				slist[i].y2 = (short)yt;
			}
			else if (yr > (float)(yorg+ylen))
			{
				slist[i].x2 = (short)xb;
				slist[i].y2 = (short)yb;
			}
			else
			{
				slist[i].x2 = (short)xr;
				slist[i].y2 = (short)yr;
			}
		}
	}

	/* Return the results */
	if (nseg)    *nseg    = num;
	if (seglist) *seglist = slist;
}


/***************************************************************************/
/*
*   Set up the clip mask for the drawing functions.
*/
/***************************************************************************/
static void create_mask(int x, int y, int width, int height)
{
	int       nlines = 0;
	XSegment  *lines = NULL;

	switch(W->fs)
	{
		case glPATTERN_HATCH:
			XSetForeground(D, WX->dep1gc, 0);
			XFillRectangle(D, WX->mask, WX->dep1gc, 0, 0, W->xm, W->ym);
			XSetForeground(D, WX->maskgc, 1);
			hatch_box(W->ha, W->hs, x, y, width, height, &nlines, &lines);
			XDrawSegments(D, WX->mask, WX->maskgc, lines, nlines);
			FREEMEM(lines);
			break;

		case glPATTERN_CROSS_HATCH:
			XSetForeground(D, WX->dep1gc, 0);
			XFillRectangle(D, WX->mask, WX->dep1gc, 0, 0, W->xm, W->ym);
			XSetForeground(D, WX->maskgc, 1);
			hatch_box(W->ha, W->hs, x, y, width, height, &nlines, &lines);
			XDrawSegments(D, WX->mask, WX->maskgc, lines, nlines);
			FREEMEM(lines);
			hatch_box(W->ha+W->hc, W->hs, x, y, width, height, &nlines, &lines);
			XDrawSegments(D, WX->mask, WX->maskgc, lines, nlines);
			FREEMEM(lines);
			break;

		default:
			XSetForeground(D, WX->dep1gc, 1);
			XFillRectangle(D, WX->mask, WX->dep1gc, 0, 0, W->xm, W->ym);
			break;
	}

	/* If we have holes punch them in the mask now.
	*/
	if(pp.have_holes)
	{
		int i;
		XSetForeground(D, WX->maskgc, 0);
		for(i = 0; i < pp.na; i++)
		{
			XFillPolygon(D, WX->mask, WX->maskgc, pp.p[i], pp.np[i], PMode, CoordModeOrigin);
		}
	}

	XSetClipOrigin(D, WX->fillgc, 0, 0);
	XSetClipMask(D, WX->fillgc, WX->mask);
}


/* Use the values in the queue to set the mask limits */
static void create_mask_from_queue(void)
{
	int n, width, height;
	XPoint maxpt, minpt;

	if(q_len < 1) return;

	/* Find the boundaries of the polygon we are to create the
	 * clip mask for. The starting limits are set just a little
	 * bigger than the clip window.
	 */
	maxpt.x = -1;
	maxpt.y = -1;
	minpt.x = (short)(W->xm + 1);
	minpt.y = (short)(W->ym + 1);
	for(n = 0; n < q_len; n++)
	{
		minpt.x = MIN(minpt.x, queue[n].x);
		minpt.y = MIN(minpt.y, queue[n].y);
		maxpt.x = MAX(maxpt.x, queue[n].x);
		maxpt.y = MAX(maxpt.y, queue[n].y);
	}
	width  = (int)maxpt.x - (int)minpt.x + 1;
	height = (int)maxpt.y - (int)minpt.y + 1;
	create_mask((int)minpt.x, (int)minpt.y, width, height);
}


/***************************************************************************/
/*
*   Drawing
*/
/***************************************************************************/
void glClear(void)
{
	XFillRectangle(DWFGC, 0, 0, W->xm, W->ym);
}

/* Points */
void glPoint(Coord x, Coord y)
{
	XDrawPoint(DWLGC, XSC(x,y), YSC(x,y));
}

/* Lines */
void glMove (Coord x, Coord y) {SCP( =x, =y);}
void glRMmove  (Coord x, Coord y) {SCP(+=x,+=y);}

void glDraw (Coord x, Coord y)
{
	XDrawLine(DWLGC, XS(W->xp,W->yp), YS(W->xp,W->yp), XS(x,y), YS(x,y));
	SCP( =x, =y);
}

void glRDraw  (Coord x, Coord y)
{
	Coord dx = W->xp+x;
	Coord dy = W->yp+y;
	XDrawLine(DWLGC, XS(W->xp,W->yp), YS(W->xp,W->yp), XS(dx,dy), YS(dx,dy));
	SCP(+=x,+=y);
}

/* Arcs & Circles */
void glArc (Coord x, Coord y, Coord r, Angle s, Angle e)
{
	Coord dx, dy;
	r=MAX(0,r);
	dx = x-r;
	dy = y+r;
	s = 64*s + 0.5;
	e = 64*(e-s) + 0.5;
	XDrawArc(DWLGC, XS(dx,dy), YS(dx,dy), (UNINT) XR(2*r), (UNINT) YR(2*r), IFLOOR(s), IFLOOR(e));
}

void glFilledArc(Coord x, Coord y, Coord r, Angle s, Angle e)
{
	int tx, ty, width, height;

	r = MAX(0,r);
	tx = XS(x-r,y+r);
	ty = YS(x-r,y+r);
	width = XR(2*r);
	height = YR(2*r);
	if(cliprect(&tx, &ty, &width, &height))
	{
		s = 64*s + 0.5;
		e = 64*(e-s) + 0.5;
		if(MASK) create_mask(tx, ty, width, height);
		XFillArc(DWFGC, tx, ty, (UNINT)width, (UNINT)height, IFLOOR(s), IFLOOR(e));
		if(MASK) free_mask();
	}
}

void glArcx(Coord x,Coord y,Coord rx,Coord ry,Angle s,Angle e)
{
	Coord dx = x-rx;
	Coord dy = y+ry;
	s = 64*s + 0.5;
	e = 64*(e-s) + 0.5;
	XDrawArc(DWLGC, XS(dx,dy), YS(dx,dy), (UNINT) XR(2*MAX(0,rx)), (UNINT) YR(2*MAX(0,ry)), IFLOOR(s), IFLOOR(e));
}

void glFilledArcx(Coord x,Coord y,Coord rx,Coord ry,Angle s,Angle e)
{
	int    tx, ty, width, height;

	tx = XS(x-rx,y+ry);
	ty = YS(x-rx,y+ry);
	width = XR(2*MAX(0,rx));
	height = YR(2*MAX(0,ry));
	if(cliprect(&tx, &ty, &width, &height))
	{
		s = 64*s + 0.5;
		e = 64*(e-s) + 0.5;
		if(MASK) create_mask(tx, ty, width, height);
		XFillArc(DWFGC, tx, ty, (UNINT)width, (UNINT)height, IFLOOR(s), IFLOOR(e));
		if(MASK) free_mask();
	}
}

void glCircle(Coord x, Coord y, Coord r)
{
	Coord dx, dy;
	r=MAX(0,r);
	dx = x-r;
	dy = y+r;
	XDrawArc(DWLGC, XS(dx,dy), YS(dx,dy), (UNINT) XR(2*r), (UNINT) YR(2*r), 0, 64*360);
}

void glFilledCircle(Coord x, Coord y, Coord r)
{
	int tx, ty, width, height;

	r = MAX(0,r);
	tx = XS(x-r,y+r);
	ty = YS(x-r,y+r);
	width = XR(2*r);
	height = YR(2*r);
	if(cliprect(&tx, &ty, &width, &height))
	{
		if(MASK) create_mask(tx, ty, width, height);
		XFillArc(DWFGC, tx, ty, (UNINT)width, (UNINT)height, 0, 64*360);
		if(MASK) free_mask();
	}
}

/* Rects & Boxes
*/
void glRectangle(Coord X1, Coord Y1, Coord X2, Coord Y2)
{
	Coord x, y;
	int tx, ty, width, height;

	x = MIN(X1,X2);
	y = MAX(Y1,Y2);
	tx = XS(x,y);
	ty = YS(x,y);
	width = XR(ABS(X2-X1));
	height = YR(ABS(Y2-Y1));
	if(cliprect(&tx, &ty, &width, &height))
	{
		XDrawRectangle(DWLGC, tx, ty, (UNINT)width, (UNINT)height);
	}
}

void glFilledRectangle(Coord X1, Coord Y1, Coord X2, Coord Y2)
{
	Coord x, y;
	int tx, ty, width, height;

	x = MIN(X1,X2);
	y = MAX(Y1,Y2);
	tx = XS(x,y);
	ty = YS(x,y);
	width = 1+XR(ABS(X2-X1));
	height = 1+YR(ABS(Y2-Y1));
	if(cliprect(&tx, &ty, &width, &height))
	{
		if(MASK) create_mask(tx, ty, width, height);
		XFillRectangle(DWFGC, tx, ty, (UNINT)width, (UNINT)height);
		if(MASK) free_mask();
	}
}

void glSetConcave(LOGICAL bool)
{
	PMode = bool ? Complex : Convex;
}

void glPolyLine (int n, Coord (*p)[2])
{
	q_len = 0;
	if(n > 1)
	{
		int i;
		POINT pp;
		pp[X] = XS(p[0][X],p[0][Y]);
		pp[Y] = YS(p[0][X],p[0][Y]);
		for(i = 1; i < n; i++)
		{
			if(clipline(XS(p[i][X],p[i][Y]), YS(p[i][X],p[i][Y]), pp)) continue;
			if(q_len > 1) XDrawLines(DWLGC, queue, q_len, CoordModeOrigin);
			q_len = 0;
		}
		if(q_len > 1) XDrawLines(DWLGC, queue, q_len, CoordModeOrigin);
	}
}

void glPolygon (int n, Coord (*p)[2])
{
	clippoly(n, p);
	if(q_len > 2)
		XDrawLines(DWLGC, queue, q_len, CoordModeOrigin);
}

void glFilledPolygon (int n, Coord (*p)[2])
{
	clippoly(n, p);
	if(q_len > 2)
	{
		if(MASK) create_mask_from_queue();
		XFillPolygon(DWFGC, queue, q_len, PMode, CoordModeOrigin);
		if(MASK) free_mask();
	}
}

void glPolygonHole(int n, Coord (*p)[2])
{
	clippoly(n, p);
	partial_poly_add();
}


/* Vertex graphics */

static int    mv = 0;
static int    nv = 0;
static POINT *va = NULL;

static void v_add(float x, int y)
{
	if(mv <= nv) va = GETMEM(va, POINT, mv+=100);
	va[nv][X] = x;
	va[nv][Y] = y;
	nv++;
}

void glBgnPoint          (void) {WX->vmode = VertexPoint;    nv = 0;}
void glBgnLine           (void) {WX->vmode = VertexLine;     nv = 0;}
void glBgnPolygon        (void) {WX->vmode = VertexPoly;     nv = 0;}
void glBgnFilledPolygon  (void) {WX->vmode = VertexPolyFill; nv = 0;}
void glBgnPolygonHole    (void) {WX->vmode = VertexPolyHole; nv = 0;}

/* Add vertex to whatever mode we are in */
void glVert(POINT v)
{
	if(WX->vmode == VertexNone)
		pr_error("glVert", "not in vertex mode\n", NULL);
	else
		v_add(v[X], v[Y]);
}

void glEndPoint(void)
{
	q_len = 0;
	if(WX->vmode != VertexPoint)
	{
		pr_error("glEndPoint", "missing glBgnPoint\n", NULL );
	}
	else
	{
		int i;
		for(i = 0; i < nv; i++)
			q_add(XS(va[i][X],va[i][Y]), YS(va[i][X],va[i][Y]));
		if(q_len > 0)
			XDrawPoints (DWLGC, queue, q_len, CoordModeOrigin);
	}
	WX->vmode = VertexNone;
}

void glEndLine(void)
{
	q_len = 0;
	if(WX->vmode != VertexLine )
	{
		pr_error("glEndLine", "missing glBgnLine\n", NULL );
	}
	else if(nv > 1)
	{
		int i;
		POINT pp;
		pp[X] = XS(va[0][X],va[0][Y]);
		pp[Y] = YS(va[0][X],va[0][Y]);
		for(i = 1; i < nv; i++)
		{
			if(clipline(XS(va[i][X],va[i][Y]), YS(va[i][X],va[i][Y]), pp)) continue;
			if(q_len > 1) XDrawLines(DWLGC, queue, q_len, CoordModeOrigin);
			q_len = 0;
		}
		if(q_len > 1) XDrawLines(DWLGC, queue, q_len, CoordModeOrigin);
	}
	WX->vmode = VertexNone;
}

void glEndPolygon(void)
{
	if(WX->vmode != VertexPoly)
	{
		pr_error("glEndPolygon", "missing glBgnPolygon\n", NULL);
	}
	else
	{
		clippoly(nv, va);
		if(q_len > 2)
			XDrawLines(DWLGC, queue, q_len, CoordModeOrigin);
	}
	WX->vmode = VertexNone;
}

void glEndFilledPolygon(void)
{
	if(WX->vmode != VertexPolyFill )
	{
		pr_error("glEndFilledPolygon", "missing glBgnFilledPolygon\n", NULL);
	}
	else
	{
		clippoly(nv, va);
		if(q_len > 2)
		{
			if(MASK) create_mask_from_queue();
			XFillPolygon(DWFGC, queue, q_len, PMode, CoordModeOrigin);
			if(MASK) free_mask();
		}
	}
	WX->vmode = VertexNone;
}

void glEndlPolygonHole(void)
{
	if(WX->vmode != VertexPolyHole )
	{
		pr_error("glEndPolygonHole", "missing glBgnPolygonHole\n", NULL);
	}
	else
	{
		clippoly(nv, va);
		partial_poly_add();
	}
	WX->vmode = VertexNone;
}
