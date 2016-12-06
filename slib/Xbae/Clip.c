/*
 * Copyright(c) 1992 Bell Communications Research, Inc. (Bellcore)
 * Copyright © 1999, 2000, 2001, 2002, 2004 by the LessTif Developers.
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
 * ClipWidget Author: Andrew Wason, Bellcore, aw@bae.bellcore.com
 *
 * $Id: Clip.c,v 1.22 2005/04/15 08:23:17 tobiasoed Exp $
 */

/*
 * Clip.c - private child of Matrix - used to clip Matrix's scrollable areas
 */

#include <X11/StringDefs.h>

#include <Xm/Xm.h>

#include <Xbae/ClipP.h>
#include <Xbae/Clip.h>

#include "XbaeDebug.h"

/*
 * ScrollMgr implementation.
 * When we scroll using XCopyArea, occluding windows will cause GraphicsExpose
 * events to be generated, if there are no occluding windows then NoExpose
 * events will be generated. The removal of occluding windows will cause Expose
 * events.  If a number of scrolls (XCopyAreas) occur in quick succession,
 * the events will contain obsolete x/y information since our internal
 * coordinates have been scrolled to a new location.  The ScrollMgr
 * keeps track of scrolls and offsets required to relocate the events to the
 * current coordinate system.
 *
 * Each widgets compress_exposures field should be XtExposeCompressSeries
 * or XtExposeNoCompress.
 *
 * The idea behind this code is based on the PanHandler posted by Chuck Ocheret
 * (chuck@fid.morgan.com)
 */

/*
 * Initialize a ScrollMgr
 */
static void xbaeSmInitScrollMgr(SmScrollMgr scrollMgr)
{
	scrollMgr->offset_x = 0;
	scrollMgr->offset_y = 0;
	scrollMgr->scroll_count = 0;
	scrollMgr->scroll_queue = NULL;
	scrollMgr->scrolling = False;
}

/*
 * Delete all queued scrolls of a scrollMgr
 */
static void xbaeSmFlushScrollMgr(SmScrollMgr scrollMgr)
{
	if(scrollMgr->scroll_queue)
	{
		SmScrollNode node = scrollMgr->scroll_queue->next;

		while(node != scrollMgr->scroll_queue)
		{
			SmScrollNode d = node;

			node = node->next;
			XtFree((char *) d);
		}
		XtFree((char *) node);

		xbaeSmInitScrollMgr(scrollMgr);
	}
}

/*
 * Remove a scroll from the ScrollMgr queue
 */
static void xbaeSmRemoveScroll(SmScrollMgr scrollMgr)
{
	if(scrollMgr->scroll_count)
	{
		SmScrollNode node = scrollMgr->scroll_queue;

		scrollMgr->offset_x -= node->x;
		scrollMgr->offset_y -= node->y;

		/*
		 * Remove node from head of queue
		 */
		if(node->next == node)
		{
			scrollMgr->scroll_queue = NULL;
		}
		else
		{
			scrollMgr->scroll_queue = node->next;
			node->next->prev = node->prev;
			node->prev->next = node->next;
		}
		XtFree((char *) node);

		scrollMgr->scroll_count--;
	}
}

/*
 * Handle an expose event
 */
