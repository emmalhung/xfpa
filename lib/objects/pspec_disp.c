/*********************************************************************/
/** @file pspec_disp.c
 *
 * Routines to handle presentation specs.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*      p s p e c _ d i s p . c                                         *
*                                                                      *
*      Routines to handle presentation specs.                          *
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

#include "pspec.h"

#include <tools/tools.h>
#include <fpa_getmem.h>

#include <stdio.h>
#include <string.h>

/***********************************************************************
*                                                                      *
*      i n i t _ p s p e c                                             *
*      f r e e _ p s p e c                                             *
*      c o p y _ p s p e c                                             *
*      d e f i n e _ p s p e c _v a l u e                              *
*      r e c a l l _ p s p e c _v a l u e                              *
*                                                                      *
*      Routines to manage generic presentation specs.                  *
*                                                                      *
***********************************************************************/
/***********************************************************************/
/**	Initialize a presentation spec object.
 *
 * @param[in]	ptype	type of pspec.
 * @param[in]	pspec	pointer to object to initialize.
 ***********************************************************************/
void	init_pspec

	(
	PTYPE	ptype,
	PSPEC	pspec
	)

	{
	switch (ptype)
		{
		case PSPEC_LINE: init_lspec((LSPEC *)pspec); break;
		case PSPEC_FILL: init_fspec((FSPEC *)pspec); break;
		case PSPEC_TEXT: init_tspec((TSPEC *)pspec); break;
		case PSPEC_MARK: init_mspec((MSPEC *)pspec); break;
		case PSPEC_BARB: init_bspec((BSPEC *)pspec); break;
		}
	}

/***********************************************************************/
/**	Free a presentation spec object.
 *
 * @param[in]	ptype	type of pspec.
 * @param[in]	pspec	pointer to memory to be released.
 ***********************************************************************/
void	free_pspec

	(
	PTYPE	ptype,
	PSPEC	pspec
	)

	{
	switch (ptype)
		{
		case PSPEC_LINE: free_lspec((LSPEC *)pspec); break;
		case PSPEC_FILL: free_fspec((FSPEC *)pspec); break;
		case PSPEC_TEXT: free_tspec((TSPEC *)pspec); break;
		case PSPEC_MARK: free_mspec((MSPEC *)pspec); break;
		case PSPEC_BARB: free_bspec((BSPEC *)pspec); break;
		}
	}

/***********************************************************************/
/**	Make an exact copy of a presentation spec.
 * @param[in]	ptype	type of pspec.
 * @param[out]	pnew	pspec copy.
 * @param[in]	pspec	pspec original.
 ***********************************************************************/
void	copy_pspec

	(
	PTYPE		ptype,
	PSPEC		pnew,
	const PSPEC	pspec
	)

	{
	switch (ptype)
		{
		case PSPEC_LINE: copy_lspec((LSPEC *)pnew, (LSPEC *)pspec); break;
		case PSPEC_FILL: copy_fspec((FSPEC *)pnew, (FSPEC *)pspec); break;
		case PSPEC_TEXT: copy_tspec((TSPEC *)pnew, (TSPEC *)pspec); break;
		case PSPEC_MARK: copy_mspec((MSPEC *)pnew, (MSPEC *)pspec); break;
		case PSPEC_BARB: copy_bspec((BSPEC *)pnew, (BSPEC *)pspec); break;
		}
	}

/***********************************************************************/
/**	Set a presentation spec value.
 *
 * @param[in]	pspec	object to edit.
 * @param[in]	param	parameter to edit.
 * @param[in]	value	value to be set.
 ***********************************************************************/
void	define_pspec_value

	(
	PSPEC			pspec,
	PPARAM			param,
	const POINTER	value
	)

	{
	switch (param)
		{
		case LINE_COLOUR:
		case LINE_STYLE:
		case LINE_PATTERN:
		case LINE_SCALE:
		case LINE_WIDTH:
		case LINE_LENGTH:
		case LINE_HILITE:	define_lspec_value((LSPEC *)pspec, param, value);
							break;
		case FILL_COLOUR:
		case FILL_STYLE:
		case FILL_PATTERN:
		case FILL_CROSS:
		case FILL_SCALE:
		case FILL_SPACE:
		case FILL_ANGLE:
		case FILL_HILITE:	define_fspec_value((FSPEC *)pspec, param, value);
							break;
		case TEXT_COLOUR:
		case TEXT_FONT:
		case TEXT_SCALE:
		case TEXT_SIZE:
		case TEXT_ANGLE:
		case TEXT_XOFF:
		case TEXT_YOFF:
		case TEXT_HJUST:
		case TEXT_VJUST:
		case TEXT_HILITE:	define_tspec_value((TSPEC *)pspec, param, value);
							break;
		case MARK_COLOUR:
		case MARK_TYPE:
		case MARK_SYMBOL:
		case MARK_FONT:
		case MARK_SCALE:
		case MARK_SIZE:
		case MARK_ANGLE:
		case MARK_XOFF:
		case MARK_YOFF:
		case MARK_HJUST:
		case MARK_VJUST:
		case MARK_HILITE:	define_mspec_value((MSPEC *)pspec, param, value);
							break;

		case BARB_COLOUR:
		case BARB_TYPE:
		case BARB_SCALE:
		case BARB_WIDTH:
		case BARB_LENGTH:
		case BARB_ANGLE:
		case BARB_XOFF:
		case BARB_YOFF:
		case BARB_SENSE:
		case BARB_VALUE:
		case BARB_XVOFF:
		case BARB_YVOFF:
		case BARB_UNAME:
		case BARB_HILITE:	define_bspec_value((BSPEC *)pspec, param, value);
							break;
		}
	}

/***********************************************************************/
/**	Recall the value of a presentation spec parameter.
 *
 * @param[in]	pspec	object to examine.
 * @param[in]	param	name of parameter to lookup.
 * @param[out]	value	value of requested parameter.
 ***********************************************************************/
void	recall_pspec_value

	(
	PSPEC	pspec,
	PPARAM	param,
	POINTER	value
	)

	{
	switch (param)
		{
		case LINE_COLOUR:
		case LINE_STYLE:
		case LINE_PATTERN:
		case LINE_SCALE:
		case LINE_WIDTH:
		case LINE_LENGTH:
		case LINE_HILITE:	recall_lspec_value((LSPEC *)pspec, param, value);
							break;
		case FILL_COLOUR:
		case FILL_STYLE:
		case FILL_PATTERN:
		case FILL_CROSS:
		case FILL_SCALE:
		case FILL_SPACE:
		case FILL_ANGLE:
		case FILL_HILITE:	recall_fspec_value((FSPEC *)pspec, param, value);
							break;
		case TEXT_COLOUR:
		case TEXT_FONT:
		case TEXT_SCALE:
		case TEXT_SIZE:
		case TEXT_ANGLE:
		case TEXT_XOFF:
		case TEXT_YOFF:
		case TEXT_HJUST:
		case TEXT_VJUST:
		case TEXT_HILITE:	recall_tspec_value((TSPEC *)pspec, param, value);
							break;
		case MARK_COLOUR:
		case MARK_TYPE:
		case MARK_SYMBOL:
		case MARK_FONT:
		case MARK_SCALE:
		case MARK_SIZE:
		case MARK_ANGLE:
		case MARK_XOFF:
		case MARK_YOFF:
		case MARK_HJUST:
		case MARK_VJUST:
		case MARK_HILITE:	recall_mspec_value((MSPEC *)pspec, param, value);
							break;

		case BARB_COLOUR:
		case BARB_TYPE:
		case BARB_SCALE:
		case BARB_WIDTH:
		case BARB_LENGTH:
		case BARB_ANGLE:
		case BARB_XOFF:
		case BARB_YOFF:
		case BARB_SENSE:
		case BARB_VALUE:
		case BARB_XVOFF:
		case BARB_YVOFF:
		case BARB_UNAME:
		case BARB_HILITE:	recall_bspec_value((BSPEC *)pspec, param, value);
							break;
		}
	}

/***********************************************************************
*                                                                      *
*      i n i t _ l s p e c                                             *
*      s k i p _ l s p e c                                             *
*      f r e e _ l s p e c                                             *
*      c o p y _ l s p e c                                             *
*      s t r i n g _ l s p e c                                         *
*      d e f i n e _ l s p e c                                         *
*      r e c a l l _ l s p e c                                         *
*      d e f i n e _ l s p e c _v a l u e                              *
*      r e c a l l _ l s p e c _v a l u e                              *
*                                                                      *
*      Routines to manage line presentation specs.                     *
*                                                                      *
***********************************************************************/

void	init_lspec

	(
	LSPEC	*lspec
	)

	{
	/* Do nothing if structure does not exist */
	if (!lspec) return;

	/* Use reasonable initial values */
	lspec->colour  = SafeColour;
	lspec->style   = SafeLstyle;
	lspec->pattern = SafePattern;
	lspec->scale   = SafeScale;
	lspec->width   = SafeWidth;
	lspec->length  = SafeLength;
	lspec->hilite  = SafeHilite;
	}

void	skip_lspec

	(
	LSPEC	*lspec
	)

	{
	/* Do nothing if structure does not exist */
	if (!lspec) return;

	/* Reset all values to skip values */
	free_lspec(lspec);
	lspec->colour  = SkipColour;
	lspec->style   = SkipLstyle;
	lspec->pattern = SkipPattern;
	lspec->scale   = SkipScale;
	lspec->width   = SkipWidth;
	lspec->length  = SkipLength;
	lspec->hilite  = SkipHilite;
	}

void	free_lspec

	(
	LSPEC	*lspec
	)

	{
	/* Do nothing if structure does not exist */
	if (!lspec) return;

	/* Free any allocated members */
	FREEMEM(lspec->pattern);

	/* Give back an initialized one */
	init_lspec(lspec);
	}

