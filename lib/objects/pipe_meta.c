/**********************************************************************/
/**	@file pipe_meta.c
 *
 *	Meta module of graphics pipe.
 *
 *	The incoming stream of points is stored in a 'CURVE' object
 *	until the 'flush' routine is called.  Then it is output to a
 *	'METAFILE' object.
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*     p i p e _ m e t a . c                                            *
*                                                                      *
*     Meta module of graphics pipe software.                           *
*                                                                      *
*     (c) Copyright 1988 Environment Canada (AES)                      *
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

#include "pipeP.h"
#include <string.h>
#include <stdio.h>

/***********************************************************************
*                                                                      *
*       e n a b l e _ m e t a   - enable output to metafile            *
*     d i s a b l e _ m e t a   - disable output to metafile           *
*             p u t _ m e t a   - add point to buffered polyline       *
*         f l u s h _ m e t a   - flush buffered polyline (pen-up)     *
*                                                                      *
*     The incoming stream of points is stored in a 'CURVE' object      *
*     until the 'flush' routine is called.  Then it is output to a     *
*     'METAFILE' object.                                               *
*                                                                      *
***********************************************************************/

/* Flags and shared parameters for meta module */
static CURVE	Curve       = NULL;
static METAFILE	Meta        = NULL;
static char		Entity[41]  = "";
static char		Element[41] = "";
static char		Level[41]   = "";
/**********************************************************************/
/** Turn on output to metafile
 *
 *	@param[in]	meta	metafile object
 *	@param[in]	ent		Entity
 *	@param[in]	elem	Element
 *	@param[in]	lev		Level
 *	@param[in]	subelem	Sub-Element
 *	@param[in]	value	Value
 *	@param[in]	labst	Label
 **********************************************************************/

void		enable_meta

	(
	METAFILE	meta,
	STRING		ent,
	STRING		elem,
	STRING		lev,
	STRING		subelem,
	STRING		value,
	STRING		labst
	)

	{
	enable_module(Pmeta);
	if (!Curve) Curve = create_curve(subelem,value,labst);
	else        define_curve_value(Curve,subelem,value,labst);
	empty_curve(Curve);

	/* Save line descriptors */
	Meta = meta;
	(void) strncpy(Entity,ent,40);
	(void) strncpy(Element,elem,40);
	(void) strncpy(Level,lev,40);
	}

/** Turn off output to metafile */
void disable_meta(void)
	{
	disable_module(Pmeta);
	Curve = destroy_curve(Curve);
	}

/** Add point to buffered polyline */
void put_meta(float x, float y)
	{
	POINT	p;

	/* Add point to curve */
	p[X] = x;
	p[Y] = y;
	add_point_to_curve(Curve,p);
	put_next(Pmeta,x,y);
	}

/** Flush buffered polyline (pen up) */
void flush_meta(void)
	{
	CURVE	c;

	/* Place the new curve into the metafile */
	if (Curve->line->numpts > 0)
		{
		c = copy_curve(Curve);
		add_item_to_metafile(Meta,"curve",Entity,Element,Level,(ITEM) c);
		}

	/* Empty the curve and flush the rest of the pipe */
	empty_curve(Curve);
	flush_next(Pmeta);
	}
