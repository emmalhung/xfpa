/*********************************************************************/
/** @file tween.c
 *
 * 2-D non-uniform cubic curve interpolation.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*  2-D non-uniform cubic curve interpolation.                          *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2003 Environment Canada (MSC)            *
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

#include "tween.h"

#include <fpa_math.h>
#include <fpa_getmem.h>

#include <stdio.h>

/* Set various debug modes */
#undef DEBUG_TWEEN
#undef DEBUG_QL_TWEEN

static	void	Spline(int, double *, double *, double *, double *, double *);
static	int		Sinterval(int, double, double *);
static	double	Seval(int, double, double *, double *, double *, double *,
					double *);
static	double	Sderiv(int, double, double *, double *, double *, double *);

/*******************************************************************************
*                                                                              *
*  T w e e n 1                                                                 *
*                                                                              *
*******************************************************************************/
/*********************************************************************/
/** Interpolate (extrapolate) from a list of values given at
 * monotonically increasing parameter values, to a given set of
 * parameter values.
 *
 * The derivatives at the given set of parameter values is also
 * available.
 *	@param[in]	numkeys 	number of key points
 *	@param[in]	*keylist	list of key points
 *	@param[in]	*keyval		values at key points
 *	@param[in]	numtweens	number of between points
 *	@param[in]	*tweenlist	list of between points
 *	@param[out]	*tweenval	values at between points
 *	@param[out]	*tweenderiv	derivative at between points
 *********************************************************************/

void	Tween1

	(
	int		numkeys ,
	double	*keylist,
	double	*keyval,
	int		numtweens,
	double	*tweenlist,
	double	*tweenval,
	double	*tweenderiv
	)

	{
	int		morekeys, ikey, itween;
	double	*morelist, *morevals;
	double	*B, *C, *D;
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
	if ( numkeys <= 1 )
		{
		morelist[0] = keylist[0];
		morevals[0] = keyval[0];
		morelist[morekeys-1] = keylist[numkeys-1];
		morevals[morekeys-1] = keyval[numkeys-1];
		}
	else
		{
		morelist[0] = 2.0*keylist[0] - keylist[1];
		morevals[0] = 2.0*keyval[0] - keyval[1];
		morelist[morekeys-1] = 2.0*keylist[numkeys-1] - keylist[numkeys-2];
		morevals[morekeys-1] = 2.0*keyval[numkeys-1] - keyval[numkeys-2];
		}

	/* Allocate storage for spline interpolation routines */
	B = INITMEM(double, morekeys);
	C = INITMEM(double, morekeys);
	D = INITMEM(double, morekeys);

	/* Fit the given values by cubic spline interpolation */
	Spline( morekeys, morelist, morevals, B, C, D );

	/* Evaluate the inbetween values and derivatives */
	for( itween=0; itween<numtweens; itween++ )
		{
		parm = tweenlist[itween];
		if (tweenval)
			tweenval[itween] = Seval( morekeys, parm, morelist, morevals, B, C, D );
		if (tweenderiv)
			tweenderiv[itween] = Sderiv( morekeys, parm, morelist, B, C, D );
		}

	FREEMEM(morelist);
	FREEMEM(morevals);
	FREEMEM(B);
	FREEMEM(C);
	FREEMEM(D);
	}

/*******************************************************************************
*                                                                              *
*  T w e e n 2                                                                 *
*                                                                              *
*******************************************************************************/
/*********************************************************************/
/** Interpolate (extrapolate) from a list of points given at
 * monotonically increasing parameter values, to a given set of
 * parameter values.
 *
 * This version is used for fields with two components
 * (example uv-wind).
 *	@param[in]	numkeys 	number of key points
 *	@param[in]	*keylist 	list of key points
 *	@param[in]	*keyx 		list of key x values
 *	@param[in]	*keyy 		list of key y values
 *	@param[in]	numtweens 	number of between points
 *	@param[in]	*tweenlist 	list of between points
 *	@param[out]	*tweenx 	list of between x values
 *	@param[out]	*tweeny		list of between y values
 *********************************************************************/

