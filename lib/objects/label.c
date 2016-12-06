/*********************************************************************/
/**	@file label.c
 *
 * Routines to handle the LABEL objects.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*      l a b e l . c                                                   *
*                                                                      *
*      Routines to handle the LABEL objects.                           *
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

#define LABEL_INIT
#include "label.h"

#include <fpa_getmem.h>

#include <string.h>

int		LabelCount = 0;

/***********************************************************************
*                                                                      *
*      c r e a t e _ l a b e l                                         *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Create a new label with given attributes.
 *
 *	@param[in] 	subelem		subelement
 *	@param[in] 	value		label value
 *	@param[in] 	text		label text
 *	@param[in] 	anchor		location of anchor point
 *	@param[in] 	angle		orientation
 *  @return Pointer to the new label object. You will need to destroy
 * 			this object when you are finished with it.
 *********************************************************************/

LABEL	create_label

	(
	STRING		subelem,
	STRING		value,
	STRING		text,
	const POINT	anchor,
	float		angle
	)

	{
	LABEL	label;

	/* Allocate space for the structure */
	label = INITMEM(struct LABEL_struct, 1);
	if (!label) return NullLabel;

	/* Initialize the structure */
	label->subelem = NULL;
	label->value   = NULL;
	label->label   = NULL;
	label->attrib  = NullAttribList;
	init_tspec(&label->tspec);

	/* Set label attributes and location */
	define_label_value(label, subelem, value, text);
	define_label_anchor(label, anchor, angle);

	/* Return the new label */
	LabelCount++;
	return label;
	}

/***********************************************************************
*                                                                      *
*      d e s t r o y _ l a b e l                                       *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Destroy space allocated to the given label.
 *
 *	@param[in] 	label	label to be destroyed
 *  @return NullLabel
 *********************************************************************/

LABEL	destroy_label

	(
	LABEL	label
	)

	{
	/* Do nothing if label not there */
	if (!label) return NullLabel;

	/* Free the space used by attributes */
	FREEMEM(label->subelem);
	FREEMEM(label->value);
	FREEMEM(label->label);
	destroy_attrib_list(label->attrib);

	/* Free the space used by presentation specs */
	free_tspec(&label->tspec);

	/* Free the structure itself */
	FREEMEM(label);
	LabelCount--;
	return NullLabel;
	}

/***********************************************************************
*                                                                      *
*      c o p y _ l a b e l                                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Make an exact copy of a label.
 *
 *	const LABEL	label	label to be copied
 *  @return Pointer to the copy of the given label. You will need to
 *			 destroy this object when you are finished with it.
 *********************************************************************/

