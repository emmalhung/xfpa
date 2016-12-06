/*********************************************************************/
/** @file pspec_attrib.c
 *
 * Routines to handle presentation specs.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*      p s p e c _ a t t r i b . c                                     *
*                                                                      *
*      Routines to handle presentation specs.                          *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2002 Environment Canada (MSC)            *
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

#define PSPEC_INIT
#include "pspec.h"

#include <tools/tools.h>

#include <stdio.h>
#include <stdlib.h>

/***********************************************************************
*                                                                      *
*      p r o v i d e _ c o l o u r _ f u n c t i o n                   *
*      p r o v i d e _ l s t y l e _ f u n c t i o n                   *
*      p r o v i d e _ l w i d t h _ f u n c t i o n                   *
*      p r o v i d e _ f s t y l e _ f u n c t i o n                   *
*      p r o v i d e _ m t y p e _ f u n c t i o n                     *
*      p r o v i d e _ f o n t _ f u n c t i o n                       *
*      p r o v i d e _ s i z e _ f u n c t i o n                       *
*      p r o v i d e _ b t y p e _ f u n c t i o n                     *
*      p r o v i d e _ p o p t i o n _ f u n c t i o n                 *
*                                                                      *
***********************************************************************/

/* Supplied functions for translating various attributes */
static	COLOUR	(*ColourFunction)  (STRING) = find_direct_colour;
static	LSTYLE	(*LstyleFunction)  (STRING) = 0;
static	float	(*LwidthFunction)  (STRING) = 0;
static	FSTYLE	(*FstyleFunction)  (STRING) = 0;
static	MTYPE	(*MtypeFunction)   (STRING) = 0;
static	FONT	(*FontFunction)    (STRING) = 0;
static	float	(*SizeFunction)    (STRING) = 0;
static	BTYPE	(*BtypeFunction)   (STRING) = 0;
static	LOGICAL	(*PoptionFunction) (STRING, STRING) = 0;

void	provide_colour_function
	( COLOUR (*function)(STRING) )
	{ ColourFunction = function; }

void	provide_lstyle_function
	( LSTYLE (*function)(STRING) )
	{ LstyleFunction = function; }

void	provide_lwidth_function
	( float (*function)(STRING) )
	{ LwidthFunction = function; }

void	provide_fstyle_function
	( FSTYLE (*function)(STRING) )
	{ FstyleFunction = function; }

void	provide_mtype_function
	( MTYPE (*function)(STRING) )
	{ MtypeFunction = function; }

void	provide_font_function
	( FONT (*function)(STRING) )
	{ FontFunction = function; }

void	provide_size_function
	( float (*function)(STRING) )
	{ SizeFunction = function; }

void	provide_btype_function
	( BTYPE (*function)(STRING) )
	{ BtypeFunction = function; }

void	provide_poption_function
	( LOGICAL (*function)(STRING, STRING) )
	{ PoptionFunction = function; }

/***********************************************************************
*                                                                      *
*      f i n d _ d i r e c t _ c o l o u r                             *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Find a colour def by name.
 *
 * @param[in]	name	name of colour
 * @return colour object.
 ***********************************************************************/
COLOUR	find_direct_colour

	(
	STRING	name
	)

	{
	COLOUR	colour;
	STRING	p;

	if (blank(name)) return SafeColour;

	if (name[0] == '=')
		{
		colour = (COLOUR) strtol(name+1, &p, 10);
		if (p != name+1) return colour;
		}

	return (COLOUR) (-1);
	}

/***********************************************************************
*                                                                      *
*      f i n d _ c o l o u r                                           *
*      f i n d _ l s t y l e                                           *
*      f i n d _ l w i d t h                                           *
*      f i n d _ f s t y l e                                           *
*      f i n d _ m t y p e                                             *
*      f i n d _ f o n t                                               *
*      f i n d _ s i z e                                               *
*      f i n d _ o f f s e t                                           *
*      f i n d _ b t y p e                                             *
*      f i n d _ p o p t i o n                                         *
*                                                                      *
*      Routines to convert presentation parameter names to values.     *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Convert presentation parameter colour name to value.
 *
 * @param[in]	name	name of colour.
 * @param[out]	*status did we identify the colour correctly?
 * @return pointer to colour item.
 ***********************************************************************/
COLOUR	find_colour

	(
	STRING	name,
	LOGICAL	*status
	)

	{
	COLOUR	colour;

	if (ColourFunction)
		{
		colour = ColourFunction(name);
		if (colour >= 0)
			{
			if (status) *status = TRUE;
			return colour;
			}
		}

	if (status) *status = FALSE;
	return SafeColour;
	}

/**********************************************************************/

/***********************************************************************/
/**	Convert presentation parameter line style name to value.
 *
 * @param[in]	name	name of line style.
 * @param[out]	*status did we identify the line style correctly?
 * @return pointer to lstyle item.
 ***********************************************************************/
LSTYLE	find_lstyle

	(
	STRING	name,
	LOGICAL	*status
	)

	{
	LSTYLE	lstyle;

	if (LstyleFunction)
		{
		lstyle = LstyleFunction(name);
		if (lstyle >= 0)
			{
			if (status) *status = TRUE;
			return lstyle;
			}
		}

	if (status) *status = FALSE;
	return SafeLstyle;
	}

/**********************************************************************/

