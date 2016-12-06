/***********************************************************************
*                                                                      *
*     g r a _ d o . c                                                  *
*                                                                      *
*     Routines to calculate parameters from pdf file directives        *
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

#include "fpagpgen_routines.h"
#include "fpagpgen_structs.h"

/* We need FPA library definitions */
#include <fpa.h>

/* Local variables to identify conversion format strings */
static  const   char  ConFormat = '%';

/* Internal static functions */
static	int		add_graphics_field(GRA_FLD_INFO **, int);
static	void	free_graphics_field(void);
static	int		add_graphics_image(GRA_IMAGE_INFO **, int);
static	void	free_graphics_image(void);
static	int		add_list_case(SPCASE **, int);
static	void	free_list_case(void);
static	int		add_xsect_location(XSECT_LOCATION **);
static	void	free_xsect_locations(void);

/* Units for wind speeds and gusts */
static	const	STRING	MperS = "m/s";

/***********************************************************************
*                                                                      *
*    d o _ a d d                                                       *
*                                                                      *
***********************************************************************/

LOGICAL		do_add

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	char		symbol[GPGMedium];
	char		lat[GPGMedium], lon[GPGMedium];
	char		map_x[GPGMedium], map_y[GPGMedium];
	char		location_ident[GPGMedium], location_look_up[GPGMedium];
	char		vtime[GPGTiny], table_name[GPGMedium];
	char		grid_name[GPGMedium], list_name[GPGMedium];
	LOGICAL		rotate_lat, rotate_lon;
	float		scale, rotation, xoff, yoff, map_units, clon;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(symbol,           FpaCblank);
	(void) strcpy(lat,              FpaCblank);
	(void) strcpy(lon,              FpaCblank);
	(void) strcpy(map_x,            FpaCblank);
	(void) strcpy(map_y,            FpaCblank);
	(void) strcpy(location_ident,   FpaCblank);
	(void) strcpy(location_look_up, FpaCblank);
	(void) strcpy(vtime,            TVstamp);
	(void) strcpy(table_name,       FpaCblank);
	(void) strcpy(grid_name,        FpaCblank);
	(void) strcpy(list_name,        FpaCblank);
	scale      = 100.0;
	rotation   =   0.0;
	rotate_lat = FALSE;
	rotate_lon = FALSE;
	xoff       =   0.0;
	yoff       =   0.0;
	map_units  = BaseMap.definition.units;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "symbol") )
			{
			(void) strcpy(symbol, find_symbol_file(action));
			}

		else if ( same(key, "scale") )
			{
			(void) sscanf(action, "%f", &scale);
			}

		else if ( same(key, "rotation") )
			{
			(void) sscanf(action, "%f", &rotation);
			}

		else if ( same(key, "rotate_to_latitude") )
			{
			if ( same_ic(action, LogicalYes) ) rotate_lat = TRUE;
			else                               rotate_lat = FALSE;
			}

		else if ( same(key, "rotate_to_longitude") )
			{
			if ( same_ic(action, LogicalYes) ) rotate_lon = TRUE;
			else                               rotate_lon = FALSE;
			}

		else if ( same(key, "x_off") )
			{
			(void) sscanf(action, "%f", &xoff);
			}

		else if ( same(key, "y_off") )
			{
			(void) sscanf(action, "%f", &yoff);
			}

		else if ( same(key, "lat") )
			{
			(void) strcpy(lat, action);
			}

		else if ( same(key, "lon") )
			{
			(void) strcpy(lon, action);
			}

		else if ( same(key, "map_x") )
			{
			(void) strcpy(map_x, action);
			}

		else if ( same(key, "map_y") )
			{
			(void) strcpy(map_y, action);
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
			(void) strcpy(location_ident, action);
			}

		else if ( same(key, "location_look_up") )
			{
			(void) strcpy(location_look_up, action);
			}

		else if ( same(key, "valid_time") )
			{
			(void) grid_center(&BaseMap, NullPointPtr, NullFloat, &clon);
			(void) strcpy(vtime, interpret_timestring(action, T0stamp, clon));
			}

		else if ( same(key, "table_name") )
			{
			(void) strcpy(table_name, action);
			}

		else if ( same(key, "grid_name") )
			{
			(void) strcpy(grid_name, action);
			}

		else if ( same(key, "list_name") )
			{
			(void) strcpy(list_name, action);
			}

		/* Skip valid presentation keywords already processed */
		else if ( same(key,  "line_width")
				|| same(key, "line_style")
				|| same(key, "outline")
				|| same(key, "fill") )
			{
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Error return for missing parameters */
	if ( blank(symbol) ) (void) error_report("No symbol file");

	/* Error if both "rotate_to_latitude" and "rotate_to_longitude" set */
	if ( rotate_lat && rotate_lon )
		(void) error_report("Can set only one of \"rotate_to_latitude\" \"rotate_to_longitude\"");

	/* Now display the symbol */
	return GRA_display_add(symbol, scale, rotation, rotate_lat, rotate_lon,
			xoff, yoff, lat, lon, map_x, map_y, map_units, location_ident,
			location_look_up, vtime, table_name, grid_name, list_name);
	}

/***********************************************************************
*                                                                      *
*    d o _ a r e a s                                                   *
*                                                                      *
***********************************************************************/

LOGICAL		do_areas

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	LOGICAL			displayok;
	int				ii;
	STRING			next;
	char			area_type[GPGMedium], cat_cascade[GPGMedium];
	char			attribute[GPGMedium], look_up[GPGMedium];
	char			interior_fill[GPGMedium], sym_fill_name[GPGMedium];
	char			pattern[GPGMedium];
	char			pattern_width[GPGMedium], pattern_length[GPGMedium];
	LOGICAL			hole_pattern;
	int				num_fields, num_elem, num_levl;
	GRA_FLD_INFO	*area_flds;
	int				num_catatt, num_att, num_cat;
	CATATTRIB		*cat_attrib;
	int				num_list, num_case, num_pres;
	SPCASE			*list_case;
	int				num_comp, num_width, num_style, num_outline, num_fill;
	PRES			named_pres;
	COMP_PRES		*comp_pres;
	STRING			key, action;
	char			tbuf[GPGMedium], err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(area_type,      AreaTypeSubareas);
	(void) strcpy(cat_cascade,    CatCascadeAnd);
	(void) strcpy(attribute,      AttribAutolabel);
	(void) strcpy(look_up,        FpaCblank);
	(void) strcpy(interior_fill,  FpaCblank);
	(void) strcpy(sym_fill_name,  FpaCblank);
	(void) strcpy(pattern,        FpaCblank);
	(void) strcpy(pattern_width,  FpaCblank);
	(void) strcpy(pattern_length, FpaCblank);
	hole_pattern = FALSE;
	num_fields   = 0;
	num_elem     = 0;
	num_levl     = 0;
	num_catatt   = 0;
	num_att      = 0;
	num_cat      = 0;
	num_list     = 0;
	num_case     = 0;
	num_pres     = 0;
	num_comp     = 0;
	num_width    = 0;
	num_style    = 0;
	num_outline  = 0;
	num_fill     = 0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "element") )
			{

			/* >>> multiple values should use "element_list" below <<< */
			/* >>> this is now an ObsoleteVersion feature          <<< */
			(void) strcpy(tbuf, action);
			next = string_arg(tbuf);
			if ( !blank(tbuf) )
				{
				(void) fprintf(stderr, ">>> Obsolete usage of keyword in");
				(void) fprintf(stderr, " directive ... \"@areas\" <<<\n");
				(void) fprintf(stderr, ">>>   Replace \"element\"");
				(void) fprintf(stderr, " by \"element_list\"");
				(void) fprintf(stderr, " for multiple values <<<\n");
				}

			num_fields = add_graphics_field(&area_flds, ++num_elem);
			(void) strcpy(area_flds[num_elem-1].element, next);
			}

		else if ( same(key, "element_list") )
			{
			(void) strcpy(tbuf, action);
			while ( !blank( next = string_arg(tbuf) ) )
				{
				num_fields = add_graphics_field(&area_flds, ++num_elem);
				(void) strcpy(area_flds[num_elem-1].element, next);
				}
			}

		else if ( same(key, "level") )
			{
			num_fields = add_graphics_field(&area_flds, ++num_levl);
			(void) strcpy(area_flds[num_levl-1].level, action);
			}

		else if ( same(key, "level_list") )
			{
			(void) strcpy(tbuf, action);
			while ( !blank( next = string_arg(tbuf) ) )
				{
				num_fields = add_graphics_field(&area_flds, ++num_levl);
				(void) strcpy(area_flds[num_levl-1].level, next);
				}
			}

		else if ( same(key, "area_type") )
			{
			(void) strcpy(area_type, action);
			}

		else if ( same(key, "category_cascade") )
			{
			(void) strcpy(cat_cascade, action);
			}

		else if ( same(key, "category_attribute") )
			{
			num_catatt = add_category_attribs(&cat_attrib, ++num_att);
			(void) strcpy(cat_attrib[num_att-1].category_attribute, action);
			}

		else if ( same(key, "category") )
			{
			num_catatt = add_category_attribs(&cat_attrib, ++num_cat);
			(void) strcpy(cat_attrib[num_cat-1].category, action);
			}

		else if ( same(key, "attribute") )
			{
			(void) strcpy(attribute, action);
			}

		else if ( same(key, "look_up") )
			{
			(void) strcpy(look_up, action);
			}

		else if ( same(key, "case") )
			{
			num_list = add_list_case(&list_case, ++num_case);
			(void) strcpy(list_case[num_case-1].spcase, action);
			}

		else if ( same(key, "case_presentation") )
			{
			num_list = add_list_case(&list_case, ++num_pres);

			/* Match name to list of named presentations */
			named_pres = get_presentation(action);
			(void) copy_presentation(&list_case[num_pres-1].pres, &named_pres);
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

		else if ( same(key, "pattern_for_holes") )
			{
			if ( same_ic(action, LogicalYes) ) hole_pattern = TRUE;
			else                               hole_pattern = FALSE;
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

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Ensure that at least one set of category parameters has been set */
	/*  ... or initialize one with some default values!                 */
	if ( num_catatt < 1 )
		{
		num_catatt = add_category_attribs(&cat_attrib, ++num_att);
		}

	/* Ensure that local component presentation is filled in */
	(void) check_comp_pres();

	/* Error return for missing parameters */
	if ( num_fields <= 0 )
		(void) error_report("No element - level identifiers");

	/* Error return for incorrect "area_type" */
	if ( !same(area_type, AreaTypeSubareas)
			&& !same(area_type, AreaTypeBoundary)
			&& !same(area_type, AreaTypeDivides) )
		{
		(void) sprintf(err_buf, "Recognized area types: %s %s %s",
				AreaTypeSubareas, AreaTypeBoundary, AreaTypeDivides);
		(void) error_report(err_buf);
		}

	/* Now display all the areas */
	displayok = GRA_display_all_areas(area_flds, num_fields, area_type,
				cat_cascade, cat_attrib, num_catatt, attribute, look_up,
				interior_fill, sym_fill_name, pattern, pattern_width,
				pattern_length, hole_pattern, comp_pres, num_comp,
				list_case, num_list);

	/* Free field, category attribute, special case, local */
	/*  presentation buffers                               */
	(void) free_graphics_field();
	(void) free_category_attribs();
	(void) free_list_case();
	(void) free_comp_pres();

	/* Return results of area display */
	return displayok;
	}

/***********************************************************************
*                                                                      *
*    d o _ b a c k g r o u n d                                         *
*                                                                      *
***********************************************************************/

LOGICAL		do_background

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	float		x_extra, y_extra;
	LOGICAL		cur_anchor;
	POINT		pos, ppos;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Storage for current background outline */
	static	LINE	BackLine = NullLine;

	/* Initialize current background outline */
	if ( IsNull(BackLine) ) BackLine = create_line();
	else                    (void) empty_line(BackLine);

	/* Initialize all parameters */
	x_extra = 0.0;
	y_extra = 0.0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "extra_x") )
			{
			(void) sscanf(action, "%f", &x_extra);
			}

		else if ( same(key, "extra_y") )
			{
			(void) sscanf(action, "%f", &y_extra);
			}

		/* Skip valid presentation keywords already processed */
		else if ( same(key,  "line_width")
				|| same(key, "line_style")
				|| same(key, "outline")
				|| same(key, "interior_fill") )
			{
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Force all locations to be anchored to the current map */
	/*  ... regardless of the current anchor position!       */
	cur_anchor  = AnchorToMap;
	AnchorToMap = TRUE;

	/* Set points for background box based on current map, expanded by */
	/*  x_extra and y_extra and adjusted by perspective (if required)  */
	/* Note that if there is no perspective adjustment,                */
	/*  perspective_location() returns the same postiion!              */
	pos[X] = (float) ULpoint[X] - x_extra/2.0;
	pos[Y] = (float) LRpoint[Y] - y_extra/2.0;
	(void) perspective_location(pos, ppos, NullFloat);
	(void) add_point_to_line(BackLine, ppos);
	pos[X] = (float) LRpoint[X] + x_extra/2.0;
	pos[Y] = (float) LRpoint[Y] - y_extra/2.0;
	(void) perspective_location(pos, ppos, NullFloat);
	(void) add_point_to_line(BackLine, ppos);
	pos[X] = (float) LRpoint[X] + x_extra/2.0;
	pos[Y] = (float) ULpoint[Y] + y_extra/2.0;
	(void) perspective_location(pos, ppos, NullFloat);
	(void) add_point_to_line(BackLine, ppos);
	pos[X] = (float) ULpoint[X] - x_extra/2.0;
	pos[Y] = (float) ULpoint[Y] + y_extra/2.0;
	(void) perspective_location(pos, ppos, NullFloat);
	(void) add_point_to_line(BackLine, ppos);
	pos[X] = (float) ULpoint[X] - x_extra/2.0;
	pos[Y] = (float) LRpoint[Y] - y_extra/2.0;
	(void) perspective_location(pos, ppos, NullFloat);
	(void) add_point_to_line(BackLine, ppos);

	/* Now draw the background outline */
	(void) write_graphics_comment("### Begin display of outline around map");
	(void) write_graphics_outlines(1, &BackLine, TRUE, TRUE);
	(void) write_graphics_comment("### End display of outline around map");

	/* Reset the current anchor */
	AnchorToMap = cur_anchor;

	/* Return TRUE */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    d o _ b o x                                                       *
*                                                                      *
***********************************************************************/

LOGICAL		do_box

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	char		lat[GPGMedium], lon[GPGMedium];
	char		map_x[GPGMedium], map_y[GPGMedium];
	char		location_ident[GPGMedium], location_look_up[GPGMedium];
	char		vtime[GPGTiny], table_name[GPGMedium];
	char		grid_name[GPGMedium], list_name[GPGMedium];
	char		sym_fill_name[GPGMedium];
	LOGICAL		rotate_lat, rotate_lon;
	float		width, height, rotation, xoff, yoff, map_units, clon;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(lat,              FpaCblank);
	(void) strcpy(lon,              FpaCblank);
	(void) strcpy(map_x,            FpaCblank);
	(void) strcpy(map_y,            FpaCblank);
	(void) strcpy(location_ident,   FpaCblank);
	(void) strcpy(location_look_up, FpaCblank);
	(void) strcpy(vtime,            TVstamp);
	(void) strcpy(table_name,       FpaCblank);
	(void) strcpy(grid_name,        FpaCblank);
	(void) strcpy(list_name,        FpaCblank);
	(void) strcpy(sym_fill_name,    FpaCblank);
	width      = 0.0;
	height     = 0.0;
	rotation   = 0.0;
	rotate_lat = FALSE;
	rotate_lon = FALSE;
	map_units  = BaseMap.definition.units;
	xoff       = 0.0;
	yoff       = 0.0;

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

		else if ( same(key, "rotation") )
			{
			(void) sscanf(action, "%f", &rotation);
			}

		else if ( same(key, "rotate_to_latitude") )
			{
			if ( same_ic(action, LogicalYes) ) rotate_lat = TRUE;
			else                               rotate_lat = FALSE;
			}

		else if ( same(key, "rotate_to_longitude") )
			{
			if ( same_ic(action, LogicalYes) ) rotate_lon = TRUE;
			else                               rotate_lon = FALSE;
			}

		else if ( same(key, "x_off") )
			{
			(void) sscanf(action, "%f", &xoff);
			}

		else if ( same(key, "y_off") )
			{
			(void) sscanf(action, "%f", &yoff);
			}

		else if ( same(key, "lat") )
			{
			(void) strcpy(lat, action);
			}

		else if ( same(key, "lon") )
			{
			(void) strcpy(lon, action);
			}

		else if ( same(key, "map_x") )
			{
			(void) strcpy(map_x, action);
			}

		else if ( same(key, "map_y") )
			{
			(void) strcpy(map_y, action);
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
			(void) strcpy(location_ident, action);
			}

		else if ( same(key, "location_look_up") )
			{
			(void) strcpy(location_look_up, action);
			}

		else if ( same(key, "valid_time") )
			{
			(void) grid_center(&BaseMap, NullPointPtr, NullFloat, &clon);
			(void) strcpy(vtime, interpret_timestring(action, T0stamp, clon));
			}

		else if ( same(key, "table_name") )
			{
			(void) strcpy(table_name, action);
			}

		else if ( same(key, "grid_name") )
			{
			(void) strcpy(grid_name, action);
			}

		else if ( same(key, "list_name") )
			{
			(void) strcpy(list_name, action);
			}

		else if ( same(key, "symbol_fill_name") )
			{
			(void) strcpy(sym_fill_name, action);
			}

		/* Skip valid presentation keywords already processed */
		else if ( same(key,  "line_width")
				|| same(key, "line_style")
				|| same(key, "outline")
				|| same(key, "interior_fill") )
			{
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Error if both "rotate_to_latitude" and "rotate_to_longitude" set */
	if ( rotate_lat && rotate_lon )
		(void) error_report("Can set only one of \"rotate_to_latitude\" \"rotate_to_longitude\"");

	/* Now display the box */
	return GRA_display_box(width, height, rotation, rotate_lat, rotate_lon,
			xoff, yoff, lat, lon, map_x, map_y, map_units, location_ident,
			location_look_up, vtime, table_name, grid_name, list_name,
			sym_fill_name);
	}

/***********************************************************************
*                                                                      *
*    d o _ c o n t o u r s                                             *
*                                                                      *
***********************************************************************/

LOGICAL		do_contours

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	LOGICAL		displayok;
	int			ii;
	char		element[GPGMedium], level[GPGMedium];
	char		equation[GPGMedium], units[GPGMedium];
	char		values[GPGMedium], min[GPGMedium], max[GPGMedium];
	char		base[GPGMedium], interval[GPGMedium];
	char		interior_fill[GPGMedium], sym_fill_name[GPGMedium];
	char		pattern[GPGMedium];
	char		pattern_width[GPGMedium], pattern_length[GPGMedium];
	LOGICAL		display_areas;
	int			num_comp, num_width, num_style, num_outline, num_fill;
	COMP_PRES	*comp_pres;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(element,        FpaCblank);
	(void) strcpy(level,          FpaCblank);
	(void) strcpy(equation,       FpaCblank);
	(void) strcpy(units,          FpaCmksUnits);
	(void) strcpy(values,         FpaCblank);
	(void) strcpy(min,            FpaCblank);
	(void) strcpy(max,            FpaCblank);
	(void) strcpy(base,           FpaCblank);
	(void) strcpy(interval,       FpaCblank);
	(void) strcpy(interior_fill,  FpaCblank);
	(void) strcpy(sym_fill_name,  FpaCblank);
	(void) strcpy(pattern,        FpaCblank);
	(void) strcpy(pattern_width,  FpaCblank);
	(void) strcpy(pattern_length, FpaCblank);
	display_areas = FALSE;
	num_comp      = 0;
	num_width     = 0;
	num_style     = 0;
	num_outline   = 0;
	num_fill      = 0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "element") )
			{
			(void) strcpy(element, action);
			}

		else if ( same(key, "level") )
			{
			(void) strcpy(level, action);
			}

		else if ( same(key, "equation") )
			{
			(void) strcpy(equation, action);
			if ( blank(element) ) (void) strcpy(element, FpaCanyElement);
			}

		else if ( same(key, "units") )
			{
			(void) strcpy(units, action);
			}

		else if ( same(key, "values") )
			{
			(void) strcpy(values, action);
			}

		else if ( same(key, "min") )
			{
			(void) strcpy(min, action);
			}

		else if ( same(key, "max") )
			{
			(void) strcpy(max, action);
			}

		else if ( same(key, "base") )
			{
			(void) strcpy(base, action);
			}

		else if ( same(key, "interval") )
			{
			(void) strcpy(interval, action);
			}

		else if ( same(key, "display_as_areas") )
			{
			if ( same_ic(action, LogicalYes) ) display_areas = TRUE;
			else                               display_areas = FALSE;
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

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Ensure that local component presentation is filled in */
	(void) check_comp_pres();

	/* Error return for missing parameters */
	if ( blank(element) && blank(equation) )
		(void) error_report("No element or equation");
	if ( blank(level) )
		(void) error_report("No level");
	if ( IsNull(identify_unit(units)) )
		{
		(void) sprintf(err_buf, "Unknown units: %s", units);
		(void) error_report(err_buf);
		}
	if ( display_areas )
		{
		if ( blank(min) || blank(max) )
			(void) error_report("Contour areas need min and max");
		if ( !blank(values) )
			(void) error_report("Contour areas cannot use values");
		}
	else
		{
		if ( blank(values)
				&& ( blank(min) || blank(max) || blank(base) || blank(interval) ) )
			(void) error_report("Contour lines need values or min, max, base, interval");
		}

	/* Now display the contours */
	displayok = GRA_display_all_contours(element, level, equation, units,
				values, min, max, base, interval, display_areas,
				interior_fill, sym_fill_name, pattern, pattern_width,
				pattern_length, comp_pres, num_comp);

	/* Free the local presentation buffers */
	(void) free_comp_pres();

	/* Return results of contour display */
	return displayok;
	}

/***********************************************************************
*                                                                      *
*    d o _ c r o s s _ s e c t i o n _ a r e a s                       *
*    d o _ c r o s s _ s e c t i o n _ a x i s _ l a b e l s           *
*    d o _ c r o s s _ s e c t i o n _ c o n t o u r s                 *
*    d o _ c r o s s _ s e c t i o n _ c u r v e s                     *
*                                                                      *
***********************************************************************/

LOGICAL		do_cross_section_areas

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	LOGICAL			displayok;
	int				ii;
	STRING			next;
	float			fval;
	double			dval;
	LOGICAL			status;
	float			proximity, box_width;
	double			dprox;
	char			cross_section_name[GPGMedium];
	char			element[GPGMedium], level[GPGMedium];
	char			equation[GPGMedium], vertical_units[GPGMedium];
	char			data_file[GPGMedium], data_file_format[GPGMedium];
	char			data_file_units[GPGMedium];
	char			cat_cascade[GPGMedium];
	char			attribute_upper[GPGMedium], attribute_lower[GPGMedium];
	char			location_look_up[GPGMedium], location_units[GPGMedium];
	char			vertical_look_up[GPGMedium];
	char			proximity_units[GPGMedium], display_function[GPGMedium];
	char			interior_fill[GPGMedium], sym_fill_name[GPGMedium];
	char			pattern[GPGMedium];
	char			pattern_width[GPGMedium], pattern_length[GPGMedium];
	float			xboff, yboff;
	int				num_xloc, ltype;
	XSECT_LOCATION	*xsect_locs;
	int				num_catatt, num_att, num_cat;
	CATATTRIB		*cat_attrib;
	int				num_comp, num_width, num_style, num_outline, num_fill;
	COMP_PRES		*comp_pres;
	STRING			key, action;
	char			tbuf[GPGMedium], err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(cross_section_name, FpaCblank);
	(void) strcpy(element,            FpaCblank);
	(void) strcpy(level,              FpaCblank);
	(void) strcpy(equation,           FpaCblank);
	(void) strcpy(vertical_units,     FpaCmksUnits);
	(void) strcpy(data_file,          FpaCblank);
	(void) strcpy(data_file_format,   FpaCblank);
	(void) strcpy(data_file_units,    FpaCblank);
	(void) strcpy(cat_cascade,        CatCascadeAnd);
	(void) strcpy(attribute_upper,    AttribGPGenDefault);
	(void) strcpy(attribute_lower,    AttribGPGenDefault);
	(void) strcpy(location_look_up,   FpaCblank);
	(void) strcpy(vertical_look_up,   FpaCblank);
	(void) strcpy(proximity_units,    ProximityUnitsKm);
	(void) strcpy(display_function,   XSectLineLinear);
	(void) strcpy(interior_fill,      FpaCblank);
	(void) strcpy(sym_fill_name,      FpaCblank);
	(void) strcpy(pattern,            FpaCblank);
	(void) strcpy(pattern_width,      FpaCblank);
	(void) strcpy(pattern_length,     FpaCblank);
	proximity   = 0.0;
	box_width   = 0.0;
	xboff       = 0.0;
	yboff       = 0.0;
	num_xloc    = 0;
	num_catatt  = 0;
	num_att     = 0;
	num_cat     = 0;
	num_comp    = 0;
	num_width   = 0;
	num_style   = 0;
	num_outline = 0;
	num_fill    = 0;
	ltype       = GPG_LocNone;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "cross_section_name") )
			{
			(void) strcpy(cross_section_name, action);
			}

		else if ( same(key, "vertical_element") )
			{
			(void) strcpy(element, action);
			}

		else if ( same(key, "vertical_level") )
			{
			(void) strcpy(level, action);
			}

		else if ( same(key, "vertical_equation") )
			{
			(void) strcpy(equation, action);
			if ( blank(element) ) (void) strcpy(element, FpaCanyElement);
			}

		else if ( same(key, "vertical_units") )
			{
			(void) strcpy(vertical_units, action);
			}

		else if ( same(key, "vertical_data_file") )
			{
			(void) strcpy(data_file, action);
			if ( blank(element) ) (void) strcpy(element, FpaCanyElement);
			if ( blank(level) )   (void) strcpy(level,   FpaCanyLevel);
			}

		else if ( same(key, "vertical_data_file_format") )
			{
			(void) strcpy(data_file_format, action);
			}

		else if ( same(key, "vertical_data_file_units") )
			{
			(void) strcpy(data_file_units, action);
			}

		else if ( same(key, "category_cascade") )
			{
			(void) strcpy(cat_cascade, action);
			}

		else if ( same(key, "category_attribute") )
			{
			num_catatt = add_category_attribs(&cat_attrib, ++num_att);
			(void) strcpy(cat_attrib[num_att-1].category_attribute, action);
			}

		else if ( same(key, "category") )
			{
			num_catatt = add_category_attribs(&cat_attrib, ++num_cat);
			(void) strcpy(cat_attrib[num_cat-1].category, action);
			}

		else if ( same(key, "vertical_attribute_upper") )
			{
			(void) strcpy(attribute_upper, action);
			}

		else if ( same(key, "vertical_attribute_lower") )
			{
			(void) strcpy(attribute_lower, action);
			}

		else if ( same(key, "location_look_up") )
			{
			(void) strcpy(location_look_up, action);
			}

		else if ( same(key, "location_distances") )
			{
			if      (ltype == GPG_LocNone) ltype = GPG_LocDistance;
			else if (ltype != GPG_LocDistance)
				{
				if (ltype == GPG_LocTime)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_distances\" and \"location_times\" in same directive");
				else if (ltype == GPG_LocFraction)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_distances\" and \"location_fractions\" in same directive");
				(void) error_report(err_buf);
				}
			(void) strcpy(tbuf, action);
			while (TRUE)
				{
				fval = float_arg(tbuf, &status);
				if ( !status ) break;

				num_xloc = add_xsect_location(&xsect_locs);
				xsect_locs[num_xloc-1].xdist = fval;
				}
			}

		else if ( same(key, "location_units") )
			{
			(void) strcpy(location_units, action);
			}

		else if ( same(key, "location_times") )
			{
			if      (ltype == GPG_LocNone) ltype = GPG_LocTime;
			else if (ltype != GPG_LocTime)
				{
				if (ltype == GPG_LocDistance)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_times\" and \"location_distances\" in same directive");
				else if (ltype == GPG_LocFraction)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_times\" and \"location_fractions\" in same directive");
				(void) error_report(err_buf);
				}
			(void) strcpy(tbuf, action);
			while ( !blank( next = string_arg(tbuf) ) )
				{
				num_xloc = add_xsect_location(&xsect_locs);
				(void) strcpy(xsect_locs[num_xloc-1].vtime, next);
				}
			}

		else if ( same(key, "location_fractions") )
			{
			if      (ltype == GPG_LocNone) ltype = GPG_LocFraction;
			else if (ltype != GPG_LocFraction)
				{
				if (ltype == GPG_LocDistance)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_fractions\" and \"location_distances\" in same directive");
				else if (ltype == GPG_LocTime)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_fractions\" and \"location_times\" in same directive");
				(void) error_report(err_buf);
				}
			(void) strcpy(tbuf, action);
			while (TRUE)
				{
				fval = float_arg(tbuf, &status);
				if ( !status ) break;

				num_xloc = add_xsect_location(&xsect_locs);
				xsect_locs[num_xloc-1].xdist = fval;
				}
			}

		else if ( same(key, "vertical_look_up") )
			{
			(void) strcpy(vertical_look_up, action);
			}

		else if ( same(key, "proximity") )
			{
			(void) sscanf(action, "%f", &proximity);
			}

		else if ( same(key, "proximity_units") )
			{
			(void) strcpy(proximity_units, action);
			}

		else if ( same(key, "display_function") )
			{
			(void) strcpy(display_function, action);
			}

		else if ( same(key, "box_width") )
			{
			(void) sscanf(action, "%f", &box_width);
			}

		else if ( same(key, "x_box_off") )
			{
			(void) sscanf(action, "%f", &xboff);
			}

		else if ( same(key, "y_box_off") )
			{
			(void) sscanf(action, "%f", &yboff);
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

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Ensure that at least one set of category parameters has been set */
	/*  ... or initialize one with some default values!                 */
	if ( num_catatt < 1 )
		{
		num_catatt = add_category_attribs(&cat_attrib, ++num_att);
		}

	/* Ensure that local component presentation is filled in */
	(void) check_comp_pres();

	/* Error return for missing parameters */
	if ( blank(cross_section_name) )
		(void) error_report("No cross section name");
	if ( blank(element) && blank(equation) && blank(data_file) )
		(void) error_report("No element or equation or data file");
	if ( blank(level) )
		(void) error_report("No level");
	if ( IsNull(identify_unit(vertical_units)) )
		{
		(void) sprintf(err_buf, "Unknown vertical units: %s", vertical_units);
		(void) error_report(err_buf);
		}
	if ( !blank(data_file) && blank(data_file_format) )
		(void) error_report("No data file format");
	if ( !blank(data_file_units) && IsNull(identify_unit(data_file_units)) )
		{
		(void) sprintf(err_buf, "Unknown vertical data file units: %s",
			data_file_units);
		(void) error_report(err_buf);
		}

	/* Convert proximity to km */
	if ( !convert_value(proximity_units, (double) proximity,
			ProximityUnitsKm, &dprox) )
		{
		(void) sprintf(err_buf,
				"Incorrect proximity units: %s", proximity_units);
		(void) error_report(err_buf);
		}
	proximity = (float) dprox;

	/* Convert cross section values to km (if required) */
	for ( ii=0; ii<num_xloc; ii++ )
		{
		if ( !convert_value(location_units, (double) xsect_locs[ii].xdist,
				LocationUnitsKm, &dval) )
			{
			(void) sprintf(err_buf,
					"Incorrect location units: %s", location_units);
			(void) error_report(err_buf);
			}
		xsect_locs[ii].xdist = (float) dval;
		}

	/* Set the parameter for cross section display */
	AnchorToCrossSection = TRUE;

	/* Now display the cross section areas */
	displayok = GRA_display_xsection_areas(cross_section_name, element, level,
				equation, vertical_units, data_file, data_file_format,
				data_file_units, cat_cascade, cat_attrib, num_catatt,
				attribute_upper, attribute_lower, location_look_up, ltype,
				num_xloc, xsect_locs, vertical_look_up, proximity,
				display_function, interior_fill, sym_fill_name,
				box_width, xboff, yboff,
				pattern, pattern_width, pattern_length, comp_pres, num_comp);

	/* Unset the parameter for cross section display */
	AnchorToCrossSection = FALSE;

	/* Free the cross section location buffers */
	(void) free_xsect_locations();

	/* Free category attribute and local presentation buffers */
	(void) free_category_attribs();
	(void) free_comp_pres();

	/* Return results of cross section areas display */
	return displayok;
	}

