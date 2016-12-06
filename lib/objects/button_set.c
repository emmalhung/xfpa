/*********************************************************************/
/**	@file button_set.c
 *
 * Assorted operations on buttons and sets of buttons.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    b u t t o n _ s e t . c                                           *
*                                                                      *
*    Assorted operations on buttons and sets of buttons.               *
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

#include "button.h"
#include "set_oper.h"

#include <tools/tools.h>
#include <fpa_math.h>

#include <stdio.h>
#include <string.h>

/***********************************************************************
*                                                                      *
*      p i c k _ s e t _ b u t t o n                                   *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Return the button from the given set which contains the given point.
 *
 *	@param[in] 	set	Set of buttons
 *	@param[in] 	p	Point to match
 *  @return Pointer to button the contains the point.
 *********************************************************************/

BUTTON	pick_set_button

	(
	SET		set,
	POINT	p
	)

	{
	int		i;
	BUTTON	button;

	/* Do nothing if not there */
	if (!p)                         return NullButton;
	if (!set)                       return NullButton;
	if (!same(set->type, "button")) return NullButton;

	for (i=0; i<set->num; i++)
		{
		button = (BUTTON) set->list[i];
		if (inside_button(button, p)) return button;
		}

	return NullButton;
	}
