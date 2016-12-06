/*
 * Copyright(c) 1992 Bell Communications Research, Inc. (Bellcore)
 * Copyright(c) 1995-99 Andrew Lister
 * Copyright © 1999, 2000, 2001, 2002, 2003, 2004 by the LessTif Developers.
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
 * $Id: Shadow.c,v 1.29 2005/04/06 23:13:14 tobiasoed Exp $
 */

/*
 * Shadow.c created by Andrew Lister (30 October, 1995)
 */

#include <stdio.h>

#include <Xm/Xm.h>
#include <Xm/XmP.h>
#include <Xm/DrawP.h>
#include <Xbae/MatrixP.h>
#include <Xbae/Shadow.h>
#include <Xbae/Draw.h>
#include <Xbae/Utils.h>

#include "XbaeDebug.h"

static void DrawRowShadow(XbaeMatrixWidget, Window, int, int, int, int, int, int, GC, GC,
						  unsigned char shadow);
static void DrawColumnShadow(XbaeMatrixWidget, Window, int, int, int, int, int, int, GC, GC,
							 unsigned char shadow);

static void DrawRowHighlight(XbaeMatrixWidget, Window, GC, int, int, int, int, int, int);
static void DrawColumnHighlight(XbaeMatrixWidget, Window, GC, int, int, int, int, int, int);

void
xbaeDrawLabelShadow(XbaeMatrixWidget mw, Window win, int x, int y, int width, int height, Boolean pressed)
{
	if(mw->matrix.cell_shadow_thickness == 0)
	{
		return;
	}

	/*
	 * Surround the label with a shadow.
	 */

	DRAW_SHADOW(XtDisplay(mw), win, "win",
				mw->manager.top_shadow_GC, mw->manager.bottom_shadow_GC,
				mw->matrix.cell_shadow_thickness, x, y, width, height, pressed ? XmSHADOW_IN : XmSHADOW_OUT);
}

void
xbaeDrawCellShadow(XbaeMatrixWidget mw, Window win, int row, int column, int x, int y, int width, int height)
{
	GC top_shadow_gc = mw->manager.top_shadow_GC;
	GC bottom_shadow_gc = mw->manager.bottom_shadow_GC;
	GC grid_line_gc = mw->matrix.grid_line_gc;

	unsigned char shadow = mw->matrix.cell_shadow_type;
	unsigned char grid_type = mw->matrix.grid_type;

/* amai, that's too verbose ...*/
	DEBUGOUT(_XbaeDebug
			 (__FILE__, (Widget) mw, "xbaeDrawCellShadow [%d,%d] wid %d, ht %d\n", row, column,
			  width, height));

	if(mw->matrix.cell_shadow_thickness == 0)
	{
		return;
	}

	if(mw->matrix.per_cell && mw->matrix.per_cell[row][column].shadow_type != 0)
	{
		shadow = mw->matrix.per_cell[row][column].shadow_type;
		grid_type = XmGRID_CELL_SHADOW;
	}
	else if(IN_GRID_ROW_MODE(mw) && mw->matrix.row_shadow_types && mw->matrix.row_shadow_types[row] != 0)
	{
		shadow = mw->matrix.row_shadow_types[row];
		grid_type = XmGRID_ROW_SHADOW;
	}
	else if(IN_GRID_COLUMN_MODE(mw) && mw->matrix.column_shadow_types
			&& mw->matrix.column_shadow_types[column] != 0)
	{
		shadow = mw->matrix.column_shadow_types[column];
		grid_type = XmGRID_COLUMN_SHADOW;
	}

	/*
	 * Surround the cell with a shadow.
	 */
	switch (grid_type)
	{
		case XmGRID_NONE:
			break;
		case XmGRID_ROW_SHADOW:
			DrawRowShadow(mw, win, row, column, x, y, width, height, top_shadow_gc, bottom_shadow_gc, shadow);
			break;
		case XmGRID_ROW_LINE:
			DrawRowShadow(mw, win, row, column, x, y, width, height, grid_line_gc, grid_line_gc, shadow);
			break;
		case XmGRID_COLUMN_SHADOW:
			DrawColumnShadow(mw, win, row, column, x, y, width, height,
							 top_shadow_gc, bottom_shadow_gc, shadow);
			break;
		case XmGRID_COLUMN_LINE:
			DrawColumnShadow(mw, win, row, column, x, y, width, height, grid_line_gc, grid_line_gc, shadow);
			break;
		case XmGRID_CELL_LINE:
			DRAW_SHADOW(XtDisplay(mw), win, "win",
						grid_line_gc, grid_line_gc,
						mw->matrix.cell_shadow_thickness, x, y, width, height, shadow);
			break;
		case XmGRID_CELL_SHADOW:
			DRAW_SHADOW(XtDisplay(mw), win, "win",
						top_shadow_gc, bottom_shadow_gc,
						mw->matrix.cell_shadow_thickness, x, y, width, height, shadow);
			break;

			/* Deprecated types. To be removed in next version. */
		case XmGRID_LINE:
			DRAW_SHADOW(XtDisplay(mw), win, "win",
						grid_line_gc, grid_line_gc,
						mw->matrix.cell_shadow_thickness, x, y, width, height, shadow);
			break;
		case XmGRID_SHADOW_OUT:
			DRAW_SHADOW(XtDisplay(mw), win, "win",
						bottom_shadow_gc, top_shadow_gc,
						mw->matrix.cell_shadow_thickness, x, y, width, height, shadow);
			break;
		case XmGRID_SHADOW_IN:
			DRAW_SHADOW(XtDisplay(mw), win, "win",
						top_shadow_gc, bottom_shadow_gc,
						mw->matrix.cell_shadow_thickness, x, y, width, height, shadow);
			break;
	}
}