LABEL	copy_label

	(
	const LABEL	label
	)

	{
	LABEL	copy;

	if (!label) return NullLabel;

	copy = create_label(NULL, NULL, label->label, label->anchor, label->angle);
	copy_tspec(&copy->tspec, &label->tspec);
	define_label_attribs(copy, label->attrib);
	return copy;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ l a b e l _ v a l u e                             *
*      r e c a l l _ l a b e l _ v a l u e                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Set or reset the attributes of the given label.
 *
 *	@param[in] 	label		given label
 *	@param[in] 	subelem		subelement
 *	@param[in] 	value		label value
 *	@param[in] 	text		label text
 *********************************************************************/

void	define_label_value

	(
	LABEL	label,
	STRING	subelem,
	STRING	value,
	STRING	text
	)

	{
	if (!label) return;

	/* Set all attributes */
	label->subelem = STRMEM(label->subelem, subelem);
	label->value   = STRMEM(label->value  , value);
	label->label   = STRMEM(label->label  , text);

	/* Replicate in attributes */
	if (IsNull(label->attrib)) label->attrib = create_attrib_list();
	add_default_attributes(label->attrib, subelem, value, text);
	}

/*********************************************************************/
/** Retrieve the attributes of the given label.
 *
 *	@param[in] 	label		given label
 *	@param[out]	*subelem	subelement
 *	@param[out]	*value		label value
 *	@param[out]	*text		label text
 *********************************************************************/
void	recall_label_value

	(
	LABEL	label,
	STRING	*subelem,
	STRING	*value,
	STRING	*text
	)

	{
	if (subelem) *subelem = NULL;
	if (value)   *value   = NULL;
	if (text)    *text    = NULL;
	if (IsNull(label)) return;

	/* Retrieve from attributes */
	get_default_attributes(label->attrib, subelem, value, text);
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ l a b e l _ a t t r i b s                         *
*      r e c a l l _ l a b e l _ a t t r i b s                         *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Set or reset the attributes of the given label.
 *
 *	@param[in] 	label	given label
 *	@param[in] 	attribs	attributes
 *********************************************************************/

void	define_label_attribs

	(
	LABEL		label,
	ATTRIB_LIST	attribs
	)

	{
	STRING	sub, val, lab;

	/* Do nothing if label not there */
	if (!label) return;

	/* Set given attributes */
	label->attrib = destroy_attrib_list(label->attrib);
	if (NotNull(attribs)) label->attrib = copy_attrib_list(attribs);

	/* >>> define sub, val, lab <<< */
	get_default_attributes(label->attrib, &sub, &val, &lab);
	label->subelem = STRMEM(label->subelem, sub);
	label->value   = STRMEM(label->value  , val);
	label->label   = STRMEM(label->label  , lab);
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve the attributes of the given label.
 *
 *	@param[in] 	label		requested label
 *	@param[out]	*attribs	attributes
 *********************************************************************/
void	recall_label_attribs

	(
	LABEL		label,
	ATTRIB_LIST	*attribs
	)

	{
	/* Retrieve the attributes */
	if (attribs) *attribs = (label) ? label->attrib : NullAttribList;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ l a b e l _ a n c h o r                           *
*      r e c a l l _ l a b e l _ a n c h o r                           *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Set or reset the location and orientation of the given label.
 *
 *	@param[in] 	lab			given label
 *	@param[in] 	anchor		label location
 *	@param[in] 	angle		label orientation
 *********************************************************************/

void	define_label_anchor

	(
	LABEL		lab,
	const POINT	anchor,
	float		angle
	)

	{
	if (!lab) return;

	/* Set all attributes */
	lab->angle = angle;
	if (anchor) copy_point(lab->anchor, anchor);
	else        copy_point(lab->anchor, ZeroPoint);
	}

/*********************************************************************/
/** Retrieve the location and orientation of the given label.
 *
 *	@param[in] 	lab		given label
 *	@param[in] 	anchor	label location
 *	@param[out]	*angle	label orientation
 *********************************************************************/
void	recall_label_anchor

	(
	LABEL	lab,
	POINT	anchor,
	float	*angle
	)

	{
	/* Retrieve all the attributes */
	if (angle) *angle   = (lab) ? lab->angle   : 0;
	if (anchor)
		{
		if (lab) copy_point(anchor, lab->anchor);
		else     copy_point(anchor, ZeroPoint);
		}
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ l a b e l _ p s p e c                             *
*      r e c a l l _ l a b e l _ p s p e c                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Set or reset the presentation specs of the given label.
 *
 *	@param[in] 	lab		given Label
 *	@param[in] 	param	parameter to set
 *	@param[in] 	value	value to set
 *********************************************************************/

void	define_label_pspec

	(
	LABEL	lab,
	PPARAM	param,
	POINTER	value
	)

	{
	/* Do nothing if label does not exist */
	if (!lab) return;

	/* Set the given parameter */
	define_tspec_value(&lab->tspec, param, value);
	}

/*********************************************************************/
/** Retrieve the presentation specs of the given label.
 *
 *	@param[in] 	lab		given Label
 *	@param[in] 	param	parameter to get
 *	@param[out]	value	value to get
 *********************************************************************/
void	recall_label_pspec

	(
	LABEL	lab,
	PPARAM	param,
	POINTER	value
	)

	{
	/* Do nothing if label does not exist */
	if (!lab) return;

	/* Return the requested parameter */
	recall_tspec_value(&lab->tspec, param, value);
	}
