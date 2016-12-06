/**********************************************************************/
/** @file tween.h
 *
 * 2-D non-uniform cubic curve interpolation.  (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*    t w e e n . h                                                     *
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

#	ifndef TWEEN_DEFS

/* Modes for QuasiLinear_Tween() function                      */
/*  Mode QL_Proportional - compares link spans to longest span */
/*  Mode QL_Fixed        - compares link spans to set distance */
typedef	enum	{QL_Proportional, QL_Fixed} QL_MODES;

void	Tween1(int numkeys, double *keylist, double *keyval, int numtweens,
		double *tweenlist, double *tweenval, double *tweenderiv);

void	Tween(int numkeys, double *keylist, double *keyx, double *keyy,
		int numtweens, double *tweenlist, double *tweenx, double *tweeny);

void	PieceWise_2D(int numkeys, double *keylist, double *keyx, double *keyy,
		int numtweens, double *tweenlist, double *tweenx, double *tweeny);

void	set_quasilinear_mode(QL_MODES mode, double minval, double avgval);

void	QuasiLinear_Tween(int numkeys, double *keylist, double *keyx, double *keyy,
		int numtweens, double *tweenlist, double *tweenx, double *tweeny);

/* Now it has been included */
#		define TWEEN_DEFS
#	endif
