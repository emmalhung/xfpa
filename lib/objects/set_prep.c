/***********************************************************************/
/**		@file	set_prep.c
 *
 * 	Assorted operations on visible display of SETs
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 ***********************************************************************/
/***********************************************************************
*                                                                      *
*    s e t _ p r e p . c                                               *
*                                                                      *
*    Assorted operations on visible display of SETs.                   *
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

#include "set.h"

/***********************************************************************
*                                                                      *
*      p r e p _ s e t                                                 *
*                                                                      *
***********************************************************************/
/***********************************************************************/
/**	Make the given set ready for display.
 *
 * @param[in]	set		set to make ready.
 * @param[in]	dformat	display format for set.
 ***********************************************************************/

void	prep_set
	(
	SET		set,
	int		dformat
	)

	{
	int		i;

	/* Do nothing if not there */
	if (!set)       return;
	if (!set->list) return;

	/* Prepare each item in the set for display */
	for (i=0; i<set->num; i++)
		{
		prep_item(set->type, set->list[i], dformat);
		}
	}
