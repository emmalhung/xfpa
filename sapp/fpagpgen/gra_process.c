/***********************************************************************
*                                                                      *
*     g r a _ p r o c e s s . c                                        *
*                                                                      *
*     Routines to process all pdf file directives                      *
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

/***********************************************************************
*                                                                      *
*    p r o c e s s _ p s m e t _ d i r e c t i v e                     *
*                                                                      *
*    p r o c e s s _ s v g m e t _ d i r e c t i v e                   *
*                                                                      *
*    p r o c e s s _ c o r m e t _ d i r e c t i v e                   *
*                                                                      *
*    p r o c e s s _ t e x m e t _ d i r e c t i v e                   *
*                                                                      *
***********************************************************************/

void		process_psmet_directive

	(
	char		*buf,
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Initialize contour presentation and return immediately */
	if ( same(buf, "@reset_contour_presentation") )
		{
		(void) initialize_contour_presentation();
		return;
		}

	/* Initialize geographic presentation and return immediately */
	if ( same(buf, "@reset_geo_presentation") )
		{
		(void) initialize_geo_presentation();
		return;
		}

	/* Initialize the current presentation parameters */
	(void) copy_presentation(&CurPres, &PresDef);

	/* First scan keyword list for any local presentation keywords */
	/*  ... but only for specific directives!                      */
	if ( same(buf, "@add")
			|| same(buf, "@background")
			|| same(buf, "@box")
			|| same(buf, "@cross_section_axis_labels")
			|| same(buf, "@draw_distance_scale")
			|| same(buf, "@distance_scale_ticks")
			|| same(buf, "@distance_scale_labels")
			|| same(buf, "@ellipse")
			|| same(buf, "@label")
			|| same(buf, "@lchain_nodes")
			|| same(buf, "@legend")
			|| same(buf, "@sample_field")
			|| same(buf, "@sample_wind")
			|| same(buf, "@table_site")
			|| same(buf, "@text")
			|| same(buf, "@write_time") )
		{
		for ( ii=0; ii<num; ii++ )
			{

			(void) parse_instruction(list[ii], &key, &action);

			if ( same(key, "line_width") )
				{
				(void) strcpy(CurPres.line_width, action);
				}

			else if ( same(key, "line_style") )
				{
				(void) strcpy(CurPres.line_style, action);
				}

			else if ( same(key, "outline") )
				{
				(void) strcpy(CurPres.outline, action);
				}

			else if ( same(key, "fill") )
				{
				(void) strcpy(CurPres.fill, action);
				}

			else if ( same(key, "outline_first") )
				{
				if ( same_ic(action, LogicalYes) )
					{
					CurPres.outline_first = TRUE;
					}
				else
					{
					CurPres.outline_first = FALSE;
					}
				}

			else if ( same(key, "interior_fill") )
				{
				(void) strcpy(CurPres.interior_fill, action);
				}

			else if ( same(key, "font") )
				{
				(void) strcpy(CurPres.font, action);
				}

			else if ( same(key, "font_weight") )
				{
				(void) strcpy(CurPres.font_weight, action);
				}

			else if ( same(key, "italics") )
				{
				(void) strcpy(CurPres.italics, action);
				}

			else if ( same(key, "justification") )
				{
				(void) strcpy(CurPres.justified, action);
				}

			else if ( same(key, "text_size") )
				{
				(void) strcpy(CurPres.text_size, action);
				}

			else if ( same(key, "char_space") )
				{
				(void) strcpy(CurPres.char_space, action);
				}

			else if ( same(key, "word_space") )
				{
				(void) strcpy(CurPres.word_space, action);
				}

			else if ( same(key, "line_space") )
				{
				(void) strcpy(CurPres.line_space, action);
				}
			}
		}

	/* Now process each of the directives */

	if ( same(buf,					"@add") )
		{
		if ( !do_add(list, num) )
			(void) error_report("Error processing @add");
		}

	else if ( same(buf,				"@anchor") )
		{
		if ( !set_anchor(list, num) )
			(void) error_report("Error processing @anchor");
		}

	else if ( same(buf,				"@areas") )
		{
		if ( !do_areas(list, num) )
			(void) error_report("Error processing @areas");
		}

	else if ( same(buf,				"@arrow_display") )
		{
		if ( !set_arrow_display(list, num) )
			(void) error_report("Error processing @arrow_display");
		}

	else if ( same(buf,				"@background") )
		{
		if ( !do_background(list, num) )
			(void) error_report("Error processing @background");
		}

	else if ( same(buf,				"@box") )
		{
		if ( !do_box(list, num) )
			(void) error_report("Error processing @box");
		}

	else if ( same(buf,				"@contour_presentation") )
		{
		if ( !set_contour_presentation(list, num) )
			(void) error_report("Error processing @contour_presentation");
		}

	else if ( same(buf,				"@contours") )
		{
		if ( !do_contours(list, num) )
			(void) error_report("Error processing @contours");
		}

	else if ( same(buf,				"@cross_section_areas") )
		{
		if ( !do_cross_section_areas(list, num) )
			(void) error_report("Error processing @cross_section_areas");
		}

	else if ( same(buf,				"@cross_section_axis_labels") )
		{
		if ( !do_cross_section_axis_labels(list, num) )
			(void) error_report("Error processing @cross_section_axis_labels");
		}

	else if ( same(buf,				"@cross_section_contours") )
		{
		if ( !do_cross_section_contours(list, num) )
			(void) error_report("Error processing @cross_section_contours");
		}

	else if ( same(buf,				"@cross_section_curves") )
		{
		if ( !do_cross_section_curves(list, num) )
			(void) error_report("Error processing @cross_section_curves");
		}

	else if ( same(buf,				"@define_cross_section") )
		{
		if ( !set_define_cross_section(list, num) )
			(void) error_report("Error processing @define_cross_section");
		}

	else if ( same(buf,				"@define_line") )
		{
		if ( !set_define_line(list, num) )
			(void) error_report("Error processing @define_line");
		}

	else if ( same(buf,				"@define_map_placement") )
		{
		if ( !set_define_map_placement(list, num) )
			(void) error_report("Error processing @define_map_placement");
		}

	else if ( same(buf,				"@define_sample_grid") )
		{
		if ( !set_define_sample_grid(list, num) )
			(void) error_report("Error processing @define_sample_grid");
		}

	else if ( same(buf,				"@define_sample_list") )
		{
		if ( !set_define_sample_list(list, num) )
			(void) error_report("Error processing @define_sample_list");
		}

	else if ( same(buf,				"@define_table") )
		{
		if ( !set_define_table(list, num) )
			(void) error_report("Error processing @define_table");
		}

	else if ( same(buf,				"@display_units") )
		{
		if ( !set_display_units(list, num) )
			(void) error_report("Error processing @display_units");
		}

	/* >>> change this to  @geography  (below) <<< */
	else if ( ObsoleteVersion && same(buf, "@do_geography") )
		{
		(void) fprintf(stderr, ">>> Obsolete directive ...");
		(void) fprintf(stderr, " \"@do_geography\" <<<\n");
		(void) fprintf(stderr, ">>>  Replace by directive ...");
		(void) fprintf(stderr, " \"@geography\" <<<\n");
		if ( !do_geography(list, num) )
			(void) error_report("Error processing @do_geography");
		}

	else if ( same(buf,				"@draw_line") )
		{
		if ( !do_draw_line(list, num) )
			(void) error_report("Error processing @draw_line");
		}

	else if ( same(buf,				"@draw_cross_section_line") )
		{
		if ( !do_draw_cross_section_line(list, num) )
			(void) error_report("Error processing @draw_cross_section_line");
		}

	else if ( same(buf,				"@draw_table_line") )
		{
		if ( !do_draw_table_line(list, num) )
			(void) error_report("Error processing @draw_table_line");
		}

	else if ( same(buf,				"@draw_distance_scale") )
		{
		if ( !do_draw_distance_scale(list, num) )
			(void) error_report("Error processing @draw_distance_scale");
		}

	else if ( same(buf,				"@distance_scale_ticks") )
		{
		if ( !do_distance_scale_ticks(list, num) )
			(void) error_report("Error processing @distance_scale_ticks");
		}

	else if ( same(buf,				"@distance_scale_labels") )
		{
		if ( !do_distance_scale_labels(list, num) )
			(void) error_report("Error processing @distance_scale_labels");
		}

	else if ( same(buf,				"@ellipse") )
		{
		if ( !do_ellipse(list, num) )
			(void) error_report("Error processing @ellipse");
		}

	else if ( same(buf,				"@filter") )
		{
		if ( !set_filter(list, num) )
			(void) error_report("Error processing @filter");
		}

	else if ( same(buf,				"@geo_presentation") )
		{
		if ( !set_geo_presentation(list, num) )
			(void) error_report("Error processing @geo_presentation");
		}

	else if ( same(buf,				"@geography") )
		{
		if ( !do_geography(list, num) )
			(void) error_report("Error processing @geography");
		}

	else if ( same(buf,				"@gpgen_group_begin") )
		{
		if ( !set_group_begin(list, num) )
			(void) error_report("Error processing @gpgen_group_begin");
		}

	else if ( same(buf,				"@gpgen_group_end") )
		{
		(void) write_graphics_group(GPGend, NullPointer, 0);
		}

	else if ( same(buf,				"@images") )
		{
		if ( !do_images(list, num) )
			(void) error_report("Error processing @images");
		}

	else if ( same(buf,				"@label") )
		{
		if ( !do_label(list, num) )
			(void) error_report("Error processing @label");
		}

	else if ( same(buf,				"@label_display") )
		{
		if ( !set_label_display(list, num) )
			(void) error_report("Error processing @label_display");
		}

	else if ( same(buf,				"@lchain_nodes") )
		{
		if ( !do_lchain_nodes(list, num) )
			(void) error_report("Error processing @lchain_nodes");
		}

	else if ( same(buf,				"@lchain_tracks") )
		{
		if ( !do_lchain_tracks(list, num) )
			(void) error_report("Error processing @lchain_tracks");
		}

	else if ( same(buf,				"@legend") )
		{
		if ( !do_legend(list, num) )
			(void) error_report("Error processing @legend");
		}

	else if ( same(buf,				"@lines") )
		{
		if ( !do_lines(list, num) )
			(void) error_report("Error processing @lines");
		}

	else if ( same(buf,				"@mapdef") )
		{
		if ( !set_mapdef(list, num) )
			(void) error_report("Error processing @mapdef");
		}

	else if ( same(buf,				"@perspective_view") )
		{
		if ( !set_perspective_view(list, num) )
			(void) error_report("Error processing @perspective_view");
		}

	else if ( (OldVersion || ObsoleteVersion) && same(buf, "@plot") )
		{
		(void) fprintf(stderr, ">>> Obsolete directive ...");
		(void) fprintf(stderr, " \"@plot\" <<<\n");
		(void) fprintf(stderr, ">>>  Replace by directive ...");
		(void) fprintf(stderr, " \"@label\" <<<\n");
		if ( !do_plot(list, num) )
			(void) error_report("Error processing @plot");
		}

	else if ( same(buf,				"@presentation") )
		{
		if ( !set_presentation(list, num) )
			(void) error_report("Error processing @presentation");
		}

	else if ( same(buf,				"@projection") )
		{
		if ( !set_projection(list, num) )
			(void) error_report("Error processing @projection");
		}

	else if ( same(buf,				"@resolution") )
		{
		if ( !set_resolution(list, num) )
			(void) error_report("Error processing @resolution");
		}

	else if ( same(buf,				"@sample_field") )
		{
		if ( !do_sample_field(list, num) )
			(void) error_report("Error processing @sample_field");
		}

	else if ( same(buf,				"@sample_wind") )
		{
		if ( !do_sample_wind(list, num) )
			(void) error_report("Error processing @sample_wind");
		}

	else if ( same(buf,				"@set_source") )
		{
		if ( !set_source(list, num) )
			(void) error_report("Error processing @set_source");
		}

	else if ( same(buf,				"@size") )
		{
		if ( !set_size(list, num) )
			(void) error_report("Error processing @size");
		}

	else if ( same(buf,				"@symbol_fill_display") )
		{
		if ( !set_symbol_fill(list, num) )
			(void) error_report("Error processing @symbol_fill_display");
		}

	else if ( same(buf,				"@table_site") )
		{
		if ( !do_table_site(list, num) )
			(void) error_report("Error processing @table_site");
		}

	else if ( same(buf,				"@text") )
		{
		if ( !do_text(list, num) )
			(void) error_report("Error processing @text");
		}

	else if ( same(buf,				"@vector_presentation") )
		{
		if ( !set_vector_presentation(list, num) )
			(void) error_report("Error processing @vector_presentation");
		}

	else if ( same(buf,				"@verbose") )
		{
		if ( !set_verbose(list, num) )
			(void) error_report("Error processing @verbose");
		}

	else if ( same(buf,				"@wind_barb_presentation") )
		{
		if ( !set_wind_barb_presentation(list, num) )
			(void) error_report("Error processing @wind_barb_presentation");
		}

	/* >>> change this to  @wind_presentation  (below) <<< */
	else if ( ObsoleteVersion && same(buf, "@wind_text_presentation") )
		{
		(void) fprintf(stderr, ">>> Obsolete directive ...");
		(void) fprintf(stderr, " \"@wind_text_presentation\" <<<\n");
		(void) fprintf(stderr, ">>>  Replace by directive ...");
		(void) fprintf(stderr, " \"@wind_presentation\"   <<<\n");
		if ( !set_wind_presentation(list, num) )
			(void) error_report("Error processing @wind_text_presentation");
		}

	else if ( same(buf,				"@wind_presentation") )
		{
		if ( !set_wind_presentation(list, num) )
			(void) error_report("Error processing @wind_presentation");
		}

	else if ( same(buf,				"@write_time") )
		{
		if ( !do_write_time(list, num) )
			(void) error_report("Error processing @write_time");
		}

	else
		{
		(void) sprintf(err_buf, "Unrecognized directive: \"%s\"", buf);
		(void) error_report(err_buf);
		}

	return;
	}

