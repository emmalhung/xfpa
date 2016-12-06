/***********************************************************************
*                                                                      *
*     s c r a t c h p a d . c                                          *
*                                                                      *
*     This module of the INteractive GRaphics EDitor (INGRED) handles  *
*     all edit and display functions for the scratchpad editor.        *
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

/* Scratchpad edit objects */
static	STRING		ScratchEnt   = "c";		/* field type */
static	STRING		ScratchElem  = "NULL";	/* element */
static	STRING		ScratchLevel = "NULL";	/* level */
static	SET			ScratchLine  = NULL;	/* annotation curves */
static	SET			ScratchLabel = NULL;	/* annotation labels */
static	SET			ScratchMark  = NULL;	/* annotation marks */

/* Identifier for scratchpad display of distance between two points */
static	const	STRING	SpanID = "span";

/* Tag for degree symbol */
#define Dsymbol  "\260"

/***********************************************************************
*                                                                      *
*     s h o w _ s c r a t c h                                          *
*     h i d e _ s c r a t c h                                          *
*     p r e s e n t _ s c r a t c h                                    *
*                                                                      *
***********************************************************************/

LOGICAL	show_scratch(void)

	{
	if (ScratchShown) return TRUE;

	/* Set display state on */
	define_dn_vis(DnScratch, TRUE);
	ScratchShown = TRUE;

	/* Read in the scratchpad if necessary */
	if (!get_scratch()) return FALSE;

	/* Re-display */
	present_all();
	return TRUE;
	}

LOGICAL	hide_scratch(void)

	{
	if (!ScratchShown) return TRUE;

	/* Turn off active element */
	/* ??? pick_active_dfield("NONE", NULL); */

	/* Set display state off */
	define_dn_vis(DnScratch, FALSE);
	ScratchShown = FALSE;
	present_all();
	return TRUE;
	}

