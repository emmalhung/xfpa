/*==========================================================================*/
/**
 * \file	CreatePixmap.c
 *
 * \brief    Pixmap creation functions.
 *
 * \note	The pixmaps created by these functions are cached so that
 *   		any pixmap is not created more than once per display. The
 *   		cache is only added to and entries are not destroyed but
 *   		made available for reuse.
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
/*==========================================================================*/

#include <stdarg.h>
#include <X11/X.h>
#include "XuP.h"

typedef unsigned int UNINT;

/* Structure private to this file, but with reference as an opaque pointer
 * in the Fxu strcture. This is used to store pixmaps created in this file
 * that are not created by XmGetPixmap.
 */
typedef struct _xu_pmc {
	int      refcount;
	String   fname;
	Display  *display;
	int      width;
	int      height;
	int      depth;
	Pixmap	 pixmap;
	Pixmap   parent;
} PMC;


/* Forward function declarations
 */
static void    draw_arrow         (Display*,Pixmap,int,int,Pixel,Pixel,XuArrowAttributes*,Boolean);
static Boolean is_xpm_file        (String);
static Pixmap  xpm_file_to_pixmap (Widget, String, XpmAttributes*);
static void    dummy_error_handler(Display*, XEvent*);
static Pixmap  store_pixmap       (Display*, String, Pixmap, int, int, int);


/**
 * \brief Create a pixmap with the same depth as the given widget.
 *
 * \param[in] w			Reference widget
 * \param[in] colour	Name of colour to fill in the pixmap with.
 * \param[in] width		Pixmap width
 * \param[in] height	Pixmap height
 */
Pixmap XuCreateColoredPixmap( Widget w, String colour, int width, int height )
{
	int        depth;
	Pixmap     pixmap;
	Pixel      fg;
	XGCValues  values;
	GC          gc;

	if(IsNull(w) || width < 1 || height < 1) return XmUNSPECIFIED_PIXMAP;

	XtVaGetValues(CW(w), XmNdepth, &depth, NULL);
	pixmap = XCreatePixmap(XtDisplay(w), XRootWindowOfScreen(XtScreenOfObject(w)),
				(UNINT)width, (UNINT)height, (UNINT)depth);
	values.foreground = XuLoadColor(w, colour); 
	gc = XCreateGC(XtDisplay(w), pixmap, GCForeground, &values);
	XFillRectangle(XtDisplay(w), pixmap, gc, 0, 0, (UNINT)width, (UNINT)height);
	XFreeGC(XtDisplay(w), gc);

	return store_pixmap(XtDisplay(w), "<cpix>", pixmap, width, height, depth);
}


/** \brief Create an insensitive pixmap from the given pixmap.
 *
 * \param[in] w		A reference widget.
 * \param[in] pixmap	The pixmap to make the insensitive pixmap from.
 *
 * \return The insensitive version of the input pixmap.
 *
 * \note
 * This we do by stippling the pixmap with 50% background colour.
 */
Pixmap XuCreateInsensitivePixmap(Widget w, Pixmap pixmap)
{
	int            x, y, i;
	int            fifty[] = {0x55, 0xaa};
	size_t         bytes_per_row;
	UNINT          width, height, border, depth;
	unsigned char  *data;
	Pixmap         pix;
	GC             gc;
	XGCValues      values;
	Window         root;
	Display        *dpy;

	if (!w) return XmUNSPECIFIED_PIXMAP;
	if (!pixmap) return XmUNSPECIFIED_PIXMAP;
	if (pixmap == XmUNSPECIFIED_PIXMAP) return XmUNSPECIFIED_PIXMAP;

	dpy = XtDisplay(w);

	/* Get the background, width, height, border and depth of the pixmap
	 * and create and background fill an identically sized pixmap.
	 */
	if(!XGetGeometry(dpy, pixmap, &root, &x, &y, &width, &height, &border, &depth))
		return XmUNSPECIFIED_PIXMAP;

	pix = XCreatePixmap(dpy, root, width, height, depth); 

	XtVaGetValues(w, XtNbackground, &values.foreground, NULL);
	gc = XCreateGC(dpy, pix, GCForeground, &values);
	XFillRectangle(dpy, pix, gc, 0, 0, width, height);

	/* Create the stipple masking pixmap and put into GC
	 */
	bytes_per_row = (width+7)/8;
	data = XTCALLOC(bytes_per_row * height, unsigned char);
	for(i = 0;i < height; i++)
	{
		(void) memset(data+(i*bytes_per_row), fifty[i%2], bytes_per_row);
	}
	values.clip_mask = XCreateBitmapFromData(dpy, root, (char*)data, width, height);
	XtFree((void *)data);
	XChangeGC(dpy, gc, GCClipMask, &values);
	XCopyArea(dpy, pixmap, pix, gc, 0, 0, width, height, 0, 0);

	XFreeGC(dpy, gc);
	XFreePixmap(dpy, values.clip_mask);

	return store_pixmap(dpy, "<inspix>", pix, (int)width, (int)height, (int)depth);
}


