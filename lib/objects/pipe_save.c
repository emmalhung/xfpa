/**********************************************************************/
/**	@file pipe_save.c
 *
 *	Save module of graphics pipe.
 *
 *	Incoming streams of points are saved in an array of "LINE" objects.
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*     p i p e _ s a v e . c                                            *
*                                                                      *
*     Save module of graphics pipe software.                           *
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
#include <fpa_getmem.h>

/***********************************************************************
*                                                                      *
*       e n a b l e _ s a v e   - enable saving                        *
*     d i s a b l e _ s a v e   - disable saving                       *
*             p u t _ s a v e   - add point to current curve           *
*         f l u s h _ s a v e   - end current curve (pen-up)           *
*       r e c a l l _ s a v e   - retrieve the saved lines             *
*           p u s h _ s a v e   - push current saved lines onto stack  *
*             p o p _ s a v e   - pop current saved lines off stack    *
*                                                                      *
*     Incoming streams of points are saved in an array of "LINE"       *
*     objects.                                                         *
*                                                                      *
***********************************************************************/

/* Shared parameters */
static	LINE	*Lstack[] = { NULL, NULL, NULL, NULL, NULL };
static	int		Nstack[]  = {    0,    0,    0,    0,    0 };
static	int		Istack    = 0;
static	int		Mstack    = 5;

static	LINE	**Lbuf    = Lstack;
static	int		*Nbuf     = Nstack;

static	LOGICAL	New       = FALSE;
static	LINE	Line      = NULL;

static	void	clean_save(void);

/** Turn on saving */
void	enable_save(void)
	{
	enable_module(Psave);
	clean_save();
	}

/** Turn off saving */
void	disable_save(void)
	{
	disable_module(Psave);
	clean_save();
	}

/** Add point to current curve */
void	put_save(float x, float y)
	{
	POINT	p;
	int		last;

	/* If starting new line - add a fresh line to the array */
	if (New)
		{
		last  = (*Nbuf)++;
		*Lbuf = GETMEM(*Lbuf,LINE,*Nbuf);
		(*Lbuf)[last] = create_line();
		Line  = (*Lbuf)[last];
		New   = FALSE;
		}

	/* Add given point to the current line */
	p[X] = x;
	p[Y] = y;
	add_point_to_line(Line,p);
	put_next(Psave,x,y);
	}

/** End current curve (pen up) */
void	flush_save(void)
	{
	New = TRUE;
	flush_next(Psave);
	}

/** Retrieve the saved lines */
int		recall_save(LINE **lbuf)
	{
	if (lbuf) *lbuf = *Lbuf;
	return *Nbuf;
	}

/** Push current saved lines onto stack */
LOGICAL	push_save(void)
	{
	if (Istack >= Mstack-1)
		{
		(void) printf("[push_save] Stack overflow\n");
		return FALSE;
		}

	Istack++;
	Lbuf = Lstack + Istack;
	Nbuf = Nstack + Istack;
	return TRUE;
	}

/** Pop current saved lines off stack */
LOGICAL	pop_save(void)
	{
	if (Istack <= 0)
		{
		(void) printf("[push_save] Stack underflow\n");
		return FALSE;
		}

	clean_save();

	Istack--;
	Lbuf = Lstack + Istack;
	Nbuf = Nstack + Istack;
	return TRUE;
	}

static	void	clean_save(void)
	{
	int	i;

	for (i=0; i<*Nbuf; i++)
		destroy_line((*Lbuf)[i]);
	FREEMEM(*Lbuf);
	*Nbuf = 0;
	Line  = NULL;
	New   = TRUE;
	}