void	copy_lspec

	(
	LSPEC		*lnew,
	const LSPEC	*lspec
	)

	{
	/* Do nothing if target does not exist */
	if (!lnew) return;

	/* If the source does not exist initialize the target */
	if (!lspec)
		{
		free_lspec(lnew);
		return;
		}

	/* Duplicate all values */
	define_lspec(lnew, lspec->colour, lspec->style, lspec->pattern,
				lspec->scale, lspec->width, lspec->length, lspec->hilite);
	}

void	string_lspec

	(
	LSPEC	*lspec,
	STRING	string
	)

	{
	STRING	splist, spec, spnam, spval;
	LOGICAL	status;

	/* Do nothing if structure does not exist */
	if (!lspec) return;

	/* Set to default values */
	init_lspec(lspec);

	/* Copy the string into a working spec list */
	/* and remove the surrounding square brackets */
	if (blank(string)) return;
	string = strchr(string, '[');	if (!string) return;
	string++;
	spec   = strchr(string, ']');	if (!spec)   return;
	*spec  = '\0';
	splist = strdup(string);
	*spec  = ']';

	/* Now interpret the individual specs in the spec list */
	while (opt_arg(splist, &spnam, &spval))
		{
		if (same(spnam, "colour"))
			{
			lspec->colour = find_colour(spval, &status);
			}
		else if (same(spnam, "style"))
			{
			lspec->style = find_lstyle(spval, &status);
			}
		else if (same(spnam, "pattern"))
			{
			lspec->pattern = STRMEM(lspec->pattern, spval);
			}
		else if (same(spnam, "scale"))
			{
			lspec->scale = find_poption(spval, "scale", &status);
			}
		else if (same(spnam, "width"))
			{
			lspec->width = find_lwidth(spval, &status);
			}
		else if (same(spnam, "length"))
			{
			lspec->length = float_arg(spval, &status);
			}
		else if (same(spnam, "hilite"))
			{
			lspec->hilite = int_arg(spval, &status);
			}
		}

	/* Clean up */
	FREEMEM(splist);
	}

void	define_lspec

	(
	LSPEC	*lspec,
	COLOUR	colour,
	LSTYLE	style,
	STRING	pattern,
	LOGICAL	scale,
	float	width,
	float	length,
	HILITE	hilite
	)

	{
	/* Do nothing if structure does not exist */
	if (!lspec) return;

	/* Use all the given values */
	if (colour  != SkipColour)  lspec->colour  = colour;
	if (style   != SkipLstyle)  lspec->style   = style;
	if (pattern != SkipPattern) lspec->pattern = STRMEM(lspec->pattern,pattern);
	if (scale   != SkipScale)   lspec->scale   = scale;
	if (width   != SkipWidth)   lspec->width   = width;
	if (length  != SkipLength)  lspec->length  = length;
	if (hilite  != SkipHilite)  lspec->hilite  = hilite;
	}

void	recall_lspec

	(
	LSPEC	*lspec,
	COLOUR	*colour,
	LSTYLE	*style,
	STRING	*pattern,
	LOGICAL	*scale,
	float	*width,
	float	*length,
	HILITE	*hilite
	)

	{
	/* Return only what was asked for */
	if (colour)  *colour  = (lspec) ? lspec->colour  : SafeColour;
	if (style)   *style   = (lspec) ? lspec->style   : SafeLstyle;
	if (pattern) *pattern = (lspec) ? lspec->pattern : NULL;
	if (scale)   *scale   = (lspec) ? lspec->scale   : SafeScale;
	if (width)   *width   = (lspec) ? lspec->width   : SafeWidth;
	if (length)  *length  = (lspec) ? lspec->length  : SafeLength;
	if (hilite)  *hilite  = (lspec) ? lspec->hilite  : SafeHilite;
	}

void	define_lspec_value

	(
	LSPEC			*lspec,
	PPARAM			param,
	const POINTER	value
	)

	{
	/* Do nothing if structure does not exist */
	if (!lspec) return;
	if (!value) return;

	/* Use the given value */
	switch (param)
		{
		case LINE_COLOUR:	lspec->colour  = *(COLOUR  *)value;		break;
		case LINE_STYLE:	lspec->style   = *(LSTYLE  *)value;		break;
		case LINE_PATTERN:	lspec->pattern = STRMEM(lspec->pattern,
											 *(STRING  *)value);	break;
		case LINE_SCALE:	lspec->scale   = *(LOGICAL *)value;		break;
		case LINE_WIDTH:	lspec->width   = *(float   *)value;		break;
		case LINE_LENGTH:	lspec->length  = *(float   *)value;		break;
		case LINE_HILITE:	lspec->hilite  = *(HILITE  *)value;		break;
		}
	}

void	recall_lspec_value

	(
	LSPEC	*lspec,
	PPARAM	param,
	POINTER	value
	)

	{
	/* Do nothing if structure does not exist */
	if (!lspec) return;
	if (!value) return;

	/* Return only what was asked for */
	switch (param)
		{
		case LINE_COLOUR:	*(COLOUR  *)value = lspec->colour;	break;
		case LINE_STYLE:	*(LSTYLE  *)value = lspec->style;	break;
		case LINE_PATTERN:	*(STRING  *)value = lspec->pattern;	break;
		case LINE_SCALE:	*(LOGICAL *)value = lspec->width;	break;
		case LINE_WIDTH:	*(float   *)value = lspec->width;	break;
		case LINE_LENGTH:	*(float   *)value = lspec->length;	break;
		case LINE_HILITE:	*(HILITE  *)value = lspec->hilite;	break;
		}
	}

/***********************************************************************
*                                                                      *
*      i n i t _ f s p e c                                             *
*      s k i p _ f s p e c                                             *
*      f r e e _ f s p e c                                             *
*      c o p y _ f s p e c                                             *
*      s t r i n g _ f s p e c                                         *
*      d e f i n e _ f s p e c                                         *
*      r e c a l l _ f s p e c                                         *
*      d e f i n e _ f s p e c _v a l u e                              *
*      r e c a l l _ f s p e c _v a l u e                              *
*                                                                      *
*      Routines to manage fill presentation specs.                     *
*                                                                      *
***********************************************************************/

void	init_fspec

	(
	FSPEC	*fspec
	)

	{
	/* Do nothing if structure does not exist */
	if (!fspec) return;

	/* Use reasonable initial values */
	fspec->colour  = SafeColour;
	fspec->style   = SafeFstyle;
	fspec->pattern = SafePattern;
	fspec->cross   = SafeCross;
	fspec->scale   = SafeScale;
	fspec->space   = SafeSpace;
	fspec->angle   = SafeAngle;
	fspec->hilite  = SafeHilite;
	}

void	skip_fspec

	(
	FSPEC	*fspec
	)

	{
	/* Do nothing if structure does not exist */
	if (!fspec) return;

	/* Reset all values to skip values */
	free_fspec(fspec);
	fspec->colour  = SkipColour;
	fspec->style   = SkipFstyle;
	fspec->pattern = SkipPattern;
	fspec->cross   = SkipCross;
	fspec->scale   = SkipScale;
	fspec->space   = SkipSpace;
	fspec->angle   = SkipAngle;
	fspec->hilite  = SkipHilite;
	}

void	free_fspec

	(
	FSPEC	*fspec
	)

	{
	/* Do nothing if structure does not exist */
	if (!fspec) return;

	/* Free any allocated members */
	FREEMEM(fspec->pattern);

	/* Give back an initialized one */
	init_fspec(fspec);
	}

void	copy_fspec

	(
	FSPEC		*fnew,
	const FSPEC	*fspec
	)

	{
	/* Do nothing if target does not exist */
	if (!fnew) return;

	/* If the source does not exist initialize the target */
	if (!fspec)
		{
		free_fspec(fnew);
		return;
		}

	/* Duplicate all values */
	define_fspec(fnew, fspec->colour, fspec->style, fspec->pattern,
					fspec->cross, fspec->scale, fspec->space, fspec->angle,
					fspec->hilite);
	}

void	string_fspec

	(
	FSPEC	*fspec,
	STRING	string
	)

	{
	STRING	splist, spec, spnam, spval;
	LOGICAL	status;

	/* Do nothing if structure does not exist */
	if (!fspec) return;

	/* Set to default values */
	init_fspec(fspec);

	/* Copy the string into a working spec list */
	/* and remove the surrounding square brackets */
	if (blank(string)) return;
	string = strchr(string, '[');	if (!string) return;
	string++;
	spec   = strchr(string, ']');	if (!spec)   return;
	*spec  = '\0';
	splist = strdup(string);
	*spec  = ']';

	/* Now interpret the individual specs in the spec list */
	while (opt_arg(splist, &spnam, &spval))
		{
		if (same(spnam, "colour"))
			{
			fspec->colour = find_colour(spval, &status);
			}
		else if (same(spnam, "style"))
			{
			fspec->style = find_fstyle(spval, &status);
			}
		else if (same(spnam, "pattern"))
			{
			fspec->pattern = STRMEM(fspec->pattern, spval);
			}
		else if (same(spnam, "cross"))
			{
			fspec->cross = find_poption(spval, "cross", &status);
			}
		else if (same(spnam, "scale"))
			{
			fspec->scale = find_poption(spval, "scale", &status);
			}
		else if (same(spnam, "space"))
			{
			fspec->space = find_size(spval, &status);
			}
		else if (same(spnam, "angle"))
			{
			fspec->angle = float_arg(spval, &status);
			}
		else if (same(spnam, "hilite"))
			{
			fspec->hilite = int_arg(spval, &status);
			}
		}

	/* Clean up */
	FREEMEM(splist);
	}

