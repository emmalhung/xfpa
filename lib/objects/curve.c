/*********************************************************************/
/**	@file curve.c
 *
 * Routines to handle the CURVE objects.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*      c u r v e . c                                                   *
*                                                                      *
*      Routines to handle the CURVE objects.                           *
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

#define CURVE_INIT
#include "curve.h"

#include <fpa_getmem.h>
#include <fpa_math.h>
#include <string.h>

int		CurveCount = 0;

/***********************************************************************
*                                                                      *
*      c r e a t e _ c u r v e                                         *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Create a new curve with given attributes.
 *
 *	@param[in] 	subelem		subelement
 *	@param[in] 	value		curve value
 *	@param[in] 	label		curve label
 * 	@return Pointer to new curve object. You need to destroy this
 * 			object when you are finished with it.
 *********************************************************************/

CURVE	create_curve

	(
	STRING	subelem,
	STRING	value,
	STRING	label
	)


	{
	CURVE	cnew;

	/* allocate space for the structure */
	cnew = INITMEM(struct CURVE_struct, 1);
	if (!cnew) return NullCurve;

	/* Initialize point buffer */
	cnew->attrib  = NullAttribList;
	cnew->subelem = NULL;
	cnew->value   = NULL;
	cnew->label   = NULL;
	cnew->line    = NullLine;
	init_lspec(&cnew->lspec);

	/* Set curve attributes */
	define_curve_sense(cnew, Right);	/* Must be right or left only */
	define_curve_value(cnew, subelem, value, label);

	/* Return the new curve */
	CurveCount++;
	return cnew;
	}

/***********************************************************************
*                                                                      *
*      d e s t r o y _ c u r v e                                       *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Destroy space allocated to the given curve.
 *
 *	@param[in] 	curve	curve to be destroyed
 * @return NullCurve
 *********************************************************************/
CURVE	destroy_curve

	(
	CURVE	curve
	)

	{
	/* Do nothing if curve not there */
	if (!curve) return NullCurve;

	/* Free the space used by attributes */
	FREEMEM(curve->subelem);
	FREEMEM(curve->value);
	FREEMEM(curve->label);
	destroy_attrib_list(curve->attrib);

	/* Free the space used by point buffer */
	destroy_line(curve->line);

	/* Free the space used by presentation specs */
	free_lspec(&curve->lspec);

	FREEMEM(curve);
	CurveCount--;
	return NullCurve;
	}

/***********************************************************************
*                                                                      *
*      c o p y _ c u r v e                                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Make an exact copy of a curve.
 *
 *	@param[in] 	curve	curve to be copied
 * 	@return Pointer to the new curve. You need to destroy this object
 * 			when you are finished with it.
 *********************************************************************/

