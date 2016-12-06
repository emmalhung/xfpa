/***********************************************************************
*                                                                      *
*     e d i t _ l i n e . c                                            *
*                                                                      *
*     Assorted operations for line fields.                             *
*     i.e. discontinuous line segments, etc.                           *
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

#define DEBUG_STRUCTURES

#define	Sl SkipLength
#define	Sw SkipWidth

/***********************************************************************
*                                                                      *
*     r e a d y _ l i n e _ f i e l d                                  *
*                                                                      *
***********************************************************************/

LOGICAL	edit_ready_line_field

	(
	STRING	mode
	)

	{
	int		butt;

	if (!EditUndoable)      edit_can_create(FALSE);
	if (!EditUndoable)      return TRUE;
	if (NotNull(OldCurves)) edit_can_create(FALSE);
	if (NotNull(OldCurves)) return TRUE;

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
			put_message("line-no-field");
			edit_can_create(FALSE);
			if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return FALSE;
			(void) ignore_Xpoint();
			}
		}

	/* No field is present, but it can be created */
	while (TRUE)
		{
		put_message("line-no-fld-cr");
		edit_can_create(TRUE);
		if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return FALSE;
		(void) ignore_Xpoint();
		}
	}

/***********************************************************************
*                                                                      *
*     d r a w _ l i n e                                                *
*                                                                      *
***********************************************************************/

static	CURVE	NewCurve = NullCurve;	/* Just drawn curve */
static	LOGICAL	Dstart   = FALSE;		/* Draw started (but not completed?) */

LOGICAL	edit_draw_line

	(
	STRING	mode,
	CAL		cal
	)

	{
	int		ii;
	int		nlines;
	LINE	*lines;
	CURVE	curve;
	STRING	ctype, sub, val, lab;
	FpaConfigElementLineTypeStruct	*ltypes;
	int		butt;
	LOGICAL	drawn = FALSE;

	/* Current state: */
	/* - where should we resume? */
	static  enum    { Draw, Set } State = Draw;

	/* Line type */
	static	char	CurrSub[40] = "";

#	ifdef DEBUG
	pr_diag("Editor", "[edit_draw_line] %s  State: %d\n", mode, State);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same_start(mode, "cancel"))
		{
		ignore_partial();
		if (EditCurves && EditUndoable)
			{
			highlight_set(EditCurves, 1);
			present_all();
			}
		(void) strcpy(CurrSub, "");
		drawing_control(FALSE);
		Dstart = FALSE;
		State  = Draw;
		return FALSE;
		}
	if (same(mode, "clear"))
		{
		State = Draw;
		return FALSE;
		}

	/* Make sure the curve set contains the right stuff */
	if (!EditCurves)                      return FALSE;
	if (!same(EditCurves->type, "curve")) return FALSE;

	/* Set the line type to highlight */
	CAL_get_defaults(cal, &sub, &val, &lab);

#	ifdef DEBUG
	pr_diag("Editor",
		"[edit_draw_line] sub: %s  val: %s  lab: %s\n", sub, val, lab);
#	endif /* DEBUG */

	/* Re-initialize for changing line types (except when drawing) */
	if (!same(sub, CurrSub) && !Dstart)
		{
		(void) safe_strcpy(CurrSub, sub);
		drawing_control(FALSE);
		if (EditUndoable)
			{
			highlight_set(EditCurves, 1);
			highlight_set_category(EditCurves, sub, 2);
			present_all();
			post_partial(FALSE);
			}
		}

	/* Initialize on startup */
	if (same(mode, "begin"))
		{
		ignore_partial();
		(void) safe_strcpy(CurrSub, sub);
		drawing_control(FALSE);
		Dstart = FALSE;
		if (EditUndoable)
			{
			highlight_set(EditCurves, 1);
			highlight_set_category(EditCurves, sub, 2);
			present_all();
			post_partial(FALSE);
			}
		edit_select(cal, TRUE);
		State = Draw;
		}

	/* Reset label for this type of line */
	ctype = CAL_get_attribute(cal, CALlinetype);
	ii = identify_line_type_by_name(CurrElement, CurrLevel, ctype, &ltypes);
	if (NotNull(ltypes) && ii >= 0)
		{
		lab = ltypes->type_labels[ii];
		}

	/* Construct a prompt */
	if (blank(lab)) (void) sprintf(Msg, "%s: %s", sub, val);
	else            (void) strcpy(Msg, lab);

	/* Take care of setting new value after drawing */
	/* Only recognized when a line has just been drawn */
	if (same(mode, "set") && NotNull(cal)) State = Set;

	/* Repeat until told to quit */
	while (TRUE)
		{

		/* Resume at current stage */
		switch (State)
			{

			/* Draw a curve */
			case Draw:

				/* Turn on set state */
				edit_select(cal, TRUE);

				/* Put up a prompt */
				if (drawing_Xcurve()) put_message("drawing");
				else                  put_message("line-draw2", Msg);

				/* Draw the curve */
				if (ready_Xcurve()) butt = LeftButton;
				else if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return drawn;

				/* Invalid button */
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
					accept_mod();	/* Leaves EditCurves alone */
					}
				post_partial(TRUE);

				/* Draw the curve */
				put_message("line-draw-rel");
				edit_select(NullCal, FALSE);
				drawing_control(TRUE);
				if (!ready_Xcurve())
					{
					Dstart = TRUE;
					if (drawing_Xcurve()) return drawn;
					(void) draw_Xcurve(DnEdit, FilterRes, SplineRes, FALSE);
					if (!ready_Xcurve()) return drawn;
					}
				Dstart = FALSE;
				drawing_control(FALSE);
				edit_select(cal, TRUE);

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
				busy_cursor(TRUE);
				put_message("line-draw-proc");
				curve = create_curve(NULL, NULL, NULL);
				define_item_attribs("curve", (ITEM) curve, cal);
				add_line_to_curve(curve, lines[0]);
				reset_pipe();

				(void) add_item_to_set(EditCurves, (ITEM) curve);

				/* Set up display specs for the new curve */
				NewCurve = curve;
				invoke_set_catspecs(EditCurves);
				if (EditUndoable)
					{
					highlight_set(EditCurves, 1);
					highlight_set_category(EditCurves, sub, 2);
					}

				/* Register the edit */
				drawn = TRUE;
				if (EditUndoable) post_mod("curves");

				/* Modify labels if necessary */
				if (recompute_curveset_labs(EditCurves, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				/* Show the results */
				present_all();
				busy_cursor(FALSE);

				/* Move on to next stage */
				State = Draw;
				continue;

			/* Set the value for the curve just drawn */
			case Set:

				if (!edit_posted())
					{
					NewCurve = NullCurve;
					State = Draw;
					continue;
					}

#				ifdef DEBUG
				pr_diag("Editor", "[edit_draw_line] Setting value (%s)\n", Msg);
#				endif /* DEBUG */

				/* Set a new value */
				busy_cursor(TRUE);
				put_message("line-value");
				define_item_attribs("curve", (ITEM) NewCurve, cal);

				/* Set up display specs for the new line field */
				invoke_set_catspecs(EditCurves);
				if (EditUndoable)
					{
					highlight_set(EditCurves, 1);
					highlight_set_category(EditCurves, sub, 2);
					}

				/* Register the edit */
				drawn = TRUE;
				if (EditUndoable) post_mod("curves");

				/* Modify labels if necessary */
				if (recompute_curveset_labs(EditCurves, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				/* Show the results */
				present_all();
				busy_cursor(FALSE);

				/* Move on to next stage */
				State = Draw;
				continue;
			}
		}
	}

/***********************************************************************
*                                                                      *
*     f l i p _ l i n e                                                *
*                                                                      *
***********************************************************************/

LOGICAL	edit_flip_line

	(
	STRING	mode
	)

	{
	CURVE	curve;
	SET		set;
	POINT	pos;
	int		butt;
	LOGICAL	drawn = FALSE;

	/* Current state: */
	/* - where should we resume? */
	static  enum    { Pick, Flip, Reverse, FlipReverse, Done } State = Pick;

	/* Flip list: */
	/* - curve to be flipped */
	/* - cflip is a pointer into EditCurves */
	/* - ccopy is a copy to be displayed in TempMeta */
	static	CURVE	cflip = NullCurve;
	static	CURVE	ccopy = NullCurve;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_flip_line] %s  State: %d\n", mode, State);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same_start(mode, "cancel"))
		{
		if (NotNull(cflip))
			{
			put_message("edit-cancel");
			ignore_partial();

#			ifdef DEBUG_STRUCTURES
			pr_diag("Editor",
				"[edit_flip_line] Cancel ... empty TempMeta %d field(s)\n",
				TempMeta->numfld);
#			endif /* DEBUG_STRUCTURES */

			/* Empty display structure */
			cflip = NullCurve;
			empty_temp();
			clear_message();
			}
		edit_select(NullCal, FALSE);
		State = Pick;
		return FALSE;
		}

	if (same(mode, "clear"))
		{
		State = Pick;
		return FALSE;
		}

	/* Make sure the curve set contains the right stuff */
	if (!EditCurves)                      return FALSE;
	if (!same(EditCurves->type, "curve")) return FALSE;

	/* Make sure there is something to flip */
	if (EditCurves->num < 1)
		{
		put_message("line-no-flip");
		edit_select(NullCal, FALSE);
		return FALSE;
		}

	/* Initialize on startup */
	if (same(mode, "begin"))
		{
		ignore_partial();
		cflip = NullCurve;
		edit_select(NullCal, FALSE);
		State = Pick;
		}

	/* Set state for Right button "flip" or "reverse" or "flip-reverse" */
	if ( (same(mode, "flip") || same(mode, "reverse")
			|| same(mode, "flip-reverse")) && IsNull(cflip) )
		{
		put_message("line-no-pick");
		(void) sleep(1);
		edit_select(NullCal, FALSE);
		State = Pick;
		return FALSE;
		}
	if (same(mode, "flip"))         State = Flip;
	if (same(mode, "reverse"))      State = Reverse;
	if (same(mode, "flip-reverse")) State = FlipReverse;

	/* Repeat until told to quit */
	while (TRUE)
		{

		/* Resume at current stage */
		switch (State)
			{

			/* Pick curve to flip */
			case Pick:

				/* Put up a prompt */
				if (IsNull(cflip)) put_message("line-flip-pick");
				else               put_message("line-flip-pick2");

				/* Get the point entered */
				if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return drawn;
				if ((butt != LeftButton))
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				put_message("line-pick");
				if (!pick_Xpoint(DnEdit, 0, pos, &butt)) continue;
				if (!inside_dn_window(DnEdit, pos))
					{
					put_message("edit-outside-map");
					(void) sleep(1);
					continue;
					}

				/* See which curve we picked */
				curve = closest_curve(EditCurves, pos, NULL, NULL, NULL);
				if (!curve)
					{
					put_message("line-no-pick");
					(void) sleep(1);
					continue;
					}

				/* If curve was already picked, un-pick it */
				if (curve == cflip)
					{
					put_message("line-unpicked");
					set = find_mf_set(TempMeta, "curve", "c", "", "");
					ccopy = remove_item_from_set(set, (ITEM) ccopy);
					cflip = NullCurve;
					present_all();
					ignore_partial();
					}

				/* Otherwise pick it */
				else
					{

					/* Note that only one curve can be picked at once */
					if (NotNull(cflip))
						{
						cflip = NullCurve;
						empty_temp();
						}
					put_message("line-picked");

					/* Produce a copy of the picked curve */
					cflip = curve;
					ccopy = copy_curve(curve);

					/* Highlight the picked curve */
					highlight_curve(ccopy, 2);
					widen_curve(ccopy, 2.0);

					add_item_to_metafile(TempMeta, "curve", "c", "", "",
										 (ITEM) ccopy);
					present_temp(TRUE);
					post_partial(TRUE);
					}

				/* Have we picked a curve? */
				if (NotNull(cflip)) edit_select((CAL) cflip->attrib, FALSE);
				else                edit_select(NullCal, FALSE);

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Flip the curve */
			case Flip:

				/* Here we go */
				put_message("line-flipping");
				flip_curve(cflip);

				/* Move on to next stage */
				State = Done;
				continue;

			/* Reverse the curve */
			case Reverse:

				/* Here we go */
				put_message("line-reversing");
				reverse_curve(cflip);

				/* Move on to next stage */
				State = Done;
				continue;

			/* Flip and reverse the curve */
			case FlipReverse:

				/* Here we go */
				put_message("line-flipreversing");
				flip_curve(cflip);
				reverse_curve(cflip);

				/* Move on to next stage */
				State = Done;
				continue;

			/* Complete the flip/reverse */
			case Done:

				if (EditUndoable) post_mod("curves");
				post_partial(TRUE);

				/* Empty the display structure */
				empty_temp();

				/* Repick the flipped/reversed curve */
				ccopy = copy_curve(cflip);
				highlight_curve(ccopy, 2);
				widen_curve(ccopy, 2.0);

				add_item_to_metafile(TempMeta, "curve", "c", "", "",
									 (ITEM) ccopy);

				/* Re-display the curve */
				present_temp(TRUE);

				/* Move on to next stage */
				edit_select((CAL) cflip->attrib, FALSE);
				State = Pick;
				continue;
			}
		}
	}

/***********************************************************************
*                                                                      *
*     m o v e _ l i n e                                                *
*     c u r v e _ i n s i d e _ o u t l i n e                          *
*                                                                      *
***********************************************************************/

static	LOGICAL	curve_inside_outline(CURVE, CURVE);

/**********************************************************************/

LOGICAL	edit_move_line

	(
	STRING	mode,
	STRING	name
	)

	{
	CURVE	curve, ecurv, copy, tcurve;
	SPOT	spot, tspot;
	SET		set, tset;
	CURVE	outline;
	int		nlines;
	LINE	*lines;
	int		im, imx, il, ilx, iec, dformat;
	LOGICAL	*pmatch;
	POINT	p0, p1, pz;
	MARK	mark0, mark1;
	CURVE	curv0, curv1;
	float	dx, dy, dx0, dy0, angle, cang, sang, tdist;
	int		butt;
	LOGICAL	drawn = FALSE;

	/* Current state: */
	/* - where should we resume? */
	static  enum    { Pick, PickAll, Draw, DrawPick, Cut, Copy, Paste,
						Translate, TranslateDone,
						Centre, Rotate, RotateDone, ReDisplay } State = Pick;

	/* Was a paste just done? */
	static	LOGICAL		pasting = FALSE;

	/* Has a translate or rotate been started but not completed? */
	static	LOGICAL		mstart  = FALSE;

	/* Should labels be moved too? */
	static	LOGICAL		mlabels = TRUE;
	static	LOGICAL		mlfirst = TRUE;

	/* Move list: */
	/* - list of curves and curve labels to be moved */
	/* - cmove/smove are pointers into EditCurves/EditLabs */
	/* - ccopy/scopy are copies to be displayed in TempMeta */
	static	const int	maxmove = 10000;
	static	int			nmove   = 0;
	static	int			amove   = 0;
	static	CURVE		*cmove  = NullCurveList;
	static	CURVE		*ccopy  = NullCurveList;
	static	int			nlabs   = 0;
	static	int			alabs   = 0;
	static	SPOT		*smove  = NullSpotList;
	static	SPOT		*scopy  = NullSpotList;
	static	int			*sids   = NullInt;

	/* Centre of rotation */
	static	POINT		Cpos    = ZERO_POINT;
	static	float		Ang     = 0.0;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_move_line] %s  State: %d\n", mode, State);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same(mode, "cancel"))
		{
		put_message("edit-cancel");
		drawing_control(FALSE);
		switch (State)
			{
			case Draw:
			case Translate:
			case Centre:
			case Rotate:	ignore_partial();
							edit_allow_preset_outline(FALSE);
							pasting = FALSE;
							mstart  = FALSE;
							State = ReDisplay;
							break;

			default:		ignore_partial();
							edit_allow_preset_outline(FALSE);

							/* Empty move list */
							if (nmove > 0)
								{
								nmove = 0;
								amove = 0;
								FREEMEM(cmove);
								FREEMEM(ccopy);
								}

							/* Empty curve label list */
							if (nlabs > 0)
								{
								nlabs = 0;
								alabs = 0;
								FREEMEM(smove);
								FREEMEM(scopy);
								FREEMEM(sids);
								}

#							ifdef DEBUG_STRUCTURES
							pr_diag("Editor",
								"[edit_move_line] Cancel ... empty TempMeta %d field(s)\n",
								TempMeta->numfld);
#							endif /* DEBUG_STRUCTURES */

							/* Empty display structure */
							empty_temp();
							pasting = FALSE;
							mstart  = FALSE;
							clear_message();

							edit_select(NullCal, FALSE);
							edit_can_copy(FALSE);
							if (copy_posted("curves", CurrElement, CurrLevel))
								edit_can_paste(TRUE);
							State = Pick;
							break;
			}
		}
	if (same(mode, "cancel all"))
		{
		ignore_partial();
		drawing_control(FALSE);
		edit_allow_preset_outline(FALSE);

		/* Empty move list and curve label list */
		if (nmove > 0)
			{

#			ifdef DEBUG_STRUCTURES
			for (im=0; im<nmove; im++)
				pr_diag("Editor",
					"[edit_move_line]   Cancel: %2d ... cmove/ccopy: %x %x\n",
				im, cmove[im], ccopy[im]);
#			endif /* DEBUG_STRUCTURES */

			nmove = 0;
			amove = 0;
			FREEMEM(cmove);
			FREEMEM(ccopy);
			}
		if (nlabs > 0)
			{

#			ifdef DEBUG_STRUCTURES
			for (il=0; il<nlabs; il++)
				pr_diag("Editor",
					"[edit_move_line]   Cancel: %2d ... smove/scopy/sids: %x %x %d\n",
				il, smove[il], scopy[il], sids[il]);
#			endif /* DEBUG_STRUCTURES */

			nlabs = 0;
			alabs = 0;
			FREEMEM(smove);
			FREEMEM(scopy);
			FREEMEM(sids);
			}

#		ifdef DEBUG_STRUCTURES
		pr_diag("Editor",
			"[edit_move_line] Cancel ... empty TempMeta %d field(s)\n",
			TempMeta->numfld);
#		endif /* DEBUG_STRUCTURES */

		/* Empty display structure */
		empty_temp();
		pasting = FALSE;
		mstart  = FALSE;
		clear_message();

		edit_select(NullCal, FALSE);
		edit_can_copy(FALSE);
		if (copy_posted("curves", CurrElement, CurrLevel))
			edit_can_paste(TRUE);
		return FALSE;
		}
	if (same(mode, "clear"))
		{
		/* >>>>> clear the paste buffer??? <<<<< */
		State = Pick;
		return FALSE;
		}

	/* Make sure the curve set contains the right stuff */
	if (!EditCurves)                      return FALSE;
	if (!same(EditCurves->type, "curve")) return FALSE;

	/* Construct temporary fields and buffers on startup only */
	if (same(mode, "begin"))
		{
		ignore_partial();

#		ifdef DEBUG_STRUCTURES
		pr_diag("Editor",
			"[edit_move_line]   Begin: nmove/nlabs: %d %d\n", nmove, nlabs);
#		endif /* DEBUG_STRUCTURES */

		/* Set up move list */
		nmove = 0;
		amove = 0;
		FREEMEM(cmove);
		FREEMEM(ccopy);
		nlabs = 0;
		alabs = 0;
		FREEMEM(smove);
		FREEMEM(scopy);
		FREEMEM(sids);

#		ifdef DEBUG_STRUCTURES
		pr_diag("Editor",
			"[edit_move_line] Begin ... empty TempMeta %d field(s)\n",
			TempMeta->numfld);
#		endif /* DEBUG_STRUCTURES */

		/* Empty display structure */
		empty_temp();
		pasting = FALSE;
		mstart  = FALSE;
		mlfirst = TRUE;

		edit_select(NullCal, FALSE);
		edit_can_copy(FALSE);
		if (copy_posted("curves", CurrElement, CurrLevel))
			edit_can_paste(TRUE);
		edit_allow_preset_outline(FALSE);

		State = Pick;
		}

	/* Check if move mode has changed */
	if ( (mlabels && MoveMode == MOVE_FIELD)
			|| (!mlabels && MoveMode == MOVE_FIELD_AND_LABELS) )
		{

		/* Cannot change move mode if current move is unresolved! */
		if (mstart)
			{

			/* Display warning only once! */
			if (mlfirst)
				{
				put_message("line-move-nomode");
				(void) sleep(1);
				}
			mlfirst = FALSE;
			}

		/* Change move mode and remove labels */
		else if (mlabels)
			{

			/* Remove each curve label from the list */
			tset = find_mf_set(TempMeta, "spot", "d", "smv", "smv");
			for (il=0; il<nlabs; il++)
				{
				(void) remove_item_from_set(tset, (ITEM) scopy[il]);
				}
			nlabs = 0;
			alabs = 0;
			FREEMEM(smove);
			FREEMEM(scopy);
			FREEMEM(sids);

			/* Re-display the picked curves */
			present_all();

			/* Reset check for move mode */
			mlabels = FALSE;
			mlfirst = TRUE;
			}

		/* Change move mode and add labels */
		else
			{

			/* Match labels to picked curves and add them to the list */
			if (nmove > 0 && NotNull(EditLabs))
				{

				/* Loop through all labels in set */
				nlabs = 0;
				for (ilx=0; ilx<EditLabs->num; ilx++)
					{
					spot = (SPOT) EditLabs->list[ilx];
					if (!spot) continue;
					if (same(spot->mclass, "legend")) continue;

					/* Check labels attached to picked curves */
					tcurve = closest_curve(EditCurves, spot->anchor,
										   &tdist, NULL, NULL);
					if (tdist > PickTol) continue;

					for (im=0; im<nmove; im++)
						{
						if (tcurve == cmove[im]) break;
						}
					if (im < nmove)
						{

						/* Highlight each curve label */
						/*  and add it to the list    */
						nlabs++;
						if (nlabs > alabs)
							{
							alabs = nlabs;
							smove = GETMEM(smove, SPOT, alabs);
							scopy = GETMEM(scopy, SPOT, alabs);
							sids  = GETMEM(sids,  int,  alabs);
							}
						smove[nlabs-1] = spot;
						scopy[nlabs-1] = copy_spot(spot);
						sids[nlabs-1]  = im;
						tspot          = scopy[nlabs-1];
						highlight_item("spot", (ITEM) tspot, 2);

						add_item_to_metafile(TempMeta, "spot", "d",
											 "smv", "smv", (ITEM) tspot);
						}
					}
				}

			/* Re-display the picked curves and curve labels */
			present_temp(TRUE);

			/* Reset check for move mode */
			mlabels = TRUE;
			mlfirst = TRUE;
			}
		}

	/* Set state for "select all" */
	if (same(mode, "select all"))
		{

		/* Select All cancels translate or rotate */
		switch (State)
			{
			case Translate:
			case Centre:
			case Rotate:	put_message("edit-cancel");
			case Pick:		State = PickAll;
							break;
			}
		}

	/* Set state for "delete" or "cut" or "copy" or "paste" */
	if ( (same(mode, "delete") || same(mode, "cut") || same(mode, "copy"))
			&& nmove <= 0 )
		{
		edit_can_copy(FALSE);
		put_message("line-no-pick");
		(void) sleep(1);
		edit_select(NullCal, FALSE);
		State = Pick;
		return FALSE;
		}
	if (same(mode, "delete")) State = Cut;
	if (same(mode, "cut"))    State = Cut;
	if (same(mode, "copy"))   State = Copy;
	if (same(mode, "paste"))  State = Paste;

	/* Set state for Right button "translate" or "rotate" */
	if ( (same(mode, "translate") || same(mode, "rotate")) && nmove <= 0 )
		{
		edit_can_copy(FALSE);
		put_message("line-no-pick");
		(void) sleep(1);
		edit_select(NullCal, FALSE);
		State = Pick;
		return FALSE;
		}
	if (same(mode, "translate")) State = Translate;
	if (same(mode, "rotate"))    State = Centre;

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
					"[edit_move_line] No outline drawn yet!\n");
				}
			else
				{
				put_message("edit-no-preset-outline", name);
				pr_error("Editor",
					"[edit_move_line] No preset map outline: \"%s\"\n", name);
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

			/* Pick curves to move */
			case Pick:

				/* See if there are any lines to move */
				if (EditCurves->num <= 0)
					{
					if (copy_posted("curves", CurrElement, CurrLevel))
						edit_can_paste(TRUE);
					put_message("line-no-move");
					edit_select(NullCal, FALSE);
					return drawn;
					}

				/* Allow specification of multiple moves */
				while (nmove < maxmove)
					{
					/* Put up a prompt and set states */
					if (nmove <= 0)
						{
						put_message("line-move-pick");
						edit_can_copy(FALSE);
						if (copy_posted("curves", CurrElement, CurrLevel))
							edit_can_paste(TRUE);
						edit_select(NullCal, FALSE);
						ignore_partial();
						}
					else
						{
						put_message("line-move-pick2");
						edit_can_copy(TRUE);
						edit_select((CAL) cmove[0]->attrib, FALSE);
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
					put_message("line-pick");
					if (!pick_Xpoint(DnEdit, 0, p1, &butt)) continue;
					if (!inside_dn_window(DnEdit, p1))
						{
						put_message("edit-outside-map");
						(void) sleep(1);
						continue;
						}

					/* See which curve we picked */
					curve = closest_curve(EditCurves, p1, NULL, NULL, NULL);
					if (!curve)
						{
						put_message("line-no-pick");
						(void) sleep(1);
						continue;
						}

					/* If curve was already picked, un-pick it */
					for (im=0; im<nmove; im++)
						{
						if (curve == cmove[im]) break;
						}
					if (im < nmove)
						{
						put_message("line-unpicked");

						/* Remove the picked curve from the list */
						set = find_mf_set(TempMeta, "curve", "c",
										  "mmv", "mmv");
						(void) remove_item_from_set(set, (ITEM) ccopy[im]);
						nmove--;
						for (imx=im; imx<nmove; imx++)
							{
							cmove[imx] = cmove[imx+1];
							ccopy[imx] = ccopy[imx+1];
							}
						cmove[imx] = NullCurve;
						ccopy[imx] = NullCurve;

						/* Remove labels attached to this curve */
						if (mlabels)
							{
							for (il=nlabs-1; il>=0; il--)
								{
								if (sids[il] < im)
									continue;
								else if (sids[il] > im)
									sids[il]--;
								else
									{
									/* Remove the curve label from the list */
									tset = find_mf_set(TempMeta, "spot", "d",
													   "smv", "smv");
									(void) remove_item_from_set(tset,
															(ITEM) scopy[il]);
									nlabs--;
									for (ilx=il; ilx<nlabs; ilx++)
										{
										smove[ilx] = smove[ilx+1];
										scopy[ilx] = scopy[ilx+1];
										sids[ilx]  = sids[ilx+1];
										}
									smove[ilx] = NullSpot;
									scopy[ilx] = NullSpot;
									sids[ilx]  = 0;
									}
								}
							}

						/* Display remaining picked curves and curve labels */
						present_all();
						pasting = FALSE;
						}

					/* Otherwise pick it */
					else
						{
						put_message("line-picked");
						nmove++;
						if (nmove > amove)
							{
							amove = nmove;
							cmove = GETMEM(cmove, CURVE, amove);
							ccopy = GETMEM(ccopy, CURVE, amove);
							}

						/* Produce a copy of the picked curve */
						cmove[im] = curve;
						ccopy[im] = copy_curve(curve);
						copy      = ccopy[im];

						/* Highlight the picked curve and add it to the list */
						highlight_curve(copy, 2);
						widen_curve(copy, 2.0);

						add_item_to_metafile(TempMeta, "curve", "c",
											 "mmv", "mmv", (ITEM) copy);

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[edit_move_line] Adding line (%d) to TempMeta (%d)\n",
							im, TempMeta->numfld);
#						endif /* DEBUG_STRUCTURES */

						/* Check for labels attached to picked curve */
						if (mlabels && NotNull(EditLabs))
							{
							for (ilx=0; ilx<EditLabs->num; ilx++)
								{
								spot = (SPOT) EditLabs->list[ilx];
								if (!spot) continue;
								if (same(spot->mclass, "legend")) continue;

								/* Check labels attached to picked curve */
								tcurve = closest_curve(EditCurves, spot->anchor,
													   &tdist, NULL, NULL);
								if (tdist > PickTol) continue;

								if (tcurve == curve)
									{
									nlabs++;
									if (nlabs > alabs)
										{
										alabs = nlabs;
										smove = GETMEM(smove, SPOT, alabs);
										scopy = GETMEM(scopy, SPOT, alabs);
										sids  = GETMEM(sids,  int,  alabs);
										}

									/* Produce a copy of the curve label */
									smove[nlabs-1] = spot;
									scopy[nlabs-1] = copy_spot(spot);
									sids[nlabs-1]  = im;
									tspot          = scopy[nlabs-1];

									/* Highlight the curve label */
									/*  and add it to the list   */
									highlight_item("spot", (ITEM) tspot, 2);
									add_item_to_metafile(TempMeta, "spot", "d",
														 "smv", "smv",
														 (ITEM) tspot);

#									ifdef DEBUG_STRUCTURES
									pr_diag("Editor",
										"[edit_move_line] Adding spot (%d) to TempMeta (%d)\n",
										il, TempMeta->numfld);
#									endif /* DEBUG_STRUCTURES */
									}
								}
							}

						/* Display picked curves and curve labels */
						present_temp(TRUE);
						}
					}

				pr_warning("Editor",
					"[edit_move_line]   End of State = Pick loop!\n");

				/* Have we picked any curves? */
				if (nmove > 0) edit_select((CAL) cmove[0]->attrib, FALSE);
				else           edit_select(NullCal, FALSE);

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Pick all curves to move */
			case PickAll:

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_move_line] PickAll ... empty TempMeta %d field(s)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();

				put_message("line-all-picked");
				nmove = EditCurves->num;
				if (nmove > amove)
					{
					amove = nmove;
					cmove = GETMEM(cmove, CURVE, amove);
					ccopy = GETMEM(ccopy, CURVE, amove);
					}

				/* Highlight all the curves and add them to the list */
				for (im=0; im<nmove; im++)
					{
					cmove[im] = (CURVE) EditCurves->list[im];
					ccopy[im] = copy_curve(cmove[im]);
					copy      = ccopy[im];
					highlight_curve(copy, 2);
					widen_curve(copy, 2.0);

					add_item_to_metafile(TempMeta, "curve", "c",
										 "mmv", "mmv", (ITEM) copy);

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_move_line] PickAll ... adding line (%d) to TempMeta (%d)\n",
						im, TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */
					}

				/* Match labels to picked curves and add them to the list */
				if (nmove > 0 && mlabels && NotNull(EditLabs))
					{

					/* Loop through all labels in set */
					nlabs = 0;
					for (ilx=0; ilx<EditLabs->num; ilx++)
						{
						spot = (SPOT) EditLabs->list[ilx];
						if (!spot) continue;
						if (same(spot->mclass, "legend")) continue;

						/* Check labels attached to picked curves */
						tcurve = closest_curve(EditCurves, spot->anchor,
											   &tdist, NULL, NULL);
						if (tdist > PickTol) continue;

						for (im=0; im<nmove; im++)
							{
							if (tcurve == cmove[im]) break;
							}
						if (im < nmove)
							{

							/* Highlight each curve label */
							/*  and add it to the list    */
							nlabs++;
							if (nlabs > alabs)
								{
								alabs = nlabs;
								smove = GETMEM(smove, SPOT, alabs);
								scopy = GETMEM(scopy, SPOT, alabs);
								sids  = GETMEM(sids,  int,  alabs);
								}
							smove[nlabs-1] = spot;
							scopy[nlabs-1] = copy_spot(spot);
							sids[nlabs-1]  = im;
							tspot          = scopy[nlabs-1];
							highlight_item("spot", (ITEM) tspot, 2);

							add_item_to_metafile(TempMeta, "spot", "d",
												 "smv", "smv", (ITEM) tspot);

#							ifdef DEBUG_STRUCTURES
							pr_diag("Editor",
								"[edit_move_line] PickAll ... adding spot (%d) to TempMeta (%d)\n",
								il, TempMeta->numfld);
#							endif /* DEBUG_STRUCTURES */
							}
						}
					}

				/* Display picked curves and curve labels */
				present_temp(TRUE);
				pasting = FALSE;

				/* Have we picked any curves? */
				if (nmove > 0) edit_select((CAL) cmove[0]->attrib, FALSE);
				else           edit_select(NullCal, FALSE);