void		process_svgmet_directive

	(
	char		*buf,
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Initialize contour presentation and return immediately */
	if ( same(buf, "@reset_contour_presentation") )
		{
		(void) initialize_contour_presentation();
		return;
		}

	/* Initialize geographic presentation and return immediately */
	if ( same(buf, "@reset_geo_presentation") )
		{
		(void) initialize_geo_presentation();
		return;
		}

	/* Initialize the current presentation parameters */
	(void) copy_presentation(&CurPres, &PresDef);

	/* First scan keyword list for any local presentation keywords */
	/*  ... but only for specific directives!                      */
	if ( same(buf, "@add")
			|| same(buf, "@background")
			|| same(buf, "@box")
			|| same(buf, "@cross_section_axis_labels")
			|| same(buf, "@draw_distance_scale")
			|| same(buf, "@distance_scale_ticks")
			|| same(buf, "@distance_scale_labels")
			|| same(buf, "@ellipse")
			|| same(buf, "@label")
			|| same(buf, "@lchain_nodes")
			|| same(buf, "@legend")
			|| same(buf, "@sample_field")
			|| same(buf, "@sample_wind")
			|| same(buf, "@table_site")
			|| same(buf, "@text")
			|| same(buf, "@write_time") )
		{
		for ( ii=0; ii<num; ii++ )
			{

			(void) parse_instruction(list[ii], &key, &action);

			if ( same(key, "line_width") )
				{
				(void) strcpy(CurPres.line_width, action);
				}

			else if ( same(key, "line_style") )
				{
				(void) strcpy(CurPres.line_style, action);
				}

			else if ( same(key, "outline") )
				{
				(void) strcpy(CurPres.outline, action);
				}

			else if ( same(key, "fill") )
				{
				(void) strcpy(CurPres.fill, action);
				}

			else if ( same(key, "outline_first") )
				{
				if ( same_ic(action, LogicalYes) )
					{
					CurPres.outline_first = TRUE;
					}
				else
					{
					CurPres.outline_first = FALSE;
					}
				}

			else if ( same(key, "interior_fill") )
				{
				(void) strcpy(CurPres.interior_fill, action);
				}

			else if ( same(key, "font") )
				{
				(void) strcpy(CurPres.font, action);
				}

			else if ( same(key, "font_weight") )
				{
				(void) strcpy(CurPres.font_weight, action);
				}

			else if ( same(key, "italics") )
				{
				(void) strcpy(CurPres.italics, action);
				}

			else if ( same(key, "justification") )
				{
				(void) strcpy(CurPres.justified, action);
				}

			else if ( same(key, "text_size") )
				{
				(void) strcpy(CurPres.text_size, action);
				}

			else if ( same(key, "char_space") )
				{
				(void) strcpy(CurPres.char_space, action);
				}

			else if ( same(key, "word_space") )
				{
				(void) strcpy(CurPres.word_space, action);
				}

			else if ( same(key, "line_space") )
				{
				(void) strcpy(CurPres.line_space, action);
				}
			}
		}

	/* Now process each of the directives */

	if ( same(buf,					"@add") )
		{
		if ( !do_add(list, num) )
			(void) error_report("Error processing @add");
		}

	else if ( same(buf,				"@anchor") )
		{
		if ( !set_anchor(list, num) )
			(void) error_report("Error processing @anchor");
		}

	else if ( same(buf,				"@areas") )
		{
		if ( !do_areas(list, num) )
			(void) error_report("Error processing @areas");
		}

	else if ( same(buf,				"@arrow_display") )
		{
		if ( !set_arrow_display(list, num) )
			(void) error_report("Error processing @arrow_display");
		}

	else if ( same(buf,				"@background") )
		{
		if ( !do_background(list, num) )
			(void) error_report("Error processing @background");
		}

	else if ( same(buf,				"@box") )
		{
		if ( !do_box(list, num) )
			(void) error_report("Error processing @box");
		}

	else if ( same(buf,				"@contour_presentation") )
		{
		if ( !set_contour_presentation(list, num) )
			(void) error_report("Error processing @contour_presentation");
		}

	else if ( same(buf,				"@contours") )
		{
		if ( !do_contours(list, num) )
			(void) error_report("Error processing @contours");
		}

	else if ( same(buf,				"@cross_section_areas") )
		{
		if ( !do_cross_section_areas(list, num) )
			(void) error_report("Error processing @cross_section_areas");
		}

	else if ( same(buf,				"@cross_section_axis_labels") )
		{
		if ( !do_cross_section_axis_labels(list, num) )
			(void) error_report("Error processing @cross_section_axis_labels");
		}

	else if ( same(buf,				"@cross_section_contours") )
		{
		if ( !do_cross_section_contours(list, num) )
			(void) error_report("Error processing @cross_section_contours");
		}

	else if ( same(buf,				"@cross_section_curves") )
		{
		if ( !do_cross_section_curves(list, num) )
			(void) error_report("Error processing @cross_section_curves");
		}

	else if ( same(buf,				"@define_cross_section") )
		{
		if ( !set_define_cross_section(list, num) )
			(void) error_report("Error processing @define_cross_section");
		}

	else if ( same(buf,				"@define_line") )
		{
		if ( !set_define_line(list, num) )
			(void) error_report("Error processing @define_line");
		}

	else if ( same(buf,				"@define_map_placement") )
		{
		if ( !set_define_map_placement(list, num) )
			(void) error_report("Error processing @define_map_placement");
		}

	else if ( same(buf,				"@define_sample_grid") )
		{
		if ( !set_define_sample_grid(list, num) )
			(void) error_report("Error processing @define_sample_grid");
		}

	else if ( same(buf,				"@define_sample_list") )
		{
		if ( !set_define_sample_list(list, num) )
			(void) error_report("Error processing @define_sample_list");
		}

	else if ( same(buf,				"@define_table") )
		{
		if ( !set_define_table(list, num) )
			(void) error_report("Error processing @define_table");
		}

	else if ( same(buf,				"@display_units") )
		{
		if ( !set_display_units(list, num) )
			(void) error_report("Error processing @display_units");
		}

	else if ( same(buf,				"@draw_line") )
		{
		if ( !do_draw_line(list, num) )
			(void) error_report("Error processing @draw_line");
		}

	else if ( same(buf,				"@draw_cross_section_line") )
		{
		if ( !do_draw_cross_section_line(list, num) )
			(void) error_report("Error processing @draw_cross_section_line");
		}

	else if ( same(buf,				"@draw_table_line") )
		{
		if ( !do_draw_table_line(list, num) )
			(void) error_report("Error processing @draw_table_line");
		}

	else if ( same(buf,				"@draw_distance_scale") )
		{
		if ( !do_draw_distance_scale(list, num) )
			(void) error_report("Error processing @draw_distance_scale");
		}

	else if ( same(buf,				"@distance_scale_ticks") )
		{
		if ( !do_distance_scale_ticks(list, num) )
			(void) error_report("Error processing @distance_scale_ticks");
		}

	else if ( same(buf,				"@distance_scale_labels") )
		{
		if ( !do_distance_scale_labels(list, num) )
			(void) error_report("Error processing @distance_scale_labels");
		}

	else if ( same(buf,				"@ellipse") )
		{
		if ( !do_ellipse(list, num) )
			(void) error_report("Error processing @ellipse");
		}

	else if ( same(buf,				"@filter") )
		{
		if ( !set_filter(list, num) )
			(void) error_report("Error processing @filter");
		}

	else if ( same(buf,				"@geo_presentation") )
		{
		if ( !set_geo_presentation(list, num) )
			(void) error_report("Error processing @geo_presentation");
		}

	else if ( same(buf,				"@geography") )
		{
		if ( !do_geography(list, num) )
			(void) error_report("Error processing @geography");
		}

	else if ( same(buf,				"@gpgen_group_begin") )
		{
		if ( !set_group_begin(list, num) )
			(void) error_report("Error processing @gpgen_group_begin");
		}

	else if ( same(buf,				"@gpgen_group_end") )
		{
		(void) write_graphics_group(GPGend, NullPointer, 0);
		}

	else if ( same(buf,				"@images") )
		{
		if ( !do_images(list, num) )
			(void) error_report("Error processing @images");
		}

	else if ( same(buf,				"@label") )
		{
		if ( !do_label(list, num) )
			(void) error_report("Error processing @label");
		}

	else if ( same(buf,				"@label_display") )
		{
		if ( !set_label_display(list, num) )
			(void) error_report("Error processing @label_display");
		}

	else if ( same(buf,				"@lchain_nodes") )
		{
		if ( !do_lchain_nodes(list, num) )
			(void) error_report("Error processing @lchain_nodes");
		}

	else if ( same(buf,				"@lchain_tracks") )
		{
		if ( !do_lchain_tracks(list, num) )
			(void) error_report("Error processing @lchain_tracks");
		}

	else if ( same(buf,				"@legend") )
		{
		if ( !do_legend(list, num) )
			(void) error_report("Error processing @legend");
		}

	else if ( same(buf,				"@lines") )
		{
		if ( !do_lines(list, num) )
			(void) error_report("Error processing @lines");
		}

	else if ( same(buf,				"@mapdef") )
		{
		if ( !set_mapdef(list, num) )
			(void) error_report("Error processing @mapdef");
		}

	else if ( same(buf,				"@perspective_view") )
		{
		if ( !set_perspective_view(list, num) )
			(void) error_report("Error processing @perspective_view");
		}

	else if ( same(buf,				"@presentation") )
		{
		if ( !set_presentation(list, num) )
			(void) error_report("Error processing @presentation");
		}

	else if ( same(buf,				"@projection") )
		{
		if ( !set_projection(list, num) )
			(void) error_report("Error processing @projection");
		}

	else if ( same(buf,				"@resolution") )
		{
		if ( !set_resolution(list, num) )
			(void) error_report("Error processing @resolution");
		}

	else if ( same(buf,				"@sample_field") )
		{
		if ( !do_sample_field(list, num) )
			(void) error_report("Error processing @sample_field");
		}

	else if ( same(buf,				"@sample_wind") )
		{
		if ( !do_sample_wind(list, num) )
			(void) error_report("Error processing @sample_wind");
		}

	else if ( same(buf,				"@set_source") )
		{
		if ( !set_source(list, num) )
			(void) error_report("Error processing @set_source");
		}

	else if ( same(buf,				"@size") )
		{
		if ( !set_size(list, num) )
			(void) error_report("Error processing @size");
		}

	else if ( same(buf,				"@symbol_fill_display") )
		{
		if ( !set_symbol_fill(list, num) )
			(void) error_report("Error processing @symbol_fill_display");
		}

	else if ( same(buf,				"@table_site") )
		{
		if ( !do_table_site(list, num) )
			(void) error_report("Error processing @table_site");
		}

	else if ( same(buf,				"@text") )
		{
		if ( !do_text(list, num) )
			(void) error_report("Error processing @text");
		}

	else if ( same(buf,				"@vector_presentation") )
		{
		if ( !set_vector_presentation(list, num) )
			(void) error_report("Error processing @vector_presentation");
		}

	else if ( same(buf,				"@verbose") )
		{
		if ( !set_verbose(list, num) )
			(void) error_report("Error processing @verbose");
		}

	else if ( same(buf,				"@wind_barb_presentation") )
		{
		if ( !set_wind_barb_presentation(list, num) )
			(void) error_report("Error processing @wind_barb_presentation");
		}

	else if ( same(buf,				"@wind_presentation") )
		{
		if ( !set_wind_presentation(list, num) )
			(void) error_report("Error processing @wind_presentation");
		}

	else if ( same(buf,				"@write_time") )
		{
		if ( !do_write_time(list, num) )
			(void) error_report("Error processing @write_time");
		}

	else
		{
		(void) sprintf(err_buf, "Unrecognized directive: \"%s\"", buf);
		(void) error_report(err_buf);
		}

	return;
	}

