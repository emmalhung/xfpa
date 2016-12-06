/*************************************************************************/
/*
*   File:     glib_lib_fcns.c
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
/*************************************************************************/
#define FGLMAIN
#include <math.h>
#include <sys/stat.h>
#include <tools/tools.h>
#include "glib_private.h"


/* Routines to scale Xgl coords to X11 coords.
 */
int _xgl_XRscale(float x)
{
	x =	floorf(x * W->Sx + 0.5);
	if(x > INT_MAX) x = INT_MAX;
	if(x < INT_MIN) x = INT_MIN;
	return (int) x;
}

int _xgl_YRscale(float y)
{
	y =	floorf(y * W->Sy + 0.5);
	if(y > INT_MAX) y = INT_MAX;
	if(y < INT_MIN) y = INT_MIN;
	return (int) y;
}

int _xgl_Xscale(float x, float y)
{
	x = floorf(x*W->RxCos - y*W->RySin + W->Tx);
	if(x > INT_MAX) x = INT_MAX;
	if(x < INT_MIN) x = INT_MIN;
	return (int) x;
}

/* The limit check here is reversed because of the axis reversal
 * and the calculation is not done as if W->ym has a normal value
 * the calculation does not make sense anyway.
 */
int _xgl_Yscale(float x, float y)
{
	y = floorf(y*W->RyCos - x*W->RxSin + W->Ty);
	if(y > INT_MAX) return INT_MIN;
	if(y < INT_MIN) return INT_MAX;
	return W->ym - 1 - (int) y;
}

/* Trig functions in float form with near-zero tolerance.
 */
#define NEAR_ZERO(x)  ((fabsf(x) < 1.0e-07)? 0.0: x)

float _xgl_sin(float t)
{
	float v = sinf(t);
	return NEAR_ZERO(v);
}

float _xgl_cos(float t)
{
	float v = cosf(t);
	return NEAR_ZERO(v);
}

float _xgl_tan(float t)
{
	float v = tanf(t);
	return NEAR_ZERO(v);
}

float _xgl_asin(float a)
{
	float v = asinf(a);
	return NEAR_ZERO(v);
}

float _xgl_acos(float a)
{
	float v = acosf(a);
	return NEAR_ZERO(v);
}

float _xgl_atan(float a)
{
	float v = atanf(a);
	return NEAR_ZERO(v);
}

float _xgl_sqrt(float x)
{
	if ( x < 0.0 )
	{
		(void) fprintf(stderr, "[_xgl_sqrt] Negative argument: %f , returning 0\n", x);
		return 0.0;
	}
	return sqrtf(x);
}



/*  The result is equivalent to doing first src1 then src2. The dst parameter can
 *  not be one of src1 or src2
 */
static void affine_multiply (float dst[6], const float src1[6], const float src2[6])
{
	dst[0] = src1[0] * src2[0] + src1[1] * src2[2];
	dst[1] = src1[0] * src2[1] + src1[1] * src2[3];
	dst[2] = src1[2] * src2[0] + src1[3] * src2[2];
	dst[3] = src1[2] * src2[1] + src1[3] * src2[3];
	dst[4] = src1[4] * src2[0] + src1[5] * src2[2] + src2[4];
	dst[5] = src1[4] * src2[1] + src1[5] * src2[3] + src2[5];
}



/* Set up an affine transformation matrix given:
 *
 * dst:	the destination coefficient array
 * sx:	scale factor in the x direction
 * sy:	scale factor in the y direction
 * ra:	rotation angle in degrees (clockwise rotation)
 * tx:	translation in the x direction
 * ty:	translation in the y direction
 *
 * This transformation matrix is used by both the image processing rfunctions and
 * the XFLD font manipulation capability of the X11R6 server.
 * 
 */
void _xgl_affine_coef( float dst[6], float sx, float sy, Angle ra, int tx, int ty )
{
	float s, c, scale[6], rotate[6], translate[6], out[6];

	/* The affine transformation works in the same sense as X, that
	 * is counter clockwise rotation and y increasing downwards.
	 * FpaXgl works with clockwise rotation and y increasing upwards,
	 * so the angle sign must be reversed.
	 */
	s = SINDEG(-ra);
	c = COSDEG(-ra);

	rotate[0] = c;
	rotate[1] = s;
	rotate[2] = -s;
	rotate[3] = c;
	rotate[4] = 0;
	rotate[5] = 0;

	scale[0] = sx;
	scale[1] = 0;
	scale[2] = 0;
	scale[3] = sy;
	scale[4] = 0;
	scale[5] = 0;

	translate[0] = 1;
	translate[1] = 0;
	translate[2] = 0;
	translate[3] = 1;
	translate[4] = (float) tx;
	translate[5] = (float) ty;
	
	affine_multiply(out, scale, rotate);
	affine_multiply(dst, out, translate);
}