#				ifdef DEBUG_STRUCTURES
				for (im=0; im<nmove; im++)
					pr_diag("Editor",
						"[edit_move_line]   PickAll: %2d ... cmove/ccopy: %x %x\n",
					im, cmove[im], ccopy[im]);
				for (il=0; il<nlabs; il++)
					pr_diag("Editor",
						"[edit_move_line]   PickAll: %2d ... smove/scopy/sids: %x %x %d\n",
					il, smove[il], scopy[il], sids[il]);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Draw outline around curves to move */
			case Draw:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_move_line] Drawing outline\n");
#				endif /* DEBUG */

				/* Draw the outline */
				post_partial(FALSE);
				edit_allow_preset_outline(FALSE);
				put_message("line-move-draw");
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

			/* Pick curves to move from inside drawn outline */
			case DrawPick:

				if (IsNull(outline))
					{
					edit_select(NullCal, FALSE);
					State = Pick;
					continue;
					}

#				ifdef DEBUG
				pr_diag("Editor",
					"[edit_move_line] Picking lines inside outline\n");
#				endif /* DEBUG */

				busy_cursor(TRUE);
				edit_allow_preset_outline(FALSE);
				put_message("line-move-select");
				(void) sleep(1);

				/* Pick curves inside drawn outline */
				for (iec=0; iec<EditCurves->num; iec++)
					{
					ecurv = (CURVE) EditCurves->list[iec];
					if (!ecurv) continue;

					/* Check if curve is inside drawn outline */
					if (!curve_inside_outline(ecurv, outline)) continue;

					/* Pick curves that are not already picked */
					for (im=0; im<nmove; im++)
						{
						if (ecurv == cmove[im]) break;
						}
					if (im >= nmove)
						{
						put_message("line-picked");
						nmove++;
						if (nmove > amove)
							{
							amove = nmove;
							cmove = GETMEM(cmove, CURVE, amove);
							ccopy = GETMEM(ccopy, CURVE, amove);
							}

						/* Produce a copy of the picked curve */
						cmove[im] = ecurv;
						ccopy[im] = copy_curve(ecurv);
						copy      = ccopy[im];

						/* Highlight the picked curve and add it to the list */
						highlight_curve(copy, 2);
						widen_curve(copy, 2.0);

						add_item_to_metafile(TempMeta, "curve", "c",
											 "mmv", "mmv", (ITEM) copy);

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[edit_move_line] Adding line (%d) to TempMeta (%d)\n",
							im, TempMeta->numfld);
#						endif /* DEBUG_STRUCTURES */

						/* Check for labels attached to picked curve */
						if (mlabels && NotNull(EditLabs))
							{
							for (ilx=0; ilx<EditLabs->num; ilx++)
								{
								spot = (SPOT) EditLabs->list[ilx];
								if (!spot) continue;
								if (same(spot->mclass, "legend")) continue;

								/* Check labels attached to picked curve */
								tcurve = closest_curve(EditCurves, spot->anchor,
													   &tdist, NULL, NULL);
								if (tdist > PickTol) continue;

								if (tcurve == ecurv)
									{
									nlabs++;
									if (nlabs > alabs)
										{
										alabs = nlabs;
										smove = GETMEM(smove, SPOT, alabs);
										scopy = GETMEM(scopy, SPOT, alabs);
										sids  = GETMEM(sids,  int,  alabs);
										}

									/* Produce a copy of the curve label */
									smove[nlabs-1] = spot;
									scopy[nlabs-1] = copy_spot(spot);
									sids[nlabs-1]  = im;
									tspot          = scopy[nlabs-1];

									/* Highlight the curve label */
									/*  and add it to the list   */
									highlight_item("spot", (ITEM) tspot, 2);
									add_item_to_metafile(TempMeta, "spot", "d",
														 "smv", "smv",
														 (ITEM) tspot);

#									ifdef DEBUG_STRUCTURES
									pr_diag("Editor",
										"[edit_move_line] Adding spot (%d) to TempMeta (%d)\n",
										il, TempMeta->numfld);
#									endif /* DEBUG_STRUCTURES */
									}
								}
							}
						}
					}

				/* Display picked curves and curve labels */
				present_temp(TRUE);
				busy_cursor(FALSE);

				/* Have we picked any curves? */
				if (nmove > 0) edit_select((CAL) cmove[0]->attrib, FALSE);
				else           edit_select(NullCal, FALSE);

				/* Re-display the picked curves */
				State = ReDisplay;
				continue;

			/* Cut curves */
			case Cut:

				/* Save the curves and curve labels in the copy buffer */
				put_message("edit-cut");
				post_line_copy(CurrElement, CurrLevel, nmove, cmove,
							   nlabs, smove, sids);

				/* Remove curves from curve set */
				for (im=0; im<nmove; im++)
					{
					curve = cmove[im];
					(void) remove_item_from_set(EditCurves, (ITEM) curve);
					cmove[im] = NullCurve;
					}

				/* Remove labels attached to curves */
				if (mlabels)
					{
					for (il=0; il<nlabs; il++)
						{
						spot = smove[il];
						(void) remove_item_from_set(EditLabs, (ITEM) spot);
						smove[il] = NullSpot;
						}
					}

				if (EditUndoable) post_mod("curves");

				/* Modify labels if necessary */
				if (recompute_curveset_labs(EditCurves, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				/* Special case if ALL the curve labels were just cut! */
				else if (nlabs > 0)
					{
					if (EditUndoable) post_mod("labs");
					}

				/* Empty move list */
				if (nmove > 0)
					{

#					ifdef DEBUG_STRUCTURES
					for (im=0; im<nmove; im++)
						pr_diag("Editor",
							"[edit_move_line]   Cut: %2d ... cmove/ccopy: %x %x\n",
						im, cmove[im], ccopy[im]);
#					endif /* DEBUG_STRUCTURES */

					nmove = 0;
					amove = 0;
					FREEMEM(cmove);
					FREEMEM(ccopy);
					}

				/* Empty curve label list */
				if (nlabs > 0)
					{

#					ifdef DEBUG_STRUCTURES
					for (il=0; il<nlabs; il++)
						pr_diag("Editor",
							"[edit_move_line]   Cut: %2d ... smove/scopy/sids: %x %x %d\n",
						il, smove[il], scopy[il], sids[il]);
#					endif /* DEBUG_STRUCTURES */

					nlabs = 0;
					alabs = 0;
					FREEMEM(smove);
					FREEMEM(scopy);
					FREEMEM(sids);
					}

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_move_line] Cut ... empty TempMeta %d field(s)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();
				pasting = FALSE;
				edit_select(NullCal, FALSE);
				clear_message();

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Copy curves */
			case Copy:

				/* Save the curves and the curve labels in the copy buffer */
				put_message("edit-copy");
				post_line_copy(CurrElement, CurrLevel, nmove, cmove,
							   nlabs, smove, sids);
				clear_message();
				pasting = FALSE;

				/* Re-display the picked curves */
				State = ReDisplay;
				continue;

			/* Paste curves */
			case Paste:

				put_message("edit-paste");

				/* Empty move list */
				if (nmove > 0)
					{
					ignore_partial();
					edit_can_copy(FALSE);

#					ifdef DEBUG_STRUCTURES
					for (im=0; im<nmove; im++)
						pr_diag("Editor",
							"[edit_move_line]   Paste: %2d ... cmove/ccopy: %x %x\n",
						im, cmove[im], ccopy[im]);
#					endif /* DEBUG_STRUCTURES */

					nmove = 0;
					amove = 0;
					FREEMEM(cmove);
					FREEMEM(ccopy);

					/* Empty curve label list */
					if (nlabs > 0)
						{

#						ifdef DEBUG_STRUCTURES
						for (il=0; il<nlabs; il++)
							pr_diag("Editor",
								"[edit_move_line]   Paste: %2d ... smove/scopy/sids: %x %x %d\n",
							il, smove[il], scopy[il], sids[il]);
#						endif /* DEBUG_STRUCTURES */

						nlabs = 0;
						alabs = 0;
						FREEMEM(smove);
						FREEMEM(scopy);
						FREEMEM(sids);
						}

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_move_line] Paste ... empty TempMeta %d field(s)\n",
						TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */

					/* Empty display structure */
					empty_temp();
					}

				/* Retrieve paste buffer */
				paste_line_copy(CurrElement, CurrLevel, &nmove, &cmove,
								&nlabs, &smove, &sids);
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
					ccopy = GETMEM(ccopy, CURVE, amove);
					}

				/* Check for pasting duplicate curves */
				pmatch = INITMEM(LOGICAL, nmove);
				for (im=0; im<nmove; im++)
					{

					/* Initialize match parameter */
					pmatch[im] = FALSE;

					/* Check for matching curves */
					curve = cmove[im];
					for (iec=0; iec<EditCurves->num; iec++)
						{
						ecurv = (CURVE) EditCurves->list[iec];
						if (!ecurv) continue;

						/* Identify curves pasted over themselves */
						if (matching_lines(curve->line, ecurv->line))
							{
							pmatch[im] = TRUE;
							break;
							}
						}
					}

				/* Add pasted curves to curve set */
				for (im=0; im<nmove; im++)
					{
					curve = cmove[im];

					/* Offset curves pasted over themselves */
					if (pmatch[im])
						{
						translate_curve(curve, PickTol, PickTol);
						}

					/* Add pasted curve to curve set */
					(void) add_item_to_set(EditCurves, (ITEM) curve);
					ccopy[im] = copy_curve(curve);
					copy      = ccopy[im];

					/* Highlight the picked curve and add it to the list */
					highlight_curve(copy, 2);
					widen_curve(copy, 2.0);

					add_item_to_metafile(TempMeta, "curve", "c",
										 "mmv", "mmv", (ITEM) copy);

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_move_line] Paste ... adding line (%d) to TempMeta (%d)\n",
						im, TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */
					}

#				ifdef DEBUG_STRUCTURES
				for (im=0; im<nmove; im++)
					pr_diag("Editor",
						"[edit_move_line]   Paste: %2d ... cmove/ccopy: %x %x\n",
					im, cmove[im], ccopy[im]);
#				endif /* DEBUG_STRUCTURES */

				/* Then add pasted labels to curve label list (if requested) */
				if (mlabels)
					{
					if (nlabs > alabs)
						{
						alabs = nlabs;
						scopy = GETMEM(scopy, SPOT, alabs);
						}
					for (il=0; il<nlabs; il++)
						{
						spot = smove[il];

						/* Offset labels for curves pasted over themselves */
						if (pmatch[sids[il]])
							{
							spot->anchor[X] += PickTol;
							spot->anchor[Y] += PickTol;
							}

						/* Add pasted curve label to curve label set */
						(void) add_item_to_set(EditLabs, (ITEM) spot);
						scopy[il] = copy_spot(spot);
						tspot     = scopy[il];

						/* Highlight the curve label and add it to the list */
						highlight_item("spot", (ITEM) tspot, 2);
						add_item_to_metafile(TempMeta, "spot", "d",
											 "smv", "smv", (ITEM) tspot);

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[edit_move_line] Paste ... adding spot (%d) to TempMeta (%d)\n",
							il, TempMeta->numfld);
#						endif /* DEBUG_STRUCTURES */
						}

#					ifdef DEBUG_STRUCTURES
					for (il=0; il<nlabs; il++)
						pr_diag("Editor",
							"[edit_move_line]   Paste: %2d ... smove/scopy/sids: %x %x %d\n",
						il, smove[il], scopy[il], sids[il]);
