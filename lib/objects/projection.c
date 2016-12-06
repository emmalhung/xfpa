/*********************************************************************/
/**	@file projection.c
 *
 * Routines to handle objects and structures related to map
 * projections.
 *
 * These are the available projections:
 *	- No Projection (orthographic)
 *		- no parameters required.
 *	- Lat-Lon (special case of plate-caree)
 *		- no parameters required.
 *	- Rotated Lat-Lon
 *		- p1=Lat of natural origin,
 *		- p2=Lon of natural origin,
 *		- p3=Angle.
 *	- Plate-Caree (flat square)
 *		- no parameters required.
 *	- Mercator Equatorial
 *		- no parameters required.
 *	- Polar Stereographic (north or south)
 *		- p1=reference pole (N|S),
 *		- p2=true lat.
 *	- Oblique Stereographic (also equatorial)
 *		- p1=Lat of natural origin,
 *		- p2=Lon of natural origin,
 *		- p3=Angle.
 *	- Lambert Conformal (1 or 2 ref latitudes)
 *		- p1=First standard parallel.
 *		- p2=Second standard parallel.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*     p r o j e c t i o n . c                                          *
*                                                                      *
*     Routines to handle objects and structures related to map         *
*     projections.                                                     *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
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

#define PROJECTION_INIT
#include "projection.h"

#include <tools/tools.h>
#include <fpa_math.h>
#include <fpa_macros.h>

#undef DEBUG_GREATCIRCLE

/* Internal static functions */

