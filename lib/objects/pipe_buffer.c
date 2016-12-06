/**********************************************************************/
/**	@file pipe_buffer.c
 *
 *	Buffer module of graphics pipe.
 *
 *	The incoming stream of points is stored in a 'LINE' object
 *	until the 'flush' routine is called.
 *
 *	You need to disable this buffer when you are finished with it.
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*     p i p e _ b u f f e r . c                                        *
*                                                                      *
*     Buffer module of graphics pipe software.                         *
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
#include <stdio.h>

/***********************************************************************
*                                                                      *
*       e n a b l e _ b u f f e r                                      *
*     d i s a b l e _ b u f f e r                                      *
*             p u t _ b u f f e r                                      *
*         f l u s h _ b u f f e r                                      *
*                                                                      *
***********************************************************************/

/* Flags and shared parameters for buffer module */
static LINE	line = NULL;

/** Turn on polyline buffering */
void	enable_buffer(void)
	{
	enable_module(Pbuffer);
	if (!line) line = create_line();
	empty_line(line);
	}

/** Turn off polyline buffering */
void	disable_buffer(void)
	{
	disable_module(Pbuffer);
	line = destroy_line(line);
	}

/** Add point to buffered polyline */
void	put_buffer
 (
 float x,	/* x coord */
 float y	/* y coord */
 )
	{
	POINT	p;

	/* Add point to line */
	p[X] = x;
	p[Y] = y;
	add_point_to_line(line,p);
	}

/** Flush buffered polyline (pen-up) */
void	flush_buffer(void)
	{
	int	i;
	float	x, y;

	/* Pass the buffered line to the rest of the pipe */
	for (i=0; i<line->numpts; i++)
		{
		x = line->points[i][X];
		y = line->points[i][Y];
		put_next(Pbuffer,x,y);
		}

	/* Empty the line and flush the rest of the pipe */
	empty_line(line);
	flush_next(Pbuffer);
	}
