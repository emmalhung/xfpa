/***********************************************************************/
/**		@file	item_prep.c
 *
 * 	Assorted operations on visible display of ITEMs
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 ***********************************************************************/
/***********************************************************************
*                                                                      *
*    i t e m _ p r e p . c                                             *
*                                                                      *
*    Assorted operations on visible display of ITEMs.                  *
*                                                                      *
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

#include "item.h"

#include <tools/tools.h>

/***********************************************************************
*                                                                      *
*      p r e p _ i t e m                                               *
*                                                                      *
***********************************************************************/
/***********************************************************************/
/**	Make the given item ready for display.
 *
 * @param[in]	type	specified item type.
 * @param[in]	item	item to make ready.
 * @param[in]	dformat	display format for item.
 ***********************************************************************/

void	prep_item
	(
	STRING	type,
	ITEM	item,
	int		dformat
	)

	{
	/* Do nothing if type or item are null */
	if (!type || !item) return;

	/* Prepare the item for display based on the type */
	if (same(type, "area"))
		{
		if (dformat == DisplayFormatComplex) prep_area_complex((AREA) item);
		else                                 prep_area((AREA) item);
		return;
		}
	else if (same(type, "subarea")) return;
	else if (same(type, "barb"))    return;
	else if (same(type, "button"))  return;
	else if (same(type, "curve"))   return;
	else if (same(type, "label"))   return;
	else if (same(type, "mark"))    return;
	else if (same(type, "spot"))    return;
	else if (same(type, "lchain"))  return;
	else                            return;
	}