LOGICAL		do_cross_section_axis_labels

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	LOGICAL			displayok;
	int				ii, nn;
	STRING			next;
	float			fval;
	double			dval;
	LOGICAL			status;
	char			cross_section_name[GPGMedium], axis_for_display[GPGMedium];
	char			location_look_up[GPGMedium], location_units[GPGMedium];
	char			vertical_look_up[GPGMedium];
	char			tzone[GPGMedium], tlang[GPGMedium];
	char			format[GPGMedium], conversion_format[GPGMedium];
	char			look_up[GPGMedium];
	char			display_name[GPGMedium], display_type[GPGMedium];
	int				num_attrib;
	LOGICAL			need_default_attrib;
	ATTRIB_DISPLAY	*temp_attrib, *attribs;
	float			size, symscale, txt_size, wthscale, hgtscale;
	float			rotation, xoff, yoff;
	int				num_xloc, ltype;
	XSECT_LOCATION	*xsect_locs;
	STRING			key, action;
	char			tbuf[GPGMedium], err_buf[GPGLong];

	/* Set current text size */
	(void) sscanf(CurPres.text_size, "%f", &size);

	/* Initialize all parameters */
	(void) strcpy(cross_section_name, FpaCblank);
	(void) strcpy(axis_for_display,   FpaCblank);
	(void) strcpy(location_look_up,   FpaCblank);
	(void) strcpy(vertical_look_up,   FpaCblank);
	(void) strcpy(tzone,              FpaCblank);
	(void) strcpy(tlang,              FpaCblank);
	(void) strcpy(format,             FpaCblank);
	(void) strcpy(conversion_format,  FpaCblank);
	(void) strcpy(look_up,            FpaCblank);
	(void) strcpy(display_name,       FpaCblank);
	(void) strcpy(display_type,       FpaCblank);
	symscale  = 100.0;
	txt_size  =  size;
	wthscale  = Program.default_width_scale;
	hgtscale  = Program.default_height_scale;
	rotation  =   0.0;
	xoff      =   0.0;
	yoff      =   0.0;
	num_xloc  = 0;
	ltype     = GPG_LocNone;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "cross_section_name") )
			{
			(void) strcpy(cross_section_name, action);
			}

		else if ( same(key, "axis_for_display") )
			{
			(void) strcpy(axis_for_display, action);
			}

		else if ( same(key, "location_look_up") )
			{
			(void) strcpy(location_look_up, action);
			}

		else if ( same(key, "location_distances") )
			{
			if      (ltype == GPG_LocNone) ltype = GPG_LocDistance;
			else if (ltype != GPG_LocDistance)
				{
				if (ltype == GPG_LocTime)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_distances\" and \"location_times\" in same directive");
				else if (ltype == GPG_LocFraction)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_distances\" and \"location_fractions\" in same directive");
				(void) error_report(err_buf);
				}
			(void) strcpy(tbuf, action);
			while (TRUE)
				{
				fval = float_arg(tbuf, &status);
				if ( !status ) break;

				num_xloc = add_xsect_location(&xsect_locs);
				xsect_locs[num_xloc-1].xdist = fval;
				}
			}

		else if ( same(key, "location_units") )
			{
			(void) strcpy(location_units, action);
			}

		else if ( same(key, "location_times") )
			{
			if      (ltype == GPG_LocNone) ltype = GPG_LocTime;
			else if (ltype != GPG_LocTime)
				{
				if (ltype == GPG_LocDistance)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_times\" and \"location_distances\" in same directive");
				else if (ltype == GPG_LocFraction)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_times\" and \"location_fractions\" in same directive");
				(void) error_report(err_buf);
				}
			(void) strcpy(tbuf, action);
			while ( !blank( next = string_arg(tbuf) ) )
				{
				num_xloc = add_xsect_location(&xsect_locs);
				(void) strcpy(xsect_locs[num_xloc-1].vtime, next);
				}
			}

		else if ( same(key, "location_fractions") )
			{
			if      (ltype == GPG_LocNone) ltype = GPG_LocFraction;
			else if (ltype != GPG_LocFraction)
				{
				if (ltype == GPG_LocDistance)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_fractions\" and \"location_distances\" in same directive");
				else if (ltype == GPG_LocTime)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_fractions\" and \"location_times\" in same directive");
				(void) error_report(err_buf);
				}
			(void) strcpy(tbuf, action);
			while (TRUE)
				{
				fval = float_arg(tbuf, &status);
				if ( !status ) break;

				num_xloc = add_xsect_location(&xsect_locs);
				xsect_locs[num_xloc-1].xdist = fval;
				}
			}

		else if ( same(key, "vertical_look_up") )
			{
			(void) strcpy(vertical_look_up, action);
			}

		else if ( same(key, "time_zone") )
			{
			(void) strcpy(tzone, action);
			}

		else if ( same(key, "language") )
			{
			(void) strcpy(tlang, action);
			}

		else if ( same(key, "format") )
			{
			(void) strcpy(format, string_arg(action));
			if ( !blank(action) )
				{
				(void) no_white(action);
				(void) strcpy(conversion_format, action);
				}
			}

		else if ( same(key, "look_up") )
			{
			(void) strcpy(look_up, action);
			}

		else if ( same(key, "symbol_scale") )
			{
			(void) sscanf(action, "%f", &symscale);
			}

		else if ( same(key, "text_size") )
			{
			(void) sscanf(action, "%f", &txt_size);
			}

		else if ( same(key, "display_name") )
			{
			(void) strcpy(display_name, action);
			}

		else if ( same(key, "display_type") )
			{
			(void) strcpy(display_type, action);
			}

		else if ( same(key, "width_scale") )
			{
			(void) sscanf(action, "%f", &wthscale);
			}

		else if ( same(key, "height_scale") )
			{
			(void) sscanf(action, "%f", &hgtscale);
			}

		else if ( same(key, "rotation") )
			{
			(void) sscanf(action, "%f", &rotation);
			}

		else if ( same(key, "x_off") )
			{
			(void) sscanf(action, "%f", &xoff);
			}

		else if ( same(key, "y_off") )
			{
			(void) sscanf(action, "%f", &yoff);
			}

		/* Skip valid attribute keywords (for the moment) */
		else if ( same(key,  "attribute")
				|| same(key, "attribute_show")
				|| same(key, "attribute_anchor")
				|| same(key, "attribute_ref")
				|| same(key, "attribute_justification")
				|| same(key, "attribute_vertical_just")
				|| same(key, "attribute_units")
				|| same(key, "attribute_format")
				|| same(key, "attribute_look_up")
				|| same(key, "attribute_symbol_scale")
				|| same(key, "attribute_text_size")
				|| same(key, "attribute_display_name")
				|| same(key, "attribute_display_type")
				|| same(key, "attribute_width_scale")
				|| same(key, "attribute_height_scale")
				|| same(key, "attribute_x_off")
				|| same(key, "attribute_y_off")
				|| same(key, "attribute_line_width")
				|| same(key, "attribute_line_style")
				|| same(key, "attribute_outline")
				|| same(key, "attribute_fill")
				|| same(key, "attribute_outline_first")
				|| same(key, "attribute_font")
				|| same(key, "attribute_font_weight")
				|| same(key, "attribute_italics")
				|| same(key, "attribute_char_space")
				|| same(key, "attribute_word_space") )
			{
			}

		/* Skip valid presentation keywords already processed */
		else if ( same(key,  "line_width")
				|| same(key, "line_style")
				|| same(key, "outline")
				|| same(key, "fill")
				|| same(key, "outline_first")
				|| same(key, "font")
				|| same(key, "font_weight")
				|| same(key, "italics")
				|| same(key, "justification")
				|| same(key, "char_space")
				|| same(key, "word_space") )
			{
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Initialize storage for attribute display list */
	(void) initialize_attribute_display();

	/* Now process all attribute keywords */
	need_default_attrib = TRUE;
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		/* Add another attribute to the list ... with default parameters */
		if ( same(key, "attribute") )
			{
			temp_attrib = add_attribute_display(action, FpaCmksUnits,
							format, conversion_format, look_up, symscale,
							txt_size, wthscale, hgtscale, &CurPres);
			need_default_attrib = FALSE;
			}

		/* Add the default attribute to the list ... if necessary */
		else if ( same_start(key, "attribute_") && need_default_attrib )
				{
				temp_attrib = add_attribute_display(AttribGPGenIdent,
								FpaCmksUnits, format, conversion_format,
								look_up, symscale, txt_size,
								wthscale, hgtscale, &CurPres);
				}

		/* Now add the attribute parameters */
		if ( same(key, "attribute") )
			{
			}

		else if ( same(key, "attribute_show") )
			{
			if ( same_ic(action, "no") ) temp_attrib->show_label = FALSE;
			else                         temp_attrib->show_label = TRUE;
			}

		/* Adjust attribute parameters for this attribute */
		else if ( same(key, "attribute_anchor") )
			{
			(void) strcpy(temp_attrib->anchor, action);
			}

		else if ( same(key, "attribute_ref") )
			{
			(void) strcpy(temp_attrib->ref, action);
			}

		else if ( same(key, "attribute_justification") )
			{
			(void) strcpy(temp_attrib->presentation.justified, action);
			}

		else if ( same(key, "attribute_vertical_just") )
			{
			(void) strcpy(temp_attrib->vertical_just, action);
			}

		else if ( same(key, "attribute_units") )
			{
			(void) strcpy(temp_attrib->units, action);
			}

		else if ( same(key, "attribute_format") )
			{
			(void) strcpy(temp_attrib->format, string_arg(action));
			if ( !blank(action) )
				{
				(void) no_white(action);
				(void) strcpy(temp_attrib->conversion_format, action);
				}
			}

		else if ( same(key, "attribute_look_up") )
			{
			(void) strcpy(temp_attrib->look_up, action);
			}

		else if ( same(key, "attribute_symbol_scale") )
			{
			(void) sscanf(action, "%f", &(temp_attrib->symbol_scale));
			}

		else if ( same(key, "attribute_text_size") )
			{
			(void) sscanf(action, "%f", &(temp_attrib->txt_size));
			}

		else if ( same(key, "attribute_display_name") )
			{
			(void) strcpy(temp_attrib->display_name, action);
			}

		else if ( same(key, "attribute_display_type") )
			{
			(void) strcpy(temp_attrib->display_type, action);
			}

		else if ( same(key, "attribute_width_scale") )
			{
			(void) sscanf(action, "%f", &(temp_attrib->width_scale));
			}

		else if ( same(key, "attribute_height_scale") )
			{
			(void) sscanf(action, "%f", &(temp_attrib->height_scale));
			}

		else if ( same(key, "attribute_x_off") )
			{
			(void) sscanf(action, "%f", &(temp_attrib->x_off));
			}

		else if ( same(key, "attribute_y_off") )
			{
			(void) sscanf(action, "%f", &(temp_attrib->y_off));
			}

		else if ( same(key, "attribute_line_width") )
			{
			(void) strcpy(temp_attrib->presentation.line_width, action);
			}

		else if ( same(key, "attribute_line_style") )
			{
			(void) strcpy(temp_attrib->presentation.line_style, action);
			}

		else if ( same(key, "attribute_outline") )
			{
			(void) strcpy(temp_attrib->presentation.outline, action);
			}

		else if ( same(key, "attribute_fill") )
			{
			(void) strcpy(temp_attrib->presentation.fill, action);
			}

		else if ( same(key, "attribute_outline_first") )
			{
			if ( same_ic(action, LogicalYes) )
				{
				temp_attrib->presentation.outline_first = TRUE;
				}
			else
				{
				temp_attrib->presentation.outline_first = FALSE;
				}
			}

		else if ( same(key, "attribute_font") )
			{
			(void) strcpy(temp_attrib->presentation.font, action);
			}

		else if ( same(key, "attribute_font_weight") )
			{
			(void) strcpy(temp_attrib->presentation.font_weight, action);
			}

		else if ( same(key, "attribute_italics") )
			{
			(void) strcpy(temp_attrib->presentation.italics, action);
			}

		else if ( same(key, "attribute_char_space") )
			{
			(void) strcpy(temp_attrib->presentation.char_space, action);
			}

		else if ( same(key, "attribute_word_space") )
			{
			(void) strcpy(temp_attrib->presentation.word_space, action);
			}
		}

	/* Ensure that at least one attribute has been set ... or set a default! */
	num_attrib = return_attribute_display(&attribs);
	if ( num_attrib < 1 )
		{
		(void) add_attribute_display(AttribGPGenIdent, FpaCmksUnits,
				format, conversion_format, look_up, symscale,
				txt_size, wthscale, hgtscale, &CurPres);
		}

	/* Error return for missing parameters */
	if ( blank(cross_section_name) )
		(void) error_report("No cross section name");
	if ( blank(axis_for_display) )
		(void) error_report("No cross section axis for display");

	/* Check "axis_for_display" keyword for acceptable values */
	if ( !same(axis_for_display, XSectAxisLower)
			&& !same(axis_for_display, XSectAxisUpper)
			&& !same(axis_for_display, XSectAxisLeft)
			&& !same(axis_for_display, XSectAxisRight) )
		{
		(void) sprintf(err_buf, "Recognized axis for display: %s %s %s %s",
				XSectAxisLower, XSectAxisUpper, XSectAxisLeft, XSectAxisRight);
		(void) error_report(err_buf);
		}

	/* Check "format" "conversion_format" "look_up" keywords for attributes */
	num_attrib = return_attribute_display(&attribs);
	for ( nn=0; nn<num_attrib; nn++ )
		{
		if ( blank(attribs[nn].format) )
			{
			(void) sprintf(err_buf, "No format for attribute: %s",
					attribs[nn].name);
			(void) error_report(err_buf);
			}

		/* Error return for incorrect "format" ... based on application */
		switch ( Program.macro )
			{
			case GPG_PSMet:
			case GPG_SVGMet:
			case GPG_CorMet:
				if ( !same(attribs[nn].format, FormatDirect)
						&& !same(attribs[nn].format, FormatSymbol)
						&& !same(attribs[nn].format, FormatText)
						&& !same(attribs[nn].format, FormatVectorSymbol)
						&& !same(attribs[nn].format, FormatVectorText)
						&& !same(attribs[nn].format, FormatNone) )
					{
					(void) sprintf(err_buf,
							"Recognized format types are: %s %s %s %s %s %s",
							FormatDirect, FormatSymbol, FormatText,
							FormatVectorSymbol, FormatVectorText, FormatNone);
					(void) error_report(err_buf);
					}
				break;
			}

		if ( !blank(attribs[nn].conversion_format) )
			{
			if ( IsNull(strchr(attribs[nn].conversion_format, ConFormat)) )
				{
				(void) sprintf(err_buf,
						"Error in conversion format: %s  for attribute: %s",
						attribs[nn].conversion_format, attribs[nn].name);
				(void) error_report(err_buf);
				}
			}

		if ( same(attribs[nn].format, FormatSymbol)
				|| same(attribs[nn].format, FormatText) )
			{
			if ( blank(attribs[nn].look_up) )
				{
				(void) sprintf(err_buf, "No look_up for attribute: %s",
						attribs[nn].name);
				(void) error_report(err_buf);
				}
			}
		}

	/* Convert cross section values to km (if required) */
	for ( ii=0; ii<num_xloc; ii++ )
		{
		if ( !convert_value(location_units, (double) xsect_locs[ii].xdist,
				LocationUnitsKm, &dval) )
			{
			(void) sprintf(err_buf,
					"Incorrect location units: %s", location_units);
			(void) error_report(err_buf);
			}
		xsect_locs[ii].xdist = (float) dval;
		}

	/* Set the parameter for cross section display */
	AnchorToCrossSection = TRUE;

	/* Now display the cross section axis labels */
	displayok = GRA_display_xsection_axis_labels(cross_section_name,
				axis_for_display, location_look_up, ltype,
				num_xloc, xsect_locs, vertical_look_up, attribs, num_attrib,
				tzone, tlang, display_name, display_type, rotation, xoff, yoff);

	/* Unset the parameter for cross section display */
	AnchorToCrossSection = FALSE;

	/* Free the cross section location buffers */
	(void) free_xsect_locations();

	/* Return results of cross section label display */
	return displayok;
	}

LOGICAL		do_cross_section_contours

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	LOGICAL			displayok;
	int				ii;
	STRING			next;
	float			fval;
	double			dval;
	LOGICAL			status;
	char			cross_section_name[GPGMedium], units[GPGMedium];
	char			location_look_up[GPGMedium], location_units[GPGMedium];
	char			vertical_look_up[GPGMedium];
	char			values[GPGMedium], min[GPGMedium], max[GPGMedium];
	char			base[GPGMedium], interval[GPGMedium];
	char			interior_fill[GPGMedium], sym_fill_name[GPGMedium];
	char			pattern[GPGMedium];
	char			pattern_width[GPGMedium], pattern_length[GPGMedium];
	LOGICAL			display_areas;
	int				num_fields, num_elem, num_levl, num_eqtn;
	GRA_FLD_INFO	*xsect_flds;
	int				num_xloc, ltype;
	XSECT_LOCATION	*xsect_locs;
	int				num_comp, num_width, num_style, num_outline, num_fill;
	COMP_PRES		*comp_pres;
	STRING			key, action;
	char			tbuf[GPGMedium], err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(cross_section_name, FpaCblank);
	(void) strcpy(units,              FpaCmksUnits);
	(void) strcpy(location_look_up,   FpaCblank);
	(void) strcpy(location_units,     LocationUnitsKm);
	(void) strcpy(vertical_look_up,   FpaCblank);
	(void) strcpy(values,             FpaCblank);
	(void) strcpy(min,                FpaCblank);
	(void) strcpy(max,                FpaCblank);
	(void) strcpy(base,               FpaCblank);
	(void) strcpy(interval,           FpaCblank);
	(void) strcpy(interior_fill,      FpaCblank);
	(void) strcpy(sym_fill_name,      FpaCblank);
	(void) strcpy(pattern,            FpaCblank);
	(void) strcpy(pattern_width,      FpaCblank);
	(void) strcpy(pattern_length,     FpaCblank);
	display_areas = FALSE;
	num_fields    = 0;
	num_elem      = 0;
	num_levl      = 0;
	num_eqtn      = 0;
	num_xloc      = 0;
	num_comp      = 0;
	num_width     = 0;
	num_style     = 0;
	num_outline   = 0;
	num_fill      = 0;
	ltype         = GPG_LocNone;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "cross_section_name") )
			{
			(void) strcpy(cross_section_name, action);
			}

		else if ( same(key, "element") )
			{
			num_fields = add_graphics_field(&xsect_flds, ++num_elem);
			(void) strcpy(xsect_flds[num_elem-1].element, action);
			}

		else if ( same(key, "element_list") )
			{
			(void) strcpy(tbuf, action);
			while ( !blank( next = string_arg(tbuf) ) )
				{
				num_fields = add_graphics_field(&xsect_flds, ++num_elem);
				(void) strcpy(xsect_flds[num_elem-1].element, next);
				}
			}

		else if ( same(key, "level") )
			{
			num_fields = add_graphics_field(&xsect_flds, ++num_levl);
			(void) strcpy(xsect_flds[num_levl-1].level, action);
			}

		else if ( same(key, "level_list") )
			{
			(void) strcpy(tbuf, action);
			while ( !blank( next = string_arg(tbuf) ) )
				{
				num_fields = add_graphics_field(&xsect_flds, ++num_levl);
				(void) strcpy(xsect_flds[num_levl-1].level, next);
				}
			}

		else if ( same(key, "equation") )
			{
			num_fields = add_graphics_field(&xsect_flds, ++num_eqtn);
			(void) strcpy(xsect_flds[num_eqtn-1].equation, action);
			if ( blank(xsect_flds[num_elem-1].element) )
				(void) strcpy(xsect_flds[num_elem-1].element, FpaCanyElement);
			}

		else if ( same(key, "units") )
			{
			(void) strcpy(units, action);
			}

		else if ( same(key, "location_look_up") )
			{
			(void) strcpy(location_look_up, action);
			}

		else if ( same(key, "location_distances") )
			{
			if      (ltype == GPG_LocNone) ltype = GPG_LocDistance;
			else if (ltype != GPG_LocDistance)
				{
				if (ltype == GPG_LocTime)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_distances\" and \"location_times\" in same directive");
				else if (ltype == GPG_LocFraction)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_distances\" and \"location_fractions\" in same directive");
				(void) error_report(err_buf);
				}
			(void) strcpy(tbuf, action);
			while (TRUE)
				{
				fval = float_arg(tbuf, &status);
				if ( !status ) break;

				num_xloc = add_xsect_location(&xsect_locs);
				xsect_locs[num_xloc-1].xdist = fval;
				}
			}

		else if ( same(key, "location_units") )
			{
			(void) strcpy(location_units, action);
			}

		else if ( same(key, "location_times") )
			{
			if      (ltype == GPG_LocNone) ltype = GPG_LocTime;
			else if (ltype != GPG_LocTime)
				{
				if (ltype == GPG_LocDistance)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_times\" and \"location_distances\" in same directive");
				else if (ltype == GPG_LocFraction)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_times\" and \"location_fractions\" in same directive");
				(void) error_report(err_buf);
				}
			(void) strcpy(tbuf, action);
			while ( !blank( next = string_arg(tbuf) ) )
				{
				num_xloc = add_xsect_location(&xsect_locs);
				(void) strcpy(xsect_locs[num_xloc-1].vtime, next);
				}
			}

		else if ( same(key, "location_fractions") )
			{
			if      (ltype == GPG_LocNone) ltype = GPG_LocFraction;
			else if (ltype != GPG_LocFraction)
				{
				if (ltype == GPG_LocDistance)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_fractions\" and \"location_distances\" in same directive");
				else if (ltype == GPG_LocTime)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_fractions\" and \"location_times\" in same directive");
				(void) error_report(err_buf);
				}
			(void) strcpy(tbuf, action);
			while (TRUE)
				{
				fval = float_arg(tbuf, &status);
				if ( !status ) break;

				num_xloc = add_xsect_location(&xsect_locs);
				xsect_locs[num_xloc-1].xdist = fval;
				}
			}

		else if ( same(key, "vertical_look_up") )
			{
			(void) strcpy(vertical_look_up, action);
			}

		else if ( same(key, "values") )
			{
			(void) strcpy(values, action);
			}

		else if ( same(key, "min") )
			{
			(void) strcpy(min, action);
			}

		else if ( same(key, "max") )
			{
			(void) strcpy(max, action);
			}

		else if ( same(key, "base") )
			{
			(void) strcpy(base, action);
			}

		else if ( same(key, "interval") )
			{
			(void) strcpy(interval, action);
			}

		else if ( same(key, "display_as_areas") )
			{
			if ( same_ic(action, LogicalYes) ) display_areas = TRUE;
			else                               display_areas = FALSE;
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

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Ensure that local component presentation is filled in */
	(void) check_comp_pres();

	/* Error return for missing parameters */
	if ( blank(cross_section_name) )
		(void) error_report("No cross section name");
	if ( num_fields <= 0 )
		(void) error_report("No element - level - equation identifiers");
	if ( IsNull(identify_unit(units)) )
		{
		(void) sprintf(err_buf, "Unknown units: %s", units);
		(void) error_report(err_buf);
		}
	if ( display_areas )
		{
		if ( blank(min) || blank(max) )
			(void) error_report("Contour areas need min and max");
		if ( !blank(values) )
			(void) error_report("Contour areas cannot use values");
		}
	else
		{
		if ( blank(values)
				&& ( blank(min) || blank(max) || blank(base) || blank(interval) ) )
			(void) error_report("Contour lines need values or min, max, base, interval");
		}

	/* Convert cross section values to km (if required) */
	for ( ii=0; ii<num_xloc; ii++ )
		{
		if ( !convert_value(location_units, (double) xsect_locs[ii].xdist,
				LocationUnitsKm, &dval) )
			{
			(void) sprintf(err_buf,
					"Incorrect location units: %s", location_units);
			(void) error_report(err_buf);
			}
		xsect_locs[ii].xdist = (float) dval;
		}

	/* Now display the cross section contours */
	displayok = GRA_display_xsection_contours(cross_section_name,
				xsect_flds, num_fields, units, location_look_up, ltype,
				num_xloc, xsect_locs, vertical_look_up, values, min, max,
				base, interval, display_areas, interior_fill, sym_fill_name,
				pattern, pattern_width, pattern_length, comp_pres, num_comp);

	/* Free the field buffers */
	(void) free_graphics_field();

	/* Free the cross section location buffers */
	(void) free_xsect_locations();

	/* Free the local presentation buffers */
	(void) free_comp_pres();

	/* Return results of cross section contour display */
	return displayok;
	}

