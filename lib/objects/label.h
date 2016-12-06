/*********************************************************************/
/** @file label.h
 *
 * LABEL object definitions (include file)
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    l a b e l . h                                                     *
*                                                                      *
*    LABEL object definitions (include file)                           *
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
#ifndef LABEL_DEFS
#define LABEL_DEFS

/* We need definitions for other objects */
#include "pspec.h"
#include "attrib.h"

/* Define LABEL object */
/** a text string with a given size and location */
typedef struct LABEL_struct
	{
	TSPEC		tspec;		/**< how to display the label */
	POINT		anchor;		/**< location of anchor point */
	float		angle;		/**< orientation of label */
	ATTRIB_LIST	attrib;		/**< attribute list */
	/* Following will become obsolete */
	STRING		subelem;	/**< subelement */
	STRING		value;		/**< label value */
	STRING		label;		/**< actual text that appears on screen */
	} *LABEL;

/* Convenient definitions */
#define NullLabel        NullPtr(LABEL)
#define NullLabelPtr     NullPtr(LABEL *)
#define NullLabelList    NullPtr(LABEL *)
#define NullLabelListPtr NullPtr(LABEL **)

/* Declare all functions in label.c */
LABEL	create_label(STRING subelement, STRING value, STRING text,
						const POINT anchor, float angle);
LABEL	destroy_label(LABEL label);
LABEL	copy_label(const LABEL label);
void	define_label_value(LABEL label,
						STRING subelem, STRING value, STRING text);
void	recall_label_value(LABEL label,
						STRING *subelem, STRING *value, STRING *text);
void	define_label_attribs(LABEL label, ATTRIB_LIST attribs);
void	recall_label_attribs(LABEL label, ATTRIB_LIST *attribs);
void	define_label_anchor(LABEL label, const POINT anchor, float angle);
void	recall_label_anchor(LABEL label, POINT anchor, float *angle);
void	define_label_pspec(LABEL label, PPARAM param, POINTER value);
void	recall_label_pspec(LABEL label, PPARAM param, POINTER value);

/* Functions in label_set.c are declared in set_oper.h */

/* Now it has been included */
#endif
