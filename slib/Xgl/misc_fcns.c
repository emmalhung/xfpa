/*************************************************************************/
/*
*   File:     misc_fcns.c
*
*   Purpose:  Contains functions required to initialize, reinitialize and
*             close the library and create windows.
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
/*************************************************************************/

#include "FpaXglP.h"

void glSetWindowAttributes(UNLONG mask, XSetWindowAttributes *attrib)
{
	if(mask != 0 && WX->dbuf != BKG_BUFFERING)
		XChangeWindowAttributes(WX->display, WX->front, mask, attrib);
}


void glGetWindowSize(int *x, int *y)
{ 
	if(NotNull(WX) && NotNull(WX->front) && WX->dbuf != BKG_BUFFERING)
	{
		XWindowAttributes wa;
		XSync(WX->display, FALSE);
		XGetWindowAttributes(WX->display, WX->front, &wa);
		*x = wa.width;
		*y = wa.height;
	}
	else
	{
		*x = (int) W->xm;
		*y = (int) W->ym;
	}
}


/* Single buffer mode does not destroy the buffers if we are in
*  double buffer mode only changes where we draw.
*/
void glSingleBuffer(void)
{
	WX->draw = WX->front;
	WX->xftdraw = WX->xftfront;
}

/*  We will use a pixmap to emulate double buffering and when true
*	double buffering becomes available we will check for that first
*	before defaulting to the pixmap style of use. If we are called
*   more than once free the existing pixmap and regenerate.
*/
void glDoubleBuffer(void)
{
	static char *MyName = "glDoubleBuffer";

	if(WX->dbuf == BKG_BUFFERING) return;
	if(WX->dbuf != NO_BUFFERING )
	{ 
		WX->draw = WX->back;
		WX->xftdraw = WX->xftback;
		return;
	}

	/* If we get to here either we do not have double-buffering or the call failed.
	*  Create a pixmap to draw into.
	*/
	WX->back = (Window)XCreatePixmap(D, WX->front, W->xm, W->ym, (UNINT)WX->depth);
	if(WX->back)
	{
		WX->draw = WX->back;
		WX->xftback = XftDrawCreate(WX->display, WX->back, WX->visual_info->visual, WX->cmap);
		WX->xftdraw = WX->xftback;
		WX->dbuf = PIX_BUFFERING;
	}
	else
	{
		WX->dbuf = NO_BUFFERING;
		WX->back = WX->front;
		WX->xftback = WX->xftfront;
		pr_error(MyName, "Unable to create backing pixmap.\n", NULL);
	}
	glFlush();
}

void glSwapBuffers(void)
{
	if(WX->draw != WX->front && WX->dbuf == PIX_BUFFERING)
	{
		XCopyArea(D, WX->back, WX->front, WX->miscgc, 0, 0, (UNINT)W->xm, (UNINT)W->ym, 0, 0);
	}
	glFlush();
}

void glFlush(void)
{
	XFlush(D);
}

Display *glGetDisplay(void)
{
	return D;
}

Drawable glGetDrawable(void)
{
	return WX->draw;
}

void glGetVdcPixelSize( float *dx, float *dy )
{
	int width, height;
	glGetWindowSize(&width, &height);
	*dx = 1.0/width;
	*dy = 1.0/height;
}


/* Reset the window scale attributes to their defaults.
*/
void glResetWindow(void)
{
    glResetViewport();
    glOrtho(0.0, (Coord)(W->xm - 1), 0.0, (Coord)(W->ym - 1));
}


void glResetViewport(void)
{
	int width, height;
	XglWindow *w = W;

	XSync(D, FALSE);
	glGetWindowSize(&width, &height);

	w->xm = (UNINT)width;
	w->ym = (UNINT)height;

	FREE_PIXMAP(w->x->display, w->x->mask);
	w->x->mask = XCreatePixmap(D, w->x->draw, w->xm, w->ym, 1);

	if(w->x->dbuf == PIX_BUFFERING)
	{
		FREE_PIXMAP(w->x->display, w->x->back);
		XftDrawDestroy(w->x->xftback);
		w->x->dbuf = NO_BUFFERING;
		glDoubleBuffer();
	}
	glViewport((Screencoord)0, (Screencoord)(w->xm - 1), (Screencoord)0, (Screencoord)(w->ym - 1));

	w->send = 0;		/* eliminate all transform matrix stack entries */
}


