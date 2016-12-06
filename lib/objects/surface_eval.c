/*********************************************************************/
/** @file surface_eval.c
 *
 * Routines to handle the SURFACE object.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*      s u r f a c e _ e v a l . c                                     *
*                                                                      *
*      Routines to handle the SURFACE object.                          *
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

#include "surface.h"
#include "set_oper.h"

#include <tools/tools.h>

#include <string.h>
#include <stdio.h>

/***********************************************************************
*                                                                      *
*      e v a l _ s f c                                                 *
*      e v a l _ s f c _ 1 s t _ d e r i v                             *
*      e v a l _ s f c _ 2 n d _ d e r i v                             *
*      e v a l _ s f c _ c u r v a t u r e                             *
*                                                                      *
*                                                                      *
*      Note: Most of the literature provides a formula for curvature   *
*            for either univariate curves, defined as y = f(x), or     *
*            parametric curves, defined as x = f(t), y = g(t).         *
*                                                                      *
*            I could find only one source (Calculus and Analytical     *
*            Geometry, by Tierney) that seems to provide a formula     *
*            for contours of a function, defined as f(x,y) = 0.        *
*            However, this formulation, given as:                      *
*                                                                      *
*                    (fy*fxx - 2fx*fy*fxy + fx*fyy)                    *
*                K = ______________________________                    *
*                                                                      *
*                        (fx*fx + fy*fy) ^ (3/2)                       *
*                                                                      *
*            turned out to be suspicious, so I re-developed it from    *
*            first principles and discovered his error.  The correct   *
*            formulation should be:                                    *
*                                                                      *
*                    (fy*fy*fxx - 2fx*fy*fxy + fx*fx*fyy)              *
*                K = ____________________________________              *
*                                                                      *
*                           (fx*fx + fy*fy) ^ (3/2)                    *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Evaluates the surface spline at the given point.
 *
 *  @param[in] 	sfc		surface to be evaluated
 *  @param[in] 	p		where to evaluate
 *  @param[out]	*val	value
 *  @return True if successful.
 *********************************************************************/
LOGICAL	eval_sfc

	(
	SURFACE	sfc,
	POINT	p,
	double	*val
	)

	{
	int		iup, ivp;
	POINT	pp, dp;
	PATCH	patch;
	LOGICAL	inside;

	/* Do nothing if surface undefined */
	if (val) *val = 0;
	if (!sfc) return FALSE;
	if (!p)   return FALSE;

	/* Find patch which contains the given point */
	inside = find_patch(&sfc->sp, p, &iup, &ivp, pp, dp);

	/* Now evaluate the patch function */
	if (val)
		{
		patch = prepare_sfc_patch(sfc, iup, ivp);
		if (!patch) return FALSE;
		*val  = (float) evaluate_bipoly(&patch->function, pp);
		patch = dispose_sfc_patch(sfc, iup, ivp);
		}

	return inside;
	}

/**********************************************************************/

/*********************************************************************/
/** Evaluates both components of the 1st derivative
 * from the given scalar field.
 *
 *	@param[in] 	sfc		surface to be evaluated
 *	@param[in] 	p		where to evaluate
 *	@param[out]	*valx	x-gradient component
 *	@param[out]	*valy	y-gradient component
 * 	@return True if successful.
 *********************************************************************/
