/*********************************************************************/
/** @file under.c
 *
 * Contains routines to solve an under-determined set of equations:
 *
 * [A]*[S] = [V]
 *
 * where the number of items to be solved for in [S] exceeds
 * (or equals) the number of right hand side values in [V].
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    u n d e r s o l v e . c                                           *
*                                                                      *
*    Contains routine to solve an under-determined set of equations:   *
*                                                                      *
*        [A]*[S] = [V]                                                 *
*                                                                      *
*    where the number of items to be solved for in [S] exceeds         *
*    (or equals) the number of right hand side values in [V].          *
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

#include "under.h"

#include <fpa_math.h>
#include <fpa_types.h>
#include <fpa_getmem.h>

/* do the LQ factorization */
static	int	factor(int, int, double **, double **);

/* back-substitute the triangular [L] */
static	void	backsolve(int, int, double **, double **, double **);

/* back-substitute the whole system */
static	void	backtrans(int, int, int, double **, double **);

static	double	*diag;		/* vector of diagonals from [L] */
static	double	*scal;		/* vector of scale factors from [Q] */
static	double	*norm;		/* vector of row norms */

/***********************************************************************
*                                                                      *
*    u n d e r s o l v e                                               *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Solve an under-determined set of equations.
 *
 *        @f[ [A]*[S] = [V] @f]
 *
 * by the technique known as L-Q decomposition.  See the routines
 * 'factor', 'backsolve' and 'backtrans' for futher details.
 *
 *	@param[in]	rows		number of values
 *	@param[in]	cols		number of solutions
 *	@param[in]	coords		number of right hand side components
 *	@param[in]	**matrix	rows*cols matrix to be inverted
 *	@param[out]	**solution	cols*coords vector of solution multiplets
 *	@param[out]	**values	rows*coords vector of right hand side multiplets
 *********************************************************************/