void
xbaeDrawCellHighlight(XbaeMatrixWidget mw, Window win, GC gc, int row, int column,
					  int x, int y, int width, int height, unsigned char hl)
{
	int thick = mw->matrix.cell_shadow_thickness;

	DEBUGOUT(_XbaeDebug
			 (__FILE__, (Widget) mw, "xbaeDrawCellHighlight [%d,%d], wid %d, ht %d\n", row,
			  column, width, height));

	if(IN_GRID_ROW_MODE(mw) && (hl & HighlightRow))
	{
		DrawRowHighlight(mw, win, gc, row, column, x, y, width, height);
	}
	else if(IN_GRID_COLUMN_MODE(mw) && (hl & HighlightColumn))
	{
		DrawColumnHighlight(mw, win, gc, row, column, x, y, width, height);
	}
	else if(hl)
	{
		DRAW_HIGHLIGHT(XtDisplay(mw), win, gc,
					   x + thick, y + thick, width - 2 * thick, height - 2 * thick,
					   mw->matrix.cell_highlight_thickness, LineSolid);
	}
}

static void
DrawRowHighlight(XbaeMatrixWidget mw, Window win, GC gc, int row, int column, int x, int y,
				 int width, int height)
{
	XRectangle rect;

	DEBUGOUT(_XbaeDebug
			 (__FILE__, (Widget) mw, "DrawRowHighlight [%d,%d] wid %d ht %d\n", row, column, width, height));

	rect.x = x;
	rect.y = y;
	rect.width = width;
	rect.height = height;

	XSetClipRectangles(XtDisplay(mw), gc, 0, 0, &rect, 1, Unsorted);

	/*
	 * The highlight is inside the cell shadow
	 */
	x += mw->matrix.cell_shadow_thickness;
	y += mw->matrix.cell_shadow_thickness;
	width -= 2 * mw->matrix.cell_shadow_thickness;
	height -= 2 * mw->matrix.cell_shadow_thickness;

	/*
	 * Make sure the left/right edge are outside the clipping recangle if this
	 * is not the first/last column
	 */
	if(column != 0)
	{
		x -= mw->matrix.cell_shadow_thickness + mw->matrix.cell_highlight_thickness;
		width += mw->matrix.cell_shadow_thickness + mw->matrix.cell_highlight_thickness;
	}
	if(column != mw->matrix.columns - 1)
	{
		width += mw->matrix.cell_shadow_thickness + mw->matrix.cell_highlight_thickness;
	}

	DRAW_HIGHLIGHT(XtDisplay(mw), win, gc, x, y, width, height,
				   mw->matrix.cell_highlight_thickness, LineSolid);

	XSetClipMask(XtDisplay(mw), gc, None);
}

