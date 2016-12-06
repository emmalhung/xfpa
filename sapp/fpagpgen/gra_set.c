/***********************************************************************
*                                                                      *
*     g r a _ s e t . c                                                *
*                                                                      *
*     Routines to set parameters from pdf file directives              *
*                                                                      *
*     Version 5 (c) Copyright 1997 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2003 Environment Canada (MSC)            *
*     Version 7 (c) Copyright 2003 Environment Canada                  *
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

#include "fpagpgen_routines.h"
#include "fpagpgen_structs.h"

/* We need FPA library definitions */
#include <fpa.h>

/* Static storage for map projection information */
static	PROJ_DEF	Projn;
static	MAP_DEF		Mapdef;
static	GRID_DEF	Grid;

/* >>>>> debug testing <<<<< */
static	LOGICAL	DebugMode = FALSE;

/* Internal static functions */
static	int		add_sample_lctn(GRA_LCTN **, GPGsample, float, float, STRING);
static	void	free_sample_lctn(void);

/***********************************************************************
*                                                                      *
*    s e t _ a n c h o r                                               *
*                                                                      *
***********************************************************************/

LOGICAL		set_anchor

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	char		anchor[GPGMedium];
	char		lat[GPGMedium], lon[GPGMedium];
	char		location_ident[GPGMedium], location_look_up[GPGMedium];
	char		vtime[GPGTiny];
	char		cross_section_name[GPGMedium];
	float		xoff, yoff, flat, flon, clon;
	STRING		loclat, loclon;
	LOGICAL		status;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(anchor,             Anchor);
	(void) strcpy(lat,                FpaCblank);
	(void) strcpy(lon,                FpaCblank);
	(void) strcpy(location_ident,     FpaCblank);
	(void) strcpy(location_look_up,   FpaCblank);
	(void) strcpy(vtime,              TVstamp);
	(void) strcpy(cross_section_name, FpaCblank);
	xoff = 0.0;
	yoff = 0.0;
	flat = 0.0;
	flon = 0.0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "ref") )
			{
			(void) strcpy(anchor, action);
			}

		/* PSMet/SVGMet/CorMet application keyword */
		else if ( same(key, "x") )
			{
			(void) sscanf(action, "%f", &xoff);
			}

		/* PSMet/SVGMet/CorMet application keyword */
		else if ( same(key, "y") )
			{
			(void) sscanf(action, "%f", &yoff);
			}

		/* PSMet/SVGMet/CorMet application keyword */
		else if ( same(key, "lat") )
			{
			(void) strcpy(lat, action);
			}

		/* PSMet/SVGMet/CorMet application keyword */
		else if ( same(key, "lon") )
			{
			(void) strcpy(lon, action);
			}

		/* PSMet/SVGMet/CorMet application keyword */
		else if ( same(key, "location_ident") )
			{
			(void) strcpy(location_ident, action);
			}

		/* PSMet/SVGMet/CorMet application keyword */
		else if ( same(key, "location_look_up") )
			{
			(void) strcpy(location_look_up, action);
			}

		/* PSMet/SVGMet/CorMet application keyword */
		else if ( same(key, "valid_time") )
			{
			(void) grid_center(&BaseMap, NullPointPtr, NullFloat, &clon);
			(void) strcpy(vtime, interpret_timestring(action, T0stamp, clon));
			}

		/* PSMet/SVGMet/CorMet application keyword */
		else if ( same(key, "cross_section_name") )
			{
			(void) strcpy(cross_section_name, action);
			}

		/* TexMet application keyword */
		else if ( same(key, "column") )
			{
			(void) sscanf(action, "%f", &xoff);
			}

		/* TexMet application keyword */
		else if ( same(key, "row") )
			{
			(void) sscanf(action, "%f", &yoff);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Set latitude and longitude for AnchorMapLatLon */
	if ( same(anchor, AnchorMapLatLon) )
		{

		/* Set anchor lat/lon from entered lat and lon */
		if ( !blank(lat) && !blank(lon) )
			{
			flat = read_lat(lat, &status);
			if ( !status ) (void) error_report("Problem with lat");
			flon = read_lon(lon, &status);
			if ( !status ) (void) error_report("Problem with lon");
			}

		/* Set anchor lat/lon from look up table location */
		else if ( !blank(location_ident) && !blank(location_look_up) )
			{
			if ( Verbose )
				{
				(void) fprintf(stdout, "   Matching location ident ... \"%s\"",
						location_ident);
				(void) fprintf(stdout, "  from look up ... %s\n",
						location_look_up);
				}
			if ( !match_location_lookup(location_look_up, location_ident,
					vtime, &loclat, &loclon, NullStringPtr) )
				{
				(void) sprintf(err_buf,
						"Error matching \"%s\" in look up ... %s",
						location_ident, location_look_up);
				(void) error_report(err_buf);
				}
			flat = read_lat(loclat, &status);
			if ( !status )
				(void) error_report("Problem with location look up lat");
			flon = read_lon(loclon, &status);
			if ( !status )
				(void) error_report("Problem with location look up lon");
			}

		/* Error if no lat/lon or location look up given */
		else
			{
			(void) error_report("Missing lat/lon or location look up");
			}
		}

	/* Check for missing cross section name for cross section anchors */
	if ( same_start(anchor, AnchorXsectStart) && blank(cross_section_name) )
		(void) error_report("No cross section name");

	/* Now define the anchor position */
	return define_graphics_anchor(anchor, xoff, yoff, flat, flon,
													cross_section_name);
	}

/***********************************************************************
*                                                                      *
*    s e t _ a r r o w _ d i s p l a y                                 *
*                                                                      *
***********************************************************************/

LOGICAL		set_arrow_display

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int				ii;
	char			arrow_name[GPGMedium];
	float			value;
	ARROW_DISPLAY	*arrow_display;
	STRING			key, action;
	char			err_buf[GPGLong];

	/* First find the keyword defining the arrow display name */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "arrow_name") )
			{
			(void) sscanf(action, "%s", arrow_name);
			break;
			}
		}

	/* Return FALSE if keyword defining arrow display name not found */
	if ( ii >= num ) return FALSE;

	/* Add the named arrow display parameters to the list */
	/*  ... or modify the values for this arrow display!  */
	arrow_display = add_arrow_display(arrow_name);

	/* Now process all remaining keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "arrow_name") )
			{
			}

		else if ( same(key, "arrow_length") )
			{
			(void) sscanf(action, "%f", &value);
			arrow_display->length = value;
			}

		else if ( same(key, "arrow_angle") )
			{
			(void) sscanf(action, "%f", &value);
			arrow_display->angle = value;
			}

		else if ( same(key, "return_angle") )
			{
			(void) sscanf(action, "%f", &value);
			arrow_display->return_angle = value;
			}

		else if ( same(key, "length_offset") )
			{
			(void) sscanf(action, "%f", &value);
			arrow_display->length_off = value;
			}

		else if ( same(key, "width_offset") )
			{
			(void) sscanf(action, "%f", &value);
			arrow_display->width_off = value;
			}

		else if ( same(key, "head_length") )
			{
			(void) sscanf(action, "%f", &value);
			arrow_display->head_length = value;
			}

		else if ( same(key, "tail_length") )
			{
			(void) sscanf(action, "%f", &value);
			arrow_display->tail_length = value;
			}

		else if ( same(key, "arrow_features") )
			{
			(void) strcpy(arrow_display->features, action);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Return TRUE */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    s e t _ c o n t o u r _ p r e s e n t a t i o n                   *
*                                                                      *
***********************************************************************/

