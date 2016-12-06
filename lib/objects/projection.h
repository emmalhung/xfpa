/*********************************************************************/
/** @file projection.h
 *
 * PROJECTION object definitions (include file)
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    p r o j e c t i o n . h                                           *
*                                                                      *
*    Map projection specifications (include file)                      *
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
#ifndef PROJ_DEFS
#define PROJ_DEFS

/* Need things from misc.h */
#include "misc.h"

/******************************************************************************
*
*   Define objects
*
******************************************************************************/

/** New "MAP_DEF" structure:  Orientation and extent regardless of projection */
typedef struct
	{
	float	olat, olon;		/**< origin lat-lon */
	float	lref;			/**< reference lon parallel to y-axis */
	float	xorg, yorg;		/**< location of origin in local co-ords */
	float	xlen, ylen;		/**< map extent */
	float	units;			/**< units of distance for all x and y parameters */
	} MAP_DEF;

/** Define "GRID_DEF" structure:  Grid size and spacing */
typedef struct
	{
	int		nx, ny;			/**< grid dimensions (# of grid points) */
	float	gridlen;		/**< grid spacing (square) */
	float	xgrid, ygrid;	/**< grid spacing (rectangular: set gridlen=0) */
	float	units;			/**< units of distance for grid spacing */
	} GRID_DEF;

/** Define known projection types */
typedef	enum
	{
	ProjectNone,			/**< no projection (orthographic) */
	ProjectLatLon,			/**< lat-lon (special case of plate-caree) */
	ProjectLatLonAng,		/**< rotated lat-lon */
	ProjectPlateCaree,		/**< plate-caree (flat square) */
	ProjectRectangular,		/**< carte parallelogrammatique by an easier to use name */
	ProjectMercatorEq,		/**< Mercator equatorial */
	ProjectPolarSt,			/**< polar stereographic (north or south) */
	ProjectObliqueSt,		/**< oblique stereographic (also equatorial) */
	ProjectLambertConf		/**< Lambert conformal (1 or 2 ref latitudes) */
	} PROJ_TYPE;
/** Projection list structure */
typedef struct
	{
	STRING		name;		/**< projection name string */
	PROJ_TYPE	type;		/**< matching projection type */
	} PROJ_LIST;

/** Define "PROJ_DEF" structure:  Projection type and ref info */
typedef struct
	{
	PROJ_TYPE	type;		/**< projection type */
	float		ref[5];		/**< list of input reference values */
	double		parm[5];	/**< list of pre-calculated constants */
	} PROJ_DEF;

/** New "MAP_PROJ" structure:  Orientation and extent together with projection */
typedef struct
	{
	PROJ_DEF	projection;	/**< projection type */
	MAP_DEF		definition;	/**< map definition */
	GRID_DEF	grid;		/**< default grid definition */
	POINT		origin;		/**< location of map origin w.r.t projection origin */
	float		clat;		/**< latitude of map centre */
	float		clon;		/**< longitude of map centre */
	} MAP_PROJ;

/******************************************************************************
*
*   Initialize global data in master module only.
*   Import global data anywhere else.
*
******************************************************************************/

#undef GLOBAL
#ifdef PROJECTION_INIT
#   define GLOBAL GLOBAL_INIT
#else
#   define GLOBAL GLOBAL_EXTERN
#endif

/* Seup assorted useful constants */
#define NullMapDef  NullPtr(MAP_DEF *)
#define NullGridDef NullPtr(GRID_DEF *)
#define NullProjDef NullPtr(PROJ_DEF *)
#define NullMapProj NullPtr(MAP_PROJ *)

#define	NO_MAPDEF	{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}
#define	NO_GRIDDEF	{0, 0, 0.0, 0.0, 0.0, 0.0}
#define	NO_PROJDEF	{ProjectNone, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}
#define	NO_MAPPROJ	{NO_PROJDEF, NO_MAPDEF, NO_GRIDDEF, {0.0, 0.0}, 0.0, 0.0}

#define	FpaComponentFlag	1

