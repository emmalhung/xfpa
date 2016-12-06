/***********************************************************************
*                                                                      *
*     e d i t _ a r e a . c                                            *
*                                                                      *
*     Assorted operations for discrete area fields.                    *
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

#undef DEBUG_EDIT
#undef DEBUG_STRUCTURES

#define	Sl SkipLength
#define	Sw SkipWidth

/***********************************************************************
*                                                                      *
*     r e a d y _ a r e a _ f i e l d                                  *
*                                                                      *
***********************************************************************/

LOGICAL	edit_ready_area_field

	(
	STRING	mode
	)

	{
	int		butt;

	if (!EditUndoable)     edit_can_create(FALSE);
	if (!EditUndoable)     return TRUE;
	if (NotNull(OldAreas)) edit_can_create(FALSE);
	if (NotNull(OldAreas)) return TRUE;

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
			put_message("area-no-field");
			edit_can_create(FALSE);
			if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return FALSE;
			(void) ignore_Xpoint();
			}
		}

	/* No field is present, but it can be created */
	while (TRUE)
		{
		put_message("area-no-fld-cr");
		edit_can_create(TRUE);
		if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return FALSE;
		(void) ignore_Xpoint();
		}
	}

/***********************************************************************
*                                                                      *
*     d r a w _ a r e a                                                *
*                                                                      *
***********************************************************************/

static	AREA	NewArea = NullArea;

LOGICAL	edit_draw_area

	(
	STRING	mode,
	CAL		cal
	)

	{
	int		nlines;
	LINE	*lines;
	AREA	area;
	STRING	lab;
	int		butt;
	LOGICAL	drawn = FALSE;

	/* Current state: */
	/* - where should we resume? */
	static  enum    { Draw, Set } State = Draw;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_draw_area] %s  State: %d\n", mode, State);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same_start(mode, "cancel"))
		{
		ignore_partial();
		if (EditAreas && EditUndoable)
			{
			highlight_set(EditAreas, 1);
			present_all();
			}
		drawing_control(FALSE);
		State = Draw;
		return FALSE;
		}
	if (same(mode, "clear"))
		{
		State = Draw;
		return FALSE;
		}

	/* Make sure the area set contains the right stuff */
	if (!EditAreas)                     return FALSE;
	if (!same(EditAreas->type, "area")) return FALSE;

	/* Initialize on startup */
	if (same(mode, "begin"))
		{
		ignore_partial();
		drawing_control(FALSE);
		edit_select(cal, TRUE);
		State = Draw;
		}

	/* Reset label for this type of area */
	lab = CAL_get_attribute(cal, CALuserlabel);

	/* Take care of setting new value after drawing */
	/* Only recognized when an area has just been drawn */
	if (same(mode, "set") && NotNull(cal)) State = Set;

	/* Repeat until told to quit */
	while (TRUE)
		{

		/* Resume at current stage */
		switch (State)
			{

			/* Draw an area */
			case Draw:

				/* Turn on set state */
				edit_select(cal, TRUE);

				/* Put up a prompt */
				if (drawing_Xcurve()) put_message("drawing");
				else                  put_message("area-draw2", lab);

				/* Draw the area boundary */
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
					accept_mod();	/* Leaves EditAreas alone */
					}
				post_partial(TRUE);

				/* Draw the area boundary */
				put_message("area-draw-rel");
				edit_select(NullCal, FALSE);
				drawing_control(TRUE);
				if (!ready_Xcurve())
					{
					if (drawing_Xcurve()) return drawn;
					(void) draw_Xcurve(DnEdit, FilterRes, SplineRes, TRUE);
					if (!ready_Xcurve()) return drawn;
					}
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

				/* Add new boundary to area field */
				busy_cursor(TRUE);
				put_message("area-draw-proc");
				area = create_area(NULL, NULL, NULL);
				define_item_attribs("area", (ITEM) area, cal);
				define_area_boundary(area, copy_line(lines[0]));
				reset_pipe();
				switch (StackMode)
					{
					case STACK_BOTTOM:
							(void) add_item_to_set(EditAreas, (ITEM) area);
							break;
					case STACK_TOP:
							(void) add_item_to_set_start(EditAreas,
															(ITEM) area);
							(void) adjust_area_link_nodes(ActiveDfld,
															EditTime, -1, 0);
							break;
					}

				/* Set up display specs for the new area field */
				NewArea = area;
				invoke_set_catspecs(EditAreas);
				if (EditUndoable) highlight_set(EditAreas, 1);

				/* Register the edit */
				drawn = TRUE;
				if (EditUndoable) post_mod("areas");

				/* Modify labels if necessary */
				if (recompute_areaset_labs(EditAreas, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				if (EditUndoable) (void) extract_area_order_tags(FALSE);

				/* Show the results */
				present_all();
				busy_cursor(FALSE);

				/* Move on to next stage */
				State = Draw;
				continue;

			/* Set the value for the area just drawn */
			case Set:

				if (!edit_posted())
					{
					NewArea = NullArea;
					State = Draw;
					continue;
					}

#				ifdef DEBUG
				pr_diag("Editor", "[edit_draw_area] Setting value (%s)\n", lab);
#				endif /* DEBUG */

				/* Set a new value */
				busy_cursor(TRUE);
				put_message("area-value");
				define_item_attribs("area", (ITEM) NewArea, cal);

				/* Set up display specs for the new area field */
				invoke_set_catspecs(EditAreas);
				if (EditUndoable) highlight_set(EditAreas, 1);

				/* Register the edit */
				drawn = TRUE;
				if (EditUndoable) post_mod("areas");

				/* Modify labels if necessary */
				if (recompute_areaset_labs(EditAreas, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				if (EditUndoable) (void) extract_area_order_tags(FALSE);

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
*     a d d h o l e _ a r e a                                          *
*     a d d h o l e _ a r e a _ p i c k                                *
*     a d d h o l e _ a r e a _ d r a w                                *
*                                                                      *
***********************************************************************/

typedef	enum
			{
			HolePick, HoleDraw,					/* interactive states */
			HoleRePick, HoleReDraw,				/* "cancel" states */
			HoleAdd, HoleAddPreset,				/* add holes */
			HoleConfirm							/* complete modification */
			}
		HOLESTATE;

static	HOLESTATE	addhole_area_pick(void);
static	HOLESTATE	addhole_area_draw(void);

static	LOGICAL	Hstart = FALSE;			/* Hole started (but not completed?) */
static	AREA	Hpick  = NullArea;		/* Picked area */
static	SUBAREA	Hpsub  = NullSubArea;	/* Picked subarea */
static	AREA	Hwork  = NullArea;		/* Working copy of picked area */
static	LINE	Hnew   = NullLine;		/* New hole */

/**********************************************************************/

LOGICAL	edit_addhole_area

	(
	STRING	mode,
	STRING	name
	)

	{
	LOGICAL		last, in;
	int			numhole, ih;
	CURVE		hole, *holes;
	HOLESTATE	next;
	LOGICAL		drawn = FALSE;

	/* Current state: */
	/* - where should we resume? */
	static	HOLESTATE	State = HolePick;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_addhole_area] %s  State: %d\n", mode, State);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same(mode, "cancel"))
		{
		edit_allow_preset_outline(FALSE);

		switch (State)
			{
			case HoleDraw:		State = (Hstart)? HoleReDraw: HoleRePick;
								Hstart = FALSE;
								break;

			default:			empty_temp();
								drawing_control(FALSE);
								return FALSE;
			}
		}
	if (same(mode, "cancel all"))
		{
		ignore_partial();
		edit_allow_preset_outline(FALSE);

		/* Clean up temporary buffers */
		edit_select(NullCal, FALSE);
		Hpick = NullArea;
		Hpsub = NullSubArea;
		Hwork = NullArea;

		/* Empty display structure */
		empty_temp();
		drawing_control(FALSE);
		return FALSE;
		}
	if (same(mode, "clear"))
		{
		return FALSE;
		}

	/* Make sure the area set contains the right stuff */
	if (!EditAreas)                     return FALSE;
	if (!same(EditAreas->type, "area")) return FALSE;

	/* Initialize on startup */
	if (same(mode, "begin"))
		{
		ignore_partial();
		drawing_control(FALSE);
		Hstart = FALSE;
		edit_select(NullCal, FALSE);
		State = HolePick;
		}

	/* Set state for Right button "preset outline" */
	if (same(mode, "preset outline"))
		{

		/* Check for last drawn hole or named holes */
		if (same(name, FpaEditorLastDrawnHole))
			{
			last    = TRUE;
			hole    = retrieve_drawn_hole();

			/* Display the hole */
			if (NotNull(hole))
				{
				define_lspec(&hole->lspec, 255, 0, NULL, False, 2.0, 0.0,
						 	(HILITE) 2);
				add_item_to_metafile(TempMeta, "curve", "c",
								 	"", "", (ITEM) hole);
				present_temp(TRUE);
				(void) sleep(1);

				/* Add the hole to the area */
				State = HoleAddPreset;
				}

			/* No hole drawn yet */
			else
				{
				put_message("edit-no-drawn-hole");
				pr_warning("Editor",
					"[edit_addhole_area] No hole drawn yet!\n");
				(void) sleep(1);
				State = HoleDraw;
				}
			}
		else
			{
			last    = FALSE;
			numhole = retrieve_named_holes(name, &holes);

			/* Display the holes */
			if (numhole > 0)
				{
				for (ih=0; ih<numhole; ih++)
					{
					hole = holes[ih];
					define_lspec(&hole->lspec, 255, 0, NULL, False, 2.0, 0.0,
								(HILITE) 2);
					add_item_to_metafile(TempMeta, "curve", "c",
										"", "", (ITEM) hole);
					}
				present_temp(TRUE);
				(void) sleep(1);

				/* Add the holes to the area */
				State = HoleAddPreset;
				}

			/* No holes in named file */
			else
				{
				put_message("edit-no-preset-holes", name);
				pr_error("Editor",
					"[edit_addhole_area] No preset holes file: \"%s\"\n",
					name);
				(void) sleep(1);
				State = HoleDraw;
				}
			}
		}

	/* Repeat until told to quit */
	while (TRUE)
		{

		/* Resume at current stage */
		switch (State)
			{

			/* Pick an area to add holes */
			case HolePick:

				/* Pick an area */
				ignore_partial();
				edit_allow_preset_outline(FALSE);
				next = addhole_area_pick();
				if (next == HolePick) return drawn;

				/* Move on to next stage */
				State = next;
				continue;

			/* Draw a new hole in the picked area */
			case HoleDraw:

				/* Draw a new segment */
				next = addhole_area_draw();
				if (next == HoleDraw) return drawn;

				/* Move on to next stage */
				State = next;
				continue;

			/*  Repick the area to add holes */
			case HoleRePick:

				put_message("edit-cancel");
				edit_allow_preset_outline(FALSE);

				/* Clean up temporary buffers */
				ignore_partial();
				edit_select(NullCal, FALSE);
				Hpick  = NullArea;
				Hpsub  = NullSubArea;
				Hwork  = NullArea;

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_addhole_area] HoleRePick ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();
				drawing_control(FALSE);

				/* Move on to next stage */
				State = HolePick;
				/* >>>>> do we need to return FALSE??? <<<<< */
				continue;

			/*  Redraw the hole */
			case HoleReDraw:

				put_message("edit-cancel");
				edit_allow_preset_outline(FALSE);

				/* Clean up temporary buffers */

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_addhole_area] HoleReDraw ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();
				drawing_control(FALSE);

				/* Replace area to draw hole with new copy of original */
				Hwork = copy_area(Hpick, TRUE);

				/* Highlight the picked area */
				widen_area(Hwork, 2.0);
				highlight_area(Hwork, 2, -1);

				add_item_to_metafile(TempMeta, "area", "c", "", "",
									 (ITEM) Hwork);
				present_temp(TRUE);

				/* Move on to next stage */
				State = HoleDraw;
				/* >>>>> do we need to return FALSE??? <<<<< */
				continue;

			/* Add a hole to the area */
			case HoleAdd:

				/* Freeze all pending edits so only this one can be undone */
				if (EditUndoable)
					{
					ignore_partial();
					put_message("edit-freeze");
					accept_mod();	/* Leaves EditAreas alone */
					}
				post_partial(TRUE);

#				ifdef DEBUG
				pr_diag("Editor", "[edit_addhole_area] Modifying\n");
#				endif /* DEBUG */

				/* Modify the area */
				busy_cursor(TRUE);
				put_message("area-mod-hole");
				edit_allow_preset_outline(FALSE);
				hole = create_curve("", "", "");
				add_line_to_curve(hole, Hnew);
				post_drawn_hole(hole);
				add_area_hole(Hpick, Hnew);
				Hnew = NullLine;

				/* Move on to next stage */
				State = HoleConfirm;
				continue;

			/* Add one or more preset holes to the area */
			case HoleAddPreset:

				/* Freeze all pending edits so only this one can be undone */
				if (EditUndoable)
					{
					ignore_partial();
					put_message("edit-freeze");
					accept_mod();	/* Leaves EditAreas alone */
					}
				post_partial(TRUE);

#				ifdef DEBUG
				pr_diag("Editor", "[edit_addhole_area] Modifying\n");
#				endif /* DEBUG */

				/* Modify the area */
				busy_cursor(TRUE);
				put_message("area-mod-hole");
				edit_allow_preset_outline(FALSE);

				/* Add drawn hole to area ... if inside */
				if (last)
					{
					in = hole_inside_area(Hpick, hole->line);
					if (in) add_area_hole(Hpick, copy_line(hole->line));
					}

				/* Add preset holes to area ... if inside */
				else
					{
					for (ih=0; ih<numhole; ih++)
						{
						hole = holes[ih];
						in = hole_inside_area(Hpick, hole->line);
						if (in) add_area_hole(Hpick, copy_line(hole->line));
						holes[ih] = NullCurve;
						}
					FREEMEM(holes);
					}

				/* Move on to next stage */
				State = HoleConfirm;
				continue;

			/* Confirm current edit */
			case HoleConfirm:

				/* Register the edit */
				ignore_partial();
				drawn = TRUE;
				invoke_set_catspecs(EditAreas);
				if (EditUndoable) highlight_set(EditAreas, 1);
				if (EditUndoable) post_mod("areas");

				/* Modify labels if necessary */
				if (recompute_areaset_labs(EditAreas, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				if (EditUndoable) (void) extract_area_order_tags(FALSE);

				/* Clean up temporary buffers */
				edit_select(NullCal, FALSE);
				Hpick  = NullArea;
				Hpsub  = NullSubArea;
				Hwork  = NullArea;

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_addhole_area] HoleConfirm ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();
				drawing_control(FALSE);
				busy_cursor(FALSE);

				/* Go back to pick stage */
				State = HolePick;
				continue;
			}
		}
	}

/**********************************************************************/

static	HOLESTATE	addhole_area_pick(void)

	{
	POINT	pos;
	int		butt;

	/* See if there are any areas to add holes */
	if (EditAreas->num <= 0)
		{
		put_message("area-no-mod");
		return HolePick;
		}

	/* Keep trying until something gets picked */
	while (TRUE)
		{
		/* Get a point */
		put_message("area-hole-pick");
		if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return HolePick;
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}
		put_message("area-pick");
		if (!pick_Xpoint(DnEdit, 0, pos, &butt)) continue;
		if (!inside_dn_window(DnEdit, pos))
			{
			put_message("edit-outside-map");
			(void) sleep(1);
			continue;
			}

		/* See which area we picked */
		Hpsub = closest_subarea(EditAreas, pos, NullFloat, NullPoint,
								NullInt, NullInt, &Hpick);
		if (!Hpsub)
			{
			put_message("area-no-pick");
			(void) sleep(1);
			continue;
			}

		/* Produce a copy of the picked area to work on */
		put_message("area-picked");
		edit_select((CAL) Hpsub->attrib, FALSE);
		Hwork = copy_area(Hpick, TRUE);

		/* Highlight the picked area */
		widen_area(Hwork, 2.0);
		highlight_area(Hwork, 2, -1);

		add_item_to_metafile(TempMeta, "area", "c", "", "", (ITEM) Hwork);
		present_temp(TRUE);

#		ifdef DEBUG_STRUCTURES
		pr_diag("Editor",
			"[addhole_area_pick] Adding Hwork to TempMeta (%d)\n",
			TempMeta->numfld);
#		endif /* DEBUG_STRUCTURES */

		return HoleDraw;
		}
	}

/**********************************************************************/

static	HOLESTATE	addhole_area_draw(void)

	{
	int		butt;
	POINT	pos;
	int		nlines;
	LINE	*lines;
	LOGICAL	in;

	/* Make sure there is an area to add holes */
	if (!Hwork)
		{
		put_message("area-none-picked");
		return HoleDraw;
		}

	/* Repeat until we get a new segment or another choice */
	while (TRUE)
		{
		/* Draw a hole */
		put_message("area-hole-draw");
		edit_allow_preset_outline(TRUE);
		/* >>>>> add this!!! <<<<< */
		post_partial(TRUE);
		/* >>>>> add this!!! <<<<< */
		if (ready_Xcurve()) butt = LeftButton;
		else if (!ready_Xpoint(DnEdit, pos, &butt)) return HoleDraw;

		/* Invalid button */
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}

		/* Draw the curve */
		put_message("area-hole-draw-rel");
		drawing_control(TRUE);
		edit_allow_preset_outline(FALSE);
		if (!ready_Xcurve())
			{
			Hstart = TRUE;
			if (drawing_Xcurve()) return HoleDraw;
			(void) draw_Xcurve(DnEdit, FilterRes, SplineRes, TRUE);
			if (!ready_Xcurve()) return HoleDraw;
			}
		Hstart = FALSE;
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

#		ifdef DEBUG
		pr_diag("Editor", "[edit_addhole_area] Analysing hole\n");
#		endif /* DEBUG */

		/* A new hole has been drawn */
		busy_cursor(TRUE);
		put_message("area-hole-proc");

		/* Extract new segment and clean up the buffer */
		if (!Hnew)  Hnew  = create_line();	else empty_line(Hnew);
		append_line(Hnew, lines[0]);
		reset_pipe();

		/* Make sure at least one point on hole is inside the area */
		in = hole_inside_area(Hpick, Hnew);
		if (!in)
			{
			put_message("area-hole-out");
			empty_line(Hnew);
			(void) sleep(1);
			busy_cursor(FALSE);
			return HoleReDraw;
			}

		busy_cursor(FALSE);
		clear_message();
		return HoleAdd;
		}
	}

/***********************************************************************
*                                                                      *
*     m o v e _ a r e a                                                *
*                                                                      *
***********************************************************************/

static	LOGICAL	matching_areas(AREA, AREA);
static	LOGICAL	area_inside_outline(AREA, CURVE);

/**********************************************************************/

LOGICAL	edit_move_area

	(
	STRING	mode,
	STRING	name
	)

	{
	AREA	area, earea, copy;
	SPOT	spot, tspot;
	SET		set, tset;
	CURVE	outline;
	int		nlines;
	LINE	*lines;
	int		im, imx, imp, ims, ime, il, ilx, dim, iea, ii, dformat, ifrom;
	LOGICAL	top, *pmatch;
	POINT	p0, p1, pz;
	MARK	mark0, mark1;
	CURVE	curv0, curv1;
	float	dx, dy, dx0, dy0, angle, cang, sang;
	int		xnum;
	AREA	*xareas;
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
	/* - list of areas and area labels to be moved */
	/* - cmove/smove are pointers into EditAreas/EditLabs */
	/* - ccopy/scopy are copies to be displayed in TempMeta */
	static	const int	maxmove = 10000;
	static	int			nmove   = 0;
	static	int			amove   = 0;
	static	AREA		*cmove  = NullAreaList;
	static	AREA		*ccopy  = NullAreaList;
	static	int			nlabs   = 0;
	static	int			alabs   = 0;
	static	SPOT		*smove  = NullSpotList;
	static	SPOT		*scopy  = NullSpotList;
	static	int			*sids   = NullInt;

	/* Centre of rotation and angle of rotation */
	static	POINT		Cpos    = ZERO_POINT;
	static	float		Ang     = 0.0;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_move_area] %s  State: %d\n", mode, State);
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

							/* Empty area label list */
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
								"[edit_move_area] Cancel ... empty TempMeta %d fields\n",
								TempMeta->numfld);
#							endif /* DEBUG_STRUCTURES */

							/* Empty display structure */
							empty_temp();
							pasting = FALSE;
							mstart  = FALSE;
							clear_message();

							edit_select(NullCal, FALSE);
							edit_can_copy(FALSE);
							if (copy_posted("areas", CurrElement, CurrLevel))
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

		/* Empty move list and area label list*/
		if (nmove > 0)
			{

#			ifdef DEBUG_STRUCTURES
			for (im=0; im<nmove; im++)
				pr_diag("Editor",
					"[edit_move_area]   Cancel: %2d ... cmove/ccopy: %x %x\n",
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
					"[edit_move_area]   Cancel: %2d ... smove/scopy/sids: %x %x %d\n",
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
			"[edit_move_area] Cancel ... empty TempMeta %d fields\n",
			TempMeta->numfld);
#		endif /* DEBUG_STRUCTURES */

		/* Empty display structure */
		empty_temp();
		pasting = FALSE;
		mstart  = FALSE;
		clear_message();

		edit_select(NullCal, FALSE);
		edit_can_copy(FALSE);
		if (copy_posted("areas", CurrElement, CurrLevel))
			edit_can_paste(TRUE);
		return FALSE;
		}
	if (same(mode, "clear"))
		{
		/* >>>>> clear the paste buffer??? <<<<<< */
		State = Pick;
		return FALSE;
		}

	/* Make sure the area set contains the right stuff */
	if (!EditAreas)                     return FALSE;
	if (!same(EditAreas->type, "area")) return FALSE;

	/* Construct temporary fields and buffers on startup only */
	if (same(mode, "begin"))
		{
		ignore_partial();

#		ifdef DEBUG_STRUCTURES
		pr_diag("Editor",
			"[edit_move_area]   Begin: nmove/nlabs: %d %d\n", nmove, nlabs);
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
			"[edit_move_area] Begin ... empty TempMeta %d fields\n",
			TempMeta->numfld);
