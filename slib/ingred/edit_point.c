/***********************************************************************
*                                                                      *
*     e d i t _ p o i n t . c                                          *
*                                                                      *
*     Assorted operations for point (spot) fields.                     *
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

/***********************************************************************
*                                                                      *
*     r e a d y _ p o i n t _ f i e l d                                *
*                                                                      *
***********************************************************************/

LOGICAL	edit_ready_point_field

	(
	STRING	mode
	)

	{
	int		butt;

	if (!EditUndoable)      edit_can_create(FALSE);
	if (!EditUndoable)      return TRUE;
	if (NotNull(OldPoints)) edit_can_create(FALSE);
	if (NotNull(OldPoints)) return TRUE;

	/* Check for create */
	if (same_ic(mode, "create"))
		{
		field_create(ActiveDfld, FALSE);
		edit_can_create(FALSE);
		return FALSE;
		}

	/* No field is present, and it cannot be created! */
	if (!ActiveDfld->doedit)
		{
		while (TRUE)
			{
			put_message("point-no-field");
			edit_can_create(FALSE);
			if (!ready_Xpoint(DnEdit, NULL, &butt)) return FALSE;
			(void) ignore_Xpoint();
			}
		}

	/* No field is present, but it can be created */
	while (TRUE)
		{
		put_message("point-no-fld-cr");
		edit_can_create(TRUE);
		if (!ready_Xpoint(DnEdit, NULL, &butt)) return FALSE;
		(void) ignore_Xpoint();
		}
	}

/***********************************************************************
*                                                                      *
*     d r a w _ p o i n t                                              *
*                                                                      *
***********************************************************************/

static	SPOT	NewSpot = NullSpot;