LOGICAL		do_cross_section_curves

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	LOGICAL			displayok;
	int				ii;
	STRING			next;
	float			fval;
	double			dval;
	LOGICAL			status;
	float			proximity;
	double			dprox;
	char			cross_section_name[GPGMedium];
	char			element[GPGMedium], level[GPGMedium];
	char			equation[GPGMedium], vertical_units[GPGMedium];
	char			data_file[GPGMedium], data_file_format[GPGMedium];
	char			data_file_units[GPGMedium];
	char			cat_cascade[GPGMedium], attribute[GPGMedium];
	char			location_look_up[GPGMedium], location_units[GPGMedium];
	char			vertical_look_up[GPGMedium];
	char			proximity_units[GPGMedium], display_function[GPGMedium];
	char			pattern[GPGMedium];
	char			pattern_width[GPGMedium], pattern_length[GPGMedium];
	int				num_xloc, ltype;
	XSECT_LOCATION	*xsect_locs;
	int				num_catatt, num_att, num_cat;
	CATATTRIB		*cat_attrib;
	int				num_comp, num_width, num_style, num_outline, num_fill;
	COMP_PRES		*comp_pres;
	STRING			key, action;
	char			tbuf[GPGMedium], err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(cross_section_name, FpaCblank);
	(void) strcpy(element,            FpaCblank);
	(void) strcpy(level,              FpaCblank);
	(void) strcpy(equation,           FpaCblank);
	(void) strcpy(vertical_units,     FpaCmksUnits);
	(void) strcpy(data_file,          FpaCblank);
	(void) strcpy(data_file_format,   FpaCblank);
	(void) strcpy(data_file_units,    FpaCblank);
	(void) strcpy(cat_cascade,        CatCascadeAnd);
	(void) strcpy(attribute,          AttribGPGenDefault);
	(void) strcpy(location_look_up,   FpaCblank);
	(void) strcpy(location_units,     LocationUnitsKm);
	(void) strcpy(vertical_look_up,   FpaCblank);
	(void) strcpy(proximity_units,    ProximityUnitsKm);
	(void) strcpy(display_function,   XSectLineLinear);
	(void) strcpy(pattern,            FpaCblank);
	(void) strcpy(pattern_width,      FpaCblank);
	(void) strcpy(pattern_length,     FpaCblank);
	proximity   = 0.0;
	num_xloc    = 0;
	num_catatt  = 0;
	num_att     = 0;
	num_cat     = 0;
	num_comp    = 0;
	num_width   = 0;
	num_style   = 0;
	num_outline = 0;
	num_fill    = 0;
	ltype       = GPG_LocNone;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "cross_section_name") )
			{
			(void) strcpy(cross_section_name, action);
			}

		else if ( same(key, "vertical_element") )
			{
			(void) strcpy(element, action);
			}

		else if ( same(key, "vertical_level") )
			{
			(void) strcpy(level, action);
			}

		else if ( same(key, "vertical_equation") )
			{
			(void) strcpy(equation, action);
			if ( blank(element) ) (void) strcpy(element, FpaCanyElement);
			}

		else if ( same(key, "vertical_units") )
			{
			(void) strcpy(vertical_units, action);
			}

		else if ( same(key, "vertical_data_file") )
			{
			(void) strcpy(data_file, action);
			if ( blank(element) ) (void) strcpy(element, FpaCanyElement);
			if ( blank(level) )   (void) strcpy(level,   FpaCanyLevel);
			}

		else if ( same(key, "vertical_data_file_format") )
			{
			(void) strcpy(data_file_format, action);
			}

		else if ( same(key, "vertical_data_file_units") )
			{
			(void) strcpy(data_file_units, action);
			}

		else if ( same(key, "category_cascade") )
			{
			(void) strcpy(cat_cascade, action);
			}

		else if ( same(key, "category_attribute") )
			{
			num_catatt = add_category_attribs(&cat_attrib, ++num_att);
			(void) strcpy(cat_attrib[num_att-1].category_attribute, action);
			}

		else if ( same(key, "category") )
			{
			num_catatt = add_category_attribs(&cat_attrib, ++num_cat);
			(void) strcpy(cat_attrib[num_cat-1].category, action);
			}

		else if ( same(key, "vertical_attribute") )
			{
			(void) strcpy(attribute, action);
			}

		else if ( same(key, "location_look_up") )
			{
			(void) strcpy(location_look_up, action);
			}

		else if ( same(key, "location_distances") )
			{
			if      (ltype == GPG_LocNone) ltype = GPG_LocDistance;
			else if (ltype != GPG_LocDistance)
				{
				if (ltype == GPG_LocTime)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_distances\" and \"location_times\" in same directive");
				else if (ltype == GPG_LocFraction)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_distances\" and \"location_fractions\" in same directive");
				(void) error_report(err_buf);
				}
			(void) strcpy(tbuf, action);
			while (TRUE)
				{
				fval = float_arg(tbuf, &status);
				if ( !status ) break;

				num_xloc = add_xsect_location(&xsect_locs);
				xsect_locs[num_xloc-1].xdist = fval;
				}
			}

		else if ( same(key, "location_units") )
			{
			(void) strcpy(location_units, action);
			}

		else if ( same(key, "location_times") )
			{
			if      (ltype == GPG_LocNone) ltype = GPG_LocTime;
			else if (ltype != GPG_LocTime)
				{
				if (ltype == GPG_LocDistance)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_times\" and \"location_distances\" in same directive");
				else if (ltype == GPG_LocFraction)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_times\" and \"location_fractions\" in same directive");
				(void) error_report(err_buf);
				}
			(void) strcpy(tbuf, action);
			while ( !blank( next = string_arg(tbuf) ) )
				{
				num_xloc = add_xsect_location(&xsect_locs);
				(void) strcpy(xsect_locs[num_xloc-1].vtime, next);
				}
			}

		else if ( same(key, "location_fractions") )
			{
			if      (ltype == GPG_LocNone) ltype = GPG_LocFraction;
			else if (ltype != GPG_LocFraction)
				{
				if (ltype == GPG_LocDistance)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_fractions\" and \"location_distances\" in same directive");
				else if (ltype == GPG_LocTime)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_fractions\" and \"location_times\" in same directive");
				(void) error_report(err_buf);
				}
			(void) strcpy(tbuf, action);
			while (TRUE)
				{
				fval = float_arg(tbuf, &status);
				if ( !status ) break;

				num_xloc = add_xsect_location(&xsect_locs);
				xsect_locs[num_xloc-1].xdist = fval;
				}
			}

		else if ( same(key, "vertical_look_up") )
			{
			(void) strcpy(vertical_look_up, action);
			}

		else if ( same(key, "proximity") )
			{
			(void) sscanf(action, "%f", &proximity);
			}

		else if ( same(key, "proximity_units") )
			{
			(void) strcpy(proximity_units, action);
			}

		else if ( same(key, "display_function") )
			{
			(void) strcpy(display_function, action);
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
	if ( blank(element) && blank(equation) && blank(data_file) )
		(void) error_report("No element or equation or data file");
	if ( blank(level) )
		(void) error_report("No level");
	if ( IsNull(identify_unit(vertical_units)) )
		{
		(void) sprintf(err_buf, "Unknown vertical units: %s", vertical_units);
		(void) error_report(err_buf);
		}
	if ( !blank(data_file) && blank(data_file_format) )
		(void) error_report("No data file format");
	if ( !blank(data_file_units) && IsNull(identify_unit(data_file_units)) )
		{
		(void) sprintf(err_buf, "Unknown vertical data file units: %s",
			data_file_units);
		(void) error_report(err_buf);
		}

	/* Ensure that at least one set of category parameters has been set */
	/*  ... or initialize one with some default values!                 */
	if ( num_catatt < 1 )
		{
		num_catatt = add_category_attribs(&cat_attrib, ++num_att);
		}

	/* Ensure that local component presentation is filled in */
	(void) check_comp_pres();

	/* Convert proximity to km */
	if ( !convert_value(proximity_units, (double) proximity,
			ProximityUnitsKm, &dprox) )
		{
		(void) sprintf(err_buf,
				"Incorrect proximity units: %s", proximity_units);
		(void) error_report(err_buf);
		}
	proximity = (float) dprox;

	/* Convert cross section values to km (if required) */
	for ( ii=0; ii<num_xloc; ii++ )
		{
		if ( !convert_value(location_units, (double) xsect_locs[ii].xdist,
				LocationUnitsKm, &dval) )
			{
			(void) sprintf(err_buf,
					"Incorrect location units: %s", location_units);
			(void) error_report(err_buf);
			}
		xsect_locs[ii].xdist = (float) dval;
		}

	/* Set the parameter for cross section display */
	AnchorToCrossSection = TRUE;

	/* Now display the cross section curves */
	displayok = GRA_display_xsection_curves(cross_section_name, element, level,
				equation, vertical_units, data_file, data_file_format,
				data_file_units, cat_cascade, cat_attrib, num_catatt, attribute,
				location_look_up, ltype, num_xloc, xsect_locs,
				vertical_look_up, proximity, display_function,
				pattern, pattern_width, pattern_length, comp_pres, num_comp);

	/* Unset the parameter for cross section display */
	AnchorToCrossSection = FALSE;

	/* Free the cross section location buffers */
	(void) free_xsect_locations();

	/* Free category attribute and local presentation buffers */
	(void) free_category_attribs();
	(void) free_comp_pres();

	/* Return results of cross section label display */
	return displayok;
	}

/***********************************************************************
*                                                                      *
*    d o _ d r a w _ l i n e                                           *
*                                                                      *
***********************************************************************/

LOGICAL		do_draw_line

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	LOGICAL		lineok;
	int			ii;
	char		line_name[GPGMedium], arrow_name[GPGMedium];
	float		xoff, yoff;
	char		pattern[GPGMedium];
	char		pattern_width[GPGMedium], pattern_length[GPGMedium];
	int			num_comp, num_width, num_style, num_outline, num_fill;
	COMP_PRES	*comp_pres;
	STRING		key, action;
	char		err_buf[GPGLong];

	LOGICAL		NewCallTo_draw_table_line = FALSE;

	/* Initialize all parameters */
	(void) strcpy(line_name,      FpaCblank);
	(void) strcpy(arrow_name,     FpaCblank);
	(void) strcpy(pattern,        FpaCblank);
	(void) strcpy(pattern_width,  FpaCblank);
	(void) strcpy(pattern_length, FpaCblank);
	xoff        = 0.0;
	yoff        = 0.0;
	num_comp    = 0;
	num_width   = 0;
	num_style   = 0;
	num_outline = 0;
	num_fill    = 0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "line_name") )
			{
			(void) strcpy(line_name, action);
			}

		else if ( ObsoleteVersion && same(key, "table_name") )
			{
			(void) fprintf(stderr, ">>> Obsolete keyword \"table_name\" in");
			(void) fprintf(stderr, " directive ... \"@draw_line\" <<<\n");
			(void) fprintf(stderr, ">>> Replace with new directive ...");
			(void) fprintf(stderr, " \"@draw_table_line\"           <<<\n");
			NewCallTo_draw_table_line = TRUE;
			}

		else if ( ObsoleteVersion && same(key, "last_site") )
			{
			(void) fprintf(stderr, ">>> Obsolete keyword \"last_site\" in");
			(void) fprintf(stderr, " directive ... \"@draw_line\" <<<\n");
			(void) fprintf(stderr, ">>> Replace with new directive ...");
			(void) fprintf(stderr, " \"@draw_table_line\"          <<<\n");
			NewCallTo_draw_table_line = TRUE;
			}

		else if ( same(key, "arrow_name") )
			{
			(void) strcpy(arrow_name, action);
			}

		else if ( same(key, "x_off") )
			{
			(void) sscanf(action, "%f", &xoff);
			}

		else if ( same(key, "y_off") )
			{
			(void) sscanf(action, "%f", &yoff);
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

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Ensure that local component presentation is filled in */
	(void) check_comp_pres();

	/* Error return for missing parameters */
	if ( blank(line_name) ) (void) error_report("No line name");

	/* >>> Obsolete keyword ... using new @draw_table_line() directive <<< */
	if ( NewCallTo_draw_table_line )
		{
		(void) free_comp_pres();
		return do_draw_table_line(list, num);
		}
	/* >>> Obsolete keyword ... using new @draw_table_line() directive <<< */

	/* Now draw the line */
	lineok = GRA_draw_line(line_name, arrow_name, xoff, yoff,
				pattern, pattern_width, pattern_length, comp_pres, num_comp);

	/* Free the local presentation buffers */
	(void) free_comp_pres();

	/* Return results of line drawing */
	return lineok;
	}

/***********************************************************************
*                                                                      *
*    d o _ d r a w _ t a b l e _ l i n e                               *
*                                                                      *
***********************************************************************/

LOGICAL		do_draw_table_line

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	LOGICAL		lineok;
	int			ii;
	char		table_name[GPGMedium], last_site[GPGMedium];
	char		line_name[GPGMedium], arrow_name[GPGMedium];
	float		xoff, yoff;
	char		pattern[GPGMedium];
	char		pattern_width[GPGMedium], pattern_length[GPGMedium];
	int			num_comp, num_width, num_style, num_outline, num_fill;
	COMP_PRES	*comp_pres;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(table_name,     FpaCblank);
	(void) strcpy(last_site,      LogicalYes);
	(void) strcpy(line_name,      FpaCblank);
	(void) strcpy(arrow_name,     FpaCblank);
	(void) strcpy(pattern,        FpaCblank);
	(void) strcpy(pattern_width,  FpaCblank);
	(void) strcpy(pattern_length, FpaCblank);
	xoff        = 0.0;
	yoff        = 0.0;
	num_comp    = 0;
	num_width   = 0;
	num_style   = 0;
	num_outline = 0;
	num_fill    = 0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "table_name") )
			{
			(void) strcpy(table_name, action);
			}

		else if ( same(key, "last_site") )
			{
			(void) strcpy(last_site, action);
			}

		else if ( same(key, "line_name") )
			{
			(void) strcpy(line_name, action);
			}

		else if ( same(key, "arrow_name") )
			{
			(void) strcpy(arrow_name, action);
			}

		else if ( same(key, "x_off") )
			{
			(void) sscanf(action, "%f", &xoff);
			}

		else if ( same(key, "y_off") )
			{
			(void) sscanf(action, "%f", &yoff);
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

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Ensure that local component presentation is filled in */
	(void) check_comp_pres();

	/* Error return for missing parameters */
	if ( blank(table_name) ) (void) error_report("No table name");
	if ( blank(line_name) )  (void) error_report("No line name");

	/* Now draw the table line */
	lineok = GRA_draw_table_line(table_name, last_site, line_name,
				arrow_name, xoff, yoff, pattern, pattern_width, pattern_length,
				comp_pres, num_comp);

	/* Free the local presentation buffers */
	(void) free_comp_pres();

	/* Return results of line drawing */
	return lineok;
	}

/***********************************************************************
*                                                                      *
*    d o _ d r a w _ c r o s s _ s e c t i o n _ l i n e               *
*                                                                      *
***********************************************************************/

LOGICAL		do_draw_cross_section_line

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	LOGICAL			lineok;
	int				ii;
	STRING			next;
	float			fval;
	double			dval;
	LOGICAL			status;
	char			cross_section_name[GPGMedium], line_to_draw[GPGMedium];
	char			axis_for_display[GPGMedium];
	char			location_look_up[GPGMedium], location_units[GPGMedium];
	char			vertical_look_up[GPGMedium];
	char			line_name[GPGMedium], arrow_name[GPGMedium];
	float			xoff, yoff;
	char			pattern[GPGMedium];
	char			pattern_width[GPGMedium], pattern_length[GPGMedium];
	int				num_xloc, ltype;
	XSECT_LOCATION	*xsect_locs;
	int				num_comp, num_width, num_style, num_outline, num_fill;
	COMP_PRES		*comp_pres;
	STRING			key, action;
	char			tbuf[GPGMedium], err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(cross_section_name, FpaCblank);
	(void) strcpy(line_to_draw,       FpaCblank);
	(void) strcpy(axis_for_display,   FpaCblank);
	(void) strcpy(location_look_up,   FpaCblank);
	(void) strcpy(vertical_look_up,   FpaCblank);
	(void) strcpy(line_name,          FpaCblank);
	(void) strcpy(arrow_name,         FpaCblank);
	(void) strcpy(pattern,            FpaCblank);
	(void) strcpy(pattern_width,      FpaCblank);
	(void) strcpy(pattern_length,     FpaCblank);
	xoff        = 0.0;
	yoff        = 0.0;
	num_xloc    = 0;
	num_comp    = 0;
	num_width   = 0;
	num_style   = 0;
	num_outline = 0;
	num_fill    = 0;
	ltype       = GPG_LocNone;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "cross_section_name") )
			{
			(void) strcpy(cross_section_name, action);
			}

		else if ( same(key, "line_to_draw") )
			{
			(void) sscanf(action, "%s", line_to_draw);
			}

		else if ( same(key, "axis_for_display") )
			{
			(void) sscanf(action, "%s", axis_for_display);
			}

		else if ( same(key, "location_look_up") )
			{
			(void) sscanf(action, "%s", location_look_up);
			}

		else if ( same(key, "location_distances") )
			{
			if      (ltype == GPG_LocNone) ltype = GPG_LocDistance;
			else if (ltype != GPG_LocDistance)
				{
				if (ltype == GPG_LocTime)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_distances\" and \"location_times\" in same directive");
				else if (ltype == GPG_LocFraction)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_distances\" and \"location_fractions\" in same directive");
				(void) error_report(err_buf);
				}
			(void) strcpy(tbuf, action);
			while (TRUE)
				{
				fval = float_arg(tbuf, &status);
				if ( !status ) break;

				num_xloc = add_xsect_location(&xsect_locs);
				xsect_locs[num_xloc-1].xdist = fval;
				}
			}

		else if ( same(key, "location_units") )
			{
			(void) strcpy(location_units, action);
			}

		else if ( same(key, "location_times") )
			{
			if      (ltype == GPG_LocNone) ltype = GPG_LocTime;
			else if (ltype != GPG_LocTime)
				{
				if (ltype == GPG_LocDistance)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_times\" and \"location_distances\" in same directive");
				else if (ltype == GPG_LocFraction)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_times\" and \"location_fractions\" in same directive");
				(void) error_report(err_buf);
				}
			(void) strcpy(tbuf, action);
			while ( !blank( next = string_arg(tbuf) ) )
				{
				num_xloc = add_xsect_location(&xsect_locs);
				(void) strcpy(xsect_locs[num_xloc-1].vtime, next);
				}
			}

		else if ( same(key, "location_fractions") )
			{
			if      (ltype == GPG_LocNone) ltype = GPG_LocFraction;
			else if (ltype != GPG_LocFraction)
				{
				if (ltype == GPG_LocDistance)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_fractions\" and \"location_distances\" in same directive");
				else if (ltype == GPG_LocTime)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_fractions\" and \"location_times\" in same directive");
				(void) error_report(err_buf);
				}
			(void) strcpy(tbuf, action);
			while (TRUE)
				{
				fval = float_arg(tbuf, &status);
				if ( !status ) break;

				num_xloc = add_xsect_location(&xsect_locs);
				xsect_locs[num_xloc-1].xdist = fval;
				}
			}

		else if ( same(key, "vertical_look_up") )
			{
			(void) sscanf(action, "%s", vertical_look_up);
			}

		else if ( same(key, "line_name") )
			{
			(void) strcpy(line_name, action);
			}

		else if ( same(key, "arrow_name") )
			{
			(void) strcpy(arrow_name, action);
			}

		else if ( same(key, "x_off") )
			{
			(void) sscanf(action, "%f", &xoff);
			}

		else if ( same(key, "y_off") )
			{
			(void) sscanf(action, "%f", &yoff);
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

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Ensure that local component presentation is filled in */
	(void) check_comp_pres();

	/* Error return for missing parameters */
	if ( blank(cross_section_name) )
		(void) error_report("No cross section name");
	if ( blank(line_to_draw) )
		(void) error_report("No cross section type of line to draw");

	/* Error return for incorrect cross section line type */
	if ( !same(line_to_draw, XSectAxis) && !same(line_to_draw, XSectTicks)
			&& !same(line_to_draw, XSectHLines)
			&& !same(line_to_draw, XSectVLines) )
		{
		(void) sprintf(err_buf, "Recognized types of line to draw: %s %s %s %s",
				XSectAxis, XSectTicks, XSectHLines, XSectVLines);
		(void) error_report(err_buf);
		}

	/* Error return for missing parameters based on type of line */
	if ( same(line_to_draw, XSectAxis) || same(line_to_draw, XSectTicks) )
		{
		if ( blank(axis_for_display) )
			(void) error_report("No cross section axis for display");
		if ( !same(axis_for_display, XSectAxisLower)
				&& !same(axis_for_display, XSectAxisUpper)
				&& !same(axis_for_display, XSectAxisLeft)
				&& !same(axis_for_display, XSectAxisRight) )
			{
			(void) sprintf(err_buf, "Recognized axis for display: %s %s %s %s",
					XSectAxisLower, XSectAxisUpper,
					XSectAxisLeft, XSectAxisRight);
			(void) error_report(err_buf);
			}
		}
	if ( same(line_to_draw, XSectTicks) )
		{
		if ( blank(line_name) )
			(void) error_report("No line name for cross section ticks");
		}

	/* Convert cross section values to km (if required) */
	for ( ii=0; ii<num_xloc; ii++ )
		{
		if ( !convert_value(location_units, (double) xsect_locs[ii].xdist,
				LocationUnitsKm, &dval) )
			{
			(void) sprintf(err_buf,
					"Incorrect location units: %s", location_units);
			(void) error_report(err_buf);
			}
		xsect_locs[ii].xdist = (float) dval;
		}

	/* Set the parameter for cross section display */
	AnchorToCrossSection = TRUE;

	/* Now draw the cross section line */
	lineok = GRA_draw_cross_section_line(cross_section_name, line_to_draw,
				axis_for_display, location_look_up, ltype, num_xloc, xsect_locs,
				vertical_look_up, line_name, arrow_name, xoff, yoff,
				pattern, pattern_width, pattern_length, comp_pres, num_comp);

	/* Unset the parameter for cross section display */
	AnchorToCrossSection = FALSE;

	/* Free the cross section location buffers */
	(void) free_xsect_locations();

	/* Free the local presentation buffers */
	(void) free_comp_pres();

	/* Return results of line drawing */
	return lineok;
	}

/***********************************************************************
*                                                                      *
*    d o _ d r a w _ d i s t a n c e _ s c a l e                       *
*    d o _ d i s t a n c e _ s c a l e _ t i c k s                     *
*    d o _ d i s t a n c e _ s c a l e _ l a b e l s                   *
*                                                                      *
***********************************************************************/

LOGICAL		do_draw_distance_scale

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	char		scale_name[GPGMedium], scale_units[GPGMedium];
	char		scale_just[GPGMedium];
	float		slength, srotate, xoff, yoff;
	double		dlength;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(scale_name,  FpaCblank);
	(void) strcpy(scale_units, FpaCmksUnits);
	(void) strcpy(scale_just,  JustifyCentre);
	slength = 0.0;
	srotate = 0.0;
	xoff    = 0.0;
	yoff    = 0.0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "scale_name") )
			{
			(void) strcpy(scale_name, action);
			}

		else if ( same(key, "scale_length") )
			{
			(void) sscanf(action, "%f", &slength);
			}

		else if ( same(key, "scale_units") )
			{
			(void) sscanf(action, "%s", scale_units);
			}

		else if ( same(key, "scale_justification") )
			{
			(void) sscanf(action, "%s", scale_just);
			}

		else if ( same(key, "scale_rotation") )
			{
			(void) sscanf(action, "%f", &srotate);
			}

		else if ( same(key, "x_off") )
			{
			(void) sscanf(action, "%f", &xoff);
			}

		else if ( same(key, "y_off") )
			{
			(void) sscanf(action, "%f", &yoff);
			}

		/* Skip valid presentation keywords already processed */
		else if ( same(key,  "line_width")
				|| same(key, "line_style")
				|| same(key, "outline") )
			{
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Error return for missing parameters */
	if ( blank(scale_name) )
		(void) error_report("No distance scale name");
	if ( slength == 0.0 )
		(void) error_report("No scale length");

	/* Convert scale length to m */
	if ( !convert_value(scale_units, (double) slength,
			DistanceScaleUnitsM, &dlength) )
		{
		(void) sprintf(err_buf,
				"Incorrect distance scale units: %s", scale_units);
		(void) error_report(err_buf);
		}
	slength = (float) dlength;

	/* First add the distance scale to the list */
	(void) add_distance_scale(scale_name, slength, scale_just, srotate,
				xoff, yoff);

	/* Now draw the distance scale */
	return GRA_draw_distance_scale(scale_name);
	}

LOGICAL		do_distance_scale_ticks

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	LOGICAL		ticksok;
	int			ii, num_lctns, nlctn;
	SCALE_LCTNS	*scale_lctns;
	char		scale_name[GPGMedium], tick_units[GPGMedium];
	char		tick_just[GPGMedium];
	float		tlctn, tlength, trotate;
	double		dlength, dlctn;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(scale_name, FpaCblank);
	(void) strcpy(tick_units, FpaCmksUnits);
	(void) strcpy(tick_just,  JustifyCentre);
	num_lctns = 0;
	nlctn     = 0;
	tlctn     = 0.0;
	tlength   = 0.0;
	trotate   = 0.0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "scale_name") )
			{
			(void) strcpy(scale_name, action);
			}

		else if ( same(key, "tick_length") )
			{
			(void) sscanf(action, "%f", &tlength);
			}

		else if ( same(key, "tick_location") )
			{
			(void) sscanf(action, "%f", &tlctn);
			num_lctns = add_distance_scale_location(&scale_lctns, ++nlctn);
			scale_lctns[nlctn-1].lctn = tlctn;
			}

		else if ( same(key, "tick_units") )
			{
			(void) sscanf(action, "%s", tick_units);
			}

		else if ( same(key, "tick_justification") )
			{
			(void) sscanf(action, "%s", tick_just);
			}

		else if ( same(key, "tick_rotation") )
			{
			(void) sscanf(action, "%f", &trotate);
			}

		/* Skip valid presentation keywords already processed */
		else if ( same(key,  "line_width")
				|| same(key, "line_style")
				|| same(key, "outline") )
			{
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Error return for missing parameters */
	if ( blank(scale_name) )
		(void) error_report("No distance scale name");
	if ( tlength == 0.0 )
		(void) error_report("No tick length");

	/* Convert tick length and tick locations to m */
	if ( !convert_value(tick_units, (double) tlength,
			DistanceScaleUnitsM, &dlength) )
		{
		(void) sprintf(err_buf, "Incorrect tick units: %s", tick_units);
		(void) error_report(err_buf);
		}
	tlength = (float) dlength;
	for ( ii=0; ii<num_lctns; ii++ )
		{
		tlctn = scale_lctns[ii].lctn;
		if ( !convert_value(tick_units, (double) tlctn,
				DistanceScaleUnitsM, &dlctn) )
			{
			(void) sprintf(err_buf, "Incorrect tick units: %s", tick_units);
			(void) error_report(err_buf);
			}
		scale_lctns[ii].lctn = (float) dlctn;
		}

	/* Display the distance scale tick marks */
	ticksok = GRA_display_dscale_ticks(scale_name, num_lctns, scale_lctns,
				tlength, tick_just, trotate);

	/* Free distance scale locations buffer */
	(void) free_distance_scale_locations();

	/* Return results of display of distance scale tick marks */
	return ticksok;
	}

LOGICAL		do_distance_scale_labels

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	LOGICAL		labelsok;
	int			ii, num_lctns, nlctn, nstrg;
	SCALE_LCTNS	*scale_lctns;
	char		scale_name[GPGMedium], label_units[GPGMedium];
	char		label_just[GPGMedium];
	float		llctn, size, lrotate, txt_size, xoff, yoff;
	double		dlctn;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Set current text size */
	(void) sscanf(CurPres.text_size, "%f", &size);

	/* Initialize all parameters */
	(void) strcpy(scale_name,  FpaCblank);
	(void) strcpy(label_units, FpaCmksUnits);
	(void) strcpy(label_just,  JustifyCentre);
	num_lctns = 0;
	nlctn     = 0;
	nstrg     = 0;
	lrotate   = 0.0;
	txt_size  = size;
	xoff      = 0.0;
	yoff      = 0.0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "scale_name") )
			{
			(void) strcpy(scale_name, action);
			}

		else if ( same(key, "label_location") )
			{
			(void) sscanf(action, "%f", &llctn);
			num_lctns = add_distance_scale_location(&scale_lctns, ++nlctn);
			scale_lctns[nlctn-1].lctn = llctn;
			}

		else if ( same(key, "label_string") )
			{
			num_lctns = add_distance_scale_location(&scale_lctns, ++nstrg);
			(void) strcpy(scale_lctns[nstrg-1].label, action);
			}

		else if ( same(key, "label_units") )
			{
			(void) sscanf(action, "%s", label_units);
			}

		else if ( same(key, "label_justification") )
			{
			(void) sscanf(action, "%s", label_just);
			}

		else if ( same(key, "label_rotation") )
			{
			(void) sscanf(action, "%f", &lrotate);
			}

		else if ( same(key, "text_size") )
			{
			(void) sscanf(action, "%f", &txt_size);
			}

		else if ( same(key, "x_off") )
			{
			(void) sscanf(action, "%f", &xoff);
			}

		else if ( same(key, "y_off") )
			{
			(void) sscanf(action, "%f", &yoff);
			}

		/* Skip valid presentation keywords already processed */
		else if ( same(key,  "line_width")
				|| same(key, "outline")
				|| same(key, "fill")
				|| same(key, "outline_first")
				|| same(key, "font")
				|| same(key, "font_weight")
				|| same(key, "italics")
				|| same(key, "char_space")
				|| same(key, "word_space") )
			{
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Error return for missing parameters */
	if ( blank(scale_name) )
		(void) error_report("No distance scale name");

	/* Convert label locations to m */
	for ( ii=0; ii<num_lctns; ii++ )
		{
		llctn = scale_lctns[ii].lctn;
		if ( !convert_value(label_units, (double) llctn,
				DistanceScaleUnitsM, &dlctn) )
			{
			(void) sprintf(err_buf, "Incorrect label units: %s", label_units);
			(void) error_report(err_buf);
			}
		scale_lctns[ii].lctn = (float) dlctn;
		}

	/* Display the distance scale labels */
	labelsok = GRA_display_dscale_labels(scale_name, num_lctns, scale_lctns,
				txt_size, label_just, lrotate, xoff, yoff);

	/* Free distance scale locations buffer */
	(void) free_distance_scale_locations();

	/* Return results of display of distance scale labels */
	return labelsok;
	}

