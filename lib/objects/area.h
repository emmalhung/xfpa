/**********************************************************************/
/** @file area.h
 *
 *  AREA object definitions (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*    a r e a . h                                                       *
*                                                                      *
*    AREA object definitions (include file)                            *
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
#ifndef AREA_DEFS
#define AREA_DEFS

/* We need definitions for other objects */
#include "segment.h"
#include "bound.h"
#include "pspec.h"
#include "attrib.h"

/* Define SUBVIS object for SUBAREA */
/** a set of segments from various sub-area segments and hole boundaries
 * which enclose a single contiguous visible region
 */
typedef struct SUBVIS_struct
	{
	int			numvis;		/**< number of segments on visible region */
	SEGMENT		*segvis;	/**< list of segments on visible region */
	} *SUBVIS;

/* Define SUBAREA object */
/** a set of boundaries from various area boundaries and dividing lines
 * which enclose a single contiguous region 
 */
typedef struct SUBAREA_struct
	{
	int			numseg;		/**< number of boundary segments */
	SEGMENT		*segments;	/**< list of boundary segments */

	ATTRIB_LIST	attrib;		/**< Attribute list */

	/* Following will become obsolete */
	STRING		subelem;	/**< subelement of the enclosed region */
	STRING		value;		/**< value given to the enclosed region */
	STRING		label;		/**< label given to the enclosed region */

	LSPEC		lspec;		/**< how to display boundary */
	FSPEC		fspec;		/**< how to display interior */

	LOGICAL		visready;	/**< is visible subarea boundary ready? */
	int			nsubvis;	/**< number of visible segment lists */
	SUBVIS		*subvis;	/**< list of visible segment lists */
	int			numhole;	/**< number of holes totally inside */
	LINE		*holes;		/**< list of holes totally inside */
	} *SUBAREA;


/* Define AREA object */
/** a contiguous area which may be divided into several sub-areas
 * by dividing lines 
 */
typedef struct AREA_struct
	{
	BOUND		bound;		/**< whole boundary and holes */

	ATTRIB_LIST	attrib;		/**< Attribute list */

	/* Following will become obsolete */
	STRING		subelem;	/**< subelement of the whole enclosed region */
	STRING		value;		/**< value given to the whole enclosed region */
	STRING		label;		/**< label given to the whole enclosed region */

	LSPEC		lspec;		/**< how to display boundary */
	FSPEC		fspec;		/**< how to display interior */

	int			numdiv;		/**< number of dividing lines */
	LINE		*divlines;	/**< list of dividing lines */
	int			*subids;	/**< subarea that each dividing line divides */
	SUBAREA		*subareas;	/**< list of divided sub-areas (numdiv+1) */
	LOGICAL		visready;	/**< are all visible subarea boundaries ready? */
	} *AREA;


/* Convenient definitions */
#define NullArea           NullPtr(AREA)
#define NullAreaPtr        NullPtr(AREA *)
#define NullAreaList       NullPtr(AREA *)
#define NullAreaListPtr    NullPtr(AREA **)
#define NullSubArea        NullPtr(SUBAREA)
#define NullSubAreaPtr     NullPtr(SUBAREA *)
#define NullSubAreaList    NullPtr(SUBAREA *)
#define NullSubAreaListPtr NullPtr(SUBAREA **)
#define NullSubVis         NullPtr(SUBVIS)
#define NullSubVisPtr      NullPtr(SUBVIS *)
#define NullSubVisList     NullPtr(SUBVIS *)
#define NullSubVisListPtr  NullPtr(SUBVIS **)

/** Area member identifier */
typedef	enum { AreaNone, AreaBound, AreaHole, AreaDiv, AreaLabel } AMEMBER;

/** Status returns from complicated functions */
typedef	enum { DivOK, DivNoInfo, DivNotMySub, DivNoSub, DivLineStart,
			   DivLineMid, DivLineEnd, DivAreaRight, DivAreaLeft,
			   DivTooClose, DivTooShort } DIVSTAT;


/* Declare all functions in area.c */
AREA	create_area(STRING subelem, STRING value, STRING label);
AREA	destroy_area(AREA area);
void	empty_area(AREA area);
AREA	copy_area(const AREA area, LOGICAL all);
void	define_area(AREA area, LINE boundary, int numhole, const LINE *holes);
void	define_area_boundary(AREA area, LINE boundary);
void	define_area_holes(AREA area, int numhole, const LINE *holes);
void	add_area_hole(AREA area, LINE hole);
void	remove_area_hole(AREA area, LINE hole);
void	remove_all_area_holes(AREA area);
void	define_area_value(AREA area,
						STRING subelem, STRING value, STRING label);
void	recall_area_value(AREA area,
						STRING *subelem, STRING *value, STRING *label);
void	define_area_attribs(AREA area, ATTRIB_LIST attribs);
void	recall_area_attribs(AREA area, ATTRIB_LIST *attribs);
void	define_area_pspec(AREA area, PPARAM param, POINTER value);
void	recall_area_pspec(AREA area, PPARAM param, POINTER value);

int		which_area_hole(AREA area, LINE line);
int		which_area_divide(AREA area, LINE line);
int		which_area_subarea(AREA area, SUBAREA sub);
void	highlight_area(AREA area, HILITE pcode, HILITE fcode);
void	widen_area(AREA area, float delta);

