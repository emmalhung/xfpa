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
 * $Id: Draw.h,v 1.18 2005/04/02 20:35:18 tobiasoed Exp $
 */

/*
 * Draw.h created by Andrew Lister (30 October, 1995)
 */

#ifndef _Xbae_Draw_h
#  define _Xbae_Draw_h

#  include <Xbae/Macros.h>

#  ifdef __cplusplus
extern "C" {
#  endif


	typedef struct {
		XbaeMatrixDrawCellCallbackStruct drawCB;
		XrmQuark qtag;
	} XbaeMatrixCellValuesStruct;

	void xbaeGetCellValues(XbaeMatrixWidget, int, int, Boolean, XbaeMatrixCellValuesStruct *);

	void xbaeChangeHighlight(XbaeMatrixWidget, int, int, unsigned char);
	void xbaeDrawCell(XbaeMatrixWidget, int, int);
	void xbaeDrawColumnLabel(XbaeMatrixWidget mw, int column, Boolean pressed);
	void xbaeDrawRowLabel(XbaeMatrixWidget mw, int row, Boolean pressed);
	int  xbaeXftStringWidth(XbaeMatrixWidget mw, XftFont *font, String string, int nc);

#  ifdef __cplusplus
}
#  endif
#endif							/* _Xbae_Draw_h */