void	define_fspec

	(
	FSPEC	*fspec,
	COLOUR	colour,
	FSTYLE	style,
	STRING	pattern,
	LOGICAL	cross,
	LOGICAL	scale,
	float	space,
	float	angle,
	HILITE	hilite
	)

	{
	/* Do nothing if structure does not exist */
	if (!fspec) return;

	/* Use all the given values */
	if (colour  != SkipColour)  fspec->colour  = colour;
	if (style   != SkipFstyle)  fspec->style   = style;
	if (pattern != SkipPattern) fspec->pattern = STRMEM(fspec->pattern,pattern);
	if (cross   != SkipCross)   fspec->cross   = cross;
	if (scale   != SkipScale)   fspec->scale   = scale;
	if (space   != SkipScale)   fspec->space   = space;
	if (angle   != SkipAngle)   fspec->angle   = angle;
	if (hilite  != SkipHilite)  fspec->hilite  = hilite;
	}

void	recall_fspec

	(
	FSPEC	*fspec,
	COLOUR	*colour,
	FSTYLE	*style,
	STRING	*pattern,
	LOGICAL	*cross,
	LOGICAL	*scale,
	float	*space,
	float	*angle,
	HILITE	*hilite
	)

	{
	/* Return only what was asked for */
	if (colour)  *colour  = (fspec) ? fspec->colour  : SafeColour;
	if (style)   *style   = (fspec) ? fspec->style   : SafeFstyle;
	if (pattern) *pattern = (fspec) ? fspec->pattern : NULL;
	if (cross)   *cross   = (fspec) ? fspec->cross   : SafeCross;
	if (scale)   *scale   = (fspec) ? fspec->scale   : SafeScale;
	if (space)   *space   = (fspec) ? fspec->space   : SafeSpace;
	if (angle)   *angle   = (fspec) ? fspec->angle   : SafeAngle;
	if (hilite)  *hilite  = (fspec) ? fspec->hilite  : SafeHilite;
	}

void	define_fspec_value

	(
	FSPEC			*fspec,
	PPARAM			param,
	const POINTER	value
	)

	{
	/* Do nothing if structure does not exist */
	if (!fspec) return;
	if (!value) return;

	/* Use the given value */
	switch (param)
		{
		case FILL_COLOUR:	fspec->colour  = *(COLOUR  *)value;		break;
		case FILL_STYLE:	fspec->style   = *(FSTYLE  *)value;		break;
		case FILL_PATTERN:	fspec->pattern = STRMEM(fspec->pattern,
											 *(STRING  *)value);	break;
		case FILL_CROSS:	fspec->cross   = *(LOGICAL *)value;		break;
		case FILL_SCALE:	fspec->scale   = *(LOGICAL *)value;		break;
		case FILL_SPACE:	fspec->space   = *(float   *)value;		break;
		case FILL_ANGLE:	fspec->angle   = *(float   *)value;		break;
		case FILL_HILITE:	fspec->hilite  = *(HILITE  *)value;		break;
		}
	}

void	recall_fspec_value

	(
	FSPEC	*fspec,
	PPARAM	param,
	POINTER	value
	)

	{
	/* Do nothing if structure does not exist */
	if (!fspec) return;
	if (!value) return;

	/* Return only what was asked for */
	switch (param)
		{
		case FILL_COLOUR:	*(COLOUR  *)value = fspec->colour;	break;
		case FILL_STYLE:	*(FSTYLE  *)value = fspec->style;	break;
		case FILL_PATTERN:	*(STRING  *)value = fspec->pattern;	break;
		case FILL_CROSS:	*(LOGICAL *)value = fspec->cross;	break;
		case FILL_SCALE:	*(LOGICAL *)value = fspec->scale;	break;
		case FILL_SPACE:	*(float   *)value = fspec->space;	break;
		case FILL_ANGLE:	*(float   *)value = fspec->angle;	break;
		case FILL_HILITE:	*(HILITE  *)value = fspec->hilite;	break;
		}
	}

/***********************************************************************
*                                                                      *
*      i n i t _ t s p e c                                             *
*      s k i p _ t s p e c                                             *
*      f r e e _ t s p e c                                             *
*      c o p y _ t s p e c                                             *
*      s t r i n g _ t s p e c                                         *
*      d e f i n e _ t s p e c                                         *
*      r e c a l l _ t s p e c                                         *
*      d e f i n e _ t s p e c _v a l u e                              *
*      r e c a l l _ t s p e c _v a l u e                              *
*                                                                      *
*      Routines to manage text presentation specs.                     *
*                                                                      *
***********************************************************************/

void	init_tspec

	(
	TSPEC	*tspec
	)

	{
	/* Do nothing if structure does not exist */
	if (!tspec) return;

	/* Use reasonable initial values */
	tspec->colour  = SafeColour;
	tspec->tcolour = SafeShadow;
	tspec->bcolour = SafeShadow;
	tspec->font    = SafeFont;
	tspec->scale   = SafeScale;
	tspec->size    = SafeTsize;
	tspec->angle   = SafeAngle;
	tspec->xoff    = SafeVoff;
	tspec->yoff    = SafeVoff;
	tspec->hjust   = SafeHjust;
	tspec->vjust   = SafeVjust;
	tspec->hilite  = SafeHilite;
	}

void	skip_tspec

	(
	TSPEC	*tspec
	)

	{
	/* Do nothing if structure does not exist */
	if (!tspec) return;

	/* Reset all values to skip values */
	free_tspec(tspec);
	tspec->colour  = SkipColour;
	tspec->tcolour = SkipShadow;
	tspec->bcolour = SkipShadow;
	tspec->font    = SkipFont;
	tspec->scale   = SkipScale;
	tspec->size    = SkipTsize;
	tspec->angle   = SkipAngle;
	tspec->xoff    = SkipVoff;
	tspec->yoff    = SkipVoff;
	tspec->hjust   = SkipHjust;
	tspec->vjust   = SkipVjust;
	tspec->hilite  = SkipHilite;
	}

void	free_tspec

	(
	TSPEC	*tspec
	)

	{
	/* Do nothing if structure does not exist */
	if (!tspec) return;

	/* Give back an initialized one */
	init_tspec(tspec);
	}

void	copy_tspec

	(
	TSPEC		*tnew,
	const TSPEC	*tspec
	)

	{
	/* Do nothing if target does not exist */
	if (!tnew) return;

	/* If the source does not exist initialize the target */
	if (!tspec)
		{
		free_tspec(tnew);
		return;
		}

	/* Duplicate all values */
	define_tspec(tnew, tspec->colour, tspec->font, tspec->scale, tspec->size,
					tspec->angle, tspec->hjust, tspec->vjust, tspec->hilite);
	if (tspec->tcolour != SkipShadow) tnew->tcolour = tspec->tcolour;
	if (tspec->bcolour != SkipShadow) tnew->bcolour = tspec->bcolour;
	if (tspec->xoff    != SkipVoff)   tnew->xoff    = tspec->xoff;
	if (tspec->yoff    != SkipVoff)   tnew->yoff    = tspec->yoff;
	}

void	string_tspec

	(
	TSPEC	*tspec,
	STRING	string
	)

	{
	STRING	splist, spec, spnam, spval;
	LOGICAL	status;

	/* Do nothing if structure does not exist */
	if (!tspec) return;

	/* Set to default values */
	init_tspec(tspec);

	/* Copy the string into a working spec list */
	/* and remove the surrounding square brackets */
	if (blank(string)) return;
	string = strchr(string, '[');	if (!string) return;
	string++;
	spec   = strchr(string, ']');	if (!spec)   return;
	*spec  = '\0';
	splist = strdup(string);
	*spec  = ']';

	/* Now interpret the individual specs in the spec list */
	while (opt_arg(splist, &spnam, &spval))
		{
		if (same(spnam, "colour"))
			{
			tspec->colour = find_colour(spval, &status);
			}
		else if (same(spnam, "tcolour"))
			{
			tspec->tcolour = find_colour(spval, &status);
			}
		else if (same(spnam, "bcolour"))
			{
			tspec->bcolour = find_colour(spval, &status);
			}
		else if (same(spnam, "font"))
			{
			tspec->font = find_font(spval, &status);
			}
		else if (same(spnam, "scale"))
			{
			tspec->scale = find_poption(spval, "scale", &status);
			}
		else if (same(spnam, "size"))
			{
			tspec->size = find_size(spval, &status);
			}
		else if (same(spnam, "angle"))
			{
			tspec->angle = float_arg(spval, &status);
			}
		else if (same(spnam, "xoff"))
			{
			tspec->xoff = float_arg(spval, &status);
			}
		else if (same(spnam, "yoff"))
			{
			tspec->yoff = float_arg(spval, &status);
			}
		else if (same(spnam, "hjust"))
			{
			tspec->hjust = spval[0];
			}
		else if (same(spnam, "vjust"))
			{
			tspec->vjust = spval[0];
			}
		else if (same(spnam, "hilite"))
			{
			tspec->hilite = int_arg(spval, &status);
			}
		}

	/* Clean up */
	FREEMEM(splist);
	}

void	define_tspec

	(
	TSPEC	*tspec,
	COLOUR	colour,
	FONT	font,
	LOGICAL	scale,
	float	size,
	float	angle,
	HJUST	hjust,
	VJUST	vjust,
	HILITE	hilite
	)

	{
	/* Do nothing if structure does not exist */
	if (!tspec) return;

	/* Use all the given values */
	if (colour != SkipColour) tspec->colour = colour;
	if (font   != SkipFont)   tspec->font   = font;
	if (scale  != SkipScale)  tspec->scale  = scale;
	if (size   != SkipTsize)  tspec->size   = size;
	if (angle  != SkipAngle)  tspec->angle  = angle;
	if (hjust  != SkipHjust)  tspec->hjust  = hjust;
	if (vjust  != SkipVjust)  tspec->vjust  = vjust;
	if (hilite != SkipHilite) tspec->hilite = hilite;
	}

