/*
 * Copyright(c) 1992 Bell Communications Research, Inc. (Bellcore)
 * Copyright(c) 1995-99 Andrew Lister
 * Copyright © 1999, 2000, 2001, 2002, 2003, 2004, 2005 by the Xbae Developers.
 *
 *                        All rights reserved
 * Permission to use, copy, modify and distribute this material for
 * any purpose and without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies, and that the name of Bellcore not be used in advertising
 * or publicity pertaining to this material without the specific,
 * prior written permission of an authorized representative of
 * Bellcore.
 *
 * BELLCORE MAKES NO REPRESENTATIONS AND EXTENDS NO WARRANTIES, EX-
 * PRESS OR IMPLIED, WITH RESPECT TO THE SOFTWARE, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR ANY PARTICULAR PURPOSE, AND THE WARRANTY AGAINST IN-
 * FRINGEMENT OF PATENTS OR OTHER INTELLECTUAL PROPERTY RIGHTS.  THE
 * SOFTWARE IS PROVIDED "AS IS", AND IN NO EVENT SHALL BELLCORE OR
 * ANY OF ITS AFFILIATES BE LIABLE FOR ANY DAMAGES, INCLUDING ANY
 * LOST PROFITS OR OTHER INCIDENTAL OR CONSEQUENTIAL DAMAGES RELAT-
 * ING TO THE SOFTWARE.
 *
 * MatrixWidget Author: Andrew Wason, Bellcore, aw@bae.bellcore.com
 *
 * $Id: Create.c,v 1.79 2005/07/31 18:35:31 tobiasoed Exp $
 */

/*
 * Create.c created by Andrew Lister (28 Jan, 1996)
 */

#include <stdio.h>
#include <stdlib.h>

#include <Xm/Xm.h>
#include <Xm/ScrollBar.h>

#include <Xbae/MatrixP.h>
#include <Xbae/Macros.h>
#include <Xbae/Utils.h>
#include <Xbae/Actions.h>
#include <Xbae/Create.h>

#include "XbaeDebug.h"

static Pixmap createInsensitivePixmap(XbaeMatrixWidget mw);

void xbaeCopyBackground(Widget widget, int offset, XrmValue * value)
{
	value->addr = (XtPointer) & (widget->core.background_pixel);
}

void xbaeCopyForeground(Widget widget, int offset, XrmValue * value)
{
	value->addr = (XtPointer) & (((XmManagerWidget) widget)->manager.foreground);
}

void xbaeCopyDoubleClick(Widget widget, int offset, XrmValue * value)
{
	static int interval;

	interval = XtGetMultiClickTime(XtDisplay(widget));
	value->addr = (XtPointer) & interval;
}

/**************************************************************************************************/

static void *copyUnchecked(void *src, int n, size_t size)
{
	void *copy = NULL;
	if(n)
	{
		copy = XtMalloc(n * size);
		memcpy(copy, src, n * size);
	}
	return copy;
}

void xbaeCopyColumnButtonLabels(XbaeMatrixWidget mw)
{
	xbaeObjectLock((Widget) mw);
	mw->matrix.column_button_labels = copyUnchecked(mw->matrix.column_button_labels,
													mw->matrix.columns,
													sizeof *mw->matrix.column_button_labels);
	xbaeObjectUnlock((Widget) mw);
}

void xbaeCopyRowButtonLabels(XbaeMatrixWidget mw)
{
	xbaeObjectLock((Widget) mw);
	mw->matrix.row_button_labels = copyUnchecked(mw->matrix.row_button_labels,
												 mw->matrix.rows, sizeof *mw->matrix.row_button_labels);
	xbaeObjectUnlock((Widget) mw);
}

void xbaeCopyRowLabelBackgrounds(XbaeMatrixWidget mw)
{
	xbaeObjectLock((Widget) mw);
	mw->matrix.row_label_backgrounds = copyUnchecked(mw->matrix.row_label_backgrounds,
													 mw->matrix.rows,
													 sizeof *mw->matrix.row_label_backgrounds);
	xbaeObjectUnlock((Widget) mw);
}

void xbaeCopyRowLabelForegrounds(XbaeMatrixWidget mw)
{
	xbaeObjectLock((Widget) mw);
	mw->matrix.row_label_foregrounds = copyUnchecked(mw->matrix.row_label_foregrounds,
													 mw->matrix.rows,
													 sizeof *mw->matrix.row_label_foregrounds);
	xbaeObjectUnlock((Widget) mw);
}

