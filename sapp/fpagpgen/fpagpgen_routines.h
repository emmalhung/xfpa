/***********************************************************************
*                                                                      *
*     f p a g p g e n _ r o u t i n e s . h                            *
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

/* See if already included */
#ifndef FPAGPGEN_ROUTINES_DEFS
#define FPAGPGEN_ROUTINES_DEFS


/* We need FpaGPgen structure definitions */
#include "fpagpgen_structs.h"


/* We need FPA types definitions */
#include <fpa_types.h>


/***********************************************************************
*                                                                      *
*  Declare external functions in gra_action.c                          *
*                                                                      *
***********************************************************************/

LOGICAL		GRA_display_add(STRING, float, float, LOGICAL, LOGICAL,
									float, float, STRING, STRING,
									STRING, STRING, float, STRING, STRING,
									STRING, STRING, STRING, STRING);
LOGICAL		GRA_display_all_areas(GRA_FLD_INFO *, int, STRING, STRING,
									CATATTRIB *, int, STRING, STRING,
									STRING, STRING, STRING, STRING, STRING,
									LOGICAL, COMP_PRES *, int, SPCASE *, int);
LOGICAL		GRA_display_all_contours(STRING, STRING, STRING, STRING,
									STRING, STRING, STRING, STRING, STRING,
									LOGICAL, STRING, STRING, STRING, STRING,
									STRING, COMP_PRES *, int);
LOGICAL		GRA_display_all_images(GRA_IMAGE_INFO *, int, STRING, STRING,
									STRING, STRING, STRING, STRING, STRING,
									STRING, STRING, LOGICAL, float, LOGICAL,
									float, STRING, LOGICAL, STRING);
LOGICAL		GRA_display_all_lchain_nodes(GRA_FLD_INFO *, int, STRING, STRING,
									STRING *, int, STRING, STRING, STRING, float,
									float, STRING, STRING, STRING, CATATTRIB *, int,
									STRING, CATATTRIB *, int, ATTRIB_DISPLAY *, int,
									SPCASE *, int, STRING, float, STRING, STRING,
									LOGICAL, STRING, float, LOGICAL, LOGICAL,
									STRING, LOGICAL, float, float, int, int,
									float, float, float, float, float, float);
LOGICAL		GRA_display_all_lchain_tracks(GRA_FLD_INFO *, int, STRING, STRING,
									float, STRING, STRING, CATATTRIB *, int,
									STRING, STRING, STRING,
									STRING, STRING, STRING, COMP_PRES *, int);
LOGICAL		GRA_display_all_lines(GRA_FLD_INFO *, int, STRING, CATATTRIB *, int,
									STRING, STRING, STRING, STRING, STRING,
									STRING, COMP_PRES *, int);
LOGICAL		GRA_display_box(float, float, float, LOGICAL, LOGICAL, float, float,
									STRING, STRING, STRING, STRING, float,
									STRING, STRING, STRING, STRING, STRING,
									STRING, STRING);
LOGICAL		GRA_display_xsection_areas(STRING, STRING, STRING, STRING, STRING,
									STRING, STRING, STRING, STRING,
									CATATTRIB *, int, STRING, STRING, STRING,
									GPGltype, int, XSECT_LOCATION *, STRING, float,
									STRING, STRING, STRING, float, float, float,
									STRING, STRING, STRING, COMP_PRES *, int);
LOGICAL		GRA_display_xsection_axis_labels(STRING, STRING,
									STRING, GPGltype, int, XSECT_LOCATION *,
									STRING, ATTRIB_DISPLAY *, int, STRING, STRING,
									STRING, STRING, float, float, float);
LOGICAL		GRA_display_xsection_contours(STRING, GRA_FLD_INFO *, int, STRING,
									STRING, GPGltype, int, XSECT_LOCATION *,
									STRING, STRING, STRING, STRING,
									STRING, STRING, LOGICAL, STRING, STRING,
									STRING, STRING, STRING, COMP_PRES *, int);
