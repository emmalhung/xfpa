/*
 * Copyright(c) 1992 Bell Communications Research, Inc. (Bellcore)
 * Copyright(c) 1995-99 Andrew Lister
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
 * $Id: Methods.h,v 1.18 2005/03/30 04:25:09 tobiasoed Exp $
 */


/*
 * Methods.h created by Andrew Lister (7 August, 1995)
 */
#ifndef _Xbae_Methods_h
#  define _Xbae_Methods_h

#  include <Xbae/Macros.h>

#  ifdef __cplusplus
extern "C" {
#  endif

	void xbaeRelayout(XbaeMatrixWidget);
	void xbaeResize(XbaeMatrixWidget);

/*
 * New Matrix methods
 */
	void xbaeUpdateTextChild(XbaeMatrixWidget mw, Boolean updateValue);
	void xbaeUpdateTextChildFont(XbaeMatrixWidget mw, XrmQuark qtag);

	void xbaeSetCell(XbaeMatrixWidget, int, int, const String, Boolean);
	void xbaeModifyVerifyCB(Widget, XtPointer, XtPointer);
	void xbaeValueChangedCB(Widget, XtPointer, XtPointer);
	void xbaeFocusCB(Widget, XtPointer, XtPointer);
	void xbaeLosingFocusCB(Widget, XtPointer, XtPointer);
	void xbaeEditCell(XbaeMatrixWidget, XEvent *, int, int, String *, Cardinal);
	void xbaeSelectCell(XbaeMatrixWidget, int, int);
	void xbaeSelectRow(XbaeMatrixWidget, int);
	void xbaeSelectColumn(XbaeMatrixWidget, int);
	void xbaeDeselectAll(XbaeMatrixWidget);
	void xbaeSelectAll(XbaeMatrixWidget);
	void xbaeDeselectCell(XbaeMatrixWidget, int, int);
	void xbaeDeselectRow(XbaeMatrixWidget, int);
	void xbaeDeselectColumn(XbaeMatrixWidget, int);
	String xbaeGetCell(XbaeMatrixWidget, int, int);
	Boolean xbaeCommitEdit(XbaeMatrixWidget, XEvent *, Boolean);
	void xbaeCancelEdit(XbaeMatrixWidget, Boolean);
	void xbaeAddRows(XbaeMatrixWidget, int, String *, String *, Pixel *, Pixel *, int);
	void xbaeAddVarRows(XbaeMatrixWidget, int, String *, String *, short *,
						int *, unsigned char *, unsigned char *, Pixel *, Pixel *, int);
	void xbaeDeleteRows(XbaeMatrixWidget, int, int);
	void xbaeAddColumns(XbaeMatrixWidget, int, String *, String *, short *,
						int *, unsigned char *, unsigned char *, Pixel *, Pixel *, int);
	void xbaeDeleteColumns(XbaeMatrixWidget, int, int);
	void xbaeSetRowColors(XbaeMatrixWidget, int, Pixel *, int, Boolean);
	void xbaeSetColumnColors(XbaeMatrixWidget, int, Pixel *, int, Boolean);
	void xbaeSetCellColor(XbaeMatrixWidget, int, int, Pixel, Boolean);
	void xbaeShowColumnArrows(XbaeMatrixWidget, int, Boolean);

#  ifdef __cplusplus
}
#  endif
#endif							/* _Xbae_Methods_h */