#					endif /* DEBUG_STRUCTURES */
					}

				if (EditUndoable) post_mod("curves");

				/* Modify labels if necessary */
				if (recompute_curveset_labs(EditCurves, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				/* Free match buffer */
				FREEMEM(pmatch);

				/* Re-display the curves */
				present_temp(TRUE);
				post_partial(TRUE);
				pasting = TRUE;
				edit_select((CAL) cmove[0]->attrib, FALSE);
				clear_message();

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Pick and translate a reference point */
			case Translate:

				/* Highlight the picked curves and curve labels */
				mstart = TRUE;
				for (im=0; im<nmove; im++)
					{
					copy = ccopy[im];
					highlight_curve(copy, 3);
					}
				if (mlabels)
					{
					for (il=0; il<nlabs; il++)
						{
						spot = scopy[il];
						highlight_item("spot", (ITEM) spot, 3);
						}
					}

				/* Remove any marks from uncompleted actions */
				set = take_mf_set(TempMeta, "mark", "d", "", "");
				if (NotNull(set)) set = destroy_set(set);

				/* Re-display the curves */
				present_all();

#				ifdef DEBUG_STRUCTURES
				for (im=0; im<nmove; im++)
					pr_diag("Editor",
						"[edit_move_line]   Translate: %2d ... cmove/ccopy: %x %x\n",
					im, cmove[im], ccopy[im]);
				for (il=0; il<nlabs; il++)
					pr_diag("Editor",
						"[edit_move_line]   Translate: %2d ... smove/scopy/sids: %x %x %d\n",
					il, smove[il], scopy[il], sids[il]);
#				endif /* DEBUG_STRUCTURES */

				post_partial(TRUE);
				edit_can_copy(TRUE);

				/* Pick a reference point */
				put_message("line-tran");
				if (!ready_Xpoint(DnEdit, p0, &butt)) return drawn;
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!inside_dn_window(DnEdit, p0))
					{
					put_message("line-tran-out1");
					(void) ignore_Xpoint();
					continue;
					}

				/* Highlight the curves to translate */
				set = copy_mf_set(TempMeta, "curve", "c", "mmv", "mmv");
				for (im=0; im<set->num; im++)
					{
					curve = (CURVE) set->list[im];
					highlight_curve(curve, 2);
					widen_curve(curve, 2.0);
					}
				dformat = field_display_format(CurrElement, CurrLevel);
				prep_set(set, dformat);

				/* Translate the reference point */
				put_message("line-tran-release");
				(void) utrack_Xpoint(DnEdit, set, p0, p1, &butt);
				set = destroy_set(set);
				if (!inside_dn_window(DnEdit, p1))
					{
					put_message("line-tran-out2");
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

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_move_line] Adding mark/mark/curve to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = TranslateDone;
				continue;

			/* Translate the curves */
			case TranslateDone:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_move_line] Translating\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);

#				ifdef DEBUG_STRUCTURES
				for (im=0; im<nmove; im++)
					pr_diag("Editor",
						"[edit_move_line]   Translating: %2d ... cmove/ccopy: %x %x\n",
					im, cmove[im], ccopy[im]);
				for (il=0; il<nlabs; il++)
					pr_diag("Editor",
						"[edit_move_line]   Translating: %2d ... smove/scopy/sids: %x %x %d\n",
					il, smove[il], scopy[il], sids[il]);
#				endif /* DEBUG_STRUCTURES */

				/* Translate the curves */
				dx = p1[X] - p0[X];
				dy = p1[Y] - p0[Y];
				for (im=0; im<nmove; im++)
					{
					curve = cmove[im];
					translate_curve(curve, dx, dy);
					ccopy[im] = NullCurve;
					}

				/* Then translate labels attached to curves (if requested) */
				if (mlabels)
					{
					(void) spot_list_translate(nlabs, smove, dx, dy);
					for (il=0; il<nlabs; il++)
						{
						scopy[il] = NullSpot;
						}
					}

				if (EditUndoable) post_mod("curves");

				/* Modify labels if necessary */
				if (recompute_curveset_labs(EditCurves, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				put_message("edit-done");
				pasting = FALSE;
				mstart  = FALSE;
				mlfirst = TRUE;
				busy_cursor(FALSE);

				/* Re-display the picked curves */
				State = ReDisplay;
				continue;

			/* Pick the centre of rotation */
			case Centre:

				/* Highlight the picked curves and curve labels */
				mstart = TRUE;
				for (im=0; im<nmove; im++)
					{
					copy = ccopy[im];
					highlight_curve(copy, 3);
					}
				if (mlabels)
					{
					for (il=0; il<nlabs; il++)
						{
						spot = scopy[il];
						highlight_item("spot", (ITEM) spot, 3);
						}
					}

				/* Remove any marks from uncompleted actions */
				set = take_mf_set(TempMeta, "mark", "d", "", "");
				if (NotNull(set)) set = destroy_set(set);

				/* Re-display the curves */
				present_all();

#				ifdef DEBUG_STRUCTURES
				for (im=0; im<nmove; im++)
					pr_diag("Editor",
						"[edit_move_line]   Rotate: %2d ... cmove/ccopy: %x %x\n",
					im, cmove[im], ccopy[im]);
				for (il=0; il<nlabs; il++)
					pr_diag("Editor",
						"[edit_move_line]   Rotate: %2d ... smove/scopy/sids: %x %x %d\n",
					il, smove[il], scopy[il], sids[il]);
#				endif /* DEBUG_STRUCTURES */

				post_partial(TRUE);
				edit_can_copy(TRUE);

				/* Pick the centre of rotation */
				put_message("line-rot-centre");
				if (!ready_Xpoint(DnEdit, p0, &butt)) return drawn;
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!pick_Xpoint(DnEdit, 0, Cpos, &butt)) continue;
				if (!inside_dn_window(DnEdit, Cpos))
					{
					put_message("line-rot-out");
					(void) sleep(1);
					continue;
					}

				mark0 = create_mark("", "", "", Cpos, 0.0);
				define_mspec(&mark0->mspec, 0, 1, NULL, 0, False, 4.0, 0.0,
							 (HILITE) 3);

				add_item_to_metafile(TempMeta, "mark",  "d", "", "",
									 (ITEM) mark0);
				present_temp(TRUE);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_move_line] Adding mark to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = Rotate;
				continue;

			/* Pick and rotate a reference point */
			case Rotate:

				/* Pick a reference point */
				put_message("line-rot");
				if (!ready_Xpoint(DnEdit, p0, &butt)) return drawn;
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!inside_dn_window(DnEdit, p0))
					{
					put_message("line-rot-out1");
					(void) ignore_Xpoint();
					continue;
					}
				if (point_dist(Cpos, p0) < SplineRes)
					{
					put_message("line-rot-ref");
					(void) ignore_Xpoint();
					(void) sleep(1);
					continue;
					}

				/* Highlight the curves to rotate */
				set = copy_mf_set(TempMeta, "curve", "c", "mmv", "mmv");
				for (im=0; im<set->num; im++)
					{
					curve = (CURVE) set->list[im];
					highlight_curve(curve, 2);
					widen_curve(curve, 2.0);
					}
				dformat = field_display_format(CurrElement, CurrLevel);
				prep_set(set, dformat);

				/* Display markers */
				mark0 = create_mark("", "", "", p0, 0.0);
				curv0 = create_curve("", "", "");
				add_point_to_curve(curv0, Cpos);
				add_point_to_curve(curv0, p0);
				define_mspec(&mark0->mspec, 0, 2, NULL, 0, False, 4.0, 0.0,
							 (HILITE) 3);
				define_lspec(&curv0->lspec, 0, 0, NULL, False, 2.0, 0.0,
							 (HILITE) 3);

				add_item_to_metafile(TempMeta, "mark",  "d", "", "",
									 (ITEM) mark0);
				add_item_to_metafile(TempMeta, "curve", "c", "", "",
									 (ITEM) curv0);
				present_temp(TRUE);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_move_line] Adding mark/curve to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Rotate the reference point */
				put_message("line-rot-release");
				(void) urotate_Xpoint(DnEdit, set, Cpos, p0, p1, &Ang, &butt);
				set = destroy_set(set);
				if (!inside_dn_window(DnEdit, p1))
					{
					put_message("line-rot-out2");
					(void) sleep(1);
					continue;
					}

				/* Determine rotation parameters */
				dx0 = p0[X] - Cpos[X];
				dy0 = p0[Y] - Cpos[Y];

				/* Display markers */
				mark0 = create_mark("", "", "", p1, 0.0);
				curv0 = create_curve("", "", "");
				add_point_to_curve(curv0, Cpos);
				add_point_to_curve(curv0, p1);
				curv1 = create_curve("", "", "");
				add_point_to_curve(curv1, p0);
				angle = Ang*0.125; cang = cos(angle); sang = sin(angle);
				pz[X] = dx0*cang - dy0*sang + Cpos[X];
				pz[Y] = dx0*sang + dy0*cang + Cpos[Y];
				add_point_to_curve(curv1, pz);
				angle = Ang*0.250; cang = cos(angle); sang = sin(angle);
				pz[X] = dx0*cang - dy0*sang + Cpos[X];
				pz[Y] = dx0*sang + dy0*cang + Cpos[Y];
				add_point_to_curve(curv1, pz);
				angle = Ang*0.375; cang = cos(angle); sang = sin(angle);
				pz[X] = dx0*cang - dy0*sang + Cpos[X];
				pz[Y] = dx0*sang + dy0*cang + Cpos[Y];
				add_point_to_curve(curv1, pz);
				angle = Ang*0.500; cang = cos(angle); sang = sin(angle);
				pz[X] = dx0*cang - dy0*sang + Cpos[X];
				pz[Y] = dx0*sang + dy0*cang + Cpos[Y];
				add_point_to_curve(curv1, pz);
				angle = Ang*0.625; cang = cos(angle); sang = sin(angle);
				pz[X] = dx0*cang - dy0*sang + Cpos[X];
				pz[Y] = dx0*sang + dy0*cang + Cpos[Y];
				add_point_to_curve(curv1, pz);
				angle = Ang*0.750; cang = cos(angle); sang = sin(angle);
				pz[X] = dx0*cang - dy0*sang + Cpos[X];
				pz[Y] = dx0*sang + dy0*cang + Cpos[Y];
				add_point_to_curve(curv1, pz);
				angle = Ang*0.875; cang = cos(angle); sang = sin(angle);
				pz[X] = dx0*cang - dy0*sang + Cpos[X];
				pz[Y] = dx0*sang + dy0*cang + Cpos[Y];
				add_point_to_curve(curv1, pz);
				add_point_to_curve(curv1, p1);
				curv1->line = smooth_line(curv1->line, FilterRes/4, SplineRes);
				define_mspec(&mark0->mspec, 0, 2, NULL, 0, False, 4.0, 0.0,
							 (HILITE) 2);
				define_lspec(&curv0->lspec, 0, 0, NULL, False, 2.0, 0.0,
							 (HILITE) 2);
				define_lspec(&curv1->lspec, 0, 0, NULL, False, 2.0, 0.0,
							 (HILITE) 2);

				add_item_to_metafile(TempMeta, "mark",  "d", "", "",
									 (ITEM) mark0);
				add_item_to_metafile(TempMeta, "curve", "c", "", "",
									 (ITEM) curv0);
				add_item_to_metafile(TempMeta, "curve", "c", "", "",
									 (ITEM) curv1);
				present_temp(TRUE);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_move_line] Adding mark/curve to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = RotateDone;
				continue;

			/* Rotate the curves */
			case RotateDone:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_move_line] Rotating\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);

#				ifdef DEBUG_STRUCTURES
				for (im=0; im<nmove; im++)
					pr_diag("Editor",
						"[edit_move_line]   Rotating: %2d ... cmove/ccopy: %x %x\n",
					im, cmove[im], ccopy[im]);
				for (il=0; il<nlabs; il++)
					pr_diag("Editor",
						"[edit_move_line]   Rotating: %2d ... smove/scopy/sids: %x %x %d\n",
					il, smove[il], scopy[il], sids[il]);
#				endif /* DEBUG_STRUCTURES */

				/* Rotate the curves */
				dx0 = p0[X] - Cpos[X];
				dy0 = p0[Y] - Cpos[Y];
				for (im=0; im<nmove; im++)
					{
					curve = cmove[im];
					rotate_curve(curve, Cpos, Ang/RAD);
					ccopy[im] = NullCurve;
					}

				/* Then rotate labels attached to curves (if requested) */
				if (mlabels)
					{
					(void) spot_list_rotate(nlabs, smove, Cpos, Ang/RAD);
					for (il=0; il<nlabs; il++)
						{
						scopy[il] = NullSpot;
						}
					}

				if (EditUndoable) post_mod("curves");

				/* Modify labels if necessary */
				if (recompute_curveset_labs(EditCurves, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				put_message("edit-done");
				pasting = FALSE;
				mstart  = FALSE;
				mlfirst = TRUE;
				busy_cursor(FALSE);

				/* Re-display the picked curves */
				State = ReDisplay;
				continue;

			/* Re-display the picked curves */
			case ReDisplay:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_move_line] Redisplay\n");
#				endif /* DEBUG */

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_move_line] Redisplay ... empty TempMeta %d field(s)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();

				/* Re-pick the picked curves */
				for (im=0; im<nmove; im++)
					{

					/* Highlight the picked curve */
					ccopy[im] = copy_curve(cmove[im]);
					copy      = ccopy[im];
					highlight_curve(copy, 2);
					widen_curve(copy, 2.0);

					add_item_to_metafile(TempMeta, "curve", "c",
										 "mmv", "mmv", (ITEM) copy);

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_move_line] Redisplay ... adding line (%d) to TempMeta (%d)\n",
						im, TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */
					}

				/* Check if move mode has changed */
				if ( (mlabels && MoveMode == MOVE_FIELD)
						|| (!mlabels && MoveMode == MOVE_FIELD_AND_LABELS) )
					{

					/* Change move mode and empty curve label list */
					if (mlabels)
						{
						nlabs = 0;
						alabs = 0;
						FREEMEM(smove);
						FREEMEM(scopy);
						FREEMEM(sids);

						/* Reset check for move mode */
						mlabels = FALSE;
						mlfirst = TRUE;
						}

					/* Change move mode and add labels to curve label list */
					else
						{

						/* Match labels to picked curves */
						/*  and add them to the list     */
						if (nmove > 0 && NotNull(EditLabs))
							{

							/* Loop through all labels in set */
							nlabs = 0;
							for (ilx=0; ilx<EditLabs->num; ilx++)
								{
								spot = (SPOT) EditLabs->list[ilx];
								if (!spot) continue;
								if (same(spot->mclass, "legend")) continue;

								/* Check labels attached to picked curves */
								tcurve = closest_curve(EditCurves, spot->anchor,
													   &tdist, NULL, NULL);
								if (tdist > PickTol) continue;

								for (im=0; im<nmove; im++)
									{
									if (tcurve == cmove[im]) break;
									}
								if (im < nmove)
									{

									/* Add the curve label to the list */
									nlabs++;
									if (nlabs > alabs)
										{
										alabs = nlabs;
										smove = GETMEM(smove, SPOT, alabs);
										scopy = GETMEM(scopy, SPOT, alabs);
										sids  = GETMEM(sids,  int,  alabs);
										}
									smove[nlabs-1] = spot;
									sids[nlabs-1]  = im;
									}
								}
							}

						/* Reset check for move mode */
						mlabels = TRUE;
						mlfirst = TRUE;
						}
					}

				/* Then re-pick labels attached to curves (if requested) */
				if (mlabels)
					{
					for (il=0; il<nlabs; il++)
						{

						/* Highlight the curve label */
						scopy[il] = copy_spot(smove[il]);
						spot      = scopy[il];
						highlight_item("spot", (ITEM) spot, 2);
						add_item_to_metafile(TempMeta, "spot", "d",
											 "smv", "smv", (ITEM) spot);

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[edit_move_line] Redisplay ... adding spot (%d) to TempMeta (%d)\n",
							il, TempMeta->numfld);
#						endif /* DEBUG_STRUCTURES */
						}
					}

				/* Re-display the curves and curve labels */
				present_all();
				post_partial(FALSE);

				/* Move on to next stage */
				State = Pick;
				continue;
			}
		}
	}

/**********************************************************************/