LOGICAL	present_scratch

	(
	LOGICAL	all
	)

	{
	if (!ScratchShown)  return TRUE;

	/* Show the whole thing if requested */
	if (all) present_node(DnScratch);

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     g e t _ s c r a t c h                                            *
*                                                                      *
***********************************************************************/

LOGICAL	get_scratch(void)

	{
	STRING		stype;
	POINTER		sdata;
	METAFILE	meta;
	FIELD		fld;
	SET			set;

	/* See if the scratchpad has already been read in */
	recall_dn_data(DnScratch, &stype, &sdata);
	if (same(stype, "metafile") && sdata) return TRUE;

	/* Read in the scratchpad metafile */
	put_message("scratch-loading");
#	ifdef DEBUG
	(void) printf("         Loading Scratch Pad %s\n", ScratchPad.wfile);
#	endif /* DEBUG */
	suspend_zoom();
	if (!input_map(DnScratch, ScratchPad.wfile, MapProj, TRUE))
	    {
		/* No current scratchpad file */
	    /* Create empty metafile structure and add it to the dispnode */
	    meta = create_metafile();
	    define_mf_projection(meta, MapProj);
	    define_dn_xform(DnScratch, "map", MapView, NullBox, MapProj,
						NullXform);
	    define_dn_data(DnScratch, "metafile", (POINTER)meta);
	    }
	resume_zoom(FALSE);

	/* Set up the presentation specs for this metafile and display it */
	ScratchMeta = DnScratch->data.meta;
	setup_metafile_presentation(ScratchMeta, "FPA");
	(void) present_scratch(TRUE);

	/* Extract curve field */
	fld = find_mf_field(ScratchMeta, "set", "curve", ScratchEnt, ScratchElem,
						ScratchLevel);
	if (IsNull(fld))
	    {
		/* No curve field */
		/* Create an empty curve field and add it to the metafile structure */
	    fld = create_field(ScratchEnt, ScratchElem, ScratchLevel);
	    set = create_set("curve");
	    define_fld_data(fld, "set", (POINTER)set);
	    add_field_to_metafile(ScratchMeta, fld);
	    }
	ScratchLine = fld->data.set;

	/* Extract label field */
	fld = find_mf_field(ScratchMeta, "set", "label", ScratchEnt, ScratchElem,
						ScratchLevel);
	if (IsNull(fld))
	    {
		/* No label field */
		/* Create an empty label field and add it to the metafile structure */
	    fld = create_field(ScratchEnt, ScratchElem, ScratchLevel);
	    set = create_set("label");
	    define_fld_data(fld, "set", (POINTER)set);
	    add_field_to_metafile(ScratchMeta, fld);
	    }
	ScratchLabel = fld->data.set;

	/* Extract mark field */
	fld = find_mf_field(ScratchMeta, "set", "mark", ScratchEnt, ScratchElem,
						ScratchLevel);
	if (IsNull(fld))
	    {
		/* No mark field */
		/* Create an empty mark field and add it to the metafile structure */
	    fld = create_field(ScratchEnt, ScratchElem, ScratchLevel);
	    set = create_set("mark");
	    define_fld_data(fld, "set", (POINTER)set);
	    add_field_to_metafile(ScratchMeta, fld);
	    }
	ScratchMark = fld->data.set;

	/* Done */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     u p d a t e _ s c r a t c h                                      *
*                                                                      *
***********************************************************************/

LOGICAL	update_scratch(void)

	{
	if (!ViewOnly)
		{
		(void) remove_file(ScratchPad.wfile, NULL);
		write_metafile(ScratchPad.wfile, ScratchMeta, MaxDigits);
		}
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     s c r a t c h _ c h e c k                                        *
*     s c r a t c h _ e d i t                                          *
*                                                                      *
***********************************************************************/

LOGICAL	scratch_check(void)

	{
	if (!ScratchShown) return FALSE;

	active_scratch_fields(FALSE, ScratchLine, ScratchLabel, ScratchMark);

	set_Xcurve_modes("draw");

	return TRUE;
	}

/**********************************************************************/

LOGICAL	scratch_edit

	(
	STRING	mode
	)

	{
	int		drawn = FALSE;
	int		cd;
	STRING	cp, colour, style, width, font, size;
	float	dist;
	char	splist[128];
	LOGICAL	valid;

	static	CAL	ScratchCal  = NullCal;
	static	CAL	ScratchCall = NullCal;
	static	CAL	ScratchCalx = NullCal;
	static	CAL	ScratchCalm = NullCal;

	static	const	STRING	lfmt = "[colour='%s' style='%s' width='%s']";
	static	const	STRING	tfmt = "[colour='%s' font='%s' size='%s']";
	static	const	STRING	mfmt = "[colour='%s' type='circle' size='8']";

#	ifdef DEBUG
	pr_diag("Editor", "[scratch_edit] %s\n", mode);
#	endif /* DEBUG */

	/* Ignore accept and undo modes */
	if (same(mode, "confirm") || same(mode, "undo"))
		{
		return scratch_check();
		}

	/* Handle "draw" edit */
	if (same(EditMode, "DRAW"))
	    {
		if (IsNull(ScratchCal)) ScratchCal = CAL_create_default();

		/* Set line colour, style and width from edit parameters */
		/* Note that style and width are concatenated in one parameter */
	    find_colour(colour=EditVal[0], &valid);	if (!valid) colour = "Black";
		if ((cp=strchr(EditVal[1], '.')) != NULL)
			{
			cd = cp - EditVal[1];
			(void) strcpy(EditVal[2], EditVal[1]);
			(void) strcpy(EditVal[1], EditVal[2]+cd+1);
			(void) strcpy(EditVal[2]+cd, "");
			}
	    find_lstyle(style=EditVal[1], &valid);	if (!valid) style  = "solid";
	    find_lwidth(width=EditVal[2], &valid);	if (!valid) width  = "thin";
		(void) sprintf(splist, lfmt, colour, style, width);
		CAL_set_defaults(ScratchCal, "LSPEC", splist, NULL);

		/* Draw and display scratchpad line */
	    drawn = edit_draw_generic_line(mode, ScratchCal);
	    }

	/* Handle "text" edit */
	else if (same(EditMode, "TEXT"))
	    {
		if (IsNull(ScratchCal)) ScratchCal = CAL_create_default();

		/* Set text colour, font and size from edit parameters */
	    find_colour(colour=EditVal[0], &valid);	if (!valid) colour = "Black";
	    find_font(  font  =EditVal[1], &valid); if (!valid) font   = "simplex";
	    find_size(  size  =EditVal[2], &valid);	if (!valid) size   = "medium";
		(void) sprintf(splist, tfmt, colour, font, size);
		CAL_set_defaults(ScratchCal, "TSPEC", splist, EditVal[3]);

		/* Draw and display scratchpad text */
	    drawn = edit_place_generic_label(mode, ScratchCal);
	    }

	/* Handle "distance" edit */
	else if (same(EditMode, "DISTANCE"))
	    {
		if (IsNull(ScratchCall)) ScratchCall = CAL_create_default();
		if (IsNull(ScratchCalx)) ScratchCalx = CAL_create_default();
		if (IsNull(ScratchCalm)) ScratchCalm = CAL_create_default();

		/* Set line colour, style and width from edit parameters */
		/* Note that style and width are concatenated in one parameter */
	    find_colour(colour=EditVal[0], &valid);	if (!valid) colour = "Black";
		if ((cp=strchr(EditVal[1], '.')) != NULL)
			{
			cd = cp - EditVal[1];
			(void) strcpy(EditVal[6], EditVal[5]);
			(void) strcpy(EditVal[5], EditVal[4]);
			(void) strcpy(EditVal[4], EditVal[3]);
			(void) strcpy(EditVal[3], EditVal[2]);
			(void) strcpy(EditVal[2], EditVal[1]);
			(void) strcpy(EditVal[1], EditVal[2]+cd+1);
			(void) strcpy(EditVal[2]+cd, "");
			}
	    find_lstyle(style=EditVal[1], &valid);	if (!valid) style  = "solid";
	    find_lwidth(width=EditVal[2], &valid);	if (!valid) width  = "thin";
		(void) sprintf(splist, lfmt, colour, style, width);
		CAL_set_defaults(ScratchCall,  "LSPEC", splist, NULL);
		CAL_add_attribute(ScratchCall, AttribLineType, SpanID);

		/* Set label colour, font and size from edit parameters */
	    find_colour(colour=EditVal[3], &valid);	if (!valid) colour = "Black";
	    find_font(  font  =EditVal[4], &valid); if (!valid) font   = "simplex";
	    find_size(  size  =EditVal[5], &valid);	if (!valid) size   = "medium";
		(void) sprintf(splist, tfmt, colour, font, size);
		CAL_set_defaults(ScratchCalx,  "TSPEC", splist, NULL);
		CAL_add_attribute(ScratchCalx, AttribLabelType, SpanID);

		/* Set mark colour from label colour above */
		/* Note that the mark is pre-defined as a circle */
		(void) sprintf(splist, mfmt, colour);
		CAL_set_defaults(ScratchCalm,  "MSPEC", splist, NULL);
		CAL_add_attribute(ScratchCalm, AttribLabelType, SpanID);

		/* Draw and display scratchpad span between two points */
	    drawn = edit_draw_generic_span(mode,
								ScratchCall, ScratchCalx, ScratchCalm, -1.0);
	    }

	/* Handle "preset distance" edit */
	else if (same(EditMode, "PRESET_DISTANCE"))
	    {
		if (IsNull(ScratchCall)) ScratchCall = CAL_create_default();
		if (IsNull(ScratchCalx)) ScratchCalx = CAL_create_default();
		if (IsNull(ScratchCalm)) ScratchCalm = CAL_create_default();

		/* Set line colour, style and width from edit parameters */
		/* Note that style and width are concatenated in one parameter */
	    find_colour(colour=EditVal[0], &valid);	if (!valid) colour = "Black";
		if ((cp=strchr(EditVal[1], '.')) != NULL)
			{
			cd = cp - EditVal[1];
			(void) strcpy(EditVal[6], EditVal[5]);
			(void) strcpy(EditVal[5], EditVal[4]);
			(void) strcpy(EditVal[4], EditVal[3]);
			(void) strcpy(EditVal[3], EditVal[2]);
			(void) strcpy(EditVal[2], EditVal[1]);
			(void) strcpy(EditVal[1], EditVal[2]+cd+1);
			(void) strcpy(EditVal[2]+cd, "");
			}
	    find_lstyle(style=EditVal[1], &valid);	if (!valid) style  = "solid";
	    find_lwidth(width=EditVal[2], &valid);	if (!valid) width  = "thin";
		(void) sprintf(splist, lfmt, colour, style, width);
		CAL_set_defaults(ScratchCall,  "LSPEC", splist, NULL);
		CAL_add_attribute(ScratchCall, AttribLineType, SpanID);

		/* Set label colour, font and size from edit parameters */
	    find_colour(colour=EditVal[3], &valid);	if (!valid) colour = "Black";
	    find_font(  font  =EditVal[4], &valid); if (!valid) font   = "simplex";
	    find_size(  size  =EditVal[5], &valid);	if (!valid) size   = "medium";
		(void) sprintf(splist, tfmt, colour, font, size);
		CAL_set_defaults(ScratchCalx,  "TSPEC", splist, NULL);
		CAL_add_attribute(ScratchCalx, AttribLabelType, SpanID);

		/* Set mark colour from label colour above */
		/* Note that the mark is pre-defined as a circle */
		(void) sprintf(splist, mfmt, colour);
		CAL_set_defaults(ScratchCalm,  "MSPEC", splist, NULL);
		CAL_add_attribute(ScratchCalm, AttribLabelType, SpanID);

		/* Set preset distance */
		if (sscanf(EditVal[6], "%g", &dist) < 1 || dist <= 0.0)
			{
			put_message("scratch-span-no-dist");
			(void) sleep(1);
			return FALSE;
			}

		/* Convert distance from km to m! */
		dist *= 1000.0;

		/* Draw and display scratchpad span between two points */
	    drawn = edit_draw_generic_span(mode,
								ScratchCall, ScratchCalx, ScratchCalm, dist);
	    }

	/* Handle "select" edit */
	else if (same(EditMode, "SELECT"))
	    {
	    drawn = edit_select_generic_feature(mode);
		}

	/* Unknown command */
	else
	    {
		put_message("scratch-unsupported", EditMode, "", "");
		(void) sleep(1);
		return FALSE;
	    }

	if (drawn) (void) update_scratch();
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     d r a w _ g e n e r i c _ l i n e                                *
*                                                                      *
***********************************************************************/

LOGICAL	edit_draw_generic_line

	(
	STRING	mode,
	CAL		cal
	)

	{
	int		butt;
	CURVE	curve;
	LINE	*lines;
	int		nlines;
	LOGICAL	drawn = FALSE;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_draw_generic_line] %s\n", mode);
#	endif /* DEBUG */

	if (same_start(mode, "cancel"))
		{
		ignore_partial();
		empty_temp();
		drawing_control(FALSE);
		return FALSE;
		}

	/* Make sure the curve set contains the right stuff */
	if (!EditCurves)                      return FALSE;
	if (!same(EditCurves->type, "curve")) return FALSE;

	/* Initialize on startup */
	if (same(mode, "begin"))
		{
		cancel_partial();
		empty_temp();
		present_all();
		}

	/* Repeat until told to quit */
	while (TRUE)
		{
		/* Put up a prompt */
		put_message("line-draw");

		/* Draw a curve */
		if (ready_Xcurve()) butt = LeftButton;
		else if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return drawn;

		/* Invalid button */
		if ((butt != LeftButton))
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}
		post_partial(FALSE);

		/* Draw the curve */
		drawing_control(TRUE);
		if (!ready_Xcurve())
			{
			if (drawing_Xcurve()) return drawn;
			put_message("line-draw-rel");
			(void) draw_Xcurve(DnEdit, FilterRes, SplineRes, FALSE);
			if (!ready_Xcurve()) return drawn;
			}
		drawing_control(FALSE);

		/* Dump the curve */
		nlines = recall_Xcurve(&lines);
		if (nlines <= 0)
			{
			put_message("edit-no-draw");
			reset_pipe();
			(void) sleep(1);
			present_all();
			continue;
			}
		if (line_too_short(lines[0], SplineRes))
			{
			put_message("edit-too-short");
			reset_pipe();
			(void) sleep(1);
			present_all();
			continue;
			}

		/* Transfer the new data to the curve set */
		put_message("line-draw-proc");
		curve = create_curve(NULL, NULL, NULL);
		define_item_attribs("curve", (ITEM) curve, cal);
		add_line_to_curve(curve, lines[0]);
		add_item_to_set(EditCurves, (ITEM) curve);
		reset_pipe();

		/* Set up display specs for the new curve */
		invoke_set_catspecs(EditCurves);
		if (EditUndoable) highlight_set(EditCurves, 1);
		drawn = TRUE;

		/* Re-display drawn curves */
		if (EditUndoable) post_mod("curves");
		present_all();
		}
	}

/***********************************************************************
*                                                                      *
*     d r a w _ g e n e r i c _ s p a n                                *
*                                                                      *
***********************************************************************/

LOGICAL	edit_draw_generic_span

	(
	STRING	mode,
	CAL		cal,
	CAL		calx,
	CAL		calm,
	float	dist
	)

	{
	int		butt;
	float	xdist, tang;
		/* >>>>> use this instead for heading
		float	xdist, xang, tang, lat0, lon0;
		double	xx, yy;
		<<<<< */
	char	gcbuf[128];
	POINT	pos, p1;
	MARK	mark;
		/* >>>>> use this instead for begin/end span marks
		MARK	mark, mark0, mark1;
		<<<<< */
	LABEL	label;
	CURVE	curve;
	LOGICAL	drawn = FALSE;

	/* Current state: */
	/* - where should we resume? */
	static  enum    { StartSpan, EndSpan } State = StartSpan;

	/* Start of span */
	static	POINT		Spos = ZERO_POINT;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_draw_generic_span] %s\n", mode);
#	endif /* DEBUG */

	if (same_start(mode, "cancel"))
		{
		ignore_partial();
		empty_temp();
		State = StartSpan;
		return FALSE;
		}

	/* Make sure the curve set contains the right stuff */
	if (!EditCurves)                      return FALSE;
	if (!same(EditCurves->type, "curve")) return FALSE;

	/* Initialize on startup */
	if (same(mode, "begin"))
		{
		cancel_partial();
		empty_temp();
		present_all();
		State = StartSpan;
		}

	/* Repeat until told to quit */
	while (TRUE)
		{
		/* Put up a prompt */
		if (State == StartSpan) put_message("scratch-span");
		else                    put_message("scratch-span-end");

		/* Get the point */
		if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return drawn;
		if ((butt != LeftButton))
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}
		post_partial(FALSE);

		/* Resume at current stage */
		switch (State)
			{

			/* Set the start point for the span */
			case StartSpan:

				/* Pick the start point */
				(void) pick_Xpoint(DnEdit, 0, Spos, &butt);
				if (!inside_dn_window(DnEdit, Spos))
					{
					put_message("edit-outside-map");
					(void) sleep(1);
					continue;
					}

				/* Display the start point marker */
				mark = create_mark("", "", "", Spos, 0.0);
				define_mspec(&mark->mspec, 0, 1, NULL, 0, False, 8.0, 0.0,
							 (HILITE) 2);
				add_item_to_metafile(TempMeta, "mark", "d", "", "",
										(ITEM) mark);
				present_temp(TRUE);

				/* Move on to next stage */
				State = EndSpan;
				continue;

			/* Choose an end point for the span */
			case EndSpan:

				if (!ready_Xpoint(DnEdit, p1, &butt)) return drawn;
				if ((butt != LeftButton))
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!inside_dn_window(DnEdit, p1))
					{
					put_message("scratch-span-out");
					(void) ignore_Xpoint();
					continue;
					}

				/* Move the end point to the desired position */
				put_message("scratch-span-rel");
				(void) utrack_Xspan(DnEdit, Spos, p1, dist, &butt);

				/* Transfer the new span to the curve set */
				put_message("scratch-span-proc");
				curve = create_curve(NULL, NULL, NULL);
				define_item_attribs("curve", (ITEM) curve, cal);
				add_point_to_curve(curve, Spos);
				add_point_to_curve(curve, p1);
				add_item_to_set(EditCurves, (ITEM) curve);

				/* Add the start mark for each curve */
				mark = create_mark("", "", "", Spos, 0.0);
				define_item_attribs("mark", (ITEM) mark, calm);
				add_item_to_set(EditMarks, (ITEM) mark);

					/* Add the start and end marks for each curve */
					/* >>>>> use this instead for begin/end span marks
					mark0 = create_mark("", "", "", Spos, 0.0);
					mark1 = create_mark("", "", "", p1, 0.0);
					define_item_attribs("mark", (ITEM) mark0, calm);
					define_item_attribs("mark", (ITEM) mark1, calm);
					add_item_to_set(EditMarks, (ITEM) mark0);
					add_item_to_set(EditMarks, (ITEM) mark1);
					<<<<< */

				/* Determine the great circle distance */
				xdist = great_circle_distance(MapProj, Spos, p1);

				/* Determine bearing in degrees true */
				tang = great_circle_bearing(MapProj, Spos, p1);

					/* Determine heading in degrees true */
					/* >>>>>
					xx   = p1[X] - Spos[X];
					yy   = p1[Y] - Spos[Y];
					xang = (float) atan2deg(yy, xx);
					(void) pos_to_ll(MapProj, Spos, &lat0, &lon0);
					tang = wind_dir_true(MapProj, lat0, lon0, xang);
					<<<<< */

				/* Add the label for each curve halfway along the span */
				(void) sprintf(gcbuf, "%d@%d%s",
								NINT(xdist/1000.0), NINT(tang), Dsymbol);
				pos[X] = (Spos[X] + p1[X]) / 2.0;
				pos[Y] = (Spos[Y] + p1[Y]) / 2.0;
				label = create_label(NULL, NULL, NULL, pos, 0.0);
				define_tspec(&label->tspec, 0, SafeFont, False, LabelSize, 0.0,
							 Hc, Vc, (HILITE) 0);
				define_item_attribs("label", (ITEM) label, calx);
				define_label_value(label, label->subelem, label->value,
					safe_strdup(gcbuf));
				add_item_to_set(EditLabs, (ITEM) label);

				/* Set up display specs for the new span and label */
				invoke_set_catspecs(EditCurves);
				if (EditUndoable) highlight_set(EditCurves, 1);
				invoke_set_catspecs(EditLabs);
				/* >>>>> remove May 2007
				if (EditColour >= 0) change_set_pspec(EditLabs, TEXT_COLOUR,
												  (POINTER)&EditColour);
				<<<<< */
				if (EditUndoable) highlight_set(EditLabs, 1);
				drawn = TRUE;
				if (EditUndoable) post_mod("curves");

				/* Move on to next stage */
				empty_temp();
				State = StartSpan;
				continue;
			}
		}
	}