#		endif /* DEBUG_STRUCTURES */

		/* Empty display structure */
		empty_temp();
		pasting = FALSE;
		mstart  = FALSE;
		mlfirst = TRUE;

		edit_select(NullCal, FALSE);
		edit_can_copy(FALSE);
		if (copy_posted("areas", CurrElement, CurrLevel))
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
				put_message("area-move-nomode");
				(void) sleep(1);
				}
			mlfirst = FALSE;
			}

		/* Change move mode and remove labels */
		else if (mlabels)
			{

			/* Remove each area label from the list */
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

			/* Re-display the picked areas */
			present_all();

			/* Reset check for move mode */
			mlabels = FALSE;
			mlfirst = TRUE;
			}

		/* Change move mode and add labels */
		else
			{

			/* Find labels inside picked areas and add them to the list */
			if (nmove > 0 && NotNull(EditLabs))
				{

				/* Loop through all labels in set */
				nlabs = 0;
				for (ilx=0; ilx<EditLabs->num; ilx++)
					{
					spot = (SPOT) EditLabs->list[ilx];
					if (!spot) continue;
					if (same(spot->mclass, "legend")) continue;

					/* Check if label is inside a picked area */
					xnum = enclosing_area_list(EditAreas, spot->anchor,
											   PickFirst, &xareas);
					for (ii=0; ii<xnum; ii++)
						{
						for (im=0; im<nmove; im++)
							{
							if (xareas[ii] == cmove[im]) break;
							}
						if (im < nmove)
							{

							/* Highlight each area label */
							/*  and add it to the list   */
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
				}

			/* Re-display the picked areas and area labels */
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
		put_message("area-no-pick");
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
		put_message("area-no-pick");
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
					"[edit_move_area] No outline drawn yet!\n");
				}
			else
				{
				put_message("edit-no-preset-outline", name);
				pr_error("Editor",
					"[edit_move_area] No preset map outline: \"%s\"\n", name);
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

			/* Pick areas to move */
			case Pick:

				/* See if there are any areas to move */
				if (EditAreas->num <= 0)
					{
					if (copy_posted("areas", CurrElement, CurrLevel))
						edit_can_paste(TRUE);
					put_message("area-no-move");
					edit_select(NullCal, FALSE);
					return drawn;
					}

				/* Allow specification of multiple moves */
				while (nmove < maxmove)
					{
					/* Put up a prompt and set states */
					if (nmove <= 0)
						{
						put_message("area-move-pick");
						edit_can_copy(FALSE);
						if (copy_posted("areas", CurrElement, CurrLevel))
							edit_can_paste(TRUE);
						edit_select(NullCal, FALSE);
						ignore_partial();
						}
					else
						{
						put_message("area-move-pick2");
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
					put_message("area-pick");
					if (!pick_Xpoint(DnEdit, 0, p1, &butt)) continue;
					if (!inside_dn_window(DnEdit, p1))
						{
						put_message("edit-outside-map");
						(void) sleep(1);
						continue;
						}

					/* See which area we picked */
					area = closest_area(EditAreas, p1, NULL, NULL, NULL, &imp,
									NULL);
					if (!area)
						{
						put_message("area-no-pick");
						(void) sleep(1);
						continue;
						}

					/* If area was already picked, un-pick it */
					for (im=0; im<nmove; im++)
						{
						if (area == cmove[im]) break;
						}
					if (im < nmove)
						{
						put_message("area-unpicked");

						/* Remove the picked area from the list */
						set = find_mf_set(TempMeta, "area", "b",
										  "mmv", "mmv");
						(void) remove_item_from_set(set, (ITEM) ccopy[im]);
						nmove--;
						for (imx=im; imx<nmove; imx++)
							{
							cmove[imx] = cmove[imx+1];
							ccopy[imx] = ccopy[imx+1];
							}
						cmove[imx] = NullArea;
						ccopy[imx] = NullArea;

						/* Remove labels inside this area */
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

									/* Is label inside another picked area? */
									xnum = enclosing_area_list(set,
											smove[il]->anchor, PickFirst,
											&xareas);
									for (ii=0; ii<xnum; ii++)
										{
										for (imx=0; imx<nmove; imx++)
											{
											if (xareas[ii] == ccopy[imx]) break;
											}
										/* Found another picked area! */
										if (imx < nmove)
											{
											sids[il] = imx;
											break;
											}
										}

									/* Label not inside another picked area */
									/*  ... so remove it from the list      */
									if (ii >= xnum)
										{
										tset = find_mf_set(TempMeta, "spot",
														   "d", "smv", "smv");
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
							}

						/* Display remaining picked areas and area labels */
						present_all();
						pasting = FALSE;
						}

					/* Otherwise pick it */
					else
						{
						put_message("area-picked");
						nmove++;
						if (nmove > amove)
							{
							amove = nmove;
							cmove = GETMEM(cmove, AREA, amove);
							ccopy = GETMEM(ccopy, AREA, amove);
							}

						/* Determine stacking order to insert item into set */
						for (im=0; im<nmove-1; im++)
							{
							if (imp < which_set_item(EditAreas,
														(ITEM) cmove[im]) )
								{
								/* Shuffle other items to make space */
								for (ii=nmove-1; ii>im; ii--)
									{
									cmove[ii] = cmove[ii-1];
									ccopy[ii] = ccopy[ii-1];
									}
								/* Reset label identifiers as well */
								for (il=0; il<nlabs; il++)
									{
									if (sids[il] >= im) sids[il]++;
									}
								break;
								}
							}

						/* Produce a copy of the picked area */
						cmove[im] = area;
						ccopy[im] = copy_area(area, TRUE);
						copy      = ccopy[im];

						/* Highlight the picked area and add it to the list */
						highlight_area(copy, 2, -1);
						widen_area(copy, 2.0);

						add_item_to_metafile(TempMeta, "area", "b",
											 "mmv", "mmv", (ITEM) copy);

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[edit_move_area] Pick ... adding area (%d) to TempMeta (%d)\n",
							im, TempMeta->numfld);
#						endif /* DEBUG_STRUCTURES */

						/* Check for labels inside picked area */
						if (mlabels && NotNull(EditLabs))
							{
							for (ilx=0; ilx<EditLabs->num; ilx++)
								{
								spot = (SPOT) EditLabs->list[ilx];
								if (!spot) continue;
								if (same(spot->mclass, "legend")) continue;

								/* Is label already in list? */
								for (il=0; il<nlabs; il++)
									{
									if (smove[il] == spot) break;
									}
								if (il < nlabs) continue;

								/* Is label inside picked area? */
								xnum = enclosing_area_list(EditAreas,
														   spot->anchor,
														   PickFirst, &xareas);
								for (ii=0; ii<xnum; ii++)
									{
									if (xareas[ii] == area) break;
									}
								if (ii < xnum)
									{
									nlabs++;
									if (nlabs > alabs)
										{
										alabs = nlabs;
										smove = GETMEM(smove, SPOT, alabs);
										scopy = GETMEM(scopy, SPOT, alabs);
										sids  = GETMEM(sids,  int,  alabs);
										}

									/* Produce a copy of the area label */
									smove[nlabs-1] = spot;
									scopy[nlabs-1] = copy_spot(spot);
									sids[nlabs-1]  = im;
									tspot          = scopy[nlabs-1];

									/* Highlight the area label */
									/*  and add it to the list  */
									highlight_item("spot", (ITEM) tspot, 2);
									add_item_to_metafile(TempMeta, "spot", "d",
														 "smv", "smv",
														 (ITEM) tspot);

#									ifdef DEBUG_STRUCTURES
									pr_diag("Editor",
										"[edit_move_area] Pick ... adding spot (%d) to TempMeta (%d)\n",
										il, TempMeta->numfld);
#									endif /* DEBUG_STRUCTURES */
									}
								}
							}

						/* Display picked areas and area labels */
						present_temp(TRUE);
						pasting = FALSE;
						}
					}

				pr_warning("Editor",
					"[edit_move_area] End of State = Pick loop!\n");

				/* Have we picked any areas? */
				if (nmove > 0) edit_select((CAL) cmove[0]->attrib, FALSE);
				else           edit_select(NullCal, FALSE);

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Pick all areas to move */
			case PickAll:

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_move_area] PickAll ... empty TempMeta %d field(s)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();

				put_message("area-all-picked");
				nmove = EditAreas->num;
				if (nmove > amove)
					{
					amove = nmove;
					cmove = GETMEM(cmove, AREA, amove);
					ccopy = GETMEM(ccopy, AREA, amove);
					}

				/* Highlight all the areas and add them to the list */
				for (im=0; im<nmove; im++)
					{
					cmove[im] = (AREA) EditAreas->list[im];
					ccopy[im] = copy_area(cmove[im], TRUE);
					copy      = ccopy[im];
					highlight_area(copy, 2, -1);
					widen_area(copy, 2.0);

					add_item_to_metafile(TempMeta, "area", "b",
										 "mmv", "mmv", (ITEM) copy);

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_move_area] PickAll ... adding area (%d) to TempMeta (%d)\n",
						im, TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */
					}

				/* Find labels inside picked areas and add them to the list */
				if (nmove > 0 && mlabels && NotNull(EditLabs))
					{

					/* Loop through all labels in set */
					nlabs = 0;
					for (ilx=0; ilx<EditLabs->num; ilx++)
						{
						spot = (SPOT) EditLabs->list[ilx];
						if (!spot) continue;
						if (same(spot->mclass, "legend")) continue;

						/* Check if label is inside a picked area */
						xnum = enclosing_area_list(EditAreas, spot->anchor,
												   PickFirst, &xareas);
						for (ii=0; ii<xnum; ii++)
							{
							for (im=0; im<nmove; im++)
								{
								if (xareas[ii] == cmove[im]) break;
								}
							if (im < nmove)
								{

								/* Highlight each area label */
								/*  and add it to the list   */
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

#								ifdef DEBUG_STRUCTURES
								pr_diag("Editor",
									"[edit_move_area] PickAll ... adding spot (%d) to TempMeta (%d)\n",
									il, TempMeta->numfld);
#								endif /* DEBUG_STRUCTURES */
								}
							}
						}
					}

				/* Display picked areas and area labels */
				present_temp(TRUE);
				pasting = FALSE;

				/* Have we picked any areas? */
				if (nmove > 0) edit_select((CAL) cmove[0]->attrib, FALSE);
				else           edit_select(NullCal, FALSE);

#				ifdef DEBUG_STRUCTURES
				for (im=0; im<nmove; im++)
					pr_diag("Editor",
						"[edit_move_area]   PickAll: %2d ... cmove/ccopy: %x %x\n",
					im, cmove[im], ccopy[im]);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Draw outline around areas to move */
			case Draw:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_move_area] Drawing outline\n");
#				endif /* DEBUG */

				/* Draw the outline */
				post_partial(FALSE);
				edit_allow_preset_outline(FALSE);
				put_message("area-move-draw");
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

			/* Pick areas to move from inside drawn outline */
			case DrawPick:

				if (IsNull(outline))
					{
					edit_select(NullCal, FALSE);
					State = Pick;
					continue;
					}

#				ifdef DEBUG
				pr_diag("Editor",
					"[edit_move_area] Picking areas inside outline\n");
#				endif /* DEBUG */

				busy_cursor(TRUE);
				edit_allow_preset_outline(FALSE);
				put_message("area-move-select");
				(void) sleep(1);

				/* Pick areas inside drawn outline */
				for (iea=0; iea<EditAreas->num; iea++)
					{
					earea = (AREA) EditAreas->list[iea];
					if (!earea) continue;

					/* Check if area is inside drawn outline */
					if (!area_inside_outline(earea, outline)) continue;

					/* Pick areas that are not already picked */
					for (im=0; im<nmove; im++)
						{
						if (earea == cmove[im]) break;
						}
					if (im >= nmove)
						{
						put_message("area-picked");
						nmove++;
						if (nmove > amove)
							{
							amove = nmove;
							cmove = GETMEM(cmove, AREA, amove);
							ccopy = GETMEM(ccopy, AREA, amove);
							}

						/* Determine stacking order to insert item into set */
						for (im=0; im<nmove-1; im++)
							{
							if (iea < which_set_item(EditAreas,
														(ITEM) cmove[im]) )
								{
								/* Shuffle other items to make space */
								for (ii=nmove-1; ii>im; ii--)
									{
									cmove[ii] = cmove[ii-1];
									ccopy[ii] = ccopy[ii-1];
									}
								/* Reset label identifiers as well */
								for (il=0; il<nlabs; il++)
									{
									if (sids[il] >= im) sids[il]++;
									}
								break;
								}
							}

						/* Produce a copy of the picked area */
						cmove[im] = earea;
						ccopy[im] = copy_area(earea, TRUE);
						copy      = ccopy[im];

						/* Highlight the picked area and add it to the list */
						highlight_area(copy, 2, -1);
						widen_area(copy, 2.0);

						add_item_to_metafile(TempMeta, "area", "b",
											 "mmv", "mmv", (ITEM) copy);

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[edit_move_area] DrawPick ... adding area (%d) to TempMeta (%d)\n",
							im, TempMeta->numfld);
#						endif /* DEBUG_STRUCTURES */

						/* Check for labels inside picked area */
						if (mlabels && NotNull(EditLabs))
							{
							for (ilx=0; ilx<EditLabs->num; ilx++)
								{
								spot = (SPOT) EditLabs->list[ilx];
								if (!spot) continue;
								if (same(spot->mclass, "legend")) continue;

								/* Is label already in list? */
								for (il=0; il<nlabs; il++)
									{
									if (smove[il] == spot) break;
									}
								if (il < nlabs) continue;

								/* Is label inside picked area? */
								xnum = enclosing_area_list(EditAreas,
														   spot->anchor,
														   PickFirst, &xareas);
								for (ii=0; ii<xnum; ii++)
									{
									if (xareas[ii] == earea) break;
									}
								if (ii < xnum)
									{
									nlabs++;
									if (nlabs > alabs)
										{
										alabs = nlabs;
										smove = GETMEM(smove, SPOT, alabs);
										scopy = GETMEM(scopy, SPOT, alabs);
										sids  = GETMEM(sids,  int,  alabs);
										}

									/* Produce a copy of the area label */
									smove[nlabs-1] = spot;
									scopy[nlabs-1] = copy_spot(spot);
									sids[nlabs-1]  = im;
									tspot          = scopy[nlabs-1];

									/* Highlight the area label */
									/*  and add it to the list  */
									highlight_item("spot", (ITEM) tspot, 2);
									add_item_to_metafile(TempMeta, "spot", "d",
														 "smv", "smv",
														 (ITEM) tspot);

#									ifdef DEBUG_STRUCTURES
									pr_diag("Editor",
										"[edit_move_area] DrawPick ... adding spot (%d) to TempMeta (%d)\n",
										il, TempMeta->numfld);
#									endif /* DEBUG_STRUCTURES */
									}
								}
							}
						}
					}

				/* Display picked areas and area labels */
				present_temp(TRUE);
				busy_cursor(FALSE);

				/* Have we picked any areas? */
				if (nmove > 0) edit_select((CAL) cmove[0]->attrib, FALSE);
				else           edit_select(NullCal, FALSE);

				/* Re-display the picked areas */
				State = ReDisplay;
				continue;

			/* Cut areas */
			case Cut:

				/* Save the areas in the copy buffer */
				put_message("edit-cut");
				post_area_copy(CurrElement, CurrLevel, nmove, cmove,
							   nlabs, smove, sids);

				/* Remove areas from area set */
				for (im=0; im<nmove; im++)
					{
					area  = cmove[im];
					ifrom = which_set_item(EditAreas, (ITEM) area);
					(void) remove_item_from_set(EditAreas, (ITEM) area);
					(void) adjust_area_link_nodes(ActiveDfld,
													EditTime, ifrom, -1);
					cmove[im] = NullArea;
					}

				/* Remove labels inside areas (if requested) */
				if (mlabels)
					{
					for (il=0; il<nlabs; il++)
						{
						spot = smove[il];
						(void) remove_item_from_set(EditLabs, (ITEM) spot);
						smove[il] = NullSpot;
						}
					}

				if (EditUndoable) post_mod("areas");

				/* Modify labels if necessary */
				if (recompute_areaset_labs(EditAreas, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				/* Special case if ALL the area labels were just cut! */
				else if (nlabs > 0)
					{
					if (EditUndoable) post_mod("labs");
					}

				if (EditUndoable) (void) extract_area_order_tags(FALSE);

				/* Empty move list */
				if (nmove > 0)
					{

#					ifdef DEBUG_STRUCTURES
					for (im=0; im<nmove; im++)
						pr_diag("Editor",
							"[edit_move_area]   Cut: %2d ... cmove/ccopy: %x %x\n",
						im, cmove[im], ccopy[im]);
#					endif /* DEBUG_STRUCTURES */

					nmove = 0;
					amove = 0;
					FREEMEM(cmove);
					FREEMEM(ccopy);
					}

				/* Empty area label list */
				if (nlabs > 0)
					{

#					ifdef DEBUG_STRUCTURES
					for (il=0; il<nlabs; il++)
						pr_diag("Editor",
							"[edit_move_area]   Cut: %2d ... smove/scopy/sids: %x %x %d\n",
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
					"[edit_move_area] Cut ... empty TempMeta %d field(s)\n",
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

			/* Copy areas */
			case Copy:

				/* Save the areas and the area labels in the copy buffer */
				put_message("edit-copy");
				post_area_copy(CurrElement, CurrLevel, nmove, cmove,
							   nlabs, smove, sids);
				clear_message();
				pasting = FALSE;

				/* Re-display the picked areas */
				State = ReDisplay;
				continue;

			/* Paste areas */
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
							"[edit_move_area]   Paste: %2d ... cmove/ccopy: %x %x\n",
						im, cmove[im], ccopy[im]);
#					endif /* DEBUG_STRUCTURES */

					nmove = 0;
					amove = 0;
					FREEMEM(cmove);
					FREEMEM(ccopy);

					/* Empty area label list */
					if (nlabs > 0)
						{

#						ifdef DEBUG_STRUCTURES
						for (il=0; il<nlabs; il++)
							pr_diag("Editor",
								"[edit_move_area]   Paste: %2d ... smove/scopy/sids: %x %x %d\n",
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
						"[edit_move_area] Paste ... empty TempMeta %d fields\n",
						TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */

					/* Empty display structure */
					empty_temp();
					}

				/* Retrieve paste buffer */
				paste_area_copy(CurrElement, CurrLevel, &nmove, &cmove,
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

				/* Determine order for copying based on stacking order */
				switch (StackMode)
					{
					case STACK_BOTTOM:
							ims = 0;
							ime = nmove-1;
							dim = +1;
							top = FALSE;
							break;
					case STACK_TOP:
							ims = nmove-1;
							ime = 0;
							dim = -1;
							top = TRUE;
							break;
					}

				/* Transfer paste buffer to move list and area set */
				if (nmove > amove)
					{
					amove = nmove;
					ccopy = GETMEM(ccopy, AREA, amove);
					}

				/* Check for pasting duplicate areas */
				pmatch = INITMEM(LOGICAL, nmove);
				for (im=0; im<nmove; im++)
					{

					/* Initialize match parameter */
					pmatch[im] = FALSE;

					/* Check for matching areas */
					area = cmove[im];
					for (iea=0; iea<EditAreas->num; iea++)
						{
						earea = (AREA) EditAreas->list[iea];
						if (!earea) continue;

						/* Identify areas pasted over themselves */
						if (matching_areas(area, earea))
							{
							pmatch[im] = TRUE;
							break;
							}
						}
					}

				/* Add pasted areas to area set */
				for (im=ims; ; im+=dim)
					{
					if ( top && im<ime) break;
					if (!top && im>ime) break;

					/* Offset areas pasted over themselves */
					area = cmove[im];
					if (pmatch[im])
						{
						translate_area(area, PickTol, PickTol);
						}

					/* Add pasted area to area set */
					switch (StackMode)
						{
						case STACK_BOTTOM:
								(void) add_item_to_set(EditAreas, (ITEM) area);
								break;
						case STACK_TOP:
								(void) add_item_to_set_start(EditAreas,
															(ITEM) area);
								(void) adjust_area_link_nodes(ActiveDfld,
															EditTime, -1, 0);
								break;
						}
					ccopy[im] = copy_area(area, TRUE);
					copy      = ccopy[im];

					/* Highlight the picked area and add it to the list */
					highlight_area(copy, 2, 1);
					widen_area(copy, 2.0);

					add_item_to_metafile(TempMeta, "area", "b",
										 "mmv", "mmv", (ITEM) copy);

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_move_area] Paste ... adding area (%d) to TempMeta (%d)\n",
						im, TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */
					}

#				ifdef DEBUG_STRUCTURES
				for (im=0; im<nmove; im++)
					pr_diag("Editor",
						"[edit_move_area]   Paste: %2d ... cmove/ccopy: %x %x\n",
					im, cmove[im], ccopy[im]);
#				endif /* DEBUG_STRUCTURES */

				/* Then add pasted labels to area label list (if requested) */
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

						/* Offset labels for areas pasted over themselves */
						if (pmatch[sids[il]])
							{
							spot->anchor[X] += PickTol;
							spot->anchor[Y] += PickTol;
							}

						/* Add pasted area label to area label set */
						(void) add_item_to_set(EditLabs, (ITEM) spot);
						scopy[il] = copy_spot(spot);
						tspot     = scopy[il];

						/* Highlight the area label and add it to the list */
						highlight_item("spot", (ITEM) tspot, 2);
						add_item_to_metafile(TempMeta, "spot", "d",
											 "smv", "smv", (ITEM) tspot);

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[edit_move_area] Paste ... adding spot (%d) to TempMeta (%d)\n",
							il, TempMeta->numfld);
#						endif /* DEBUG_STRUCTURES */
						}

#					ifdef DEBUG_STRUCTURES
					for (il=0; il<nlabs; il++)
						pr_diag("Editor",
							"[edit_move_area]   Paste: %2d ... smove/scopy/sids: %x %x %d\n",
						il, smove[il], scopy[il], sids[il]);
#					endif /* DEBUG_STRUCTURES */
					}

				if (EditUndoable) post_mod("areas");

				/* Modify labels if necessary */
				if (recompute_areaset_labs(EditAreas, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				if (EditUndoable) (void) extract_area_order_tags(FALSE);

				/* Free match buffer */
				FREEMEM(pmatch);

				/* Re-display the areas */
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

				/* Highlight the picked areas and area labels */
				mstart = TRUE;
				for (im=0; im<nmove; im++)
					{
					copy = ccopy[im];
					highlight_area(copy, 3, -1);
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

				/* Re-display the areas */
				present_all();
				post_partial(TRUE);
				edit_can_copy(TRUE);

				/* Pick a reference point */
				put_message("area-tran");
				if (!ready_Xpoint(DnEdit, p0, &butt)) return drawn;
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!inside_dn_window(DnEdit, p0))
					{
					put_message("area-tran-out1");
					(void) ignore_Xpoint();
					continue;
					}

				/* Highlight the areas (without holes) to translate */
				set = copy_mf_set(TempMeta, "area", "b", "mmv", "mmv");
				for (im=0; im<set->num; im++)
					{
					area = (AREA) set->list[im];
					remove_all_area_holes(area);
					highlight_area(area, 2, -1);
					widen_area(area, 2.0);
					}
				dformat = field_display_format(CurrElement, CurrLevel);
				prep_set(set, dformat);

				/* Translate the reference point */
				put_message("area-tran-release");
				(void) utrack_Xpoint(DnEdit, set, p0, p1, &butt);
				set = destroy_set(set);
				if (!inside_dn_window(DnEdit, p1))
					{
					put_message("area-tran-out2");
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
				define_lspec(&curv0->lspec, 0, 0, NULL, False, 2.0, 0.0, 2);

				add_item_to_metafile(TempMeta, "mark",  "d", "", "",
									 (ITEM) mark0);
				add_item_to_metafile(TempMeta, "mark",  "d", "", "",
									 (ITEM) mark1);
				add_item_to_metafile(TempMeta, "curve", "c", "", "",
									 (ITEM) curv0);
				present_temp(TRUE);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_move_area] Adding mark/mark/curve to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = TranslateDone;
				continue;

			/* Translate the areas */
			case TranslateDone:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_move_area] Translating\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);

				/* Translate the areas */
				dx = p1[X] - p0[X];
				dy = p1[Y] - p0[Y];
				for (im=0; im<nmove; im++)
					{
					area = cmove[im];
					translate_area(area, dx, dy);
					ccopy[im] = NullArea;
					}

				/* Then translate the area labels (if requested) */
				if (mlabels)
					{
					(void) spot_list_translate(nlabs, smove, dx, dy);
					for (il=0; il<nlabs; il++)
						{
						scopy[il] = NullSpot;
						}
					}

				if (EditUndoable) post_mod("areas");

				/* Modify labels if necessary */
				if (recompute_areaset_labs(EditAreas, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				if (EditUndoable) (void) extract_area_order_tags(FALSE);

				put_message("edit-done");
				pasting = FALSE;
				mstart  = FALSE;
				mlfirst = TRUE;
				busy_cursor(FALSE);

				/* Re-display the picked areas */
				State = ReDisplay;
				continue;

			/* Pick the centre of rotation */
			case Centre:

				/* Highlight the picked areas and area labels */
				mstart = TRUE;
				for (im=0; im<nmove; im++)
					{
					copy = ccopy[im];
					highlight_area(copy, 3, -1);
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

				/* Re-display the areas */
				present_all();
				post_partial(TRUE);
				edit_can_copy(TRUE);

				/* Pick the centre of rotation */
				put_message("area-rot-centre");
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
					put_message("area-rot-out");
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
					"[edit_move_area] Adding mark to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = Rotate;
				continue;

			/* Pick and rotate a reference point */
			case Rotate:

				/* Pick a reference point */
				put_message("area-rot");
				if (!ready_Xpoint(DnEdit, p0, &butt)) return drawn;
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!inside_dn_window(DnEdit, p0))
					{
					put_message("area-rot-out1");
					(void) ignore_Xpoint();
					continue;
					}
				if (point_dist(Cpos, p0) < SplineRes)
					{
					put_message("area-rot-ref");
					(void) ignore_Xpoint();
					(void) sleep(1);
					continue;
					}

				/* Highlight the areas (without holes) to rotate */
				set = copy_mf_set(TempMeta, "area", "b", "mmv", "mmv");
				for (im=0; im<set->num; im++)
					{
					area = (AREA) set->list[im];
					remove_all_area_holes(area);
					highlight_area(area, 2, -1);
					widen_area(area, 2.0);
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
					"[edit_move_area] Adding mark/curve to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Rotate the reference point */
				put_message("area-rot-release");
				(void) urotate_Xpoint(DnEdit, set, Cpos, p0, p1, &Ang, &butt);
				set = destroy_set(set);
				if (!inside_dn_window(DnEdit, p1))
					{
					put_message("area-rot-out2");
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
					"[edit_move_area] Adding mark/curve to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = RotateDone;
				continue;

			/* Rotate the areas */
			case RotateDone:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_move_area] Rotating\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);

				/* Rotate the areas */
				dx0 = p0[X] - Cpos[X];
				dy0 = p0[Y] - Cpos[Y];
				for (im=0; im<nmove; im++)
					{
					area = cmove[im];
					rotate_area(area, Cpos, Ang/RAD);
					ccopy[im] = NullArea;
					}

				/* Then rotate the area labels (if requested) */
				if (mlabels)
					{
					(void) spot_list_rotate(nlabs, smove, Cpos, Ang/RAD);
					for (il=0; il<nlabs; il++)
						{
						scopy[il] = NullSpot;
						}
					}

				if (EditUndoable) post_mod("areas");

				/* Modify labels if necessary */
				if (recompute_areaset_labs(EditAreas, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				if (EditUndoable) (void) extract_area_order_tags(FALSE);

				put_message("edit-done");
				pasting = FALSE;
				mstart  = FALSE;
				mlfirst = TRUE;
				busy_cursor(FALSE);

				/* Re-display the picked areas */
				State = ReDisplay;
				continue;

			/* Re-display the picked areas */
			case ReDisplay:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_move_area] Redisplay\n");
#				endif /* DEBUG */

				/* Empty display structure */
				empty_temp();

				/* Re-pick the picked areas */
				for (im=0; im<nmove; im++)
					{
					ccopy[im] = copy_area(cmove[im], TRUE);
					copy      = ccopy[im];

					/* Highlight the picked area */
					highlight_area(copy, 2, -1);
					widen_area(copy, 2.0);

					add_item_to_metafile(TempMeta, "area", "b",
										 "mmv", "mmv", (ITEM) copy);

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_move_area] Redisplay ... adding area (%d) to TempMeta (%d)\n",
						im, TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */
					}

				/* Check if move mode has changed */
				if ( (mlabels && MoveMode == MOVE_FIELD)
						|| (!mlabels && MoveMode == MOVE_FIELD_AND_LABELS) )
					{

					/* Change move mode and empty area label list */
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

					/* Change move mode and add labels to area label list */
					else
						{

						/* Match labels to picked areas */
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

								/* Check if label is inside a picked area */
								xnum = enclosing_area_list(EditAreas,
														   spot->anchor,
											   			   PickFirst, &xareas);
								for (ii=0; ii<xnum; ii++)
									{
									for (im=0; im<nmove; im++)
										{
										if (xareas[ii] == cmove[im]) break;
										}
									if (im < nmove)
										{

										/* Add the area label to the list */
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
							}

						/* Reset check for move mode */
						mlabels = TRUE;
						mlfirst = TRUE;
						}
					}

				/* Then re-pick the area labels (if requested) */
				if (mlabels)
					{
					for (il=0; il<nlabs; il++)
						{

						/* Highlight the area label */
						scopy[il] = copy_spot(smove[il]);
						spot      = scopy[il];
						highlight_item("spot", (ITEM) spot, 2);
						add_item_to_metafile(TempMeta, "spot", "d",
											 "smv", "smv", (ITEM) spot);

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[edit_move_area] Redisplay ... adding spot (%d) to TempMeta (%d)\n",
							il, TempMeta->numfld);
#						endif /* DEBUG_STRUCTURES */
						}
					}

				/* Re-display the areas and area labels */
				present_all();
				post_partial(FALSE);

				/* Move on to next stage */
				State = Pick;
				continue;
			}
		}
	}

/**********************************************************************/

