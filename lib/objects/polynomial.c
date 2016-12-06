/*********************************************************************/
/** @file polynomial.c
 *
 * Routines to handle the UNIPOLY and BIPOLY objects.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
#undef DEBUG_POLY
/***********************************************************************
*                                                                      *
*      p o l y n o m i a l . c                                         *
*                                                                      *
*      Routines to handle the UNIPOLY and BIPOLY objects.              *
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

#define POLYNOMIAL_INIT
#include "polynomial.h"

#include <tools/tools.h>

#include <string.h>
#include <stdio.h>

/* Define pointers to fast evaluation routines */
typedef double (FF1)(double [], float);
typedef double (FF2)(double [][MAXTERM], float, float);
static	FF1 eval_0, eval_1, eval_2, eval_3;
static	FF1 *eval1d[] =
		{ eval_0, eval_1, eval_2, eval_3 };
static	FF2 eval_0_0, eval_0_1, eval_0_2, eval_0_3;
static	FF2 eval_1_0, eval_1_1, eval_1_2, eval_1_3;
static	FF2 eval_2_0, eval_2_1, eval_2_2, eval_2_3;
static	FF2 eval_3_0, eval_3_1, eval_3_2, eval_3_3;
static	FF2 *eval2d[][MAXTERM] =
		{
		{ eval_0_0, eval_0_1, eval_0_2, eval_0_3 },
		{ eval_1_0, eval_1_1, eval_1_2, eval_1_3 },
		{ eval_2_0, eval_2_1, eval_2_2, eval_2_3 },
		{ eval_3_0, eval_3_1, eval_3_2, eval_3_3 }
		};

/***********************************************************************
*                                                                      *
*      i n i t _ u n i p o l y                                         *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Initialize unipoly polynomial coefficients.
 *
 *	@param[out]	*f		function to be initialized
 *********************************************************************/

void init_unipoly

	(
	UNIPOLY			*f
	)

	{
	int	i;

	/* Return if undefined */
	if (!f) return;

	f->order = 0;
	for (i=0; i<MAXTERM; i++)
		f->coeffs[i] = 0;
	}

/***********************************************************************
*                                                                      *
*      c o p y _ u n i p o l y                                         *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Copy the second unipoly into the first.
 *
 *	@param[out]	*fnew	function to be copied into
 *	@param[in] 	*f		function to be copied
 *********************************************************************/

void copy_unipoly

	(
	UNIPOLY			*fnew,
	const UNIPOLY	*f
	)

	{
	int	i;

	/* Return if undefined */
	if (!f)    return;
	if (!fnew) return;

	init_unipoly(fnew);

	fnew->order = f->order;
	for (i=0; i<=f->order; i++)
		fnew->coeffs[i] = f->coeffs[i];
	}

/***********************************************************************
*                                                                      *
*      e v a l u a t e _ u n i p o l y                                 *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Evaluate the given unipoly at the specified value.
 * Horner's rule is used.
 *
 *	@param[in] 	*f	function to be evaluated
 *	@param[in] 	x	location at which to evaluate
 *  @return The value of the polynomial evaluated at the given location.
 *********************************************************************/

double evaluate_unipoly

	(
	UNIPOLY	*f,
	float	x
	)

	{
	/* Return 0 if undefined */
	if (!f) return 0;
	if (f->order < 0) return 0;

	/* Special case where x=0 */
	if (x == 0) return f->coeffs[0];

	/* Evaluate using explicit Horner's rule */
	return eval1d[f->order] (f->coeffs,x);
	}

/***********************************************************************
*                                                                      *
*      d i f f e r e n t i a t e _ u n i p o l y                       *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Generate the first derivative of the given unipoly f, and
 * return it in dfdx.  If dfdx and f point to the same place,
 * then the original is correctly overwritten by its derivative.
 *
 *	@param[in] 	*f		given unipoly
 *	@param[in] 	*dfdx	resulting derivative unipoly
 *********************************************************************/