/* Initialize/import global data */
GLOBAL( const MAP_DEF,	NoMapDef       , NO_MAPDEF  );
GLOBAL( const GRID_DEF,	NoGridDef      , NO_GRIDDEF );
GLOBAL( const PROJ_DEF,	NoProjDef      , NO_PROJDEF );
GLOBAL(       PROJ_DEF,	DefaultProjDef , NO_PROJDEF );
GLOBAL( const MAP_PROJ,	NoMapProj      , NO_MAPPROJ );

/* Initialize internal lists */
#ifdef PROJECTION_INIT
static	const	PROJ_LIST	ProjectionList[] =
	{
		{ "none",						ProjectNone },
		{ "",							ProjectNone },
		{ "radar",						ProjectNone },
		{ "latitude_longitude",			ProjectLatLon },
		{ "lat_lon",					ProjectLatLon },
		{ "rotated_lat_lon",			ProjectLatLonAng },
		{ "plate_caree",				ProjectPlateCaree },
		{ "rectangular",				ProjectRectangular },
		{ "carte_parallelogrammatique",	ProjectRectangular },
		{ "mercator_equatorial",		ProjectMercatorEq },
		{ "polar_stereographic",		ProjectPolarSt },
		{ "oblique_stereographic",		ProjectObliqueSt },
		{ "stereographic",				ProjectObliqueSt },
		{ "lambert_conformal",			ProjectLambertConf },
		{ "lambert_conformal_secant",	ProjectLambertConf },
		{ "lambert_conformal_2lat",		ProjectLambertConf },
		{ "lambert_conformal_tangent",	ProjectLambertConf },
		{ "lambert_conformal_1lat",		ProjectLambertConf }
	};

static	const	int			NumProjectionList =
	(int) (sizeof(ProjectionList) / sizeof(PROJ_LIST));
#endif

/******************************************************************************
*
*   Declare functions.
*
******************************************************************************/

/* Declare functions in projection.c */
LOGICAL		define_map_def(MAP_DEF *mdef, STRING olat, STRING olon, STRING rlon,
						float xmin, float xmax, float ymin, float ymax,
						float map_units);
void		copy_map_def(MAP_DEF *mnew, const MAP_DEF *mdef);
void		normalize_map_def(MAP_DEF *mdef);
void		set_map_def_units(MAP_DEF *mdef, float map_units);
LOGICAL		same_map_def(const MAP_DEF *mdef1, const MAP_DEF *mdef2);
LOGICAL		equivalent_map_def(const MAP_DEF *mdef1, const MAP_DEF *mdef2);
LOGICAL		inside_map_def(const MAP_DEF *mdef, POINT pos);
LOGICAL		inside_map_def_xy(const MAP_DEF *mdef, float x, float y);
LOGICAL		clip_to_map_def(const MAP_DEF *mdef, const POINT pin, POINT pout);
LOGICAL		clip_to_map_def_xy(const MAP_DEF *mdef, float xin, float yin,
						float *xout, float *yout);

LOGICAL		define_grid_def(GRID_DEF *gdef, int nx, int ny, float grid,
						float xgrid, float ygrid, float map_units);
void		copy_grid_def(GRID_DEF *gnew, const GRID_DEF *gdef);
void		set_grid_def_units(GRID_DEF *gdef, float map_units);
LOGICAL		same_grid_def(const GRID_DEF *gdef1, const GRID_DEF *gdef2);
LOGICAL		equivalent_grid_def(const GRID_DEF *gdef1, const GRID_DEF *gdef2);
LOGICAL		inside_grid_def(const GRID_DEF *gdef, POINT pos);
LOGICAL		inside_grid_def_xy(const GRID_DEF *gdef, float x, float y);

PROJ_TYPE	which_projection_type(STRING name);
STRING		which_projection_name(PROJ_TYPE type);
LOGICAL		define_projection_by_name(PROJ_DEF *pdef, STRING name,
						STRING p1, STRING p2, STRING p3, STRING p4, STRING p5);