static	LOGICAL	matching_areas

	(
	AREA	a1,
	AREA	a2
	)

	{
	LOGICAL	fwd;
	LINE	l1, l2;
	int		np, ii, ix, ih, id;
	float	x1, y1, x2, y2, xr, yr;

	if (!a1)                  return FALSE;
	if (!a1->bound)           return FALSE;
	if (!a1->bound->boundary) return FALSE;
	if (!a2)                  return FALSE;
	if (!a2->bound)           return FALSE;
	if (!a2->bound->boundary) return FALSE;

	/* Check boundaries */
	l1 = a1->bound->boundary;
	l2 = a2->bound->boundary;
	if (l1->numpts != l2->numpts) return FALSE;

	np = l1->numpts;
	x1 = l1->points[0][X];  x2 = l2->points[0][X];  xr = l2->points[np-1][X];
	y1 = l1->points[0][Y];  y2 = l2->points[0][Y];  yr = l2->points[np-1][Y];
	if ((x1 != x2 || y1 != y2) && (x1 != xr || y1 != yr)) return FALSE;
	if      (x1 == x2 && y1 == y2) fwd = TRUE;
	else if (x1 == xr && y1 == yr) fwd = FALSE;

	for (ii=1; ii<np; ii++)
		{
		if (fwd) ix = ii;
		else     ix = np-ii-1;
		x1 = l1->points[ii][X]; x2 = l2->points[ix][X];
		y1 = l1->points[ii][Y]; y2 = l2->points[ix][Y];
		if (x1 != x2 || y1 != y2) return FALSE;
		}

	/* Check holes */
	if (a1->bound->numhole != a2->bound->numhole) return FALSE;
	for (ih=0; ih<a1->bound->numhole; ih++)
		{
		l1 = a1->bound->holes[ih];
		l2 = a2->bound->holes[ih];
		if (l1->numpts != l2->numpts) return FALSE;

		np = l1->numpts;
		x1 = l1->points[0][X]; x2 = l2->points[0][X]; xr = l2->points[np-1][X];
		y1 = l1->points[0][Y]; y2 = l2->points[0][Y]; yr = l2->points[np-1][Y];
		if ((x1 != x2 || y1 != y2) && (x1 != xr || y1 != yr)) return FALSE;
		if      (x1 == x2 && y1 == y2) fwd = TRUE;
		else if (x1 == xr && y1 == yr) fwd = FALSE;

		for (ii=1; ii<np; ii++)
			{
			if (fwd) ix = ii;
			else     ix = np-ii-1;
			x1 = l1->points[ii][X]; x2 = l2->points[ix][X];
			y1 = l1->points[ii][Y]; y2 = l2->points[ix][Y];
			if (x1 != x2 || y1 != y2) return FALSE;
			}
		}

	/* Check dividing lines */
	if (a1->numdiv != a2->numdiv) return FALSE;
	for (id=0; id<a1->numdiv; id++)
		{
		l1 = a1->divlines[id];
		l2 = a2->divlines[id];
		if (l1->numpts != l2->numpts) return FALSE;

		np = l1->numpts;
		x1 = l1->points[0][X]; x2 = l2->points[0][X]; xr = l2->points[np-1][X];
		y1 = l1->points[0][Y]; y2 = l2->points[0][Y]; yr = l2->points[np-1][Y];
		if ((x1 != x2 || y1 != y2) && (x1 != xr || y1 != yr)) return FALSE;
		if      (x1 == x2 && y1 == y2) fwd = TRUE;
		else if (x1 == xr && y1 == yr) fwd = FALSE;

		for (ii=1; ii<np; ii++)
			{
			if (fwd) ix = ii;
			else     ix = np-ii-1;
			x1 = l1->points[ii][X]; x2 = l2->points[ix][X];
			y1 = l1->points[ii][Y]; y2 = l2->points[ix][Y];
			if (x1 != x2 || y1 != y2) return FALSE;
			}
		}

	return TRUE;
	}

/**********************************************************************/

static	LOGICAL	area_inside_outline

	(
	AREA	area,
	CURVE	outline
	)

	{
	int		ii;
	LOGICAL	inside;
	LINE	oline, bndry;

	if (!area)                       return FALSE;
	if (!area->bound)                return FALSE;
	if (!area->bound->boundary)      return FALSE;
	if (!outline)                    return FALSE;
	if (!outline->line)              return FALSE;
	if (!line_closed(outline->line)) return FALSE;

	/* Ensure that all points in area boundary are inside outline */
	oline = outline->line;
	bndry = area->bound->boundary;
	for (ii=0; ii<bndry->numpts; ii++)
		{
		line_test_point(oline, bndry->points[ii], NullFloat, NullPoint, NullInt,
						&inside, NullChar);
		if (!inside) return FALSE;
		}

	/* Return TRUE if all points in area boundary were inside outline */
	return TRUE;
	}

#ifdef OBSOLETE
/***********************************************************************
*                                                                      *
*     b a c k g r o u n d _ a r e a                                    *
*                                                                      *
***********************************************************************/

LOGICAL	edit_background_area

	(
	STRING	mode,
	CAL		cal
	)

	{
	LOGICAL	drawn = FALSE;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_background_area] %s\n", mode);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same_start(mode, "cancel"))
		{
		return FALSE;
		}
	if (same(mode, "clear"))
		{
		return FALSE;
		}

	/* Make sure the area set contains the right stuff */
	if (!EditAreas)                     return FALSE;
	if (!same(EditAreas->type, "area")) return FALSE;

	/* Freeze all pending edits so that only this one can be undone */
	if (EditUndoable)
		{
		put_message("edit-freeze");
		accept_mod();	/* Leaves EditAreas alone */
		}

	busy_cursor(TRUE);
	put_message("area-bgnd");
	define_set_bg_attribs(EditAreas, cal);

	drawn = TRUE;
	if (EditUndoable) post_mod("areas");

	/* Modify labels if necessary */
	if (recompute_areaset_labs(EditAreas, EditLabs, EditRetain))
		{
		if (EditUndoable) post_mod("labs");
		}

	if (EditUndoable) (void) extract_area_order_tags(FALSE);

	/* Display the new area field */
	present_all();
	busy_cursor(FALSE);
	return drawn;
	}
#endif


/***********************************************************************
*                                                                      *
*     m o d i f y _ a r e a                                            *
*     m o d i f y _ a r e a _ p i c k                                  *
*     m o d i f y _ a r e a _ d r a w                                  *
*     m o d i f y _ a r e a _ d i s c a r d                            *
*     m o d i f y _ a r e a _ s c u l p t                              *
*     m o d i f y _ a r e a _ d r a w _ d i v                          *
*     m o d i f y _ a r e a _ d i s c a r d _ d i v                    *
*     m o d i f y _ a r e a _ s c u l p t _ d i v                      *
*     m o d i f y _ a r e a _ d r a w _ h o l e                        *
*     m o d i f y _ a r e a _ d i s c a r d _ h o l e                  *
*     m o d i f y _ a r e a _ s c u l p t _ h o l e                    *
*                                                                      *
***********************************************************************/

typedef	enum
			{
			ModPick, ModDraw, ModDiscard,			/* interactive states */
			ModSculpt,
			ModRePick, ModReDraw,					/* "cancel" states */
			ModSet, ModDelete, ModDeleteHole,		/* modification states */
			ModDrawDone, ModSculptDone, ModStack,
			ModDivideWarn, ModDivideGone,			/* "warning" states */
			ModHoleGone,
			ModConfirm, ModBailout					/* complete modification */
			}
		MODSTATE;

static	MODSTATE	modify_area_pick(void);
static	void		modify_area_select(LOGICAL, POINT);
static	void		modify_area_enhance(void);
static	MODSTATE	modify_area_draw(void);
static	MODSTATE	modify_area_draw_div(void);
static	MODSTATE	modify_area_draw_hole(void);
static	MODSTATE	modify_area_discard(void);
static	MODSTATE	modify_area_discard_div(void);
static	MODSTATE	modify_area_discard_hole(void);
static	MODSTATE	modify_area_sculpt(void);
static	MODSTATE	modify_area_sculpt_div(void);
static	MODSTATE	modify_area_sculpt_hole(void);

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
		case ModDeleteHole:	name = "DeleteHole";	break;
		case ModDrawDone:	name = "DrawDone";		break;
		case ModSculptDone:	name = "SculptDone";	break;
		case ModStack:		name = "Stack";			break;
		case ModDivideWarn:	name = "DivideWarn";	break;
		case ModDivideGone:	name = "DivideGone";	break;
		case ModHoleGone:	name = "HoleGone";		break;
		case ModConfirm:	name = "Confirm";		break;
		case ModBailout:	name = "BailOut";		break;
		default:			name = "!Unknown!";		break;
		}
	pr_diag("Editor", "[edit_modify_area] State: %s %s\n", pre, name);
#	endif /* DEBUG */

	return;
	}
/* For debug purposes */

static	LOGICAL	Mfresh = FALSE;		/* Just picked */
static	LOGICAL	Mdrawn = FALSE;		/* Line has been drawn */
static	LOGICAL	Mstart = FALSE;		/* Modify started (but not completed?) */
static	POINT	Mpos   = {0,0};		/* Picked point */
static	AREA	Mpick  = NullArea;	/* Picked area */
static	SUBAREA	Mpsub  = NullSubArea;
									/* Picked subarea */
static	AMEMBER	Mptype = AreaNone;	/* Which part of area was picked */
static	int		Mpmem  = -1;		/* Which dividing line, hole or subarea */
static	LOGICAL	Mpset  = FALSE;		/* Can we set area attributes? */
static	AREA	Mwork  = NullArea;	/* Working copy of picked area */
static	SUBAREA	Mwsub  = NullSubArea;
									/* Working copy of picked subarea */
static	CURVE	Mwdiv  = NullCurve;	/* Working copy of picked dividing line */
static	CURVE	Mwhole = NullCurve;	/* Working copy of picked hole */
static	LINE	Mbound = NullLine;	/* Working copy of picked boundary */
static	LINE	Mnew   = NullLine;	/* Newly drawn line segment */
static	LINE	MsegA  = NullLine;	/* First piece from working copy */
static	LINE	MsegB  = NullLine;	/* Second piece from working copy */
static	AREA	MareaA = NullArea;	/* New area constructed from 1st piece */
static	AREA	MareaB = NullArea;	/* New area constructed from 2nd piece */
static	AREA	MareaS = NullArea;	/* New area constructed from sculpting */
static	AREA	Makeep = NullArea;	/* Current new area to keep */
static	CURVE	Mbdiv  = NullCurve;	/* New constructed dividing line */
static	AREA	MareaX = NullArea;	/* Divided area up to new dividing line */
static	SUBAREA	Mxsub  = NullSubArea;
									/* Subarea divided by new dividing line */
static	LOGICAL	Mright = TRUE;		/* Side of dividing line last modified */
static	LOGICAL	Mtrunc = FALSE;		/* Dividing line has been truncated */
static	CURVE	MholeA = NullCurve;	/* New hole constructed from 1st piece */
static	CURVE	MholeB = NullCurve;	/* New hole constructed from 2nd piece */
static	CURVE	MholeS = NullCurve;	/* New hole constructed from sculpting */
static	CURVE	Mhkeep = NullCurve;	/* Current new hole to keep */
static	SET		Mtemp  = NullSet;	/* Set of areas in temp */
static	LINE	Lbox    = NullLine;
static	AREA	*Lmarks = NullAreaList;	/* Marker at each point of boundary */
										/*  or divide or hole */
static	float	Msize   = 0;			/* Marker size */
static	float	EdRad  = 150.0;
static	float	EdSpr  = 100.0;
static	float	EdTol  = 0.0;

/**********************************************************************/

LOGICAL	edit_modify_area

	(
	STRING	mode,
	STRING	order,
	CAL		cal
	)

	{
	int			ifrom, idest;
	MODSTATE	next;
	LOGICAL		drawn = FALSE;

	/* Current state: */
	/* - where should we resume? */
	static	MODSTATE	State = ModPick;

	/* Status of dividing lines */
	static	DIVSTAT		dstat;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_modify_area] %s  State: %d\n", mode, State);
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
								return FALSE;
			}
		}
	if (same(mode, "cancel all"))
		{
		ignore_partial();

		/* Clean up temporary buffers */
		edit_select(NullCal, FALSE);
		Mfresh = FALSE;
		Mdrawn = FALSE;
		Mpick  = NullArea;
		Mpsub  = NullSubArea;
		Mptype = AreaNone;
		Mpmem  = -1;
		Mpset  = FALSE;
		Mwork  = NullArea;
		Mwsub  = NullSubArea;
		Mwdiv  = NullCurve;
		Mwhole = NullCurve;
		Mbound = NullLine;
		Mtrunc = FALSE;
		Makeep = NullArea;
		MareaA = NullArea;
		MareaB = NullArea;
		MareaS = NullArea;
		Mbdiv  = NullCurve;
		Mhkeep = NullCurve;
		MholeA = NullCurve;
		MholeB = NullCurve;
		MholeS = NullCurve;
		Lmarks = NullAreaList;

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

	/* Make sure the area set contains the right stuff */
	if (!EditAreas)                     return FALSE;
	if (!same(EditAreas->type, "area")) return FALSE;

	/* Initialize on startup */
	if (same(mode, "begin"))
		{
		ignore_partial();
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
	if (same(mode, "stack"))
		{
		if (State == ModDraw)   State = ModStack;
		if (State == ModSculpt) State = ModStack;
		show_mod_state("STACK", State);
		}
	if (same(mode, "delete"))
		{
		if (State == ModDraw)   State = ModDelete;
		if (State == ModSculpt) State = ModDelete;
		show_mod_state("DELETE", State);
		}
	if (same(mode, "delete_hole"))
		{
		if (State == ModDraw)   State = ModDeleteHole;
		if (State == ModSculpt) State = ModDeleteHole;
		show_mod_state("DELETE_HOLE", State);
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

			/* Pick an area to modify */
			case ModPick:

				/* Pick an area */
				ignore_partial();
				next = modify_area_pick();
				if (next == ModPick) return drawn;

				/* Move on to next stage */
				State = next;
				show_mod_state("->", State);
				continue;

			/*  Repick the area to modify */
			case ModRePick:

				put_message("edit-cancel");

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor", "[edit_modify_area] ModRePick ...\n");
				pr_diag("Editor",
					"[edit_modify_area]   Mwork: %x  Mwsub: %x  Mwdiv: %x  Mwhole: %x  Mbound: %x\n",
					Mwork, Mwsub, Mwdiv, Mwhole, Mbound);
				pr_diag("Editor",
					"[edit_modify_area]   MareaA: %x  MareaB: %x  Makeep: %x  MareaS: %x\n",
					MareaA, MareaB, Makeep, MareaS);
				pr_diag("Editor",
					"[edit_modify_area]   Mbdiv: %x\n",
					Mbdiv);
				pr_diag("Editor",
					"[edit_modify_area]   MholeA: %x-NoDestroy  MholeB: %x-NoDestroy  Mhkeep: %x  MholeS: %x-NoDestroy\n",
					MholeA, MholeB, Mhkeep, MholeS);
#				endif /* DEBUG_STRUCTURES */

				/* Clean up temporary buffers */
				circle_echo(FALSE);
				ignore_partial();
				edit_select(NullCal, FALSE);
				Mfresh = FALSE;
				Mdrawn = FALSE;
				Mpick  = NullArea;
				Mpsub  = NullSubArea;
				Mptype = AreaNone;
				Mpmem  = -1;
				Mpset  = FALSE;
				Mwork  = NullArea;
				Mwsub  = NullSubArea;
				Mwdiv  = NullCurve;
				Mwhole = NullCurve;
				Mbound = NullLine;
				Mtrunc = FALSE;
				Makeep = NullArea;
				MareaA = NullArea;
				MareaB = NullArea;
				MareaS = NullArea;
				Mbdiv  = NullCurve;
				Mhkeep = NullCurve;
				MholeA = NullCurve;
				MholeB = NullCurve;
				MholeS = NullCurve;
				Lmarks = NullAreaList;

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_modify_area] ModRePick ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();
				drawing_control(FALSE);
				modifying_control(FALSE);

				/* Move on to next stage */
				State = ModPick;
				show_mod_state("->", State);
				continue;

			/* Delete the picked area */
			case ModDelete:

				/* Freeze all pending edits so only this one can be undone */
				if (EditUndoable)
					{
					ignore_partial();
					put_message("edit-freeze");
					accept_mod();	/* Leaves EditAreas alone */
					}
				post_partial(TRUE);

#				ifdef DEBUG
				pr_diag("Editor", "[edit_modify_area] Deleting\n");
#				endif /* DEBUG */

				/* Remove the area */
				busy_cursor(TRUE);
				put_message("area-deleting");
				ifrom = which_set_item(EditAreas, Mpick);
				remove_item_from_set(EditAreas, Mpick);
				(void) adjust_area_link_nodes(ActiveDfld, EditTime, ifrom, -1);

				/* Show the results */
				present_all();

				/* Move on to next stage */
				State = ModConfirm;
				show_mod_state("->", State);
				continue;

			/* Delete the picked hole */
			case ModDeleteHole:

				/* Freeze all pending edits so only this one can be undone */
				if (EditUndoable)
					{
					ignore_partial();
					put_message("edit-freeze");
					accept_mod();	/* Leaves EditAreas alone */
					}
				post_partial(TRUE);

#				ifdef DEBUG
				pr_diag("Editor", "[edit_modify_area] Deleting hole\n");
#				endif /* DEBUG */

				/* Remove the hole */
				(void) remove_area_hole(Mpick, Mpick->bound->holes[Mpmem]);
				put_message("area-hole-warn1");
				(void) sleep(1);

				/* Move on to next stage */
				State = ModConfirm;
				show_mod_state("->", State);
				continue;

			/* Set the value of the picked area */
			case ModSet:

				/* Freeze all pending edits so only this one can be undone */
				/* (first time after picking) */
				if (Mfresh)
					{
					if (EditUndoable && edit_posted())
						{
						ignore_partial();
						put_message("edit-freeze");
						accept_mod();	/* Leaves EditAreas alone */
						drawn = FALSE;
						}
					}
				post_partial(TRUE);

#				ifdef DEBUG
				pr_diag("Editor", "[edit_modify_area] Setting value (%s)\n",
						CAL_get_attribute(cal, CALuserlabel));
				if (NotNull(Mpsub))
					{
					pr_diag("Editor",
						"[edit_modify_area] Reset subarea attribs\n");
					if (Mpmem == 0 && Mpsub == Mpick->subareas[Mpmem])
						{
						pr_diag("Editor",
							"[edit_modify_area] Reset default area attribs\n");
						}
					}
				else
					{
					pr_diag("Editor",
						"[edit_modify_area] Reset area attribs\n");
					}
#				endif /* DEBUG */

				/* Set a new value */
				busy_cursor(TRUE);
				put_message("area-value");
				if (NotNull(Mpsub))
					{
					define_item_attribs("subarea", (ITEM) Mpsub, cal);

					/* Reset default area attributes if setting first subarea */
					if (Mpmem == 0 && Mpsub == Mpick->subareas[Mpmem])
						{
						define_item_attribs("area", (ITEM) Mpick, cal);
						}
					}
				else
					{
					define_item_attribs("area", (ITEM) Mpick, cal);
					}

				/* Register the edit */
				ignore_partial();
				drawn  = TRUE;
				Mdrawn = TRUE;
				Mfresh = FALSE;
				invoke_set_catspecs(EditAreas);
				if (EditUndoable) highlight_set(EditAreas, 1);
				if (EditUndoable) post_mod("areas");

				/* Modify labels if necessary */
				if (recompute_areaset_labs(EditAreas, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				if (EditUndoable) (void) extract_area_order_tags(FALSE);

				/* Reset working area/subarea/boundary */
				modify_area_select(TRUE, Mpos);
				modify_area_enhance();

				/* Show the results */
				busy_cursor(FALSE);
				drawing_control(FALSE);
				present_all();

				/* Move on to next stage */
				State = (ModifyMode==MODIFY_PUCK)? ModSculpt: ModDraw;
				show_mod_state("->", State);
				continue;

			/* Re-arrange the stacking order */
			case ModStack:

				/* Freeze all pending edits so only this one can be undone */
				/* (first time after picking) */
				if (Mfresh)
					{
					if (EditUndoable && edit_posted())
						{
						ignore_partial();
						put_message("edit-freeze");
						accept_mod();	/* Leaves EditAreas alone */
						drawn = FALSE;
						}
					}
				post_partial(TRUE);

				/* Interpret the new order */
				ifrom = which_set_item(EditAreas, (ITEM) Mpick);
				if (same_ic(order, "TOP"))         idest = 0;
				else if (same_ic(order, "UP"))     idest = ifrom - 1;
				else if (same_ic(order, "DOWN"))   idest = ifrom + 1;
				else if (same_ic(order, "BOTTOM")) idest = EditAreas->num - 1;
				else
					{
					State = (ModifyMode==MODIFY_PUCK)? ModSculpt: ModDraw;
					show_mod_state("->", State);
					continue;
					}

#				ifdef DEBUG
				pr_diag("Editor",
						"[edit_modify_area] Reordering area (%s) from %d to %d\n",
						CAL_get_attribute(cal, CALuserlabel), ifrom, idest);
#				endif /* DEBUG */

				/* Move the picked area */
				move_set_item(EditAreas, ifrom, idest);
				(void) adjust_area_link_nodes(ActiveDfld,
												EditTime, ifrom, idest);

				/* Register the edit */
				ignore_partial();
				drawn  = TRUE;
				Mdrawn = TRUE;
				Mfresh = FALSE;
				invoke_set_catspecs(EditAreas);
				if (EditUndoable) highlight_set(EditAreas, 1);
				if (EditUndoable) post_mod("areas");

				/* Modify labels if necessary */
				if (recompute_areaset_labs(EditAreas, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				if (EditUndoable) (void) extract_area_order_tags(FALSE);

				/* Reset working area/subarea/boundary */
				modify_area_select(TRUE, Mpos);
				modify_area_enhance();

				/* Show the results */
				busy_cursor(FALSE);
				drawing_control(FALSE);
				present_all();

				/* Move on to next stage */
				State = (ModifyMode==MODIFY_PUCK)? ModSculpt: ModDraw;
				show_mod_state("->", State);
				continue;

			/* Draw a new segment on the picked area */
			case ModDraw:

				/* Check if drawing mode has changed */
				if (ModifyMode == MODIFY_PUCK)
					{

					/* Clean up temporary buffers */
					Mwork  = NullArea;
					Mwsub  = NullSubArea;
					Mwdiv  = NullCurve;
					Mwhole = NullCurve;
					Mbound = NullLine;
					Makeep = NullArea;
					MareaA = NullArea;
					MareaB = NullArea;
					Mbdiv  = NullCurve;
					Mhkeep = NullCurve;
					MholeA = NullCurve;
					MholeB = NullCurve;
					Lmarks = NullAreaList;
					empty_line(Mnew);
					empty_line(MsegA);
					empty_line(MsegB);

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_modify_area] ModDraw ... empty TempMeta %d fields\n",
						TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */

					/* Empty display structure */
					empty_temp();

					/* Reset working area/subarea/boundary/hole */
					modify_area_select(TRUE, Mpos);
					modify_area_enhance();

					/* Show the results */
					present_all();

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
						accept_mod();	/* Leaves EditAreas alone */
						drawn = FALSE;
						}
					}
				post_partial(TRUE);

				/* Draw a new segment */
				switch (Mptype)
					{
					case AreaNone:
					case AreaBound:	next = modify_area_draw();
									break;
					case AreaDiv:	next = modify_area_draw_div();
									break;
					case AreaHole:	next = modify_area_draw_hole();
									break;
					default:		put_message("area-mod-warn");
									(void) sleep(2);
									State = ModRePick;
									continue;
					}
				if (next == ModDraw) return drawn;

				/* Move on to next stage */
				State = next;
				show_mod_state("->", State);
				continue;

			/*  Redraw the new segment */
			case ModReDraw:

				put_message("edit-cancel");
				if (Mdrawn) ignore_partial();

				/* Re-display the picked feature */
				switch (Mptype)
					{
					case AreaNone:
					case AreaBound:

							/* Un-highlight the chosen area */
							/*  ... which may be previously picked area! */
							highlight_area(Makeep, 2, -1);
							widen_area(Makeep, -2.0);

							/* Re-highlight the previously picked area */
							if (NotNull(Mwsub) && Mwork->numdiv > 0)
								{
								highlight_area(Mwork, 2, -1);
								highlight_subarea(Mwsub, 2, 50);
								}
							else
								{
								highlight_area(Mwork, 2, 50);
								}

							/* Save only original area in TempMeta */
							Mtemp = find_mf_set(TempMeta, "area", "c", "", "");
							Mtemp->num = 0;
							add_item_to_metafile(TempMeta, "area", "c", "", "",
												(ITEM) Mwork);

#							ifdef DEBUG_STRUCTURES
							pr_diag("Editor",
								"[edit_modify_area] ModReDraw Mtemp->num: %d\n",
								Mtemp->num);
							pr_diag("Editor",
								"[edit_modify_area] Adding Mwork to TempMeta (%d)\n",
								TempMeta->numfld);
#							endif /* DEBUG_STRUCTURES */

							break;

					case AreaDiv:

							/* Save only original dividing line in TempMeta */
							Mtemp = find_mf_set(TempMeta, "curve", "c", "", "");
							Mtemp->num = 0;
							add_item_to_metafile(TempMeta, "curve", "c", "", "",
												(ITEM) Mwdiv);

#							ifdef DEBUG_STRUCTURES
							pr_diag("Editor",
								"[edit_modify_area] ModReDraw Mtemp->num: %d\n",
								Mtemp->num);
							pr_diag("Editor",
								"[edit_modify_area] Adding Mwdiv to TempMeta (%d)\n",
								TempMeta->numfld);
#							endif /* DEBUG_STRUCTURES */

							break;

					case AreaHole:

							/* Un-highlight the chosen hole */
							/*  ... which may be previously picked hole! */
							highlight_curve(Mhkeep, 2);
							widen_curve(Mhkeep, -2.0);

							/* Re-highlight the previously picked hole */
							highlight_curve(Mwhole, 2);

							/* Save only original hole in TempMeta */
							Mtemp = find_mf_set(TempMeta, "curve", "c", "", "");
							Mtemp->num = 0;
							add_item_to_metafile(TempMeta, "curve", "c", "", "",
												(ITEM) Mwhole);

#							ifdef DEBUG_STRUCTURES
							pr_diag("Editor",
								"[edit_modify_area] ModReDraw Mtemp->num: %d\n",
								Mtemp->num);
							pr_diag("Editor",
								"[edit_modify_area] Adding Mwhole to TempMeta (%d)\n",
								TempMeta->numfld);
#							endif /* DEBUG_STRUCTURES */

							break;

					default:
							break;
					}

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor", "[edit_modify_area] ModReDraw ...\n");
				pr_diag("Editor",
					"[edit_modify_area]   Mwork: %x  Mwsub: %x  Mwdiv: %x  Mwhole: %x  Mbound: %x\n",
					Mwork, Mwsub, Mwdiv, Mwhole, Mbound);
				pr_diag("Editor",
					"[edit_modify_area]   MareaA: %x  MareaB: %x  Makeep: %x  MareaS: %x\n",
					MareaA, MareaB, Makeep, MareaS);
				pr_diag("Editor",
					"[edit_modify_area]   Mbdiv: %x\n",
					Mbdiv);
				pr_diag("Editor",
					"[edit_modify_area]   MholeA: %x-Destroy  MholeB: %x-Destroy  Mhkeep: %x  MholeS: %x-NoDestroy\n",
					MholeA, MholeB, Mhkeep, MholeS);
#				endif /* DEBUG_STRUCTURES */

				/* Clean up temporary buffers */
				Makeep = NullArea;
				MareaA = destroy_area(MareaA);
				MareaB = destroy_area(MareaB);
				Mbdiv  = destroy_curve(Mbdiv);
				Mhkeep = NullCurve;
				MholeA = destroy_curve(MholeA);
				MholeB = destroy_curve(MholeB);
				empty_line(Mnew);
				empty_line(MsegA);
				empty_line(MsegB);

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

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor", "[edit_modify_area] ModDiscard ...\n");
					pr_diag("Editor",
						"[edit_modify_area]   Mwork: %x  Mwsub: %x  Mwdiv: %x  Mwhole: %x  Mbound: %x\n",
						Mwork, Mwsub, Mwdiv, Mwhole, Mbound);
					pr_diag("Editor",
						"[edit_modify_area]   MareaA: %x  MareaB: %x  Makeep: %x  MareaS: %x\n",
						MareaA, MareaB, Makeep, MareaS);
					pr_diag("Editor",
						"[edit_modify_area]   Mbdiv: %x\n",
						Mbdiv);
					pr_diag("Editor",
						"[edit_modify_area]   MholeA: %x-NoDestroy  MholeB: %x-NoDestroy  Mhkeep: %x  MholeS: %x-NoDestroy\n",
						MholeA, MholeB, Mhkeep, MholeS);
#					endif /* DEBUG_STRUCTURES */

					/* Clean up temporary buffers */
					Mwork  = NullArea;
					Mwsub  = NullSubArea;
					Mwdiv  = NullCurve;
					Mwhole = NullCurve;
					Mbound = NullLine;
					Makeep = NullArea;
					MareaA = NullArea;
					MareaB = NullArea;
					Mbdiv  = NullCurve;
					Mhkeep = NullCurve;
					MholeA = NullCurve;
					MholeB = NullCurve;
					Lmarks = NullAreaList;
					empty_line(Mnew);
					empty_line(MsegA);
					empty_line(MsegB);

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_modify_area] ModDiscard ... empty TempMeta %d fields\n",
						TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */

					/* Empty display structure */
					empty_temp();

					/* Reset working area/subarea/boundary/hole */
					modify_area_select(TRUE, Mpos);
					modify_area_enhance();

					/* Show the results */
					present_all();

					State = ModSculpt;
					show_mod_state("->", State);
					continue;
					}

				/* Discard a segment */
				post_partial(TRUE);
				switch (Mptype)
					{
					case AreaNone:
					case AreaBound:	next = modify_area_discard();
									break;
					case AreaDiv:	next = modify_area_discard_div();
									break;
					case AreaHole:	next = modify_area_discard_hole();
									break;
					default:		put_message("area-mod-warn");
									(void) sleep(2);
									State = ModRePick;
									continue;
					}
				if (next == ModDiscard) return drawn;

				/* Move on to next stage */
				State = next;
				show_mod_state("->", State);
				continue;

			/* Modify the area boundary or dividing line or hole */
			case ModDrawDone:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_modify_area] Modifying\n");
#				endif /* DEBUG */

				/* Modify the area */
				/* Keep dividing lines if possible */
				busy_cursor(TRUE);
				switch (Mptype)
					{
					case AreaNone:
					case AreaBound:

							/* Replace the boundary */
							put_message("area-mod-boundary");
							Mbound = copy_line(Makeep->bound->boundary);
							if (!replace_area_boundary(Mpick, Mbound, &dstat))
								{
								State = ModDivideWarn;
								show_mod_state("->", State);
								continue;
								}
							break;

					case AreaDiv:

							/* Replace the dividing line */
							put_message("area-mod-divide");
							if (!replace_area_divide(Mpick, Mpmem,
													 Mbdiv->line, &dstat))
								{
								State = ModDivideWarn;
								show_mod_state("->", State);
								continue;
								}
							break;

					case AreaHole:

							/* Replace the hole */
							put_message("area-mod-hole");
							if( !replace_area_hole(Mpick, Mpmem,
													 Mhkeep->line, &dstat))
								{
								State = ModConfirm;
								show_mod_state("->", State);
								continue;
								}
							break;

					default:
							break;
					}

				/* Register the edit */
				ignore_partial();
				drawn  = TRUE;
				Mdrawn = TRUE;
				Mfresh = FALSE;
				invoke_set_catspecs(EditAreas);
				if (EditUndoable) highlight_set(EditAreas, 1);
				if (EditUndoable) post_mod("areas");

				/* Modify labels if necessary */
				if (recompute_areaset_labs(EditAreas, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				if (EditUndoable) (void) extract_area_order_tags(FALSE);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor", "[edit_modify_area] ModDrawDone ...\n");
				pr_diag("Editor",
					"[edit_modify_area]   Mwork: %x  Mwsub: %x  Mwdiv: %x  Mwhole: %x  Mbound: %x\n",
					Mwork, Mwsub, Mwdiv, Mwhole, Mbound);
				pr_diag("Editor",
					"[edit_modify_area]   MareaA: %x  MareaB: %x  Makeep: %x  MareaS: %x\n",
					MareaA, MareaB, Makeep, MareaS);
				pr_diag("Editor",
					"[edit_modify_area]   Mbdiv: %x\n",
					Mbdiv);
				pr_diag("Editor",
					"[edit_modify_area]   MholeA: %x-NoDestroy  MholeB: %x-NoDestroy  Mhkeep: %x  MholeS: %x-NoDestroy\n",
					MholeA, MholeB, Mhkeep, MholeS);
#				endif /* DEBUG_STRUCTURES */

				/* Clean up temporary buffers */
				Mwork  = NullArea;
				Mwsub  = NullSubArea;
				Mwdiv  = NullCurve;
				Mwhole = NullCurve;
				Mbound = NullLine;
				Makeep = NullArea;
				MareaA = NullArea;
				MareaB = NullArea;
				Mbdiv  = NullCurve;
				Mhkeep = NullCurve;
				MholeA = NullCurve;
				MholeB = NullCurve;
				Lmarks = NullAreaList;
				empty_line(Mnew);
				empty_line(MsegA);
				empty_line(MsegB);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_modify_area] ModDrawDone ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();

				/* Reset working area/subarea/boundary/divide/hole */
				modify_area_select(TRUE, Mpos);
				modify_area_enhance();

				/* Show the results */
				busy_cursor(FALSE);
				present_all();
				modifying_control(FALSE);

				/* Move on to next stage - do more drawing */
				State = ModDraw;
				show_mod_state("->", State);
				continue;

			/* Sculpt the picked area */
			case ModSculpt:

				/* Check if drawing mode has changed */
				if (ModifyMode != MODIFY_PUCK)
					{

					/* Clean up temporary buffers */
					Mwork  = NullArea;
					Mwsub  = NullSubArea;
					Mwdiv  = NullCurve;
					Mwhole = NullCurve;
					Mbound = NullLine;
					Makeep = NullArea;
					MareaA = NullArea;
					MareaB = NullArea;
					Mbdiv  = NullCurve;
					Mhkeep = NullCurve;
					MholeA = NullCurve;
					MholeB = NullCurve;
					Lmarks = NullAreaList;
					empty_line(Mnew);
					empty_line(MsegA);
					empty_line(MsegB);

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_modify_area] ModSculpt ... empty TempMeta %d fields\n",
						TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */

					/* Empty display structure */
					empty_temp();

					/* Reset working area/subarea/boundary/hole */
					modify_area_select(TRUE, Mpos);
					modify_area_enhance();

					/* Show the results */
					present_all();

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
						accept_mod();	/* Leaves EditAreas alone */
						drawn = FALSE;
						}
					}
				post_partial(TRUE);

				/* Draw a new segment */
				switch (Mptype)
					{
					case AreaNone:
					case AreaBound:	next = modify_area_sculpt();
									break;
					case AreaDiv:	next = modify_area_sculpt_div();
									break;
					case AreaHole:	next = modify_area_sculpt_hole();
									break;
					default:		put_message("area-mod-warn");
									(void) sleep(2);
									State = ModRePick;
									continue;
					}
				if (next == ModSculpt) return drawn;

				/* Move on to next stage */
				State = next;
				show_mod_state("->", State);
				continue;

			/* Modify the area after sculpting */
			case ModSculptDone:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_modify_area] Modifying\n");
