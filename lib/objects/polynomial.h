/*********************************************************************/
/** @file polynomial.h
 *
 * UNIPOLY and BIPOLY object definitions (include file)
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    p o l y n o m i a l . h                                           *
*                                                                      *
*    UNIPOLY and BIPOLY object definitions (include file)              *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2002 Environment Canada (MSC)            *
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

/* See if already included */
#ifndef POLY_DEFS
#define POLY_DEFS

/* Need definition for POINT */
#include "misc.h"

/* Define the maximum order of polynomials to be cubic */
/* Also set max order for which roots can be computed explicitly */
/* rather that using a root finder */
#define MAXORD 3
#define MAXTERM 4
#define MAX_EXPLICIT_ORDER 3

/* Define UNIPOLY object */
/** a univariate polynomial defined by a set of 
 * power series coefficients and an order */
typedef struct UNIPOLY_struct
	{
	short	order;				/**< order of polynomial */
	double	coeffs[MAXTERM];	/**< coefficients */
	} UNIPOLY;


/* Define BIPOLY object */
/** a bivariate polynomial defined by an array of power series
 * coefficients and an x and y order */
typedef struct BIPOLY_struct
	{
	short	xorder;						/**< x-order of polynomial */
	short	yorder;						/**< y-order of polynomial */
	double	coeffs[MAXTERM][MAXTERM];	/**< coefficients */
	} BIPOLY;


/* Declare functions in polynomial.c */
void	init_unipoly(UNIPOLY *func);
void	copy_unipoly(UNIPOLY *fnew, const UNIPOLY *func);
double	evaluate_unipoly(UNIPOLY *func, float x);
void	differentiate_unipoly(UNIPOLY *func, UNIPOLY *dfdx);
void	find_unipoly_roots(UNIPOLY *func, float cval, float xstart, float xend,
						float *roots, int *nroot);
void	find_unipoly_range(UNIPOLY *func, float xstart, float xend,
						float *vmin, float *vmax);
double	zeroin_unipoly(UNIPOLY *func, float cval, float xstart, float xend,
						float tolerance);

void	init_bipoly(BIPOLY *func);
void	copy_bipoly(BIPOLY *fnew, const BIPOLY *func);
double	evaluate_bipoly(BIPOLY *func, POINT pos);
void	differentiate_bipoly(BIPOLY *func, BIPOLY *dfds, char coord);
void	project_bipoly(BIPOLY *func, UNIPOLY *fnew, char coord, float value);
double	zeroin_bipoly(BIPOLY *func, float cval, POINT centre, float radius,
						float start_angle, float end_angle, float tolerance);

/* Now it has been included */
#endif