void	Tween

	(
	int		numkeys,
	double	*keylist,
	double	*keyx,
	double	*keyy,
	int		numtweens,
	double	*tweenlist,
	double	*tweenx,
	double	*tweeny
	)

	{
	int		morekeys, ikey, itween;
	double	*morelist, *morevalx, *morevaly;
	double	*Bx, *By, *Cx, *Cy, *Dx, *Dy;
	double	parm;

	if (!keylist || !keyx || !keyy || !tweenlist) return;

#	ifdef DEBUG_TWEEN
	(void) printf("[Tween] numkeys: %d \n", numkeys);
	for ( ikey=0; ikey<numkeys; ikey++ )
		(void) printf("[Tween] ikey: %.2d  keylist: %.2f  keyx/keyy: %.3f %.3f\n",
			ikey, keylist[ikey], keyx[ikey], keyy[ikey]);
#	endif /* DEBUG_TWEEN */


	/* Allocate storage for extended list of keys */
	morekeys = numkeys + 2;
	morelist = INITMEM(double, morekeys);
	morevalx = INITMEM(double, morekeys);
	morevaly = INITMEM(double, morekeys);

	/* Set interior values from keylist and keyval */
	for ( ikey=0; ikey<numkeys; ikey++ )
		{
		morelist[ikey+1] = keylist[ikey];
		morevalx[ikey+1] = keyx[ikey];
		morevaly[ikey+1] = keyy[ikey];
		}

	/* Extrapolate to get values at beginning and end */
	if ( numkeys <= 1 )
		{
		morelist[0] = keylist[0];
		morevalx[0] = keyx[0];
		morevaly[0] = keyy[0];
		morelist[morekeys-1] = keylist[numkeys-1];
		morevalx[morekeys-1] = keyx[numkeys-1];
		morevaly[morekeys-1] = keyy[numkeys-1];
		}
	else
		{
		morelist[0] = 2.0*keylist[0] - keylist[1];
		morevalx[0] = 2.0*keyx[0] - keyx[1];
		morevaly[0] = 2.0*keyy[0] - keyy[1];
		morelist[morekeys-1] = 2.0*keylist[numkeys-1] - keylist[numkeys-2];
		morevalx[morekeys-1] = 2.0*keyx[numkeys-1] - keyx[numkeys-2];
		morevaly[morekeys-1] = 2.0*keyy[numkeys-1] - keyy[numkeys-2];
		}

#	ifdef DEBUG_TWEEN
	for ( ikey=0; ikey<morekeys; ikey++ )
		(void) printf("[Tween] ikey: %.2d  morelist: %.3f  morevalx: %.3f  morevaly: %.3f\n",
			ikey, morelist[ikey], morevalx[ikey], morevaly[ikey]);
#	endif /* DEBUG_TWEEN */

	/* Allocate storage for spline interpolation routines */
	Bx = INITMEM(double, morekeys);
	Cx = INITMEM(double, morekeys);
	Dx = INITMEM(double, morekeys);
	By = INITMEM(double, morekeys);
	Cy = INITMEM(double, morekeys);
	Dy = INITMEM(double, morekeys);

	/* Fit the given values by cubic spline interpolation */
	Spline( morekeys, morelist, morevalx, Bx, Cx, Dx );
	Spline( morekeys, morelist, morevaly, By, Cy, Dy );

#	ifdef DEBUG_TWEEN
	for ( ikey=0; ikey<morekeys; ikey++ )
		(void) printf("[Tween] ikey: %.2d  Bx/Cx/Dx: %.3f %.3f %.3f  By/Cy/Dy: %.3f %.3f %.3f\n",
			ikey, Bx[ikey], Cx[ikey], Dx[ikey], By[ikey], Cy[ikey], Dy[ikey]);
#	endif /* DEBUG_TWEEN */

	/* Evaluate the inbetween values */
	for( itween=0; itween<numtweens; itween++ )
		{
		parm = tweenlist[itween];
		if (tweenx)
			tweenx[itween] = Seval( morekeys, parm, morelist, morevalx, Bx, Cx, Dx );
		if (tweeny)
			tweeny[itween] = Seval( morekeys, parm, morelist, morevaly, By, Cy, Dy );
		}

#	ifdef DEBUG_TWEEN
	(void) printf("[Tween] numtweens: %d \n", numtweens);
	for( itween=0; itween<numtweens; itween++ )
		{
		(void) printf("[Tween]   %.3d", itween);
		if (tweenx && tweeny)
			(void) printf("  tweenx/y: %.3f %.3f",
				tweenx[itween], tweeny[itween]);
		else if (tweenx)
			(void) printf("  tweenx: %.3f", tweenx[itween]);
		else if (tweeny)
			(void) printf("  tweeny: %.3f", tweeny[itween]);
		if (itween>0)
			{
			if (tweenx && tweeny)
				(void) printf("  (dx/dy/dd: %.3f %.3f %.3f)",
					tweenx[itween]-tweenx[itween-1],
					tweeny[itween]-tweeny[itween-1],
					hypot(tweenx[itween]-tweenx[itween-1],
							tweeny[itween]-tweeny[itween-1]));
			else if (tweenx)
				(void) printf("  (dx: %.3f)", tweenx[itween]-tweenx[itween-1]);
			else if (tweeny)
				(void) printf("  (dy: %.3f)", tweeny[itween]-tweeny[itween-1]);
			}
		(void) printf("\n");
		}
#	endif /* DEBUG_TWEEN */

	FREEMEM(morelist);
	FREEMEM(morevalx);
	FREEMEM(morevaly);
	FREEMEM(Bx);
	FREEMEM(Cx);
	FREEMEM(Dx);
	FREEMEM(By);
	FREEMEM(Cy);
	FREEMEM(Dy);
	}

