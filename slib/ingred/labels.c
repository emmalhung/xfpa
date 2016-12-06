/***********************************************************************
*                                                                      *
*     l a b e l s . c                                                  *
*                                                                      *
*     This module of the INteractive GRaphics EDitor (INGRED)          *
*     handles operations on companion fields to the principal edit     *
*     fileds (i.e. labels, high and low marks, wind barbs, etc.).      *
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

#undef DEBUG_LABELS

/***********************************************************************
*                                                                      *
*     l a b e l _ t y p e _ d e s c r i p t i o n s                    *
*                                                                      *
*     Define all the supported label types and describe how to build   *
*     their individual members.                                        *
*                                                                      *
*     Later, this will only define the built-in labels, which will     *
*     be merged with external label type definitions.                  *
*                                                                      *
***********************************************************************/

/* Label type descriptions */
typedef	struct	ldef_struct
	{
	STRING	type;
	STRING	mclass;
	SPFEAT	attach;
	LOGICAL	entry;
	LOGICAL	modify;
	CAL		cal;
	} LAB_DEF;

/* Descriptions fpr built-in labelling modes */
static	LAB_DEF	LabDefs[] =
	{
		{ "auto-spline",       "none",    AttachAuto,    FALSE, FALSE, NullCal },
		{ "auto-area",         "none",    AttachAuto,    FALSE, FALSE, NullCal },
		{ "auto-wind",         "none",    AttachAuto,    FALSE, FALSE, NullCal },
		{ "auto-adjust",       "none",    AttachAuto,    FALSE, FALSE, NullCal },
		{ "auto-line",         "none",    AttachAuto,    FALSE, FALSE, NullCal },
		{ "auto-scat",         "none",    AttachAuto,    FALSE, FALSE, NullCal },
		{ FpaDefLabContour,    "contour", AttachContour, FALSE, FALSE, NullCal },
		{ FpaDefLabLowAtMin,   "hilo",    AttachMin,     FALSE, FALSE, NullCal },
		{ FpaDefLabHighAtMax,  "hilo",    AttachMax,     FALSE, FALSE, NullCal },
		{ FpaDefLabArea,       "area",    AttachNone,    FALSE, FALSE, NullCal },
		{ FpaDefLabWind,       "wind",    AttachNone,    FALSE, FALSE, NullCal },
		{ FpaDefLabAdjustment, "area",    AttachNone,    FALSE, FALSE, NullCal },
		{ FpaDefLabLine,       "line",    AttachLine,    FALSE, FALSE, NullCal },
		{ FpaDefLabLegend,     "legend",  AttachNone,    FALSE, FALSE, NullCal },
		/* >>>>>
		{ "auto-spline",    "none",    AttachAuto,    FALSE, FALSE, NullCal },
		{ "auto-area",      "none",    AttachAuto,    FALSE, FALSE, NullCal },
		{ "auto-wind",      "none",    AttachAuto,    FALSE, FALSE, NullCal },
		{ "auto-adjust",    "none",    AttachAuto,    FALSE, FALSE, NullCal },
		{ "auto-line",      "none",    AttachAuto,    FALSE, FALSE, NullCal },
		{ "auto-scat",      "none",    AttachAuto,    FALSE, FALSE, NullCal },
		{ "contour",        "contour", AttachContour, FALSE, FALSE, NullCal },
		{ "low_at_min",     "hilo",    AttachMin,     FALSE, FALSE, NullCal },
		{ "high_at_max",    "hilo",    AttachMax,     FALSE, FALSE, NullCal },
		{ "area",           "area",    AttachNone,    FALSE, FALSE, NullCal },
		{ "wind",           "wind",    AttachNone,    FALSE, FALSE, NullCal },
		{ "adjustment",     "area",    AttachNone,    FALSE, FALSE, NullCal },
		{ "line",           "line",    AttachLine,    FALSE, FALSE, NullCal },
		{ "legend",         "legend",  AttachNone,    FALSE, FALSE, NullCal },
		<<<<< */
	};
static	int		NLabDefs = ( sizeof(LabDefs) / sizeof(LAB_DEF) );

/* Predefined attributes for built-in labelling modes */
typedef	struct	alist_struct
	{
	STRING	type;
	ATTRIB	adef;
	} ATT_LIST;
static	ATT_LIST	AttList[] =
		{
			{ FpaDefLabContour,   "EVAL_contour", "yes"  },
			{ FpaDefLabLowAtMin,  "hilo_type",    "low"  },
			{ FpaDefLabLowAtMin,  "EVAL_spval",   "yes"  },
			{ FpaDefLabHighAtMax, "hilo_type",    "high" },
			{ FpaDefLabHighAtMax, "EVAL_spval",   "yes"  },
			{ FpaDefLabWind,      "EVAL_wind",    "yes"  },
			/* >>>>>
			{ "contour", 	    "EVAL_contour", "yes"  },
			{ "low_at_min",     "hilo_type",    "low"  },
			{ "low_at_min",     "EVAL_spval",   "yes"  },
			{ "high_at_max",    "hilo_type",    "high" },
			{ "high_at_max",    "EVAL_spval",   "yes"  },
			{ "wind",           "EVAL_wind",    "yes"  },
			<<<<< */
		};
static	int		NAttList = ( sizeof(AttList) / sizeof(ATT_LIST) );

static	LOGICAL	LabDefsReady = FALSE;

/***********************************************************************
*                                                                      *
*     Static function declarationse                                    *
*                                                                      *
***********************************************************************/

static	LOGICAL	clear_all_labels(void);
static	LOGICAL	construct_spline_label(POINT, LAB_DEF *);
static	LOGICAL	construct_area_label(POINT, LAB_DEF *);
static	LOGICAL	construct_wind_label(POINT, LAB_DEF *);
static	LOGICAL	construct_line_label(POINT, LAB_DEF *);
static	LOGICAL	add_spline_label(LAB_DEF *, POINT, STRING, ITEM);
static	LOGICAL	add_area_label(LAB_DEF *, POINT, STRING, SUBAREA, CAL);
static	LOGICAL	add_wind_label(LAB_DEF *, POINT, WIND_VAL *, STRING, SUBAREA,
							CAL);
static	LOGICAL	add_line_label(LAB_DEF *, POINT, STRING, CURVE, CAL);
static	LOGICAL	recalc_spline_label(SURFACE, SET, SPOT, LOGICAL *);
static	LOGICAL	recalc_area_label(SET, SET, SPOT, LOGICAL *);
static	LOGICAL	recalc_wind_label(SET, SET, SPOT, LOGICAL *);
static	LOGICAL	recalc_line_label(SET, SET, SPOT, LOGICAL *);
static	void	retain_modified_attribs(SPOT);
static	LOGICAL	eval_spline_label(SURFACE, STRING, ITEM, SPOT);
static	LOGICAL	eval_area_label(SET, STRING, SPOT);
static	LOGICAL	eval_wind_label(SET, WIND_VAL *, SPOT);
static	LOGICAL	eval_line_label(SET, STRING, SPOT);
static	void	build_spot_pspecs(SPOT, LOGICAL, COLOUR, HILITE, float, float);
static	void	check_spot_attribs(SPOT);
static	void	remember_current_label(SPOT);
static	void	forget_current_label(void);
static	void	get_current_label_value(void);
static	LOGICAL	set_current_label_value(CAL);
static	void	remember_current_cal(CAL);
static	void	update_current_cal(void);
static	void	forget_current_cal(void);
static	LAB_DEF	*find_label_type_info(STRING, FLD_DESCRIPT *);
static	LAB_DEF	*find_label_type(STRING, FLD_DESCRIPT *);
static	LAB_DEF	*find_preset_label_type(STRING);
static	LOGICAL	same_label_type(STRING, STRING);
static	STRING	equiv_label_type(STRING, LOGICAL);

/***********************************************************************
*                                                                      *
*     l a b e l _ a d d                                                *
*                                                                      *
***********************************************************************/

LOGICAL	label_add

	(
	STRING	mode,
	STRING	ltype,
	CAL		cal
	)

	{
	LAB_DEF	*ldef;
	POINT	pos;
	int		butt;
	LOGICAL	drawn = FALSE;
	LOGICAL	ok, attach;

#	ifdef DEBUG_LABELS
	pr_diag("Editor", "[label_add] %s\n", mode);
#	endif /* DEBUG_LABELS */

	/* Clean up previous edits */
	if (same_start(mode, "cancel"))
		{
		ignore_partial();
		forget_current_label();
		return FALSE;
		}

	/* Clear all labels on request */
	if (same(mode, "clear"))
		{
		forget_current_label();
		return clear_all_labels();
		}

	/* Do we already have the info about this label type? */
	ldef = find_label_type_info(ltype, &EditFd);
	if (IsNull(ldef)) return FALSE;

	/* Make sure main field and spot label field are there */
	if (!EditLabs)                        return FALSE;
	if (!same(EditLabs->type, "spot"))    return FALSE;

	/* Initialize on startup */
	if (same(mode, "begin"))
		{
		forget_current_cal();
		forget_current_label();
		}

	/* Set label values if requested */
	if (same(mode, "set"))
		{
		if (IsNull(cal)) return FALSE;
		drawn |= set_current_label_value(cal);
		/* >>> reset cal to raw */
		cal = NullCal;
		forget_current_label();
		if (drawn) present_all();
		}

	attach = (LOGICAL) (ldef->attach != AttachNone);

	/* Now label the field */
	/* Repeat until told to quit */
	while (TRUE)
	    {
	    /* Put up a prompt */
		if (attach) put_message("label-feature");
		else        put_message("label-point");

		/* See if clear is allowed */
		if (set_count(EditLabs, "!legend") > 0) edit_can_clear(TRUE);
		else                                    edit_can_clear(FALSE);

		/* Get the point */
		if (!ready_Xpoint(DnEdit, pos, &butt)) return drawn;
		forget_current_label();
		if ((butt != LeftButton))
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}

		/* Left button - Inserting a label */
		else
			{
			/* Putting a new label/high/low */
			if (attach) put_message("label-pick-feature");
			else        put_message("label-pick-point");
			(void) pick_Xpoint(DnEdit, 0, pos, &butt);
			if (!inside_dn_window(DnEdit, pos))
				{
				put_message("edit-outside-map");
				(void) sleep(1);
				continue;
				}

			/* Label nearest requested feature */
			ok = FALSE;
			switch (EditFd.edef->fld_type)
				{
				case FpaC_VECTOR:
				case FpaC_CONTINUOUS:
						ok = construct_spline_label(pos, ldef);
						break;
				case FpaC_DISCRETE:
						ok = construct_area_label(pos, ldef);
						break;
				case FpaC_WIND:
						ok = construct_wind_label(pos, ldef);
						break;
				case FpaC_LINE:
						ok = construct_line_label(pos, ldef);
						break;
				}
			if (ok)
				{
				drawn = TRUE;
				if (ldef->entry)
					{
					present_all();
					update_current_cal();
					get_current_label_value();
					post_partial(TRUE);
					return drawn;
					}
				else
					{
					ignore_partial();
					}
				}
			}

		/* Re-display if necessary */
		if (drawn) present_all();
	    }
	}