static Boolean xbaeSmScrollEvent(SmScrollMgr scrollMgr, XEvent * event, XRectangle * expose)
{
	switch (event->type)
	{

		case Expose:
			/*
			 * Normal Expose event
			 */
			DEBUGOUT(_XbaeDebug(__FILE__, NULL, "xbaeSmScrollEvent: Expose\n"));

			expose->x = event->xexpose.x;
			expose->y = event->xexpose.y;
			expose->width = event->xexpose.width;
			expose->height = event->xexpose.height;

			break;

		case GraphicsExpose:
			DEBUGOUT(_XbaeDebug(__FILE__, NULL, "xbaeSmScrollEvent: GraphicsExpose\n"));

			if(scrollMgr->scrolling == False)
			{
				/*
				 * This is the first GraphicsExpose event. 
				 * Remove the corresponding scroll from the queue and 
				 * set scrolling to True
				 */
				xbaeSmRemoveScroll(scrollMgr);
				scrollMgr->scrolling = True;
			}

			if(event->xgraphicsexpose.count == 0)
			{
				/*
				 * This is the last GraphicsExpose. 
				 * Set scrolling to False.
				 */
				scrollMgr->scrolling = False;
			}

			expose->x = event->xgraphicsexpose.x;
			expose->y = event->xgraphicsexpose.y;
			expose->width = event->xgraphicsexpose.width;
			expose->height = event->xgraphicsexpose.height;

			break;

		case NoExpose:

			/*
			 * A NoExpose event means we won't be getting any GraphicsExpose
			 * events, so remove the scroll from the queue and set scrolling
			 * to False.
			 */
			xbaeSmRemoveScroll(scrollMgr);
			scrollMgr->scrolling = False;

			return False;

		default:
			return False;
	}

	/*
	 * Translate the event into our scrolled coordinate system.
	 */

	expose->x += scrollMgr->offset_x;
	expose->y += scrollMgr->offset_y;

	return True;
}

/*
 * Record a new scroll request in the ScrollMgr
 */
static void xbaeSmAddScroll(SmScrollMgr scrollMgr, int delta_x, int delta_y)
{
	SmScrollNode node = XtNew(SmScrollNodeRec);

	node->x = delta_x;
	node->y = delta_y;

	scrollMgr->offset_x += delta_x;
	scrollMgr->offset_y += delta_y;
	scrollMgr->scroll_count++;

	/*
	 * Insert the node at the end of the queue
	 */
	if(!scrollMgr->scroll_queue)
	{
		scrollMgr->scroll_queue = node;
		node->next = node;
		node->prev = node;
	}
	else
	{
		SmScrollNode last = scrollMgr->scroll_queue->prev;

		last->next = node;
		node->next = scrollMgr->scroll_queue;
		node->prev = last;
		scrollMgr->scroll_queue->prev = node;
	}
}

void XbaeClipScrollVert(Widget w, GC gc, int delta)
{

	XbaeClipWidget cw;

	int src_y, dest_y, copy_height;
	int y_clear, clear_height;

	/*
	 * Make sure w is a Clip or a subclass
	 */
	XtCheckSubclass(w, xbaeClipWidgetClass, NULL);

	cw = (XbaeClipWidget) w;

	if(delta == 0)
	{
		/* Didn't scroll */
		return;
	}
	else if(delta < 0)
	{
		/* scrolled down */
		dest_y = 0;
		src_y = -delta;
		copy_height = cw->core.height - src_y;
		y_clear = copy_height;
		clear_height = cw->core.height - copy_height;
	}
	else
	{
		/* scrolled up */
		dest_y = delta;
		src_y = 0;
		copy_height = cw->core.height - dest_y;
		y_clear = 0;
		clear_height = cw->core.height - copy_height;
	}

	if(copy_height <= 0)
	{
		/* nothing to copy, clear and redraw everything */
		y_clear = 0;
		clear_height = cw->core.height;
	}
	else
	{
		/* 
		 * There is something that can be copied
		 * Queue this scroll with the ScrollMgr and copy contents of the clip
		 */
		xbaeSmAddScroll(&cw->clip.scroll, 0, dest_y - src_y);
		XCopyArea(XtDisplay(cw), XtWindow(cw), XtWindow(cw), gc,
				  0, src_y, cw->core.width, copy_height, 0, dest_y);
	}

	if(cw->clip.expose_proc)
	{
		XRectangle expose;
		expose.x = 0;
		expose.y = (short) y_clear;
		expose.width = (unsigned short) cw->core.width;
		expose.height = (unsigned short) clear_height;
		cw->clip.expose_proc(w, &expose, NULL, NULL);
	}
}

