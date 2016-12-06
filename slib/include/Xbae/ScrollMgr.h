/*
 * Copyright(c) 1992 Bell Communications Research, Inc. (Bellcore)
 * Copyright(c) 1995-99 Andrew Lister
 * Copyright (c) 1999-2002, 2004 by the LessTif Developers.
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
 * $Id: ScrollMgr.h,v 1.16 2005/04/03 16:41:38 tobiasoed Exp $
 */

/*
 * ScrollMgr.h created by Andrew Lister (6 August, 1995)
 */

#ifndef _Xbae_ScrollMgr_h
#  define _Xbae_ScrollMgr_h

#  include <Xbae/Macros.h>

#  ifdef __cplusplus
extern "C" {
#  endif

/*
 * Scrollbar callbacks
 */
	void xbaeScrollVertCB(Widget, XtPointer, XmScrollBarCallbackStruct *);
	void xbaeScrollHorizCB(Widget, XtPointer, XmScrollBarCallbackStruct *);

	void xbaeRedrawRegion(XbaeMatrixWidget mw, XRectangle * expose, XRectangle * region);
	void xbaeRedrawLabelsAndFixed(XbaeMatrixWidget mw, XRectangle * expose);
	void xbaeRedrawAll(XbaeMatrixWidget mw, int rs, int cs, int re, int ce);
	void xbaeRefresh(XbaeMatrixWidget mw, Boolean relayout);

#  ifdef __cplusplus
}
#  endif
#endif							/* _Xbae_ScrollMgr_h */
