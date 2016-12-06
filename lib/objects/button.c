/*********************************************************************/
/**	@file button.c
 *
 * Routines to handle the BUTTON object.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*      b u t t o n . c                                                 *
*                                                                      *
*      Routines to handle the BUTTON object.                           *
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

#define BUTTON_INIT
#include "button.h"

#include <tools/tools.h>
#include <fpa_getmem.h>
#include <fpa_math.h>
#include <string.h>

static	int		normal_font  = 4;
static	float	normal_size  = 0;
static	float	normal_angle = 0;
static	HJUST	normal_hjust = Hc;
static	VJUST	normal_vjust = Vc;

int		ButtonCount = 0;

/***********************************************************************
*                                                                      *
*      c r e a t e _ b u t t o n                                       *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Create a new button with given attributes.
 *
 *	@param[in] 	subelem		subelement (element/attribute/action)
 *	@param[in] 	value		text to be returned to program
 *	@param[in] 	label		text to appear inside the button
 *	@param[in]  	*box		button dimensions
 * 	@return Pointer to a new BUTTON object. You need to destroy this
 * 			object when you are finished with it.
 *********************************************************************/
BUTTON	create_button

	(
	STRING		subelem,
	STRING		value,
	STRING		label,
	const BOX	*box
	)

	{
	BUTTON	button;

	/* Allocate space for the structure */
	button = INITMEM(struct BUTTON_struct, 1);
	if (!button) return NullButton;

	/* Initialize button structure */
	button->attrib  = NullAttribList;
	button->subelem = NULL;
	button->label   = NULL;
	button->value   = NULL;
	button->options = NULL;
	button->numopt  = 0;
	button->maxopt  = 0;
	init_lspec(&button->lspec);
	init_fspec(&button->fspec);
	init_tspec(&button->tspec);

	/* Set button attributes and location */
	define_button_value(button, subelem, value);
	define_button_label(button, label, box);

	/* Return the new button */
	ButtonCount++;
	return button;
	}

/***********************************************************************
*                                                                      *
*      d e s t r o y _ b u t t o n                                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Destroy space allocated to the given button.
 *
 *	@param[in] 	button	button to be destroyed
 *  @return NullButton
 *********************************************************************/

BUTTON	destroy_button

	(
	BUTTON	button
	)

	{
	int		i;
	TABLE	*opt;

	/* Do nothing if button not there */
	if (!button) return NullButton;

	/* Free the space used by attributes */
	FREEMEM(button->subelem);
	FREEMEM(button->label);
	FREEMEM(button->value);
	destroy_attrib_list(button->attrib);

	/* Free space used by option list */
	opt = button->options;
	for (i=0; i<button->numopt; i++)
		{
		FREEMEM(opt[i].index);
		FREEMEM(opt[i].value);
		}

	/* Free the space used by presentation specs */
	free_lspec(&button->lspec);
	free_fspec(&button->fspec);
	free_tspec(&button->tspec);

	/* Free space for structure */
	FREEMEM(button);
	ButtonCount--;
	return NullButton;
	}

/***********************************************************************
*                                                                      *
*      c o p y _ b u t t o n                                           *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Make an exact copy of a button.
 *
 *	@param[in] 	button	button to be copied
 * 	@return Pointer to copy of given button. You need to destroy this
 * 			object when you are finished with it.
 *********************************************************************/

