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
 * $Id: Actions.c,v 1.95 2005/06/13 15:22:44 tobiasoed Exp $
 */

#define	VERBOSE_SLIDE

/*
 * Actions.c created by Andrew Lister (7 August, 1995)
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <Xm/Xm.h>
#include <Xm/XmP.h>
#include <Xm/DrawP.h>
#include <Xm/ScrollBar.h>
#include <Xbae/MatrixP.h>
#include <Xbae/Draw.h>
#include <Xbae/Actions.h>
#include <Xbae/Utils.h>
#include <Xbae/ScrollMgr.h>
#include <X11/cursorfont.h>

#include "XbaeDebug.h"


#ifndef XlibSpecificationRelease
#  define XrmPermStringToQuark XrmStringToQuark
#endif

/* One of DRAW_RESIZE_LINE and DRAW_RESIZE_SHADOW must be defined. */
#if !defined(DRAW_RESIZE_LINE) && !defined(DRAW_RESIZE_SHADOW)
#  define DRAW_RESIZE_SHADOW
#endif

#ifndef DEFAULT_SCROLL_SPEED
#  define DEFAULT_SCROLL_SPEED 30
#endif

/*
 * This is the number of pixels that you can 'miss' the cell edge when
 * trying to resize a row or column.
 */
#ifndef XBAE_RESIZE_FUZZ
#  define XBAE_RESIZE_FUZZ	4
#endif

typedef struct {
	XbaeMatrixWidget mw;
	int row, column;
	int currentx, currenty;
	short *columnWidths, *rowHeights;
	Boolean resize_row, resize_column;
	Boolean grabbed;
} XbaeMatrixResizeStruct;

typedef struct {
	XbaeMatrixWidget mw;
	int row;
	int column;
	Boolean pressed;
	Boolean grabbed;
} XbaeMatrixButtonPressedStruct;

typedef struct {
	XbaeMatrixWidget mw;
	XEvent *event;
	XtIntervalId timerID;
	XtAppContext app_context;
	Boolean grabbed;
	int currentx;
	int currenty;
	int scroll_region;
} XbaeMatrixScrollStruct;

static int DoubleClick(XbaeMatrixWidget, XEvent *, int, int);
static void DrawSlideRow(XbaeMatrixWidget, int);
static void DrawSlideColumn(XbaeMatrixWidget, int);
static void PushButton(Widget, XtPointer, XEvent *, Boolean *);
static void updateScroll(XtPointer);
static void checkScrollValues(Widget, XtPointer, XEvent *, Boolean *);
static void callSelectCellCallbacks(XbaeMatrixWidget, XEvent *, int, int, String *, Cardinal);

static int last_row = -1;
static int last_column = -1;

/* ARGSUSED */
void xbaeDefaultActionACT(Widget w, XEvent * event, String * params, Cardinal * nparams)
{
	XbaeMatrixWidget mw;
	int x, y;
	int row, column;

	DEBUGOUT(_XbaeDebug
			 (__FILE__, w, "xbaeDefaultActionACT(x %d y %d)\n", event->xbutton.x, event->xbutton.y));

	/*
	 * Get Matrix widget and make sure it is a Matrix subclass.
	 * w could be Matrix, or the Clip or textField children of Matrix
	 */

	if(XtIsSubclass(w, xbaeMatrixWidgetClass))
		mw = (XbaeMatrixWidget) w;
	else if(XtIsSubclass(XtParent(w), xbaeMatrixWidgetClass))
		mw = (XbaeMatrixWidget) XtParent(w);
	else
	{
		XtAppWarningMsg(XtWidgetToApplicationContext(w), "defaultActionACT", "badWidget",
						"XbaeMatrix", "XbaeMatrix: Bad widget passed to DefaultAction action", NULL, 0);
		return;
	}

	if(!mw->matrix.default_action_callback)
		return;

	if(!xbaeEventToMatrixXY(mw, event, &x, &y))
		return;

	if(!xbaeMatrixXYToRowCol(mw, &x, &y, &row, &column))
		return;

	if(DoubleClick(mw, event, row, column))
	{
		XbaeMatrixDefaultActionCallbackStruct call_data;

		call_data.reason = XbaeDefaultActionReason;
		call_data.event = event;
		call_data.row = row;
		call_data.column = column;

		XtCallCallbackList((Widget) mw, mw->matrix.default_action_callback, (XtPointer) & call_data);
	}
}

/* ARGSUSED */
void xbaeHandleTrackingACT(Widget w, XEvent * event, String * params, Cardinal * nparams)
{
	XbaeMatrixTrackCellCallbackStruct call_data;

	XbaeMatrixWidget mw;
	int x, y;
	int row, column;

	DEBUGOUT(_XbaeDebug(__FILE__, w, "%s(x %d y %d)\n", __FUNCTION__, event->xbutton.x, event->xbutton.y));

	/*
	 * Get Matrix widget and make sure it is a Matrix subclass.
	 * w could be Matrix, or the Clip or textField children of Matrix
	 */

	if(XtIsSubclass(w, xbaeMatrixWidgetClass))
		mw = (XbaeMatrixWidget) w;
	else if(XtIsSubclass(XtParent(w), xbaeMatrixWidgetClass))
		mw = (XbaeMatrixWidget) XtParent(w);
	else
	{
		XtAppWarningMsg(XtWidgetToApplicationContext(w), "handleTrackingACT", "badWidget",
						"XbaeMatrix", "XbaeMatrix: Bad widget passed to HandleTracking action", NULL, 0);
		return;
	}

	if(!mw->matrix.track_cell_callback)
		return;


	if(!xbaeEventToMatrixXY(mw, event, &x, &y))
		return;

	call_data.pointer_x = x;
	call_data.pointer_y = y;

	(void) xbaeMatrixXYToRowCol(mw, &x, &y, &row, &column);

	if((column != mw->matrix.prev_column) || (row != mw->matrix.prev_row))
	{
		call_data.reason = XbaeTrackCellReason;
		call_data.event = event;
		call_data.prev_row = mw->matrix.prev_row;
		call_data.prev_column = mw->matrix.prev_column;
		call_data.row = row;
		call_data.column = column;

		/* printf ("%s %s %d  XY:%d %d RC: %d %d\n",
		 * __FILE__,__FUNCTION__,__LINE__,x, y, row, column);
		 */

		XtCallCallbackList((Widget) mw, mw->matrix.track_cell_callback, (XtPointer) & call_data);

		/*
		 * Merken
		 */
		mw->matrix.prev_column = column;
		mw->matrix.prev_row = row;
	}
}

#ifdef DRAW_RESIZE_LINE
#  define RESIZE_SIZE 0
#  define DRAW_RESIZE(mw, display, win, dbg, x, y, w, h) \
        XDrawLine(display, win, mw->matrix.draw_gc, x, y, x + w, y + h)
#endif

#ifdef DRAW_RESIZE_SHADOW
/* These values derived through that age-old process of what looks good to me */
#  define RESIZE_SIZE 4
#  define DRAW_RESIZE(mw, display, win, dbg, x, y, w, h) \
        DRAW_SHADOW(display, win, dbg, \
                    mw->matrix.resize_top_shadow_gc, \
                    mw->matrix.resize_bottom_shadow_gc, RESIZE_SIZE / 2, x, y, w, h, XmSHADOW_OUT)
#endif