void	recall_tspec

	(
	TSPEC	*tspec,
	COLOUR	*colour,
	FONT	*font,
	LOGICAL	*scale,
	float	*size,
	float	*angle,
	HJUST	*hjust,
	VJUST	*vjust,
	HILITE	*hilite
	)

	{
	/* Return only what was asked for */
	if (colour) *colour = (tspec) ? tspec->colour : SafeColour;
	if (font)   *font   = (tspec) ? tspec->font   : 0;
	if (scale)  *scale  = (tspec) ? tspec->scale  : SafeScale;
	if (size)   *size   = (tspec) ? tspec->size   : SafeTsize;
	if (angle)  *angle  = (tspec) ? tspec->angle  : SafeAngle;
	if (hjust)  *hjust  = (tspec) ? tspec->hjust  : SafeHjust;
	if (vjust)  *vjust  = (tspec) ? tspec->vjust  : SafeVjust;
	if (hilite) *hilite = (tspec) ? tspec->hilite : SafeHilite;
	}

void	define_tspec_value

	(
	TSPEC			*tspec,
	PPARAM			param,
	const POINTER	value
	)

	{
	/* Do nothing if structure does not exist */
	if (!tspec) return;
	if (!value) return;

	/* Use the given value */
	switch (param)
		{
		case TEXT_COLOUR:	tspec->colour  = *(COLOUR  *)value;	break;
		case TEXT_TCOLOUR:	tspec->tcolour = *(COLOUR  *)value;	break;
		case TEXT_BCOLOUR:	tspec->bcolour = *(COLOUR  *)value;	break;
		case TEXT_FONT:		tspec->font    = *(FONT    *)value;	break;
		case TEXT_SCALE:	tspec->scale   = *(LOGICAL *)value;	break;
		case TEXT_SIZE:		tspec->size    = *(float   *)value;	break;
		case TEXT_ANGLE:	tspec->angle   = *(float   *)value;	break;
		case TEXT_XOFF:		tspec->xoff    = *(float   *)value;	break;
		case TEXT_YOFF:		tspec->yoff    = *(float   *)value;	break;
		case TEXT_HJUST:	tspec->hjust   = *(HJUST   *)value;	break;
		case TEXT_VJUST:	tspec->vjust   = *(VJUST   *)value;	break;
		case TEXT_HILITE:	tspec->hilite  = *(HILITE  *)value;	break;
		}
	}

void	recall_tspec_value

	(
	TSPEC	*tspec,
	PPARAM	param,
	POINTER	value
	)

	{
	/* Do nothing if structure does not exist */
	if (!tspec) return;
	if (!value) return;

	/* Return only what was asked for */
	switch (param)
		{
		case TEXT_COLOUR:	*(COLOUR  *)value = tspec->colour;	break;
		case TEXT_TCOLOUR:	*(COLOUR  *)value = tspec->tcolour;	break;
		case TEXT_BCOLOUR:	*(COLOUR  *)value = tspec->bcolour;	break;
		case TEXT_FONT:		*(FONT    *)value = tspec->font;	break;
		case TEXT_SCALE:	*(LOGICAL *)value = tspec->scale;	break;
		case TEXT_SIZE:		*(float   *)value = tspec->size;	break;
		case TEXT_ANGLE:	*(float   *)value = tspec->angle;	break;
		case TEXT_XOFF:		*(float   *)value = tspec->xoff;	break;
		case TEXT_YOFF:		*(float   *)value = tspec->yoff;	break;
		case TEXT_HJUST:	*(HJUST   *)value = tspec->hjust;	break;
		case TEXT_VJUST:	*(VJUST   *)value = tspec->vjust;	break;
		case TEXT_HILITE:	*(HILITE  *)value = tspec->hilite;	break;
		}
	}

/***********************************************************************
*                                                                      *
*      i n i t _ m s p e c                                             *
*      s k i p _ m s p e c                                             *
*      f r e e _ m s p e c                                             *
*      c o p y _ m s p e c                                             *
*      s t r i n g _ m s p e c                                         *
*      d e f i n e _ m s p e c                                         *
*      r e c a l l _ m s p e c                                         *
*      d e f i n e _ m s p e c _v a l u e                              *
*      r e c a l l _ m s p e c _v a l u e                              *
*                                                                      *
*      Routines to manage marker presentation specs.                   *
*                                                                      *
***********************************************************************/

void	init_mspec

	(
	MSPEC	*mspec
	)

	{
	/* Do nothing if structure does not exist */
	if (!mspec) return;

	/* Use reasonable initial values */
	mspec->colour  = SafeColour;
	mspec->tcolour = SafeShadow;
	mspec->bcolour = SafeShadow;
	mspec->type    = SafeMtype;
	mspec->symbol  = SafeSymbol;
	mspec->font    = SafeFont;
	mspec->scale   = SafeScale;
	mspec->size    = SafeMsize;
	mspec->angle   = SafeAngle;
	mspec->xoff    = SafeVoff;
	mspec->yoff    = SafeVoff;
	mspec->hjust   = SafeHjust;
	mspec->vjust   = SafeVjust;
	mspec->hilite  = SafeHilite;
	}

void	skip_mspec

	(
	MSPEC	*mspec
	)

	{
	/* Do nothing if structure does not exist */
	if (!mspec) return;

	/* Reset all values to skip values */
	free_mspec(mspec);
	mspec->colour  = SkipColour;
	mspec->tcolour = SkipShadow;
	mspec->bcolour = SkipShadow;
	mspec->type    = SkipMtype;
	mspec->symbol  = SkipSymbol;
	mspec->font    = SkipFont;
	mspec->scale   = SkipScale;
	mspec->size    = SkipMsize;
	mspec->angle   = SkipAngle;
	mspec->xoff    = SkipVoff;
	mspec->yoff    = SkipVoff;
	mspec->hjust   = SkipHjust;
	mspec->vjust   = SkipVjust;
	mspec->hilite  = SkipHilite;
	}

void	free_mspec

	(
	MSPEC	*mspec
	)

	{
	/* Do nothing if structure does not exist */
	if (!mspec) return;

	/* Free any allocated members */
	FREEMEM(mspec->symbol);

	/* Give back an initialized one */
	init_mspec(mspec);
	}

void	copy_mspec

	(
	MSPEC		*mnew,
	const MSPEC	*mspec
	)

	{
	/* Do nothing if target does not exist */
	if (!mnew) return;

	/* If the source does not exist initialize the target */
	if (!mspec)
		{
		free_mspec(mnew);
		return;
		}

	/* Duplicate all values */
	define_mspec(mnew, mspec->colour, mspec->type, mspec->symbol, mspec->font,
					mspec->scale, mspec->size, mspec->angle, mspec->hilite);
	if (mspec->hjust   != SkipHjust)  mnew->hjust   = mspec->hjust;
	if (mspec->vjust   != SkipVjust)  mnew->vjust   = mspec->vjust;
	if (mspec->tcolour != SkipShadow) mnew->tcolour = mspec->tcolour;
	if (mspec->bcolour != SkipShadow) mnew->bcolour = mspec->bcolour;
	if (mspec->xoff    != SkipVoff)   mnew->xoff    = mspec->xoff;
	if (mspec->yoff    != SkipVoff)   mnew->yoff    = mspec->yoff;
	}

void	string_mspec

	(
	MSPEC	*mspec,
	STRING	string
	)

	{
	STRING	splist, spec, spnam, spval;
	LOGICAL	status;

	/* Do nothing if structure does not exist */
	if (!mspec) return;

	/* Set to default values */
	init_mspec(mspec);

	/* Copy the string into a working spec list */
	/* and remove the surrounding square brackets */
	if (blank(string)) return;
	string = strchr(string, '[');	if (!string) return;
	string++;
	spec   = strchr(string, ']');	if (!spec)   return;
	*spec  = '\0';
	splist = strdup(string);
	*spec  = ']';

	/* Now interpret the individual specs in the spec list */
	while (opt_arg(splist, &spnam, &spval))
		{
		if (same(spnam, "colour"))
			{
			mspec->colour = find_colour(spval, &status);
			}
		else if (same(spnam, "tcolour"))
			{
			mspec->tcolour = find_colour(spval, &status);
			}
		else if (same(spnam, "bcolour"))
			{
			mspec->bcolour = find_colour(spval, &status);
			}
		else if (same(spnam, "type"))
			{
			mspec->type = find_mtype(spval, &status);
			}
		else if (same(spnam, "symbol"))
			{
			mspec->symbol = STRMEM(mspec->symbol, spval);
			}
		else if (same(spnam, "font"))
			{
			mspec->font = find_font(spval, &status);
			}
		else if (same(spnam, "scale"))
			{
			mspec->scale = find_poption(spval, "scale", &status);
			}
		else if (same(spnam, "size"))
			{
			mspec->size = find_size(spval, &status);
			}
		else if (same(spnam, "angle"))
			{
			mspec->angle = float_arg(spval, &status);
			}
		else if (same(spnam, "xoff"))
			{
			mspec->xoff = float_arg(spval, &status);
			}
		else if (same(spnam, "yoff"))
			{
			mspec->yoff = float_arg(spval, &status);
			}
		else if (same(spnam, "hjust"))
			{
			mspec->hjust = spval[0];
			}
		else if (same(spnam, "vjust"))
			{
			mspec->vjust = spval[0];
			}
		else if (same(spnam, "hilite"))
			{
			mspec->hilite = int_arg(spval, &status);
			}
		}

	/* Clean up */
	FREEMEM(splist);
	}

