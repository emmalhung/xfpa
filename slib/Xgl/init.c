/*************************************************************************/
/*
*   File:     init.c
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

#include <limits.h>
#include "FpaXglP.h"

/* forward function declarations
 */
static XVisualInfo *set_visual_info       (XglWindow*, Visual*);
static void         set_viewport          (XglWindow *w, Screencoord d);
static void         close_xstuff          (XglWindow *);
static void         exit_xstuff           (int);
static void         exit_x                (void);
static void         create_active_window  (void);
static void         init_remaining_values (int buffering);


/* Graphic library intialization
 */
void glInit(void)
{
	const  String MyName  = "glInit";
	static int    endian_test[1] = {1};

	Display *dpy;

	if(Xgl.windows) return;

	if ((dpy = XOpenDisplay(NULL)) == NULL)
	{
		pr_error(MyName, "Unable to open display \"%s\".\n", XDisplayName(NULL));
		exit(1);
	}

	/* Do the machine endian test. If the upper byte contains the 1
	 * then we are operating on a little endian machine.
	 */
	if( ((char*) endian_test)[0] == 1 )
		MACHINE_ENDIAN = IMAGE_LITTLE_ENDIAN;
	else
		MACHINE_ENDIAN = IMAGE_BIG_ENDIAN;

	/* Set the maximum value of a short integer as float value.
	 * It is set to less than the actual max to account for roundoff.
	 */
	Xgl.short_limit = (float) SHRT_MAX * 0.95;

	Xgl.windows    = INITMEM(XglWindow, INITIAL_WINDOW_COUNT);
	Xgl.windows->x = ONEMEM(XSTUFF_STRUCT);

	Xgl.display    = dpy;
	Xgl.active     = Xgl.windows;
	Xgl.active_ndx = 0;
	Xgl.last_ndx   = INITIAL_WINDOW_COUNT;

	Xgl.windows->x->display = dpy;
	Xgl.windows->x->screen  = XDefaultScreen(dpy);
	Xgl.windows->x->depth   = XDefaultDepth(dpy, Xgl.windows->x->screen);
	Xgl.windows->x->cmap    = XDefaultColormap(dpy, Xgl.windows->x->screen);

	/* If we don't have a working directory set to the default.
	 */
	if(!Xgl.work_directory)
		glSetWorkingDirectory(DEFAULT_WORKING_DIRECTORY);

	/* Initialize the functions found in the Xgl data structure that are
	 * only used if X functionality is required.
	 */
	Xgl.image_xlib_output = _xgl_image_xlib_output;
	Xgl.set_viewport      = set_viewport;
	Xgl.close_xstuff      = close_xstuff;
	Xgl.exit_xstuff       = exit_xstuff;
	Xgl.x_exit            = exit_x;
}


/* This function is normally called after a fork(). In this case we want to
*  shutdown the library and re-initialize. We do not want to touch any of the
*  resources external to the process, such as those on the server, so this
*  procedure releases internal resources only and then initializes the library.
*  Note that the colour arrays are not touched since the original process
*  and this one will share the reserved colours and such. The same goes for
*  any images being displayed.
*/
void glResetDisplayConnection(void)
{
	int     i, n, ndpy = 0;
	Display **dpys = NULL;

	/* Add the main display to the display list by default.
	 * This will ensure it gets closed no matter what
	 */
	dpys = ONEMEM(Display*);
	dpys[ndpy++] = Xgl.display;

	/* Free all display related items. The first window is never used
	 * thus the start at 1.
	 */
	for(n = 1; n < Xgl.last_ndx; n++)
	{
		XglWindow *w = Xgl.windows+n;
		if(!w->inuse || IsNull(w->x)) continue;
		if(NotNull((w->x->display)))
		{
			for(i=0; i < ndpy; i++)
			{
				if(w->x->display == dpys[i]) break;
			}
			if(i >= ndpy)
			{
				dpys = GETMEM(dpys, Display*, ndpy+1);
				dpys[ndpy++] = w->x->display;
			}
		}

		FREEMEM(w->x->visual_info);
		if(w->x->front)
		{
			FREEMEM(w->x->colors);
			FREEMEM(w->x->colorXRef);
			FREEMEM(w->stack);
		}
	}

	/* close all of our display connections */
	for( n = 0; n < ndpy; n++ )
	{
		(void) close(ConnectionNumber(dpys[n]));
	}
	FREEMEM(dpys);
	FREEMEM(Xgl.windows);

	/* Force a regenerate on all images as any file cache will
	 * not be valid.
	 */
	for( n = 0; n < Xgl.nimages; n++ )
	{
		if(Xgl.images[i]) Xgl.images[i]->opstat = ImageGenerate;
	}


	/* Reset and free snapshot images
	*/
	for(n = 0; n < Xgl.nsnapshot; n++)
	{
		switch(Xgl.snapshot[n].type)
		{
			case SNAP_FILE:
				if(Xgl.snapshot[n].fp) (void) fclose(Xgl.snapshot[n].fp);

			case SNAP_IMAGE:
				FREE_XIMAGE(Xgl.snapshot[n].data.ix);
				FREE_XIMAGE(Xgl.snapshot[n].mask.ix);
				break;

		}
	}
	FREEMEM(Xgl.snapshot);
	Xgl.nsnapshot = 0;

	Xgl.display    = NULL;
	Xgl.windows    = NULL;
	Xgl.active     = NULL;
	Xgl.active_ndx = 0;
	Xgl.last_ndx   = 1;

	FREEMEM(Xgl.work_directory);
	FREEMEM(Xgl.work_directory_root);

	/* Re-initialize the library */
	glInit();
}