static void DrawSlideColumn(XbaeMatrixWidget mw, int x)
{
	Display *display = XtDisplay(mw);

	/*
	 * Draw On the matrix
	 */

	DRAW_RESIZE(mw, display, XtWindow(mw), "win",
				x, FIXED_ROW_POSITION(mw),
				RESIZE_SIZE,
				VISIBLE_NON_FIXED_HEIGHT(mw) + VISIBLE_FIXED_ROW_HEIGHT(mw)
				+ VISIBLE_TRAILING_FIXED_ROW_HEIGHT(mw));

	/* 
	 * Draw on the clips
	 */

	if(x >= FIXED_COLUMN_POSITION(mw) && x < FIXED_COLUMN_POSITION(mw) + VISIBLE_FIXED_COLUMN_WIDTH(mw))
	{
		if(XtIsManaged(LeftClip(mw)))
		{
			DRAW_RESIZE(mw, display, XtWindow(LeftClip(mw)), "LeftClip",
						x - FIXED_COLUMN_POSITION(mw), 0, RESIZE_SIZE, LeftClip(mw)->core.height);
		}
	}
	else if(x >= TRAILING_FIXED_COLUMN_POSITION(mw)
			&& x < TRAILING_FIXED_COLUMN_POSITION(mw) + VISIBLE_TRAILING_FIXED_COLUMN_WIDTH(mw))
	{
		if(XtIsManaged(RightClip(mw)))
		{
			DRAW_RESIZE(mw, display, XtWindow(RightClip(mw)), "RightClip",
						x - TRAILING_FIXED_COLUMN_POSITION(mw), 0, RESIZE_SIZE, RightClip(mw)->core.height);
		}
	}
	else if(x >= NON_FIXED_COLUMN_POSITION(mw)
			&& x < NON_FIXED_COLUMN_POSITION(mw) + VISIBLE_NON_FIXED_WIDTH(mw))
	{
		if(XtIsManaged(ClipChild(mw)))
		{
			DRAW_RESIZE(mw, display, XtWindow(ClipChild(mw)), "ClipChild",
						x - NON_FIXED_COLUMN_POSITION(mw), 0, RESIZE_SIZE, ClipChild(mw)->core.height);
		}
		if(XtIsManaged(TopClip(mw)))
		{
			DRAW_RESIZE(mw, display, XtWindow(TopClip(mw)), "TopClip",
						x - NON_FIXED_COLUMN_POSITION(mw), 0, RESIZE_SIZE, TopClip(mw)->core.height);
		}
		if(XtIsManaged(BottomClip(mw)))
		{
			DRAW_RESIZE(mw, display, XtWindow(BottomClip(mw)), "BottomClip",
						x - NON_FIXED_COLUMN_POSITION(mw), 0, RESIZE_SIZE, BottomClip(mw)->core.height);
		}
	}
}

/* Derived from DrawSlideColumn */
static void DrawSlideRow(XbaeMatrixWidget mw, int y)
{
	Display *display = XtDisplay(mw);

	/*
	 * Draw On the matrix
	 */

	DRAW_RESIZE(mw, display, XtWindow(mw), "win",
				FIXED_COLUMN_POSITION(mw), y,
				VISIBLE_NON_FIXED_WIDTH(mw) + VISIBLE_FIXED_COLUMN_WIDTH(mw)
				+ VISIBLE_TRAILING_FIXED_COLUMN_WIDTH(mw), RESIZE_SIZE);

	/* 
	 * Draw on the clips
	 */

	if(y >= FIXED_ROW_POSITION(mw) && y < FIXED_ROW_POSITION(mw) + VISIBLE_FIXED_ROW_HEIGHT(mw))
	{
		if(XtIsManaged(TopClip(mw)))
		{
			DRAW_RESIZE(mw, display, XtWindow(TopClip(mw)), "TopClip",
						0, y - FIXED_ROW_POSITION(mw), TopClip(mw)->core.width, RESIZE_SIZE);
		}
	}
	else if(y >= TRAILING_FIXED_ROW_POSITION(mw)
			&& y < TRAILING_FIXED_ROW_POSITION(mw) + VISIBLE_TRAILING_FIXED_ROW_HEIGHT(mw))
	{
		if(XtIsManaged(BottomClip(mw)))
		{
			DRAW_RESIZE(mw, display, XtWindow(BottomClip(mw)), "BottomClip",
						0, y - TRAILING_FIXED_ROW_POSITION(mw), BottomClip(mw)->core.width, RESIZE_SIZE);
		}
	}
	else if(y >= NON_FIXED_ROW_POSITION(mw) && y < NON_FIXED_ROW_POSITION(mw) + VISIBLE_NON_FIXED_HEIGHT(mw))
	{
		if(XtIsManaged(ClipChild(mw)))
		{
			DRAW_RESIZE(mw, display, XtWindow(ClipChild(mw)), "ClipChild",
						0, y - NON_FIXED_ROW_POSITION(mw), ClipChild(mw)->core.width, RESIZE_SIZE);

		}
		if(XtIsManaged(LeftClip(mw)))
		{
			DRAW_RESIZE(mw, display, XtWindow(LeftClip(mw)), "LeftClip",
						0, y - NON_FIXED_ROW_POSITION(mw), LeftClip(mw)->core.width, RESIZE_SIZE);
		}
		if(XtIsManaged(RightClip(mw)))
		{
			DRAW_RESIZE(mw, display, XtWindow(RightClip(mw)), "RightClip",
						0, y - NON_FIXED_ROW_POSITION(mw), RightClip(mw)->core.width, RESIZE_SIZE);
		}
	}
}

static void MoveSlide(XbaeMatrixWidget mw, int event_pos, int *current_pos, short *current_size,
					  Boolean size_in_pixels, int font_size, int smallest_allowed,
					  void (*DrawSlide) (XbaeMatrixWidget, int))
{

	int new_size, new_pos, delta;

	/*
	 * Calculate the new size and the new position of the slider
	 */
	if(size_in_pixels)
	{
		delta = event_pos - *current_pos;
		if(*current_size + delta < smallest_allowed)
		{
			/* Must keep the size greate than smallest_allowed */
			delta = smallest_allowed - *current_size;
		}

		new_size = *current_size + delta;
		new_pos = *current_pos + delta;
	}
	else
	{
		delta = (event_pos - *current_pos) / font_size;
		if(*current_size + delta < 1)
		{
			/* Must keep the size at least on character big */
			delta = 1 - *current_size;
		}

		new_size = *current_size + delta;
		new_pos = *current_pos + delta * font_size;
	}

	/*
	 * If the new size is different from the old one, move the slider and update its position
	 */
	if(new_size != *current_size)
	{
		/* erase the old marker */
		DrawSlide(mw, *current_pos);

		/* Save the new size of the row and the position of the marker */
		*current_size = new_size;
		*current_pos = new_pos;

		/* Draw the marker line in the new location */
		DrawSlide(mw, *current_pos);
	}
}

/* Derived from SlideColumn */
static void Slide(Widget w, XtPointer data, XEvent * event, Boolean * cont)
{
	XbaeMatrixResizeStruct *rd = (XbaeMatrixResizeStruct *) data;

	if(event->type == MotionNotify)
	{
		/* 
		 * SGO: implemented a limit so that rows can't be smaller than 5 pixels
		 * that does't makes sense in most cases. 
		 */
		int ROW_MINIMUM_HEIGHT = 5;
		int COLUMN_MINIMUM_WIDTH = 1;

		XMotionEvent *motionEvent = (XMotionEvent *) event;

#ifdef XBAE_VERY_VERBOSE
		DEBUGOUT(_XbaeDebug(__FILE__, (Widget) w, "Slide MotionNotify()\n"));
#endif

		if(rd->resize_row)
		{
			MoveSlide(rd->mw, motionEvent->y, &rd->currenty, &rd->rowHeights[rd->row],
					  rd->mw->matrix.row_height_in_pixels, TEXT_HEIGHT(rd->mw), ROW_MINIMUM_HEIGHT,
					  DrawSlideRow);

		}

		if(rd->resize_column)
		{
			MoveSlide(rd->mw, motionEvent->x, &rd->currentx, &rd->columnWidths[rd->column],
					  rd->mw->matrix.column_width_in_pixels, CELL_FONT_WIDTH(rd->mw), COLUMN_MINIMUM_WIDTH,
					  DrawSlideColumn);
		}

	}
	else if(event->type == ButtonRelease)
	{
		DEBUGOUT(_XbaeDebug(__FILE__, w, "Slide ButtonRelease()\n"));

		/*
		 * We are done
		 */

		rd->grabbed = False;
	}
}

