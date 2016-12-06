/**
 *
 * $Header: /cvsroot/xbae/Xbae/include/XbaeDebug.h,v 1.11 2005/04/02 16:56:23 tobiasoed Exp $
 * 
 * Copyright (C) 1995 Free Software Foundation, Inc.
 * Copyright © 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004 LessTif Development Team
 *
 * This file is part of the GNU LessTif Library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 **/
#ifndef	_XBAE_DEBUG_H
#  define _XBAE_DEBUG_H

/* Define this to avoid installing debug code */
#  define XBAE_PRODUCTION

#  ifndef	__GNUC__
/*
 * GNU C defines __FUNCTION__.
 * Pretend that this extension exists for those environments that lack it.
 */
#    define	__FUNCTION__	"(not known)"

#  endif
	   /* __GNUC__ */

#  include <Xm/XmP.h>

/* initialize debug system */
Boolean _XbaeDebugInit(void);
/* turn Debugging on/off */
void _XbaeDebugSet(Boolean flag);
/* toggle Debugging */
void _XbaeDebugToggle(void);
/* query about debugging */
Boolean _XbaeDebugQueryState(void);

/*
 * Print a widget tree
 */
void _XbaeDebugPrintTree(Widget w);
void _XbaeDebugPrintCompleteTree(Widget w);

/*
 *  State of a widget
 */
const char *_XbaeDebugState(Widget w);

/*
 * Print an Arg list
 */
void _XbaeDebugPrintArgList(const char *fn, Widget w, ArgList al, int n, Boolean Get);

/*
 * Convert types into string format
 */
const char *_XbaeDebugFocusDetail2String(int type);
const char *_XbaeDebugFocusMode2String(int type);
const char *_XbaeDebugEventType2String(int type);
const char *_XbaeDebugComboBoxType2String(unsigned char type);
const char *_XbaeDebugGeoAction2String(int action);
const char *_XbaeDebugDeleteResponse2String(int d);
const char *_XbaeDebugGeometryResult2String(XtGeometryResult r);
const char *_XbaeDebugDragAndDropMessageType2String(unsigned char r);
const char *_XbaeDebugDragType2String(unsigned char r);
const char *_XbaeDebugWidgetGeometry2String(XtWidgetGeometry * g);
const char *_XbaeDebugAttachment2String(int i);
const char *_XbaeDebugMenuFocusOp2String(int f);
const char *_XbaeDebugMenuEnum2String(int f);
const char *_XbaeDebugMwmInput2String(int a);
const char *_XbaeDebugBoolean2String(Boolean b);
const char *_XbaeDebugXmString2String(XmString xms);
const char *_XbaeDebugPacking2String(unsigned char p);
const char *_XbaeDebugRcType2String(unsigned char t);
const char *_XbaeDebugAlignment2String(int n);
const char *_XbaeDebugMenuType2String(int n);
const char *_XbaeDebugNavigability2String(unsigned char n);
const char *_XbaeDebugHighlightMode2String(int mode);
const char *_XbaeDebugSelectionPolicy2String(int n);
const char *_XbaeDebugReason2String(int reason);

const char *_XbaeDebugFocusChange2String(XmFocusChange c);

const char *_XbaeDebugNavigationType2String(XmNavigationType nt);
const char *_XbaeDebugEditMode2String(int n);
const char *_XbaeDebugSBDisplayPolicy2String(int n);
const char *_XbaeDebugSBPlacement2String(int n);
const char *_XbaeDebugListSizePolicy2String(int n);
const char *_XbaeDebugResizePolicy2String(int n);

const char *_XbaeDebugScrollingPolicy2String(unsigned char r);
const char *_XbaeDebugDialogStyle2String(int a);
const char *_XbaeDebugShadowTypeToString(const char s);

/*
 * Debug printing functions
 */
void _XbaeDebug(const char *fn, Widget w, const char *fmt, ...);
void _XbaeDebug2(const char *fn, Widget w, Widget c, const char *fmt, ...);
void _XbaeDebug0(const char *fn, Widget w, const char *fmt, ...);
void _XbaeDebugPrintString(const char *s);
void _XbaeDebugAction(const char *fn, Widget w, const String action,
					  const String * params, const Cardinal * num_params);

#  ifdef XBAE_PRODUCTION
#    define	_XbaeDebugInDebug(x, y)	False
#    define DEBUGOUT(x)

#    ifdef	USE_DMALLOC
#      undef	USE_DMALLOC
#    endif
#  else
Boolean _XbaeDebugInDebug(const char *fn, Widget w);
#    define DEBUGOUT(x)	x
#  endif
	   /* XBAE_PRODUCTION */

/*
 * Some stuff to produce sensible tracing with dmalloc.
 * Check the INSTALL(.html) doc for references about the
 * dmalloc package!
 */
#  ifdef WITH_DMALLOC

#    include <dmalloc.h>

/* Our Xt*alloc() replacements */
XtPointer _XbaeDebugMalloc(const char *f, int l, Cardinal size);
XtPointer _XbaeDebugCalloc(const char *f, int l, Cardinal count, Cardinal size);
XtPointer _XbaeDebugRealloc(const char *f, int l, XtPointer p, Cardinal size);
void _XbaeDebugFree(const char *f, int l, XtPointer p);

#    ifdef	XtMalloc
#      undef	XtMalloc
#    endif
#    define	XtMalloc(x)	_XbaeDebugMalloc(__FILE__, __LINE__, x)
#    ifdef	XtCalloc
#      undef	XtCalloc
#    endif
#    define	XtCalloc(x,y)	_XbaeDebugCalloc(__FILE__, __LINE__, x, y)
#    ifdef	XtRealloc
#      undef	XtRealloc
#    endif
#    define	XtRealloc(x,y)	_XbaeDebugRealloc(__FILE__, __LINE__, x, y)
#    ifdef	XtFree
#      undef	XtFree
#    endif
#    define	XtFree(x)	_XbaeDebugFree(__FILE__, __LINE__, x)

#  else
#    ifdef WITH_DBMALLOC
#      include <dbmalloc.h>
#    endif
	   /* WITH_DBMALLOC */

#  endif
	   /* WITH_DMALLOC */


#endif /* _XBAE_DEBUG_H */
