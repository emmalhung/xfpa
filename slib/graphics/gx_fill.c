/***********************************************************************
*                                                                      *
*     g x _ f i l l . c                                                *
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
				FstyleNone,
				FstyleSolid,
				FstylePattern,
				FstylePatternBg,
				FstyleTile,
				FstyleHatch,
				FstyleCrossHatch
				} FILL_STYLE_LIST;

static	const	ITBL	FstyleDefs[] =
							{
							FstyleNone,			"empty",
							FstyleNone,			"no_fill",
							FstyleNone,			"hollow",
							FstyleNone,			"hollow_fill",
							FstyleSolid,		"solid",
							FstyleSolid,		"solid_fill",
							FstylePattern,		"pattern",
							FstylePattern,		"pattern_fill",
							FstylePatternBg,	"pattern_bg",
							FstyleTile,			"tile",
							FstyleTile,			"tile_fill",
							FstyleHatch,		"hatch",
							FstyleHatch,		"hatch_fill",
							FstyleCrossHatch,	"cross_hatch",
							FstyleCrossHatch,	"cross_hatch_fill"
							};
static	const	int		NumFstyle = ITBL_SIZE(FstyleDefs);

/***********************************************************************
*                                                                      *
*   g x F i n d F i l l S t y l e   - map fill style names             *
*                                                                      *
***********************************************************************/

FSTYLE	gxFindFillStyle ( STRING name )
	{
	return (FSTYLE) find_itbl_entry(FstyleDefs, NumFstyle, name);
	}

/***********************************************************************
*                                                                      *
*   g x N e e d F i l l         - is it filled?                        *
*   g x N e e d P r e F i l l   - is it pre-filled?                    *
*   g x F i l l S t y l e       - set fill type                        *
*                                                                      *
***********************************************************************/

static	LOGICAL	NeedFill  = FALSE;

LOGICAL	gxNeedFill ( FSTYLE fstyle )
	{
	switch (fstyle)
		{
		case FstyleSolid:
		case FstylePattern:
		case FstylePatternBg:
		case FstyleTile:
		case FstyleHatch:	return TRUE;

		case FstyleNone:
		default:			return FALSE;
		}
	}

LOGICAL	gxNeedPreFill ( FSTYLE fstyle, HILITE hilite )
	{
	if (hilite >= 10) return TRUE;
	else              return FALSE;
	}

void	gxFillStyle ( FSTYLE fstyle, LOGICAL cross )
	{
	switch (fstyle)
		{
		case FstyleSolid:		glFillStyle(glPATTERN_SOLID);
								NeedFill  = TRUE;
								return;

		case FstylePattern:		glFillStyle(glPATTERN_TRANSPARENT);
								NeedFill  = TRUE;
								return;

		case FstylePatternBg:	glFillStyle(glPATTERN_BACKGROUND);
								NeedFill  = TRUE;
								return;

		case FstyleTile:		glFillStyle(glPATTERN_TILE);
								NeedFill  = TRUE;
								return;

		case FstyleHatch:		glFillStyle(glPATTERN_HATCH);
								NeedFill  = TRUE;
								if (!cross) return;

		case FstyleCrossHatch:	glFillStyle(glPATTERN_CROSS_HATCH);
								NeedFill  = TRUE;
								return;

		case FstyleNone:
		default:				NeedFill  = FALSE;
								return;
		}
	}

/***********************************************************************
*                                                                      *
*   g x F i l l P a t t e r n   - define fill bitmap pattern           *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*   g x F i l l S p e c   - invoke given fill spec                     *
*                                                                      *
***********************************************************************/

#undef FILL_PATTERN_SUPPORTED

void	gxFillSpec ( FSPEC *fspec, LOGICAL pre )
	{
	float	size, space, angle;
	HILITE	hilite;

	/* Handle highlight pre-fill if requested */
	if (pre)
		{
		hilite = fspec->hilite / 10;
		gxFillStyle(FstyleHatch, TRUE);
		gxSetColorIndex(fspec->colour, hilite);
		space = fspec->space / 2;
		angle = fspec->angle;
		if (fspec->style != FstyleHatch && fspec->style != FstyleCrossHatch)
			space = 10;
		}
	else
		{
		hilite = fspec->hilite % 10;
		gxFillStyle(fspec->style, fspec->cross);
		gxSetColorIndex(fspec->colour, hilite);
		space = fspec->space;
		angle = fspec->angle;
		}

	if (NeedFill && fspec->space > 0)
		{
		size = gxScaleSize(space/1000.0, fspec->scale);
		glVdcHatchSpacing(size);
		glHatchAngle(angle);
		}

#	ifdef FILL_PATTERN_SUPPORTED
	if (NeedFill && !blank(fspec->pattern))
		{
		glFillPattern(...);
		}
#	endif
	}