LOGICAL	eval_sfc_1st_deriv

	(
	SURFACE	sfc,
	POINT	p,
	double	*valx,
	double	*valy
	)

	{
	int		iup, ivp;
	double	sxx, sxy, syx, syy, gx, gy;
	float	cx, cy;
	POINT	pp, dp;
	PATCH	patch;
	BIPOLY	derivx, derivy;
	LOGICAL	inside;

	/* Do nothing if surface undefined */
	if (valx) *valx = 0;
	if (valy) *valy = 0;
	if (!sfc) return FALSE;
	if (!p)   return FALSE;

	/* Find patch which contains the given point */
	inside = find_patch(&sfc->sp, p, &iup, &ivp, pp, dp);

	/* Now evaluate the gradients if requested */
	if (valx || valy)
		{
		patch = prepare_sfc_patch(sfc, iup, ivp);
		if (!patch) return FALSE;

		differentiate_bipoly(&patch->function, &derivx, 'x');
		differentiate_bipoly(&patch->function, &derivy, 'y');

		/* >>> should go into find_patch (dp corrected) <<< */
		pos_distort(&sfc->sp.mp, p, &cx, &cy);
		dp[X] /= cx;
		dp[Y] /= cy;
		gx = (float) evaluate_bipoly(&derivx, pp) / dp[X];
		gy = (float) evaluate_bipoly(&derivy, pp) / dp[Y];

		sxx = sfc->sp.xform[X][X];
		sxy = sfc->sp.xform[X][Y];
		syx = sfc->sp.xform[Y][X];
		syy = sfc->sp.xform[Y][Y];

		if (valx) *valx = gx*sxx + gy*syx;
		if (valy) *valy = gx*sxy + gy*syy;

		patch = dispose_sfc_patch(sfc, iup, ivp);
		}

	return inside;
	}

/**********************************************************************/

/*********************************************************************/
/** Evaluates all 3 2nd derivatives from the given
 * scalar field.
 *
 *	@param[in] 	sfc		surface to be evaluated
 *	@param[in] 	p		where to evaluate
 *	@param[out]	*valxx	xx-gradient component
 *	@param[out]	*valxy	xy-gradient component (same as yx)
 *	@param[out]	*valyy	yy-gradient component
 * 	@return True if successful.
 *********************************************************************/
LOGICAL	eval_sfc_2nd_deriv

	(
	SURFACE	sfc,
	POINT	p,
	double	*valxx,
	double	*valxy,
	double	*valyy
	)

	{
	int		iup, ivp;
	float	cx, cy;
	double	gxx, gxy, gyy;
	double	sxx, sxy, syx, syy;
	POINT	pp, dp;
	PATCH	patch;
	BIPOLY	derivx, derivy;
	BIPOLY	derivxx, derivxy, derivyy;
	LOGICAL	inside;

	/* Do nothing if surface undefined */
	if (valxx) *valxx = 0;
	if (valxy) *valxy = 0;
	if (valyy) *valyy = 0;
	if (!sfc) return FALSE;
	if (!p)   return FALSE;

	/* Find patch which contains the given point */
	inside = find_patch(&sfc->sp, p, &iup, &ivp, pp, dp);

	/* Now evaluate the gradients if requested */
	if (valxx || valxy || valyy)
		{
		patch = prepare_sfc_patch(sfc, iup, ivp);
		if (!patch) return FALSE;

		differentiate_bipoly(&patch->function, &derivx, 'x');
		differentiate_bipoly(&patch->function, &derivy, 'y');
		differentiate_bipoly(&derivx, &derivxx, 'x');
		differentiate_bipoly(&derivx, &derivxy, 'y');
		differentiate_bipoly(&derivy, &derivyy, 'y');

		/* >>> should go into find_patch (dp corrected) <<< */
		pos_distort(&sfc->sp.mp, p, &cx, &cy);
		dp[X] /= cx;
		dp[Y] /= cy;
		gxx = (float) evaluate_bipoly(&derivxx, pp) / dp[X] / dp[X];
		gxy = (float) evaluate_bipoly(&derivxy, pp) / dp[X] / dp[Y];
		gyy = (float) evaluate_bipoly(&derivyy, pp) / dp[Y] / dp[Y];

		sxx = sfc->sp.xform[X][X];
		sxy = sfc->sp.xform[X][Y];
		syx = sfc->sp.xform[Y][X];
		syy = sfc->sp.xform[Y][Y];

		if (valxx) *valxx = (gxx*sxx + gxy*syx) * sxx
						  + (gxy*sxx + gyy*syx) * syx;
		if (valxy) *valxy = (gxx*sxy + gxy*syy) * sxx
						  + (gxy*sxy + gyy*syy) * syx;
		if (valyy) *valyy = (gxx*sxy + gxy*syy) * sxy
						  + (gxy*sxy + gyy*syy) * syy;

		patch = dispose_sfc_patch(sfc, iup, ivp);
		}

	return inside;
	}

