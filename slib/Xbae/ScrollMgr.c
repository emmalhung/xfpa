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
 * $Id: ScrollMgr.c,v 1.78 2005/04/15 21:43:49 tobiasoed Exp $
 */

#include <stdio.h>
#include <assert.h>

#include <Xm/Xm.h>
#include <Xm/ScrollBar.h>

#include <Xbae/MatrixP.h>
#include <Xbae/Draw.h>
#include <Xbae/Shadow.h>
#include <Xbae/ScrollMgr.h>
#include <Xbae/Utils.h>
#include <Xbae/Clip.h>
#include <Xbae/Methods.h>		/* for relayout */

#include "XbaeDebug.h"

/*
 * Callback for vertical scrollbar
 * SGO: changed scrolling routine for vertical scrolling.
 * using same approach as in xbaeScrollHorizCB.
 */

void xbaeScrollVertCB(Widget w, XtPointer client_data, XmScrollBarCallbackStruct * call_data)
{
	XbaeMatrixWidget mw = (XbaeMatrixWidget) XtParent(w);
	int delta = VERT_ORIGIN(mw) - call_data->value;

	if(delta == 0)
	{
		/* Didn't scroll */
		return;
	}

	/*
	 * Adjust our vertical origin
	 */
	VERT_ORIGIN(mw) = call_data->value;

	/*
	 * The textField needs to scroll along with the cells.
	 */
	if(mw->matrix.text_child_is_mapped && !IS_FIXED_ROW(mw, mw->matrix.current_row))
	{
		xbaePositionTextChild(mw);
	}

	/* Cell widgets that are in the vertical scrolling region
	 * need to scroll along as well... Linas */
	if(mw->matrix.per_cell)
	{
		int roe, kol;

		for(kol = 0; kol < (mw->matrix.columns); kol++)
		{
			for(roe = mw->matrix.fixed_rows; roe < TRAILING_ROW_ORIGIN(mw); roe++)
			{
				xbaePositionCellWidget(mw, roe, kol);
			}
		}
		xbaeSetInitialFocus(mw);
	}

	if(!XtIsRealized((Widget) mw))
		return;

	/*
	 * Scroll the contents of the clips
	 */

	if(XtIsManaged(ClipChild(mw)))
		XbaeClipScrollVert(ClipChild(mw), mw->matrix.draw_gc, delta);

	if(XtIsManaged(LeftClip(mw)))
		XbaeClipScrollVert(LeftClip(mw), mw->matrix.draw_gc, delta);

	if(XtIsManaged(RightClip(mw)))
		XbaeClipScrollVert(RightClip(mw), mw->matrix.draw_gc, delta);

	if(XtIsManaged(RowLabelClip(mw)))
		XbaeClipScrollVert(RowLabelClip(mw), mw->matrix.draw_gc, delta);
}

/*
 * Callback for horizontal scrollbar
 */

/* ARGSUSED */
void xbaeScrollHorizCB(Widget w, XtPointer client_data, XmScrollBarCallbackStruct * call_data)
{
	XbaeMatrixWidget mw = (XbaeMatrixWidget) XtParent(w);
	int delta = HORIZ_ORIGIN(mw) - call_data->value;

	if(delta == 0)
	{
		/* Didn't scroll */
		return;
	}

	/*
	 * Adjust our horizontal origin
	 */
	HORIZ_ORIGIN(mw) = call_data->value;

	/*
	 * The textField needs to scroll along with the cells.
	 */
	if(mw->matrix.text_child_is_mapped && !IS_FIXED_COLUMN(mw, mw->matrix.current_column))
	{
		xbaePositionTextChild(mw);
	}

	/* Cell widgets that are in the horizontal scrolling region
	 * need to scroll along as well... Linas */
	if(mw->matrix.per_cell)
	{
		int roe, kol;

		for(roe = 0; roe < (mw->matrix.rows); roe++)
		{
			for(kol = mw->matrix.fixed_columns; kol < TRAILING_COLUMN_ORIGIN(mw); kol++)
			{
				xbaePositionCellWidget(mw, roe, kol);
			}
		}
		xbaeSetInitialFocus(mw);
	}

	if(!XtIsRealized((Widget) mw))
		return;

	/*
	 * Scroll the contents of the clips
	 */

	if(XtIsManaged(ClipChild(mw)))
		XbaeClipScrollHoriz(ClipChild(mw), mw->matrix.draw_gc, delta);

	if(XtIsManaged(TopClip(mw)))
		XbaeClipScrollHoriz(TopClip(mw), mw->matrix.draw_gc, delta);

	if(XtIsManaged(BottomClip(mw)))
		XbaeClipScrollHoriz(BottomClip(mw), mw->matrix.draw_gc, delta);

	if(XtIsManaged(ColumnLabelClip(mw)))
		XbaeClipScrollHoriz(ColumnLabelClip(mw), mw->matrix.draw_gc, delta);

}

