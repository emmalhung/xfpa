/*********************************************************************/
/** @file menu.h
 *
 * MENU object definitions (include file)
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    m e n u . h                                                       *
*                                                                      *
*    MENU object definitions (include file)                            *
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
#ifndef MENU_DEFS
#define MENU_DEFS

/* We need definitions for various objects */
#include "set.h"

/* Useful parameters */
#define DELTA_ELEM 1
#define DELTA_VALID 1

/** Define ELEM structure */
typedef struct
	{
	STRING	element;	/**< element */
	STRING	entity;		/**< entity */
	STRING	level;		/**< level (sfc, pressure, sigma, theta...) */
	COLOUR	colour;		/**< line colour to draw with */
	LSTYLE	style;		/**< line style to draw with */
	STRING	button;		/**< ident of button which picks this element */
	STRING	*attrib;	/**< list of valid attribute button idents */
	short	numatt;		/**< number of attributes */
	short	maxatt;		/**< max number of attributes before a resize is required  */
	STRING	*action;	/**< list of valid action button idents */
	short	numact;		/**< number of actions */
	short	maxact;		/**< max number of actions before a resize is required */
	} ELEM;

/** Define MENU object */
typedef struct MENU_struct
	{
	float	xlen, ylen;	/**< menu dimensions */
	ELEM	*elems;		/**< list of elements */
	short	numelem;	/**< number of elements */
	short	maxelem;	/**< allocated elements */
	SET	buttons;		/**< set of buttons */
	} *MENU;

/* Convenient definitions */
#define NullMenu      NullPtr(MENU)
#define NullMelem     NullPtr(ELEM)
#define NullMelemList NullPtr(ELEM *)

/* Declare all functions in menu.c */
MENU	create_menu(float xlen, float ylen);
MENU	destroy_menu(MENU menu);
void	define_menu_size(MENU menu, float xlen, float ylen);
void	add_button_to_menu(MENU menu, BUTTON button);
BUTTON	find_menu_button(MENU menu, STRING type, STRING ident);
void	elem_info(MENU menu, STRING element, STRING *entity, STRING *level,
						int *colour, int *style, STRING *ident);
BUTTON	pick_menu_button(MENU menu, float x, float y);
void	add_elem_to_menu(MENU menu, STRING element, STRING entity, STRING level,
						int colour, int style, STRING ident);
void	add_valid_button(MENU menu, STRING idelem, STRING ident, STRING type);
LOGICAL	check_valid_button(MENU menu, STRING idelem, STRING ident, STRING type);

/* Now it has been included */
#endif
