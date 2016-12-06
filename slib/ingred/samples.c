/***********************************************************************
*                                                                      *
*     s a m p l e s . c                                                *
*                                                                      *
*     Assorted sampling operations for all fields.                     *
*                                                                      *
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

#include "ingred_private.h"

static  char    msg[256]  = "";
static  char    buf[4096] = "";

static	void	clear_sample_box(void);
static	LOGICAL	construct_value_sample(FLD_DESCRIPT *,
						FpaConfigSampleStruct *,
						FpaConfigCrossRefStruct *,
						int, POINT *);
static	LOGICAL	construct_attrib_sample(FLD_DESCRIPT *,
						STRING, int, POINT *);
static	LOGICAL	construct_wind_sample(FLD_DESCRIPT *,
						FpaConfigSampleStruct *,
						FpaConfigCrossRefStruct *,
						int, POINT *);
static	LOGICAL	construct_field_label_sample(FLD_DESCRIPT *, POINT);
static	LOGICAL	construct_lchain_node_sample(FLD_DESCRIPT *, POINT);
static	LOGICAL	construct_image_sample(Image,
						STRING, int, POINT *);
static	void	build_spline_value_sample(POINT);
static	void	build_spline_gradient_sample(POINT);
static	void	build_spline_curvature_sample(POINT);
static	void	build_vector_magnitude_sample(POINT);
static	void	build_vector_direction_sample(POINT);
static	void	build_vector_value_sample(POINT);
static	void	build_area_attrib_sample(POINT, STRING);
static	void	build_area_attrib_box_sample(POINT);
static	void	build_line_attrib_sample(POINT, STRING);
static	void	build_line_attrib_box_sample(POINT);
static	void	build_point_attrib_sample(POINT, STRING);
static	void	build_point_attrib_box_sample(POINT);
static	void	build_lchain_attrib_sample(POINT, STRING);
static	void	build_lchain_attrib_box_sample(POINT);
static	void	build_wind_barb_sample(POINT, float, float, STRING);

/***********************************************************************
*                                                                      *
*     s a m p l e _ b y _ t y p e                                      *
*                                                                      *
***********************************************************************/

LOGICAL	sample_by_type

	(
	STRING	mode,
	STRING	stype,
	STRING	subtype,
	STRING	source,
	STRING	subsrc,
	STRING	rtime,
	STRING	elem,
	STRING	level
	)

	{
	FpaConfigSampleStruct		*sdef;
	FpaConfigCrossRefStruct		*xdef;

#	ifdef DEBUG
	pr_diag("Editor", "[sample_by_type] %s %s\n", stype, mode);
#	endif /* DEBUG */

	/* Value samples (value, gradient, curvature, ...) */
	if (same(stype, FpaCsampleControlValueType))
		{
		sdef = identify_sample(FpaCsamplesValues, subtype);
		if (!sdef)
			{
			put_message("sample-unsupported", stype, subtype);
			return FALSE;
			}

		switch (sdef->samp_type)
			{
			case FpaC_SAMPLE_VALUE:
			case FpaC_SAMPLE_LABEL:
			case FpaC_SAMPLE_GRADIENT:
			case FpaC_SAMPLE_CURVATURE:
			case FpaC_SAMPLE_MAGNITUDE:
			case FpaC_SAMPLE_DIRECTION:
				break;

			default:
				put_message("sample-unsupported", stype, subtype);
				return FALSE;
			}

		return sample_value(mode, sdef, NoXref, source, subsrc, rtime,
					elem, level);
		}

	/* Attribute samples */
	else if (same(stype, FpaCsampleControlAttribType))
		{
		/* Next value is attribute name or "ALL" */
		if (same(subtype, FpaCsampleValue)) subtype = CALuserlabel;
		if (same(subtype, FpaCsampleLabel)) subtype = CALautolabel;

		return sample_attribute(mode, subtype, source, subsrc, rtime,
					elem, level);
		}

	/* Wind samples (Adjusted) */
	else if (same(stype, FpaCsampleControlWindType))
		{
		sdef = identify_sample(FpaCsamplesWinds, subtype);
		if (!sdef)
			{
			put_message("sample-unsupported", stype, subtype);
			return FALSE;
			}

		return sample_wind(mode, sdef, NoXref, source, subsrc, rtime,
							elem, level);
		}

	/* Wind cross-reference samples (Vg, Vc, Vr, Cardone, ...) */
	else if (same(stype, FpaCsampleControlWindCrossRef))
		{
		xdef = identify_crossref(FpaCcRefsWinds, subtype);
		if (!xdef)
			{
			put_message("sample-unsupported", stype, subtype);
			return FALSE;
			}

		return sample_wind(mode, NoSamp, xdef, source, subsrc, rtime,
							elem, level);
		}

	/* Field label samples */
	else if (same(stype, FpaCsampleControlFieldLabels))
		{
		return sample_field_label(mode, source, subsrc, rtime, elem, level);
		}

	/* Link chain node samples */
	else if (same(stype, FpaCsampleControlLinkNodes))
		{
		return sample_lchain_node(mode, source, subsrc, rtime, elem, level);
		}

	/* Unknown */
	put_message("sample-unsupported", stype, subtype);
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*     s a m p l e _ v a l u e                                          *
*                                                                      *
***********************************************************************/

LOGICAL	sample_value

	(
	STRING					mode,
	FpaConfigSampleStruct	*sdef,
	FpaConfigCrossRefStruct	*xref,
	STRING					source,
	STRING					subsrc,
	STRING					rtime,
	STRING					elem,
	STRING					level
	)

	{
	POINT	spos;
	int		butt;
	LOGICAL	valid;

	static	FLD_DESCRIPT	fd;

#	ifdef DEBUG
	pr_diag("Editor", "[sample_value] %s\n", mode);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same_start(mode, "cancel"))
		{
		clear_sample_box();
		return FALSE;
		}

	/* Clear all samples on request */
	if (same(mode, "clear"))
		{
		put_message("edit-clear");
		empty_sample();
		sample_box(FALSE, NullCal);
		return FALSE;
		}

	/* Interpret wind type - If blank use appropriate wind */
	(void) init_fld_descript(&fd);
	if (!set_fld_descript(&fd,
					FpaF_MAP_PROJECTION,		MapProj,
					FpaF_SOURCE_NAME,			source,
					FpaF_SUBSOURCE_NAME,		subsrc,
					FpaF_RUN_TIME,				rtime,
					FpaF_VALID_TIME,			EditFd.vtime,
					FpaF_END_OF_LIST))
		{
		put_message("sample-no-eval");
		return FALSE;
		}
	if (sdef && !xref)
		{
		if (!set_fld_descript(&fd,
						FpaF_ELEMENT_NAME,			elem,
						FpaF_LEVEL_NAME,			level,
					/*	FpaF_VALUE_FUNCTION_NAME,	sdef->samp_name, */
						FpaF_END_OF_LIST))
			{
			put_message("sample-no-eval");
			return FALSE;
			}
		}

	/* Sample list if requested */
	if (EditUseList)
		{
		/* Construct value displays */
		valid = construct_value_sample(&fd, sdef, xref, EditNumP, EditPlist);
		if (!valid)
			{
			put_message("sample-no-eval");
			(void) sleep(1);
			}
		present_sample(TRUE);
		(void) eval_list_reset();
		}

	edit_can_clear(TRUE);

	/* Repeat until told to quit */
	allow_obscured_input(TRUE);
	while (TRUE)
		{
		/* Put up a prompt */
		put_message("sample-value");

		/* Pick a point */
		if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return FALSE;
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}

		/* Left button - Add a sample */
		put_message("sample-value-add");
		(void) pick_Xpoint(DnEdit, 0, spos, &butt);
		if (!inside_dn_window(DnEdit, spos))
			{
			put_message("edit-outside-map");
			(void) sleep(1);
			continue;
			}

		/* Construct value to display */
		put_message("sample-adding");
		valid = construct_value_sample(&fd, sdef, xref, 1, make_plist(spos));
		if (!valid)
			{
			put_message("sample-no-eval");
			(void) sleep(1);
			continue;
			}
		present_sample(TRUE);
		}
	}

/***********************************************************************
*                                                                      *
*     s a m p l e _ a t t r i b u t e                                  *
*                                                                      *
***********************************************************************/

