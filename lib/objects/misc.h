/*********************************************************************/
/** @file misc.h
 *
 * Assorted low level object definitions (include file)
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    m i s c . h                                                       *
*                                                                      *
*    assorted low level object definitions (include file)              *
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
#ifndef MISC_DEFS
#define MISC_DEFS

/* Need things from fpa_types.h and fpa_macros.h */
#include <fpa_types.h>
#include <fpa_macros.h>
#include <fpa_math.h>



/** Define POINT object - (x,y) pairs */
typedef	float	POINT[2];
typedef	double	DBLPT[2];
#define X (0)
#define Y (1)



/* Define COMPONENT and COMP_INFO objects */
/** Component */
typedef unsigned char	COMPONENT;
/** Component info */
typedef	struct
	{
	COMPONENT	need;
	COMPONENT	have;
	} COMP_INFO;

/* Define types of COMPONENTs */
#define No_Comp (0)
/**  X_Comp - x (or u) component of fields                      */
#define X_Comp (1<<0)
/**  Y_Comp - y (or v) component of fields                      */
#define Y_Comp (1<<1)
/**  Z_Comp - z component of fields (not yet used)              */
#define Z_Comp (1<<2)
/**  T_Comp - time component of fields (not yet used)           */
#define T_Comp (1<<3)
/**  D_Comp - direction component of fields (for vector fields) */
#define D_Comp (1<<4)
/**  M_Comp - magnitude component of fields (for vector fields) */
#define M_Comp (1<<5)
#define XY_Comp  (X_Comp | Y_Comp)
#define All_Comp (X_Comp | Y_Comp)
#define DM_Comp  (D_Comp | M_Comp)
#define INIT_NO_COMP_INFO { No_Comp, No_Comp }
#define INIT_XY_COMP_INFO { XY_Comp, No_Comp }
#define INIT_DM_COMP_INFO { DM_Comp, No_Comp }



/** Define "BOX" structure:  Four sides of a rectangle */
typedef struct
	{
	float	left, right, bottom, top;
	} BOX;




/* Define "XFORM" structure: */
/** 2-D display transform matrix 3rd row (called "h" for homogeneous)
 * is used for translations */
typedef float XFORM[3][2];
#define H (2)




/** Define "ITBL" structure:  indexed list */
typedef	struct
		{
		int		ival;
		STRING	name;
		} ITBL;
#define ITBL_SIZE(list) ( sizeof(list) / sizeof(ITBL) )




/** Define range end condition */
typedef enum
	{
	RangeUnlim,	/**< unlimited */
	RangeIncl,	/**< inclusive */
	RangeExcl	/**< exclusive */
	} RANGE_COND;

/** Define range structure - min and max with matching end conditions */
typedef	struct	range_struct
	{
	double		minval;	/**< minimum value */
	RANGE_COND	mincon;	/**< minimum condition */
	double		maxval;	/**< maximum value */
	RANGE_COND	maxcon;	/**< maximum condition */
	} RANGE;




/** Define various display formats */
typedef enum
	{
	DisplayFormatSimple,	/**< Simple display */
	DisplayFormatComplex	/**< Complex display */
	} DISPLAY_FORMATS;


/* Now define some external variables */
#undef  GLOBAL
#ifdef MISC_INIT
#	define GLOBAL GLOBAL_INIT
#else
#	define GLOBAL GLOBAL_EXTERN
#endif

#define NullPoint        NullPtr(float *)
#define NullPointPtr     NullPtr(POINT *)
#define NullPointList    NullPtr(POINT *)
#define	ZeroPointListNum (1)
#define	ZeroPointList    make_plist(ZeroPoint)
#define NullBox          NullPtr(BOX *)
#define NullXform        (0)

#define	ZERO_POINT	{0.0, 0.0}
#define	UNIT_BOX	{0.0, 1.0, 0.0, 1.0}
#define	ZERO_BOX	{0.0, 1.0, 0.0, 1.0}
#define	IDENT_XFORM	{ {1.0,0.0}, {0.0,1.0}, {0.0,0.0} }

#define	UNLIMITED_RANGE	{ -FPA_FLT_MAX, RangeUnlim, FPA_FLT_MAX, RangeUnlim }

GLOBAL(const POINT,	ZeroPoint,        ZERO_POINT);
GLOBAL(const BOX,	UnitBox,          UNIT_BOX);
GLOBAL(const BOX,	ZeroBox,          ZERO_BOX);
GLOBAL(const XFORM,	IdentXform,       IDENT_XFORM);

GLOBAL(const COMP_INFO,	NoCompInfo,	INIT_NO_COMP_INFO);
GLOBAL(const COMP_INFO,	XYCompInfo,	INIT_XY_COMP_INFO);
GLOBAL(const COMP_INFO,	DMCompInfo,	INIT_DM_COMP_INFO);

GLOBAL(const RANGE, UnlimitedRange, UNLIMITED_RANGE);

/* Now declare public functions in misc.c */
void	copy_point(POINT pos1, const POINT pos2);
void	set_point(POINT pos, float x, float y);
float	point_dist(POINT posa, POINT posb);
float	point_dist2(POINT posa, POINT posb);
float	*make_point(float x, float y);
POINT	*make_plist(const POINT pos);
LOGICAL	add_component(COMP_INFO *info, const COMPONENT comp);
LOGICAL	need_component(const COMP_INFO *info, const COMPONENT comp);
LOGICAL	have_component(const COMP_INFO *info, const COMPONENT comp);
LOGICAL	ready_components(const COMP_INFO *info);
void	copy_box(BOX *box1, const BOX *box2);
LOGICAL	inside_box(const BOX *box, const POINT pos);
LOGICAL	inside_box_xy(const BOX *box, float x, float y);
void	copy_xform(XFORM xf1, const XFORM xf2);
void	build_xform(XFORM xf, float scalex, float scaley,
						float translatex, float translatey, float rotation);
void	translate_xform(XFORM	xform, float tx, float ty);
void	rotate_xform(XFORM xform, POINT pivot, float angle);
void	scale_xform(XFORM	xform, POINT point, float sx, float sy);
void	transform_point(XFORM	xform, POINT pin, POINT pout);
void	block_xform(XFORM xf, const BOX *viewport, const BOX *window);
int		find_itbl_entry(const ITBL *tbl, const int num, const STRING name);
STRING	find_itbl_string(const ITBL *tbl, const int num, const int ival);
void	copy_range(RANGE *range1, const RANGE *range2);
LOGICAL	read_range(STRING string, RANGE *range);
LOGICAL	test_range(double value, RANGE *range);
LOGICAL	same_range(const RANGE *range1, const RANGE *range2);
LOGICAL	sub_range(const RANGE *range, const RANGE *subrange);

/* Now it has been included */
#endif