void xbaeCopyColumnLabelBackgrounds(XbaeMatrixWidget mw)
{
	xbaeObjectLock((Widget) mw);
	mw->matrix.column_label_backgrounds = copyUnchecked(mw->matrix.column_label_backgrounds,
														mw->matrix.columns,
														sizeof *mw->matrix.column_label_backgrounds);
	xbaeObjectUnlock((Widget) mw);
}

void xbaeCopyColumnLabelForegrounds(XbaeMatrixWidget mw)
{
	xbaeObjectLock((Widget) mw);
	mw->matrix.column_label_foregrounds = copyUnchecked(mw->matrix.column_label_foregrounds,
														mw->matrix.columns,
														sizeof *mw->matrix.column_label_foregrounds);
	xbaeObjectUnlock((Widget) mw);
}

void xbaeCopyShowColumnArrows(XbaeMatrixWidget mw)
{
	xbaeObjectLock((Widget) mw);
	mw->matrix.show_column_arrows = copyUnchecked(mw->matrix.show_column_arrows,
												  mw->matrix.columns, sizeof *mw->matrix.show_column_arrows);
	xbaeObjectUnlock((Widget) mw);
}

void xbaeCopyColumnFontBold(XbaeMatrixWidget mw)
{
	xbaeObjectLock((Widget) mw);
	mw->matrix.column_font_bold = copyUnchecked(mw->matrix.column_font_bold,
												mw->matrix.columns, sizeof *mw->matrix.column_font_bold);
	xbaeObjectUnlock((Widget) mw);
}

void xbaeCopyRowUserData(XbaeMatrixWidget mw)
{
	xbaeObjectLock((Widget) mw);
	mw->matrix.row_user_data = copyUnchecked(mw->matrix.row_user_data,
											 mw->matrix.rows, sizeof *mw->matrix.row_user_data);
	xbaeObjectUnlock((Widget) mw);
}

void xbaeCopyColumnUserData(XbaeMatrixWidget mw)
{
	xbaeObjectLock((Widget) mw);
	mw->matrix.column_user_data = copyUnchecked(mw->matrix.column_user_data,
												mw->matrix.columns, sizeof *mw->matrix.column_user_data);
	xbaeObjectUnlock((Widget) mw);
}

/**************************************************************************************************/

static unsigned char *copyAlignments(Widget w, unsigned char *alignments, int n)
{
	unsigned char *copy = NULL;
	int i;
	Boolean bad = False;

	if(n)
	{
		copy = (unsigned char *) XtMalloc(n * sizeof(unsigned char));

		for(i = 0; i < n; i++)
		{
			if(bad)
			{
				copy[i] = XmALIGNMENT_BEGINNING;
			}
			else if(alignments[i] == BAD_ALIGNMENT)
			{
				copy[i] = XmALIGNMENT_BEGINNING;
				bad = True;
				XtAppWarningMsg(XtWidgetToApplicationContext(w),
								"copyAlignments", "tooShort", "XbaeMatrix",
								"XbaeMatrix: Column or column label alignments array is too short", NULL, 0);
			}
			else
			{
				copy[i] = alignments[i];
			}
		}
	}

	return copy;
}

void xbaeCopyColumnAlignments(XbaeMatrixWidget mw)
{
	xbaeObjectLock((Widget) mw);
	mw->matrix.column_alignments =
		copyAlignments((Widget) mw, mw->matrix.column_alignments, mw->matrix.columns);
	xbaeObjectUnlock((Widget) mw);
}

void xbaeCopyColumnLabelAlignments(XbaeMatrixWidget mw)
{
	xbaeObjectLock((Widget) mw);
	mw->matrix.column_label_alignments =
		copyAlignments((Widget) mw, mw->matrix.column_label_alignments, mw->matrix.columns);
	xbaeObjectUnlock((Widget) mw);
}

/**************************************************************************************************/

static unsigned char *copyShadowTypes(Widget w, unsigned char *shadow_types, int n)
{
	unsigned char *copy = NULL;
	int i;
	Boolean bad = False;

	if(n)
	{
		copy = (unsigned char *) XtMalloc(n * sizeof(unsigned char));

		for(i = 0; i < n; i++)
		{
			if(bad)
			{
				copy[i] = 0;
			}
			else if(shadow_types[i] == BAD_SHADOW)
			{
				copy[i] = 0;
				bad = True;
				XtAppWarningMsg(XtWidgetToApplicationContext(w),
								"copyShadowTypes", "tooShort", "XbaeMatrix",
								"XbaeMatrix: Row or column shadowTypes array is too short", NULL, 0);
			}
			else
			{
				copy[i] = shadow_types[i];
			}
		}
	}

	return copy;
}