void differentiate_unipoly

	(
	UNIPOLY	*f,
	UNIPOLY	*dfdx
	)

	{
	int	term, order;

	/* Return if undefined */
	if (!f)    return;
	if (!dfdx) return;

	init_unipoly(dfdx);

	/* Setup derivative order */
	order = f->order;
	while (f->coeffs[order] == 0 && order > 0) order--;
	if (order < 1)
		{
		dfdx->order = 0;
		dfdx->coeffs[0] = 0;
		return;
		}

	/* Differentiate each term */
	dfdx->order = order - 1;
	for (term=1; term<=order; term++)
		dfdx->coeffs[term-1] = term * f->coeffs[term];
	return;
	}

/***********************************************************************
*                                                                      *
*      f i n d _ u n i p o l y _ r o o t s                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find the roots of @f$ f({x}) - {c} = 0 @f$, where f is the
 * given unipoly.
 *
 * The main approach here is to sub-divide the range into sub-ranges
 * that are either monotonically increasing or decreasing. This is
 * done by locating the function's stationary points
 * @f$(f'({x}) = 0) @f$. The end points are also included.
 *
 * Once this is done, we know that each sub-range has at most
 * one root.  This can be trivially detected by testing for
 * opposite signs at either end.  If a root is so indicated,
 * we can use a conventional root finder to zero in on it.
 *
 *	@param[in] 	*f		given ploynomial
 *	@param[in] 	cval	given constant
 *	@param[in] 	xstart	left boundary
 *	@param[in] 	xend	right boundary
 *	@param[out]	*roots	roots returned
 *	@param[out]	*nroot	number of roots found
 *********************************************************************/