void XbaeClipScrollHoriz(Widget w, GC gc, int delta)
{

	XbaeClipWidget cw;

	int src_x, dest_x, copy_width;
	int x_clear, clear_width;

	/*
	 * Make sure w is a Clip or a subclass
	 */
	XtCheckSubclass(w, xbaeClipWidgetClass, NULL);

	cw = (XbaeClipWidget) w;

	if(delta == 0)
		/* Didn't scroll */
		return;
	else if(delta < 0)
	{
		/* Scrolled right */
		dest_x = 0;
		src_x = -delta;
		copy_width = cw->core.width - src_x;
		x_clear = copy_width;
		clear_width = cw->core.width - copy_width;
	}
	else
	{
		/* Scrolled left */
		dest_x = delta;
		src_x = 0;
		copy_width = cw->core.width - dest_x;
		x_clear = 0;
		clear_width = cw->core.width - copy_width;
	}

	if(copy_width <= 0)
	{
		/* nothing to copy, clear and redraw everything */
		x_clear = 0;
		clear_width = cw->core.width;
	}
	else
	{
		/* 
		 * There is something that can be copied
		 * Queue this scroll with the ScrollMgr and copy contents of the clip
		 */
		xbaeSmAddScroll(&cw->clip.scroll, dest_x - src_x, 0);
		XCopyArea(XtDisplay(cw), XtWindow(cw), XtWindow(cw), gc,
				  src_x, 0, copy_width, cw->core.height, dest_x, 0);
	}

	if(cw->clip.expose_proc)
	{
		XRectangle expose;
		expose.x = (short) x_clear;
		expose.y = 0;
		expose.width = (unsigned short) clear_width;
		expose.height = (unsigned short) cw->core.height;
		cw->clip.expose_proc(w, &expose, NULL, NULL);
	}
}

#ifdef	WIN32
#  define	EXTERNALREF	externalref __declspec(dllexport)
#else
#  define	EXTERNALREF			/* nothing */
#endif

static XtResource resources[] = {
	{XmNexposeProc, XmCFunction, XtRFunction, sizeof(XbaeClipExposeProc),
	 XtOffsetOf(XbaeClipRec, clip.expose_proc),
	 XtRFunction, (XtPointer) NULL}
	,
};

/*
 * Declaration of methods
 */
static void ClassPartInitialize(WidgetClass);
static void Initialize(Widget, Widget, ArgList, Cardinal *);
static void Destroy(Widget);
static void Realize(Widget, XtValueMask *, XSetWindowAttributes *);
static void Redisplay(Widget, XEvent *, Region);
static void Resize(Widget);
static void Redraw(Widget);

/*
 * Public convenience function
 */
void XbaeClipRedraw(Widget w);

XbaeClipClassRec xbaeClipClassRec = {
	/* core_class fields */
	{
	 /* superclass            */ (WidgetClass) & xmPrimitiveClassRec,
	 /* class_name            */ "XbaeClip",
	 /* widget_size           */ sizeof(XbaeClipRec),
	 /* class_initialize      */ NULL,
	 /* class_part_initialize */ ClassPartInitialize,
	 /* class_inited          */ False,
	 /* initialize            */ Initialize,
	 /* initialize_hook       */ NULL,
	 /* realize               */ Realize,
	 /* actions               */ NULL,
	 /* num_actions           */ 0,
	 /* resources             */ resources,
	 /* num_resources         */ XtNumber(resources),
	 /* xrm_class             */ NULLQUARK,
	 /* compress_motion       */ True,
	 /* compress_exposure     */ XtExposeCompressSeries |
	 XtExposeGraphicsExpose | XtExposeNoExpose,
	 /* compress_enterleave   */ True,
	 /* visible_interest      */ False,
	 /* destroy               */ Destroy,
	 /* resize                */ Resize,
	 /* expose                */ Redisplay,
	 /* set_values            */ NULL,
	 /* set_values_hook       */ NULL,
	 /* set_values_almost     */ XtInheritSetValuesAlmost,
	 /* get_values_hook       */ NULL,
	 /* accept_focus          */ NULL,
	 /* version               */ XtVersion,
	 /* callback_private      */ NULL,
	 /* tm_table              */ NULL,
	 /* query_geometry        */ NULL,
	 /* display_accelerator   */ NULL,
	 /* extension             */ NULL
	 }
	,
	/* primitive_class fields */
	{
	 /* border_highlight      */ NULL,
	 /* border_unhighlight    */ NULL,
	 /* translations          */ NULL,
	 /* arm_and_activate      */ NULL,
	 /* syn_resources         */ NULL,
	 /* num_syn_resources     */ 0,
	 /* extension             */ NULL
	 }
	,
	/* clip_class fields */
	{
	 /* redraw                */ Redraw,
	 /* extension             */ NULL,
	 }
};