LOGICAL		GRA_display_xsection_curves(STRING, STRING, STRING, STRING, STRING,
									STRING, STRING, STRING, STRING,
									CATATTRIB *, int, STRING, STRING,
									GPGltype, int, XSECT_LOCATION *, STRING,
									float, STRING, STRING, STRING, STRING,
									COMP_PRES *, int);
LOGICAL		GRA_display_ellipse(float, float, float, float, LOGICAL,
									float, LOGICAL, LOGICAL, float, float,
									STRING, STRING, STRING, STRING, float,
									STRING, STRING, STRING, STRING, STRING,
									STRING, STRING);
LOGICAL		GRA_display_geography(STRING, STRING, STRING, STRING,
									CATATTRIB *, int, STRING);
LOGICAL		GRA_display_label(STRING, STRING, STRING, STRING, CATATTRIB *, int,
									ATTRIB_DISPLAY *, int, SPCASE *, int,
									STRING, float, STRING, STRING, STRING, STRING,
									LOGICAL, STRING, float, LOGICAL, LOGICAL,
									STRING, LOGICAL, float, float, int, int,
									float, float, float, float);
LOGICAL		GRA_display_plot(STRING, STRING, STRING, STRING, float, float,
									float, float, float, float, float);
LOGICAL		GRA_display_text(STRING, STRING, STRING, STRING, STRING, float,
									float, LOGICAL, LOGICAL, float, float,
									STRING, STRING, STRING, STRING, float,
									STRING, STRING, STRING, STRING, STRING,
									STRING);
LOGICAL		GRA_display_time(TTYPFMT *, int, float, float, float, float);

LOGICAL		GRA_define_line(STRING, STRING);
LOGICAL		GRA_draw_line(STRING, STRING, float, float,
									STRING, STRING, STRING, COMP_PRES *, int);
LOGICAL		GRA_draw_table_line(STRING, STRING, STRING, STRING, float, float,
									STRING, STRING, STRING, COMP_PRES *, int);
LOGICAL		GRA_draw_cross_section_line(STRING, STRING, STRING, STRING,
									GPGltype, int, XSECT_LOCATION *,
									STRING, STRING, STRING, float, float,
									STRING, STRING, STRING, COMP_PRES *, int);

LOGICAL		GRA_draw_distance_scale(STRING);
LOGICAL		GRA_display_dscale_ticks(STRING, int, SCALE_LCTNS *,
									float, STRING, float);
LOGICAL		GRA_display_dscale_labels(STRING, int, SCALE_LCTNS *,
									float, STRING, float, float, float);

LOGICAL		GRA_add_table_site(STRING, STRING, float, float, float, float,
									float, float, STRING, STRING,
									STRING, STRING, float, STRING);
LOGICAL		GRA_sample_field(STRING, STRING, STRING, STRING, STRING, STRING,
									STRING, STRING, STRING, STRING, STRING,
									CATATTRIB *, int, ATTRIB_DISPLAY *, int,
									SPCASE *, int, STRING, float, STRING,
									STRING, STRING, STRING, LOGICAL, STRING,
									float, LOGICAL, LOGICAL, STRING, LOGICAL,
									float, float, float, float, STRING, STRING,
									STRING, STRING, float, STRING, STRING,
									float, LOGICAL, STRING, STRING,
									STRING, STRING, STRING, STRING, GPGltype,
									int, XSECT_LOCATION *, STRING, STRING,
									STRING, STRING, STRING, STRING, STRING,
									STRING, STRING, STRING, STRING, STRING);
LOGICAL		GRA_sample_wind(STRING, STRING, STRING, STRING, float, STRING, STRING,
									float, float, LOGICAL, float, LOGICAL,
									LOGICAL, float, float, float, float, STRING,
									STRING, STRING, STRING, float, STRING, STRING,
									STRING, STRING, STRING, STRING, STRING,
									STRING, GPGltype, int, XSECT_LOCATION *,
									STRING, STRING, STRING, STRING, STRING,
									STRING, STRING, STRING);


/***********************************************************************
*                                                                      *
*  Declare external functions in gra_do.c                              *
*                                                                      *
***********************************************************************/