void find_unipoly_roots

	(
	UNIPOLY	*f,
	float	cval,
	float	xstart,
	float	xend,
	float	*roots,
	int		*nroot
	)

	{
	double	f1, f2, s1, s2, x1, x2, rs, r1, r2, r3;
	double	a, b, c, d, p, q, asub, t1, t2, t3, ang, cang, rbar, rdif, det;
	UNIPOLY	dfdx;
	float	droots[MAXTERM-1];
	int	sub, ndroot, order;

	static	double	tol = 1e-6;
	static	double	a60 = PI*.666666667;
#	define	cbrt(a) SIGN(a)*pow(fabs(a),.3333333333);

	/* Return if undefined */
	if (!f)     return;
	if (!nroot) return;
	if (!roots) return;

	/* Assume no roots for now and determine effective order */
	*nroot = 0;
	order  = f->order;
	while (order > 0)
		{
		a = f->coeffs[order];
		b = f->coeffs[order-1];
		if (order == 1) b -= cval;
		if (fabs(a) > tol*fabs(b)) break;
		order--;
		}

	/* Handle higher orders which require a root finder */
	if (order > MAX_EXPLICIT_ORDER)
		{
		differentiate_unipoly(f,&dfdx);
		find_unipoly_roots(&dfdx,0.,xstart,xend,droots,&ndroot);
		x1 = xstart;
		f1 = eval1d[f->order] (f->coeffs,x1) - cval;
		s1 = SIGN(f1);
		for (sub=0; sub<=ndroot; sub++)
			{
			if (sub == ndroot) x2 = xend;
			else x2 = droots[sub];
			f2 = eval1d[f->order] (f->coeffs,x2) - cval;
			s2 = SIGN(f2);
			if (s1 != s2) roots[(*nroot)++] =
				(float) zeroin_unipoly(f,cval,(float) x1,(float) x2,.0001);
			else if(fabs(f1) <= tol) roots[(*nroot)++] = (float) x1;
			else if(fabs(f2) <= tol) roots[(*nroot)++] = (float) x2;
			x1 = x2;
			f1 = f2;
			s1 = s2;
			}
		}

	/* Handle constant function - no solution */
	/* Ignore possible case where constant function equals cval everywhere */
	else if (order <= 0)
		return;

	/* Handle linear function - explicit solution */
	else if (order == 1)
		{
		r1 = (-b / a);
		if ((r1>=xstart) && (r1<=xend)) roots[(*nroot)++] = (float) r1;
		return;
		}

	/* Handle quadric function - classical explicit solution */
	/* See CRC Standard Mathematical Tables, for example */
	else if (order == 2)
		{
		c = f->coeffs[0] - cval;

		/* Determine how many roots */
		rbar = -b / (2*a);
		det  = rbar*rbar - (c/a);

		/* No real roots */
		if (det < 0) return;

		/* Two real roots - These will be in increasing order */
		rdif = sqrt(det);
		r1   = (rbar - rdif);
		r2   = (rbar + rdif);
		if ((r1>=xstart) && (r1<=xend)) roots[(*nroot)++] = (float) r1;
		if ((r2>=xstart) && (r2<=xend)) roots[(*nroot)++] = (float) r2;
		return;
		}

	/* Handle cubic function - classical explicit solution */
	/* See CRC Standard Mathematical Tables, for example */
	else if (order == 3)
		{
		c = f->coeffs[1];
		d = f->coeffs[0] - cval;

		/* Normalize and reduce to the form:   y^3 + p*y + q = 0   */
		/* by the substitution:   x = y - b/3   */
		d   /= a;
		c   /= a;
		b   /= a;
		a    = 1;
		asub = b/3;
		p    = c - b*asub;
		q    = asub * (2*asub*asub - c) + d;

		/* Determine how many roots */
		t1  = p / 3;
		t2  = q / 2;
		det = t1*t1*t1 + t2*t2;

		/* One real root */
		if (det > 0)
			{
			t3 = sqrt(det);
			t1 = -t2 + t3;
			t2 = -t2 - t3;
			t1 = cbrt(t1);
			t2 = cbrt(t2);
			r1 = (t1 + t2 - asub);
			if ((r1>=xstart) && (r1<=xend)) roots[(*nroot)++] = (float) r1;
			return;
			}

		/* Three real roots, at least two equal */
		else if (det == 0)
			{
			t3 = cbrt(t2);
			r1 = (-2*t3 - asub);
			r2 = (t3    - asub);
			r3 = (t3    - asub);

			/* Sort them (remember r2=r3) */
			if (r2 < r1) { rs = r1; r1 = r2; r3 = rs; }
			if ((r1>=xstart) && (r1<=xend)) roots[(*nroot)++] = (float) r1;
			if ((r2>=xstart) && (r2<=xend)) roots[(*nroot)++] = (float) r2;
			if ((r3>=xstart) && (r3<=xend)) roots[(*nroot)++] = (float) r3;
			return;
			}

		/* Three different real roots */
		else
			{
			t3   = sqrt(fabs(t1));
			cang = t2/(t1*t3);
			if (fabs(cang) > 1)
				{
				(void) fprintf(stderr,"DANGER Explicit cubic roots: cos(a) > 1\n");
				(void) fflush(stderr);
				return;
				}
			ang  = acos(cang) / 3;
			t3  *= 2;
			r1   = (t3*cos(ang)         - asub);
			r2   = (t3*cos(ang+a60)     - asub);
			r3   = (t3*cos(ang+a60+a60) - asub);

			/* Sort them */
			if (r2 < r1) { rs = r1; r1 = r2; r2 = rs; }
			if (r3 < r2) { rs = r2; r2 = r3; r3 = rs; }
			if (r2 < r1) { rs = r1; r1 = r2; r2 = rs; }
			if ((r1>=xstart) && (r1<=xend)) roots[(*nroot)++] = (float) r1;
			if ((r2>=xstart) && (r2<=xend)) roots[(*nroot)++] = (float) r2;
			if ((r3>=xstart) && (r3<=xend)) roots[(*nroot)++] = (float) r3;
			return;
			}
		}

	/* Sorry - no more explicit solutions */
	/* Note: an explicit solution for quartic equations is provided in */
	/*       the CRC Standard Mathematical Tables */
	else
		{
		(void) fprintf(stderr,"DANGER No explicit root finder for order %d\n",order);
		(void) fflush(stderr);
		return;
		}
	}

/***********************************************************************
*                                                                      *
*      f i n d _ u n i p o l y _ r a n g e                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Compute value range of the given UNIPOLY, over the given domain.
 *
 *	@param[in] 	*func	given UNIPOLY
 *	@param[in] 	xstart	start of given domain
 *	@param[in] 	xend	end of given domain
 *	@param[out]	*vmin	min of function range
 *	@param[out]	*vmax	max of function range
 *********************************************************************/