LOGICAL	edit_draw_point

	(
	STRING	mode,
	STRING	ltype,
	CAL		cal
	)

	{
	int		ii;
	POINT	pos;
	SPOT	spot;
	STRING	ctype, mclass, sub, val, lab;
	SPFEAT	attach;
	LOGICAL	entry;
	FpaConfigElementScatteredTypeStruct	*stypes;
	int		butt;
	LOGICAL	drawn = FALSE;

	/* Current state: */
	/* - where should we resume? */
	static  enum    { Add, Wait, Set } State = Add;

	static	SPOT	copy = NullSpot;
	static	char	CurrSub[40] = "";

#	ifdef DEBUG
	pr_diag("Editor", "[edit_draw_point] %s  State: %d\n", mode, State);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same_start(mode, "cancel"))
		{
		ignore_partial();
		if (EditPoints && EditUndoable)
			{
			highlight_set(EditPoints, 1);
			present_all();
			}
		(void) strcpy(CurrSub, "");
		State = Add;
		edit_select(NullCal, FALSE);
		return FALSE;
		}
	if (same(mode, "clear"))
		{
		State = Add;
		return FALSE;
		}

	/* Make sure the spot set contains the right stuff */
	if (!EditPoints)                     return FALSE;
	if (!same(EditPoints->type, "spot")) return FALSE;

	/* Make sure that the spot type is OK */
	if (NotNull(cal))
		{
		ctype = CAL_get_attribute(cal, CALscatteredtype);

#		ifdef DEBUG
		pr_diag("Editor",
			"[edit_draw_point] ltype: %s  ctype: %s\n", ltype, ctype);
#		endif /* DEBUG */

		if (!same(ltype, ctype)) return FALSE;
		}

	/* Set the spot type to highlight */
	CAL_get_defaults(cal, &sub, &val, &lab);

	/* Re-initialize for changing spot types */
	if (!same(sub, CurrSub))
		{
		(void) safe_strcpy(CurrSub, sub);
		if (EditUndoable)
			{
			highlight_set(EditPoints, 1);
			highlight_set_category(EditPoints, sub, 2);
			post_partial(FALSE);
			present_all();
			}
		}

	/* Initialize on startup */
	if (same(mode, "begin"))
		{
		ignore_partial();
		(void) safe_strcpy(CurrSub, sub);
		if (EditUndoable)
			{
			highlight_set(EditPoints, 1);
			highlight_set_category(EditPoints, sub, 2);
			present_all();
			post_partial(FALSE);
			}
		edit_select(NullCal, FALSE);
		State = Add;
		}

	/* Set class, attach option and entry file flag for this type of spot */
	ii = identify_scattered_type_by_name(CurrElement, CurrLevel,
			ltype, &stypes);
	if (NotNull(stypes) && ii >= 0)
		{
		lab    = stypes->type_labels[ii];
		mclass = stypes->type_classes[ii];
		(void) check_attach_option(FpaC_SCATTERED,
					stypes->type_attach_opts[ii], &attach);
		if (!blank(stypes->type_entry_files[ii])) entry = TRUE;
		else                                      entry = FALSE;
		}
	else
		{
		mclass = FpaCdefaultScatteredTypesClass;
		attach = AttachNone;
		entry  = FALSE;
		}

	/* Construct a prompt */
	if (blank(lab)) (void) sprintf(Msg, "%s: %s", sub, val);
	else            (void) strcpy(Msg, lab);

	/* Take care of setting new value after drawing */
	/* Only recognized when a spot has just been drawn */
	if (same(mode, "set") && NotNull(cal)) State = Set;

	/* Repeat until told to quit */
	while (TRUE)
		{

		/* Resume at current stage */
		switch (State)
			{

			/* Add a spot */
			case Add:

				/* Put up a prompt */
				if (!entry) put_message("point-draw");
				else        put_message("point-draw2", Msg);

				/* Get the point */
				if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return drawn;
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				drawn = FALSE;

				/* Freeze all pending edits so only this one can be undone */
				if (EditUndoable)
					{
					ignore_partial();
					put_message("edit-freeze");
					accept_mod();	/* Leaves EditPoints alone */
					}
				post_partial(TRUE);

				/* Place the spot */
				put_message("point-pick");
				(void) pick_Xpoint(DnEdit, 0, pos, &butt);
				if (!inside_dn_window(DnEdit, pos))
					{
					put_message("edit-outside-map");
					(void) sleep(1);
					continue;
					}

				/* Transfer the new data to the spot set */
				spot = create_spot(pos, mclass, attach, cal);
				if (IsNull(spot))
					{
					put_message("edit-no-edit");
					(void) sleep(1);
					present_all();
					ignore_partial();
					continue;
					}
				add_item_to_set(EditPoints, (ITEM) spot);

				/* Set up display specs for the new spot */
				NewSpot = spot;
				invoke_set_catspecs(EditPoints);
				if (EditUndoable)
					{
					highlight_set(EditPoints, 1);
					highlight_set_category(EditPoints, sub, 2);
					}

				/* Make a copy of the new spot to work with */
				if (NotNull(copy)) copy = destroy_spot(copy);
				copy = copy_spot(NewSpot);
				CAL_add_location((CAL) copy->attrib, MapProj, copy->anchor);

				/* Register the edit */
				drawn = TRUE;
				if (EditUndoable) post_mod("points");

				/* Modify labels if necessary */
				if (recompute_spotset_labs(EditPoints, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				present_all();

				/* Move on to next stage */
				State = (entry)? Wait: Add;
				continue;

			/* Wait around for a "set" */
			case Wait:

				/* Keep returning until we get a "set" */
				edit_select((CAL) copy->attrib, TRUE);
				return FALSE;

			/* Set the value for the spot just added */
			case Set:

				if (!edit_posted())
					{
					NewSpot = NullSpot;
					State = Add;
					continue;
					}

#				ifdef DEBUG
				pr_diag("Editor",
					"[edit_draw_point] Setting value (%s)\n", Msg);
#				endif /* DEBUG */

				/* Set a new value */
				busy_cursor(TRUE);
				put_message("point-value");
				define_item_attribs("spot", (ITEM) NewSpot, cal);

				/* Set up display specs for the new spot field */
				invoke_set_catspecs(EditPoints);
				if (EditUndoable)
					{
					highlight_set(EditPoints, 1);
					highlight_set_category(EditPoints, sub, 2);
					present_all();
					}

				/* Register the edit */
				drawn = TRUE;
				if (EditUndoable) post_mod("points");

				/* Modify labels if necessary */
				if (recompute_spotset_labs(EditPoints, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				edit_select(NullCal, FALSE);
				present_all();
				busy_cursor(FALSE);

				/* Move on to next stage */
				State = Add;
				continue;
			}
		}
	}

/***********************************************************************
*                                                                      *
*     m o v e _ p o i n t                                              *
*                                                                      *
***********************************************************************/

static	LOGICAL	spot_inside_outline(SPOT, CURVE);

/**********************************************************************/

LOGICAL	edit_move_point

	(
	STRING	mode,
	STRING	name
	)

	{
	SPOT	spot, espot, copy;
	SET		set;
	CURVE	outline;
	int		nlines;
	LINE	*lines;
	int		im, iep, dformat;
	POINT	p0, p1;
	MARK	mark0, mark1;
	CURVE	curv0;
	float	dx, dy;
	int		butt;
	LOGICAL	drawn = FALSE;

	/* Current state: */
	/* - where should we resume? */
	static  enum    { Pick, PickAll, Draw, DrawPick, Cut, Copy, Paste,
						Translate, TranslateDone, ReDisplay } State = Pick;

	/* Was a paste just done */
	static	LOGICAL		pasting = FALSE;

	/* Move list: */
	/* - list of spots to be moved */
	static	const int	maxmove = 100000;
	static	int			nmove   = 0;
	static	int			amove   = 0;
	static	SPOT		*smove  = NullSpotList;
	static	SPOT		*scopy  = NullSpotList;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_move_point] %s  State: %d\n", mode, State);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same(mode, "cancel"))
		{
		put_message("edit-cancel");
		switch (State)
			{
			case Draw:
			case Translate:	ignore_partial();
							edit_allow_preset_outline(FALSE);
							pasting = FALSE;
							State = ReDisplay;
							break;

			default:		ignore_partial();
							edit_allow_preset_outline(FALSE);

							/* Empty move list */
							if (nmove > 0)
								{
								nmove = 0;
								amove = 0;
								FREEMEM(smove);
								FREEMEM(scopy);
								}

							/* Empty display structure */
							empty_temp();
							pasting = FALSE;
							clear_message();

							edit_select(NullCal, FALSE);
							edit_can_copy(FALSE);
							if (copy_posted("points", CurrElement, CurrLevel))
								edit_can_paste(TRUE);
							State = Pick;
							break;
			}
		}
	if (same(mode, "cancel all"))
		{
		ignore_partial();
		edit_allow_preset_outline(FALSE);

		/* Empty move list */
		if (nmove > 0)
			{
			nmove = 0;
			amove = 0;
			FREEMEM(smove);
			FREEMEM(scopy);
			}

		/* Empty display structure */
		empty_temp();
		pasting = FALSE;
		clear_message();

		edit_select(NullCal, FALSE);
		edit_can_copy(FALSE);
		if (copy_posted("points", CurrElement, CurrLevel))
			edit_can_paste(TRUE);
		return FALSE;
		}
	if (same(mode, "clear"))
		{
		/* >>>>> clear the paste buffer??? <<<<< */
		State = Pick;
		return FALSE;
		}

	/* Make sure the spot set contains the right stuff */
	if (!EditPoints)                     return FALSE;
	if (!same(EditPoints->type, "spot")) return FALSE;

	/* Construct temporary fields and buffers on startup only */
	if (same(mode, "begin"))
		{
		ignore_partial();

		/* Set up move list */
		nmove = 0;
		amove = 0;
		FREEMEM(smove);
		FREEMEM(scopy);

		/* Empty display structure */
		empty_temp();
		pasting = FALSE;

		edit_select(NullCal, FALSE);
		edit_can_copy(FALSE);
		if (copy_posted("points", CurrElement, CurrLevel))
			edit_can_paste(TRUE);
		edit_allow_preset_outline(FALSE);

		State = Pick;
		}

	/* Set state for "select all" */
	if (same(mode, "select all"))
		{

		/* Select All cancels translate */
		switch (State)
			{
			case Translate: put_message("edit-cancel");
			case Pick:		State = PickAll;
							break;
			}
		}

	/* Set state for "delete" or "cut" or "copy" or "paste" */
	if ( (same(mode, "delete") || same(mode, "cut") || same(mode, "copy"))
			&& nmove <= 0 )
		{
		edit_can_copy(FALSE);
		put_message("point-no-pick");
		(void) sleep(1);
		edit_select(NullCal, FALSE);
		State = Pick;
		return FALSE;
		}
	if (same(mode, "delete")) State = Cut;
	if (same(mode, "cut"))    State = Cut;
	if (same(mode, "copy"))   State = Copy;
	if (same(mode, "paste"))  State = Paste;

	/* Set state for Right button "translate" */
	if ( same(mode, "translate") && nmove <= 0 )
		{
		edit_can_copy(FALSE);
		put_message("point-no-pick");
		(void) sleep(1);
		edit_select(NullCal, FALSE);
		State = Pick;
		return FALSE;
		}
	if (same(mode, "translate")) State = Translate;

	/* Set state for Right button "draw outline" */
	if (same(mode, "draw outline"))	State = Draw;

	/* Set state for Right button "preset outline" */
	if (same(mode, "preset outline"))
		{

		/* Check for last drawn outline or named outline */
		if (same(name, FpaEditorLastDrawnBoundary))
			outline = retrieve_drawn_outline();
		else
			outline = retrieve_named_outline(name);
		if (IsNull(outline))
			{
			if (same(name, FpaEditorLastDrawnBoundary))
				{
				put_message("edit-no-drawn-outline");
				pr_warning("Editor",
					"[edit_move_point] No outline drawn yet!\n");
				}
			else
				{
				put_message("edit-no-preset-outline", name);
				pr_error("Editor",
					"[edit_move_point] No preset map outline: \"%s\"\n", name);
				}
			(void) sleep(1);
			State = Pick;
			}

		/* Display the outline */
		else
			{
			define_lspec(&outline->lspec, 255, 0, NULL, False, 2.0, 0.0,
						 (HILITE) 2);
			add_item_to_metafile(TempMeta, "curve", "c",
								 "", "", (ITEM) outline);
			present_temp(TRUE);
			State = DrawPick;
			}
		}

	/* Repeat until told to quit */
	while (TRUE)
		{

		/* Resume at current stage */
		switch (State)
			{

			/* Pick spots to move */
			case Pick:

				/* See if there are any spots to move */
				if (EditPoints->num <= 0)
					{
					if (copy_posted("points", CurrElement, CurrLevel))
						edit_can_paste(TRUE);
					put_message("point-no-move");
					edit_select(NullCal, FALSE);
					return drawn;
					}

				/* Allow specification of multiple moves */
				while (nmove < maxmove)
					{
					/* Put up a prompt and set states */
					if (nmove <= 0)
						{
						put_message("point-move-pick");
						edit_can_copy(FALSE);
						if (copy_posted("points", CurrElement, CurrLevel))
							edit_can_paste(TRUE);
						edit_select(NullCal, FALSE);
						ignore_partial();
						}
					else
						{
						put_message("point-move-pick2");
						edit_can_copy(TRUE);
						edit_select((CAL) smove[0]->attrib, FALSE);
						post_partial(TRUE);
						}
					edit_allow_preset_outline(TRUE);

					/* Get the point entered */
					if (!ready_Xpoint(DnEdit, p0, &butt)) return drawn;
					if (butt != LeftButton)
						{
						put_message("edit-wrong-button");
						(void) ignore_Xpoint();
						continue;
						}
					put_message("point-pick");
					if (!pick_Xpoint(DnEdit, 0, p1, &butt)) continue;
					if (!inside_dn_window(DnEdit, p1))
						{
						put_message("edit-outside-map");
						(void) sleep(1);
						continue;
						}

					/* See which spot we picked */
					spot = closest_spot(EditPoints, p1, NULL, NULL, NULL, NULL);
					if (!spot)
						{
						put_message("point-no-pick");
						(void) sleep(1);
						continue;
						}

					/* If spot was already picked, un-pick it */
					for (im=0; im<nmove; im++)
						{
						if (spot == smove[im]) break;
						}
					if (im < nmove)
						{
						put_message("point-unpicked");

						set = find_mf_set(TempMeta, "spot", "d", "", "");
						scopy[im] = remove_item_from_set(set, (ITEM) scopy[im]);
						smove[im] = NullSpot;
						nmove--;
						for ( ; im<nmove; im++)
							{
							smove[im] = smove[im+1];
							scopy[im] = scopy[im+1];
							}
						smove[im] = NullSpot;
						scopy[im] = NullSpot;

						/* Display remaining picked spots */
						present_all();
						pasting = FALSE;
						}

					/* Otherwise pick it */
					else
						{
						put_message("point-picked");
						nmove++;
						if (nmove > amove)
							{
							amove = nmove;
							smove = GETMEM(smove, SPOT, amove);
							scopy = GETMEM(scopy, SPOT, amove);
							}

						/* Produce a copy of the picked spot */
						smove[im] = spot;
						scopy[im] = copy_spot(spot);
						copy      = scopy[im];

						/* Highlight the picked spot */
						highlight_item("spot", (ITEM) copy, 2);
						add_item_to_metafile(TempMeta, "spot", "d", "", "",
											 (ITEM) copy);

						/* Display picked spots */
						present_temp(TRUE);
						pasting = FALSE;
						}
					}

				/* Have we picked any spots? */
				if (nmove > 0) edit_select((CAL) smove[0]->attrib, FALSE);
				else           edit_select(NullCal, FALSE);

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Pick all spots to move */
			case PickAll:

				/* Empty move list */
				if (nmove > 0)
					{
					nmove = 0;
					amove = 0;
					FREEMEM(smove);
					FREEMEM(scopy);
					}

				/* Empty display structure */
				empty_temp();

				put_message("point-all-picked");
				nmove = EditPoints->num;
				if (nmove > amove)
					{
					amove = nmove;
					smove = GETMEM(smove, SPOT, amove);
					scopy = GETMEM(scopy, SPOT, amove);
					}

				/* Highlight all the spots and add them to the list */
				for (im=0; im<nmove; im++)
					{
					smove[im] = (SPOT) EditPoints->list[im];
					scopy[im] = copy_spot(smove[im]);
					copy      = scopy[im];
					highlight_item("spot", (ITEM) copy, 2);
					add_item_to_metafile(TempMeta, "spot", "d", "", "",
										 (ITEM) copy);
					}

				/* Display picked spots */
				present_temp(TRUE);
				pasting = FALSE;

				/* Have we picked any spots? */
				if (nmove > 0) edit_select((CAL) smove[0]->attrib, FALSE);
				else           edit_select(NullCal, FALSE);

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Draw outline around spots to move */
			case Draw:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_move_point] Drawing outline\n");
#				endif /* DEBUG */

				/* Draw the outline */
				post_partial(FALSE);
				edit_allow_preset_outline(FALSE);
				put_message("point-move-draw");
				if (ready_Xcurve()) butt = LeftButton;
				else if (!ready_Xpoint(DnEdit, NULL, &butt)) return drawn;

				/* Invalid button */
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					ignore_Xpoint();
					continue;
					}

				/* Draw the outline */
				drawing_control(TRUE);
				if (!ready_Xcurve())
					{
					if (drawing_Xcurve()) return drawn;
					(void) draw_Xcurve(DnEdit, FilterRes*4, SplineRes*4, TRUE);
					if (!ready_Xcurve()) return drawn;
					}
				drawing_control(FALSE);

				/* Dump the outline */
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

				/* Extract and display outline */
				outline = create_curve("", "", "");
				add_line_to_curve(outline, lines[0]);
				post_drawn_outline(outline);
				reset_pipe();
				define_lspec(&outline->lspec, 255, 0, NULL, False, 2.0, 0.0,
							 (HILITE) 2);
				add_item_to_metafile(TempMeta, "curve", "c", "", "",
									 (ITEM) outline);
				present_temp(TRUE);

				/* Move on to next stage */
				State = DrawPick;
				continue;

			/* Pick spots to move from inside drawn outline */
			case DrawPick:

				if (IsNull(outline))
					{
					edit_select(NullCal, FALSE);
					State = Pick;
					continue;
					}

#				ifdef DEBUG
				pr_diag("Editor",
					"[edit_move_point] Picking spots inside outline\n");
#				endif /* DEBUG */

				busy_cursor(TRUE);
				edit_allow_preset_outline(FALSE);
				put_message("point-move-select");
				(void) sleep(1);

				/* Pick spots inside drawn outline */
				for (iep=0; iep<EditPoints->num; iep++)
					{
					espot = (SPOT) EditPoints->list[iep];
					if (!espot) continue;

					/* Check if spot is inside drawn outline */
					if (!spot_inside_outline(espot, outline)) continue;

					/* Pick spots that are not already picked */
					for (im=0; im<nmove; im++)
						{
						if (espot == smove[im]) break;
						}
					if (im >= nmove)
						{
						put_message("point-picked");
						nmove++;
						if (nmove > amove)
							{
							amove = nmove;
							smove = GETMEM(smove, SPOT, amove);
							scopy = GETMEM(scopy, SPOT, amove);
							}

						/* Produce a copy of the picked spot */
						smove[im] = espot;
						scopy[im] = copy_spot(espot);
						copy      = scopy[im];

						/* Highlight the picked spot */
						highlight_item("spot", (ITEM) copy, 2);
						add_item_to_metafile(TempMeta, "spot", "d", "", "",
											 (ITEM) copy);
						}
					}
				present_temp(TRUE);
				busy_cursor(FALSE);

				/* Have we picked any spots? */
				if (nmove > 0) edit_select((CAL) smove[0]->attrib, FALSE);
				else           edit_select(NullCal, FALSE);

				/* Re-display the picked spots */
				State = ReDisplay;
				continue;

			/* Cut spots */
			case Cut:

				/* Save the spots in the copy buffer */
				put_message("edit-cut");
				post_point_copy(CurrElement, CurrLevel, nmove, smove);

				/* Remove spots from spot set */
				for (im=0; im<nmove; im++)
					{
					spot = smove[im];
					(void) remove_item_from_set(EditPoints, (ITEM) spot);
					smove[im] = NullSpot;
					}

				if (EditUndoable) post_mod("points");

				/* Modify labels if necessary */
				if (recompute_spotset_labs(EditPoints, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				/* Empty move list */
				if (nmove > 0)
					{
					nmove = 0;
					amove = 0;
					FREEMEM(smove);
					FREEMEM(scopy);
					}

				/* Empty display structure */
				empty_temp();
				pasting = FALSE;
				edit_select(NullCal, FALSE);
				clear_message();

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Copy spots */
			case Copy:

				/* Save the spots in the copy buffer */
				put_message("edit-copy");
				post_point_copy(CurrElement, CurrLevel, nmove, smove);
				clear_message();
				pasting = FALSE;

				/* Re-display the picked spots */
				State = ReDisplay;
				continue;

			/* Paste spots */
			case Paste:

				put_message("edit-paste");

				/* Empty move list */
				if (nmove > 0)
					{
					ignore_partial();
					edit_can_copy(FALSE);
					nmove = 0;
					amove = 0;
					FREEMEM(smove);
					FREEMEM(scopy);

					/* Empty display structure */
					empty_temp();
					}

				/* Retrieve paste buffer */
				paste_point_copy(CurrElement, CurrLevel, &nmove, &smove);
				if (nmove <= 0)
					{
					edit_can_paste(FALSE);
					put_message("edit-no-paste");
					(void) sleep(1);
					edit_select(NullCal, FALSE);
					State = Pick;
					return FALSE;
					}

				/* Transfer paste buffer to move list */
				if (nmove > amove)
					{
					amove = nmove;
					scopy = GETMEM(scopy, SPOT, amove);
				for (im=0; im<nmove; im++)
					{

					/* Check for matching spots */
					spot = smove[im];
					for (iep=0; iep<EditPoints->num; iep++)
						{
						espot = (SPOT) EditPoints->list[iep];
						if (!espot) continue;

						/* Offset spots pasted over themselves */
						if (spot->anchor[X] == espot->anchor[X] &&
							spot->anchor[Y] == espot->anchor[Y])
							{
							spot->anchor[X] += PickTol;
							spot->anchor[Y] += PickTol;
							break;
							}
						}

					/* Add pasted spot to spot set */
					(void) add_item_to_set(EditPoints, (ITEM) spot);
					scopy[im] = copy_spot(spot);
					copy      = scopy[im];

					/* Highlight the picked spot */
					highlight_item("spot", (ITEM) copy, 2);
					add_item_to_metafile(TempMeta, "spot", "d", "", "",
										 (ITEM) copy);
					}

				/* Re-display the spots */
				present_temp(TRUE);
				pasting = TRUE;

				if (EditUndoable) post_mod("points");

				/* Modify labels if necessary */
				if (recompute_spotset_labs(EditPoints, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				/* Move on to next stage */
				edit_select((CAL) smove[0]->attrib, FALSE);
				post_partial(TRUE);
				clear_message();
				State = Pick;
				continue;
				}

			/* Pick and translate a reference point */
			case Translate:

				/* Highlight the picked spots */
				for (im=0; im<nmove; im++)
					{
					copy = scopy[im];
					highlight_item("spot", (ITEM) copy, 3);
					}
				present_temp(TRUE);

				post_partial(TRUE);
				edit_can_copy(TRUE);

				/* Pick a reference point */
				put_message("point-tran");
				if (!ready_Xpoint(DnEdit, p0, &butt)) return drawn;
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!inside_dn_window(DnEdit, p0))
					{
					put_message("point-tran-out1");
					(void) ignore_Xpoint();
					continue;
					}

				/* Highlight the spots to translate */
				set = copy_mf_set(TempMeta, "spot", "d", "", "");
				for (im=0; im<set->num; im++)
					{
					spot = (SPOT) set->list[im];
					highlight_item("spot", (ITEM) spot, 2);
					}
				dformat = field_display_format(CurrElement, CurrLevel);
				prep_set(set, dformat);

				/* Translate the reference point */
				put_message("point-tran-release");
				(void) utrack_Xpoint(DnEdit, set, p0, p1, &butt);
				set = destroy_set(set);
				if (!inside_dn_window(DnEdit, p1))
					{
					put_message("point-tran-out2");
					(void) sleep(1);
					continue;
					}

				/* Display markers */
				mark0 = create_mark("", "", "", p0, 0.0);
				mark1 = create_mark("", "", "", p1, 0.0);
				curv0 = create_curve("", "", "");
				add_point_to_curve(curv0, p0);
				add_point_to_curve(curv0, p1);
				define_mspec(&mark0->mspec, 0, 2, NULL, 0, False, 2.0, 0.0,
							 (HILITE) 2);
				define_mspec(&mark1->mspec, 0, 3, NULL, 0, False, 2.0, 0.0,
							 (HILITE) 2);
				define_lspec(&curv0->lspec, 0, 0, NULL, False, 2.0, 0.0,
							 (HILITE) 2);

				add_item_to_metafile(TempMeta, "mark",  "d", "", "",
									 (ITEM) mark0);
				add_item_to_metafile(TempMeta, "mark",  "d", "", "",
									 (ITEM) mark1);
				add_item_to_metafile(TempMeta, "curve", "c", "", "",
									 (ITEM) curv0);
				present_temp(TRUE);

				/* Move on to next stage */
				State = TranslateDone;
				continue;

			/* Translate the spots */
			case TranslateDone:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_move_point] Translating\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);

				/* Translate the spots */
				dx = p1[X] - p0[X];
				dy = p1[Y] - p0[Y];
				for (im=0; im<nmove; im++)
					{
					spot = smove[im];
					spot->anchor[X] += dx;
					spot->anchor[Y] += dy;
					scopy[im] = NullSpot;
					}

				if (EditUndoable) post_mod("points");

				/* Modify labels if necessary */
				if (recompute_spotset_labs(EditPoints, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				put_message("edit-done");
				pasting = FALSE;
				busy_cursor(FALSE);

				/* Re-display the picked spots */
				State = ReDisplay;
				continue;

			/* Re-display the picked spots */
			case ReDisplay:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_move_point] Redisplay\n");
#				endif /* DEBUG */

				/* Empty display structure */
				empty_temp();

				/* Re-pick the picked spots */
				for (im=0; im<nmove; im++)
					{
					scopy[im] = copy_spot(smove[im]);
					copy      = scopy[im];

					/* Highlight the picked spot */
					highlight_item("spot", (ITEM) copy, 2);
					add_item_to_metafile(TempMeta, "spot", "d", "", "",
										 (ITEM) copy);
					}

				/* Re-display the spots */
				present_all();
				post_partial(FALSE);

				/* Move on to next stage */
				State = Pick;
				continue;
			}
		}
	}

/**********************************************************************/

static	LOGICAL	spot_inside_outline

	(
	SPOT	spot,
	CURVE	outline
	)

	{
	LOGICAL	inside;
	LINE	oline;

	if (!spot)                       return FALSE;
	if (!outline)                    return FALSE;
	if (!outline->line)              return FALSE;
	if (!line_closed(outline->line)) return FALSE;

	/* Ensure that spot location is inside outline */
	oline = outline->line;
	line_test_point(oline, spot->anchor, NullFloat, NullPoint, NullInt,
					&inside, NullChar);
	if (!inside) return FALSE;

	/* Return TRUE if spot location was inside outline */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     m o d i f y _ p o i n t                                          *
*     m o d i f y _ p o i n t _ p i c k                                *
*                                                                      *
***********************************************************************/

typedef	enum
			{
			ModPick, ModPicked,	/* interactive states */
			ModRePick,			/* "cancel" states */
			ModSet, ModDelete,	/* modification states */
			ModConfirm			/* complete modification */
			}
		MODSTATE;

static	MODSTATE	modify_point_pick(STRING, CAL);

static	LOGICAL	Mfresh = FALSE;		/* Just picked */
static	SPOT	Mpick  = NullSpot;	/* Picked spot */
static	SPOT	Mwork  = NullSpot;	/* Working copy of picked spot */

/**********************************************************************/

LOGICAL	edit_modify_point

	(
	STRING	mode,
	STRING	ltype,
	CAL		cal
	)

	{
	MODSTATE	next;
	LOGICAL		drawn = FALSE;

	/* Current state: */
	/* - where should we resume? */
	static	MODSTATE	State = ModPick;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_modify_point] %s  State: %d\n", mode, State);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same(mode, "cancel"))
		{
		switch (State)
			{
			case ModPicked:		State = ModRePick;
								break;

			default:			empty_temp();
								edit_select(NullCal, FALSE);
								present_all();
								return FALSE;
			}
		}
	if (same(mode, "cancel all"))
		{
		ignore_partial();

		/* Clean up temporary buffers */
		edit_select(NullCal, FALSE);
		Mfresh = FALSE;
		Mpick  = NullSpot;
		Mwork  = NullSpot;

		/* Empty display structure */
		empty_temp();
		return FALSE;
		}
	if (same(mode, "clear"))
		{
		return FALSE;
		}

	/* Make sure the spot set contains the right stuff */
	if (!EditPoints)                     return FALSE;
	if (!same(EditPoints->type, "spot")) return FALSE;

	/* Initialize on startup */
	if (same(mode, "begin"))
		{
		ignore_partial();
		Mfresh = FALSE;
		edit_select(NullCal, FALSE);
		State = ModPick;
		}

	/* Take care of deleting or setting new value */
	/* Only recognized when something is picked */
	if (same(mode, "set"))
		{
		if (State == ModPicked) State = ModSet;
		}
	if (same(mode, "delete"))
		{
		if (State == ModPicked) State = ModDelete;
		}

	/* Repeat until told to quit */
	while (TRUE)
		{

#		ifdef DEBUG
		pr_diag("Editor", "[edit_modify_point] State: %d\n", State);
#		endif /* DEBUG */

		/* Resume at current stage */
		switch (State)
			{

			/* Pick a spot to modify */
			case ModPick:

				/* Pick a spot */
				ignore_partial();
				/* >>>>> may not need to pass cal any longer <<<<< */
				next = modify_point_pick(ltype, cal);
				if (next == ModPick) return drawn;

				/* Move on to next stage */
				State = next;
				continue;

			/* Repick the spot to modify */
			case ModRePick:

				put_message("edit-cancel");

				/* Clean up temporary buffers */
				ignore_partial();
				edit_select(NullCal, FALSE);
				Mfresh = FALSE;
				Mpick  = NullSpot;
				Mwork  = NullSpot;

				/* Empty display structure */
				empty_temp();

				/* Move on to next stage */
				State = ModPick;
				/* >>>>> do we need to return FALSE??? <<<<< */
				continue;

			/* Wait for delete or set mode */
			case ModPicked:

				/* Freeze all pending edits so only this one can be undone */
				/* (first time after picking) */
				if (Mfresh)
					{
					Mfresh = FALSE;
					if (EditUndoable && edit_posted())
						{
						ignore_partial();
						put_message("edit-freeze");
						accept_mod();   /* Leaves EditPoints alone */
						drawn = FALSE;
						}
					}
				post_partial(TRUE);
				put_message("point-modify");

				return drawn;

			/* Delete the picked spot */
			case ModDelete:

				/* Freeze all pending edits so only this one can be undone */
				if (EditUndoable)
					{
					ignore_partial();
					put_message("edit-freeze");
					accept_mod();	/* Leaves EditPoints alone */
					}
				post_partial(TRUE);

#				ifdef DEBUG
				pr_diag("Editor", "[edit_modify_point] Deleting\n");
#				endif /* DEBUG */

				/* Remove the spot */
				busy_cursor(TRUE);
				put_message("point-deleting");
				remove_item_from_set(EditPoints, (ITEM) Mpick);

				/* Show the results */
				present_all();

				/* Move on to next stage */
				State = ModConfirm;
				continue;

			/* Set the value of the picked spot */
			case ModSet:

				/* Freeze all pending edits so only this one can be undone */
				if (EditUndoable)
					{
					ignore_partial();
					put_message("edit-freeze");
					accept_mod();	/* Leaves EditPoints alone */
					}
				post_partial(TRUE);

#				ifdef DEBUG
				pr_diag("Editor", "[edit_modify_point] Setting value (%s)\n",
						CAL_get_attribute(cal, CALuserlabel));
#				endif /* DEBUG */

				/* Set a new value */
				busy_cursor(TRUE);
				put_message("point-value");
				define_spot_attribs(Mpick, cal);

				/* Move on to next stage */
				State = ModConfirm;
				continue;

			/* Confirm current edit */
			case ModConfirm:

				/* Register the edit */
				ignore_partial();
				drawn = TRUE;
				invoke_set_catspecs(EditPoints);
				if (EditUndoable) highlight_set(EditPoints, 1);
				if (EditUndoable) post_mod("points");

				/* Modify labels if necessary */
				if (recompute_spotset_labs(EditPoints, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				/* Clean up temporary buffers */
				edit_select(NullCal, FALSE);
				Mfresh = FALSE;
				Mpick  = NullSpot;
				Mwork  = NullSpot;

				/* Empty display structure */
				empty_temp();
				busy_cursor(FALSE);

				/* Go back to pick stage */
				State = ModPick;
				continue;
			}
		}
	}

/**********************************************************************/

static	MODSTATE	modify_point_pick

	(
	STRING	ltype,
	CAL		cal
	)

	{
	int		butt, ii;
	POINT	pos;
	STRING	mtype;
	FpaConfigElementScatteredTypeStruct	*stypes;

	static	SPOT	copy = NullSpot;

	/* See if there are any spots to modify */
	if (EditPoints->num <= 0)
		{
		put_message("point-no-mod");
		return ModPick;
		}

	/* Keep trying until something gets picked */
	while (TRUE)
		{
		/* Get a point */
		put_message("point-mod-pick");
		if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return ModPick;
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}
		put_message("point-pick");
		if (!pick_Xpoint(DnEdit, 0, pos, &butt)) continue;
		if (!inside_dn_window(DnEdit, pos))
			{
			put_message("edit-outside-map");
			(void) sleep(1);
			continue;
			}

		/* See which spot we picked */
		Mpick = closest_spot(EditPoints, pos, NULL, NULL, NULL, NULL);
		if (!Mpick)
			{
			put_message("point-no-pick");
			(void) sleep(1);
			continue;
			}
		Mfresh = TRUE;

		/* Is this the correct type of point */
		mtype = CAL_get_attribute((CAL) Mpick->attrib, CALscatteredtype);

#		ifdef DEBUG
		pr_diag("Editor",
			"[edit_modify_point] ltype/mtype: %s/%s/%s\n", ltype, mtype);
#		endif /* DEBUG */

		if (!same(ltype, mtype))
			{
			put_message("point-mod-wrong");
			(void) sleep(1);
			continue;
			}

		/* Does this type of point have a modify file */
		ii = identify_scattered_type_by_name(CurrElement, CurrLevel,
				ltype, &stypes);
		if (IsNull(stypes) || ii < 0 || blank(stypes->type_modify_files[ii]))
			{
			put_message("point-mod-nofile");
			(void) sleep(1);
			continue;
			}

		/* Produce a copy of the picked spot to work on */
		put_message("point-picked");
		if (NotNull(copy)) copy = destroy_spot(copy);
		copy = copy_spot(Mpick);
		CAL_add_location((CAL) copy->attrib, MapProj, copy->anchor);
		edit_select((CAL) copy->attrib, TRUE);

		/* Highlight the picked spot */
		Mwork = copy_spot(Mpick);
		highlight_item("spot", (ITEM) Mwork, 2);
		add_item_to_metafile(TempMeta, "spot", "d", "", "", (ITEM) Mwork);
		present_temp(TRUE);

		return ModPicked;
		}
	}

/***********************************************************************
*                                                                      *
*     m e r g e _ p o i n t                                            *
*                                                                      *
***********************************************************************/

LOGICAL	edit_merge_point

	(
	STRING	mode,
	STRING	source,
	STRING	subsrc,
	STRING	rtime,
	STRING	vtime,
	STRING	element,
	STRING	level
	)

	{
	SPOT	spot, copy;
	SET		set;
	CURVE	outline;
	int		nlines;
	LINE	*lines;
	int		im, imx;
	POINT	p0, p1;
	MARK	mark0, mark1;
	CURVE	curv0;
	float	dx, dy;
	int		butt;
	LOGICAL	drawn   = FALSE;

	STRING			dpath;
	FLD_DESCRIPT	fdesc;

	/* Current state: */
	/* - where should we resume? */
	static	enum	{ Wait, Fetch, ReFetch,
						Pick, PickAll, Draw, DrawPick, RePick,
						WaitAction, Merge, Translate, TranslateDone,
						ReDisplay, ReDisplayAction, ReStart } State = Wait;

	/* Is a fetch being done? */
	static	LOGICAL		fetch     = FALSE;

	/* Has a merge or translate just been completed? */
	static	LOGICAL		maction   = FALSE;

	/* Move list: */
	/* - list of spots to be moved */
	static	const int	maxmove = 100000;
	static	int			nmove   = 0;
	static	int			amove   = 0;
	static	SPOT		*smove  = NullSpotList;
	static	SPOT		*scopy  = NullSpotList;

	/* Field to merge from */
	static	STRING	msource  = NullString;
	static	STRING	msubsrc  = NullString;
	static	STRING	mrtime   = NullString;
	static	STRING	mvtime   = NullString;
	static	STRING	melement = NullString;
	static	STRING	mlevel   = NullString;
	static	int		mformat  = DisplayFormatSimple;
	static	SET		mset     = NullSet;
	static	LOGICAL	fullset  = FALSE;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_merge_point] %s  State: %d\n", mode, State);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same(mode, "cancel"))
		{
		put_message("edit-cancel");
		drawing_control(FALSE);
		switch (State)
			{
			case Translate:	ignore_partial();
							edit_allow_preset_outline(FALSE);
							State = (maction)? ReDisplayAction: ReStart;
							break;

			case Draw:		ignore_partial();
							edit_allow_preset_outline(FALSE);
							State = ReDisplay;
							break;

			case Pick:		State = (nmove > 0)? RePick: ReFetch;
							break;

			default:		ignore_partial();
							edit_allow_preset_outline(FALSE);

							/* Empty picked spot list */
							if (nmove > 0)
								{
								if (!maction)
									{
									for (im=0; im<nmove; im++)
										(void) destroy_spot(scopy[im]);
									}
								nmove = 0;
								amove = 0;
								FREEMEM(smove);
								FREEMEM(scopy);
								}
							edit_select(NullCal, FALSE);

							/* Get rid of picked spots and fetched field */
							if (fullset)
								mset = take_mf_set(TempMeta, "spot", "d",
												   "mset", "mset");
							if (NotNull(mset))
								{
								mset = destroy_set(mset);
								FREEMEM(msource);
								FREEMEM(msubsrc);
								FREEMEM(mrtime);
								FREEMEM(mvtime);
								FREEMEM(melement);
								FREEMEM(mlevel);
								mformat = DisplayFormatSimple;
								}

							/* Empty display structure */
							empty_temp();
							present_all();
							return FALSE;
			}
		}
	if (same(mode, "cancel all"))
		{
		ignore_partial();
		drawing_control(FALSE);
		edit_allow_preset_outline(FALSE);

		/* Get rid of displayed picked spots and fetched field */
		if (fullset)
			mset = take_mf_set(TempMeta, "spot", "d", "mset", "mset");
		if (NotNull(mset))
			{
			if (nmove > 0)
				{
				for (im=0; im<nmove; im++) (void) destroy_spot(scopy[im]);
				nmove = 0;
				amove = 0;
				FREEMEM(smove);
				FREEMEM(scopy);
				}
			mset = destroy_set(mset);
			FREEMEM(msource);
			FREEMEM(msubsrc);
			FREEMEM(mrtime);
			FREEMEM(mvtime);
			FREEMEM(melement);
			FREEMEM(mlevel);
			mformat = DisplayFormatSimple;
			}

		/* Empty picked spot list */
		else if (nmove > 0)
			{
			if (!maction)
				{
				for (im=0; im<nmove; im++)
					scopy[im] = remove_item_from_set(EditPoints,
													 (ITEM) scopy[im]);
				}
			nmove = 0;
			amove = 0;
			FREEMEM(smove);
			FREEMEM(scopy);
			}

		/* Empty display structure */
		empty_temp();
		present_all();
		edit_select(NullCal, FALSE);
		return FALSE;
		}
	if (same(mode, "clear"))
		{
		return FALSE;
		}

	/* Make sure the spot set contains the right stuff */
	if (!EditPoints)                     return FALSE;
	if (!same(EditPoints->type, "spot")) return FALSE;

	/* Initialize on startup */
	if (same(mode, "begin"))
		{
		ignore_partial();

		/* Set up move list */
		nmove = 0;
		amove = 0;
		FREEMEM(smove);
		FREEMEM(scopy);

		/* Get rid of displayed picked spots and fetched field */
		if (fullset)
			mset = take_mf_set(TempMeta, "spot", "d", "mset", "mset");
		if (NotNull(mset))
			{
			mset = destroy_set(mset);
			FREEMEM(msource);
			FREEMEM(msubsrc);
			FREEMEM(mrtime);
			FREEMEM(mvtime);
			FREEMEM(melement);
			FREEMEM(mlevel);
			mformat = DisplayFormatSimple;
			}

		/* Empty display structure */
		empty_temp();
		present_all();
		maction = FALSE;

		edit_select(NullCal, FALSE);
		State = Wait;
		}

	/* Set state for "fetch" */
	if (same(mode, "fetch"))
		{
		/* Fetch also cancels */
		switch (State)
			{

			/* Cannot ReFetch until current merge is resolved! */
			case Translate:
			case WaitAction:
							return FALSE;

			case Pick:		State = ReFetch;
							put_message("edit-cancel");
							break;

			default:		State = Fetch;
			}
		fetch = TRUE;
		}

	/* Set state for "select all" */
	if (same(mode, "select all"))
		{

		/* Select All only works while in pick */
		switch (State)
			{
			case Pick:	State = PickAll;
						break;
			}
		}

	/* Set state for Right button "merge" or "translate" */
	if ( (same(mode, "merge") || same(mode, "translate")) && nmove <= 0 )
		{
		put_message("point-no-pick");
		(void) sleep(1);
		edit_select(NullCal, FALSE);
		State = Pick;
		return FALSE;
		}
	if (same(mode, "merge"))     State = Merge;
	if (same(mode, "translate")) State = Translate;

	/* Set state for Right button "draw outline" */
	if (same(mode, "draw outline"))	State = Draw;

	/* Set state for Right button "preset outline" */
	if (same(mode, "preset outline"))
		{

		/* Check for last drawn outline or named outline */
		if (same(source, FpaEditorLastDrawnBoundary))
			outline = retrieve_drawn_outline();
		else
			outline = retrieve_named_outline(source);
		if (IsNull(outline))
			{
			if (same(source, FpaEditorLastDrawnBoundary))
				{
				put_message("edit-no-drawn-outline");
				pr_warning("Editor",
					"[edit_merge_point] No outline drawn yet!\n");
				}
			else
				{
				put_message("edit-no-preset-outline", source);
				pr_error("Editor",
					"[edit_merge_point] No preset map outline: \"%s\"\n",
					source);
				}
			(void) sleep(1);
			State = Pick;
			}

		/* Display the outline */
		else
			{
			define_lspec(&outline->lspec, 255, 0, NULL, False, 2.0, 0.0,
						 (HILITE) 2);
			add_item_to_metafile(TempMeta, "curve", "c",
								 "", "", (ITEM) outline);
			present_temp(TRUE);
			State = DrawPick;
			}
		}

	/* Repeat until told to quit */
	while (TRUE)
		{

		/* Resume at current stage */
		switch (State)
			{

			/* Wait around for a "fetch" */
			case Wait:

				/* Keep returning until we get a "fetch" */
				put_message("merge-field");
				edit_allow_preset_outline(FALSE);
				return FALSE;

			/* Wait around for a "merge" or "translate" */
			case WaitAction:

				/* Keep returning until we get a "merge" or "translate" */
				put_message("point-merge-action");
				edit_allow_preset_outline(FALSE);
				return FALSE;

			/* Fetch the requested merge field */
			case Fetch:

				fetch = FALSE;

				if (same(source,  "-")) (void) strcpy(source,  "");
				if (same(subsrc,  "-")) (void) strcpy(subsrc,  "");
				if (same(rtime,   "-")) (void) strcpy(rtime,   "");
				if (same(vtime,   "-")) (void) strcpy(vtime,   "");
				if (same(element, "-")) (void) strcpy(element, "");
				if (same(level,   "-")) (void) strcpy(level,   "");
				if (blank(source))
					{
					/* Need a merge field */
					put_message("merge-nofld");
					(void) sleep(2);
					State = Wait;
					continue;
					}

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_point] Fetching merge field\n");
#				endif /* DEBUG */

				/* Interpret and fetch the requested field */
				put_message("merge-fetching");
				(void) init_fld_descript(&fdesc);
				dpath = get_directory("Data");
				if (!set_fld_descript(&fdesc,
					FpaF_MAP_PROJECTION,    MapProj,
					FpaF_DIRECTORY_PATH,    dpath,
					FpaF_SOURCE_NAME,       source,
					FpaF_SUBSOURCE_NAME,    subsrc,
					FpaF_RUN_TIME,          rtime,
					FpaF_VALID_TIME,        vtime,
					FpaF_ELEMENT_NAME,      element,
					FpaF_LEVEL_NAME,        level,
					FpaF_END_OF_LIST)) return FALSE;
					/* >>>>>
					FpaF_ELEMENT_NAME,      CurrElement,
					FpaF_LEVEL_NAME,        CurrLevel,
					FpaF_END_OF_LIST)) return FALSE;
					<<<<< */
				if (NotNull(mset))
					{
					mset = destroy_set(mset);
					FREEMEM(msource);
					FREEMEM(msubsrc);
					FREEMEM(mrtime);
					FREEMEM(mvtime);
					FREEMEM(melement);
					FREEMEM(mlevel);
					mformat = DisplayFormatSimple;
					}
				mset = retrieve_spotset(&fdesc);
				if (IsNull(mset))
					{
					/* Can't access given source */
					put_message("merge-access");
					(void) sleep(2);
					State = Wait;
					continue;
					}
				if (mset->num <= 0)
					{
					/* No spots to merge */
					put_message("point-merge-none");
					(void) sleep(2);
					State = Wait;
					continue;
					}

				/* Got it - prepare set of spots for display */
				fullset  = TRUE;
				msource  = STRMEM(msource,  source);
				msubsrc  = STRMEM(msubsrc,  subsrc);
				mrtime   = STRMEM(mrtime,   rtime);
				mvtime   = STRMEM(mvtime,   vtime);
				melement = STRMEM(melement, element);
				mlevel   = STRMEM(mlevel,   level);
				mformat  = field_display_format(element, level);

				/* >>>>> modify field units for CurrElement/CurrLevel?? <<<<< */
				prep_set(mset, mformat);
				setup_set_presentation(mset, CurrElement, CurrLevel, "FPA");
				highlight_set(mset, 4);
				add_set_to_metafile(TempMeta, "d", "mset", "mset", mset);

				/* Display the fetched field */
				present_temp(TRUE);
				post_partial(TRUE);
				clear_message();

				State = Pick;
				continue;

			/* Re-fetch the requested merge field */
			case ReFetch:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_point] ReFetch\n");
#				endif /* DEBUG */

				ignore_partial();

				/* Empty picked spot list */
				if (nmove > 0)
					{
					for (im=0; im<nmove; im++) (void) destroy_spot(scopy[im]);
					nmove = 0;
					amove = 0;
					FREEMEM(smove);
					FREEMEM(scopy);
					}

				/* Get rid of displayed picked spots and fetched field */
				if (fullset)
					mset = take_mf_set(TempMeta, "spot", "d", "mset", "mset");
				if (NotNull(mset))
					{
					mset = destroy_set(mset);
					FREEMEM(msource);
					FREEMEM(msubsrc);
					FREEMEM(mrtime);
					FREEMEM(mvtime);
					FREEMEM(melement);
					FREEMEM(mlevel);
					mformat = DisplayFormatSimple;
					}

				/* Empty display structure */
				empty_temp();
				present_all();

				/* Wait for a "fetch" */
				State = (fetch)? Fetch: Wait;
				continue;

			/* Pick spots to merge */
			case Pick:

				/* Use the current list of spots to pick from */
				set = find_mf_set(TempMeta, "spot", "d", "mset", "mset");

				/* Allow specification of multiple moves */
				while (nmove < maxmove)
					{

					/* Put up a prompt and set states */
					if (nmove <= 0)
						{
						put_message("point-merge-pick");
						edit_select(NullCal, FALSE);
						}
					else
						{
						put_message("point-merge-pick2");
						edit_select((CAL) smove[0]->attrib, FALSE);
						}
					edit_allow_preset_outline(TRUE);

					/* Get the point entered */
					if (!ready_Xpoint(DnEdit, p0, &butt)) return drawn;
					if (butt != LeftButton)
						{
						put_message("edit-wrong-button");
						(void) ignore_Xpoint();
						continue;
						}
					put_message("point-pick");
					if (!pick_Xpoint(DnEdit, 0, p1, &butt)) continue;
					if (!inside_dn_window(DnEdit, p1))
						{
						put_message("edit-outside-map");
						(void) sleep(1);
						continue;
						}

					/* See which spot we picked */
					spot = closest_spot(set, p1, NULL, NULL, NULL, NULL);
					if (!spot)
						{
						put_message("point-no-pick");
						(void) sleep(1);
						continue;
						}

					/* If spot was already picked, un-pick it */
					for (im=0; im<nmove; im++)
						{
						if (spot == smove[im]) break;
						}
					if (im < nmove)
						{
						put_message("point-unpicked");

						/* Un-highlight the picked spot */
						/*  and remove it from the list */
						highlight_item("spot", (ITEM) spot, 4);
						(void) destroy_spot(scopy[im]);
						nmove--;
						for ( ; im<nmove; im++)
							{
							smove[im] = smove[im+1];
							scopy[im] = scopy[im+1];
							}
						smove[im] = NullSpot;
						scopy[im] = NullSpot;

						present_all();
						}

					/* Otherwise pick it */
					else
						{
						put_message("point-picked");
						nmove++;
						if (nmove > amove)
							{
							amove = nmove;
							smove = GETMEM(smove, SPOT, amove);
							scopy = GETMEM(scopy, SPOT, amove);
							}

						/* Highlight the picked spot */
						/*  and add it to the list */
						scopy[im] = copy_spot(spot);
						highlight_item("spot", (ITEM) spot, 2);
						smove[im] = spot;
						present_temp(TRUE);
						}
					}

				/* Have we picked any spots? */
				if (nmove > 0) edit_select((CAL) smove[0]->attrib, FALSE);
				else           edit_select(NullCal, FALSE);

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Pick all spots to merge */
			case PickAll:

				put_message("point-all-picked");

				/* Remove picked spots from the list */
				for (im=0; im<nmove; im++) (void) destroy_spot(scopy[im]);

				set = find_mf_set(TempMeta, "spot", "d", "mset", "mset");
				nmove = set->num;
				if (nmove > amove)
					{
					amove = nmove;
					smove = GETMEM(smove, SPOT, amove);
					scopy = GETMEM(scopy, SPOT, amove);
					}

				/* Highlight all the spots and add them to the list */
				for (im=0; im<nmove; im++)
					{
					spot      = (SPOT) set->list[im];
					scopy[im] = copy_spot(spot);
					highlight_item("spot", (ITEM) spot, 2);
					smove[im] = spot;
					}
				present_temp(TRUE);

				/* Have we picked any spots? */
				if (nmove > 0) edit_select((CAL) smove[0]->attrib, FALSE);
				else           edit_select(NullCal, FALSE);

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Draw outline around spots to merge */
			case Draw:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_point] Drawing outline\n");
#				endif /* DEBUG */

				/* Draw the outline */
				post_partial(FALSE);
				edit_allow_preset_outline(FALSE);
				put_message("point-merge-draw");
				if (ready_Xcurve()) butt = LeftButton;
				else if (!ready_Xpoint(DnEdit, NULL, &butt)) return drawn;

				/* Invalid button */
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					ignore_Xpoint();
					continue;
					}

				/* Draw the outline */
				drawing_control(TRUE);
				if (!ready_Xcurve())
					{
					if (drawing_Xcurve()) return drawn;
					(void) draw_Xcurve(DnEdit, FilterRes*4, SplineRes*4, TRUE);
					if (!ready_Xcurve()) return drawn;
					}
				drawing_control(FALSE);

				/* Dump the outline */
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

				/* Extract and display outline */
				outline = create_curve("", "", "");
				add_line_to_curve(outline, lines[0]);
				post_drawn_outline(outline);
				reset_pipe();
				define_lspec(&outline->lspec, 255, 0, NULL, False, 2.0, 0.0,
							 (HILITE) 2);
				add_item_to_metafile(TempMeta, "curve", "c", "", "",
									 (ITEM) outline);
				present_temp(TRUE);

				/* Move on to next stage */
				State = DrawPick;
				continue;

			/* Pick spots to merge from inside drawn outline */
			case DrawPick:

				if (IsNull(outline))
					{
					edit_select(NullCal, FALSE);
					State = Pick;
					continue;
					}

#				ifdef DEBUG
				pr_diag("Editor",
					"[edit_merge_point] Picking spots inside outline\n");
#				endif /* DEBUG */

				/* Pick spots inside drawn outline */
				busy_cursor(TRUE);
				edit_allow_preset_outline(FALSE);
				put_message("point-merge-select");
				(void) sleep(1);

				/* Use the current list of spots to pick from */
				set = find_mf_set(TempMeta, "spot", "d", "mset", "mset");

				/* Loop through all spots in set */
				for (imx=0; imx<set->num; imx++)
					{
					spot = (SPOT) set->list[imx];
					if (!spot) continue;

					/* Check if spot is inside drawn outline */
					if (!spot_inside_outline(spot, outline)) continue;

					/* Pick spots that are not already picked */
					for (im=0; im<nmove; im++)
						{
						if (spot == smove[im]) break;
						}
					if (im >= nmove)
						{
						put_message("point-picked");
						nmove++;
						if (nmove > amove)
							{
							amove = nmove;
							smove = GETMEM(smove, SPOT, amove);
							scopy = GETMEM(scopy, SPOT, amove);
							}

						/* Highlight the picked spot */
						/*  and add it to the list */
						scopy[im] = copy_spot(spot);
						highlight_item("spot", (ITEM) spot, 2);
						smove[im] = spot;
						}
					}
				present_temp(TRUE);
				busy_cursor(FALSE);

				/* Have we picked any spots? */
				if (nmove > 0) edit_select((CAL) smove[0]->attrib, FALSE);
				else           edit_select(NullCal, FALSE);

				/* Re-display the picked spots */
				State = ReDisplay;
				continue;

			/* Re-pick spots to merge */
			case RePick:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_point] RePick\n");