LOGICAL		set_contour_presentation

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	char		units[GPGMedium], values[GPGMedium], range[GPGMedium];
	PRES		cur_pres;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(units,  FpaCmksUnits);
	(void) strcpy(values, FpaCblank);
	(void) strcpy(range,  FpaCblank);

	/* Set current presentation from default */
	(void) copy_presentation(&cur_pres, &PresDef);

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "units") )
			{
			(void) strcpy(units, action);
			}

		else if ( same(key, "values") )
			{
			(void) strcpy(values, action);
			}

		else if ( same(key, "range") )
			{
			(void) strcpy(range, action);
			}

		else if ( same(key, "line_width") )
			{
			(void) strcpy(cur_pres.line_width, action);
			}

		else if ( same(key, "line_style") )
			{
			(void) strcpy(cur_pres.line_style, action);
			}

		else if ( same(key, "outline") )
			{
			(void) strcpy(cur_pres.outline, action);
			}

		else if ( same(key, "fill") )
			{
			(void) strcpy(cur_pres.fill, action);
			}

		else if ( same(key, "interior_fill") )
			{
			(void) strcpy(cur_pres.interior_fill, action);
			}

		else if ( same(key, "pattern") )
			{
			(void) strcpy(cur_pres.pattern, action);
			}

		else if ( same(key, "pattern_width") )
			{
			(void) strcpy(cur_pres.pattern_width, action);
			}

		else if ( same(key, "pattern_length") )
			{
			(void) strcpy(cur_pres.pattern_length, action);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Error return for missing parameters */
	if ( blank(values) && blank(range) )
		(void) error_report("No values or range");
	if ( IsNull(identify_unit(units)) )
		{
		(void) sprintf(err_buf, "Unknown units: %s", units);
		(void) error_report(err_buf);
		}

	/* Set contour presentation from values */
	if ( !blank(values) )
		{
		if ( !add_contour_values(values, units, &cur_pres) )
			(void) error_report("Error in format of values");
		}

	/* Set contour presentation from range */
	if ( !blank(range) )
		{
		if ( !add_contour_ranges(range, units, &cur_pres) )
			(void) error_report("Error in format of range");
		}

	/* Return TRUE */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    s e t _ d e f i n e _ c r o s s _ s e c t i o n                   *
*                                                                      *
***********************************************************************/

LOGICAL		set_define_cross_section

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	char		cross_section_name[GPGMedium], type[GPGMedium];
	char		location_look_up[GPGMedium], vertical_look_up[GPGMedium];
	float		width, map_scale, height, xoff, yoff;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(cross_section_name, FpaCblank);
	(void) strcpy(type,               FpaCblank);
	(void) strcpy(location_look_up,   FpaCblank);
	(void) strcpy(vertical_look_up,   FpaCblank);
	width     = 0.0;
	map_scale = 0.0;
	height    = 0.0;
	xoff      = 0.0;
	yoff      = 0.0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "cross_section_name") )
			{
			(void) sscanf(action, "%s", cross_section_name);
			}

		else if ( same(key, "type") )
			{
			(void) sscanf(action, "%s", type);
			}

		else if ( same(key, "width") )
			{
			(void) sscanf(action, "%f", &width);
			}

		else if ( same(key, "height") )
			{
			(void) sscanf(action, "%f", &height);
			}

		else if ( same(key, "map_scale") )
			{
			(void) sscanf(action, "%f", &map_scale);
			}

		else if ( same(key, "location_look_up") )
			{
			(void) sscanf(action, "%s", location_look_up);
			}

		else if ( same(key, "vertical_look_up") )
			{
			(void) sscanf(action, "%s", vertical_look_up);
			}

		else if ( same(key, "x_off") )
			{
			(void) sscanf(action, "%f", &xoff);
			}

		else if ( same(key, "y_off") )
			{
			(void) sscanf(action, "%f", &yoff);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Error return for missing parameters */
	if ( blank(cross_section_name) )
		(void) error_report("No cross section name");
	if ( blank(type) )
		(void) error_report("No cross section type");
	if ( width <= 0.0 && map_scale <= 0.0 )
		(void) error_report("No width or map_scale for cross section");
	if ( height <= 0.0 )
		(void) error_report("No height for cross section");
	if ( blank(location_look_up) )
		(void) error_report("No location look up table for cross section");
	if ( blank(vertical_look_up) )
		(void) error_report("No vertical look up table for cross section");

	/* Error return for incorrect cross section type */
	if ( !same(type, XSectTime) && !same(type, XSectTimeRoute)
			&& !same(type, XSectSpace) && !same(type, XSectSpaceRoute) )
		{
		(void) sprintf(err_buf, "Recognized cross section types are: %s %s %s %s",
				XSectTime, XSectTimeRoute, XSectSpace, XSectSpaceRoute);
		(void) error_report(err_buf);
		}

	/* Now define the placement of the map */
	return add_cross_section(cross_section_name, type, width, map_scale, height,
			location_look_up, vertical_look_up, xoff, yoff);
	}

/***********************************************************************
*                                                                      *
*    s e t _ d e f i n e _ l i n e                                     *
*                                                                      *
***********************************************************************/

LOGICAL		set_define_line

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	char		line_name[GPGMedium], line[GPGMedium], sline[GPGHuge];
	STRING		key, action;
	float		value;
	LOGICAL		status;
	char		vbuf[GPGShort], err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(line_name, FpaCblank);
	(void) strcpy(line,      FpaCblank);

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "line_name") )
			{
			(void) strcpy(line_name, action);
			}

		else if ( same(key, "line") )
			{
			(void) strcpy(line, action);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Error return for missing parameters */
	if ( blank(line_name) ) (void) error_report("No line name");
	if ( blank(line) )      (void) error_report("No line values");

	/* Scale the line locations                                         */
	/* Scaling done here to avoid the 256 character limit for keywords! */
	/* Note that an even number of locations has already been checked!  */
	/* Note that correct conversion has already been checked!           */
	(void) strcpy(sline, "");
	while (TRUE)
		{
		value = float_arg(line, &status);
		if ( !status ) break;

		value *= DisplayUnits.conversion;
		(void) sprintf(vbuf, "%f ", value);
		(void) strcat(sline, vbuf);
		}

	/* Now define the line */
	return GRA_define_line(line_name, sline);
	}

/***********************************************************************
*                                                                      *
*    s e t _ d e f i n e _ m a p _ p l a c e m e n t                   *
*                                                                      *
***********************************************************************/

LOGICAL		set_define_map_placement

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	char		axis[GPGMedium];
	float		map_scale, size, xoff, yoff;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(axis, ScaleLongest);
	map_scale = 0.0;
	size      = 0.0;
	xoff      = 0.0;
	yoff      = 0.0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "map_scale") )
			{
			(void) sscanf(action, "%f", &map_scale);
			}

		else if ( same(key, "size") )
			{
			(void) sscanf(action, "%f", &size);
			}

		else if ( same(key, "axis_to_scale") )
			{
			(void) strcpy(axis, action);
			}

		else if ( same(key, "x_off") )
			{
			(void) sscanf(action, "%f", &xoff);
			}

		else if ( same(key, "y_off") )
			{
			(void) sscanf(action, "%f", &yoff);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Error return for missing parameters */
	if ( map_scale <= 0.0 && size <= 0.0 )
		(void) error_report("No map_scale or size");

	/* Now define the placement of the map */
	return define_graphics_placement(map_scale, size, axis, xoff, yoff);
	}

/***********************************************************************
*                                                                      *
*    s e t _ d e f i n e _ s a m p l e _ g r i d                       *
*                                                                      *
***********************************************************************/

LOGICAL		set_define_sample_grid

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	char		grid_name[GPGMedium];
	char		lat_bgn[GPGMedium], lat_end[GPGMedium], lat_int[GPGMedium];
	char		lon_bgn[GPGMedium], lon_end[GPGMedium], lon_int[GPGMedium];
	char		mapx_bgn[GPGMedium], mapx_end[GPGMedium], mapx_int[GPGMedium];
	char		mapy_bgn[GPGMedium], mapy_end[GPGMedium], mapy_int[GPGMedium];
	float		map_units, xsoff, ysoff;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(grid_name, FpaCblank);
	(void) strcpy(lat_bgn,   FpaCblank);
	(void) strcpy(lat_end,   FpaCblank);
	(void) strcpy(lat_int,   FpaCblank);
	(void) strcpy(lon_bgn,   FpaCblank);
	(void) strcpy(lon_end,   FpaCblank);
	(void) strcpy(lon_int,   FpaCblank);
	(void) strcpy(mapx_bgn,  FpaCblank);
	(void) strcpy(mapx_end,  FpaCblank);
	(void) strcpy(mapx_int,  FpaCblank);
	(void) strcpy(mapy_bgn,  FpaCblank);
	(void) strcpy(mapy_end,  FpaCblank);
	(void) strcpy(mapy_int,  FpaCblank);
	map_units = BaseMap.definition.units;
	xsoff     = 0.0;
	ysoff     = 0.0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "grid_name") )
			{
			(void) strcpy(grid_name, action);
			}

		else if ( same(key, "lat_begin") )
			{
			(void) strcpy(lat_bgn, action);
			}

		else if ( same(key, "lat_end") )
			{
			(void) strcpy(lat_end, action);
			}

		else if ( same(key, "lat_interval") )
			{
			(void) strcpy(lat_int, action);
			}

		else if ( same(key, "lon_begin") )
			{
			(void) strcpy(lon_bgn, action);
			}

		else if ( same(key, "lon_end") )
			{
			(void) strcpy(lon_end, action);
			}

		else if ( same(key, "lon_interval") )
			{
			(void) strcpy(lon_int, action);
			}

		else if ( same(key, "map_x_begin") )
			{
			(void) strcpy(mapx_bgn, action);
			}

		else if ( same(key, "map_x_end") )
			{
			(void) strcpy(mapx_end, action);
			}

		else if ( same(key, "map_x_interval") )
			{
			(void) strcpy(mapx_int, action);
			}

		else if ( same(key, "map_y_begin") )
			{
			(void) strcpy(mapy_bgn, action);
			}

		else if ( same(key, "map_y_end") )
			{
			(void) strcpy(mapy_end, action);
			}

		else if ( same(key, "map_y_interval") )
			{
			(void) strcpy(mapy_int, action);
			}

		else if ( same(key, "map_units") )
			{
			(void) sscanf(action, "%f", &map_units);
			if ( map_units <= 0.0 )
				{
				(void) sprintf(err_buf, "Invalid map units ... %s", list[ii]);
				(void) error_report(err_buf);
				}
			}

		else if ( same(key, "x_shift") )
			{
			(void) sscanf(action, "%f", &xsoff);
			}

		else if ( same(key, "y_shift") )
			{
			(void) sscanf(action, "%f", &ysoff);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Error return for missing parameters */
	if ( blank(grid_name) ) (void) error_report("No grid name");
	if ( ( blank(lat_bgn) || blank(lat_end) || blank(lat_int)
			|| blank(lon_bgn) || blank(lon_end) || blank(lon_int) )
		&& ( blank(mapx_bgn) || blank(mapx_end) || blank(mapx_int)
			|| blank(mapy_bgn) || blank(mapy_end) || blank(mapy_int) ) )
		(void) error_report("Missing lat/lon or map_x/map_y parameters");

	/* Now add the grid to the list */
	return add_sample_grid(grid_name, lat_bgn, lat_end, lat_int,
			lon_bgn, lon_end, lon_int, mapx_bgn, mapx_end, mapx_int,
			mapy_bgn, mapy_end, mapy_int, map_units, xsoff, ysoff);
	}