/* Initialize the graphics library window from an existing X window.
 */
int glInitWindow(Display *dpy, Window win)
{
	XWindowAttributes wa;
	
	glInit();
	create_active_window();
	
	XGetWindowAttributes(dpy, win, &wa);
	
	/* Setup window structure */
	W->xm	        = (UNINT) wa.width;
	W->ym	        = (UNINT) wa.height;
	WX->front       = win;
	WX->back        = win;
	WX->draw        = win;
	WX->display     = dpy;
	WX->screen      = XScreenNumberOfScreen(wa.screen);
	WX->depth       = wa.depth;	
	WX->visual_info = set_visual_info(W, wa.visual);
	WX->cmap        = wa.colormap;
	WX->xftdraw     = XftDrawCreate(dpy, win, WX->visual_info->visual, WX->cmap);
	WX->xftfront    = WX->xftdraw;
	WX->xftback     = WX->xftdraw;
	
	init_remaining_values(NO_BUFFERING);
	
	return Xgl.active_ndx;
}


/* Create a pixmap based window based on a reference X window. The pixmap
 * will have the same depth, viewable, etc. as the X window. If either width
 * or height are <= 0, then the window dimension will be used.
*/
int glCreatePixmapFromXWindow(Display *dpy, Window win, int width, int height )
{
	XWindowAttributes wa;
	Pixmap pixmap;
	
	glInit();
	create_active_window();
	
	XGetWindowAttributes(dpy, win, &wa);

	width  = (width  > 0)? width:wa.width;
	height = (height > 0)? height:wa.height;

	pixmap = XCreatePixmap(dpy, win, (UNINT)width, (UNINT)height, (UNINT)wa.depth);
	
	/* Setup window structure */
	WX->front       = pixmap;
	WX->back        = pixmap;
	WX->draw        = pixmap;
	WX->display     = dpy;
	WX->screen      = XScreenNumberOfScreen(wa.screen);
	WX->depth       = wa.depth;	
	WX->visual_info = set_visual_info(W, wa.visual);
	WX->cmap        = wa.colormap;
	W->xm	        = (UNINT) width;
	W->ym	        = (UNINT) height;
	WX->xftfront    = XftDrawCreate(WX->display, WX->front, WX->visual_info->visual, WX->cmap);
	WX->xftback     = WX->xftfront;
	WX->xftdraw     = WX->xftfront;
	
	init_remaining_values(NO_BUFFERING);
	
	return Xgl.active_ndx;
}