void xbaeCopyRowShadowTypes(XbaeMatrixWidget mw)
{
	xbaeObjectLock((Widget) mw);
	mw->matrix.row_shadow_types = copyShadowTypes((Widget) mw, mw->matrix.row_shadow_types, mw->matrix.rows);
	xbaeObjectUnlock((Widget) mw);
}

void xbaeCopyColumnShadowTypes(XbaeMatrixWidget mw)
{
	xbaeObjectLock((Widget) mw);
	mw->matrix.column_shadow_types =
		copyShadowTypes((Widget) mw, mw->matrix.column_shadow_types, mw->matrix.columns);
	xbaeObjectUnlock((Widget) mw);
}

/**************************************************************************************************/

static short *copySizes(Widget w, short *sizes, int n)
{
	short *copy = NULL;
	int i;
	Boolean bad = False;

	if(n)
	{
		copy = (short *) XtMalloc(n * sizeof(short));
		for(i = 0; i < n; i++)
		{
			if(bad)
			{
				copy[i] = 1;
			}
			else if(sizes[i] < 1)
			{
				copy[i] = 1;
				bad = True;
				XtAppWarningMsg(XtWidgetToApplicationContext(w),
								"copySizes", "tooShort", "XbaeMatrix",
								"XbaeMatrix: Row width or column height array is too short or contains illigal values",
								NULL, 0);
			}
			else
			{
				copy[i] = sizes[i];
			}
		}
	}

	return copy;
}

void xbaeCopyColumnWidths(XbaeMatrixWidget mw)
{
	xbaeObjectLock((Widget) mw);
	mw->matrix.column_widths = copySizes((Widget) mw, mw->matrix.column_widths, mw->matrix.columns);
	xbaeObjectUnlock((Widget) mw);
}

void xbaeCopyRowHeights(XbaeMatrixWidget mw)
{
	xbaeObjectLock((Widget) mw);
	mw->matrix.row_heights = copySizes((Widget) mw, mw->matrix.row_heights, mw->matrix.rows);
	xbaeObjectUnlock((Widget) mw);
}

/**************************************************************************************************/

void xbaeCopyColumnMaxLengths(XbaeMatrixWidget mw)
{
	int *copy = NULL;
	int i;
	Boolean bad = False;

	xbaeObjectLock((Widget) mw);

	if(mw->matrix.columns)
	{
		copy = (int *) XtMalloc(mw->matrix.columns * sizeof(int));

		for(i = 0; i < mw->matrix.columns; i++)
		{
			if(bad)
			{
				copy[i] = 0;
			}
			else if(mw->matrix.column_max_lengths[i] < 0)
			{
				copy[i] = 0;
				bad = True;
				XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw),
								"copyColumnMaxLengths", "tooShort", "XbaeMatrix",
								"XbaeMatrix: Column max lengths array is too short or contains illigal values",
								NULL, 0);
			}
			else
			{
				copy[i] = mw->matrix.column_max_lengths[i];
			}
		}
	}
	mw->matrix.column_max_lengths = copy;

	xbaeObjectUnlock((Widget) mw);
}

/**************************************************************************************************/

static String *copyLabels(Widget w, String * labels, int n)
{
	String *copy = NULL;
	int i;
	Boolean bad = False;

	if(n && labels)
	{
		copy = (String *) XtMalloc(n * sizeof(String));

		for(i = 0; i < n; i++)
		{
			if(bad)
			{
				copy[i] = NULL;
			}
			else if(labels[i] == &xbaeBadString)
			{
				copy[i] = NULL;
				bad = True;
				XtAppWarningMsg(XtWidgetToApplicationContext(w),
								"copyLabels", "tooShort", "XbaeMatrix",
								"XbaeMatrix: Row or column labels array is too short", NULL, 0);
			}
			else
			{
				copy[i] = (labels[i] == NULL) ? NULL : XtNewString(labels[i]);
			}
		}
	}
	return copy;
}

static XmString *copyXmlabels(Widget w, XmString * xmlabels, int n)
{
	XmString *xmcopy = NULL;
	int i;

	if(n && xmlabels)
	{
		xmcopy = (XmString *) XtMalloc(n * sizeof(XmString));
		for(i = 0; i < n; i++)
		{
			xmcopy[i] = (xmlabels[i] == NULL) ? NULL : XmStringCopy(xmlabels[i]);
		}
	}
	return xmcopy;
}