#				endif /* DEBUG */

				/* Modify the area */
				/* Keep dividing lines if possible */
				busy_cursor(TRUE);
				switch (Mptype)
					{
					case AreaNone:
					case AreaBound:

							/* Replace the boundary */
							put_message("area-mod-boundary");
							Mbound = copy_line(MareaS->bound->boundary);
							if (!replace_area_boundary(Mpick, Mbound, &dstat))
								{
								State = ModDivideWarn;
								show_mod_state("->", State);
								continue;
								}
							break;

					case AreaDiv:

							/* Replace the dividing line */
							put_message("area-mod-divide");
							if( !replace_area_divide(Mpick, Mpmem,
													 Mbdiv->line, &dstat))
								{
								State = ModDivideWarn;
								show_mod_state("->", State);
								continue;
								}
							break;

					case AreaHole:

							/* Replace the hole */
							put_message("area-mod-hole");
							if( !replace_area_hole(Mpick, Mpmem,
													 MholeS->line, &dstat))
								{
								State = ModConfirm;
								show_mod_state("->", State);
								continue;
								}
							break;

					default:
							break;
					}

				/* Register the edit */
				ignore_partial();
				drawn  = TRUE;
				Mdrawn = TRUE;
				Mfresh = FALSE;
				invoke_set_catspecs(EditAreas);
				if (EditUndoable) highlight_set(EditAreas, 1);
				if (EditUndoable) post_mod("areas");

				/* Modify labels if necessary */
				if (recompute_areaset_labs(EditAreas, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				if (EditUndoable) (void) extract_area_order_tags(FALSE);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor", "[edit_modify_area] ModSculptDone ...\n");
				pr_diag("Editor",
					"[edit_modify_area]   Mwork: %x  Mwsub: %x  Mwdiv: %x  Mwhole: %x  Mbound: %x\n",
					Mwork, Mwsub, Mwdiv, Mwhole, Mbound);
				pr_diag("Editor",
					"[edit_modify_area]   MareaA: %x  MareaB: %x  Makeep: %x  MareaS: %x\n",
					MareaA, MareaB, Makeep, MareaS);
				pr_diag("Editor",
					"[edit_modify_area]   Mbdiv: %x\n",
					Mbdiv);
				pr_diag("Editor",
					"[edit_modify_area]   MholeA: %x-NoDestroy  MholeB: %x-NoDestroy  Mhkeep: %x  MholeS: %x-Destroy\n",
					MholeA, MholeB, Mhkeep, MholeS);
#				endif /* DEBUG_STRUCTURES */

				/* Clean up temporary buffers */
				Mwork  = NullArea;
				Mwsub  = NullSubArea;
				Mwdiv  = NullCurve;
				Mwhole = NullCurve;
				Mbound = NullLine;
				Makeep = NullArea;
				MareaA = NullArea;
				MareaB = NullArea;
				MareaS = destroy_area(MareaS);
				Mbdiv  = NullCurve;
				Mhkeep = NullCurve;
				MholeA = NullCurve;
				MholeB = NullCurve;
				MholeS = destroy_curve(MholeS);
				Lmarks = NullAreaList;
				empty_line(Mnew);
				empty_line(MsegA);
				empty_line(MsegB);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_modify_area] ModSculptDone ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();

				/* Reset working area/subarea/boundary/divide/hole */
				modify_area_select(TRUE, Mpos);
				modify_area_enhance();

				/* Show the results */
				busy_cursor(FALSE);
				present_all();
				modifying_control(FALSE);

				/* Save truncated dividing lines */
				if (Mtrunc)
					State = ModConfirm;

				/* Move on to next stage - do more sculpting */
				else
					State = ModSculpt;
				show_mod_state("->", State);
				continue;

			/* Warning messages if dividing lines have been lost */
			case ModDivideWarn:
				pr_warning("Editor",
					"[edit_modify_area] Some dividing lines will be deleted!\n");
				switch (Mptype)
					{
					case AreaNone:
					case AreaBound:
							switch(dstat)
								{
								default:
									put_message("area-div-warn2");
									(void) sleep(2);
									break;
								}
							break;

					case AreaDiv:
							switch(dstat)
								{
								case DivAreaLeft:
								case DivAreaRight:
									pr_warning("Editor",
										"[edit_modify_area] Dividing line has moved outside subarea!\n");
									put_message("area-div-warn1");
									(void) sleep(2);
									break;

								default:
									put_message("area-div-warn2");
									(void) sleep(2);
									break;
								}
							break;

					case AreaHole:
					default:
							break;
					}

				/* Clean up temporary buffer for sculpting */
				MareaS = destroy_area(MareaS);

				/* Move on to next stage */
				State = ModConfirm;
				show_mod_state("->", State);
				continue;

			/* Warning message if dividing line has been sculpted away */
			case ModDivideGone:

				/* Remove the dividing line */
				(void) remove_area_divide(Mpick, Mpmem, Mright, &dstat);
				pr_warning("Editor",
					"[edit_modify_area] Dividing line has moved outside subarea!\n");
				put_message("area-div-warn1");
				(void) sleep(2);

				/* Move on to next stage */
				State = ModConfirm;
				show_mod_state("->", State);
				continue;

			/* Warning message if hole has been sculpted away */
			case ModHoleGone:

				/* Remove the hole */
				(void) remove_area_hole(Mpick, Mpick->bound->holes[Mpmem]);
				put_message("area-hole-warn1");
				(void) sleep(2);

				/* Move on to next stage */
				State = ModConfirm;
				show_mod_state("->", State);
				continue;

			/* Confirm current edit */
			case ModConfirm:

				/* Register the edit */
				ignore_partial();
				drawn  = TRUE;
				Mdrawn = TRUE;
				Mfresh = FALSE;
				invoke_set_catspecs(EditAreas);
				if (EditUndoable) highlight_set(EditAreas, 1);
				if (EditUndoable) post_mod("areas");

				/* Modify labels if necessary */
				if (recompute_areaset_labs(EditAreas, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				if (EditUndoable) (void) extract_area_order_tags(FALSE);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor", "[edit_modify_area] ModConfirm ...\n");
				pr_diag("Editor",
					"[edit_modify_area]   Mwork: %x  Mwsub: %x  Mwdiv: %x  Mwhole: %x  Mbound: %x\n",
					Mwork, Mwsub, Mwdiv, Mwhole, Mbound);
				pr_diag("Editor",
					"[edit_modify_area]   MareaA: %x  MareaB: %x  Makeep: %x  MareaS: %x\n",
					MareaA, MareaB, Makeep, MareaS);
				pr_diag("Editor",
					"[edit_modify_area]   Mbdiv: %x\n",
					Mbdiv);
				pr_diag("Editor",
					"[edit_modify_area]   MholeA: %x-NoDestroy  MholeB: %x-NoDestroy  Mhkeep: %x  MholeS: %x-Destroy\n",
					MholeA, MholeB, Mhkeep, MholeS);
#				endif /* DEBUG_STRUCTURES */

				/* Clean up temporary buffers */
				circle_echo(FALSE);
				edit_select(NullCal, FALSE);
				Mpick  = NullArea;
				Mpsub  = NullSubArea;
				Mptype = AreaNone;
				Mpmem  = -1;
				Mpset  = FALSE;
				Mwork  = NullArea;
				Mwsub  = NullSubArea;
				Mwdiv  = NullCurve;
				Mwhole = NullCurve;
				Mbound = NullLine;
				Makeep = NullArea;
				MareaA = NullArea;
				MareaB = NullArea;
				MareaS = destroy_area(MareaS);
				Mbdiv  = NullCurve;
				Mhkeep = NullCurve;
				MholeA = NullCurve;
				MholeB = NullCurve;
				MholeS = destroy_curve(MholeS);
				Lmarks = NullAreaList;
				empty_line(Mnew);
				empty_line(MsegA);
				empty_line(MsegB);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_modify_area] ModConfirm ... empty TempMeta %d fields\n",
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
					"[edit_modify_area] Unsuccessful - Bailing out\n");

				/* Register the edit */
				ignore_partial();
				drawn  = FALSE;
				Mdrawn = FALSE;
				Mfresh = FALSE;
				if (EditUndoable)
					{
					post_mod("areas");
					put_message("edit-bail-out");
					(void) sleep(2);
					reject_mod();	/* Must reset EditAreas */
					active_area_fields(TRUE, NewAreas, NewLabs);
					}

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor", "[edit_modify_area] ModBailout ...\n");
				pr_diag("Editor",
					"[edit_modify_area]   Mwork: %x  Mwsub: %x  Mwdiv: %x  Mwhole: %x  Mbound: %x\n",
					Mwork, Mwsub, Mwdiv, Mwhole, Mbound);
				pr_diag("Editor",
					"[edit_modify_area]   MareaA: %x  MareaB: %x  Makeep: %x  MareaS: %x\n",
					MareaA, MareaB, Makeep, MareaS);
				pr_diag("Editor",
					"[edit_modify_area]   Mbdiv: %x\n",
					Mbdiv);
				pr_diag("Editor",
					"[edit_modify_area]   MholeA: %x-NoDestroy  MholeB: %x-NoDestroy  Mhkeep: %x  MholeS: %x-Destroy\n",
					MholeA, MholeB, Mhkeep, MholeS);
#				endif /* DEBUG_STRUCTURES */

				/* Clean up temporary buffers */
				circle_echo(FALSE);
				edit_select(NullCal, FALSE);
				Mpick  = NullArea;
				Mpsub  = NullSubArea;
				Mptype = AreaNone;
				Mpmem  = -1;
				Mpset  = FALSE;
				Mwork  = NullArea;
				Mwsub  = NullSubArea;
				Mwdiv  = NullCurve;
				Mwhole = NullCurve;
				Mbound = NullLine;
				Makeep = NullArea;
				MareaA = NullArea;
				MareaB = NullArea;
				MareaS = destroy_area(MareaS);
				Mbdiv  = NullCurve;
				Mhkeep = NullCurve;
				MholeA = NullCurve;
				MholeB = NullCurve;
				MholeS = destroy_curve(MholeS);
				Lmarks = NullAreaList;
				empty_line(Mnew);
				empty_line(MsegA);
				empty_line(MsegB);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_modify_area] ModBailout ... empty TempMeta %d fields\n",
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

static	MODSTATE	modify_area_pick(void)

	{
	POINT	pos;
	int		butt;

	/* See if there are any areas to modify */
	if (EditAreas->num <= 0)
		{
		put_message("area-no-mod");
		return ModPick;
		}

	/* Keep trying until something gets picked */
	while (TRUE)
		{
		/* Get a point */
		put_message("area-mod-pick");
		if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return ModPick;
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}
		put_message("area-pick");
		if (!pick_Xpoint(DnEdit, 0, pos, &butt)) continue;
		if (!inside_dn_window(DnEdit, pos))
			{
			put_message("edit-outside-map");
			(void) sleep(1);
			continue;
			}

		/* See if we picked a dividing line, boundary, or hole */
		modify_area_select(FALSE, pos);
		Mfresh = TRUE;

		/* Produce a copy of the picked area to work on */
		modify_area_enhance();
		present_temp(TRUE);

		return (ModifyMode==MODIFY_PUCK)? ModSculpt: ModDraw;
		}
	}

/**********************************************************************/

static	void	modify_area_select
	(
	LOGICAL	usepick,
	POINT	pos
	)

	{
	int		imem, isub;
	AMEMBER	mtype;
	float	dist;
	LOGICAL	in;
	DIVSTAT	dstat;

	if (usepick && NotNull(Mpick))
		{
		LOGICAL	good = FALSE;

		switch (Mptype)
			{
			case AreaDiv:
					if (Mpmem < 0) break;
					if (Mpmem >= Mpick->numdiv) break;

					/* Set the subarea to clip to */
					/* Note that we must redivide the area to just before */
					/*  the dividing line identified by Mpmem is applied! */
					if (NotNull(MareaX))
						{
#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[modify_area_select] Removing MareaX/Mxsub!\n");
#						endif /* DEBUG_STRUCTURES */
						MareaX = destroy_area(MareaX);
						}
					MareaX = copy_area(Mpick, TRUE);
					(void) partial_area_redivide(MareaX, Mpmem, &dstat);
					isub   = Mpick->subids[Mpmem];
					Mxsub  = MareaX->subareas[isub];

					good = TRUE;
					break;

			case AreaHole:
					if (Mpmem < 0) break;
					if (Mpmem >= Mpick->bound->numhole) break;

					good = TRUE;
					break;

			case AreaBound:
			case AreaNone:
					if (NotNull(Mpsub))
						{
						if (Mpmem < 0) break;
						if (Mpmem > Mpick->numdiv) break;

						/* Reset the subarea */
						Mpsub = Mpick->subareas[Mpmem];
						}

					good = TRUE;
					break;
			}

		if (good)
			{
			if (Mptype==AreaHole)
				edit_select_hole();
			else if (NotNull(Mpsub))
				edit_select((CAL) Mpsub->attrib, Mpset);
			else
				edit_select((CAL) Mpick->attrib, Mpset);

			return;
			}
		}

	/* See if we picked a dividing line, boundary, or hole */
	Mpick  = NullArea;
	Mpsub  = NullSubArea;
	Mptype = AreaNone;
	Mpmem  = -1;
	Mpset  = FALSE;
	Mpick  = closest_area(EditAreas, pos, &dist, NullPoint, &mtype,
				&imem, NullInt);
	if (IsNull(Mpick))
		{
		put_message("area-no-pick");
		(void) sleep(1);
		return;
		}

	/* Pick specific features if close enough */
	if (dist < SplineRes)
		{
		Mptype = mtype;
		Mpmem  = imem;
		}

	/* Otherwise, pick the boundary */
	else
		{
		Mptype = AreaBound;
		}

	/* Set the subarea to clip to if we picked a dividing line */
	/* Note that we must redivide the area to just before */
	/*  the dividing line identified by Mpmem is applied! */
	if (Mptype==AreaDiv)
		{
		if (NotNull(MareaX))
			{
#			ifdef DEBUG_STRUCTURES
			pr_diag("Editor",
				"[modify_area_select] Removing MareaX/Mxsub!\n");
#			endif /* DEBUG_STRUCTURES */
			MareaX = destroy_area(MareaX);
			}
		MareaX = copy_area(Mpick, TRUE);
		(void) partial_area_redivide(MareaX, Mpmem, &dstat);
		isub   = Mpick->subids[Mpmem];
		Mxsub  = MareaX->subareas[isub];
		}

	/* Also see if we picked a subarea */
	area_test_point(Mpick, pos, NULL, NULL, NULL, NULL, NULL, &in);
	if (in && Mptype==AreaBound)
		{

		/* Pick whole area if no dividing lines */
		if (Mpick->numdiv <= 0)
			{
			Mpmem = 0;
			Mpset = TRUE;
			}

		/* Otherwise determine closest subarea */
		else
			{
			Mpsub = closest_subarea(EditAreas, pos, NullFloat, NullPoint,
									NullInt, NullInt, &Mpick);
			Mpmem = which_area_subarea(Mpick, Mpsub);
#			ifdef DEBUG_STRUCTURES
			pr_diag("Editor",
				"[modify_area_select] Picking subarea: %d\n", Mpmem);
#			endif /* DEBUG_STRUCTURES */
			Mpset = TRUE;
			}
		}

	/* Make sure we picked something */
	if (IsNull(Mpick) || (IsNull(Mpsub) && Mptype==AreaNone))
		{
		put_message("area-no-pick");
		(void) sleep(1);
		return;
		}

	if (Mptype==AreaHole)
		edit_select_hole();
	else if (NotNull(Mpsub))
		edit_select((CAL) Mpsub->attrib, Mpset);
	else
		edit_select((CAL) Mpick->attrib, Mpset);

	copy_point(Mpos, pos);
	}

/**********************************************************************/