/* Create a pixmap based on an existing Xgl window.
*/
int glCreatePixmap( void )
{
	Pixmap pixmap;
	XglWindow *w;
	const String MyName  = "glCreateWindowPixmap";

	glInit();

	w = Xgl.active;

	if(IsNull(w) || IsNull(w->x))
	{
		pr_error(MyName,"No valid active window exists.\n", NULL);
		return 0;
	}

	pixmap = XCreatePixmap(w->x->display, w->x->draw, (UNINT)w->xm, (UNINT)w->ym, (UNINT)w->x->depth);

	if(!pixmap)
	{
		pr_error(MyName,"Pixmap creation failure.\n", NULL);
		return 0;
	}

	create_active_window();

	W->xm	        = w->xm;
	W->ym	        = w->ym;
	WX->front       = pixmap;
	WX->back        = pixmap;
	WX->draw        = pixmap;
	WX->display     = w->x->display;
	WX->screen      = w->x->screen;
	WX->depth       = w->x->depth;
	WX->visual_info = w->x->visual_info;
	WX->cmap        = w->x->cmap;
	WX->xftfront    = XftDrawCreate(WX->display, WX->front, WX->visual_info->visual, WX->cmap);
	WX->xftback     = WX->xftfront;
	WX->xftdraw     = WX->xftfront;
	
	init_remaining_values(NO_BUFFERING);
	
	return Xgl.active_ndx;
}


/* This is used when we want to create our graphic into a pixmap which will not
*  ever be used for drawing to a window but for file or snapshot output. If both
*  width and height are 0, then the width and height values will be taken from
*  the window active at the time this function is called. This function creates
*  a background window of the greatest possible depth.
*/
int glCreateBkgndWindow(int width, int height)
{
	int         nd, screen, depth;
	XVisualInfo visual_info;
	Display     *dpy;
	Window      win;
	GC          gc;
    XGCValues   values;

	static int   depths[]    = {         24,         16,           8 };
	static int   vis_type[]  = {  TrueColor,  TrueColor, PseudoColor };
	static char  *MyName = "glCreateBkgndWindow";

	glInit();

	/* Display set here as I may have to scan different X servers in the future.
	 * Sun uses multiple X servers instead of screens. Do only if client has
	 * problems.
	 */
	dpy = Xgl.display;

	/* Run through our depths and then through all of the screens to see if
	 * any one of the screens matches our required depth.
	 */
	for(depth = 0, nd = 0; nd < 3, depth == 0; nd++ )
	{
		for(screen = 0; screen < XScreenCount(dpy); screen++)
		{
			if(XMatchVisualInfo(dpy, screen, depths[nd], vis_type[nd], &visual_info))
			{
				depth = depths[nd];
				break;
			}
		}
	}
	if (!depth)
	{
		pr_error(MyName, "Unable to find a usable pixmap depth (24, 16 or 8 bits).\n", NULL);
		return 0;
	}

	/* need to do this before finding a new window.
	 */
	if( width  < 1 ) width  = (int) W->xm;
	if( height < 1 ) height = (int) W->ym;

	create_active_window();

	W->xm           = (UNINT) width;
	W->ym   	    = (UNINT) height;
	WX->display     = dpy;
	WX->screen      = screen;
	WX->depth       = depth;
	WX->visual_info = set_visual_info(W, visual_info.visual);

	win = XRootWindow(dpy,screen);

	/* Create our pixmap and an associated colormap.
	*/
	WX->front = XCreatePixmap(dpy, win, (UNINT)width, (UNINT)height, (UNINT)depth);
	if(!WX->front)
	{
		pr_error(MyName, "Unable to create background window.\n", NULL);
		return 0;
	}

	WX->cmap = XCreateColormap(dpy, win, WX->visual_info->visual, AllocNone);
	if(!WX->cmap)
	{
		XFreePixmap(dpy, WX->front);
		WX->front = 0;
		pr_error(MyName, "Unable to create colormap.\n", NULL);
		return 0;
	}

	WX->back = WX->front;
	WX->draw = WX->front;
	WX->xftfront = XftDrawCreate(WX->display, WX->front, WX->visual_info->visual, WX->cmap);
	WX->xftback  = WX->xftfront;
	WX->xftdraw  = WX->xftfront;
	
	init_remaining_values(BKG_BUFFERING);

	/* Initialize this window to black.
	*/
    values.foreground = 0;
	gc = XCreateGC(dpy, WX->front, GCForeground, &values);
	XFillRectangle(dpy, WX->front, gc, 0, 0, (UNINT)width, (UNINT)height);
	FREE_GC(dpy, gc);

	return Xgl.active_ndx;
}