LOGICAL	sample_attribute

	(
	STRING	mode,
	STRING	attrib,
	STRING	source,
	STRING	subsrc,
	STRING	rtime,
	STRING	elem,
	STRING	level
	)

	{
	POINT	spos;
	int		butt;
	LOGICAL	valid;

	static	FLD_DESCRIPT	fd;

#	ifdef DEBUG
	pr_diag("Editor", "[sample_attribute] %s\n", mode);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same_start(mode, "cancel"))
		{
		clear_sample_box();
		return FALSE;
		}

	/* Clear all samples on request */
	if (same(mode, "clear"))
		{
		put_message("edit-clear");
		empty_sample();
		sample_box(FALSE, NullCal);
		return FALSE;
		}

	/* Interpret wind type - If blank use appropriate wind */
	(void) init_fld_descript(&fd);
	if (!set_fld_descript(&fd,
					FpaF_MAP_PROJECTION,		MapProj,
					FpaF_SOURCE_NAME,			source,
					FpaF_SUBSOURCE_NAME,		subsrc,
					FpaF_RUN_TIME,				rtime,
					FpaF_VALID_TIME,			EditFd.vtime,
					FpaF_END_OF_LIST))
		{
		put_message("sample-no-eval");
		return FALSE;
		}
	if (!blank(attrib))
		{
		if (!set_fld_descript(&fd,
						FpaF_ELEMENT_NAME,				elem,
						FpaF_LEVEL_NAME,				level,
						FpaF_END_OF_LIST))
			{
			put_message("sample-no-eval");
			return FALSE;
			}
		}

	/* Sample list if requested */
	if (EditUseList)
		{
		/* Don't sample list if "ALL" requested */
		if (!same_ic(attrib, AttribAll))
			{
			/* Construct value displays */
			valid = construct_attrib_sample(&fd, attrib, EditNumP, EditPlist);
			if (!valid)
				{
				put_message("sample-no-eval");
				(void) sleep(1);
				}
			}
		present_sample(TRUE);
		(void) eval_list_reset();
		}

	edit_can_clear(TRUE);

	/* Repeat until told to quit */
	allow_obscured_input(TRUE);
	while (TRUE)
		{
		/* Put up a prompt */
		put_message("sample-value");

		/* Pick a point */
		if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return FALSE;
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}

		/* Left button - Add a sample */
		put_message("sample-value-add");
		(void) pick_Xpoint(DnEdit, 0, spos, &butt);
		if (!inside_dn_window(DnEdit, spos))
			{
			put_message("edit-outside-map");
			(void) sleep(1);
			continue;
			}

		/* Construct value to display */
		put_message("sample-adding");
		valid = construct_attrib_sample(&fd, attrib, 1, make_plist(spos));
		if (!valid)
			{
			put_message("sample-no-eval");
			(void) sleep(1);
			continue;
			}
		present_sample(TRUE);
		}
	}

/***********************************************************************
*                                                                      *
*     s a m p l e _ w i n d                                            *
*                                                                      *
***********************************************************************/

LOGICAL	sample_wind

	(
	STRING					mode,
	FpaConfigSampleStruct	*sdef,
	FpaConfigCrossRefStruct	*xref,
	STRING					source,
	STRING					subsrc,
	STRING					rtime,
	STRING					elem,
	STRING					level
	)

	{
	POINT	spos;
	int		butt;
	LOGICAL	valid;

	static	FLD_DESCRIPT	fd;

#	ifdef DEBUG
	pr_diag("Editor", "[sample_wind] %s\n", mode);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same_start(mode, "cancel"))
		{
		clear_sample_box();
		return FALSE;
		}

	/* Clear all samples on request */
	if (same(mode, "clear"))
		{
		put_message("edit-clear");
		empty_sample();
		sample_box(FALSE, NullCal);
		return FALSE;
		}

	/* Interpret wind type - If blank use appropriate wind */
	(void) init_fld_descript(&fd);
	if (!set_fld_descript(&fd,
					FpaF_MAP_PROJECTION,		MapProj,
					FpaF_SOURCE_NAME,			source,
					FpaF_SUBSOURCE_NAME,		subsrc,
					FpaF_RUN_TIME,				rtime,
					FpaF_VALID_TIME,			EditFd.vtime,
					FpaF_END_OF_LIST))
		{
		put_message("sample-no-eval");
		return FALSE;
		}
	if (sdef && !xref)
		{
		if (!set_fld_descript(&fd,
						FpaF_ELEMENT_NAME,			elem,
						FpaF_LEVEL_NAME,			level,
						FpaF_WIND_FUNCTION_NAME,	sdef->samp_func,
						FpaF_END_OF_LIST))
			{
			put_message("sample-no-eval");
			return FALSE;
			}
		}

	/* Sample list if requested */
	if (EditUseList)
		{
		/* Construct wind barbs */
		valid = construct_wind_sample(&fd, sdef, xref, EditNumP, EditPlist);
		if (!valid)
			{
			put_message("sample-no-eval");
			(void) sleep(1);
			}
		present_sample(TRUE);
		(void) eval_list_reset();
		}

	edit_can_clear(TRUE);

	/* Repeat until told to quit */
	allow_obscured_input(TRUE);
	while (TRUE)
		{
		/* Put up a prompt */
		put_message("sample-wind");

		/* Pick a point */
		if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return FALSE;
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}

		/* Left button - Add a sample */
		put_message("sample-wind-add");
		(void) pick_Xpoint(DnEdit, 0, spos, &butt);
		if (!inside_dn_window(DnEdit, spos))
			{
			put_message("edit-outside-map");
			(void) sleep(1);
			continue;
			}

		/* Construct wind barb */
		put_message("sample-adding");
		valid = construct_wind_sample(&fd, sdef, xref, 1, make_plist(spos));
		if (!valid)
			{
			put_message("sample-no-eval");
			(void) sleep(1);
			continue;
			}
		present_sample(TRUE);
		}
	}

/***********************************************************************
*                                                                      *
*     s a m p l e _ f i e l d _ l a b e l                              *
*                                                                      *
***********************************************************************/

LOGICAL	sample_field_label

	(
	STRING	mode,
	STRING	source,
	STRING	subsrc,
	STRING	rtime,
	STRING	elem,
	STRING	level
	)

	{
	POINT	spos;
	int		butt;
	LOGICAL	valid;

	static	FLD_DESCRIPT	fd;

#	ifdef DEBUG
	pr_diag("Editor", "[sample_field_label] %s\n", mode);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same_start(mode, "cancel"))
		{
		clear_sample_box();
		return FALSE;
		}

	/* Clear all samples on request */
	if (same(mode, "clear"))
		{
		put_message("edit-clear");
		empty_sample();
		sample_box(FALSE, NullCal);
		return FALSE;
		}

	/* Set field to sample */
	(void) init_fld_descript(&fd);
	if (!set_fld_descript(&fd,
					FpaF_MAP_PROJECTION,		MapProj,
					FpaF_SOURCE_NAME,			source,
					FpaF_SUBSOURCE_NAME,		subsrc,
					FpaF_RUN_TIME,				rtime,
					FpaF_VALID_TIME,			EditFd.vtime,
					FpaF_ELEMENT_NAME,			elem,
					FpaF_LEVEL_NAME,			level,
					FpaF_END_OF_LIST))
		{
		put_message("sample-no-eval");
		return FALSE;
		}

	edit_can_clear(TRUE);

	/* Repeat until told to quit */
	allow_obscured_input(TRUE);
	while (TRUE)
		{
		/* Put up a prompt */
		put_message("sample-value");

		/* Pick a point */
		if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return FALSE;
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}

		/* Left button - Add a sample */
		put_message("sample-value-add");
		(void) pick_Xpoint(DnEdit, 0, spos, &butt);
		if (!inside_dn_window(DnEdit, spos))
			{
			put_message("edit-outside-map");
			(void) sleep(1);
			continue;
			}

		/* Construct value to display */
		put_message("sample-adding");
		valid = construct_field_label_sample(&fd, spos);
		if (!valid)
			{
			put_message("sample-no-eval");
			(void) sleep(1);
			continue;
			}
		present_sample(TRUE);
		}
	}

/***********************************************************************
*                                                                      *
*     s a m p l e _ l c h a i n _ n o d e                              *
*                                                                      *
***********************************************************************/