LOGICAL		do_add(char list[][GPGMedium], int);
LOGICAL		do_areas(char list[][GPGMedium], int);
LOGICAL		do_background(char list[][GPGMedium], int);
LOGICAL		do_box(char list[][GPGMedium], int);
LOGICAL		do_contours(char list[][GPGMedium], int);
LOGICAL		do_cross_section_areas(char list[][GPGMedium], int);
LOGICAL		do_cross_section_axis_labels(char list[][GPGMedium], int);
LOGICAL		do_cross_section_contours(char list[][GPGMedium], int);
LOGICAL		do_cross_section_curves(char list[][GPGMedium], int);
LOGICAL		do_draw_line(char list[][GPGMedium], int);
LOGICAL		do_draw_table_line(char list[][GPGMedium], int);
LOGICAL		do_draw_cross_section_line(char list[][GPGMedium], int);
LOGICAL		do_draw_distance_scale(char list[][GPGMedium], int);
LOGICAL		do_distance_scale_ticks(char list[][GPGMedium], int);
LOGICAL		do_distance_scale_labels(char list[][GPGMedium], int);
LOGICAL		do_ellipse(char list[][GPGMedium], int);
LOGICAL		do_geography(char list[][GPGMedium], int);
LOGICAL		do_images(char list[][GPGMedium], int);
LOGICAL		do_label(char list[][GPGMedium], int);
LOGICAL		do_lchain_nodes(char list[][GPGMedium], int);
LOGICAL		do_lchain_tracks(char list[][GPGMedium], int);
LOGICAL		do_legend(char list[][GPGMedium], int);
LOGICAL		do_lines(char list[][GPGMedium], int);
LOGICAL		do_plot(char list[][GPGMedium], int);
LOGICAL		do_sample_field(char list[][GPGMedium], int);
LOGICAL		do_sample_wind(char list[][GPGMedium], int);
LOGICAL		do_table_site(char list[][GPGMedium], int);
LOGICAL		do_text(char list[][GPGMedium], int);
LOGICAL		do_write_time(char list[][GPGMedium], int);


/***********************************************************************
*                                                                      *
*  Declare external functions in gra_io.c                              *
*                                                                      *
***********************************************************************/

void		(*initialize_graphics_display)(void);
/* PSMet application specific */
void		initialize_psmet_display(void);
/* SVGMet application specific */
void		initialize_svgmet_display(void);
/* CorMet application specific */
void		initialize_cormet_display(void);
/* TexMet application specific */
void		initialize_texmet_display(void);

LOGICAL		initialize_graphics_directories(STRING, STRING);

STRING		find_pdf_file(STRING);
STRING		find_lookup_file(STRING, STRING);
STRING		find_symbol_file(STRING);
STRING		find_text_file(STRING);
STRING		find_data_file(STRING);

LOGICAL		open_graphics_file(STRING, STRING);

void		(*close_graphics_file)(void);
LOGICAL		(*initialize_graphics_size)(float, float);
/* PSMet application specific */
void		close_psmet_file(void);
LOGICAL		initialize_psmet_size(float, float);
/* SVGMet application specific */
void		close_svgmet_file(void);
LOGICAL		initialize_svgmet_size(float, float);
/* CorMet application specific */
void		close_cormet_file(void);
LOGICAL		initialize_cormet_size(float, float);
/* TexMet application specific */
void		close_texmet_file(void);
LOGICAL		initialize_texmet_size(float, float);

void		(*write_graphics_comment)(STRING);
void		(*write_graphics_group)(STRING, char list[][GPGMedium], int);
void		(*write_graphics_bitmap)(FILE *);
void		(*write_graphics_image)(Image);
void		(*write_graphics_box)(float, float, float, float, float,
									LOGICAL, LOGICAL);
void		(*write_graphics_ellipse)(float, float, float, float, float,
									float, LOGICAL, float, LOGICAL, LOGICAL);
void		(*write_graphics_underline)(float, float, float, float);
void		(*write_graphics_text)(STRING, float, float, float, STRING, float,
									LOGICAL);