/* ARGSUSED */
void xbaeResizeColumnsACT(Widget w, XEvent * event, String * params, Cardinal * nparams)
{
	XbaeMatrixWidget mw;
	int x, y;
	int start_row, start_column;
	int i;
	XbaeMatrixResizeStruct resizeData;
	XtAppContext appcontext;
#ifdef DRAW_RESIZE_LINE
	XGCValues save, values;
	unsigned long gcmask;
#endif
	unsigned long event_mask;
	Display *display = XtDisplay(w);
	int fuzzy = XBAE_RESIZE_FUZZ;

	/*
	 * Get Matrix widget and make sure it is a Matrix subclass.
	 * w could be Matrix, or the Clip or textField children of Matrix
	 */
	if(XtIsSubclass(w, xbaeMatrixWidgetClass))
		mw = (XbaeMatrixWidget) w;
	else if(XtIsSubclass(XtParent(w), xbaeMatrixWidgetClass))
		mw = (XbaeMatrixWidget) XtParent(w);
	else
	{
		XtAppWarningMsg(XtWidgetToApplicationContext(w), "resizeColumnsACT", "badWidget",
						"XbaeMatrix", "XbaeMatrix: Bad widget passed to ResizeColumns action", NULL, 0);
		return;
	}

	if(!xbaeEventToMatrixXY(mw, event, &x, &y))
	{
		DEBUGOUT(_XbaeDebug(__FILE__, (Widget) mw, "xbaeResizeColumnsACT: cannot convert to x/y\n"));
		return;
	}
	DEBUGOUT(_XbaeDebug(__FILE__, (Widget) mw, "xbaeResizeColumnsACT: x %d y %d\n", x, y));

	if(!xbaeMatrixXYToRowCol(mw, &x, &y, &start_row, &start_column))
	{
		DEBUGOUT(_XbaeDebug(__FILE__, (Widget) mw, "xbaeResizeColumnsACT: cannot convert to row/col\n"));
		return;
	}
	DEBUGOUT(_XbaeDebug
			 (__FILE__, (Widget) mw, "xbaeResizeColumnsACT: x %d y %d, row %d col %d\n", x, y,
			  resizeData.row, resizeData.column));

	/*
	 * Figure out what the fuzzyness is
	 */
	if(mw->matrix.cell_shadow_thickness > fuzzy)
	{
		fuzzy = mw->matrix.cell_shadow_thickness;
	}

	/*
	 * Figure out if we're resizing a row and/or a column
	 */
	resizeData.resize_row = False;
	if(mw->matrix.allow_row_resize)
	{
		if(y <= fuzzy)
		{
			/* 
			 * The click is at the top edge of the row
			 */
			resizeData.row = start_row - 1;
			resizeData.resize_row = (resizeData.row != -1);	/* Can't resize top edge of top row */
		}
		else if(ROW_HEIGHT(mw, start_row) - y <= fuzzy)
		{
			/* 
			 * The click is at the bottom edge of a row
			 */
			resizeData.row = start_row;
			resizeData.resize_row = True;
		}
	}

	resizeData.resize_column = False;
	if(mw->matrix.allow_column_resize)
	{
		if(x <= fuzzy)
		{
			/* 
			 * The click is at the left edge of the column
			 */
			resizeData.column = start_column - 1;
			resizeData.resize_column = (resizeData.column != -1);	/* Can't resize left edge of left column */
		}
		else if(COLUMN_WIDTH(mw, start_column) - x <= fuzzy)
		{
			/* 
			 * The click is at the right edge of a column
			 */
			resizeData.column = start_column;
			resizeData.resize_column = True;
		}
	}

	if(resizeData.resize_row || resizeData.resize_column)
	{

		/*
		 * Make it here and it's time to start the fun stuff!
		 */

		Boolean haveText, haveHSB, haveVSB, resized_rows, resized_columns;

		haveText = mw->matrix.text_child_is_mapped;
		if(haveText)
		{
			/* 
			 * Say goodbye to the TextChild -> it only gets in the way!
			 */
			xbaeHideTextChild(mw);
			/*
			 * Redraw the cell that had the text field in it or it might stay blank
			 */
			xbaeDrawCell(mw, mw->matrix.current_row, mw->matrix.current_column);
		}

#ifdef DRAW_RESIZE_LINE
		/*
		 * Flush the commit events out to the server.  Otherwise, our changes
		 * to the GCs below have a bad effect.
		 */
		XSync(display, False);

		gcmask = GCForeground | GCBackground | GCFunction | GCPlaneMask | GCLineWidth;
		XGetGCValues(display, mw->matrix.draw_gc, gcmask, &save);
		values.foreground = mw->core.background_pixel;
		values.background = mw->manager.foreground;
		values.function = GXxor;
		values.plane_mask = AllPlanes;
		values.line_width = 3;
		XChangeGC(display, mw->matrix.draw_gc, gcmask, &values);
#endif

		haveHSB = False;
		if(resizeData.resize_row)
		{
			if(XtIsManaged(HorizScrollChild(mw)) && !SCROLLBAR_LEFT(mw))
			{
				/*
				 * Say goodbye to the Horizontal ScrollBar -> it only gets in the way!
				 */
				haveHSB = True;
				XtUnmanageChild(HorizScrollChild(mw));
			}

			/* Copy the rowHeight array */
			resizeData.rowHeights = (short *) XtMalloc(mw->matrix.rows * sizeof(short));
			for(i = 0; i < mw->matrix.rows; i++)
				resizeData.rowHeights[i] = mw->matrix.row_heights[i];

			DrawSlideRow(mw, event->xbutton.y);
		}

		haveVSB = False;
		if(resizeData.resize_column)
		{
			if(XtIsManaged(VertScrollChild(mw)) && !SCROLLBAR_TOP(mw))
			{
				/*
				 * Say goodbye to the Vertical ScrollBar -> it only gets in the way!
				 */
				haveVSB = True;
				XtUnmanageChild(VertScrollChild(mw));
			}

			/* Copy the columnWidth array */
			resizeData.columnWidths = (short *) XtMalloc(mw->matrix.columns * sizeof(short));
			for(i = 0; i < mw->matrix.columns; i++)
				resizeData.columnWidths[i] = mw->matrix.column_widths[i];

			DrawSlideColumn(mw, event->xbutton.x);
		}

		/* Create the cursor */
		if(mw->matrix.cursor)
		{
			XFreeCursor(display, mw->matrix.cursor);
		}

		if(resizeData.resize_row && resizeData.resize_column)
		{
			mw->matrix.cursor = XCreateFontCursor(display, XC_sizing);
		}
		else if(resizeData.resize_row)
		{
			mw->matrix.cursor = XCreateFontCursor(display, XC_sb_v_double_arrow);
		}
		else if(resizeData.resize_column)
		{
			mw->matrix.cursor = XCreateFontCursor(display, XC_sb_h_double_arrow);
		}

		event_mask = PointerMotionMask | ButtonReleaseMask;
		XtAddEventHandler(w, event_mask, True, (XtEventHandler) Slide, (XtPointer) & resizeData);

		XGrabPointer(display, XtWindow(w), True, event_mask, GrabModeAsync, GrabModeAsync,
					 XtWindow((Widget) mw), mw->matrix.cursor, CurrentTime);

		resizeData.grabbed = True;
		resizeData.mw = mw;
		resizeData.currentx = event->xbutton.x;
		resizeData.currenty = event->xbutton.y;

		appcontext = XtWidgetToApplicationContext(w);

		while(resizeData.grabbed)
			XtAppProcessEvent(appcontext, XtIMAll);

		XtRemoveEventHandler(w, event_mask, True, (XtEventHandler) Slide, (XtPointer) & resizeData);

		XUngrabPointer(XtDisplay(w), CurrentTime);

		resized_rows = False;
		if(resizeData.resize_row)
		{
			/*
			 * Erase the slider
			 */
			DrawSlideRow(mw, resizeData.currenty);

			if(mw->matrix.resize_row_callback)
			{
				XbaeMatrixResizeRowCallbackStruct call_data;

				call_data.reason = XbaeResizeRowReason;
				call_data.event = event;
				call_data.row = start_row;
				call_data.column = start_column;
				call_data.which = resizeData.row;
				call_data.rows = mw->matrix.rows;
				call_data.row_heights = resizeData.rowHeights;
				XtCallCallbackList((Widget) mw, mw->matrix.resize_row_callback, (XtPointer) & call_data);
			}

			for(i = 0; !resized_rows && i < mw->matrix.rows; i++)
			{
				resized_rows = (resizeData.rowHeights[i] != mw->matrix.row_heights[i]);
			}
		}

		resized_columns = False;
		if(resizeData.resize_column)
		{
			/*
			 * Erase the slider
			 */
			DrawSlideColumn(mw, resizeData.currentx);

			if(mw->matrix.resize_column_callback)
			{
				XbaeMatrixResizeColumnCallbackStruct call_data;

				call_data.reason = XbaeResizeColumnReason;
				call_data.event = event;
				call_data.row = start_row;
				call_data.column = start_column;
				call_data.which = resizeData.row;
				call_data.columns = resizeData.mw->matrix.columns;
				call_data.column_widths = resizeData.columnWidths;
				XtCallCallbackList((Widget) mw, mw->matrix.resize_column_callback, (XtPointer) & call_data);
			}

			for(i = 0; !resized_columns && i < mw->matrix.columns; i++)
			{
				resized_columns = (resizeData.columnWidths[i] != mw->matrix.column_widths[i]);
			}
		}

#ifdef DRAW_RESIZE_LINE
		XChangeGC(display, mw->matrix.draw_gc, gcmask, &save);
#endif

		/*
		 * Make sure everything is handled correctly with SetValues 
		 */
		if(resized_rows && resized_columns)
		{
			XtVaSetValues((Widget) mw,
						  XmNrowHeights, resizeData.rowHeights,
						  XmNcolumnWidths, resizeData.columnWidths, NULL);
		}
		else if(resized_rows)
		{
			XtVaSetValues((Widget) mw, XmNrowHeights, resizeData.rowHeights, NULL);
		}
		else if(resized_columns)
		{
			XtVaSetValues((Widget) mw, XmNcolumnWidths, resizeData.columnWidths, NULL);
		}
		else
		{
			/* 
			 * Since we don't call SetValues in this case, we must remanage the 
			 * scrollbars if we unmanaged them.
			 */

			if(haveHSB)
			{
				XtManageChild(HorizScrollChild(mw));
			}

			if(haveVSB)
			{
				XtManageChild(VertScrollChild(mw));
			}
		}

		if(resizeData.resize_row)
		{
			XtFree((XtPointer) resizeData.rowHeights);
		}
		if(resizeData.resize_column)
		{
			XtFree((XtPointer) resizeData.columnWidths);
		}

		if(haveText)
		{
			xbaePositionTextChild(mw);
		}
	}
}