void glClipMode( int key )
{
	XRectangle rect;
	XglWindow *w = W;

	switch(key)
	{
		case glCLIP_ON:
			w->clipping = TRUE;
			rect.x = (short) w->cl;
			rect.y = (short) (w->ym - w->ct - 1);
			rect.width	= (UNSHORT)(w->cr - w->cl + 1);
			rect.height = (UNSHORT)(w->ct - w->cb + 1);
			XSetClipRectangles(D, w->x->linegc, 0, 0, &rect, 1, Unsorted);
			XSetClipRectangles(D, w->x->fillgc, 0, 0, &rect, 1, Unsorted);
			XSetClipRectangles(D, w->x->maskgc, 0, 0, &rect, 1, Unsorted);
			break;

		case glCLIP_OFF:
			w->clipping = FALSE;
			if(w->viewport)
			{
				rect.x = (short) w->vl;
				rect.y = (short) (w->ym - w->vt - 1);
				rect.width	= (UNSHORT)(w->vr - w->vl + 1);
				rect.height = (UNSHORT)(w->vt - w->vb + 1);
				XSetClipRectangles(D, w->x->linegc, 0, 0, &rect, 1, Unsorted);
				XSetClipRectangles(D, w->x->fillgc, 0, 0, &rect, 1, Unsorted);
				XSetClipRectangles(D, w->x->maskgc, 0, 0, &rect, 1, Unsorted);
			}
			else
			{
				XSetClipMask(D, w->x->linegc, None);
				XSetClipMask(D, w->x->fillgc, None);
				XSetClipMask(D, w->x->maskgc, None);
			}
			break;
	}
}


void glLineWidth(int rw)
{
	short		w;
	UNLONG		mask;
	XGCValues	values;

	w = (short) MAX(1,rw);
	if(w == (short) W->lw) return; 			/* already set */
	W->lw = values.line_width = w;
	mask = GCLineWidth;
	XChangeGC(D, WX->linegc, mask, &values);
}

void glVdcLineWidth(float w)
{
	glLineWidth((int)((W->ym-1)*w + 0.5));
}

void glMapLineWidth(float w)
{
	glLineWidth(YR(w));
}

int glGetLineWidth(void)
{
	return W->lw;
}

void glLineStyle(int s)
{
	int         ls, cs, js;
	UNLONG		mask = 0;
	XGCValues	values;


	ls = (s   ) & 7;
	cs = (s>>3) & 7;
	js = (s>>6) & 7;

	if(ls == W->ls && cs == W->cs && js == W->js) return; /* Already set */

	switch(ls)
	{
		case 1: W->ls = ls; values.line_style = LineSolid;      mask |= GCLineStyle; break;
		case 2: W->ls = ls; values.line_style = LineOnOffDash;  mask |= GCLineStyle; break;
		case 3: W->ls = ls; values.line_style = LineDoubleDash; mask |= GCLineStyle; break;
	}
	switch(cs)
	{
		case 1: W->cs = cs; values.cap_style = CapNotLast;    mask |= GCCapStyle; break;
		case 2: W->cs = cs; values.cap_style = CapButt;       mask |= GCCapStyle; break;
		case 3: W->cs = cs; values.cap_style = CapRound;      mask |= GCCapStyle; break;
		case 4: W->cs = cs; values.cap_style = CapProjecting; mask |= GCCapStyle; break;
	}
	switch(js)
	{
		case 1: W->js = js; values.join_style = JoinRound; mask |= GCJoinStyle; break;
		case 2: W->js = js; values.join_style = JoinMiter; mask |= GCJoinStyle; break;
		case 3: W->js = js; values.join_style = JoinBevel; mask |= GCJoinStyle; break;
	}
	if(mask != 0) XChangeGC(D, WX->linegc, mask, &values);
}

void glDashStyle(int nlist, int list[])
{
	int	n;
	char *dashes;
	static char * MyName = "glDashStyle";
	XglWindow *w = &Xgl.windows[Xgl.active_ndx];

	if(nlist < 2)
	{
		pr_error(MyName, "glDashStyle requires at least 2 values.\n", NULL);
		return;
	}
	dashes = MEM(char, nlist);
	for(n = 0; n < nlist; n++)
	{
		dashes[n] = (char)(MAX(1, list[n]));
	}
	XSetDashes(D, w->x->linegc, 0, dashes, nlist);
	FREEMEM(dashes);
}

