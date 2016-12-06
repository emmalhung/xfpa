/***********************************************************************/
/**		@file	field_prep.c
 *
 * 	Assorted operations on visible display of FIELDs
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 ***********************************************************************/
/***********************************************************************
*                                                                      *
*    f i e l d _ p r e p . c                                           *
*                                                                      *
*    Assorted operations on visible display of FIELDs.                 *
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

#include "field.h"

/***********************************************************************
*                                                                      *
*      p r e p _ f i e l d                                             *
*                                                                      *
***********************************************************************/
/***********************************************************************/
/**	Make the given field ready for display.
 *
 * @param[in]	fld		field to make ready.
 * @param[in]	dformat	display format for field.
 ***********************************************************************/

void	prep_field
	(
	FIELD	fld,
	int		dformat
	)

	{
	/* Do nothing if not there */
	if (!fld) return;

	/* Prepare the field for display based on the type */
	switch (fld->ftype)
		{
		case FtypeNone:		break;

		case FtypeSfc:		break;

		case FtypeSet:		prep_set(fld->data.set, dformat);
							break;

		case FtypePlot:		break;

		case FtypeRaster:	break;
		}
	}