/*
 * Action to process a drag out
 */
void xbaeProcessDragACT(Widget w, XEvent * event, String * params, Cardinal * nparams)
{
	XbaeMatrixWidget mw;
	XbaeMatrixCellValuesStruct cell_values;

	int x, y;
	int row, column;
	XbaeMatrixProcessDragCallbackStruct call_data;

	/*
	 * Get Matrix widget and make sure it is a Matrix subclass.
	 * w could be Matrix, or the Clip or textField children of Matrix
	 */
	if(XtIsSubclass(w, xbaeMatrixWidgetClass))
		mw = (XbaeMatrixWidget) w;
	else if(XtIsSubclass(XtParent(w), xbaeMatrixWidgetClass))
		mw = (XbaeMatrixWidget) XtParent(w);
	else
	{
		XtAppWarningMsg(XtWidgetToApplicationContext(w), "processDragACT", "badWidget",
						"XbaeMatrix", "XbaeMatrix: Bad widget passed to ProcessDrag action", NULL, 0);
		return;
	}

	if(!mw->matrix.process_drag_callback)
		return;

	if(!xbaeEventToMatrixXY(mw, event, &x, &y))
		return;

	if(!xbaeMatrixXYToRowCol(mw, &x, &y, &row, &column))
		return;

	xbaeGetCellValues(mw, row, column, False, &cell_values);

	call_data.type = cell_values.drawCB.type;
	call_data.string = cell_values.drawCB.string;
	call_data.pixmap = cell_values.drawCB.pixmap;
	call_data.mask = cell_values.drawCB.mask;

	call_data.reason = XbaeProcessDragReason;
	call_data.event = event;
	call_data.row = row;
	call_data.column = column;
	call_data.num_params = *nparams;
	call_data.params = params;

	XtCallCallbackList((Widget) mw, mw->matrix.process_drag_callback, (XtPointer) & call_data);

	if((cell_values.drawCB.type & XbaeStringFree) == XbaeStringFree)
	{
		XtFree((XtPointer) cell_values.drawCB.string);
	}
}

/*
 * Action to edit a non-fixed cell.
 */
static void
callTraverseCellCallbacks(XbaeMatrixWidget mw, XEvent * event, int *row, int *column, String param,
						  XrmQuark qparam)
{
	/*
	 * Call the traverseCellCallback to allow the application to
	 * perform custom traversal.
	 */
	XbaeMatrixTraverseCellCallbackStruct call_data;

	call_data.reason = XbaeTraverseCellReason;
	call_data.event = event;
	call_data.row = mw->matrix.current_row;
	call_data.column = mw->matrix.current_column;
	call_data.next_row = *row;
	call_data.next_column = *column;
	call_data.fixed_rows = mw->matrix.fixed_rows;
	call_data.fixed_columns = mw->matrix.fixed_columns;
	call_data.trailing_fixed_rows = mw->matrix.trailing_fixed_rows;
	call_data.trailing_fixed_columns = mw->matrix.trailing_fixed_columns;
	call_data.num_rows = mw->matrix.rows;
	call_data.num_columns = mw->matrix.columns;
	call_data.param = param;
	call_data.qparam = qparam;

	XtCallCallbackList((Widget) mw, mw->matrix.traverse_cell_callback, (XtPointer) & call_data);

	*row = call_data.next_row;
	*column = call_data.next_column;
}


void xbaeEditCellACT(Widget w, XEvent * event, String * params, Cardinal * nparams)
{
	XbaeMatrixWidget mw;
	int row, column, current_focus_row, current_focus_column;
	XrmQuark q;
	static XrmQuark QPointer, QLeft, QRight, QUp, QDown;
	static Boolean haveQuarks = False;
	/*
	 * Get static quarks for the parms we understand
	 */
	if(!haveQuarks)
	{
		QPointer = XrmPermStringToQuark("Pointer");
		QLeft = XrmPermStringToQuark("Left");
		QRight = XrmPermStringToQuark("Right");
		QUp = XrmPermStringToQuark("Up");
		QDown = XrmPermStringToQuark("Down");
		haveQuarks = True;
	}

	/*
	 * Get Matrix widget and make sure it is a Matrix subclass.
	 * w could be Matrix, or the Clip or textField children of Matrix
	 */
	if(XtIsSubclass(w, xbaeMatrixWidgetClass))
		mw = (XbaeMatrixWidget) w;
	else if(XtIsSubclass(XtParent(w), xbaeMatrixWidgetClass))
		mw = (XbaeMatrixWidget) XtParent(w);
	else
	{
		XtAppWarningMsg(XtWidgetToApplicationContext(w), "editCellACT", "badWidget",
						"XbaeMatrix", "XbaeMatrix: Bad widget passed to EditCell action", NULL, 0);
		return;
	}

	/*
	 * Make sure we have a single parm
	 */
	if(*nparams != 1)
	{
		XtAppWarningMsg(XtWidgetToApplicationContext(w), "editCellACT", "badParms",
						"XbaeMatrix", "XbaeMatrix: Wrong params passed to EditCell action, needs 1", NULL, 0);
		return;
	}

	/*
	 * Quarkify the string param
	 */
	q = XrmStringToQuark(params[0]);

	/*
	 * We don't know who has the focus
	 */
	current_focus_row = -1;
	current_focus_column = -1;

	if(q == QPointer)
	{
		int x, y;
		if(!xbaeEventToMatrixXY(mw, event, &x, &y)
		   || !xbaeMatrixXYToRowCol(mw, &x, &y, &row, &column)
		   || (IS_FIXED(mw, row, column) && !mw->matrix.traverse_fixed))
		{
			return;
		}
	}
	else
	{
		/*
		 * Find out what cell has the focus
		 */
		if(w == TextChild(mw))
		{
			current_focus_row = mw->matrix.current_row;
			current_focus_column = mw->matrix.current_column;
		}
		else if(mw->matrix.per_cell)
		{
			/* Find the cell widget that has focus */

			Boolean found = False;
			int search_row, search_column;

#if 0							/* Brutal method */

			for(search_row = 0; !found && search_row < mw->matrix.rows; search_row++)
			{
				for(search_column = 0; !found && search_column < mw->matrix.columns; search_column++)
				{
					found = (mw->matrix.per_cell[search_row][search_column].widget == w);
				}
			}

			if(found)
			{
				current_focus_row = search_row - 1;
				current_focus_column = search_column - 1;
			}

#else /* less brutal */

			int x = w->core.x;
			int y = w->core.y;

			found = xbaeMatrixXYToRowCol(mw, &x, &y, &search_row, &search_column);

			if(found)
			{
				current_focus_row = search_row;
				current_focus_column = search_column;
			}

#endif
		}

		/*
		 * Make sure current_focus_row/column are sane
		 */
		if(current_focus_row < 0 || current_focus_row >= mw->matrix.rows)
		{
			current_focus_row = xbaeTopRow(mw);
		}
		if(current_focus_column < 0 || current_focus_column >= mw->matrix.columns)
		{
			current_focus_column = xbaeLeftColumn(mw);
		}

		/*
		 * Initialize row/column to the current position
		 */
		row = current_focus_row;
		column = current_focus_column;

		if(q == QRight)
		{
			/*
			 * If we are in the lower right corner, stay there.
			 * Otherwise move over a column. If we move off to the right of the
			 * final column to which traversing is allowed then move down a row
			 * and back to the first column to which traversing is allowed.
			 */
			if(!mw->matrix.traverse_fixed)
			{
				/* check scrollable boundary */
				if(current_focus_row != TRAILING_ROW_ORIGIN(mw) - 1
				   || current_focus_column != TRAILING_COLUMN_ORIGIN(mw) - 1)
				{
					column++;
					if(IS_TRAILING_FIXED_COLUMN(mw, column))
					{
						column = mw->matrix.fixed_columns;
						row++;
					}
				}
			}
			else
			{
				/* check matrix boundary */
				if(current_focus_row != mw->matrix.rows - 1 || current_focus_column != mw->matrix.columns - 1)
				{
					column++;
					if(column >= mw->matrix.columns)
					{
						column = 0;
						row++;
					}
				}
			}
		}
		else if(q == QLeft)
		{
			/*
			 * If we are in the upper left corner, stay there.
			 * Otherwise move back a column. If we move before the first column
			 * to which traversing is allowed, move up a row and over to the last
			 * column to which traversing is allowed.
			 */
			if(!mw->matrix.traverse_fixed)
			{
				/* check scrollable boundary */
				if(current_focus_row != mw->matrix.fixed_rows
				   || current_focus_column != mw->matrix.fixed_columns)
				{
					column--;
					if(IS_LEADING_FIXED_COLUMN(mw, column))
					{
						column = TRAILING_COLUMN_ORIGIN(mw) - 1;
						row--;
					}
				}
			}
			else
			{
				if(current_focus_row != 0 || current_focus_column != 0)
				{
					column--;
					if(column < 0)
					{
						column = mw->matrix.columns - 1;
						row--;
					}
				}
			}
		}
		else if(q == QDown)
		{
			row++;

			/* adjust row for allowable traversable regions */
			if(!mw->matrix.traverse_fixed)
			{
				if(IS_TRAILING_FIXED_ROW(mw, row))
					row = mw->matrix.fixed_rows;
			}
			else
			{
				if(row >= mw->matrix.rows)
					row = 0;
			}
		}
		else if(q == QUp)
		{
			row--;

			if(!mw->matrix.traverse_fixed)
			{
				if(IS_LEADING_FIXED_ROW(mw, row))
					row = TRAILING_ROW_ORIGIN(mw) - 1;
			}
			else
			{
				if(row < 0)
					row = mw->matrix.rows - 1;
			}
		}
	}

	/*
	 * Call the traverseCellCallback to allow the application to
	 * perform custom traversal.
	 */
	if(mw->matrix.traverse_cell_callback)
	{
		callTraverseCellCallbacks(mw, event, &row, &column, params[0], q);
	}

	((XbaeMatrixWidgetClass) XtClass(mw))->matrix_class.edit_cell(mw, event, row, column, params, *nparams);
}