/*================== Local Functions ========================*/


static void create_active_window(void)
{
	Xgl.active_ndx = 1;
	while(Xgl.active_ndx < Xgl.last_ndx && Xgl.windows[Xgl.active_ndx].inuse)
		Xgl.active_ndx++;

	if(Xgl.active_ndx == Xgl.last_ndx)
	{
		Xgl.last_ndx++;
		Xgl.windows = GETMEM(Xgl.windows, XglWindow,  Xgl.last_ndx);
	}
	W = &Xgl.windows[Xgl.active_ndx];
	(void) memset((void *)W, 0, sizeof(XglWindow));
	/* Allocate memory for a new X information structure */
	WX = ONEMEM(XSTUFF_STRUCT);
}


static XVisualInfo *set_visual_info (XglWindow *w, Visual *visual )
{
	XVisualInfo *visuals;
	XVisualInfo template;
	XVisualInfo *final_visual;
	int num_visuals;
	int i;

	template.screen = w->x->screen;
	visuals = XGetVisualInfo(w->x->display, VisualScreenMask, &template, &num_visuals);
 
	 for (i = 0; i < num_visuals; i++)
	 {
		if ((visuals+i)->visual == visual)
		{
			/* make a copy of the visual so that we can free the
	 		* allocated visual list above.
	 		*/
			final_visual = ONEMEM(XVisualInfo);
			(void) memcpy(final_visual, &visuals[i], sizeof(XVisualInfo));
			XFree((void *)visuals);
			return final_visual;
		}
	}
	return (XVisualInfo *)0;
}


static void init_remaining_values(int buffering)
{
	int       i;
    UNLONG    mask = GCGraphicsExposures | GCForeground | GCBackground | GCFillRule;
    XGCValues values;
	
	W->inuse    = TRUE;
	W->xp	    = 0;
	W->yp       = 0;
	W->viewport = FALSE;
	W->clipping = FALSE;
	W->hs       = 10;
	W->ha       = 45.0;
	W->hc       = 90.0;
	W->slen     = 0;
	W->send     = 0;
	W->stack    = (MATRIXSTACK*)0;

	WX->mask  = XCreatePixmap(WX->display, WX->draw, W->xm, W->ym, 1);
	WX->dbuf  = (UNCHAR) buffering;
	WX->vmode = VertexNone;

	WX->text.height.pix   = 10;
	WX->text.height_units = PIX_UNITS;
	WX->text.size         = 20;
	WX->text.angle        = 0;

    values.graphics_exposures = FALSE;
    values.foreground         = XBlackPixel(WX->display, WX->screen);
    values.background         = XWhitePixel(WX->display, WX->screen);
    values.fill_rule          = WindingRule;

	WX->linegc = XCreateGC(WX->display, WX->back, mask, &values);
	WX->fillgc = XCreateGC(WX->display, WX->back, mask, &values);
	WX->miscgc = XCreateGC(WX->display, WX->back, mask, &values);
	WX->xftfg = ONEMEM(XftColor);
	_xgl_set_xft_color(values.foreground, WX->xftfg);
	WX->xftbg = ONEMEM(XftColor);
	_xgl_set_xft_color(values.background, WX->xftbg);


    values.foreground = values.background = 0;
	WX->maskgc = XCreateGC(WX->display, WX->mask, mask, &values);
	WX->dep1gc = XCreateGC(WX->display, WX->mask, mask, &values);

	WX->ncolors = 0;
	WX->colors  = (XColor *)NULL;
	if(Xgl.ColorArrayLen > 0)
	{
		WX->colorXRef = MEM(int, Xgl.ColorArrayLen);
		for(i = 0; i < Xgl.ColorArrayLen; i++) WX->colorXRef[i] = UnallocatedColorIndex;
	}

	/* Set a default font just in case the user does not set one. There is no font
	 * named 'default' so this will load of whatever is defined as the default.
	 */
	glSetFont(glLoadFont(glDefaultFontName));
	
	/* Set default scale and viewport */
	glOrtho (0.0, (Coord)(W->xm - 1), 0.0, (Coord)(W->ym - 1));
	glViewport((Screencoord)0, (Screencoord)(W->xm - 1), (Screencoord)0, (Screencoord)(W->ym - 1));
}