LOGICAL	sample_lchain_node

	(
	STRING	mode,
	STRING	source,
	STRING	subsrc,
	STRING	rtime,
	STRING	elem,
	STRING	level
	)

	{
	POINT	spos;
	int		butt;
	LOGICAL	valid;

	static	FLD_DESCRIPT	fd;

#	ifdef DEBUG
	pr_diag("Editor", "[sample_lchain_node] %s\n", mode);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same_start(mode, "cancel"))
		{
		clear_sample_box();
		return FALSE;
		}

	/* Clear all samples on request */
	if (same(mode, "clear"))
		{
		put_message("edit-clear");
		empty_sample();
		sample_box(FALSE, NullCal);
		return FALSE;
		}

	/* Set field to sample */
	(void) init_fld_descript(&fd);
	if (!set_fld_descript(&fd,
					FpaF_MAP_PROJECTION,		MapProj,
					FpaF_SOURCE_NAME,			source,
					FpaF_SUBSOURCE_NAME,		subsrc,
					FpaF_RUN_TIME,				rtime,
					FpaF_VALID_TIME,			EditFd.vtime,
					FpaF_ELEMENT_NAME,			elem,
					FpaF_LEVEL_NAME,			level,
					FpaF_END_OF_LIST))
		{
		put_message("sample-no-eval");
		return FALSE;
		}

	edit_can_clear(TRUE);

	/* Repeat until told to quit */
	allow_obscured_input(TRUE);
	while (TRUE)
		{
		/* Put up a prompt */
		put_message("sample-lnode");

		/* Pick a point */
		if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return FALSE;
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}

		/* Left button - Add a sample */
		put_message("sample-lnode-add");
		(void) pick_Xpoint(DnEdit, 0, spos, &butt);
		if (!inside_dn_window(DnEdit, spos))
			{
			put_message("edit-outside-map");
			(void) sleep(1);
			continue;
			}

		/* Construct value to display */
		put_message("sample-adding");
		valid = construct_lchain_node_sample(&fd, spos);
		if (!valid)
			{
			put_message("sample-no-eval");
			(void) sleep(1);
			continue;
			}
		present_sample(TRUE);
		}
	}

/***********************************************************************
*                                                                      *
*     s a m p l e _ i m a g e                                          *
*                                                                      *
***********************************************************************/

LOGICAL	sample_image

	(
	STRING		mode,
	Image		image,
	STRING		type,
	STRING		item
	)

	{
	POINT	spos;
	int		butt;
	LOGICAL	valid;

#	ifdef DEBUG
	pr_diag("Editor", "[sample_image] %s\n", mode);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same_start(mode, "cancel"))
		{
		return FALSE;
		}

	/* Clear all samples on request */
	if (same(mode, "clear"))
		{
		put_message("edit-clear");
		empty_sample();
		sample_box(FALSE, NullCal);
		return FALSE;
		}

	/* Interpret image type ... only "Default" is used as yet! */
	if (!same_ic(type, "Default"))
		{
		/* >>>>> print out a message <<<<< */
		return FALSE;
		}

	/* Sample list if requested */
	if (EditUseList)
		{
		/* Construct value displays */
		valid = construct_image_sample(image, item, EditNumP, EditPlist);
		if (!valid)
			{
			put_message("sample-no-eval");
			(void) sleep(1);
			}
		present_sample(TRUE);
		(void) eval_list_reset();
		}

	edit_can_clear(TRUE);

	/* Repeat until told to quit */
	allow_obscured_input(TRUE);
	while (TRUE)
		{
		/* Put up a prompt */
		put_message("sample-image");

		/* Pick a point */
		if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return FALSE;
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}

		/* Left button - Add a sample */
		put_message("sample-image-add");
		(void) pick_Xpoint(DnEdit, 0, spos, &butt);
		if (!inside_dn_window(DnEdit, spos))
			{
			put_message("edit-outside-map");
			(void) sleep(1);
			continue;
			}

		/* Construct value to display */
		put_message("sample-adding");
		valid = construct_image_sample(image, item, 1, make_plist(spos));
		if (!valid)
			{
			put_message("sample-no-eval");
			(void) sleep(1);
			continue;
			}
		present_sample(TRUE);
		}
	}

/***********************************************************************
*                                                                      *
*     c l e a r _ s a m p l e _ b o x                                  *
*                                                                      *
***********************************************************************/

static	void	clear_sample_box(void)

	{
	LOGICAL	redisplay;
	SET		set;

#	ifdef DEBUG
	pr_diag("Editor",
		"[clear_sample_box] SampleMeta fields (before): %d\n",
		NotNull(SampleMeta)? SampleMeta->numfld: 0);
#	endif /* DEBUG */

	/* Remove all previously displayed features from sample box display */
	redisplay = FALSE;
	set = take_mf_set(SampleMeta, "spot", "d", "box", "box");
	if (NotNull(set))
		{
		set = destroy_set(set);
		redisplay = TRUE;
		}
	set = take_mf_set(SampleMeta, "mark", "d", "box", "box");
	if (NotNull(set))
		{
		set = destroy_set(set);
		redisplay = TRUE;
		}
	set = take_mf_set(SampleMeta, "curve", "c", "box", "box");
	if (NotNull(set))
		{
		set = destroy_set(set);
		redisplay = TRUE;
		}

	/* Show the results (if required) */
	if (redisplay) present_all();

#	ifdef DEBUG
	pr_diag("Editor",
		"[clear_sample_box] SampleMeta fields (after):  %d\n",
		NotNull(SampleMeta)? SampleMeta->numfld: 0);
#	endif /* DEBUG */
	}

/***********************************************************************
*                                                                      *
*     c o n s t r u c t _ v a l u e _ s a m p l e                      *
*                                                                      *
***********************************************************************/

