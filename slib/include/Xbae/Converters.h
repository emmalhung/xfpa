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
 * $Id: Converters.h,v 1.11 2005/03/14 00:18:28 tobiasoed Exp $
 */

#ifndef _Xbae_Converters_h
#  define _Xbae_Converters_h

#  include <Xm/Xm.h>
#  include <Xbae/Macros.h>

#  ifdef __cplusplus
extern "C" {
#  endif

/*
 * Converters.h created by Andrew Lister (6 August, 1995)
 */

/*
 * Type converters
 */
	Boolean XbaeCvtStringToStringArray(Display *, XrmValuePtr, Cardinal *,
									   XrmValuePtr, XrmValuePtr, XtPointer *);
	void XbaeStringArrayDestructor(XtAppContext, XrmValuePtr, XtPointer, XrmValuePtr, Cardinal *);
	Boolean XbaeCvtStringToCellTable(Display *, XrmValuePtr, Cardinal *,
									 XrmValuePtr, XrmValuePtr, XtPointer *);
	void XbaeStringCellDestructor(XtAppContext, XrmValuePtr, XtPointer, XrmValuePtr, Cardinal *);
	Boolean XbaeCvtStringToWidthArray(Display *, XrmValuePtr, Cardinal *,
									  XrmValuePtr, XrmValuePtr, XtPointer *);
	void XbaeWidthArrayDestructor(XtAppContext, XrmValuePtr, XtPointer, XrmValuePtr, Cardinal *);
	Boolean XbaeCvtStringToMaxLengthArray(Display *, XrmValuePtr, Cardinal *,
										  XrmValuePtr, XrmValuePtr, XtPointer *);
	void XbaeMaxLengthArrayDestructor(XtAppContext, XrmValuePtr, XtPointer, XrmValuePtr, Cardinal *);
	Boolean XbaeCvtStringToBooleanArray(Display *, XrmValuePtr, Cardinal *,
										XrmValuePtr, XrmValuePtr, XtPointer *);
	void XbaeBooleanArrayDestructor(XtAppContext, XrmValuePtr, XtPointer, XrmValuePtr, Cardinal *);
	Boolean XbaeCvtStringToAlignmentArray(Display *, XrmValuePtr, Cardinal *,
										  XrmValuePtr, XrmValuePtr, XtPointer *);
	void XbaeAlignmentArrayDestructor(XtAppContext, XrmValuePtr, XtPointer, XrmValuePtr, Cardinal * num_args);
	Boolean XbaeCvtStringToShadowTypeArray(Display *, XrmValuePtr, Cardinal *,
										   XrmValuePtr, XrmValuePtr, XtPointer *);
	void XbaeShadowTypeArrayDestructor(XtAppContext, XrmValuePtr, XtPointer,
									   XrmValuePtr, Cardinal * num_args);
	Boolean XbaeCvtStringToPixelTable(Display *, XrmValuePtr, Cardinal *,
									  XrmValuePtr, XrmValuePtr, XtPointer *);
	void XbaePixelTableDestructor(XtAppContext, XrmValuePtr, XtPointer, XrmValuePtr, Cardinal *);
	Boolean XbaeCvtStringToGridType(Display *, XrmValuePtr, Cardinal *,
									XrmValuePtr, XrmValuePtr, XtPointer *);
	Boolean XbaeCvtStringToWrapType(Display *, XrmValuePtr, Cardinal *,
									XrmValuePtr, XrmValuePtr, XtPointer *);
	Boolean XbaeCvtStringToPixelArray(Display *, XrmValuePtr, Cardinal *,
									  XrmValuePtr, XrmValuePtr, XtPointer *);
	void XbaePixelArrayDestructor(XtAppContext, XrmValuePtr, XtPointer, XrmValuePtr, Cardinal *);

	     Boolean
#  ifdef __VMS
	     XbaeCvtStringToMatrixScrollBarD
#  else
	     XbaeCvtStringToMatrixScrollBarDisplayPolicy
#  endif
	     (Display *, XrmValuePtr, Cardinal *, XrmValuePtr, XrmValuePtr, XtPointer *);

#  ifdef __cplusplus
}
#  endif
#endif