/**
 * \brief Create a pixmap containing an arrow
 *
 * \param[in] w			A reference widget
 * \param[in] attrib	The arrow attributes
 *
 * \return The created pixmap
 *
 * <b>Arrow Attributes:</b>
 *
 * The arrow attributes are set through the use the following XuArrowAttributes
 * structure.
 *
 * \verbatim
 long       flags
 long       appearance
 XuARROWDIR direction
 int        height
 int        width
 int        margin_height
 int        margin_width
 int        outline_width
 Pixel      foreground
 Pixel      background
 \endverbatim
 *
 * Which elements in the structure are used depends on the values set in the \c flags variable.
 * This consists of a set of enumerated values which can be or'ed together to allow more then
 * one element to be set. The values are:
 *
 * \arg \c XuARROW_APPEARANCE
 * \arg \c XuARROW_DIRECTION
 * \arg \c XuARROW_HEIGHT
 * \arg \c XuARROW_WIDTH
 * \arg \c XuARROW_MARGIN_HEIGHT
 * \arg \c XuARROW_MARGIN_WIDTH
 * \arg \c XuARROW_OUTLINE_WIDTH
 * \arg \c XuARROW_FOREGROUND
 * \arg \c XuARROW_BACKGROUND
 * \arg \c XuARROW_TEXT_UNDER
 * \arg \c XuARROW_ALL
 *
 * The \c XuARROW_ALL is a convienience entry that indicates that all elements in the
 * structure contain data.
 *
 * The \c appearance must be some combination of the following values which can
 * be or'ed together to produce a variety of arrow styles.
 *
 * \arg \c XuARROW_PLAIN The arrow head is a plain unadorned style
 * \arg \c XuARROW_BARBED The arrow head has barbs.
 * \arg \c XuARROW_BAR The arrow head has a bar across the tip
 * \arg \c XuARROW_STEM The arrow head has a stem
 * \arg \c XuARROW_OUTLINED The arrow is outlined
 *
 * The \c direction must be one of the following:
 *
 * \arg \c XuARROW_UP
 * \arg \c XuARROW_DOWN
 * \arg \c XuARROW_RIGHT
 * \arg \c XuARROW_LEFT
 */
Pixmap XuCreateArrowPixmap(Widget w , XuArrowAttributes *attrib )
{
	int        depth;
	Dimension  width, height;
	Pixmap     pixmap;
	Pixel      fg, bg;
	Display    *dpy;
	Window     win;

	dpy = XtDisplayOfObject(w);
	win = XRootWindowOfScreen(XtScreenOfObject(w));

	XtVaGetValues(w,
		XmNwidth,      &width,
		XmNheight,     &height,
		XmNdepth,      &depth,
		XmNforeground, &fg,
		XmNbackground, &bg,
		NULL);

	if(attrib->flags & XuARROW_WIDTH ) width  = (Dimension) attrib->width;
	if(attrib->flags & XuARROW_HEIGHT) height = (Dimension) attrib->height;

	pixmap = XCreatePixmap( dpy, win, (UNINT)width, (UNINT)height, (UNINT)depth); 
	draw_arrow(dpy, pixmap, (int)width, (int)height, fg, bg, attrib, True);

	return store_pixmap(dpy, "<arrowpix>", pixmap, (int)width, (int)height, depth);
}


/* Read the given image file and convert it to a pixmap. The file does not need to
 * have the complete path name specified if it is in one of the standard pixmap
 * locations used by the Aurora program.
 */
Pixmap XuGetPixmap(Widget w, String fname)
{
	int    i,j;
	String path;

	static Boolean first = True;

	const String module = "XuGetPixmap";
	/* The possible directories where the pixmap file are stored under the other possible directories */
	const String dn[] = {"Pixmaps","pixmaps","Pixmap","pixmap","Bitmaps","bitmaps","Bitmap","bitmap",NULL};
	/* The file name extensions */
	const String px[] = {".png",".jpg",".jpeg",".xpm",".xbm",".bm",NULL};

	if(!w || !XtIsWidget(w))
	{
		(void) fprintf(stderr,"%s: Invalid widget provided to function.\n", module);
		return XmUNSPECIFIED_PIXMAP;
	}
	if(blank(fname))
	{
		(void) fprintf(stderr,"%s: Blank filename provided to function.\n", module);
		return XmUNSPECIFIED_PIXMAP;
	}

	for( i = 0; i < XtNumber(px); i++ )
	{
		for( j = 0; j < XtNumber(dn); j++ )
		{
			if((path = XuFindTypeFile(dn[j], fname, px[i])))
			{
				Pixel fg, bg;
				Pixmap pixmap;
				XtVaGetValues(w, XmNforeground, &fg, XmNbackground, &bg, NULL);
				pixmap = XmGetPixmap(XtScreenOfObject(w), path, fg, bg);
				if (pixmap == XmUNSPECIFIED_PIXMAP && is_xpm_file(path) )
				{
					/* Try old xpm format file */
					pixmap = xpm_file_to_pixmap(w, path, NULL);
					if (pixmap == XmUNSPECIFIED_PIXMAP)
					{
						(void) fprintf(stderr,
								"%s: Unrecognized image file type: \"%s\"\n",
								module, path);
					}
					else if (first)
					{
						(void) fprintf(stderr,
								"%s: Reading image files with old xpm code!\n",
								module);
						(void) fprintf(stderr,
								"%s: First image file \"%s\"\n", module, path);
						first = False;
					}
				}
				else if (pixmap == XmUNSPECIFIED_PIXMAP)
				{
					(void) fprintf(stderr,
							"%s: Unrecognized image file type: \"%s\"\n",
							module, path);
				}
				XtFree((void *)path);
				return pixmap;
			}
		}
	}
	(void) fprintf(stderr,"%s: Unable to find image file: \"%s\"\n", module, fname);
	return XmUNSPECIFIED_PIXMAP;
}