void		process_cormet_directive

	(
	char		*buf,
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Initialize contour presentation and return immediately */
	if ( same(buf, "@reset_contour_presentation") )
		{
		(void) initialize_contour_presentation();
		return;
		}

	/* Initialize geographic presentation and return immediately */
	if ( same(buf, "@reset_geo_presentation") )
		{
		(void) initialize_geo_presentation();
		return;
		}

	/* Initialize the current presentation parameters */
	(void) copy_presentation(&CurPres, &PresDef);

	/* First scan keyword list for any local presentation keywords */
	/*  ... but only for specific directives!                      */
	if ( same(buf, "@add")
			|| same(buf, "@background")
			|| same(buf, "@box")
			|| same(buf, "@cross_section_axis_labels")
			|| same(buf, "@draw_distance_scale")
			|| same(buf, "@distance_scale_ticks")
			|| same(buf, "@distance_scale_labels")
			|| same(buf, "@ellipse")
			|| same(buf, "@label")
			|| same(buf, "@legend")
			|| same(buf, "@sample_field")
			|| same(buf, "@sample_wind")
			|| same(buf, "@table_site")
			|| same(buf, "@text")
			|| same(buf, "@write_time") )
		{
		for ( ii=0; ii<num; ii++ )
			{

			(void) parse_instruction(list[ii], &key, &action);

			if ( same(key, "line_width") )
				{
				(void) strcpy(CurPres.line_width, action);
				}

			else if ( same(key, "line_style") )
				{
				(void) strcpy(CurPres.line_style, action);
				}

			else if ( same(key, "outline") )
				{
				(void) strcpy(CurPres.outline, action);
				}

			else if ( same(key, "fill") )
				{
				(void) strcpy(CurPres.fill, action);
				}

			else if ( same(key, "outline_first") )
				{
				if ( same_ic(action, LogicalYes) )
					{
					CurPres.outline_first = TRUE;
					}
				else
					{
					CurPres.outline_first = FALSE;
					}
				}

			else if ( same(key, "interior_fill") )
				{
				(void) strcpy(CurPres.interior_fill, action);
				}

			else if ( same(key, "font") )
				{
				(void) strcpy(CurPres.font, action);
				}

			else if ( same(key, "font_weight") )
				{
				(void) strcpy(CurPres.font_weight, action);
				}

			else if ( same(key, "italics") )
				{
				(void) strcpy(CurPres.italics, action);
				}

			else if ( same(key, "justification") )
				{
				(void) strcpy(CurPres.justified, action);
				}

			else if ( same(key, "text_size") )
				{
				(void) strcpy(CurPres.text_size, action);
				}

			else if ( same(key, "char_space") )
				{
				(void) strcpy(CurPres.char_space, action);
				}

			else if ( same(key, "word_space") )
				{
				(void) strcpy(CurPres.word_space, action);
				}

			else if ( same(key, "line_space") )
				{
				(void) strcpy(CurPres.line_space, action);
				}
			}
		}

	/* Now process each of the directives */

	if ( same(buf,					"@add") )
		{
		if ( !do_add(list, num) )
			(void) error_report("Error processing @add");
		}

	else if ( same(buf,				"@anchor") )
		{
		if ( !set_anchor(list, num) )
			(void) error_report("Error processing @anchor");
		}

	else if ( same(buf,				"@areas") )
		{
		if ( !do_areas(list, num) )
			(void) error_report("Error processing @areas");
		}

	else if ( same(buf,				"@arrow_display") )
		{
		if ( !set_arrow_display(list, num) )
			(void) error_report("Error processing @arrow_display");
		}

	else if ( same(buf,				"@background") )
		{
		if ( !do_background(list, num) )
			(void) error_report("Error processing @background");
		}

	else if ( same(buf,				"@box") )
		{
		if ( !do_box(list, num) )
			(void) error_report("Error processing @box");
		}

	/* >>> change this to  @gpgen_group_begin  (below) <<< */
	else if ( ObsoleteVersion && same(buf, "@cmf_group_start") )
		{
		(void) fprintf(stderr, ">>> Obsolete directive ...");
		(void) fprintf(stderr, " \"@cmf_group_start\" <<<\n");
		(void) fprintf(stderr, ">>>  Replace by directive ...");
		(void) fprintf(stderr, " \"@gpgen_group_begin\" <<<\n");
		(void) write_graphics_group(GPGstart, NullPointer, 0);
		}

	/* >>> change this to  @gpgen_group_end  (below) <<< */
	else if ( ObsoleteVersion && same(buf, "@cmf_group_end") )
		{
		(void) fprintf(stderr, ">>> Obsolete directive ...");
		(void) fprintf(stderr, " \"@cmf_group_end\" <<<\n");
		(void) fprintf(stderr, ">>>  Replace by directive ...");
		(void) fprintf(stderr, " \"@gpgen_group_end\" <<<\n");
		(void) write_graphics_group(GPGend, NullPointer, 0);
		}

	else if ( same(buf,				"@contour_presentation") )
		{
		if ( !set_contour_presentation(list, num) )
			(void) error_report("Error processing @contour_presentation");
		}

	else if ( same(buf,				"@contours") )
		{
		if ( !do_contours(list, num) )
			(void) error_report("Error processing @contours");
		}

	else if ( same(buf,				"@cross_section_areas") )
		{
		if ( !do_cross_section_areas(list, num) )
			(void) error_report("Error processing @cross_section_areas");
		}

	else if ( same(buf,				"@cross_section_axis_labels") )
		{
		if ( !do_cross_section_axis_labels(list, num) )
			(void) error_report("Error processing @cross_section_axis_labels");
		}

	else if ( same(buf,				"@cross_section_contours") )
		{
		if ( !do_cross_section_contours(list, num) )
			(void) error_report("Error processing @cross_section_contours");
		}

	else if ( same(buf,				"@cross_section_curves") )
		{
		if ( !do_cross_section_curves(list, num) )
			(void) error_report("Error processing @cross_section_curves");
		}

	else if ( same(buf,				"@define_cross_section") )
		{
		if ( !set_define_cross_section(list, num) )
			(void) error_report("Error processing @define_cross_section");
		}

	else if ( same(buf,				"@define_line") )
		{
		if ( !set_define_line(list, num) )
			(void) error_report("Error processing @define_line");
		}

	else if ( same(buf,				"@define_map_placement") )
		{
		if ( !set_define_map_placement(list, num) )
			(void) error_report("Error processing @define_map_placement");
		}

	else if ( same(buf,				"@define_sample_grid") )
		{
		if ( !set_define_sample_grid(list, num) )
			(void) error_report("Error processing @define_sample_grid");
		}

	else if ( same(buf,				"@define_sample_list") )
		{
		if ( !set_define_sample_list(list, num) )
			(void) error_report("Error processing @define_sample_list");
		}

	else if ( same(buf,				"@define_table") )
		{
		if ( !set_define_table(list, num) )
			(void) error_report("Error processing @define_table");
		}

	else if ( same(buf,				"@display_units") )
		{
		if ( !set_display_units(list, num) )
			(void) error_report("Error processing @display_units");
		}

	/* >>> change this to  @geography  (below) <<< */
	else if ( ObsoleteVersion && same(buf, "@do_geography") )
		{
		(void) fprintf(stderr, ">>> Obsolete directive ...");
		(void) fprintf(stderr, " \"@do_geography\" <<<\n");
		(void) fprintf(stderr, ">>>  Replace by directive ...");
		(void) fprintf(stderr, " \"@geography\" <<<\n");
		if ( !do_geography(list, num) )
			(void) error_report("Error processing @do_geography");
		}

	else if ( same(buf,				"@draw_line") )
		{
		if ( !do_draw_line(list, num) )
			(void) error_report("Error processing @draw_line");
		}

	else if ( same(buf,				"@draw_cross_section_line") )
		{
		if ( !do_draw_cross_section_line(list, num) )
			(void) error_report("Error processing @draw_cross_section_line");
		}

	else if ( same(buf,				"@draw_table_line") )
		{
		if ( !do_draw_table_line(list, num) )
			(void) error_report("Error processing @draw_table_line");
		}

	else if ( same(buf,				"@draw_distance_scale") )
		{
		if ( !do_draw_distance_scale(list, num) )
			(void) error_report("Error processing @draw_distance_scale");
		}

	else if ( same(buf,				"@distance_scale_ticks") )
		{
		if ( !do_distance_scale_ticks(list, num) )
			(void) error_report("Error processing @distance_scale_ticks");
		}

	else if ( same(buf,				"@distance_scale_labels") )
		{
		if ( !do_distance_scale_labels(list, num) )
			(void) error_report("Error processing @distance_scale_labels");
		}

	else if ( same(buf,				"@ellipse") )
		{
		if ( !do_ellipse(list, num) )
			(void) error_report("Error processing @ellipse");
		}

	else if ( same(buf,				"@filter") )
		{
		if ( !set_filter(list, num) )
			(void) error_report("Error processing @filter");
		}

	else if ( same(buf,				"@geo_presentation") )
		{
		if ( !set_geo_presentation(list, num) )
			(void) error_report("Error processing @geo_presentation");
		}

	else if ( same(buf,				"@geography") )
		{
		if ( !do_geography(list, num) )
			(void) error_report("Error processing @geography");
		}

	else if ( same(buf,				"@gpgen_group_begin") )
		{
		if ( !set_group_begin(list, num) )
			(void) error_report("Error processing @gpgen_group_begin");
		}

	else if ( same(buf,				"@gpgen_group_end") )
		{
		(void) write_graphics_group(GPGend, NullPointer, 0);
		}

	else if ( same(buf,				"@label") )
		{
		if ( !do_label(list, num) )
			(void) error_report("Error processing @label");
		}

	else if ( same(buf,				"@label_display") )
		{
		if ( !set_label_display(list, num) )
			(void) error_report("Error processing @label_display");
		}

	else if ( same(buf,				"@legend") )
		{
		if ( !do_legend(list, num) )
			(void) error_report("Error processing @legend");
		}

	else if ( same(buf,				"@lines") )
		{
		if ( !do_lines(list, num) )
			(void) error_report("Error processing @lines");
		}

	else if ( same(buf,				"@mapdef") )
		{
		if ( !set_mapdef(list, num) )
			(void) error_report("Error processing @mapdef");
		}

	else if ( same(buf,				"@perspective_view") )
		{
		if ( !set_perspective_view(list, num) )
			(void) error_report("Error processing @perspective_view");
		}

	else if ( (OldVersion || ObsoleteVersion) && same(buf, "@plot") )
		{
		(void) fprintf(stderr, ">>> Obsolete directive ...");
		(void) fprintf(stderr, " \"@plot\" <<<\n");
		(void) fprintf(stderr, ">>>  Replace by directive ...");
		(void) fprintf(stderr, " \"@label\" <<<\n");
		if ( !do_plot(list, num) )
			(void) error_report("Error processing @plot");
		}

	else if ( same(buf,				"@presentation") )
		{
		if ( !set_presentation(list, num) )
			(void) error_report("Error processing @presentation");
		}

	else if ( same(buf,				"@projection") )
		{
		if ( !set_projection(list, num) )
			(void) error_report("Error processing @projection");
		}

	else if ( same(buf,				"@resolution") )
		{
		if ( !set_resolution(list, num) )
			(void) error_report("Error processing @resolution");
		}

	else if ( same(buf,				"@sample_field") )
		{
		if ( !do_sample_field(list, num) )
			(void) error_report("Error processing @sample_field");
		}

	else if ( same(buf,				"@sample_wind") )
		{
		if ( !do_sample_wind(list, num) )
			(void) error_report("Error processing @sample_wind");
		}

	else if ( same(buf,				"@set_source") )
		{
		if ( !set_source(list, num) )
			(void) error_report("Error processing @set_source");
		}

	else if ( same(buf,				"@size") )
		{
		if ( !set_size(list, num) )
			(void) error_report("Error processing @size");
		}

	else if ( same(buf,				"@symbol_fill_display") )
		{
		if ( !set_symbol_fill(list, num) )
			(void) error_report("Error processing @symbol_fill_display");
		}

	else if ( same(buf,				"@table_site") )
		{
		if ( !do_table_site(list, num) )
			(void) error_report("Error processing @table_site");
		}

	else if ( same(buf,				"@text") )
		{
		if ( !do_text(list, num) )
			(void) error_report("Error processing @text");
		}

	else if ( same(buf,				"@vector_presentation") )
		{
		if ( !set_vector_presentation(list, num) )
			(void) error_report("Error processing @vector_presentation");
		}

	else if ( same(buf,				"@verbose") )
		{
		if ( !set_verbose(list, num) )
			(void) error_report("Error processing @verbose");
		}

	else if ( same(buf,				"@wind_barb_presentation") )
		{
		if ( !set_wind_barb_presentation(list, num) )
			(void) error_report("Error processing @wind_barb_presentation");
		}

	/* >>> change this to  @wind_presentation  (below) <<< */
	else if ( ObsoleteVersion && same(buf, "@wind_text_presentation") )
		{
		(void) fprintf(stderr, ">>> Obsolete directive ...");
		(void) fprintf(stderr, " \"@wind_text_presentation\" <<<\n");
		(void) fprintf(stderr, ">>>  Replace by directive ...");
		(void) fprintf(stderr, " \"@wind_presentation\"   <<<\n");
		if ( !set_wind_presentation(list, num) )
			(void) error_report("Error processing @wind_text_presentation");
		}

	else if ( same(buf,				"@wind_presentation") )
		{
		if ( !set_wind_presentation(list, num) )
			(void) error_report("Error processing @wind_presentation");
		}

	else if ( same(buf,				"@write_time") )
		{
		if ( !do_write_time(list, num) )
			(void) error_report("Error processing @write_time");
		}

	else
		{
		(void) sprintf(err_buf, "Unrecognized directive: \"%s\"", buf);
		(void) error_report(err_buf);
		}

	return;
	}