static	void	modify_area_enhance(void)

	{
	int		isub, ips, ipe, ipx, nn;
	float	spfact;
	POINT	spos, epos;
	LINE	cdiv;
	SUBAREA	lsub, rsub;
	AREA	lmark;

#	ifdef DEBUG_STRUCTURES
	pr_diag("Editor", "[modify_area_enhance] ...\n");
	pr_diag("Editor",
		"[modify_area_enhance]   Mwork: %x  Mwsub: %x  Mwdiv: %x  Mwhole: %x  Mbound: %x\n",
		Mwork, Mwsub, Mwdiv, Mwhole, Mbound);
	pr_diag("Editor",
		"[modify_area_enhance]   MareaA: %x  MareaB: %x  Makeep: %x  MareaS: %x\n",
		MareaA, MareaB, Makeep, MareaS);
	pr_diag("Editor",
		"[modify_area_enhance]   Mbdiv: %x\n",
		Mbdiv);
	pr_diag("Editor",
		"[modify_area_enhance]   MholeA: %x  MholeB: %x  Mhkeep: %x  MholeS: %x\n",
		MholeA, MholeB, Mhkeep, MholeS);
#	endif /* DEBUG_STRUCTURES */

	/* Produce a copy of the picked area/subarea/divide/hole to work on */
	switch (Mptype)
		{
		case AreaNone:
		case AreaBound: put_message("area-picked");
						break;
		case AreaDiv:   put_message("area-picked-div");
						break;
		case AreaHole:  put_message("area-picked-hole");
						break;
		}
	Mwork  = copy_area(Mpick, TRUE);
	Mwsub  = NullSubArea;
	Mwdiv  = NullCurve;
	Mwhole = NullCurve;
	Mbound = NullLine;

	add_item_to_metafile(TempMeta, "area", "c", "", "", (ITEM) Mwork);

#	ifdef DEBUG_STRUCTURES
	pr_diag("Editor",
		"[modify_area_enhance] Adding Mwork to TempMeta (%d)\n",
		TempMeta->numfld);
#	endif /* DEBUG_STRUCTURES */

	switch (Mptype)
		{
		case AreaDiv:
				Mwdiv = create_curve("", "", "");
				Mwdiv->line = copy_line(Mwork->divlines[Mpmem]);

				/* Truncate the dividing line if it crosses itself */
				if ( looped_line_crossing(Mwdiv->line, spos, &ips, &ipx) )
					{

					/* A dividing line that crosses itself has been found */
					pr_warning("Editor",
						"[modify_area_enhance] Truncating dividing line cross over!\n");
					put_message("area-div-cross");
					(void) sleep(1);

					/* Find first cross over from the opposite end too */
					cdiv = copy_line(Mwdiv->line);
					reverse_line(cdiv);
					if ( !looped_line_crossing(cdiv, epos, &ipe, NullInt) )
						{
						pr_warning("Editor",
							"[modify_area_enhance] Problem with reverse cross over!\n");
						ipe = Mwdiv->line->numpts - 2 - ipx;
						copy_point(epos, spos);
						}

#					ifdef DEBUG_EDIT
					pr_diag("Editor",
						"[modify_area_enhance] Crossover at span %d of %d\n",
						ips, Mwdiv->line->numpts-1);
					pr_diag("Editor",
						"[modify_area_enhance] Reverse crossover at span %d of %d\n",
						ipe, cdiv->numpts-1);
#					endif /* DEBUG_EDIT */

					/* Truncate dividing line to location of cross over */
					/* Exclude cross over point and preceding point if  */
					/*  enough points found, to improve line smoothing  */
					if ( ips < 2 )
						{
						Mwdiv->line->numpts = ips + 1;
						add_point_to_line(Mwdiv->line, spos);
						}
					else if ( ips < 3 )
						Mwdiv->line->numpts = ips + 1;
					else
						Mwdiv->line->numpts = ips;

					/* Truncate dividing line from the opposite end too */
					/* Exclude cross over point and preceding point if  */
					/*  enough points found, to improve line smoothing  */
					if ( ipe < 2 )
						{
						cdiv->numpts = ipe + 1;
						add_point_to_line(cdiv, epos);
						}
					else if ( ipe < 3 )
						cdiv->numpts = ipe + 1;
					else
						cdiv->numpts = ipe;

					/* Set smoothing options */
					spfact = ModifySmth;
					spfact = MAX(spfact, 0.0);
					spfact = MIN(spfact, 100.0);

					/* Join the two portions of the dividing line */
					reverse_line(cdiv);
					if ( spfact > 1.0 )
						smjoin_lines(Mwdiv->line, cdiv, FilterRes/4, SplineRes);
					else
						append_line(Mwdiv->line, cdiv);
					cdiv = destroy_line(cdiv);

					/* Set flag for truncated dividing line */
					Mtrunc = TRUE;
					}

				/* Determine subarea attributes */
				(void) adjacent_subareas(Mwork, Mpmem, &lsub, &rsub);
				if (NotNull(lsub) && NotNull(rsub))
					{
					if (lsub->lspec.width > rsub->lspec.width)
							copy_lspec(&Mwdiv->lspec, &lsub->lspec);
					else	copy_lspec(&Mwdiv->lspec, &rsub->lspec);
					}
				else if (NotNull(lsub))
					copy_lspec(&Mwdiv->lspec, &lsub->lspec);
				else if (NotNull(rsub))
					copy_lspec(&Mwdiv->lspec, &rsub->lspec);

				/* Highlight the dividing line */
				widen_curve(Mwdiv, 2.0);
				highlight_curve(Mwdiv, 2);

				add_item_to_metafile(TempMeta, "curve", "c", "", "",
									(ITEM) Mwdiv);

				/* Highlight the points on the divide for point-by-point draw */
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
					Msize  = 0.5 * LmarkSize * gxGetMfact() / 1000;
					Lmarks = GETMEM(Lmarks, AREA, Mwdiv->line->numpts);
					for (nn=0; nn<Mwdiv->line->numpts; nn++)
						{
						lmark = create_area("", "", "");
						define_area_boundary(lmark, copy_line(Lbox));
						scale_line(lmark->bound->boundary, Msize, Msize);
						translate_line(lmark->bound->boundary,
										Mwdiv->line->points[nn][X],
										Mwdiv->line->points[nn][Y]);
						define_lspec(&lmark->lspec, 0, 0, NULL, False, 2.0, 0.0,
																	(HILITE) 3);
						add_item_to_metafile(TempMeta, "area",  "b", "", "",
											(ITEM) lmark);
						Lmarks[nn] = lmark;
						}
					}

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[modify_area_enhance] Adding Mwdiv to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				break;

		case AreaHole:
				Mwhole = create_curve("", "", "");
				Mwhole->line = copy_line(Mwork->bound->holes[Mpmem]);

				/* Use area attributes for hole */
				copy_lspec(&Mwhole->lspec, &Mwork->lspec);

				/* Highlight the hole */
				widen_curve(Mwhole, 2.0);
				highlight_curve(Mwhole, 2);

				add_item_to_metafile(TempMeta, "curve", "c", "", "",
									(ITEM) Mwhole);

				/* Highlight the points on the hole for point-by-point draw */
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
					Lmarks = GETMEM(Lmarks, AREA, Mwhole->line->numpts);
					for (nn=0; nn<Mwhole->line->numpts; nn++)
						{
						lmark = create_area("", "", "");
						define_area_boundary(lmark, copy_line(Lbox));
						scale_line(lmark->bound->boundary, Msize, Msize);
						translate_line(lmark->bound->boundary,
										Mwhole->line->points[nn][X],
										Mwhole->line->points[nn][Y]);
						define_lspec(&lmark->lspec, 0, 0, NULL, False, 2.0, 0.0,
																	(HILITE) 3);
						add_item_to_metafile(TempMeta, "area",  "b", "", "",
											(ITEM) lmark);
						Lmarks[nn] = lmark;
						}
					}

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[modify_area_enhance] Adding Mwhole to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				break;

		case AreaNone:
		case AreaBound:
				Mbound = Mwork->bound->boundary;

				/* Highlight the area and subarea */
				if (NotNull(Mpsub))
					{
					isub  = which_area_subarea(Mpick, Mpsub);
					if (NotNull(Mwork->subareas) && isub >= 0)
						Mwsub = Mwork->subareas[isub];

					/* Highlight the picked (sub)area */
					if (Mwork->numdiv <= 0)
						{
						widen_area(Mwork, 2.0);
						highlight_area(Mwork, 2, 50);
						}
					else
						{
						highlight_area(Mwork, 2, -1);
						widen_subarea(Mwsub, 3.0);
						highlight_subarea(Mwsub, 2, 50);
						}
					}

				/* Highlight the area */
				else
					{
					widen_area(Mwork, 2.0);
					highlight_area(Mwork, 2, 50);
					}

				/* Highlight the points on the boundary for point-by-point draw */
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
					Lmarks = GETMEM(Lmarks, AREA, Mbound->numpts);
					for (nn=0; nn<Mbound->numpts; nn++)
						{
						lmark = create_area("", "", "");
						define_area_boundary(lmark, copy_line(Lbox));
						scale_line(lmark->bound->boundary, Msize, Msize);
						translate_line(lmark->bound->boundary,
										Mbound->points[nn][X],
										Mbound->points[nn][Y]);
						define_lspec(&lmark->lspec, 0, 0, NULL, False, 2.0, 0.0,
																	(HILITE) 3);
						add_item_to_metafile(TempMeta, "area",  "b", "", "",
											(ITEM) lmark);
						Lmarks[nn] = lmark;
						}
					}
				break;
		}

	if (EditUndoable) (void) extract_area_order_tags(TRUE);
	}

/**********************************************************************/

static	MODSTATE	modify_area_draw(void)

	{
	POINT	spos, epos, *newpts;
	int		butt;
	int		nlines;
	LINE	*lines;
	int		npn, npb, ips, ipe, ipmid;
	LOGICAL	sclose, eclose, inl;
	float	dxs, dxe, dists, diste, distA, distB;
	float	spfact, smdist;
	LINE	abound, bbound;

	/* Make sure there is an area to modify */
	if (!Mwork)
		{
		put_message("area-none-picked");
		return ModDraw;
		}

	/* Turn on "Set" button if required */
	if (NotNull(Mpsub))
		edit_select((CAL) Mpsub->attrib, Mpset);
	else
		edit_select((CAL) Mpick->attrib, Mpset);

	/* Repeat until we get a new segment or another choice */
	while (TRUE)
		{
		/* Draw a new segment */
		put_message("area-mod-draw");
		/* >>>>> add this!!! <<<<< */
		post_partial(TRUE);
		/* >>>>> add this!!! <<<<< */
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
		put_message("area-mod-draw-rel");
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
		if (NotNull(Mpsub))
			edit_select((CAL) Mpsub->attrib, Mpset);
		else
			edit_select((CAL) Mpick->attrib, Mpset);

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
		pr_diag("Editor", "[edit_modify_area] Analysing new segment\n");
#		endif /* DEBUG */

		/* A new segment has been drawn */
		busy_cursor(TRUE);
		put_message("area-mod-proc");

		/* Extract new segment and clean up the buffer */
		if (!Mnew)  Mnew  = create_line();
		else        empty_line(Mnew);
		append_line(Mnew, lines[0]);
		reset_pipe();

		/* Set smoothing options */
		spfact = ModifySmth;
		spfact = MAX(spfact, 0.0);
		spfact = MIN(spfact, 100.0);

		/* Set parameters for new segment */
		npn    = Mnew->numpts;
		newpts = Mnew->points;

		/* Is start of new segment inside point markers for original boundary? */
		for (ips=0; ips<Mbound->numpts; ips++)
			{
			area_test_point(Lmarks[ips], newpts[0], NullFloat, NullPoint,
						(AMEMBER *) 0, NullInt, NullInt, &inl);
			if (inl)
				{
				copy_point(spos, Mbound->points[ips]);
				sclose = TRUE;
				break;
				}
			}

		/* Start of new segment not inside markers for original boundary! */
		/* Find closest point on original boundary to start of new segment */
		/* Decide whether to skip point on new segment if close enough */
		if (ips >= Mbound->numpts)
			{
			line_test_point(Mbound, newpts[0], &dists, spos, &ips,
								NullLogical, NullLogical);
			sclose = (LOGICAL) (dists < SplineRes);
			}

		/* Is end of new segment inside point markers for original boundary? */
		for (ipe=0; ipe<Mbound->numpts; ipe++)
			{
			area_test_point(Lmarks[ips], newpts[npn-1], NullFloat, NullPoint,
						(AMEMBER *) 0, NullInt, NullInt, &inl);
			if (inl)
				{
				copy_point(epos, Mbound->points[ipe]);
				eclose = TRUE;
				break;
				}
			}

		/* End of new segment not inside markers for original boundary! */
		/* Find closest point on original boundary to end of new segment */
		/* Decide whether to skip point on new segment if close enough */
		if (ipe >= Mbound->numpts)
			{
			line_test_point(Mbound, newpts[npn-1], &diste, epos, &ipe,
								NullLogical, NullLogical);
			eclose = (LOGICAL) (diste < SplineRes);
			}

		/* Construct two pieces of original boundary so that both parts */
		/* go from ipe to ips */
		npb = Mbound->numpts;
		if (!MsegA) MsegA = create_line();	else empty_line(MsegA);
		if (!MsegB) MsegB = create_line();	else empty_line(MsegB);
		add_point_to_line(MsegA, epos);
		add_point_to_line(MsegB, epos);
		if (ips == ipe)
			{

			/* Determine distances if intersection on same span */
			dxs = point_dist(spos, Mbound->points[ips]);
			dxe = point_dist(epos, Mbound->points[ipe]);
			if (dxs <= dxe)
				{
				append_line_pdir(MsegA, Mbound, ipe+1, npb-1, TRUE);
				append_line_pdir(MsegA, Mbound, 0,     ips,   TRUE);
				}
			else
				{
				append_line_pdir(MsegA, Mbound, 0,     ipe,   FALSE);
				append_line_pdir(MsegA, Mbound, ips+1, npb-1, FALSE);
				}
			}
		else if (ips < ipe)
			{
			append_line_pdir(MsegA, Mbound, ipe+1, npb-1, TRUE);
			append_line_pdir(MsegA, Mbound, 0,     ips,   TRUE);
			append_line_pdir(MsegB, Mbound, ips+1, ipe,   FALSE);
			}
		else
			{
			append_line_pdir(MsegA, Mbound, 0,     ipe,   FALSE);
			append_line_pdir(MsegA, Mbound, ips+1, npb-1, FALSE);
			append_line_pdir(MsegB, Mbound, ipe+1, ips,   TRUE);
			}
		add_point_to_line(MsegA, spos);
		add_point_to_line(MsegB, spos);
		condense_line(MsegA);
		condense_line(MsegB);

		/* Take care of smoothly joining the new segment to each of the */
		/* two pieces, depending on smoothing option selected */
		if ( spfact > 1.0 )
			{
			/* Back up a set distance from the ends of each segment to */
			/* make room for smoothing */
			/* (Move back between 0 and 2 spline steps) */
			smdist = SplineRes * (spfact*0.02);
			(void) trunc_line(Mnew, smdist, TRUE, TRUE);
			(void) trunc_line(MsegA, smdist, TRUE, TRUE);
			(void) trunc_line(MsegB, smdist, TRUE, TRUE);
			}

		/* Join each of the two pieces with the new line to form two */
		/* alternative areas */
		if ( spfact > 1.0 )
			{
			float fres = FilterRes/4;
			abound = copy_line(Mnew);
			bbound = copy_line(Mnew);
			smjoin_lines(abound, MsegA, fres, SplineRes);
			smjoin_lines(bbound, MsegB, fres, SplineRes);
			smclose_line(abound, fres, SplineRes);
			smclose_line(bbound, fres, SplineRes);
			}
		else
			{
#			ifdef DEBUG_STRUCTURES
			pr_diag("Editor",
				"[modify_area_draw] Adding unsmoothed segments!  spfact: %.6f\n",
				spfact);
#			endif /* DEBUG_STRUCTURES */
			if (sclose && eclose)
				{
				abound = append_line_portion(NullLine, Mnew, 1, Mnew->numpts-2);
				bbound = append_line_portion(NullLine, Mnew, 1, Mnew->numpts-2);
				}
			else if (sclose)
				{
				abound = append_line_portion(NullLine, Mnew, 1, Mnew->numpts-1);
				bbound = append_line_portion(NullLine, Mnew, 1, Mnew->numpts-1);
				}
			else if (eclose)
				{
				abound = append_line_portion(NullLine, Mnew, 0, Mnew->numpts-2);
				bbound = append_line_portion(NullLine, Mnew, 0, Mnew->numpts-2);
				}
			else
				{
				abound = copy_line(Mnew);
				bbound = copy_line(Mnew);
				}
			append_line(abound, MsegA);
			append_line(bbound, MsegB);
			close_line(abound);
			close_line(bbound);
			}
		MareaA = copy_area(Mwork, TRUE);
		MareaB = copy_area(Mwork, TRUE);
		define_area_boundary(MareaA, abound);
		define_area_boundary(MareaB, bbound);

		/* Set default area to keep (implies which segment to discard) */
		/* Default to discard closest segment to mid-point of new segment */
		/* Therefore keep the area made from the other segment */
		ipmid  = Mnew->numpts / 2;
		newpts = Mnew->points;
		line_test_point(MsegA, newpts[ipmid], &distA, NULL, NULL, NULL, NULL);
		line_test_point(MsegB, newpts[ipmid], &distB, NULL, NULL, NULL, NULL);
		Makeep = (distA < distB)? MareaB: MareaA;

		/* Highlight the two new areas */
		highlight_area(Mwork,  2, -1);
		highlight_area(MareaA, 2, -1);
		highlight_area(MareaB, 2, -1);
		highlight_area(Makeep, 3, -1);
		widen_area(Makeep, 2.0);

		if (Makeep == MareaA)
			{
			add_item_to_metafile(TempMeta, "area", "c", "", "", (ITEM) MareaB);
			add_item_to_metafile(TempMeta, "area", "c", "", "", (ITEM) MareaA);
#			ifdef DEBUG_STRUCTURES
			pr_diag("Editor",
				"[modify_area_draw] Adding MareaB/MareaA to TempMeta (%d)\n",
				TempMeta->numfld);
#			endif /* DEBUG_STRUCTURES */
			}
		else
			{
			add_item_to_metafile(TempMeta, "area", "c", "", "", (ITEM) MareaA);
			add_item_to_metafile(TempMeta, "area", "c", "", "", (ITEM) MareaB);
#			ifdef DEBUG_STRUCTURES
			pr_diag("Editor",
				"[modify_area_draw] Adding MareaA/MareaB to TempMeta (%d)\n",
				TempMeta->numfld);
#			endif /* DEBUG_STRUCTURES */
			}

		/* Show the results */
		present_all();
		busy_cursor(FALSE);
		clear_message();
		return ModDiscard;
		}
	}

/**********************************************************************/

static	MODSTATE	modify_area_discard(void)

	{
	POINT	pos;
	int		butt;
	float	distN, distA, distB;

	/* Make sure there is something to discard */
	if (!MareaA || !MareaB)
		{
		put_message("area-no-discard");
		return ModDiscard;
		}

	/* Repeat until we have a selection */
	while (TRUE)
		{

		/* Get a point */
		put_message("area-mod-discard");
		if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return ModDiscard;
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}

		/* Choose a segment to discard */
		put_message("area-mod-disc-rel");
		(void) pick_Xpoint(DnEdit, 0, pos, &butt);
		if (!inside_dn_window(DnEdit, pos))
			{
			put_message("edit-outside-map");
			(void) sleep(1);
			continue;
			}

		/* Un-highlight the previously picked area */
		highlight_area(Makeep, 2, -1);
		widen_area(Makeep, -2.0);

		/* See which segment was picked */
		line_test_point(Mnew, pos, &distN, NULL, NULL, NULL, NULL);
		line_test_point(MsegA, pos, &distA, NULL, NULL, NULL, NULL);
		line_test_point(MsegB, pos, &distB, NULL, NULL, NULL, NULL);
		if ((distN < distA) && (distN < distB)) Makeep = Mwork;
		else Makeep = (distA < distB)? MareaB: MareaA;

		/* Highlight the the new picked area */
		highlight_area(Makeep, 3, -1);
		widen_area(Makeep, 2.0);

		/* Re-insert the areas with "keep" last */
		Mtemp = find_mf_set(TempMeta, "area", "c", "", "");

#		ifdef DEBUG_STRUCTURES
		pr_diag("Editor",
			"[modify_area_discard]  Mtemp->num: %d\n", Mtemp->num);
#		endif /* DEBUG_STRUCTURES */

		Mtemp->num = 0;
		if (Makeep == MareaA)
			{
			add_item_to_metafile(TempMeta, "area", "c", "", "", (ITEM) Mwork);
			add_item_to_metafile(TempMeta, "area", "c", "", "", (ITEM) MareaB);
			add_item_to_metafile(TempMeta, "area", "c", "", "", (ITEM) MareaA);
#			ifdef DEBUG_STRUCTURES
			pr_diag("Editor",
				"[modify_area_discard] Adding Mwork/MareaB/MareaA to TempMeta (%d)\n",
				TempMeta->numfld);
#			endif /* DEBUG_STRUCTURES */
			}
		else if (Makeep == MareaB)
			{
			add_item_to_metafile(TempMeta, "area", "c", "", "", (ITEM) Mwork);
			add_item_to_metafile(TempMeta, "area", "c", "", "", (ITEM) MareaA);
			add_item_to_metafile(TempMeta, "area", "c", "", "", (ITEM) MareaB);
#			ifdef DEBUG_STRUCTURES
			pr_diag("Editor",
				"[modify_area_discard] Adding Mwork/MareaA/MareaB to TempMeta (%d)\n",
				TempMeta->numfld);
#			endif /* DEBUG_STRUCTURES */
			}
		else
			{
			add_item_to_metafile(TempMeta, "area", "c", "", "", (ITEM) MareaA);
			add_item_to_metafile(TempMeta, "area", "c", "", "", (ITEM) MareaB);
			add_item_to_metafile(TempMeta, "area", "c", "", "", (ITEM) Mwork);
#			ifdef DEBUG_STRUCTURES
			pr_diag("Editor",
				"[modify_area_discard] Adding MareaA/MareaB/Mwork to TempMeta (%d)\n",
				TempMeta->numfld);
#			endif /* DEBUG_STRUCTURES */
			}

		/* Show the results */
		present_all();
		}
	}

/**********************************************************************/

static	MODSTATE	modify_area_sculpt(void)

	{
	int		butt;
	POINT	pos;
	LOGICAL	right;

	/* Make sure there is an area to modify */
	if (!Mwork)
		{
		put_message("area-none-picked");
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
		put_message("area-sculpt");
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

		/* Edit the area */
		put_message("area-sculpt-rel");
		MareaS = copy_area(Mwork, TRUE);
		circle_echo(FALSE);

		/* Invoke the sculpting tool */
		if (!uedit_Xcurve(DnEdit, MareaS->bound->boundary, NullSubArea,
				EdRad, EdSpr, &butt, &right))
			{
			if (MareaS->bound->boundary->numpts <= 2)
				{
				/* Delete the area from the area set */
				put_message("area-deleting");
				MareaS = destroy_area(MareaS);
				(void) sleep(1);
				return ModDelete;
				}
			else
				{
				put_message("edit-no-edit");
				MareaS = destroy_area(MareaS);
				(void) sleep(1);
				continue;
				}
			}

		/* Show the results */
		clear_message();
		return ModSculptDone;
		}
	}

/**********************************************************************/

static	MODSTATE	modify_area_draw_div(void)

	{
	POINT	spos, epos, tpos, *newpts;
	int		butt;
	int		nlines;
	LINE	*lines;
	int		npn, npd, ips, ipe, ipt, ipx;
	LOGICAL	joins, joine, joint, inl;
	float	dxs, dxe, dists, diste, distt;
	float	spfact, smdist;
	LINE	divln, sseg, eseg, cdiv;
	DIVSTAT	dstat;
	STRING	msgkey;

	/* Make sure there is an area and dividing line to modify */
	if (!Mwork || !Mwdiv || !Mxsub)
		{
		put_message("area-none-picked");
		return ModDraw;
		}

	/* Repeat until we get a new segment or another choice */
	while (TRUE)
		{
		/* Draw a new segment */
		put_message("area-mod-draw");
		/* >>>>> add this!!! <<<<< */
		post_partial(TRUE);
		/* >>>>> add this!!! <<<<< */
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
		put_message("area-mod-draw-rel");
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
		pr_diag("Editor", "[edit_modify_area] Analysing new segment\n");
#		endif /* DEBUG */

		/* A new segment has been drawn */
		busy_cursor(TRUE);
		put_message("area-mod-proc");

		/* Extract new segment and clean up the buffer */
		if (!Mnew)  Mnew  = create_line();
		else        empty_line(Mnew);
		append_line(Mnew, lines[0]);
		reset_pipe();

		/* Set smoothing options */
		spfact = ModifySmth;
		spfact = MAX(spfact, 0.0);
		spfact = MIN(spfact, 100.0);

		/* Set parameters for new segment */
		npn    = Mnew->numpts;
		newpts = Mnew->points;
		divln  = Mwdiv->line;

		/* Is start of new segment inside point markers for original divide? */
		for (ips=0; ips<divln->numpts; ips++)
			{
			area_test_point(Lmarks[ips], newpts[0], NullFloat, NullPoint,
						(AMEMBER *) 0, NullInt, NullInt, &inl);
			if (inl)
				{
				copy_point(spos, divln->points[ips]);
				joins = TRUE;
				break;
				}
			}

		/* Start of new segment not inside markers for original dividing line! */
		/* Find closest point on original dividing line to start of new segment */
		/* Decide whether to join at this point if close enough */
		if (ips >= divln->numpts)
			{
			line_test_point(divln, newpts[0], &dists, spos, &ips,
								NullLogical, NullLogical);
			joins = (LOGICAL) (dists <= SplineRes);
			}

		/* Is end of new segment inside point markers for original divide? */
		for (ipe=0; ipe<divln->numpts; ipe++)
			{
			area_test_point(Lmarks[ipe], newpts[npn-1], NullFloat, NullPoint,
						(AMEMBER *) 0, NullInt, NullInt, &inl);
			if (inl)
				{
				copy_point(epos, divln->points[ipe]);
				joine = TRUE;
				break;
				}
			}

		/* End of new segment not inside markers for original dividing line! */
		/* Find closest point on original dividing line to end of new line */
		/* Decide whether to join at this point if close enough */
		if (ipe >= divln->numpts)
			{
			line_test_point(divln, newpts[npn-1], &diste, epos, &ipe,
								NullLogical, NullLogical);
			joine = (LOGICAL) (diste <= SplineRes);
			}

		/* Determine distances if intersections are on same span */
		dxs = dxe = 0.0;
		if (ips == ipe)
			{
			dxs = point_dist(spos, divln->points[ips]);
			dxe = point_dist(epos, divln->points[ipe]);
			}

		/* Force new line to match the sense of the original line */
		if (ips > ipe || (ips == ipe && dxs > dxe))
			{
#			ifdef DEBUG
			pr_diag("Editor", "[edit_modify_area] Reversing new segment\n");
#			endif /* DEBUG */
			ipt = ips;	distt = dists;	joint = joins;	copy_point(tpos, spos);
			ips = ipe;	dists = diste;	joins = joine;	copy_point(spos, epos);
			ipe = ipt;	diste = distt;	joine = joint;	copy_point(epos, tpos);
			reverse_line(Mnew);
			}

		/* Break the original line into the before and after parts */
		sseg = NullLine;
		eseg = NullLine;
		npd  = divln->numpts;
		if (joins)
			{
			sseg = create_line();
			append_line_portion(sseg, divln, 0, ips);
			add_point_to_line(sseg, spos);
			condense_line(sseg);
			}
		if (joine)
			{
			eseg = create_line();
			add_point_to_line(eseg, epos);
			append_line_portion(eseg, divln, ipe+1, npd-1);
			condense_line(eseg);
			}

		/* Take care of smoothly joining the new segment to one or both */
		/* ends of the original, depending on smoothing option selected */
		if ( spfact > 1.0 )
			{
			/* Back up a set distance from the ends of each segment to */
			/* make room for smoothing */
			/* (Move back between 0 and 2 spline steps) */
			smdist = SplineRes * (spfact*0.02);
			(void) trunc_line(Mnew, smdist, joins, joine);
			if (joins) (void) trunc_line(sseg, smdist, FALSE, TRUE);
			if (joine) (void) trunc_line(eseg, smdist, TRUE, FALSE);
			}

		/* Construct modified curve from start and end of original line */
		/* with new line in the middle */
		Mbdiv = copy_curve(Mwdiv);
		empty_curve(Mbdiv);
		if ( spfact > 1.0 )
			{
			float   fres = FilterRes/4;
			if (joins)
				{
				append_line(Mbdiv->line, sseg);
				smjoin_lines(Mbdiv->line, Mnew, fres, SplineRes);
				}
			else append_line(Mbdiv->line, Mnew);
			if (joine) smjoin_lines(Mbdiv->line, eseg, fres, SplineRes);
			}
		else
			{
			if (joins && joine)
				{
				append_line(Mbdiv->line, sseg);
				append_line_portion(Mbdiv->line, Mnew, 1, Mnew->numpts-2);
				append_line(Mbdiv->line, eseg);
				}
			else if (joins)
				{
				append_line(Mbdiv->line, sseg);
				append_line_portion(Mbdiv->line, Mnew, 1, Mnew->numpts-1);
				}
			else if (joine)
				{
				append_line_portion(Mbdiv->line, Mnew, 0, Mnew->numpts-2);
				append_line(Mbdiv->line, eseg);
				}
			else
				{
				append_line(Mbdiv->line, Mnew);
				}
			}
		sseg = destroy_line(sseg);
		eseg = destroy_line(eseg);

		/* Clip the new dividing line to area boundary */
		cdiv = clip_divline_to_subarea(Mxsub, Mbdiv->line, TRUE, TRUE, &dstat);
		if (!cdiv)
			{
			switch (dstat)
				{
				case DivAreaRight:
				case DivAreaLeft:
					msgkey = "area-div-out1";	break;
				case DivTooShort:
					msgkey = "area-div-out2";	break;
				default:
					msgkey = "area-div-out3";	break;
				}
			put_message(msgkey);
			(void) sleep(1);
			busy_cursor(FALSE);
			return ModReDraw;
			}
		empty_curve(Mbdiv);
		add_line_to_curve(Mbdiv, cdiv);
		cdiv = destroy_line(cdiv);

		/* Truncate the dividing line if it crosses itself */
		if ( looped_line_crossing(Mbdiv->line, spos, &ips, &ipx) )
			{

			/* A dividing line that crosses itself has been found */
			pr_warning("Editor",
				"[modify_area_draw_div] Truncating dividing line cross over!\n");
			put_message("area-div-cross");
			(void) sleep(1);

			/* Find first cross over from the opposite end too */
			cdiv = copy_line(Mbdiv->line);
			reverse_line(cdiv);
			if ( !looped_line_crossing(cdiv, epos, &ipe, NullInt) )
				{
				pr_warning("Editor",
					"[modify_area_draw_div] Problem with reverse cross over!\n");
				ipe = Mbdiv->line->numpts - 2 - ipx;
				copy_point(epos, spos);
				}

#			ifdef DEBUG_EDIT
			pr_diag("Editor",
				"[modify_area_draw_div] Crossover at span %d of %d\n",
				ips, Mbdiv->line->numpts-1);
			pr_diag("Editor",
				"[modify_area_draw_div] Reverse crossover at span %d of %d\n",
				ipe, cdiv->numpts-1);
#			endif /* DEBUG_EDIT */

			/* Truncate dividing line to location of cross over */
			/* Exclude cross over point and preceding point if  */
			/*  enough points found, to improve line smoothing  */
			if ( ips < 2 )
				{
				Mbdiv->line->numpts = ips + 1;
				add_point_to_line(Mbdiv->line, spos);
				}
			else if ( ips < 3 )
				Mbdiv->line->numpts = ips + 1;
			else
				Mbdiv->line->numpts = ips;

			/* Truncate dividing line from the opposite end too */
			/* Exclude cross over point and preceding point if  */
			/*  enough points found, to improve line smoothing  */
			if ( ipe < 2 )
				{
				cdiv->numpts = ipe + 1;
				add_point_to_line(cdiv, epos);
				}
			else if ( ipe < 3 )
				cdiv->numpts = ipe + 1;
			else
				cdiv->numpts = ipe;

			/* Join the two portions of the dividing line */
			reverse_line(cdiv);
			if ( spfact > 1.0 )
				smjoin_lines(Mbdiv->line, cdiv, FilterRes/4, SplineRes);
			else
				append_line(Mbdiv->line, cdiv);
			cdiv = destroy_line(cdiv);
			}

		/* Highlight the new dividing line */
		highlight_curve(Mwdiv, 2);
		highlight_curve(Mbdiv, 3);

		add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) Mbdiv);