static	LOGICAL	lln_ll_xy(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	lln_ll_xyu(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	lln_xy_ll(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	lln_xy_llu(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	lln_distort(const MAP_PROJ *, float, float, float *, float *);

static	LOGICAL	llr_ll_xy(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	llr_ll_xyu(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	llr_xy_ll(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	llr_xy_llu(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	llr_distort(const MAP_PROJ *, float, float, float *, float *);

static	LOGICAL	pc_ll_xy(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	pc_ll_xyu(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	pc_xy_ll(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	pc_xy_llu(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	pc_distort(const MAP_PROJ *, float, float, float *, float *);

static	LOGICAL	rec_ll_xy(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	rec_ll_xyu(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	rec_xy_ll(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	rec_xy_llu(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	rec_distort(const MAP_PROJ *, float, float, float *, float *);

static	LOGICAL	meq_ll_xy(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	meq_ll_xyu(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	meq_xy_ll(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	meq_xy_llu(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	meq_distort(const MAP_PROJ *, float, float, float *, float *);

static	LOGICAL	ps_ll_xy(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	ps_ll_xyu(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	ps_xy_ll(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	ps_xy_llu(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	ps_distort(const MAP_PROJ *, float, float, float *, float *);

static	LOGICAL	os_ll_xy(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	os_ll_xyu(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	os_xy_ll(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	os_xy_llu(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	os_distort(const MAP_PROJ *, float, float, float *, float *);

static	LOGICAL	lc_ll_xy(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	lc_ll_xyu(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	lc_xy_ll(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	lc_xy_llu(const MAP_PROJ *, float, float, float *, float *);
static	LOGICAL	lc_distort(const MAP_PROJ *, float, float, float *, float *);

/* Define useful internal constants */
/* RE = radius of Earth (m) */
#define	RE 6367650.0

/* Define useful shorthands for projection parameters */
#define ProjectPole    ref[0]
#define ProjectTrueLat ref[1]
#define ProjectPlat    ref[0]
#define ProjectPlon    ref[1]
#define ProjectSecant  ref[2]
#define ProjectOrigin  ref[2]
#define ProjectPdist   parm[0]
#define ProjectSinPlat parm[1]
#define ProjectCosPlat parm[2]
#define ProjectLat1    ref[0]
#define ProjectLat2    ref[1]
#define ProjectSinPhi0 parm[0]
#define ProjectRho1    parm[1]
#define ProjectPsi     parm[2]

/***********************************************************************
*                                                                      *
*      d e f i n e _ m a p _ d e f                                     *
*      c o p y _ m a p _ d e f                                         *
*      n o r m a l i z e _ m a p _ d e f                               *
*      s e t _ m a p _ d e f _u n i t s                                *
*      s a m e _ m a p _ d e f                                         *
*      e q u i v a l e n t _ m a p _ d e f                             *
*      i n s i d e _ m a p _ d e f                                     *
*      i n s i d e _ m a p _ d e f _ x y                               *
*      c l i p _ t o _ m a p _ d e f                                   *
*      c l i p _ t o _ m a p _ d e f _ x y                             *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set a map definition with the given data.
 *
 *	@param[in] 	*m		map def to set
 *	@param[in] 	olat	origin latitude
 *	@param[in] 	olon	origin longitude
 *	@param[in] 	rlon	reference longitude (vertical)
 *	@param[in] 	xmin	x minimum
 *	@param[in] 	ymin	y minumum
 *	@param[in] 	xmax	x maximum
 *	@param[in] 	ymax	y maximum
 *	@param[in] 	units	meters per unit
 *  @return True if successful.
 *********************************************************************/
LOGICAL	define_map_def

	(
	MAP_DEF	*m,
	STRING	olat,
	STRING	olon,
	STRING	rlon,
	float	xmin,
	float	ymin,
	float	xmax,
	float	ymax,
	float	units
	)

	{
	LOGICAL		argOK, valid = TRUE;

	/* Error return for missing parameters */
	if (!m) return FALSE;

	/* Define the map definition parameters */
	m->olat  = read_lat(olat, &argOK);	if ( !argOK ) valid = FALSE;
	m->olon  = read_lon(olon, &argOK);	if ( !argOK ) valid = FALSE;
	m->lref  = read_lon(rlon, &argOK);	if ( !argOK ) valid = FALSE;
	m->xorg  = -xmin;
	m->yorg  = -ymin;
	m->xlen  = xmax - xmin;
	m->ylen  = ymax - ymin;
	m->units = units;

	/* Reset to empty map definition if any problems */
	if ( !valid ) copy_map_def(m, &NoMapDef);

	/* Return the map definition */
	return valid;
	}

/*********************************************************************/
/**	Copy the given map definition.
 *
 *	@param[out]	*m1	copy definition
 *	@param[in] 	*m2	original
 *********************************************************************/
void	copy_map_def

	(
	MAP_DEF			*m1,
	const MAP_DEF	*m2
	)

	{
	/* Error return for missing parameters */
	if (!m1) return;
	if (!m2) return;

	m1->olat  = m2->olat;
	m1->olon  = m2->olon;
	m1->lref  = m2->lref;
	m1->xorg  = m2->xorg;
	m1->yorg  = m2->yorg;
	m1->xlen  = m2->xlen;
	m1->ylen  = m2->ylen;
	m1->units = m2->units;
	}

/*********************************************************************/
/** Normalize a map definition.
 *
 * Make sure all longitudes are between -180 and 180.
 *
 *	@param[in] 	*m	definition to normalize
 *********************************************************************/
void	normalize_map_def

	(
	MAP_DEF	*m
	)

	{
	/* Error return for missing parameters */
	if (!m) return;

	while (m->olon <= -180) m->olon += 360;
	while (m->olon >   180) m->olon -= 360;
	while (m->lref <= -180) m->lref += 360;
	while (m->lref >   180) m->lref -= 360;
	}

/*********************************************************************/
/**	 Set or reset the given map definition's units. And fix any
 * attributes that may be affected.
 *
 *	@param[in] 	*m		given map def
 *	@param[in] 	units	meters per map unit
 *********************************************************************/
void	set_map_def_units

	(
	MAP_DEF	*m,
	float	units
	)

	{
	float	fact;

	/* Error return for missing parameters */
	if (!m)                return;
	if (m->units == units) return;
	if (m->units <= 0)     return;
	if (units <= 0)        return;

	fact = m->units / units;
	m->xorg *= fact;
	m->yorg *= fact;
	m->xlen *= fact;
	m->ylen *= fact;
	m->units = units;
	}

/*********************************************************************/
/** Compare two map defs to determine if they are the same.
 *
 *	@param[in]  *m1	first map def to consider
 *	@param[in]  *m2	second map def to consider
 * 	@return True if map defs are the same.
 *********************************************************************/
LOGICAL	same_map_def

	(
	const MAP_DEF	*m1,
	const MAP_DEF	*m2
	)

	{
	/* Error return for missing parameters */
	if (!m1 && !m2) return TRUE;
	if (!m1) return FALSE;
	if (!m2) return FALSE;

	if ( !fcompare(m1->olat,  m2->olat,  90.0, 1e-5) ) return FALSE;
	if ( !fcompare(m1->olon,  m2->olon,  90.0, 1e-5) ) return FALSE;
	if ( !fcompare(m1->lref,  m2->lref,  90.0, 1e-5) ) return FALSE;
	if ( !fcompare(m1->xorg,  m2->xorg,   1.0, 1e-5) ) return FALSE;
	if ( !fcompare(m1->yorg,  m2->yorg,   1.0, 1e-5) ) return FALSE;
	if ( !fcompare(m1->xlen,  m2->xlen,   1.0, 1e-5) ) return FALSE;
	if ( !fcompare(m1->ylen,  m2->ylen,   1.0, 1e-5) ) return FALSE;
	if ( !fcompare(m1->units, m2->units,  1.0, 1e-5) ) return FALSE;

	return TRUE;
	}

/*********************************************************************/
/** Compare two map defs to determine if they are equivalent. This
 * comparison takes into account different units.
 *
 * @return True if map defs are equivalent.
 *********************************************************************/
LOGICAL	equivalent_map_def

	(
	const MAP_DEF	*m1,
	const MAP_DEF	*m2
	)

	{
	float	u1, u2;

	/* Error return for missing parameters */
	if (!m1 && !m2) return TRUE;
	if (!m1) return FALSE;
	if (!m2) return FALSE;

	u1 = m1->units;
	u2 = m2->units;
	if (u1==u2) return same_map_def(m1, m2);

	if ( !fcompare(m1->olat,    m2->olat,    90.0, 1e-5) ) return FALSE;
	if ( !fcompare(m1->olon,    m2->olon,    90.0, 1e-5) ) return FALSE;
	if ( !fcompare(m1->lref,    m2->lref,    90.0, 1e-5) ) return FALSE;
	if ( !fcompare(m1->xorg*u1, m2->xorg*u2,  1.0, 1e-5) ) return FALSE;
	if ( !fcompare(m1->yorg*u1, m2->yorg*u2,  1.0, 1e-5) ) return FALSE;
	if ( !fcompare(m1->xlen*u1, m2->xlen*u2,  1.0, 1e-5) ) return FALSE;
	if ( !fcompare(m1->ylen*u1, m2->ylen*u2,  1.0, 1e-5) ) return FALSE;

	return TRUE;
	}

/*********************************************************************/
/** Check if a given point is inside a given map def.
 *
 *	@param[in]  *m	given map def
 *	@param[in] 	p	point to query
 *  @return True if the point is inside the map def.
 *********************************************************************/
LOGICAL	inside_map_def

	(
	const MAP_DEF	*m,
	POINT			p
	)

	{
	/* Error return for missing parameters */
	if (!m) return FALSE;
	if (!p) return FALSE;

	return inside_map_def_xy(m, p[X], p[Y]);
	}

/*********************************************************************/
/** Check if a given x-y pair is inside a given map def.
 *
 *	@param[in]  *m	given map def
 *	@param[in] 	x	x coord
 *	@param[in] 	y	y coord
 *  @return True if the point is inside the map def.
 *********************************************************************/
LOGICAL	inside_map_def_xy

	(
	const MAP_DEF	*m,
	float			x,
	float			y
	)

	{
	/* Error return for missing parameters */
	if (!m) return FALSE;

	x *= .999;
	y *= .999;
	if (x < 0)       return FALSE;
	if (x > m->xlen) return FALSE;
	if (y < 0)       return FALSE;
	if (y > m->ylen) return FALSE;
	return TRUE;
	}

/*********************************************************************/
/** Clip point to map def. If point falls outside of the map it is moved
 * to the closest location on the map.
 *
 *	@param[in]  *m	given map def
 * 	@param[in]  p	point to clip
 *	@param[in]  pos	new position on map
 *  @return True if successful.
 *********************************************************************/
LOGICAL	clip_to_map_def

	(
	const MAP_DEF	*m,
	const POINT		p,
	POINT			pos
	)

	{
	/* Error return for missing parameters */
	if (!m)   return FALSE;
	if (!p)   return FALSE;
	if (!pos) return FALSE;

	return clip_to_map_def_xy(m, p[X], p[Y], &pos[X], &pos[Y]);
	}

/*********************************************************************/
/** Clip x-y pair to map def. If point falls outside of the map it is moved
 * to the closest location on the map.
 *
 *	@param[in]  *m	given map def
 *	@param[in] 	x	x coord to clip
 *	@param[in] 	y	y coord to clip
 *	@param[out]	*cx	clipped x coord
 *	@param[out]	*cy	clipped y coord
 *  @return True if successful.
 *********************************************************************/
LOGICAL	clip_to_map_def_xy

	(
	const MAP_DEF	*m,
	float			x,
	float			y,
	float			*cx,
	float			*cy
	)

	{
	if (cx) *cx = 0.0;
	if (cy) *cy = 0.0;

	/* Error return for missing parameters */
	if (!m)  return FALSE;

	if (cx)
		{
		if      (x < 0.0)     *cx = 0.0;
		else if (x > m->xlen) *cx = m->xlen;
		else                  *cx = x;
		}
	if (cy)
		{
		if      (y < 0.0)     *cy = 0.0;
		else if (y > m->ylen) *cy = m->ylen;
		else                  *cy = y;
		}
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ g r i d _ d e f                                   *
*      c o p y _ g r i d _ d e f                                       *
*      s e t _ g r i d _ d e f _ u n i t s                             *
*      s a m e _ g r i d _ d e f                                       *
*      e q u i v a l e n t _ g r i d _ d e f                           *
*      i n s i d e _ g r i d _ d e f                                   *
*      i n s i d e _ g r i d _ d e f _ x y                             *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Define a grid def. If grid is non zero then a equally space
 * grid is defined otherwise xgrid and ygrid define the dimentions in
 * the x and y directions respectively.
 *
 *	@param[out]	*g		Grid def to define
 *	@param[in] 	nx		number of x points
 *	@param[in] 	ny		number of y points
 *	@param[in] 	grid	grid spacing x and y
 *	@param[in] 	xgrid	grid spacing x direction
 *	@param[in] 	ygrid	grid spacing y direcition
 *	@param[in] 	units	meters per unit
 *  @return True if successful.
 *********************************************************************/
LOGICAL	define_grid_def

	(
	GRID_DEF	*g,
	int			nx,
	int			ny,
	float		grid,
	float		xgrid,
	float		ygrid,
	float		units
	)

	{
	/* Error return for missing parameters */
	if (!g) return FALSE;

	/* Return empty grid definition if any problems */
	if ( nx <= 0 || ny <= 0
			|| ( grid == 0.0 && xgrid == 0.0 && ygrid == 0.0 ) || units <= 0.0 )
		{
		copy_grid_def(g, &NoGridDef);
		return FALSE;
		}

	/* Define the grid definition parameters */
	g->nx = nx;
	g->ny = ny;
	if ( grid > 0.0 )
		{
		g->gridlen = grid;
		g->xgrid   = 0.0;
		g->ygrid   = 0.0;
		}
	else
		{
		g->gridlen = 0.0;
		g->xgrid   = xgrid;
		g->ygrid   = ygrid;
		}
	g->units = units;

	/* Return the grid definition */
	return TRUE;
	}

/*********************************************************************/
/** Copy one grid def to another.
 *
 *	@param[out]	*g1		Copy of grid
 *	@param[in] 	*g2		Original
 *********************************************************************/
void	copy_grid_def

	(
	GRID_DEF		*g1,
	const GRID_DEF	*g2
	)

	{
	/* Error return for missing parameters */
	if (!g1) return;
	if (!g2) return;

	g1->nx      = g2->nx;
	g1->ny      = g2->ny;
	g1->gridlen = g2->gridlen;
	g1->xgrid   = g2->xgrid;
	g1->ygrid   = g2->ygrid;
	g1->units   = g2->units;
	}

/*********************************************************************/
/** Set or reset the grid def units. Recalculate any dependant
 * values.
 *
 *	@param[in] 	*g		given grid def
 *	@param[in] 	units	new units (meters per grid space
 *********************************************************************/
void	set_grid_def_units

	(
	GRID_DEF	*g,
	float		units
	)

	{
	float	fact;

	/* Error return for missing parameters */
	if (!g)                return;
	if (g->units == units) return;
	if (g->units <= 0)     return;
	if (units <= 0)        return;

	fact = g->units / units;
	g->gridlen *= fact;
	g->xgrid   *= fact;
	g->ygrid   *= fact;
	g->units    = units;
	}

/*********************************************************************/
/** Compare grid defs to determine if they are the same.
 *
 *	@param[in]  *g1	first grid to compare
 *	@param[in]  *g2	second grid to compare
 *  @return True if definitions are the same.
 *********************************************************************/
LOGICAL	same_grid_def

	(
	const GRID_DEF	*g1,
	const GRID_DEF	*g2
	)

	{
	/* Error return for missing parameters */
	if (!g1 && !g2) return TRUE;
	if (!g1) return FALSE;
	if (!g2) return FALSE;

	if (g1->nx      != g2->nx)      return FALSE;
	if (g1->ny      != g2->ny)      return FALSE;

	if ( !fcompare(g1->gridlen, g2->gridlen,  1.0, 1e-5) ) return FALSE;
	if (g1->gridlen <= 0)
		{
		if ( !fcompare(g1->xgrid, g2->xgrid,  1.0, 1e-5) ) return FALSE;
		if ( !fcompare(g1->ygrid, g2->ygrid,  1.0, 1e-5) ) return FALSE;
		}
	if ( !fcompare(g1->units,   g2->units,    1.0, 1e-5) ) return FALSE;

	return TRUE;
	}

/*********************************************************************/
/** Compare grid defs to determine if they are the equivalent. Units
 * are taken into consideration.
 *
 *	@param[in] 	*g1	first grid to compare
 *	@param[in] 	*g2	second grid to compare
 *  @return True if definitions are the equivalent.
 *********************************************************************/
LOGICAL	equivalent_grid_def

	(
	const GRID_DEF	*g1,
	const GRID_DEF	*g2
	)

	{
	float	u1, u2;

	/* Error return for missing parameters */
	if (!g1 && !g2) return TRUE;
	if (!g1) return FALSE;
	if (!g2) return FALSE;

	u1 = g1->units;
	u2 = g2->units;
	if (u1==u2) return same_grid_def(g1, g2);

	if (g1->nx != g2->nx) return FALSE;
	if (g1->ny != g2->ny) return FALSE;
	if ( !fcompare(g1->gridlen*u1, g2->gridlen*u2,  1.0, 1e-5) ) return FALSE;
	if (g1->gridlen <= 0)
		{
		if ( !fcompare(g1->xgrid*u1, g2->xgrid*u2,  1.0, 1e-5) ) return FALSE;
		if ( !fcompare(g1->ygrid*u1, g2->ygrid*u2,  1.0, 1e-5) ) return FALSE;
		}

	return TRUE;
	}

/*********************************************************************/
/** Check if a given point is inside a given grid.
 *
 *	@param[in]  *g	given grid def
 *	@param[in] 	p	given point
 *  @return True if the point is inside the grid.
 *********************************************************************/
LOGICAL	inside_grid_def

	(
	const GRID_DEF	*g,
	POINT			p
	)

	{
	/* Error return for missing parameters */
	if (!g) return FALSE;
	if (!p) return FALSE;

	return inside_grid_def_xy(g, p[X], p[Y]);
	}

/*********************************************************************/
/** Check if a given x-y pair is inside a given grid.
 *
 *	@param[in]  *g	given grid
 *	@param[in] 	x	x coord
 *	@param[in] 	y	y coord
 *  @return True if the point is inside the grid.
 *********************************************************************/
LOGICAL	inside_grid_def_xy

	(
	const GRID_DEF	*g,
	float			x,
	float			y
	)

	{
	/* Error return for missing parameters */
	if (!g) return FALSE;

	x *= .999;
	y *= .999;
	if (x < 0)                        return FALSE;
	if (y < 0)                        return FALSE;
	if (g->gridlen > 0)
		{
		if (x > g->gridlen*(g->nx-1)) return FALSE;
		if (y > g->gridlen*(g->ny-1)) return FALSE;
		}
	else
		{
		if (x > g->xgrid*(g->nx-1))   return FALSE;
		if (y > g->ygrid*(g->ny-1))   return FALSE;
		}
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     w h i c h _ p r o j e c t i o n _ t y p e                        *
*     w h i c h _ p r o j e c t i o n _ n a m e                        *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Retrieve a projection type by name.
 *
 * @return Pointer to a Projection type. DO NOT Destroy this object
 * it is a static variable and is used elsewhere.
 *********************************************************************/
PROJ_TYPE	which_projection_type

	(
	STRING	name
	)

	{
	int		i;

	for (i=0; i<NumProjectionList; i++)
		{
		if (same(name, ProjectionList[i].name))
			return ProjectionList[i].type;
		}

	/* Didn't find it, return the default */
	return ProjectNone;
	}

/*********************************************************************/
/** Retrieve a projection name given a projection type.
 *
 *	@param[in] 	type	projection type to match
 *  @return Pointer to the name of the projection. DO NOT destroy this
 * 			object it is a static variable and is used elsewhere.
 *********************************************************************/
STRING	which_projection_name

	(
	PROJ_TYPE	type
	)

	{
	int		i;

	for (i=0; i<NumProjectionList; i++)
		{
		if (type == ProjectionList[i].type)
			return ProjectionList[i].name;
		}

	/* Didn't find it, return the default */
	return "none";
	}

/***********************************************************************
*                                                                      *
*     d e f i n e _ p r o j e c t i o n _ b y _ n a m e                *
*     p r o j e c t i o n _ i n f o                                    *
*     d e f i n e _ p r o j e c t i o n                                *
*     c o p y _ p r o j e c t i o n                                    *
*     s a m e _ p r o j e c t i o n                                    *
*                                                                      *
***********************************************************************/

/*ARGSUSED*/

/*********************************************************************/
/** Define projection of a type give by name.
 *
 *	@param[out]	*proj	Projection to define
 *	@param[in] 	name	name of projection
 *	@param[in] 	p1		parameter 1 (meaning depends on type of projection)
 *	@param[in] 	p2		parameter 2 (meaning depends on type of projection)
 *	@param[in] 	p3		parameter 3 (meaning depends on type of projection)
 *	@param[in] 	p4		parameter 4 (meaning depends on type of projection)
 *	@param[in] 	p5		parameter 5 (meaning depends on type of projection)
 *  @return True if successful.
 *********************************************************************/
LOGICAL	define_projection_by_name

	(
	PROJ_DEF	*proj,
	STRING		name,
	STRING		p1,
	STRING		p2,
	STRING		p3,
	STRING		p4,
	STRING		p5
	)

	{
	PROJ_TYPE	type;
	float		r1=0, r2=0, r3=0, r4=0, r5=0;
	int			i;
	LOGICAL		valid;

	/* Error return for missing parameters */
	if (!proj) return FALSE;

	/* Find the given projection name */
	for (i=0; i<NumProjectionList; i++)
		{
		if (same(name, ProjectionList[i].name))
			{
			type = ProjectionList[i].type;
			break;
			}
		}
	if (i >= NumProjectionList) return FALSE;

	/* Now interpret the reference parameters */
	switch (type)
		{
		/* No projection (orthographic) */
		case ProjectNone:			break;

		/* Lat-Lon */
		case ProjectLatLon:			break;

		/* Rotated (also transverse) Lat-Lon */
		case ProjectLatLonAng:		r1 = read_lat(p1, &valid);
									if (!valid)                 return FALSE;
									r2 = read_lon(p2, &valid);
									if (!valid)                 return FALSE;
									if (blank(p3))				break;
									r3 = read_ang(p3, &valid);
									if (!valid)                 return FALSE;
									break;

		/* Plate-Caree */
		case ProjectPlateCaree:		break;

		/* Rectangular (Carte Parallelogrammatique) */
		case ProjectRectangular:	r1 = read_lat(p1, &valid);
									if (!valid)					return FALSE;
									break;

		/* Mercator */
		case ProjectMercatorEq:		break;

		/* Polar stereographic */
		case ProjectPolarSt:		if (same(p1, "north"))      r1 = 90.0;
									else if (same(p1, "south")) r1 = -90.0;
									else                        return FALSE;
									r2 = read_lat(p2, &valid);
									if (!valid)                 return FALSE;
									break;

		/* Oblique (also equatorial) stereographic */
		case ProjectObliqueSt:		r1 = read_lat(p1, &valid);
									if (!valid)                 return FALSE;
									r2 = read_lon(p2, &valid);
									if (!valid)                 return FALSE;
									if (blank(p3))				break;
									r3 = read_ang(p3, &valid);
									if (!valid)                 return FALSE;
									break;

		/* Lambert conformal (secant or tangent) */
		case ProjectLambertConf:	r1 = read_lat(p1, &valid);
									if (!valid)                 return FALSE;
									r2 = read_lat(p2, &valid);
									if (!valid)                 r2 = r1;
									break;
		}

	/* Go ahead and define it */
	return define_projection(proj, type, r1,r2,r3,r4,r5);
	}

/*********************************************************************/
/** Retrive information of a given Projection. In particular its name
 * and associated parameters.
 *
 *	@param[in] 	*proj	given projection
 *	@param[out]	*pname	projection name
 *	@param[out]	*p1		parameter 1
 *	@param[out]	*p2		parameter 2
 *	@param[out]	*p3		parameter 3
 *	@param[out]	*p4		parameter 4
 *	@param[out]	*p5		parameter 5
 *  @return True if successful.
 *********************************************************************/
LOGICAL	projection_info

	(
	PROJ_DEF	*proj,
	STRING		*pname,
	STRING		*p1,
	STRING		*p2,
	STRING		*p3,
	STRING		*p4,
	STRING		*p5
	)

	{
	int		i;

	static	char	a1[20], a2[20], a3[20];

	/* Error return for missing parameters */
	if (!proj) return FALSE;

	if (pname) *pname = NULL;
	if (p1)    *p1 = NULL;
	if (p2)    *p2 = NULL;
	if (p3)    *p3 = NULL;
	if (p4)    *p4 = NULL;
	if (p5)    *p5 = NULL;
	if (!proj) return FALSE;

	/* Find the given projection type and get the first name */
	for (i=0; i<NumProjectionList; i++)
		{
		if (proj->type == ProjectionList[i].type)
			{
			if (pname) *pname = ProjectionList[i].name;
			break;
			}
		}
	if (i >= NumProjectionList) return FALSE;

	/* Now set the reference parameters */
	switch (proj->type)
		{
		/* No projection (orthographic) */
		case ProjectNone:			break;

		/* Lat-Lon */
		case ProjectLatLon:			break;

		/* Rotated Lat-Lon */
		case ProjectLatLonAng:		if (p1)
										{
										(void) sprintf(a1, "%g", proj->ref[0]);
										*p1 = a1;
										}
									if (p2)
										{
										(void) sprintf(a2, "%g", proj->ref[1]);
										*p2 = a2;
										}
									if (p3 && proj->ref[2] > 0.0)
										{
										(void) sprintf(a3, "%g", proj->ref[2]);
										*p3 = a3;
										}
									break;

		/* Plate-Caree */
		case ProjectPlateCaree:		break;

		/* Rectangular (Carte Parallelogrammatique) */
		case ProjectRectangular:	if (p1)
										{
										(void) sprintf(a1, "%g", proj->ref[0]);
										*p1 = a1;
										}
									break;


		/* Mercator */
		case ProjectMercatorEq:		break;

		/* Polar stereographic */
		case ProjectPolarSt:		if (p1)
										{
										if (proj->ref[0] > 0) *p1 = "north";
										else                  *p1 = "south";
										}
									if (p2)
										{
										(void) sprintf(a2, "%g", proj->ref[1]);
										*p2 = a2;
										}
									break;

		/* Oblique stereographic */
		case ProjectObliqueSt:		if (p1)
										{
										(void) sprintf(a1, "%g", proj->ref[0]);
										*p1 = a1;
										}
									if (p2)
										{
										(void) sprintf(a2, "%g", proj->ref[1]);
										*p2 = a2;
										}
									if (p3 && proj->ref[2] > 0.0)
										{
										(void) sprintf(a3, "%g", proj->ref[2]);
										*p3 = a3;
										}
									break;

		/* Lambert conformal (secant or tangent) */
		case ProjectLambertConf:	if (p1)
										{
										(void) sprintf(a1, "%g", proj->ref[0]);
										*p1 = a1;
										}
									if (p2)
										{
										(void) sprintf(a2, "%g", proj->ref[1]);
										*p2 = a2;
										}
									break;
		}

	return TRUE;
	}

/*ARGSUSED*/
/*********************************************************************/
/** Define projection of a type give by type.
 *
 *  @param[in] 	*proj 	projection to define
 *  @param[in] 	type 	type of projection to define
 *  @param[in] 	r1 		parameter 1 (meaning depends on projection type)
 *  @param[in] 	r2 		parameter 2 (meaning depends on projection type)
 *  @param[in] 	r3 		parameter 3 (meaning depends on projection type)
 *  @param[in] 	r4 		parameter 4 (meaning depends on projection type)
 *  @param[in] 	r5		parameter 5 (meaning depends on projection type)
 *  @return True if successful.
 *********************************************************************/
LOGICAL	define_projection
(
 PROJ_DEF	*proj,
 PROJ_TYPE	type,
 float	r1,
 float	r2,
 float	r3,
 float	r4,
 float	r5
 )

{
	int		i;
	double	st, pdist, ndist, secang, pref, nref;
	double	phi1, phi2, rho1, t1, sinphi0, cosphi1, psi;

	/* Error return for missing parameters */
	if (!proj) return FALSE;

	/* Clean out the projection no matter what */
	proj->type = ProjectNone;
	for (i=0; i<5; i++)
		{
		proj->ref[i]  = 0;
		proj->parm[i] = 0;
		}

	switch (type)
		{
		/* No projection (orthographic) */
		case ProjectNone:
			return TRUE;

		/* Lat-Lon */
		case ProjectLatLon:
			proj->type = type;
			return TRUE;

		/* Rotated Lat-Lon */
		case ProjectLatLonAng:
			proj->type          = type;
			proj->ProjectPlat   = r1;
			proj->ProjectPlon   = r2;
			proj->ProjectOrigin = r3;

			/* The constants "sin(plat)" and "cos(plat)" can be */
			/* calculated once */
			proj->ProjectSinPlat = sindeg(r1);
			proj->ProjectCosPlat = cosdeg(r1);

			return TRUE;

		/* Plate-Caree */
		case ProjectPlateCaree:
			proj->type = type;
			return TRUE;

		/* Rectangular (Carte Parallelogrammatique) */
		case ProjectRectangular:
			proj->type        = type;
			proj->ProjectLat1 = r1;
			return TRUE;

		/* Mercator */
		case ProjectMercatorEq:
			proj->type = type;
			return TRUE;

		/* Polar stereographic */
		case ProjectPolarSt:
			proj->type           = type;
			proj->ProjectPole    = r1;
			proj->ProjectTrueLat = r2;

			/* The constant "pdist" can be calculated once */
			st    = sindeg(proj->ProjectTrueLat);
			pdist = (proj->ProjectPole > 0)? RE*(1+st): RE*(1-st);
			proj->ProjectPdist = pdist;

			return TRUE;

		/* Oblique stereographic */
		case ProjectObliqueSt:
			proj->type          = type;
			proj->ProjectPlat   = r1;
			proj->ProjectPlon   = r2;
			proj->ProjectSecant = r3;

			/* The constants "pdist", "sin(plat)" and "cos(plat)" can be */
			/* calculated once */
			proj->ProjectPdist   = RE + RE;
			proj->ProjectSinPlat = sindeg(r1);
			proj->ProjectCosPlat = cosdeg(r1);

			/* Re-calculate "pdist" for secant projection specified */
			secang = proj->ProjectSecant;
			if (secang > 0)
				{
				pref  = RE * (1+cosdeg(secang));
				nref  = RE * sindeg(secang);
				ndist = secang * RE * RAD;
				pdist = pref * ndist / nref;
				proj->ProjectPdist = pdist;
				}

			return TRUE;

		/* Lambert conformal (secant or tangent) */
		case ProjectLambertConf:

			/* Ensure correct ordering of projection latitudes */
			if (fabs((double) r1) > fabs((double) r2))
				{
				r5 = r1;
				r1 = r2;
				r2 = r5;
				r5 = 0;
				}
			proj->type        = type;
			proj->ProjectLat1 = r1;
			proj->ProjectLat2 = r2;

			/* The constants "sin(phi0)", "rho1" and "psi" */
			/* can be calculated once */
			phi1 = (double) proj->ProjectLat1;
			phi2 = (double) proj->ProjectLat2;
			cosphi1 = cosdeg(phi1);
			if (phi1 == phi2)
				{
				sinphi0 = sindeg(phi1);
				}
			else if (phi2 >= 90.0)
				{
				sinphi0 = 1.0;
				}
			else if (phi2 <= -90.0)
				{
				sinphi0 = -1.0;
				}
			else
				{
				/* t1 = log( tandeg(45 - phi1/2.0) / tandeg(45 + phi2/2.0) ); */
				t1 = log( tandeg(45 - phi1/2.0) / tandeg(45 - phi2/2.0) );
				sinphi0 = log( cosphi1 / cosdeg(phi2) ) / t1;
				}
			proj->ProjectSinPhi0 = sinphi0;

			rho1 = RE * cosphi1 / sinphi0;
			proj->ProjectRho1 = rho1;

			t1  = tandeg(45 - phi1/2.0);
			t1  = pow(t1, sinphi0);
			psi = rho1 / t1;
			proj->ProjectPsi = psi;

			return TRUE;

		/* Unrecognized */
		default:
			return FALSE;
		}
	}

/*********************************************************************/
/** Copy a projection.
 *
 *	@param[out]	*p1	copy of projection
 *	@param[in] 	*p2	original
 *********************************************************************/
void	copy_projection

	(
	PROJ_DEF		*p1,
	const PROJ_DEF	*p2
	)

	{
	int	i;

	/* Error return for missing parameters */
	if (!p1) return;
	if (!p2) return;

	/* Make sure the default projection is completely defined */
	if (DefaultProjDef.type != ProjectPolarSt)
		{
		(void) define_projection(&DefaultProjDef, ProjectPolarSt,
									90.0, 60.0, 0.0, 0.0, 0.0);
		}

	p1->type = p2->type;
	for (i=0; i<5; i++)
		{
		p1->ref[i]  = p2->ref[i];
		p1->parm[i] = p2->parm[i];
		}
	}

/*********************************************************************/
/**	 Compare two projections to determine if they are the same.
 *
 *	@param[in]  *p1	first projection to compare
 *	@param[in]  *p2	second projection to compare
 *  @return True if the projections are the same.
 *********************************************************************/
LOGICAL	same_projection

	(
	const PROJ_DEF	*p1,
	const PROJ_DEF	*p2
	)

	{
	int	i;

	/* Error return for missing parameters */
	if (!p1 && !p2) return TRUE;
	if (!p1) return FALSE;
	if (!p2) return FALSE;

	if (p1->type != p2->type) return FALSE;
	for (i=0; i<5; i++)
		{
		if ( !fcompare(p1->ref[i],  p2->ref[i],   1.0, 1e-6) ) return FALSE;
		if ( !fcompare((float)p1->parm[i], (float)p2->parm[i],  1.0, 1e-6) )
				return FALSE;
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     d e f i n e _ m a p _ p r o j e c t i o n                        *
*     c o p y _ m a p _ p r o j e c t i o n                            *
*     s e t _ m a p _ p r o j e c t i o n _ u n i t s                  *
*     s a m e _ m a p _ p r o j e c t i o n                            *
*     e q u i v a l e n t _ m a p _ p r o j e c t i o n                *
*     c o m p l e t e _ m a p _ p r o j e c t i o n                    *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Define a map projection given projection, map and grid definitions
 *
 *	@param[out]	*mproj	map projection
 *	@param[in] 	*proj	projection def
 *	@param[in] 	*map	map def
 *	@param[in] 	*grid	grid def
 *********************************************************************/
void	define_map_projection

	(
	MAP_PROJ		*mproj,
	const PROJ_DEF	*proj,
	const MAP_DEF	*map,
	const GRID_DEF	*grid
	)

	{
	PROJ_DEF	*cproj;
	MAP_DEF		*cmap;
	GRID_DEF	*cgrid;
	float		xo, yo, xc, yc, oplat, oplon;

	/* Error return for missing parameters */
	if (!mproj) return;

	/* Use cproj, cmap and cgrid to point back into the struct */
	cproj = &(mproj->projection);
	cmap  = &(mproj->definition);
	cgrid = &(mproj->grid);

	/* Replace or define key parts if given */
	copy_projection(cproj, (proj)? proj: &NoProjDef);
	copy_map_def(cmap, (map)? map: &NoMapDef);
	normalize_map_def(cmap);
	copy_grid_def(cgrid, (grid)? grid: &NoGridDef);

	/* Calculate the reference position for the given projection and */
	/* map definition */
	if (cmap->units <= 0)
		{
		copy_point(mproj->origin, ZeroPoint);
		mproj->clat = 0.0;
		mproj->clon = 0.0;
		}
	else
		{
		switch (cproj->type)
			{
			/* No projection (orthographic) */
			case ProjectNone:

				/* Compute location of map origin relative to the natural */
				/* origin of the projection */
				copy_point(mproj->origin, ZeroPoint);

				/* Compute central lat-lon of map */
				mproj->clat = 0.0;
				mproj->clon = 0.0;
				break;

			/* Lat-Lon */
			case ProjectLatLon:

				/* Compute location of map origin relative to the natural */
				/* origin of the projection */
				set_point(mproj->origin, cmap->xorg, cmap->yorg);
				(void) lln_ll_xyu(mproj, cmap->olat, cmap->olon, &xo, &yo);
				set_point(mproj->origin, xo, yo);

				/* Compute central lat-lon of map */
				xc = cmap->xlen/2.0;
				yc = cmap->ylen/2.0;
				(void) lln_xy_llu(mproj, xc, yc, &mproj->clat, &mproj->clon);

				break;

			/* Rotated Lat-Lon */
			case ProjectLatLonAng:

				/* Compute location of map origin relative to the natural */
				/* origin of the projection */
				/*
				set_point(mproj->origin, cmap->xorg, cmap->yorg);
				oplat = cmap->olat + cproj->ProjectPlat + 90;
				oplon = cmap->olon + cproj->ProjectPlon;
				*/
				xo = cmap->xorg - cmap->olon/cmap->units;
				yo = cmap->yorg - cmap->olat/cmap->units;
				set_point(mproj->origin, xo, yo);
				oplat = cproj->ProjectPlat + 90;
				oplon = cproj->ProjectPlon;
				(void) llr_ll_xyu(mproj, oplat, oplon, &xo, &yo);
				set_point(mproj->origin, xo, yo);

				/* Compute central lat-lon of map */
				xc = cmap->xlen/2.0;
				yc = cmap->ylen/2.0;
				(void) llr_xy_llu(mproj, xc, yc, &mproj->clat, &mproj->clon);

				break;

			/* Plate-Caree (lat-lon) */
			case ProjectPlateCaree:

				/* Compute location of map origin relative to the natural */
				/* origin of the projection */
				set_point(mproj->origin, cmap->xorg, cmap->yorg);
				(void) pc_ll_xyu(mproj, cmap->olat, cmap->olon, &xo, &yo);
				set_point(mproj->origin, xo, yo);

				/* Compute central lat-lon of map */
				xc = cmap->xlen/2.0;
				yc = cmap->ylen/2.0;
				(void) pc_xy_llu(mproj, xc, yc, &mproj->clat, &mproj->clon);

				break;

			/* Rectangular (Carte Parallelogrammatique) */
			case ProjectRectangular:

				/* Compute location of map origin relative to the natural */
				/* origin of the projection */
				set_point(mproj->origin, cmap->xorg, cmap->yorg);
				(void) rec_ll_xyu(mproj, cmap->olat, cmap->olon, &xo, &yo);
				set_point(mproj->origin, xo, yo);

				/* Compute central lat-lon of map */
				xc = cmap->xlen/2.0;
				yc = cmap->ylen/2.0;
				(void) rec_xy_llu(mproj, xc, yc, &mproj->clat, &mproj->clon);

				break;

			/* Mercator */
			case ProjectMercatorEq:

				/* Compute location of map origin relative to the natural */
				/* origin of the projection */
				set_point(mproj->origin, cmap->xorg, cmap->yorg);
				(void) meq_ll_xyu(mproj, cmap->olat, cmap->olon, &xo, &yo);
				set_point(mproj->origin, xo, yo);

				/* Compute central lat-lon of map */
				xc = cmap->xlen/2.0;
				yc = cmap->ylen/2.0;
				(void) meq_xy_llu(mproj, xc, yc, &mproj->clat, &mproj->clon);

				break;

			/* Polar stereographic */
			case ProjectPolarSt:

				/* Compute location of map origin relative to the natural */
				/* origin of the projection */
				set_point(mproj->origin, cmap->xorg, cmap->yorg);
				(void) ps_ll_xyu(mproj, cmap->olat, cmap->olon, &xo, &yo);
				set_point(mproj->origin, xo, yo);

				/* Compute central lat-lon of map */
				xc = cmap->xlen/2.0;
				yc = cmap->ylen/2.0;
				(void) ps_xy_llu(mproj, xc, yc, &mproj->clat, &mproj->clon);

				break;

			/* Oblique stereographic */
			case ProjectObliqueSt:

				/* Compute location of map origin relative to the natural */
				/* origin of the projection */
				set_point(mproj->origin, cmap->xorg, cmap->yorg);
				(void) os_ll_xyu(mproj, cmap->olat, cmap->olon, &xo, &yo);
				set_point(mproj->origin, xo, yo);

				/* Compute central lat-lon of map */
				xc = cmap->xlen/2.0;
				yc = cmap->ylen/2.0;
				(void) os_xy_llu(mproj, xc, yc, &mproj->clat, &mproj->clon);

				break;

			/* Lambert conformal (secant or tangent) */
			case ProjectLambertConf:

				/* Compute location of map origin relative to the natural */
				/* origin of the projection */
				set_point(mproj->origin, cmap->xorg, cmap->yorg);
				(void) lc_ll_xyu(mproj, cmap->olat, cmap->olon, &xo, &yo);
				set_point(mproj->origin, xo, yo);

				/* Compute central lat-lon of map */
				xc = cmap->xlen/2.0;
				yc = cmap->ylen/2.0;
				(void) lc_xy_llu(mproj, xc, yc, &mproj->clat, &mproj->clon);

				break;

			/* Unrecognized */
			default:
				copy_point(mproj->origin, ZeroPoint);
				break;
			}
		}
	}

/*********************************************************************/
/**	 Copy a given map projection.
 *
 *	@param[out]	*mp1	copy
 *	@param[in] 	*mp2	original
 *********************************************************************/
void	copy_map_projection

	(
	MAP_PROJ		*mp1,
	const MAP_PROJ	*mp2
	)

	{
	/* Error return for missing parameters */
	if (!mp1) return;
	if (!mp2) return;

	define_map_projection(mp1, &(mp2->projection), &(mp2->definition),
							&(mp2->grid));
	}

/*********************************************************************/
/** Set or reset a given map projection's units.
 *
 *	@param[in] 	*mp		map projection
 *	@param[in] 	units	new units
 *********************************************************************/
void	set_map_projection_units

	(
	MAP_PROJ	*mp,
	float		units
	)

	{
	/* Error return for missing parameters */
	if (!mp)        return;
	if (units <= 0) return;

	set_map_def_units(&mp->definition, units);
	set_grid_def_units(&mp->grid, units);
	}

/*********************************************************************/
/** Compare two map projections to determine if they are the same.
 *
 *	@param[in]  *mp1 	first map projection to compare
 *	@param[in]  *mp2		second map projection to compare
 *  @return True if the map projections are the same.
 *********************************************************************/
LOGICAL	same_map_projection

	(
	const MAP_PROJ	*mp1,
	const MAP_PROJ	*mp2
	)

	{
	/* Error return for missing parameters */
	if (!mp1 && !mp2) return TRUE;
	if (!mp1) return FALSE;
	if (!mp2) return FALSE;

	if (!same_projection(&mp1->projection, &mp2->projection))
			return FALSE;
	if (!same_map_def(&mp1->definition, &mp2->definition))
			return FALSE;
	if (!same_grid_def(&mp1->grid, &mp2->grid))
			return FALSE;

	return TRUE;
	}

/*********************************************************************/
/** Compare two map projections to determine if they are equivalent.
 *
 *	@param[in]  *mp1 	first map projection to compare
 *	@param[in]  *mp2		second map projection to compare
 *  @return True if the map projections are equivalent.
 *********************************************************************/
LOGICAL	equivalent_map_projection

	(
	const MAP_PROJ	*mp1,
	const MAP_PROJ	*mp2
	)

	{
	/* Error return for missing parameters */
	if (!mp1 && !mp2) return TRUE;
	if (!mp1) return FALSE;
	if (!mp2) return FALSE;

	if (!same_projection(&mp1->projection, &mp2->projection))
			return FALSE;
	if (!equivalent_map_def(&mp1->definition, &mp2->definition))
			return FALSE;
	if (!equivalent_grid_def(&mp1->grid, &mp2->grid))
			return FALSE;

	return TRUE;
	}

/*********************************************************************/
/**	Check if the given map projection wraps around the globe.
 *
 * @param[in]	*mp		Given Map projection
 * @param[out]	*wrap_x	Does the map projection wrap around the 
 * 						in the x direction?
 * @param[out]	*wrap_i	Does the last column overlap the first column?
 *
 * @return True if the projection wraps around the globe.
 *********************************************************************/
LOGICAL	wrap_map_projection

	(
	const MAP_PROJ	*mp,
	LOGICAL			*wrap_x,
	int				*wrap_i
	)

	{
	float xlen, xgrid, diff;
	float map_units, grid_units;
	int   idiff;

	/* Initialize return values */
	if (wrap_x) *wrap_x = FALSE;
	if (wrap_i) *wrap_i = 0;

	/* Length of map in map units */
	xlen       = mp->definition.xlen;

	/* Map units */
	map_units  = mp->definition.units;

	/* Grid spacing in grid units */
	if (mp->grid.gridlen == 0)
		xgrid      = mp->grid.xgrid;
	else
		xgrid = mp->grid.gridlen;

	/* Grid units */
	grid_units = mp->grid.units;

	switch (mp->projection.type)
		{
		/* No projection (orthographic) */
		case ProjectNone:
			return FALSE;

		/* Lat-Lon */
		case ProjectLatLon:
			diff  = (xlen * map_units) - 360;
			idiff = NINT(diff/(xgrid * grid_units));
			if ( ( idiff >= -1 ) &&
					fcompare(diff, idiff * xgrid * grid_units, 1.0, 1e-5 ) )
				{
				if (wrap_x)	*wrap_x = TRUE;
				if (wrap_i)	*wrap_i = idiff;
				return TRUE;
				}
			return FALSE;

		/* Rotated Lat-Lon */
		case ProjectLatLonAng:
			diff  = (xlen * map_units) - 360;
			idiff = NINT(diff/(xgrid * grid_units));
			if ( ( idiff >= -1 ) &&
					fcompare(diff, idiff * xgrid * grid_units, 1.0, 1e-5 ) )
				{
				if (wrap_x)	*wrap_x = TRUE;
				if (wrap_i)	*wrap_i = idiff;
				return TRUE;
				}
			return FALSE;

		/* Plate-Caree */
		case ProjectPlateCaree:
			diff  = (xlen * map_units) - 360;
			idiff = NINT(diff/(xgrid * grid_units));
			if ( ( idiff >= -1 ) &&
					fcompare(diff, idiff * xgrid * grid_units, 1.0, 1e-5 ) )
				{
				if (wrap_x)	*wrap_x = TRUE;
				if (wrap_i)	*wrap_i = idiff;
				return TRUE;
				}
			return FALSE;

		/* Mercator */
		case ProjectMercatorEq:
			diff  = (xlen * map_units) - 360;
			idiff = NINT(diff/(xgrid * grid_units));
			if ( ( idiff >= -1 ) &&
					fcompare(diff, idiff * xgrid * grid_units, 1.0, 1e-5 ) )
				{
				if (wrap_x)	*wrap_x = TRUE;
				if (wrap_i)	*wrap_i = idiff;
				return TRUE;
				}
			return FALSE;

		/* Rectangular (Carte Parallelogrammatique) */
		case ProjectRectangular:
			return FALSE;

		/* Polar stereographic */
		case ProjectPolarSt:
			return FALSE;

		/* Oblique stereographic */
		case ProjectObliqueSt:
			return FALSE;

		/* Lambert conformal (secant or tangent) */
		case ProjectLambertConf:
			return FALSE;

		/* Unrecognized */
		default:
			return FALSE;
		}
	}
/*********************************************************************/
/** Check if the given map projection is complete.
 *
 *	@param[in]  *mp	given map projection
 *  @return False if at least one of the projection, map or grid is
 * 			not defined.
 *********************************************************************/
LOGICAL	complete_map_projection

	(
	const MAP_PROJ	*mp
	)

	{
	/* Error return for missing parameters */
	if (!mp) return FALSE;

	if (same_projection(&mp->projection, &NoProjDef))
			return FALSE;
	if (same_map_def(&mp->definition, &NoMapDef))
			return FALSE;
	if (same_grid_def(&mp->grid, &NoGridDef))
			return FALSE;

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     p o s _ t o _ g r i d                                            *
*     g r i d _ t o _ p o s                                            *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Convert given x, y in map units to gx, gy in grid lengths
 *
 *	@param[in]  *mproj	given map projection
 *	@param[in] 	pos		map position
 *	@param[out]	gpos	return grid position
 * @return True if successful.
 *********************************************************************/
LOGICAL	pos_to_grid

	(
	const MAP_PROJ	*mproj,
	POINT			pos,
	POINT			gpos
	)

	{
	float	fact;

	/* Error return for missing parameters */
	if (!mproj)                       return FALSE;
	if (mproj->definition.units <= 0) return FALSE;
	if (mproj->grid.units <= 0)       return FALSE;
	if (!pos)                         return FALSE;
	if (!gpos)                        return FALSE;

	/* Convert from map units to grid lengths */
	fact = mproj->definition.units / mproj->grid.units;
	if (mproj->grid.gridlen > 0)
		{
		fact /= mproj->grid.gridlen;
		gpos[X] = pos[X] * fact;
		gpos[Y] = pos[Y] * fact;
		}
	else if (mproj->grid.xgrid > 0 && mproj->grid.ygrid > 0)
		{
		gpos[X] = pos[X] * fact / mproj->grid.xgrid;
		gpos[Y] = pos[Y] * fact / mproj->grid.ygrid;
		}
	else
		return FALSE;

	return TRUE;
	}

/*********************************************************************/
/** Convert given gx, gy in grid lengths to x, y in map units
 *
 *	@param[in]  *mproj	given map projection
 *	@param[in] 	gpos	given grid position
 *	@param[out]	pos		return map position
 *  @return True if successful.
 *********************************************************************/
LOGICAL	grid_to_pos

	(
	const MAP_PROJ	*mproj,
	POINT			gpos,
	POINT			pos
	)

	{
	float	fact;

	/* Error return for missing parameters */
	if (!mproj)                       return FALSE;
	if (mproj->definition.units <= 0) return FALSE;
	if (mproj->grid.units <= 0)       return FALSE;
	if (!gpos)                        return FALSE;
	if (!pos)                         return FALSE;

	/* Convert from grid lengths to map units */
	fact = mproj->definition.units / mproj->grid.units;
	if (mproj->grid.gridlen > 0)
		{
		fact /= mproj->grid.gridlen;
		pos[X] = gpos[X] / fact;
		pos[Y] = gpos[Y] / fact;
		}
	else if (mproj->grid.xgrid > 0 && mproj->grid.ygrid > 0)
		{
		pos[X] = gpos[X] / fact * mproj->grid.xgrid;
		pos[Y] = gpos[Y] / fact * mproj->grid.ygrid;
		}
	else
		return FALSE;

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     l l _ t o _ p o s
*                                                                      *
*     p o s _ t o _ l l
*                                                                      *
*     p o s _ t o _ p o s
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** For given lat, lon returns x, y from origin of mproj
 *
 *	@param[in]  *mproj	given map projection
 *	@param[in] 	lat		given latitude
 *	@param[in] 	lon		given longitude
 *	@param[out]	pos		return map position
 *  @return True if successful.
 *********************************************************************/
LOGICAL	ll_to_pos

	(
	const MAP_PROJ	*mproj,
	float			lat,
	float			lon,
	POINT			pos
	)

	{
	/* Error return for missing parameters */
	if (!mproj)                  return FALSE;
	if (fabs((double) lat) > 90) return FALSE;
	if (!pos)                    return FALSE;

	switch (mproj->projection.type)
		{
		/* No projection (orthographic) */
		case ProjectNone:
			set_point(pos, lat, lon);
			return TRUE;

		/* Lat-Lon */
		case ProjectLatLon:
			return lln_ll_xy(mproj, lat, lon, pos+X, pos+Y);

		/* Rotated Lat-Lon */
		case ProjectLatLonAng:
			return llr_ll_xy(mproj, lat, lon, pos+X, pos+Y);

		/* Plate-Caree */
		case ProjectPlateCaree:
			return pc_ll_xy(mproj, lat, lon, pos+X, pos+Y);

		/* Rectangular (Carte Parallelogrammatique) */
		case ProjectRectangular:
			return rec_ll_xy(mproj, lat, lon, pos+X, pos+Y);

		/* Mercator */
		case ProjectMercatorEq:
			return meq_ll_xy(mproj, lat, lon, pos+X, pos+Y);

		/* Polar stereographic */
		case ProjectPolarSt:
			return ps_ll_xy(mproj, lat, lon, pos+X, pos+Y);

		/* Oblique stereographic */
		case ProjectObliqueSt:
			return os_ll_xy(mproj, lat, lon, pos+X, pos+Y);

		/* Lambert conformal (secant or tangent) */
		case ProjectLambertConf:
			return lc_ll_xy(mproj, lat, lon, pos+X, pos+Y);

		/* Unrecognized */
		default:
			copy_point(pos, ZeroPoint);
			return FALSE;
		}
	}

/*********************************************************************/
/** For given x, y from origin of mproj returns lat and lon
 *
 *	@param[in]   *mproj	given map projection
 *	@param[in] 	pos		given map position
 *	@param[out]	*lat	return latitude
 *	@param[out]	*lon	return longitude
 *  @return True if successful.
 *********************************************************************/
LOGICAL	pos_to_ll

	(
	const MAP_PROJ	*mproj,
	POINT			pos,
	float			*lat,
	float			*lon
	)

	{
	/* Error return for missing parameters */
	if (!mproj) return FALSE;
	if (!pos)   return FALSE;

	switch (mproj->projection.type)
		{
		/* No projection (orthographic) */
		case ProjectNone:
			if (lat) *lat = pos[X];
			if (lon) *lon = pos[Y];
			return TRUE;

		/* Lat-Lon */
		case ProjectLatLon:
			return lln_xy_ll(mproj, pos[X], pos[Y], lat, lon);

		/* Rotated Lat-Lon */
		case ProjectLatLonAng:
			return llr_xy_ll(mproj, pos[X], pos[Y], lat, lon);

		/* Plate-Caree (lat-lon) */
		case ProjectPlateCaree:
			return pc_xy_ll(mproj, pos[X], pos[Y], lat, lon);

		/* Rectangular (Carte Parallelogrammatique) */
		case ProjectRectangular:
			return rec_xy_ll(mproj, pos[X], pos[Y], lat, lon);

		/* Mercator */
		case ProjectMercatorEq:
			return meq_xy_ll(mproj, pos[X], pos[Y], lat, lon);

		/* Polar stereographic */
		case ProjectPolarSt:
			return ps_xy_ll(mproj, pos[X], pos[Y], lat, lon);

		/* Oblique stereographic */
		case ProjectObliqueSt:
			return os_xy_ll(mproj, pos[X], pos[Y], lat, lon);

		/* Lambert conformal (secant or tangent) */
		case ProjectLambertConf:
			return lc_xy_ll(mproj, pos[X], pos[Y], lat, lon);

		/* Unrecognized */
		default:
			if (lat) *lat = 0;
			if (lon) *lon = 0;
			return FALSE;
		}
	}

/*********************************************************************/
/** For given x, y on mp1 returns x, y from origin on mp2
 *
 *	@param[in] 	*mp1	first given map projection
 *	@param[in] 	pos1	given point
 *	@param[in] 	*mp2	second given map projection
 *	@param[out]	pos2	return point in new projection
 *  @return True if successful.
 *********************************************************************/
LOGICAL	pos_to_pos

	(
	const MAP_PROJ	*mp1,
	POINT			pos1,
	const MAP_PROJ	*mp2,
	POINT			pos2
	)

	{
	float	lat, lon, fact, x, y;
	LOGICAL	valid;

	/* Error return for missing parameters */
	if (!mp1)  return FALSE;
	if (!pos1) return FALSE;
	if (!mp2)  return FALSE;
	if (!pos2) return FALSE;

	/* If the projections are the same, we may be able to do it the */
	/* quick way. */
	if (same_projection(&mp1->projection, &mp2->projection))
		{
		/* If the maps are identical, return the same point. */
		if (same_map_def(&mp1->definition, &mp2->definition))
			{
			copy_point(pos2, pos1);
			return TRUE;
			}

		/* If the reference longitudes are the same, scale and translate */
		else if (mp1->definition.lref == mp2->definition.lref)
			{
			fact = mp1->definition.units / mp2->definition.units;

			x = (mp1->origin[X] + pos1[X])*fact;
			y = (mp1->origin[Y] + pos1[Y])*fact;

			pos2[X] = x - mp2->origin[X];
			pos2[Y] = y - mp2->origin[Y];

			return TRUE;
			}
		}

	/* We need to use brute force! */
	/* Convert first point to lat-lon. */
	switch (mp1->projection.type)
		{
		/* No projection (orthographic) */
		case ProjectNone:
			lat   = pos1[X];
			lon   = pos1[Y];
			valid = TRUE;
			break;

		/* Lat-Lon */
		case ProjectLatLon:
			valid = lln_xy_ll(mp1, pos1[X], pos1[Y], &lat, &lon);
			break;

		/* Rotated Lat-Lon */
		case ProjectLatLonAng:
			valid = llr_xy_ll(mp1, pos1[X], pos1[Y], &lat, &lon);
			break;

		/* Plate-Caree (lat-lon) */
		case ProjectPlateCaree:
			valid = pc_xy_ll(mp1, pos1[X], pos1[Y], &lat, &lon);
			break;

		/* Rectangular (Carte Parallelogrammatique) */
		case ProjectRectangular:
			valid = rec_xy_ll(mp1, pos1[X], pos1[Y], &lat, &lon);
			break;

		/* Mercator */
		case ProjectMercatorEq:
			valid = meq_xy_ll(mp1, pos1[X], pos1[Y], &lat, &lon);
			break;

		/* Polar stereographic */
		case ProjectPolarSt:
			valid = ps_xy_ll(mp1, pos1[X], pos1[Y], &lat, &lon);
			break;

		/* Oblique stereographic */
		case ProjectObliqueSt:
			valid = os_xy_ll(mp1, pos1[X], pos1[Y], &lat, &lon);
			break;

		/* Lambert conformal (secant or tangent) */
		case ProjectLambertConf:
			valid = lc_xy_ll(mp1, pos1[X], pos1[Y], &lat, &lon);
			break;

		/* Unrecognized */
		default:
			valid = FALSE;
		}

	if (!valid)
		{
		copy_point(pos2, ZeroPoint);
		return FALSE;
		}

	/* Now convert lat-lon to point in second projection */
	switch (mp2->projection.type)
		{
		/* No projection (orthographic) */
		case ProjectNone:
			set_point(pos2, lat, lon);
			return TRUE;

		/* Lat-Lon */
		case ProjectLatLon:
			return lln_ll_xy(mp2, lat, lon, pos2+X, pos2+Y);

		/* Rotated Lat-Lon */
		case ProjectLatLonAng:
			return llr_ll_xy(mp2, lat, lon, pos2+X, pos2+Y);

		/* Plate-Caree (lat-lon) */
		case ProjectPlateCaree:
			return pc_ll_xy(mp2, lat, lon, pos2+X, pos2+Y);

		/* Rectangular (Carte Parallelogrammatique) */
		case ProjectRectangular:
			return rec_ll_xy(mp2, lat, lon, pos2+X, pos2+Y);

		/* Mercator */
		case ProjectMercatorEq:
			return meq_ll_xy(mp2, lat, lon, pos2+X, pos2+Y);

		/* Polar stereographic */
		case ProjectPolarSt:
			return ps_ll_xy(mp2, lat, lon, pos2+X, pos2+Y);

		/* Oblique stereographic */
		case ProjectObliqueSt:
			return os_ll_xy(mp2, lat, lon, pos2+X, pos2+Y);

		/* Lambert conformal (secant or tangent) */
		case ProjectLambertConf:
			return lc_ll_xy(mp2, lat, lon, pos2+X, pos2+Y);

		/* Unrecognized */
		default:
			copy_point(pos2, ZeroPoint);
			return FALSE;
		}
	}

/***********************************************************************
*                                                                      *
*     l l _ d i s t o r t                                              *
*                                                                      *
*     p o s _ d i s t o r t                                            *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Calculate projection distortion for given lat/lon.
 *
 *	@param[in] 	*mproj	given map projection
 *	@param[in] 	lat		given latitude
 *	@param[in] 	lon		given longitude
 *	@param[out]	*sx		distortion in the x direction
 *	@param[out]	*sy		distortion in the y direction
 *  @return Ture if successful.
 *********************************************************************/

LOGICAL	ll_distort

	(
	const MAP_PROJ	*mproj,
	float			lat,
	float			lon,
	float			*sx,
	float			*sy
	)

	{
	/* Error return for missing parameters */
	if (!mproj)                  return FALSE;
	if (fabs((double) lat) > 90) return FALSE;

	switch (mproj->projection.type)
		{
		/* No projection (orthographic) */
		case ProjectNone:
			if (sx) *sx = 1.0;
			if (sy) *sy = 1.0;
			return TRUE;

		/* Lat-Lon */
		case ProjectLatLon:
			return lln_distort(mproj, lat, lon, sx, sy);

		/* Rotated Lat-Lon */
		case ProjectLatLonAng:
			return llr_distort(mproj, lat, lon, sx, sy);

		/* Plate-Caree */
		case ProjectPlateCaree:
			return pc_distort(mproj, lat, lon, sx, sy);

		/* Rectangular (Carte Parallelogrammatique) */
		case ProjectRectangular:
			return rec_distort(mproj, lat, lon, sx, sy);

		/* Mercator */
		case ProjectMercatorEq:
			return meq_distort(mproj, lat, lon, sx, sy);

		/* Polar stereographic */
		case ProjectPolarSt:
			return ps_distort(mproj, lat, lon, sx, sy);

		/* Oblique stereographic */
		case ProjectObliqueSt:
			return os_distort(mproj, lat, lon, sx, sy);

		/* Lambert conformal (secant or tangent) */
		case ProjectLambertConf:
			return lc_distort(mproj, lat, lon, sx, sy);

		/* Unrecognized */
		default:
			if (sx) *sx = 1.0;
			if (sy) *sy = 1.0;
			return FALSE;
		}
	}

/*********************************************************************/
/** Calculate projection distortion for given x/y position.
 *
 *	@param[in] 	*mproj	given map projection
 *	@param[in] 	pos		given point
 *	@param[in] 	*sx		distortion is the x direction
 *	@param[in] 	*sy		distortion is the y direction
 *  @return Ture if successful.
 *********************************************************************/
LOGICAL	pos_distort

	(
	const MAP_PROJ	*mproj,
	POINT			pos,
	float			*sx,
	float			*sy
	)

	{
	float	lat, lon;

	/* Error return for missing parameters */
	if (!mproj) return FALSE;
	if (!pos)   return FALSE;

	if (!pos_to_ll(mproj, pos, &lat, &lon))   return FALSE;
	if (!ll_distort(mproj, lat, lon, sx, sy)) return FALSE;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*   m a p _ t o _ m a p                                                *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Determine whether a remapping can be made between the 2 given map
 * projections using simple translation and rotation.
 *
 *	@param[in] 	*mp1	given starting map projection
 *	@param[in] 	org1	origin of starting projection
 *	@param[in] 	ang1	starting angle
 *	@param[in] 	*mp2	given map resulting projection
 *	@param[in] 	org2	origin of resulting projection
 *	@param[out]	*ang2	rotation required
 *  @return True if the remapping can be done.
 *********************************************************************/

LOGICAL		map_to_map

	(
	const MAP_PROJ	*mp1,
	POINT			org1,
	float			ang1,
	const MAP_PROJ	*mp2,
	POINT			org2,
	float			*ang2
	)

	{
	/*
	float	rlon;
	*/
	float	angle, llim, rlim;
	POINT	off1, off2;

	/* Start with copy of original translation and rotation */
	if (NotNull(org2)) copy_point(org2, org1);
	if (NotNull(ang2)) *ang2 = ang1;

	if (IsNull(mp1)) return FALSE;
	if (IsNull(mp2)) return FALSE;

	/* Cannot do it unless projections match */
	if ( !same_projection(&mp1->projection, &mp2->projection) )
		return FALSE;

	/* Recalculate existing translation */
	if (NotNull(org2)) (void) pos_to_pos(mp1, org1, mp2, org2);

	/* Recalculate existing rotation */
	switch (mp1->projection.type)
		{
		case ProjectNone:		/* No projection (orthographic) */
			return TRUE;

		/* Do pure translation for cylindrical projections */
		case ProjectLatLon:		/* Lat-Lon */
		case ProjectLatLonAng:	/* Rotated Lat-Lon */
		case ProjectPlateCaree:	/* Plate-Caree */
		case ProjectRectangular:/* Rectangular */
		case ProjectMercatorEq:	/* Mercator */

			/* Cannot do it if source intersects the target in two places */
			/* Find where left (western) and right (eastern) edges of mp1 */
			/* lie wrt mp2 */
			set_point(off1, 0., 0.);
			(void) pos_to_pos(mp1, off1, mp2, off2);
			llim = off2[X];
			set_point(off1, mp1->definition.xlen, 0.);
			(void) pos_to_pos(mp1, off1, mp2, off2);
			rlim = off2[X];
			if (llim > rlim)
				{
				/* If left > right then mp1 wraps around other side of globe */
				/* Must reproject if both are inside mp2 (2 intersections) */
				if (llim>0 && llim<mp2->definition.xlen &&
					rlim>0 && rlim<mp2->definition.xlen)
					return FALSE;
				}

			/* Leave the angle at zero */
			return TRUE;

		/* Do pure rotation for azimuthal projections */
		case ProjectPolarSt:	/* Polar stereographic */
		case ProjectObliqueSt:	/* Oblique stereographic */

			/* Rotate the azimuthal plane */
			angle = ang1 - mp2->definition.lref + mp1->definition.lref;
			if (NotNull(ang2)) *ang2 = (float) fmod((double) angle, 360.0);
			return TRUE;

		/* Do mix of translation and rotation for conical projections */
		case ProjectLambertConf:/* Lambert conformal (secant or tangent) */

			/* Cannot do it if source intersects the target in two places */
			/* Cannot do it if source cut-line is on the target map */
			/*
			rlon = mp1->clon + 180;
			if (rlon > ??? && rlon < ???) return FALSE;
			*/

			/* Do rotation similar to azimuthal case, except scale the */
			/* resultant angle according to the cone constant */
			angle = ang1 - mp2->definition.lref + mp1->definition.lref;
			angle *= mp1->projection.ProjectSinPhi0;

			/* Cannot trust this yet - force reproject */
			return FALSE;

		default:				/* Unrecognized */
			return FALSE;
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*   w i n d _ d i r _ t r u e                                          *
*   w i n d _ d i r _ x y                                              *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Convert wind direction wrt x axis to wind direction wrt true North.
 *
 *	@param[in] 	*mproj	given map projection
 *	@param[in] 	lat		given latitude
 *	@param[in] 	lon		given longitude
 *	@param[in] 	xyang	wind direction wrt x-axis
 *  @return wind direction wrt to true North.
 *********************************************************************/

float	wind_dir_true

	(
	const MAP_PROJ	*mproj,
	float			lat,
	float			lon,
	float			xyang
	)

	{
	/* Error return for missing parameters */
	if (!mproj) return 0.0;

	/* Oddly enough, the equations are symmetrical */
	/* (true = north - xy  =>  xy = north - true) */
	return wind_dir_xy(mproj, lat, lon, xyang);
	}

/*ARGSUSED*/
/*********************************************************************/
/** Convert wind direction wrt true North to wind direction wrt x axis.
 *
 *	@param[in] 	*mproj 	given map projection
 *	@param[in] 	lat 	given latitude
 *	@param[in] 	lon 	given longitude
 *	@param[in] 	trueang	wind direction wrt true North
 *  @return wind direction wrt x axis.
 *********************************************************************/
float	wind_dir_xy

	(
	const MAP_PROJ	*mproj,
	float			lat,
	float			lon,
	float			trueang
	)

	{
	double	xyang;

	/* Error return for missing parameters */
	if (!mproj) return 0.0;

	/* Determine which way north is then subtract the true wind direction */
	switch (mproj->projection.type)
		{
		/* No projection (orthographic) */
		case ProjectNone:
			return 0.0;

		/* Lat-Lon */
		case ProjectLatLon:
			xyang = 90 - trueang;
			break;

		/* Rotated Lat-Lon */
		case ProjectLatLonAng:
			xyang = 90 - trueang;
			break;

		/* Plate-Caree (lat-lon) */
		case ProjectPlateCaree:
			xyang = 90 - trueang;
			break;

		/* Rectangular (Carte Parallelogrammatique) */
		case ProjectRectangular:
			xyang = 90 - trueang;
			break;

		/* Mercator */
		case ProjectMercatorEq:
			xyang = 90 - trueang;
			break;

		/* Polar stereographic */
		case ProjectPolarSt:
			if (mproj->projection.ProjectPole >= 0.0)	/* North Polar stereographic */
				xyang = 90 + lon - mproj->definition.lref - trueang;
			else										/* South Polar stereographic */
				xyang = 90 - lon + mproj->definition.lref - trueang;
			break;

		/* Oblique stereographic */
		case ProjectObliqueSt:
			/* >>> not correct - fix later <<< */
			xyang = 90 + lon - mproj->definition.lref - trueang;
			break;

		/* Lambert conformal (secant or tangent) */
		case ProjectLambertConf:
			xyang = 90 + lon - mproj->definition.lref - trueang;
			break;

		/* Unrecognized */
		default:
			return 0.0;
		}

	xyang = fmod(xyang, 360.0);
	if (xyang >= 0.0) return (float) xyang;
	else return (float) (xyang + 360.0);
	}

/***********************************************************************
*                                                                      *
*   g r e a t _ c i r c l e _ d i s t a n c e                          *
*                                                                      *
 **********************************************************************/
/**********************************************************************/
/** determine a great circle distance between two points using
 *    the Haversine formula
 *
 *	@param[in] 	*mproj		Map projection
 *	@param[in] 	spos		Start point
 *	@param[in] 	epos		End point
 *	@return distance in meters
 **********************************************************************/

float	great_circle_distance

	(
	const MAP_PROJ	*mproj,
	POINT			spos,
	POINT			epos
	)

	{
	float	slat, slon, elat, elon;
	double	dlats, dlons, dang, dist;

	/* Error return for missing parameters */
	if (!mproj) return 0.0;

	/* Get latitude and longitude from start and end points */
	if (!pos_to_ll(mproj, spos, &slat, &slon)) return 0.0;
	if (!pos_to_ll(mproj, epos, &elat, &elon)) return 0.0;

	/* Determine angles for Haversine formula */
	dlats = sindeg((double)(elat-slat)/2.0);
	dlons = sindeg((double)(elon-slon)/2.0);
	dang  = (dlats * dlats)
			+ (cosdeg((double)slat) * cosdeg((double)elat) * dlons * dlons);

	/* Return the distance (in m) */
	dist  = 2.0 * RE * fpa_atan2(fpa_sqrt(dang), fpa_sqrt(1-dang));
	return (float) dist;
	}

/***********************************************************************
*                                                                      *
*   g r e a t _ c i r c l e _ b e a r i n g                            *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** determine a bearing from one location to another on the Earth
 *    on a spherical Earth
 *
 *	@param[in] 	*mproj		Map projection
 *	@param[in] 	spos		Start point
 *	@param[in] 	epos		End point
 *	@return bearing in degrees true
 **********************************************************************/

float	great_circle_bearing

	(
	const MAP_PROJ	*mproj,
	POINT			spos,
	POINT			epos
	)

	{
	float	slat, slon, elat, elon, dlon;
	float	dy, dx, dang;

	/* Error return for missing parameters */
	if (!mproj) return -1.0;

	/* Get latitude and longitude from start and end points */
	if (!pos_to_ll(mproj, spos, &slat, &slon)) return -1.0;
	if (!pos_to_ll(mproj, epos, &elat, &elon)) return -1.0;

	/* Determine angles for bearing */
	dlon = elon - slon;
	dx   = (cosdeg(slat) * sindeg(elat))
			- (sindeg(slat) * cosdeg(elat) * cosdeg(dlon));
	dy   = sindeg(dlon) * cosdeg(elat);

	/* Return the bearing (in degrees true) */
	dang = atan2deg(dy, dx);
	if (dang < 0.0) dang += 360.0;
	return dang;
	}

/***********************************************************************
*                                                                      *
*   g r e a t _ c i r c l e _ s p a n                                  *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** determine an end location from a start location and a great
 *    circle distance on a given bearing
 *
 *	@param[in] 	*mproj		Map projection
 *	@param[in] 	spos		Start point
 *	@param[in] 	dir			Bearing to end point in degrees true
 *	@param[in] 	dist		Distance to end point in meters
 *	@param[out]	epos		End point
 *	@return True if successful
 **********************************************************************/

LOGICAL	great_circle_span

	(
	const MAP_PROJ	*mproj,
	POINT			spos,
	float			dir,
	float			dist,
	POINT			epos
	)

	{
	float	slat, slon, xdir;
	double	adist, adist2, dang, dangx, dlon, dlate, dlone;

	/* Error return for missing parameters */
	if (!mproj) return FALSE;
	if (!epos)  return FALSE;

	/* Initialize end point */
	(void) set_point(epos, -1.0, -1.0);

	/* Get latitude and longitude from start point */
	if (!pos_to_ll(mproj, spos, &slat, &slon)) return FALSE;

	/* Convert direction to degrees on the map projection */
	xdir = wind_dir_xy(mproj, slat, slon, dir);

#	ifdef DEBUG_GREATCIRCLE
	(void) fprintf(stdout,
		"[great_circle_span]: Start Lat/Lon: %g %g\n", slat, slon);
	(void) fprintf(stdout,
		"[great_circle_span]: dir true/map: %g %g\n", dir, xdir);
#	endif /* DEBUG_GREATCIRCLE */

	/* Determine arc lengths from distance */
	adist  = cos((double) (dist / RE));
	adist2 = sqrt(1.0 - adist*adist);

#	ifdef DEBUG_GREATCIRCLE
	(void) fprintf(stdout,
		"[great_circle_span]: dist/adist/adist2: %g %g %g\n",
		dist, adist, adist2);
#	endif /* DEBUG_GREATCIRCLE */

	/* Determine ending latitude */
	dang  = adist2 * cosdeg((double) dir) * cosdeg((double) slat)
				+ adist * sindeg((double) slat);
	dlate = asindeg(dang);

#	ifdef DEBUG_GREATCIRCLE
	(void) fprintf(stdout,
		"[great_circle_span]: Lat ang/dlate: %g %g\n", dang, dlate);
#	endif /* DEBUG_GREATCIRCLE */

	/* Determine longitude increment */
	dang  = (adist - (sindeg((double) slat) * sindeg(dlate)))
				/ (cosdeg((double) slat) * cosdeg(dlate));
	dangx = dang;
	if (dangx >  1.0) dangx =  1.0;
	if (dangx < -1.0) dangx = -1.0;
	dlon  = acosdeg(dangx);

#	ifdef DEBUG_GREATCIRCLE
	(void) fprintf(stdout,
		"[great_circle_span]: Lon ang/dlon: %g %g\n", dang, dlon);
#	endif /* DEBUG_GREATCIRCLE */

	/* Determine ending longitude based on quadrant */
	xdir = (float) fmod((double) dir, 360.0);
	if (xdir < -180.0) xdir += 360.0;
	if (xdir >  180.0) xdir -= 360.0;
	if ( xdir >= 0.0 ) dlone = (double) slon + dlon;
	else               dlone = (double) slon - dlon;

#	ifdef DEBUG_GREATCIRCLE
	(void) fprintf(stdout,
		"[great_circle_span]: End Lat/Lon: %g %g\n", dlate, dlone);
#	endif /* DEBUG_GREATCIRCLE */

	/* Convert ending latitude and longitude to position on map */
	return ll_to_pos(mproj, (float) dlate, (float) dlone, epos);
	}

/****************************************************************************
*                                                                           *
*   STATIC Functions                                                        *
*                                                                           *
*   The following functions can only be used internally.                    *
*                                                                           *
****************************************************************************/

/***********************************************************************
*                                                                      *
*     l l n _ l l _ x y                                                *
*     l l n _ l l _ x y u                                              *
*     l l n _ x y _ l l                                                *
*     l l n _ x y _ l l u                                              *
*                                                                      *
*     transform and inverse for Lat-Lon projections                    *
*                                                                      *
*     l l n _ d i s t o r t                                            *
*                                                                      *
*     distortion for Lat-Lon projections                               *
*                                                                      *
***********************************************************************/
static	LOGICAL	lln_ll_xy

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			lat,
	float			lon,
	float			*x,
	float			*y
	)

	{
	if (x) *x = 0;
	if (y) *y = 0;
	if (!mproj) return FALSE;

	if (lat > 90)  return FALSE;
	if (lat < -90) return FALSE;

	/* Normalize longitude wrt center of projection */
	while (lon <= mproj->clon-180) lon += 360;
	while (lon >  mproj->clon+180) lon -= 360;

	return lln_ll_xyu(mproj, lat, lon, x, y);
	}

static	LOGICAL	lln_ll_xyu

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			lat,
	float			lon,
	float			*x,
	float			*y
	)

	{
	double	units;

	if (x) *x = 0;
	if (y) *y = 0;
	if (!mproj) return FALSE;

	units = mproj->definition.units;
	if (x) *x = lon/units - mproj->origin[X];
	if (y) *y = lat/units - mproj->origin[Y];
	return TRUE;
	}

static	LOGICAL	lln_xy_ll

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			x,
	float			y,
	float			*lat,
	float			*lon
	)

	{
	if (lat) *lat = 0;
	if (lon) *lon = 0;
	if (!mproj) return FALSE;

	if (!lln_xy_llu(mproj, x, y, lat, lon)) return FALSE;
	norm_lat_lon(lat, lon);
	return TRUE;
	}

static	LOGICAL	lln_xy_llu

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			x,
	float			y,
	float			*lat,
	float			*lon
	)

	{
	double	units;

	if (lat) *lat = 0;
	if (lon) *lon = 0;
	if (!mproj) return FALSE;

	units = mproj->definition.units;
	if (lon) *lon = (x + mproj->origin[X]) * units;
	if (lat) *lat = (y + mproj->origin[Y]) * units;
	return TRUE;
	}

/*ARGSUSED*/
static	LOGICAL	lln_distort

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			lat,
	float			lon,
	float			*sx,
	float			*sy
	)

	{
	if (sx) *sx = 1;
	if (sy) *sy = 1;
	if (!mproj) return FALSE;

	if (lat > 90)  return FALSE;
	if (lat < -90) return FALSE;

	/* Figure this out later */
	if (sx) *sx = 1;
	if (sy) *sy = 1;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     l l r _ l l _ x y                                                *
*     l l r _ l l _ x y u                                              *
*     l l r _ x y _ l l                                                *
*     l l r _ x y _ l l u                                              *
*                                                                      *
*     transform and inverse for rotated Lat-Lon projections            *
*                                                                      *
*     l l r _ d i s t o r t                                            *
*                                                                      *
*     distortion for rotated Lat-Lon projections                       *
*                                                                      *
***********************************************************************/
static	LOGICAL	llr_ll_xy

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			lat,
	float			lon,
	float			*x,
	float			*y
	)

	{
	if (x) *x = 0;
	if (y) *y = 0;
	if (!mproj) return FALSE;

	if (lat > 90)  return FALSE;
	if (lat < -90) return FALSE;

	/* Normalize longitude wrt center of projection */
	while (lon <= mproj->clon-180) lon += 360;
	while (lon >  mproj->clon+180) lon -= 360;

	return llr_ll_xyu(mproj, lat, lon, x, y);
	}

static	LOGICAL	llr_ll_xyu

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			lat,
	float			lon,
	float			*x,
	float			*y
	)

	{
	double	dlon, alf, alfc, units;
	double	phi, h, cosp, sinp, cosl, sinl;

	if (x) *x = 0;
	if (y) *y = 0;
	if (!mproj) return FALSE;

	units = mproj->definition.units;
	dlon  = lon + 180
			- mproj->projection.ProjectPlon
			+ mproj->projection.ProjectOrigin;
	phi   = lat;

	/* Rotate virtual pole to true pole */
	cosp = mproj->projection.ProjectCosPlat;
	sinp = -mproj->projection.ProjectSinPlat;
	cosl = fpa_cosdeg(phi);
	sinl = fpa_sindeg(phi);
	if (dlon == 0)
		{
		if (sinl > sinp) alf = 180;
		else             alf = 0;
		}
	else
		{
		alf = fpa_atan2deg(sindeg(dlon)*cosl,
							-cosp*sinl + sinp*cosdeg(dlon)*cosl);
		}
	h    = asindeg(sinl*sinp + cosl*cosp*cosdeg(dlon));

	/* Normalize longitude wrt center of projection */
	alfc = (mproj->definition.xlen/2.0 + mproj->origin[X]) * units;
	while (alf <= alfc-180) alf += 360;
	while (alf >  alfc+180) alf -= 360;

	if (x) *x = alf/units - mproj->origin[X];
	if (y) *y = h/units   - mproj->origin[Y];
	return TRUE;
	}

static	LOGICAL	llr_xy_ll

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			x,
	float			y,
	float			*lat,
	float			*lon
	)

	{
	if (lat) *lat = 0;
	if (lon) *lon = 0;
	if (!mproj) return FALSE;

	if (!llr_xy_llu(mproj, x, y, lat, lon)) return FALSE;
	norm_lat_lon(lat, lon);
	return TRUE;
	}

static	LOGICAL	llr_xy_llu

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			x,
	float			y,
	float			*lat,
	float			*lon
	)

	{
	double	dlon, h, alf;
	double	units, sinp, cosp, cosl, sinl;

	if (lat) *lat = 0;
	if (lon) *lon = 0;
	if (!mproj) return FALSE;

	units = mproj->definition.units;
	alf = (x + mproj->origin[X]) * units;
	h   = (y + mproj->origin[Y]) * units;

	/* Rotate true pole to virtual pole */
	cosp = mproj->projection.ProjectCosPlat;
	sinp = -mproj->projection.ProjectSinPlat;
	cosl = fpa_cosdeg(h);
	sinl = fpa_sindeg(h);
	dlon = (alf == 0)? 0:
			fpa_atan2deg(sindeg(alf)*cosl, cosp*sinl + sinp*cosdeg(alf)*cosl);

	if (lon) *lon = dlon - 180
						+ mproj->projection.ProjectPlon
						- mproj->projection.ProjectOrigin;
	if (lat) *lat = asindeg(sinp*sinl - cosp*cosl*cosdeg(alf));
	return TRUE;
	}

/*ARGSUSED*/
static	LOGICAL	llr_distort

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			lat,
	float			lon,
	float			*sx,
	float			*sy
	)

	{
	if (sx) *sx = 1;
	if (sy) *sy = 1;
	if (!mproj) return FALSE;

	if (lat > 90)  return FALSE;
	if (lat < -90) return FALSE;

	/* Figure this out later */
	if (sx) *sx = 1;
	if (sy) *sy = 1;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     p c _ l l _ x y                                                  *
*     p c _ l l _ x y u                                                *
*     p c _ x y _ l l                                                  *
*     p c _ x y _ l l u                                                *
*                                                                      *
*     transform and inverse for Plate-Caree projections                *
*                                                                      *
*     p c _ d i s t o r t                                              *
*                                                                      *
*     distortion for Plate-Caree projections                           *
*                                                                      *
*     mp = 1                                                           *
*                                                                      *
*     mm = 1 / cos(lat)                                                *
*                                                                      *
***********************************************************************/
static	LOGICAL	pc_ll_xy

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			lat,
	float			lon,
	float			*x,
	float			*y
	)

	{
	if (x) *x = 0;
	if (y) *y = 0;
	if (!mproj) return FALSE;

	if (lat > 90)  return FALSE;
	if (lat < -90) return FALSE;

	/* Normalize longitude wrt center of projection */
	while (lon <= mproj->clon-180) lon += 360;
	while (lon >  mproj->clon+180) lon -= 360;

	return pc_ll_xyu(mproj, lat, lon, x, y);
	}

static	LOGICAL	pc_ll_xyu

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			lat,
	float			lon,
	float			*x,
	float			*y
	)

	{
	double	units;

	if (x) *x = 0;
	if (y) *y = 0;
	if (!mproj) return FALSE;

	units = mproj->definition.units / (RE*RAD);
	if (x) *x = lon/units - mproj->origin[X];
	if (y) *y = lat/units - mproj->origin[Y];
	return TRUE;
	}

static	LOGICAL	pc_xy_ll

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			x,
	float			y,
	float			*lat,
	float			*lon
	)

	{
	if (lat) *lat = 0;
	if (lon) *lon = 0;
	if (!mproj) return FALSE;

	if (!pc_xy_llu(mproj, x, y, lat, lon)) return FALSE;
	norm_lat_lon(lat, lon);
	return TRUE;
	}

static	LOGICAL	pc_xy_llu

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			x,
	float			y,
	float			*lat,
	float			*lon
	)

	{
	double	units;

	if (lat) *lat = 0;
	if (lon) *lon = 0;
	if (!mproj) return FALSE;

	units = mproj->definition.units / (RE*RAD);
	if (lon) *lon = (x + mproj->origin[X]) * units;
	if (lat) *lat = (y + mproj->origin[Y]) * units;
	return TRUE;
	}

/*ARGSUSED*/
static	LOGICAL	pc_distort

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			lat,
	float			lon,
	float			*sx,
	float			*sy
	)

	{
	if (sx) *sx = 1;
	if (sy) *sy = 1;
	if (!mproj) return FALSE;

	if (lat > 90)  return FALSE;
	if (lat < -90) return FALSE;

	if (sx) *sx = 1 / fpa_cosdeg((double) lat);
	if (sy) *sy = 1;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     r e c _ l l _ x y                                                *
*     r e c _ l l _ x y u                                              *
*     r e c _ x y _ l l                                                *
*     r e c _ x y _ l l u                                              *
*                                                                      *
*     transform and inverse for rectangular projections                *
*                                                                      *
*     r e c _ d i s t o r t                                            *
*                                                                      *
*     distortion for rectangular projections                           *
*                                                                      *
*     mp = 1                                                           *
*                                                                      *
*     mm = 1 / cos(delta_lat)                                          *
*                                                                      *
***********************************************************************/
static	LOGICAL	rec_ll_xy

	(
	const MAP_PROJ	*mproj,
	float			lat,
	float			lon,
	float			*x,
	float			*y
	)

	{
	if (x) *x = 0;
	if (y) *y = 0;
	if (!mproj) return FALSE;

	if (lat > 90)  return FALSE;
	if (lat < -90) return FALSE;

	/* Normalize longitude wrt center of projection */
	while (lon <= mproj->clon-180) lon += 360;
	while (lon >  mproj->clon+180) lon -= 360;

	return rec_ll_xyu(mproj, lat, lon, x, y);
	}

static	LOGICAL	rec_ll_xyu

	(
	const MAP_PROJ	*mproj,
	float			lat,
	float			lon,
	float			*x,
	float			*y
	)

	{
	double	units;

	if (x) *x = 0;
	if (y) *y = 0;
	if (!mproj) return FALSE;

	units = mproj->definition.units / (RE*RAD);
	if (x) *x = lon*fpa_cosdeg((double)mproj->projection.ProjectLat1)/units - mproj->origin[X];
	if (y) *y = lat/units - mproj->origin[Y];
	return TRUE;
	}

static	LOGICAL	rec_xy_ll

	(
	const MAP_PROJ	*mproj,
	float			x,
	float			y,
	float			*lat,
	float			*lon
	)

	{
	if (lat) *lat = 0;
	if (lon) *lon = 0;
	if (!mproj) return FALSE;

	if (!rec_xy_llu(mproj, x, y, lat, lon)) return FALSE;
	norm_lat_lon(lat, lon);
	return TRUE;
	}

static	LOGICAL	rec_xy_llu

	(
	const MAP_PROJ	*mproj,
	float			x,
	float			y,
	float			*lat,
	float			*lon
	)

	{
	double	units;

	if (lat) *lat = 0;
	if (lon) *lon = 0;
	if (!mproj) return FALSE;

	units = mproj->definition.units / (RE*RAD);
	if (lon) *lon = (x + mproj->origin[X]) * units / fpa_cosdeg((double)mproj->projection.ProjectLat1);
	if (lat) *lat = (y + mproj->origin[Y]) * units;
	return TRUE;
	}

/*ARGSUSED*/
static	LOGICAL	rec_distort

	(
	const MAP_PROJ	*mproj,
	float			lat,
	float			lon,
	float			*sx,
	float			*sy
	)

	{
	if (sx) *sx = 1;
	if (sy) *sy = 1;
	if (!mproj) return FALSE;

	if (lat > 90)  return FALSE;
	if (lat < -90) return FALSE;

	if (sx) *sx = 1 / fpa_cosdeg((double)(lat - mproj->projection.ProjectLat1));
	if (sy) *sy = 1;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     m e q _ l l _ x y                                                *
*     m e q _ l l _ x y u                                              *
*     m e q _ x y _ l l                                                *
*     m e q _ x y _ l l u                                              *
*                                                                      *
*     transform and inverse for Equatorial Mercator projections        *
*                                                                      *
*     m e q _ d i s t o r t                                            *
*                                                                      *
*     distortion for Equatorial Mercator projections                   *
*                                                                      *
*     mp = mm = 1 / cos(lat)                                           *
*                                                                      *
***********************************************************************/
static	LOGICAL	meq_ll_xy

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			lat,
	float			lon,
	float			*x,
	float			*y
	)

	{
	if (x) *x = 0;
	if (y) *y = 0;
	if (!mproj) return FALSE;

	if (lat > 90)  return FALSE;
	if (lat < -90) return FALSE;

	/* Normalize longitude wrt center of projection */
	while (lon <= mproj->clon-180) lon += 360;
	while (lon >  mproj->clon+180) lon -= 360;

	return meq_ll_xyu(mproj, lat, lon, x, y);
	}

static	LOGICAL	meq_ll_xyu

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			lat,
	float			lon,
	float			*x,
	float			*y
	)

	{
	double	dlon, tphi, t1, units, xv, yv;

	if (x) *x = 0;
	if (y) *y = 0;
	if (!mproj) return FALSE;

	units = mproj->definition.units;
	dlon  = lon;
	tphi  = fabs((double) lat);
	t1    = tandeg(45 + tphi/2.0);
	if (fabs(t1) > 1000000.0) t1 = 1000000.0;

	xv = RE * RAD * dlon;
	yv = RE * log(t1);
	if (lat < 0.0) yv = -yv;
	if (x) *x = xv/units - mproj->origin[X];
	if (y) *y = yv/units - mproj->origin[Y];
	return TRUE;
	}

static	LOGICAL	meq_xy_ll

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			x,
	float			y,
	float			*lat,
	float			*lon
	)

	{
	if (lat) *lat = 0;
	if (lon) *lon = 0;
	if (!mproj) return FALSE;

	if (!meq_xy_llu(mproj, x, y, lat, lon)) return FALSE;
	norm_lat_lon(lat, lon);
	return TRUE;
	}

static	LOGICAL	meq_xy_llu

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			x,
	float			y,
	float			*lat,
	float			*lon
	)

	{
	double	xv, yv, units;

	if (lat) *lat = 0;
	if (lon) *lon = 0;
	if (!mproj) return FALSE;

	units = mproj->definition.units;
	xv    = (x + mproj->origin[X]) * units;
	yv    = (y + mproj->origin[Y]) * units;
	if (lon) *lon = xv/RE/RAD;
	if (lat) *lat = 2.0 * (atandeg(exp(yv/RE)) - 45);
	return TRUE;
	}

/*ARGSUSED*/
static	LOGICAL	meq_distort

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			lat,
	float			lon,
	float			*sx,
	float			*sy
	)

	{
	if (sx) *sx = 1;
	if (sy) *sy = 1;
	if (!mproj) return FALSE;

	if (lat > 90)  return FALSE;
	if (lat < -90) return FALSE;

	if (sx) *sx = 1 / fpa_cosdeg((double) lat);
	if (sy) *sy = 1 / fpa_cosdeg((double) lat);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     p s _ l l _ x y                                                  *
*     p s _ l l _ x y u                                                *
*     p s _ x y _ l l                                                  *
*     p s _ x y _ l l u                                                *
*                                                                      *
*     transform and inverse for Polar Stereographic projections        *
*                                                                      *
*     p s _ d i s t o r t                                              *
*                                                                      *
*     distortion for Polar Stereographic projections                   *
*                                                                      *
*     mp = mm = (1 + sin(lat_T)) / (1 + sin(lat))                      *
*                                                                      *
***********************************************************************/

static	LOGICAL	ps_ll_xy

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			lat,
	float			lon,
	float			*x,
	float			*y
	)

	{
	if (x) *x = 0;
	if (y) *y = 0;
	if (!mproj) return FALSE;

	if (lat > 90)  return FALSE;
	if (lat < -90) return FALSE;

	return ps_ll_xyu(mproj, lat, lon, x, y);
	}

static	LOGICAL	ps_ll_xyu

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			lat,
	float			lon,
	float			*x,
	float			*y
	)

	{
	double	dlon, units;
	double	phi;
	double	d, xv, yv;

	if (x) *x = 0;
	if (y) *y = 0;
	if (!mproj) return FALSE;

	units = mproj->definition.units;
	dlon  = lon - mproj->definition.lref;
	phi   = lat;
	if (mproj->projection.ProjectPole < 0) phi = -phi;
    d = mproj->projection.ProjectPdist * tandeg(45 - phi/2.0);

	xv =  d*fpa_sindeg(dlon);
	yv = -d*fpa_cosdeg(dlon);
	if (mproj->projection.ProjectPole < 0) yv = -yv;
	if (x) *x = xv/units - mproj->origin[X];
	if (y) *y = yv/units - mproj->origin[Y];
	return TRUE;
	}

static	LOGICAL	ps_xy_ll

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			x,
	float			y,
	float			*lat,
	float			*lon
	)

	{
	if (lat) *lat = 0;
	if (lon) *lon = 0;
	if (!mproj) return FALSE;

	if (!ps_xy_llu(mproj, x, y, lat, lon)) return FALSE;
	norm_lat_lon(lat, lon);
	return TRUE;
	}

static	LOGICAL	ps_xy_llu

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			x,
	float			y,
	float			*lat,
	float			*lon
	)

	{
	double	xv, yv, lref;
	double	d;
	double	pdist, units;

	if (lat) *lat = 0;
	if (lon) *lon = 0;
	if (!mproj) return FALSE;

	units = mproj->definition.units;
	xv = (x + mproj->origin[X]) * units;
	yv = (y + mproj->origin[Y]) * units;
	if (mproj->projection.ProjectPole < 0) yv = -yv;

	lref  = mproj->definition.lref;
	pdist = mproj->projection.ProjectPdist;
	d     = hypot(xv, yv);

	if (lat)
		{
		*lat = 90 - 2*fpa_atan2deg(d, pdist);
		if (mproj->projection.ProjectPole < 0) *lat = -(*lat);
		}

	if (lon)
		{
		if (xv == 0)
			{
			if (yv < 0)       *lon = lref;
			else if (yv == 0) *lon = lref + 90;
			else              *lon = lref + 180;
			}
		else                  *lon = lref + 90 + atan2deg(yv, xv);
		}

	return TRUE;
	}

/*ARGSUSED*/
static	LOGICAL	ps_distort

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			lat,
	float			lon,
	float			*sx,
	float			*sy
	)

	{
	double	top, bot;

	if (sx) *sx = 1;
	if (sy) *sy = 1;
	if (!mproj) return FALSE;

	if (lat > 90)  return FALSE;
	if (lat < -90) return FALSE;

	if (mproj->projection.ProjectPole > 0)
		{
		top = 1 + fpa_sindeg((double) mproj->projection.ProjectTrueLat);
		bot = 1 + fpa_sindeg((double) lat);
		}
	else
		{
		top = 1 - fpa_sindeg((double) mproj->projection.ProjectTrueLat);
		bot = 1 - fpa_sindeg((double) lat);
		}
	if (sx) *sx = top / bot;
	if (sy) *sy = top / bot;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     o s _ l l _ x y                                                  *
*     o s _ l l _ x y u                                                *
*     o s _ x y _ l l                                                  *
*     o s _ x y _ l l u                                                *
*                                                                      *
*     transform and inverse for Oblique Stereographic projections      *
*                                                                      *
*     o s _ d i s t o r t                                              *
*                                                                      *
*     distortion for Oblique Stereographic projections                 *
*                                                                      *
*     mp = mm = sec*RAD * (1 + cos(sec)) / sin(sec) / (1 + sin(h))     *
*                                                                      *
***********************************************************************/

static	LOGICAL	os_ll_xy

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			lat,
	float			lon,
	float			*x,
	float			*y
	)

	{
	if (x) *x = 0;
	if (y) *y = 0;
	if (!mproj) return FALSE;

	if (lat > 90)  return FALSE;
	if (lat < -90) return FALSE;

	return os_ll_xyu(mproj, lat, lon, x, y);
	}

static	LOGICAL	os_ll_xyu

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			lat,
	float			lon,
	float			*x,
	float			*y
	)

	{
	double	lref, dlon, alf, units;
	double	phi, h, cosp, sinp, cosl, sinl;
	double	d, xv, yv, xw, yw;

	if (x) *x = 0;
	if (y) *y = 0;
	if (!mproj) return FALSE;

	units = mproj->definition.units;
	dlon  = lon - mproj->projection.ProjectPlon;
	phi   = lat;

	/* Rotate point of tangency to pole */
	cosp = mproj->projection.ProjectCosPlat;
	sinp = mproj->projection.ProjectSinPlat;
	cosl = fpa_cosdeg(phi);
	sinl = fpa_sindeg(phi);
	if (dlon == 0)
		{
		if (sinl > sinp) alf = 180;
		else             alf = 0;
		}
	else
		{
		alf = fpa_atan2deg(sindeg(dlon)*cosl,
							-cosp*sinl + sinp*cosdeg(dlon)*cosl);
		}
	h  = asindeg(sinl*sinp + cosl*cosp*cosdeg(dlon));

    d  = mproj->projection.ProjectPdist * tandeg(45 - h/2.0);
	xv =  d*fpa_sindeg(alf);
	yv = -d*fpa_cosdeg(alf);
	xw = xv/units - mproj->origin[X];
	yw = yv/units - mproj->origin[Y];

	/* Rotate about origin until ref lon is vertical */
	lref = mproj->definition.lref;
	dlon = lref - mproj->projection.ProjectPlon;
	cosp = cosdeg(dlon);
	sinp = sindeg(dlon);
	if (x) *x =  xw*cosp + yw*sinp;
	if (y) *y = -xw*sinp + yw*cosp;
	return TRUE;
	}

static	LOGICAL	os_xy_ll

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			x,
	float			y,
	float			*lat,
	float			*lon
	)

	{
	if (lat) *lat = 0;
	if (lon) *lon = 0;
	if (!mproj) return FALSE;

	if (!os_xy_llu(mproj, x, y, lat, lon)) return FALSE;
	norm_lat_lon(lat, lon);
	return TRUE;
	}

static	LOGICAL	os_xy_llu

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			x,
	float			y,
	float			*lat,
	float			*lon
	)

	{
	double	xv, yv, lref, dlon, xw, yw;
	double	d, h, alf;
	double	pdist, units, sinp, cosp, cosl, sinl;

	if (lat) *lat = 0;
	if (lon) *lon = 0;
	if (!mproj) return FALSE;

	/* Rotate about origin until projection lon is vertical */
	lref = mproj->definition.lref;
	dlon = lref - mproj->projection.ProjectPlon;
	cosp = cosdeg(dlon);
	sinp = sindeg(dlon);
	xw =  x*cosp - y*sinp;
	yw =  x*sinp + y*cosp;

	units = mproj->definition.units;
	xv = (xw + mproj->origin[X]) * units;
	yv = (yw + mproj->origin[Y]) * units;

	pdist = mproj->projection.ProjectPdist;
	d     = hypot(xv, yv);

	h = 90 - 2*fpa_atan2deg(d, pdist);

	if (xv == 0)
		{
		if (yv < 0)       alf = 0;
		else if (yv == 0) alf = 90;
		else              alf = 180;
		}
	else                  alf = 90 + atan2deg(yv, xv);

	/* Rotate pole to point of tangency */
	cosp = mproj->projection.ProjectCosPlat;
	sinp = mproj->projection.ProjectSinPlat;
	cosl = fpa_cosdeg(h);
	sinl = fpa_sindeg(h);
	dlon = (alf == 0)? 0:
			fpa_atan2deg(sindeg(alf)*cosl, cosp*sinl + sinp*cosdeg(alf)*cosl);
	if (lon) *lon = dlon + mproj->projection.ProjectPlon;
	if (lat) *lat = asindeg(sinp*sinl - cosp*cosl*cosdeg(alf));

	return TRUE;
	}

/*ARGSUSED*/
static	LOGICAL	os_distort

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			lat,
	float			lon,
	float			*sx,
	float			*sy
	)

	{
	double	dlon, sins, coss, ss;
	double	phi, h, cosp, sinp, cosl, sinl;

	if (sx) *sx = 1;
	if (sy) *sy = 1;
	if (!mproj) return FALSE;

	if (lat > 90)  return FALSE;
	if (lat < -90) return FALSE;

	/* Rotate point of tangency to pole (only need lat) */
	dlon  = lon - mproj->projection.ProjectPlon;
	phi   = lat;
	cosp = mproj->projection.ProjectCosPlat;
	sinp = mproj->projection.ProjectSinPlat;
	cosl = fpa_cosdeg(phi);
	sinl = fpa_sindeg(phi);
	h    = asindeg(sinl*sinp + cosl*cosp*cosdeg(dlon));

	sins = fpa_sindeg((double) mproj->projection.ProjectSecant);
	coss = fpa_cosdeg((double) mproj->projection.ProjectSecant);
	ss   = RAD * mproj->projection.ProjectSecant;
	if (sx) *sx = ss * (1 + coss) / sins / (1 + fpa_sindeg(h));
	if (sy) *sy = ss * (1 + coss) / sins / (1 + fpa_sindeg(h));
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     l c _ l l _ x y                                                  *
*     l c _ l l _ x y u                                                *
*     l c _ x y _ l l                                                  *
*     l c _ x y _ l l u                                                *
*                                                                      *
*     transform and inverse for Secant or Tangent Lambert Conformal    *
*     projections (Tangent => phi0=phi1=phi2)                          *
*                                                                      *
*     l c _ d i s t o r t                                              *
*                                                                      *
*     distortion for Secant or Tangent Lambert Conformal projections   *
*                                                                      *
*               cos(phi1) * [ tan(45 - lat/2) ]^sin(phi0)              *
*     mp = mm = ----------------------------------------               *
*               cos(lat) * [ tan(45 - phi1/2) ]^sin(phi0)              *
*                                                                      *
***********************************************************************/

static	LOGICAL	lc_ll_xy

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			lat,
	float			lon,
	float			*x,
	float			*y
	)

	{
	if (x) *x = 0;
	if (y) *y = 0;
	if (!mproj) return FALSE;

	if (lat > 90)  return FALSE;
	if (lat < -90) return FALSE;

	/* Normalize longitude wrt center of projection */
	while (lon <= mproj->clon-180) lon += 360;
	while (lon >  mproj->clon+180) lon -= 360;

	return lc_ll_xyu(mproj, lat, lon, x, y);
	}

static	LOGICAL	lc_ll_xyu

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			lat,
	float			lon,
	float			*x,
	float			*y
	)

	{
	double	dlon, phi, t1, units, xv, yv;
	double	sinphi0, rho1, rho, psi, theta;

	if (x) *x = 0;
	if (y) *y = 0;
	if (!mproj) return FALSE;

	units   = mproj->definition.units;
	dlon    = lon - mproj->definition.lref;
	phi     = lat;
	sinphi0 = mproj->projection.ProjectSinPhi0;
	theta   = dlon * sinphi0;
	psi     = mproj->projection.ProjectPsi;

	t1   = tandeg(45 - phi/2.0);
	rho  = psi * pow(t1, sinphi0);
	rho1 = mproj->projection.ProjectRho1;

	xv = rho * sindeg(theta);
	yv = rho1 - rho*cosdeg(theta);

	if (x) *x = xv/units - mproj->origin[X];
	if (y) *y = yv/units - mproj->origin[Y];
	return TRUE;
	}

static	LOGICAL	lc_xy_ll

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			x,
	float			y,
	float			*lat,
	float			*lon
	)

	{
	if (lat) *lat = 0;
	if (lon) *lon = 0;
	if (!mproj) return FALSE;

	if (!lc_xy_llu(mproj, x, y, lat, lon)) return FALSE;
	norm_lat_lon(lat, lon);
	return TRUE;
	}

static	LOGICAL	lc_xy_llu

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			x,
	float			y,
	float			*lat,
	float			*lon
	)

	{
	double	xv, yv, lref, dlat, dlon;
	double	sinphi0, rho1, theta, psi;
	double	num, units;

	if (lat) *lat = 0;
	if (lon) *lon = 0;
	if (!mproj) return FALSE;

	units = mproj->definition.units;
	lref  = mproj->definition.lref;
	xv = (mproj->origin[X] + x) * units;
	yv = (mproj->origin[Y] + y) * units;

	sinphi0 = mproj->projection.ProjectSinPhi0;
	rho1    = mproj->projection.ProjectRho1;
	psi     = mproj->projection.ProjectPsi;

	if (xv == 0.0)
		{
		dlon = lref;
		num  = (rho1 - yv)/psi;
		if (num < 0.0) num = 0.0;
		num  = pow(num, 1/sinphi0);
		dlat = 90 - 2.0*atandeg(num);
		}
	else
		{
		if (yv == rho1 && xv > 0.0) theta =  90.0;
		else if (yv == rho1)        theta = -90.0;
		else                        theta = atandeg(xv/(rho1-yv));

		dlon = lref + (1/sinphi0)*theta;
		num  = xv/(psi*sindeg(theta));

		if (num < 0.0) num = 0.0;
		num  = pow(num, 1/sinphi0);
		dlat = 90 - 2.0*atandeg(num);
		}

	if (lon) *lon = dlon;
	if (lat) *lat = dlat;
	return TRUE;
	}

/*ARGSUSED*/
static	LOGICAL	lc_distort

	(
	const MAP_PROJ	*mproj,	/* given map projection */
	float			lat,
	float			lon,
	float			*sx,
	float			*sy
	)

	{
	double	top, bot;

	if (sx) *sx = 1;
	if (sy) *sy = 1;
	if (!mproj) return FALSE;

	if (lat > 90)  return FALSE;
	if (lat < -90) return FALSE;

	top = fpa_cosdeg((double) mproj->projection.ProjectLat1)
			* tandeg(45.0 - 0.5*lat);
	bot = fpa_cosdeg((double) lat)
			* tandeg(45.0 - 0.5*mproj->projection.ProjectLat1);
	if (sx) *sx = pow(top/bot, (double) mproj->projection.ProjectSinPhi0);
	if (sy) *sy = pow(top/bot, (double) mproj->projection.ProjectSinPhi0);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Testing static routines)                *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

#ifdef STANDALONE

/**********************************************************************
 *** routine to display projection information                      ***
 **********************************************************************/

static	void	display_projection(mproj)

MAP_PROJ	*mproj;			/* map projection */
	{
	fprintf(stdout, "  Projection: %s",
		which_projection_name(mproj->projection.type));
	switch (mproj->projection.type)
		{
		/* No projection (orthographic) */
		case ProjectNone:
			fprintf(stdout, "\n");
			break;

		/* Lat-Lon */
		case ProjectLatLon:
			fprintf(stdout, "\n");
			break;

		/* Rotated Lat-Lon */
		case ProjectLatLonAng:
			fprintf(stdout, "  Plat: %.2f", mproj->projection.ProjectPlat);
			fprintf(stdout, "  Plon: %.2f", mproj->projection.ProjectPlon);
			fprintf(stdout, "  Origin: %.2f\n",
											mproj->projection.ProjectOrigin);
			break;

		/* Plate-Caree */
		case ProjectPlateCaree:
			fprintf(stdout, "\n");
			break;

		/* Rectangular (Carte Parallelogrammatique) */
		case ProjectRectangular:
			fprintf(stdout, "\n");
			break;

		/* Mercator */
		case ProjectMercatorEq:
			fprintf(stdout, "\n");
			break;

		/* Polar stereographic */
		case ProjectPolarSt:
			fprintf(stdout, "  Pole: %.2f", mproj->projection.ProjectPole);
			fprintf(stdout, "  TrueLat: %.2f\n",
											mproj->projection.ProjectTrueLat);
			break;

		/* Oblique stereographic */
		case ProjectObliqueSt:
			fprintf(stdout, "  Plat: %.2f", mproj->projection.ProjectPlat);
			fprintf(stdout, "  Plon: %.2f", mproj->projection.ProjectPlon);
			fprintf(stdout, "  Secant: %.2f\n",
											mproj->projection.ProjectSecant);
			break;

		/* Lambert conformal (secant or tangent) */
		case ProjectLambertConf:
			fprintf(stdout, "  Lat1: %.2f", mproj->projection.ProjectLat1);
			fprintf(stdout, "  Lat2: %.2f\n", mproj->projection.ProjectLat2);
			break;

		/* Unrecognized */
		default:
			break;
		}

	fprintf(stdout, "  Basemap  olat: %f", mproj->definition.olat);
	fprintf(stdout, "  olon: %f", mproj->definition.olon);
	fprintf(stdout, "  lref: %f\n", mproj->definition.lref);
	fprintf(stdout, "           xorg: %f", mproj->definition.xorg);
	fprintf(stdout, "  yorg: %f\n", mproj->definition.yorg);
	fprintf(stdout, "           xlen: %f", mproj->definition.xlen);
	fprintf(stdout, "  ylen: %f", mproj->definition.ylen);
	fprintf(stdout, "  units: %f\n", mproj->definition.units);

	fprintf(stdout, "  Map origin  origin[X]: %f", mproj->origin[X]);
	fprintf(stdout, "  origin[Y]: %f\n\n", mproj->origin[Y]);
	}

/**********************************************************************
 *** routine to test pos_to_ll                                      ***
 **********************************************************************/

static	void	test_pos_to_ll(mproj, xpos, ypos)

MAP_PROJ	*mproj;			/* map projection */
float		xpos, ypos;		/* x/y position of point */
	{
	POINT	pos;
	float	lat, lon;

	set_point(pos, xpos, ypos);
	(void) pos_to_ll(mproj, pos, &lat, &lon);
	fprintf(stdout, "Point: (%f,%f) -> %f %f\n", pos[X], pos[Y], lat, lon);
	}

/**********************************************************************
 *** routine to test ll_to_pos                                      ***
 **********************************************************************/

static	void	test_ll_to_pos(mproj, lat, lon)

MAP_PROJ	*mproj;			/* map projection */
float		lat, lon;		/* lat/lon position of point */
	{
	POINT	pos;

	(void) ll_to_pos(mproj, lat, lon, pos);
	fprintf(stdout, "LatLon: %f %f -> (%f,%f)\n", lat, lon, pos[X], pos[Y]);
	}

/**********************************************************************
 *** routine to test scaled_map_projection                          ***
 **********************************************************************/

static	void	test_scaled_map_projection(mproj, scale)

MAP_PROJ	*mproj;			/* map projection */
float		scale;			/* scaling factor */
	{
	MAP_PROJ	sproj;

	fprintf(stdout, " Scaling factor: %f\n", scale);
	fprintf(stdout, "  Original Basemap  olat: %f", mproj->definition.olat);
	fprintf(stdout, "  olon: %f", mproj->definition.olon);
	fprintf(stdout, "  lref: %f\n", mproj->definition.lref);
	fprintf(stdout, "         xorg: %f", mproj->definition.xorg);
	fprintf(stdout, "  yorg: %f\n", mproj->definition.yorg);
	fprintf(stdout, "         xlen: %f", mproj->definition.xlen);
	fprintf(stdout, "  ylen: %f", mproj->definition.ylen);
	fprintf(stdout, "  units: %f\n", mproj->definition.units);

	fprintf(stdout, "  Original Grid definition  nx: %d", mproj->grid.nx);
	fprintf(stdout, "  ny: %d", mproj->grid.ny);
	fprintf(stdout, "  units: %f\n", mproj->grid.units);
	fprintf(stdout, "         gridlen: %f", mproj->grid.gridlen);
	fprintf(stdout, "  xgrid: %f", mproj->grid.xgrid);
	fprintf(stdout, "  ygrid: %f\n", mproj->grid.ygrid);

	fprintf(stdout, "  Original Map origin  origin[X]: %f", mproj->origin[X]);
	fprintf(stdout, "  origin[Y]: %f\n\n", mproj->origin[Y]);

	(void) scaled_map_projection(&sproj, mproj, scale);
	fprintf(stdout, "  Scaled Basemap  olat: %f", sproj.definition.olat);
	fprintf(stdout, "  olon: %f", sproj.definition.olon);
	fprintf(stdout, "  lref: %f\n", sproj.definition.lref);
	fprintf(stdout, "         xorg: %f", sproj.definition.xorg);
	fprintf(stdout, "  yorg: %f\n", sproj.definition.yorg);
	fprintf(stdout, "         xlen: %f", sproj.definition.xlen);
	fprintf(stdout, "  ylen: %f", sproj.definition.ylen);
	fprintf(stdout, "  units: %f\n", sproj.definition.units);

	fprintf(stdout, "  Scaled Grid definition  nx: %d", sproj.grid.nx);
	fprintf(stdout, "  ny: %d", sproj.grid.ny);
	fprintf(stdout, "  units: %f\n", sproj.grid.units);
	fprintf(stdout, "         gridlen: %f", sproj.grid.gridlen);
	fprintf(stdout, "  xgrid: %f", sproj.grid.xgrid);
	fprintf(stdout, "  ygrid: %f\n", sproj.grid.ygrid);

	fprintf(stdout, "  Scaled Map origin  origin[X]: %f", sproj.origin[X]);
	fprintf(stdout, "  origin[Y]: %f\n\n", sproj.origin[Y]);
	}

/**********************************************************************
 *** routine to test evaluation_map_projection                      ***
 **********************************************************************/

/* Internal variables to test routines */
static	int		TestNumber       = 1;
static	POINT	TestPointArray[] = { ZERO_POINT };

static	void	test_evaluation_map_projection(mproj, pos)

MAP_PROJ	*mproj;			/* map projection */
POINT		pos;			/* position on map projection */
	{
	MAP_PROJ	eproj;
	POINT		*epos;

	fprintf(stdout, "  Original Basemap  olat: %f", mproj->definition.olat);
	fprintf(stdout, "  olon: %f", mproj->definition.olon);
	fprintf(stdout, "  lref: %f\n", mproj->definition.lref);
	fprintf(stdout, "         xorg: %f", mproj->definition.xorg);
	fprintf(stdout, "  yorg: %f\n", mproj->definition.yorg);
	fprintf(stdout, "         xlen: %f", mproj->definition.xlen);
	fprintf(stdout, "  ylen: %f", mproj->definition.ylen);
	fprintf(stdout, "  units: %f\n", mproj->definition.units);

	fprintf(stdout, "  Original Grid definition  nx: %d", mproj->grid.nx);
	fprintf(stdout, "  ny: %d", mproj->grid.ny);
	fprintf(stdout, "  units: %f\n", mproj->grid.units);
	fprintf(stdout, "         gridlen: %f", mproj->grid.gridlen);
	fprintf(stdout, "  xgrid: %f", mproj->grid.xgrid);
	fprintf(stdout, "  ygrid: %f\n", mproj->grid.ygrid);

	fprintf(stdout, "  Original Map origin  origin[X]: %f", mproj->origin[X]);
	fprintf(stdout, "  origin[Y]: %f\n\n", mproj->origin[Y]);

	fprintf(stdout, "  Original Position  pos[X]: %f", pos[X]);
	fprintf(stdout, "  pos[Y]: %f\n\n", pos[Y]);

	(void) copy_point(TestPointArray[0], pos);
	if ( !evaluation_map_projection(mproj, TestNumber, TestPointArray,
			&eproj, &epos) )
		{
		fprintf(stdout, "  Failure in evaluation_map_projection\n\n");
		return;
		}

	fprintf(stdout, "  Evaluation Basemap  olat: %f", eproj.definition.olat);
	fprintf(stdout, "  olon: %f", eproj.definition.olon);
	fprintf(stdout, "  lref: %f\n", eproj.definition.lref);
	fprintf(stdout, "         xorg: %f", eproj.definition.xorg);
	fprintf(stdout, "  yorg: %f\n", eproj.definition.yorg);
	fprintf(stdout, "         xlen: %f", eproj.definition.xlen);
	fprintf(stdout, "  ylen: %f", eproj.definition.ylen);
	fprintf(stdout, "  units: %f\n", eproj.definition.units);

	fprintf(stdout, "  Evaluation Grid definition  nx: %d", eproj.grid.nx);
	fprintf(stdout, "  ny: %d", eproj.grid.ny);
	fprintf(stdout, "  units: %f\n", eproj.grid.units);
	fprintf(stdout, "         gridlen: %f", eproj.grid.gridlen);
	fprintf(stdout, "  xgrid: %f", eproj.grid.xgrid);
	fprintf(stdout, "  ygrid: %f\n", eproj.grid.ygrid);

	fprintf(stdout, "  Evaluation Map origin  origin[X]: %f", eproj.origin[X]);
	fprintf(stdout, "  origin[Y]: %f\n\n", eproj.origin[Y]);

	fprintf(stdout, "  Evaluation Position  epos[X]: %f", epos[0][X]);
	fprintf(stdout, "  epos[Y]: %f\n\n", epos[0][Y]);
	}

/**********************************************************************
 *** routine to test great_circle_distance                          ***
 **********************************************************************/

static	void	test_great_circle_distance(mproj, spos, epos)

MAP_PROJ	*mproj;			/* map projection */
POINT		spos;			/* start position on map projection */
POINT		epos;			/* end position on map projection */
	{
	float		lat, lon, dist;

	(void) pos_to_ll(mproj, spos, &lat, &lon);
	fprintf(stdout, "  Start Position  X/Y: %.2f %.2f  Lat/Lon: %.2f %.2f\n",
			spos[X], spos[Y], lat, lon);

	(void) pos_to_ll(mproj, epos, &lat, &lon);
	fprintf(stdout, "  End Position    X/Y: %.2f %.2f  Lat/Lon: %.2f %.2f\n",
			epos[X], epos[Y], lat, lon);

	dist = great_circle_distance(mproj, spos, epos);
	fprintf(stdout, "  Distance (m): %.2f  Circumference of Earth (m): %.2f\n",
			dist, 2.0*M_PI*RE);
	}

/**********************************************************************
 ***  m a i n   - Stand-alone test program                          ***
 **********************************************************************/

void	main()
	{
	PROJ_DEF	proj;
	MAP_DEF		map;
	GRID_DEF	grid;
	MAP_PROJ	mproj;
	POINT		pos, spos, epos;

	fpalib_license(FpaAccessLib);

	/* Test the familiar North Polar Stereographic */
	fprintf(stdout, "\nNorth Polar Stereographic:\n");
	(void) define_projection(&proj, ProjectPolarSt, 90., 60., 0., 0., 0.);
	map.olat = 26.75;
	map.olon = -90;
	map.lref = -85;
	map.xorg = 0;
	map.yorg = 0;
	map.xlen = 4000;
	map.ylen = 5000;
	map.units = 1000;
	(void) define_map_projection(&mproj, &proj, &map, NullGridDef);
	(void) display_projection(&mproj);

	test_pos_to_ll(&mproj, 0., 0.);
	test_pos_to_ll(&mproj, 1000., 0.);
	test_pos_to_ll(&mproj, 4000., 0.);
	test_pos_to_ll(&mproj, 0., 1000.);
	test_pos_to_ll(&mproj, 0., 5000.);

	test_ll_to_pos(&mproj, 26.75, -90.0);
	test_ll_to_pos(&mproj, 26.75, -85.0);
	test_ll_to_pos(&mproj, 26.75, -80.0);

	/* Test the same North Polar Stereographic with origin at top left */
	fprintf(stdout, "\nNorth Polar Stereographic with origin at top left:\n");
	(void) define_projection(&proj, ProjectPolarSt, 90., 60., 0., 0., 0.);
	map.olat = 67.38;
	map.olon = -100.57;
	map.lref = -85;
	map.xorg = 0;
	map.yorg = 5000;
	map.xlen = 4000;
	map.ylen = 5000;
	map.units = 1000;
	(void) define_map_projection(&mproj, &proj, &map, NullGridDef);
	(void) display_projection(&mproj);

	test_pos_to_ll(&mproj, 0., 0.);
	test_pos_to_ll(&mproj, 1000., 0.);
	test_pos_to_ll(&mproj, 4000., 0.);
	test_pos_to_ll(&mproj, 0., 1000.);
	test_pos_to_ll(&mproj, 0., 5000.);

	test_ll_to_pos(&mproj, 26.75, -90.0);
	test_ll_to_pos(&mproj, 26.75, -85.0);
	test_ll_to_pos(&mproj, 26.75, -80.0);

	/* Test the North Polar Stereographic with offsets */
	fprintf(stdout, "\nNorth Polar Stereographic with offset:\n");
	(void) define_projection(&proj, ProjectPolarSt, 90., 60., 0., 0., 0.);
	map.olat = 26.75;
	map.olon = -90;
	map.lref = -85;
	map.xorg = 1000;
	map.yorg = 1000;
	map.xlen = 4000;
	map.ylen = 5000;
	map.units = 1000;
	(void) define_map_projection(&mproj, &proj, &map, NullGridDef);
	(void) display_projection(&mproj);

	test_pos_to_ll(&mproj, 1000., 1000.);
	test_pos_to_ll(&mproj, 2000., 1000.);
	test_pos_to_ll(&mproj, 3000., 1000.);
	test_pos_to_ll(&mproj, 1000., 2000.);
	test_pos_to_ll(&mproj, 1000., 4000.);

	test_ll_to_pos(&mproj, 26.75, -90.0);
	test_ll_to_pos(&mproj, 26.75, -85.0);
	test_ll_to_pos(&mproj, 26.75, -80.0);

	/* Test the South Polar Stereographic with offset */
	fprintf(stdout, "\nSouth Polar Stereographic with offset:\n");
	(void) define_projection(&proj, ProjectPolarSt, -90., -60., 0., 0., 0.);
	map.olat = -26.75;
	map.olon = -90;
	map.lref = -85;
	map.xorg = 0;
	map.yorg = 5000;
	map.xlen = 4000;
	map.ylen = 5000;
	map.units = 1000;
	(void) define_map_projection(&mproj, &proj, &map, NullGridDef);
	(void) display_projection(&mproj);

	test_pos_to_ll(&mproj, 0., 5000.);
	test_pos_to_ll(&mproj, 1000., 5000.);
	test_pos_to_ll(&mproj, 4000., 5000.);
	test_pos_to_ll(&mproj, 0., 4000.);
	test_pos_to_ll(&mproj, 0., 0.);

	test_ll_to_pos(&mproj, -26.75, -90.0);
	test_ll_to_pos(&mproj, -26.75, -85.0);
	test_ll_to_pos(&mproj, -26.75, -80.0);

	/* Test the Oblique Stereographic */
	fprintf(stdout, "\nOblique Stereographic:\n");
	(void) define_projection(&proj, ProjectObliqueSt, 44.5, -67.75, 0., 0., 0.);
	map.olat = 44.5;
	map.olon = -67.75;
	map.lref = -67.75;
	map.xorg = 0;
	map.yorg = 0;
	map.xlen = 1000;
	map.ylen = 1000;
	map.units = 1000;
	(void) define_map_projection(&mproj, &proj, &map, NullGridDef);
	(void) display_projection(&mproj);

	test_pos_to_ll(&mproj, 0., 0.);
	test_pos_to_ll(&mproj, 1., 0.);
	test_pos_to_ll(&mproj, 2., 0.);
	test_pos_to_ll(&mproj, 0., 1.);
	test_pos_to_ll(&mproj, 0., 2.);

	test_ll_to_pos(&mproj, 44.5, -67.75);
	test_ll_to_pos(&mproj, 44.5, -67.00);
	test_ll_to_pos(&mproj, 44.5, -66.0);
	test_ll_to_pos(&mproj, 45.0, -67.75);
	test_ll_to_pos(&mproj, 45.5, -67.75);

	/* Test the Mercator */
	fprintf(stdout, "\nMercator Equatorial:\n");
	(void) define_projection(&proj, ProjectMercatorEq, 0., 0., 0., 0., 0.);
	map.olat = 26.75;
	map.olon = -90;
	map.lref = -85;	/* Should be ignored */
	map.xorg = 0;
	map.yorg = 0;
	map.xlen = 4000;
	map.ylen = 5000;
	map.units = 1000;
	(void) define_map_projection(&mproj, &proj, &map, NullGridDef);
	(void) display_projection(&mproj);

	test_pos_to_ll(&mproj, 0., 0.);
	test_pos_to_ll(&mproj, 1000., 0.);
	test_pos_to_ll(&mproj, 4000., 0.);
	test_pos_to_ll(&mproj, 0., 1000.);
	test_pos_to_ll(&mproj, 0., 5000.);
	test_pos_to_ll(&mproj, 45000., 0.);
	test_pos_to_ll(&mproj, -25000., 0.);

	test_ll_to_pos(&mproj, 26.75, -90.0);
	test_ll_to_pos(&mproj, 26.75, -85.0);
	test_ll_to_pos(&mproj, 26.75, -80.0);
	test_ll_to_pos(&mproj, 26.75, -440.0);
	test_ll_to_pos(&mproj, 26.75, -270.0);
	test_ll_to_pos(&mproj, 26.75, -200.0);

	/* Test the Mercator at the Greenwich meridian */
	fprintf(stdout, "\nMercator Equatorial at Greenwich:\n");
	(void) define_projection(&proj, ProjectMercatorEq, 0., 0., 0., 0., 0.);
	map.olat = 0.0;
	map.olon = 0.0;
	map.lref = 0.0;	/* Should be ignored */
	map.xorg = 2000;
	map.yorg = 2500;
	map.xlen = 4000;
	map.ylen = 5000;
	map.units = 1000;
	(void) define_map_projection(&mproj, &proj, &map, NullGridDef);
	(void) display_projection(&mproj);

	test_pos_to_ll(&mproj, 0., 0.);
	test_pos_to_ll(&mproj, 1000., 0.);
	test_pos_to_ll(&mproj, 4000., 0.);
	test_pos_to_ll(&mproj, 0., 1000.);
	test_pos_to_ll(&mproj, 0., 5000.);
	test_pos_to_ll(&mproj, 45000., 0.);
	test_pos_to_ll(&mproj, -25000., 0.);

	test_ll_to_pos(&mproj, 0.0, 0.0);
	test_ll_to_pos(&mproj, -10.0, -10.0);
	test_ll_to_pos(&mproj, 10.0, 10.0);
	test_ll_to_pos(&mproj, 10.0, 370.0);
	test_ll_to_pos(&mproj, 10.0, -270.0);
	test_ll_to_pos(&mproj, 10.0, -90.0);

	/* Test the Lambert Conformal */
	fprintf(stdout, "\nLambert Conformal:\n");
	(void) define_projection(&proj, ProjectLambertConf, 20., 60., 0., 0., 0.);
	map.olat =   40.0;
	map.olon =  -95.0;
	map.lref =  -95.0;
	map.xorg =   3000;
	map.yorg =   2500;
	map.xlen =   6000;
	map.ylen =   5000;
	map.units =  1000;
	(void) define_map_projection(&mproj, &proj, &map, NullGridDef);
	(void) display_projection(&mproj);

	test_pos_to_ll(&mproj,     0.,  10000.);
	test_pos_to_ll(&mproj,  3000.,  10000.);

	test_pos_to_ll(&mproj,     0.,      0.);
	test_pos_to_ll(&mproj,  3000.,      0.);
	test_pos_to_ll(&mproj,  6000.,      0.);
	test_pos_to_ll(&mproj,     0.,   2500.);
	test_pos_to_ll(&mproj,  3000.,   2500.);
	test_pos_to_ll(&mproj,  6000.,   2500.);
	test_pos_to_ll(&mproj,     0.,   5000.);
	test_pos_to_ll(&mproj,  3000.,   5000.);
	test_pos_to_ll(&mproj,  6000.,   5000.);
	test_pos_to_ll(&mproj, -2000.,      0.);
	test_pos_to_ll(&mproj,  8000.,      0.);
	test_pos_to_ll(&mproj,  3000.,-200000.);
	test_pos_to_ll(&mproj,  3000., -30000.);
	test_pos_to_ll(&mproj,  3000., -15000.);
	test_pos_to_ll(&mproj,  3000.,  -5000.);
	test_pos_to_ll(&mproj,  3000.,  -2500.);
	test_pos_to_ll(&mproj,  3000.,   7500.);
	test_pos_to_ll(&mproj,  3000.,  10000.);
	test_pos_to_ll(&mproj,  3010.,-200000.);
	test_pos_to_ll(&mproj,  3010., -30000.);
	test_pos_to_ll(&mproj,  3010., -15000.);
	test_pos_to_ll(&mproj,  3010.,  -5000.);
	test_pos_to_ll(&mproj,  3010.,  -2500.);
	test_pos_to_ll(&mproj,  3010.,   7500.);
	test_pos_to_ll(&mproj,  3010.,  10000.);
	test_pos_to_ll(&mproj,     0.,-200000.);
	test_pos_to_ll(&mproj,     0., -30000.);
	test_pos_to_ll(&mproj,     0., -15000.);
	test_pos_to_ll(&mproj,     0.,  -5000.);
	test_pos_to_ll(&mproj,     0.,  -2500.);
	test_pos_to_ll(&mproj,     0.,   7500.);
	test_pos_to_ll(&mproj,     0.,  10000.);
	test_pos_to_ll(&mproj,     5.,-200000.);
	test_pos_to_ll(&mproj,     5., -30000.);
	test_pos_to_ll(&mproj,     5., -15000.);
	test_pos_to_ll(&mproj,     5.,  -5000.);
	test_pos_to_ll(&mproj,     5.,  -2500.);
	test_pos_to_ll(&mproj,     5.,   7500.);
	test_pos_to_ll(&mproj,     5.,  10000.);
	test_pos_to_ll(&mproj, -6000.,      0.);
	test_pos_to_ll(&mproj,  3000.,  -1400.);
	test_pos_to_ll(&mproj,  3000.,  -1900.);
	test_pos_to_ll(&mproj,  3000.,  -2150.);
	test_pos_to_ll(&mproj,  3000.,  -2650.);

	test_ll_to_pos(&mproj,  40.0,  -95.0);
	test_ll_to_pos(&mproj,  20.0, -120.0);
	test_ll_to_pos(&mproj,  20.0,  -70.0);
	test_ll_to_pos(&mproj,  60.0, -120.0);
	test_ll_to_pos(&mproj,  60.0,  -70.0);
	test_ll_to_pos(&mproj,  80.0,  -95.0);
	test_ll_to_pos(&mproj,  10.0,  -95.0);
	test_ll_to_pos(&mproj,   5.0,  -95.0);
	test_ll_to_pos(&mproj,   1.0,  -95.0);
	test_ll_to_pos(&mproj,   0.0,  -95.0);
	test_ll_to_pos(&mproj,  -1.0,  -95.0);
	test_ll_to_pos(&mproj,  -5.0,  -95.0);
	test_ll_to_pos(&mproj, -10.0,  -95.0);
	test_ll_to_pos(&mproj, -40.0,  -95.0);
	test_ll_to_pos(&mproj, -80.0,  -95.0);

	/* Test the Lambert Conformal ... southern hemisphere */
	fprintf(stdout, "\nLambert Conformal:\n");
	(void) define_projection(&proj, ProjectLambertConf, -20., -60., 0., 0., 0.);
	map.olat =  -40.0;
	map.olon =  -95.0;
	map.lref =  -95.0;
	map.xorg =   3000;
	map.yorg =   2500;
	map.xlen =   6000;
	map.ylen =   5000;
	map.units =  1000;
	(void) define_map_projection(&mproj, &proj, &map, NullGridDef);
	(void) display_projection(&mproj);

	test_pos_to_ll(&mproj,     0.,     0.);
	test_pos_to_ll(&mproj,  3000.,     0.);
	test_pos_to_ll(&mproj,  6000.,     0.);
	test_pos_to_ll(&mproj,     0.,  2500.);
	test_pos_to_ll(&mproj,  3000.,  2500.);
	test_pos_to_ll(&mproj,  6000.,  2500.);
	test_pos_to_ll(&mproj,     0.,  5000.);
	test_pos_to_ll(&mproj,  3000.,  5000.);
	test_pos_to_ll(&mproj,  6000.,  5000.);
	test_pos_to_ll(&mproj, -2000.,     0.);
	test_pos_to_ll(&mproj,  8000.,     0.);
	test_pos_to_ll(&mproj,  3000., -5000.);
	test_pos_to_ll(&mproj,  3000.,  7500.);
	test_pos_to_ll(&mproj,  3000., 10000.);
	test_pos_to_ll(&mproj,  3000., 20000.);
	test_pos_to_ll(&mproj,  3000., 50000.);
	test_pos_to_ll(&mproj,  3000.,200000.);
	test_pos_to_ll(&mproj,  2995., -5000.);
	test_pos_to_ll(&mproj,  2995.,  7500.);
	test_pos_to_ll(&mproj,  2995., 10000.);
	test_pos_to_ll(&mproj,  2995., 20000.);
	test_pos_to_ll(&mproj,  2995., 50000.);
	test_pos_to_ll(&mproj,  2995.,200000.);
	test_pos_to_ll(&mproj,     0., -5000.);
	test_pos_to_ll(&mproj,     0.,  7500.);
	test_pos_to_ll(&mproj,     0., 10000.);
	test_pos_to_ll(&mproj,     0., 20000.);
	test_pos_to_ll(&mproj,     0., 50000.);
	test_pos_to_ll(&mproj,     0.,200000.);
	test_pos_to_ll(&mproj,    -5., -5000.);
	test_pos_to_ll(&mproj,    -5.,  7500.);
	test_pos_to_ll(&mproj,    -5., 10000.);
	test_pos_to_ll(&mproj,    -5., 50000.);
	test_pos_to_ll(&mproj,    -5.,200000.);
	test_pos_to_ll(&mproj, -6000.,     0.);
	test_pos_to_ll(&mproj,  3000.,  7125.);
	test_pos_to_ll(&mproj,  3000.,  7625.);
	test_pos_to_ll(&mproj,  3000.,  7900.);
	test_pos_to_ll(&mproj,  3000.,  8400.);

	test_ll_to_pos(&mproj, -40.0,  -95.0);
	test_ll_to_pos(&mproj, -20.0, -120.0);
	test_ll_to_pos(&mproj, -20.0,  -70.0);
	test_ll_to_pos(&mproj, -60.0, -120.0);
	test_ll_to_pos(&mproj, -60.0,  -70.0);
	test_ll_to_pos(&mproj, -80.0,  -95.0);
	test_ll_to_pos(&mproj, -10.0,  -95.0);
	test_ll_to_pos(&mproj,  -5.0,  -95.0);
	test_ll_to_pos(&mproj,  -1.0,  -95.0);
	test_ll_to_pos(&mproj,   0.0,  -95.0);
	test_ll_to_pos(&mproj,   1.0,  -95.0);
	test_ll_to_pos(&mproj,   5.0,  -95.0);
	test_ll_to_pos(&mproj,  10.0,  -95.0);
	test_ll_to_pos(&mproj,  40.0,  -95.0);
	test_ll_to_pos(&mproj,  80.0,  -95.0);

	/* Test the Lambert Conformal ... southern hemisphere */
	fprintf(stdout, "\nLambert Conformal:\n");
	(void) define_projection(&proj, ProjectLambertConf, 5., -85., 0., 0., 0.);
	map.olat =  -40.0;
	map.olon =  -95.0;
	map.lref =  -95.0;
	map.xorg =   3000;
	map.yorg =   2500;
	map.xlen =   6000;
	map.ylen =   5000;
	map.units =  1000;
	(void) define_map_projection(&mproj, &proj, &map, NullGridDef);
	(void) display_projection(&mproj);

	test_pos_to_ll(&mproj,     0.,     0.);
	test_pos_to_ll(&mproj,  3000.,     0.);
	test_pos_to_ll(&mproj,  6000.,     0.);
	test_pos_to_ll(&mproj,     0.,  2500.);
	test_pos_to_ll(&mproj,  3000.,  2500.);
	test_pos_to_ll(&mproj,  6000.,  2500.);
	test_pos_to_ll(&mproj,     0.,  5000.);
	test_pos_to_ll(&mproj,  3000.,  5000.);
	test_pos_to_ll(&mproj,  6000.,  5000.);
	test_pos_to_ll(&mproj, -2000.,     0.);
	test_pos_to_ll(&mproj,  8000.,     0.);
	test_pos_to_ll(&mproj,  3000., -5000.);
	test_pos_to_ll(&mproj,  3000.,  7500.);
	test_pos_to_ll(&mproj,  3000., 10000.);
	test_pos_to_ll(&mproj,  3000., 20000.);
	test_pos_to_ll(&mproj,  3000., 50000.);
	test_pos_to_ll(&mproj,  3000.,200000.);
	test_pos_to_ll(&mproj,  2995., -5000.);
	test_pos_to_ll(&mproj,  2995.,  7500.);
	test_pos_to_ll(&mproj,  2995., 10000.);
	test_pos_to_ll(&mproj,  2995., 20000.);
	test_pos_to_ll(&mproj,  2995., 50000.);
	test_pos_to_ll(&mproj,  2995.,200000.);
	test_pos_to_ll(&mproj,     0., -5000.);
	test_pos_to_ll(&mproj,     0.,  7500.);
	test_pos_to_ll(&mproj,     0., 10000.);
	test_pos_to_ll(&mproj,     0., 20000.);
	test_pos_to_ll(&mproj,     0., 50000.);
	test_pos_to_ll(&mproj,     0.,200000.);
	test_pos_to_ll(&mproj,    -5., -5000.);
	test_pos_to_ll(&mproj,    -5.,  7500.);
	test_pos_to_ll(&mproj,    -5., 10000.);
	test_pos_to_ll(&mproj,    -5., 50000.);
	test_pos_to_ll(&mproj,    -5.,200000.);
	test_pos_to_ll(&mproj, -6000.,     0.);
	test_pos_to_ll(&mproj,  3000.,  7125.);
	test_pos_to_ll(&mproj,  3000.,  7625.);
	test_pos_to_ll(&mproj,  3000.,  7900.);
	test_pos_to_ll(&mproj,  3000.,  8400.);

	test_ll_to_pos(&mproj, -40.0,  -95.0);
	test_ll_to_pos(&mproj, -20.0, -120.0);
	test_ll_to_pos(&mproj, -20.0,  -70.0);
	test_ll_to_pos(&mproj, -60.0, -120.0);
	test_ll_to_pos(&mproj, -60.0,  -70.0);
	test_ll_to_pos(&mproj, -80.0,  -95.0);
	test_ll_to_pos(&mproj, -10.0,  -95.0);
	test_ll_to_pos(&mproj,  -5.0,  -95.0);
	test_ll_to_pos(&mproj,  -1.0,  -95.0);
	test_ll_to_pos(&mproj,   0.0,  -95.0);
	test_ll_to_pos(&mproj,   1.0,  -95.0);
	test_ll_to_pos(&mproj,   5.0,  -95.0);
	test_ll_to_pos(&mproj,  10.0,  -95.0);
	test_ll_to_pos(&mproj,  40.0,  -95.0);
	test_ll_to_pos(&mproj,  80.0,  -95.0);

	/* Test scaled_map_projection and evaluation_map_projection */
	/*  for north polar stereographic */
	fprintf(stdout, "\nScaled North Polar Stereographic:\n");
	(void) define_projection(&proj, ProjectPolarSt, 90., 60., 0., 0., 0.);
	map.olat = 26.75;
	map.olon = -90;
	map.lref = -85;
	map.xorg = 0;
	map.yorg = 0;
	map.xlen = 4000;
	map.ylen = 5000;
	map.units = 1000;
	grid.nx      = 9;
	grid.ny      = 11;
	grid.gridlen = 500;
	grid.xgrid   = 500;
	grid.ygrid   = 500;
	grid.units   = 1000;
	(void) define_map_projection(&mproj, &proj, &map, &grid);

	test_scaled_map_projection(&mproj, 0.75);
	test_scaled_map_projection(&mproj, 2.00);

	fprintf(stdout, "\nEvaluation North Polar Stereographic:\n");
	pos[X] = 0.0;		pos[Y] = 0.0;
	test_evaluation_map_projection(&mproj, pos);
	pos[X] = 1500.0;	pos[Y] = 1000.0;
	test_evaluation_map_projection(&mproj, pos);

	/* Test scaled_map_projection and evaluation_map_projection */
	/*  for Mercator Equatorial */
	fprintf(stdout, "\nScaled Mercator Equatorial at Greenwich:\n");
	(void) define_projection(&proj, ProjectMercatorEq, 0., 0., 0., 0., 0.);
	map.olat = 0.0;
	map.olon = 0.0;
	map.lref = 0.0;	/* Should be ignored */
	map.xorg = 2000;
	map.yorg = 2500;
	map.xlen = 4000;
	map.ylen = 5000;
	map.units = 1000;
	grid.nx      = 9;
	grid.ny      = 11;
	grid.gridlen = 250;
	grid.xgrid   = 250;
	grid.ygrid   = 250;
	grid.units   = 1000;
	(void) define_map_projection(&mproj, &proj, &map, &grid);

	test_scaled_map_projection(&mproj, 0.75);
	test_scaled_map_projection(&mproj, 2.00);

	fprintf(stdout, "\nEvaluation Mercator Equatorial at Greenwich:\n");
	pos[X] = 0.0;		pos[Y] = 0.0;
	test_evaluation_map_projection(&mproj, pos);
	pos[X] = 1500.0;	pos[Y] = 1000.0;
	test_evaluation_map_projection(&mproj, pos);

	/* Test scaled_map_projection and evaluation_map_projection */
	/*  for Lat/Long */
	fprintf(stdout, "\nScaled Lat/Long:\n");
	(void) define_projection(&proj, ProjectLatLon, 0., 0., 0., 0., 0.);
	map.olat = 0.0;
	map.olon = 0.0;
	map.lref = 0.0;	/* Should be ignored */
	map.xorg = 0.0;
	map.yorg = 0.0;
	map.xlen = 180.0;
	map.ylen = 90.0;
	map.units = 1.0;
	grid.nx      = 37;
	grid.ny      = 37;
	grid.gridlen = 0.0;
	grid.xgrid   = 5.0;
	grid.ygrid   = 2.5;
	grid.units   = 1.0;
	(void) define_map_projection(&mproj, &proj, &map, &grid);

	test_scaled_map_projection(&mproj, 0.75);
	test_scaled_map_projection(&mproj, 2.00);

	fprintf(stdout, "\nEvaluation Lat/Long:\n");
	pos[X] = 0.0;		pos[Y] = 0.0;
	test_evaluation_map_projection(&mproj, pos);
	pos[X] = 1500.0;	pos[Y] = 1000.0;
	test_evaluation_map_projection(&mproj, pos);
	pos[X] = 50.0;		pos[Y] = 30.0;
	test_evaluation_map_projection(&mproj, pos);

	/* Test great_circle_distance for north polar stereographic */
	fprintf(stdout, "\nGreat Circle Distance for North Polar Stereographic:\n");
	(void) define_projection(&proj, ProjectPolarSt, 90., 60., 0., 0., 0.);
	map.olat = 26.75;
	map.olon = -90;
	map.lref = -85;
	map.xorg = 0;
	map.yorg = 0;
	map.xlen = 4000;
	map.ylen = 5000;
	map.units = 1000;
	grid.nx      = 9;
	grid.ny      = 11;
	grid.gridlen = 500;
	grid.xgrid   = 500;
	grid.ygrid   = 500;
	grid.units   = 1000;
	(void) define_map_projection(&mproj, &proj, &map, &grid);

	spos[X] =    0.0;	spos[Y] =    0.0;
	epos[X] = 1500.0;	epos[Y] = 1000.0;
	test_great_circle_distance(&mproj, spos, epos);
	spos[X] =    0.0;	spos[Y] =    0.0;
	epos[X] = 4000.0;	epos[Y] = 5000.0;
	test_great_circle_distance(&mproj, spos, epos);
	spos[X] =    0.0;	spos[Y] =     0.0;
	epos[X] = 8000.0;	epos[Y] = 10000.0;
	test_great_circle_distance(&mproj, spos, epos);

	/* Test great_circle_distance for for Mercator Equatorial */
	fprintf(stdout, "\nGreat Circle Distance for Mercator Equatorial at Greenwich:\n");
	(void) define_projection(&proj, ProjectMercatorEq, 0., 0., 0., 0., 0.);
	map.olat = 0.0;
	map.olon = 0.0;
	map.lref = 0.0;	/* Should be ignored */
	map.xorg =    0;
	map.yorg =    0;
	map.xlen = 4000;
	map.ylen = 5000;
	map.units = 1000;
	grid.nx      = 17;
	grid.ny      = 21;
	grid.gridlen = 250;
	grid.xgrid   = 250;
	grid.ygrid   = 250;
	grid.units   = 1000;
	(void) define_map_projection(&mproj, &proj, &map, &grid);

	spos[X] =    0.0;	spos[Y] =    0.0;
	epos[X] = 1500.0;	epos[Y] = 1000.0;
	test_great_circle_distance(&mproj, spos, epos);

	/* Test great_circle_distance for for Lat/Long */
	fprintf(stdout, "\nGreat Circle Distance for Lat/Long:\n");
	(void) define_projection(&proj, ProjectLatLon, 0., 0., 0., 0., 0.);
	map.olat = 0.0;
	map.olon = 0.0;
	map.lref = 0.0;	/* Should be ignored */
	map.xorg = 0.0;
	map.yorg = 0.0;
	map.xlen = 180.0;
	map.ylen = 90.0;
	map.units = 1.0;
	grid.nx      = 37;
	grid.ny      = 37;
	grid.gridlen = 0.0;
	grid.xgrid   = 5.0;
	grid.ygrid   = 2.5;
	grid.units   = 1.0;
	(void) define_map_projection(&mproj, &proj, &map, &grid);

	spos[X] =   0.0;	spos[Y] =   0.0;
	epos[X] =  90.0;	epos[Y] =  90.0;
	test_great_circle_distance(&mproj, spos, epos);
	spos[X] =   0.0;	spos[Y] =   0.0;
	epos[X] = 175.0;	epos[Y] =   0.0;
	test_great_circle_distance(&mproj, spos, epos);
	spos[X] =   0.0;	spos[Y] =   0.0;
	epos[X] = 180.0;	epos[Y] =   0.0;
	test_great_circle_distance(&mproj, spos, epos);
	spos[X] =   0.0;	spos[Y] =   0.0;
	epos[X] = 185.0;	epos[Y] =   0.0;
	test_great_circle_distance(&mproj, spos, epos);
	spos[X] =   0.0;	spos[Y] =   0.0;
	epos[X] =   0.0;	epos[Y] = 180.0;
	test_great_circle_distance(&mproj, spos, epos);
	spos[X] =   0.0;	spos[Y] =   0.0;
	epos[X] =   0.0;	epos[Y] = 185.0;
	test_great_circle_distance(&mproj, spos, epos);
	spos[X] =   0.0;	spos[Y] =   0.0;
	epos[X] =   0.0;	epos[Y] = 175.0;
	test_great_circle_distance(&mproj, spos, epos);
	}
#endif /* STANDALONE */