/**********************************************************************************
 * 
 * Drawing and elipse by integer calculation of the points on an ellipse.
 * 
 * Reference:  "Programmers Guide to PC&PS/2 Video Systems",
 *             Richard wilton  Microsoft Press, ISBN 1-55615-103-9
 *
 * This function draws an ellipse by calculating the points in the first quadrant
 * and reflecting them into the other 3 quadrants.  There are two different
 * algorithms used, one for when dy/dx < -1 and one for when dy/dx >-1.  Refer to
 * the book for a derivation of the algorithm; it is very similar to the derivation
 * of bresenham's line algorithm.  There are problems related with drawing very
 * small ellipses; he recommends using a more intense graphics mode.  Also, the
 * iteration will not terminate if rx or ry is 0.
 *
 * Note that this creates a non-rotated ellipse.
 *
 **********************************************************************************/

/* If the pixel location is within the area of the raster fill in
 * the given pixel with the assigned colour.
 */
static void set_pixel(UNCHAR *raster, int width, int height, int x, int y, UNCHAR *color)
{
	UNCHAR *p;
	
	if( x < 0 || x >= width || y < 0 || y >= height ) return;

	p = raster + y * width * 3 + x * 3;
	*p++ = color[0];
	*p++ = color[1];
	*p++ = color[2];
}


/*
 * Elipse drawing function.
 *
 * r      - raster to draw the ellipse into.
 * w, h   - raster width and height
 * xc, yc - center point of the ellipse.
 * rx,ry  - major and minor axis (radius) 
 * c      - colour of pixel as rgb.
 *
 * Note: There are two version here based on the same code. One is the original and
 *       uses integers for speed. The second uses floats for the variables. The second
 *       version is used when it is determined that one of the calculated values will
 *       overflow the integer limit.
 */
void _xgl_draw_ellipse(UNCHAR *r, int w, int h, int xc, int yc, int rx, int ry, UNCHAR *c)
{
	int    x, y;
	static float upper_limit = 0.0;

	/* Calculate the upper limit only once as this is math intensive */
	if(upper_limit == 0.0) upper_limit = pow(2.0, (float)((sizeof(long) * 8) - 1));

	if( rx <= 0 || ry <= 0) return;

	/* Check the limit on the biggest number used in the integer based algorithm.
	 * If less than this we can use integers.
	 */
	if( fabsf(2.0 * (float)rx * (float)rx * (float)ry) < upper_limit )
	{
		/* Use the fast integer calculation method */
		long  a, b, Asquared, TwoAsquared, Bsquared, TwoBsquared, d, dx, dy;

		x = 0;
		y = ry;
		a = (long) rx;
		b = (long) ry;

		Asquared    = a*a;
		TwoAsquared = 2 * Asquared;
		Bsquared    = b*b;
		TwoBsquared = 2 * Bsquared;

		d  = Bsquared - Asquared*b + Asquared/4L;
		dx = 0;
		dy = TwoAsquared * b;

		while (dx < dy)
		{
			set_pixel(r, w, h, xc + x, yc + y, c);
			set_pixel(r, w, h, xc - x, yc + y, c);
			set_pixel(r, w, h, xc + x, yc - y, c);
			set_pixel(r, w, h, xc - x, yc - y, c);
		
			if (d > 0L)
			{
				y--;
				dy -= TwoAsquared;
				d -= dy;
			}
			x++;
			dx += TwoBsquared;
			d  += Bsquared + dx;
		}

		d += (3L *(Asquared-Bsquared)/2L -(dx+dy)) / 2L;

		while (y >= 0)
		{
			set_pixel(r, w, h, xc + x, yc + y, c);
			set_pixel(r, w, h, xc - x, yc + y, c);
			set_pixel(r, w, h, xc + x, yc - y, c);
			set_pixel(r, w, h, xc - x, yc - y, c);

			if (d < 0L)
			{
				x++;
				dx += TwoBsquared;
				d  += dx;
			}
			y--;
			dy -= TwoAsquared;
			d  += Asquared - dy;
		}
	}
	else
	{
		/* This is the same as the integer version but done in float */
		float a, b, Asquared, TwoAsquared, Bsquared, TwoBsquared, d, dx, dy;

		x = 0;
		y = ry;
		a = (float) rx;
		b = (float) ry;

		Asquared    = a*a;
		TwoAsquared = 2 * Asquared;
		Bsquared    = b*b;
		TwoBsquared = 2 * Bsquared;

		d  = Bsquared - Asquared*b + Asquared/4L;
		dx = 0.0;
		dy = TwoAsquared * b;

		while (dx < dy)
		{
			set_pixel(r, w, h, xc + x, yc + y, c);
			set_pixel(r, w, h, xc - x, yc + y, c);
			set_pixel(r, w, h, xc + x, yc - y, c);
			set_pixel(r, w, h, xc - x, yc - y, c);
		
			if (d > 0.0)
			{
				y--;
				dy -= TwoAsquared;
				d -= dy;
			}
			x++;
			dx += TwoBsquared;
			d  += Bsquared + dx;
		}

		d += (3.0 *(Asquared-Bsquared)/2.0 -(dx+dy)) / 2.0;

		while (y >= 0)
		{
			set_pixel(r, w, h, xc + x, yc + y, c);
			set_pixel(r, w, h, xc - x, yc + y, c);
			set_pixel(r, w, h, xc + x, yc - y, c);
			set_pixel(r, w, h, xc - x, yc - y, c);

			if (d < 0.0)
			{
				x++;
				dx += TwoBsquared;
				d  += dx;
			}
			y--;
			dy -= TwoAsquared;
			d  += Asquared - dy;
		}
	}
}