void		(*write_graphics_lines)(int, LINE *);
void		(*write_graphics_outlines)(int, LINE *, LOGICAL, LOGICAL);
void		(*write_graphics_boundaries)(int, BOUND *, LOGICAL, LOGICAL);
void		(*write_graphics_features)(int, LINE *, LOGICAL, LOGICAL);
void		(*write_graphics_symbol)(STRING, float, float, float, float);
void		(*write_graphics_outline_mask)(LINE, LOGICAL);
void		(*write_graphics_boundary_mask)(BOUND, LOGICAL);

/* PSMet application specific */
void		write_psmet_comment(STRING);
void		write_psmet_group(STRING, char list[][GPGMedium], int);
void		write_psmet_bitmap(FILE *);
void		write_psmet_image(Image);
void		write_psmet_box(float, float, float, float, float,
									LOGICAL, LOGICAL);
void		write_psmet_ellipse(float, float, float, float, float,
									float, LOGICAL, float, LOGICAL, LOGICAL);
void		write_psmet_underline(float, float, float, float);
void		write_psmet_text(STRING, float, float, float, STRING, float,
									LOGICAL);
void		write_psmet_lines(int, LINE *);
void		write_psmet_outlines(int, LINE *, LOGICAL, LOGICAL);
void		write_psmet_boundaries(int, BOUND *, LOGICAL, LOGICAL);
void		write_psmet_features(int, LINE *, LOGICAL, LOGICAL);
void		write_psmet_symbol(STRING, float, float, float, float);
void		write_psmet_outline_mask(LINE, LOGICAL);
void		write_psmet_boundary_mask(BOUND, LOGICAL);
/* SVGMet application specific */
void		write_svgmet_comment(STRING);
void		write_svgmet_group(STRING, char list[][GPGMedium], int);
void		write_svgmet_bitmap(FILE *);
void		write_svgmet_image(Image);
void		write_svgmet_box(float, float, float, float, float,
									LOGICAL, LOGICAL);
void		write_svgmet_ellipse(float, float, float, float, float,
									float, LOGICAL, float, LOGICAL, LOGICAL);
void		write_svgmet_underline(float, float, float, float);
void		write_svgmet_text(STRING, float, float, float, STRING, float,
									LOGICAL);
void		write_svgmet_lines(int, LINE *);
void		write_svgmet_outlines(int, LINE *, LOGICAL, LOGICAL);
void		write_svgmet_boundaries(int, BOUND *, LOGICAL, LOGICAL);
void		write_svgmet_features(int, LINE *, LOGICAL, LOGICAL);
void		write_svgmet_symbol(STRING, float, float, float, float);
void		write_svgmet_outline_mask(LINE, LOGICAL);
void		write_svgmet_boundary_mask(BOUND, LOGICAL);
/* CorMet application specific */
void		write_cormet_comment(STRING);
void		write_cormet_group(STRING, char list[][GPGMedium], int);
void		write_cormet_bitmap(FILE *);
void		write_cormet_box(float, float, float, float, float,
									LOGICAL, LOGICAL);
void		write_cormet_ellipse(float, float, float, float, float,
									float, LOGICAL, float, LOGICAL, LOGICAL);
void		write_cormet_underline(float, float, float, float);
void		write_cormet_text(STRING, float, float, float, STRING, float,
									LOGICAL);
void		write_cormet_lines(int, LINE *);
void		write_cormet_outlines(int, LINE *, LOGICAL, LOGICAL);
void		write_cormet_boundaries(int, BOUND *, LOGICAL, LOGICAL);
void		write_cormet_features(int, LINE *, LOGICAL, LOGICAL);
void		write_cormet_symbol(STRING, float, float, float, float);
void		write_cormet_outline_mask(LINE, LOGICAL);
void		write_cormet_boundary_mask(BOUND, LOGICAL);
LOGICAL		write_cormet_direct(char list[][GPGMedium], int);
/* TexMet application specific */
void		write_texmet_comment(STRING);
void		write_texmet_group(STRING, char list[][GPGMedium], int);
void		write_texmet_text(STRING, float, float, float, STRING, float,
									LOGICAL);

