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
 * MatrixWidget Author: Andrew Wason, Bellcore, aw@bae.bellcore.com
 *
 * $Id: patchlevel.h.in,v 1.6 2002/03/08 16:12:23 amai Exp $
 */

#ifndef _Xbae_patchlevel_h
#  define _Xbae_patchlevel_h

#  ifndef	XbaeVERSION
#    define	XbaeVERSION	4
#  endif
#  ifndef	XbaeREVISION
#    define	XbaeREVISION	60
#  endif
#  ifndef	XbaeUPDATE
#    define	XbaeUPDATE	2
#  endif
#  ifndef	XbaeVersion
#    define XbaeVersion (XbaeVERSION * 10000 + XbaeREVISION * 100 + XbaeUPDATE)
#  endif
#  ifndef	XbaeVersionTxt
#    define XbaeVersionTxt "@(#) Xbae Widget Set, version 4.60.2"
#  endif

#endif
