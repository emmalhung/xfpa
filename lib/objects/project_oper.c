/*********************************************************************/
/** @file project_oper.c
 *
 * Routines to handle objects and structures related to map projections
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*     p r o j e c t _ o p e r . c                                      *
*                                                                      *
*     Routines to handle objects and structures related to map         *
*     projections.                                                     *
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

#include "projection.h"
#include "spline.h"
#include "line.h"

#include <limits.h>
#include <tools/tools.h>
#include <fpa_getmem.h>
#include <fpa_math.h>
#include <fpa_macros.h>


/***********************************************************************
*                                                                      *
*    e v a l u a t i o n _ m a p _ p r o j e c t i o n                 *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Make a copy of a given map projection which includes all given
 * positions and is just large enough to evaluate a surface at all
 * given positions.
 *
 *	@param[in] 	*mp			Input map projection
 *	@param[in] 	npos		Number of positions
 *	@param[in] 	*pos		Positions on input map projection
 *	@param[out]	*mpeval		Evaluation map projection
 *	@param[out]	**poseval	Positions on evaluation map projection
 *  @return True if copy successful.
 *********************************************************************/

LOGICAL	evaluation_map_projection

	(
	const MAP_PROJ	*mp,
	int				npos,
	POINT			*pos,
	MAP_PROJ		*mpeval,
	POINT			**poseval
	)

	{
	PROJ_DEF	*cproj;
	MAP_DEF		*cmap;
	GRID_DEF	*cgrid;
	int			numxy, nn;
	float		minx, maxx, miny, maxy;
	POINT		gpos;
	int			nbx, nex, nby, ney;
	float		gridln, gridlnx, gridlny;

	/* Storage for returned parameters */
	static	int		maxpos = 0;
	static	POINT	*Epos  = NULL;

	/* Error return for missing parameters */
	if (!mp)     return FALSE;
	if (!mpeval) return FALSE;
	if (npos<=0) return FALSE;
	if (!pos)    return FALSE;

	/* First make a copy of the input map projection */
	copy_map_projection(mpeval, mp);
	if (poseval) *poseval = NULL;

	/* Use cproj, cmap and cgrid to point back into the struct */
	cproj = &(mpeval->projection);
	cmap  = &(mpeval->definition);
	cgrid = &(mpeval->grid);

	/* Initialize positions on evalution map projection */
	if (poseval && npos > maxpos)
		{
		maxpos = npos;
		Epos   = GETMEM(Epos, POINT, maxpos);
		}

	/* Set dimensions of evaluation map projection */
	numxy = 2;
	numxy = MAX(numxy, ORDER);

	/* Return a copy of the projection and all positions if no */
	/*  smaller evaluation map projection can be extracted     */
	if ( cgrid->nx <= numxy && cgrid->ny <= numxy )
		{
		if (poseval)
			{
			for (nn=0; nn<npos; nn++)
				copy_point(Epos[nn], pos[nn]);
			*poseval = Epos;
			}
		return TRUE;
		}

	/* Determine maximum and minimum positions from positions on */
	/*  the input map projection                                 */
	minx = miny = FPA_FLT_MAX;
	maxx = maxy = -FPA_FLT_MAX;
	for (nn=0; nn<npos; nn++)
		{
		if ( !pos_to_grid(mp, pos[nn], gpos) ) return FALSE;
		minx  = MIN(minx, gpos[X]);
		maxx  = MAX(maxx, gpos[X]);
		miny  = MIN(miny, gpos[Y]);
		maxy  = MAX(maxy, gpos[Y]);
		}

	/* Determine start and end grid positions for evalution map */
	/*  projection which will contain all positions             */
	if ( cgrid->nx <= numxy )
		{
		nbx  = 0;
		nex  = cgrid->nx - 1;
		}
	else
		{
		nbx  = NINT(ceil((double) minx)) - (numxy/2);
		nbx  = (nbx < 0) ? 0:
				((nbx > (cgrid->nx - numxy)) ? (cgrid->nx - numxy) : nbx);
		nex  = NINT(ceil((double) maxx)) + (numxy/2) - 1;
		nex  = (nex > (cgrid->nx - 1)) ? (cgrid->nx - 1):
				((nex < (nbx + numxy - 1)) ? (nbx + numxy - 1) : nex);
		}
	if ( cgrid->ny <= numxy )
		{
		nby  = 0;
		ney  = cgrid->ny - 1;
		}
	else
		{
		nby  = NINT(ceil((double) miny)) - (numxy/2);
		nby  = (nby < 0) ? 0:
				((nby > (cgrid->ny - numxy)) ? (cgrid->ny - numxy) : nby);
		ney  = NINT(ceil((double) maxy)) + (numxy/2) - 1;
		ney  = (ney > (cgrid->ny - 1)) ? (cgrid->ny - 1):
				((ney < (nby + numxy - 1)) ? (nby + numxy - 1) : ney);
		}

	/* Now reset the parameters of the evaluation map projection */
	if (cgrid->gridlen > 0)
		{
		gridln = cgrid->gridlen * cgrid->units / cmap->units;
		cmap->xorg -= (float) nbx * gridln;
		cmap->yorg -= (float) nby * gridln;
		cmap->xlen  = (float) (nex - nbx) * gridln;
		cmap->ylen  = (float) (ney - nby) * gridln;
		cgrid->nx   = (nex - nbx) + 1;
		cgrid->ny   = (ney - nby) + 1;
		}
	else
		{
		gridlnx = cgrid->xgrid * cgrid->units / cmap->units;
		gridlny = cgrid->ygrid * cgrid->units / cmap->units;
		cmap->xorg -= (float) nbx * gridlnx;
		cmap->yorg -= (float) nby * gridlny;
		cmap->xlen  = (float) (nex - nbx) * gridlnx;
		cmap->ylen  = (float) (ney - nby) * gridlny;
		cgrid->nx   = (nex - nbx) + 1;
		cgrid->ny   = (ney - nby) + 1;
		}

	/* Set new map projection and positions */
	define_map_projection(mpeval, cproj, cmap, cgrid);
	if (poseval)
		{
		for (nn=0; nn<npos; nn++)
			(void) pos_to_pos(mp, pos[nn], mpeval, Epos[nn]);
		*poseval = Epos;
		}
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    p o s _ t o _ e d g e _ g r i d _ p o s                           *
*                                                                      *
***********************************************************************/

/* ScaleFactors = scaling factors for creating limit boxes */
static const float	ScaleFactors[] = {1.00, 0.99, 0.97, 0.94, 0.90,
										0.85, 0.70, 0.50, 0.15, 0.0};
static const int	NScaleFactors  = (int) (sizeof(ScaleFactors)
												/ sizeof(float));

/* LINE_CHECK_ANGLE = comparison angle for checking limit boxes */
#define	LINE_CHECK_ANGLE 45.0


/*********************************************************************/
/** Convert x/y position on a target map projection to the "best"
 * grid position gx/gy on the boundary of a source map projection
 *
 *	@param[in] 	*tmproj		Target map projection
 *	@param[in] 	tpos		Position on target map projection
 *	@param[in] 	*smproj		Source map projection
 *	@param[out]	edgepos		Edge position on source map projection
 *  @return True if successful.
 *********************************************************************/
LOGICAL	pos_to_edge_grid_pos

	(
	const MAP_PROJ	*tmproj,
	POINT			tpos,
	const MAP_PROJ	*smproj,
	POINT			edgepos
	)

	{
	int		nn, nscal, nx, ny;
	float	scale, xgrid, ygrid, xpos, ypos;
	POINT	xp, tp, tpedge;
	LOGICAL	inside;
	LOGICAL	wrap, wrap_x;
	int		wrap_i;

	/* Storage for returned parameters */
	static	MAP_PROJ	Smp  = NO_MAPPROJ;
	static	MAP_PROJ	Tmp  = NO_MAPPROJ;
	static	MAP_PROJ	Xmp  = NO_MAPPROJ;
	static	LINE		Xlim = NullLine;
	static	LINE		Xbox = NullLine;
	static	LOGICAL		OkX  = FALSE;

	/* Error return for missing parameters */
	if (!tmproj)  return FALSE;
	if (!smproj)  return FALSE;
	if (!tpos)    return FALSE;
	if (!edgepos) return FALSE;

	/* Check source and target map projections */
	if (same_map_projection(smproj, &Smp)
		&& same_map_projection(tmproj, &Tmp))
		{

		/* Return now if failed to produce limit curve and corner */
		/*  box for this source and target map projections        */
		if (!OkX) return FALSE;
		}

	/* Construct limit curve and corner box only once */
	/*  for given source and target map projections   */
	else
		{
		/* Save new map projections */
		copy_map_projection(&Smp, smproj);
		copy_map_projection(&Tmp, tmproj);

		/* Construct limit curve and corner box from the source map    */
		/* projection (scaled as necessary) in target map co-ordinates */
		for (nscal=0; nscal<NScaleFactors; nscal++)
			{
			/* Scale the source map projection */
			scale = ScaleFactors[nscal];
			if ( !scaled_map_projection(&Xmp, &Smp, scale) )
				{
				Xlim = destroy_line(Xlim);
				Xbox = destroy_line(Xbox);
				return FALSE;
				}

			/* Create the limit curve and corner box */
			Xlim = destroy_line(Xlim);
			Xbox = destroy_line(Xbox);
			Xlim = create_line();
			Xbox = create_line();

			/* Set grid spacing from scaled projection */
			if (Xmp.grid.gridlen > 0)
				{
				xgrid = Xmp.grid.gridlen;
				ygrid = Xmp.grid.gridlen;
				}
			else
				{
				xgrid = Xmp.grid.xgrid;
				ygrid = Xmp.grid.ygrid;
				}

			/* Check if projection wraps around globe */
			nx = Xmp.grid.nx;
			ny = Xmp.grid.ny;
			wrap = wrap_map_projection( smproj, &wrap_x, &wrap_i);
			if ( wrap && wrap_i < 0)
				{
				if (wrap && wrap_x )       nx -= wrap_i;
				else if (wrap && !wrap_x ) ny -= wrap_i;
				}

			/* Determine limit curve and corner box by row and column */
			xpos = 0.0;
			ypos = 0.0;
			for (nn = 0; nn < nx; nn++)
				{
				xpos = (float) nn * xgrid;
				set_point(xp, xpos, ypos);
				(void) pos_to_pos(&Xmp, xp, tmproj, tp);
				/* Add point to corner box */
				if ( nn == 0 ) add_point_to_line(Xbox, tp);
				/* Add point to limit curve */
				add_point_to_line(Xlim, tp);
				}
			for (nn = 0; nn < ny; nn++)
				{
				ypos = (float) nn * ygrid;
				set_point(xp, xpos, ypos);
				(void) pos_to_pos(&Xmp, xp, tmproj, tp);
				/* Add point to corner box */
				if ( nn == 0 ) add_point_to_line(Xbox, tp);
				/* Add point to limit curve */
				add_point_to_line(Xlim, tp);
				}
			for (nn = (nx - 1); nn >= 0; nn--)
				{
				xpos = (float) nn * xgrid;
				set_point(xp, xpos, ypos);
				(void) pos_to_pos(&Xmp, xp, tmproj, tp);
				/* Add point to corner box */
				if ( nn == (nx - 1) ) add_point_to_line(Xbox, tp);
				/* Add point to limit curve */
				add_point_to_line(Xlim, tp);
				}
			for (nn = (ny - 1); nn >= 0; nn--)
				{
				ypos = (float) nn * ygrid;
				set_point(xp, xpos, ypos);
				(void) pos_to_pos(&Xmp, xp, tmproj, tp);
				/* Add point to corner box */
				if ( nn == (ny - 1) ) add_point_to_line(Xbox, tp);
				/* Add point to limit curve */
				add_point_to_line(Xlim, tp);
				}
			/* Add last point to corner box */
			add_point_to_line(Xbox, tp);

			(void) pr_diag("Limits", "Limit corner box:\n");
			for (nn=0; nn<Xbox->numpts; nn++)
				(void) pr_diag("Limits", "   (%g, %g)\n",
						Xbox->points[nn][X], Xbox->points[nn][Y]);
			(void) pr_diag("Limits", "Limit curve:\n");
			for (nn=0; nn<Xlim->numpts; nn++)
				(void) pr_diag("Limits", "   (%g, %g)\n",
						Xlim->points[nn][X], Xlim->points[nn][Y]);

			/* Stop when limit curve is acceptable */
			xpos = (float) (nx - 1) / 2.0 * xgrid;
			ypos = (float) (ny - 1) / 2.0 * ygrid;
			set_point(xp, xpos, ypos);
			(void) pos_to_pos(&Xmp, xp, tmproj, tp);
			if ( contiguous_line(Xlim, tp, LINE_CHECK_ANGLE) )
				{
				OkX = TRUE;
				break;
				}
			}

		(void) pr_diag("Limits", "       Scale factor used: %f\n", scale);
		}

	/* Determine the location of the target position from the limit */
	/* curve of the scaled source map projection                    */
	line_test_point(Xlim, tpos, NullFloat, NullPoint, NullInt, &inside,
					NullChar);

	/* Convert directly to grid position on the scaled source map  */
	/* projection if the target position is inside the limit curve */
	if (inside)
		{
		(void) pos_to_pos(tmproj, tpos, &Xmp, xp);
		(void) pos_to_grid(&Xmp, xp, edgepos);
		return TRUE;
		}

	/* Find the closest point on the corner box, extend to the limit  */
	/* curve of the scaled source map projection, and then convert to */
	/* grid position if the target point is outside the limit curve   */
	else
		{
		line_test_point(Xbox, tpos, NullFloat, tp, NullInt, NullChar, NullChar);
		line_sight(Xlim, tpos, tp, TRUE, NullFloat, NullFloat, tpedge, NullInt,
					NullChar);
		(void) pos_to_pos(tmproj, tpedge, &Xmp, xp);
		(void) pos_to_grid(&Xmp, xp, edgepos);
		return TRUE;
		}
	}

/***********************************************************************
*                                                                      *
*    s c a l e d _ m a p _ p r o j e c t i o n                         *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Make a scaled copy of a given map projection.
 *
 * The scaled projection will have the same reference origin, but
 * will be shifted by the scale, which is given as a fraction.
 *
 * A scale of 0.75 will result in a map projection three quarters
 * as large.
 * A scale of 2.00 will result in a map projection twice as large.
 *
 *	@param[out]	*mp1		Scaled map projection
 *	@param[in] 	*mp2		Input map projection
 *	@param[in] 	scale		Fractional scale factor
 *	@return True if successful
 *********************************************************************/

LOGICAL	scaled_map_projection

	(
	MAP_PROJ		*mp1,
	const MAP_PROJ	*mp2,
	float			scale
	)

	{
	float		fact, xdiff, ydiff;
	PROJ_DEF	*cproj;
	MAP_DEF		*cmap;
	GRID_DEF	*cgrid;

	/* Error return for missing parameters */
	if (!mp1) return FALSE;
	if (!mp2) return FALSE;

	/* Map projection cannot be less than 5% of input size */
	if (scale <= 0.05 ) return FALSE;

	/* First make a copy */
	copy_map_projection(mp1, mp2);

	/* Use cproj, cmap and cgrid to point back into the struct */
	cproj = &(mp1->projection);
	cmap  = &(mp1->definition);
	cgrid = &(mp1->grid);

	/* Then reset the origin and grid lengths based on the scale */
	fact = scale - 1.0;
	if (cgrid->gridlen > 0)
		{
		xdiff = (cgrid->nx - 1) * cgrid->gridlen * fact / 2.0;
		ydiff = (cgrid->ny - 1) * cgrid->gridlen * fact / 2.0;
		cmap->xorg     += xdiff;
		cmap->yorg     += ydiff;
		cmap->xlen     *= scale;
		cmap->ylen     *= scale;
		cgrid->gridlen *= scale;
		cgrid->xgrid   *= scale;
		cgrid->ygrid   *= scale;
		}
	else
		{
		xdiff = (cgrid->nx - 1) * cgrid->xgrid * fact / 2.0;
		ydiff = (cgrid->ny - 1) * cgrid->ygrid * fact / 2.0;
		cmap->xorg   += xdiff;
		cmap->yorg   += ydiff;
		cmap->xlen   *= scale;
		cmap->ylen   *= scale;
		cgrid->xgrid *= scale;
		cgrid->ygrid *= scale;
		}

	/* Set new map projection */
	define_map_projection(mp1, cproj, cmap, cgrid);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    c l o s e s t _ m a p _ p r o j e c t i o n                       *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find the "closest" map projection from the given list, to the
 * given lat-lon position.
 *
 * This is determined as the smallest map projection which contains
 * the given point.
 *
 * If none of the map projections in the list contain the point,
 * then the map projection with the closest boundary is used.
 *
 * This function can be forced to consider only enclosing map
 * projections by setting enclose_only = TRUE.
 *
 * The calling software can determine whether the selected map
 * projection actually contains the point by examining the value
 * of inside:
 *
 *	@param[in] 	lat				Latitude
 *	@param[in] 	lon				Longitude
 *	@param[in] 	num				Number of map projections
 *	@param[in] 	*mplist			List of map projections
 *	@param[out]	*inside			Is lat/lon inside a map projection?
 *	@param[in] 	enclose_only	Return only if inside?
 *  @return TRUE if point is inside selected map projection, or
 *			FALSE if point is outside selected map projection
 *
 *********************************************************************/

int		closest_map_projection

	(
	float			lat,
	float			lon,
	int				num,
	const MAP_PROJ	*mplist,
	LOGICAL			*inside,
	LOGICAL			enclose_only
	)

	{
	int				i, bindx;
	const MAP_PROJ	*mp;
	POINT			pos, cpos;
	float			xlen, ylen, xmin, ymin;
	float			dist, bdist, area, barea;
	LOGICAL			xin, yin;
	LOGICAL			bydist, byarea;
	LOGICAL			wrap, wrap_x;
	int     		wrap_i;

	/* Set default return parameters */
	if (inside) *inside = FALSE;

	/* Error return for missing parameters */
	if (num <= 0) return -1;
	if (!mplist)  return -1;

	bydist = FALSE;
	byarea = FALSE;
	bdist  = 0;
	barea  = 0;
	bindx  = -1;
	for (i=0; i<num; i++)
		{
		mp = mplist + i;
		if (!mp) continue;

		/* Transform lat-lon to a point in this projection */
		/* and scale all dimensions to meters */
		(void) ll_to_pos(mp, lat, lon, pos);
		pos[X] *= mp->definition.units;
		pos[Y] *= mp->definition.units;
		xlen    = mp->definition.xlen * mp->definition.units;
		ylen    = mp->definition.ylen * mp->definition.units;
		xmin    = 0;
		ymin    = 0;

		/* Check if projection wraps around globe */
		wrap = wrap_map_projection( mp, &wrap_x, &wrap_i);
		if ( wrap && wrap_x && wrap_i < 0 )
			{
			xlen -= wrap_i;
			xmin += wrap_i;
			}
		else if ( wrap && !wrap_x && wrap_i < 0 )
			{
			ylen -= wrap_i;
			ymin += wrap_i;
			}
	
		/* Determine if inside or outside and calculate */
		/* closest point on boundary if outside */
		if (pos[X] > xlen)      { xin = FALSE; cpos[X] = xlen; }
		else if (pos[X] < xmin) { xin = FALSE; cpos[X] = xmin; }
		else                    { xin = TRUE;  cpos[X] = pos[X]; }
		if (pos[Y] > ylen)      { yin = FALSE; cpos[Y] = ylen; }
		else if (pos[Y] < ymin) { yin = FALSE; cpos[Y] = ymin; }
		else                    { yin = TRUE;  cpos[Y] = pos[Y]; }

		/* If inside judge by the smallest size */
		if (xin && yin)
			{
			area = xlen*ylen;
			if (!byarea || area<barea)
				{
				barea = area;
				bindx = i;
				}
			byarea = TRUE;
			}

		/* If outside, judge by smallest distance */
		/* (unless we already had one inside) */
		else if (!byarea && !enclose_only)
			{
			/* Calculate and compare distance */
			dist = hypot(cpos[X]-pos[X], cpos[Y]-pos[Y]);
			if (!bydist || dist<bdist)
				{
				bdist = dist;
				bindx = i;
				}
			bydist = TRUE;
			}
		}

	if (inside) *inside = (LOGICAL) (bindx>=0 && byarea);
	return bindx;
	}

/***********************************************************************
*                                                                      *
*     g r i d _ p o s i t i o n s                                      *
*     g r i d _ c e n t e r                                            *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Determine x/y and lat/lon position values for all locations from
 * the given map projection.
 *
 *	@param[in] 	*mproj		map projection
 *	@param[out]	*numx		number of points in x direction
 *	@param[out]	*numy		number of points in y direction
 *	@param[out]	*glen		grid length (in km)
 *	@param[out]	***gpos		position for each gridpoint
 *	@param[out]	***glat		latitude value for each gridpoint
 *	@param[out]	***glon		longitude value for each gridpoint
 *  @return True if successful.
 *********************************************************************/

LOGICAL	grid_positions

	(
	const MAP_PROJ	*mproj,
	int				*numx,
	int				*numy,
	float			*glen,
	POINT			***gpos,
	float			***glat,
	float			***glon
	)

	{
	/* Storage for returned parameters */
	static	MAP_PROJ	Improj  = NO_MAPPROJ;
	static	int			Inumx   = 0;
	static	int			Inumy   = 0;
	static	float		Aglen   = 0.0;
	static	POINT		**Apos  = NULL, *Ppos  = NULL;
	static	float		**Alats = NULL, *Plats = NULL;
	static	float		**Alons = NULL, *Plons = NULL;

	static	int			xymax   = 0;
	static	int			ymax    = 0;

	int		valid;
	int		iix, iiy;

	/* Free the current arrays if NULL map projection has been given */
	if (!mproj)
		{
		copy_map_projection(&Improj, &NoMapProj);
		Inumx = 0;
		Inumy = 0;
		Aglen = 0.0;
		FREEMEM(Ppos);
		FREEMEM(Plats);
		FREEMEM(Plons);
		FREEMEM(Apos);
		FREEMEM(Alats);
		FREEMEM(Alons);

		xymax = 0;
		ymax  = 0;

		/* Return code indicates failure */
		valid = FALSE;
		}

	/* Return the current arrays if basemap and grid definitions */
	/*  have not changed                                         */
	else if (same_map_projection(&Improj, mproj))
		{
		/* Return code indicates success */
		valid = TRUE;
		}

	/* Otherwise compute new arrays */
	else
		{
		/* Save the basemap and grid definitions */
		copy_map_projection(&Improj, mproj);
		Inumx = Improj.grid.nx;
		Inumy = Improj.grid.ny;
		Aglen = Improj.grid.gridlen;

		/* Allocate space to hold the locations */
		if ( Inumx * Inumy > xymax )
			{
			xymax = Inumx * Inumy;
			Ppos  = GETMEM(Ppos,  POINT, xymax);
			Plats = GETMEM(Plats, float, xymax);
			Plons = GETMEM(Plons, float, xymax);
			}
		if ( Inumy > ymax )
			{
			ymax = Inumy;
			Apos  = GETMEM(Apos,  POINT *, ymax);
			Alats = GETMEM(Alats, float *, ymax);
			Alons = GETMEM(Alons, float *, ymax);
			}

		/* Compute x/y and latitude/longitude position values */
		for (iiy=0; iiy<Inumy; iiy++)
			{
			/* Set up row pointers in doubly dimensioned arrays */
			Apos[iiy]  = Ppos  + iiy*Inumx;
			Alats[iiy] = Plats + iiy*Inumx;
			Alons[iiy] = Plons + iiy*Inumx;

			/* Compute values for each row */
			for (iix=0; iix<Inumx; iix++)
				{
				Apos[iiy][iix][Y] = iiy*Aglen;
				Apos[iiy][iix][X] = iix*Aglen;
				(void) pos_to_ll(&Improj, Apos[iiy][iix],
						&Alats[iiy][iix], &Alons[iiy][iix]);
				}
			}

		/* Return code indicates success */
		valid = TRUE;
		}

	/* Return pointers to locations */
	if (numx) *numx = Inumx;
	if (numy) *numy = Inumy;
	if (glen) *glen  = Aglen;
	if (gpos) *gpos  = Apos;
	if (glat) *glat  = Alats;
	if (glon) *glon  = Alons;
	return valid;
	}

/*********************************************************************/
/** Determine x/y and lat/lon position values for the center of the
 * given map projection.
 *
 *	@param[in] 	*mproj		map projection
 *	@param[out]	*cpos		position for center gridpoint
 *	@param[out]	*clat		latitude value for center gridpoint
 *	@param[out]	*clon		longitude value for center gridpoint
 *  @return True if successful.
 *********************************************************************/
LOGICAL	grid_center

	(
	const MAP_PROJ	*mproj,
	POINT			*cpos,
	float			*clat,
	float			*clon
	)

	{
	/* Storage for returned parameters */
	static	MAP_PROJ	Cmproj = NO_MAPPROJ;
	static	POINT		Cpos   = ZERO_POINT;
	static	float		Clat   = 0.0;
	static	float		Clon   = 0.0;

	int		valid;

	/* Zero the current values if NULL map projection has been given */
	if (!mproj)
		{
		copy_map_projection(&Cmproj, &NoMapProj);
		copy_point(Cpos, ZeroPoint);
		Clat = 0.0;
		Clon = 0.0;

		/* Return code indicates failure */
		valid = FALSE;
		}

	/* Return the current values if basemap and grid definitions */
	/*  have not changed                                         */
	else if (same_map_projection(&Cmproj, mproj))
		{
		/* Return code indicates success */
		valid = TRUE;
		}

	/* Otherwise compute new arrays */
	else
		{
		/* Save the basemap and grid definitions */
		copy_map_projection(&Cmproj, mproj);

		/* Determine x/y location of center position */
		if (Cmproj.grid.gridlen > 0)
			{
			Cpos[X] = (float) (Cmproj.grid.nx - 1)
								* Cmproj.grid.gridlen / 2.0;
			Cpos[Y] = (float) (Cmproj.grid.ny - 1)
								* Cmproj.grid.gridlen  / 2.0;
			}
		else
			{
			Cpos[X] = (float) (Cmproj.grid.nx - 1)
								* Cmproj.grid.xgrid  / 2.0;
			Cpos[Y] = (float) (Cmproj.grid.ny - 1)
								* Cmproj.grid.ygrid  / 2.0;
			}

		/* Determine lat/long location of center position */
		(void) pos_to_ll(&Cmproj, Cpos, &Clat, &Clon);

		/* Return code indicates success */
		valid = TRUE;
		}

	/* Return pointers to locations */
	if (cpos) copy_point(*cpos, Cpos);
	if (clat) *clat = Clat;
	if (clon) *clon = Clon;
	return valid;
	}

/***********************************************************************
*                                                                      *
*    g r i d _ c o m p o n e n t _ c o e f f i c i e n t s             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Return coefficients to convert a vector component on an input
 * map projection to a vector component on an output map projection.
 *
 * Note that we only recognize x and y components presently.
 *
 * Note that projections and coefficients are stored for each of
 * 2*2 input/output component combinations so as to be able to reuse
 * coefficients if the projections do not change, but without having
 * to calculate all component combinations if projections do change.
 *
 *	@param[in] 	compflag	component flag for coordinate system
 *							0 = East/North coordinate system
 *							1 = x/y coord system of map projection
 *	@param[in] 	*mprojin	map projection of input component
 *	@param[in] 	compin		input component
 *	@param[in] 	*mprojout	map projection of output component
 *	@param[in] 	compout		output component
 *	@param[out]	***coefs	conversion coefficients at each gridpoint
 *							of output component map projection
 *  @return True if successful.
 *********************************************************************/

LOGICAL	grid_component_coefficients

	(
	int				compflag,
	const MAP_PROJ	*mprojin,
	COMPONENT		compin,
	const MAP_PROJ	*mprojout,
	COMPONENT		compout,
	float			***coefs
	)

	{
	int			in, out;
	int			Inumx, Inumy;
	float		**Alats, **Alons;
	int			iix, iiy;
	float		trueang, xyang;

	/* Storage for input/output map projections and conversion coefficients */
	static	MAP_PROJ	IMproj[2][2] = { { NO_MAPPROJ, NO_MAPPROJ },
											{ NO_MAPPROJ, NO_MAPPROJ } };
	static	MAP_PROJ	OMproj[2][2] = { { NO_MAPPROJ, NO_MAPPROJ },
											{ NO_MAPPROJ, NO_MAPPROJ } };
	static	float		*(PCoefs[2][2])  = { { NULL, NULL }, { NULL, NULL } };
	static	float		**(ACoefs[2][2]) = { { NULL, NULL }, { NULL, NULL } };

	/* Initialize return parameters */
	if ( coefs ) *coefs = NULL;

	/* Return FALSE for unrecognized component flag */
	if ( compflag < 0 || compflag > 1 ) return FALSE;

	/* Set location in arrays from input and output components */
	switch ( compin )
		{
		case X_Comp:
			in = 0;
			break;
		case Y_Comp:
			in = 1;
			break;
		default:
			return FALSE;
		}
	switch ( compout )
		{
		case X_Comp:
			out = 0;
			break;
		case Y_Comp:
			out = 1;
			break;
		default:
			return FALSE;
		}

	/* Set grid locations from ouput component map projection */
	if ( !grid_positions(mprojout, &Inumx, &Inumy, NULL,
			NULL, &Alats, &Alons) ) return FALSE;

	/* Compute new coefficients if map projections have changed */
	if ( !same_map_projection(&IMproj[in][out], mprojin)
			|| !same_map_projection(&OMproj[in][out], mprojout) )
		{

		/* Save the map projections for comparision */
		copy_map_projection(&IMproj[in][out], mprojin);
		copy_map_projection(&OMproj[in][out], mprojout);

		/* Allocate space to hold the new coefficients */
		FREEMEM(PCoefs[in][out]);
		PCoefs[in][out] = INITMEM(float,   Inumx*Inumy);
		FREEMEM(ACoefs[in][out]);
		ACoefs[in][out] = INITMEM(float *, Inumy);

		/* Compute new coefficients based on input and output components */
		for ( iiy=0; iiy<Inumy; iiy++ )
			{

			/* Set up row pointers in quadruply dimensioned arrays */
			ACoefs[in][out][iiy] = PCoefs[in][out] + iiy*Inumx;

			/* Compute coefficients for each row */
			for ( iix=0; iix<Inumx; iix++ )
				{

				/* Convert input component direction to degrees true */
				switch ( compin )
					{

					/*  ... input x component (from the West) */
					case X_Comp:
						if      ( compflag == 0 )
							trueang = 270.0;
						else if ( compflag == 1 )
							trueang = wind_dir_true(mprojin,
									Alats[iiy][iix], Alons[iiy][iix], 180.0);
						break;

					/*  ... input y component (from the South) */
					case Y_Comp:
						if      ( compflag == 0 )
							trueang = 180.0;
						else if ( compflag == 1 )
							trueang = wind_dir_true(mprojin,
									Alats[iiy][iix], Alons[iiy][iix], 270.0);
						break;
					}

				/* Determine angle of rotation to output map projection */
				xyang = wind_dir_xy(mprojout,
							Alats[iiy][iix], Alons[iiy][iix], trueang) - 180.0;

				/* Determine conversion coefficient by output component */
				switch ( compout )
					{

					/*  ... output x component (from -x direction) */
					case X_Comp:
						ACoefs[in][out][iiy][iix] = cos(RAD * xyang);
						break;

					/*  ... output y component (from -y direction) */
					case Y_Comp:
						ACoefs[in][out][iiy][iix] = sin(RAD * xyang);
						break;
					}
				}
			}
		}

	/* Return pointer to coefficients for input and output components */
	if ( coefs ) *coefs = ACoefs[in][out];
	return TRUE;
	}
