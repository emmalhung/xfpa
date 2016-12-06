/**********************************************************************/
/** @file target_map.c
 *
 * Routines to interpret the "target_map" setup block
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   t a r g e t _ m a p . c                                            *
*                                                                      *
*   Routines to interpret the "target_map" setup block                 *
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

#define TARGET_MAP_INIT		/* To initialize declarations in target_map.h */

#include "read_setup.h"
#include "target_map.h"

#include <objects/objects.h>
#include <tools/tools.h>
#include <fpa_types.h>
#include <fpa_getmem.h>

#include <stdio.h>
#include <string.h>

/* Internal static function to read "target_map" setup block */
static	int			read_target_map_setup(void);

/* Global variables to hold target map information */
static	int			TargetBlockReady = FALSE;		/* Has it been read yet */
static	MAP_PROJ	TargetMap        = NO_MAPPROJ;	/* Target map projection */

/**********************************************************************
*                                                                     *
*   g e t _ t a r g e t _ m a p                                       *
*                                                                     *
***********************************************************************/

/**********************************************************************/
/** Retrieve the target map information from the setup file.
 *
 * @return A MAP_PROJ structure populated with data from the setup file.
 **********************************************************************/
MAP_PROJ		*get_target_map

	(
	)

	{
	if (!read_target_map_setup())        return NullMapProj;
	if (TargetMap.definition.units <= 0) return NullMapProj;
	if (TargetMap.grid.units <= 0)       return NullMapProj;

	return &TargetMap;
	}

/**********************************************************************
*                                                                     *
*   r e a d _ t a r g e t _ m a p _ s e t u p                         *
*                                                                     *
**********************************************************************/

static	int		read_target_map_setup

	(
	)

	{
	LOGICAL		argOK;
	int			nx, ny;
	float		olat, olon, lref, xmin, ymin, xmax, ymax, units;
	float		gridlen=0, xlen=0, ylen=0;
	STRING		line, key;
	PROJ_DEF	proj;
	MAP_DEF		mdef;
	GRID_DEF	grid;
	char		ptype[40], p1[40], p2[40], p3[40], p4[40], p5[40], lval[40];

	if (TargetBlockReady)                        return TRUE;
	if ( !find_setup_block("target_map", TRUE) ) return FALSE;

	/* Set up default projection info */
	(void) copy_projection(&proj, &NoProjDef);
	(void) copy_map_def(&mdef, &NoMapDef);
	(void) copy_grid_def(&grid, &NoGridDef);

	/* Store the ingest block */
	while ( line = setup_block_line() )
		{
		/* Read the keyword */
		key = string_arg(line);

		if (same(key, "projection"))
			{
			/* Read the projection type */
			(void) strcpy_arg(ptype, line, &argOK);	if (!argOK) continue;

			/* Read the projection parameters */
			(void) strcpy_arg(p1, line, &argOK);	if (!argOK)
														(void) strcpy(p1, "");
			(void) strcpy_arg(p2, line, &argOK);	if (!argOK)
														(void) strcpy(p2, "");
			(void) strcpy_arg(p3, line, &argOK);	if (!argOK)
														(void) strcpy(p3, "");
			(void) strcpy_arg(p4, line, &argOK);	if (!argOK)
														(void) strcpy(p4, "");
			(void) strcpy_arg(p5, line, &argOK);	if (!argOK)
														(void) strcpy(p5, "");

			/* Build the projection */
			(void) define_projection_by_name(&proj, ptype, p1, p2, p3, p4, p5);
			}

		else if (same(key, "mapdef"))
			{
			/* Read the origin info */
			(void) strcpy_arg(lval, line, &argOK);	if (!argOK) continue;
			olat = read_lat(lval, &argOK);			if (!argOK) continue;
			(void) strcpy_arg(lval, line, &argOK);	if (!argOK) continue;
			olon = read_lon(lval, &argOK);			if (!argOK) continue;
			(void) strcpy_arg(lval, line, &argOK);	if (!argOK) continue;
			lref = read_lon(lval, &argOK);			if (!argOK) continue;

			/* Read the map dimensions */
			xmin  = float_arg(line, &argOK);	if (!argOK) continue;
			ymin  = float_arg(line, &argOK);	if (!argOK) continue;
			xmax  = float_arg(line, &argOK);	if (!argOK) continue;
			ymax  = float_arg(line, &argOK);	if (!argOK) continue;
			units = float_arg(line, &argOK);	if (!argOK) continue;

			/* Build the map definition */
			if (olon <= -180) olon +=360;
			if (lref <= -180) lref +=360;
			mdef.olat  = olat;
			mdef.olon  = olon;
			mdef.lref  = lref;
			mdef.xorg  = -xmin;
			mdef.yorg  = -ymin;
			mdef.xlen  = xmax - xmin;
			mdef.ylen  = ymax - ymin;
			mdef.units = units;

			/* Remember dimensions for grid calculation */
			xlen = fabs(xmax-xmin);
			ylen = fabs(ymax-ymin);
			}

		else if (same(key, "resolution"))
			{
			/* Read the grid dimension */
			gridlen = float_arg(line, &argOK);	if (!argOK) continue;
			units   = float_arg(line, &argOK);	if (!argOK) continue;

			/* Build the grid definition */
			grid.nx      = 0;
			grid.ny      = 0;
			grid.gridlen = gridlen;
			grid.xgrid   = gridlen;
			grid.ygrid   = gridlen;
			grid.units   = units;
			}
		}

	/* Make sure we got something */
	TargetBlockReady = TRUE;
	if (gridlen<=0 || xlen<=0 || ylen<=0)
		{
		pr_error("Environ", "Problem reading target_map setup block");
		return FALSE;
		}

	/* Build the map projection */
	(void) set_grid_def_units(&grid, mdef.units);
	nx = 1 + (int) (xlen/grid.gridlen + .75);
	ny = 1 + (int) (ylen/grid.gridlen + .75);
	grid.nx = nx;
	grid.ny = ny;
	(void) define_map_projection(&TargetMap, &proj, &mdef, &grid);

	return TRUE;
	}
