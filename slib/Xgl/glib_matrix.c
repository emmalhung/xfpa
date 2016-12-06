/***********************************************************************/
/*
*	File: glib_matrix.c
*
*   Purpose: Contains all functions related to transformation matrix
*            manipulation.
*
*   Notes:   This library only uses that part of the normal 4x4 graphic
*            display matrix that applies to 2-dimensional images and
*            thus reduces to a 3x2 matrix.
*
*     Version 8 (c) Copyright 2011 Environment Canada
*
*   This file is part of the Forecast Production Assistant (FPA).
*   The FPA is free software: you can redistribute it and/or modify it
*   under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   any later version.
*
*   The FPA is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*   See the GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.
*/
/***********************************************************************/

#include <fpa_math.h>
#include <tools/tools.h>
#include "glib_private.h"


/*================ STATIC LOCAL FUNCTIONS ===========================*/


/* Performs a matrix multiplication z = x*y on a 3x2 matrix. Note that
 * this restricts us to 2D manipulation, but for this library this is
 * just fine.
*/
static void mult(Matrix x, Matrix y, Matrix z)
{
	Matrix a;

	a[0][0] = x[0][0]*y[0][0] + x[0][1]*y[1][0];
	a[0][1] = x[0][0]*y[0][1] + x[0][1]*y[1][1];
	a[1][0] = x[1][0]*y[0][0] + x[1][1]*y[1][0];
	a[1][1] = x[1][0]*y[0][1] + x[1][1]*y[1][1];
	a[2][0] = x[2][0]*y[0][0] + x[2][1]*y[1][0] + y[2][0];
	a[2][1] = x[2][0]*y[0][1] + x[2][1]*y[1][1] + y[2][1];

	glCopyMatrix(z, a);
}



/*================ PUBLIC FUNCTIONS ===========================*/


/* Get the matrix at the top of the stack. If the stack is empty
 * return the unit matrix.
*/
void glGetMatrix(Matrix m)
{
	if(W->send < 1)
		glIdentityMatrix(m);
	else
		glCopyMatrix(m, W->stack[0].matrix);
}



void glCopyMatrix(Matrix a, Matrix b)
{
	a[0][0] = b[0][0];
	a[0][1] = b[0][1];
	a[1][0] = b[1][0];
	a[1][1] = b[1][1];
	a[2][0] = b[2][0];
	a[2][1] = b[2][1];
}



/* Push down the matrix stack and make the given matrix the new top-of-stack
 * entry. Reset the rotation, translation and scale factors.
*
*  Note: We save Sx, Sy and ra as separate items so that we do not have to
*        recalculate them on popup as they are computationaly intensive.
*/
void glPushMatrix(Matrix m)
{
	int n;
	float  x, dm[3][2];

	if(W->send >= W->slen)
	{
		W->slen += 3;
		W->stack = GETMEM(W->stack, MATRIXSTACK, W->slen);
	}

	for(n = W->send; n > 0; n--)
	{
		glCopyMatrix(W->stack[n].matrix, W->stack[n-1].matrix);
		W->stack[n].Sx = W->stack[n-1].Sx;
		W->stack[n].Sy = W->stack[n-1].Sy;
		W->stack[n].ra = W->stack[n-1].ra;
	}
	W->send++;

	/* Set the scaling, rotation and translation factors  Note that
	 * Tx and Ty have 0.5 added to them so that when the coordinate
	 * transformation is converted to int (truncated) that the
	 * rounding will be done properly.
	 */
	dm[0][0] = m[0][0];
	dm[0][1] = m[0][1];
	dm[1][0] = m[1][0];
	dm[1][1] = m[1][1];
	dm[2][0] = m[2][0];
	dm[2][1] = m[2][1];
	
	W->Sx    = SQRT(dm[0][0]*dm[0][0] + dm[1][0]*dm[1][0]);
	W->Sy    = SQRT(dm[0][1]*dm[0][1] + dm[1][1]*dm[1][1]);
	W->RxCos = dm[0][0];
	W->RySin = dm[0][1];
	W->RxSin = dm[1][0];
	W->RyCos = dm[1][1];
	W->Tx    = dm[2][0] + 0.5;
	W->Ty    = dm[2][1] + 0.5;
	W->ra    = 0.0;
	/*
	 * For the angle calculation check the limits, for if x == y 
	 * the result of x/y can be greater than 1.0 due to roundoff.
	 */
	x = dm[0][0]/W->Sx;
	if( x < 1.0 && x > -1.0 )
	{
		W->ra = ACOS(x) * 180.0/M_PI;
		if(dm[0][1]/W->Sy < 0.0) W->ra = 360.0 - W->ra;
	}

	/* Copy into the stack */
	glCopyMatrix(W->stack[0].matrix, m);
	W->stack[0].Sx = W->Sx;
	W->stack[0].Sy = W->Sy;
	W->stack[0].ra = W->ra;
}


/* Pop the matrix stack and reset the rotation, translation and scale factors.
*/
void glPopMatrix(void)
{
	if(W->send < 1)
	{
		pr_error("glPopMatrix", "Attempt to pop an empty matrix stack.\n");
	}
	else
	{
		int n;
		W->send--;
		for(n = 0; n < W->send; n++)
		{
			glCopyMatrix(W->stack[n].matrix, W->stack[n+1].matrix);
			W->stack[n].Sx = W->stack[n+1].Sx;
			W->stack[n].Sy = W->stack[n+1].Sy;
			W->stack[n].ra = W->stack[n+1].ra;
		}
		W->RxCos = W->stack[0].matrix[0][0];
		W->RySin = W->stack[0].matrix[0][1];
		W->RxSin = W->stack[0].matrix[1][0];
		W->RyCos = W->stack[0].matrix[1][1];
		W->Tx    = W->stack[0].matrix[2][0] + 0.5;
		W->Ty    = W->stack[0].matrix[2][1] + 0.5;
		W->Sx    = W->stack[0].Sx;
		W->Sy    = W->stack[0].Sy;
		W->ra    = W->stack[0].ra;
	}
}



