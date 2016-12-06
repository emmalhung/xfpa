/***********************************************************************
*                                                                      *
*     g x _ c o l o u r . c                                            *
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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <Xu.h>

#include "gx.h"

/***********************************************************************
*                                                                      *
*     g x F i n d C o l o u r   - Map a colour name to a pixel value   *
*                                                                      *
***********************************************************************/

COLOUR	gxFindColour
	
	(
	STRING	name
	)

	{
	/**********************************************************************
	*                                                                     *
	* This function identifies the given colour 'name' and returns the    *
	* corresponding index (pixel) value.  Three different forms of colour *
	* 'name' are supported:                                               *
	*                                                                     *
	* 1 - Direct (index) form (e.g. "=123") specifies the index value     *
	*     directly.                                                       *
	*                                                                     *
	* 2 - Hexadecimal (RGB) form (e.g. "#000FFF000") specifies the        *
	*     desired red/green/blue values for the requested colour.         *
	*                                                                     *
	* 3 - X-color name (e.g. MediumAquamarine) specifies a name that is   *
	*     known in the standard X color file, which in turn defines the   *
	*     red/green/blue values for the requested colour.                 *
	*                                                                     *
	**********************************************************************/
	COLOUR	colour;

	colour = find_direct_colour(name);
	if (colour >= 0) return colour;

	colour = (COLOUR) glSetColor(name);
	return colour;
	}

/***********************************************************************
*                                                                      *
*     g x C o l o r I n d e x   - Map colour with highlight to an      *
*                                 allocated pixel value                *
*                                                                      *
*     g x S e t C o l o r I n d e x   - Set mapped pixel value as the  *
*                                       current colour                 *
*                                                                      *
***********************************************************************/

Pixel	gxColorIndex

	(
	COLOUR	colour,	/* requested colour index */
	HILITE	hilite	/* requested highlight level */
	)

	{
	static	Pixel	H1 = (Pixel) 0;
	static	Pixel	H2 = (Pixel) 0;
	static	Pixel	H3 = (Pixel) 0;
	static	Pixel	H4 = (Pixel) 0;
	static	Pixel	H5 = (Pixel) 0;
	static	LOGICAL	Defined = FALSE;

	STRING	cname;

	if (!Defined)
		{
		cname = XuGetStringResource(".ingred.editHighlight",   "White");
		H1    = glSetColor(cname);
		cname = XuGetStringResource(".ingred.pickHighlight",   "Yellow");
		H2    = glSetColor(cname);
		cname = XuGetStringResource(".ingred.actionHighlight", "Cyan");
		H3    = glSetColor(cname);
		cname = XuGetStringResource(".ingred.mergeHighlight",  "Green");
		H4    = glSetColor(cname);
		cname = XuGetStringResource(".ingred.fillHighlight",   "LightYellow");
		H5    = glSetColor(cname);
		Defined = TRUE;
		}

	switch (hilite)
		{
		case 0:		return (Pixel) colour;
		case 1:		return H1;
		case 2:		return H2;
		case 3:		return H3;
		case 4:		return H4;
		case 5:		return H5;
		default:	return (Pixel) SafeColour;
		}
	}

/**********************************************************************/

void	gxSetColorIndex

	(
	COLOUR colour,
	HILITE hilite
	)

	{
	glSetColorIndex( gxColorIndex(colour, hilite) );
	}