void		(*graphics_symbol_size)(STRING, float, float * , float *,
									float *, float *);
/* PSMet application specific */
void		psmet_symbol_size(STRING, float, float *, float *,
									float *, float *);
/* SVGMet application specific */
void		svgmet_symbol_size(STRING, float, float *, float *,
									float *, float *);
/* CorMet application specific */
void		cormet_symbol_size(STRING, float, float *, float *,
									float *, float *);

LOGICAL		process_pdf_file(STRING);

void		parse_instruction(STRING, STRING *, STRING *);
void		parse_program_instruction(STRING, STRING *, STRING *);

void		error_report(STRING);
void		warn_report(STRING);


/***********************************************************************
*                                                                      *
*  Declare external functions in gra_misc.c                            *
*                                                                      *
***********************************************************************/

void		(*default_graphics_presentation)(void);
/* PSMet application specific */
void		default_psmet_presentation(void);
/* SVGMet application specific */
void		default_svgmet_presentation(void);
/* CorMet application specific */
void		default_cormet_presentation(void);
/* TexMet application specific */
void		default_texmet_presentation(void);

void		copy_presentation(PRES *, PRES *);
PRES		*add_presentation(STRING);
PRES		get_presentation(STRING);

int			add_comp_pres(COMP_PRES **, int);
void		copy_comp_pres(COMP_PRES *, COMP_PRES *);
void		check_comp_pres(void);
void		free_comp_pres(void);
void		replace_presentation_comp_pres(PRES *, int, COMP_PRES *);
void		reset_presentation_by_comp_pres(PRES *, COMP_PRES *);

int			add_category_attribs(CATATTRIB **, int);
void		free_category_attribs(void);
int			add_track_category_attribs(CATATTRIB **, int);
void		free_track_category_attribs(void);

int			add_time_types(TTYPFMT **, int);
void		free_time_types(void);

STRING		replace_default_attribute(int, STRING);
LOGICAL		magic_is_attribute(STRING);
STRING		magic_get_attribute(STRING, STRING, STRING, STRING, STRING,
									STRING, POINT, float, float, float,
									float, float, STRING, STRING, STRING);

LOGICAL		check_label_attributes(CAL, ATTRIB_DISPLAY *, int);
LOGICAL		check_value_attributes(ATTRIB_DISPLAY *, int);
LOGICAL		check_vector_attributes(ATTRIB_DISPLAY *, int);
LOGICAL		match_category_attributes(CAL, STRING, CATATTRIB *, int);

void		initialize_contour_presentation(void);
LOGICAL		add_contour_values(STRING, STRING, PRES *);
LOGICAL		add_contour_ranges(STRING, STRING, PRES *);
PRES		get_contour_presentation(float, STRING);

void		initialize_geo_presentation(void);
PRES		*add_geo_presentation(STRING);
PRES		get_geo_presentation(STRING);

void			copy_arrow_display(ARROW_DISPLAY *, ARROW_DISPLAY *);
ARROW_DISPLAY	*add_arrow_display(STRING);
ARROW_DISPLAY	get_arrow_display(STRING);

void			copy_label_display(LABEL_DISPLAY *, LABEL_DISPLAY *);
LABEL_DISPLAY	*add_label_display(STRING);
LABEL_DISPLAY	get_label_display(STRING);

void			initialize_attribute_display(void);
void			copy_attribute_display(ATTRIB_DISPLAY *, ATTRIB_DISPLAY *);
ATTRIB_DISPLAY	*add_attribute_display(STRING, STRING, STRING, STRING, STRING,
									float, float, float, float, PRES *);
ATTRIB_DISPLAY	get_attribute_display(STRING);
void			set_attribute_placement(ATTRIB_DISPLAY *, STRING, STRING);
void			set_wind_barb_placement(ATTRIB_DISPLAY *, float, float, float);
void			set_wind_placement(STRING, ATTRIB_DISPLAY *,
									float, float, float, float, float);
void			set_vector_placement(STRING, ATTRIB_DISPLAY *,
									float, float, float, float);