/***********************************************************************
*                                                                      *
*    d o _ e l l i p s e                                               *
*                                                                      *
***********************************************************************/

LOGICAL		do_ellipse

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	char		lat[GPGMedium], lon[GPGMedium];
	char		map_x[GPGMedium], map_y[GPGMedium];
	char		location_ident[GPGMedium], location_look_up[GPGMedium];
	char		vtime[GPGTiny], table_name[GPGMedium];
	char		grid_name[GPGMedium], list_name[GPGMedium];
	char		sym_fill_name[GPGMedium];
	LOGICAL		closed, rotate_lat, rotate_lon;
	float		width, height, diameter, radius;
	float		sangle, eangle, rotation, xoff, yoff;
	float		map_units, clon;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(lat,              FpaCblank);
	(void) strcpy(lon,              FpaCblank);
	(void) strcpy(map_x,            FpaCblank);
	(void) strcpy(map_y,            FpaCblank);
	(void) strcpy(location_ident,   FpaCblank);
	(void) strcpy(location_look_up, FpaCblank);
	(void) strcpy(vtime,            TVstamp);
	(void) strcpy(table_name,       FpaCblank);
	(void) strcpy(grid_name,        FpaCblank);
	(void) strcpy(list_name,        FpaCblank);
	(void) strcpy(sym_fill_name,    FpaCblank);
	width      = 0.0;
	height     = 0.0;
	diameter   = 0.0;
	radius     = 0.0;
	sangle     = 0.0;
	eangle     = 0.0;
	closed     = TRUE;
	rotation   = 0.0;
	rotate_lat = FALSE;
	rotate_lon = FALSE;
	map_units  = BaseMap.definition.units;
	xoff       = 0.0;
	yoff       = 0.0;

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

		else if ( same(key, "diameter") )
			{
			(void) sscanf(action, "%f", &diameter);
			}

		else if ( same(key, "radius") )
			{
			(void) sscanf(action, "%f", &radius);
			}

		else if ( same(key, "start_angle") )
			{
			(void) sscanf(action, "%f", &sangle);
			}

		else if ( same(key, "end_angle") )
			{
			(void) sscanf(action, "%f", &eangle);
			}

		else if ( same(key, "closed") )
			{
			if ( same_ic(action, LogicalYes) ) closed = TRUE;
			else                               closed = FALSE;
			}

		else if ( same(key, "rotation") )
			{
			(void) sscanf(action, "%f", &rotation);
			}

		else if ( same(key, "rotate_to_latitude") )
			{
			if ( same_ic(action, LogicalYes) ) rotate_lat = TRUE;
			else                               rotate_lat = FALSE;
			}

		else if ( same(key, "rotate_to_longitude") )
			{
			if ( same_ic(action, LogicalYes) ) rotate_lon = TRUE;
			else                               rotate_lon = FALSE;
			}

		else if ( same(key, "x_off") )
			{
			(void) sscanf(action, "%f", &xoff);
			}

		else if ( same(key, "y_off") )
			{
			(void) sscanf(action, "%f", &yoff);
			}

		else if ( same(key, "lat") )
			{
			(void) strcpy(lat, action);
			}

		else if ( same(key, "lon") )
			{
			(void) strcpy(lon, action);
			}

		else if ( same(key, "map_x") )
			{
			(void) strcpy(map_x, action);
			}

		else if ( same(key, "map_y") )
			{
			(void) strcpy(map_y, action);
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
			(void) strcpy(location_ident, action);
			}

		else if ( same(key, "location_look_up") )
			{
			(void) strcpy(location_look_up, action);
			}

		else if ( same(key, "valid_time") )
			{
			(void) grid_center(&BaseMap, NullPointPtr, NullFloat, &clon);
			(void) strcpy(vtime, interpret_timestring(action, T0stamp, clon));
			}

		else if ( same(key, "table_name") )
			{
			(void) strcpy(table_name, action);
			}

		else if ( same(key, "grid_name") )
			{
			(void) strcpy(grid_name, action);
			}

		else if ( same(key, "list_name") )
			{
			(void) strcpy(list_name, action);
			}

		else if ( same(key, "symbol_fill_name") )
			{
			(void) strcpy(sym_fill_name, action);
			}

		/* Skip valid presentation keywords already processed */
		else if ( same(key,  "line_width")
				|| same(key, "line_style")
				|| same(key, "outline")
				|| same(key, "interior_fill") )
			{
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Error return for missing parameters */
	if ( (width <= 0.0 || height <= 0.0 ) && diameter <= 0.0 && radius <= 0.0 )
		(void) error_report("No width and height or diameter or radius");

	/* Set width and height from diameter or radius (if required) */
	if ( width <= 0.0 || height <= 0.0 )
		{
		if ( diameter > 0.0 )
			{
			width  = diameter;
			height = diameter;
			}
		else if ( radius > 0.0 )
			{
			width  = 2.0 * radius;
			height = 2.0 * radius;
			}
		}

	/* Error if "symbol_fill_name" and "closed = no" set */
	if ( !closed && !blank(sym_fill_name) )
		(void) error_report("Cannot set \"symbol_fill_name\" if \"closed = no\"");

	/* Error if both "rotate_to_latitude" and "rotate_to_longitude" set */
	if ( rotate_lat && rotate_lon )
		(void) error_report("Can set only one of \"rotate_to_latitude\" \"rotate_to_longitude\"");

	/* Now display the ellipse */
	return GRA_display_ellipse(width, height, sangle, eangle, closed,
			rotation, rotate_lat, rotate_lon, xoff, yoff, lat, lon,
			map_x, map_y, map_units, location_ident, location_look_up,
			vtime, table_name, grid_name, list_name, sym_fill_name);
	}

/***********************************************************************
*                                                                      *
*    d o _ g e o g r a p h y                                           *
*                                                                      *
***********************************************************************/

LOGICAL		do_geography

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	LOGICAL		displayok;
	int			ii;
	char		geo_file[GPGMedium];
	char		element[GPGMedium], level[GPGMedium];
	char		cat_cascade[GPGMedium], attribute[GPGMedium];
	int			num_catatt, num_att, num_cat;
	CATATTRIB	*cat_attrib;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(geo_file,    FpaCblank);
	(void) strcpy(element,     FpaCblank);
	(void) strcpy(level,       FpaCblank);
	(void) strcpy(cat_cascade, CatCascadeAnd);
	(void) strcpy(attribute,   AttribCategory);
	num_catatt = 0;
	num_att    = 0;
	num_cat    = 0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "geo_file") )
			{
			(void) strcpy(geo_file, action);
			}

		/* >>> this gets replaced by "geo_file" above <<< */
		else if ( (OldVersion || ObsoleteVersion) && same(key, "file") )
			{
			(void) fprintf(stderr, ">>> Obsolete keyword in directive");
			(void) fprintf(stderr, " ... \"@geography\" <<<\n");
			(void) fprintf(stderr, ">>>   Replace \"file\"");
			(void) fprintf(stderr, " by \"geo_file\" <<<\n");
			(void) strcpy(geo_file, action);
			}

		else if ( same(key, "element") )
			{
			(void) strcpy(element, action);
			}

		else if ( same(key, "level") )
			{
			(void) strcpy(level, action);
			}

		else if ( same(key, "category_cascade") )
			{
			(void) strcpy(cat_cascade, action);
			}

		else if ( same(key, "category_attribute") )
			{
			num_catatt = add_category_attribs(&cat_attrib, ++num_att);
			(void) strcpy(cat_attrib[num_att-1].category_attribute, action);
			}

		else if ( same(key, "category") )
			{
			num_catatt = add_category_attribs(&cat_attrib, ++num_cat);
			(void) strcpy(cat_attrib[num_cat-1].category, action);
			}

		/* >>> this gets replaced by "category" above <<< */
		else if ( ObsoleteVersion && same(key, "subelements") )
			{
			(void) fprintf(stderr, ">>> Obsolete keyword in directive");
			(void) fprintf(stderr, " ... \"@geography\" <<<\n");
			(void) fprintf(stderr, ">>>   Replace \"subelements\"");
			(void) fprintf(stderr, " by \"category\" <<<\n");
			num_catatt = add_category_attribs(&cat_attrib, ++num_cat);
			(void) strcpy(cat_attrib[num_cat-1].category, action);
			}

		else if ( same(key, "attribute") )
			{
			(void) strcpy(attribute, action);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Ensure that at least one set of category parameters has been set */
	/*  ... or initialize one with some default values!                 */
	if ( num_catatt < 1 )
		{
		num_catatt = add_category_attribs(&cat_attrib, ++num_att);
		}

	/* Error return for missing parameters */
	if ( blank(geo_file) )    (void) error_report("No file name");
	if ( blank(element) )     (void) error_report("No element");
	if ( blank(level) )       (void) error_report("No level");

	/* Now display the geography */
	displayok = GRA_display_geography(geo_file, element, level,
				cat_cascade, cat_attrib, num_catatt, attribute);

	/* Free category attribute buffers */
	(void) free_category_attribs();

	/* Return results of geography display */
	return displayok;
	}

/***********************************************************************
*                                                                      *
*    d o _ i m a g e s                                                 *
*                                                                      *
***********************************************************************/

LOGICAL		do_images

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	LOGICAL			displayok;
	int				ii;
	STRING			next;
	char			sat_bright[GPGMedium], radar_bright[GPGMedium];
	char			vtime[GPGTiny];
	char			match_before[GPGMedium], match_after[GPGMedium];
	char			sat_before[GPGMedium],   sat_after[GPGMedium];
	char			radar_before[GPGMedium], radar_after[GPGMedium];
	char			range_units[GPGMedium];
	char			range_colour[GPGMedium], limit_colour[GPGMedium];
	float			blend_ratio, range_interval, clon;
	double			drange;
	int				num_images, num_itags, num_ctabs, num_bright;
	GRA_IMAGE_INFO	*image_info;
	LOGICAL			blend_images, range_rings, limit_ring;
	STRING			key, action;
	char			tbuf[GPGMedium], err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(sat_bright,   FpaCblank);
	(void) strcpy(radar_bright, FpaCblank);
	(void) strcpy(vtime,        TVstamp);
	(void) strcpy(match_before, FpaCblank);
	(void) strcpy(match_after,  FpaCblank);
	(void) strcpy(sat_before,   FpaCblank);
	(void) strcpy(sat_after,    FpaCblank);
	(void) strcpy(radar_before, FpaCblank);
	(void) strcpy(radar_after,  FpaCblank);
	(void) strcpy(range_units,  RangeRingUnitsKm);
	(void) strcpy(range_colour, FpaCblank);
	(void) strcpy(limit_colour, FpaCblank);
	blend_images   = FALSE;
	range_rings    = FALSE;
	limit_ring     = FALSE;
	blend_ratio    = 100.0;
	range_interval =  50.0;
	num_images     = 0;
	num_itags      = 0;
	num_ctabs      = 0;
	num_bright     = 0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "image_tag") )
			{
			num_images = add_graphics_image(&image_info, ++num_itags);
			(void) strcpy(image_info[num_itags-1].image_tag, action);
			}

		else if ( same(key, "image_tag_list") )
			{
			(void) strcpy(tbuf, action);
			while ( !blank( next = string_arg(tbuf) ) )
				{
				num_images = add_graphics_image(&image_info, ++num_itags);
				(void) strcpy(image_info[num_itags-1].image_tag, next);
				}
			}

		else if ( same(key, "colour_table") )
			{
			num_images = add_graphics_image(&image_info, ++num_ctabs);
			(void) strcpy(image_info[num_ctabs-1].ctable, action);
			}

		else if ( same(key, "colour_table_list") )
			{
			(void) strcpy(tbuf, action);
			while ( !blank( next = string_arg(tbuf) ) )
				{
				num_images = add_graphics_image(&image_info, ++num_ctabs);
				(void) strcpy(image_info[num_ctabs-1].ctable, next);
				}
			}

		else if ( same(key, "brightness") )
			{
			num_images = add_graphics_image(&image_info, ++num_bright);
			(void) strcpy(image_info[num_bright-1].brightness, action);
			}

		else if ( same(key, "brightness_list") )
			{
			(void) strcpy(tbuf, action);
			while ( !blank( next = string_arg(tbuf) ) )
				{
				num_images = add_graphics_image(&image_info, ++num_bright);
				(void) strcpy(image_info[num_bright-1].brightness, next);
				}
			}

		else if ( same(key, "satellite_brightness") )
			{
			(void) strcpy(sat_bright, action);
			}

		else if ( same(key, "radar_brightness") )
			{
			(void) strcpy(radar_bright, action);
			}

		else if ( same(key, "valid_time") )
			{
			(void) grid_center(&BaseMap, NullPointPtr, NullFloat, &clon);
			(void) strcpy(vtime, interpret_timestring(action, T0stamp, clon));
			}

		else if ( same(key, "match_time_before") )
			{
			(void) strcpy(match_before, action);
			}

		else if ( same(key, "match_time_after") )
			{
			(void) strcpy(match_after, action);
			}

		else if ( same(key, "satellite_time_before") )
			{
			(void) strcpy(sat_before, action);
			}

		else if ( same(key, "satellite_time_after") )
			{
			(void) strcpy(sat_after, action);
			}

		else if ( same(key, "radar_time_before") )
			{
			(void) strcpy(radar_before, action);
			}

		else if ( same(key, "radar_time_after") )
			{
			(void) strcpy(radar_after, action);
			}

		else if ( same(key, "blend_images") )
			{
			if ( same_ic(action, LogicalYes) ) blend_images = TRUE;
			else                               blend_images = FALSE;
			}

		else if ( same(key, "blend_ratio") )
			{
			(void) sscanf(action, "%f", &blend_ratio);
			}

		else if ( same(key, "radar_range_rings") )
			{
			if ( same_ic(action, LogicalYes) ) range_rings = TRUE;
			else                               range_rings = FALSE;
			}

		else if ( same(key, "radar_range_ring_interval") )
			{
			(void) sscanf(action, "%f", &range_interval);
			}

		else if ( same(key, "radar_range_ring_units") )
			{
			(void) strcpy(range_units, action);
			}

		else if ( same(key, "radar_range_ring_colour") )
			{
			(void) strcpy(range_colour, action);
			}

		else if ( same(key, "radar_limit_ring") )
			{
			if ( same_ic(action, LogicalYes) ) limit_ring = TRUE;
			else                               limit_ring = FALSE;
			}

		else if ( same(key, "radar_limit_ring_colour") )
			{
			(void) strcpy(limit_colour, action);
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Error return for missing parameters */
	if ( num_images <= 0 )
		(void) error_report("No image identifiers");

	/* Convert range ring interval to km */
	if ( !convert_value(range_units, (double) range_interval,
			RangeRingUnitsKm, &drange) )
		{
		(void) sprintf(err_buf,
				"Incorrect radar range ring units: %s", range_units);
		(void) error_report(err_buf);
		}
	range_interval = (float) drange;

	/* Now display all the images */
	displayok = GRA_display_all_images(image_info, num_images,
				sat_bright, radar_bright, vtime, match_before, match_after,
				sat_before, sat_after, radar_before, radar_after,
				blend_images, blend_ratio, range_rings, range_interval,
				range_colour, limit_ring, limit_colour);

	/* Free image buffers */
	(void) free_graphics_image();

	/* Return results of image display */
	return displayok;
	}

/***********************************************************************
*                                                                      *
*    d o _ l a b e l                                                   *
*                                                                      *
***********************************************************************/

LOGICAL		do_label

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	LOGICAL			displayok;
	int				ii, nn;
	char			element[GPGMedium], level[GPGMedium];
	char			units[GPGMedium], geo_file[GPGMedium];
	char			cat_cascade[GPGMedium];
	char			tzone[GPGMedium], tlang[GPGMedium];
	char			format[GPGMedium], conversion_format[GPGMedium];
	char			look_up[GPGMedium], mark[GPGMedium];
	char			display_name[GPGMedium], display_type[GPGMedium];
	char			fit_to_map_ref[GPGMedium];
	char			rot_attrib[GPGMedium];
	LOGICAL			rotate_lat, rotate_lon, constrain_rot;
	int				num_catatt, num_att, num_cat;
	CATATTRIB		*cat_attrib;
	int				num_attrib, num_list, num_case, num_look;
	LOGICAL			fit_to_map, need_default_attrib;
	ATTRIB_DISPLAY	*temp_attrib, *attribs;
	SPCASE			*list_case;
	int				xwrap, ywrap;
	float			size, markscale, symscale, txt_size, wthscale, hgtscale;
	float			rotation, xsoff, ysoff, xdoff, ydoff, xoff, yoff, xval;
	STRING			key, action;
	char			err_buf[GPGLong];

	/* Set current text size */
	(void) sscanf(CurPres.text_size, "%f", &size);

	/* Initialize all parameters */
	(void) strcpy(element,           FpaCblank);
	(void) strcpy(level,             FpaCblank);
	(void) strcpy(units,             FpaCmksUnits);
	(void) strcpy(geo_file,          FpaCblank);
	(void) strcpy(cat_cascade,       CatCascadeAnd);
	(void) strcpy(tzone,             FpaCblank);
	(void) strcpy(tlang,             FpaCblank);
	(void) strcpy(format,            FpaCblank);
	(void) strcpy(conversion_format, FpaCblank);
	(void) strcpy(look_up,           FpaCblank);
	(void) strcpy(mark,              FpaCblank);
	(void) strcpy(display_name,      FpaCblank);
	(void) strcpy(display_type,      FpaCblank);
	(void) strcpy(fit_to_map_ref,    FitToMapRefNone);
	(void) strcpy(rot_attrib,        FpaCblank);
	fit_to_map    = FALSE;
	markscale     = 100.0;
	symscale      = 100.0;
	txt_size      =  size;
	wthscale      = Program.default_width_scale;
	hgtscale      = Program.default_height_scale;
	rotation      =   0.0;
	rotate_lat    = FALSE;
	rotate_lon    = FALSE;
	constrain_rot =  TRUE;
	xsoff         =   0.0;
	ysoff         =   0.0;
	xwrap         =   1;
	ywrap         =   1;
	xdoff         =   0.0;
	ydoff         =   0.0;
	xoff          =   0.0;
	yoff          =   0.0;
	num_catatt    =     0;
	num_att       =     0;
	num_cat       =     0;
	num_list      =     0;
	num_case      =     0;
	num_look      =     0;

	/* Process all keywords in list ... except the attribute keywords */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "element") )
			{
			(void) strcpy(element, action);
			}

		else if ( same(key, "level") )
			{
			(void) strcpy(level, action);
			}

		else if ( same(key, "units") )
			{
			(void) strcpy(units, action);
			}

		else if ( same(key, "geo_file") )
			{
			(void) strcpy(geo_file, action);
			}

		else if ( same(key, "category_cascade") )
			{
			(void) strcpy(cat_cascade, action);
			}

		else if ( same(key, "category_attribute") )
			{
			num_catatt = add_category_attribs(&cat_attrib, ++num_att);
			(void) strcpy(cat_attrib[num_att-1].category_attribute, action);
			}

		else if ( same(key, "category") )
			{
			num_catatt = add_category_attribs(&cat_attrib, ++num_cat);
			(void) strcpy(cat_attrib[num_cat-1].category, action);
			}

		else if ( same(key, "time_zone") )
			{
			(void) strcpy(tzone, action);
			}

		else if ( same(key, "language") )
			{
			(void) strcpy(tlang, action);
			}

		else if ( same(key, "format") )
			{
			(void) strcpy(format, string_arg(action));
			if ( !blank(action) )
				{
				(void) no_white(action);
				(void) strcpy(conversion_format, action);
				}
			}

		else if ( same(key, "look_up") )
			{
			(void) strcpy(look_up, action);
			}

		else if ( same(key, "case") )
			{
			num_list = add_list_case(&list_case, ++num_case);
			(void) strcpy(list_case[num_case-1].spcase, action);
			}

		else if ( same(key, "case_look_up") )
			{
			num_list = add_list_case(&list_case, ++num_look);
			(void) strcpy(list_case[num_look-1].lookup, action);
			}

		else if ( same(key, "mark") )
			{
			(void) strcpy(mark, find_symbol_file(action));
			}

		else if ( same(key, "mark_scale") )
			{
			(void) sscanf(action, "%f", &markscale);
			}

		else if ( same(key, "symbol_scale") )
			{
			(void) sscanf(action, "%f", &symscale);
			}

		else if ( same(key, "text_size") )
			{
			(void) sscanf(action, "%f", &txt_size);
			}

		else if ( same(key, "display_name") )
			{
			(void) strcpy(display_name, action);
			}

		else if ( same(key, "display_type") )
			{
			(void) strcpy(display_type, action);
			}

		else if ( same(key, "width_scale") )
			{
			(void) sscanf(action, "%f", &wthscale);
			}

		else if ( same(key, "height_scale") )
			{
			(void) sscanf(action, "%f", &hgtscale);
			}

		else if ( same(key, "fit_to_map") )
			{
			if ( same_ic(action, LogicalYes) ) fit_to_map = TRUE;
			else                               fit_to_map = FALSE;
			}

		else if ( same(key, "fit_to_map_ref") )
			{
			(void) strcpy(fit_to_map_ref, action);
			}

		else if ( same(key, "rotation") )
			{
			(void) sscanf(action, "%f", &rotation);
			}

		else if ( same(key, "rotate_to_latitude") )
			{
			if ( same_ic(action, LogicalYes) ) rotate_lat = TRUE;
			else                               rotate_lat = FALSE;
			}

		else if ( same(key, "rotate_to_longitude") )
			{
			if ( same_ic(action, LogicalYes) ) rotate_lon = TRUE;
			else                               rotate_lon = FALSE;
			}

		else if ( same(key, "rotation_attribute") )
			{
			(void) strcpy(rot_attrib, action);
			}

		else if ( same(key, "constrain_rotation") )
			{
			if ( same_ic(action, LogicalYes) ) constrain_rot = TRUE;
			else                               constrain_rot = FALSE;
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

		else if ( same(key, "x_display_off") )
			{
			(void) sscanf(action, "%f", &xdoff);
			}

		else if ( same(key, "y_display_off") )
			{
			(void) sscanf(action, "%f", &ydoff);
			}

		else if ( same(key, "x_off") )
			{
			(void) sscanf(action, "%f", &xoff);
			}

		else if ( same(key, "y_off") )
			{
			(void) sscanf(action, "%f", &yoff);
			}

		/* Skip valid attribute keywords (for the moment) */
		else if ( same(key,  "attribute")
				|| same(key, "attribute_show")
				|| same(key, "attribute_anchor")
				|| same(key, "attribute_ref")
				|| same(key, "attribute_justification")
				|| same(key, "attribute_vertical_just")
				|| same(key, "attribute_units")
				|| same(key, "attribute_format")
				|| same(key, "attribute_look_up")
				|| same(key, "attribute_symbol_scale")
				|| same(key, "attribute_text_size")
				|| same(key, "attribute_display_name")
				|| same(key, "attribute_display_type")
				|| same(key, "attribute_width_scale")
				|| same(key, "attribute_height_scale")
				|| same(key, "attribute_x_off")
				|| same(key, "attribute_y_off")
				|| same(key, "attribute_line_width")
				|| same(key, "attribute_line_style")
				|| same(key, "attribute_outline")
				|| same(key, "attribute_fill")
				|| same(key, "attribute_outline_first")
				|| same(key, "attribute_font")
				|| same(key, "attribute_font_weight")
				|| same(key, "attribute_italics")
				|| same(key, "attribute_char_space")
				|| same(key, "attribute_word_space") )
			{
			}

		/* Skip valid presentation keywords already processed */
		else if ( same(key,  "line_width")
				|| same(key, "line_style")
				|| same(key, "outline")
				|| same(key, "fill")
				|| same(key, "outline_first")
				|| same(key, "font")
				|| same(key, "font_weight")
				|| same(key, "italics")
				|| same(key, "justification")
				|| same(key, "char_space")
				|| same(key, "word_space") )
			{
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Initialize storage for attribute display list */
	(void) initialize_attribute_display();

	/* Now process all attribute keywords */
	need_default_attrib = TRUE;
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		/* Add another attribute to the list ... with default parameters */
		if ( same(key, "attribute") )
			{
			temp_attrib = add_attribute_display(action, units,
							format, conversion_format, look_up, symscale,
							txt_size, wthscale, hgtscale, &CurPres);
			need_default_attrib = FALSE;
			}

		/* Add the default attribute to the list ... if necessary */
		else if ( same_start(key, "attribute_") && need_default_attrib )
				{
				temp_attrib = add_attribute_display(AttribAutolabel, units,
								format, conversion_format, look_up, symscale,
								txt_size, wthscale, hgtscale, &CurPres);
				}

		/* Now add the attribute parameters */
		if ( same(key, "attribute") )
			{
			}

		else if ( same(key, "attribute_show") )
			{
			if ( same_ic(action, "no") ) temp_attrib->show_label = FALSE;
			else                         temp_attrib->show_label = TRUE;
			}

		/* Adjust attribute parameters for this attribute */
		else if ( same(key, "attribute_anchor") )
			{
			(void) strcpy(temp_attrib->anchor, action);
			}

		else if ( same(key, "attribute_ref") )
			{
			(void) strcpy(temp_attrib->ref, action);
			}

		else if ( same(key, "attribute_justification") )
			{
			(void) strcpy(temp_attrib->presentation.justified, action);
			}

		else if ( same(key, "attribute_vertical_just") )
			{
			(void) strcpy(temp_attrib->vertical_just, action);
			}

		else if ( same(key, "attribute_units") )
			{
			(void) strcpy(temp_attrib->units, action);
			}

		else if ( same(key, "attribute_format") )
			{
			(void) strcpy(temp_attrib->format, string_arg(action));
			if ( !blank(action) )
				{
				(void) no_white(action);
				(void) strcpy(temp_attrib->conversion_format, action);
				}
			}

		else if ( same(key, "attribute_look_up") )
			{
			(void) strcpy(temp_attrib->look_up, action);
			}

		else if ( same(key, "attribute_symbol_scale") )
			{
			(void) sscanf(action, "%f", &(temp_attrib->symbol_scale));
			}

		else if ( same(key, "attribute_text_size") )
			{
			(void) sscanf(action, "%f", &(temp_attrib->txt_size));
			}

		else if ( same(key, "attribute_display_name") )
			{
			(void) strcpy(temp_attrib->display_name, action);
			}

		else if ( same(key, "attribute_display_type") )
			{
			(void) strcpy(temp_attrib->display_type, action);
			}

		else if ( same(key, "attribute_width_scale") )
			{
			(void) sscanf(action, "%f", &(temp_attrib->width_scale));
			}

		else if ( same(key, "attribute_height_scale") )
			{
			(void) sscanf(action, "%f", &(temp_attrib->height_scale));
			}

		else if ( same(key, "attribute_x_off") )
			{
			(void) sscanf(action, "%f", &(temp_attrib->x_off));
			}

		else if ( same(key, "attribute_y_off") )
			{
			(void) sscanf(action, "%f", &(temp_attrib->y_off));
			}

		else if ( same(key, "attribute_line_width") )
			{
			(void) strcpy(temp_attrib->presentation.line_width, action);
			}

		else if ( same(key, "attribute_line_style") )
			{
			(void) strcpy(temp_attrib->presentation.line_style, action);
			}

		else if ( same(key, "attribute_outline") )
			{
			(void) strcpy(temp_attrib->presentation.outline, action);
			}

		else if ( same(key, "attribute_fill") )
			{
			(void) strcpy(temp_attrib->presentation.fill, action);
			}

		else if ( same(key, "attribute_outline_first") )
			{
			if ( same_ic(action, LogicalYes) )
				{
				temp_attrib->presentation.outline_first = TRUE;
				}
			else
				{
				temp_attrib->presentation.outline_first = FALSE;
				}
			}

		else if ( same(key, "attribute_font") )
			{
			(void) strcpy(temp_attrib->presentation.font, action);
			}

		else if ( same(key, "attribute_font_weight") )
			{
			(void) strcpy(temp_attrib->presentation.font_weight, action);
			}

		else if ( same(key, "attribute_italics") )
			{
			(void) strcpy(temp_attrib->presentation.italics, action);
			}

		else if ( same(key, "attribute_char_space") )
			{
			(void) strcpy(temp_attrib->presentation.char_space, action);
			}

		else if ( same(key, "attribute_word_space") )
			{
			(void) strcpy(temp_attrib->presentation.word_space, action);
			}
		}

	/* Ensure that at least one set of category parameters has been set */
	/*  ... or initialize one with some default values!                 */
	if ( num_catatt < 1 )
		{
		num_catatt = add_category_attribs(&cat_attrib, ++num_att);
		}

	/* Error return for missing parameters */
	if ( blank(element) ) (void) error_report("No element");
	if ( blank(level) )   (void) error_report("No level");
	if ( IsNull(identify_unit(units)) )
		{
		(void) sprintf(err_buf, "Unknown units: %s", units);
		(void) error_report(err_buf);
		}

	/* Check "format" "conversion_format" "units" "look_up" */
	/*  keywords for attributes (based on label format)     */
	num_attrib = return_attribute_display(&attribs);
	for ( nn=0; nn<num_attrib; nn++ )
		{
		if ( blank(attribs[nn].format) )
			{
			(void) sprintf(err_buf, "No label format for attribute: %s",
					attribs[nn].name);
			(void) error_report(err_buf);
			}

		/* Error return for incorrect "format" ... based on application */
		switch ( Program.macro )
			{
			case GPG_PSMet:
			case GPG_SVGMet:
			case GPG_CorMet:
				if ( !same(attribs[nn].format, FormatDirect)
						&& !same(attribs[nn].format, FormatSymbol)
						&& !same(attribs[nn].format, FormatText)
						&& !same(attribs[nn].format, FormatWindBarb)
						&& !same(attribs[nn].format, FormatWindText)
						&& !same(attribs[nn].format, FormatWindSymbol)
						&& !same(attribs[nn].format, FormatNone) )
					{
					(void) sprintf(err_buf,
							"Recognized format types are: %s %s %s %s %s %s %s",
							FormatDirect, FormatSymbol, FormatText,
							FormatWindBarb, FormatWindText, FormatWindSymbol,
							FormatNone);
					(void) error_report(err_buf);
					}
				break;

			case GPG_TexMet:
				if ( !same(attribs[nn].format, FormatDirect)
						&& !same(attribs[nn].format, FormatText)
						&& !same(attribs[nn].format, FormatWindText) )
					{
					(void) sprintf(err_buf,
							"Recognized format types are: %s %s %s",
							FormatDirect, FormatText, FormatWindText);
					(void) error_report(err_buf);
					}
				break;
			}

		if ( !blank(attribs[nn].conversion_format) )
			{
			if ( IsNull(strchr(attribs[nn].conversion_format, ConFormat)) )
				{
				(void) sprintf(err_buf,
						"Error in label conversion format: %s  for attribute: %s",
						attribs[nn].conversion_format, attribs[nn].name);
				(void) error_report(err_buf);
				}
			}

		if ( IsNull(identify_unit(attribs[nn].units)) )
			{
			(void) sprintf(err_buf,
					"Unknown attribute units: %s  for attribute: %s",
					attribs[nn].units, attribs[nn].name);
			(void) error_report(err_buf);
			}

		if ( same(attribs[nn].format, FormatSymbol)
				|| same(attribs[nn].format, FormatText) )
			{
			if ( blank(attribs[nn].look_up) )
				{
				(void) sprintf(err_buf, "No look_up for attribute: %s",
						attribs[nn].name);
				(void) error_report(err_buf);
				}
			}

		else if ( same(attribs[nn].format, FormatWindBarb)
				|| same(attribs[nn].format, FormatWindText)
				|| same(attribs[nn].format, FormatWindSymbol) )
			{
			if ( !convert_value(attribs[nn].units, 0.0, MperS, NullDouble) )
				{
				(void) sprintf(err_buf,
						"Incorrect speed units: %s  for wind attribute: %s",
						attribs[nn].units, attribs[nn].name);
				(void) error_report(err_buf);
				}
			}
		}

	/* Error return for incorrect "fit_to_map_ref" ... based on application */
	/*  ... and reset "fit_to_map_ref" if "rotation" is non-zero            */
	switch ( Program.macro )
		{
		case GPG_PSMet:
		case GPG_SVGMet:
		case GPG_CorMet:
			if ( !same(fit_to_map_ref, FitToMapRefNone)
					&& !same(fit_to_map_ref, FitToMapRefUpper)
					&& !same(fit_to_map_ref, FitToMapRefLower)
					&& !same(fit_to_map_ref, FitToMapRefLeft)
					&& !same(fit_to_map_ref, FitToMapRefRight) )
				{
				(void) sprintf(err_buf,
						"Recognized fit_to_map_ref types are: %s %s %s %s %s",
						FitToMapRefNone, FitToMapRefUpper, FitToMapRefLower,
						FitToMapRefLeft, FitToMapRefRight);
				(void) error_report(err_buf);
				}
			if ( !same(fit_to_map_ref, FitToMapRefNone) && (rotation != 0.0
						|| rotate_lat || rotate_lon || !blank(rot_attrib)) )
				{
				(void) sprintf(err_buf,
						"Rotated labels allow only \"fit_to_map_ref = %s\"",
						FitToMapRefNone);
				(void) warn_report(err_buf);
				(void) strcpy(fit_to_map_ref, FitToMapRefNone);
				}
		}

	/* Error if more than one "rotate_..." keyword set */
	if ( (rotate_lat && rotate_lon) || (rotate_lat && !blank(rot_attrib))
			|| (rotate_lon && !blank(rot_attrib)) )
		(void) error_report("Can set only one of \"rotate_to_latitude\" \"rotate_to_longitude\" \"rotation_attribute\"");

	/* Check "x_wrap" "y_wrap" keywords */
	if ( xwrap < 1 )
		(void) error_report("Cannot set \"x_wrap\" less than 1!");
	if ( ywrap < 1 )
		(void) error_report("Cannot set \"y_wrap\" less than 1!");
	if ( xwrap > 1 && ywrap > 1 )
		(void) error_report("Cannot set both \"x_wrap\" and \"y_wrap\" greater than 1!");

	/* Now display the label */
	displayok = GRA_display_label(element, level, geo_file,
				cat_cascade, cat_attrib, num_catatt, attribs, num_attrib,
				list_case, num_list, mark, markscale, tzone, tlang,
				display_name, display_type, fit_to_map, fit_to_map_ref,
				rotation, rotate_lat, rotate_lon, rot_attrib, constrain_rot,
				xsoff, ysoff, xwrap, ywrap, xdoff, ydoff, xoff, yoff);

	/* Free category attribute and special case buffers */
	(void) free_category_attribs();
	(void) free_list_case();

	/* Return results of label display */
	return displayok;
	}

