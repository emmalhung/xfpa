/*********************************************************************/
/**	@file barb.c
 *
 * Routines to handle the BARB object.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*      b a r b . c                                                     *
*                                                                      *
*      Routines to handle the BARB object.                             *
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

#define BARB_INIT
#include "barb.h"

#include <fpa_getmem.h>
#include <tools/tools.h>

int		BarbCount = 0;

/***********************************************************************
*                                                                      *
*      c r e a t e _ b a r b                                           *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Create a new barb with given attributes.
 *
 *	@param[in] 	subelem 	subelement
 *	@param[in] 	anchor 		barb location
 *	@param[in] 	dir 		direction
 *	@param[in] 	speed 		speed
 *	@param[in] 	gust		gust
 * 	@return Pointer to a new BARB object. You will have to destroy it
 * 			when you are finished with it.
 *********************************************************************/

BARB	create_barb

	(
	STRING		subelem,
	const POINT	anchor,
	float		dir,
	float		speed,
	float		gust
	)

	{
	BARB	barb;

	/* Allocate space for the structure */
	barb = INITMEM(struct BARB_struct, 1);
	if (!barb) return NullBarb;

	/* Initialize the structure */
	barb->attrib  = NullAttribList;
	barb->subelem = NULL;
	barb->value   = NULL;
	barb->label   = NULL;
	barb->dir     = 0;
	barb->speed   = 0;
	barb->gust    = 0;
	init_bspec(&barb->bspec);
	init_tspec(&barb->tspec);

	/* Set barb attributes and value */
	define_barb_value(barb, subelem, anchor, dir, speed, gust);

	/* Return the new barb */
	BarbCount++;
	return barb;
	}

/***********************************************************************
*                                                                      *
*      d e s t r o y _ b a r b                                         *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Destroy space allocated to the given barb.
 *
 *	@param[in] 	barb	barb to be destroyed
 * 	@return NullBarb
 *********************************************************************/
BARB destroy_barb

	(
	BARB	barb
	)

	{
	/* Do nothing if barb not there */
	if (!barb) return NullBarb;

	/* Free the space used by attributes */
	FREEMEM(barb->subelem);
	FREEMEM(barb->value);
	FREEMEM(barb->label);
	destroy_attrib_list(barb->attrib);

	/* Free the space used by presentation specs */
	free_bspec(&barb->bspec);
	free_tspec(&barb->tspec);

	/* Free the structure itself */
	FREEMEM(barb);
	BarbCount--;
	return NullBarb;
	}

/***********************************************************************
*                                                                      *
*      c o p y _ b a r b                                               *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Make an exact copy of a barb.
 *
 *	@param[in] 	barb	barb to be copied
 * 	@return Pointer to a copy of the barb. You will need to destroy
 * 			the copy when you are done with it.
 *********************************************************************/