EXTERNALREF WidgetClass xbaeClipWidgetClass = (WidgetClass) & xbaeClipClassRec;

static void ClassPartInitialize(WidgetClass wc)
{
	XbaeClipWidgetClass cwc = (XbaeClipWidgetClass) wc;
	XbaeClipWidgetClass super = (XbaeClipWidgetClass) cwc->core_class.superclass;

	/*
	 * Allow subclasses to inherit our redraw method
	 */
	if(cwc->clip_class.redraw == XbaeInheritRedraw)
		cwc->clip_class.redraw = super->clip_class.redraw;
}


static void Initialize(Widget rw, Widget nw, ArgList args, Cardinal * num_args)
{
	XbaeClipWidget cw = (XbaeClipWidget) nw;

	xbaeSmInitScrollMgr(&cw->clip.scroll);
}

static void Destroy(Widget w)
{
	XbaeClipWidget cw = (XbaeClipWidget) w;

	xbaeSmFlushScrollMgr(&cw->clip.scroll);
}


static void Realize(Widget w, XtValueMask * valueMask, XSetWindowAttributes * attributes)
{
	/*
	 * Don't call our superclasses realize method, because Primitive sets
	 * bit_gravity and do_not_propagate
	 */
	XtCreateWindow(w, InputOutput, CopyFromParent, *valueMask, attributes);
}

/* ARGSUSED */
static void Redisplay(Widget w, XEvent * event, Region region)
{
	XbaeClipWidget cw = (XbaeClipWidget) w;
	XRectangle expose;

	/*
	 * Notify the scroll manager that we got an event and get an
	 * expose rectangle in return
	 */
	if(!xbaeSmScrollEvent(&cw->clip.scroll, event, &expose))
		return;

	if(cw->clip.expose_proc)
		cw->clip.expose_proc((Widget) cw, &expose, event, region);
}

/* ARGSUSED */
static void Resize(Widget w)
{
	/*
	 * Xt will call the expose method when this method returns.
	 * So we won't have to do any exposure stuff here, which 
	 * means the Matrix's  SetValues method only needs to force a
	 * redraw when a redisplay is needed, not when a relayout is performed.
	 */
}

/*
 * Clip redraw method
 */

/* ARGSUSED */
static void Redraw(Widget w)
{
	if(XtIsRealized(w))
		XClearArea(XtDisplay(w), XtWindow(w), 0, 0, 0, 0, True);
}

/*
 * Public interface to redraw method
 */
void XbaeClipRedraw(Widget w)
{
	/*
	 * Make sure w is a Clip or a subclass
	 */
	XtCheckSubclass(w, xbaeClipWidgetClass, NULL);

	/*
	 * Call the redraw method
	 */
	if(XtIsRealized(w))
		((XbaeClipWidgetClass) XtClass(w))->clip_class.redraw(w);
}
