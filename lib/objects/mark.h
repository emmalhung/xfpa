/*********************************************************************/
/** @file mark.h
 *
 * MARK object definitions (include file)
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    m a r k . h                                                       *
*                                                                      *
*    MARK object definitions (include file)                            *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2002 Environment Canada (MSC)            *
*     Version 7 (c) Copyright 2006 Environment Canada                  *
*     Version 8 (c) Copyright 2011 Environment Canada                  *
*                                                                      *
*   This file is part of the Forecast Production Assistant (FPA).      *
*   The FPA is free software: you can redistribute it and/or modify it *
*   under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation, either version 3 of the License, or  *
*   any later version.                                                 *
*                                                                      *
*   The FPA is distributed in the hope that it will be useful, but     *
*   WITHOUT ANY WARRANTY; without even the implied warranty of         *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.               *
*   See the GNU General Public License for more details.               *
*                                                                      *
*   You should have received a copy of the GNU General Public License  *
*   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.   *
*                                                                      *
***********************************************************************/

/* See if already included */
#ifndef MARK_DEFS
#define MARK_DEFS

/* We need definitions for other objects */
#include "pspec.h"
#include "attrib.h"

/* Define MARK object */
/** a text string with a given size and location */
typedef struct MARK_struct
	{
	POINT		anchor;		/**< location of anchor point */
	float		angle;		/**< orientation of mark */
	MSPEC		mspec;		/**< how to display the mark */
	ATTRIB_LIST	attrib;		/**< attribute list */
	/* Following will become obsolete */
	STRING		subelem;	/**< subelement */
	STRING		value;		/**< value */
	STRING		label;		/**< label */
	} *MARK;

/* Convenient definitions */
#define NullMark        NullPtr(MARK)
#define NullMarkPtr     NullPtr(MARK *)
#define NullMarkList    NullPtr(MARK *)
#define NullMarkListPtr NullPtr(MARK **)

/* Declare all functions in mark.c */
MARK create_mark(STRING subelem, STRING value, STRING label,
						const POINT anchor, float angle);
MARK destroy_mark(MARK mark);
MARK copy_mark(const MARK mark);
void define_mark_value(MARK mark,
						STRING subelem, STRING value, STRING label);
void recall_mark_value(MARK mark,
						STRING *subelem, STRING *value, STRING *label);
void define_mark_attribs(MARK mark, ATTRIB_LIST attribs);
void recall_mark_attribs(MARK mark, ATTRIB_LIST *attribs);
void define_mark_anchor(MARK mark, const POINT anchor, float angle);
void recall_mark_anchor(MARK mark, POINT anchor, float *angle);
void define_mark_pspec(MARK mark, PPARAM param, POINTER value);
void recall_mark_pspec(MARK mark, PPARAM param, POINTER value);

/* Functions in mark_set.c declared in set_oper.h */

/* Now it has been included */
#endif