/***********************************************************************
*                                                                      *
*     p l a c e _ g e n e r i c _ l a b e l                            *
*                                                                      *
***********************************************************************/

/*ARGSUSED*/
LOGICAL	edit_place_generic_label

	(
	STRING	mode,
	CAL		cal
	)

	{
	int		butt;
	float	ldist;
	STRING	text, ltype;
	POINT	pos, cpos;
	LABEL	label, lab2;
	LOGICAL	drawn = FALSE;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_place_generic_label] %s\n", mode);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same_start(mode, "cancel"))
		{
		ignore_partial();
		return FALSE;
		}

	/* Make sure the label set contains the right stuff */
	if (!EditLabs)                      return FALSE;
	if (!same(EditLabs->type, "label")) return FALSE;
	text = CAL_get_attribute(cal, CALuserlabel);

	/* Initialize on startup */
	if (same(mode, "begin"))
		{
		cancel_partial();
		/* >>>>> should not need this???
		empty_temp();
		<<<<< */
		present_all();
		}

	/* Repeat until told to quit */
	while (TRUE)
		{
		/* Put up a prompt */
		put_message("label-text");

		/* Pick a point */
		if (!ready_Xpoint(DnEdit, pos, &butt)) return drawn;
		if ((butt != LeftButton))
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}
		post_partial(FALSE);

		/* See if we are very close to an existing label */
		/*  which is not a legend or SpanID label!       */
		label = closest_label(EditLabs, pos, NULL, &ldist, NullPoint);
		if (NotNull(label))
			{
			ltype = CAL_get_attribute(label->attrib, AttribLabelType);
			if (ldist > PickTol*2)                  label = NullLabel;
			else if (same(label->subelem,"legend")) label = NullLabel;
			else if (same_ic(ltype, SpanID))        label = NullLabel;
			}

		/* See if we want to move a label */
		if (NotNull(label))
			{
			SET		bset;

			/* Build a tracking marker */
			bset = create_set("label");
			lab2 = copy_label(label);
			add_item_to_set(bset, (ITEM) lab2);
			highlight_set(bset, 2);
			display_set(bset);

			/* Get the new location */
			put_message("label-move-rel");
			(void) utrack_Xpoint(DnEdit, bset, NullPoint, cpos, &butt);
			(void) destroy_set(bset);
			if (!inside_dn_window(DnEdit, cpos))
				{
				put_message("label-move-out3");
				present_all();
				(void) sleep(1);
				continue;
				}

			/* Moving a label */
			copy_point(label->anchor, cpos);
			invoke_set_catspecs(EditLabs);
			/* >>>>> remove May 2007
			if (EditColour >= 0) change_set_pspec(EditLabs, TEXT_COLOUR,
											  (POINTER)&EditColour);
			<<<<< */
			if (EditUndoable) highlight_set(EditLabs, 1);

			/* Re-display the scratchpad */
			if (EditUndoable) post_mod("labs");
			drawn = TRUE;
			present_all();
			}

		/* Inserting a label */
		else
			{
			/* Complain if no text given */
			if (blank(text))
				{
				put_message("label-text-empty");
				(void) ignore_Xpoint();
				continue;
				}

			put_message("label-text-add");
			(void) pick_Xpoint(DnEdit, 0, pos, &butt);
			if (!inside_dn_window(DnEdit, pos))
				{
				put_message("edit-outside-map");
				(void) sleep(1);
				continue;
				}

			post_partial(FALSE);
			put_message("label-adding");
			label = create_label(NULL, NULL, text, pos, 0.0);
			define_item_attribs("label", (ITEM) label, cal);
			define_tspec(&label->tspec, 0, SafeFont, False, LabelSize, 0.0,
						 Hc, Vc, (HILITE) 0);
			add_item_to_set(EditLabs, (ITEM) label);
			invoke_set_catspecs(EditLabs);
			/* >>>>> remove May 2007
			if (EditColour >= 0) change_set_pspec(EditLabs, TEXT_COLOUR,
											  (POINTER)&EditColour);
			<<<<< */
			if (EditUndoable) highlight_set(EditLabs, 1);

			/* Display the scratchpad */
			if (EditUndoable) post_mod("labs");
			drawn = TRUE;
			(void) present_scratch(TRUE);
			}
		}
	}