static	LOGICAL	curve_inside_outline

	(
	CURVE	curve,
	CURVE	outline
	)

	{
	int		ii;
	LOGICAL	inside;
	LINE	oline, line;

	if (!curve)                      return FALSE;
	if (!curve->line)                return FALSE;
	if (!outline)                    return FALSE;
	if (!outline->line)              return FALSE;
	if (!line_closed(outline->line)) return FALSE;

	/* Ensure that all points in curve are inside outline */
	oline = outline->line;
	line  = curve->line;
	for (ii=0; ii<line->numpts; ii++)
		{
		line_test_point(oline, line->points[ii], NullFloat, NullPoint, NullInt,
						&inside, NullLogical);
		if (!inside) return FALSE;
		}

	/* Return TRUE if all points in curve were inside outline */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     m o d i f y _ l i n e                                            *
*     m o d i f y _ l i n e _ p i c k                                  *
*     m o d i f y _ l i n e _ s e l e c t                              *
*     m o d i f y _ l i n e _ e n h a n c e                            *
*     m o d i f y _ l i n e _ d r a w                                  *
*     m o d i f y _ l i n e _ d i s c a r d                            *
*     m o d i f y _ l i n e _ s c u l p t                              *
*                                                                      *
***********************************************************************/

typedef	enum
			{
			ModPick, ModDraw, ModDiscard,	/* interactive states */
			ModSculpt,
			ModRePick, ModReDraw,			/* "cancel" states */
			ModSet, ModDelete, ModDrawDone,	/* modification states */
			ModSculptDone,
			ModConfirm, ModBailout			/* complete modification */
			}
		MODSTATE;

static	MODSTATE	modify_line_pick(void);
static	void		modify_line_select(LOGICAL);
static	void		modify_line_enhance(void);
static	MODSTATE	modify_line_draw(void);
static	MODSTATE	modify_line_discard(void);
static	MODSTATE	modify_line_sculpt(void);

/* For debug purposes */
static	void		show_mod_state(STRING, MODSTATE);

static	void		show_mod_state
	(
	STRING		pre,
	MODSTATE	state
	)

	{
	STRING	name;

#	ifdef DEBUG
	switch (state)
		{
		case ModPick:		name = "Pick";			break;
		case ModDraw:		name = "Draw";			break;
		case ModDiscard:	name = "Discard";		break;
		case ModSculpt:		name = "Sculpt";		break;
		case ModRePick:		name = "RePick";		break;
		case ModReDraw:		name = "ReDraw";		break;
		case ModSet:		name = "Set";			break;
		case ModDelete:		name = "Delete";		break;
		case ModDrawDone:	name = "DrawDone";		break;
		case ModSculptDone:	name = "SculptDone";	break;
		case ModConfirm:	name = "Confirm";		break;
		case ModBailout:	name = "BailOut";		break;
		default:			name = "!Unknown!";		break;
		}
	pr_diag("Editor", "[edit_modify_line] State: %s %s\n", pre, name);
#	endif /* DEBUG */

	return;
	}
/* For debug purposes */

static	LOGICAL	Mfresh = FALSE;		/* Just picked */
static	LOGICAL	Mdrawn = FALSE;		/* Line has been drawn */
static	LOGICAL	Mstart = FALSE;		/* Modify started (but not completed?) */
static	CURVE	Mpick  = NullCurve;	/* Picked curve */
static	CURVE	Mwork  = NullCurve;	/* Working copy of picked curve */
static	LINE	Mline  = NullLine;	/* Line buffer of picked curve */
static	LINE	Mnew   = NullLine;	/* New line segment */
static	CURVE	Mbuild = NullCurve;	/* New curve constructed from pieces */
static	LINE	Sbox    = NullLine;
static	AREA	Smark0  = NullArea;	/* Marker at first point of curve */
static	AREA	Smark1  = NullArea;	/* Marker at last point of curve */
static	LINE	Lbox    = NullLine;
static	AREA	*Lmarks = NullAreaList;	/* Marker at each point of curve */
static	float	Msize   = 0;			/* Marker size */
static	float	EdRad  = 150.0;
static	float	EdSpr  = 100.0;
static	float	EdTol  = 0.0;

/**********************************************************************/

LOGICAL	edit_modify_line

	(
	STRING	mode,
	CAL		cal
	)

	{
	int			il, im, lp;
	float		tdist;
	CURVE		curve;
	SPOT		spot;
	POINT		pos;
	MODSTATE	next;
	LOGICAL		drawn = FALSE;

	/* Current state: */
	/* - where should we resume? */
	static	MODSTATE	State = ModPick;

	/* Move list: */
	/* - list of curve labels to be moved */
	/* - smove is pointers into EditLabs  */
	static	int			nlabs   = 0;
	static	int			alabs   = 0;
	static	SPOT		*smove  = NullSpotList;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_modify_line] %s  State: %d\n", mode, State);
#	endif /* DEBUG */
	show_mod_state("INITIAL", State);

	/* Clean up previous edits */
	if (same(mode, "cancel"))
		{

		switch (State)
			{
			case ModDiscard:	State = ModReDraw;
								show_mod_state("RESET", State);
								break;

			case ModDraw:		State = (Mstart)? ModReDraw: ModRePick;
								show_mod_state("RESET", State);
								Mstart = FALSE;
								circle_echo(FALSE);
								break;

			case ModSculpt:		State = ModRePick;
								show_mod_state("RESET", State);
								circle_echo(FALSE);
								break;

			default:			empty_temp();
								drawing_control(FALSE);
								modifying_control(FALSE);
								if (nlabs > 0)
									{
									nlabs = 0;
									alabs = 0;
									FREEMEM(smove);
									}
								return FALSE;
			}
		}
	if (same(mode, "cancel all"))
		{
		ignore_partial();

		/* Empty curve label list */
		if (nlabs > 0)
			{
			nlabs = 0;
			alabs = 0;
			FREEMEM(smove);
			}

		/* Clean up temporary buffers */
		edit_select(NullCal, FALSE);
		Mfresh = FALSE;
		Mdrawn = FALSE;
		Mpick  = NullCurve;
		Mwork  = NullCurve;
		Mline  = NullLine;
		Mbuild = NullCurve;
		Smark0 = NullArea;
		Smark1 = NullArea;
		Lmarks = NullAreaList;
		empty_line(Mnew);

		/* Empty display structure */
		empty_temp();
		drawing_control(FALSE);
		modifying_control(FALSE);
		return FALSE;
		}
	if (same(mode, "clear"))
		{
		return FALSE;
		}

	/* Make sure the curve set contains the right stuff */
	if (!EditCurves)                      return FALSE;
	if (!same(EditCurves->type, "curve")) return FALSE;

	/* Initialize on startup */
	if (same(mode, "begin"))
		{
		ignore_partial();

		/* Set up move list */
		if (nlabs > 0)
			{
			nlabs = 0;
			alabs = 0;
			FREEMEM(smove);
			}

		drawing_control(FALSE);
		modifying_control(FALSE);
		Mfresh = FALSE;
		Mdrawn = FALSE;
		Mstart = FALSE;
		edit_select(NullCal, FALSE);
		State = ModPick;
		show_mod_state("BEGIN", State);
		}

	/* Take care of deleting or setting new value */
	/* Only recognized when about to draw */
	if (same(mode, "set"))
		{
		if (State == ModDraw)   State = ModSet;
		if (State == ModSculpt) State = ModSet;
		show_mod_state("SET", State);
		}
	if (same(mode, "delete"))
		{
		if (State == ModDraw)   State = ModDelete;
		if (State == ModSculpt) State = ModDelete;
		show_mod_state("DELETE", State);
		}

	/* Set state for Right button "modify_confirm" */
	if (same(mode, "modify_confirm"))
		{
		if (State == ModDiscard) State = ModDrawDone;
		if (State == ModSculpt)  State = ModSculptDone;
		show_mod_state("CONFIRM", State);
		}

	/* Repeat until told to quit */
	while (TRUE)
		{
		show_mod_state("==", State);

		/* Resume at current stage */
		switch (State)
			{

			/* Pick a curve to modify */
			case ModPick:

				/* Pick a curve */
				ignore_partial();
				next = modify_line_pick();
				if (next == ModPick) return drawn;

				/* Curve has been picked ... so get the curve labels too */
				nlabs = 0;
				if (NotNull(EditLabs) && EditLabs->num > 0)
					{
					for (il=0; il<EditLabs->num; il++)
						{
						spot = (SPOT) EditLabs->list[il];
						if (!spot) continue;
						if (same(spot->mclass, "legend")) continue;

						/* Only check labels attached to picked curve */
						curve = closest_curve(EditCurves, spot->anchor,
										  	&tdist, NULL, NULL);
						if (tdist > PickTol) continue;
						if (curve != Mpick) continue;

						/* Add the curve label to the list */
						nlabs++;
						if (nlabs > alabs)
							{
							alabs = nlabs;
							smove = GETMEM(smove, SPOT, alabs);
							}
						smove[nlabs-1] = spot;
						}
					}

#				ifdef DEBUG_STRUCTURES
				if (NotNull(EditLabs) && EditLabs->num > 0)
					{
					for (il=0; il<EditLabs->num; il++)
						{
						pr_diag("Editor",
							"[edit_modify_line] EditLabs[%d]: %x\n",
							il, EditLabs->list[il]);
						}
					}
				pr_diag("Editor",
					"[edit_modify_line] Modifying curve with %d labels\n", nlabs);
				if (nlabs > 0)
					{
					for (im=0; im<nlabs; im++)
						{
						pr_diag("Editor",
							"[edit_modify_line] smove[%d]: %x\n",
							im, smove[im]);
						}
					}
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = next;
				show_mod_state("->", State);
				continue;

			/*  Repick the curve to modify */
			case ModRePick:

				put_message("edit-cancel");

				/* Empty curve label list */
				if (nlabs > 0)
					{
					nlabs = 0;
					alabs = 0;
					FREEMEM(smove);
					}

				/* Clean up temporary buffers */
				circle_echo(FALSE);
				ignore_partial();
				edit_select(NullCal, FALSE);
				Mfresh = FALSE;
				Mdrawn = FALSE;
				Mpick  = NullCurve;
				Mwork  = NullCurve;
				Mline  = NullLine;
				Mbuild = NullCurve;
				Smark0 = NullArea;
				Smark1 = NullArea;
				Lmarks = NullAreaList;

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_modify_line] ModRePick ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();
				drawing_control(FALSE);
				modifying_control(FALSE);

				/* Move on to next stage */
				State = ModPick;
				show_mod_state("->", State);
				/* >>>>> do we need to return FALSE??? <<<<< */
				continue;

			/* Delete the picked curve */
			case ModDelete:

				/* Freeze all pending edits so only this one can be undone */
				if (EditUndoable)
					{
					ignore_partial();
					put_message("edit-freeze");
					accept_mod();	/* Leaves EditCurves alone */
					}
				post_partial(TRUE);

#				ifdef DEBUG
				pr_diag("Editor", "[edit_modify_line] Deleting\n");
#				endif /* DEBUG */

				/* Remove the curve */
				busy_cursor(TRUE);
				put_message("line-deleting");
				remove_item_from_set(EditCurves, (ITEM) Mpick);

				/* Remove the labels attached to the curve too */
				if (nlabs > 0)
					{
					for (im=nlabs-1; im>=0; im--)
						(void) remove_item_from_set(EditLabs, (ITEM) smove[im]);
					}

				/* Show the results */
				present_all();

				/* Move on to next stage */
				State = ModConfirm;
				show_mod_state("->", State);
				continue;

			/* Set the value of the picked curve */
			case ModSet:

				/* Freeze all pending edits so only this one can be undone */
				/* (first time after picking) */
				if (Mfresh)
					{
					if (EditUndoable && edit_posted())
						{
						ignore_partial();
						put_message("edit-freeze");
						accept_mod();	/* Leaves EditCurves alone */
						drawn = FALSE;
						}
					}
				post_partial(TRUE);

#				ifdef DEBUG
				pr_diag("Editor", "[edit_modify_line] Setting value (%s)\n",
						CAL_get_attribute(cal, CALuserlabel));
#				endif /* DEBUG */

				/* Set a new value */
				busy_cursor(TRUE);
				put_message("line-value");
				define_item_attribs("curve", (ITEM) Mpick, cal);

				/* Register the edit */
				ignore_partial();
				drawn  = TRUE;
				Mdrawn = TRUE;
				Mfresh = FALSE;
				invoke_set_catspecs(EditCurves);
				if (EditUndoable) highlight_set(EditCurves, 1);
				if (EditUndoable) post_mod("curves");

				/* Modify labels if necessary */
				if (recompute_curveset_labs(EditCurves, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				/* Clean up temporary buffers */
				Mwork  = NullCurve;
				Mline  = NullLine;
				Mbuild = NullCurve;
				Smark0 = NullArea;
				Smark1 = NullArea;
				Lmarks = NullAreaList;

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_modify_line] ModSet ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();

				/* Reset the working curve */
				modify_line_select(TRUE);
				modify_line_enhance();

				/* Show the results */
				busy_cursor(FALSE);
				drawing_control(FALSE);
				present_all();

				/* Move on to next stage */
				State = (ModifyMode==MODIFY_PUCK)? ModSculpt: ModDraw;
				show_mod_state("->", State);
				continue;

			/* Draw a new segment on the picked curve */
			case ModDraw:

				/* Check if drawing mode has changed */
				if (ModifyMode == MODIFY_PUCK)
					{

					/* Clean up temporary buffers */
					Mwork  = NullCurve;
					Mline  = NullLine;
					Mbuild = NullCurve;
					Smark0 = NullArea;
					Smark1 = NullArea;
					Lmarks = NullAreaList;

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_modify_line] ModDraw ... empty TempMeta %d fields\n",
						TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */

					/* Empty display structure */
					empty_temp();

					/* Reset the working curve */
					modify_line_select(TRUE);
					modify_line_enhance();

					State = ModSculpt;
					show_mod_state("->", State);
					continue;
					}

				/* Freeze all pending edits so only this one can be undone */
				/* (first time after picking) */
				if (Mfresh)
					{
					if (EditUndoable && edit_posted())
						{
						ignore_partial();
						put_message("edit-freeze");
						accept_mod();	/* Leaves EditCurves alone */
						drawn = FALSE;
						}
					}
				post_partial(TRUE);

				/* Add a mark at each end of curve */
				if (IsNull(Smark0) || IsNull(Smark1))
					{
					if (IsNull(Sbox))
						{
						Sbox = create_line();
						add_point_to_line(Sbox, make_point(-0.5,-0.5));
						add_point_to_line(Sbox, make_point( 0.5,-0.5));
						add_point_to_line(Sbox, make_point( 0.5, 0.5));
						add_point_to_line(Sbox, make_point(-0.5, 0.5));
						add_point_to_line(Sbox, make_point(-0.5,-0.5));
						}
					gxSetupTransform(DnEdit);
					lp     = Mline->numpts - 1;
					Msize  = 0.5 * LmarkSize * gxGetMfact() / 1000;
					Smark0 = create_area("", "", "");
					Smark1 = create_area("", "", "");
					define_area_boundary(Smark0, copy_line(Sbox));
					define_area_boundary(Smark1, copy_line(Sbox));
					scale_line(Smark0->bound->boundary, Msize, Msize);
					scale_line(Smark1->bound->boundary, Msize, Msize);
					translate_line(Smark0->bound->boundary,
								Mline->points[0][X], Mline->points[0][Y]);
					translate_line(Smark1->bound->boundary,
								Mline->points[lp][X], Mline->points[lp][Y]);
					define_lspec(&Smark0->lspec, 0, 0, NULL, False, 2.0, 0.0,
								 (HILITE) 3);
					define_lspec(&Smark1->lspec, 0, 0, NULL, False, 2.0, 0.0,
								 (HILITE) 3);

					add_item_to_metafile(TempMeta, "area",  "b", "", "",
								(ITEM) Smark0);
					add_item_to_metafile(TempMeta, "area",  "b", "", "",
								(ITEM) Smark1);
					present_all();

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_modify_line] Adding mark/mark to TempMeta (%d)\n",
						TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */
					}

				/* Draw a new segment */
				next = modify_line_draw();
				if (next == ModDraw) return drawn;

				/* Move on to next stage */
				State = next;
				show_mod_state("->", State);
				continue;

			/*  Redraw the new segment */
			case ModReDraw:

				put_message("edit-cancel");
				if (Mdrawn) ignore_partial();

				/* Un-highlight the modified curve */
				if (Mbuild) highlight_curve(Mbuild, -1);

				/* Clean up temporary buffers */
				Mbuild = NullCurve;
				empty_line(Mnew);

				/* Show the results */
				present_all();
				drawing_control(FALSE);
				modifying_control(FALSE);

				/* Move on to next stage */
				State = (ModifyMode==MODIFY_PUCK)? ModSculpt: ModDraw;
				show_mod_state("->", State);
				/* >>>>> do we need to return FALSE??? <<<<< */
				continue;

			/* Discard the unwanted segment */
			case ModDiscard:

				/* Check if drawing mode has changed */
				if (ModifyMode == MODIFY_PUCK)
					{

					/* Clean up temporary buffers */
					Mwork  = NullCurve;
					Mline  = NullLine;
					Mbuild = NullCurve;
					Smark0 = NullArea;
					Smark1 = NullArea;
					Lmarks = NullAreaList;

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_modify_line] ModDiscard ... empty TempMeta %d fields\n",
						TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */

					/* Empty display structure */
					empty_temp();

					/* Reset the working curve */
					modify_line_select(TRUE);
					modify_line_enhance();

					State = ModSculpt;
					show_mod_state("->", State);
					continue;
					}

				/* Discard a segment */
				post_partial(TRUE);
				next = modify_line_discard();
				if (next == ModDiscard) return drawn;

				/* Move on to next stage */
				State = next;
				show_mod_state("->", State);
				continue;

			/* Modify the curve after drawing */
			case ModDrawDone:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_modify_line] Modifying\n");
#				endif /* DEBUG */

				/* Modify the curve */
				busy_cursor(TRUE);
				put_message("line-modifying");
				empty_curve(Mpick);
				append_curve(Mpick, Mbuild);

				/* Reposition the labels on this curve */
				if (nlabs > 0)
					{
					for (im=0; im<nlabs; im++)
						{

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[edit_modify_line] smove[%d]: %x  Anchor: %.2f/%.2f\n",
							im, smove[im],
							smove[im]->anchor[X], smove[im]->anchor[Y]);
#						endif /* DEBUG_STRUCTURES */

						spot = smove[im];
						(void) curve_test_point(Mpick, spot->anchor, NullFloat,
									pos, NullInt, NullLogical, NullLogical);
						copy_point(spot->anchor, pos);

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[edit_modify_line] spot: %x  Anchor: %.2f/%.2f\n",
							spot, spot->anchor[X], spot->anchor[Y]);
#						endif /* DEBUG_STRUCTURES */
						}
					}

				/* Register the edit */
				ignore_partial();
				drawn  = TRUE;
				Mdrawn = TRUE;
				Mfresh = FALSE;
				invoke_set_catspecs(EditCurves);
				if (EditUndoable) highlight_set(EditCurves, 1);
				if (EditUndoable) post_mod("curves");

				/* Modify labels if necessary */
				if (recompute_curveset_labs(EditCurves, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				/* Clean up temporary buffers */
				Mwork  = NullCurve;
				Mline  = NullLine;
				Mbuild = NullCurve;
				Smark0 = NullArea;
				Smark1 = NullArea;
				Lmarks = NullAreaList;

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_modify_line] ModDrawDone ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();

				/* Reset the working curve */
				modify_line_select(TRUE);
				modify_line_enhance();

				/* Show the results */
				busy_cursor(FALSE);
				present_all();
				modifying_control(FALSE);

				/* Move on to next stage - Do more drawing */
				State = ModDraw;
				show_mod_state("->", State);
				continue;

			/* Sculpt the picked curve */
			case ModSculpt:

				/* Check if drawing mode has changed */
				if (ModifyMode != MODIFY_PUCK)
					{

					/* Clean up temporary buffers */
					Mwork  = NullCurve;
					Mline  = NullLine;
					Mbuild = NullCurve;
					Smark0 = NullArea;
					Smark1 = NullArea;
					Lmarks = NullAreaList;

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_modify_line] ModSculpt ... empty TempMeta %d fields\n",
						TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */

					/* Empty display structure */
					empty_temp();

					/* Reset the working curve */
					modify_line_select(TRUE);
					modify_line_enhance();

					State = ModDraw;
					show_mod_state("->", State);
					continue;
					}

				/* Freeze all pending edits so only this one can be undone */
				/* (first time after picking) */
				if (Mfresh)
					{
					if (EditUndoable && edit_posted())
						{
						ignore_partial();
						put_message("edit-freeze");
						accept_mod();	/* Leaves EditCurves alone */
						drawn = FALSE;
						}
					}
				post_partial(TRUE);

				/* Add a mark at each end of curve */
				if (IsNull(Smark0) || IsNull(Smark1))
					{
					if (IsNull(Sbox))
						{
						Sbox = create_line();
						add_point_to_line(Sbox, make_point(-0.5,-0.5));
						add_point_to_line(Sbox, make_point( 0.5,-0.5));
						add_point_to_line(Sbox, make_point( 0.5, 0.5));
						add_point_to_line(Sbox, make_point(-0.5, 0.5));
						add_point_to_line(Sbox, make_point(-0.5,-0.5));
						}
					gxSetupTransform(DnEdit);
					lp     = Mline->numpts - 1;
					Msize  = 0.5 * LmarkSize * gxGetMfact() / 1000;
					Smark0 = create_area("", "", "");
					Smark1 = create_area("", "", "");
					define_area_boundary(Smark0, copy_line(Sbox));
					define_area_boundary(Smark1, copy_line(Sbox));
					scale_line(Smark0->bound->boundary, Msize, Msize);
					scale_line(Smark1->bound->boundary, Msize, Msize);
					translate_line(Smark0->bound->boundary,
								Mline->points[0][X], Mline->points[0][Y]);
					translate_line(Smark1->bound->boundary,
								Mline->points[lp][X], Mline->points[lp][Y]);
					define_lspec(&Smark0->lspec, 0, 0, NULL, False, 2.0, 0.0,
								 (HILITE) 3);
					define_lspec(&Smark1->lspec, 0, 0, NULL, False, 2.0, 0.0,
								 (HILITE) 3);

					add_item_to_metafile(TempMeta, "area",  "b", "", "",
								(ITEM) Smark0);
					add_item_to_metafile(TempMeta, "area",  "b", "", "",
								(ITEM) Smark1);
					present_all();

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_modify_line] Adding mark/mark to TempMeta (%d)\n",
						TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */
					}

				/* Draw a new segment */
				next = modify_line_sculpt();
				if (next == ModSculpt) return drawn;

				/* Move on to next stage */
				State = next;
				show_mod_state("->", State);
				continue;

			/* Modify the curve after sculpting */
			case ModSculptDone:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_modify_line] Modifying\n");
#				endif /* DEBUG */

				/* Modify the curve */
				busy_cursor(TRUE);
				put_message("line-modifying");
				empty_curve(Mpick);
				append_curve(Mpick, Mbuild);

				/* Reposition the labels on this curve */
				if (nlabs > 0)
					{
					for (im=0; im<nlabs; im++)
						{

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[edit_modify_line] smove[%d]: %x  Anchor: %.2f/%.2f\n",
							im, smove[im],
							smove[im]->anchor[X], smove[im]->anchor[Y]);
#						endif /* DEBUG_STRUCTURES */

						spot = smove[im];
						(void) curve_test_point(Mpick, spot->anchor, NullFloat,
									pos, NullInt, NullLogical, NullLogical);
						copy_point(spot->anchor, pos);

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[edit_modify_line] spot: %x  Anchor: %.2f/%.2f\n",
							spot, spot->anchor[X], spot->anchor[Y]);
#						endif /* DEBUG_STRUCTURES */
						}
					}

				/* Register the edit */
				ignore_partial();
				drawn  = TRUE;
				Mdrawn = TRUE;
				Mfresh = FALSE;
				invoke_set_catspecs(EditCurves);
				if (EditUndoable) highlight_set(EditCurves, 1);
				if (EditUndoable) post_mod("curves");

				/* Modify labels if necessary */
				if (recompute_curveset_labs(EditCurves, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				/* Clean up temporary buffers */
				Mwork  = NullCurve;
				Mline  = NullLine;
				Mbuild = NullCurve;
				Smark0 = NullArea;
				Smark1 = NullArea;
				Lmarks = NullAreaList;

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_modify_line] ModSculptDone ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();

				/* Reset the working curve */
				modify_line_select(TRUE);
				modify_line_enhance();

				/* Show the results */
				busy_cursor(FALSE);
				present_all();
				modifying_control(FALSE);

				/* Move on to next stage - Do more sculpting */
				State = ModSculpt;
				show_mod_state("->", State);
				continue;

			/* Confirm current edit */
			case ModConfirm:

				/* Register the edit */
				ignore_partial();
				drawn  = TRUE;
				Mdrawn = TRUE;
				invoke_set_catspecs(EditCurves);
				if (EditUndoable) highlight_set(EditCurves, 1);
				if (EditUndoable) post_mod("curves");

				/* Modify labels if necessary */
				if (recompute_curveset_labs(EditCurves, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				/* Clean up temporary buffers */
				circle_echo(FALSE);
				edit_select(NullCal, FALSE);
				Mfresh = FALSE;
				Mpick  = NullCurve;
				Mwork  = NullCurve;
				Mline  = NullLine;
				Mbuild = NullCurve;
				Smark0 = NullArea;
				Smark1 = NullArea;
				Lmarks = NullAreaList;
				empty_line(Mnew);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_modify_line] ModConfirm ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();
				busy_cursor(FALSE);

				/* Go back to pick stage */
				State = ModPick;
				show_mod_state("->", State);
				continue;

			/* Bail out from current edit */
			case ModBailout:

				pr_warning("Editor",
					"[edit_modify_line] Unsuccessful - Bailing out\n");

				/* Register the edit */
				ignore_partial();
				drawn  = FALSE;
				Mdrawn = FALSE;
				if (EditUndoable)
					{
					post_mod("curves");
					put_message("edit-bail-out");
					(void) sleep(2);
					reject_mod();	/* Must reset EditCurves */
					active_line_fields(TRUE, NewCurves, NewLabs);
					}

				/* Clean up temporary buffers */
				circle_echo(FALSE);
				edit_select(NullCal, FALSE);
				Mfresh = FALSE;
				Mpick  = NullCurve;
				Mwork  = NullCurve;
				Mline  = NullLine;
				Mbuild = NullCurve;
				Smark0 = NullArea;
				Smark1 = NullArea;
				Lmarks = NullAreaList;
				empty_line(Mnew);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_modify_line] ModBailout ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();
				busy_cursor(FALSE);

				/* Go back to pick stage */
				State = ModPick;
				show_mod_state("->", State);
				continue;
			}
		}
	}

/**********************************************************************/

static	MODSTATE	modify_line_pick(void)

	{
	POINT	pos;
	int		butt;

	/* See if there are any curves to modify */
	if (EditCurves->num <= 0)
		{
		put_message("line-no-mod");
		return ModPick;
		}

	/* Keep trying until something gets picked */
	while (TRUE)
		{
		/* Get a point */
		put_message("line-mod-pick");
		if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return ModPick;
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}
		put_message("line-pick");
		if (!pick_Xpoint(DnEdit, 0, pos, &butt)) continue;
		if (!inside_dn_window(DnEdit, pos))
			{
			put_message("edit-outside-map");
			(void) sleep(1);
			continue;
			}

		/* See which curve we picked */
		Mpick = closest_curve(EditCurves, pos, NULL, NULL, NULL);
		if (!Mpick)
			{
			put_message("line-no-pick");
			(void) sleep(1);
			continue;
			}
		Mfresh = TRUE;

		/* Produce a copy of the picked curve to work on */
		put_message("line-picked");
		modify_line_select(FALSE);
		modify_line_enhance();
		present_temp(TRUE);

		return (ModifyMode==MODIFY_PUCK)? ModSculpt: ModDraw;
		}
	}

/**********************************************************************/

static	void	modify_line_select
	(
	LOGICAL	usepick
	)

	{

	if (usepick && NotNull(Mpick))
		{
		return;
		}

	edit_select((NotNull(Mpick))? (CAL) Mpick->attrib: NullCal, FALSE);
	}

/**********************************************************************/