void	find_unipoly_range

	(
	UNIPOLY	*func,
	float	xstart,
	float	xend,
	float	*vmin,
	float	*vmax
	)

	{
	UNIPOLY	dfdu;
	float	fstart, fend, fseg, xseg[MAXORD-1];
	int		iseg, nseg;
	float	xmin, xmax;

	/* Return if undefined */
	if (vmin) *vmin = 0;
	if (vmax) *vmax = 0;
	if (!func) return;

	/* Take the derivative of the projected patch function */
	differentiate_unipoly(func, &dfdu);

	/* Find the roots of the patch edge derivative function and */
	/* define monotonic segments between these roots. */
	find_unipoly_roots(&dfdu, 0., xstart, xend, xseg, &nseg);
	fstart = (float) evaluate_unipoly(func, xstart);
	fend   = (float) evaluate_unipoly(func, xend);

	/* Max and min will occur either at an end point or at a root of */
	/* the derivative function */
	xmin = MIN(fstart, fend);
	xmax = MAX(fstart, fend);
	for (iseg=0; iseg<nseg; iseg++)
		{
		fseg = evaluate_unipoly(func, xseg[iseg]);
		xmin = MIN(xmin, fseg);
		xmax = MAX(xmax, fseg);
		}

	if (vmin) *vmin = xmin;
	if (vmax) *vmax = xmax;
	}

/***********************************************************************
*                                                                      *
*      z e r o i n _ u n i p o l y                                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find a root of the given function at @f$ f({x})-{c} = 0@f$ using a
 * hybrid of the secant and bisection methods.  This method
 * converges almost as fast as Newton's method, but has the
 * added advantage that it never leaves the bracketting range
 * and guarantees to converge to the root if there is one!
 * This routine expects that @f$ f({x_1})-{c}@f$  and @f$ f({x_2})-{c}@f$
 * have opposite signs, hence guaranteeing that a root exists.
 *
 *	@param[in] 	*f	given function
 *	@param[in] 	c	contour value
 *	@param[in] 	x1	start of range
 *	@param[in] 	x2	end of range
 *	@param[in] 	tol	tolerance
 *	@return a root of the function in the given range
 *********************************************************************/

double zeroin_unipoly

	(
	UNIPOLY	*f,
	float	c,
	float	x1,
	float	x2,
	float	tol
	)

	{
	FF1		*eval;
	double	*fbuf;
	int		nconv, nsteps;
	float	seglen, delta, segtest;
	float	x0, f0, f1, f2, p, q;

	/* Return 0 if undefined */
	if (!f) return 0;

	/* Set up first interval */
	eval    = eval1d[f->order];
	fbuf    = f->coeffs;
	nconv   = 0;
	segtest = fabs((double) (x2-x1));
	f1 = (float) eval(fbuf,x1) - c;
	f2 = (float) eval(fbuf,x2) - c;
	x0 = x2;
	f0 = f2;

	/* Main iteration loop */
#	ifdef DEBUG_POLY
	(void) printf("    begin ");
#	endif /* DEBUG_POLY */
	for (nsteps=2; nsteps<500; nsteps++)
		{
		/* Interchange x1 and x2 if f(x2) is better */
		if (fabs((double) f2) < fabs((double) f1))
			{
			x0 = x1;
			f0 = f1;
			x1 = x2;
			f1 = f2;
			x2 = x0;
			f2 = f0;
			}

		/* Test stopping criterion */
#		ifdef DEBUG_POLY
		(void) printf("   %f %f   %f\n",x1,x2,f1);
#		endif /* DEBUG_POLY */
		if (f1 == 0) return x1;
		delta  = 0.5*(x2-x1);
		seglen = fabs((double) delta);
		if (seglen <= tol) return x1;

		/* Set up for new iterate as b + p/q (arrange for p>=0) */
		p = (x1-x0)/f1;
		q = f0 - f1;
		if (p < 0)
			{
			p *= -1;
			q *= -1;
			}

		/* Update x0 and check if interval is sufficiently */
		/* reduced over 4 iterations - if not force bisection */
		x0 = x1;
		f0 = f1;
		if (++nconv >= 4)
			{
			if (8.0*seglen >= segtest) q = 0;
			nconv   = 0;
			segtest = seglen;
			}

		/* If change is too small, increment by tolerance */
		if (p <= tol*fabs((double) q))
			{
			x1 += (delta>=0) ? tol : -tol;
#			ifdef DEBUG_POLY
			(void) printf("    incrmt");
#			endif /* DEBUG_POLY */
			}

		/* If root is closer to x1 than x2 we can use secant rule */
		else if (p < delta*q)
			{
			x1 += p/q;
#			ifdef DEBUG_POLY
			(void) printf("    secant");
#			endif /* DEBUG_POLY */
			}

		/* Otherwise use bisection */
		else	{
			x1 = 0.5*(x2+x1);
#			ifdef DEBUG_POLY
			(void) printf("    bisect");
#			endif /* DEBUG_POLY */
			}

		/* We have a new iterate */
		f1 = (float) eval(fbuf,x1) - c;
		if (f1 == 0) return x1;
		if (SIGN(f1) == SIGN(f2))
			{
			x2 = x0;
			f2 = f0;
			}
		}

	/* We have exceeded the maximum number of iterations */
	/* this is only possible if the tolerance is less than */
	/* 1/500 of the interval - return the mid-point */
	return 0.5*(x1+x2);
	}