/***********************************************************************
*                                                                      *
*    s e t _ d e f i n e _ s a m p l e _ l i s t                       *
*                                                                      *
***********************************************************************/

LOGICAL		set_define_sample_list

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	LOGICAL		displayok;
	int			ii;
	STRING		next;
	char		list_name[GPGMedium];
	int			num_lctns;
	GRA_LCTN	*lctns;
	int			xwrap, ywrap;
	float		flat, flon, mapx, mapy, map_units, xsoff, ysoff, xval;
	LOGICAL		status;
	STRING		key, action;
	char		tbuf[GPGMedium], err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(list_name, FpaCblank);
	map_units = BaseMap.definition.units;
	xsoff     = 0.0;
	ysoff     = 0.0;
	xwrap     =   1;
	ywrap     =   1;
	num_lctns =   0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "list_name") )
			{
			(void) strcpy(list_name, action);
			}

		else if ( same(key, "lat_lon_ident") )
			{
			(void) strcpy(tbuf, action);
			next = string_arg(tbuf);
			flat = read_lat(next, &status);
			if ( !status )
				{
				(void) sprintf(err_buf,
						"Problem with lat in ... %s", list[ii]);
				(void) error_report(err_buf);
				}
			next = string_arg(tbuf);
			flon = read_lon(next, &status);
			if ( !status )
				{
				(void) sprintf(err_buf,
						"Problem with lon in ... %s", list[ii]);
				(void) error_report(err_buf);
				}
			next = string_arg(tbuf);
			num_lctns = add_sample_lctn(&lctns, GPG_LatLon, flat, flon, next);
			}

		else if ( same(key, "map_x_y_ident") )
			{
			(void) strcpy(tbuf, action);
			next = string_arg(tbuf);
			if ( blank(next) )
				{
				(void) sprintf(err_buf,
						"Problem with map_x in ... %s", list[ii]);
				(void) error_report(err_buf);
				}
			(void) sscanf(next, "%f", &mapx);
			next = string_arg(tbuf);
			if ( blank(next) )
				{
				(void) sprintf(err_buf,
						"Problem with map_y in ... %s", list[ii]);
				(void) error_report(err_buf);
				}
			(void) sscanf(next, "%f", &mapy);
			next = string_arg(tbuf);
			num_lctns = add_sample_lctn(&lctns, GPG_MapXY, mapx, mapy, next);
			}

		else if ( same(key, "map_units") )
			{
			(void) sscanf(action, "%f", &map_units);
			if ( map_units <= 0.0 )
				{
				(void) sprintf(err_buf, "Invalid map units ... %s", list[ii]);
				(void) error_report(err_buf);
				}
			}

		else if ( same(key, "location_ident") )
			{
			if ( blank(action) )
				{
				(void) sprintf(err_buf,
						"Problem with identifier in ... %s", list[ii]);
				(void) error_report(err_buf);
				}
			num_lctns = add_sample_lctn(&lctns, GPG_Ident, 0.0, 0.0, action);
			}

		else if ( same(key, "location_ident_list") )
			{
			if ( blank(action) )
				{
				(void) sprintf(err_buf,
						"Problem with identifier(s) in ... %s", list[ii]);
				(void) error_report(err_buf);
				}
			(void) strcpy(tbuf, action);
			while ( !blank( next = string_arg(tbuf) ) )
				{
				num_lctns = add_sample_lctn(&lctns, GPG_Ident, 0.0, 0.0, next);
				}
			}

		else if ( same(key, "x_shift") )
			{
			(void) sscanf(action, "%f", &xsoff);
			}

		else if ( same(key, "y_shift") )
			{
			(void) sscanf(action, "%f", &ysoff);
			}

		else if ( same(key, "x_wrap") )
			{
			(void) sscanf(action, "%f", &xval);
			xwrap = NINT(xval);
			}

		else if ( same(key, "y_wrap") )
			{
			(void) sscanf(action, "%f", &xval);
			ywrap = NINT(xval);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Error return for missing parameters */
	if ( blank(list_name) ) (void) error_report("No list name");
	if ( num_lctns <= 0 )   (void) error_report("No sampling locations");

	/* Check "x_wrap" "y_wrap" keywords */
	if ( xwrap < 1 )
		(void) error_report("Cannot set \"x_wrap\" less than 1!");
	if ( ywrap < 1 )
		(void) error_report("Cannot set \"y_wrap\" less than 1!");
	if ( xwrap > 1 && ywrap > 1 )
		(void) error_report("Cannot set both \"x_wrap\" and \"y_wrap\" greater than 1!");

	/* Now add the sampling list to the list */
	displayok = add_sample_list(list_name, num_lctns, lctns, map_units,
				xsoff, ysoff, xwrap, ywrap);

	/* Free sampling location buffer */
	(void) free_sample_lctn();

	/* Return results of defining sample list */
	return displayok;
	}

/***********************************************************************
*                                                                      *
*    s e t _ d e f i n e _ t a b l e                                   *
*                                                                      *
***********************************************************************/

LOGICAL		set_define_table

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	char		table_name[GPGMedium], type[GPGMedium];
	float		xoff, yoff;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(table_name, FpaCblank);
	(void) strcpy(type,       FpaCblank);
	xoff = 0.0;
	yoff = 0.0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "table_name") )
			{
			(void) strcpy(table_name, action);
			}

		else if ( same(key, "type") )
			{
			(void) strcpy(type, action);
			}

		else if ( same(key, "x_off") )
			{
			(void) sscanf(action, "%f", &xoff);
			}

		else if ( same(key, "y_off") )
			{
			(void) sscanf(action, "%f", &yoff);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Error return for missing parameters */
	if ( blank(table_name) ) (void) error_report("No table name");
	if ( blank(type) )       (void) error_report("No table type");

	/* Error return for incorrect table type */
	if ( !same(type, TableCol) && !same(type, TableRow) )
		{
		(void) sprintf(err_buf, "Recognized table types are: %s %s",
				TableCol, TableRow);
		(void) error_report(err_buf);
		}

	/* Now add the table to the list */
	return add_table(table_name, type, xoff, yoff);
	}

/***********************************************************************
*                                                                      *
*    s e t _ d i s p l a y _ u n i t s                                 *
*                                                                      *
***********************************************************************/

LOGICAL		set_display_units

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	char		type[GPGMedium];
	float		scale_factor;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(type, FpaCblank);
	scale_factor = 100.0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "type") )
			{
			(void) strcpy(type, action);
			}

		else if ( same(key, "scale_factor") )
			{
			(void) sscanf(action, "%f", &scale_factor);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Now define the display units */
	return define_graphics_units(type, scale_factor);
	}

/***********************************************************************
*                                                                      *
*    s e t _ f i l t e r                                               *
*                                                                      *
***********************************************************************/

