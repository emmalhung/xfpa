/*
 * Copyright (c) 1992 Bell Communications Research, Inc. (Bellcore)
 * Copyright (c) 1995-99 Andrew Lister
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
 * $Id: Create.h,v 1.29 2005/06/13 13:36:59 tobiasoed Exp $
 */

/*
 * Create.h created by Andrew Lister (28 January, 1996)
 */

#ifndef _Xbae_Create_h
#  define _Xbae_Create_h

#  include <Xbae/Macros.h>

#  ifdef __cplusplus
extern "C" {
#  endif

	void xbaeCopyBackground(Widget, int, XrmValue *);
	void xbaeCopyForeground(Widget, int, XrmValue *);
	void xbaeCopyDoubleClick(Widget, int, XrmValue *);
	void xbaeCopyCellShadowTypes(XbaeMatrixWidget);
	void xbaeCopyRowShadowTypes(XbaeMatrixWidget);
	void xbaeCopyColumnShadowTypes(XbaeMatrixWidget);
	void xbaeCopyCellUserData(XbaeMatrixWidget);
	void xbaeCopyRowUserData(XbaeMatrixWidget);
	void xbaeCopyColumnUserData(XbaeMatrixWidget);
	void xbaeCopySelectedCells(XbaeMatrixWidget);
	void xbaeCopyRowLabels(XbaeMatrixWidget);
	void xbaeCopyColumnLabels(XbaeMatrixWidget);
	void xbaeCopyCells(XbaeMatrixWidget);
	void xbaeCopyCellWidgets(XbaeMatrixWidget);
	void xbaeCopyColumnWidths(XbaeMatrixWidget);
	void xbaeCopyRowHeights(XbaeMatrixWidget);
	void xbaeCopyColumnMaxLengths(XbaeMatrixWidget);
	void xbaeCopyBackgrounds(XbaeMatrixWidget);
	void xbaeCopyColumnAlignments(XbaeMatrixWidget);
	void xbaeCopyColumnLabelAlignments(XbaeMatrixWidget);
	void xbaeCopyColumnLabelBackgrounds(XbaeMatrixWidget);
	void xbaeCopyColumnLabelForegrounds(XbaeMatrixWidget);
	void xbaeCopyRowLabelBackgrounds(XbaeMatrixWidget);
	void xbaeCopyRowLabelForegrounds(XbaeMatrixWidget);
	void xbaeCopyColumnButtonLabels(XbaeMatrixWidget);
	void xbaeCopyColumnShowArrows(XbaeMatrixWidget);
	void xbaeCopyColumnFontBold(XbaeMatrixWidget);
	void xbaeCopyRowButtonLabels(XbaeMatrixWidget);
	void xbaeCopyColors(XbaeMatrixWidget);
	void xbaeCopyHighlightedCells(XbaeMatrixWidget);
	void xbaeCopyUnderlinedCells(XbaeMatrixWidget);
	void xbaeCreateDrawGC(XbaeMatrixWidget);
	void xbaeCreatePixmapGC(XbaeMatrixWidget);
	void xbaeCreateLabelGC(XbaeMatrixWidget);
	void xbaeCreateLabelClipGC(XbaeMatrixWidget);
	void xbaeGetGridLineGC(XbaeMatrixWidget);
	void xbaeGetResizeTopShadowGC(XbaeMatrixWidget);
	void xbaeGetResizeBottomShadowGC(XbaeMatrixWidget);

	void xbaeInitFont(XbaeMatrixWidget, XmStringTag, XbaeMatrixFontInfo*);
	void xbaeInitFonts(XbaeMatrixWidget);
	void xbaeSetDrawFont(XbaeMatrixWidget, XrmQuark, Boolean);

	void xbaeFreeCells(XbaeMatrixWidget);
	void xbaeFreeCellWidgets(XbaeMatrixWidget);
	void xbaeFreeRowLabels(XbaeMatrixWidget);
	void xbaeFreeColumnLabels(XbaeMatrixWidget);
	void xbaeFreeColors(XbaeMatrixWidget);
	void xbaeFreeBackgrounds(XbaeMatrixWidget);
	void xbaeFreeSelectedCells(XbaeMatrixWidget);
	void xbaeFreeCellUserData(XbaeMatrixWidget);
	void xbaeFreeCellShadowTypes(XbaeMatrixWidget);
	void xbaeFreeHighlightedCells(XbaeMatrixWidget);
	void xbaeFreeUnderlinedCells(XbaeMatrixWidget);
	void xbaeCreateColors(XbaeMatrixWidget);
	void xbaeCopyShowColumnArrows(XbaeMatrixWidget mw);

	void xbaeCreatePerCell(XbaeMatrixWidget);
	void xbaeRegisterDisplay(Widget w);

/* ARCAD SYSTEMHAUS */

	void xbaeFill_WithEmptyValues_PerCell(XbaeMatrixWidget mw, XbaeMatrixPerCellRec * p);

	void xbaeFreePerCellEntity(XbaeMatrixWidget mw, int row, int column);
	void xbaeFreePerCellRow(XbaeMatrixWidget mw, int row);
	void xbaeFreePerCell(XbaeMatrixWidget mw);

#  ifdef __cplusplus
}
#  endif
#endif							/* _Xbae_Create_h */