#define OVERLAP(r1, r2) ( \
            (r1).x + (r1).width > (r2).x && \
            (r1).x < (r2).x + (r2).width && \
            (r1).y + (r1).height > (r2).y && \
            (r1).y < (r2).y + (r2).height)

/*
 * Redraw the cells or labels of a region that are in the expose Rectangle. 
 */
void xbaeRedrawRegion(XbaeMatrixWidget mw, XRectangle * expose, XRectangle * region)
{
	int row, start_row, end_row;
	int column, start_column, end_column;

	assert(!mw->matrix.disable_redisplay);

	DEBUGOUT(_XbaeDebug
			 (__FILE__, (Widget) mw,
			  "redrawing region with expose (x,y,w,h)=(%d,%d,%d,%d)\n",
			  expose->x, expose->y, expose->width, expose->height));

	if(OVERLAP(*expose, *region))
	{

		int xmin = Max(expose->x, region->x);
		int ymin = Max(expose->y, region->y);

		int xmax = Min(expose->x + expose->width - 1, region->x + region->width - 1);
		int ymax = Min(expose->y + expose->height - 1, region->y + region->height - 1);

		xbaeMatrixYtoRow(mw, &ymin, &start_row);
		xbaeMatrixYtoRow(mw, &ymax, &end_row);

		xbaeMatrixXtoColumn(mw, &xmin, &start_column);
		xbaeMatrixXtoColumn(mw, &xmax, &end_column);

		DEBUGOUT(_XbaeDebug
				 (__FILE__, (Widget) mw,
				  "redrawing region (rows,columns)=((%d,%d)-(%d,%d))\n",
				  start_row, start_column, end_row, end_column));

		if(start_row == -2 || start_column == -2 || (start_row == -1 && start_column == -1))
		{
			/* Do nothing, there are no rows or columns to display. */
		}
		else if(start_row == -1)
		{
			assert(end_row == -1 && start_column >= 0 && end_column >= start_column
				   && end_column < mw->matrix.columns);
			for(column = start_column; column <= end_column; column++)
			{
				xbaeDrawColumnLabel(mw, column, False);
			}
		}
		else if(start_column == -1)
		{
			assert(end_column == -1 && start_row >= 0 && end_row >= start_row && end_row < mw->matrix.rows);
			for(row = start_row; row <= end_row; row++)
			{
				xbaeDrawRowLabel(mw, row, False);
			}
		}
		else
		{
			assert(start_row >= 0 && end_row >= start_row && end_row < mw->matrix.rows);
			assert(start_column >= 0 && end_column >= start_column && end_column < mw->matrix.columns);
			for(row = start_row; row <= end_row; row++)
			{
				for(column = start_column; column <= end_column; column++)
				{
					xbaeDrawCell(mw, row, column);
				}
			}
		}
	}
}

/*
 * Redraw the fixed labels and the totally fixed cells that are in the expose Rectangle.
 */