/***********************************************************************
*                                                                      *
*    d o _ l c h a i n _ n o d e s                                     *
*                                                                      *
***********************************************************************/

LOGICAL		do_lchain_nodes

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	LOGICAL			displayok;
	int				ii, nn;
	STRING			next;
	char			stime[GPGTiny], etime[GPGTiny];
	char			tzone[GPGMedium], tlang[GPGMedium];
	char			nspeed_units[GPGMedium], nstnry_label[GPGMedium];
	char			units[GPGMedium], cat_cascade[GPGMedium];
	char			tcat_cascade[GPGMedium];
	char			format[GPGMedium], conversion_format[GPGMedium];
	char			look_up[GPGMedium], mark[GPGMedium];
	char			display_name[GPGMedium], display_type[GPGMedium];
	char			fit_to_map_ref[GPGMedium];
	char			rot_attrib[GPGMedium];
	LOGICAL			rotate_lat, rotate_lon, constrain_rot;
	int				num_times;
	STRING			*times;
	int				num_fields, num_elem, num_levl;
	GRA_FLD_INFO	*lchain_flds;
	int				num_catatt, num_att, num_cat;
	CATATTRIB		*cat_attrib;
	int				num_tcatatt, num_tatt, num_tcat;
	CATATTRIB		*tcat_attrib;
	int				num_attrib, num_list, num_case, num_look;
	LOGICAL			fit_to_map, need_default_attrib;
	ATTRIB_DISPLAY	*temp_attrib, *attribs;
	SPCASE			*list_case;
	int				xwrap, ywrap;
	float			size, xval;
	float			nround, nqsmax;
	float			markscale, symscale, txt_size, wthscale, hgtscale;
	float			rotation, xsoff, ysoff, xdoff, ydoff, xoff, yoff;
	float			xqsoff, yqsoff;
	STRING			key, action;
	char			tbuf[GPGMedium], err_buf[GPGLong];

	/* Set current text size */
	(void) sscanf(CurPres.text_size, "%f", &size);

	/* Initialize all parameters */
	(void) strcpy(stime,                     FpaCblank);
	(void) strcpy(etime,                     FpaCblank);
	(void) strcpy(tzone,                     FpaCblank);
	(void) strcpy(tlang,                     FpaCblank);
	(void) strcpy(nspeed_units,              LnodeSpeedUnitsKnot);
	(void) strcpy(nstnry_label,              FpaCblank);
	(void) strcpy(units,                     FpaCmksUnits);
	(void) strcpy(cat_cascade,               CatCascadeAnd);
	(void) strcpy(tcat_cascade,              CatCascadeAnd);
	(void) strcpy(format,                    FpaCblank);
	(void) strcpy(conversion_format,         FpaCblank);
	(void) strcpy(look_up,                   FpaCblank);
	(void) strcpy(mark,                      FpaCblank);
	(void) strcpy(display_name,              FpaCblank);
	(void) strcpy(display_type,              FpaCblank);
	(void) strcpy(fit_to_map_ref,            FitToMapRefNone);
	(void) strcpy(rot_attrib,                FpaCblank);
	fit_to_map         = FALSE;
	nround             =   0.0;
	nqsmax             =   0.0;
	markscale          = 100.0;
	symscale           = 100.0;
	txt_size           =  size;
	wthscale           = Program.default_width_scale;
	hgtscale           = Program.default_height_scale;
	rotation           =   0.0;
	rotate_lat         = FALSE;
	rotate_lon         = FALSE;
	constrain_rot      =  TRUE;
	xsoff              =   0.0;
	ysoff              =   0.0;
	xwrap              =   1;
	ywrap              =   1;
	xdoff              =   0.0;
	ydoff              =   0.0;
	xoff               =   0.0;
	yoff               =   0.0;
	xqsoff             =   0.0;
	yqsoff             =   0.0;
	num_times          =     0;
	times              = NullStringPtr;
	num_fields         =     0;
	num_elem           =     0;
	num_levl           =     0;
	num_catatt         =     0;
	num_att            =     0;
	num_cat            =     0;
	num_tcatatt        =     0;
	num_tatt           =     0;
	num_tcat           =     0;
	num_list           =     0;
	num_case           =     0;
	num_look           =     0;

	/* Process all keywords in list ... except the attribute keywords */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "element") )
			{
			num_fields = add_graphics_field(&lchain_flds, ++num_elem);
			(void) strcpy(lchain_flds[num_elem-1].element, action);
			}

		else if ( same(key, "element_list") )
			{
			(void) strcpy(tbuf, action);
			while ( !blank( next = string_arg(tbuf) ) )
				{
				num_fields = add_graphics_field(&lchain_flds, ++num_elem);
				(void) strcpy(lchain_flds[num_elem-1].element, next);
				}
			}

		else if ( same(key, "level") )
			{
			num_fields = add_graphics_field(&lchain_flds, ++num_levl);
			(void) strcpy(lchain_flds[num_levl-1].level, action);
			}

		else if ( same(key, "level_list") )
			{
			(void) strcpy(tbuf, action);
			while ( !blank( next = string_arg(tbuf) ) )
				{
				num_fields = add_graphics_field(&lchain_flds, ++num_levl);
				(void) strcpy(lchain_flds[num_levl-1].level, next);
				}
			}

		else if ( same(key, "node_speed_units") )
			{
			(void) strcpy(nspeed_units, action);
			}

		else if ( same(key, "node_speed_round") )
			{
			(void) sscanf(action, "%f", &xval);
			if ( xval > 0.0 ) nround = xval;
			}

		else if ( same(key, "node_stationary_max") )
			{
			(void) sscanf(action, "%f", &xval);
			if ( xval > 0.0 ) nqsmax = xval;
			}

		else if ( same(key, "node_stationary_label") )
			{
			(void) strcpy(nstnry_label, action);
			}

		else if ( same(key, "units") )
			{
			(void) strcpy(units, action);
			}

		else if ( same(key, "start_time") )
			{
			(void) strcpy(stime, action);
			}

		else if ( same(key, "end_time") )
			{
			(void) strcpy(etime, action);
			}

		else if ( same(key, "times") )
			{
			(void) strcpy(tbuf, action);
			while ( !blank( next = string_arg(tbuf) ) )
				{
				num_times++;
				times = GETMEM(times, STRING, num_times);
				times[num_times-1] = safe_strdup(next);
				}
			}

		else if ( same(key, "time_zone") )
			{
			(void) strcpy(tzone, action);
			}

		else if ( same(key, "language") )
			{
			(void) strcpy(tlang, action);
			}

		else if ( same(key, "category_cascade") )
			{
			(void) strcpy(cat_cascade, action);
			}

		else if ( same(key, "category_attribute") )
			{
			num_catatt = add_category_attribs(&cat_attrib, ++num_att);
			(void) strcpy(cat_attrib[num_att-1].category_attribute, action);
			}

		else if ( same(key, "category") )
			{
			num_catatt = add_category_attribs(&cat_attrib, ++num_cat);
			(void) strcpy(cat_attrib[num_cat-1].category, action);
			}

		else if ( same(key, "track_category_cascade") )
			{
			(void) strcpy(tcat_cascade, action);
			}

		else if ( same(key, "track_category_attribute") )
			{
			num_tcatatt = add_track_category_attribs(&tcat_attrib, ++num_tatt);
			(void) strcpy(tcat_attrib[num_tatt-1].category_attribute, action);
			}

		else if ( same(key, "track_category") )
			{
			num_tcatatt = add_track_category_attribs(&tcat_attrib, ++num_tcat);
			(void) strcpy(tcat_attrib[num_tcat-1].category, action);
			}

		else if ( same(key, "format") )
			{
			(void) strcpy(format, string_arg(action));
			if ( !blank(action) )
				{
				(void) no_white(action);
				(void) strcpy(conversion_format, action);
				}
			}

		else if ( same(key, "look_up") )
			{
			(void) strcpy(look_up, action);
			}

		else if ( same(key, "case") )
			{
			num_list = add_list_case(&list_case, ++num_case);
			(void) strcpy(list_case[num_case-1].spcase, action);
			}

		else if ( same(key, "case_look_up") )
			{
			num_list = add_list_case(&list_case, ++num_look);
			(void) strcpy(list_case[num_look-1].lookup, action);
			}

		else if ( same(key, "mark") )
			{
			(void) strcpy(mark, find_symbol_file(action));
			}

		else if ( same(key, "mark_scale") )
			{
			(void) sscanf(action, "%f", &markscale);
			}

		else if ( same(key, "symbol_scale") )
			{
			(void) sscanf(action, "%f", &symscale);
			}

		else if ( same(key, "text_size") )
			{
			(void) sscanf(action, "%f", &txt_size);
			}

		else if ( same(key, "display_name") )
			{
			(void) strcpy(display_name, action);
			}

		else if ( same(key, "display_type") )
			{
			(void) strcpy(display_type, action);
			}

		else if ( same(key, "width_scale") )
			{
			(void) sscanf(action, "%f", &wthscale);
			}

		else if ( same(key, "height_scale") )
			{
			(void) sscanf(action, "%f", &hgtscale);
			}

		else if ( same(key, "fit_to_map") )
			{
			if ( same_ic(action, LogicalYes) ) fit_to_map = TRUE;
			else                               fit_to_map = FALSE;
			}

		else if ( same(key, "fit_to_map_ref") )
			{
			(void) strcpy(fit_to_map_ref, action);
			}

		else if ( same(key, "rotation") )
			{
			(void) sscanf(action, "%f", &rotation);
			}

		else if ( same(key, "rotate_to_latitude") )
			{
			if ( same_ic(action, LogicalYes) ) rotate_lat = TRUE;
			else                               rotate_lat = FALSE;
			}

		else if ( same(key, "rotate_to_longitude") )
			{
			if ( same_ic(action, LogicalYes) ) rotate_lon = TRUE;
			else                               rotate_lon = FALSE;
			}

		else if ( same(key, "rotation_attribute") )
			{
			(void) strcpy(rot_attrib, action);
			}

		else if ( same(key, "constrain_rotation") )
			{
			if ( same_ic(action, LogicalYes) ) constrain_rot = TRUE;
			else                               constrain_rot = FALSE;
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

		else if ( same(key, "x_display_off") )
			{
			(void) sscanf(action, "%f", &xdoff);
			}

		else if ( same(key, "y_display_off") )
			{
			(void) sscanf(action, "%f", &ydoff);
			}

		else if ( same(key, "x_off") )
			{
			(void) sscanf(action, "%f", &xoff);
			}

		else if ( same(key, "y_off") )
			{
			(void) sscanf(action, "%f", &yoff);
			}

		else if ( same(key, "x_stationary") )
			{
			(void) sscanf(action, "%f", &xqsoff);
			}

		else if ( same(key, "y_stationary") )
			{
			(void) sscanf(action, "%f", &yqsoff);
			}

		/* Skip valid attribute keywords (for the moment) */
		else if ( same(key,  "attribute")
				|| same(key, "attribute_show")
				|| same(key, "attribute_anchor")
				|| same(key, "attribute_ref")
				|| same(key, "attribute_justification")
				|| same(key, "attribute_vertical_just")
				|| same(key, "attribute_units")
				|| same(key, "attribute_format")
				|| same(key, "attribute_look_up")
				|| same(key, "attribute_symbol_scale")
				|| same(key, "attribute_text_size")
				|| same(key, "attribute_display_name")
				|| same(key, "attribute_display_type")
				|| same(key, "attribute_width_scale")
				|| same(key, "attribute_height_scale")
				|| same(key, "attribute_x_off")
				|| same(key, "attribute_y_off")
				|| same(key, "attribute_line_width")
				|| same(key, "attribute_line_style")
				|| same(key, "attribute_outline")
				|| same(key, "attribute_fill")
				|| same(key, "attribute_outline_first")
				|| same(key, "attribute_font")
				|| same(key, "attribute_font_weight")
				|| same(key, "attribute_italics")
				|| same(key, "attribute_char_space")
				|| same(key, "attribute_word_space") )
			{
			}

		/* Skip valid presentation keywords already processed */
		else if ( same(key,  "line_width")
				|| same(key, "line_style")
				|| same(key, "outline")
				|| same(key, "fill")
				|| same(key, "outline_first")
				|| same(key, "font")
				|| same(key, "font_weight")
				|| same(key, "italics")
				|| same(key, "justification")
				|| same(key, "char_space")
				|| same(key, "word_space") )
			{
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Initialize storage for attribute display list */
	(void) initialize_attribute_display();

	/* Now process all attribute keywords */
	need_default_attrib = TRUE;
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		/* Add another attribute to the list ... with default parameters */
		if ( same(key, "attribute") )
			{
			temp_attrib = add_attribute_display(action, units,
							format, conversion_format, look_up, symscale,
							txt_size, wthscale, hgtscale, &CurPres);
			need_default_attrib = FALSE;
			}

		/* Add the default attribute to the list ... if necessary */
		else if ( same_start(key, "attribute_") && need_default_attrib )
				{
				temp_attrib = add_attribute_display(AttribAutolabel, units,
								format, conversion_format, look_up, symscale,
								txt_size, wthscale, hgtscale, &CurPres);
				}

		/* Now add the attribute parameters */
		if ( same(key, "attribute") )
			{
			}

		else if ( same(key, "attribute_show") )
			{
			if ( same_ic(action, "no") ) temp_attrib->show_label = FALSE;
			else                         temp_attrib->show_label = TRUE;
			}

		/* Adjust attribute parameters for this attribute */
		else if ( same(key, "attribute_anchor") )
			{
			(void) strcpy(temp_attrib->anchor, action);
			}

		else if ( same(key, "attribute_ref") )
			{
			(void) strcpy(temp_attrib->ref, action);
			}

		else if ( same(key, "attribute_justification") )
			{
			(void) strcpy(temp_attrib->presentation.justified, action);
			}

		else if ( same(key, "attribute_vertical_just") )
			{
			(void) strcpy(temp_attrib->vertical_just, action);
			}

		else if ( same(key, "attribute_units") )
			{
			(void) strcpy(temp_attrib->units, action);
			}

		else if ( same(key, "attribute_format") )
			{
			(void) strcpy(temp_attrib->format, string_arg(action));
			if ( !blank(action) )
				{
				(void) no_white(action);
				(void) strcpy(temp_attrib->conversion_format, action);
				}
			}

		else if ( same(key, "attribute_look_up") )
			{
			(void) strcpy(temp_attrib->look_up, action);
			}

		else if ( same(key, "attribute_symbol_scale") )
			{
			(void) sscanf(action, "%f", &(temp_attrib->symbol_scale));
			}

		else if ( same(key, "attribute_text_size") )
			{
			(void) sscanf(action, "%f", &(temp_attrib->txt_size));
			}

		else if ( same(key, "attribute_display_name") )
			{
			(void) strcpy(temp_attrib->display_name, action);
			}

		else if ( same(key, "attribute_display_type") )
			{
			(void) strcpy(temp_attrib->display_type, action);
			}

		else if ( same(key, "attribute_width_scale") )
			{
			(void) sscanf(action, "%f", &(temp_attrib->width_scale));
			}

		else if ( same(key, "attribute_height_scale") )
			{
			(void) sscanf(action, "%f", &(temp_attrib->height_scale));
			}

		else if ( same(key, "attribute_x_off") )
			{
			(void) sscanf(action, "%f", &(temp_attrib->x_off));
			}

		else if ( same(key, "attribute_y_off") )
			{
			(void) sscanf(action, "%f", &(temp_attrib->y_off));
			}

		else if ( same(key, "attribute_line_width") )
			{
			(void) strcpy(temp_attrib->presentation.line_width, action);
			}

		else if ( same(key, "attribute_line_style") )
			{
			(void) strcpy(temp_attrib->presentation.line_style, action);
			}

		else if ( same(key, "attribute_outline") )
			{
			(void) strcpy(temp_attrib->presentation.outline, action);
			}

		else if ( same(key, "attribute_fill") )
			{
			(void) strcpy(temp_attrib->presentation.fill, action);
			}

		else if ( same(key, "attribute_outline_first") )
			{
			if ( same_ic(action, LogicalYes) )
				{
				temp_attrib->presentation.outline_first = TRUE;
				}
			else
				{
				temp_attrib->presentation.outline_first = FALSE;
				}
			}

		else if ( same(key, "attribute_font") )
			{
			(void) strcpy(temp_attrib->presentation.font, action);
			}

		else if ( same(key, "attribute_font_weight") )
			{
			(void) strcpy(temp_attrib->presentation.font_weight, action);
			}

		else if ( same(key, "attribute_italics") )
			{
			(void) strcpy(temp_attrib->presentation.italics, action);
			}

		else if ( same(key, "attribute_char_space") )
			{
			(void) strcpy(temp_attrib->presentation.char_space, action);
			}

		else if ( same(key, "attribute_word_space") )
			{
			(void) strcpy(temp_attrib->presentation.word_space, action);
			}
		}

	/* Ensure that at least one set of category parameters has been set */
	/*  ... or initialize one with some default values!                 */
	if ( num_catatt < 1 )
		{
		num_catatt = add_category_attribs(&cat_attrib, ++num_att);
		}
	if ( num_tcatatt < 1 )
		{
		num_tcatatt = add_track_category_attribs(&tcat_attrib, ++num_tatt);
		}

	/* Error return for missing parameters */
	if ( num_fields <= 0 )
		(void) error_report("No element - level identifiers");

	/* Check "node_speed_units" keyword */
	if ( IsNull(identify_unit(nspeed_units)) )
		{
		(void) sprintf(err_buf, "Unknown node speed units: %s", nspeed_units);
		(void) error_report(err_buf);
		}
	if ( !convert_value(nspeed_units, 0.0, LnodeSpeedUnitsKnot, NullDouble) )
		{
		(void) sprintf(err_buf,
				"Incorrect node speed units: %s  (must convert to: %s)",
				nspeed_units, LnodeSpeedUnitsKnot);
		(void) error_report(err_buf);
		}

	/* Check "format" "conversion_format" "units" "look_up" */
	/*  keywords for attributes (based on label format)     */
	num_attrib = return_attribute_display(&attribs);
	for ( nn=0; nn<num_attrib; nn++ )
		{
		if ( blank(attribs[nn].format) )
			{
			(void) sprintf(err_buf, "No label format for attribute: %s",
					attribs[nn].name);
			(void) error_report(err_buf);
			}

		/* Error return for incorrect "format" ... based on application */
		switch ( Program.macro )
			{
			case GPG_PSMet:
			case GPG_SVGMet:
			case GPG_CorMet:
				if ( !same(attribs[nn].format, FormatDirect)
						&& !same(attribs[nn].format, FormatSymbol)
						&& !same(attribs[nn].format, FormatText)
						&& !same(attribs[nn].format, FormatWindBarb)
						&& !same(attribs[nn].format, FormatWindText)
						&& !same(attribs[nn].format, FormatWindSymbol)
						&& !same(attribs[nn].format, FormatNone) )
					{
					(void) sprintf(err_buf,
							"Recognized format types are: %s %s %s %s %s %s %s",
							FormatDirect, FormatSymbol, FormatText,
							FormatWindBarb, FormatWindText, FormatWindSymbol,
							FormatNone);
					(void) error_report(err_buf);
					}
				break;

			case GPG_TexMet:
				if ( !same(attribs[nn].format, FormatDirect)
						&& !same(attribs[nn].format, FormatText)
						&& !same(attribs[nn].format, FormatWindText) )
					{
					(void) sprintf(err_buf,
							"Recognized format types are: %s %s %s",
							FormatDirect, FormatText, FormatWindText);
					(void) error_report(err_buf);
					}
				break;
			}

		if ( !blank(attribs[nn].conversion_format) )
			{
			if ( IsNull(strchr(attribs[nn].conversion_format, ConFormat)) )
				{
				(void) sprintf(err_buf,
						"Error in label conversion format: %s  for attribute: %s",
						attribs[nn].conversion_format, attribs[nn].name);
				(void) error_report(err_buf);
				}
			}

		if ( IsNull(identify_unit(attribs[nn].units)) )
			{
			(void) sprintf(err_buf,
					"Unknown attribute units: %s  for attribute: %s",
					attribs[nn].units, attribs[nn].name);
			(void) error_report(err_buf);
			}

		if ( same(attribs[nn].format, FormatSymbol)
				|| same(attribs[nn].format, FormatText) )
			{
			if ( blank(attribs[nn].look_up) )
				{
				(void) sprintf(err_buf, "No look_up for attribute: %s",
						attribs[nn].name);
				(void) error_report(err_buf);
				}
			}

		else if ( same(attribs[nn].format, FormatWindBarb)
				|| same(attribs[nn].format, FormatWindText)
				|| same(attribs[nn].format, FormatWindSymbol) )
			{
			if ( !convert_value(attribs[nn].units, 0.0, MperS, NullDouble) )
				{
				(void) sprintf(err_buf,
						"Incorrect speed units: %s  for wind attribute: %s",
						attribs[nn].units, attribs[nn].name);
				(void) error_report(err_buf);
				}
			}
		}

	/* Error return for incorrect "fit_to_map_ref" ... based on application */
	/*  ... and reset "fit_to_map_ref" if "rotation" is non-zero            */
	switch ( Program.macro )
		{
		case GPG_PSMet:
		case GPG_SVGMet:
		case GPG_CorMet:
			if ( !same(fit_to_map_ref, FitToMapRefNone)
					&& !same(fit_to_map_ref, FitToMapRefUpper)
					&& !same(fit_to_map_ref, FitToMapRefLower)
					&& !same(fit_to_map_ref, FitToMapRefLeft)
					&& !same(fit_to_map_ref, FitToMapRefRight) )
				{
				(void) sprintf(err_buf,
						"Recognized fit_to_map_ref types are: %s %s %s %s %s",
						FitToMapRefNone, FitToMapRefUpper, FitToMapRefLower,
						FitToMapRefLeft, FitToMapRefRight);
				(void) error_report(err_buf);
				}
			if ( !same(fit_to_map_ref, FitToMapRefNone) && (rotation != 0.0
						|| rotate_lat || rotate_lon || !blank(rot_attrib)) )
				{
				(void) sprintf(err_buf,
						"Rotated labels allow only \"fit_to_map_ref = %s\"",
						FitToMapRefNone);
				(void) warn_report(err_buf);
				(void) strcpy(fit_to_map_ref, FitToMapRefNone);
				}
		}

	/* Error if more than one "rotate_..." keyword set */
	if ( (rotate_lat && rotate_lon) || (rotate_lat && !blank(rot_attrib))
			|| (rotate_lon && !blank(rot_attrib)) )
		(void) error_report("Can set only one of \"rotate_to_latitude\" \"rotate_to_longitude\" \"rotation_attribute\"");

	/* Check "x_wrap" "y_wrap" keywords */
	if ( xwrap < 1 )
		(void) error_report("Cannot set \"x_wrap\" less than 1!");
	if ( ywrap < 1 )
		(void) error_report("Cannot set \"y_wrap\" less than 1!");
	if ( xwrap > 1 && ywrap > 1 )
		(void) error_report("Cannot set both \"x_wrap\" and \"y_wrap\" greater than 1!");

	/* Now display all the link chain nodes */
	displayok = GRA_display_all_lchain_nodes(lchain_flds, num_fields,
				stime, etime, times, num_times, tzone, tlang,
				nspeed_units, nround, nqsmax, nstnry_label, units,
				cat_cascade, cat_attrib, num_catatt,
				tcat_cascade, tcat_attrib, num_tcatatt,
				attribs, num_attrib, list_case, num_list, mark, markscale,
				display_name, display_type, fit_to_map, fit_to_map_ref,
				rotation, rotate_lat, rotate_lon, rot_attrib, constrain_rot,
				xsoff, ysoff, xwrap, ywrap, xdoff, ydoff, xoff, yoff,
				xqsoff, yqsoff);

	/* Free field, category attribute, and special case buffers */
	(void) free_graphics_field();
	(void) free_category_attribs();
	(void) free_track_category_attribs();
	(void) free_list_case();

	/* Free space used by node times */
	for ( nn=0; nn<num_times; nn++ )
		{
		FREEMEM(times[nn]);
		}
	FREEMEM(times);

	/* Return results of link chain node display */
	return displayok;
	}

