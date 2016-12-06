/*********************************************************************/
/** @file button.h
 *
 * BUTTON object definitions (include file)
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    b u t t o n . h                                                   *
*                                                                      *
*    BUTTON object definitions (include file)                          *
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
#ifndef BUTTON_DEFS
#define BUTTON_DEFS

/* We need definitions for other objects */
#include "pspec.h"
#include "attrib.h"

/* Parameters */
#define DELTA_OPTION 1

/* Define BUTTON object */
/** a text string with a given size and location */
typedef struct BUTTON_struct
	{
	BOX			box;		/**< box dimensions in world co-ords */
	POINT		lpos;		/**< label centre position */
	LSPEC		lspec;		/**< how to display button border */
	FSPEC		fspec;		/**< how to display button interior */
	TSPEC		tspec;		/**< how to display button text */
	TABLE		*options;	/**< list of options */
	int			numopt;		/**< number of options */
	int			maxopt;		/**< allocated options */
	ATTRIB_LIST	attrib;		/**< attribute list */
	/* Following will become obsolete */
	STRING		subelem;	/**< button subelement */
	STRING		value;		/**< text transmitted to program */
	STRING		label;		/**< text that appears on screen */
	} *BUTTON;

/* Convenient definitions */
#define NullButton        NullPtr(BUTTON)
#define NullButtonPtr     NullPtr(BUTTON *)
#define NullButtonList    NullPtr(BUTTON *)
#define NullButtonListPtr NullPtr(BUTTON **)

/* Declare all functions in button.c */
BUTTON	create_button(STRING subelem, STRING value, STRING label,
						const BOX *box);
BUTTON	destroy_button(BUTTON button);
BUTTON	copy_button(const BUTTON button);
void	define_button_value(BUTTON button, STRING subelem, STRING value);
void	recall_button_value(BUTTON button, STRING *subelem, STRING *value);
void	define_button_attribs(BUTTON button, ATTRIB_LIST attribs);
void	recall_button_attribs(BUTTON button, ATTRIB_LIST *attribs);
void	define_button_label(BUTTON button, STRING label, const BOX *box);
void	recall_button_label(BUTTON button, STRING *label, BOX **box);
void	define_button_pspec(BUTTON button, PPARAM param, POINTER value);
void	recall_button_pspec(BUTTON button, PPARAM param, POINTER value);
void	add_option_to_button(BUTTON button, STRING optnam, STRING optval);
STRING	search_button_option(BUTTON button, STRING optnam);
LOGICAL	inside_button(BUTTON button, POINT ptest);
LOGICAL	inside_button_xy(BUTTON button, float x, float y);

/* Functions in button_set.c are declared in set_oper.h */

/* Now it has been included */
#endif
