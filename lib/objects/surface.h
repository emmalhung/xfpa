/*********************************************************************/
/** @file surface.h
 *
 * SURFACE object definitions (include file)
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    s u r f a c e . h                                                 *
*                                                                      *
*    SURFACE object definitions (include file)                         *
*                                                                      *
*    A SURFACE object may be used to represent either:                 *
*                                                                      *
*       - a bi-variate scalar field F(x,y)                             *
*         (e.g. pressure/temperature), or                              *
*                                                                      *
*       - a bi-variate 2-D vector field F(x,y) = [ U(x,y), V(x,y) ]    *
*         (e.g. wind/currents).                                        *
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
#ifndef SURFACE_DEFS
#define SURFACE_DEFS

/* We need definitions for various objects */
#include "spline.h"
#include "set.h"


/* Define SURFACE object */
/** a B-spline with corresponding patch and ilist arrays */
typedef struct SURFACE_struct
	{
	USPEC		units;		/**< units spec */
	short		ncspec;		/**< number of contour specs */
	CONSPEC		*cspecs;	/**< contour specs */
	short		nuknot;		/**< number of u-knots */
	short		nvknot;		/**< number of v-knots */
	short		nupatch;	/**< number of u-patches */
	short		nvpatch;	/**< number of v-patches */
	SPLINE		sp;			/**< spline definition */
	ILIST		**ulist;	/**< horizontal int lists */
	ILIST		**vlist;	/**< vertical int lists */
	PATCH		**patches;	/**< array of patches */
	SET			bands;		/**< band contour areas */
	} *SURFACE;

/* Convenient definitions */
#define NullSfc    NullPtr(SURFACE)
#define NullSfcPtr NullPtr(SURFACE *)
#define SfcIsVector(sfc) ( (!sfc)? FALSE: SplineIsVector((sfc)->sp) )
#define SfcIsScalar(sfc) ( (!sfc)? FALSE: SplineIsScalar((sfc)->sp) )

/* Declare all functions in surface.c */
SURFACE	create_surface(void);
SURFACE	destroy_surface(SURFACE sfc);
SURFACE	copy_surface(const SURFACE sfc, LOGICAL all);
void	define_surface_spline(SURFACE sfc, int m, int n,
						const MAP_PROJ *mproj, const POINT origin, float orient,
						float gridlen, float *cvs, int ncol);
void	define_surface_spline_2D(SURFACE sfc, int m, int n,
						const MAP_PROJ *mproj, const POINT origin, float orient,
						float gridlen, float *cvx, float *cvy, int ncol);
void	define_surface_patches(SURFACE sfc, int m, int n);
LOGICAL	inside_surface_spline(SURFACE sfc, POINT ptest);
LOGICAL	inside_surface_spline_xy(SURFACE sfc, float xtest, float ytest);
void	define_surface_units(SURFACE sfc, const USPEC *uspec);
void	change_surface_units(SURFACE sfc, const USPEC *uspec);
void	recall_surface_units(SURFACE sfc, USPEC **uspec);
void	define_surface_conspecs(SURFACE sfc, int ncspec, CONSPEC *cspecs);
void	recall_surface_conspecs(SURFACE sfc, int *ncspec, CONSPEC **cspecs);
void	add_conspec_to_surface(SURFACE sfc, CONSPEC *cspec);
void	invoke_surface_conspecs(SURFACE sfc);
void	highlight_surface(SURFACE sfc, HILITE code);
void	change_surface_pspec(SURFACE sfc, PPARAM param, POINTER value);
void	recall_surface_pspec(SURFACE sfc, PPARAM param, POINTER value);

/* Declare all functions in surface_eval.c */
LOGICAL	eval_sfc(SURFACE sfc, POINT pos, double *val);
LOGICAL	eval_sfc_UV(SURFACE sfc, POINT pos, double *uval, double *vval);
LOGICAL	eval_sfc_MD(SURFACE sfc, POINT pos, double *mag, double *dir);
LOGICAL	eval_sfc_1st_deriv(SURFACE sfc, POINT pos, double *valx, double *valy);
LOGICAL	eval_sfc_2nd_deriv(SURFACE sfc, POINT pos,
						double *valxx, double *valxy, double *valyy);
LOGICAL	eval_sfc_curvature(SURFACE sfc, POINT pos, double *curv, POINT centre);
LOGICAL	eval_sfc_comp_1st_deriv(SURFACE sfc, POINT pos, int comp,
						double *valx, double *valy);
LOGICAL	eval_sfc_comp_2nd_deriv(SURFACE sfc, POINT pos, int comp,
						double *valxx, double *valxy, double *valyy);
LOGICAL	eval_sfc_unmapped(SURFACE sfc, POINT pos, double *val);
LOGICAL	eval_sfc_UV_unmapped(SURFACE sfc, POINT pos,
						double *uval, double *vval);
LOGICAL	eval_sfc_MD_unmapped(SURFACE sfc, POINT pos, double *mag, double *dir);
STRING	eval_sfc_feature(SURFACE sfc, POINT pos, STRING features,
						POINT plab, char *which, ITEM *item, LOGICAL *valid);