/***********************************************************************
*                                                                      *
*     P i e c e W i s e _ 2 D   - Piecewise linear interpolation       *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Interpolate (extrapolate) by piecewise linear interpolation from
 * a list of points given at monotonically increasing parameter
 * values, to a given set of parameter values.
 *
 * This version is used for fields with two components
 *	@param[in]	numkeys 	number of key points
 *	@param[in]	*keylist 	list of key points
 *	@param[in]	*keyx 		list of key x values
 *	@param[in]	*keyy 		list of key y values
 *	@param[in]	numtweens 	number of between points
 *	@param[in]	*tweenlist 	list of between points
 *	@param[out]	*tweenx 	list of between x values
 *	@param[out]	*tweeny		list of between y values
 *********************************************************************/

void	PieceWise_2D

	(
	int		numkeys,
	double	*keylist,
	double	*keyx,
	double	*keyy,
	int		numtweens,
	double	*tweenlist,
	double	*tweenx,
	double	*tweeny
	)

	{
	int		ikey, itween, pkey;
	double	parm, fact, finv;

	if (!keylist || !keyx || !keyy || !tweenlist) return;

#	ifdef DEBUG_TWEEN
	(void) printf("[PieceWise_2D] numkeys: %d \n", numkeys);
	for ( ikey=0; ikey<numkeys; ikey++ )
		(void) printf("[PieceWise_2D] ikey: %.2d  keylist: %.2f  keyx/keyy: %.3f %.3f\n",
			ikey, keylist[ikey], keyx[ikey], keyy[ikey]);
	(void) printf("[PieceWise_2D] numtweens: %d \n", numtweens);
#	endif /* DEBUG_TWEEN */

	/* Evaluate the inbetween values */
	for( ikey=0, itween=0; itween<numtweens; itween++ )
		{
		parm = tweenlist[itween];
		fact = -1.0;

		/* Extrapolate before start of keylist */
		if (parm <= keylist[0])
			{
			pkey = ikey;
			if (tweenx) tweenx[itween] = keyx[0];
			if (tweeny) tweeny[itween] = keyy[0];
			}

		/* Extrapolate after end of keylist */
		else if (parm >= keylist[numkeys-1])
			{
			pkey = numkeys-1;
			if (tweenx) tweenx[itween] = keyx[numkeys-1];
			if (tweeny) tweeny[itween] = keyy[numkeys-1];
			}

		/* Determine intermediate span in keylist */
		else
			{
			while (ikey < numkeys-1)
				{
				if (ikey == 0) ikey++;
				if (parm >= keylist[ikey-1] && parm < keylist[ikey]) break;
				ikey++;
				}
			if (parm == keylist[ikey])
				{
				pkey = ikey;
				if (tweenx) tweenx[itween] = keyx[ikey];
				if (tweeny) tweeny[itween] = keyy[ikey];
				}
			else
				{
				pkey = ikey - 1;
				fact = (parm - keylist[pkey]) / (keylist[ikey] - keylist[pkey]);
				finv = 1.0 - fact;
				if (tweenx)
					tweenx[itween] = (keyx[pkey] * finv) + (keyx[ikey] * fact);
				if (tweeny)
					tweeny[itween] = (keyy[pkey] * finv) + (keyy[ikey] * fact);
				}
			}

#		ifdef DEBUG_TWEEN
		(void) printf("[PieceWise_2D]   %.3d %.2f (%d-%d  fact: %.3f)",
			itween, parm, pkey, ikey, fact);
		if (tweenx && tweeny)
			(void) printf("  tweenx/y: %.3f %.3f",
				tweenx[itween], tweeny[itween]);
		else if (tweenx)
			(void) printf("  tweenx: %.3f", tweenx[itween]);
		else if (tweeny)
			(void) printf("  tweeny: %.3f", tweeny[itween]);
		if (itween>0)
			{
			if (tweenx && tweeny)
				(void) printf("  (dx/dy/dd: %.3f %.3f %.3f)",
					tweenx[itween]-tweenx[itween-1],
					tweeny[itween]-tweeny[itween-1],
					hypot(tweenx[itween]-tweenx[itween-1],
							tweeny[itween]-tweeny[itween-1]));
			else if (tweenx)
				(void) printf("  (dx: %.3f)", tweenx[itween]-tweenx[itween-1]);
			else if (tweeny)
				(void) printf("  (dy: %.3f)", tweeny[itween]-tweeny[itween-1]);
			}
		(void) printf("\n");
#		endif /* DEBUG_TWEEN */
		}
	}

