/*********************************************************************/
/**	@file pipe.h
 *
 * Definitions and connections for graphics pipe software.
 *
 * Graphics Pipe Manager
 *
 * This package passes a stream of x-y points through a number of
 * option modules. Certain modules perform some type of operation
 * on the stream of points, while the rest produce some form of
 * output.
 *
 * Initially, all of the modules are disabled. This means that 
 * points passed down the pipe will be discarded. Each module
 * is subsequently enabled or disabled with an appropriate
 * subroutine call.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    p i p e . h                                                       *
*                                                                      *
*    Definitions and connections for graphics pipe software.           *
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

/* See if already included */
#ifndef PIPE_DEFS

/* We need various include files */
#include "metafile.h"

/* Declare external functions in pipe.c */

void	reset_pipe(void);			/**< Disable all modules */
void	line_pipe(LINE line);		/**< Process an entire set of points */
void	point_pipe(POINT pos);		/**< Put next point into pipe */
void	put_pipe(float x, float y); /**< Put next point (x, y) into pipe */
void	flush_pipe(void);			/**< Terminate current point stream (pen-up) */
void	provide_pipe_colour_fn(void (*)(COLOUR colour, HILITE hilite));
void	provide_pipe_lstyle_fn(void (*)(LSTYLE style,
						float width, float length));
void	provide_pipe_move_fn(void (*)(float x, float y));
void	provide_pipe_draw_fn(void (*)(float x, float y));
void	provide_pipe_msize_fn(void (*)(float msize));
void	provide_pipe_mangle_fn(void (*)(float mangle));
void	provide_pipe_marker_fn(void (*)(int type, float xoff, float yoff));
void	provide_pipe_flush_fn(void (*)(void));

/* Declare external functions in pipe_filter.c */
void	enable_filter(float fres, float fangle);	/**< Enable polyline filtering */
void	disable_filter(void);						/**< Disable polyline filtering */
void	put_filter(float x, float y);				/**< Add point to filtered polyline */
void	flush_filter(void);							/**< Flush filtered polyline */

/* Declare external functions in pipe_buffer.c */
void	enable_buffer(void);						/**< Enable polyline buffering */
void	disable_buffer(void);						/**< Disable polyline buffering */
void	put_buffer(float x, float y);				/**< Add point to buffered polyline */
void	flush_buffer(void);							/**< Flush buffered polyline */

/* Declare external functions in pipe_spline.c */
void	enable_spline(float sres, LOGICAL closed,
						float tension, float period, float amplitude);
													/**< Enable spline */
void	disable_spline(void);						/**< Disable spline */
void	put_spline(float x, float y);				/**< Pass point to spline */
void	flush_spline(void);							/**< End the stream */

/* Declare external functions in pipe_clip.c */
void	enable_clip(float left, float right, float bottom, float top,
						LOGICAL polygon, LOGICAL closed);
													/**< Enable clipping */
void	disable_clip(void);							/**< Disable clipping */
void	put_clip(float x, float y);					/**< Pass point to clipper */
void	flush_clip(void);							/**< End the stream */

/* Declare external functions in pipe_fill.c */
void	enable_fill(float fres);					/**< Enable polyline filling */
void	disable_fill(void);							/**< Disable polyline filling */
void	put_fill(float x, float y);					/**< Pass point to filled polyline */
void	flush_fill(void);							/**< Flush filled polyline */

/* Declare external functions in pipe_echo.c */
void	enable_echo(COLOUR colour);					/**< Enable screen echo */
void	disable_echo(void);							/**< Disable screen echo */
void	put_echo(float x, float y);					/**< Echo the given point */
void	flush_echo(void);							/**< Does nothing */

/* Declare external functions in pipe_disp.c */
void	enable_disp(COLOUR colour, LSTYLE style);	/**< Enable screen display */
void	disable_disp(void);							/**< Disable screen display */
void	put_disp(float x, float y);					/**< Add given point to display */
void	flush_disp(void);							/**< Terminate the current curve
													 (pen up) */

/* Declare external functions in pipe_save.c */
void	enable_save(void);							/**< Enable save */
void	disable_save(void);							/**< Disable save */
void	put_save(float x, float y);					/**< Add given point to curve */
void	flush_save(void);							/**< End current curve */
int		recall_save(LINE **lbuf);					/**< Retrieve saved lines */
LOGICAL	push_save(void);							/**< Push current saved lines
													  onto stack */
LOGICAL	pop_save(void);								/**< Pop current saved lines 
													  off stack */

/* Declare external functions in pipe_meta.c */
void	enable_meta(METAFILE meta, STRING entity, STRING elem, STRING level,
						STRING subelem, STRING value, STRING label);
													/**< Enable output to metafile */
void	disable_meta(void);							/**< Disable output to metafile */
void	put_meta(float x, float y);					/**< Add point to buffered polyline */
void	flush_meta(void);							/**< Flush buffered polyline */

/* Now it has been included */
#define PIPE_DEFS
#endif