static	void	modify_line_enhance(void)

	{
	int		nn;
	AREA	lmark;

	Mwork = copy_curve(Mpick);
	Mline = Mwork->line;
	condense_line(Mline);

	/* Highlight the picked curve */
	widen_curve(Mwork, 2.0);
	highlight_curve(Mwork, 2);
	add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) Mwork);

	/* Highlight the points on the picked curve for point-by-point draw     */
	/* Note that the first and last points on the picked curve are not done */
	if (ModifyMode == MODIFY_CONT || ModifyMode == MODIFY_PTPT)
		{
		if (IsNull(Lbox))
			{
			Lbox = create_line();
			add_point_to_line(Lbox, make_point( 0.0,-0.5));
			add_point_to_line(Lbox, make_point( 0.5, 0.0));
			add_point_to_line(Lbox, make_point( 0.0, 0.5));
			add_point_to_line(Lbox, make_point(-0.5, 0.0));
			add_point_to_line(Lbox, make_point( 0.0,-0.5));
			}
		Msize = 0.5 * LmarkSize * gxGetMfact() / 1000;
		Lmarks = GETMEM(Lmarks, AREA, Mline->numpts);
		Lmarks[0] = NullArea;
		for (nn=1; nn<Mline->numpts-1; nn++)
			{
			lmark = create_area("", "", "");
			define_area_boundary(lmark, copy_line(Lbox));
			scale_line(lmark->bound->boundary, Msize, Msize);
			translate_line(lmark->bound->boundary,
						Mline->points[nn][X], Mline->points[nn][Y]);
			define_lspec(&lmark->lspec, 0, 0, NULL, False, 2.0, 0.0, (HILITE) 3);
			add_item_to_metafile(TempMeta, "area",  "b", "", "", (ITEM) lmark);
			Lmarks[nn] = lmark;
			}
		Lmarks[nn] = NullArea;
		}

	/* Display the highlighted curve */
	present_temp(TRUE);

#	ifdef DEBUG_STRUCTURES
	pr_diag("Editor",
		"[modify_line_enhance] Adding Mwork to TempMeta (%d)\n",
		TempMeta->numfld);
#	endif /* DEBUG_STRUCTURES */
	}

/**********************************************************************/

static	MODSTATE	modify_line_draw(void)

	{
	POINT	spos, epos, tpos, *origpts, *newpts;
	int		butt;
	int		nlines;
	LINE	*lines;
	int		npo, npn, ips, ipe, ipt;
	LOGICAL	ins0, ine0, ins1, ine1, joins, joine, joint, inl;
	float	dxs, dxe, dists, diste, distt;
	float	spfact, smdist;
	LINE	sseg, eseg;

	/* Make sure there is a curve to modify */
	if (!Mwork)
		{
		put_message("line-none-picked");
		return ModDraw;
		}

	/* Turn on "Set" button if required */
	edit_select((CAL) Mpick->attrib, TRUE);

	/* Repeat until we get a new segment or another choice */
	while (TRUE)
		{
		/* Draw a new segment */
		put_message("line-mod-draw");
		post_partial(TRUE);
		if (ready_Xcurve()) butt = LeftButton;
		else if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return ModDraw;

		/* Invalid button */
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}

		/* Draw the curve */
		put_message("line-mod-draw-rel");
		edit_select(NullCal, FALSE);
		drawing_control(TRUE);
		if (!ready_Xcurve())
			{
			Mstart = TRUE;
			if (drawing_Xcurve()) return ModDraw;
			(void) draw_Xcurve(DnEdit, FilterRes, SplineRes, FALSE);
			if (!ready_Xcurve()) return ModDraw;
			}
		Mstart = FALSE;
		drawing_control(FALSE);
		edit_select((CAL) Mpick->attrib, TRUE);

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
		modifying_control(TRUE);

#		ifdef DEBUG
		pr_diag("Editor", "[edit_modify_line] Analysing new segment\n");
#		endif /* DEBUG */

		/* A new segment has been drawn */
		busy_cursor(TRUE);
		put_message("line-mod-proc");

		/* Extract new segment and clean up the buffer */
		if (!Mnew) Mnew = create_line();
		else       empty_line(Mnew);
		append_line(Mnew, lines[0]);
		reset_pipe();

		/* Set smoothing options */
		spfact = ModifySmth;
		spfact = MAX(spfact, 0.0);
		spfact = MIN(spfact, 100.0);

		/* Set parameters for original line and new segment */
		origpts = Mline->points;
		npo     = Mline->numpts;
		newpts  = Mnew->points;
		npn     = Mnew->numpts;

		/* Is start/end of new segment inside start marker for original line? */
		area_test_point(Smark0, newpts[0], NullFloat, NullPoint,
					(AMEMBER *) 0, NullInt, NullInt, &ins0);
		area_test_point(Smark0, newpts[npn-1], NullFloat, NullPoint,
					(AMEMBER *) 0, NullInt, NullInt, &ine0);

		/* Is start/end of new segment inside end marker for original line? */
		area_test_point(Smark1, newpts[0], NullFloat, NullPoint,
					(AMEMBER *) 0, NullInt, NullInt, &ins1);
		area_test_point(Smark1, newpts[npn-1], NullFloat, NullPoint,
					(AMEMBER *) 0, NullInt, NullInt, &ine1);

		/* Extend start of original line */
		if ( ins0 || ine0 )
			{

			/* Start of new segment connects close to start of original line */
			/*  ... so reverse new segment                                   */
			if ( ins0 )
				{
				reverse_line(Mnew);

#				ifdef DEBUG
				pr_diag("Editor",
					"[modify_line_draw] Reverse new segment\n");
#				endif /* DEBUG */
				}

			/* Join end of new segment to start of original line */
			joins = FALSE;
			joine = TRUE;
			ipe   = 0;
			copy_point(epos, origpts[0]);

#			ifdef DEBUG
			pr_diag("Editor",
				"[modify_line_draw] Extend start of original line\n");
#			endif /* DEBUG */
			}

		/* Extend end of original line */
		else if ( ins1 || ine1 )
			{

			/* End of new segment connects close to end of original line */
			/*  ... so reverse new segment                               */
			if ( ine1 )
				{
				reverse_line(Mnew);

#				ifdef DEBUG
				pr_diag("Editor",
					"[modify_line_draw] Reverse new segment\n");
#				endif /* DEBUG */
				}

			/* Join start of new segment to end of original line */
			joins = TRUE;
			joine = FALSE;
			ips   = npo-2;
			copy_point(spos, origpts[npo-1]);

#			ifdef DEBUG
			pr_diag("Editor",
				"[modify_line_draw] Extend end of original line\n");
#			endif /* DEBUG */
			}

		/* Modify interior of original line */
		else
			{

			/* Is start of new segment inside point markers for original line? */
			/* Note that start/end of original line was tested above */
			for (ips=1; ips<Mline->numpts-1; ips++)
				{
				area_test_point(Lmarks[ips], newpts[0], NullFloat, NullPoint,
							(AMEMBER *) 0, NullInt, NullInt, &inl);
				if (inl)
					{
					copy_point(spos, origpts[ips]);
					joins = TRUE;
					break;
					}
				}

			/* Start of new segment not inside markers for original line! */
			/* Find closest point on original line to start of new line */
			/* Decide whether to join at this point if close enough */
			if (ips >= Mline->numpts-1)
				{
				line_test_point(Mline, newpts[0], &dists, spos, &ips,
									NullLogical, NullLogical);
				joins = (LOGICAL) (dists <= SplineRes);
				}

			/* Is end of new segment inside point markers for original line? */
			/* Note that start/end of original line was tested above */
			for (ipe=1; ipe<Mline->numpts-1; ipe++)
				{
				area_test_point(Lmarks[ipe], newpts[npn-1], NullFloat, NullPoint,
							(AMEMBER *) 0, NullInt, NullInt, &inl);
				if (inl)
					{
					copy_point(epos, origpts[ipe]);
					joine = TRUE;
					break;
					}
				}

			/* End of new segment not inside markers for original line! */
			/* Find closest point on original boundary to end of new line */
			/* Decide whether to join at this point if close enough */
			if (ipe >= Mline->numpts-1)
				{
				line_test_point(Mline, newpts[npn-1], &diste, epos, &ipe,
									NullLogical, NullLogical);
				joine = (LOGICAL) (diste <= SplineRes);
				}

			/* Determine distances if intersection on same span */
			if (ips == ipe)
				{
				dxs = point_dist(spos, Mline->points[ips]);
				dxe = point_dist(epos, Mline->points[ipe]);
				}

			/* Force new line to match the sense of the original line */
			if ( ((joins && joine) || (!joins && !joine))
					&& (ips > ipe || (ips == ipe && dxs > dxe)) )
				{

				/* Flip sense of new line segment */
				ipt = ips;	distt = dists;	joint = joins;
				copy_point(tpos, spos);
				ips = ipe;	dists = diste;	joins = joine;
				copy_point(spos, epos);
				ipe = ipt;	diste = distt;	joine = joint;
				copy_point(epos, tpos);
				reverse_line(Mnew);

#				ifdef DEBUG
				pr_diag("Editor", "[modify_line_draw] Reversing new segment\n");
#				endif /* DEBUG */
				}
			}

		/* Break the original line into the before and after parts */
		sseg = NullLine;
		eseg = NullLine;
		if (joins)
			{
			sseg = create_line();
			append_line_portion(sseg, Mline, 0, ips);
			add_point_to_line(sseg, spos);
			condense_line(sseg);
			}
		if (joine)
			{
			eseg = create_line();
			add_point_to_line(eseg, epos);
			append_line_portion(eseg, Mline, ipe+1, npo-1);
			condense_line(eseg);
			}

		/* Take care of smoothly joining the new segment to one or both ends */
		/* of the original curve, depending on smoothing option selected */
		if (spfact > 1.0)
			{
			/* Back up a set distance from the ends of each segment to */
			/* make room for smoothing */
			/* (Move back between 0 and 2 spline steps) */
			smdist = SplineRes * (spfact*0.02);
			(void) trunc_line(Mnew, smdist, joins, joine);
			if (joins) (void) trunc_line(sseg, smdist, FALSE, TRUE);
			if (joine) (void) trunc_line(eseg, smdist, TRUE, FALSE);
			}

		/* Construct modified curve from start and end of original line with */
		/* new line in the middle */
		Mbuild = copy_curve(Mwork);
		empty_curve(Mbuild);
		if (spfact > 1.0)
			{
			float	fres = FilterRes/4;
			if (joins)
				{
				append_line(Mbuild->line, sseg);
				smjoin_lines(Mbuild->line, Mnew, fres, SplineRes);
				}
			else append_line(Mbuild->line, Mnew);
			if (joine) smjoin_lines(Mbuild->line, eseg, fres, SplineRes);
			}
		else
			{
			if (joins && joine)
				{
				append_line(Mbuild->line, sseg);
				append_line_portion(Mbuild->line, Mnew, 1, Mnew->numpts-2);
				append_line(Mbuild->line, eseg);
				}
			else if (joins)
				{
				append_line(Mbuild->line, sseg);
				append_line_portion(Mbuild->line, Mnew, 1, Mnew->numpts-1);
				}
			else if (joine)
				{
				append_line_portion(Mbuild->line, Mnew, 0, Mnew->numpts-2);
				append_line(Mbuild->line, eseg);
				}
			else
				{
				append_line(Mbuild->line, Mnew);
				}
			}
		sseg = destroy_line(sseg);
		eseg = destroy_line(eseg);

		/* Highlight the new curve */
		highlight_curve(Mbuild, 3);

		add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) Mbuild);

#		ifdef DEBUG_STRUCTURES
		pr_diag("Editor",
			"[modify_line_draw] Adding Mbuild to TempMeta (%d)\n",
			TempMeta->numfld);
#		endif /* DEBUG_STRUCTURES */

		/* Show the results */
		present_all();
		busy_cursor(FALSE);
		clear_message();
		return ModDiscard;
		}
	}

/**********************************************************************/

static	MODSTATE	modify_line_discard(void)

	{
	int		butt;

	/* Make sure there is something to discard */
	if (!Mbuild)
		{
		put_message("line-no-discard");
		return ModDiscard;
		}

	/* Repeat until we have a selection */
	while (TRUE)
		{

		/* Get a point */
		put_message("line-mod-discard");
		if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return ModDiscard;

		/* Wait for a "modify_confirm" */
		(void) ignore_Xpoint();
		continue;
		}
	}

/**********************************************************************/

static	MODSTATE	modify_line_sculpt(void)

	{
	int		butt;
	POINT	pos;
	LOGICAL	in0, in1, right;

	/* Make sure there is a curve to modify */
	if (!Mwork)
		{
		put_message("line-none-picked");
		return ModSculpt;
		}

	if (NotNull(Mpick))
		{
		EdTol = SplineRes;
		calc_puck(&EdRad, &EdSpr);
		gxSetupTransform(DnEdit);
		define_circle_echo(EdRad, EdRad+EdSpr);
		circle_echo(TRUE);
		}

	/* Repeat until we get a new segment or another choice */
	while (TRUE)
		{
		/* Draw a new segment */
		circle_echo(TRUE);
		put_message("line-sculpt");
		if (!ready_Xpoint(DnEdit, pos, &butt))
			{
			circle_echo(TRUE);
			return ModSculpt;
			}

		/* Invalid button */
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}

		/* Is the cursor inside one of the end of curve markers? */
		area_test_point(Smark0, pos, NullFloat, NullPoint,
					(AMEMBER *) 0, NullInt, NullInt, &in0);
		area_test_point(Smark1, pos, NullFloat, NullPoint,
					(AMEMBER *) 0, NullInt, NullInt, &in1);

		/* Warning for overlapping end of curve markers */
		if (in0 && in1)
			{
			put_message("line-sculpt-ovlp");
			(void) sleep(1);
			}

		/* Edit the curve */
		put_message("line-sculpt-rel");
		Mbuild = copy_curve(Mwork);
		circle_echo(FALSE);

		/* Shorten/lengthen the curve */
		if (in0 || in1)
			{
			if (!uextend_Xcurve(DnEdit, Mbuild->line, in0, EdTol, EdSpr, &butt))
				{
				if (Mbuild->line->numpts <= 1)
					{
					/* Delete the curve from the curve set */
					put_message("line-deleting");
					Mbuild = destroy_curve(Mbuild);
					(void) sleep(1);
					return ModDelete;
					}
				else
					{
					put_message("edit-no-edit");
					Mbuild = destroy_curve(Mbuild);
					(void) sleep(1);
					continue;
					}
				}
			}

		/* Invoke the sculpting tool */
		else
			{
			if (!uedit_Xcurve(DnEdit, Mbuild->line, NullSubArea,
					EdRad, EdSpr, &butt, &right))
				{
				if (Mbuild->line->numpts <= 1)
					{
					/* Delete the curve from the curve set */
					put_message("line-deleting");
					Mbuild = destroy_curve(Mbuild);
					(void) sleep(1);
					return ModDelete;
					}
				else
					{
					put_message("edit-no-edit");
					Mbuild = destroy_curve(Mbuild);
					(void) sleep(1);
					continue;
					}
				}
			}

		/* Show the results */
		clear_message();
		return ModSculptDone;
		}
	}

/***********************************************************************
*                                                                      *
*     m e r g e _ l i n e                                              *
*                                                                      *
***********************************************************************/

LOGICAL	edit_merge_line

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
	CURVE	curve, copy, tcurve;
	SPOT	spot;
	SET		set, tset;
	CURVE	outline;
	int		nlines;
	LINE	*lines;
	int		im, imx, il, ilx;
	POINT	p0, p1, pz;
	MARK	mark0, mark1;
	CURVE	curv0, curv1;
	float	dx, dy, dx0, dy0, angle, cang, sang, tdist;
	int		butt;
	LOGICAL	drawn = FALSE;

	STRING			dpath;
	FLD_DESCRIPT	fdesc;

	/* Current state: */
	/* - where should we resume? */
	static	enum	{ Wait, Fetch, ReFetch,
						Pick, PickAll, Draw, DrawPick, RePick,
						WaitAction, Merge, Translate, TranslateDone,
						Centre, Rotate, RotateDone,
						ReDisplay, ReDisplayAction, ReStart } State = Wait;

	/* Is a fetch being done? */
	static	LOGICAL		fetch   = FALSE;

	/* Has a merge or translate or rotate been started or completed? */
	static	LOGICAL		mstart  = FALSE;
	static	LOGICAL		maction = FALSE;

	/* Should labels be merged too? */
	static	LOGICAL		mlabels = FALSE;
	static	LOGICAL		mlfirst = TRUE;

	/* Move list: */
	/* - list of curves and curve labels to be moved */
	/* - cmove/smove are pointers into merge field sets (before merge) */
	/*    and then copies to be displayed in TempMeta (after merge)    */
	/* - ccopy/scopy are copies to be added to EditCurves/EditLabs */
	static	const int	maxmove = 10000;
	static	int			nmove   = 0;
	static	int			amove   = 0;
	static	CURVE		*cmove  = NullCurveList;
	static	CURVE		*ccopy  = NullCurveList;
	static	int			nlabs   = 0;
	static	int			alabs   = 0;
	static	SPOT		*smove  = NullSpotList;
	static	SPOT		*scopy  = NullSpotList;
	static	int			*sids   = NullInt;

	/* Centre of rotation */
	static	POINT		Cpos    = ZERO_POINT;
	static	float		Ang     = 0.0;

	/* Field to merge from */
	static	STRING	msource  = NullString;
	static	STRING	msubsrc  = NullString;
	static	STRING	mrtime   = NullString;
	static	STRING	mvtime   = NullString;
	static	STRING	melement = NullString;
	static	STRING	mlevel   = NullString;
	static	int		mformat  = DisplayFormatSimple;
	static	SET		mset     = NullSet;
	static	SET		sset     = NullSet;
	static	LOGICAL	fullset  = FALSE;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_merge_line] %s  State: %d\n", mode, State);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same(mode, "cancel"))
		{
		put_message("edit-cancel");
		drawing_control(FALSE);
		switch (State)
			{
			case Translate:
			case Centre:
			case Rotate:	ignore_partial();
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

							/* Empty picked curve list */
							if (nmove > 0)
								{
								if (mstart && !maction)
									{
									for (im=0; im<nmove; im++)
										(void) remove_item_from_set(EditCurves,
															 (ITEM) ccopy[im]);
									}
								else if (!maction)
									{
									for (im=0; im<nmove; im++)
										(void) destroy_curve(ccopy[im]);
									}
								nmove = 0;
								amove = 0;
								FREEMEM(cmove);
								FREEMEM(ccopy);
								}
							edit_select(NullCal, FALSE);

							/* Empty curve label list */
							if (nlabs > 0)
								{
								if (mstart && !maction)
									{
									for (il=0; il<nlabs; il++)
										(void) remove_item_from_set(EditLabs,
															 (ITEM) scopy[il]);
									}
								else if (!maction)
									{
									for (il=0; il<nlabs; il++)
										(void) destroy_spot(scopy[il]);
									}
								nlabs = 0;
								alabs = 0;
								FREEMEM(smove);
								FREEMEM(scopy);
								FREEMEM(sids);
								}

							/* Get rid of picked curves and fetched field */
							if (fullset)
								{
								mset = take_mf_set(TempMeta, "curve", "c",
												   "mset", "mset");
								sset = take_mf_set(TempMeta, "spot", "d",
												   "sset", "sset");
								}
							if (NotNull(mset))
								{
								mset = destroy_set(mset);
								sset = destroy_set(sset);
								FREEMEM(msource);
								FREEMEM(msubsrc);
								FREEMEM(mrtime);
								FREEMEM(mvtime);
								FREEMEM(melement);
								FREEMEM(mlevel);
								mformat = DisplayFormatSimple;
								}

#							ifdef DEBUG_STRUCTURES
							pr_diag("Editor",
								"[edit_merge_line] Cancel ... empty TempMeta %d fields\n",
								TempMeta->numfld);
#							endif /* DEBUG_STRUCTURES */

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

		/* Empty picked curve list and curve label list */
		if (nmove > 0)
			{
			if (mstart && !maction)
				{
				for (im=0; im<nmove; im++)
					(void) remove_item_from_set(EditCurves, (ITEM) ccopy[im]);
				}
			else if (!maction)
				{
				for (im=0; im<nmove; im++) (void) destroy_curve(ccopy[im]);
				}
			nmove = 0;
			amove = 0;
			FREEMEM(cmove);
			FREEMEM(ccopy);
			}
		if (nlabs > 0)
			{
			if (mstart && !maction)
				{
				for (il=0; il<nlabs; il++)
					(void) remove_item_from_set(EditLabs, (ITEM) scopy[il]);
				}
			else if (!maction)
				{
				for (il=0; il<nlabs; il++) (void) destroy_spot(scopy[il]);
				}
			nlabs = 0;
			alabs = 0;
			FREEMEM(smove);
			FREEMEM(scopy);
			FREEMEM(sids);
			}

		/* Get rid of displayed picked curves and fetched field */
		if (fullset)
			{
			mset = take_mf_set(TempMeta, "curve", "c", "mset", "mset");
			sset = take_mf_set(TempMeta, "spot",  "d", "sset", "sset");
			}
		if (NotNull(mset))
			{
			mset = destroy_set(mset);
			sset = destroy_set(sset);
			FREEMEM(msource);
			FREEMEM(msubsrc);
			FREEMEM(mrtime);
			FREEMEM(mvtime);
			FREEMEM(melement);
			FREEMEM(mlevel);
			mformat = DisplayFormatSimple;
			}

#		ifdef DEBUG_STRUCTURES
		pr_diag("Editor",
			"[edit_merge_line] Cancel All ... empty TempMeta %d fields\n",
			TempMeta->numfld);
#		endif /* DEBUG_STRUCTURES */

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

	/* Make sure the curve set contains the right stuff */
	if (!EditCurves)                      return FALSE;
	if (!same(EditCurves->type, "curve")) return FALSE;

	/* Initialize on startup */
	if (same(mode, "begin"))
		{
		ignore_partial();

		/* Set up move list */
		nmove = 0;
		amove = 0;
		FREEMEM(cmove);
		FREEMEM(ccopy);
		nlabs = 0;
		alabs = 0;
		FREEMEM(smove);
		FREEMEM(scopy);
		FREEMEM(sids);

		/* Get rid of displayed picked curves and fetched field */
		if (fullset)
			{
			mset = take_mf_set(TempMeta, "curve", "c", "mset", "mset");
			sset = take_mf_set(TempMeta, "spot",  "d", "sset", "sset");
			}
		if (NotNull(mset))
			{
			mset = destroy_set(mset);
			sset = destroy_set(sset);
			FREEMEM(msource);
			FREEMEM(msubsrc);
			FREEMEM(mrtime);
			FREEMEM(mvtime);
			FREEMEM(melement);
			FREEMEM(mlevel);
			mformat = DisplayFormatSimple;
			}

#		ifdef DEBUG_STRUCTURES
		pr_diag("Editor",
			"[edit_merge_line] Begin ... empty TempMeta %d fields\n",
			TempMeta->numfld);
#		endif /* DEBUG_STRUCTURES */

		/* Empty display structure */
		empty_temp();
		present_all();
		mstart  = FALSE;
		maction = FALSE;
		mlfirst = TRUE;

		edit_select(NullCal, FALSE);
		State = Wait;
		}

	/* Check if merge mode has changed */
	if ( (mlabels && MergeMode == MERGE_FIELD)
			|| (!mlabels && MergeMode == MERGE_FIELD_AND_LABELS) )
		{

		/* Cannot change merge mode if current merge is unresolved! */
		if (mstart || maction)
			{

			/* Display warning only once! */
			if (mlfirst)
				{
				put_message("line-merge-nomode");
				(void) sleep(1);
				}
			mlfirst = FALSE;
			}

		/* Change merge mode and remove labels */
		else if (mlabels)
			{

			/* Un-highlight each curve label and remove it from the list */
			for (il=0; il<nlabs; il++)
				{
				highlight_item("spot", (ITEM) smove[il], 4);
				(void) destroy_spot(scopy[il]);
				}
			nlabs = 0;
			alabs = 0;
			FREEMEM(smove);
			FREEMEM(scopy);
			FREEMEM(sids);

			/* Re-display the picked curves */
			present_all();

			/* Reset check for merge mode */
			mlabels = FALSE;
			mlfirst = TRUE;
			}

		/* Change merge mode and add labels */
		else
			{

			/* Match current list of curve labels to  */
			/*  picked lines and add them to the list */
			set  = find_mf_set(TempMeta, "curve", "c", "mset", "mset");
			tset = find_mf_set(TempMeta, "spot",  "d", "sset", "sset");

			/* Loop through all labels in set */
			if (nmove > 0 && NotNull(tset))
				{
				for (ilx=0; ilx<tset->num; ilx++)
					{
					spot = (SPOT) tset->list[ilx];
					if (!spot) continue;
					if (same(spot->mclass, "legend")) continue;

					/* Check labels attached to picked curve */
					curve = closest_curve(set, spot->anchor,
										  &tdist, NULL, NULL);
					if (tdist > PickTol) continue;

					for (im=0; im<nmove; im++)
						{
						if (curve == cmove[im]) break;
						}
					if (im < nmove)
						{

						/* Highlight each curve label */
						/*  and add it to the list    */
						nlabs++;
						if (nlabs > alabs)
							{
							alabs = nlabs;
							smove = GETMEM(smove, SPOT, alabs);
							scopy = GETMEM(scopy, SPOT, alabs);
							sids  = GETMEM(sids,  int,  alabs);
							}
						highlight_item("spot", (ITEM) spot, 2);
						smove[nlabs-1] = spot;
						scopy[nlabs-1] = copy_spot(spot);
						sids[nlabs-1]  = im;
						}
					}
				}

			/* Re-display the picked curves and curve labels */
			present_all();

			/* Reset check for merge mode */
			mlabels = TRUE;
			mlfirst = TRUE;
			}
		}

	/* Set state for "fetch" */
	if (same(mode, "fetch"))
		{
		/* Fetch also cancels */
		switch (State)
			{

			/* Cannot ReFetch until current merge is resolved! */
			case Translate:
			case Centre:
			case Rotate:
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
			case Pick:		State = PickAll;
							break;
			}
		}

	/* Set state for Right button "merge" or "translate" or "rotate" */
	if ( (same(mode, "merge") || same(mode, "translate")
			|| same(mode, "rotate")) && nmove <= 0 )
		{
		put_message("line-no-pick");
		(void) sleep(1);
		edit_select(NullCal, FALSE);
		State = Pick;
		return FALSE;
		}
	if (same(mode, "merge"))     State = Merge;
	if (same(mode, "translate")) State = Translate;
	if (same(mode, "rotate"))    State = Centre;

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
					"[edit_merge_line] No outline drawn yet!\n");
				}
			else
				{
				put_message("edit-no-preset-outline", source);
				pr_error("Editor",
					"[edit_merge_line] No preset map outline: \"%s\"\n",
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

#			ifdef DEBUG_STRUCTURES
			pr_diag("Editor",
				"[edit_merge_line] Preset Outline ... adding outline to TempMeta (%d)\n",
				TempMeta->numfld);
#			endif /* DEBUG_STRUCTURES */

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

				/* Keep returning until we get a "Fetch" */
				put_message("merge-field");
				edit_allow_preset_outline(FALSE);
				return FALSE;

			/* Wait around for a "merge" or "translate" or "rotate" */
			case WaitAction:

				/* Keep returning until we get a "merge" or "translate" or "rotate" */
				put_message("line-merge-action");
				edit_allow_preset_outline(FALSE);
				return FALSE;

			/* Fetch the requested merge field */
			case Fetch:

				fetch   = FALSE;
				mstart  = FALSE;
				maction = FALSE;
				mlfirst = TRUE;

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
				pr_diag("Editor", "[edit_merge_line] Fetching merge field\n");
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
					sset = destroy_set(sset);
					FREEMEM(msource);
					FREEMEM(msubsrc);
					FREEMEM(mrtime);
					FREEMEM(mvtime);
					FREEMEM(melement);
					FREEMEM(mlevel);
					mformat = DisplayFormatSimple;
					}
				mset = retrieve_curveset(&fdesc);
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
					/* No curves to merge */
					put_message("line-merge-none");
					(void) sleep(2);
					State = Wait;
					continue;
					}

				/* Got it - prepare set of curves for display */
				fullset  = TRUE;
				msource  = STRMEM(msource,  source);
				msubsrc  = STRMEM(msubsrc,  subsrc);
				mrtime   = STRMEM(mrtime,   rtime);
				mvtime   = STRMEM(mvtime,   vtime);
				melement = STRMEM(melement, element);
				mlevel   = STRMEM(mlevel,   level);
				mformat  = field_display_format(element, level);

				/* Get the labels for the fetched field */
				(void) set_fld_descript(&fdesc,
					FpaF_FIELD_MACRO,       FpaC_SCATTERED,
					FpaF_END_OF_LIST);
				sset = retrieve_spotset(&fdesc);

				/* Display the fetched field and labels */
				/* >>>>> modify field units for CurrElement/CurrLevel?? <<<<< */
				prep_set(mset, mformat);
				setup_set_presentation(mset, CurrElement, CurrLevel, "FPA");
				highlight_set(mset, 4);
				setup_set_presentation(sset, CurrElement, CurrLevel, "FPA");
				highlight_set(sset, 4);

				add_set_to_metafile(TempMeta, "c", "mset", "mset", mset);
				add_set_to_metafile(TempMeta, "d", "sset", "sset", sset);
				present_temp(TRUE);
				post_partial(TRUE);
				clear_message();

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_merge_line] Fetch ... adding mset/sset to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Re-fetch the requested merge field */
			case ReFetch:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_line] ReFetch\n");