void xbaeCopyRowLabels(XbaeMatrixWidget mw)
{
	xbaeObjectLock((Widget) mw);
	mw->matrix.row_labels = copyLabels((Widget) mw, mw->matrix.row_labels, mw->matrix.rows);
	mw->matrix.xmrow_labels = copyXmlabels((Widget) mw, mw->matrix.xmrow_labels, mw->matrix.rows);
	xbaeObjectUnlock((Widget) mw);
}

void xbaeCopyColumnLabels(XbaeMatrixWidget mw)
{
	xbaeObjectLock((Widget) mw);
	mw->matrix.column_labels = copyLabels((Widget) mw, mw->matrix.column_labels, mw->matrix.columns);
	mw->matrix.xmcolumn_labels = copyXmlabels((Widget) mw, mw->matrix.xmcolumn_labels, mw->matrix.columns);
	xbaeObjectUnlock((Widget) mw);
}

/**************************************************************************************************/

void xbaeFreeLabels(String * labels, XmString * xmlabels, int n)
{
	int i;

	if(labels)
	{
		for(i = 0; i < n; i++)
		{
			if(labels[i])
			{
				XtFree((char *) labels[i]);
			}
		}
		XtFree((char *) labels);
	}

	if(xmlabels)
	{
		for(i = 0; i < n; i++)
		{
			if(xmlabels[i])
			{
				XmStringFree(xmlabels[i]);
			}
		}
		XtFree((char *) xmlabels);
	}
}

void xbaeFreeRowLabels(XbaeMatrixWidget mw)
{
	xbaeObjectLock((Widget) mw);
	xbaeFreeLabels(mw->matrix.row_labels, mw->matrix.xmrow_labels, mw->matrix.rows);
	mw->matrix.row_labels = NULL;
	mw->matrix.xmrow_labels = NULL;
	xbaeObjectUnlock((Widget) mw);
}

void xbaeFreeColumnLabels(XbaeMatrixWidget mw)
{
	xbaeObjectLock((Widget) mw);
	xbaeFreeLabels(mw->matrix.column_labels, mw->matrix.xmcolumn_labels, mw->matrix.columns);
	mw->matrix.column_labels = NULL;
	mw->matrix.xmcolumn_labels = NULL;
	xbaeObjectUnlock((Widget) mw);
}

/**************************************************************************************************/

/*
 * A cache for pixmaps which remembers the screen a pixmap belongs to
 * (see bug #591306) and that permits clearing by screen when that
 * screen gets closed.
 */
static struct pcache {
	Pixmap pixmap;
	Screen *scr;

}     *stipple_cache = NULL;
static int ncache = 0;			/* Allocated size of the array. */
								/* Empty places have NULL screen */

static Pixmap PixmapFromCache(Screen * scr)
{
	int i;

	for(i = 0; i < ncache; i++)
		if(scr == stipple_cache[i].scr)
			return stipple_cache[i].pixmap;
	return (Pixmap) 0;
}

static void AddPixmapToCache(Screen * scr, Pixmap p)
{
	int i, old;

	for(i = 0; i < ncache; i++)
		if(stipple_cache[i].scr == 0)
		{
			stipple_cache[i].scr = scr;
			stipple_cache[i].pixmap = p;
			return;
		}

	/* Allocate more */
	if(ncache)
	{
		old = ncache;
		ncache *= 2;
		stipple_cache = (struct pcache *) XtRealloc((char *) stipple_cache, ncache * sizeof(struct pcache));
		for(i = old; i < ncache; i++)
			stipple_cache[i].scr = NULL;
		stipple_cache[old].scr = scr;
		stipple_cache[old].pixmap = p;
	}
	else
	{
		ncache = 16;			/* Some silly initial value */
		stipple_cache = (struct pcache *) XtMalloc(ncache * sizeof(struct pcache));
		for(i = 0; i < ncache; i++)
			stipple_cache[i].scr = NULL;
		stipple_cache[0].scr = scr;
		stipple_cache[0].pixmap = p;
	}
}

/*
 * Remove the pixmaps with this screen from the cache
 */
static void RemovePixmapsFromScreen(Screen * scr)
{
	int i;
	for(i = 0; i < ncache; i++)
		if(stipple_cache[i].scr == scr)
		{
			XFreePixmap(DisplayOfScreen(stipple_cache[i].scr), stipple_cache[i].pixmap);
			stipple_cache[i].pixmap = (Pixmap) 0;
			stipple_cache[i].scr = NULL;
		}
}

/*
 * Create a pixmap to be used for drawing the matrix contents when
 * XmNsensitive is set to False
 */