/***********************************************************************
*                                                                      *
*     s e l e c t _ g e n e r i c _ f e a t u r e                      *
*                                                                      *
***********************************************************************/

typedef	enum	{ FeatureLine, FeatureSpan, FeatureText } FEATURE;

LOGICAL	edit_select_generic_feature

	(
	STRING	mode
	)

	{
	int		npick, nn, ii;
	float	cdist, mdist, ldist, width, size;
	STRING	ctype, ltype, mtype, text;
	FONT	font;
	FEATURE	ftype;
	POINT	pos, mpos, lpos, spos, epos, xpos;
	SET		set;
	LINE	line;
	CURVE	curve, ccopy;
	MARK	mark,  mcopy;
	LABEL	label, lcopy;
	int		butt;
	LOGICAL	drawn = FALSE;

	/* Current state: */
	/* - where should we resume? */
	static  enum    { Pick, PickAll, Delete } State = Pick;

	/* Pick list: */
	/* - list of features to be deleted */
	static	const int	maxpick = 10000;
	static	int			npickL  = 0;
	static	int			apickL  = 0;
	static	CURVE		*cpickL = NullCurveList;
	static	CURVE		*ccopyL = NullCurveList;
	static	int			npickS  = 0;
	static	int			apickS  = 0;
	static	CURVE		*cpickS = NullCurveList;
	static	CURVE		*ccopyS = NullCurveList;
	static	MARK		*mpickS = NullMarkList;
	static	MARK		*mcopyS = NullMarkList;
	static	LABEL		*lpickS = NullLabelList;
	static	LABEL		*lcopyS = NullLabelList;
	static	int			npickT  = 0;
	static	int			apickT  = 0;
	static	LABEL		*lpickT = NullLabelList;
	static	LABEL		*lcopyT = NullLabelList;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_select_generic_feature] %s\n", mode);