/***********************************************************************
*                                                                      *
*     l a b e l _ m o v e                                              *
*                                                                      *
***********************************************************************/

LOGICAL	label_move

	(
	STRING	mode
	)

	{
	float	sdist, dx, dy;
	POINT	pos, cpos;
	SET		tset;
	SPOT	spot, tspot;
	int		butt;
	LOGICAL	drawn = FALSE;
	LOGICAL	del;

#	ifdef DEBUG_LABELS
	pr_diag("Editor", "[label_move] %s\n", mode);
#	endif /* DEBUG_LABELS */

	/* Clean up previous edits */
	if (same_start(mode, "cancel"))
		{
		forget_current_label();
		return FALSE;
		}

	/* Clear all labels on request */
	if (same(mode, "clear"))
		{
		forget_current_label();
		return clear_all_labels();
		}

	/* Make sure main field and spot label field are there */
	switch (EditFd.edef->fld_type)
		{
		case FpaC_VECTOR:
		case FpaC_CONTINUOUS:	if (!EditSfc)    return FALSE;
								break;
		case FpaC_DISCRETE:
		case FpaC_WIND:			if (!EditAreas)  return FALSE;
								break;
		case FpaC_LINE:			if (!EditCurves) return FALSE;
								break;
		}
	if (!EditLabs)                        return FALSE;
	if (!same(EditLabs->type, "spot"))    return FALSE;

	/* Repeat until told to quit */
	while (TRUE)
	    {
	    /* Put up a prompt */
		/* See if there are any labels left */
		if (set_count(EditLabs, "!legend") > 0)
			{
			put_message("label-move");
			edit_can_clear(TRUE);
			}
		else
			{
			put_message("label-no-move");
			edit_can_clear(FALSE);
			return drawn;
			}

		/* Get the point */
		if (!ready_Xpoint(DnEdit, pos, &butt)) return drawn;
		forget_current_label();
		if ((butt != LeftButton))
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}

		/* Left button - Moving a label */
		else
			{
			/* Find the closest label to the selected point */
			spot = closest_spot(EditLabs, pos, "!legend", "!legend", &sdist,
						NULL);
			if (!spot) continue;
			dx = spot->anchor[X] - pos[X];
			dy = spot->anchor[Y] - pos[Y];

			/* Build a tracking marker */
			tset  = create_set("spot");
			tspot = copy_spot(spot);
			add_item_to_set(tset, (ITEM)tspot);
			highlight_set(tset, 2);
			display_set(tset);

			/* Get the new location */
			put_message("label-move-rel");
			(void) utrack_Xpoint(DnEdit, tset, NullPoint, cpos, &butt);
			(void) destroy_set(tset);
			cpos[X] += dx;
			cpos[Y] += dy;
			if (!inside_dn_window(DnEdit, cpos))
				{
				put_message("label-move-out3");
				present_all();
				(void) sleep(1);
				continue;
				}

			/* Prevent multiple labels at the same location */
			tspot = closest_spot(EditLabs, cpos, "!legend", "!legend", &sdist,
						NULL);
			if (NotNull(tspot) && tspot!=spot && sdist<PickTol)
				{
				put_message("label-move-mult");
				present_all();
				(void) sleep(1);
				continue;
				}

			/* Move the label */
			put_message("label-moving");
			define_spot_anchor(spot, cpos);

			switch (EditFd.edef->fld_type)
				{
				case FpaC_VECTOR:
				case FpaC_CONTINUOUS:
					(void) recalc_spline_label(EditSfc, EditLabs, spot, &del);
					break;

				case FpaC_DISCRETE:
					(void) recalc_area_label(EditAreas, EditLabs, spot, &del);
					break;

				case FpaC_WIND:
					(void) recalc_wind_label(EditAreas, EditLabs, spot, &del);
					break;

				case FpaC_LINE:
					(void) recalc_line_label(EditCurves, EditLabs, spot, &del);
					break;
				}
			
			if (del)
				{
				put_message("label-deleting");
				(void) sleep(1);
				}

			invoke_set_catspecs(EditLabs);
			if (EditColour >= 0) change_set_pspec(EditLabs, TEXT_COLOUR,
									(POINTER)&EditColour);
			if (EditUndoable) highlight_set(EditLabs, 1);
			drawn = TRUE;
			if (EditUndoable) post_mod("labs");
			}

		/* Re-display if necessary */
		if (drawn) present_all();
	    }
	}

/***********************************************************************
*                                                                      *
*     l a b e l _ s h o w                                              *
*                                                                      *
***********************************************************************/

LOGICAL	label_show

	(
	STRING	mode
	)

	{
	POINT	pos;
	SPOT	spot, tspot;
	int		butt;
	LOGICAL	drawn = FALSE;

#	ifdef DEBUG_LABELS
	pr_diag("Editor", "[label_show] %s\n", mode);
#	endif /* DEBUG_LABELS */

	/* Clean up previous edits */
	if (same_start(mode, "cancel"))
		{
		ignore_partial();
		empty_temp();
		forget_current_label();
		sample_box(FALSE, NullCal);
		return FALSE;
		}

	/* Clear all labels on request */
	if (same(mode, "clear"))
		{
		ignore_partial();
		empty_temp();
		forget_current_label();
		sample_box(FALSE, NullCal);
		return clear_all_labels();
		}

	/* Make sure main field and spot label field are there */
	switch (EditFd.edef->fld_type)
		{
		case FpaC_VECTOR:
		case FpaC_CONTINUOUS:	if (!EditSfc)    return FALSE;
								break;
		case FpaC_DISCRETE:
		case FpaC_WIND:			if (!EditAreas)  return FALSE;
								break;
		case FpaC_LINE:			if (!EditCurves) return FALSE;
								break;
		}
	if (!EditLabs)                        return FALSE;
	if (!same(EditLabs->type, "spot"))    return FALSE;

	/* Repeat until told to quit */
	allow_obscured_input(TRUE);
	while (TRUE)
	    {
	    /* Put up a prompt */
		/* See if there are any labels left */
		if (set_count(EditLabs, "!legend") > 0)
			{
			put_message("label-show");
			edit_can_clear(TRUE);
			}
		else
			{
			put_message("label-no-show");
			edit_can_clear(FALSE);
			return drawn;
			}

		/* Get the point */
		if (!ready_Xpoint(DnEdit, pos, &butt)) return drawn;
		forget_current_label();
		if ((butt != LeftButton))
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}

		/* Left button - Showing a label */
		else
			{
			(void) pick_Xpoint(DnEdit, 0, pos, &butt);
			if (!inside_dn_window(DnEdit, pos))
				{
				put_message("edit-outside-map");
				(void) sleep(1);
				continue;
				}

			/* Find the closest label to the selected point */
			spot = closest_spot(EditLabs, pos, "!legend", "!legend", NullFloat,
						NULL);
			if (!spot) continue;

			/* Build a tracking marker */
			ignore_partial();
			empty_temp();
			tspot = copy_spot(spot);
			highlight_item("spot", (ITEM)tspot, 2);
			add_item_to_metafile(TempMeta, "spot", "d", "", "", (ITEM)tspot);
			present_temp(TRUE);
			post_partial(TRUE);

			/* Display label parameters */
			sample_box(TRUE, spot->attrib);
			remember_current_cal(spot->attrib);
			}

		/* Re-display if necessary */
		if (drawn) present_all();
	    }
	}

/***********************************************************************
*                                                                      *
*     l a b e l _ m o d i f y                                          *
*                                                                      *
***********************************************************************/

LOGICAL	label_modify

	(
	STRING	mode,
	STRING	ltype,
	CAL		cal
	)

	{
	LAB_DEF	*ldef;
	POINT	pos, spos, epos;
	SPOT	spot, tspot;
	CURVE	curve;
	int		butt, seg;
	LOGICAL	drawn = FALSE;
	STRING	type;

#	ifdef DEBUG_LABELS
	pr_diag("Editor", "[label_modify] %s\n", mode);
#	endif /* DEBUG_LABELS */

	/* Clean up previous edits */
	if (same_start(mode, "cancel"))
		{
		ignore_partial();
		empty_temp();
		forget_current_label();
		return FALSE;
		}

	/* Clear all labels on request */
	if (same(mode, "clear"))
		{
		ignore_partial();
		empty_temp();
		forget_current_label();
		return clear_all_labels();
		}

	/* Do we already have the info about this label type? */
	ldef = find_label_type_info(ltype, &EditFd);
	if (IsNull(ldef)) return FALSE;

	/* Obtain CAL for "set" mode */
	if (same(mode, "set"))
		{
		if (IsNull(cal)) return FALSE;

		drawn |= set_current_label_value(cal);
		/* >>> reset cal to raw */
		cal = NullCal;
		forget_current_label();
		ignore_partial();
		empty_temp();
		if (drawn) present_all();
		}

	/* Make sure spot label field is there */
	if (!EditLabs)                     return FALSE;
	if (!same(EditLabs->type, "spot")) return FALSE;

	/* Now modify labels */
	/* Repeat until told to quit */
	while (TRUE)
	    {
	    /* Put up a prompt */
		/* See if there are any labels left */
		if (set_count(EditLabs, "!legend") > 0)
			{
			put_message("label-mod");
			edit_can_clear(TRUE);
			}
		else
			{
			put_message("label-no-mod");
			edit_can_clear(FALSE);
			return drawn;
			}

		/* Get the point */
		if (!ready_Xpoint(DnEdit, pos, &butt)) return drawn;
		forget_current_label();
		if ((butt != LeftButton))
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}

		/* Left button - Modifying a label */
		else
			{
			(void) pick_Xpoint(DnEdit, 0, pos, &butt);
			if (!inside_dn_window(DnEdit, pos))
				{
				put_message("edit-outside-map");
				(void) sleep(1);
				continue;
				}

			/* Find the closest label to the selected point */
			spot = closest_spot(EditLabs, pos, "!legend", "!legend", NullFloat,
						NULL);
			if (!spot) continue;
			type = CAL_get_attribute(spot->attrib, AttribLabelType);
			if (!same_label_type(type, ltype))
				{
				put_message("label-mod-wrong");
				(void) sleep(1);
				continue;
				}

			/* Add line direction at point and line length for line objects */
			if (EditFd.edef->fld_type == FpaC_LINE)
				{
				curve = closest_curve(EditCurves, spot->anchor, NULL, NULL, &seg);
				if (line_span_info(curve->line, seg, spos, epos, NullPoint) > 0.0)
					CAL_add_line_dir((CAL) spot->attrib, MapProj, spos, epos);
				CAL_add_line_len((CAL) spot->attrib, MapProj, curve->line);
				}

			/* Invoke "label" type rules */
			CAL_invoke_label_rules_by_name(spot->attrib, EditFd.edef->name,
				EditFd.ldef->name);

			/* Build a tracking marker */
			ignore_partial();
			empty_temp();
			tspot = copy_spot(spot);
			highlight_item("spot", (ITEM)tspot, 2);
			add_item_to_metafile(TempMeta, "spot", "d", "", "", (ITEM)tspot);
			present_temp(TRUE);
			post_partial(TRUE);

			drawn = TRUE;
			if (ldef->modify)
				{
				remember_current_label(spot);
				forget_current_cal();
				get_current_label_value();
				return drawn;
				}
			else
				{
				ignore_partial();
				empty_temp();
				}

			invoke_set_catspecs(EditLabs);
			if (EditColour >= 0) change_set_pspec(EditLabs, TEXT_COLOUR,
									(POINTER)&EditColour);
			if (EditUndoable) highlight_set(EditLabs, 1);
			drawn = TRUE;
			if (EditUndoable) post_mod("labs");
			}

		/* Re-display if necessary */
		if (drawn) present_all();
	    }
	}