void	define_mspec

	(
	MSPEC	*mspec,
	COLOUR	colour,
	MTYPE	type,
	STRING	symbol,
	FONT	font,
	LOGICAL	scale,
	float	size,
	float	angle,
	HILITE	hilite
	)

	{
	/* Do nothing if structure does not exist */
	if (!mspec) return;

	/* Use all the given values */
	if (colour != SkipColour) mspec->colour = colour;
	if (type   != SkipMtype)  mspec->type   = type;
	if (symbol != SkipSymbol) mspec->symbol = STRMEM(mspec->symbol,symbol);
	if (font   != SkipFont)   mspec->font   = font;
	if (scale  != SkipScale)  mspec->scale  = scale;
	if (size   != SkipMsize)  mspec->size   = size;
	if (angle  != SkipAngle)  mspec->angle  = angle;
	if (hilite != SkipHilite) mspec->hilite = hilite;
	}

void	recall_mspec

	(
	MSPEC	*mspec,
	COLOUR	*colour,
	MTYPE	*type,
	STRING	*symbol,
	FONT	*font,
	LOGICAL	*scale,
	float	*size,
	float	*angle,
	HILITE	*hilite
	)

	{
	/* Return only what was asked for */
	if (colour) *colour = (mspec) ? mspec->colour : SafeColour;
	if (type)   *type   = (mspec) ? mspec->type   : SafeMtype;
	if (symbol) *symbol = (mspec) ? mspec->symbol : NULL;
	if (font)   *font   = (mspec) ? mspec->font   : 0;
	if (scale)  *scale  = (mspec) ? mspec->scale  : SafeScale;
	if (size)   *size   = (mspec) ? mspec->size   : SafeMsize;
	if (angle)  *angle  = (mspec) ? mspec->angle  : SafeAngle;
	if (hilite) *hilite = (mspec) ? mspec->hilite : SafeHilite;
	}

void	define_mspec_value

	(
	MSPEC			*mspec,
	PPARAM			param,
	const POINTER	value
	)

	{
	/* Do nothing if structure does not exist */
	if (!mspec) return;
	if (!value) return;

	/* Use the given value */
	switch (param)
		{
		case MARK_COLOUR:	mspec->colour  = *(COLOUR  *)value;		break;
		case MARK_TCOLOUR:	mspec->tcolour = *(COLOUR  *)value;		break;
		case MARK_BCOLOUR:	mspec->bcolour = *(COLOUR  *)value;		break;
		case MARK_TYPE:		mspec->type    = *(MTYPE   *)value;		break;
		case MARK_SYMBOL:	mspec->symbol  = STRMEM(mspec->symbol,
											 *(STRING  *)value);	break;
		case MARK_FONT:		mspec->font    = *(FONT    *)value;		break;
		case MARK_SCALE:	mspec->scale   = *(LOGICAL *)value;		break;
		case MARK_SIZE:		mspec->size    = *(float   *)value;		break;
		case MARK_ANGLE:	mspec->angle   = *(float   *)value;		break;
		case MARK_XOFF:		mspec->xoff    = *(float   *)value;		break;
		case MARK_YOFF:		mspec->yoff    = *(float   *)value;		break;
		case MARK_HJUST:	mspec->hjust   = *(HJUST   *)value;		break;
		case MARK_VJUST:	mspec->vjust   = *(VJUST   *)value;		break;
		case MARK_HILITE:	mspec->hilite  = *(HILITE  *)value;		break;
		}
	}

void	recall_mspec_value

	(
	MSPEC	*mspec,
	PPARAM	param,
	POINTER	value
	)

	{
	/* Do nothing if structure does not exist */
	if (!mspec) return;
	if (!value) return;

	/* Return only what was asked for */
	switch (param)
		{
		case MARK_COLOUR:	*(COLOUR  *)value = mspec->colour;	break;
		case MARK_TCOLOUR:	*(COLOUR  *)value = mspec->tcolour;	break;
		case MARK_BCOLOUR:	*(COLOUR  *)value = mspec->bcolour;	break;
		case MARK_TYPE:		*(MTYPE   *)value = mspec->type;	break;
		case MARK_SYMBOL:	*(STRING  *)value = mspec->symbol;	break;
		case MARK_FONT:		*(FONT    *)value = mspec->font;	break;
		case MARK_SCALE:	*(LOGICAL *)value = mspec->scale;	break;
		case MARK_SIZE:		*(float   *)value = mspec->size;	break;
		case MARK_ANGLE:	*(float   *)value = mspec->angle;	break;
		case MARK_XOFF:		*(float   *)value = mspec->xoff;	break;
		case MARK_YOFF:		*(float   *)value = mspec->yoff;	break;
		case MARK_HJUST:	*(HJUST   *)value = mspec->hjust;	break;
		case MARK_VJUST:	*(VJUST   *)value = mspec->vjust;	break;
		case MARK_HILITE:	*(HILITE  *)value = mspec->hilite;	break;
		}
	}

/***********************************************************************
*                                                                      *
*      i n i t _ b s p e c                                             *
*      s k i p _ b s p e c                                             *
*      f r e e _ b s p e c                                             *
*      c o p y _ b s p e c                                             *
*      s t r i n g _ b s p e c                                         *
*      d e f i n e _ b s p e c                                         *
*      r e c a l l _ b s p e c                                         *
*      d e f i n e _ b s p e c _v a l u e                              *
*      r e c a l l _ b s p e c _v a l u e                              *
*                                                                      *
*      Routines to manage barb presentation specs.                     *
*                                                                      *
***********************************************************************/

void	init_bspec

	(
	BSPEC	*bspec
	)

	{
	/* Do nothing if structure does not exist */
	if (!bspec) return;

	/* Use reasonable initial values */
	bspec->colour  = SafeColour;
	bspec->type    = SafeBtype;
	bspec->scale   = SafeScale;
	bspec->width   = SafeWidth;
	bspec->length  = SafeLength;
	bspec->angle   = SafeAngle;
	bspec->xoff    = SafeVoff;
	bspec->yoff    = SafeVoff;
	bspec->sense   = SafeSense;
	bspec->value   = SafeValue;
	bspec->xvoff   = SafeVoff;
	bspec->yvoff   = SafeVoff;
	bspec->uname   = SafeUname;
	bspec->hilite  = SafeHilite;
	}

void	skip_bspec

	(
	BSPEC	*bspec
	)

	{
	/* Do nothing if structure does not exist */
	if (!bspec) return;

	/* Reset all values to skip values */
	free_bspec(bspec);
	bspec->colour  = SkipColour;
	bspec->type    = SkipBtype;
	bspec->scale   = SkipScale;
	bspec->width   = SkipWidth;
	bspec->length  = SkipLength;
	bspec->angle   = SkipAngle;
	bspec->xoff    = SkipVoff;
	bspec->yoff    = SkipVoff;
	bspec->sense   = SkipSense;
	bspec->value   = SkipValue;
	bspec->xvoff   = SkipVoff;
	bspec->yvoff   = SkipVoff;
	bspec->uname   = SkipUname;
	bspec->hilite  = SkipHilite;
	}

void	free_bspec

	(
	BSPEC	*bspec
	)

	{
	/* Do nothing if structure does not exist */
	if (!bspec) return;

	/* Free any allocated members */
	FREEMEM(bspec->uname);

	/* Give back an initialized one */
	init_bspec(bspec);
	}

void	copy_bspec

	(
	BSPEC		*bnew,
	const BSPEC	*bspec
	)

	{
	/* Do nothing if target does not exist */
	if (!bnew) return;

	/* If the source does not exist initialize the target */
	if (!bspec)
		{
		free_bspec(bnew);
		return;
		}

	/* Duplicate all values */
	define_bspec(bnew, bspec->colour, bspec->type, bspec->scale, bspec->width,
					bspec->length, bspec->sense, bspec->value, bspec->xvoff,
					bspec->yvoff, bspec->uname, bspec->hilite);
	if (bspec->angle != SkipAngle) bnew->angle = bspec->angle;
	if (bspec->xoff  != SkipVoff)  bnew->xoff  = bspec->xoff;
	if (bspec->yoff  != SkipVoff)  bnew->yoff  = bspec->yoff;
	}

void	string_bspec

	(
	BSPEC	*bspec,
	STRING	string
	)

	{
	STRING	splist, spec, spnam, spval;
	LOGICAL	status;

	/* Do nothing if structure does not exist */
	if (!bspec) return;

	/* Set to default values */
	init_bspec(bspec);

	/* Copy the string into a working spec list */
	/* and remove the surrounding square brackets */
	if (blank(string)) return;
	string = strchr(string, '[');	if (!string) return;
	string++;
	spec   = strchr(string, ']');	if (!spec)   return;
	*spec  = '\0';
	splist = strdup(string);
	*spec  = ']';

	/* Now interpret the individual specs in the spec list */
	while (opt_arg(splist, &spnam, &spval))
		{
		if (same(spnam, "colour"))
			{
			bspec->colour = find_colour(spval, &status);
			}
		else if (same(spnam, "type"))
			{
			bspec->type = find_btype(spval, &status);
			}
		else if (same(spnam, "scale"))
			{
			bspec->scale = find_poption(spval, "scale", &status);
			}
		else if (same(spnam, "width"))
			{
			bspec->width = find_lwidth(spval, &status);
			}
		else if (same(spnam, "length"))
			{
			bspec->length = float_arg(spval, &status);
			}
		else if (same(spnam, "angle"))
			{
			bspec->angle = float_arg(spval, &status);
			}
		else if (same(spnam, "xoff"))
			{
			bspec->xoff = float_arg(spval, &status);
			}
		else if (same(spnam, "yoff"))
			{
			bspec->yoff = float_arg(spval, &status);
			}
		else if (same(spnam, "sense"))
			{
			bspec->sense = find_poption(spval, "sense", &status);
			}
		else if (same(spnam, "value"))
			{
			bspec->value = find_poption(spval, "value", &status);
			}
		else if (same(spnam, "xvoff"))
			{
			bspec->xvoff = float_arg(spval, &status);
			}
		else if (same(spnam, "yvoff"))
			{
			bspec->yvoff = float_arg(spval, &status);
			}
		else if (same(spnam, "uname"))
			{
			bspec->uname = STRMEM(bspec->uname, spval);
			}
		else if (same(spnam, "hilite"))
			{
			bspec->hilite = int_arg(spval, &status);
			}
		}

	/* Clean up */
	FREEMEM(splist);
	}