/**********************************************************************/

/*********************************************************************/
/** Computes the curvature of an imaginary contour
 * line passing through the given point.
 *
 *	@param[in] 	sfc		surface to be evaluated
 *	@param[in] 	p		where to evaluate
 *	@param[out]	*curv	returned curvature
 *	@param[out]	centre	returned centre of curvature
 * 	@return True if successful.
 *********************************************************************/
LOGICAL	eval_sfc_curvature

	(
	SURFACE	sfc,
	POINT	p,
	double	*curv,
	POINT	centre
	)

	{
	double	vx, vy, vxx, vxy, vyy;
	double	top, bottom;
	LOGICAL	inside;

	/* Do nothing if surface undefined */
	if (curv)   *curv = 0.0;
	if (centre) copy_point(centre, p);
	if (!sfc) return FALSE;
	if (!p)   return FALSE;

	/* Obtain required derivatives */
	inside = eval_sfc_1st_deriv(sfc, p, &vx, &vy);
	inside = eval_sfc_2nd_deriv(sfc, p, &vxx, &vxy, &vyy);

	/* Compute curvature value */
	top    = vxx*vy*vy - 2*vxy*vx*vy + vyy*vx*vx;
	bottom = hypot(vx, vy);
	if (bottom == 0)
		{
		if (curv)
			{
			if (top == 0) *curv = 0.0;
			else          *curv = SIGN(top) * FPA_FLT_MAX;
			}
		}
	else
		{
		if (curv) *curv = (float) (top / bottom / bottom / bottom);
		if (centre && top != 0)
			{
			centre[X] -= vx * bottom * bottom / top;
			centre[Y] -= vy * bottom * bottom / top;
			}
		}

	return inside;
	}

/***********************************************************************
*                                                                      *
*      e v a l _ s f c _ U V                                           *
*      e v a l _ s f c _ M D                                           *
*      e v a l _ s f c _ c o m p _ 1 s t _ d e r i v                   *
*      e v a l _ s f c _ c o m p _ 2 n d _ d e r i v                   *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Evaluates U and V components from the given vector field
 * (2D surface spline) at the given point.
 *
 *	@param[in] 	sfc		surface to be evaluated
 *	@param[in] 	p		where to evaluate
 *	@param[out]	*uval	U value
 *	@param[out]	*vval	V value
 *  @return True if successful.
 *********************************************************************/
LOGICAL	eval_sfc_UV

	(
	SURFACE	sfc,
	POINT	p,
	double	*uval,
	double	*vval
	)

	{
	int		iup, ivp;
	POINT	pp, dp;
	PATCH	patch;
	LOGICAL	inside;

	/* Do nothing if surface undefined */
	if (uval) *uval = 0.0;
	if (vval) *vval = 0.0;
	if (!sfc) return FALSE;
	if (!p)   return FALSE;

	/* Find patch which contains the given point */
	inside = find_patch(&sfc->sp, p, &iup, &ivp, pp, dp);

	if (uval || vval)
		{
		patch = prepare_sfc_patch(sfc, iup, ivp);
		if (!patch) return FALSE;
		if (uval) *uval = evaluate_bipoly(&patch->xfunc, pp);
		if (vval) *vval = evaluate_bipoly(&patch->yfunc, pp);
		patch = dispose_sfc_patch(sfc, iup, ivp);
		}

	return inside;
	}

/**********************************************************************/

/*********************************************************************/
/** Evaluates magnitude and direction from the given vector
 *  field at the given point.
 *
 *	@param[in] 	sfc		surface to be evaluated
 *	@param[in] 	p		where to evaluate
 *	@param[out]	*mag	magnitude
 *	@param[out]	*dir	direction
 *  @return True if successful.
 *********************************************************************/