static Pixmap createInsensitivePixmap(XbaeMatrixWidget mw)
{
	static char stippleBits[] = { 0x01, 0x02 };
	Display *dpy = XtDisplay(mw);
	Screen *scr = XtScreen(mw);
	Pixmap p;

	xbaeObjectLock((Widget) mw);

	p = PixmapFromCache(XtScreen((Widget) mw));
	if(p)
	{
		xbaeObjectUnlock((Widget) mw);
		return p;
	}

	p = XCreatePixmapFromBitmapData(dpy, RootWindowOfScreen(scr), stippleBits, 2, 2, 0, 1, 1);
	AddPixmapToCache(scr, p);
	xbaeObjectUnlock((Widget) mw);
	return p;
}

/*
 * Make sure to know when our display connection dies.
 */
static void DisplayDied(Widget w, XtPointer client, XtPointer call)
{
	XtDestroyHookDataRec *p = (XtDestroyHookDataRec *) call;

	if(p == NULL || p->type != XtHdestroy)
		return;

	if(XtIsSubclass(p->widget, xmPrimitiveWidgetClass))
		RemovePixmapsFromScreen(XtScreen(p->widget));
}

void xbaeRegisterDisplay(Widget w)
{
	Display *d = XtDisplay(w);
	XtAddCallback(XtHooksOfDisplay(d), XtNdestroyHook, DisplayDied, NULL);
}

/**************************************************************************************************/

void xbaeCreateDrawGC(XbaeMatrixWidget mw)
{
	XGCValues values;
	unsigned long mask = GCStipple | GCFillStyle;

	xbaeObjectLock((Widget) mw);

	/*
	 * GC for drawing cells. We create it instead of using a cached one,
	 * since the foreground may change frequently.
	 */
	values.stipple = createInsensitivePixmap(mw);
	values.fill_style = (XtIsSensitive((Widget) mw)) ? FillSolid : FillStippled;

	DEBUGOUT(_XbaeDebug(__FILE__, (Widget) mw,
						"xbaeCreateDrawGC(dpy %p win %p fg %d stip %p)\n",
						XtDisplay(mw), GC_PARENT_WINDOW(mw), values.foreground, values.stipple));

	/* Font id isn't used for fontsets or xft fonts */
	if(mw->matrix.draw_font.type == XmFONT_IS_FONT)
	{
		mask |= GCFont;
		values.font = mw->matrix.draw_font.id;
	}
	mw->matrix.draw_gc = XCreateGC(XtDisplay(mw), GC_PARENT_WINDOW(mw), mask, &values);

	xbaeObjectUnlock((Widget) mw);
}

void xbaeCreatePixmapGC(XbaeMatrixWidget mw)
{
	XGCValues values;
	unsigned long mask = GCStipple | GCFillStyle | GCGraphicsExposures;

	xbaeObjectLock((Widget) mw);

	values.stipple = createInsensitivePixmap(mw);
	values.fill_style = (XtIsSensitive((Widget) mw)) ? FillSolid : FillStippled;
	values.graphics_exposures = False;

	mw->matrix.pixmap_gc = XCreateGC(XtDisplay(mw), GC_PARENT_WINDOW(mw), mask, &values);

	xbaeObjectUnlock((Widget) mw);
}

void xbaeCreateLabelGC(XbaeMatrixWidget mw)
{
	XGCValues values;
	unsigned long mask = GCStipple | GCFillStyle;

	xbaeObjectLock((Widget) mw);

	/*
	 * GC for drawing labels
	 */
	values.stipple = createInsensitivePixmap(mw);
	values.fill_style = (XtIsSensitive((Widget) mw)) ? FillSolid : FillStippled;

	/* Font id isn't used for fontsets or xft fonts */
	if(mw->matrix.label_font.type == XmFONT_IS_FONT)
	{
		mask |= GCFont;
		values.font = mw->matrix.label_font.id;
	}
	mw->matrix.label_gc = XCreateGC(XtDisplay(mw), GC_PARENT_WINDOW(mw), mask, &values);

	xbaeObjectUnlock((Widget) mw);
}

void xbaeGetGridLineGC(XbaeMatrixWidget mw)
{
	XGCValues values;
	XtGCMask mask = GCForeground | GCBackground;

	xbaeObjectLock((Widget) mw);

	/*
	 * GC for drawing grid lines
	 */
	values.foreground = mw->matrix.grid_line_color;
	values.background = mw->manager.foreground;

	/* Release the GC before getting another one */
	if(mw->matrix.grid_line_gc)
		XtReleaseGC((Widget) mw, mw->matrix.grid_line_gc);

	mw->matrix.grid_line_gc = XtGetGC((Widget) mw, mask, &values);

	xbaeObjectUnlock((Widget) mw);
}