void xbaeRedrawLabelsAndFixed(XbaeMatrixWidget mw, XRectangle * expose)
{
	/*
	 * Set up some local variables to avoid calling too many macros
	 */
	int horiz_sb_offset = HORIZ_SB_OFFSET(mw);
	int vert_sb_offset = VERT_SB_OFFSET(mw);
	int column_label_height = COLUMN_LABEL_HEIGHT(mw);
	int row_label_width = ROW_LABEL_WIDTH(mw);
	int r, c;

	/* Needed to change this for HPUX compile - it is not as smart as GNU C
	   struct {
	   Bool exists;
	   int position;
	   int size;
	   } row_regions[] = {
	   {mw->matrix.column_labels || mw->matrix.xmcolumn_labels, HORIZ_SB_OFFSET(mw), COLUMN_LABEL_HEIGHT(mw)},
	   {mw->matrix.fixed_rows, FIXED_ROW_POSITION(mw), VISIBLE_FIXED_ROW_HEIGHT(mw)},
	   {mw->matrix.trailing_fixed_rows, TRAILING_FIXED_ROW_POSITION(mw), VISIBLE_TRAILING_FIXED_ROW_HEIGHT(mw)}
	   }, column_regions[] = {
	   {mw->matrix.row_labels != NULL || mw->matrix.xmrow_labels, VERT_SB_OFFSET(mw), ROW_LABEL_WIDTH(mw)},
	   {mw->matrix.fixed_columns, FIXED_COLUMN_POSITION(mw), VISIBLE_FIXED_COLUMN_WIDTH(mw)},
	   {mw->matrix.trailing_fixed_columns, TRAILING_FIXED_COLUMN_POSITION(mw), VISIBLE_TRAILING_FIXED_COLUMN_WIDTH(mw)}
	   };
	 */
	struct {
		Bool exists;
		int position;
		int size;
	} row_regions[3], column_regions[3];

	row_regions[0].exists = (mw->matrix.column_labels || mw->matrix.xmcolumn_labels);
	row_regions[0].position = HORIZ_SB_OFFSET(mw);
	row_regions[0].size = COLUMN_LABEL_HEIGHT(mw);
	row_regions[1].exists = mw->matrix.fixed_rows;
	row_regions[1].position = FIXED_ROW_POSITION(mw);
	row_regions[1].size = VISIBLE_FIXED_ROW_HEIGHT(mw);
	row_regions[2].exists = mw->matrix.trailing_fixed_rows;
	row_regions[2].position = TRAILING_FIXED_ROW_POSITION(mw);
	row_regions[2].size = VISIBLE_TRAILING_FIXED_ROW_HEIGHT(mw);

	column_regions[0].exists = (mw->matrix.row_labels != NULL || mw->matrix.xmrow_labels);
	column_regions[0].position = VERT_SB_OFFSET(mw);
	column_regions[0].size = ROW_LABEL_WIDTH(mw);
	column_regions[1].exists = mw->matrix.fixed_columns;
	column_regions[1].position = FIXED_COLUMN_POSITION(mw);
	column_regions[1].size = VISIBLE_FIXED_COLUMN_WIDTH(mw);
	column_regions[2].exists = mw->matrix.trailing_fixed_columns;
	column_regions[2].position = TRAILING_FIXED_COLUMN_POSITION(mw);
	column_regions[2].size = VISIBLE_TRAILING_FIXED_COLUMN_WIDTH(mw);

	assert(!mw->matrix.disable_redisplay);

	DEBUGOUT(_XbaeDebug
			 (__FILE__, (Widget) mw,
			  "xbaeRedrawLabelsAndFixed() with expose (x,y)=((%d,%d)-(%d,%d))\n",
			  expose->x, expose->y, expose->width, expose->height));

	for(r = 0; r < sizeof row_regions / sizeof *row_regions; r++)
	{
		if(row_regions[r].exists)
		{
			for(c = 0; c < sizeof column_regions / sizeof *column_regions; c++)
			{
				if(column_regions[c].exists && (c != 0 || r != 0))
				{
					XRectangle region;
					region.x = (short) column_regions[c].position,
						region.y = (short) row_regions[r].position,
						region.width = (unsigned short) column_regions[c].size,
						region.height = (unsigned short) row_regions[r].size;
					xbaeRedrawRegion(mw, expose, &region);
				}
			}
		}
	}

	/* Danny Here */
	/*
	 * Draw a shadow just inside row/column labels and around outer edge
	 * of clip widget.  
	 */
	if(mw->manager.shadow_thickness)
	{
		Dimension width, height;

		width =
			VISIBLE_NON_FIXED_WIDTH(mw) + VISIBLE_FIXED_COLUMN_WIDTH(mw) +
			VISIBLE_TRAILING_FIXED_COLUMN_WIDTH(mw) + 2 * mw->manager.shadow_thickness;
		height =
			VISIBLE_NON_FIXED_HEIGHT(mw) + VISIBLE_FIXED_ROW_HEIGHT(mw) +
			VISIBLE_TRAILING_FIXED_ROW_HEIGHT(mw) + 2 * mw->manager.shadow_thickness;

		DRAW_SHADOW(XtDisplay(mw), XtWindow(mw), "win", mw->manager.top_shadow_GC,
					mw->manager.bottom_shadow_GC, mw->manager.shadow_thickness,
					row_label_width + vert_sb_offset, column_label_height + horiz_sb_offset,
					width, height, mw->matrix.shadow_type);
	}
}