LOGICAL	eval_sfc_MD

	(
	SURFACE	sfc,
	POINT	p,
	double	*mag,
	double	*dir
	)

	{
	LOGICAL	inside;
	double	uval, vval;

	/* Do nothing if surface undefined */
	if (mag) *mag = 0.0;
	if (dir) *dir = 0.0;
	if (!sfc) return FALSE;
	if (!p)   return FALSE;

	inside = eval_sfc_UV(sfc, p, &uval, &vval);

	if (mag) *mag = hypot(vval, uval);
	if (dir) *dir = fpa_atan2deg(vval, uval);

	return inside;
	}

/**********************************************************************/

/*********************************************************************/
/** Evaluates both components of the 1st derivative from the
 * 	specified component of the given vector field.
 *
 *	@param[in] 	sfc		surface to be evaluated
 *	@param[in] 	p		where to evaluate
 *	@param[in] 	which	which component to use?
 *	@param[out]	*valx	x-gradient component
 *	@param[out]	*valy	y-gradient component
 *  @return True if successful.
 *********************************************************************/
LOGICAL	eval_sfc_comp_1st_deriv

	(
	SURFACE	sfc,
	POINT	p,
	int		which,
	double	*valx,
	double	*valy
	)

	{
	int		iup, ivp;
	double	sxx, sxy, syx, syy,	gx, gy;
	float	cx, cy;
	POINT	pp, dp;
	PATCH	patch;
	BIPOLY	*pf, derivx, derivy;
	LOGICAL	inside;

	/* Do nothing if surface undefined */
	if (valx) *valx = 0;
	if (valy) *valy = 0;
	if (!sfc) return FALSE;
	if (!p)   return FALSE;

	/* Find patch which contains the given point */
	inside = find_patch(&sfc->sp, p, &iup, &ivp, pp, dp);

	/* Now evaluate the gradients if requested */
	if (valx || valy)
		{
		patch = prepare_sfc_patch(sfc, iup, ivp);
		if (!patch) return FALSE;

		switch (which)
			{
			case M_Comp:	pf = &patch->function;	break;
			case X_Comp:	pf = &patch->xfunc;		break;
			case Y_Comp:	pf = &patch->yfunc;		break;
			default:		return FALSE;
			}

		differentiate_bipoly(pf, &derivx, 'x');
		differentiate_bipoly(pf, &derivy, 'y');

		/* >>> should go into find_patch (dp corrected) <<< */
		pos_distort(&sfc->sp.mp, p, &cx, &cy);
		dp[X] /= cx;
		dp[Y] /= cy;
		gx = evaluate_bipoly(&derivx, pp) / dp[X];
		gy = evaluate_bipoly(&derivy, pp) / dp[Y];

		sxx = sfc->sp.xform[X][X];
		sxy = sfc->sp.xform[X][Y];
		syx = sfc->sp.xform[Y][X];
		syy = sfc->sp.xform[Y][Y];

		if (valx) *valx = gx*sxx + gy*syx;
		if (valy) *valy = gx*sxy + gy*syy;

		patch = dispose_sfc_patch(sfc, iup, ivp);
		}

	return inside;
	}

/**********************************************************************/

/*********************************************************************/
/** Evaluates all 3 2nd derivatives from the specified  component of
 * the given vector field.
 *
 *	@param[in] 	sfc 	surface to be evaluated
 *	@param[in] 	p		where to evaluate
 *	@param[in] 	which	which component to use?
 *	@param[out]	*valxx	xx-gradient component
 *	@param[out]	*valxy	xy-gradient component (same as yx)
 *	@param[out]	*valyy	yy-gradient component
 * 	@return True if successful.
 *********************************************************************/
