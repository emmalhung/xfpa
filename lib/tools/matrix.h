/**********************************************************************/
/** @file matrix.h
 *
 *  Routines to manipulate matrices (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*    m a t r i x . h                                                   *
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

#include <fpa_types.h>

LOGICAL	qsolve_matrix(double **Matrix, double *Vector, int nrow);
LOGICAL	qsolve_matrix_2D(double **Matrix, double *VecU, double *VecV,
				double *VecS, int nrow);
LOGICAL	solve_matrix(double **Matrix, double *Vector, int nrow,
				int swap, int zero);
LOGICAL	lsq_matrix(double **Matrix, double *Vector, int nrow, int ncol);
void	print_matrix_eq(double **Matrix, double *Vector, int nrow, int ncol);