#		ifdef DEBUG_STRUCTURES
		pr_diag("Editor",
			"[modify_area_draw_div] Adding Mbdiv to TempMeta (%d)\n",
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

static	MODSTATE	modify_area_discard_div(void)

	{
	int		butt;

	/* Make sure there is something to discard */
	if (!Mbdiv)
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

static	MODSTATE	modify_area_sculpt_div(void)

	{
	int		butt;
	POINT	pos;
	LOGICAL	right;

	/* Make sure there is an area and dividing line to modify */
	if (!Mwork || !Mwdiv || !Mxsub)
		{
		put_message("area-none-picked");
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
		put_message("area-sculpt");
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

		/* Edit the dividing line */
		put_message("area-sculpt-rel");
		Mbdiv = copy_curve(Mwdiv);
		circle_echo(FALSE);

		/* Invoke the sculpting tool */
		if (!uedit_Xcurve(DnEdit, Mbdiv->line, Mxsub, EdRad, EdSpr,
				&butt, &right))
			{
			if (Mbdiv->line->numpts <= 1)
				{
				/* Delete dividing line from area set */
				Mbdiv  = destroy_curve(Mbdiv);
				Mright = right;
				return ModDivideGone;
				}
			else
				{
				put_message("edit-no-edit");
				Mbdiv = destroy_curve(Mbdiv);
				(void) sleep(1);
				continue;
				}
			}

		/* Turn off flag if truncated dividing line has been modified */
		Mtrunc = FALSE;

		/* Show the results */
		clear_message();
		return ModSculptDone;
		}
	}

/**********************************************************************/

static	MODSTATE	modify_area_draw_hole(void)

	{
	POINT	spos, epos, *newpts;
	int		butt;
	int		nlines;
	LINE	*lines;
	int		npn, nph, ips, ipe, ipmid;
	LOGICAL	sclose, eclose, inl;
	float	dists, diste, distA, distB;
	float	spfact, smdist;
	LINE	hole;

	/* Make sure there is an area and hole to modify */
	if (!Mwork || !Mwhole)
		{
		put_message("area-none-picked");
		return ModDraw;
		}

	/* Repeat until we get a new segment or another choice */
	while (TRUE)
		{
		/* Draw a new segment */
		put_message("area-mod-draw");
		/* >>>>> add this!!! <<<<< */
		post_partial(TRUE);
		/* >>>>> add this!!! <<<<< */
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
		put_message("area-mod-draw-rel");
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
		pr_diag("Editor", "[edit_modify_area] Analysing new segment\n");
#		endif /* DEBUG */

		/* A new segment has been drawn */
		busy_cursor(TRUE);
		put_message("area-mod-proc");

		/* Extract new segment and clean up the buffer */
		if (!Mnew)  Mnew  = create_line();
		else        empty_line(Mnew);
		append_line(Mnew, lines[0]);
		reset_pipe();

		/* Set smoothing options */
		spfact = ModifySmth;
		spfact = MAX(spfact, 0.0);
		spfact = MIN(spfact, 100.0);

		/* Set parameters for new segment */
		npn    = Mnew->numpts;
		newpts = Mnew->points;
		hole   = Mwhole->line;

		/* Is start of new segment inside point markers for original hole? */
		for (ips=0; ips<hole->numpts; ips++)
			{
			area_test_point(Lmarks[ips], newpts[0], NullFloat, NullPoint,
						(AMEMBER *) 0, NullInt, NullInt, &inl);
			if (inl)
				{
				copy_point(spos, hole->points[ips]);
				sclose = TRUE;
				break;
				}
			}

		/* Start of new segment not inside markers for original hole! */
		/* Find closest point on original hole to start of new segment */
		/* Decide whether to skip point on new segment if close enough */
		if (ips >= hole->numpts)
			{
			line_test_point(hole, newpts[0], &dists, spos, &ips,
								NullLogical, NullLogical);
			sclose = (LOGICAL) (dists < SplineRes);
			}

		/* Is end of new segment inside point markers for original hole? */
		for (ipe=0; ipe<hole->numpts; ipe++)
			{
			area_test_point(Lmarks[ips], newpts[npn-1], NullFloat, NullPoint,
						(AMEMBER *) 0, NullInt, NullInt, &inl);
			if (inl)
				{
				copy_point(epos, hole->points[ipe]);
				eclose = TRUE;
				break;
				}
			}

		/* End of new segment not inside markers for original hole! */
		/* Find closest point on original hole to end of new segment */
		/* Decide whether to skip point on new segment if close enough */
		if (ipe >= hole->numpts)
			{
			line_test_point(hole, newpts[npn-1], &diste, epos, &ipe,
								NullLogical, NullLogical);
			eclose = (LOGICAL) (diste < SplineRes);
			}

		/* Construct two pieces of original hole so that both parts */
		/* go from ipe to ips */
		nph = hole->numpts;
		if (!MsegA) MsegA = create_line();	else empty_line(MsegA);
		if (!MsegB) MsegB = create_line();	else empty_line(MsegB);
		add_point_to_line(MsegA, epos);
		add_point_to_line(MsegB, epos);
		if (ips <= ipe)
			{
			append_line_pdir(MsegA, hole, ipe+1, nph-1, TRUE);
			append_line_pdir(MsegA, hole, 0,     ips,   TRUE);
			append_line_pdir(MsegB, hole, ips+1, ipe,   FALSE);
			}
		else
			{
			append_line_pdir(MsegA, hole, 0,     ipe,   FALSE);
			append_line_pdir(MsegA, hole, ips+1, nph-1, FALSE);
			append_line_pdir(MsegB, hole, ipe+1, ips,   TRUE);
			}
		add_point_to_line(MsegA, spos);
		add_point_to_line(MsegB, spos);
		condense_line(MsegA);
		condense_line(MsegB);

		/* Take care of smoothly joining the new segment to each of the */
		/* two pieces, depending on smoothing option selected */
		if ( spfact > 1.0 )
			{
			/* Back up a set distance from the ends of each segment to */
			/* make room for smoothing */
			/* (Move back between 0 and 2 spline steps) */
			smdist = SplineRes * (spfact*0.02);
			(void) trunc_line(Mnew,  smdist, TRUE, TRUE);
			(void) trunc_line(MsegA, smdist, TRUE, TRUE);
			(void) trunc_line(MsegB, smdist, TRUE, TRUE);
			}

		/* Join each of the two pieces with the new line to form two */
		/* alternative holes */
		MholeA = copy_curve(Mwhole);
		MholeB = copy_curve(Mwhole);
		empty_curve(MholeA);
		empty_curve(MholeB);
		if ( spfact > 1.0 )
			{
			float fres = FilterRes/4;
			append_line(MholeA->line, Mnew);
			append_line(MholeB->line, Mnew);
			smjoin_lines(MholeA->line, MsegA, fres, SplineRes);
			smjoin_lines(MholeB->line, MsegB, fres, SplineRes);
			smclose_line(MholeA->line, fres, SplineRes);
			smclose_line(MholeB->line, fres, SplineRes);
			}
		else
			{
			if (sclose && eclose)
				{
				append_line_portion(MholeA->line, Mnew, 1, Mnew->numpts-2);
				append_line_portion(MholeB->line, Mnew, 1, Mnew->numpts-2);
				}
			else if (sclose)
				{
				append_line_portion(MholeA->line, Mnew, 1, Mnew->numpts-1);
				append_line_portion(MholeB->line, Mnew, 1, Mnew->numpts-1);
				}
			else if (eclose)
				{
				append_line_portion(MholeA->line, Mnew, 0, Mnew->numpts-2);
				append_line_portion(MholeB->line, Mnew, 0, Mnew->numpts-2);
				}
			else
				{
				append_line(MholeA->line, Mnew);
				append_line(MholeB->line, Mnew);
				}
			append_line(MholeA->line, MsegA);
			append_line(MholeB->line, MsegB);
			close_line(MholeA->line);
			close_line(MholeB->line);
			}

		/* Set default hole to keep (implies which segment to discard) */
		/* Default to discard closest segment to mid-point of new segment */
		/* Therefore keep the hole made from the other segment */
		ipmid  = Mnew->numpts / 2;
		newpts = Mnew->points;
		line_test_point(MsegA, newpts[ipmid], &distA, NULL, NULL, NULL, NULL);
		line_test_point(MsegB, newpts[ipmid], &distB, NULL, NULL, NULL, NULL);
		Mhkeep = (distA < distB)? MholeB: MholeA;

		/* Highlight the two new holes */
		highlight_curve(Mwhole, 2);
		highlight_curve(MholeA, 2);
		highlight_curve(MholeB, 2);
		highlight_curve(Mhkeep, 3);
		widen_curve(Mhkeep, 2.0);

		if (Mhkeep == MholeA)
			{
			add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) MholeB);
			add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) MholeA);
#			ifdef DEBUG_STRUCTURES
			pr_diag("Editor",
				"[modify_area_draw] Adding MholeB/MholeA to TempMeta (%d)\n",
				TempMeta->numfld);
#			endif /* DEBUG_STRUCTURES */
			}
		else
			{
			add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) MholeA);
			add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) MholeB);
#			ifdef DEBUG_STRUCTURES
			pr_diag("Editor",
				"[modify_area_draw] Adding MholeA/MholeB to TempMeta (%d)\n",
				TempMeta->numfld);
#			endif /* DEBUG_STRUCTURES */
			}

		/* Show the results */
		present_all();
		busy_cursor(FALSE);
		clear_message();
		return ModDiscard;
		}
	}

/**********************************************************************/

static	MODSTATE	modify_area_discard_hole(void)

	{
	POINT	pos;
	int		butt;
	float	distN, distA, distB;

	/* Make sure there is something to discard */
	if (!MholeA || !MholeB)
		{
		put_message("area-no-discard");
		return ModDiscard;
		}

	/* Repeat until we have a selection */
	while (TRUE)
		{

		/* Get a point */
		put_message("area-mod-discard");
		if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return ModDiscard;
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}

		/* Choose a segment to discard */
		put_message("area-mod-disc-rel");
		(void) pick_Xpoint(DnEdit, 0, pos, &butt);
		if (!inside_dn_window(DnEdit, pos))
			{
			put_message("edit-outside-map");
			(void) sleep(1);
			continue;
			}

		/* Un-highlight the previously picked hole */
		highlight_curve(Mhkeep, 2);
		widen_curve(Mhkeep, -2.0);

		/* See which segment was picked */
		line_test_point(Mnew,  pos, &distN, NULL, NULL, NULL, NULL);
		line_test_point(MsegA, pos, &distA, NULL, NULL, NULL, NULL);
		line_test_point(MsegB, pos, &distB, NULL, NULL, NULL, NULL);
		if ((distN < distA) && (distN < distB)) Mhkeep = Mwhole;

		else Mhkeep = (distA < distB)? MholeB: MholeA;

		/* Highlight the the new picked hole */
		highlight_curve(Mhkeep, 3);
		widen_curve(Mhkeep, 2.0);

		/* Re-insert the holes with "keep" last */
		Mtemp = find_mf_set(TempMeta, "curve", "c", "", "");

#		ifdef DEBUG_STRUCTURES
		pr_diag("Editor",
			"[modify_area_discard]  Mtemp->num: %d\n", Mtemp->num);
#		endif /* DEBUG_STRUCTURES */

		Mtemp->num = 0;
		if (Mhkeep == MholeA)
			{
			add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) Mwhole);
			add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) MholeB);
			add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) MholeA);
#			ifdef DEBUG_STRUCTURES
			pr_diag("Editor",
				"[modify_area_discard] Adding Mwhole/MholeB/MholeA to TempMeta (%d)\n",
				TempMeta->numfld);
#			endif /* DEBUG_STRUCTURES */
			}
		else if (Mhkeep == MholeB)
			{
			add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) Mwhole);
			add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) MholeA);
			add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) MholeB);
#			ifdef DEBUG_STRUCTURES
			pr_diag("Editor",
				"[modify_area_discard] Adding Mwhole/MholeA/MholeB to TempMeta (%d)\n",
				TempMeta->numfld);
#			endif /* DEBUG_STRUCTURES */
			}
		else
			{
			add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) MholeA);
			add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) MholeB);
			add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) Mwhole);
#			ifdef DEBUG_STRUCTURES
			pr_diag("Editor",
				"[modify_area_discard] Adding MholeA/MholeB/Mwhole to TempMeta (%d)\n",
				TempMeta->numfld);
#			endif /* DEBUG_STRUCTURES */
			}

		/* Show the results */
		present_all();
		}
	}

/**********************************************************************/

static	MODSTATE	modify_area_sculpt_hole(void)

	{
	int		butt;
	POINT	pos;
	LOGICAL	right;

	/* Make sure there is an area and hole to modify */
	if (!Mwork || !Mwhole)
		{
		put_message("area-none-picked");
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
		put_message("area-sculpt");
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

		/* Edit the hole */
		put_message("area-sculpt-rel");
		MholeS = copy_curve(Mwhole);
		circle_echo(FALSE);

		/* Invoke the sculpting tool */
		if (!uedit_Xcurve(DnEdit, MholeS->line, NullSubArea, EdRad, EdSpr,
				&butt, &right))
			{
			if (MholeS->line->numpts <= 2)
				{
				/* Delete hole from area set */
				MholeS  = destroy_curve(MholeS);
				return ModHoleGone;
				}
			else
				{
				put_message("edit-no-edit");
				MholeS = destroy_curve(MholeS);
				(void) sleep(1);
				continue;
				}
			}

		/* Show the results */
		clear_message();
		return ModSculptDone;
		}
	}

/**********************************************************************/

#ifdef NOT_READY
static	void	modify_show

	(
	DVDSHOW	show
	)

	{
	static	DVDSHOW	Prev = DvdShowNone;

	if (show == Prev) return;

	/* Un-show what was already shown */
	switch (Prev)
		{
		case DvdShowPick:
					highlight_area(DWarea, -1, -1);
					if (IsNull(DWsub)) break;
					widen_subarea(DWsub, -2.0);
					highlight_subarea(DWsub, -1, -1);
					break;

		case DvdShowA:
					if (IsNull(DnewA)) break;
					widen_subarea(DnewA, -2.0);
					highlight_subarea(DnewA, 2, -1);
					break;

		case DvdShowB:
					if (IsNull(DnewB)) break;
					widen_subarea(DnewB, -2.0);
					highlight_subarea(DnewB, 2, -1);
					break;

		case DvdShowNone:
		default:	break;
		}

	/* Show what is being requested */
	switch (show)
		{
		case DvdShowPick:
					highlight_area(DWarea, -1, -1);
					if (IsNull(DWsub)) break;
					widen_subarea(DWsub, 2.0);
					highlight_subarea(DWsub, 2, 1);
					break;

		case DvdShowA:
					highlight_area(DWarea, -1, -1);
					if (IsNull(DnewA)) break;
					widen_subarea(DnewA, 2.0);
					highlight_subarea(DnewA, 3, 1);
					break;

		case DvdShowB:
					highlight_area(DWarea, -1, -1);
					if (IsNull(DnewB)) break;
					widen_subarea(DnewB, 2.0);
					highlight_subarea(DnewB, 3, 1);
					break;

		case DvdShowNone:
		default:	break;
		}

	present_all();
	Prev = show;
	}
#endif /* NOT_READY */

/***********************************************************************
*                                                                      *
*     d i v i d e _ a r e a                                            *
*     d i v i d e _ a r e a _ p i c k                                  *
*     d i v i d e _ a r e a _ d r a w                                  *
*     d i v i d e _ a r e a _ s e t                                    *
*     d i v i d e _ a r e a _ d e f a u l t s                          *
*                                                                      *
***********************************************************************/

typedef	enum
			{
			DivPick, DivDraw, DivSetA, DivSetB,	/* interactive states */
			DivJoin,
			DivRePick, DivReDraw, DivReset,		/* "cancel" states */
			DivDivide, DivConfirm, DivBailout	/* complete modification */
			}
		DVDSTATE;

typedef	enum
			{
			DvdShowNone, DvdShowPick, DvdShowA, DvdShowB
			}
		DVDSHOW;

static	DVDSTATE	divide_area_pick(void);
static	DVDSTATE	divide_area_draw(void);
static	DVDSTATE	divide_area_set(DVDSTATE);
static	void		divide_area_defaults(void);
static	void		divide_show(DVDSHOW);

/* For debug purposes */
static	void		show_dvd_state(STRING, DVDSTATE);

static	void		show_dvd_state
	(
	STRING		pre,
	DVDSTATE	state
	)
	{
	STRING	name;

#	ifdef DEBUG
	switch (state)
		{
		case DivPick:		name = "Pick";			break;
		case DivDraw:		name = "Draw";			break;
		case DivSetA:		name = "Set A";			break;
		case DivSetB:		name = "Set B";			break;
		case DivJoin:		name = "ReJoin";		break;
		case DivRePick:		name = "RePick";		break;
		case DivReDraw:		name = "ReDraw";		break;
		case DivReset:		name = "Reset";			break;
		case DivDivide:		name = "Divide";		break;
		case DivConfirm:	name = "Confirm";		break;
		case DivBailout:	name = "BailOut";		break;
		default:			name = "!Unknown!";		break;
		}
	pr_diag("Editor", "[edit_divide_area] State: %s %s\n", pre, name);
#	endif /* DEBUG */

	return;
	}
/* For debug purposes */

static	LOGICAL	ReJoin = FALSE;		/* Can picked subarea be re-joined? */
static	LOGICAL	Dstart = FALSE;		/* Divide started (but not completed?) */
static	AREA	DParea = NullArea;	/* Picked area */
static	SUBAREA	DPsub  = NullSubArea;	/* Picked subarea */
static	SUBAREA	DJsub  = NullSubArea;	/* Picked subarea to join*/
static	AREA	DWarea = NullArea;	/* Working copy of picked area */
static	SUBAREA	DWsub  = NullSubArea;	/* Working copy of picked subarea */
static	SUBAREA	DXsub  = NullSubArea;	/* Working copy of subarea to re-join */
static	LINE	Ddiv   = NullLine;		/* Dividing segment */
static	SUBAREA	DnewA  = NullSubArea;	/* New subarea built from 1st piece */
static	SUBAREA	DnewB  = NullSubArea;	/* New subarea built from 2nd piece */

static	CAL		CalA   = NullCal;
static	CAL		CalB   = NullCal;

/**********************************************************************/

LOGICAL	edit_divide_area

	(
	STRING	mode,
	CAL		cal
	)

	{
	int			isub;
	DVDSTATE	next;
	LINE		divl;
	SUBAREA		subA, subB;
	DIVSTAT		dstat;
	LOGICAL		drawn = FALSE;

	/* Current state: */
	/* - where should we resume? */
	static	DVDSTATE	State = DivPick;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_divide_area] %s  State: %d\n", mode, State);
#	endif /* DEBUG */
	show_dvd_state("INITIAL", State);

	/* Clean up previous edits */
	if (same(mode, "cancel"))
		{
		switch (State)
			{
			case DivSetB:	State = DivReset;
							show_dvd_state("RESET", State);
							break;

			case DivSetA:	State = DivReDraw;
							show_dvd_state("RESET", State);
							break;

			case DivDraw:	State = (Dstart)? DivReDraw: DivRePick;
							show_dvd_state("RESET", State);
							if (Dstart) divide_show(DvdShowNone);
							Dstart = FALSE;
							break;

			default:		empty_temp();
							drawing_control(FALSE);
							edit_can_rejoin(FALSE);
							return FALSE;
			}
		}
	if (same(mode, "cancel all"))
		{
		ignore_partial();

		/* Clean up temporary buffers */
		edit_select(NullCal, FALSE);
		DParea = NullArea;
		DPsub  = NullSubArea;
		DJsub  = NullSubArea;
		DWarea = NullArea;
		DWsub  = NullSubArea;
		DXsub  = NullSubArea;
		DnewA  = NullSubArea;
		DnewB  = NullSubArea;
		Ddiv   = destroy_line(Ddiv);

		/* Empty display structure */
		empty_temp();
		drawing_control(FALSE);
		edit_can_rejoin(FALSE);
		/* >>>>> Do we need this???
		divide_area_defaults();
		<<<<< */
		return FALSE;
		}
	if (same(mode, "clear"))
		{
		return FALSE;
		}

	/* Make sure the area set contains the right stuff */
	if (!EditAreas)                     return FALSE;
	if (!same(EditAreas->type, "area")) return FALSE;

	/* Initialize on startup */
	if (same(mode, "begin"))
		{
		ignore_partial();
		drawing_control(FALSE);
		edit_can_rejoin(FALSE);
		Dstart = FALSE;
		edit_select(NullCal, FALSE);
		State = DivPick;
		show_dvd_state("BEGIN", State);
		}

	/* Take care of setting new value */
	/* Only recognized when setting first or second area */
	if (same(mode, "set"))
		{

#		ifdef DEBUG
		if (NotNull(cal))
			pr_diag("Editor", "[edit_divide_area] Reset attributes\n");
		else
			pr_diag("Editor", "[edit_divide_area] Use Default attributes\n");
#		endif /* DEBUG */

		switch (State)
			{
			case DivSetA:	if (NotNull(cal))
								{
								CAL_destroy(CalA);
								CalA = CAL_duplicate(cal);
								}
							State = DivSetB;
							show_dvd_state("SET", State);
							break;

			case DivSetB:	if (NotNull(cal))
								{
								CAL_destroy(CalB);
								CalB = CAL_duplicate(cal);
								}
							State = DivDivide;
							show_dvd_state("SET", State);
							break;
			}
		}

	/* Set state for Right button "rejoin" */
	if ( same(mode, "rejoin") && !ReJoin )
		{
		put_message("area-div-join-not");
		(void) sleep(1);
		State = DivRePick;
		return FALSE;
		}
	if (same(mode, "rejoin")) State = DivJoin;

	/* Repeat until told to quit */
	while (TRUE)
		{
		show_dvd_state("==", State);

		/* Resume at current stage */
		switch (State)
			{

			/* Pick an area to divide */
			case DivPick:

				/* Initialize area values to background */
				ignore_partial();
				divide_area_defaults();

				/* Pick an area */
				divide_show(DvdShowNone);
				next = divide_area_pick();
				if (next == DivPick) return drawn;

				/* Move on to next stage */
				State = next;
				show_dvd_state("->", State);
				continue;

			/* Draw a dividing segment on the picked area */
			case DivDraw:

				/* Draw a dividing segment */
				divide_show(DvdShowPick);
				next = divide_area_draw();
				if (next == DivDraw) return drawn;

				/* Move on to next stage */
				State = next;
				show_dvd_state("->", State);
				continue;

			/* Set the value for the first area */
			case DivSetA:

				/* Set the value */
				post_partial(TRUE);
				edit_select((NotNull(DnewA))? (CAL) DnewA->attrib: NullCal,
																		TRUE);
				divide_show(DvdShowA);
				next = divide_area_set(State);
				if (next == DivSetA) return drawn;

				/* Move on to next stage */
				State = next;
				show_dvd_state("->", State);
				continue;

			/* Set the value for the second area */
			case DivSetB:

				/* Set the value */
				post_partial(TRUE);
				edit_select((NotNull(DnewB))? (CAL) DnewB->attrib: NullCal,
																		TRUE);
				divide_show(DvdShowB);
				next = divide_area_set(State);
				if (next == DivSetB) return drawn;

				/* Move on to next stage */
				State = next;
				show_dvd_state("->", State);
				continue;

			/*  Repick the area to divide */
			case DivRePick:

				put_message("edit-cancel");

				/* Clean up temporary buffers */
				ignore_partial();
				edit_select(NullCal, FALSE);
				DParea = NullArea;
				DPsub  = NullSubArea;
				DJsub  = NullSubArea;
				DWarea = NullArea;
				DWsub  = NullSubArea;
				DXsub  = NullSubArea;

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_divide_area] DivRePick ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();
				drawing_control(FALSE);
				edit_can_rejoin(FALSE);

				/* Move on to next stage */
				State = DivPick;
				show_dvd_state("->", State);
				/* >>>>> do we need to return FALSE??? <<<<< */
				continue;

			/*  Redraw the new segment */
			case DivReDraw:

				put_message("edit-cancel");

				/* Reset default area values */
				post_partial(TRUE);
				edit_select(NullCal, FALSE);
				divide_area_defaults();

				/* Clean up temporary buffers */
				DnewA = NullSubArea;
				DnewB = NullSubArea;
				Ddiv  = destroy_line(Ddiv);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_divide_area] DivReDraw ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();
				drawing_control(FALSE);
				edit_can_rejoin(FALSE);

				/* Replace area to divide with new copy of original */
				isub   = which_area_subarea(DParea, DPsub);
				DWarea = copy_area(DParea, TRUE);
				if (NotNull(DWarea->subareas) && isub >= 0)
					DWsub = DWarea->subareas[isub];
				if (ReJoin)
					{
					edit_can_rejoin(TRUE);
					isub = which_area_subarea(DParea, DJsub);
					if (NotNull(DWarea->subareas) && isub >= 0)
						{
						DXsub = DWarea->subareas[isub];
						}
					}

				add_item_to_metafile(TempMeta, "area", "c", "", "",
									 (ITEM) DWarea);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_divide_area] Adding DWarea to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = DivDraw;
				show_dvd_state("->", State);
				/* >>>>> do we need to return FALSE??? <<<<< */
				continue;

			/* Reset the first area value */
			case DivReset:

				put_message("edit-cancel");

				/* Reset default area values */
				post_partial(TRUE);
				divide_area_defaults();

				/* Move on to next stage */
				State = DivSetA;
				show_dvd_state("->", State);
				/* >>>>> do we need to return FALSE??? <<<<< */
				continue;

			/* Divide the area now */
			case DivDivide:

				/* Freeze all pending edits so only this one can be undone */
				if (EditUndoable)
					{
					ignore_partial();
					put_message("edit-freeze");
					accept_mod();	/* Leaves EditAreas alone */
					}
				post_partial(TRUE);