#				endif /* DEBUG */

				ignore_partial();

				/* Empty picked curve list */
				if (nmove > 0)
					{
					for (im=0; im<nmove; im++) (void) destroy_curve(ccopy[im]);
					nmove = 0;
					amove = 0;
					FREEMEM(cmove);
					FREEMEM(ccopy);
					}
				edit_select(NullCal, FALSE);

				/* Empty curve label list */
				if (nlabs > 0)
					{
					for (il=0; il<nlabs; il++) (void) destroy_spot(scopy[il]);
					nlabs = 0;
					alabs = 0;
					FREEMEM(smove);
					FREEMEM(scopy);
					FREEMEM(sids);
					}

				/* Get rid of displayed picked curves and fetched field */
				if (fullset)
					{
					mset = take_mf_set(TempMeta, "curve", "c", "mset", "mset");
					sset = take_mf_set(TempMeta, "spot",  "d", "sset", "sset");
					}
				if (NotNull(mset))
					{
					mset = destroy_set(mset);
					sset = destroy_set(sset);
					FREEMEM(msource);
					FREEMEM(msubsrc);
					FREEMEM(mrtime);
					FREEMEM(mvtime);
					FREEMEM(melement);
					FREEMEM(mlevel);
					mformat = DisplayFormatSimple;
					}

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_merge_line] Refetch ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();
				present_all();

				/* Wait for a "fetch" */
				State = (fetch)? Fetch: Wait;
				continue;

			/* Pick curves to merge */
			case Pick:

				/* Use the current list of curves to pick from */
				set = find_mf_set(TempMeta, "curve", "c", "mset", "mset");

				/* Allow specification of multiple moves */
				while (nmove < maxmove)
					{

					/* Put up a prompt and set states */
					if (nmove <= 0)
						{
						put_message("line-merge-pick");
						edit_select(NullCal, FALSE);
						}
					else
						{
						put_message("line-merge-pick2");
						edit_select((CAL) cmove[0]->attrib, FALSE);
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
					put_message("line-pick");
					if (!pick_Xpoint(DnEdit, 0, p1, &butt)) continue;
					if (!inside_dn_window(DnEdit, p1))
						{
						put_message("edit-outside-map");
						(void) sleep(1);
						continue;
						}

					/* See which curve we picked */
					curve = closest_curve(set, p1, NULL, NULL, NULL);
					if (!curve)
						{
						put_message("line-no-pick");
						(void) sleep(1);
						continue;
						}

					/* If curve was already picked, un-pick it */
					for (im=0; im<nmove; im++)
						{
						if (curve == cmove[im]) break;
						}
					if (im < nmove)
						{
						put_message("line-unpicked");

						/* Un-highlight the picked curve */
						/*  and remove it from the list  */
						highlight_curve(curve, 4);
						widen_curve(curve, -2.0);
						(void) destroy_curve(ccopy[im]);
						nmove--;
						for (imx=im; imx<nmove; imx++)
							{
							cmove[imx] = cmove[imx+1];
							ccopy[imx] = ccopy[imx+1];
							}
						cmove[imx] = NullCurve;
						ccopy[imx] = NullCurve;

						/* Remove labels attached to this curve */
						if (mlabels)
							{
							for (il=nlabs-1; il>=0; il--)
								{
								if (sids[il] < im)
									continue;
								else if (sids[il] > im)
									sids[il]--;
								else
									{
									/* Un-highlight the curve label */
									/*  and remove it from the list */
									highlight_item("spot", (ITEM) smove[il], 4);
									(void) destroy_spot(scopy[il]);
									nlabs--;
									for (ilx=il; ilx<nlabs; ilx++)
										{
										smove[ilx] = smove[ilx+1];
										scopy[ilx] = scopy[ilx+1];
										sids[ilx]  = sids[ilx+1];
										}
									smove[ilx] = NullSpot;
									scopy[ilx] = NullSpot;
									sids[ilx]  = 0;
									}
								}
							}

						/* Display remaining picked curves and curve labels */
						present_all();
						}

					/* Otherwise pick it */
					else
						{
						put_message("line-picked");
						nmove++;
						if (nmove > amove)
							{
							amove = nmove;
							cmove = GETMEM(cmove, CURVE, amove);
							ccopy = GETMEM(ccopy, CURVE, amove);
							}

						/* Highlight the picked curve */
						/*  and add it to the list */
						ccopy[im] = copy_curve(curve);
						highlight_curve(curve, 2);
						widen_curve(curve, 2.0);
						cmove[im] = curve;

						/* Check for labels attached to picked curve */
						if (mlabels)
							{
							tset = find_mf_set(TempMeta, "spot",  "d",
											   "sset", "sset");
							if (NotNull(tset))
								{
								for (ilx=0; ilx<tset->num; ilx++)
									{
									spot = (SPOT) tset->list[ilx];
									if (!spot) continue;
									if (same(spot->mclass, "legend")) continue;

									/* Check labels attached to picked curve */
									tcurve = closest_curve(set, spot->anchor,
														   &tdist, NULL, NULL);
									if (tdist > PickTol) continue;

									if (tcurve == curve)
										{

										/* Highlight the curve label */
										/*  and add it to the list   */
										nlabs++;
										if (nlabs > alabs)
											{
											alabs = nlabs;
											smove = GETMEM(smove, SPOT, alabs);
											scopy = GETMEM(scopy, SPOT, alabs);
											sids  = GETMEM(sids,  int,  alabs);
											}
										highlight_item("spot", (ITEM) spot, 2);
										smove[nlabs-1] = spot;
										scopy[nlabs-1] = copy_spot(spot);
										sids[nlabs-1]  = im;
										}
									}
								}
							}

						/* Display picked curves and curve labels */
						present_temp(TRUE);
						}
					}

				pr_warning("Editor",
					"[edit_merge_line]   End of State = Pick loop!\n");

				/* Have we picked any curves? */
				if (nmove > 0) edit_select((CAL) cmove[0]->attrib, FALSE);
				else           edit_select(NullCal, FALSE);

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Pick all curves to merge */
			case PickAll:

				put_message("line-all-picked");

				/* Un-highlight picked curves and remove them from the list */
				for (im=0; im<nmove; im++)
					{
					highlight_curve(cmove[im], 4);
					widen_curve(cmove[im], -2.0);
					(void) destroy_curve(ccopy[im]);
					}

				/* Un-highlight curve labels and remove them from the list */
				if (mlabels)
					{
					for (il=0; il<nlabs; il++)
						{
						highlight_item("spot", (ITEM) smove[il], 4);
						(void) destroy_spot(scopy[il]);
						}
					}

				/* Highlight all the curves and add them to the list */
				set = find_mf_set(TempMeta, "curve", "c", "mset", "mset");
				nmove = set->num;
				if (nmove > amove)
					{
					amove = nmove;
					cmove = GETMEM(cmove, CURVE, amove);
					ccopy = GETMEM(ccopy, CURVE, amove);
					}
				for (im=0; im<nmove; im++)
					{
					curve     = (CURVE) set->list[im];
					ccopy[im] = copy_curve(curve);
					highlight_curve(curve, 2);
					widen_curve(curve, 2.0);
					cmove[im] = curve;
					}

				/* Match labels to picked curves and add them to the list */
				if (nmove > 0 && mlabels)
					{
					tset = find_mf_set(TempMeta, "spot",  "d", "sset", "sset");

					/* Loop through all labels in set */
					nlabs = 0;
					if (NotNull(tset))
						{
						for (ilx=0; ilx<tset->num; ilx++)
							{
							spot = (SPOT) tset->list[ilx];
							if (!spot) continue;
							if (same(spot->mclass, "legend")) continue;

							/* Check labels attached to picked curves */
							tcurve = closest_curve(set, spot->anchor,
												   &tdist, NULL, NULL);
							if (tdist > PickTol) continue;

							for (im=0; im<nmove; im++)
								{
								if (tcurve == cmove[im]) break;
								}
							if (im < nmove)
								{

								/* Highlight each curve label */
								/*  and add it to the list    */
								nlabs++;
								if (nlabs > alabs)
									{
									alabs = nlabs;
									smove = GETMEM(smove, SPOT, alabs);
									scopy = GETMEM(scopy, SPOT, alabs);
									sids  = GETMEM(sids,  int,  alabs);
									}
								highlight_item("spot", (ITEM) spot, 2);
								smove[nlabs-1] = spot;
								scopy[nlabs-1] = copy_spot(spot);
								sids[nlabs-1]  = im;
								}
							}
						}
					}

				/* Display picked curves and curve labels */
				present_temp(TRUE);

				/* Have we picked any curves? */
				if (nmove > 0) edit_select((CAL) cmove[0]->attrib, FALSE);
				else           edit_select(NullCal, FALSE);

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Draw outline around curves to merge */
			case Draw:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_line] Drawing outline\n");
#				endif /* DEBUG */

				/* Draw the outline */
				post_partial(FALSE);
				edit_allow_preset_outline(FALSE);
				put_message("line-merge-draw");
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

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_merge_line] Draw Outline ... adding outline to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = DrawPick;
				continue;

			/* Pick curves to merge from inside drawn outline */
			case DrawPick:

				if (IsNull(outline))
					{
					edit_select(NullCal, FALSE);
					State = Pick;
					continue;
					}

#				ifdef DEBUG
				pr_diag("Editor",
					"[edit_merge_line] Picking curves inside outline\n");
#				endif /* DEBUG */

				/* Pick curves inside drawn outline */
				busy_cursor(TRUE);
				edit_allow_preset_outline(FALSE);
				put_message("line-merge-select");
				(void) sleep(1);

				/* Use the current list of curves to pick from */
				set = find_mf_set(TempMeta, "curve", "c", "mset", "mset");

				/* Loop through all curves in set */
				for (imx=0; imx<set->num; imx++)
					{
					curve = (CURVE) set->list[imx];
					if (!curve) continue;

					/* Check if curve is inside drawn outline */
					if (!curve_inside_outline(curve, outline)) continue;

					/* Pick curves that are not already picked */
					for (im=0; im<nmove; im++)
						{
						if (curve == cmove[im]) break;
						}
					if (im >= nmove)
						{
						put_message("line-picked");
						nmove++;
						if (nmove > amove)
							{
							amove = nmove;
							cmove = GETMEM(cmove, CURVE, amove);
							ccopy = GETMEM(ccopy, CURVE, amove);
							}

						/* Highlight the picked curve */
						/*  and add it to the list */
						ccopy[im] = copy_curve(curve);
						highlight_curve(curve, 2);
						widen_curve(curve, 2.0);
						cmove[im] = curve;

						/* Check for labels attached to picked curve */
						if (mlabels)
							{
							tset = find_mf_set(TempMeta, "spot",  "d",
											   "sset", "sset");
							if (NotNull(tset))
								{
								for (ilx=0; ilx<tset->num; ilx++)
									{
									spot = (SPOT) tset->list[ilx];
									if (!spot) continue;
									if (same(spot->mclass, "legend")) continue;

									/* Check labels attached to picked curve */
									tcurve = closest_curve(set, spot->anchor,
														   &tdist, NULL, NULL);
									if (tdist > PickTol) continue;

									if (tcurve == curve)
										{

										/* Highlight the curve label */
										/*  and add it to the list   */
										nlabs++;
										if (nlabs > alabs)
											{
											alabs = nlabs;
											smove = GETMEM(smove, SPOT, alabs);
											scopy = GETMEM(scopy, SPOT, alabs);
											sids  = GETMEM(sids,  int,  alabs);
											}
										highlight_item("spot", (ITEM) spot, 2);
										smove[nlabs-1] = spot;
										scopy[nlabs-1] = copy_spot(spot);
										sids[nlabs-1]  = im;
										}
									}
								}
							}
						}
					}

				/* Display picked curves and curve labels */
				present_temp(TRUE);
				busy_cursor(FALSE);

				/* Have we picked any curves? */
				if (nmove > 0) edit_select((CAL) cmove[0]->attrib, FALSE);
				else           edit_select(NullCal, FALSE);

				/* Re-display the picked curves */
				State = ReDisplay;
				continue;

			/* Re-pick curves to merge */
			case RePick:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_line] RePick\n");
#				endif /* DEBUG */

				/* Empty picked curve list */
				if (nmove > 0)
					{
					/* First need to un-widen the picked curves */
					/*  and remove them from the list */
					for (im=0; im<nmove; im++)
						{
						curve = cmove[im];
						widen_curve(curve, -2.0);
						(void) destroy_curve(ccopy[im]);
						}
					nmove = 0;
					amove = 0;
					FREEMEM(cmove);
					FREEMEM(ccopy);
					}
				edit_select(NullCal, FALSE);

				/* Empty curve label list */
				if (nlabs > 0)
					{
					for (il=0; il<nlabs; il++) (void) destroy_spot(scopy[il]);
					nlabs = 0;
					alabs = 0;
					FREEMEM(smove);
					FREEMEM(scopy);
					FREEMEM(sids);
					}

				/* Remove displayed picked curves, but keep the fetched field */
				if (fullset)
					{
					mset = take_mf_set(TempMeta, "curve", "c", "mset", "mset");
					sset = take_mf_set(TempMeta, "spot",  "d", "sset", "sset");
					}

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_merge_line] Repick ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();

				/* Re-display the fetched field */
				fullset = TRUE;
				highlight_set(mset, 4);
				highlight_set(sset, 4);
				add_set_to_metafile(TempMeta, "c", "mset", "mset", mset);
				add_set_to_metafile(TempMeta, "d", "sset", "sset", sset);
				present_all();
				post_partial(TRUE);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_merge_line] Repick ... adding mset/sset to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Merge curves in place */
			case Merge:

				/* Remove displayed fetched field (if required) */
				mstart = TRUE;
				if (fullset)
					{

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_merge_line] Merge ... empty TempMeta %d fields\n",
						TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */

					/* Empty display structure */
					empty_temp();
					fullset = FALSE;
					mset    = NullSet;
					sset    = NullSet;

					/* Now add the picked curves */
					for (im=0; im<nmove; im++)
						{
						(void) add_item_to_set(EditCurves, (ITEM) ccopy[im]);
						curve = copy_curve(ccopy[im]);
						add_item_to_metafile(TempMeta, "curve", "c",
											 "cmerge", "cmerge", (ITEM) curve);
						cmove[im] = curve;

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[edit_merge_line] Adding line (%d) to TempMeta (%d)\n",
							im, TempMeta->numfld);
#						endif /* DEBUG_STRUCTURES */
						}

					/* Then add labels attached to curves (if requested) */
					if (mlabels)
						{
						for (il=0; il<nlabs; il++)
							{
							(void) add_item_to_set(EditLabs, (ITEM) scopy[il]);
							spot = copy_spot(scopy[il]);
							add_item_to_metafile(TempMeta, "spot", "d",
												 "smerge", "smerge", (ITEM) spot);
							smove[il] = spot;

#							ifdef DEBUG_STRUCTURES
							pr_diag("Editor",
								"[edit_merge_line] Adding spot (%d) to TempMeta (%d)\n",
								il, TempMeta->numfld);
#							endif /* DEBUG_STRUCTURES */
							}
						}
					}

				/* Remove any marks from uncompleted actions */
				set = take_mf_set(TempMeta, "mark", "d", "", "");
				if (NotNull(set)) set = destroy_set(set);

