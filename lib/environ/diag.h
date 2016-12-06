/**********************************************************************/
/** @file diag.h
 *
 *  Routines to interpret the "diag" setup block (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   d i a g . h                                                        *
*                                                                      *
*   Routines to interpret the "diag" setup block (include file)        *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2002 Environment Canada (MSC)            *
*	  Version 7 (c) Copyright 2006 Environment Canada				   *
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
#ifndef DIAG_DEFS
#define DIAG_DEFS

/* Other header files */
#include <fpa_types.h>

/***********************************************************************
*                                                                      *
*  Declare external functions in diag.c                                *
*                                                                      *
***********************************************************************/

void	diag_control(LOGICAL enable, int olevel, int mstyle);


/* Now it has been included */
#endif
