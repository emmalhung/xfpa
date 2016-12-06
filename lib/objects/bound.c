/*********************************************************************/
/**	@file bound.c
 *
 * Routines to handle the BOUND object.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*      b o u n d . c                                                   *
*                                                                      *
*      Routines to handle the BOUND object.                            *
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

#define BOUND_INIT
#include "bound.h"

#include <fpa_getmem.h>
#include <fpa_math.h>

#include <string.h>

int		BoundCount = 0;

/***********************************************************************
*                                                                      *
*      c r e a t e _ b o u n d                                         *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Create an empty boundary object.
 *
 * @return Pointer to new boudary object. You will need to destroy
 * the object when you are done with it.
 *********************************************************************/
BOUND	create_bound(void)

	{
	BOUND	bnew;

	/* Allocate structure */
	bnew = INITMEM(struct BOUND_struct, 1);
	if (!bnew) return NullBound;

	/* Initialize bound buffer */
	bnew->boundary = NullLine;
	bnew->numhole  = 0;
	bnew->holes    = NullLineList;

	BoundCount++;
	return bnew;
	}

/***********************************************************************
*                                                                      *
*      d e s t r o y _ b o u n d                                       *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Free all allocated memory from the given bound.
 *
 *	@param[in] 	bound	Object to destroy
 *  @return NullBound
 *********************************************************************/

BOUND	destroy_bound

	(
	BOUND	bound
	)

	{
	/* Do nothing if not there */
	if (!bound) return NullBound;

	/* Empty it first */
	empty_bound(bound);

	/* Destroy the boundary line */
	define_bound_boundary(bound, NullLine);

	/* Free the structure itself */
	FREEMEM(bound);
	BoundCount--;
	return NullBound;
	}

/***********************************************************************
*                                                                      *
*      e m p t y _ b o u n d                                           *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Empty the point buffer of the given bound without actually
 * freeing any memory.
 *
 *	@param[in] 	bound	Object to empty
 *********************************************************************/
void	empty_bound

	(
	BOUND	bound
	)

	{
	/* Do nothing if not there */
	if (!bound) return;

	/* Empty the boundary line */
	empty_line(bound->boundary);

	/* Fill in the holes */
	define_bound_holes(bound, 0, NullLineList);
	}

/***********************************************************************
*                                                                      *
*      c o p y _ b o u n d                                             *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Make an exact copy of a bound.
 *
 *	@param[in]  bound	bound to be copied
 * 	@return Pointer to copy of boundary. You need to destroy the copy
 * 			when you are done with it.
 *********************************************************************/
BOUND	copy_bound

	(
	const BOUND	bound
	)

	{
	BOUND	bnew;
	int		i;

	if (!bound) return NullBound;

	/* Create an empty one */
	bnew = create_bound();

	/* Copy the boundary */
	bnew->boundary = copy_line(bound->boundary);

	/* Copy the holes */
	bnew->numhole = bound->numhole;
	if (bnew->numhole > 0)
		{
		bnew->holes = INITMEM(LINE, bnew->numhole);
		for (i=0; i<bnew->numhole; i++)
			bnew->holes[i] = copy_line(bound->holes[i]);
		}

	return bnew;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ b o u n d                                         *
*      d e f i n e _ b o u n d _ b o u n d a r y                       *
*      d e f i n e _ b o u n d _ h o l e s                             *
*      a d d _ b o u n d _ h o l e                                     *
*      r e m o v e _ b o u n d _ h o l e                               *
*                                                                      *
*      Set or reset the boundary or holes of the given bound.          *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set or reset the boundary of the given bound object.
 *
 *	@param[in] 	bound		given bound
 *	@param[in] 	boundary	line to define boundary
 *	@param[in] 	numhole		number of holes
 *	@param[in] 	*holes		list of holes
 *********************************************************************/
void	define_bound

	(
	BOUND		bound,
	LINE		boundary,
	int			numhole,
	const LINE	*holes
	)

	{
	/* Do nothing if bound not there */
	if (!bound) return;

	/* Define the boundary */
	define_bound_boundary(bound, boundary);

	/* Define the holes */
	define_bound_holes(bound, numhole, holes);
	}

/**********************************************************************/

/*********************************************************************/
/** Set or reset the boundary of the given bound object.
 *
 *	@param[in] 	bound		given bound
 *	@param[in] 	boundary	line to define boundary
 *********************************************************************/
void	define_bound_boundary

	(
	BOUND	bound,
	LINE	boundary
	)

	{
	/* Do nothing if bound not there */
	if (!bound) return;

	/* Define the boundary */
	destroy_line(bound->boundary);
	bound->boundary = boundary;
	}

/**********************************************************************/

/*********************************************************************/
/**	Set or reset the holes of the given bound object.
 *
 *	@param[in] 	bound	given bound
 *	@param[in] 	numhole	number of holes
 *	@param[in] 	*holes	list of holes
 *********************************************************************/
void	define_bound_holes

	(
	BOUND		bound,
	int			numhole,
	const LINE	*holes
	)

	{
	int		i;

	/* Do nothing if bound not there */
	if (!bound) return;

	/* Delete the old hole list */
	for (i=0; i<bound->numhole; i++)
		destroy_line(bound->holes[i]);
	bound->numhole = 0;
	FREEMEM(bound->holes);

	if (numhole <= 0) return;
	if (!holes)       return;

	/* Define the new hole list */
	bound->numhole = numhole;
	bound->holes   = (LINE *) holes;
	}

/**********************************************************************/

/*********************************************************************/
/**	 Add a hole to the given bound.
 *
 *	@param[in] 	bound		given bound
 *	@param[in] 	hole		hole to add
 *********************************************************************/
void	add_bound_hole

	(
	BOUND	bound,
	LINE	hole
	)

	{
	int		last;

	/* Do nothing if bound not there */
	if (!bound) return;
	if (!hole)  return;

	/* Add the new hole to the list */
	last = bound->numhole++;
	bound->holes = GETMEM(bound->holes, LINE, bound->numhole);
	bound->holes[last] = hole;
	}

/**********************************************************************/

/*********************************************************************/
/**	Remove a hole from the given bound.
 *
 *	@param[in] 	bound		given bound
 *	@param[in] 	hole		hole to remove
 *********************************************************************/
void	remove_bound_hole

	(
	BOUND	bound,
	LINE	hole
	)

	{
	int		ih;
	LINE	*hp;

	/* Do nothing if bound not there */
	if (!bound) return;
	if (!hole)  return;

	/* See if it is in the list */
	ih = which_bound_hole(bound, hole);
	if (ih < 0) return;

	/* Remove it from the list */
	hole = destroy_line(hole);
	hp   = bound->holes;
	bound->numhole--;
	for (; ih < bound->numhole; ih++)
		hp[ih] = hp[ih+1];
	}

/***********************************************************************
*                                                                      *
*      w h i c h _ b o u n d _ h o l e                                 *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find the position of the given hole in the given bound's list.
 *
 *	@param[in] 	bound	bound to search
 *	@param[in] 	hole	dividing hole to look for
 *  @return index position of the given hole in the bound's list.
 *********************************************************************/

int	which_bound_hole

	(
	BOUND	bound,
	LINE	hole
	)

	{
	int		i;

	/* Do nothing if bound not there */
	if (!bound) return -1;
	if (!hole) return -1;

	/* Search the list */
	for (i=0; i<bound->numhole; i++)
		if (bound->holes[i] == hole) return i;

	/* Not found */
	return -1;
	}