/***********************************************************************
*                                                                      *
*      i n i t _ b i p o l y                                           *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Initialize bipoly polynomial coefficients.
 *
 *	@param[out]	*f		function to be initialized
 *
 *********************************************************************/

void init_bipoly

	(
	BIPOLY			*f
	)

	{
	int	i, j;

	/* Return if undefined */
	if (!f) return;

	f->xorder = 0;
	f->yorder = 0;
	for (i=0; i<MAXTERM; i++)
		for (j=0; j<MAXTERM; j++)
			f->coeffs[i][j] = 0;
	}

/***********************************************************************
*                                                                      *
*      c o p y _ b i p o l y                                           *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Copy the first  bipoly into the second.
 *
 *	@param[out]	*fnew	function to be copied into
 *	@param[in] 	*f		function to be copied
 *********************************************************************/

void copy_bipoly

	(
	BIPOLY			*fnew,
	const BIPOLY	*f
	)

	{
	int	i, j;

	/* Return if undefined */
	if (!f)    return;
	if (!fnew) return;

	init_bipoly(fnew);

	fnew->xorder = f->xorder;
	fnew->yorder = f->yorder;
	for (i=0; i<=f->xorder; i++)
		for (j=0; j<=f->yorder; j++)
			fnew->coeffs[i][j] = f->coeffs[i][j];
	}

/***********************************************************************
*                                                                      *
*      e v a l u a t e _ b i p o l y                                   *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Evaluate the given bipoly at the specified point.
 * Horner's rule is used.
 *
 *	@param[in] 	*f	function to be evaluated
 *	@param[in] 	p	location at which to evaluate
 * 	@return The value of the function evaluated at the point.
 *********************************************************************/

double evaluate_bipoly

	(
	BIPOLY	*f,
	POINT	p
	)

	{
	/* Return 0 if undefined */
	if (!f) return 0;
	if (!p) return 0;
	if (f->xorder < 0) return 0;
	if (f->yorder < 0) return 0;

	/* Evaluate using explicit Horner's rule */
	return eval2d[f->xorder][f->yorder] (f->coeffs,p[X],p[Y]);
	}

/***********************************************************************
*                                                                      *
*      d i f f e r e n t i a t e _ b i p o l y                         *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Generate the first derivative of the given bipoly f, wrt the
 * specified co-ordinate and return it in dfds.  If dfds and f
 * point to the same place, then the original is correctly
 * overwritten by its derivative.
 *
 *	@param[in] 	*f		given bipoly
 *	@param[out]	*dfds	resulting derivative bipoly
 *	@param[in] 	s		which co-ordinate
 *********************************************************************/

