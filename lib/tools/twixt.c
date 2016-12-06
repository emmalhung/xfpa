/*********************************************************************/
/** @file twixt.c
 *
 * 1-D non-uniform linear curve interpolatoin.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*  1-D non-uniform linear curve interpolation.                         *
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

#include "twixt.h"

#include <fpa_getmem.h>

#include <stdio.h>

static	void	Spline(int, double *, double *, double *);
static	int		Sinterval(int, double, double *);
static	double	Seval(int, double, double *, double *, double *);

/*******************************************************************************
*                                                                              *
*  T w i x t                                                                   *
*                                                                              *
*******************************************************************************/
/*********************************************************************/
/** Interpolate (extrapolate) from a list of values given at
 * monotonically increasing parameter values, to a given set of
 * parameter values.
 *	@param[in]	numkeys 	number of key points
 *	@param[in]	*keylist	list of key points
 *	@param[in]	*keyval	    list of key values
 *	@param[in]	numtweens	number of between points
 *	@param[in]	*tweenlist  list of between points
 *	@param[out]	*tweenval	list of between values
 *********************************************************************/

void	Twixt

	(
	int		numkeys,
	double	*keylist,
	double	*keyval,
	int		numtweens,
	double	*tweenlist,
	double	*tweenval
	)

	{
	int		morekeys, ikey, itween;
	double	*morelist, *morevals;
	double	*B;
	double	parm;

	if (!keylist || !keyval || !tweenlist) return;

	/* Allocate storage for extended list of keys */
	morekeys = numkeys + 2;
	morelist = INITMEM(double, morekeys);
	morevals = INITMEM(double, morekeys);

	/* Set interior values from keylist and keyval */
	for ( ikey=0; ikey<numkeys; ikey++ )
		{
		morelist[ikey+1] = keylist[ikey];
		morevals[ikey+1] = keyval[ikey];
		}

	/* Extrapolate to get values at beginning and end */
	morelist[0] = keylist[0];
	morevals[0] = keyval[0];
	morelist[morekeys-1] = keylist[numkeys-1];
	morevals[morekeys-1] = keyval[numkeys-1];

	/* Allocate storage for spline interpolation routines */
	B = INITMEM(double, morekeys);

	/* Fit the given values by cubic spline interpolation */
	Spline( morekeys, morelist, morevals, B);

	/* Evaluate the inbetween values and derivatives */
	for( itween=0; itween<numtweens; itween++ )
		{
		parm = tweenlist[itween];
		if (tweenval)
			tweenval[itween] = Seval( morekeys, parm, morelist, morevals, B);
		}

	FREEMEM(morelist);
	FREEMEM(morevals);
	FREEMEM(B);
	}

/*******************************************************************************
*                                                                              *
*  S p l i n e                                                                 *
*                                                                              *
*  The coefficients a[i], i=0,2,...,n-1 are computed for a linear              *
*  interpolating spline, as in:                                                *
*                                                                              *
*     s(t) = coord[i] + a[i]*(t-t[i])                                          *
*                                                                              *
*     for  t[i] .LE. t .LE. t[i+1]                                             *
*                                                                              *
*  Input:                                                                      *
*                                                                              *
*     n = the number of data points or knots (n.GE.2)                          *
*     t = the abscissas of the knots in strictly increasing order              *
*     coord = the ordinates of the knots                                       *
*                                                                              *
*  Output:                                                                     *
*                                                                              *
*     a  = array of spline coefficients as defined above                       *
*                                                                              *
*  Using  '  to denote differentiation,                                        *
*                                                                              *
*     coord[i] = s(t[i])                                                       *
*     a[i] = s'(t[i])                                                          *
*                                                                              *
*  The accompanying function Seval() can be used to evaluate the spline.       *
*                                                                              *
*******************************************************************************/

static	void	Spline

	(
	int		n,
	double	*t,
	double	*coord,
	double	*a
	)

	{
	int		i;

	if (!t || !coord || !a) return;

	a[0] = 0;
	for (i=1; i<n-2; i++)
		{
		a[i] = (coord[i+1]-coord[i])/(t[i+1]-t[i]);
		}
	a[n-2] = 0;
	a[n-1] = 0;
	}

/*******************************************************************************
*                                                                              *
*  S e v a l                                                                   *
*                                                                              *
*  These subroutines evaluate the linear spline function                       *
*                                                                              *
*     SEVAL = coord[I]                                                         *
*           + a[I]*(arg-t[I])                                                  *
*                                                                              *
*     where  t[I] .LT. arg .LT. t[I+1], using Horner's rule.                   *
*                                                                              *
*     If  arg .LT. t[1] then  I = 1  is used.                                  *
*     If  arg .GE. t[N] then  I = n  is used.                                  *
*                                                                              *
*  Input:                                                                      *
*                                                                              *
*     n = the number of data points                                            *
*     arg = the abscissa at which the spline is to be evaluated                *
*     t,coord = the arrays of data abscissas and ordinates                     *
*     a = arrays of spline coefficients computed by spline                     *
*                                                                              *
*  If arg is not in the same interval as the previous call, then a binary      *
*  search is performed to determine the proper interval.                       *
*                                                                              *
*******************************************************************************/

static	int	Sinterval

	(
	int		n,
	double	arg,
	double	*t
	)

	{
	int		j, k;

	static	int		icurr=0;

	if (!t) return icurr;

	if ( icurr >= n-1 ) icurr = 0;

	if ( (arg>=t[icurr]) && (arg<t[icurr+1]) ) return icurr;

	/* Binary search: */
	icurr = 0;
	j = n;
	do	{
		k = (icurr+j)/2;
		if ( arg < t[k] ) j = k;
		if ( arg >= t[k] ) icurr = k;
		} while ( j > icurr+1 );

	return icurr;
	}

static	double	Seval

	(
	int		n,
	double	arg,
	double	*t,
	double	*coord,
	double	*a
	)

	{
	int i;
	double dx;

	if (!t || !coord || !a) return 0.0;

	/* Locate the appropriate interval */
	i = Sinterval(n, arg, t);

	/* Evaluate spline. */
	dx = arg - t[i];
	return  coord[i] + dx*a[i];
	}