/***********************************************************************
*                                                                      *
*     s e t _ q u a s i l i n e a r _ m o d e                          *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Set parameters for use in QuasiLinear_Tween() function
 *
 *	@param[in]	mode 	Quasilinear mode to use
 *	@param[in]	minval 	Minimum value
 *	@param[in]	avgval 	Average value
 *********************************************************************/

/* Constants for QuasiLinear_Tween() function                         */
/*  Mode QL_Proportional ... compares link spans to longest span      */
/*   QL_propmin - fraction of longest span that uses piecewise linear */
/*                 interpolation to determine intermediate locations  */
/*                 (originally hardcoded as 1/100.0 or 0.01)          */
/*   QL_propavg - fraction of longest span that uses weighted average */
/*                 of piecewise linear and cubic spline interpolation */
/*                 to determine intermediate locations                */
/*                 (originally hardcoded as 1/50.0 or 0.02)           */
/*  Mode QL_Fixed ... compares link spans to set distance             */
/*   QL_fixedmin - distance of span that uses piecewise linear        */
/*                  interpolation to determine intermediate locations */
/*   QL_fixedavg - distance of span that uses weighted average of     */
/*                  piecewise linear and cubic spline interpolation   */

static	QL_MODES	QL_mode     = QL_Proportional;
static	double		QL_propmin  = 0.02;
static	double		QL_propavg  = 0.05;
static	double		QL_fixedmin = 10.0;
static	double		QL_fixedavg = 25.0;

void	set_quasilinear_mode

	(
	QL_MODES	mode,
	double		minval,
	double		avgval
	)

	{

	/* Set mode */
	if ( QL_mode != mode) QL_mode = mode;

	/* Set values */
	if ( QL_mode == QL_Proportional)
		{
		QL_propmin = minval;
		QL_propavg = avgval;
		if (QL_propavg < QL_propmin) QL_propavg = QL_propmin;
		}
	else if ( QL_mode == QL_Fixed)
		{
		QL_fixedmin = minval;
		QL_fixedavg = avgval;
		if (QL_fixedavg < QL_fixedmin) QL_fixedavg = QL_fixedmin;
		}

	return;
	}

/***********************************************************************
*                                                                      *
*     Q u a s i L i n e a r _ T w e e n                                *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Interpolate (extrapolate) by combining spline and piecewise linear
 * interpolation from a list of points given at monotonically
 * increasing parameter values, to a given set of parameter values.
 *
 * This version is used for fields with two components
 *	@param[in]	numkeys 	number of key points
 *	@param[in]	*keylist 	list of key points
 *	@param[in]	*keyx 		list of key x values
 *	@param[in]	*keyy 		list of key y values
 *	@param[in]	numtweens 	number of between points
 *	@param[in]	*tweenlist 	list of between points
 *	@param[out]	*tweenx 	list of between x values
 *	@param[out]	*tweeny		list of between y values
 *********************************************************************/