/***********************************************************************
*                                                                      *
*     l a b e l _ d e l e t e                                          *
*                                                                      *
***********************************************************************/

LOGICAL	label_delete
	(
	STRING	mode
	)

	{
	POINT	pos;
	SPOT	spot;
	int		butt;
	LOGICAL	drawn = FALSE;

#	ifdef DEBUG_LABELS
	pr_diag("Editor", "[label_delete] %s\n", mode);
#	endif /* DEBUG_LABELS */

	/* Clean up previous edits */
	if (same_start(mode, "cancel"))
		{
		forget_current_label();
		return FALSE;
		}

	/* Clear all labels on request */
	if (same(mode, "clear"))
		{
		forget_current_label();
		return clear_all_labels();
		}

	/* Make sure main field and spot label field are there */
	switch (EditFd.edef->fld_type)
		{
		case FpaC_VECTOR:
		case FpaC_CONTINUOUS:	if (!EditSfc)    return FALSE;
								break;
		case FpaC_DISCRETE:
		case FpaC_WIND:			if (!EditAreas)  return FALSE;
								break;
		case FpaC_LINE:			if (!EditCurves) return FALSE;
								break;
		}
	if (!EditLabs)                        return FALSE;
	if (!same(EditLabs->type, "spot"))    return FALSE;

	/* Repeat until told to quit */
	while (TRUE)
	    {
	    /* Put up a prompt */
		/* See if there are any labels left */
		if (set_count(EditLabs, "!legend") > 0)
			{
			put_message("label-del");
			edit_can_clear(TRUE);
			}
		else
			{
			put_message("label-no-del");
			edit_can_clear(FALSE);
			return drawn;
			}

		/* Get the point */
		if (!ready_Xpoint(DnEdit, pos, &butt)) return drawn;
		forget_current_label();
		if ((butt != LeftButton))
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}

		/* Left button - Deleting a label */
		else
			{
			(void) pick_Xpoint(DnEdit, 0, pos, &butt);
			if (!inside_dn_window(DnEdit, pos))
				{
				put_message("edit-outside-map");
				(void) sleep(1);
				continue;
				}

			/* Find the closest label to the selected point */
			spot = closest_spot(EditLabs, pos, "!legend", "!legend", NullFloat,
						NULL);
			if (!spot) continue;

			/* Delete the label */
			put_message("label-deleting");
			remove_item_from_set(EditLabs, (ITEM) spot);
			drawn = TRUE;
			if (EditUndoable) post_mod("labs");
			}

		/* Re-display if necessary */
		if (drawn) present_all();
	    }
	}

/***********************************************************************
*                                                                      *
*     c l e a r _ a l l _ l a b e l s                                  *
*                                                                      *
***********************************************************************/

static	LOGICAL	clear_all_labels(void)

	{
	int		i;
	LOGICAL	drawn=FALSE;
	STRING	cat;
	SPOT	spot;

	forget_current_label();

	/* Delete all labels except for legend */
	if (EditLabs && set_count(EditLabs, NULL) > 0)
		{
		for (i=EditLabs->num-1; i>=0; i--)
			{
			spot = (SPOT) EditLabs->list[i];
			if (!spot) continue;
			if (same(spot->mclass,"legend")) continue;
			cat = CAL_get_attribute(spot->attrib, CALcategory);
			if (same(cat,"legend")) continue;
			remove_item_from_set(EditLabs, (ITEM)spot);
			drawn = TRUE;
			if (EditUndoable) post_mod("labs");
			}
		}

	/* Re-display if necessary */
	if (drawn) present_all();
	return drawn;
	}

/***********************************************************************
*                                                                      *
*     m a k e _ d e p i c t _ l e g e n d                              *
*     m a k e _ g u i d _ l e g e n d                                  *
*     m a k e _ l e g e n d                                            *
*                                                                      *
***********************************************************************/

LOGICAL	make_depict_legend

	(
	SET		labels,
	DFLIST	*dfld,
	STRING	mtime,
	COLOUR	colour,
	float	xval,
	float	yval
	)

	{
	STRING	fmt, elem, level;

	static const STRING	dfmt = "DAILY %s %s - Valid %s";
	static const STRING	sfmt = "STATIC %s %s - Updated %sZ";

	/* Make sure label field is there */
	if (IsNull(labels))              return FALSE;
	if (!same(labels->type, "spot")) return FALSE;
	if (IsNull(dfld))                return FALSE;
	if (!tdep_special(dfld->tdep))   return FALSE;

	/* Find out if the label set contains a proper legend */
#	ifdef LATER
	if (blank(mtime))
		{
		SPOT	legend = NullSpot;

		legend = closest_spot(labels, ZeroPoint, "legend", "legend",
					NullFloat, NullPoint);
		}
#	endif

	/* If there is a legend try to get the valid time decription */
	elem  = dfld->element;
	level = dfld->level;

	/* Daily fields */
	if (tdep_daily(dfld->tdep))
		{
		char	date[20];
		(void) sscanf(mtime, "%s", date);
		fmt = XuGetMdbLine("daily", "legend");
		if (same(fmt, "daily"))
			(void) sprintf(Msg, dfmt, level, elem, date);
		else
			(void) sprintf(Msg, fmt, level, elem, date);
		XuFree(fmt);
		}

	/* Static fields */
	else if (tdep_static(dfld->tdep))
		{
		fmt = XuGetMdbLine("static", "legend");
		if (same(fmt, "static"))
			(void) sprintf(Msg, sfmt, level, elem, mtime);
		else
			(void) sprintf(Msg, fmt, level, elem, mtime);
		XuFree(fmt);
		}

	/* Don't need a legend */
	else return FALSE;

	/* >>>>> 2007 <<<<< */
	/* Transfer depiction legend to panel for display */
	/* >>>>> 2007 <<<<< */

	setup_set_presentation(labels, elem, level, "FPA");
	return make_legend(labels, Msg, colour, LgndSize, xval, yval);
	}

/**********************************************************************/

