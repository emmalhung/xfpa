/*********************************************************************/
/** @file matrix.c
 *
 * Contains routine to solve a conventional, fully determined set
 * of equations:
 *
 * [A] * [S] = [V].
 *
 * Where the number of items to be solved for in [S] equals the
 * number of right hand side values in [V].
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    m a t r i x . c                                                   *
*                                                                      *
*    Contains routine to solve an conventional, fully determined set   *
*    of equations:                                                     *
*                                                                      *
*        [A]*[S] = [V]                                                 *
*                                                                      *
*    where the number of items to be solved for in [S] equals the      *
*    number of right hand side values in [V].                          *
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

#include "matrix.h"

#include <fpa_math.h>
#include <fpa_types.h>
#include <fpa_getmem.h>

#include <stdio.h>

/***********************************************************************
*                                                                      *
*    q s o l v e _ m a t r i x                                         *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Solve a fully-determined set of equations.
 *
 * @f[ [A]*[S] = [V] @f]
 *
 * by classical Gaussian elimination.
 *
 * This is the most simplistic and efficient algorithm, intended
 * only for fully-determined, diagonally dominant systems, which
 * guarantee a solution.
 *
 * The solution is returned in the original right-hand-side vector
 * [V], and the original matrix [A] is reduced to the identity
 * matrix.
 *
 *	@param[in]	**Matrix	Matrix
 *	@param[in]	*Vector		RHS vector in, solution vector out
 *	@param[in]	nrow		number of rows and columns
 * 	@return TRUE if successful.
 *********************************************************************/

