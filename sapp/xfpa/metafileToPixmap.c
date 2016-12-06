/*================================================================*/
/*
*     File: metafileToPixmap.c
*
*  Purpose: Contains functions to create pixmaps from metafile
*           patterns and symbols.
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
/*================================================================*/

#include "global.h"
#include <graphics.h>
#include "metafileToPixmap.h"


/* Create a pixmap which contains the image of a line pattern.
*  The image is assumed to go horizontally across the pixmap.
*/
Boolean MetafilePatternToPixmap(
	Widget w,               /* widget used for display reference */
    String pattern_name,    /* name of the metafile pattern to put into pixmap */
   	int width,              /* width of created pixmap in pixels */
    int height,             /* height of created pixmap in pixels */
    int margin_width,       /* width of left and right margin in pixels */
    int margin_height,      /* height of top and bottom margin in pixels */
    int pattern_height_pct, /* height of pattern as a percentage of available space */
    int repeat_count,       /* number of times to repeat the pattern */
   	String fgnd,            /* foreground - if NULL use Presentation fg */
    String bgnd,            /* background - if NULL use application background */
	Pixmap *label_px,		/* label pixmap */
	Pixmap *select_px,		/* select pixmap */
	Pixmap *insensitive_px	/* insensitive pixmap */
)
{
	int ix, iy, wid, depth;
	float delta, flength, fwidth, mfact;
	POINT points[2];
	Pixmap px, pix;
	Pixel fp, fg, bg;
	Display *dpy;
	Window win;
	GC gc;
	XImage *im;

	if(IsNull(pattern_name)) return False;

	dpy = XtDisplay(GW_mainWindow);
	win = XtWindow(GW_mainWindow);
	XtVaGetValues(GW_mainWindow, XmNdepth, &depth, NULL);

	/* Initialize for drawing into a pixel.
	*/
	glInit();
	glPushWindow();
	wid = glCreatePixmapFromXWindow(dpy, win, width, height);
	px  = glGetDrawable();
	glOrtho(0.0, 1.0, 0.0, 1.0);
	glViewport(margin_width, width - margin_width, margin_height, height - margin_height);

	/* Fill in the pixmap background.
	*/
	gc = XCreateGC(dpy, px, None, NULL);
	if(blank(bgnd)) XtVaGetValues(GW_mainWindow, XmNbackground, &bg, NULL);
	else            bg = XuLoadColorResource(w, bgnd, XmNbackground);
	XSetForeground(dpy, gc, bg);
	XFillRectangle(dpy, px, gc, 0, 0, (uint)width, (uint)height);

	/* Set the height adjustment delta factor. The width and length arguments of
	*  the pattern function are in parts-per-thousand, thus the 1000 factor.
	*/
	delta = (float)pattern_height_pct/100.0;
	mfact = 1000.0 / gxGetMfact();

	/* Baseline in middle of the pixmap
	*/
	points[0][X] = 0.0;
	points[1][X] = 1.0;
	points[0][Y] = 0.5;
	points[1][Y] = 0.5;

	fwidth = mfact * delta;

	/* Pattern repeat factor.
	*/
	flength = mfact / (float)repeat_count;

	if(!centre_pattern(points, 2, pattern_name, Right, fwidth, flength, 0))
	{
		XFreeGC(dpy, gc);
		glCloseWindow(wid);
		glPopWindow();
		return False;
	}

	/* If a foreground colour was given then we must convert any
	*  non-background colour in the pixmap to the given foreground.
	*/
	if(!blank(fgnd))
	{
		/* Assume a '*' or '.' in name is a resource, else a colour name */
		if(strchr(fgnd,'*') || strchr(fgnd,'.'))
			fg = XuLoadColorResource(w, fgnd, XtNforeground);
		else
			fg = XuLoadColor(w, fgnd);

		im = XGetImage(dpy, px, 0, 0, width, height, ~((unsigned long)0), ZPixmap);
		for(iy = margin_height; iy < height-margin_height; iy++)
		{
			for(ix = margin_width; ix < width-margin_width; ix++)
			{
				fp = XGetPixel(im, ix, iy);
				if(fp != bg) XPutPixel(im, ix, iy, fg);
			}
		}
		XPutImage(dpy, px, gc, im, 0, 0, 0, 0, width, height);
	}

	/* If requested supply an "insensitive" looking pixmap.
	*/
	if(insensitive_px)
	{
		int yoff;
		im = XGetImage(dpy, px, 0, 0, width, height, ~((unsigned long)0), ZPixmap);
		XtVaGetValues(GW_mainWindow, XmNbackground, &bg, NULL);
		for(iy = 0; iy < height; iy++)
		{
			yoff = iy%3;
			for(ix = 0; ix < width; ix++)
			{
				if((ix+yoff)%3 == 0) XPutPixel(im, ix, iy, bg);
			}
		}
		pix = XCreatePixmap(dpy, win, width, height, depth);
		XPutImage(dpy, pix, gc, im, 0, 0, 0, 0, width, height);
		*insensitive_px = pix;
	}

	if(select_px)
	{
	}

	if(label_px)
	{
		*label_px = px;
	}
	else
	{
		XFreePixmap(dpy, px);
	}

	XDestroyImage(im);
	XFreeGC(dpy, gc);
	glCloseWindow(wid);
	glPopWindow();
	return True;
}