/*
 * Redraws labels and cells on rows between rs and re and columns between cs and ce
 */
void xbaeRedrawAll(XbaeMatrixWidget mw, int rs, int cs, int re, int ce)
{
	int c;

	XRectangle rect;
	/* Changed for HPUX
	   Widget clips[] = {
	   ClipChild(mw),
	   TopClip(mw),
	   LeftClip(mw),
	   RightClip(mw),
	   BottomClip(mw),
	   RowLabelClip(mw),
	   ColumnLabelClip(mw)
	   };
	 */
	Widget clips[7];
	clips[0] = ClipChild(mw);
	clips[1] = TopClip(mw);
	clips[2] = LeftClip(mw);
	clips[3] = RightClip(mw);
	clips[4] = BottomClip(mw);
	clips[5] = RowLabelClip(mw);
	clips[6] = ColumnLabelClip(mw);

	assert(!mw->matrix.disable_redisplay && XtIsRealized((Widget) mw));

	xbaeSaneRectangle(mw, &rect, rs, cs, re, ce);

	for(c = 0; c < sizeof clips / sizeof *clips; c++)
	{
		if(XtIsManaged(clips[c]))
		{
			XRectangle region;
			region.x = (short) clips[c]->core.x;
			region.y = (short) clips[c]->core.y;
			region.width = (unsigned short) clips[c]->core.width;
			region.height = (unsigned short) clips[c]->core.height;
			xbaeRedrawRegion(mw, &rect, &region);
		}
	}

	xbaeRedrawLabelsAndFixed(mw, &rect);
}

/*
 * Redraws the whole matrix
 */
void xbaeRefresh(XbaeMatrixWidget mw, Boolean relayout)
{
	/*
	 * Don't respond to exposures.
	 */
	mw->matrix.disable_redisplay++;
	if(relayout)
	{
		xbaeRelayout(mw);
	}
	/*
	 * Flush pending expose events.
	 */
	XmUpdateDisplay((Widget) mw);
	/*
	 * Respond to exposures.
	 */
	mw->matrix.disable_redisplay--;

	XClearArea(XtDisplay((Widget) mw), XtWindow((Widget) mw), 0, 0, 0, 0, True);

	if(XtIsManaged(ClipChild(mw)))
		XClearArea(XtDisplay(mw), XtWindow(ClipChild(mw)), 0, 0, 0, 0, True);
	if(XtIsManaged(TopClip(mw)))
		XClearArea(XtDisplay(mw), XtWindow(TopClip(mw)), 0, 0, 0, 0, True);
	if(XtIsManaged(LeftClip(mw)))
		XClearArea(XtDisplay(mw), XtWindow(LeftClip(mw)), 0, 0, 0, 0, True);
	if(XtIsManaged(RightClip(mw)))
		XClearArea(XtDisplay(mw), XtWindow(RightClip(mw)), 0, 0, 0, 0, True);
	if(XtIsManaged(BottomClip(mw)))
		XClearArea(XtDisplay(mw), XtWindow(BottomClip(mw)), 0, 0, 0, 0, True);
	if(XtIsManaged(RowLabelClip(mw)))
		XClearArea(XtDisplay(mw), XtWindow(RowLabelClip(mw)), 0, 0, 0, 0, True);
	if(XtIsManaged(ColumnLabelClip(mw)))
		XClearArea(XtDisplay(mw), XtWindow(ColumnLabelClip(mw)), 0, 0, 0, 0, True);
}