/**
 * \brief Free the specified pixmap.
 *
 * param[in] wid	A reference widget
 * param[in] pix	The pixmap to free
 *
 * \note
 * The pixmap and its cache structure are not actually freed until
 * the reference count decreases to zero. If a pixmap is not found
 * the assumption is made that it was created by XmGetPixmap() and
 * will be freed by XmDestroyPixmap as these function have their own
 * cache procedure.
 */
void XuFreePixmap(Widget wid, Pixmap pix)
{
	int           n, id;
	UNINT         ud;
	Window        root;
	XErrorHandler xeh;
	Display       *dpy;

	if (!wid) return;
	if (!pix) return;
	if (pix == XmUNSPECIFIED_PIXMAP) return;

	dpy = XtDisplay(wid);
	for( n = 0; n < Fxu.pixmap_cache_len; n++ )
	{ 
		if(Fxu.pixmap_cache[n].display == NULL) continue;
		if(Fxu.pixmap_cache[n].display != dpy ) continue;
		if(Fxu.pixmap_cache[n].pixmap  != pix ) continue;

		Fxu.pixmap_cache[n].refcount--;
		if(Fxu.pixmap_cache[n].refcount > 0) return;

		XtFree((void*)Fxu.pixmap_cache[n].fname);
		(void) memset((void*)&Fxu.pixmap_cache[n], 0, sizeof(PMC));
		/*
		 * To avoid X errors and complaints if the pixmap no longer exists
		 * the error handler is set to a no action function and XGetGeometry
		 * is used to test for the existance of the pixmap before the free.
		 */
		xeh = XSetErrorHandler((XErrorHandler)dummy_error_handler);
		if(XGetGeometry(dpy, pix, &root, &id, &id, &ud, &ud, &ud, &ud))
			XFreePixmap(dpy, pix);
		(void) XSetErrorHandler(xeh);
		return;
	}
	/*
	 * Not found so assume that the Xm function handled it.
	 */
	(void) XmDestroyPixmap(XtScreenOfObject(wid), pix);
}



/**
 * \brief Replaces the text in the given widget with a pixmap containing an arrow.
 *
 * \param[in] w			The widget, which must be a subclass of label
 * \param[in] attrib	The arrow attribute structure
 *
 * \attention
 * The XmNuserData resouce is used to hold the initial state of the widget and
 * thus must not be used by any widget called with these functions.
 *
 * <b>Arrow Attributes:</b>
 * The arrow attributes are set through the use the following XuArrowAttributes
 * structure.
 *
 * \verbatim
 long       flags
 long       appearance
 XuARROWDIR direction
 int        height
 int        width
 int        margin_height
 int        margin_width
 int        outline_width
 Pixel      foreground
 Pixel      background
 Boolean    text_under
 \endverbatim
 *
 * Which elements in the structure are used depends on the values set in the \c flags variable
 * and not all of the elements in the structure are allowed to be set in this function.
 * This consists of a set of enumerated values which can be or'ed together to allow more then
 * one element to be set. The allowed values are:
 *
 * \arg \c XuARROW_APPEARANCE
 * \arg \c XuARROW_DIRECTION
 * \arg \c XuARROW_OUTLINE_WIDTH
 * \arg \c XuARROW_FOREGROUND
 * \arg \c XuARROW_BACKGROUND
 * \arg \c XuARROW_TEXT_UNDER
 *
 * The \c appearance must be some combination of the following values which can
 * be or'ed together to produce a variety of arrow styles.
 *
 * \arg \c XuARROW_PLAIN The arrow head is a plain unadorned style
 * \arg \c XuARROW_BARBED The arrow head has barbs.
 * \arg \c XuARROW_BAR The arrow head has a bar across the tip
 * \arg \c XuARROW_STEM The arrow head has a stem
 * \arg \c XuARROW_OUTLINED The arrow is outlined
 *
 * The \c direction must be one of the following:
 *
 * \arg \c XuARROW_UP
 * \arg \c XuARROW_DOWN
 * \arg \c XuARROW_RIGHT
 * \arg \c XuARROW_LEFT
 *
 * Note that there is not a \c flags entry for text_under as this is automatically looked for
 * by this function. This determines if any text in the button is inserted "under" the
 * arrow and would thus appear in the button along with the arrow. The default value is
 * true.
 */