void xbaeGetResizeTopShadowGC(XbaeMatrixWidget mw)
{
	XGCValues values;
	XtGCMask mask = GCForeground | GCBackground | GCFunction;

	xbaeObjectLock((Widget) mw);

	/*
	 * GC for drawing the top shadow when resizing
	 */
	values.foreground = mw->manager.top_shadow_color;
	values.background = mw->manager.foreground;
	values.function = GXxor;

	if(mw->manager.top_shadow_pixmap != XmUNSPECIFIED_PIXMAP)
	{
		mask |= GCFillStyle | GCTile;
		values.fill_style = FillTiled;
		values.tile = mw->manager.top_shadow_pixmap;
	}

	/* Release the GC before getting another one */
	if(mw->matrix.resize_top_shadow_gc)
		XtReleaseGC((Widget) mw, mw->matrix.resize_top_shadow_gc);

	mw->matrix.resize_top_shadow_gc = XtGetGC((Widget) mw, mask, &values);

	xbaeObjectUnlock((Widget) mw);
}

void xbaeGetResizeBottomShadowGC(XbaeMatrixWidget mw)
{
	XGCValues values;
	XtGCMask mask = GCForeground | GCBackground | GCFunction;

	xbaeObjectLock((Widget) mw);

	/*
	 * GC for drawing the bottom shadow when resizing
	 */
	values.foreground = mw->manager.bottom_shadow_color;
	values.background = mw->manager.foreground;
	values.function = GXxor;

	if(mw->manager.bottom_shadow_pixmap != XmUNSPECIFIED_PIXMAP)
	{
		mask |= GCFillStyle | GCTile;
		values.fill_style = FillTiled;
		values.tile = mw->manager.bottom_shadow_pixmap;
	}

	/* Release the GC before getting another one */
	if(mw->matrix.resize_bottom_shadow_gc)
		XtReleaseGC((Widget) mw, mw->matrix.resize_bottom_shadow_gc);

	mw->matrix.resize_bottom_shadow_gc = XtGetGC((Widget) mw, mask, &values);

	xbaeObjectUnlock((Widget) mw);
}

/**************************************************************************************************/


/* Set the current draw font to that defined by qtag. In the case of Type1 fonts
 * only set the drawing gc font id if set_gc is True. This is used to avoid this
 * action when the font is not to be used for drawing as font information may be
 * required before the widget is realized and the gc exists.
 */
void xbaeSetDrawFont( XbaeMatrixWidget mw, XrmQuark qtag, Boolean set_gc )
{
	static XrmQuark default_qtag = NULLQUARK;

	if(default_qtag == NULLQUARK)
	{
		default_qtag = mw->matrix.cell_font.qtag;
		if(default_qtag == NULLQUARK)
			default_qtag = XrmUniqueQuark();
	}

	if(mw->matrix.current_draw_qtag == NULLQUARK
	   || (qtag == NULLQUARK && mw->matrix.current_draw_qtag != default_qtag)
	   || (qtag != NULLQUARK && mw->matrix.current_draw_qtag != qtag))
	{
		if(qtag == NULLQUARK)
		{
			mw->matrix.current_draw_qtag = default_qtag;
			memcpy((void *) &mw->matrix.draw_font, (void *) &mw->matrix.cell_font,
				   sizeof(XbaeMatrixFontInfo));
		}
		else
		{
			mw->matrix.current_draw_qtag = qtag;
			xbaeInitFont(mw, XrmQuarkToString(qtag), &mw->matrix.draw_font);
		}

		if(set_gc && mw->matrix.draw_font.type == XmFONT_IS_FONT)
		{
			XSetFont(XtDisplay(mw), mw->matrix.draw_gc, mw->matrix.draw_font.id);
		}
	}
}


static short xbaeGetFontStructWidth(XFontStruct * font_struct)
{
	short width;
	unsigned long fp;
	unsigned char char_idx;

	/*
	 *  From the XmText man page: If the em-space value is
	 *  available, it is used. If not, the width of the  numeral  "0"
	 *  is used. If this is not available, the maximum width is used.
	 */
	if(XGetFontProperty(font_struct, XA_QUAD_WIDTH, &fp) && fp != 0)
	{
		width = (short) fp;
	}
	else
	{
		if(font_struct->min_char_or_byte2 <= '0' &&
		   font_struct->max_char_or_byte2 >= '0' && font_struct->per_char)
		{
			char_idx = '0' - font_struct->min_char_or_byte2;
			width = font_struct->per_char[char_idx].width;
		}
		else
		{
			width = font_struct->max_bounds.width;
		}
	}

	/* last safety check */
	if(width <= 0)
	{
		width = 1;
	}

	return width;
}

