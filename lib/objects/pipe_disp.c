/**********************************************************************/
/**	@file pipe_disp.c
 * 
 *	Display module of the graphics pipe
 *
 *	Display the incoming point stream on the graphics display
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*     p i p e _ d i s p . c                                            *
*                                                                      *
*     Display module of the graphics pipe software.                    *
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
*       e n a b l e _ d i s p   - turn on screen display               *
*     d i s a b l e _ d i s p   - turn off screen display              *
*             p u t _ d i s p   - add the given point to the display   *
*         f l u s h _ d i s p   - terminate the current curve (pen up) *
*                                                                      *
*     Display the incoming point stream on the graphics display.       *
*                                                                      *
***********************************************************************/

/* Flags and shared parameters for disp module */
static LOGICAL	New    = FALSE;
static COLOUR	Colour = 0;
static LSTYLE	Style  = 0;
static float	xprev;
static float	yprev;
/**********************************************************************/
/** Turn on screen display
 *
 * 	@param[in]	colour	Line colour
 * 	@param[in]	style	Line style
 **********************************************************************/
void enable_disp

	(
	COLOUR	colour,	/* line colour */
	LSTYLE	style	/* line style */
	)

	{
	enable_module(Pdisp);
	New    = TRUE;
	Colour = colour;
	Style  = style;
	}

/** Turn off screen display */
void disable_disp(void)
	{
	disable_module(Pdisp);
	New    = FALSE;
	Colour = 0;
	Style  = 0;
	}

/** Add the given point to the display */
void put_disp(float x, float y)
	{
	if (!New)
		{
		pipe_colour_fn(Colour,0);
		pipe_lstyle_fn(Style,0.0,0.0);
		pipe_move_fn(xprev,yprev);
		pipe_draw_fn(x,y);
		}
	New   = !New;
	xprev = x;
	yprev = y;
	put_next(Pdisp,x,y);
	}

/** Terminate the current curve (Pen up) */
void flush_disp(void)
	{
	flush_next(Pdisp);
	pipe_flush_fn();
	New = TRUE;
	}
