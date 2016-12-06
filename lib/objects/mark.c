/*********************************************************************/
/**	@file mark.c
 *
 * Routines to handle the MARK objects.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*      m a r k . c                                                     *
*                                                                      *
*      Routines to handle the MARK objects.                            *
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

#define MARK_INIT
#include "mark.h"

#include <fpa_getmem.h>

#include <string.h>

int		MarkCount = 0;

/***********************************************************************
*                                                                      *
*      c r e a t e _ m a r k                                           *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Create a new mark with given attributes.
 *
 *	@param[in] 	subelem		subelement
 *	@param[in] 	value		value
 *	@param[in] 	label		label
 *	@param[in] 	anchor		location of anchor point
 *	@param[in] 	angle		orientation
 *  @return Pointer to a new mark object. You will need to destroy
 * 			this object when you are finished with it.
 *********************************************************************/

MARK	create_mark

	(
	STRING		subelem,
	STRING		value,
	STRING		label,
	const POINT	anchor,
	float		angle
	)

	{
	MARK	mark;

	/* Allocate space for the structure */
	mark = INITMEM(struct MARK_struct, 1);
	if (!mark) return NullMark;

	/* Initialize mark structure */
	mark->attrib  = NullAttribList;
	mark->subelem = NULL;
	mark->value   = NULL;
	mark->label   = NULL;
	init_mspec(&mark->mspec);

	/* Set mark attributes and location */
	define_mark_value(mark, subelem, value, label);
	define_mark_anchor(mark, anchor, angle);

	/* Return the new mark */
	MarkCount++;
	return mark;
	}

/***********************************************************************
*                                                                      *
*      d e s t r o y _ m a r k                                         *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Destroy space allocated to the given mark.
 *
 *	@param[in] 	mark	mark to be destroyed
 *  @return NullMark
 *********************************************************************/

MARK	destroy_mark

	(
	MARK	mark
	)

	{
	/* Do nothing if mark not there */
	if (!mark) return NullMark;

	/* Free the space used by attributes */
	FREEMEM(mark->subelem);
	FREEMEM(mark->value);
	FREEMEM(mark->label);
	destroy_attrib_list(mark->attrib);

	/* Free the space used by presentation specs */
	free_mspec(&mark->mspec);

	/* Free the structure itself */
	FREEMEM(mark);
	MarkCount--;
	return NullMark;
	}

/***********************************************************************
*                                                                      *
*      c o p y _ m a r k                                               *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Make an exact copy of a mark.
 *
 *	@param[in] 	mark	mark to be copied
 *  @return Pointer to copy of given mark object. You will need to
 * 			destroy this object when you are finished with it.
 *********************************************************************/