/*================= X Specific Functions ======================*/

/*  These functions are specific to X functionality and are only
 *  allocated to the function pointers found in the glib library
 *  if glInit() is called.
 */

static void set_viewport(XglWindow *w, Screencoord top)
{
	XRectangle rect;

	XFlush(w->x->display);

	if(w->vl > 0 || w->vr < w->xm - 1 || w->vb > 0 || w->vt < w->ym - 1)
	{
		/* Clip window */
		w->viewport = TRUE;
		rect.x = (short)w->vl;
		rect.y = (short)(w->ym - top - 1);
		rect.width	= (UNSHORT)(w->vr - w->vl + 1);
		rect.height = (UNSHORT)(w->vt - w->vb + 1);
		XSetClipRectangles(w->x->display, w->x->linegc, 0, 0, &rect, 1, Unsorted);
		XSetClipRectangles(w->x->display, w->x->fillgc, 0, 0, &rect, 1, Unsorted);
		XSetClipRectangles(w->x->display, w->x->maskgc, 0, 0, &rect, 1, Unsorted);
	}
	else if(w->viewport)
	{
		w->viewport = FALSE;
		XSetClipMask(w->x->display, w->x->linegc, None);
		XSetClipMask(w->x->display, w->x->fillgc, None);
		XSetClipMask(w->x->display, w->x->maskgc, None);
	}
}


static void close_xstuff(XglWindow *w)
{
	if(IsNull(w))    return;
	if(IsNull(w->x)) return;

	FREEMEM(w->x->colors);
	FREEMEM(w->x->colorXRef);

	FREE_GC(w->x->display, w->x->linegc);
	FREE_GC(w->x->display, w->x->fillgc);
	FREE_GC(w->x->display, w->x->maskgc);
	FREE_GC(w->x->display, w->x->dep1gc);
	FREE_GC(w->x->display, w->x->miscgc);
	XftColorFree(w->x->display, w->x->visual_info->visual, w->x->cmap, w->x->xftfg);
	FREEMEM(w->x->xftfg);
	XftColorFree(w->x->display, w->x->visual_info->visual, w->x->cmap, w->x->xftbg);
	FREEMEM(w->x->xftbg);

	FREEMEM(w->x->visual_info);

	if(NotNull(w->x->outinfo) && NotNull(w->x->outinfo->image))
	{
		FREEMEM(w->x->outinfo->image->data);
		XDestroyImage(w->x->outinfo->image);
	}

	switch(w->x->dbuf)
	{
		case PIX_BUFFERING:
			FREE_PIXMAP(w->x->display, w->x->back);
			XftDrawDestroy(w->x->xftback);
			break;
		case BKG_BUFFERING:
			FREE_PIXMAP(w->x->display, w->x->front);
			XftDrawDestroy(w->x->xftfront);
			FREE_COLORMAP(w->x->display, w->x->cmap);
			break;
	}

	FREE_PIXMAP(w->x->display, w->x->mask);
	XFlush(w->x->display);
	FREEMEM(w->x);
}


static void exit_xstuff(int ndx)
{
	int m;
	Pixel *px;
	XglWindow *w;

	if(IsNull(Xgl.windows)) return;

	w = Xgl.windows + ndx;

	if(IsNull(w))                 return;
	if(IsNull(w->x))              return;
	if(w->x->cmap == (Colormap)0) return;
	if(w->x->ncolors <= 0)        return;

	px = MEM(Pixel, w->x->ncolors);
	if (px)
	{
		for(m = 0; m < w->x->ncolors; m++)
			px[m] = w->x->colors[m].pixel;
		XFreeColors(Xgl.display, w->x->cmap, px, w->x->ncolors, 0);
		FREEMEM(px);
	}
}

/* Make sure that we release all server resources. Note that the working
 * directory release will not happen if the directory is not empty.
 */
static void exit_x(void)
{
	int n;

	_xgl_free_fonts();
	glClearAllSnapshots();
	for(n = 0; n < Xgl.LastColor; n++)
		FREEMEM(Xgl.Colors[n].name);
	FREEMEM(Xgl.Colors);
	FREEMEM(Xgl.ColorXRef);
}