/*
 * Action to unmap the textField and discard any edits made
 */

/* ARGSUSED */
void xbaeCancelEditACT(Widget w, XEvent * event, String * params, Cardinal * nparams)
{
	XbaeMatrixWidget mw;
	Boolean unmap;

	/*
	 * Get Matrix widget and make sure it is a Matrix subclass.
	 * w could be Matrix, or the Clip or textField children of Matrix
	 */
	if(XtIsSubclass(w, xbaeMatrixWidgetClass))
		mw = (XbaeMatrixWidget) w;
	else if(XtIsSubclass(XtParent(w), xbaeMatrixWidgetClass))
		mw = (XbaeMatrixWidget) XtParent(w);
	else
	{
		XtAppWarningMsg(XtWidgetToApplicationContext(w), "cancelEditACT", "badWidget",
						"XbaeMatrix", "XbaeMatrix: Bad widget passed to CancelEdit action", NULL, 0);
		return;
	}

	/*
	 * Make sure we have a single param
	 */
	if(*nparams != 1)
	{
		XtAppWarningMsg(XtWidgetToApplicationContext(w), "cancelEditACT", "badParms",
						"XbaeMatrix",
						"XbaeMatrix: Wrong params passed to CancelEdit action, needs 1", NULL, 0);
		return;
	}

	/*
	 * Validate our param
	 */
	if(!strcmp(params[0], "True"))
		unmap = True;
	else if(!strcmp(params[0], "False"))
		unmap = False;
	else
	{
		XtAppWarningMsg(XtWidgetToApplicationContext(w), "cancelEditACT", "badParm",
						"XbaeMatrix", "XbaeMatrix: Bad parameter for CancelEdit action", NULL, 0);
		return;
	}

	if(!mw->matrix.text_child_is_mapped)
	{
		/*
		 * The textChild is not visible, call ManagerParentCancel
		 */
		XtCallActionProc((Widget) mw, "ManagerParentCancel", event, params, *nparams);
	}
	else
	{
		/*
		 * The textChild is visible, call the cancel_edit method
		 */
		((XbaeMatrixWidgetClass) XtClass(mw))->matrix_class.cancel_edit(mw, unmap);
	}
}

/*
 * Action save any edits made and unmap the textField if params[0] is True
 */

/* ARGSUSED */
void xbaeCommitEditACT(Widget w, XEvent * event, String * params, Cardinal * nparams)
{
	XbaeMatrixWidget mw;
	Boolean unmap;

	/*
	 * Get Matrix widget and make sure it is a Matrix subclass.
	 * w could be Matrix, or the Clip or textField children of Matrix
	 */
	if(XtIsSubclass(w, xbaeMatrixWidgetClass))
		mw = (XbaeMatrixWidget) w;
	else if(XtIsSubclass(XtParent(w), xbaeMatrixWidgetClass))
		mw = (XbaeMatrixWidget) XtParent(w);
	else
	{
		XtAppWarningMsg(XtWidgetToApplicationContext(w), "commitEditACT", "badWidget",
						"XbaeMatrix", "XbaeMatrix: Bad widget passed to CommitEdit action", NULL, 0);
		return;
	}

	/*
	 * Make sure we have a single param
	 */
	if(*nparams != 1)
	{
		XtAppWarningMsg(XtWidgetToApplicationContext(w), "commitEditACT", "badParms",
						"XbaeMatrix", "XbaeMatrix: Wrong params for CommitEdit action, needs 1", NULL, 0);
		return;
	}

	/*
	 * Validate our param
	 */
	if(!strcmp(params[0], "True"))
		unmap = True;
	else if(!strcmp(params[0], "False"))
		unmap = False;
	else
	{
		XtAppWarningMsg(XtWidgetToApplicationContext(w), "commitEditACT", "badParm",
						"XbaeMatrix", "XbaeMatrix: Bad parameter for CommitEdit action", NULL, 0);
		return;
	}

	if(!mw->matrix.text_child_is_mapped)
	{
		/*
		 * The textChild is not visible, call ManagerParentActivate
		 */
		XtCallActionProc((Widget) mw, "ManagerParentActivate", event, params, *nparams);
	}
	else
	{
		/*
		 * The textChild is visible, call the commit_edit method
		 */
		((XbaeMatrixWidgetClass) XtClass(mw))->matrix_class.commit_edit(mw, event, unmap);
	}
}