/***********************************************************************/
/**	Convert presentation parameter line width name to value.
 *
 * @param[in]	name	name of line width.
 * @param[out]	*status did we identify the line width correctly?
 * @return line width value.
 ***********************************************************************/
float	find_lwidth

	(
	STRING	name,
	LOGICAL	*status
	)

	{
	float	lwidth;

	if (LwidthFunction)
		{
		lwidth = LwidthFunction(name);
		if (lwidth >= 0)
			{
			if (status) *status = TRUE;
			return lwidth;
			}
		}

	if (status) *status = FALSE;
	return SafeWidth;
	}

/**********************************************************************/

/***********************************************************************/
/**	Convert presentation parameter fstyle name to value.
 *
 * @param[in]	name	name of fstyle.
 * @param[out]	*status did we identify the fstyle correctly?
 * @return pointer to fstyle item.
 ***********************************************************************/
FSTYLE	find_fstyle

	(
	STRING	name,
	LOGICAL	*status
	)

	{
	FSTYLE	fstyle;

	if (FstyleFunction)
		{
		fstyle = FstyleFunction(name);
		if (fstyle >= 0)
			{
			if (status) *status = TRUE;
			return fstyle;
			}
		}

	if (status) *status = FALSE;
	return SafeFstyle;
	}

/**********************************************************************/

/***********************************************************************/
/**	Convert presentation parameter mtype name to value.
 *
 * @param[in]	name	name of mtype.
 * @param[out]	*status did we identify the mtype correctly?
 * @return pointer to mtype item.
 ***********************************************************************/
MTYPE	find_mtype

	(
	STRING	name,
	LOGICAL	*status
	)

	{
	MTYPE	mtype;

	if (MtypeFunction)
		{
		mtype = MtypeFunction(name);
		if (mtype >= 0)
			{
			if (status) *status = TRUE;
			return mtype;
			}
		}

	if (status) *status = FALSE;
	return SafeMtype;
	}

/**********************************************************************/

/***********************************************************************/
/**	Convert presentation parameter font name to value.
 *
 * @param[in]	name	name of font.
 * @param[out]	*status did we identify the font correctly?
 * @return pointer to font item.
 ***********************************************************************/
FONT	find_font

	(
	STRING	name,
	LOGICAL	*status
	)

	{
	FONT	font;

	if (FontFunction)
		{
		font = FontFunction(name);
		if (font >= 0)
			{
			if (status) *status = TRUE;
			return font;
			}
		}

	if (status) *status = FALSE;
	return SafeFont;
	}

/**********************************************************************/

/***********************************************************************/
/**	Convert presentation parameter font size name to value.
 *
 * @param[in]	name	name of font size.
 * @param[out]	*status did we identify the font size correctly?
 * @return font size value.
 ***********************************************************************/
float	find_size

	(
	STRING	name,
	LOGICAL	*status
	)

	{
	float	size;

	if (SizeFunction)
		{
		size = SizeFunction(name);
		if (size == 0)
			pr_error("Presentation", "Zero size for font size %s\n", name);
		if (size >= 0)
			{
			if (status) *status = TRUE;
			return size;
			}
		}

	if (status) *status = FALSE;
	return SafeTsize;
	}

/**********************************************************************/

/***********************************************************************/
/**	Convert presentation parameter offset name to value.
 *
 * @param[in]	name	name of offset.
 * @param[out]	*status did we identify the offset correctly?
 * @return offset value.
 ***********************************************************************/
float	find_offset

	(
	STRING	name,
	LOGICAL	*status
	)

	{
	float	size;
	STRING	pname;
	LOGICAL	neg = FALSE;

	if (!blank(name) && SizeFunction)
		{
		pname = name;
		if (pname[0] == '-')
			{
			pname++;
			neg = TRUE;
			}
		size = SizeFunction(pname);
		if (size >= 0)
			{
			if (status) *status = TRUE;
			return ((neg)? -size: size);
			}
		}

	if (status) *status = FALSE;
	return SafeWidth;
	}

/**********************************************************************/

/***********************************************************************/
/**	Convert presentation parameter barb type name to value.
 *
 * @param[in]	name	name of barb type.
 * @param[out]	*status did we identify the barb type correctly?
 * @return pointer to btype item.
 ***********************************************************************/
BTYPE	find_btype

	(
	STRING	name,
	LOGICAL	*status
	)

	{
	BTYPE	btype;

	if (BtypeFunction)
		{
		btype = BtypeFunction(name);
		if (btype >= 0)
			{
			if (status) *status = TRUE;
			return btype;
			}
		}

	if (status) *status = FALSE;
	return SafeBtype;
	}

/**********************************************************************/

/***********************************************************************/
/**	Convert presentation parameter poption name to value.
 *
 * @param[in]	name	name of poption.
 * @param[in]	type	option type (reserved for later)
 * @param[out]	*status did we identify the poption correctly?
 * @return poption value
 ***********************************************************************/
LOGICAL	find_poption

	(
	STRING	name	/* given name */ ,
	STRING	type	/* option type (reserved for later) */ ,
	LOGICAL	*status	/* status */
	)

	{
	LOGICAL	popt;

	if (PoptionFunction)
		{
		popt = PoptionFunction(name, type);
		if (popt >= 0)
			{
			if (status) *status = TRUE;
			return popt;
			}
		}

	if (status) *status = FALSE;
	return FALSE;
	}