LOGICAL	eval_sfc_comp_2nd_deriv

	(
	SURFACE	sfc,
	POINT	p,
	int		which,
	double	*valxx,
	double	*valxy,
	double	*valyy
	)

	{
	int		iup, ivp;
	float	cx, cy;
	double	gxx, gxy, gyy;
	double	sxx, sxy, syx, syy;
	POINT	pp, dp;
	PATCH	patch;
	BIPOLY	*pf, derivx, derivy;
	BIPOLY	derivxx, derivxy, derivyy;
	LOGICAL	inside;

	/* Do nothing if surface undefined */
	if (valxx) *valxx = 0;
	if (valxy) *valxy = 0;
	if (valyy) *valyy = 0;
	if (!sfc) return FALSE;
	if (!p)   return FALSE;

	/* Find patch which contains the given point */
	inside = find_patch(&sfc->sp, p, &iup, &ivp, pp, dp);

	/* Now evaluate the gradients if requested */
	if (valxx || valxy || valyy)
		{
		patch = prepare_sfc_patch(sfc, iup, ivp);
		if (!patch) return FALSE;

		switch (which)
			{
			case M_Comp:	pf = &patch->function;	break;
			case X_Comp:	pf = &patch->xfunc;		break;
			case Y_Comp:	pf = &patch->yfunc;		break;
			default:		return FALSE;
			}

		differentiate_bipoly(pf, &derivx, 'x');
		differentiate_bipoly(pf, &derivy, 'y');
		differentiate_bipoly(&derivx, &derivxx, 'x');
		differentiate_bipoly(&derivx, &derivxy, 'y');
		differentiate_bipoly(&derivy, &derivyy, 'y');

		/* >>> should go into find_patch (dp corrected) <<< */
		pos_distort(&sfc->sp.mp, p, &cx, &cy);
		dp[X] /= cx;
		dp[Y] /= cy;
		gxx = evaluate_bipoly(&derivxx, pp) / dp[X] / dp[X];
		gxy = evaluate_bipoly(&derivxy, pp) / dp[X] / dp[Y];
		gyy = evaluate_bipoly(&derivyy, pp) / dp[Y] / dp[Y];

		sxx = sfc->sp.xform[X][X];
		sxy = sfc->sp.xform[X][Y];
		syx = sfc->sp.xform[Y][X];
		syy = sfc->sp.xform[Y][Y];

		if (valxx) *valxx = (gxx*sxx + gxy*syx) * sxx
						  + (gxy*sxx + gyy*syx) * syx;
		if (valxy) *valxy = (gxx*sxy + gxy*syy) * sxx
						  + (gxy*sxy + gyy*syy) * syx;
		if (valyy) *valyy = (gxx*sxy + gxy*syy) * sxy
						  + (gxy*sxy + gyy*syy) * syy;

		patch = dispose_sfc_patch(sfc, iup, ivp);
		}

	return inside;
	}

/***********************************************************************
*                                                                      *
*      e v a l _ s f c _ u n m a p p e d                               *
*      e v a l _ s f c _ U V _ u n m a p p e d                         *
*      e v a l _ s f c _ M D _ u n m a p p e d                         *
*                                                                      *
*      These functions do the same thing as their equivalents above,   *
*      except assuming the evaluation point is expressed in spline     *
*      co-ordinates rather than world co-ordinates.                    *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Evaluates the surface spline at the given point. Assume that the
 *  evaluation point is expressed in spline co-ordinates rather than
 *  world co-ordinates.
 *
 *	@param[in] 	sfc		surface to be evaluated
 *	@param[in] 	p		where to evaluate
 *	@param[out]	*val	value
 *  @return True if successful.
 *********************************************************************/
LOGICAL	eval_sfc_unmapped

	(
	SURFACE	sfc,
	POINT	p,
	double	*val
	)

	{
	int		iup, ivp;
	POINT	pp, dp;
	PATCH	patch;
	LOGICAL	inside;

	/* Do nothing if surface undefined */
	if (val) *val = 0.0;
	if (!sfc) return FALSE;
	if (!p)   return FALSE;

	/* Find patch which contains the given point */
	inside = find_patch_unmapped(&sfc->sp, p, &iup, &ivp, pp, dp);

	/* Now evaluate the patch function */
	if (val)
		{
		patch = prepare_sfc_patch(sfc, iup, ivp);
		if (!patch) return FALSE;
		*val = evaluate_bipoly(&patch->function, pp);
		patch = dispose_sfc_patch(sfc, iup, ivp);
		}

	return inside;
	}

/**********************************************************************/

