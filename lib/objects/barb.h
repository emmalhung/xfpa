/*********************************************************************/
/** @file barb.h
 *
 * BARB object definitions (include file)
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    b a r b . h                                                       *
*                                                                      *
*    BARB object definitions (include file)                            *
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
#ifndef BARB_DEFS
#define BARB_DEFS

/* We need definitions for other objects */
#include "pspec.h"
#include "attrib.h"

/* Define BARB object */
/** a wind barb (speed and direction) */
typedef struct BARB_struct
	{
	float		dir;		/**< direction */
	float		speed;		/**< wind speed */
	float		gust;		/**< wind gust speed */
	POINT		anchor;		/**< location of barb */
	BSPEC		bspec;		/**< how to display barb arrow */
	TSPEC		tspec;		/**< how to display barb value */
	ATTRIB_LIST	attrib;		/**< attribute list */
	/* following will become obsolete */
	STRING		subelem;	/**< subelement */
	STRING		value;		/**< value !!! left in for completeness only !!! */
	STRING		label;		/**< label !!! left in for completeness only !!! */
	} *BARB;

/* Convenient definitions */
#define NullBarb        NullPtr(BARB)
#define NullBarbPtr     NullPtr(BARB *)
#define NullBarbList    NullPtr(BARB *)
#define NullBarbListPtr NullPtr(BARB **)

/* Declare all functions in barb.c */
BARB	create_barb(STRING subelem, const POINT anchor,
						float dir, float speed, float gust);
BARB	destroy_barb(BARB barb);
BARB	copy_barb(const BARB barb);
void	define_barb_value(BARB barb, STRING subelem, const POINT anchor,
						float dir, float speed, float gust);
void	recall_barb_value(BARB barb, STRING *subelem, POINT anchor,
						float *dir, float *speed, float *gust);
void	define_barb_attribs(BARB barb, ATTRIB_LIST attribs);
void	recall_barb_attribs(BARB barb, ATTRIB_LIST *attribs);
void	define_barb_pspec(BARB barb, PPARAM param, POINTER value);
void	recall_barb_pspec(BARB barb, PPARAM param, POINTER value);

/* Functions in barb_set.c are declared in set_oper.h */

/* Now it has been included */
#endif
