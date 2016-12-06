/**********************************************************************/
/** @file spline.h
 *
 *  SPLINE, PATCH and ILIST object definitions (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*    s p l i n e . h                                                   *
*                                                                      *
*    SPLINE, PATCH and ILIST object definitions (include file)         *
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
#ifndef SPLINE_DEFS
#define SPLINE_DEFS

/* Need other objects for patch function */
#include "set.h"
#include "polynomial.h"


/* Define the number of values per line for debug functions */
#define MAX_PCOUNT	5


/* Define SCALAR Object */
/** contains a constant scalar value */
typedef struct SCALAR_struct
	{
	float	sval;			/**< value of scalar */
	} SCALAR;

/* Declare scalar related functions in spline.c */
void	init_scalar(SCALAR *scalr);
void	free_scalar(SCALAR *scalr);
void	copy_scalar(SCALAR *scopy, const SCALAR *scalr);
void	debug_scalar(SCALAR *scalr);


/* Define VLIST Object */
/** contains a list of values at random locations */
typedef struct VLIST_struct
	{
	int		numpts;		/**< number of points/values */
	int		maxpts;		/**< allocated points/values */
	float	*val;		/**< list of values */
	POINT	*pos;		/**< list of points */
	} VLIST;

/* Declare vlist related functions in spline.c */
void	init_vlist(VLIST *vlist);
void	free_vlist(VLIST *vlist);
void	copy_vlist(VLIST *vcopy, const VLIST *vlist);
void	empty_vlist(VLIST *vlist);
void	add_point_to_vlist(VLIST *vlist, POINT pos, float value);
LOGICAL	same_vlist_points(const VLIST *vlist1, const VLIST *vlist2);
void	debug_vlist(VLIST *vlist);


/* Define GRID Object */
/** an array of grid values on a defined grid */
typedef struct GRID_struct
	{
	int		nx, ny;		/**< size of grid in x an y directions */
	float	**gval;		/**< grid value array [ny][nx] */
	POINT	origin;		/**< origin of grid relative to local co-ords */
	float	orient;		/**< orientation of grid relative to local co-ords */
	float	gridlen;	/**< grid length for uniform points (in km) */
	} GRID;

/* Declare grid related functions in spline.c */
void	init_grid(GRID *gridd);
void	free_grid(GRID *gridd);
void	define_grid(GRID *gridd, int nx, int ny, const POINT origin,
						float orient, float gridlen, float *data, int ncol);
void	copy_grid(GRID *gcopy, const GRID *gridd);
LOGICAL	same_grid_size(const GRID *gridd1, const GRID *gridd2);
void	debug_grid(GRID *gridd);


/* Define the order of the spline to be cubic */
#define ORDER 4


/* Define scalar/vector field mode */
typedef	enum	{ DimScalar, DimVector2D } SPDIM;


/* Define SPLINE object */
/** a bi-variate B-spline defined by a control vertex 
 * array and a pair of knot vectors */
typedef struct SPLINE_struct
	{
	int		m, n;		/**< size of spline in u and v directions */
	SPDIM	dim;		/**< scalar or vector? */
	float	**cvs;		/**< control vertex array [m][n] (becomes vector mag) */
	float	**cvx;		/**< x-component control vertex array (vector only) */
	float	**cvy;		/**< y-component control vertex array (vector only) */
	MAP_PROJ	mp;		/**< projection (for gradient corrections) */
	POINT	origin;		/**< origin of spline relative to local co-ords */
	float	orient;		/**< orientation of spline relative to local co-ords */
	XFORM	xform;		/**< transform to get to origin and orientation */
	float	gridlen;	/**< grid length for uniform knots (in map units) */
	float	*uknots;	/**< u knot sequence [m+k] */
	float	*vknots;	/**< v knot sequence [n+l] */
	} SPLINE;

/* Useful parameters/macros */
#define SplineIsVector(sp) ( (!sp)? FALSE: (LOGICAL)((sp).dim == DimVector2D) )
#define SplineIsScalar(sp) ( (!sp)? FALSE: (LOGICAL)((sp).dim == DimScalar) )

/* Declare spline related functions in spline.c */
void	init_spline(SPLINE *spln);
void	free_spline(SPLINE *spln);
void	define_spline(SPLINE *spln, int m, int n, const MAP_PROJ *mproj,
						const POINT origin, float orient, float gridlen,
						float *cvs, int ncol);
void	define_spline_2D(SPLINE *spln, int m, int n, const MAP_PROJ *mproj,
						const POINT origin, float orient, float gridlen,
						float *cvx, float *cvy, int ncol);
void	copy_spline(SPLINE *spcopy, const SPLINE *spln);
void	calc_spline_mag_cvs(SPLINE *spln);
LOGICAL	same_spline_size(const SPLINE *spln1, const SPLINE *spln2);
void	debug_spline(SPLINE *spln);
void	change_spline_units(SPLINE *spln, double factor, double offset);
float	x_control_vertex(SPLINE *spln, int iu);
float	y_control_vertex(SPLINE *spln, int iv);
LOGICAL	find_patch(SPLINE *spln, POINT pw, int *iup, int *ivp,
						POINT pp, POINT dp);
LOGICAL	find_patch_unmapped(SPLINE *spln, POINT ps, int *iup, int *ivp,
						POINT pp, POINT dp);
