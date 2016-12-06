/*
 * Copyright(c) 1992 Bell Communications Research, Inc. (Bellcore)
 * Copyright(c) 1995-99 Andrew Lister
 * Copyright © 1999-2002, 2004, 2005 by the LessTif Developers.
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
 * $Id: Actions.h,v 1.11 2005/02/14 19:08:37 dannybackx Exp $
 */

/*
 * Actions.h created by Andrew Lister (6 August, 1995)
 */

#ifndef _Xbae_Actions_h
#  define _Xbae_Actions_h

#  include <Xbae/Macros.h>

/*
 * Actions
 */

#  ifdef __cplusplus
extern "C" {
#  endif

	void xbaeEditCellACT(Widget, XEvent *, String *, Cardinal *);
	void xbaeCancelEditACT(Widget, XEvent *, String *, Cardinal *);
	void xbaeCommitEditACT(Widget, XEvent *, String *, Cardinal *);
	void xbaeSelectCellACT(Widget, XEvent *, String *, Cardinal *);
	void xbaeDefaultActionACT(Widget, XEvent *, String *, Cardinal *);
	void xbaeResizeColumnsACT(Widget, XEvent *, String *, Cardinal *);
	void xbaeTraverseNextACT(Widget, XEvent *, String *, Cardinal *);
	void xbaeTraversePrevACT(Widget, XEvent *, String *, Cardinal *);
	void xbaeProcessDragACT(Widget, XEvent *, String *, Cardinal *);
	void xbaeHandleClick(Widget, XtPointer, XEvent *, Boolean *);
	void xbaeHandleMotionACT(Widget, XEvent *, String *, Cardinal *);
	void xbaePageDownACT(Widget, XEvent *, String *, Cardinal *);
	void xbaePageUpACT(Widget, XEvent *, String *, Cardinal *);

	void xbaeHandleTrackingACT(Widget, XEvent *, String *, Cardinal *);

	void xbaeScrollRowsACT(Widget, XEvent *, String *, Cardinal *);
	void xbaeScrollColumnsACT(Widget, XEvent *, String *, Cardinal *);

#  ifdef __cplusplus
}
#  endif
#endif							/* _Xbae_Actions_h */