LOGICAL		set_filter

	(
	char		list[][GPGMedium],
	int			num
	)

	{

	/* Only one value is allowed */
	if ( num != 1 ) return FALSE;

	/* Set filter value */
	if ( sscanf(list[0], "%f", &PolyFilter) )
		{
		if ( PolyFilter < 0.0 ) PolyFilter = 0.0;
		PolyFilter *= DisplayUnits.conversion;
		if ( Verbose )
			{
			(void) fprintf(stdout, " Reset filter value ... %f", PolyFilter);
			(void) fprintf(stdout, " (1000ths of an inch)\n");
			}
		return TRUE;
		}

	/* Return FALSE if problem reading filter */
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*    s e t _ g e o _ p r e s e n t a t i o n                           *
*                                                                      *
***********************************************************************/

LOGICAL		set_geo_presentation

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	char		geo_name[GPGMedium];
	PRES		*geo_pres;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* First find the keyword defining the geographical name */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "geo_name") )
			{
			(void) sscanf(action, "%s", geo_name);
			break;
			}

		/* >>> this gets replaced by "geo_name" above <<< */
		else if ( ObsoleteVersion && same(key, "subelement") )
			{
			(void) fprintf(stderr, ">>> Obsolete keyword in directive");
			(void) fprintf(stderr, " ... \"@geo_presentation\" <<<\n");
			(void) fprintf(stderr, ">>>   Replace \"subelement\"");
			(void) fprintf(stderr, " by \"geo_name\" <<<\n");
			(void) sscanf(action, "%s", geo_name);
			break;
			}
		}

	/* Return FALSE if keyword defining geographical name not found */
	if ( ii >= num )
		{
		(void) sprintf(err_buf, "No keyword: \"geo_name\"");
		(void) error_report(err_buf);
		}

	/* Add the named geographical presentation to the list */
	/*  ... or modify the values for this presentation!    */
	geo_pres = add_geo_presentation(geo_name);

	/* Now process all remaining keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "geo_name") )
			{
			}

		/* >>> this gets replaced by "geo_name" above <<< */
		else if ( ObsoleteVersion && same(key, "subelement") )
			{
			}

		else if ( same(key, "line_width") )
			{
			(void) strcpy(geo_pres->line_width, action);
			}

		else if ( same(key, "line_style") )
			{
			(void) strcpy(geo_pres->line_style, action);
			}

		else if ( same(key, "outline") )
			{
			(void) strcpy(geo_pres->outline, action);
			}

		else if ( same(key, "interior_fill") )
			{
			(void) strcpy(geo_pres->interior_fill, action);
			}

		else if ( same(key, "symbol_fill_name") )
			{
			(void) strcpy(geo_pres->sym_fill_name, action);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Return TRUE */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    s e t _ g r o u p _ b e g i n                                     *
*                                                                      *
***********************************************************************/

LOGICAL		set_group_begin

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	/* >>>>> for testing <<<<< */
	int		ii;
	STRING	key, action;
	/* >>>>> for testing <<<<< */

	/* Start a group with no parameters */
	if ( num == 0 )
		{
		(void) write_graphics_group(GPGstart, NullPointer, 0);
		return TRUE;
		}

	/* Start a group with optional program parameters */
	else if ( num >= 1 )
		{
		/* >>>>> for testing <<<<< */
		if ( DebugMode )
			{
			for ( ii=0; ii<num; ii++ )
				{
				(void) parse_program_instruction(list[ii], &key, &action);
				if ( same(key, KeyNone) )
					(void) fprintf(stdout,
							" Group begin with ... %s\n", action);
				else
					(void) fprintf(stdout,
							" Group begin with ... %s = %s\n", key, action);
				}
			}
		/* >>>>> for testing <<<<< */
		(void) write_graphics_group(GPGstart, list, num);
		return TRUE;
		}

	/* Return FALSE if problem with parameters */
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*    s e t _ l a b e l _ d i s p l a y                                 *
*                                                                      *
***********************************************************************/

LOGICAL		set_label_display

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int				ii;
	char			display_name[GPGMedium];
	float			value;
	LABEL_DISPLAY	*label_display;
	STRING			key, action;
	char			err_buf[GPGLong];

	/* First find the keyword defining the label display name */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "display_name") )
			{
			(void) sscanf(action, "%s", display_name);
			break;
			}
		}

	/* Return FALSE if keyword defining label display name not found */
	if ( ii >= num ) return FALSE;

	/* Add the named label display parameters to the list */
	/*  ... or modify the values for this label display!  */
	label_display = add_label_display(display_name);

	/* Initialize default units for width and height */
	(void) strcpy(label_display->attrib_units, LabelDisplayUnitsKm);

	/* Now process all remaining keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "display_name") )
			{
			}

		else if ( same(key, "width") )
			{
			(void) sscanf(action, "%f", &value);
			label_display->width = value;
			}

		else if ( same(key, "height") )
			{
			(void) sscanf(action, "%f", &value);
			label_display->height = value;
			}

		else if ( same(key, "diameter") )
			{
			(void) sscanf(action, "%f", &value);
			label_display->width  = value;
			label_display->height = value;
			}

		else if ( same(key, "radius") )
			{
			(void) sscanf(action, "%f", &value);
			label_display->width  = 2.0 * value;
			label_display->height = 2.0 * value;
			}

		else if ( same(key, "width_attribute") )
			{
			(void) strcpy(label_display->width_attrib, action);
			}

		else if ( same(key, "height_attribute") )
			{
			(void) strcpy(label_display->height_attrib, action);
			}

		else if ( same(key, "diameter_attribute") )
			{
			(void) strcpy(label_display->diameter_attrib, action);
			}

		else if ( same(key, "radius_attribute") )
			{
			(void) strcpy(label_display->radius_attrib, action);
			}

		else if ( same(key, "attribute_units") )
			{
			if ( IsNull(identify_unit(action)) )
				{
				(void) sprintf(err_buf,
						"Unknown attribute units: %s", action);
				(void) error_report(err_buf);
				}
			(void) strcpy(label_display->attrib_units, action);
			}

		else if ( same(key, "start_angle") )
			{
			(void) sscanf(action, "%f", &value);
			label_display->start_angle = value;
			}

		else if ( same(key, "end_angle") )
			{
			(void) sscanf(action, "%f", &value);
			label_display->end_angle = value;
			}

		else if ( same(key, "closed") )
			{
			if ( same_ic(action, LogicalYes) ) label_display->closed = TRUE;
			else                               label_display->closed = FALSE;
			}

		else if ( same(key, "rotation") )
			{
			(void) sscanf(action, "%f", &value);
			label_display->rotation = value;
			}

		else if ( same(key, "rotation_attribute") )
			{
			(void) strcpy(label_display->rot_attrib, action);
			}

		else if ( same(key, "x_off") )
			{
			(void) sscanf(action, "%f", &value);
			label_display->x_off = value;
			}

		else if ( same(key, "y_off") )
			{
			(void) sscanf(action, "%f", &value);
			label_display->y_off = value;
			}

		else if ( same(key, "margin_left") )
			{
			(void) sscanf(action, "%f", &value);
			label_display->margin_left = value;
			}

		else if ( same(key, "margin_right") )
			{
			(void) sscanf(action, "%f", &value);
			label_display->margin_right = value;
			}

		else if ( same(key, "margin_top") )
			{ (void) sscanf(action, "%f", &value);
			label_display->margin_top = value;
			}

		else if ( same(key, "margin_bottom") )
			{
			(void) sscanf(action, "%f", &value);
			label_display->margin_bottom = value;
			}

		else if ( same(key, "margin_width") )
			{
			(void) sscanf(action, "%f", &value);
			label_display->margin_left  = value;
			label_display->margin_right = value;
			}

		else if ( same(key, "margin_height") )
			{
			(void) sscanf(action, "%f", &value);
			label_display->margin_top    = value;
			label_display->margin_bottom = value;
			}

		else if ( same(key, "line_width") )
			{
			(void) strcpy(label_display->presentation.line_width, action);
			}

		else if ( same(key, "line_style") )
			{
			(void) strcpy(label_display->presentation.line_style, action);
			}

		else if ( same(key, "outline") )
			{
			(void) strcpy(label_display->presentation.outline, action);
			}

		else if ( same(key, "interior_fill") )
			{
			(void) strcpy(label_display->presentation.interior_fill, action);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Return TRUE */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    s e t _ m a p d e f                                               *
*                                                                      *
***********************************************************************/