CURVE	copy_curve

	(
	const CURVE	curve
	)

	{
	CURVE	copy;

	if (!curve) return NullCurve;

	copy        = create_curve(NULL, NULL, NULL);
	copy->line  = copy_line(curve->line);
	define_curve_sense(copy, curve->sense);
	copy_lspec(&copy->lspec, &curve->lspec);
	define_curve_attribs(copy, curve->attrib);
	return copy;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ c u r v e _ s e n s e                             *
*      r e c a l l _ c u r v e _ s e n s e                             *
*                                                                      *
*      Set/reset or retrieve the handedness of the given curve.        *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set or rest the handedness of the given curve.
 *
 *	@param[in] 	curve		given curve
 *	@param[in] 	sense		handedness
 *********************************************************************/
void	define_curve_sense

	(
	CURVE	curve,
	HAND	sense
	)

	{
	if (!curve) return;

	/* Set the sense only if legal */
	switch (sense)
		{
		case Right:
		case Left:
					curve->sense = sense;
					break;
		}
	}

/*********************************************************************/
/** Retrieve the handedness of the given curve.
 *
 *	@param[in] 	curve	requested curve pointer
 *	@param[out] 	*sense	handedness
 *********************************************************************/
void	recall_curve_sense

	(
	CURVE	curve,
	HAND	*sense
	)

	{
	/* Retrieve the sense */
	if (sense) *sense = (curve) ? curve->sense : Ambi;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ c u r v e _ v a l u e                             *
*      r e c a l l _ c u r v e _ v a l u e                             *
*                                                                      *
*      Set/reset or retrieve the attributes of the given curve.        *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set or reset the value of the attributes of the given curve.
 *
 *	@param[in] 	curve		given curve
 *	@param[in] 	subelem 	subelement
 *	@param[in] 	value		curve value
 *	@param[in] 	label		curve label
 *********************************************************************/
void	define_curve_value

	(
	CURVE	curve,
	STRING	subelem,
	STRING	value,
	STRING	label
	)

	{
	if (!curve) return;

	/* Set all attributes */
	curve->subelem = STRMEM(curve->subelem, subelem);
	curve->value   = STRMEM(curve->value  , value);
	curve->label   = STRMEM(curve->label  , label);

	/* Replicate in attributes */
	if (IsNull(curve->attrib)) curve->attrib = create_attrib_list();
	add_default_attributes(curve->attrib, subelem, value, label);
	}

/*********************************************************************/
/** Retrieve the  value of theattributes of the given curve.
 *
 *	@param[in] 	curve		requested curve pointer
 *	@param[out] 	*subelem	subelement
 *	@param[out] 	*value		curve value
 *	@param[out] 	*label		curve label
 *********************************************************************/
void	recall_curve_value

	(
	CURVE	curve,
	STRING	*subelem,
	STRING	*value,
	STRING	*label
	)

	{
	if (subelem) *subelem = NULL;
	if (value)   *value   = NULL;
	if (label)   *label   = NULL;
	if (!curve) return;

	/* Retrieve all the attributes */
	get_default_attributes(curve->attrib, subelem, value, label);
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ c u r v e _ a t t r i b s                         *
*      r e c a l l _ c u r v e _ a t t r i b s                         *
*                                                                      *
*      Set/reset or retrieve the attributes of the given curve.        *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set or reset the attributes of the given curve.
 *
 *	@param[in] 	curve	given curve
 *	@param[in] 	attribs	attributes
 *********************************************************************/
void	define_curve_attribs

	(
	CURVE		curve,
	ATTRIB_LIST	attribs
	)

	{
	STRING	sub, val, lab;

	/* Do nothing if curve not there */
	if (!curve) return;

	/* Set given attributes */
	curve->attrib = destroy_attrib_list(curve->attrib);
	if (NotNull(attribs)) curve->attrib = copy_attrib_list(attribs);

	/* >>> define sub, val, lab <<< */
	get_default_attributes(curve->attrib, &sub, &val, &lab);
	curve->subelem = STRMEM(curve->subelem, sub);
	curve->value   = STRMEM(curve->value  , val);
	curve->label   = STRMEM(curve->label  , lab);
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve the attributes of the given curve.
 *
 *	@param[in] 	curve		requested curve
 *	@param[out] 	*attribs	attributes
 *********************************************************************/
void	recall_curve_attribs

	(
	CURVE		curve,
	ATTRIB_LIST	*attribs
	)

	{
	/* Retrieve the attributes */
	if (attribs) *attribs = (curve) ? curve->attrib : NullAttribList;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ c u r v e _ p s p e c                             *
*      r e c a l l _ c u r v e _ p s p e c                             *
*                                                                      *
*      Set/reset or retrieve the presentation specs of the given       *
*      curve.                                                          *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set or reset the presentation specs of the given curve.
 *
 *	@param[in] 	curve	given curve
 *	@param[in] 	param	parameter to define
 *	@param[in] 	value	value to assigne param
 *********************************************************************/
void	define_curve_pspec

	(
	CURVE	curve,
	PPARAM	param,
	POINTER	value
	)

	{
	/* Do nothing if curve does not exist */
	if (!curve) return;

	/* Set the given parameter */
	define_lspec_value(&curve->lspec, param, value);
	}

/*********************************************************************/
/** Retrieve the presentation specs of the given curve.
 *
 *	@param[in] 	curve	given curve
 *	@param[in] 	param	parameter to look up
 *	@param[out] 	value	value of parameter
 *********************************************************************/
void	recall_curve_pspec

	(
	CURVE	curve,
	PPARAM	param,
	POINTER	value
	)

	{
	/* Do nothing if curve does not exist */
	if (!curve) return;

	/* Return the requested parameter */
	recall_lspec_value(&curve->lspec, param, value);
	}

/***********************************************************************
*                                                                      *
*      h i g h l i g h t _ c u r v e                                   *
*      w i d e n _ c u r v e                                           *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set the highlight flag for the given curve.
 *
 *	@param[in] 	curve	given curve
 *	@param[in] 	code	hilite code
 *********************************************************************/
void	highlight_curve

	(
	CURVE	curve,
	HILITE	code
	)

	{
	/* Do nothing if no curve */
	if (!curve) return;

	if (code != SkipHilite)
		define_lspec_value(&curve->lspec, LINE_HILITE, (POINTER)&code);
	}

/*********************************************************************/
/** Widen the curve by delta.
 *
 * @note a negative value means to erase.
 *
 *	@param[in] 	curve	given curve
 *	@param[in] 	delta	this value is added to the current width
 *********************************************************************/
void	widen_curve

	(
	CURVE	curve,
	float	delta
	)

	{
	float	width;

	/* Do nothing if no curve */
	if (!curve) return;

	/* Widen simple curves by delta - patterned curves by 1.25 */
	recall_lspec_value(&curve->lspec, LINE_WIDTH, (POINTER)&width);
	if (!curve->lspec.pattern) width += delta;
	else if (delta > 0.0)      width *= 1.25;
	else if (delta < 0.0)      width /= 1.25;
	define_lspec_value(&curve->lspec, LINE_WIDTH, (POINTER)&width);
	}

/***********************************************************************
*                                                                      *
*      e m p t y _ c u r v e                                           *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Zero the point buffer of the given curve.
 *
 *	@param[in] 	curve	curve to be emptied
 *********************************************************************/
void	empty_curve

	(
	CURVE	curve
	)

	{
	/* Do nothing if curve not there */
	if (!curve) return;

	/* Zero the point counter */
	empty_line(curve->line);
	}

/***********************************************************************
*                                                                      *
*      a p p e n d _ c u r v e                                         *
*      a d d _ l i n e _ t o _ c u r v e                               *
*      a d d _ p o i n t _ t o _ c u r v e                             *
*                                                                      *
*      Append points to the end of a curve.                            *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Append the points of the second curve to the first curve.
 *
 *	@param[in] 	c1	curve to append the other one to
 *	@param[in] 	c2	curve to be appended
 * 	@return Pointer to the resulting curve.
 *********************************************************************/
CURVE	append_curve

	(
	CURVE		c1,
	const CURVE	c2
	)

	{
	/* Do nothing if c2 not there */
	if (!c2)       return c1;
	if (!c2->line) return c1;

	/* If c1 not there, return a copy of c2 */
	if (!c1) return copy_curve(c2);

	/* Now append the points from c2 to c1 */
	if (!c1->line) c1->line = create_line();
	append_line(c1->line, c2->line);
	return c1;
	}

/**********************************************************************/

/*********************************************************************/
/**	Append the points of the line to the curve.
 *
 *	@param[in] 	curve	curve to append the points to
 *	@param[in] 	line	line to be appended
 *********************************************************************/
void	add_line_to_curve

	(
	CURVE		curve,
	const LINE	line
	)

	{
	/* Do nothing if curve or line not there */
	if (!curve) return;
	if (!line)  return;

	/* Now append the points from line to curve */
	if (!curve->line) curve->line = create_line();
	append_line(curve->line, line);
	}

/**********************************************************************/

/*********************************************************************/
/** Append a single point to the curve.
 *
 *	@param[in] 	curve	curve to add point to
 *	@param[in] 	p	    point to add to curve
 *********************************************************************/
void	add_point_to_curve

	(
	CURVE	curve,
	POINT	p
	)

	{
	/* Do nothing if curve not there */
	if (!p)     return;
	if (!curve) return;

	/* Copy the given point onto the point buffer */
	if (!curve->line) curve->line = create_line();
	add_point_to_line(curve->line, p);
	}

/***********************************************************************
*                                                                      *
*     r e v e r s e _ c u r v e                                        *
*     f l i p _ c u r v e                                              *
*                                                                      *
*     Reverse the point order or sense of the given curve.             *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Reverse the point order of the given curve.
 *
 *	@param[in] 	curve	given curve
 *********************************************************************/
void	reverse_curve

	(
	CURVE	curve
	)

	{
	/* Do nothing if curve not there */
	if (!curve) return;

	/* Reverse the points */
	reverse_line(curve->line);
	flip_curve(curve);
	}

/*********************************************************************/
/**	Reverse the "sense" or "handedness" of the given curve.
 *
 *	@param[in] 	curve	given curve
 *********************************************************************/
void	flip_curve

	(
	CURVE	curve
	)

	{
	/* Do nothing if curve not there */
	if (!curve) return;

	/* Reverse the sense */
	switch (curve->sense)
		{
		case Left:	curve->sense = Right;	break;
		case Right:	curve->sense = Left;	break;
		}
	}