/*********************************************************************/
/** Evaluates U and V components from the given vector field
 * (2D surface spline) at the given point. Assume that the evaluation
 * point is expressed in spline co-ordinates rather than world
 * co-ordinates.
 *
 *	@param[in] 	sfc 	surface to be evaluated
 *	@param[in] 	p		where to evaluate
 *	@param[out]	*uval	U value
 *	@param[out]	*vval	V value
 *  @return True if successful.
 *********************************************************************/
LOGICAL	eval_sfc_UV_unmapped

	(
	SURFACE	sfc,
	POINT	p,
	double	*uval,
	double	*vval
	)

	{
	int		iup, ivp;
	POINT	pp, dp;
	PATCH	patch;
	LOGICAL	inside;

	/* Do nothing if surface undefined */
	if (uval) *uval = 0.0;
	if (vval) *vval = 0.0;
	if (!sfc) return FALSE;
	if (!p)   return FALSE;

	/* Find patch which contains the given point */
	inside = find_patch_unmapped(&sfc->sp, p, &iup, &ivp, pp, dp);

	if (uval || vval)
		{
		patch = prepare_sfc_patch(sfc, iup, ivp);
		if (!patch) return FALSE;
		if (uval) *uval = evaluate_bipoly(&patch->xfunc, pp);
		if (vval) *vval = evaluate_bipoly(&patch->yfunc, pp);
		patch = dispose_sfc_patch(sfc, iup, ivp);
		}

	return inside;
	}

/**********************************************************************/

/*********************************************************************/
/** Evaluates magnitude and direction from the given vector
 *  field at the given point. Assume that the evaluation point is
 *  expressed in spline co-ordinates rather than world co-ordinates.
 *
 *	@param[in] 	sfc 	 surface to be evaluated
 *	@param[in] 	p		 where to evaluate
 *	@param[out]	*mag	 magnitude
 *	@param[out]	*dir	 direction
 *  @return True if successful.
 *********************************************************************/
LOGICAL	eval_sfc_MD_unmapped

	(
	SURFACE	sfc,
	POINT	p,
	double	*mag,
	double	*dir
	)

	{
	LOGICAL	inside;
	double	uval, vval;

	/* Do nothing if surface undefined */
	if (mag) *mag = 0.0;
	if (dir) *dir = 0.0;
	if (!sfc) return FALSE;
	if (!p)   return FALSE;

	inside = eval_sfc_UV_unmapped(sfc, p, &uval, &vval);

	if (mag) *mag = hypot(vval, uval);
	if (dir) *dir = fpa_atan2deg(vval, uval);

	return inside;
	}

/***********************************************************************
*                                                                      *
*      e v a l _ s f c _ f e a t u r e                                 *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Evaluates the surface spline at the closest feature of the
 *  requested type.  Feature code is the concatenation of any of
 *  the following:
 * 	- c = contours
 * 	- x = maxima
 * 	- n = minima
 * 	- s = saddle points
 *
 *	@param[in] 	sfc 		surface to be evaluated
 *	@param[in] 	p 			where to start looking
 *	@param[in] 	features 	which features to search (contours, maxima, etc)
 *	@param[in] 	plab 		where to place label
 *	@param[out]	*which 		which feature was closest
 *	@param[out]	*item 		curve or mark where label goes
 *	@param[out]	*valid		did it work?
 * 	@return Pointer to the feature's label if successful.
 * 			Null otherwise.
 *********************************************************************/

