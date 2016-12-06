/***********************************************************************
*                                                                      *
*   s u p p o r t _ s f c . c                                          *
*                                                                      *
*   Obsolescent functions to handle the SURFACE object.                *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
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

#include "support.h"

#include <string.h>
#include <stdio.h>

/***********************************************************************
*                                                                      *
*      e v a l _ s u r f a c e                                         *
*      e v a l _ s u r f a c e _ 1 s t _ d e r i v                     *
*      e v a l _ s u r f a c e _ 2 n d _ d e r i v                     *
*      e v a l _ s u r f a c e _ c u r v a t u r e                     *
*      e v a l _ s u r f a c e _ u n m a p p e d                       *
*      e v a l _ s u r f a c e _ f e a t u r e                         *
*                                                                      *
*      Obsolescent functions!                                          *
*                                                                      *
*      Use equivalent eval_sfc... functions.  Most float arguments     *
*      have been promoted to double.                                   *
*                                                                      *
***********************************************************************/

float	eval_surface

	(
	SURFACE	sfc,	/* surface to be evaluated */
	POINT	p,		/* where to evaluate */
	float	*valx,	/* x-gradient component */
	float	*valy,	/* y-gradient component */
	LOGICAL	*valid	/* did it work? */
	)

	{
	double	val=0, gx=0, gy=0;
	LOGICAL	ok;

	static	LOGICAL	done = FALSE;
	if (!done)
		{
		done = TRUE;
		(void) printf("*** Obsolete function temoprarily supported:\n");
		(void) printf("Replace:\n");
		(void) printf("    SURFACE  sfc;\n");
		(void) printf("    POINT    pos;\n");
		(void) printf("    LOGICAL  valid;\n");
		(void) printf("    float    val, gx, gy;\n");
		(void) printf("    val = eval_surface(sfc, pos, &gx, &gy, &valid);\n");
		(void) printf("With:\n");
		(void) printf("    SURFACE  sfc;\n");
		(void) printf("    POINT    pos;\n");
		(void) printf("    LOGICAL  valid;\n");
		(void) printf("->  double   val, gx, gy;\n");
		(void) printf("    valid = eval_sfc(sfc, pos, &val);\n");
		(void) printf("    valid = eval_sfc_1st_deriv(sfc, pos, &gx, &gy);\n");
		(void) printf("*** End\n");
		}

	ok = eval_sfc(sfc, p, &val);
	if (valx || valy) ok = eval_sfc_1st_deriv(sfc, p, &gx, &gy);

	if (valid) *valid = ok;
	if (valx)  *valx  = (float) gx;
	if (valy)  *valy  = (float) gy;
	return (float) val;
	}

/**********************************************************************/

LOGICAL	eval_surface_1st_deriv

	(
	SURFACE	sfc,	/* surface to be evaluated */
	POINT	p,		/* where to evaluate */
	float	*valx,	/* x-gradient component */
	float	*valy	/* y-gradient component */
	)

	{
	double	gx=0, gy=0;
	LOGICAL	ok;

	static	LOGICAL	done = FALSE;
	if (!done)
		{
		done = TRUE;
		(void) printf("*** Obsolete function temoprarily supported:\n");
		(void) printf("Replace:\n");
		(void) printf("    SURFACE  sfc;\n");
		(void) printf("    POINT    pos;\n");
		(void) printf("    LOGICAL  valid;\n");
		(void) printf("    float    gx, gy;\n");
		(void) printf("    valid = eval_surface_1st_deriv(sfc, pos, &gx, &gy);\n");
		(void) printf("With:\n");
		(void) printf("    SURFACE  sfc;\n");
		(void) printf("    POINT    pos;\n");
		(void) printf("    LOGICAL  valid;\n");
		(void) printf("->  double   gx, gy;\n");
		(void) printf("    valid = eval_sfc_1st_deriv(sfc, pos, &gx, &gy);\n");
		(void) printf("*** End\n");
		}

	ok = eval_sfc_1st_deriv(sfc, p, &gx, &gy);

	if (valx)  *valx  = (float) gx;
	if (valy)  *valy  = (float) gy;
	return ok;
	}

/**********************************************************************/