void	QuasiLinear_Tween

	(
	int		numkeys,
	double	*keylist,
	double	*keyx,
	double	*keyy,
	int		numtweens,
	double	*tweenlist,
	double	*tweenx,
	double	*tweeny
	)

	{
	int		ikey, itween;
	double	parm, fact, finv;
	double	*keydist2, diffl, distx, disty, maxdist2, maxdistmin, maxdistavg;
	double	*tweenvalx, *tweenvaly;
	double	*piecevalx, *piecevaly;

	if (!keylist || !keyx || !keyy || !tweenlist) return;

#	ifdef DEBUG_QL_TWEEN
	(void) printf("[QuasiLinear_Tween] numkeys: %d \n", numkeys);
	for ( ikey=0; ikey<numkeys; ikey++ )
		(void) printf("[QuasiLinear_Tween] ikey: %.2d  keylist: %.2f  keyx/keyy: %.3f %.3f\n",
			ikey, keylist[ikey], keyx[ikey], keyy[ikey]);
	(void) printf("[QuasiLinear_Tween] numtweens: %d \n", numtweens);
#	endif /* DEBUG_QL_TWEEN */

	/* Allocate storage for combining spline and piecewise linear interpolation */
	keydist2  = INITMEM(double, numkeys);
	tweenvalx = INITMEM(double, numtweens);
	tweenvaly = INITMEM(double, numtweens);
	piecevalx = INITMEM(double, numtweens);
	piecevaly = INITMEM(double, numtweens);

	/* Determine span ratios */
	keydist2[0] = 0.0;
	maxdist2 = 0.0;
	for( ikey=1; ikey<numkeys; ikey++ )
	{
		diffl = keylist[ikey] - keylist[ikey-1];
		if ( diffl <= 0.0 )
			{
			keydist2[ikey] = 0.0;
			continue;
			}
		distx = keyx[ikey] - keyx[ikey-1];
		disty = keyy[ikey] - keyy[ikey-1];
		keydist2[ikey] = (distx*distx + disty*disty) / (diffl*diffl);
		if ( keydist2[ikey] > maxdist2 ) maxdist2 = keydist2[ikey];
	}

	/* Set comparison based on mode */
	if ( QL_mode == QL_Proportional)
		{
		maxdistmin = maxdist2 * QL_propmin * QL_propmin;
		maxdistavg = maxdist2 * QL_propavg * QL_propavg;
		}
	else if ( QL_mode == QL_Fixed)
		{
		maxdistmin = QL_fixedmin * QL_fixedmin;
		maxdistavg = QL_fixedavg * QL_fixedavg;
		}

#	ifdef DEBUG_QL_TWEEN
	(void) printf("[QuasiLinear_Tween] maxdist2: %.5f  maxdistmin: %.5f  maxdistavg: %.5f \n",
		maxdist2, maxdistmin, maxdistavg);
#	endif /* DEBUG_QL_TWEEN */

	/* Determine values by spline interpolation */
	Tween(numkeys, keylist, keyx, keyy,
			numtweens, tweenlist, tweenvalx, tweenvaly);

	/* Determine values by piecewise linear interpolation */
	PieceWise_2D(numkeys, keylist, keyx, keyy,
			numtweens, tweenlist, piecevalx, piecevaly);
	
	/* Evaluate the inbetween values based on each span ratio */
	for( ikey=1, itween=0; itween<numtweens; itween++ )
		{
		parm = tweenlist[itween];

		/* Determine span in keylist */
		while (ikey < numkeys-1)
			{
			if (parm <= keylist[ikey]) break;
			ikey++;
			}

		/* Use piecewise linear interpolation for inbetween values */
		if ( keydist2[ikey] <= 0.0 || keydist2[ikey] < maxdistmin )
			{
			if (tweenx) tweenx[itween] = piecevalx[itween];
			if (tweeny) tweeny[itween] = piecevaly[itween];

#			ifdef DEBUG_QL_TWEEN
			(void) printf("[QuasiLinear_Tween]   %.3d %.2f (span: %d  dist: %.3f  Linear)",
				itween, parm, ikey, keydist2[ikey]);
			if (tweenx && tweeny)
				(void) printf("  piecevalx/y: %.3f %.3f  tweenx/y: %.3f %.3f\n",
					piecevalx[itween], piecevaly[itween],
					tweenx[itween], tweeny[itween]);
			else if (tweenx)
				(void) printf("  piecevalx: %.3f  tweenx: %.3f\n",
					piecevalx[itween], tweenx[itween]);
			else if (tweeny)
				(void) printf("  piecevaly: %.3f  tweeny: %.3f\n",
					piecevaly[itween], tweeny[itween]);
#			endif /* DEBUG_QL_TWEEN */
			}

		/* Use weighted average of interpolations for inbetween values */
		else if ( keydist2[ikey] < maxdistavg )
			{
			fact = (keydist2[ikey] - maxdistmin) / (maxdistavg - maxdistmin);
			finv = 1.0 - fact;
			if (tweenx) tweenx[itween] = (piecevalx[itween] * finv)
											+ (tweenvalx[itween] * fact);
			if (tweeny) tweeny[itween] = (piecevaly[itween] * finv)
											+ (tweenvaly[itween] * fact);

#			ifdef DEBUG_QL_TWEEN
			(void) printf("[QuasiLinear_Tween]   %.3d %.2f (span: %d  dist: %.3f  Average - fact: %.3f)",
				itween, parm, ikey, keydist2[ikey], fact);
			if (tweenx && tweeny)
				(void) printf("  tweenvalx/y: %.3f %.3f  piecevalx/y: %.3f %.3f  tweenx/y: %.3f %.3f\n",
					tweenvalx[itween], tweenvaly[itween],
					piecevalx[itween], piecevaly[itween],
					tweenx[itween], tweeny[itween]);
			else if (tweenx)
				(void) printf("  tweenvalx: %.3f  piecevalx: %.3f  tweenx: %.3f\n",
					tweenvalx[itween], piecevalx[itween], tweenx[itween]);
			else if (tweeny)
				(void) printf("  tweenvaly: %.3f  piecevaly: %.3f  tweeny: %.3f\n",
					tweenvaly[itween], piecevaly[itween], tweeny[itween]);
#			endif /* DEBUG_QL_TWEEN */
			}

		/* Use spline interpolation for inbetween values */
		else
			{
			if (tweenx) tweenx[itween] = tweenvalx[itween];
			if (tweeny) tweeny[itween] = tweenvaly[itween];

#			ifdef DEBUG_QL_TWEEN
			(void) printf("[QuasiLinear_Tween]   %.3d %.2f (span: %d  dist: %.3f  Spline)",
				itween, parm, ikey, keydist2[ikey]);
			if (tweenx && tweeny)
				(void) printf("  tweenvalx/y: %.3f %.3f  tweenx/y: %.3f %.3f\n",
					tweenvalx[itween], tweenvaly[itween],
					tweenx[itween], tweeny[itween]);
			else if (tweenx)
				(void) printf("  tweenvalx: %.3f  tweenx: %.3f\n",
					tweenvalx[itween], tweenx[itween]);
			else if (tweeny)
				(void) printf("  tweenvaly: %.3f  tweeny: %.3f\n",
					tweenvaly[itween], tweeny[itween]);
#			endif /* DEBUG_QL_TWEEN */
			}
		}

	/* Free storage space */
	FREEMEM(keydist2);
	FREEMEM(tweenvalx);
	FREEMEM(tweenvaly);
	FREEMEM(piecevalx);
	FREEMEM(piecevaly);
	}