void	define_bspec

	(
	BSPEC	*bspec,
	COLOUR	colour,
	BTYPE	type,
	LOGICAL	scale,
	float	width,
	float	length,
	LOGICAL	sense,
	LOGICAL	value,
	float	xvoff,
	float	yvoff,
	STRING	uname,
	HILITE	hilite
	)

	{
	/* Do nothing if structure does not exist */
	if (!bspec) return;

	/* Use all the given values */
	if (colour  != SkipColour)  bspec->colour  = colour;
	if (type    != SkipBtype)   bspec->type    = type;
	if (scale   != SkipScale)   bspec->scale   = scale;
	if (width   != SkipWidth)   bspec->width   = width;
	if (length  != SkipLength)  bspec->length  = length;
	if (sense   != SkipSense)   bspec->sense   = sense;
	if (value   != SkipValue)   bspec->value   = value;
	if (xvoff   != SkipVoff)    bspec->xvoff   = xvoff;
	if (yvoff   != SkipVoff)    bspec->yvoff   = yvoff;
	if (uname   != SkipUname)   bspec->uname   = STRMEM(bspec->uname,uname);
	if (hilite  != SkipHilite)  bspec->hilite  = hilite;
	}

void	recall_bspec

	(
	BSPEC	*bspec,
	COLOUR	*colour,
	BTYPE	*type,
	LOGICAL	*scale,
	float	*width,
	float	*length,
	LOGICAL	*sense,
	LOGICAL	*value,
	float	*xvoff,
	float	*yvoff,
	STRING	*uname,
	HILITE	*hilite
	)

	{
	/* Return only what was asked for */
	if (colour)  *colour  = (bspec) ? bspec->colour  : SafeColour;
	if (type)    *type    = (bspec) ? bspec->type    : 0;
	if (scale)   *scale   = (bspec) ? bspec->scale   : SafeScale;
	if (width)   *width   = (bspec) ? bspec->width   : SafeWidth;
	if (length)  *length  = (bspec) ? bspec->length  : SafeLength;
	if (sense)   *sense   = (bspec) ? bspec->sense   : SafeSense;
	if (value)   *value   = (bspec) ? bspec->value   : SafeValue;
	if (xvoff)   *xvoff   = (bspec) ? bspec->xvoff   : SafeVoff;
	if (yvoff)   *yvoff   = (bspec) ? bspec->yvoff   : SafeVoff;
	if (uname)   *uname   = (bspec) ? bspec->uname   : NULL;
	if (hilite)  *hilite  = (bspec) ? bspec->hilite  : SafeHilite;
	}

void	define_bspec_value

	(
	BSPEC			*bspec,
	PPARAM			param,
	const POINTER	value
	)

	{
	/* Do nothing if structure does not exist */
	if (!bspec) return;
	if (!value) return;

	/* Use the given value */
	switch (param)
		{
		case BARB_COLOUR:	bspec->colour  = *(COLOUR  *)value;		break;
		case BARB_TYPE:		bspec->type    = *(BTYPE   *)value;		break;
		case BARB_SCALE:	bspec->scale   = *(LOGICAL *)value;		break;
		case BARB_WIDTH:	bspec->width   = *(float   *)value;		break;
		case BARB_LENGTH:	bspec->length  = *(float   *)value;		break;
		case BARB_ANGLE:	bspec->angle   = *(float   *)value;		break;
		case BARB_XOFF:		bspec->xoff    = *(float   *)value;		break;
		case BARB_YOFF:		bspec->yoff    = *(float   *)value;		break;
		case BARB_SENSE:	bspec->sense   = *(LOGICAL *)value;		break;
		case BARB_VALUE:	bspec->value   = *(LOGICAL *)value;		break;
		case BARB_XVOFF:	bspec->xvoff   = *(float   *)value;		break;
		case BARB_YVOFF:	bspec->yvoff   = *(float   *)value;		break;
		case BARB_UNAME:	bspec->uname   = STRMEM(bspec->uname,
											 *(STRING  *)value);	break;
		case BARB_HILITE:	bspec->hilite  = *(HILITE  *)value;		break;
		}
	}

void	recall_bspec_value

	(
	BSPEC	*bspec,
	PPARAM	param,
	POINTER	value
	)

	{
	/* Do nothing if structure does not exist */
	if (!bspec) return;
	if (!value) return;

	/* Return only what was asked for */
	switch (param)
		{
		case BARB_COLOUR:	*(COLOUR  *)value = bspec->colour;	break;
		case BARB_TYPE:		*(BTYPE   *)value = bspec->type;	break;
		case BARB_SCALE:	*(LOGICAL *)value = bspec->scale;	break;
		case BARB_WIDTH:	*(float   *)value = bspec->width;	break;
		case BARB_LENGTH:	*(float   *)value = bspec->length;	break;
		case BARB_ANGLE:	*(float   *)value = bspec->angle;	break;
		case BARB_XOFF:		*(float   *)value = bspec->xoff;	break;
		case BARB_YOFF:		*(float   *)value = bspec->yoff;	break;
		case BARB_SENSE:	*(LOGICAL *)value = bspec->sense;	break;
		case BARB_VALUE:	*(LOGICAL *)value = bspec->value;	break;
		case BARB_XVOFF:	*(float   *)value = bspec->xvoff;	break;
		case BARB_YVOFF:	*(float   *)value = bspec->yvoff;	break;
		case BARB_UNAME:	*(STRING  *)value = bspec->uname;	break;
		case BARB_HILITE:	*(HILITE  *)value = bspec->hilite;	break;
		}
	}

/***********************************************************************
*                                                                      *
*      i n i t _ c o n s p e c                                         *
*      f r e e _ c o n s p e c                                         *
*      c o p y _ c o n s p e c                                         *
*      d e f i n e _ c o n s p e c _ r a n g e                         *
*      d e f i n e _ c o n s p e c _ l i s t                           *
*      d e f i n e _ c o n s p e c _ v e c t o r                       *
*      d e f i n e _ c o n s p e c _ s p e c i a l                     *
*      a d d _ c v a l _ t o _ c o n s p e c                           *
*                                                                      *
*      Routines to manage contour presentation specs.                  *
*                                                                      *
***********************************************************************/

void	init_conspec

	(
	CONSPEC	*cspec
	)

	{
	/* Do nothing if structure does not exist */
	if (!cspec) return;

	/* Initialize the contour line/text/mark specs */
	init_lspec(&cspec->lspec);
	init_fspec(&cspec->fspec);
	init_tspec(&cspec->tspec);
	init_bspec(&cspec->bspec);
	init_mspec(&cspec->mspec);

	/* Use reasonable initial values */
	cspec->type  = NULL;
	cspec->vmult = 1;
	cspec->cmin  = 0.0;
	cspec->cmax  = 0.0;
	cspec->cstd  = 0.0;
	cspec->cint  = 0.0;
	cspec->nval  = 0;
	cspec->cvals = NULL;
	cspec->clabs = NULL;
	}

void	free_conspec

	(
	CONSPEC	*cspec
	)

	{
	/* Do nothing if structure does not exist */
	if (!cspec) return;

	/* Free the contour line/text/mark specs */
	free_lspec(&cspec->lspec);
	free_fspec(&cspec->fspec);
	free_tspec(&cspec->tspec);
	free_bspec(&cspec->bspec);
	free_mspec(&cspec->mspec);

	/* Free optional label list */
	FREELIST(cspec->clabs, cspec->nval);
	cspec->nval = 0;

	/* Free other allocated members */
	FREEMEM(cspec->type);
	FREEMEM(cspec->cvals);
	FREEMEM(cspec->clabs);

	/* Give back an initialized one */
	init_conspec(cspec);
	}

void	copy_conspec

	(
	CONSPEC			*cnew,
	const CONSPEC	*cspec
	)

	{
	/* Do nothing if target does not exist */
	if (!cnew) return;

	/* If the source does not exist initialize the target */
	if (!cspec)
		{
		free_conspec(cnew);
		return;
		}

	/* Duplicate a range or list contour spec */
	/* Otherwise return the target to an initialized state */
	if ( same(cspec->type, "range") )
		define_conspec_range(cnew, cspec->cmin, cspec->cmax, cspec->cstd,
								cspec->cint);
	else if ( same(cspec->type, "list") )
		define_conspec_list(cnew, cspec->nval, cspec->cvals, cspec->clabs);
	else if ( same(cspec->type, "vector") )
		define_conspec_vector(cnew, cspec->vmult);
	else if ( same(cspec->type, "maxima") )
		define_conspec_special(cnew, cspec->type, cspec->cmin, cspec->cmax);
	else if ( same(cspec->type, "minima") )
		define_conspec_special(cnew, cspec->type, cspec->cmin, cspec->cmax);
	else if ( same(cspec->type, "saddle") )
		define_conspec_special(cnew, cspec->type, cspec->cmin, cspec->cmax);
	else
		free_conspec(cnew);

	/* Duplicate the contour line/text/mark specs */
	skip_lspec(&cnew->lspec);
	skip_fspec(&cnew->fspec);
	skip_tspec(&cnew->tspec);
	skip_bspec(&cnew->bspec);
	skip_mspec(&cnew->mspec);
	copy_lspec(&cnew->lspec, &cspec->lspec);
	copy_fspec(&cnew->fspec, &cspec->fspec);
	copy_tspec(&cnew->tspec, &cspec->tspec);
	copy_bspec(&cnew->bspec, &cspec->bspec);
	copy_mspec(&cnew->mspec, &cspec->mspec);
	}

