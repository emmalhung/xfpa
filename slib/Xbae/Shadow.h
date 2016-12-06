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
 * $Id: Shadow.h,v 1.11 2004/10/15 16:13:14 dannybackx Exp $
 */

/*
 * Shadow.h created by Andrew Lister (30 October, 1995)
 */

#ifndef _Xbae_Shadow_h
#  define _Xbae_Shadow_h

#  include <Xbae/Macros.h>

#  ifdef __cplusplus
extern "C" {
#  endif

	void xbaeDrawLabelShadow(XbaeMatrixWidget mw, Window win,
							 int x, int y, int width, int height, Boolean pressed);

	void xbaeDrawCellShadow(XbaeMatrixWidget mw, Window win, int row, int column,
							int x, int y, int width, int height);

	void xbaeDrawCellHighlight(XbaeMatrixWidget mw, Window win, GC gc, int row, int column,
							   int x, int y, int width, int height, unsigned char hl);

#  ifdef __cplusplus
}
#  endif
#endif							/* _Xbae_Shadow_h */