/***********************************************************************
*                                                                      *
*    d o _ l c h a i n _ t r a c k s                                   *
*                                                                      *
***********************************************************************/

LOGICAL		do_lchain_tracks

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	LOGICAL			displayok;
	int				ii;
	STRING			next;
	char			stime[GPGTiny], etime[GPGTiny];
	char			tlen_units[GPGMedium], cat_cascade[GPGMedium];
	char			attribute[GPGMedium], look_up[GPGMedium];
	char			arrow_name[GPGMedium], pattern[GPGMedium];
	char			pattern_width[GPGMedium], pattern_length[GPGMedium];
	int				num_fields, num_elem, num_levl;
	GRA_FLD_INFO	*lchain_flds;
	int				num_catatt, num_att, num_cat;
	CATATTRIB		*cat_attrib;
	int				num_comp, num_width, num_style, num_outline, num_fill;
	COMP_PRES		*comp_pres;
	float			xval, tlmin;
	STRING			key, action;
	char			tbuf[GPGMedium], err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(stime,          FpaCblank);
	(void) strcpy(etime,          FpaCblank);
	(void) strcpy(tlen_units,     LineLengthUnitsKm);
	(void) strcpy(cat_cascade,    CatCascadeAnd);
	(void) strcpy(attribute,      AttribCategory);
	(void) strcpy(look_up,        FpaCblank);
	(void) strcpy(arrow_name,     FpaCblank);
	(void) strcpy(pattern,        FpaCblank);
	(void) strcpy(pattern_width,  FpaCblank);
	(void) strcpy(pattern_length, FpaCblank);
	tlmin       = 0.0;
	num_fields  = 0;
	num_elem    = 0;
	num_levl    = 0;
	num_catatt  = 0;
	num_att     = 0;
	num_cat     = 0;
	num_comp    = 0;
	num_width   = 0;
	num_style   = 0;
	num_outline = 0;
	num_fill    = 0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "element") )
			{
			num_fields = add_graphics_field(&lchain_flds, ++num_elem);
			(void) strcpy(lchain_flds[num_elem-1].element, action);
			}

		else if ( same(key, "element_list") )
			{
			(void) strcpy(tbuf, action);
			while ( !blank( next = string_arg(tbuf) ) )
				{
				num_fields = add_graphics_field(&lchain_flds, ++num_elem);
				(void) strcpy(lchain_flds[num_elem-1].element, next);
				}
			}

		else if ( same(key, "level") )
			{
			num_fields = add_graphics_field(&lchain_flds, ++num_levl);
			(void) strcpy(lchain_flds[num_levl-1].level, action);
			}

		else if ( same(key, "level_list") )
			{
			(void) strcpy(tbuf, action);
			while ( !blank( next = string_arg(tbuf) ) )
				{
				num_fields = add_graphics_field(&lchain_flds, ++num_levl);
				(void) strcpy(lchain_flds[num_levl-1].level, next);
				}
			}

		else if ( same(key, "start_time") )
			{
			(void) strcpy(stime, action);
			}

		else if ( same(key, "end_time") )
			{
			(void) strcpy(etime, action);
			}

		else if ( same(key, "track_length_min") )
			{
			(void) sscanf(action, "%f", &xval);
			if ( xval > 0.0 ) tlmin = xval;
			}

		else if ( same(key, "track_length_units") )
			{
			(void) strcpy(tlen_units, action);
			}

		else if ( same(key, "category_cascade") )
			{
			(void) strcpy(cat_cascade, action);
			}

		else if ( same(key, "category_attribute") )
			{
			num_catatt = add_category_attribs(&cat_attrib, ++num_att);
			(void) strcpy(cat_attrib[num_att-1].category_attribute, action);
			}

		else if ( same(key, "category") )
			{
			num_catatt = add_category_attribs(&cat_attrib, ++num_cat);
			(void) strcpy(cat_attrib[num_cat-1].category, action);
			}

		else if ( same(key, "attribute") )
			{
			(void) strcpy(attribute, action);
			}

		else if ( same(key, "look_up") )
			{
			(void) strcpy(look_up, action);
			}

		else if ( same(key, "arrow_name") )
			{
			(void) strcpy(arrow_name, action);
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

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Ensure that at least one set of category parameters has been set */
	/*  ... or initialize one with some default values!                 */
	if ( num_catatt < 1 )
		{
		num_catatt = add_category_attribs(&cat_attrib, ++num_att);
		}

	/* Ensure that local component presentation is filled in */
	(void) check_comp_pres();

	/* Error return for missing parameters */
	if ( num_fields <= 0 )
		(void) error_report("No element - level identifiers");

	/* Check "track_length_units" keyword */
	if ( IsNull(identify_unit(tlen_units)) )
		{
		(void) sprintf(err_buf, "Unknown track length units: %s", tlen_units);
		(void) error_report(err_buf);
		}
	if ( !convert_value(tlen_units, 0.0, LineLengthUnitsKm, NullDouble) )
		{
		(void) sprintf(err_buf,
				"Incorrect track length units: %s  (must convert to: %s)",
				tlen_units, LineLengthUnitsKm);
		(void) error_report(err_buf);
		}

	/* Now display all the link chain tracks */
	displayok = GRA_display_all_lchain_tracks(lchain_flds, num_fields,
				stime, etime, tlmin, tlen_units, cat_cascade,
				cat_attrib, num_catatt, attribute, look_up, arrow_name,
				pattern, pattern_width, pattern_length, comp_pres, num_comp);

	/* Free field, category attribute, local presentation buffers */
	(void) free_graphics_field();
	(void) free_category_attribs();
	(void) free_comp_pres();

	/* Return results of link chain track display */
	return displayok;
	}

/***********************************************************************
*                                                                      *
*    d o _ l e g e n d                                                 *
*                                                                      *
***********************************************************************/

LOGICAL		do_legend

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	char		symbol[GPGMedium], string[GPGMedium];
	float		txt_size, scale, rotation, xoff, yoff;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Set current text size */
	(void) sscanf(CurPres.text_size, "%f", &txt_size);

	/* Initialize all parameters */
	(void) strcpy(symbol, FpaCblank);
	(void) strcpy(string, FpaCblank);
	scale    = 100.0;
	rotation =   0.0;
	xoff     =   0.0;
	yoff     =   0.0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "symbol") )
			{
			(void) strcpy(symbol, find_symbol_file(action));
			}

		else if ( same(key, "string") )
			{
			(void) strcpy(string, action);
			}

		else if ( same(key, "scale") )
			{
			(void) sscanf(action, "%f", &scale);
			}

		else if ( same(key, "text_size") )
			{
			(void) sscanf(action, "%f", &txt_size);
			}

		else if ( same(key, "rotation") )
			{
			(void) sscanf(action, "%f", &rotation);
			}

		else if ( same(key, "x_off") )
			{
			(void) sscanf(action, "%f", &xoff);
			}

		else if ( same(key, "y_off") )
			{
			(void) sscanf(action, "%f", &yoff);
			}

		/* Skip valid presentation keywords already processed */
		else if ( same(key,  "line_width")
				|| same(key, "line_style")
				|| same(key, "outline")
				|| same(key, "fill")
				|| same(key, "outline_first")
				|| same(key, "font")
				|| same(key, "font_weight")
				|| same(key, "italics")
				|| same(key, "justification")
				|| same(key, "char_space")
				|| same(key, "word_space") )
			{
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Error return for missing parameter */
	if ( blank(symbol) && blank(string) )
		(void) error_report("No symbol file or text string");

	/* Now add the legend to the list */
	return add_legend(symbol, string, scale, txt_size, rotation, xoff, yoff,
																	&CurPres);
	}

/***********************************************************************
*                                                                      *
*    d o _ l i n e s                                                   *
*                                                                      *
***********************************************************************/

LOGICAL		do_lines

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	LOGICAL			displayok;
	int				ii;
	STRING			next;
	char			cat_cascade[GPGMedium];
	char			attribute[GPGMedium], look_up[GPGMedium];
	char			arrow_name[GPGMedium], pattern[GPGMedium];
	char			pattern_width[GPGMedium], pattern_length[GPGMedium];
	int				num_fields, num_elem, num_levl;
	GRA_FLD_INFO	*line_flds;
	int				num_catatt, num_att, num_cat;
	CATATTRIB		*cat_attrib;
	int				num_comp, num_width, num_style, num_outline, num_fill;
	COMP_PRES		*comp_pres;
	STRING			key, action;
	char			tbuf[GPGMedium], err_buf[GPGLong];

	/* Initialize all parameters */
	(void) strcpy(cat_cascade,    CatCascadeAnd);
	(void) strcpy(attribute,      AttribCategory);
	(void) strcpy(look_up,        FpaCblank);
	(void) strcpy(arrow_name,     FpaCblank);
	(void) strcpy(pattern,        FpaCblank);
	(void) strcpy(pattern_width,  FpaCblank);
	(void) strcpy(pattern_length, FpaCblank);
	num_fields  = 0;
	num_elem    = 0;
	num_levl    = 0;
	num_catatt  = 0;
	num_att     = 0;
	num_cat     = 0;
	num_comp    = 0;
	num_width   = 0;
	num_style   = 0;
	num_outline = 0;
	num_fill    = 0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "element") )
			{

			/* >>> multiple values should use "element_list" below <<< */
			/* >>> this is now an ObsoleteVersion feature          <<< */
			(void) strcpy(tbuf, action);
			next = string_arg(tbuf);
			if ( !blank(tbuf) )
				{
				(void) fprintf(stderr, ">>> Obsolete usage of keyword in");
				(void) fprintf(stderr, " directive ... \"@lines\" <<<\n");
				(void) fprintf(stderr, ">>>   Replace \"element\"");
				(void) fprintf(stderr, " by \"element_list\"");
				(void) fprintf(stderr, " for multiple values <<<\n");
				}

			num_fields = add_graphics_field(&line_flds, ++num_elem);
			(void) strcpy(line_flds[num_elem-1].element, next);
			}

		else if ( same(key, "element_list") )
			{
			(void) strcpy(tbuf, action);
			while ( !blank( next = string_arg(tbuf) ) )
				{
				num_fields = add_graphics_field(&line_flds, ++num_elem);
				(void) strcpy(line_flds[num_elem-1].element, next);
				}
			}

		else if ( same(key, "level") )
			{
			num_fields = add_graphics_field(&line_flds, ++num_levl);
			(void) strcpy(line_flds[num_levl-1].level, action);
			}

		else if ( same(key, "level_list") )
			{
			(void) strcpy(tbuf, action);
			while ( !blank( next = string_arg(tbuf) ) )
				{
				num_fields = add_graphics_field(&line_flds, ++num_levl);
				(void) strcpy(line_flds[num_levl-1].level, next);
				}
			}

		else if ( same(key, "category_cascade") )
			{
			(void) strcpy(cat_cascade, action);
			}

		else if ( same(key, "category_attribute") )
			{
			num_catatt = add_category_attribs(&cat_attrib, ++num_att);
			(void) strcpy(cat_attrib[num_att-1].category_attribute, action);
			}

		else if ( same(key, "category") )
			{
			num_catatt = add_category_attribs(&cat_attrib, ++num_cat);
			(void) strcpy(cat_attrib[num_cat-1].category, action);
			}

		/* >>> this gets replaced by "category" above <<< */
		else if ( ObsoleteVersion && same(key, "subelement") )
			{
			(void) fprintf(stderr, ">>> Obsolete keyword in directive");
			(void) fprintf(stderr, " ... \"@lines\" <<<\n");
			(void) fprintf(stderr, ">>>   Replace \"subelement\"");
			(void) fprintf(stderr, " by \"category\" <<<\n");
			num_catatt = add_category_attribs(&cat_attrib, ++num_cat);
			(void) strcpy(cat_attrib[num_cat-1].category, action);
			}

		else if ( same(key, "attribute") )
			{
			(void) strcpy(attribute, action);
			}

		else if ( same(key, "look_up") )
			{
			(void) strcpy(look_up, action);
			}

		else if ( same(key, "arrow_name") )
			{
			(void) strcpy(arrow_name, action);
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

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Ensure that at least one set of category parameters has been set */
	/*  ... or initialize one with some default values!                 */
	if ( num_catatt < 1 )
		{
		num_catatt = add_category_attribs(&cat_attrib, ++num_att);
		}

	/* Ensure that local component presentation is filled in */
	(void) check_comp_pres();

	/* Error return for missing parameters */
	if ( num_fields <= 0 )
		(void) error_report("No element - level identifiers");

	/* Now display all the lines */
	displayok = GRA_display_all_lines(line_flds, num_fields,
				cat_cascade, cat_attrib, num_catatt, attribute, look_up,
				arrow_name, pattern, pattern_width, pattern_length,
				comp_pres, num_comp);

	/* Free field, category attribute, local presentation buffers */
	(void) free_graphics_field();
	(void) free_category_attribs();
	(void) free_comp_pres();

	/* Return results of line display */
	return displayok;
	}

/***********************************************************************
*                                                                      *
*    d o _ p l o t                                                     *
*                                                                      *
***********************************************************************/

LOGICAL		do_plot

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	char		element[GPGMedium], level[GPGMedium], type[GPGMedium];
	char		mark[GPGMedium];
	float		size, markscale, txt_size;
	float		rotation, xdoff, ydoff, xval, yval;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Set current text size */
	(void) sscanf(CurPres.text_size, "%f", &size);

	/* Initialize all parameters */
	(void) strcpy(element, FpaCblank);
	(void) strcpy(level,   FpaCblank);
	(void) strcpy(type,    FpaCblank);
	(void) strcpy(mark,    FpaCblank);
	markscale = 100.0;
	txt_size  =  size;
	rotation  =   0.0;
	xdoff     =   0.0;
	ydoff     =   0.0;
	xval      =   0.0;
	yval      =   0.0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "element") )
			{
			(void) strcpy(element, action);
			}

		else if ( same(key, "level") )
			{
			(void) strcpy(level, action);
			}

		else if ( same(key, "type") )
			{
			(void) strcpy(type, action);
			}

		else if ( same(key, "mark") )
			{
			(void) strcpy(mark, find_symbol_file(action));
			}

		else if ( same(key, "mark_scale") )
			{
			(void) sscanf(action, "%f", &markscale);
			}

		else if ( same(key, "text_size") )
			{
			(void) sscanf(action, "%f", &txt_size);
			}

		else if ( same(key, "rotation") )
			{
			(void) sscanf(action, "%f", &rotation);
			}

		else if ( same(key, "x_display_off") )
			{
			(void) sscanf(action, "%f", &xdoff);
			}

		else if ( same(key, "y_display_off") )
			{
			(void) sscanf(action, "%f", &ydoff);
			}

		else if ( same(key, "x_val") )
			{
			(void) sscanf(action, "%f", &xval);
			}

		else if ( same(key, "y_val") )
			{
			(void) sscanf(action, "%f", &yval);
			}

		/* Skip valid presentation keywords already processed */
		else if ( same(key,  "line_width")
				|| same(key, "line_style")
				|| same(key, "outline")
				|| same(key, "fill")
				|| same(key, "outline_first")
				|| same(key, "font")
				|| same(key, "font_weight")
				|| same(key, "italics")
				|| same(key, "justification")
				|| same(key, "char_space")
				|| same(key, "word_space") )
			{
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Error return for missing parameters */
	if ( blank(element) ) (void) error_report("No element");
	if ( blank(level) )   (void) error_report("No level");
	if ( blank(type) )    (void) error_report("No type");

	/* Now output the plot objects */
	return GRA_display_plot(element, level, type, mark, markscale,
			txt_size, rotation, xdoff, ydoff, xval, yval);
	}

/***********************************************************************
*                                                                      *
*    d o _ s a m p l e _ f i e l d                                     *
*                                                                      *
***********************************************************************/