/*******************************************************************************
*                                                                              *
*  S p l i n e                                                                 *
*                                                                              *
*******************************************************************************/
/******************************************************************************/
/**  The coefficients a[i], b[i], and c[i], i=0,2,...,n-1 are computed for a
 *  cubic interpolating spline, as in:
 *
 *    \f$ s(t) = coord[i] + a[i]*(t-t[i]) + b[i]*(t-t[i])^2 + c[i]*(t-t[i])^3 \f$
 *
 *     for \f$ t[i] \le t \le t[i+1] \f$
 *
 *  @param[in] n     	the number of data points or knots \f$(n\ge2)\f$
 *  @param[in] t     	the abscissas of the knots in strictly increasing order
 *  @param[in] coord 	the ordinates of the knots
 *  @param[out] a 		array of spline coefficient as defined above
 *  @param[out] b 		array of spline coefficient as defined above
 *  @param[out] c 		array of spline coefficient as defined above
 *
 *  Using  '  to denote differentiation,
 *
 *     coord[i] = s(t[i])
 *     a[i] = s'(t[i])
 *     b[i] = s''(t[i])/2
 *     c[i] = s'''(t[i])/6  (derivative from the right)
 *
 *  The accompanying function Seval() can be used to evaluate the spline.
 *
 ******************************************************************************/