static int DoubleClick(XbaeMatrixWidget mw, XEvent * event, int row, int column)
{
	/* A double click in this instance is two clicks in the
	   same cell in a time period < double_click_interval */
	Time current_time;
	unsigned long delta;
	static int ret = 0;
	static int lastButton = 0;

	if(event->type == ButtonRelease)
	{
		/* If the button is released, store the current location and time -
		   next time through, if it's a button press event, we check for
		   double click */
		mw->matrix.last_row = row;
		mw->matrix.last_column = column;
		if(ret)					/* just had a double click */
			mw->matrix.last_click_time = (Time) 0;
		else
			mw->matrix.last_click_time = event->xbutton.time;
		ret = 0;

		lastButton = event->xbutton.button;

		return ret;
	}

	current_time = event->xbutton.time;
	delta = current_time - mw->matrix.last_click_time;

	if(row == mw->matrix.last_row && column == mw->matrix.last_column
	   && delta < (unsigned long) mw->matrix.double_click_interval)
		ret = 1;
	else
		ret = 0;

	if(event->xbutton.button != lastButton)
	{
		ret = 0;
	}
	return ret;
}

 /*ARGSUSED*/ static void PushButton(Widget w, XtPointer data, XEvent * event, Boolean * cont)
{
	XbaeMatrixButtonPressedStruct *button = (XbaeMatrixButtonPressedStruct *) data;
	XMotionEvent *motionEvent;
	int x, y;
	int row, column;
	Boolean pressed = button->pressed;

	DEBUGOUT(_XbaeDebug(__FILE__, w, "PushButton()\n"));

	if(event->type == ButtonRelease)
	{
		button->grabbed = False;
		XtRemoveGrab(w);

		if(button->pressed)
		{
			/* If the button is still pressed, it has been released in the
			   same button that was pressed.  "Unpress" it and call the
			   callbacks */
			if(button->column == -1)
				xbaeDrawRowLabel(button->mw, button->row, False);
			else if(button->row == -1)
				xbaeDrawColumnLabel(button->mw, button->column, False);

			if(button->mw->matrix.label_activate_callback)
			{
				XbaeMatrixLabelActivateCallbackStruct call_data;

				call_data.reason = XbaeLabelActivateReason;
				call_data.event = event;
				call_data.row_label = (button->column == -1);
				call_data.row = button->row;
				call_data.column = button->column;
				call_data.label = NULL;

				/* BugFix: If the xmlabels are used and not labels then the original code
				 * indexed into an unallocated array and broke.
				 * ToDo: return either xmlabel or label as required.
				 */
				if(button->column == -1)
				{
					if(button->mw->matrix.row_labels)
						call_data.label = button->mw->matrix.row_labels[button->row];
				}
				else
				{
					if(button->mw->matrix.column_labels)
						call_data.label = button->mw->matrix.column_labels[button->column];
				}

				XtCallCallbackList((Widget) button->mw,
								   button->mw->matrix.label_activate_callback, (XtPointer) & call_data);
			}
		}
		return;
	}

	if(event->type != MotionNotify)	/* We want to be sure about this! */
		return;

	motionEvent = (XMotionEvent *) event;
	x = motionEvent->x;
	y = motionEvent->y;

	if(!xbaeEventToMatrixXY(button->mw, event, &x, &y))
		return;

	if(xbaeMatrixXYToRowCol(button->mw, &x, &y, &row, &column))
		/* Moved off the labels */
		pressed = False;
	else
	{
		if(button->column != column || button->row != row)
			/* Moved out of the button that was originally pressed */
			pressed = False;
		else if(button->column == column || button->row == row)
			pressed = True;
	}
	/* If the status of whether or not the button should be pressed has
	   changed, redraw the appropriate visual */
	if(pressed != button->pressed)
	{
		if(button->column == -1)
			xbaeDrawRowLabel(button->mw, button->row, pressed);
		else if(button->row == -1)
			xbaeDrawColumnLabel(button->mw, button->column, pressed);
		/* And set our struct's pressed member to the current setting */
		button->pressed = pressed;
	}
}

 /*ARGSUSED*/ void xbaeHandleClick(Widget w, XtPointer data, XEvent * event, Boolean * cont)
{
	XbaeMatrixWidget mw = (XbaeMatrixWidget) data;
	int x, y;
	int row, column;
	Boolean in_rows, in_columns, in_cells, in_labels;

	DEBUGOUT(_XbaeDebug(__FILE__, w, "xbaeHandleClick()\n"));

	/* if we have a double click and a callback - break out! */
	if(event->type != ButtonPress && event->type != ButtonRelease)
		return;

	if(!xbaeEventToMatrixXY(mw, event, &x, &y))
	{
		DEBUGOUT(_XbaeDebug(__FILE__, w, "xbaeEventToXY() fails\n"));
		return;
	}

	in_rows = xbaeMatrixYtoRow(mw, &y, &row);
	in_columns = xbaeMatrixXtoColumn(mw, &x, &column);
	in_cells = in_rows && in_columns && row != -1 && column != -1;
	in_labels = in_rows && in_columns && ((row == -1) ^ (column == -1));

	if(event->type == ButtonPress)
	{
		last_row = row;
		last_column = column;
	}

	if(in_labels
	   && (mw->matrix.button_labels
		   || (row == -1 && mw->matrix.column_button_labels
			   && mw->matrix.column_button_labels[column])
		   || (column == -1 && mw->matrix.row_button_labels && mw->matrix.row_button_labels[row])))
	{
		unsigned long event_mask;
		XtAppContext appcontext;
		XbaeMatrixButtonPressedStruct button;

		/*
		 * If it is ButtonRelease event, return - the ButtonRelease
		 * events are handled in the event handler loop below.
		 */
		if(event->type != ButtonPress)
		{
			DEBUGOUT(_XbaeDebug(__FILE__, w, "xbaeHandleClick - no ButtonPress, return\n"));
			return;
		}

		if(column == -1)
		{
			/* row label */
			DEBUGOUT(_XbaeDebug(__FILE__, w, "Action in row label\n"));
			xbaeDrawRowLabel(mw, row, True);
		}
		else if(row == -1)
		{
			/* Column label */
			DEBUGOUT(_XbaeDebug(__FILE__, w, "Action in column label\n"));
			xbaeDrawColumnLabel(mw, column, True);
		}

		/* Action stations! */
		event_mask = ButtonReleaseMask | PointerMotionMask;

		XtAddGrab(w, True, False);
		/* Copy the data needed to be passed to the event handler */
		button.mw = mw;
		button.row = row;
		button.column = column;
		button.pressed = True;
		button.grabbed = True;

		XtAddEventHandler(w, event_mask, True, (XtEventHandler) PushButton, (XtPointer) & button);
		XtAddEventHandler(TextChild(mw), event_mask, True, (XtEventHandler) PushButton, (XtPointer) & button);

		appcontext = XtWidgetToApplicationContext(w);

		while(button.grabbed)
			XtAppProcessEvent(appcontext, XtIMAll);

		XtRemoveEventHandler(w, event_mask, True, (XtEventHandler) PushButton, (XtPointer) & button);
		XtRemoveEventHandler(TextChild(mw), event_mask, True, (XtEventHandler) PushButton,
							 (XtPointer) & button);

	}

	if(in_cells && mw->matrix.default_action_callback && w != (Widget) mw
	   && DoubleClick(mw, event, mw->matrix.current_row, mw->matrix.current_column))
	{
		XbaeMatrixDefaultActionCallbackStruct call_data;

		DEBUGOUT(_XbaeDebug(__FILE__, w, "xbaeHandleClick - DefaultActionCallback\n"));

		call_data.reason = XbaeDefaultActionReason;
		call_data.event = event;
		call_data.row = row;
		call_data.column = column;

		XtCallCallbackList((Widget) mw, mw->matrix.default_action_callback, (XtPointer) & call_data);
	}
	else
	{
		DEBUGOUT(_XbaeDebug
				 (__FILE__, w,
				  "xbaeHandleClick() uncaught case (x %d y %d, row %d col %d)\n", x, y, row, column));
	}
}

/* ARGSUSED */
/*
 * Dean Phillips needs xbaeTraversePrev/NextACT because ManagerGadgetNext/PrevTabGroup()
 * cause NutCracker's Motif implementation to crash.
 */
void xbaeTraverseNextACT(Widget w, XEvent * event, String * params, Cardinal * nparams)
{
	XbaeMatrixWidget mw;

	/*
	 * Get Matrix widget and make sure it is a Matrix subclass.
	 * w should be the textField widget.
	 */
	if(XtIsSubclass(XtParent(w), xbaeMatrixWidgetClass))
		mw = (XbaeMatrixWidget) XtParent(w);
	else
	{
		XtAppWarningMsg(XtWidgetToApplicationContext(w), "traverseNextACT", "badWidget",
						"XbaeMatrix", "XbaeMatrix: Bad widget passed to TraverseNext action", NULL, 0);
		return;
	}

	XmProcessTraversal(TextChild(mw), XmTRAVERSE_NEXT_TAB_GROUP);
}

/* ARGSUSED */
void xbaeTraversePrevACT(Widget w, XEvent * event, String * params, Cardinal * nparams)
{
	XbaeMatrixWidget mw;

	/*
	 * Get Matrix widget and make sure it is a Matrix subclass.
	 * w should be the textField widget.
	 */
	if(XtIsSubclass(XtParent(w), xbaeMatrixWidgetClass))
		mw = (XbaeMatrixWidget) XtParent(w);
	else
	{
		XtAppWarningMsg(XtWidgetToApplicationContext(w), "traversePrevACT", "badWidget",
						"XbaeMatrix", "XbaeMatrix: Bad widget passed to TraversePrev action", NULL, 0);
		return;
	}

	XmProcessTraversal(TextChild(mw), XmTRAVERSE_PREV_TAB_GROUP);
}