LOGICAL	make_legend

	(
	SET		labels,
	STRING	legend,
	COLOUR	colour,
	float	size,
	float	xval,
	float	yval
	)

	{
	int		i;
	SPOT	spot;
	POINT	pos;
	CAL		cal;
	LAB_DEF	*ldef;
	LOGICAL	ok;

	/* Make sure label field is there */
	if (IsNull(labels))              return FALSE;
	if (!same(labels->type, "spot")) return FALSE;
	if (blank(legend))               return FALSE;

	/* Destroy existing legend if there */
	for (i=labels->num-1; i>=0; i--)
		{
		spot = (SPOT) labels->list[i];
		if (IsNull(spot) || same(spot->mclass, "legend"))
			remove_item_from_set(labels, (ITEM) spot);
		}

	/* >>>>> for testing <<<<< */
#	ifdef DEBUG_LABELS
	pr_diag("Editor", "[make_legend] Legend: %s   Colour: %d   Size: %.2f\n",
		legend, colour, size);
#	endif /* DEBUG_LABELS */
	/* >>>>> for testing <<<<< */

	/* Put on the legend if provided */
	set_point(pos, xval, yval);
	ldef = find_preset_label_type("legend");
	if (!ldef) return FALSE;
	cal  = CAL_create_default();
	spot = create_spot(pos, ldef->mclass, ldef->attach, cal);
	cal  = CAL_destroy(cal);
	cal  = spot->attrib;
	CAL_add_attribute(cal, CALcategory,  "legend");
	CAL_add_attribute(cal, CALuserlabel, legend);

	/* Add and define its members */
	if (colour < 0) colour = find_colour("black", &ok);
	build_spot_members(spot, labels->ncspec, labels->cspecs);
	build_spot_pspecs(spot, TRUE, colour, 0, size, WbarbSize);

	add_item_to_set(labels, (ITEM) spot);
	invoke_set_catspecs(labels);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     g e n e r a t e _ s u r f a c e _ l a b s                        *
*     g e n e r a t e _ a r e a s e t _ l a b s                        *
*     g e n e r a t e _ c u r v e s e t _ l a b s                      *
*     g e n e r a t e _ s p o t s e t _ l a b s                        *
*                                                                      *
***********************************************************************/

LOGICAL	generate_surface_labs

	(
	SURFACE	sfc,
	SET		labels,
	float	size,
	float	xval,
	float	yinc,
	float	ymax
	)

	{
	float	yy;
	POINT	pos, lpos;
	STRING	value;
	char	which;
	ITEM	iold, inew;
	CURVE	curve;
	SPOT	spot;
	CAL		lcal;
	LAB_DEF	*ldef;
	COLOUR	colour;
	HILITE	hilite;
	LOGICAL	valid;
	LOGICAL	modified = FALSE;

	/* Make sure surface is there */
	if (IsNull(sfc))                 return FALSE;
	if (IsNull(labels))              return FALSE;
	if (!same(labels->type, "spot")) return FALSE;

	ldef = find_preset_label_type("contour");
	if (IsNull(ldef)) return FALSE;

	/* Repeat along specified line */
	iold = NullItem;
	for (yy=yinc; yy<ymax; yy+=yinc)
	    {
	    /* Find the closest contour to the selected point */
		set_point(pos, xval, yy);
	    value = eval_sfc_feature(sfc, pos, "c", lpos, &which, &inew, &valid);
	    if (!valid)       continue;
	    if (inew == iold) continue;
	    iold = inew;

	    /* Define the value */
		curve  = (CURVE) iold;
		colour = curve->lspec.colour;
		hilite = curve->lspec.hilite;

		/* Create SPOT object to hold contour information */
		spot = create_spot(lpos, ldef->mclass, ldef->attach, curve->attrib);
		lcal = spot->attrib;

		/* Add label type and default information */
		CAL_add_attribute(lcal, AttribLabelType, ldef->type);
		CAL_merge(lcal, ldef->cal, FALSE);

		/* Add and define its members */
		build_spot_members(spot, labels->ncspec, labels->cspecs);
		build_spot_pspecs(spot, FALSE, colour, hilite, size, WbarbSize);
		(void) eval_spline_label(sfc, value, iold, spot);
	    add_item_to_set(labels, (ITEM) spot);

	    modified = TRUE;
	    }

	invoke_set_catspecs(labels);
	return modified;
	}

/**********************************************************************/

LOGICAL	generate_curveset_labs

	(
	SET		curves,
	SET		labels,
	float	size
	)

	{
	CURVE	curve;
	SPOT	spot;
	int		nseg = 4;
	float	seg;
	int		i, ip, np;
	STRING	value;
	CAL		lcal;
	LAB_DEF	*ldef;
	COLOUR	colour;
	HILITE	hilite;
	LINE	line;
	POINT	lpos;
	LOGICAL	modified = FALSE;

	/* Make sure sets contain the right stuff */
	if (IsNull(curves))               return FALSE;
	if (IsNull(labels))               return FALSE;
	if (!same(curves->type, "curve")) return FALSE;
	if (!same(labels->type, "spot"))  return FALSE;
	if (curves->num < 1)              return FALSE;

	ldef = find_preset_label_type("line");
	if (IsNull(ldef)) return FALSE;

	/* Repeat for each curve */
	for (i=0; i<curves->num; i++)
	    {
	    /* Retrieve the curve */
	    curve = (CURVE) curves->list[i];
	    if (IsNull(curve)) continue;
		value = CAL_get_attribute(curve->attrib, CALuserlabel);
	    if (blank(value))  continue;

	    /* Pick a point on the curve to display the label */
		line = curve->line;
		if (IsNull(line)) continue;
	    np  = line->numpts;
	    seg = i%nseg + .5;
	    ip  = NINT(np*seg/nseg);
		copy_point(lpos, line->points[ip]);

	    /* Define the value */
		colour = curve->lspec.colour;
		hilite = curve->lspec.hilite;

		spot = create_spot(lpos, ldef->mclass, ldef->attach, curve->attrib);
		lcal = spot->attrib;
		CAL_add_attribute(lcal, AttribLabelType, ldef->type);
		CAL_merge(lcal, ldef->cal, FALSE);

		/* Add and define its members */
		build_spot_members(spot, labels->ncspec, labels->cspecs);
		build_spot_pspecs(spot, FALSE, colour, hilite, size, WbarbSize);
		(void) eval_line_label(curves, value, spot);
	    add_item_to_set(labels, (ITEM) spot);

	    modified = TRUE;
	    }

	invoke_set_catspecs(labels);
	return modified;
	}

/***********************************************************************
*                                                                      *
*     r e c o m p u t e _ s u r f a c e _ l a b s                      *
*     r e c o m p u t e _ a r e a s e t _ l a b s                      *
*     r e c o m p u t e _ c u r v e s e t _ l a b s                    *
*     r e c o m p u t e _ s p o t s e t _ l a b e l s                  *
*                                                                      *
***********************************************************************/

LOGICAL	recompute_surface_labs

	(
	SURFACE	sfc,
	SET	    spots,
	LOGICAL	use_active
	)

	{
	int		i, imem;
	COLOUR	dcolour;
	SPOT	spot;
	SPMEM	*mem;
	int		modified = FALSE;

	/* Make sure sets contain the right stuff */
	if (IsNull(spots))              return FALSE;
	if (!same(spots->type, "spot")) return FALSE;
	if (spots->num < 1)             return FALSE;

	/* Get default colour for the surface */
	if (NotNull(sfc)) recall_surface_pspec(sfc, LINE_COLOUR, (POINTER)&dcolour);
	else              dcolour = 1;

	/* Fool the equation database into using the new surface */
	/* (Only needed to re-calculate wind labels) */
	if (!use_active)
		{
		FIELD	fld;
		SURFACE	sf2;

		fld = create_field("a", EditFd.edef->name, EditFd.ldef->name);
		/* fld = create_field("a", ActiveField->element, ActiveField->level); */
		sf2 = copy_surface(sfc, FALSE);
		change_surface_units(sf2, &MKS_UNITS);
		define_fld_data(fld, "surface", (POINTER)sf2);
		replace_field_in_equation_database(&EditFd, &(EditFd.mproj), fld);
		}

	/* Repeat for each spot */
	for (i=spots->num-1; i>=0; i--)
	    {
	    /* Retrieve the spot */
	    spot = (SPOT) spots->list[i];
	    if (IsNull(spot)) continue;

		/* Reset legend to proper colour if necessary */
	    if (same(spot->mclass, "legend"))
			{
			for (imem=0; imem<spot->nmem; imem++)
				{
				mem = spot->members + imem;
				if (mem->tspec.colour == dcolour) continue;
				modified = TRUE;
				mem->tspec.colour = dcolour;
				}
			continue;
			}

		/* Re-define the label */
		modified |= recalc_spline_label(sfc, spots, spot, 0);
	    }

	/* Remove the new surface from the equation database */
	if (!use_active)
		{
		delete_field_in_equation_database(&EditFd);
		}

	invoke_set_catspecs(spots);
	return modified;
	}

/**********************************************************************/

LOGICAL	recompute_areaset_labs

	(
	SET		areas,
	SET	    spots,
	LOGICAL	use_active
	)

	{
	int		i, imem;
	SPMEM	*mem;
	COLOUR	dcolour;
	SPOT	spot;
	LOGICAL	modified = FALSE;

	/* Make sure sets contain the right stuff */
	if (IsNull(spots))              return FALSE;
	if (!same(spots->type, "spot")) return FALSE;
	if (spots->num < 1)             return FALSE;

	/* Get default colour for the area set */
	if (NotNull(areas)) recall_set_pspec(areas, LINE_COLOUR, (POINTER)&dcolour);
	else                dcolour = 1;

	/* Fool the equation database into using the new area or wind field */
	/* (Only needed to re-calculate wind labels) */
	if (!use_active)
		{
		FIELD			fld;
		SET				set2;
		FLD_DESCRIPT	*fd;

		fld = create_field("b", EditFd.edef->name, EditFd.ldef->name);
		/* fld = create_field("b", ActiveField->element, ActiveField->level); */
		set2 = copy_set(areas);
		define_fld_data(fld, "set", (POINTER)set2);
		if (EditFd.edef->fld_type == FpaC_WIND) fd = &WindFd;
		else                                    fd = &EditFd;

		replace_field_in_equation_database(fd, &(EditFd.mproj), fld);
		}

	/* Repeat for each spot */
	for (i=spots->num-1; i>=0; i--)
	    {
	    /* Retrieve the label */
	    spot = (SPOT) spots->list[i];
	    if (IsNull(spot)) continue;

		/* Reset legend to proper colour if necessary */
	    if (same(spot->mclass, "legend"))
			{
			for (imem=0; imem<spot->nmem; imem++)
				{
				mem = spot->members + imem;
				if (mem->tspec.colour == dcolour) continue;
				modified = TRUE;
				mem->tspec.colour = dcolour;
				}
			continue;
			}
		
		/* Re-define the label */
		switch (EditFd.edef->fld_type)
			{
			case FpaC_DISCRETE:
					modified |= recalc_area_label(areas, spots, spot, 0);
					break;

			case FpaC_WIND:
					modified |= recalc_wind_label(areas, spots, spot, 0);
					break;
			}
	    }

	/* Remove the new field from the equation database */
	if (!use_active)
		{
		delete_field_in_equation_database(&EditFd);
		}

	invoke_set_catspecs(spots);
	return modified;
	}

/**********************************************************************/

/*ARGSUSED*/
LOGICAL	recompute_curveset_labs

	(
	SET		curves,
	SET	    spots,
	LOGICAL	use_active
	)

	{
	int		i, imem;
	COLOUR	dcolour;
	SPOT	spot;
	SPMEM	*mem;
	int		modified = FALSE;

	/* Make sure sets contain the right stuff */
	if (IsNull(spots))              return FALSE;
	if (!same(spots->type, "spot")) return FALSE;
	if (spots->num < 1)             return FALSE;

	/* Get default colour for the surface */
	if (NotNull(curves)) recall_set_pspec(curves, LINE_COLOUR,
								(POINTER)&dcolour);
	else                 dcolour = 1;

	/* Repeat for each spot */
	for (i=spots->num-1; i>=0; i--)
	    {
	    /* Retrieve the spot */
	    spot = (SPOT) spots->list[i];
	    if (IsNull(spot)) continue;

		/* Reset legend to proper colour if necessary */
	    if (same(spot->mclass, "legend"))
			{
			for (imem=0; imem<spot->nmem; imem++)
				{
				mem = spot->members + imem;
				if (mem->tspec.colour == dcolour) continue;
				modified = TRUE;
				mem->tspec.colour = dcolour;
				}
			continue;
			}
		
		/* Re-define the label */
		modified |= recalc_line_label(curves, spots, spot, 0);
	    }

	invoke_set_catspecs(spots);
	return modified;
	}

/**********************************************************************/

/*ARGSUSED*/
LOGICAL	recompute_spotset_labs

	(
	SET		points,
	SET	    spots,
	LOGICAL	use_active
	)

	{
	int		i, imem;
	COLOUR	dcolour;
	SPOT	spot;
	SPMEM	*mem;
	int		modified = FALSE;

	/* Make sure sets contain the right stuff */
	if (IsNull(spots))              return FALSE;
	if (!same(spots->type, "spot")) return FALSE;
	if (spots->num < 1)             return FALSE;

	/* Get default colour for the spot set */
	/*
	if (NotNull(points)) recall_set_pspec(points, LINE_COLOUR,
								(POINTER)&dcolour);
	else */      dcolour = 1;

	/* Repeat for each label */
	for (i=spots->num-1; i>=0; i--)
	    {
	    /* Retrieve the label */
	    spot = (SPOT) spots->list[i];
	    if (IsNull(spot)) continue;

		/* Reset legend to proper colour if necessary */
	    if (same(spot->mclass, "legend"))
			{
			for (imem=0; imem<spot->nmem; imem++)
				{
				mem = spot->members + imem;
				if (mem->tspec.colour == dcolour) continue;
				modified = TRUE;
				mem->tspec.colour = dcolour;
				}
			continue;
			}
		
	    modified = TRUE;
	    }

	invoke_set_catspecs(spots);
	return modified;
	}

/***********************************************************************
*                                                                      *
*     d e p e n d e n t _ w i n d s _ a f f e c t e d                  *
*     r e c o m p u t e _ d e p e n d e n t _ w i n d s                *
*                                                                      *
***********************************************************************/

static	DFLIST	**WxDflds = NULL;
static	int		nWxDflds = 0;

/**********************************************************************/

LOGICAL	dependent_winds_affected

	(
	DFLIST	*dfld,
	LOGICAL	if_own
	)

	{
	int		iwx;

	/* First time only: */
	/* Determine which fields could possibly affect a wind field */
	if (!WxDflds)
		{
		FpaConfigCrossRefStruct	**xdefs, *xdef;
		FpaConfigFieldStruct	*fdef;
		int						ix, nx, ifld;
		DFLIST					*xdfld;
		STRING					elem, level;

		/* Search list of cross references for all wind fields */
		/* >>> Later:  Only look at crossrefs that are actually used */
		/* >>>         in depiction wind fields */
		nx = identify_crossrefs_for_winds(&xdefs);
		for (ix=0; ix<nx; ix++)
			{
			/* Search list of fields for each cross reference */
			xdef = xdefs[ix];
			for (ifld=0; ifld<xdef->nfld; ifld++)
				{
				/* Find the dfield struct for this field */
				fdef  = xdef->flds[ifld];
				elem  = fdef->element->name;
				level = fdef->level->name;
				xdfld = find_dfield(elem, level);
				if (IsNull(xdfld)) continue;

				/* See if its already in the list */
				for(iwx=0; iwx<nWxDflds; iwx++)
					{
					if (xdfld == WxDflds[iwx]) break;
					}
				if (iwx < nWxDflds) continue;

				/* Add it */
				nWxDflds++;
				WxDflds = GETMEM(WxDflds, DFLIST *, nWxDflds);
				WxDflds[nWxDflds-1] = xdfld;
				}
			}

		/* Free list of cross references */
		nx = identify_crossrefs_for_winds_free(&xdefs, nx);
		}

	/* Check input */
	if (IsNull(dfld)) return FALSE;

	/* Decide whether some wind field depends on the given field */
	for(iwx=0; iwx<nWxDflds; iwx++)
		{
		if (dfld == WxDflds[iwx]) return TRUE;
		}

	/* Force a wind field itself if requested, even if not a crossref */
	if (dfld->editor == FpaC_WIND) return (if_own)? TRUE: FALSE;
	return FALSE;
	}

/**********************************************************************/

LOGICAL	recompute_dependent_winds

	(
	DFLIST	*dfld,
	LOGICAL	if_own,
	int		itime
	)

	{
	int			idfld;
	DFLIST		*wdfld;
	FRAME		*frame;
	METAFILE	meta;
	FIELD		fld;
	SET			areas;
	SET			barbs;
	STRING		elem, level;

	/* Check input */
	if (!dfld)            return FALSE;
	if (itime < 0)        return FALSE;
	if (itime >= NumTime) return FALSE;
	if (!dependent_winds_affected(dfld, if_own)) return FALSE;

	/* Re-label all wind type fields */
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		/* Find the next wind field */
		wdfld = DfldList + idfld;
		if (wdfld->editor != FpaC_WIND) continue;

		/* Only redo the same wind field itself if requested */
		if (wdfld == dfld && !if_own) continue;

		/* Extract default background subelement and value */
		elem  = wdfld->element;
		level = wdfld->level;

		/* Extract wind field and barb labels */
		frame = wdfld->frames + itime;
		meta  = frame->meta;
		fld   = find_mf_field(meta, "set", "area", "b", elem, level);
		areas = (fld)? fld->data.set: NULL;
		fld   = find_mf_field(meta, "set", "spot", NULL, elem, level);
		barbs = (fld)? fld->data.set: NULL;

		/* Recompute wind barbs */
		active_field_info(elem, level, "depict", NULL, NULL,
						TimeList[itime].jtime);
		(void) recompute_areaset_labs(areas, barbs, TRUE);
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     c o n v e r t _ w i n d                                          *
*                                                                      *
***********************************************************************/

#define Knots "knots"

double	convert_wind

	(
	double	spd,
	STRING	units,
	LOGICAL	*valid
	)

	{
	double	xspd;

	/* Convert the wind speed to default wind units */
	*valid = convert_value(units, spd, WindUnits, &xspd);
	return xspd;
	}

/***********************************************************************
*                                                                      *
*     c o n s t r u c t _ s p l i n e _ l a b e l                      *
*     c o n s t r u c t _ a r e a _ l a b e l                          *
*     c o n s t r u c t _ w i n d _ l a b e l                          *
*     c o n s t r u c t _ l i n e _ l a b e l                          *
*                                                                      *
***********************************************************************/

static	LOGICAL	construct_spline_label

	(
	POINT	pos,
	LAB_DEF	*ldef
	)

	{
	STRING	val, type, search;
	char	which;
	POINT	cpos;
	ITEM	item;
	SPOT	spot;
	LOGICAL	valid;
	float	sdist;
	double	fval;
	char	sval[20];

	if (!ldef) return FALSE;

	search = NULL;
	switch (ldef->attach)
		{
		case AttachAuto:	search = (EditDoHiLo)? "cxn": "c";
							break;
		case AttachContour:	search = "c";
							break;
		case AttachMax:		search = "x";
							break;
		case AttachMin:		search = "n";
							break;
		case AttachCol:		search = "s";
							break;
		}

	/* Interpolate value near or at selected point */
	if (!blank(search))
		{
		val = eval_sfc_feature(EditSfc, pos, search, cpos, &which, &item,
				&valid);
		if (!valid)
			{
			/* >>>>> warning message for no feature nearby!!! <<<<< */
			return FALSE;
			}

		/* If auto, set to what was actually found */
		if (ldef->attach == AttachAuto)
			{
			switch (which)
				{
				case 'c':	type = "contour";		break;
				case 'x':	type = "high_at_max";	break;
				case 'n':	type = "low_at_min";	break;
				default:	return FALSE;
				}
			ldef = find_label_type(type, &EditFd);
			if (!ldef) return FALSE;
			}
		}
	else
		{
		valid = eval_sfc(EditSfc, pos, &fval);
		if (!valid) return FALSE;
		(void) sprintf(sval, "%d", NINT(fval));
		val  = sval;
		item = NullItem;
		copy_point(cpos, pos);
		}

	/* Prevent multiple labels/highs/lows at the same point */
	spot = closest_spot(EditLabs, cpos, NULL, NULL, &sdist, NULL);
	if (spot && (sdist < PickTol))
		{
		put_message("label-too-close");
		(void) sleep(1);
		return FALSE;
		}

	put_message("label-adding");
	return add_spline_label(ldef, cpos, val, item);
	}

/**********************************************************************/

static	LOGICAL	construct_area_label

	(
	POINT	pos,
	LAB_DEF	*ldef
	)

	{
	SUBAREA	sub;
	CAL		scal;
	STRING	lab;
	SPOT	spot;
	float	sdist;

	if (!ldef) return FALSE;

	/* Get attributes at selected point */
	(void) eval_areaset(EditAreas, pos, PickFirst, &sub, &scal);
	if (IsNull(scal)) return FALSE;
	lab = CAL_get_attribute(scal, CALuserlabel);

	if (ldef->attach == AttachAuto)
		{
		ldef = find_label_type("area", &EditFd);
		if (!ldef) return FALSE;
		}

	/* Prevent multiple labels at the same point */
	spot = closest_spot(EditLabs, pos, NULL, NULL, &sdist, NULL);
	if (spot && (sdist < PickTol))
		{
		put_message("label-too-close");
		(void) sleep(1);
		return FALSE;
		}

	put_message("label-adding");
	return add_area_label(ldef, pos, lab, sub, scal);
	}

/**********************************************************************/

static	LOGICAL	construct_wind_label

	(
	POINT	pos,
	LAB_DEF	*ldef
	)

	{
	SUBAREA		sub;
	CAL			scal;
	STRING		lab;
	float		dir, spd, gust;
	WIND_VAL	wv;
	STRING		units;
	SPOT		spot;
	float		sdist;
	LOGICAL		valid;

	if (!ldef) return FALSE;

	/* Get attributes at selected point */
	(void) eval_areaset(EditAreas, pos, PickFirst, &sub, &scal);
	if (IsNull(scal)) return FALSE;
	lab = CAL_get_attribute(scal, CALuserlabel);

	/* Interpolate wind at selected point */
	/* Could get CAL with base wind and adjustments */
	valid = extract_awind(1, &WindFd, FALSE, 1, make_plist(pos), MapProj->clon,
					&dir, &spd, &gust, &units);
	if (!valid)
		{
		put_message("label-no-eval");
		(void) sleep(1);
		return FALSE;
		}

	if (ldef->attach == AttachAuto)
		{
		if (same_label_type(ldef->type, "auto-wind"))
			ldef = find_label_type("wind", &EditFd);
		else if (same_label_type(ldef->type, "auto-adjust"))
			ldef = find_label_type("adjustment", &EditFd);
		else return FALSE;
		if (!ldef) return FALSE;
		}

	/* Prevent multiple wind labels at the same point */
	spot = closest_spot(EditLabs, pos, NULL, NULL, &sdist, NULL);
	if (spot && (sdist < PickTol))
		{
		put_message("label-too-close");
		(void) sleep(1);
		return FALSE;
		}

	wv.dir   = dir;
	wv.dunit = "degrees_true";
	wv.speed = convert_wind((double)spd,  units, &valid);
	wv.gust  = convert_wind((double)gust, units, &valid);
	wv.sunit = WindUnits;

	put_message("label-adding");
	return add_wind_label(ldef, pos, &wv, lab, sub, scal);
	}

/**********************************************************************/

static	LOGICAL	construct_line_label

	(
	POINT	pos,
	LAB_DEF	*ldef
	)

	{
	STRING	search;
	CURVE	curve;
	POINT	cpos;
	CAL		scal;
	STRING	lab;
	SPOT	spot;
	float	sdist;

	if (!ldef) return FALSE;

	search = NULL;
	switch (ldef->attach)
		{
		case AttachAuto:
		case AttachLine:	search = "l";
							break;
		}
	if (blank(search)) return FALSE;

	/* Check for no lines to label */
	if (IsNull(EditCurves) || EditCurves->num <= 0)
		{
		put_message("line-no-label");
		(void) sleep(1);
		return FALSE;
		}

	/* Get attributes at selected point */
	curve = closest_curve(EditCurves, pos, NULL, cpos, NULL);
	if (!curve)
		{
		put_message("label-no-eval");
		(void) sleep(1);
		return FALSE;
		}
	scal = curve->attrib;
	if (IsNull(scal)) return FALSE;
	lab = CAL_get_attribute(scal, CALuserlabel);

	if (ldef->attach == AttachAuto)
		{
		ldef = find_label_type("line", &EditFd);
		if (!ldef) return FALSE;
		}

	/* Prevent multiple labels at the same point */
	spot = closest_spot(EditLabs, cpos, NULL, NULL, &sdist, NULL);
	if (spot && (sdist < PickTol))
		{
		put_message("label-too-close");
		(void) sleep(1);
		return FALSE;
		}

	put_message("label-adding");
	return add_line_label(ldef, cpos, lab, curve, scal);
	}

/***********************************************************************
*                                                                      *
*     a d d _ s p l i n e _ l a b e l                                  *
*     a d d _ a r e a _ l a b e l                                      *
*     a d d _ w i n d _ l a b e l                                      *
*     a d d _ l i n e _ l a b e l                                      *
*                                                                      *
***********************************************************************/

static	LOGICAL	add_spline_label

	(
	LAB_DEF	*ldef,
	POINT	pos,
	STRING	val,
	ITEM	item
	)

	{
	SPOT	spot;
	COLOUR	colour;
	HILITE	hilite;

	if (!ldef) return FALSE;

	/* Construct empty spot label */
	spot = create_spot(pos, ldef->mclass, ldef->attach, ldef->cal);
	add_item_to_set(EditLabs, (ITEM)spot);
	remember_current_label(spot);

	/* Determine colour and highlight */
	colour = SafeColour;
	hilite = SafeHilite;
	if (NotNull(item))
		{
		if (same(ldef->mclass, "contour"))
			{
			colour = ((CURVE)item)->lspec.colour;
			hilite = ((CURVE)item)->lspec.hilite;
			}
		else if (same(ldef->mclass, "hilo"))
			{
			colour = ((MARK)item)->mspec.colour;
			hilite = ((MARK)item)->mspec.hilite;
			}
		}

	/* Add and define its members */
	build_spot_members(spot, EditLabs->ncspec, EditLabs->cspecs);
	build_spot_pspecs(spot, FALSE, colour, hilite, LabelSize, WbarbSize);
	(void) eval_spline_label(EditSfc, val, item, spot);

	/* Define its presentation */
	invoke_set_catspecs(EditLabs);
	if (EditColour >= 0)
		change_set_pspec(EditLabs, TEXT_COLOUR, (POINTER)&EditColour);
	if (EditUndoable) highlight_set(EditLabs, 1);

	if (EditUndoable) post_mod("labs");
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL	add_area_label

	(
	LAB_DEF	*ldef,
	POINT	pos,
	STRING	val,
	SUBAREA	sub,
	CAL		scal
	)

	{
	SPOT	spot;
	CAL		lcal;
	COLOUR	colour;
	HILITE	hilite;

	if (!ldef) return FALSE;

	/* Construct empty spot label */
	spot = create_spot(pos, ldef->mclass, ldef->attach, ldef->cal);
	add_item_to_set(EditLabs, (ITEM)spot);
	remember_current_label(spot);

	/* Set its attributes */
	lcal = spot->attrib;
	CAL_merge(lcal, scal, TRUE);
	CAL_add_attribute(lcal, AttribUserlabel, val);

	/* Determine colour and highlight */
	colour = SafeColour;
	hilite = SafeHilite;
	if (NotNull(sub))
		{
		colour = sub->lspec.colour;
		hilite = sub->lspec.hilite;
		}

	/* Add and define its members */
	build_spot_members(spot, EditLabs->ncspec, EditLabs->cspecs);
	build_spot_pspecs(spot, FALSE, colour, hilite, LabelSize, WbarbSize);
	(void) eval_area_label(EditAreas, val, spot);

	/* Define its presentation */
	invoke_set_catspecs(EditLabs);
	if (EditColour >= 0)
		change_set_pspec(EditLabs, TEXT_COLOUR, (POINTER)&EditColour);
	if (EditUndoable) highlight_set(EditLabs, 1);

	if (EditUndoable) post_mod("labs");
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL	add_wind_label

	(
	LAB_DEF		*ldef,
	POINT		pos,
	WIND_VAL	*wv,
	STRING		val,
	SUBAREA		sub,
	CAL			scal
	)

	{
	SPOT	spot;
	CAL		lcal;
	COLOUR	colour;
	HILITE	hilite;

	if (!ldef) return FALSE;

	/* Construct empty spot label */
	spot = create_spot(pos, ldef->mclass, ldef->attach, ldef->cal);
	add_item_to_set(EditLabs, (ITEM)spot);
	remember_current_label(spot);

	/* Set its attributes */
	lcal = spot->attrib;
	CAL_merge(lcal, scal, TRUE);
	CAL_add_attribute(lcal, AttribUserlabel, val);

	/* Determine colour and highlight */
	colour = SafeColour;
	hilite = SafeHilite;
	if (NotNull(sub))
		{
		colour = sub->lspec.colour;
		hilite = sub->lspec.hilite;
		}

	/* Add and define its members */
	build_spot_members(spot, EditLabs->ncspec, EditLabs->cspecs);
	build_spot_pspecs(spot, FALSE, colour, hilite, LabelSize, WbarbSize);
	(void) eval_wind_label(EditAreas, wv, spot);

	/* Define its presentation */
	invoke_set_catspecs(EditLabs);
	if (EditColour >= 0)
		change_set_pspec(EditLabs, BARB_COLOUR, (POINTER)&EditColour);
	if (EditUndoable) highlight_set(EditLabs, 1);

	if (EditUndoable) post_mod("labs");
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL	add_line_label

	(
	LAB_DEF	*ldef,
	POINT	pos,
	STRING	val,
	CURVE	curve,
	CAL		scal
	)

	{
	SPOT	spot;
	CAL		lcal;
	COLOUR	colour;
	HILITE	hilite;

	if (!ldef) return FALSE;

	/* Construct empty spot label */
	spot = create_spot(pos, ldef->mclass, ldef->attach, ldef->cal);
	add_item_to_set(EditLabs, (ITEM)spot);
	remember_current_label(spot);

	/* Set its attributes */
	lcal = spot->attrib;
	CAL_merge(lcal, scal, TRUE);
	CAL_add_attribute(lcal, AttribUserlabel, val);

	/* Determine colour and highlight */
	colour = SafeColour;
	hilite = SafeHilite;
	if (NotNull(curve))
		{
		colour = curve->lspec.colour;
		hilite = curve->lspec.hilite;
		}

	/* Add and define its members */
	build_spot_members(spot, EditLabs->ncspec, EditLabs->cspecs);
	build_spot_pspecs(spot, FALSE, colour, hilite, LabelSize, WbarbSize);
	(void) eval_line_label(EditCurves, val, spot);

	/* Define its presentation */
	invoke_set_catspecs(EditLabs);
	if (EditColour >= 0)
		change_set_pspec(EditLabs, TEXT_COLOUR, (POINTER)&EditColour);
	if (EditUndoable) highlight_set(EditLabs, 1);

	if (EditUndoable) post_mod("labs");
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     r e c a l c _ s p l i n e _ l a b e l                            *
*     r e c a l c _ a r e a _ l a b e l                                *
*     r e c a l c _ w i n d _ l a b e l                                *
*     r e c a l c _ l i n e _ l a b e l                                *
*                                                                      *
***********************************************************************/

static	LOGICAL	recalc_spline_label

	(
	SURFACE	sfc,
	SET		spots,
	SPOT	spot,
	LOGICAL	*del
	)

	{
	POINT	lpos;
	STRING	lval, search;
	char	which;
	ITEM	item;
	LOGICAL	valid;
	int		imem;
	SPMEM	*mem;
	COLOUR	colour;

	if (del) *del = FALSE;
	if (IsNull(sfc))  return FALSE;
	if (IsNull(spot)) return FALSE;

	/* See if label is attached to a feature and may need to be moved */
	switch (spot->feature)
		{
		case AttachContour:	search = "c";	break;
		case AttachMax:		search = "x";	break;
		case AttachMin:		search = "n";	break;
		default:			search = NULL;	break;
		}

	/* Move it if necessary */
	lval = NULL;
	item = NullItem;
	if (!blank(search))
		{
		lval = eval_sfc_feature(sfc, spot->anchor, search, lpos, &which,
					&item, &valid);
		if (!valid)
			{
			/* If can't be replaced remove it */
			remove_item_from_set(spots, (ITEM) spot);
			if (del) *del = TRUE;
			return TRUE;
			}

		/* Move the label */
		define_spot_anchor(spot, lpos);
		}
	
	/* Obtain preset attributes */
	retain_modified_attribs(spot);

	/* Reset colour if necessary */
	if (NotNull(item) && (which=='c'))
		{
		colour = ((CURVE)item)->lspec.colour;
		for (imem=0; imem<spot->nmem; imem++)
			{
			mem = spot->members + imem;
			define_tspec_value(&mem->tspec, TEXT_COLOUR, (POINTER)&colour);
			define_mspec_value(&mem->mspec, MARK_COLOUR, (POINTER)&colour);
			define_bspec_value(&mem->bspec, BARB_COLOUR, (POINTER)&colour);
			}
		}

	/* Re-define its members if necessary */
	return eval_spline_label(sfc, lval, item, spot);
	}

/**********************************************************************/

static	LOGICAL	recalc_area_label

	(
	SET		areas,
	SET		spots,
	SPOT	spot,
	LOGICAL	*del
	)

	{
	STRING	lval;
	SUBAREA	sub;
	CAL		cal;
	int		imem;
	SPMEM	*mem;
	COLOUR	colour;

	if (del) *del = FALSE;
	if (IsNull(areas)) return FALSE;
	if (IsNull(spot))  return FALSE;
	
	/* These labels do not attach to anything at present */

	/* Obtain preset attributes */
	retain_modified_attribs(spot);

	/* Evaluate from modified field or moved location */
	(void) eval_areaset(areas, spot->anchor, PickFirst, &sub, &cal);
	CAL_merge(spot->attrib, cal, TRUE);

	/* Reset colour if necessary */
	if (NotNull(sub))
		{
		colour = sub->lspec.colour;
		for (imem=0; imem<spot->nmem; imem++)
			{
			mem = spot->members + imem;
			define_tspec_value(&mem->tspec, TEXT_COLOUR, (POINTER)&colour);
			define_mspec_value(&mem->mspec, MARK_COLOUR, (POINTER)&colour);
			define_bspec_value(&mem->bspec, BARB_COLOUR, (POINTER)&colour);
			}
		}

	/* Re-define its members if necessary */
	lval = CAL_get_attribute(spot->attrib, CALuserlabel);
	return eval_area_label(areas, lval, spot);
	}

/**********************************************************************/

static	LOGICAL	recalc_wind_label

	(
	SET		wareas,
	SET		spots,
	SPOT	spot,
	LOGICAL	*del
	)

	{
	SUBAREA		sub;
	CAL			cal;
	float		dir, spd, gust;
	WIND_VAL	wv;
	STRING		units;
	LOGICAL		valid;
	int			imem;
	SPMEM		*mem;
	COLOUR		colour;

	if (del) *del = FALSE;
	if (IsNull(wareas)) return FALSE;
	if (IsNull(spot))   return FALSE;
	
	/* These labels do not attach to anything at present */

	/* Obtain preset attributes */
	retain_modified_attribs(spot);

	/* Evaluate from modified field or moved location */
	(void) eval_areaset(wareas, spot->anchor, PickFirst, &sub, &cal);
	CAL_merge(spot->attrib, cal, TRUE);

	/* Reset colour if necessary */
	if (NotNull(sub))
		{
		colour = sub->lspec.colour;
		for (imem=0; imem<spot->nmem; imem++)
			{
			mem = spot->members + imem;
			define_tspec_value(&mem->tspec, TEXT_COLOUR, (POINTER)&colour);
			define_mspec_value(&mem->mspec, MARK_COLOUR, (POINTER)&colour);
			define_bspec_value(&mem->bspec, BARB_COLOUR, (POINTER)&colour);
			}
		}

	/* Interpolate wind at same point */
	/* Could get CAL with base wind and adjustments */
	valid = extract_awind(1, &WindFd, FALSE, 1, make_plist(spot->anchor),
					MapProj->clon, &dir, &spd, &gust, &units);
	if (!valid)
		{
		/* If can't be replaced remove it */
		remove_item_from_set(spots, (ITEM) spot);
		if (del) *del = TRUE;
		put_message("label-no-eval");
		return FALSE;
		}
	wv.dir   = dir;
	wv.dunit = "degrees_true";
	wv.speed = convert_wind((double)spd,  units, &valid);
	wv.gust  = convert_wind((double)gust, units, &valid);
	wv.sunit = WindUnits;

	/* Re-define its members if necessary */
	return eval_wind_label(wareas, &wv, spot);
	}

/**********************************************************************/

static	LOGICAL	recalc_line_label

	(
	SET		curves,
	SET		spots,
	SPOT	spot,
	LOGICAL	*del
	)

	{
	POINT	lpos;
	STRING	lval, search;
	CURVE	curve;
	CAL		cal;
	int		imem;
	SPMEM	*mem;
	COLOUR	colour;

	if (del) *del = FALSE;
	if (IsNull(curves)) return FALSE;
	if (IsNull(spot))   return FALSE;

	/* See if label is attached to a feature and may need to be moved */
	switch (spot->feature)
		{
		case AttachLine:	search = "l";	break;
		default:			search = NULL;	break;
		}

	/* Move it if necessary */
	curve = NullCurve;
	if (!blank(search))
		{
		curve = closest_curve(curves, spot->anchor, NULL, lpos, NULL);
		if (IsNull(curve))
			{
			/* If can't be replaced remove it */
			remove_item_from_set(spots, (ITEM) spot);
			if (del) *del = TRUE;
			return TRUE;
			}

		/* Move the label */
		define_spot_anchor(spot, lpos);
		}

	/* Obtain preset attributes */
	retain_modified_attribs(spot);

	/* Evaluate from modified field or moved location */
	if (NotNull(curve))
		{
		cal = curve->attrib;
		CAL_merge(spot->attrib, cal, TRUE);
		}

	/* Reset colour if necessary */
	if (NotNull(curve))
		{
		colour = curve->lspec.colour;
		for (imem=0; imem<spot->nmem; imem++)
			{
			mem = spot->members + imem;
			define_tspec_value(&mem->tspec, TEXT_COLOUR, (POINTER)&colour);
			define_mspec_value(&mem->mspec, MARK_COLOUR, (POINTER)&colour);
			define_bspec_value(&mem->bspec, BARB_COLOUR, (POINTER)&colour);
			}
		}

	/* Re-define its members if necessary */
	lval = CAL_get_attribute(spot->attrib, CALuserlabel);
	return eval_line_label(curves, lval, spot);
	}

/***********************************************************************
*                                                                      *
*     r e t a i n _ m o d i f i e d _ a t t r i b s                    *
*                                                                      *
***********************************************************************/

static	void	retain_modified_attribs

	(
	SPOT	spot
	)

	{
	LAB_DEF	*ldef;
	STRING	ltype, aval;
	int		iatt;
	ATTRIB	*att;
	CAL		cal;
	
	/* See what we know about the label type of this label */
	ltype = CAL_get_attribute(spot->attrib, AttribLabelType);
	ldef  = find_label_type_info(ltype, &EditFd);
	if (IsNull(ldef)) return;

	/* Retain only the preset attributes but keep modified values */
	cal = CAL_create_empty();
	cal->defs = spot->attrib->defs;
	for (iatt=0; iatt<ldef->cal->nattribs; iatt++)
		{
		att = ldef->cal->attribs + iatt;
		if (IsNull(att)) continue;

		aval = CAL_get_attribute(spot->attrib, att->name);
		if (CAL_no_value(aval))
			{
			aval = att->value;
			if (CAL_no_value(aval)) continue;
			}

		CAL_add_attribute(cal, att->name, aval);
		}

	/* Replace its attributes with the new list */
	define_spot_attribs(spot, cal);
	CAL_destroy(cal);
	}

/***********************************************************************
*                                                                      *
*     e v a l _ s p l i n e _ l a b e l                                *
*     e v a l _ a r e a _ l a b e l                                    *
*     e v a l _ w i n d _ l a b e l                                    *
*     e v a l _ l i n e _ l a b e l                                    *
*                                                                      *
***********************************************************************/

static	LOGICAL	eval_spline_label

	(
	SURFACE	sfc,
	STRING	val,
	ITEM	item,
	SPOT	spot
	)

	{
	int			iup, ivp, pseg;
	POINT		ppos, pdiff, pspos, pepos, spos, epos;
	CURVE		curve;
	float		dir, spd, gust;
	double		fval;
	STRING		wval, aval, units;
	char		sval[20];
	LOGICAL		valid;
	WIND_VAL	wv;

	if (!spot) return FALSE;

	/* Add spot location */
	CAL_add_location((CAL) spot->attrib, MapProj, spot->anchor);

	/* Add contour direction at spot location (if required) */
	/* Note that contour curve is in patch coordinates!     */
	if (NotNull(item) && same(spot->mclass, "contour"))
		{
		curve = (CURVE) item;
		valid = find_patch(&sfc->sp, spot->anchor, &iup, &ivp, ppos, pdiff);
		if (NotNull(curve) && NotNull(curve->line) && valid)
			{
			curve_test_point(curve, ppos,
					NullFloat, NullPoint, &pseg, NullLogical, NullLogical);
			if (line_span_info(curve->line, pseg, pspos, pepos, NullPoint) > 0.0)
				{
				(void) patch_to_world(&sfc->sp, pspos, iup, ivp, spos);
				(void) patch_to_world(&sfc->sp, pepos, iup, ivp, epos);
				CAL_add_line_dir((CAL) spot->attrib, MapProj, spos, epos);
				}
			}
		}

	/* Evaluate special attributes */

	aval = CAL_get_attribute(spot->attrib, AttribEvalContour);
	if (CAL_is_value(aval))
		{
		CAL_set_attribute(spot->attrib, AttribEvalContour, val);
		}

	aval = CAL_get_attribute(spot->attrib, AttribEvalSpval);
	if (CAL_is_value(aval))
		{
		valid = eval_sfc(sfc, spot->anchor, &fval);
		if (!valid)
			(void) strcpy(sval, "N/A");
		else
			(void) sprintf(sval, "%d", NINT(fval));
		CAL_set_attribute(spot->attrib, AttribEvalSpval, sval);
		}

	aval = CAL_get_attribute(spot->attrib, AttribEvalWind);
	if (CAL_is_value(aval))
		{
		valid = extract_awind(1, &WindFd, FALSE, 1, make_plist(spot->anchor),
						MapProj->clon, &dir, &spd, &gust, &units);
		if (!valid)
			wval = strdup("N/A");
		else
			{
			wv.dir   = dir;
			wv.dunit = "degrees_true";
			wv.speed = convert_wind((double)spd,  units, &valid);
			wv.gust  = convert_wind((double)gust, units, &valid);
			wv.sunit = WindUnits;
			wval     = build_wind_value_string(&wv);
			}
		CAL_set_attribute(spot->attrib, AttribEvalWind, wval);
		FREEMEM(wval);
		}

	/* Invoke "label" type rules */
	CAL_invoke_label_rules_by_name(spot->attrib, EditFd.edef->name,
		EditFd.ldef->name);
	check_spot_attribs(spot);
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL	eval_area_label

	(
	SET		set,
	STRING	val,
	SPOT	spot
	)

	{
	if (!spot) return FALSE;

	/* Add spot location */
	CAL_add_location((CAL) spot->attrib, MapProj, spot->anchor);

	/* Invoke "label" type rules */
	CAL_invoke_label_rules_by_name(spot->attrib, EditFd.edef->name,
		EditFd.ldef->name);
	check_spot_attribs(spot);
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL	eval_wind_label

	(
	SET			set,
	WIND_VAL	*wv,
	SPOT		spot
	)

	{
	STRING		aval, wval;

	if (!spot) return FALSE;

	/* Add spot location */
	CAL_add_location((CAL) spot->attrib, MapProj, spot->anchor);

	/* Evaluate special attributes */

	aval = CAL_get_attribute(spot->attrib, "EVAL_wind");
	if (CAL_is_value(aval))
		{
		wval = build_wind_value_string(wv);
		CAL_set_attribute(spot->attrib, "EVAL_wind", wval);
		FREEMEM(wval);
		}

	/* Invoke "label" type rules */
	CAL_invoke_label_rules_by_name(spot->attrib, EditFd.edef->name,
		EditFd.ldef->name);
	check_spot_attribs(spot);
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL	eval_line_label

	(
	SET		set,
	STRING	val,
	SPOT	spot
	)

	{
	CURVE	curve;
	int		seg;
	POINT	spos, epos;

	if (!spot) return FALSE;

	/* Add spot location */
	CAL_add_location((CAL) spot->attrib, MapProj, spot->anchor);

	/* Add line direction at point and line length */
	curve = closest_curve(set, spot->anchor, NULL, NULL, &seg);
	if (curve) 
		{
		if (line_span_info(curve->line, seg, spos, epos, NullPoint) > 0.0)
			CAL_add_line_dir((CAL) spot->attrib, MapProj, spos, epos);
		CAL_add_line_len((CAL) spot->attrib, MapProj, curve->line);
		}

	/* Invoke "label" type rules */
	CAL_invoke_label_rules_by_name(spot->attrib, EditFd.edef->name,
		EditFd.ldef->name);
	check_spot_attribs(spot);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     b u i l d _ s p o t _ p s p e c s                                *
*     c h e c k _ s p o t _ a t t r i b s                              *
*     c h e c k _ l a b e l s                                          *
*                                                                      *
***********************************************************************/

static	void	build_spot_pspecs

	(
	SPOT	spot,
	LOGICAL	legend,
	COLOUR	colour,
	HILITE	hilite,
	float	tsize,
	float	bsize
	)

	{
	int		imem;
	SPMEM	*mem;

	/* Add and define its members */
	for (imem=0; imem<spot->nmem; imem++)
		{
		mem = spot->members + imem;
		define_tspec(&mem->tspec, colour, SafeFont, False, tsize, 0.0,
				(legend)?Hl:Hc, Vc, hilite);
		define_mspec(&mem->mspec, colour, 0, NULL, 0, FALSE, 0.0, 0.0,
				hilite);
		define_bspec(&mem->bspec, colour, BarbWind, False, 0.0, bsize,
				False, False, 0.0, 0.0, WindUnits, hilite);
		}
	}

/**********************************************************************/

void	check_labels

	(
	SET		spots
	)

	{
	SPOT	spot;
	int		isp;

	if (!spots)                     return;
	if (!same(spots->type, "spot")) return;

	for (isp=0; isp<spots->num; isp++)
		{
		spot = (SPOT) spots->list[isp];
		check_spot_attribs(spot);
		}
	}

/**********************************************************************/

static	void	check_spot_attribs

	(
	SPOT	spot
	)

	{
	int			imem;
	SPMEM		*mem;
	STRING		name, val;
	WIND_VAL	wval;

	if (IsNull(spot))    return;
	if (spot->nmem <= 0) return;

	for (imem=0; imem<spot->nmem; imem++)
		{
		mem = spot->members + imem;

		switch (mem->type)
			{
			case SpotText:
					name = blank(mem->attrib)? AttribUserlabel: mem->attrib;
					val  = CAL_get_attribute(spot->attrib, name);
					if (CAL_no_value(val))
						{
						(void) pr_diag("Editor",
							"Spot lacks attribute \"%s\" for spot class \"%s\"\n",
							name, spot->mclass);
						continue;
						}
					break;

			case SpotBarb:
					name = blank(mem->attrib)? AttribUserlabel: mem->attrib;
					val  = CAL_get_attribute(spot->attrib, name);
					if (CAL_no_value(val))
						{
						(void) pr_diag("Editor",
							"Spot lacks attribute \"%s\" for spot class \"%s\"\n",
							name, spot->mclass);
						continue;
						}
					if (!parse_wind_value_string(val, &wval))
						{
						(void) pr_warning("Editor",
							"Spot attribute \"%s\" contains invalid wind for spot class \"%s\"\n",
							name, spot->mclass);
						continue;
						}
					break;
			}
		}
	}

/***********************************************************************
*                                                                      *
*     r e m e m b e r _ c u r r e n t _ l a b e l                      *
*     f o r g e t _ c u r r e n t _ l a b e l                          *
*     g e t _ c u r r e n t _ l a b e l _ v a l u e                    *
*     s e t _ c u r r e n t _ l a b e l _ v a l u e                    *
*     r e m e m b e r _ c u r r e n t _ c a l                          *
*     f o r g e t _ c u r r e n t _ c a l                              *
*     u p d a t e _ c u r r e n t _ c a l                              *
*                                                                      *
***********************************************************************/

static	SPOT	CurrSpot  = NullSpot;
static	CAL		CurrCal   = NullCal;

static	void	remember_current_label

	(
	SPOT	spot
	)

	{
	CurrSpot = spot;

	if (NotNull(CurrSpot) && NotNull(CurrCal))
		{
		STRING	ctype, stype;

		ctype = CAL_get_attribute(CurrCal, AttribLabelType);
		stype = CAL_get_attribute(CurrSpot->attrib, AttribLabelType);
		if (!same(ctype, stype)) forget_current_cal();
		}
	}

/**********************************************************************/

static	void	forget_current_label

	(void)

	{
	remember_current_label(NullSpot);
	}

/**********************************************************************/

static	void	get_current_label_value

	(void)

	{
	if (IsNull(CurrSpot)) return;
	if (NotNull(CurrCal)) label_select(CurrCal);
	else                  label_select(CurrSpot->attrib);
	}

/**********************************************************************/

static	LOGICAL	set_current_label_value

	(
	CAL		cal
	)

	{
	if (IsNull(CurrSpot)) return FALSE;

	/* Remember it for later */
	remember_current_cal(cal);

	/* Set its attributes */
	define_spot_attribs(CurrSpot, cal);

	/* Set its presentation */
	invoke_set_catspecs(EditLabs);
	if (EditColour >= 0)
		change_set_pspec(EditLabs, TEXT_COLOUR, (POINTER)&EditColour);
	if (EditUndoable) highlight_set(EditLabs, 1);

	if (EditUndoable) post_mod("labs");
	return TRUE;
	}

/**********************************************************************/

static	void	remember_current_cal

	(
	CAL		cal
	)

	{
	if (NotNull(CurrCal)) CurrCal = CAL_destroy(CurrCal);
	if (NotNull(cal))     CurrCal = CAL_duplicate(cal);
	}

/**********************************************************************/

static	void	forget_current_cal

	(void)

	{
	remember_current_cal(NullCal);
	}

/**********************************************************************/

static	void	update_current_cal

	(void)

	{
	if (IsNull(CurrSpot)) return;
	if (IsNull(CurrCal))  return;
	CAL_merge(CurrCal, CurrSpot->attrib, TRUE);
	}

/***********************************************************************
*                                                                      *
*     f i n d _ l a b e l _ t y p e _ i n f o                          *
*     f i n d _ l a b e l _ t y p e                                    *
*                                                                      *
***********************************************************************/

static	LAB_DEF	*find_label_type_info
	(
	STRING			ltype,
	FLD_DESCRIPT	*fd
	)

	{
	/* Info about the current label type */
	static	LOGICAL			First       = TRUE;
	static	STRING			CurrLtype   = NULL;
	static	FLD_DESCRIPT	CurrFd      = {0};
	static	LAB_DEF			*CurrLabDef = (LAB_DEF *)0;

	/* Initialize the file descriptor */
	if (First)
		{
		(void) init_fld_descript(&CurrFd);
		First = FALSE;
		}

	/* Do we already have the info about this label type? */
	if (!same(ltype, CurrLtype) || IsNull(fd)
			|| fd->edef != CurrFd.edef || fd->ldef != CurrFd.ldef)
		{
		/* New field or label type */
		forget_current_label();

#		ifdef DEBUG_LABELS
		pr_diag("Editor", "[find_label_type_info] Setting %s for %s %s\n",
			ltype, fd->edef->name, fd->ldef->name);
#		endif /* DEBUG_LABELS */

		/* Find given label type in config file or built-in list */
		CurrLabDef = find_label_type(ltype, fd);
		CurrLtype  = STRMEM(CurrLtype, ltype);
		(void) copy_fld_descript(&CurrFd, fd);
		}

	return CurrLabDef;
	}

/**********************************************************************/

static	LAB_DEF	*find_label_type
	(
	STRING			ltype,
	FLD_DESCRIPT	*fd
	)

	{
	FpaConfigElementLabellingStruct	*cfg_ldef;
	FpaConfigDefaultAttribStruct	*cfg_attlist;
	int			np, idef, itype, ifound, iatt;
	STRING		attname, attval;
	LAB_DEF		*ldef;
	FpaConfigElementStruct	*edef;

	static	int						Ndefs   = 0;
	static	LAB_DEF					*Ldefs  = (LAB_DEF *)0;
	static	FpaConfigElementStruct	**Edefs = (FpaConfigElementStruct **)0;

	if (IsNull(fd))                return (LAB_DEF *)0;
	if (IsNull(edef = fd->edef))   return (LAB_DEF *)0;
	if (IsNull(edef->elem_detail)) return (LAB_DEF *)0;

#	ifdef DEBUG_LABELS
	pr_diag("Editor", "[find_label_type] %s for %s %s\n",
		ltype, fd->edef->name, fd->ldef->name);
#	endif /* DEBUG_LABELS */

	/* Make sure we have enough label defs */
	/* It won't be necessary to blow away all saved label defs when */
	/* the field has changed, since each new label def that collides */
	/* with an existing one will automatically replace it */
	cfg_ldef = edef->elem_detail->labelling;
	if (Ndefs < cfg_ldef->ntypes)
		{
		np    = Ndefs;
		Ndefs = cfg_ldef->ntypes;
		Ldefs = GETMEM(Ldefs, LAB_DEF, Ndefs);
		Edefs = GETMEM(Edefs, FpaConfigElementStruct *, Ndefs);
		for (idef=np; idef<Ndefs; idef++)
			{
			ldef = Ldefs + idef;
			ldef->type   = NULL;
			ldef->mclass = NULL;
			ldef->attach = AttachNone;
			ldef->entry  = FALSE;
			ldef->modify = FALSE;
			ldef->cal    = NullCal;
			Edefs[idef]  = (FpaConfigElementStruct *)0;
			}
		}

	/* First see if we already know about this one */
	for (idef=0; idef<Ndefs; idef++)
		{
		if (Edefs[idef] != edef)  continue;
		ldef = Ldefs + idef;
		if (blank(ldef->type))        continue;
		if (!same(ltype, ldef->type)) continue;
		return ldef;
		}

	/* Find given label type in config file */
	ifound = -1;
	for (itype=0; itype<cfg_ldef->ntypes; itype++)
		{
		if (!same(ltype, cfg_ldef->type_names[itype])) continue;
		ifound = itype;
		break;
		}

	/* If still not found - find a built-in type */
	if (ifound < 0)
		{
		return find_preset_label_type(ltype);
		}

	/* Now build a LAB_DEF to describe this label type */
	ldef = Ldefs + ifound;
	ldef->type   = STRMEM(ldef->type, ltype);
	ldef->mclass = STRMEM(ldef->mclass, cfg_ldef->type_classes[ifound]);
	ldef->attach = AttachNone;
	ldef->entry  = blank(cfg_ldef->type_entry_files[ifound])?  FALSE: TRUE;
	ldef->modify = blank(cfg_ldef->type_modify_files[ifound])? FALSE: TRUE;
	CAL_destroy(ldef->cal);
	ldef->cal       = CAL_create_empty();
	ldef->cal->defs = (POINTER) edef->elem_detail->attributes;
	Edefs[ifound]   = edef;

	/* Interpret the attach option */
	if (!check_attach_option(edef->fld_type,
			cfg_ldef->type_attach_opts[ifound], &ldef->attach))
		return (LAB_DEF *)0;

	/* Build pre-defined attribute list */
	CAL_add_attribute(ldef->cal, AttribLabelType, ltype);
	cfg_attlist = cfg_ldef->type_attribs + ifound;
	if (NotNull(cfg_attlist))
		{
		for (iatt=0; iatt<cfg_attlist->nattrib_defs; iatt++)
			{
			attname = cfg_attlist->attrib_def_names[iatt];
			attval  = cfg_attlist->attrib_def_values[iatt];
			CAL_add_attribute(ldef->cal, attname, attval);
			}
		}

	return ldef;
	}

/***********************************************************************
*                                                                      *
*     f i n d _ p r e s e t _ l a b e l _ t y p e                      *
*     s a m e _ l a b e l _ t y p e                                    *
*     e q u i v _ l a b e l _ t y p e                                  *
*                                                                      *
***********************************************************************/

static	LAB_DEF	*find_preset_label_type

	(
	STRING	type
	)

	{
	LAB_DEF		*ldef;
	ATT_LIST	*alist;
	int			i, j;

	/* Build label def list */
	if (!LabDefsReady)
		{
		for (i=0; i<NLabDefs; i++)
			{
			ldef = LabDefs + i;

			/* Duplicate attribute info */
			ldef->cal = CAL_create_empty();
			CAL_add_attribute(ldef->cal, AttribLabelType, ldef->type);
			for (j=0; j<NAttList; j++)
				{
				alist = AttList + j;
				if (same(alist->type, ldef->type))
					{
					CAL_add_attribute(ldef->cal, alist->adef.name,
							alist->adef.value);
					}
				}
			}

		LabDefsReady = TRUE;
		}

	/* Is it a built-in auto/combination label type */
	type = equiv_label_type(type, FALSE);

	/* Find the label def */
	for (i=0; i<NLabDefs; i++)
		{
		ldef = LabDefs + i;
		if (same(ldef->type, type)) return ldef;
		}

	return (LAB_DEF *)0;
	}

/**********************************************************************/

static	LOGICAL	same_label_type

	(
	STRING	type1,
	STRING	type2
	)

	{
	if (blank(type1)) return FALSE;
	if (blank(type2)) return FALSE;

	type1 = equiv_label_type(type1, TRUE);
	type2 = equiv_label_type(type2, TRUE);
	return same(type1, type2);
	}

/**********************************************************************/

static	STRING	equiv_label_type

	(
	STRING	type,
	LOGICAL	back
	)

	{
	if (blank(type)) return NULL;

	/* Is it a built-in auto/combination label type */
	if (same(type, FpaLabellingContinuous))     return "auto-spline";
	else if (same(type, FpaLabellingDiscrete))  return "auto-area";
	else if (same(type, FpaLabellingWindBarb))  return "auto-wind";
	else if (same(type, FpaLabellingWindArea))  return "auto-adjust";
	else if (same(type, FpaLabellingLine))      return "auto-line";
	else if (same(type, FpaLabellingScattered)) return "auto-scat";
	if (!back) return type;

	/* Is it the result of a built-in auto/combination label type */
	if (same(type, "contour"))                  return "auto-spline";
	else if (same(type, "low_at_min"))          return "auto-spline";
	else if (same(type, "high_at_max"))         return "auto-spline";
	else if (same(type, "area"))                return "auto-area";
	else if (same(type, "wind"))                return "auto-wind";
	else if (same(type, "adjustment"))          return "auto-adjust";
	else if (same(type, "line"))                return "auto-line";
	return type;
	}