LOGICAL	undersolve

	(
	int		rows,
	int		cols,
	int		coords,
	double	**matrix,
	double	**solution,
	double	**values
	)

	{
	int		rank;	/* actual rank of matrix (<= rows) */
	register int	icol, coord;

	/* Check input */
	if (rows <= 0)   return FALSE;
	if (cols <= 0)   return FALSE;
	if (coords <= 0) return FALSE;
	if (!matrix || !solution || !values) return FALSE;

	/* Allocate space for diag, scal and norm vectors */
	diag = INITMEM(double,rows);
	scal = INITMEM(double,rows);
	norm = INITMEM(double,rows);

	/* Zero the solution vector */
	for (icol=cols-1; icol>=0; icol--)
		for (coord=0; coord<coords; coord++)
		solution[icol][coord] = 0.0;

	/* Factor and solve the system */
	rank = factor(rows,cols,matrix,values);
	(void) backsolve(rank,coords,matrix,solution,values);
	(void) backtrans(rows,cols,coords,matrix,solution);

	/* Free space for diag, scal and norm vectors */
	FREEMEM(diag);
	FREEMEM(scal);
	FREEMEM(norm);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    f a c t o r                                                       *
*                                                                      *
*    Factor the matrix from the under-determined system, by L-Q        *
*    decomposition, into the form:                                     *
*                                                                      *
*        [A] = [L|0]*[Q]                                               *
*                                                                      *
*    where [L] is lower triangular, and [Q] is orthogonal.  The        *
*    diagonal elements of [L] are stored in the vector [diag].         *
*    [Q] is implicitly represented as a product of Householder         *
*    transformations, of the form:                                     *
*                                                                      *
*        [H] = [I] + scal*[ROW]*[ROW_Transpose]                        *
*                                                                      *
*    for the i'th matrix row.  The scale factors are stored in the     *
*    vector [scal].                                                    *
*                                                                      *
*    The actual rank of the system is returned, as this can be less    *
*    than the number of rows.                                          *
*                                                                      *
***********************************************************************/

static	int	factor

	(
	int		rows		/* number of values */ ,
	int		cols		/* number of solutions */ ,
	double	**matrix	/* rows*cols matrix to be inverted */ ,
	double	**values	/* rows*(*) vector of right hand side multiplets */
	)

	{
	int		rank;	/* actual rank of matrix (<= rows) returned */
	int		imax, renorm;
	register int	irow, icol, ipiv;
	double		maxnorm, test, ro, sum, temp, *temp_ptr;

	if (!matrix || !values) return 0;

	/* Main loop (over rows) */
	renorm = TRUE;
	for (ipiv=0; ipiv<rows; ipiv++)
		{
		/* Compute the squared row norms and find the largest */
		if (renorm)
		{
		imax = ipiv;
		for (irow=ipiv; irow<rows; irow++)
			{
			sum = 0.0;
			for (icol=0; icol<cols; icol++)
			sum += matrix[irow][icol] * matrix[irow][icol];
			norm[irow] = sum;
			if (sum > norm[imax]) imax = irow;
			}
		maxnorm = norm[imax];
		if (ipiv == 0) test = 0.01 * maxnorm;
		}

		/* Interchange current row with the row that has the largest norm */
		if (imax > ipiv)
		{
		temp         = norm[ipiv];	/* Exchange row norms */
		norm[ipiv]   = norm[imax];
		norm[imax]   = temp;

		temp_ptr     = matrix[ipiv];	/* Exchange matrix rows */
		matrix[ipiv] = matrix[imax];
		matrix[imax] = temp_ptr;

		temp_ptr     = values[ipiv];	/* Exchange value rows */
		values[ipiv] = values[imax];
		values[imax] = temp_ptr;
		}

		/* Generate the ipiv'th Householder transformation */
		diag[ipiv] = 0.0;
		scal[ipiv] = 1.0;
		ro         = -1.0;
		for (icol=ipiv; icol<cols; icol++)
		ro = MAX(ro,fabs(matrix[ipiv][icol]));
		if (ro > 0.0)
		{
		sum = 0.0;
		for (icol=ipiv; icol<cols; icol++)
			sum += matrix[ipiv][icol] * matrix[ipiv][icol];
		ro = -SIGN(matrix[ipiv][ipiv]) * sum / ro;
		matrix[ipiv][ipiv] -= ro;
		diag[ipiv]          = ro;
		scal[ipiv]          = 1.0 / (ro*matrix[ipiv][ipiv]);
		}

		/* Apply the ipiv'th Householder transformation */
		for (irow=ipiv+1; irow<rows; irow++)
		{
		sum = 0.0;
		for (icol=ipiv; icol<cols; icol++)
			sum += matrix[ipiv][icol] * matrix[irow][icol];
		if (sum == 0.0) continue;
		for (icol=ipiv; icol<cols; icol++)
			matrix[irow][icol] += sum * matrix[ipiv][icol];
		}

		/* Update squared row norms and see if this is good enough */
		imax = ipiv + 1;
		for (irow=ipiv+1; irow<rows; irow++)
		{
		norm[irow] -= matrix[irow][ipiv] * matrix[irow][ipiv];
		if (norm[irow] > norm[imax]) imax = irow;
		}
		renorm = (int) (10.0*norm[imax] < maxnorm);

		}	/* End of main loop */

	/* Determine the numerical rank */
	rank = 0;
	for (irow=0; irow<rows; irow++)
		{
		if (fabs(diag[irow]) < test) break;
		rank++;
		}

	return rank;
	}

/***********************************************************************
*                                                                      *
*    b a c k s o l v e                                                 *
*                                                                      *
*    Back substitute the under-determined system that has been         *
*    factored by L-Q decomposition.  The result is to orthogonalize    *
*    (diagonalize) the lower triangular sub-matrix [L].  This          *
*    produces an intermediate solution vector which is equivalent to:  *
*                                                                      *
*        [S'] = [Q]*[S]                                                *
*                                                                      *
*    These intermediate solutions are stored temporarily in the        *
*    solution vector [S], since is is not being used yet.  Hence, the  *
*    solution vector should be zeroed before entering this routine.    *
*                                                                      *
***********************************************************************/

static	void	backsolve

	(
	int		rank		/* actual rank of matrix (<= rows) */ ,
	int		coords		/* number of right hand side components */ ,
	double	**matrix	/* rows*cols matrix to be inverted */ ,
	double	**solution	/* cols*coords vector of solution multiplets */ ,
	double	**values	/* rows*coords vector of right hand side multiplets */
	)

	{
	register int	irow, icol, coord;
	double		sum;

	if (!matrix || !solution || !values) return;

	for (irow=0; irow<rank; irow++)
		for (coord=0; coord<coords; coord++)
		{
		sum = values[irow][coord];
		for (icol=0; icol<irow; icol++)
			sum -= matrix[irow][icol] * solution[icol][coord];
		solution[irow][coord] = sum / diag[irow];
		}
	}

/***********************************************************************
*                                                                      *
*    b a c k t r a n s                                                 *
*                                                                      *
*    All that remains is to solve the intermediate system:             *
*                                                                      *
*        [Q]*[S] = [S']                                                *
*                                                                      *
*    This necessitates regenerating the matrix [Q] implicitly from     *
*    its component Householder transformations.                        *
*                                                                      *
***********************************************************************/

static	void	backtrans

	(
	int		rows		/* number of values */ ,
	int		cols		/* number of solutions */ ,
	int		coords		/* number of right hand side components */ ,
	double	**matrix	/* rows*cols matrix to be inverted */ ,
	double	**solution	/* cols*coords vector of solution multiplets */
	)

	{
	register int	irow, icol, coord;
	double		sum;

	if (!matrix || !solution) return;

	for (irow=rows-1; irow>=0; irow--)
		for (coord=0; coord<coords; coord++)
		{
		sum = 0.0;
		for (icol=irow; icol<cols; icol++)
			sum += matrix[irow][icol] * solution[icol][coord];
		if (sum == 0.0) continue;
		sum *= scal[irow];
		for (icol=irow; icol<cols; icol++)
			solution[icol][coord] += sum * matrix[irow][icol];
		}
	}
