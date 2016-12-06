/*********************************************************************/
/** @file item.h
 *
 * ITEM object definitions (include file)
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    i t e m . h                                                       *
*                                                                      *
*    ITEM object definitions (include file)                            *
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
#ifndef ITEM_DEFS
#define ITEM_DEFS

/* Assorted parameters */
#define DELTA_ITEM 1

/* Define ITEM object */
/** "any of the above", cast as void pointer */
typedef void *ITEM;
/* Define SITEM structure */
/** Permits sorting of various items by some numerical value - see itemcmp function */
typedef	struct
	{
	ITEM	item;	/**< Pointer to item */
	float	value;	/**< value to sort by */
	}	SITEM;

/* Convenient definitions */
#define NullItem        NullPtr(ITEM)
#define NullItemPtr     NullPtr(ITEM *)
#define NullItemList    NullPtr(ITEM *)
#define NullItemListPtr NullPtr(ITEM **)

/* Include things that can be treated as an item */
#include "area.h"
#include "barb.h"
#include "button.h"
#include "curve.h"
#include "label.h"
#include "lchain.h"
#include "mark.h"
#include "spot.h"
#include "projection.h"

/* Declare item functions in item.c */
ITEM	create_bgnd_item(STRING type,
						STRING subelem, STRING value, STRING label);
ITEM	destroy_item(STRING type, ITEM item);
ITEM	empty_item(STRING type, ITEM item);
ITEM	copy_item(STRING type, const ITEM item);
void	define_item_value(STRING type, ITEM item,
						STRING subelem, STRING value, STRING label);
void	recall_item_value(STRING type, ITEM item,
						STRING *subelem, STRING *value, STRING *label);
void	define_item_attribs(STRING type, ITEM item, ATTRIB_LIST attribs);
void	recall_item_attribs(STRING type, ITEM item, ATTRIB_LIST *attribs);
void	highlight_item(STRING type, ITEM item, HILITE code);
void	highlight_item_category(STRING type, ITEM item,
						STRING category, HILITE code);
void	invoke_item_catspec(STRING type, ITEM item,
						int ncspec, const CATSPEC *cspecs);
void	invoke_item_pltspec(STRING type, ITEM item,
						int ncspec, const PLTSPEC *cspecs);
void	change_item_pspec(STRING type, ITEM item, PPARAM param, POINTER value);
void	recall_item_pspec(STRING type, ITEM item, PPARAM param, POINTER value);
LOGICAL	inbox_item(STRING type, ITEM item, const BOX * box);
LOGICAL	reproject_item(STRING type, ITEM item,
						const MAP_PROJ *smproj, const MAP_PROJ *tmproj);
LOGICAL	offset_item(STRING type, ITEM item, float xoff, float yoff);
LOGICAL	scale_item(STRING type, ITEM item, float xscale, float yscale);
STRING	item_attribute(STRING type, ITEM item, STRING name);
STRING	item_category(STRING type, ITEM item);
STRING	item_autolabel(STRING type, ITEM item);
STRING	item_userlabel(STRING type, ITEM item);
int		itemcmp(const void *s1, const void *s2);
int		itemrev(const void *s1, const void *s2);

/* Declare all functions in item_prep.c */
void		prep_item(STRING type, ITEM item, int dformat);

/* Now it has been included */
#endif