LOGICAL		do_sample_field

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	LOGICAL			sampleok;
	int				ii, nn;
	STRING			next;
	float			fval;
	double			dval;
	LOGICAL			status;
	char			element[GPGMedium], level[GPGMedium];
	char			equation[GPGMedium], units[GPGMedium];
	char			field_type[GPGMedium];
	char			geo_file[GPGMedium];
	char			data_file[GPGMedium], data_file_format[GPGMedium];
	char			data_file_units[GPGMedium], data_file_wind_units[GPGMedium];
	char			cat_cascade[GPGMedium];
	char			tzone[GPGMedium], tlang[GPGMedium];
	char			format[GPGMedium], conversion_format[GPGMedium];
	char			look_up[GPGMedium], mark[GPGMedium];
	char			display_name[GPGMedium], display_type[GPGMedium];
	char			fit_to_map_ref[GPGMedium];
	char			rot_attrib[GPGMedium];
	LOGICAL			rotate_lat, rotate_lon, constrain_rot;
	int				num_catatt, num_att, num_cat;
	CATATTRIB		*cat_attrib;
	int				num_attrib, num_list, num_case, num_look;
	LOGICAL			fit_to_map, display_at_feature, need_default_attrib;
	ATTRIB_DISPLAY	*temp_attrib, *attribs;
	SPCASE			*list_case;
	float			size, markscale, symscale, txt_size, wthscale, hgtscale;
	float			rotation, map_units, proximity;
	float			xdoff, ydoff, xoff, yoff, clon;
	double			dprox;
	char			lat[GPGMedium], lon[GPGMedium];
	char			map_x[GPGMedium], map_y[GPGMedium];
	char			location_ident[GPGMedium], location_look_up[GPGMedium];
	char			proximity_units[GPGMedium], table_name[GPGMedium];
	char			grid_name[GPGMedium], list_name[GPGMedium];
	char			source[GPGMedium], vtime[GPGTiny];
	char			cross_section_name[GPGMedium];
	char			location_units[GPGMedium], vertical_look_up[GPGMedium];
	char			vertical_element[GPGMedium], vertical_level[GPGMedium];
	char			vertical_equation[GPGMedium], vertical_units[GPGMedium];
	char			vertical_field_type[GPGMedium];
	char			vertical_data_file[GPGMedium];
	char			vertical_data_file_format[GPGMedium];
	char			vertical_data_file_units[GPGMedium];
	char			vertical_attribute[GPGMedium];
	char			vertical_attribute_upper[GPGMedium];
	char			vertical_attribute_lower[GPGMedium];
	int				num_xloc, ltype;
	XSECT_LOCATION	*xsect_locs;
	STRING			key, action;
	char			tbuf[GPGMedium], err_buf[GPGLong];

	FpaConfigSourceStruct		*sdef;

	/* Set current text size */
	(void) sscanf(CurPres.text_size, "%f", &size);

	/* Initialize all parameters */
	(void) strcpy(element,                   FpaCblank);
	(void) strcpy(level,                     FpaCblank);
	(void) strcpy(equation,                  FpaCblank);
	(void) strcpy(units,                     FpaCmksUnits);
	(void) strcpy(field_type,                FpaCblank);
	(void) strcpy(geo_file,                  FpaCblank);
	(void) strcpy(data_file,                 FpaCblank);
	(void) strcpy(data_file_format,          FpaCblank);
	(void) strcpy(data_file_units,           FpaCblank);
	(void) strcpy(data_file_wind_units,      FpaCblank);
	(void) strcpy(cat_cascade,               CatCascadeAnd);
	(void) strcpy(tzone,                     FpaCblank);
	(void) strcpy(tlang,                     FpaCblank);
	(void) strcpy(format,                    FpaCblank);
	(void) strcpy(conversion_format,         FpaCblank);
	(void) strcpy(look_up,                   FpaCblank);
	(void) strcpy(mark,                      FpaCblank);
	(void) strcpy(display_name,              FpaCblank);
	(void) strcpy(display_type,              FpaCblank);
	(void) strcpy(fit_to_map_ref,            FitToMapRefNone);
	(void) strcpy(rot_attrib,                FpaCblank);
	(void) strcpy(lat,                       FpaCblank);
	(void) strcpy(lon,                       FpaCblank);
	(void) strcpy(map_x,                     FpaCblank);
	(void) strcpy(map_y,                     FpaCblank);
	(void) strcpy(location_ident,            FpaCblank);
	(void) strcpy(location_look_up,          FpaCblank);
	(void) strcpy(proximity_units,           ProximityUnitsKm);
	(void) strcpy(table_name,                FpaCblank);
	(void) strcpy(grid_name,                 FpaCblank);
	(void) strcpy(list_name,                 FpaCblank);
	(void) strcpy(source,                    CurSource);
	(void) strcpy(vtime,                     TVstamp);
	(void) strcpy(cross_section_name,        FpaCblank);
	(void) strcpy(vertical_look_up,          FpaCblank);
	(void) strcpy(vertical_element,          FpaCblank);
	(void) strcpy(vertical_level,            FpaCblank);
	(void) strcpy(vertical_equation,         FpaCblank);
	(void) strcpy(vertical_units,            FpaCmksUnits);
	(void) strcpy(vertical_field_type,       FpaCblank);
	(void) strcpy(vertical_data_file,        FpaCblank);
	(void) strcpy(vertical_data_file_format, FpaCblank);
	(void) strcpy(vertical_data_file_units,  FpaCblank);
	(void) strcpy(vertical_attribute,        FpaCblank);
	(void) strcpy(vertical_attribute_upper,  FpaCblank);
	(void) strcpy(vertical_attribute_lower,  FpaCblank);
	fit_to_map         = FALSE;
	display_at_feature = TRUE;
	markscale          = 100.0;
	symscale           = 100.0;
	txt_size           =  size;
	wthscale           = Program.default_width_scale;
	hgtscale           = Program.default_height_scale;
	rotation           =   0.0;
	rotate_lat         = FALSE;
	rotate_lon         = FALSE;
	constrain_rot      =  TRUE;
	map_units          = BaseMap.definition.units;
	proximity          =   0.0;
	xdoff              =   0.0;
	ydoff              =   0.0;
	xoff               =   0.0;
	yoff               =   0.0;
	num_xloc           =     0;
	num_catatt         =     0;
	num_att            =     0;
	num_cat            =     0;
	num_list           =     0;
	num_case           =     0;
	num_look           =     0;
	ltype              = GPG_LocNone;

	/* Process all keywords in list ... except the attribute keywords */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "element") )
			{
			(void) strcpy(element, action);
			}

		else if ( same(key, "level") )
			{
			(void) strcpy(level, action);
			}

		else if ( same(key, "equation") )
			{
			(void) strcpy(equation, action);
			if ( blank(element) ) (void) strcpy(element, FpaCanyElement);
			}

		else if ( same(key, "units") )
			{
			(void) strcpy(units, action);
			}

		else if ( same(key, "field_type") )
			{
			(void) strcpy(field_type, action);
			}

		else if ( same(key, "geo_file") )
			{
			(void) strcpy(geo_file, action);
			}

		else if ( same(key, "data_file") )
			{
			(void) strcpy(data_file, action);
			if ( blank(element) ) (void) strcpy(element, FpaCanyElement);
			if ( blank(level) )   (void) strcpy(level,   FpaCanyLevel);
			}

		else if ( same(key, "data_file_format") )
			{
			(void) strcpy(data_file_format, action);
			}

		else if ( same(key, "data_file_units") )
			{
			(void) strcpy(data_file_units, action);
			}

		else if ( same(key, "data_file_wind_units") )
			{
			(void) strcpy(data_file_wind_units, action);
			}

		else if ( same(key, "category_cascade") )
			{
			(void) strcpy(cat_cascade, action);
			}

		else if ( same(key, "category_attribute") )
			{
			num_catatt = add_category_attribs(&cat_attrib, ++num_att);
			(void) strcpy(cat_attrib[num_att-1].category_attribute, action);
			}

		else if ( same(key, "category") )
			{
			num_catatt = add_category_attribs(&cat_attrib, ++num_cat);
			(void) strcpy(cat_attrib[num_cat-1].category, action);
			}

		else if ( same(key, "time_zone") )
			{
			(void) strcpy(tzone, action);
			}

		else if ( same(key, "language") )
			{
			(void) strcpy(tlang, action);
			}

		else if ( same(key, "format") )
			{
			(void) strcpy(tbuf, action);

			/* Warning for obsolete format parameters */
			if ( NotNull(strchr(action, ConFormat))
					&& !same_start(action, FormatDirect)
					&& !same_start(action, FormatSymbol)
					&& !same_start(action, FormatText)
					&& !same_start(action, FormatWindBarb)
					&& !same_start(action, FormatWindText)
					&& !same_start(action, FormatWindSymbol)
					&& !same_start(action, FormatVectorText)
					&& !same_start(action, FormatVectorSymbol)
					&& !same_start(action, FormatNone) )
				{
				(void) sprintf(err_buf,
						"Missing \"direct\" parameter in \"format\" keyword");
				(void) warn_report(err_buf);
				(void) strcpy(tbuf, FormatDirect);
				(void) strcat(tbuf, " ");
				(void) strcat(tbuf, action);
				}

			(void) strcpy(format, string_arg(tbuf));
			if ( !blank(tbuf) )
				{
				(void) no_white(tbuf);
				(void) strcpy(conversion_format, tbuf);
				}
			}

		else if ( same(key, "look_up") )
			{
			(void) strcpy(look_up, action);
			}

		else if ( same(key, "case") )
			{
			num_list = add_list_case(&list_case, ++num_case);
			(void) strcpy(list_case[num_case-1].spcase, action);
			}

		else if ( same(key, "case_look_up") )
			{
			num_list = add_list_case(&list_case, ++num_look);
			(void) strcpy(list_case[num_look-1].lookup, action);
			}

		else if ( same(key, "mark") )
			{
			(void) strcpy(mark, find_symbol_file(action));
			}

		else if ( same(key, "mark_scale") )
			{
			(void) sscanf(action, "%f", &markscale);
			}

		else if ( same(key, "symbol_scale") )
			{
			(void) sscanf(action, "%f", &symscale);
			}

		else if ( same(key, "text_size") )
			{
			(void) sscanf(action, "%f", &txt_size);
			}

		else if ( same(key, "display_name") )
			{
			(void) strcpy(display_name, action);
			}

		else if ( same(key, "display_type") )
			{
			(void) strcpy(display_type, action);
			}

		else if ( same(key, "width_scale") )
			{
			(void) sscanf(action, "%f", &wthscale);
			}

		else if ( same(key, "height_scale") )
			{
			(void) sscanf(action, "%f", &hgtscale);
			}

		else if ( same(key, "fit_to_map") )
			{
			if ( same_ic(action, LogicalYes) ) fit_to_map = TRUE;
			else                               fit_to_map = FALSE;
			}

		else if ( same(key, "fit_to_map_ref") )
			{
			(void) strcpy(fit_to_map_ref, action);
			}

		else if ( same(key, "rotation") )
			{
			(void) sscanf(action, "%f", &rotation);
			}

		else if ( same(key, "rotate_to_latitude") )
			{
			if ( same_ic(action, LogicalYes) ) rotate_lat = TRUE;
			else                               rotate_lat = FALSE;
			}

		else if ( same(key, "rotate_to_longitude") )
			{
			if ( same_ic(action, LogicalYes) ) rotate_lon = TRUE;
			else                               rotate_lon = FALSE;
			}

		else if ( same(key, "rotation_attribute") )
			{
			(void) strcpy(rot_attrib, action);
			}

		else if ( same(key, "constrain_rotation") )
			{
			if ( same_ic(action, LogicalYes) ) constrain_rot = TRUE;
			else                               constrain_rot = FALSE;
			}

		else if ( same(key, "x_display_off") )
			{
			(void) sscanf(action, "%f", &xdoff);
			}

		else if ( same(key, "y_display_off") )
			{
			(void) sscanf(action, "%f", &ydoff);
			}

		else if ( same(key, "x_off") )
			{
			(void) sscanf(action, "%f", &xoff);
			}

		else if ( same(key, "y_off") )
			{
			(void) sscanf(action, "%f", &yoff);
			}

		else if ( same(key, "lat") )
			{
			(void) strcpy(lat, action);
			}

		else if ( same(key, "lon") )
			{
			(void) strcpy(lon, action);
			}

		else if ( same(key, "map_x") )
			{
			(void) strcpy(map_x, action);
			}

		else if ( same(key, "map_y") )
			{
			(void) strcpy(map_y, action);
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
			(void) strcpy(location_ident, action);
			}

		else if ( same(key, "location_look_up") )
			{
			(void) strcpy(location_look_up, action);
			}

		else if ( same(key, "proximity") )
			{
			(void) sscanf(action, "%f", &proximity);
			}

		else if ( same(key, "proximity_units") )
			{
			(void) strcpy(proximity_units, action);
			}

		else if ( same(key, "display_at_feature") )
			{
			if ( same_ic(action, LogicalYes) ) display_at_feature = TRUE;
			else                               display_at_feature = FALSE;
			}

		else if ( same(key, "table_name") )
			{
			(void) strcpy(table_name, action);
			}

		else if ( same(key, "grid_name") )
			{
			(void) strcpy(grid_name, action);
			}

		else if ( same(key, "list_name") )
			{
			(void) strcpy(list_name, action);
			}

		else if ( same(key, "source") )
			{
			sdef = identify_source(action, FpaCblank);
			if ( IsNull(sdef) )
				{
				(void) sprintf(err_buf, "Unrecognized source ... %s", action);
				(void) error_report(err_buf);
				}
			(void) strcpy(source, action);
			}

		else if ( same(key, "valid_time") )
			{
			(void) grid_center(&BaseMap, NullPointPtr, NullFloat, &clon);
			(void) strcpy(vtime, interpret_timestring(action, T0stamp, clon));
			}

		else if ( same(key, "cross_section_name") )
			{
			(void) strcpy(cross_section_name, action);
			}

		else if ( same(key, "location_distances") )
			{
			if      (ltype == GPG_LocNone) ltype = GPG_LocDistance;
			else if (ltype != GPG_LocDistance)
				{
				if (ltype == GPG_LocTime)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_distances\" and \"location_times\" in same directive");
				else if (ltype == GPG_LocFraction)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_distances\" and \"location_fractions\" in same directive");
				(void) error_report(err_buf);
				}
			(void) strcpy(tbuf, action);
			while (TRUE)
				{
				fval = float_arg(tbuf, &status);
				if ( !status ) break;

				num_xloc = add_xsect_location(&xsect_locs);
				xsect_locs[num_xloc-1].xdist = fval;
				}
			}

		else if ( same(key, "location_units") )
			{
			(void) strcpy(location_units, action);
			}

		else if ( same(key, "location_times") )
			{
			if      (ltype == GPG_LocNone) ltype = GPG_LocTime;
			else if (ltype != GPG_LocTime)
				{
				if (ltype == GPG_LocDistance)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_times\" and \"location_distances\" in same directive");
				else if (ltype == GPG_LocFraction)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_times\" and \"location_fractions\" in same directive");
				(void) error_report(err_buf);
				}
			(void) strcpy(tbuf, action);
			while ( !blank( next = string_arg(tbuf) ) )
				{
				num_xloc = add_xsect_location(&xsect_locs);
				(void) strcpy(xsect_locs[num_xloc-1].vtime, next);
				}
			}

		else if ( same(key, "location_fractions") )
			{
			if      (ltype == GPG_LocNone) ltype = GPG_LocFraction;
			else if (ltype != GPG_LocFraction)
				{
				if (ltype == GPG_LocDistance)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_fractions\" and \"location_distances\" in same directive");
				else if (ltype == GPG_LocTime)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_fractions\" and \"location_times\" in same directive");
				(void) error_report(err_buf);
				}
			(void) strcpy(tbuf, action);
			while (TRUE)
				{
				fval = float_arg(tbuf, &status);
				if ( !status ) break;

				num_xloc = add_xsect_location(&xsect_locs);
				xsect_locs[num_xloc-1].xdist = fval;
				}
			}

		else if ( same(key, "vertical_look_up") )
			{
			(void) strcpy(vertical_look_up, action);
			}

		else if ( same(key, "vertical_element") )
			{
			(void) strcpy(vertical_element, action);
			}

		else if ( same(key, "vertical_level") )
			{
			(void) strcpy(vertical_level, action);
			}

		else if ( same(key, "vertical_equation") )
			{
			(void) strcpy(vertical_equation, action);
			if ( blank(vertical_element) )
				(void) strcpy(vertical_element, FpaCanyElement);
			}

		else if ( same(key, "vertical_units") )
			{
			(void) strcpy(vertical_units, action);
			}

		else if ( same(key, "vertical_field_type") )
			{
			(void) strcpy(vertical_field_type, action);
			}

		else if ( same(key, "vertical_data_file") )
			{
			(void) strcpy(data_file, action);
			if ( blank(vertical_element) )
				(void) strcpy(vertical_element, FpaCanyElement);
			if ( blank(vertical_level) )
				(void) strcpy(vertical_level,   FpaCanyLevel);
			}

		else if ( same(key, "vertical_data_file_format") )
			{
			(void) strcpy(vertical_data_file_format, action);
			}

		else if ( same(key, "vertical_data_file_units") )
			{
			(void) strcpy(vertical_data_file_units, action);
			}

		else if ( same(key, "vertical_attribute") )
			{
			(void) strcpy(vertical_attribute, action);
			}

		else if ( same(key, "vertical_attribute_upper") )
			{
			(void) strcpy(vertical_attribute_upper, action);
			}

		else if ( same(key, "vertical_attribute_lower") )
			{
			(void) strcpy(vertical_attribute_lower, action);
			}

		/* Skip valid attribute keywords (for the moment) */
		else if ( same(key,  "attribute")
				|| same(key, "attribute_show")
				|| same(key, "attribute_anchor")
				|| same(key, "attribute_ref")
				|| same(key, "attribute_justification")
				|| same(key, "attribute_vertical_just")
				|| same(key, "attribute_units")
				|| same(key, "attribute_format")
				|| same(key, "attribute_look_up")
				|| same(key, "attribute_symbol_scale")
				|| same(key, "attribute_text_size")
				|| same(key, "attribute_display_name")
				|| same(key, "attribute_display_type")
				|| same(key, "attribute_width_scale")
				|| same(key, "attribute_height_scale")
				|| same(key, "attribute_x_off")
				|| same(key, "attribute_y_off")
				|| same(key, "attribute_line_width")
				|| same(key, "attribute_line_style")
				|| same(key, "attribute_outline")
				|| same(key, "attribute_fill")
				|| same(key, "attribute_outline_first")
				|| same(key, "attribute_font")
				|| same(key, "attribute_font_weight")
				|| same(key, "attribute_italics")
				|| same(key, "attribute_char_space")
				|| same(key, "attribute_word_space") )
			{
			}

		/* Skip valid presentation keywords already processed */
		else if ( same(key,  "line_width")
				|| same(key, "line_style")
				|| same(key, "outline")
				|| same(key, "fill")
				|| same(key, "outline_first")
				|| same(key, "font")
				|| same(key, "font_weight")
				|| same(key, "italics")
				|| same(key, "justification")
				|| same(key, "char_space")
				|| same(key, "word_space") )
			{
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Initialize storage for attribute display list */
	(void) initialize_attribute_display();

	/* Now process all attribute keywords */
	need_default_attrib = TRUE;
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		/* Add another attribute to the list ... with default parameters */
		if ( same(key, "attribute") )
			{
			temp_attrib = add_attribute_display(action, units,
							format, conversion_format, look_up, symscale,
							txt_size, wthscale, hgtscale, &CurPres);
			need_default_attrib = FALSE;
			}

		/* Add the default attribute to the list ... if necessary */
		else if ( same_start(key, "attribute_") && need_default_attrib )
				{
				temp_attrib = add_attribute_display(AttribGPGenDefault, units,
								format, conversion_format, look_up, symscale,
								txt_size, wthscale, hgtscale, &CurPres);
				}

		/* Now add the attribute parameters */
		if ( same(key, "attribute") )
			{
			}

		else if ( same(key, "attribute_show") )
			{
			if ( same_ic(action, "no") ) temp_attrib->show_label = FALSE;
			else                         temp_attrib->show_label = TRUE;
			}

		/* Adjust attribute parameters for this attribute */
		else if ( same(key, "attribute_anchor") )
			{
			(void) strcpy(temp_attrib->anchor, action);
			}

		else if ( same(key, "attribute_ref") )
			{
			(void) strcpy(temp_attrib->ref, action);
			}

		else if ( same(key, "attribute_justification") )
			{
			(void) strcpy(temp_attrib->presentation.justified, action);
			}

		else if ( same(key, "attribute_vertical_just") )
			{
			(void) strcpy(temp_attrib->vertical_just, action);
			}

		else if ( same(key, "attribute_units") )
			{
			(void) strcpy(temp_attrib->units, action);
			}

		else if ( same(key, "attribute_format") )
			{
			(void) strcpy(temp_attrib->format, string_arg(action));
			if ( !blank(action) )
				{
				(void) no_white(action);
				(void) strcpy(temp_attrib->conversion_format, action);
				}
			}

		else if ( same(key, "attribute_look_up") )
			{
			(void) strcpy(temp_attrib->look_up, action);
			}

		else if ( same(key, "attribute_symbol_scale") )
			{
			(void) sscanf(action, "%f", &(temp_attrib->symbol_scale));
			}

		else if ( same(key, "attribute_text_size") )
			{
			(void) sscanf(action, "%f", &(temp_attrib->txt_size));
			}

		else if ( same(key, "attribute_display_name") )
			{
			(void) strcpy(temp_attrib->display_name, action);
			}

		else if ( same(key, "attribute_display_type") )
			{
			(void) strcpy(temp_attrib->display_type, action);
			}

		else if ( same(key, "attribute_width_scale") )
			{
			(void) sscanf(action, "%f", &(temp_attrib->width_scale));
			}

		else if ( same(key, "attribute_height_scale") )
			{
			(void) sscanf(action, "%f", &(temp_attrib->height_scale));
			}

		else if ( same(key, "attribute_x_off") )
			{
			(void) sscanf(action, "%f", &(temp_attrib->x_off));
			}

		else if ( same(key, "attribute_y_off") )
			{
			(void) sscanf(action, "%f", &(temp_attrib->y_off));
			}

		else if ( same(key, "attribute_line_width") )
			{
			(void) strcpy(temp_attrib->presentation.line_width, action);
			}

		else if ( same(key, "attribute_line_style") )
			{
			(void) strcpy(temp_attrib->presentation.line_style, action);
			}

		else if ( same(key, "attribute_outline") )
			{
			(void) strcpy(temp_attrib->presentation.outline, action);
			}

		else if ( same(key, "attribute_fill") )
			{
			(void) strcpy(temp_attrib->presentation.fill, action);
			}

		else if ( same(key, "attribute_outline_first") )
			{
			if ( same_ic(action, LogicalYes) )
				{
				temp_attrib->presentation.outline_first = TRUE;
				}
			else
				{
				temp_attrib->presentation.outline_first = FALSE;
				}
			}

		else if ( same(key, "attribute_font") )
			{
			(void) strcpy(temp_attrib->presentation.font, action);
			}

		else if ( same(key, "attribute_font_weight") )
			{
			(void) strcpy(temp_attrib->presentation.font_weight, action);
			}

		else if ( same(key, "attribute_italics") )
			{
			(void) strcpy(temp_attrib->presentation.italics, action);
			}

		else if ( same(key, "attribute_char_space") )
			{
			(void) strcpy(temp_attrib->presentation.char_space, action);
			}

		else if ( same(key, "attribute_word_space") )
			{
			(void) strcpy(temp_attrib->presentation.word_space, action);
			}
		}

	/* Ensure that at least one set of category parameters has been set */
	/*  ... or initialize one with some default values!                 */
	if ( num_catatt < 1 )
		{
		num_catatt = add_category_attribs(&cat_attrib, ++num_att);
		}

	/* Ensure that at least one attribute has been set ... or set a default! */
	num_attrib = return_attribute_display(&attribs);
	if ( num_attrib < 1 )
		{
		(void) add_attribute_display(AttribGPGenDefault, units,
				format, conversion_format, look_up, symscale,
				txt_size, wthscale, hgtscale, &CurPres);
		}

	/* Error return for missing parameters */
	if ( blank(element) && blank(equation) && blank(data_file) )
		(void) error_report("No element or equation or data file");
	if ( blank(level) )
		(void) error_report("No level");
	if ( IsNull(identify_unit(units)) )
		{
		(void) sprintf(err_buf, "Unknown units: %s", units);
		(void) error_report(err_buf);
		}
	if ( !blank(data_file) && blank(data_file_format) )
		(void) error_report("No data file format");
	if ( !blank(data_file_units) && IsNull(identify_unit(data_file_units)) )
		{
		(void) sprintf(err_buf, "Unknown data file units: %s", data_file_units);
		(void) error_report(err_buf);
		}
	if ( !blank(data_file_wind_units)
			&& IsNull(identify_unit(data_file_wind_units)) )
		{
		(void) sprintf(err_buf,
				"Unknown data file wind units: %s", data_file_wind_units);
		(void) error_report(err_buf);
		}
	if ( IsNull(identify_unit(vertical_units)) )
		{
		(void) sprintf(err_buf, "Unknown vertical units: %s", vertical_units);
		(void) error_report(err_buf);
		}
	if ( !blank(vertical_data_file) && blank(vertical_data_file_format) )
		(void) error_report("No vertical data file format");
	if ( !blank(vertical_data_file_units)
			&& IsNull(identify_unit(vertical_data_file_units)) )
		{
		(void) sprintf(err_buf, "Unknown vertical data file units: %s",
			vertical_data_file_units);
		(void) error_report(err_buf);
		}

	/* Error return for no locations to sample */
	if ( ( blank(lat) || blank(lon) )
			&& ( blank(map_x) || blank(map_y) )
			&& ( blank(location_look_up) || blank(location_ident) )
			&& blank(table_name) && blank(grid_name) && blank(list_name)
			&& blank(cross_section_name) )
		(void) error_report("No location, table, grid, list or cross section");

	/* Check "format" "conversion_format" "units" "look_up" */
	/*  keywords for attributes (based on sample format)    */
	num_attrib = return_attribute_display(&attribs);
	for ( nn=0; nn<num_attrib; nn++ )
		{
		if ( blank(attribs[nn].format) )
			{
			(void) sprintf(err_buf, "No sample format for attribute: %s",
					attribs[nn].name);
			(void) error_report(err_buf);
			}

		/* Error return for incorrect "format" ... based on application */
		switch ( Program.macro )
			{
			case GPG_PSMet:
			case GPG_SVGMet:
			case GPG_CorMet:
				if ( !same(attribs[nn].format, FormatDirect)
						&& !same(attribs[nn].format, FormatSymbol)
						&& !same(attribs[nn].format, FormatText)
						&& !same(attribs[nn].format, FormatWindBarb)
						&& !same(attribs[nn].format, FormatWindText)
						&& !same(attribs[nn].format, FormatWindSymbol)
						&& !same(attribs[nn].format, FormatVectorText)
						&& !same(attribs[nn].format, FormatVectorSymbol)
						&& !same(attribs[nn].format, FormatNone) )
					{
					(void) sprintf(err_buf,
							"Recognized format types are: %s %s %s %s %s %s %s %s %s",
							FormatDirect, FormatSymbol, FormatText,
							FormatWindBarb, FormatWindText, FormatWindSymbol,
							FormatVectorText, FormatVectorSymbol, FormatNone);
					(void) error_report(err_buf);
					}
				break;

			case GPG_TexMet:
				if ( !same(attribs[nn].format, FormatDirect)
						&& !same(attribs[nn].format, FormatText)
						&& !same(attribs[nn].format, FormatWindText)
						&& !same(attribs[nn].format, FormatVectorText) )
					{
					(void) sprintf(err_buf,
							"Recognized format types are: %s %s %s %s",
							FormatDirect,
							FormatText, FormatWindText, FormatVectorText);
					(void) error_report(err_buf);
					}
				break;
			}

		if ( !blank(attribs[nn].conversion_format) )
			{
			if ( IsNull(strchr(attribs[nn].conversion_format, ConFormat)) )
				{
				(void) sprintf(err_buf,
						"Error in sample conversion format: %s  for attribute: %s",
						attribs[nn].conversion_format, attribs[nn].name);
				(void) error_report(err_buf);
				}
			}

		if ( IsNull(identify_unit(attribs[nn].units)) )
			{
			(void) sprintf(err_buf,
					"Unknown attribute units: %s  for attribute: %s",
					attribs[nn].units, attribs[nn].name);
			(void) error_report(err_buf);
			}

		if ( same(attribs[nn].format, FormatSymbol)
				|| same(attribs[nn].format, FormatText) )
			{
			if ( blank(attribs[nn].look_up) )
				{
				(void) sprintf(err_buf, "No look_up for attribute: %s",
						attribs[nn].name);
				(void) error_report(err_buf);
				}
			}
		}

	/* Error return for incorrect "fit_to_map_ref" ... based on application */
	/*  ... and reset "fit_to_map_ref" if "rotation" is non-zero            */
	switch ( Program.macro )
		{
		case GPG_PSMet:
		case GPG_SVGMet:
		case GPG_CorMet:
			if ( !same(fit_to_map_ref, FitToMapRefNone)
					&& !same(fit_to_map_ref, FitToMapRefUpper)
					&& !same(fit_to_map_ref, FitToMapRefLower)
					&& !same(fit_to_map_ref, FitToMapRefLeft)
					&& !same(fit_to_map_ref, FitToMapRefRight) )
				{
				(void) sprintf(err_buf,
						"Recognized fit_to_map_ref types are: %s %s %s %s %s",
						FitToMapRefNone, FitToMapRefUpper, FitToMapRefLower,
						FitToMapRefLeft, FitToMapRefRight);
				(void) error_report(err_buf);
				}
			if ( !same(fit_to_map_ref, FitToMapRefNone) && (rotation != 0.0
						|| rotate_lat || rotate_lon || !blank(rot_attrib)) )
				{
				(void) sprintf(err_buf,
						"Rotated labels allow only \"fit_to_map_ref = %s\"",
						FitToMapRefNone);
				(void) warn_report(err_buf);
				(void) strcpy(fit_to_map_ref, FitToMapRefNone);
				}
		}

	/* Set the parameter for cross section display */
	if ( !blank(cross_section_name) ) AnchorToCrossSection = TRUE;

	/* Error if more than one "rotate_..." keyword set */
	if ( (rotate_lat && rotate_lon) || (rotate_lat && !blank(rot_attrib))
			|| (rotate_lon && !blank(rot_attrib)) )
		(void) error_report("Can set only one of \"rotate_to_latitude\" \"rotate_to_longitude\" \"rotation_attribute\"");

	/* Convert proximity to km */
	if ( !convert_value(proximity_units, (double) proximity,
			ProximityUnitsKm, &dprox) )
		{
		(void) sprintf(err_buf,
				"Incorrect proximity units: %s", proximity_units);
		(void) error_report(err_buf);
		}
	proximity = (float) dprox;

	/* Convert cross section values to km (if required) */
	for ( ii=0; ii<num_xloc; ii++ )
		{
		if ( !convert_value(location_units, (double) xsect_locs[ii].xdist,
				LocationUnitsKm, &dval) )
			{
			(void) sprintf(err_buf,
					"Incorrect location units: %s", location_units);
			(void) error_report(err_buf);
			}
		xsect_locs[ii].xdist = (float) dval;
		}

	/* Now output the field samples */
	sampleok = GRA_sample_field(element, level, equation, units, field_type,
				geo_file, data_file, data_file_format, data_file_units,
				data_file_wind_units, cat_cascade, cat_attrib, num_catatt,
				attribs, num_attrib, list_case, num_list, mark, markscale,
				tzone, tlang, display_name, display_type, fit_to_map,
				fit_to_map_ref, rotation, rotate_lat, rotate_lon, rot_attrib,
				constrain_rot, xdoff, ydoff, xoff, yoff, lat, lon,
				map_x, map_y, map_units, location_ident, location_look_up,
				proximity, display_at_feature, table_name, grid_name,
				list_name, source, vtime, cross_section_name, ltype,
				num_xloc, xsect_locs, vertical_look_up, vertical_element,
				vertical_level, vertical_equation, vertical_units,
				vertical_field_type,
				vertical_data_file, vertical_data_file_format,
				vertical_data_file_units, vertical_attribute,
				vertical_attribute_upper, vertical_attribute_lower);

	/* Unset the parameter for cross section display */
	if ( !blank(cross_section_name) ) AnchorToCrossSection = FALSE;

	/* Free the cross section location buffers */
	(void) free_xsect_locations();

	/* Free category attribute and special case buffers */
	(void) free_category_attribs();
	(void) free_list_case();

	/* Return results of field sampling */
	return sampleok;
	}

/***********************************************************************
*                                                                      *
*    d o _ s a m p l e _ w i n d                                       *
*                                                                      *
***********************************************************************/

LOGICAL		do_sample_wind

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	LOGICAL			sampleok;
	int				ii;
	STRING			next;
	float			fval;
	double			dval;
	LOGICAL			status;
	char			wind_crossref[GPGMedium], units[GPGMedium];
	char			format[GPGMedium], mark[GPGMedium];
	char			display_name[GPGMedium], display_type[GPGMedium];
	float			markscale, wthscale, hgtscale;
	LOGICAL			fit_to_map;
	float			rotation, map_units, xdoff, ydoff, xoff, yoff, clon;
	LOGICAL			rotate_lat, rotate_lon;
	char			lat[GPGMedium], lon[GPGMedium];
	char			map_x[GPGMedium], map_y[GPGMedium];
	char			location_ident[GPGMedium], location_look_up[GPGMedium];
	char			table_name[GPGMedium];
	char			grid_name[GPGMedium], list_name[GPGMedium];
	char			source[GPGMedium], vtime[GPGTiny];
	char			cross_section_name[GPGMedium];
	char			location_units[GPGMedium], vertical_look_up[GPGMedium];
	char			vertical_element[GPGMedium], vertical_level[GPGMedium];
	char			vertical_equation[GPGMedium], vertical_units[GPGMedium];
	char			vertical_attribute[GPGMedium];
	char			vertical_attribute_upper[GPGMedium];
	char			vertical_attribute_lower[GPGMedium];
	int				num_xloc, ltype;
	XSECT_LOCATION	*xsect_locs;
	STRING			key, action;
	char			tbuf[GPGMedium], err_buf[GPGLong];

	FpaConfigSourceStruct		*sdef;

	/* Initialize all parameters */
	(void) strcpy(wind_crossref,            FpaCblank);
	(void) strcpy(units,                    FpaCmksUnits);
	(void) strcpy(format,                   FormatWindText);
	(void) strcpy(mark,                     FpaCblank);
	(void) strcpy(display_name,             FpaCblank);
	(void) strcpy(display_type,             FpaCblank);
	(void) strcpy(lat,                      FpaCblank);
	(void) strcpy(lon,                      FpaCblank);
	(void) strcpy(map_x,                    FpaCblank);
	(void) strcpy(map_y,                    FpaCblank);
	(void) strcpy(location_ident,           FpaCblank);
	(void) strcpy(location_look_up,         FpaCblank);
	(void) strcpy(table_name,               FpaCblank);
	(void) strcpy(grid_name,                FpaCblank);
	(void) strcpy(list_name,                FpaCblank);
	(void) strcpy(source,                   CurSource);
	(void) strcpy(vtime,                    TVstamp);
	(void) strcpy(cross_section_name,       FpaCblank);
	(void) strcpy(vertical_look_up,         FpaCblank);
	(void) strcpy(vertical_element,         FpaCblank);
	(void) strcpy(vertical_level,           FpaCblank);
	(void) strcpy(vertical_equation,        FpaCblank);
	(void) strcpy(vertical_units,           FpaCmksUnits);
	(void) strcpy(vertical_attribute,       FpaCblank);
	(void) strcpy(vertical_attribute_upper, FpaCblank);
	(void) strcpy(vertical_attribute_lower, FpaCblank);
	fit_to_map = FALSE;
	markscale  = 100.0;
	wthscale   = Program.default_width_scale;
	hgtscale   = Program.default_height_scale;
	rotation   = 0.0;
	rotate_lat = FALSE;
	rotate_lon = FALSE;
	map_units  = BaseMap.definition.units;
	xdoff      = 0.0;
	ydoff      = 0.0;
	xoff       = 0.0;
	yoff       = 0.0;
	num_xloc   =   0;
	ltype      = GPG_LocNone;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "wind_crossref") )
			{
			(void) strcpy(wind_crossref, action);
			}

		else if ( same(key, "units") )
			{
			(void) strcpy(units, action);
			}

		else if ( same(key, "format") )
			{
			(void) strcpy(format, action);
			}

		else if ( same(key, "mark") )
			{
			(void) strcpy(mark, find_symbol_file(action));
			}

		else if ( same(key, "mark_scale") )
			{
			(void) sscanf(action, "%f", &markscale);
			}

		else if ( same(key, "display_name") )
			{
			(void) strcpy(display_name, action);
			}

		else if ( same(key, "display_type") )
			{
			(void) strcpy(display_type, action);
			}

		else if ( same(key, "width_scale") )
			{
			(void) sscanf(action, "%f", &wthscale);
			}

		else if ( same(key, "height_scale") )
			{
			(void) sscanf(action, "%f", &hgtscale);
			}

		else if ( same(key, "fit_to_map") )
			{
			if ( same_ic(action, LogicalYes) ) fit_to_map = TRUE;
			else                               fit_to_map = FALSE;
			}

		else if ( same(key, "rotation") )
			{
			(void) sscanf(action, "%f", &rotation);
			}

		else if ( same(key, "rotate_to_latitude") )
			{
			if ( same_ic(action, LogicalYes) ) rotate_lat = TRUE;
			else                               rotate_lat = FALSE;
			}

		else if ( same(key, "rotate_to_longitude") )
			{
			if ( same_ic(action, LogicalYes) ) rotate_lon = TRUE;
			else                               rotate_lon = FALSE;
			}

		else if ( same(key, "x_display_off") )
			{
			(void) sscanf(action, "%f", &xdoff);
			}

		else if ( same(key, "y_display_off") )
			{
			(void) sscanf(action, "%f", &ydoff);
			}

		else if ( same(key, "x_off") )
			{
			(void) sscanf(action, "%f", &xoff);
			}

		else if ( same(key, "y_off") )
			{
			(void) sscanf(action, "%f", &yoff);
			}

		else if ( same(key, "lat") )
			{
			(void) strcpy(lat, action);
			}

		else if ( same(key, "lon") )
			{
			(void) strcpy(lon, action);
			}

		else if ( same(key, "map_x") )
			{
			(void) strcpy(map_x, action);
			}

		else if ( same(key, "map_y") )
			{
			(void) strcpy(map_y, action);
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
			(void) strcpy(location_ident, action);
			}

		else if ( same(key, "location_look_up") )
			{
			(void) strcpy(location_look_up, action);
			}

		else if ( same(key, "table_name") )
			{
			(void) strcpy(table_name, action);
			}

		else if ( same(key, "grid_name") )
			{
			(void) strcpy(grid_name, action);
			}

		else if ( same(key, "list_name") )
			{
			(void) strcpy(list_name, action);
			}

		else if ( same(key, "source") )
			{
			sdef = identify_source(action, FpaCblank);
			if ( IsNull(sdef) )
				{
				(void) sprintf(err_buf, "Unrecognized source ... %s", action);
				(void) error_report(err_buf);
				}
			(void) strcpy(source, action);
			}

		else if ( same(key, "valid_time") )
			{
			(void) grid_center(&BaseMap, NullPointPtr, NullFloat, &clon);
			(void) strcpy(vtime, interpret_timestring(action, T0stamp, clon));
			}

		else if ( same(key, "cross_section_name") )
			{
			(void) strcpy(cross_section_name, action);
			}

		else if ( same(key, "location_distances") )
			{
			if      (ltype == GPG_LocNone) ltype = GPG_LocDistance;
			else if (ltype != GPG_LocDistance)
				{
				if (ltype == GPG_LocTime)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_distances\" and \"location_times\" in same directive");
				else if (ltype == GPG_LocFraction)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_distances\" and \"location_fractions\" in same directive");
				(void) error_report(err_buf);
				}
			(void) strcpy(tbuf, action);
			while (TRUE)
				{
				fval = float_arg(tbuf, &status);
				if ( !status ) break;

				num_xloc = add_xsect_location(&xsect_locs);
				xsect_locs[num_xloc-1].xdist = fval;
				}
			}

		else if ( same(key, "location_units") )
			{
			(void) strcpy(location_units, action);
			}

		else if ( same(key, "location_times") )
			{
			if      (ltype == GPG_LocNone) ltype = GPG_LocTime;
			else if (ltype != GPG_LocTime)
				{
				if (ltype == GPG_LocDistance)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_times\" and \"location_distances\" in same directive");
				else if (ltype == GPG_LocFraction)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_times\" and \"location_fractions\" in same directive");
				(void) error_report(err_buf);
				}
			(void) strcpy(tbuf, action);
			while ( !blank( next = string_arg(tbuf) ) )
				{
				num_xloc = add_xsect_location(&xsect_locs);
				(void) strcpy(xsect_locs[num_xloc-1].vtime, next);
				}
			}

		else if ( same(key, "location_fractions") )
			{
			if      (ltype == GPG_LocNone) ltype = GPG_LocFraction;
			else if (ltype != GPG_LocFraction)
				{
				if (ltype == GPG_LocDistance)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_fractions\" and \"location_distances\" in same directive");
				else if (ltype == GPG_LocTime)
					(void) sprintf(err_buf,
						"Cannot have keywords \"location_fractions\" and \"location_times\" in same directive");
				(void) error_report(err_buf);
				}
			(void) strcpy(tbuf, action);
			while (TRUE)
				{
				fval = float_arg(tbuf, &status);
				if ( !status ) break;

				num_xloc = add_xsect_location(&xsect_locs);
				xsect_locs[num_xloc-1].xdist = fval;
				}
			}

		else if ( same(key, "vertical_look_up") )
			{
			(void) strcpy(vertical_look_up, action);
			}

		else if ( same(key, "vertical_element") )
			{
			(void) strcpy(vertical_element, action);
			}

		else if ( same(key, "vertical_level") )
			{
			(void) strcpy(vertical_level, action);
			}

		else if ( same(key, "vertical_equation") )
			{
			(void) strcpy(vertical_equation, action);
			if ( blank(vertical_element) )
				(void) strcpy(vertical_element, FpaCanyElement);
			}

		else if ( same(key, "vertical_units") )
			{
			(void) strcpy(vertical_units, action);
			}

		else if ( same(key, "vertical_attribute") )
			{
			(void) strcpy(vertical_attribute, action);
			}

		else if ( same(key, "vertical_attribute_upper") )
			{
			(void) strcpy(vertical_attribute_upper, action);
			}

		else if ( same(key, "vertical_attribute_lower") )
			{
			(void) strcpy(vertical_attribute_lower, action);
			}

		/* Skip valid presentation keywords already processed */
		else if ( same(key,  "line_width")
				|| same(key, "line_style")
				|| same(key, "outline")
				|| same(key, "fill")
				|| same(key, "outline_first")
				|| same(key, "font")
				|| same(key, "font_weight")
				|| same(key, "italics")
				|| same(key, "justification")
				|| same(key, "char_space")
				|| same(key, "word_space") )
			{
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Error return for missing parameters */
	if ( blank(wind_crossref) )
		(void) error_report("No wind_crossref");
	if ( blank(format) )
		(void) error_report("No wind format");
	if ( IsNull(identify_unit(units)) )
		{
		(void) sprintf(err_buf, "Unknown units: %s", units);
		(void) error_report(err_buf);
		}
	if ( IsNull(identify_unit(vertical_units)) )
		{
		(void) sprintf(err_buf, "Unknown vertical units: %s", vertical_units);
		(void) error_report(err_buf);
		}

	/* Error return for no locations to sample */
	if ( ( blank(lat) || blank(lon) )
			&& ( blank(map_x) || blank(map_y) )
			&& ( blank(location_look_up) || blank(location_ident) )
			&& blank(table_name) && blank(grid_name) && blank(list_name)
			&& blank(cross_section_name) )
		(void) error_report("No location, table, grid or cross section");

	/* Error return for incorrect "format" ... based on application */
	switch ( Program.macro )
		{
		case GPG_PSMet:
		case GPG_SVGMet:
		case GPG_CorMet:
			if ( blank(cross_section_name) )
				{
				if ( !same(format, FormatWindBarb)
						&& !same(format, FormatWindText)
						&& !same(format, FormatWindSymbol) )
					{
					(void) sprintf(err_buf,
							"Recognized wind format types are: %s %s %s",
							FormatWindBarb, FormatWindText, FormatWindSymbol);
					(void) error_report(err_buf);
					}
				}
			else
				{
				if ( !same(format, FormatWindBarb)
						&& !same(format, FormatWindText)
						&& !same(format, FormatWindSymbol)
						&& !same(format, FormatTailWindBarb)
						&& !same(format, FormatTailWindText)
						&& !same(format, FormatTailWindSymbol)
						&& !same(format, FormatCrossWindBarb)
						&& !same(format, FormatCrossWindText)
						&& !same(format, FormatCrossWindSymbol) )
					{
					(void) sprintf(err_buf,
							"Recognized wind format types (for cross sections) are: %s %s %s %s %s %s %s %s %s",
							FormatWindBarb, FormatWindText, FormatWindSymbol
							FormatTailWindBarb, FormatTailWindText,
							FormatTailWindSymbol
							FormatCrossWindBarb, FormatCrossWindText,
							FormatCrossWindSymbol);
					(void) error_report(err_buf);
					}
				}
			break;

		case GPG_TexMet:
			if ( blank(cross_section_name) )
				{
				if ( !same(format, FormatWindText) )
					{
					(void) sprintf(err_buf,
							"Recognized wind format types are: %s",
							FormatWindText);
					(void) error_report(err_buf);
					}
				}
			else
				{
				if ( !same(format, FormatWindText)
						&& !same(format, FormatTailWindText)
						&& !same(format, FormatCrossWindText) )
					{
					(void) sprintf(err_buf,
							"Recognized wind format types (for cross sections) are: %s %s %s",
							FormatWindText, FormatTailWindText,
							FormatCrossWindText);
					(void) error_report(err_buf);
					}
				}
			break;
		}

	/* Convert cross section values to km (if required) */
	for ( ii=0; ii<num_xloc; ii++ )
		{
		if ( !convert_value(location_units, (double) xsect_locs[ii].xdist,
				LocationUnitsKm, &dval) )
			{
			(void) sprintf(err_buf,
					"Incorrect location units: %s", location_units);
			(void) error_report(err_buf);
			}
		xsect_locs[ii].xdist = (float) dval;
		}

	/* Set the parameter for cross section display */
	if ( !blank(cross_section_name) ) AnchorToCrossSection = TRUE;

	/* Error if both "rotate_to_latitude" and "rotate_to_longitude" set */
	if ( rotate_lat && rotate_lon )
		(void) error_report("Can set only one of \"rotate_to_latitude\" \"rotate_to_longitude\"");

	/* Now output the wind samples */
	sampleok = GRA_sample_wind(wind_crossref, units, format, mark, markscale,
				display_name, display_type, wthscale, hgtscale, fit_to_map,
				rotation, rotate_lat, rotate_lon, xdoff, ydoff, xoff, yoff,
				lat, lon, map_x, map_y, map_units, location_ident,
				location_look_up, table_name, grid_name, list_name, source,
				vtime, cross_section_name, ltype, num_xloc, xsect_locs,
				vertical_look_up, vertical_element, vertical_level,
				vertical_equation, vertical_units, vertical_attribute,
				vertical_attribute_upper, vertical_attribute_lower);

	/* Unset the parameter for cross section display */
	if ( !blank(cross_section_name) ) AnchorToCrossSection = FALSE;

	/* Free the cross section location buffers */
	(void) free_xsect_locations();

	/* Return results of wind sampling */
	return sampleok;
	}

