/*********************************************************************/
/**	@file menu.c
 *
 * Routines to handle the MENU object.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*      m e n u . c                                                     *
*                                                                      *
*      Routines to handle the MENU object.                             *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
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

#define MENU_INIT
#include "menu.h"

#include <tools/tools.h>
#include <fpa_getmem.h>
#include <fpa_math.h>
#include <string.h>

int		MenuCount = 0;

/***********************************************************************
*                                                                      *
*      c r e a t e _ m e n u                                           *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Create a new menu with given attributes.
 *
 *	@param[in] 	xlen		Width
 *	@param[in] 	ylen		Height
 *  @return Pointer to new menu object. You will need to destroy this
 * 			object when you are finished with it.
 *********************************************************************/

MENU    create_menu

	(
	float	xlen,
	float	ylen
	)

	{
	MENU    menu;

	/* Allocate memory for structure */
	menu = INITMEM(struct MENU_struct,1);
	if (!menu) return NullMenu;

	/* Define dimensions */
	define_menu_size(menu,xlen,ylen);

	/* Initialize element list */
	menu->elems   = NullMelemList;
	menu->numelem = 0;
	menu->maxelem = 0;

	/* Initialize button set */
	menu->buttons = create_set("button");

	/* Return the new metafile */
	MenuCount++;
	return menu;
	}

/***********************************************************************
*                                                                      *
*      d e s t r o y _ m e n u                                         *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Destroy a menu.
 *
 *	@param[in]     menu	menu to be destroyed
 *  @return NullMenu
 *********************************************************************/