/* Declare all functions in surface_cont.c */
void	reset_surface_mask(SURFACE sfc);
void	add_surface_mask(SURFACE sfc, SET set, STRING usub, STRING subelem,
						STRING mode, STRING rule);
void	contour_surface(SURFACE sfc);
void	band_contour_surface(SURFACE sfc);
void	contour_surface_partial(SURFACE sfc,
						int ipl, int ipr, int ipb, int ipt);
void	redefine_surface_patches(SURFACE sfc,
						int ipl, int ipr, int ipb, int ipt, LOGICAL force);
void	find_surface_range(SURFACE sfc, float *vmin, float *vmax);
LOGICAL	contour_curveset_failure(void);
SET		contour_curveset(SURFACE sfc, float cval, USPEC *uspec);
SET		contour_areaset(SURFACE sfc, float lower, float upper, USPEC *uspec,
						BOX *box);
LOGICAL	contour_areaset_failure(void);
SET		contour_areaset_from_curves(SET lcurves, SET ucurves, BOX *box);

/* Declare all functions in surface_oper.c */
void	scalar_grid(GRID *gridd, float gridlen, int nx, int ny, SCALAR *scalr);
void	spline_grid(GRID *gridd, float gridlen, int nx, int ny, SPLINE *spln);
void	grid_buffer_control(STRING mode);
void	grid_surface(SURFACE sfc, float gridlen, int nx, int ny, float **vals);
void	grid_surface_2D(SURFACE sfc, float gridlen, int nx, int ny,
						float **xvals, float **yvals);
void	grid_spline(SPLINE *spln, float gridlen, int nx, int ny, float **vals);
void	grid_spline_2D(SPLINE *spln, float gridlen, int nx, int ny,
						float **xvals, float **yvals);
void	grid_spline_mag(SPLINE *spln, float gridlen, int nx, int ny,
						float **vals);
void	edit_surface(SURFACE sfc, POINT centre, float value,
						LOGICAL absolute, LOGICAL recontour);
void	edit_surface_2D(SURFACE sfc, POINT centre, float xvalue, float yvalue,
						LOGICAL absolute, LOGICAL recontour);
int		fit_surface(SURFACE sfc, int npts, POINT *points, float *values,
						LOGICAL absolute, LOGICAL recontour);
int		sfit_surface(SURFACE sfc, int npts, POINT *points, float *values,
						float influence, float weighting,
						LOGICAL absolute, LOGICAL recontour);
int		fit_surface_2D(SURFACE sfc, int npts, POINT *points, float *xvalues,
						float *yvalues, LOGICAL absolute, LOGICAL recontour);
int		sfit_surface_2D(SURFACE sfc, int npts, POINT *points, float *xvalues,
						float *yvalues, float influence, float weighting,
						LOGICAL absolute, LOGICAL recontour);
void	remap_grid(GRID *gridd, const MAP_PROJ *smproj, const MAP_PROJ *tmproj);
void	remap_surface(SURFACE sfc, const MAP_PROJ *smproj,
						const MAP_PROJ *tmproj);
LOGICAL	reproject_surface(SURFACE sfc, const MAP_PROJ *smproj,
						const MAP_PROJ *tmproj, const GRID_DEF *gdef);
LOGICAL	reproject_surface_2D(SURFACE sfc, const MAP_PROJ *smproj,
						const MAP_PROJ *tmproj, const GRID_DEF *gdef);
LOGICAL	reproject_xy_surfaces(SURFACE usfc, SURFACE vsfc,
						const MAP_PROJ *smproj, const MAP_PROJ *tmproj);
SURFACE	build_surface_2D(SURFACE usfc, SURFACE vsfc,
						const MAP_PROJ *smproj, const MAP_PROJ *tmproj);
LOGICAL	evaluation_surface(SURFACE sfc, const MAP_PROJ *smproj,
						const MAP_PROJ *tmproj);
LOGICAL	set_grid_from_surface(SURFACE sfc, MAP_PROJ *mproj);
SURFACE	merge_surfaces(int numsfc, SURFACE *sfcs, const MAP_PROJ *mproj,
						int numsrc, const MAP_PROJ *smprojs,
						int *nscmp, int **scmps);

/* Declare all functions in surface_mem.c */
PATCH	get_sfc_patch(SURFACE sfc, int iu, int iv);
PATCH	prepare_sfc_patch(SURFACE sfc, int iu, int iv);
PATCH	dispose_sfc_patch(SURFACE sfc, int iu, int iv);
PATCH	destroy_sfc_patch(SURFACE sfc, int iu, int iv);
PATCH	redef_sfc_patch(SURFACE sfc, int iu, int iv, LOGICAL force);
ILIST	prepare_sfc_ulist(SURFACE sfc, int iu, int iv);
ILIST	destroy_sfc_ulist(SURFACE sfc, int iu, int iv);
ILIST	prepare_sfc_vlist(SURFACE sfc, int iu, int iv);
ILIST	destroy_sfc_vlist(SURFACE sfc, int iu, int iv);

/* Now it has been included */
#endif