void differentiate_bipoly

	(
	BIPOLY	*f,
	BIPOLY	*dfds,
	char	s
	)

	{
	int	xterm, xorder = -1;
	int	yterm, yorder = -1;

	/* Return if undefined */
	if (!f)    return;
	if (!dfds) return;

	init_bipoly(dfds);

	/* Setup derivative order */
	xorder = f->xorder;
	yorder = f->yorder;
	if (s == 'x')
		{
		dfds->xorder = xorder - 1;
		dfds->yorder = yorder;
		}
	else if (s == 'y')
		{
		dfds->xorder = xorder;
		dfds->yorder = yorder - 1;
		}
	if ((xorder < 1) || (yorder < 1))
		{
		dfds->xorder = 0;
		dfds->yorder = 0;
		dfds->coeffs[0][0] = 0;
		return;
		}

	/* Differentiate each term wrt x or y as requested */
	if (s == 'x')
		{
		for (xterm=1; xterm<=xorder; xterm++)
			for (yterm=0; yterm<=yorder; yterm++)
				dfds->coeffs[xterm-1][yterm]
				= xterm * f->coeffs[xterm][yterm];
		}
	else if (s == 'y')
		{
		for (xterm=0; xterm<=xorder; xterm++)
			for (yterm=1; yterm<=yorder; yterm++)
				dfds->coeffs[xterm][yterm-1]
				= yterm * f->coeffs[xterm][yterm];
		}

	return;
	}

/***********************************************************************
*                                                                      *
*      p r o j e c t _ b i p o l y                                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Project a bivariate polynomial onto a univariate polynomial by
 * fixing one co-ordinate value.
 *
 *	@param[in] 	*f2		given bipoly
 *	@param[out]	*f1		resulting unipoly
 *	@param[in] 	sense	project in 'x' or 'y' direction
 *	@param[in] 	value	value to fix specified co-ordinate at
 *********************************************************************/

void project_bipoly

	(
	BIPOLY	*f2,
	UNIPOLY	*f1,
	char	sense,
	float	value
	)

	{
	int	xorder, yorder, ix, iy;
	double	temp[MAXTERM];

	/* Return if undefined */
	if (!f2) return;
	if (!f1) return;

	init_unipoly(f1);

	xorder = f2->xorder;
	yorder = f2->yorder;

	/* Project in y direction (return function of x) */
	if (sense == 'y')
		{
		f1->order = xorder;
		if (value == 0)
			for (ix=0; ix<=xorder; ix++)
				f1->coeffs[ix] = f2->coeffs[ix][0];

		else	for (ix=0; ix<=xorder; ix++)
				{
				for (iy=0; iy<=yorder; iy++)
					temp[iy] = f2->coeffs[ix][iy];
				f1->coeffs[ix] = eval1d[yorder](temp,value);
				}
		}

	/* Project in x direction (return function of y) */
	else if (sense == 'x')
		{
		f1->order = yorder;
		if (value == 0)
			for (iy=0; iy<=yorder; iy++)
				f1->coeffs[iy] = f2->coeffs[0][iy];

		else	for (iy=0; iy<=yorder; iy++)
				{
				for (ix=0; ix<=xorder; ix++)
					temp[ix] = f2->coeffs[ix][iy];
				f1->coeffs[iy] = eval1d[xorder](temp,value);
				}
		}
	}

/***********************************************************************
*                                                                      *
*      z e r o i n _ b i p o l y                                       *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Bivariate version of zeroin_unipoly, with the added constraint
 * that the root must lie on the arc of a circle of given radius
 * and centre.  This is useful for contour tracking, where it is
 * assumed that the centre of the circle is the previous point
 * found on the contour.
 *
 *	@param[in] 	*f		given function
 *	@param[in] 	c		contour value
 *	@param[in] 	centre	centre of constraining circle
 *	@param[in] 	radius	radius of constraining circle
 *	@param[in] 	as		start angle on circle
 *	@param[in] 	ae		end angle on circle
 *	@param[in] 	tol		tolerance
 *  @return The zeroing angle. Indicates the location on the cicle of
 * 			the root found.
 *********************************************************************/