MENU    destroy_menu

	(
	MENU    menu
	)

	{
	/* Do nothing if not there */
	if (!menu) return NullMenu;

	/* Return space for element list */
	FREEMEM(menu->elems);
	menu->numelem = 0;
	menu->maxelem = 0;

	/* Return space for button set */
	menu->buttons = destroy_set(menu->buttons);

	/* Return structure itself */
	FREEMEM(menu);
	MenuCount--;
	return NullMenu;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ m e n u _ s i z e                                 *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Define the menu dimensions.
 *
 *	@param[in] 	menu	given menu
 *	@param[in] 	xlen	Width
 *	@param[in] 	ylen	Height
 *********************************************************************/

void	define_menu_size

	(
	MENU	menu,
	float	xlen,
	float	ylen
	)

	{
	/* Do nothing if menu not there */
	if (!menu)   return;

	/* Set values */
	menu->xlen = xlen;
	menu->ylen = ylen;
	}

/***********************************************************************
*                                                                      *
*      a d d _ b u t t o n _ t o _ m e n u                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Add the given button to the given menu.
 *
 *	@param[in] 	menu		given menu
 *	@param[in] 	button		button to add
 *********************************************************************/

void	add_button_to_menu

	(
	MENU	menu,
	BUTTON	button
	)

	{
	/* Do nothing if menu not there */
	if (!menu)   return;
	if (!button) return;

	/* Add button to current list */
	add_item_to_set(menu->buttons,(ITEM) button);
	}

/***********************************************************************
*                                                                      *
*      f i n d _ m e n u _ b u t t o n                                 *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find the specified button in the given menu.
 *
 *	@param[in] 	menu	menu
 *	@param[in] 	type	button type
 *	@param[in] 	ident	button ident
 *  @return Pointer to the requested button.
 *********************************************************************/

BUTTON	find_menu_button

	(
	MENU	menu,
	STRING	type,
	STRING	ident
	)

	{
	int		i;
	BUTTON	button;
	SET		set;
	STRING	cat;

	/* Do nothing if menu not there */
	if (!menu) return NullButton;

	/* Retrieve set of buttons */
	set = menu->buttons;
	if (!same(set->type,"button")) return NullButton;

	/* Scan set for desired button */
	for (i=0; i<set->num; i++)
		{
		button = (BUTTON) set->list[i];
		(void) get_attribute(button->attrib, AttribCategory, &cat);
		if (!same_start(cat,type))      continue;
		if (!same(button->label,ident)) continue;
		return button;
		}

	/* Didn't find it */
	return NullButton;
	}

/***********************************************************************
*                                                                      *
*      e l e m _ i n f o                                               *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find stored info about the specified element in the given menu.
 *
 *	@param[in] 	menu	menu
 *	@param[in] 	elem	element name
 *	@param[out] 	*entity	entity name
 *	@param[out] 	*level	level name
 *	@param[out] 	*colour	colour code
 *	@param[out] 	*style	line style
 *	@param[out] 	*ident	identity
 *********************************************************************/

void	elem_info

	(
	MENU	menu,
	STRING	elem,
	STRING	*entity,
	STRING	*level,
	int		*colour,
	int		*style,
	STRING	*ident
	)

	{
	int	i;
	ELEM	*elemi;

	/* Do nothing if menu not there */
	if (entity) *entity = NULL;
	if (level)  *level  = NULL;
	if (colour) *colour = 0;
	if (style)  *style  = 0;
	if (ident)  *ident  = NULL;
	if (!menu) return;

	/* Scan element list for given element */
	for (i=0; i<menu->numelem; i++)
		{
		elemi = &menu->elems[i];
		if (same(elem,elemi->element))
			{
			if (entity) *entity = elemi->entity;
			if (level)  *level  = elemi->level;
			if (colour) *colour = elemi->colour;
			if (style)  *style  = elemi->style;
			if (ident)  *ident  = elemi->button;
			break;
			}
		}
	}

/***********************************************************************
*                                                                      *
*      p i c k _ m e n u _ b u t t o n                                 *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find the button in the given menu at the specified point.
 *
 *	@param[in] 	menu	menu
 *	@param[in] 	x		specified x coord
 *	@param[in] 	y		specified y coord
 *  @return Pointer to a button at the given point if one exists.
 *********************************************************************/

BUTTON	pick_menu_button

	(
	MENU	menu,
	float	x,
	float	y
	)

	{
	int	i;
	BUTTON	button;
	SET	set;

	/* Do nothing if menu not there */
	if (!menu) return NullButton;

	/* Retrieve set of buttons */
	set = menu->buttons;
	if (!same(set->type,"button")) return NullButton;

	/* Scan set for first button whick contains the given point */
	for (i=0; i<set->num; i++)
		{
		button = (BUTTON) set->list[i];
		if (inside_button_xy(button,x,y)) return button;
		}

	/* Didn't find it */
	return NullButton;
	}

/***********************************************************************
*                                                                      *
*      a d d _ e l e m _ t o _ m e n u                                 *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Add the given element to the given menu.
 *
 *	@param[in] 	menu		menu to add element to
 *	@param[in] 	element		element name
 *	@param[in] 	entity		entity
 *	@param[in] 	level		level
 *	@param[in] 	colour		colour for drawing this element
 *	@param[in] 	style		line style for drawing
 *	@param[in] 	ident		button ident
 *********************************************************************/

void	add_elem_to_menu

	(
	MENU	menu,
	STRING	element,
	STRING	entity,
	STRING	level,
	int		colour,
	int		style,
	STRING	ident
	)

	{
	ELEM	*elem;

	/* Do nothing if menu not there */
	if (!menu) return;

	/* See if we need more space */
	if (menu->numelem >= menu->maxelem)
		{
		menu->maxelem += DELTA_ELEM;
		menu->elems    = GETMEM(menu->elems,ELEM,menu->maxelem);
		}

	/* Define the given element */
	elem = menu->elems + menu->numelem++;
	elem->element = INITSTR(element);
	elem->entity  = INITSTR(entity);
	elem->level   = INITSTR(level);
	elem->colour  = colour;
	elem->style   = style;
	elem->button  = INITSTR(ident);
	elem->attrib  = NULL;
	elem->numatt  = 0;
	elem->maxatt  = 0;
	elem->action  = NULL;
	elem->numact  = 0;
	elem->maxact  = 0;
	}

/***********************************************************************
*                                                                      *
*      a d d _ v a l i d _ b u t t o n                                 *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Add the given button (ident) to the list of valid attribute or
 * action buttons for the given element (ident).
 *
 *	@param[in] 	menu	given menu
 *	@param[in] 	idelem	element button ident
 *	@param[in] 	ident	button ident
 *	@param[in] 	type	"attribute" or "action"
 *********************************************************************/

void	add_valid_button

	(
	MENU	menu,
	STRING	idelem,
	STRING	ident,
	STRING	type
	)

	{
	int	i, found=FALSE;
	ELEM	*elem;

	/* Do nothing if menu not there */
	if (!menu) return;

	/* First find given element */
	elem = menu->elems;
	for (i=0; i<menu->numelem; i++)
		if ( found = same(elem[i].button,idelem) ) break;
	if (!found) return;
	elem += i;

	/* Add to valid attribute list */
	if (same_start("attribute",type))
		{
		/* See if we need more room */
		if (elem->numatt >= elem->maxatt)
		{
		elem->maxatt += DELTA_VALID;
		elem->attrib  = GETMEM(elem->attrib,STRING,elem->maxatt);
		}

		/* Add the new attribute ident */
		elem->attrib[elem->numatt++] = INITSTR(ident);
		}

	/* Add to valid action list */
	else if (same_start("action",type))
		{
		/* See if we need more room */
		if (elem->numact >= elem->maxact)
		{
		elem->maxact += DELTA_VALID;
		elem->action  = GETMEM(elem->action,STRING,elem->maxact);
		}

		/* Add the new action ident */
		elem->action[elem->numact++] = INITSTR(ident);
		}
	}

/***********************************************************************
*                                                                      *
*      c h e c k _ v a l i d _ b u t t o n                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Determine if the given button (ident) is a valid attribute or
 * action button for the given element (ident).
 *
 *	@param[in] 	menu	given menu
 *	@param[in] 	idelem	element button ident
 *	@param[in] 	ident	button ident
 *	@param[in] 	type	"attribute" or "action"
 *  @return True if button is valid.
 *********************************************************************/

LOGICAL	check_valid_button

	(
	MENU	menu,
	STRING	idelem,
	STRING	ident,
	STRING	type
	)

	{
	int	i, found=FALSE;
	ELEM	*elem;

	/* Do nothing if menu not there */
	if (!menu) return FALSE;

	/* First find given element */
	elem = menu->elems;
	for (i=0; i<menu->numelem; i++)
		if ( found = same(elem[i].button,idelem) ) break;
	if (!found) return FALSE;
	found = FALSE;
	elem += i;

	/* Search valid attribute list */
	if (same_start("attribute",type))
		{
		for (i=0; i<elem->numatt; i++)
		if ( found = same(elem->attrib[i],ident) ) break;
		return found;
		}

	/* Search valid action list */
	else if (same_start("action",type))
		{
		for (i=0; i<elem->numact; i++)
		if ( found = same(elem->action[i],ident) ) break;
		return found;
		}

	else return FALSE;
	}
