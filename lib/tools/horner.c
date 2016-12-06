/*********************************************************************/
/** @file horner.c
 *
 * Contains code to perform efficient evaluation of univariate
 * polynomials.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
#include "horner.h"

/***********************************************************************
*                                                                      *
*      h o r n e r                                                     *
*                                                                      *
*      Horner's rule for evaluating a univariate polynomial.           *
*      Coefficients are in order of ascending powers of x.             *
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

/*********************************************************************/
/** Horner's rule for evaluating a univariate polynomial.
 * Coefficients are in order of ascending powers of x.
 *
 *	@param[in]	*coeffs		coefficients of function to be evaluated
 *	@param[in]	order		order of polynomial (How many terms)
 *	@param[in]	x			location at which to evaluat
 * 	@return solution to polynomial at a point.
 *********************************************************************/
float	horner

	(
	float	*coeffs,
	int		order,
	float	x
	)

	{
	float	val;

	/* Return 0 if undefined */
	if (!coeffs) return 0;
	if (order < 0) return 0;

	/* Special case where x=0 */
	if (x == 0) return coeffs[0];

	/* Evaluate using Horner's rule */
	val = coeffs[order--];
	while (order >= 0)
		val = val*x + coeffs[order--];

	return val;
	}
