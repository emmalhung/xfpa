/**********************************************************************/
/**	@file pipe_echo.c
 *
 *	Echo module of the graphics pipe 
 *
 *	Echo points on the graphics display as they are encountered.
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*     p i p e _ e c h o . c                                            *
*                                                                      *
*     Echo module of the graphics pipe software.                       *
*                                                                      *
*     (c) Copyright 1987 Environment Canada (AES)                      *
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

/***********************************************************************
*                                                                      *
*       e n a b l e _ e c h o   - turn on screen echo                  *
*     d i s a b l e _ e c h o   - turn off screen echo                 *
*             p u t _ e c h o   - echo the given point                 *
*         f l u s h _ e c h o   - (does nothing)                       *
*                                                                      *
*     Echo points on the graphics display as they are encountered.     *
*                                                                      *
***********************************************************************/

/* Flags and shared parameters for echo module */
static COLOUR	Colour = 0;

/**********************************************************************/
/** Turn on screen echo
 *
 * 	@param[in]	colour	Colour of points
 **********************************************************************/
void enable_echo

	(
	COLOUR	colour	/* colour of points */
	)

	{
	enable_module(Pecho);
	Colour = colour;
	}

/** Turn off screen echo */
void disable_echo(void)
	{
	disable_module(Pecho);
	}

/** echo the given point */
void put_echo(float x, float y)
	{
	pipe_colour_fn(Colour,0);
	pipe_msize_fn(0.0);
	pipe_mangle_fn(0.0);
	pipe_marker_fn(0,x,y);
	pipe_flush_fn();
	put_next(Pecho,x,y);
	}

/** Does nothing */
void flush_echo(void)
	{ flush_next(Pecho); }