/***********************************************************************
*                                                                      *
*    d o _ t a b l e _ s i t e                                         *
*                                                                      *
***********************************************************************/

LOGICAL		do_table_site

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	char		table_name[GPGMedium], site_label[GPGMedium];
	char		lat[GPGMedium], lon[GPGMedium];
	char		map_x[GPGMedium], map_y[GPGMedium];
	char		location_ident[GPGMedium];
	float		size, txt_size, rotation, map_units;
	float		xlabel, ylabel, xoff, yoff;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Set current text size */
	(void) sscanf(CurPres.text_size, "%f", &size);

	/* Initialize all parameters */
	(void) strcpy(table_name,     FpaCblank);
	(void) strcpy(site_label,     FpaCblank);
	(void) strcpy(lat,            FpaCblank);
	(void) strcpy(lon,            FpaCblank);
	(void) strcpy(map_x,          FpaCblank);
	(void) strcpy(map_y,          FpaCblank);
	(void) strcpy(location_ident, FpaCblank);
	txt_size  = size;
	rotation  =  0.0;
	map_units = BaseMap.definition.units;
	xlabel    =  0.0;
	ylabel    =  0.0;
	xoff      =  0.0;
	yoff      =  0.0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "table_name") )
			{
			(void) strcpy(table_name, action);
			}

		else if ( same(key, "site_label") )
			{
			(void) strcpy(site_label, action);
			}

		else if ( same(key, "text_size") )
			{
			(void) sscanf(action, "%f", &txt_size);
			}

		else if ( same(key, "rotation") )
			{
			(void) sscanf(action, "%f", &rotation);
			}

		else if ( same(key, "x_label") )
			{
			(void) sscanf(action, "%f", &xlabel);
			}

		else if ( same(key, "y_label") )
			{
			(void) sscanf(action, "%f", &ylabel);
			}

		else if ( same(key, "x_off") )
			{
			(void) sscanf(action, "%f", &xoff);
			}

		else if ( same(key, "y_off") )
			{
			(void) sscanf(action, "%f", &yoff);
			}

		else if ( same(key, "lat") )
			{
			(void) strcpy(lat, action);
			}

		else if ( same(key, "lon") )
			{
			(void) strcpy(lon, action);
			}

		else if ( same(key, "map_x") )
			{
			(void) strcpy(map_x, action);
			}

		else if ( same(key, "map_y") )
			{
			(void) strcpy(map_y, action);
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
			(void) strcpy(location_ident, action);
			}

		/* Skip valid presentation keywords already processed */
		else if ( same(key,  "line_width")
				|| same(key, "line_style")
				|| same(key, "outline")
				|| same(key, "fill")
				|| same(key, "outline_first")
				|| same(key, "font")
				|| same(key, "font_weight")
				|| same(key, "italics")
				|| same(key, "justification")
				|| same(key, "char_space")
				|| same(key, "word_space") )
			{
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
	if ( ( blank(lat) || blank(lon) )
			&& ( blank(map_x) || blank(map_y) ) && blank(location_ident) )
		(void) error_report("No lat/lon or map_x/map_y or location_ident");

	/* Now add the table site */
	return GRA_add_table_site(table_name, site_label, txt_size, rotation,
			xlabel, ylabel, xoff, yoff, lat, lon,
			map_x, map_y, map_units, location_ident);
	}

/***********************************************************************
*                                                                      *
*    d o _ t e x t                                                     *
*                                                                      *
***********************************************************************/

LOGICAL		do_text

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	char		text_file[GPGMedium], string[GPGMedium];
	char		attribute[GPGMedium];
	char		format[GPGMedium], conversion_format[GPGMedium];
	char		lat[GPGMedium], lon[GPGMedium];
	char		map_x[GPGMedium], map_y[GPGMedium];
	char		location_ident[GPGMedium], location_look_up[GPGMedium];
	char		vtime[GPGTiny], table_name[GPGMedium];
	char		grid_name[GPGMedium], list_name[GPGMedium];
	LOGICAL		rotate_lat, rotate_lon;
	float		size, txt_size, rotation, map_units, xoff, yoff, clon;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Set current text size */
	(void) sscanf(CurPres.text_size, "%f", &size);

	/* Initialize all parameters */
	(void) strcpy(text_file,         FpaCblank);
	(void) strcpy(string,            FpaCblank);
	(void) strcpy(attribute,         FpaCblank);
	(void) strcpy(format,            FormatDirect);
	(void) strcpy(conversion_format, FpaCblank);
	(void) strcpy(lat,               FpaCblank);
	(void) strcpy(lon,               FpaCblank);
	(void) strcpy(map_x,             FpaCblank);
	(void) strcpy(map_y,             FpaCblank);
	(void) strcpy(location_ident,    FpaCblank);
	(void) strcpy(location_look_up,  FpaCblank);
	(void) strcpy(vtime,             TVstamp);
	(void) strcpy(table_name,        FpaCblank);
	(void) strcpy(grid_name,         FpaCblank);
	(void) strcpy(list_name,         FpaCblank);
	txt_size   = size;
	rotation   =  0.0;
	rotate_lat = FALSE;
	rotate_lon = FALSE;
	map_units  = BaseMap.definition.units;
	xoff       =  0.0;
	yoff       =  0.0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "text_file") )
			{
			(void) strcpy(text_file, find_text_file(action));
			}

		else if ( same(key, "string") )
			{
			(void) strcpy(string, action);
			}

		else if ( same(key, "attribute") )
			{
			(void) strcpy(attribute, action);
			}

		else if ( same(key, "format") )
			{
			(void) strcpy(format, string_arg(action));
			if ( !blank(action) )
				{
				(void) no_white(action);
				(void) strcpy(conversion_format, action);
				}
			}

		else if ( same(key, "lat") )
			{
			(void) strcpy(lat, action);
			}

		else if ( same(key, "lon") )
			{
			(void) strcpy(lon, action);
			}

		else if ( same(key, "map_x") )
			{
			(void) strcpy(map_x, action);
			}

		else if ( same(key, "map_y") )
			{
			(void) strcpy(map_y, action);
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
			(void) strcpy(location_ident, action);
			}

		else if ( same(key, "location_look_up") )
			{
			(void) strcpy(location_look_up, action);
			}

		else if ( same(key, "valid_time") )
			{
			(void) grid_center(&BaseMap, NullPointPtr, NullFloat, &clon);
			(void) strcpy(vtime, interpret_timestring(action, T0stamp, clon));
			}

		else if ( same(key, "table_name") )
			{
			(void) strcpy(table_name, action);
			}

		else if ( same(key, "grid_name") )
			{
			(void) strcpy(grid_name, action);
			}

		else if ( same(key, "list_name") )
			{
			(void) strcpy(list_name, action);
			}

		else if ( same(key, "text_size") )
			{
			(void) sscanf(action, "%f", &txt_size);
			}

		else if ( same(key, "rotation") )
			{
			(void) sscanf(action, "%f", &rotation);
			}

		else if ( same(key, "rotate_to_latitude") )
			{
			if ( same_ic(action, LogicalYes) ) rotate_lat = TRUE;
			else                               rotate_lat = FALSE;
			}

		else if ( same(key, "rotate_to_longitude") )
			{
			if ( same_ic(action, LogicalYes) ) rotate_lon = TRUE;
			else                               rotate_lon = FALSE;
			}

		else if ( same(key, "x_off") )
			{
			(void) sscanf(action, "%f", &xoff);
			}

		else if ( same(key, "y_off") )
			{
			(void) sscanf(action, "%f", &yoff);
			}

		/* Skip valid presentation keywords already processed */
		else if ( same(key,  "line_width")
				|| same(key, "line_style")
				|| same(key, "outline")
				|| same(key, "fill")
				|| same(key, "outline_first")
				|| same(key, "font")
				|| same(key, "font_weight")
				|| same(key, "italics")
				|| same(key, "justification")
				|| same(key, "char_space")
				|| same(key, "word_space")
				|| same(key, "line_space") )
			{
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Error return for missing parameters */
	if ( blank(text_file) && blank(string) && blank(attribute) )
			(void) error_report("No text file or string or attribute");

	/* Error return for incorrect "format" or "conversion_format" */
	if ( !same(format, FormatDirect) )
		{
		(void) sprintf(err_buf,
				"Recognized format types are: %s", FormatDirect);
		(void) error_report(err_buf);
		}
	if ( !blank(conversion_format) )
		{
		if ( IsNull(strchr(conversion_format, ConFormat)) )
			{
			(void) sprintf(err_buf,
					"Error in text conversion format: %s  for attribute: %s",
					conversion_format, attribute);
			(void) error_report(err_buf);
			}
		}

	/* Error if both "rotate_to_latitude" and "rotate_to_longitude" set */
	if ( rotate_lat && rotate_lon )
		(void) error_report("Can set only one of \"rotate_to_latitude\" \"rotate_to_longitude\"");

	/* Now display the text file or string or attribute */
	return GRA_display_text(text_file, string, attribute, format,
			conversion_format, txt_size, rotation, rotate_lat, rotate_lon,
			xoff, yoff, lat, lon, map_x, map_y, map_units, location_ident,
			location_look_up, vtime, table_name, grid_name, list_name);
	}

/***********************************************************************
*                                                                      *
*    d o _ w r i t e _ t i m e                                         *
*                                                                      *
***********************************************************************/

LOGICAL		do_write_time

	(
	char		list[][GPGMedium],
	int			num
	)

	{
	LOGICAL		wtimeok;
	int			ii, nn;
	int			num_typfmt, num_typ, num_ztyp, num_zone, num_lang, num_fmt;
	TTYPFMT		*type_fmt;
	float		size, txt_size, rotation, xoff, yoff;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Set current text size */
	(void) sscanf(CurPres.text_size, "%f", &size);

	/* Initialize all parameters */
	num_typfmt = 0;
	num_typ    = 0;
	num_ztyp   = 0;
	num_zone   = 0;
	num_lang   = 0;
	num_fmt    = 0;
	txt_size   = size;
	rotation   =  0.0;
	xoff       =  0.0;
	yoff       =  0.0;

	/* Process all keywords in list */
	for ( ii=0; ii<num; ii++ )
		{
		(void) parse_instruction(list[ii], &key, &action);

		if ( same(key, "type") )
			{
			num_typfmt = add_time_types(&type_fmt, ++num_typ);
			(void) strcpy(type_fmt[num_typ-1].time_type, action);
			}

		else if ( same(key, "zone_type") )
			{
			num_typfmt = add_time_types(&type_fmt, ++num_ztyp);
			(void) strcpy(type_fmt[num_ztyp-1].zone_type, action);
			}

		/* >>> this gets replaced by "zone_type" above <<< */
		else if ( (OldVersion || ObsoleteVersion) && same(key, "time") )
			{
			(void) fprintf(stderr, ">>> Obsolete keyword in directive");
			(void) fprintf(stderr, " ... \"@write_time\" <<<\n");
			(void) fprintf(stderr, ">>>   Replace \"time\"");
			(void) fprintf(stderr, " by \"zone_type\" <<<\n");
			num_typfmt = add_time_types(&type_fmt, ++num_ztyp);
			(void) strcpy(type_fmt[num_ztyp-1].zone_type, action);
			}

		else if ( same(key, "time_zone") )
			{
			num_typfmt = add_time_types(&type_fmt, ++num_zone);
			(void) strcpy(type_fmt[num_zone-1].time_zone, action);
			}

		else if ( same(key, "language") )
			{
			num_typfmt = add_time_types(&type_fmt, ++num_lang);
			(void) strcpy(type_fmt[num_lang-1].language, action);
			}

		else if ( same(key, "format") )
			{
			num_typfmt = add_time_types(&type_fmt, ++num_fmt);
			(void) strcpy(type_fmt[num_fmt-1].time_format, action);
			}

		else if ( same(key, "text_size") )
			{
			(void) sscanf(action, "%f", &txt_size);
			}

		else if ( same(key, "rotation") )
			{
			(void) sscanf(action, "%f", &rotation);
			}

		else if ( same(key, "x_off") )
			{
			(void) sscanf(action, "%f", &xoff);
			}

		else if ( same(key, "y_off") )
			{
			(void) sscanf(action, "%f", &yoff);
			}

		/* Skip valid presentation keywords already processed */
		else if ( same(key,  "line_width")
				|| same(key, "line_style")
				|| same(key, "outline")
				|| same(key, "fill")
				|| same(key, "outline_first")
				|| same(key, "font")
				|| same(key, "font_weight")
				|| same(key, "italics")
				|| same(key, "justification")
				|| same(key, "char_space")
				|| same(key, "word_space")
				|| same(key, "line_space") )
			{
			}

		/* Error return for all unacceptable keywords */
		else
			{
			(void) sprintf(err_buf, "Unacceptable keyword: %s", list[ii]);
			(void) error_report(err_buf);
			}
		}

	/* Error return for missing parameters */
	if ( num_typfmt < 1 ) (void) error_report("No time parameters");

	/* Warning for multiple parameters in older versions */
	if ( (OldVersion || ObsoleteVersion) && num_typfmt > 1 )
		{
		(void) sprintf(err_buf, "May be problem with multiple \"type\" \"time\" \"time_zone\" \"language\" or \"format\" keywords with this version");
		(void) warn_report(err_buf);
		}

	/* Error return for missing or incorrect "type" or "zone_type" or "format" */
	for ( nn=0; nn<num_typfmt; nn++ )
		{
		if ( nn == 0 && blank(type_fmt[nn].time_type) )
			(void) error_report("Missing \"type\" keyword");
		if ( nn == 0 && blank(type_fmt[nn].zone_type) )
			(void) error_report("Missing \"zone_type\" keyword");
		if ( nn == 0 && blank(type_fmt[nn].time_format) )
			(void) error_report("Missing \"format\" keyword");
		if ( !blank(type_fmt[nn].time_type)
				&& !same_start_ic(type_fmt[nn].time_type, WriteTimeIssue)
				&& !same_start_ic(type_fmt[nn].time_type, WriteTimeValid)
				&& !same_start_ic(type_fmt[nn].time_type, WriteTimeCreation) )
			{
			(void) sprintf(err_buf, "Keyword \"type\" must be one of: %s %s %s",
					WriteTimeIssue, WriteTimeValid, WriteTimeCreation);
			(void) error_report(err_buf);
			}
		if ( !blank(type_fmt[nn].zone_type)
				&& !same_start_ic(type_fmt[nn].zone_type, WriteTimeGMT)
				&& !same_start_ic(type_fmt[nn].zone_type, WriteTimeUTC)
				&& !same_start_ic(type_fmt[nn].zone_type, WriteTimeLMT)
				&& !same_start_ic(type_fmt[nn].zone_type, WriteTimeLCL) )
			{
			(void) sprintf(err_buf, "Keyword \"zone_type\" must be one of: %s %s %s %s",
					WriteTimeGMT, WriteTimeUTC, WriteTimeLMT, WriteTimeLCL);
			(void) error_report(err_buf);
			}
		}

	/* Now display the time with the given format */
	wtimeok = GRA_display_time(type_fmt, num_typfmt,
				txt_size, rotation, xoff, yoff);

	/* Free time types and formats buffer */
	(void) free_time_types();

	/* Return results of time display */
	return wtimeok;
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
*    a d d _ g r a p h i c s _ f i e l d                               *
*    f r e e _ g r a p h i c s _ f i e l d                             *
*                                                                      *
***********************************************************************/

/* Storage for graphics fields */
static	int				NumGraphicsFlds = 0;
static	GRA_FLD_INFO	*GraphicsFlds   = NullPtr(GRA_FLD_INFO *);

static	int			add_graphics_field

	(
	GRA_FLD_INFO	**graphics_flds,
	int				num_fld
	)

	{
	int			nc;

	/* Return if space is already available */
	if ( num_fld <= NumGraphicsFlds ) return NumGraphicsFlds;

	/* Add another field buffer */
	nc = NumGraphicsFlds++;
	GraphicsFlds = GETMEM(GraphicsFlds, GRA_FLD_INFO, NumGraphicsFlds);

	/* Set first field buffer to default values */
	if ( NumGraphicsFlds == 1 )
		{
		(void) strcpy(GraphicsFlds[nc].element,  NoGraFldInfo.element);
		(void) strcpy(GraphicsFlds[nc].level,    NoGraFldInfo.level);
		(void) strcpy(GraphicsFlds[nc].equation, NoGraFldInfo.equation);
		}

	/* Set all other field buffers from previous field buffer */
	else
		{
		(void) strcpy(GraphicsFlds[nc].element,  GraphicsFlds[nc-1].element);
		(void) strcpy(GraphicsFlds[nc].level,    GraphicsFlds[nc-1].level);
		(void) strcpy(GraphicsFlds[nc].equation, GraphicsFlds[nc-1].equation);
		}

	/* Return the number of field buffers */
	if ( NotNull(graphics_flds) ) *graphics_flds = GraphicsFlds;
	return NumGraphicsFlds;
	}

static	void		free_graphics_field

	(
	)

	{

	/* Return if no field buffers to free */
	if ( NumGraphicsFlds <= 0 ) return;

	/* Free space used by field buffers */
	FREEMEM(GraphicsFlds);

	/* Reset the number of field buffers */
	NumGraphicsFlds = 0;
	}

/***********************************************************************
*                                                                      *
*    a d d _ g r a p h i c s _ i m a g e                               *
*    f r e e _ g r a p h i c s _ i m a g e                             *
*                                                                      *
***********************************************************************/

/* Storage for graphics images */
static	int				NumGraphicsIms = 0;
static	GRA_IMAGE_INFO	*GraphicsIms   = NullPtr(GRA_IMAGE_INFO *);

static	int			add_graphics_image

	(
	GRA_IMAGE_INFO	**graphics_images,
	int				num_image
	)

	{
	int			nc;

	/* Return if space is already available */
	if ( num_image <= NumGraphicsIms ) return NumGraphicsIms;

	/* Add another image buffer */
	nc = NumGraphicsIms++;
	GraphicsIms = GETMEM(GraphicsIms, GRA_IMAGE_INFO, NumGraphicsIms);

	/* Set all image buffers to default values */
	(void) strcpy(GraphicsIms[nc].image_tag,  NoGraImageInfo.image_tag);
	(void) strcpy(GraphicsIms[nc].ctable,     NoGraImageInfo.ctable);
	(void) strcpy(GraphicsIms[nc].brightness, NoGraImageInfo.brightness);

	/* Return the number of image buffers */
	if ( NotNull(graphics_images) ) *graphics_images = GraphicsIms;
	return NumGraphicsIms;
	}

static	void		free_graphics_image

	(
	)

	{

	/* Return if no image buffers to free */
	if ( NumGraphicsIms <= 0 ) return;

	/* Free space used by image buffers */
	FREEMEM(GraphicsIms);

	/* Reset the number of image buffers */
	NumGraphicsIms = 0;
	}

/***********************************************************************
*                                                                      *
*    a d d _ l i s t _ c a s e                                         *
*    f r e e _ l i s t _ c a s e                                       *
*                                                                      *
***********************************************************************/

/* Storage for special cases */
static	int			NumListCase = 0;
static	SPCASE		*ListCase   = NullPtr(SPCASE *);

static	int			add_list_case

	(
	SPCASE		**list_case,
	int			num_in
	)

	{
	int			nc;

	/* Return if space is already available */
	if ( num_in <= NumListCase ) return NumListCase;

	/* Add another special case to the list */
	nc = NumListCase++;
	ListCase = GETMEM(ListCase, SPCASE, NumListCase);

	/* Set defaults for first special case                                */
	/* Note that special case presentation is set to current presentation */
	if ( NumListCase == 1 )
		{
		(void) strcpy(ListCase[nc].spcase, FpaCblank);
		(void) strcpy(ListCase[nc].lookup, FpaCblank);
		(void) copy_presentation(&ListCase[nc].pres, &CurPres);
		}

	/* Set all other defaults from previous special case values */
	else
		{
		(void) strcpy(ListCase[nc].spcase, ListCase[nc-1].spcase);
		(void) strcpy(ListCase[nc].lookup, ListCase[nc-1].lookup);
		(void) copy_presentation(&ListCase[nc].pres, &ListCase[nc-1].pres);
		}

	/* Return the number of special cases in the list */
	if ( NotNull(list_case) ) *list_case = ListCase;
	return NumListCase;
	}

static	void		free_list_case

	(
	)

	{

	/* Return if no special cases to free */
	if ( NumListCase <= 0 ) return;

	/* Free space used by special cases */
	FREEMEM(ListCase);

	/* Reset the number of special cases */
	NumListCase = 0;
	}

/***********************************************************************
*                                                                      *
*    a d d _ x s e c t _ l o c a t i o n                               *
*    f r e e _ x s e c t _ l o c a t i o n s                           *
*                                                                      *
***********************************************************************/

/* Storage for cross section locations */
static	int				NumXsectLoc = 0;
static	XSECT_LOCATION	*XsectLocs  = NullPtr(XSECT_LOCATION *);

static	int			add_xsect_location

	(
	XSECT_LOCATION	**xsect_locs
	)

	{
	int			nc;

	/* Add another cross section location to the list */
	nc = NumXsectLoc++;
	XsectLocs = GETMEM(XsectLocs, XSECT_LOCATION, NumXsectLoc);

	/* Initialize the cross section location time */
	(void) strcpy(XsectLocs[nc].vtime, FpaCblank);

	/* Return the number of cross section locations in the list */
	if ( NotNull(xsect_locs) ) *xsect_locs = XsectLocs;
	return NumXsectLoc;
	}

static	void		free_xsect_locations

	(
	)

	{

	/* Return if no cross section locations to free */
	if ( NumXsectLoc <= 0 ) return;

	/* Free space used by cross section locations */
	FREEMEM(XsectLocs);

	/* Reset the number of cross section locations */
	NumXsectLoc = 0;
	}