void XuSetButtonArrow( Widget w, XuArrowAttributes *attrib )
{
	int                  depth;
	unsigned char        align, type;
	Boolean              text_under = True;
	XmString             xmlabel;
	XmFontList           fontlist;
	Pixel                fg, bg;
	Pixmap               pixmap;
	XtPointer            data;
	XuArrowAttributes    atts;
	Display              *dpy;

	dpy = XtDisplayOfObject(w);

	/* The size of the arrow is determined by the size of the text in the widget */
	XtVaGetValues(w,
		XmNforeground,  &fg,
		XmNbackground,  &bg,
		XmNlabelType,   &type,
		XmNlabelString, &xmlabel,
		XmNlabelPixmap, &pixmap,
		XmNalignment,   &align,
		XmNfontList,    &fontlist,
		XmNdepth,       &depth,
		XmNuserData,    &data,
		NULL);

	/* We use userData to hold the original state of the button. The default is
	 * NULL, so if there is a value we have already put the original state into
	 * the button and so do not want to change it. This can happen if this function
	 * is called more than once on a widget before the XuClearButtonArrow is called.
	 */
	if (!data)
	{
		XtVaSetValues(w,
			XmNuserData, (XtPointer)((type == XmPIXMAP)? pixmap : XmUNSPECIFIED_PIXMAP),
			NULL);
	}

	atts.flags         = XuARROW_ALL;
	atts.appearance    = XuARROW_PLAIN;
	atts.direction     = XuARROW_RIGHT;
	atts.width         = (int) XmStringWidth(fontlist, xmlabel);
	atts.height        = (int) XmStringHeight(fontlist, xmlabel);
	atts.margin_width  = 0;
	atts.margin_height = 0;
	atts.outline_width = 2;
	atts.foreground    = fg;
	atts.background    = bg;

	/* Just in case
	 */
	if(atts.width < 1 ) atts.width  = 10;
	if(atts.height < 1) atts.height = 10;

	if(attrib->flags & XuARROW_APPEARANCE   ) atts.appearance    = attrib->appearance;
	if(attrib->flags & XuARROW_DIRECTION    ) atts.direction     = attrib->direction;
	if(attrib->flags & XuARROW_FOREGROUND   ) atts.foreground    = attrib->foreground;
	if(attrib->flags & XuARROW_BACKGROUND   ) atts.background    = attrib->background;
	if(attrib->flags & XuARROW_OUTLINE_WIDTH) atts.outline_width = attrib->outline_width;
	if(attrib->flags & XuARROW_TEXT_UNDER   ) text_under         = attrib->text_under;

	pixmap = XCreatePixmap( dpy, XRootWindowOfScreen(XtScreen(w)),
		(UNINT)atts.width, (UNINT)atts.height, (UNINT)depth);

	if (pixmap)
	{
		if(text_under)
		{
			GC        gc;
			XGCValues values;

			values.foreground = bg;
			gc = XCreateGC(dpy, pixmap, GCForeground, &values);
			XFillRectangle(dpy, pixmap, gc, 0, 0, (UNINT)atts.width, (UNINT)atts.height);
			XFreeGC(dpy, gc);

			/* For some reason the XmStringDraw function in Motif 1.2 generates a
			 * XChangeCG error in the following code and I could not find the
			 * reason. It works just fine in Motif 2.x so I use a define switch.
			 */
#if (XmVERSION >= 2)
			values.foreground = fg;
			gc = XtAllocateGC(w, 0, GCForeground, &values, GCForeground, 0);
			XmStringDraw(dpy, pixmap, fontlist, xmlabel, gc, 0, 0, atts.width, align,
					XmSTRING_DIRECTION_L_TO_R, NULL);
			XtReleaseGC(w, gc);
#endif
		}

		draw_arrow(dpy, pixmap, atts.width, atts.height, fg, bg, &atts, !text_under);

		XtVaSetValues(w,
			XmNlabelType, XmPIXMAP,
			XmNlabelPixmap, pixmap,
			NULL);
	}
	XmStringFree(xmlabel);
	XuUpdateDisplay(w);
}


/**
 * \brief Removes a pixmap from a widget that was set through the use of XuSetButtonArrow 
 * and restores it to its original form.
 *
 * param[in] w The widget to restore
 *
 * \note
 * The widget userData resource is set back to NULL.
 */
void XuClearButtonArrow( Widget w )
{
	Pixmap        pixmap;
	unsigned char type;
	XtPointer     data;

	XtVaGetValues(w,
		XmNlabelType,   &type,
		XmNlabelPixmap, &pixmap,
		XmNuserData,    &data,
		NULL);

	if(NotNull(data) && data != (XtPointer)XmUNSPECIFIED_PIXMAP)
	{
		XtVaSetValues(w,
			XmNlabelType,   XmPIXMAP,
			XmNlabelPixmap, (Pixmap)data,
			XmNuserData,    NULL,
			NULL);
	}
	else
	{
		XtVaSetValues(w,
			XmNlabelType,   XmSTRING,
			XmNlabelPixmap, XmUNSPECIFIED_PIXMAP,
			XmNuserData,    NULL,
			NULL);
	}

	if(type == XmPIXMAP && pixmap != (Pixmap)NULL && pixmap != XmUNSPECIFIED_PIXMAP)
		XFreePixmap(XtDisplay(w), pixmap);

	XuUpdateDisplay(w);
}


/*================== Private functions =========================*/