BARB copy_barb

	(
	const BARB	barb
	)

	{
	BARB	copy;

	if (!barb) return NullBarb;

	/* Create a copy with the same value */
	copy = create_barb(NULL, barb->anchor, barb->dir, barb->speed, barb->gust);

	/* Duplicate presentation specs */
	copy_bspec(&copy->bspec, &barb->bspec);
	copy_tspec(&copy->tspec, &barb->tspec);
	define_barb_attribs(copy, barb->attrib);
	return copy;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ b a r b _ v a l u e                               *
*      r e c a l l _ b a r b _ v a l u e                               *
*                                                                      *
*      Set/reset or retrieve the position and value of the given barb. *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/**	Set or reset the position and value of the given barb.
 *
 *	@param[in] 	barb 		 given barb
 *	@param[in] 	subelem 	 subelement
 *	@param[in] 	anchor 		 barb location
 *	@param[in] 	dir 		 direction
 *	@param[in] 	speed 		 speed
 *	@param[in] 	gust		 gust
 *********************************************************************/
void define_barb_value

	(
	BARB		barb,
	STRING		subelem,
	const POINT	anchor,
	float		dir,
	float		speed,
	float		gust
	)

	{
	if (!barb) return;

	/* Set location and value */
	barb->subelem = STRMEM(barb->subelem, subelem);
	if (anchor) copy_point(barb->anchor, anchor);
	else        copy_point(barb->anchor, ZeroPoint);
	barb->dir     = range_norm(dir, 0., 360., NullInt);
	barb->speed   = speed;
	barb->gust    = gust;

	/* Replicate in attributes */
	if (IsNull(barb->attrib)) barb->attrib = create_attrib_list();
	add_default_attributes(barb->attrib, subelem, NULL, NULL);
	}

/*********************************************************************/
/**	Retrieve the position and value of the given barb.
 *
 *	@param[in] 	barb		given barb
 *	@param[out] *subelem	subelement
 *	@param[out] anchor		barb location
 *	@param[out] *dir		direction
 *	@param[out] *speed		speed
 *	@param[out] *gust		gust
 *********************************************************************/
void recall_barb_value

	(
	BARB	barb,
	STRING	*subelem,
	POINT	anchor,
	float	*dir,
	float	*speed,
	float	*gust
	)

	{
	if (subelem) *subelem = NULL;
	if (dir)     *dir     = 0.0;
	if (speed)   *speed   = 0.0;
	if (gust)    *gust    = 0.0;
	if (anchor) copy_point(anchor, ZeroPoint);
	if (!barb) return;

	/* Retrieve location and value */
	get_default_attributes(barb->attrib, subelem, NULL, NULL);
	if (dir)     *dir     = barb->dir;
	if (speed)   *speed   = barb->speed;
	if (gust)    *gust    = barb->gust;
	if (anchor) copy_point(anchor, barb->anchor);
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ b a r b _ a t t r i b s                           *
*      r e c a l l _ b a r b _ a t t r i b s                           *
*                                                                      *
*      Set/reset or retrieve the attributes of the given barb.         *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set or reset the attributes of the given barb.
 *
 *	@param[in] 	barb	given barb
 *	@param[in] 	attribs	attributes
 *********************************************************************/
void	define_barb_attribs

	(
	BARB		barb,
	ATTRIB_LIST	attribs
	)

	{
	STRING	sub;
	/* Do nothing if barb not there */
	if (!barb) return;

	/* Set given attributes */
	barb->attrib = destroy_attrib_list(barb->attrib);
	if (NotNull(attribs)) barb->attrib = copy_attrib_list(attribs);

	/* >>> define sub, val, lab <<< */
	get_default_attributes(barb->attrib, &sub, NULL, NULL);
	barb->subelem = STRMEM(barb->subelem, sub);
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve the attributes of the given barb.
 *
 *	@param[in] 	barb		requested barb
 *	@param[out]	*attribs	attributes
 *********************************************************************/
void	recall_barb_attribs

	(
	BARB		barb,
	ATTRIB_LIST	*attribs
	)

	{
	/* Retrieve the attributes */
	if (attribs) *attribs = (barb) ? barb->attrib : NullAttribList;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ b a r b _ p s p e c                               *
*      r e c a l l _ b a r b _ p s p e c                               *
*                                                                      *
*      Set/reset or retrieve the presentation specs of the given barb. *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/**	Set or reset the presentation specs of the given barb.
 *
 *	@param[in] 	barb	given barb
 *	@param[in] 	param	parameter to set
 *	@param[in] 	value	value
 *********************************************************************/
void	define_barb_pspec

	(
	BARB	barb,
	PPARAM	param,
	POINTER	value
	)

	{
	/* Do nothing if barb does not exist */
	if (!barb) return;

	/* Set the given parameter */
	define_bspec_value(&barb->bspec, param, value);
	define_tspec_value(&barb->tspec, param, value);
	}

/*********************************************************************/
/**	Retrieve the presentation specs of the given barb.
 *
 *	@param[in] 	barb	given barb
 *	@param[in] 	param	parameter to retrieve
 *	@param[out]	value	value
 *********************************************************************/
void	recall_barb_pspec

	(
	BARB	barb,
	PPARAM	param,
	POINTER	value
	)

	{
	/* Do nothing if barb does not exist */
	if (!barb) return;

	/* Return the requested parameter */
	recall_bspec_value(&barb->bspec, param, value);
	recall_tspec_value(&barb->tspec, param, value);
	}
