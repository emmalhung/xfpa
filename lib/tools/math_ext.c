/*********************************************************************/
/** @file math_ext.c
 *
 * Natural extensions to the UNIX math library and math.h
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*     m a t h _ e x t . c                                              *
*                                                                      *
*     Natural extensions to the UNIX math library and math.h           *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2001 Environment Canada (AES)            *
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

#include "math_ext.h"
#include <stdlib.h>
#include <stdio.h>


/***********************************************************************
*                                                                      *
*     c o p y s i g n   - copy the sign from y to x                    *
*                                                                      *
***********************************************************************/

#ifdef MACHINE_NOT_NEEDED
/*********************************************************************/
/** Copy the sign from y to x.
 *
 *	@param[in]	x	has desired magnitude
 *	@param[in]	y	has desired sign
 * 	@return The absolute value of x with the same sign as y.
 *********************************************************************/
double	copysign

	(
	double	x,
	double	y
	)

	{
	return ABS(x)*SIGN(y);
	}
#endif

/***********************************************************************
*                                                                      *
*    Trig (radian form) with near-zero tolerance:                      *
*                                                                      *
*    f p a _ s i n       - sin function                                *
*    f p a _ c o s       - cos function                                *
*    f p a _ t a n       - tan function                                *
*    f p a _ a s i n     - asin function                               *
*    f p a _ a c o s     - acos function                               *
*    f p a _ a t a n     - atan function                               *
*    f p a _ a t a n 2   - atan2 function                              *
*                                                                      *
*    Trig (degree form) with near-zero tolerance:                      *
*                                                                      *
*    f p a _ s i n d e g       - sin function                          *
*    f p a _ c o s d e g       - cos function                          *
*    f p a _ t a n d e g       - tan function                          *
*    f p a _ a s i n d e g     - acos function                         *
*    f p a _ a c o s d e g     - acos function                         *
*    f p a _ a t a n d e g     - atan function                         *
*    f p a _ a t a n 2 d e g   - atan2 function                        *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Evaluate the sin function (radian form). With near-zero tolerance.
 *
 * @return sin(t)
 *********************************************************************/
double	fpa_sin(double t)

	{
	double v;

	v = sin(t);
	return (fabs(v) < 1.0e-07)? 0.0: v;
	}

/*********************************************************************/
/** Evaluate the cos function (radian form). With near-zero tolerance.
 *
 * @return cos(t)
 *********************************************************************/
double	fpa_cos(double t)

	{
	double v;

	v = cos(t);
	return (fabs(v) < 1.0e-07)? 0.0: v;
	}

/*********************************************************************/
/** Evaluate the tan function (radian form). With near-zero tolerance.
 *
 * @return tan(t)
 *********************************************************************/
double	fpa_tan(double t)

	{
	double v;

	v = tan(t);
	return (fabs(v) < 1.0e-07)? 0.0: v;
	}

/*********************************************************************/
/** Evaluate the asin function (radian form). With near-zero tolerance.
 *
 * @return asin(a)
 *********************************************************************/
double	fpa_asin(double a)

	{
	double v;

	v = asin(a);
	return (fabs(v) < 1.0e-07)? 0.0: v;
	}

/*********************************************************************/
/** Evaluate the acos function (radian form). With near-zero tolerance.
 *
 * @return acos(a)
 *********************************************************************/
double	fpa_acos(double a)

	{
	double v;

	v = acos(a);
	return (fabs(v) < 1.0e-07)? 0.0: v;
	}

/*********************************************************************/
/** Evaluate the atan function (radian form). With near-zero tolerance.
 *
 * @return atan(a)
 *********************************************************************/
double	fpa_atan(double a)

	{
	double v;

	v = atan(a);
	return (fabs(v) < 1.0e-07)? 0.0: v;
	}

/*********************************************************************/
/** Evaluate the atan2 function (radian form). With near-zero tolerance.
 *
 * @return atan2(y,x)
 *********************************************************************/
double	fpa_atan2(double y, double x)

	{
	double v;

	v = atan2(y, x);

	/* Check for 0/0 case */
	/* Usually means zero magnitude, so direction doesn't matter */
	if (isnan(v)) return 0.0;

	return (fabs(v) < 1.0e-07)? 0.0: v;
	}

/**********************************************************************/

/*********************************************************************/
/** Evaluate the sin function (degree form). With near-zero tolerance.
 *
 * @return sin(t)
 *********************************************************************/
double	fpa_sindeg(double t)				{ return fpa_sin(RAD*t); }
/*********************************************************************/
/** Evaluate the cos function (degree form). With near-zero tolerance.
 *
 * @return cos(t)
 *********************************************************************/

double	fpa_cosdeg(double t)				{ return fpa_cos(RAD*t); }
/*********************************************************************/
/** Evaluate the tan function (degree form). With near-zero tolerance.
 *
 * @return tan(t)
 *********************************************************************/

double	fpa_tandeg(double t)				{ return fpa_tan(RAD*t); }
/*********************************************************************/
/** Evaluate the asin function (degree form). With near-zero tolerance.
 *
 * @return asin(a)
 *********************************************************************/

double	fpa_asindeg(double a)				{ return fpa_asin(a)/RAD; }
/*********************************************************************/
/** Evaluate the acos function (degree form). With near-zero tolerance.
 *
 * @return acos(a)
 *********************************************************************/

double	fpa_acosdeg(double a)				{ return fpa_acos(a)/RAD; }
/*********************************************************************/
/** Evaluate the atan function (degree form). With near-zero tolerance.
 *
 * @return atan(a)
 *********************************************************************/

double	fpa_atandeg(double a)				{ return fpa_atan(a)/RAD; }
/*********************************************************************/
/** Evaluate the atan2 function (degree form). With near-zero tolerance.
 *
 * @return atan2(a)
 *********************************************************************/

double	fpa_atan2deg(double y, double x)	{ return fpa_atan2(y, x)/RAD; }

/***********************************************************************
*                                                                      *
*     q a t a n 2                                                      *
*                                                                      *
*     Quick approximate atan2() function.                              *
*                                                                      *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Quick approximate atan2() (radian form). With near-zero tolerance.
 *
 * @return approximation to atan2(y,x)
 *********************************************************************/
double	qatan2

	(
	double	y,
	double	x
	)

	{
	/* Closer to x-axis */
	if (fabs(x) >= fabs(y))
		{
		if (x > 0)      return (M_PI_4 * (y/x));
		else if (x < 0)
			{
			if (y >= 0) return (M_PI_4 * (4 + y/x));
			else        return (M_PI_4 * (-4 + y/x));
			}
		else
			{
			(void) fprintf(stderr, "qatan2 error\n");
			return 0;
			}
		}

	/* Closer to y-axis */
	else
		{
		if (y > 0)      return (M_PI_4 * (2 - x/y));
		else if (y < 0) return (M_PI_4 * (-2 - x/y));
		else
			{
			(void) fprintf(stderr, "qatan2 error\n");
			return 0;
			}
		}
	}

/***********************************************************************
*                                                                      *
*    f p a _ s q r t  - sqrt function returning 0 for negative values  *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Evaluate the square root function.
 *
 * @return
 * 	- @f$\sqrt{x}@f$ or
 * 	- 0 if x<0
 *********************************************************************/
double	fpa_sqrt(double x)

	{

	/* Return 0 for negative values */
	if ( x < 0.0 )
		{
		(void) fprintf(stderr, "[fpa_sqrt] Negative argument: %g", x);
		(void) fprintf(stderr, "  ...  Returning 0.0\n");
		return 0.0;
		}

	/* Return the standard UNIX value */
	return sqrt(x);
	}