ATTRIB_DISPLAY	full_attribute_placement(void);
LOGICAL			fit_attribute_placement(ATTRIB_DISPLAY, STRING,
									float, float, float, float, float,
									float *, float *, float *);
int				return_attribute_display(ATTRIB_DISPLAY **);

void			add_distance_scale(STRING, float, STRING, float, float, float);
SCALE_DISPLAY	*get_distance_scale(STRING);
LINE			distance_scale_line(float, STRING, float, float, float);
int				add_distance_scale_location(SCALE_LCTNS **, int);
void			free_distance_scale_locations(void);

void			copy_symbol_fill(SYMBOL_FILL *, SYMBOL_FILL *);
SYMBOL_FILL		*add_symbol_fill(STRING);
SYMBOL_FILL		get_symbol_fill(STRING);

LOGICAL		define_source(STRING, STRING);

LOGICAL		(*define_graphics_units)(STRING, float);
/* PSMet application specific */
LOGICAL		define_psmet_units(STRING, float);
/* SVGMet application specific */
LOGICAL		define_svgmet_units(STRING, float);
/* CorMet application specific */
LOGICAL		define_cormet_units(STRING, float);

LOGICAL		(*define_graphics_placement)(float, float, STRING, float, float);
LOGICAL		(*define_graphics_anchor)(STRING, float, float, float, float,
																	STRING);
/* PSMet application specific */
LOGICAL		define_psmet_placement(float, float, STRING, float, float);
LOGICAL		define_psmet_anchor(STRING, float, float, float, float, STRING);
/* SVGMet application specific */
LOGICAL		define_svgmet_placement(float, float, STRING, float, float);
LOGICAL		define_svgmet_anchor(STRING, float, float, float, float, STRING);
/* CorMet application specific */
LOGICAL		define_cormet_placement(float, float, STRING, float, float);
LOGICAL		define_cormet_anchor(STRING, float, float, float, float, STRING);
/* TexMet application specific */
LOGICAL		define_texmet_anchor(STRING, float, float, float, float, STRING);

float		map_scaling(void);
void		map_dimensions(POINT, float *, float *);

LOGICAL		define_perspective_view(LOGICAL, LOGICAL, float, float,
									float, float, float, float, float);
LOGICAL		perspective_location(const POINT, POINT, float *);
LOGICAL		perspective_original(const POINT, POINT, float *);
LOGICAL		perspective_scale(float *);

void		anchored_location(const POINT, float, float, float *, float *);
void		anchored_line(const LINE, float, float, LINE *);
void		anchored_boundary(const BOUND, float, float, BOUND *);
void		rotated_location(float, float, float, float *, float *);
void		unanchored_location(const POINT, float, float, float *, float *);
void		unanchored_line(const LINE, float, float, LINE *);

LOGICAL		match_category_lookup(STRING, STRING, STRING *, STRING *, STRING *);
LOGICAL		match_wind_lookup(STRING, STRING, STRING, float, float,
									float, float, float,
									STRING *, STRING *, float *, float *);
LOGICAL		match_vector_lookup(STRING, STRING, STRING, float, float,
									float, float,
									STRING *, STRING *, float *, float *);
LOGICAL		add_loop_location_lookup(STRING, STRING, STRING, STRING *, int,
									STRING *, int, float);
LOGICAL		match_location_lookup(STRING, STRING, STRING, STRING *, STRING *,
									STRING *);
int			next_location_lookup_line(STRING, int, STRING, STRING *, STRING *,
									STRING *, STRING *);
LOGICAL		match_data_file(STRING, STRING, STRING, STRING, STRING, STRING,
									STRING, STRING, CAL *);

PATTERN		*get_pattern(STRING, float, float);
PATTERN		*get_default_baseline_pattern(float, float);

LOGICAL		add_cross_section(STRING, STRING, float, float, float,
									STRING, STRING, float, float);
GRA_XSECT	*get_cross_section(STRING);
void		free_cross_section_axes(GRA_XSECT *);
LOGICAL		define_cross_section_horizontal_axis(GRA_XSECT *, STRING, double *);
XSECT_HOR_AXIS	*cross_section_horizontal_axis(GRA_XSECT *, STRING, 
									GPGltype, int, XSECT_LOCATION *, double *);