static void draw_arrow(Display *dpy, Pixmap pixmap, int width, int height,
						Pixel fg, Pixel bg, XuArrowAttributes *attrib, const Boolean _set_bg )
{
	int        i, margin_width, margin_height, npoints, outline_width;
	int        h, w, bar_len, tm, lm, btm, blm, barb_len, offset;
	int        start, barb, interior, left, right, top, bottom, left_stem, right_stem, stem_width;
	long       appearance;
	Boolean    stemless, tip_bar, outlined, barbed;
	float      barb_interior_pct;
	XPoint     p[8], b[5];
	GC         gc;
	XGCValues  values;
	XuARROWDIR direction;

	if(!pixmap) return;

	/* These are our default values for arrow creation
	 */
	appearance        = XuARROW_PLAIN;
	direction         = XuARROW_UP;
	outline_width     = 2;
	margin_width      = 0;
	margin_height     = 0;
	barb_interior_pct = 1.;
	tip_bar           = False;
	stemless          = True;
	outlined          = False;
	offset            = 0;
	barbed            = False;

	/* Apply overrides if any are given
	 */
	if(attrib->flags & XuARROW_APPEARANCE)    appearance    = attrib->appearance;
	if(attrib->flags & XuARROW_DIRECTION)     direction     = attrib->direction;
	if(attrib->flags & XuARROW_HEIGHT)        height        = attrib->height;
	if(attrib->flags & XuARROW_WIDTH)         width         = attrib->width;
	if(attrib->flags & XuARROW_MARGIN_WIDTH)  margin_width  = attrib->margin_width;
	if(attrib->flags & XuARROW_MARGIN_HEIGHT) margin_height = attrib->margin_height;
	if(attrib->flags & XuARROW_OUTLINE_WIDTH) outline_width = attrib->outline_width;
	if(attrib->flags & XuARROW_BACKGROUND)    bg            = attrib->background;
	if(attrib->flags & XuARROW_FOREGROUND)    fg            = attrib->foreground;


	/* If the arrow is to be drawn in outlined form we need to move the points
	 * defining the arrow by half of the line width as the drawing function uses
	 * the center of the path for the line.
	 */
	if( appearance & XuARROW_OUTLINED )
	{
		outlined = True;
		offset   = outline_width/2;
	}
	if( appearance & XuARROW_BARBED )
	{
		barbed            = True;
		barb_interior_pct = 0.85;
	}
	if( appearance & XuARROW_BAR  ) tip_bar  = True;
	if( appearance & XuARROW_STEM ) stemless = False;


	/* Set the left and top margins for the arrow and for the bar (if there is one)
	 */
	lm = blm = (short)margin_width;
	tm = btm = (short)margin_height;

	/* From the arrows perspective the width and height do not include the margins.
	 */
	w  = (short)(width  - margin_width*2);
	h  = (short)(height - margin_height*2);

	/* Set all of the parameters common to the two basic directions along with the
	 * tip bar. Even if not used setting the bar does not hurt.
	 */
	switch(direction)
	{
		case XuARROW_UP:
		case XuARROW_DOWN:
			barb_len = w;
			bar_len  = tip_bar ? (barb_len*0.25 + 0.5) : 0;
			if(stemless)
			{
				tm += (h - barb_len - bar_len)/2;
				h  = (short)(height - tm*2);
			}
			h      = h - bar_len;
			top    = offset;
			bottom = h - 1 - offset;
			left   = offset;
			right  = w - 1 - offset;
			start  = right/2;

			b[0].x = b[3].x = b[4].x = (short) right;
			b[0].y = b[1].y = b[4].y = (short) (bar_len - 1 - offset);
			b[1].x = b[2].x          = (short) left;
			b[2].y = b[3].y          = (short) offset;
			break;

		case XuARROW_RIGHT:
		case XuARROW_LEFT:
			barb_len = h;
			bar_len  = tip_bar ? (barb_len*0.25 + 0.5) : 0;
			if(stemless)
			{
				lm += (w - barb_len - bar_len)/2;
				w  = (short)(width  - lm*2);
			}
			w      = w - bar_len;
			top    = offset;
			bottom = w - 1 - offset;
			left   = offset;
			right  = h - 1 - offset;
			start  = right/2;

			b[0].x = b[3].x = b[4].x = (short) (bar_len - offset);
			b[0].y = b[3].y = b[4].y = (short) right;
			b[1].x = b[2].x          = (short) offset;
			b[1].y = b[2].y          = (short) left;
			break;
	}


	/*
             2 o---------o 3        - bar is created separately if the bar
               |         |            thickness is > 0.
             1 o---------o 0

             7      oo       0       - 0 and 7 are the same for
                   /  \                odd numbered widths.
                  /    \
             5   / o  o \    2
             6  o /|  |\ o   1
             4     o--o      3
	*/

	/* The following code adjusts for an even or odd number of pixels. If odd then arrow
	 * head points 0 and 7 will be the same. If even they will be 1 pixel apart.
	 */
	switch(direction)
	{
		case XuARROW_UP:

			barb     = barb_len - 1 - offset;
			interior = (barbed)? (int)((float)w * barb_interior_pct  + 0.5) : barb;

			if(stemless)
			{
				btm = tm;
				tm += bar_len;

				p[0].x = (short) ((right%2 != 0) ? start+1:start);
				p[0].y = (short) top;
				p[1].x = (short) right;
				p[1].y = (short) barb;
				p[2].x = p[0].x;
				p[2].y = (short) interior;
				p[3].x = (short) start;
				p[3].y = (short) interior;
				p[4].x = (short) left;
				p[4].y = (short) barb;
				p[5].x = (short) start;
				p[5].y = (short) top;
			}
			else
			{
				stem_width = w*0.1 - 0.5;
				left_stem  = start - stem_width;
				right_stem = (right%2 == 0) ? start+stem_width: start+stem_width+1;;
				tm         += bar_len;

				p[0].x = (short) ((right%2 != 0) ? start+1:start);
				p[0].y = (short) top;
				p[1].x = (short) right;
				p[1].y = (short) barb;
				p[2].x = (short) right_stem;
				p[2].y = (short) interior;
				p[3].x = (short) right_stem;
				p[3].y = (short) bottom;
				p[4].x = (short) left_stem;
				p[4].y = (short) bottom;
				p[5].x = (short) left_stem;
				p[5].y = (short) interior;
				p[6].x = (short) left;
				p[6].y = (short) barb;
				p[7].x = (short) start;
				p[7].y = (short) top;
			}
			break;

		case XuARROW_DOWN:

			barb     = bottom - barb_len + 1 + 2*offset;
			interior = (barbed)? bottom - (int)((float)w * barb_interior_pct  + 0.5) : barb;

			if(stemless)
			{
				btm = tm + barb_len;

				p[0].x = (short) ((right%2 != 0) ? start+1:start);
				p[0].y = (short) bottom;
				p[1].x = (short) right;
				p[1].y = (short) barb;
				p[2].x = p[0].x;
				p[2].y = (short) interior;
				p[3].x = (short) start;
				p[3].y = (short) interior;
				p[4].x = (short) left;
				p[4].y = (short) barb;
				p[5].x = (short) start;
				p[5].y = (short) bottom;
			}
			else
			{
				stem_width = w*0.1 - 0.5;
				left_stem  = start - stem_width;
				right_stem = (right%2 == 0) ? start+stem_width: start+stem_width+1;
				btm = tm + h;

				p[0].x = (short)  ((right%2 != 0) ? start+1:start);
				p[0].y = (short)  bottom;
				p[1].x = (short)  right;
				p[1].y = (short)  barb;
				p[2].x = (short)  right_stem;
				p[2].y = (short)  interior;
				p[3].x = (short)  right_stem;
				p[3].y = (short)  top;
				p[4].x = (short)  left_stem;
				p[4].y = (short)  top;
				p[5].x = (short)  left_stem;
				p[5].y = (short)  interior;
				p[6].x = (short)  left;
				p[6].y = (short)  barb;
				p[7].x = (short)  start;
				p[7].y = (short)  bottom;
			}
			break;

		case XuARROW_LEFT:

			barb     = barb_len - 1 - offset;
			interior = (barbed)? (int)((float)h * barb_interior_pct  + 0.5) : barb;

			if(stemless)
			{
				blm = lm;
				lm += bar_len;

				p[0].x = (short)  top;
				p[0].y = (short)  ((right%2 != 0) ? start+1:start);
				p[1].x = (short)  barb;
				p[1].y = (short)  right;
				p[2].x = (short)  interior;
				p[2].y = p[0].y;
				p[3].x = (short)  interior;
				p[3].y = (short)  start;
				p[4].x = (short)  barb;
				p[4].y = (short)  left;
				p[5].x = (short)  top;
				p[5].y = (short)  start;
			}
			else
			{
				stem_width = h*0.1 - 0.5;
				left_stem  = start - stem_width;
				right_stem = (right%2 == 0) ? start+stem_width: start+stem_width+1;;
				lm         += bar_len;

				p[0].x = (short)  top;
				p[0].y = (short)  ((right%2 != 0) ? start+1:start);
				p[1].x = (short)  barb;
				p[1].y = (short)  right;
				p[2].x = (short)  interior;
				p[2].y = (short)  right_stem;
				p[3].x = (short)  bottom;
				p[3].y = (short)  right_stem;
				p[4].x = (short)  bottom;
				p[4].y = (short)  left_stem;
				p[5].x = (short)  interior;
				p[5].y = (short)  left_stem;
				p[6].x = (short)  barb;
				p[6].y = (short)  left;
				p[7].x = (short)  top;
				p[7].y = (short)  start;
			}
			break;

		case XuARROW_RIGHT:

			barb     = bottom - barb_len + 1 + 2*offset;
			interior = (barbed) ? bottom - (int)((float)h * barb_interior_pct  + 0.5) : barb;

			if(stemless)
			{
				blm = lm + barb_len;

				p[0].x = (short)  bottom;
				p[0].y = (short)  ((right%2 != 0) ? start+1:start);
				p[1].x = (short)  barb;
				p[1].y = (short)  right;
				p[2].x = (short)  interior;
				p[2].y = p[0].y;
				p[3].x = (short)  interior;
				p[3].y = (short)  start;
				p[4].x = (short)  barb;
				p[4].y = (short)  left;
				p[5].x = (short)  bottom;
				p[5].y = (short)  start;
			}
			else
			{
				stem_width = h*0.1 - 0.5;
				left_stem  = start - stem_width;
				right_stem = (right%2 == 0) ? start+stem_width: start+stem_width+1;;
				blm = lm + w;

				p[0].x = (short)  bottom;
				p[0].y = (short)  ((right%2 != 0) ? start+1:start);
				p[1].x = (short)  barb;
				p[1].y = (short)  right;
				p[2].x = (short)  interior;
				p[2].y = (short)  right_stem;
				p[3].x = (short)  top;
				p[3].y = (short)  right_stem;
				p[4].x = (short)  top;
				p[4].y = (short)  left_stem;
				p[5].x = (short)  interior;
				p[5].y = (short)  left_stem;
				p[6].x = (short)  barb;
				p[6].y = (short)  left;
				p[7].x = (short)  bottom;
				p[7].y = (short)  start;
			}
			break;
	}

	values.line_width = (outlined)? outline_width : 1;
	values.join_style = JoinMiter;

	if (_set_bg)
	{
		values.foreground = bg;
		gc = XCreateGC(dpy, pixmap, GCForeground|GCLineWidth|GCJoinStyle, &values);
		XFillRectangle(dpy, pixmap, gc, 0, 0, (UNINT)width, (UNINT)height);
		values.foreground = fg;
		XChangeGC(dpy, gc, GCForeground, &values);
	}
	else
	{
		values.foreground = fg;
		gc = XCreateGC(dpy, pixmap, GCForeground|GCLineWidth|GCJoinStyle, &values);
	}

	/* Add the margin offsets to the arrow positions and draw.
	*/
	npoints = (stemless)? 6:8;
	for(i = 0; i < npoints; i++)
	{
		p[i].x += lm;
		p[i].y += tm;
	}

	XDrawLines(dpy, pixmap, gc, p, npoints, CoordModeOrigin);
	if (!outlined) XFillPolygon(dpy, pixmap, gc, p, npoints, Nonconvex, CoordModeOrigin);

	if(tip_bar)
	{
		for(i = 0; i < 5; i++)
		{
			b[i].x += blm;
			b[i].y += btm;
		}
		XDrawLines(dpy, pixmap, gc, b, 5, CoordModeOrigin);
		if (!outlined) XFillPolygon(dpy, pixmap, gc, b, 5, Nonconvex, CoordModeOrigin);
	}
	XFreeGC(dpy, gc);
}