/* Concatenate the given matrix with the first matrix in the stack,
*  push the stack down and make the concatenated matrix the top of
*  the stack.
*/
void glConcatMatrix(Matrix m)
{
	Matrix a, b;
	glGetMatrix(a);
	mult(m, a, b);
	glPushMatrix(b);
}



/* Initialize the given matrix to unit matrix.
*/
void glIdentityMatrix(Matrix m)
{
	m[0][0] = 1.0;
	m[0][1] = 0.0;
	m[1][0] = 0.0;
	m[1][1] = 1.0;
	m[2][0] = 0.0;
	m[2][1] = 0.0;
}


/* Create a rotation matrix and multiply the given argument
*  list matrix by it.
*/
void glRotate(Matrix m, float deg)
{
	Matrix n;
	float  c, s;

	c = COSDEG(deg);
	s = SINDEG(deg);
	n[0][0] = c;
	n[0][1] = s;
	n[1][0] = -s;
	n[1][1] = c;
	n[2][0] = 0.0;
	n[2][1] = 0.0;
	mult(m, n, m);
}


/* Create a translation matrix and multiply the given argument
*  list matrix by it.
*/
void glTranslate(Matrix m, float tx, float ty)
{
	Matrix n;
	n[0][0] = 1.0;
	n[0][1] = 0.0;
	n[1][0] = 0.0;
	n[1][1] = 1.0;
	n[2][0] = tx;
	n[2][1] = ty;
	mult(m, n, m);
}


/* Create a scale matrix and multiply the given argument
*  list matrix by it.
*/
void glScale(Matrix m, float sx, float sy)
{
	Matrix n;
	n[0][0] = sx;
	n[0][1] = 0.0;
	n[1][0] = 0.0;
	n[1][1] = sy;
	n[2][0] = 0.0;
	n[2][1] = 0.0;
	mult(m, n, m);
}


/* Create a matrix given scale, rotation and translation.
 *
 * Parameters: sx, sy - scale factors
 *             tx, ty - translation
 *             angle  - rotation angle in degrees.
 */
void glBuildMatrix( Matrix m, float sx, float sy, float tx, float ty, float angle )
{
	float  c, s;

	c = COSDEG(angle);
	s = SINDEG(angle);

	m[0][0] = sx*c;
	m[0][1] = sx*s;
	m[1][0] = -sy*s;
	m[1][1] = sy*c;
	m[2][0] = tx;
	m[2][1] = ty;
}


/**********************************************************************/
/*
 *   The following functions are here as they use the matrix for their
 *   results and I wanted to keep all of such functions together for
 *   ease of maintenance.
 */
/**********************************************************************/


/* Convert screen coordinates into map coordinates. The matrix transform
*  equations are for a bottom left origin coordinate system so the y
*  screen coordinate must be inverted.
*/
void glScreen2Map(Screencoord sx, Screencoord sy, Coord *x, Coord *y)
{
	float  a, xp, yp;
	Matrix m;

	glGetMatrix(m);
	xp = (float) sx;
	yp = (float)W->ym - (float)(sy + 1);
	a = m[0][0]*m[1][1] - m[1][0]*m[0][1];
	*x = (m[1][1] * ( xp - m[2][0] ) - m[1][0] * ( yp - m[2][1] )) / a;
	*y = (m[0][0] * ( yp - m[2][1] ) - m[0][1] * ( xp - m[2][0] )) / a;
}


/* Convert map coordinates into screen coordinates
*/
void glMap2Screen(Coord x, Coord y, Screencoord *sx, Screencoord *sy)
{
	Matrix m;
	glGetMatrix(m);
	*sx = (Screencoord) (x * m[0][0] - y * m[0][1] + m[2][0] + 0.5);
	*sy = (Screencoord) ((float)W->ym - y * m[1][1] + x * m[1][0] - m[2][1] - 0.5);
}


/* Convert vdc coordinates into map coordinates
*/
void glVdc2Map(Coord vx, Coord vy, Coord *x, Coord *y)
{
	float  a, xp, yp;
	Matrix m;

	glGetMatrix(m);
	xp = vx * (float)(W->xm - 1);
	yp = vy * (float)(W->ym - 1);
	a = m[0][0]*m[1][1] - m[1][0]*m[0][1];
	*x = (m[1][1] * ( xp - m[2][0] ) - m[1][0] * ( yp - m[2][1] )) / a;
	*y = (m[0][0] * ( yp - m[2][1] ) - m[0][1] * ( xp - m[2][0] )) / a;
}


/* Convert map coordinates into vdc coordinates
*/
void glMap2Vdc(Coord mx, Coord my, Coord *x, Coord *y)
{
	Matrix m;
	glGetMatrix(m);
	*x = (mx * m[0][0] - my * m[0][1] + m[2][0]) / (float)(W->xm - 1);
	*y = (my * m[1][1] - mx * m[1][0] + m[2][1]) / (float)(W->ym - 1);
}