double zeroin_bipoly

	(
	BIPOLY	*f,
	float	c,
	POINT	centre,
	float	radius,
	float	as,
	float	ae,
	float	tol
	)

	{
	FF2		*eval;
	double	(*fbuf)[MAXTERM];
	int		nconv, nsteps;
	float	seglen, delta, segtest;
	float	xm, ym, fm, am, p, q, xc, yc;
	float	xs, ys, fs;
	float	xe, ye, fe;

	/* Return if 0 undefined */
	if (!f)      return 0;
	if (!centre) return 0;

	/* Set up first interval */
	eval    = eval2d[f->xorder][f->yorder];
	fbuf    = f->coeffs;
	nconv   = 0;
	segtest = fabs((double) (ae-as));
	xc = centre[X];
	yc = centre[Y];
	xs = xc + radius*cos((double) as);
	ys = yc + radius*sin((double) as);
	fs = eval(fbuf,xs,ys) - c;
	xe = xc + radius*cos((double) ae);
	ye = yc + radius*sin((double) ae);
	fe = eval(fbuf,xe,ye) - c;
	am = ae;
	xm = xe;
	ym = ye;
	fm = fe;

	/* Main iteration loop */
	for (nsteps=2; nsteps<500; nsteps++)
		{
		/* Interchange as and ae if f(ae) is better */
		if (fabs((double) fe) < fabs((double) fs))
			{
			am = as;
			xm = xs;
			ym = ys;
			fm = fs;
			as = ae;
			xs = xe;
			ys = ye;
			fs = fe;
			ae = am;
			xe = xm;
			ye = ym;
			fe = fm;
			}

		/* Test stopping criterion */
		if (fs == 0) return as;
		delta  = 0.5*(ae-as);
		seglen = fabs((double) delta);
		if (seglen <= tol) return 0.5*(as+ae);

		/* Set up for new iterate as b + p/q (arrange for p>=0) */
		p = (as-am)/fs;
		q = fm - fs;
		if (p < 0)
			{
			p *= -1;
			q *= -1;
			}

		/* Update am and check if interval is sufficiently */
		/* reduced over 4 iterations - if not force bisection */
		am = as;
		xm = xs;
		ym = ys;
		fm = fs;
		if (++nconv >= 4)
			{
			if (8.0*seglen >= segtest) q = 0;
			nconv   = 0;
			segtest = seglen;
			}

		/* If change is too small, increment by tolerance */
		if (p <= tol*fabs((double) q)) as += (delta>=0) ? tol : -tol;

		/* If root is closer to xs than xe we can use secant rule */
		else if (p < delta*q) as += p/q;

		/* Otherwise use bisection */
		else as = 0.5*(ae+as);

		/* We have a new iterate */
		xs = xc + radius*cos((double) as);
		ys = yc + radius*sin((double) as);
		fs = eval(fbuf,xs,ys) - c;
		if (fs == 0) return as;
		if (fs*fe > 0)
			{
			ae = am;
			xe = xm;
			ye = ym;
			fe = fm;
			}
		}

	/* We have exceeded the maximum number of iterations */
	/* this is only possible if the tolerance is less than */
	/* 1/500 of the interval - return the mid-point */
	return 0.5*(as+ae);
	}

/***********************************************************************
*                                                                      *
*      e v a l 1 d                                                     *
*                                                                      *
*      Explicitly expanded Horner's rule for univariate polymnomials.  *
*                                                                      *
***********************************************************************/

/*ARGSUSED*/
static double eval_0(double f[], float u)
{ return (f[0]); }

static double eval_1(double f[], float u)
{ return ((f[1])*u+f[0]); }

static double eval_2(double f[], float u)
{ return (((f[2])*u+f[1])*u+f[0]); }

static double eval_3(double f[], float u)
{ return ((((f[3])*u+f[2])*u+f[1])*u+f[0]); }

/***********************************************************************
*                                                                      *
*      e v a l 2 d                                                     *
*                                                                      *
*      Explicitly expanded Horner's rule for bivariate polymnomials.   *
*                                                                      *
***********************************************************************/