LOGICAL		set_mapdef

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	float		gridlen, map_units, xmin, xmax, ymin, ymax;
	LOGICAL		status;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Set grid length and map_units from current basemap grid */
	gridlen   = BaseMap.grid.gridlen;
	map_units = BaseMap.grid.units;

	/* Return immediately if invalid grid definition */
	if ( gridlen <= 0.0 || map_units <= 0.0 ) return FALSE;

	/* Initialize map definition parameters from current basemap */
	Mapdef.olat  = BaseMap.definition.olat;
	Mapdef.olon  = BaseMap.definition.olon;
	Mapdef.lref  = BaseMap.definition.lref;
	xmin         = -BaseMap.definition.xorg;
	ymin         = -BaseMap.definition.yorg;
	xmax         = xmin + BaseMap.definition.xlen;
	ymax         = ymin + BaseMap.definition.ylen;
	Mapdef.units = BaseMap.definition.units;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "olat") )
			{
			Mapdef.olat = read_lat(action, &status);
			if ( !status ) return FALSE;
			}

		else if ( same(key, "olon") )
			{
			Mapdef.olon = read_lon(action, &status);
			if ( !status ) return FALSE;
			}

		else if ( same(key, "rlon") )
			{
			Mapdef.lref = read_lon(action, &status);
			if ( !status ) return FALSE;
			}

		else if ( same(key, "xmin") )
			{
			(void) sscanf(action, "%f", &xmin);
			}

		else if ( same(key, "ymin") )
			{
			(void) sscanf(action, "%f", &ymin);
			}

		else if ( same(key, "xmax") )
			{
			(void) sscanf(action, "%f", &xmax);
			}

		else if ( same(key, "ymax") )
			{
			(void) sscanf(action, "%f", &ymax);
			}

		else if ( same(key, "map_units") )
			{
			(void) sscanf(action, "%f", &Mapdef.units);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Define map definition as specified */
	Mapdef.xorg  = -xmin;
	Mapdef.yorg  = -ymin;
	Mapdef.xlen  = xmax - xmin;
	Mapdef.ylen  = ymax - ymin;

	/* Check for valid map definition */
	if ( Mapdef.units <= 0.0 || Mapdef.xlen <= 0.0 || Mapdef.ylen <= 0.0 )
			return FALSE;

	/* Define the grid dimensions */
	(void) set_grid_def_units(&BaseMap.grid, Mapdef.units);
	BaseMap.grid.nx = 1 + (int) (fabs(Mapdef.xlen) / gridlen + 0.75);
	BaseMap.grid.ny = 1 + (int) (fabs(Mapdef.ylen) / gridlen + 0.75);

	/* Define the map projection in the default field descriptor */
	(void) define_map_projection(&BaseMap, &BaseMap.projection,
									&Mapdef, &BaseMap.grid);
	(void) set_fld_descript(&Fdesc, FpaF_MAP_PROJECTION, &BaseMap,
									FpaF_END_OF_LIST);

	/* Print out the adjusted map and grid definitions */
	(void) fprintf(stdout, " Adjusted map definition ...");
	(void) fprintf(stdout, "  olat: %.1f  olon: %.1f  rlon: %.1f\n",
			BaseMap.definition.olat, BaseMap.definition.olon,
			BaseMap.definition.lref);
	(void) fprintf(stdout, "       xorg: %.0f  yorg: %.0f",
			BaseMap.definition.xorg, BaseMap.definition.yorg);
	(void) fprintf(stdout, "  xlen: %.0f  ylen: %.0f  map_units: %.0f\n",
			BaseMap.definition.xlen, BaseMap.definition.ylen,
			BaseMap.definition.units);
	(void) fprintf(stdout, " Adjusted grid definition ...");
	(void) fprintf(stdout, "  nx: %d  ny: %d  gridlen: %.0f\n",
			BaseMap.grid.nx, BaseMap.grid.ny, BaseMap.grid.gridlen);
	(void) fprintf(stdout, "       xgrid: %.0f  ygrid: %.0f",
			BaseMap.grid.xgrid, BaseMap.grid.ygrid);
	(void) fprintf(stdout, "  map_units: %.0f\n",
			BaseMap.grid.units);

	/* Return TRUE */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    s e t _ p e r s p e c t i v e _ v i e w                           *
*                                                                      *
***********************************************************************/

LOGICAL		set_perspective_view

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	LOGICAL		perspective_view, scale_to_perspective;
	float		ytoff, tilt, xeye, yeye, zeye, xstretch, ystretch;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Initialize all parameters */
	perspective_view     = TRUE;
	scale_to_perspective = FALSE;
	ytoff    =   0.0;
	tilt     =   0.0;
	xeye     =   0.0;
	yeye     =   0.0;
	zeye     =   0.0;
	xstretch = 100.0;
	ystretch = 100.0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "show_perspective_view") )
			{
			if ( same_ic(action, "no") ) perspective_view = FALSE;
			else                         perspective_view = TRUE;
			}

		else if ( same(key, "scale_to_perspective") )
			{
			if ( same_ic(action, LogicalYes) ) scale_to_perspective = TRUE;
			else                               scale_to_perspective = FALSE;
			}

		else if ( same(key, "y_tilt_off") )
			{
			(void) sscanf(action, "%f", &ytoff);
			}

		else if ( same(key, "tilt_angle") )
			{
			(void) sscanf(action, "%f", &tilt);
			}

		else if ( same(key, "x_eye") )
			{
			(void) sscanf(action, "%f", &xeye);
			}

		else if ( same(key, "y_eye") )
			{
			(void) sscanf(action, "%f", &yeye);
			}

		else if ( same(key, "z_eye") )
			{
			(void) sscanf(action, "%f", &zeye);
			}

		else if ( same(key, "x_stretch") )
			{
			(void) sscanf(action, "%f", &xstretch);
			}

		else if ( same(key, "y_stretch") )
			{
			(void) sscanf(action, "%f", &ystretch);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Now define the perspective view */
	return define_perspective_view(perspective_view, scale_to_perspective,
			ytoff, tilt, xeye, yeye, zeye, xstretch, ystretch);
	}

/***********************************************************************
*                                                                      *
*    s e t _ p r e s e n t a t i o n                                   *
*                                                                      *
***********************************************************************/