#				endif /* DEBUG */

				/* Empty picked spot list */
				if (nmove > 0)
					{
					for (im=0; im<nmove; im++) (void) destroy_spot(scopy[im]);
					nmove = 0;
					amove = 0;
					FREEMEM(smove);
					FREEMEM(scopy);
					}

				/* Remove displayed picked spots, but keep the fetched field */
				if (fullset)
					mset = take_mf_set(TempMeta, "spot", "d", "mset", "mset");

				/* Empty display structure */
				empty_temp();

				/* Add the fetched field for re-display */
				fullset = TRUE;
				highlight_set(mset, 4);
				add_set_to_metafile(TempMeta, "d", "mset", "mset", mset);

				/* Re-display the fetched field */
				present_all();
				post_partial(TRUE);

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Merge spots in place */
			case Merge:

				/* Remove displayed fetched field (if required) */
				if (fullset)
					{

					/* Empty display structure */
					empty_temp();
					fullset = FALSE;
					mset    = NullSet;

					/* Now add the picked spots */
					for (im=0; im<nmove; im++)
						{
						(void) add_item_to_set(EditPoints, (ITEM) scopy[im]);
						spot = copy_spot(scopy[im]);
						add_item_to_metafile(TempMeta, "spot", "d", "", "",
											 (ITEM) spot);
						smove[im] = spot;
						}
					}

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_point] Merging\n");
#				endif /* DEBUG */

				ignore_partial();

				if (EditUndoable) post_mod("points");

				/* Modify labels if necessary */
				if (recompute_spotset_labs(EditPoints, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				put_message("edit-done");
				maction = TRUE;

				/* Re-display the picked spots */
				State = ReDisplayAction;
				continue;

			/* Pick and translate a reference point */
			case Translate:

				/* Remove displayed fetched field (if required) */
				if (fullset)
					{

					/* Empty display structure */
					empty_temp();
					fullset = FALSE;
					mset    = NullSet;

					/* Now add the picked spots */
					for (im=0; im<nmove; im++)
						{
						(void) add_item_to_set(EditPoints, (ITEM) scopy[im]);
						spot = copy_spot(scopy[im]);
						add_item_to_metafile(TempMeta, "spot", "d", "", "",
											 (ITEM) spot);
						smove[im] = spot;
						}
					}

				/* Highlight the picked spots */
				for (im=0; im<nmove; im++)
					{
					spot = smove[im];
					highlight_item("spot", (ITEM) spot, 3);
					}

				/* Re-display the spots */
				present_all();
				post_partial(TRUE);

				/* Pick a reference point */
				put_message("point-tran");
				if (!ready_Xpoint(DnEdit, p0, &butt)) return drawn;
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!inside_dn_window(DnEdit, p0))
					{
					put_message("point-tran-out1");
					(void) ignore_Xpoint();
					continue;
					}

				/* Highlight the spots to translate */
				set = copy_mf_set(TempMeta, "spot", "d", "", "");
				for (im=0; im<set->num; im++)
					{
					spot = (SPOT) set->list[im];
					highlight_item("spot", (ITEM) spot, 2);
					}
				prep_set(set, mformat);

				/* Translate the reference point */
				put_message("point-tran-release");
				(void) utrack_Xpoint(DnEdit, set, p0, p1, &butt);
				set = destroy_set(set);
				if (!inside_dn_window(DnEdit, p1))
					{
					put_message("point-tran-out2");
					(void) sleep(1);
					continue;
					}

				/* Display markers */
				mark0 = create_mark("", "", "", p0, 0.0);
				mark1 = create_mark("", "", "", p1, 0.0);
				curv0 = create_curve("", "", "");
				add_point_to_curve(curv0, p0);
				add_point_to_curve(curv0, p1);
				define_mspec(&mark0->mspec, 0, 2, NULL, 0, False, 2.0, 0.0,
							 (HILITE) 2);
				define_mspec(&mark1->mspec, 0, 3, NULL, 0, False, 2.0, 0.0,
							 (HILITE) 2);
				define_lspec(&curv0->lspec, 0, 0, NULL, False, 2.0, 0.0,
							 (HILITE) 2);

				add_item_to_metafile(TempMeta, "mark",  "d", "", "",
									 (ITEM) mark0);
				add_item_to_metafile(TempMeta, "mark",  "d", "", "",
									 (ITEM) mark1);
				add_item_to_metafile(TempMeta, "curve", "c", "", "",
									 (ITEM) curv0);
				present_temp(TRUE);

				/* Move on to next stage */
				State = TranslateDone;
				continue;

			/* Translate the spots */
			case TranslateDone:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_point] Translating\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);

				/* Translate the spots */
				dx = p1[X] - p0[X];
				dy = p1[Y] - p0[Y];
				for (im=0; im<nmove; im++)
					{
					spot = scopy[im];
					spot->anchor[X] += dx;
					spot->anchor[Y] += dy;
					CAL_add_location((CAL) spot->attrib, MapProj, spot->anchor);
					}

				if (EditUndoable) post_mod("points");

				/* Modify labels if necessary */
				if (recompute_spotset_labs(EditPoints, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				put_message("edit-done");
				maction = TRUE;
				busy_cursor(FALSE);

				/* Re-display the picked spots */
				State = ReDisplayAction;
				continue;

			/* Re-display the picked spots */
			case ReDisplay:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_point] Redisplay\n");
#				endif /* DEBUG */

				/* Keep the fetched field (including picked spots) */
				if (fullset)
					mset = take_mf_set(TempMeta, "spot", "d", "mset", "mset");

				/* Empty display structure */
				empty_temp();

				/* Re-display the fetched field (including picked spots) */
				if (NotNull(mset))
					add_set_to_metafile(TempMeta, "d", "mset", "mset", mset);
				present_all();
				post_partial(TRUE);

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Re-display the picked spots (after merging) */
			case ReDisplayAction:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_point] ReDisplayAction\n");
#				endif /* DEBUG */

				/* Empty display structure */
				empty_temp();

				/* Re-pick the picked spots */
				for (im=0; im<nmove; im++)
					{

					/* Highlight the picked spots */
					copy = copy_spot(scopy[im]);
					highlight_item("spot", (ITEM) copy, 2);
					add_item_to_metafile(TempMeta, "spot", "d", "", "",
										 (ITEM) copy);
					smove[im] = copy;
					}

				/* Re-display the spots */
				present_all();
				post_partial(FALSE);

				/* Move on to next stage */
				State = WaitAction;
				continue;

			/* Start again */
			case ReStart:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_point] ReStart\n");
#				endif /* DEBUG */

				/* Empty picked spot list */
				if (nmove > 0)
					{
					for (im=0; im<nmove; im++)
						scopy[im] = remove_item_from_set(EditPoints,
														 (ITEM) scopy[im]);
					nmove = 0;
					amove = 0;
					FREEMEM(smove);
					FREEMEM(scopy);
					}
				edit_select(NullCal, FALSE);

				/* Empty display structure */
				empty_temp();

				/* Wait for a "fetch" */
				State = (fetch)? Fetch: Wait;
				continue;
			}
		}
	}