#				ifdef DEBUG_STRUCTURES
				for (im=0; im<nmove; im++)
					pr_diag("Editor",
						"[edit_merge_line]   Merge: %2d ... cmove: %x\n",
					im, cmove[im]);
				for (il=0; il<nlabs; il++)
					pr_diag("Editor",
						"[edit_merge_line]   Merge: %2d ... smove/sids: %x %d\n",
					il, smove[il], sids[il]);
#				endif /* DEBUG_STRUCTURES */

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_line] Merging\n");
#				endif /* DEBUG */

				ignore_partial();

				if (EditUndoable) post_mod("curves");

				/* Modify labels if necessary */
				if (recompute_curveset_labs(EditCurves, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				put_message("edit-done");
				mstart  = FALSE;
				maction = TRUE;

				/* Re-display the picked curves and labels */
				State = ReDisplayAction;
				continue;

			/* Pick and translate a reference point */
			case Translate:

				/* Remove displayed fetched field (if required) */
				mstart = TRUE;
				if (fullset)
					{

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_merge_line] Translate ... empty TempMeta %d fields\n",
						TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */

					/* Empty display structure */
					empty_temp();
					fullset = FALSE;
					mset    = NullSet;
					sset    = NullSet;

					/* Now add the picked curves */
					for (im=0; im<nmove; im++)
						{
						(void) add_item_to_set(EditCurves, (ITEM) ccopy[im]);
						curve = copy_curve(ccopy[im]);
						add_item_to_metafile(TempMeta, "curve", "c",
											 "mmerge", "mmerge", (ITEM) curve);
						cmove[im] = curve;

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[edit_merge_line] Adding line (%d) to TempMeta (%d)\n",
							im, TempMeta->numfld);
#						endif /* DEBUG_STRUCTURES */
						}

					/* Then add labels attached to curves (if requested) */
					if (mlabels)
						{
						for (il=0; il<nlabs; il++)
							{
							(void) add_item_to_set(EditLabs, (ITEM) scopy[il]);
							spot = copy_spot(scopy[il]);
							add_item_to_metafile(TempMeta, "spot", "d",
												 "smerge", "smerge", (ITEM) spot);
							smove[il] = spot;

#							ifdef DEBUG_STRUCTURES
							pr_diag("Editor",
							"[edit_merge_line] Adding spot (%d) to TempMeta (%d)\n",
								il, TempMeta->numfld);
#							endif /* DEBUG_STRUCTURES */
							}
						}
					}

				/* Highlight the picked curves and labels */
				for (im=0; im<nmove; im++)
					{
					curve = cmove[im];
					highlight_curve(curve, 3);
					}
				if (mlabels)
					{
					for (il=0; il<nlabs; il++)
						{
						spot = smove[il];
						highlight_item("spot", (ITEM) spot, 3);
						}
					}

				/* Remove any marks from uncompleted actions */
				set = take_mf_set(TempMeta, "mark", "d", "", "");
				if (NotNull(set)) set = destroy_set(set);

				/* Re-display the picked curves and labels */
				present_all();
				post_partial(TRUE);

#				ifdef DEBUG_STRUCTURES
				for (im=0; im<nmove; im++)
					pr_diag("Editor",
						"[edit_merge_line]   Translate: %2d ... cmove: %x\n",
					im, cmove[im]);
				for (il=0; il<nlabs; il++)
					pr_diag("Editor",
						"[edit_merge_line]   Translate: %2d ... smove/sids: %x %d\n",
					il, smove[il], sids[il]);
#				endif /* DEBUG_STRUCTURES */

				/* Pick a reference point */
				put_message("line-tran");
				if (!ready_Xpoint(DnEdit, p0, &butt)) return drawn;
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!inside_dn_window(DnEdit, p0))
					{
					put_message("line-tran-out1");
					(void) ignore_Xpoint();
					continue;
					}

				/* Highlight the curves to translate */
				set = copy_mf_set(TempMeta, "curve", "c", "mmerge", "mmerge");
				for (im=0; im<set->num; im++)
					{
					curve = (CURVE) set->list[im];
					highlight_curve(curve, 2);
					widen_curve(curve, 2.0);
					}
				prep_set(set, mformat);

				/* Translate the reference point */
				put_message("line-tran-release");
				(void) utrack_Xpoint(DnEdit, set, p0, p1, &butt);
				set = destroy_set(set);
				if (!inside_dn_window(DnEdit, p1))
					{
					put_message("line-tran-out2");
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

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_merge_line] Adding mark/mark/curve to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = TranslateDone;
				continue;

			/* Translate the curves */
			case TranslateDone:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_line] Translating\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);

#				ifdef DEBUG_STRUCTURES
				for (im=0; im<nmove; im++)
					pr_diag("Editor",
						"[edit_merge_line]   Translating: %2d ... cmove: %x\n",
					im, cmove[im]);
				for (il=0; il<nlabs; il++)
					pr_diag("Editor",
						"[edit_merge_line]   Translating: %2d ... smove/sids: %x %d\n",
					il, smove[il], sids[il]);
#				endif /* DEBUG_STRUCTURES */

				/* Translate the curves */
				dx = p1[X] - p0[X];
				dy = p1[Y] - p0[Y];
				for (im=0; im<nmove; im++)
					{
					curve = ccopy[im];
					translate_curve(curve, dx, dy);
					}

				/* Then translate labels attached to curves (if requested) */
				if (mlabels)
					{
					(void) spot_list_translate(nlabs, scopy, dx, dy);
					}

				if (EditUndoable) post_mod("curves");

				/* Modify labels if necessary */
				if (recompute_curveset_labs(EditCurves, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				put_message("edit-done");
				mstart  = FALSE;
				maction = TRUE;
				busy_cursor(FALSE);

				/* Re-display the picked curves and labels */
				State = ReDisplayAction;
				continue;

			/* Pick the centre of rotation */
			case Centre:

				/* Remove displayed fetched field (if required) */
				mstart = TRUE;
				if (fullset)
					{

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_merge_line] Rotate ... empty TempMeta %d fields\n",
						TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */

					/* Empty display structure */
					empty_temp();
					fullset = FALSE;
					mset    = NullSet;
					sset    = NullSet;

					/* Now add the picked curves */
					for (im=0; im<nmove; im++)
						{
						(void) add_item_to_set(EditCurves, (ITEM) ccopy[im]);
						curve = copy_curve(ccopy[im]);

						add_item_to_metafile(TempMeta, "curve", "c",
											 "mmerge", "mmerge", (ITEM) curve);
						cmove[im] = curve;

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[edit_merge_line] Adding line (%d) to TempMeta (%d)\n",
							im, TempMeta->numfld);
#						endif /* DEBUG_STRUCTURES */
						}

					/* Then add labels attached to curves (if requested) */
					if (mlabels)
						{
						for (il=0; il<nlabs; il++)
							{
							(void) add_item_to_set(EditLabs, (ITEM) scopy[il]);
							spot = copy_spot(scopy[il]);
							add_item_to_metafile(TempMeta, "spot", "d",
												 "smerge", "smerge", (ITEM) spot);
							smove[il] = spot;

#							ifdef DEBUG_STRUCTURES
							pr_diag("Editor",
							"[edit_merge_line] Adding spot (%d) to TempMeta (%d)\n",
								il, TempMeta->numfld);
#							endif /* DEBUG_STRUCTURES */
							}
						}
					}

				/* Highlight the picked curves and labels */
				for (im=0; im<nmove; im++)
					{
					curve = cmove[im];
					highlight_curve(curve, 3);
					}
				if (mlabels)
					{
					for (il=0; il<nlabs; il++)
						{
						spot = smove[il];
						highlight_item("spot", (ITEM) spot, 3);
						}
					}

				/* Remove any marks from uncompleted actions */
				set = take_mf_set(TempMeta, "mark", "d", "", "");
				if (NotNull(set)) set = destroy_set(set);

				/* Re-display the curves */
				present_all();
				post_partial(TRUE);

#				ifdef DEBUG_STRUCTURES
				for (im=0; im<nmove; im++)
					pr_diag("Editor",
						"[edit_merge_line]   Rotate: %2d ... cmove: %x\n",
					im, cmove[im]);
				for (il=0; il<nlabs; il++)
					pr_diag("Editor",
						"[edit_merge_line]   Rotate: %2d ... smove/sids: %x %d\n",
					il, smove[il], sids[il]);
#				endif /* DEBUG_STRUCTURES */

				/* Pick the centre of rotation */
				put_message("line-rot-centre");
				if (!ready_Xpoint(DnEdit, p0, &butt)) return drawn;
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!pick_Xpoint(DnEdit, 0, Cpos, &butt)) continue;
				if (!inside_dn_window(DnEdit, Cpos))
					{
					put_message("line-rot-out");
					(void) sleep(1);
					continue;
					}

				mark0 = create_mark("", "", "", Cpos, 0.0);
				define_mspec(&mark0->mspec, 0, 1, NULL, 0, False, 4.0, 0.0,
							 (HILITE) 3);

				add_item_to_metafile(TempMeta, "mark",  "d", "", "",
									 (ITEM) mark0);
				present_temp(TRUE);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_merge_line] Adding mark to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = Rotate;
				continue;

			/* Pick and rotate a reference point */
			case Rotate:

				/* Pick a reference point */
				put_message("line-rot");
				if (!ready_Xpoint(DnEdit, p0, &butt)) return drawn;
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!inside_dn_window(DnEdit, p0))
					{
					put_message("line-rot-out1");
					(void) ignore_Xpoint();
					continue;
					}
				if (point_dist(Cpos, p0) < SplineRes)
					{
					put_message("line-rot-ref");
					(void) ignore_Xpoint();
					(void) sleep(1);
					continue;
					}

				/* Highlight the curves to rotate */
				set = copy_mf_set(TempMeta, "curve", "c", "mmerge", "mmerge");
				for (im=0; im<set->num; im++)
					{
					curve = (CURVE) set->list[im];
					highlight_curve(curve, 2);
					widen_curve(curve, 2.0);
					}
				prep_set(set, mformat);

				/* Display markers */
				mark0 = create_mark("", "", "", p0, 0.0);
				curv0 = create_curve("", "", "");
				add_point_to_curve(curv0, Cpos);
				add_point_to_curve(curv0, p0);
				define_mspec(&mark0->mspec, 0, 2, NULL, 0, False, 4.0, 0.0,
							 (HILITE) 3);
				define_lspec(&curv0->lspec, 0, 0, NULL, False, 2.0, 0.0,
							 (HILITE) 3);

				add_item_to_metafile(TempMeta, "mark",  "d", "", "",
									 (ITEM) mark0);
				add_item_to_metafile(TempMeta, "curve", "c", "", "",
									 (ITEM) curv0);
				present_temp(TRUE);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_merge_line] Adding mark/curve to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Rotate the reference point */
				put_message("line-rot-release");
				(void) urotate_Xpoint(DnEdit, set, Cpos, p0, p1, &Ang, &butt);
				set = destroy_set(set);
				if (!inside_dn_window(DnEdit, p1))
					{
					put_message("line-rot-out2");
					(void) sleep(1);
					continue;
					}

				/* Determine rotation parameters */
				dx0 = p0[X] - Cpos[X];
				dy0 = p0[Y] - Cpos[Y];

				/* Display markers */
				mark0 = create_mark("", "", "", p1, 0.0);
				curv0 = create_curve("", "", "");
				add_point_to_curve(curv0, Cpos);
				add_point_to_curve(curv0, p1);
				curv1 = create_curve("", "", "");
				add_point_to_curve(curv1, p0);
				angle = Ang*0.125; cang = cos(angle); sang = sin(angle);
				pz[X] = dx0*cang - dy0*sang + Cpos[X];
				pz[Y] = dx0*sang + dy0*cang + Cpos[Y];
				add_point_to_curve(curv1, pz);
				angle = Ang*0.250; cang = cos(angle); sang = sin(angle);
				pz[X] = dx0*cang - dy0*sang + Cpos[X];
				pz[Y] = dx0*sang + dy0*cang + Cpos[Y];
				add_point_to_curve(curv1, pz);
				angle = Ang*0.375; cang = cos(angle); sang = sin(angle);
				pz[X] = dx0*cang - dy0*sang + Cpos[X];
				pz[Y] = dx0*sang + dy0*cang + Cpos[Y];
				add_point_to_curve(curv1, pz);
				angle = Ang*0.500; cang = cos(angle); sang = sin(angle);
				pz[X] = dx0*cang - dy0*sang + Cpos[X];
				pz[Y] = dx0*sang + dy0*cang + Cpos[Y];
				add_point_to_curve(curv1, pz);
				angle = Ang*0.625; cang = cos(angle); sang = sin(angle);
				pz[X] = dx0*cang - dy0*sang + Cpos[X];
				pz[Y] = dx0*sang + dy0*cang + Cpos[Y];
				add_point_to_curve(curv1, pz);
				angle = Ang*0.750; cang = cos(angle); sang = sin(angle);
				pz[X] = dx0*cang - dy0*sang + Cpos[X];
				pz[Y] = dx0*sang + dy0*cang + Cpos[Y];
				add_point_to_curve(curv1, pz);
				angle = Ang*0.875; cang = cos(angle); sang = sin(angle);
				pz[X] = dx0*cang - dy0*sang + Cpos[X];
				pz[Y] = dx0*sang + dy0*cang + Cpos[Y];
				add_point_to_curve(curv1, pz);
				add_point_to_curve(curv1, p1);
				curv1->line = smooth_line(curv1->line, FilterRes/4, SplineRes);
				define_mspec(&mark0->mspec, 0, 2, NULL, 0, False, 4.0, 0.0,
							 (HILITE) 2);
				define_lspec(&curv0->lspec, 0, 0, NULL, False, 2.0, 0.0,
							 (HILITE) 2);
				define_lspec(&curv1->lspec, 0, 0, NULL, False, 2.0, 0.0,
							 (HILITE) 2);

				add_item_to_metafile(TempMeta, "mark",  "d", "", "",
									 (ITEM) mark0);
				add_item_to_metafile(TempMeta, "curve", "c", "", "",
									 (ITEM) curv0);
				add_item_to_metafile(TempMeta, "curve", "c", "", "",
									 (ITEM) curv1);
				present_temp(TRUE);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_merge_line] Adding mark/curve to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = RotateDone;
				continue;

			/* Rotate the curves */
			case RotateDone:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_line] Rotating\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);

#				ifdef DEBUG_STRUCTURES
				for (im=0; im<nmove; im++)
					pr_diag("Editor",
						"[edit_merge_line]   Rotating: %2d ... cmove: %x\n",
					im, cmove[im]);
				for (il=0; il<nlabs; il++)
					pr_diag("Editor",
						"[edit_merge_line]   Rotating: %2d ... smove/sids: %x %d\n",
					il, smove[il], sids[il]);
#				endif /* DEBUG_STRUCTURES */

				/* Rotate the curves */
				dx0 = p0[X] - Cpos[X];
				dy0 = p0[Y] - Cpos[Y];
				for (im=0; im<nmove; im++)
					{
					curve = ccopy[im];
					rotate_curve(curve, Cpos, Ang/RAD);
					}

				/* Then rotate labels attached to curves (if requested) */
				if (mlabels)
					{
					(void) spot_list_rotate(nlabs, scopy, Cpos, Ang/RAD);
					}

				if (EditUndoable) post_mod("curves");

				/* Modify labels if necessary */
				if (recompute_curveset_labs(EditCurves, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				put_message("edit-done");
				mstart  = FALSE;
				maction = TRUE;
				busy_cursor(FALSE);

				/* Re-display the picked curves and labels */
				State = ReDisplayAction;
				continue;

			/* Re-display the picked curves */
			case ReDisplay:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_line] Redisplay\n");
#				endif /* DEBUG */

				/* Keep the fetched field (including picked curves) */
				if (fullset)
					{
					mset = take_mf_set(TempMeta, "curve", "c", "mset", "mset");
					sset = take_mf_set(TempMeta, "spot",  "d", "sset", "sset");
					}

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_merge_line] ReDisplay ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();

				/* Re-display the fetched field (including picked curves) */
				if (NotNull(mset))
					{
					add_set_to_metafile(TempMeta, "c", "mset", "mset", mset);
					add_set_to_metafile(TempMeta, "d", "sset", "sset", sset);
					}
				present_all();
				post_partial(TRUE);

#				ifdef DEBUG_STRUCTURES
				if (NotNull(mset))
					pr_diag("Editor",
						"[edit_merge_line] ReDisplay ... adding mset/sset to TempMeta (%d)\n",
						TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Re-display the picked curves (after merging) */
			case ReDisplayAction:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_line] RedisplayAction\n");
#				endif /* DEBUG */

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_merge_line] ReDisplayAction ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();

				/* Re-pick the picked curves */
				for (im=0; im<nmove; im++)
					{

					/* Highlight the picked curve */
					copy = copy_curve(ccopy[im]);
					highlight_curve(copy, 2);
					widen_curve(copy, 2.0);

					add_item_to_metafile(TempMeta, "curve", "c",
										 "mmerge", "mmerge", (ITEM) copy);
					cmove[im] = copy;

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_merge_line] Adding line (%d) to TempMeta (%d)\n",
						im, TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */
					}

				/* Then re-pick labels attached to curves (if requested) */
				if (mlabels)
					{
					for (il=0; il<nlabs; il++)
						{

						/* Highlight the curve label */
						spot = copy_spot(scopy[il]);
						highlight_item("spot", (ITEM) spot, 2);
						add_item_to_metafile(TempMeta, "spot", "d",
											 "smerge", "smerge", (ITEM) spot);
						smove[il] = spot;

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[edit_merge_line] Adding spot (%d) to TempMeta (%d)\n",
							il, TempMeta->numfld);
#						endif /* DEBUG_STRUCTURES */
						}
					}

				/* Re-display the picked curves and labels */
				present_all();
				post_partial(FALSE);

				/* Move on to next stage */
				State = WaitAction;
				continue;

			/* Start again */
			case ReStart:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_line] ReStart\n");
#				endif /* DEBUG */

				/* Empty picked curve list */
				if (nmove > 0)
					{
					for (im=0; im<nmove; im++)
						ccopy[im] = remove_item_from_set(EditCurves,
														 (ITEM) ccopy[im]);
					nmove = 0;
					amove = 0;
					FREEMEM(cmove);
					FREEMEM(ccopy);
					}
				edit_select(NullCal, FALSE);

				/* Empty curve label list */
				if (nlabs > 0)
					{
					for (il=0; il<nlabs; il++)
						scopy[il] = remove_item_from_set(EditLabs,
														 (ITEM) scopy[il]);
					nlabs = 0;
					alabs = 0;
					FREEMEM(smove);
					FREEMEM(scopy);
					FREEMEM(sids);
					}

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_merge_line] ReStart ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();

				/* Wait for a "fetch" */
				State = (fetch)? Fetch: Wait;
				continue;
			}
		}
	}

/***********************************************************************
*                                                                      *
*     j o i n _ l i n e                                                *
*     j o i n _ l i n e _ p i c k                                      *
*     j o i n _ l i n e _ e n h a n c e                                *
*     j o i n _ l i n e _ p i c k _ 2 n d                              *
*     j o i n _ l i n e _ e n h a n c e _ 2 n d                        *
*     j o i n _ l i n e _ p i c k _ b r e a k                          *
*                                                                      *
***********************************************************************/

typedef	enum
			{
			JoinPick, JoinPick2nd, JoinPickWait,	/* interactive states */
			JoinRePick, JoinRePick2nd,				/* "cancel" states */
			JoinJoin, JoinBreak						/* join/break lines */
			}
		JOINSTATE;

static	JOINSTATE	join_line_pick(void);
static	void		join_line_enhance(void);
static	JOINSTATE	join_line_pick_2nd(STRING);
static	void		join_line_enhance_2nd(void);
static	JOINSTATE	join_line_pick_break(void);

/* For debug purposes */
static	void		show_join_state(STRING, JOINSTATE);

static	void		show_join_state
	(
	STRING		pre,
	JOINSTATE	state
	)

	{
	STRING	name;

#	ifdef DEBUG
	switch (state)
		{
		case JoinPick:		name = "Pick";			break;
		case JoinPick2nd:	name = "Pick 2nd";		break;
		case JoinPickWait:	name = "WaitingJoin";	break;
		case JoinRePick:	name = "RePick";		break;
		case JoinRePick2nd:	name = "RePick 2nd";	break;
		case JoinJoin:		name = "Join";			break;
		case JoinBreak:		name = "Break";			break;
		default:			name = "!Unknown!";		break;
		}
	pr_diag("Editor", "[edit_join_line] State: %s %s\n", pre, name);
#	endif /* DEBUG */

	return;
	}
/* For debug purposes */

static	LOGICAL	Jfresh = FALSE;		/* Just picked */
static	CURVE	Jpick  = NullCurve;	/* Picked curve */
static	CURVE	Jwork  = NullCurve;	/* Working copy of picked curve */
static	LINE	Jline  = NullLine;	/* Line buffer of picked curve */
static	CURVE	Jpick2 = NullCurve;	/* Second picked curve */
static	LOGICAL	AtBgn2 = TRUE;		/* Join begin portion of second curve */
static	CURVE	Jtemp2 = NullCurve;	/* Temporary copy of second picked curve */
static	CURVE	Jwork2 = NullCurve;	/* Working copy of second picked curve */
static	LINE	Jline2 = NullLine;	/* Line buffer of second picked curve */
static	CURVE	Jbuild = NullCurve;	/* New curve constructed from picked curves */

/**********************************************************************/

LOGICAL	edit_join_line

	(
	STRING	mode
	)

	{
	int			il, im, lp;
	float		tdist, tdist2, spfact, smdist;
	CURVE		curve;
	SPOT		spot;
	POINT		pos;
	STRING		sub, val, lab;
	JOINSTATE	next;
	LOGICAL		drawn = FALSE;

	/* Current state: */
	/* - where should we resume? */
	static	JOINSTATE	State = JoinPick;

	/* Line type */
	static	char	CurrSub[40] = "";

	/* Label list: */
	/* - list of curve labels to be removed */
	/* - sremove is pointers into EditLabs  */
	static	int			nlabs    = 0;
	static	int			alabs    = 0;
	static	SPOT		*sremove = NullSpotList;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_join_line] %s  State: %d\n", mode, State);