LOGICAL		set_presentation

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	char		name[GPGMedium];
	char		outline_first[GPGMedium];
	char		interior_fill[GPGMedium], sym_fill_name[GPGMedium];
	char		pattern[GPGMedium];
	char		pattern_width[GPGMedium], pattern_length[GPGMedium];
	char		font[GPGMedium], font_weight[GPGMedium];
	char		italics[GPGMedium], justification[GPGMedium];
	char		text_size[GPGMedium];
	char		char_space[GPGMedium], word_space[GPGMedium];
	char		line_space[GPGMedium];
	int			num_comp, num_width, num_style, num_outline, num_fill;
	PRES		*named_pres;
	COMP_PRES	*comp_pres;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Initialize all presentation parameters */
	(void) strcpy(name,           FpaCblank);
	(void) strcpy(outline_first,  FpaCblank);
	(void) strcpy(interior_fill,  FpaCblank);
	(void) strcpy(sym_fill_name,  FpaCblank);
	(void) strcpy(pattern,        FpaCblank);
	(void) strcpy(pattern_width,  FpaCblank);
	(void) strcpy(pattern_length, FpaCblank);
	(void) strcpy(font,           FpaCblank);
	(void) strcpy(font_weight,    FpaCblank);
	(void) strcpy(italics,        FpaCblank);
	(void) strcpy(justification,  FpaCblank);
	(void) strcpy(text_size,      FpaCblank);
	(void) strcpy(char_space,     FpaCblank);
	(void) strcpy(word_space,     FpaCblank);
	(void) strcpy(line_space,     FpaCblank);
	num_comp    = 0;
	num_width   = 0;
	num_style   = 0;
	num_outline = 0;
	num_fill    = 0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "name") )
			{
			(void) strcpy(name, action);
			}

		else if ( same(key, "line_width") )
			{
			num_comp = add_comp_pres(&comp_pres, ++num_width);
			(void) strcpy(comp_pres[num_width-1].line_width, action);
			}

		else if ( same(key, "line_style") )
			{
			num_comp = add_comp_pres(&comp_pres, ++num_style);
			(void) strcpy(comp_pres[num_style-1].line_style, action);
			}

		else if ( same(key, "outline") )
			{
			num_comp = add_comp_pres(&comp_pres, ++num_outline);
			(void) strcpy(comp_pres[num_outline-1].outline, action);
			}

		else if ( same(key, "fill") )
			{
			num_comp = add_comp_pres(&comp_pres, ++num_fill);
			(void) strcpy(comp_pres[num_fill-1].fill, action);
			}

		else if ( same(key, "outline_first") )
			{
			(void) strcpy(outline_first, action);
			}

		else if ( same(key, "interior_fill") )
			{
			(void) strcpy(interior_fill, action);
			}

		else if ( same(key, "symbol_fill_name") )
			{
			(void) strcpy(sym_fill_name, action);
			}

		else if ( same(key, "pattern") )
			{
			(void) strcpy(pattern, action);
			}

		else if ( same(key, "pattern_width") )
			{
			(void) strcpy(pattern_width, action);
			}

		else if ( same(key, "pattern_length") )
			{
			(void) strcpy(pattern_length, action);
			}

		else if ( same(key, "font") )
			{
			(void) strcpy(font, action);
			}

		else if ( same(key, "font_weight") )
			{
			(void) strcpy(font_weight, action);
			}

		else if ( same(key, "italics") )
			{
			(void) strcpy(italics, action);
			}

		else if ( same(key, "justification") )
			{
			(void) strcpy(justification, action);
			}

		else if ( same(key, "text_size") )
			{
			(void) strcpy(text_size, action);
			}

		else if ( same(key, "char_space") )
			{
			(void) strcpy(char_space, action);
			}

		else if ( same(key, "word_space") )
			{
			(void) strcpy(word_space, action);
			}

		else if ( same(key, "line_space") )
			{
			(void) strcpy(line_space, action);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Reset default (un-named) presentation */
	if ( blank(name) )
		{

		/* Ensure that local component presentation is filled in */
		(void) check_comp_pres();

		/* First reset default presentation parameters from last */
		/*  local component presentation                         */
		if ( num_comp > 0)
			{
			(void) reset_presentation_by_comp_pres(&PresDef,
													&comp_pres[num_comp-1]);
			}

		/* Then reset remaining default presentation parameters */
		if ( !blank(outline_first) )
			{
			if ( same_ic(outline_first, LogicalYes) )
				PresDef.outline_first = TRUE;
			else
				PresDef.outline_first = FALSE;
			}
		if ( !blank(interior_fill) )
					(void) strcpy(PresDef.interior_fill,  interior_fill);
		if ( !blank(sym_fill_name) )
					(void) strcpy(PresDef.sym_fill_name,  sym_fill_name);
		if ( !blank(pattern) )
					(void) strcpy(PresDef.pattern,        pattern);
		if ( !blank(pattern_width) )
					(void) strcpy(PresDef.pattern_width,  pattern_width);
		if ( !blank(pattern_length) )
					(void) strcpy(PresDef.pattern_length, pattern_length);
		if ( !blank(font) )
					(void) strcpy(PresDef.font,           font);
		if ( !blank(font_weight) )
					(void) strcpy(PresDef.font_weight,    font_weight);
		if ( !blank(italics) )
					(void) strcpy(PresDef.italics,        italics);
		if ( !blank(justification) )
					(void) strcpy(PresDef.justified,      justification);
		if ( !blank(text_size) )
					(void) strcpy(PresDef.text_size,      text_size);
		if ( !blank(char_space) )
					(void) strcpy(PresDef.char_space,     char_space);
		if ( !blank(word_space) )
					(void) strcpy(PresDef.word_space,     word_space);
		if ( !blank(line_space) )
					(void) strcpy(PresDef.line_space,     line_space);

		/* Then reset current presentation */
		(void) copy_presentation(&CurPres, &PresDef);
		}

	/* Set (or reset) named presentation */
	else
		{

		/* Add the named presentation to the list           */
		/*  ... or modify the values for this presentation! */
		named_pres = add_presentation(name);

		/* Save parameters already entered to local component presentation */
		if ( num_comp > 0 )
			{
			if ( blank(comp_pres[0].line_width) )
				(void) strcpy(comp_pres[0].line_width, named_pres->line_width);
			if ( blank(comp_pres[0].line_style) )
				(void) strcpy(comp_pres[0].line_style, named_pres->line_style);
			if ( blank(comp_pres[0].outline) )
				(void) strcpy(comp_pres[0].outline,    named_pres->outline);
			if ( blank(comp_pres[0].fill) )
				(void) strcpy(comp_pres[0].fill,       named_pres->fill);
			}

		/* Ensure that local component presentation is filled in */
		(void) check_comp_pres();

		/* First reset presentation parameters in named presentation */
		/*  from local component presentation                        */
		if ( num_comp > 0)
			{
			(void) replace_presentation_comp_pres(named_pres,
														num_comp, comp_pres);
			(void) reset_presentation_by_comp_pres(named_pres, &comp_pres[0]);
			}

		/* Then reset remaining parameters in named presentation */
		if ( !blank(outline_first) )
			{
			if ( same_ic(outline_first, LogicalYes) )
				named_pres->outline_first = TRUE;
			else
				named_pres->outline_first = FALSE;
			}
		if ( !blank(interior_fill) )
					(void) strcpy(named_pres->interior_fill,  interior_fill);
		if ( !blank(sym_fill_name) )
					(void) strcpy(named_pres->sym_fill_name,  sym_fill_name);
		if ( !blank(pattern) )
					(void) strcpy(named_pres->pattern,        pattern);
		if ( !blank(pattern_width) )
					(void) strcpy(named_pres->pattern_width,  pattern_width);
		if ( !blank(pattern_length) )
					(void) strcpy(named_pres->pattern_length, pattern_length);
		if ( !blank(font) )
					(void) strcpy(named_pres->font,           font);
		if ( !blank(font_weight) )
					(void) strcpy(named_pres->font_weight,    font_weight);
		if ( !blank(italics) )
					(void) strcpy(named_pres->italics,        italics);
		if ( !blank(justification) )
					(void) strcpy(named_pres->justified,      justification);
		if ( !blank(text_size) )
					(void) strcpy(named_pres->text_size,      text_size);
		if ( !blank(char_space) )
					(void) strcpy(named_pres->char_space,     char_space);
		if ( !blank(word_space) )
					(void) strcpy(named_pres->word_space,     word_space);
		if ( !blank(line_space) )
					(void) strcpy(named_pres->line_space,     line_space);
		}

	/* Free the local presentation buffers */
	(void) free_comp_pres();

	/* Return TRUE */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    s e t _ p r o j e c t i o n                                       *
*                                                                      *
***********************************************************************/

LOGICAL		set_projection

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	char		type[GPGMedium];
	char		ref1[GPGMedium], ref2[GPGMedium], ref3[GPGMedium];
	char		ref4[GPGMedium], ref5[GPGMedium];
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(type, FpaCblank);
	(void) strcpy(ref1, FpaCblank);
	(void) strcpy(ref2, FpaCblank);
	(void) strcpy(ref3, FpaCblank);
	(void) strcpy(ref4, FpaCblank);
	(void) strcpy(ref5, FpaCblank);

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "type") )
			{
			(void) strcpy(type, action);
			}

		else if ( same(key, "ref1") )
			{
			(void) strcpy(ref1, action);
			}

		else if ( same(key, "ref2") )
			{
			(void) strcpy(ref2, action);
			}

		else if ( same(key, "ref3") )
			{
			(void) strcpy(ref3, action);
			}

		else if ( same(key, "ref4") )
			{
			(void) strcpy(ref4, action);
			}

		else if ( same(key, "ref5") )
			{
			(void) strcpy(ref5, action);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Now define the projection */
	if ( !define_projection_by_name(&Projn, type, ref1, ref2, ref3, ref4, ref5) )
		{
		(void) fprintf(stdout, " Unrecognized map projection type ... %s\n",
				type);
		(void) fprintf(stdout, "   or parameters ... %s %s %s %s %s\n",
				ref1, ref2, ref3, ref4, ref5);
		return FALSE;
		}

	/* Define the map projection in the default field descriptor */
	(void) define_map_projection(&BaseMap, &Projn,
									&BaseMap.definition, &BaseMap.grid);
	(void) set_fld_descript(&Fdesc, FpaF_MAP_PROJECTION, &BaseMap,
									FpaF_END_OF_LIST);

	/* Print out the adjusted projection definition */
	(void) fprintf(stdout, " Adjusted map projection ...");
	(void) fprintf(stdout, "  type: %s",
			which_projection_name(BaseMap.projection.type));
	(void) fprintf(stdout, "  ref[1-5]: %.1f %.1f %.1f %.1f %.1f\n",
			BaseMap.projection.ref[0], BaseMap.projection.ref[1],
			BaseMap.projection.ref[2], BaseMap.projection.ref[3],
			BaseMap.projection.ref[4]);

	/* Return TRUE */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    s e t _ r e s o l u t i o n                                       *
*                                                                      *
***********************************************************************/

LOGICAL		set_resolution

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Return immediately if invalid map definition units */
	if ( BaseMap.definition.units <= 0.0 ) return FALSE;

	/* Initialize grid definition parameters from current basemap */
	Grid.gridlen = BaseMap.grid.gridlen;
	Grid.xgrid   = BaseMap.grid.xgrid;
	Grid.ygrid   = BaseMap.grid.ygrid;
	Grid.units   = BaseMap.grid.units;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "res") )
			{
			(void) sscanf(action, "%f", &Grid.gridlen);
			Grid.xgrid = Grid.gridlen;
			Grid.ygrid = Grid.gridlen;
			}

		else if ( same(key, "map_units") )
			{
			(void) sscanf(action, "%f", &Grid.units);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Check for valid grid definition */
	if ( Grid.units <= 0.0 || Grid.gridlen <= 0.0 ) return FALSE;

	/* Define the grid dimensions */
	(void) set_grid_def_units(&Grid, BaseMap.definition.units);
	Grid.nx = 1 + (int) (fabs(BaseMap.definition.xlen) / Grid.gridlen + 0.75);
	Grid.ny = 1 + (int) (fabs(BaseMap.definition.ylen) / Grid.gridlen + 0.75);

	/* Define the map projection in the default field descriptor */
	(void) define_map_projection(&BaseMap, &BaseMap.projection,
									&BaseMap.definition, &Grid);
	(void) set_fld_descript(&Fdesc, FpaF_MAP_PROJECTION, &BaseMap,
									FpaF_END_OF_LIST);

	/* Print out the adjusted grid definition */
	(void) fprintf(stdout, " Adjusted grid definition ...");
	(void) fprintf(stdout, "  nx: %d  ny: %d  gridlen: %.0f\n",
			BaseMap.grid.nx, BaseMap.grid.ny, BaseMap.grid.gridlen);
	(void) fprintf(stdout, "       xgrid: %.0f  ygrid: %.0f",
			BaseMap.grid.xgrid, BaseMap.grid.ygrid);
	(void) fprintf(stdout, "  map_units: %.0f\n",
			BaseMap.grid.units);

	/* Return TRUE */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    s e t _ s i z e                                                   *
*                                                                      *
***********************************************************************/

LOGICAL		set_size

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	float		width, height, columns, rows;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Initialize all parameters */
	width   = 0.0;
	height  = 0.0;
	columns = 0.0;
	rows    = 0.0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "width") )
			{
			(void) sscanf(action, "%f", &width);
			}

		else if ( same(key, "height") )
			{
			(void) sscanf(action, "%f", &height);
			}

		else if ( same(key, "columns") )
			{
			(void) sscanf(action, "%f", &columns);
			}

		else if ( same(key, "rows") )
			{
			(void) sscanf(action, "%f", &rows);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Now initialize the output graphics size or text buffer */
	switch ( Program.macro )
		{
		case GPG_PSMet:
		case GPG_SVGMet:
		case GPG_CorMet:
			return initialize_graphics_size(width, height);
		case GPG_TexMet:
			return initialize_graphics_size(columns, rows);
		}

	/* Return FALSE if problem initializing */
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*    s e t _ s o u r c e                                               *
*                                                                      *
***********************************************************************/

LOGICAL		set_source

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	char		source[GPGMedium], valid_time[GPGMedium];
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(source,     FpaCblank);
	(void) strcpy(valid_time, FpaCblank);

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "source") )
			{
			(void) strcpy(source, action);
			}

		else if ( same(key, "valid_time") )
			{
			(void) strcpy(valid_time, action);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Error return for missing parameters */
	if ( blank(source) )     (void) error_report("No source name");
	if ( blank(valid_time) ) (void) error_report("No valid time");

	/* Now define the new source and time */
	return define_source(source, valid_time);
	}