static void
callSelectCellCallbacks(XbaeMatrixWidget mw, XEvent * event, int row, int column, String * params,
						Cardinal num_params)
{
	XbaeMatrixSelectCellCallbackStruct call_data;

	call_data.reason = XbaeSelectCellReason;
	call_data.event = event;
	call_data.row = row;
	call_data.column = column;
	call_data.num_params = num_params;
	call_data.params = params;
	call_data.cells = NULL;		/* mw->matrix.cells */
	call_data.selected_cells = NULL;	/* mw->matrix.selected_cells */
	/*
	 * Need to create a new structure and pass this to the callback.
	 * Before the PER_CELL change, this was an easy action because
	 * we could pass an existing structure. Now we need to build
	 * it especially for this callback.
	 */

	XtCallCallbackList((Widget) mw, mw->matrix.select_cell_callback, (XtPointer) & call_data);

}

/* ARGSUSED */
void xbaeSelectCellACT(Widget w, XEvent * event, String * params, Cardinal * num_params)
{
	XbaeMatrixWidget mw;
	int x, y;
	int row, column;

	/*
	 * Get Matrix widget and make sure it is a Matrix subclass.
	 * w could be Matrix, or the Clip or textField children of Matrix
	 */
	if(XtIsSubclass(w, xbaeMatrixWidgetClass))
		mw = (XbaeMatrixWidget) w;
	else if(XtIsSubclass(XtParent(w), xbaeMatrixWidgetClass))
		mw = (XbaeMatrixWidget) XtParent(w);
	else
	{
		XtAppWarningMsg(XtWidgetToApplicationContext(w), "xbaeSelectCellACT", "badWidget",
						"XbaeMatrix", "XbaeMatrix: Bad widget passed to SelectCell action", NULL, 0);
		return;
	}

	/*
	 * If we don't have a selectCellCallback, then return now
	 */
	if(!mw->matrix.select_cell_callback)
		return;

	if(!xbaeEventToMatrixXY(mw, event, &x, &y))
	{
		row = -1;
		column = -1;
	}
	else
	{
		xbaeMatrixXYToRowCol(mw, &x, &y, &row, &column);
	}

	/*
	 * Call our select_cell callbacks
	 */
	callSelectCellCallbacks(mw, event, row, column, params, *num_params);
}

 /*ARGSUSED*/ static void scrollSelect(XbaeMatrixScrollStruct * ss, int new_row, int new_column)
{
	if((new_row != last_row || new_column != last_column)
	   && (ss->mw->matrix.selection_policy == XmMULTIPLE_SELECT
		   || ss->mw->matrix.selection_policy == XmEXTENDED_SELECT))
	{
		Boolean old_scroll_select = ss->mw->matrix.scroll_select;
		String params[1] = { "extend" };
		ss->mw->matrix.scroll_select = False;

		callSelectCellCallbacks(ss->mw, ss->event, new_row, new_column, params,
								sizeof params / sizeof *params);

		ss->mw->matrix.scroll_select = old_scroll_select;

		last_row = new_row;
		last_column = new_column;
	}
}

static void checkScrollValues(Widget w, XtPointer data, XEvent * event, Boolean * cont)
{
	XbaeMatrixScrollStruct *ss = (XbaeMatrixScrollStruct *) data;
	Boolean in_rows, in_columns;
	int x, y;
	int row, column;

	if(event->type == ButtonRelease)
	{
		XtRemoveGrab(w);
		ss->grabbed = False;
	}
	else
	{
		if(!xbaeEventToMatrixXY(ss->mw, event, &x, &y))
			return;

		ss->event = event;
		ss->currentx = x;
		ss->currenty = y;

		/* figure out the next row/column in which to select */

		in_rows = xbaeMatrixYtoRow(ss->mw, &y, &row);
		if(!in_rows || !(xbaeRowClip(ss->mw, row) & ss->scroll_region))
		{
			row = last_row;
		}

		in_columns = xbaeMatrixXtoColumn(ss->mw, &x, &column);
		if(!in_columns || !(xbaeColumnClip(ss->mw, column) & ss->scroll_region))
		{
			column = last_column;
		}

		scrollSelect(ss, row, column);
	}
}

static void updateScroll(XtPointer data)
{
	XbaeMatrixScrollStruct *ss = (XbaeMatrixScrollStruct *) data;
	int row = last_row;
	int column = last_column;
	int dist;

	if(!ss->grabbed)
		return;

	/*
	 * If we are off the clip widget and there is somthing that could
	 * be scrolled into a visible position. Scroll them into view.  
	 */

	/*Scroll now with aspect ratio (step 4 for vertical, step 3 for
	   horizontal, for same speed on monitors with 4:3 aspect ratio) */

	if(ss->scroll_region & CLIP_VISIBLE_HEIGHT)
	{
		if(ss->currenty < NON_FIXED_ROW_POSITION(ss->mw) && VERT_ORIGIN(ss->mw) > 0)
		{
			/* above the non fixed cells */
			dist = NON_FIXED_ROW_POSITION(ss->mw) - ss->currenty;
			xbaeScrollRows(ss->mw, True, Min(dist, VERT_ORIGIN(ss->mw)));
			row = xbaeTopRow(ss->mw);
		}
		else if(ss->currenty >= NON_FIXED_ROW_POSITION(ss->mw) + VISIBLE_NON_FIXED_HEIGHT(ss->mw)
				&& VERT_ORIGIN(ss->mw) + VISIBLE_NON_FIXED_HEIGHT(ss->mw) < NON_FIXED_HEIGHT(ss->mw))
		{
			/* bellow the non fixed cells */
			dist = ss->currenty - (NON_FIXED_ROW_POSITION(ss->mw) + VISIBLE_NON_FIXED_HEIGHT(ss->mw) - 1);
			xbaeScrollRows(ss->mw, False,
						   Min(dist,
							   NON_FIXED_HEIGHT(ss->mw) - VERT_ORIGIN(ss->mw) -
							   VISIBLE_NON_FIXED_HEIGHT(ss->mw)));
			row = xbaeBottomRow(ss->mw);
		}
	}
	if(ss->scroll_region & CLIP_VISIBLE_WIDTH)
	{
		if(ss->currentx < NON_FIXED_COLUMN_POSITION(ss->mw) && HORIZ_ORIGIN(ss->mw) > 0)
		{
			dist = NON_FIXED_COLUMN_POSITION(ss->mw) - ss->currentx;
			xbaeScrollColumns(ss->mw, True, Min(dist, HORIZ_ORIGIN(ss->mw)));
			column = xbaeLeftColumn(ss->mw);
		}
		else if(ss->currentx >= NON_FIXED_COLUMN_POSITION(ss->mw) + VISIBLE_NON_FIXED_WIDTH(ss->mw)
				&& HORIZ_ORIGIN(ss->mw) + VISIBLE_NON_FIXED_WIDTH(ss->mw) < NON_FIXED_WIDTH(ss->mw))
		{
			dist = ss->currentx - (NON_FIXED_COLUMN_POSITION(ss->mw) + VISIBLE_NON_FIXED_WIDTH(ss->mw) - 1);
			xbaeScrollColumns(ss->mw, False,
							  Min(dist,
								  NON_FIXED_WIDTH(ss->mw) - HORIZ_ORIGIN(ss->mw) -
								  VISIBLE_NON_FIXED_WIDTH(ss->mw)));
			column = xbaeRightColumn(ss->mw);
		}
	}

	scrollSelect(ss, row, column);

	/*
	 * Flush the updates out to the server so we don't end up lagging
	 * behind too far and end up with a million redraw requests.
	 * Particularly for higher update speeds
	 */

	XFlush(XtDisplay((Widget) ss->mw));

	ss->timerID =
		XtAppAddTimeOut(ss->app_context, DEFAULT_SCROLL_SPEED, (XtTimerCallbackProc) updateScroll,
						(XtPointer) ss);
}