LOGICAL		projection_info(PROJ_DEF *pdef, STRING *name, STRING *p1,
						STRING *p2, STRING *p3, STRING *p4, STRING *p5);
LOGICAL		define_projection(PROJ_DEF *pdef, PROJ_TYPE type,
						float r1, float r2, float r3, float r4, float r5);
void		copy_projection(PROJ_DEF *pnew, const PROJ_DEF *pdef);
LOGICAL		same_projection(const PROJ_DEF *pdef1, const PROJ_DEF *pdef2);

void		define_map_projection(MAP_PROJ *mproj, const PROJ_DEF *pdef,
						const MAP_DEF *mdef, const GRID_DEF *gdef);
void		copy_map_projection(MAP_PROJ *mpnew, const MAP_PROJ *mproj);
void		set_map_projection_units(MAP_PROJ *mproj, float map_units);
LOGICAL		same_map_projection(const MAP_PROJ *mp1, const MAP_PROJ *mp2);
LOGICAL		equivalent_map_projection(const MAP_PROJ *mp1, const MAP_PROJ *mp2);
LOGICAL		wrap_map_projection(const MAP_PROJ *mproj, LOGICAL *wrap_x,
						int *wrap_i);
LOGICAL		complete_map_projection(const MAP_PROJ *mproj);

LOGICAL		pos_to_grid(const MAP_PROJ *mproj, POINT pos, POINT gpos);
LOGICAL		grid_to_pos(const MAP_PROJ *mproj, POINT gpos, POINT pos);
LOGICAL		ll_to_pos(const MAP_PROJ *mproj, float lat, float lon, POINT pos);
LOGICAL		pos_to_ll(const MAP_PROJ *mproj, POINT pos, float *lat, float *lon);
LOGICAL		pos_to_pos(const MAP_PROJ *mp1, POINT pos1,
						const MAP_PROJ *mp2, POINT pos2);
LOGICAL		ll_distort(const MAP_PROJ *mproj, float lat, float lon,
						float *scalex, float *scaley);
LOGICAL		pos_distort(const MAP_PROJ *mproj, POINT pos,
						float *scalex, float *scaley);
LOGICAL		map_to_map(const MAP_PROJ *mp1, POINT org1, float ang1,
						const MAP_PROJ *mp2, POINT org2, float *ang2);
float		wind_dir_true(const MAP_PROJ *mproj,
						float lat, float lon, float xyang);
float		wind_dir_xy(const MAP_PROJ *mproj,
						float lat, float lon, float trueang);
float		great_circle_distance(const MAP_PROJ *mproj, POINT spos, POINT epos);
float		great_circle_bearing(const MAP_PROJ *mproj, POINT spos, POINT epos);
LOGICAL		great_circle_span(const MAP_PROJ *mproj, POINT spos,
						float direction, float distance, POINT epos);

/* Declare functions in project_oper.c */
LOGICAL		scaled_map_projection(MAP_PROJ *smproj,
						const MAP_PROJ *mproj, float scale);
LOGICAL		evaluation_map_projection(const MAP_PROJ *mproj, int npos,
						POINT *ppos, MAP_PROJ *mpeval, POINT **poseval);
int			closest_map_projection(float lat, float lon, int num,
						const MAP_PROJ *mplist, LOGICAL *inside, LOGICAL in);
LOGICAL		pos_to_edge_grid_pos(const MAP_PROJ *tmproj, POINT tpos,
						const MAP_PROJ *smproj, POINT edgepos);
LOGICAL		grid_positions(const MAP_PROJ *mproj, int *numx, int *numy,
						float *gridlen, POINT ***gpos,
						float ***glat, float ***glon);
LOGICAL		grid_center(const MAP_PROJ *mproj,
						POINT *cpos, float *clat, float *clon);
LOGICAL		grid_component_coefficients(int compflag,
						const MAP_PROJ *mprojin, COMPONENT compin,
						const MAP_PROJ *mprojout, COMPONENT compout,
						float ***coefficients);

/* Now it has been included */
#endif