#	endif /* DEBUG */
	show_join_state("INITIAL", State);

	/* Clean up previous edits */
	if (same(mode, "cancel"))
		{

		switch (State)
			{
			case JoinPick2nd:	State = JoinRePick;
								show_join_state("RESET", State);
								break;

			case JoinPickWait:	State = JoinRePick2nd;
								show_join_state("RESET", State);
								break;

			case JoinBreak:		State = JoinRePick;
								show_join_state("RESET", State);
								break;

			default:			empty_temp();
								edit_can_join(FALSE);
								edit_can_break(FALSE);
								if (nlabs > 0)
									{
									nlabs = 0;
									alabs = 0;
									FREEMEM(sremove);
									}
								return FALSE;
			}
		}
	if (same(mode, "cancel all"))
		{
		ignore_partial();

		/* Empty curve label list */
		if (nlabs > 0)
			{
			nlabs = 0;
			alabs = 0;
			FREEMEM(sremove);
			}

		/* Clean up temporary buffers */
		Jfresh = FALSE;
		Jpick  = NullCurve;
		Jwork  = NullCurve;
		Jline  = NullLine;
		Jpick2 = NullCurve;
		Jtemp2 = destroy_curve(Jtemp2);
		Jwork2 = NullCurve;
		Jline2 = NullLine;
		Jbuild = NullCurve;

		/* Empty display structure */
		empty_temp();
		edit_can_join(FALSE);
		edit_can_break(FALSE);
		(void) strcpy(CurrSub, "");
		return FALSE;
		}
	if (same(mode, "clear"))
		{
		return FALSE;
		}

	/* Make sure the curve set contains the right stuff */
	if (!EditCurves)                      return FALSE;
	if (!same(EditCurves->type, "curve")) return FALSE;

	/* Initialize on startup */
	if (same(mode, "begin"))
		{
		ignore_partial();
		Jfresh = FALSE;

		/* Set up curve label list */
		if (nlabs > 0)
			{
			nlabs = 0;
			alabs = 0;
			FREEMEM(sremove);
			}

		edit_can_join(FALSE);
		edit_can_break(FALSE);
		(void) strcpy(CurrSub, "");
		State = JoinPick;
		show_join_state("BEGIN", State);
		}

	/* Set state for Right button "join" */
	if ( (same(mode, "join")) && IsNull(Jpick) )
		{
		put_message("line-no-pick");
		(void) sleep(1);
		State = JoinRePick;
		return FALSE;
		}
	if ( (same(mode, "join")) && IsNull(Jpick2) )
		{
		put_message("line-no-pick");
		(void) sleep(1);
		State = JoinRePick2nd;
		return FALSE;
		}
	if (same(mode, "join"))
		{
		State = JoinJoin;
		show_join_state("JOIN", State);
		}

	/* Set state for Right button "break" */
	if ( (same(mode, "break")) && IsNull(Jpick) )
		{
		put_message("line-no-pick");
		(void) sleep(1);
		State = JoinRePick;
		return FALSE;
		}
	if (same(mode, "break"))
		{
		State = JoinBreak;
		show_join_state("BREAK", State);
		}

	/* Repeat until told to quit */
	while (TRUE)
		{
		show_join_state("==", State);

		/* Resume at current stage */
		switch (State)
			{

			/* Pick a curve to join to */
			case JoinPick:

				/* Pick a curve to join to */
				next = join_line_pick();
				if (next == JoinPick) return drawn;

				/* Set the line type (if required) */
				CAL_get_defaults((CAL) Jpick->attrib, &sub, &val, &lab);
				if (!same(sub, CurrSub)) (void) safe_strcpy(CurrSub, sub);

				/* Display the picked curve */
				join_line_enhance();
				edit_can_break(TRUE);
				post_partial(TRUE);

				/* Move on to next stage */
				State = next;
				show_join_state("->", State);
				continue;

			/*  Repick the curve to join to */
			case JoinRePick:

				put_message("edit-cancel");

				/* Empty curve label list */
				if (nlabs > 0)
					{
					nlabs = 0;
					alabs = 0;
					FREEMEM(sremove);
					}

				/* Clean up temporary buffers */
				ignore_partial();
				Jfresh = FALSE;
				Jpick  = NullCurve;
				Jwork  = NullCurve;
				Jline  = NullLine;
				Jpick2 = NullCurve;
				Jtemp2 = destroy_curve(Jtemp2);
				Jwork2 = NullCurve;
				Jline2 = NullLine;
				Jbuild = NullCurve;

				/* Empty display structure */
				empty_temp();
				edit_can_join(FALSE);
				edit_can_break(FALSE);

				/* Move on to next stage */
				State = JoinPick;
				show_join_state("->", State);
				continue;

			/* Pick a second curve to join */
			case JoinPick2nd:

				/* Pick a second curve to join */
				next = join_line_pick_2nd(CurrSub);
				if (next == JoinPick2nd) return drawn;

				/* Display the second picked curve */
				join_line_enhance_2nd();
				edit_can_join(TRUE);
				edit_can_break(FALSE);
				post_partial(TRUE);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_join_line] Jpick: <%x>  Jpick2: <%x>\n",
					Jpick, Jpick2);
#				endif /* DEBUG_STRUCTURES */

				/* Second curve has been picked ... so check the curve labels */
				nlabs = 0;
				if (NotNull(EditLabs) && EditLabs->num > 0)
					{
					for (il=0; il<EditLabs->num; il++)
						{
						spot = (SPOT) EditLabs->list[il];
						if (!spot) continue;
						if (same(spot->mclass, "legend")) continue;

						/* Only check labels attached to picked curve */
						curve = closest_curve(EditCurves, spot->anchor,
										  	&tdist, NULL, NULL);
						if (tdist > PickTol) continue;
						if (curve != Jpick2) continue;

						/* Save labels that are not on new curve segment */
						(void) curve_test_point(Jwork2, spot->anchor, &tdist,
									NullPoint, NullInt, NullLogical, NullLogical);
						if (tdist > PickTol)
							{

							/* Add the curve label to the list */
							nlabs++;
							if (nlabs > alabs)
								{
								alabs   = nlabs;
								sremove = GETMEM(sremove, SPOT, alabs);
								}
							sremove[nlabs-1] = spot;
							}
						}
					}

				/* Move on to next stage */
				State = next;
				show_join_state("->", State);
				continue;

			/* Allow repick while waiting for a join */
			case JoinPickWait:

				/* Repick a second curve to join */
				next = join_line_pick_2nd(CurrSub);
				if (next == JoinPick2nd) return drawn;

				/* Clean up temporary buffers */
				Jwork  = NullCurve;
				Jline  = NullLine;
				Jwork2 = NullCurve;
				Jline2 = NullLine;
				Jbuild = NullCurve;

				/* Empty display structure */
				empty_temp();

				/* Redisplay the first picked curve */
				join_line_enhance();

				/* Empty curve label list */
				if (nlabs > 0)
					{
					nlabs = 0;
					alabs = 0;
					FREEMEM(sremove);
					}

				/* Display the second picked curve */
				join_line_enhance_2nd();
				edit_can_join(TRUE);
				edit_can_break(FALSE);
				post_partial(TRUE);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_join_line] Jpick: <%x>  Jpick2: <%x>\n",
					Jpick, Jpick2);
#				endif /* DEBUG_STRUCTURES */

				/* Second curve has been picked ... so check the curve labels */
				nlabs = 0;
				if (NotNull(EditLabs) && EditLabs->num > 0)
					{
					for (il=0; il<EditLabs->num; il++)
						{
						spot = (SPOT) EditLabs->list[il];
						if (!spot) continue;
						if (same(spot->mclass, "legend")) continue;

						/* Only check labels attached to picked curve */
						curve = closest_curve(EditCurves, spot->anchor,
										  	&tdist, NULL, NULL);
						if (tdist > PickTol) continue;
						if (curve != Jpick2) continue;

						/* Save labels that are not on new curve segment */
						(void) curve_test_point(Jwork2, spot->anchor, &tdist,
									NullPoint, NullInt, NullLogical, NullLogical);
						if (tdist > PickTol)
							{

							/* Add the curve label to the list */
							nlabs++;
							if (nlabs > alabs)
								{
								alabs   = nlabs;
								sremove = GETMEM(sremove, SPOT, alabs);
								}
							sremove[nlabs-1] = spot;
							}
						}
					}

				/* Move on to next stage */
				State = next;
				show_join_state("->", State);
				continue;

			/* Repick the second curve to join */
			case JoinRePick2nd:

				/* Clean up temporary buffers */
				Jwork  = NullCurve;
				Jline  = NullLine;
				Jpick2 = NullCurve;
				Jtemp2 = destroy_curve(Jtemp2);
				Jwork2 = NullCurve;
				Jline2 = NullLine;
				Jbuild = NullCurve;

				/* Empty display structure */
				empty_temp();
				edit_can_join(FALSE);

				/* Redisplay the first picked curve */
				join_line_enhance();
				edit_can_break(TRUE);

				/* Empty curve label list */
				if (nlabs > 0)
					{
					nlabs = 0;
					alabs = 0;
					FREEMEM(sremove);
					}
				post_partial(TRUE);

				/* Move on to next stage */
				State = JoinPick2nd;
				show_join_state("->", State);
				continue;

			/* Join the two curves */
			case JoinJoin:

				/* Freeze all pending edits so only this one can be undone */
				/* (first time after picking) */
				if (Jfresh)
					{
					if (EditUndoable && edit_posted())
						{
						ignore_partial();
						put_message("edit-freeze");
						accept_mod();	/* Leaves EditCurves alone */
						}
					}
				post_partial(TRUE);

				/* Here we go */
				busy_cursor(TRUE);
				put_message("line-join-joining");

				/* Set smoothing options */
				spfact = DrawSmth;
				spfact = MAX(spfact, 0.0);
				spfact = MIN(spfact, 100.0);

				/* Construct modified curve from first picked curve and */
				/*  truncated portion of second picked curve */
				Jbuild = copy_curve(Jwork);
				empty_curve(Jbuild);

				/* Take care of smoothly joining the curves (if required) */
				if (spfact > 1.0)
					{
					float	fres = FilterRes/4;

					/* Back up from one end of second curve to make room for */
					/*  smoothing ... move back between 0 and 2 spline steps */
					smdist = SplineRes * (spfact*0.02);
					(void) trunc_line(Jline2, smdist, !AtBgn2, AtBgn2);

					/* Join truncated second curve to start of first curve */
					if (AtBgn2)
						{
						append_line(Jbuild->line, Jline2);
						smjoin_lines(Jbuild->line, Jline, fres, SplineRes);
						}

					/* Join truncated second curve to end of first curve */
					else
						{
						append_line(Jbuild->line, Jline);
						smjoin_lines(Jbuild->line, Jline2, fres, SplineRes);
						}
					}

				/* Join curves directly if no smoothing */
				else
					{

					/* Join second curve to start of first curve */
					if (AtBgn2)
						{
						append_line(Jbuild->line, Jline2);
						append_line(Jbuild->line, Jline);
						}

					/* Join second curve to end of first curve */
					else
						{
						append_line(Jbuild->line, Jline);
						append_line(Jbuild->line, Jline2);
						}
					}

				/* Highlight the new curve */
				highlight_curve(Jbuild, 3);
				add_item_to_metafile(TempMeta, "curve", "c", "", "",
						(ITEM) Jbuild);
				present_temp(TRUE);
				(void) sleep(1);

				/* Replace the first picked curve with the new curve */
				empty_curve(Jpick);
				add_line_to_curve(Jpick, Jbuild->line);

				/* Remove the second picked curve from the set of curves */
				(void) remove_item_from_set(EditCurves, (ITEM) Jpick2);

				/* Remove labels on truncated part of second curve */
				if (NotNull(EditLabs) && EditLabs->num > 0 && nlabs > 0)
					{
					for (il=nlabs-1; il>=0; il--)
						{
						(void) remove_item_from_set(EditLabs, (ITEM) sremove[il]);
						}
					}

				/* Register the edit */
				drawn  = TRUE;
				Jfresh = FALSE;
				invoke_set_catspecs(EditCurves);
				if (EditUndoable) highlight_set(EditCurves, 1);
				if (EditUndoable) post_mod("curves");

				/* Modify labels if necessary */
				if (recompute_curveset_labs(EditCurves, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				/* Show the results */
				busy_cursor(FALSE);
				present_all();

				/* Go back to 2nd curve pick stage */
				State = JoinRePick2nd;
				show_join_state("->", State);
				continue;

			/* Break the picked curve in two */
			case JoinBreak:

				/* Pick point on curve to break at */
				next = join_line_pick_break();
				if (next == JoinPick2nd) return drawn;

				/* Freeze all pending edits so only this one can be undone */
				/* (first time after picking) */
				if (Jfresh)
					{
					if (EditUndoable && edit_posted())
						{
						ignore_partial();
						put_message("edit-freeze");
						accept_mod();	/* Leaves EditCurves alone */
						}
					}
				post_partial(TRUE);

				/* Here we go */
				busy_cursor(TRUE);
				put_message("line-join-breaking");
				(void) sleep(1);

				/* Replace the first picked curve with the first section */
				empty_curve(Jpick);
				add_line_to_curve(Jpick, Jline);

				/* Add the second section to set of curves */
				/* Remove the second picked curve from the set of curves */
				curve = copy_curve(Jbuild);
				(void) add_item_to_set(EditCurves, (ITEM) curve);

				/* Register the edit */
				drawn  = TRUE;
				Jfresh = FALSE;
				invoke_set_catspecs(EditCurves);
				if (EditUndoable) highlight_set(EditCurves, 1);
				if (EditUndoable) post_mod("curves");

				/* Modify labels if necessary */
				if (recompute_curveset_labs(EditCurves, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				/* Show the results */
				busy_cursor(FALSE);
				present_all();

				/* Go back to curve pick stage */
				State = JoinRePick;
				show_join_state("->", State);
				continue;
			}
		}
	}

/**********************************************************************/

static	JOINSTATE	join_line_pick(void)

	{
	POINT	pos;
	int		butt;

	/* See if there are enough curves to join to */
	if (EditCurves->num < 1)
		{
		put_message("line-no-join");
		return JoinPick;
		}

	/* Keep trying until something gets picked */
	while (TRUE)
		{

		/* Get a point */
		put_message("line-join-pick");
		if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return JoinPick;
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}
		put_message("line-pick");
		if (!pick_Xpoint(DnEdit, 0, pos, &butt)) continue;
		if (!inside_dn_window(DnEdit, pos))
			{
			put_message("edit-outside-map");
			(void) sleep(1);
			continue;
			}

		/* See which curve we picked */
		Jpick = closest_curve(EditCurves, pos, NULL, NULL, NULL);
		if (!Jpick)
			{
			put_message("line-no-pick");
			(void) sleep(1);
			continue;
			}
		put_message("line-picked");
		Jfresh = TRUE;

		/* Go on to the next stage */
		return JoinPick2nd;
		}
	}

/**********************************************************************/

static	void	join_line_enhance(void)

	{

	/* Produce a copy of the picked curve to work on */
	Jwork = copy_curve(Jpick);
	Jline = Jwork->line;
	condense_line(Jline);

	/* Highlight the picked curve */
	widen_curve(Jwork, 2.0);
	highlight_curve(Jwork, 2);
	add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) Jwork);

	/* Display the highlighted curve */
	present_temp(TRUE);
	}

/**********************************************************************/

static	JOINSTATE	join_line_pick_2nd
	(
	STRING		csub
	)

	{
	int		lp, lp2, sseg, eseg;
	float	sdist, edist, xlen, dx, dy;
	STRING	sub2, val2, lab2;
	POINT	spos, epos;
	LINE	xline;
	CURVE	curve;
	POINT	pos;
	int		butt;

	/* See if there are enough curves to join */
	if (EditCurves->num < 2)
		{
		put_message("line-no-join2");
		(void) sleep(1);
		return JoinPick2nd;
		}

#	ifdef DEBUG_STRUCTURES
	pr_diag("Editor",
		"[edit_join_line] Jpick: <%x>  Jpick2: <%x>\n",
		Jpick, Jpick2);
#	endif /* DEBUG_STRUCTURES */

	/* Keep trying until something gets picked */
	while (TRUE)
		{

		/* Put up a prompt */
		if (IsNull(Jpick2)) put_message("line-join2-pick");
		else                put_message("line-join2-pick2");

		/* Get the point entered */
		if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return JoinPick2nd;
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}
		put_message("line-pick");
		if (!pick_Xpoint(DnEdit, 0, pos, &butt)) continue;
		if (!inside_dn_window(DnEdit, pos))
			{
			put_message("edit-outside-map");
			(void) sleep(1);
			continue;
			}

		/* See which curve we picked */
		curve = closest_curve(EditCurves, pos, NULL, NULL, NULL);
		if (!curve)
			{
			put_message("line-no-pick");
			(void) sleep(1);
			continue;
			}

		/* Cannot join to the same curve! */
		else if (curve == Jpick)
			{
			put_message("line-join2-nojoin");
			(void) sleep(1);
			continue;
			}

		/* Ignore if already picked */
		else if (NotNull(Jpick2) && curve == Jpick2)
			{
			put_message("line-join2-same");
			(void) sleep(1);
			continue;
			}

		/* Check the line type */
		CAL_get_defaults((CAL) curve->attrib, &sub2, &val2, &lab2);
		if (!same(sub2, csub))
			{
			put_message("line-join2-type");
			(void) sleep(1);
			continue;
			}

		/* Make a copy of the picked curve */
		Jtemp2 = copy_curve(curve);

		/* Ensure that second curve matches sense of first curve */
		if (Jtemp2->sense != Jwork->sense) reverse_curve(Jtemp2);

		/* Find closest points on second curve to endpoints on first curve */
		(void) curve_test_point(Jtemp2, Jline->points[0], NullFloat,
								spos, &sseg, NullLogical, NullLogical);
		lp = Jline->numpts - 1;
		(void) curve_test_point(Jtemp2, Jline->points[lp], NullFloat,
								epos, &eseg, NullLogical, NullLogical);

		/* Determine longest segment to join */
		sdist  = line_slen(Jtemp2->line, 0.0, (float) sseg);
		dx     = spos[X] - Jtemp2->line->points[sseg][X];
		dy     = spos[Y] - Jtemp2->line->points[sseg][Y];
		sdist += (float) hypot((float) dx, (float) dy);
		lp2    = Jtemp2->line->numpts - 1;
		edist  = line_slen(Jtemp2->line, (float) eseg, (float) lp2);
		dx     = epos[X] - Jtemp2->line->points[eseg][X];
		dy     = epos[Y] - Jtemp2->line->points[eseg][Y];
		edist -= (float) hypot((float) dx, (float) dy);

#		ifdef DEBUG_STRUCTURES
		pr_diag("Editor",
			"[edit_join_line] Start length: %.2f on segment: %d\n",
			sdist, sseg);
		pr_diag("Editor",
			"[edit_join_line] End length:   %.2f on segment: %d\n",
			edist, eseg);
#		endif /* DEBUG_STRUCTURES */

		/* Determine if second curve is not an extension */
		if ( sdist <= 0.0 && edist <= 0.0 )
			{
			Jtemp2 = destroy_curve(Jtemp2);
			put_message("line-join2-extend");
			(void) sleep(1);
			continue;
			}

		/* Set the picked curve */
		put_message("line-picked");
		Jpick2 = curve;

		/* Rebuild begin portion of second curve to join */
		if ( sdist > edist )
			{
			AtBgn2 = TRUE;
			xline  = create_line();
			append_line_portion(xline, Jtemp2->line, 0, sseg);
			add_point_to_line(xline, spos);
			condense_line(xline);
			empty_curve(Jtemp2);
			add_line_to_curve(Jtemp2, xline);
			(void) destroy_line(xline);
			}

		/* Rebuild end portion of second curve to join */
		else
			{
			AtBgn2 = FALSE;
			xline  = create_line();
			add_point_to_line(xline, epos);
			if (eseg < lp2) append_line_portion(xline, Jtemp2->line, eseg+1, lp2);
			empty_curve(Jtemp2);
			add_line_to_curve(Jtemp2, xline);
			(void) destroy_line(xline);
			}

		/* Go on to the next stage */
		return JoinPickWait;
		}
	}

/**********************************************************************/

static	void	join_line_enhance_2nd(void)

	{

	/* Produce a copy of the temporary curve to work on */
	Jwork2 = copy_curve(Jtemp2);
	Jtemp2 = destroy_curve(Jtemp2);
	Jline2 = Jwork2->line;
	condense_line(Jline2);

	/* Highlight the second picked curve */
	widen_curve(Jwork2, 2.0);
	highlight_curve(Jwork2, 2);
	add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) Jwork2);

	/* Display the highlighted curve */
	present_temp(TRUE);
	}

/**********************************************************************/

static	JOINSTATE	join_line_pick_break(void)

	{
	int		lp, xseg;
	POINT	xpos;
	POINT	pos;
	int		butt;

	/* Keep trying until a break point gets picked */
	while (TRUE)
		{

		/* Put up a prompt */
		put_message("line-join-break");

		/* Get the point entered */
		if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return JoinPick2nd;
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}
		put_message("line-join-break-rel");
		if (!pick_Xpoint(DnEdit, 0, pos, &butt)) continue;
		if (!inside_dn_window(DnEdit, pos))
			{
			put_message("edit-outside-map");
			(void) sleep(1);
			continue;
			}

		/* Find closest point on picked curve to break point */
		(void) curve_test_point(Jwork, pos, NullFloat,
								xpos, &xseg, NullLogical, NullLogical);

		/* Make a copy of the working curve for the second section */
		Jbuild = copy_curve(Jwork);
		empty_curve(Jbuild);

		/* Rebuild second portion of picked curve, starting with break point */
		add_point_to_line(Jbuild->line, xpos);
		lp    = Jline->numpts - 1;
		append_line_portion(Jbuild->line, Jline, xseg+1, lp);

		/* Highlight the new curve */
		highlight_curve(Jbuild, 3);
		add_item_to_metafile(TempMeta, "curve", "c", "", "",
				(ITEM) Jbuild);
		present_temp(TRUE);

		/* Shorten the working curve to break point segment */
		/*  and then add the break point for first portion  */
		Jline->numpts = xseg + 1;
		add_point_to_line(Jline, xpos);

		/* Go on to the next stage */
		return JoinBreak;
		}
	}