static void
DrawColumnHighlight(XbaeMatrixWidget mw, Window win, GC gc, int row, int column, int x, int y,
					int width, int height)
{
	XRectangle rect;

	DEBUGOUT(_XbaeDebug
			 (__FILE__, (Widget) mw, "DrawRowHighlight [%d,%d] wid %d ht %d\n", row, column, width, height));

	rect.x = x;
	rect.y = y;
	rect.width = width;
	rect.height = height;

	XSetClipRectangles(XtDisplay(mw), gc, 0, 0, &rect, 1, Unsorted);

	/*
	 * The highlight is inside the cell shadow
	 */
	x += mw->matrix.cell_shadow_thickness;
	y += mw->matrix.cell_shadow_thickness;
	width -= 2 * mw->matrix.cell_shadow_thickness;
	height -= 2 * mw->matrix.cell_shadow_thickness;

	/*
	 * Make sure the top/bottom edge are outside the clipping recangle if this
	 * is not the first/last row
	 */
	if(row != 0)
	{
		y -= mw->matrix.cell_shadow_thickness + mw->matrix.cell_highlight_thickness;
		height += mw->matrix.cell_shadow_thickness + mw->matrix.cell_highlight_thickness;
	}
	if(row != mw->matrix.rows - 1)
	{
		height += mw->matrix.cell_shadow_thickness + mw->matrix.cell_highlight_thickness;
	}

	DRAW_HIGHLIGHT(XtDisplay(mw), win, gc, x, y, width, height,
				   mw->matrix.cell_highlight_thickness, LineSolid);

	XSetClipMask(XtDisplay(mw), gc, None);
}

static void
DrawRowShadow(XbaeMatrixWidget mw, Window win, int row, int column, int x, int y, int width,
			  int height, GC topGC, GC bottomGC, unsigned char shadow)
{
	XRectangle rect;

	/*
	 * Set up a clipping rectangle over the current cell. 
	 */
	rect.x = x;
	rect.y = y;
	rect.width = width;
	rect.height = height;

	XSetClipRectangles(XtDisplay(mw), topGC, 0, 0, &rect, 1, Unsorted);
	if(topGC != bottomGC)
		XSetClipRectangles(XtDisplay(mw), bottomGC, 0, 0, &rect, 1, Unsorted);

	/*
	 * Make sure the left/right edge are outside the clipping recangle if this
	 * is not the first/last column
	 */
	if(column != 0)
	{
		x -= mw->matrix.cell_shadow_thickness;
		width += mw->matrix.cell_shadow_thickness;
	}
	if(column != mw->matrix.columns - 1)
	{
		width += mw->matrix.cell_shadow_thickness;
	}

	DRAW_SHADOW(XtDisplay(mw), win, "win", topGC, bottomGC,
				mw->matrix.cell_shadow_thickness, x, y, width, height, shadow);

	XSetClipMask(XtDisplay(mw), topGC, None);
	if(topGC != bottomGC)
		XSetClipMask(XtDisplay(mw), bottomGC, None);
}

static void
DrawColumnShadow(XbaeMatrixWidget mw, Window win, int row, int column, int x, int y, int width,
				 int height, GC topGC, GC bottomGC, unsigned char shadow)
{
	XRectangle rect;

	/*
	 * Set up a clipping rectangle over the current cell. 
	 */
	rect.x = x;
	rect.y = y;
	rect.width = width;
	rect.height = height;

	XSetClipRectangles(XtDisplay(mw), topGC, 0, 0, &rect, 1, Unsorted);
	if(topGC != bottomGC)
		XSetClipRectangles(XtDisplay(mw), bottomGC, 0, 0, &rect, 1, Unsorted);

	/*
	 * Make sure the top/bottom edge are outside the clipping recangle if this
	 * is not the first/last row
	 */
	if(row != 0)
	{
		y -= mw->matrix.cell_shadow_thickness;
		height += mw->matrix.cell_shadow_thickness;
	}
	if(row != mw->matrix.rows - 1)
	{
		height += mw->matrix.cell_shadow_thickness;
	}

	DRAW_SHADOW(XtDisplay(mw), win, "win", topGC, bottomGC,
				mw->matrix.cell_shadow_thickness, x, y, width, height, shadow);

	XSetClipMask(XtDisplay(mw), topGC, None);
	if(topGC != bottomGC)
		XSetClipMask(XtDisplay(mw), bottomGC, None);
}
