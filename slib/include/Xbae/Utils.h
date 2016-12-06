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
 * $Id: Utils.h,v 1.40 2005/03/30 04:25:09 tobiasoed Exp $
 */

/*
 * Utils.h created by Andrew Lister (6 August, 1995)
 */
#ifndef _Xbae_Utils_h
#  define _Xbae_Utils_h

#  include <X11/Intrinsic.h>
#  include <Xbae/MatrixP.h>

#  include <Xbae/Macros.h>


#  ifdef __cplusplus
extern "C" {
#  endif

	extern int xbaeRowClip(XbaeMatrixWidget mw, int row);
	extern int xbaeColumnClip(XbaeMatrixWidget mw, int column);
	extern int xbaeCellClip(XbaeMatrixWidget mw, int row, int column);

	Widget xbaeGetCellClip(XbaeMatrixWidget, int, int);

	void xbaeGetColumnPositions(XbaeMatrixWidget);
	void xbaeGetRowPositions(XbaeMatrixWidget);
	int xbaeCheckRowPosition(XbaeMatrixWidget mw, int row);
	int xbaeCheckColumnPosition(XbaeMatrixWidget mw, int column);

	int xbaeTopRow(XbaeMatrixWidget mw);
	int xbaeBottomRow(XbaeMatrixWidget mw);
	int xbaeLeftColumn(XbaeMatrixWidget mw);
	int xbaeRightColumn(XbaeMatrixWidget mw);
	void xbaeGetVisibleRows(XbaeMatrixWidget, int *, int *);
	void xbaeGetVisibleColumns(XbaeMatrixWidget, int *, int *);
	void xbaeGetVisibleCells(XbaeMatrixWidget mw, int *, int *, int *, int *);

	int xbaeCalculateHorizOrigin(XbaeMatrixWidget mw, int left_column);
	int xbaeCalculateVertOrigin(XbaeMatrixWidget mw, int top_row);

	Boolean xbaeIsRowVisible(XbaeMatrixWidget, int);
	Boolean xbaeIsColumnVisible(XbaeMatrixWidget, int);
	Boolean xbaeIsCellVisible(XbaeMatrixWidget, int, int);

	void xbaeMakeRowVisible(XbaeMatrixWidget, int);
	void xbaeMakeColumnVisible(XbaeMatrixWidget, int);
	void xbaeMakeCellVisible(XbaeMatrixWidget, int, int);

	int xbaeStringWidth(XbaeMatrixWidget mw, XbaeMatrixFontInfo *font, String string, Boolean multiline);
	void xbaeComputeSize(XbaeMatrixWidget, Boolean, Boolean);
	int xbaeCalculateLabelMaxWidth(XbaeMatrixWidget mw, String * labels, XmString * xmlabels, int n_labels);
	int xbaeCalculateLabelMaxLines(String * labels, XmString * xmlabels, int n_labels);

	Boolean xbaeEventToMatrixXY(XbaeMatrixWidget mw, XEvent * event, int *x, int *y);
	Boolean xbaeMatrixXtoColumn(XbaeMatrixWidget mw, int *x, int *column);
	Boolean xbaeMatrixYtoRow(XbaeMatrixWidget mw, int *y, int *row);
	Boolean xbaeMatrixXYToRowCol(XbaeMatrixWidget mw, int *x, int *y, int *row, int *column);

	int xbaeColumnToMatrixX(XbaeMatrixWidget mw, int column);
	int xbaeRowToMatrixY(XbaeMatrixWidget mw, int row);
	void xbaeRowColToMatrixXY(XbaeMatrixWidget mw, int row, int column, int *widget_x, int *widget_y);
	Widget xbaeRowColToClipXY(XbaeMatrixWidget, int, int, int *, int *);

	void xbaeHideCellWidget(XbaeMatrixWidget mw, Widget cw);
	void xbaePositionCellWidget(XbaeMatrixWidget mw, int row, int column);
	void xbaeHideTextChild(XbaeMatrixWidget mw);
	void xbaePositionTextChild(XbaeMatrixWidget mw);
	void xbaeSetInitialFocus(XbaeMatrixWidget mw);

	void xbaeSaneRectangle(XbaeMatrixWidget mw, XRectangle * rect_p, int rs, int cs, int re, int ce);

	void xbaeObjectLock(Widget);
	void xbaeObjectUnlock(Widget);

	int _xbaeStrcasecmp(const char *s1, const char *s2);
	int _xbaeStrncasecmp(const char *s1, const char *s2, size_t count);

	void xbaeScrollRows(XbaeMatrixWidget, Boolean Left, int step);
	void xbaeScrollColumns(XbaeMatrixWidget, Boolean Up, int step);

#  ifdef __cplusplus
}
#  endif
#endif							/* _Xbae_Utils_h */