void		process_texmet_directive

	(
	char		*buf,
	char		list[][GPGMedium],
	int			num
	)

	{
	int			ii;
	STRING		key, action;
	char		err_buf[GPGLong];

	/* Initialize the current presentation parameters */
	(void) copy_presentation(&CurPres, &PresDef);

	/* First scan keyword list for any local presentation keywords */
	/*  ... but only for specific directives!                      */
	if ( same(buf, "@label")
			|| same(buf, "@lchain_nodes")
			|| same(buf, "@sample_field")
			|| same(buf, "@sample_wind")
			|| same(buf, "@table_site")
			|| same(buf, "@text")
			|| same(buf, "@write_time") )
		{
		for ( ii=0; ii<num; ii++ )
			{
			(void) parse_instruction(list[ii], &key, &action);

			if ( same(key, "justification") )
				{
				(void) strcpy(CurPres.justified, action);
				}
			}
		}

	/* Now process each of the directives */

	if ( same(buf,					"@anchor") )
		{
		if ( !set_anchor(list, num) )
			(void) error_report("Error processing @anchor");
		}

	/* >>>>> add @areas here (display list of points) <<<<< */

	else if ( same(buf,				"@define_sample_grid") )
		{
		if ( !set_define_sample_grid(list, num) )
			(void) error_report("Error processing @define_sample_grid");
		}

	else if ( same(buf,				"@define_sample_list") )
		{
		if ( !set_define_sample_list(list, num) )
			(void) error_report("Error processing @define_sample_list");
		}

	else if ( same(buf,				"@define_table") )
		{
		if ( !set_define_table(list, num) )
			(void) error_report("Error processing @define_table");
		}

	else if ( same(buf,				"@label") )
		{
		if ( !do_label(list, num) )
			(void) error_report("Error processing @label");
		}

	else if ( same(buf,				"@lchain_nodes") )
		{
		if ( !do_lchain_nodes(list, num) )
			(void) error_report("Error processing @lchain_nodes");
		}

	/* >>>>> add @lchain_tracks here (display list of points) <<<<< */

	/* >>>>> add @lines here (display list of points) <<<<< */

	else if ( same(buf,				"@mapdef") )
		{
		if ( !set_mapdef(list, num) )
			(void) error_report("Error processing @mapdef");
		}

	else if ( same(buf,				"@presentation") )
		{
		if ( !set_presentation(list, num) )
			(void) error_report("Error processing @presentation");
		}

	else if ( same(buf,				"@projection") )
		{
		if ( !set_projection(list, num) )
			(void) error_report("Error processing @projection");
		}

	else if ( same(buf,				"@resolution") )
		{
		if ( !set_resolution(list, num) )
			(void) error_report("Error processing @resolution");
		}

	else if ( same(buf,				"@sample_field") )
		{
		if ( !do_sample_field(list, num) )
			(void) error_report("Error processing @sample_field");
		}

	else if ( same(buf,				"@sample_wind") )
		{
		if ( !do_sample_wind(list, num) )
			(void) error_report("Error processing @sample_wind");
		}

	else if ( same(buf,				"@set_source") )
		{
		if ( !set_source(list, num) )
			(void) error_report("Error processing @set_source");
		}

	else if ( same(buf,				"@size") )
		{
		if ( !set_size(list, num) )
			(void) error_report("Error processing @size");
		}

	else if ( same(buf,				"@table_site") )
		{
		if ( !do_table_site(list, num) )
			(void) error_report("Error processing @table_site");
		}

	else if ( same(buf,				"@text") )
		{
		if ( !do_text(list, num) )
			(void) error_report("Error processing @text");
		}

	else if ( same(buf,				"@vector_presentation") )
		{
		if ( !set_vector_presentation(list, num) )
			(void) error_report("Error processing @vector_presentation");
		}

	else if ( same(buf,				"@verbose") )
		{
		if ( !set_verbose(list, num) )
			(void) error_report("Error processing @verbose");
		}

	/* >>> change this to  @wind_presentation  (below) <<< */
	else if ( ObsoleteVersion && same(buf, "@wind_text_presentation") )
		{
		(void) fprintf(stderr, ">>> Obsolete directive ...");
		(void) fprintf(stderr, " \"@wind_text_presentation\" <<<\n");
		(void) fprintf(stderr, ">>>  Replace by directive ...");
		(void) fprintf(stderr, " \"@wind_presentation\"   <<<\n");
		if ( !set_wind_presentation(list, num) )
			(void) error_report("Error processing @wind_text_presentation");
		}

	else if ( same(buf,				"@wind_presentation") )
		{
		if ( !set_wind_presentation(list, num) )
			(void) error_report("Error processing @wind_presentation");
		}

	else if ( same(buf,				"@write_time") )
		{
		if ( !do_write_time(list, num) )
			(void) error_report("Error processing @write_time");
		}

	else
		{
		(void) sprintf(err_buf, "Unrecognized directive: \"%s\"", buf);
		(void) error_report(err_buf);
		}

	return;
	}