STRING	eval_sfc_feature

	(
	SURFACE	sfc,
	POINT	p,
	STRING	features,
	POINT	plab,
	char	*which,
	ITEM	*item,
	LOGICAL	*valid
	)

	{
	int		i, iup, ivp, jup, jvp, seg;
	float	dist, dmin;
	double	fval;
	POINT	ps, pp, dp, pslab, psmin, pplab;
	SET		contours, extrema;
	SPLINE	*spline;
	PATCH	patch;
	CURVE	curve;
	MARK	mark;
	STRING	value, sub, val, lab;
	LOGICAL	search_x, search_n, search_s, search_c;
	LOGICAL	found_x, found_n, found_s;
	LOGICAL	ok;

	static	char	valbuf[10];

	/* Do nothing if surface undefined */
	value = NULL;
	if (plab)  copy_point(plab, ZeroPoint);
	if (which) *which = '\0';
	if (item)  *item  = (ITEM) 0;
	if (valid) *valid = FALSE;
	if (!sfc)                 return value;
	if (!(spline = &sfc->sp)) return value;
	if (!p)                   return value;

	/* Find patch which contains the given point */
	if (!world_to_spline(spline, p, ps))                      return value;
	if (!find_patch_unmapped(spline, ps, &iup, &ivp, pp, dp)) return value;

	/* Interpret feature code */
	if (blank(features)) return value;
	search_x = (LOGICAL) (strchr(features, 'x') != NULL);
	search_n = (LOGICAL) (strchr(features, 'n') != NULL);
	search_s = (LOGICAL) (strchr(features, 's') != NULL);
	search_c = (LOGICAL) (strchr(features, 'c') != NULL);

	dmin = -1;

	/* Search neighbouring patches as well */
	for (jup=iup-1; jup<=iup+1; jup++)
		{
		if (jup < 0)             continue;
		if (jup >= sfc->nupatch) break;
		for (jvp=ivp-1; jvp<=ivp+1; jvp++)
			{
			if (jvp < 0)             continue;
			if (jvp >= sfc->nvpatch) break;

			if (!spline_to_patch(spline, ps, jup, jvp, pp, dp)) continue;

			patch    = prepare_sfc_patch(sfc, jup, jvp);
			extrema  = patch->extrema;
			contours = patch->contours;

			/* Find the closest max, min and saddle in the patch */
			if (extrema && (search_x || search_n || search_s))
				{
				for (i=0; i<extrema->num; i++)
					{
					/* See if mark is one we're looking for */
					mark = (MARK) extrema->list[i];
					if (!mark) continue;
					get_default_attributes(mark->attrib, &sub, &val, &lab);
					found_x = same(sub, "maxima");
					found_n = same(sub, "minima");
					found_s = same(sub, "saddle");
					if (!found_x && !found_n && !found_s) continue;
					if (found_x && !search_x)             continue;
					if (found_n && !search_n)             continue;
					if (found_s && !search_s)             continue;

					/* Check distance from point */
					if (!patch_to_spline(spline, mark->anchor, jup, jvp,
						pslab)) continue;
					dist = hypot((double) (ps[X]-pslab[X]),
									(double) (ps[Y]-pslab[Y]));
					if (dmin<0 || dist<dmin)
						{
						copy_point(psmin, pslab);
						dmin  = dist;
						value = lab;
						if (item) *item = (ITEM) mark;
						if (which)
							{
							if (found_x) *which = 'x';
							if (found_n) *which = 'n';
							if (found_s) *which = 's';
							}
						}
					}
				}

			/* Now find the closest contour in the patch */
			if (contours && search_c)
				{
				if (curve = closest_curve(contours, pp, &dist, pplab, &seg))
					{
					get_default_attributes(curve->attrib, &sub, &val, &lab);
					/* Check distance from point */
					(void) patch_to_spline(spline, pplab, jup, jvp, pslab);
					dist = hypot((double) (ps[X]-pslab[X]),
									(double) (ps[Y]-pslab[Y]));
					if (dmin<0 || dist<dmin)
						{
						copy_point(psmin, pslab);
						dmin  = dist;
						value = val;
						if (item)  *item = (ITEM) curve;
						if (which) *which = 'c';
						}
					}
				}
			}
		}

	/* Did we find anything? */
	if (dmin < 0)
		{
		patch = dispose_sfc_patch(sfc, iup, ivp);
		return value;
		}

	/* Transform label point back to normal co-ordinate system */
	if (plab)
		{
		(void) spline_to_world(spline, psmin, plab);
		}

	/* Evaluate if necessary */
	if (valid) *valid = TRUE;
	if (value) return value;
	ok = eval_sfc_unmapped(sfc, psmin, &fval);
	(void) sprintf(valbuf, "%d", NINT(fval));
	if (valid) *valid = ok;
	return valbuf;
	}