/***********************************************************************
*                                                                      *
*    s e t _ s y m b o l _ f i l l                                     *
*                                                                      *
***********************************************************************/

LOGICAL		set_symbol_fill

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int				ii;
	char			sym_fill_name[GPGMedium];
	float			value;
	SYMBOL_FILL		*sym_fill;
	STRING			key, action;
	char			err_buf[GPGLong];

	/* First find the keyword defining the symbol fill name */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "symbol_fill_name") )
			{
			(void) sscanf(action, "%s", sym_fill_name);
			break;
			}
		}

	/* Return FALSE if keyword defining symbol fill name not found */
	if ( ii >= num ) return FALSE;

	/* Error return for using unacceptable symbol fill name */
	if ( same(sym_fill_name, SymbolFillNone) )
		{
		(void) sprintf(err_buf, "Symbol fill name cannot be: %s",
				SymbolFillNone);
		(void) error_report(err_buf);
		}

	/* Add the named symbol fill parameters to the list */
	/*  ... or modify the values for this symbol fill!  */
	sym_fill = add_symbol_fill(sym_fill_name);

	/* Now process all remaining keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "symbol_fill_name") )
			{
			}

		else if ( same(key, "symbol") )
			{
			(void) strcpy(sym_fill->symbol, find_symbol_file(action));
			}

		else if ( same(key, "symbol_scale") )
			{
			(void) sscanf(action, "%f", &value);
			sym_fill->sym_scale = value;
			}

		else if ( same(key, "symbol_rotation") )
			{
			(void) sscanf(action, "%f", &value);
			sym_fill->sym_rotate = value;
			}

		/* >>> "repeat_rotation" not used yet! <<< */
		else if ( same(key, "repeat_rotation") )
			{
			(void) sscanf(action, "%f", &value);
			sym_fill->rep_rotate = value;
			}

		else if ( same(key, "x_off") )
			{
			(void) sscanf(action, "%f", &value);
			sym_fill->x_off = value;
			}

		else if ( same(key, "y_off") )
			{
			(void) sscanf(action, "%f", &value);
			sym_fill->y_off = value;
			}

		else if ( same(key, "x_repeat") )
			{
			(void) sscanf(action, "%f", &value);
			sym_fill->x_repeat = value;
			}

		else if ( same(key, "y_repeat") )
			{
			(void) sscanf(action, "%f", &value);
			sym_fill->y_repeat = value;
			}

		else if ( same(key, "x_shift") )
			{
			(void) sscanf(action, "%f", &value);
			sym_fill->x_shift = value;
			}

		else if ( same(key, "y_shift") )
			{
			(void) sscanf(action, "%f", &value);
			sym_fill->y_shift = value;
			}

		else if ( same(key, "line_width") )
			{
			(void) strcpy(sym_fill->presentation.line_width, action);
			}

		else if ( same(key, "line_style") )
			{
			(void) strcpy(sym_fill->presentation.line_style, action);
			}

		else if ( same(key, "outline") )
			{
			(void) strcpy(sym_fill->presentation.outline, action);
			}

		else if ( same(key, "fill") )
			{
			(void) strcpy(sym_fill->presentation.fill, action);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Return TRUE */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    s e t _ v e r b o s e                                             *
*                                                                      *
***********************************************************************/

LOGICAL		set_verbose

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	char		err_buf[GPGLong];

	/* Only one value is allowed */
	if ( num != 1 ) return FALSE;

	/* Set flag for debugging output */
	if ( same_ic(list[0], LogicalYes) )
		{
		Verbose = TRUE;
		return TRUE;
		}

	else if ( same_ic(list[0], LogicalNo) )
		{
		Verbose = FALSE;
		return TRUE;
		}

	else if ( same_ic(list[0], "true") || same_ic(list[0], "t")
				|| same_ic(list[0], "on") )
		{
		(void) sprintf(err_buf, "Parameter for @verbose should be ... %s",
				LogicalYes);
		(void) warn_report(err_buf);
		Verbose = TRUE;
		return TRUE;
		}

	else if ( same_ic(list[0], "false") || same_ic(list[0], "f")
				|| same_ic(list[0], "off") )
		{
		(void) sprintf(err_buf, "Parameter for @verbose should be ... %s",
				LogicalNo);
		(void) warn_report(err_buf);
		Verbose = FALSE;
		return TRUE;
		}

	/* Return FALSE if problem with debugging flag */
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*    s e t _ w i n d _ b a r b _ p r e s e n t a t i o n               *
*    s e t _ w i n d _ p r e s e n t a t i o n                         *
*                                                                      *
***********************************************************************/

LOGICAL		set_wind_barb_presentation

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "shaft_length") )
			{
			(void) sscanf(action, "%f", &BarbDef.shaft_length);
			}

		else if ( same(key, "barb_length") )
			{
			(void) sscanf(action, "%f", &BarbDef.barb_length);
			}

		else if ( same(key, "barb_width") )
			{
			(void) sscanf(action, "%f", &BarbDef.barb_width);
			}

		else if ( same(key, "barb_space") )
			{
			(void) sscanf(action, "%f", &BarbDef.barb_space);
			}

		else if ( same(key, "barb_angle") )
			{
			(void) sscanf(action, "%f", &BarbDef.barb_angle);
			}

		else if ( same(key, "speed_round") )
			{
			(void) sscanf(action, "%f", &BarbDef.speed_round);
			}

		else if ( same(key, "gust_above") )
			{
			(void) sscanf(action, "%f", &BarbDef.gust_above);
			}

		else if ( same(key, "gust_round") )
			{
			(void) sscanf(action, "%f", &BarbDef.gust_round);
			}

		else if ( same(key, "gust_size") )
			{
			(void) sscanf(action, "%f", &BarbDef.gust_size);
			}

		else if ( same(key, "gust_distance") )
			{
			(void) sscanf(action, "%f", &BarbDef.gust_distance);
			}

		else if ( same(key, "gust_angle") )
			{
			(void) sscanf(action, "%f", &BarbDef.gust_angle);
			}

		else if ( same(key, "gust_justification") )
			{
			(void) strcpy(BarbDef.gust_just, action);
			}

		else if ( same(key, "gust_format") )
			{
			(void) strcpy(BarbDef.gust_format, action);
			}

		else if ( same(key, "calm_max") )
			{
			(void) sscanf(action, "%f", &BarbDef.calm_max);
			}

		else if ( same(key, "calm_symbol") )
			{
			(void) strcpy(BarbDef.calm_symbol, find_symbol_file(action));
			}

		else if ( same(key, "calm_scale") )
			{
			(void) sscanf(action, "%f", &BarbDef.calm_scale);
			}

		else if ( same(key, "huge_min") )
			{
			(void) sscanf(action, "%f", &BarbDef.huge_min);
			}

		else if ( same(key, "huge_symbol") )
			{
			(void) strcpy(BarbDef.huge_symbol, find_symbol_file(action));
			}

		else if ( same(key, "huge_scale") )
			{
			(void) sscanf(action, "%f", &BarbDef.huge_scale);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Return TRUE */
	return TRUE;
	}