MARK	copy_mark

	(
	const MARK	mark
	)

	{
	MARK	copy;

	if (!mark) return NullMark;

	copy = create_mark(NULL, NULL, NULL, mark->anchor, mark->angle);
	copy_mspec(&copy->mspec, &mark->mspec);
	define_mark_attribs(copy, mark->attrib);
	return copy;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ m a r k _ v a l u e                               *
*      r e c a l l _ m a r k _ v a l u e                               *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Set or reset the attributes of the given mark.
 *
 *	@param[in] 	mark		given mark
 *	@param[in] 	subelem		subelement
 *	@param[in] 	value		value
 *	@param[in] 	label		label
 *********************************************************************/

void	define_mark_value

	(
	MARK	mark,
	STRING	subelem,
	STRING	value,
	STRING	label
	)

	{
	if (!mark) return;

	/* Set all attributes */
	mark->subelem = STRMEM(mark->subelem, subelem);
	mark->value   = STRMEM(mark->value  , value);
	mark->label   = STRMEM(mark->label  , label);

	/* Replicate in attributes */
	if (IsNull(mark->attrib)) mark->attrib = create_attrib_list();
	add_default_attributes(mark->attrib, subelem, value, label);
	}

/*********************************************************************/
/** Retrieve the attributes of the given mark.
 *
 *	@param[in] 	mark		given mark
 *	@param[out]	*subelem	subelement
 *	@param[out]	*value		value
 *	@param[out]	*label		label
 *********************************************************************/
void	recall_mark_value

	(
	MARK	mark,
	STRING	*subelem,
	STRING	*value,
	STRING	*label
	)

	{
	if (subelem) *subelem = NULL;
	if (value)   *value   = NULL;
	if (label)   *label   = NULL;
	if (!mark)   return;

	/* Retrieve from attributes */
	get_default_attributes(mark->attrib, subelem, value, label);
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ m a r k _ a t t r i b s                           *
*      r e c a l l _ m a r k _ a t t r i b s                           *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Set or reset the attributes of the given mark.
 *
 *	@param[in] 	mark	given mark
 *	@param[in] 	attribs	attributes
 *********************************************************************/

void	define_mark_attribs

	(
	MARK		mark,
	ATTRIB_LIST	attribs
	)

	{
	STRING	sub, val, lab;

	/* Do nothing if mark not there */
	if (!mark) return;

	/* Set given attributes */
	mark->attrib = destroy_attrib_list(mark->attrib);
	if (NotNull(attribs)) mark->attrib = copy_attrib_list(attribs);

	/* >>> define sub, val, lab <<< */
	get_default_attributes(mark->attrib, &sub, &val, &lab);
	mark->subelem = STRMEM(mark->subelem, sub);
	mark->value   = STRMEM(mark->value  , val);
	mark->label   = STRMEM(mark->label  , lab);
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve the attributes of the given mark.
 *
 *	@param[in] 	mark		requested mark
 *	@param[out]	*attribs	attributes
 *********************************************************************/
void	recall_mark_attribs

	(
	MARK		mark,
	ATTRIB_LIST	*attribs
	)

	{
	/* Retrieve the attributes */
	if (attribs) *attribs = (mark) ? mark->attrib : NullAttribList;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ m a r k _ a n c h o r                             *
*      r e c a l l _ m a r k _ a n c h o r                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Set or reset the location and orientation of the given mark.
 *
 *	@param[in] 	mark		given mark
 *	@param[in] 	anchor		mark location
 *	@param[in] 	angle		mark orientation
 *********************************************************************/

void	define_mark_anchor

	(
	MARK		mark,
	const POINT	anchor,
	float		angle
	)

	{
	if (!mark) return;

	/* Set all attributes */
	mark->angle = angle;
	if (anchor) copy_point(mark->anchor, anchor);
	else        copy_point(mark->anchor, ZeroPoint);
	}

/*********************************************************************/
/** Retrieve the location and orientation of the given mark.
 *
 *	@param[in] 	mark		given mark
 *	@param[out]	anchor		mark location
 *	@param[out]	angle		mark orientation
 *********************************************************************/
void	recall_mark_anchor

	(
	MARK	mark,
	POINT	anchor,
	float	*angle
	)

	{
	/* Retrieve all the attributes */
	if (angle) *angle = (mark) ? mark->angle : 0;
	if (anchor)
		{
		if (mark) copy_point(anchor, mark->anchor);
		else      copy_point(anchor, ZeroPoint);
		}
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ m a r k _ p s p e c                               *
*      r e c a l l _ m a r k _ p s p e c                               *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Set or reset the presentation specs of the given mark.
 *
 *	@param[in] 	mark		given mark
 *	@param[in] 	param		parameter to set
 *	@param[in] 	value		value to set parameter
 *********************************************************************/

void	define_mark_pspec

	(
	MARK	mark,
	PPARAM	param,
	POINTER	value
	)

	{
	/* Do nothing if mark does not exist */
	if (!mark) return;

	/* Set the given parameter */
	define_mspec_value(&mark->mspec, param, value);
	}

/*********************************************************************/
/** Retrieve the presentation specs of the given mark.
 *
 *	@param[in] 	mark		given mark
 *	@param[in] 	param		parameter to set
 *	@param[out]	value		value to get
 *********************************************************************/
void	recall_mark_pspec

	(
	MARK	mark,
	PPARAM	param,
	POINTER	value
	)

	{
	/* Do nothing if mark does not exist */
	if (!mark) return;

	/* Return the requested parameter */
	recall_mspec_value(&mark->mspec, param, value);
	}