BUTTON	copy_button

	(
	const BUTTON	button
	)

	{
	int		i;
	TABLE	*opt;
	BUTTON	copy;

	if (!button) return NullButton;

	copy = create_button(NULL, NULL, button->label, &button->box);

	copy_lspec(&copy->lspec, &button->lspec);
	copy_fspec(&copy->fspec, &button->fspec);
	copy_tspec(&copy->tspec, &button->tspec);
	define_button_attribs(copy, button->attrib);

	opt = button->options;
	for (i=0; i<button->numopt; i++)
		add_option_to_button(copy, opt[i].index, opt[i].value);

	return copy;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ b u t t o n _ v a l u e                           *
*      r e c a l l _ b u t t o n _ v a l u e                           *
*                                                                      *
*      Set/reset or retrieve the value of the given button.            *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set or reset the value of the given button.
 *
 *	@param[in] 	button		given button
 *	@param[in] 	subelem 	subelement
 *	@param[in] 	value		text to be used by program
 *********************************************************************/
void	define_button_value

	(
	BUTTON	button,
	STRING	subelem,
	STRING	value
	)

	{
	if (!button) return;

	/* Allocate memory and assign character attributes */
	button->subelem = STRMEM(button->subelem, subelem);
	button->value   = STRMEM(button->value, value);

	/* Replicate in attributes */
	if (IsNull(button->attrib)) button->attrib = create_attrib_list();
	add_default_attributes(button->attrib, subelem, value, NULL);
	}

/*********************************************************************/
/** Retrieve the value of the given button.
 *
 *	@param[in] 	button		given button
 *	@param[in] 	*subelem	subelement
 *	@param[in] 	*value		text to be used by program
 *********************************************************************/
void	recall_button_value

	(
	BUTTON	button,
	STRING	*subelem,
	STRING	*value
	)

	{
	if (subelem) *subelem = NULL;
	if (value)   *value   = NULL;
	if (!button) return;

	/* Retrieve all the attributes */
	get_default_attributes(button->attrib, subelem, value, NULL);
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ b u t t o n _ a t t r i b s                       *
*      r e c a l l _ b u t t o n _ a t t r i b s                       *
*                                                                      *
*      Set/reset or retrieve the attributes of the given button.       *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set or reset the attributes of the given button.
 *
 *	@param[in] 	button	given button
 *	@param[in] 	attribs	attributes
 *********************************************************************/
void	define_button_attribs

	(
	BUTTON		button,
	ATTRIB_LIST	attribs
	)

	{
	STRING	sub, val;
	/* Do nothing if button not there */
	if (!button) return;

	/* Set given attributes */
	button->attrib = destroy_attrib_list(button->attrib);
	if (NotNull(attribs)) button->attrib = copy_attrib_list(attribs);

	/* >>> define sub, val, lab <<< */
	get_default_attributes(button->attrib, &sub, &val, NULL);
	button->subelem = STRMEM(button->subelem, sub);
	button->value   = STRMEM(button->value,   val);
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve the attributes of the given button.
 *
 *	@param[in] 	button		requested button
 *	@param[in] 	*attribs	attributes
 *********************************************************************/
void	recall_button_attribs

	(
	BUTTON		button,
	ATTRIB_LIST	*attribs
	)

	{
	/* Retrieve the attributes */
	if (attribs) *attribs = (button) ? button->attrib : NullAttribList;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ b u t t o n _ l a b e l                           *
*      r e c a l l _ b u t t o n _ l a b e l                           *
*                                                                      *
*      Set/reset or retriev the label and box of the given button.     *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set or reset the label of the given button.
 *
 *	@param[in] 	button		given button
 *	@param[in] 	label		text to appear on screen
 *	@param[in]  	*box		button dimensions
 *********************************************************************/
void	define_button_label

	(
	BUTTON		button,
	STRING		label,
	const BOX	*box
	)

	{
	float	x, y;

	if (!button) return;
	if (!box) return;

	/* Allocate memory and assign attributes */
	button->label = STRMEM(button->label, label);
	copy_box(&button->box, box);
	x = 0.5*(box->right + box->left);
	y = 0.5*(box->top + box->bottom);
	set_point(button->lpos, x, y);

	/* Override certain text presentation attributes */
	define_button_pspec(button, TEXT_FONT,  (POINTER) &normal_font);
	define_button_pspec(button, TEXT_ANGLE, (POINTER) &normal_angle);
	define_button_pspec(button, TEXT_SIZE,  (POINTER) &normal_size);
	define_button_pspec(button, TEXT_HJUST, (POINTER) &normal_hjust);
	define_button_pspec(button, TEXT_VJUST, (POINTER) &normal_vjust);
	}

/*********************************************************************/
/** Retrieve the label of the given button.
 *
 *	@param[in] 	button		given button
 *	@param[in] 	*label		text to appear on screen
 *	@param[in] 	**box		button dimensions
 *********************************************************************/
void	recall_button_label

	(
	BUTTON	button,
	STRING	*label,
	BOX		**box
	)

	{
	/* Retrieve all the attributes */
	if (label) *label = (button) ? button->label : NULL;
	if (box)   *box   = (button) ? &button->box  : NullBox;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ b u t t o n _ p s p e c                           *
*      r e c a l l _ b u t t o n _ p s p e c                           *
*                                                                      *
*      Set/reset or retrieve the presentation specs of the given       *
*      button.                                                         *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set or reset the presentation specs of the given button.
 *	@param[in] 	button	given button
 *	@param[in] 	param	Presentation spec
 *	@param[in] 	value	value of spec
 *********************************************************************/
void	define_button_pspec

	(
	BUTTON	button,
	PPARAM	param,
	POINTER	value
	)

	{
	int		len;
	float	xsize, ysize, size;

	/* Do nothing if button does not exist */
	if (!button) return;

	/* Compute automatic label size if necessary */
	if (param == TEXT_SIZE)
		{
		size = *(float *)value;
		if (size <= normal_size)
			{
			size = normal_size;
			if (!blank(button->label))
				{
				xsize  = fabs(button->box.right - button->box.left);
				ysize  = fabs(button->box.top - button->box.bottom);
				len    = strlen(button->label);
				xsize *= 1.42/len;
				ysize *= 0.80;
				size   = MIN(xsize, ysize);
				}
			value = (POINTER) &size;
			}
		}

	/* Set the given parameter */
	define_lspec_value(&button->lspec, param, value);
	define_fspec_value(&button->fspec, param, value);
	define_tspec_value(&button->tspec, param, value);
	}

/*********************************************************************/
/** Retrieve the presentation specs of the given button.
 *
 *	@param[in] 	button	given button
 *	@param[in] 	param	Presentation spec
 *	@param[in] 	value	value of spec
 *********************************************************************/
void	recall_button_pspec

	(
	BUTTON	button,
	PPARAM	param,
	POINTER	value
	)

	{
	/* Do nothing if button does not exist */
	if (!button) return;

	/* Return the requested parameter */
	recall_lspec_value(&button->lspec, param, value);
	recall_fspec_value(&button->fspec, param, value);
	recall_tspec_value(&button->tspec, param, value);
	}

/***********************************************************************
*                                                                      *
*      a d d _ o p t i o n _ t o _ b u t t o n                         *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Add the given option to the option list of the given button.
 *
 *	@param[in] 	button	given button
 *	@param[in] 	optnam	option index
 *	@param[in] 	optval	option value
 *********************************************************************/

void	add_option_to_button

	(
	BUTTON	button,
	STRING	optnam,
	STRING	optval
	)

	{
	TABLE	*opt;

	/* Do noting if button not given */
	if (!button) return;

	/* Make sure there is enough space allocated */
	if (button->numopt >= button->maxopt)
		{
		button->maxopt += DELTA_OPTION;
		button->options = GETMEM(button->options, TABLE, button->maxopt);
		}

	opt = button->options + button->numopt++;
	opt->index = INITSTR(optnam);
	opt->value = INITSTR(optval);
	}

/***********************************************************************
*                                                                      *
*      s e a r c h _ b u t t o n _ o p t i o n                         *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Scan the option list of the given button for the specified option.
 *
 *	@param[in] 	button	given button
 *	@param[in] 	optnam	specified option index
 * 	@return Value of specified option. Null if it can't be found.
 *********************************************************************/

STRING	search_button_option

	(
	BUTTON	button,
	STRING	optnam
	)

	{
	int		i;
	TABLE	*opt;

	/* Do nothing if no button or option given */
	if (!button)       return NULL;
	if (blank(optnam)) return NULL;

	/* Search option list */
	opt = button->options;
	for (i=0; i<button->numopt; i++)
		{
		if (same(optnam, opt[i].index))
			{
			/* Found it */
			return opt[i].value;
			}
		}

	/* Not found */
	return NULL;
	}

/***********************************************************************
*                                                                      *
*      i n s i d e _ b u t t o n                                       *
*      i n s i d e _ b u t t o n _ x y                                 *
*                                                                      *
*      Compute whether the given point is within the given button.     *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/**	Compute whether the given point is within the given button.
 *
 * Point passed as a POINT object.
 *
 *	@param[in] 	button	button
 *	@param[in] 	ptest	specified point
 * 	@return True if point is within button.
 *********************************************************************/
LOGICAL	inside_button

	(
	BUTTON	button,
	POINT	ptest
	)

	{
	/* Do nothing if button not there */
	if (!ptest) return FALSE;
	if (!button) return FALSE;
	return inside_box(&button->box, ptest);
	}

/*********************************************************************/
/**	Compute whether the given point is within the given button.
 *
 * Point passed as a x/y pair.
 *
 * 	@param[in] 	button 	button
 *	@param[in] 	x 		specified x-coord
 *	@param[in] 	y		specified y-ccord
 * 	@return True if point is within button.
 *********************************************************************/
LOGICAL	inside_button_xy

	(
	BUTTON	button,
	float	x,
	float	y
	)

	{
	/* Do nothing if button not there */
	if (!button) return FALSE;
	return inside_box_xy(&button->box, x, y);
	}