#	endif /* DEBUG */

	if (same_start(mode, "cancel"))
		{
		empty_temp();
		npickL = 0;
		apickL = 0;
		npickS = 0;
		apickS = 0;
		npickT = 0;
		apickT = 0;
		FREEMEM(cpickL);
		FREEMEM(ccopyL);
		FREEMEM(cpickS);
		FREEMEM(ccopyS);
		FREEMEM(mpickS);
		FREEMEM(mcopyS);
		FREEMEM(lpickS);
		FREEMEM(lcopyS);
		FREEMEM(lpickT);
		FREEMEM(lcopyT);
		scratch_can_delete(FALSE);
		State  = Pick;
		return FALSE;
		}

	/* Make sure the curve and label sets contain the right stuff */
	if (!EditCurves)                      return FALSE;
	if (!same(EditCurves->type, "curve")) return FALSE;
	if (!EditLabs)                        return FALSE;
	if (!same(EditLabs->type, "label"))   return FALSE;

	/* Initialize on startup */
	if (same(mode, "begin"))
		{
		cancel_partial();
		empty_temp();
		npickL = 0;
		apickL = 0;
		npickS = 0;
		apickS = 0;
		npickT = 0;
		apickT = 0;
		FREEMEM(cpickL);
		FREEMEM(ccopyL);
		FREEMEM(cpickS);
		FREEMEM(ccopyS);
		FREEMEM(mpickS);
		FREEMEM(mcopyS);
		FREEMEM(lpickS);
		FREEMEM(lcopyS);
		FREEMEM(lpickT);
		FREEMEM(lcopyT);
		present_all();
		scratch_can_delete(FALSE);
		State  = Pick;
		}

	/* Set state for "select all" */
	if (same(mode, "select all")) State = PickAll;

	/* Set state for Right button "delete" */
	if ( same(mode, "delete") && (npick = npickL + npickS + npickT) <= 0 )
		{
		scratch_can_delete(FALSE);
		put_message("scratch-no-features");
		(void) sleep(1);
		State = Pick;
		return FALSE;
		}
	if (same(mode, "delete")) State = Delete;

	/* Repeat until told to quit */
	while (TRUE)
		{

		/* Resume at current stage */
		switch (State)
			{

			/* Pick features to delete */
			case Pick:

				/* See if there are lines, spans or labels to delete */
				if (EditCurves->num <= 0 && EditLabs->num <= 0)
					{
					scratch_can_delete(FALSE);
					put_message("scratch-no-features");
					return drawn;
					}

				/* Allow specification of multiple moves */
				while ((npick = npickL + npickS + npickT) < maxpick)
					{

					/* Put up a prompt */
					if (npick <= 0) put_message("scratch-feature-pick");
					else            put_message("scratch-feature-pick2");

					/* Have we picked any features? */
					if (npick > 0) scratch_can_delete(TRUE);
					else           scratch_can_delete(FALSE);

					/* Get the point entered */
					if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return drawn;
					if (butt != LeftButton)
						{
						put_message("edit-wrong-button");
						(void) ignore_Xpoint();
						continue;
						}
					put_message("scratch-feature-rel");
					if (!pick_Xpoint(DnEdit, 0, pos, &butt)) continue;
					if (!inside_dn_window(DnEdit, pos))
						{
						put_message("edit-outside-map");
						(void) sleep(1);
						continue;
						}
					post_partial(FALSE);

					/* See which line, span or label we picked */
					curve = closest_curve(EditCurves, pos, &cdist, NULL, NULL);
					label = closest_label(EditLabs, pos, NULL, &ldist, lpos);
					if (IsNull(curve) && IsNull(label))
						{
						put_message("scratch-no-features");
						(void) sleep(1);
						continue;
						}

					/* See which feature type is closest */
					if (NotNull(curve) && NotNull(label))
						{
						ctype = CAL_get_attribute(curve->attrib,
													AttribLineType);
						ltype = CAL_get_attribute(label->attrib,
													AttribLabelType);

						/* Determine which type of curve for "legend" label */
						if (same(label->subelem, "legend"))
							{
							if (same_ic(ctype, SpanID)) ftype = FeatureSpan;
							else                        ftype = FeatureLine;
							}

						/* Determine which type of label */
						else if (ldist < cdist)
							{
							if (same_ic(ltype, SpanID)) ftype = FeatureSpan;
							else                        ftype = FeatureText;
							}

						/* Determine which type of curve */
						else
							{
							if (same_ic(ctype, SpanID)) ftype = FeatureSpan;
							else                        ftype = FeatureLine;
							}
						}
					else if (NotNull(curve))
						{
						ctype = CAL_get_attribute(curve->attrib,
													AttribLineType);
						if (same_ic(ctype, SpanID)) ftype = FeatureSpan;
						else                        ftype = FeatureLine;
						}
					else
						{
						if (same(label->subelem, "legend")) continue;
						ltype = CAL_get_attribute(label->attrib,
													AttribLabelType);
						if (same_ic(ltype, SpanID)) ftype = FeatureSpan;
						else                        ftype = FeatureText;
						}

					/* Add features to the appropriate list */
					switch (ftype)
						{
						
						/* Add to list of lines */
						case FeatureLine:

							/* If line was already picked, un-pick it */
							for (nn=0; nn<npickL; nn++)
								{
								if (curve == cpickL[nn]) break;
								}
							if (nn < npickL)
								{
								put_message("scratch-line-unpicked");

								set = find_mf_set(TempMeta,
													"curve", "c", "", "");
								ccopyL[nn] = remove_item_from_set(set,
													(ITEM) ccopyL[nn]);
								cpickL[nn] = NullCurve;
								npickL--;
								for ( ; nn<npickL; nn++)
									{
									cpickL[nn] = cpickL[nn+1];
									ccopyL[nn] = ccopyL[nn+1];
									}
								cpickL[nn] = NullCurve;
								ccopyL[nn] = NullCurve;

								present_all();
								}

							/* Otherwise pick it */
							else
								{
								put_message("scratch-line-picked");

								npickL++;
								if (npickL > apickL)
									{
									apickL = npickL;
									cpickL = GETMEM(cpickL, CURVE, apickL);
									ccopyL = GETMEM(ccopyL, CURVE, apickL);
									}

								/* Produce a copy of the picked curve */
								cpickL[nn] = curve;
								ccopyL[nn] = create_curve(NULL, NULL, NULL);
								ccopyL[nn]->line = copy_line(curve->line);
								ccopy      = ccopyL[nn];

								/* Highlight the selected curve */
								/* (using same line width)      */
								recall_lspec_value(&curve->lspec, LINE_WIDTH,
												   (POINTER)&width);
								define_lspec(&ccopy->lspec, 0, 0, NULL, False,
											 width, 0.0, (HILITE) 2);

								add_item_to_metafile(TempMeta, "curve",
													 "c", "", "", (ITEM) ccopy);
								present_temp(TRUE);
								}
							break;
						
						/* Add to list of spans */
						case FeatureSpan:

							/* If span was already picked, un-pick it */
							for (nn=0; nn<npickS; nn++)
								{
								if (curve == cpickS[nn]) break;
								}
							if (nn < npickS)
								{
								put_message("scratch-span-unpicked");

								set = find_mf_set(TempMeta,
													"curve", "c", "", "");
								ccopyS[nn] = remove_item_from_set(set,
													(ITEM) ccopyS[nn]);
								cpickS[nn] = NullCurve;

								set = find_mf_set(TempMeta,
													"mark", "d", "", "");
								mcopyS[nn] = remove_item_from_set(set,
													(ITEM) mcopyS[nn]);
								mpickS[nn] = NullMark;

								set = find_mf_set(TempMeta,
													"label", "d", "", "");
								lcopyS[nn] = remove_item_from_set(set,
													(ITEM) lcopyS[nn]);
								lpickS[nn] = NullLabel;

								npickS--;
								for ( ; nn<npickS; nn++)
									{
									cpickS[nn] = cpickS[nn+1];
									ccopyS[nn] = ccopyS[nn+1];
									mpickS[nn] = mpickS[nn+1];
									mcopyS[nn] = mcopyS[nn+1];
									lpickS[nn] = lpickS[nn+1];
									lcopyS[nn] = lcopyS[nn+1];
									}
								cpickS[nn] = NullCurve;
								ccopyS[nn] = NullCurve;
								mpickS[nn] = NullMark;
								mcopyS[nn] = NullMark;
								lpickS[nn] = NullLabel;
								lcopyS[nn] = NullLabel;

								present_all();
								}

							/* Otherwise pick it */
							else
								{
								put_message("scratch-span-picked");

								npickS++;
								if (npickS > apickS)
									{
									apickS = npickS;
									cpickS = GETMEM(cpickS, CURVE, apickS);
									ccopyS = GETMEM(ccopyS, CURVE, apickS);
									mpickS = GETMEM(mpickS, MARK,  apickS);
									mcopyS = GETMEM(mcopyS, MARK,  apickS);
									lpickS = GETMEM(lpickS, LABEL, apickS);
									lcopyS = GETMEM(lcopyS, LABEL, apickS);
									}

								/* Set endpoints of span */
								line = curve->line;
								(void) copy_point(spos, line->points[0]);
								(void) copy_point(epos,
												  line->points[line->numpts-1]);

								/* Find the mark that matches this span */
								(void) copy_point(xpos, spos);
								mark = closest_mark(EditMarks, xpos, NULL,
													&mdist, mpos);
								if (mdist > 1.0)
									{
									pr_diag("Editor",
										"[edit_select_generic_feature] Unattached span mark!\n");
									mark = NullMark;
									}

								/* Find the label that matches this span */
								xpos[X] = (spos[X] + epos[X]) / 2.0;
								xpos[Y] = (spos[Y] + epos[Y]) / 2.0;
								label = closest_label(EditLabs, xpos, NULL,
													  &ldist, lpos);
								if (ldist > 1.0)
									{
									pr_diag("Editor",
										"[edit_select_generic_feature] Unattached span label!\n");
									label = NullLabel;
									}

								/* Produce a copy of the picked span */
								cpickS[nn] = curve;
								ccopyS[nn] = create_curve(NULL, NULL, NULL);
								ccopyS[nn]->line = copy_line(curve->line);
								ccopy      = ccopyS[nn];

								mpickS[nn] = mark;
								if (NotNull(mark))
									{
									mcopyS[nn] = create_mark("", "", "",
															 mpos, 0.0);
									}
								else
									{
									mcopyS[nn] = NullMark;
									}
								mcopy      = mcopyS[nn];

								lpickS[nn] = label;
								if (NotNull(label))
									{
									(void) recall_label_value(label, NULL,
															  NULL, &text);
									lcopyS[nn] = create_label("", "", text,
															  lpos, 0.0);
									}
								else
									{
									lcopyS[nn] = NullLabel;
									}
								lcopy      = lcopyS[nn];

								/* Highlight the selected span           */
								/* (using same line width and text size) */
								recall_lspec_value(&curve->lspec, LINE_WIDTH,
												   (POINTER)&width);
								define_lspec(&ccopy->lspec, 0, 0, NULL, False,
											 width, 0.0, (HILITE) 2);

								add_item_to_metafile(TempMeta, "curve",
													 "c", "", "", (ITEM) ccopy);

								if (NotNull(mcopy))
									{
									define_mspec(&mcopy->mspec, 0, 1, NULL, 0,
												 False, 8.0, 0.0, (HILITE) 2);

									add_item_to_metafile(TempMeta, "mark", "d",
														 "", "", (ITEM) mcopy);
									}

								if (NotNull(lcopy))
									{
									recall_tspec_value(&label->tspec, TEXT_FONT,
													   (POINTER)&font);
									recall_tspec_value(&label->tspec, TEXT_SIZE,
													   (POINTER)&size);
									define_tspec(&lcopy->tspec, 0, font, False,
												 size, 0.0, Hc, Vc, (HILITE) 2);

									add_item_to_metafile(TempMeta, "label", "d",
														 "", "", (ITEM) lcopy);
									}

								present_temp(TRUE);
								}
							break;
						
						/* Add to list of labels */
						case FeatureText:

							/* If label was already picked, un-pick it */
							for (nn=0; nn<npickT; nn++)
								{
								if (label == lpickT[nn]) break;
								}
							if (nn < npickT)
								{
								put_message("scratch-text-unpicked");

								set = find_mf_set(TempMeta,
													"label", "d", "", "");
								lcopyT[nn] = remove_item_from_set(set,
													(ITEM) lcopyT[nn]);
								lpickT[nn] = NullLabel;
								npickT--;
								for ( ; nn<npickT; nn++)
									{
									lpickT[nn] = lpickT[nn+1];
									lcopyT[nn] = lcopyT[nn+1];
									}
								lpickT[nn] = NullLabel;
								lcopyT[nn] = NullLabel;

								present_all();
								}

							/* Otherwise pick it */
							else
								{
								put_message("scratch-text-picked");

								npickT++;
								if (npickT > apickT)
									{
									apickT = npickT;
									lpickT = GETMEM(lpickT, LABEL, apickT);
									lcopyT = GETMEM(lcopyT, LABEL, apickT);
									}

								/* Produce a copy of the picked label */
								lpickT[nn] = label;
								(void) recall_label_value(label,
														  NULL, NULL, &text);
								lcopyT[nn] = create_label("", "", text,
														  lpos, 0.0);
								lcopy      = lcopyT[nn];

								/* Highlight the selected label      */
								/*  (using same text font and size)  */
								recall_tspec_value(&label->tspec, TEXT_FONT,
												   (POINTER)&font);
								recall_tspec_value(&label->tspec, TEXT_SIZE,
												   (POINTER)&size);
								define_tspec(&lcopy->tspec, 0, font, False,
											 size, 0.0, Hc, Vc, (HILITE) 2);

								add_item_to_metafile(TempMeta, "label",
													 "d", "", "", (ITEM) lcopy);
								present_temp(TRUE);
								}
							break;
						}
					}

				pr_warning("Editor",
					"[edit_select_generic_feature]   End of State = Pick loop!\n");

				/* Have we picked any features? */
				if (npick > 0) scratch_can_delete(TRUE);
				else           scratch_can_delete(FALSE);

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Pick all features to delete */
			case PickAll:

				/* Empty pick list */
				empty_temp();
				npickL = 0;
				apickL = 0;
				npickS = 0;
				apickS = 0;
				npickT = 0;
				apickT = 0;
				FREEMEM(cpickL);
				FREEMEM(ccopyL);
				FREEMEM(cpickS);
				FREEMEM(ccopyS);
				FREEMEM(mpickS);
				FREEMEM(mcopyS);
				FREEMEM(lpickS);
				FREEMEM(lcopyS);
				FREEMEM(lpickT);
				FREEMEM(lcopyT);
				scratch_can_delete(FALSE);

				put_message("scratch-all-picked");
				post_partial(FALSE);

				/* Add all lines and spans to the feature lists */
				for (ii=0; ii<EditCurves->num; ii++)
					{
					curve = (CURVE) EditCurves->list[ii];
					if (IsNull(curve)) continue;

					/* Set feature type for each curve */
					ctype = CAL_get_attribute(curve->attrib, AttribLineType);
					if (same_ic(ctype, SpanID)) ftype = FeatureSpan;
					else                        ftype = FeatureLine;

					/* Add features to the appropriate list */
					switch (ftype)
						{
						
						/* Add to list of lines */
						case FeatureLine:

							npickL++;
							if (npickL > apickL)
								{
								apickL = npickL;
								cpickL = GETMEM(cpickL, CURVE, apickL);
								ccopyL = GETMEM(ccopyL, CURVE, apickL);
								}

							/* Produce a copy of the picked curve */
							nn = npickL - 1;
							cpickL[nn] = curve;
							ccopyL[nn] = create_curve(NULL, NULL, NULL);
							ccopyL[nn]->line = copy_line(curve->line);
							ccopy      = ccopyL[nn];

							/* Highlight the selected curve */
							/* (using same line width)      */
							recall_lspec_value(&curve->lspec, LINE_WIDTH,
											   (POINTER)&width);
							define_lspec(&ccopy->lspec, 0, 0, NULL, False,
										 width, 0.0, (HILITE) 2);

							add_item_to_metafile(TempMeta, "curve",
												 "c", "", "", (ITEM) ccopy);
							break;
						
						/* Add to list of spans */
						case FeatureSpan:

							npickS++;
							if (npickS > apickS)
								{
								apickS = npickS;
								cpickS = GETMEM(cpickS, CURVE, apickS);
								ccopyS = GETMEM(ccopyS, CURVE, apickS);
								mpickS = GETMEM(mpickS, MARK,  apickS);
								mcopyS = GETMEM(mcopyS, MARK,  apickS);
								lpickS = GETMEM(lpickS, LABEL, apickS);
								lcopyS = GETMEM(lcopyS, LABEL, apickS);
								}

							/* Set endpoints of span */
							line = curve->line;
							(void) copy_point(spos, line->points[0]);
							(void) copy_point(epos,
											  line->points[line->numpts-1]);

							/* Find the mark that matches this span */
							(void) copy_point(xpos, spos);
							mark = closest_mark(EditMarks, xpos, NULL,
												&mdist, mpos);
							if (mdist > 1.0)
								{
								pr_diag("Editor",
									"[edit_select_generic_feature] Unattached span mark!\n");
								mark = NullMark;
								}

							/* Find the label that matches this span */
							xpos[X] = (spos[X] + epos[X]) / 2.0;
							xpos[Y] = (spos[Y] + epos[Y]) / 2.0;
							label = closest_label(EditLabs, xpos, NULL,
												  &ldist, lpos);
							if (ldist > 1.0)
								{
								pr_diag("Editor",
									"[edit_select_generic_feature] Unattached span label!\n");
								label = NullLabel;
								}

							/* Produce a copy of the picked span */
							nn = npickS - 1;
							cpickS[nn] = curve;
							ccopyS[nn] = create_curve(NULL, NULL, NULL);
							ccopyS[nn]->line = copy_line(curve->line);
							ccopy      = ccopyS[nn];

							mpickS[nn] = mark;
							if (NotNull(mark))
								{
								mcopyS[nn] = create_mark("", "", "", mpos, 0.0);
								}
							else
								{
								mcopyS[nn] = NullMark;
								}
							mcopy      = mcopyS[nn];

							lpickS[nn] = label;
							if (NotNull(label))
								{
								(void) recall_label_value(label, NULL,
														  NULL, &text);
								lcopyS[nn] = create_label("", "", text,
														  lpos, 0.0);
								}
							else
								{
								lcopyS[nn] = NullLabel;
								}
							lcopy      = lcopyS[nn];

							/* Highlight the selected span           */
							/* (using same line width and text size) */
							recall_lspec_value(&curve->lspec, LINE_WIDTH,
											   (POINTER)&width);
							define_lspec(&ccopy->lspec, 0, 0, NULL, False,
										 width, 0.0, (HILITE) 2);

							add_item_to_metafile(TempMeta, "curve",
												 "c", "", "", (ITEM) ccopy);

							if (NotNull(mcopy))
								{
								define_mspec(&mcopy->mspec, 0, 1, NULL, 0,
											 False, 8.0, 0.0, (HILITE) 2);

								add_item_to_metafile(TempMeta, "mark",
													 "d", "", "", (ITEM) mcopy);
								}

							if (NotNull(lcopy))
								{
								recall_tspec_value(&label->tspec, TEXT_FONT,
												   (POINTER)&font);
								recall_tspec_value(&label->tspec, TEXT_SIZE,
												   (POINTER)&size);
								define_tspec(&lcopy->tspec, 0, font, False,
											 size, 0.0, Hc, Vc, (HILITE) 2);

								add_item_to_metafile(TempMeta, "label",
													 "d", "", "", (ITEM) lcopy);
								}
							break;
						}
					}

				/* Add all text labels to the feature list */
				for (ii=0; ii<EditLabs->num; ii++)
					{
					label = (LABEL) EditLabs->list[ii];
					if (IsNull(label)) continue;
					if (same(label->subelem,"legend")) continue;

					/* Set feature type for each label */
					ltype = CAL_get_attribute(label->attrib, AttribLabelType);
					if (same_ic(ltype, SpanID)) ftype = FeatureSpan;
					else                        ftype = FeatureText;

					/* Add features to the appropriate list */
					switch (ftype)
						{
						
						/* Add to list of labels */
						case FeatureText:

							npickT++;
							if (npickT > apickT)
								{
								apickT = npickT;
								lpickT = GETMEM(lpickT, LABEL, apickT);
								lcopyT = GETMEM(lcopyT, LABEL, apickT);
								}

							/* Produce a copy of the picked label */
							nn = npickT - 1;
							lpickT[nn] = label;
							(void) recall_label_value(label, NULL, NULL, &text);
							lcopyT[nn] = create_label("", "", text,
													  label->anchor, 0.0);
							lcopy      = lcopyT[nn];

							/* Highlight the selected label      */
							/*  (using same text font and size)  */
							recall_tspec_value(&label->tspec, TEXT_FONT,
									   		(POINTER)&font);
							recall_tspec_value(&label->tspec, TEXT_SIZE,
									   		(POINTER)&size);
							define_tspec(&lcopy->tspec, 0, font, False,
								 		size, 0.0, Hc, Vc, (HILITE) 2);

							add_item_to_metafile(TempMeta, "label",
										 		"d", "", "", (ITEM) lcopy);
							break;
						
						/* Check for missing labels in list of spans */
						case FeatureSpan:

							/* Check for label in list of spans */
							for (nn=0; nn<npickS; nn++)
								{
								if (label == lpickS[nn]) break;
								}
							if (nn < npickS) continue;

							/* Add missing labels to list of spans */
							pr_diag("Editor",
								"[edit_select_generic_feature] Adding unattached span label!\n");
							npickS++;
							if (npickS > apickS)
								{
								apickS = npickS;
								cpickS = GETMEM(cpickS, CURVE, apickS);
								ccopyS = GETMEM(ccopyS, CURVE, apickS);
								mpickS = GETMEM(mpickS, MARK,  apickS);
								mcopyS = GETMEM(mcopyS, MARK,  apickS);
								lpickS = GETMEM(lpickS, LABEL, apickS);
								lcopyS = GETMEM(lcopyS, LABEL, apickS);
								}
							nn = npickS - 1;
							cpickS[nn] = NullCurve;
							ccopyS[nn] = NullCurve;

							mpickS[nn] = NullMark;
							mcopyS[nn] = NullMark;

							lpickS[nn] = label;
							(void) recall_label_value(label, NULL, NULL, &text);
							lcopyS[nn] = create_label("", "", text,
													  label->anchor, 0.0);
							lcopy      = lcopyS[nn];

							/* Highlight the span label */
							/* (using same text size)   */
							recall_tspec_value(&label->tspec, TEXT_FONT,
											   (POINTER)&font);
							recall_tspec_value(&label->tspec, TEXT_SIZE,
											   (POINTER)&size);
							define_tspec(&lcopy->tspec, 0, font, False,
										 size, 0.0, Hc, Vc, (HILITE) 2);

							add_item_to_metafile(TempMeta, "label",
												 "d", "", "", (ITEM) lcopy);
							break;
						}
					}

				/* Check for missing marks in list of spans */
				for (ii=0; ii<EditMarks->num; ii++)
					{
					mark = (MARK) EditMarks->list[ii];
					if (IsNull(mark)) continue;

					/* Set feature type for each mark */
					mtype = CAL_get_attribute(mark->attrib, AttribLabelType);
					if (same_ic(mtype, SpanID)) ftype = FeatureSpan;
					else                        ftype = -1;

					/* Check features in the appropriate list */
					switch (ftype)
						{
						
						/* Check for missing marks in list of spans */
						case FeatureSpan:

							/* Check for mark in list of spans */
							for (nn=0; nn<npickS; nn++)
								{
								if (mark == mpickS[nn]) break;
								}
							if (nn < npickS) continue;

							/* Add missing marks to list of spans */
							pr_diag("Editor",
								"[edit_select_generic_feature] Adding unattached span mark!\n");
							npickS++;
							if (npickS > apickS)
								{
								apickS = npickS;
								cpickS = GETMEM(cpickS, CURVE, apickS);
								ccopyS = GETMEM(ccopyS, CURVE, apickS);
								mpickS = GETMEM(mpickS, MARK,  apickS);
								mcopyS = GETMEM(mcopyS, MARK,  apickS);
								lpickS = GETMEM(lpickS, LABEL, apickS);
								lcopyS = GETMEM(lcopyS, LABEL, apickS);
								}
							nn = npickS - 1;
							cpickS[nn] = NullCurve;
							ccopyS[nn] = NullCurve;

							mpickS[nn] = mark;
							mcopyS[nn] = create_mark("", "", "",
													 mark->anchor, 0.0);
							mcopy      = mcopyS[nn];

							lpickS[nn] = NullLabel;
							lcopyS[nn] = NullLabel;

							/* Highlight the span mark */
							define_mspec(&mcopy->mspec, 0, 1, NULL, 0,
										 False, 8.0, 0.0, (HILITE) 2);

							add_item_to_metafile(TempMeta, "mark",
												 "d", "", "", (ITEM) mcopy);
							break;
						}
					}

				/* Display the results */
				present_temp(TRUE);

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Delete the picked features */
			case Delete:

				/* Remove line features */
				for (nn=0; nn<npickL; nn++)
					{
					remove_item_from_set(EditCurves, (ITEM) cpickL[nn]);
					}

				/* Remove span features */
				for (nn=0; nn<npickS; nn++)
					{
					remove_item_from_set(EditCurves, (ITEM) cpickS[nn]);
					remove_item_from_set(EditMarks,  (ITEM) mpickS[nn]);
					remove_item_from_set(EditLabs,   (ITEM) lpickS[nn]);
					}

				/* Remove label features */
				for (nn=0; nn<npickT; nn++)
					{
					remove_item_from_set(EditLabs,   (ITEM) lpickT[nn]);
					}

				/* Empty pick list */
				empty_temp();
				npickL = 0;
				apickL = 0;
				npickS = 0;
				apickS = 0;
				npickT = 0;
				apickT = 0;
				FREEMEM(cpickL);
				FREEMEM(ccopyL);
				FREEMEM(cpickS);
				FREEMEM(ccopyS);
				FREEMEM(mpickS);
				FREEMEM(mcopyS);
				FREEMEM(lpickS);
				FREEMEM(lcopyS);
				FREEMEM(lpickT);
				FREEMEM(lcopyT);
				scratch_can_delete(FALSE);

				/* Show the results */
				present_all();

				/* Move on to next stage */
				State = Pick;
				return TRUE;
			}
		}
	}
