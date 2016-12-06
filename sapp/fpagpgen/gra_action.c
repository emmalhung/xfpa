/***********************************************************************
*                                                                      *
*     g r a _ a c t i o n . c                                          *
*                                                                      *
*     Routines to define and display various FPA field types           *
*                                                                      *
*     Version 5 (c) Copyright 1997 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2003 Environment Canada (MSC)            *
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

#include "clipper.h"
#include "fpagpgen_routines.h"
#include "fpagpgen_structs.h"

/* We need FPA library definitions */
#include <fpa.h>

/* We need C standard library definitions */
#include <math.h>
#include <time.h>

/* >>>>> debug testing <<<<< */
static	LOGICAL	DebugMode = FALSE;
/* >>>>> debug testing <<<<< */

/* Internal static functions */
static  STRING		parse_ctable_name(STRING);
static  void		reverse_image_list(Image *, int);
static	STRING		check_sample_cases(POINT, STRING, STRING, SPCASE *, int);
static	float		set_vertical_position(FLD_DESCRIPT *, XSECT_VER_AXIS *,
											POINT, STRING, STRING, STRING,
											STRING, STRING, STRING,
											CATATTRIB *, int, STRING, STRING,
											STRING, float, LOGICAL *);
static	float		set_vertical_position_data_file(XSECT_VER_AXIS *,
											STRING, STRING, STRING,
											STRING, float, float, STRING,
											STRING, STRING, LOGICAL *);
static	LINE		xsection_line_coordinates(LINE, GRA_XSECT *,
											XSECT_HOR_AXIS *, XSECT_VER_AXIS *);
static	BOUND		xsection_boundary_coordinates(BOUND, GRA_XSECT *,
											XSECT_HOR_AXIS *, XSECT_VER_AXIS *);
static	LOGICAL		GRA_replicate_pattern(LINE, STRING, ITEM, float, float,
											int *, LINE **);
static	LOGICAL		GRA_display_draw_line(LINE, STRING, float, float,
											LOGICAL, STRING, STRING, STRING,
											COMP_PRES *, int);
static	LOGICAL		GRA_display_draw_outline(LINE, float, float, LOGICAL,
											STRING, STRING, STRING, STRING,
											STRING, COMP_PRES *, int);
static	LOGICAL		GRA_display_simple_line(LINE, float, float, PRES,
											LOGICAL);
static	LOGICAL		GRA_display_simple_outline(LINE, float, float, PRES,
											LOGICAL);
static	LOGICAL		GRA_display_simple_boundary(BOUND, float, float, PRES,
											LOGICAL);
static	LOGICAL		GRA_display_patterned_line(LINE, float, float, PRES,
											LOGICAL, HAND);
static	LOGICAL		GRA_display_patterned_outline(LINE, float, float, PRES,
											LOGICAL, HAND);
static	LOGICAL		GRA_display_patterned_boundary(BOUND, float, float, PRES,
											LOGICAL, HAND, LOGICAL);
static	LOGICAL		GRA_display_contour_line(SURFACE, float, STRING,
											STRING, STRING, STRING,
											COMP_PRES *, int, LOGICAL);
static	LOGICAL		GRA_display_contour_area(SURFACE, float, float, STRING,
											STRING, STRING, STRING, STRING,
											STRING, COMP_PRES *, int, LOGICAL);
static	LOGICAL		GRA_display_xsection_contour_line(SURFACE, GRA_XSECT *,
											XSECT_HOR_AXIS *, XSECT_VER_AXIS *,
											float, STRING,
											STRING, STRING, STRING,
											COMP_PRES *, int, LOGICAL);
static	LOGICAL		GRA_display_xsection_contour_area(SURFACE, GRA_XSECT *,
											XSECT_HOR_AXIS *, XSECT_VER_AXIS *,
											float, float, STRING, STRING,
											STRING, STRING, STRING, STRING,
											COMP_PRES *, int, LOGICAL);
static	LOGICAL		GRA_display_box_symbol_fill(float, float, float, float,
											float, STRING);
static	LOGICAL		GRA_display_ellipse_symbol_fill(float, float, float, float,
											float, float, float, STRING);
static	LOGICAL		GRA_display_outline_symbol_fill(LINE, STRING);
static	LOGICAL		GRA_display_boundary_symbol_fill(BOUND, STRING);
static	LOGICAL		GRA_display_attribute(CAL, ATTRIB_DISPLAY, float, float,
											float, float, float, STRING);
static	LOGICAL		GRA_display_attribute_outline(CAL, ATTRIB_DISPLAY,
											float, float, float, float, float);
static	LOGICAL		GRA_display_sampled_attributes(STRING, STRING, STRING,
											STRING, STRING,
											POINT, float, float, float, float,
											float, CAL, STRING, CATATTRIB *, int,
											ATTRIB_DISPLAY *, int,
											SPCASE *, int, STRING, float,
											STRING, STRING, LOGICAL, STRING,
											float, LOGICAL, LOGICAL, STRING,
											LOGICAL, float, float,
											float, float);
static	LOGICAL		GRA_display_sampled_value(STRING, STRING, STRING,
											STRING, STRING,
											POINT, float, ATTRIB_DISPLAY *, int,
											SPCASE *, int, STRING, float,
											STRING, STRING, LOGICAL, STRING,
											float, float, float, float, float);
static	LOGICAL		GRA_display_sampled_vector(STRING, STRING, STRING,
											STRING, STRING, POINT, float, float,
											ATTRIB_DISPLAY *, int,
											SPCASE *, int, STRING, float,
											STRING, STRING, LOGICAL, STRING,
											float, float, float, float, float);
static	LOGICAL		GRA_display_arrow(STRING, LINE, float, float, PRES,
											LOGICAL);
static	void		GRA_display_arrow_head(ARROW_DISPLAY, LINE, float, float,
											LOGICAL);
static	void		GRA_display_arrow_tail(ARROW_DISPLAY, LINE, float, float,
											LOGICAL);
static	void		GRA_display_arrow_revtail(ARROW_DISPLAY, LINE, float, float,
											LOGICAL);
static	LOGICAL		GRA_display_windbarb(float, float, float, float, float,
											float, float, float, float, float);
static	LOGICAL		GRA_display_wind(STRING, float, float, float, float, float,
											float, float, float, float, float);
static	LOGICAL		GRA_display_vector(STRING, float, float, float, float,
											float, float, float, float, float);

/***********************************************************************
*                                                                      *
*    G R A _ d i s p l a y _ a d d                                     *
*                                                                      *
***********************************************************************/

LOGICAL			GRA_display_add

	(
	STRING		symbol,		/* Symbol file */
	float		scale,		/* Symbol scaling factor (percent) */
	float		rotation,	/* Symbol rotation (degrees) */
	LOGICAL		rotate_lat,	/* Rotate symbol to align with latitude? */
	LOGICAL		rotate_lon,	/* Rotate symbol to align with longitude? */
	float		xoff,		/* x offset of center of symbol (display units) */
	float		yoff,		/* y offset of center of symbol (display units) */
	STRING		lat,		/* Latitude location for symbol */
	STRING		lon,		/* Longitude location for symbol */
	STRING		map_x,		/* x axis position for symbol */
	STRING		map_y,		/* y axis position for symbol */
	float		map_units,	/* Units for x and y axis */
	STRING		loc_ident,	/* Location identifier for symbol */
	STRING		loc_lookup,	/* Location look up table */
	STRING		vtime,		/* Valid time to match */
	STRING		table_name,	/* Table name for locations */
	STRING		grid_name,	/* Grid name for locations */
	STRING		list_name	/* List name for locations */
	)

	{
	int			iix, iiy, isite, iloc, iil;
	float		pscale, flat, flon, fact, rotadj;
	float		xxo, yyo, xx, yy, xxt, yyt, xxs, yys;
	STRING		loclat, loclon, vt;
	LOGICAL		status;
	POINT		pos;
	GRA_TABLE	*cur_table;
	GRA_GRID	*cur_grid;
	GRA_LIST	*cur_list;
	char		err_buf[GPGLong];

	/* Display symbol for each table site */
	if ( !blank(table_name) )
		{

		/* Find the named table */
		cur_table = get_table(table_name);
		if ( IsNull(cur_table) )
			{
			(void) sprintf(err_buf, "Table ... %s ... not yet defined",
					table_name);
			(void) error_report(err_buf);
			}

		/* Display symbol for all table sites */
		for ( isite=0; isite<cur_table->nsites; isite++ )
			{

			/* Set table location for symbol display */
			xxt = cur_table->x_off;
			yyt = cur_table->y_off;
			if ( same(cur_table->type, TableCol) )
				yyt += cur_table->offset[isite];
			else if ( same(cur_table->type, TableRow) )
				xxt += cur_table->offset[isite];
			(void) anchored_location(ZeroPoint, xxt, yyt, &xxo, &yyo);

			/* Set offset location */
			xx = xxo + xoff;
			yy = yyo + yoff;

			/* Display symbol */
			(void) write_graphics_symbol(symbol, xx, yy, scale, rotation);
			}

		/* Return TRUE when all table sites have been displayed */
		return TRUE;
		}

	/* Display symbol for each grid location */
	else if ( !blank(grid_name) )
		{

		/* Find the named grid */
		cur_grid = get_sample_grid(grid_name);
		if ( IsNull(cur_grid) )
			{
			(void) sprintf(err_buf, "Grid ... %s ... not yet defined",
					grid_name);
			(void) error_report(err_buf);
			}

		/* Display symbol for all grid locations */
		for ( iiy=0; iiy<cur_grid->numy; iiy++ )
			for ( iix=0; iix<cur_grid->numx; iix++ )
				{

				/* Set latitude/longitude and position for display */
				flat = cur_grid->flats[iiy][iix];
				flon = cur_grid->flons[iiy][iix];
				(void) ll_to_pos(&BaseMap, flat, flon, pos);

				/* Check for grid location off the map */
				if ( pos[X] < 0.0 || pos[Y] < 0.0
						|| pos[X] > BaseMap.definition.xlen
						|| pos[Y] > BaseMap.definition.ylen )
					{

					if ( Verbose )
						{
						(void) fprintf(stdout,
								"  Grid lat/lon outside map at ... %.1f %.1f\n",
								flat, flon);
						}

					/* Continue for grid location off map */
					if ( AnchorToMap ) continue;
					}

				/* Comment for grid location on the map */
				else
					{
					if ( Verbose )
						{
						(void) fprintf(stdout,
								"  Grid lat/lon at ... %.1f %.1f\n",
								flat, flon);
						}
					}

				/* Set grid location on the current map */
				if ( AnchorToMap )
					{
					(void) anchored_location(pos, 0.0, 0.0, &xxo, &yyo);
					}

				/* Set display location to current anchor position */
				else
					{
					xxs = (float) iix * cur_grid->x_shift;
					yys = (float) iiy * cur_grid->y_shift;
					(void) anchored_location(ZeroPoint, xxs, yys, &xxo, &yyo);
					}

				/* Set offset location */
				xx = xxo + xoff;
				yy = yyo + yoff;

				/* Adjust symbol scale for perspective (if required) */
				if ( perspective_scale(&pscale) ) scale *= pscale;

				/* Set rotation from latitude or longitude */
				if ( AnchorToMap && rotate_lat )
					rotadj = wind_dir_xy(&BaseMap, flat, flon, 90.0);
				else if ( AnchorToMap && rotate_lon )
					rotadj = wind_dir_xy(&BaseMap, flat, flon,  0.0);
				else
					rotadj = 0.0;

				/* Add default rotation */
				rotadj += rotation;

				/* Display symbol */
				(void) write_graphics_symbol(symbol, xx, yy, scale, rotadj);
				}

		/* Return TRUE when all grid locations have been displayed */
		return TRUE;
		}

	/* Display symbol for each list location */
	else if ( !blank(list_name) )
		{

		/* Find the named list */
		cur_list = get_sample_list(list_name);
		if ( IsNull(cur_list) )
			{
			(void) sprintf(err_buf, "List ... %s ... not yet defined",
					list_name);
			(void) error_report(err_buf);
			}

		/* Initialize offsets for list locations */
		iil = 0;
		xxs = 0.0;
		yys = 0.0;

		/* Display symbol for all list locations */
		for ( isite=0; isite<cur_list->num; isite++ )
			{

			/* Set latitude/longitude for display */
			if ( cur_list->usell[isite] )
				{
				flat   = cur_list->flats[isite];
				flon   = cur_list->flons[isite];
				}

			/* Get latitude/longitude for display from all locations */
			/*  in a location look up table                          */
			else if ( same_ic(cur_list->idents[isite], LocIdentAll)
						|| same_ic(cur_list->idents[isite], LocIdentAllVtime) )
				{
				if ( blank(loc_lookup) )
					(void) error_report("No location look up table");
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"   Display at all locations from look up ... %s\n",
							loc_lookup);
					}

				/* Set valid time to check */
				if ( same_ic(cur_list->idents[isite], LocIdentAllVtime) )
					vt = vtime;
				else
					vt = FpaCblank;

				/* Set display locations for all look up table locations */
				iloc = -1;
				while ( (iloc = next_location_lookup_line(loc_lookup, iloc, vt,
						NullStringPtr, &loclat, &loclon, NullStringPtr)) >= 0 )
					{

					/* Set latitude/longitude for display */
					flat = read_lat(loclat, &status);
					if ( !status )
						(void) error_report("Problem with location look up lat");
					flon = read_lon(loclon, &status);
					if ( !status )
						(void) error_report("Problem with location look up lon");

					/* Set map position for display */
					(void) ll_to_pos(&BaseMap, flat, flon, pos);

					/* Check for list location off the map */
					if ( pos[X] < 0.0 || pos[Y] < 0.0
							|| pos[X] > BaseMap.definition.xlen
							|| pos[Y] > BaseMap.definition.ylen )
						{

						if ( Verbose )
							{
							(void) fprintf(stdout,
									"  List lat/lon outside map at ... %.1f %.1f\n",
									flat, flon);
							}

						/* Continue for list location off map */
						if ( AnchorToMap ) continue;
						}

					/* Comment for list location on the map */
					else
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
									"  List lat/lon at ... %.1f %.1f\n",
									flat, flon);
							}
						}

					/* Set list location on the current map */
					if ( AnchorToMap )
						{
						(void) anchored_location(pos, 0.0, 0.0, &xxo, &yyo);
						}

					/* Set display location to current anchor position   */
					/*  and set offset for each successive list location */
					else
						{
						(void) anchored_location(ZeroPoint, xxs, yys,
								&xxo, &yyo);
						iil++;
						if ( cur_list->x_wrap > 1
									&& iil % cur_list->x_wrap == 0 )
							{
							xxs  = 0.0;
							yys += cur_list->y_shift;
							}
						else if ( cur_list->x_wrap > 1 )
							{
							xxs += cur_list->x_shift;
							}
						else if ( cur_list->y_wrap > 1
									&& iil % cur_list->y_wrap == 0 )
							{
							xxs += cur_list->x_shift;
							yys  = 0.0;
							}
						else if ( cur_list->y_wrap > 1 )
							{
							yys += cur_list->y_shift;
							}
						else
							{
							xxs += cur_list->x_shift;
							yys += cur_list->y_shift;
							}
						}

					/* Set offset location */
					xx = xxo + xoff;
					yy = yyo + yoff;

					/* Adjust symbol scale for perspective (if required) */
					if ( perspective_scale(&pscale) ) scale *= pscale;

					/* Set rotation from latitude or longitude */
					if ( AnchorToMap && rotate_lat )
						rotadj = wind_dir_xy(&BaseMap, flat, flon, 90.0);
					else if ( AnchorToMap && rotate_lon )
						rotadj = wind_dir_xy(&BaseMap, flat, flon,  0.0);
					else
						rotadj = 0.0;

					/* Add default rotation */
					rotadj += rotation;

					/* Display symbol */
					(void) write_graphics_symbol(symbol, xx, yy, scale, rotadj);
					}

				/* Go on to next display location in sample list */
				continue;
				}

			/* Get latitude/longitude for display from location look up */
			else
				{
				if ( blank(loc_lookup) )
					(void) error_report("No location look up table");
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"   Matching location ident ... \"%s\"",
							cur_list->idents[isite]);
					(void) fprintf(stdout, "  from look up ... %s\n",
							loc_lookup);
					}
				if ( !match_location_lookup(loc_lookup,
						cur_list->idents[isite], vtime, &loclat, &loclon,
						NullStringPtr) )
					{
					(void) sprintf(err_buf,
							"Error matching \"%s\" in look up ... %s",
							cur_list->idents[isite], loc_lookup);
					(void) error_report(err_buf);
					}
				flat = read_lat(loclat, &status);
				if ( !status )
					(void) error_report("Problem with location look up lat");
				flon = read_lon(loclon, &status);
				if ( !status )
					(void) error_report("Problem with location look up lon");
				}

			/* Set map position for display */
			(void) ll_to_pos(&BaseMap, flat, flon, pos);

			/* Check for list location off the map */
			if ( pos[X] < 0.0 || pos[Y] < 0.0
					|| pos[X] > BaseMap.definition.xlen
					|| pos[Y] > BaseMap.definition.ylen )
				{

				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  List lat/lon outside map at ... %.1f %.1f\n",
							flat, flon);
					}

				/* Continue for list location off map */
				if ( AnchorToMap ) continue;
				}

			/* Comment for list location on the map */
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  List lat/lon at ... %.1f %.1f\n",
							flat, flon);
					}
				}

			/* Set list location on the current map */
			if ( AnchorToMap )
				{
				(void) anchored_location(pos, 0.0, 0.0, &xxo, &yyo);
				}

			/* Set display location to current anchor position   */
			/*  and set offset for each successive list location */
			else
				{
				(void) anchored_location(ZeroPoint, xxs, yys, &xxo, &yyo);
				iil++;
				if ( cur_list->x_wrap > 1 && iil % cur_list->x_wrap == 0 )
					{
					xxs  = 0.0;
					yys += cur_list->y_shift;
					}
				else if ( cur_list->x_wrap > 1 )
					{
					xxs += cur_list->x_shift;
					}
				else if ( cur_list->y_wrap > 1 && iil % cur_list->y_wrap == 0 )
					{
					xxs += cur_list->x_shift;
					yys  = 0.0;
					}
				else if ( cur_list->y_wrap > 1 )
					{
					yys += cur_list->y_shift;
					}
				else
					{
					xxs += cur_list->x_shift;
					yys += cur_list->y_shift;
					}
				}

			/* Set offset location */
			xx = xxo + xoff;
			yy = yyo + yoff;

			/* Adjust symbol scale for perspective (if required) */
			if ( perspective_scale(&pscale) ) scale *= pscale;

			/* Set rotation from latitude or longitude */
			if ( AnchorToMap && rotate_lat )
				rotadj = wind_dir_xy(&BaseMap, flat, flon, 90.0);
			else if ( AnchorToMap && rotate_lon )
				rotadj = wind_dir_xy(&BaseMap, flat, flon,  0.0);
			else
				rotadj = 0.0;

			/* Add default rotation */
			rotadj += rotation;

			/* Display symbol */
			(void) write_graphics_symbol(symbol, xx, yy, scale, rotadj);
			}

		/* Return TRUE when all list locations have been displayed */
		return TRUE;
		}

	/* Display symbol for all locations in a location look up table */
	else if ( !blank(loc_ident) && !blank(loc_lookup)
				&& ( same_ic(loc_ident, LocIdentAll)
						|| same_ic(loc_ident, LocIdentAllVtime) ) )
		{

		/* Ensure that symbols can be displayed on current map */
		if ( !AnchorToMap )
			{
			(void) warn_report("Must set anchor to map!");
			return TRUE;
			}

		/* Set valid time to check */
		if ( same_ic(loc_ident, LocIdentAllVtime) ) vt = vtime;
		else                                        vt = FpaCblank;

		/* Display symbol for all look up table locations */
		iloc = -1;
		while ( (iloc = next_location_lookup_line(loc_lookup, iloc, vt,
						NullStringPtr, &loclat, &loclon, NullStringPtr)) >= 0 )
			{

			/* Set latitude/longitude for display */
			flat = read_lat(loclat, &status);
			if ( !status )
				(void) error_report("Problem with location look up lat");
			flon = read_lon(loclon, &status);
			if ( !status )
				(void) error_report("Problem with location look up lon");

			/* Set map position for display */
			(void) ll_to_pos(&BaseMap, flat, flon, pos);

			/* Check for look up table location off the map */
			if ( pos[X] < 0.0 || pos[Y] < 0.0
					|| pos[X] > BaseMap.definition.xlen
					|| pos[Y] > BaseMap.definition.ylen )
				{

				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Lookup lat/lon outside map at ... %.1f %.1f\n",
							flat, flon);
					}

				/* Continue for look up table location off map */
				continue;
				}

			/* Comment for look up table location on the map */
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Lookup lat/lon at ... %.1f %.1f\n",
							flat, flon);
					}
				}

			/* Set offset position from map position */
			(void) anchored_location(pos, xoff, yoff, &xx, &yy);

			/* Adjust symbol scale for perspective (if required) */
			if ( perspective_scale(&pscale) ) scale *= pscale;

			/* Set rotation from latitude or longitude */
			if ( AnchorToMap && rotate_lat )
				rotadj = wind_dir_xy(&BaseMap, flat, flon, 90.0);
			else if ( AnchorToMap && rotate_lon )
				rotadj = wind_dir_xy(&BaseMap, flat, flon,  0.0);
			else
				rotadj = 0.0;

			/* Add default rotation */
			rotadj += rotation;

			/* Display symbol */
			(void) write_graphics_symbol(symbol, xx, yy, scale, rotadj);
			}

		/* Return TRUE when all look up table locations have been displayed */
		return TRUE;
		}

	/* Display symbol at a set location */
	else
		{

		/* Set an offset position based on latitude and longitude */
		if ( ( !blank(lat) && !blank(lon) )
				|| ( !blank(map_x) && !blank(map_y) )
				|| ( !blank(loc_ident) && !blank(loc_lookup) ) )
			{

			/* Ensure that symbol can be displayed on current map */
			if ( !AnchorToMap )
				{
				(void) warn_report("Must set anchor to map!");
				return TRUE;
				}

			/* Set the latitude and longitude */
			if ( !blank(lat) && !blank(lon) )
				{
				flat = read_lat(lat, &status);
				if ( !status ) (void) error_report("Problem with lat");
				flon = read_lon(lon, &status);
				if ( !status ) (void) error_report("Problem with lon");

				/* Convert latitude and longitude to map position */
				(void) ll_to_pos(&BaseMap, flat, flon, pos);
				}

			/* Set the map position (adjusted by map_units) */
			else if ( !blank(map_x) && !blank(map_y) )
				{
				(void) sscanf(map_x, "%f", &pos[X]);
				(void) sscanf(map_y, "%f", &pos[Y]);
				fact = map_units / BaseMap.definition.units;
				pos[X] *= fact;
				pos[Y] *= fact;

				/* Convert map position to latitude and longitude */
				(void) pos_to_ll(&BaseMap, pos, &flat, &flon);
				}

			/* Get the latitude and longitude from location look up table */
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"   Matching location ident ... \"%s\"",
							loc_ident);
					(void) fprintf(stdout, "  from look up ... %s\n",
							loc_lookup);
					}
				if ( !match_location_lookup(loc_lookup, loc_ident,
						vtime, &loclat, &loclon, NullStringPtr) )
					{
					(void) sprintf(err_buf,
							"Error matching \"%s\" in look up ... %s",
							loc_ident, loc_lookup);
					(void) error_report(err_buf);
					}
				flat = read_lat(loclat, &status);
				if ( !status )
					(void) error_report("Problem with location look up lat");
				flon = read_lon(loclon, &status);
				if ( !status )
					(void) error_report("Problem with location look up lon");

				/* Convert latitude and longitude to map position */
				(void) ll_to_pos(&BaseMap, flat, flon, pos);
				}

			/* Return for map positions off the map */
			if ( pos[X] < 0.0 || pos[Y] < 0.0
					|| pos[X] > BaseMap.definition.xlen
					|| pos[Y] > BaseMap.definition.ylen )
				{
				if ( Verbose )
					{
					if ( !blank(map_x) && !blank(map_y) )
						(void) fprintf(stdout,
								"  Symbol position off map at ... %f %f\n",
								pos[X], pos[Y]);
					else
						(void) fprintf(stdout,
								"  Symbol lat/lon off map at ... %.1f %.1f\n",
								flat, flon);
					}

				return TRUE;
				}

			/* Comments for map positions on the map */
			else
				{
				if ( Verbose )
					{
					if ( !blank(map_x) && !blank(map_y) )
						(void) fprintf(stdout,
								"  Symbol position at ... %f %f\n",
								pos[X], pos[Y]);
					else
						(void) fprintf(stdout,
								"  Symbol lat/lon at ... %.1f %.1f\n",
								flat, flon);
					}
				}

			/* Set offset position from map position */
			(void) anchored_location(pos, xoff, yoff, &xx, &yy);
			}

		/* Set an absolute offset position */
		else
			{
			(void) anchored_location(ZeroPoint, xoff, yoff, &xx, &yy);
			}

		/* Adjust symbol scale for perspective (if required) */
		if ( perspective_scale(&pscale) ) scale *= pscale;

		/* Set rotation from latitude or longitude */
		if ( AnchorToMap && rotate_lat )
			rotadj = wind_dir_xy(&BaseMap, flat, flon, 90.0);
		else if ( AnchorToMap && rotate_lon )
			rotadj = wind_dir_xy(&BaseMap, flat, flon,  0.0);
		else
			rotadj = 0.0;

		/* Add default rotation */
		rotadj += rotation;

		/* Display symbol */
		(void) write_graphics_symbol(symbol, xx, yy, scale, rotadj);

		/* Return TRUE when symbol has been displayed */
		return	TRUE;
		}
	}

/***********************************************************************
*                                                                      *
*    G R A _ d i s p l a y _ a l l _ a r e a s                         *
*                                                                      *
***********************************************************************/

LOGICAL			GRA_display_all_areas

	(
	GRA_FLD_INFO	*area_flds,		/* Fields for areas */
	int				num_fields,		/* Number of fields for areas */
	STRING			area_type,		/* Type of area feature to display */
	STRING			cat_cascade,	/* And/Or for multiple CATATTRIB structs */
	CATATTRIB		*cat_attrib,	/* Structure containing categories */
	int				num_catatt,		/* Number of categories */
	STRING			attribute,		/* Attribute containing value to match */
	STRING			look_up,		/* Look up table for presentation */
	STRING			interior_fill,	/* Colour for interior of areas */
	STRING			sym_fill_name,	/* Symbol pattern for interior of areas */
	STRING			pattern,		/* Name of pattern for lines */
	STRING			pattern_width,	/* Pattern width factor */
	STRING			pattern_length,	/* Pattern repetition factor */
	LOGICAL			hole_pattern,	/* Display pattern for holes too? */
	COMP_PRES		*comp_pres,		/* Structure containing presentations */
	int				num_comp,		/* Number of presentations */
	SPCASE			*unused_list_case,	/* Not yet used */
	int				unused_num_list		/* Not yet used */
	)

	{
	STRING		stype, value, pname;
	LOGICAL		cat_match, lcat_match, rcat_match, cur_anchor, clip_to_map;
	PRES		cur_pres, named_pres;
	char		all_fields[GPGLong];
	char		out_buf[GPGLong], err_buf[GPGLong];

	int			nn, num_bounds, num_divlines;
	int			iarea, isub, ivis, ihole, idiv, iout;
	int			rformat, nseg, iseg;
	SET			set = NullSet;
	BOX			box;
	AREA		area, carea;
	SUBAREA		subarea, lsub, rsub;
	SUBVIS		subvis;

	BOUND		bound;
	LINE		hole, outline, divline;
	BOUND		*bounds  = NullBoundList;
	LINE		*lsegs    = NullLineList;
	LINE		*divlines = NullLineList;
	STRING		*values   = NullStringList;
	HAND		*divsense = NullPtr(HAND *);

	LOGICAL		inside, closed, clockwise;
	HAND		sense;              /* pattern handedness (flip sense) */

	FLD_DESCRIPT	descript;

	/* Loop to check one (or more) fields for areas */
	(void) strcpy(all_fields, FpaCblank);
	for ( nn=0; nn<num_fields; nn++ )
		{

		/* Make a copy of the global field descriptor */
		(void) copy_fld_descript(&descript, &Fdesc);

		/* Re-initialize the field descriptor for this element and level */
		if ( !set_fld_descript(&descript, FpaF_DIRECTORY_PATH, FpaCblank,
									FpaF_SOURCE_NAME,   CurSource,
									FpaF_RUN_TIME,      FpaCblank,
									FpaF_ELEMENT_NAME,  area_flds[nn].element,
									FpaF_LEVEL_NAME,    area_flds[nn].level,
									FpaF_VALID_TIME,    TVstamp,
									FpaF_END_OF_LIST) )
			{
			(void) sprintf(err_buf,
					" Error setting field descriptor for ... %s %s from %s at %s\n",
					area_flds[nn].element, area_flds[nn].level,
					CurSource, TVstamp);
			(void) error_report(err_buf);
			}

		/* Add field name to output buffer */
		(void) strcat(all_fields, " ");
		(void) strcat(all_fields, area_flds[nn].element);
		(void) strcat(all_fields, " ");
		(void) strcat(all_fields, area_flds[nn].level);
		(void) strcat(all_fields, " ");
		if ( Verbose )
			{
			(void) fprintf(stdout,
					" Retrieving areas for field ... %s %s from %s at %s\n",
					area_flds[nn].element, area_flds[nn].level,
					CurSource, TVstamp);
			}

		/* Warning if field format is not Complex */
		rformat = field_display_format(area_flds[nn].element,
										area_flds[nn].level);
		if ( rformat != DisplayFormatComplex )
			{
			(void) sprintf(err_buf,
						   "Field ... %s %s is not \"display_format = complex\"\n",
							area_flds[nn].element, area_flds[nn].level);
			/* >>>>> not ready yet!
			(void) warn_report(err_buf);
			<<<<< */
			}

		/* Add areas to the list */
		set = append_set(set, retrieve_areaset(&descript));
		}

	/* Return now if no areas found */
	if ( IsNull(set) )
		{
		if ( Verbose )
			{
			(void) fprintf(stdout,
					" No areas for field(s) ... %s from %s at %s\n",
					all_fields, CurSource, TVstamp);
			}
		return TRUE;
		}
	(void) recall_set_type(set, &stype);
	if ( set->num <= 0 || !same(stype, "area") )
		{
		if ( Verbose )
			{
			(void) fprintf(stdout,
					" No areas for field(s) ... %s from %s at %s\n",
					all_fields, CurSource, TVstamp);
			}
		(void) destroy_set(set);
		return TRUE;
		}

	/* Strip out all areas that fall outside the display map */
	box.left   = 0.0;
	box.right  = BaseMap.definition.xlen;
	box.bottom = 0.0;
	box.top    = BaseMap.definition.ylen;
	(void) strip_set(set, &box);

	/* Return now if no remaining areas */
	if ( set->num <= 0 )
		{
		if ( Verbose )
			{
			(void) fprintf(stdout, " No areas within map");
			(void) fprintf(stdout, " for field(s) ... %s from %s at %s\n",
					all_fields, CurSource, TVstamp);
			}
		(void) destroy_set(set);
		return TRUE;
		}

	/* Remove all areas that do not match the required categories */
	for ( iarea=set->num-1; iarea>=0; iarea-- )
		{

		/* Define each area in the list */
		area = (AREA) set->list[iarea];

		/* Check areas with no subareas */
		if ( area->numdiv == 0 )
			{

			/* Check for areas that match the desired categories         */
			/* Note that each category may contain more than one member! */
			cat_match = match_category_attributes(area->attrib,
										cat_cascade, cat_attrib, num_catatt);
			}

		/* Check areas with subareas */
		else
			{

			/* Loop through subareas */
			cat_match = FALSE;
			for ( isub=0; isub<=area->numdiv; isub++ )
				{

				/* Check for areas with any subareas that match the desired  */
				/*  categories                                               */
				/* Note that each category may contain more than one member! */
				subarea = area->subareas[isub];
				if ( match_category_attributes(subarea->attrib,
										cat_cascade, cat_attrib, num_catatt) )
					{
					cat_match = TRUE;
					break;
					}
				}
			}

		/* Remove areas that are not one of the desired categories,  */
		/*  or areas that have no subareas of the desired categories */
		if ( !cat_match )
			{
			(void) remove_item_from_set(set, set->list[iarea]);
			}
		}

	/* Return now if no remaining areas */
	if ( set->num <= 0 )
		{
		if ( Verbose )
			{
			(void) fprintf(stdout, " No areas matching any category\n");
			(void) fprintf(stdout,
					"            for field(s) ... %s from %s at %s\n",
					all_fields, CurSource, TVstamp);
			}
		(void) destroy_set(set);
		return TRUE;
		}

	/* >>>>> Here is where the "cases" are evaluated, if required <<<<< */
	/* >>>>>
	if ( num_list > 0 )
		{
		}
	<<<<< */

	/* Now create a list of outlines or lines from the remaining areas  */
	/* Note that this is done in reverse order, to maintain precedence! */
	for ( num_bounds=0, num_divlines=0, iarea=set->num-1; iarea>=0; iarea-- )
		{

		/* Define each area in the list */
		area = (AREA) set->list[iarea];

		/* Create outlines from boundary and area holes */
		if ( same(area_type, AreaTypeBoundary) )
			{

			/* Make a copy of the area containing just the boundary and holes */
			carea = create_area(NULL, NULL, NULL);
			define_area_boundary(carea, copy_line(area->bound->boundary));
			for ( ihole=0; ihole<area->bound->numhole; ihole++ )
				add_area_hole(carea, copy_line(area->bound->holes[ihole]));

			/* Prepare visible subareas for display */
			prep_area_complex(carea);
			subarea = carea->subareas[0];

			/* Loop through visible subarea outlines */
			for ( ivis=0; ivis<subarea->nsubvis; ivis++ )
				{
				subvis = subarea->subvis[ivis];
				if (IsNull(subvis)) continue;

				/* Allocate space in the list */
				num_bounds++;
				bounds = GETMEM(bounds, BOUND,  num_bounds);
				values = GETMEM(values, STRING, num_bounds);

				/* Make a boundary from visible subarea segments */
				outline = outline_from_subvis(subvis);
				bound   = create_bound();
				define_bound_boundary(bound, outline);

				/* Add subarea holes enclosed by this visible region */
				for ( ihole=0; ihole<subarea->numhole; ihole++ )
					{
					hole = subarea->holes[ihole];
					line_test_point(outline, hole->points[0], NullFloat,
									NullPoint, NullInt, &inside, NullChar);
					if (inside) add_bound_hole(bound, copy_line(hole));
					}

				/* Prepare the holes for display */
				(void) prep_area_bound_holes(bound);

				/* Add the boundary with holes to the list */
				bounds[num_bounds-1] = bound;

				/* Add the attribute value to the list */
				value = CAL_get_attribute(subarea->attrib, attribute);
				values[num_bounds-1] = strdup(SafeStr(value));
				}
			carea = destroy_area(carea);
			}

		/* Create BOUND structure from subarea outlines and enclosed holes */
		else if ( same(area_type, AreaTypeSubareas) )
			{

			/* Ensure that subareas have been created */
			build_area_subareas(area);

			/* Loop through subareas */
			for ( isub=0; isub<=area->numdiv; isub++ )
				{

				/* Check that this subarea is one of the desired categories  */
				/* Note that each category may contain more than one member! */
				subarea   = area->subareas[isub];
				cat_match = match_category_attributes(subarea->attrib,
										cat_cascade, cat_attrib, num_catatt);

				/* Add the subarea outlines to the list */
				if ( cat_match )
					{

					/* Prepare visible subareas for display */
					prep_subarea_complex(area, subarea);

					/* Loop through visible subarea outlines */
					for ( ivis=0; ivis<subarea->nsubvis; ivis++ )
						{
						subvis = subarea->subvis[ivis];
						if (IsNull(subvis)) continue;

						/* Allocate space in the list */
						num_bounds++;
						bounds = GETMEM(bounds, BOUND,  num_bounds);
						values = GETMEM(values, STRING, num_bounds);

						/* Make a boundary from visible subarea segments */
						outline = outline_from_subvis(subvis);
						bound   = create_bound();
						define_bound_boundary(bound, outline);

						/* Add subarea holes enclosed by this visible region */
						for ( ihole=0; ihole<subarea->numhole; ihole++ )
							{
							hole = subarea->holes[ihole];
							line_test_point(outline, hole->points[0], NullFloat,
											NullPoint, NullInt, &inside,
											NullChar);
							if (inside) add_bound_hole(bound, copy_line(hole));
							}

						/* Prepare the holes for display */
						(void) prep_area_bound_holes(bound);

						/* Add the boundary with holes to the list */
						bounds[num_bounds-1] = bound;

						/* Add the attribute value to the list */
						value = CAL_get_attribute(subarea->attrib, attribute);
						values[num_bounds-1] = strdup(SafeStr(value));
						}
					}
				}
			}

		/* Create lines from areas with dividing lines */
		else if ( same(area_type, AreaTypeDivides) && area->numdiv > 0 )
			{

			/* Loop through dividing lines */
			for ( idiv=0; idiv<area->numdiv; idiv++ )
				{

				/* Check that this dividing line borders a subarea with one  */
				/*  of the desired categories                                */
				/* Note that each category may contain more than one member! */
				(void) adjacent_subareas(area, idiv, &lsub, &rsub);
				lcat_match = match_category_attributes(lsub->attrib,
										cat_cascade, cat_attrib, num_catatt);
				rcat_match = match_category_attributes(rsub->attrib,
										cat_cascade, cat_attrib, num_catatt);

				/* Add the dividing line to the list */
				if ( lcat_match || rcat_match )
					{

					/* Clip dividing line with area holes (if required) */
					nseg = clip_line_by_area_holes(area->divlines[idiv],
													area, &lsegs);

					/* Allocate space in list for each dividing line segment */
					for ( iseg=0; iseg<nseg; iseg++ )
						{
						num_divlines++;
						divlines = GETMEM(divlines,  LINE,   num_divlines);
						values   = GETMEM(values,    STRING, num_divlines);
						divsense = GETMEM(divsense,  HAND,   num_divlines);

						/* Add the dividing line segment to the list */
						divlines[num_divlines-1] = lsegs[iseg];
						if ( lcat_match ) divsense[num_divlines-1] = Left;
						else              divsense[num_divlines-1] = Right;

						/* Add the attribute value to the list */
						if ( lcat_match )
							value = CAL_get_attribute(lsub->attrib, attribute);
						else
							value = CAL_get_attribute(rsub->attrib, attribute);
						values[num_divlines-1] = strdup(SafeStr(value));
						}
					}
				}
			}
		}

	/* Force all locations to be anchored to the current map */
	/*  ... regardless of the current anchor position!       */
	cur_anchor  = AnchorToMap;
	clip_to_map = TRUE;
	AnchorToMap = TRUE;

	/* Output outlines with holes from boundary or subareas */
	if ( same(area_type, AreaTypeBoundary)
			|| same(area_type, AreaTypeSubareas) )
		{
		(void) sprintf(out_buf,
				"### Begin outlines for field(s) ... %s", all_fields);
		(void) write_graphics_comment(out_buf);
		if ( Verbose )
			{
			(void) fprintf(stdout,
					" Drawing areas for field(s) ... %s from %s at %s\n",
					all_fields, CurSource, TVstamp);
			}

		/* Now loop to output each outline with holes */
		for ( iout=0; iout<num_bounds; iout++ )
			{

			/* Set outline and holes */
			bound = bounds[iout];

			/* Set the current presentation parameters */
			(void) copy_presentation(&cur_pres, &CurPres);

			/* Reset presentation parameters from look up table (if required) */
			if ( !blank(look_up) )
				{

				/* Find presentation for value matched in look up table */
				if ( match_category_lookup(look_up, values[iout],
						NullStringPtr, NullStringPtr, &pname) )
					{

					if ( Verbose )
						{
						(void) fprintf(stdout,
								"   Matching presentation for ... \"%s\"",
								values[iout]);
						(void) fprintf(stdout, "  from look up ... %s\n", look_up);
						}

					/* Match presentation from list of named presentations */
					named_pres = get_presentation(pname);
					(void) copy_presentation(&cur_pres, &named_pres);
					}

				/* Warning if value cannot be matched */
				else
					{
					(void) fprintf(stdout, "   Presentation for ... \"%s\"",
							values[iout]);
					(void) fprintf(stdout, "  not found in look up ... %s\n",
							look_up);
					}
				}

			/* Override interior_fill and symbol_fill_name (if requested) */
			if ( !blank(interior_fill) )
						(void) strcpy(cur_pres.interior_fill, interior_fill);
			if ( !blank(sym_fill_name) )
						(void) strcpy(cur_pres.sym_fill_name, sym_fill_name);

			/* Override pattern and pattern sizes (if requested) */
			if ( !blank(pattern) )
						(void) strcpy(cur_pres.pattern,        pattern);
			if ( !blank(pattern_width) )
						(void) strcpy(cur_pres.pattern_width,  pattern_width);
			if ( !blank(pattern_length) )
						(void) strcpy(cur_pres.pattern_length, pattern_length);

			/* Override component presentation (if requested) */
			if ( num_comp > 0 )
				{
				cur_pres.num_comp  = num_comp;
				cur_pres.comp_pres = comp_pres;
				(void) reset_presentation_by_comp_pres(&cur_pres,
														&cur_pres.comp_pres[0]);
				}

			/* Start of outline grouping */
			(void) sprintf(out_buf,
					"### Begin outline for ... %s", values[iout]);
			(void) write_graphics_comment(out_buf);
			(void) write_graphics_group(GPGstart, NullPointer, 0);

			/* Display a "simple" outline with holes */
			if ( blank(cur_pres.pattern)
					|| same(cur_pres.pattern, PatternSimple) )
				{

				/* Display the outline with holes */
				if ( !GRA_display_simple_boundary(bound, 0.0, 0.0,
						cur_pres, clip_to_map) )
					{
					(void) error_report("Error drawing simple boundary");
					}
				}

			/* Display an outline with holes using the given pattern */
			else
				{

				/* Set direction to draw outline */
				(void) line_properties(bound->boundary, &closed, &clockwise,
						NullFloat, NullFloat);
				if ( clockwise ) sense = Right;
				else             sense = Left;

				/* Display the outline with holes */
				if ( !GRA_display_patterned_boundary(bound, 0.0, 0.0,
						cur_pres, clip_to_map, sense, hole_pattern) )
					{
					(void) error_report("Error drawing patterned boundary");
					}
				}

			/* End of outline grouping */
			(void) write_graphics_group(GPGend, NullPointer, 0);
			(void) sprintf(out_buf, "### End outline for ... %s", values[iout]);
			(void) write_graphics_comment(out_buf);
			}

		(void) sprintf(out_buf,
				"### End outlines for field(s) ... %s", all_fields);
		(void) write_graphics_comment(out_buf);

		/* Free space used by work objects */
		for ( iout=0; iout<num_bounds; iout++ )
			{
			(void) destroy_bound(bounds[iout]);
			}
		FREEMEM(bounds);
		FREELIST(values, num_bounds);
		(void) destroy_set(set);
		}

	/* Output dividing lines */
	else if ( same(area_type, AreaTypeDivides) )
		{
		(void) sprintf(out_buf,
				"### Begin dividing lines for field(s) ... %s", all_fields);
		(void) write_graphics_comment(out_buf);
		if ( Verbose )
			{
			(void) fprintf(stdout,
					" Drawing dividing lines for field(s) ... %s from %s at %s\n",
					all_fields, CurSource, TVstamp);
			}

		/* Now loop to output each dividing line */
		for ( iout=0; iout<num_divlines; iout++ )
			{

			/* Set dividing line */
			divline = divlines[iout];
			sense   = divsense[iout];

			/* Set the current presentation parameters */
			(void) copy_presentation(&cur_pres, &CurPres);

			/* Reset presentation parameters from look up table (if required) */
			if ( !blank(look_up) )
				{

				/* Find presentation for value matched in look up table */
				if ( match_category_lookup(look_up, values[iout],
						NullStringPtr, NullStringPtr, &pname) )
					{

					if ( Verbose )
						{
						(void) fprintf(stdout,
								"   Matching presentation for ... \"%s\"",
								values[iout]);
						(void) fprintf(stdout, "  from look up ... %s\n", look_up);
						}

					/* Match presentation from list of named presentations */
					named_pres = get_presentation(pname);
					(void) copy_presentation(&cur_pres, &named_pres);
					}

				/* Warning if value cannot be matched */
				else
					{
					(void) fprintf(stdout, "   Presentation for ... \"%s\"",
							values[iout]);
					(void) fprintf(stdout, "  not found in look up ... %s\n",
							look_up);
					}
				}

			/* Override pattern and pattern sizes (if requested) */
			if ( !blank(pattern) )
						(void) strcpy(cur_pres.pattern,        pattern);
			if ( !blank(pattern_width) )
						(void) strcpy(cur_pres.pattern_width,  pattern_width);
			if ( !blank(pattern_length) )
						(void) strcpy(cur_pres.pattern_length, pattern_length);

			/* Override component presentation (if requested) */
			if ( num_comp > 0 )
				{
				cur_pres.num_comp  = num_comp;
				cur_pres.comp_pres = comp_pres;
				(void) reset_presentation_by_comp_pres(&cur_pres,
														&cur_pres.comp_pres[0]);
				}

			/* Start of dividing line grouping */
			(void) sprintf(out_buf,
					"### Begin dividing line for ... %s", values[iout]);
			(void) write_graphics_comment(out_buf);
			(void) write_graphics_group(GPGstart, NullPointer, 0);

			/* Display a "simple" dividing line */
			if ( blank(cur_pres.pattern)
					|| same(cur_pres.pattern, PatternSimple) )
				{

				/* Display the dividing line */
				if ( !GRA_display_simple_line(divline, 0.0, 0.0,
						cur_pres, clip_to_map) )
					{
					(void) error_report("Error drawing simple line");
					}
				}

			/* Display a dividing line using the given pattern */
			else
				{

				/* Display the dividing line */
				if ( !GRA_display_patterned_line(divline, 0.0, 0.0,
						cur_pres, clip_to_map, sense) )
					{
					(void) error_report("Error drawing patterned line");
					}
				}

			/* End of dividing line grouping */
			(void) write_graphics_group(GPGend, NullPointer, 0);
			(void) sprintf(out_buf,
					"### End dividing line for ... %s", values[iout]);
			(void) write_graphics_comment(out_buf);
			}

		(void) sprintf(out_buf,
				"### End dividing lines for field(s) ... %s", all_fields);
		(void) write_graphics_comment(out_buf);

		/* Free space used by work objects */
		for ( iout=0; iout<num_divlines; iout++ )
			{
			(void) destroy_line(divlines[iout]);
			}
		FREEMEM(divlines);
		FREEMEM(divsense);
		FREELIST(values, num_divlines);
		(void) destroy_set(set);
		}

	/* Reset the current anchor and return TRUE */
	AnchorToMap = cur_anchor;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    G R A _ d i s p l a y _ a l l _ c o n t o u r s                   *
*                                                                      *
***********************************************************************/

LOGICAL			GRA_display_all_contours

	(
	STRING		element,		/* Element name */
	STRING		level,			/* Level name */
	STRING		equation,		/* Equation to calculate */
	STRING		units,			/* Units for contours */
	STRING		values,			/* List of values for contours */
	STRING		min,			/* Minimum value for contours */
	STRING		max,			/* Maximum value for contours */
	STRING		base,			/* Base value for contours */
	STRING		interval,		/* Interval between contours */
	LOGICAL		display_areas,	/* Display banded contours? */
	STRING		interior_fill,	/* Colour for interior of banded contours */
	STRING		sym_fill_name,	/* Symbol pattern for banded contours */
	STRING		pattern,		/* Name of pattern for contours */
	STRING		pattern_width,	/* Pattern width factor */
	STRING		pattern_length,	/* Pattern repetition factor */
	COMP_PRES	*comp_pres,		/* Structure containing presentations */
	int			num_comp		/* Number of presentations */
	)

	{
	int			fkind;
	float		fval, fminv, fmaxv, fbase, fint, vmin, vmax, diff;
	double		dval;
	LOGICAL		status = FALSE;
	char		err_buf[GPGLong];

	SURFACE		sfc;
	LOGICAL		cur_anchor, clip_to_map;

	FLD_DESCRIPT	descript;

	FpaConfigFieldStruct	*fdef;
	FpaConfigUnitStruct		*udef;

	/* Make a copy of the global field descriptor */
	(void) copy_fld_descript(&descript, &Fdesc);

	/* Re-initialize the field descriptor for this element and level */
	if ( !set_fld_descript(&descript, FpaF_DIRECTORY_PATH, FpaCblank,
									   FpaF_SOURCE_NAME,   CurSource,
									   FpaF_RUN_TIME,      FpaCblank,
									   FpaF_ELEMENT_NAME,  element,
									   FpaF_LEVEL_NAME,    level,
									   FpaF_VALID_TIME,    TVstamp,
									   FpaF_END_OF_LIST) )
		{
		(void) sprintf(err_buf,
				" Error setting field descriptor for ... %s %s from %s at %s\n",
				element, level, CurSource, TVstamp);
		(void) error_report(err_buf);
		}

	/* Set the field type for sampling by equation */
	if ( !blank(equation) )
		{
		fkind = FpaC_CONTINUOUS;
		}

	/* Set the field type from the element and level */
	else
		{
		fdef = get_field_info(descript.edef->name, descript.ldef->name);
		if ( IsNull(fdef) )
			{
			(void) sprintf(err_buf,
					"Unrecognized element ... %s  or level ... %s",
					element, level);
			(void) error_report(err_buf);
			}
		fkind = fdef->element->fld_type;

		/* Check that units match with field information */
		switch ( fkind )
			{

			/* Must match field units for continuous/vector fields */
			case FpaC_CONTINUOUS:
			case FpaC_VECTOR:
				udef = fdef->element->elem_io->units;
				if ( NotNull(udef)
						&& !convert_value(udef->name, 0.0, units, NullDouble) )
					{
					(void) sprintf(err_buf,
							"Incorrect units: %s  for field: %s %s with units %s",
							units, element, level, udef->name);
					(void) error_report(err_buf);
					}
				break;
			}
		}

	/* Retrieve contour surface depending on field type */
	switch ( fkind )
		{

		/* Retrieve contour surface for continuous type fields */
		case FpaC_CONTINUOUS:

			/* Retrieve contour surface by equation */
			if ( !blank(equation) )
				{
				sfc = retrieve_surface_by_equation(&descript, FpaCmksUnits,
						equation);
				if ( IsNull(sfc) )
					{
					if ( Verbose )
						{
						(void) fprintf(stdout, " No field for ... %s",
								equation);
						(void) fprintf(stdout, "  from %s at %s\n",
								CurSource, TVstamp);
						}
					return TRUE;
					}
				}

			/* Retrieve contour surface directly */
			else
				{
				sfc = retrieve_surface(&descript);
				if ( IsNull(sfc) )
					{
					if ( Verbose )
						{
						(void) fprintf(stdout, " No field for ... %s %s",
								element, level);
						(void) fprintf(stdout, "  from %s at %s\n",
								CurSource, TVstamp);
						}
					return TRUE;
					}
				}
			break;

		/* Retrieve contour surface for vector type fields */
		case FpaC_VECTOR:
			sfc = retrieve_surface(&descript);
			if ( IsNull(sfc) )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout, " No field for ... %s %s",
							element, level);
					(void) fprintf(stdout, "  from %s at %s\n",
							CurSource, TVstamp);
					}
				return TRUE;
				}
			break;

		/* Default for all other types of fields */
		case FpaC_DISCRETE:
		case FpaC_LINE:
		case FpaC_SCATTERED:
		case FpaC_LCHAIN:
		default:
			if ( Verbose )
				{
				(void) fprintf(stdout,
					"Cannot contour field ... %s %s from %s at %s\n",
					element, level, CurSource, TVstamp);
				}
			return TRUE;
		}

	/* Force all locations to be anchored to the current map */
	/*  ... regardless of the current anchor position!       */
	cur_anchor  = AnchorToMap;
	clip_to_map = TRUE;
	AnchorToMap = TRUE;

	/* Display contours as areas */
	if ( display_areas )
		{

		/* For a single contour area ...       */
		/*  only contour max and min are given */
		if ( blank(base) || blank(interval) )
			{

			if ( Verbose )
				{
				(void) fprintf(stdout,
						" Plotting contour area for %s to %s\n", min, max);
				(void) fprintf(stdout,
						"            for ... %s %s from %s at %s\n",
						element, level, CurSource, TVstamp);
				}

			/* Extract the contour minimum and maximum */
			(void) sscanf(min, "%f", &fminv);
			(void) sscanf(max, "%f", &fmaxv);

			/* Display the contour area */
			if ( !GRA_display_contour_area(sfc, fminv, fmaxv, units,
					interior_fill, sym_fill_name, pattern, pattern_width,
					pattern_length, comp_pres, num_comp, clip_to_map) )
				(void) error_report("Error displaying contour area ...");
			}

		/* For contour areas at set intervals ...               */
		/*  the contour min, max, base, and interval are given  */
		else
			{

			if ( Verbose )
				{
				(void) fprintf(stdout,
						" Plotting contour areas for %s to %s every %s",
						min, max, interval);
				(void) fprintf(stdout,
						" starting at %s\n", base);
				(void) fprintf(stdout,
						"            for ... %s %s from %s at %s\n",
						element, level, CurSource, TVstamp);
				}

			/* Extract the initial contour parameters */
			(void) sscanf(min,      "%f", &fminv);
			(void) sscanf(max,      "%f", &fmaxv);
			(void) sscanf(base,     "%f", &fbase);
			(void) sscanf(interval, "%f", &fint);

			/* Find the actual contours available in the given field */
			(void) find_surface_range(sfc, &vmin, &vmax);
			(void) convert_value(sfc->units.name, (double) vmin, units, &dval);
			vmin = (float) dval;
			(void) convert_value(sfc->units.name, (double) vmax, units, &dval);
			vmax = (float) dval;

			/* Reset the initial contour min and max, if required */
			if ( vmin > fminv ) fminv = vmin;
			if ( vmax < fmaxv ) fmaxv = vmax;

			/* Reset the minimum contour value, adjusted to the base value */
			diff   = fmod((fminv - fbase), fint);
			fminv -= diff;
			if ( diff <= 0 ) fminv -= fint;

			/* Reset the maximum contour value, adjusted to the base value */
			diff   = fmod((fmaxv - fbase), fint);
			fmaxv -= diff;
			if ( diff >= 0 ) fmaxv += fint;

			/* Display each contour area from min to max at each interval */
			fval = fminv;
			while ( fval < fmaxv )
				{

				/* Display the contour area */
				if ( !GRA_display_contour_area(sfc, fval, (fval + fint), units,
						interior_fill, sym_fill_name, pattern, pattern_width,
						pattern_length, comp_pres, num_comp, clip_to_map) )
					(void) error_report("Error displaying contour area ...");

				/* Increment the contour value */
				fval += fint;
				}
			}
		}

	/* Display contours as lines */
	else
		{

		/* For a list of contours ...                                  */
		/*  "values" contains a white-separated list of contour values */
		if ( !blank(values) )
			{

			if ( Verbose )
				{
				(void) fprintf(stdout,
						" Plotting contour(s) %s\n", values);
				(void) fprintf(stdout,
						"            for ... %s %s from %s at %s\n",
						element, level, CurSource, TVstamp);
				}

			/* Display one or more specific contours */
			while ( !blank(values) )
				{

				/* Extract next contour value in the list */
				fval = float_arg(values, &status);
				if ( !status ) (void) error_report("Error in contour values");

				/* Display the contour */
				if ( !GRA_display_contour_line(sfc, fval, units,
						pattern, pattern_width, pattern_length,
						comp_pres, num_comp, clip_to_map) )
					(void) error_report("Error displaying contour line ...");
				}
			}

		/* For a series of contours at set intervals ...       */
		/*  the contour min, max, base, and interval are given */
		else
			{

			if ( Verbose )
				{
				(void) fprintf(stdout,
						" Plotting contours from %s to %s every %s starting at %s\n",
						min, max, interval, base);
				(void) fprintf(stdout,
						"            for ... %s %s from %s at %s\n",
						element, level, CurSource, TVstamp);
				}

			/* Extract the initial contour parameters */
			(void) sscanf(min,      "%f", &fminv);
			(void) sscanf(max,      "%f", &fmaxv);
			(void) sscanf(base,     "%f", &fbase);
			(void) sscanf(interval, "%f", &fint);

			/* Find the actual contours available in the given field */
			(void) find_surface_range(sfc, &vmin, &vmax);
			(void) convert_value(sfc->units.name, (double) vmin, units, &dval);
			vmin = (float) dval;
			(void) convert_value(sfc->units.name, (double) vmax, units, &dval);
			vmax = (float) dval;

			/* Reset the initial contour min and max, if required */
			if ( vmin > fminv ) fminv = vmin;
			if ( vmax < fmaxv ) fmaxv = vmax;

			/* Reset the minimum contour value, adjusted to the base value */
			diff   = fmod((fminv - fbase), fint);
			fminv -= diff;
			if ( diff > 0 ) fminv += fint;

			/* Reset the maximum contour value, adjusted to the base value */
			diff   = fmod((fmaxv - fbase), fint);
			fmaxv -= diff;
			if ( diff < 0 ) fmaxv -= fint;

			/* Display each contour from min to max at each interval */
			fval = fminv;
			while ( fval <= fmaxv )
				{

				/* Display the contour */
				if ( !GRA_display_contour_line(sfc, fval, units,
						pattern, pattern_width, pattern_length,
						comp_pres, num_comp, clip_to_map) )
					(void) error_report("Error displaying contour line ...");

				/* Increment the contour value */
				fval += fint;
				}
			}
		}

	/* Reset the current anchor */
	AnchorToMap = cur_anchor;

	/* Free space used by SURFACE object and return TRUE */
	sfc = destroy_surface(sfc);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    G R A _ d i s p l a y _ a l l _ i m a g e s                       *
*                                                                      *
***********************************************************************/

LOGICAL			GRA_display_all_images

	(
	GRA_IMAGE_INFO	*image_info,	/* Image information */
	int				num_images,		/* Number of images */
	STRING			sat_bright,		/* Default satellite image brightness */
	STRING			radar_bright,	/* Default radar image brightness */
	STRING			vtime,			/* Valid time to match */
	STRING			match_before,	/* Default before time */
	STRING			match_after,	/* Default after time */
	STRING			sat_before,		/* Satellite before time */
	STRING			sat_after,		/* Satellite after time */
	STRING			radar_before,	/* Radar before time */
	STRING			radar_after,	/* Radar after time */
	LOGICAL			blend_images,	/* Blend satellite and radar images? */
	float			blend_ratio,	/* Ratio for blending (percent radar) */
	LOGICAL			range_rings,	/* Display radar range rings? */
	float			range_interval,	/* Radar range ring interval (in km) */
	STRING			range_colour,	/* Colour for range rings */
	LOGICAL			limit_ring,		/* Display radar limit ring? */
	STRING			limit_colour	/* Colour for limit ring */
	)

	{
	LOGICAL		cur_anchor;
	char		radar_images[GPGHuge], sat_images[GPGHuge];
	char		out_buf[GPGHuge], err_buf[GPGLong];

	int			view_window, brightness;
	int			num_radar, num_sat, nn, nvt, pos;
	Image		*radar_list = (Image *)0;
	Image		*sat_list   = (Image *)0;
	Image		current_image, final_image, radar_image, sat_image;
	ImageLUT	lut;
	STRING		*vtimes, *ftimes;
	STRING		match_b, match_a;

	int			nf, nl;

	enum IMAGE_TYPE	image_type;

	/* Set default brightness for radar & satellite
	 * If an image tag's brightness is not set these will take effect
	 */
	if (!blank(sat_bright))
	{
		brightness = atoi(sat_bright);
		if ( brightness < 1 || brightness > 100 )
		{
			(void) sprintf(err_buf,
						   "Error in satellite_brightness %s - must be between 1 and 100\n",
						   sat_bright);
			(void) error_report(err_buf);
		}
		(void) glImageTypeSetBrightness(ImageTypeSatellite, brightness/100.0);
	}
	if (!blank(radar_bright))
	{
		brightness = atoi(radar_bright);
		if ( brightness < 1 || brightness > 100 )
		{
			(void) sprintf(err_buf,
						   "Error in radar_brightness %s - must be between 1 and 100\n",
						   radar_bright);
			(void) error_report(err_buf);
		}
		(void) glImageTypeSetBrightness(ImageTypeRadar, brightness/100.0);
	}

	/* Initialize Drawing Window */
	(void) glVirtualInit();
	view_window = glCreateVirtualWindow((int)(LRpoint[X] - ULpoint[X]),
										(int)(ULpoint[Y] - LRpoint[Y]));
	(void) glOrtho(0, (int)BaseMap.definition.xlen,
				   0, (int)BaseMap.definition.ylen);

	/* Identify and save images as satellite or radar */
	radar_list = INITMEM(Image, num_images);
	sat_list   = INITMEM(Image, num_images);
	(void) strcpy(radar_images, FpaCblank);
	(void) strcpy(sat_images,   FpaCblank);
	for ( num_radar=0, num_sat=0, nn=0; nn<num_images; nn++ )
	{

		/* Ignore missing image tags */
		if ( blank(image_info[nn].image_tag) ) continue;

		/* What kind of image are we dealing with? */
		image_type = glImageInfoGetType(image_info[nn].image_tag);

		/* Complain if image type cannot be handled */
		/* For now only radar or satellite allowed! */
		if (image_type != ImageTypeRadar && image_type != ImageTypeSatellite)
		{
			(void) sprintf(err_buf,
						   "Error in image type for %s - must be Satellite or Radar\n",
						   image_info[nn].image_tag);
			(void) error_report(err_buf);
		}

		/* Check valid times for each image */
		nvt = glImageInfoFindValidTimes(image_info[nn].image_tag,
				glSORT_ASCENDING, &vtimes, &ftimes);

		/* Set match times based on image type */
		switch(image_type)
		{
			case ImageTypeRadar:
				match_b = blank(radar_before) ? match_before : radar_before;
				match_a = blank(radar_after)  ? match_after  : radar_after;
				break;
			case ImageTypeSatellite:
				match_b = blank(sat_before) ? match_before : sat_before;
				match_a = blank(sat_after)  ? match_after  : sat_after;
				break;
		}

		/* Find the closest match time in the list
		 * Note that if no before/after times are given, the closest available
		 *  matching time is returned
		 */
		pos = closest_tstamp(nvt, vtimes, vtime, BaseMap.clon, match_b, match_a,
				NULL, NULL);

		if ( DebugMode )
			{
			pos = closest_tstamp(nvt, vtimes, vtime, BaseMap.clon,
					match_b, match_a, &nf, &nl);
			(void) fprintf(stdout,
						   "Closest tstamp: %d  from %d to %d (0 to %d)\n",
						   pos, nf, nl, nvt-1);
			}

		switch(pos)
		{
		case -1:
			if ( Verbose )
			{
				(void) fprintf(stdout,
							   "Problem with timestamps for %s\n",
							   image_info[nn].image_tag);
			}
			continue;
		case -2:
			if ( Verbose )
			{
				(void) fprintf(stdout,
							   "All timestamps are before range for %s\n",
							   image_info[nn].image_tag);
			}
			continue;
		case -3:
			if ( Verbose )
			{
				(void) fprintf(stdout,
							   "All timestamps are after range for %s\n",
							   image_info[nn].image_tag);
			}
			continue;
		case -4:
			if ( Verbose )
			{
				(void) fprintf(stdout,
							   "No timestamps found within range for %s\n",
							   image_info[nn].image_tag);
			}
			continue;
		default:
			/* value between 0 and nvt-1 is position of closest match */
			break;
		}

		if ( DebugMode )
			{
			(void) fprintf(stdout,
						   "Matching: %s  with: %s (%s)  from: %s to %s\n",
						   vtime, ftimes[pos], vtimes[pos],
						   vtimes[nf], vtimes[nl]);
			}

		/* Fetch the matching image */
		current_image = glImageFetch(image_info[nn].image_tag, ftimes[pos],
									 &BaseMap);

		/* No matching image */
		if ( current_image <= 0 ) continue;

		/* Separate Radar images and Satellite images into lists */
		switch(image_type)
		{
			case ImageTypeRadar:
				radar_list[num_radar++] = current_image;
				/* Add image name to output buffer */
				(void) strcat(radar_images, "  ");
				(void) strcat(radar_images, image_info[nn].image_tag);
				(void) strcat(radar_images, "@");
				(void) strcat(radar_images, ftimes[pos]);
				break;
			case ImageTypeSatellite:
				sat_list[num_sat++] = current_image;
				/* Add image name to output buffer */
				(void) strcat(sat_images, "  ");
				(void) strcat(sat_images, image_info[nn].image_tag);
				(void) strcat(sat_images, "@");
				(void) strcat(sat_images, ftimes[pos]);
				break;
		}

		/* Apply colour table to each image (if given) */
		if ( !blank(image_info[nn].ctable)
				&& !same_ic("default", image_info[nn].ctable) )
		{
			STRING ctable_path;

			/* Find and apply the colour table */
			ctable_path = parse_ctable_name(image_info[nn].ctable);
			if ( !blank(ctable_path) ) lut = glImageReadLUT(ctable_path);
			else                       lut = glNoLUT;
			if ( lut != glNoLUT )
			{
				if ( Verbose )
				{
					(void) fprintf(stdout,
							   "Applying colour table \"%s\" to %s\n",
							   image_info[nn].ctable, image_info[nn].image_tag);
				}
				glImageSetLUT(current_image, lut);
			}

			/* If we fail to find the colour table with the given name
			 *  use the default colour table
			 */
			else
			{
				(void) sprintf(err_buf,
							   "Unable to find colour table \"%s\" for %s\n",
							   image_info[nn].ctable, image_info[nn].image_tag);
				(void) warn_report(err_buf);
			}
		}

		/* Apply brightness to each image (if given) */
		if (!blank(image_info[nn].brightness))
		{
			brightness = atoi(image_info[nn].brightness);
			if ( brightness < 1 || brightness > 100 )
			{
				(void) sprintf(err_buf,
							   "Error in brightness %s for %s - must be between 1 and 100\n",
							   image_info[nn].brightness,
							   image_info[nn].image_tag);
				(void) error_report(err_buf);
			}
			(void) glImageTagSetBrightness(image_info[nn].image_tag,
										   brightness/100.0);
		}
	}

	/* Apply range/limit rings to all radar images (if requested) */
	if ( limit_ring )
	{
		/* Set range interval to zero (no range rings) to start */
		(void) glImageShowRadarRangeRings( 1 , 0 );

		/* Reset the radar limit ring colour (if requested) */
		if (!blank(limit_colour) && !same(ColourNone, limit_colour))
		{
			int    r0, r1, r2;
			UNCHAR rcolour[3];

			/* Parse radar limit ring colour */
			(void) sscanf(limit_colour, "%d:%d:%d", &r0, &r1, &r2);
			rcolour[0] = (UNCHAR) r0;
			rcolour[1] = (UNCHAR) r1;
			rcolour[2] = (UNCHAR) r2;

			/* Set radar limit ring colour */
			(void) glImageSetRadarRangeRingColor(NULL, rcolour);
		}
	}
	if ( range_rings )
	{

		/* Set range interval (including limit ring) */
		(void) glImageShowRadarRangeRings( 1 , NINT(range_interval));

		/* Reset the radar range ring colour (if requested) */
		if (!blank(range_colour) && !same(range_colour, ColourNone))
		{
			int    r0, r1, r2;
			UNCHAR rcolour[3];

			/* Parse radar range ring colour */
			(void) sscanf(range_colour, "%d:%d:%d", &r0, &r1, &r2);
			rcolour[0] = (UNCHAR) r0;
			rcolour[1] = (UNCHAR) r1;
			rcolour[2] = (UNCHAR) r2;

			/* Set radar range ring colour */
			(void) glImageSetRadarRangeRingColor(rcolour, NULL);
		}
	}

	/* Force all locations to be anchored to the current map */
	/*  ... regardless of the current anchor position!       */
	cur_anchor  = AnchorToMap;
	AnchorToMap = TRUE;

	(void) sprintf(out_buf, "### Begin display of images ...");
	(void) write_graphics_comment(out_buf);
	if ( !blank(radar_images) )
	{
		if ( Verbose )
		{
			(void) fprintf(stdout,
						   " Displaying radar images ... %s\n", radar_images);
		}
		(void) sprintf(out_buf, "###   Radar images ...%s", radar_images);
		(void) write_graphics_comment(out_buf);
	}
	if ( !blank(sat_images) )
	{
		if ( Verbose )
		{
			(void) fprintf(stdout,
						   " Displaying satellite images ... %s\n", sat_images);
		}
		(void) sprintf(out_buf, "###   Satellite images ...%s", sat_images);
		(void) write_graphics_comment(out_buf);
	}

	/* Combine all radar imagery (in reverse order given) */
	(void) reverse_image_list(radar_list, num_radar);
	radar_image = glImageCombine(0, &BaseMap, radar_list, num_radar);

	/* Combine all satellite imagery (in reverse order given) */
	(void) reverse_image_list(sat_list, num_sat);
	sat_image = glImageCombine(0, &BaseMap, sat_list, num_sat);

	/* Blend radar with satellite imagery (if requested) */
	if ( blend_images )
	{
		final_image = glImageBlend(0, &BaseMap,
								   sat_image, radar_image, NINT(blend_ratio));
	}

	/* Otherwise, just use blend ratio of 100 */
	else
	{
		final_image = glImageBlend(0, &BaseMap,
								   sat_image, radar_image, 100);
	}

	/* Display the combined and blended image */
	if (num_sat + num_radar > 0)
	{
		(void) write_graphics_image(final_image);
	}

	/* Clean up Drawing Window */
	glImageDestroyAll();
	glCloseWindow(view_window);
	glExit();

	/* Reset the current anchor */
	AnchorToMap = cur_anchor;
	(void) sprintf(out_buf, "### End display of images ...");
	(void) write_graphics_comment(out_buf);
	if ( !blank(radar_images) )
	{
		(void) sprintf(out_buf, "###   Radar images ...%s", radar_images);
		(void) write_graphics_comment(out_buf);
	}
	if ( !blank(sat_images) )
	{
		(void) sprintf(out_buf, "###   Satellite images ...%s", sat_images);
		(void) write_graphics_comment(out_buf);
	}

	/* Free space used by work objects and return TRUE */
    FREEMEM(radar_list);
    FREEMEM(sat_list);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    G R A _ d i s p l a y _ a l l _ l c h a i n _ n o d e s           *
*                                                                      *
***********************************************************************/

LOGICAL			GRA_display_all_lchain_nodes

	(
	GRA_FLD_INFO	*lchain_flds,	/* Fields for link chain nodes */
	int				num_fields,		/* Number of fields for link chain nodes */
	STRING			stime,			/* Start time for link chain nodes */
	STRING			etime,			/* End time for link chain nodes */
	STRING			*times,			/* Times for link chain nodes */
	int				num_times,		/* Number of times for link chain nodes */
	STRING			time_zone,		/* Time zone (used in strftime) */
	STRING			language,		/* Language (used in strftime) */
	STRING			nspeed_units,	/* Units for node speeds */
	float			nround,			/* Rounding for node speeds */
	float			nqsmax,			/* Maximum speed for quasi-stationary nodes */
	STRING			nstnry_label,	/* Label for quasi-stationary nodes */
	STRING			units,			/* Units for node wind display */
	STRING			cat_cascade,	/* And/Or for multiple CATATTRIB structs */
	CATATTRIB		*cat_attrib,	/* Structure containing categories */
	int				num_catatt,		/* Number of categories */
	STRING			tcat_cascade,	/* And/Or for multiple track CATATTRIB structs */
	CATATTRIB		*tcat_attrib,	/* Structure containing track categories */
	int				num_tcatatt,	/* Number of track categories */
	ATTRIB_DISPLAY	*attribs,		/* Attributes to display */
	int				num_attrib,		/* Number of attributes */
	SPCASE			*list_case,		/* Structure for special cases */
	int				num_list,		/* Number of special cases */
	STRING			inmark,			/* Name of location marker */
	float			markscale,		/* Marker scaling factor (percent) */
	STRING			display_name,	/* Name for label display */
	STRING			display_type,	/* Type of label display */
	LOGICAL			fit_to_map,		/* Fit display within map? */
	STRING			fit_to_map_ref,	/* Reference position for fit */
	float			rotation,		/* Label rotation (degrees) */
	LOGICAL			rotate_lat,		/* Rotate label to align with latitude? */
	LOGICAL			rotate_lon,		/* Rotate label to align with longitude? */
	STRING			rot_attrib,		/* Attribute to align label to */
	LOGICAL			constrain_rot,	/* Constrain rotation to display upright? */
	float			xshift,			/* x shift of label (off map) */
	float			yshift,			/* y shift of label (off map) */
	int				xwrap,			/* x count for list of labels (off map) */
	int				ywrap,			/* y count for list of labels (off map) */
	float			xdoff,			/* x offset of label and mark */
	float			ydoff,			/* y offset of label and mark */
	float			xoff,			/* x offset of label (display units) */
	float			yoff,			/* y offset of label (display units) */
	float			xqsoff,			/* x offset for quasi-stationary nodes */
	float			yqsoff			/* y offset for quasi-stationary nodes */
	)

	{
	STRING		stype;
	LOGICAL		status;
	char		all_fields[GPGLong];
	char		out_buf[GPGLong], err_buf[GPGLong];

	int			nn, ilist, iil, inode, itime, mplus;
	float		xxs, yys, xxo, yyo, xx, yy, clon, rspd;
	double		vspd, dspd;
	STRING		xtime, ttime, vtime, value;
	ATTRIB_LIST	xattrib;
	char		sspd[20];
	SET			set = NullSet;
	BOX			box;
	LCHAIN		lchain;
	LNODE		lnode;
	LINTERP		linterp;

	FLD_DESCRIPT	descript;

	/* Loop to check one (or more) fields for link chain nodes */
	(void) strcpy(all_fields, FpaCblank);
	for ( nn=0; nn<num_fields; nn++ )
		{

		/* Make a copy of the global field descriptor */
		(void) copy_fld_descript(&descript, &Fdesc);

		/* Re-initialize the field descriptor for this element and level */
		if ( !set_fld_descript(&descript, FpaF_DIRECTORY_PATH, FpaCblank,
									FpaF_SOURCE_NAME,   CurSource,
									FpaF_RUN_TIME,      FpaCblank,
									FpaF_ELEMENT_NAME,  lchain_flds[nn].element,
									FpaF_LEVEL_NAME,    lchain_flds[nn].level,
									FpaF_VALID_TIME,    TVstamp,
									FpaF_END_OF_LIST) )
			{
			(void) sprintf(err_buf,
					" Error setting field descriptor for ... %s %s from %s at %s\n",
					lchain_flds[nn].element, lchain_flds[nn].level,
					CurSource, TVstamp);
			(void) error_report(err_buf);
			}

		/* Add field name to output buffer */
		(void) strcat(all_fields, " ");
		(void) strcat(all_fields, lchain_flds[nn].element);
		(void) strcat(all_fields, " ");
		(void) strcat(all_fields, lchain_flds[nn].level);
		(void) strcat(all_fields, " ");
		if ( Verbose )
			{
			(void) fprintf(stdout,
					" Retrieving link chains for field ... %s %s from %s at %s\n",
					lchain_flds[nn].element, lchain_flds[nn].level,
					CurSource, TVstamp);
			}

		/* Retrieve the link chains */
		set = append_set(set, retrieve_lchainset(&descript));
		}

	/* Return now if no link chains found */
	if ( IsNull(set) )
		{
		if ( Verbose )
			{
			(void) fprintf(stdout,
					" No link chains for field(s) ... %s from %s at %s\n",
					all_fields, CurSource, TVstamp);
			}
		return TRUE;
		}
	(void) recall_set_type(set, &stype);
	if ( set->num <= 0 || !same(stype, "lchain") )
		{
		if ( Verbose )
			{
			(void) fprintf(stdout,
					" No link chains for field(s) ... %s from %s at %s\n",
					all_fields, CurSource, TVstamp);
			}
		(void) destroy_set(set);
		return TRUE;
		}

	/* Ensure that all link chains in the list have been interpolated */
	for ( ilist=0; ilist<set->num; ilist++ )
		{

		/* Interpolate each link chain in the list */
		lchain = (LCHAIN) set->list[ilist];
		if ( IsNull(lchain) ) continue;
		if ( lchain->dointerp ) (void) interpolate_lchain(lchain);
		}

	/* Strip out all link chains that fall outside the display map */
	box.left   = 0.0;
	box.right  = BaseMap.definition.xlen;
	box.bottom = 0.0;
	box.top    = BaseMap.definition.ylen;
	(void) strip_set(set, &box);

	/* Return now if no remaining link chains */
	if ( set->num <= 0 )
		{
		if ( Verbose )
			{
			(void) fprintf(stdout, " No link chains within map");
			(void) fprintf(stdout, " for field(s) ... %s from %s at %s\n",
					all_fields, CurSource, TVstamp);
			}
		(void) destroy_set(set);
		return TRUE;
		}

	/* Determine centre longitude for current map */
	(void) grid_center(&BaseMap, NullPointPtr, NullFloat, &clon);

	/* Initialize offsets for lists of link chain nodes */
	iil = 0;
	xxs = 0.0;
	yys = 0.0;

	/* Now display link chain nodes that match the required categories  */
	/*  from all link chains in the set                                 */
	/* Note that this is done in reverse order, to maintain precedence! */
	(void) sprintf(out_buf,
			"### Begin link chain nodes for field(s) ... %s", all_fields);
	(void) write_graphics_comment(out_buf);
	if ( Verbose )
		{
		(void) fprintf(stdout,
				" Displaying link chain nodes for field(s) ... %s from %s at %s\n",
				all_fields, CurSource, TVstamp);
		}
	for ( ilist=set->num-1; ilist>=0; ilist-- )
		{

		/* Define each link chain in the list */
		lchain = (LCHAIN) set->list[ilist];
		if ( IsNull(lchain) ) continue;

		/* Set the link chain reference time */
		xtime = lchain->xtime;

		/* Check for link chain tracks that match desired categories */
		/* Note that each category may contain more than one member! */
		if ( !match_category_attributes(lchain->attrib, tcat_cascade,
				tcat_attrib, num_tcatatt) ) continue;

		/* First display the interpolated nodes */
		for ( inode=0; inode<lchain->inum; inode++ )
			{

			/* Define each interpolated node */
			linterp = lchain->interps[inode];
			if ( !linterp->there ) continue;

			/* Check interpolated node time against list of times (if required) */
			if ( num_times > 0)
				{

				/* Check each time in list */
				for ( itime=0; itime<num_times; itime++ )
					{
					ttime = interpret_timestring(times[itime], xtime, clon);
					if ( !blank(ttime) )
						{
						mplus = calc_prog_time_minutes(xtime, ttime, &status);
						if ( mplus == linterp->mplus ) break;
						}
					}

				/* Continue if no match found */
				if ( itime >= num_times )
					{
					if ( Verbose )
						{
						(void) fprintf(stdout,
							" Interpolated node at ... %d  does not match times ...",
							linterp->mplus);
						for ( itime=0; itime<num_times; itime++ )
							(void) fprintf(stdout," %s", times[itime]);
						(void) fprintf(stdout,"\n");
						}
					continue;
					}
				}

			/* Check interpolated node time against a start time (if required) */
			if ( !blank(stime) )
				{
				ttime = interpret_timestring(stime, xtime, clon);
				if ( !blank(ttime) )
					{
					mplus = calc_prog_time_minutes(xtime, ttime, &status);
					if ( linterp->mplus < mplus )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								" Interpolated node at ... %d  before start time ... %d (%s)\n",
								linterp->mplus, mplus, stime);
							}
						continue;
						}
					}
				}

			/* Check interpolated node time against an end time (if required) */
			if ( !blank(etime) )
				{
				ttime = interpret_timestring(etime, xtime, clon);
				if ( !blank(ttime) )
					{
					mplus = calc_prog_time_minutes(xtime, ttime, &status);
					if ( linterp->mplus > mplus )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								" Interpolated node at ... %d  after end time ... %d (%s)\n",
								linterp->mplus, mplus, etime);
							}
						continue;
						}
					}
				}

			/* Check the interpolated node location */
			if ( linterp->node[X] < 0.0 || linterp->node[Y] < 0.0
					|| linterp->node[X] > BaseMap.definition.xlen
					|| linterp->node[Y] > BaseMap.definition.ylen )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Interpolated node off map at ... %.0f %.0f\n",
							linterp->node[X], linterp->node[Y]);
					}

				/* Continue (if requested) for map labels off map */
				if ( AnchorToMap ) continue;
				else if ( fit_to_map ) continue;
				}
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout, "  Interpolated node at ... %.0f %.0f\n",
							linterp->node[X], linterp->node[Y]);
					}
				}

			/* Check for interpolated nodes that match desired categories */
			/* Note that each category may contain more than one member!  */
			if ( !match_category_attributes(linterp->attrib, cat_cascade,
					cat_attrib, num_catatt) ) continue;

			/* Set interpolated node location on the current map */
			if ( AnchorToMap )
				{
				(void) anchored_location(linterp->node, 0.0, 0.0, &xxo, &yyo);
				}

			/* Set interpolated node location to current anchor position     */
			/*  and set offset for each successive interpolated node in list */
			else
				{
				(void) anchored_location(ZeroPoint, xxs, yys, &xxo, &yyo);
				iil++;
				if ( xwrap > 1 && iil % xwrap == 0 )
					{
					xxs  = 0.0;
					yys += yshift;
					}
				else if ( xwrap > 1 )
					{
					xxs += xshift;
					}
				else if ( ywrap > 1 && iil % ywrap == 0 )
					{
					xxs += xshift;
					yys  = 0.0;
					}
				else if ( ywrap > 1 )
					{
					yys += yshift;
					}
				else
					{
					xxs += xshift;
					yys += yshift;
					}
				}

			/* Set interpolated node location (offset by xdoff/ydoff) */
			/*  and offset location for interpolated node attributes  */
			xxo += xdoff;
			yyo += ydoff;
			xx = xxo + xoff;
			yy = yyo + yoff;

			/* Make a copy of the interpolated node attributes */
			xattrib = copy_attrib_list(linterp->attrib);
			
			/* Set the valid time for the interpolated node */
			vtime = calc_valid_time_minutes(xtime, 0, linterp->mplus);

			/* Add the node motion at this interpolated node to the attributes */
			CAL_add_lchain_node_motion(xattrib, &BaseMap, lchain, linterp->mplus);

			if ( Verbose )
				{
				(void) fprintf(stdout,
						"  Interpolated node time ... %s\n", vtime);
				value = CAL_get_attribute(xattrib, AttribLnodeDirection);
				if ( !blank(value) )
					(void) fprintf(stdout,
							"  Interpolated node direction ... %s\n", value);
				value = CAL_get_attribute(xattrib, AttribLnodeSpeed);
				if ( !blank(value) )
					(void) fprintf(stdout,
							"  Interpolated node speed ... %s MKS\n", value);
				}

			/* Convert the node speed in the attribute list (if required) */
			value = CAL_get_attribute(xattrib, AttribLnodeSpeed);
			if ( !blank(value) )
				{

				/* Set the node speed units */
				vspd = atof(value);
				(void) convert_value(FpaCmksUnits, vspd, nspeed_units, &dspd);

				/* Check for quasi-stationary nodes */
				if ( dspd < nqsmax )
					{

					/* Reset node label and offset for quasi-stationary nodes */
					CAL_add_attribute(xattrib, AttribLnodeSpeed, nstnry_label);
					xx = xxo + xqsoff;
					yy = yyo + yqsoff;

					if ( Verbose )
						{
						(void) fprintf(stdout,
								"  Interpolated node speed (quasi-stationary) ... %s\n",
								nstnry_label);
						}
					}

				/* Otherwise, reformat the node speed */
				else
					{

					/* Round the node speed */
					if ( nround > 0.0 )
						rspd = NINT((float) dspd / nround) * nround;
					else
						rspd = (float) dspd;

					/* Add the node speed back to the attribute list */
					/*  ... formatted based on the round off!        */
					if ( nround >= 1.0 )
						(void) sprintf(sspd, "%.0f", rspd);
					else if ( nround >= 0.1 )
						(void) sprintf(sspd, "%.1f", rspd);
					else
						(void) sprintf(sspd, "%.2f", rspd);
					CAL_add_attribute(xattrib, AttribLnodeSpeed, sspd);

					if ( Verbose )
						{
						(void) fprintf(stdout,
								"  Interpolated node speed (reformat) ... %s %s (rounded by %.2f)\n",
								sspd, nspeed_units, nround);
						}
					}
				}

			/* Add link chain attributes (not already in link node) */
			CAL_merge(xattrib, lchain->attrib, FALSE);

			/* Display the interpolated node containing all attributes */
			(void) GRA_display_sampled_attributes(FpaCblank, vtime, time_zone,
					language, FpaCblank, linterp->node, DefProximity, DefBearing,
					DefLineLen, DefSegDir, DefSegSpd,
					xattrib, cat_cascade, cat_attrib, num_catatt,
					attribs, num_attrib, list_case, num_list,
					inmark, markscale, display_name, display_type, fit_to_map,
					fit_to_map_ref, rotation, rotate_lat, rotate_lon,
					rot_attrib, constrain_rot, xxo, yyo, xx, yy);

			/* Destroy the copy of the interpolated node attributes */
			xattrib = destroy_attrib_list(xattrib);
			}

		/* Next display the link nodes */
		for ( inode=0; inode<lchain->lnum; inode++ )
			{

			/* Define each link node */
			lnode = lchain->nodes[inode];
			if ( !lnode->there ) continue;

			/* Check the link node time against list of times (if required) */
			if ( num_times > 0)
				{

				/* Check each time in list */
				for ( itime=0; itime<num_times; itime++ )
					{
					ttime = interpret_timestring(times[itime], xtime, clon);
					if ( !blank(ttime) )
						{
						mplus = calc_prog_time_minutes(xtime, ttime, &status);
						if ( mplus == lnode->mplus ) break;
						}
					}

				/* Continue if no match found */
				if ( itime >= num_times )
					{
					if ( Verbose )
						{
						(void) fprintf(stdout,
							" Link node at ... %d  does not match times ...",
							lnode->mplus);
						for ( itime=0; itime<num_times; itime++ )
							(void) fprintf(stdout," %s", times[itime]);
						(void) fprintf(stdout,"\n");
						}
					continue;
					}
				}

			/* Check the link node time against a start time (if required) */
			if ( !blank(stime) )
				{
				ttime = interpret_timestring(stime, xtime, clon);
				if ( !blank(ttime) )
					{
					mplus = calc_prog_time_minutes(xtime, ttime, &status);
					if ( lnode->mplus < mplus )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								" Link node at ... %d  before start time ... %d (%s)\n",
								lnode->mplus, mplus, stime);
							}
						continue;
						}
					}
				}

			/* Check the link node time against an end time (if required) */
			if ( !blank(etime) )
				{
				ttime = interpret_timestring(etime, xtime, clon);
				if ( !blank(ttime) )
					{
					mplus = calc_prog_time_minutes(xtime, ttime, &status);
					if ( lnode->mplus > mplus )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								" Link node at ... %d  after end time ... %d (%s)\n",
								lnode->mplus, mplus, etime);
							}
						continue;
						}
					}
				}

			/* Check the link node location */
			if ( lnode->node[X] < 0.0 || lnode->node[Y] < 0.0
					|| lnode->node[X] > BaseMap.definition.xlen
					|| lnode->node[Y] > BaseMap.definition.ylen )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Link node off map at ... %.0f %.0f\n",
							lnode->node[X], lnode->node[Y]);
					}

				/* Continue (if requested) for map labels off map */
				if ( AnchorToMap ) continue;
				else if ( fit_to_map ) continue;
				}
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout, "  Link node at ... %.0f %.0f\n",
							lnode->node[X], lnode->node[Y]);
					}
				}

			/* Check for link nodes that match desired categories        */
			/* Note that each category may contain more than one member! */
			if ( !match_category_attributes(lnode->attrib, cat_cascade,
					cat_attrib, num_catatt) ) continue;

			/* Set link node location on the current map */
			if ( AnchorToMap )
				{
				(void) anchored_location(lnode->node, 0.0, 0.0, &xxo, &yyo);
				}

			/* Set link node location to current anchor position     */
			/*  and set offset for each successive link node in list */
			else
				{
				(void) anchored_location(ZeroPoint, xxs, yys, &xxo, &yyo);
				iil++;
				if ( xwrap > 1 && iil % xwrap == 0 )
					{
					xxs  = 0.0;
					yys += yshift;
					}
				else if ( xwrap > 1 )
					{
					xxs += xshift;
					}
				else if ( ywrap > 1 && iil % ywrap == 0 )
					{
					xxs += xshift;
					yys  = 0.0;
					}
				else if ( ywrap > 1 )
					{
					yys += yshift;
					}
				else
					{
					xxs += xshift;
					yys += yshift;
					}
				}

			/* Set link node location (offset by xdoff/ydoff) */
			/*  and offset location for link node attributes  */
			xxo += xdoff;
			yyo += ydoff;
			xx = xxo + xoff;
			yy = yyo + yoff;

			/* Make a copy of the link node attributes */
			xattrib = copy_attrib_list(lnode->attrib);
			
			/* Set the valid time for the interpolated node */
			vtime = calc_valid_time_minutes(xtime, 0, lnode->mplus);

			/* Add the node motion at this link node to the attributes */
			CAL_add_lchain_node_motion(xattrib, &BaseMap, lchain, lnode->mplus);

			if ( Verbose )
				{
				value = CAL_get_attribute(xattrib, AttribLnodeDirection);
				if ( !blank(value) )
					(void) fprintf(stdout,
							"  Link node direction ... %s\n", value);
				value = CAL_get_attribute(xattrib, AttribLnodeSpeed);
				if ( !blank(value) )
					(void) fprintf(stdout,
							"  Link node speed ... %s MKS\n", value);
				}

			/* Convert the node speed in the attribute list (if required) */
			value = CAL_get_attribute(xattrib, AttribLnodeSpeed);
			if ( !blank(value) )
				{

				/* Set the node speed units */
				vspd = atof(value);
				(void) convert_value(FpaCmksUnits, vspd, nspeed_units, &dspd);

				/* Check for quasi-stationary nodes */
				if ( dspd < nqsmax )
					{

					/* Reset node label and offset for quasi-stationary nodes */
					CAL_add_attribute(xattrib, AttribLnodeSpeed, nstnry_label);
					xx = xxo + xqsoff;
					yy = yyo + yqsoff;

					if ( Verbose )
						{
						(void) fprintf(stdout,
								"  Link node speed (quasi-stationary) ... %s\n",
								nstnry_label);
						}
					}

				/* Otherwise, reformat the node speed */
				else
					{

					/* Round the node speed */
					if ( nround > 0.0 )
						rspd = NINT((float) dspd / nround) * nround;
					else
						rspd = (float) dspd;

					/* Add the node speed back to the attribute list */
					/*  ... formatted based on the round off!        */
					if ( nround >= 1.0 )
						(void) sprintf(sspd, "%.0f", rspd);
					else if ( nround >= 0.1 )
						(void) sprintf(sspd, "%.1f", rspd);
					else
						(void) sprintf(sspd, "%.2f", rspd);
					CAL_add_attribute(xattrib, AttribLnodeSpeed, sspd);

					if ( Verbose )
						{
						(void) fprintf(stdout,
								"  Link node speed (reformat) ... %s %s (rounded by %.2f)\n",
								sspd, nspeed_units, nround);
						}
					}
				}

			/* Add link chain attributes (not already in link node) */
			CAL_merge(xattrib, lchain->attrib, FALSE);

			/* Display the link node containing all attributes */
			(void) GRA_display_sampled_attributes(FpaCblank, vtime, time_zone,
					language, FpaCblank, lnode->node, DefProximity, DefBearing,
					DefLineLen, DefSegDir, DefSegSpd,
					xattrib, cat_cascade, cat_attrib, num_catatt,
					attribs, num_attrib, list_case, num_list,
					inmark, markscale, display_name, display_type, fit_to_map,
					fit_to_map_ref, rotation, rotate_lat, rotate_lon,
					rot_attrib, constrain_rot, xxo, yyo, xx, yy);

			/* Destroy the copy of the link node attributes */
			xattrib = destroy_attrib_list(xattrib);
			}
		}

	(void) sprintf(out_buf,
			"### End link chain nodes for field(s) ... %s", all_fields);
	(void) write_graphics_comment(out_buf);

	/* Free space used by work objects and return TRUE */
	(void) destroy_set(set);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    G R A _ d i s p l a y _ a l l _ l c h a i n _ t r a c k s         *
*                                                                      *
***********************************************************************/

LOGICAL			GRA_display_all_lchain_tracks

	(
	GRA_FLD_INFO	*lchain_flds,	/* Fields for link chains */
	int				num_fields,		/* Number of fields for link chains */
	STRING			stime,			/* Start time for link chains */
	STRING			etime,			/* End time for link chains */
	float			tlmin,			/* Minimum track length for display */
	STRING			tlen_units,		/* Units for minimum track length */
	STRING			cat_cascade,	/* And/Or for multiple CATATTRIB structs */
	CATATTRIB		*cat_attrib,	/* Structure containing categories */
	int				num_catatt,		/* Number of categories */
	STRING			attribute,		/* Attribute containing value to match */
	STRING			look_up,		/* Look up table for presentation */
	STRING			arrow_name,		/* Arrow display for link chains */
	STRING			pattern,		/* Name of pattern for link chains */
	STRING			pattern_width,	/* Pattern width factor */
	STRING			pattern_length,	/* Pattern repetition factor */
	COMP_PRES		*comp_pres,		/* Structure containing presentations */
	int				num_comp		/* Number of presentations */
	)

	{
	STRING		stype, value, pname;
	LOGICAL		cur_anchor, clip_to_map, status;
	PRES		cur_pres, named_pres;
	char		all_fields[GPGLong];
	char		out_buf[GPGLong], err_buf[GPGLong];

	int			nn, num_tracks, ilist, itrack;
	int			mplus, ibgn, iend;
	float		clon, flen;
	double		dlen;
	STRING		xtime, vtime;
	SET			set = NullSet;
	BOX			box;
	LCHAIN		lchain;
	LINE		xtrack;
	LINE		*tracks = NullLineList;
	STRING		*values = NullStringList;

	FLD_DESCRIPT	descript;

	/* Loop to check one (or more) fields for link chains */
	(void) strcpy(all_fields, FpaCblank);
	for ( nn=0; nn<num_fields; nn++ )
		{

		/* Make a copy of the global field descriptor */
		(void) copy_fld_descript(&descript, &Fdesc);

		/* Re-initialize the field descriptor for this element and level */
		if ( !set_fld_descript(&descript, FpaF_DIRECTORY_PATH, FpaCblank,
									FpaF_SOURCE_NAME,   CurSource,
									FpaF_RUN_TIME,      FpaCblank,
									FpaF_ELEMENT_NAME,  lchain_flds[nn].element,
									FpaF_LEVEL_NAME,    lchain_flds[nn].level,
									FpaF_VALID_TIME,    TVstamp,
									FpaF_END_OF_LIST) )
			{
			(void) sprintf(err_buf,
					" Error setting field descriptor for ... %s %s from %s at %s\n",
					lchain_flds[nn].element, lchain_flds[nn].level,
					CurSource, TVstamp);
			(void) error_report(err_buf);
			}

		/* Add field name to output buffer */
		(void) strcat(all_fields, " ");
		(void) strcat(all_fields, lchain_flds[nn].element);
		(void) strcat(all_fields, " ");
		(void) strcat(all_fields, lchain_flds[nn].level);
		(void) strcat(all_fields, " ");
		if ( Verbose )
			{
			(void) fprintf(stdout,
					" Retrieving link chains for field ... %s %s from %s at %s\n",
					lchain_flds[nn].element, lchain_flds[nn].level,
					CurSource, TVstamp);
			}

		/* Retrieve the link chains */
		set = append_set(set, retrieve_lchainset(&descript));
		}

	/* Return now if no link chains found */
	if ( IsNull(set) )
		{
		if ( Verbose )
			{
			(void) fprintf(stdout,
					" No link chains for field(s) ... %s from %s at %s\n",
					all_fields, CurSource, TVstamp);
			}
		return TRUE;
		}
	(void) recall_set_type(set, &stype);
	if ( set->num <= 0 || !same(stype, "lchain") )
		{
		if ( Verbose )
			{
			(void) fprintf(stdout,
					" No link chains for field(s) ... %s from %s at %s\n",
					all_fields, CurSource, TVstamp);
			}
		(void) destroy_set(set);
		return TRUE;
		}

	/* Ensure that all link chains in the list have been interpolated */
	for ( ilist=0; ilist<set->num; ilist++ )
		{

		/* Interpolate each link chain in the list */
		lchain = (LCHAIN) set->list[ilist];
		if ( IsNull(lchain) ) continue;
		if ( lchain->dointerp ) (void) interpolate_lchain(lchain);
		}

	/* Strip out all link chains that fall outside the display map */
	box.left   = 0.0;
	box.right  = BaseMap.definition.xlen;
	box.bottom = 0.0;
	box.top    = BaseMap.definition.ylen;
	(void) strip_set(set, &box);

	/* Return now if no remaining link chains */
	if ( set->num <= 0 )
		{
		if ( Verbose )
			{
			(void) fprintf(stdout, " No link chains within map");
			(void) fprintf(stdout, " for field(s) ... %s from %s at %s\n",
					all_fields, CurSource, TVstamp);
			}
		(void) destroy_set(set);
		return TRUE;
		}

	/* Determine centre longitude for current map */
	(void) grid_center(&BaseMap, NullPointPtr, NullFloat, &clon);

	/* Now create a list of tracks from the link chain objects that     */
	/*  match the required categories                                   */
	/* Note that this is done in reverse order, to maintain precedence! */
	for ( num_tracks=0, ilist=set->num-1; ilist>=0; ilist-- )
		{

		/* Define each link chain in the list */
		lchain = (LCHAIN) set->list[ilist];
		if ( IsNull(lchain) ) continue;
		if ( IsNull(lchain->track) ) continue;
		if ( lchain->track->numpts < 2 ) continue;

		/* Match the link chains to the required categories */
		if ( match_category_attributes(lchain->attrib,
										cat_cascade, cat_attrib, num_catatt) )
			{

			/* Set start node for link chain track from start time */
			ibgn = 0;
			if ( !blank(stime) )
				{
				xtime = lchain->xtime;
				vtime = interpret_timestring(stime, xtime, clon);
				if ( !blank(vtime) )
					{
					mplus = calc_prog_time_minutes(xtime, vtime, &status);
					if ( mplus >= lchain->splus )
						{
						ibgn = which_lchain_node(lchain, LchainInterp, mplus);
						}
					}
				}

			/* Set end node for link chain track from end time */
			iend = lchain->track->numpts - 1;
			if ( !blank(etime) )
				{
				xtime = lchain->xtime;
				vtime = interpret_timestring(etime, xtime, clon);
				if ( !blank(vtime) )
					{
					mplus = calc_prog_time_minutes(xtime, vtime, &status);
					if ( mplus <= lchain->eplus )
						{
						iend = which_lchain_node(lchain, LchainInterp, mplus);
						}
					}
				}

			/* Continue if no track remains */
			if ( iend <= ibgn ) continue;

			/* Check for minimum track length (if required) */
			if ( tlmin > 0.0 )
				{

				/* Determine great circle length of time-limited track */
				xtrack = append_line_portion(NullLine, lchain->track, ibgn, iend);
				flen   = great_circle_line_length(&BaseMap, xtrack);
				destroy_line(xtrack);
				(void) convert_value(FpaCmksUnits, (double) flen,
						tlen_units, &dlen);

				if ( Verbose )
					{
					if ( dlen < tlmin )
						(void) fprintf(stdout,
								" Link chain track length ... %.2f %s (minimum %.2f)\n",
								dlen, tlen_units, tlmin);
					else
						(void) fprintf(stdout,
								" Link chain track length ... %.2f %s\n",
								dlen, tlen_units);
					}

				/* Do not display if track length is too short */
				if ( dlen < tlmin ) continue;
				}

			/* Allocate space in the list */
			num_tracks++;
			tracks = GETMEM(tracks, LINE,   num_tracks);
			values = GETMEM(values, STRING, num_tracks);

			/* Add the link chain track to the list */
			tracks[num_tracks-1] = append_line_portion(NullLine, lchain->track,
																	ibgn, iend);

			/* Add the attribute value to the list */
			value = CAL_get_attribute(lchain->attrib, attribute);
			values[num_tracks-1] = strdup(SafeStr(value));
			}
		}

	/* Return now if no link chain tracks created */
	if ( num_tracks <= 0 )
		{
		if ( Verbose )
			{
			(void) fprintf(stdout,
					" No link chain tracks for field(s) ... %s from %s at %s\n",
					all_fields, CurSource, TVstamp);
			}
		(void) destroy_set(set);
		return TRUE;
		}

	/* Force all locations to be anchored to the current map */
	/*  ... regardless of the current anchor position!       */
	cur_anchor  = AnchorToMap;
	clip_to_map = TRUE;
	AnchorToMap = TRUE;
	(void) sprintf(out_buf,
			"### Begin link chain tracks for field(s) ... %s", all_fields);
	(void) write_graphics_comment(out_buf);
	if ( Verbose )
		{
		(void) fprintf(stdout,
				" Drawing link chain tracks for field(s) ... %s from %s at %s\n",
				all_fields, CurSource, TVstamp);
		}

	/* Now loop to output each link chain track */
	for ( itrack=0; itrack<num_tracks; itrack++ )
		{

		/* Set the current presentation parameters */
		(void) copy_presentation(&cur_pres, &CurPres);

		/* Reset presentation parameters from look up table (if required) */
		if ( !blank(look_up) )
			{

			/* Find presentation for value matched in look up table */
			if ( match_category_lookup(look_up, values[itrack],
					NullStringPtr, NullStringPtr, &pname) )
				{

				if ( Verbose )
					{
					(void) fprintf(stdout,
							"   Matching presentation for ... \"%s\"",
							values[itrack]);
					(void) fprintf(stdout, "  from look up ... %s\n", look_up);
					}

				/* Match presentation from list of named presentations */
				named_pres = get_presentation(pname);
				(void) copy_presentation(&cur_pres, &named_pres);
				}

			/* Warning if value cannot be matched */
			else
				{
				(void) fprintf(stdout, "   Presentation for ... \"%s\"",
						values[itrack]);
				(void) fprintf(stdout, "  not found in look up ... %s\n",
						look_up);
				}
			}

		/* Override pattern and pattern sizes (if requested) */
		if ( !blank(pattern) )
						(void) strcpy(cur_pres.pattern,        pattern);
		if ( !blank(pattern_width) )
						(void) strcpy(cur_pres.pattern_width,  pattern_width);
		if ( !blank(pattern_length) )
						(void) strcpy(cur_pres.pattern_length, pattern_length);

		/* Override component presentation (if requested) */
		if ( num_comp > 0 )
			{
			cur_pres.num_comp  = num_comp;
			cur_pres.comp_pres = comp_pres;
			(void) reset_presentation_by_comp_pres(&cur_pres,
													&cur_pres.comp_pres[0]);
			}

		/* Start of link chain track grouping */
		(void) sprintf(out_buf,
				"### Begin link chain track for ... %s", values[itrack]);
		(void) write_graphics_comment(out_buf);
		(void) write_graphics_group(GPGstart, NullPointer, 0);

		/* Display a "simple" link chain track */
		if ( blank(cur_pres.pattern) || same(cur_pres.pattern, PatternSimple) )
			{

			/* Display the link chain track */
			if ( !GRA_display_simple_line(tracks[itrack], 0.0, 0.0,
					cur_pres, clip_to_map) )
				{
				(void) error_report("Error drawing simple line");
				}
			}

		/* Display a link chain track using the given pattern */
		else
			{

			/* Display the link chain track */
			if ( !GRA_display_patterned_line(tracks[itrack], 0.0, 0.0,
					cur_pres, clip_to_map, Right) )
				{
				(void) error_report("Error drawing patterned line");
				}
			}

		/* Now display the arrow (if required) */
		if ( !blank(arrow_name) )
			{

			/* Set presentation from last component presentation */
			/*  (if available)                                   */
			if ( cur_pres.num_comp > 0 )
				{
				(void) reset_presentation_by_comp_pres(&cur_pres,
						&cur_pres.comp_pres[cur_pres.num_comp-1]);
				}

			/* Display the arrow */
			if ( !GRA_display_arrow(arrow_name, tracks[itrack], 0.0, 0.0,
					cur_pres, clip_to_map) )
				{
				(void) error_report("Error drawing arrow");
				}
			}

		/* End link chain track grouping */
		(void) write_graphics_group(GPGend, NullPointer, 0);
		(void) sprintf(out_buf,
				"### End link chain track for ... %s", values[itrack]);
		(void) write_graphics_comment(out_buf);
		}

	/* Reset the current anchor */
	AnchorToMap = cur_anchor;
	(void) sprintf(out_buf,
			"### End link chain tracks for field(s) ... %s", all_fields);
	(void) write_graphics_comment(out_buf);

	/* Free space used by work objects and return TRUE */
	for ( itrack=0; itrack<num_tracks; itrack++ )
		{
		(void) destroy_line(tracks[itrack]);
		}
	FREEMEM(tracks);
	FREELIST(values, num_tracks);
	(void) destroy_set(set);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    G R A _ d i s p l a y _ a l l _ l i n e s                         *
*                                                                      *
***********************************************************************/

LOGICAL			GRA_display_all_lines

	(
	GRA_FLD_INFO	*line_flds,		/* Fields for lines */
	int				num_fields,		/* Number of fields for lines */
	STRING			cat_cascade,	/* And/Or for multiple CATATTRIB structs */
	CATATTRIB		*cat_attrib,	/* Structure containing categories */
	int				num_catatt,		/* Number of categories */
	STRING			attribute,		/* Attribute containing value to match */
	STRING			look_up,		/* Look up table for presentation */
	STRING			arrow_name,		/* Arrow display for lines */
	STRING			pattern,		/* Name of pattern for lines */
	STRING			pattern_width,	/* Pattern width factor */
	STRING			pattern_length,	/* Pattern repetition factor */
	COMP_PRES		*comp_pres,		/* Structure containing presentations */
	int				num_comp		/* Number of presentations */
	)

	{
	STRING		stype, value, pname;
	LOGICAL		cur_anchor, clip_to_map;
	PRES		cur_pres, named_pres;
	char		all_fields[GPGLong];
	char		out_buf[GPGLong], err_buf[GPGLong];

	int			nn, num_lines, ilist, iline;
	SET			set = NullSet;
	BOX			box;

	CURVE		curve;
	LINE		*lines  = NullLineList;
	HAND		*sense  = NullPtr(HAND *);
	STRING		*values = NullStringList;

	FLD_DESCRIPT	descript;

	/* Loop to check one (or more) fields for lines */
	(void) strcpy(all_fields, FpaCblank);
	for ( nn=0; nn<num_fields; nn++ )
		{

		/* Make a copy of the global field descriptor */
		(void) copy_fld_descript(&descript, &Fdesc);

		/* Re-initialize the field descriptor for this element and level */
		if ( !set_fld_descript(&descript, FpaF_DIRECTORY_PATH, FpaCblank,
									FpaF_SOURCE_NAME,   CurSource,
									FpaF_RUN_TIME,      FpaCblank,
									FpaF_ELEMENT_NAME,  line_flds[nn].element,
									FpaF_LEVEL_NAME,    line_flds[nn].level,
									FpaF_VALID_TIME,    TVstamp,
									FpaF_END_OF_LIST) )
			{
			(void) sprintf(err_buf,
					" Error setting field descriptor for ... %s %s from %s at %s\n",
					line_flds[nn].element, line_flds[nn].level,
					CurSource, TVstamp);
			(void) error_report(err_buf);
			}

		/* Add field name to output buffer */
		(void) strcat(all_fields, " ");
		(void) strcat(all_fields, line_flds[nn].element);
		(void) strcat(all_fields, " ");
		(void) strcat(all_fields, line_flds[nn].level);
		(void) strcat(all_fields, " ");
		if ( Verbose )
			{
			(void) fprintf(stdout,
					" Retrieving lines for field ... %s %s from %s at %s\n",
					line_flds[nn].element, line_flds[nn].level,
					CurSource, TVstamp);
			}

		/* Retrieve the lines */
		set = append_set(set, retrieve_curveset(&descript));
		}

	/* Return now if no lines found */
	if ( IsNull(set) )
		{
		if ( Verbose )
			{
			(void) fprintf(stdout,
					" No lines for field(s) ... %s from %s at %s\n",
					all_fields, CurSource, TVstamp);
			}
		return TRUE;
		}
	(void) recall_set_type(set, &stype);
	if ( set->num <= 0 || !same(stype, "curve") )
		{
		if ( Verbose )
			{
			(void) fprintf(stdout,
					" No lines for field(s) ... %s from %s at %s\n",
					all_fields, CurSource, TVstamp);
			}
		(void) destroy_set(set);
		return TRUE;
		}

	/* Strip out all lines that fall outside the display map */
	box.left   = 0.0;
	box.right  = BaseMap.definition.xlen;
	box.bottom = 0.0;
	box.top    = BaseMap.definition.ylen;
	(void) strip_set(set, &box);

	/* Return now if no remaining lines */
	if ( set->num <= 0 )
		{
		if ( Verbose )
			{
			(void) fprintf(stdout, " No lines within map");
			(void) fprintf(stdout, " for field(s) ... %s from %s at %s\n",
					all_fields, CurSource, TVstamp);
			}
		(void) destroy_set(set);
		return TRUE;
		}

	/* Now create a list of LINE objects that match the required categories */
	/* Note that this is done in reverse order, to maintain precedence!     */
	for ( num_lines=0, ilist=set->num-1; ilist>=0; ilist-- )
		{

		/* Define each curve in the list */
		curve = (CURVE) set->list[ilist];
		if ( IsNull(curve) ) continue;
		if ( IsNull(curve->line) ) continue;
		if ( curve->line->numpts < 2 ) continue;

		/* Create lines from curves that match the required categories */
		if ( match_category_attributes(curve->attrib,
										cat_cascade, cat_attrib, num_catatt) )
			{

			/* Allocate space in the list */
			num_lines++;
			lines  = GETMEM(lines,  LINE,   num_lines);
			sense  = GETMEM(sense,  HAND,   num_lines);
			values = GETMEM(values, STRING, num_lines);

			/* Add the line to the list */
			lines[num_lines-1] = copy_line(curve->line);
			sense[num_lines-1] = curve->sense;

			/* Add the attribute value to the list */
			value = CAL_get_attribute(curve->attrib, attribute);
			values[num_lines-1] = strdup(SafeStr(value));
			}
		}

	/* Return now if no lines created */
	if ( num_lines <= 0 )
		{
		if ( Verbose )
			{
			(void) fprintf(stdout,
					" No lines for field(s) ... %s from %s at %s\n",
					all_fields, CurSource, TVstamp);
			}
		(void) destroy_set(set);
		return TRUE;
		}

	/* Force all locations to be anchored to the current map */
	/*  ... regardless of the current anchor position!       */
	cur_anchor  = AnchorToMap;
	clip_to_map = TRUE;
	AnchorToMap = TRUE;
	(void) sprintf(out_buf, "### Begin lines for field(s) ... %s", all_fields);
	(void) write_graphics_comment(out_buf);
	if ( Verbose )
		{
		(void) fprintf(stdout,
				" Drawing lines for field(s) ... %s from %s at %s\n",
				all_fields, CurSource, TVstamp);
		}

	/* Now loop to output each line */
	for ( iline=0; iline<num_lines; iline++ )
		{

		/* Set the current presentation parameters */
		(void) copy_presentation(&cur_pres, &CurPres);

		/* Reset presentation parameters from look up table (if required) */
		if ( !blank(look_up) )
			{

			/* Find presentation for value matched in look up table */
			if ( match_category_lookup(look_up, values[iline],
					NullStringPtr, NullStringPtr, &pname) )
				{

				if ( Verbose )
					{
					(void) fprintf(stdout,
							"   Matching presentation for ... \"%s\"",
							values[iline]);
					(void) fprintf(stdout, "  from look up ... %s\n", look_up);
					}

				/* Match presentation from list of named presentations */
				named_pres = get_presentation(pname);
				(void) copy_presentation(&cur_pres, &named_pres);
				}

			/* Warning if value cannot be matched */
			else
				{
				(void) fprintf(stdout, "   Presentation for ... \"%s\"",
						values[iline]);
				(void) fprintf(stdout, "  not found in look up ... %s\n",
						look_up);
				}
			}

		/* Override pattern and pattern sizes (if requested) */
		if ( !blank(pattern) )
						(void) strcpy(cur_pres.pattern,        pattern);
		if ( !blank(pattern_width) )
						(void) strcpy(cur_pres.pattern_width,  pattern_width);
		if ( !blank(pattern_length) )
						(void) strcpy(cur_pres.pattern_length, pattern_length);

		/* Override component presentation (if requested) */
		if ( num_comp > 0 )
			{
			cur_pres.num_comp  = num_comp;
			cur_pres.comp_pres = comp_pres;
			(void) reset_presentation_by_comp_pres(&cur_pres,
													&cur_pres.comp_pres[0]);
			}

		/* Start of line grouping */
		(void) sprintf(out_buf, "### Begin line for ... %s", values[iline]);
		(void) write_graphics_comment(out_buf);
		(void) write_graphics_group(GPGstart, NullPointer, 0);

		/* Display a "simple" line */
		if ( blank(cur_pres.pattern) || same(cur_pres.pattern, PatternSimple) )
			{

			/* Display the line */
			if ( !GRA_display_simple_line(lines[iline], 0.0, 0.0,
					cur_pres, clip_to_map) )
				{
				(void) error_report("Error drawing simple line");
				}
			}

		/* Display a line using the given pattern */
		else
			{

			/* Display the line */
			if ( !GRA_display_patterned_line(lines[iline], 0.0, 0.0,
					cur_pres, clip_to_map, sense[iline]) )
				{
				(void) error_report("Error drawing patterned line");
				}
			}

		/* Now display the arrow (if required) */
		if ( !blank(arrow_name) )
			{

			/* Set presentation from last component presentation */
			/*  (if available)                                   */
			if ( cur_pres.num_comp > 0 )
				{
				(void) reset_presentation_by_comp_pres(&cur_pres,
						&cur_pres.comp_pres[cur_pres.num_comp-1]);
				}

			/* Display the arrow */
			if ( !GRA_display_arrow(arrow_name, lines[iline], 0.0, 0.0,
					cur_pres, clip_to_map) )
				{
				(void) error_report("Error drawing arrow");
				}
			}

		/* End line grouping */
		(void) write_graphics_group(GPGend, NullPointer, 0);
		(void) sprintf(out_buf, "### End line for ... %s", values[iline]);
		(void) write_graphics_comment(out_buf);
		}

	/* Reset the current anchor */
	AnchorToMap = cur_anchor;
	(void) sprintf(out_buf, "### End lines for field(s) ... %s", all_fields);
	(void) write_graphics_comment(out_buf);

	/* Free space used by work objects and return TRUE */
	for ( iline=0; iline<num_lines; iline++ )
		{
		(void) destroy_line(lines[iline]);
		}
	FREEMEM(lines);
	FREEMEM(sense);
	FREELIST(values, num_lines);
	(void) destroy_set(set);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    G R A _ d i s p l a y _ b o x                                     *
*                                                                      *
***********************************************************************/

LOGICAL			GRA_display_box

	(
	float		width,			/* Width of box (display units) */
	float		height,			/* Height of box (display units) */
	float		rotation,		/* Box rotation (degrees) */
	LOGICAL		rotate_lat,		/* Rotate box to align with latitude? */
	LOGICAL		rotate_lon,		/* Rotate box to align with longitude? */
	float		xoff,			/* x offset of center of box (display units) */
	float		yoff,			/* y offset of center of box (display units) */
	STRING		lat,			/* Latitude location for symbol */
	STRING		lon,			/* Longitude location for symbol */
	STRING		map_x,			/* x axis position for symbol */
	STRING		map_y,			/* y axis position for symbol */
	float		map_units,		/* Units for x and y axis */
	STRING		loc_ident,		/* Location identifier for symbol */
	STRING		loc_lookup,		/* Location look up table */
	STRING		vtime,			/* Valid time to match */
	STRING		table_name,		/* Table name for locations */
	STRING		grid_name,		/* Grid name for locations */
	STRING		list_name,		/* List name for locations */
	STRING		sym_fill_name	/* Symbol pattern for interior of box */
	)

	{
	int			iix, iiy, isite, iloc, iil;
	float		pscale, flat, flon, fact, roadadj;
	float		xxo, yyo, xx, yy, xxt, yyt, xxs, yys;
	STRING		loclat, loclon, vt;
	LOGICAL		status;
	POINT		pos;
	GRA_TABLE	*cur_table;
	GRA_GRID	*cur_grid;
	GRA_LIST	*cur_list;
	char		err_buf[GPGLong];

	/* Display box for each table site */
	if ( !blank(table_name) )
		{

		/* Find the named table */
		cur_table = get_table(table_name);
		if ( IsNull(cur_table) )
			{
			(void) sprintf(err_buf, "Table ... %s ... not yet defined",
					table_name);
			(void) error_report(err_buf);
			}

		/* Display box for all table sites */
		(void) write_graphics_comment("### Begin box display");
		for ( isite=0; isite<cur_table->nsites; isite++ )
			{

			/* Set table location for box display */
			xxt = cur_table->x_off;
			yyt = cur_table->y_off;
			if ( same(cur_table->type, TableCol) )
				yyt += cur_table->offset[isite];
			else if ( same(cur_table->type, TableRow) )
				xxt += cur_table->offset[isite];
			(void) anchored_location(ZeroPoint, xxt, yyt, &xxo, &yyo);

			/* Set offset location */
			xx = xxo + xoff;
			yy = yyo + yoff;

			/* Now draw the box filled with symbols               */
			/*  ... centered on the anchored position and rotated */
			if ( !blank(sym_fill_name) )
				{

				/* First display the "filled" box */
				(void) write_graphics_box(xx, yy, width, height, rotation,
						FALSE, TRUE);

				/* Then display the symbols */
				if ( !GRA_display_box_symbol_fill(xx, yy, width, height,
						rotation, sym_fill_name) ) return FALSE;

				/* Then display the box "outline" */
				(void) write_graphics_box(xx, yy, width, height, rotation,
						TRUE, FALSE);
				}

			/* Or just draw the box                               */
			/*  ... centered on the anchored position and rotated */
			else
				{
				(void) write_graphics_box(xx, yy, width, height, rotation,
						TRUE, TRUE);
				}
			}

		/* Return TRUE when all table sites have been displayed */
		(void) write_graphics_comment("### End box display");
		return TRUE;
		}

	/* Display box for each grid location */
	else if ( !blank(grid_name) )
		{

		/* Find the named grid */
		cur_grid = get_sample_grid(grid_name);
		if ( IsNull(cur_grid) )
			{
			(void) sprintf(err_buf, "Grid ... %s ... not yet defined",
					grid_name);
			(void) error_report(err_buf);
			}

		/* Display box for all grid locations */
		(void) write_graphics_comment("### Begin box display");
		for ( iiy=0; iiy<cur_grid->numy; iiy++ )
			for ( iix=0; iix<cur_grid->numx; iix++ )
				{

				/* Set latitude/longitude and position for display */
				flat = cur_grid->flats[iiy][iix];
				flon = cur_grid->flons[iiy][iix];
				(void) ll_to_pos(&BaseMap, flat, flon, pos);

				/* Check for grid location off the map */
				if ( pos[X] < 0.0 || pos[Y] < 0.0
						|| pos[X] > BaseMap.definition.xlen
						|| pos[Y] > BaseMap.definition.ylen )
					{

					if ( Verbose )
						{
						(void) fprintf(stdout,
								"  Grid lat/lon outside map at ... %.1f %.1f\n",
								flat, flon);
						}

					/* Continue for grid location off map */
					if ( AnchorToMap ) continue;
					}

				/* Comment for grid location on the map */
				else
					{
					if ( Verbose )
						{
						(void) fprintf(stdout,
								"  Grid lat/lon at ... %.1f %.1f\n",
								flat, flon);
						}
					}

				/* Set grid location on the current map */
				if ( AnchorToMap )
					{
					(void) anchored_location(pos, 0.0, 0.0, &xxo, &yyo);
					}

				/* Set display location to current anchor position */
				else
					{
					xxs = (float) iix * cur_grid->x_shift;
					yys = (float) iiy * cur_grid->y_shift;
					(void) anchored_location(ZeroPoint, xxs, yys, &xxo, &yyo);
					}

				/* Set offset location */
				xx = xxo + xoff;
				yy = yyo + yoff;

				/* Adjust box size for perspective (if required) */
				if ( perspective_scale(&pscale) )
					{
					width  *= pscale;
					height *= pscale;
					}

				/* Set rotation from latitude or longitude */
				if ( AnchorToMap && rotate_lat )
					roadadj = wind_dir_xy(&BaseMap, flat, flon, 90.0);
				else if ( AnchorToMap && rotate_lon )
					roadadj = wind_dir_xy(&BaseMap, flat, flon,  0.0);
				else
					roadadj = 0.0;

				/* Add default rotation */
				roadadj += rotation;

				/* Now draw the box filled with symbols               */
				/*  ... centered on the anchored position and rotated */
				if ( !blank(sym_fill_name) )
					{

					/* First display the "filled" box */
					(void) write_graphics_box(xx, yy, width, height, roadadj,
							FALSE, TRUE);

					/* Then display the symbols */
					if ( !GRA_display_box_symbol_fill(xx, yy, width, height,
							roadadj, sym_fill_name) ) return FALSE;

					/* Then display the box "outline" */
					(void) write_graphics_box(xx, yy, width, height, roadadj,
							TRUE, FALSE);
					}

				/* Or just draw the box                               */
				/*  ... centered on the anchored position and rotated */
				else
					{
					(void) write_graphics_box(xx, yy, width, height, roadadj,
							TRUE, TRUE);
					}
				}

		/* Return TRUE when all grid locations have been displayed */
		(void) write_graphics_comment("### End box display");
		return TRUE;
		}

	/* Display box for each list location */
	else if ( !blank(list_name) )
		{

		/* Find the named list */
		cur_list = get_sample_list(list_name);
		if ( IsNull(cur_list) )
			{
			(void) sprintf(err_buf, "List ... %s ... not yet defined",
					list_name);
			(void) error_report(err_buf);
			}

		/* Initialize offsets for list locations */
		iil = 0;
		xxs = 0.0;
		yys = 0.0;

		/* Display box for all list locations */
		(void) write_graphics_comment("### Begin box display");
		for ( isite=0; isite<cur_list->num; isite++ )
			{

			/* Set latitude/longitude for display */
			if ( cur_list->usell[isite] )
				{
				flat   = cur_list->flats[isite];
				flon   = cur_list->flons[isite];
				}

			/* Get latitude/longitude for display from all locations */
			/*  in a location look up table                          */
			else if ( same_ic(cur_list->idents[isite], LocIdentAll)
						|| same_ic(cur_list->idents[isite], LocIdentAllVtime) )
				{
				if ( blank(loc_lookup) )
					(void) error_report("No location look up table");
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"   Display at all locations from look up ... %s\n",
							loc_lookup);
					}

				/* Set valid time to check */
				if ( same_ic(cur_list->idents[isite], LocIdentAllVtime) )
					vt = vtime;
				else
					vt = FpaCblank;

				/* Set display locations for all look up table locations */
				iloc = -1;
				while ( (iloc = next_location_lookup_line(loc_lookup, iloc, vt,
						NullStringPtr, &loclat, &loclon, NullStringPtr)) >= 0 )
					{

					/* Set latitude/longitude for display */
					flat = read_lat(loclat, &status);
					if ( !status )
						(void) error_report("Problem with location look up lat");
					flon = read_lon(loclon, &status);
					if ( !status )
						(void) error_report("Problem with location look up lon");

					/* Set map position for display */
					(void) ll_to_pos(&BaseMap, flat, flon, pos);

					/* Check for list location off the map */
					if ( pos[X] < 0.0 || pos[Y] < 0.0
							|| pos[X] > BaseMap.definition.xlen
							|| pos[Y] > BaseMap.definition.ylen )
						{

						if ( Verbose )
							{
							(void) fprintf(stdout,
									"  List lat/lon outside map at ... %.1f %.1f\n",
									flat, flon);
							}

						/* Continue for list location off map */
						if ( AnchorToMap ) continue;
						}

					/* Comment for list location on the map */
					else
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
									"  List lat/lon at ... %.1f %.1f\n",
									flat, flon);
							}
						}

					/* Set list location on the current map */
					if ( AnchorToMap )
						{
						(void) anchored_location(pos, 0.0, 0.0, &xxo, &yyo);
						}

					/* Set display location to current anchor position   */
					/*  and set offset for each successive list location */
					else
						{
						(void) anchored_location(ZeroPoint, xxs, yys,
								&xxo, &yyo);
						iil++;
						if ( cur_list->x_wrap > 1
									&& iil % cur_list->x_wrap == 0 )
							{
							xxs  = 0.0;
							yys += cur_list->y_shift;
							}
						else if ( cur_list->x_wrap > 1 )
							{
							xxs += cur_list->x_shift;
							}
						else if ( cur_list->y_wrap > 1
									&& iil % cur_list->y_wrap == 0 )
							{
							xxs += cur_list->x_shift;
							yys  = 0.0;
							}
						else if ( cur_list->y_wrap > 1 )
							{
							yys += cur_list->y_shift;
							}
						else
							{
							xxs += cur_list->x_shift;
							yys += cur_list->y_shift;
							}
						}

					/* Set offset location */
					xx = xxo + xoff;
					yy = yyo + yoff;

					/* Adjust box size for perspective (if required) */
					if ( perspective_scale(&pscale) )
						{
						width  *= pscale;
						height *= pscale;
						}

					/* Set rotation from latitude or longitude */
					if ( AnchorToMap && rotate_lat )
						roadadj = wind_dir_xy(&BaseMap, flat, flon, 90.0);
					else if ( AnchorToMap && rotate_lon )
						roadadj = wind_dir_xy(&BaseMap, flat, flon,  0.0);
					else
						roadadj = 0.0;

					/* Add default rotation */
					roadadj += rotation;

					/* Now draw the box filled with symbols               */
					/*  ... centered on the anchored position and rotated */
					if ( !blank(sym_fill_name) )
						{

						/* First display the "filled" box */
						(void) write_graphics_box(xx, yy, width, height,
								roadadj, FALSE, TRUE);

						/* Then display the symbols */
						if ( !GRA_display_box_symbol_fill(xx, yy, width, height,
								roadadj, sym_fill_name) ) return FALSE;

						/* Then display the box "outline" */
						(void) write_graphics_box(xx, yy, width, height,
								roadadj, TRUE, FALSE);
						}

					/* Or just draw the box                               */
					/*  ... centered on the anchored position and rotated */
					else
						{
						(void) write_graphics_box(xx, yy, width, height,
								roadadj, TRUE, TRUE);
						}
					}

				/* Go on to next display location in sample list */
				continue;
				}

			/* Get latitude/longitude for display from location look up */
			else
				{
				if ( blank(loc_lookup) )
					(void) error_report("No location look up table");
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"   Matching location ident ... \"%s\"",
							cur_list->idents[isite]);
					(void) fprintf(stdout, "  from look up ... %s\n",
							loc_lookup);
					}
				if ( !match_location_lookup(loc_lookup,
						cur_list->idents[isite], vtime, &loclat, &loclon,
						NullStringPtr) )
					{
					(void) sprintf(err_buf,
							"Error matching \"%s\" in look up ... %s",
							cur_list->idents[isite], loc_lookup);
					(void) error_report(err_buf);
					}
				flat = read_lat(loclat, &status);
				if ( !status )
					(void) error_report("Problem with location look up lat");
				flon = read_lon(loclon, &status);
				if ( !status )
					(void) error_report("Problem with location look up lon");
				}

			/* Set map position for display */
			(void) ll_to_pos(&BaseMap, flat, flon, pos);

			/* Check for list location off the map */
			if ( pos[X] < 0.0 || pos[Y] < 0.0
					|| pos[X] > BaseMap.definition.xlen
					|| pos[Y] > BaseMap.definition.ylen )
				{

				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  List lat/lon outside map at ... %.1f %.1f\n",
							flat, flon);
					}

				/* Continue for list location off map */
				if ( AnchorToMap ) continue;
				}

			/* Comment for list location on the map */
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  List lat/lon at ... %.1f %.1f\n",
							flat, flon);
					}
				}

			/* Set list location on the current map */
			if ( AnchorToMap )
				{
				(void) anchored_location(pos, 0.0, 0.0, &xxo, &yyo);
				}

			/* Set display location to current anchor position   */
			/*  and set offset for each successive list location */
			else
				{
				(void) anchored_location(ZeroPoint, xxs, yys, &xxo, &yyo);
				iil++;
				if ( cur_list->x_wrap > 1 && iil % cur_list->x_wrap == 0 )
					{
					xxs  = 0.0;
					yys += cur_list->y_shift;
					}
				else if ( cur_list->x_wrap > 1 )
					{
					xxs += cur_list->x_shift;
					}
				else if ( cur_list->y_wrap > 1 && iil % cur_list->y_wrap == 0 )
					{
					xxs += cur_list->x_shift;
					yys  = 0.0;
					}
				else if ( cur_list->y_wrap > 1 )
					{
					yys += cur_list->y_shift;
					}
				else
					{
					xxs += cur_list->x_shift;
					yys += cur_list->y_shift;
					}
				}

			/* Set offset location */
			xx = xxo + xoff;
			yy = yyo + yoff;

			/* Adjust box size for perspective (if required) */
			if ( perspective_scale(&pscale) )
				{
				width  *= pscale;
				height *= pscale;
				}

			/* Set rotation from latitude or longitude */
			if ( AnchorToMap && rotate_lat )
				roadadj = wind_dir_xy(&BaseMap, flat, flon, 90.0);
			else if ( AnchorToMap && rotate_lon )
				roadadj = wind_dir_xy(&BaseMap, flat, flon,  0.0);
			else
				roadadj = 0.0;

			/* Add default rotation */
			roadadj += rotation;

			/* Now draw the box filled with symbols               */
			/*  ... centered on the anchored position and rotated */
			if ( !blank(sym_fill_name) )
				{

				/* First display the "filled" box */
				(void) write_graphics_box(xx, yy, width, height, roadadj,
						FALSE, TRUE);

				/* Then display the symbols */
				if ( !GRA_display_box_symbol_fill(xx, yy, width, height,
						roadadj, sym_fill_name) ) return FALSE;

				/* Then display the box "outline" */
				(void) write_graphics_box(xx, yy, width, height, roadadj,
						TRUE, FALSE);
				}

			/* Or just draw the box                               */
			/*  ... centered on the anchored position and rotated */
			else
				{
				(void) write_graphics_box(xx, yy, width, height, roadadj,
						TRUE, TRUE);
				}
			}

		/* Return TRUE when all list locations have been displayed */
		(void) write_graphics_comment("### End box display");
		return TRUE;
		}

	/* Display box for all locations in a location look up table */
	else if ( !blank(loc_ident) && !blank(loc_lookup)
				&& ( same_ic(loc_ident, LocIdentAll)
						|| same_ic(loc_ident, LocIdentAllVtime) ) )
		{

		/* Ensure that box can be displayed on current map */
		if ( !AnchorToMap )
			{
			(void) warn_report("Must set anchor to map!");
			return TRUE;
			}

		/* Set valid time to check */
		if ( same_ic(loc_ident, LocIdentAllVtime) ) vt = vtime;
		else                                        vt = FpaCblank;

		/* Display box for all look up table locations */
		iloc = -1;
		(void) write_graphics_comment("### Begin box display");
		while ( (iloc = next_location_lookup_line(loc_lookup, iloc, vt,
						NullStringPtr, &loclat, &loclon, NullStringPtr)) >= 0 )
			{

			/* Set latitude/longitude for display */
			flat = read_lat(loclat, &status);
			if ( !status )
				(void) error_report("Problem with location look up lat");
			flon = read_lon(loclon, &status);
			if ( !status )
				(void) error_report("Problem with location look up lon");

			/* Set map position for display */
			(void) ll_to_pos(&BaseMap, flat, flon, pos);

			/* Check for look up table location off the map */
			if ( pos[X] < 0.0 || pos[Y] < 0.0
					|| pos[X] > BaseMap.definition.xlen
					|| pos[Y] > BaseMap.definition.ylen )
				{

				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Lookup lat/lon outside map at ... %.1f %.1f\n",
							flat, flon);
					}

				/* Continue for look up table location off map */
				continue;
				}

			/* Comment for look up table location on the map */
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Lookup lat/lon at ... %.1f %.1f\n",
							flat, flon);
					}
				}

			/* Set offset position from map position */
			(void) anchored_location(pos, xoff, yoff, &xx, &yy);

			/* Adjust box size for perspective (if required) */
			if ( perspective_scale(&pscale) )
				{
				width  *= pscale;
				height *= pscale;
				}

			/* Set rotation from latitude or longitude */
			if ( AnchorToMap && rotate_lat )
				roadadj = wind_dir_xy(&BaseMap, flat, flon, 90.0);
			else if ( AnchorToMap && rotate_lon )
				roadadj = wind_dir_xy(&BaseMap, flat, flon,  0.0);
			else
				roadadj = 0.0;

			/* Add default rotation */
			roadadj += rotation;

			/* Now draw the box filled with symbols               */
			/*  ... centered on the anchored position and rotated */
			if ( !blank(sym_fill_name) )
				{

				/* First display the "filled" box */
				(void) write_graphics_box(xx, yy, width, height, roadadj,
						FALSE, TRUE);

				/* Then display the symbols */
				if ( !GRA_display_box_symbol_fill(xx, yy, width, height,
						roadadj, sym_fill_name) ) return FALSE;

				/* Then display the box "outline" */
				(void) write_graphics_box(xx, yy, width, height, roadadj,
						TRUE, FALSE);
				}

			/* Or just draw the box                               */
			/*  ... centered on the anchored position and rotated */
			else
				{
				(void) write_graphics_box(xx, yy, width, height, roadadj,
						TRUE, TRUE);
				}
			}

		/* Return TRUE when all look up table locations have been displayed */
		(void) write_graphics_comment("### End box display");
		return TRUE;
		}

	/* Display box at a set location */
	else
		{

		/* Set an offset position based on latitude and longitude */
		if ( ( !blank(lat) && !blank(lon) )
				|| ( !blank(map_x) && !blank(map_y) )
				|| ( !blank(loc_ident) && !blank(loc_lookup) ) )
			{

			/* Ensure that box can be displayed on current map */
			if ( !AnchorToMap )
				{
				(void) warn_report("Must set anchor to map!");
				return TRUE;
				}

			/* Set the latitude and longitude */
			if ( !blank(lat) && !blank(lon) )
				{
				flat = read_lat(lat, &status);
				if ( !status ) (void) error_report("Problem with lat");
				flon = read_lon(lon, &status);
				if ( !status ) (void) error_report("Problem with lon");

				/* Convert latitude and longitude to map position */
				(void) ll_to_pos(&BaseMap, flat, flon, pos);
				}

			/* Set the map position (adjusted by map_units) */
			else if ( !blank(map_x) && !blank(map_y) )
				{
				(void) sscanf(map_x, "%f", &pos[X]);
				(void) sscanf(map_y, "%f", &pos[Y]);
				fact = map_units / BaseMap.definition.units;
				pos[X] *= fact;
				pos[Y] *= fact;

				/* Convert map position to latitude and longitude */
				(void) pos_to_ll(&BaseMap, pos, &flat, &flon);
				}

			/* Get the latitude and longitude from location look up table */
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"   Matching location ident ... \"%s\"",
							loc_ident);
					(void) fprintf(stdout, "  from look up ... %s\n",
							loc_lookup);
					}
				if ( !match_location_lookup(loc_lookup, loc_ident,
						vtime, &loclat, &loclon, NullStringPtr) )
					{
					(void) sprintf(err_buf,
							"Error matching \"%s\" in look up ... %s",
							loc_ident, loc_lookup);
					(void) error_report(err_buf);
					}
				flat = read_lat(loclat, &status);
				if ( !status )
					(void) error_report("Problem with location look up lat");
				flon = read_lon(loclon, &status);
				if ( !status )
					(void) error_report("Problem with location look up lon");

				/* Convert latitude and longitude to map position */
				(void) ll_to_pos(&BaseMap, flat, flon, pos);
				}

			/* Return for map positions off the map */
			if ( pos[X] < 0.0 || pos[Y] < 0.0
					|| pos[X] > BaseMap.definition.xlen
					|| pos[Y] > BaseMap.definition.ylen )
				{
				if ( Verbose )
					{
					if ( !blank(map_x) && !blank(map_y) )
						(void) fprintf(stdout,
								"  Box position off map at ... %f %f\n",
								pos[X], pos[Y]);
					else
						(void) fprintf(stdout,
								"  Box lat/lon off map at ... %.1f %.1f\n",
								flat, flon);
					}

				return TRUE;
				}

			/* Comments for map positions on the map */
			else
				{
				if ( Verbose )
					{
					if ( !blank(map_x) && !blank(map_y) )
						(void) fprintf(stdout,
								"  Box position at ... %f %f\n",
								pos[X], pos[Y]);
					else
						(void) fprintf(stdout,
								"  Box lat/lon at ... %.1f %.1f\n",
								flat, flon);
					}
				}

			/* Set offset position from map position */
			(void) anchored_location(pos, xoff, yoff, &xx, &yy);
			}

		/* Set an absolute offset position */
		else
			{
			(void) anchored_location(ZeroPoint, xoff, yoff, &xx, &yy);
			}

		/* Adjust box size for perspective (if required) */
		if ( perspective_scale(&pscale) )
			{
			width  *= pscale;
			height *= pscale;
			}

		/* Set rotation from latitude or longitude */
		if ( AnchorToMap && rotate_lat )
			roadadj = wind_dir_xy(&BaseMap, flat, flon, 90.0);
		else if ( AnchorToMap && rotate_lon )
			roadadj = wind_dir_xy(&BaseMap, flat, flon,  0.0);
		else
			roadadj = 0.0;

		/* Add default rotation */
		roadadj += rotation;

		(void) write_graphics_comment("### Begin box display");

		/* Now draw the box filled with symbols               */
		/*  ... centered on the anchored position and rotated */
		if ( !blank(sym_fill_name) )
			{

			/* First display the "filled" box */
			(void) write_graphics_box(xx, yy, width, height, roadadj,
					FALSE, TRUE);

			/* Then display the symbols */
			if ( !GRA_display_box_symbol_fill(xx, yy, width, height,
					roadadj, sym_fill_name) ) return FALSE;

			/* Then display the box "outline" */
			(void) write_graphics_box(xx, yy, width, height, roadadj,
					TRUE, FALSE);
			}

		/* Or just draw the box                               */
		/*  ... centered on the anchored position and rotated */
		else
			{
			(void) write_graphics_box(xx, yy, width, height, roadadj,
					TRUE, TRUE);
			}

		/* Return TRUE when box has been displayed */
		(void) write_graphics_comment("### End box display");
		return	TRUE;
		}
	}

/***********************************************************************
*                                                                      *
*    G R A _ d i s p l a y _ x s e c t i o n _ a r e a s               *
*    G R A _ d i s p l a y _ x s e c t i o n _ a x i s _ l a b e l s   *
*    G R A _ d i s p l a y _ x s e c t i o n _ c o n t o u r s         *
*    G R A _ d i s p l a y _ x s e c t i o n _ c u r v e s             *
*                                                                      *
***********************************************************************/

LOGICAL			GRA_display_xsection_areas

	(
	STRING			xsection_name,	/* Cross section name */
	STRING			element,		/* Element name */
	STRING			level,			/* Level name */
	STRING			equation,		/* Equation to calculate */
	STRING			ver_units,		/* Units for values */
	STRING			data_file,		/* Data file for parameters */
	STRING			data_file_format,	/* Format for data file */
	STRING			data_file_units,	/* Units for values in data file */
	STRING			cat_cascade,	/* And/Or for multiple CATATTRIB structs */
	CATATTRIB		*cat_attrib,	/* Structure containing categories */
	int				num_catatt,		/* Number of categories */
	STRING			attribute_upper,	/* Upper attribute from field */
	STRING			attribute_lower,	/* Lower attribute from field */
	STRING			loc_lookup,		/* Look up file for horizontal axis */
	GPGltype		ltype,			/* Type of locations for horizontal axis */
	int				num_xloc,		/* Number of locations for horizontal */
	XSECT_LOCATION	*xsect_locs,	/* Locations for horizontal axis */
	STRING			ver_lookup,		/* Look up file for vertical axis */
	float			proximity,		/* Proximity to area/curve/spot (in km) */
	STRING			display_func,	/* Display function for joining values */
	STRING			interior_fill,	/* Colour for interior of areas */
	STRING			sym_fill_name,	/* Symbol pattern for interior of areas */
	float			box_width,		/* Width of box display (display units) */
	float			xboff,			/* x offset of box display (display units) */
	float			yboff,			/* y offset of box display (display units) */
	STRING			pattern,		/* Name of pattern for lines */
	STRING			pattern_width,	/* Pattern width factor */
	STRING			pattern_length,	/* Pattern repetition factor */
	COMP_PRES		*comp_pres,		/* Structure containing presentations */
	int				num_comp		/* Number of presentations */
	)

	{
	int				iloc;
	LOGICAL			clip_to_map, status_upper, status_lower;
	STRING			ident, vtime;
	PRES			temp_pres;
	float			flat, flon, yloc_upper, yloc_lower;
	float			xxl, yyl_upper, yyl_lower, xxc, yyc_upper, yyc_lower, yyc;
	float			xmid, xwidth, yheight, xxo, yyo;
	POINT			pos, upos, lpos;
	GRA_XSECT		*cur_xsect;
	XSECT_HOR_AXIS	*haxis;
	XSECT_VER_AXIS	*vaxis;
	char			err_buf[GPGLong];

	FLD_DESCRIPT	descript;

	/* Static buffers to hold cross section lines */
	static	LINE	XSectLineUpper = NullLine;
	static	LINE	XSectLineLower = NullLine;

	/* Make a copy of the global field descriptor */
	(void) copy_fld_descript(&descript, &Fdesc);

	/* Re-initialize the field descriptor for this element and level */
	if ( !set_fld_descript(&descript, FpaF_DIRECTORY_PATH, FpaCblank,
									FpaF_SOURCE_NAME,   CurSource,
									FpaF_RUN_TIME,      FpaCblank,
									FpaF_ELEMENT_NAME,  element,
									FpaF_LEVEL_NAME,    level,
									FpaF_VALID_TIME,    FpaCblank,
									FpaF_END_OF_LIST) )
		{
		(void) sprintf(err_buf,
				" Error setting field descriptor for ... %s %s from %s\n",
				element, level, CurSource);
		(void) error_report(err_buf);
		}

	/* Find the named cross section */
	cur_xsect = get_cross_section(xsection_name);
	if ( IsNull(cur_xsect) )
		{
		(void) sprintf(err_buf, "Cross Section ... %s ... not yet defined",
				xsection_name);
		(void) error_report(err_buf);
		}

	/* Get horizontal axis parameters */
	haxis = cross_section_horizontal_axis(cur_xsect, loc_lookup, ltype,
											num_xloc, xsect_locs, NullDouble);
	if ( IsNull(haxis) )
		{
		(void) sprintf(err_buf,
				"Error in cross section horizontal look up ... %s", loc_lookup);
		(void) error_report(err_buf);
		}

	/* Get vertical axis parameters */
	vaxis = cross_section_vertical_axis(cur_xsect, ver_lookup, NullDouble);
	if ( IsNull(vaxis) )
		{
		(void) sprintf(err_buf,
				"Error in cross section vertical look up ... %s", ver_lookup);
		(void) error_report(err_buf);
		}

	/* Create or clear the cross section lines */
	if ( IsNull(XSectLineUpper) ) XSectLineUpper = create_line();
	else                          (void) empty_line(XSectLineUpper);
	if ( IsNull(XSectLineLower) ) XSectLineLower = create_line();
	else                          (void) empty_line(XSectLineLower);

	/* Clip the final line to the current cross section */
	/*  (except for box display of attributes)          */
	if ( same(display_func, XSectLineBox) ) clip_to_map = FALSE;
	else                                    clip_to_map = TRUE;

	/* Save the current presentation (for box display) */
	(void) copy_presentation(&temp_pres, &CurPres);

	/* Set the current presentation (for box display) */
	/* Note that only outline and interior_fill are used for box display */
	if ( same(display_func, XSectLineBox) )
		{
		if ( num_comp > 0 )
			{
			if ( !blank(comp_pres[0].outline) )
					(void) strcpy(CurPres.outline, comp_pres[0].outline);
			}
		if ( !blank(interior_fill) )
				(void) strcpy(CurPres.interior_fill, interior_fill);
		}

	/* Sample fields for all cross section locations */
	xxl = yyl_upper = yyl_lower = 0.0;
	for ( iloc=0; iloc<haxis->num; iloc++ )
		{

		/* Set horizontal axis parameters for sampling */
		ident = haxis->idents[iloc];
		flat  = haxis->flats[iloc];
		flon  = haxis->flons[iloc];
		vtime = haxis->vtimes[iloc];

		/* Set map position for sampling */
		(void) ll_to_pos(&BaseMap, flat, flon, pos);
		if ( pos[X] < 0.0 || pos[Y] < 0.0
				|| pos[X] > BaseMap.definition.xlen
				|| pos[Y] > BaseMap.definition.ylen )
			{
			if ( Verbose )
				{
				(void) fprintf(stdout,
					"  Cross section lat/lon outside map at ... %.1f %.1f\n",
					flat, flon);
				}
			}

		/* Reset field descriptor for cross section valid time */
		if ( !set_fld_descript(&descript, FpaF_VALID_TIME, vtime,
										FpaF_END_OF_LIST) )
			{
			(void) sprintf(err_buf, "Error re-setting valid time ... %s\n",
					vtime);
			(void) error_report(err_buf);
			}

		/* Set vertical location of values from data file (if found) */
		if ( !blank(data_file) )
			{
			yloc_upper = set_vertical_position_data_file(vaxis, data_file,
					data_file_format, data_file_units, ident, flat, flon,
					vtime, attribute_upper, ver_units, &status_upper);
			yloc_lower = set_vertical_position_data_file(vaxis, data_file,
					data_file_format, data_file_units, ident, flat, flon,
					vtime, attribute_lower, ver_units, &status_lower);
			}

		/* Set vertical location of parameters (if found) */
		else
			{
			yloc_upper = set_vertical_position(&descript, vaxis, pos,
					element, level, equation, ver_units, FpaCblank,
					cat_cascade, cat_attrib, num_catatt, attribute_upper,
					FpaCblank, FpaCblank, proximity, &status_upper);
			yloc_lower = set_vertical_position(&descript, vaxis, pos,
					element, level, equation, ver_units, FpaCblank,
					cat_cascade, cat_attrib, num_catatt, attribute_lower,
					FpaCblank, FpaCblank, proximity, &status_lower);
			}

		/* Set cross section locations for these values */
		xxc       = cur_xsect->x_off + (cur_xsect->width  * haxis->locs[iloc]);
		yyc_upper = cur_xsect->y_off + (cur_xsect->height * yloc_upper);
		yyc_lower = cur_xsect->y_off + (cur_xsect->height * yloc_lower);
		yyc       = (yyc_upper + yyc_lower) / 2.0;
		xmid      = (xxc + xxl) / 2.0;
		xwidth    = box_width;
		yheight   = yyc_upper - yyc_lower;

		/* Display cross section parameters as box */
		if ( same(display_func, XSectLineBox) )
			{

			/* Sample next location if parameters not found */
			if ( !status_upper || !status_lower ) continue;

			/* Set the anchored location for display */
			(void) anchored_location(ZeroPoint, xxc, yyc, &xxo, &yyo);
			xxo += xboff;
			yyo += yboff;

			/* Draw a box filled with symbols              */
			/*  ... centered on the cross section location */
			if ( !blank(sym_fill_name) )
				{

				/* First display the "filled" box */
				(void) write_graphics_box(xxo, yyo, xwidth, yheight, 0.0,
						FALSE, TRUE);

				/* Then display the symbols */
				if ( !GRA_display_box_symbol_fill(xxo, yyo, xwidth, yheight,
						0.0, sym_fill_name) ) return FALSE;

				/* Then display the box "outline" */
				(void) write_graphics_box(xxo, yyo, xwidth, yheight, 0.0,
						TRUE, FALSE);
				}

			/* Or just draw a box                          */
			/*  ... centered on the cross section location */
			else
				{
				(void) write_graphics_box(xxo, yyo, xwidth, yheight, 0.0,
						TRUE, TRUE);
				}

			/* Then sample next location */
			continue;
			}

		/* Display cross section line segments as area if parameter not found */
		if ( !status_upper || !status_lower )
			{

			/* Continue if cross section line segments have no locations */
			if ( XSectLineUpper->numpts < 1 )
				{
				xxl       = xxc;
				yyl_upper = yyc_upper;
				yyl_lower = yyc_lower;
				continue;
				}

			/* Set end locations on each cross section line segment */
			/*  ... for linear or centred step display              */
			if ( same(display_func, XSectLineLinear)
					|| same(display_func, XSectLineStepCentre) )
				{

				/* Replicate last value on each cross section line */
				/*  to half way between last and current position  */
				upos[X] = xmid;		upos[Y] = yyl_upper;
				lpos[X] = xmid;		lpos[Y] = yyl_lower;
				(void) add_point_to_line(XSectLineUpper, upos);
				(void) add_point_to_line(XSectLineLower, lpos);
				}

			/* Set end locations on each cross section line segment */
			/*  ... for step before value display                   */
			else if ( same(display_func, XSectLineStepBefore) )
				{

				/* No end locations required for step before! */
				}

			/* Set end locations on each cross section line segment */
			/*  ... for step after value display                    */
			else if ( same(display_func, XSectLineStepAfter) )
				{

				/* Replicate last value on each cross section line */
				/*  to current position                            */
				upos[X] = xxc;		upos[Y] = yyl_upper;
				lpos[X] = xxc;		lpos[Y] = yyl_lower;
				(void) add_point_to_line(XSectLineUpper, upos);
				(void) add_point_to_line(XSectLineLower, lpos);
				}

			/* Append the lower line (reversed) to the upper line */
			XSectLineUpper = append_line_dir(XSectLineUpper, XSectLineLower,
																		FALSE);
			(void) close_line(XSectLineUpper);

			/* Now display the cross section outline */
			if ( !GRA_display_draw_outline(XSectLineUpper, 0.0, 0.0,
					clip_to_map, interior_fill, sym_fill_name, pattern,
					pattern_width, pattern_length, comp_pres, num_comp) )
				{
				(void) fprintf(stdout, "Error drawing cross section area\n");
				return FALSE;
				}

			/* Reset the cross section line segments */
			(void) empty_line(XSectLineUpper);
			(void) empty_line(XSectLineLower);

			/* Then sample next location */
			xxl       = xxc;
			yyl_upper = yyc_upper;
			yyl_lower = yyc_lower;
			continue;
			}

		/* Initialize new cross section segments (if required) */
		if ( iloc > 0 && XSectLineUpper->numpts < 1 )
			{

			/* Set start locations on each cross section line segment */
			/*  ... for linear display                                */
			if ( same(display_func, XSectLineLinear) )
				{

				/* Replicate first value on each cross section line */
				/*  to half way between last and current position   */
				upos[X] = xmid;		upos[Y] = yyc_upper;
				lpos[X] = xmid;		lpos[Y] = yyc_lower;
				(void) add_point_to_line(XSectLineUpper, upos);
				(void) add_point_to_line(XSectLineLower, lpos);
				}

			/* Set start locations on each cross section line segment */
			/*  ... for step display                                  */
			else if ( same(display_func, XSectLineStepCentre)
						|| same(display_func, XSectLineStepBefore)
						|| same(display_func, XSectLineStepAfter) )
				{

				/* No start locations required for step functions! */
				}
			}

		/* Add locations to cross section line segments */
		/*  ... for linear display                      */
		if ( same(display_func, XSectLineLinear) )
			{

			/* Add current value to each cross section line */
			upos[X] = xxc;		upos[Y] = yyc_upper;
			lpos[X] = xxc;		lpos[Y] = yyc_lower;
			(void) add_point_to_line(XSectLineUpper, upos);
			(void) add_point_to_line(XSectLineLower, lpos);
			}

		/* Add locations to cross section line segments */
		/*  ... for centred step display                */
		else if ( same(display_func, XSectLineStepCentre) )
			{

			/* Replicate last value on each cross section line */
			/*  to half way between last and current position  */
			/* Note that cross section lines will contain      */
			/*  points only if last value exists!              */
			if ( iloc > 0 && XSectLineUpper->numpts > 0 )
				{
				upos[X] = xmid;		upos[Y] = yyl_upper;
				lpos[X] = xmid;		lpos[Y] = yyl_lower;
				(void) add_point_to_line(XSectLineUpper, upos);
				(void) add_point_to_line(XSectLineLower, lpos);
				}

			/* Replicate current value on each cross section line */
			/*  to half way between last and current position     */
			if ( iloc > 0 )
				{
				upos[X] = xmid;		upos[Y] = yyc_upper;
				lpos[X] = xmid;		lpos[Y] = yyc_lower;
				(void) add_point_to_line(XSectLineUpper, upos);
				(void) add_point_to_line(XSectLineLower, lpos);
				}

			/* Add current value on each cross section line */
			upos[X] = xxc;		upos[Y] = yyc_upper;
			lpos[X] = xxc;		lpos[Y] = yyc_lower;
			(void) add_point_to_line(XSectLineUpper, upos);
			(void) add_point_to_line(XSectLineLower, lpos);
			}

		/* Add locations to cross section line segments */
		/*  ... for step before value display           */
		else if ( same(display_func, XSectLineStepBefore) )
			{

			/* Replicate current value on each cross section line */
			/*  to last position                                  */
			if ( iloc > 0 )
				{
				upos[X] = xxl;		upos[Y] = yyc_upper;
				lpos[X] = xxl;		lpos[Y] = yyc_lower;
				(void) add_point_to_line(XSectLineUpper, upos);
				(void) add_point_to_line(XSectLineLower, lpos);
				}

			/* Add current value on each cross section line */
			upos[X] = xxc;		upos[Y] = yyc_upper;
			lpos[X] = xxc;		lpos[Y] = yyc_lower;
			(void) add_point_to_line(XSectLineUpper, upos);
			(void) add_point_to_line(XSectLineLower, lpos);
			}

		/* Add locations to cross section line segments */
		/*  ... for step after value display            */
		else if ( same(display_func, XSectLineStepAfter) )
			{

			/* Replicate last value on each cross section line */
			/*  to current position                            */
			/* Note that cross section lines will contain      */
			/*  points only if last value exists!              */
			if ( iloc > 0 && XSectLineUpper->numpts > 0 )
				{
				upos[X] = xxc;		upos[Y] = yyl_upper;
				lpos[X] = xxc;		lpos[Y] = yyl_lower;
				(void) add_point_to_line(XSectLineUpper, upos);
				(void) add_point_to_line(XSectLineLower, lpos);
				}

			/* Add current value on each cross section line */
			upos[X] = xxc;		upos[Y] = yyc_upper;
			lpos[X] = xxc;		lpos[Y] = yyc_lower;
			(void) add_point_to_line(XSectLineUpper, upos);
			(void) add_point_to_line(XSectLineLower, lpos);
			}

		/* Then sample next location */
		xxl       = xxc;
		yyl_upper = yyc_upper;
		yyl_lower = yyc_lower;
		}

	/* Append the lower line (reversed) to the upper line     */
	/*  after last location in cross section has been sampled */
	XSectLineUpper = append_line_dir(XSectLineUpper, XSectLineLower, FALSE);
	(void) close_line(XSectLineUpper);

	/* Now display the cross section outline                  */
	/*  after last location in cross section has been sampled */
	if ( !GRA_display_draw_outline(XSectLineUpper, 0.0, 0.0,
			clip_to_map, interior_fill, sym_fill_name, pattern,
			pattern_width, pattern_length, comp_pres, num_comp) )
		{
		(void) fprintf(stdout, "Error drawing cross section area\n");
		return FALSE;
		}

	/* Restore the original presentation */
	(void) copy_presentation(&CurPres, &temp_pres);

	/* Return TRUE */
	return TRUE;
	}

LOGICAL			GRA_display_xsection_axis_labels

	(
	STRING			xsection_name,	/* Cross section name */
	STRING			axis_display,	/* Cross section axis for labels */
	STRING			loc_lookup,		/* Look up file for horizontal axis */
	GPGltype		ltype,			/* Type of locations for horizontal axis */
	int				num_xloc,		/* Number of locations for horizontal */
	XSECT_LOCATION	*xsect_locs,	/* Locations for horizontal axis */
	STRING			ver_lookup,		/* Look up file for vertical axis */
	ATTRIB_DISPLAY	*attribs,		/* Attributes to display */
	int				num_attrib,		/* Number of attributes */
	STRING			time_zone,		/* Time zone (used in strftime) */
	STRING			language,		/* Language (used in strftime) */
	STRING			display_name,	/* Name for label display */
	STRING			display_type,	/* Type of label display */
	float			rotation,		/* Label rotation (degrees) */
	float			xoff,			/* x offset of labels (display units) */
	float			yoff			/* y offset of labels (display units) */
	)

	{
	int				ii;
	float			xshift, yshift, xxo, yyo, xx, yy;
	POINT			pos;
	GRA_XSECT		*cur_xsect;
	LOGICAL			cur_anchor;
	XSECT_HOR_AXIS	*haxis;
	XSECT_VER_AXIS	*vaxis;
	char			err_buf[GPGLong];

	/* Find the named cross section */
	cur_xsect = get_cross_section(xsection_name);
	if ( IsNull(cur_xsect) )
		{
		(void) sprintf(err_buf, "Cross Section ... %s ... not yet defined",
				xsection_name);
		(void) error_report(err_buf);
		}

	/* Ensure that cross section labels are NOT anchored to current */
	/*  map ... regardless of the current anchor position!          */
	cur_anchor  = AnchorToMap;
	AnchorToMap = FALSE;

	/* Display labels on a horizontal cross section axis */
	if ( same(axis_display, XSectAxisLower)
			|| same(axis_display, XSectAxisUpper) )
		{

		/* Get horizontal axis parameters */
		haxis = cross_section_horizontal_axis(cur_xsect, loc_lookup, ltype,
											num_xloc, xsect_locs, NullDouble);
		if ( IsNull(haxis) )
			{
			(void) sprintf(err_buf,
					"Error in cross section horizontal look up ... %s",
					loc_lookup);
			(void) error_report(err_buf);
			}

		/* Set vertical offset for horizontal axis */
		if ( same(axis_display, XSectAxisLower) )
			yshift = cur_xsect->y_off;
		else if ( same(axis_display, XSectAxisUpper) )
			yshift = cur_xsect->y_off + cur_xsect->height;

		/* Loop through locations on horizontal axis */
		for ( ii=0; ii<haxis->num; ii++ )
			{

			/* Set horizontal offset */
			xshift = cur_xsect->x_off + cur_xsect->width * haxis->locs[ii];

			/* Set map position */
			(void) ll_to_pos(&BaseMap, haxis->flats[ii], haxis->flons[ii], pos);

			/* Set offset location for label attributes */
			(void) anchored_location(ZeroPoint, xshift, yshift, &xxo, &yyo);
			xx = xxo + xoff;
			yy = yyo + yoff;

			/* Display labels on the horizontal axis */
			(void) GRA_display_sampled_attributes(haxis->idents[ii],
					haxis->vtimes[ii], time_zone, language, haxis->labels[ii],
					pos, DefProximity, DefBearing, DefLineLen,
					haxis->dirs[ii], haxis->spds[ii],
					NullCal, FpaCblank, NullPtr(CATATTRIB *), 0,
					attribs, num_attrib, NullPtr(SPCASE *), 0, FpaCblank, 100.0,
					display_name, display_type, FALSE, FpaCblank,
					rotation, FALSE, FALSE, FpaCblank, FALSE, xxo, yyo, xx, yy);
			}
		}

	/* Display labels on a vertical cross section axis */
	else if ( same(axis_display, XSectAxisLeft)
			|| same(axis_display, XSectAxisRight) )
		{

		/* Get vertical axis parameters */
		vaxis = cross_section_vertical_axis(cur_xsect, ver_lookup, NullDouble);
		if ( IsNull(vaxis) )
			{
			(void) sprintf(err_buf,
					"Error in cross section vertical look up ... %s",
					ver_lookup);
			(void) error_report(err_buf);
			}

		/* Set horizontal offset for vertical axis */
		if ( same(axis_display, XSectAxisLeft) )
			xshift = cur_xsect->x_off;
		else if ( same(axis_display, XSectAxisRight) )
			xshift = cur_xsect->x_off + cur_xsect->width;

		/* Loop through locations on vertical axis */
		for ( ii=0; ii<vaxis->num; ii++ )
			{

			/* Set vertical offset */
			yshift = cur_xsect->y_off + cur_xsect->height * vaxis->locs[ii];

			/* Set map position (default) */
			(void) copy_point(pos, ZeroPoint);

			/* Set offset location for label attributes */
			(void) anchored_location(ZeroPoint, xshift, yshift, &xxo, &yyo);
			xx = xxo + xoff;
			yy = yyo + yoff;

			/* Display labels on the vertical axis */
			(void) GRA_display_sampled_attributes(vaxis->idents[ii],
					FpaCblank, FpaCblank, FpaCblank, vaxis->labels[ii],
					pos, DefProximity, DefBearing, DefLineLen,
					DefSegDir, DefSegSpd, NullCal, FpaCblank,
					NullPtr(CATATTRIB *), 0, attribs, num_attrib,
					NullPtr(SPCASE *), 0, FpaCblank, 100.0,
					display_name, display_type, FALSE, FpaCblank,
					rotation, FALSE, FALSE, FpaCblank, FALSE, xxo, yyo, xx, yy);
			}
		}

	/* Reset the current anchor */
	AnchorToMap = cur_anchor;

	/* Return TRUE */
	return TRUE;
	}

LOGICAL			GRA_display_xsection_contours

	(
	STRING			xsection_name,	/* Cross section name */
	GRA_FLD_INFO	*xsect_flds,	/* Fields for cross section contours */
	int				num_fields,		/* Number of fields for contours */
	STRING			units,			/* Units for contour field */
	STRING			loc_lookup,		/* Look up file for horizontal axis */
	GPGltype		ltype,			/* Type of locations for horizontal axis */
	int				num_xloc,		/* Number of locations for horizontal */
	XSECT_LOCATION	*xsect_locs,	/* Locations for horizontal axis */
	STRING			ver_lookup,		/* Look up file for vertical axis */
	STRING			values,			/* List of values for contours */
	STRING			min,			/* Minimum value for contours */
	STRING			max,			/* Maximum value for contours */
	STRING			base,			/* Base value for contours */
	STRING			interval,		/* Interval between contours */
	LOGICAL			display_areas,	/* Display banded contours? */
	STRING			interior_fill,	/* Colour for interior of banded contours */
	STRING			sym_fill_name,	/* Symbol pattern for banded contours */
	STRING			pattern,		/* Name of pattern for contours */
	STRING			pattern_width,	/* Pattern width factor */
	STRING			pattern_length,	/* Pattern repetition factor */
	COMP_PRES		*comp_pres,		/* Structure containing presentations */
	int				num_comp		/* Number of presentations */
	)

	{
	int				nfld, nx, ny, fkind;
	LOGICAL			reversed, match_vaxis, clip_to_map;
	GRA_XSECT		*cur_xsect;
	XSECT_HOR_AXIS	*haxis;
	XSECT_VER_AXIS	*vaxis;
	char			err_buf[GPGLong];

	LOGICAL			status = FALSE;
	float			flat, flon;
	POINT			pos[1] = { ZERO_POINT };
	VLIST			*vlist;
	SURFACE			sfc;
	float			fval, fminv, fmaxv, fbase, fint, vmin, vmax, diff;
	double			dval;

	FLD_DESCRIPT	descript;

	FpaConfigFieldStruct	*fdef;
	FpaConfigUnitStruct		*udef;

	/* Storage for cross section contour field values */
	static float		*FldVals   = NullPtr(float *);
	static float		**GridVals = NullPtr(float **);

	/* Storage for cross section contour field units specification */
	static USPEC		uspec = {NullString, 1.0, 0.0};

	/* Find the named cross section */
	cur_xsect = get_cross_section(xsection_name);
	if ( IsNull(cur_xsect) )
		{
		(void) sprintf(err_buf, "Cross Section ... %s ... not yet defined",
				xsection_name);
		(void) error_report(err_buf);
		}

	/* Get horizontal axis parameters */
	haxis = cross_section_horizontal_axis(cur_xsect, loc_lookup, ltype,
											num_xloc, xsect_locs, NullDouble);
	if ( IsNull(haxis) )
		{
		(void) sprintf(err_buf,
				"Error in cross section horizontal look up ... %s", loc_lookup);
		(void) error_report(err_buf);
		}

	/* Get vertical axis parameters */
	vaxis = cross_section_vertical_axis(cur_xsect, ver_lookup, NullDouble);
	if ( IsNull(vaxis) )
		{
		(void) sprintf(err_buf,
				"Error in cross section vertical look up ... %s", ver_lookup);
		(void) error_report(err_buf);
		}

	/* Check for matching numbers of field levels */
	if ( num_fields != vaxis->num )
		{
		(void) sprintf(err_buf,
				"Mismatch in number of fields ... %d  vertical levels ... %d",
				num_fields, vaxis->num);
		(void) error_report(err_buf);
		}

	/* Set the order for extracting the contour data */
	if ( same(xsect_flds[0].level, vaxis->idents[0]) ) reversed = FALSE;
	else                                               reversed = TRUE;

	/* Match the field levels with the vertical axis parameters */
	match_vaxis = TRUE;
	for ( ny=0; ny<vaxis->num; ny++ )
		{

		/* Match each field level with the vertical axis parameters */
		nfld = ny;
		if ( reversed ) nfld = num_fields - ny - 1;
		if ( !same(xsect_flds[nfld].level, vaxis->idents[ny]) )
			{
			match_vaxis = FALSE;
			(void) sprintf(err_buf,
					"Mismatch in field level ... %s  vertical level ... %s",
					xsect_flds[nfld].level, vaxis->idents[ny]);
			(void) warn_report(err_buf);
			}
		}
	if ( !match_vaxis ) return TRUE;

	/* Initialize space for cross section contour field values */
	if ( NotNull(GridVals) ) FREEMEM(GridVals);
	if ( NotNull(FldVals) )  FREEMEM(FldVals);
	FldVals  = INITMEM(float,   haxis->num * vaxis->num);
	GridVals = INITMEM(float *, vaxis->num);

	/* Now extract the data for cross section contouring field by field */
	/* Note that field data is ordered wrt vertical axis parameters     */
	for ( ny=0; ny<vaxis->num; ny++ )
		{

		/* Set field to use for each vertical axis parameter */
		nfld = ny;
		if ( reversed ) nfld = num_fields - ny - 1;

		/* Set the row count location */
		GridVals[ny] = FldVals + ny * haxis->num;

		/* Extract the data for cross section contouring point by point       */
		/* Note that the field data is ordered wrt horizontal axis parameters */
		for ( nx=0; nx<haxis->num; nx++ )
			{

			/* Set latitude/longitude for field data */
			flat = haxis->flats[nx];
			flon = haxis->flons[nx];

			/* Set map position for field data */
			(void) ll_to_pos(&BaseMap, flat, flon, pos[0]);
			if ( pos[0][X] < 0.0 || pos[0][Y] < 0.0
					|| pos[0][X] > BaseMap.definition.xlen
					|| pos[0][Y] > BaseMap.definition.ylen )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
						"  Cross section lat/lon outside map at ... %.1f %.1f\n",
						flat, flon);
					}
				}

			/* Make a copy of the global field descriptor */
			(void) copy_fld_descript(&descript, &Fdesc);

			/* Re-initialize field descriptor for element, level, valid time */
			if ( !set_fld_descript(&descript, FpaF_DIRECTORY_PATH, FpaCblank,
								FpaF_SOURCE_NAME,   CurSource,
								FpaF_RUN_TIME,      FpaCblank,
								FpaF_ELEMENT_NAME,  xsect_flds[nfld].element,
								FpaF_LEVEL_NAME,    xsect_flds[nfld].level,
								FpaF_VALID_TIME,    haxis->vtimes[nx],
								FpaF_END_OF_LIST) )
				{
				(void) sprintf(err_buf,
						" Error setting field descriptor for ... %s %s from %s at %s\n",
						xsect_flds[nfld].element, xsect_flds[nfld].level,
						CurSource, haxis->vtimes[nx]);
				(void) error_report(err_buf);
				}

			/* Set the field type for sampling by equation */
			if ( !blank(xsect_flds[nfld].equation) )
				{
				fkind = FpaC_CONTINUOUS;
				}

			/* Set the field type from the element and level */
			else
				{
				fdef = get_field_info(descript.edef->name, descript.ldef->name);
				if ( IsNull(fdef) )
					{
					(void) sprintf(err_buf,
							"Unrecognized element ... %s  or level ... %s",
							xsect_flds[nfld].element, xsect_flds[nfld].level);
					(void) error_report(err_buf);
					}
				fkind = fdef->element->fld_type;

				/* Check that units match with field information */
				switch ( fkind )
					{

					/* Must match field units for continuous/vector fields */
					case FpaC_CONTINUOUS:
					case FpaC_VECTOR:
						udef = fdef->element->elem_io->units;
						if ( NotNull(udef)
								&& !convert_value(udef->name, 0.0,
															units, NullDouble) )
							{
							(void) sprintf(err_buf,
									"Incorrect units: %s  for field: %s %s with units %s",
									units, xsect_flds[nfld].element,
									xsect_flds[nfld].level, udef->name);
							(void) error_report(err_buf);
							}
						break;
					}
				}

			/* Extract field value depending on field type */
			switch ( fkind )
				{

				/* Extract field value for continuous type fields */
				case FpaC_CONTINUOUS:

					/* Extract field value by equation */
					if ( !blank(xsect_flds[nfld].equation) )
						{
						vlist = retrieve_vlist_by_equation(&descript, 1, pos,
								FpaCmksUnits, xsect_flds[nfld].equation);
						if ( IsNull(vlist) )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
									"Cannot calculate equation ... %s",
									xsect_flds[nfld].equation);
								(void) fprintf(stdout,
									"  from %s at %s\n",
									CurSource, haxis->vtimes[nx]);
								}
							return TRUE;
							}
						}

					/* Extract field value directly */
					else
						{
						vlist = retrieve_vlist(&descript, 1, pos);
						if ( IsNull(vlist) )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
									"Cannot extract field ... %s %s",
									xsect_flds[nfld].element,
									xsect_flds[nfld].level);
								(void) fprintf(stdout,
									"  from %s at %s\n",
									CurSource, haxis->vtimes[nx]);
								}
							return TRUE;
							}
						}
					break;

				/* Extract field values for vector type fields */
				case FpaC_VECTOR:
					vlist = retrieve_vlist_component(&descript, M_Comp, 1, pos);
					if ( IsNull(vlist) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s",
								xsect_flds[nfld].element,
								xsect_flds[nfld].level);
							(void) fprintf(stdout,
								"  from %s at %s\n",
								CurSource, haxis->vtimes[nx]);
							}
						return TRUE;
						}
					break;

				/* Default for all other types of fields */
				case FpaC_DISCRETE:
				case FpaC_LINE:
				case FpaC_SCATTERED:
				case FpaC_LCHAIN:
				default:
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot extract values from field ... %s %s",
							xsect_flds[nfld].element, xsect_flds[nfld].level);
						(void) fprintf(stdout,
							"  from %s at %s\n",
							CurSource, haxis->vtimes[nx]);
						}
					return TRUE;
				}

			/* Convert field value to required units */
			(void) convert_value(FpaCmksUnits, (double) vlist->val[0],
					units, &dval);
			(void) free_vlist(vlist);
			FREEMEM(vlist);

			/* Set cross section data for this location */
			GridVals[ny][nx] = (float) dval;
			}
		}

	/* Now create a surface object from the cross section data */
	sfc = create_surface();
	(void) grid_surface(sfc, 1.0, haxis->num, vaxis->num, GridVals);

	/* Set units specifications for the cross section surface object */
	udef = identify_unit(units);
	(void) define_uspec(&uspec, udef->name, udef->factor, udef->offset);
	(void) define_surface_units(sfc, &uspec);

	/* Clip the contours to the current cross section */
	clip_to_map = TRUE;

	/* Display contours as areas */
	if ( display_areas )
		{

		/* For a single contour area ...       */
		/*  only contour max and min are given */
		if ( blank(base) || blank(interval) )
			{

			if ( Verbose )
				{
				(void) fprintf(stdout,
						" Plotting contour area for %s to %s\n", min, max);
				(void) fprintf(stdout,
						"            for ... %s %s from %s at %s\n",
						xsect_flds[0].element, xsect_flds[0].level,
						CurSource, TVstamp);
				}

			/* Extract the contour minimum and maximum */
			(void) sscanf(min, "%f", &fminv);
			(void) sscanf(max, "%f", &fmaxv);

			/* Display the contour area */
			if ( !GRA_display_xsection_contour_area(sfc, cur_xsect,
					haxis, vaxis, fminv, fmaxv, units,
					interior_fill, sym_fill_name, pattern, pattern_width,
					pattern_length, comp_pres, num_comp, clip_to_map) )
				(void) error_report("Error displaying contour area ...");
			}

		/* For contour areas at set intervals ...               */
		/*  the contour min, max, base, and interval are given  */
		else
			{

			if ( Verbose )
				{
				(void) fprintf(stdout,
						" Plotting contour areas for %s to %s every %s",
						min, max, interval);
				(void) fprintf(stdout,
						" starting at %s\n", base);
				(void) fprintf(stdout,
						"            for ... %s %s from %s at %s\n",
						xsect_flds[0].element, xsect_flds[0].level,
						CurSource, TVstamp);
				}

			/* Extract the initial contour parameters */
			(void) sscanf(min,      "%f", &fminv);
			(void) sscanf(max,      "%f", &fmaxv);
			(void) sscanf(base,     "%f", &fbase);
			(void) sscanf(interval, "%f", &fint);

			/* Find the actual contours available in the given field */
			(void) find_surface_range(sfc, &vmin, &vmax);
			(void) convert_value(sfc->units.name, (double) vmin, units, &dval);
			vmin = (float) dval;
			(void) convert_value(sfc->units.name, (double) vmax, units, &dval);
			vmax = (float) dval;

			/* Reset the initial contour min and max, if required */
			if ( vmin > fminv ) fminv = vmin;
			if ( vmax < fmaxv ) fmaxv = vmax;

			/* Reset the minimum contour value, adjusted to the base value */
			diff   = fmod((fminv - fbase), fint);
			fminv -= diff;
			if ( diff <= 0 ) fminv -= fint;

			/* Reset the maximum contour value, adjusted to the base value */
			diff   = fmod((fmaxv - fbase), fint);
			fmaxv -= diff;
			if ( diff >= 0 ) fmaxv += fint;

			/* Display each contour area from min to max at each interval */
			fval = fminv;
			while ( fval < fmaxv )
				{

				/* Display the contour area */
				if ( !GRA_display_xsection_contour_area(sfc, cur_xsect,
						haxis, vaxis, fval, (fval + fint), units,
						interior_fill, sym_fill_name, pattern, pattern_width,
						pattern_length, comp_pres, num_comp, clip_to_map) )
					(void) error_report("Error displaying contour area ...");

				/* Increment the contour value */
				fval += fint;
				}
			}
		}

	/* Display contours as lines */
	else
		{

		/* For a list of contours ...                                  */
		/*  "values" contains a white-separated list of contour values */
		if ( !blank(values) )
			{

			if ( Verbose )
				{
				(void) fprintf(stdout,
						" Plotting contour(s) %s\n", values);
				(void) fprintf(stdout,
						"            for ... %s %s from %s at %s\n",
						xsect_flds[0].element, xsect_flds[0].level,
						CurSource, TVstamp);
				}

			/* Display one or more specific contours */
			while ( !blank(values) )
				{

				/* Extract next contour value in the list */
				fval = float_arg(values, &status);
				if ( !status ) (void) error_report("Error in contour values");

				/* Display the contour */
				if ( !GRA_display_xsection_contour_line(sfc, cur_xsect,
						haxis, vaxis, fval, units,
						pattern, pattern_width, pattern_length,
						comp_pres, num_comp, clip_to_map) )
					(void) error_report("Error displaying contour line ...");
				}
			}

		/* For a series of contours at set intervals ...       */
		/*  the contour min, max, base, and interval are given */
		else
			{

			if ( Verbose )
				{
				(void) fprintf(stdout,
						" Plotting contours from %s to %s every %s starting at %s\n",
						min, max, interval, base);
				(void) fprintf(stdout,
						"            for ... %s %s from %s at %s\n",
						xsect_flds[0].element, xsect_flds[0].level,
						CurSource, TVstamp);
				}

			/* Extract the initial contour parameters */
			(void) sscanf(min,      "%f", &fminv);
			(void) sscanf(max,      "%f", &fmaxv);
			(void) sscanf(base,     "%f", &fbase);
			(void) sscanf(interval, "%f", &fint);

			/* Find the actual contours available in the given field */
			(void) find_surface_range(sfc, &vmin, &vmax);
			(void) convert_value(sfc->units.name, (double) vmin, units, &dval);
			vmin = (float) dval;
			(void) convert_value(sfc->units.name, (double) vmax, units, &dval);
			vmax = (float) dval;

			/* Reset the initial contour min and max, if required */
			if ( vmin > fminv ) fminv = vmin;
			if ( vmax < fmaxv ) fmaxv = vmax;

			/* Reset the minimum contour value, adjusted to the base value */
			diff   = fmod((fminv - fbase), fint);
			fminv -= diff;
			if ( diff > 0 ) fminv += fint;

			/* Reset the maximum contour value, adjusted to the base value */
			diff   = fmod((fmaxv - fbase), fint);
			fmaxv -= diff;
			if ( diff < 0 ) fmaxv -= fint;

			/* Display each contour from min to max at each interval */
			fval = fminv;
			while ( fval <= fmaxv )
				{

				/* Display the contour */
				if ( !GRA_display_xsection_contour_line(sfc, cur_xsect,
						haxis, vaxis, fval, units,
						pattern, pattern_width, pattern_length,
						comp_pres, num_comp, clip_to_map) )
					(void) error_report("Error displaying contour line ...");

				/* Increment the contour value */
				fval += fint;
				}
			}
		}

	/* Free space used by SURFACE object and return TRUE */
	sfc = destroy_surface(sfc);
	return TRUE;
	}

LOGICAL			GRA_display_xsection_curves

	(
	STRING			xsection_name,	/* Cross section name */
	STRING			element,		/* Element name */
	STRING			level,			/* Level name */
	STRING			equation,		/* Equation to calculate */
	STRING			ver_units,		/* Units for sampled values */
	STRING			data_file,		/* Data file for parameters */
	STRING			data_file_format,	/* Format for data file */
	STRING			data_file_units,	/* Units for values in data file */
	STRING			cat_cascade,	/* And/Or for multiple CATATTRIB structs */
	CATATTRIB		*cat_attrib,	/* Structure containing categories */
	int				num_catatt,		/* Number of categories */
	STRING			attribute,		/* Attribute from field */
	STRING			loc_lookup,		/* Look up file for horizontal axis */
	GPGltype		ltype,			/* Type of locations for horizontal axis */
	int				num_xloc,		/* Number of locations for horizontal */
	XSECT_LOCATION	*xsect_locs,	/* Locations for horizontal axis */
	STRING			ver_lookup,		/* Look up file for vertical axis */
	float			proximity,		/* Proximity to area/curve/spot (in km) */
	STRING			display_func,	/* Display function for joining values */
	STRING			pattern,		/* Name of pattern for lines */
	STRING			pattern_width,	/* Pattern width factor */
	STRING			pattern_length,	/* Pattern repetition factor */
	COMP_PRES		*comp_pres,		/* Structure containing presentations */
	int				num_comp		/* Number of presentations */
	)

	{
	int				iloc;
	LOGICAL			clip_to_map, status_attribute;
	STRING			ident, vtime;
	float			flat, flon, yloc, xxl, yyl, xxc, yyc, xmid;
	POINT			pos, xpos;
	GRA_XSECT		*cur_xsect;
	XSECT_HOR_AXIS	*haxis;
	XSECT_VER_AXIS	*vaxis;
	char			err_buf[GPGLong];

	FLD_DESCRIPT	descript;

	/* Static buffer to hold cross section line segment */
	static	LINE	XSectLine = NullLine;

	/* Make a copy of the global field descriptor */
	(void) copy_fld_descript(&descript, &Fdesc);

	/* Re-initialize the field descriptor for this element and level */
	if ( !set_fld_descript(&descript, FpaF_DIRECTORY_PATH, FpaCblank,
									   FpaF_SOURCE_NAME,   CurSource,
									   FpaF_RUN_TIME,      FpaCblank,
									   FpaF_ELEMENT_NAME,  element,
									   FpaF_LEVEL_NAME,    level,
									   FpaF_VALID_TIME,    FpaCblank,
									   FpaF_END_OF_LIST) )
		{
		(void) sprintf(err_buf,
				" Error setting field descriptor for ... %s %s from %s\n",
				element, level, CurSource);
		(void) error_report(err_buf);
		}

	/* Find the named cross section */
	cur_xsect = get_cross_section(xsection_name);
	if ( IsNull(cur_xsect) )
		{
		(void) sprintf(err_buf, "Cross Section ... %s ... not yet defined",
				xsection_name);
		(void) error_report(err_buf);
		}

	/* Get horizontal axis parameters */
	haxis = cross_section_horizontal_axis(cur_xsect, loc_lookup, ltype,
											num_xloc, xsect_locs, NullDouble);
	if ( IsNull(haxis) )
		{
		(void) sprintf(err_buf,
				"Error in cross section horizontal look up ... %s", loc_lookup);
		(void) error_report(err_buf);
		}

	/* Get vertical axis parameters */
	vaxis = cross_section_vertical_axis(cur_xsect, ver_lookup, NullDouble);
	if ( IsNull(vaxis) )
		{
		(void) sprintf(err_buf,
				"Error in cross section vertical look up ... %s", ver_lookup);
		(void) error_report(err_buf);
		}

	/* Create or clear storage for cross section line segment */
	if ( IsNull(XSectLine) ) XSectLine = create_line();
	else                     (void) empty_line(XSectLine);

	/* Clip the final line to the current cross section */
	clip_to_map = TRUE;

	/* Sample fields for all cross section locations */
	xxl = yyl = 0.0;
	for ( iloc=0; iloc<haxis->num; iloc++ )
		{

		/* Set horizontal axis parameters for sampling */
		ident = haxis->idents[iloc];
		flat  = haxis->flats[iloc];
		flon  = haxis->flons[iloc];
		vtime = haxis->vtimes[iloc];

		/* Set map position for sampling */
		(void) ll_to_pos(&BaseMap, flat, flon, pos);
		if ( pos[X] < 0.0 || pos[Y] < 0.0
				|| pos[X] > BaseMap.definition.xlen
				|| pos[Y] > BaseMap.definition.ylen )
			{
			if ( Verbose )
				{
				(void) fprintf(stdout,
					"  Cross section lat/lon outside map at ... %.1f %.1f\n",
					flat, flon);
				}
			}

		/* Reset field descriptor for cross section valid time */
		if ( !set_fld_descript(&descript, FpaF_VALID_TIME, vtime,
										FpaF_END_OF_LIST) )
			{
			(void) sprintf(err_buf, "Error re-setting valid time ... %s\n",
					vtime);
			(void) error_report(err_buf);
			}

		/* Set vertical location of values from data file (if found) */
		if ( !blank(data_file) )
			{
			yloc = set_vertical_position_data_file(vaxis, data_file,
					data_file_format, data_file_units, ident, flat, flon,
					vtime, attribute, ver_units, &status_attribute);
			}

		/* Set vertical location of parameter (if found) */
		else
			{
			yloc = set_vertical_position(&descript, vaxis, pos,
					element, level, equation, ver_units, FpaCblank,
					cat_cascade, cat_attrib, num_catatt, attribute,
					FpaCblank, FpaCblank, proximity, &status_attribute);
			}

		/* Set cross section location for this value */
		xxc  = cur_xsect->x_off + (cur_xsect->width  * haxis->locs[iloc]);
		yyc  = cur_xsect->y_off + (cur_xsect->height * yloc);
		xmid = (xxc + xxl) / 2.0;

		/* Display segment of cross section line if parameter not found */
		if ( !status_attribute )
			{

			/* Continue if cross section line segment has no locations */
			if ( XSectLine->numpts < 1 )
				{
				xxl = xxc;
				yyl = yyc;
				continue;
				}

			/* Set end location on cross section line segment */
			/*  ... for linear or centred step display        */
			if ( same(display_func, XSectLineLinear)
					|| same(display_func, XSectLineStepCentre) )
				{

				/* Replicate last value on cross section line     */
				/*  to half way between last and current position */
				xpos[X] = xmid;		xpos[Y] = yyl;
				(void) add_point_to_line(XSectLine, xpos);
				}

			/* Set end location on cross section line segment */
			/*  ... for step before value display             */
			else if ( same(display_func, XSectLineStepBefore) )
				{

				/* No end locations required for step before! */
				}

			/* Set end location on cross section line segment */
			/*  ... for step after value display              */
			else if ( same(display_func, XSectLineStepAfter) )
				{

				/* Replicate last value on cross section line */
				/*  to current position                       */
				xpos[X] = xxc;		xpos[Y] = yyl;
				(void) add_point_to_line(XSectLine, xpos);
				}

			/* Now display cross section line segment */
			if ( !GRA_display_draw_line(XSectLine, FpaCblank, 0.0, 0.0,
					clip_to_map, pattern, pattern_width, pattern_length,
					comp_pres, num_comp) )
				{
				(void) fprintf(stdout, "Error drawing cross section curve\n");
				return FALSE;
				}

			/* Reset storage for cross section line segment */
			(void) empty_line(XSectLine);

			/* Then sample next location */
			xxl = xxc;
			yyl = yyc;
			continue;
			}

		/* Initialize a new cross section segment (if required) */
		if ( iloc > 0 && XSectLine->numpts < 1 )
			{

			/* Set start location on cross section line segment */
			/*  ... for linear display                          */
			if ( same(display_func, XSectLineLinear) )
				{

				/* Replicate first value on cross section line    */
				/*  to half way between last and current position */
				xpos[X] = xmid;		xpos[Y] = yyc;
				(void) add_point_to_line(XSectLine, xpos);
				}

			/* Set start location on cross section line segment */
			/*  ... for step display                            */
			else if ( same(display_func, XSectLineStepCentre)
						|| same(display_func, XSectLineStepBefore)
						|| same(display_func, XSectLineStepAfter) )
				{

				/* No start location required for step functions! */
				}
			}

		/* Add location to cross section line segment */
		/*  ... for linear display                    */
		if ( same(display_func, XSectLineLinear) )
			{

			/* Add current value to cross section line */
			xpos[X] = xxc;		xpos[Y] = yyc;
			(void) add_point_to_line(XSectLine, xpos);
			}

		/* Add location to cross section line segment */
		/*  ... for centred step display              */
		else if ( same(display_func, XSectLineStepCentre) )
			{

			/* Replicate last value on cross section line     */
			/*  to half way between last and current position */
			/* Note that cross section line will contain      */
			/*  points only if last value exists!             */
			if ( iloc > 0 && XSectLine->numpts > 0 )
				{
				xpos[X] = xmid;		xpos[Y] = yyl;
				(void) add_point_to_line(XSectLine, xpos);
				}

			/* Replicate current value on cross section line  */
			/*  to half way between last and current position */
			if ( iloc > 0 )
				{
				xpos[X] = xmid;		xpos[Y] = yyc;
				(void) add_point_to_line(XSectLine, xpos);
				}

			/* Add current value on cross section line */
			xpos[X] = xxc;		xpos[Y] = yyc;
			(void) add_point_to_line(XSectLine, xpos);
			}

		/* Add location to cross section line segment */
		/*  ... for step before value display         */
		else if ( same(display_func, XSectLineStepBefore) )
			{

			/* Replicate current value on cross section line */
			/*  to last position                             */
			if ( iloc > 0 )
				{
				xpos[X] = xxl;		xpos[Y] = yyc;
				(void) add_point_to_line(XSectLine, xpos);
				}

			/* Add current value on cross section line */
			xpos[X] = xxc;		xpos[Y] = yyc;
			(void) add_point_to_line(XSectLine, xpos);
			}

		/* Add location to cross section line segment */
		/*  ... for step after value display          */
		else if ( same(display_func, XSectLineStepAfter) )
			{

			/* Replicate last value on cross section line */
			/*  to current position                       */
			/* Note that cross section line will contain  */
			/*  points only if last value exists!         */
			if ( iloc > 0 && XSectLine->numpts > 0 )
				{
				xpos[X] = xxc;		xpos[Y] = yyl;
				(void) add_point_to_line(XSectLine, xpos);
				}

			/* Add current value on cross section line */
			xpos[X] = xxc;		xpos[Y] = yyc;
			(void) add_point_to_line(XSectLine, xpos);
			}

		/* Then sample next location */
		xxl = xxc;
		yyl = yyc;
		}

	/* Now display remaining cross section line segment */
	if ( !GRA_display_draw_line(XSectLine, FpaCblank, 0.0, 0.0,
			clip_to_map, pattern, pattern_width, pattern_length,
			comp_pres, num_comp) )
		{
		(void) fprintf(stdout, "Error drawing cross section curve\n");
		return FALSE;
		}

	/* Return TRUE */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    G R A _ d i s p l a y _ e l l i p s e                             *
*                                                                      *
***********************************************************************/

LOGICAL			GRA_display_ellipse

	(
	float		width,			/* Width of ellipse (display units) */
	float		height,			/* Height of ellipse (display units) */
	float		sangle,			/* Start angle of ellipse (degrees) */
	float		eangle,			/* End angle of ellipse (degrees) */
	LOGICAL		closed,			/* Draw closed ellipse? */
	float		rotation,		/* Ellipse rotation (degrees) */
	LOGICAL		rotate_lat,		/* Rotate ellipse to align with latitude? */
	LOGICAL		rotate_lon,		/* Rotate ellipse to align with longitude? */
	float		xoff,			/* x offset of center of ellipse */
								/*  (display units)              */
	float		yoff,			/* y offset of center of ellipse */
								/*  (display units)              */
	STRING		lat,			/* Latitude location for symbol */
	STRING		lon,			/* Longitude location for symbol */
	STRING		map_x,			/* x axis position for symbol */
	STRING		map_y,			/* y axis position for symbol */
	float		map_units,		/* Units for x and y axis */
	STRING		loc_ident,		/* Location identifier for symbol */
	STRING		loc_lookup,		/* Location look up table */
	STRING		vtime,			/* Valid time to match */
	STRING		table_name,		/* Table name for locations */
	STRING		grid_name,		/* Grid name for locations */
	STRING		list_name,		/* List name for locations */
	STRING		sym_fill_name	/* Symbol pattern for interior of box */
	)

	{
	int			iix, iiy, isite, iloc, iil;
	float		pscale, flat, flon, fact, rotadj;
	float		xxo, yyo, xx, yy, xxt, yyt, xxs, yys;
	STRING		loclat, loclon, vt;
	LOGICAL		status;
	POINT		pos;
	GRA_TABLE	*cur_table;
	GRA_GRID	*cur_grid;
	GRA_LIST	*cur_list;
	char		err_buf[GPGLong];

	/* Display ellipse for each table site */
	if ( !blank(table_name) )
		{

		/* Find the named table */
		cur_table = get_table(table_name);
		if ( IsNull(cur_table) )
			{
			(void) sprintf(err_buf, "Table ... %s ... not yet defined",
					table_name);
			(void) error_report(err_buf);
			}

		/* Display ellipse for all table sites */
		(void) write_graphics_comment("### Begin ellipse display");
		for ( isite=0; isite<cur_table->nsites; isite++ )
			{

			/* Set table location for ellipse display */
			xxt = cur_table->x_off;
			yyt = cur_table->y_off;
			if ( same(cur_table->type, TableCol) )
				yyt += cur_table->offset[isite];
			else if ( same(cur_table->type, TableRow) )
				xxt += cur_table->offset[isite];
			(void) anchored_location(ZeroPoint, xxt, yyt, &xxo, &yyo);

			/* Set offset location */
			xx = xxo + xoff;
			yy = yyo + yoff;

			/* Now draw the ellipse filled with symbols           */
			/*  ... centered on the anchored position and rotated */
			if ( !blank(sym_fill_name) )
				{

				/* First display the "filled" ellipse */
				(void) write_graphics_ellipse(xx, yy, width, height,
						sangle, eangle, closed, rotation, FALSE, TRUE);

				/* Then display the symbols */
				if ( !GRA_display_ellipse_symbol_fill(xx, yy, width, height,
						sangle, eangle, rotation, sym_fill_name) )
					return FALSE;

				/* Then display the ellipse "outline" */
				(void) write_graphics_ellipse(xx, yy, width, height,
						sangle, eangle, closed, rotation, TRUE, FALSE);
				}

			/* Or just draw the ellipse                           */
			/*  ... centered on the anchored position and rotated */
			else
				{
				(void) write_graphics_ellipse(xx, yy, width, height,
						sangle, eangle, closed, rotation, TRUE, TRUE);
				}
			}

		/* Return TRUE when all table sites have been displayed */
		(void) write_graphics_comment("### End ellipse display");
		return TRUE;
		}

	/* Display ellipse for each grid location */
	else if ( !blank(grid_name) )
		{

		/* Find the named grid */
		cur_grid = get_sample_grid(grid_name);
		if ( IsNull(cur_grid) )
			{
			(void) sprintf(err_buf, "Grid ... %s ... not yet defined",
					grid_name);
			(void) error_report(err_buf);
			}

		/* Display ellipse for all grid locations */
		(void) write_graphics_comment("### Begin ellipse display");
		for ( iiy=0; iiy<cur_grid->numy; iiy++ )
			for ( iix=0; iix<cur_grid->numx; iix++ )
				{

				/* Set latitude/longitude and position for display */
				flat = cur_grid->flats[iiy][iix];
				flon = cur_grid->flons[iiy][iix];
				(void) ll_to_pos(&BaseMap, flat, flon, pos);

				/* Check for grid location off the map */
				if ( pos[X] < 0.0 || pos[Y] < 0.0
						|| pos[X] > BaseMap.definition.xlen
						|| pos[Y] > BaseMap.definition.ylen )
					{

					if ( Verbose )
						{
						(void) fprintf(stdout,
								"  Grid lat/lon outside map at ... %.1f %.1f\n",
								flat, flon);
						}

					/* Continue for grid location off map */
					if ( AnchorToMap ) continue;
					}

				/* Comment for grid location on the map */
				else
					{
					if ( Verbose )
						{
						(void) fprintf(stdout,
								"  Grid lat/lon at ... %.1f %.1f\n",
								flat, flon);
						}
					}

				/* Set grid location on the current map */
				if ( AnchorToMap )
					{
					(void) anchored_location(pos, 0.0, 0.0, &xxo, &yyo);
					}

				/* Set display location to current anchor position */
				else
					{
					xxs = (float) iix * cur_grid->x_shift;
					yys = (float) iiy * cur_grid->y_shift;
					(void) anchored_location(ZeroPoint, xxs, yys, &xxo, &yyo);
					}

				/* Set offset location */
				xx = xxo + xoff;
				yy = yyo + yoff;

				/* Adjust ellipse size for perspective (if required) */
				if ( perspective_scale(&pscale) )
					{
					width  *= pscale;
					height *= pscale;
					}

				/* Set rotation from latitude or longitude */
				if ( AnchorToMap && rotate_lat )
					rotadj = wind_dir_xy(&BaseMap, flat, flon, 90.0);
				else if ( AnchorToMap && rotate_lon )
					rotadj = wind_dir_xy(&BaseMap, flat, flon,  0.0);
				else
					rotadj = 0.0;

				/* Add default rotation */
				rotadj += rotation;

				/* Now draw the ellipse filled with symbols           */
				/*  ... centered on the anchored position and rotated */
				if ( !blank(sym_fill_name) )
					{

					/* First display the "filled" ellipse */
					(void) write_graphics_ellipse(xx, yy, width, height,
							sangle, eangle, closed, rotadj, FALSE, TRUE);

					/* Then display the symbols */
					if ( !GRA_display_ellipse_symbol_fill(xx, yy, width, height,
							sangle, eangle, rotadj, sym_fill_name) )
						return FALSE;

					/* Then display the ellipse "outline" */
					(void) write_graphics_ellipse(xx, yy, width, height,
							sangle, eangle, closed, rotadj, TRUE, FALSE);
					}

				/* Or just draw the ellipse                           */
				/*  ... centered on the anchored position and rotated */
				else
					{
					(void) write_graphics_ellipse(xx, yy, width, height,
							sangle, eangle, closed, rotadj, TRUE, TRUE);
					}
				}

		/* Return TRUE when all grid locations have been displayed */
		(void) write_graphics_comment("### End ellipse display");
		return TRUE;
		}

	/* Display ellipse for each list location */
	else if ( !blank(list_name) )
		{

		/* Find the named list */
		cur_list = get_sample_list(list_name);
		if ( IsNull(cur_list) )
			{
			(void) sprintf(err_buf, "List ... %s ... not yet defined",
					list_name);
			(void) error_report(err_buf);
			}

		/* Initialize offsets for list locations */
		iil = 0;
		xxs = 0.0;
		yys = 0.0;

		/* Display ellipse for all list locations */
		(void) write_graphics_comment("### Begin ellipse display");
		for ( isite=0; isite<cur_list->num; isite++ )
			{

			/* Set latitude/longitude for display */
			if ( cur_list->usell[isite] )
				{
				flat   = cur_list->flats[isite];
				flon   = cur_list->flons[isite];
				}

			/* Get latitude/longitude for display from all locations */
			/*  in a location look up table                          */
			else if ( same_ic(cur_list->idents[isite], LocIdentAll)
						|| same_ic(cur_list->idents[isite], LocIdentAllVtime) )
				{
				if ( blank(loc_lookup) )
					(void) error_report("No location look up table");
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"   Display at all locations from look up ... %s\n",
							loc_lookup);
					}

				/* Set valid time to check */
				if ( same_ic(cur_list->idents[isite], LocIdentAllVtime) )
					vt = vtime;
				else
					vt = FpaCblank;

				/* Set display locations for all look up table locations */
				iloc = -1;
				while ( (iloc = next_location_lookup_line(loc_lookup, iloc, vt,
						NullStringPtr, &loclat, &loclon, NullStringPtr)) >= 0 )
					{

					/* Set latitude/longitude for display */
					flat = read_lat(loclat, &status);
					if ( !status )
						(void) error_report("Problem with location look up lat");
					flon = read_lon(loclon, &status);
					if ( !status )
						(void) error_report("Problem with location look up lon");

					/* Set map position for display */
					(void) ll_to_pos(&BaseMap, flat, flon, pos);

					/* Check for list location off the map */
					if ( pos[X] < 0.0 || pos[Y] < 0.0
							|| pos[X] > BaseMap.definition.xlen
							|| pos[Y] > BaseMap.definition.ylen )
						{

						if ( Verbose )
							{
							(void) fprintf(stdout,
									"  List lat/lon outside map at ... %.1f %.1f\n",
									flat, flon);
							}

						/* Continue for list location off map */
						if ( AnchorToMap ) continue;
						}

					/* Comment for list location on the map */
					else
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
									"  List lat/lon at ... %.1f %.1f\n",
									flat, flon);
							}
						}

					/* Set list location on the current map */
					if ( AnchorToMap )
						{
						(void) anchored_location(pos, 0.0, 0.0, &xxo, &yyo);
						}

					/* Set display location to current anchor position   */
					/*  and set offset for each successive list location */
					else
						{
						(void) anchored_location(ZeroPoint, xxs, yys,
								&xxo, &yyo);
						iil++;
						if ( cur_list->x_wrap > 1
									&& iil % cur_list->x_wrap == 0 )
							{
							xxs  = 0.0;
							yys += cur_list->y_shift;
							}
						else if ( cur_list->x_wrap > 1 )
							{
							xxs += cur_list->x_shift;
							}
						else if ( cur_list->y_wrap > 1
									&& iil % cur_list->y_wrap == 0 )
							{
							xxs += cur_list->x_shift;
							yys  = 0.0;
							}
						else if ( cur_list->y_wrap > 1 )
							{
							yys += cur_list->y_shift;
							}
						else
							{
							xxs += cur_list->x_shift;
							yys += cur_list->y_shift;
							}
						}

					/* Set offset location */
					xx = xxo + xoff;
					yy = yyo + yoff;

					/* Adjust ellipse size for perspective (if required) */
					if ( perspective_scale(&pscale) )
						{
						width  *= pscale;
						height *= pscale;
						}

					/* Set rotation from latitude or longitude */
					if ( AnchorToMap && rotate_lat )
						rotadj = wind_dir_xy(&BaseMap, flat, flon, 90.0);
					else if ( AnchorToMap && rotate_lon )
						rotadj = wind_dir_xy(&BaseMap, flat, flon,  0.0);
					else
						rotadj = 0.0;

					/* Add default rotation */
					rotadj += rotation;

					/* Now draw the ellipse filled with symbols           */
					/*  ... centered on the anchored position and rotated */
					if ( !blank(sym_fill_name) )
						{

						/* First display the "filled" ellipse */
						(void) write_graphics_ellipse(xx, yy, width, height,
								sangle, eangle, closed, rotadj, FALSE, TRUE);

						/* Then display the symbols */
						if ( !GRA_display_ellipse_symbol_fill(xx, yy, width,
								height, sangle, eangle, rotadj, sym_fill_name) )
							return FALSE;

						/* Then display the ellipse "outline" */
						(void) write_graphics_ellipse(xx, yy, width, height,
								sangle, eangle, closed, rotadj, TRUE, FALSE);
						}

					/* Or just draw the ellipse                           */
					/*  ... centered on the anchored position and rotated */
					else
						{
						(void) write_graphics_ellipse(xx, yy, width, height,
								sangle, eangle, closed, rotadj, TRUE, TRUE);
						}
					}

				/* Go on to next display location in sample list */
				continue;
				}

			/* Get latitude/longitude for display from location look up */
			else
				{
				if ( blank(loc_lookup) )
					(void) error_report("No location look up table");
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"   Matching location ident ... \"%s\"",
							cur_list->idents[isite]);
					(void) fprintf(stdout, "  from look up ... %s\n",
							loc_lookup);
					}
				if ( !match_location_lookup(loc_lookup,
						cur_list->idents[isite], vtime, &loclat, &loclon,
						NullStringPtr) )
					{
					(void) sprintf(err_buf,
							"Error matching \"%s\" in look up ... %s",
							cur_list->idents[isite], loc_lookup);
					(void) error_report(err_buf);
					}
				flat = read_lat(loclat, &status);
				if ( !status )
					(void) error_report("Problem with location look up lat");
				flon = read_lon(loclon, &status);
				if ( !status )
					(void) error_report("Problem with location look up lon");
				}

			/* Set map position for display */
			(void) ll_to_pos(&BaseMap, flat, flon, pos);

			/* Check for list location off the map */
			if ( pos[X] < 0.0 || pos[Y] < 0.0
					|| pos[X] > BaseMap.definition.xlen
					|| pos[Y] > BaseMap.definition.ylen )
				{

				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  List lat/lon outside map at ... %.1f %.1f\n",
							flat, flon);
					}

				/* Continue for list location off map */
				if ( AnchorToMap ) continue;
				}

			/* Comment for list location on the map */
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  List lat/lon at ... %.1f %.1f\n",
							flat, flon);
					}
				}

			/* Set list location on the current map */
			if ( AnchorToMap )
				{
				(void) anchored_location(pos, 0.0, 0.0, &xxo, &yyo);
				}

			/* Set display location to current anchor position   */
			/*  and set offset for each successive list location */
			else
				{
				(void) anchored_location(ZeroPoint, xxs, yys, &xxo, &yyo);
				iil++;
				if ( cur_list->x_wrap > 1 && iil % cur_list->x_wrap == 0 )
					{
					xxs  = 0.0;
					yys += cur_list->y_shift;
					}
				else if ( cur_list->x_wrap > 1 )
					{
					xxs += cur_list->x_shift;
					}
				else if ( cur_list->y_wrap > 1 && iil % cur_list->y_wrap == 0 )
					{
					xxs += cur_list->x_shift;
					yys  = 0.0;
					}
				else if ( cur_list->y_wrap > 1 )
					{
					yys += cur_list->y_shift;
					}
				else
					{
					xxs += cur_list->x_shift;
					yys += cur_list->y_shift;
					}
				}

			/* Set offset location */
			xx = xxo + xoff;
			yy = yyo + yoff;

			/* Adjust ellipse size for perspective (if required) */
			if ( perspective_scale(&pscale) )
				{
				width  *= pscale;
				height *= pscale;
				}

			/* Set rotation from latitude or longitude */
			if ( AnchorToMap && rotate_lat )
				rotadj = wind_dir_xy(&BaseMap, flat, flon, 90.0);
			else if ( AnchorToMap && rotate_lon )
				rotadj = wind_dir_xy(&BaseMap, flat, flon,  0.0);
			else
				rotadj = 0.0;

			/* Add default rotation */
			rotadj += rotation;

			/* Now draw the ellipse filled with symbols           */
			/*  ... centered on the anchored position and rotated */
			if ( !blank(sym_fill_name) )
				{

				/* First display the "filled" ellipse */
				(void) write_graphics_ellipse(xx, yy, width, height,
						sangle, eangle, closed, rotadj, FALSE, TRUE);

				/* Then display the symbols */
				if ( !GRA_display_ellipse_symbol_fill(xx, yy, width, height,
						sangle, eangle, rotadj, sym_fill_name) )
					return FALSE;

				/* Then display the ellipse "outline" */
				(void) write_graphics_ellipse(xx, yy, width, height,
						sangle, eangle, closed, rotadj, TRUE, FALSE);
				}

			/* Or just draw the ellipse                           */
			/*  ... centered on the anchored position and rotated */
			else
				{
				(void) write_graphics_ellipse(xx, yy, width, height,
						sangle, eangle, closed, rotadj, TRUE, TRUE);
				}
			}

		/* Return TRUE when all list locations have been displayed */
		(void) write_graphics_comment("### End ellipse display");
		return TRUE;
		}

	/* Display ellipse for all locations in a location look up table */
	else if ( !blank(loc_ident) && !blank(loc_lookup)
				&& ( same_ic(loc_ident, LocIdentAll)
						|| same_ic(loc_ident, LocIdentAllVtime) ) )
		{

		/* Ensure that ellipses can be displayed on current map */
		if ( !AnchorToMap )
			{
			(void) warn_report("Must set anchor to map!");
			return TRUE;
			}

		/* Set valid time to check */
		if ( same_ic(loc_ident, LocIdentAllVtime) ) vt = vtime;
		else                                        vt = FpaCblank;

		/* Display ellipse for all look up table locations */
		iloc = -1;
		(void) write_graphics_comment("### Begin ellipse display");
		while ( (iloc = next_location_lookup_line(loc_lookup, iloc, vt,
						NullStringPtr, &loclat, &loclon, NullStringPtr)) >= 0 )
			{

			/* Set latitude/longitude for display */
			flat = read_lat(loclat, &status);
			if ( !status )
				(void) error_report("Problem with location look up lat");
			flon = read_lon(loclon, &status);
			if ( !status )
				(void) error_report("Problem with location look up lon");

			/* Set map position for display */
			(void) ll_to_pos(&BaseMap, flat, flon, pos);

			/* Check for look up table location off the map */
			if ( pos[X] < 0.0 || pos[Y] < 0.0
					|| pos[X] > BaseMap.definition.xlen
					|| pos[Y] > BaseMap.definition.ylen )
				{

				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Lookup lat/lon outside map at ... %.1f %.1f\n",
							flat, flon);
					}

				/* Continue for look up table location off map */
				continue;
				}

			/* Comment for look up table location on the map */
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Lookup lat/lon at ... %.1f %.1f\n",
							flat, flon);
					}
				}

			/* Set offset position from map position */
			(void) anchored_location(pos, xoff, yoff, &xx, &yy);

			/* Adjust ellipse size for perspective (if required) */
			if ( perspective_scale(&pscale) )
				{
				width  *= pscale;
				height *= pscale;
				}

			/* Set rotation from latitude or longitude */
			if ( AnchorToMap && rotate_lat )
				rotadj = wind_dir_xy(&BaseMap, flat, flon, 90.0);
			else if ( AnchorToMap && rotate_lon )
				rotadj = wind_dir_xy(&BaseMap, flat, flon,  0.0);
			else
				rotadj = 0.0;

			/* Add default rotation */
			rotadj += rotation;

			/* Now draw the ellipse filled with symbols           */
			/*  ... centered on the anchored position and rotated */
			if ( !blank(sym_fill_name) )
				{

				/* First display the "filled" ellipse */
				(void) write_graphics_ellipse(xx, yy, width, height,
						sangle, eangle, closed, rotadj, FALSE, TRUE);

				/* Then display the symbols */
				if ( !GRA_display_ellipse_symbol_fill(xx, yy, width, height,
						sangle, eangle, rotadj, sym_fill_name) )
					return FALSE;

				/* Then display the ellipse "outline" */
				(void) write_graphics_ellipse(xx, yy, width, height,
						sangle, eangle, closed, rotadj, TRUE, FALSE);
				}

			/* Or just draw the ellipse                           */
			/*  ... centered on the anchored position and rotated */
			else
				{
				(void) write_graphics_ellipse(xx, yy, width, height,
						sangle, eangle, closed, rotadj, TRUE, TRUE);
				}
			}

		/* Return TRUE when all look up table locations have been displayed */
		(void) write_graphics_comment("### End ellipse display");
		return TRUE;
		}

	/* Display ellipse at a set location */
	else
		{

		/* Set an offset position based on latitude and longitude */
		if ( ( !blank(lat) && !blank(lon) )
				|| ( !blank(map_x) && !blank(map_y) )
				|| ( !blank(loc_ident) && !blank(loc_lookup) ) )
			{

			/* Ensure that ellipse can be displayed on current map */
			if ( !AnchorToMap )
				{
				(void) warn_report("Must set anchor to map!");
				return TRUE;
				}

			/* Set the latitude and longitude */
			if ( !blank(lat) && !blank(lon) )
				{
				flat = read_lat(lat, &status);
				if ( !status ) (void) error_report("Problem with lat");
				flon = read_lon(lon, &status);
				if ( !status ) (void) error_report("Problem with lon");

				/* Convert latitude and longitude to map position */
				(void) ll_to_pos(&BaseMap, flat, flon, pos);
				}

			/* Set the map position (adjusted by map_units) */
			else if ( !blank(map_x) && !blank(map_y) )
				{
				(void) sscanf(map_x, "%f", &pos[X]);
				(void) sscanf(map_y, "%f", &pos[Y]);
				fact = map_units / BaseMap.definition.units;
				pos[X] *= fact;
				pos[Y] *= fact;

				/* Convert map position to latitude and longitude */
				(void) pos_to_ll(&BaseMap, pos, &flat, &flon);
				}

			/* Get the latitude and longitude from location look up table */
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"   Matching location ident ... \"%s\"",
							loc_ident);
					(void) fprintf(stdout, "  from look up ... %s\n",
							loc_lookup);
					}
				if ( !match_location_lookup(loc_lookup, loc_ident,
						vtime, &loclat, &loclon, NullStringPtr) )
					{
					(void) sprintf(err_buf,
							"Error matching \"%s\" in look up ... %s",
							loc_ident, loc_lookup);
					(void) error_report(err_buf);
					}
				flat = read_lat(loclat, &status);
				if ( !status )
					(void) error_report("Problem with location look up lat");
				flon = read_lon(loclon, &status);
				if ( !status )
					(void) error_report("Problem with location look up lon");

				/* Convert latitude and longitude to map position */
				(void) ll_to_pos(&BaseMap, flat, flon, pos);
				}

			/* Return for map positions off the map */
			if ( pos[X] < 0.0 || pos[Y] < 0.0
					|| pos[X] > BaseMap.definition.xlen
					|| pos[Y] > BaseMap.definition.ylen )
				{
				if ( Verbose )
					{
					if ( !blank(map_x) && !blank(map_y) )
						(void) fprintf(stdout,
								"  Ellipse position off map at ... %f %f\n",
								pos[X], pos[Y]);
					else
						(void) fprintf(stdout,
								"  Ellipse lat/lon off map at ... %.1f %.1f\n",
								flat, flon);
					}

				return TRUE;
				}

			/* Comments for map positions on the map */
			else
				{
				if ( Verbose )
					{
					if ( !blank(map_x) && !blank(map_y) )
						(void) fprintf(stdout,
								"  Ellipse position at ... %f %f\n",
								pos[X], pos[Y]);
					else
						(void) fprintf(stdout,
								"  Ellipse lat/lon at ... %.1f %.1f\n",
								flat, flon);
					}
				}

			/* Set offset position from map position */
			(void) anchored_location(pos, xoff, yoff, &xx, &yy);
			}

		/* Set an absolute offset position */
		else
			{
			(void) anchored_location(ZeroPoint, xoff, yoff, &xx, &yy);
			}

		/* Adjust ellipse size for perspective (if required) */
		if ( perspective_scale(&pscale) )
			{
			width  *= pscale;
			height *= pscale;
			}

		/* Set rotation from latitude or longitude */
		if ( AnchorToMap && rotate_lat )
			rotadj = wind_dir_xy(&BaseMap, flat, flon, 90.0);
		else if ( AnchorToMap && rotate_lon )
			rotadj = wind_dir_xy(&BaseMap, flat, flon,  0.0);
		else
			rotadj = 0.0;

		/* Add default rotation */
		rotadj += rotation;

		(void) write_graphics_comment("### Begin ellipse display");

		/* Now draw the ellipse filled with symbols           */
		/*  ... centered on the anchored position and rotated */
		if ( !blank(sym_fill_name) )
			{

			/* First display the "filled" ellipse */
			(void) write_graphics_ellipse(xx, yy, width, height,
					sangle, eangle, closed, rotadj, FALSE, TRUE);

			/* Then display the symbols */
			if ( !GRA_display_ellipse_symbol_fill(xx, yy, width, height,
					sangle, eangle, rotadj, sym_fill_name) )
				return FALSE;

			/* Then display the ellipse "outline" */
			(void) write_graphics_ellipse(xx, yy, width, height,
					sangle, eangle, closed, rotadj, TRUE, FALSE);
			}

		/* Or just draw the ellipse                           */
		/*  ... centered on the anchored position and rotated */
		else
			{
			(void) write_graphics_ellipse(xx, yy, width, height,
					sangle, eangle, closed, rotadj, TRUE, TRUE);
			}

		/* Return TRUE when ellipse has been displayed */
		(void) write_graphics_comment("### End ellipse display");
		return	TRUE;
		}
	}

/***********************************************************************
*                                                                      *
*    G R A _ d i s p l a y _ g e o g r a p h y                         *
*                                                                      *
***********************************************************************/

LOGICAL			GRA_display_geography

	(
	STRING		geo_file,		/* Map file for geography */
	STRING		element,		/* Element name for geography field */
	STRING		level,			/* Level name for geography field */
	STRING		cat_cascade,	/* And/Or for multiple CATATTRIB structs */
	CATATTRIB	*cat_attrib,	/* Structure containing categories */
	int			num_catatt,		/* Number of categories */
	STRING		attribute		/* Attribute containing value to match */
	)

	{
	int			ii, ifld, ilist, isub;
	STRING		gfile, stype, value;
	LOGICAL		cur_anchor, clip_to_map;
	METAFILE	meta;
	FIELD		fld;
	SET			set;
	BOX			box;
	AREA		area;
	SUBAREA		subarea;
	LINE		line;
	CURVE		curve;
	PRES		geo_pres;
	char		geofile[GPGLong], out_buf[GPGLong], err_buf[GPGLong];

	/* First read the geography metafile */
	gfile = get_file("Maps", env_sub(geo_file));
	if ( IsNull(gfile) )
		{
		(void) sprintf(err_buf, "Problem finding map file ... %s", geo_file);
		(void) warn_report(err_buf);
		return TRUE;
		}
	(void) strcpy(geofile, gfile);
	meta = read_metafile(geofile, &BaseMap);
	if ( IsNull(meta) )
		{
		(void) sprintf(err_buf, "Problem reading map file ... %s", geofile);
		(void) warn_report(err_buf);
		return TRUE;
		}

	/* Check for recognized element and level parameters */
	if ( IsNull(identify_element(element))
			|| IsNull(identify_level(level)) )
		{
		(void) sprintf(err_buf,
				"Unrecognized element ... %s  or level ... %s  for map file... %s",
				element, level, geo_file);
		(void) warn_report(err_buf);
		return TRUE;
		}

	if ( Verbose )
		{
		(void) fprintf(stdout, " Searching for");
		for ( ii=0; ii<num_catatt; ii++ )
			(void) fprintf(stdout, " ... %s in %s",
					cat_attrib[ii].category, cat_attrib[ii].category_attribute);
		(void) fprintf(stdout, "\n");
		(void) fprintf(stdout, "    in map file ... %s\n", geofile);
		}

	/* Force all locations to be anchored to the current map */
	/*  ... regardless of the current anchor position!       */
	cur_anchor  = AnchorToMap;
	clip_to_map = TRUE;
	AnchorToMap = TRUE;

	/* Start grouping for geographic features */
	(void) sprintf(out_buf, "### Begin geography for ...  %s  at  %s",
			element, level);
	(void) write_graphics_comment(out_buf);
	for ( ii=0; ii<num_catatt; ii++ )
		{
		(void) sprintf(out_buf,
			"### Geography feature(s) ...  %s  -  %s",
				cat_attrib[ii].category_attribute, cat_attrib[ii].category);
		(void) write_graphics_comment(out_buf);
		}
	(void) write_graphics_group(GPGstart, NullPointer, 0);

	/* Repeat for each field in the METAFILE */
	for ( ifld=0; ifld<meta->numfld; ifld++ )
		{

		/* Extract the field */
		fld = meta->fields[ifld];
		if ( IsNull(fld) ) continue;

		/* Check for matching element and level */
		if ( !equivalent_element_definitions(element, fld->element)
				|| !equivalent_level_definitions(level, fld->level) ) continue;

		/* Field contains a surface */
		if ( fld->ftype == FtypeSfc )
			{
			}

		/* Field contains a plot */
		else if ( fld->ftype == FtypePlot )
			{
			}

		/* Field contains a set */
		else if ( fld->ftype == FtypeSet )
			{

			/* Extract the SET Object */
			set = fld->data.set;
			if ( IsNull(set) ) continue;
			if ( set->num <= 0 ) continue;

			/* Strip out all features that fall outside the display map */
			box.left   = 0.0;
			box.right  = BaseMap.definition.xlen;
			box.bottom = 0.0;
			box.top    = BaseMap.definition.ylen;
			(void) strip_set(set, &box);
			if ( set->num <= 0 ) continue;

			/* Determine type of Objects */
			(void) recall_set_type(set, &stype);

			/* Display AREA Objects */
			if ( same(stype, "area") )
				{
				for ( ilist=0; ilist<set->num; ilist++ )
					{

					/* Extract each AREA Object */
					area = (AREA) set->list[ilist];
					if ( IsNull(area) ) continue;

					/* Check areas with no subareas */
					if ( area->numdiv == 0 )
						{
						if ( IsNull(area->bound) ) continue;
						if ( IsNull(area->bound->boundary) ) continue;
						if ( area->bound->boundary->numpts < 2 ) continue;

						/* Check for areas that match desired categories  */
						/* Note that each category may contain more than  */
						/*  one member!                                   */
						if ( !match_category_attributes(area->attrib,
								cat_cascade, cat_attrib, num_catatt) ) continue;

						/* Find the named presentation                    */
						/* Note that the default presentation is returned */
						/*  if the named presentation is not found!       */
						value = CAL_get_attribute(area->attrib, attribute);
						geo_pres = get_geo_presentation(value);
						if ( Verbose )
							{
							(void) fprintf(stdout, "  Displaying ... %s",
									value);
							(void) fprintf(stdout, "   (%d  points)\n",
									area->bound->boundary->numpts);
							}

						/* Display boundary ... only simple lines allowed! */
						(void) GRA_display_simple_boundary(area->bound, 0.0,
								0.0, geo_pres, clip_to_map);
						}

					/* Check areas with subareas */
					else
						{

						/* Loop through subareas */
						for ( isub=0; isub<=area->numdiv; isub++ )
							{
							subarea = area->subareas[isub];
							if ( IsNull(subarea) ) continue;

							/* Check for subareas that match desired */
							/*  categories                           */
							/* Note that each category may contain   */
							/*  more than one member!                */
							if ( !match_category_attributes(subarea->attrib,
									cat_cascade, cat_attrib, num_catatt) )
																	continue;

							/* Construct the subarea outline */
							line = outline_from_subarea(subarea);
							if ( IsNull(line) ) continue;
							if ( line->numpts < 2 ) continue;

							/* Find the named presentation                    */
							/* Note that the default presentation is returned */
							/*  if the named presentation is not found!       */
							value = CAL_get_attribute(subarea->attrib,
										attribute);
							geo_pres = get_geo_presentation(value);
							if ( Verbose )
								{
								(void) fprintf(stdout, "  Displaying ... %s",
										value);
								(void) fprintf(stdout, "   (%d  points)\n",
										line->numpts);
								}

							/* Display outline ... only simple lines allowed! */
							(void) GRA_display_simple_outline(line, 0.0,
									0.0, geo_pres, clip_to_map);

							line = destroy_line(line);
							}
						}
					}
				}

			/* Display CURVE Objects */
			else if ( same(stype, "curve") )
				{
				for ( ilist=0; ilist<set->num; ilist++ )
					{

					/* Extract each CURVE Object */
					curve = (CURVE) set->list[ilist];
					if ( IsNull(curve) ) continue;
					if ( IsNull(curve->line) ) continue;
					if ( curve->line->numpts < 2 ) continue;

					/* Check for areas that match desired categories  */
					/* Note that each category may contain more than  */
					/*  one member!                                   */
					if ( !match_category_attributes(curve->attrib,
							cat_cascade, cat_attrib, num_catatt) ) continue;

					/* Find the named presentation                       */
					/* Note that the default presentation is returned if */
					/*  the named presentation is not found!             */
					value = CAL_get_attribute(curve->attrib, attribute);
					geo_pres = get_geo_presentation(value);
					if ( Verbose )
						{
						(void) fprintf(stdout, "  Displaying ... %s", value);
						(void) fprintf(stdout, "   (%d  points)\n",
								curve->line->numpts);
						}

					/* Display the line ... only simple lines allowed! */
					(void) GRA_display_simple_line(curve->line, 0.0, 0.0,
							geo_pres, clip_to_map);
					}
				}

			/* Labels are not displayed */
			else if ( same(stype, "label") )
				{
				}

			/* Marks are not displayed */
			else if ( same(stype, "mark") )
				{
				}

			/* Wind barbs are not displayed */
			else if ( same(stype, "barb") )
				{
				}

			/* Buttons are not displayed */
			else if ( same(stype, "button") )
				{
				}
			}
		}

	/* End grouping for geographic features */
	(void) write_graphics_group(GPGend, NullPointer, 0);
	(void) sprintf(out_buf, "### End geography for ...  %s  at  %s",
			element, level);
	(void) write_graphics_comment(out_buf);
	for ( ii=0; ii<num_catatt; ii++ )
		{
		(void) sprintf(out_buf,
			"### Geography feature(s) ...  %s  -  %s",
				cat_attrib[ii].category_attribute, cat_attrib[ii].category);
		(void) write_graphics_comment(out_buf);
		}

	/* Reset the current anchor */
	AnchorToMap = cur_anchor;

	/* Free space used by METAFILE Object and return TRUE */
	meta = destroy_metafile(meta);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    G R A _ d i s p l a y _ l a b e l                                 *
*                                                                      *
***********************************************************************/

LOGICAL			GRA_display_label

	(
	STRING			element,		/* Element name */
	STRING			level,			/* Level name */
	STRING			geo_file,		/* Map file for geography */
	STRING			cat_cascade,	/* And/Or for multiple CATATTRIB structs */
	CATATTRIB		*cat_attrib,	/* Structure containing categories */
	int				num_catatt,		/* Number of categories */
	ATTRIB_DISPLAY	*attribs,		/* Attributes to display */
	int				num_attrib,		/* Number of attributes */
	SPCASE			*list_case,		/* Structure for special cases */
	int				num_list,		/* Number of special cases */
	STRING			inmark,			/* Name of location marker */
	float			markscale,		/* Marker scaling factor (percent) */
	STRING			time_zone,		/* Time zone (used in strftime) */
	STRING			language,		/* Language (used in strftime) */
	STRING			display_name,	/* Name for label display */
	STRING			display_type,	/* Type of label display */
	LOGICAL			fit_to_map,		/* Fit display within map? */
	STRING			fit_to_map_ref,	/* Reference position for fit */
	float			rotation,		/* Label rotation (degrees) */
	LOGICAL			rotate_lat,		/* Rotate label to align with latitude? */
	LOGICAL			rotate_lon,		/* Rotate label to align with longitude? */
	STRING			rot_attrib,		/* Attribute to align label to */
	LOGICAL			constrain_rot,	/* Constrain rotation to display upright? */
	float			xshift,			/* x shift of label (off map) */
	float			yshift,			/* y shift of label (off map) */
	int				xwrap,			/* x count for list of labels (off map) */
	int				ywrap,			/* y count for list of labels (off map) */
	float			xdoff,			/* x offset of label and mark */
	float			ydoff,			/* y offset of label and mark */
	float			xoff,			/* x offset of label (display units) */
	float			yoff			/* y offset of label (display units) */
	)

	{
	int				ii, iil;
	float			xxs, yys, xxo, yyo, xx, yy;
	STRING			fname;
	METAFILE		meta;
	SPOT			spot;
	char			tbuf[GPGLong], sbuf[GPGLong];
	char			out_buf[GPGLong], err_buf[GPGLong];

	/* ObsoleteVersion - Force anchor to current map */
	LOGICAL			cur_anchor;

	FLD_DESCRIPT	descript;

	static	SET			spotset = NullSet;
	static	char		lastfname[GPGLong] = "";
	static	MAP_PROJ	lastmproj          = NO_MAPPROJ;

	/* Special check for geography file */
	if ( !blank(geo_file) )
		{

		/* Make a copy of the global field descriptor */
		(void) copy_fld_descript(&descript, &Fdesc);

		/* Re-initialize the field descriptor for this element and level */
		if ( !set_fld_descript(&descript, FpaF_DIRECTORY_PATH, FpaCblank,
										   FpaF_ELEMENT_NAME,  element,
										   FpaF_LEVEL_NAME,    level,
										   FpaF_END_OF_LIST) )
			{
			(void) sprintf(err_buf,
					" Error setting field descriptor for ... %s %s\n",
					element, level);
			(void) error_report(err_buf);
			}

		/* Check for geography file of this name */
		fname = get_file("Maps", env_sub(geo_file));
		if ( IsNull(fname) )
			{
			(void) sprintf(err_buf,
					"Problem finding map file ... %s", geo_file);
			(void) warn_report(err_buf);
			return TRUE;
			}

		(void) strcpy(tbuf, geo_file);
		(void) strcpy(sbuf, geo_file);
		}

	/* Special check for Scratchpad file */
	else if ( same_ic(element, FpaFile_Scratch) )
		{
		fname = source_path_by_name("Depict", NullString, NullString,
															FpaFile_Scratch);
		if ( blank(fname) )
			{
			(void) warn_report("*** Could not find Scratchpad!");
			return TRUE;
			}

		(void) strcpy(tbuf, FpaFile_Scratch);
		(void) strcpy(sbuf, FpaFile_Scratch);
		}

	/* Build filename for all other files */
	else
		{

		/* Make a copy of the global field descriptor */
		(void) copy_fld_descript(&descript, &Fdesc);

		/* Re-initialize the field descriptor for this element and level */
		if ( !set_fld_descript(&descript, FpaF_DIRECTORY_PATH, FpaCblank,
										   FpaF_SOURCE_NAME,   CurSource,
										   FpaF_RUN_TIME,      FpaCblank,
										   FpaF_ELEMENT_NAME,  element,
										   FpaF_LEVEL_NAME,    level,
										   FpaF_VALID_TIME,    TVstamp,
										   FpaF_END_OF_LIST) )
			{
			(void) sprintf(err_buf,
					" Error setting field descriptor for ... %s %s from %s at %s\n",
					element, level, CurSource, TVstamp);
			(void) error_report(err_buf);
			}

		/* Check for file of this name */
		fname = find_meta_filename(&descript);
		if ( blank(fname) )
			{
			if ( Verbose )
				{
				(void) fprintf(stdout,
						" No filename for ... %s %s from %s at %s\n",
						element, level, CurSource, TVstamp);
				}
			return TRUE;
			}

		(void) sprintf(tbuf,
				"%s %s from %s at %s", element, level, CurSource, TVstamp);
		(void) sprintf(sbuf, "%s  at  %s", element, level);
		}

	/* Read and extract SPOT Objects from named file */
	if ( !same(fname, lastfname)
			|| !equivalent_map_projection(&BaseMap, &lastmproj) )
		{

		/* Save the name and map projection for next call */
		(void) strcpy(lastfname, fname);
		(void) copy_map_projection(&lastmproj, &BaseMap);

		/* Read a new metafile */
		meta    = read_metafile(fname, &BaseMap);
		spotset = destroy_set(spotset);

		/* Return now if metafile is missing or empty */
		if ( IsNull(meta) )
			{
			if ( Verbose )
				{
				(void) fprintf(stdout, " Cannot find file ... %s\n", fname);
				(void) fprintf(stdout, "   for ... %s\n", tbuf);
				}
			return TRUE;
			}

		if ( Verbose )
			{
			(void) fprintf(stdout,
					" Extracting labels from file ... %s\n", fname);
			(void) fprintf(stdout,
					"   for ... %s\n", tbuf);
			}

		/* Extract SPOT Objects from Scratchpad */
		if ( same_ic(element, FpaFile_Scratch) )
			{
			spotset = take_mf_set(meta, "spot",  NullString, NullString,
																	NullString);
			}

		/* Extract SPOT Objects from other files */
		else
			{
			spotset = take_mf_set(meta, "spot",  NullString,
									descript.edef->name, descript.ldef->name);
			}

		/* Free space used by METAFILE Object */
		meta = destroy_metafile(meta);

		if ( IsNull(spotset) )
			{
			if ( Verbose )
				{
				(void) fprintf(stdout, " No spot labels for ... %s\n", tbuf);
				}
			return TRUE;
			}
		}

	/* ObsoleteVersion - Force all locations to be anchored to the  */
	/*  current map  ... regardless of the current anchor position! */
	if ( ObsoleteVersion )
		{
		cur_anchor  = AnchorToMap;
		AnchorToMap = TRUE;
		}

	/* Display SPOT Object labels */
	if ( NotNull(spotset) )
		{

		(void) sprintf(out_buf, "### Begin spot labels for ... %s ###", sbuf);
		(void) write_graphics_comment(out_buf);
		if ( Verbose )
			{
			(void) fprintf(stdout,
					" Displaying spot labels for ... %s\n", tbuf);
			}

		/* Initialize offsets for lists of labels */
		iil = 0;
		xxs = 0.0;
		yys = 0.0;

		/* Extract spot labels from spotset */
		for ( ii=0; ii<spotset->num; ii++ )
			{

			/* Check the spot label location */
			spot = (SPOT) spotset->list[ii];
			if ( spot->anchor[X] < 0.0 || spot->anchor[Y] < 0.0
					|| spot->anchor[X] > BaseMap.definition.xlen
					|| spot->anchor[Y] > BaseMap.definition.ylen )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Spot label off map at ... %.0f %.0f\n",
							spot->anchor[X], spot->anchor[Y]);
					}

				/* Continue (if requested) for map labels off map */
				if ( AnchorToMap ) continue;
				else if ( fit_to_map ) continue;
				}
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout, "  Spot label at ... %.0f %.0f\n",
							spot->anchor[X], spot->anchor[Y]);
					}
				}

			/* Check the spot attributes and matching category attributes */
			if ( !check_label_attributes(spot->attrib, attribs, num_attrib)
					|| !match_category_attributes(spot->attrib, cat_cascade,
							cat_attrib, num_catatt) ) continue;

			/* Set label location on the current map */
			if ( AnchorToMap )
				{
				(void) anchored_location(spot->anchor, 0.0, 0.0, &xxo, &yyo);
				}

			/* Set label location to current anchor position     */
			/*  and set offset for each successive label in list */
			else
				{
				(void) anchored_location(ZeroPoint, xxs, yys, &xxo, &yyo);
				iil++;
				if ( xwrap > 1 && iil % xwrap == 0 )
					{
					xxs  = 0.0;
					yys += yshift;
					}
				else if ( xwrap > 1 )
					{
					xxs += xshift;
					}
				else if ( ywrap > 1 && iil % ywrap == 0 )
					{
					xxs += xshift;
					yys  = 0.0;
					}
				else if ( ywrap > 1 )
					{
					yys += yshift;
					}
				else
					{
					xxs += xshift;
					yys += yshift;
					}
				}

			/* Set label location (offset by xdoff/ydoff) */
			/*  and offset location for label attributes  */
			xxo += xdoff;
			yyo += ydoff;
			xx = xxo + xoff;
			yy = yyo + yoff;

			/* Display the spot label containing all attributes */
			(void) GRA_display_sampled_attributes(FpaCblank, TVstamp,
					time_zone, language, FpaCblank, spot->anchor,
					DefProximity, DefBearing, DefLineLen, DefSegDir, DefSegSpd,
					spot->attrib, cat_cascade, cat_attrib, num_catatt,
					attribs, num_attrib, list_case, num_list,
					inmark, markscale, display_name, display_type, fit_to_map,
					fit_to_map_ref, rotation, rotate_lat, rotate_lon,
					rot_attrib, constrain_rot, xxo, yyo, xx, yy);
			}

		(void) sprintf(out_buf, "### End spot labels for ... %s ###", sbuf);
		(void) write_graphics_comment(out_buf);
		}

	/* ObsoleteVersion - Reset the current anchor */
	if ( ObsoleteVersion )
		{
		AnchorToMap = cur_anchor;
		}

	/* Return TRUE */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    G R A _ d i s p l a y _ p l o t                                   *
*                                                                      *
***********************************************************************/

LOGICAL			GRA_display_plot

	(
	STRING		element,	/* Element name */
	STRING		level,		/* Level name */
	STRING		intype,		/* Name of plot member */
	STRING		inmark,		/* Name of location marker */
	float		markscale,	/* Marker scaling factor (percent) */
	float		txt_size,	/* Size of text characters */
	float		rotation,	/* Text rotation (degrees) */
	float		xdoff,		/* x offset of label and mark */
	float		ydoff,		/* y offset of label and mark */
	float		xval,		/* x offset of label (display units */
	float		yval		/* y offset of label (display units */
	)

	{
	int			ii, num_sub;
	float		mscale, tsize, pscale, xxo, yyo, xx, yy, flat, flon;
	STRING		justified, fname;
	LOGICAL		cur_anchor;
	METAFILE	meta;
	PSUB		psub;
	char		tbuf[GPGMedium], err_buf[GPGLong];

	FLD_DESCRIPT	descript;

	static	PLOT		plot = NullPlot;
	static	char		lastfname[GPGLong] = "";
	static	MAP_PROJ	lastmproj          = NO_MAPPROJ;

	/* Set the justification */
	justified = CurPres.justified;

	/* Make a copy of the global field descriptor */
	(void) copy_fld_descript(&descript, &Fdesc);

	/* Re-initialize the field descriptor for this element and level */
	if ( !set_fld_descript(&descript, FpaF_DIRECTORY_PATH, FpaCblank,
									   FpaF_SOURCE_NAME,   CurSource,
									   FpaF_RUN_TIME,      FpaCblank,
									   FpaF_ELEMENT_NAME,  element,
									   FpaF_LEVEL_NAME,    level,
									   FpaF_VALID_TIME,    TVstamp,
									   FpaF_END_OF_LIST) )
		{
		(void) sprintf(err_buf,
				" Error setting field descriptor for ... %s %s from %s at %s\n",
				element, level, CurSource, TVstamp);
		(void) error_report(err_buf);
		}

	/* Check for file of this name */
	fname = find_meta_filename(&descript);
	if ( blank(fname) )
		{
		if ( Verbose )
			{
			(void) fprintf(stdout,
					" No filename for ... %s %s from %s at %s\n",
					element, level, CurSource, TVstamp);
			}
		return FALSE;
		}

	/* Read and extract PLOT Object from named file */
	/* Save the PLOT Object for subsequent calls    */
	if ( !same(fname, lastfname)
			|| !equivalent_map_projection(&BaseMap, &lastmproj) )
		{

		/* Save the name and map projection for next call */
		(void) strcpy(lastfname, fname);
		(void) copy_map_projection(&lastmproj, &BaseMap);

		/* Read the new metafile */
		meta = read_metafile(fname, &BaseMap);
		plot = destroy_plot(plot);

		/* Return now if metafile is empty */
		if ( IsNull(meta) )
			{
			if ( Verbose )
				{
				(void) fprintf(stdout, " Cannot find file ... %s\n", fname);
				(void) fprintf(stdout, "   for ... %s %s from %s at %s\n",
						element, level, CurSource, TVstamp);
				}
			return TRUE;
			}

		if ( Verbose )
			{
			(void) fprintf(stdout,
					" Extracting points from file ... %s\n", fname);
			(void) fprintf(stdout,
					"   for ... %s %s from %s at %s\n",
					element, level, CurSource, TVstamp);
			}

		/* Extract the PLOT Object */
		plot = take_mf_plot(meta, "d",
								descript.edef->name, descript.ldef->name);

		/* Free space used by METAFILE Object */
		meta = destroy_metafile(meta);

		if ( IsNull(plot) )
			{
			if ( Verbose )
				{
				(void) fprintf(stdout,
						" No plot objects for ... %s %s from %s at %s\n",
						element, level, CurSource, TVstamp);
				}
			return TRUE;
			}
		}

	/* Look for matching intype */
	for ( ii=0; ii<plot->nsubs; ii++ )
		{
		if ( same(plot->subs[ii].name, intype) )
			{
			num_sub = ii;
			break;
			}
		}
	if ( ii >= plot->nsubs )
		{
		if ( Verbose )
			{
			(void) fprintf(stdout,
					" Cannot find %s for ... %s %s from %s at %s\n",
					intype, element, level, CurSource, TVstamp);
			}
		return TRUE;
		}

	if ( Verbose )
		{
		(void) fprintf(stdout,
				" Displaying plot data for ... %s %s from %s at %s\n",
				element, level, CurSource, TVstamp);
		}

	/* Force all locations to be anchored to the current map */
	/*  ... regardless of the current anchor position!       */
	cur_anchor  = AnchorToMap;
	AnchorToMap = TRUE;

	/* Loop through all plot locations */
	for ( ii=0; ii<plot->numpts; ii++ )
		{

		/* Check for valid hi/lo symbols */
		psub = plot->subs[num_sub];
		if ( same (psub.sval1[ii], "!") ) continue;

		/* Check for plot locations off the map */
		if ( plot->pts[ii][X] <= 0.0 || plot->pts[ii][Y] <= 0.0
				|| plot->pts[ii][X] >= BaseMap.definition.xlen
				|| plot->pts[ii][Y] >= BaseMap.definition.ylen )
			{
			if ( Verbose )
				{
				(void) fprintf(stdout,
						"  Plot location off map at ... %.0f %.0f\n",
						plot->pts[ii][X], plot->pts[ii][Y]);
				}
			continue;
			}
		else
			{
			if ( Verbose )
				{
				(void) fprintf(stdout, "  Plot location at ... %.0f %.0f\n",
						plot->pts[ii][X], plot->pts[ii][Y]);
				}
			}

		/* Set plot location on the current map */
		(void) anchored_location(plot->pts[ii], 0.0, 0.0, &xxo, &yyo);

		/* Set plot location (offset by xdoff/ydoff) */
		/*  and offset location for parameters       */
		xxo += xdoff;
		yyo += ydoff;
		xx = xxo + xval;
		yy = yyo + yval;

		/* Adjust symbol scale and text size for perspective (if required) */
		mscale = markscale;
		tsize  = txt_size;
		if ( perspective_scale(&pscale) )
			{
			mscale *= pscale;
			tsize  *= pscale;
			}

		/* Display the mark (if requested) ... not offset! */
		if ( !blank(inmark) )
			{
			(void) write_graphics_symbol(inmark, xxo, yyo, mscale, rotation);
			}

		/* Display a plot label */
		if ( same(psub.type, "label") )
			{
			(void) write_graphics_text(psub.sval2[ii], xx, yy, tsize,
										justified, rotation, TRUE);
			}

		/* Display a plot barb */
		else if ( same(psub.type, "barb") )
			{
			(void) pos_to_ll(&BaseMap, plot->pts[ii], &flat, &flon);
			/* >>> get "gust" from PLOT object and add to call string <<< */
			/* >>> check that full windbarb will fit on map???        <<< */
			(void) GRA_display_windbarb(psub.fval1[ii], psub.fval2[ii], 0.0,
										flat, flon, xx, yy,
										Program.default_width_scale,
										Program.default_height_scale,
										rotation);
			}

		/* Display a plot integer */
		else if ( same(psub.type, "int") )
			{
			(void) sprintf(tbuf, "%d", psub.ival1[ii]);
			(void) write_graphics_text(tbuf, xx, yy, tsize,
										justified, rotation, TRUE);
			}

		/* Display a plot float */
		else if ( same(psub.type, "float") )
			{
			(void) sprintf(tbuf, "%f", psub.fval1[ii]);
			(void) write_graphics_text(tbuf, xx, yy, tsize,
										justified, rotation, TRUE);
			}
		}

	/* Reset the current anchor */
	AnchorToMap = cur_anchor;

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    G R A _ d i s p l a y _ t e x t                                   *
*                                                                      *
***********************************************************************/

LOGICAL			GRA_display_text

	(
	STRING		text_file,	/* File containing text to display */
	STRING		string,		/* Text string to display */
	STRING		attribute,	/* "Magic" attribute to display */
	STRING		format,		/* Format of "magic" attribute to display */
	STRING		conversion_format,
							/* Conversion format of "magic" attribute */
	float		txt_size,	/* Size of text characters */
	float		rotation,	/* Text rotation (degrees) */
	LOGICAL		rotate_lat,	/* Rotate text to align with latitude? */
	LOGICAL		rotate_lon,	/* Rotate text to align with longitude? */
	float		xoff,		/* x offset of text (display units) */
	float		yoff,		/* y offset of text (display units) */
	STRING		lat,		/* Latitude location for text */
	STRING		lon,		/* Longitude location for text */
	STRING		map_x,		/* x axis position for text */
	STRING		map_y,		/* y axis position for text */
	float		map_units,	/* Units for x and y axis */
	STRING		loc_ident,	/* Location identifier for text */
	STRING		loc_lookup,	/* Location look up table */
	STRING		vtime,		/* Valid time to match */
	STRING		table_name,	/* Table name for locations */
	STRING		grid_name,	/* Grid name for locations */
	STRING		list_name	/* List name for locations */
	)

	{
	int			iix, iiy, isite, iloc, iil;
	float		tsize, pscale, spacing, flat, flon, fact, rotadj;
	float		xxo, yyo, xx, yy, xxt, yyt, xxs, yys;
	STRING		justified, value, locid, loclat, loclon, loclab, vt;
	LOGICAL		status;
	POINT		pos;
	GRA_TABLE	*cur_table;
	GRA_GRID	*cur_grid;
	GRA_LIST	*cur_list;
	FILE		*fp;
	char		out_buf[GPGLong], err_buf[GPGLong];

	/* Set the justification */
	justified = CurPres.justified;

	/* Display text for table */
	if ( !blank(table_name) )
		{

		/* Find the named table */
		cur_table = get_table(table_name);
		if ( IsNull(cur_table) )
			{
			(void) sprintf(err_buf, "Table ... %s ... not yet defined",
					table_name);
			(void) error_report(err_buf);
			}

		/* Display text for each table site */
		for ( isite=0; isite<cur_table->nsites; isite++ )
			{

			/* Set table location for text output */
			xxt = cur_table->x_off;
			yyt = cur_table->y_off;
			if ( same(cur_table->type, TableCol) )
				yyt += cur_table->offset[isite];
			else if ( same(cur_table->type, TableRow) )
				xxt += cur_table->offset[isite];
			(void) anchored_location(ZeroPoint, xxt, yyt, &xxo, &yyo);

			/* Set offset location */
			xx = xxo + xoff;
			yy = yyo + yoff;

			/* Set text size */
			tsize = txt_size;

			/* Set space between lines of text from a file */
			(void) sscanf(CurPres.line_space, "%f", &spacing);
			spacing *= tsize / 100.0;

			/* Display text string */
			if ( !blank(string) )
				{
				(void) write_graphics_text(string, xx, yy, tsize,
											justified, rotation, TRUE);
				}

			/* Display text file */
			if ( !blank(text_file) )
				{
				/* Display text file line by line */
				if ( NotNull( fp = fopen(text_file, "r") ) )
					{

					/* Loop through each line of text file */
					while ( NotNull(getfileline(fp, out_buf, (size_t) GPGLong)) )
						{
						(void) write_graphics_text(out_buf, xx, yy, tsize,
													justified, rotation, TRUE);
						yy -= spacing;
						}
					}

				/* Warning if text file cannot be found */
				else
					{
					(void) sprintf(err_buf, "Cannot open text file ... %s",
							text_file);
					(void) warn_report(err_buf);
					}
				}

			/* Display "magic" attribute */
			if ( !blank(attribute) )
				{

				/* Check for "magic" attribute */
				if ( !magic_is_attribute(attribute) )
					{
					(void) sprintf(err_buf, "Attribute is not \"magic\": %s",
							attribute);
					(void) error_report(err_buf);
					}

				/* Display "magic" attribute */
				(void) copy_point(pos, ZeroPoint);
				value = magic_get_attribute(attribute, loc_ident, vtime,
											FpaCblank, FpaCblank, FpaCblank,
											pos, DefProximity,
											DefBearing, DefLineLen, DefSegDir,
											DefSegSpd, FpaCmksUnits,
											format, conversion_format);
				(void) write_graphics_text(value, xx, yy, tsize,
											justified, rotation, TRUE);
				}
			}

		/* Return TRUE when all table sites have been displayed */
		return TRUE;
		}

	/* Display text for each grid location */
	else if ( !blank(grid_name) )
		{

		/* Find the named grid */
		cur_grid = get_sample_grid(grid_name);
		if ( IsNull(cur_grid) )
			{
			(void) sprintf(err_buf, "Grid ... %s ... not yet defined",
					grid_name);
			(void) error_report(err_buf);
			}

		/* Display text for all grid locations */
		for ( iiy=0; iiy<cur_grid->numy; iiy++ )
			for ( iix=0; iix<cur_grid->numx; iix++ )
				{

				/* Set latitude/longitude and position for display */
				flat = cur_grid->flats[iiy][iix];
				flon = cur_grid->flons[iiy][iix];
				(void) ll_to_pos(&BaseMap, flat, flon, pos);

				/* Check for grid location off the map */
				if ( pos[X] < 0.0 || pos[Y] < 0.0
						|| pos[X] > BaseMap.definition.xlen
						|| pos[Y] > BaseMap.definition.ylen )
					{

					if ( Verbose )
						{
						(void) fprintf(stdout,
								"  Grid lat/lon outside map at ... %.1f %.1f\n",
								flat, flon);
						}

					/* Continue for grid location off map */
					if ( AnchorToMap ) continue;
					}

				/* Comment for grid location on the map */
				else
					{
					if ( Verbose )
						{
						(void) fprintf(stdout,
								"  Grid lat/lon at ... %.1f %.1f\n",
								flat, flon);
						}
					}

				/* Set grid location on the current map */
				if ( AnchorToMap )
					{
					(void) anchored_location(pos, 0.0, 0.0, &xxo, &yyo);
					}

				/* Set display location to current anchor position */
				else
					{
					xxs = (float) iix * cur_grid->x_shift;
					yys = (float) iiy * cur_grid->y_shift;
					(void) anchored_location(ZeroPoint, xxs, yys, &xxo, &yyo);
					}

				/* Set offset location */
				xx = xxo + xoff;
				yy = yyo + yoff;

				/* Adjust text size for perspective (if required) */
				tsize = txt_size;
				if ( perspective_scale(&pscale) ) tsize *= pscale;

				/* Set rotation from latitude or longitude */
				if ( AnchorToMap && rotate_lat )
					rotadj = wind_dir_xy(&BaseMap, flat, flon, 90.0);
				else if ( AnchorToMap && rotate_lon )
					rotadj = wind_dir_xy(&BaseMap, flat, flon,  0.0);
				else
					rotadj = 0.0;

				/* Add default rotation */
				rotadj += rotation;

				/* Set space between lines of text from a file */
				(void) sscanf(CurPres.line_space, "%f", &spacing);
				spacing *= tsize / 100.0;

				/* Display text string */
				if ( !blank(string) )
					{
					(void) write_graphics_text(string, xx, yy, tsize,
												justified, rotadj, TRUE);
					}

				/* Display text file */
				if ( !blank(text_file) )
					{
					/* Display text file line by line */
					if ( NotNull( fp = fopen(text_file, "r") ) )
						{

						/* Loop through each line of text file */
						while ( NotNull(getfileline(fp, out_buf, (size_t) GPGLong)) )
							{
							(void) write_graphics_text(out_buf, xx, yy, tsize,
													justified, rotadj, TRUE);
							yy -= spacing;
							}
						}

					/* Warning if text file cannot be found */
					else
						{
						(void) sprintf(err_buf, "Cannot open text file ... %s",
								text_file);
						(void) warn_report(err_buf);
						}
					}

				/* Display "magic" attribute */
				if ( !blank(attribute) )
					{

					/* Check for "magic" attribute */
					if ( !magic_is_attribute(attribute) )
						{
						(void) sprintf(err_buf, "Attribute is not \"magic\": %s",
								attribute);
						(void) error_report(err_buf);
						}

					/* Display "magic" attribute */
					value = magic_get_attribute(attribute, loc_ident, vtime,
												FpaCblank, FpaCblank, FpaCblank,
												pos, DefProximity,
												DefBearing, DefLineLen,
												DefSegDir, DefSegSpd, 
												FpaCmksUnits,
												format, conversion_format);
					(void) write_graphics_text(value, xx, yy, tsize,
												justified, rotadj, TRUE);
					}
				}

		/* Return TRUE when all grid locations have been displayed */
		return TRUE;
		}

	/* Display text for each list location */
	else if ( !blank(list_name) )
		{

		/* Find the named list */
		cur_list = get_sample_list(list_name);
		if ( IsNull(cur_list) )
			{
			(void) sprintf(err_buf, "List ... %s ... not yet defined",
					list_name);
			(void) error_report(err_buf);
			}

		/* Initialize offsets for list locations */
		iil = 0;
		xxs = 0.0;
		yys = 0.0;

		/* Display text for all list locations */
		for ( isite=0; isite<cur_list->num; isite++ )
			{

			/* Set latitude/longitude for display */
			if ( cur_list->usell[isite] )
				{
				flat   = cur_list->flats[isite];
				flon   = cur_list->flons[isite];
				loclab = FpaCblank;
				}

			/* Get latitude/longitude for display from all locations */
			/*  in a location look up table                          */
			else if ( same_ic(cur_list->idents[isite], LocIdentAll)
						|| same_ic(cur_list->idents[isite], LocIdentAllVtime) )
				{
				if ( blank(loc_lookup) )
					(void) error_report("No location look up table");
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"   Display at all locations from look up ... %s\n",
							loc_lookup);
					}

				/* Set valid time to check */
				if ( same_ic(cur_list->idents[isite], LocIdentAllVtime) )
					vt = vtime;
				else
					vt = FpaCblank;

				/* Set display locations for all look up table locations */
				iloc = -1;
				while ( (iloc = next_location_lookup_line(loc_lookup, iloc, vt,
									&locid, &loclat, &loclon, &loclab)) >= 0 )
					{

					/* Set latitude/longitude for display */
					flat = read_lat(loclat, &status);
					if ( !status )
						(void) error_report("Problem with location look up lat");
					flon = read_lon(loclon, &status);
					if ( !status )
						(void) error_report("Problem with location look up lon");

					/* Set map position for display */
					(void) ll_to_pos(&BaseMap, flat, flon, pos);

					/* Check for list location off the map */
					if ( pos[X] < 0.0 || pos[Y] < 0.0
							|| pos[X] > BaseMap.definition.xlen
							|| pos[Y] > BaseMap.definition.ylen )
						{

						if ( Verbose )
							{
							(void) fprintf(stdout,
									"  List lat/lon outside map at ... %.1f %.1f\n",
									flat, flon);
							}

						/* Continue for list location off map */
						if ( AnchorToMap ) continue;
						}

					/* Comment for list location on the map */
					else
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
									"  List lat/lon at ... %.1f %.1f\n",
									flat, flon);
							}
						}

					/* Set list location on the current map */
					if ( AnchorToMap )
						{
						(void) anchored_location(pos, 0.0, 0.0, &xxo, &yyo);
						}

					/* Set display location to current anchor position   */
					/*  and set offset for each successive list location */
					else
						{
						(void) anchored_location(ZeroPoint, xxs, yys,
								&xxo, &yyo);
						iil++;
						if ( cur_list->x_wrap > 1
									&& iil % cur_list->x_wrap == 0 )
							{
							xxs  = 0.0;
							yys += cur_list->y_shift;
							}
						else if ( cur_list->x_wrap > 1 )
							{
							xxs += cur_list->x_shift;
							}
						else if ( cur_list->y_wrap > 1
									&& iil % cur_list->y_wrap == 0 )
							{
							xxs += cur_list->x_shift;
							yys  = 0.0;
							}
						else if ( cur_list->y_wrap > 1 )
							{
							yys += cur_list->y_shift;
							}
						else
							{
							xxs += cur_list->x_shift;
							yys += cur_list->y_shift;
							}
						}

					/* Set offset location */
					xx = xxo + xoff;
					yy = yyo + yoff;

					/* Adjust text size for perspective (if required) */
					tsize = txt_size;
					if ( perspective_scale(&pscale) ) tsize *= pscale;

					/* Set rotation from latitude or longitude */
					if ( AnchorToMap && rotate_lat )
						rotadj = wind_dir_xy(&BaseMap, flat, flon, 90.0);
					else if ( AnchorToMap && rotate_lon )
						rotadj = wind_dir_xy(&BaseMap, flat, flon,  0.0);
					else
						rotadj = 0.0;

					/* Add default rotation */
					rotadj += rotation;

					/* Set space between lines of text from a file */
					(void) sscanf(CurPres.line_space, "%f", &spacing);
					spacing *= tsize / 100.0;

					/* Display text string */
					if ( !blank(string) )
						{
						(void) write_graphics_text(string, xx, yy, tsize,
													justified, rotadj, TRUE);
						}

					/* Display text file */
					if ( !blank(text_file) )
						{
						/* Display text file line by line */
						if ( NotNull( fp = fopen(text_file, "r") ) )
							{

							/* Loop through each line of text file */
							while ( NotNull(getfileline(fp, out_buf,
															(size_t) GPGLong)) )
								{
								(void) write_graphics_text(out_buf, xx, yy,
															tsize, justified,
															rotadj, TRUE);
								yy -= spacing;
								}
							}

						/* Warning if text file cannot be found */
						else
							{
							(void) sprintf(err_buf, "Cannot open text file ... %s",
									text_file);
							(void) warn_report(err_buf);
							}
						}

					/* Display "magic" attribute */
					if ( !blank(attribute) )
						{

						/* Check for "magic" attribute */
						if ( !magic_is_attribute(attribute) )
							{
							(void) sprintf(err_buf, "Attribute is not \"magic\": %s",
									attribute);
							(void) error_report(err_buf);
							}

						/* Display "magic" attribute */
						value = magic_get_attribute(attribute, locid, vtime,
													FpaCblank, FpaCblank,
													loclab, pos, DefProximity,
													DefBearing, DefLineLen,
													DefSegDir, DefSegSpd,
													FpaCmksUnits,
													format, conversion_format);
						(void) write_graphics_text(value, xx, yy, tsize,
													justified, rotadj, TRUE);
						}
					}

				/* Go on to next display location in sample list */
				continue;
				}

			/* Get latitude/longitude for display from location look up */
			else
				{
				if ( blank(loc_lookup) )
					(void) error_report("No location look up table");
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"   Matching location ident ... \"%s\"",
							cur_list->idents[isite]);
					(void) fprintf(stdout, "  from look up ... %s\n",
							loc_lookup);
					}
				if ( !match_location_lookup(loc_lookup,
						cur_list->idents[isite], vtime, &loclat, &loclon,
						&loclab) )
					{
					(void) sprintf(err_buf,
							"Error matching \"%s\" in look up ... %s",
							cur_list->idents[isite], loc_lookup);
					(void) error_report(err_buf);
					}
				flat = read_lat(loclat, &status);
				if ( !status )
					(void) error_report("Problem with location look up lat");
				flon = read_lon(loclon, &status);
				if ( !status )
					(void) error_report("Problem with location look up lon");
				}

			/* Set map position for display */
			(void) ll_to_pos(&BaseMap, flat, flon, pos);

			/* Check for list location off the map */
			if ( pos[X] < 0.0 || pos[Y] < 0.0
					|| pos[X] > BaseMap.definition.xlen
					|| pos[Y] > BaseMap.definition.ylen )
				{

				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  List lat/lon outside map at ... %.1f %.1f\n",
							flat, flon);
					}

				/* Continue for list location off map */
				if ( AnchorToMap ) continue;
				}

			/* Comment for list location on the map */
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  List lat/lon at ... %.1f %.1f\n",
							flat, flon);
					}
				}

			/* Set list location on the current map */
			if ( AnchorToMap )
				{
				(void) anchored_location(pos, 0.0, 0.0, &xxo, &yyo);
				}

			/* Set display location to current anchor position   */
			/*  and set offset for each successive list location */
			else
				{
				(void) anchored_location(ZeroPoint, xxs, yys, &xxo, &yyo);
				iil++;
				if ( cur_list->x_wrap > 1 && iil % cur_list->x_wrap == 0 )
					{
					xxs  = 0.0;
					yys += cur_list->y_shift;
					}
				else if ( cur_list->x_wrap > 1 )
					{
					xxs += cur_list->x_shift;
					}
				else if ( cur_list->y_wrap > 1 && iil % cur_list->y_wrap == 0 )
					{
					xxs += cur_list->x_shift;
					yys  = 0.0;
					}
				else if ( cur_list->y_wrap > 1 )
					{
					yys += cur_list->y_shift;
					}
				else
					{
					xxs += cur_list->x_shift;
					yys += cur_list->y_shift;
					}
				}

			/* Set offset location */
			xx = xxo + xoff;
			yy = yyo + yoff;

			/* Adjust text size for perspective (if required) */
			tsize = txt_size;
			if ( perspective_scale(&pscale) ) tsize *= pscale;

			/* Set rotation from latitude or longitude */
			if ( AnchorToMap && rotate_lat )
				rotadj = wind_dir_xy(&BaseMap, flat, flon, 90.0);
			else if ( AnchorToMap && rotate_lon )
				rotadj = wind_dir_xy(&BaseMap, flat, flon,  0.0);
			else
				rotadj = 0.0;

			/* Add default rotation */
			rotadj += rotation;

			/* Set space between lines of text from a file */
			(void) sscanf(CurPres.line_space, "%f", &spacing);
			spacing *= tsize / 100.0;

			/* Display text string */
			if ( !blank(string) )
				{
				(void) write_graphics_text(string, xx, yy, tsize,
											justified, rotadj, TRUE);
				}

			/* Display text file */
			if ( !blank(text_file) )
				{
				/* Display text file line by line */
				if ( NotNull( fp = fopen(text_file, "r") ) )
					{

					/* Loop through each line of text file */
					while ( NotNull(getfileline(fp, out_buf, (size_t) GPGLong)) )
						{
						(void) write_graphics_text(out_buf, xx, yy, tsize,
													justified, rotadj, TRUE);
						yy -= spacing;
						}
					}

				/* Warning if text file cannot be found */
				else
					{
					(void) sprintf(err_buf, "Cannot open text file ... %s",
							text_file);
					(void) warn_report(err_buf);
					}
				}

			/* Display "magic" attribute */
			if ( !blank(attribute) )
				{

				/* Check for "magic" attribute */
				if ( !magic_is_attribute(attribute) )
					{
					(void) sprintf(err_buf, "Attribute is not \"magic\": %s",
							attribute);
					(void) error_report(err_buf);
					}

				/* Display "magic" attribute */
				value = magic_get_attribute(attribute,
											cur_list->idents[isite], vtime,
											FpaCblank, FpaCblank, loclab,
											pos, DefProximity,
											DefBearing, DefLineLen,
											DefSegDir, DefSegSpd, FpaCmksUnits,
											format, conversion_format);
				(void) write_graphics_text(value, xx, yy, tsize,
											justified, rotadj, TRUE);
				}
			}

		/* Return TRUE when all list locations have been displayed */
		return TRUE;
		}

	/* Display text for all locations in a location look up table */
	else if ( !blank(loc_ident) && !blank(loc_lookup)
				&& ( same_ic(loc_ident, LocIdentAll)
						|| same_ic(loc_ident, LocIdentAllVtime) ) )
		{

		/* Ensure that text can be displayed on current map */
		if ( !AnchorToMap )
			{
			(void) warn_report("Must set anchor to map!");
			return TRUE;
			}

		/* Set valid time to check */
		if ( same_ic(loc_ident, LocIdentAllVtime) ) vt = vtime;
		else                                        vt = FpaCblank;

		/* Display text for all look up table locations */
		iloc = -1;
		while ( (iloc = next_location_lookup_line(loc_lookup, iloc, vt,
							&locid, &loclat, &loclon, &loclab)) >= 0 )
			{

			/* Set latitude/longitude for display */
			flat = read_lat(loclat, &status);
			if ( !status )
				(void) error_report("Problem with location look up lat");
			flon = read_lon(loclon, &status);
			if ( !status )
				(void) error_report("Problem with location look up lon");

			/* Set map position for display */
			(void) ll_to_pos(&BaseMap, flat, flon, pos);

			/* Check for look up table location off the map */
			if ( pos[X] < 0.0 || pos[Y] < 0.0
					|| pos[X] > BaseMap.definition.xlen
					|| pos[Y] > BaseMap.definition.ylen )
				{

				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Lookup lat/lon outside map at ... %.1f %.1f\n",
							flat, flon);
					}

				/* Continue for look up table location off map */
				continue;
				}

			/* Comment for look up table location on the map */
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Lookup lat/lon at ... %.1f %.1f\n",
							flat, flon);
					}
				}

			/* Set offset position from map position */
			(void) anchored_location(pos, xoff, yoff, &xx, &yy);

			/* Adjust text size for perspective (if required) */
			tsize = txt_size;
			if ( perspective_scale(&pscale) ) tsize *= pscale;

			/* Set rotation from latitude or longitude */
			if ( AnchorToMap && rotate_lat )
				rotadj = wind_dir_xy(&BaseMap, flat, flon, 90.0);
			else if ( AnchorToMap && rotate_lon )
				rotadj = wind_dir_xy(&BaseMap, flat, flon,  0.0);
			else
				rotadj = 0.0;

			/* Add default rotation */
			rotadj += rotation;

			/* Set space between lines of text from a file */
			(void) sscanf(CurPres.line_space, "%f", &spacing);
			spacing *= tsize / 100.0;

			/* Display text string */
			if ( !blank(string) )
				{
				(void) write_graphics_text(string, xx, yy, tsize,
											justified, rotadj, TRUE);
				}

			/* Display text file */
			if ( !blank(text_file) )
				{
				/* Display text file line by line */
				if ( NotNull( fp = fopen(text_file, "r") ) )
					{

					/* Loop through each line of text file */
					while ( NotNull(getfileline(fp, out_buf, (size_t) GPGLong)) )
						{
						(void) write_graphics_text(out_buf, xx, yy, tsize,
													justified, rotadj, TRUE);
						yy -= spacing;
						}
					}

				/* Warning if text file cannot be found */
				else
					{
					(void) sprintf(err_buf, "Cannot open text file ... %s",
							text_file);
					(void) warn_report(err_buf);
					}
				}

			/* Display "magic" attribute */
			if ( !blank(attribute) )
				{

				/* Check for "magic" attribute */
				if ( !magic_is_attribute(attribute) )
					{
					(void) sprintf(err_buf, "Attribute is not \"magic\": %s",
							attribute);
					(void) error_report(err_buf);
					}

				/* Display "magic" attribute */
				value = magic_get_attribute(attribute, locid, vtime,
											FpaCblank, FpaCblank, loclab,
											pos, DefProximity,
											DefBearing, DefLineLen, DefSegDir,
											DefSegSpd, FpaCmksUnits,
											format, conversion_format);
				(void) write_graphics_text(value, xx, yy, tsize,
											justified, rotadj, TRUE);
				}
			}

		/* Return TRUE when all look up table locations have been displayed */
		return TRUE;
		}

	/* Display text at a set location */
	else
		{

		/* Set an offset position based on latitude and longitude */
		if ( ( !blank(lat) && !blank(lon) )
				|| ( !blank(map_x) && !blank(map_y) )
				|| ( !blank(loc_ident) && !blank(loc_lookup) ) )
			{

			/* Ensure that text can be displayed on current map */
			if ( !AnchorToMap )
				{
				(void) warn_report("Must set anchor to map!");
				return TRUE;
				}

			/* Set the latitude and longitude */
			if ( !blank(lat) && !blank(lon) )
				{
				flat = read_lat(lat, &status);
				if ( !status ) (void) error_report("Problem with lat");
				flon = read_lon(lon, &status);
				if ( !status ) (void) error_report("Problem with lon");

				/* Convert latitude and longitude to map position */
				(void) ll_to_pos(&BaseMap, flat, flon, pos);
				loclab = FpaCblank;
				}

			/* Set the map position (adjusted by map_units) */
			else if ( !blank(map_x) && !blank(map_y) )
				{
				(void) sscanf(map_x, "%f", &pos[X]);
				(void) sscanf(map_y, "%f", &pos[Y]);
				fact = map_units / BaseMap.definition.units;
				pos[X] *= fact;
				pos[Y] *= fact;

				/* Convert map position to latitude and longitude */
				(void) pos_to_ll(&BaseMap, pos, &flat, &flon);
				loclab = FpaCblank;
				}

			/* Get the latitude and longitude from location look up table */
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"   Matching location ident ... \"%s\"",
							loc_ident);
					(void) fprintf(stdout, "  from look up ... %s\n",
							loc_lookup);
					}
				if ( !match_location_lookup(loc_lookup, loc_ident,
						vtime, &loclat, &loclon, &loclab) )
					{
					(void) sprintf(err_buf,
							"Error matching \"%s\" in look up ... %s",
							loc_ident, loc_lookup);
					(void) error_report(err_buf);
					}
				flat = read_lat(loclat, &status);
				if ( !status )
					(void) error_report("Problem with location look up lat");
				flon = read_lon(loclon, &status);
				if ( !status )
					(void) error_report("Problem with location look up lon");

				/* Convert latitude and longitude to map position */
				(void) ll_to_pos(&BaseMap, flat, flon, pos);
				}

			/* Return for map positions off the map */
			if ( pos[X] < 0.0 || pos[Y] < 0.0
					|| pos[X] > BaseMap.definition.xlen
					|| pos[Y] > BaseMap.definition.ylen )
				{
				if ( Verbose )
					{
					if ( !blank(map_x) && !blank(map_y) )
						(void) fprintf(stdout,
								"  Text position off map at ... %f %f\n",
								pos[X], pos[Y]);
					else
						(void) fprintf(stdout,
								"  Text lat/lon off map at ... %.1f %.1f\n",
								flat, flon);
					}

				return TRUE;
				}

			/* Comments for map positions on the map */
			else
				{
				if ( Verbose )
					{
					if ( !blank(map_x) && !blank(map_y) )
						(void) fprintf(stdout,
								"  Text position at ... %f %f\n",
								pos[X], pos[Y]);
					else
						(void) fprintf(stdout,
								"  Text lat/lon at ... %.1f %.1f\n",
								flat, flon);
					}
				}

			/* Set offset position from map position */
			(void) anchored_location(pos, xoff, yoff, &xx, &yy);
			}

		/* Set an absolute offset position */
		else
			{
			(void) anchored_location(ZeroPoint, xoff, yoff, &xx, &yy);
			}

		/* Adjust text size for perspective (if required) */
		tsize = txt_size;
		if ( perspective_scale(&pscale) ) tsize *= pscale;

		/* Set rotation from latitude or longitude */
		if ( AnchorToMap && rotate_lat )
			rotadj = wind_dir_xy(&BaseMap, flat, flon, 90.0);
		else if ( AnchorToMap && rotate_lon )
			rotadj = wind_dir_xy(&BaseMap, flat, flon,  0.0);
		else
			rotadj = 0.0;

		/* Add default rotation */
		rotadj += rotation;

		/* Set space between lines of text from a file */
		(void) sscanf(CurPres.line_space, "%f", &spacing);
		spacing *= tsize / 100.0;

		/* Display text string */
		if ( !blank(string) )
			{
			(void) write_graphics_text(string, xx, yy, tsize,
										justified, rotadj, TRUE);
			}

		/* Display text file */
		if ( !blank(text_file) )
			{
			/* Display text file line by line */
			if ( NotNull( fp = fopen(text_file, "r") ) )
				{

				/* Loop through each line of text file */
				while ( NotNull(getfileline(fp, out_buf, (size_t) GPGLong)) )
					{
					(void) write_graphics_text(out_buf, xx, yy, tsize,
												justified, rotadj, TRUE);
					yy -= spacing;
					}
				}

			/* Warning if text file cannot be found */
			else
				{
				(void) sprintf(err_buf, "Cannot open text file ... %s",
						text_file);
				(void) warn_report(err_buf);
				}
			}

		/* Display "magic" attribute */
		if ( !blank(attribute) )
			{

			/* Check for "magic" attribute */
			if ( !magic_is_attribute(attribute) )
				{
				(void) sprintf(err_buf, "Attribute is not \"magic\": %s",
						attribute);
				(void) error_report(err_buf);
				}

			/* Display "magic" attribute */
			value = magic_get_attribute(attribute, loc_ident, vtime,
										FpaCblank, FpaCblank, loclab,
										pos, DefProximity,
										DefBearing, DefLineLen, DefSegDir,
										DefSegSpd, FpaCmksUnits,
										format, conversion_format);
			(void) write_graphics_text(value, xx, yy, tsize,
										justified, rotadj, TRUE);
			}

		/* Return TRUE when string or file or attribute has been displayed */
		return	TRUE;
		}
	}

/***********************************************************************
*                                                                      *
*    G R A _ d i s p l a y _ t i m e                                   *
*                                                                      *
***********************************************************************/

LOGICAL			GRA_display_time

	(
	TTYPFMT		*type_fmt,	/* Structure containing type/format values */
	int			num_typfmt,	/* Number of type/format values */
	float		txt_size,	/* Size of text characters */
	float		rotation,	/* Label rotation (degrees) */
	float		xoff,		/* x offset of time (display units) */
	float		yoff		/* y offset of time (display units) */
	)

	{
	int			gyear, gjday, ghour, gmin, gsec=0;
	float		clon, tsize, pscale, xx, yy;
	STRING		xtype, vtime, xtime;
	time_t		itime;
	struct tm	*tm;
	int			nt;
	char		ttype[GPGMedium], ttadj[GPGMedium], tformat[GPGMedium];
	char		ztype[GPGMedium], tzone[GPGMedium], tlang[GPGMedium];
	char		tstring[GPGMedium], tfinal[GPGLong];
	char		env_tzone[GPGMedium], env_lang[GPGMedium];
	STRING		justified;
	char		err_buf[GPGLong];

	/* Static storage for environment variables */
	static	char	EnvTzone[GPGMedium], EnvLang[GPGLong];

	/* Set the justification */
	justified = CurPres.justified;

	/* Set centre longitude from current map projection */
	(void) grid_center(&BaseMap, NullPointPtr, NullFloat, &clon);

	/* Save current environment variables for time zone and language       */
	/* Note that once the environment variable has been stored with a call */
	/*  to putenv(), any change to the variable changes the environment!   */
	/* Note that changing these variables changes the environment! */
	(void) safe_strcpy(env_tzone, getenv("TZ"));
	(void) sprintf(EnvTzone, "TZ=%s",   env_tzone);
	(void) putenv(EnvTzone);
	(void) safe_strcpy(env_lang,  getenv("LANG"));
	(void) sprintf(EnvLang,  "LANG=%s", env_lang);
	(void) putenv(EnvLang);
	if ( Verbose )
		{
		(void) fprintf(stdout, " Original  Timezone ... %s  Language ... %s\n",
				env_tzone, env_lang);
		}

	/* Build formatted time string based on list of time parameters */
	(void) strcpy(ttype,   FpaCblank);
	(void) strcpy(tformat, FpaCblank);
	(void) strcpy(ztype,   FpaCblank);
	(void) strcpy(tzone,   FpaCblank);
	(void) strcpy(tlang,   FpaCblank);
	(void) strcpy(tfinal,  FpaCblank);
	for ( nt=0; nt<num_typfmt; nt++ )
		{

		/* Make a copy of the time parameters (if they exist) */
		if ( !blank(type_fmt[nt].time_type) )
			(void) strcpy(ttype, type_fmt[nt].time_type);
		if ( !blank(type_fmt[nt].time_format) )
			(void) strcpy(tformat, type_fmt[nt].time_format);
		if ( !blank(type_fmt[nt].zone_type) )
			(void) strcpy(ztype, type_fmt[nt].zone_type);
		if ( !blank(type_fmt[nt].time_zone) )
			(void) strcpy(tzone, type_fmt[nt].time_zone);
		if ( !blank(type_fmt[nt].language) )
			(void) strcpy(tlang, type_fmt[nt].language);

		/* Reset environment variables for time zone and language (if required) */
		if ( !blank(tzone) ) (void) sprintf(EnvTzone, "TZ=%s",   tzone);
		if ( !blank(tlang) ) (void) sprintf(EnvLang,  "LANG=%s", tlang);

		/* Set environment for call to strftime() */
		(void) set_locale_from_environment(LC_TIME);
		if ( Verbose )
			{
			(void) fprintf(stdout, " Temporary Timezone ... %s  Language ... %s\n",
					getenv("TZ"), getenv("LANG"));
			}

		/* Extract the date and time based on the appropriate time type */
		(void) strcpy(ttadj, ttype);
		xtype = string_arg(ttadj);
		if ( Verbose && !blank(ttadj) )
			{
			(void) fprintf(stdout, " Time Type ... %s  Adjustment ... %s\n",
					xtype, ttadj);
			}
		if ( same_ic(xtype, WriteTimeIssue) )
			{

			/* Convert local times to GMT (if required) */
			vtime = local_to_gmt(T0stamp, clon);
			if ( IsNull(vtime) )
				{
				(void) sprintf(err_buf, "Error reading issue time ... %s", T0stamp);
				(void) error_report(err_buf);
				}
			}
		else if ( same_ic(xtype, WriteTimeValid) )
			{

			/* Convert local times to GMT (if required) */
			vtime = local_to_gmt(TVstamp, clon);
			if ( IsNull(vtime) )
				{
				(void) sprintf(err_buf, "Error reading valid time ... %s", TVstamp);
				(void) error_report(err_buf);
				}
			}
		else if ( same_ic(xtype, WriteTimeCreation) )
			{

			/* Set the creation time ... which is ALWAYS in GMT */
			vtime = TCstamp;
			}

		/* Adjust the time (if required) */
		if ( !blank(ttadj) )
			{
			if ( Verbose )
				{
				(void) fprintf(stdout, " Time ... %s  Adjustment ... %s\n",
						vtime, ttadj);
				}
			xtime = interpret_timestring(ttadj, vtime, clon);
			if ( IsNull(xtime) )
				{
				(void) sprintf(err_buf,
						"Error adjusting time ... %s with %s", vtime, ttadj);
				(void) error_report(err_buf);
				}
			if ( Verbose )
				{
				(void) fprintf(stdout, " End time after adjustment ... %s\n", xtime);
				}
			}
		else
			{
			xtime = vtime;
			}

		/* Parse the adjusted time */
		(void) parse_tstamp(xtime, &gyear, &gjday, &ghour, &gmin,
												NullLogicalPtr, NullLogicalPtr);

		/* Now reformat the time based on the appropriate time zone type */
		itime = (time_t) encode_clock(gyear, gjday, ghour, gmin, gsec);
		if ( same_ic(ztype, WriteTimeGMT) )      tm = gmtime(&itime);
		else if ( same_ic(ztype, WriteTimeUTC) ) tm = gmtime(&itime);
		else if ( same_ic(ztype, WriteTimeLCL) ) tm = localtime(&itime);
		else if ( same_ic(ztype, WriteTimeLMT) ) tm = localtime(&itime);
		(void) strcpy(tstring,  FpaCblank);
		(void) strftime(tstring, GPGMedium, tformat, tm);

		/* Add the formatted time to the full string */
		(void) strcat(tfinal, tstring);

		/* Reset current environment variables for time zone and language */
		(void) sprintf(EnvTzone, "TZ=%s",   env_tzone);
		(void) sprintf(EnvLang,  "LANG=%s", env_lang);
		(void) reset_locale();

		if ( Verbose )
			{
			(void) fprintf(stdout, " Reset     Timezone ... %s  Language ... %s\n",
					getenv("TZ"), getenv("LANG"));
			}
		}

	/* Set the offset position */
	(void) anchored_location(ZeroPoint, xoff, yoff, &xx, &yy);

	/* Adjust text size for perspective (if required) */
	tsize = txt_size;
	if ( perspective_scale(&pscale) ) tsize *= pscale;

	/* Output the buffer */
	(void) write_graphics_text(tfinal, xx, yy, tsize,
			justified, rotation, TRUE);

	/* Return TRUE */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    G R A _ d e f i n e _ l i n e                                     *
*    G R A _ d r a w _ l i n e                                         *
*    G R A _ d r a w _ t a b l e _ l i n e                             *
*    G R A _ d r a w _ c r o s s _ s e c t i o n _ l i n e             *
*                                                                      *
***********************************************************************/

/* Storage for named lines */
static	int			NumGraLines = 0;
static	GRA_LINE	*GraLines   = NullPtr(GRA_LINE *);

LOGICAL			GRA_define_line

	(
	STRING		line_name,	/* line name */
	STRING		line		/* line specification */
	)

	{
	int			ii;
	LOGICAL		status;
	POINT		pos;

	/* Check list for line with this name */
	for ( ii=0; ii<NumGraLines; ii++ )
		{
		if ( same(line_name, GraLines[ii].name) )
			{

			if ( Verbose )
				{
				(void) fprintf(stdout, " Re-defining line ... %s\n", line_name);
				}

			/* Reinitialize the line */
			(void) empty_line(GraLines[ii].line);

			/* Add points to line */
			status = TRUE;
			while (status)
				{
				pos[X] = float_arg(line, &status);
				if ( !status ) break;
				pos[Y] = float_arg(line, &status);
				if ( !status )
					{
					(void) error_report ("Odd number of points in line!");
					}
				(void) add_point_to_line(GraLines[ii].line, pos);
				}
			break;
			}
		}

	/* Add another line to the list */
	if ( ii==NumGraLines )
		{

		if ( Verbose )
			{
			(void) fprintf(stdout, " Adding line ... %s\n", line_name);
			}

		/* Allocate space for another line */
		NumGraLines++;
		GraLines = GETMEM(GraLines, GRA_LINE, NumGraLines);
		GraLines[NumGraLines-1].line = create_line();

		/* Add points to line */
		status = TRUE;
		while (status)
			{
			pos[X] = float_arg(line, &status);
			if ( !status ) break;
			pos[Y] = float_arg(line, &status);
			if ( !status )
				{
				(void) error_report ("Odd number of points in line!");
				}
			(void) add_point_to_line(GraLines[NumGraLines-1].line, pos);
			}
		(void) strcpy(GraLines[NumGraLines-1].name, line_name);
		}

	/* Return TRUE if all went well */
	return TRUE;
	}

LOGICAL			GRA_draw_line

	(
	STRING		line_name,		/* Line name */
	STRING		arrow_name,		/* Arrow name (for ends of lines) */
	float		xoff,			/* x offset of line (display units) */
	float		yoff,			/* y offset of line (display units) */
	STRING		pattern,		/* Name of pattern for line */
	STRING		pattern_width,	/* Pattern width factor */
	STRING		pattern_length,	/* Pattern repetition factor */
	COMP_PRES	*comp_pres,		/* Structure containing presentations */
	int			num_comp		/* Number of presentations */
	)

	{
	int			iline;
	LOGICAL		clip_to_map;
	char		err_buf[GPGLong];

	/* Find the named line */
	for ( iline=0; iline<NumGraLines; iline++ )
		{
		if ( same(GraLines[iline].name, line_name) ) break;
		}
	if ( iline == NumGraLines )
		{
		(void) sprintf(err_buf, "Line name ... %s ... not yet defined",
				line_name);
		(void) error_report(err_buf);
		}

	/* Draw all lines regardless of current map coverage! */
	clip_to_map = FALSE;

	if ( Verbose )
		{
		(void) fprintf(stdout, " Drawing line ... %s\n", line_name);
		}

	/* Call general line display routine */
	return GRA_display_draw_line(GraLines[iline].line, arrow_name,
			xoff, yoff, clip_to_map, pattern, pattern_width, pattern_length,
			comp_pres, num_comp);
	}

LOGICAL			GRA_draw_table_line

	(
	STRING		table_name,		/* Table name */
	STRING		last_site,		/* Draw line for last site in table? */
	STRING		line_name,		/* Line name */
	STRING		arrow_name,		/* Arrow name (for ends of lines) */
	float		xoff,			/* x offset of line (display units) */
	float		yoff,			/* y offset of line (display units) */
	STRING		pattern,		/* Name of pattern for line */
	STRING		pattern_width,	/* Pattern width factor */
	STRING		pattern_length,	/* Pattern repetition factor */
	COMP_PRES	*comp_pres,		/* Structure containing presentations */
	int			num_comp		/* Number of presentations */
	)

	{
	int			iline, nsites, isite;
	float		xofft, yofft;
	LOGICAL		clip_to_map;
	GRA_TABLE	*cur_table;
	char		err_buf[GPGLong];

	/* Find the named table */
	cur_table = get_table(table_name);
	if ( IsNull(cur_table) )
		{
		(void) sprintf(err_buf, "Table ... %s ... not yet defined",
				table_name);
		(void) error_report(err_buf);
		}

	/* Find the named line */
	for ( iline=0; iline<NumGraLines; iline++ )
		{
		if ( same(GraLines[iline].name, line_name) ) break;
		}
	if ( iline == NumGraLines )
		{
		(void) sprintf(err_buf, "Line name ... %s ... not yet defined",
				line_name);
		(void) error_report(err_buf);
		}

	if ( Verbose )
		{
		(void) fprintf(stdout, " Drawing line(s) ... %s", line_name);
		(void) fprintf(stdout, "   for table ... %s\n", table_name);
		}

	/* Draw all lines regardless of current map coverage! */
	clip_to_map = FALSE;

	/* Draw line for each table site (but skip last site if requested) */
	nsites = cur_table->nsites;
	if ( same_ic(last_site, "no") ) nsites--;
	for ( isite=0; isite<nsites; isite++ )
		{

		/* Set the table offsets based on the table type */
		if ( same(cur_table->type, TableCol) )
			{
			xofft = cur_table->x_off + xoff;
			yofft = cur_table->y_off + cur_table->offset[isite] + yoff;
			}
		else if ( same(cur_table->type, TableRow) )
			{
			xofft = cur_table->x_off + cur_table->offset[isite] + xoff;
			yofft = cur_table->y_off + yoff;
			}

		/* Call general line display routine */
		if ( !GRA_display_draw_line(GraLines[iline].line, arrow_name,
				xofft, yofft, clip_to_map, pattern, pattern_width,
				pattern_length, comp_pres, num_comp) ) return FALSE;
		}

	/* Return TRUE */
	return TRUE;
	}

LOGICAL			GRA_draw_cross_section_line

	(
	STRING			xsection_name,	/* Cross section name */
	STRING			line_to_draw,	/* Type of cross section line to draw */
	STRING			axis_display,	/* Axis of cross section to display */
	STRING			loc_lookup,		/* Look up file for horizontal axis */
	GPGltype		ltype,			/* Type of locations for horizontal axis */
	int				num_xloc,		/* Number of locations for horizontal */
	XSECT_LOCATION	*xsect_locs,	/* Locations for horizontal axis */
	STRING			ver_lookup,		/* Look up file for vertical axis */
	STRING			line_name,		/* Line name */
	STRING			arrow_name,		/* Arrow name (for ends of lines) */
	float			xoff,			/* x offset of line (display units) */
	float			yoff,			/* y offset of line (display units) */
	STRING			pattern,		/* Name of pattern for line */
	STRING			pattern_width,	/* Pattern width factor */
	STRING			pattern_length,	/* Pattern repetition factor */
	COMP_PRES		*comp_pres,		/* Structure containing presentations */
	int				num_comp		/* Number of presentations */
	)

	{
	int				ii, iline, nlast;
	float			xshift, yshift;
	LOGICAL			clip_to_map;
	POINT			pos;
	GRA_XSECT		*cur_xsect;
	XSECT_HOR_AXIS	*haxis;
	XSECT_VER_AXIS	*vaxis;
	char			err_buf[GPGLong];

	/* Static buffer to hold cross section axis line */
	static	LINE	AxisLine = NullLine;

	/* Find the named cross section */
	cur_xsect = get_cross_section(xsection_name);
	if ( IsNull(cur_xsect) )
		{
		(void) sprintf(err_buf, "Cross Section ... %s ... not yet defined",
				xsection_name);
		(void) error_report(err_buf);
		}

	if ( Verbose )
		{
		(void) fprintf(stdout, " Drawing lines for cross section ... %s\n",
				xsection_name);
		}

	/* Create or clear the cross section axis line */
	if ( IsNull(AxisLine) ) AxisLine = create_line();
	else                    (void) empty_line(AxisLine);

	/* Draw all lines regardless of current map coverage! */
	clip_to_map = FALSE;

	/* Draw a cross section axis */
	if ( same(line_to_draw, XSectAxis) )
		{

		if ( Verbose )
			{
			(void) fprintf(stdout, "  Drawing axis ... %s\n", axis_display);
			}

		/* Draw a horizontal cross section axis */
		if ( same(axis_display, XSectAxisLower)
				|| same(axis_display, XSectAxisUpper) )
			{

			/* Get horizontal axis parameters */
			haxis = cross_section_horizontal_axis(cur_xsect, loc_lookup, ltype,
											num_xloc, xsect_locs, NullDouble);
			if ( IsNull(haxis) )
				{
				(void) sprintf(err_buf,
						"Error in cross section horizontal look up ... %s",
						loc_lookup);
				(void) error_report(err_buf);
				}

			/* Set start and end points on the horizontal cross section axis */
			nlast = haxis->num - 1;
			pos[X] = cur_xsect->width * haxis->locs[0];      pos[Y] = 0.0;
			(void) add_point_to_line(AxisLine, pos);
			pos[X] = cur_xsect->width * haxis->locs[nlast];  pos[Y] = 0.0;
			(void) add_point_to_line(AxisLine, pos);

			/* Set horizontal offset for horizontal axis */
			xshift = cur_xsect->x_off;

			/* Set vertical offset for horizontal axis */
			if ( same(axis_display, XSectAxisLower) )
				yshift = cur_xsect->y_off;
			else if ( same(axis_display, XSectAxisUpper) )
				yshift = cur_xsect->y_off + cur_xsect->height;

			/* Display the horizontal cross section axis */
			if ( !GRA_display_draw_line(AxisLine, arrow_name,
					(xoff + xshift), (yoff + yshift),
					clip_to_map, pattern, pattern_width, pattern_length,
					comp_pres, num_comp) ) return FALSE;
			}

		/* Display a vertical cross section axis */
		else if ( same(axis_display, XSectAxisLeft)
				|| same(axis_display, XSectAxisRight) )
			{

			/* Get vertical axis parameters */
			vaxis = cross_section_vertical_axis(cur_xsect, ver_lookup,
																	NullDouble);
			if ( IsNull(vaxis) )
				{
				(void) sprintf(err_buf,
						"Error in cross section vertical look up ... %s",
						ver_lookup);
				(void) error_report(err_buf);
				}

			/* Set start and end points on the vertical cross section axis */
			nlast = vaxis->num - 1;
			pos[X] = 0.0;  pos[Y] = cur_xsect->height * vaxis->locs[0];
			(void) add_point_to_line(AxisLine, pos);
			pos[X] = 0.0;  pos[Y] = cur_xsect->height * vaxis->locs[nlast];
			(void) add_point_to_line(AxisLine, pos);

			/* Set horizontal offset for vertical axis */
			if ( same(axis_display, XSectAxisLeft) )
				xshift = cur_xsect->x_off;
			else if ( same(axis_display, XSectAxisRight) )
				xshift = cur_xsect->x_off + cur_xsect->width;

			/* Set vertical offset for vertical axis */
			yshift = cur_xsect->y_off;

			/* Display the vertical cross section axis */
			if ( !GRA_display_draw_line(AxisLine, arrow_name,
					(xoff + xshift), (yoff + yshift),
					clip_to_map, pattern, pattern_width, pattern_length,
					comp_pres, num_comp) ) return FALSE;
			}
		}

	/* Draw tick marks on a cross section axis */
	else if ( same(line_to_draw, XSectTicks) )
		{

		if ( Verbose )
			{
			(void) fprintf(stdout, "  Drawing tick marks on axis ... %s\n",
					axis_display);
			}

		/* Find the named line */
		for ( iline=0; iline<NumGraLines; iline++ )
			{
			if ( same(GraLines[iline].name, line_name) ) break;
			}
		if ( iline == NumGraLines )
			{
			(void) sprintf(err_buf, "Line name ... %s ... not yet defined",
					line_name);
			(void) error_report(err_buf);
			}

		/* Display tick marks on a horizontal cross section axis */
		if ( same(axis_display, XSectAxisLower)
				|| same(axis_display, XSectAxisUpper) )
			{

			/* Get horizontal axis parameters */
			haxis = cross_section_horizontal_axis(cur_xsect, loc_lookup, ltype,
											num_xloc, xsect_locs, NullDouble);
			if ( IsNull(haxis) )
				{
				(void) sprintf(err_buf,
						"Error in cross section horizontal look up ... %s",
						loc_lookup);
				(void) error_report(err_buf);
				}

			/* Set vertical offset for horizontal axis */
			if ( same(axis_display, XSectAxisLower) )
				yshift = cur_xsect->y_off;
			else if ( same(axis_display, XSectAxisUpper) )
				yshift = cur_xsect->y_off + cur_xsect->height;

			/* Loop through locations on horizontal axis */
			for ( ii=0; ii<haxis->num; ii++ )
				{

				/* Set horizontal offset */
				xshift = cur_xsect->x_off + cur_xsect->width * haxis->locs[ii];

				/* Display tick marks on the horizontal axis */
				if ( !GRA_display_draw_line(GraLines[iline].line,
						arrow_name, (xoff + xshift), (yoff + yshift),
						clip_to_map, pattern, pattern_width, pattern_length,
						comp_pres, num_comp) ) return FALSE;
				}
			}

		/* Display tick marks on a vertical cross section axis */
		else if ( same(axis_display, XSectAxisLeft)
				|| same(axis_display, XSectAxisRight) )
			{

			/* Get vertical axis parameters */
			vaxis = cross_section_vertical_axis(cur_xsect, ver_lookup,
																	NullDouble);
			if ( IsNull(vaxis) )
				{
				(void) sprintf(err_buf,
						"Error in cross section vertical look up ... %s",
						ver_lookup);
				(void) error_report(err_buf);
				}

			/* Set horizontal offset for vertical axis */
			if ( same(axis_display, XSectAxisLeft) )
				xshift = cur_xsect->x_off;
			else if ( same(axis_display, XSectAxisRight) )
				xshift = cur_xsect->x_off + cur_xsect->width;

			/* Loop through locations on vertical axis */
			for ( ii=0; ii<vaxis->num; ii++ )
				{

				/* Set vertical offset */
				yshift = cur_xsect->y_off + cur_xsect->height * vaxis->locs[ii];

				/* Display tick marks on the vertical axis */
				if ( !GRA_display_draw_line(GraLines[iline].line,
						arrow_name, (xoff + xshift), (yoff + yshift),
						clip_to_map, pattern, pattern_width, pattern_length,
						comp_pres, num_comp) ) return FALSE;
				}
			}
		}

	/* Draw horizontal lines on a cross section */
	else if ( same(line_to_draw, XSectHLines) )
		{

		if ( Verbose )
			{
			(void) fprintf(stdout, "  Drawing horizontal lines\n");
			}

		/* Get horizontal axis parameters */
		haxis = cross_section_horizontal_axis(cur_xsect, loc_lookup, ltype,
											num_xloc, xsect_locs, NullDouble);
		if ( IsNull(haxis) )
			{
			(void) sprintf(err_buf,
					"Error in cross section horizontal look up ... %s",
					loc_lookup);
			(void) error_report(err_buf);
			}

		/* Get vertical axis parameters */
		vaxis = cross_section_vertical_axis(cur_xsect, ver_lookup, NullDouble);
		if ( IsNull(vaxis) )
			{
			(void) sprintf(err_buf,
					"Error in cross section vertical look up ... %s",
					ver_lookup);
			(void) error_report(err_buf);
			}

		/* Set start and end points on the horizontal cross section axis */
		nlast = haxis->num - 1;
		pos[X] = cur_xsect->width * haxis->locs[0];      pos[Y] = 0.0;
		(void) add_point_to_line(AxisLine, pos);
		pos[X] = cur_xsect->width * haxis->locs[nlast];  pos[Y] = 0.0;
		(void) add_point_to_line(AxisLine, pos);

		/* Set horizontal offset for horizontal lines */
		xshift = cur_xsect->x_off;

		/* Loop through locations on vertical axis */
		for ( ii=0; ii<vaxis->num; ii++ )
			{

			/* Set vertical offset for horizontal lines */
			yshift = cur_xsect->y_off + cur_xsect->height * vaxis->locs[ii];

			/* Display horizontal line at vertical axis location */
			if ( !GRA_display_draw_line(AxisLine, arrow_name,
					(xoff + xshift), (yoff + yshift),
					clip_to_map, pattern, pattern_width, pattern_length,
					comp_pres, num_comp) ) return FALSE;
			}
		}

	/* Draw vertical lines on a cross section */
	else if ( same(line_to_draw, XSectVLines) )
		{

		if ( Verbose )
			{
			(void) fprintf(stdout, "  Drawing vertical lines\n");
			}

		/* Get horizontal axis parameters */
		haxis = cross_section_horizontal_axis(cur_xsect, loc_lookup, ltype,
											num_xloc, xsect_locs, NullDouble);
		if ( IsNull(haxis) )
			{
			(void) sprintf(err_buf,
					"Error in cross section horizontal look up ... %s",
					loc_lookup);
			(void) error_report(err_buf);
			}

		/* Get vertical axis parameters */
		vaxis = cross_section_vertical_axis(cur_xsect, ver_lookup, NullDouble);
		if ( IsNull(vaxis) )
			{
			(void) sprintf(err_buf,
					"Error in cross section vertical look up ... %s",
					ver_lookup);
			(void) error_report(err_buf);
			}

		/* Set start and end points on the vertical cross section axis */
		nlast = vaxis->num - 1;
		pos[X] = 0.0;  pos[Y] = cur_xsect->height * vaxis->locs[0];
		(void) add_point_to_line(AxisLine, pos);
		pos[X] = 0.0;  pos[Y] = cur_xsect->height * vaxis->locs[nlast];
		(void) add_point_to_line(AxisLine, pos);

		/* Set vertical offset for vertical lines */
		yshift = cur_xsect->y_off;

		/* Loop through locations on horizontal axis */
		for ( ii=0; ii<haxis->num; ii++ )
			{

			/* Set horizontal offset for vertical lines */
			xshift = cur_xsect->x_off + cur_xsect->width * haxis->locs[ii];

			/* Display vertical line at horizontal axis location */
			if ( !GRA_display_draw_line(AxisLine, arrow_name,
					(xoff + xshift), (yoff + yshift),
					clip_to_map, pattern, pattern_width, pattern_length,
					comp_pres, num_comp) ) return FALSE;
			}
		}

	/* Error for unknown type of line to display */
	else
		{
		(void) sprintf(err_buf, "Unknown type of line to draw ... %s",
				line_to_draw);
		(void) error_report(err_buf);
		}

	/* Return TRUE */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    G R A _ d r a w _ d i s t a n c e _ s c a l e                     *
*    G R A _ d i s p l a y _ d s c a l e _ t i c k s                   *
*    G R A _ d i s p l a y _ d s c a l e _ l a b e l s                 *
*                                                                      *
***********************************************************************/

LOGICAL			GRA_draw_distance_scale

	(
	STRING		scale_name		/* Distance scale name */
	)

	{
	SCALE_DISPLAY	*cur_scale;
	LINE			sline;
	char			err_buf[GPGLong];

	/* Find the named distance scale */
	cur_scale = get_distance_scale(scale_name);
	if ( IsNull(cur_scale) )
		{
		(void) sprintf(err_buf, "Distance Scale ... %s ... not yet defined",
				scale_name);
		(void) error_report(err_buf);
		}
	if ( IsNull(cur_scale->sline) || cur_scale->sline->numpts != 2 )
		{
		(void) sprintf(err_buf, "Distance Scale ... %s ... is incomplete!",
				scale_name);
		(void) error_report(err_buf);
		}

	/* Display the distance scale */
	sline = cur_scale->sline;
	(void) write_graphics_outlines(1, &sline, TRUE, FALSE);

	/* Return TRUE */
	return TRUE;
	}

LOGICAL			GRA_display_dscale_ticks

	(
	STRING		scale_name,		/* Distance scale name */
	int			num_lctns,		/* Number of tick locations */
	SCALE_LCTNS	*scale_lctns,	/* Structure for tick locations */
	float		tlength,		/* Length of tick mark */
	STRING		tjust,			/* Tick mark justification */
	float		trotate			/* Tick mark rotation */
	)

	{
	SCALE_DISPLAY	*cur_scale;
	int				ii;
	float			xx, yy, xbgn, ybgn, xend, yend, xdel, ydel;
	float			mrotate, mratio;
	LINE			xline;
	char			err_buf[GPGLong];

	/* Find the named distance scale */
	cur_scale = get_distance_scale(scale_name);
	if ( IsNull(cur_scale) )
		{
		(void) sprintf(err_buf, "Distance Scale ... %s ... not yet defined",
				scale_name);
		(void) error_report(err_buf);
		}
	if ( IsNull(cur_scale->sline) || cur_scale->sline->numpts != 2 )
		{
		(void) sprintf(err_buf, "Distance Scale ... %s ... is incomplete!",
				scale_name);
		(void) error_report(err_buf);
		}

	/* Set start and end locations on distance scale */
	xbgn = cur_scale->sline->points[0][0];
	ybgn = cur_scale->sline->points[0][1];
	xend = cur_scale->sline->points[1][0];
	yend = cur_scale->sline->points[1][1];
	xdel = xend - xbgn;
	ydel = yend - ybgn;

	/* Set rotation for tick marks (added to distance scale rotation) */
	mrotate = trotate + cur_scale->srotate;

	/* Display tick marks at all locations */
	for ( ii=0; ii<num_lctns; ii++ )
		{

		/* Set ratio of tick location to distance scale */
		mratio = scale_lctns[ii].lctn / cur_scale->slength;

		/* Determine position on distance scale for tick location */
		xx = xbgn + (mratio * xdel);
		yy = ybgn + (mratio * ydel);

		/* Convert length of tick into an offset and rotated line */
		xline = distance_scale_line(tlength, tjust, mrotate, xx, yy);

		/* Display the offset and rotated tick mark */
		(void) write_graphics_outlines(1, &xline, TRUE, FALSE);
		(void) destroy_line(xline);
		}

	/* Return TRUE */
	return TRUE;
	}

LOGICAL			GRA_display_dscale_labels

	(
	STRING		scale_name,		/* Distance scale name */
	int			num_lctns,		/* Number of label locations */
	SCALE_LCTNS	*scale_lctns,	/* Structure for label locations */
	float		txt_size,		/* Size of label characters */
	STRING		ljust,			/* Label mark justification */
	float		lrotate,		/* Label mark rotation */
	float		xoff,			/* x offset of label (display units) */
	float		yoff			/* y offset of label (display units) */
	)

	{
	SCALE_DISPLAY	*cur_scale;
	int				ii;
	float			xx, yy, xbgn, ybgn, xend, yend, xdel, ydel;
	float			mrotate, mratio;
	char			err_buf[GPGLong];

	/* Find the named distance scale */
	cur_scale = get_distance_scale(scale_name);
	if ( IsNull(cur_scale) )
		{
		(void) sprintf(err_buf, "Distance Scale ... %s ... not yet defined",
				scale_name);
		(void) error_report(err_buf);
		}
	if ( IsNull(cur_scale->sline) || cur_scale->sline->numpts != 2 )
		{
		(void) sprintf(err_buf, "Distance Scale ... %s ... is incomplete!",
				scale_name);
		(void) error_report(err_buf);
		}

	/* Set start and end locations on distance scale */
	xbgn = cur_scale->sline->points[0][0];
	ybgn = cur_scale->sline->points[0][1];
	xend = cur_scale->sline->points[1][0];
	yend = cur_scale->sline->points[1][1];
	xdel = xend - xbgn;
	ydel = yend - ybgn;

	/* Set rotation for labels (added to distance scale rotation) */
	mrotate = lrotate + cur_scale->srotate;

	/* Display labels at all locations */
	for ( ii=0; ii<num_lctns; ii++ )
		{

		/* Set ratio of label location to distance scale */
		mratio = scale_lctns[ii].lctn / cur_scale->slength;

		/* Determine position on distance scale for label location */
		xx = xbgn + (mratio * xdel);
		yy = ybgn + (mratio * ydel);

		/* Adjust label location wrt offsets */
		xx += xoff;
		yy += yoff;

		/* Display the offset and rotated label string */
		(void) write_graphics_text(scale_lctns[ii].label, xx, yy,
				txt_size, ljust, mrotate, TRUE);
		}

	/* Return TRUE */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    G R A _ a d d _ t a b l e _ s i t e                               *
*                                                                      *
***********************************************************************/

LOGICAL			GRA_add_table_site

	(
	STRING		table_name,	/* Table name */
	STRING		site_label,	/* Site label */
	float		size,		/* Size for site label text */
	float		rotation,	/* Rotation for site label (degrees) */
	float		xlabel,		/* x offset for site label */
	float		ylabel,		/* y offset for site label */
	float		xoff,		/* x offset for site */
	float		yoff,		/* y offset for site */
	STRING		lat,		/* Latitude for site */
	STRING		lon,		/* Longitude for site */
	STRING		map_x,		/* x axis position for site */
	STRING		map_y,		/* y axis position for site */
	float		map_units,	/* Units for x and y axis */
	STRING		loc_ident	/* Location identifier for site */
	)

	{
	int			ns;
	LOGICAL		status;
	POINT		pos;
	float		fact, xxo, yyo, xx, yy;
	STRING		justified;
	GRA_TABLE	*cur_table;
	char		err_buf[GPGLong];

	/* Set the justification */
	justified = CurPres.justified;

	/* Find the named table */
	cur_table = get_table(table_name);
	if ( IsNull(cur_table) )
		{
		(void) sprintf(err_buf, "Table ... %s ... not yet defined", table_name);
		(void) error_report(err_buf);
		}

	/* Make space for table site */
	ns = cur_table->nsites++;
	cur_table->usell  = GETMEM(cur_table->usell,  LOGICAL, cur_table->nsites);
	cur_table->flats  = GETMEM(cur_table->flats,  float,   cur_table->nsites);
	cur_table->flons  = GETMEM(cur_table->flons,  float,   cur_table->nsites);
	cur_table->idents = GETMEM(cur_table->idents, STRING,  cur_table->nsites);
	cur_table->offset = GETMEM(cur_table->offset, float,   cur_table->nsites);

	/* Set latitude and longitude directly */
	if ( !blank(lat) && !blank(lon) )
		{
		cur_table->usell[ns] = TRUE;
		cur_table->flats[ns] = read_lat(lat, &status);
		if ( !status ) (void) error_report("Problem with lat");
		cur_table->flons[ns] = read_lon(lon, &status);
		if ( !status ) (void) error_report("Problem with lon");
		if ( !blank(loc_ident) )
			cur_table->idents[ns] = strdup(loc_ident);
		else
			cur_table->idents[ns] = strdup(FpaCblank);
		}

	/* Set latitude and longitude from map position */
	else if ( !blank(map_x) && !blank(map_y) )
		{

		/* Set position from map_x and map_y */
		(void) sscanf(map_x, "%f", &pos[X]);
		(void) sscanf(map_y, "%f", &pos[Y]);

		/* Set scaling from map_units */
		fact = map_units / BaseMap.definition.units;
		pos[X] *= fact;
		pos[Y] *= fact;

		/* Convert map position to latitude and longitude */
		cur_table->usell[ns] = TRUE;
		(void) pos_to_ll(&BaseMap, pos,
				&cur_table->flats[ns], &cur_table->flons[ns]);
		if ( !blank(loc_ident) )
			cur_table->idents[ns] = strdup(loc_ident);
		else
			cur_table->idents[ns] = strdup(FpaCblank);
		}

	/* Set location identifier */
	else if ( !blank(loc_ident) )
		{
		cur_table->usell[ns]  = FALSE;
		cur_table->flats[ns]  = 0.0;
		cur_table->flons[ns]  = 0.0;
		cur_table->idents[ns] = strdup(loc_ident);
		}

	/* Error if location cannot be set */
	else
		{
		(void) error_report("No lat/lon or map_x/map_y or location_ident");
		}

	/* Set row or column offset in table */
	if ( same(cur_table->type, TableCol) )
		cur_table->offset[ns] = yoff;
	else if ( same(cur_table->type, TableRow) )
		cur_table->offset[ns] = xoff;

	/* Set site label location using table offsets */
	xxo = cur_table->x_off + xoff + xlabel;
	yyo = cur_table->y_off + yoff + ylabel;
	(void) anchored_location(ZeroPoint, xxo, yyo, &xx, &yy);

	/* Display site label at offset location */
	(void) write_graphics_text(site_label, xx, yy, size,
			justified, rotation, TRUE);

	/* Return TRUE if all went well */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    G R A _ s a m p l e _ f i e l d                                   *
*                                                                      *
***********************************************************************/

LOGICAL			GRA_sample_field

	(
	STRING			element,		/* Element name */
	STRING			level,			/* Level name */
	STRING			equation,		/* Equation to calculate */
	STRING			units,			/* Units for sampled values */
	STRING			field_type,		/* Field type (for sampling labels) */
	STRING			geo_file,		/* Map file for geography */
	STRING			data_file,		/* Data file for parameters */
	STRING			data_file_format,	/* Format for data file */
	STRING			data_file_units,	/* Units for values in data file */
	STRING			data_file_wind_units,	/* Units for winds in data file */
	STRING			cat_cascade,	/* And/Or for multiple CATATTRIB */
	CATATTRIB		*cat_attrib,	/* Structure containing categories */
	int				num_catatt,		/* Number of categories */
	ATTRIB_DISPLAY	*attribs,		/* Attributes to display */
	int				num_attrib,		/* Number of attributes */
	SPCASE			*list_case,		/* Structure for special cases */
	int				num_list,		/* Number of special cases */
	STRING			inmark,			/* Name of location marker */
	float			markscale,		/* Marker scaling factor (percent) */
	STRING			time_zone,		/* Time zone (used in strftime) */
	STRING			language,		/* Language (used in strftime) */
	STRING			display_name,	/* Name for label display */
	STRING			display_type,	/* Type of label display */
	LOGICAL			fit_to_map,		/* Fit display within map? */
	STRING			fit_to_map_ref,	/* Reference position for fit */
	float			rotation,		/* Label rotation (degrees) */
	LOGICAL			rotate_lat,		/* Rotate label to align with latitude? */
	LOGICAL			rotate_lon,		/* Rotate label to align with longitude? */
	STRING			rot_attrib,		/* Attribute to align label to */
	LOGICAL			constrain_rot,	/* Constrain rotation to display upright? */
	float			xdoff,			/* x offset of label and mark */
	float			ydoff,			/* y offset of label and mark */
	float			xoff,			/* x offset of label (display units) */
	float			yoff,			/* y offset of label (display units) */
	STRING			lat,			/* Latitude of sample location */
	STRING			lon,			/* Longitude of sample location */
	STRING			map_x,			/* Map x coordinate of sample location */
	STRING			map_y,			/* Map y coordinate of sample location */
	float			map_units,		/* Map units for sample location */
	STRING			loc_ident,		/* Location identifier for sample location */
	STRING			loc_lookup,		/* Location look up table for location */
	float			proximity,		/* Proximity to sample location (in km) */
	LOGICAL			display_at_feature,	/* Display at location of feature? */
	STRING			table_name,		/* Table name for locations */
	STRING			grid_name,		/* Grid name for locations */
	STRING			list_name,		/* List name for locations */
	STRING			source,			/* Source name for field to sample */
	STRING			vtime,			/* Valid time for field to sample */
	STRING			xsection_name,	/* Cross section to sample */
	GPGltype		ltype,			/* Type of locations for horizontal axis */
	int				num_xloc,		/* Number of locations for horizontal */
	XSECT_LOCATION	*xsect_locs,	/* Locations for horizontal axis */
	STRING			ver_lookup,		/* Lookup file for xsect vertical position */
	STRING			ver_element,	/* Element name for xsect vertical position */
	STRING			ver_level,		/* Level name for xsect vertical position */
	STRING			ver_equation,	/* Equation for xsect vertical position */
	STRING			ver_units,		/* Units for xsect vertical position */
	STRING			ver_field_type,	/* Field type for xsect vertical position */
	STRING			ver_data_file,			/* Data file for xsect position */
	STRING			ver_data_file_format,	/* Format for data file */
	STRING			ver_data_file_units,	/* Units for data file parameter */
	STRING			ver_attrib,			/* Parameter for xsect position */
	STRING			ver_attrib_upper,	/* Parameter for xsect upper position */
	STRING			ver_attrib_lower	/* Parameter for xsect lower position */
	)

	{
	STRING			fname;
	int				fkind, nn, iix, iiy, isite, iloc, iil, iarea, isub, iseg;
	float			flat, flon, fact, fdist, fbear, flen, dang, xdir, xspd;
	float			yloc, xxo, yyo, xx, yy, xxt, yyt, xxs, yys, xxc, yyc;
	double			dval, mval;
	STRING			locid, loclat, loclon, loclab, vt, ident, label;
	LOGICAL			status, cur_anchor, inside, right;
	HAND			sense;
	POINT			pos[1] = { ZERO_POINT };
	VLIST			*vlist, *mlist, *dlist;
	SET				areas, curves, spots, lchains, copyset;
	AREA			area, carea;
	SUBAREA			subarea;
	CURVE			curve;
	SPOT			spot;
	LCHAIN			lchain;
	CAL				cal;
	POINT			fpos, spos, epos;
	char			mlat[GPGMedium], mlon[GPGMedium];
	GRA_TABLE		*cur_table;
	GRA_GRID		*cur_grid;
	GRA_LIST		*cur_list;
	GRA_XSECT		*cur_xsect;
	XSECT_HOR_AXIS	*haxis;
	XSECT_VER_AXIS	*vaxis;
	char			err_buf[GPGLong];

	FLD_DESCRIPT	descript;

	FpaConfigFieldStruct	*fdef;
	FpaConfigUnitStruct		*udef;

	static	METAFILE	gmeta = NullMeta;
	static	SURFACE		gsfc  = NullSfc;
	static	SET			gset  = NullSet;
	static	char		lastfname[GPGLong]   = "";
	static	MAP_PROJ	lastmproj            = NO_MAPPROJ;
	static	char		lastelement[GPGLong] = "";
	static	char		lastlevel[GPGLong]   = "";
	static	int			lastfkind            = FpaCnoMacro;

	/* Arrays for table or grid sampling */
	static	int		Nump   = 0;
	static	POINT	*Ppos  = NullPointList;
	static	float	*Plats = NullFloat;
	static	float	*Plons = NullFloat;
	static	STRING	*Pids  = NullStringList;
	static	STRING	*Plabs = NullStringList;

	/* Set field type to extract parameters from data files */
	if ( !blank(data_file) )
		{

		/* Set the field type for parameters from data files */
		fkind = FpaC_SCATTERED;
		}

	/* Extract data from geography files */
	else if ( !blank(geo_file) )
		{

		/* Make a copy of the global field descriptor */
		(void) copy_fld_descript(&descript, &Fdesc);

		/* Re-initialize the field descriptor for this element and level */
		if ( !set_fld_descript(&descript, FpaF_DIRECTORY_PATH, FpaCblank,
										   FpaF_ELEMENT_NAME,  element,
										   FpaF_LEVEL_NAME,    level,
										   FpaF_END_OF_LIST) )
			{
			(void) sprintf(err_buf,
					" Error setting field descriptor for ... %s %s\n",
					element, level);
			(void) error_report(err_buf);
			}

		/* Check for geography file of this name */
		fname = get_file("Maps", env_sub(geo_file));
		if ( IsNull(fname) )
			{
			(void) sprintf(err_buf,
					"Problem finding map file ... %s", geo_file);
			(void) warn_report(err_buf);
			return TRUE;
			}

		/* Set the field type from the element and level */
		fdef = get_field_info(descript.edef->name, descript.ldef->name);
		if ( IsNull(fdef) )
			{
			(void) sprintf(err_buf,
					"Unrecognized field ... %s  %s", element, level);
			(void) error_report(err_buf);
			}
		fkind = fdef->element->fld_type;

		/* Reset the field type (if requested) */
		if ( !blank(field_type) )
			{
			fkind = field_data_type(field_type);
			if ( fkind == FpaCnoMacro )
				{
				(void) sprintf(err_buf,
						" Unrecognized field type ... %s\n", field_type);
				(void) error_report(err_buf);
				}
			}

		/* Set the field type for retrieving the features */
		(void) set_fld_descript(&descript, FpaF_FIELD_MACRO, fkind,
											FpaF_END_OF_LIST);

		/* Read the geography file                             */
		/* Note that we save the METAFILE for subsequent calls */
		if ( !same(fname, lastfname)
				|| !equivalent_map_projection(&BaseMap, &lastmproj) )
			{

			/* Save the name and map projection for next call */
			(void) strcpy(lastfname, fname);
			(void) copy_map_projection(&lastmproj, &BaseMap);

			/* Reset pointers to data and current element and level */
			gsfc = NullSfc;
			gset = NullSet;
			(void) strcpy(lastelement, FpaCblank);
			(void) strcpy(lastlevel,   FpaCblank);
			lastfkind = FpaCnoMacro;

			/* Read the geography metafile */
			gmeta = destroy_metafile(gmeta);
			gmeta = read_metafile(fname, &BaseMap);

			/* Error if geography metafile is empty */
			if ( IsNull(gmeta) )
				{
				(void) sprintf(err_buf,
						"Problem reading map file ... %s", geo_file);
				(void) warn_report(err_buf);
				return TRUE;
				}
			}

		/* Extract data from the geography file */
		if ( !same(element, lastelement) || !same(level, lastlevel)
				|| fkind != lastfkind )
			{

			/* Save the element and level names for next call */
			(void) strcpy(lastelement, element);
			(void) strcpy(lastlevel,   level);
			lastfkind = fkind;

			/* Reset pointers to data */
			gsfc = NullSfc;
			gset = NullSet;

			/* Extract data depending on field type */
			switch ( fkind )
				{

				/* Extract data for continuous type fields */
				case FpaC_CONTINUOUS:
					gsfc = find_mf_sfc(gmeta, "a",
									descript.edef->name, descript.ldef->name);
					break;

				/* Extract data for vector type fields */
				case FpaC_VECTOR:
					gsfc = find_mf_sfc(gmeta, "v",
									descript.edef->name, descript.ldef->name);
					break;

				/* Extract data for discrete type fields */
				case FpaC_DISCRETE:
					gset = find_mf_set(gmeta, "area", "b",
									descript.edef->name, descript.ldef->name);
					break;

				/* Extract data for line type fields */
				case FpaC_LINE:
					gset = find_mf_set(gmeta, "curve", "c",
									descript.edef->name, descript.ldef->name);
					break;

				/* Extract data for scattered type fields */
				case FpaC_SCATTERED:
					gset = find_mf_set(gmeta, "spot", "d",
									descript.edef->name, descript.ldef->name);
					break;

				/* Extract data for link chain type fields */
				case FpaC_LCHAIN:
					gset = find_mf_set(gmeta, "lchain", "l",
									descript.edef->name, descript.ldef->name);
					break;

				/* Cannot sample other types of fields */
				default:
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot sample field ... %s %s from map file %s\n",
							element, level, geo_file);
						}
					break;
				}
			}

		/* Return if field cannot be extracted */
		if ( IsNull(gsfc) && IsNull(gset) )
			{
			if ( Verbose )
				{
				(void) fprintf(stdout,
					"Cannot extract field ... %s %s from map file %s\n",
					element, level, geo_file);
				}
			return TRUE;
			}
		}

	/* Set field descriptor for all other fields */
	else
		{

		/* Make a copy of the global field descriptor */
		(void) copy_fld_descript(&descript, &Fdesc);

		/* Re-initialize the field descriptor for this element and level */
		if ( !set_fld_descript(&descript, FpaF_DIRECTORY_PATH, FpaCblank,
										   FpaF_SOURCE_NAME,   source,
										   FpaF_RUN_TIME,      FpaCblank,
										   FpaF_ELEMENT_NAME,  element,
										   FpaF_LEVEL_NAME,    level,
										   FpaF_VALID_TIME,    vtime,
										   FpaF_END_OF_LIST) )
			{
			(void) sprintf(err_buf,
					" Error setting field descriptor for ... %s %s from %s at %s\n",
					element, level, source, vtime);
			(void) error_report(err_buf);
			}

		/* Set the field type for sampling by equation */
		if ( !blank(equation) )
			{
			fkind = FpaC_CONTINUOUS;
			}

		/* Set the field type from the element and level */
		else
			{
			fdef = get_field_info(descript.edef->name, descript.ldef->name);
			if ( IsNull(fdef) )
				{
				(void) sprintf(err_buf,
						"Unrecognized element ... %s  or level ... %s",
						element, level);
				(void) error_report(err_buf);
				}
			fkind = fdef->element->fld_type;

			/* Reset the field type (if requested) */
			if ( !blank(field_type) )
				{
				fkind = field_data_type(field_type);
				if ( fkind == FpaCnoMacro )
					{
					(void) sprintf(err_buf,
							" Unrecognized field type ... %s\n", field_type);
					(void) error_report(err_buf);
					}
				}

			/* Set the field type for retrieving the features */
			(void) set_fld_descript(&descript, FpaF_FIELD_MACRO, fkind,
												FpaF_END_OF_LIST);

			/* Check that units match with field information */
			switch ( fkind )
				{

				/* Must match field units for continuous/vector fields */
				case FpaC_CONTINUOUS:
				case FpaC_VECTOR:
					udef = fdef->element->elem_io->units;
					if ( NotNull(udef)
							&& !convert_value(udef->name, 0.0,
															units, NullDouble) )
						{
						(void) sprintf(err_buf,
								"Incorrect units: %s  for field: %s %s with units %s",
								units, element, level, udef->name);
						(void) error_report(err_buf);
						}
					break;
				}
			}
		}

	/* Replace "default" attributes (if required) */
	for ( nn=0; nn<num_attrib; nn++ )
		{
		(void) strcpy(attribs[nn].name,
						replace_default_attribute(fkind, attribs[nn].name));
		}

	/* Sample fields for all locations in a location look up table */
	if ( !blank(loc_ident) && !blank(loc_lookup)
				&& ( same_ic(loc_ident, LocIdentAll)
						|| same_ic(loc_ident, LocIdentAllVtime) ) )
		{

		/* Ensure that samples can be displayed on current map */
		if ( !AnchorToMap )
			{
			(void) warn_report("Must set anchor to map!");
			return TRUE;
			}

		/* Set valid time to check */
		if ( same_ic(loc_ident, LocIdentAllVtime) ) vt = vtime;
		else                                        vt = FpaCblank;

		if ( Verbose )
			{
			if ( !blank(data_file) )
				(void) fprintf(stdout,
						"Lookup sampling from data file %s\n", data_file);
			else if ( !blank(geo_file) )
				(void) fprintf(stdout,
						"Lookup sampling for field ... %s %s from map file %s\n",
						element, level, geo_file);
			else if ( !blank(equation) )
				(void) fprintf(stdout,
						" Lookup sampling for equation ... %s from %s at %s\n",
						equation, source, vtime);
			else
				(void) fprintf(stdout,
						" Lookup sampling for field ... %s %s from %s at %s\n",
						element, level, source, vtime);
			}

		/* Set sample locations for all look up table locations */
		Nump = 0;
		iloc = -1;
		while ( (iloc = next_location_lookup_line(loc_lookup, iloc, vt,
							&locid, &loclat, &loclon, &loclab)) >= 0 )
			{

			/* Set latitude/longitude for sampling */
			flat = read_lat(loclat, &status);
			if ( !status )
				(void) error_report("Problem with location look up lat");
			flon = read_lon(loclon, &status);
			if ( !status )
				(void) error_report("Problem with location look up lon");

			/* Allocate space for this sampling location */
			Nump++;
			Ppos  = GETMEM(Ppos,  POINT,  Nump);
			Plats = GETMEM(Plats, float,  Nump);
			Plons = GETMEM(Plons, float,  Nump);
			Pids  = GETMEM(Pids,  STRING, Nump);
			Plabs = GETMEM(Plabs, STRING, Nump);

			/* Set map position for sampling */
			Plats[Nump-1] = flat;
			Plons[Nump-1] = flon;
			(void) ll_to_pos(&BaseMap, flat, flon, Ppos[Nump-1]);
			Pids[Nump-1]  = safe_strdup(locid);
			Plabs[Nump-1] = safe_strdup(loclab);
			}

		/* Sample at all look up table locations from data files */
		if ( !blank(data_file) )
			{
			status = TRUE;

			/* Create a set of spots from all lookup table locations */
			spots = create_set("spot");
			for ( isite=0; isite<Nump; isite++ )
				{

				/* Convert latitude and longitude to STRING format */
				(void) sprintf(mlat, "%f", Plats[isite]);
				(void) sprintf(mlon, "%f", Plons[isite]);

				/* Extract attributes from matching data file entry */
				if ( !match_data_file(data_file, data_file_format,
						data_file_units, data_file_wind_units,
						Pids[isite], mlat, mlon, vtime, &cal) )
					{
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot match ident/lat/lon/vtime ... %s/%f/%f/%s  in data file ... %s",
							Pids[isite], Plats[isite], Plons[isite], vtime,
							data_file);
						}
					}

				/* Build a spot object from the data file attributes */
				spot = create_spot(Ppos[isite], FpaCblank, AttachNone, cal);

				/* Add the spot object to the set */
				(void) add_item_to_set(spots, (ITEM) spot);
				}
			}

		/* Sample fields at all look up table locations from geography files */
		/*  depending on field type                                          */
		else if ( !blank(geo_file) )
			{
			status = TRUE;
			switch ( fkind )
				{

				/* Sample continuous type fields */
				case FpaC_CONTINUOUS:

					/* Create VLIST Object to hold sampled values */
					vlist = INITMEM(VLIST, 1);
					(void) init_vlist(vlist);

					/* Sample field at all look up table locations */
					for ( isite=0; isite<Nump; isite++ )
						{

						/* Sample field at look up table location */
						status = eval_sfc(gsfc, Ppos[isite], &dval);

						/* Error if field cannot be sampled */
						if ( !status )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
									"Cannot sample field ... %s %s from map file %s\n",
									element, level, geo_file);
								}
							(void) free_vlist(vlist);
							FREEMEM(vlist);
							break;
							}

						/* Add value to VLIST Object */
						(void) add_point_to_vlist(vlist, Ppos[isite],
																(float) dval);
						}
					break;

				/* Sample vector type fields */
				case FpaC_VECTOR:

					/* Create VLIST Objects to hold sampled values */
					mlist = INITMEM(VLIST, 1);
					(void) init_vlist(mlist);
					dlist = INITMEM(VLIST, 1);
					(void) init_vlist(dlist);

					/* Sample field at all look up table locations */
					for ( isite=0; isite<Nump; isite++ )
						{

						/* Sample field at look up table location */
						status = eval_sfc_MD(gsfc, Ppos[isite], &mval, &dval);

						/* Error if field cannot be sampled */
						if ( !status )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
									"Cannot sample field ... %s %s from map file %s\n",
									element, level, geo_file);
								}
							(void) free_vlist(mlist);
							(void) free_vlist(dlist);
							FREEMEM(mlist);
							FREEMEM(dlist);
							break;
							}

						/* Add values to VLIST Objects */
						(void) add_point_to_vlist(mlist, Ppos[isite],
																(float) mval);
						(void) add_point_to_vlist(dlist, Ppos[isite],
																(float) dval);
						}
					break;

				/* Sample discrete type fields */
				case FpaC_DISCRETE:

					/* Copy the discrete areas */
					areas = copy_set(gset);

					/* Error if field cannot be evaluated */
					if ( IsNull(areas) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from map file %s\n",
								element, level, geo_file);
							}
						status = FALSE;
						}
					break;

				/* Sample line type fields */
				case FpaC_LINE:

					/* Copy the line curves */
					curves = copy_set(gset);

					/* Error if field cannot be evaluated */
					if ( IsNull(curves) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from map file %s\n",
								element, level, geo_file);
							}
						status = FALSE;
						}
					break;

				/* Sample scattered type fields */
				case FpaC_SCATTERED:

					/* Copy the scattered spots */
					spots = copy_set(gset);

					/* Error if field cannot be evaluated */
					if ( IsNull(spots) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from map file %s\n",
								element, level, geo_file);
							}
						status = FALSE;
						}
					break;

				/* Sample link chain type fields */
				case FpaC_LCHAIN:

					/* Copy the link chains */
					lchains = copy_set(gset);

					/* Error if field cannot be evaluated */
					if ( IsNull(lchains) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from map file %s\n",
								element, level, geo_file);
							}
						status = FALSE;
						}
					break;

				/* Cannot sample other types of fields */
				default:
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot sample field ... %s %s from map file %s\n",
							element, level, geo_file);
						}
					status = FALSE;
					break;
				}
			}

		/* Sample fields at all look up table locations from all other files */
		/*  depending on field type                                          */
		else
			{
			status = TRUE;
			switch ( fkind )
				{

				/* Sample continuous type fields */
				case FpaC_CONTINUOUS:

					/* Extract field values by equation */
					if ( !blank(equation) )
						vlist = retrieve_vlist_by_equation(&descript,
											Nump, Ppos, FpaCmksUnits, equation);

					/* Extract field values directly */
					else
						vlist = retrieve_vlist(&descript, Nump, Ppos);

					/* Error if field cannot be evaluated */
					if ( IsNull(vlist) )
						{
						if ( Verbose )
							{
							if ( !blank(equation) )
								(void) fprintf(stdout,
									"Cannot calculate equation ... %s from %s at %s\n",
									equation, source, vtime);
							else
								(void) fprintf(stdout,
									"Cannot extract field ... %s %s from %s at %s\n",
									element, level, source, vtime);
							}
						status = FALSE;
						}
					break;

				/* Sample vector type fields */
				case FpaC_VECTOR:

					/* Extract magnitude and direction of vector field */
					mlist = retrieve_vlist_component(&descript, M_Comp, Nump,
																		Ppos);
					dlist = retrieve_vlist_component(&descript, D_Comp, Nump,
																		Ppos);

					/* Error if field cannot be evaluated */
					if ( IsNull(mlist) || IsNull(dlist) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from %s at %s\n",
								element, level, source, vtime);
							}
						status = FALSE;
						}
					break;

				/* Sample discrete type fields */
				case FpaC_DISCRETE:

					/* Find the field containing the discrete areas */
					areas = retrieve_areaset(&descript);

					/* Error if field cannot be evaluated */
					if ( IsNull(areas) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from %s at %s\n",
								element, level, source, vtime);
							}
						status = FALSE;
						}
					break;

				/* Sample line type fields */
				case FpaC_LINE:

					/* Find the field containing the line curves */
					curves = retrieve_curveset(&descript);

					/* Error if field cannot be evaluated */
					if ( IsNull(curves) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from %s at %s\n",
								element, level, source, vtime);
							}
						status = FALSE;
						}
					break;

				/* Sample scattered type fields */
				case FpaC_SCATTERED:

					/* Find the field containing the scattered spots */
					spots = retrieve_spotset(&descript);

					/* Error if field cannot be evaluated */
					if ( IsNull(spots) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from %s at %s\n",
								element, level, source, vtime);
							}
						status = FALSE;
						}
					break;

				/* Sample link chain type fields */
				case FpaC_LCHAIN:

					/* Find the field containing the link chains */
					lchains = retrieve_lchainset(&descript);

					/* Error if field cannot be evaluated */
					if ( IsNull(lchains) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from %s at %s\n",
								element, level, source, vtime);
							}
						status = FALSE;
						}
					break;

				/* Cannot sample other types of fields */
				default:
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot sample field ... %s %s from %s at %s\n",
							element, level, source, vtime);
						}
					status = FALSE;
					break;
				}
			}

		/* Return now if field could not be sampled */
		if ( !status )
			{

			/* Free space used by idents and labels */
			FREELIST(Pids,  Nump);
			FREELIST(Plabs, Nump);

			return TRUE;
			}

		/* Display values for all look up table locations */
		for ( isite=0; isite<Nump; isite++ )
			{

			/* Check for sample locations off the map */
			if ( Ppos[isite][X] < 0.0 || Ppos[isite][Y] < 0.0
					|| Ppos[isite][X] > BaseMap.definition.xlen
					|| Ppos[isite][Y] > BaseMap.definition.ylen )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Sample lat/lon outside map at ... %.1f %.1f\n",
							Plats[isite], Plons[isite]);
					}

				/* Continue for sample locations off map */
				continue;
				}

			/* Comments for sample locations on the map */
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Sample lat/lon at ... %.1f %.1f\n",
							Plats[isite], Plons[isite]);
					}
				}

			/* Set display location on the current map */
			(void) anchored_location(Ppos[isite], 0.0, 0.0, &xxo, &yyo);

			/* Set display location (offset by xdoff/ydoff) */
			/*  and offset location for sampled attributes  */
			xxo += xdoff;
			yyo += ydoff;
			xx = xxo + xoff;
			yy = yyo + yoff;

			/* Display values depending on field type */
			switch ( fkind )
				{

				/* Display values for continuous type fields */
				case FpaC_CONTINUOUS:

					/* Convert value to required units */
					(void) convert_value(FpaCmksUnits,
							(double) vlist->val[isite], units, &dval);

					/* Display the field value */
					(void) GRA_display_sampled_value(Pids[isite],
							vtime, time_zone, language,
							Plabs[isite], Ppos[isite], (float) dval,
							attribs, num_attrib, list_case, num_list,
							inmark, markscale, display_name, display_type,
							fit_to_map, fit_to_map_ref,
							rotation, xxo, yyo, xx, yy);
					break;

				/* Display values for vector type fields */
				case FpaC_VECTOR:

					/* Convert magnitude to required units */
					(void) convert_value(FpaCmksUnits,
							(double) mlist->val[isite], units, &mval);

					/* Convert sampled direction to degrees true */
					dang = wind_dir_true(&BaseMap,
							Plats[isite], Plons[isite], dlist->val[isite]);

					/* >>>>> debug testing for vector fields in sample_field() <<<<< */
					if ( DebugMode )
						{
						(void) fprintf(stdout,
							"Vector field at %.1f %.1f ... m/d: %.1f %.1f  mval/dang: %.1f %.1f\n",
							Plats[isite], Plons[isite], mlist->val[isite], dlist->val[isite],
							mval, dang);
						}
					/* >>>>> debug testing for vector fields in sample_field() <<<<< */

					/* Display the vector */
					(void) GRA_display_sampled_vector(Pids[isite],
							vtime, time_zone, language,
							Plabs[isite], Ppos[isite], (float) mval,
							dang, attribs, num_attrib, list_case, num_list,
							inmark, markscale, display_name, display_type,
							fit_to_map, fit_to_map_ref,
							rotation, xxo, yyo, xx, yy);
					break;

				/* Display values for discrete type fields */
				case FpaC_DISCRETE:

					/* Sample areas based on proximity */
					if ( proximity > 0.0 )
						{

						/* Make a copy of the area or subareas in the area */
						copyset = create_set("area");
						for ( iarea=0; iarea<areas->num; iarea++ )
							{
							area = (AREA) areas->list[iarea];

							/* Ensure that area is within proximity */
							(void) area_test_point(area, Ppos[isite], NULL,
													fpos, NULL, NULL, NULL,
													&inside);
							if ( !inside )
								{
								fdist  = great_circle_distance(&BaseMap,
															Ppos[isite], fpos);
								fdist /= 1000.0;
								if ( fdist > proximity ) continue;
								}

							/* Copy the area if no subareas */
							if ( area->numdiv == 0)
								{
								carea = copy_area(area, FALSE);
								if ( NotNull(carea) )
									{
									(void) add_item_to_set(copyset,
															(ITEM) carea);
									}
								}

							/* Create an area from each subarea */
							else
								{
								for ( isub=0; isub<=area->numdiv; isub++ )
									{
									subarea = area->subareas[isub];
									carea = area_from_subarea(subarea);
									if ( NotNull(carea) )
										{
										(void) add_item_to_set(copyset,
																(ITEM) carea);
										}
									}
								}
							}

						/* Search for closest area within proximity */
						/*  that matches the required attributes    */
						while ( copyset->num > 0 )
							{

							/* Find an enclosing area */
							area = enclosing_area(copyset, Ppos[isite],
												PickFirst, NullFloat, NullChar);

							/* Set sampling point and proximity (if inside) */
							if ( NotNull(area) )
								{
								(void) copy_point(fpos, Ppos[isite]);
								inside = TRUE;
								fdist  = 0.0;
								fbear  = 0.0;
								}

							/* Find the closest area within proximity */
							else
								{
								area   = closest_area(copyset, Ppos[isite],
												NULL, fpos, NULL, NULL, NULL);
								inside = FALSE;
								fdist  = great_circle_distance(&BaseMap,
															Ppos[isite], fpos);
								fdist /= 1000.0;
								if ( fdist > proximity )
									{
									if ( Verbose )
										{
										(void) fprintf(stdout,
												"   Feature beyond proximity for lat/lon ... %.1f %.1f\n",
												Plats[isite], Plons[isite]);
										}

									break;
									}

								/* Determine bearing from sample to feature */
								fbear  = great_circle_bearing(&BaseMap,
															Ppos[isite], fpos);
								}

							/* Ensure that feature is on the map */
							if ( fpos[X] < 0.0 || fpos[Y] < 0.0
									|| fpos[X] > BaseMap.definition.xlen
									|| fpos[Y] > BaseMap.definition.ylen )
								{
								if ( Verbose )
									{
									(void) fprintf(stdout,
											"   Feature outside map for lat/lon ... %.1f %.1f\n",
											Plats[isite], Plons[isite]);
									}

								(void) remove_item_from_set(copyset,
																(ITEM) area);
								continue;
								}

							/* Reset the sampling location (if not inside) */
							if ( !inside )
								{
								(void) anchored_location(fpos, 0.0, 0.0,
																&xxo, &yyo);
								xxo += xdoff;
								yyo += ydoff;
								}

							/* Display area attributes ... if they match! */
							if ( GRA_display_sampled_attributes(Pids[isite],
									vtime, time_zone, language,
									Plabs[isite], fpos, fdist, fbear,
									DefLineLen, DefSegDir, DefSegSpd,
									area->attrib, cat_cascade,
									cat_attrib, num_catatt,
									attribs, num_attrib, list_case, num_list,
									inmark, markscale, display_name,
									display_type, fit_to_map, fit_to_map_ref,
									rotation, rotate_lat, rotate_lon,
									rot_attrib, constrain_rot,
									xxo, yyo, xx, yy) )
								break;

							/* Attributes did not match              */
							/*  ... so remove this area from the set */
							(void) remove_item_from_set(copyset, (ITEM) area);
							}

						/* Destroy what is left of the copy */
						(void) destroy_set(copyset);
						break;
						}

					/* Sample areas directly */
					else
						{

						/* Find area enclosing point                        */
						/* Note that this should return a background value  */
						/*  with no subarea if no enclosing area is found   */
						/*  ... except map files, which have no background! */
						if ( !eval_areaset(areas, Ppos[isite], PickFirst,
								&subarea, &cal) )
							{
							if ( blank(geo_file) && Verbose )
								{
								(void) fprintf(stdout,
									"No areas or background for field ... %s %s from %s at %s\n",
									element, level, source, vtime);
								}
							continue;
							}

						/* Display attributes from attribute structure */
						(void) GRA_display_sampled_attributes(Pids[isite],
								vtime, time_zone, language,
								Plabs[isite], Ppos[isite], DefProximity,
								DefBearing, DefLineLen, DefSegDir, DefSegSpd,
								cal, cat_cascade, cat_attrib, num_catatt,
								attribs, num_attrib, list_case, num_list,
								inmark, markscale, display_name,
								display_type, fit_to_map, fit_to_map_ref,
								rotation, rotate_lat, rotate_lon,
								rot_attrib, constrain_rot,
								xxo, yyo, xx, yy);
						}
					break;

				/* Display values for line type fields */
				case FpaC_LINE:

					/* Make a copy of the line curves */
					copyset = copy_set(curves);

					/* Search for closest curve within proximity */
					/*  that matches the required attributes     */
					while ( copyset->num > 0 )
						{

						/* Find the closest curve */
						curve = closest_curve(copyset, Ppos[isite],
												NullFloat, fpos, &iseg);
						if ( IsNull(curve) || IsNull(curve->attrib) )
							{
							(void) remove_item_from_set(copyset, (ITEM) curve);
							continue;
							}

						/* Check the proximity */
						fdist  = great_circle_distance(&BaseMap,
														Ppos[isite], fpos);
						fdist /= 1000.0;
						if ( fdist > proximity )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
										"   Feature beyond proximity for lat/lon ... %.1f %.1f\n",
										Plats[isite], Plons[isite]);
								}

							break;
							}

						/* Ensure that feature is on the map */
						if ( fpos[X] < 0.0 || fpos[Y] < 0.0
								|| fpos[X] > BaseMap.definition.xlen
								|| fpos[Y] > BaseMap.definition.ylen )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
										"   Feature outside map for lat/lon ... %.1f %.1f\n",
										Plats[isite], Plons[isite]);
								}

							(void) remove_item_from_set(copyset, (ITEM) curve);
							continue;
							}

						/* Determine bearing from sample to feature */
						fbear  = great_circle_bearing(&BaseMap,
														Ppos[isite], fpos);

						/* Change sign of proximity wrt curve sense */
						(void) recall_curve_sense(curve, &sense);
						(void) curve_test_point(curve, Ppos[isite], NullFloat,
								NullPoint, NullInt, NullLogical, &right);
						if ( (sense == Right && right)
								|| (sense == Left && !right) ) fdist = -fdist;

						/* Determine length of curve */
						flen = great_circle_line_length(&BaseMap, curve->line);

						/* Reset the sampling location */
						(void) anchored_location(fpos, 0.0, 0.0, &xxo, &yyo);
						xxo += xdoff;
						yyo += ydoff;

						/* Reset the display location (if required) */
						if ( display_at_feature )
							{
							xx = xxo + xoff;
							yy = yyo + yoff;
							}

						/* Determine direction of curve */
						(void) line_span_info(curve->line, iseg,
													spos, epos, NullPoint);
						xdir = great_circle_bearing(&BaseMap, spos, epos);

						/* Display curve attributes ... if they match! */
						if ( GRA_display_sampled_attributes(Pids[isite],
								vtime, time_zone, language,
								Plabs[isite], fpos, fdist, fbear, flen,
								xdir, DefSegSpd, curve->attrib,
								cat_cascade, cat_attrib, num_catatt,
								attribs, num_attrib, list_case, num_list,
								inmark, markscale, display_name,
								display_type, fit_to_map, fit_to_map_ref,
								rotation, rotate_lat, rotate_lon,
								rot_attrib, constrain_rot,
								xxo, yyo, xx, yy) )
							break;

						/* Attributes did not match               */
						/*  ... so remove this curve from the set */
						(void) remove_item_from_set(copyset, (ITEM) curve);
						}

					/* Destroy what is left of the copy */
					(void) destroy_set(copyset);
					break;

				/* Display values for scattered type fields */
				case FpaC_SCATTERED:

					/* Extract the spot from the data file set */
					if ( !blank(data_file) )
						{

						/* Get the spot */
						spot = spots->list[isite];
						if ( IsNull(spot) || IsNull(spot->attrib) ) continue;

						/* Display attributes from spot attribute structure */
						(void) GRA_display_sampled_attributes(Pids[isite],
								vtime, time_zone, language,
								Plabs[isite], spot->anchor, DefProximity,
								DefBearing, DefLineLen, DefSegDir, DefSegSpd,
								spot->attrib,
								cat_cascade, cat_attrib, num_catatt,
								attribs, num_attrib, list_case, num_list,
								inmark, markscale, display_name,
								display_type, fit_to_map, fit_to_map_ref,
								rotation, rotate_lat, rotate_lon,
								rot_attrib, constrain_rot,
								xxo, yyo, xx, yy);
						}

					/* Extract the spot from the set */
					else
						{
						/* Make a copy of the scattered spots */
						copyset = copy_set(spots);

						/* Search for closest spot within proximity */
						/*  that matches the required attributes    */
						while ( copyset->num > 0 )
							{

							/* Find the closest spot */
							spot = closest_spot(copyset, Ppos[isite], NULL,
													NULL, NullFloat, fpos);
							if ( IsNull(spot) || IsNull(spot->attrib) )
								{
								(void) remove_item_from_set(copyset,
																(ITEM) spot);
								continue;
								}

							/* Check the proximity */
							fdist  = great_circle_distance(&BaseMap,
															Ppos[isite], fpos);
							fdist /= 1000.0;
							if ( fdist > proximity )
								{
								if ( Verbose )
									{
									(void) fprintf(stdout,
											"   Feature beyond proximity for lat/lon ... %.1f %.1f\n",
											Plats[isite], Plons[isite]);
									}

								break;
								}

							/* Determine bearing from sample to feature */
							fbear  = great_circle_bearing(&BaseMap,
															Ppos[isite], fpos);

							/* Ensure that feature is on the map */
							if ( fpos[X] < 0.0 || fpos[Y] < 0.0
									|| fpos[X] > BaseMap.definition.xlen
									|| fpos[Y] > BaseMap.definition.ylen )
								{
								if ( Verbose )
									{
									(void) fprintf(stdout,
											"   Feature outside map for lat/lon ... %.1f %.1f\n",
											Plats[isite], Plons[isite]);
									}

								(void) remove_item_from_set(copyset, (ITEM) spot);
								continue;
								}

							/* Reset the sampling location */
							(void) anchored_location(fpos, 0.0, 0.0, &xxo, &yyo);
							xxo += xdoff;
							yyo += ydoff;

							/* Reset the display location (if required) */
							if ( display_at_feature )
								{
								xx = xxo + xoff;
								yy = yyo + yoff;
								}

							/* Display spot attributes ... if they match! */
							if ( GRA_display_sampled_attributes(Pids[isite],
									vtime, time_zone, language,
									Plabs[isite], fpos, fdist, fbear,
									DefLineLen, DefSegDir, DefSegSpd,
									spot->attrib,
									cat_cascade, cat_attrib, num_catatt,
									attribs, num_attrib, list_case, num_list,
									inmark, markscale, display_name,
									display_type, fit_to_map, fit_to_map_ref,
									rotation, rotate_lat, rotate_lon,
									rot_attrib, constrain_rot,
									xxo, yyo, xx, yy) )
								break;

							/* Attributes did not match              */
							/*  ... so remove this spot from the set */
							(void) remove_item_from_set(copyset, (ITEM) spot);
							}

						/* Destroy what is left of the copy */
						(void) destroy_set(copyset);
						}
					break;

				/* Display values for link chain type fields */
				case FpaC_LCHAIN:

					/* >>>>> Display link chain based on type? <<<<< */

					/* Make a copy of the link chains */
					copyset = copy_set(lchains);

					/* Search for closest link chain within proximity */
					/*  that matches the required attributes          */
					while ( copyset->num > 0 )
						{

						/* Find the closest link chain */
						lchain = closest_lchain(copyset, Ppos[isite],
												NullFloat, fpos, &iseg);
						if ( IsNull(lchain) || IsNull(lchain->attrib) )
							{
							(void) remove_item_from_set(copyset, (ITEM) lchain);
							continue;
							}

						/* Check the proximity */
						fdist  = great_circle_distance(&BaseMap,
														Ppos[isite], fpos);
						fdist /= 1000.0;
						if ( fdist > proximity )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
										"   Feature beyond proximity for lat/lon ... %.1f %.1f\n",
										Plats[isite], Plons[isite]);
								}

							break;
							}

						/* Ensure that feature is on the map */
						if ( fpos[X] < 0.0 || fpos[Y] < 0.0
								|| fpos[X] > BaseMap.definition.xlen
								|| fpos[Y] > BaseMap.definition.ylen )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
										"   Feature outside map for lat/lon ... %.1f %.1f\n",
										Plats[isite], Plons[isite]);
								}

							(void) remove_item_from_set(copyset, (ITEM) lchain);
							continue;
							}

						/* Determine bearing from sample to feature */
						fbear  = great_circle_bearing(&BaseMap,
														Ppos[isite], fpos);

						/* Interpolate the link chain (if required) */
						if ( lchain->dointerp ) (void) interpolate_lchain(lchain);

						/* Determine length of link chain track */
						flen = great_circle_line_length(&BaseMap, lchain->track);

						/* Reset the sampling location */
						(void) anchored_location(fpos, 0.0, 0.0, &xxo, &yyo);
						xxo += xdoff;
						yyo += ydoff;

						/* Reset the display location (if required) */
						if ( display_at_feature )
							{
							xx = xxo + xoff;
							yy = yyo + yoff;
							}

						/* Determine direction and speed on link chain track */
						if ( lchain->inum > 1 )
							{
							(void) line_span_info(lchain->track, iseg,
														spos, epos, NullPoint);
							xdir  = great_circle_bearing(&BaseMap, spos, epos);
							xspd  = point_dist(spos, epos) /
														(float) lchain->minterp;
							xspd *= BaseMap.definition.units;
							xspd /= 60.0;
							}
						else
							{
							xdir = DefSegDir;
							xspd = DefSegSpd;
							}

						/* Display link chain attributes ... if they match! */
						if ( GRA_display_sampled_attributes(Pids[isite],
								vtime, time_zone, language,
								Plabs[isite], fpos, fdist, fbear, flen,
								xdir, xspd, lchain->attrib,
								cat_cascade, cat_attrib, num_catatt,
								attribs, num_attrib, list_case, num_list,
								inmark, markscale, display_name,
								display_type, fit_to_map, fit_to_map_ref,
								rotation, rotate_lat, rotate_lon,
								rot_attrib, constrain_rot,
								xxo, yyo, xx, yy) )
							break;

						/* Attributes did not match                    */
						/*  ... so remove this link chain from the set */
						(void) remove_item_from_set(copyset, (ITEM) lchain);
						}

					/* Destroy what is left of the copy */
					(void) destroy_set(copyset);
					break;

				/* All other type fields have already been handled */
				default:
					break;
				}
			}

		/* Free the sampling structures */
		switch ( fkind )
			{

			/* Free sampling structures for continuous type fields */
			case FpaC_CONTINUOUS:
				(void) free_vlist(vlist);
				FREEMEM(vlist);
				break;

			/* Free sampling structures for vector type fields */
			case FpaC_VECTOR:
				(void) free_vlist(mlist);
				(void) free_vlist(dlist);
				FREEMEM(mlist);
				FREEMEM(dlist);
				break;

			/* Free sampling structures for discrete type fields */
			case FpaC_DISCRETE:
				(void) destroy_set(areas);
				break;

			/* Free sampling structures for line type fields */
			case FpaC_LINE:
				(void) destroy_set(curves);
				break;

			/* Free sampling structures for scattered type fields */
			case FpaC_SCATTERED:
				(void) destroy_set(spots);
				break;

			/* Free sampling structures for link chain type fields */
			case FpaC_LCHAIN:
				(void) destroy_set(lchains);
				break;
			}

		/* Free space used by idents and labels */
		FREELIST(Pids,  Nump);
		FREELIST(Plabs, Nump);

		/* Return TRUE when all look up table locations have been sampled */
		return TRUE;
		}

	/* Sample fields by latitude and longitude */
	else if ( ( !blank(lat) && !blank(lon) )
				|| ( !blank(map_x) && !blank(map_y) )
				|| ( !blank(loc_ident) && !blank(loc_lookup) ) )
		{
		if ( Verbose )
			{
			if ( !blank(data_file) )
				(void) fprintf(stdout,
						"Sampling from data file %s\n", data_file);
			else if ( !blank(geo_file) )
				(void) fprintf(stdout,
						"Sampling for field ... %s %s from map file %s\n",
						element, level, geo_file);
			else if ( !blank(equation) )
				(void) fprintf(stdout,
						" Sampling for equation ... %s from %s at %s\n",
						equation, source, vtime);
			else
				(void) fprintf(stdout,
						" Sampling for field ... %s %s from %s at %s\n",
						element, level, source, vtime);
			}

		/* Set the latitude and longitude */
		if ( !blank(lat) && !blank(lon) )
			{
			flat = read_lat(lat, &status);
			if ( !status ) (void) error_report("Problem with lat");
			flon = read_lon(lon, &status);
			if ( !status ) (void) error_report("Problem with lon");

			/* Convert latitude and longitude to map position */
			(void) ll_to_pos(&BaseMap, flat, flon, pos[0]);
			loclab = FpaCblank;
			}

		/* Set the map position (adjusted by map_units) */
		else if ( !blank(map_x) && !blank(map_y) )
			{
			(void) sscanf(map_x, "%f", &pos[0][X]);
			(void) sscanf(map_y, "%f", &pos[0][Y]);
			fact = map_units / BaseMap.definition.units;
			pos[0][X] *= fact;
			pos[0][Y] *= fact;

			/* Convert map position to latitude and longitude */
			(void) pos_to_ll(&BaseMap, pos[0], &flat, &flon);
			loclab = FpaCblank;
			}

		/* Get the latitude and longitude from location look up table */
		else
			{
			if ( Verbose )
				{
				(void) fprintf(stdout, "   Matching location ident ... \"%s\"",
						loc_ident);
				(void) fprintf(stdout, "  from look up ... %s\n",
						loc_lookup);
				}
			if ( !match_location_lookup(loc_lookup, loc_ident,
					vtime, &loclat, &loclon, &loclab) )
				{
				(void) sprintf(err_buf,
						"Error matching \"%s\" in look up ... %s",
						loc_ident, loc_lookup);
				(void) error_report(err_buf);
				}
			flat = read_lat(loclat, &status);
			if ( !status )
				(void) error_report("Problem with location look up lat");
			flon = read_lon(loclon, &status);
			if ( !status )
				(void) error_report("Problem with location look up lon");

			/* Convert latitude and longitude to map position */
			(void) ll_to_pos(&BaseMap, flat, flon, pos[0]);
			}

		/* Check for sample locations off the map */
		if ( pos[0][X] < 0.0 || pos[0][Y] < 0.0
				|| pos[0][X] > BaseMap.definition.xlen
				|| pos[0][Y] > BaseMap.definition.ylen )
			{
			if ( Verbose )
				{
				if ( !blank(map_x) && !blank(map_y) )
					(void) fprintf(stdout,
							"  Sample position off map at ... %f %f\n",
							pos[0][X], pos[0][Y]);
				else
					(void) fprintf(stdout,
							"  Sample lat/lon off map at ... %.1f %.1f\n",
							flat, flon);
				}

			/* Return (if requested) for map samples off map */
			if ( AnchorToMap ) return TRUE;
			else if ( fit_to_map ) return TRUE;
			}

		/* Comments for sample locations on the map */
		else
			{
			if ( Verbose )
				{
				if ( !blank(map_x) && !blank(map_y) )
					(void) fprintf(stdout,
							"  Sample position at ... %f %f\n",
							pos[0][X], pos[0][Y]);
				else
					(void) fprintf(stdout,
							"  Sample lat/lon at ... %.1f %.1f\n",
							flat, flon);
				}
			}

		/* Set display location on the current map */
		if ( AnchorToMap )
			{
			(void) anchored_location(pos[0], 0.0, 0.0, &xxo, &yyo);
			}

		/* Set display location to current anchor position */
		else
			{
			(void) anchored_location(ZeroPoint, 0.0, 0.0, &xxo, &yyo);
			}

		/* Set display location (offset by xdoff/ydoff) */
		/*  and offset location for sampled attributes */
		xxo += xdoff;
		yyo += ydoff;
		xx = xxo + xoff;
		yy = yyo + yoff;

		/* Sample from data files */
		if ( !blank(data_file) )
			{

			/* Convert latitude and longitude to STRING format */
			(void) sprintf(mlat, "%f", flat);
			(void) sprintf(mlon, "%f", flon);

			/* Extract attributes from matching data file entry */
			if ( !match_data_file(data_file, data_file_format,
					data_file_units, data_file_wind_units,
					loc_ident, mlat, mlon, vtime, &cal) )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
						"Cannot match ident/lat/lon/vtime ... %s/%f/%f/%s  in data file ... %s",
						loc_ident, flat, flon, vtime, data_file);
					}
				return TRUE;
				}

			/* Build a spot object from the data file attributes */
			spot = create_spot(pos[0], FpaCblank, AttachNone, cal);
			}

		/* Sample fields from geography files depending on field type */
		else if ( !blank(geo_file) )
			{
			switch ( fkind )
				{

				/* Sample continuous type fields */
				case FpaC_CONTINUOUS:

					/* Create VLIST Object to hold sampled value */
					vlist = INITMEM(VLIST, 1);
					(void) init_vlist(vlist);

					/* Sample field at the requested position */
					status = eval_sfc(gsfc, pos[0], &dval);

					/* Return if field cannot be evaluated */
					if ( !status )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot sample field ... %s %s from map file %s\n",
								element, level, geo_file);
							}
						(void) free_vlist(vlist);
						FREEMEM(vlist);
						return TRUE;
						}

					/* Add value to VLIST Object */
					(void) add_point_to_vlist(vlist, pos[0], (float) dval);
					break;

				/* Sample vector type fields */
				case FpaC_VECTOR:

					/* Create VLIST Objects to hold sampled values */
					mlist = INITMEM(VLIST, 1);
					(void) init_vlist(mlist);
					dlist = INITMEM(VLIST, 1);
					(void) init_vlist(dlist);

					/* Sample field at the requested position */
					status = eval_sfc_MD(gsfc, pos[0], &mval, &dval);

					/* Return if field cannot be evaluated */
					if ( !status )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot sample field ... %s %s from map file %s\n",
								element, level, geo_file);
							}
						(void) free_vlist(mlist);
						(void) free_vlist(dlist);
						FREEMEM(mlist);
						FREEMEM(dlist);
						return TRUE;
						}

					/* Add values to VLIST Objects */
					(void) add_point_to_vlist(mlist, pos[0], (float) mval);
					(void) add_point_to_vlist(dlist, pos[0], (float) dval);
					break;

				/* Sample discrete type fields */
				case FpaC_DISCRETE:

					/* Copy the discrete areas */
					areas = copy_set(gset);

					/* Return if field cannot be evaluated */
					if ( IsNull(areas) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from map file %s\n",
								element, level, geo_file);
							}
						return TRUE;
						}
					break;

				/* Sample line type fields */
				case FpaC_LINE:

					/* Copy the line curves */
					curves = copy_set(gset);

					/* Return if field cannot be evaluated */
					if ( IsNull(curves) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from map file %s\n",
								element, level, geo_file);
							}
						return TRUE;
						}
					break;

				/* Sample scattered type fields */
				case FpaC_SCATTERED:

					/* Copy the scattered spots */
					spots = copy_set(gset);

					/* Return if field cannot be evaluated */
					if ( IsNull(spots) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from map file %s\n",
								element, level, geo_file);
							}
						return TRUE;
						}
					break;

				/* Sample link chain type fields */
				case FpaC_LCHAIN:

					/* Copy the link chains */
					lchains = copy_set(gset);

					/* Return if field cannot be evaluated */
					if ( IsNull(lchains) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from map file %s\n",
								element, level, geo_file);
							}
						return TRUE;
						}
					break;

				/* Cannot sample other types of fields */
				default:
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot sample field ... %s %s from map file %s\n",
							element, level, geo_file);
						}
					return TRUE;
				}
			}

		/* Sample fields from all other files depending on field type */
		else
			{
			switch ( fkind )
				{

				/* Sample continuous type fields */
				case FpaC_CONTINUOUS:

					/* Extract field value by equation */
					if ( !blank(equation) )
						vlist = retrieve_vlist_by_equation(&descript, 1, pos,
													FpaCmksUnits, equation);

					/* Extract field value directly */
					else
						vlist = retrieve_vlist(&descript, 1, pos);

					/* Return if field cannot be evaluated */
					if ( IsNull(vlist) )
						{
						if ( Verbose )
							{
							if ( !blank(equation) )
								(void) fprintf(stdout,
									"Cannot calculate equation ... %s from %s at %s\n",
									equation, source, vtime);
							else
								(void) fprintf(stdout,
									"Cannot extract field ... %s %s from %s at %s\n",
									element, level, source, vtime);
							}
						return TRUE;
						}
					break;

				/* Sample vector type fields */
				case FpaC_VECTOR:

					/* Extract magnitude and direction of vector field */
					mlist = retrieve_vlist_component(&descript, M_Comp, 1, pos);
					dlist = retrieve_vlist_component(&descript, D_Comp, 1, pos);

					/* Return if field cannot be evaluated */
					if ( IsNull(mlist) || IsNull(dlist) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from %s at %s\n",
								element, level, source, vtime);
							}
						return TRUE;
						}
					break;

				/* Sample discrete type fields */
				case FpaC_DISCRETE:

					/* Find the field containing the discrete areas */
					areas = retrieve_areaset(&descript);

					/* Return if field cannot be evaluated */
					if ( IsNull(areas) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from %s at %s\n",
								element, level, source, vtime);
							}
						return TRUE;
						}
					break;

				/* Sample line type fields */
				case FpaC_LINE:

					/* Find the field containing the line curves */
					curves = retrieve_curveset(&descript);

					/* Return if field cannot be evaluated */
					if ( IsNull(curves) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from %s at %s\n",
								element, level, source, vtime);
							}
						return TRUE;
						}
					break;

				/* Sample scattered type fields */
				case FpaC_SCATTERED:

					/* Find the field containing the scattered spots */
					spots = retrieve_spotset(&descript);

					/* Return if field cannot be evaluated */
					if ( IsNull(spots) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from %s at %s\n",
								element, level, source, vtime);
							}
						return TRUE;
						}
					break;

				/* Sample link chain type fields */
				case FpaC_LCHAIN:

					/* Find the field containing the link chains */
					lchains = retrieve_lchainset(&descript);

					/* Return if field cannot be evaluated */
					if ( IsNull(lchains) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from %s at %s\n",
								element, level, source, vtime);
							}
						return TRUE;
						}
					break;

				/* Cannot sample other types of fields */
				default:
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot sample field ... %s %s from %s at %s\n",
							element, level, source, vtime);
						}
					return TRUE;
				}
			}

		/* Display sampled value depending on field type */
		status = TRUE;
		switch ( fkind )
			{

			/* Display sampled value for continuous type fields */
			case FpaC_CONTINUOUS:

				/* Convert sampled value to required units */
				(void) convert_value(FpaCmksUnits, (double) vlist->val[0],
						units, &dval);

				/* Display the field value */
				(void) GRA_display_sampled_value(loc_ident,
						vtime, time_zone, language,
						loclab, pos[0], (float) dval, attribs, num_attrib,
						list_case, num_list, inmark, markscale,
						display_name, display_type, fit_to_map,
						fit_to_map_ref, rotation, xxo, yyo, xx, yy);
				break;

			/* Display sampled value for vector type fields */
			case FpaC_VECTOR:

				/* Convert sampled magnitude to required units */
				(void) convert_value(FpaCmksUnits, (double) mlist->val[0],
						units, &mval);

				/* Convert sampled direction to degrees true */
				dang = wind_dir_true(&BaseMap, flat, flon, dlist->val[0]);

				/* >>>>> debug testing for vector fields in sample_field() <<<<< */
				if ( DebugMode )
					{
					(void) fprintf(stdout,
						"Vector field at %.1f %.1f ... m/d: %.1f %.1f  mval/dang: %.1f %.1f\n",
						flat, flon, mlist->val[0], dlist->val[0], mval, dang);
					}
				/* >>>>> debug testing for vector fields in sample_field() <<<<< */

				/* Display the vector */
				(void) GRA_display_sampled_vector(loc_ident,
						vtime, time_zone, language,
						loclab, pos[0], (float) mval, dang,
						attribs, num_attrib, list_case, num_list,
						inmark, markscale, display_name, display_type,
						fit_to_map, fit_to_map_ref,
						rotation, xxo, yyo, xx, yy);
				break;

			/* Display sampled value for discrete type fields */
			case FpaC_DISCRETE:

				/* Sample areas based on proximity */
				if ( proximity > 0.0 )
					{

					/* Make a copy of the area or subareas in the area */
					copyset = create_set("area");
					for ( iarea=0; iarea<areas->num; iarea++ )
						{
						area = (AREA) areas->list[iarea];

						/* Ensure that area is within proximity */
						(void) area_test_point(area, pos[0], NULL,
												fpos, NULL, NULL, NULL,
												&inside);
						if ( !inside )
							{
							fdist  = great_circle_distance(&BaseMap,
															pos[0], fpos);
							fdist /= 1000.0;
							if ( fdist > proximity ) continue;
							}

						/* Copy the area if no subareas */
						if ( area->numdiv == 0)
							{
							carea = copy_area(area, FALSE);
							if ( NotNull(carea) )
								{
								(void) add_item_to_set(copyset,
														(ITEM) carea);
								}
							}

						/* Create an area from each subarea */
						else
							{
							for ( isub=0; isub<=area->numdiv; isub++ )
								{
								subarea = area->subareas[isub];
								carea = area_from_subarea(subarea);
								if ( NotNull(carea) )
									{
									(void) add_item_to_set(copyset,
															(ITEM) carea);
									}
								}
							}
						}

					/* Search for closest area within proximity */
					/*  that matches the required attributes    */
					while ( copyset->num > 0 )
						{

						/* Find an enclosing area */
						area = enclosing_area(copyset, pos[0],
											PickFirst, NullFloat, NullChar);

						/* Set sampling point and proximity (if inside) */
						if ( NotNull(area) )
							{
							(void) copy_point(fpos, pos[0]);
							inside = TRUE;
							fdist  = 0.0;
							fbear  = 0.0;
							}

						/* Find the closest area within proximity */
						else
							{
							area   = closest_area(copyset, pos[0],
											NULL, fpos, NULL, NULL, NULL);
							inside = FALSE;
							fdist  = great_circle_distance(&BaseMap,
														pos[0], fpos);
							fdist /= 1000.0;
							if ( fdist > proximity )
								{
								if ( Verbose )
									{
									if ( !blank(map_x) && !blank(map_y) )
										(void) fprintf(stdout,
												"  Feature beyond proximity for position ... %f %f\n",
												pos[0][X], pos[0][Y]);
									else
										(void) fprintf(stdout,
												"  Feature beyond proximity for lat/lon ... %.1f %.1f\n",
												flat, flon);
									}

								break;
								}

							/* Determine bearing from sample to feature */
							fbear  = great_circle_bearing(&BaseMap,
														pos[0], fpos);
							}

						/* Check if feature is on the map */
						if ( fpos[X] < 0.0 || fpos[Y] < 0.0
								|| fpos[X] > BaseMap.definition.xlen
								|| fpos[Y] > BaseMap.definition.ylen )
							{
							if ( Verbose )
								{
								if ( !blank(map_x) && !blank(map_y) )
									(void) fprintf(stdout,
											"  Feature off map for position ... %f %f\n",
											pos[0][X], pos[0][Y]);
								else
									(void) fprintf(stdout,
											"  Feature off map for lat/lon ... %.1f %.1f\n",
											flat, flon);
								}

							if ( AnchorToMap || fit_to_map )
								{
								(void) remove_item_from_set(copyset,
																(ITEM) area);
								continue;
								}
							}

						/* Reset the sampling location (if not inside) */
						if ( !inside && AnchorToMap )
							{
							(void) anchored_location(fpos, 0.0, 0.0,
																&xxo, &yyo);
							xxo += xdoff;
							yyo += ydoff;
							}

						/* Display area attributes ... if they match! */
						if ( GRA_display_sampled_attributes(loc_ident,
								vtime, time_zone, language,
								loclab, fpos, fdist, fbear, DefLineLen,
								DefSegDir, DefSegSpd, area->attrib,
								cat_cascade, cat_attrib, num_catatt,
								attribs, num_attrib, list_case, num_list,
								inmark, markscale, display_name,
								display_type, fit_to_map, fit_to_map_ref,
								rotation, rotate_lat, rotate_lon,
								rot_attrib, constrain_rot,
								xxo, yyo, xx, yy) )
							break;

						/* Attributes did not match              */
						/*  ... so remove this area from the set */
						(void) remove_item_from_set(copyset, (ITEM) area);
						}

					/* Destroy what is left of the copy */
					(void) destroy_set(copyset);
					break;
					}

				/* Sample areas directly */
				else
					{

					/* Find area enclosing point                        */
					/* Note that this should return a background value  */
					/*  with no subarea if no enclosing area is found   */
					/*  ... except map files, which have no background! */
					if ( !eval_areaset(areas, pos[0], PickFirst,
							&subarea, &cal) )
						{
						if ( blank(geo_file) && Verbose )
							{
							(void) fprintf(stdout,
								"No areas or background for field ... %s %s from %s at %s\n",
								element, level, source, vtime);
							}
						status = FALSE;
						break;
						}

					/* Display area attributes from attribute structure */
					(void) GRA_display_sampled_attributes(loc_ident,
							vtime, time_zone, language,
							loclab, pos[0], DefProximity, DefBearing,
							DefLineLen, DefSegDir, DefSegSpd,
							cal, cat_cascade, cat_attrib, num_catatt,
							attribs, num_attrib, list_case, num_list,
							inmark, markscale, display_name,
							display_type, fit_to_map, fit_to_map_ref,
							rotation, rotate_lat, rotate_lon,
							rot_attrib, constrain_rot,
							xxo, yyo, xx, yy);
					}
				break;

			/* Display sampled value for line type fields */
			case FpaC_LINE:

				/* Make a copy of the line curves */
				copyset = copy_set(curves);

				/* Search for closest curve within proximity */
				/*  that matches the required attributes     */
				while ( copyset->num > 0 )
					{

					/* Find the closest curve */
					curve = closest_curve(copyset, pos[0],
											NullFloat, fpos, &iseg);
					if ( IsNull(curve) || IsNull(curve->attrib) )
						{
						(void) remove_item_from_set(copyset, (ITEM) curve);
						continue;
						}

					/* Check the proximity */
					fdist  = great_circle_distance(&BaseMap, pos[0], fpos);
					fdist /= 1000.0;
					if ( fdist > proximity )
						{
						if ( Verbose )
							{
							if ( !blank(map_x) && !blank(map_y) )
								(void) fprintf(stdout,
										"  Feature beyond proximity for position ... %f %f\n",
										pos[0][X], pos[0][Y]);
							else
								(void) fprintf(stdout,
										"  Feature beyond proximity for lat/lon ... %.1f %.1f\n",
										flat, flon);
							}

						break;
						}

					/* Check if feature is on the map */
					if ( fpos[X] < 0.0 || fpos[Y] < 0.0
							|| fpos[X] > BaseMap.definition.xlen
							|| fpos[Y] > BaseMap.definition.ylen )
						{
						if ( Verbose )
							{
							if ( !blank(map_x) && !blank(map_y) )
								(void) fprintf(stdout,
										"  Feature off map for position ... %f %f\n",
										pos[0][X], pos[0][Y]);
							else
								(void) fprintf(stdout,
										"  Feature off map for lat/lon ... %.1f %.1f\n",
										flat, flon);
							}

						if ( AnchorToMap || fit_to_map )
							{
							(void) remove_item_from_set(copyset, (ITEM) curve);
							continue;
							}
						}

					/* Determine bearing from sample to feature */
					fbear  = great_circle_bearing(&BaseMap, pos[0], fpos);

					/* Change sign of proximity wrt curve sense */
					(void) recall_curve_sense(curve, &sense);
					(void) curve_test_point(curve, pos[0], NullFloat,
							NullPoint, NullInt, NullLogical, &right);
					if ( (sense == Right && right)
							|| (sense == Left && !right) ) fdist = -fdist;

					/* Determine length of curve */
					flen = great_circle_line_length(&BaseMap, curve->line);

					/* Reset the sampling location */
					if ( AnchorToMap )
						{
						(void) anchored_location(fpos, 0.0, 0.0, &xxo, &yyo);
						xxo += xdoff;
						yyo += ydoff;

						/* Reset the display location (if required) */
						if ( display_at_feature )
							{
							xx = xxo + xoff;
							yy = yyo + yoff;
							}
						}

					/* Determine direction of curve */
					(void) line_span_info(curve->line, iseg,
												spos, epos, NullPoint);
					xdir = great_circle_bearing(&BaseMap, spos, epos);

					/* Display curve attributes ... if they match! */
					if ( GRA_display_sampled_attributes(loc_ident,
							vtime, time_zone, language,
							loclab, fpos, fdist, fbear, flen,
							xdir, DefSegSpd, curve->attrib,
							cat_cascade, cat_attrib, num_catatt,
							attribs, num_attrib, list_case, num_list,
							inmark, markscale, display_name,
							display_type, fit_to_map, fit_to_map_ref,
							rotation, rotate_lat, rotate_lon,
							rot_attrib, constrain_rot,
							xxo, yyo, xx, yy) )
						break;

					/* Attributes did not match               */
					/*  ... so remove this curve from the set */
					(void) remove_item_from_set(copyset, (ITEM) curve);
					}

				/* Destroy what is left of the copy */
				(void) destroy_set(copyset);
				break;

			/* Display sampled value for scattered type fields */
			case FpaC_SCATTERED:

				/* Display attributes from data file attribute structure */
				if ( !blank(data_file) )
					{
					(void) GRA_display_sampled_attributes(loc_ident,
							vtime, time_zone, language,
							loclab, spot->anchor, DefProximity, DefBearing,
							DefLineLen, DefSegDir, DefSegSpd, spot->attrib,
							cat_cascade, cat_attrib, num_catatt,
							attribs, num_attrib, list_case, num_list,
							inmark, markscale, display_name,
							display_type, fit_to_map, fit_to_map_ref,
							rotation, rotate_lat, rotate_lon,
							rot_attrib, constrain_rot,
							xxo, yyo, xx, yy);
					}

				/* Display attributes from closest spot attribute structure */
				else
					{
					/* Make a copy of the scattered spots */
					copyset = copy_set(spots);

					/* Search for closest spot within proximity */
					/*  that matches the required attributes    */
					while ( copyset->num > 0 )
						{

						/* Find the closest spot */
						spot = closest_spot(copyset, pos[0], NULL, NULL,
												NullFloat, fpos);
						if ( IsNull(spot) || IsNull(spot->attrib) )
							{
							(void) remove_item_from_set(copyset, (ITEM) spot);
							continue;
							}

						/* Check the proximity */
						fdist  = great_circle_distance(&BaseMap, pos[0], fpos);
						fdist /= 1000.0;
						if ( fdist > proximity )
							{
							if ( Verbose )
								{
								if ( !blank(map_x) && !blank(map_y) )
									(void) fprintf(stdout,
											"  Feature beyond proximity for position ... %f %f\n",
											pos[0][X], pos[0][Y]);
								else
									(void) fprintf(stdout,
											"  Feature beyond proximity for lat/lon ... %.1f %.1f\n",
											flat, flon);
								}

							break;
							}

						/* Determine bearing from sample to feature */
						fbear  = great_circle_bearing(&BaseMap, pos[0], fpos);

						/* Check if feature is on the map */
						if ( fpos[X] < 0.0 || fpos[Y] < 0.0
								|| fpos[X] > BaseMap.definition.xlen
								|| fpos[Y] > BaseMap.definition.ylen )
							{
							if ( Verbose )
								{
								if ( !blank(map_x) && !blank(map_y) )
									(void) fprintf(stdout,
											"  Feature off map for position ... %f %f\n",
											pos[0][X], pos[0][Y]);
								else
									(void) fprintf(stdout,
											"  Feature off map for lat/lon ... %.1f %.1f\n",
											flat, flon);
								}

							if ( AnchorToMap || fit_to_map )
								{
								(void) remove_item_from_set(copyset, (ITEM) spot);
								continue;
								}
							}

						/* Reset the sampling location */
						if ( AnchorToMap )
							{
							(void) anchored_location(fpos, 0.0, 0.0, &xxo, &yyo);
							xxo += xdoff;
							yyo += ydoff;

							/* Reset the display location (if required) */
							if ( display_at_feature )
								{
								xx = xxo + xoff;
								yy = yyo + yoff;
								}
							}

						/* Display spot attributes ... if they match! */
						if ( GRA_display_sampled_attributes(loc_ident,
								vtime, time_zone, language,
								loclab, fpos, fdist, fbear, DefLineLen,
								DefSegDir, DefSegSpd, spot->attrib,
								cat_cascade, cat_attrib, num_catatt,
								attribs, num_attrib, list_case, num_list,
								inmark, markscale, display_name,
								display_type, fit_to_map, fit_to_map_ref,
								rotation, rotate_lat, rotate_lon,
								rot_attrib, constrain_rot,
								xxo, yyo, xx, yy) )
							break;

						/* Attributes did not match              */
						/*  ... so remove this spot from the set */
						(void) remove_item_from_set(copyset, (ITEM) spot);
						}

					/* Destroy what is left of the copy */
					(void) destroy_set(copyset);
					}
				break;

			/* Display sampled value for link chain type fields */
			case FpaC_LCHAIN:

				/* >>>>> Display link chain based on type? <<<<< */

				/* Make a copy of the link chains */
				copyset = copy_set(lchains);

				/* Search for closest link chain within proximity */
				/*  that matches the required attributes          */
				while ( copyset->num > 0 )
					{

					/* Find the closest link chain */
					lchain = closest_lchain(copyset, pos[0],
											NullFloat, fpos, &iseg);
					if ( IsNull(lchain) || IsNull(lchain->attrib) )
						{
						(void) remove_item_from_set(copyset, (ITEM) lchain);
						continue;
						}

					/* Check the proximity */
					fdist  = great_circle_distance(&BaseMap, pos[0], fpos);
					fdist /= 1000.0;
					if ( fdist > proximity )
						{
						if ( Verbose )
							{
							if ( !blank(map_x) && !blank(map_y) )
								(void) fprintf(stdout,
										"  Feature beyond proximity for position ... %f %f\n",
										pos[0][X], pos[0][Y]);
							else
								(void) fprintf(stdout,
										"  Feature beyond proximity for lat/lon ... %.1f %.1f\n",
										flat, flon);
							}

						break;
						}

					/* Check if feature is on the map */
					if ( fpos[X] < 0.0 || fpos[Y] < 0.0
							|| fpos[X] > BaseMap.definition.xlen
							|| fpos[Y] > BaseMap.definition.ylen )
						{
						if ( Verbose )
							{
							if ( !blank(map_x) && !blank(map_y) )
								(void) fprintf(stdout,
										"  Feature off map for position ... %f %f\n",
										pos[0][X], pos[0][Y]);
							else
								(void) fprintf(stdout,
										"  Feature off map for lat/lon ... %.1f %.1f\n",
										flat, flon);
							}

						if ( AnchorToMap || fit_to_map )
							{
							(void) remove_item_from_set(copyset, (ITEM) lchain);
							continue;
							}
						}

					/* Determine bearing from sample to feature */
					fbear  = great_circle_bearing(&BaseMap, pos[0], fpos);

					/* Interpolate the link chain (if required) */
					if ( lchain->dointerp ) (void) interpolate_lchain(lchain);

					/* Determine length of link chain track */
					flen = great_circle_line_length(&BaseMap, lchain->track);

					/* Reset the sampling location */
					if ( AnchorToMap )
						{
						(void) anchored_location(fpos, 0.0, 0.0, &xxo, &yyo);
						xxo += xdoff;
						yyo += ydoff;

						/* Reset the display location (if required) */
						if ( display_at_feature )
							{
							xx = xxo + xoff;
							yy = yyo + yoff;
							}
						}

					/* Determine direction and speed on link chain track */
					if ( lchain->inum > 1 )
						{
						(void) line_span_info(lchain->track, iseg,
													spos, epos, NullPoint);
						xdir  = great_circle_bearing(&BaseMap, spos, epos);
						xspd  = point_dist(spos, epos) / (float) lchain->minterp;
						xspd *= BaseMap.definition.units;
						xspd /= 60.0;
						}
					else
						{
						xdir = DefSegDir;
						xspd = DefSegSpd;
						}

					/* Display link chain attributes ... if they match! */
					if ( GRA_display_sampled_attributes(loc_ident,
							vtime, time_zone, language,
							loclab, fpos, fdist, fbear, flen, xdir, xspd,
							lchain->attrib, cat_cascade, cat_attrib, num_catatt,
							attribs, num_attrib, list_case, num_list,
							inmark, markscale, display_name,
							display_type, fit_to_map, fit_to_map_ref,
							rotation, rotate_lat, rotate_lon,
							rot_attrib, constrain_rot,
							xxo, yyo, xx, yy) )
						break;

					/* Attributes did not match                    */
					/*  ... so remove this link chain from the set */
					(void) remove_item_from_set(copyset, (ITEM) lchain);
					}

				/* Destroy what is left of the copy */
				(void) destroy_set(copyset);
				break;

			/* All other type fields have already been handled */
			default:
				status = FALSE;
				break;
			}

		/* Free the sampling structures */
		switch ( fkind )
			{

			/* Free sampling structures for continuous type fields */
			case FpaC_CONTINUOUS:
				(void) free_vlist(vlist);
				FREEMEM(vlist);
				break;

			/* Free sampling structures for vector type fields */
			case FpaC_VECTOR:
				(void) free_vlist(mlist);
				(void) free_vlist(dlist);
				FREEMEM(mlist);
				FREEMEM(dlist);
				break;

			/* Free sampling structures for discrete type fields */
			case FpaC_DISCRETE:
				(void) destroy_set(areas);
				break;

			/* Free sampling structures for line type fields */
			case FpaC_LINE:
				(void) destroy_set(curves);
				break;

			/* Free sampling structures for scattered type fields */
			case FpaC_SCATTERED:
				if ( !blank(data_file) ) (void) destroy_spot(spot);
				else                     (void) destroy_set(spots);
				break;

			/* Free sampling structures for link chain type fields */
			case FpaC_LCHAIN:
				(void) destroy_set(lchains);
				break;
			}

		/* Return results of sampling display */
		return status;
		}

	/* Sample fields for each table site */
	else if ( !blank(table_name) )
		{
		if ( Verbose )
			{
			if ( !blank(data_file) )
				(void) fprintf(stdout,
						"Table sampling from data file %s\n", data_file);
			else if ( !blank(geo_file) )
				(void) fprintf(stdout,
						"Table sampling for field ... %s %s from map file %s\n",
						element, level, geo_file);
			else if ( !blank(equation) )
				(void) fprintf(stdout,
						" Table sampling for equation ... %s from %s at %s\n",
						equation, source, vtime);
			else
				(void) fprintf(stdout,
						" Table sampling for field ... %s %s from %s at %s\n",
						element, level, source, vtime);
			}

		/* Ensure that table locations are NOT anchored to current map */
		/*  ... regardless of the current anchor position!             */
		cur_anchor  = AnchorToMap;
		AnchorToMap = FALSE;

		/* Find the named table */
		cur_table = get_table(table_name);
		if ( IsNull(cur_table) )
			{
			(void) sprintf(err_buf, "Table ... %s ... not yet defined",
					table_name);
			(void) error_report(err_buf);
			}

		/* Set sample locations for all table sites */
		Nump  = cur_table->nsites;
		Ppos  = GETMEM(Ppos,  POINT,  Nump);
		Plats = GETMEM(Plats, float,  Nump);
		Plons = GETMEM(Plons, float,  Nump);
		Pids  = GETMEM(Pids,  STRING, Nump);
		Plabs = GETMEM(Plabs, STRING, Nump);
		for ( isite=0; isite<Nump; isite++ )
			{

			/* Set latitude/longitude for sampling */
			if ( cur_table->usell[isite] )
				{
				flat   = cur_table->flats[isite];
				flon   = cur_table->flons[isite];
				loclab = FpaCblank;
				}

			/* Get latitude/longitude for sampling from location look up */
			else
				{
				if ( blank(loc_lookup) )
					(void) error_report("No location look up table");
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"   Matching location ident ... \"%s\"",
							cur_table->idents[isite]);
					(void) fprintf(stdout, "  from look up ... %s\n",
							loc_lookup);
					}
				if ( !match_location_lookup(loc_lookup,
						cur_table->idents[isite], vtime, &loclat, &loclon,
						&loclab) )
					{
					(void) sprintf(err_buf,
							"Error matching \"%s\" in look up ... %s",
							cur_table->idents[isite], loc_lookup);
					(void) error_report(err_buf);
					}
				flat = read_lat(loclat, &status);
				if ( !status )
					(void) error_report("Problem with location look up lat");
				flon = read_lon(loclon, &status);
				if ( !status )
					(void) error_report("Problem with location look up lon");
				}

			/* Set map position for sampling */
			Plats[isite] = flat;
			Plons[isite] = flon;
			(void) ll_to_pos(&BaseMap, flat, flon, Ppos[isite]);
			Pids[isite]  = safe_strdup(cur_table->idents[isite]);
			Plabs[isite] = safe_strdup(loclab);
			}

		/* Sample at all table sites from data files */
		if ( !blank(data_file) )
			{

			/* Create a set of spots from all table sites */
			spots = create_set("spot");
			for ( isite=0; isite<Nump; isite++ )
				{

				/* Convert latitude and longitude to STRING format */
				(void) sprintf(mlat, "%f", Plats[isite]);
				(void) sprintf(mlon, "%f", Plons[isite]);

				/* Extract attributes from matching data file entry */
				if ( !match_data_file(data_file, data_file_format,
						data_file_units, data_file_wind_units,
						Pids[isite], mlat, mlon, vtime, &cal) )
					{
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot match ident/lat/lon/vtime ... %s/%f/%f/%s  in data file ... %s",
							Pids[isite], Plats[isite], Plons[isite], vtime,
							data_file);
						}
					}

				/* Build a spot object from the data file attributes */
				spot = create_spot(Ppos[isite], FpaCblank, AttachNone, cal);

				/* Add the spot object to the set */
				(void) add_item_to_set(spots, (ITEM) spot);
				}
			}

		/* Sample fields at all table sites from geography files */
		/*  depending on field type                              */
		else if ( !blank(geo_file) )
			{
			status = TRUE;
			switch ( fkind )
				{

				/* Sample continuous type fields */
				case FpaC_CONTINUOUS:

					/* Create VLIST Object to hold sampled values */
					vlist = INITMEM(VLIST, 1);
					(void) init_vlist(vlist);

					/* Sample field at all table sites */
					for ( isite=0; isite<Nump; isite++ )
						{

						/* Sample field at table site */
						status = eval_sfc(gsfc, Ppos[isite], &dval);

						/* Error if field cannot be sampled */
						if ( !status )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
									"Cannot sample field ... %s %s from map file %s\n",
									element, level, geo_file);
								}
							(void) free_vlist(vlist);
							FREEMEM(vlist);
							break;
							}

						/* Add value to VLIST Object */
						(void) add_point_to_vlist(vlist, Ppos[isite],
																(float) dval);
						}
					break;

				/* Sample vector type fields */
				case FpaC_VECTOR:

					/* Create VLIST Objects to hold sampled values */
					mlist = INITMEM(VLIST, 1);
					(void) init_vlist(mlist);
					dlist = INITMEM(VLIST, 1);
					(void) init_vlist(dlist);

					/* Sample field at all table sites */
					for ( isite=0; isite<Nump; isite++ )
						{

						/* Sample field at table site */
						status = eval_sfc_MD(gsfc, Ppos[isite], &mval, &dval);

						/* Error if field cannot be sampled */
						if ( !status )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
									"Cannot sample field ... %s %s from map file %s\n",
									element, level, geo_file);
								}
							(void) free_vlist(mlist);
							(void) free_vlist(dlist);
							FREEMEM(mlist);
							FREEMEM(dlist);
							break;
							}

						/* Add values to VLIST Objects */
						(void) add_point_to_vlist(mlist, Ppos[isite],
																(float) mval);
						(void) add_point_to_vlist(dlist, Ppos[isite],
																(float) dval);
						}
					break;

				/* Sample discrete type fields */
				case FpaC_DISCRETE:

					/* Copy the discrete areas */
					areas = copy_set(gset);

					/* Error if field cannot be evaluated */
					if ( IsNull(areas) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from map file %s\n",
								element, level, geo_file);
							}
						status = FALSE;
						}
					break;

				/* Sample line type fields */
				case FpaC_LINE:

					/* Copy the line curves */
					curves = copy_set(gset);

					/* Error if field cannot be evaluated */
					if ( IsNull(curves) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from map file %s\n",
								element, level, geo_file);
							}
						status = FALSE;
						}
					break;

				/* Sample scattered type fields */
				case FpaC_SCATTERED:

					/* Copy the scattered spots */
					spots = copy_set(gset);

					/* Error if field cannot be evaluated */
					if ( IsNull(spots) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from map file %s\n",
								element, level, geo_file);
							}
						status = FALSE;
						}
					break;

				/* Sample link chain type fields */
				case FpaC_LCHAIN:

					/* Copy the link chains */
					lchains = copy_set(gset);

					/* Error if field cannot be evaluated */
					if ( IsNull(lchains) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from map file %s\n",
								element, level, geo_file);
							}
						status = FALSE;
						}
					break;

				/* Cannot sample other types of fields */
				default:
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot sample field ... %s %s from map file %s\n",
							element, level, geo_file);
						}
					status = FALSE;
					break;
				}
			}

		/* Sample fields at all table sites from all other files */
		/*  depending on field type                              */
		else
			{
			status = TRUE;
			switch ( fkind )
				{

				/* Sample continuous type fields */
				case FpaC_CONTINUOUS:

					/* Extract field values by equation */
					if ( !blank(equation) )
						vlist = retrieve_vlist_by_equation(&descript,
											Nump, Ppos, FpaCmksUnits, equation);

					/* Extract field values directly */
					else
						vlist = retrieve_vlist(&descript, Nump, Ppos);

					/* Error if field cannot be evaluated */
					if ( IsNull(vlist) )
						{
						if ( Verbose )
							{
							if ( !blank(equation) )
								(void) fprintf(stdout,
									"Cannot calculate equation ... %s from %s at %s\n",
									equation, source, vtime);
							else
								(void) fprintf(stdout,
									"Cannot extract field ... %s %s from %s at %s\n",
									element, level, source, vtime);
							}
						status = FALSE;
						}
					break;

				/* Sample vector type fields */
				case FpaC_VECTOR:

					/* Extract magnitude and direction of vector field */
					mlist = retrieve_vlist_component(&descript, M_Comp, Nump,
																		Ppos);
					dlist = retrieve_vlist_component(&descript, D_Comp, Nump,
																		Ppos);

					/* Error if field cannot be evaluated */
					if ( IsNull(mlist) || IsNull(dlist) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from %s at %s\n",
								element, level, source, vtime);
							}
						status = FALSE;
						}
					break;

				/* Sample discrete type fields */
				case FpaC_DISCRETE:

					/* Find the field containing the discrete areas */
					areas = retrieve_areaset(&descript);

					/* Error if field cannot be evaluated */
					if ( IsNull(areas) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from %s at %s\n",
								element, level, source, vtime);
							}
						status = FALSE;
						}
					break;

				/* Sample line type fields */
				case FpaC_LINE:

					/* Find the field containing the line curves */
					curves = retrieve_curveset(&descript);

					/* Error if field cannot be evaluated */
					if ( IsNull(curves) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from %s at %s\n",
								element, level, source, vtime);
							}
						status = FALSE;
						}
					break;

				/* Sample scattered type fields */
				case FpaC_SCATTERED:

					/* Find the field containing the scattered spots */
					spots = retrieve_spotset(&descript);

					/* Error if field cannot be evaluated */
					if ( IsNull(spots) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from %s at %s\n",
								element, level, source, vtime);
							}
						status = FALSE;
						}
					break;

				/* Sample link chain type fields */
				case FpaC_LCHAIN:

					/* Find the field containing the link chains */
					lchains = retrieve_lchainset(&descript);

					/* Error if field cannot be evaluated */
					if ( IsNull(lchains) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from %s at %s\n",
								element, level, source, vtime);
							}
						status = FALSE;
						}
					break;

				/* Cannot sample other types of fields */
				default:
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot sample field ... %s %s from %s at %s\n",
							element, level, source, vtime);
						}
					status = FALSE;
					break;
				}
			}

		/* Return now if field could not be sampled */
		if ( !status )
			{

			/* Reset the current anchor */
			AnchorToMap = cur_anchor;

			/* Free space used by idents and labels */
			FREELIST(Pids,  Nump);
			FREELIST(Plabs, Nump);

			/* Return TRUE */
			return TRUE;
			}

		/* Display values for all table sites */
		for ( isite=0; isite<Nump; isite++ )
			{

			/* Check for sample locations off the map */
			if ( Ppos[isite][X] < 0.0 || Ppos[isite][Y] < 0.0
					|| Ppos[isite][X] > BaseMap.definition.xlen
					|| Ppos[isite][Y] > BaseMap.definition.ylen )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Sample lat/lon outside map at ... %.1f %.1f\n",
							Plats[isite], Plons[isite]);
					}
				}

			/* Comments for sample locations on the map */
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Sample lat/lon at ... %.1f %.1f\n",
							Plats[isite], Plons[isite]);
					}
				}

			/* Set table location for displaying */
			xxt = cur_table->x_off;
			yyt = cur_table->y_off;
			if ( same(cur_table->type, TableCol) )
				yyt += cur_table->offset[isite];
			else if ( same(cur_table->type, TableRow) )
				xxt += cur_table->offset[isite];
			(void) anchored_location(ZeroPoint, xxt, yyt, &xxo, &yyo);

			/* Set display location (offset by xdoff/ydoff) */
			/*  and offset location for sampled attributes  */
			xxo += xdoff;
			yyo += ydoff;
			xx = xxo + xoff;
			yy = yyo + yoff;

			/* Display values depending on field type */
			switch ( fkind )
				{

				/* Display values for continuous type fields */
				case FpaC_CONTINUOUS:

					/* Convert value to required units */
					(void) convert_value(FpaCmksUnits,
							(double) vlist->val[isite], units, &dval);

					/* Display the field value */
					(void) GRA_display_sampled_value(Pids[isite],
							vtime, time_zone, language,
							Plabs[isite], Ppos[isite], (float) dval,
							attribs, num_attrib, list_case, num_list,
							inmark, markscale, display_name, display_type,
							fit_to_map, fit_to_map_ref,
							rotation, xxo, yyo, xx, yy);
					break;

				/* Display values for vector type fields */
				case FpaC_VECTOR:

					/* Convert magnitude to required units */
					(void) convert_value(FpaCmksUnits,
							(double) mlist->val[isite], units, &mval);

					/* Convert sampled direction to degrees true */
					dang = wind_dir_true(&BaseMap,
							Plats[isite], Plons[isite], dlist->val[isite]);

					/* >>>>> debug testing for vector fields in sample_field() <<<<< */
					if ( DebugMode )
						{
						(void) fprintf(stdout,
							"Vector field at %.1f %.1f ... m/d: %.1f %.1f  mval/dang: %.1f %.1f\n",
							Plats[isite], Plons[isite], mlist->val[isite], dlist->val[isite],
							mval, dang);
						}
					/* >>>>> debug testing for vector fields in sample_field() <<<<< */

					/* Display the vector */
					(void) GRA_display_sampled_vector(Pids[isite],
							vtime, time_zone, language,
							Plabs[isite], Ppos[isite], (float) mval,
							dang, attribs, num_attrib, list_case, num_list,
							inmark, markscale, display_name, display_type,
							fit_to_map, fit_to_map_ref,
							rotation, xxo, yyo, xx, yy);
					break;

				/* Display values for discrete type fields */
				case FpaC_DISCRETE:

					/* Sample areas based on proximity */
					if ( proximity > 0.0 )
						{

						/* Make a copy of the area or subareas in the area */
						copyset = create_set("area");
						for ( iarea=0; iarea<areas->num; iarea++ )
							{
							area = (AREA) areas->list[iarea];

							/* Ensure that area is within proximity */
							(void) area_test_point(area, Ppos[isite], NULL,
													fpos, NULL, NULL, NULL,
													&inside);
							if ( !inside )
								{
								fdist  = great_circle_distance(&BaseMap,
															Ppos[isite], fpos);
								fdist /= 1000.0;
								if ( fdist > proximity ) continue;
								}

							/* Copy the area if no subareas */
							if ( area->numdiv == 0)
								{
								carea = copy_area(area, FALSE);
								if ( NotNull(carea) )
									{
									(void) add_item_to_set(copyset,
															(ITEM) carea);
									}
								}

							/* Create an area from each subarea */
							else
								{
								for ( isub=0; isub<=area->numdiv; isub++ )
									{
									subarea = area->subareas[isub];
									carea = area_from_subarea(subarea);
									if ( NotNull(carea) )
										{
										(void) add_item_to_set(copyset,
																(ITEM) carea);
										}
									}
								}
							}

						/* Search for closest area within proximity */
						/*  that matches the required attributes    */
						while ( copyset->num > 0 )
							{

							/* Find an enclosing area */
							area = enclosing_area(copyset, Ppos[isite],
												PickFirst, NullFloat, NullChar);

							/* Set sampling point and proximity (if inside) */
							if ( NotNull(area) )
								{
								(void) copy_point(fpos, Ppos[isite]);
								inside = TRUE;
								fdist  = 0.0;
								fbear  = 0.0;
								}

							/* Find the closest area within proximity */
							else
								{
								area   = closest_area(copyset, Ppos[isite],
												NULL, fpos, NULL, NULL, NULL);
								inside = FALSE;
								fdist  = great_circle_distance(&BaseMap,
															Ppos[isite], fpos);
								fdist /= 1000.0;
								if ( fdist > proximity )
									{
									if ( Verbose )
										{
										(void) fprintf(stdout,
												"   Feature beyond proximity for lat/lon ... %.1f %.1f\n",
												Plats[isite], Plons[isite]);
										}

									break;
									}

								/* Determine bearing from sample to feature */
								fbear  = great_circle_bearing(&BaseMap,
															Ppos[isite], fpos);
								}

							/* Check if feature is on the map */
							if ( fpos[X] < 0.0 || fpos[Y] < 0.0
									|| fpos[X] > BaseMap.definition.xlen
									|| fpos[Y] > BaseMap.definition.ylen )
								{
								if ( Verbose )
									{
									(void) fprintf(stdout,
											"   Feature outside map for lat/lon ... %.1f %.1f\n",
											Plats[isite], Plons[isite]);
									}
								}

							/* Display area attributes ... if they match! */
							if ( GRA_display_sampled_attributes(Pids[isite],
									vtime, time_zone, language,
									Plabs[isite], fpos, fdist, fbear,
									DefLineLen, DefSegDir, DefSegSpd,
									area->attrib,
									cat_cascade, cat_attrib, num_catatt,
									attribs, num_attrib, list_case, num_list,
									inmark, markscale, display_name,
									display_type, fit_to_map, fit_to_map_ref,
									rotation, rotate_lat, rotate_lon,
									rot_attrib, constrain_rot,
									xxo, yyo, xx, yy) )
								break;

							/* Attributes did not match              */
							/*  ... so remove this area from the set */
							(void) remove_item_from_set(copyset, (ITEM) area);
							}

						/* Destroy what is left of the copy */
						(void) destroy_set(copyset);
						break;
						}

					/* Sample areas directly */
					else
						{

						/* Find area enclosing point                        */
						/* Note that this should return a background value  */
						/*  with no subarea if no enclosing area is found   */
						/*  ... except map files, which have no background! */
						if ( !eval_areaset(areas, Ppos[isite], PickFirst,
								&subarea, &cal) )
							{
							if ( blank(geo_file) && Verbose )
								{
								(void) fprintf(stdout,
									"No areas or background for field ... %s %s from %s at %s\n",
									element, level, source, vtime);
								}
							continue;
							}

						/* Display attributes from attribute structure */
						(void) GRA_display_sampled_attributes(Pids[isite],
								vtime, time_zone, language,
								Plabs[isite], Ppos[isite], DefProximity,
								DefBearing, DefLineLen, DefSegDir, DefSegSpd,
								cal, cat_cascade, cat_attrib, num_catatt,
								attribs, num_attrib, list_case, num_list,
								inmark, markscale, display_name,
								display_type, fit_to_map, fit_to_map_ref,
								rotation, rotate_lat, rotate_lon,
								rot_attrib, constrain_rot,
								xxo, yyo, xx, yy);
						}
					break;

				/* Display values for line type fields */
				case FpaC_LINE:

					/* Make a copy of the line curves */
					copyset = copy_set(curves);

					/* Search for closest curve within proximity */
					/*  that matches the required attributes     */
					while ( copyset->num > 0 )
						{

						/* Find the closest curve */
						curve = closest_curve(copyset, Ppos[isite],
												NullFloat, fpos, &iseg);
						if ( IsNull(curve) || IsNull(curve->attrib) )
							{
							(void) remove_item_from_set(copyset, (ITEM) curve);
							continue;
							}

						/* Check the proximity */
						fdist  = great_circle_distance(&BaseMap,
														Ppos[isite], fpos);
						fdist /= 1000.0;
						if ( fdist > proximity )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
										"   Feature beyond proximity for lat/lon ... %.1f %.1f\n",
										Plats[isite], Plons[isite]);
								}

							break;
							}

						/* Check if feature is on the map */
						if ( fpos[X] < 0.0 || fpos[Y] < 0.0
								|| fpos[X] > BaseMap.definition.xlen
								|| fpos[Y] > BaseMap.definition.ylen )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
										"   Feature outside map for lat/lon ... %.1f %.1f\n",
										Plats[isite], Plons[isite]);
								}
							}

						/* Determine bearing from sample to feature */
						fbear  = great_circle_bearing(&BaseMap,
														Ppos[isite], fpos);

						/* Change sign of proximity wrt curve sense */
						(void) recall_curve_sense(curve, &sense);
						(void) curve_test_point(curve, Ppos[isite], NullFloat,
								NullPoint, NullInt, NullLogical, &right);
						if ( (sense == Right && right)
								|| (sense == Left && !right) ) fdist = -fdist;

						/* Determine length of curve */
						flen = great_circle_line_length(&BaseMap, curve->line);

						/* Determine direction of curve */
						(void) line_span_info(curve->line, iseg,
													spos, epos, NullPoint);
						xdir = great_circle_bearing(&BaseMap, spos, epos);

						/* Display curve attributes ... if they match! */
						if ( GRA_display_sampled_attributes(Pids[isite],
								vtime, time_zone, language,
								Plabs[isite], fpos, fdist, fbear, flen,
								xdir, DefSegSpd, curve->attrib,
								cat_cascade, cat_attrib, num_catatt,
								attribs, num_attrib, list_case, num_list,
								inmark, markscale, display_name,
								display_type, fit_to_map, fit_to_map_ref,
								rotation, rotate_lat, rotate_lon,
								rot_attrib, constrain_rot,
								xxo, yyo, xx, yy) )
							break;

						/* Attributes did not match               */
						/*  ... so remove this curve from the set */
						(void) remove_item_from_set(copyset, (ITEM) curve);
						}

					/* Destroy what is left of the copy */
					(void) destroy_set(copyset);
					break;

				/* Display values for scattered type fields */
				case FpaC_SCATTERED:

					/* Make a copy of the scattered spots */
					copyset = copy_set(spots);

					/* Search for closest spot within proximity */
					/*  that matches the required attributes    */
					while ( copyset->num > 0 )
						{

						/* Find the closest spot */
						spot = closest_spot(copyset, Ppos[isite], NULL,
												NULL, NullFloat, fpos);
						if ( IsNull(spot) || IsNull(spot->attrib) )
							{
							(void) remove_item_from_set(copyset, (ITEM) spot);
							continue;
							}

						/* Check the proximity */
						fdist  = great_circle_distance(&BaseMap,
														Ppos[isite], fpos);
						fdist /= 1000.0;
						if ( fdist > proximity )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
										"   Feature beyond proximity for lat/lon ... %.1f %.1f\n",
										Plats[isite], Plons[isite]);
								}

							break;
							}

						/* Determine bearing from sample to feature */
						fbear  = great_circle_bearing(&BaseMap,
														Ppos[isite], fpos);

						/* Check if feature is on the map */
						if ( fpos[X] < 0.0 || fpos[Y] < 0.0
								|| fpos[X] > BaseMap.definition.xlen
								|| fpos[Y] > BaseMap.definition.ylen )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
										"   Feature outside map for lat/lon ... %.1f %.1f\n",
										Plats[isite], Plons[isite]);
								}
							}

						/* Display spot attributes ... if they match! */
						if ( GRA_display_sampled_attributes(Pids[isite],
								vtime, time_zone, language,
								Plabs[isite], fpos, fdist, fbear, 
								DefLineLen, DefSegDir, DefSegSpd, spot->attrib,
								cat_cascade, cat_attrib, num_catatt,
								attribs, num_attrib, list_case, num_list,
								inmark, markscale, display_name,
								display_type, fit_to_map, fit_to_map_ref,
								rotation, rotate_lat, rotate_lon,
								rot_attrib, constrain_rot,
								xxo, yyo, xx, yy) )
							break;

						/* Attributes did not match              */
						/*  ... so remove this spot from the set */
						(void) remove_item_from_set(copyset, (ITEM) spot);
						}

					/* Destroy what is left of the copy */
					(void) destroy_set(copyset);
					break;

				/* Display values for link chain type fields */
				case FpaC_LCHAIN:

					/* >>>>> Display link chain based on type? <<<<< */

					/* Make a copy of the link chains */
					copyset = copy_set(lchains);

					/* Search for closest link chain within proximity */
					/*  that matches the required attributes          */
					while ( copyset->num > 0 )
						{

						/* Find the closest link chain */
						lchain = closest_lchain(copyset, Ppos[isite],
												NullFloat, fpos, &iseg);
						if ( IsNull(lchain) || IsNull(lchain->attrib) )
							{
							(void) remove_item_from_set(copyset, (ITEM) lchain);
							continue;
							}

						/* Check the proximity */
						fdist  = great_circle_distance(&BaseMap,
														Ppos[isite], fpos);
						fdist /= 1000.0;
						if ( fdist > proximity )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
										"   Feature beyond proximity for lat/lon ... %.1f %.1f\n",
										Plats[isite], Plons[isite]);
								}

							break;
							}

						/* Check if feature is on the map */
						if ( fpos[X] < 0.0 || fpos[Y] < 0.0
								|| fpos[X] > BaseMap.definition.xlen
								|| fpos[Y] > BaseMap.definition.ylen )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
										"   Feature outside map for lat/lon ... %.1f %.1f\n",
										Plats[isite], Plons[isite]);
								}
							}

						/* Determine bearing from sample to feature */
						fbear  = great_circle_bearing(&BaseMap,
														Ppos[isite], fpos);

						/* Interpolate the link chain (if required) */
						if ( lchain->dointerp ) (void) interpolate_lchain(lchain);

						/* Determine length of link chain track */
						flen = great_circle_line_length(&BaseMap, lchain->track);

						/* Determine direction and speed on link chain track */
						if ( lchain->inum > 1 )
							{
							(void) line_span_info(lchain->track, iseg,
														spos, epos, NullPoint);
							xdir  = great_circle_bearing(&BaseMap, spos, epos);
							xspd  = point_dist(spos, epos) /
														(float) lchain->minterp;
							xspd *= BaseMap.definition.units;
							xspd /= 60.0;
							}
						else
							{
							xdir = DefSegDir;
							xspd = DefSegSpd;
							}

						/* Display link chain attributes ... if they match! */
						if ( GRA_display_sampled_attributes(Pids[isite],
								vtime, time_zone, language,
								Plabs[isite], fpos, fdist, fbear, flen,
								xdir, xspd, lchain->attrib,
								cat_cascade, cat_attrib, num_catatt,
								attribs, num_attrib, list_case, num_list,
								inmark, markscale, display_name,
								display_type, fit_to_map, fit_to_map_ref,
								rotation, rotate_lat, rotate_lon,
								rot_attrib, constrain_rot,
								xxo, yyo, xx, yy) )
							break;

						/* Attributes did not match                    */
						/*  ... so remove this link chain from the set */
						(void) remove_item_from_set(copyset, (ITEM) lchain);
						}

					/* Destroy what is left of the copy */
					(void) destroy_set(copyset);
					break;

				/* All other type fields have already been handled */
				default:
					break;
				}
			}

		/* Free the sampling structures */
		switch ( fkind )
			{

			/* Free sampling structures for continuous type fields */
			case FpaC_CONTINUOUS:
				(void) free_vlist(vlist);
				FREEMEM(vlist);
				break;

			/* Free sampling structures for vector type fields */
			case FpaC_VECTOR:
				(void) free_vlist(mlist);
				(void) free_vlist(dlist);
				FREEMEM(mlist);
				FREEMEM(dlist);
				break;

			/* Free sampling structures for discrete type fields */
			case FpaC_DISCRETE:
				(void) destroy_set(areas);
				break;

			/* Free sampling structures for line type fields */
			case FpaC_LINE:
				(void) destroy_set(curves);
				break;

			/* Free sampling structures for scattered type fields */
			case FpaC_SCATTERED:
				(void) destroy_set(spots);
				break;

			/* Free sampling structures for link chain type fields */
			case FpaC_LCHAIN:
				(void) destroy_set(lchains);
				break;
			}

		/* Reset the current anchor */
		AnchorToMap = cur_anchor;

		/* Free space used by idents and labels */
		FREELIST(Pids,  Nump);
		FREELIST(Plabs, Nump);

		/* Return TRUE when all table sites have been sampled */
		return TRUE;
		}

	/* Sample fields for each sample grid site */
	else if ( !blank(grid_name) )
		{
		if ( Verbose )
			{
			if ( !blank(data_file) )
				(void) fprintf(stdout,
						"Grid sampling from data file %s\n", data_file);
			else if ( !blank(geo_file) )
				(void) fprintf(stdout,
						"Grid sampling for field ... %s %s from map file %s\n",
						element, level, geo_file);
			else if ( !blank(equation) )
				(void) fprintf(stdout,
						" Grid sampling for equation ... %s from %s at %s\n",
						equation, source, vtime);
			else
				(void) fprintf(stdout,
						" Grid sampling for field ... %s %s from %s at %s\n",
						element, level, source, vtime);
			}

		/* Find the named sample grid */
		cur_grid = get_sample_grid(grid_name);
		if ( IsNull(cur_grid) )
			{
			(void) sprintf(err_buf, "Sample grid ... %s ... not yet defined",
					grid_name);
			(void) error_report(err_buf);
			}

		/* Set sample locations from sample grid */
		Nump  = cur_grid->numx * cur_grid->numy;
		Ppos  = GETMEM(Ppos,  POINT, Nump);
		Plats = GETMEM(Plats, float, Nump);
		Plons = GETMEM(Plons, float, Nump);
		for ( isite=0, iiy=0; iiy<cur_grid->numy; iiy++ )
			for ( iix=0; iix<cur_grid->numx; iix++, isite++ )
				{

				/* Set latitude/longitude for sampling */
				flat = cur_grid->flats[iiy][iix];
				flon = cur_grid->flons[iiy][iix];

				/* Set map position for sampling */
				Plats[isite] = flat;
				Plons[isite] = flon;
				(void) ll_to_pos(&BaseMap, flat, flon, Ppos[isite]);
				}

		/* Sample all sample grid sites from data files */
		if ( !blank(data_file) )
			{
			status = TRUE;

			/* Create a set of spots from all sample grid locations */
			spots = create_set("spot");
			for ( isite=0; isite<Nump; isite++ )
				{

				/* Convert latitude and longitude to STRING format */
				(void) sprintf(mlat, "%f", Plats[isite]);
				(void) sprintf(mlon, "%f", Plons[isite]);

				/* Extract attributes from matching data file entry */
				if ( !match_data_file(data_file, data_file_format,
						data_file_units, data_file_wind_units,
						FpaCblank, mlat, mlon, vtime, &cal) )
					{
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot match lat/lon/vtime ... %f/%f/%s  in data file ... %s",
							Plats[isite], Plons[isite], vtime, data_file);
						}
					}

				/* Build a spot object from the data file attributes */
				spot = create_spot(Ppos[isite], FpaCblank, AttachNone, cal);

				/* Add the spot object to the set */
				(void) add_item_to_set(spots, (ITEM) spot);
				}
			}

		/* Sample fields at all sample grid sites from geography files */
		/*  depending on field type                                    */
		else if ( !blank(geo_file) )
			{
			status = TRUE;
			switch ( fkind )
				{

				/* Sample continuous type fields */
				case FpaC_CONTINUOUS:

					/* Create VLIST Object to hold sampled values */
					vlist = INITMEM(VLIST, 1);
					(void) init_vlist(vlist);

					/* Sample field at all sample grid sites */
					for ( isite=0; isite<Nump; isite++ )
						{

						/* Sample field at sample grid site */
						status = eval_sfc(gsfc, Ppos[isite], &dval);

						/* Error if field cannot be sampled */
						if ( !status )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
									"Cannot sample field ... %s %s from map file %s\n",
									element, level, geo_file);
								}
							(void) free_vlist(vlist);
							FREEMEM(vlist);
							break;
							}

						/* Add value to VLIST Object */
						(void) add_point_to_vlist(vlist, Ppos[isite],
																(float) dval);
						}
					break;

				/* Sample vector type fields */
				case FpaC_VECTOR:

					/* Create VLIST Objects to hold sampled values */
					mlist = INITMEM(VLIST, 1);
					(void) init_vlist(mlist);
					dlist = INITMEM(VLIST, 1);
					(void) init_vlist(dlist);

					/* Sample field at all sample grid sites */
					for ( isite=0; isite<Nump; isite++ )
						{

						/* Sample field at sample grid site */
						status = eval_sfc_MD(gsfc, Ppos[isite], &mval, &dval);

						/* Error if field cannot be sampled */
						if ( !status )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
									"Cannot sample field ... %s %s from map file %s\n",
									element, level, geo_file);
								}
							(void) free_vlist(mlist);
							(void) free_vlist(dlist);
							FREEMEM(mlist);
							FREEMEM(dlist);
							break;
							}

						/* Add values to VLIST Objects */
						(void) add_point_to_vlist(mlist, Ppos[isite],
																(float) mval);
						(void) add_point_to_vlist(dlist, Ppos[isite],
																(float) dval);
						}
					break;

				/* Sample discrete type fields */
				case FpaC_DISCRETE:

					/* Copy the discrete areas */
					areas = copy_set(gset);

					/* Error if field cannot be evaluated */
					if ( IsNull(areas) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from map file %s\n",
								element, level, geo_file);
							}
						status = FALSE;
						}
					break;

				/* Sample line type fields */
				case FpaC_LINE:

					/* Copy the line curves */
					curves = copy_set(gset);

					/* Error if field cannot be evaluated */
					if ( IsNull(curves) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from map file %s\n",
								element, level, geo_file);
							}
						status = FALSE;
						}
					break;

				/* Sample scattered type fields */
				case FpaC_SCATTERED:

					/* Copy the scattered spots */
					spots = copy_set(gset);

					/* Error if field cannot be evaluated */
					if ( IsNull(spots) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from map file %s\n",
								element, level, geo_file);
							}
						status = FALSE;
						}
					break;

				/* Sample link chain type fields */
				case FpaC_LCHAIN:

					/* Copy the link chains */
					lchains = copy_set(gset);

					/* Error if field cannot be evaluated */
					if ( IsNull(lchains) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from map file %s\n",
								element, level, geo_file);
							}
						status = FALSE;
						}
					break;

				/* Cannot sample other types of fields */
				default:
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot sample field ... %s %s from map file %s\n",
							element, level, geo_file);
						}
					status = FALSE;
					break;
				}
			}

		/* Sample fields at all sample grid sites from all other files */
		/*  depending on field type                                    */
		else
			{
			status = TRUE;
			switch ( fkind )
				{

				/* Sample continuous type fields */
				case FpaC_CONTINUOUS:

					/* Extract field values by equation */
					if ( !blank(equation) )
						vlist = retrieve_vlist_by_equation(&descript,
											Nump, Ppos, FpaCmksUnits, equation);

					/* Extract field values directly */
					else
						vlist = retrieve_vlist(&descript, Nump, Ppos);

					/* Error if field cannot be evaluated */
					if ( IsNull(vlist) )
						{
						if ( Verbose )
							{
							if ( !blank(equation) )
								(void) fprintf(stdout,
									"Cannot calculate equation ... %s from %s at %s\n",
									equation, source, vtime);
							else
								(void) fprintf(stdout,
									"Cannot extract field ... %s %s from %s at %s\n",
									element, level, source, vtime);
							}
						status = FALSE;
						}
					break;

				/* Sample vector type fields */
				case FpaC_VECTOR:

					/* Extract magnitude and direction of vector field */
					mlist = retrieve_vlist_component(&descript, M_Comp, Nump,
																		Ppos);
					dlist = retrieve_vlist_component(&descript, D_Comp, Nump,
																		Ppos);

					/* Error if field cannot be evaluated */
					if ( IsNull(mlist) || IsNull(dlist) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from %s at %s\n",
								element, level, source, vtime);
							}
						status = FALSE;
						}
					break;

				/* Sample discrete type fields */
				case FpaC_DISCRETE:

					/* Find the field containing the discrete areas */
					areas = retrieve_areaset(&descript);

					/* Error if field cannot be evaluated */
					if ( IsNull(areas) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from %s at %s\n",
								element, level, source, vtime);
							}
						status = FALSE;
						}
					break;

				/* Sample line type fields */
				case FpaC_LINE:

					/* Find the field containing the line curves */
					curves = retrieve_curveset(&descript);

					/* Error if field cannot be evaluated */
					if ( IsNull(curves) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from %s at %s\n",
								element, level, source, vtime);
							}
						status = FALSE;
						}
					break;

				/* Sample scattered type fields */
				case FpaC_SCATTERED:

					/* Find the field containing the scattered spots */
					spots = retrieve_spotset(&descript);

					/* Error if field cannot be evaluated */
					if ( IsNull(spots) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from %s at %s\n",
								element, level, source, vtime);
							}
						status = FALSE;
						}
					break;

				/* Sample link chain type fields */
				case FpaC_LCHAIN:

					/* Find the field containing the link chains */
					lchains = retrieve_lchainset(&descript);

					/* Error if field cannot be evaluated */
					if ( IsNull(lchains) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from %s at %s\n",
								element, level, source, vtime);
							}
						status = FALSE;
						}
					break;

				/* Cannot sample other types of fields */
				default:
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot sample field ... %s %s from %s at %s\n",
							element, level, source, vtime);
						}
					status = FALSE;
					break;
				}
			}

		/* Return now if field could not be sampled */
		if ( !status ) return TRUE;

		/* Display values for all sample grid sites */
		for ( isite=0, iiy=0; iiy<cur_grid->numy; iiy++ )
			for ( iix=0; iix<cur_grid->numx; iix++, isite++ )
				{

				/* Check for sample locations off the map */
				if ( Ppos[isite][X] < 0.0 || Ppos[isite][Y] < 0.0
						|| Ppos[isite][X] > BaseMap.definition.xlen
						|| Ppos[isite][Y] > BaseMap.definition.ylen )
					{
					if ( Verbose )
						{
						(void) fprintf(stdout,
								"  Sample lat/lon outside map at ... %.1f %.1f\n",
								Plats[isite], Plons[isite]);
						}

					/* Continue (if requested) for grid samples off map */
					if ( AnchorToMap ) continue;
					else if ( fit_to_map ) continue;
					}

				/* Comments for sample locations on the map */
				else
					{
					if ( Verbose )
						{
						(void) fprintf(stdout,
								"  Sample lat/lon at ... %.1f %.1f\n",
								Plats[isite], Plons[isite]);
						}
					}

				/* Set sample grid location on the current map */
				if ( AnchorToMap )
					{
					(void) anchored_location(Ppos[isite], 0.0, 0.0, &xxo, &yyo);
					}

				/* Set display location to current anchor position */
				else
					{
					xxs = (float) iix * cur_grid->x_shift;
					yys = (float) iiy * cur_grid->y_shift;
					(void) anchored_location(ZeroPoint, xxs, yys, &xxo, &yyo);
					}

				/* Set display location (offset by xdoff/ydoff) */
				/*  and offset location for sampled attributes  */
				xxo += xdoff;
				yyo += ydoff;
				xx = xxo + xoff;
				yy = yyo + yoff;

				/* Display values depending on field type */
				switch ( fkind )
					{

					/* Display values for continuous type fields */
					case FpaC_CONTINUOUS:

						/* Convert value to required units */
						(void) convert_value(FpaCmksUnits,
								(double) vlist->val[isite], units, &dval);

						/* Display the field value */
						(void) GRA_display_sampled_value(FpaCblank,
								vtime, time_zone, language,
								FpaCblank, Ppos[isite], (float) dval,
								attribs, num_attrib, list_case, num_list,
								inmark, markscale, display_name, display_type,
								fit_to_map, fit_to_map_ref,
								rotation, xxo, yyo, xx, yy);
						break;

					/* Display values for vector type fields */
					case FpaC_VECTOR:

						/* Convert magnitude to required units */
						(void) convert_value(FpaCmksUnits,
								(double) mlist->val[isite], units, &mval);

						/* Convert sampled direction to degrees true */
						dang = wind_dir_true(&BaseMap,
								Plats[isite], Plons[isite], dlist->val[isite]);

						/* >>>>> debug testing for vector fields in sample_field() <<<<< */
						if ( DebugMode )
							{
							(void) fprintf(stdout,
								"Vector field at %.1f %.1f ... m/d: %.1f %.1f  mval/dang: %.1f %.1f\n",
								Plats[isite], Plons[isite],
								mlist->val[isite], dlist->val[isite], mval, dang);
							}
						/* >>>>> debug testing for vector fields in sample_field() <<<<< */

						/* Display the vector */
						(void) GRA_display_sampled_vector(FpaCblank,
								vtime, time_zone, language,
								FpaCblank, Ppos[isite], (float) mval,
								dang, attribs, num_attrib, list_case, num_list,
								inmark, markscale, display_name, display_type,
								fit_to_map, fit_to_map_ref,
								rotation, xxo, yyo, xx, yy);
						break;

					/* Display values for discrete type fields */
					case FpaC_DISCRETE:

						/* Sample areas based on proximity */
						if ( proximity > 0.0 )
							{

							/* Make a copy of the area or subareas in the area */
							copyset = create_set("area");
							for ( iarea=0; iarea<areas->num; iarea++ )
								{
								area = (AREA) areas->list[iarea];

								/* Ensure that area is within proximity */
								(void) area_test_point(area, Ppos[isite], NULL,
														fpos, NULL, NULL, NULL,
														&inside);
								if ( !inside )
									{
									fdist  = great_circle_distance(&BaseMap,
															Ppos[isite], fpos);
									fdist /= 1000.0;
									if ( fdist > proximity ) continue;
									}

								/* Copy the area if no subareas */
								if ( area->numdiv == 0)
									{
									carea = copy_area(area, FALSE);
									if ( NotNull(carea) )
										{
										(void) add_item_to_set(copyset,
																(ITEM) carea);
										}
									}

								/* Create an area from each subarea */
								else
									{
									for ( isub=0; isub<=area->numdiv; isub++ )
										{
										subarea = area->subareas[isub];
										carea = area_from_subarea(subarea);
										if ( NotNull(carea) )
											{
											(void) add_item_to_set(copyset,
																(ITEM) carea);
											}
										}
									}
								}

							/* Search for closest area within proximity */
							/*  that matches the required attributes    */
							while ( copyset->num > 0 )
								{

								/* Find an enclosing area */
								area = enclosing_area(copyset, Ppos[isite],
												PickFirst, NullFloat, NullChar);

								/* Set sampling point and proximity (if inside) */
								if ( NotNull(area) )
									{
									(void) copy_point(fpos, Ppos[isite]);
									inside = TRUE;
									fdist  = 0.0;
									fbear  = 0.0;
									}

								/* Find the closest area within proximity */
								else
									{
									area   = closest_area(copyset, Ppos[isite],
												NULL, fpos, NULL, NULL, NULL);
									inside = FALSE;
									fdist  = great_circle_distance(&BaseMap,
															Ppos[isite], fpos);
									fdist /= 1000.0;
									if ( fdist > proximity )
										{
										if ( Verbose )
											{
											(void) fprintf(stdout,
													"   Feature beyond proximity for lat/lon ... %.1f %.1f\n",
													Plats[isite], Plons[isite]);
											}

										break;
										}

									/* Determine bearing from sample to feature */
									fbear  = great_circle_bearing(&BaseMap,
															Ppos[isite], fpos);
									}

								/* Ensure that feature is on the map */
								if ( fpos[X] < 0.0 || fpos[Y] < 0.0
										|| fpos[X] > BaseMap.definition.xlen
										|| fpos[Y] > BaseMap.definition.ylen )
									{
									if ( Verbose )
										{
										(void) fprintf(stdout,
												"   Feature outside map for lat/lon ... %.1f %.1f\n",
												Plats[isite], Plons[isite]);
										}

									(void) remove_item_from_set(copyset,
																(ITEM) area);
									continue;
									}

								/* Reset the sampling location (if not inside) */
								if ( !inside && AnchorToMap )
									{
									(void) anchored_location(fpos, 0.0, 0.0,
																	&xxo, &yyo);
									xxo += xdoff;
									yyo += ydoff;
									}

								/* Display area attributes ... if they match! */
								if ( GRA_display_sampled_attributes(FpaCblank,
										vtime, time_zone, language,
										FpaCblank, fpos, fdist, fbear,
										DefLineLen, DefSegDir, DefSegSpd,
										area->attrib,
										cat_cascade, cat_attrib, num_catatt,
										attribs, num_attrib, list_case, num_list,
										inmark, markscale, display_name,
										display_type, fit_to_map, fit_to_map_ref,
										rotation, rotate_lat, rotate_lon,
										rot_attrib, constrain_rot,
										xxo, yyo, xx, yy) )
									break;

								/* Attributes did not match              */
								/*  ... so remove this area from the set */
								(void) remove_item_from_set(copyset,
																(ITEM) area);
								}

							/* Destroy what is left of the copy */
							(void) destroy_set(copyset);
							break;
							}

						/* Sample areas directly */
						else
							{

							/* Find area enclosing point                        */
							/* Note that this should return a background value  */
							/*  with no subarea if no enclosing area is found   */
							/*  ... except map files, which have no background! */
							if ( !eval_areaset(areas, Ppos[isite], PickFirst,
									&subarea, &cal) )
								{
								if ( blank(geo_file) && Verbose )
									{
									(void) fprintf(stdout,
										"No areas or background for field ... %s %s from %s at %s\n",
										element, level, source, vtime);
									}
								continue;
								}

							/* Display attributes from attribute structure */
							(void) GRA_display_sampled_attributes(FpaCblank,
									vtime, time_zone, language,
									FpaCblank, Ppos[isite], DefProximity,
									DefBearing, DefLineLen, DefSegDir, DefSegSpd,
									cal, cat_cascade, cat_attrib, num_catatt,
									attribs, num_attrib, list_case, num_list,
									inmark, markscale, display_name,
									display_type, fit_to_map, fit_to_map_ref,
									rotation, rotate_lat, rotate_lon,
									rot_attrib, constrain_rot,
									xxo, yyo, xx, yy);
							}
						break;

					/* Display values for line type fields */
					case FpaC_LINE:

						/* Make a copy of the line curves */
						copyset = copy_set(curves);

						/* Search for closest curve within proximity */
						/*  that matches the required attributes     */
						while ( copyset->num > 0 )
							{

							/* Find the closest curve */
							curve = closest_curve(copyset, Ppos[isite],
													NullFloat, fpos, &iseg);
							if ( IsNull(curve) || IsNull(curve->attrib) )
								{
								(void) remove_item_from_set(copyset,
																(ITEM) curve);
								continue;
								}

							/* Check the proximity */
							fdist  = great_circle_distance(&BaseMap,
															Ppos[isite], fpos);
							fdist /= 1000.0;
							if ( fdist > proximity )
								{
								if ( Verbose )
									{
									(void) fprintf(stdout,
											"   Feature beyond proximity for lat/lon ... %.1f %.1f\n",
											Plats[isite], Plons[isite]);
									}

								break;
								}

							/* Check if feature is on the map */
							if ( fpos[X] < 0.0 || fpos[Y] < 0.0
									|| fpos[X] > BaseMap.definition.xlen
									|| fpos[Y] > BaseMap.definition.ylen )
								{
								if ( Verbose )
									{
									(void) fprintf(stdout,
											"   Feature outside map for lat/lon ... %.1f %.1f\n",
											Plats[isite], Plons[isite]);
									}

								if ( AnchorToMap || fit_to_map )
									{
									(void) remove_item_from_set(copyset,
																(ITEM) curve);
									continue;
									}
								}

							/* Determine bearing from sample to feature */
							fbear  = great_circle_bearing(&BaseMap,
															Ppos[isite], fpos);

							/* Change sign of proximity wrt curve sense */
							(void) recall_curve_sense(curve, &sense);
							(void) curve_test_point(curve, Ppos[isite],
									NullFloat, NullPoint, NullInt, NullLogical,
									&right);
							if ( (sense == Right && right)
									|| (sense == Left && !right) )
								fdist = -fdist;

							/* Determine length of curve */
							flen = great_circle_line_length(&BaseMap,
															curve->line);

							/* Reset the sampling location */
							if ( AnchorToMap )
								{
								(void) anchored_location(fpos, 0.0, 0.0,
																&xxo, &yyo);
								xxo += xdoff;
								yyo += ydoff;

								/* Reset the display location (if required) */
								if ( display_at_feature )
									{
									xx = xxo + xoff;
									yy = yyo + yoff;
									}
								}

							/* Determine direction of curve */
							(void) line_span_info(curve->line, iseg,
														spos, epos, NullPoint);
							xdir = great_circle_bearing(&BaseMap, spos, epos);

							/* Display curve attributes ... if they match! */
							if ( GRA_display_sampled_attributes(FpaCblank,
									vtime, time_zone, language,
									FpaCblank, fpos, fdist, fbear, flen,
									xdir, DefSegSpd, curve->attrib,
									cat_cascade, cat_attrib, num_catatt,
									attribs, num_attrib, list_case, num_list,
									inmark, markscale, display_name,
									display_type, fit_to_map, fit_to_map_ref,
									rotation, rotate_lat, rotate_lon,
									rot_attrib, constrain_rot,
									xxo, yyo, xx, yy) )
								break;

							/* Attributes did not match               */
							/*  ... so remove this curve from the set */
							(void) remove_item_from_set(copyset, (ITEM) curve);
							}

						/* Destroy what is left of the copy */
						(void) destroy_set(copyset);
						break;

					/* Display values for scattered type fields */
					case FpaC_SCATTERED:

						/* Extract the spot from the data file set */
						if ( !blank(data_file) )
							{

							/* Get the spot */
							spot = spots->list[isite];
							if ( IsNull(spot) || IsNull(spot->attrib) )
								continue;

							/* Display attributes from spot attribute structure */
							(void) GRA_display_sampled_attributes(FpaCblank,
									vtime, time_zone, language,
									FpaCblank, spot->anchor, DefProximity,
									DefBearing, DefLineLen, DefSegDir, DefSegSpd,
									spot->attrib,
									cat_cascade, cat_attrib, num_catatt,
									attribs, num_attrib, list_case, num_list,
									inmark, markscale, display_name,
									display_type, fit_to_map, fit_to_map_ref,
									rotation, rotate_lat, rotate_lon,
									rot_attrib, constrain_rot,
									xxo, yyo, xx, yy);
							}

						/* Extract the spot from the set */
						else
							{
							/* Make a copy of the scattered spots */
							copyset = copy_set(spots);

							/* Search for closest spot within proximity */
							/*  that matches the required attributes    */
							while ( copyset->num > 0 )
								{

								/* Find the closest spot */
								spot = closest_spot(copyset, Ppos[isite], NULL,
														NULL, NullFloat, fpos);
								if ( IsNull(spot) || IsNull(spot->attrib) )
									{
									(void) remove_item_from_set(copyset,
																(ITEM) spot);
									continue;
									}

								/* Check the proximity */
								fdist  = great_circle_distance(&BaseMap,
															Ppos[isite], fpos);
								fdist /= 1000.0;
								if ( fdist > proximity )
									{
									if ( Verbose )
										{
										(void) fprintf(stdout,
												"   Feature beyond proximity for lat/lon ... %.1f %.1f\n",
												Plats[isite], Plons[isite]);
										}

									break;
									}

								/* Determine bearing from sample to feature */
								fbear  = great_circle_bearing(&BaseMap,
															Ppos[isite], fpos);

								/* Check if feature is on the map */
								if ( fpos[X] < 0.0 || fpos[Y] < 0.0
										|| fpos[X] > BaseMap.definition.xlen
										|| fpos[Y] > BaseMap.definition.ylen )
									{
									if ( Verbose )
										{
										(void) fprintf(stdout,
												"   Feature outside map for lat/lon ... %.1f %.1f\n",
												Plats[isite], Plons[isite]);
										}

									if ( AnchorToMap || fit_to_map )
										{
										(void) remove_item_from_set(copyset,
																(ITEM) spot);
										continue;
										}
									}

								/* Reset the sampling location */
								if ( AnchorToMap )
									{
									(void) anchored_location(fpos, 0.0, 0.0,
																	&xxo, &yyo);
									xxo += xdoff;
									yyo += ydoff;

									/* Reset the display location (if required) */
									if ( display_at_feature )
										{
										xx = xxo + xoff;
										yy = yyo + yoff;
										}
									}

								/* Display spot attributes ... if they match! */
								if ( GRA_display_sampled_attributes(FpaCblank,
										vtime, time_zone, language,
										FpaCblank, fpos, fdist, fbear,
										DefLineLen, DefSegDir, DefSegSpd,
										spot->attrib,
										cat_cascade, cat_attrib, num_catatt,
										attribs, num_attrib, list_case, num_list,
										inmark, markscale, display_name,
										display_type, fit_to_map, fit_to_map_ref,
										rotation, rotate_lat, rotate_lon,
										rot_attrib, constrain_rot,
										xxo, yyo, xx, yy) )
									break;

								/* Attributes did not match              */
								/*  ... so remove this spot from the set */
								(void) remove_item_from_set(copyset,
																(ITEM) spot);
								}

							/* Destroy what is left of the copy */
							(void) destroy_set(copyset);
							}

						break;

					/* Display values for link chain type fields */
					case FpaC_LCHAIN:

						/* >>>>> Display link chain based on type? <<<<< */

						/* Make a copy of the link chains */
						copyset = copy_set(lchains);

						/* Search for closest link chain within proximity */
						/*  that matches the required attributes          */
						while ( copyset->num > 0 )
							{

							/* Find the closest link chain */
							lchain = closest_lchain(copyset, Ppos[isite],
													NullFloat, fpos, &iseg);
							if ( IsNull(lchain) || IsNull(lchain->attrib) )
								{
								(void) remove_item_from_set(copyset,
																(ITEM) lchain);
								continue;
								}

							/* Check the proximity */
							fdist  = great_circle_distance(&BaseMap,
															Ppos[isite], fpos);
							fdist /= 1000.0;
							if ( fdist > proximity )
								{
								if ( Verbose )
									{
									(void) fprintf(stdout,
											"   Feature beyond proximity for lat/lon ... %.1f %.1f\n",
											Plats[isite], Plons[isite]);
									}

								break;
								}

							/* Check if feature is on the map */
							if ( fpos[X] < 0.0 || fpos[Y] < 0.0
									|| fpos[X] > BaseMap.definition.xlen
									|| fpos[Y] > BaseMap.definition.ylen )
								{
								if ( Verbose )
									{
									(void) fprintf(stdout,
											"   Feature outside map for lat/lon ... %.1f %.1f\n",
											Plats[isite], Plons[isite]);
									}

								if ( AnchorToMap || fit_to_map )
									{
									(void) remove_item_from_set(copyset,
																(ITEM) lchain);
									continue;
									}
								}

							/* Determine bearing from sample to feature */
							fbear  = great_circle_bearing(&BaseMap,
															Ppos[isite], fpos);

							/* Interpolate the link chain (if required) */
							if ( lchain->dointerp )
								(void) interpolate_lchain(lchain);

							/* Determine length of link chain track */
							flen = great_circle_line_length(&BaseMap,
																lchain->track);

							/* Reset the sampling location */
							if ( AnchorToMap )
								{
								(void) anchored_location(fpos, 0.0, 0.0,
																&xxo, &yyo);
								xxo += xdoff;
								yyo += ydoff;

								/* Reset the display location (if required) */
								if ( display_at_feature )
									{
									xx = xxo + xoff;
									yy = yyo + yoff;
									}
								}

							/* Determine direction and speed on link chain track */
							if ( lchain->inum > 1 )
								{
								(void) line_span_info(lchain->track, iseg,
														spos, epos, NullPoint);
								xdir  = great_circle_bearing(&BaseMap,
																	spos, epos);
								xspd  = point_dist(spos, epos) /
														(float) lchain->minterp;
								xspd *= BaseMap.definition.units;
								xspd /= 60.0;
								}
							else
								{
								xdir = DefSegDir;
								xspd = DefSegSpd;
								}
	
							/* Display link chain attributes ... if they match! */
							if ( GRA_display_sampled_attributes(FpaCblank,
									vtime, time_zone, language,
									FpaCblank, fpos, fdist, fbear, flen,
									xdir, xspd, lchain->attrib,
									cat_cascade, cat_attrib, num_catatt,
									attribs, num_attrib, list_case, num_list,
									inmark, markscale, display_name,
									display_type, fit_to_map, fit_to_map_ref,
									rotation, rotate_lat, rotate_lon,
									rot_attrib, constrain_rot,
									xxo, yyo, xx, yy) )
								break;

							/* Attributes did not match                    */
							/*  ... so remove this link chain from the set */
							(void) remove_item_from_set(copyset, (ITEM) lchain);
							}

						/* Destroy what is left of the copy */
						(void) destroy_set(copyset);
						break;

					/* All other type fields have already been handled */
					default:
						break;
					}
				}

		/* Free the sampling structures */
		switch ( fkind )
			{

			/* Free sampling structures for continuous type fields */
			case FpaC_CONTINUOUS:
				(void) free_vlist(vlist);
				FREEMEM(vlist);
				break;

			/* Free sampling structures for vector type fields */
			case FpaC_VECTOR:
				(void) free_vlist(mlist);
				(void) free_vlist(dlist);
				FREEMEM(mlist);
				FREEMEM(dlist);
				break;

			/* Free sampling structures for discrete type fields */
			case FpaC_DISCRETE:
				(void) destroy_set(areas);
				break;

			/* Free sampling structures for line type fields */
			case FpaC_LINE:
				(void) destroy_set(curves);
				break;

			/* Free sampling structures for scattered type fields */
			case FpaC_SCATTERED:
				(void) destroy_set(spots);
				break;

			/* Free sampling structures for link chain type fields */
			case FpaC_LCHAIN:
				(void) destroy_set(lchains);
				break;
			}

		/* Return TRUE when all grid locations have been sampled */
		return TRUE;
		}

	/* Sample fields for each sample list site */
	else if ( !blank(list_name) )
		{
		if ( Verbose )
			{
			if ( !blank(data_file) )
				(void) fprintf(stdout,
						"List sampling from data file %s\n", data_file);
			else if ( !blank(geo_file) )
				(void) fprintf(stdout,
						"List sampling for field ... %s %s from map file %s\n",
						element, level, geo_file);
			else if ( !blank(equation) )
				(void) fprintf(stdout,
						" List sampling for equation ... %s from %s at %s\n",
						equation, source, vtime);
			else
				(void) fprintf(stdout,
						" List sampling for field ... %s %s from %s at %s\n",
						element, level, source, vtime);
			}

		/* Find the named sample list */
		cur_list = get_sample_list(list_name);
		if ( IsNull(cur_list) )
			{
			(void) sprintf(err_buf, "Sample list ... %s ... not yet defined",
					list_name);
			(void) error_report(err_buf);
			}

		/* Set sample locations from sample list */
		for ( Nump=0, isite=0; isite<cur_list->num; isite++ )
			{

			/* Set latitude/longitude for sampling */
			if ( cur_list->usell[isite] )
				{
				flat   = cur_list->flats[isite];
				flon   = cur_list->flons[isite];
				loclab = FpaCblank;
				}

			/* Get latitude/longitude for sampling from all locations */
			/*  in a location look up table                           */
			else if ( same_ic(cur_list->idents[isite], LocIdentAll)
						|| same_ic(cur_list->idents[isite], LocIdentAllVtime) )
				{
				if ( blank(loc_lookup) )
					(void) error_report("No location look up table");
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"   Sampling all locations from look up ... %s\n",
							loc_lookup);
					}

				/* Set valid time to check */
				if ( same_ic(cur_list->idents[isite], LocIdentAllVtime) )
					vt = vtime;
				else
					vt = FpaCblank;

				/* Set sample locations for all look up table locations */
				iloc = -1;
				while ( (iloc = next_location_lookup_line(loc_lookup, iloc, vt,
									&locid, &loclat, &loclon, &loclab)) >= 0 )
					{

					/* Set latitude/longitude for sampling */
					flat = read_lat(loclat, &status);
					if ( !status )
						(void) error_report("Problem with location look up lat");
					flon = read_lon(loclon, &status);
					if ( !status )
						(void) error_report("Problem with location look up lon");

					/* Allocate space for this sampling location */
					Nump++;
					Ppos  = GETMEM(Ppos,  POINT,  Nump);
					Plats = GETMEM(Plats, float,  Nump);
					Plons = GETMEM(Plons, float,  Nump);
					Pids  = GETMEM(Pids,  STRING, Nump);
					Plabs = GETMEM(Plabs, STRING, Nump);

					/* Set map position for sampling */
					Plats[Nump-1] = flat;
					Plons[Nump-1] = flon;
					(void) ll_to_pos(&BaseMap, flat, flon, Ppos[Nump-1]);
					Pids[Nump-1]  = safe_strdup(locid);
					Plabs[Nump-1] = safe_strdup(loclab);
					}

				/* Go on to next sample location in sample list */
				continue;
				}

			/* Get latitude/longitude for sampling from location look up */
			else
				{
				if ( blank(loc_lookup) )
					(void) error_report("No location look up table");
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"   Matching location ident ... \"%s\"",
							cur_list->idents[isite]);
					(void) fprintf(stdout, "  from look up ... %s\n",
							loc_lookup);
					}
				if ( !match_location_lookup(loc_lookup,
						cur_list->idents[isite], vtime, &loclat, &loclon,
						&loclab) )
					{
					(void) sprintf(err_buf,
							"Error matching \"%s\" in look up ... %s",
							cur_list->idents[isite], loc_lookup);
					(void) error_report(err_buf);
					}
				flat = read_lat(loclat, &status);
				if ( !status )
					(void) error_report("Problem with location look up lat");
				flon = read_lon(loclon, &status);
				if ( !status )
					(void) error_report("Problem with location look up lon");
				}

			/* Allocate space for this sampling location */
			Nump++;
			Ppos  = GETMEM(Ppos,  POINT,  Nump);
			Plats = GETMEM(Plats, float,  Nump);
			Plons = GETMEM(Plons, float,  Nump);
			Pids  = GETMEM(Pids,  STRING, Nump);
			Plabs = GETMEM(Plabs, STRING, Nump);

			/* Set map position for sampling */
			Plats[Nump-1] = flat;
			Plons[Nump-1] = flon;
			(void) ll_to_pos(&BaseMap, flat, flon, Ppos[Nump-1]);
			Pids[Nump-1]  = safe_strdup(cur_list->idents[isite]);
			Plabs[Nump-1] = safe_strdup(loclab);
			}

		/* Sample at all sample list sites from data files */
		if ( !blank(data_file) )
			{
			status = TRUE;

			/* Create a set of spots from all lookup table locations */
			spots = create_set("spot");
			for ( iloc=0; iloc<Nump; iloc++ )
				{

				/* Convert latitude and longitude to STRING format */
				(void) sprintf(mlat, "%f", Plats[iloc]);
				(void) sprintf(mlon, "%f", Plons[iloc]);

				/* Extract attributes from matching data file entry */
				if ( !match_data_file(data_file, data_file_format,
						data_file_units, data_file_wind_units,
						Pids[iloc], mlat, mlon, vtime, &cal) )
					{
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot match ident/lat/lon/vtime ... %s/%f/%f/%s  in data file ... %s",
							Pids[iloc], Plats[iloc], Plons[iloc], vtime,
							data_file);
						}
					}

				/* Build a spot object from the data file attributes */
				spot = create_spot(Ppos[iloc], FpaCblank, AttachNone, cal);

				/* Add the spot object to the set */
				(void) add_item_to_set(spots, (ITEM) spot);
				}
			}

		/* Sample fields at all sample list sites from geography files */
		/*  depending on field type                                    */
		else if ( !blank(geo_file) )
			{
			status = TRUE;
			switch ( fkind )
				{

				/* Sample continuous type fields */
				case FpaC_CONTINUOUS:

					/* Create VLIST Object to hold sampled values */
					vlist = INITMEM(VLIST, 1);
					(void) init_vlist(vlist);

					/* Sample field at all sample list sites */
					for ( iloc=0; iloc<Nump; iloc++ )
						{

						/* Sample field at sample list site */
						status = eval_sfc(gsfc, Ppos[iloc], &dval);

						/* Error if field cannot be sampled */
						if ( !status )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
									"Cannot sample field ... %s %s from map file %s\n",
									element, level, geo_file);
								}
							(void) free_vlist(vlist);
							FREEMEM(vlist);
							break;
							}

						/* Add value to VLIST Object */
						(void) add_point_to_vlist(vlist, Ppos[iloc],
																(float) dval);
						}
					break;

				/* Sample vector type fields */
				case FpaC_VECTOR:

					/* Create VLIST Objects to hold sampled values */
					mlist = INITMEM(VLIST, 1);
					(void) init_vlist(mlist);
					dlist = INITMEM(VLIST, 1);
					(void) init_vlist(dlist);

					/* Sample field at all sample list sites */
					for ( iloc=0; iloc<Nump; iloc++ )
						{

						/* Sample field at sample list site */
						status = eval_sfc_MD(gsfc, Ppos[iloc], &mval, &dval);

						/* Error if field cannot be sampled */
						if ( !status )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
									"Cannot sample field ... %s %s from map file %s\n",
									element, level, geo_file);
								}
							(void) free_vlist(mlist);
							(void) free_vlist(dlist);
							FREEMEM(mlist);
							FREEMEM(dlist);
							break;
							}

						/* Add values to VLIST Objects */
						(void) add_point_to_vlist(mlist, Ppos[iloc],
																(float) mval);
						(void) add_point_to_vlist(dlist, Ppos[iloc],
																(float) dval);
						}
					break;

				/* Sample discrete type fields */
				case FpaC_DISCRETE:

					/* Copy the discrete areas */
					areas = copy_set(gset);

					/* Error if field cannot be evaluated */
					if ( IsNull(areas) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from map file %s\n",
								element, level, geo_file);
							}
						status = FALSE;
						}
					break;

				/* Sample line type fields */
				case FpaC_LINE:

					/* Copy the line curves */
					curves = copy_set(gset);

					/* Error if field cannot be evaluated */
					if ( IsNull(curves) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from map file %s\n",
								element, level, geo_file);
							}
						status = FALSE;
						}
					break;

				/* Sample scattered type fields */
				case FpaC_SCATTERED:

					/* Copy the scattered spots */
					spots = copy_set(gset);

					/* Error if field cannot be evaluated */
					if ( IsNull(spots) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from map file %s\n",
								element, level, geo_file);
							}
						status = FALSE;
						}
					break;

				/* Sample link chain type fields */
				case FpaC_LCHAIN:

					/* Copy the link chains */
					lchains = copy_set(gset);

					/* Error if field cannot be evaluated */
					if ( IsNull(lchains) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from map file %s\n",
								element, level, geo_file);
							}
						status = FALSE;
						}
					break;

				/* Cannot sample other types of fields */
				default:
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot sample field ... %s %s from map file %s\n",
							element, level, geo_file);
						}
					status = FALSE;
					break;
				}
			}

		/* Sample fields at all sample list sites from all other files */
		/*  depending on field type                                    */
		else
			{
			status = TRUE;
			switch ( fkind )
				{

				/* Sample continuous type fields */
				case FpaC_CONTINUOUS:

					/* Extract field values by equation */
					if ( !blank(equation) )
						vlist = retrieve_vlist_by_equation(&descript,
											Nump, Ppos, FpaCmksUnits, equation);

					/* Extract field values directly */
					else
						vlist = retrieve_vlist(&descript, Nump, Ppos);

					/* Error if field cannot be evaluated */
					if ( IsNull(vlist) )
						{
						if ( Verbose )
							{
							if ( !blank(equation) )
								(void) fprintf(stdout,
									"Cannot calculate equation ... %s from %s at %s\n",
									equation, source, vtime);
							else
								(void) fprintf(stdout,
									"Cannot extract field ... %s %s from %s at %s\n",
									element, level, source, vtime);
							}
						status = FALSE;
						}
					break;

				/* Sample vector type fields */
				case FpaC_VECTOR:

					/* Extract magnitude and direction of vector field */
					mlist = retrieve_vlist_component(&descript, M_Comp, Nump,
																		Ppos);
					dlist = retrieve_vlist_component(&descript, D_Comp, Nump,
																		Ppos);

					/* Error if field cannot be evaluated */
					if ( IsNull(mlist) || IsNull(dlist) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from %s at %s\n",
								element, level, source, vtime);
							}
						status = FALSE;
						}
					break;

				/* Sample discrete type fields */
				case FpaC_DISCRETE:

					/* Find the field containing the discrete areas */
					areas = retrieve_areaset(&descript);

					/* Error if field cannot be evaluated */
					if ( IsNull(areas) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from %s at %s\n",
								element, level, source, vtime);
							}
						status = FALSE;
						}
					break;

				/* Sample line type fields */
				case FpaC_LINE:

					/* Find the field containing the line curves */
					curves = retrieve_curveset(&descript);

					/* Error if field cannot be evaluated */
					if ( IsNull(curves) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from %s at %s\n",
								element, level, source, vtime);
							}
						status = FALSE;
						}
					break;

				/* Sample scattered type fields */
				case FpaC_SCATTERED:

					/* Find the field containing the scattered spots */
					spots = retrieve_spotset(&descript);

					/* Error if field cannot be evaluated */
					if ( IsNull(spots) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from %s at %s\n",
								element, level, source, vtime);
							}
						status = FALSE;
						}
					break;

				/* Sample link chain type fields */
				case FpaC_LCHAIN:

					/* Find the field containing the link chains */
					lchains = retrieve_lchainset(&descript);

					/* Error if field cannot be evaluated */
					if ( IsNull(lchains) )
						{
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot extract field ... %s %s from %s at %s\n",
								element, level, source, vtime);
							}
						status = FALSE;
						}
					break;

				/* Cannot sample other types of fields */
				default:
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot sample field ... %s %s from %s at %s\n",
							element, level, source, vtime);
						}
					status = FALSE;
					break;
				}
			}

		/* Return now if field could not be sampled */
		if ( !status )
			{

			/* Free space used by idents and labels */
			FREELIST(Pids,  Nump);
			FREELIST(Plabs, Nump);

			return TRUE;
			}

		/* Initialize offsets for sample list sites */
		iil = 0;
		xxs = 0.0;
		yys = 0.0;

		/* Display values for all sample list sites */
		for ( iloc=0; iloc<Nump; iloc++ )
			{

			/* Check for sample locations off the map */
			if ( Ppos[iloc][X] < 0.0 || Ppos[iloc][Y] < 0.0
					|| Ppos[iloc][X] > BaseMap.definition.xlen
					|| Ppos[iloc][Y] > BaseMap.definition.ylen )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Sample lat/lon outside map at ... %.1f %.1f\n",
							Plats[iloc], Plons[iloc]);
					}

				/* Continue (if requested) for list samples off map */
				if ( AnchorToMap ) continue;
				else if ( fit_to_map ) continue;
				}

			/* Comments for sample locations on the map */
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Sample lat/lon at ... %.1f %.1f\n",
							Plats[iloc], Plons[iloc]);
					}
				}

			/* Set sample list location on the current map */
			if ( AnchorToMap )
				{
				(void) anchored_location(Ppos[iloc], 0.0, 0.0, &xxo, &yyo);
				}

			/* Set display location to current anchor position  */
			/*  and set offset for each successive site in list */
			else
				{
				(void) anchored_location(ZeroPoint, xxs, yys, &xxo, &yyo);
				iil++;
				if ( cur_list->x_wrap > 1 && iil % cur_list->x_wrap == 0 )
					{
					xxs  = 0.0;
					yys += cur_list->y_shift;
					}
				else if ( cur_list->x_wrap > 1 )
					{
					xxs += cur_list->x_shift;
					}
				else if ( cur_list->y_wrap > 1 && iil % cur_list->y_wrap == 0 )
					{
					xxs += cur_list->x_shift;
					yys  = 0.0;
					}
				else if ( cur_list->y_wrap > 1 )
					{
					yys += cur_list->y_shift;
					}
				else
					{
					xxs += cur_list->x_shift;
					yys += cur_list->y_shift;
					}
				}

			/* Set display location (offset by xdoff/ydoff) */
			/*  and offset location for sampled attributes  */
			xxo += xdoff;
			yyo += ydoff;
			xx = xxo + xoff;
			yy = yyo + yoff;

			/* Display values depending on field type */
			switch ( fkind )
				{

				/* Display values for continuous type fields */
				case FpaC_CONTINUOUS:

					/* Convert value to required units */
					(void) convert_value(FpaCmksUnits,
							(double) vlist->val[iloc], units, &dval);

					/* Display the field value */
					(void) GRA_display_sampled_value(Pids[iloc],
							vtime, time_zone, language,
							Plabs[iloc], Ppos[iloc], (float) dval,
							attribs, num_attrib, list_case, num_list,
							inmark, markscale, display_name, display_type,
							fit_to_map, fit_to_map_ref,
							rotation, xxo, yyo, xx, yy);
					break;

				/* Display values for vector type fields */
				case FpaC_VECTOR:

					/* Convert magnitude to required units */
					(void) convert_value(FpaCmksUnits,
							(double) mlist->val[iloc], units, &mval);

					/* Convert sampled direction to degrees true */
					dang = wind_dir_true(&BaseMap,
							Plats[iloc], Plons[iloc], dlist->val[iloc]);

					/* >>>>> debug testing for vector fields in sample_field() <<<<< */
					if ( DebugMode )
						{
						(void) fprintf(stdout,
							"Vector field at %.1f %.1f ... m/d: %.1f %.1f  mval/dang: %.1f %.1f\n",
							Plats[iloc], Plons[iloc], mlist->val[iloc], dlist->val[iloc],
							mval, dang);
						}
					/* >>>>> debug testing for vector fields in sample_field() <<<<< */

					/* Display the vector */
					(void) GRA_display_sampled_vector(Pids[iloc],
							vtime, time_zone, language,
							Plabs[iloc], Ppos[iloc], (float) mval,
							dang, attribs, num_attrib, list_case, num_list,
							inmark, markscale, display_name, display_type,
							fit_to_map, fit_to_map_ref,
							rotation, xxo, yyo, xx, yy);
					break;

				/* Display values for discrete type fields */
				case FpaC_DISCRETE:

					/* Sample areas based on proximity */
					if ( proximity > 0.0 )
						{

						/* Make a copy of the area or subareas in the area */
						copyset = create_set("area");
						for ( iarea=0; iarea<areas->num; iarea++ )
							{
							area = (AREA) areas->list[iarea];

							/* Ensure that area is within proximity */
							(void) area_test_point(area, Ppos[iloc], NULL,
													fpos, NULL, NULL, NULL,
													&inside);
							if ( !inside )
								{
								fdist  = great_circle_distance(&BaseMap,
															Ppos[iloc], fpos);
								fdist /= 1000.0;
								if ( fdist > proximity ) continue;
								}

							/* Copy the area if no subareas */
							if ( area->numdiv == 0)
								{
								carea = copy_area(area, FALSE);
								if ( NotNull(carea) )
									{
									(void) add_item_to_set(copyset,
															(ITEM) carea);
									}
								}

							/* Create an area from each subarea */
							else
								{
								for ( isub=0; isub<=area->numdiv; isub++ )
									{
									subarea = area->subareas[isub];
									carea = area_from_subarea(subarea);
									if ( NotNull(carea) )
										{
										(void) add_item_to_set(copyset,
																(ITEM) carea);
										}
									}
								}
							}

						/* Search for closest area within proximity */
						/*  that matches the required attributes    */
						while ( copyset->num > 0 )
							{

							/* Find an enclosing area */
							area = enclosing_area(copyset, Ppos[iloc],
												PickFirst, NullFloat, NullChar);

							/* Set sampling point and proximity (if inside) */
							if ( NotNull(area) )
								{
								(void) copy_point(fpos, Ppos[iloc]);
								inside = TRUE;
								fdist  = 0.0;
								fbear  = 0.0;
								}

							/* Find the closest area within proximity */
							else
								{
								area   = closest_area(copyset, Ppos[iloc],
												NULL, fpos, NULL, NULL, NULL);
								inside = FALSE;
								fdist  = great_circle_distance(&BaseMap,
															Ppos[iloc], fpos);
								fdist /= 1000.0;
								if ( fdist > proximity )
									{
									if ( Verbose )
										{
										(void) fprintf(stdout,
												"   Feature beyond proximity for lat/lon ... %.1f %.1f\n",
												Plats[iloc], Plons[iloc]);
										}

									break;
									}

								/* Determine bearing from sample to feature */
								fbear  = great_circle_bearing(&BaseMap,
															Ppos[iloc], fpos);
								}

							/* Ensure that feature is on the map */
							if ( fpos[X] < 0.0 || fpos[Y] < 0.0
									|| fpos[X] > BaseMap.definition.xlen
									|| fpos[Y] > BaseMap.definition.ylen )
								{
								if ( Verbose )
									{
									(void) fprintf(stdout,
											"   Feature outside map for lat/lon ... %.1f %.1f\n",
											Plats[iloc], Plons[iloc]);
									}

								(void) remove_item_from_set(copyset,
																(ITEM) area);
								continue;
								}

							/* Reset the sampling location (if not inside) */
							if ( !inside && AnchorToMap )
								{
								(void) anchored_location(fpos, 0.0, 0.0,
																&xxo, &yyo);
								xxo += xdoff;
								yyo += ydoff;
								}

							/* Display area attributes ... if they match! */
							if ( GRA_display_sampled_attributes(Pids[iloc],
									vtime, time_zone, language,
									Plabs[iloc], fpos, fdist, fbear,
									DefLineLen, DefSegDir, DefSegSpd,
									area->attrib,
									cat_cascade, cat_attrib, num_catatt,
									attribs, num_attrib, list_case, num_list,
									inmark, markscale, display_name,
									display_type, fit_to_map, fit_to_map_ref,
									rotation, rotate_lat, rotate_lon,
									rot_attrib, constrain_rot,
									xxo, yyo, xx, yy) )
								break;

							/* Attributes did not match              */
							/*  ... so remove this area from the set */
							(void) remove_item_from_set(copyset, (ITEM) area);
							}

						/* Destroy what is left of the copy */
						(void) destroy_set(copyset);
						break;
						}

					/* Sample areas directly */
					else
						{

						/* Find area enclosing point                        */
						/* Note that this should return a background value  */
						/*  with no subarea if no enclosing area is found   */
						/*  ... except map files, which have no background! */
						if ( !eval_areaset(areas, Ppos[iloc], PickFirst,
								&subarea, &cal) )
							{
							if ( blank(geo_file) && Verbose )
								{
								(void) fprintf(stdout,
									"No areas or background for field ... %s %s from %s at %s\n",
									element, level, source, vtime);
								}
							continue;
							}

						/* Display attributes from attribute structure */
						(void) GRA_display_sampled_attributes(Pids[iloc],
								vtime, time_zone, language,
								Plabs[iloc], Ppos[iloc], DefProximity,
								DefBearing, DefLineLen, DefSegDir, DefSegSpd,
								cal, cat_cascade, cat_attrib, num_catatt,
								attribs, num_attrib, list_case, num_list,
								inmark, markscale, display_name,
								display_type, fit_to_map, fit_to_map_ref,
								rotation, rotate_lat, rotate_lon,
								rot_attrib, constrain_rot,
								xxo, yyo, xx, yy);
						}
					break;

				/* Display values for line type fields */
				case FpaC_LINE:

					/* Make a copy of the line curves */
					copyset = copy_set(curves);

					/* Search for closest curve within proximity */
					/*  that matches the required attributes     */
					while ( copyset->num > 0 )
						{

						/* Find the closest curve */
						curve = closest_curve(copyset, Ppos[iloc],
												NullFloat, fpos, &iseg);
						if ( IsNull(curve) || IsNull(curve->attrib) )
							{
							(void) remove_item_from_set(copyset, (ITEM) curve);
							continue;
							}

						/* Check the proximity */
						fdist  = great_circle_distance(&BaseMap,
														Ppos[iloc], fpos);
						fdist /= 1000.0;
						if ( fdist > proximity )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
										"   Feature beyond proximity for lat/lon ... %.1f %.1f\n",
										Plats[iloc], Plons[iloc]);
								}

							break;
							}

						/* Check if feature is on the map */
						if ( fpos[X] < 0.0 || fpos[Y] < 0.0
								|| fpos[X] > BaseMap.definition.xlen
								|| fpos[Y] > BaseMap.definition.ylen )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
										"   Feature outside map for lat/lon ... %.1f %.1f\n",
										Plats[iloc], Plons[iloc]);
								}

							if ( AnchorToMap || fit_to_map )
								{
								(void) remove_item_from_set(copyset,
																(ITEM) curve);
								continue;
								}
							}

						/* Determine bearing from sample to feature */
						fbear  = great_circle_bearing(&BaseMap,
														Ppos[iloc], fpos);

						/* Change sign of proximity wrt curve sense */
						(void) recall_curve_sense(curve, &sense);
						(void) curve_test_point(curve, Ppos[iloc], NullFloat,
								NullPoint, NullInt, NullLogical, &right);
						if ( (sense == Right && right)
								|| (sense == Left && !right) ) fdist = -fdist;

						/* Determine length of curve */
						flen = great_circle_line_length(&BaseMap, curve->line);

						/* Reset the sampling location */
						if ( AnchorToMap )
							{
							(void) anchored_location(fpos, 0.0, 0.0,
															&xxo, &yyo);
							xxo += xdoff;
							yyo += ydoff;

							/* Reset the display location (if required) */
							if ( display_at_feature )
								{
								xx = xxo + xoff;
								yy = yyo + yoff;
								}
							}

						/* Determine direction of curve */
						(void) line_span_info(curve->line, iseg,
													spos, epos, NullPoint);
						xdir = great_circle_bearing(&BaseMap, spos, epos);

						/* Display curve attributes ... if they match! */
						if ( GRA_display_sampled_attributes(Pids[iloc],
								vtime, time_zone, language,
								Plabs[iloc], fpos, fdist, fbear, flen,
								xdir, DefSegSpd, curve->attrib,
								cat_cascade, cat_attrib, num_catatt,
								attribs, num_attrib, list_case, num_list,
								inmark, markscale, display_name,
								display_type, fit_to_map, fit_to_map_ref,
								rotation, rotate_lat, rotate_lon,
								rot_attrib, constrain_rot,
								xxo, yyo, xx, yy) )
							break;

						/* Attributes did not match               */
						/*  ... so remove this curve from the set */
						(void) remove_item_from_set(copyset, (ITEM) curve);
						}

					/* Destroy what is left of the copy */
					(void) destroy_set(copyset);
					break;

				/* Display values for scattered type fields */
				case FpaC_SCATTERED:

					/* Extract the spot from the data file set */
					if ( !blank(data_file) )
						{

						/* Get the spot */
						spot = spots->list[iloc];
						if ( IsNull(spot) || IsNull(spot->attrib) ) continue;

						/* Display attributes from spot attribute structure */
						(void) GRA_display_sampled_attributes(Pids[iloc],
								vtime, time_zone, language,
								Plabs[iloc], spot->anchor, DefProximity,
								DefBearing, DefLineLen, DefSegDir, DefSegSpd,
								spot->attrib,
								cat_cascade, cat_attrib, num_catatt,
								attribs, num_attrib, list_case, num_list,
								inmark, markscale, display_name,
								display_type, fit_to_map, fit_to_map_ref,
								rotation, rotate_lat, rotate_lon,
								rot_attrib, constrain_rot,
								xxo, yyo, xx, yy);
						}

					/* Extract the spot from the set */
					else
						{
						/* Make a copy of the scattered spots */
						copyset = copy_set(spots);

						/* Search for closest spot within proximity */
						/*  that matches the required attributes    */
						while ( copyset->num > 0 )
							{

							/* Find the closest spot */
							spot = closest_spot(copyset, Ppos[iloc], NULL,
													NULL, NullFloat, fpos);
							if ( IsNull(spot) || IsNull(spot->attrib) )
								{
								(void) remove_item_from_set(copyset,
																(ITEM) spot);
								continue;
								}

							/* Check the proximity */
							fdist  = great_circle_distance(&BaseMap,
															Ppos[iloc], fpos);
							fdist /= 1000.0;
							if ( fdist > proximity )
								{
								if ( Verbose )
									{
									(void) fprintf(stdout,
											"   Feature beyond proximity for lat/lon ... %.1f %.1f\n",
											Plats[iloc], Plons[iloc]);
									}

								break;
								}

							/* Determine bearing from sample to feature */
							fbear  = great_circle_bearing(&BaseMap,
															Ppos[iloc], fpos);

							/* Check if feature is on the map */
							if ( fpos[X] < 0.0 || fpos[Y] < 0.0
									|| fpos[X] > BaseMap.definition.xlen
									|| fpos[Y] > BaseMap.definition.ylen )
								{
								if ( Verbose )
									{
									(void) fprintf(stdout,
											"   Feature outside map for lat/lon ... %.1f %.1f\n",
											Plats[iloc], Plons[iloc]);
									}

								if ( AnchorToMap || fit_to_map )
									{
									(void) remove_item_from_set(copyset,
																(ITEM) spot);
									continue;
									}
								}

							/* Reset the sampling location */
							if ( AnchorToMap )
								{
								(void) anchored_location(fpos, 0.0, 0.0,
																&xxo, &yyo);
								xxo += xdoff;
								yyo += ydoff;

								/* Reset the display location (if required) */
								if ( display_at_feature )
									{
									xx = xxo + xoff;
									yy = yyo + yoff;
									}
								}

							/* Display spot attributes ... if they match! */
							if ( GRA_display_sampled_attributes(Pids[iloc],
									vtime, time_zone, language,
									Plabs[iloc], fpos, fdist, fbear,
									DefLineLen, DefSegDir, DefSegSpd,
									spot->attrib,
									cat_cascade, cat_attrib, num_catatt,
									attribs, num_attrib, list_case, num_list,
									inmark, markscale, display_name,
									display_type, fit_to_map, fit_to_map_ref,
									rotation, rotate_lat, rotate_lon,
									rot_attrib, constrain_rot,
									xxo, yyo, xx, yy) )
								break;

							/* Attributes did not match              */
							/*  ... so remove this spot from the set */
							(void) remove_item_from_set(copyset, (ITEM) spot);
							}

						/* Destroy what is left of the copy */
						(void) destroy_set(copyset);
						}
					break;

				/* Display values for link chain type fields */
				case FpaC_LCHAIN:

					/* >>>>> Display link chain based on type? <<<<< */

					/* Make a copy of the link chains */
					copyset = copy_set(lchains);

					/* Search for closest link chain within proximity */
					/*  that matches the required attributes          */
					while ( copyset->num > 0 )
						{

						/* Find the closest link chain */
						lchain = closest_lchain(copyset, Ppos[iloc],
												NullFloat, fpos, &iseg);
						if ( IsNull(lchain) || IsNull(lchain->attrib) )
							{
							(void) remove_item_from_set(copyset, (ITEM) lchain);
							continue;
							}

						/* Check the proximity */
						fdist  = great_circle_distance(&BaseMap,
														Ppos[iloc], fpos);
						fdist /= 1000.0;
						if ( fdist > proximity )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
										"   Feature beyond proximity for lat/lon ... %.1f %.1f\n",
										Plats[iloc], Plons[iloc]);
								}

							break;
							}

						/* Check if feature is on the map */
						if ( fpos[X] < 0.0 || fpos[Y] < 0.0
								|| fpos[X] > BaseMap.definition.xlen
								|| fpos[Y] > BaseMap.definition.ylen )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
										"   Feature outside map for lat/lon ... %.1f %.1f\n",
										Plats[iloc], Plons[iloc]);
								}

							if ( AnchorToMap || fit_to_map )
								{
								(void) remove_item_from_set(copyset,
																(ITEM) lchain);
								continue;
								}
							}

						/* Determine bearing from sample to feature */
						fbear  = great_circle_bearing(&BaseMap,
														Ppos[iloc], fpos);

						/* Interpolate the link chain (if required) */
						if ( lchain->dointerp ) (void) interpolate_lchain(lchain);

						/* Determine length of link chain track */
						flen = great_circle_line_length(&BaseMap, lchain->track);

						/* Reset the sampling location */
						if ( AnchorToMap )
							{
							(void) anchored_location(fpos, 0.0, 0.0,
															&xxo, &yyo);
							xxo += xdoff;
							yyo += ydoff;

							/* Reset the display location (if required) */
							if ( display_at_feature )
								{
								xx = xxo + xoff;
								yy = yyo + yoff;
								}
							}

						/* Determine direction and speed on link chain track */
						if ( lchain->inum > 1 )
							{
							(void) line_span_info(lchain->track, iseg,
														spos, epos, NullPoint);
							xdir  = great_circle_bearing(&BaseMap, spos, epos);
							xspd  = point_dist(spos, epos) /
														(float) lchain->minterp;
							xspd *= BaseMap.definition.units;
							xspd /= 60.0;
							}
						else
							{
							xdir = DefSegDir;
							xspd = DefSegSpd;
							}

						/* Display link chain attributes ... if they match! */
						if ( GRA_display_sampled_attributes(Pids[iloc],
								vtime, time_zone, language,
								Plabs[iloc], fpos, fdist, fbear, flen,
								xdir, xspd, lchain->attrib,
								cat_cascade, cat_attrib, num_catatt,
								attribs, num_attrib, list_case, num_list,
								inmark, markscale, display_name,
								display_type, fit_to_map, fit_to_map_ref,
								rotation, rotate_lat, rotate_lon,
								rot_attrib, constrain_rot,
								xxo, yyo, xx, yy) )
							break;

						/* Attributes did not match                    */
						/*  ... so remove this link chain from the set */
						(void) remove_item_from_set(copyset, (ITEM) lchain);
						}

					/* Destroy what is left of the copy */
					(void) destroy_set(copyset);
					break;

				/* All other type fields have already been handled */
				default:
					break;
				}
			}

		/* Free the sampling structures */
		switch ( fkind )
			{

			/* Free sampling structures for continuous type fields */
			case FpaC_CONTINUOUS:
				(void) free_vlist(vlist);
				FREEMEM(vlist);
				break;

			/* Free sampling structures for vector type fields */
			case FpaC_VECTOR:
				(void) free_vlist(mlist);
				(void) free_vlist(dlist);
				FREEMEM(mlist);
				FREEMEM(dlist);
				break;

			/* Free sampling structures for discrete type fields */
			case FpaC_DISCRETE:
				(void) destroy_set(areas);
				break;

			/* Free sampling structures for line type fields */
			case FpaC_LINE:
				(void) destroy_set(curves);
				break;

			/* Free sampling structures for scattered type fields */
			case FpaC_SCATTERED:
				(void) destroy_set(spots);
				break;

			/* Free sampling structures for link chain type fields */
			case FpaC_LCHAIN:
				(void) destroy_set(lchains);
				break;
			}

		/* Free space used by idents and labels */
		FREELIST(Pids,  Nump);
		FREELIST(Plabs, Nump);

		/* Return TRUE when all list locations have been sampled */
		return TRUE;
		}

	/* Sample fields for each location in cross section */
	else if ( !blank(xsection_name) )
		{
		if ( Verbose )
			{
			if ( !blank(data_file) )
				(void) fprintf(stdout,
						"Cross section sampling from data file %s\n", data_file);
			else if ( !blank(geo_file) )
				(void) fprintf(stdout,
						"Cross section sampling for field ... %s %s from map file %s\n",
						element, level, geo_file);
			else if ( !blank(equation) )
				(void) fprintf(stdout,
						" Cross section sampling for equation ... %s from %s\n",
						equation, source);
			else
				(void) fprintf(stdout,
						" Cross section sampling for field ... %s %s from %s\n",
						element, level, source);
			}

		/* Ensure that cross section locations are NOT anchored to current */
		/*  map ... regardless of the current anchor position!             */
		cur_anchor  = AnchorToMap;
		AnchorToMap = FALSE;

		/* Find the named cross section */
		cur_xsect = get_cross_section(xsection_name);
		if ( IsNull(cur_xsect) )
			{
			(void) sprintf(err_buf, "Cross Section ... %s ... not yet defined",
					xsection_name);
			(void) error_report(err_buf);
			}

		/* Get horizontal axis parameters */
		haxis = cross_section_horizontal_axis(cur_xsect, loc_lookup, ltype,
											num_xloc, xsect_locs, NullDouble);
		if ( IsNull(haxis) )
			{
			(void) sprintf(err_buf,
					"Error in cross section horizontal look up ... %s",
					loc_lookup);
			(void) error_report(err_buf);
			}

		/* Get vertical axis parameters */
		vaxis = cross_section_vertical_axis(cur_xsect, ver_lookup, NullDouble);
		if ( IsNull(vaxis) )
			{
			(void) sprintf(err_buf,
					"Error in cross section vertical look up ... %s",
					ver_lookup);
			(void) error_report(err_buf);
			}

		/* Sample fields for all cross section locations */
		for ( iloc=0; iloc<haxis->num; iloc++ )
			{

			/* Set horizontal axis parameters for sampling */
			ident = haxis->idents[iloc];
			flat  = haxis->flats[iloc];
			flon  = haxis->flons[iloc];
			if ( !blank(haxis->vtimes[iloc]) ) vt = haxis->vtimes[iloc];
			else                               vt = vtime;
			label = haxis->labels[iloc];

			/* Set map position for sampling */
			(void) ll_to_pos(&BaseMap, flat, flon, pos[0]);

			/* Check for sample locations off the map */
			if ( pos[0][X] < 0.0 || pos[0][Y] < 0.0
					|| pos[0][X] > BaseMap.definition.xlen
					|| pos[0][Y] > BaseMap.definition.ylen )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Sample lat/lon outside map at ... %.1f %.1f\n",
							flat, flon);
					}
				}

			/* Comments for sample locations on the map */
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Sample lat/lon at ... %.1f %.1f\n",
							flat, flon);
					}
				}

			/* Reset field descriptor for cross section valid time */
			if ( !set_fld_descript(&descript, FpaF_VALID_TIME, vt,
											FpaF_END_OF_LIST) )
				{
				(void) sprintf(err_buf,
						"Error re-setting valid time ... %s\n", vt);
				(void) error_report(err_buf);
				}

			/* Set vertical location of parameter from data file */
			if ( !blank(ver_data_file) )
				{
				yloc = set_vertical_position_data_file(vaxis, ver_data_file,
						ver_data_file_format, ver_data_file_units,
						ident, flat, flon, vt, ver_attrib, units, &status);
				}

			/* Set vertical location of parameter */
			else
				{
				yloc = set_vertical_position(&descript, vaxis, pos[0],
						ver_element, ver_level, ver_equation, ver_units,
						ver_field_type, cat_cascade, cat_attrib, num_catatt,
						ver_attrib, ver_attrib_upper, ver_attrib_lower,
						proximity, &status);
				}
			if ( !status ) continue;

			/* Set cross section location for displaying */
			xxc = cur_xsect->x_off + (cur_xsect->width  * haxis->locs[iloc]);
			yyc = cur_xsect->y_off + (cur_xsect->height * yloc);
			(void) anchored_location(ZeroPoint, xxc, yyc, &xxo, &yyo);

			/* Set display location (offset by xdoff/ydoff) */
			/*  and offset location for sampled attributes  */
			xxo += xdoff;
			yyo += ydoff;
			xx = xxo + xoff;
			yy = yyo + yoff;

			/* Sample at cross section locations from data files */
			if ( !blank(data_file) )
				{

				/* Convert latitude and longitude to STRING format */
				(void) sprintf(mlat, "%f", flat);
				(void) sprintf(mlon, "%f", flon);

				/* Extract attributes from matching data file entry */
				if ( !match_data_file(data_file, data_file_format,
						data_file_units, data_file_wind_units,
						ident, mlat, mlon, vt, &cal) )
					{
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot match ident/lat/lon/vtime ... %s/%f/%f/%s  in data file ... %s",
							ident, flat, flon, vt, data_file);
						}
					continue;
					}

				/* Build a spot object from the data file attributes */
				spot = create_spot(pos[0], FpaCblank, AttachNone, cal);
				}

			/* Sample fields at cross section locations from geography files */
			/*  depending on field type                                      */
			else if ( !blank(geo_file) )
				{
				switch ( fkind )
					{

					/* Sample continuous type fields */
					case FpaC_CONTINUOUS:

						/* Create VLIST Object to hold sampled value */
						vlist = INITMEM(VLIST, 1);
						(void) init_vlist(vlist);

						/* Sample field at the requested position */
						status = eval_sfc(gsfc, pos[0], &dval);

						/* Skip to next location if field cannot be evaluated */
						if ( !status )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
									"Cannot sample field ... %s %s from map file %s\n",
									element, level, geo_file);
								}
							(void) free_vlist(vlist);
							FREEMEM(vlist);
							continue;
							}

						/* Add value to VLIST Object */
						(void) add_point_to_vlist(vlist, pos[0], (float) dval);
						break;

					/* Sample vector type fields */
					case FpaC_VECTOR:

						/* Create VLIST Objects to hold sampled values */
						mlist = INITMEM(VLIST, 1);
						(void) init_vlist(mlist);
						dlist = INITMEM(VLIST, 1);
						(void) init_vlist(dlist);

						/* Sample field at the requested position */
						status = eval_sfc_MD(gsfc, pos[0], &mval, &dval);

						/* Skip to next location if field cannot be evaluated */
						if ( !status )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
									"Cannot sample field ... %s %s from map file %s\n",
									element, level, geo_file);
								}
							(void) free_vlist(mlist);
							(void) free_vlist(dlist);
							FREEMEM(mlist);
							FREEMEM(dlist);
							continue;
							}

						/* Add values to VLIST Objects */
						(void) add_point_to_vlist(mlist, pos[0], (float) mval);
						(void) add_point_to_vlist(dlist, pos[0], (float) dval);
						break;

					/* Sample discrete type fields */
					case FpaC_DISCRETE:

						/* Copy the discrete areas */
						areas = copy_set(gset);

						/* Skip to next location if field cannot be evaluated */
						if ( IsNull(areas) )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
									"Cannot extract field ... %s %s from map file %s\n",
									element, level, geo_file);
								}
							continue;
							}
						break;

					/* Sample line type fields */
					case FpaC_LINE:

						/* Copy the line curves */
						curves = copy_set(gset);

						/* Skip to next location if field cannot be evaluated */
						if ( IsNull(curves) )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
									"Cannot extract field ... %s %s from map file %s\n",
									element, level, geo_file);
								}
							continue;
							}
						break;

					/* Sample scattered type fields */
					case FpaC_SCATTERED:

						/* Copy the scattered spots */
						spots = copy_set(gset);

						/* Skip to next location if field cannot be evaluated */
						if ( IsNull(spots) )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
									"Cannot extract field ... %s %s from map file %s\n",
									element, level, geo_file);
								}
							continue;
							}
						break;

					/* Sample link chain type fields */
					case FpaC_LCHAIN:

						/* Copy the link chains */
						lchains = copy_set(gset);

						/* Skip to next location if field cannot be evaluated */
						if ( IsNull(lchains) )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
									"Cannot extract field ... %s %s from map file %s\n",
									element, level, geo_file);
								}
							continue;
							}
						break;

					/* Cannot sample other types of fields */
					default:
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot sample field ... %s %s from map file %s\n",
								element, level, geo_file);
							}
						continue;
					}
				}

			/* Sample fields at cross section locations from all other files */
			/*  depending on field type                                      */
			else
				{
				switch ( fkind )
					{

					/* Sample continuous type fields */
					case FpaC_CONTINUOUS:

						/* Extract field value by equation */
						if ( !blank(equation) )
							vlist = retrieve_vlist_by_equation(&descript,
												1, pos, FpaCmksUnits, equation);

						/* Extract field value directly */
						else
							vlist = retrieve_vlist(&descript, 1, pos);

						/* Skip to next location if field cannot be evaluated */
						if ( IsNull(vlist) )
							{
							if ( Verbose )
								{
								if ( !blank(equation) )
									(void) fprintf(stdout,
										"Cannot calculate equation ... %s from %s at %s\n",
										equation, source, vt);
								else
									(void) fprintf(stdout,
										"Cannot extract field ... %s %s from %s at %s\n",
										element, level, source, vt);
								}
							continue;
							}
						break;

					/* Sample vector type fields */
					case FpaC_VECTOR:

						/* Extract magnitude and direction of vector field */
						mlist = retrieve_vlist_component(&descript, M_Comp,
																		1, pos);
						dlist = retrieve_vlist_component(&descript, D_Comp,
																		1, pos);

						/* Skip to next location if field cannot be evaluated */
						if ( IsNull(mlist) || IsNull(dlist) )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
									"Cannot extract field ... %s %s from %s at %s\n",
									element, level, source, vt);
								}
							continue;
							}
						break;

					/* Sample discrete type fields */
					case FpaC_DISCRETE:

						/* Find the field containing the discrete areas */
						areas = retrieve_areaset(&descript);

						/* Skip to next location if field cannot be evaluated */
						if ( IsNull(areas) )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
									"Cannot extract field ... %s %s from %s at %s\n",
									element, level, source, vt);
								}
							continue;
							}
						break;

					/* Sample line type fields */
					case FpaC_LINE:

						/* Find the field containing the line curves */
						curves = retrieve_curveset(&descript);

						/* Skip to next location if field cannot be evaluated */
						if ( IsNull(curves) )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
									"Cannot extract field ... %s %s from %s at %s\n",
									element, level, source, vt);
								}
							continue;
							}
						break;

					/* Sample scattered type fields */
					case FpaC_SCATTERED:

						/* Find the field containing the scattered spots */
						spots = retrieve_spotset(&descript);

						/* Skip to next location if field cannot be evaluated */
						if ( IsNull(spots) )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
									"Cannot extract field ... %s %s from %s at %s\n",
									element, level, source, vt);
								}
							continue;
							}
						break;

					/* Sample link chain type fields */
					case FpaC_LCHAIN:

						/* Find the field containing the link chains */
						lchains = retrieve_lchainset(&descript);

						/* Skip to next location if field cannot be evaluated */
						if ( IsNull(lchains) )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
									"Cannot extract field ... %s %s from %s at %s\n",
									element, level, source, vt);
								}
							continue;
							}
						break;

					/* Cannot sample other types of fields */
					default:
						if ( Verbose )
							{
							(void) fprintf(stdout,
								"Cannot sample field ... %s %s from %s at %s\n",
								element, level, source, vt);
							}
						continue;
					}
				}

			/* Display sampled value depending on field type */
			switch ( fkind )
				{

				/* Display sampled value for continuous type fields */
				case FpaC_CONTINUOUS:

					/* Convert value to required units */
					(void) convert_value(FpaCmksUnits,
							(double) vlist->val[0], units, &dval);

					/* Display the field value */
					(void) GRA_display_sampled_value(ident,
							vt, time_zone, language,
							label, pos[0], (float) dval, attribs, num_attrib,
							list_case, num_list, inmark, markscale,
							display_name, display_type, fit_to_map,
							fit_to_map_ref, rotation, xxo, yyo, xx, yy);
					break;

				/* Display sampled value for vector type fields */
				case FpaC_VECTOR:

					/* Convert magnitude to required units */
					(void) convert_value(FpaCmksUnits,
							(double) mlist->val[0], units, &mval);

					/* Convert sampled direction to degrees true */
					dang = wind_dir_true(&BaseMap, flat, flon,
							dlist->val[0]);

					/* >>>>> debug testing for vector fields in sample_field() <<<<< */
					if ( DebugMode )
						{
						(void) fprintf(stdout,
							"Vector field at %.1f %.1f ... m/d: %.1f %.1f  mval/dang: %.1f %.1f\n",
							flat, flon, mlist->val[0], dlist->val[0], mval, dang);
						}
					/* >>>>> debug testing for vector fields in sample_field() <<<<< */

					/* Display the vector */
					(void) GRA_display_sampled_vector(ident,
							vt, time_zone, language, label,
							pos[0], (float) mval, dang, attribs, num_attrib,
							list_case, num_list, inmark, markscale,
							display_name, display_type, fit_to_map,
							fit_to_map_ref, rotation, xxo, yyo, xx, yy);
					break;

				/* Display sampled value for discrete type fields */
				case FpaC_DISCRETE:

					/* Sample areas based on proximity */
					if ( proximity > 0.0 )
						{

						/* Make a copy of the area or subareas in the area */
						copyset = create_set("area");
						for ( iarea=0; iarea<areas->num; iarea++ )
							{
							area = (AREA) areas->list[iarea];

							/* Ensure that area is within proximity */
							(void) area_test_point(area, pos[0], NULL,
													fpos, NULL, NULL, NULL,
													&inside);
							if ( !inside )
								{
								fdist  = great_circle_distance(&BaseMap,
																pos[0], fpos);
								fdist /= 1000.0;
								if ( fdist > proximity ) continue;
								}

							/* Copy the area if no subareas */
							if ( area->numdiv == 0)
								{
								carea = copy_area(area, FALSE);
								if ( NotNull(carea) )
									{
									(void) add_item_to_set(copyset,
															(ITEM) carea);
									}
								}

							/* Create an area from each subarea */
							else
								{
								for ( isub=0; isub<=area->numdiv; isub++ )
									{
									subarea = area->subareas[isub];
									carea = area_from_subarea(subarea);
									if ( NotNull(carea) )
										{
										(void) add_item_to_set(copyset,
																(ITEM) carea);
										}
									}
								}
							}

						/* Search for closest area within proximity */
						/*  that matches the required attributes    */
						while ( copyset->num > 0 )
							{

							/* Find an enclosing area */
							area = enclosing_area(copyset, pos[0],
												PickFirst, NullFloat, NullChar);

							/* Set sampling point and proximity (if inside) */
							if ( NotNull(area) )
								{
								(void) copy_point(fpos, pos[0]);
								inside = TRUE;
								fdist  = 0.0;
								fbear  = 0.0;
								}

							/* Find the closest area within proximity */
							else
								{
								area   = closest_area(copyset, pos[0],
												NULL, fpos, NULL, NULL, NULL);
								inside = FALSE;
								fdist  = great_circle_distance(&BaseMap,
															pos[0], fpos);
								fdist /= 1000.0;
								if ( fdist > proximity )
									{
									if ( Verbose )
										{
										(void) fprintf(stdout,
												"   Feature beyond proximity for lat/lon ... %.1f %.1f\n",
												flat, flon);
										}

									break;
									}

								/* Determine bearing from sample to feature */
								fbear  = great_circle_bearing(&BaseMap,
															pos[0], fpos);
								}

							/* Ensure that feature is on the map */
							if ( fpos[X] < 0.0 || fpos[Y] < 0.0
									|| fpos[X] > BaseMap.definition.xlen
									|| fpos[Y] > BaseMap.definition.ylen )
								{
								if ( Verbose )
									{
									(void) fprintf(stdout,
											"   Feature outside map for lat/lon ... %.1f %.1f\n",
											flat, flon);
									}

								(void) remove_item_from_set(copyset,
																(ITEM) area);
								continue;
								}

							/* Display area attributes ... if they match! */
							if ( GRA_display_sampled_attributes(ident,
									vt, time_zone, language,
									label, fpos, fdist, fbear, DefLineLen,
									DefSegDir, DefSegSpd, area->attrib,
									cat_cascade, cat_attrib, num_catatt,
									attribs, num_attrib, list_case, num_list,
									inmark, markscale, display_name,
									display_type, fit_to_map, fit_to_map_ref,
									rotation, rotate_lat, rotate_lon,
									rot_attrib, constrain_rot,
									xxo, yyo, xx, yy) )
								break;

							/* Attributes did not match              */
							/*  ... so remove this area from the set */
							(void) remove_item_from_set(copyset, (ITEM) area);
							}

						/* Destroy what is left of the copy */
						(void) destroy_set(copyset);
						break;
						}

					/* Sample areas directly */
					else
						{

						/* Find area enclosing point                        */
						/* Note that this should return a background value  */
						/*  with no subarea if no enclosing area is found   */
						/*  ... except map files, which have no background! */
						if ( !eval_areaset(areas, pos[0], PickFirst,
								&subarea, &cal) )
							{
							if ( blank(geo_file) && Verbose )
								{
								(void) fprintf(stdout,
									"No areas or background for field ... %s %s from %s at %s\n",
									element, level, source, vt);
								}
							continue;
							}

						/* Display attributes from attribute structure */
						(void) GRA_display_sampled_attributes(ident,
								vt, time_zone, language,
								label, pos[0], DefProximity, DefBearing,
								DefLineLen, DefSegDir, DefSegSpd, cal,
								cat_cascade, cat_attrib, num_catatt,
								attribs, num_attrib, list_case, num_list,
								inmark, markscale, display_name,
								display_type, fit_to_map, fit_to_map_ref,
								rotation, rotate_lat, rotate_lon,
								rot_attrib, constrain_rot,
								xxo, yyo, xx, yy);
						}
					break;

				/* Display sampled value for line type fields */
				case FpaC_LINE:

					/* Make a copy of the line curves */
					copyset = copy_set(curves);

					/* Search for closest curve within proximity */
					/*  that matches the required attributes     */
					while ( copyset->num > 0 )
						{

						/* Find the closest curve */
						curve = closest_curve(copyset, pos[0],
												NullFloat, fpos, &iseg);
						if ( IsNull(curve) || IsNull(curve->attrib) )
							{
							(void) remove_item_from_set(copyset, (ITEM) curve);
							continue;
							}

						/* Check the proximity */
						fdist  = great_circle_distance(&BaseMap, pos[0], fpos);
						fdist /= 1000.0;
						if ( fdist > proximity )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
										"   Feature beyond proximity for lat/lon ... %.1f %.1f\n",
										flat, flon);
								}

							break;
							}

						/* Check if feature is on the map */
						if ( fpos[X] < 0.0 || fpos[Y] < 0.0
								|| fpos[X] > BaseMap.definition.xlen
								|| fpos[Y] > BaseMap.definition.ylen )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
										"   Feature outside map for lat/lon ... %.1f %.1f\n",
										flat, flon);
								}
							}

						/* Determine bearing from sample to feature */
						fbear  = great_circle_bearing(&BaseMap, pos[0], fpos);

						/* Change sign of proximity wrt curve sense */
						(void) recall_curve_sense(curve, &sense);
						(void) curve_test_point(curve, pos[0], NullFloat,
								NullPoint, NullInt, NullLogical, &right);
						if ( (sense == Right && right)
								|| (sense == Left && !right) ) fdist = -fdist;

						/* Determine length of curve */
						flen = great_circle_line_length(&BaseMap, curve->line);

						/* Determine direction of curve */
						(void) line_span_info(curve->line, iseg,
													spos, epos, NullPoint);
						xdir = great_circle_bearing(&BaseMap, spos, epos);

						/* Display curve attributes ... if they match! */
						if ( GRA_display_sampled_attributes(ident,
								vt, time_zone, language,
								label, fpos, fdist, fbear, flen,
								xdir, DefSegSpd, curve->attrib,
								cat_cascade, cat_attrib, num_catatt,
								attribs, num_attrib, list_case, num_list,
								inmark, markscale, display_name,
								display_type, fit_to_map, fit_to_map_ref,
								rotation, rotate_lat, rotate_lon,
								rot_attrib, constrain_rot,
								xxo, yyo, xx, yy) )
							break;

						/* Attributes did not match               */
						/*  ... so remove this curve from the set */
						(void) remove_item_from_set(copyset, (ITEM) curve);
						}

					/* Destroy what is left of the copy */
					(void) destroy_set(copyset);
					break;

				/* Display sampled value for scattered type fields */
				case FpaC_SCATTERED:

					/* Display attributes from data file attribute structure */
					if ( !blank(data_file) )
						{
						(void) GRA_display_sampled_attributes(ident,
								vt, time_zone, language,
								label, spot->anchor, DefProximity,
								DefBearing, DefLineLen, DefSegDir, DefSegSpd,
								spot->attrib,
								cat_cascade, cat_attrib, num_catatt,
								attribs, num_attrib, list_case, num_list,
								inmark, markscale, display_name,
								display_type, fit_to_map, fit_to_map_ref,
								rotation, rotate_lat, rotate_lon,
								rot_attrib, constrain_rot,
								xxo, yyo, xx, yy);
						}

					/* Display attributes from closest spot attribute structure */
					else
						{
						/* Make a copy of the scattered spots */
						copyset = copy_set(spots);

						/* Search for closest spot within proximity */
						/*  that matches the required attributes    */
						while ( copyset->num > 0 )
							{

							/* Find the closest spot */
							spot = closest_spot(copyset, pos[0], NULL,
													NULL, NullFloat, fpos);
							if ( IsNull(spot) || IsNull(spot->attrib) )
								{
								(void) remove_item_from_set(copyset,
																(ITEM) spot);
								continue;
								}

							/* Check the proximity */
							fdist  = great_circle_distance(&BaseMap,
																pos[0], fpos);
							fdist /= 1000.0;
							if ( fdist > proximity )
								{
								if ( Verbose )
									{
									(void) fprintf(stdout,
											"   Feature beyond proximity for lat/lon ... %.1f %.1f\n",
											flat, flon);
									}

								break;
								}

							/* Determine bearing from sample to feature */
							fbear  = great_circle_bearing(&BaseMap,
																pos[0], fpos);

							/* Check if feature is on the map */
							if ( fpos[X] < 0.0 || fpos[Y] < 0.0
									|| fpos[X] > BaseMap.definition.xlen
									|| fpos[Y] > BaseMap.definition.ylen )
								{
								if ( Verbose )
									{
									(void) fprintf(stdout,
											"   Feature outside map for lat/lon ... %.1f %.1f\n",
											flat, flon);
									}
								}

							/* Display spot attributes ... if they match! */
							if ( GRA_display_sampled_attributes(ident,
									vt, time_zone, language,
									label, fpos, fdist, fbear, DefLineLen,
									DefSegDir, DefSegSpd, spot->attrib,
									cat_cascade, cat_attrib, num_catatt,
									attribs, num_attrib, list_case, num_list,
									inmark, markscale, display_name,
									display_type, fit_to_map, fit_to_map_ref,
									rotation, rotate_lat, rotate_lon,
									rot_attrib, constrain_rot,
									xxo, yyo, xx, yy) )
								break;

							/* Attributes did not match              */
							/*  ... so remove this spot from the set */
							(void) remove_item_from_set(copyset, (ITEM) spot);
							}

						/* Destroy what is left of the copy */
						(void) destroy_set(copyset);
						}
					break;

				/* Display sampled value for link chain type fields */
				case FpaC_LCHAIN:

					/* >>>>> Display link chain based on type? <<<<< */

					/* Make a copy of the link chains */
					copyset = copy_set(lchains);

					/* Search for closest link chain within proximity */
					/*  that matches the required attributes          */
					while ( copyset->num > 0 )
						{

						/* Find the closest link chain */
						lchain = closest_lchain(copyset, pos[0],
												NullFloat, fpos, &iseg);
						if ( IsNull(lchain) || IsNull(lchain->attrib) )
							{
							(void) remove_item_from_set(copyset, (ITEM) lchain);
							continue;
							}

						/* Check the proximity */
						fdist  = great_circle_distance(&BaseMap, pos[0], fpos);
						fdist /= 1000.0;
						if ( fdist > proximity )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
										"   Feature beyond proximity for lat/lon ... %.1f %.1f\n",
										flat, flon);
								}

							break;
							}

						/* Check if feature is on the map */
						if ( fpos[X] < 0.0 || fpos[Y] < 0.0
								|| fpos[X] > BaseMap.definition.xlen
								|| fpos[Y] > BaseMap.definition.ylen )
							{
							if ( Verbose )
								{
								(void) fprintf(stdout,
										"   Feature outside map for lat/lon ... %.1f %.1f\n",
										flat, flon);
								}
							}

						/* Determine bearing from sample to feature */
						fbear  = great_circle_bearing(&BaseMap, pos[0], fpos);

						/* Interpolate the link chain (if required) */
						if ( lchain->dointerp ) (void) interpolate_lchain(lchain);

						/* Determine length of link chain track */
						flen = great_circle_line_length(&BaseMap, lchain->track);

						/* Determine direction and speed on link chain track */
						if ( lchain->inum > 1 )
							{
							(void) line_span_info(lchain->track, iseg,
														spos, epos, NullPoint);
							xdir  = great_circle_bearing(&BaseMap, spos, epos);
							xspd  = point_dist(spos, epos) /
														(float) lchain->minterp;
							xspd *= BaseMap.definition.units;
							xspd /= 60.0;
							}
						else
							{
							xdir = DefSegDir;
							xspd = DefSegSpd;
							}

						/* Display link chain attributes ... if they match! */
						if ( GRA_display_sampled_attributes(ident,
								vt, time_zone, language,
								label, fpos, fdist, fbear, flen,
								xdir, xspd, lchain->attrib,
								cat_cascade, cat_attrib, num_catatt,
								attribs, num_attrib, list_case, num_list,
								inmark, markscale, display_name,
								display_type, fit_to_map, fit_to_map_ref,
								rotation, rotate_lat, rotate_lon,
								rot_attrib, constrain_rot,
								xxo, yyo, xx, yy) )
							break;

						/* Attributes did not match                    */
						/*  ... so remove this link chain from the set */
						(void) remove_item_from_set(copyset, (ITEM) lchain);
						}

					/* Destroy what is left of the copy */
					(void) destroy_set(copyset);
					break;

				/* All other type fields have already been handled */
				default:
					break;
				}

			/* Free the sampling structures */
			switch ( fkind )
				{

				/* Free sampling structures for continuous type fields */
				case FpaC_CONTINUOUS:
					(void) free_vlist(vlist);
					FREEMEM(vlist);
					break;

				/* Free sampling structures for vector type fields */
				case FpaC_VECTOR:
					(void) free_vlist(mlist);
					(void) free_vlist(dlist);
					FREEMEM(mlist);
					FREEMEM(dlist);
					break;

				/* Free sampling structures for discrete type fields */
				case FpaC_DISCRETE:
					(void) destroy_set(areas);
					break;

				/* Free sampling structures for line type fields */
				case FpaC_LINE:
					(void) destroy_set(curves);
					break;

				/* Free sampling structures for scattered type fields */
				case FpaC_SCATTERED:
					if ( !blank(data_file) ) (void) destroy_spot(spot);
					else                     (void) destroy_set(spots);
					break;

				/* Free sampling structures for link chain type fields */
				case FpaC_LCHAIN:
					(void) destroy_set(lchains);
					break;
				}
			}

		/* Reset the current anchor */
		AnchorToMap = cur_anchor;

		/* Return TRUE when all cross section locations have been sampled */
		return TRUE;
		}

	/* Return FALSE if no lat/lon or table or grid or cross section */
	(void) fprintf(stderr, " No lat/lon or table or grid or cross section\n");
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*    G R A _ s a m p l e _ w i n d                                     *
*                                                                      *
***********************************************************************/

LOGICAL			GRA_sample_wind

	(
	STRING			wcref,			/* Wind cross reference */
	STRING			units,			/* Units for wind speed/gust */
	STRING			format,			/* Format of sampled wind */
	STRING			inmark,			/* Name of location marker */
	float			markscale,		/* Marker scaling factor (percent) */
	STRING			display_name,	/* Name for label display */
	STRING			display_type,	/* Type of label display */
	float			wthscale,		/* Width of characters (percent) */
	float			hgtscale,		/* Height of characters (percent) */
	LOGICAL			fit_to_map,		/* Fit display within map? */
	float			rotation,		/* Label rotation (degrees) */
	LOGICAL			rotate_lat,		/* Rotate label to align with latitude? */
	LOGICAL			rotate_lon,		/* Rotate label to align with longitude? */
	float			xdoff,			/* x offset of wind and mark */
	float			ydoff,			/* y offset of wind and mark */
	float			xoff,			/* x offset of wind (display units) */
	float			yoff,			/* x offset of wind (display units) */
	STRING			lat,
	STRING			lon,
	STRING			map_x,
	STRING			map_y,
	float			map_units,
	STRING			loc_ident,
	STRING			loc_lookup,
	STRING			table_name,		/* Table name for locations */
	STRING			grid_name,		/* Grid name for locations */
	STRING			list_name,		/* List name for locations */
	STRING			source,			/* Source name for field to sample */
	STRING			vtime,			/* Valid time for field to sample */
	STRING			xsection_name,	/* Cross section to sample */
	GPGltype		ltype,			/* Type of locations for horizontal axis */
	int				num_xloc,		/* Number of locations for horizontal */
	XSECT_LOCATION	*xsect_locs,	/* Locations for horizontal axis */
	STRING			ver_lookup,
	STRING			ver_element,
	STRING			ver_level,
	STRING			ver_equation,
	STRING			ver_units,
	STRING			ver_attrib,
	STRING			ver_attrib_upper,
	STRING			ver_attrib_lower
	)

	{
	int				iix, iiy, isite, iloc, iil;
	float			mscale, pscale;
	float			clon, flat, flon, fspd, fdir, fact, rotadj, tspd, tdir;
	float			yloc, xxo, yyo, xx, yy, xxt, yyt, xxs, yys, xxc, yyc;
	float			dir, spd, gst;
	double			dval;
	STRING			loclat, loclon, vt, wunits;
	LOGICAL			status, cur_anchor;
	POINT			pos[1] = { ZERO_POINT };
	float			wdir[1], wspd[1], wgst[1];
	GRA_TABLE		*cur_table;
	GRA_GRID		*cur_grid;
	GRA_LIST		*cur_list;
	GRA_XSECT		*cur_xsect;
	XSECT_HOR_AXIS	*haxis;
	XSECT_VER_AXIS	*vaxis;
	char			err_buf[GPGLong];

	FLD_DESCRIPT	descript;

	/* Arrays for table or grid sampling */
	static	int		Nump   = 0;
	static	POINT	*Ppos  = NullPointList;
	static	float	*Plats = NullFloat;
	static	float	*Plons = NullFloat;
	static	float	*Pdirs = NullFloat;
	static	float	*Pspds = NullFloat;
	static	float	*Pgsts = NullFloat;

	/* Storage for default attribute for display of wind parameters */
	static	ATTRIB_DISPLAY	WindDisplay = NO_ATTRIB_DISPLAY;

	/* Make a copy of the global field descriptor */
	(void) copy_fld_descript(&descript, &Fdesc);

	/* Re-initialize the field descriptor for this wind calculation */
	if ( !set_fld_descript(&descript, FpaF_DIRECTORY_PATH, FpaCblank,
									   FpaF_SOURCE_NAME,   source,
									   FpaF_RUN_TIME,      FpaCblank,
									   FpaF_ELEMENT_NAME,  FpaCblank,
									   FpaF_LEVEL_NAME,    FpaCblank,
									   FpaF_VALID_TIME,    vtime,
									   FpaF_END_OF_LIST) )
		{
		(void) sprintf(err_buf,
				" Error setting field descriptor for ... %s at %s\n",
				source, vtime);
		(void) error_report(err_buf);
		}

	/* Set centre longitude from current map projection */
	(void) grid_center(&BaseMap, NullPointPtr, NullFloat, &clon);

	/* Sample winds for all locations in a location look up table */
	if ( !blank(loc_ident) && !blank(loc_lookup)
				&& ( same_ic(loc_ident, LocIdentAll)
						|| same_ic(loc_ident, LocIdentAllVtime) ) )
		{

		/* Ensure that samples can be displayed on current map */
		if ( !AnchorToMap )
			{
			(void) warn_report("Must set anchor to map!");
			return TRUE;
			}

		/* Set valid time to check */
		if ( same_ic(loc_ident, LocIdentAllVtime) ) vt = vtime;
		else                                        vt = FpaCblank;

		if ( Verbose )
			{
			(void) fprintf(stdout, " Lookup sampling for wind ... %s from %s at %s\n",
					wcref, source, vtime);
			}

		/* Set sample locations for all look up table locations */
		Nump = 0;
		iloc = -1;
		while ( (iloc = next_location_lookup_line(loc_lookup, iloc, vt,
						NullStringPtr, &loclat, &loclon, NullStringPtr)) >= 0 )
			{

			/* Set latitude/longitude for sampling */
			flat = read_lat(loclat, &status);
			if ( !status )
				(void) error_report("Problem with location look up lat");
			flon = read_lon(loclon, &status);
			if ( !status )
				(void) error_report("Problem with location look up lon");

			/* Allocate space for this sampling location */
			Nump++;
			Ppos  = GETMEM(Ppos,  POINT,  Nump);
			Plats = GETMEM(Plats, float,  Nump);
			Plons = GETMEM(Plons, float,  Nump);

			/* Set map position for sampling */
			Plats[Nump-1] = flat;
			Plons[Nump-1] = flon;
			(void) ll_to_pos(&BaseMap, flat, flon, Ppos[Nump-1]);
			}

		/* Sample winds at all look up table locations */
		Pdirs = GETMEM(Pdirs, float, Nump);
		Pspds = GETMEM(Pspds, float, Nump);
		Pgsts = GETMEM(Pgsts, float, Nump);
		status = extract_awind_by_crossref(wcref, &descript, FALSE,
					Nump, Ppos, clon, Pdirs, Pspds, Pgsts, &wunits);

		/* Return now if winds could not be sampled */
		if ( !status )
			{

			if ( Verbose )
				{
				(void) fprintf(stdout,
					"Cannot extract wind ... %s from %s at %s\n",
					wcref, source, vtime);
				}

			/* Return TRUE */
			return TRUE;
			}

		/* Display winds for all look up table locations */
		for ( isite=0; isite<Nump; isite++ )
			{

			/* Check for sample locations off the map */
			if ( Ppos[isite][X] < 0.0 || Ppos[isite][Y] < 0.0
					|| Ppos[isite][X] > BaseMap.definition.xlen
					|| Ppos[isite][Y] > BaseMap.definition.ylen )
				{

				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Sample lat/lon outside map at ... %.1f %.1f\n",
							Plats[isite], Plons[isite]);
					}

				/* Continue for sample locations off map */
				continue;
				}

			/* Comments for sample locations on the map */
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Sample lat/lon at ... %.1f %.1f\n",
							Plats[isite], Plons[isite]);
					}
				}

			/* Set display location on the current map */
			(void) anchored_location(Ppos[isite], 0.0, 0.0, &xxo, &yyo);

			/* Set display location (offset by xdoff/ydoff) */
			/*  and offset location for sampled winds       */
			xxo += xdoff;
			yyo += ydoff;
			xx = xxo + xoff;
			yy = yyo + yoff;

			/* Adjust symbol scale for perspective (if required) */
			mscale = markscale;
			if ( perspective_scale(&pscale) ) mscale *= pscale;

			/* Set rotation from latitude or longitude */
			if ( AnchorToMap && rotate_lat )
				rotadj = wind_dir_xy(&BaseMap, Plats[isite], Plons[isite], 90.0);
			else if ( AnchorToMap && rotate_lon )
				rotadj = wind_dir_xy(&BaseMap, Plats[isite], Plons[isite],  0.0);
			else
				rotadj = 0.0;

			/* Add default rotation */
			rotadj += rotation;

			/* Display the mark (if requested) ... not offset! */
			if ( !blank(inmark) )
				{
				(void) write_graphics_symbol(inmark, xxo, yyo, mscale, rotadj);
				}

			/* Set final wind parameters */
			dir = (float) Pdirs[isite];
			(void) convert_value(wunits, (double) Pspds[isite],
					units, &dval);
			spd = (float) dval;
			(void) convert_value(wunits, (double) Pgsts[isite],
					units, &dval);
			gst = (float) dval;

			/* Display graphics outline for sampled wind (if requested) */
			if ( !blank(display_name) )
				{

				/* Set display parameters in default attribute structure */
				(void) strcpy(WindDisplay.units,        units);
				(void) strcpy(WindDisplay.format,       format);
				(void) strcpy(WindDisplay.display_name, display_name);
				(void) strcpy(WindDisplay.display_type, display_type);
				WindDisplay.width_scale  = wthscale;
				WindDisplay.height_scale = hgtscale;
				WindDisplay.x_off        = xx;
				WindDisplay.y_off        = yy;

				/* Determine placement for winds displayed as wind barbs */
				/* Then display the graphics outline                     */
				if ( same(format, FormatWindBarb) )
					{
					(void) set_wind_barb_placement(&WindDisplay,
							dir, spd, gst);
					(void) GRA_display_attribute_outline(NullCal, WindDisplay,
							Plats[isite], Plons[isite], 0.0, 0.0, rotation);
					}

				/* Determine placement for winds displayed               */
				/*  as text or symbols from the given wind look up table */
				/* Then display the graphics outline                     */
				else if ( same(format, FormatWindText)
						|| same(format, FormatWindSymbol) )
					{
					(void) set_wind_placement(format, &WindDisplay,
							dir, spd, gst, Plats[isite], Plons[isite]);
					(void) GRA_display_attribute_outline(NullCal, WindDisplay,
							Plats[isite], Plons[isite], 0.0, 0.0, rotadj);
					}
				}

			/* Then display the sampled wind based on the format */
			if ( same(format, FormatWindBarb) )
				{
				if ( !GRA_display_windbarb(dir, spd, gst,
						Plats[isite], Plons[isite], xx, yy,
						wthscale, hgtscale, rotadj) ) continue;
				}
			else
				{
				if ( !GRA_display_wind(format, dir, spd, gst,
						Plats[isite], Plons[isite], xx, yy,
						wthscale, hgtscale, rotadj) ) continue;
				}
			}

		/* Return TRUE when all look up table locations have been sampled */
		return TRUE;
		}

	/* Sample winds by latitude and longitude */
	else if ( ( !blank(lat) && !blank(lon) )
			|| ( !blank(map_x) && !blank(map_y) )
			|| ( !blank(loc_ident) && !blank(loc_lookup) ) )
		{

		if ( Verbose )
			{
			(void) fprintf(stdout, " Sampling for wind ... %s from %s at %s\n",
					wcref, source, vtime);
			}

		/* Set the latitude and longitude */
		if ( !blank(lat) && !blank(lon) )
			{
			flat = read_lat(lat, &status);
			if ( !status ) (void) error_report("Problem with lat");
			flon = read_lon(lon, &status);
			if ( !status ) (void) error_report("Problem with lon");

			/* Convert latitude and longitude to map position */
			(void) ll_to_pos(&BaseMap, flat, flon, pos[0]);
			}

		/* Set the map position (adjusted by map_units) */
		else if ( !blank(map_x) && !blank(map_y) )
			{
			(void) sscanf(map_x, "%f", &pos[0][X]);
			(void) sscanf(map_y, "%f", &pos[0][Y]);
			fact = map_units / BaseMap.definition.units;
			pos[0][X] *= fact;
			pos[0][Y] *= fact;

			/* Convert map position to latitude and longitude */
			(void) pos_to_ll(&BaseMap, pos[0], &flat, &flon);
			}

		/* Get the latitude and longitude from location look up table */
		else
			{
			if ( Verbose )
				{
				(void) fprintf(stdout, "   Matching location ident ... \"%s\"",
						loc_ident);
				(void) fprintf(stdout, "  from look up ... %s\n",
						loc_lookup);
				}
			if ( !match_location_lookup(loc_lookup, loc_ident,
					vtime, &loclat, &loclon, NullStringPtr) )
				{
				(void) sprintf(err_buf,
						"Error matching \"%s\" in look up ... %s",
						loc_ident, loc_lookup);
				(void) error_report(err_buf);
				}
			flat = read_lat(loclat, &status);
			if ( !status )
				(void) error_report("Problem with location look up lat");
			flon = read_lon(loclon, &status);
			if ( !status )
				(void) error_report("Problem with location look up lon");

			/* Convert latitude and longitude to map position */
			(void) ll_to_pos(&BaseMap, flat, flon, pos[0]);
			}

		/* Check for sample locations off the map */
		if ( pos[0][X] < 0.0 || pos[0][Y] < 0.0
				|| pos[0][X] > BaseMap.definition.xlen
				|| pos[0][Y] > BaseMap.definition.ylen )
			{
			if ( Verbose )
				{
				if ( !blank(map_x) && !blank(map_y) )
					(void) fprintf(stdout,
							"  Sample position off map at ... %f %f\n",
							pos[0][X], pos[0][Y]);
				else
					(void) fprintf(stdout,
							"  Sample lat/lon off map at ... %.1f %.1f\n",
							flat, flon);
				}

			/* Return (if requested) for map samples off map */
			if ( AnchorToMap ) return TRUE;
			else if ( fit_to_map ) return TRUE;
			}

		/* Comments for sample locations on the map */
		else
			{
			if ( Verbose )
				{
				if ( !blank(map_x) && !blank(map_y) )
					(void) fprintf(stdout,
							"  Sample position at ... %f %f\n",
							pos[0][X], pos[0][Y]);
				else
					(void) fprintf(stdout,
							"  Sample lat/lon at ... %.1f %.1f\n",
							flat, flon);
				}
			}

		/* Set display location on the current map */
		if ( AnchorToMap )
			{
			(void) anchored_location(pos[0], 0.0, 0.0, &xxo, &yyo);
			}

		/* Set display location to current anchor position */
		else
			{
			(void) anchored_location(ZeroPoint, 0.0, 0.0, &xxo, &yyo);
			}

		/* Set display location (offset by xdoff/ydoff) */
		/*  and offset location for sampled winds       */
		xxo += xdoff;
		yyo += ydoff;
		xx = xxo + xoff;
		yy = yyo + yoff;

		/* Adjust symbol scale for perspective (if required) */
		mscale = markscale;
		if ( perspective_scale(&pscale) ) mscale *= pscale;

		/* Set rotation from latitude or longitude */
		if ( AnchorToMap && rotate_lat )
			rotadj = wind_dir_xy(&BaseMap, flat, flon, 90.0);
		else if ( AnchorToMap && rotate_lon )
			rotadj = wind_dir_xy(&BaseMap, flat, flon,  0.0);
		else
			rotadj = 0.0;

		/* Add default rotation */
		rotadj += rotation;

		/* Display the mark (if requested) ... not offset! */
		if ( !blank(inmark) )
			{
			(void) write_graphics_symbol(inmark, xxo, yyo, mscale, rotadj);
			}

		/* Extract the wind at lat/lon */
		if ( !extract_awind_by_crossref(wcref, &descript, FALSE, 1, pos, clon,
											wdir, wspd, wgst, &wunits) )
			{
			if ( Verbose )
				{
				(void) fprintf(stdout,
					"Cannot extract wind ... %s from %s at %s\n",
					wcref, source, vtime);
				}
			return TRUE;
			}

		/* Set final wind parameters */
		dir = (float) wdir[0];
		(void) convert_value(wunits, (double) wspd[0], units, &dval);
		spd = (float) dval;
		(void) convert_value(wunits, (double) wgst[0], units, &dval);
		gst = (float) dval;

		/* Display graphics outline for sampled wind (if requested) */
		if ( !blank(display_name) )
			{

			/* Set display parameters in default attribute structure */
			(void) strcpy(WindDisplay.units,        units);
			(void) strcpy(WindDisplay.format,       format);
			(void) strcpy(WindDisplay.display_name, display_name);
			(void) strcpy(WindDisplay.display_type, display_type);
			WindDisplay.width_scale  = wthscale;
			WindDisplay.height_scale = hgtscale;
			WindDisplay.x_off        = xx;
			WindDisplay.y_off        = yy;

			/* Determine placement for winds displayed as wind barbs */
			/* Then display the graphics outline                     */
			if ( same(format, FormatWindBarb) )
				{
				(void) set_wind_barb_placement(&WindDisplay, dir, spd, gst);
				(void) GRA_display_attribute_outline(NullCal, WindDisplay,
						flat, flon, 0.0, 0.0, rotation);
				}

			/* Determine placement for winds displayed               */
			/*  as text or symbols from the given wind look up table */
			/* Then display the graphics outline                     */
			else if ( same(format, FormatWindText)
					|| same(format, FormatWindSymbol) )
				{
				(void) set_wind_placement(format, &WindDisplay,
						dir, spd, gst, flat, flon);
				(void) GRA_display_attribute_outline(NullCal, WindDisplay,
						flat, flon, 0.0, 0.0, rotadj);
				}
			}

		/* Then display the sampled wind based on the format */
		if ( same(format, FormatWindBarb) )
			{
			return GRA_display_windbarb(dir, spd, gst, flat, flon,
						xx, yy, wthscale, hgtscale, rotadj);
			}
		else
			{
			return GRA_display_wind(format, dir, spd, gst, flat, flon,
						xx, yy, wthscale, hgtscale, rotadj);
			}
		}

	/* Sample winds for each table site */
	else if ( !blank(table_name) )
		{

		if ( Verbose )
			{
			(void) fprintf(stdout,
					" Table sampling for wind ... %s from %s at %s\n",
					wcref, source, vtime);
			}

		/* Ensure that table locations are NOT anchored to current map */
		/*  ... regardless of the current anchor position!             */
		cur_anchor  = AnchorToMap;
		AnchorToMap = FALSE;

		/* Find the named table */
		cur_table = get_table(table_name);
		if ( IsNull(cur_table) )
			{
			(void) sprintf(err_buf, "Table ... %s ... not yet defined",
					table_name);
			(void) error_report(err_buf);
			}

		/* Set sample locations for all table sites */
		Nump  = cur_table->nsites;
		Ppos  = GETMEM(Ppos,  POINT, Nump);
		Plats = GETMEM(Plats, float, Nump);
		Plons = GETMEM(Plons, float, Nump);
		Pdirs = GETMEM(Pdirs, float, Nump);
		Pspds = GETMEM(Pspds, float, Nump);
		Pgsts = GETMEM(Pgsts, float, Nump);
		for ( isite=0; isite<Nump; isite++ )
			{

			/* Set latitude/longitude for sampling */
			if ( cur_table->usell[isite] )
				{
				flat   = cur_table->flats[isite];
				flon   = cur_table->flons[isite];
				}

			/* Get latitude/longitude for sampling from location look up */
			else
				{
				if ( blank(loc_lookup) )
					(void) error_report("No location look up table");
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"   Matching location ident ... \"%s\"",
							cur_table->idents[isite]);
					(void) fprintf(stdout, "  from look up ... %s\n",
							loc_lookup);
					}
				if ( !match_location_lookup(loc_lookup,
						cur_table->idents[isite], vtime, &loclat, &loclon,
						NullStringPtr) )
					{
					(void) sprintf(err_buf,
							"Error matching \"%s\" in look up ... %s",
							cur_table->idents[isite], loc_lookup);
					(void) error_report(err_buf);
					}
				flat = read_lat(loclat, &status);
				if ( !status )
					(void) error_report("Problem with location look up lat");
				flon = read_lon(loclon, &status);
				if ( !status )
					(void) error_report("Problem with location look up lon");
				}

			/* Set map position for sampling */
			Plats[isite] = flat;
			Plons[isite] = flon;
			(void) ll_to_pos(&BaseMap, flat, flon, Ppos[isite]);
			}

		/* Sample winds at all table sites */
		status = extract_awind_by_crossref(wcref, &descript, FALSE,
					Nump, Ppos, clon, Pdirs, Pspds, Pgsts, &wunits);

		/* Return now if winds could not be sampled */
		if ( !status )
			{

			if ( Verbose )
				{
				(void) fprintf(stdout,
					"Cannot extract wind ... %s from %s at %s\n",
					wcref, source, vtime);
				}

			/* Reset the current anchor */
			AnchorToMap = cur_anchor;

			/* Return TRUE */
			return TRUE;
			}

		/* Display winds for all table sites */
		for ( isite=0; isite<Nump; isite++ )
			{

			/* Check for sample locations off the map */
			if ( Ppos[isite][X] < 0.0 || Ppos[isite][Y] < 0.0
					|| Ppos[isite][X] > BaseMap.definition.xlen
					|| Ppos[isite][Y] > BaseMap.definition.ylen )
				{

				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Sample lat/lon outside map at ... %.1f %.1f\n",
							Plats[isite], Plons[isite]);
					}
				}

			/* Comments for sample locations on the map */
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Sample lat/lon at ... %.1f %.1f\n",
							Plats[isite], Plons[isite]);
					}
				}

			/* Set table location for displaying */
			xxt = cur_table->x_off;
			yyt = cur_table->y_off;
			if ( same(cur_table->type, TableCol) )
				yyt += cur_table->offset[isite];
			else if ( same(cur_table->type, TableRow) )
				xxt += cur_table->offset[isite];
			(void) anchored_location(ZeroPoint, xxt, yyt, &xxo, &yyo);

			/* Set display location (offset by xdoff/ydoff) */
			/*  and offset location for sampled winds       */
			xxo += xdoff;
			yyo += ydoff;
			xx = xxo + xoff;
			yy = yyo + yoff;

			/* Adjust symbol scale for perspective (if required) */
			mscale = markscale;
			if ( perspective_scale(&pscale) ) mscale *= pscale;

			/* Set rotation from latitude or longitude */
			if ( AnchorToMap && rotate_lat )
				rotadj = wind_dir_xy(&BaseMap, Plats[isite], Plons[isite], 90.0);
			else if ( AnchorToMap && rotate_lon )
				rotadj = wind_dir_xy(&BaseMap, Plats[isite], Plons[isite],  0.0);
			else
				rotadj = 0.0;

			/* Add default rotation */
			rotadj += rotation;

			/* Display the mark (if requested) ... not offset! */
			if ( !blank(inmark) )
				{
				(void) write_graphics_symbol(inmark, xxo, yyo, mscale, rotadj);
				}

			/* Set final wind parameters */
			dir = (float) Pdirs[isite];
			(void) convert_value(wunits, (double) Pspds[isite], units, &dval);
			spd = (float) dval;
			(void) convert_value(wunits, (double) Pgsts[isite], units, &dval);
			gst = (float) dval;

			/* Display graphics outline for sampled wind (if requested) */
			if ( !blank(display_name) )
				{

				/* Set display parameters in default attribute structure */
				(void) strcpy(WindDisplay.units,        units);
				(void) strcpy(WindDisplay.format,       format);
				(void) strcpy(WindDisplay.display_name, display_name);
				(void) strcpy(WindDisplay.display_type, display_type);
				WindDisplay.width_scale  = wthscale;
				WindDisplay.height_scale = hgtscale;
				WindDisplay.x_off        = xx;
				WindDisplay.y_off        = yy;

				/* Determine placement for winds displayed as wind barbs */
				/* Then display the graphics outline                     */
				if ( same(format, FormatWindBarb) )
					{
					(void) set_wind_barb_placement(&WindDisplay, dir, spd, gst);
					(void) GRA_display_attribute_outline(NullCal, WindDisplay,
							Plats[isite], Plons[isite], 0.0, 0.0, rotation);
					}

				/* Determine placement for winds displayed               */
				/*  as text or symbols from the given wind look up table */
				/* Then display the graphics outline                     */
				else if ( same(format, FormatWindText)
						|| same(format, FormatWindSymbol) )
					{
					(void) set_wind_placement(format, &WindDisplay,
							dir, spd, gst, Plats[isite], Plons[isite]);
					(void) GRA_display_attribute_outline(NullCal, WindDisplay,
							Plats[isite], Plons[isite], 0.0, 0.0, rotadj);
					}
				}

			/* Then display the sampled wind based on the format */
			if ( same(format, FormatWindBarb) )
				{
				if ( !GRA_display_windbarb(dir, spd, gst,
						Plats[isite], Plons[isite], xx, yy,
						wthscale, hgtscale, rotadj) ) continue;
				}
			else
				{
				if ( !GRA_display_wind(format, dir, spd, gst,
						Plats[isite], Plons[isite], xx, yy,
						wthscale, hgtscale, rotadj) ) continue;
				}
			}

		/* Reset the current anchor */
		AnchorToMap = cur_anchor;

		/* Return TRUE when all table sites have been sampled */
		return TRUE;
		}

	/* Sample winds for each sample grid site */
	else if ( !blank(grid_name) )
		{

		if ( Verbose )
			{
			(void) fprintf(stdout,
					" Grid sampling for wind ... %s from %s at %s\n",
					wcref, source, vtime);
			}

		/* Find the named sample grid */
		cur_grid = get_sample_grid(grid_name);
		if ( IsNull(cur_grid) )
			{
			(void) sprintf(err_buf, "Sample grid ... %s ... not yet defined",
					grid_name);
			(void) error_report(err_buf);
			}

		/* Set sample locations from sample grid */
		Nump  = cur_grid->numx * cur_grid->numy;
		Ppos  = GETMEM(Ppos,  POINT, Nump);
		Plats = GETMEM(Plats, float, Nump);
		Plons = GETMEM(Plons, float, Nump);
		Pdirs = GETMEM(Pdirs, float, Nump);
		Pspds = GETMEM(Pspds, float, Nump);
		Pgsts = GETMEM(Pgsts, float, Nump);
		for ( isite=0, iiy=0; iiy<cur_grid->numy; iiy++ )
			for ( iix=0; iix<cur_grid->numx; iix++, isite++ )
				{

				/* Set latitude/longitude for sampling */
				flat = cur_grid->flats[iiy][iix];
				flon = cur_grid->flons[iiy][iix];

				/* Set map position for sampling */
				Plats[isite] = flat;
				Plons[isite] = flon;
				(void) ll_to_pos(&BaseMap, flat, flon, Ppos[isite]);
				}

		/* Sample winds at all sample grid sites */
		status = extract_awind_by_crossref(wcref, &descript, FALSE,
					Nump, Ppos, clon, Pdirs, Pspds, Pgsts, &wunits);

		/* Return now if winds could not be sampled */
		if ( !status )
			{

			if ( Verbose )
				{
				(void) fprintf(stdout,
					"Cannot extract wind ... %s from %s at %s\n",
					wcref, source, vtime);
				}

			/* Return TRUE */
			return TRUE;
			}

		/* Display winds for all sample grid sites */
		for ( isite=0, iiy=0; iiy<cur_grid->numy; iiy++ )
			for ( iix=0; iix<cur_grid->numx; iix++, isite++ )
				{

				/* Check for sample locations off the map */
				if ( Ppos[isite][X] < 0.0 || Ppos[isite][Y] < 0.0
						|| Ppos[isite][X] > BaseMap.definition.xlen
						|| Ppos[isite][Y] > BaseMap.definition.ylen )
					{

					if ( Verbose )
						{
						(void) fprintf(stdout,
								"  Sample lat/lon outside map at ... %.1f %.1f\n",
								Plats[isite], Plons[isite]);
						}

					/* Continue (if requested) for grid samples off map */
					if ( AnchorToMap ) continue;
					else if ( fit_to_map ) continue;
					}

				/* Comments for sample locations on the map */
				else
					{
					if ( Verbose )
						{
						(void) fprintf(stdout,
								"  Sample lat/lon at ... %.1f %.1f\n",
								Plats[isite], Plons[isite]);
						}
					}

				/* Set sample grid location on the current map */
				if ( AnchorToMap )
					{
					(void) anchored_location(Ppos[isite], 0.0, 0.0, &xxo, &yyo);
					}

				/* Set display location to current anchor position */
				else
					{
					xxs = (float) iix * cur_grid->x_shift;
					yys = (float) iiy * cur_grid->y_shift;
					(void) anchored_location(ZeroPoint, xxs, yys, &xxo, &yyo);
					}

				/* Set display location (offset by xdoff/ydoff) */
				/*  and offset location for sampled winds       */
				xxo += xdoff;
				yyo += ydoff;
				xx = xxo + xoff;
				yy = yyo + yoff;

				/* Adjust symbol scale for perspective (if required) */
				mscale = markscale;
				if ( perspective_scale(&pscale) ) mscale *= pscale;

				/* Set rotation from latitude or longitude */
				if ( AnchorToMap && rotate_lat )
					rotadj = wind_dir_xy(&BaseMap,
											Plats[isite], Plons[isite], 90.0);
				else if ( AnchorToMap && rotate_lon )
					rotadj = wind_dir_xy(&BaseMap,
											Plats[isite], Plons[isite],  0.0);
				else
					rotadj = 0.0;

				/* Add default rotation */
				rotadj += rotation;

				/* Display the mark (if requested) ... not offset! */
				if ( !blank(inmark) )
					{
					(void) write_graphics_symbol(inmark, xxo, yyo, mscale,
							rotadj);
					}

				/* Set final wind parameters */
				dir = (float) Pdirs[isite];
				(void) convert_value(wunits, (double) Pspds[isite],
						units, &dval);
				spd = (float) dval;
				(void) convert_value(wunits, (double) Pgsts[isite],
						units, &dval);
				gst = (float) dval;

				/* Display graphics outline for sampled wind (if requested) */
				if ( !blank(display_name) )
					{

					/* Set display parameters in default attribute structure */
					(void) strcpy(WindDisplay.units,        units);
					(void) strcpy(WindDisplay.format,       format);
					(void) strcpy(WindDisplay.display_name, display_name);
					(void) strcpy(WindDisplay.display_type, display_type);
					WindDisplay.width_scale  = wthscale;
					WindDisplay.height_scale = hgtscale;
					WindDisplay.x_off        = xx;
					WindDisplay.y_off        = yy;

					/* Determine placement for winds displayed as wind barbs */
					/* Then display the graphics outline                     */
					if ( same(format, FormatWindBarb) )
						{
						(void) set_wind_barb_placement(&WindDisplay,
								dir, spd, gst);
						(void) GRA_display_attribute_outline(NullCal,
								WindDisplay, Plats[isite], Plons[isite],
								0.0, 0.0, rotation);
						}

					/* Determine placement for winds displayed               */
					/*  as text or symbols from the given wind look up table */
					/* Then display the graphics outline                     */
					else if ( same(format, FormatWindText)
							|| same(format, FormatWindSymbol) )
						{
						(void) set_wind_placement(format, &WindDisplay,
								dir, spd, gst, Plats[isite], Plons[isite]);
						(void) GRA_display_attribute_outline(NullCal,
								WindDisplay, Plats[isite], Plons[isite],
								0.0, 0.0, rotadj);
						}
					}

				/* Then display the sampled wind based on the format */
				if ( same(format, FormatWindBarb) )
					{
					if ( !GRA_display_windbarb(dir, spd, gst,
							Plats[isite], Plons[isite], xx, yy,
							wthscale, hgtscale, rotadj) ) continue;
					}
				else
					{
					if ( !GRA_display_wind(format, dir, spd, gst,
							Plats[isite], Plons[isite], xx, yy,
							wthscale, hgtscale, rotadj) ) continue;
					}
				}

		/* Return TRUE when all grid locations have been sampled */
		return TRUE;
		}

	/* Sample winds for each sample list site */
	else if ( !blank(list_name) )
		{

		if ( Verbose )
			{
			(void) fprintf(stdout,
					" List sampling for wind ... %s from %s at %s\n",
					wcref, source, vtime);
			}

		/* Find the named sample list */
		cur_list = get_sample_list(list_name);
		if ( IsNull(cur_list) )
			{
			(void) sprintf(err_buf, "Sample list ... %s ... not yet defined",
					list_name);
			(void) error_report(err_buf);
			}

		/* Set sample locations from sample list */
		for ( Nump=0, isite=0; isite<cur_list->num; isite++ )
			{

			/* Set latitude/longitude for sampling */
			if ( cur_list->usell[isite] )
				{
				flat   = cur_list->flats[isite];
				flon   = cur_list->flons[isite];
				}

			/* Get latitude/longitude for sampling from all locations */
			/*  in a location look up table                           */
			else if ( same_ic(cur_list->idents[isite], LocIdentAll)
						|| same_ic(cur_list->idents[isite], LocIdentAllVtime) )
				{
				if ( blank(loc_lookup) )
					(void) error_report("No location look up table");
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"   Sampling all locations from look up ... %s\n",
							loc_lookup);
					}

				/* Set valid time to check */
				if ( same_ic(cur_list->idents[isite], LocIdentAllVtime) )
					vt = vtime;
				else
					vt = FpaCblank;

				/* Set sample locations for all look up table locations */
				iloc = -1;
				while ( (iloc = next_location_lookup_line(loc_lookup, iloc, vt,
						NullStringPtr, &loclat, &loclon, NullStringPtr)) >= 0 )
					{

					/* Set latitude/longitude for sampling */
					flat = read_lat(loclat, &status);
					if ( !status )
						(void) error_report("Problem with location look up lat");
					flon = read_lon(loclon, &status);
					if ( !status )
						(void) error_report("Problem with location look up lon");

					/* Allocate space for this sampling location */
					Nump++;
					Ppos  = GETMEM(Ppos,  POINT,  Nump);
					Plats = GETMEM(Plats, float,  Nump);
					Plons = GETMEM(Plons, float,  Nump);

					/* Set map position for sampling */
					Plats[Nump-1] = flat;
					Plons[Nump-1] = flon;
					(void) ll_to_pos(&BaseMap, flat, flon, Ppos[Nump-1]);
					}

				/* Go on to next sample location in sample list */
				continue;
				}

			/* Get latitude/longitude for sampling from location look up */
			else
				{
				if ( blank(loc_lookup) )
					(void) error_report("No location look up table");
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"   Matching location ident ... \"%s\"",
							cur_list->idents[isite]);
					(void) fprintf(stdout, "  from look up ... %s\n",
							loc_lookup);
					}
				if ( !match_location_lookup(loc_lookup,
						cur_list->idents[isite], vtime, &loclat, &loclon,
						NullStringPtr) )
					{
					(void) sprintf(err_buf,
							"Error matching \"%s\" in look up ... %s",
							cur_list->idents[isite], loc_lookup);
					(void) error_report(err_buf);
					}
				flat = read_lat(loclat, &status);
				if ( !status )
					(void) error_report("Problem with location look up lat");
				flon = read_lon(loclon, &status);
				if ( !status )
					(void) error_report("Problem with location look up lon");
				}

			/* Allocate space for this sampling location */
			Nump++;
			Ppos  = GETMEM(Ppos,  POINT,  Nump);
			Plats = GETMEM(Plats, float,  Nump);
			Plons = GETMEM(Plons, float,  Nump);

			/* Set map position for sampling */
			Plats[Nump-1] = flat;
			Plons[Nump-1] = flon;
			(void) ll_to_pos(&BaseMap, flat, flon, Ppos[Nump-1]);
			}

		/* Allocate space for wind samples */
		Pdirs = GETMEM(Pdirs, float, Nump);
		Pspds = GETMEM(Pspds, float, Nump);
		Pgsts = GETMEM(Pgsts, float, Nump);

		/* Sample winds at all sample list sites */
		status = extract_awind_by_crossref(wcref, &descript, FALSE,
					Nump, Ppos, clon, Pdirs, Pspds, Pgsts, &wunits);

		/* Return now if winds could not be sampled */
		if ( !status )
			{

			if ( Verbose )
				{
				(void) fprintf(stdout,
					"Cannot extract wind ... %s from %s at %s\n",
					wcref, source, vtime);
				}

			/* Return TRUE */
			return TRUE;
			}

		/* Initialize offsets for sample list sites */
		iil = 0;
		xxs = 0.0;
		yys = 0.0;

		/* Display winds for all sample list sites */
		for ( iloc=0; iloc<Nump; iloc++ )
			{

			/* Check for sample locations off the map */
			if ( Ppos[iloc][X] < 0.0 || Ppos[iloc][Y] < 0.0
					|| Ppos[iloc][X] > BaseMap.definition.xlen
					|| Ppos[iloc][Y] > BaseMap.definition.ylen )
				{

				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Sample lat/lon outside map at ... %.1f %.1f\n",
							Plats[iloc], Plons[iloc]);
					}

				/* Continue (if requested) for list samples off map */
				if ( AnchorToMap ) continue;
				else if ( fit_to_map ) continue;
				}

			/* Comments for sample locations on the map */
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Sample lat/lon at ... %.1f %.1f\n",
							Plats[iloc], Plons[iloc]);
					}
				}

			/* Set sample list location on the current map */
			if ( AnchorToMap )
				{
				(void) anchored_location(Ppos[iloc], 0.0, 0.0, &xxo, &yyo);
				}

			/* Set display location to current anchor position  */
			/*  and set offset for each successive site in list */
			else
				{
				(void) anchored_location(ZeroPoint, xxs, yys, &xxo, &yyo);
				iil++;
				if ( cur_list->x_wrap > 1 && iil % cur_list->x_wrap == 0 )
					{
					xxs  = 0.0;
					yys += cur_list->y_shift;
					}
				else if ( cur_list->x_wrap > 1 )
					{
					xxs += cur_list->x_shift;
					}
				else if ( cur_list->y_wrap > 1 && iil % cur_list->y_wrap == 0 )
					{
					xxs += cur_list->x_shift;
					yys  = 0.0;
					}
				else if ( cur_list->y_wrap > 1 )
					{
					yys += cur_list->y_shift;
					}
				else
					{
					xxs += cur_list->x_shift;
					yys += cur_list->y_shift;
					}
				}

			/* Set display location (offset by xdoff/ydoff) */
			/*  and offset location for sampled winds       */
			xxo += xdoff;
			yyo += ydoff;
			xx = xxo + xoff;
			yy = yyo + yoff;

			/* Adjust symbol scale for perspective (if required) */
			mscale = markscale;
			if ( perspective_scale(&pscale) ) mscale *= pscale;

			/* Set rotation from latitude or longitude */
			if ( AnchorToMap && rotate_lat )
				rotadj = wind_dir_xy(&BaseMap, Plats[isite], Plons[isite], 90.0);
			else if ( AnchorToMap && rotate_lon )
				rotadj = wind_dir_xy(&BaseMap, Plats[isite], Plons[isite],  0.0);
			else
				rotadj = 0.0;

			/* Add default rotation */
			rotadj += rotation;

			/* Display the mark (if requested) ... not offset! */
			if ( !blank(inmark) )
				{
				(void) write_graphics_symbol(inmark, xxo, yyo, mscale, rotadj);
				}

			/* Set final wind parameters */
			dir = (float) Pdirs[iloc];
			(void) convert_value(wunits, (double) Pspds[iloc],
					units, &dval);
			spd = (float) dval;
			(void) convert_value(wunits, (double) Pgsts[iloc],
					units, &dval);
			gst = (float) dval;

			/* Display graphics outline for sampled wind (if requested) */
			if ( !blank(display_name) )
				{

				/* Set display parameters in default attribute structure */
				(void) strcpy(WindDisplay.units,        units);
				(void) strcpy(WindDisplay.format,       format);
				(void) strcpy(WindDisplay.display_name, display_name);
				(void) strcpy(WindDisplay.display_type, display_type);
				WindDisplay.width_scale  = wthscale;
				WindDisplay.height_scale = hgtscale;
				WindDisplay.x_off        = xx;
				WindDisplay.y_off        = yy;

				/* Determine placement for winds displayed as wind barbs */
				/* Then display the graphics outline                     */
				if ( same(format, FormatWindBarb) )
					{
					(void) set_wind_barb_placement(&WindDisplay,
							dir, spd, gst);
					(void) GRA_display_attribute_outline(NullCal, WindDisplay,
							Plats[isite], Plons[isite], 0.0, 0.0, rotation);
					}

				/* Determine placement for winds displayed               */
				/*  as text or symbols from the given wind look up table */
				/* Then display the graphics outline                     */
				else if ( same(format, FormatWindText)
						|| same(format, FormatWindSymbol) )
					{
					(void) set_wind_placement(format, &WindDisplay,
							dir, spd, gst, Plats[iloc], Plons[iloc]);
					(void) GRA_display_attribute_outline(NullCal, WindDisplay,
							Plats[isite], Plons[isite], 0.0, 0.0, rotadj);
					}
				}

			/* Then display the sampled wind based on the format */
			if ( same(format, FormatWindBarb) )
				{
				if ( !GRA_display_windbarb(dir, spd, gst,
						Plats[iloc], Plons[iloc], xx, yy,
						wthscale, hgtscale, rotadj) ) continue;
				}
			else
				{
				if ( !GRA_display_wind(format, dir, spd, gst,
						Plats[iloc], Plons[iloc], xx, yy,
						wthscale, hgtscale, rotadj) ) continue;
				}
			}

		/* Return TRUE when all list locations have been sampled */
		return TRUE;
		}

	/* Sample winds for each location in cross section */
	else if ( !blank(xsection_name) )
		{

		if ( Verbose )
			{
			(void) fprintf(stdout,
					" Cross section sampling for wind ... %s from %s\n",
					wcref, source);
			}

		/* Ensure that cross section locations are NOT anchored to current */
		/*  map ... regardless of the current anchor position!             */
		cur_anchor  = AnchorToMap;
		AnchorToMap = FALSE;

		/* Find the named cross section */
		cur_xsect = get_cross_section(xsection_name);
		if ( IsNull(cur_xsect) )
			{
			(void) sprintf(err_buf, "Cross Section ... %s ... not yet defined",
					xsection_name);
			(void) error_report(err_buf);
			}

		/* Get horizontal axis parameters */
		haxis = cross_section_horizontal_axis(cur_xsect, loc_lookup, ltype,
											num_xloc, xsect_locs, NullDouble);
		if ( IsNull(haxis) )
			{
			(void) sprintf(err_buf,
					"Error in cross section horizontal look up ... %s",
					loc_lookup);
			(void) error_report(err_buf);
			}

		/* Get vertical axis parameters */
		vaxis = cross_section_vertical_axis(cur_xsect, ver_lookup, NullDouble);
		if ( IsNull(vaxis) )
			{
			(void) sprintf(err_buf,
					"Error in cross section vertical look up ... %s",
					ver_lookup);
			(void) error_report(err_buf);
			}

		/* Sample fields for all cross section locations */
		for ( iloc=0; iloc<haxis->num; iloc++ )
			{

			/* Set latitude/longitude for sampling */
			flat = haxis->flats[iloc];
			flon = haxis->flons[iloc];

			/* Set cross section route speed/direction for tail/cross winds */
			(void) convert_value(FpaCmksUnits, (double) haxis->spds[iloc],
									units, &dval);
			fspd = (float) dval;
			fdir = haxis->dirs[iloc];

			/* Set map position for sampling */
			(void) ll_to_pos(&BaseMap, flat, flon, pos[0]);

			/* Check for sample locations off the map */
			if ( pos[0][X] < 0.0 || pos[0][Y] < 0.0
					|| pos[0][X] > BaseMap.definition.xlen
					|| pos[0][Y] > BaseMap.definition.ylen )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Sample lat/lon outside map at ... %.1f %.1f\n",
							flat, flon);
					}
				}

			/* Comments for sample locations on the map */
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
							"  Sample lat/lon at ... %.1f %.1f\n",
							flat, flon);
					}
				}

			/* Reset field descriptor for cross section valid time */
			if ( !set_fld_descript(&descript,
										FpaF_VALID_TIME, haxis->vtimes[iloc],
										FpaF_END_OF_LIST) )
				{
				(void) sprintf(err_buf,
						"Error re-setting valid time ... %s\n",
						haxis->vtimes[iloc]);
				(void) error_report(err_buf);
				}

			/* Set vertical location of parameter */
			yloc = set_vertical_position(&descript, vaxis, pos[0],
					ver_element, ver_level, ver_equation, ver_units, FpaCblank,
					CatCascadeAnd, NullPtr(CATATTRIB *), 0, ver_attrib,
					ver_attrib_upper, ver_attrib_lower, 0.0, &status);
			if ( !status ) continue;

			/* Set cross section location for displaying */
			xxc = cur_xsect->x_off + (cur_xsect->width  * haxis->locs[iloc]);
			yyc = cur_xsect->y_off + (cur_xsect->height * yloc);
			(void) anchored_location(ZeroPoint, xxc, yyc, &xxo, &yyo);

			/* Set display location (offset by xdoff/ydoff) */
			/*  and offset location for sampled winds       */
			xxo += xdoff;
			yyo += ydoff;
			xx = xxo + xoff;
			yy = yyo + yoff;

			/* Adjust symbol scale for perspective (if required) */
			mscale = markscale;
			if ( perspective_scale(&pscale) ) mscale *= pscale;

			/* Set rotation from latitude or longitude */
			if ( AnchorToMap && rotate_lat )
				rotadj = wind_dir_xy(&BaseMap, flat, flon, 90.0);
			else if ( AnchorToMap && rotate_lon )
				rotadj = wind_dir_xy(&BaseMap, flat, flon,  0.0);
			else
				rotadj = 0.0;

			/* Add default rotation */
			rotadj += rotation;

			/* Display the mark (if requested) ... not offset! */
			if ( !blank(inmark) )
				{
				(void) write_graphics_symbol(inmark, xxo, yyo, mscale, rotadj);
				}

			/* Extract the wind at cross section location */
			if ( !extract_awind_by_crossref(wcref, &descript, FALSE, 1, pos,
											clon, wdir, wspd, wgst, &wunits) )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
						"Cannot extract wind ... %s from %s at %s\n",
						wcref, source, haxis->vtimes[iloc]);
					}
				continue;
				}

			/* Set final wind parameters */
			dir = (float) wdir[0];
			(void) convert_value(wunits, (double) wspd[0], units, &dval);
			spd = (float) dval;
			(void) convert_value(wunits, (double) wgst[0], units, &dval);
			gst = (float) dval;

			/* Determine component of tail wind (if required) */
			if ( same(format, FormatTailWindBarb)
					|| same(format, FormatTailWindText)
					|| same(format, FormatTailWindSymbol) )
				{

				/* Determine component of tail wind */
				tspd = spd * fpa_cosdeg((double) (dir - fdir));
				tdir = (tspd > 0.0) ? fdir : fdir + 180.0;
				tdir = fmod(tdir, 360.0);
				tspd = fabs((double) tspd);

				if ( Verbose )
					{
					(void) fprintf(stdout,
						"Cross section location at %.1f %.1f ... fspd/fdir: %.1f %.1f\n",
						flat, flon, fspd, fdir);
					(void) fprintf(stdout,
						" Tail wind component from spd/dir %.1f %.1f ... tspd/tdir: %.1f %.1f\n",
						spd, dir, tspd, tdir);
					}

				/* Reset final wind parameters */
				dir = tdir;
				spd = (fspd > 0.0) ? tspd : 0.0;
				gst = (fspd > 0.0) ? tspd : 0.0;
				}

			/* Determine component of cross track wind (if required) */
			if ( same(format, FormatCrossWindBarb)
					|| same(format, FormatCrossWindText)
					|| same(format, FormatCrossWindSymbol) )
				{

				/* Determine component of cross track wind */
				tspd = spd * fpa_sindeg((double) (dir - fdir));
				tdir = (tspd > 0.0) ? fdir + 90.0 : fdir - 90.0;
				tdir = fmod(tdir, 360.0);
				if ( tdir < 0.0 ) tdir += 360.0;
				tspd = fabs((double) tspd);

				if ( Verbose )
					{
					(void) fprintf(stdout,
						"Cross section location at %.1f %.1f ... fspd/fdir: %.1f %.1f\n",
						flat, flon, fspd, fdir);
					(void) fprintf(stdout,
						" Cross track wind component from spd/dir %.1f %.1f ... tspd/tdir: %.1f %.1f\n",
						spd, dir, tspd, tdir);
					}

				/* Reset final wind parameters */
				dir = tdir;
				spd = (fspd > 0.0) ? tspd : 0.0;
				gst = (fspd > 0.0) ? tspd : 0.0;
				}

			/* Display graphics outline for sampled wind (if requested) */
			if ( !blank(display_name) )
				{

				/* Set display parameters in default attribute structure */
				(void) strcpy(WindDisplay.units,        units);
				(void) strcpy(WindDisplay.format,       format);
				(void) strcpy(WindDisplay.display_name, display_name);
				(void) strcpy(WindDisplay.display_type, display_type);
				WindDisplay.width_scale  = wthscale;
				WindDisplay.height_scale = hgtscale;
				WindDisplay.x_off        = xx;
				WindDisplay.y_off        = yy;

				/* Determine placement for winds displayed as wind barbs */
				/* Then display the graphics outline                     */
				if ( same(format, FormatWindBarb)
						|| same(format, FormatTailWindBarb)
						|| same(format, FormatCrossWindBarb) )
					{
					(void) set_wind_barb_placement(&WindDisplay, dir, spd, gst);
					(void) GRA_display_attribute_outline(NullCal, WindDisplay,
							flat, flon, 0.0, 0.0, rotation);
					}

				/* Determine placement for winds displayed               */
				/*  as text or symbols from the given wind look up table */
				/* Then display the graphics outline                     */
				else if ( same(format, FormatWindText)
						|| same(format, FormatWindSymbol)
						|| same(format, FormatTailWindText)
						|| same(format, FormatTailWindSymbol)
						|| same(format, FormatCrossWindText)
						|| same(format, FormatCrossWindSymbol) )
					{
					(void) set_wind_placement(format, &WindDisplay,
							dir, spd, gst, flat, flon);
					(void) GRA_display_attribute_outline(NullCal, WindDisplay,
							flat, flon, 0.0, 0.0, rotadj);
					}
				}

			/* Then display the sampled wind based on the format */
			if ( same(format, FormatWindBarb)
					|| same(format, FormatTailWindBarb)
					|| same(format, FormatCrossWindBarb) )
				{
				if ( !GRA_display_windbarb(dir, spd, gst, flat, flon,
							xx, yy, wthscale, hgtscale, rotadj) ) continue;
				}
			else if ( same(format, FormatWindText)
					|| same(format, FormatWindSymbol)
					|| same(format, FormatTailWindText)
					|| same(format, FormatTailWindSymbol)
					|| same(format, FormatCrossWindText)
					|| same(format, FormatCrossWindSymbol) )
				{
				if ( !GRA_display_wind(format, dir, spd, gst, flat, flon,
							xx, yy, wthscale, hgtscale, rotadj) ) continue;
				}
			}

		/* Reset the current anchor */
		AnchorToMap = cur_anchor;

		/* Return TRUE when all cross section locations have been sampled */
		return TRUE;
		}

	/* Return FALSE if no lat/lon or table or grid or cross section */
	(void) fprintf(stderr, " No lat/lon or table or grid or cross section\n");
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES                                          *
*                                                                      *
*     All the routines after this point are available only within      *
*     this source file.                                                *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*    p a r s e _ c t a b l e _ n a m e                                 *
*    r e v e r s e _ i m a g e _ l i s t                               *
*                                                                      *
***********************************************************************/

static STRING parse_ctable_name
(
 STRING ident		/* directory / colour table identifier */
)

{
	const   STRING default_dir = "ctables";
	const   STRING delim = ":";
	size_t	nlen, ndelim, ndir;
	char    dir[GPGLong], file[GPGLong];

	/* Static buffers for full file path */
	static char	dir_path[GPGLong];

	/* Return NULL if no colour table identifier passed */
	if ( blank(ident) ) return NullString;

	/* Identify length of directory name & colour table name (if one is found) */
	nlen	= strlen(ident);
	ndelim  = strcspn(ident, delim);
	ndir    = ndelim;

	/* Return ident as the file name if no delimiter is found */
	if ( ndelim >= nlen )
	{
		(void) safe_strcpy(dir_path, get_path(default_dir, ident));
		return dir_path;
	}

	/* Split the dir:file identifier into directory and file names */
	(void) strncpy(dir, ident, ndir);
	dir[ndir] = '\0';
	(void) safe_strcpy(file, ident+ndelim +1);

	/* Return the file path */
	(void) safe_strcpy(dir_path, get_path(dir, file));
	return dir_path;
}

static void reverse_image_list
(
 Image *list,
 int   size
)

{
	int nn;
	Image tmp;

	for(nn = 0; nn < size/2; nn++)
	{
		tmp                 = list[nn];
		list[nn]            = list[size - nn - 1];
		list[size - nn - 1] = tmp;
	}
}

/***********************************************************************
*                                                                      *
*    c h e c k _ s a m p l e _ c a s e s                               *
*                                                                      *
***********************************************************************/

static	STRING		check_sample_cases

	(
	POINT		pos,			/* Current position */
	STRING		vtime,			/* Current valid time */
	STRING		look_up,		/* Default look up table */
	SPCASE		*list_case,		/* Structure for special case look up tables */
	int			num_list		/* Number of special cases */
	)

	{
	int			year, jday, hour, minute, seconds, ii;
	float		clon, flat, flon, azimuth, zenith;
	STRING		vtr;
	char		err_buf[GPGLong];

	/* Set centre longitude from current map projection */
	(void) grid_center(&BaseMap, NullPointPtr, NullFloat, &clon);

	/* Convert local times to GMT (if required) */
	vtr = local_to_gmt(vtime, clon);
	if ( IsNull(vtr) )
		{
		(void) sprintf(err_buf, "Error reading valid time ... %s", vtime);
		(void) error_report(err_buf);
		}

	/* Read the current valid time */
	(void) parse_tstamp(vtr, &year, &jday, &hour, &minute,
											NullLogicalPtr, NullLogicalPtr);

	/* Set the time offset and sample location */
	seconds  = hour * 3600;
	seconds += minute * 60;
	(void) pos_to_ll(&BaseMap, pos, &flat, &flon);

	/* Set the solar zenith angle for the sample location */
	(void) sun_angle(year, jday, seconds, flat, flon, &azimuth, &zenith);

	/* Check through list of cases to find the one to check */
	for ( ii=0; ii<num_list; ii++ )
		{

		/* Check for day time look up table */
		if ( same_ic(list_case[ii].spcase, CaseDay) )
			{
			if ( zenith <= 85.0 ) return list_case[ii].lookup;
			}

		/* Check for night time look up table */
		else if ( same_ic(list_case[ii].spcase, CaseNight) )
			{
			if ( zenith >= 95.0 ) return list_case[ii].lookup;
			}

		/* Check for dusk look up table */
		else if ( same_ic(list_case[ii].spcase, CaseDusk) )
			{
			if ( zenith > 85.0 && zenith < 95.0 ) return list_case[ii].lookup;
			}

		/* Check for northern hemisphere */
		else if ( same_ic(list_case[ii].spcase, CaseNorthernHem) )
			{
			if ( flat >= 0.0 ) return list_case[ii].lookup;
			}

		/* Check for southern hemisphere */
		else if ( same_ic(list_case[ii].spcase, CaseSouthernHem) )
			{
			if ( flat < 0.0 ) return list_case[ii].lookup;
			}
		}

	/* Return default look up table if matching case not found */
	return look_up;
	}

/***********************************************************************
*                                                                      *
*    s e t _ v e r t i c a l _ p o s i t i o n                         *
*    s e t _ v e r t i c a l _ p o s i t i o n _ d a t a _ f i l e     *
*    x s e c t i o n _ l i n e _ c o o r d i n a t e s                 *
*    x s e c t i o n _ b o u n d a r y _ c o o r d i n a t e s         *
*                                                                      *
***********************************************************************/

static	float		set_vertical_position

	(
	FLD_DESCRIPT	*fdesc,				/* Field descriptor */
	XSECT_VER_AXIS	*vaxis,
	POINT			ver_pos,
	STRING			ver_element,
	STRING			ver_level,
	STRING			ver_equation,
	STRING			ver_units,
	STRING			ver_field_type,
	STRING			ver_cat_cascade,	/* And/Or for multiple CATATTRIB structs */
	CATATTRIB		*ver_cat_attrib,	/* Structure containing categories */
	int				ver_num_catatt,		/* Number of categories */
	STRING			ver_attrib,
	STRING			ver_attrib_upper,
	STRING			ver_attrib_lower,
	float			proximity,			/* Proximity to location (in km) */
	LOGICAL			*status
	)

	{
	int				fkind, iarea, isub, iseg;
	float			fdist, fbear, flen, xdir, xspd;
	double			dval[1], dloc[1], dvalu, dvall;
	STRING			value, valueu, valuel;
	LOGICAL			inside, right;
	HAND			sense;
	FLD_DESCRIPT	descript;
	POINT			pos[1] = { ZERO_POINT };
	VLIST			*vlist;
	SET				areas, curves, spots, chains, copyset;
	AREA			area, carea;
	SUBAREA			subarea;
	CURVE			curve;
	SPOT			spot;
	LCHAIN			chain;
	POINT			fpos, spos, epos;
	CAL				cal, copycal = NullCal;
	char			err_buf[GPGLong];

	FpaConfigFieldStruct	*fdef;
	FpaConfigUnitStruct		*udef;

	/* Initialize return parameter */
	if ( status ) *status = TRUE;

	/* Return immediately for "special" vertical attributes */
	if ( same(ver_attrib, VerAttribTop) )
		{
		return vaxis->locs[vaxis->num-1];
		}
	else if ( same(ver_attrib, VerAttribBase) )
		{
		return vaxis->locs[0];
		}
	else if ( same(ver_attrib_upper, VerAttribTop)
				&& same(ver_attrib_lower, VerAttribBase) )
		{
		return (vaxis->locs[vaxis->num-1] + vaxis->locs[0]) / 2.0;
		}

	/* Return immediately if no vertical field defined */
	if ( ( blank(ver_element) || blank(ver_level) ) && blank(ver_equation) )
		{
		return 0.0;
		}

	/* Make a copy of the field descriptor */
	(void) copy_fld_descript(&descript, fdesc);

	/* Modify the field descriptor (if required) */
	if ( !blank(ver_element) )
		{
		if ( !set_fld_descript(&descript, FpaF_ELEMENT_NAME, ver_element,
											FpaF_END_OF_LIST) )
			{
			(void) sprintf(err_buf, " Error re-setting element ... %s\n",
					ver_element);
			(void) error_report(err_buf);
			}
		}
	if ( !blank(ver_level) )
		{
		if ( !set_fld_descript(&descript, FpaF_LEVEL_NAME, ver_level,
											FpaF_END_OF_LIST) )
			{
			(void) sprintf(err_buf, " Error re-setting level ... %s\n",
					ver_level);
			(void) error_report(err_buf);
			}
		}

	/* Set the sample location */
	(void) copy_point(pos[0], ver_pos);

	/* Set the field type for sampling by equation */
	if ( !blank(ver_equation) )
		{
		fkind = FpaC_CONTINUOUS;
		}

	/* Set the field type from the element and level */
	else
		{
		fdef = get_field_info(descript.edef->name, descript.ldef->name);
		if ( IsNull(fdef) )
			{
			(void) sprintf(err_buf,
					"Unrecognized element ... %s  or level ... %s",
					descript.edef->name, descript.ldef->name);
			(void) error_report(err_buf);
			}
		fkind = fdef->element->fld_type;

		/* Reset the field type (if requested) */
		if ( !blank(ver_field_type) )
			{
			fkind = field_data_type(ver_field_type);
			if ( fkind == FpaCnoMacro )
				{
				(void) sprintf(err_buf,
						" Unrecognized vertical field type ... %s\n",
						ver_field_type);
				(void) error_report(err_buf);
				}
			}

		/* Set the field type for retrieving the features */
		(void) set_fld_descript(&descript, FpaF_FIELD_MACRO, fkind,
											FpaF_END_OF_LIST);

		/* Check that units match with field information */
		switch ( fkind )
			{

			/* Must match field units for continuous/vector fields */
			case FpaC_CONTINUOUS:
			case FpaC_VECTOR:
				udef = fdef->element->elem_io->units;
				if ( NotNull(udef)
						&& !convert_value(udef->name, 0.0,
											ver_units, NullDouble) )
					{
					(void) sprintf(err_buf,
							"Incorrect vertical units: %s  for field: %s %s with units %s",
							ver_units, descript.edef->name,
							descript.ldef->name, udef->name);
					(void) error_report(err_buf);
					}
				break;
			}
		}

	/* Replace "default" attributes (if required) */
	if ( !blank(ver_attrib) )
		ver_attrib = replace_default_attribute(fkind, ver_attrib);
	if ( !blank(ver_attrib_upper) )
		ver_attrib_upper = replace_default_attribute(fkind, ver_attrib_upper);
	if ( !blank(ver_attrib_lower) )
		ver_attrib_lower = replace_default_attribute(fkind, ver_attrib_lower);

	/* Initialize proximity parameters */
	fdist = 0.0;
	fbear = 0.0;
	flen  = 0.0;

	/* Extract vertical value depending on field type */
	switch ( fkind )
		{

		/* Extract vertical value for continuous type fields */
		case FpaC_CONTINUOUS:

			/* Extract vertical value by equation */
			if ( !blank(ver_equation) )
				{
				vlist = retrieve_vlist_by_equation(&descript, 1, pos,
											FpaCmksUnits, ver_equation);
				if ( IsNull(vlist) )
					{
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot calculate equation ... %s from %s %s at %s\n",
							ver_equation, descript.sdef->name,
							descript.subdef->name, descript.vtime);
						}
					if ( status ) *status = FALSE;
					return 0.0;
					}
				}

			/* Extract vertical value from field */
			else if ( same_ic(ver_attrib, AttribEvalSpval) )
				{
				vlist = retrieve_vlist(&descript, 1, pos);
				if ( IsNull(vlist) )
					{
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot extract field ... %s %s from %s %s at %s\n",
							descript.edef->name, descript.ldef->name,
							descript.sdef->name, descript.subdef->name,
							descript.vtime);
						}
					if ( status ) *status = FALSE;
					return 0.0;
					}
				}

			/* Error return for incorrect vertical attribute */
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
						"Cannot extract attribute ... %s", ver_attrib);
					(void) fprintf(stdout,
						"  from field ... %s %s from %s %s at %s\n",
						descript.edef->name, descript.ldef->name,
						descript.sdef->name, descript.subdef->name,
						descript.vtime);
					}
				if ( status ) *status = FALSE;
				return 0.0;
				}

			/* Convert value to required units */
			(void) convert_value(FpaCmksUnits, (double) vlist->val[0],
					ver_units, &dval[0]);
			(void) free_vlist(vlist);
			FREEMEM(vlist);
			break;

		/* Extract vertical value for vector type fields */
		case FpaC_VECTOR:

			/* Extract vertical value from field */
			if ( same_ic(ver_attrib, AttribEvalSpval) )
				{
				vlist = retrieve_vlist_component(&descript, M_Comp, 1, pos);
				if ( IsNull(vlist) )
					{
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot extract field ... %s %s from %s %s at %s\n",
							descript.edef->name, descript.ldef->name,
							descript.sdef->name, descript.subdef->name,
							descript.vtime);
						}
					if ( status ) *status = FALSE;
					return 0.0;
					}
				}

			/* Error return for incorrect vertical attribute */
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
						"Cannot extract attribute ... %s", ver_attrib);
					(void) fprintf(stdout,
						"  from field ... %s %s from %s %s at %s\n",
						descript.edef->name, descript.ldef->name,
						descript.sdef->name, descript.subdef->name,
						descript.vtime);
					}
				if ( status ) *status = FALSE;
				return 0.0;
				}

			/* Convert value to required units */
			(void) convert_value(FpaCmksUnits, (double) vlist->val[0],
					ver_units, &dval[0]);
			(void) free_vlist(vlist);
			FREEMEM(vlist);
			break;

		/* Extract vertical value for discrete type fields */
		case FpaC_DISCRETE:

			/* Find the field containing the discrete areas */
			areas = retrieve_areaset(&descript);
			if ( IsNull(areas) )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
						"Cannot extract field ... %s %s from %s %s at %s\n",
						descript.edef->name, descript.ldef->name,
						descript.sdef->name, descript.subdef->name,
						descript.vtime);
					}
				if ( status ) *status = FALSE;
				return 0.0;
				}

			/* Check areas based on proximity */
			if ( proximity > 0.0 )
				{

				/* Make a copy of the area or subareas in the area */
				copyset = create_set("area");
				for ( iarea=0; iarea<areas->num; iarea++ )
					{
					area = (AREA) areas->list[iarea];

					/* Ensure that area is within proximity */
					(void) area_test_point(area, pos[0], NULL, fpos,
											NULL, NULL, NULL, &inside);
					if ( !inside )
						{
						fdist  = great_circle_distance(&BaseMap, pos[0], fpos);
						fdist /= 1000.0;
						if ( fdist > proximity ) continue;
						}

					/* Copy the area if no subareas */
					if ( area->numdiv == 0)
						{
						carea = copy_area(area, FALSE);
						if ( NotNull(carea) )
							{
							(void) add_item_to_set(copyset, (ITEM) carea);
							}
						}

					/* Create an area from each subarea */
					else
						{
						for ( isub=0; isub<=area->numdiv; isub++ )
							{
							subarea = area->subareas[isub];
							carea = area_from_subarea(subarea);
							if ( NotNull(carea) )
								{
								(void) add_item_to_set(copyset, (ITEM) carea);
								}
							}
						}
					}

				/* Search for closest area within proximity */
				/*  that matches the required attributes    */
				while ( copyset->num > 0 )
					{

					/* Find an enclosing area */
					area = enclosing_area(copyset, pos[0],
										PickFirst, NullFloat, NullChar);

					/* Set sampling point (if inside) */
					if ( NotNull(area) )
						{
						(void) copy_point(fpos, pos[0]);
						inside = TRUE;
						}

					/* Find the closest area within proximity */
					else
						{
						area   = closest_area(copyset, pos[0],
										NULL, fpos, NULL, NULL, NULL);
						inside = FALSE;
						fdist  = great_circle_distance(&BaseMap, pos[0], fpos);
						fdist /= 1000.0;
						if ( fdist > proximity ) break;

						/* Determine bearing from location to feature */
						fbear  = great_circle_bearing(&BaseMap, pos[0], fpos);
						}

					/* Check for matching category attributes */
					if ( match_category_attributes(area->attrib,
							ver_cat_cascade, ver_cat_attrib, ver_num_catatt) )
						{
						copycal = CAL_duplicate(area->attrib);
						break;
						}

					/* Category attributes did not match     */
					/*  ... so remove this area from the set */
					(void) remove_item_from_set(copyset, (ITEM) area);
					}

				/* Destroy what is left of the copy */
				(void) destroy_set(copyset);

				/* Ensure that matching attributes were found */
				if ( IsNull(copycal) )
					{
					(void) destroy_set(areas);
					if ( status ) *status = FALSE;
					return 0.0;
					}
				else
					cal = copycal;
				}

			/* Check areas directly */
			else
				{

				/* Find area enclosing point                       */
				/* Note that this should return a background value */
				/*  with no subarea if no enclosing area is found  */
				if ( !eval_areaset(areas, pos[0], PickFirst, &subarea, &cal) )
					{
					(void) error_report("Cannot find areas in discrete data!");
					}

				/* Check for matching category attributes */
				if ( !match_category_attributes(cal, ver_cat_cascade,
											ver_cat_attrib, ver_num_catatt) )
					{
					(void) destroy_set(areas);
					if ( status ) *status = FALSE;
					return 0.0;
					}
				}

			/* Extract vertical attribute from attribute structure */
			if ( !blank(ver_attrib) )
				{

				/* Extract a "magic" vertical attribute ... converted to real */
				/* Note that blank attributes return a value of 0!            */
				if ( magic_is_attribute(ver_attrib) )
					{
					value = magic_get_attribute(ver_attrib, FpaCblank,
								descript.vtime, FpaCblank, FpaCblank, FpaCblank,
								fpos, fdist, fbear, flen, DefSegDir, DefSegSpd,
								ver_units, FormatDirect, FpaCblank);
					if ( blank(value) && status ) *status = FALSE;
					dval[0] = atof(value);
					}

				/* Extract the vertical attribute ... converted to real */
				/* Note that blank attributes return a value of 0!      */
				else if ( CAL_has_attribute(cal, ver_attrib) )
					{
					value = CAL_get_attribute(cal, ver_attrib);
					if ( blank(value) && status ) *status = FALSE;
					dval[0] = atof(value);
					}

				/* Error for missing vertical attribute */
				else
					{
					(void) destroy_set(areas);
					if ( NotNull(copycal) ) (void) CAL_destroy(copycal);
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot extract attribute ... %s", ver_attrib);
						(void) fprintf(stdout,
							"  from field ... %s %s from %s %s at %s\n",
							descript.edef->name, descript.ldef->name,
							descript.sdef->name, descript.subdef->name,
							descript.vtime);
						}
					if ( status ) *status = FALSE;
					return 0.0;
					}
				}

			/* Extract vertical attributes from attribute structure */
			else if ( !blank(ver_attrib_upper) && !blank(ver_attrib_lower) )
				{

				/* Set upper vertical attribute for "special" case */
				if ( same(ver_attrib_upper, VerAttribTop) )
					{
					dvalu = vaxis->locs[vaxis->num-1];
					}

				/* Extract a "magic" upper vertical attribute      */
				/*  ... converted to real                          */
				/* Note that blank attributes return a value of 0! */
				else if ( magic_is_attribute(ver_attrib_upper) )
					{
					valueu = magic_get_attribute(ver_attrib_upper, FpaCblank,
								descript.vtime, FpaCblank, FpaCblank, FpaCblank,
								fpos, fdist, fbear, flen, DefSegDir, DefSegSpd,
								ver_units, FormatDirect, FpaCblank);
					if ( blank(valueu) && status ) *status = FALSE;
					dvalu = atof(valueu);
					}

				/* Extract the upper vertical attribute ... converted to real */
				/* Note that blank attributes return a value of 0!            */
				else if ( CAL_has_attribute(cal, ver_attrib_upper) )
					{
					valueu = CAL_get_attribute(cal, ver_attrib_upper);
					if ( blank(valueu) && status ) *status = FALSE;
					dvalu  = atof(valueu);
					}

				/* Error for missing upper vertical attribute */
				else
					{
					(void) destroy_set(areas);
					if ( NotNull(copycal) ) (void) CAL_destroy(copycal);
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot extract attribute ... %s",
							ver_attrib_upper);
						(void) fprintf(stdout,
							"  from field ... %s %s from %s %s at %s\n",
							descript.edef->name, descript.ldef->name,
							descript.sdef->name, descript.subdef->name,
							descript.vtime);
						}
					if ( status ) *status = FALSE;
					return 0.0;
					}

				/* Set lower vertical attribute for "special" case */
				if ( same(ver_attrib_lower, VerAttribBase) )
					{
					dvall = vaxis->locs[0];
					}

				/* Extract a "magic" lower vertical attribute      */
				/*  ... converted to real                          */
				/* Note that blank attributes return a value of 0! */
				else if ( magic_is_attribute(ver_attrib_lower) )
					{
					valuel = magic_get_attribute(ver_attrib_lower, FpaCblank,
								descript.vtime, FpaCblank, FpaCblank, FpaCblank,
								fpos, fdist, fbear, flen, DefSegDir, DefSegSpd,
								ver_units, FormatDirect, FpaCblank);
					if ( blank(valuel) && status ) *status = FALSE;
					dvall = atof(valuel);
					}

				/* Extract the lower vertical attribute ... converted to real */
				/* Note that blank attributes return a value of 0!            */
				else if ( CAL_has_attribute(cal, ver_attrib_lower) )
					{
					valuel = CAL_get_attribute(cal, ver_attrib_lower);
					if ( blank(valuel) && status ) *status = FALSE;
					dvall  = atof(valuel);
					}

				/* Error for missing lower vertical attribute */
				else
					{
					(void) destroy_set(areas);
					if ( NotNull(copycal) ) (void) CAL_destroy(copycal);
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot extract attribute ... %s",
							ver_attrib_lower);
						(void) fprintf(stdout,
							"  from field ... %s %s from %s %s at %s\n",
							descript.edef->name, descript.ldef->name,
							descript.sdef->name, descript.subdef->name,
							descript.vtime);
						}
					if ( status ) *status = FALSE;
					return 0.0;
					}

				/* Set the vertical parameter */
				dval[0] = (dvalu + dvall) / 2.0;
				}

			/* Error if no vertical attributes given */
			else
				{
				(void) error_report("Missing vertical attributes!");
				}

			/* Free space used by discrete areas */
			(void) destroy_set(areas);
			if ( NotNull(copycal) ) (void) CAL_destroy(copycal);
			break;

		/* Extract vertical value for line type fields */
		case FpaC_LINE:

			/* Find the field containing the line curves */
			curves = retrieve_curveset(&descript);
			if ( IsNull(curves) )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
						"Cannot extract field ... %s %s from %s %s at %s\n",
						descript.edef->name, descript.ldef->name,
						descript.sdef->name, descript.subdef->name,
						descript.vtime);
					}
				if ( status ) *status = FALSE;
				return 0.0;
				}

			/* Search for closest curve within proximity */
			/*  that matches the required attributes     */
			while ( curves->num > 0 )
				{

				/* Find the closest curve */
				curve = closest_curve(curves, pos[0], NullFloat, fpos, &iseg);
				if ( IsNull(curve) || IsNull(curve->attrib) )
					{
					(void) remove_item_from_set(curves, (ITEM) curve);
					continue;
					}

				/* Check the proximity */
				fdist  = great_circle_distance(&BaseMap, pos[0], fpos);
				fdist /= 1000.0;
				if ( fdist > proximity ) break;

				/* Determine bearing from location to feature */
				fbear  = great_circle_bearing(&BaseMap, pos[0], fpos);

				/* Change sign of proximity wrt curve sense */
				(void) recall_curve_sense(curve, &sense);
				(void) curve_test_point(curve, pos[0],
						NullFloat, NullPoint, NullInt, NullLogical, &right);
				if ( (sense == Right && right) || (sense == Left && !right) )
					fdist = -fdist;

				/* Determine length of curve */
				flen = great_circle_line_length(&BaseMap, curve->line);

				/* Determine direction of curve */
				(void) line_span_info(curve->line, iseg, spos, epos, NullPoint);
				xdir = great_circle_bearing(&BaseMap, spos, epos);

				/* Check for matching category attributes */
				if ( match_category_attributes(curve->attrib,
						ver_cat_cascade, ver_cat_attrib, ver_num_catatt) )
					{
					copycal = CAL_duplicate(curve->attrib);
					break;
					}

				/* Attributes did not match               */
				/*  ... so remove this curve from the set */
				(void) remove_item_from_set(curves, (ITEM) curve);
				}

			/* Ensure that matching attributes were found */
			if ( IsNull(copycal) )
				{
				(void) destroy_set(curves);
				if ( status ) *status = FALSE;
				return 0.0;
				}
			else
				cal = copycal;

			/* Extract vertical attribute from attribute structure */
			if ( !blank(ver_attrib) )
				{

				/* Extract a "magic" vertical attribute ... converted to real */
				/* Note that blank attributes return a value of 0!            */
				if ( magic_is_attribute(ver_attrib) )
					{
					value = magic_get_attribute(ver_attrib, FpaCblank,
								descript.vtime, FpaCblank, FpaCblank, FpaCblank,
								fpos, fdist, fbear, flen, xdir, DefSegSpd,
								ver_units, FormatDirect, FpaCblank);
					if ( blank(value) && status ) *status = FALSE;
					dval[0] = atof(value);
					}

				/* Extract the vertical attribute ... converted to real */
				/* Note that blank attributes return a value of 0!      */
				else if ( CAL_has_attribute(cal, ver_attrib) )
					{
					value = CAL_get_attribute(cal, ver_attrib);
					if ( blank(value) && status ) *status = FALSE;
					dval[0] = atof(value);
					}

				/* Error for missing vertical attribute */
				else
					{
					(void) destroy_set(curves);
					(void) CAL_destroy(copycal);
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot extract attribute ... %s", ver_attrib);
						(void) fprintf(stdout,
							"  from field ... %s %s from %s %s at %s\n",
							descript.edef->name, descript.ldef->name,
							descript.sdef->name, descript.subdef->name,
							descript.vtime);
						}
					if ( status ) *status = FALSE;
					return 0.0;
					}
				}

			/* Extract vertical attributes from attribute structure */
			else if ( !blank(ver_attrib_upper) && !blank(ver_attrib_lower) )
				{

				/* Set upper vertical attribute for "special" case */
				if ( same(ver_attrib_upper, VerAttribTop) )
					{
					dvalu = vaxis->locs[vaxis->num-1];
					}

				/* Extract a "magic" upper vertical attribute      */
				/*  ... converted to real                          */
				/* Note that blank attributes return a value of 0! */
				else if ( magic_is_attribute(ver_attrib_upper) )
					{
					valueu = magic_get_attribute(ver_attrib_upper, FpaCblank,
								descript.vtime, FpaCblank, FpaCblank, FpaCblank,
								fpos, fdist, fbear, flen, xdir, DefSegSpd,
								ver_units, FormatDirect, FpaCblank);
					if ( blank(valueu) && status ) *status = FALSE;
					dvalu = atof(valueu);
					}

				/* Extract the upper vertical attribute ... converted to real */
				/* Note that blank attributes return a value of 0!            */
				else if ( CAL_has_attribute(cal, ver_attrib_upper) )
					{
					valueu = CAL_get_attribute(cal, ver_attrib_upper);
					if ( blank(valueu) && status ) *status = FALSE;
					dvalu  = atof(valueu);
					}

				/* Error for missing upper vertical attribute */
				else
					{
					(void) destroy_set(curves);
					(void) CAL_destroy(copycal);
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot extract attribute ... %s",
							ver_attrib_upper);
						(void) fprintf(stdout,
							"  from field ... %s %s from %s %s at %s\n",
							descript.edef->name, descript.ldef->name,
							descript.sdef->name, descript.subdef->name,
							descript.vtime);
						}
					if ( status ) *status = FALSE;
					return 0.0;
					}

				/* Set lower vertical attribute for "special" case */
				if ( same(ver_attrib_lower, VerAttribBase) )
					{
					dvall = vaxis->locs[0];
					}

				/* Extract a "magic" lower vertical attribute      */
				/*  ... converted to real                          */
				/* Note that blank attributes return a value of 0! */
				else if ( magic_is_attribute(ver_attrib_lower) )
					{
					valuel = magic_get_attribute(ver_attrib_lower, FpaCblank,
								descript.vtime, FpaCblank, FpaCblank, FpaCblank,
								fpos, fdist, fbear, flen, xdir, DefSegSpd,
								ver_units, FormatDirect, FpaCblank);
					if ( blank(valuel) && status ) *status = FALSE;
					dvall = atof(valuel);
					}

				/* Extract the lower vertical attribute ... converted to real */
				/* Note that blank attributes return a value of 0!            */
				else if ( CAL_has_attribute(cal, ver_attrib_lower) )
					{
					valuel = CAL_get_attribute(cal, ver_attrib_lower);
					if ( blank(valuel) && status ) *status = FALSE;
					dvall  = atof(valuel);
					}

				/* Error for missing lower vertical attribute */
				else
					{
					(void) destroy_set(curves);
					(void) CAL_destroy(copycal);
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot extract attribute ... %s",
							ver_attrib_lower);
						(void) fprintf(stdout,
							"  from field ... %s %s from %s %s at %s\n",
							descript.edef->name, descript.ldef->name,
							descript.sdef->name, descript.subdef->name,
							descript.vtime);
						}
					if ( status ) *status = FALSE;
					return 0.0;
					}

				/* Set the vertical parameter */
				dval[0] = (dvalu + dvall) / 2.0;
				}

			/* Error if no vertical attributes given */
			else
				{
				(void) error_report("Missing vertical attributes!");
				}

			/* Free space used by line curves */
			(void) destroy_set(curves);
			(void) CAL_destroy(copycal);
		break;

		/* Extract vertical value for scattered type fields */
		case FpaC_SCATTERED:

			/* Find the field containing the scattered spots */
			spots = retrieve_spotset(&descript);
			if ( IsNull(spots) )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
						"Cannot extract field ... %s %s from %s %s at %s\n",
						descript.edef->name, descript.ldef->name,
						descript.sdef->name, descript.subdef->name,
						descript.vtime);
					}
				if ( status ) *status = FALSE;
				return 0.0;
				}

			/* Search for closest spot within proximity */
			/*  that matches the required attributes    */
			while ( spots->num > 0 )
				{

				/* Find the closest spot */
				spot = closest_spot(spots, pos[0], NULL, NULL, NullFloat, fpos);
				if ( IsNull(spot) || IsNull(spot->attrib) )
					{
					(void) remove_item_from_set(spots, (ITEM) spot);
					continue;
					}

				/* Check the proximity */
				fdist  = great_circle_distance(&BaseMap, pos[0], fpos);
				fdist /= 1000.0;
				if ( fdist > proximity ) break;

				/* Determine bearing from location to feature */
				fbear  = great_circle_bearing(&BaseMap, pos[0], fpos);

				/* Check for matching category attributes */
				if ( match_category_attributes(spot->attrib,
						ver_cat_cascade, ver_cat_attrib, ver_num_catatt) )
					{
					copycal = CAL_duplicate(spot->attrib);
					break;
					}

				/* Attributes did not match              */
				/*  ... so remove this spot from the set */
				(void) remove_item_from_set(spots, (ITEM) spot);
				}

			/* Ensure that matching attributes were found */
			if ( IsNull(copycal) )
				{
				(void) destroy_set(spots);
				if ( status ) *status = FALSE;
				return 0.0;
				}
			else
				cal = copycal;

			/* Extract vertical attribute from attribute structure */
			if ( !blank(ver_attrib) )
				{

				/* Extract a "magic" vertical attribute ... converted to real */
				/* Note that blank attributes return a value of 0!            */
				if ( magic_is_attribute(ver_attrib) )
					{
					value = magic_get_attribute(ver_attrib, FpaCblank,
								descript.vtime, FpaCblank, FpaCblank, FpaCblank,
								fpos, fdist, fbear, flen, DefSegDir, DefSegSpd,
								ver_units, FormatDirect, FpaCblank);
					if ( blank(value) && status ) *status = FALSE;
					dval[0] = atof(value);
					}

				/* Extract the vertical attribute ... converted to real */
				/* Note that blank attributes return a value of 0!      */
				else if ( CAL_has_attribute(cal, ver_attrib) )
					{
					value = CAL_get_attribute(cal, ver_attrib);
					if ( blank(value) && status ) *status = FALSE;
					dval[0] = atof(value);
					}

				/* Error for missing vertical attribute */
				else
					{
					(void) destroy_set(spots);
					(void) CAL_destroy(copycal);
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot extract attribute ... %s", ver_attrib);
						(void) fprintf(stdout,
							"  from field ... %s %s from %s %s at %s\n",
							descript.edef->name, descript.ldef->name,
							descript.sdef->name, descript.subdef->name,
							descript.vtime);
						}
					if ( status ) *status = FALSE;
					return 0.0;
					}
				}

			/* Extract vertical attributes from attribute structure */
			else if ( !blank(ver_attrib_upper) && !blank(ver_attrib_lower) )
				{

				/* Set upper vertical attribute for "special" case */
				if ( same(ver_attrib_upper, VerAttribTop) )
					{
					dvalu = vaxis->locs[vaxis->num-1];
					}

				/* Extract a "magic" upper vertical attribute      */
				/*  ... converted to real                          */
				/* Note that blank attributes return a value of 0! */
				else if ( magic_is_attribute(ver_attrib_upper) )
					{
					valueu = magic_get_attribute(ver_attrib_upper, FpaCblank,
								descript.vtime, FpaCblank, FpaCblank, FpaCblank,
								fpos, fdist, fbear, flen, DefSegDir, DefSegSpd,
								ver_units, FormatDirect, FpaCblank);
					if ( blank(valueu) && status ) *status = FALSE;
					dvalu = atof(valueu);
					}

				/* Extract the upper vertical attribute ... converted to real */
				/* Note that blank attributes return a value of 0!            */
				else if ( CAL_has_attribute(cal, ver_attrib_upper) )
					{
					valueu = CAL_get_attribute(cal, ver_attrib_upper);
					if ( blank(valueu) && status ) *status = FALSE;
					dvalu  = atof(valueu);
					}

				/* Error for missing upper vertical attribute */
				else
					{
					(void) destroy_set(spots);
					(void) CAL_destroy(copycal);
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot extract attribute ... %s",
							ver_attrib_upper);
						(void) fprintf(stdout,
							"  from field ... %s %s from %s %s at %s\n",
							descript.edef->name, descript.ldef->name,
							descript.sdef->name, descript.subdef->name,
							descript.vtime);
						}
					if ( status ) *status = FALSE;
					return 0.0;
					}

				/* Set lower vertical attribute for "special" case */
				if ( same(ver_attrib_lower, VerAttribBase) )
					{
					dvall = vaxis->locs[0];
					}

				/* Extract a "magic" lower vertical attribute      */
				/*  ... converted to real                          */
				/* Note that blank attributes return a value of 0! */
				else if ( magic_is_attribute(ver_attrib_lower) )
					{
					valuel = magic_get_attribute(ver_attrib_lower, FpaCblank,
								descript.vtime, FpaCblank, FpaCblank, FpaCblank,
								fpos, fdist, fbear, flen, DefSegDir, DefSegSpd,
								ver_units, FormatDirect, FpaCblank);
					if ( blank(valuel) && status ) *status = FALSE;
					dvall = atof(valuel);
					}

				/* Extract the lower vertical attribute ... converted to real */
				/* Note that blank attributes return a value of 0!            */
				else if ( CAL_has_attribute(cal, ver_attrib_lower) )
					{
					valuel = CAL_get_attribute(cal, ver_attrib_lower);
					if ( blank(valuel) && status ) *status = FALSE;
					dvall  = atof(valuel);
					}

				/* Error for missing lower vertical attribute */
				else
					{
					(void) destroy_set(spots);
					(void) CAL_destroy(copycal);
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot extract attribute ... %s",
							ver_attrib_lower);
						(void) fprintf(stdout,
							"  from field ... %s %s from %s %s at %s\n",
							descript.edef->name, descript.ldef->name,
							descript.sdef->name, descript.subdef->name,
							descript.vtime);
						}
					if ( status ) *status = FALSE;
					return 0.0;
					}

				/* Set the vertical parameter */
				dval[0] = (dvalu + dvall) / 2.0;
				}

			/* Error if no vertical attributes given */
			else
				{
				(void) error_report("Missing vertical attributes!");
				}

			/* Free space used by scattered spots */
			(void) destroy_set(spots);
			(void) CAL_destroy(copycal);
		break;

		/* Extract vertical value for link chain type fields */
		case FpaC_LCHAIN:

			/* Find the field containing the link chains */
			chains = retrieve_lchainset(&descript);
			if ( IsNull(chains) )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout,
						"Cannot extract field ... %s %s from %s %s at %s\n",
						descript.edef->name, descript.ldef->name,
						descript.sdef->name, descript.subdef->name,
						descript.vtime);
					}
				if ( status ) *status = FALSE;
				return 0.0;
				}

			/* Search for closest link chain within proximity */
			/*  that matches the required attributes          */
			while ( chains->num > 0 )
				{

				/* Find the closest link chain */
				chain = closest_lchain(chains, pos[0], NullFloat, fpos, &iseg);
				if ( IsNull(chain) || IsNull(chain->attrib) )
					{
					(void) remove_item_from_set(chains, (ITEM) chain);
					continue;
					}

				/* Check the proximity */
				fdist  = great_circle_distance(&BaseMap, pos[0], fpos);
				fdist /= 1000.0;
				if ( fdist > proximity ) break;

				/* Determine bearing from location to feature */
				fbear  = great_circle_bearing(&BaseMap, pos[0], fpos);

				/* Interpolate the link chain (if required) */
				if ( chain->dointerp ) (void) interpolate_lchain(chain);

				/* Determine length of link chain track */
				flen = great_circle_line_length(&BaseMap, chain->track);

				/* Determine direction and speed on link chain track */
				if ( chain->inum > 1 )
					{
					(void) line_span_info(chain->track, iseg, spos, epos,
																	NullPoint);
					xdir  = great_circle_bearing(&BaseMap, spos, epos);
					xspd  = point_dist(spos, epos) / (float) chain->minterp;
					xspd *= BaseMap.definition.units;
					xspd /= 60.0;
					}
				else
					{
					xdir = DefSegDir;
					xspd = DefSegSpd;
					}

				/* Check for matching category attributes */
				if ( match_category_attributes(chain->attrib,
						ver_cat_cascade, ver_cat_attrib, ver_num_catatt) )
					{
					copycal = CAL_duplicate(chain->attrib);
					break;
					}

				/* Attributes did not match                    */
				/*  ... so remove this link chain from the set */
				(void) remove_item_from_set(chains, (ITEM) chain);
				}

			/* Ensure that matching attributes were found */
			if ( IsNull(copycal) )
				{
				(void) destroy_set(chains);
				if ( status ) *status = FALSE;
				return 0.0;
				}
			else
				cal = copycal;

			/* Extract vertical attribute from attribute structure */
			if ( !blank(ver_attrib) )
				{

				/* Extract a "magic" vertical attribute ... converted to real */
				/* Note that blank attributes return a value of 0!            */
				if ( magic_is_attribute(ver_attrib) )
					{
					value = magic_get_attribute(ver_attrib, FpaCblank,
								descript.vtime, FpaCblank, FpaCblank, FpaCblank,
								fpos, fdist, fbear, flen, xdir, xspd,
								ver_units, FormatDirect, FpaCblank);
					if ( blank(value) && status ) *status = FALSE;
					dval[0] = atof(value);
					}

				/* Extract the vertical attribute ... converted to real */
				/* Note that blank attributes return a value of 0!      */
				else if ( CAL_has_attribute(cal, ver_attrib) )
					{
					value = CAL_get_attribute(cal, ver_attrib);
					if ( blank(value) && status ) *status = FALSE;
					dval[0] = atof(value);
					}

				/* Error for missing vertical attribute */
				else
					{
					(void) destroy_set(chains);
					(void) CAL_destroy(copycal);
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot extract attribute ... %s", ver_attrib);
						(void) fprintf(stdout,
							"  from field ... %s %s from %s %s at %s\n",
							descript.edef->name, descript.ldef->name,
							descript.sdef->name, descript.subdef->name,
							descript.vtime);
						}
					if ( status ) *status = FALSE;
					return 0.0;
					}
				}

			/* Extract vertical attributes from attribute structure */
			else if ( !blank(ver_attrib_upper) && !blank(ver_attrib_lower) )
				{

				/* Set upper vertical attribute for "special" case */
				if ( same(ver_attrib_upper, VerAttribTop) )
					{
					dvalu = vaxis->locs[vaxis->num-1];
					}

				/* Extract a "magic" upper vertical attribute      */
				/*  ... converted to real                          */
				/* Note that blank attributes return a value of 0! */
				else if ( magic_is_attribute(ver_attrib_upper) )
					{
					valueu = magic_get_attribute(ver_attrib_upper, FpaCblank,
								descript.vtime, FpaCblank, FpaCblank, FpaCblank,
								fpos, fdist, fbear, flen, xdir, xspd,
								ver_units, FormatDirect, FpaCblank);
					if ( blank(valueu) && status ) *status = FALSE;
					dvalu = atof(valueu);
					}

				/* Extract the upper vertical attribute ... converted to real */
				/* Note that blank attributes return a value of 0!            */
				else if ( CAL_has_attribute(cal, ver_attrib_upper) )
					{
					valueu = CAL_get_attribute(cal, ver_attrib_upper);
					if ( blank(valueu) && status ) *status = FALSE;
					dvalu  = atof(valueu);
					}

				/* Error for missing upper vertical attribute */
				else
					{
					(void) destroy_set(chains);
					(void) CAL_destroy(copycal);
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot extract attribute ... %s",
							ver_attrib_upper);
						(void) fprintf(stdout,
							"  from field ... %s %s from %s %s at %s\n",
							descript.edef->name, descript.ldef->name,
							descript.sdef->name, descript.subdef->name,
							descript.vtime);
						}
					if ( status ) *status = FALSE;
					return 0.0;
					}

				/* Set lower vertical attribute for "special" case */
				if ( same(ver_attrib_lower, VerAttribBase) )
					{
					dvall = vaxis->locs[0];
					}

				/* Extract a "magic" lower vertical attribute      */
				/*  ... converted to real                          */
				/* Note that blank attributes return a value of 0! */
				else if ( magic_is_attribute(ver_attrib_lower) )
					{
					valuel = magic_get_attribute(ver_attrib_lower, FpaCblank,
								descript.vtime, FpaCblank, FpaCblank, FpaCblank,
								fpos, fdist, fbear, flen, xdir, xspd,
								ver_units, FormatDirect, FpaCblank);
					if ( blank(valuel) && status ) *status = FALSE;
					dvall = atof(valuel);
					}

				/* Extract the lower vertical attribute ... converted to real */
				/* Note that blank attributes return a value of 0!            */
				else if ( CAL_has_attribute(cal, ver_attrib_lower) )
					{
					valuel = CAL_get_attribute(cal, ver_attrib_lower);
					if ( blank(valuel) && status ) *status = FALSE;
					dvall  = atof(valuel);
					}

				/* Error for missing lower vertical attribute */
				else
					{
					(void) destroy_set(chains);
					(void) CAL_destroy(copycal);
					if ( Verbose )
						{
						(void) fprintf(stdout,
							"Cannot extract attribute ... %s",
							ver_attrib_lower);
						(void) fprintf(stdout,
							"  from field ... %s %s from %s %s at %s\n",
							descript.edef->name, descript.ldef->name,
							descript.sdef->name, descript.subdef->name,
							descript.vtime);
						}
					if ( status ) *status = FALSE;
					return 0.0;
					}

				/* Set the vertical parameter */
				dval[0] = (dvalu + dvall) / 2.0;
				}

			/* Error if no vertical attributes given */
			else
				{
				(void) error_report("Missing vertical attributes!");
				}

			/* Free space used by link chains */
			(void) destroy_set(chains);
			(void) CAL_destroy(copycal);
		break;

		/* Cannot extract values for other types of fields */
		default:
			if ( Verbose )
				{
				(void) fprintf(stdout,
					"Cannot sample field ... %s %s from %s %s at %s\n",
					descript.edef->name, descript.ldef->name,
					descript.sdef->name, descript.subdef->name, descript.vtime);
				}
			if ( status ) *status = FALSE;
			return 0.0;
		}

	/* Determine position on vertical axis */
	(void) Tween1(vaxis->num, vaxis->vals, vaxis->locs,
			1, dval, dloc, NullDouble);

	/* Return position on vertical axis */
	return (float) dloc[0];
	}

static	float		set_vertical_position_data_file

	(
	XSECT_VER_AXIS	*vaxis,
	STRING			ver_data_file,
	STRING			ver_data_file_format,
	STRING			ver_data_file_units,
	STRING			ver_ident,
	float			ver_flat,
	float			ver_flon,
	STRING			ver_vtime,
	STRING			ver_attrib,
	STRING			ver_units,
	LOGICAL			*status
	)

	{
	char			mlat[GPGMedium], mlon[GPGMedium];
	CAL				cal;
	STRING			val;
	float			fval;
	double			dval[1], dloc[1];

	/* Initialize return parameter */
	if ( status ) *status = TRUE;

	/* Return immediately for "special" vertical attributes */
	if ( same(ver_attrib, VerAttribTop) )
		{
		return vaxis->locs[vaxis->num-1];
		}
	else if ( same(ver_attrib, VerAttribBase) )
		{
		return vaxis->locs[0];
		}

	/* Return immediately if no data file or format defined */
	if ( blank(ver_data_file) || blank(ver_data_file_format) )
		{
		return 0.0;
		}

	/* Convert latitude and longitude to STRING format */
	(void) sprintf(mlat, "%f", ver_flat);
	(void) sprintf(mlon, "%f", ver_flon);

	/* Extract vertical value from the data file */
	if ( !match_data_file(ver_data_file, ver_data_file_format,
			ver_data_file_units, FpaCblank,
			ver_ident, mlat, mlon, ver_vtime, &cal) )
		{
		if ( Verbose )
			{
			(void) fprintf(stdout,
				"Cannot match info in data file ... %s\n", ver_data_file);
			}
		if ( status ) *status = FALSE;
		return 0.0;
		}

	/* >>>>> debug testing for set_vertical_position_data_file() <<<<< */
	if ( DebugMode )
		{
		(void) fprintf(stdout,
					"  Matching ident/lat/lon/vtime ... %s %s %s\n",
					ver_ident, mlat, mlon, ver_vtime);
		val = CAL_get_attribute(cal, AttribGPGenIdent);
		if ( !blank(val) )
			(void) fprintf(stdout, "    Return ident ... %s\n", val);
		val = CAL_get_attribute(cal, AttribGPGenLat);
		if ( !blank(val) )
			(void) fprintf(stdout, "    Return latitude ... %s\n", val);
		val = CAL_get_attribute(cal, AttribGPGenLon);
		if ( !blank(val) )
			(void) fprintf(stdout, "    Return longitude ... %s\n", val);
		val = CAL_get_attribute(cal, AttribGPGenLabel);
		if ( !blank(val) )
			(void) fprintf(stdout, "    Return label ... %s\n", val);
		val = CAL_get_attribute(cal, AttribGPGenValue);
		if ( !blank(val) )
			(void) fprintf(stdout, "    Return value ... %s\n", val);
		val = CAL_get_attribute(cal, AttribGPGenWind);
		if ( !blank(val) )
			(void) fprintf(stdout, "    Return wind ... %s\n", val);
		}
	/* >>>>> debug testing for set_vertical_position_data_file() <<<<< */

	/* Extract the value attribute from the CAL structure */
	/* Note that value has been converted to MKS!         */
	val = CAL_get_attribute(cal, AttribGPGenValue);
	if ( blank(val) )
		{
		if ( Verbose )
			{
			(void) fprintf(stdout,
				"No value found in data file ... %s", ver_data_file);
			}
		if ( status ) *status = FALSE;
		return 0.0;
		}
	(void) sscanf(val, "%f", &fval);

	/* Convert the value to required units */
	(void) convert_value(FpaCmksUnits, (double) fval, ver_units, &dval[0]);

	/* Determine position on vertical axis */
	(void) Tween1(vaxis->num, vaxis->vals, vaxis->locs,
			1, dval, dloc, NullDouble);

	/* Return position on vertical axis */
	return (float) dloc[0];
	}

static	LINE		xsection_line_coordinates

	(
	LINE			line,		/* Line to display */
	GRA_XSECT		*xsect,
	XSECT_HOR_AXIS	*haxis,
	XSECT_VER_AXIS	*vaxis
	)

	{
	int				ipts;
	double			dval[1], dloc[1];
	POINT			pos;

	/* Static buffer to hold cross section line */
	static	LINE	XSecLine = NullLine;

	/* Return immediately for no line to convert */
	if ( IsNull(line) ) return NullLine;
	if ( line->numpts < 2 ) return NullLine;

	/* Return immediately for no cross section information */
	if ( IsNull(xsect) ) return NullLine;
	if ( IsNull(haxis) ) return NullLine;
	if ( IsNull(vaxis) ) return NullLine;

	/* Create or empty current line */
	if ( IsNull(XSecLine) ) XSecLine = create_line();
	else                    (void) empty_line(XSecLine);

	/* Loop through the points in the line */
	for ( ipts=0; ipts<line->numpts; ipts++ )
		{

		/* Determine the horizontal axis location */
		dval[0] = (double) line->points[ipts][X];
		(void) Tween1(haxis->num, haxis->pstns, haxis->locs,
				1, dval, dloc, NullDouble);
		pos[X] = xsect->x_off + (dloc[0] * xsect->width);

		/* Determine the vertical axis location */
		dval[0] = (double) line->points[ipts][Y];
		(void) Tween1(vaxis->num, vaxis->pstns, vaxis->locs,
				1, dval, dloc, NullDouble);
		pos[Y] = xsect->y_off + (dloc[0] * xsect->height);

		/* Add point to current line */
		(void) add_point_to_line(XSecLine, pos);
		}

	/* Return the current line */
	return XSecLine;
	}

static	BOUND		xsection_boundary_coordinates

	(
	BOUND			bound,
	GRA_XSECT		*xsect,
	XSECT_HOR_AXIS	*haxis,
	XSECT_VER_AXIS	*vaxis
	)

	{
	int				ipts, ihole;
	double			dval[1], dloc[1];
	POINT			pos;
	LINE			hole;

	/* Static buffer to hold cross section boundary */
	static	BOUND	XSecBound = NullBound;

	/* Return immediately for no boundary to convert */
	if ( IsNull(bound) ) return NullBound;
	if ( IsNull(bound->boundary) ) return NullBound;
	if ( bound->boundary->numpts < 2 ) return NullBound;

	/* Return immediately for no cross section information */
	if ( IsNull(xsect) ) return NullBound;
	if ( IsNull(haxis) ) return NullBound;
	if ( IsNull(vaxis) ) return NullBound;

	/* Empty current boundary */
	(void) empty_bound(XSecBound);

	/* Loop through the points in the boundary outline */
	for ( ipts=0; ipts<bound->boundary->numpts; ipts++ )
		{

		/* Determine the horizontal axis location */
		dval[0] = (double) bound->boundary->points[ipts][X];
		(void) Tween1(haxis->num, haxis->pstns, haxis->locs,
				1, dval, dloc, NullDouble);
		pos[X] = xsect->x_off + (dloc[0] * xsect->width);

		/* Determine the vertical axis location */
		dval[0] = (double) bound->boundary->points[ipts][Y];
		(void) Tween1(vaxis->num, vaxis->pstns, vaxis->locs,
				1, dval, dloc, NullDouble);
		pos[Y] = xsect->y_off + (dloc[0] * xsect->height);

		/* Add point to current boundary outline */
		(void) add_point_to_line(XSecBound->boundary, pos);
		}

	/* Now loop through the boundary holes */
	for ( ihole=0; ihole<bound->numhole; ihole++ )
		{

		/* Skip empty holes or holes with too few points */
		if ( IsNull(bound->holes[ihole]) ) continue;
		if ( bound->holes[ihole]->numpts < 2 ) continue;

		/* Create the hole */
		hole = create_line();

		/* Loop through the points in the boundary outline */
		for ( ipts=0; ipts<bound->holes[ihole]->numpts; ipts++ )
			{

			/* Determine the horizontal axis location */
			dval[0] = (double) bound->holes[ihole]->points[ipts][X];
			(void) Tween1(haxis->num, haxis->pstns, haxis->locs,
					1, dval, dloc, NullDouble);
			pos[X] = xsect->x_off + (dloc[0] * xsect->width);

			/* Determine the vertical axis location */
			dval[0] = (double) bound->holes[ihole]->points[ipts][Y];
			(void) Tween1(vaxis->num, vaxis->pstns, vaxis->locs,
					1, dval, dloc, NullDouble);
			pos[Y] = xsect->y_off + (dloc[0] * xsect->height);

			/* Add point to current hole */
			(void) add_point_to_line(hole, pos);
			}

		/* Add hole to current boundary */
		(void) add_bound_hole(XSecBound, hole);
		}

	/* Return the current boundary */
	return XSecBound;
	}

/***********************************************************************
*                                                                      *
*    G R A _ r e p l i c a t e _ p a t t e r n                         *
*                                                                      *
*     Replicate one component of a pattern along a line, returning a   *
*     series of lines.                                                 *
*                                                                      *
***********************************************************************/

static	LOGICAL		GRA_replicate_pattern

	(
	LINE		line,			/* Line to display */
	STRING		type,
	ITEM		item,
	float		width,
	float		length,
	int			*num_out,
	LINE		**out_lines
	)

	{
	int			ispan, ip, ipfilt;
	float		swidth, slength;
	AREA		parea = NullArea;
	CURVE		pcurve = NullCurve;
	LINE		pline, tline;
	float		x, xb, xc, xd, xpat;
	float		y, yb, yc, yd, ypat;
	float		dxcurr, dxnext;
	float		dycurr, dynext;
	float		dscurr, dsnext;
	float		nxcurr, nxnext, nxlbar, nxrbar;
	float		nycurr, nynext, nylbar, nyrbar;
	float		wb, wc, ww;
	float		nscale, lpos, rpos, lpat, rpat;
	int			last, rclose, poly;
	float		dx, dy, dang, ang1, ang2;
	int			iline, nline;
	LINE		*lines;

	static	int		num_ol  = 0;
	static	LINE	*olines = NullLineList;

	/* Initialize output parameters */
	if ( NotNull(num_out) )   *num_out   = 0;
	if ( NotNull(out_lines) ) *out_lines = NullLineList;

	/* Return immediately if no line to replicate */
	if ( IsNull(line) ) return FALSE;
	if ( line->numpts < 2 ) return FALSE;

	/* Empty the output list (if required) */
	if ( num_ol > 0 )
		{
		for ( iline=0; iline<num_ol; iline++ )
			{
			(void) destroy_line(olines[iline]);
			}
		FREEMEM(olines);
		num_ol = 0;
		}

	if ( same(type, "area") )
		{
		parea   = (AREA) item;
		pline   = parea->bound->boundary;
		poly    = TRUE;
		}
	else if ( same(type, "curve") )
		{
		pcurve  = (CURVE) item;
		pline   = pcurve->line;
		poly    = FALSE;
		}
	else
		{
		return FALSE;
		}

	/* Set pattern width and length ... scaled to map (if required) */
	swidth  = width  / map_scaling();
	slength = length / map_scaling();

	/* Match pattern component against each span of the given curve.      */
	/* Each span defines a trapeziod, having a height equal to twice the  */
	/* signed amplitude, and mean length equal to the length of the span. */
	/* The ends of the trapezoid are tilted according to the average of   */
	/* the span perpendicular with that of either adjacent span.          */
	/* Hence we need to keep track of 3 spans (4 points) running:         */
	/*   The 4 points are refered to as a, b, c and d in sequence         */
	/*   Span prev (ab) is the span preceding the current span            */
	/*   Span curr (bc) is the span that we are currently plotting        */
	/*   Span next (cd) is the span following the current span            */
	/*   frst and last tell whether prev or next span are absent          */
	/*   dx.. and dy.. are the x and y lengths of the corresponding span  */
	/*   nx.. and ny.. are the x and y normals of the corresponding span  */
	/*   nx.bar and ny.bar are the average normals at the span ends       */

	xb = line->points[0][X];
	yb = line->points[0][Y];
	for ( ispan=0; ispan<line->numpts-1; ispan++ )
		{
		xc     = line->points[ispan+1][X];
		yc     = line->points[ispan+1][Y];
		dxcurr = xc - xb;
		dycurr = yc - yb;
		dscurr = (float) hypot((double) dxcurr, (double) dycurr);
		if ( dscurr > 0 ) break;
		}

	if ( ispan >= line->numpts-1 ) return FALSE;

	lpos   = 0;
	rpos   = lpos + dscurr;
	nscale = swidth / dscurr;
	nxcurr = -dycurr * nscale;
	nycurr = dxcurr * nscale;
	nxlbar = nxcurr;
	nylbar = nycurr;
	nxrbar = nxcurr;
	nyrbar = nycurr;

	for ( ; ispan<line->numpts-1; ispan++ )
		{
		/* Make sure we remain continuous with following span */

		last = (int) (ispan >= line->numpts-2);
		if ( !last )
			{

			xd     = line->points[ispan+2][X];
			yd     = line->points[ispan+2][Y];
			dxnext = xd - xc;
			dynext = yd - yc;
			dsnext = (float) hypot((double) dxnext, (double) dynext);

			if ( dsnext <= 0 ) continue;
			nscale = swidth / dsnext;
			nxnext = -dynext * nscale;
			nynext = dxnext * nscale;
			nxrbar = (nxcurr+nxnext)/2;
			nyrbar = (nycurr+nynext)/2;
			}

		/* Map the trapezoid to the corresponding portion of the pattern */
		lpat = lpos;
		while (lpat < rpos)
			{
			/* Break apart at multiples of the pattern length */
			if ( rpos >= slength )
				{
				rclose = TRUE;
				rpat   = slength;
				}
			else
				{

				rclose = last;
				rpat   = rpos;
				}

			/* Set up clipping to the current portion of the pattern     */
			/* Clip the current pattern component and display the pieces */

			reset_pipe();
			enable_clip(lpat, rpat, -swidth, swidth, poly, poly);
			enable_save();
			line_pipe(pline);
			nline = recall_save(&lines);
			for ( iline=0; iline<nline; iline++ )
				{
				tline  = create_line();
				ipfilt = 0;
				for ( ip=0; ip<lines[iline]->numpts; ip++ )
					{
					if ( ip>0 )
						{
						dx = lines[iline]->points[ip][X]-tline->points[ipfilt-1][X];
						dy = lines[iline]->points[ip][Y]-tline->points[ipfilt-1][Y];
						if ( dx == 0.0 && dy == 0.0 )
							{
							continue;
							}
						}
					add_point_to_line(tline, lines[iline]->points[ip]);
					ipfilt++;
					}
				for ( ip=0; ip<tline->numpts; ip++ )
					{
					xpat = tline->points[ip][X];
					ypat = tline->points[ip][Y];
					wc = (xpat-lpos)/dscurr;
					wb = 1 - wc;
					ww = ypat/swidth;
					x  = wb*(xb + ww*nxlbar) + wc*(xc + ww*nxrbar);
					y  = wb*(yb + ww*nylbar) + wc*(yc + ww*nyrbar);

					tline->points[ip][X] = x;
					tline->points[ip][Y] = y;
					if ( ip>1 )
						{
						dx = tline->points[ip-1][X]-tline->points[ip-2][X];
						dy = tline->points[ip-1][Y]-tline->points[ip-2][Y];
						ang1 = fpa_atan2deg(dy, dx);
						dx = tline->points[ip][X]-tline->points[ip-1][X];
						dy = tline->points[ip][Y]-tline->points[ip-1][Y];
						ang2 = fpa_atan2deg(dy, dx);
						dang = fabs(ang1-ang2);
						if ( dang >= 175.0 && dang <= 185.0 )
							{
							tline->points[ip-1][X] = tline->points[ip][X];
							tline->points[ip-1][Y] = tline->points[ip][Y];
							}
						}
					if ( ip == tline->numpts-1 && poly )
						{
						dx = tline->points[ip][X]-tline->points[ip-1][X];
						dy = tline->points[ip][Y]-tline->points[ip-1][Y];
						ang1 = fpa_atan2deg(dy, dx);
						dx = tline->points[0][X]-tline->points[1][X];
						dy = tline->points[0][Y]-tline->points[1][Y];
						ang2 = fpa_atan2deg(dy, dx);
						dang = fabs(ang1-ang2);
						if ( dang >= 175.0 && dang <= 185.0 )
							{
							tline->points[0][X] = tline->points[1][X];
							tline->points[0][Y] = tline->points[1][Y];
							tline->points[ip][X] = tline->points[ip-1][X];
							tline->points[ip][Y] = tline->points[ip-1][Y];
							}
						}
					}

				/* Save the line to the output list */
				olines = GETMEM(olines, LINE, ++num_ol);
				olines[num_ol-1] = tline;
				}

			if ( rclose )
				{
				lpos -= slength;
				rpos -= slength;
				lpat -= slength;
				rpat  = 0;
				}

			rclose = FALSE;
			lpat   = rpat;
			}

		/* Close off last polygon if filled */
		if ( poly && last )
			{
			}

		/* Prepare next span */
		xb     = xc;		yb     = yc;
		xc     = xd;		yc     = yd;

		dxcurr = dxnext;	dycurr = dynext;
		nxcurr = nxnext;	nycurr = nynext;
		nxlbar = nxrbar;	nylbar = nyrbar;
		nxrbar = nxcurr;	nyrbar = nycurr;

		dscurr = dsnext;
		lpos   = rpos;
		rpos   = lpos + dscurr;
		}

	/* Return the saved lines */
	if ( NotNull(num_out) )   *num_out   = num_ol;
	if ( NotNull(out_lines) ) *out_lines = olines;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    G R A _ d i s p l a y _ d r a w _ l i n e                         *
*    G R A _ d i s p l a y _ d r a w _ o u t l i n e                   *
*    G R A _ d i s p l a y _ s i m p l e _ l i n e                     *
*    G R A _ d i s p l a y _ s i m p l e _ o u t l i n e               *
*    G R A _ d i s p l a y _ s i m p l e _ b o u n d a r y             *
*    G R A _ d i s p l a y _ p a t t e r n e d _ l i n e               *
*    G R A _ d i s p l a y _ p a t t e r n e d _ o u t l i n e         *
*    G R A _ d i s p l a y _ p a t t e r n e d _ b o u n d a r y       *
*                                                                      *
***********************************************************************/

static	LOGICAL		GRA_display_draw_line

	(
	LINE		line,			/* Line to display */
	STRING		arrow_name,		/* Arrow name (for ends of lines) */
	float		xoff,			/* x offset of line (display units) */
	float		yoff,			/* y offset of line (display units) */
	LOGICAL		clip_to_map,	/* Clip line to current map? */
	STRING		pattern,		/* Name of pattern for line */
	STRING		pattern_width,	/* Pattern width factor */
	STRING		pattern_length,	/* Pattern repetition factor */
	COMP_PRES	*comp_pres,		/* Structure containing presentations */
	int			num_comp		/* Number of presentations */
	)

	{
	PRES		cur_pres;

	/* Check that line exists */
	if ( IsNull(line) ) return TRUE;
	if ( line->numpts < 2 ) return TRUE;

	/* Set the current presentation parameters */
	(void) copy_presentation(&cur_pres, &CurPres);

	/* Override pattern and pattern sizes (if requested) */
	if ( !blank(pattern) )
						(void) strcpy(cur_pres.pattern,        pattern);
	if ( !blank(pattern_width) )
						(void) strcpy(cur_pres.pattern_width,  pattern_width);
	if ( !blank(pattern_length) )
						(void) strcpy(cur_pres.pattern_length, pattern_length);

	/* Override component presentation (if requested) */
	if ( num_comp > 0 )
		{
		cur_pres.num_comp  = num_comp;
		cur_pres.comp_pres = comp_pres;
		(void) reset_presentation_by_comp_pres(&cur_pres, &comp_pres[0]);
		}

	/* Override component presentation (if requested) */
	if ( num_comp > 0 )
		{
		cur_pres.num_comp  = num_comp;
		cur_pres.comp_pres = comp_pres;
		(void) reset_presentation_by_comp_pres(&cur_pres,
												&cur_pres.comp_pres[0]);
		}

	/* Start of line grouping */
	(void) write_graphics_group(GPGstart, NullPointer, 0);

	/* Display a "simple" line */
	if ( blank(cur_pres.pattern) || same(cur_pres.pattern, PatternSimple) )
		{

		/* Display the line */
		if ( !GRA_display_simple_line(line, xoff, yoff,
				cur_pres, clip_to_map) ) return FALSE;
		}

	/* Display a line using the given pattern */
	else
		{

		/* Display the line */
		if ( !GRA_display_patterned_line(line, xoff, yoff,
				cur_pres, clip_to_map, Right) ) return FALSE;
		}

	/* Now display the arrow (if required) */
	if ( !blank(arrow_name) )
		{

		/* Set presentation from last component presentation */
		/*  (if available)                                   */
		if ( cur_pres.num_comp > 0 )
			{
			(void) reset_presentation_by_comp_pres(&cur_pres,
					&cur_pres.comp_pres[cur_pres.num_comp-1]);
			}

		/* Display the arrow */
		if ( !GRA_display_arrow(arrow_name, line, xoff, yoff,
				cur_pres, clip_to_map) ) return FALSE;
		}

	/* End of line grouping */
	(void) write_graphics_group(GPGend, NullPointer, 0);

	/* Return TRUE */
	return TRUE;
	}

static	LOGICAL		GRA_display_draw_outline

	(
	LINE		outline,		/* Outline to display */
	float		xoff,			/* x offset of line (display units) */
	float		yoff,			/* y offset of line (display units) */
	LOGICAL		clip_to_map,	/* Clip outline to current map? */
	STRING		interior_fill,	/* Colour for interior of outline */
	STRING		sym_fill_name,	/* Symbol pattern for interior of outline */
	STRING		pattern,		/* Name of pattern for outline */
	STRING		pattern_width,	/* Pattern width factor */
	STRING		pattern_length,	/* Pattern repetition factor */
	COMP_PRES	*comp_pres,		/* Structure containing presentations */
	int			num_comp		/* Number of presentations */
	)

	{
	PRES		cur_pres;

	/* Check that outline exists */
	if ( IsNull(outline) ) return TRUE;
	if ( outline->numpts < 2 ) return TRUE;

	/* Set the current presentation parameters */
	(void) copy_presentation(&cur_pres, &CurPres);

	/* Override interior_fill and symbol_fill_name (if requested) */
	if ( !blank(interior_fill) )
					(void) strcpy(cur_pres.interior_fill, interior_fill);
	if ( !blank(sym_fill_name) )
					(void) strcpy(cur_pres.sym_fill_name, sym_fill_name);

	/* Override pattern and pattern sizes (if requested) */
	if ( !blank(pattern) )
					(void) strcpy(cur_pres.pattern,        pattern);
	if ( !blank(pattern_width) )
					(void) strcpy(cur_pres.pattern_width,  pattern_width);
	if ( !blank(pattern_length) )
					(void) strcpy(cur_pres.pattern_length, pattern_length);

	/* Override component presentation (if requested) */
	if ( num_comp > 0 )
		{
		cur_pres.num_comp  = num_comp;
		cur_pres.comp_pres = comp_pres;
		(void) reset_presentation_by_comp_pres(&cur_pres,
												&cur_pres.comp_pres[0]);
		}

	/* Start of outline grouping */
	(void) write_graphics_group(GPGstart, NullPointer, 0);

	/* Display a "simple" outline */
	if ( blank(cur_pres.pattern) || same(cur_pres.pattern, PatternSimple) )
		{

		/* Display the outline */
		if ( !GRA_display_simple_outline(outline, xoff, yoff,
				cur_pres, clip_to_map) ) return FALSE;
		}

	/* Display an outline using the given pattern */
	else
		{

		/* Display the outline */
		if ( !GRA_display_patterned_outline(outline, xoff, yoff,
				cur_pres, clip_to_map, Right) ) return FALSE;
		}

	/* End of outline grouping */
	(void) write_graphics_group(GPGend, NullPointer, 0);

	/* Return TRUE */
	return TRUE;
	}

static	LOGICAL		GRA_display_simple_line

	(
	LINE		line,			/* Line to display */
	float		xoff,			/* x offset of line (display units) */
	float		yoff,			/* y offset of line (display units) */
	PRES		pres,			/* Presentation parameters */
	LOGICAL		clip_to_map		/* Clip line to current map? */
	)

	{
	int			nl, il;
	LINE		seg, *segs;
	PRES		temp_pres;

	/* Check that line exists */
	if ( IsNull(line) ) return TRUE;
	if ( line->numpts < 2 ) return TRUE;

	/* Save the current presentation */
	(void) copy_presentation(&temp_pres, &CurPres);

	/* Reset the current presentation to display the line */
	(void) copy_presentation(&CurPres, &pres);

	/* Clip the line to fit the current map (if requested) */
	if ( clip_to_map )
		{

		/* Clip the line into line segments */
		nl = GRA_clip_line(line, &segs);

		/* Draw each line segment */
		for ( il=0; il<nl; il++ )
			{

			/* Check number of points in each line segment */
			if ( segs[il]->numpts < 2 ) continue;

			/* Adjust each line segment based on current anchor and offsets */
			(void) anchored_line(segs[il], xoff, yoff, &seg);

			/* Display each line segment */
			(void) write_graphics_lines(1, &seg);
			}
		}

	/* Display all other lines as entered */
	else
		{

		/* Adjust the line based on current anchor and offsets */
		(void) anchored_line(line, xoff, yoff, &seg);

		/* Display the line */
		(void) write_graphics_lines(1, &seg);
		}

	/* Restore the original presentation */
	(void) copy_presentation(&CurPres, &temp_pres);

	/* Return TRUE */
	return TRUE;
	}

static	LOGICAL		GRA_display_simple_outline

	(
	LINE		line,			/* Outline to display */
	float		xoff,			/* x offset of outline (display units) */
	float		yoff,			/* y offset of outline (display units) */
	PRES		pres,			/* Presentation parameters */
	LOGICAL		clip_to_map		/* Clip outline to current map? */
	)

	{
	LINE		sego, seg;
	PRES		temp_pres;

	/* Check that outline exists */
	if ( IsNull(line) ) return TRUE;
	if ( line->numpts < 2 ) return TRUE;

	/* Save the current presentation */
	(void) copy_presentation(&temp_pres, &CurPres);

	/* Reset the current presentation to display the outline */
	(void) copy_presentation(&CurPres, &pres);

	/* Clip outline to fit the current map (if requested) */
	if ( clip_to_map )
		{

		/* Clip the outline to a single continuous line */
		sego = GRA_clip_outline(line);
		if ( IsNull(sego) ) return TRUE;
		if ( sego->numpts < 2 ) return TRUE;
		}

	/* Display all other outlines as entered */
	else
		{
		sego = line;
		}

	/* Adjust the outline based on current anchor and offsets */
	(void) anchored_line(sego, xoff, yoff, &seg);

	/* Display the outline filled with symbols */
	if ( !blank(pres.sym_fill_name) )
		{

		/* First display the "filled" outline */
		(void) write_graphics_outlines(1, &seg, FALSE, TRUE);

		/* Then display the symbols */
		if ( !GRA_display_outline_symbol_fill(seg, pres.sym_fill_name) )
				return FALSE;

		/* Then display the "outline" */
		(void) write_graphics_outlines(1, &seg, TRUE, FALSE);
		}

	/* Display the outline */
	else
		{
		(void) write_graphics_outlines(1, &seg, TRUE, TRUE);
		}

	/* Restore the original presentation */
	(void) copy_presentation(&CurPres, &temp_pres);

	/* Return TRUE */
	return TRUE;
	}

static	LOGICAL		GRA_display_simple_boundary

	(
	BOUND		bound,			/* Boundary to display */
	float		xoff,			/* x offset of boundary (display units) */
	float		yoff,			/* y offset of boundary (display units) */
	PRES		pres,			/* Presentation parameters */
	LOGICAL		clip_to_map		/* Clip boundary to current map? */
	)

	{
	BOUND		bndo, bnd;
	PRES		temp_pres;

	/* Check that boundary exists */
	if ( IsNull(bound) ) return TRUE;
	if ( IsNull(bound->boundary) ) return TRUE;
	if ( bound->boundary->numpts < 2 ) return TRUE;

	/* Save the current presentation */
	(void) copy_presentation(&temp_pres, &CurPres);

	/* Reset the current presentation to display the boundary */
	(void) copy_presentation(&CurPres, &pres);

	/* Clip boundary to fit the current map (if requested) */
	if ( clip_to_map )
		{

		/* Clip the boundary to a single continuous line (and holes) */
		bndo = GRA_clip_boundary(bound);
		if ( IsNull(bndo) ) return TRUE;
		if ( IsNull(bndo->boundary) ) return TRUE;
		if ( bndo->boundary->numpts < 2 ) return TRUE;
		}

	/* Display all other boundaries as entered */
	else
		{
		bndo = bound;
		}

	/* Adjust the boundary based on current anchor and offsets */
	(void) anchored_boundary(bndo, xoff, yoff, &bnd);

	/* Display the boundary filled with symbols */
	if ( !blank(pres.sym_fill_name) )
		{

		/* First display the "filled" boundary */
		(void) write_graphics_boundaries(1, &bnd, FALSE, TRUE);

		/* Then display the symbols */
		if ( !GRA_display_boundary_symbol_fill(bnd, pres.sym_fill_name) )
				return FALSE;

		/* Then display the boundary "outline" */
		(void) write_graphics_boundaries(1, &bnd, TRUE, FALSE);
		}

	/* Display the boundary */
	else
		{
		(void) write_graphics_boundaries(1, &bnd, TRUE, TRUE);
		}

	/* Restore the original presentation */
	(void) copy_presentation(&CurPres, &temp_pres);

	/* Return TRUE */
	return TRUE;
	}

static	LOGICAL			GRA_display_patterned_line

	(
	LINE		line,			/* Line to display */
	float		xoff,			/* x offset of line (display units) */
	float		yoff,			/* y offset of line (display units) */
	PRES		cur_pres,		/* Presentation parameters */
	LOGICAL		clip_to_map,	/* Clip line to current map? */
	HAND		sense			/* Line handedness */
	)

	{
	int			icomp, numlines, il, nl, iil;
	float		width, ampl, length;
	PATTERN		*ptemp;
	LINE		tline, xline;
	LINE		*rlines = NullLineList, *tlines = NullLineList;
	PRES		temp_pres;

	static	int		num_ol  = 0;
	static	LINE	*olines = NullLineList;

	/* Check that line exists */
	if ( IsNull(line) ) return TRUE;
	if ( line->numpts < 2 ) return TRUE;

	/* Check that a pattern is given */
	if ( blank(cur_pres.pattern) )
		{
		if ( Verbose )
			{
			(void) fprintf(stdout, " No pattern given\n");
			}
		return FALSE;
		}

	/* Set the pattern amplitude and length */
	(void) sscanf(cur_pres.pattern_width, "%f", &width);
	width = fabs(width);
	ampl  = ( sense == Left )? -width: width;
	(void) sscanf(cur_pres.pattern_length, "%f", &length);
	length = fabs(length);

	/* Get the requested pattern ... scaled to the amplitude and length */
	ptemp = get_pattern(cur_pres.pattern, ampl, length);
	if ( IsNull(ptemp) )
		{
		if ( Verbose )
			{
			(void) fprintf(stdout,
				" No pattern file for ... %s\n", cur_pres.pattern);
			}
		return FALSE;
		}

	/* Save the current presentation */
	(void) copy_presentation(&temp_pres, &CurPres);

	/* Replicate each component of the pattern along the whole line */
	for ( icomp=0; icomp<ptemp->num; icomp++ )
		{

		/* Set the default presentation if no component presentations used */
		if ( cur_pres.num_comp == 0 )
			{
			(void) copy_presentation(&CurPres, &cur_pres);
			}

		/* Match the presentation with the appropriate component presentation */
		else if ( icomp < cur_pres.num_comp )
			{
			(void) copy_presentation(&CurPres, &cur_pres);
			(void) reset_presentation_by_comp_pres(&CurPres,
													&cur_pres.comp_pres[icomp]);
			}

		/* Use last component presentation if not enough given! */
		else
			{
			(void) copy_presentation(&CurPres, &cur_pres);
			(void) reset_presentation_by_comp_pres(&CurPres,
									&cur_pres.comp_pres[cur_pres.num_comp-1]);
			}

		/* Use component "fill" as "interior_fill" for each pattern component */
		(void) strcpy(CurPres.interior_fill, CurPres.fill);

		/* Replicate the pattern along the line */
		if ( !GRA_replicate_pattern(line,
				ptemp->type[icomp], ptemp->list[icomp], width, length,
				&numlines, &rlines) ) continue;

		/* Display the pattern as a sequence of closed areas */
		if ( same(ptemp->type[icomp], "area") )
			{

			/* Create a list of closed areas to display */
			for ( il=0; il<numlines; il++ )
				{

				/* Clip areas (if requested), adjust them based on current */
				/*  anchor and offsets, and append them to the output list */
				if ( clip_to_map )
					{
					tline = GRA_clip_outline(rlines[il]);
					(void) anchored_line(tline, xoff, yoff, &xline);
					olines = GETMEM(olines, LINE, ++num_ol);
					olines[num_ol-1] = copy_line(xline);
					}

				/* Adjust all other areas based on current anchor and */
				/*  offsets, and append them to the output list       */
				else
					{
					(void) anchored_line(rlines[il], xoff, yoff, &xline);
					olines = GETMEM(olines, LINE, ++num_ol);
					olines[num_ol-1] = copy_line(xline);
					}
				}

			/* Display the list of closed areas */
			(void) write_graphics_outlines(num_ol, olines, TRUE, TRUE);
			}

		/* Display the pattern as one contiguous line */
		else if ( same(ptemp->type[icomp], "curve") && ptemp->contig[icomp])
			{

			/* Append sequence of pattern lines to form complete line */
			tline = create_line();
			for ( il=0; il<numlines; il++ )
				{
				tline = append_line(tline, rlines[il]);
				}

			/* Clip the complete line (if requested), adjust the clipped */
			/*  segments based on current anchor and offsets, and append */
			/*  them to the output list                                  */
			if ( clip_to_map )
				{
				nl = GRA_clip_line(tline, &tlines);
				(void) destroy_line(tline);
				for ( iil=0; iil<nl; iil++ )
					{
					(void) anchored_line(tlines[iil], xoff, yoff, &xline);
					olines = GETMEM(olines, LINE, ++num_ol);
					olines[num_ol-1] = copy_line(xline);
					}
				}

			/* Adjust all other complete lines based on current anchor */
			/*  and offsets, and append them to the output list        */
			else
				{
				(void) anchored_line(tline, xoff, yoff, &xline);
				(void) destroy_line(tline);
				olines = GETMEM(olines, LINE, ++num_ol);
				olines[num_ol-1] = copy_line(xline);
				}

			/* Display the list of lines */
			(void) write_graphics_lines(num_ol, olines);
			}

		/* Display the pattern as a sequence of lines */
		else if ( same(ptemp->type[icomp], "curve") && !ptemp->contig[icomp])
			{

			/* Create a list of lines to display */
			for ( il=0; il<numlines; il++ )
				{

				/* Clip lines (if requested), adjust them based on current */
				/*  anchor and offsets, and append them to the output list */
				if ( clip_to_map )
					{
					nl = GRA_clip_line(rlines[il], &tlines);
					for ( iil=0; iil<nl; iil++ )
						{
						(void) anchored_line(tlines[iil], xoff, yoff, &xline);
						olines = GETMEM(olines, LINE, ++num_ol);
						olines[num_ol-1] = copy_line(xline);
						}
					}

				/* Adjust all other lines based on current anchor and */
				/*  offsets, and append them to the output list       */
				else
					{
					(void) anchored_line(rlines[il], xoff, yoff, &xline);
					olines = GETMEM(olines, LINE, ++num_ol);
					olines[num_ol-1] = copy_line(xline);
					}
				}

			/* Display the list of lines */
			(void) write_graphics_lines(num_ol, olines);
			}

		/* Empty the list of output lines */
		if ( num_ol > 0 )
			{
			for ( il=0; il<num_ol; il++ )
				{
				(void) destroy_line(olines[il]);
				}
			FREEMEM(olines);
			num_ol = 0;
			}

		/* Process the next component of the pattern */
		}

	/* Restore the original presentation */
	(void) copy_presentation(&CurPres, &temp_pres);

	/* Return TRUE */
	return TRUE;
	}

static	LOGICAL			GRA_display_patterned_outline

	(
	LINE		line,			/* Outline to display */
	float		xoff,			/* x offset of outline (display units) */
	float		yoff,			/* y offset of outline (display units) */
	PRES		cur_pres,		/* Presentation parameters */
	LOGICAL		clip_to_map,	/* Clip outline to current map? */
	HAND		sense			/* Line handedness */
	)

	{
	int			icomp, numlines, il, nl, iil;
	float		width, ampl, length;
	PATTERN		*ptemp;
	LINE		tline, cline, xline;
	LINE		*rlines = NullLineList, *tlines = NullLineList;
	PRES		temp_pres;

	/* >>> this is temporary ... mostly for "area" type patterns <<< */
	PATTERN		*ptemp_base;
	LOGICAL		status_ok;
	/* >>> this is temporary ... mostly for "area" type patterns <<< */

	static	int		num_ol  = 0;
	static	LINE	*olines = NullLineList;

	/* Check that outline exists */
	if ( IsNull(line) ) return TRUE;
	if ( line->numpts < 2 ) return TRUE;

	/* Check that a pattern is given */
	if ( blank(cur_pres.pattern) )
		{
		if ( Verbose )
			{
			(void) fprintf(stdout, " No pattern given\n");
			}
		return FALSE;
		}

	/* Set the pattern amplitude and length */
	(void) sscanf(cur_pres.pattern_width, "%f", &width);
	width = fabs(width);
	ampl  = ( sense == Left )? -width: width;
	(void) sscanf(cur_pres.pattern_length, "%f", &length);
	length = fabs(length);

	/* Get the requested pattern ... scaled to the amplitude and length */
	ptemp = get_pattern(cur_pres.pattern, ampl, length);
	if ( IsNull(ptemp) )
		{
		if ( Verbose )
			{
			(void) fprintf(stdout,
				" No pattern file for ... %s\n", cur_pres.pattern);
			}
		return FALSE;
		}

	/* Save the current presentation */
	(void) copy_presentation(&temp_pres, &CurPres);

	/* Set presentation for displaying the interior of the outline */
	(void) copy_presentation(&CurPres, &cur_pres);

	/* Display the interior of the outline by replicating the inner edge */
	/*  of the pattern around the entire outline                         */
	/* >>> below is a stub until "inner edges" of patterns are defined <<< */
	/* >>> stub code for single "curve" type patterns <<< */
	status_ok = TRUE;
	if ( ptemp->num == 1 && same(ptemp->type[0], "curve") )
		{

		/* Replicate the pattern along the line */
		if ( !GRA_replicate_pattern(line, ptemp->type[0], ptemp->list[0],
				width, length, &numlines, &rlines) ) status_ok = FALSE;
		}
	/* >>> stub code for multiple "curve" and "area" type patterns <<< */
	else
		{
		ptemp_base = get_default_baseline_pattern(ampl, length);
		if ( IsNull(ptemp_base) || ptemp_base->num != 1
				|| !same(ptemp_base->type[0], "curve") )
			{
			(void) fprintf(stdout, " Problem with baseline pattern file ...");
			(void) fprintf(stdout, " $FPA/config/patterns/default_baseline\n");
			status_ok = FALSE;
			}
		else
			{

			/* Replicate the pattern along the line */
			if ( !GRA_replicate_pattern(line, ptemp_base->type[0],
					ptemp_base->list[0], width, length, &numlines, &rlines) )
				status_ok = FALSE;
			}
		}
	if ( status_ok )
		{

		/* The pattern is returned as a sequence of lines     */
		/*  ... which are appended to form a complete outline */
		tline = create_line();
		for ( il=0; il<numlines; il++ )
			{
			tline = append_line(tline, rlines[il]);
			}

		/* Clip the outline (if requested),                   */
		/*  and adjust it based on current anchor and offsets */
		if ( clip_to_map )
			{
			cline = GRA_clip_outline(tline);
			(void) destroy_line(tline);
			(void) anchored_line(cline, xoff, yoff, &xline);
			}

		/* Adjust all other outlines based on current anchor and offsets */
		else
			{
			(void) anchored_line(tline, xoff, yoff, &xline);
			(void) destroy_line(tline);
			}

		/* Display the "filled" interior of the outline */
		(void) write_graphics_outlines(1, &xline, FALSE, TRUE);

		/* Display the interior of the outline filled with symbols */
		if ( !blank(cur_pres.sym_fill_name) )
			{
			if ( !GRA_display_outline_symbol_fill(xline,
					cur_pres.sym_fill_name) ) return FALSE;
			}
		}
	/* >>> above is a stub until "inner edges" of patterns are defined <<< */

	/* Display the edge of the outline by replicating each component */
	/*  of the pattern around the entire outline                     */
	for ( icomp=0; icomp<ptemp->num; icomp++ )
		{

		/* Set the default presentation if no component presentations used */
		if ( cur_pres.num_comp == 0 )
			{
			(void) copy_presentation(&CurPres, &cur_pres);
			}

		/* Match the presentation with the appropriate component presentation */
		else if ( icomp < cur_pres.num_comp )
			{
			(void) copy_presentation(&CurPres, &cur_pres);
			(void) reset_presentation_by_comp_pres(&CurPres,
													&cur_pres.comp_pres[icomp]);
			}

		/* Use last component presentation if not enough given! */
		else
			{
			(void) copy_presentation(&CurPres, &cur_pres);
			(void) reset_presentation_by_comp_pres(&CurPres,
									&cur_pres.comp_pres[cur_pres.num_comp-1]);
			}

		/* Use component "fill" as "interior_fill" for each pattern component */
		(void) strcpy(CurPres.interior_fill, CurPres.fill);

		/* Replicate the pattern along the line */
		if ( !GRA_replicate_pattern(line,
				ptemp->type[icomp], ptemp->list[icomp], width, length,
				&numlines, &rlines) ) continue;

		/* Display the pattern as a sequence of closed areas */
		if ( same(ptemp->type[icomp], "area") )
			{

			/* Create a list of closed areas to display */
			for ( il=0; il<numlines; il++ )
				{

				/* Clip areas (if requested), adjust them based on current */
				/*  anchor and offsets, and append them to the output list */
				if ( clip_to_map )
					{
					tline = GRA_clip_outline(rlines[il]);
					(void) anchored_line(tline, xoff, yoff, &xline);
					olines = GETMEM(olines, LINE, ++num_ol);
					olines[num_ol-1] = copy_line(xline);
					}

				/* Adjust all other areas based on current anchor and */
				/*  offsets, and append them to the output list       */
				else
					{
					(void) anchored_line(rlines[il], xoff, yoff, &xline);
					olines = GETMEM(olines, LINE, ++num_ol);
					olines[num_ol-1] = copy_line(xline);
					}
				}

			/* Display the list of closed areas */
			(void) write_graphics_outlines(num_ol, olines, TRUE, TRUE);
			}

		/* Display the pattern as one contiguous line */
		else if ( same(ptemp->type[icomp], "curve") && ptemp->contig[icomp])
			{

			/* Append sequence of pattern lines to form complete outline */
			tline = create_line();
			for ( il=0; il<numlines; il++ )
				{
				tline = append_line(tline, rlines[il]);
				}

			/* Clip the outline (if requested),                   */
			/*  and adjust it based on current anchor and offsets */
			if ( clip_to_map )
				{
				cline = GRA_clip_outline(tline);
				(void) destroy_line(tline);
				(void) anchored_line(cline, xoff, yoff, &xline);
				}

			/* Adjust all other outlines based on current anchor and offsets */
			else
				{
				(void) anchored_line(tline, xoff, yoff, &xline);
				(void) destroy_line(tline);
				}

			/* Display only the outline */
			(void) write_graphics_outlines(1, &xline, TRUE, FALSE);
			}

		/* Display the pattern as a sequence of lines */
		else if ( same(ptemp->type[icomp], "curve") && !ptemp->contig[icomp])
			{

			/* Create a list of lines to display */
			for ( il=0; il<numlines; il++ )
				{

				/* Clip lines (if requested), adjust them based on current */
				/*  anchor and offsets, and append them to the output list */
				if ( clip_to_map )
					{
					nl = GRA_clip_line(rlines[il], &tlines);
					for ( iil=0; iil<nl; iil++ )
						{
						(void) anchored_line(tlines[iil], xoff, yoff, &xline);
						olines = GETMEM(olines, LINE, ++num_ol);
						olines[num_ol-1] = copy_line(xline);
						}
					}

				/* Adjust all other lines based on current anchor and */
				/*  offsets, and append them to the output list       */
				else
					{
					(void) anchored_line(rlines[il], xoff, yoff, &xline);
					olines = GETMEM(olines, LINE, ++num_ol);
					olines[num_ol-1] = copy_line(xline);
					}
				}

			/* Display the list of lines */
			(void) write_graphics_lines(num_ol, olines);
			}

		/* Empty the list of output lines */
		if ( num_ol > 0 )
			{
			for ( il=0; il<num_ol; il++ )
				{
				(void) destroy_line(olines[il]);
				}
			FREEMEM(olines);
			num_ol = 0;
			}

		/* Process the next component of the pattern */
		}

	/* Restore the original presentation */
	(void) copy_presentation(&CurPres, &temp_pres);

	/* Return TRUE */
	return TRUE;
	}

static	LOGICAL			GRA_display_patterned_boundary

	(
	BOUND		bound,			/* Boundary to display */
	float		xoff,			/* x offset of boundary (display units) */
	float		yoff,			/* y offset of boundary (display units) */
	PRES		cur_pres,		/* Presentation parameters */
	LOGICAL		clip_to_map,	/* Clip boundary to current map? */
	HAND		sense,			/* Boundary handedness */
	LOGICAL		hole_pattern	/* Display pattern for holes too? */
	)

	{
	int			icomp, numlines, il, ih, nl, iil;
	float		width, ampl, length;
	PATTERN		*ptemp;
	LINE		bline, tline, xline, cline, bhole, thole, xhole;
	LINE		*rlines = NullLineList, *tlines = NullLineList;
	BOUND		tbound, cbound, xbound;

	PRES		temp_pres;

	/* >>> this is temporary ... mostly for "area" type patterns <<< */
	PATTERN		*ptemp_base;
	LOGICAL		status_ok, hstat_ok;
	/* >>> this is temporary ... mostly for "area" type patterns <<< */

	static	int		num_ol  = 0;
	static	LINE	*olines = NullLineList;

	/* Check that boundary exists */
	if ( IsNull(bound) ) return TRUE;
	if ( IsNull(bound->boundary) ) return TRUE;
	if ( bound->boundary->numpts < 2 ) return TRUE;
	bline = bound->boundary;

	/* Check that a pattern is given */
	if ( blank(cur_pres.pattern) )
		{
		if ( Verbose )
			{
			(void) fprintf(stdout, " No pattern given\n");
			}
		return FALSE;
		}

	/* Set the pattern amplitude and length */
	(void) sscanf(cur_pres.pattern_width, "%f", &width);
	width = fabs(width);
	ampl  = ( sense == Left )? -width: width;
	(void) sscanf(cur_pres.pattern_length, "%f", &length);
	length = fabs(length);

	/* Get the requested pattern ... scaled to the amplitude and length */
	ptemp = get_pattern(cur_pres.pattern, ampl, length);
	if ( IsNull(ptemp) )
		{
		if ( Verbose )
			{
			(void) fprintf(stdout,
				" No pattern file for ... %s\n", cur_pres.pattern);
			}
		return FALSE;
		}

	/* Save the current presentation */
	(void) copy_presentation(&temp_pres, &CurPres);

	/* Set presentation for displaying the interior of the boundary */
	(void) copy_presentation(&CurPres, &cur_pres);

	/* Display the interior of the boundary by replicating the inner edge */
	/*  of the pattern around the entire boundary                         */
	/* >>> below is a stub until "inner edges" of patterns are defined <<< */
	/* >>> stub code for single "curve" type patterns <<< */

	/* First replicate the boundary itself */
	status_ok = TRUE;
	if ( ptemp->num == 1 && same(ptemp->type[0], "curve") )
		{

		/* Replicate the pattern along the boundary */
		if ( !GRA_replicate_pattern(bline, ptemp->type[0], ptemp->list[0],
				width, length, &numlines, &rlines) )
			status_ok = FALSE;
		}
	/* >>> stub code for multiple "curve" and "area" type patterns <<< */
	else
		{
		ptemp_base = get_default_baseline_pattern(ampl, length);
		if ( IsNull(ptemp_base) || ptemp_base->num != 1
				|| !same(ptemp_base->type[0], "curve") )
			{
			(void) fprintf(stdout, " Problem with baseline pattern file ...");
			(void) fprintf(stdout, " $FPA/config/patterns/default_baseline\n");
			status_ok = FALSE;
			}
		else
			{

			/* Replicate the pattern along the boundary */
			if ( !GRA_replicate_pattern(bline, ptemp_base->type[0],
					ptemp_base->list[0], width, length, &numlines, &rlines) )
				status_ok = FALSE;
			}
		}
	if ( status_ok )
		{

		/* The pattern is returned as a sequence of lines      */
		/*  ... which are appended to form a complete boundary */
		tline = create_line();
		for ( il=0; il<numlines; il++ )
			{
			tline = append_line(tline, rlines[il]);
			}

		/* Rebuild the boundary with the patterned line */
		tbound = create_bound();
		define_bound_boundary(tbound, tline);
		}

	/* Then replicate the holes (if requested) */
	if (status_ok && hole_pattern)
		{
		for ( ih=0; ih<bound->numhole; ih++ )
			{
			bhole = bound->holes[ih];
			hstat_ok = TRUE;
			if ( ptemp->num == 1 && same(ptemp->type[0], "curve") )
				{

				/* Replicate the pattern along the hole */
				if ( !GRA_replicate_pattern(bhole, ptemp->type[0],
						ptemp->list[0], width, length, &numlines, &rlines) )
					hstat_ok = FALSE;
				}
			/* >>> stub code for multiple "curve" / "area" type patterns <<< */
			else
				{
				ptemp_base = get_default_baseline_pattern(ampl, length);
				if ( IsNull(ptemp_base) || ptemp_base->num != 1
						|| !same(ptemp_base->type[0], "curve") )
					{
					(void) fprintf(stdout,
						" Problem with baseline pattern file ...");
					(void) fprintf(stdout,
						" $FPA/config/patterns/default_baseline\n");
					hstat_ok = FALSE;
					}
				else
					{

					/* Replicate the pattern along the hole */
					if ( !GRA_replicate_pattern(bhole, ptemp_base->type[0],
							ptemp_base->list[0], width, length,
							&numlines, &rlines) )
						hstat_ok = FALSE;
					}
				}
			if ( hstat_ok )
				{

				/* The pattern is returned as a sequence of lines  */
				/*  ... which are appended to form a complete hole */
				thole = create_line();
				for ( il=0; il<numlines; il++ )
					{
					thole = append_line(thole, rlines[il]);
					}

				/* Add the patterned hole to the boundary */
				add_bound_hole(tbound, thole);
				}
			}
		}

	/* Otherwise, add the holes */
	else if (status_ok)
		{
		for ( ih=0; ih<bound->numhole; ih++ )
			add_bound_hole(tbound, copy_line(bound->holes[ih]));
		}

	/* Display the interior of the boundary (with holes) */
	if ( status_ok )
		{

		/* Clip the boundary (if requested),                  */
		/*  and adjust it based on current anchor and offsets */
		if ( clip_to_map )
			{
			cbound = GRA_clip_boundary(tbound);
			(void) destroy_bound(tbound);
			(void) anchored_boundary(cbound, xoff, yoff, &xbound);
			}

		/* Adjust all other boundaries based on current anchor and offsets */
		else
			{
			(void) anchored_boundary(tbound, xoff, yoff, &xbound);
			(void) destroy_bound(tbound);
			}

		/* Display the "filled" interior of the boundary */
		(void) write_graphics_boundaries(1, &xbound, FALSE, TRUE);

		/* Display the interior of the boundary filled with symbols */
		if ( !blank(cur_pres.sym_fill_name) )
			{
			if ( !GRA_display_boundary_symbol_fill(xbound,
					cur_pres.sym_fill_name) ) return FALSE;
			}
		}
	/* >>> above is a stub until "inner edges" of patterns are defined <<< */

	/* Display the edge of the boundary by replicating each component */
	/*  of the pattern around the entire boundary                     */
	for ( icomp=0; icomp<ptemp->num; icomp++ )
		{

		/* Set the default presentation if no component presentations used */
		if ( cur_pres.num_comp == 0 )
			{
			(void) copy_presentation(&CurPres, &cur_pres);
			}

		/* Match the presentation with the appropriate component presentation */
		else if ( icomp < cur_pres.num_comp )
			{
			(void) copy_presentation(&CurPres, &cur_pres);
			(void) reset_presentation_by_comp_pres(&CurPres,
													&cur_pres.comp_pres[icomp]);
			}

		/* Use last component presentation if not enough given! */
		else
			{
			(void) copy_presentation(&CurPres, &cur_pres);
			(void) reset_presentation_by_comp_pres(&CurPres,
									&cur_pres.comp_pres[cur_pres.num_comp-1]);
			}

		/* Use component "fill" as "interior_fill" for each pattern component */
		(void) strcpy(CurPres.interior_fill, CurPres.fill);

		/* Replicate the pattern along the boundary */
		if ( !GRA_replicate_pattern(bline,
				ptemp->type[icomp], ptemp->list[icomp], width, length,
				&numlines, &rlines) ) continue;

		/* Display the pattern as a sequence of closed areas */
		if ( same(ptemp->type[icomp], "area") )
			{

			/* Create a list of closed areas to display */
			for ( il=0; il<numlines; il++ )
				{

				/* Clip areas (if requested), adjust them based on current */
				/*  anchor and offsets, and append them to the output list */
				if ( clip_to_map )
					{
					tline = GRA_clip_outline(rlines[il]);
					(void) anchored_line(tline, xoff, yoff, &xline);
					olines = GETMEM(olines, LINE, ++num_ol);
					olines[num_ol-1] = copy_line(xline);
					}

				/* Adjust all other areas based on current anchor and */
				/*  offsets, and append them to the output list       */
				else
					{
					(void) anchored_line(rlines[il], xoff, yoff, &xline);
					olines = GETMEM(olines, LINE, ++num_ol);
					olines[num_ol-1] = copy_line(xline);
					}
				}

			/* Display the list of closed areas */
			(void) write_graphics_outlines(num_ol, olines, TRUE, TRUE);
			}

		/* Display the pattern as one contiguous line */
		else if ( same(ptemp->type[icomp], "curve") && ptemp->contig[icomp])
			{

			/* Append sequence of pattern lines to form complete boundary */
			tline = create_line();
			for ( il=0; il<numlines; il++ )
				{
				tline = append_line(tline, rlines[il]);
				}

			/* Clip the boundary (if requested),                  */
			/*  and adjust it based on current anchor and offsets */
			if ( clip_to_map )
				{
				cline = GRA_clip_outline(tline);
				(void) destroy_line(tline);
				(void) anchored_line(cline, xoff, yoff, &xline);
				}

			/* Adjust other boundaries based on current anchor and offsets */
			else
				{
				(void) anchored_line(tline, xoff, yoff, &xline);
				(void) destroy_line(tline);
				}

			/* Display only the boundary */
			(void) write_graphics_outlines(1, &xline, TRUE, FALSE);
			}

		/* Display the pattern as a sequence of lines */
		else if ( same(ptemp->type[icomp], "curve") && !ptemp->contig[icomp])
			{

			/* Create a list of lines to display */
			for ( il=0; il<numlines; il++ )
				{

				/* Clip boundaries (if requested), adjust based on current */
				/*  anchor and offsets, and append them to the output list */
				if ( clip_to_map )
					{
					nl = GRA_clip_line(rlines[il], &tlines);
					for ( iil=0; iil<nl; iil++ )
						{
						(void) anchored_line(tlines[iil], xoff, yoff, &xline);
						olines = GETMEM(olines, LINE, ++num_ol);
						olines[num_ol-1] = copy_line(xline);
						}
					}

				/* Adjust all other boundaries based on current anchor */
				/*  and offsets, and append them to the output list    */
				else
					{
					(void) anchored_line(rlines[il], xoff, yoff, &xline);
					olines = GETMEM(olines, LINE, ++num_ol);
					olines[num_ol-1] = copy_line(xline);
					}
				}

			/* Display the list of boundaries */
			(void) write_graphics_lines(num_ol, olines);
			}

		/* Empty the list of output lines */
		if ( num_ol > 0 )
			{
			for ( il=0; il<num_ol; il++ )
				{
				(void) destroy_line(olines[il]);
				}
			FREEMEM(olines);
			num_ol = 0;
			}

		/* Process the next component of the pattern */
		}

	/* Reset presentation for displaying the holes */
	(void) copy_presentation(&CurPres, &cur_pres);

	/* Display each hole with a patterned line */
	if (hole_pattern)
		{
		for ( ih=0; ih<bound->numhole; ih++ )
			{
			bhole = bound->holes[ih];

			/* Display the edge of each hole by replicating each component */
			/*  of the pattern around the hole                             */
			for ( icomp=0; icomp<ptemp->num; icomp++ )
				{

				/* Set default presentation if no component presentations */
				if ( cur_pres.num_comp == 0 )
					{
					(void) copy_presentation(&CurPres, &cur_pres);
					}

				/* Match presentation with appropriate component presentation */
				else if ( icomp < cur_pres.num_comp )
					{
					(void) copy_presentation(&CurPres, &cur_pres);
					(void) reset_presentation_by_comp_pres(&CurPres,
													&cur_pres.comp_pres[icomp]);
					}

				/* Use last component presentation if not enough given! */
				else
					{
					(void) copy_presentation(&CurPres, &cur_pres);
					(void) reset_presentation_by_comp_pres(&CurPres,
									&cur_pres.comp_pres[cur_pres.num_comp-1]);
					}

				/* Use component "fill" as "interior_fill" for each component */
				(void) strcpy(CurPres.interior_fill, CurPres.fill);

				/* Replicate the pattern along the hole */
				if ( !GRA_replicate_pattern(bhole,
						ptemp->type[icomp], ptemp->list[icomp], width, length,
						&numlines, &rlines) ) continue;

				/* Display the pattern as a sequence of closed areas */
				if ( same(ptemp->type[icomp], "area") )
					{

					/* Create a list of closed areas to display */
					for ( il=0; il<numlines; il++ )
						{

						/* Clip areas (if requested), adjust based on current */
						/*  anchor and offsets, and append to the output list */
						if ( clip_to_map )
							{
							tline = GRA_clip_outline(rlines[il]);
							(void) anchored_line(tline, xoff, yoff, &xline);
							olines = GETMEM(olines, LINE, ++num_ol);
							olines[num_ol-1] = copy_line(xline);
							}

						/* Adjust all other areas based on current anchor and */
						/*  offsets, and append them to the output list       */
						else
							{
							(void) anchored_line(rlines[il], xoff, yoff,
																		&xline);
							olines = GETMEM(olines, LINE, ++num_ol);
							olines[num_ol-1] = copy_line(xline);
							}
						}

					/* Display the list of closed areas */
					(void) write_graphics_outlines(num_ol, olines, TRUE, TRUE);
					}

				/* Display the pattern as one contiguous line */
				else if ( same(ptemp->type[icomp], "curve")
							&& ptemp->contig[icomp])
					{

					/* Append sequence of pattern lines to form complete hole */
					tline = create_line();
					for ( il=0; il<numlines; il++ )
						{
						tline = append_line(tline, rlines[il]);
						}

					/* Clip the hole (if requested),                      */
					/*  and adjust it based on current anchor and offsets */
					if ( clip_to_map )
						{
						cline = GRA_clip_outline(tline);
						(void) destroy_line(tline);
						(void) anchored_line(cline, xoff, yoff, &xline);
						}

					/* Adjust other holes based on current anchor and offsets */
					else
						{
						(void) anchored_line(tline, xoff, yoff, &xline);
						(void) destroy_line(tline);
						}

					/* Display only the hole */
					(void) write_graphics_outlines(1, &xline, TRUE, FALSE);
					}

				/* Display the pattern as a sequence of lines */
				else if ( same(ptemp->type[icomp], "curve")
							&& !ptemp->contig[icomp])
					{

					/* Create a list of lines to display */
					for ( il=0; il<numlines; il++ )
						{

						/* Clip holes (if requested), adjust based on current */
						/*  anchor and offsets, and append to the output list */
						if ( clip_to_map )
							{
							nl = GRA_clip_line(rlines[il], &tlines);
							for ( iil=0; iil<nl; iil++ )
								{
								(void) anchored_line(tlines[iil], xoff, yoff,
																		&xline);
								olines = GETMEM(olines, LINE, ++num_ol);
								olines[num_ol-1] = copy_line(xline);
								}
							}

						/* Adjust all other holes based on current anchor   */
						/*  and offsets, and append them to the output list */
						else
							{
							(void) anchored_line(rlines[il], xoff, yoff,
																		&xline);
							olines = GETMEM(olines, LINE, ++num_ol);
							olines[num_ol-1] = copy_line(xline);
							}
						}

					/* Display the list of hole segments */
					(void) write_graphics_lines(num_ol, olines);
					}

				/* Empty the list of output lines */
				if ( num_ol > 0 )
					{
					for ( il=0; il<num_ol; il++ )
						{
						(void) destroy_line(olines[il]);
						}
					FREEMEM(olines);
					num_ol = 0;
					}

				/* Process the next component of the pattern */
				}
			}
		}

	/* Otherwise, display each hole with a simple line */
	else
		{

		/* Create a list of holes to display */
		for ( ih=0; ih<bound->numhole; ih++ )
			{
			bhole = bound->holes[ih];

			/* Clip the hole (if requested),                      */
			/*  and adjust it based on current anchor and offsets */
			if ( clip_to_map )
				{
				thole = GRA_clip_outline(bhole);
				(void) anchored_line(thole, xoff, yoff, &xhole);
				}

			/* Adjust hole based on current anchor and offsets */
			else
				{
				(void) anchored_line(bhole, xoff, yoff, &xhole);
				}

			/* Display the hole */
			(void) write_graphics_outlines(1, &xhole, TRUE, FALSE);
			}
		}

	/* Restore the original presentation */
	(void) copy_presentation(&CurPres, &temp_pres);

	/* Return TRUE */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    G R A _ d i s p l a y _ c o n t o u r _ l i n e                   *
*    G R A _ d i s p l a y _ c o n t o u r _ a r e a                   *
*                                                                      *
***********************************************************************/

static	LOGICAL		GRA_display_contour_line

	(
	SURFACE		sfc,			/* Surface to contour */
	float		fval,			/* Value to contour */
	STRING		units,			/* Units for contour */
	STRING		pattern,		/* Name of pattern for contour */
	STRING		pattern_width,	/* Pattern width factor */
	STRING		pattern_length,	/* Pattern repetition factor */
	COMP_PRES	*comp_pres,		/* Structure containing presentations */
	int			num_comp,		/* Number of presentations */
	LOGICAL		clip_to_map		/* Clip contour to current map? */
	)

	{
	int			ilist;
	STRING		stype;
	double		dval;
	PRES		cur_pres;

	SET			set;
	CURVE		curve;
	LOGICAL		closed, clockwise;
	HAND		sense;

	char		out_buf[GPGLong];

	/* Obtain contours of the current value */
	(void) convert_value(units, (double) fval, sfc->units.name, &dval);
	set = contour_curveset(sfc, (float) dval, NullPtr(USPEC *));
	if ( IsNull(set) ) return TRUE;
	(void) recall_set_type(set, &stype);
	if ( set->num <= 0 || !same(stype, "curve") )
		{
		(void) destroy_set(set);
		return TRUE;
		}

	(void) sprintf(out_buf, "### Begin %6.2f %s Contour ###", fval, units);
	(void) write_graphics_comment(out_buf);

	/* Ensure that curves have been reprojected to target map */
	if ( sfc->sp.origin[X] != ZeroPoint[X]
			|| sfc->sp.origin[Y] != ZeroPoint[Y]
			|| sfc->sp.orient != 0.0 )

		{
		(void) warn_report("Contact FPA Group ... problem with contours");
		return TRUE;
		}

	/* Now output the contours */
	for ( ilist=0; ilist<set->num; ilist++ )
		{

		/* Set contour presentation from list of presentations */
		cur_pres = get_contour_presentation(fval, units);

		/* Override pattern and pattern sizes (if requested) */
		if ( !blank(pattern) )
						(void) strcpy(cur_pres.pattern,        pattern);
		if ( !blank(pattern_width) )
						(void) strcpy(cur_pres.pattern_width,  pattern_width);
		if ( !blank(pattern_length) )
						(void) strcpy(cur_pres.pattern_length, pattern_length);

		/* Override component presentation (if requested) */
		if ( num_comp > 0 )
			{
			cur_pres.num_comp  = num_comp;
			cur_pres.comp_pres = comp_pres;
			(void) reset_presentation_by_comp_pres(&cur_pres,
													&cur_pres.comp_pres[0]);
			}

		/* Extract this curve */
		curve = (CURVE) set->list[ilist];
		if ( IsNull(curve) ) continue;
		if ( IsNull(curve->line) ) continue;
		if ( curve->line->numpts < 2 ) continue;

		/* Start grouping for contour line */
		(void) write_graphics_group(GPGstart, NullPointer, 0);

		/* Display contour as a "simple" line */
		if ( blank(cur_pres.pattern) || same(cur_pres.pattern, PatternSimple) )
			{

			/* Display the line */
			(void) GRA_display_simple_line(curve->line, 0.0, 0.0,
					cur_pres, clip_to_map);
			}

		/* Display contour as a line using the given pattern */
		else
			{

			/* Set the curve properties */
			(void) curve_properties(curve, &closed, &clockwise,
					NullFloat, NullFloat);
			if ( clockwise ) sense = Right;
			else             sense = Left;

			/* Display the line */
			(void) GRA_display_patterned_line(curve->line, 0.0, 0.0,
					cur_pres, clip_to_map, sense);
			}

		/* End grouping for contour line */
		(void) write_graphics_group(GPGend, NullPointer, 0);
		}

	(void) sprintf(out_buf, "### End %6.2f %s Contour ###", fval, units);
	(void) write_graphics_comment(out_buf);

	/* Free space used by work object and return TRUE */
	(void) destroy_set(set);
	return TRUE;
	}

static	LOGICAL		GRA_display_contour_area

	(
	SURFACE		sfc,			/* Surface to contour */
	float		fminv,			/* Minimum value for contour area */
	float		fmaxv,			/* Maximum value for contour area */
	STRING		units,			/* Units for contours */
	STRING		interior_fill,	/* Colour for interior of banded contours */
	STRING		sym_fill_name,	/* Symbol pattern for banded contours */
	STRING		pattern,		/* Name of pattern for contour */
	STRING		pattern_width,	/* Pattern width factor */
	STRING		pattern_length,	/* Pattern repetition factor */
	COMP_PRES	*comp_pres,		/* Structure containing presentations */
	int			num_comp,		/* Number of presentations */
	LOGICAL		clip_to_map		/* Clip banded countour to current map? */
	)

	{
	int			ilist;
	STRING		stype;
	double		valmin, valmax;
	PRES		cur_pres;

	SET			set;
	AREA		area;
	LOGICAL		clockwise;
	HAND		sense;

	char		out_buf[GPGLong];

	/* Obtain contour areas from the current values */
	(void) convert_value(units, (double) fminv, sfc->units.name, &valmin);
	(void) convert_value(units, (double) fmaxv, sfc->units.name, &valmax);
	set = contour_areaset(sfc, (float) valmin, (float) valmax,
			NullPtr(USPEC *), NullPtr(BOX *));
	if ( IsNull(set) ) return TRUE;
	(void) recall_set_type(set, &stype);
	if ( set->num <= 0 || !same(stype, "area") )
		{
		(void) destroy_set(set);
		return TRUE;
		}

	if ( sfc->sp.origin[X] != ZeroPoint[X]
			|| sfc->sp.origin[Y] != ZeroPoint[Y]
			|| sfc->sp.orient != 0.0 )

		{
		(void) warn_report("Contact FPA Group ... problem with contour areas");
		return TRUE;
		}

	(void) sprintf(out_buf, "### Begin %6.2f to %6.2f %s Contour Area ###",
			fminv, fmaxv, units);
	(void) write_graphics_comment(out_buf);

	/* Now output the areas */
	for ( ilist=0; ilist<set->num; ilist++ )
		{

		/* Set contour presentation from list of presentations */
		cur_pres = get_contour_presentation(fminv, units);

		/* Override interior_fill and symbol_fill_name (if requested) */
		if ( !blank(interior_fill) )
						(void) strcpy(cur_pres.interior_fill, interior_fill);
		if ( !blank(sym_fill_name) )
						(void) strcpy(cur_pres.sym_fill_name, sym_fill_name);

		/* Override pattern and pattern sizes (if requested) */
		if ( !blank(pattern) )
						(void) strcpy(cur_pres.pattern,        pattern);
		if ( !blank(pattern_width) )
						(void) strcpy(cur_pres.pattern_width,  pattern_width);
		if ( !blank(pattern_length) )
						(void) strcpy(cur_pres.pattern_length, pattern_length);

		/* Override component presentation (if requested) */
		if ( num_comp > 0 )
			{
			cur_pres.num_comp  = num_comp;
			cur_pres.comp_pres = comp_pres;
			(void) reset_presentation_by_comp_pres(&cur_pres,
													&cur_pres.comp_pres[0]);
			}

		/* Extract this area */
		area = (AREA) set->list[ilist];
		if ( IsNull(area) ) continue;
		if ( IsNull(area->bound) ) continue;
		if ( IsNull(area->bound->boundary) ) continue;
		if ( area->bound->boundary->numpts < 2 ) continue;

		/* Start grouping for contour area */
		(void) write_graphics_group(GPGstart, NullPointer, 0);

		/* Display contour area as a "simple" filled outline */
		if ( blank(cur_pres.pattern) || same(cur_pres.pattern, PatternSimple) )
			{

			/* Display the filled boundary */
			(void) GRA_display_simple_boundary(area->bound, 0.0, 0.0,
					cur_pres, clip_to_map);
			}

		/* Display contour area as a filled outline using the given pattern */
		else
			{

			/* Set the area outline properties */
			(void) area_properties(area, &clockwise, NullFloat, NullFloat);
			if ( clockwise ) sense = Right;
			else             sense = Left;

			/* Display the filled boundary */
			/* >>> replace below with GRA_display_patterned_boundary() <<< */
			(void) GRA_display_patterned_outline(area->bound->boundary,
					0.0, 0.0, cur_pres, clip_to_map, sense);
			/* >>> replace above with GRA_display_patterned_boundary() <<< */
			}

		/* End grouping for contour area */
		(void) write_graphics_group(GPGend, NullPointer, 0);
		}

	(void) sprintf(out_buf, "### End %6.2f to %6.2f %s Contour Area ###",
			fminv, fmaxv, units);
	(void) write_graphics_comment(out_buf);

	/* Free space used by work object and return TRUE */
	(void) destroy_set(set);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    G R A _ d i s p l a y _ x s e c t i o n _ c o n t o u r _ l i n e *
*    G R A _ d i s p l a y _ x s e c t i o n _ c o n t o u r _ a r e a *
*                                                                      *
***********************************************************************/

static	LOGICAL		GRA_display_xsection_contour_line

	(
	SURFACE			sfc,
	GRA_XSECT		*xsect,
	XSECT_HOR_AXIS	*haxis,
	XSECT_VER_AXIS	*vaxis,
	float			fval,
	STRING			units,
	STRING			pattern,		/* Name of pattern for contour */
	STRING			pattern_width,	/* Pattern width factor */
	STRING			pattern_length,	/* Pattern repetition factor */
	COMP_PRES		*comp_pres,		/* Structure containing presentations */
	int				num_comp,		/* Number of presentations */
	LOGICAL			clip_to_map		/* Clip contour to current map? */
	)

	{
	int			ilist;
	STRING		stype;
	double		dval;
	PRES		cur_pres;

	SET			set;
	CURVE		curve;
	LINE		line;
	LOGICAL		closed, clockwise;
	HAND		sense;

	char		out_buf[GPGLong];

	/* Obtain contours of the current value */
	(void) convert_value(units, (double) fval, sfc->units.name, &dval);
	set = contour_curveset(sfc, (float) dval, NullPtr(USPEC *));
	if ( IsNull(set) ) return TRUE;
	(void) recall_set_type(set, &stype);
	if ( set->num <= 0 || !same(stype, "curve") )
		{
		(void) destroy_set(set);
		return TRUE;
		}

	(void) sprintf(out_buf, "### Begin %6.2f %s Contour ###", fval, units);
	(void) write_graphics_comment(out_buf);

	/* Now output the contours */
	for ( ilist=0; ilist<set->num; ilist++ )
		{

		/* Set contour presentation from list of presentations */
		cur_pres = get_contour_presentation(fval, units);

		/* Override pattern and pattern sizes (if requested) */
		if ( !blank(pattern) )
						(void) strcpy(cur_pres.pattern,        pattern);
		if ( !blank(pattern_width) )
						(void) strcpy(cur_pres.pattern_width,  pattern_width);
		if ( !blank(pattern_length) )
						(void) strcpy(cur_pres.pattern_length, pattern_length);

		/* Override component presentation (if requested) */
		if ( num_comp > 0 )
			{
			cur_pres.num_comp  = num_comp;
			cur_pres.comp_pres = comp_pres;
			(void) reset_presentation_by_comp_pres(&cur_pres,
													&cur_pres.comp_pres[0]);
			}

		/* Extract this curve */
		curve = (CURVE) set->list[ilist];
		if ( IsNull(curve) ) continue;
		if ( IsNull(curve->line) ) continue;
		if ( curve->line->numpts < 2 ) continue;

		/* Set the curve properties */
		(void) curve_properties(curve, &closed, &clockwise,
				NullFloat, NullFloat);
		if ( clockwise ) sense = Right;
		else             sense = Left;

		/* Convert the curve line to cross section coordinates */
		line = xsection_line_coordinates(curve->line, xsect, haxis, vaxis);
		if ( IsNull(line) ) continue;
		if ( line->numpts < 2 ) continue;

		/* Start grouping for contour line */
		(void) write_graphics_group(GPGstart, NullPointer, 0);

		/* Display contour as a "simple" line */
		if ( blank(cur_pres.pattern) || same(cur_pres.pattern, PatternSimple) )
			{

			/* Display the line */
			(void) GRA_display_simple_line(line, 0.0, 0.0,
					cur_pres, clip_to_map);
			}

		/* Display contour as a line using the given pattern */
		else
			{

			/* Display the line */
			(void) GRA_display_patterned_line(line, 0.0, 0.0,
					cur_pres, clip_to_map, sense);
			}

		/* End grouping for contour line */
		(void) write_graphics_group(GPGend, NullPointer, 0);
		}

	(void) sprintf(out_buf, "### End %6.2f %s Contour ###", fval, units);
	(void) write_graphics_comment(out_buf);

	/* Free space used by work object and return TRUE */
	(void) destroy_set(set);
	return TRUE;
	}

static	LOGICAL		GRA_display_xsection_contour_area

	(
	SURFACE			sfc,
	GRA_XSECT		*xsect,
	XSECT_HOR_AXIS	*haxis,
	XSECT_VER_AXIS	*vaxis,
	float			fminv,
	float			fmaxv,
	STRING			units,
	STRING			interior_fill,	/* Colour for interior of areas */
	STRING			sym_fill_name,	/* Symbol pattern for interior of areas */
	STRING			pattern,		/* Name of pattern for lines */
	STRING			pattern_width,	/* Pattern width factor */
	STRING			pattern_length,	/* Pattern repetition factor */
	COMP_PRES		*comp_pres,		/* Structure containing presentations */
	int				num_comp,		/* Number of presentations */
	LOGICAL			clip_to_map		/* Clip area to current map? */
	)

	{
	int			ilist;
	STRING		stype;
	double		valmin, valmax;
	PRES		cur_pres;

	SET			set;
	AREA		area;
	BOUND		bound;
	LOGICAL		clockwise;
	HAND		sense;

	char		out_buf[GPGLong];

	/* Obtain contour areas from the current values */
	(void) convert_value(units, (double) fminv, sfc->units.name, &valmin);
	(void) convert_value(units, (double) fmaxv, sfc->units.name, &valmax);
	set = contour_areaset(sfc, (float) valmin, (float) valmax,
			NullPtr(USPEC *), NullPtr(BOX *));
	if ( IsNull(set) ) return TRUE;
	(void) recall_set_type(set, &stype);
	if ( set->num <= 0 || !same(stype, "area") )
		{
		(void) destroy_set(set);
		return TRUE;
		}

	(void) sprintf(out_buf, "### Begin %6.2f to %6.2f %s Contour Area ###",
			fminv, fmaxv, units);
	(void) write_graphics_comment(out_buf);

	/* Now output the areas */
	for ( ilist=0; ilist<set->num; ilist++ )
		{

		/* Set contour presentation from list of presentations */
		cur_pres = get_contour_presentation(fminv, units);

		/* Override interior_fill and symbol_fill_name (if requested) */
		if ( !blank(interior_fill) )
						(void) strcpy(cur_pres.interior_fill, interior_fill);
		if ( !blank(sym_fill_name) )
						(void) strcpy(cur_pres.sym_fill_name, sym_fill_name);

		/* Override pattern and pattern sizes (if requested) */
		if ( !blank(pattern) )
						(void) strcpy(cur_pres.pattern,        pattern);
		if ( !blank(pattern_width) )
						(void) strcpy(cur_pres.pattern_width,  pattern_width);
		if ( !blank(pattern_length) )
						(void) strcpy(cur_pres.pattern_length, pattern_length);

		/* Override component presentation (if requested) */
		if ( num_comp > 0 )
			{
			cur_pres.num_comp  = num_comp;
			cur_pres.comp_pres = comp_pres;
			(void) reset_presentation_by_comp_pres(&cur_pres,
													&cur_pres.comp_pres[0]);
			}

		/* Extract this area */
		area = (AREA) set->list[ilist];
		if ( IsNull(area) ) continue;
		if ( IsNull(area->bound) ) continue;
		if ( IsNull(area->bound->boundary) ) continue;
		if ( area->bound->boundary->numpts < 2 ) continue;

		/* Convert the bound to cross section coordinates */
		bound = xsection_boundary_coordinates(area->bound, xsect, haxis, vaxis);
		if ( IsNull(bound) ) continue;
		if ( IsNull(bound->boundary) ) continue;
		if ( bound->boundary->numpts < 2 ) continue;

		/* Start grouping for contour area */
		(void) write_graphics_group(GPGstart, NullPointer, 0);

		/* Display contour area as a "simple" filled outline */
		if ( blank(cur_pres.pattern) || same(cur_pres.pattern, PatternSimple) )
			{

			/* Display the filled boundary */
			(void) GRA_display_simple_boundary(bound, 0.0, 0.0,
					cur_pres, clip_to_map);
			}

		/* Display contour area as a filled outline using the given pattern */
		else
			{

			/* Set the area outline properties */
			(void) area_properties(area, &clockwise, NullFloat, NullFloat);
			if ( clockwise ) sense = Right;
			else             sense = Left;

			/* Display the filled boundary */
			/* >>> replace below with GRA_display_patterned_boundary() <<< */
			(void) GRA_display_patterned_outline(bound->boundary,
					0.0, 0.0, cur_pres, clip_to_map, sense);
			/* >>> replace above with GRA_display_patterned_boundary() <<< */
			}

		/* End grouping for contour area */
		(void) write_graphics_group(GPGend, NullPointer, 0);
		}

	(void) sprintf(out_buf, "### End %6.2f to %6.2f %s Contour Area ###",
			fminv, fmaxv, units);
	(void) write_graphics_comment(out_buf);

	/* Free space used by work object and return TRUE */
	(void) destroy_set(set);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    G R A _ d i s p l a y _ b o x _ s y m b o l _ f i l l             *
*    G R A _ d i s p l a y _ e l l i p s e _ s y m b o l _ f i l l     *
*    G R A _ d i s p l a y _ o u t l i n e _ s y m b o l _ f i l l     *
*    G R A _ d i s p l a y _ b o u n d a r y _ s y m b o l _ f i l l   *
*                                                                      *
***********************************************************************/

static	LOGICAL		GRA_display_box_symbol_fill

	(
	float		xx,				/* x offset of center of box (display units) */
	float		yy,				/* y offset of center of box (display units) */
	float		width,			/* Width of box (display units) */
	float		height,			/* Height of box (display units) */
	float		rotation,		/* Box rotation (degrees) */
	STRING		sym_fill_name	/* Symbol pattern for interior of box */
	)

	{
	POINT			cpos, llpos, lrpos, urpos, ulpos;

	static	LINE	bline = NullLine;

	/* Return immediately for box with zero width or height */
	if ( width <= 0.0 || height <= 0.0 ) return TRUE;

	/* Create line to hold box outline */
	if ( IsNull(bline) ) bline = create_line();
	else                 empty_line(bline);

	/* Set position of center of box */
	cpos[X]  = xx;
	cpos[Y]  = yy;

	/* Determine position of box corners wrt center of box (unrotated) */
	llpos[X] = xx -  width / 2.0;
	llpos[Y] = yy - height / 2.0;
	lrpos[X] = xx +  width / 2.0;
	lrpos[Y] = yy - height / 2.0;
	urpos[X] = xx +  width / 2.0;
	urpos[Y] = yy + height / 2.0;
	ulpos[X] = xx -  width / 2.0;
	ulpos[Y] = yy + height / 2.0;

	/* Turn the unrotated box into a simple closed outline */
	add_point_to_line(bline, llpos);
	add_point_to_line(bline, lrpos);
	add_point_to_line(bline, urpos);
	add_point_to_line(bline, ulpos);
	close_line(bline);

	/* Rotate the box outline about the center of the box */
	(void) rotate_line(bline, cpos, rotation);

	/* Symbol fill the rotated box outline */
	return GRA_display_outline_symbol_fill(bline, sym_fill_name);
	}

static	LOGICAL		GRA_display_ellipse_symbol_fill

	(
	float		xx,				/* x offset of center of ellipse */
								/*  (display units)              */
	float		yy,				/* y offset of center of ellipse */
								/*  (display units)              */
	float		width,			/* Width of ellipse (display units) */
	float		height,			/* Height of ellipse (display units) */
	float		sangle,			/* Start angle of ellipse (degrees) */
	float		eangle,			/* End angle of ellipse (degrees) */
	float		rotation,		/* Ellipse rotation (degrees) */
	STRING		sym_fill_name	/* Symbol pattern for interior of ellipse */
	)

	{
	LOGICAL			partial;
	float			ang;
	POINT			cpos, pos;

	static	LINE	eline = NullLine;

	/* Return immediately for ellipse with zero width or height */
	if ( width <= 0.0 || height <= 0.0 ) return TRUE;

	/* Create line to hold ellipse outline */
	if ( IsNull(eline) ) eline = create_line();
	else                 empty_line(eline);

	/* Set position of center of ellipse */
	cpos[X] = xx;
	cpos[Y] = yy;

	/* Start the outline at the center for partial ellipses */
	partial = (sangle != eangle)? TRUE: FALSE;
	if ( partial ) add_point_to_line(eline, cpos);

	/* Determine positions wrt center of ellipse (unrotated) */
	/*  and add them to a simple closed outline              */
	if ( sangle >= eangle ) eangle += 360.0;
	ang = sangle;
	while ( ang <= eangle )
		{
		pos[X]  = xx +  width / 2.0 * fpa_cosdeg(ang);
		pos[Y]  = yy + height / 2.0 * fpa_sindeg(ang);
		add_point_to_line(eline, pos);
		ang += 1.0;
		}

	/* End the outline at the center for partial ellipses */
	if ( partial ) add_point_to_line(eline, cpos);

	/* Ensure that the outline is closed */
	close_line(eline);

	/* Rotate the ellipse outline about the center of the ellipse */
	(void) rotate_line(eline, cpos, rotation);

	/* Symbol fill the rotated ellipse outline */
	return GRA_display_outline_symbol_fill(eline, sym_fill_name);
	}

static	LOGICAL		GRA_display_outline_symbol_fill

	(
	LINE		outline,		/* Outline to display */
	STRING		sym_fill_name	/* Symbol pattern for interior of outline */
	)

	{
	int				ipts, minrow, maxrow, mincol, maxcol, irow, icol;
	float			rmin1, rmin2, rmax1, rmax2;
	float			cmin1, cmin2, cmax1, cmax2;
	float			sscale, pscale, swidth, sheight, maxsize;
	float			xmin, xmax, ymin, ymax, xcen, ycen, xx, yy;
	float			xmind, xmaxd, ymind, ymaxd, denom, dist;
	POINT			mcen, pos, ppos;
	LOGICAL			inside;
	PRES			temp_pres;
	SYMBOL_FILL		sym_fill;

	/* Check that line exists */
	if ( IsNull(outline) ) return TRUE;
	if ( outline->numpts < 2 ) return TRUE;

	/* Check for acceptable symbol fill name */
	if ( blank(sym_fill_name)
			|| same(sym_fill_name, SymbolFillNone) ) return TRUE;

	/* Get the named symbol fill parameters */
	sym_fill = get_symbol_fill(sym_fill_name);

	/* Return if named symbol fill does not contain a symbol */
	if ( blank(sym_fill.symbol) ) return TRUE;

	/* Determine the scaled symbol size */
	sscale = sym_fill.sym_scale;
	(void) graphics_symbol_size(sym_fill.symbol, sscale, &swidth, &sheight,
														NullFloat, NullFloat);
	maxsize = MAX(swidth, sheight);

	/* Set dimensions of symbol fill box for outlines */
	xmin = xmax = outline->points[0][X];
	ymin = ymax = outline->points[0][Y];
	for ( ipts=1; ipts<outline->numpts; ipts++ )
		{
		xx = outline->points[ipts][X];
		yy = outline->points[ipts][Y];
		if ( xx < xmin ) xmin = xx;
		if ( xx > xmax ) xmax = xx;
		if ( yy < ymin ) ymin = yy;
		if ( yy > ymax ) ymax = yy;
		}

	/* >>>>> debug testing for GRA_display_outline_symbol_fill() <<<<< */
	if ( DebugMode )
		{
		(void) fprintf(stdout,
			"Outline Symbol Fill - xmin/xmax ymin/ymax ... %.2f/%.2f %.2f/%.2f\n",
			xmin, xmax, ymin, ymax);
		}
	/* >>>>> debug testing for GRA_display_outline_symbol_fill() <<<<< */

	/* Unadjust dimensions for perspective (if required) */
	(void) set_point(pos, xmin, ymin);
	if ( perspective_original(pos, ppos, NullFloat) )
		{
		xmin = ppos[X];		ymin = ppos[Y];
		}
	(void) set_point(pos, xmax, ymax);
	if ( perspective_original(pos, ppos, NullFloat) )
		{
		xmax = ppos[X];		ymax = ppos[Y];
		}

	/* >>>>> debug testing for GRA_display_outline_symbol_fill() <<<<< */
	if ( DebugMode )
		{
		(void) fprintf(stdout,
			"Outline Symbol Fill - xmin/xmax ymin/ymax ... %.2f/%.2f %.2f/%.2f (Adjusted)\n",
			xmin, xmax, ymin, ymax);
		}
	/* >>>>> debug testing for GRA_display_outline_symbol_fill() <<<<< */

	/* Set starting location of symbol fill wrt current map */
	if ( AnchorToMap )
		{
		(void) map_dimensions(mcen, NullFloat, NullFloat);
		xcen  = mcen[X];
		ycen  = mcen[Y];
		}

	/* Set starting location of symbol fill box wrt center of outline */
	else
		{
		xcen = (xmin + xmax) / 2.0;
		ycen = (ymin + ymax) / 2.0;
		}

	/* Reset dimensions of symbol fill box wrt xcen/ycen */
	xmin -= xcen;
	xmax -= xcen;
	ymin -= ycen;
	ymax -= ycen;

	/* Expand dimensions of symbol fill box for edge symbols */
	xmin -= maxsize;
	xmax += maxsize;
	ymin -= maxsize;
	ymax += maxsize;

	/* Determine row and column dimensions for symbol fill */

	/*  ... special case for no "x_repeat" or "y_repeat" */
	if ( sym_fill.x_repeat == 0.0 && sym_fill.y_repeat == 0.0 )
		{
		minrow = maxrow = mincol = maxcol = 0;
		}

	/*  ... special case for no "x_repeat" */
	else if ( sym_fill.x_repeat == 0.0 )
		{
		minrow = NINT((ymin - sym_fill.y_off) / sym_fill.y_repeat - 0.5);
		maxrow = NINT((ymax - sym_fill.y_off) / sym_fill.y_repeat + 0.5);
		mincol = maxcol = 0;
		}

	/*  ... special case for no "y_repeat" */
	else if ( sym_fill.y_repeat == 0.0 )
		{
		minrow = maxrow = 0;
		mincol = NINT((xmin - sym_fill.x_off) / sym_fill.x_repeat - 0.5);
		maxcol = NINT((xmax - sym_fill.x_off) / sym_fill.x_repeat + 0.5);
		}

	/*  ... special case for problem with "..._shift" parameters */
	else if ( (sym_fill.x_repeat * sym_fill.y_repeat)
					== (sym_fill.x_shift * sym_fill.y_shift) )
		{
		minrow = maxrow = mincol = maxcol = 0;
		}

	/*  ... default case for all other parameters */
	else
		{
		xmind = xmin - sym_fill.x_off;
		xmaxd = xmax - sym_fill.x_off;
		ymind = ymin - sym_fill.y_off;
		ymaxd = ymax - sym_fill.y_off;
		denom = (sym_fill.x_repeat * sym_fill.y_repeat)
					- (sym_fill.x_shift * sym_fill.y_shift);
		rmin1 = (ymind * sym_fill.x_repeat) - (xmind * sym_fill.y_shift);
		rmin2 = (ymind * sym_fill.x_repeat) - (xmaxd * sym_fill.y_shift);
		rmax1 = (ymaxd * sym_fill.x_repeat) - (xmaxd * sym_fill.y_shift);
		rmax2 = (ymaxd * sym_fill.x_repeat) - (xmind * sym_fill.y_shift);
		cmin1 = (xmind * sym_fill.y_repeat) - (ymind * sym_fill.x_shift);
		cmin2 = (xmind * sym_fill.y_repeat) - (ymaxd * sym_fill.x_shift);
		cmax1 = (xmaxd * sym_fill.y_repeat) - (ymaxd * sym_fill.x_shift);
		cmax2 = (xmaxd * sym_fill.y_repeat) - (ymind * sym_fill.x_shift);
		minrow = NINT(MIN(rmin1, rmin2) / denom - 0.5);
		maxrow = NINT(MAX(rmax1, rmax2) / denom + 0.5);
		mincol = NINT(MIN(cmin1, cmin2) / denom - 0.5);
		maxcol = NINT(MAX(cmax1, cmax2) / denom + 0.5);
		}

	/* Set the symbol fill mask from the outline */
	(void) write_graphics_outline_mask(outline, TRUE);

	/* Save the current presentation */
	(void) copy_presentation(&temp_pres, &CurPres);

	/* Reset the presentation to display the symbols */
	(void) copy_presentation(&CurPres, &(sym_fill.presentation));

	/* Cycle through rows and columns to display symbols */
	for ( irow=minrow; irow<=maxrow; irow++ )
		for ( icol=mincol; icol<=maxcol; icol++ )
			{

			/* Set symbol location */
			pos[X] = xcen + sym_fill.x_off
					+ (float) icol * sym_fill.x_repeat
					+ (float) irow * sym_fill.x_shift;
			pos[Y] = ycen + sym_fill.y_off
					+ (float) irow * sym_fill.y_repeat
					+ (float) icol * sym_fill.y_shift;

			/* Adjust symbol location for perspective (if required) */
			if ( perspective_location(pos, ppos, &pscale) )
				{
				(void) copy_point(pos, ppos);
				sscale = sym_fill.sym_scale * pscale;
				}

			/* Check symbol location ... based on application */
			(void) line_test_point(outline, pos,
					&dist, NullPoint, NullInt, &inside, NullChar);
			switch ( Program.macro )
				{

				/* PSMet symbols must be inside outline */
				/*  ... or within maxsize of outline    */
				case GPG_PSMet:
					if ( !inside && dist >= maxsize ) continue;
					break;

				/* SVGMet symbols must be inside outline */
				/*  ... or within maxsize of outline     */
				case GPG_SVGMet:
					if ( !inside && dist >= maxsize ) continue;
					break;

				/* CorMet symbols must be inside outline      */
				/*  ... and further than maxsize from outline */
				case GPG_CorMet:
					if ( !inside || dist <= maxsize ) continue;
					break;

				/* Skip symbols for unknown applications */
				default:
					continue;
				}

			/* Display the symbol */
			(void) write_graphics_symbol(sym_fill.symbol, pos[X], pos[Y],
					sscale, sym_fill.sym_rotate);
			}

	/* Restore the original presentation */
	(void) copy_presentation(&CurPres, &temp_pres);

	/* Turn off the symbol fill mask */
	(void) write_graphics_outline_mask(NullLine, FALSE);

	/* Return TRUE */
	return TRUE;
	}

static	LOGICAL		GRA_display_boundary_symbol_fill

	(
	BOUND		bound,			/* Boundary to display */
	STRING		sym_fill_name	/* Symbol pattern for interior of boundary */
	)

	{
	int				ipts, minrow, maxrow, mincol, maxcol, irow, icol;
	float			rmin1, rmin2, rmax1, rmax2;
	float			cmin1, cmin2, cmax1, cmax2;
	float			sscale, pscale, swidth, sheight, maxsize;
	float			xmin, xmax, ymin, ymax, xcen, ycen, xx, yy;
	float			xmind, xmaxd, ymind, ymaxd, denom, dist;
	POINT			mcen, pos, ppos;
	LOGICAL			inside;
	PRES			temp_pres;
	SYMBOL_FILL		sym_fill;

	/* Check that boundary exists */
	if ( IsNull(bound) ) return TRUE;
	if ( IsNull(bound->boundary) ) return TRUE;
	if ( bound->boundary->numpts < 2 ) return TRUE;

	/* Check for acceptable symbol fill name */
	if ( blank(sym_fill_name)
			|| same(sym_fill_name, SymbolFillNone) ) return TRUE;

	/* Get the named symbol fill parameters */
	sym_fill = get_symbol_fill(sym_fill_name);

	/* Return if named symbol fill does not contain a symbol */
	if ( blank(sym_fill.symbol) ) return TRUE;

	/* Determine the scaled symbol size */
	sscale = sym_fill.sym_scale;
	(void) graphics_symbol_size(sym_fill.symbol, sscale, &swidth, &sheight,
														NullFloat, NullFloat);
	maxsize = MAX(swidth, sheight);

	/* Set dimensions of symbol fill box for boundaries */
	xmin = xmax = bound->boundary->points[0][X];
	ymin = ymax = bound->boundary->points[0][Y];
	for ( ipts=1; ipts<bound->boundary->numpts; ipts++ )
		{
		xx = bound->boundary->points[ipts][X];
		yy = bound->boundary->points[ipts][Y];
		if ( xx < xmin ) xmin = xx;
		if ( xx > xmax ) xmax = xx;
		if ( yy < ymin ) ymin = yy;
		if ( yy > ymax ) ymax = yy;
		}

	/* >>>>> debug testing for GRA_display_boundary_symbol_fill() <<<<< */
	if ( DebugMode )
		{
		(void) fprintf(stdout,
			"Boundary Symbol Fill - xmin/xmax ymin/ymax ... %.2f/%.2f %.2f/%.2f\n",
			xmin, xmax, ymin, ymax);
		}
	/* >>>>> debug testing for GRA_display_boundary_symbol_fill() <<<<< */

	/* Unadjust dimensions for perspective (if required) */
	(void) set_point(pos, xmin, ymin);
	if ( perspective_original(pos, ppos, NullFloat) )
		{
		xmin = ppos[X];		ymin = ppos[Y];
		}
	(void) set_point(pos, xmax, ymax);
	if ( perspective_original(pos, ppos, NullFloat) )
		{
		xmax = ppos[X];		ymax = ppos[Y];
		}

	/* >>>>> debug testing for GRA_display_boundary_symbol_fill() <<<<< */
	if ( DebugMode )
		{
		(void) fprintf(stdout,
			"Boundary Symbol Fill - xmin/xmax ymin/ymax ... %.2f/%.2f %.2f/%.2f (Adjusted)\n",
			xmin, xmax, ymin, ymax);
		}
	/* >>>>> debug testing for GRA_display_boundary_symbol_fill() <<<<< */

	/* Set starting location of symbol fill wrt current map */
	if ( AnchorToMap )
		{
		(void) map_dimensions(mcen, NullFloat, NullFloat);
		xcen  = mcen[X];
		ycen  = mcen[Y];
		}

	/* Set starting location of symbol fill box wrt center of outline */
	else
		{
		xcen = (xmin + xmax) / 2.0;
		ycen = (ymin + ymax) / 2.0;
		}

	/* Reset dimensions of symbol fill box wrt xcen/ycen */
	xmin -= xcen;
	xmax -= xcen;
	ymin -= ycen;
	ymax -= ycen;

	/* Expand dimensions of symbol fill box for edge symbols */
	xmin -= maxsize;
	xmax += maxsize;
	ymin -= maxsize;
	ymax += maxsize;

	/* Determine row and column dimensions for symbol fill */

	/*  ... special case for no "x_repeat" or "y_repeat" */
	if ( sym_fill.x_repeat == 0.0 && sym_fill.y_repeat == 0.0 )
		{
		minrow = maxrow = mincol = maxcol = 0;
		}

	/*  ... special case for no "x_repeat" */
	else if ( sym_fill.x_repeat == 0.0 )
		{
		minrow = NINT((ymin - sym_fill.y_off) / sym_fill.y_repeat - 0.5);
		maxrow = NINT((ymax - sym_fill.y_off) / sym_fill.y_repeat + 0.5);
		mincol = maxcol = 0;
		}

	/*  ... special case for no "y_repeat" */
	else if ( sym_fill.y_repeat == 0.0 )
		{
		minrow = maxrow = 0;
		mincol = NINT((xmin - sym_fill.x_off) / sym_fill.x_repeat - 0.5);
		maxcol = NINT((xmax - sym_fill.x_off) / sym_fill.x_repeat + 0.5);
		}

	/*  ... special case for problem with "..._shift" parameters */
	else if ( (sym_fill.x_repeat * sym_fill.y_repeat)
					== (sym_fill.x_shift * sym_fill.y_shift) )
		{
		minrow = maxrow = mincol = maxcol = 0;
		}

	/*  ... default case for all other parameters */
	else
		{
		xmind = xmin - sym_fill.x_off;
		xmaxd = xmax - sym_fill.x_off;
		ymind = ymin - sym_fill.y_off;
		ymaxd = ymax - sym_fill.y_off;
		denom = (sym_fill.x_repeat * sym_fill.y_repeat)
					- (sym_fill.x_shift * sym_fill.y_shift);
		rmin1 = (ymind * sym_fill.x_repeat) - (xmind * sym_fill.y_shift);
		rmin2 = (ymind * sym_fill.x_repeat) - (xmaxd * sym_fill.y_shift);
		rmax1 = (ymaxd * sym_fill.x_repeat) - (xmaxd * sym_fill.y_shift);
		rmax2 = (ymaxd * sym_fill.x_repeat) - (xmind * sym_fill.y_shift);
		cmin1 = (xmind * sym_fill.y_repeat) - (ymind * sym_fill.x_shift);
		cmin2 = (xmind * sym_fill.y_repeat) - (ymaxd * sym_fill.x_shift);
		cmax1 = (xmaxd * sym_fill.y_repeat) - (ymaxd * sym_fill.x_shift);
		cmax2 = (xmaxd * sym_fill.y_repeat) - (ymind * sym_fill.x_shift);
		minrow = NINT(MIN(rmin1, rmin2) / denom - 0.5);
		maxrow = NINT(MAX(rmax1, rmax2) / denom + 0.5);
		mincol = NINT(MIN(cmin1, cmin2) / denom - 0.5);
		maxcol = NINT(MAX(cmax1, cmax2) / denom + 0.5);
		}

	/* Set the symbol fill mask from the boundary */
	(void) write_graphics_boundary_mask(bound, TRUE);

	/* Save the current presentation */
	(void) copy_presentation(&temp_pres, &CurPres);

	/* Reset the presentation to display the symbols */
	(void) copy_presentation(&CurPres, &(sym_fill.presentation));

	/* Cycle through rows and columns to display symbols */
	for ( irow=minrow; irow<=maxrow; irow++ )
		for ( icol=mincol; icol<=maxcol; icol++ )
			{

			/* Set symbol location */
			pos[X] = xcen + sym_fill.x_off
					+ (float) icol * sym_fill.x_repeat
					+ (float) irow * sym_fill.x_shift;
			pos[Y] = ycen + sym_fill.y_off
					+ (float) irow * sym_fill.y_repeat
					+ (float) icol * sym_fill.y_shift;

			/* Adjust symbol location for perspective (if required) */
			if ( perspective_location(pos, ppos, &pscale) )
				{
				(void) copy_point(pos, ppos);
				sscale = sym_fill.sym_scale * pscale;
				}

			/* Check symbol location ... based on application */
			(void) bound_test_point(bound, pos,
					&dist, NullPoint, NullInt, NullInt, &inside);
			switch ( Program.macro )
				{

				/* PSMet symbols must be inside boundary */
				/*  ... or within maxsize of boundary    */
				case GPG_PSMet:
					if ( !inside && dist >= maxsize ) continue;
					break;

				/* SVGMet symbols must be inside boundary */
				/*  ... or within maxsize of boundary     */
				case GPG_SVGMet:
					if ( !inside && dist >= maxsize ) continue;
					break;

				/* CorMet symbols must be inside boundary      */
				/*  ... and further than maxsize from boundary */
				case GPG_CorMet:
					if ( !inside || dist <= maxsize ) continue;
					break;

				/* Skip symbols for unknown applications */
				default:
					continue;
				}

			/* Display the symbol */
			(void) write_graphics_symbol(sym_fill.symbol, pos[X], pos[Y],
					sscale, sym_fill.sym_rotate);
			}

	/* Restore the original presentation */
	(void) copy_presentation(&CurPres, &temp_pres);

	/* Turn off the symbol fill mask */
	(void) write_graphics_boundary_mask(NullBound, FALSE);

	/* Return TRUE */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    G R A _ d i s p l a y _ a t t r i b u t e                         *
*    G R A _ d i s p l a y _ a t t r i b u t e _ o u t l i n e         *
*                                                                      *
***********************************************************************/

static	LOGICAL		GRA_display_attribute

	(
	CAL				cal,
	ATTRIB_DISPLAY	attrib,
	float			flat,
	float			flon,
	float			xorig,
	float			yorig,
	float			rotation,	/* Label rotation (degrees) */
	STRING			text
	)

	{
	float		tsize, pscale, xpos, ypos, xx, yy;
	STRING		justified;

	/* First do the box, ellipse, or underline display */
	if ( !blank(attrib.display_name) )
		{
		if ( !GRA_display_attribute_outline(cal, attrib, flat, flon,
													xorig, yorig, rotation) )
			return FALSE;
		}

	/* Then display the attribute text */
	if ( !blank(text) && attrib.show_label )
		{

		/* Set the justification */
		justified = CurPres.justified;

		/* Set the display location for the attribute text */
		xpos = attrib.x_attrib;
		ypos = attrib.y_attrib;

		/* Convert the display location to rotated coordinates */
		(void) rotated_location(xpos, ypos, rotation, &xx, &yy);

		/* Adjust display location wrt origin */
		xx += xorig;
		yy += yorig;

		/* Adjust text size for perspective (if required) */
		tsize = attrib.txt_size;
		if ( perspective_scale(&pscale) ) tsize *= pscale;

		/* Now display the attribute text */
		(void) write_graphics_text(text, xx, yy, tsize,
				justified, rotation, TRUE);
		}

	/* Return TRUE */
	return TRUE;
	}

static	LOGICAL		GRA_display_attribute_outline

	(
	CAL				cal,
	ATTRIB_DISPLAY	attrib,
	float			flat,
	float			flon,
	float			xorig,
	float			yorig,
	float			rotation	/* Outline rotation (degrees) */
	)

	{
	float			sx, sy, sadj;
	float			xleft, xright, ybottom, ytop;
	float			xpos, ypos, xx, yy, width, height, xval, rotate;
	double			dval, dadj;
	STRING			val;
	PRES			temp_pres;
	LABEL_DISPLAY	label_display;

	/* Return immediately if no box, ellipse, or underline display */
	if ( blank(attrib.display_name) ) return FALSE;

	/* Get the named label display */
	label_display = get_label_display(attrib.display_name);

	/* Get the map distortion for display from attributes (if required) */
	sadj = 1.0;
	if ( !blank(label_display.width_attrib)
			|| !blank(label_display.height_attrib)
			|| !blank(label_display.diameter_attrib)
			|| !blank(label_display.radius_attrib) )
		{
		(void) ll_distort(&BaseMap, flat, flon, &sx, &sy);
		sadj = (sx + sy) / 2.0;

		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */
		if ( DebugMode )
			{
			(void) fprintf(stdout,
				"Map distortion at %.3f/%.3f  sx/sy/sadj ... %.5f/%.5f/%.5f\n",
				flat, flon, sx, sy, sadj);
			}
		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */
		}

	/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */
	if ( DebugMode )
		{
		if ( !blank(label_display.width_attrib)
				|| !blank(label_display.height_attrib)
				|| !blank(label_display.diameter_attrib)
				|| !blank(label_display.radius_attrib)
				|| !blank(label_display.rot_attrib) )
			(void) debug_attrib_list("GRA_display_attribute_outline", cal);
		}
	/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */

	/* Set initial display width and height from parameters */
	width  = label_display.width;
	height = label_display.height;

	/* Reset display width from width attribute */
	if ( !blank(label_display.width_attrib) )
		{
		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */
		if ( DebugMode )
			{
			(void) fprintf(stdout,
				"Start width from attribute ... %.5f\n", width);
			}
		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */

		/* Extract width from attribute */
		val = CAL_get_attribute(cal, label_display.width_attrib);

		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */
		if ( DebugMode )
			{
			(void) fprintf(stdout,
				"Extract width from attribute ... %s ... %s\n",
				label_display.width_attrib, val);
			}
		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */

		if ( !blank(val) )
			{

			/* Convert width to map units scaled to current display */
			dval   = atof(val);
			(void) convert_value(label_display.attrib_units, dval,
					LabelDisplayUnitsKm, &dadj);
			width  = (float) dadj *1000.0 / BaseMap.definition.units;
			width *= map_scaling();
			width *= sadj;
			}

		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */
		if ( DebugMode )
			{
			(void) fprintf(stdout,
				"Final width from attribute ... %.5f  scaling ... %.5f\n",
				width, sadj*map_scaling());
			}
		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */
		}

	/* Reset display height from height attribute */
	if ( !blank(label_display.height_attrib) )
		{
		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */
		if ( DebugMode )
			{
			(void) fprintf(stdout,
				"Start height from attribute ... %.5f\n", height);
			}
		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */

		/* Extract height from attribute */
		val = CAL_get_attribute(cal, label_display.height_attrib);

		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */
		if ( DebugMode )
			{
			(void) fprintf(stdout,
				"Extract height from attribute ... %s ... %s\n",
				label_display.height_attrib, val);
			}
		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */

		if ( !blank(val) )
			{

			/* Convert height to map units scaled to current display */
			dval    = atof(val);
			(void) convert_value(label_display.attrib_units, dval,
					LabelDisplayUnitsKm, &dadj);
			height  = (float) dadj *1000.0 / BaseMap.definition.units;
			height *= map_scaling();
			height *= sadj;
			}

		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */
		if ( DebugMode )
			{
			(void) fprintf(stdout,
				"Final height from attribute ... %.5f  scaling ... %.5f\n",
			
				height, sadj*map_scaling());
			}
		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */
		}

	/* Reset display width and height from diameter attribute */
	if ( !blank(label_display.diameter_attrib) )
		{
		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */
		if ( DebugMode )
			{
			(void) fprintf(stdout,
				"Start width/height from diameter attribute ... %.5f/%.5f\n",
				width, height);
			}
		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */

		/* Extract diameter from attribute */
		val = CAL_get_attribute(cal, label_display.diameter_attrib);

		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */
		if ( DebugMode )
			{
			(void) fprintf(stdout,
				"Extract width/height from diameter attribute ... %s ... %s\n",
				label_display.diameter_attrib, val);
			}
		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */

		if ( !blank(val) )
			{

			/* Convert diameter to map units scaled to current display */
			dval    = atof(val);
			(void) convert_value(label_display.attrib_units, dval,
					LabelDisplayUnitsKm, &dadj);
			xval   = (float) dadj *1000.0 / BaseMap.definition.units;
			xval  *= map_scaling();
			xval  *= sadj;

			/* Set width and height from diameter */
			width  = xval;
			height = xval;
			}

		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */
		if ( DebugMode )
			{
			(void) fprintf(stdout,
				"Final width/height from diameter attribute ... %.5f/%.5f  scaling ... %.5f\n",
			
				width, height, sadj*map_scaling());
			}
		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */
		}

	/* Reset display width and height from radius attribute */
	if ( !blank(label_display.radius_attrib) )
		{
		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */
		if ( DebugMode )
			{
			(void) fprintf(stdout,
				"Start width/height from radius attribute ... %.5f/%.5f\n",
				width, height);
			}
		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */

		/* Extract radius from attribute */
		val = CAL_get_attribute(cal, label_display.radius_attrib);

		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */
		if ( DebugMode )
			{
			(void) fprintf(stdout,
				"Extract width/height from radius attribute ... %s ... %s\n",
				label_display.radius_attrib, val);
			}
		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */

		if ( !blank(val) )
			{

			/* Convert radius to map units scaled to current display */
			dval    = atof(val);
			(void) convert_value(label_display.attrib_units, dval,
					LabelDisplayUnitsKm, &dadj);
			xval   = (float) dadj *1000.0 / BaseMap.definition.units;
			xval  *= map_scaling();
			xval  *= sadj;

			/* Set width and height from radius */
			width  = 2.0 * xval;
			height = 2.0 * xval;
			}

		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */
		if ( DebugMode )
			{
			(void) fprintf(stdout,
				"Final width/height from radius attribute ... %.5f/%.5f  scaling ... %.5f\n",
			
				width, height, sadj*map_scaling());
			}
		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */
		}

	/* Set initial display rotation from parameter or attribute */
	rotate = label_display.rotation;
	if ( !blank(label_display.rot_attrib) )
		{
		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */
		if ( DebugMode )
			{
			(void) fprintf(stdout,
				"Start rotation from attribute ... %.5f\n", rotate);
			}
		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */

		/* Extract rotation from attribute */
		val = CAL_get_attribute(cal, label_display.rot_attrib);

		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */
		if ( DebugMode )
			{
			(void) fprintf(stdout,
				"Extract rotation from attribute ... %s ... %s\n",
				label_display.rot_attrib, val);
			}
		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */

		if ( !blank(val) )
			{

			/* Convert rotation to degrees on the current basemap */
			dval   = atof(val);
			rotate = wind_dir_xy(&BaseMap, flat, flon, (float) dval);
			}

		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */
		if ( DebugMode )
			{
			(void) fprintf(stdout,
				"Rotation on current basemap ... %.5f\n", rotate);
			}
		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */

		/* Add the label display rotation */
		rotate += label_display.rotation;

		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */
		if ( DebugMode )
			{
			(void) fprintf(stdout,
				"Final rotation from attribute ... %.5f\n", rotate);
			}
		/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */
		}

	/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */
	if ( DebugMode )
		{
		(void) fprintf(stdout, "Attribute Display Type ... %s\n",
			attrib.display_name);
		(void) fprintf(stdout, "  x_attrib/y_attrib ... %.3f %.3f\n",
			attrib.x_attrib, attrib.y_attrib);
		(void) fprintf(stdout, "  x_centre/y_centre ... %.3f %.3f\n",
			attrib.x_centre, attrib.y_centre);
		(void) fprintf(stdout, "     x_left/x_right ... %.3f %.3f ... %.3f\n",
			attrib.x_left, attrib.x_right, (attrib.x_right+attrib.x_left)/2.0);
		(void) fprintf(stdout, "     y_bottom/y_top ... %.3f %.3f ... %.3f\n",
			attrib.y_bottom, attrib.y_top, (attrib.y_top+attrib.y_bottom)/2.0);
		}
	/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */

	/* Set display parameters for box sized to attribute text */
	/* Note that box is centered around the attribute, with   */
	/*  adjustments for margins on each side                  */
	if ( same(attrib.display_type, LabelSizedBox) )
		{
		xleft   = attrib.x_left   - label_display.margin_left;
		xright  = attrib.x_right  + label_display.margin_right;
		xpos    = (xright + xleft) / 2.0;
		xpos   += label_display.x_off;
		ybottom = attrib.y_bottom - label_display.margin_bottom;
		ytop    = attrib.y_top    + label_display.margin_top;
		ypos    = (ytop + ybottom) / 2.0;
		ypos   += label_display.y_off;
		width   = xright - xleft;
		height  = ytop - ybottom;
		}

	/* Set display parameters for fixed sized box      */
	/* Note that box is centered on attribute location */
	else if ( same(attrib.display_type, LabelFixedBox) )
		{
		xpos = attrib.x_attrib + label_display.x_off;
		ypos = attrib.y_attrib + label_display.y_off;
		}

	/* Set display parameters for ellipse sized to attribute text */
	/* Note that ellipse is centered around the attribute, with   */
	/*  adjustments for margins on each side                      */
	else if ( same(attrib.display_type, LabelSizedEllipse) )
		{
		xleft   = attrib.x_left   - label_display.margin_left;
		xright  = attrib.x_right  + label_display.margin_right;
		xpos    = (xright + xleft) / 2.0;
		xpos   += label_display.x_off;
		ybottom = attrib.y_bottom - label_display.margin_bottom;
		ytop    = attrib.y_top    + label_display.margin_top;
		ypos    = (ytop + ybottom) / 2.0;
		ypos   += label_display.y_off;
		width   = xright - xleft;
		height  = ytop - ybottom;
		}

	/* Set display parameters for fixed sized ellipse      */
	/* Note that ellipse is centered on attribute location */
	else if ( same(attrib.display_type, LabelFixedEllipse) )
		{
		xpos = attrib.x_attrib + label_display.x_off;
		ypos = attrib.y_attrib + label_display.y_off;
		}

	/* Set display parameters for underline sized to attribute text */
	/* Note that underline is centered around the attribute, with   */
	/*  adjustments for margins on each side                        */
	else if ( same(attrib.display_type, LabelSizedUnderline) )
		{
		xleft   = attrib.x_left  - label_display.margin_left;
		xright  = attrib.x_right + label_display.margin_right;
		xpos    = (xright + xleft) / 2.0;
		xpos   += label_display.x_off;
		ypos    = attrib.y_bottom + label_display.y_off;
		ypos   += label_display.margin_top;
		ypos   -= label_display.margin_bottom;
		width   = xright - xleft;
		height  = 0.0;
		}

	/* Set display parameters for fixed sized underline      */
	/* Note that underline is centered on attribute location */
	else if ( same(attrib.display_type, LabelFixedUnderline) )
		{
		xpos = attrib.x_attrib + label_display.x_off;
		ypos = attrib.y_attrib + label_display.y_off;
		}

	/* Convert the display location to rotated coordinates */
	(void) rotated_location(xpos, ypos, rotation, &xx, &yy);

	/* Adjust display location wrt origin */
	xx += xorig;
	yy += yorig;

	/* Add the rotation of the box, ellipse, or underline */
	rotate += rotation;

	/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */
	if ( DebugMode )
		{
		(void) fprintf(stdout, "rotation for display ... %.5f\n", rotate);
		}
	/* >>>>> debug testing for GRA_display_attribute_outline() <<<<< */

	/* Save the current presentation */
	(void) copy_presentation(&temp_pres, &CurPres);

	/* Reset the presentation to display the box, ellipse, or underline */
	(void) copy_presentation(&CurPres, &(label_display.presentation));

	/* Display a box using the label display parameters */
	if ( same(attrib.display_type, LabelSizedBox)
			|| same(attrib.display_type, LabelFixedBox) )
		{
		(void) write_graphics_box(xx, yy, width, height, rotate, TRUE, TRUE);
		}

	/* Display an ellipse using the label display parameters */
	else if ( same(attrib.display_type, LabelSizedEllipse)
			|| same(attrib.display_type, LabelFixedEllipse) )
		{
		if ( width > 0.0 && height > 0.0 )
			(void) write_graphics_ellipse(xx, yy, width, height,
					label_display.start_angle, label_display.end_angle,
					label_display.closed, rotate, TRUE, TRUE);
		}

	/* Display an underline using the label display parameters */
	else if ( same(attrib.display_type, LabelSizedUnderline)
			|| same(attrib.display_type, LabelFixedUnderline) )
		{
		(void) write_graphics_underline(xx, yy, width, rotate);
		}

	/* Restore the original presentation */
	(void) copy_presentation(&CurPres, &temp_pres);

	/* Return TRUE */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    G R A _ d i s p l a y _ s a m p l e d _ a t t r i b u t e s       *
*    G R A _ d i s p l a y _ s a m p l e d _ v a l u e                 *
*    G R A _ d i s p l a y _ s a m p l e d _ v e c t o r               *
*                                                                      *
***********************************************************************/

static	LOGICAL		GRA_display_sampled_attributes

	(
	STRING			loc_ident,		/* Current identifier */
	STRING			vtime,			/* Current valid time */
	STRING			time_zone,		/* Time zone (used in strftime) */
	STRING			language,		/* Language (used in strftime) */
	STRING			loc_label,		/* Current label */
	POINT			pos,			/* Current position */
	float			fdist,			/* Distance to feature */
	float			fbear,			/* Bearing to feature */
	float			flen,			/* Length of current line */
	float			xdir,			/* Direction at current segment (deg True) */
	float			xspd,			/* Speed at current segment (m/s) */
	CAL				fld_attrib,		/* Sampled attributes */
	STRING			cat_cascade,	/* And/Or for multiple CATATTRIB structs */
	CATATTRIB		*cat_attrib,	/* Structure containing categories */
	int				num_catatt,		/* Number of categories */
	ATTRIB_DISPLAY	*attribs,		/* Attributes to display */
	int				num_attrib,		/* Number of attributes */
	SPCASE			*list_case,		/* Structure for special cases */
	int				num_list,		/* Number of special cases */
	STRING			inmark,			/* Name of location marker */
	float			markscale,		/* Marker scaling factor (percent) */
	STRING			display_name,	/* Name for label display */
	STRING			display_type,	/* Type of label display */
	LOGICAL			fit_to_map,		/* Fit display within map? */
	STRING			fit_to_map_ref,	/* Reference position for fit */
	float			rotation,		/* Label rotation (degrees) */
	LOGICAL			rotate_lat,		/* Rotate label to align with latitude? */
	LOGICAL			rotate_lon,		/* Rotate label to align with longitude? */
	STRING			rot_attrib,		/* Attribute to align label to */
	LOGICAL			constrain_rot,	/* Constrain rotation to display upright? */
	float			xorig,
	float			yorig,
	float			xpos,
	float			ypos
	)

	{
	int				nn;
	float			mscale, sscale, pscale;
	float			flat, flon, xadj, yadj, rotadj, rval, rotmark, xxs, yys;
	double			dval;
	STRING			value, lookup_used, symbol, text;
	WIND_VAL		windval;
	ATTRIB_DISPLAY	full_display;
	PRES			temp_pres;
	char			out_buf[GPGLong];

	/* Check for acceptable attributes */
	if ( !check_label_attributes(fld_attrib, attribs, num_attrib) )
		{
		return FALSE;
		}

	/* Check for values that match each desired category         */
	/* Note that each category may contain more than one member! */
	if ( !match_category_attributes(fld_attrib,
										cat_cascade, cat_attrib, num_catatt) )
		{
		return FALSE;
		}

	/* Set the label location */
	(void) pos_to_ll(&BaseMap, pos, &flat, &flon);

	/* Set rotation from latitude or longitude */
	if ( AnchorToMap && rotate_lat )
		rotadj = wind_dir_xy(&BaseMap, flat, flon, 90.0);
	else if ( AnchorToMap && rotate_lon )
		rotadj = wind_dir_xy(&BaseMap, flat, flon,  0.0);
	else if ( AnchorToMap && !blank(rot_attrib) )
		{

		/* Note that rotation attribute cannot be magic distance/length/speed */
		if ( magic_is_attribute(rot_attrib) )
			{
			value = magic_get_attribute(rot_attrib, loc_ident, vtime,
						FpaCblank, FpaCblank, loc_label, pos, DefProximity,
						fbear, DefLineLen, xdir, DefSegSpd, FpaCmksUnits,
						FormatDirect, FpaCblank);
			}
		else
			{
			value = CAL_get_attribute(fld_attrib, rot_attrib);
			}
		rval = 0.0;
		if ( !blank(value) )
			{
			rval   = atof(value);
			rotadj = wind_dir_xy(&BaseMap, flat, flon, rval);
			}
		else
			{
			rotadj = 0.0;
			}
		}
	else
		{
		rotadj = 0.0;
		}

	/* Add default rotation */
	rotadj += rotation;

	/* Constrain rotation to upright (if requested) */
	if ( constrain_rot && rotadj > 90.0 && rotadj < 270.0) rotadj -= 180.0;

	/* Determine the placement of each attribute */
	for ( nn=0; nn<num_attrib; nn++ )
		{

		/* Extract formatted values for "magic" attributes */
		if ( magic_is_attribute(attribs[nn].name) )
			{
			value = magic_get_attribute(attribs[nn].name, loc_ident, vtime,
						time_zone, language, loc_label, pos, fdist, fbear,
						flen, xdir, xspd, attribs[nn].units,
						attribs[nn].format, attribs[nn].conversion_format);
			(void) strcpy(out_buf, value);

			/* Determine the vector parameters */
			if ( same(attribs[nn].format, FormatVectorText)
					|| same(attribs[nn].format, FormatVectorSymbol) )
				{
				if ( parse_wind_value_string(value, &windval) )
					{
					/* Convert the vector speed units */
					(void) convert_value(windval.sunit, (double) windval.speed,
							attribs[nn].units, &dval);
					windval.speed = (float) dval;
					}
				else
					{
					(void) fprintf(stdout,
							"   Cannot parse vector value ... %s\n", value);
					continue;
					}
				}
			}

		/* Extract formatted values for all other attributes */
		else
			{

			/* Extract the attribute value */
			value = CAL_get_attribute(fld_attrib, attribs[nn].name);

			/* Set blank display for FormatNone */
			if ( same(attribs[nn].format, FormatNone) )
				{
				(void) strcpy(out_buf, FpaCblank);
				}

			/* Apply the format conversion (if required) */
			else if ( same(attribs[nn].format, FormatDirect)
					|| same(attribs[nn].format, FormatSymbol)
					|| same(attribs[nn].format, FormatText) )
				{
				if ( blank(value) || blank(attribs[nn].conversion_format) )
					{
					(void) strcpy(out_buf, value);
					}
				else
					{
					(void) sprintf(out_buf, attribs[nn].conversion_format,
																		value);
					}
				}

			/* Determine the wind parameters */
			else if ( same(attribs[nn].format, FormatWindBarb)
					|| same(attribs[nn].format, FormatWindText)
					|| same(attribs[nn].format, FormatWindSymbol) )
				{
				if ( parse_wind_value_string(value, &windval) )
					{
					/* Convert the wind speed and gust units */
					(void) convert_value(windval.sunit, (double) windval.speed,
							attribs[nn].units, &dval);
					windval.speed = (float) dval;
					(void) convert_value(windval.sunit, (double) windval.gust,
							attribs[nn].units, &dval);
					windval.gust  = (float) dval;
					}
				else
					{
					(void) fprintf(stdout,
							"   Cannot parse wind value ... %s\n", value);
					continue;
					}
				}

			/* Determine the vector parameters */
			else if ( same(attribs[nn].format, FormatVectorText)
					|| same(attribs[nn].format, FormatVectorSymbol) )
				{
				if ( parse_wind_value_string(value, &windval) )
					{
					/* Convert the vector speed units */
					(void) convert_value(windval.sunit, (double) windval.speed,
							attribs[nn].units, &dval);
					windval.speed = (float) dval;
					}
				else
					{
					(void) fprintf(stdout,
							"   Cannot parse vector value ... %s\n", value);
					continue;
					}
				}
			}

		/* Determine placement for values displayed directly */
		if ( same(attribs[nn].format, FormatNone)
				|| same(attribs[nn].format, FormatDirect) )
			{
			(void) set_attribute_placement(&attribs[nn], out_buf, NullString);
			}

		/* Determine placement for values displayed from */
		/*  the given look up table                      */
		else if ( same(attribs[nn].format, FormatSymbol)
				|| same(attribs[nn].format, FormatText) )
			{

			/* Use attribute look up table if no special cases */
			/* Note that special cases are not allowed for     */
			/*  complex labels!                                */
			if ( num_list <= 0 || num_attrib > 1 )
				{
				lookup_used = attribs[nn].look_up;
				}

			/* Set look up table based on special cases */
			else
				{
				lookup_used = check_sample_cases(pos, vtime,
						attribs[nn].look_up, list_case, num_list);
				}

			/* Match the value in the chosen look up table */
			if ( match_category_lookup(lookup_used, out_buf,
					&symbol, &text, NullStringPtr) )
				{
				if ( same(attribs[nn].format, FormatSymbol) )
					{
					(void) set_attribute_placement(&attribs[nn],
							NullString, symbol);
					}
				else
					{
					(void) set_attribute_placement(&attribs[nn],
							text, NullString);
					}
				}
			}

		/* Determine placement for wind parameters displayed as wind barbs */
		else if ( same(attribs[nn].format, FormatWindBarb) )
			{
			(void) set_wind_barb_placement(&attribs[nn],
					windval.dir, windval.speed, windval.gust);
			}

		/* Determine placement for wind parameters displayed     */
		/*  as text or symbols from the given wind look up table */
		else if ( same(attribs[nn].format, FormatWindText)
				|| same(attribs[nn].format, FormatWindSymbol) )
			{
			(void) set_wind_placement(attribs[nn].format, &attribs[nn],
					windval.dir, windval.speed, windval.gust, flat, flon);
			}

		/* Determine placement for vector parameters displayed     */
		/*  as text or symbols from the given vector look up table */
		else if ( same(attribs[nn].format, FormatVectorText)
				|| same(attribs[nn].format, FormatVectorSymbol) )
			{
			(void) set_vector_placement(attribs[nn].format, &attribs[nn],
					windval.dir, windval.speed, flat, flon);
			}
		}

	/* Start grouping for label containing all attributes */
	(void) write_graphics_group(GPGstart, NullPointer, 0);

	/* Determine positioning for complete set of attributes */
	full_display = full_attribute_placement();
	if ( !blank(display_name) )
		{
		(void) strcpy(full_display.display_name, display_name);
		(void) strcpy(full_display.display_type, display_type);
		}

	/* Adjust positioning for attributes (if requested) */
	xadj = xpos;
	yadj = ypos;
	rotmark = rotadj;
	if ( fit_to_map )
		{
		if ( !fit_attribute_placement(full_display, fit_to_map_ref,
				rotadj, xorig, yorig, xpos, ypos, &xadj, &yadj, &rotmark) )
			{
			(void) warn_report("Problem fitting label to map!");
			}
		}

	/* Adjust symbol scale for perspective (if required) */
	mscale = markscale;
	if ( perspective_scale(&pscale) ) mscale *= pscale;

	/* Display the mark (if requested) ... not offset! */
	if ( !blank(inmark) )
		{
		(void) write_graphics_symbol(inmark, xorig, yorig, mscale, rotmark);
		}

	/* Display graphics outline for complete set of attributes */
	if ( !blank(display_name) )
		{
		(void) GRA_display_attribute_outline(fld_attrib, full_display,
				flat, flon, xadj, yadj, rotadj);
		}

	/* Now display each attribute */
	for ( nn=0; nn<num_attrib; nn++ )
		{

		/* Save the current presentation */
		(void) copy_presentation(&temp_pres, &CurPres);

		/* Reset the current presentation for this attribute */
		(void) copy_presentation(&CurPres, &(attribs[nn].presentation));

		/* Extract formatted values for "magic" attributes */
		if ( magic_is_attribute(attribs[nn].name) )
			{
			value = magic_get_attribute(attribs[nn].name, loc_ident, vtime,
						time_zone, language, loc_label, pos, fdist, fbear,
						flen, xdir, xspd, attribs[nn].units,
						attribs[nn].format, attribs[nn].conversion_format);
			(void) strcpy(out_buf, value);

			/* Determine the vector parameters */
			if ( same(attribs[nn].format, FormatVectorText)
					|| same(attribs[nn].format, FormatVectorSymbol) )
				{
				if ( parse_wind_value_string(value, &windval) )
					{
					/* Convert the vector speed units */
					(void) convert_value(windval.sunit, (double) windval.speed,
							attribs[nn].units, &dval);
					windval.speed = (float) dval;
					}
				else
					{
					(void) fprintf(stdout,
							"   Cannot parse vector value ... %s\n", value);
					continue;
					}
				}
			}

		/* Extract formatted values for all other attributes */
		else
			{

			/* Extract the attribute value */
			value = CAL_get_attribute(fld_attrib, attribs[nn].name);

			/* Set blank display for FormatNone */
			if ( same(attribs[nn].format, FormatNone) )
				{
				(void) strcpy(out_buf, FpaCblank);
				}

			/* Apply the format conversion (if required) */
			else if ( same(attribs[nn].format, FormatDirect)
					|| same(attribs[nn].format, FormatSymbol)
					|| same(attribs[nn].format, FormatText) )
				{
				if ( blank(value) || blank(attribs[nn].conversion_format) )
					{
					(void) strcpy(out_buf, value);
					}
				else
					{
					(void) sprintf(out_buf, attribs[nn].conversion_format,
																		value);
					}
				}

			/* Determine the wind parameters */
			else if ( same(attribs[nn].format, FormatWindBarb)
					|| same(attribs[nn].format, FormatWindText)
					|| same(attribs[nn].format, FormatWindSymbol) )
				{
				if ( parse_wind_value_string(value, &windval) )
					{
					/* Convert the wind speed and gust units */
					(void) convert_value(windval.sunit, (double) windval.speed,
							attribs[nn].units, &dval);
					windval.speed = (float) dval;
					(void) convert_value(windval.sunit, (double) windval.gust,
							attribs[nn].units, &dval);
					windval.gust  = (float) dval;
					}
				else
					{
					(void) fprintf(stdout,
							"   Cannot parse wind value ... %s\n", value);
					continue;
					}
				}

			/* Determine the vector parameters */
			else if ( same(attribs[nn].format, FormatVectorText)
					|| same(attribs[nn].format, FormatVectorSymbol) )
				{
				if ( parse_wind_value_string(value, &windval) )
					{
					/* Convert the vector speed units */
					(void) convert_value(windval.sunit, (double) windval.speed,
							attribs[nn].units, &dval);
					windval.speed = (float) dval;
					}
				else
					{
					(void) fprintf(stdout,
							"   Cannot parse vector value ... %s\n", value);
					continue;
					}
				}
			}

		/* Display the attribute value directly */
		if ( same(attribs[nn].format, FormatNone)
				|| same(attribs[nn].format, FormatDirect) )
			{
			(void) GRA_display_attribute(fld_attrib, attribs[nn],
					flat, flon, xadj, yadj, rotadj, out_buf);
			}

		/* Display the corresponding symbol or text from */
		/*  the chosen look up table                     */
		else if ( same(attribs[nn].format, FormatSymbol)
				|| same(attribs[nn].format, FormatText) )
			{

			/* Use attribute look up table if no special cases */
			/* Note that special cases are not allowed for     */
			/*  complex labels!                                */
			if ( num_list <= 0 || num_attrib > 1 )
				{
				lookup_used = attribs[nn].look_up;
				}

			/* Set look up table based on special cases */
			else
				{
				lookup_used = check_sample_cases(pos, vtime,
						attribs[nn].look_up, list_case, num_list);
				}

			/* Match the value in the chosen look up table */
			if ( match_category_lookup(lookup_used, out_buf,
					&symbol, &text, NullStringPtr) )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout, "   Matching value ... \"%s\"",
							out_buf);
					(void) fprintf(stdout, "  from look up ... %s\n",
							lookup_used);
					}
				if ( same(attribs[nn].format, FormatSymbol) )
					{

					/* Adjust symbol scale for perspective (if required) */
					sscale = attribs[nn].symbol_scale;
					if ( perspective_scale(&pscale) ) sscale *= pscale;

					/* First do the box, ellipse, or underline display */
					if ( !blank(attribs[nn].display_name) )
						{
						(void) GRA_display_attribute_outline(fld_attrib,
								attribs[nn], flat, flon, xadj, yadj, rotadj);
						}

					/* Display symbol only if requested! */
					if ( attribs[nn].show_label )
						{
						(void) rotated_location(attribs[nn].x_attrib,
								attribs[nn].y_attrib, rotadj, &xxs, &yys);
						(void) write_graphics_symbol(symbol,
								(xadj + xxs), (yadj + yys), sscale, rotadj);
						}
					}
				else
					{
					(void) GRA_display_attribute(fld_attrib, attribs[nn],
							flat, flon, xadj, yadj, rotadj, text);
					}
				}
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout, "   Value ... \"%s\"",
							out_buf);
					(void) fprintf(stdout, "  not found in look up ... %s\n",
							lookup_used);
					}
				}
			}

		/* Display wind parameters as wind barbs */
		else if ( same(attribs[nn].format, FormatWindBarb) )
			{
			(void) rotated_location(attribs[nn].x_attrib,
					attribs[nn].y_attrib, rotation, &xxs, &yys);
			(void) GRA_display_windbarb(windval.dir, windval.speed,
					windval.gust, flat, flon, (xadj + xxs), (yadj + yys),
					attribs[nn].width_scale, attribs[nn].height_scale, rotadj);
			}

		/* Display wind parameters as text or symbols  */
		/*  from the given wind look up table          */
		else if ( same(attribs[nn].format, FormatWindText)
				|| same(attribs[nn].format, FormatWindSymbol) )
			{
			(void) rotated_location(attribs[nn].x_attrib,
					attribs[nn].y_attrib, rotadj, &xxs, &yys);
			(void) GRA_display_wind(attribs[nn].format, windval.dir,
					windval.speed, windval.gust, flat, flon,
					(xadj + xxs), (yadj + yys), attribs[nn].width_scale,
					attribs[nn].height_scale, rotadj);
			}

		/* Display vector parameters as text or symbols  */
		/*  from the given vector look up table          */
		else if ( same(attribs[nn].format, FormatVectorText)
				|| same(attribs[nn].format, FormatVectorSymbol) )
			{
			(void) rotated_location(attribs[nn].x_attrib,
					attribs[nn].y_attrib, rotadj, &xxs, &yys);
			(void) GRA_display_vector(attribs[nn].format, windval.dir,
					windval.speed, flat, flon, (xadj + xxs), (yadj + yys),
					attribs[nn].width_scale, attribs[nn].height_scale, rotadj);
			}

		/* Restore the original presentation */
		(void) copy_presentation(&CurPres, &temp_pres);
		}

	/* End grouping for label containing all attributes */
	(void) write_graphics_group(GPGend, NullPointer, 0);

	/* Return TRUE if all went well */
	return TRUE;
	}

static	LOGICAL		GRA_display_sampled_value

	(
	STRING			loc_ident,		/* Current identifier */
	STRING			vtime,			/* Current valid time */
	STRING			time_zone,		/* Time zone (used in strftime) */
	STRING			language,		/* Language (used in strftime) */
	STRING			loc_label,		/* Current label */
	POINT			pos,			/* Current position */
	float			fld_value,
	ATTRIB_DISPLAY	*attribs,		/* Attributes to display */
	int				num_attrib,		/* Number of attributes */
	SPCASE			*list_case,		/* Structure for special cases */
	int				num_list,		/* Number of special cases */
	STRING			inmark,			/* Name of location marker */
	float			markscale,		/* Marker scaling factor (percent) */
	STRING			display_name,	/* Name for label display */
	STRING			display_type,	/* Type of label display */
	LOGICAL			fit_to_map,		/* Fit display within map? */
	STRING			fit_to_map_ref,	/* Reference position for fit */
	float			rotation,		/* Label rotation (degrees) */
	float			xorig,
	float			yorig,
	float			xpos,
	float			ypos
	)

	{
	int				nn;
	float			mscale, sscale, pscale;
	float			flat, flon, xadj, yadj, rotmark, xxs, yys;
	STRING			value, lookup_used, symbol, text;
	ATTRIB_DISPLAY	full_display;
	PRES			temp_pres;
	char			out_buf[GPGLong];

	/* Check for acceptable attributes */
	if ( !check_value_attributes(attribs, num_attrib) )
		{
		return FALSE;
		}

	/* Set the label location */
	(void) pos_to_ll(&BaseMap, pos, &flat, &flon);

	/* Determine the placement of each value */
	for ( nn=0; nn<num_attrib; nn++ )
		{

		/* Extract formatted values for "magic" attributes */
		if ( magic_is_attribute(attribs[nn].name) )
			{
			value = magic_get_attribute(attribs[nn].name, loc_ident, vtime,
						time_zone, language, loc_label,
						pos, DefProximity, DefBearing,
						DefLineLen, DefSegDir, DefSegSpd, FpaCmksUnits,
						attribs[nn].format, attribs[nn].conversion_format);
			(void) strcpy(out_buf, value);
			}

		/* Set blank display for FormatNone */
		else if ( same(attribs[nn].format, FormatNone) )
			{
			(void) strcpy(out_buf, FpaCblank);
			}

		/* Extract formatted value for the field value */
		else
			{
			if ( blank(attribs[nn].conversion_format) )
				{
				(void) sprintf(out_buf, "%.0f", fld_value);
				}
			else
				{
				(void) sprintf(out_buf, attribs[nn].conversion_format,
																	fld_value);
				}
			}

		/* Determine placement for values displayed directly */
		if ( same(attribs[nn].format, FormatNone)
				|| same(attribs[nn].format, FormatDirect) )
			{
			(void) set_attribute_placement(&attribs[nn], out_buf, NullString);
			}

		/* Determine placement for values displayed from */
		/*  the given look up table                      */
		else if ( same(attribs[nn].format, FormatSymbol)
				|| same(attribs[nn].format, FormatText) )
			{

			/* Use attribute look up table if no special cases */
			/* Note that special cases are not allowed for     */
			/*  complex labels!                                */
			if ( num_list <= 0 || num_attrib > 1 )
				{
				lookup_used = attribs[nn].look_up;
				}

			/* Set look up table based on special cases */
			else
				{
				lookup_used = check_sample_cases(pos, vtime,
						attribs[nn].look_up, list_case, num_list);
				}

			/* Match the value in the chosen look up table */
			if ( match_category_lookup(lookup_used, out_buf,
					&symbol, &text, NullStringPtr) )
				{
				if ( same(attribs[nn].format, FormatSymbol) )
					{
					(void) set_attribute_placement(&attribs[nn],
							NullString, symbol);
					}
				else
					{
					(void) set_attribute_placement(&attribs[nn],
							text, NullString);
					}
				}
			}
		}

	/* Start grouping for label containing all values */
	(void) write_graphics_group(GPGstart, NullPointer, 0);

	/* Determine positioning for complete set of values */
	full_display = full_attribute_placement();
	if ( !blank(display_name) )
		{
		(void) strcpy(full_display.display_name, display_name);
		(void) strcpy(full_display.display_type, display_type);
		}

	/* Adjust positioning for values (if requested) */
	xadj    = xpos;
	yadj    = ypos;
	rotmark = rotation;
	if ( fit_to_map )
		{
		if ( !fit_attribute_placement(full_display, fit_to_map_ref,
				rotation, xorig, yorig, xpos, ypos, &xadj, &yadj, &rotmark) )
			{
			(void) warn_report("Problem fitting sample to map!");
			}
		}

	/* Adjust symbol scale for perspective (if required) */
	mscale = markscale;
	if ( perspective_scale(&pscale) ) mscale *= pscale;

	/* Display the mark (if requested) ... not offset! */
	if ( !blank(inmark) )
		{
		(void) write_graphics_symbol(inmark, xorig, yorig, mscale, rotmark);
		}

	/* Display graphics outline for complete set of values */
	if ( !blank(display_name) )
		{
		(void) GRA_display_attribute_outline(NullCal, full_display,
				flat, flon, xadj, yadj, rotation);
		}

	/* Now display each value */
	for ( nn=0; nn<num_attrib; nn++ )
		{

		/* Save the current presentation */
		(void) copy_presentation(&temp_pres, &CurPres);

		/* Reset the current presentation for this attribute */
		(void) copy_presentation(&CurPres, &(attribs[nn].presentation));

		/* Extract formatted values for "magic" attributes */
		if ( magic_is_attribute(attribs[nn].name) )
			{
			value = magic_get_attribute(attribs[nn].name, loc_ident, vtime,
						time_zone, language, loc_label,
						pos, DefProximity, DefBearing,
						DefLineLen, DefSegDir, DefSegSpd, FpaCmksUnits,
						attribs[nn].format, attribs[nn].conversion_format);
			(void) strcpy(out_buf, value);
			}

		/* Set blank display for FormatNone */
		else if ( same(attribs[nn].format, FormatNone) )
			{
			(void) strcpy(out_buf, FpaCblank);
			}

		/* Extract formatted value for the field value */
		else
			{
			if ( blank(attribs[nn].conversion_format) )
				{
				(void) sprintf(out_buf, "%.0f", fld_value);
				}
			else
				{
				(void) sprintf(out_buf, attribs[nn].conversion_format,
																	fld_value);
				}
			}

		/* Display the value directly */
		if ( same(attribs[nn].format, FormatNone)
				|| same(attribs[nn].format, FormatDirect) )
			{
			(void) GRA_display_attribute(NullCal, attribs[nn],
					flat, flon, xadj, yadj, rotation, out_buf);
			}

		/* Display the corresponding symbol or text from */
		/*  the chosen look up table                     */
		else if ( same(attribs[nn].format, FormatSymbol)
				|| same(attribs[nn].format, FormatText) )
			{

			/* Use attribute look up table if no special cases */
			/* Note that special cases are not allowed for     */
			/*  complex labels!                                */
			if ( num_list <= 0 || num_attrib > 1 )
				{
				lookup_used = attribs[nn].look_up;
				}

			/* Set look up table based on special cases */
			else
				{
				lookup_used = check_sample_cases(pos, vtime,
						attribs[nn].look_up, list_case, num_list);
				}

			/* Match the value in the chosen look up table */
			if ( match_category_lookup(lookup_used, out_buf,
					&symbol, &text, NullStringPtr) )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout, "   Matching value ... \"%s\"",
							out_buf);
					(void) fprintf(stdout, "  from look up ... %s\n",
							lookup_used);
					}
				if ( same(attribs[nn].format, FormatSymbol) )
					{

					/* Adjust symbol scale for perspective (if required) */
					sscale = attribs[nn].symbol_scale;
					if ( perspective_scale(&pscale) ) sscale *= pscale;

					/* First do the box, ellipse, or underline display */
					if ( !blank(attribs[nn].display_name) )
						{
						(void) GRA_display_attribute_outline(NullCal,
								attribs[nn], flat, flon, xadj, yadj, rotation);
						}

					/* Display symbol only if requested! */
					if ( attribs[nn].show_label )
						{
						(void) rotated_location(attribs[nn].x_attrib,
								attribs[nn].y_attrib, rotation, &xxs, &yys);
						(void) write_graphics_symbol(symbol,
								(xadj + xxs), (yadj + yys), sscale, rotation);
						}
					}
				else
					{
					(void) GRA_display_attribute(NullCal, attribs[nn],
							flat, flon, xadj, yadj, rotation, text);
					}
				}
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout, "   Value ... \"%s\"",
							out_buf);
					(void) fprintf(stdout, "  not found in look up ... %s\n",
							lookup_used);
					}
				}
			}

		/* Restore the original presentation */
		(void) copy_presentation(&CurPres, &temp_pres);
		}

	/* End grouping for label containing all values */
	(void) write_graphics_group(GPGend, NullPointer, 0);

	/* Return TRUE if all went well */
	return TRUE;
	}

static	LOGICAL		GRA_display_sampled_vector

	(
	STRING			loc_ident,		/* Current identifier */
	STRING			vtime,			/* Current valid time */
	STRING			time_zone,		/* Time zone (used in strftime) */
	STRING			language,		/* Language (used in strftime) */
	STRING			loc_label,		/* Current label */
	POINT			pos,			/* Current position */
	float			magnitude,
	float			direction,
	ATTRIB_DISPLAY	*attribs,		/* Attributes to display */
	int				num_attrib,		/* Number of attributes */
	SPCASE			*list_case,		/* Structure for special cases */
	int				num_list,		/* Number of special cases */
	STRING			inmark,			/* Name of location marker */
	float			markscale,		/* Marker scaling factor (percent) */
	STRING			display_name,	/* Name for label display */
	STRING			display_type,	/* Type of label display */
	LOGICAL			fit_to_map,		/* Fit display within map? */
	STRING			fit_to_map_ref,	/* Reference position for fit */
	float			rotation,		/* Label rotation (degrees) */
	float			xorig,
	float			yorig,
	float			xpos,
	float			ypos
	)

	{
	int				nn;
	float			mscale, sscale, pscale;
	float			flat, flon, dang, xadj, yadj, rotmark, xxs, yys;
	STRING			value, lookup_used, symbol, text;
	ATTRIB_DISPLAY	full_display;
	PRES			temp_pres;
	char			out_buf[GPGLong];

	/* Check for acceptable attributes */
	if ( !check_vector_attributes(attribs, num_attrib) )
		{
		return FALSE;
		}

	/* Set the sample location */
	(void) pos_to_ll(&BaseMap, pos, &flat, &flon);

	/* Adjust the direction to use for winds */
	dang = direction + 180.0;
	if ( dang > 360.0 ) dang -= 360.0;

	/* Determine the placement of each value */
	for ( nn=0; nn<num_attrib; nn++ )
		{

		/* Extract formatted values for "magic" attributes */
		if ( magic_is_attribute(attribs[nn].name) )
			{
			value = magic_get_attribute(attribs[nn].name, loc_ident, vtime,
						time_zone, language, loc_label,
						pos, DefProximity, DefBearing,
						DefLineLen, DefSegDir, DefSegSpd, FpaCmksUnits,
						attribs[nn].format, attribs[nn].conversion_format);
			(void) strcpy(out_buf, value);
			}

		/* Set blank display for FormatNone */
		else if ( same(attribs[nn].format, FormatNone) )
			{
			(void) strcpy(out_buf, FpaCblank);
			}

		/* Extract formatted value for the magnitude */
		else if ( same_ic(attribs[nn].name, AttribEvalSpval) )
			{
			if ( blank(attribs[nn].conversion_format) )
				{
				(void) sprintf(out_buf, "%.0f", magnitude);
				}
			else
				{
				(void) sprintf(out_buf, attribs[nn].conversion_format,
																	magnitude);
				}
			}

		/* Extract formatted value for the magnitude and direction */
		else if ( same_ic(attribs[nn].name, AttribEvalVector) )
			{
			if ( blank(attribs[nn].conversion_format) )
				{
				(void) sprintf(out_buf, "%.0f:%.0f", direction, magnitude);
				}
			else
				{
				(void) sprintf(out_buf, attribs[nn].conversion_format,
														direction, magnitude);
				}
			}

		/* Determine placement for values displayed directly */
		if ( same(attribs[nn].format, FormatNone)
				|| same(attribs[nn].format, FormatDirect) )
			{
			(void) set_attribute_placement(&attribs[nn], out_buf, NullString);
			}

		/* Determine placement for values displayed from */
		/*  the given look up table                      */
		else if ( same(attribs[nn].format, FormatSymbol)
				|| same(attribs[nn].format, FormatText) )
			{

			/* Use attribute look up table if no special cases */
			/* Note that special cases are not allowed for     */
			/*  complex labels!                                */
			if ( num_list <= 0 || num_attrib > 1 )
				{
				lookup_used = attribs[nn].look_up;
				}

			/* Set look up table based on special cases */
			else
				{
				lookup_used = check_sample_cases(pos, vtime,
						attribs[nn].look_up, list_case, num_list);
				}

			/* Match the value in the chosen look up table */
			if ( match_category_lookup(lookup_used, out_buf,
					&symbol, &text, NullStringPtr) )
				{
				if ( same(attribs[nn].format, FormatSymbol) )
					{
					(void) set_attribute_placement(&attribs[nn],
							NullString, symbol);
					}
				else
					{
					(void) set_attribute_placement(&attribs[nn],
							text, NullString);
					}
				}
			}

		/* Determine placement for vector parameters displayed as wind barbs */
		else if ( same(attribs[nn].format, FormatWindBarb) )
			{
			(void) set_wind_barb_placement(&attribs[nn],
					dang, magnitude, magnitude);
			}

		/* Determine placement for vector parameters displayed   */
		/*  as text or symbols from the given wind look up table */
		else if ( same(attribs[nn].format, FormatWindText)
				|| same(attribs[nn].format, FormatWindSymbol) )
			{
			(void) set_wind_placement(attribs[nn].format, &attribs[nn],
					dang, magnitude, magnitude, flat, flon);
			}

		/* Determine placement for vector parameters displayed     */
		/*  as text or symbols from the given vector look up table */
		else if ( same(attribs[nn].format, FormatVectorText)
				|| same(attribs[nn].format, FormatVectorSymbol) )
			{
			(void) set_vector_placement(attribs[nn].format, &attribs[nn],
					direction, magnitude, flat, flon);
			}
		}

	/* Start grouping for label containing all values */
	(void) write_graphics_group(GPGstart, NullPointer, 0);

	/* Determine positioning for complete set of values */
	full_display = full_attribute_placement();
	if ( !blank(display_name) )
		{
		(void) strcpy(full_display.display_name, display_name);
		(void) strcpy(full_display.display_type, display_type);
		}

	/* Adjust positioning for values (if requested) */
	xadj    = xpos;
	yadj    = ypos;
	rotmark = rotation;
	if ( fit_to_map )
		{
		if ( !fit_attribute_placement(full_display, fit_to_map_ref,
				rotation, xorig, yorig, xpos, ypos, &xadj, &yadj, &rotmark) )
			{
			(void) warn_report("Problem fitting sample to map!");
			}
		}

	/* Adjust symbol scale for perspective (if required) */
	mscale = markscale;
	if ( perspective_scale(&pscale) ) mscale *= pscale;

	/* Display the mark (if requested) ... not offset! */
	if ( !blank(inmark) )
		{
		(void) write_graphics_symbol(inmark, xorig, yorig, mscale, rotmark);
		}

	/* Display graphics outline for complete set of values */
	if ( !blank(display_name) )
		{
		(void) GRA_display_attribute_outline(NullCal, full_display,
				flat, flon, xadj, yadj, rotation);
		}

	/* Now display each value */
	for ( nn=0; nn<num_attrib; nn++ )
		{

		/* Save the current presentation */
		(void) copy_presentation(&temp_pres, &CurPres);

		/* Reset the current presentation for this attribute */
		(void) copy_presentation(&CurPres, &(attribs[nn].presentation));

		/* Extract formatted values for "magic" attributes */
		if ( magic_is_attribute(attribs[nn].name) )
			{
			value = magic_get_attribute(attribs[nn].name, loc_ident, vtime,
						time_zone, language, loc_label,
						pos, DefProximity, DefBearing,
						DefLineLen, DefSegDir, DefSegSpd, FpaCmksUnits,
						attribs[nn].format, attribs[nn].conversion_format);
			(void) strcpy(out_buf, value);
			}

		/* Set blank display for FormatNone */
		else if ( same(attribs[nn].format, FormatNone) )
			{
			(void) strcpy(out_buf, FpaCblank);
			}

		/* Extract formatted value for the magnitude */
		else if ( same_ic(attribs[nn].name, AttribEvalSpval) )
			{
			if ( blank(attribs[nn].conversion_format) )
				{
				(void) sprintf(out_buf, "%.0f", magnitude);
				}
			else
				{
				(void) sprintf(out_buf, attribs[nn].conversion_format,
																	magnitude);
				}
			}

		/* Extract formatted value for the magnitude and direction */
		else if ( same_ic(attribs[nn].name, AttribEvalVector) )
			{
			if ( blank(attribs[nn].conversion_format) )
				{
				(void) sprintf(out_buf, "%.0f:%.0f", direction, magnitude);
				}
			else
				{
				(void) sprintf(out_buf, attribs[nn].conversion_format,
														direction, magnitude);
				}
			}

		/* Display the value directly */
		if ( same(attribs[nn].format, FormatNone)
				|| same(attribs[nn].format, FormatDirect) )
			{
			(void) GRA_display_attribute(NullCal, attribs[nn],
					flat, flon, xadj, yadj, rotation, out_buf);
			}

		/* Display the corresponding symbol or text from */
		/*  the chosen look up table                     */
		else if ( same(attribs[nn].format, FormatSymbol)
				|| same(attribs[nn].format, FormatText) )
			{

			/* Use attribute look up table if no special cases */
			/* Note that special cases are not allowed for     */
			/*  complex labels!                                */
			if ( num_list <= 0 || num_attrib > 1 )
				{
				lookup_used = attribs[nn].look_up;
				}

			/* Set look up table based on special cases */
			else
				{
				lookup_used = check_sample_cases(pos, vtime,
						attribs[nn].look_up, list_case, num_list);
				}

			/* Match the value in the chosen look up table */
			if ( match_category_lookup(lookup_used, out_buf,
					&symbol, &text, NullStringPtr) )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout, "   Matching value ... \"%s\"",
							out_buf);
					(void) fprintf(stdout, "  from look up ... %s\n",
							lookup_used);
					}
				if ( same(attribs[nn].format, FormatSymbol) )
					{

					/* Adjust symbol scale for perspective (if required) */
					sscale = attribs[nn].symbol_scale;
					if ( perspective_scale(&pscale) ) sscale *= pscale;

					/* First do the box, ellipse, or underline display */
					if ( !blank(attribs[nn].display_name) )
						{
						(void) GRA_display_attribute_outline(NullCal,
								attribs[nn], flat, flon, xadj, yadj, rotation);
						}

					/* Display symbol only if requested! */
					if ( attribs[nn].show_label )
						{
						(void) rotated_location(attribs[nn].x_attrib,
								attribs[nn].y_attrib, rotation, &xxs, &yys);
						(void) write_graphics_symbol(symbol,
								(xadj + xxs), (yadj + yys), sscale, rotation);
						}
					}
				else
					{
					(void) GRA_display_attribute(NullCal, attribs[nn],
							flat, flon, xadj, yadj, rotation, text);
					}
				}
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout, "   Value ... \"%s\"",
							out_buf);
					(void) fprintf(stdout, "  not found in look up ... %s\n",
							lookup_used);
					}
				}
			}

		/* Display vector parameters as wind barbs */
		else if ( same(attribs[nn].format, FormatWindBarb) )
			{
			(void) rotated_location(attribs[nn].x_attrib,
					attribs[nn].y_attrib, rotation, &xxs, &yys);
			(void) GRA_display_windbarb(dang, magnitude, magnitude,
					flat, flon, (xadj + xxs), (yadj + yys),
					attribs[nn].width_scale, attribs[nn].height_scale,
					rotation);
			}

		/* Display vector parameters as text or symbols */
		/*  from the given wind look up table           */
		else if ( same(attribs[nn].format, FormatWindText)
				|| same(attribs[nn].format, FormatWindSymbol) )
			{
			(void) rotated_location(attribs[nn].x_attrib,
					attribs[nn].y_attrib, rotation, &xxs, &yys);
			(void) GRA_display_wind(attribs[nn].format, dang, magnitude,
					magnitude, flat, flon, (xadj + xxs), (yadj + yys),
					attribs[nn].width_scale, attribs[nn].height_scale,
					rotation);
			}

		/* Display vector parameters as text or symbols */
		/*  from the given vector look up table         */
		else if ( same(attribs[nn].format, FormatVectorText)
				|| same(attribs[nn].format, FormatVectorSymbol) )
			{
			(void) rotated_location(attribs[nn].x_attrib,
					attribs[nn].y_attrib, rotation, &xxs, &yys);
			(void) GRA_display_vector(attribs[nn].format, direction, magnitude,
					flat, flon, (xadj + xxs), (yadj + yys),
					attribs[nn].width_scale, attribs[nn].height_scale,
					rotation);
			}

		/* Restore the original presentation */
		(void) copy_presentation(&CurPres, &temp_pres);
		}

	/* End grouping for label containing all values */
	(void) write_graphics_group(GPGend, NullPointer, 0);

	/* Return TRUE if all went well */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    G R A _ d i s p l a y _ a r r o w                                 *
*    G R A _ d i s p l a y _ a r r o w _ h e a d                       *
*    G R A _ d i s p l a y _ a r r o w _ b a s e                       *
*    G R A _ d i s p l a y _ a r r o w _ r e v b a s e                 *
*                                                                      *
***********************************************************************/

static	LOGICAL		GRA_display_arrow

	(
	STRING		arrow_name,		/* Arrow name (for ends of lines) */
	LINE		line,			/* Line for arrow */
	float		xoff,			/* x offset of line (display units) */
	float		yoff,			/* y offset of line (display units) */
	PRES		pres,			/* Presentation parameters */
	LOGICAL		clip_to_map		/* Clip line to current map? */
	)

	{
	int				nl, il;
	LINE			seg, *segs;
	PRES			temp_pres;
	ARROW_DISPLAY	cur_arrow;

	/* Static buffers to hold clipped line segments */
	static	int		NumXseg = 0;
	static	LINE	*Xsegs  = NullLineList;

	/* Check that line exists */
	if ( IsNull(line) ) return TRUE;
	if ( line->numpts < 2 ) return TRUE;

	/* Destroy static buffers for line segments (if required) */
	for ( il=0; il<NumXseg; il++ ) Xsegs[il] = destroy_line(Xsegs[il]);

	/* Save the current presentation */
	(void) copy_presentation(&temp_pres, &CurPres);

	/* Reset the current presentation to display the arrow features */
	(void) copy_presentation(&CurPres, &pres);

	/* Get the arrow presentation */
	cur_arrow = get_arrow_display(arrow_name);

	/* Clip the arrow features to fit the current map (if requested) */
	if ( clip_to_map )
		{

		/* Clip the line into line segments */
		nl = GRA_clip_line(line, &segs);

		/* Allocate memory for line segments (if required) */
		if ( nl > NumXseg )
			{
			Xsegs = GETMEM(Xsegs, LINE, nl);
			for ( il=NumXseg; il<nl; il++ ) Xsegs[il] = NullLine;
			NumXseg = nl;
			}

		/* Make copies of the line segments                         */
		/*  ... after adjusting based on current anchor and offsets */
		for ( il=0; il<nl; il++ )
			{
			(void) anchored_line(segs[il], xoff, yoff, &seg);
			Xsegs[il] = copy_line(seg);
			}

		/* Display arrows for each line segment */
		for ( il=0; il<nl; il++ )
			{

			/* Check number of points in each line segment */
			if ( Xsegs[il]->numpts < 2 ) continue;

			/* Display arrow head (if requested) */
			if ( same(cur_arrow.features, ArrowHead)
					|| same(cur_arrow.features, ArrowBoth)
					|| same(cur_arrow.features, ArrowBothRev) )
				{
				(void) GRA_display_arrow_head(cur_arrow, Xsegs[il],
						xoff, yoff, clip_to_map);
				}

			/* Set arrow tail (if requested) */
			if ( same(cur_arrow.features, ArrowTail)
					|| same(cur_arrow.features, ArrowBoth) )
				{
				(void) GRA_display_arrow_tail(cur_arrow, Xsegs[il],
						xoff, yoff, clip_to_map);
				}

			/* Set reversed arrow tail (if requested) */
			else if ( same(cur_arrow.features, ArrowBothRev) )
				{
				(void) GRA_display_arrow_revtail(cur_arrow, Xsegs[il],
						xoff, yoff, clip_to_map);
				}
			}
		}

	/* Display the arrow featuress as entered */
	else
		{

		/* Allocate memory to save line (if required) */
		if ( NumXseg < 1 )
			{
			il = NumXseg++;
			Xsegs = GETMEM(Xsegs, LINE, NumXseg);
			Xsegs[il] = NullLine;
			}

		/* Adjust the line based on current anchor and offsets */
		(void) anchored_line(line, xoff, yoff, &seg);
		Xsegs[il] = copy_line(seg);

		/* Display arrow head (if requested) */
		if ( same(cur_arrow.features, ArrowHead)
				|| same(cur_arrow.features, ArrowBoth)
				|| same(cur_arrow.features, ArrowBothRev) )
			{
			(void) GRA_display_arrow_head(cur_arrow, Xsegs[il],
					xoff, yoff, clip_to_map);
			}

		/* Set arrow tail (if requested) */
		if ( same(cur_arrow.features, ArrowTail)
				|| same(cur_arrow.features, ArrowBoth) )
			{
			(void) GRA_display_arrow_tail(cur_arrow, Xsegs[il],
					xoff, yoff, clip_to_map);
			}

		/* Set reversed arrow tail (if requested) */
		else if ( same(cur_arrow.features, ArrowBothRev) )
			{
			(void) GRA_display_arrow_revtail(cur_arrow, Xsegs[il],
					xoff, yoff, clip_to_map);
			}
		}

	/* Restore the original presentation */
	(void) copy_presentation(&CurPres, &temp_pres);

	/* Return TRUE */
	return TRUE;
	}

static	void		GRA_display_arrow_head

	(
	ARROW_DISPLAY	cur_arrow,		/* Arrow display for arrow head */
	LINE			line,			/* Line for arrow head */
	float			xoff,			/* x offset of line (display units) */
	float			yoff,			/* y offset of line (display units) */
	LOGICAL			clip_to_map		/* Clip line to current map? */
	)

	{
	float		xang, xdeg, bang, cos_b, sin_b, rang, sin_r, sin_ra;
	float		xpos, ypos, xend, yend, xret, yret, xx, yy;
	POINT		pos;
	LINE		sego, seg;

	/* Static buffer to hold arrow head feature */
	static	LINE	CurLine = NullLine;

	/* Check that line exists */
	if ( IsNull(line) ) return;
	if ( line->numpts < 2 ) return;

	/* Create or clear the buffer to hold the arrow head as a closed outline */
	if ( IsNull(CurLine) ) CurLine = create_line();
	else                   (void) empty_line(CurLine);

	/* Set angle of line from last two points in line */
	xpos = line->points[line->numpts-1][X];
	ypos = line->points[line->numpts-1][Y];
	xend = line->points[line->numpts-2][X];
	yend = line->points[line->numpts-2][Y];
	xang = (float) fpa_atan2( (double) (ypos - yend), (double) (xpos - xend) );
	xdeg = xang / RAD;

	/* Set angle of arrow barb */
	bang  = RAD * cur_arrow.angle;
	cos_b = (float) cos((double) bang);
	sin_b = (float) sin((double) bang);

	/* Set return angle of arrow barb */
	rang   = RAD * cur_arrow.return_angle;
	sin_r  = (float) sin((double) rang);
	sin_ra = (float) sin((double) (rang - bang));

	/* Set starting point of arrow head from last point in line */
	/*  ... after adding rotated offset!                        */
	(void) rotated_location(cur_arrow.length_off, cur_arrow.width_off, xdeg,
			&xx, &yy);
	xpos  += xx;
	ypos  += yy;
	pos[X] = xpos;
	pos[Y] = ypos;
	(void) add_point_to_line(CurLine, pos);

	/* Set end of arrow barb (including head width) ... rotated! */
	xend   = -cur_arrow.length * cos_b;
	yend   = -cur_arrow.length * sin_b;
	(void) rotated_location(xend, yend, xdeg, &xx, &yy);
	pos[X] = xpos + xx;
	pos[Y] = ypos + yy;
	(void) add_point_to_line(CurLine, pos);
	xend  += cur_arrow.head_length * cur_arrow.length;
	(void) rotated_location(xend, yend, xdeg, &xx, &yy);
	pos[X] = xpos + xx;
	pos[Y] = ypos + yy;
	(void) add_point_to_line(CurLine, pos);

	/* Set end of return arrow barb (including head width) ... rotated! */
	xret   = -cur_arrow.length * sin_ra / sin_r;
	xret  += cur_arrow.head_length * cur_arrow.length;
	yret   = 0.0;
	(void) rotated_location(xret, yret, xdeg, &xx, &yy);
	pos[X] = xpos + xx;
	pos[Y] = ypos + yy;
	(void) add_point_to_line(CurLine, pos);

	/* Set end of second arrow barb (including head width) ... rotated! */
	yend   = -yend;
	(void) rotated_location(xend, yend, xdeg, &xx, &yy);
	pos[X] = xpos + xx;
	pos[Y] = ypos + yy;
	(void) add_point_to_line(CurLine, pos);
	xend  -= cur_arrow.head_length * cur_arrow.length;
	(void) rotated_location(xend, yend, xdeg, &xx, &yy);
	pos[X] = xpos + xx;
	pos[Y] = ypos + yy;
	(void) add_point_to_line(CurLine, pos);

	/* Close arrow head with starting point */
	pos[X] = xpos;
	pos[Y] = ypos;
	(void) add_point_to_line(CurLine, pos);

	/* Clip the arrow head to fit the current map (if requested) */
	if ( clip_to_map )
		{

		/* Convert the arrow head to unanchored state */
		(void) unanchored_line(CurLine, xoff, yoff, &seg);

		/* Clip the arrow head to a single continuous line */
		sego = GRA_clip_outline(seg);
		if ( IsNull(sego) ) return;
		if ( sego->numpts < 2 ) return;

		/* Adjust the clipped arrow head based on current anchor and offsets */
		(void) anchored_line(sego, xoff, yoff, &seg);
		}

	/* Display all other arrow heads as determined */
	else
		{
		seg = CurLine;
		}

	/* Display the arrow head as a feature */
	(void) write_graphics_features(1, &seg, TRUE, TRUE);
	}

static	void		GRA_display_arrow_tail

	(
	ARROW_DISPLAY	cur_arrow,		/* Arrow display for arrow tail */
	LINE			line,			/* Line for arrow tail */
	float			xoff,			/* x offset of line (display units) */
	float			yoff,			/* y offset of line (display units) */
	LOGICAL			clip_to_map		/* Clip line to current map? */
	)

	{
	float		xang, xdeg, bang, cos_b, sin_b, rang, sin_r, sin_ra;
	float		xpos, ypos, xend, yend, xret, yret, xx, yy;
	POINT		pos;
	LINE		sego, seg;

	/* Static buffer to hold arrow tail feature */
	static	LINE	CurLine = NullLine;

	/* Check that line exists */
	if ( IsNull(line) ) return;
	if ( line->numpts < 2 ) return;

	/* Create or clear the buffer to hold the arrow tail as a closed outline */
	if ( IsNull(CurLine) ) CurLine = create_line();
	else                   (void) empty_line(CurLine);

	/* Set angle of line from first two points in line */
	xpos = line->points[0][X];
	ypos = line->points[0][Y];
	xend = line->points[1][X];
	yend = line->points[1][Y];
	xang = (float) fpa_atan2( (double) (yend - ypos), (double) (xend - xpos) );
	xdeg = xang / RAD;

	/* Set angle of arrow barb */
	bang  = RAD * cur_arrow.angle;
	cos_b = (float) cos((double) bang);
	sin_b = (float) sin((double) bang);

	/* Set return angle of arrow barb */
	rang   = RAD * cur_arrow.return_angle;
	sin_r  = (float) sin((double) rang);
	sin_ra = (float) sin((double) (rang - bang));

	/* Set starting point of arrow tail from first point in line */
	/*  ... after adding rotated offset!                         */
	(void) rotated_location(-cur_arrow.length_off, cur_arrow.width_off, xdeg,
			&xx, &yy);
	xpos  += xx;
	ypos  += yy;
	pos[X] = xpos;
	pos[Y] = ypos;
	(void) add_point_to_line(CurLine, pos);

	/* Set end of arrow barb (including tail width) ... rotated! */
	xend   = -cur_arrow.length * cos_b;
	yend   = -cur_arrow.length * sin_b;
	(void) rotated_location(xend, yend, xdeg, &xx, &yy);
	pos[X] = xpos + xx;
	pos[Y] = ypos + yy;
	(void) add_point_to_line(CurLine, pos);
	xend  -= cur_arrow.tail_length * cur_arrow.length;
	(void) rotated_location(xend, yend, xdeg, &xx, &yy);
	pos[X] = xpos + xx;
	pos[Y] = ypos + yy;
	(void) add_point_to_line(CurLine, pos);

	/* Set end of return arrow barb (including tail width) ... rotated! */
	xret   = -cur_arrow.length * sin_ra / sin_r;
	xret  -= cur_arrow.tail_length * cur_arrow.length;
	yret   = 0.0;
	(void) rotated_location(xret, yret, xdeg, &xx, &yy);
	pos[X] = xpos + xx;
	pos[Y] = ypos + yy;
	(void) add_point_to_line(CurLine, pos);

	/* Set end of second arrow barb (including tail width) ... rotated! */
	yend   = -yend;
	(void) rotated_location(xend, yend, xdeg, &xx, &yy);
	pos[X] = xpos + xx;
	pos[Y] = ypos + yy;
	(void) add_point_to_line(CurLine, pos);
	xend  += cur_arrow.tail_length * cur_arrow.length;
	(void) rotated_location(xend, yend, xdeg, &xx, &yy);
	pos[X] = xpos + xx;
	pos[Y] = ypos + yy;
	(void) add_point_to_line(CurLine, pos);

	/* Close arrow tail with starting point */
	pos[X] = xpos;
	pos[Y] = ypos;
	(void) add_point_to_line(CurLine, pos);

	/* Clip the arrow tail to fit the current map (if requested) */
	if ( clip_to_map )
		{

		/* Convert the arrow tail to unanchored state */
		(void) unanchored_line(CurLine, xoff, yoff, &seg);

		/* Clip the arrow tail to a single continuous line */
		sego = GRA_clip_outline(seg);
		if ( IsNull(sego) ) return;
		if ( sego->numpts < 2 ) return;

		/* Adjust the clipped arrow tail based on current anchor and offsets */
		(void) anchored_line(sego, xoff, yoff, &seg);
		}

	/* Display all other arrow tails as determined */
	else
		{
		seg = CurLine;
		}

	/* Display the arrow tail as a feature */
	(void) write_graphics_features(1, &seg, TRUE, TRUE);
	}

static	void		GRA_display_arrow_revtail

	(
	ARROW_DISPLAY	cur_arrow,		/* Arrow display for reversed arrow tail */
	LINE			line,			/* Line for reversed arrow tail */
	float			xoff,			/* x offset of line (display units) */
	float			yoff,			/* y offset of line (display units) */
	LOGICAL			clip_to_map		/* Clip line to current map? */
	)

	{
	float		xang, xdeg, bang, cos_b, sin_b, rang, sin_r, sin_ra;
	float		xpos, ypos, xend, yend, xret, yret, xx, yy;
	POINT		pos;
	LINE		sego, seg;

	/* Static buffer to hold arrow tail feature */
	static	LINE	CurLine = NullLine;

	/* Check that line exists */
	if ( IsNull(line) ) return;
	if ( line->numpts < 2 ) return;

	/* Create or clear the buffer to hold the arrow tail as a closed outline */
	if ( IsNull(CurLine) ) CurLine = create_line();
	else                   (void) empty_line(CurLine);

	/* Set angle of line from first two points in line */
	xpos = line->points[0][X];
	ypos = line->points[0][Y];
	xend = line->points[1][X];
	yend = line->points[1][Y];
	xang = (float) fpa_atan2( (double) (yend - ypos), (double) (xend - xpos) );
	xdeg = xang / RAD;

	/* Set angle of arrow barb (reversed) */
	bang  = RAD * (180.0 - cur_arrow.angle);
	cos_b = (float) cos((double) bang);
	sin_b = (float) sin((double) bang);

	/* Set return angle of arrow barb (reversed) */
	rang   = RAD * (180.0 - cur_arrow.return_angle);
	sin_r  = (float) sin((double) rang);
	sin_ra = (float) sin((double) (rang - bang));

	/* Set starting point of arrow tail from first point in line */
	/*  ... after adding rotated offset!                         */
	(void) rotated_location(-cur_arrow.length_off, cur_arrow.width_off, xdeg,
			&xx, &yy);
	xpos  += xx;
	ypos  += yy;
	pos[X] = xpos;
	pos[Y] = ypos;
	(void) add_point_to_line(CurLine, pos);

	/* Set end of arrow barb (including tail width) ... rotated! */
	xend   = -cur_arrow.length * cos_b;
	yend   = -cur_arrow.length * sin_b;
	(void) rotated_location(xend, yend, xdeg, &xx, &yy);
	pos[X] = xpos + xx;
	pos[Y] = ypos + yy;
	(void) add_point_to_line(CurLine, pos);
	xend  -= cur_arrow.tail_length * cur_arrow.length;
	(void) rotated_location(xend, yend, xdeg, &xx, &yy);
	pos[X] = xpos + xx;
	pos[Y] = ypos + yy;
	(void) add_point_to_line(CurLine, pos);

	/* Set end of return arrow barb (including tail width) ... rotated! */
	xret   = -cur_arrow.length * sin_ra / sin_r;
	xret  -= cur_arrow.tail_length * cur_arrow.length;
	yret   = 0.0;
	(void) rotated_location(xret, yret, xdeg, &xx, &yy);
	pos[X] = xpos + xx;
	pos[Y] = ypos + yy;
	(void) add_point_to_line(CurLine, pos);

	/* Set end of second arrow barb (including tail width) ... rotated! */
	yend   = -yend;
	(void) rotated_location(xend, yend, xdeg, &xx, &yy);
	pos[X] = xpos + xx;
	pos[Y] = ypos + yy;
	(void) add_point_to_line(CurLine, pos);
	xend  += cur_arrow.tail_length * cur_arrow.length;
	(void) rotated_location(xend, yend, xdeg, &xx, &yy);
	pos[X] = xpos + xx;
	pos[Y] = ypos + yy;
	(void) add_point_to_line(CurLine, pos);

	/* Close arrow tail with starting point */
	pos[X] = xpos;
	pos[Y] = ypos;
	(void) add_point_to_line(CurLine, pos);

	/* Clip the arrow tail to fit the current map (if requested) */
	if ( clip_to_map )
		{

		/* Convert the arrow tail to unanchored state */
		(void) unanchored_line(CurLine, xoff, yoff, &seg);

		/* Clip the arrow tail to a single continuous line */
		sego = GRA_clip_outline(seg);
		if ( IsNull(sego) ) return;
		if ( sego->numpts < 2 ) return;

		/* Adjust the clipped arrow tail based on current anchor and offsets */
		(void) anchored_line(sego, xoff, yoff, &seg);
		}

	/* Display all other arrow tails as determined */
	else
		{
		seg = CurLine;
		}

	/* Display the arrow tail as a feature */
	(void) write_graphics_features(1, &seg, TRUE, TRUE);
	}

/***********************************************************************
*                                                                      *
*    G R A _ d i s p l a y _ w i n d b a r b                           *
*    G R A _ d i s p l a y _ w i n d                                   *
*    G R A _ d i s p l a y _ v e c t o r                               *
*                                                                      *
***********************************************************************/

static	LOGICAL		GRA_display_windbarb

	(
	float		dir,
	float		spd,
	float		gst,
	float		flat,
	float		flon,
	float		xpos,
	float		ypos,
	float		wthscale,	/* Width of characters (percent) */
	float		hgtscale,	/* Height of characters (percent) */
	float		rotation	/* Label rotation (degrees) */
	)

	{
	int			ii, ispd, num_1, num_10, num_50;
	float		rspd, rgst;
	float		cscale, hscale, gsize, pscale;
	float		windangle, cos_wind, sin_wind;
	float		barbangle, cos_barb, sin_barb;
	float		gustangle, cos_gust, sin_gust;
	float		shaftlen, barblen, unitslen, barbspace, barbwidth, gustdist;
	float		xend, yend, xx, yy;
	POINT		pos;
	LINE		line;

	/* Static buffer for formatted wind gust */
	static	char	ftext[GPGMedium];

	/* Round the wind speed (if required) */
	if ( BarbDef.speed_round > 0.0 )
		rspd = NINT(spd/BarbDef.speed_round) * BarbDef.speed_round;
	else
		rspd = spd;

	/* Round the wind gust (if required) */
	if ( BarbDef.gust_round > 0.0 )
		rgst = NINT(gst/BarbDef.gust_round) * BarbDef.gust_round;
	else
		rgst = gst;

	/* Adjust symbol scale and text size for perspective (if required) */
	cscale = BarbDef.calm_scale;
	hscale = BarbDef.huge_scale;
	gsize  = BarbDef.gust_size;
	if ( perspective_scale(&pscale) )
		{
		cscale *= pscale;
		hscale *= pscale;
		gsize  *= pscale;
		}

	/* Display "calm" symbol if wind speed is very small */
	if ( rspd < BarbDef.calm_max )
		{
		(void) write_graphics_symbol(BarbDef.calm_symbol, xpos, ypos,
				cscale, 0.0);
		return TRUE;
		}

	/* Display "huge" symbol if wind speed is very large */
	/* Check wind value - if very large, plot a large circle */
	else if ( rspd > BarbDef.huge_min )
		{
		(void) write_graphics_symbol(BarbDef.huge_symbol, xpos, ypos,
				hscale, 0.0);
		return TRUE;
		}

	/* Determine wind direction angles */
	if ( AnchorToMap )
		{
		windangle = RAD * wind_dir_xy(&BaseMap, flat, flon, dir);
		}
	else
		{
		windangle = RAD * (90.0 - dir);
		}
	cos_wind  = (float) cos((double) windangle);
	sin_wind  = (float) sin((double) windangle);

	/* Determine barb angles based on hemisphere */
	if ( flat >= 0.0 )
		{
		barbangle = windangle - RAD * BarbDef.barb_angle;
		}
	else
		{
		barbangle = windangle + RAD * BarbDef.barb_angle;
		}
	cos_barb = (float) cos((double) barbangle);
	sin_barb = (float) sin((double) barbangle);

	/* Determine gust angles based on hemisphere */
	if ( flat >= 0.0 )
		{
		gustangle = windangle - RAD * BarbDef.gust_angle;
		}
	else
		{
		gustangle = windangle + RAD * BarbDef.gust_angle;
		}
	cos_gust = (float) cos((double) gustangle);
	sin_gust = (float) sin((double) gustangle);

	/* Determine barb size parameters based on wind speed */
	ispd   = NINT(rspd);
	num_1  = ispd % 10;
	num_10 = ispd % 50 / 10;
	num_50 = ispd / 50;

	/* Determine spacing parameters based on shaft length */
	shaftlen  = BarbDef.shaft_length;
	barblen   = BarbDef.barb_length * shaftlen;
	unitslen  = (float) num_1 * barblen / 10;
	barbspace = BarbDef.barb_space * shaftlen;
	barbwidth = BarbDef.barb_width * shaftlen;
	gustdist  = BarbDef.gust_distance * shaftlen;

	/* Extend shaft length based on number of 50 unit wind barbs */
	if ( num_50 > 1 ) shaftlen += (float) (num_50 - 1) * barbspace;

	/* Determine end of wind shaft                                  */
	/* Note that first 50 unit barb extends wind shaft by barbwidth */
	xend = xpos + shaftlen * cos_wind;
	yend = ypos + shaftlen * sin_wind;

	/* Create a line object to hold the wind barb as a closed outline */
	line = create_line();

	/* Draw the wind shaft */
	pos[X] = xpos;
	pos[Y] = ypos;
	(void) add_point_to_line(line, pos);
	pos[X] = xend;
	pos[Y] = yend;
	(void) add_point_to_line(line, pos);

	/* Add 50 unit wind barbs                                       */
	/* Note that first 50 unit barb extends wind shaft by barbwidth */
	for ( ii=0; ii<num_50; ii++ )
		{
		pos[X] = xend + barblen * cos_barb;
		pos[Y] = yend + barblen * sin_barb;
		(void) add_point_to_line(line, pos);
		pos[X] = xend + barbwidth * cos_wind;
		pos[Y] = yend + barbwidth * sin_wind;
		(void) add_point_to_line(line, pos);
		xend -= barbspace * cos_wind;
		yend -= barbspace * sin_wind;
		pos[X] = xend;
		pos[Y] = yend;
		(void) add_point_to_line(line, pos);
		}

	/* Add 10 unit wind barbs */
	for ( ii=0; ii<num_10; ii++ )
		{
		pos[X] = xend + barblen * cos_barb;
		pos[Y] = yend + barblen * sin_barb;
		(void) add_point_to_line(line, pos);
		pos[X] = xend;
		pos[Y] = yend;
		(void) add_point_to_line(line, pos);
		xend -= barbspace * cos_wind;
		yend -= barbspace * sin_wind;
		pos[X] = xend;
		pos[Y] = yend;
		(void) add_point_to_line(line, pos);
		}

	/* If no 50 unit or 10 unit wind barbs, indent the units barb */
	if ( (num_50 <= 0) && (num_10 <= 0) )
		{
		xend -= barbspace * cos_wind;
		yend -= barbspace * sin_wind;
		pos[X] = xend;
		pos[Y] = yend;
		(void) add_point_to_line(line, pos);
		}

	/* Add the units barb */
	pos[X] = xend + unitslen * cos_barb;
	pos[Y] = yend + unitslen * sin_barb;
	(void) add_point_to_line(line, pos);
	pos[X] = xend;
	pos[Y] = yend;
	(void) add_point_to_line(line, pos);

	/* Close the wind barb outline */
	pos[X] = xpos;
	pos[Y] = ypos;
	(void) add_point_to_line(line, pos);

	/* Display the wind barb as a feature */
	(void) write_graphics_features(1, &line, TRUE, TRUE);
	line = destroy_line(line);

	/* Return now if gust not required */
	if ( (rgst - rspd) < BarbDef.gust_above ) return TRUE;

	/* Display gust at set location ... centred vertically and outline off! */
	xx = xpos + gustdist * cos_gust;
	yy = ypos + gustdist * sin_gust - gsize / 2.0;
	(void) sprintf(ftext, BarbDef.gust_format, rgst);
	(void) write_graphics_text(ftext, xx, yy, gsize,
			BarbDef.gust_just, rotation, FALSE);

	/* Return TRUE if all went well */
	return TRUE;
	}

static	LOGICAL		GRA_display_wind

	(
	STRING		format,		/* Format of wind to display */
	float		dir,
	float		spd,
	float		gst,
	float		flat,
	float		flon,
	float		xpos,
	float		ypos,
	float		wthscale,	/* Width of characters (percent) */
	float		hgtscale,	/* Height of characters (percent) */
	float		rotation	/* Label rotation (degrees) */
	)

	{
	float		xxc, yyc, xxd, yyd, xxs, yys, xxg, yyg;
	STRING		wformat, text, symbol;
	float		sscale, tsize, pscale;
	float		sym_scale, sym_rotate;

	/* Static buffer for formatted wind values */
	static	char	ftext[GPGMedium];

	/* First display calm conditions */
	wformat = WindDef.calm_type;
	if ( blank(wformat) )
		{
		if      ( same(format, FormatWindText))   wformat = WVsubText;
		else if ( same(format, FormatWindSymbol)) wformat = WVsubSymbol;
		}
	if ( !same(wformat, WVsubNone)
			&& match_wind_lookup(WindDef.wind_lookup, WindCalm,
					wformat, flat, flon, dir, spd, gst,
					&text, &symbol, &sym_scale, &sym_rotate) )
		{

		/* Convert the display location to rotated coordinates */
		(void) rotated_location(WindDef.x_calm, WindDef.y_calm, rotation,
				&xxc, &yyc);

		/* Adjust symbol scale and text size for perspective (if required) */
		sscale = WindDef.calm_scale;
		tsize  = WindDef.calm_size;
		if ( perspective_scale(&pscale) )
			{
			sscale    *= pscale;
			tsize     *= pscale;
			sym_scale *= pscale;
			}

		/* Display calm conditions as a text string */
		if ( same(wformat, WVsubValue) || same(wformat, WVsubText) )
			{
			(void) sprintf(ftext, WindDef.calm_format, text);
			(void) write_graphics_text(ftext, (xpos + xxc), (ypos + yyc),
					tsize, WindDef.calm_just, rotation, TRUE);
			return TRUE;
			}

		/* Display calm conditions as a symbol */
		else if ( same(wformat, WVsubSymbol) )
			{
			(void) write_graphics_symbol(symbol, (xpos + xxc), (ypos + yyc),
					sscale, rotation);
			return TRUE;
			}

		/* Return FALSE if problems encountered */
		else
			{
			return FALSE;
			}
		}

	/* Next display wind direction */
	wformat = WindDef.direction_type;
	if ( blank(wformat) )
		{
		if      ( same(format, FormatWindText))   wformat = WVsubText;
		else if ( same(format, FormatWindSymbol)) wformat = WVsubUniform;
		}
	if ( !same(wformat, WVsubNone)
			&& match_wind_lookup(WindDef.wind_lookup, WindDirection,
					wformat, flat, flon, dir, spd, gst,
					&text, &symbol, &sym_scale, &sym_rotate) )
		{

		/* Convert the display location to rotated coordinates */
		(void) rotated_location(WindDef.x_dir, WindDef.y_dir, rotation,
				&xxd, &yyd);

		/* Adjust symbol scale and text size for perspective (if required) */
		sscale = WindDef.direction_scale;
		tsize  = WindDef.direction_size;
		if ( perspective_scale(&pscale) )
			{
			sscale    *= pscale;
			tsize     *= pscale;
			sym_scale *= pscale;
			}

		/* Display wind direction as a text string */
		if ( same(wformat, WVsubValue) || same(wformat, WVsubText) )
			{
			(void) sprintf(ftext, WindDef.direction_format, text);
			(void) write_graphics_text(ftext, (xpos + xxd), (ypos + yyd),
					tsize, WindDef.direction_just, rotation, TRUE);
			}

		/* Display wind direction as a uniform symbol */
		else if ( same(wformat, WVsubUniform) )
			{
			(void) write_graphics_symbol(symbol, (xpos + xxd), (ypos + yyd),
					sscale, sym_rotate);
			}

		/* Display wind direction as a proportional symbol */
		else if ( same(wformat, WVsubProportional) )
			{
			(void) write_graphics_symbol(symbol, (xpos + xxd), (ypos + yyd),
					sym_scale, sym_rotate);
			}

		/* Return FALSE if problems encountered */
		else
			{
			return FALSE;
			}
		}

	/* Next display wind speed */
	wformat = WindDef.speed_type;
	if ( blank(wformat) )
		{
		if      ( same(format, FormatWindText))   wformat = WVsubText;
		else if ( same(format, FormatWindSymbol)) wformat = WVsubSymbol;
		}
	if ( !same(wformat, WVsubNone)
			&& match_wind_lookup(WindDef.wind_lookup, WindSpeed,
					wformat, flat, flon, dir, spd, gst,
					&text, &symbol, &sym_scale, &sym_rotate) )
		{

		/* Convert the display location to rotated coordinates */
		(void) rotated_location(WindDef.x_spd, WindDef.y_spd, rotation,
				&xxs, &yys);

		/* Adjust symbol scale and text size for perspective (if required) */
		sscale = WindDef.speed_scale;
		tsize  = WindDef.speed_size;
		if ( perspective_scale(&pscale) )
			{
			sscale    *= pscale;
			tsize     *= pscale;
			sym_scale *= pscale;
			}

		/* Display wind speed as a text string */
		if ( same(wformat, WVsubValue) || same(wformat, WVsubText) )
			{
			(void) sprintf(ftext, WindDef.speed_format, text);
			(void) write_graphics_text(ftext, (xpos + xxs), (ypos + yys),
					tsize, WindDef.speed_just, rotation, TRUE);
			}

		/* Display wind speed as a symbol */
		else if ( same(wformat, WVsubSymbol) )
			{
			(void) write_graphics_symbol(symbol, (xpos + xxs), (ypos + yys),
					sscale, rotation);
			}

		/* Return FALSE if problems encountered */
		else
			{
			return FALSE;
			}
		}

	/* Next check for gust speed */
	if ( (gst - spd) < WindDef.gust_above )
		{
		return TRUE;
		}

	/* Next display gust speed */
	wformat = WindDef.gust_type;
	if ( blank(wformat) )
		{
		if      ( same(format, FormatWindText))   wformat = WVsubText;
		else if ( same(format, FormatWindSymbol)) wformat = WVsubSymbol;
		}
	if ( !same(wformat, WVsubNone)
			&& match_wind_lookup(WindDef.wind_lookup, WindGust,
					wformat, flat, flon, dir, spd, gst,
					&text, &symbol, &sym_scale, &sym_rotate) )
		{

		/* Convert the display location to rotated coordinates */
		(void) rotated_location(WindDef.x_gust, WindDef.y_gust, rotation,
				&xxg, &yyg);

		/* Adjust symbol scale and text size for perspective (if required) */
		sscale = WindDef.gust_scale;
		tsize  = WindDef.gust_size;
		if ( perspective_scale(&pscale) )
			{
			sscale    *= pscale;
			tsize     *= pscale;
			sym_scale *= pscale;
			}

		/* Display wind gust as a text string */
		if ( same(wformat, WVsubValue) || same(wformat, WVsubText) )
			{
			(void) sprintf(ftext, WindDef.gust_format, text);
			(void) write_graphics_text(ftext, (xpos + xxg), (ypos + yyg),
					tsize, WindDef.gust_just, rotation, TRUE);
			}

		/* Display wind gust as a symbol */
		else if ( same(wformat, WVsubSymbol) )
			{
			(void) write_graphics_symbol(symbol, (xpos + xxg), (ypos + yyg),
					sscale, rotation);
			}

		/* Return FALSE if problems encountered */
		else
			{
			return FALSE;
			}
		}

	/* Return TRUE if all went well */
	return TRUE;
	}

static	LOGICAL		GRA_display_vector

	(
	STRING		format,		/* Format of vector to display */
	float		dir,
	float		spd,
	float		flat,
	float		flon,
	float		xpos,
	float		ypos,
	float		wthscale,	/* Width of characters (percent) */
	float		hgtscale,	/* Height of characters (percent) */
	float		rotation	/* Label rotation (degrees) */
	)

	{
	float		xxc, yyc, xxd, yyd, xxs, yys;
	STRING		vformat, text, symbol;
	float		sscale, tsize, pscale;
	float		sym_scale, sym_rotate;

	/* Static buffer for formatted vector values */
	static	char	ftext[GPGMedium];

	/* First display calm conditions */
	vformat = VectorDef.calm_type;
	if ( blank(vformat) )
		{
		if      ( same(format, FormatVectorText))   vformat = WVsubText;
		else if ( same(format, FormatVectorSymbol)) vformat = WVsubSymbol;
		}
	if ( !same(vformat, WVsubNone)
			&& match_vector_lookup(VectorDef.vector_lookup, VectorCalm,
					vformat, flat, flon, dir, spd,
					&text, &symbol, &sym_scale, &sym_rotate) )
		{

		/* Convert the display location to rotated coordinates */
		(void) rotated_location(VectorDef.x_calm, VectorDef.y_calm, rotation,
				&xxc, &yyc);

		/* Adjust symbol scale and text size for perspective (if required) */
		sscale = VectorDef.calm_scale;
		tsize  = VectorDef.calm_size;
		if ( perspective_scale(&pscale) )
			{
			sscale    *= pscale;
			tsize     *= pscale;
			sym_scale *= pscale;
			}

		/* Display calm conditions as a text string */
		if ( same(vformat, WVsubValue) || same(vformat, WVsubText) )
			{
			(void) sprintf(ftext, VectorDef.calm_format, text);
			(void) write_graphics_text(ftext, (xpos + xxc), (ypos + yyc),
					tsize, VectorDef.calm_just, rotation, TRUE);
			return TRUE;
			}

		/* Display calm conditions as a symbol */
		else if ( same(vformat, WVsubSymbol) )
			{
			(void) write_graphics_symbol(symbol, (xpos + xxc), (ypos + yyc),
					sscale, rotation);
			return TRUE;
			}

		/* Return FALSE if problems encountered */
		else
			{
			return FALSE;
			}
		}

	/* Next display vector direction */
	vformat = VectorDef.direction_type;
	if ( blank(vformat) )
		{
		if      ( same(format, FormatVectorText))   vformat = WVsubText;
		else if ( same(format, FormatVectorSymbol)) vformat = WVsubUniform;
		}
	if ( !same(vformat, WVsubNone)
			&& match_vector_lookup(VectorDef.vector_lookup, VectorDirection,
					vformat, flat, flon, dir, spd,
					&text, &symbol, &sym_scale, &sym_rotate) )
		{

		/* Convert the display location to rotated coordinates */
		(void) rotated_location(VectorDef.x_dir, VectorDef.y_dir, rotation,
				&xxd, &yyd);

		/* Adjust symbol scale and text size for perspective (if required) */
		sscale = VectorDef.direction_scale;
		tsize  = VectorDef.direction_size;
		if ( perspective_scale(&pscale) )
			{
			sscale    *= pscale;
			tsize     *= pscale;
			sym_scale *= pscale;
			}

		/* Display vector direction as a text string */
		if ( same(vformat, WVsubValue) || same(vformat, WVsubText) )
			{
			(void) sprintf(ftext, VectorDef.direction_format, text);
			(void) write_graphics_text(ftext, (xpos + xxd), (ypos + yyd),
					tsize, VectorDef.direction_just, rotation, TRUE);
			}

		/* Display vector direction as a uniform symbol */
		else if ( same(vformat, WVsubUniform) )
			{
			(void) write_graphics_symbol(symbol, (xpos + xxd), (ypos + yyd),
					sscale, sym_rotate);
			}

		/* Display vector direction as a proportional symbol */
		else if ( same(vformat, WVsubProportional) )
			{
			(void) write_graphics_symbol(symbol, (xpos + xxd), (ypos + yyd),
					sym_scale, sym_rotate);
			}

		/* Return FALSE if problems encountered */
		else
			{
			return FALSE;
			}
		}

	/* Next display vector speed */
	vformat = VectorDef.speed_type;
	if ( blank(vformat) )
		{
		if      ( same(format, FormatVectorText))   vformat = WVsubText;
		else if ( same(format, FormatVectorSymbol)) vformat = WVsubSymbol;
		}
	if ( !same(vformat, WVsubNone)
			&& match_vector_lookup(VectorDef.vector_lookup, VectorSpeed,
					vformat, flat, flon, dir, spd,
					&text, &symbol, &sym_scale, &sym_rotate) )
		{

		/* Convert the display location to rotated coordinates */
		(void) rotated_location(VectorDef.x_spd, VectorDef.y_spd, rotation,
				&xxs, &yys);

		/* Adjust symbol scale and text size for perspective (if required) */
		sscale = VectorDef.speed_scale;
		tsize  = VectorDef.speed_size;
		if ( perspective_scale(&pscale) )
			{
			sscale    *= pscale;
			tsize     *= pscale;
			sym_scale *= pscale;
			}

		/* Display vector speed as a text string */
		if ( same(vformat, WVsubValue) || same(vformat, WVsubText) )
			{
			(void) sprintf(ftext, VectorDef.speed_format, text);
			(void) write_graphics_text(ftext, (xpos + xxs), (ypos + yys),
					tsize, VectorDef.speed_just, rotation, TRUE);
			}

		/* Display vector speed as a symbol */
		else if ( same(vformat, WVsubSymbol) )
			{
			(void) write_graphics_symbol(symbol, (xpos + xxs), (ypos + yys),
					sscale, rotation);
			}

		/* Return FALSE if problems encountered */
		else
			{
			return FALSE;
			}
		}

	/* Return TRUE if all went well */
	return TRUE;
	}