/*ARGSUSED*/
static double eval_0_0(double f[][MAXTERM], float u, float v)
{ return ((f[0][0])); }

/*ARGSUSED*/
static double eval_0_1(double f[][MAXTERM], float u, float v)
{ return (((f[0][1])*v+f[0][0])); }

/*ARGSUSED*/
static double eval_0_2(double f[][MAXTERM], float u, float v)
{ return ((((f[0][2])*v+f[0][1])*v+f[0][0])); }

/*ARGSUSED*/
static double eval_0_3(double f[][MAXTERM], float u, float v)
{ return (((((f[0][3])*v+f[0][2])*v+f[0][1])*v+f[0][0])); }

/*ARGSUSED*/
static double eval_1_0(double f[][MAXTERM], float u, float v)
{ return (((f[1][0]))*u+(f[0][0])); }

static double eval_1_1(double f[][MAXTERM], float u, float v)
{ return ((((f[1][1])*v+f[1][0]))*u
	  +((f[0][1])*u+f[0][0])); }

static double eval_1_2(double f[][MAXTERM], float u, float v)
{ return (((((f[1][2])*v+f[1][1])*v+f[1][0]))*u
	  +(((f[0][2])*v+f[0][1])*v+f[0][0])); }

static double eval_1_3(double f[][MAXTERM], float u, float v)
{ return ((((((f[1][3])*v+f[1][2])*v+f[1][1])*v+f[1][0]))*u
	  +((((f[0][3])*v+f[0][2])*v+f[0][1])*v+f[0][0])); }

/*ARGSUSED*/
static double eval_2_0(double f[][MAXTERM], float u, float v)
{ return ((((f[2][0]))*u+(f[1][0]))*u+(f[0][0])); }

static double eval_2_1(double f[][MAXTERM], float u, float v)
{ return (((((f[2][1])*v+f[2][0]))*u
	   +((f[1][1])*v+f[1][0]))*u
	   +((f[0][1])*v+f[0][0])); }

static double eval_2_2(double f[][MAXTERM], float u, float v)
{ return ((((((f[2][2])*v+f[2][1])*v+f[2][0]))*u
	   +(((f[1][2])*v+f[1][1])*v+f[1][0]))*u
	   +(((f[0][2])*v+f[0][1])*v+f[0][0])); }

static double eval_2_3(double f[][MAXTERM], float u, float v)
{ return (((((((f[2][3])*v+f[2][2])*v+f[2][1])*v+f[2][0]))*u
	   +((((f[1][3])*v+f[1][2])*v+f[1][1])*v+f[1][0]))*u
	   +((((f[0][3])*v+f[0][2])*v+f[0][1])*v+f[0][0])); }

/*ARGSUSED*/
static double eval_3_0(double f[][MAXTERM], float u, float v)
{ return (((((f[3][0]))*u+(f[2][0]))*u+(f[1][0]))*u+(f[0][0])); }

static double eval_3_1(double f[][MAXTERM], float u, float v)
{ return ((((((f[3][1])*v+f[3][0]))*u
		+((f[2][1])*v+f[2][0]))*u
		+((f[1][1])*v+f[1][0]))*u
		+((f[0][1])*v+f[0][0])); }

static double eval_3_2(double f[][MAXTERM], float u, float v)
{ return (((((((f[3][2])*v+f[3][1])*v+f[3][0]))*u
		+(((f[2][2])*v+f[2][1])*v+f[2][0]))*u
		+(((f[1][2])*v+f[1][1])*v+f[1][0]))*u
		+(((f[0][2])*v+f[0][1])*v+f[0][0])); }

static double eval_3_3(double f[][MAXTERM], float u, float v)
{ return ((((((((f[3][3])*v+f[3][2])*v+f[3][1])*v+f[3][0]))*u
		+((((f[2][3])*v+f[2][2])*v+f[2][1])*v+f[2][0]))*u
		+((((f[1][3])*v+f[1][2])*v+f[1][1])*v+f[1][0]))*u
		+((((f[0][3])*v+f[0][2])*v+f[0][1])*v+f[0][0])); }
