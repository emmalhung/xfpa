/***********************************************************************
*                                                                      *
*     f p a _ m a t h . h                                              *
*                                                                      *
*     Natural extensions to the UNIX math library and math.h           *
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
#ifndef FPA_MATH

/* Need to define __EXTENSIONS__ to get definitions of M_PI etc. */
#ifdef MACHINE_SUN
#	ifdef __EXTENSIONS__
#		define _EXTENSIONS_ALREADY
#	else
#		define __EXTENSIONS__
#		undef _EXTENSIONS_ALREADY
#	endif
#endif
#ifdef MACHINE_PCLINUX
#	ifdef __USE_BSD
#		define _BSD_ALREADY
#	else
#		define __USE_BSD
#		undef _BSD_ALREADY
#	endif
#endif

#	include <math.h>
#	include <sys/param.h>
#	undef MAXINT	/* this is defined in both places */
#	include <values.h>

#ifdef MACHINE_SUN
#	ifdef _EXTENSIONS_ALREADY
#		undef _EXTENSIONS_ALREADY
#	else
#		undef __EXTENSIONS__
#	endif
#endif
#ifdef MACHINE_PCLINUX
#	ifdef _BSD_ALREADY
#		undef _BSD_ALREADY
#	else
#		undef __USE_BSD
#	endif
#endif

/* Convenient constants */
#	ifndef M_PI
#		define M_PI   3.14159265358979323846
#		define M_PI_2 1.57079632679489661923
#		define M_PI_4 0.78539816339744830962
#	endif
#	ifndef PI
#		define PI (M_PI)
#	endif
#	ifndef RAD
#		define RAD (M_PI/180.0)
#	endif
#	define FPA_FLT_MAX 3.40282347E+38
#	define FPA_FLT_MIN 1.17549435E-38

/* Convenient in-line functions */
#	ifndef MIN
#		define MIN(x,y) ( ((x)<(y)) ? (x) : (y) )
#	endif
#	ifndef MAX
#		define MAX(x,y) ( ((x)>(y)) ? (x) : (y) )
#	endif
#	ifndef ABS
#		define ABS(x) ( ((x)>=0) ? x : -x )
#	endif
#	ifndef SIGN
#		define SIGN(x) ( ((x)>=0) ? 1 : -1 )
#	endif
#	ifndef NINT
#		define NINT(x) (int) ( ((x)>=0) ? ((x) + .5) : ((x) - .5) )
#	endif
#	ifndef Divisible
#		define Divisible(i, j) ( (i)%(j) == 0 )
#	endif
#	ifndef BIT
#		define BIT(i) ( 1 << (i) )
#	endif
#	ifndef SETBIT
#		define SETBIT(a, i) ( (a) |= BIT(i) )
#	endif
#	ifndef GETBIT
#		define GETBIT(a, i) ( ( (a) & BIT(i) ) != 0 )
#	endif
#	ifndef BYTE
#		define BYTE(i) ( 255 << (i*8) )
#	endif
#	ifndef SETBYTE
#		define SETBYTE(a, i, b) \
			{ (a) &= ~BYTE(i); (a) |= ( ((b)&255 << (i*8)) ); }
#	endif
#	ifndef GETBYTE
#		define GETBYTE(a, i) ( ( (a) & BYTE(i) ) >> (i*8) )
#	endif

/* Degree trig */
#	define	sindeg(x)		sin(RAD*(x))
#	define	cosdeg(x)		cos(RAD*(x))
#	define	tandeg(x)		tan(RAD*(x))
#	define	asindeg(x)		(asin(x)/RAD)
#	define	acosdeg(x)		(acos(x)/RAD)
#	define	atandeg(x)		(atan(x)/RAD)
#	define	atan2deg(y,x)	(atan2(y,x)/RAD)

/* Functions in fpa_math.c */
#	ifdef MACHINE_SUN_NOT
	double	copysign(double, double);
#	endif

/* This is nicely defined in math.h on the HP */
/* It appears to be defined in math.h on the SUN - but causes an error */
#	ifdef MACHINE_SUN_NOT
		struct exception
			{
			int type;
			char *name;
			double arg1;
			double arg2;
			double retval;
			};
#		define DOMAIN      1
#		define SING        2
#		define OVERFLOW    3
#		define UNDERFLOW   4
#		define TLOSS       5
#		define PLOSS       6
#	endif

/* Now it has been included */
#	define FPA_MATH
#endif