#				ifdef DEBUG
				pr_diag("Editor", "[edit_divide_area] Dividing\n");
#				endif /* DEBUG */

				/* Divide the area */
				busy_cursor(TRUE);
				put_message("area-dividing");
				divl = prepare_area_divline(DParea, DPsub, Ddiv, &dstat);
				if (IsNull(divl))
					{
					/* Bail out if unsuccessful */
					Ddiv  = destroy_line(Ddiv);
					State = DivBailout;
					show_dvd_state("->", State);
					continue;
					}
				if (!divide_area(DParea, DPsub, divl, &subA, &subB, &dstat))
					{
					/* Bail out if unsuccessful */
					divl  = destroy_line(divl);
					Ddiv  = destroy_line(Ddiv);
					State = DivBailout;
					show_dvd_state("->", State);
					continue;
					}

				/* Set values as entered */
				Ddiv = destroy_line(Ddiv);
				define_item_attribs("subarea", (ITEM) subA, CalA);
				define_item_attribs("subarea", (ITEM) subB, CalB);

				/* Reset default area attributes if dividing first subarea */
				isub = which_area_subarea(DParea, subA);
				if (isub == 0)
					{

#					ifdef DEBUG
					pr_diag("Editor",
						"[edit_divide_area] Reset default area attribs\n");
#					endif /* DEBUG */

					define_item_attribs("area", (ITEM) DParea, CalA);
					}

				/* Move on to next stage */
				State = DivConfirm;
				show_dvd_state("->", State);
				continue;

			/* Join the subareas now */
			case DivJoin:

				/* Freeze all pending edits so only this one can be undone */
				if (EditUndoable)
					{
					ignore_partial();
					put_message("edit-freeze");
					accept_mod();	/* Leaves EditAreas alone */
					}
				post_partial(TRUE);
				if (!undivide_area(DParea, DPsub, DJsub))
					{
					/* Bail out if unsuccessful */
					State = DivBailout;
					show_dvd_state("->", State);
					continue;
					}

#				ifdef DEBUG
				pr_diag("Editor", "[edit_divide_area] Un-dividing\n");
#				endif /* DEBUG */

				/* Modify the area */
				busy_cursor(TRUE);
				put_message("area-undividing");

				State = DivConfirm;
				show_dvd_state("->", State);
				continue;

			/* Bail out from current edit */
			case DivBailout:

				pr_warning("Editor",
					"[edit_divide_area] Unsuccessful - Bailing out\n");

				/* Register the edit */
				drawn = FALSE;
				ignore_partial();
				if (EditUndoable)
					{
					post_mod("areas");
					put_message("edit-bail-out");
					(void) sleep(1);
					reject_mod();	/* Must reset EditAreas */
					active_area_fields(TRUE, NewAreas, NewLabs);
					}

				/* Clean up temporary buffers */
				edit_select(NullCal, FALSE);
				DParea = NullArea;
				DPsub  = NullSubArea;
				DJsub  = NullSubArea;
				DWarea = NullArea;
				DWsub  = NullSubArea;
				DXsub  = NullSubArea;
				DnewA  = NullSubArea;
				DnewB  = NullSubArea;
				Ddiv   = destroy_line(Ddiv);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_divide_area] DivBailout ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();
				busy_cursor(FALSE);
				drawing_control(FALSE);
				edit_can_rejoin(FALSE);

				/* Go back to pick stage */
				State = DivPick;
				show_dvd_state("->", State);
				continue;

			/* Confirm current edit */
			case DivConfirm:

				/* Register the edit */
				ignore_partial();
				drawn = TRUE;
				invoke_set_catspecs(EditAreas);
				if (EditUndoable) highlight_set(EditAreas, 1);
				if (EditUndoable) post_mod("areas");

				/* Modify labels if necessary */
				if (recompute_areaset_labs(EditAreas, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				if (EditUndoable) (void) extract_area_order_tags(FALSE);

				/* Clean up temporary buffers */
				edit_select(NullCal, FALSE);
				DParea = NullArea;
				DPsub  = NullSubArea;
				DJsub  = NullSubArea;
				DWarea = NullArea;
				DWsub  = NullSubArea;
				DXsub  = NullSubArea;
				DnewA  = NullSubArea;
				DnewB  = NullSubArea;
				Ddiv   = destroy_line(Ddiv);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_divide_area] Done ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();
				drawing_control(FALSE);
				edit_can_rejoin(FALSE);
				busy_cursor(FALSE);

				/* Go back to pick stage */
				State = DivPick;
				show_dvd_state("->", State);
				continue;
			}
		}
	}

/**********************************************************************/

static	DVDSTATE	divide_area_pick(void)

	{
	POINT	pos;
	int		butt, isub;
	SUBAREA	lsub, rsub;

	/* See if there are any areas to divide */
	if (EditAreas->num <= 0)
		{
		put_message("area-no-div");
		return DivPick;
		}

	/* Keep trying until something gets picked */
	while (TRUE)
		{
		/* Get a point */
		put_message("area-div-pick");
		if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return DivPick;
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}
		put_message("area-pick");
		if (!pick_Xpoint(DnEdit, 0, pos, &butt)) continue;
		if (!inside_dn_window(DnEdit, pos))
			{
			put_message("edit-outside-map");
			(void) sleep(1);
			continue;
			}

		/* See which area we picked */
		DPsub = closest_subarea(EditAreas, pos, NullFloat, NullPoint,
								NullInt, NullInt, &DParea);
		if (!DPsub)
			{
			put_message("area-no-pick");
			(void) sleep(1);
			continue;
			}

		/* Produce a copy of the picked area to work on */
		put_message("area-picked");
		edit_select(NullCal, FALSE);
		isub   = which_area_subarea(DParea, DPsub);
		DWarea = copy_area(DParea, TRUE);
		if (NotNull(DWarea->subareas) && isub >= 0)
			DWsub = DWarea->subareas[isub];

		/* Set default area values */
		if (NotNull(DPsub->attrib))
			{
			CAL_destroy(CalA);	CalA = CAL_duplicate(DPsub->attrib);
			CAL_destroy(CalB);	CalB = CAL_duplicate(DPsub->attrib);
			}

		/* Is re-join an option? ... must have at least one dividing line! */
		ReJoin = (LOGICAL) (DWarea->numdiv > 0);

		/* Is re-join an option? ... must be adjacent to last dividing line */
		/*  ... to be consistent with function undivide_area() */
		if (ReJoin)
			{
			(void) adjacent_subareas(DParea, DParea->numdiv-1, &lsub, &rsub);
			if (lsub == DPsub)      DJsub  = rsub;
			else if (rsub == DPsub) DJsub  = lsub;
			else                    ReJoin = FALSE;
			}

		/* Set subarea to re-join in working copy */
		if (ReJoin)
			{
			edit_can_rejoin(TRUE);
			isub = which_area_subarea(DParea, DJsub);
			if (NotNull(DWarea->subareas) && isub >= 0)
				{
				DXsub = DWarea->subareas[isub];
				}
			}

		/* Highlight the picked subarea */
		add_item_to_metafile(TempMeta, "area", "c", "", "", (ITEM) DWarea);

#		ifdef DEBUG_STRUCTURES
		pr_diag("Editor",
			"[divide_area_pick] Adding DWarea to TempMeta (%d)\n",
			TempMeta->numfld);
#		endif /* DEBUG_STRUCTURES */

		return DivDraw;
		}
	}

/**********************************************************************/

static	DVDSTATE	divide_area_draw(void)

	{
	int		butt;
	POINT	spos, epos;
	int		nlines;
	LINE	*lines;
	int		ips, ipe, ipx;
	float	spfact;
	LINE	divln, cdiv, dcopy;
	DIVSTAT	dstat;
	STRING	msgkey;

	/* Make sure there is an area to divide */
	if (!DWsub)
		{
		put_message("area-none-picked");
		return DivDraw;
		}

	/* Repeat until we get a new segment or another choice */
	while (TRUE)
		{
		/* Draw a new segment */
		if (ReJoin) put_message("area-div-join");
		else        put_message("area-div-draw");
		/* >>>>> add this!!! <<<<< */
		post_partial(TRUE);
		/* >>>>> add this!!! <<<<< */
		if (ready_Xcurve()) butt = LeftButton;
		else if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return DivDraw;

		/* Invalid button */
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			reset_pipe();
			(void) ignore_Xpoint();
			continue;
			}

		/* Draw the curve */
		put_message("area-div-draw-rel");
		drawing_control(TRUE);
		if (!ready_Xcurve())
			{
			Dstart = TRUE;
			if (drawing_Xcurve()) return DivDraw;
			(void) draw_Xcurve(DnEdit, FilterRes, SplineRes, FALSE);
			if (!ready_Xcurve()) return DivDraw;
			}
		Dstart = FALSE;
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
		edit_can_rejoin(FALSE);

#		ifdef DEBUG
		pr_diag("Editor", "[divide_area_draw] Analysing dividing line\n");
#		endif /* DEBUG */

		/* A dividing line has been drawn */
		busy_cursor(TRUE);
		put_message("area-div-proc");

		/* Save a copy of the dividing line */
		divln = copy_line(lines[0]);
		reset_pipe();

		/* Set smoothing options */
		spfact = ModifySmth;
		spfact = MAX(spfact, 0.0);
		spfact = MIN(spfact, 100.0);

		/* Extract and trim the dividing line */
		Ddiv  = prepare_area_divline(DWarea, DWsub, divln, &dstat);
		divln = destroy_line(divln);
		if (!Ddiv)
			{
			switch (dstat)
				{
				case DivAreaRight:
				case DivAreaLeft:
					msgkey = "area-div-out1";	break;
				case DivTooShort:
					msgkey = "area-div-out2";	break;
				default:
					msgkey = "area-div-out3";	break;
				}
			put_message(msgkey);
			(void) sleep(1);
			busy_cursor(FALSE);
			return DivBailout;
			}

		/* Truncate the dividing line if it crosses itself */
		if ( looped_line_crossing(Ddiv, spos, &ips, &ipx) )
			{

			/* A dividing line that crosses itself has been found */
			pr_warning("Editor",
				"[divide_area_draw] Truncating dividing line cross over!\n");
			put_message("area-div-cross");
			(void) sleep(1);

			/* Find first cross over from the opposite end too */
			cdiv = copy_line(Ddiv);
			reverse_line(cdiv);
			if ( !looped_line_crossing(cdiv, epos, &ipe, NullInt) )
				{
				pr_warning("Editor",
					"[divide_area_draw] Problem with reverse cross over!\n");
				ipe = Ddiv->numpts - 2 - ipx;
				copy_point(epos, spos);
				}

#			ifdef DEBUG_EDIT
			pr_diag("Editor",
				"[divide_area_draw] Crossover at span %d of %d\n",
				ips, Ddiv->numpts-1);
			pr_diag("Editor",
				"[divide_area_draw] Reverse crossover at span %d of %d\n",
				ipe, cdiv->numpts-1);
#			endif /* DEBUG_EDIT */

			/* Truncate dividing line to location of cross over */
			/* Exclude cross over point and preceding point if  */
			/*  enough points found, to improve line smoothing  */
			if ( ips < 2 )
				{
				Ddiv->numpts = ips + 1;
				add_point_to_line(Ddiv, spos);
				}
			else if ( ips < 3 )
				Ddiv->numpts = ips + 1;
			else
				Ddiv->numpts = ips;

			/* Truncate dividing line from the opposite end too */
			/* Exclude cross over point and preceding point if  */
			/*  enough points found, to improve line smoothing  */
			if ( ipe < 2 )
				{
				cdiv->numpts = ipe + 1;
				add_point_to_line(cdiv, epos);
				}
			else if ( ipe < 3 )
				cdiv->numpts = ipe + 1;
			else
				cdiv->numpts = ipe;

			/* Join the two portions of the dividing line */
			reverse_line(cdiv);
			if ( spfact > 1.0 )
				smjoin_lines(Ddiv, cdiv, FilterRes/4, SplineRes);
			else
				append_line(Ddiv, cdiv);
			cdiv = destroy_line(cdiv);
			}

		/* Perform the divide on the working copy */
		/* Use a copy of the dividing line so it will stay around */
		widen_subarea(DWsub, -2.0);
		dcopy = copy_line(Ddiv);
		if (!divide_area(DWarea, DWsub, dcopy, &DnewA, &DnewB, &dstat))
			{
			put_message("area-div-out3");
			Ddiv  = destroy_line(Ddiv);
			dcopy = destroy_line(dcopy);
			(void) sleep(1);
			busy_cursor(FALSE);
			return DivBailout;
			}

		/* Original subarea is no more */
		DWsub = NullSubArea;

		/* Highlight the two new areas (pick the first one) */

		busy_cursor(FALSE);
		clear_message();
		return DivSetA;
		}
	}

/**********************************************************************/

static	DVDSTATE	divide_area_set

	(
	DVDSTATE	state
	)

	{
	int		butt;

	static	STRING	msgkey = NULL;
	static	STRING	msgval = NULL;

	/* Make sure there is something to set */
	switch (state)
		{
		case DivSetA:	break;
		case DivSetB:	break;
		default:		return state;
		}
	if (!DnewA || !DnewB)
		{
		put_message("area-no-set");
		return state;
		}

	/* Construct a prompt */
	switch (state)
		{
		case DivSetA:	msgkey = "area-div-set1";
						msgval = CAL_get_attribute(DnewA->attrib, CALuserlabel);
						break;

		case DivSetB:	msgkey = "area-div-set2";
						msgval = CAL_get_attribute(DnewB->attrib, CALuserlabel);
						break;

		default:		return state;
		}

	/* Repeat until we have a selection */
	while (TRUE)
		{

		/* Get a point */
		put_message(msgkey, msgval);
		if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return state;

		/* Wait for a "set" */
		(void) ignore_Xpoint();
		continue;
		}
	}

/**********************************************************************/

static	void	divide_area_defaults(void)

	{
	AREA	bgnd;

	/* Reset default area values */
	CAL_destroy(CalA);	CalA = NullCal;
	CAL_destroy(CalB);	CalB = NullCal;
	if (DPsub && NotNull(DPsub->attrib))
		{
		CalA = CAL_duplicate(DPsub->attrib);
		CalB = CAL_duplicate(DPsub->attrib);
		}
	else if (EditAreas->bgnd && NotNull(((AREA) EditAreas->bgnd)->attrib))
		{
		bgnd = (AREA) EditAreas->bgnd;
		CalA = CAL_duplicate(bgnd->attrib);
		CalB = CAL_duplicate(bgnd->attrib);
		}
	else
		{
		(void) printf("...No Background\n");
		}
	}

/**********************************************************************/

static	void	divide_show

	(
	DVDSHOW	show
	)

	{
	static	DVDSHOW	Prev = DvdShowNone;

	if (show == Prev) return;

	/* Un-show what was already shown */
	switch (Prev)
		{
		case DvdShowPick:

#					ifdef DEBUG
					pr_diag("Editor", "[divide_show] Erase DvdShowPick\n");
#					endif /* DEBUG */

					highlight_area(DWarea, -1, -1);
					if (NotNull(DWsub))
						{
						widen_subarea(DWsub, -2.0);
						highlight_subarea(DWsub, -1, -1);
						}
					if (NotNull(DXsub))
						{
						highlight_subarea(DXsub, -1, -1);
						}
					break;

		case DvdShowA:

#					ifdef DEBUG
					pr_diag("Editor", "[divide_show] Erase DvdShowA\n");
#					endif /* DEBUG */

					if (IsNull(DnewA)) break;
					widen_subarea(DnewA, -2.0);
					highlight_subarea(DnewA, 2, -1);
					break;

		case DvdShowB:

#					ifdef DEBUG
					pr_diag("Editor", "[divide_show] Erase DvdShowB\n");
#					endif /* DEBUG */

					if (IsNull(DnewB)) break;
					widen_subarea(DnewB, -2.0);
					highlight_subarea(DnewB, 2, -1);
					break;

		case DvdShowNone:
		default:	break;
		}

	/* Show what is being requested */
	switch (show)
		{
		case DvdShowPick:

#					ifdef DEBUG
					pr_diag("Editor", "[divide_show] Show DvdShowPick\n");
#					endif /* DEBUG */

					highlight_area(DWarea, -1, -1);
					if (NotNull(DWsub))
						{
						widen_subarea(DWsub, 2.0);
						highlight_subarea(DWsub, 2, 50);
						}
					if (NotNull(DXsub))
						{
						highlight_subarea(DXsub, 4, 0);
						}
					break;

		case DvdShowA:

#					ifdef DEBUG
					pr_diag("Editor", "[divide_show] Show DvdShowA\n");
#					endif /* DEBUG */

					highlight_area(DWarea, -1, -1);
					if (IsNull(DnewA)) break;
					widen_subarea(DnewA, 2.0);
					highlight_subarea(DnewA, 3, 1);
					break;

		case DvdShowB:

#					ifdef DEBUG
					pr_diag("Editor", "[divide_show] Show DvdShowB\n");
#					endif /* DEBUG */

					highlight_area(DWarea, -1, -1);
					if (IsNull(DnewB)) break;
					widen_subarea(DnewB, 2.0);
					highlight_subarea(DnewB, 3, 1);
					break;

		case DvdShowNone:
		default:	break;
		}

	present_all();
	Prev = show;
	}

/***********************************************************************
*                                                                      *
*     m e r g e _ a r e a                                              *
*                                                                      *
***********************************************************************/