SUBAREA	create_subarea(STRING subelem, STRING value, STRING label);
SUBAREA	destroy_subarea(SUBAREA subarea);
void	empty_subarea(SUBAREA subarea);
SUBAREA	copy_subarea(const SUBAREA subarea, LOGICAL all);
void	define_subarea_value(SUBAREA subarea,
						STRING subelem, STRING value, STRING label);
void	recall_subarea_value(SUBAREA subarea,
						STRING *subelem, STRING *value, STRING *label);
void	define_subarea_attribs(SUBAREA subarea, ATTRIB_LIST attribs);
void	recall_subarea_attribs(SUBAREA subarea, ATTRIB_LIST *attribs);
void	define_subarea_pspec(SUBAREA subarea, PPARAM param, POINTER value);
void	recall_subarea_pspec(SUBAREA subarea, PPARAM param, POINTER value);
void	highlight_subarea(SUBAREA subarea, HILITE pcode, HILITE fcode);
void	widen_subarea(SUBAREA subarea, float delta);

SUBVIS	create_subvis(void);
SUBVIS	destroy_subvis(SUBVIS subvis);

/* Declare all functions in area_oper.c */
void	area_properties(AREA area,
						LOGICAL *clockwise, float *size, float *length);
void	area_test_point(AREA area, POINT ptest,
						float *pdist, POINT ppoint, AMEMBER *mtype,
						int *imem, int *ispan, LOGICAL *inside);
LOGICAL	area_sight(AREA area, POINT pos1, POINT pos2, LOGICAL back,
						float *dist, float *approach, POINT point,
						AMEMBER *mtype , int *imem, int *ispan,
						LOGICAL *between);
AMEMBER	area_closest_feature(AREA area, POINT ptest,
						float *pdist, POINT ppoint, int *imem, int *ispan);
int		area_closest_point(AREA area, POINT ptest, float *dist, POINT point);
LINE	area_closest_hole(AREA area, POINT ptest, float *dist, POINT point);
LOGICAL	hole_inside_area(AREA area, LINE hole);
LOGICAL	inbox_area(AREA area, const BOX *box);
LOGICAL	translate_area(AREA area, float dx, float dy);
LOGICAL	rotate_area(AREA area, POINT ref, float angle);
LINE	clip_divline_to_area(AREA area, LINE divl, LOGICAL fwd,
						LOGICAL nearedge, DIVSTAT *status);
void	subarea_properties(SUBAREA sub,
						LOGICAL *clockwise, float *size, float *length);
void	subarea_test_point(SUBAREA sub, POINT ptest,
						float *pdist, POINT ppoint, int *iseg, int *ispan,
						LOGICAL *inside);
LOGICAL	subarea_sight(SUBAREA sub, POINT pos1, POINT pos2, LOGICAL back,
						float *dist, float *approach, POINT point,
						int *iseg, int *ispan, LOGICAL *between);
int		find_subarea_crossover(SUBAREA sub, LINE line, int ipin, int ipout,
						POINT px, int *iseg, int *ispan);
void	build_area_subareas(AREA area);
AREA	area_from_subarea(SUBAREA);
LINE	outline_from_subarea(SUBAREA);
LINE	outline_from_subvis(SUBVIS);
LOGICAL	replace_area_boundary(AREA area, LINE boundary, DIVSTAT *status);
LOGICAL	replace_area_hole(AREA area, int chole, LINE modhole, DIVSTAT *status);
LOGICAL	replace_area_divide(AREA area, int cdiv, LINE moddiv, DIVSTAT *status);
LOGICAL	remove_area_divide(AREA area, int cdiv, LOGICAL right, DIVSTAT *status);
LOGICAL	partial_area_redivide(AREA area, int cdiv, DIVSTAT *status);
LOGICAL	divide_area(AREA area, SUBAREA sub, LINE divl,
						SUBAREA *lsub, SUBAREA *rsub, DIVSTAT *status);
LINE	prepare_area_divline(AREA area, SUBAREA sub, LINE divl,
						DIVSTAT *status);
LINE	clip_divline_to_subarea(SUBAREA sub, LINE divl, LOGICAL fwd,
						LOGICAL nearedge, DIVSTAT *status);
LOGICAL	undivide_area(AREA area, SUBAREA sub1, SUBAREA sub2);
LOGICAL	adjacent_subareas(AREA area, int idiv, SUBAREA *lsub, SUBAREA *rsub);
void	reset_area_subids(int numdiv, int *subids, int mdiv, int next,
						LOGICAL setnext);
LOGICAL	line_inside_subarea(SUBAREA sub, LINE line);

/* Declare all functions in area_prep.c */
void	prep_area(AREA area);
void	prep_subarea(AREA area, SUBAREA sub);
void	prep_area_complex(AREA area);
void	prep_subarea_complex(AREA area, SUBAREA sub);
LOGICAL	divide_subarea_holes(SUBAREA area, BOUND bound);
int		clip_line_by_area_holes(LINE line, AREA area, LINE **lsegs);
LOGICAL	prep_area_bound_holes(BOUND bound);

/* Functions in area_set.c are declared in set_oper.h */

/* Now it has been included */
#endif