void	define_conspec_range

	(
	CONSPEC	*cspec,
	float	cmin,
	float	cmax,
	float	cstd,
	float	cint
	)

	{
	/* Do nothing if structure does not exist */
	if (!cspec) return;

	/* Return to an initialized state */
	free_conspec(cspec);

	/* Set type to "range" */
	/* Use given range parameters */
	/* Empty explicit contour value list */
	cspec->type  = SETSTR(cspec->type, "range");
	cspec->vmult = 0;
	cspec->cmin  = cmin;
	cspec->cmax  = cmax;
	cspec->cstd  = cstd;
	cspec->cint  = fabs(cint);
	cspec->nval  = 0;
	cspec->cvals = NULL;
	cspec->clabs = NULL;
	}

void	define_conspec_list

	(
	CONSPEC			*cspec,
	int				nval,
	const float		*cvals,
	const STRING	*clabs
	)

	{
	int		i;
	float	cval;
	STRING	clab;

	/* Do nothing if structure does not exist */
	if (!cspec) return;

	/* Return to an initialized state */
	free_conspec(cspec);

	/* Set type to "list" */
	/* Use null range parameters */
	/* Start with empty explicit contour value list */
	cspec->type  = SETSTR(cspec->type, "list");
	cspec->vmult = 0;
	cspec->cmin  = 0;
	cspec->cmax  = 0;
	cspec->cstd  = 0;
	cspec->cint  = 0;
	cspec->nval  = 0;
	cspec->cvals = NULL;
	cspec->clabs = NULL;

	/* If a contour value list is given, build a list */
	if (!cvals) return;
	for (i=0; i<nval; i++)
		{
		cval = cvals[i];
		clab = (clabs)? clabs[i]: NULL;
		add_cval_to_conspec(cspec, cval, clab);
		}
	}

void	define_conspec_vector

	(
	CONSPEC	*cspec,
	int		vmult
	)

	{
	/* Do nothing if structure does not exist */
	if (!cspec) return;

	/* Return to an initialized state */
	free_conspec(cspec);

	/* Set type to "vector" */
	/* Use null range parameters */
	/* Empty explicit contour value list */
	cspec->type  = SETSTR(cspec->type, "vector");
	cspec->vmult = vmult;
	cspec->cmin  = 0;
	cspec->cmax  = 0;
	cspec->cstd  = 0;
	cspec->cint  = 0;
	cspec->nval  = 0;
	cspec->cvals = NULL;
	cspec->clabs = NULL;
	}

void	define_conspec_special

	(
	CONSPEC	*cspec,
	STRING	type,
	float	cmin,
	float	cmax
	)

	{
	int		OK=FALSE;

	/* Do nothing if structure does not exist */
	if (!cspec) return;

	/* Return to an initialized state */
	free_conspec(cspec);

	/* Make sure we know the given type */
	OK |= same(type, "maxima");
	OK |= same(type, "minima");
	OK |= same(type, "saddle");
	if (!OK) return;

	/* Set type to the given type */
	/* Use given range parameters */
	/* Empty explicit contour value list */
	cspec->type  = STRMEM(cspec->type, type);
	cspec->vmult = 0;
	cspec->cmin  = cmin;
	cspec->cmax  = cmax;
	cspec->cstd  = 0;
	cspec->cint  = 0;
	cspec->nval  = 0;
	cspec->cvals = NULL;
	cspec->clabs = NULL;
	}

void	add_cval_to_conspec

	(
	CONSPEC	*cspec,
	float	cval,
	STRING	clab
	)

	{
	int		n;

	/* Do nothing if structure does not exist */
	if (!cspec)                     return;
	if (!same(cspec->type, "list")) return;

	/* Increment the size of the explicit contour value and label lists */
	/* and insert the new value at the end */
	n = cspec->nval++;
	cspec->cvals    = GETMEM(cspec->cvals, float, cspec->nval);
	cspec->clabs    = GETMEM(cspec->clabs, STRING, cspec->nval);
	cspec->cvals[n] = cval;
	cspec->clabs[n] = (clab)? INITSTR(clab): NULL;
	}

/***********************************************************************
*                                                                      *
*      i n i t _ c a t s p e c                                         *
*      f r e e _ c a t s p e c                                         *
*      c o p y _ c a t s p e c                                         *
*      d e f i n e _ c a t s p e c                                     *
*                                                                      *
*      Routines to manage category presentation specs.                 *
*                                                                      *
***********************************************************************/

void	init_catspec

	(
	CATSPEC	*cspec
	)

	{
	/* Do nothing if structure does not exist */
	if (!cspec) return;

	/* Initialize the category line/text/fill specs */
	init_lspec(&cspec->lspec);
	init_tspec(&cspec->tspec);
	init_fspec(&cspec->fspec);
	init_mspec(&cspec->mspec);
	init_bspec(&cspec->bspec);

	/* Use reasonable initial values */
	cspec->mclass  = NULL;
	cspec->name    = NULL;
	cspec->type    = NULL;
	cspec->cat     = NULL;
	cspec->val     = NULL;
	cspec->attrib  = NULL;
	copy_point(cspec->offset, ZeroPoint);
	cspec->angle   = 0.0;
	}

void	free_catspec

	(
	CATSPEC	*cspec
	)

	{
	/* Do nothing if structure does not exist */
	if (!cspec) return;

	/* Free the category line/text/fill specs */
	free_lspec(&cspec->lspec);
	free_tspec(&cspec->tspec);
	free_fspec(&cspec->fspec);
	free_mspec(&cspec->mspec);
	free_bspec(&cspec->bspec);

	/* Free other allocated members */
	FREEMEM(cspec->mclass);
	FREEMEM(cspec->name);
	FREEMEM(cspec->type);
	FREEMEM(cspec->cat);
	FREEMEM(cspec->val);
	FREEMEM(cspec->attrib);

	/* Give back an initialized one */
	init_catspec(cspec);
	}

void	copy_catspec

	(
	CATSPEC			*cnew,
	const CATSPEC	*cspec
	)

	{
	/* Do nothing if target does not exist */
	if (!cnew) return;

	/* If the source does not exist initialize the target */
	if (!cspec)
		{
		free_catspec(cnew);
		return;
		}

	/* Duplicate category type and name */
	define_catspec(cnew, cspec->mclass, cspec->name, cspec->type, cspec->cat,
			cspec->val, cspec->attrib, cspec->offset, cspec->angle);

	/* Duplicate the category line/text/fill specs */
	skip_lspec(&cnew->lspec);
	skip_fspec(&cnew->fspec);
	skip_tspec(&cnew->tspec);
	skip_mspec(&cnew->mspec);
	skip_bspec(&cnew->bspec);
	copy_lspec(&cnew->lspec, &cspec->lspec);
	copy_tspec(&cnew->tspec, &cspec->tspec);
	copy_fspec(&cnew->fspec, &cspec->fspec);
	copy_mspec(&cnew->mspec, &cspec->mspec);
	copy_bspec(&cnew->bspec, &cspec->bspec);
	}

void	define_catspec

	(
	CATSPEC		*cspec,
	STRING		class,
	STRING		name,
	STRING		type,
	STRING		cat,
	STRING		val,
	STRING		attrib,
	const POINT	offset,
	float		angle
	)

	{
	/* Do nothing if structure does not exist */
	if (!cspec) return;

	/* Return to an initialized state */
	free_catspec(cspec);

	/* Set category type and name */
	cspec->mclass  = STRMEM(cspec->mclass, class);
	cspec->name    = STRMEM(cspec->name,   name);
	cspec->type    = STRMEM(cspec->type,   type);
	cspec->cat     = STRMEM(cspec->cat,    cat);
	cspec->val     = STRMEM(cspec->val,    val);
	cspec->attrib  = STRMEM(cspec->attrib, attrib);
	if (offset) copy_point(cspec->offset, offset);
	else        copy_point(cspec->offset, ZeroPoint);
	cspec->angle   = angle;
	}

/***********************************************************************
*                                                                      *
*      i n i t _ p l t s p e c                                         *
*      f r e e _ p l t s p e c                                         *
*      c o p y _ p l t s p e c                                         *
*      d e f i n e _ p l t s p e c                                     *
*                                                                      *
*      Routines to manage plot presentation specs.                     *
*                                                                      *
***********************************************************************/

void	init_pltspec

	(
	PLTSPEC	*cspec
	)

	{
	/* Do nothing if structure does not exist */
	if (!cspec) return;

	/* Initialize the category line/text/fill specs */
	init_lspec(&cspec->lspec);
	init_tspec(&cspec->tspec);
	init_fspec(&cspec->fspec);
	init_mspec(&cspec->mspec);
	init_bspec(&cspec->bspec);

	/* Use reasonable initial values */
	cspec->type = NULL;
	cspec->name = NULL;
	copy_point(cspec->offset, ZeroPoint);
	cspec->angle = 0.0;
	}