static	LOGICAL	construct_value_sample

	(
	FLD_DESCRIPT				*fd,
	FpaConfigSampleStruct		*sdef,
	FpaConfigCrossRefStruct		*xref,
	int							np,
	POINT						*pbuf
	)

	{
	int		ip, ftype;
	float	lat, lon;

	/* Remove display from last sample box */
	clear_sample_box();

	/* We don't handle cross refs yet */
	if (!sdef) return FALSE;
	if (sdef && !xref)
		{
		if (sdef->samp_type == FpaCnoMacro) return FALSE;
		if (!find_retrieve_metasfc(fd))     return FALSE;
		}

	/* Verify that we have what we need for the type of field */
	ftype = fd->edef->fld_type;
	switch (ftype)
		{
		case FpaC_CONTINUOUS:
		case FpaC_VECTOR:
			if (!EditSfc) return FALSE;
			break;

		case FpaC_WIND:
			if (!EditAreas) return FALSE;
			break;

		case FpaC_DISCRETE:
		case FpaC_LINE:
		case FpaC_SCATTERED:
		case FpaC_LCHAIN:
		default:
			return FALSE;
		}

	/* Repeat for each point */
	for (ip=0; ip<np; ip++)
		{
		pos_to_ll(MapProj, pbuf[ip], &lat, &lon);
		switch (ftype)
			{
			case FpaC_CONTINUOUS:
				switch (sdef->samp_type)
					{
					case FpaC_SAMPLE_VALUE:
						build_spline_value_sample(pbuf[ip]);
						break;
					case FpaC_SAMPLE_GRADIENT:
						build_spline_gradient_sample(pbuf[ip]);
						break;
					case FpaC_SAMPLE_CURVATURE:
						build_spline_curvature_sample(pbuf[ip]);
						break;
					}
				break;

			case FpaC_VECTOR:
				switch (sdef->samp_type)
					{
					case FpaC_SAMPLE_MAGNITUDE:
						build_vector_magnitude_sample(pbuf[ip]);
						break;
					case FpaC_SAMPLE_DIRECTION:
						build_vector_direction_sample(pbuf[ip]);
						break;
					case FpaC_SAMPLE_VALUE:
						build_vector_value_sample(pbuf[ip]);
						break;
					}
				break;

			case FpaC_WIND:
				switch (sdef->samp_type)
					{
					case FpaC_SAMPLE_VALUE:
						build_area_attrib_sample(pbuf[ip], CALautolabel);
						break;
					case FpaC_SAMPLE_LABEL:
						build_area_attrib_sample(pbuf[ip], CALuserlabel);
						break;
					}
				break;
			}
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     c o n s t r u c t _ a t t r i b _ s a m p l e                    *
*                                                                      *
***********************************************************************/

static	LOGICAL	construct_attrib_sample

	(
	FLD_DESCRIPT	*fd,
	STRING			attrib,
	int				np,
	POINT			*pbuf
	)

	{
	int		ip, ftype;

	/* Remove display from last sample box */
	clear_sample_box();

	/* Return if nothing to sample */
	if (blank(attrib))              return FALSE;
	if (!find_retrieve_metasfc(fd)) return FALSE;

	/* Verify that we have what we need for the type of field */
	ftype = fd->edef->fld_type;
	switch (ftype)
		{
		case FpaC_DISCRETE:
		case FpaC_WIND:
			if (!EditAreas) return FALSE;
			break;

		case FpaC_LINE:
			if (!EditCurves) return FALSE;
			if (EditCurves->num <= 0)
							 {
							 put_message("line-no-sample");
							 (void) sleep(1);
							 return FALSE;
							 }
			break;

		case FpaC_SCATTERED:
			if (!EditPoints) return FALSE;
			if (EditPoints->num <= 0)
							 {
							 put_message("point-no-sample");
							 (void) sleep(1);
							 return FALSE;
							 }
			break;

		case FpaC_LCHAIN:
			if (!EditLchains) return FALSE;
			if (EditLchains->num <= 0)
							 {
							 put_message("lchain-no-sample");
							 (void) sleep(1);
							 return FALSE;
							 }
			break;

		case FpaC_CONTINUOUS:
		case FpaC_VECTOR:
		default:
			return FALSE;
		}

	/* Repeat for each point */
	for (ip=0; ip<np; ip++)
		{
		switch (ftype)
			{
			case FpaC_DISCRETE:
			case FpaC_WIND:
				if (same_ic(attrib, AttribAll))
					{
					build_area_attrib_box_sample(pbuf[ip]);
					}
				else
					{
					build_area_attrib_sample(pbuf[ip], attrib);
					}
				break;

			case FpaC_LINE:
				if (same_ic(attrib, AttribAll))
					{
					build_line_attrib_box_sample(pbuf[ip]);
					}
				else
					{
					build_line_attrib_sample(pbuf[ip], attrib);
					}
				break;

			case FpaC_SCATTERED:
				if (same_ic(attrib, AttribAll))
					{
					build_point_attrib_box_sample(pbuf[ip]);
					}
				else
					{
					build_point_attrib_sample(pbuf[ip], attrib);
					}
				break;

			case FpaC_LCHAIN:
				if (same_ic(attrib, AttribAll))
					{
					build_lchain_attrib_box_sample(pbuf[ip]);
					}
				else
					{
					build_lchain_attrib_sample(pbuf[ip], attrib);
					}
				break;
			}
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     c o n s t r u c t _ w i n d _ s a m p l e                        *
*                                                                      *
***********************************************************************/

static	LOGICAL	construct_wind_sample

	(
	FLD_DESCRIPT				*fd,
	FpaConfigSampleStruct		*sdef,
	FpaConfigCrossRefStruct		*xref,
	int							np,
	POINT						*pbuf
	)

	{
	float	*dirs, *spds, *gusts;
	STRING	units;
	LOGICAL	valid;
	int		ip;

	/* Remove display from last sample box */
	clear_sample_box();

	/* Return if nothing to sample */
	if (sdef && !xref)
		{
		if (blank(sdef->samp_func))     return FALSE;
		if (!find_retrieve_metasfc(fd)) return FALSE;
		}

	dirs  = INITMEM(float, np);
	spds  = INITMEM(float, np);
	gusts = INITMEM(float, np);

	/* Evaluate the wind direction(s) and speed(s) */
	if (xref) valid = extract_awind_by_crossref(xref->name, fd, FALSE, np, pbuf,
								MapProj->clon, dirs, spds, gusts, &units);
	else      valid = extract_awind(1, fd, FALSE, np, pbuf, MapProj->clon,
								dirs, spds, gusts, &units);

	if (!valid)
		{
		FREEMEM(dirs);
		FREEMEM(spds);
		FREEMEM(gusts);
		return FALSE;
		}

	for (ip=0; ip<np; ip++)
		{
		/* >>> pass gust <<< */
		build_wind_barb_sample(pbuf[ip], dirs[ip], spds[ip], units);
		}

	FREEMEM(dirs);
	FREEMEM(spds);
	FREEMEM(gusts);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     c o n s t r u c t _ f i e l d _ l a b e l _ s a m p l e          *
*                                                                      *
***********************************************************************/

static	LOGICAL	construct_field_label_sample

	(
	FLD_DESCRIPT	*fd,
	POINT			pos
	)

	{
	int		ftype;
	SPOT	spot, tspot;
	CURVE	scurve;
    MARK    mark;

	/* Remove display from last sample box */
	clear_sample_box();

	/* Return if nothing to sample */
	if (!find_retrieve_metasfc(fd)) return FALSE;

	/* Verify that we have what we need for the type of field */
	ftype = fd->edef->fld_type;
	switch (ftype)
		{
		case FpaC_CONTINUOUS:
		case FpaC_VECTOR:
		case FpaC_DISCRETE:
		case FpaC_WIND:
		case FpaC_LINE:
			if (!EditLabs)                     return FALSE;
			if (!same(EditLabs->type, "spot")) return FALSE;
			if (EditLabs->num <= 0)
				 {
				 put_message("label-no-show");
				 (void) sleep(1);
				 return FALSE;
				 }
			break;

		default:
			return FALSE;
		}

	/* Find the closest label to the selected point */
	spot = closest_spot(EditLabs, pos, "!legend", "!legend", NullFloat, NULL);
	if (!spot) return FALSE;

	/* Highlight the field label */
	tspot = copy_spot(spot);
	highlight_item("spot", (ITEM)tspot, 2);
	add_item_to_metafile(SampleMeta, "spot", "d", "box", "box", (ITEM) tspot);

	/* Construct line to field label location */
	scurve = create_curve("", "", "");
	add_point_to_curve(scurve, pos);
	add_point_to_curve(scurve, tspot->anchor);
	define_lspec(&scurve->lspec, EditColour, 0, NULL, False, 0.0, 0.0,
				 (HILITE) 0);
	add_item_to_metafile(SampleMeta, "curve", "c", "box", "box", (ITEM) scurve);
	mark = create_mark("", "", "", pos, 0.0);
	define_mspec(&mark->mspec, EditColour, 2, NULL, 0, False, 5.0, 0.0,
				 (HILITE) 2);
	add_item_to_metafile(SampleMeta, "mark",  "d", "box", "box", (ITEM) mark);

	/* Display label parameters */
	sample_box(TRUE, spot->attrib);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     c o n s t r u c t _ l c h a i n _ n o d e _ s a m p l e          *
*                                                                      *
***********************************************************************/

static	LOGICAL	construct_lchain_node_sample

	(
	FLD_DESCRIPT	*fd,
	POINT			pos
	)

	{
	int			ftype;
	LCHAIN		lchain;
	POINT		cpos;
	LMEMBER		ltype;
	int			inode, mplus;
	STRING		vtime;
	ATTRIB_LIST	xatt;
	CURVE		scurve;
    MARK    	mark;

	/* Remove display from last sample box */
	clear_sample_box();

	/* Return if nothing to sample */
	if (!find_retrieve_metasfc(fd)) return FALSE;

	/* Verify that we have what we need for the type of field */
	ftype = fd->edef->fld_type;
	switch (ftype)
		{
		case FpaC_LCHAIN:
			if (!EditLchains) return FALSE;
			if (EditLchains->num <= 0)
				 {
				 put_message("lchain-no-node-show");
				 (void) sleep(1);
				 return FALSE;
				 }
			break;

		default:
			return FALSE;
		}

	/* Find the closest link node */
	lchain = closest_lchain_node(EditLchains, pos, NullFloat, cpos,
				&ltype, &inode);
	if (!lchain) return FALSE;

	/* Highlight the node */
	/* >>>>>
	tspot = copy_spot(spot);
	highlight_item("spot", (ITEM)tspot, 2);
	add_item_to_metafile(SampleMeta, "spot", "d", "box", "box", (ITEM) tspot);
	<<<<< */

	/* Construct line to link node location */
	scurve = create_curve("", "", "");
	add_point_to_curve(scurve, pos);
	add_point_to_curve(scurve, cpos);
	define_lspec(&scurve->lspec, EditColour, 0, NULL, False, 0.0, 0.0,
				 (HILITE) 0);
	add_item_to_metafile(SampleMeta, "curve", "c", "box", "box", (ITEM) scurve);
	mark = create_mark("", "", "", pos, 0.0);
	define_mspec(&mark->mspec, EditColour, 2, NULL, 0, False, 5.0, 0.0,
				 (HILITE) 2);
	add_item_to_metafile(SampleMeta, "mark",  "d", "box", "box", (ITEM) mark);

	/* Make a copy of the node attributes */
	if      (ltype == LchainNode)
		{
		mplus = lchain->nodes[inode]->mplus;
		xatt  = copy_attrib_list(lchain->nodes[inode]->attrib);
		}
	else if (ltype == LchainControl)
		{
		mplus = lchain->nodes[inode]->mplus;
		xatt  = copy_attrib_list(lchain->nodes[inode]->attrib);
		}
	else if (ltype == LchainFloating)
		{
		mplus = lchain->nodes[inode]->mplus;
		xatt  = copy_attrib_list(lchain->nodes[inode]->attrib);
		}
	else if (ltype == LchainInterp)
		{
		mplus = lchain->interps[inode]->mplus;
		xatt  = copy_attrib_list(lchain->interps[inode]->attrib);
		}
	else
		return FALSE;

	/* Add the node timestamp to the attributes */
	vtime = calc_valid_time_minutes(lchain->xtime, 0, mplus);
	CAL_add_attribute(xatt, AttribLnodeTstamp, vtime);

	/* Add the node motion to the attributes */
	CAL_add_lchain_node_motion(xatt, MapProj, lchain, mplus);

	/* Display link chain node parameters based on node type */
	sample_box(TRUE, xatt);

	/* Destroy the copied node attributes */
	xatt = destroy_attrib_list(xatt);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     c o n s t r u c t _ i m a g e _ s a m p l e                      *
*                                                                      *
***********************************************************************/

static	LOGICAL	construct_image_sample

	(
	Image		image,
	STRING		item,
	int			np,
	POINT		*pbuf
	)

	{
	int				ip;
	float			*xpos, *ypos, *values;
	POINT			lpos;
	LABEL			label;
	MARK			mark;

	/* Verify that that image exists */
	if (!image) return FALSE;

	if (np < 1) return FALSE;
	if (!pbuf)  return FALSE;

	/* Create arrays for sample locations */
	xpos = INITMEM(float, np);
	ypos = INITMEM(float, np);
	for (ip=0; ip<np; ip++)
		{
		xpos[ip] = pbuf[ip][X];
		ypos[ip] = pbuf[ip][Y];
		}

	/* Sample the imagery for all sample locations */
	values = glImageSampleArray(image, item, xpos, ypos, np);

	/* Construct sample labels for each point */
	for (ip=0; ip<np; ip++)
		{

		/* Construct the label */
		lpos[X] = pbuf[ip][X] + EditXoff;
		lpos[Y] = pbuf[ip][Y] + EditYoff;
		if (values[ip] == glDATA_MISSING)
			(void) sprintf(msg, "N/A");
		else
			(void) sprintf(msg, "%.1f", values[ip]);
		label = create_label("", "", msg, lpos, 0.0);
		define_tspec(&label->tspec, EditColour, EditFont, False, EditLsize, 0.0,
					 Hl, Vc, (HILITE) 0);
		add_item_to_metafile(SampleMeta, "label", "d", "", "", (ITEM) label);

		/* Construct mark */
		mark = create_mark("", "", "", pbuf[ip], 0.0);
		define_mspec(&mark->mspec, EditColour, 7, NULL, 0, False, 5.0, 0.0,
					 (HILITE) 2);
		add_item_to_metafile(SampleMeta, "mark",  "d", "", "", (ITEM) mark);
		}

	FREEMEM(xpos);
	FREEMEM(ypos);
	FREEMEM(values);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     b u i l d _ s p l i n e _ v a l u e _ s a m p l e                *
*     b u i l d _ s p l i n e _ g r a d i e n t _ s a m p l e          *
*     b u i l d _ s p l i n e _ c u r v a t u r e _ s a m p l e        *
*     b u i l d _ v e c t o r _ m a g n i t u d e _ s a m p l e        *
*     b u i l d _ v e c t o r _ d i r e c t i o n _ s a m p l e        *
*     b u i l d _ v e c t o r _ v a l u e _ s a m p l e                *
*     b u i l d _ a r e a _ a t t r i b _ s a m p l e                  *
*     b u i l d _ a r e a _ a t t r i b _ b o x _ s a m p l e          *
*     b u i l d _ l i n e _ a t t r i b _ s a m p l e                  *
*     b u i l d _ l i n e _ a t t r i b _ b o x _ s a m p l e          *
*     b u i l d _ p o i n t _ a t t r i b _ s a m p l e                *
*     b u i l d _ p o i n t _ a t t r i b _ b o x _ s a m p l e        *
*     b u i l d _ l c h a i n _ a t t r i b _ s a m p l e              *
*     b u i l d _ l c h a i n _ a t t r i b _ b o x _ s a m p l e      *
*     b u i l d _ w i n d _ b a r b _ s a m p l e                      *
*                                                                      *
***********************************************************************/

static	void	build_spline_value_sample

	(
	POINT	pos
	)

	{
	double	value;
	POINT	lpos;
	LABEL	label;
	MARK	mark;
	LOGICAL	valid;

	/* Interpolate value at selected point */
	valid = eval_sfc(EditSfc, pos, &value);
	if (!valid) return;

	/* Construct label */
	lpos[X] = pos[X] + EditXoff;
	lpos[Y] = pos[Y] + EditYoff;
	(void) sprintf(msg, "%.1f", value);
	label = create_label("", "", msg, lpos, 0.0);
	define_tspec(&label->tspec, EditColour, EditFont, False, EditLsize, 0.0,
				 Hl, Vc, (HILITE) 0);
	add_item_to_metafile(SampleMeta, "label", "d", "", "", (ITEM) label);

	/* Construct mark at sample location */
	mark = create_mark("", "", "", pos, 0.0);
	define_mspec(&mark->mspec, EditColour, 7, NULL, 0, False, 5.0, 0.0,
				 (HILITE) 2);
	add_item_to_metafile(SampleMeta, "mark",  "d", "", "", (ITEM) mark);
	}

/**********************************************************************/

static	void	build_spline_gradient_sample

	(
	POINT	pos
	)

	{
    double  gradx, grady;
    POINT   lpos, epos;
    LOGICAL valid;
    LABEL   label;
    MARK    mark;
    CURVE   curve;

	/* Interpolate gradient at selected point */
	valid = eval_sfc_1st_deriv(EditSfc, pos, &gradx, &grady);
	if (!valid) return;

	gradx *= EditFact;
	grady *= EditFact;

	/* Construct label */
	if (EditFullSam)
		{
		lpos[X] = pos[X] + EditXoff;
		lpos[Y] = pos[Y] + EditYoff;
		(void) sprintf(msg, "(%.2f, %.2f)", gradx, grady);
		label = create_label("", "", msg, lpos, 0.0);
		define_tspec(&label->tspec, EditColour, EditFont, False, EditLsize, 0.0,
					 Hl, Vc, (HILITE) 0);
		add_item_to_metafile(SampleMeta, "label", "d", "", "", (ITEM) label);
		}

	/* Construct gradient vector */
	epos[X] = pos[X] + gradx*FilterRes/5;
	epos[Y] = pos[Y] + grady*FilterRes/5;
	curve = create_curve("", "", "");
	add_point_to_curve(curve, pos);
	add_point_to_curve(curve, epos);
	define_lspec(&curve->lspec, EditColour, 0, NULL, False, 2.0, 0.0,
				 (HILITE) 0);
	add_item_to_metafile(SampleMeta, "curve", "c", "", "", (ITEM) curve);

	/* Construct mark at sample location */
	mark = create_mark("", "", "", pos, 0.0);
	define_mspec(&mark->mspec, EditColour, 7, NULL, 0, False, 5.0, 0.0,
				 (HILITE) 2);
	add_item_to_metafile(SampleMeta, "mark",  "d", "", "", (ITEM) mark);
	}

/**********************************************************************/

static	void	build_spline_curvature_sample

	(
	POINT	pos
	)

	{
	double  value;
	float   ang, ang0, radius;
	POINT   lpos, cpos, p;
	int     iarc;
	LOGICAL valid;
	LABEL   label;
	MARK    mark;
	CURVE   curve;

	/* Interpolate curvature at selected point */
	valid = eval_sfc_curvature(EditSfc, pos, &value, cpos);
	if (!valid) return;

	/* Construct curve */
	if (EditFullSam)
		{
		curve = create_curve("", "", "");
		define_lspec(&curve->lspec, EditColour, 0, NULL, False, 0.0, 0.0,
					 (HILITE) 0);
		radius = fabs(1/value);
		ang0   = atan2(pos[Y]-cpos[Y], pos[X]-cpos[X]);
		add_point_to_curve(curve, pos);
		add_point_to_curve(curve, cpos);
		for (iarc=-45; iarc<=45; iarc+=5)
			{
			ang = ang0 + iarc*RAD;
			p[X] = cpos[X] + radius*cos(ang);
			p[Y] = cpos[Y] + radius*sin(ang);
			add_point_to_curve(curve, p);
			}
		add_point_to_curve(curve, cpos);
		add_item_to_metafile(SampleMeta, "curve", "c", "", "", (ITEM) curve);
		}

	/* Construct label */
	lpos[X] = pos[X] + EditXoff;
	lpos[Y] = pos[Y] + EditYoff;
	(void) sprintf(msg, "%.4f", value);
	label = create_label("", "", msg, lpos, 0.0);
	define_tspec(&label->tspec, EditColour, EditFont, False, EditLsize, 0.0,
				 Hl, Vc, (HILITE) 0);
	add_item_to_metafile(SampleMeta, "label", "d", "", "", (ITEM) label);

	/* Construct mark at sample location */
	mark  = create_mark("", "", "", pos, 0.0);
	define_mspec(&mark->mspec, EditColour, 7, NULL, 0, False, 5.0, 0.0,
				 (HILITE) 2);
	add_item_to_metafile(SampleMeta, "mark",  "d", "", "", (ITEM) mark);
	}

/**********************************************************************/

static	void	build_vector_magnitude_sample

	(
	POINT	pos
	)

	{
	double	mag;
	POINT	lpos;
	LABEL	label;
	MARK	mark;
	LOGICAL	valid;

	/* Interpolate value at selected point */
	valid = eval_sfc_MD(EditSfc, pos, &mag, NullDouble);
	if (!valid) return;

	/* Construct label */
	lpos[X] = pos[X] + EditXoff;
	lpos[Y] = pos[Y] + EditYoff;
	(void) sprintf(msg, "%.1f", mag);
	label = create_label("", "", msg, lpos, 0.0);
	define_tspec(&label->tspec, EditColour, EditFont, False, EditLsize, 0.0,
				 Hl, Vc, (HILITE) 0);
	add_item_to_metafile(SampleMeta, "label", "d", "", "", (ITEM) label);

	/* Construct mark at sample location */
	mark = create_mark("", "", "", pos, 0.0);
	define_mspec(&mark->mspec, EditColour, 7, NULL, 0, False, 5.0, 0.0,
				 (HILITE) 2);
	add_item_to_metafile(SampleMeta, "mark",  "d", "", "", (ITEM) mark);
	}

/**********************************************************************/

static	void	build_vector_direction_sample

	(
	POINT	pos
	)

	{
	double	dir;
	POINT	lpos;
	LABEL	label;
	MARK	mark;
	LOGICAL	valid;

	/* Interpolate value at selected point */
	valid = eval_sfc_MD(EditSfc, pos, NullDouble, &dir);
	if (!valid) return;

	/* Construct label */
	lpos[X] = pos[X] + EditXoff;
	lpos[Y] = pos[Y] + EditYoff;
	(void) sprintf(msg, "%.1f", dir);
	label = create_label("", "", msg, lpos, 0.0);
	define_tspec(&label->tspec, EditColour, EditFont, False, EditLsize, 0.0,
				 Hl, Vc, (HILITE) 0);
	add_item_to_metafile(SampleMeta, "label", "d", "", "", (ITEM) label);

	/* Construct mark at sample location */
	mark = create_mark("", "", "", pos, 0.0);
	define_mspec(&mark->mspec, EditColour, 7, NULL, 0, False, 5.0, 0.0,
				 (HILITE) 2);
	add_item_to_metafile(SampleMeta, "mark",  "d", "", "", (ITEM) mark);
	}

/**********************************************************************/

static	void	build_vector_value_sample

	(
	POINT	pos
	)

	{
	double	uval, vval;
	POINT	lpos;
	LABEL	label;
	MARK	mark;
	LOGICAL	valid;

	/* Interpolate value at selected point */
	valid = eval_sfc_UV(EditSfc, pos, &uval, &vval);
	if (!valid) return;

	/* Construct label */
	lpos[X] = pos[X] + EditXoff;
	lpos[Y] = pos[Y] + EditYoff;
	(void) sprintf(msg, "(%.1f, %.1f)", uval, vval);
	label = create_label("", "", msg, lpos, 0.0);
	define_tspec(&label->tspec, EditColour, EditFont, False, EditLsize, 0.0,
				 Hl, Vc, (HILITE) 0);
	add_item_to_metafile(SampleMeta, "label", "d", "", "", (ITEM) label);

	/* Construct mark at sample location */
	mark = create_mark("", "", "", pos, 0.0);
	define_mspec(&mark->mspec, EditColour, 7, NULL, 0, False, 5.0, 0.0,
				 (HILITE) 2);
	add_item_to_metafile(SampleMeta, "mark",  "d", "", "", (ITEM) mark);
	}

/**********************************************************************/

static	void	build_area_attrib_sample

	(
	POINT	pos,
	STRING	name
	)

	{
	float	size;
    STRING  lab;
	SUBAREA	*slist;
	CAL		calx, *clist;
    int     nlist, ilist, blist;
    POINT   lpos;
    LABEL   label;
    MARK    mark;
	LOGICAL	first;

	/* Find the enclosing area, otherwise use background value */
	nlist = eval_areaset_list(EditAreas, pos, PickFirst, &slist, &clist);
	if (nlist <= 0) return;

	/* Construct label */
	lpos[X] = pos[X] + EditXoff;
	lpos[Y] = pos[Y] + EditYoff;
	(void) strcpy(buf, "");
	blist = (EditFullSam)? nlist: 1;
	first = TRUE;
	for (ilist=0; ilist<blist; ilist++)
		{
		calx = CAL_duplicate(clist[ilist]);
		CAL_add_location(calx, MapProj, pos);
		if (NotNull(slist[ilist]))
			{
			subarea_properties(slist[ilist], NullLogical, &size, NullFloat);
			if (size > 0.0) CAL_add_area_size(calx, MapProj, pos, size);
			}
		lab = CAL_get_attribute(calx, name);
		if (CAL_no_value(lab)) lab = SafeStr(lab);
		(void) sprintf(msg, " %s", lab);
		CAL_destroy(calx);
		if (!first) (void) strcat(buf, "\r\n");
		first = FALSE;
		(void) strcat(buf, msg);
		}
	label = create_label("", "", buf, lpos, 0.0);
	define_tspec(&label->tspec, EditColour, EditFont, False, EditLsize, 0.0,
				 Hl, Vc, (HILITE) 0);
	add_item_to_metafile(SampleMeta, "label", "d", "", "", (ITEM) label);
	FREEMEM(clist);

	/* Construct mark at sample location */
	mark = create_mark("", "", "", pos, 0.0);
	define_mspec(&mark->mspec, EditColour, 7, NULL, 0, False, 5.0, 0.0,
				 (HILITE) 2);
	add_item_to_metafile(SampleMeta, "mark",  "d", "", "", (ITEM) mark);
	}

/**********************************************************************/

static	void	build_area_attrib_box_sample

	(
	POINT	pos
	)

	{
	float	size;
	SUBAREA	sub;
	CAL		cal, calx;
	MARK	mark;

	/* Find the enclosing area, otherwise use background value */
	(void) eval_areaset(EditAreas, pos, PickFirst, &sub, &cal);
	calx = CAL_duplicate(cal);
	CAL_add_location(calx, MapProj, pos);
	if (NotNull(sub))
		{
		subarea_properties(sub, NullLogical, &size, NullFloat);
		if (size > 0.0) CAL_add_area_size(calx, MapProj, pos, size);
		}
	sample_box(TRUE, calx);
	CAL_destroy(calx);

	/* Construct mark at sample location */
	mark = create_mark("", "", "", pos, 0.0);
	define_mspec(&mark->mspec, EditColour, 8, NULL, 0, False, 10.0, 0.0,
				 (HILITE) 2);
	add_item_to_metafile(SampleMeta, "mark",  "d", "box", "box", (ITEM) mark);
	}

/**********************************************************************/

static	void	build_line_attrib_sample

	(
	POINT	pos,
	STRING	name
	)

	{
	CURVE	curve, scurve;
	POINT   cpos, lpos, spos, epos;
	int		seg;
	HAND	sense;
	LOGICAL	right, sym;
	MARK    mark;
	STRING  lab;
	CAL		calx;
	LABEL   label;

	/* Find the closest line, otherwise sample nothing */
	curve = closest_curve(EditCurves, pos, NullFloat, cpos, &seg);
	if (IsNull(curve))
		{
		put_message("sample-no-eval");
		return;
		}
	if (IsNull(curve->attrib))
		{
		put_message("sample-no-eval");
		return;
		}

	/* Construct label */
	lpos[X] = cpos[X] + EditXoff;
	lpos[Y] = cpos[Y] + EditYoff;
	calx = CAL_duplicate(curve->attrib);
	CAL_add_location(calx, MapProj, cpos);
	(void) recall_curve_sense(curve, &sense);
	if (!get_pattern_info(curve->lspec.pattern, &sym)) sym = TRUE;
	(void) curve_test_point(curve, pos,
			NullFloat, NullPoint, NullInt, NullLogical, &right);
	if ( sym && right )
		CAL_add_negative_proximity(calx, MapProj, pos, cpos);
	else if ( sym && !right )
		CAL_add_proximity(calx, MapProj, pos, cpos);
	else if ( (sense == Right && right) || (sense == Left && !right) )
		CAL_add_negative_proximity(calx, MapProj, pos, cpos);
	else
		CAL_add_proximity(calx, MapProj, pos, cpos);
	if (line_span_info(curve->line, seg, spos, epos, NullPoint) > 0.0)
		CAL_add_line_dir(calx, MapProj, spos, epos);
	CAL_add_line_len(calx, MapProj, curve->line);
	lab = CAL_get_attribute(calx, name);
	(void) sprintf(msg, " %s", lab);
	CAL_destroy(calx);

	label = create_label("", "", msg, lpos, 0.0);
	define_tspec(&label->tspec, EditColour, EditFont, False, EditLsize, 0.0,
				 Hl, Vc, (HILITE) 0);
	add_item_to_metafile(SampleMeta, "label", "d", "", "", (ITEM) label);

	/* Construct mark at sample location */
	scurve = create_curve("", "", "");
	add_point_to_curve(scurve, pos);
	add_point_to_curve(scurve, cpos);
	define_lspec(&scurve->lspec, EditColour, 0, NULL, False, 0.0, 0.0,
				 (HILITE) 0);
	add_item_to_metafile(SampleMeta, "curve", "c", "", "", (ITEM) scurve);
	mark = create_mark("", "", "", pos, 0.0);
	define_mspec(&mark->mspec, EditColour, 3, NULL, 0, False, 5.0, 0.0,
				 (HILITE) 2);
	add_item_to_metafile(SampleMeta, "mark",  "d", "", "", (ITEM) mark);
	mark = create_mark("", "", "", cpos, 0.0);
	define_mspec(&mark->mspec, EditColour, 7, NULL, 0, False, 5.0, 0.0,
				 (HILITE) 2);
	add_item_to_metafile(SampleMeta, "mark",  "d", "", "", (ITEM) mark);
	}

/**********************************************************************/

static	void	build_line_attrib_box_sample

	(
	POINT	pos
	)

	{
	CURVE	curve, scurve;
	POINT	cpos, spos, epos;
	int		seg;
	HAND	sense;
	LOGICAL	right, sym;
	CAL		calx;
	MARK	mark;

	/* Find the closest line, otherwise sample nothing */
	curve = closest_curve(EditCurves, pos, NullFloat, cpos, &seg);
	if (IsNull(curve))
		{
		put_message("sample-no-eval");
		(void) sleep(1);
		return;
		}
	if (IsNull(curve->attrib))
		{
		put_message("sample-no-eval");
		(void) sleep(1);
		return;
		}

	/* Display attributes */
	calx = CAL_duplicate(curve->attrib);
	CAL_add_location(calx, MapProj, cpos);
	(void) recall_curve_sense(curve, &sense);
	if (!get_pattern_info(curve->lspec.pattern, &sym)) sym = TRUE;
	(void) curve_test_point(curve, pos,
			NullFloat, NullPoint, NullInt, NullLogical, &right);
	if ( sym && right )
		CAL_add_negative_proximity(calx, MapProj, pos, cpos);
	else if ( sym && !right )
		CAL_add_proximity(calx, MapProj, pos, cpos);
	else if ( (sense == Right && right) || (sense == Left && !right) )
		CAL_add_negative_proximity(calx, MapProj, pos, cpos);
	else
		CAL_add_proximity(calx, MapProj, pos, cpos);
	if (line_span_info(curve->line, seg, spos, epos, NullPoint) > 0.0)
		CAL_add_line_dir(calx, MapProj, spos, epos);
	CAL_add_line_len(calx, MapProj, curve->line);
	sample_box(TRUE, calx);
	CAL_destroy(calx);

	/* Construct mark and line to sample location */
	scurve = create_curve("", "", "");
	add_point_to_curve(scurve, pos);
	add_point_to_curve(scurve, cpos);
	define_lspec(&scurve->lspec, EditColour, 0, NULL, False, 0.0, 0.0,
				 (HILITE) 0);
	add_item_to_metafile(SampleMeta, "curve", "c", "box", "box", (ITEM) scurve);
	mark = create_mark("", "", "", pos, 0.0);
	define_mspec(&mark->mspec, EditColour, 2, NULL, 0, False, 5.0, 0.0,
				 (HILITE) 2);
	add_item_to_metafile(SampleMeta, "mark",  "d", "box", "box", (ITEM) mark);
	mark = create_mark("", "", "", cpos, 0.0);
	define_mspec(&mark->mspec, EditColour, 8, NULL, 0, False, 10.0, 0.0,
				 (HILITE) 2);
	add_item_to_metafile(SampleMeta, "mark",  "d", "box", "box", (ITEM) mark);
	}

/**********************************************************************/

static	void	build_point_attrib_sample

	(
	POINT	pos,
	STRING	name
	)

	{
	SPOT	spot;
	CURVE	scurve;
    POINT   cpos, lpos;
    MARK    mark;
    STRING  lab;
	CAL		calx;
    LABEL   label;

	/* Find the closest spot, otherwise sample nothing */
	spot = closest_spot(EditPoints, pos, NullString, NullString, NullFloat,
				cpos);
	if (IsNull(spot))
		{
		put_message("sample-no-eval");
		return;
		}
	if (IsNull(spot->attrib))
		{
		put_message("sample-no-eval");
		return;
		}

	/* Construct label */
	lpos[X] = cpos[X] + EditXoff;
	lpos[Y] = cpos[Y] + EditYoff;
	calx = CAL_duplicate(spot->attrib);
	CAL_add_location(calx, MapProj, spot->anchor);
	CAL_add_proximity(calx, MapProj, pos, spot->anchor);
	lab = CAL_get_attribute(calx, name);
	(void) sprintf(msg, " %s", lab);
	CAL_destroy(calx);

	label = create_label("", "", msg, lpos, 0.0);
	define_tspec(&label->tspec, EditColour, EditFont, False, EditLsize, 0.0,
				 Hl, Vc, (HILITE) 0);
	add_item_to_metafile(SampleMeta, "label", "d", "", "", (ITEM) label);

	/* Construct mark and line to sample location */
	scurve = create_curve("", "", "");
	add_point_to_curve(scurve, pos);
	add_point_to_curve(scurve, cpos);
	define_lspec(&scurve->lspec, EditColour, 0, NULL, False, 0.0, 0.0,
				 (HILITE) 0);
	add_item_to_metafile(SampleMeta, "curve", "c", "", "", (ITEM) scurve);
	mark = create_mark("", "", "", pos, 0.0);
	define_mspec(&mark->mspec, EditColour, 3, NULL, 0, False, 5.0, 0.0,
				 (HILITE) 2);
	add_item_to_metafile(SampleMeta, "mark",  "d", "", "", (ITEM) mark);
	mark = create_mark("", "", "", cpos, 0.0);
	define_mspec(&mark->mspec, EditColour, 7, NULL, 0, False, 5.0, 0.0,
				 (HILITE) 2);
	add_item_to_metafile(SampleMeta, "mark",  "d", "", "", (ITEM) mark);
	}

/**********************************************************************/

static	void	build_point_attrib_box_sample

	(
	POINT	pos
	)

	{
	SPOT	spot;
	CURVE	scurve;
	POINT	cpos;
	CAL		calx;
	MARK	mark;

	/* Find the closest spot, otherwise sample nothing */
	spot = closest_spot(EditPoints, pos, NullString, NullString, NullFloat,
				cpos);
	if (IsNull(spot))
		{
		put_message("sample-no-eval");
		(void) sleep(1);
		return;
		}
	if (IsNull(spot->attrib))
		{
		put_message("sample-no-eval");
		(void) sleep(1);
		return;
		}

	/* Display attributes */
	calx = CAL_duplicate(spot->attrib);
	CAL_add_location(calx, MapProj, spot->anchor);
	CAL_add_proximity(calx, MapProj, pos, spot->anchor);
	sample_box(TRUE, calx);
	CAL_destroy(calx);

	/* Construct mark and line to sample location */
	scurve = create_curve("", "", "");
	add_point_to_curve(scurve, pos);
	add_point_to_curve(scurve, cpos);
	define_lspec(&scurve->lspec, EditColour, 0, NULL, False, 0.0, 0.0,
				 (HILITE) 0);
	add_item_to_metafile(SampleMeta, "curve", "c", "box", "box", (ITEM) scurve);
	mark = create_mark("", "", "", pos, 0.0);
	define_mspec(&mark->mspec, EditColour, 2, NULL, 0, False, 5.0, 0.0,
				 (HILITE) 2);
	add_item_to_metafile(SampleMeta, "mark",  "d", "box", "box", (ITEM) mark);
	mark = create_mark("", "", "", cpos, 0.0);
	define_mspec(&mark->mspec, EditColour, 8, NULL, 0, False, 10.0, 0.0,
				 (HILITE) 2);
	add_item_to_metafile(SampleMeta, "mark",  "d", "box", "box", (ITEM) mark);
	}

/**********************************************************************/

static	void	build_lchain_attrib_sample

	(
	POINT	pos,
	STRING	name
	)

	{
	LCHAIN	lchain;
	POINT   cpos, lpos;
	int		iseg;
	MARK    mark;
	STRING  lab;
	CAL		cal, calx;
	STRING	vtime;
	CURVE	scurve;
	LABEL   label;

	/* Find the closest link chain, otherwise sample nothing */
	lchain = closest_lchain(EditLchains, pos, NullFloat, cpos, &iseg);
	if (IsNull(lchain))
		{
		put_message("sample-no-eval");
		return;
		}
	cal = lchain->attrib;
	if (IsNull(cal))
		{
		put_message("sample-no-eval");
		return;
		}

	/* Construct label */
	lpos[X] = cpos[X] + EditXoff;
	lpos[Y] = cpos[Y] + EditYoff;
	calx = CAL_duplicate(cal);
	vtime = calc_valid_time_minutes(lchain->xtime, 0, lchain->splus);
	CAL_add_attribute(calx, AttribLchainStartTstamp, vtime);
	vtime = calc_valid_time_minutes(lchain->xtime, 0, lchain->eplus);
	CAL_add_attribute(calx, AttribLchainEndTstamp, vtime);
	CAL_add_location(calx, MapProj, cpos);
	lab = CAL_get_attribute(calx, name);
	(void) sprintf(msg, " %s", lab);
	CAL_destroy(calx);

	label = create_label("", "", msg, lpos, 0.0);
	define_tspec(&label->tspec, EditColour, EditFont, False, EditLsize, 0.0,
				 Hl, Vc, (HILITE) 0);
	add_item_to_metafile(SampleMeta, "label", "d", "", "", (ITEM) label);

	/* Construct mark at sample location */
	scurve = create_curve("", "", "");
	add_point_to_curve(scurve, pos);
	add_point_to_curve(scurve, cpos);
	define_lspec(&scurve->lspec, EditColour, 0, NULL, False, 0.0, 0.0,
				 (HILITE) 0);
	add_item_to_metafile(SampleMeta, "curve", "c", "", "", (ITEM) scurve);
	mark = create_mark("", "", "", pos, 0.0);
	define_mspec(&mark->mspec, EditColour, 3, NULL, 0, False, 5.0, 0.0,
				 (HILITE) 2);
	add_item_to_metafile(SampleMeta, "mark",  "d", "", "", (ITEM) mark);
	mark = create_mark("", "", "", cpos, 0.0);
	define_mspec(&mark->mspec, EditColour, 7, NULL, 0, False, 5.0, 0.0,
				 (HILITE) 2);
	add_item_to_metafile(SampleMeta, "mark",  "d", "", "", (ITEM) mark);
	}

/**********************************************************************/

static	void	build_lchain_attrib_box_sample

	(
	POINT	pos
	)

	{
	LCHAIN	lchain;
	POINT   cpos;
	int		iseg;
	CAL		cal, calx;
	STRING	vtime;
	CURVE	scurve;
	MARK	mark;

	/* Find the closest link chain, otherwise sample nothing */
	lchain = closest_lchain(EditLchains, pos, NullFloat, cpos, &iseg);
	if (IsNull(lchain))
		{
		put_message("sample-no-eval");
		return;
		}
	cal = lchain->attrib;
	if (IsNull(cal))
		{
		put_message("sample-no-eval");
		return;
		}

	/* Display attributes */
	calx = CAL_duplicate(cal);
	vtime = calc_valid_time_minutes(lchain->xtime, 0, lchain->splus);
	CAL_add_attribute(calx, AttribLchainStartTstamp, vtime);
	vtime = calc_valid_time_minutes(lchain->xtime, 0, lchain->eplus);
	CAL_add_attribute(calx, AttribLchainEndTstamp, vtime);
	CAL_add_location(calx, MapProj, cpos);
	sample_box(TRUE, calx);
	CAL_destroy(calx);

	/* Construct mark and line to sample location */
	scurve = create_curve("", "", "");
	add_point_to_curve(scurve, pos);
	add_point_to_curve(scurve, cpos);
	define_lspec(&scurve->lspec, EditColour, 0, NULL, False, 0.0, 0.0,
				 (HILITE) 0);
	add_item_to_metafile(SampleMeta, "curve", "c", "box", "box", (ITEM) scurve);
	mark = create_mark("", "", "", pos, 0.0);
	define_mspec(&mark->mspec, EditColour, 2, NULL, 0, False, 5.0, 0.0,
				 (HILITE) 2);
	add_item_to_metafile(SampleMeta, "mark",  "d", "box", "box", (ITEM) mark);
	mark = create_mark("", "", "", cpos, 0.0);
	define_mspec(&mark->mspec, EditColour, 8, NULL, 0, False, 10.0, 0.0,
				 (HILITE) 2);
	add_item_to_metafile(SampleMeta, "mark",  "d", "box", "box", (ITEM) mark);
	}

/**********************************************************************/

static	void	build_wind_barb_sample

	(
	POINT	pos,
	float	dir,
	float	spd,
	STRING	units
	)

	{
	BARB	barb;
	MARK	mark;
	LOGICAL	valid;

	/* Construct wind barb */
	spd  = (float) convert_wind((double)spd, units, &valid);
	barb = create_barb("", pos, dir, spd, 0.0);
	mark = create_mark("", "", "", pos, 0.0);
	define_bspec(&barb->bspec, EditColour, BarbWind, False, 2.0, EditBsize,
				 False, False, 0.0, 0.0, WindUnits, (HILITE) 0);
	define_mspec(&mark->mspec, EditColour, 3, NULL, 0, False, 0.0, 0.0,
				 (HILITE) 2);

	add_item_to_metafile(SampleMeta, "barb", "d", "", "", (ITEM) barb);
	add_item_to_metafile(SampleMeta, "mark", "d", "", "", (ITEM) mark);
	}