LOGICAL		define_cross_section_vertical_axis(GRA_XSECT *, STRING, double *);
XSECT_VER_AXIS	*cross_section_vertical_axis(GRA_XSECT *, STRING, double *);

LOGICAL		add_sample_grid(STRING, STRING, STRING, STRING, STRING, STRING,
									STRING, STRING, STRING, STRING, STRING,
									STRING, STRING, float, float, float);
GRA_GRID	*get_sample_grid(STRING);
void		free_sample_grid_locations(GRA_GRID *);

LOGICAL		add_sample_list(STRING, int, GRA_LCTN *, float, float, float, int, int);
GRA_LIST	*get_sample_list(STRING);
void		free_sample_list_locations(GRA_LIST *);

LOGICAL		add_table(STRING, STRING, float, float);
GRA_TABLE	*get_table(STRING);

LOGICAL		add_legend(STRING, STRING, float, float, float, float, float,
									PRES *);
LOGICAL		display_legend(void);

void		(*check_graphics_keyword)(STRING, STRING);
/* PSMet application specific */
void		check_psmet_keyword(STRING, STRING);
/* SVGMet application specific */
void		check_svgmet_keyword(STRING, STRING);
/* CorMet application specific */
void		check_cormet_keyword(STRING, STRING);
/* TexMet application specific */
void		check_texmet_keyword(STRING, STRING);


/***********************************************************************
*                                                                      *
*  Declare external functions in gra_process.c                         *
*                                                                      *
***********************************************************************/

void		(*process_graphics_directive)(char *, char list[][GPGMedium], int);
/* PSMet application specific */
void		process_psmet_directive(char *, char list[][GPGMedium], int);
/* SVGMet application specific */
void		process_svgmet_directive(char *, char list[][GPGMedium], int);
/* CorMet application specific */
void		process_cormet_directive(char *, char list[][GPGMedium], int);
/* TexMet application specific */
void		process_texmet_directive(char *, char list[][GPGMedium], int);


/***********************************************************************
*                                                                      *
*  Declare external functions in gra_set.c                             *
*                                                                      *
***********************************************************************/

LOGICAL		set_anchor(char list[][GPGMedium], int);
LOGICAL		set_arrow_display(char list[][GPGMedium], int);
LOGICAL		set_contour_presentation(char list[][GPGMedium], int);
LOGICAL		set_define_cross_section(char list[][GPGMedium], int);
LOGICAL		set_define_line(char list[][GPGMedium], int);
LOGICAL		set_define_map_placement(char list[][GPGMedium], int);
LOGICAL		set_define_sample_grid(char list[][GPGMedium], int);
LOGICAL		set_define_sample_list(char list[][GPGMedium], int);
LOGICAL		set_define_table(char list[][GPGMedium], int);
LOGICAL		set_display_units(char list[][GPGMedium], int);
LOGICAL		set_filter(char list[][GPGMedium], int);
LOGICAL		set_geo_presentation(char list[][GPGMedium], int);
LOGICAL		set_group_begin(char list[][GPGMedium], int);
LOGICAL		set_label_display(char list[][GPGMedium], int);
LOGICAL		set_mapdef(char list[][GPGMedium], int);
LOGICAL		set_perspective_view(char list[][GPGMedium], int);
LOGICAL		set_presentation(char list[][GPGMedium], int);
LOGICAL		set_projection(char list[][GPGMedium], int);
LOGICAL		set_resolution(char list[][GPGMedium], int);
LOGICAL		set_size(char list[][GPGMedium], int);
LOGICAL		set_source(char list[][GPGMedium], int);
LOGICAL		set_symbol_fill(char list[][GPGMedium], int);
LOGICAL		set_verbose(char list[][GPGMedium], int);
LOGICAL		set_wind_barb_presentation(char list[][GPGMedium], int);
LOGICAL		set_wind_presentation(char list[][GPGMedium], int);
LOGICAL		set_vector_presentation(char list[][GPGMedium], int);


/* Now it has been included */
#endif