static	void	Spline

	(
	int		n,
	double	*t,
	double	*coord,
	double	*a,
	double	*b,
	double	*c
	)

	{
	int		nm1, nm2, i;
	double	tmp;

	if (!t || !coord || !a || !b || !c) return;

	for ( i=0; i<n; i++ ) a[i] = b[i] = c[i] = 0.0;

	nm1 = n-1;
	nm2 = n-2;
	if ( n < 2 ) return;
	if ( n < 3 )
		{
		a[0] = (coord[1]-coord[0])/(t[1]-t[0]);
		b[0] = 0.0;
		c[0] = 0.0;
		a[1] = a[0];
		b[1] = 0.0;
		c[1] = 0.0;
		return;
		}

	/* Set up tridiagonal system: */
	/* a = diagonal, c = off diagonal, b = right hand side. */
	c[0] = t[1] - t[0];
	b[1] = (coord[1] - coord[0])/c[0];
	for ( i=1; i<nm1; i++ )
		{
		c[i]   = t[i+1] - t[i];
		a[i]   = 2.0*(c[i-1] + c[i]);
		b[i+1] = (coord[i+1] - coord[i])/c[i];
		b[i]   = b[i+1] - b[i];
		}

	/* End conditions: */
	/* Third derivatives at t[1] and t[N] obtained from divided differences. */
	a[0]   = -c[0];
	a[nm1] = -c[nm2];
	b[0]   = 0.0;
	b[nm1] = 0.0;
	if ( n != 3 )
		{
		b[0]   = b[2]/(t[3]-t[1]) - b[1]/(t[2]-t[0]);
		b[nm1] = b[nm2]/(t[nm1]-t[n-3]) - b[n-3]/(t[nm2]-t[n-4]);
		b[0]   = b[0]*c[0]*c[0]/(t[3]-t[0]);
		b[nm1] = -b[nm1]*c[nm2]*c[nm2]/(t[nm1]-t[n-4]);
		}

	/* Forward elimination: */
	for ( i=1; i<n; i++ )
		{
		tmp  = c[i-1]/a[i-1];
		a[i] = a[i] - tmp*c[i-1];
		b[i] = b[i] - tmp*b[i-1];
		}

	/* Back substitution: */
	b[nm1] = b[nm1]/a[nm1];
	for ( i=nm2; i>=0; i-- )
		{
		b[i] = (b[i] - c[i]*b[i+1])/a[i];
		}

	/* Compute polynomial coefficients. */
	a[nm1] = (coord[nm1] - coord[nm2])/c[nm2] + c[nm2]*(b[nm2] + 2.0*b[nm1]);
	for ( i=0; i<nm1; i++ )
		{
		a[i] = (coord[i+1] - coord[i])/c[i] - c[i]*(b[i+1] + 2.0*b[i]);
		c[i] = (b[i+1] - b[i])/c[i];
		b[i] = 3.0*b[i];
		}
	b[nm1] = 3.0*b[nm1];
	c[nm1] = c[nm2];
	}

/*******************************************************************************
*                                                                              *
*  S e v a l                                                                   *
*                                                                              *
*  S d e r i v                                                                 *
*                                                                              *
*  These subroutines evaluate the cubic spline function (or its derivative):   *
*                                                                              *
*     SEVAL = coord[I]                                                         *
*           + a[I]*(arg-t[I])                                                  *
*           + C[I]*(arg-t[I])**2                                               *
*           + c[I]*(arg-t[I])**3                                               *
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
*     a,b,c = arrays of spline coefficients computed by spline                 *
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
    double	*a,
    double	*b,
    double	*c
	)

	{
	int i;
	double dx;

	if (!t || !coord || !a || !b || !c) return 0.0;

	/* Locate the appropriate interval */
	i = Sinterval(n, arg, t);

	/* Evaluate spline. */
	dx = arg - t[i];
	return  coord[i] + dx*(a[i] + dx*(b[i] + dx*c[i]));
	}

static	double	Sderiv

	(
	int		n,
	double	arg,
	double	*t,
	double	*a,
	double	*b,
	double	*c
	)

	{
	int i;
	double dx;

	if (!t || !a || !b || !c) return 0.0;

	/* Locate the appropriate interval */
	i = Sinterval(n, arg, t);

	/* Evaluate spline. */
	dx = arg - t[i];
	return  a[i] + dx*(2*b[i] + dx*3*c[i]);
	}