LOGICAL	world_to_spline(SPLINE *spln, POINT pw, POINT ps);
LOGICAL	world_to_patch(SPLINE *spln, POINT pw, int iup, int ivp,
						POINT pp, POINT dp);
LOGICAL	spline_to_world(SPLINE *spln, POINT ps, POINT pw);
LOGICAL	spline_to_patch(SPLINE *spln, POINT ps, int iup, int ivp,
						POINT pp, POINT dp);
LOGICAL	patch_to_world(SPLINE *spln, POINT pp, int iup, int ivp, POINT pw);
LOGICAL	patch_to_spline(SPLINE *spln, POINT pp, int iup, int ivp, POINT ps);


/* Define croot structure */
/** information about one intersection */
typedef struct CROOT_struct
	{
	float	cval;	/**< contour value */
	float	root;	/**< a root of F(u) = cval */
	float	slope;	/**< slope at root */
	short	icspec;	/**< contour spec pointer */
	} CROOT;

/* Define ILIST object */
/** a buffer of points where contours intersect a 
 * particular patch boundary */
typedef struct ILIST_struct
	{
	CROOT	*froots;	/**< intersections of F(u) = cval */
	CROOT	*xroots;	/**< intersections of dFdx = 0 */
	CROOT	*yroots;	/**< intersections of dFdy = 0 */
	short	numf, maxf;	/**< number of intersections */
	short	numx, numy;	/**< number of derivative zeros */
	short	defined;	/**< has this ilist been defined */
	} *ILIST;

/* Useful parameters for ilists */
#define DELTA_ROOTS 1
#define	NullIlist NullPtr(ILIST)

/* Declare ilist related functions in spline.c */
ILIST	create_ilist(void);
ILIST	destroy_ilist(ILIST lst);
void	free_ilist(ILIST lst);
void	empty_ilist(ILIST lst);
ILIST	copy_ilist(const ILIST lst);
void	compute_ilist(ILIST lst, BIPOLY *func, char sense, float value,
						int ncspec, CONSPEC *cspecs);
void	find_ilist_roots(ILIST lst, UNIPOLY *func, char type,
						int ncspec, CONSPEC *cspecs);
void	add_root_to_ilist(ILIST lst, char type, float cval,
						float root, float slope, int icspec);


/* Define LSPLINE structure */
 /** a local segment of a B-spline enough to define 
  * the surface within one patch (only used for 
  * generating a patch) */
typedef struct LSPLINE_struct
	{
	float	cvs[ORDER][ORDER];	/**< local control vertex array [k][l] */
	float	uknots[ORDER+ORDER];	/**< local u knot sequence [2k] */
	float	vknots[ORDER+ORDER];	/**< local v knot sequence [2l] */
	} LSPLINE;

/* Define PATCH object */
/** the region bounded by two consecutive uknots 
 * and two consecutive vknots */
typedef struct PATCH_struct
	{
	SET		contours;	/**< contour curve set */
	SET		extrema;	/**< max/min/saddle point mark set */
	SET		vectors;	/**< to display wind barbs or arrows */
	struct
		{
		UNSIGN	all:1;	/**< patch completely unmasked */
		UNSIGN	none:1;	/**< patch completely masked */
		SET		masks;	/**< mask definitions */
		} mask;
	SPDIM	dim;		/**< scalar or vector? */
	BIPOLY	function;	/**< patch function (becomes magnitude for vector) */
	BIPOLY	xfunc;		/**< x-component patch function (vector only) */
	BIPOLY	yfunc;		/**< y-component patch function (vector only) */
	XFORM	xform;		/**< transform back to surface */
	short	defined;	/**< has this patch been defined */
	} *PATCH;

/* Useful parameters/macros for patches */
#define	NullPatch NullPtr(PATCH)
#define PatchIsVector(p) ( (!p)? FALSE: (LOGICAL)((p).dim == DimVector2D) )
#define PatchIsScalar(p) ( (!p)? FALSE: (LOGICAL)((p).dim == DimScalar) )

/* Declare patch related functions in spline.c */
PATCH	create_patch(void);
PATCH	destroy_patch(PATCH ptch);
void	free_patch(PATCH ptch);
void	empty_patch(PATCH ptch);
void	define_patch(PATCH ptch, SPLINE *pspln, int iup, int ivp);
void	define_lspline(LSPLINE *local, SPLINE *pspln, int comp,
						int iup, int ivp);
PATCH	copy_patch(const PATCH ptch);
void	evaluate_patch_basis(float U, float *B_vals);
void	evaluate_patch_deriv(int deriv, float U, float *B_vals);
void	generate_patch_function(BIPOLY *func, LSPLINE *local);
void	reset_patch_mask(PATCH ptch);
void	add_patch_mask(PATCH ptch, SET set, STRING bcat, STRING category,
						STRING mode, STRING rule);
void	patch_control(int nu, int nv, float gridlen);
void	set_patch_index(int iup, int ivp);
void	contour_patch(PATCH ptch, ILIST llist, ILIST rlist,
						ILIST blist, ILIST tlist, int ncspec, CONSPEC *cspecs);
void	do_patch_vectors(PATCH ptch, int iu, int iv, int ncspec,
						CONSPEC *cspecs, double factor, double offset);
void	reset_patch_conspec(PATCH ptch, int ncspec, CONSPEC *cspecs);

/* Now it has been included */
#endif