static Boolean is_xpm_file( String fname )
{
	char    line[100], *ptr;
	FILE    *fp;

	fp = fopen(fname, "r");
	if(IsNull(fp)) return False;

	/* The first valid line must contain "XPM" */
	while(NotNull((ptr = fgets(line, 100, fp))))
	{
		if(blank(ptr)) continue;
		if(strstr(ptr,"XPM")) break;
		if(strstr(ptr,"xpm")) break;
	}
	(void) fclose(fp);
	return (ptr != NULL);
}



/* Read the given xmp style file and create a pixmap. The Widget parameter is
 * required both to determine the display and the colormap if one is not supplied
 * in the xpmatts parameter which may be entered as NULL.
 *
 * Several attributes are automatically added if not specified in xpmatts.
 * These are closeness and colormap and a colour remapping that sets "none"
 * and "background" as a colour in the pixmap to the background colour of
 * the given widget.
 */
static Pixmap xpm_file_to_pixmap( Widget w, String _path , XpmAttributes *xpmatts)
{
	int           i, depth;
	Boolean       found_none, found_back, found_fore;
	Pixel         background, foreground;
	Colormap      colormap;
	Display       *dpy;
	Window        win;
	Pixmap        pixmap, mask;
	XpmAttributes atts;

	static String none   = "none";
	static String back   = "background";
	static String fore   = "foreground";

	dpy = XtDisplay(w);

	XtVaGetValues(CW(w),
		XmNdepth,      &depth,
		XmNbackground, &background,
		XmNforeground, &foreground,
		XmNcolormap,   &colormap,
		NULL);

	if(xpmatts) (void) memcpy((void *)&atts, (void *)xpmatts, sizeof(XpmAttributes));
	else         (void) memset((void *)&atts, 0,                sizeof(XpmAttributes));

	if(!(atts.valuemask & XpmCloseness))
	{
		atts.valuemask |= XpmCloseness;
		atts.closeness = 65535;
	}
	if(!(atts.valuemask & XpmColormap))
	{
		atts.valuemask |= XpmColormap;
		atts.colormap = colormap;
	}
	if(!(atts.valuemask & XpmDepth))
	{
		atts.valuemask |= XpmDepth;
		atts.depth = (unsigned int) depth;
	}

	/* Add the mapping of "none", "background" and "foreground".
	*/
	atts.colorsymbols = XTCALLOC(atts.numsymbols+3, XpmColorSymbol);
	if(atts.valuemask & XpmColorSymbols)
	{
		(void) memcpy((void*)atts.colorsymbols, (void*)xpmatts->colorsymbols,
					  	(size_t)atts.numsymbols*sizeof(XpmColorSymbol));
	}

	/* We don't want to set the following attributes if the attribute is passed in.
	*/
	atts.valuemask |= XpmColorSymbols;
	found_none = found_back = found_fore = False;
	
	for(i=0; i< (int) atts.numsymbols; i++)
	{
		if (same(atts.colorsymbols[i].value,none)) found_none = True;
		if (same(atts.colorsymbols[i].value,back)) found_back = True;
		if (same(atts.colorsymbols[i].value,fore)) found_fore = True;
	}
	if(!found_none)
	{
		atts.colorsymbols[atts.numsymbols].name  = NULL;
		atts.colorsymbols[atts.numsymbols].value = none;
		atts.colorsymbols[atts.numsymbols].pixel = background;
		atts.numsymbols++;
	}
	if(!found_back)
	{
		atts.colorsymbols[atts.numsymbols].name  = NULL;
		atts.colorsymbols[atts.numsymbols].value = back;
		atts.colorsymbols[atts.numsymbols].pixel = background;
		atts.numsymbols++;
	}
	if(!found_fore)
	{
		atts.colorsymbols[atts.numsymbols].name  = NULL;
		atts.colorsymbols[atts.numsymbols].value = fore;
		atts.colorsymbols[atts.numsymbols].pixel = foreground;
		atts.numsymbols++;
	}

	win = XRootWindowOfScreen(XtScreenOfObject(w));
	if(XpmReadFileToPixmap(dpy, win, _path, &pixmap, &mask, &atts) == XpmSuccess)
	{
		if(mask) XFreePixmap(dpy, mask);
	}
	else
	{
		pixmap = XmUNSPECIFIED_PIXMAP;
	}
	XtFree((void *)atts.colorsymbols);

	return pixmap;
}