LOGICAL		set_wind_presentation

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "wind_look_up") )
			{
			(void) strcpy(WindDef.wind_lookup, action);
			}

		else if ( same(key, "calm_type") )
			{
			(void) strcpy(WindDef.calm_type, action);
			}

		else if ( same(key, "calm_justification") )
			{
			(void) strcpy(WindDef.calm_just, action);
			}

		else if ( same(key, "calm_format") )
			{
			(void) strcpy(WindDef.calm_format, action);
			}

		else if ( same(key, "calm_size") )
			{
			(void) sscanf(action, "%f", &WindDef.calm_size);
			}

		else if ( same(key, "calm_scale") )
			{
			(void) sscanf(action, "%f", &WindDef.calm_scale);
			}

		else if ( same(key, "x_calm") )
			{
			(void) sscanf(action, "%f", &WindDef.x_calm);
			}

		else if ( same(key, "y_calm") )
			{
			(void) sscanf(action, "%f", &WindDef.y_calm);
			}

		else if ( same(key, "direction_type") )
			{
			(void) strcpy(WindDef.direction_type, action);
			}

		else if ( same(key, "direction_justification") )
			{
			(void) strcpy(WindDef.direction_just, action);
			}

		else if ( same(key, "direction_format") )
			{
			(void) strcpy(WindDef.direction_format, action);
			}

		else if ( same(key, "direction_size") )
			{
			(void) sscanf(action, "%f", &WindDef.direction_size);
			}

		else if ( same(key, "direction_scale") )
			{
			(void) sscanf(action, "%f", &WindDef.direction_scale);
			}

		else if ( same(key, "x_dir") )
			{
			(void) sscanf(action, "%f", &WindDef.x_dir);
			}

		else if ( same(key, "y_dir") )
			{
			(void) sscanf(action, "%f", &WindDef.y_dir);
			}

		else if ( same(key, "speed_type") )
			{
			(void) strcpy(WindDef.speed_type, action);
			}

		else if ( same(key, "speed_justification") )
			{
			(void) strcpy(WindDef.speed_just, action);
			}

		else if ( same(key, "speed_format") )
			{
			(void) strcpy(WindDef.speed_format, action);
			}

		else if ( same(key, "speed_size") )
			{
			(void) sscanf(action, "%f", &WindDef.speed_size);
			}

		else if ( same(key, "speed_scale") )
			{
			(void) sscanf(action, "%f", &WindDef.speed_scale);
			}

		else if ( same(key, "x_spd") )
			{
			(void) sscanf(action, "%f", &WindDef.x_spd);
			}

		else if ( same(key, "y_spd") )
			{
			(void) sscanf(action, "%f", &WindDef.y_spd);
			}

		else if ( same(key, "gust_type") )
			{
			(void) strcpy(WindDef.gust_type, action);
			}

		else if ( same(key, "gust_justification") )
			{
			(void) strcpy(WindDef.gust_just, action);
			}

		else if ( same(key, "gust_format") )
			{
			(void) strcpy(WindDef.gust_format, action);
			}

		else if ( same(key, "gust_above") )
			{
			(void) sscanf(action, "%f", &WindDef.gust_above);
			}

		else if ( same(key, "gust_size") )
			{
			(void) sscanf(action, "%f", &WindDef.gust_size);
			}

		else if ( same(key, "gust_scale") )
			{
			(void) sscanf(action, "%f", &WindDef.gust_scale);
			}

		else if ( same(key, "x_gust") )
			{
			(void) sscanf(action, "%f", &WindDef.x_gust);
			}

		else if ( same(key, "y_gust") )
			{
			(void) sscanf(action, "%f", &WindDef.y_gust);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Error return for missing parameters */
	if ( blank(WindDef.wind_lookup) )
		{
		(void) error_report("No \"wind_look_up\" parameter");
		}

	/* Return TRUE */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    s e t _ v e c t o r _ p r e s e n t a t i o n                     *
*                                                                      *
***********************************************************************/

LOGICAL		set_vector_presentation

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "vector_look_up") )
			{
			(void) strcpy(VectorDef.vector_lookup, action);
			}

		else if ( same(key, "calm_type") )
			{
			(void) strcpy(VectorDef.calm_type, action);
			}

		else if ( same(key, "calm_justification") )
			{
			(void) strcpy(VectorDef.calm_just, action);
			}

		else if ( same(key, "calm_format") )
			{
			(void) strcpy(VectorDef.calm_format, action);
			}

		else if ( same(key, "calm_size") )
			{
			(void) sscanf(action, "%f", &VectorDef.calm_size);
			}

		else if ( same(key, "calm_scale") )
			{
			(void) sscanf(action, "%f", &VectorDef.calm_scale);
			}

		else if ( same(key, "x_calm") )
			{
			(void) sscanf(action, "%f", &VectorDef.x_calm);
			}

		else if ( same(key, "y_calm") )
			{
			(void) sscanf(action, "%f", &VectorDef.y_calm);
			}

		else if ( same(key, "direction_type") )
			{
			(void) strcpy(VectorDef.direction_type, action);
			}

		else if ( same(key, "direction_justification") )
			{
			(void) strcpy(VectorDef.direction_just, action);
			}

		else if ( same(key, "direction_format") )
			{
			(void) strcpy(VectorDef.direction_format, action);
			}

		else if ( same(key, "direction_size") )
			{
			(void) sscanf(action, "%f", &VectorDef.direction_size);
			}

		else if ( same(key, "direction_scale") )
			{
			(void) sscanf(action, "%f", &VectorDef.direction_scale);
			}

		else if ( same(key, "x_dir") )
			{
			(void) sscanf(action, "%f", &VectorDef.x_dir);
			}

		else if ( same(key, "y_dir") )
			{
			(void) sscanf(action, "%f", &VectorDef.y_dir);
			}

		else if ( same(key, "speed_type") )
			{
			(void) strcpy(VectorDef.speed_type, action);
			}

		else if ( same(key, "speed_justification") )
			{
			(void) strcpy(VectorDef.speed_just, action);
			}

		else if ( same(key, "speed_format") )
			{
			(void) strcpy(VectorDef.speed_format, action);
			}

		else if ( same(key, "speed_size") )
			{
			(void) sscanf(action, "%f", &VectorDef.speed_size);
			}

		else if ( same(key, "speed_scale") )
			{
			(void) sscanf(action, "%f", &VectorDef.speed_scale);
			}

		else if ( same(key, "x_spd") )
			{
			(void) sscanf(action, "%f", &VectorDef.x_spd);
			}

		else if ( same(key, "y_spd") )
			{
			(void) sscanf(action, "%f", &VectorDef.y_spd);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Error return for missing parameters */
	if ( blank(VectorDef.vector_lookup) )
		{
		(void) error_report("No \"vector_look_up\" parameter");
		}

	/* Return TRUE */
	return TRUE;
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
*    a d d _ s a m p l e _ l c t n                                     *
*    f r e e _ s a m p l e _ l c t n                                   *
*                                                                      *
***********************************************************************/

/* Storage for sample locations */
static	int			NumSampleLctn = 0;
static	GRA_LCTN	*SampleLctn   = NullPtr(GRA_LCTN *);

static	int		add_sample_lctn

	(
	GRA_LCTN	**lctns,
	GPGsample	macro,
	float		xval,
	float		yval,
	STRING		ident
	)

	{
	int			nc;

	/* Add another sample location */
	nc = NumSampleLctn++;
	SampleLctn = GETMEM(SampleLctn, GRA_LCTN, NumSampleLctn);

	/* Set sample location parameters */
	SampleLctn[nc].macro = macro;
	SampleLctn[nc].xval  = xval;
	SampleLctn[nc].yval  = yval;
	if ( !blank(ident) )
		(void) strcpy(SampleLctn[nc].ident, ident);
	else
		(void) strcpy(SampleLctn[nc].ident, FpaCblank);

	/* Return the number of sample locations */
	if ( NotNull(lctns) ) *lctns = SampleLctn;
	return NumSampleLctn;
	}

static	void	free_sample_lctn

	(
	)

	{

	/* Return if no sample locations to free */
	if ( NumSampleLctn <= 0 ) return;

	/* Free space used by sample locations */
	FREEMEM(SampleLctn);

	/* Reset the number of sample locations */
	NumSampleLctn = 0;
	}
