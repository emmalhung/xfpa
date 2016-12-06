/**********************************************************************/
/** @file math_ext.h
 *
 *  Natural extensions to the UNIX math library and math.h (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*    m a t h _ e x t . h                                               *
*                                                                      *
*    Natural extensions to the UNIX math library and math.h            *
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
#ifndef MATH_EXT

#	include <fpa_math.h>

/* Declare functions in math_ext.c */
#ifdef MACHINE_NOT_NEEDED
double	copysign(double x, double y);
#endif
double	fpa_sin(double t);
double	fpa_cos(double t);
double	fpa_tan(double t);
double	fpa_asin(double a);
double	fpa_acos(double a);
double	fpa_atan(double a);
double	fpa_atan2(double y, double x);
double	fpa_sindeg(double t);
double	fpa_cosdeg(double t);
double	fpa_tandeg(double t);
double	fpa_asindeg(double a);
double	fpa_acosdeg(double a);
double	fpa_atandeg(double a);
double	fpa_atan2deg(double y, double x);
double	qatan2(double y, double x);
double	fpa_sqrt(double x);

/* Now it has been included */
#	define MATH_EXT
#endif