void glVdcDashStyle(int nlist, float list[])
{
	int n, *ds;
	ds = MEM(int, nlist);
	for(n = 0; n < nlist; n++) ds[n] = (int)((W->ym-1)*list[n]+0.5);
	glDashStyle(nlist, ds);
	FREEMEM(ds);
}

void glMapDashStyle(int nlist, float list[])
{
	int n, *ds;
	ds = MEM(int, nlist);
	for(n = 0; n < nlist; n++) ds[n] = (int)YR(list[n]);
	glDashStyle(nlist, ds);
	FREEMEM(ds);
}


void glFillStyle(int style)
{
	if(style == W->fs) return;
	W->fs = style;
	if(style == glPATTERN_HATCH || style == glPATTERN_CROSS_HATCH)
	{
		XSetFillStyle(D, WX->fillgc, FillSolid);
	}
	else
	{
		XSetFillStyle(D, WX->fillgc, style);
	}
}


/* Create the stipple pixmap. The dx and dy parameters specify
*  the x length and y length of the pattern respectively. The
*  pattern starts at the upper left hand corner and continues
*  across then down the	pixmap.
*/
void glFillPattern(int dx, int dy, UNCHAR pattern[])
{
	int n, x, y;
	GC  gc;

	static Pixmap  stipple = (Pixmap)NULL;
	static Display *dpy    = (Display*)NULL;

	if (stipple) FREE_PIXMAP(dpy, stipple);
	dpy = D;
	stipple = XCreatePixmap(dpy, WX->draw, (UNINT)dx, (UNINT)dy, 1);
	gc = XCreateGC(dpy, stipple, 0, NULL);
	XSetForeground(dpy, gc, 0);
	XFillRectangle(dpy, stipple, gc, 0, 0, (UNINT)dx, (UNINT)dy);
	XSetForeground(dpy, gc, 1);
	for(y = 0, n = 0; y < dy; y++)
	{
		for(x = 0; x < dx; x++, n++)
		{
			if(pattern[n]) XDrawPoint(dpy, stipple, gc, x, y);
		}
	}
	FREE_GC(dpy, gc);
	XSetStipple(dpy, WX->fillgc, stipple);
}

/* Create the tile pixmap. The pixmap is square so that the
*  size parameter refers to width and size and the number of
*  pattern elements must be size*size. The pattern starts at the
*  upper left hand corner and continues across then down the
*  pixmap.
*/
void glTilePattern(int width, int height, char *pattern[])
{
	int	n, x, y;
	GC  save;

	static Pixmap  tile = (Pixmap)NULL;
	static Display *dpy = NULL;

	if (tile && dpy == D) FREE_PIXMAP(D, tile);
	tile = XCreatePixmap(D, WX->draw, (UNINT)width, (UNINT)height, (UNINT)WX->depth);
	dpy = D;

	save = WX->linegc;
	for(y = 0, n = 0; y < height; y++)
	{
		for(x = 0; x < width; x++, n++)
		{
			(void) glSetLineColor(pattern[n]);
			XDrawPoint(D, tile, WX->linegc, x, y);
		}
	}
	WX->linegc = save;
	XSetTile(D, WX->fillgc, tile);
}

void glLogicOp(int op)
{
	static char * MyName = "glLogicOp";

    switch(op) {
        case glLO_ZERO: WX->xop = GXclear;        break;
        case glLO_AND:  WX->xop = GXand;          break;
        case glLO_ANDR: WX->xop = GXandReverse;   break;
        case glLO_SRC:  WX->xop = GXcopy;         break;
        case glLO_ANDI: WX->xop = GXandInverted;  break;
        case glLO_DST:  WX->xop = GXnoop;         break;
        case glLO_XOR:  WX->xop = GXxor;          break;
        case glLO_OR:   WX->xop = GXor;           break;
        case glLO_NOR:  WX->xop = GXnor;          break;
        case glLO_XNOR: WX->xop = GXequiv;        break;
        case glLO_NDST: WX->xop = GXinvert;       break;
        case glLO_ORR:  WX->xop = GXorReverse;    break;
        case glLO_NSRC: WX->xop = GXcopyInverted; break;
        case glLO_ORI:  WX->xop = GXorInverted;   break;
        case glLO_NAND: WX->xop = GXnand;         break;
        case glLO_ONE:  WX->xop = GXset;          break;
        default:
            pr_error(MyName, "unknown argument: %d.\n", op);
			break;
	}
	XSetFunction(D, WX->linegc, WX->xop);
	XSetFunction(D, WX->fillgc, WX->xop);
}
