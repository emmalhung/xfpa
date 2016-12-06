/**********************************************************************/
/** @file pipeP.h
 *
 *  Private definitions and connections for graphics pipe software.  
 *
 *	This package passes a stream of x-y points through a number
 *	of optional modules.  Certain modules perform some type of
 *	operation on the stream of points, while the rest produce
 *	some form of output.
 *
 *	Initially, all of the modules are disabled.  This means that
 *	points passed down the pipe will be discarded.  Each module
 *	is subsequently enabled or disabled with an appropriate
 *	subroutine call.
 *
 *	The modules are ordered in the following manner:
 *
 * @verbatim
           pipe ---- filter
                       |
                     buffer ---- echo
                       |
                     spline ---- disp
                       |
                     clip
                       |
                     fill ------ save
                           |
                           `---- meta
	@endverbatim 
 *	@par Main Module:
 *
 *	@li	pipe   -> pass the stream of points along the 'pipe'
 *
 *	@par Operations:
 *
 *	@li	filter -> filter out points closer than given distance
 *	@li	buffer -> hold points in an internal buffer until flushed
 *	@li	spline -> fit a parametric cubic spline to the curve
 *	@li	clip   -> perform software clipping to a given window
 *	@li	fill   -> fill between points further than given distance
 *
 *	@par Outputs:
 *
 *	@li	echo   -> echo each point on the graphics display
 *	@li	disp   -> plot the curve on the graphics display
 *	@li	save   -> save the curve in a 'SET' object
 *	@li	meta   -> output the curve to a metafile
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*     p i p e P . h                                                    *
*                                                                      *
*     Private definitions and connections for graphics pipe software.  *
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
*     The modules are ordered in the following manner:                 *
*                                                                      *
*          pipe ---- filter                                            *
*                      |                                               *
*                    buffer ---- echo                                  *
*                      |                                               *
*                    spline ---- disp                                  *
*                      |                                               *
*                    clip                                              *
*                      |                                               *
*                    fill ------ save                                  *
*                          |                                           *
*                          `---- meta                                  *
*                                                                      *
*     Main Module:                                                     *
*                                                                      *
*          pipe   - pass the stream of points along the 'pipe'         *
*                                                                      *
*     Operations:                                                      *
*                                                                      *
*          filter - filter out points closer than given distance       *
*          buffer - hold points in an internal buffer until flushed    *
*          spline - fit a parametric cubic spline to the curve         *
*          clip   - perform software clipping to a given window        *
*          fill   - fill between points further than given distance    *
*                                                                      *
*     Outputs:                                                         *
*                                                                      *
*          echo   - echo each point on the graphics display            *
*          disp   - plot the curve on the graphics display             *
*          save   - save the curve in a 'SET' object                   *
*          meta   - output the curve to a metafile                     *
*                                                                      *
*     (c) Copyright 1988 Environment Canada (AES)                      *
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

/* We need the public include file */
#include "pipe.h"

/* Define "put" and "flush" function pointer types */
typedef void	(*PUTfn)(float, float);
typedef void	(*FLUSHfn)(void);

/* Structure to manage pipe connections */
typedef struct pnode_struct
	{
	LOGICAL				enabled;	/* is this node enabled? */
	PUTfn				put;		/* put function pointer */
	FLUSHfn				flush;		/* flush function pointer */
	struct pnode_struct	*next1;		/* next operation */
	struct pnode_struct	*next2;		/* next operation */
	} pnode;

#define NullPn (pnode *)(0)

/* Set up the initial pipe connections (must be defined in reverse order) */
#ifdef PIPE_INIT
static pnode  Nmeta   = { FALSE, put_meta,   flush_meta,  NullPn,   NullPn   };
static pnode  Nsave   = { FALSE, put_save,   flush_save,  NullPn,   NullPn   };
static pnode  Ndisp   = { FALSE, put_disp,   flush_disp,  NullPn,   NullPn   };
static pnode  Necho   = { FALSE, put_echo,   flush_echo,  NullPn,   NullPn   };
static pnode  Nfill   = { FALSE, put_fill,   flush_fill,  &Nsave,   &Nmeta   };
static pnode  Nclip   = { FALSE, put_clip,   flush_clip,  &Nfill,   NullPn   };
static pnode  Nspline = { FALSE, put_spline, flush_spline,&Ndisp,   &Nclip   };
static pnode  Nbuffer = { FALSE, put_buffer, flush_buffer,&Necho,   &Nspline };
static pnode  Nfilter = { FALSE, put_filter, flush_filter,&Nbuffer, NullPn   };
static pnode  Npipe   = { TRUE,  put_pipe,   flush_pipe,  &Nfilter, NullPn   };
#endif

#undef GLOBAL
#ifdef PIPE_INIT
#	define GLOBAL GLOBAL_INIT
#else
#	define GLOBAL GLOBAL_EXTERN
#endif

/* Set up pointers to the pipe connections */
GLOBAL(pnode *,	Ppipe,		&Npipe);
GLOBAL(pnode *,	Pfilter,	&Nfilter);
GLOBAL(pnode *,	Pbuffer,	&Nbuffer);
GLOBAL(pnode *,	Pspline,	&Nspline);
GLOBAL(pnode *,	Pclip,		&Nclip);
GLOBAL(pnode *,	Pfill,		&Nfill);
GLOBAL(pnode *,	Pecho,		&Necho);
GLOBAL(pnode *,	Pdisp,		&Ndisp);
GLOBAL(pnode *,	Psave,		&Nsave);
GLOBAL(pnode *,	Pmeta,		&Nmeta);

/* Declare internal functions */
void	enable_module(pnode *p);
void	disable_module(pnode *p);
void	put_module(pnode *p, float x, float y);
void	flush_module(pnode *p);
void	put_next(pnode *p, float x, float y);
void	flush_next(pnode *p);
void	pipe_colour_fn(COLOUR colour, HILITE hilite);
void	pipe_lstyle_fn(LSTYLE style, float width, float length);
void	pipe_move_fn(float x, float y);
void	pipe_draw_fn(float x, float y);
void	pipe_msize_fn(float msize);
void	pipe_mangle_fn(float mangle);
void	pipe_marker_fn(int type, float xoff, float yoff);
void	pipe_flush_fn(void);
