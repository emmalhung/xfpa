/*********************************************************************/
/** @file set.h
 *
 * SET object definitions (include file)
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    s e t . h                                                         *
*                                                                      *
*    SET object definitions (include file)                             *
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
#ifndef SET_DEFS
#define SET_DEFS

/* Include things that can belong to a set */
#include "item.h"

/* Assorted parameters */
#define DELTA_SET 1

/** Define SET object - a group of similar items */
typedef struct SET_struct
	{
	STRING	type;		/**< type of item in set */
	ITEM	bgnd;		/**< pseudo-item containing attributes of background */
	ITEM	*list;		/**< list of items */
	int		num;		/**< number of items */
	int		max;		/**< allocated items */
	CATSPEC	*cspecs;	/**< list of category specs */
	short	ncspec;		/**< number of category specs */
	CATSPEC	*xspecs;	/**< secondary list of category specs */
	short	nxspec;		/**< number of secondary category specs */
	} *SET;

/* Convenient definitions */
#define NullSet    NullPtr(SET)
#define NullSetPtr NullPtr(SET *)

/* Declare functions in set.c */
SET		create_set(STRING type);
SET		destroy_set(SET set);
SET		copy_set(const SET set);
void	define_set_bgnd(SET set, ITEM bgnd);
void	define_set_bgval(SET set, STRING subelem, STRING value, STRING label);
void	define_set_bg_attribs(SET set, ATTRIB_LIST attribs);
void	recall_set_bg_attribs(SET set, ATTRIB_LIST *attribs);
void	define_set_type(SET set, STRING type);
void	recall_set_type(SET set, STRING *type);
void	empty_set(SET set);
SET		append_set(SET set1, const SET set2);
ITEM	add_item_to_set(SET set, ITEM item);
ITEM	add_item_to_set_start(SET set, ITEM item);
ITEM	remove_item_from_set(SET set, ITEM item);
int		which_set_item(SET set, ITEM item);
void	move_set_item(SET set, int ifrom, int ito);
void	recall_set_list(SET set, ITEM **list, int *num, int *max);
ITEM	recall_set_item(SET set, int nitem);
void	define_set_catspecs(SET set, int ncspec, CATSPEC *cspecs);
void	add_catspec_to_set(SET set, CATSPEC *cspec);
void	recall_set_catspecs(SET set, int *ncspec, CATSPEC **cspecs);
CATSPEC	*find_set_catspec(SET set, STRING subelem);
void	define_set_secondary_catspecs(SET set, int nxspec, CATSPEC *xspecs);
void	add_secondary_catspec_to_set(SET set, CATSPEC *xspec);
void	invoke_set_catspecs(SET set);
void	change_set_pspec(SET set, PPARAM param, POINTER value);
void	recall_set_pspec(SET set, PPARAM param, POINTER value);
void	highlight_set(SET set, HILITE code);
void	highlight_set_category(SET set, STRING category, HILITE code);
void	highlight_set_secondary(SET set, HILITE code);
void	highlight_set_secondary_category(SET set, STRING category, HILITE code);
void	strip_set(SET set, const BOX *box);
LOGICAL	reproject_set(SET set, const MAP_PROJ *smproj, const MAP_PROJ *tmproj);
int		set_count(SET set, STRING category);
void	prepare_set(SET set);

/* Declare all functions in set_prep.c */
void		prep_set(SET set, int dformat);

/* Now it has been included */
#endif