LOGICAL	edit_merge_area

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
	AREA	area, copy;
	SPOT	spot;
	SET		set, tset;
	CURVE	outline;
	int		nlines;
	LINE	*lines;
	int		im, imx, imp, ims, ime, il, ilx, dim, ii;
	LOGICAL	top;
	POINT	p0, p1, pz;
	MARK	mark0, mark1;
	CURVE	curv0, curv1;
	float	dx, dy, dx0, dy0, angle, cang, sang;
	int		xnum;
	AREA	*xareas;
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
	/* - list of areas and area labels to be moved */
	/* - cmove/smove are pointers into merge field sets (before merge) */
	/*    and then copies to be displayed in TempMeta (after merge)    */
	/* - ccopy/scopy are copies to be added to EditAreas/EditLabs */
	static	const int	maxmove = 10000;
	static	int			nmove   = 0;
	static	int			amove   = 0;
	static	AREA		*cmove  = NullAreaList;
	static	AREA		*ccopy  = NullAreaList;
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
	pr_diag("Editor", "[edit_merge_area] %s  State: %d\n", mode, State);
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

							/* Empty picked area list */
							if (nmove > 0)
								{
								if (mstart && !maction)
									{
									for (im=0; im<nmove; im++)
										(void) remove_item_from_set(EditAreas,
															 (ITEM) ccopy[im]);
									}
								else if (!maction)
									{
									for (im=0; im<nmove; im++)
										(void) destroy_area(ccopy[im]);
									}
								nmove = 0;
								amove = 0;
								FREEMEM(cmove);
								FREEMEM(ccopy);
								}
							edit_select(NullCal, FALSE);

							/* Empty area label list */
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

							/* Get rid of picked areas and fetched field */
							if (fullset)
								{
								mset = take_mf_set(TempMeta, "area", "b",
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
								"[edit_merge_area] Cancel ... empty TempMeta %d fields\n",
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

		/* Empty picked area list and area label list */
		if (nmove > 0)
			{
			if (mstart && !maction)
				{
				for (im=0; im<nmove; im++)
					(void) remove_item_from_set(EditAreas, (ITEM) ccopy[im]);
				}
			else if (!maction)
				{
				for (im=0; im<nmove; im++) (void) destroy_area(ccopy[im]);
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

		/* Get rid of displayed picked areas and fetched field */
		if (fullset)
			{
			mset = take_mf_set(TempMeta, "area", "b", "mset", "mset");
			sset = take_mf_set(TempMeta, "spot", "d", "sset", "sset");
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
			"[edit_merge_area] Cancel All ... empty TempMeta %d fields\n",
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

	/* Make sure the area set contains the right stuff */
	if (!EditAreas)                     return FALSE;
	if (!same(EditAreas->type, "area")) return FALSE;

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

		/* Get rid of displayed picked areas and fetched field */
		if (fullset)
			{
			mset = take_mf_set(TempMeta, "area", "b", "mset", "mset");
			sset = take_mf_set(TempMeta, "spot", "d", "sset", "sset");
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
			"[edit_merge_area] Begin ... empty TempMeta %d fields\n",
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
				put_message("area-merge-nomode");
				(void) sleep(1);
				}
			mlfirst = FALSE;
			}

		/* Change merge mode and remove labels */
		else if (mlabels)
			{

			/* Un-highlight each area label and remove it from the list */
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

			/* Re-display the picked areas */
			present_all();

			/* Reset check for merge mode */
			mlabels = FALSE;
			mlfirst = TRUE;
			}

		/* Change merge mode and add labels */
		else
			{

			/* Match current list of area labels to   */
			/*  picked areas and add them to the list */
			set  = find_mf_set(TempMeta, "area", "b", "mset", "mset");
			tset = find_mf_set(TempMeta, "spot",  "d", "sset", "sset");

			/* Loop through all labels in set */
			if (nmove > 0 && NotNull(tset))
				{
				for (ilx=0; ilx<tset->num; ilx++)
					{
					spot = (SPOT) tset->list[ilx];
					if (!spot) continue;
					if (same(spot->mclass, "legend")) continue;

					/* Check if label is inside a picked area */
					xnum = enclosing_area_list(set, spot->anchor,
											   PickFirst, &xareas);
					for (ii=0; ii<xnum; ii++)
						{
						for (im=0; im<nmove; im++)
							{
							if (xareas[ii] == cmove[im]) break;
							}
						if (im < nmove)
							{

							/* Highlight each area label */
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
							break;
							}
						}
					}
				}

			/* Re-display the picked areas and area labels */
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
		put_message("area-no-pick");
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
					"[edit_merge_area] No outline drawn yet!\n");
				}
			else
				{
				put_message("edit-no-preset-outline", source);
				pr_error("Editor",
					"[edit_merge_area] No preset map outline: \"%s\"\n",
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
				"[edit_merge_area] Preset Outline ... adding outline to TempMeta (%d)\n",
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
				put_message("area-merge-action");
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
				pr_diag("Editor", "[edit_merge_area] Fetching merge field\n");
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
				mset = retrieve_areaset(&fdesc);
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
					/* No areas to merge */
					put_message("area-merge-none");
					(void) sleep(2);
					State = Wait;
					continue;
					}

				/* Got it - prepare set of areas for display */
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

				add_set_to_metafile(TempMeta, "b", "mset", "mset", mset);
				add_set_to_metafile(TempMeta, "d", "sset", "sset", sset);
				present_temp(TRUE);
				post_partial(TRUE);
				clear_message();

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_merge_area] Fetch ... adding mset/sset to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Re-fetch the requested merge field */
			case ReFetch:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_area] ReFetch\n");
#				endif /* DEBUG */

				ignore_partial();

				/* Empty picked area list */
				if (nmove > 0)
					{
					for (im=0; im<nmove; im++) (void) destroy_area(ccopy[im]);
					nmove = 0;
					amove = 0;
					FREEMEM(cmove);
					FREEMEM(ccopy);
					}
				edit_select(NullCal, FALSE);

				/* Empty area label list */
				if (nlabs > 0)
					{
					for (il=0; il<nlabs; il++) (void) destroy_spot(scopy[il]);
					nlabs = 0;
					alabs = 0;
					FREEMEM(smove);
					FREEMEM(scopy);
					FREEMEM(sids);
					}

				/* Get rid of displayed picked areas and fetched field */
				if (fullset)
					{
					mset = take_mf_set(TempMeta, "area", "b", "mset", "mset");
					sset = take_mf_set(TempMeta, "spot", "d", "sset", "sset");
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
					"[edit_merge_area] ReFetch ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();
				present_all();

				/* Wait for a "fetch" */
				State = (fetch)? Fetch: Wait;
				continue;

			/* Pick areas to merge */
			case Pick:

				/* Use the current list of areas to pick from */
				set = find_mf_set(TempMeta, "area", "b", "mset", "mset");

				/* Allow specification of multiple moves */
				while (nmove < maxmove)
					{

					/* Put up a prompt and set states */
					if (nmove <= 0)
						{
						put_message("area-merge-pick");
						edit_select(NullCal, FALSE);
						}
					else
						{
						put_message("area-merge-pick2");
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
					put_message("area-pick");
					if (!pick_Xpoint(DnEdit, 0, p1, &butt)) continue;
					if (!inside_dn_window(DnEdit, p1))
						{
						put_message("edit-outside-map");
						(void) sleep(1);
						continue;
						}

					/* See which area we picked */
					area = closest_area(set, p1, NULL, NULL, NULL, &imp, NULL);
					if (!area)
						{
						put_message("area-no-pick");
						(void) sleep(1);
						continue;
						}

					/* If area was already picked, un-pick it */
					for (im=0; im<nmove; im++)
						{
						if (area == cmove[im]) break;
						}
					if (im < nmove)
						{
						put_message("area-unpicked");

						/* Un-highlight the picked area */
						/*  and remove it from the list */
						highlight_area(area, 4, 0);
						widen_area(area, -2.0);
						(void) destroy_area(ccopy[im]);
						nmove--;
						for (imx=im; imx<nmove; imx++)
							{
							cmove[imx] = cmove[imx+1];
							ccopy[imx] = ccopy[imx+1];
							}
						cmove[imx] = NullArea;
						ccopy[imx] = NullArea;

						/* Remove labels inside this area */
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

									/* Is label inside another picked area? */
									xnum = enclosing_area_list(set,
											smove[il]->anchor, PickFirst,
											&xareas);
									for (ii=0; ii<xnum; ii++)
										{
										for (imx=0; imx<nmove; imx++)
											{
											if (xareas[ii] == cmove[imx]) break;
											}
										/* Found another picked area! */
										if (imx < nmove)
											{
											sids[il] = imx;
											break;
											}
										}

									/* Label not inside another picked area */
									/*  ... so un-highlight the area label  */
									/*       and remove it from the list    */
									if (ii >= xnum)
										{
										highlight_item("spot",
													   (ITEM) smove[il], 4);
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
							}

						/* Display remaining picked areas and area labels */
						present_all();
						}

					/* Otherwise pick it */
					else
						{
						put_message("area-picked");
						nmove++;
						if (nmove > amove)
							{
							amove = nmove;
							cmove = GETMEM(cmove, AREA, amove);
							ccopy = GETMEM(ccopy, AREA, amove);
							}

						/* Determine stacking order to insert item into set */
						for (im=0; im<nmove-1; im++)
							{
							if (imp < which_set_item(set, (ITEM) cmove[im]) )
								{
								/* Shuffle other items to make space */
								for (ii=nmove-1; ii>im; ii--)
									{
									cmove[ii] = cmove[ii-1];
									ccopy[ii] = ccopy[ii-1];
									}
								/* Reset label identifiers as well */
								for (il=0; il<nlabs; il++)
									{
									if (sids[il] >= im) sids[il]++;
									}
								break;
								}
							}

						/* Highlight the picked area */
						/*  and add it to the list */
						ccopy[im] = copy_area(area, TRUE);
						highlight_area(area, 2, 0);
						widen_area(area, 2.0);
						cmove[im] = area;

						/* Check for labels inside picked area */
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

									/* Is label already in list? */
									for (il=0; il<nlabs; il++)
										{
										if (smove[il] == spot) break;
										}
									if (il < nlabs) continue;

									/* Is label inside picked area? */
									xnum = enclosing_area_list(set,
											spot->anchor, PickFirst, &xareas);
									for (ii=0; ii<xnum; ii++)
										{
										if (xareas[ii] == area) break;
										}
									if (ii < xnum)
										{

										/* Highlight the area label */
										/*  and add it to the list  */
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

						/* Display picked areas and area labels */
						present_temp(TRUE);
						}
					}

				pr_warning("Editor",
					"[edit_merge_area]   End of State = Pick loop!\n");

				/* Have we picked any areas? */
				if (nmove > 0) edit_select((CAL) cmove[0]->attrib, FALSE);
				else           edit_select(NullCal, FALSE);

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Pick all areas to merge */
			case PickAll:

				put_message("area-all-picked");

				/* Un-highlight picked areas and remove them from the list */
				for (im=0; im<nmove; im++)
					{
					highlight_area(cmove[im], 4, 0);
					widen_area(cmove[im], -2.0);
					(void) destroy_area(ccopy[im]);
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

				/* Highlight all the areas and add them to the list */
				set = find_mf_set(TempMeta, "area", "b", "mset", "mset");
				nmove = set->num;
				if (nmove > amove)
					{
					amove = nmove;
					cmove = GETMEM(cmove, AREA, amove);
					ccopy = GETMEM(ccopy, AREA, amove);
					}
				for (im=0; im<nmove; im++)
					{
					area      = (AREA) set->list[im];
					ccopy[im] = copy_area(area, TRUE);
					highlight_area(area, 2, 0);
					widen_area(area, 2.0);
					cmove[im] = area;
					}

				/* Find labels inside picked areas and add them to the list */
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

							/* Check if label is inside a picked area */
							xnum = enclosing_area_list(set, spot->anchor,
													   PickFirst, &xareas);
							for (ii=0; ii<xnum; ii++)
								{
								for (im=0; im<nmove; im++)
									{
									if (xareas[ii] == cmove[im]) break;
									}
								if (im < nmove)
									{

									/* Highlight each area label */
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
									break;
									}
								}
							}
						}
					}

				/* Display picked areas and area labels */
				present_temp(TRUE);

				/* Have we picked any areas? */
				if (nmove > 0) edit_select((CAL) cmove[0]->attrib, FALSE);
				else           edit_select(NullCal, FALSE);

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Draw outline around areas to merge */
			case Draw:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_area] Drawing outline\n");
#				endif /* DEBUG */

				/* Draw the outline */
				post_partial(FALSE);
				edit_allow_preset_outline(FALSE);
				put_message("area-merge-draw");
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
					"[edit_merge_area] Draw Outline ... adding outline to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = DrawPick;
				continue;

			/* Pick areas to merge from inside drawn outline */
			case DrawPick:

				if (IsNull(outline))
					{
					edit_select(NullCal, FALSE);
					State = Pick;
					continue;
					}

#				ifdef DEBUG
				pr_diag("Editor",
					"[edit_merge_area] Picking areas inside outline\n");
#				endif /* DEBUG */

				/* Pick areas inside drawn outline */
				busy_cursor(TRUE);
				edit_allow_preset_outline(FALSE);
				put_message("area-merge-select");
				(void) sleep(1);

				/* Use the current list of areas to pick from */
				set = find_mf_set(TempMeta, "area", "b", "mset", "mset");

				/* Loop through all areas in set */
				for (imx=0; imx<set->num; imx++)
					{
					area = (AREA) set->list[imx];
					if (!area) continue;

					/* Check if area is inside drawn outline */
					if (!area_inside_outline(area, outline)) continue;

					/* Pick areas that are not already picked */
					for (im=0; im<nmove; im++)
						{
						if (area == cmove[im]) break;
						}
					if (im >= nmove)
						{
						put_message("area-picked");
						nmove++;
						if (nmove > amove)
							{
							amove = nmove;
							cmove = GETMEM(cmove, AREA, amove);
							ccopy = GETMEM(ccopy, AREA, amove);
							}

						/* Determine stacking order to insert item into set */
						for (im=0; im<nmove-1; im++)
							{
							if (imx < which_set_item(set, (ITEM) cmove[im]) )
								{
								/* Shuffle other items to make space */
								for (ii=nmove-1; ii>im; ii--)
									{
									cmove[ii] = cmove[ii-1];
									ccopy[ii] = ccopy[ii-1];
									}
								/* Reset label identifiers as well */
								for (il=0; il<nlabs; il++)
									{
									if (sids[il] >= im) sids[il]++;
									}
								break;
								}
							}

						/* Highlight the picked area */
						/*  and add it to the list */
						ccopy[im] = copy_area(area, TRUE);
						highlight_area(area, 2, 0);
						widen_area(area, 2.0);
						cmove[im] = area;

						/* Check for labels inside picked area */
						if (mlabels)
							{
							tset = find_mf_set(TempMeta, "spot",
											   "d", "sset", "sset");
							if (NotNull(tset))
								{
								for (ilx=0; ilx<tset->num; ilx++)
									{
									spot = (SPOT) tset->list[ilx];
									if (!spot) continue;
									if (same(spot->mclass, "legend")) continue;

									/* Is label already in list? */
									for (il=0; il<nlabs; il++)
										{
										if (smove[il] == spot) break;
										}
									if (il < nlabs) continue;

									/* Is label inside picked area? */
									xnum = enclosing_area_list(set,
											spot->anchor, PickFirst, &xareas);
									for (ii=0; ii<xnum; ii++)
										{
										if (xareas[ii] == area) break;
										}
									if (ii < xnum)
										{

										/* Highlight the area label */
										/*  and add it to the list  */
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

				/* Display picked areas and area labels */
				present_temp(TRUE);
				busy_cursor(FALSE);

				/* Have we picked any areas? */
				if (nmove > 0) edit_select((CAL) cmove[0]->attrib, FALSE);
				else           edit_select(NullCal, FALSE);

				/* Re-display the picked areas */
				State = ReDisplay;
				continue;

			/* Re-pick areas to merge */
			case RePick:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_area] RePick\n");
#				endif /* DEBUG */

				/* Empty picked area list */
				if (nmove > 0)
					{
					/* First need to un-widen the picked areas */
					/*  and remove them from the list */
					for (im=0; im<nmove; im++)
						{
						area = cmove[im];
						widen_area(area, -2.0);
						(void) destroy_area(ccopy[im]);
						}
					nmove = 0;
					amove = 0;
					FREEMEM(cmove);
					FREEMEM(ccopy);
					}
				edit_select(NullCal, FALSE);

				/* Empty area label list */
				if (nlabs > 0)
					{
					for (il=0; il<nlabs; il++) (void) destroy_spot(scopy[il]);
					nlabs = 0;
					alabs = 0;
					FREEMEM(smove);
					FREEMEM(scopy);
					FREEMEM(sids);
					}

				/* Remove displayed picked areas, but keep the fetched field */
				if (fullset)
					{
					mset = take_mf_set(TempMeta, "area", "b", "mset", "mset");
					sset = take_mf_set(TempMeta, "spot", "d", "sset", "sset");
					}

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_merge_area] RePick ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();

				/* Re-display the fetched field */
				fullset = TRUE;
				highlight_set(mset, 4);
				highlight_set(sset, 4);
				add_set_to_metafile(TempMeta, "b", "mset", "mset", mset);
				add_set_to_metafile(TempMeta, "d", "sset", "sset", sset);
				present_all();
				post_partial(TRUE);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_merge_area] RePick ... adding mset/sset to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Merge areas in place */
			case Merge:

				/* Remove displayed fetched field (if required) */
				mstart = TRUE;
				if (fullset)
					{

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_merge_area] Merge ... empty TempMeta %d fields\n",
						TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */

					/* Empty display structure */
					empty_temp();
					fullset = FALSE;
					mset    = NullSet;
					sset    = NullSet;

					/* Determine order for adding based on stacking order */
					switch (StackMode)
						{
						case STACK_BOTTOM:
								ims = 0;
								ime = nmove-1;
								dim = +1;
								top = FALSE;
								break;
						case STACK_TOP:
								ims = nmove-1;
								ime = 0;
								dim = -1;
								top = TRUE;
								break;
						}

					/* Now add the picked areas */
					for (im=ims; ; im+=dim)
						{
						if ( top && im<ime) break;
						if (!top && im>ime) break;

						switch (StackMode)
							{
							case STACK_BOTTOM:
									(void) add_item_to_set(EditAreas,
															(ITEM) ccopy[im]);
									break;
							case STACK_TOP:
									(void) add_item_to_set_start(EditAreas,
															(ITEM) ccopy[im]);
									(void) adjust_area_link_nodes(ActiveDfld,
															EditTime, -1, 0);
									break;
							}
						area = copy_area(ccopy[im], TRUE);
						add_item_to_metafile(TempMeta, "area", "b",
											 "mmerge", "mmerge", (ITEM) area);
						cmove[im] = area;

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[edit_merge_area] Adding area (%d) to TempMeta (%d)\n",
							im, TempMeta->numfld);
#						endif /* DEBUG_STRUCTURES */
						}

					/* Then add the area labels (if requested) */
					if (mlabels)
						{
						for (il=0; il<nlabs; il++)
							{
							(void) add_item_to_set(EditLabs, (ITEM) scopy[il]);
							spot = copy_spot(scopy[il]);
							add_item_to_metafile(TempMeta, "spot", "d",
												 "smerge", "smerge",
												 (ITEM) spot);
							smove[il] = spot;

#							ifdef DEBUG_STRUCTURES
							pr_diag("Editor",
								"[edit_merge_area] Adding spot (%d) to TempMeta (%d)\n",
								il, TempMeta->numfld);
#							endif /* DEBUG_STRUCTURES */
							}
						}
					}

				/* Remove any marks from uncompleted actions */
				set = take_mf_set(TempMeta, "mark", "d", "", "");
				if (NotNull(set)) set = destroy_set(set);

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_area] Merging\n");
#				endif /* DEBUG */

				ignore_partial();

				if (EditUndoable) post_mod("areas");

				/* Modify labels if necessary */
				if (recompute_areaset_labs(EditAreas, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				if (EditUndoable) (void) extract_area_order_tags(FALSE);

				put_message("edit-done");
				mstart  = FALSE;
				maction = TRUE;

				/* Re-display the picked areas and labels */
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
						"[edit_merge_area] Translate ... empty TempMeta %d fields\n",
						TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */

					/* Empty display structure */
					empty_temp();
					fullset = FALSE;
					mset    = NullSet;
					sset    = NullSet;

					/* Determine order for adding based on stacking order */
					switch (StackMode)
						{
						case STACK_BOTTOM:
								ims = 0;
								ime = nmove-1;
								dim = +1;
								top = FALSE;
								break;
						case STACK_TOP:
								ims = nmove-1;
								ime = 0;
								dim = -1;
								top = TRUE;
								break;
						}

					/* Now add the picked areas */
					for (im=ims; ; im+=dim)
						{
						if ( top && im<ime) break;
						if (!top && im>ime) break;

						switch (StackMode)
							{
							case STACK_BOTTOM:
									(void) add_item_to_set(EditAreas,
															(ITEM) ccopy[im]);
									break;
							case STACK_TOP:
									(void) add_item_to_set_start(EditAreas,
															(ITEM) ccopy[im]);
									(void) adjust_area_link_nodes(ActiveDfld,
															EditTime, -1, 0);
									break;
							}
						area = copy_area(ccopy[im], TRUE);
						add_item_to_metafile(TempMeta, "area", "b",
											 "mmerge", "mmerge", (ITEM) area);
						cmove[im] = area;

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[edit_merge_area] Adding area (%d) to TempMeta (%d)\n",
							im, TempMeta->numfld);
#						endif /* DEBUG_STRUCTURES */
						}

					/* Then add the area labels (if requested) */
					if (mlabels)
						{
						for (il=0; il<nlabs; il++)
							{
							(void) add_item_to_set(EditLabs, (ITEM) scopy[il]);
							spot = copy_spot(scopy[il]);
							add_item_to_metafile(TempMeta, "spot", "d",
												 "smerge", "smerge",
												 (ITEM) spot);
							smove[il] = spot;

#							ifdef DEBUG_STRUCTURES
							pr_diag("Editor",
								"[edit_merge_area] Adding spot (%d) to TempMeta (%d)\n",
								il, TempMeta->numfld);
#							endif /* DEBUG_STRUCTURES */
							}
						}
					}

				/* Highlight the picked areas and labels */
				for (im=0; im<nmove; im++)
					{
					area = cmove[im];
					highlight_area(area, 3, 0);
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

				/* Re-display the picked areas and labels */
				present_all();
				post_partial(TRUE);

				/* Pick a reference point */
				put_message("area-tran");
				if (!ready_Xpoint(DnEdit, p0, &butt)) return drawn;
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!inside_dn_window(DnEdit, p0))
					{
					put_message("area-tran-out1");
					(void) ignore_Xpoint();
					continue;
					}

				/* Highlight the areas (without holes) to translate */
				set = copy_mf_set(TempMeta, "area", "b", "mmerge", "mmerge");
				for (im=0; im<set->num; im++)
					{
					area = (AREA) set->list[im];
					remove_all_area_holes(area);
					highlight_area(area, 2, -1);
					widen_area(area, 2.0);
					}
				prep_set(set, mformat);

				/* Translate the reference point */
				put_message("area-tran-release");
				(void) utrack_Xpoint(DnEdit, set, p0, p1, &butt);
				set = destroy_set(set);
				if (!inside_dn_window(DnEdit, p1))
					{
					put_message("area-tran-out2");
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
					"[edit_merge_area] Adding mark/mark/curve to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = TranslateDone;
				continue;

			/* Translate the areas */
			case TranslateDone:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_area] Translating\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);

				/* Translate the areas */
				dx = p1[X] - p0[X];
				dy = p1[Y] - p0[Y];
				for (im=0; im<nmove; im++)
					{
					area = ccopy[im];
					translate_area(area, dx, dy);
					}

				/* Then translate the area labels (if requested) */
				if (mlabels)
					{
					(void) spot_list_translate(nlabs, scopy, dx, dy);
					}

				if (EditUndoable) post_mod("areas");

				/* Modify labels if necessary */
				if (recompute_areaset_labs(EditAreas, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				if (EditUndoable) (void) extract_area_order_tags(FALSE);

				put_message("edit-done");
				mstart  = FALSE;
				maction = TRUE;
				busy_cursor(FALSE);

				/* Re-display the picked areas and labels */
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
						"[edit_merge_area] Rotate ... empty TempMeta %d fields\n",
						TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */

					/* Empty display structure */
					empty_temp();
					fullset = FALSE;
					mset    = NullSet;
					sset    = NullSet;

					/* Determine order for adding based on stacking order */
					switch (StackMode)
						{
						case STACK_BOTTOM:
								ims = 0;
								ime = nmove-1;
								dim = +1;
								top = FALSE;
								break;
						case STACK_TOP:
								ims = nmove-1;
								ime = 0;
								dim = -1;
								top = TRUE;
								break;
						}

					/* Now add the picked areas */
					for (im=ims; ; im+=dim)
						{
						if ( top && im<ime) break;
						if (!top && im>ime) break;

						switch (StackMode)
							{
							case STACK_BOTTOM:
									(void) add_item_to_set(EditAreas,
															(ITEM) ccopy[im]);
									break;
							case STACK_TOP:
									(void) add_item_to_set_start(EditAreas,
															(ITEM) ccopy[im]);
									(void) adjust_area_link_nodes(ActiveDfld,
															EditTime, -1, 0);
									break;
							}
						area = copy_area(ccopy[im], TRUE);
						add_item_to_metafile(TempMeta, "area", "b",
											 "mmerge", "mmerge", (ITEM) area);
						cmove[im] = area;

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[edit_merge_area] Adding area (%d) to TempMeta (%d)\n",
							im, TempMeta->numfld);
#						endif /* DEBUG_STRUCTURES */
						}

					/* Then add the area labels (if requested) */
					if (mlabels)
						{
						for (il=0; il<nlabs; il++)
							{
							(void) add_item_to_set(EditLabs, (ITEM) scopy[il]);
							spot = copy_spot(scopy[il]);
							add_item_to_metafile(TempMeta, "spot", "d",
												 "smerge", "smerge",
												 (ITEM) spot);
							smove[il] = spot;

#							ifdef DEBUG_STRUCTURES
							pr_diag("Editor",
								"[edit_merge_area] Adding spot (%d) to TempMeta (%d)\n",
								il, TempMeta->numfld);
#							endif /* DEBUG_STRUCTURES */
							}
						}
					}

				/* Highlight the picked areas and labels */
				for (im=0; im<nmove; im++)
					{
					area = cmove[im];
					highlight_area(area, 3, 0);
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

				/* Re-display the areas */
				present_all();
				post_partial(TRUE);

				/* Pick the centre of rotation */
				put_message("area-rot-centre");
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
					put_message("area-rot-out");
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
					"[edit_merge_area] Adding mark to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = Rotate;
				continue;

			/* Pick and rotate a reference point */
			case Rotate:

				/* Pick a reference point */
				put_message("area-rot");
				if (!ready_Xpoint(DnEdit, p0, &butt)) return drawn;
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!inside_dn_window(DnEdit, p0))
					{
					put_message("area-rot-out1");
					(void) ignore_Xpoint();
					continue;
					}
				if (point_dist(Cpos, p0) < SplineRes)
					{
					put_message("area-rot-ref");
					(void) ignore_Xpoint();
					(void) sleep(1);
					continue;
					}

				/* Highlight the areas (without holes) to rotate */
				set = copy_mf_set(TempMeta, "area", "b", "mmerge", "mmerge");
				for (im=0; im<set->num; im++)
					{
					area = (AREA) set->list[im];
					remove_all_area_holes(area);
					highlight_area(area, 2, -1);
					widen_area(area, 2.0);
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
					"[edit_merge_area] Adding mark/curve to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Rotate the reference point */
				put_message("area-rot-release");
				(void) urotate_Xpoint(DnEdit, set, Cpos, p0, p1, &Ang, &butt);
				set = destroy_set(set);
				if (!inside_dn_window(DnEdit, p1))
					{
					put_message("area-rot-out2");
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
					"[edit_merge_area] Adding mark/curve to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = RotateDone;
				continue;

			/* Rotate the areas */
			case RotateDone:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_area] Rotating\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);

				/* Rotate the areas */
				dx0 = p0[X] - Cpos[X];
				dy0 = p0[Y] - Cpos[Y];
				for (im=0; im<nmove; im++)
					{
					area = ccopy[im];
					rotate_area(area, Cpos, Ang/RAD);
					}

				/* Then rotate the area labels (if requested) */
				if (mlabels)
					{
					(void) spot_list_rotate(nlabs, scopy, Cpos, Ang/RAD);
					}

				if (EditUndoable) post_mod("areas");

				/* Modify labels if necessary */
				if (recompute_areaset_labs(EditAreas, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				if (EditUndoable) (void) extract_area_order_tags(FALSE);

				put_message("edit-done");
				mstart  = FALSE;
				maction = TRUE;
				busy_cursor(FALSE);

				/* Re-display the picked areas and labels */
				State = ReDisplayAction;
				continue;

			/* Re-display the picked areas */
			case ReDisplay:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_area] Redisplay\n");
#				endif /* DEBUG */

				/* Keep the fetched field (including picked areas) */
				if (fullset)
					{
					mset = take_mf_set(TempMeta, "area", "b", "mset", "mset");
					sset = take_mf_set(TempMeta, "spot", "d", "sset", "sset");
					}

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_merge_area] ReDisplay ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();

				/* Re-display the fetched field (including picked areas) */
				if (NotNull(mset))
					{
					add_set_to_metafile(TempMeta, "b", "mset", "mset", mset);
					add_set_to_metafile(TempMeta, "d", "sset", "sset", sset);
					}
				present_all();
				post_partial(TRUE);

#				ifdef DEBUG_STRUCTURES
				if (NotNull(mset))
					pr_diag("Editor",
						"[edit_merge_area] ReDisplay ... adding mset/sset to TempMeta (%d)\n",
						TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Re-display the picked areas (after merging) */
			case ReDisplayAction:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_area] ReDisplayAction\n");
#				endif /* DEBUG */

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_merge_area] ReDisplayAction ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();

				/* Re-pick the picked areas */
				for (im=0; im<nmove; im++)
					{

					/* Highlight the picked area */
					copy = copy_area(ccopy[im], TRUE);
					highlight_area(copy, 2, 0);
					widen_area(copy, 2.0);

					add_item_to_metafile(TempMeta, "area", "b",
										 "mmerge", "mmerge", (ITEM) copy);
					cmove[im] = copy;

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_merge_area] Adding area (%d) to TempMeta (%d)\n",
						im, TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */
					}

				/* Then re-pick the area labels (if requested) */
				if (mlabels)
					{
					for (il=0; il<nlabs; il++)
						{

						/* Highlight the area label */
						spot = copy_spot(scopy[il]);
						highlight_item("spot", (ITEM) spot, 2);
						add_item_to_metafile(TempMeta, "spot", "d",
											 "smerge", "smerge", (ITEM) spot);
						smove[il] = spot;

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[edit_merge_area] Adding spot (%d) to TempMeta (%d)\n",
							il, TempMeta->numfld);
#						endif /* DEBUG_STRUCTURES */
						}
					}

				/* Re-display the picked areas and labels */
				present_all();
				post_partial(FALSE);

				/* Move on to next stage */
				State = WaitAction;
				continue;

			/* Start again */
			case ReStart:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_area] ReStart\n");
#				endif /* DEBUG */

				/* Empty picked area list */
				if (nmove > 0)
					{
					for (im=0; im<nmove; im++)
						ccopy[im] = remove_item_from_set(EditAreas,
														 (ITEM) ccopy[im]);
					nmove = 0;
					amove = 0;
					FREEMEM(cmove);
					FREEMEM(ccopy);
					}
				edit_select(NullCal, FALSE);

				/* Empty area label list */
				if (nlabs > 0)
					{
					for (il=0; il<nlabs; il++)
						scopy[il] = remove_item_from_set(EditLabs,
														 (ITEM) scopy[il]);
					nlabs = 0;
					alabs = 0;
					FREEMEM(smove);
					FREEMEM(scopy);
					}

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_merge_area] ReStart ... empty TempMeta %d fields\n",
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
*     e x t r a c t _ a r e a _ o r d e r _ t a g s                    *
*     r e l e a s e _ a r e a _ o r d e r _ t a g s                    *
*                                                                      *
***********************************************************************/

LOGICAL	extract_area_order_tags
	(
	LOGICAL	temp
	)

	{
	int			i;
	AREA		area;
	LINE		bound, line;
	POINT		pos;
	float		size, xsize, ysize, csize;
	float		xc, xl, xr, yc, yb, yt;
	AREA		box;
	LABEL		lab;
	char		cbuf[10];
	COLOUR		fcolr, tcolr;
	LOGICAL		valid, lok;
	METAFILE	meta;

	static  const   float   xfact = 1.4;
	static  const   float   yfact = 1.0;
	static  const   float   cfact = 0.9;
	static			POINT	rpos = {0,0};
	static			POINT	rver = {0,100};
	static			POINT	rhor = {100,0};

	(void) release_area_order_tags(temp);
	if (IsNull(NewAreas))     return FALSE;
	if (NewAreas->num <= 0)   return TRUE;
	if (IsNull(ActiveDfld) || !ActiveDfld->reorder) return FALSE;

	meta = (temp)? TempMeta: EditMeta;

	/* Compute appropriate size for the tag markers and labels */
	gxSetupTransform(DnEdit);
	size  = LmarkSize * gxGetMfact() / 1000;
	xsize = xfact * size;
	ysize = yfact * size;
	csize = cfact * LmarkSize;
	fcolr = find_colour("yellow", &valid);
	tcolr = find_colour("black", &valid);

	for (i=0; i<NewAreas->num; i++)
		{
		area  = (AREA) NewAreas->list[i];
		if (IsNull(area)) continue;
		bound = area->bound->boundary;
		if (IsNull(bound)) continue;

		/* Find an appropriate point to put the mark on */
		/* First check for areas that intersect the left edge of the map */
		lok = line_sight(bound, rpos, rver, FALSE, NullFloat, NullFloat,
				pos, NullInt, NullChar);

		/* Next check for areas that intersect the bottom edge of the map */
		if (!lok)
			lok = line_sight(bound, rpos, rhor, FALSE, NullFloat, NullFloat,
					pos, NullInt, NullChar);

		/* Last check for areas inside the map */
		if (!lok)
			line_test_point(bound, rpos, NullFloat,
					pos, NullInt, NullChar, NullChar);

		/* Construct a box */
		xl = pos[X];	xr = xl + xsize;
		yb = pos[Y];	yt = yb + ysize;
		line = create_line();
		add_point_to_line(line, make_point(xl, yb));
		add_point_to_line(line, make_point(xl, yt));
		add_point_to_line(line, make_point(xr, yt));
		add_point_to_line(line, make_point(xr, yb));
		add_point_to_line(line, make_point(xl, yb));
		box = create_area("", "", "");
		define_area_boundary(box, line);
		define_lspec(&box->lspec, 0, 0, NULL, FALSE, 2.0, 0.0,
					 (HILITE) 1);
		define_fspec(&box->fspec, fcolr, 1, NULL, FALSE, FALSE,
					 0.0, 0.0, (HILITE) 0);

		/* Add an order marker */
		xc = xl + xsize/2;
		yc = yb + ysize/2;
		(void) sprintf(cbuf, "%d", (i+1));
		lab = create_label("", "", cbuf, make_point(xc, yc), 0.0);
		define_tspec(&lab->tspec, tcolr, SafeFont, FALSE, csize, 0.0,
					 Hc, Vc, (HILITE) 0);

		/* Add them to the display */
		add_item_to_metafile(meta, "area",  "c", "tags", "", (ITEM) box);
		add_item_to_metafile(meta, "label", "d", "tags", "", (ITEM) lab);

#		ifdef DEBUG_STRUCTURES
		if (temp)
			pr_diag("Editor",
				"[extract_area_order_tags] Adding box/label to TempMeta (%d)\n",
				TempMeta->numfld);
#		endif /* DEBUG_STRUCTURES */
		}

	return TRUE;
	}

/**********************************************************************/

LOGICAL	release_area_order_tags
	(
	LOGICAL	temp
	)

	{
	METAFILE	meta;
	SET			set;

	if (IsNull(NewAreas))   return FALSE;

	meta = (temp)? TempMeta: EditMeta;

	set = take_mf_set(meta, "label", "d", "tags", "");
	if (NotNull(set)) set = destroy_set(set);
	set = take_mf_set(meta, "area", "c", "tags", "");
	if (NotNull(set)) set = destroy_set(set);
	return TRUE;
	}