/* Do nothing function used to ignore X errors */
/*ARGSUSED*/
static void dummy_error_handler( Display *dpy, XEvent *event ) {}


/* Try and find the pixmap in the existing cache. If it exists return the cache pixmap and
 * if not add it to the chache. Note that the pixmap can be complex so if all parameters
 * are the same we compare the actual pixmap contents.
 */
static Pixmap store_pixmap(Display *dpy, String name, Pixmap pixmap, int width, int height, int depth)
{
	int           n;
	XErrorHandler old_handler;
	XImage        *xi = (XImage*)NULL;

	if(!pixmap) return XmUNSPECIFIED_PIXMAP;
	if(pixmap == XmUNSPECIFIED_PIXMAP) return XmUNSPECIFIED_PIXMAP;

	/* Reset the error handler. If the pixmap does not actually exist XGetImage() will
	 * generate an error message. The default X message handler exits. We do not want
	 * this as XGetImage will return NULL on error anyway and we want to be able to 
	 * trap this and take appropriate action.
	 */
	old_handler = XSetErrorHandler((XErrorHandler)dummy_error_handler);

	/* See if this pixmap already is in the cache */
	for( n = 0; n < Fxu.pixmap_cache_len; n++ )
	{
		int     x, y;
		Boolean same_pixmap;
		XImage  *xi2;
		PMC     *cache = Fxu.pixmap_cache + n;

		/* Compare the major parameters */
		if(IsNull(cache->display)   ) continue;
		if(IsNull(cache->fname)     ) continue;
		if(strcmp(cache->fname,name)) continue;
		if(cache->display != dpy    ) continue;
		if(cache->depth   != depth  ) continue;
		if(cache->width   != width  ) continue;
		if(cache->height  != height ) continue;

		/* Major parameters are the same so compare the pixmap contents */
		if(IsNull(xi))
		{
			xi = XGetImage(dpy, pixmap, 0, 0, (unsigned int) width, (unsigned int) height, AllPlanes, ZPixmap);
			if (IsNull(xi))
			{
				(void) XSetErrorHandler(old_handler);
				return XmUNSPECIFIED_PIXMAP;
			}
		}

    	xi2 = XGetImage(dpy, cache->pixmap, 0, 0, (unsigned int) width, (unsigned int) height, AllPlanes, ZPixmap);
		if(IsNull(xi2))
		{
			/* Pixmap does not exist. Make cache available for reuse */
			XtFree(cache->fname);
			(void) memset((void*)cache, 0, sizeof(PMC));
			continue;
		}

		for( same_pixmap = True, y = 0; y < height && same_pixmap; y++ )
		{
			for( x = 0; x < width; x++ )
			{
				if( XGetPixel(xi,x,y) != XGetPixel(xi2,x,y) )
				{
					same_pixmap = False;
					break;
				}
			}
		}
		XDestroyImage(xi2);
		if(!same_pixmap) continue;

		/* Match found so increment the reference count and return */
		(void) XSetErrorHandler(old_handler);
		XDestroyImage(xi);
		XFreePixmap(dpy, pixmap);
		cache->refcount++;
		return cache->pixmap;
	}

	(void) XSetErrorHandler(old_handler);
	if(NotNull(xi)) XDestroyImage(xi);

	/* Use an empty available slot if possible. If none create an entry */
	for( n = 0; n < Fxu.pixmap_cache_len; n++ )
		if(!Fxu.pixmap_cache[n].display) break;

	if( n >= Fxu.pixmap_cache_len)
	{
		Fxu.pixmap_cache_len++;
		Fxu.pixmap_cache = (PMC*)XtRealloc((void*)Fxu.pixmap_cache, Fxu.pixmap_cache_len*sizeof(PMC));
	}

	/* Add the pixmap to the cache */
	Fxu.pixmap_cache[n].refcount = 1;
	Fxu.pixmap_cache[n].display  = dpy;
	Fxu.pixmap_cache[n].fname    = XtNewString(name);
	Fxu.pixmap_cache[n].width    = width;
	Fxu.pixmap_cache[n].height   = height;
	Fxu.pixmap_cache[n].depth    = depth;
	Fxu.pixmap_cache[n].pixmap   = pixmap;

	return pixmap;
}
