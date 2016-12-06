/*
 * Copyright(c) 1992 Bell Communications Research, Inc. (Bellcore)
 * Copyright (c) 1999-2002 by the LessTif Developers.
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
 * $Id: ClipP.h,v 1.9 2005/03/09 04:05:58 tobiasoed Exp $
 */

/*
 * ClipP.h - Private definitions for Clip widget
 */

#ifndef _Xbae_ClipP_h
#  define _Xbae_ClipP_h

#  include <Xm/PrimitiveP.h>
#  include <Xbae/Clip.h>

#  ifdef __cplusplus
extern "C" {
#  endif

/*
 * New data structures for the ScrollMgr code
 */
	typedef struct _SmScrollNode {
		int x;
		int y;
		struct _SmScrollNode *next;
		struct _SmScrollNode *prev;
	} SmScrollNodeRec, *SmScrollNode;

	typedef struct _SmScrollMgr {
		int offset_x;
		int offset_y;
		int scroll_count;
		SmScrollNode scroll_queue;
		Boolean scrolling;
	} SmScrollMgrRec, *SmScrollMgr;

/*
 * New type for class method
 */

	typedef void (*XbaeClipRedrawProc) (Widget);

/*
 * New fields for the Clip widget class record
 */
	typedef struct {
		XbaeClipRedrawProc redraw;
		XtPointer extension;
	} XbaeClipClassPart;

/*
 * Full class record declaration
 */
	typedef struct _XbaeClipClassRec {
		CoreClassPart core_class;
		XmPrimitiveClassPart primitive_class;
		XbaeClipClassPart clip_class;
	} XbaeClipClassRec;

	extern XbaeClipClassRec xbaeClipClassRec;

/*
 * New inheritance constant
 */
#  define XbaeInheritRedraw ((XbaeClipRedrawProc) _XtInherit)

/*
 * New fields for the Clip widget record
 */
	typedef struct {
		/* resources */
		XbaeClipExposeProc expose_proc;	/* function to call on expose */
		/* private state */
		SmScrollMgrRec scroll;
	} XbaeClipPart;

/*
 * Full instance record declaration
 */
	typedef struct _XbaeClipRec {
		CorePart core;
		XmPrimitivePart primitive;
		XbaeClipPart clip;
	} XbaeClipRec;

#  ifdef __cplusplus
}
#  endif
#endif							/* _Xbae_ClipP_h */
