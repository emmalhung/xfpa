/**********************************************************************/
/**	@file pipe.c
 *
 * 	Graphics Pipe Manager
 *
 * 	This package passes a stream of x-y points through a number
 * 	of optional modules.  Certain modules perform some type of
 * 	operation on the stream of points, while the rest produce
 * 	some form of output.
 *
 * 	Initially, all of the modules are disabled.  This means that
 * 	points passed down the pipe will be discarded.  Each module
 * 	is subsequently enabled or disabled with an appropriate
 * 	subroutine call.
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*     p i p e . c                                                      *
*                                                                      *
*     Graphics Pipe Manager                                            *
*                                                                      *
*     This package passes a stream of x-y points through a number      *
*     of optional modules.  Certain modules perform some type of       *
*     operation on the stream of points, while the rest produce        *
*     some form of output.                                             *
*                                                                      *
*     Initially, all of the modules are disabled.  This means that     *
*     points passed down the pipe will be discarded.  Each module      *
*     is subsequently enabled or disabled with an appropriate          *
*     subroutine call.                                                 *
*                                                                      *
*     The description of all the modules is given in pipeP.h.          *
*                                                                      *
*     Only the pipe management routines and the main module are found  *
*     in this source file.  The remaining modules are found in source  *
*     files prefixed with "pipe_".                                     *
*                                                                      *
*     (c) Copyright 1988 Environment Canada (AES)                      *
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

#define PIPE_INIT
#include "pipeP.h"

/***********************************************************************
*                                                                      *
*     r e s e t _ p i p e                                              *
*         p u t _ p i p e                                              *
*     p o i n t _ p i p e                                              *
*     f l u s h _ p i p e                                              *
*       l i n e _ p i p e                                              *
*                                                                      *
*     These are the principal calls in the graphics "pipe" package.    *
*                                                                      *
***********************************************************************/

void	reset_pipe(void)

	{
	/* Turn everything off except pipe itself */
	disable_module(Pfilter);
	disable_module(Pbuffer);
	disable_module(Pecho);
	disable_module(Pspline);
	disable_module(Pdisp);
	disable_module(Pclip);
	disable_module(Pfill);
	disable_module(Psave);
	disable_module(Pmeta);

	/* Get rid of garbage */
	enable_module(Ppipe);
	flush_module(Ppipe);
	}

void	put_pipe(float x, float y)
	{ put_next(Ppipe,x,y); }


void	point_pipe(POINT p)
	{ put_next(Ppipe,p[X],p[Y]); }


void	flush_pipe(void)
	{ flush_next(Ppipe); }


void	line_pipe(LINE line)
	{
	int	i;
	float	x, y;

	/* If no line given do nothing */
	if (!line) return;

	/* Pass each point from line to pipe */
	for (i=0; i<line->numpts; i++)
		{
		x = line->points[i][X];
		y = line->points[i][Y];
		put_next(Ppipe,x,y);
		}
	flush_next(Ppipe);
	}


/***********************************************************************
*                                                                      *
*       e n a b l e _ m o d u l e                                      *
*     d i s a b l e _ m o d u l e                                      *
*             p u t _ m o d u l e                                      *
*         f l u s h _ m o d u l e                                      *
*             p u t _ n e x t                                          *
*         f l u s h _ n e x t                                          *
*                                                                      *
*     Pipe management routines.                                        *
*                                                                      *
***********************************************************************/

void enable_module(pnode *p)
{ if(p) p->enabled = TRUE; }

void disable_module(pnode *p)
{ if(p) p->enabled = FALSE; }

void put_module(pnode *p, float x, float y)
	{
	if (!p) return;
	if (p->enabled) p->put(x,y);
	else	{
		put_module(p->next1,x,y);
		put_module(p->next2,x,y);
		}
	}

void flush_module(pnode *p)
	{
	if (!p) return;
	if (p->enabled) p->flush();
	else	{
		flush_module(p->next1);
		flush_module(p->next2);
		}
	}

void put_next(pnode *p, float x, float y)
	{
	if (!p) return;
	put_module(p->next1,x,y);
	put_module(p->next2,x,y);
	}

void flush_next(pnode *p)
	{
	if (!p) return;
	flush_module(p->next1);
	flush_module(p->next2);
	}


/***********************************************************************
*                                                                      *
*     p r o v i d e _ p i p e _ * _ f n                                *
*                     p i p e _ * _ f n                                *
*                                                                      *
*     Hook up to upper level graphics functions for display and echo.  *
*                                                                      *
***********************************************************************/

/* Supplied upper level graphics functions */
static	void (*colour_fn) (COLOUR, HILITE)       = 0;
static	void (*lstyle_fn) (LSTYLE, float, float) = 0;
static	void (*move_fn)   (float, float)         = 0;
static	void (*draw_fn)   (float, float)         = 0;
static	void (*msize_fn)  (float)                = 0;
static	void (*mangle_fn) (float)                = 0;
static	void (*marker_fn) (int, float, float)    = 0;
static	void (*flush_fn)  (void)                 = 0;

/**********************************************************************/

void    provide_pipe_colour_fn
	( void (*fn)(COLOUR, HILITE) )
	{ colour_fn = fn; }

void    provide_pipe_lstyle_fn
	( void (*fn)(LSTYLE, float, float) )
	{ lstyle_fn = fn; }

void    provide_pipe_move_fn
	( void (*fn)(float, float) )
	{ move_fn = fn; }

void    provide_pipe_draw_fn
	( void (*fn)(float, float) )
	{ draw_fn = fn; }

void    provide_pipe_msize_fn
	( void (*fn)(float) )
	{ msize_fn = fn; }

void    provide_pipe_mangle_fn
	( void (*fn)(float) )
	{ mangle_fn = fn; }

void    provide_pipe_marker_fn
	( void (*fn)(int, float, float) )
	{ marker_fn = fn; }

void    provide_pipe_flush_fn
	( void (*fn)(void) )
	{ flush_fn = fn; }

/**********************************************************************/

void	pipe_colour_fn
	( COLOUR i, HILITE j )
	{ if (colour_fn) colour_fn(i, j); }

void	pipe_lstyle_fn
	( LSTYLE i, float f, float g )
	{ if (lstyle_fn) lstyle_fn(i, f, g); }

void	pipe_move_fn
	( float f, float g )
	{ if (move_fn) move_fn(f, g); }

void	pipe_draw_fn
	( float f, float g )
	{ if (draw_fn) draw_fn(f, g); }

void	pipe_msize_fn
	( float f )
	{ if (msize_fn) msize_fn(f); }

void	pipe_mangle_fn
	( float f )
	{ if (mangle_fn) mangle_fn(f); }

void	pipe_marker_fn
	( int i, float f, float g )
	{ if (marker_fn) marker_fn(i, f, g); }

void	pipe_flush_fn
	( void )
	{ if (flush_fn) flush_fn(); }
