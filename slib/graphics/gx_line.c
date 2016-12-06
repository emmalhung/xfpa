/***********************************************************************
*                                                                      *
*     g x _ l i n e . c                                                *
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

typedef	enum	{
				LtypeSolid,
				LtypeDash,
				LtypeDot,
				LtypeDashDot
				} LINE_STYLE_LIST;

static	const	ITBL	LstyleDefs[] =
							{
							LtypeSolid,		"solid",
							LtypeSolid,		"wide",
							LtypeDash,		"dash",
							LtypeDash,		"dashed",
							LtypeDot,		"dot",
							LtypeDot,		"dotted",
							LtypeDashDot,	"dash_dot"
							};
static	const	int		NumLstyle = ITBL_SIZE(LstyleDefs);

static	const	ITBL	LwidthDefs[] =
							{
							0,	"zero",
							0,	"thin",
							0,	"normal",
							3,	"medium",
							6,	"thick",
							9,	"fat"
							};
static	const	int		NumLwidth = ITBL_SIZE(LwidthDefs);

/***********************************************************************
*                                                                      *
*   g x F i n d L i n e S t y l e   - map line style names             *
*   g x F i n d L i n e W i d t h   - map line width names             *
*                                                                      *
***********************************************************************/

LSTYLE	gxFindLineStyle ( STRING name )
	{
	return (LSTYLE) find_itbl_entry(LstyleDefs, NumLstyle, name);
	}

float	gxFindLineWidth ( STRING name )
	{
	float   lwidth;
	STRING  p;
	
	/* Interpret as an actual number first */
	lwidth = (float) strtod(name, &p);
	if (blank(p)) return (lwidth);

	lwidth = (float) find_itbl_entry(LwidthDefs, NumLwidth, name);
	if (lwidth >= 0) return (lwidth);

	return -1.0;
	}

/***********************************************************************
*                                                                      *
*   g x L i n e S t y l e   - mapped line style                        *
*                                                                      *
***********************************************************************/

static	const	float	MinWidth  = 0;
static	const	float	MinLength = .02;

void	gxLineStyle ( LSTYLE lstyle, float width, float length )
	{
	int		nlist;
	float	dlist[50];

	width = MAX(width, MinWidth);
	glVdcLineWidth(width);

	length = MAX(length, MinLength);

	switch (lstyle)
		{
		case LtypeDash:		glLineStyle(glDASH | glCAP_BUTT);
							nlist = 0;
							dlist[nlist++] = .5*length;
							dlist[nlist++] = .5*length;
							glVdcDashStyle(nlist, dlist);
							break;

		case LtypeDot:		glLineStyle(glDASH | glCAP_ROUND);
							nlist = 0;
							dlist[nlist++] = 0;
							dlist[nlist++] = .5*length;
							dlist[nlist++] = 0;
							dlist[nlist++] = .5*length;
							glVdcDashStyle(nlist, dlist);
							break;

		case LtypeDashDot:	glLineStyle(glDASH | glCAP_BUTT);
							nlist = 0;
							dlist[nlist++] = .5*length;
							dlist[nlist++] = .2*length;
							dlist[nlist++] = .1*length;
							dlist[nlist++] = .2*length;
							glVdcDashStyle(nlist, dlist);
							break;

		case LtypeSolid:
		default:			glLineStyle(glSOLID | glCAP_BUTT);
		}
	}

/***********************************************************************
*                                                                      *
*   g x L i n e S p e c   - invoke given line spec                     *
*                                                                      *
***********************************************************************/

#undef LINE_PATTERN_SUPPORTED

void	gxLineSpec ( LSPEC *lspec )
	{
	gxLineStyle(lspec->style, lspec->width/1000.0, lspec->length/1000.0);
	gxSetColorIndex(lspec->colour, lspec->hilite);

#	ifdef LINE_PATTERN_SUPPORTED
	if (!blank(lspec->pattern))
		{
		}
#	endif
	}
