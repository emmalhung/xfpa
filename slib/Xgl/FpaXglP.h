/*********************************************************************************/
/*
*    FpaXglP.h
*
*    This is a private header file for the Xgl library which extends the private
*    header file of the glib library.
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
/*********************************************************************************/

#ifndef _FPAXGLP_H
#define _FPAXGLP_H

#include <X11/Xft/Xft.h>
#include <Xu.h>
#include <fpa_types.h>
#include <fpa_getmem.h>
#include "glib_private.h"
#include <fpa.h>
#include "FpaXgl.h"

#define BASE_DEFAULT_FONT	"XFT:DejaVu sans"

#define WX  W->x            /* active window x components */
#define D  	WX->display		/* active window x display */

#define XFREE(x)           {if(x) XFree((void*)(x)); x=NULL; }
#define FREE_COLORMAP(d,x) {if(x) XFreeColormap(d,x); x=(Colormap)NULL; }
#define FREE_PIXMAP(d,x)   {if(x) XFreePixmap(d,x); x=(Pixmap)NULL; }
#define FREE_XIMAGE(x)     {if(x) XDestroyImage(x); x=(XImage*)NULL; }
#define FREE_GC(d,x)       {if(x) XFreeGC(d,x); x=(GC)NULL; }


/* For double buffering define the types.
*/
#define NO_BUFFERING    0   /* No buffering */
#define PIX_BUFFERING   1   /* Pixmap used for double buffering */
#define BKG_BUFFERING	2   /* Used when window is a pixmap */

/* Vertex modes
*/
enum { VertexNone, VertexPoint, VertexLine, VertexPoly, VertexPolyFill, VertexPolyHole };

/* Needed by strcuture below and in image_output().
 */
typedef void (*OutputConvFunc)(XImage *image, int w, int h, UNCHAR *buf, int rowstride);

/* Structure used for X output
 */
typedef struct _xgl_x_output_info {
	unsigned long   red_shift;
	unsigned long   red_prec;
	unsigned long   blue_shift;
	unsigned long   blue_prec;
	unsigned long   green_shift;
	unsigned long   green_prec;
	unsigned int    bpp;
	OutputConvFunc  conv;
	XImage          *image;
} XOUTPUTINFO;


/* X specific information. This is referenced as a pointer in a structure in
 * glib_private.h but is only actually created in the Xgl library. This way
 * the glib library functions know nothing about X but the structure can still
 * reference things when required for X output.
 */
typedef struct _xgl_xstuff_struct {
    Window      front;              /* front window */
	Window      back;               /* backing store (swap window) */
	Window      draw;               /* what we actually draw into */
	XftDraw		*xftfront;			/* for xft font draw functions */
	XftDraw		*xftback;			/* for xft font draw functions */
	XftDraw		*xftdraw;			/* for xft font draw functions */
	Pixmap      mask;               /* masking pixmap for the draw window */
	UNCHAR      dbuf;				/* is double buffering in effect? */
	UNCHAR      vmode;				/* vertex mode */
	int      	screen;
	int         depth;
	Display     *display;
	XVisualInfo *visual_info;
	Colormap    cmap;
	XColor      *colors;            /* list of allocated colours for this window */
    int         ncolors;            /* length of allocated colour list */
	int         *colorXRef;         /* global color list to window color list x ref list */
    int         xop;				/* current gc operator function */
    GC          linegc;       		/* line drawing gc */
	GC          fillgc;             /* area fill gc */
    GC          miscgc;             /* unrestricted gc (no clipping) */
	GC          maskgc;             /* raster image mask gc */
	GC          dep1gc;             /* unrestricted depth 1 gc (no clipping) */
	XftColor    *xftfg				/* foreground for xft font draw functions */;
	XftColor    *xftbg				/* background for xft font draw functions */;
	struct {
		int    font_ndx;            /* index of currently active font */
		union {						
			float  vdc;				/* virtual device coordinates */
			float  map;				/* map coordinates */
			int	   pix;				/* original height units specified */
		} height;                   /* font height */
		int        height_units;    /* what height units is the original specification in? */
		float      angle;			/* text rotation angle */
		int        size;            /* font size in pixels */
		int        alignment;       /* the alignment of the string */
	} text;
	XOUTPUTINFO    *outinfo;        /* used by image_output.c functions only */
} XSTUFF_STRUCT;


/* Colour name to colour index reference structure. The xc variable holds the
*  actual rgb values that correspond to the colour independent of window and
*  its associated colormap.
*/
typedef struct _xgl_color_struct {
	char       *name;
	ColorIndex ndx;
	XColor     xc;
} XglColor;


/* Snapshot enum and structure
*/
enum { SNAP_NONE = 0, SNAP_PIXMAP, SNAP_IMAGE, SNAP_FILE };

typedef struct _xgl_snapshot_struct {
	Display *dpy;
	char    type;
	int     x, y;
	UNINT    w, h;
	union {
		Pixmap px;
		XImage *ix;
	} data;
	union {
		Pixmap px;
		XImage *ix;
	} mask;
	FILE *fp;
} SNAPSHOT;


/* Functions internal to the library only.
 */
extern Pixel      _xgl_pixel_from_XColor      (String, XColor);
extern Pixel      _xgl_pixel_from_name        (String, String);
extern Pixel      _xgl_pixel_from_color_index (String, ColorIndex);
extern ColorIndex _xgl_color_index_from_name  (String, String, int);
extern void       _xgl_free_fonts             (void);
extern void       _xgl_image_xlib_output      (struct _image_struct*);
extern void       _xgl_set_xft_color          (Pixel, XftColor*);

#endif /* _FPAXGLP_H */