LOGICAL	eval_surface_2nd_deriv

	(
	SURFACE	sfc,	/* surface to be evaluated */
	POINT	p,		/* where to evaluate */
	float	*valxx,	/* xx-gradient component */
	float	*valxy,	/* xy-gradient component (same as yx) */
	float	*valyy	/* yy-gradient component */
	)

	{
	double	gxx=0, gxy=0, gyy=0;
	LOGICAL	ok;

	static	LOGICAL	done = FALSE;
	if (!done)
		{
		done = TRUE;
		(void) printf("*** Obsolete function temoprarily supported:\n");
		(void) printf("Replace:\n");
		(void) printf("    SURFACE  sfc;\n");
		(void) printf("    POINT    pos;\n");
		(void) printf("    LOGICAL  valid;\n");
		(void) printf("    float    gxx, gxy, gyy;\n");
		(void) printf("    valid = eval_surface_2nd_deriv(sfc, pos, &gxx, &gxy, &gyy);\n");
		(void) printf("With:\n");
		(void) printf("    SURFACE  sfc;\n");
		(void) printf("    POINT    pos;\n");
		(void) printf("    LOGICAL  valid;\n");
		(void) printf("->  double   gxx, gxy, gyy;\n");
		(void) printf("    valid = eval_sfc_2nd_deriv(sfc, pos, &gxx, &gxy, &gyy);\n");
		(void) printf("*** End\n");
		}

	ok = eval_sfc_2nd_deriv(sfc, p, &gxx, &gxy, &gyy);

	if (valxx)  *valxx  = (float) gxx;
	if (valxy)  *valxy  = (float) gxy;
	if (valyy)  *valyy  = (float) gyy;
	return ok;
	}

/**********************************************************************/

LOGICAL	eval_surface_curvature

	(
	SURFACE	sfc,	/* surface to be evaluated */
	POINT	p,		/* where to evaluate */
	float	*curv,	/* returned curvature */
	POINT	centre	/* returned centre of curvature */
	)

	{
	double	k=0;
	LOGICAL	ok;

	static	LOGICAL	done = FALSE;
	if (!done)
		{
		done = TRUE;
		(void) printf("*** Obsolete function temoprarily supported:\n");
		(void) printf("Replace:\n");
		(void) printf("    SURFACE  sfc;\n");
		(void) printf("    POINT    pos, centre;\n");
		(void) printf("    LOGICAL  valid;\n");
		(void) printf("    float    curv;\n");
		(void) printf("    valid = eval_surface_curvature(sfc, pos, &curv, centre);\n");
		(void) printf("With:\n");
		(void) printf("    SURFACE  sfc;\n");
		(void) printf("    POINT    pos, centre;\n");
		(void) printf("    LOGICAL  valid;\n");
		(void) printf("->  double   curv;\n");
		(void) printf("    valid = eval_sfc_curvature(sfc, pos, &curv, centre);\n");
		(void) printf("*** End\n");
		}

	ok = eval_sfc_curvature(sfc, p, &k, centre);

	if (curv) *curv = (float) k;
	return ok;
	}

/**********************************************************************/

float	eval_surface_unmapped

	(
	SURFACE	sfc,	/* surface to be evaluated */
	POINT	p,		/* where to evaluate */
	LOGICAL	*valid	/* did it work? */
	)

	{
	double	val=0;
	LOGICAL	ok;

	static	LOGICAL	done = FALSE;
	if (!done)
		{
		done = TRUE;
		(void) printf("*** Obsolete function temoprarily supported:\n");
		(void) printf("Replace:\n");
		(void) printf("    SURFACE  sfc;\n");
		(void) printf("    POINT    pos;\n");
		(void) printf("    LOGICAL  valid;\n");
		(void) printf("    float    val;\n");
		(void) printf("    val = eval_surface_unmapped(sfc, pos, &valid);\n");
		(void) printf("With:\n");
		(void) printf("    SURFACE  sfc;\n");
		(void) printf("    POINT    pos;\n");
		(void) printf("    LOGICAL  valid;\n");
		(void) printf("->  double   val;\n");
		(void) printf("    valid = eval_sfc_unmapped(sfc, pos, &val);\n");
		(void) printf("*** End\n");
		}

	ok = eval_sfc_unmapped(sfc, p, &val);

	if (valid) *valid = ok;
	return (float) val;
	}

/**********************************************************************/

STRING	eval_surface_feature

	(
	SURFACE	sfc,		/* surface to be evaluated */
	POINT	p,			/* where to start looking */
	STRING	features,	/* which features to search (contours, maxima, etc) */
	POINT	plab,		/* where to place label */
	char	*which,		/* which feature was closest */
	ITEM	*item,		/* curve or mark where label goes */
	LOGICAL	*valid		/* did it work? */
	)

	{
	static	LOGICAL	done = FALSE;
	if (!done)
		{
		done = TRUE;
		(void) printf("*** Obsolete function temoprarily supported:\n");
		(void) printf("Replace:\n");
		(void) printf("    val = eval_surface_feature(sfc, pos, ...);\n");
		(void) printf("With:\n");
		(void) printf("    val = eval_sfc_feature(sfc, pos, ...);\n");
		(void) printf("*** End\n");
		}

	return eval_sfc_feature(sfc, p, features, plab, which, item, valid);
	}