void	free_pltspec

	(
	PLTSPEC	*cspec
	)

	{
	/* Do nothing if structure does not exist */
	if (!cspec) return;

	/* Free the category line/text/fill specs */
	free_lspec(&cspec->lspec);
	free_tspec(&cspec->tspec);
	free_fspec(&cspec->fspec);
	free_mspec(&cspec->mspec);
	free_bspec(&cspec->bspec);

	/* Free other allocated members */
	FREEMEM(cspec->type);
	FREEMEM(cspec->name);

	/* Give back an initialized one */
	init_pltspec(cspec);
	}

void	copy_pltspec

	(
	PLTSPEC			*cnew,
	const PLTSPEC	*cspec
	)

	{
	/* Do nothing if target does not exist */
	if (!cnew) return;

	/* If the source does not exist initialize the target */
	if (!cspec)
		{
		free_pltspec(cnew);
		return;
		}

	/* Duplicate category type and name */
	define_pltspec(cnew, cspec->type, cspec->name, cspec->offset, cspec->angle);

	/* Duplicate the category line/text/fill specs */
	skip_lspec(&cnew->lspec);
	skip_fspec(&cnew->fspec);
	skip_tspec(&cnew->tspec);
	skip_mspec(&cnew->mspec);
	skip_bspec(&cnew->bspec);
	copy_lspec(&cnew->lspec, &cspec->lspec);
	copy_tspec(&cnew->tspec, &cspec->tspec);
	copy_fspec(&cnew->fspec, &cspec->fspec);
	copy_mspec(&cnew->mspec, &cspec->mspec);
	copy_bspec(&cnew->bspec, &cspec->bspec);
	}

void	define_pltspec

	(
	PLTSPEC		*cspec,
	STRING		type,
	STRING		name,
	const POINT	offset,
	float		angle
	)

	{
	/* Do nothing if structure does not exist */
	if (!cspec) return;

	/* Return to an initialized state */
	free_pltspec(cspec);

	/* Set category type and name */
	cspec->type = STRMEM(cspec->type, type);
	cspec->name = STRMEM(cspec->name, name);
	if (offset) copy_point(cspec->offset, offset);
	else        copy_point(cspec->offset, ZeroPoint);
	cspec->angle = angle;
	}

#ifdef DEBUG_PSPEC

/***********************************************************************
*                                                                      *
*      d e b u g _ l s p e c                                           *
*      d e b u g _ f s p e c                                           *
*      d e b u g _ t s p e c                                           *
*      d e b u g _ m s p e c                                           *
*      d e b u g _ b s p e c                                           *
*      d e b u g _ c o n s p e c                                       *
*      d e b u g _ c a t s p e c                                       *
*      d e b u g _ p l t s p e c                                       *
*                                                                      *
*      Routines to display presentation specs.                         *
*                                                                      *
***********************************************************************/

void	debug_lspec(LSPEC	*lspec ,
					STRING	msg ,
					int		indent	)

{
	int		i;
	char	ind[256];

	for (i=0; i<indent; i++)
		{
		ind[i] = ' ';
		}
	ind[indent] = '\0';

	(void) printf("%s",ind);
	if (!blank(msg)) (void) printf("%s:",msg);
	if (lspec)
		{
		(void) printf(" colour:%d style:%d width:%g pattern:%s hilite:%d",
				lspec->colour, lspec->style, lspec->width, lspec->pattern,
				lspec->hilite);
		}
	(void) printf("\n");
	}

void	debug_fspec(FSPEC	*fspec ,
					STRING	msg ,
					int		indent	)

{
	int		i;
	char	ind[256];

	for (i=0; i<indent; i++)
		{
		ind[i] = ' ';
		}
	ind[indent] = '\0';

	(void) printf("%s",ind);
	if (!blank(msg)) (void) printf("%s:",msg);
	if (fspec)
		{
		(void) printf(" colour:%d style:%d pattern:%s cross:%d scale:%d space:%g angle:%g hilite:%d",
				fspec->colour, fspec->style, fspec->pattern, fspec->cross,
				fspec->scale, fspec->space, fspec->angle, fspec->hilite);
		}
	(void) printf("\n");
	}

void	debug_tspec(TSPEC	*tspec ,
					STRING	msg ,
					int		indent	)

{
	int		i;
	char	ind[256];

	for (i=0; i<indent; i++)
		{
		ind[i] = ' ';
		}
	ind[indent] = '\0';

	(void) printf("%s",ind);
	if (!blank(msg)) (void) printf("%s:",msg);
	if (tspec)
		{
		(void) printf(" colour:%d font:%d size:%g angle:%g hjust:%c vjust:%c hilite:%d",
				tspec->colour, tspec->font, tspec->size, tspec->angle,
				tspec->hjust, tspec->vjust, tspec->hilite);
		}
	(void) printf("\n");
	}

void	debug_mspec(MSPEC	*mspec ,
					STRING	msg ,
					int		indent	)

{
	int		i;
	char	ind[256];

	for (i=0; i<indent; i++)
		{
		ind[i] = ' ';
		}
	ind[indent] = '\0';

	(void) printf("%s",ind);
	if (!blank(msg)) (void) printf("%s:",msg);
	if (mspec)
		{
		(void) printf(" colour:%d type:%d symbol:'%s' font:%d size:%g angle:%g hjust:%c vjust:%c hilite:%d",
				mspec->colour, mspec->type, mspec->symbol, mspec->font,
				mspec->size, mspec->angle, mspec->hjust, mspec->vjust,
				mspec->hilite);
		}
	(void) printf("\n");
	}

void	debug_bspec(BSPEC	*bspec ,
					STRING	msg ,
					int		indent	)

{
	int		i;
	char	ind[256];

	for (i=0; i<indent; i++)
		{
		ind[i] = ' ';
		}
	ind[indent] = '\0';

	(void) printf("%s",ind);
	if (!blank(msg)) (void) printf("%s:",msg);
	if (bspec)
		{
		(void) printf(" colour:%d type:%d width:%g length:%g xvoff:%g yvoff:%g uname:'%s' hilite:%d",
				bspec->colour, bspec->type, bspec->width, bspec->length,
				bspec->xvoff, bspec->yvoff, bspec->uname, bspec->hilite);
		}
	(void) printf("\n");
	}

void	debug_conspec(CONSPEC	*cspec ,
					  STRING	msg ,
					  int		indent	)

{
	int		i;
	char	ind[256];

	for (i=0; i<indent; i++)
		{
		ind[i] = ' ';
		}
	ind[indent] = '\0';

	(void) printf("%s",ind);
	if (!blank(msg)) (void) printf("%s:",msg);
	if (cspec)
		{
		(void) printf(" type:%s vmult:%d cmin/cmax:%g~%g cstd/cint:%g~%g nval:%d",
				cspec->type, cspec->vmult, cspec->cmin, cspec->cmax,
				cspec->cstd, cspec->cint, cspec->nval);
		}
	(void) printf("\n");
	if (cspec) debug_lspec(&cspec->lspec,"lspec",indent+3);
	if (cspec) debug_fspec(&cspec->fspec,"fspec",indent+3);
	if (cspec) debug_tspec(&cspec->tspec,"tspec",indent+3);
	if (cspec) debug_bspec(&cspec->bspec,"bspec",indent+3);
	if (cspec) debug_mspec(&cspec->mspec,"mspec",indent+3);
	}

void	debug_catspec(CATSPEC	*cspec ,
					  STRING	msg ,
					  int		indent	)

{
	int		i;
	char	ind[256];

	for (i=0; i<indent; i++)
		{
		ind[i] = ' ';
		}
	ind[indent] = '\0';

	if (cspec)
		{
		(void) printf("%s",ind);
		if (!blank(msg)) (void) printf("%s:",msg);
		(void) printf("  type:%s  mclass:%s  name:%s",
					cspec->type, cspec->mclass, cspec->name);
		(void) printf("\n");
		(void) printf("%s",ind);
		if (!blank(msg)) (void) printf("%s:",msg);
		(void) printf("  cat:%s  val:%s  attrib:%s  offset:(%g,%g)  angle:%g",
					cspec->cat, cspec->val, cspec->attrib,
					cspec->offset[X], cspec->offset[Y], cspec->angle);
		(void) printf("\n");
		}
	else
		{
		(void) printf("%s",ind);
		if (!blank(msg)) (void) printf("%s:",msg);
		(void) printf("\n");
		}
	if (cspec) debug_lspec(&cspec->lspec,"lspec",indent+3);
	if (cspec) debug_fspec(&cspec->fspec,"fspec",indent+3);
	if (cspec) debug_tspec(&cspec->tspec,"tspec",indent+3);
	if (cspec) debug_mspec(&cspec->mspec,"mspec",indent+3);
	if (cspec) debug_bspec(&cspec->bspec,"bspec",indent+3);
	}

void	debug_pltspec(PLTSPEC	*cspec ,
					  STRING	msg ,
					  int		indent	)

{
	int		i;
	char	ind[256];

	for (i=0; i<indent; i++)
		{
		ind[i] = ' ';
		}
	ind[indent] = '\0';

	(void) printf("%s",ind);
	if (!blank(msg)) (void) printf("%s:",msg);
	if (cspec)
		{
		(void) printf(" type:%s name:%s offset:(%g,%g) angle:%g",
				cspec->type, cspec->name, cspec->offset[X], cspec->offset[Y],
				cspec->angle);
		}
	(void) printf("\n");
	if (cspec) debug_lspec(&cspec->lspec,"lspec",indent+3);
	if (cspec) debug_fspec(&cspec->fspec,"fspec",indent+3);
	if (cspec) debug_tspec(&cspec->tspec,"tspec",indent+3);
	if (cspec) debug_mspec(&cspec->mspec,"mspec",indent+3);
	if (cspec) debug_bspec(&cspec->bspec,"bspec",indent+3);
	}

#endif /* DEBUG_PSPEC */