static short xbaeGetFontSetWidth(XFontSet font_set)
{
	XFontStruct **font_struct_list;
	char **font_name_list;
	int n, i, max_width;

	n = XFontsOfFontSet(font_set, &font_struct_list, &font_name_list);

	if(n <= 0)
	{
		max_width = 1;
	}
	else
	{
		max_width = xbaeGetFontStructWidth(font_struct_list[0]);
		for(i = 1; i < n; i++)
		{
			int width = xbaeGetFontStructWidth(font_struct_list[i]);
			if(width > max_width)
			{
				max_width = width;
			}
		}
	}

	return max_width;
}


void xbaeInitFont(XbaeMatrixWidget mw, XmStringTag intag, XbaeMatrixFontInfo *font)
{
	Boolean font_error;
	String font_type;
	XmRendition rendition = NULL;
	XmStringTag tag;
	XtPointer fontp, xftp;
	Arg args[4];
	int i;

	/* Search for the requested tag but also the default tags if the requested not found */
	XmStringTag tags[] = { intag, _MOTIF_DEFAULT_LOCALE, XmFONTLIST_DEFAULT_TAG };

	/* Find the first tag that exists in the render_table */
	for(i = 0; i < 3 && rendition == NULL; i++)
	{
		tag = tags[i];
		rendition = XmRenderTableGetRendition(mw->matrix.render_table, tags[i]);
	}

	/* This condition should never happen, but ... */
	if(rendition == NULL)
	{
		char buf[100];
		snprintf(buf, 100, "XbaeMatrix: Couldn't find tag \'%s\', \'%s\' or \'%s\' in renderTable",
				tags[0], tags[1], tags[2]);
		XtAppErrorMsg(XtWidgetToApplicationContext((Widget) mw),
					  "xbaeInitFont", "badFont", "XbaeMatrix", buf, NULL, 0);
		return;
	}

	/* Get font information from the rendition */
	i = 0;
	XtSetArg(args[i], XmNfont, &fontp); i++;
	XtSetArg(args[i], XmNxftFont, &xftp); i++;
	XtSetArg(args[i], XmNfontType, &font->type); i++;
	XmRenditionRetrieve(rendition, args, i);

	/* Check for a font error. Should never happen, but ... */
	if(font->type == XmFONT_IS_XFT)
	{
		font_error = (xftp == NULL);
		font_type = "FreeType";
	}
	else
	{
		font_error = (fontp == NULL || fontp == (XtPointer) XmAS_IS);
		font_type = "XLFD";
	}

	if(font_error)
	{
		char buf[100];
		snprintf(buf, 100, "XbaeMatrix: The specified %s font tag \'%s\' has no font loaded",
				font_type, tag);
		XtAppErrorMsg(XtWidgetToApplicationContext((Widget) mw),
				  "xbaeInitFont", "badFont", "XbaeMatrix", buf, NULL, 0);
	}
	else if(font->type == XmFONT_IS_XFT)
	{
		int n, i;
		XftChar8 ascii[128];
		XftFont *xftFont = (XftFont *) xftp;

		/* Find the average character width of the ascii part of the font */
		for(n = i = 0; i < 127; i++)
			if(isprint(i)) ascii[n++] = (XftChar8) i;
		ascii[n++] = '\0';
		font->width = (short) (xbaeXftStringWidth(mw, xftFont, ascii, n) / n);
		font->height = xftFont->height;
		font->y = -xftFont->ascent;
		font->id = 0;
		font->fontp = xftp;
		font->qtag = XrmStringToQuark(tag);
	}
	else if(font->type == XmFONT_IS_FONTSET)
	{
		XFontSet font_set = (XFontSet) fontp;
		XFontSetExtents *extents = XExtentsOfFontSet(font_set);
		font->width = xbaeGetFontSetWidth(font_set);
		font->height = extents->max_logical_extent.height;
		font->y = extents->max_logical_extent.y;
		font->id = 0;
		font->fontp = fontp;
		font->qtag = XrmStringToQuark(tag);
	}
	else
	{
		XFontStruct *font_struct = (XFontStruct *) fontp;
		font->width = xbaeGetFontStructWidth(font_struct);
		font->height = font_struct->max_bounds.descent + font_struct->max_bounds.ascent;
		font->y = -font_struct->max_bounds.ascent;
		font->id = font_struct->fid;
		font->fontp = fontp;
		font->qtag = XrmStringToQuark(tag);
	}

	XmRenditionFree(rendition);
}


