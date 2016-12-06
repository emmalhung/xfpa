/***********************************************************************
*                                                                      *
*     g x _ b a r b . c                                                *
*                                                                      *
*     Useful extensions to the FpaXgl library.                         *
*                                                                      *
*     (c) Copyright 1996 Environment Canada (AES)                      *
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

#include   "gx.h"

/* Pre-defined Barb types */
static 	const	ITBL	BtypeDefs[] =
						{
						BarbNone,	"none",
						BarbWind,	"wind",
						BarbArrow,	"arrow"
						};
static	const	int	NumBtype = ITBL_SIZE(BtypeDefs);

/***********************************************************************
*                                                                      *
*   g x F i n d B a r b T y p e   - map barb type names                *
*                                                                      *
***********************************************************************/

BTYPE	gxFindBarbType ( STRING name )
	{
	return (BTYPE) find_itbl_entry(BtypeDefs, NumBtype, name);
	}

/***********************************************************************
*                                                                      *
*   g x B a r b S p e c   - invoke given barb spec                     *
*                                                                      *
***********************************************************************/

void	gxBarbSpec ( BSPEC *bspec )
	{
	static	LSTYLE	Solid = -1;

	if (Solid < 0) Solid = gxFindLineStyle("solid");

	gxSetColorIndex(bspec->colour, bspec->hilite);
	gxLineStyle(0, bspec->width/1000.0, 0.0);
	}