LOGICAL	qsolve_matrix

	(
	double	**Matrix,
	double	*Vector,
	int		nrow
	)

	{
	int		pivot, row, col;
	double	A;

	/* Error return for missing parameters */
	if (!Matrix) return FALSE;
	if (!Vector) return FALSE;

	/* Forward elimination */
	for (pivot=0; pivot<nrow; pivot++)
		{
		/* Scale pivot row */
		A = Matrix[pivot][pivot];
		if (A == 0) return FALSE;
		Matrix[pivot][pivot] = 1;
		Vector[pivot] /= A;
		for (col=pivot+1; col<nrow; col++)
		Matrix[pivot][col] /= A;

		/* Eliminate elements from column beneath */
		for (row=pivot+1; row<nrow; row++)
			{
			A = Matrix[row][pivot];
			if (A == 0) continue;
			Matrix[row][pivot] = 0;
			Vector[row] -= A * Vector[pivot];
			for (col=pivot+1; col<nrow; col++)
				Matrix[row][col] -= A * Matrix[pivot][col];
			}
		}

	/* Back substitution */
	for (row=nrow-2; row>=0; row--)
		for (pivot=row+1; pivot<nrow; pivot++)
		{
		A = Matrix[row][pivot];
		if (A == 0) continue;
		Matrix[row][pivot] = 0;
		Vector[row] -= A * Vector[pivot];
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    q s o l v e _ m a t r i x _ 2 D                                   *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Solve a fully-determined set of equations with 2d RHS.
 *
 * @f[ [A]*[S] = [U|V|M] @f]
 *
 * by classical Gaussian elimination.
 *
 * This is the most simplistic and efficient algorithm, intended
 * only for fully-determined, diagonally dominant systems, which
 * guarantee a solution.
 *
 * The solution is returned in the original right-hand-side vectors
 * [U|V|M], and the original matrix [A] is reduced to the identity
 * matrix.
 *
 *	@param[in]	**Matrix	Matrix
 *	@param[in]	*VecU		U-component RHS vector in, solution vector out
 *	@param[in]	*VecV		U-component RHS vector in, solution vector out
 *	@param[in]	*VecS		Magnitude RHS vector in, solution vector out
 *	@param[in]	nrow		number of rows and columns
 * 	@return TRUE if successful.
 *********************************************************************/

LOGICAL	qsolve_matrix_2D

	(
	double	**Matrix,
	double	*VecU,
	double	*VecV,
	double	*VecS,
	int		nrow
	)

	{
	int		pivot, row, col;
	double	A;

	/* Error return for missing parameters */
	if (!Matrix) return FALSE;
	if (!VecU)   return FALSE;
	if (!VecV)   return FALSE;
	/* VecS is allowed to be missing */

	/* Forward elimination */
	for (pivot=0; pivot<nrow; pivot++)
		{
		/* Scale pivot row */
		A = Matrix[pivot][pivot];
		if (A == 0) return FALSE;
		Matrix[pivot][pivot] = 1;
		VecU[pivot] /= A;
		VecV[pivot] /= A;
		if (VecS) VecS[pivot] /= A;
		for (col=pivot+1; col<nrow; col++)
		Matrix[pivot][col] /= A;

		/* Eliminate elements from column beneath */
		for (row=pivot+1; row<nrow; row++)
			{
			A = Matrix[row][pivot];
			if (A == 0) continue;
			Matrix[row][pivot] = 0;
			VecU[row] -= A * VecU[pivot];
			VecV[row] -= A * VecV[pivot];
			if (VecS) VecS[row] -= A * VecS[pivot];
			for (col=pivot+1; col<nrow; col++)
				Matrix[row][col] -= A * Matrix[pivot][col];
			}
		}

	/* Back substitution */
	for (row=nrow-2; row>=0; row--)
		for (pivot=row+1; pivot<nrow; pivot++)
		{
		A = Matrix[row][pivot];
		if (A == 0) continue;
		Matrix[row][pivot] = 0;
		VecU[row] -= A * VecU[pivot];
		VecV[row] -= A * VecV[pivot];
		if (VecS) VecS[row] -= A * VecS[pivot];
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    s o l v e _ m a t r i x                                           *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Solve a fully-determined set of equations:
 *
 * @f[ [A]*[S] = [V] @f]
 *
 * by Gauss-Jordan elimination, with row swapping.
 *
 * This is a more general algorithm, intended for fully-determined
 * systems that are not necessarily diagonally dominant.  Swapping
 * of rows is done to minimize numerical errors and guarantee a
 * solution as long as the system is not singular.
 *
 * The solution is returned in the original right-hand-side vector
 * [V], and the original matrix [A] is reduced to the identity
 * matrix.
 *
 * The solution method can be altered in some ways by setting the
 * following switches:
 *
 * swap =
 * 	- 0 (FALSE) . . . no row swapping (like qsolve_matrix)
 *	- 1 (TRUE)  . . . normal row swapping (by copying)
 *	- >1 . . . . . .  more efficient pointer row swapping
 *                    only possible if the matrix was
 *                    allocated as an array of pointers.
 *
 * zero =
 *	- 0 (FALSE) . . . give up on a singular row (all zero)
 *	- 1 (TRUE)  . . . continue even if a singular row is
 *                    encountered, as long as no other
 *                    variables depend on the un-determined
 *                    one (i.e. zero column).
 *
 *	@param[in]	**Matrix	Matrix
 *	@param[in]	*Vector		RHS vector in, solution vector out
 *	@param[in]	nrow		number of rows and columns
 *	@param[in]	swap		should we use row swapping
 *	@param[in]	zero		should we try to continue with a singularity
 * 	@return TRUE if successful.
 *********************************************************************/

LOGICAL	solve_matrix

	(
	double	**Matrix,
	double	*Vector,
	int		nrow,
	int		swap,
	int		zero
	)

	{
	int		pivot, row, col, rbest, rzero;
	double	A, *P, rmax, norm, nbest;

	/* Error return for missing parameters */
	if (!Matrix) return FALSE;
	if (!Vector) return FALSE;

	/* Forward elimination */
	(void) printf("\n[solve_matrix] %d*%d\n", nrow, nrow);
	(void) fflush(stdout);
	for (pivot=0; pivot<nrow; pivot++)
		{
		print_matrix_eq(Matrix, Vector, nrow, nrow);
		(void) printf("    Pivot = %d\n", pivot);

		if (swap)
			{
			/* Find best row to use as pivot row */
			rzero = -1;
			rbest = pivot;
			nbest = 0;
			for (row=pivot; row<nrow; row++)
				{
				/* Find largest element in row */
				rmax = 0;
				for (col=pivot; col<nrow; col++)
					rmax = MAX(rmax, fabs(Matrix[row][col]));

				/* Non-singular row - compare against best so far */
				if (rmax > 0)
					{
					norm = fabs(Matrix[row][pivot])/rmax;
					(void) printf("    Row %d - Norm %e\n", row, norm);
					if (norm > nbest)
						{
						rbest = row;
						nbest = norm;
						}
					}

				/* Singular row - give up if not tolerated */
				else
					{
					(void) printf("    Row %d - All zero\n", row);
					if (!zero) return FALSE;
					rzero = row;
					}
				}

			/* If no pivot row can be found (column beneath is all zero) */
			/* see if this represents a removable singularity */
			if (nbest <= 0)
				{
				/* If no singularities tolerated - give up */
				(void) printf("    Column %d - zero below\n", pivot);
				if (!zero) return FALSE;

				/* See if column above current pivot is also all zero */
				for (row=0; row<pivot; row++)
					if (Matrix[row][pivot] != 0) return FALSE;

				/* Find best row to remove (elements must be represented */
				/* in other rows below) and zero it */
				(void) printf("    Column %d - Removable singularity\n", pivot);
				rbest = MAX(rzero, pivot);	/* could do better */
				Vector[rbest] = 0;
				for (col=pivot+1; col<nrow; col++)
					Matrix[rbest][col] = 0;
				}

			/* Swap best row with current pivot row */
			if (rbest > pivot)
				{
				(void) printf("    Swapping row %d\n", rbest);
				A             = Vector[rbest];
				Vector[rbest] = Vector[pivot];
				Vector[pivot] = A;
				if (swap > 1)
					{
					P             = Matrix[rbest];
					Matrix[rbest] = Matrix[pivot];
					Matrix[pivot] = P;
					}
				else
					{
					for (col=pivot; col<nrow; col++)
						{
						A                  = Matrix[rbest][col];
						Matrix[rbest][col] = Matrix[pivot][col];
						Matrix[pivot][col] = A;
						}
					}
				}

			/* If removable singularity omit forward elimination */
			if (nbest <= 0) continue;
			}

		/* Scale pivot row */
		A = Matrix[pivot][pivot];
		if (A == 0) return FALSE;
		Matrix[pivot][pivot] = 1;
		Vector[pivot] /= A;
		for (col=pivot+1; col<nrow; col++)
		Matrix[pivot][col] /= A;

		/* Eliminate elements from column beneath */
		for (row=pivot+1; row<nrow; row++)
			{
			A = Matrix[row][pivot];
			if (A == 0) continue;
			Matrix[row][pivot] = 0;
			Vector[row] -= A * Vector[pivot];
			for (col=pivot+1; col<nrow; col++)
				Matrix[row][col] -= A * Matrix[pivot][col];
			}
		}

	/* Back substitution */
	for (row=nrow-2; row>=0; row--)
		{
		print_matrix_eq(Matrix, Vector, nrow, nrow);
		(void) printf("    Back Substitute = %d\n", row);

		/* Non-singular row - eliminate remainder of row as usual */
		if (Matrix[row][row] != 0)
			{
			for (pivot=row+1; pivot<nrow; pivot++)
				{
				A = Matrix[row][pivot];
				if (A == 0) continue;
				Matrix[row][pivot] = 0;
				Vector[row] -= A * Vector[pivot];
				}
			}
		}

	print_matrix_eq(Matrix, Vector, nrow, nrow);
	(void) printf("    Solved\n");
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    l s q _ m a t r i x                                               *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Solve a set of equations.
 *
 *    @f[ [A]*[S] = [V] @f]
 *
 *    which may be locally under- or over-determined, by applying the
 *    least-squares constraint.  This is equivalent to pre-multiplying
 *    the system by the transpose of the matrix, as in:
 *
 *    @f[ [A]^T*[A]*[S] = [A]^T*[V] @f]
 *
 *    The routine "solve_matrix" above is used to perform the actual
 *    solution.
 *
 *    The solution is returned in the original right-hand-side vector
 *    [V], and the original matrix [A] is untouched.
 *
 *	@param[in]	**Matrix	Matrix
 *	@param[in]	*Vector		RHS vector in, solution vector out
 *	@param[in]	nrow		number of rows
 *	@param[in]	ncol		number of columns
 *	@return True if successful
 *********************************************************************/

LOGICAL	lsq_matrix

	(
	double	**Matrix,
	double	*Vector,
	int		nrow,
	int		ncol
	)

	{
	double	*Mbuf, **Mnew, *Vnew, sum;
	int		row, col, term;

	/* Error return for missing parameters */
	if (!Matrix) return FALSE;
	if (!Vector) return FALSE;

	/* Allocate room for pre-multiplied matrix (ncol*ncol) and vector */
	Mbuf = INITMEM(double, ncol*ncol);
	Mnew = INITMEM(double *, ncol);
	Vnew = INITMEM(double, ncol);
	for (row=0; row<ncol; row++)
		Mnew[row] = Mbuf + row*ncol;

	/* Pre-multiply system into new matrix and vector */
	(void) printf("\nPre-Multiplying Equation: %d*%d\n", ncol, ncol);
	for (row=0; row<ncol; row++)
		{
		for (col=0; col<ncol; col++)
			{
			sum = 0;
			for (term=0; term<nrow; term++)
				sum += Matrix[term][row] * Matrix[term][col];
			Mnew[row][col] = sum;
			(void) printf(" %9.2e", Mnew[row][col]);
			}
		sum = 0;
		for (term=0; term<nrow; term++)
			sum += Matrix[term][row] * Vector[term];
		Vnew[row] = sum;
		(void) printf(" ----- %9.2e\n", Vnew[row]);
		(void) fflush(stdout);
		}

	/* Solve the transformed system */
	if ( !solve_matrix(Mnew, Vnew, ncol, 2, 1) ) return FALSE;

	/* Copy the solution into the vector */
	for (row=0; row<ncol; row++)
		Vector[row] = Vnew[row];
	for (row=ncol; row<nrow; row++)
		Vector[row] = 0;

	/* Free the temporary matrix and vector */
	FREEMEM(Mbuf);
	FREEMEM(Mnew);
	FREEMEM(Vnew);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     p r i n t _ m a r t i x _ e q                                    *
*                                                                      *
***********************************************************************/

void	print_matrix_eq

	(
	double	**Matrix	/* Matrix */ ,
	double	*Vector		/* RHS vector in, solution vector out */ ,
	int		nrow		/* number of rows */ ,
	int		ncol		/* number of columns */
	)

	{
	int		row, col;

	/* Error return for missing parameters */
	if (!Matrix) return;
	if (!Vector) return;

	(void) printf("\nMatrix Equation: %d*%d\n", nrow, ncol);
	(void) fflush(stdout);
	for (row=0; row<nrow; row++)
		{
		for (col=0; col<ncol; col++)
			(void) printf(" %9.2e", Matrix[row][col]);
		(void) printf(" ----- %9.2e\n", Vector[row]);
		(void) fflush(stdout);
		}
	}