/* The draw_font is initialized to the cell_font */
void xbaeInitFonts(XbaeMatrixWidget mw)
{
	xbaeInitFont(mw, mw->matrix.cell_rendition_tag, &mw->matrix.cell_font);
	xbaeInitFont(mw, mw->matrix.label_rendition_tag, &mw->matrix.label_font);
	memcpy((void *) &mw->matrix.draw_font, (void *) &mw->matrix.cell_font, sizeof(XbaeMatrixFontInfo));
}

/**************************************************************************************************/

/*
 * ARCAD SYSTEMHAUS
 */
void xbaeFill_WithEmptyValues_PerCell(XbaeMatrixWidget mw, struct _XbaeMatrixPerCellRec *p)
{
	p->shadow_type = 0;			/* 0 means to use matrix.cell_shadow_type */
	p->highlighted = HighlightNone;
	p->selected = False;
	p->underlined = False;
	p->user_data = NULL;
	p->background = XmUNSPECIFIED_PIXEL;
	p->color = XmUNSPECIFIED_PIXEL;
	p->widget = NULL;
	p->pixmap = XmUNSPECIFIED_PIXMAP;
	p->mask = XmUNSPECIFIED_PIXMAP;
	p->CellValue = NULL;
	p->qtag = NULLQUARK;
}

void xbaeCreatePerCell(XbaeMatrixWidget mw)
{
	/*
	 * Create with empty values
	 */
	struct _XbaeMatrixPerCellRec **copy = NULL;
	int i, j;

	DEBUGOUT(_XbaeDebug(__FILE__, (Widget) mw, "xbaeCreatePerCell(%d, %d)\n",
						mw->matrix.rows, mw->matrix.columns));
	xbaeObjectLock((Widget) mw);

	/*
	 * Malloc an array of row pointers
	 */
	if(mw->matrix.rows && mw->matrix.columns)
	{
		copy = (struct _XbaeMatrixPerCellRec **) XtMalloc(mw->matrix.rows * sizeof *copy);
		for(i = 0; i < mw->matrix.rows; i++)
		{
			copy[i] = (struct _XbaeMatrixPerCellRec *) XtMalloc(mw->matrix.columns * sizeof *copy[i]);
			for(j = 0; j < mw->matrix.columns; j++)
				xbaeFill_WithEmptyValues_PerCell(mw, &copy[i][j]);
		}
	}
	mw->matrix.per_cell = copy;
	xbaeObjectUnlock((Widget) mw);
}

static void __FreePixmap(XbaeMatrixWidget mw, Pixmap * p)
{
	if(*p && (*p != XmUNSPECIFIED_PIXMAP))
	{
		XFreePixmap(XtDisplay(mw), *p);
		*p = XmUNSPECIFIED_PIXMAP;
	}
}

static void __FreeString(String * p)
{
	if(*p != NULL)
	{
		XtFree(*p);
		*p = NULL;
	}
}

void xbaeFreePerCellEntity(XbaeMatrixWidget mw, int row, int column)
{
	DEBUGOUT(_XbaeDebug(__FILE__, (Widget) mw, "xbaeFreePerCellEntity(%d,%d)\n", row, column));
	/*
	 * CellValue: must be freed, alloc in xbae!
	 */
	__FreeString(&mw->matrix.per_cell[row][column].CellValue);

	/*
	 * user_data: must not be freed, user-defined
	 */

	/*
	 * Pixmap: free it, alloc in xbae!
	 */
	__FreePixmap(mw, &mw->matrix.per_cell[row][column].pixmap);

	/*
	 * mask: free it, alloc in xbae!
	 */
	__FreePixmap(mw, &mw->matrix.per_cell[row][column].mask);

}

void xbaeFreePerCellRow(XbaeMatrixWidget mw, int row)
{
	int j = 0;
	if(mw->matrix.per_cell[row])
	{
		for(j = 0; j < mw->matrix.columns; j++)
			xbaeFreePerCellEntity(mw, row, j);
		XtFree((char *) mw->matrix.per_cell[row]);
		mw->matrix.per_cell[row] = NULL;
	}
}

void xbaeFreePerCell(XbaeMatrixWidget mw)
{
	int i = 0;

	/*
	 * Free each row of XtPointer pointers
	 */
	if(mw->matrix.per_cell)
	{

		xbaeObjectLock((Widget) mw);

		for(i = 0; i < mw->matrix.rows; i++)
			xbaeFreePerCellRow(mw, i);

		XtFree((char *) mw->matrix.per_cell);
		mw->matrix.per_cell = NULL;

		xbaeObjectUnlock((Widget) mw);
	}

}