/* ARGSUSED */
void xbaeHandleMotionACT(Widget w, XEvent * event, String * params, Cardinal * nparams)
{
	XbaeMatrixWidget mw;
	int x, y, row, column;
	XbaeMatrixScrollStruct scrollData;
	static Boolean scrolling = False;

	if(scrolling)
		return;

	/*
	 * Get Matrix widget and make sure it is a Matrix subclass.
	 * w could be Matrix, or the Clip or textField children of Matrix
	 */
	if(XtIsSubclass(w, xbaeMatrixWidgetClass))
		mw = (XbaeMatrixWidget) w;
	else if(XtIsSubclass(XtParent(w), xbaeMatrixWidgetClass))
		mw = (XbaeMatrixWidget) XtParent(w);
	else
	{
		XtAppWarningMsg(XtWidgetToApplicationContext(w), "handleMotionACT", "badWidget",
						"XbaeMatrix", "XbaeMatrix: Bad widget passed to HandleMotion action", NULL, 0);
		return;
	}

	if(!xbaeEventToMatrixXY(mw, event, &x, &y))
		return;

	/*
	 * In this instance, we don't care if a valid row and column are
	 * returned as we'll be the judge of the result
	 */
	xbaeMatrixXYToRowCol(mw, &x, &y, &row, &column);

	/*
	 * Grab the pointer and add a timeout routine to start modifying
	 * the current row and/or column in the matrix.  Also add an
	 * event handler to monitor the current distance outside the
	 * matrix so we can adjust the timeout routine to go faster when
	 * the pointer is further away from the matrix.
	 */

	scrolling = True;

	XtAddGrab(w, True, False);

	scrollData.mw = mw;
	scrollData.event = event;
	scrollData.grabbed = True;
	scrollData.app_context = XtWidgetToApplicationContext(w);
	scrollData.scroll_region = xbaeCellClip(mw, last_row, last_column);

	XtAddEventHandler(w, PointerMotionMask | ButtonReleaseMask, True,
					  (XtEventHandler) checkScrollValues, (XtPointer) & scrollData);
	/*
	 * Call checkScrollValues() to find out where exactly we are in
	 * relation to the clip widget
	 */
	checkScrollValues(w, (XtPointer) & scrollData, event, NULL);

	/*
	 * The above / below / left / right members of the scrollData struct
	 * should now be set so we know where we should be moving.  Let's
	 * get on with it, eh?
	 */
	updateScroll((XtPointer) & scrollData);

	while(scrollData.grabbed)
		XtAppProcessEvent(scrollData.app_context, XtIMAll);

	XtRemoveEventHandler(w, PointerMotionMask | ButtonReleaseMask, True,
						 (XtEventHandler) checkScrollValues, (XtPointer) & scrollData);

	/*
	 * We don't want the timeout getting called again as, in two lines,
	 * we'll be way out of scope!
	 */
	XtRemoveTimeOut(scrollData.timerID);
	scrolling = False;
}

/* ARGSUSED */
void xbaePageDownACT(Widget w, XEvent * event, String * params, Cardinal * nparams)
{
	XbaeMatrixWidget mw;
	char *down = "0";
	int vert_origin;

	char *param = "PageDown";
	static XrmQuark qparam = NULLQUARK;
	/*
	 * Get a static quarks for our parm 
	 */
	if(qparam == NULLQUARK)
	{
		qparam = XrmPermStringToQuark(param);
	}

	/*
	 * Get Matrix widget and make sure it is a Matrix subclass.
	 * w should be the textField widget.
	 */
	if(XtIsSubclass(XtParent(w), xbaeMatrixWidgetClass))
		mw = (XbaeMatrixWidget) XtParent(w);
	else
	{
		XtAppWarningMsg(XtWidgetToApplicationContext(w), "pageDownACT", "badWidget",
						"XbaeMatrix", "XbaeMatrix: Bad widget passed to PageDown action", NULL, 0);
		return;
	}

	if(!XtIsManaged(VertScrollChild(mw)))
		return;

	/*
	 * Save the vert_origin - if scrolling occurs, the text widget needs
	 * to be moved
	 */
	vert_origin = VERT_ORIGIN(mw);

	XtCallActionProc(VertScrollChild(mw), "PageDownOrRight", event, &down, 1);

	if(VERT_ORIGIN(mw) != vert_origin)
	{
		/*
		 * Position the cursor at the top most non fixed row if there was
		 * a page down
		 */
		int row = xbaeTopRow(mw);
		int column = mw->matrix.current_column < 0 ? xbaeLeftColumn(mw) : mw->matrix.current_column;

		if(mw->matrix.traverse_cell_callback)
		{
			callTraverseCellCallbacks(mw, event, &row, &column, param, qparam);
		}

		((XbaeMatrixWidgetClass) XtClass(mw))->matrix_class.edit_cell(mw, event,
																	  row, column, params, *nparams);
	}
}

/* ARGSUSED */
void xbaePageUpACT(Widget w, XEvent * event, String * params, Cardinal * nparams)
{
	XbaeMatrixWidget mw;
	char *up = "0";
	int vert_origin;

	char *param = "PageUp";
	static XrmQuark qparam = NULLQUARK;
	/*
	 * Get a static quarks for our parm 
	 */
	if(qparam == NULLQUARK)
	{
		qparam = XrmPermStringToQuark(param);
	}

	/*
	 * Get Matrix widget and make sure it is a Matrix subclass.
	 * w should be the textField widget.
	 */
	if(XtIsSubclass(XtParent(w), xbaeMatrixWidgetClass))
		mw = (XbaeMatrixWidget) XtParent(w);
	else
	{
		XtAppWarningMsg(XtWidgetToApplicationContext(w), "pageUpACT", "badWidget",
						"XbaeMatrix", "XbaeMatrix: Bad widget passed to PageUp action", NULL, 0);
		return;
	}

	if(!XtIsManaged(VertScrollChild(mw)))
		return;

	/*
	 * Save the vert_origin - if scrolling occurs, the text widget needs
	 * to be moved
	 */
	vert_origin = VERT_ORIGIN(mw);

	XtCallActionProc(VertScrollChild(mw), "PageUpOrLeft", event, &up, 1);

	if(VERT_ORIGIN(mw) != vert_origin)
	{
		/*
		 * Position the cursor at the top most non fixed row if there was
		 * a page up
		 */
		int row = xbaeTopRow(mw);
		int column = mw->matrix.current_column < 0 ? xbaeLeftColumn(mw) : mw->matrix.current_column;

		if(mw->matrix.traverse_cell_callback)
		{
			callTraverseCellCallbacks(mw, event, &row, &column, param, qparam);
		}

		((XbaeMatrixWidgetClass) XtClass(mw))->matrix_class.edit_cell(mw, event,
																	  row, column, params, *nparams);
	}
}

void xbaeScrollRowsACT(Widget w, XEvent * event, String * params, Cardinal * nparams)
{
	XbaeMatrixWidget mw;
	int step;
	String end;
	/*
	 * Get Matrix widget and make sure it is a Matrix subclass.
	 * w should be the textField widget.
	 */
	if(XtIsSubclass(w, xbaeMatrixWidgetClass))
		mw = (XbaeMatrixWidget) w;
	else if(XtIsSubclass(XtParent(w), xbaeMatrixWidgetClass))
		mw = (XbaeMatrixWidget) XtParent(w);
	else
	{
		XtAppWarningMsg(XtWidgetToApplicationContext(w), "scrollRowsAct", "badWidget",
						"XbaeMatrix", "XbaeMatrix: Bad widget passed to ScrollRows action", NULL, 0);
		return;
	}

	if(*nparams == 1)
	{
		step = strtol(params[0], &end, 0);
	}

	if(*nparams != 1 || end == params[0])
	{
		XtAppWarningMsg(XtWidgetToApplicationContext(w), "scrollRowsAct", "badWidget",
						"XbaeMatrix", "XbaeMatrix: Bad parameter passed to ScrollRows action", NULL, 0);
		return;
	}

	xbaeScrollRows(mw, step < 0, step < 0 ? -step : step);
}

void xbaeScrollColumnsACT(Widget w, XEvent * event, String * params, Cardinal * nparams)
{
	XbaeMatrixWidget mw;
	int step;
	String end;
	/*
	 * Get Matrix widget and make sure it is a Matrix subclass.
	 * w should be the textField widget.
	 */
	if(XtIsSubclass(w, xbaeMatrixWidgetClass))
		mw = (XbaeMatrixWidget) w;
	else if(XtIsSubclass(XtParent(w), xbaeMatrixWidgetClass))
		mw = (XbaeMatrixWidget) XtParent(w);
	else
	{
		XtAppWarningMsg(XtWidgetToApplicationContext(w), "scrollColumnsAct", "badWidget",
						"XbaeMatrix", "XbaeMatrix: Bad widget passed to ScrollColumns action", NULL, 0);
		return;
	}

	if(*nparams == 1)
	{
		step = strtol(params[0], &end, 0);
	}

	if(*nparams != 1 || end == params[0])
	{
		XtAppWarningMsg(XtWidgetToApplicationContext(w), "scrollColumnssAct", "badWidget",
						"XbaeMatrix", "XbaeMatrix: Bad parameter passed to ScrollColumns action", NULL, 0);
		return;
	}

	xbaeScrollColumns(mw, step < 0, step < 0 ? -step : step);
}
