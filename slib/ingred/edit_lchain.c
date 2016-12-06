/***********************************************************************
*                                                                      *
*     e d i t _ l c h a i n . c                                        *
*                                                                      *
*     Assorted operations for link chain fields.                       *
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

#undef DEBUG_STRUCTURES

/***********************************************************************
*                                                                      *
*     r e a d y _ l c h a i n _ f i e l d                              *
*                                                                      *
***********************************************************************/

LOGICAL	edit_ready_lchain_field

	(
	STRING	mode
	)

	{
	int		butt;

	if (!EditUndoable)       edit_can_create(FALSE);
	if (!EditUndoable)       return TRUE;
	if (NotNull(OldLchains)) edit_can_create(FALSE);
	if (NotNull(OldLchains)) return TRUE;

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
			put_message("lchain-no-field");
			edit_can_create(FALSE);
			if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return FALSE;
			(void) ignore_Xpoint();
			}
		}

	/* No field is present, but it can be created */
	while (TRUE)
		{
		put_message("lchain-no-fld-cr");
		edit_can_create(TRUE);
		if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return FALSE;
		(void) ignore_Xpoint();
		}
	}

/***********************************************************************
*                                                                      *
*     a d d _ l c h a i n                                              *
*                                                                      *
***********************************************************************/

static	void		draw_lchain_display(LCHAIN);

static	LCHAIN	NewLchain = NullLchain;	/* Just added link chain */

LOGICAL	edit_add_lchain

	(
	STRING	mode,
	STRING	xtime,
	int		ndelta,
	STRING	ntype,
	CAL		cal,
	CAL		calx
	)

	{
	LNODE	lnode;
	POINT	pos, jpos;
	LOGICAL	joinstart, joinend;
	int		inode;
	float	dist;
	LMEMBER	ltype;
	STRING	vtime;
	int		butt;
	LOGICAL	drawn = FALSE;

	/* Current link chain */
	static	LOGICAL	empty = TRUE;
	static	LCHAIN	chain = NullLchain;
	static	POINT	lpos  = ZERO_POINT;
	static	int		lplus = 0;
	static	int		ijoin = -1;
	static	LCHAIN	join  = NullLchain;

	/* Current state: */
	/* - where should we resume? */
	static  enum    { Start, Add, AddChain, AddNode, Join,
						Set, End } State = Start;

#	ifdef DEBUG
	pr_diag("Editor",
		"[edit_add_lchain] %s  State: %d  Times: %s %d  for ntype: %s\n",
		mode, State, xtime, ndelta, ntype);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same_start(mode, "cancel"))
		{
		ignore_partial();
		if (!empty)
			{
			chain = destroy_lchain(chain);
			lplus = 0;
			empty = TRUE;
			empty_temp();
			}
		if (EditLchains && EditUndoable)
			{
			highlight_set(EditLchains, 1);
			highlight_set_secondary(EditLchains, 0);
			}
		edit_select(NullCal, FALSE);
		present_all();
		State = Start;
		return FALSE;
		}
	if (same(mode, "clear"))
		{
		State = Start;
		return FALSE;
		}

	/* Make sure the link chain set contains the right stuff */
	if (!EditLchains)                       return FALSE;
	if (!same(EditLchains->type, "lchain")) return FALSE;

	/* Check node time delta */
	if (ndelta == 0)
		{
		put_message("lchain-no-delta");
		return FALSE;
		}

	/* Initialize on startup */
	if (same(mode, "begin"))
		{
		ignore_partial();
		if (EditUndoable)
			{
			highlight_set(EditLchains, 1);
			highlight_set_secondary(EditLchains, 0);
			present_all();
			post_partial(FALSE);
			}
		State = Start;
		}

	/* Set state for Right button "end_chain" */
	if (same(mode, "end_chain"))
		{
		switch (State)
			{
			case Add:	State = End;
						break;

			default:	ignore_partial();
						break;
			}
		}

	/* Set state for Right button "new_chain" */
	if (same(mode, "new_chain"))
		{
		switch (State)
			{
			case Join:	ijoin = -1;
						join  = NullLchain;

						/* Create a new chain */
						State = AddChain;
						break;
			}
		}

	/* Take care of setting new value after adding */
	/* Only recognized when a link chain has just been added */
	if (same(mode, "set") && NotNull(cal)) State = Set;

	/* Repeat until told to quit */
	while (TRUE)
		{

		/* Resume at current stage */
		switch (State)
			{

			/* Start by setting the first link node */
			case Start:

				/* Select the start link chain node */
				edit_adding_lchain(xtime);
				if (same(ntype, FpaNodeClass_Normal))
					put_message("lchain-start-norm", xtime);
				else if (same(ntype, FpaNodeClass_Control))
					put_message("lchain-start-cont", xtime);
				if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return drawn;
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}

				/* Freeze all pending edits so only this one can be undone */
				if (EditUndoable)
					{
					ignore_partial();
					put_message("edit-freeze");
					accept_mod();	/* Leaves EditLchains alone */
					}
				post_partial(TRUE);

				/* Place the start link chain node */
				put_message("lchain-pick-node-loc");
				(void) pick_Xpoint(DnEdit, 0, pos, &butt);
				if (!inside_dn_window(DnEdit, pos))
					{
					put_message("edit-outside-map");
					(void) sleep(1);
					continue;
					}

				/* Check if inside start/end of another link chain */
				copy_point(lpos, pos);
				State = Join;
				continue;

			/* Add the next link node */
			case Add:

				/* Place the next link chain node */
				vtime = calc_valid_time_minutes(xtime, 0, lplus + ndelta);
				edit_adding_lchain(vtime);
				if (same(ntype, FpaNodeClass_Normal))
					put_message("lchain-next-norm", vtime);
				else if (same(ntype, FpaNodeClass_Control))
					put_message("lchain-next-cont", vtime);
				if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return drawn;
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}

				/* Set the next link node location */
				put_message("lchain-pick-node-loc");
				(void) utrack_Xnode_add(DnEdit, lpos, pos, ndelta, &butt);
				if (!inside_dn_window(DnEdit, pos))
					{
					put_message("edit-outside-map");
					(void) sleep(1);
					continue;
					}

				/* Add the next node */
				lplus += ndelta;
				copy_point(lpos, pos);
				State = AddNode;
				continue;

			/* Add a new link chain */
			case AddChain:

				/* Initialize a new link chain */
				put_message("lchain-adding", xtime);
				if (!chain) chain = create_lchain();
				define_lchain_reference_time(chain, xtime);
				define_lchain_interp_delta(chain, LchainDelta);
				define_lchain_attribs(chain, (ATTRIB_LIST) cal);
				edit_select((CAL) chain->attrib, TRUE);

				/* Display the active link chain */
				draw_lchain_display(chain);

				/* Add the current node to the new link chain */
				lplus = 0;
				empty = FALSE;
				State = AddNode;
				continue;

			/* Add a new link node to the current link chain */
			case AddNode:

				/* Adjust the start/end times */
				if (lplus < chain->splus)
					define_lchain_start_time(chain, lplus);
				if (lplus > chain->eplus)
					define_lchain_end_time(chain, lplus);
				define_lchain_default_attribs(chain);

				/* Add the node to the current link chain                      */
				/* Note that only normal nodes and control nodes can be added! */
				if (same(ntype, FpaNodeClass_Normal))
					{
					lnode = create_lnode(lplus);
					define_lnode_type(lnode, TRUE, FALSE, LchainNode);
					define_lnode_node(lnode, lpos);
					define_lnode_attribs(lnode, (ATTRIB_LIST) calx);
					(void) add_lchain_lnode(chain, lnode);
					}
				else if (same(ntype, FpaNodeClass_Control))
					{
					lnode = create_lnode(lplus);
					define_lnode_type(lnode, TRUE, FALSE, LchainControl);
					define_lnode_node(lnode, lpos);
					define_lnode_attribs(lnode, (ATTRIB_LIST) calx);
					(void) add_lchain_lnode(chain, lnode);
					}

				/* Display the active link chain */
				draw_lchain_display(chain);

				/* Add the next node */
				empty = FALSE;
				State = Add;
				continue;

			case Join:

				/* See if we can join to an existing chain */
				join = closest_lchain_node(EditLchains, lpos,
											&dist, jpos, &ltype, &inode); 
				/* Create a new link chain if not close to an existing node */
				if (!join || inode < 0 || dist > PickTol)
					{
					ijoin = -1;
					join  = NullLchain;
					State = AddChain;
					continue;
					}

				/* Create a new link chain if closest node is not a start/end */
				joinstart = lchain_start_node(join, ltype, inode);
				joinend   = lchain_end_node(join, ltype, inode);
				if (!joinstart && !joinend)
					{
					ijoin = -1;
					join  = NullLchain;
					State = AddChain;
					continue;
					}

				/* Start again if join is not at the correct end point */
				if (joinstart && !joinend && ndelta > 0)
					{
					put_message("lchain-join-not-start", ndelta);
					(void) sleep(1);
					ijoin = -1;
					join  = NullLchain;
					State = Start;
					continue;
					}
				if (!joinstart && joinend && ndelta < 0)
					{
					put_message("lchain-join-not-end", ndelta);
					(void) sleep(1);
					ijoin = -1;
					join  = NullLchain;
					State = Start;
					continue;
					}

				/* Can join to an existing link chain */
				put_message("lchain-join");
				edit_adding_lchain("NEW");
				if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return drawn;
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}

				put_message("lchain-release-join");
				(void) ignore_Xpoint();
				put_message("lchain-joining");

				/* Copy the existing link chain */
				chain = copy_lchain(join);
				ijoin = which_set_item(EditLchains, join);
				edit_select((CAL) chain->attrib, TRUE);

				/* Set the node time for the current chain */
				if (joinstart && ndelta < 0) lplus = chain->splus;
				if (joinend   && ndelta > 0) lplus = chain->eplus;
				copy_point(lpos, jpos);

				/* Display the active link chain */
				draw_lchain_display(chain);

				/* Add the next node */
				empty = FALSE;
				State = Add;
				continue;

			/* Set the value for the link chain just added */
			case Set:

				if (!edit_posted())
					{
					NewLchain = NullLchain;
					State = Start;
					continue;
					}

				/* Set a new value */
				busy_cursor(TRUE);
				put_message("lchain-attribs");
				define_item_attribs("lchain", (ITEM) NewLchain, cal);

				/* Set up display specs for the new link chain field */
				invoke_set_catspecs(EditLchains);
				if (EditUndoable)
					{
					highlight_set(EditLchains, 1);
					highlight_set_secondary(EditLchains, 0);
					}

				/* Register the edit */
				drawn = TRUE;
				if (EditUndoable) post_mod("lchains");

				/* Show the results */
				present_all();
				busy_cursor(FALSE);

				/* Move on to next stage */
				State = Start;
				continue;

			/* Set the value for the link chain just added */
			case End:

				/* Add the current link chain to the set */
				if (!empty)
					{
					put_message("lchain-draw-proc");
					ignore_partial();
					busy_cursor(TRUE);
					
					/* Invoke rules for all nodes and interpolate link chain */
					define_lchain_default_attribs(chain);
					CAL_invoke_all_lchain_lnode_rules(chain,
														CurrElement, CurrLevel);
					chain->dointerp = TRUE;
					(void) interpolate_lchain(chain);
					
					/* Replace existing link chain if joining */
					if (join)
						{
						join = destroy_lchain(join);
						EditLchains->list[ijoin] = (ITEM) chain;
						}

					/* Otherwise add the new link chain to the set */
					else
						{
						(void) add_item_to_set(EditLchains, (ITEM) chain);
						}

					/* Set up display specs for the new link chain */
					invoke_set_catspecs(EditLchains);
					if (EditUndoable)
						{
						highlight_set(EditLchains, 1);
						highlight_set_secondary(EditLchains, 0);
						}

					/* Save the new link chain for setting attributes */
					NewLchain = chain;
					chain = NullLchain;

					/* Register the edit */
					drawn = TRUE;
					if (EditUndoable) post_mod("lchains");

					/* Show the results */
					empty_temp();
					edit_select(NullCal, FALSE);
					present_all();
					busy_cursor(FALSE);
					}

				/* Move on to the next stage */
				State = Start;
				continue;
			}
		}
	}

/**********************************************************************/

static	void	draw_lchain_display

	(
	LCHAIN	chain
	)

	{
	int			inode;
	CURVE		track, box;
	POINT		pos;
	float		size, msize;
	float		xl, xr, yt, yb, xc, yc;

	/* >>>> turn these into set numbers? <<<<< */
	COLOUR		colour;
	HILITE		hilite;

	static	const	float	mfact  = 0.7;

	/* Return if no link chain */
	if (!chain) return;

	/* Empty the display */
	empty_temp();

	/* Build interpolated chain */
	(void) interpolate_lchain(chain);

	/* Decide where links are going */
	colour = SafeColour;
	hilite = 2;

	/* Compute appropriate size for the link markers */
	gxSetupTransform(DnLink);
	size  = LmarkSize * gxGetMfact() / 1000;
	msize = mfact * size;

	/* Display the link chain track as a thin line */
	if (chain->track && chain->track->numpts >= 2)
		{
		track = create_curve("", "", "");
		add_line_to_curve(track, chain->track);
		define_lspec(&track->lspec, colour, 0, NULL, FALSE, 1.5, 0.0, hilite);
		add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) track);
		}

	/* Display the interpolated nodes as small boxes */
	for (inode=0; inode<chain->inum; inode++)
		{
		if (!chain->interps[inode]->there) continue;

		/* Display a small box around interpolated node position */
		copy_point(pos, chain->interps[inode]->node);
		if (!inside_dn_window(DnMap, pos)) continue;
		xc = pos[X];	xl = xc - msize/8.0;	xr = xc + msize/8.0;
		yc = pos[Y];	yb = yc - msize/8.0;	yt = yc + msize/8.0;
		box = create_curve("", "", "");
		define_lspec(&box->lspec, colour, 0, NULL, FALSE, 1.5, 0.0, hilite);
		add_point_to_curve(box, make_point(xl, yt));
		add_point_to_curve(box, make_point(xr, yt));
		add_point_to_curve(box, make_point(xr, yb));
		add_point_to_curve(box, make_point(xl, yb));
		add_point_to_curve(box, make_point(xl, yt));

		/* Display the small box */
		add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) box);
		}

	/* Display the floating nodes as small diamonds */
	for (inode=0; inode<chain->lnum; inode++)
		{
		if (!chain->nodes[inode]->there)                  continue;
		if (chain->nodes[inode]->ltype != LchainFloating) continue;

		/* Display a diamond around control node position */
		copy_point(pos, chain->nodes[inode]->node);
		if (!inside_dn_window(DnMap, pos)) continue;
		xc = pos[X];	xl = xc - msize/2.0;	xr = xc + msize/2.0;
		yc = pos[Y];	yb = yc - msize/2.0;	yt = yc + msize/2.0;
		box = create_curve("", "", "");
		define_lspec(&box->lspec, colour, 0, NULL, FALSE, 1.5, 0.0, hilite);
		add_point_to_curve(box, make_point(xl, yc));
		add_point_to_curve(box, make_point(xc, yt));
		add_point_to_curve(box, make_point(xr, yc));
		add_point_to_curve(box, make_point(xc, yb));
		add_point_to_curve(box, make_point(xl, yc));

		/* Display the diamond */
		add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) box);
		}

	/* Display the control nodes as diamonds */
	for (inode=0; inode<chain->lnum; inode++)
		{
		if (!chain->nodes[inode]->there)                 continue;
		if (chain->nodes[inode]->ltype != LchainControl) continue;

		/* Display a diamond around control node position */
		copy_point(pos, chain->nodes[inode]->node);
		if (!inside_dn_window(DnMap, pos)) continue;
		xc = pos[X];	xl = xc - msize/1.5;	xr = xc + msize/1.5;
		yc = pos[Y];	yb = yc - msize/1.5;	yt = yc + msize/1.5;
		box = create_curve("", "", "");
		define_lspec(&box->lspec, colour, 0, NULL, FALSE, 1.5, 0.0, hilite);
		add_point_to_curve(box, make_point(xl, yc));
		add_point_to_curve(box, make_point(xc, yt));
		add_point_to_curve(box, make_point(xr, yc));
		add_point_to_curve(box, make_point(xc, yb));
		add_point_to_curve(box, make_point(xl, yc));

		/* Display the diamond */
		add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) box);
		}

	/* Display the link nodes as squares */
	for (inode=0; inode<chain->lnum; inode++)
		{
		if (!chain->nodes[inode]->there)              continue;
		if (chain->nodes[inode]->ltype != LchainNode) continue;

		/* Display a square around link node position */
		copy_point(pos, chain->nodes[inode]->node);
		if (!inside_dn_window(DnMap, pos)) continue;
		xc = pos[X];	xl = xc - msize/2.0;	xr = xc + msize/2.0;
		yc = pos[Y];	yb = yc - msize/2.0;	yt = yc + msize/2.0;
		box = create_curve("", "", "");
		define_lspec(&box->lspec, colour, 0, NULL, FALSE, 1.5, 0.0, hilite);
		add_point_to_curve(box, make_point(xl, yb));
		add_point_to_curve(box, make_point(xl, yt));
		add_point_to_curve(box, make_point(xr, yt));
		add_point_to_curve(box, make_point(xr, yb));
		add_point_to_curve(box, make_point(xl, yb));

		/* Display the box */
		add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) box);
		}

	/* Display the results */
	present_temp(TRUE);
	}

/***********************************************************************
*                                                                      *
*     m o v e _ l c h a i n                                            *
*                                                                      *
***********************************************************************/

static	LOGICAL	lchain_inside_outline(LCHAIN, CURVE);

/**********************************************************************/

LOGICAL	edit_move_lchain

	(
	STRING	mode,
	STRING	name
	)

	{
	LCHAIN	chain, echain, copy;
	SET		set, tset;
	CURVE	outline, tcurve;
	int		nlines;
	LINE	*lines;
	int		im, imx, iec;
	LOGICAL	*pmatch;
	POINT	p0, p1, pz;
	MARK	mark0, mark1;
	CURVE	curv0, curv1;
	float	dx, dy, dx0, dy0, angle, cang, sang;
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

	/* Move list: */
	/* - list of link chains to be moved */
	/* - cmove are pointers into EditLchains */
	/* - ccopy are copies to be displayed in TempMeta */
	static	const int	maxmove = 10000;
	static	int			nmove   = 0;
	static	int			amove   = 0;
	static	LCHAIN		*cmove  = NullLchainList;
	static	LCHAIN		*ccopy  = NullLchainList;

	/* Centre of rotation */
	static	POINT		Cpos    = ZERO_POINT;
	static	float		Ang     = 0.0;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_move_lchain] %s  State: %d\n", mode, State);
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

#							ifdef DEBUG_STRUCTURES
							pr_diag("Editor",
								"[edit_move_lchain] Cancel ... empty TempMeta %d field(s)\n",
								TempMeta->numfld);
#							endif /* DEBUG_STRUCTURES */

							/* Empty display structure */
							empty_temp();
							pasting = FALSE;
							mstart  = FALSE;
							clear_message();

							if (EditLchains && EditUndoable)
								{
								highlight_set(EditLchains, 1);
								highlight_set_secondary(EditLchains, 0);
								}

							edit_select(NullCal, FALSE);
							edit_can_copy(FALSE);
							if (copy_posted("lchains", CurrElement, CurrLevel))
								edit_can_paste(TRUE);
							present_all();
							State = Pick;
							break;
			}
		}
	if (same(mode, "cancel all"))
		{
		ignore_partial();
		drawing_control(FALSE);
		edit_allow_preset_outline(FALSE);

		/* Empty move list */
		if (nmove > 0)
			{

#			ifdef DEBUG_STRUCTURES
			for (im=0; im<nmove; im++)
				pr_diag("Editor",
					"[edit_move_lchain]   Cancel: %2d ... cmove/ccopy: %x %x\n",
				im, cmove[im], ccopy[im]);
#			endif /* DEBUG_STRUCTURES */

			nmove = 0;
			amove = 0;
			FREEMEM(cmove);
			FREEMEM(ccopy);
			}

#		ifdef DEBUG_STRUCTURES
		pr_diag("Editor",
			"[edit_move_lchain] Cancel ... empty TempMeta %d field(s)\n",
			TempMeta->numfld);
#		endif /* DEBUG_STRUCTURES */

		/* Empty display structure */
		empty_temp();
		pasting = FALSE;
		mstart  = FALSE;
		clear_message();

		edit_select(NullCal, FALSE);
		edit_can_copy(FALSE);
		if (copy_posted("lchains", CurrElement, CurrLevel))
			edit_can_paste(TRUE);
		return FALSE;
		}
	if (same(mode, "clear"))
		{
		/* >>>>> clear the paste buffer??? <<<<< */
		State = Pick;
		return FALSE;
		}

	/* Make sure the link chain set contains the right stuff */
	if (!EditLchains)                       return FALSE;
	if (!same(EditLchains->type, "lchain")) return FALSE;

	/* Construct temporary fields and buffers on startup only */
	if (same(mode, "begin"))
		{
		ignore_partial();
		if (EditUndoable)
			{
			highlight_set(EditLchains, 1);
			highlight_set_secondary(EditLchains, 0);
			present_all();
			post_partial(FALSE);
			}

#		ifdef DEBUG_STRUCTURES
		pr_diag("Editor",
			"[edit_move_lchain]   Begin: nmove: %d\n", nmove);
#		endif /* DEBUG_STRUCTURES */

		/* Set up move list */
		nmove = 0;
		amove = 0;
		FREEMEM(cmove);
		FREEMEM(ccopy);

#		ifdef DEBUG_STRUCTURES
		pr_diag("Editor",
			"[edit_move_lchain] Begin ... empty TempMeta %d field(s)\n",
			TempMeta->numfld);
#		endif /* DEBUG_STRUCTURES */

		/* Empty display structure */
		empty_temp();
		pasting = FALSE;
		mstart  = FALSE;

		edit_select(NullCal, FALSE);
		edit_can_copy(FALSE);
		if (copy_posted("lchains", CurrElement, CurrLevel))
			edit_can_paste(TRUE);
		edit_allow_preset_outline(FALSE);

		State = Pick;
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
		put_message("lchain-no-pick");
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
		put_message("lchain-no-pick");
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
					"[edit_move_lchain] No outline drawn yet!\n");
				}
			else
				{
				put_message("edit-no-preset-outline", name);
				pr_error("Editor",
					"[edit_move_lchain] No preset map outline: \"%s\"\n", name);
				}
			(void) sleep(1);
			State = Pick;
			}

		/* Display the outline */
		else
			{
			define_lspec(&outline->lspec, 255, 0, NULL, False, 2.0, 0.0,
						 (HILITE) 2);
			add_item_to_metafile(TempMeta, "curve", "c", "", "",
								(ITEM) outline);
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

			/* Pick link chains to move */
			case Pick:

				/* See if there are any link chains to move */
				if (EditLchains->num <= 0)
					{
					if (copy_posted("lchains", CurrElement, CurrLevel))
						edit_can_paste(TRUE);
					put_message("lchain-no-move");
					edit_select(NullCal, FALSE);
					return drawn;
					}

				/* Allow specification of multiple moves */
				while (nmove < maxmove)
					{
					/* Put up a prompt and set states */
					if (nmove <= 0)
						{
						put_message("lchain-move-pick");
						edit_can_copy(FALSE);
						if (copy_posted("lchains", CurrElement, CurrLevel))
							edit_can_paste(TRUE);
						edit_select(NullCal, FALSE);
						ignore_partial();
						}
					else
						{
						put_message("lchain-move-pick2");
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
					put_message("lchain-pick");
					if (!pick_Xpoint(DnEdit, 0, p1, &butt)) continue;
					if (!inside_dn_window(DnEdit, p1))
						{
						put_message("edit-outside-map");
						(void) sleep(1);
						continue;
						}

					/* See which link chain we picked */
					chain = closest_lchain(EditLchains, p1,
											NullFloat, NullPoint, NullInt);
					if (!chain)
						{
						put_message("lchain-no-pick");
						(void) sleep(1);
						continue;
						}

					/* If link chain was already picked, un-pick it */
					for (im=0; im<nmove; im++)
						{
						if (chain == cmove[im]) break;
						}
					if (im < nmove)
						{
						put_message("lchain-unpicked");

						/* Remove the picked link chain from the list */
						set = find_mf_set(TempMeta, "lchain", "l", "mmv", "mmv");
						(void) remove_item_from_set(set, (ITEM) ccopy[im]);
						nmove--;
						for (imx=im; imx<nmove; imx++)
							{
							cmove[imx] = cmove[imx+1];
							ccopy[imx] = ccopy[imx+1];
							}
						cmove[imx] = NullLchain;
						ccopy[imx] = NullLchain;

						/* Display remaining picked link chains */
						present_all();
						pasting = FALSE;
						}

					/* Otherwise pick it */
					else
						{
						put_message("lchain-picked");
						nmove++;
						if (nmove > amove)
							{
							amove = nmove;
							cmove = GETMEM(cmove, LCHAIN, amove);
							ccopy = GETMEM(ccopy, LCHAIN, amove);
							}

						/* Produce a copy of the picked link chain */
						cmove[im] = chain;
						ccopy[im] = copy_lchain(chain);
						copy      = ccopy[im];

						/* Highlight the picked chain and add it to the list */
						highlight_lchain(copy, 2);
						highlight_lchain_nodes(copy, 2, 2);
						widen_lchain(copy, 2.0);

						add_item_to_metafile(TempMeta, "lchain", "l",
											 "mmv", "mmv", (ITEM) copy);

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[edit_move_lchain] Adding link chain (%d) to TempMeta (%d)\n",
							im, TempMeta->numfld);
#						endif /* DEBUG_STRUCTURES */

						/* Display picked link chains */
						present_temp(TRUE);
						}
					}

				pr_warning("Editor",
					"[edit_move_lchain]   End of State = Pick loop!\n");

				/* Have we picked any link chains? */
				if (nmove > 0) edit_select((CAL) cmove[0]->attrib, FALSE);
				else           edit_select(NullCal, FALSE);

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Pick all link chains to move */
			case PickAll:

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_move_lchain] PickAll ... empty TempMeta %d field(s)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();

				put_message("lchain-all-picked");
				nmove = EditLchains->num;
				if (nmove > amove)
					{
					amove = nmove;
					cmove = GETMEM(cmove, LCHAIN, amove);
					ccopy = GETMEM(ccopy, LCHAIN, amove);
					}

				/* Highlight all the link chains and add them to the list */
				for (im=0; im<nmove; im++)
					{
					cmove[im] = (LCHAIN) EditLchains->list[im];
					ccopy[im] = copy_lchain(cmove[im]);
					copy      = ccopy[im];
					highlight_lchain(copy, 2);
					highlight_lchain_nodes(copy, 2, 2);
					widen_lchain(copy, 2.0);

					add_item_to_metafile(TempMeta, "lchain", "l",
										 "mmv", "mmv", (ITEM) copy);

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_move_lchain] PickAll ... adding link chain (%d) to TempMeta (%d)\n",
						im, TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */
					}

				/* Display picked link chains */
				present_temp(TRUE);
				pasting = FALSE;

				/* Have we picked any link chains? */
				if (nmove > 0) edit_select((CAL) cmove[0]->attrib, FALSE);
				else           edit_select(NullCal, FALSE);

#				ifdef DEBUG_STRUCTURES
				for (im=0; im<nmove; im++)
					pr_diag("Editor",
						"[edit_move_lchain]   PickAll: %2d ... cmove/ccopy: %x %x\n",
					im, cmove[im], ccopy[im]);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Draw outline around link chains to move */
			case Draw:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_move_lchain] Drawing outline\n");
#				endif /* DEBUG */

				/* Draw the outline */
				post_partial(FALSE);
				edit_allow_preset_outline(FALSE);
				put_message("lchain-move-draw");
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

			/* Pick link chains to move from inside drawn outline */
			case DrawPick:

				if (IsNull(outline))
					{
					edit_select(NullCal, FALSE);
					State = Pick;
					continue;
					}

#				ifdef DEBUG
				pr_diag("Editor",
					"[edit_move_lchain] Picking link chains inside outline\n");
#				endif /* DEBUG */

				busy_cursor(TRUE);
				edit_allow_preset_outline(FALSE);
				put_message("lchain-move-select");
				(void) sleep(1);

				/* Pick link chains inside drawn outline */
				for (iec=0; iec<EditLchains->num; iec++)
					{
					echain = (LCHAIN) EditLchains->list[iec];
					if (!echain) continue;

					/* Check if link chain is inside drawn outline */
					if (!lchain_inside_outline(echain, outline)) continue;

					/* Pick link chains that are not already picked */
					for (im=0; im<nmove; im++)
						{
						if (echain == cmove[im]) break;
						}
					if (im >= nmove)
						{
						put_message("lchain-picked");
						nmove++;
						if (nmove > amove)
							{
							amove = nmove;
							cmove = GETMEM(cmove, LCHAIN, amove);
							ccopy = GETMEM(ccopy, LCHAIN, amove);
							}

						/* Produce a copy of the picked link chain */
						cmove[im] = echain;
						ccopy[im] = copy_lchain(echain);
						copy      = ccopy[im];

						/* Highlight the picked chain and add it to the list */
						highlight_lchain(copy, 2);
						highlight_lchain_nodes(copy, 2, 2);
						widen_lchain(copy, 2.0);

						add_item_to_metafile(TempMeta, "lchain", "l",
											 "mmv", "mmv", (ITEM) copy);

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[edit_move_lchain] Adding link chain (%d) to TempMeta (%d)\n",
							im, TempMeta->numfld);
#						endif /* DEBUG_STRUCTURES */
						}
					}

				/* Display picked link chains */
				present_temp(TRUE);
				busy_cursor(FALSE);

				/* Have we picked any link chains? */
				if (nmove > 0) edit_select((CAL) cmove[0]->attrib, FALSE);
				else           edit_select(NullCal, FALSE);

				/* Re-display the picked link chains */
				State = ReDisplay;
				continue;

			/* Cut link chains */
			case Cut:

				/* Save the link chains in the copy buffer */
				put_message("edit-cut");
				post_lchain_copy(CurrElement, CurrLevel, nmove, cmove);

				/* Remove link chains from link chain set */
				for (im=0; im<nmove; im++)
					{
					chain = cmove[im];
					(void) remove_item_from_set(EditLchains, (ITEM) chain);
					cmove[im] = NullLchain;
					}

				if (EditUndoable) post_mod("lchains");

				/* Empty move list */
				if (nmove > 0)
					{

#					ifdef DEBUG_STRUCTURES
					for (im=0; im<nmove; im++)
						pr_diag("Editor",
							"[edit_move_lchain]   Cut: %2d ... cmove/ccopy: %x %x\n",
						im, cmove[im], ccopy[im]);
#					endif /* DEBUG_STRUCTURES */

					nmove = 0;
					amove = 0;
					FREEMEM(cmove);
					FREEMEM(ccopy);
					}

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_move_lchain] Cut ... empty TempMeta %d field(s)\n",
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

			/* Copy link chains */
			case Copy:

				/* Save the link chains in the copy buffer */
				put_message("edit-copy");
				post_lchain_copy(CurrElement, CurrLevel, nmove, cmove);
				clear_message();
				pasting = FALSE;

				/* Re-display the picked link chains */
				State = ReDisplay;
				continue;

			/* Paste link chains */
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
							"[edit_move_lchain]   Paste: %2d ... cmove/ccopy: %x %x\n",
						im, cmove[im], ccopy[im]);
#					endif /* DEBUG_STRUCTURES */

					nmove = 0;
					amove = 0;
					FREEMEM(cmove);
					FREEMEM(ccopy);

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_move_lchain] Paste ... empty TempMeta %d field(s)\n",
						TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */

					/* Empty display structure */
					empty_temp();
					}

				/* Retrieve paste buffer */
				paste_lchain_copy(CurrElement, CurrLevel, &nmove, &cmove);
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
					ccopy = GETMEM(ccopy, LCHAIN, amove);
					}

				/* Check for pasting duplicate link chains */
				pmatch = INITMEM(LOGICAL, nmove);
				for (im=0; im<nmove; im++)
					{

					/* Initialize match parameter */
					pmatch[im] = FALSE;

					/* Check for matching link chains */
					chain = cmove[im];
					for (iec=0; iec<EditLchains->num; iec++)
						{
						echain = (LCHAIN) EditLchains->list[iec];
						if (!echain) continue;

						/* Identify link chains pasted over themselves */
						if (matching_lines(chain->track, echain->track))
							{
							pmatch[im] = TRUE;
							break;
							}
						}
					}

				/* Add pasted link chains to link chain set */
				for (im=0; im<nmove; im++)
					{
					chain = cmove[im];

					/* Offset link chains pasted over themselves */
					if (pmatch[im])
						{
						translate_lchain(chain, PickTol, PickTol);
						}

					/* Adjust the link chain reference time to match */
					/*  the current depiction time (if required)     */
					if ( !same(chain->xtime, TimeList[EditTime].jtime) )
						{

#						ifdef DEBUG
						pr_diag("Editor",
							"[edit_move_lchain] Reset reference time for %d from: %s to: %s\n",
							im, chain->xtime, TimeList[EditTime].jtime);
#						endif /* DEBUG */

						define_lchain_reference_time(chain,
														TimeList[EditTime].jtime);

						/* Ensure "normal" type node at current reference time */
						promote_lchain_node(chain, LchainNode, 0);
						}

					/* Invoke rules for all nodes and re-interpolate link chain */
					define_lchain_default_attribs(chain);
					CAL_invoke_all_lchain_lnode_rules(chain,
														CurrElement, CurrLevel);
					chain->dointerp = TRUE;
					(void) interpolate_lchain(chain);

					/* Add pasted link chain to link chain set */
					(void) add_item_to_set(EditLchains, (ITEM) chain);
					ccopy[im] = copy_lchain(chain);
					copy      = ccopy[im];

					/* Highlight the picked link chain and add it to the list */
					highlight_lchain(copy, 2);
					highlight_lchain_nodes(copy, 2, 2);
					widen_lchain(copy, 2.0);

					add_item_to_metafile(TempMeta, "lchain", "l",
										 "mmv", "mmv", (ITEM) copy);

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_move_lchain] Paste ... adding link chain (%d) to TempMeta (%d)\n",
						im, TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */
					}

#				ifdef DEBUG_STRUCTURES
				for (im=0; im<nmove; im++)
					pr_diag("Editor",
						"[edit_move_lchain]   Paste: %2d ... cmove/ccopy: %x %x\n",
					im, cmove[im], ccopy[im]);
#				endif /* DEBUG_STRUCTURES */

				if (EditUndoable) post_mod("lchains");

				/* Free match buffer */
				FREEMEM(pmatch);

				/* Re-display the link chains */
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

				/* Highlight the picked link chains */
				mstart = TRUE;
				for (im=0; im<nmove; im++)
					{
					copy = ccopy[im];
					highlight_lchain(copy, 3);
					highlight_lchain_nodes(copy, 3, 3);
					}

				/* Remove any marks from uncompleted actions */
				set = take_mf_set(TempMeta, "mark", "d", "", "");
				if (NotNull(set)) set = destroy_set(set);

				/* Re-display the lchains */
				present_all();

#				ifdef DEBUG_STRUCTURES
				for (im=0; im<nmove; im++)
					pr_diag("Editor",
						"[edit_move_lchain]   Translate: %2d ... cmove/ccopy: %x %x\n",
					im, cmove[im], ccopy[im]);
#				endif /* DEBUG_STRUCTURES */

				post_partial(TRUE);
				edit_can_copy(TRUE);

				/* Pick a reference point */
				put_message("lchain-tran");
				if (!ready_Xpoint(DnEdit, p0, &butt)) return drawn;
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!inside_dn_window(DnEdit, p0))
					{
					put_message("lchain-tran-out1");
					(void) ignore_Xpoint();
					continue;
					}

				/* Highlight the link chain tracks */
				set = find_mf_set(TempMeta, "lchain", "l", "mmv", "mmv");
				tset = create_set("curve");
				for (im=0; im<set->num; im++)
					{
					chain  = (LCHAIN) set->list[im];
					tcurve = create_curve("", "", "");
					add_line_to_curve(tcurve, chain->track);
					highlight_curve(tcurve, 2);
					widen_curve(tcurve, 4.0);
					(void) add_item_to_set(tset, (ITEM) tcurve);
					}

				/* Translate the reference point */
				put_message("lchain-tran-release");
				(void) utrack_Xpoint(DnEdit, tset, p0, p1, &butt);
				tset = destroy_set(tset);
				if (!inside_dn_window(DnEdit, p1))
					{
					put_message("lchain-tran-out2");
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
					"[edit_move_lchain] Adding mark/mark/curve to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = TranslateDone;
				continue;

			/* Translate the link chains */
			case TranslateDone:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_move_lchain] Translating\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);

#				ifdef DEBUG_STRUCTURES
				for (im=0; im<nmove; im++)
					pr_diag("Editor",
						"[edit_move_lchain]   Translating: %2d ... cmove/ccopy: %x %x\n",
					im, cmove[im], ccopy[im]);
#				endif /* DEBUG_STRUCTURES */

				/* Translate the link chains */
				dx = p1[X] - p0[X];
				dy = p1[Y] - p0[Y];
				for (im=0; im<nmove; im++)
					{
					chain = cmove[im];
					translate_lchain(chain, dx, dy);
					ccopy[im] = NullLchain;
					}

				if (EditUndoable) post_mod("lchains");

				put_message("edit-done");
				pasting = FALSE;
				mstart  = FALSE;
				busy_cursor(FALSE);

				/* Re-display the picked link chains */
				State = ReDisplay;
				continue;

			/* Pick the centre of rotation */
			case Centre:

				/* Highlight the picked link chains */
				mstart = TRUE;
				for (im=0; im<nmove; im++)
					{
					copy = ccopy[im];
					highlight_lchain(copy, 3);
					highlight_lchain_nodes(copy, 3, 3);
					}

				/* Remove any marks from uncompleted actions */
				set = take_mf_set(TempMeta, "mark", "d", "", "");
				if (NotNull(set)) set = destroy_set(set);

				/* Re-display the link chains */
				present_all();

#				ifdef DEBUG_STRUCTURES
				for (im=0; im<nmove; im++)
					pr_diag("Editor",
						"[edit_move_lchain]   Rotate: %2d ... cmove/ccopy: %x %x\n",
					im, cmove[im], ccopy[im]);
#				endif /* DEBUG_STRUCTURES */

				post_partial(TRUE);
				edit_can_copy(TRUE);

				/* Pick the centre of rotation */
				put_message("lchain-rot-centre");
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
					put_message("lchain-rot-out");
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
					"[edit_move_lchain] Adding mark to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = Rotate;
				continue;

			/* Pick and rotate a reference point */
			case Rotate:

				/* Pick a reference point */
				put_message("lchain-rot");
				if (!ready_Xpoint(DnEdit, p0, &butt)) return drawn;
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!inside_dn_window(DnEdit, p0))
					{
					put_message("lchain-rot-out1");
					(void) ignore_Xpoint();
					continue;
					}
				if (point_dist(Cpos, p0) < SplineRes)
					{
					put_message("lchain-rot-ref");
					(void) ignore_Xpoint();
					(void) sleep(1);
					continue;
					}

				/* Highlight the link chain tracks */
				set = find_mf_set(TempMeta, "lchain", "l", "mmv", "mmv");
				tset = create_set("curve");
				for (im=0; im<set->num; im++)
					{
					chain  = (LCHAIN) set->list[im];
					tcurve = create_curve("", "", "");
					add_line_to_curve(tcurve, chain->track);
					highlight_curve(tcurve, 2);
					widen_curve(tcurve, 4.0);
					(void) add_item_to_set(tset, (ITEM) tcurve);
					}

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
					"[edit_move_lchain] Adding mark/curve to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Rotate the reference point */
				put_message("lchain-rot-release");
				(void) urotate_Xpoint(DnEdit, tset, Cpos, p0, p1, &Ang, &butt);
				tset = destroy_set(tset);
				if (!inside_dn_window(DnEdit, p1))
					{
					put_message("lchain-rot-out2");
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
					"[edit_move_lchain] Adding mark/curve to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = RotateDone;
				continue;

			/* Rotate the link chains */
			case RotateDone:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_move_lchain] Rotating\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);

#				ifdef DEBUG_STRUCTURES
				for (im=0; im<nmove; im++)
					pr_diag("Editor",
						"[edit_move_lchain]   Rotating: %2d ... cmove/ccopy: %x %x\n",
					im, cmove[im], ccopy[im]);
#				endif /* DEBUG_STRUCTURES */

				/* Rotate the link chains */
				dx0 = p0[X] - Cpos[X];
				dy0 = p0[Y] - Cpos[Y];
				for (im=0; im<nmove; im++)
					{
					chain = cmove[im];
					rotate_lchain(chain, Cpos, Ang/RAD);
					ccopy[im] = NullLchain;
					}

				if (EditUndoable) post_mod("lchains");

				put_message("edit-done");
				pasting = FALSE;
				mstart  = FALSE;
				busy_cursor(FALSE);

				/* Re-display the picked link chains */
				State = ReDisplay;
				continue;

			/* Re-display the picked link chains */
			case ReDisplay:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_move_lchain] Redisplay\n");
#				endif /* DEBUG */

				if (EditLchains && EditUndoable)
					{
					highlight_set(EditLchains, 1);
					highlight_set_secondary(EditLchains, 0);
					}

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_move_lchain] Redisplay ... empty TempMeta %d field(s)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();

				/* Re-pick the picked link chains */
				for (im=0; im<nmove; im++)
					{

					/* Highlight the picked link chain */
					ccopy[im] = copy_lchain(cmove[im]);
					copy      = ccopy[im];
					highlight_lchain(copy, 2);
					highlight_lchain_nodes(copy, 2, 2);
					widen_lchain(copy, 2.0);

					add_item_to_metafile(TempMeta, "lchain", "l",
										 "mmv", "mmv", (ITEM) copy);

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_move_lchain] Redisplay ... adding link chain (%d) to TempMeta (%d)\n",
						im, TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */
					}

				/* Re-display the link chains */
				present_all();
				post_partial(FALSE);

				/* Move on to next stage */
				State = Pick;
				continue;
			}
		}
	}

/**********************************************************************/

static	LOGICAL	lchain_inside_outline

	(
	LCHAIN	chain,
	CURVE	outline
	)

	{
	int		ii;
	LOGICAL	inside;
	LINE	oline, track;

	if (!chain)                      return FALSE;
	if (!chain->track)               return FALSE;
	if (!outline)                    return FALSE;
	if (!outline->line)              return FALSE;
	if (!line_closed(outline->line)) return FALSE;

	/* Ensure that all points in link chain track are inside outline */
	oline = outline->line;
	track = chain->track;
	for (ii=0; ii<track->numpts; ii++)
		{
		line_test_point(oline, track->points[ii], NullFloat, NullPoint, NullInt,
						&inside, NullLogical);
		if (!inside) return FALSE;
		}

	/* Return TRUE if all points in link chain track were inside outline */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     m o d i f y _ l c h a i n                                        *
*     m o d i f y _ l c h a i n _ p i c k                              *
*                                                                      *
***********************************************************************/

typedef	enum
			{
			ModPick, ModPicked,				/* interactive states */
			ModRePick,						/* "cancel" states */
			ModSet, ModSetTimes, ModDelete,	/* modification states */
			ModConfirm, ModDelConfirm		/* complete modification */
			}
		MODSTATE;

static	MODSTATE	modify_lchain_pick(void);

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
		case ModPicked:		name = "Picked";		break;
		case ModRePick:		name = "RePick";		break;
		case ModSet:		name = "Set";			break;
		case ModSetTimes:	name = "SetTimes";		break;
		case ModDelete:		name = "Delete";		break;
		case ModConfirm:	name = "Confirm";		break;
		case ModDelConfirm:	name = "DelConfirm";	break;
		default:			name = "!Unknown!";		break;
		}
	pr_diag("Editor", "[edit_modify_lchain] State: %s %s\n", pre, name);
#	endif /* DEBUG */

	return;
	}
/* For debug purposes */

static	LOGICAL	Mfresh = FALSE;			/* Just picked */
static	LCHAIN	Mpick  = NullLchain;	/* Picked link chain */
static	LCHAIN	Mwork  = NullLchain;	/* Working copy of picked link chain */

/**********************************************************************/

LOGICAL	edit_modify_lchain

	(
	STRING	mode,
	STRING	xtime,
	int		splus,
	int		eplus,
	CAL		cal
	)

	{
	MODSTATE	next;
	LOGICAL		drawn = FALSE;

	/* Current state: */
	/* - where should we resume? */
	static	MODSTATE	State = ModPick;

#	ifdef DEBUG
	pr_diag("Editor",
		"[edit_modify_lchain] %s  State: %d  Times: \"%s\" %d to %d\n",
		mode, State, xtime, splus, eplus);
#	endif /* DEBUG */
	show_mod_state("INITIAL", State);

	/* Clean up previous edits */
	if (same(mode, "cancel"))
		{
		switch (State)
			{
			case ModPicked:		State = ModRePick;
								break;

			default:			empty_temp();
								if (EditLchains && EditUndoable)
									{
									highlight_set(EditLchains, 1);
									highlight_set_secondary(EditLchains, 0);
									}
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
		Mpick  = NullLchain;
		Mwork  = NullLchain;

		/* Empty display structure */
		empty_temp();
		return FALSE;
		}
	if (same(mode, "clear"))
		{
		return FALSE;
		}

	/* Make sure the link chain set contains the right stuff */
	if (!EditLchains)                       return FALSE;
	if (!same(EditLchains->type, "lchain")) return FALSE;

	/* Initialize on startup */
	if (same(mode, "begin"))
		{
		ignore_partial();
		if (EditUndoable)
			{
			highlight_set(EditLchains, 1);
			highlight_set_secondary(EditLchains, 0);
			present_all();
			post_partial(FALSE);
			}
		Mfresh = FALSE;
		edit_select(NullCal, FALSE);
		State = ModPick;
		show_mod_state("BEGIN", State);
		}

	/* Take care of deleting or setting new value */
	/* Only recognized when something is picked */
	if (same(mode, "set"))
		{
		if (State == ModPicked) State = ModSet;
		show_mod_state("SET", State);
		}
	if (same(mode, "set_times"))
		{
		if (State == ModPicked) State = ModSetTimes;
		show_mod_state("SET_TIMES", State);
		}
	if (same(mode, "delete"))
		{
		if (State == ModPicked) State = ModDelete;
		show_mod_state("DELETE", State);
		}

	/* Repeat until told to quit */
	while (TRUE)
		{
		show_mod_state("==", State);

		/* Resume at current stage */
		switch (State)
			{

			/* Pick a link chain to modify */
			case ModPick:

				/* Pick a link chain */
				ignore_partial();
				next = modify_lchain_pick();
				if (next == ModPick) return drawn;

				/* Move on to next stage */
				State = next;
				show_mod_state("->", State);
				continue;

			/*  Repick the link chain to modify */
			case ModRePick:

				put_message("edit-cancel");

				/* Clean up temporary buffers */
				ignore_partial();
				edit_select(NullCal, FALSE);
				Mfresh = FALSE;
				Mpick  = NullLchain;
				Mwork  = NullLchain;

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_modify_lchain] ModRePick ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();

				/* Move on to next stage */
				State = ModPick;
				show_mod_state("->", State);
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
						accept_mod();	/* Leaves EditLchains alone */
						drawn = FALSE;
						}
					}
				post_partial(TRUE);
				put_message("lchain-modify");

				return drawn;

			/* Delete the picked link chain */
			case ModDelete:

				/* Freeze all pending edits so only this one can be undone */
				if (EditUndoable)
					{
					ignore_partial();
					put_message("edit-freeze");
					accept_mod();	/* Leaves EditLchains alone */
					}
				post_partial(TRUE);

#				ifdef DEBUG
				pr_diag("Editor", "[edit_modify_lchain] Deleting\n");
#				endif /* DEBUG */

				/* Remove the link chain */
				busy_cursor(TRUE);
				if (Mpick)
					{
					put_message("lchain-deleting");
					remove_item_from_set(EditLchains, (ITEM) Mpick);
					}

				/* Show the results */
				present_all();

				/* Move on to next stage */
				State = ModDelConfirm;
				show_mod_state("->", State);
				continue;

			/* Set the attributes of the picked link chain */
			case ModSet:

				/* Freeze all pending edits so only this one can be undone */
				/* (first time after picking) */
				if (Mfresh)
					{
					Mfresh = FALSE;
					if (EditUndoable && edit_posted())
						{
						ignore_partial();
						put_message("edit-freeze");
						accept_mod();	/* Leaves EditLchains alone */
						drawn = FALSE;
						}
					}
				post_partial(TRUE);

#				ifdef DEBUG
				pr_diag("Editor", "[edit_modify_lchain] Setting value (%s)\n",
						CAL_get_attribute(cal, CALuserlabel));
#				endif /* DEBUG */

				/* Set new attributes */
				busy_cursor(TRUE);
				if (Mpick)
					{
					put_message("lchain-attribs");
					define_lchain_attribs(Mpick, cal);
					}

				/* Move on to the next stage */
				State = ModConfirm;
				continue;

			/* Set the time parameters of the picked link chain */
			case ModSetTimes:

				/* Freeze all pending edits so only this one can be undone */
				/* (first time after picking) */
				if (Mfresh)
					{
					Mfresh = FALSE;
					if (EditUndoable && edit_posted())
						{
						ignore_partial();
						put_message("edit-freeze");
						accept_mod();	/* Leaves EditLchains alone */
						drawn = FALSE;
						}
					}
				post_partial(TRUE);

#				ifdef DEBUG
				pr_diag("Editor",
						"[edit_modify_lchain] Setting times (%s %d %d)\n",
						xtime, splus, eplus);
#				endif /* DEBUG */

				/* Set new time parameters */
				busy_cursor(TRUE);
				if (Mpick)
					{

					/* Set start and end times first              */
					/*  since they are relative to reference time */
					put_message("lchain-times");
					define_lchain_start_time(Mpick, splus);
					define_lchain_end_time(Mpick, eplus);

					/* Adjust the reference time (if required) */
					if ( !same(Mpick->xtime, xtime) )
						{

#						ifdef DEBUG
						pr_diag("Editor",
							"[edit_modify_lchain] Reset reference time from: %s to: %s\n",
							Mpick->xtime, xtime);
#						endif /* DEBUG */

						define_lchain_reference_time(Mpick, xtime);

						/* Ensure "normal" type node at current reference time */
						promote_lchain_node(Mpick, LchainNode, 0);
						}

					/* Invoke rules for all nodes and re-interpolate link chain */
					define_lchain_default_attribs(Mpick);
					CAL_invoke_all_lchain_lnode_rules(Mpick,
														CurrElement, CurrLevel);
					Mpick->dointerp = TRUE;
					(void) interpolate_lchain(Mpick);
					}

				/* Move on to the next stage */
				State = ModConfirm;
				continue;

			/* Confirm current edit */
			case ModConfirm:

				/* Register the edit */
				ignore_partial();
				drawn  = TRUE;
				invoke_set_catspecs(EditLchains);
				if (EditUndoable)
					{
					highlight_set(EditLchains, 1);
					highlight_set_secondary(EditLchains, 0);
					}
				if (EditUndoable) post_mod("lchains");

				/* Clean up temporary buffers */
				edit_select(NullCal, FALSE);
				Mfresh = FALSE;
				Mwork  = NullLchain;

				/* Empty display structure */
				empty_temp();

				/* Produce a copy of the picked link chain to work on */
				put_message("lchain-picked");
				edit_select((CAL) Mpick->attrib, TRUE);

				/* Highlight the picked link chain */
				Mwork = copy_lchain(Mpick);
				widen_lchain(Mwork, 2.0);
				highlight_lchain(Mwork, 2);
				highlight_lchain_nodes(Mwork, 2, 2);
				add_item_to_metafile(TempMeta, "lchain", "l", "", "",
						(ITEM) Mwork);

				/* Display the link chain */
				present_temp(TRUE);
				busy_cursor(FALSE);

				/* Wait for next action */
				State = ModPicked;
				show_mod_state("->", State);
				continue;

			/* Confirm current edit */
			case ModDelConfirm:

				/* Register the edit */
				ignore_partial();
				drawn  = TRUE;
				invoke_set_catspecs(EditLchains);
				if (EditUndoable)
					{
					highlight_set(EditLchains, 1);
					highlight_set_secondary(EditLchains, 0);
					}
				if (EditUndoable) post_mod("lchains");

				/* Clean up temporary buffers */
				edit_select(NullCal, FALSE);
				Mfresh = FALSE;
				Mpick = NullLchain;
				Mwork  = NullLchain;

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_modify_lchain] ModConfirm ... empty TempMeta %d fields\n",
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

static	MODSTATE	modify_lchain_pick(void)

	{
	POINT	pos;
	int		butt;

	/* See if there are any link chains to modify */
	if (EditLchains->num <= 0)
		{
		put_message("lchain-no-mod");
		return ModPick;
		}

	/* Keep trying until something gets picked */
	while (TRUE)
		{
		/* Get a point */
		put_message("lchain-mod-pick");
		if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return ModPick;
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}
		put_message("lchain-pick");
		if (!pick_Xpoint(DnEdit, 0, pos, &butt)) continue;
		if (!inside_dn_window(DnEdit, pos))
			{
			put_message("edit-outside-map");
			(void) sleep(1);
			continue;
			}

		/* See which link chain was picked */
		/* Find the closest link chain */
		Mpick = closest_lchain(EditLchains, pos, NullFloat, NullPoint, NullInt);
		if (!Mpick)
			{
			put_message("lchain-no-pick");
			(void) sleep(1);
			continue;
			}
		Mfresh = TRUE;

		/* Produce a copy of the picked link chain to work on */
		put_message("lchain-picked");
		edit_select((CAL) Mpick->attrib, TRUE);

		/* Highlight the picked link chain */
		Mwork = copy_lchain(Mpick);
		widen_lchain(Mwork, 2.0);
		highlight_lchain(Mwork, 2);
		highlight_lchain_nodes(Mwork, 2, 2);
		add_item_to_metafile(TempMeta, "lchain", "l", "", "", (ITEM) Mwork);

		/* Display the link chain */
		present_temp(TRUE);
		return ModPicked;
		}
	}

/***********************************************************************
*                                                                      *
*     m e r g e _ l c h a i n                                          *
*                                                                      *
***********************************************************************/

LOGICAL	edit_merge_lchain

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
	LCHAIN	chain, copy;
	SET		set;
	CURVE	outline;
	int		nlines;
	LINE	*lines;
	int		im, imx;
	POINT	p0, p1, pz;
	MARK	mark0, mark1;
	CURVE	curv0, curv1;
	float	dx, dy, dx0, dy0, angle, cang, sang;
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

	/* Move list: */
	/* - list of link chains to be moved */
	/* - cmove are pointers into merge field sets (before merge)    */
	/*    and then copies to be displayed in TempMeta (after merge) */
	/* - ccopy are copies to be added to EditLchains                */
	static	const int	maxmove = 10000;
	static	int			nmove   = 0;
	static	int			amove   = 0;
	static	LCHAIN		*cmove  = NullLchainList;
	static	LCHAIN		*ccopy  = NullLchainList;

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
	static	LOGICAL	fullset  = FALSE;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_merge_lchain] %s  State: %d\n", mode, State);
	pr_diag("Editor", "[edit_merge_lchain]   Current edit time: %s\n",
													TimeList[EditTime].jtime);
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

							/* Empty picked link chain list */
							if (nmove > 0)
								{
								if (mstart && !maction)
									{
									for (im=0; im<nmove; im++)
										(void) remove_item_from_set(EditLchains,
															(ITEM) ccopy[im]);
									}
								else if (!maction)
									{
									for (im=0; im<nmove; im++)
										(void) destroy_lchain(ccopy[im]);
									}
								nmove = 0;
								amove = 0;
								FREEMEM(cmove);
								FREEMEM(ccopy);
								}
							edit_select(NullCal, FALSE);

							/* Get rid of picked link chains and fetched field */
							if (fullset)
								{
								mset = take_mf_set(TempMeta, "lchain", "l",
												   "mset", "mset");
								}
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

#							ifdef DEBUG_STRUCTURES
							pr_diag("Editor",
								"[edit_merge_lchain] Cancel ... empty TempMeta %d fields\n",
								TempMeta->numfld);
#							endif /* DEBUG_STRUCTURES */

							/* Empty display structure */
							empty_temp();
							if (EditLchains && EditUndoable)
								{
								highlight_set(EditLchains, 1);
								highlight_set_secondary(EditLchains, 0);
								}
							present_all();
							return FALSE;
			}
		}
	if (same(mode, "cancel all"))
		{
		ignore_partial();
		drawing_control(FALSE);
		edit_allow_preset_outline(FALSE);

		/* Empty picked link chain list */
		if (nmove > 0)
			{
			if (mstart && !maction)
				{
				for (im=0; im<nmove; im++)
					(void) remove_item_from_set(EditLchains, (ITEM) ccopy[im]);
				}
			else if (!maction)
				{
				for (im=0; im<nmove; im++) (void) destroy_lchain(ccopy[im]);
				}
			nmove = 0;
			amove = 0;
			FREEMEM(cmove);
			FREEMEM(ccopy);
			}

		/* Get rid of displayed picked link chains and fetched field */
		if (fullset)
			{
			mset = take_mf_set(TempMeta, "lchain", "l", "mset", "mset");
			}
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

#		ifdef DEBUG_STRUCTURES
		pr_diag("Editor",
			"[edit_merge_lchain] Cancel All ... empty TempMeta %d fields\n",
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

	/* Make sure the link chain set contains the right stuff */
	if (!EditLchains)                       return FALSE;
	if (!same(EditLchains->type, "lchain")) return FALSE;

	/* Initialize on startup */
	if (same(mode, "begin"))
		{
		ignore_partial();
		if (EditUndoable)
			{
			highlight_set(EditLchains, 1);
			highlight_set_secondary(EditLchains, 0);
			present_all();
			post_partial(FALSE);
			}

		/* Set up move list */
		nmove = 0;
		amove = 0;
		FREEMEM(cmove);
		FREEMEM(ccopy);

		/* Get rid of displayed picked link chains and fetched field */
		if (fullset)
			{
			mset = take_mf_set(TempMeta, "lchain", "l", "mset", "mset");
			}
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

#		ifdef DEBUG_STRUCTURES
		pr_diag("Editor",
			"[edit_merge_lchain] Begin ... empty TempMeta %d fields\n",
			TempMeta->numfld);
#		endif /* DEBUG_STRUCTURES */

		/* Empty display structure */
		empty_temp();
		present_all();
		mstart  = FALSE;
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
		put_message("lchain-no-pick");
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
					"[edit_merge_lchain] No outline drawn yet!\n");
				}
			else
				{
				put_message("edit-no-preset-outline", source);
				pr_error("Editor",
					"[edit_merge_lchain] No preset map outline: \"%s\"\n",
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
			add_item_to_metafile(TempMeta, "curve", "c", "", "",
								(ITEM) outline);
			present_temp(TRUE);

#			ifdef DEBUG_STRUCTURES
			pr_diag("Editor",
				"[edit_merge_lchain] Preset Outline ... adding outline to TempMeta (%d)\n",
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
				put_message("lchain-merge-action");
				edit_allow_preset_outline(FALSE);
				return FALSE;

			/* Fetch the requested merge field */
			case Fetch:

				fetch   = FALSE;
				mstart  = FALSE;
				maction = FALSE;

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
				pr_diag("Editor", "[edit_merge_lchain] Fetching merge field\n");
#				endif /* DEBUG */

				/* Interpret and fetch the requested field */
				put_message("merge-fetching");
				(void) init_fld_descript(&fdesc);
				dpath = get_directory("Data");
				if (same(vtime, FpaFile_Links))
					{
					if (!set_fld_descript(&fdesc,
						FpaF_MAP_PROJECTION,    MapProj,
						FpaF_DIRECTORY_PATH,    dpath,
						FpaF_SOURCE_NAME,       source,
						FpaF_SUBSOURCE_NAME,    subsrc,
						FpaF_RUN_TIME,          rtime,
						FpaF_ELEMENT_NAME,      element,
						FpaF_LEVEL_NAME,        level,
						FpaF_END_OF_LIST)) return FALSE;
					}
				else
					{
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
					}
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
				if (same(vtime, FpaFile_Links))
					mset = retrieve_linkset(&fdesc);
				else
					mset = retrieve_lchainset(&fdesc);
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
					/* No link chains to merge */
					put_message("lchain-merge-none");
					(void) sleep(2);
					State = Wait;
					continue;
					}

				/* Got it - prepare set of link chains for display */
				fullset  = TRUE;
				msource  = STRMEM(msource,  source);
				msubsrc  = STRMEM(msubsrc,  subsrc);
				mrtime   = STRMEM(mrtime,   rtime);
				mvtime   = STRMEM(mvtime,   vtime);
				melement = STRMEM(melement, element);
				mlevel   = STRMEM(mlevel,   level);
				mformat  = field_display_format(element, level);

#				ifdef DEBUG
				pr_diag("Editor",
					"[edit_merge_lchain] Merge field: %s %s  from: %s %s  at run/valid time: %s/%s\n",
					element, level, source, subsrc, rtime, vtime);
#				endif /* DEBUG */

				/* Display the fetched field */
				/* >>>>> modify field units for CurrElement/CurrLevel?? <<<<< */
				prep_set(mset, mformat);
				setup_set_presentation(mset, CurrElement, CurrLevel, "FPA");
				highlight_set(mset, 4);
				highlight_set_secondary(mset, 4);

				add_set_to_metafile(TempMeta, "l", "mset", "mset", mset);
				present_all();
				post_partial(TRUE);
				clear_message();

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_merge_lchain] Fetch ... adding mset to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Re-fetch the requested merge field */
			case ReFetch:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_lchain] ReFetch\n");
#				endif /* DEBUG */

				ignore_partial();

				/* Empty picked link chain list */
				if (nmove > 0)
					{
					for (im=0; im<nmove; im++) (void) destroy_lchain(ccopy[im]);
					nmove = 0;
					amove = 0;
					FREEMEM(cmove);
					FREEMEM(ccopy);
					}
				edit_select(NullCal, FALSE);

				/* Get rid of displayed picked link chains and fetched field */
				if (fullset)
					{
					mset = take_mf_set(TempMeta, "lchain", "l", "mset", "mset");
					}
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

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_merge_lchain] Refetch ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();
				present_all();

				/* Wait for a "fetch" */
				State = (fetch)? Fetch: Wait;
				continue;

			/* Pick link chains to merge */
			case Pick:

				/* Use the current list of link chains to pick from */
				set = find_mf_set(TempMeta, "lchain", "l", "mset", "mset");

				/* Allow specification of multiple moves */
				while (nmove < maxmove)
					{

					/* Put up a prompt and set states */
					if (nmove <= 0)
						{
						put_message("lchain-merge-pick");
						edit_select(NullCal, FALSE);
						}
					else
						{
						put_message("lchain-merge-pick2");
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
					put_message("lchain-pick");
					if (!pick_Xpoint(DnEdit, 0, p1, &butt)) continue;
					if (!inside_dn_window(DnEdit, p1))
						{
						put_message("edit-outside-map");
						(void) sleep(1);
						continue;
						}

					/* See which link chains we picked */
					chain = closest_lchain(set, p1,
											NullFloat, NullPoint, NullInt);
					if (!chain)
						{
						put_message("lchain-no-pick");
						(void) sleep(1);
						continue;
						}

					/* If link chain was already picked, un-pick it */
					for (im=0; im<nmove; im++)
						{
						if (chain == cmove[im]) break;
						}
					if (im < nmove)
						{
						put_message("lchain-unpicked");

						/* Un-highlight the picked link chain */
						/*  and remove it from the list */
						highlight_lchain(chain, 4);
						highlight_lchain_nodes(chain, 4, 4);
						widen_lchain(chain, -2.0);
						(void) destroy_lchain(ccopy[im]);
						nmove--;
						for (imx=im; imx<nmove; imx++)
							{
							cmove[imx] = cmove[imx+1];
							ccopy[imx] = ccopy[imx+1];
							}
						cmove[imx] = NullLchain;
						ccopy[imx] = NullLchain;

						/* Display remaining picked link chains */
						present_all();
						}

					/* Otherwise pick it */
					else
						{
						put_message("lchain-picked");
						nmove++;
						if (nmove > amove)
							{
							amove = nmove;
							cmove = GETMEM(cmove, LCHAIN, amove);
							ccopy = GETMEM(ccopy, LCHAIN, amove);
							}

						/* Make a copy of the picked link chain */
						ccopy[im] = copy_lchain(chain);

						/* Adjust the link chain reference time to match */
						/*  the current depiction time (if required)     */
						if ( !same(ccopy[im]->xtime, TimeList[EditTime].jtime) )
							{

#							ifdef DEBUG
							pr_diag("Editor",
								"[edit_merge_lchain] Reset reference time for %d from: %s to: %s\n",
								im, ccopy[im]->xtime, TimeList[EditTime].jtime);
#							endif /* DEBUG */

							define_lchain_reference_time(ccopy[im],
														TimeList[EditTime].jtime);

							/* Ensure "normal" node at current reference time */
							promote_lchain_node(ccopy[im], LchainNode, 0);
							}

						/* Invoke rules for nodes and re-interpolate link chain */
						define_lchain_default_attribs(ccopy[im]);
						CAL_invoke_all_lchain_lnode_rules(ccopy[im],
														CurrElement, CurrLevel);
						ccopy[im]->dointerp = TRUE;
						(void) interpolate_lchain(ccopy[im]);

						/* Highlight the picked link chain */
						/*  and add it to the list         */
						highlight_lchain(chain, 2);
						highlight_lchain_nodes(chain, 2, 2);
						widen_lchain(chain, 2.0);
						cmove[im] = chain;

						/* Display picked link chains */
						present_temp(TRUE);
						}
					}

				pr_warning("Editor",
					"[edit_merge_lchain]   End of State = Pick loop!\n");

				/* Have we picked any link chains? */
				if (nmove > 0) edit_select((CAL) cmove[0]->attrib, FALSE);
				else           edit_select(NullCal, FALSE);

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Pick all link chains to merge */
			case PickAll:

				put_message("lchain-all-picked");

				/* Un-highlight picked chains and remove them from the list */
				for (im=0; im<nmove; im++)
					{
					highlight_lchain(cmove[im], 4);
					highlight_lchain_nodes(cmove[im], 4, 4);
					widen_lchain(cmove[im], -2.0);
					(void) destroy_lchain(ccopy[im]);
					}

				/* Highlight all the link chains and add them to the list */
				/* Note that the link chain reference time is adjusted to */
				/*  match the current depiction time!                     */
				set = find_mf_set(TempMeta, "lchain", "l", "mset", "mset");
				nmove = set->num;
				if (nmove > amove)
					{
					amove = nmove;
					cmove = GETMEM(cmove, LCHAIN, amove);
					ccopy = GETMEM(ccopy, LCHAIN, amove);
					}
				for (im=0; im<nmove; im++)
					{
					chain     = (LCHAIN) set->list[im];
					ccopy[im] = copy_lchain(chain);

					/* Adjust the link chain reference time to match */
					/*  the current depiction time (if required)     */
					if ( !same(ccopy[im]->xtime, TimeList[EditTime].jtime) )
						{

#						ifdef DEBUG
						pr_diag("Editor",
							"[edit_merge_lchain] Reset reference time for %d from: %s to: %s\n",
							im, ccopy[im]->xtime, TimeList[EditTime].jtime);
#						endif /* DEBUG */

						define_lchain_reference_time(ccopy[im],
														TimeList[EditTime].jtime);

						/* Ensure "normal" type node at current reference time */
						promote_lchain_node(ccopy[im], LchainNode, 0);
						}

					/* Invoke rules for all nodes and re-interpolate link chain */
					define_lchain_default_attribs(ccopy[im]);
					CAL_invoke_all_lchain_lnode_rules(ccopy[im],
														CurrElement, CurrLevel);
					ccopy[im]->dointerp = TRUE;
					(void) interpolate_lchain(ccopy[im]);

					highlight_lchain(chain, 2);
					highlight_lchain_nodes(chain, 2, 2);
					widen_lchain(chain, 2.0);
					cmove[im] = chain;
					}

				/* Display picked link chains */
				present_temp(TRUE);

				/* Have we picked any link chains? */
				if (nmove > 0) edit_select((CAL) cmove[0]->attrib, FALSE);
				else           edit_select(NullCal, FALSE);

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Draw outline around link chains to merge */
			case Draw:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_lchain] Drawing outline\n");
#				endif /* DEBUG */

				/* Draw the outline */
				post_partial(FALSE);
				edit_allow_preset_outline(FALSE);
				put_message("lchain-merge-draw");
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
					"[edit_merge_lchain] Draw Outline ... adding outline to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = DrawPick;
				continue;

			/* Pick link chains to merge from inside drawn outline */
			case DrawPick:

				if (IsNull(outline))
					{
					edit_select(NullCal, FALSE);
					State = Pick;
					continue;
					}

#				ifdef DEBUG
				pr_diag("Editor",
					"[edit_merge_lchain] Picking link chains inside outline\n");
#				endif /* DEBUG */

				/* Pick link chains inside drawn outline */
				busy_cursor(TRUE);
				edit_allow_preset_outline(FALSE);
				put_message("lchain-merge-select");
				(void) sleep(1);

				/* Use the current list of link chains to pick from */
				set = find_mf_set(TempMeta, "lchain", "l", "mset", "mset");

				/* Loop through all link chains in set */
				for (imx=0; imx<set->num; imx++)
					{
					chain = (LCHAIN) set->list[imx];
					if (!chain) continue;

					/* Check if link chain is inside drawn outline */
					if (!lchain_inside_outline(chain, outline)) continue;

					/* Pick link chains that are not already picked */
					for (im=0; im<nmove; im++)
						{
						if (chain == cmove[im]) break;
						}
					if (im >= nmove)
						{
						put_message("lchain-picked");
						nmove++;
						if (nmove > amove)
							{
							amove = nmove;
							cmove = GETMEM(cmove, LCHAIN, amove);
							ccopy = GETMEM(ccopy, LCHAIN, amove);
							}

						/* Make a copy of the picked link chain */
						ccopy[im] = copy_lchain(chain);

						/* Adjust the link chain reference time to match */
						/*  the current depiction time (if required)     */
						if ( !same(ccopy[im]->xtime, TimeList[EditTime].jtime) )
							{

#							ifdef DEBUG
							pr_diag("Editor",
								"[edit_merge_lchain] Reset reference time for %d from: %s to: %s\n",
								im, ccopy[im]->xtime, TimeList[EditTime].jtime);
#							endif /* DEBUG */

							define_lchain_reference_time(ccopy[im],
														TimeList[EditTime].jtime);

							/* Ensure "normal" node at current reference time */
							promote_lchain_node(ccopy[im], LchainNode, 0);
							}

						/* Invoke rules for nodes and re-interpolate link chain */
						define_lchain_default_attribs(ccopy[im]);
						CAL_invoke_all_lchain_lnode_rules(ccopy[im],
														CurrElement, CurrLevel);
						ccopy[im]->dointerp = TRUE;
						(void) interpolate_lchain(ccopy[im]);

						/* Highlight the picked link chain */
						/*  and add it to the list         */
						highlight_lchain(chain, 2);
						highlight_lchain_nodes(chain, 2, 2);
						widen_lchain(chain, 2.0);
						cmove[im] = chain;

						}
					}

				/* Display picked link chains */
				present_temp(TRUE);
				busy_cursor(FALSE);

				/* Have we picked any link chains? */
				if (nmove > 0) edit_select((CAL) cmove[0]->attrib, FALSE);
				else           edit_select(NullCal, FALSE);

				/* Re-display the picked link chains */
				State = ReDisplay;
				continue;

			/* Re-pick link chains to merge */
			case RePick:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_lchain] RePick\n");
#				endif /* DEBUG */

				/* Empty picked link chain list */
				if (nmove > 0)
					{
					/* First need to un-widen the picked link chains */
					/*  and remove them from the list */
					for (im=0; im<nmove; im++)
						{
						chain = cmove[im];
						highlight_lchain(chain, 2);
						highlight_lchain_nodes(chain, 2, 2);
						widen_lchain(chain, -2.0);
						(void) destroy_lchain(ccopy[im]);
						}
					nmove = 0;
					amove = 0;
					FREEMEM(cmove);
					FREEMEM(ccopy);
					}
				edit_select(NullCal, FALSE);

				/* Remove displayed picked chains, but keep the fetched field */
				if (fullset)
					{
					mset = take_mf_set(TempMeta, "lchain", "l", "mset", "mset");
					}

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_merge_lchain] RePick ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();

				/* Re-display the fetched field */
				fullset = TRUE;
				highlight_set(mset, 4);
				highlight_set_secondary(mset, 4);
				add_set_to_metafile(TempMeta, "l", "mset", "mset", mset);
				present_all();
				post_partial(TRUE);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_merge_lchain] RePick ... adding mset to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Merge link chains in place */
			case Merge:

				/* Remove displayed fetched field (if required) */
				mstart = TRUE;
				if (fullset)
					{

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_merge_lchain] Merge ... empty TempMeta %d fields\n",
						TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */

					/* Empty display structure */
					empty_temp();
					fullset = FALSE;
					mset    = NullSet;

					/* Now add the picked link chains */
					for (im=0; im<nmove; im++)
						{
						(void) add_item_to_set(EditLchains, (ITEM) ccopy[im]);
						chain = copy_lchain(ccopy[im]);
						add_item_to_metafile(TempMeta, "lchain", "l",
											 "cmerge", "cmerge", (ITEM) chain);
						cmove[im] = chain;

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[edit_merge_lchain] Adding link chain (%d) to TempMeta (%d)\n",
							im, TempMeta->numfld);
#						endif /* DEBUG_STRUCTURES */
						}
					}

				/* Remove any marks from uncompleted actions */
				set = take_mf_set(TempMeta, "mark", "d", "", "");
				if (NotNull(set)) set = destroy_set(set);

#				ifdef DEBUG_STRUCTURES
				for (im=0; im<nmove; im++)
					pr_diag("Editor",
						"[edit_merge_lchain]   Merge: %2d ... cmove: %x\n",
					im, cmove[im]);
#				endif /* DEBUG_STRUCTURES */

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_lchain] Merging\n");
#				endif /* DEBUG */

				ignore_partial();

				if (EditUndoable) post_mod("lchains");

				put_message("edit-done");
				mstart  = FALSE;
				maction = TRUE;

				/* Re-display the picked link chains */
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
						"[edit_merge_lchain] Translate ... empty TempMeta %d fields\n",
						TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */

					/* Empty display structure */
					empty_temp();
					fullset = FALSE;
					mset    = NullSet;

					/* Now add the picked link chains */
					for (im=0; im<nmove; im++)
						{
						(void) add_item_to_set(EditLchains, (ITEM) ccopy[im]);
						chain = copy_lchain(ccopy[im]);
						add_item_to_metafile(TempMeta, "lchain", "l",
											 "mmerge", "mmerge", (ITEM) chain);
						cmove[im] = chain;

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[edit_merge_lchain] Adding link chain (%d) to TempMeta (%d)\n",
							im, TempMeta->numfld);
#						endif /* DEBUG_STRUCTURES */
						}
					}

				/* Highlight the picked link chains */
				for (im=0; im<nmove; im++)
					{
					chain = cmove[im];
					highlight_lchain(chain, 3);
					highlight_lchain_nodes(chain, 3, 3);
					}

				/* Remove any marks from uncompleted actions */
				set = take_mf_set(TempMeta, "mark", "d", "", "");
				if (NotNull(set)) set = destroy_set(set);

				/* Re-display the picked link chains */
				present_all();
				post_partial(TRUE);

#				ifdef DEBUG_STRUCTURES
				for (im=0; im<nmove; im++)
					pr_diag("Editor",
						"[edit_merge_lchain]   Translate: %2d ... cmove: %x\n",
					im, cmove[im]);
#				endif /* DEBUG_STRUCTURES */

				/* Pick a reference point */
				put_message("lchain-tran");
				if (!ready_Xpoint(DnEdit, p0, &butt)) return drawn;
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!inside_dn_window(DnEdit, p0))
					{
					put_message("lchain-tran-out1");
					(void) ignore_Xpoint();
					continue;
					}

				/* Highlight the link chains to translate */
				set = copy_mf_set(TempMeta, "lchain", "l", "mmerge", "mmerge");
				for (im=0; im<set->num; im++)
					{
					chain = (LCHAIN) set->list[im];
					highlight_lchain(chain, 2);
					highlight_lchain_nodes(chain, 2, 2);
					widen_lchain(chain, 2.0);
					}
				prep_set(set, mformat);

				/* Translate the reference point */
				put_message("lchain-tran-release");
				(void) utrack_Xpoint(DnEdit, set, p0, p1, &butt);
				set = destroy_set(set);
				if (!inside_dn_window(DnEdit, p1))
					{
					put_message("lchain-tran-out2");
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
					"[edit_merge_lchain] Adding mark/mark/curve to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = TranslateDone;
				continue;

			/* Translate the link chains */
			case TranslateDone:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_lchain] Translating\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);

#				ifdef DEBUG_STRUCTURES
				for (im=0; im<nmove; im++)
					pr_diag("Editor",
						"[edit_merge_lchain]   Translating: %2d ... cmove: %x\n",
					im, cmove[im]);
#				endif /* DEBUG_STRUCTURES */

				/* Translate the link chains */
				dx = p1[X] - p0[X];
				dy = p1[Y] - p0[Y];
				for (im=0; im<nmove; im++)
					{
					chain = ccopy[im];
					translate_lchain(chain, dx, dy);
					}

				if (EditUndoable) post_mod("lchains");

				put_message("edit-done");
				mstart  = FALSE;
				maction = TRUE;
				busy_cursor(FALSE);

				/* Re-display the picked link chains */
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
						"[edit_merge_lchain] Rotate ... empty TempMeta %d fields\n",
						TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */

					/* Empty display structure */
					empty_temp();
					fullset = FALSE;
					mset    = NullSet;

					/* Now add the picked link chains */
					for (im=0; im<nmove; im++)
						{
						(void) add_item_to_set(EditLchains, (ITEM) ccopy[im]);
						chain = copy_lchain(ccopy[im]);

						add_item_to_metafile(TempMeta, "lchain", "l",
											 "mmerge", "mmerge", (ITEM) chain);
						cmove[im] = chain;

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[edit_merge_lchain] Adding link chain (%d) to TempMeta (%d)\n",
							im, TempMeta->numfld);
#						endif /* DEBUG_STRUCTURES */
						}
					}

				/* Highlight the picked link chains */
				for (im=0; im<nmove; im++)
					{
					chain = cmove[im];
					highlight_lchain(chain, 3);
					highlight_lchain_nodes(chain, 3, 3);
					}

				/* Remove any marks from uncompleted actions */
				set = take_mf_set(TempMeta, "mark", "d", "", "");
				if (NotNull(set)) set = destroy_set(set);

				/* Re-display the link chains */
				present_all();
				post_partial(TRUE);

#				ifdef DEBUG_STRUCTURES
				for (im=0; im<nmove; im++)
					pr_diag("Editor",
						"[edit_merge_lchain]   Rotate: %2d ... cmove: %x\n",
					im, cmove[im]);
#				endif /* DEBUG_STRUCTURES */

				/* Pick the centre of rotation */
				put_message("lchain-rot-centre");
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
					put_message("lchain-rot-out");
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
					"[edit_merge_lchain] Adding mark to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = Rotate;
				continue;

			/* Pick and rotate a reference point */
			case Rotate:

				/* Pick a reference point */
				put_message("lchain-rot");
				if (!ready_Xpoint(DnEdit, p0, &butt)) return drawn;
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!inside_dn_window(DnEdit, p0))
					{
					put_message("lchain-rot-out1");
					(void) ignore_Xpoint();
					continue;
					}
				if (point_dist(Cpos, p0) < SplineRes)
					{
					put_message("lchain-rot-ref");
					(void) ignore_Xpoint();
					(void) sleep(1);
					continue;
					}

				/* Highlight the link chains to rotate */
				set = copy_mf_set(TempMeta, "lchain", "l", "mmerge", "mmerge");
				for (im=0; im<set->num; im++)
					{
					chain = (LCHAIN) set->list[im];
					highlight_lchain(chain, 2);
					highlight_lchain_nodes(chain, 2, 2);
					widen_lchain(chain, 2.0);
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
					"[edit_merge_lchain] Adding mark/curve to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Rotate the reference point */
				put_message("lchain-rot-release");
				(void) urotate_Xpoint(DnEdit, set, Cpos, p0, p1, &Ang, &butt);
				set = destroy_set(set);
				if (!inside_dn_window(DnEdit, p1))
					{
					put_message("lchain-rot-out2");
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
					"[edit_merge_lchain] Adding mark/curve to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = RotateDone;
				continue;

			/* Rotate the link chains */
			case RotateDone:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_lchain] Rotating\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);

#				ifdef DEBUG_STRUCTURES
				for (im=0; im<nmove; im++)
					pr_diag("Editor",
						"[edit_merge_lchain]   Rotating: %2d ... cmove: %x\n",
					im, cmove[im]);
#				endif /* DEBUG_STRUCTURES */

				/* Rotate the link chains */
				dx0 = p0[X] - Cpos[X];
				dy0 = p0[Y] - Cpos[Y];
				for (im=0; im<nmove; im++)
					{
					chain = ccopy[im];
					rotate_lchain(chain, Cpos, Ang/RAD);
					}

				if (EditUndoable) post_mod("lchains");

				put_message("edit-done");
				mstart  = FALSE;
				maction = TRUE;
				busy_cursor(FALSE);

				/* Re-display the picked link chains */
				State = ReDisplayAction;
				continue;

			/* Re-display the picked link chains */
			case ReDisplay:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_lchain] Redisplay\n");
#				endif /* DEBUG */

				/* Keep the fetched field (including picked link chains) */
				if (fullset)
					{
					mset = take_mf_set(TempMeta, "lchain", "l", "mset", "mset");
					}

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_merge_lchain] ReDisplay ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();

				/* Re-display the fetched field (including picked link chains) */
				if (NotNull(mset))
					{
					add_set_to_metafile(TempMeta, "l", "mset", "mset", mset);
					}
				present_all();
				post_partial(TRUE);

#				ifdef DEBUG_STRUCTURES
				if (NotNull(mset))
					pr_diag("Editor",
						"[edit_merge_lchain] ReDisplay ... adding mset to TempMeta (%d)\n",
						TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Re-display the picked link chains (after merging) */
			case ReDisplayAction:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_lchain] RedisplayAction\n");
#				endif /* DEBUG */

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_merge_lchain] ReDisplayAction ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();

				/* Re-pick the picked link chains */
				for (im=0; im<nmove; im++)
					{

					/* Highlight the picked link chain */
					copy = copy_lchain(ccopy[im]);
					highlight_lchain(copy, 2);
					highlight_lchain_nodes(copy, 2, 2);
					widen_lchain(copy, 2.0);

					add_item_to_metafile(TempMeta, "lchain", "l",
										 "mmerge", "mmerge", (ITEM) copy);
					cmove[im] = copy;

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_merge_lchain] Adding link chain (%d) to TempMeta (%d)\n",
						im, TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */
					}

				/* Re-display the picked link chains */
				present_all();
				post_partial(FALSE);

				/* Move on to next stage */
				State = WaitAction;
				continue;

			/* Start again */
			case ReStart:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_lchain] ReStart\n");
#				endif /* DEBUG */

				/* Empty picked link chain list */
				if (nmove > 0)
					{
					for (im=0; im<nmove; im++)
						ccopy[im] = remove_item_from_set(EditLchains,
														 (ITEM) ccopy[im]);
					nmove = 0;
					amove = 0;
					FREEMEM(cmove);
					FREEMEM(ccopy);
					}
				edit_select(NullCal, FALSE);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_merge_lchain] ReStart ... empty TempMeta %d fields\n",
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
*     e d i t _ n o d e s _ l c h a i n                                *
*                                                                      *
***********************************************************************/

typedef	enum
			{
			PickChain, RePickChain,				/* choose chain states */
			PickNode, PickNodeAmbig,
			MoveNode, CopyNode, PasteNode,		/* "action" states */
			ModifyNode, DeleteNode, ShowNode,
			SetNode, DisplayChain,				/* notification states */
			Confirm								/* complete changes */
			}
		NODESTATE;

static	NODESTATE	node_lchain_pick(void);
static	void		clear_node_sample(void);

static	LCHAIN	Lpick   = NullLchain;	/* Picked link chain */
static	LCHAIN	Lwork   = NullLchain;	/* Working copy of picked link chain */
static	LNODE	NLpick  = NullLnode;	/* Picked link node */
static	LINTERP	NIpick  = NullLinterp;	/* Picked interpolated node */
static	LOGICAL	Nfresh  = FALSE;		/* First link node adjustment */
static	LOGICAL	Nadjust = FALSE;		/* Link node adjusted */
static	LOGICAL	Nmodify = FALSE;		/* Link node being modified */

LOGICAL	edit_nodes_lchain

	(
	STRING	mode,
	STRING	ntype,
	CAL		cal
	)

	{
	NODESTATE	next;
	LMEMBER		ctype;
	LNODE		lnode, cnode;
	LINTERP		linterp;
	POINT		pos, lpos, fpos, p0, p1;
	int			mplus, lplus, ldiff, ilnode, fplus, fdiff, ifnode, jnode;
	ATTRIB_LIST	attrib, xattrib;
	float		dx, dy;
	STRING		cval, vtime;
	CURVE		curve;
	MARK		mark, mark0, mark1;
	int			butt;
	LOGICAL		drawn = FALSE;

	static	POINT	opos    = ZERO_POINT;
	static	POINT	npos    = ZERO_POINT;
	static	LMEMBER	ltype   = LchainUnknown;
	static	int		inode   = -1;
	static	int		anum    =  0;
	static	int		*anodes = NullInt;
	static	STRING	atype   = NullString;

	/* Current state: */
	/* - where should we resume? */
	static	NODESTATE	State = PickChain;
	static	NODESTATE	ActionState = MoveNode;

#	ifdef DEBUG
	pr_diag("Editor",
		"[edit_nodes_lchain] \"%s\" \"%s\"  State: %d  ActionState: %d\n",
		mode, ntype, State, ActionState);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same(mode, "cancel"))
		{
		switch (State)
			{
			case PickNode:
			case SetNode:		State = RePickChain;
								break;

			case PickNodeAmbig:	empty_temp();
								edit_select_node(NullCal, FALSE);
								(void) release_ambiguous_nodes();
								State = DisplayChain;
								break;

			case MoveNode:
			case CopyNode:
			case PasteNode:
			case ModifyNode:
			case DeleteNode:	empty_temp();
								edit_select_node(NullCal, FALSE);
								if (copy_posted("lnodes", CurrElement, CurrLevel))
									edit_can_paste(TRUE);
								State = DisplayChain;
								break;

			default:			empty_temp();
								if (EditLchains && EditUndoable)
									{
									highlight_set(EditLchains, 1);
									highlight_set_secondary(EditLchains, 0);
									}
								edit_select_node(NullCal, FALSE);
								if (copy_posted("lnodes", CurrElement, CurrLevel))
									edit_can_paste(TRUE);
								present_all();
								return FALSE;
			}
		}
	if (same(mode, "cancel all"))
		{
		ignore_partial();

		/* Clean up temporary buffers */
		edit_select_node(NullCal, FALSE);
		edit_can_paste(FALSE);
		(void) release_ambiguous_nodes();
		Lpick   = NullLchain;
		Lwork   = NullLchain;
		NLpick  = NullLnode;
		NIpick  = NullLinterp;
		Nfresh  = FALSE;
		Nadjust = FALSE;
		Nmodify = FALSE;
		atype   = NullString;

		/* Empty display structure */
		empty_temp();

		return FALSE;
		}
	if (same(mode, "clear"))
		{
		(void) clear_node_sample();
		sample_box(FALSE, NullCal);
		edit_can_clear(FALSE);
		return FALSE;
		}

	/* Make sure the link chain set contains the right stuff */
	if (!EditLchains)                       return FALSE;
	if (!same(EditLchains->type, "lchain")) return FALSE;

	/* Initialize on startup */
	if (same(mode, "begin"))
		{
		ignore_partial();
		if (EditUndoable)
			{
			highlight_set(EditLchains, 1);
			highlight_set_secondary(EditLchains, 0);
			present_all();
			post_partial(FALSE);
			}
		Nfresh  = TRUE;
		Nadjust = FALSE;
		Nmodify = FALSE;
		atype   = NullString;
		edit_select_node(NullCal, FALSE);
		edit_can_paste(FALSE);
		State = PickChain;
		}

	/* Clean up if the action state has changed */
	if (       (same(mode, "move")   && ActionState != MoveNode)
			|| (same(mode, "copy")   && ActionState != CopyNode)
			|| (same(mode, "paste")  && ActionState != PasteNode)
			|| (same(mode, "modify") && ActionState != ModifyNode)
			|| (same(mode, "delete") && ActionState != DeleteNode)
			|| (same(mode, "show")   && ActionState != ShowNode) )
		{

#		ifdef DEBUG
		pr_diag("Editor",
			"[edit_nodes_lchain] Changing Action State: %d  to: %s\n",
			ActionState, mode);
#		endif /* DEBUG */

		if (Nadjust)
			{
			ignore_partial();
			if (EditUndoable) accept_mod();
			}
		(void) clear_node_sample();
		(void) release_ambiguous_nodes();
		Nfresh  = TRUE;
		Nadjust = FALSE;
		Nmodify = FALSE;
		atype   = NullString;
		edit_select_node(NullCal, FALSE);
		if (Lpick) post_partial(TRUE);
		}

	/* Set the action state */
	/* Return to this state after link chain is picked! */
	if (same(mode, "move"))
		{
		State       = (Lpick)? PickNode: PickChain;
		ActionState = MoveNode;
		}
	if (same(mode, "copy"))
		{
		State       = (Lpick)? PickNode: PickChain;
		ActionState = CopyNode;
		}
	if (same(mode, "paste"))
		{
		State       = (Lpick)? PickNode: PickChain;
		ActionState = PasteNode;
		}
	if (same(mode, "modify"))
		{
		if (Nmodify) State = ModifyNode;
		else         State = (Lpick)? PickNode: PickChain;
		ActionState = ModifyNode;
		}
	if (same(mode, "delete"))
		{
		State       = (Lpick)? PickNode: PickChain;
		ActionState = DeleteNode;
		}
	if (same(mode, "show"))
		{
		State       = (Lpick)? PickNode: PickChain;
		ActionState = ShowNode;
		}

	/* Take care of setting new value */
	if (same(mode, "set") && NotNull(cal)) State = SetNode;

	/* Set state for Right button "choose_lchain" */
	if (same(mode, "choose_chain"))
		{
		(void) release_ambiguous_nodes();
		State = (Lpick)? RePickChain: PickChain;
		}

#	ifdef DEBUG
	pr_diag("Editor",
		"[edit_nodes_lchain]       State: %d  ActionState: %d\n",
		State, ActionState);
#	endif /* DEBUG */

	/* Repeat until told to quit */
	while (TRUE)
		{

		/* Resume at current stage */
		switch (State)
			{

			/* Pick a link chain */
			case PickChain:

				/* Pick a link chain */
				ignore_partial();
				edit_can_paste(FALSE);
				next = node_lchain_pick();
				if (next == PickChain) return drawn;

				/* Move on to next stage */
				State = next;
				continue;

			/* Re-pick a link chain */
			case RePickChain:

				put_message("edit-cancel");

				/* Clean up temporary buffers */
				ignore_partial();
				edit_select_node(NullCal, FALSE);
				Lpick   = NullLchain;
				Lwork   = NullLchain;
				NLpick  = NullLnode;
				NIpick  = NullLinterp;
				Nfresh  = FALSE;
				Nadjust = FALSE;
				Nmodify = FALSE;
				atype   = NullString;

				/* Empty display structure */
				empty_temp();

				/* Move on to next stage */
				State = PickChain;
				continue;

			/* Pick a link node */
			case PickNode:

				/* Can the node information be pasted? */
				if (copy_posted("lnodes", CurrElement, CurrLevel))
					edit_can_paste(TRUE);

				/* Pick a link chain node to move */
				if (ActionState == MoveNode)
					{
					put_message("lchain-node-move-pick");
					if (!ready_Xpoint(DnEdit, opos, &butt)) return drawn;
					if (butt != LeftButton)
						{
						put_message("edit-wrong-button");
						(void) ignore_Xpoint();
						continue;
						}
					if (!inside_dn_window(DnEdit, opos))
						{
						put_message("edit-outside-map");
						(void) ignore_Xpoint();
						continue;
						}
					}

				/* Pick a link chain node to copy or paste or modify or delete */
				else if (ActionState == CopyNode
						|| ActionState == PasteNode
						|| ActionState == ModifyNode
						|| ActionState == DeleteNode)
					{
					if      (ActionState == CopyNode)
						put_message("lchain-node-copy-pick");
					else if (ActionState == PasteNode)
						put_message("lchain-node-paste-pick");
					else if (ActionState == ModifyNode)
						put_message("lchain-node-mod-pick");
					else if (ActionState == DeleteNode)
					put_message("lchain-node-del-pick");
					if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return drawn;
					if (butt != LeftButton)
						{
						put_message("edit-wrong-button");
						(void) ignore_Xpoint();
						continue;
						}
					put_message("lchain-pick-node-rel");
					(void) pick_Xpoint(DnEdit, 0, opos, &butt);
					if (!inside_dn_window(DnEdit, opos))
						{
						put_message("edit-outside-map");
						(void) ignore_Xpoint();
						continue;
						}
					}

				/* Pick a link chain node to show */
				else if (ActionState == ShowNode)
					{
					put_message("lchain-node-show-pick");
					if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return drawn;
					if (butt != LeftButton)
						{
						put_message("edit-wrong-button");
						(void) ignore_Xpoint();
						continue;
						}

					/* Clear previous display */
					(void) clear_node_sample();

					/* Pick the new link node */
					put_message("lchain-pick-node-rel");
					(void) pick_Xpoint(DnEdit, 0, opos, &butt);
					if (!inside_dn_window(DnEdit, opos))
						{
						put_message("edit-outside-map");
						(void) ignore_Xpoint();
						continue;
						}
					}

				/* Check which type of node was picked */
				inode = lchain_closest_node(Lpick, 0.0, PickTol/100.0,
							opos, NullFloat, npos, &ltype, &anum, &anodes);

#				ifdef DEBUG
				pr_diag("Editor",
					"[edit_nodes_lchain] Pick - ltype: %d  inode: %d  anum: %d\n",
					ltype, inode, anum);
#				endif /* DEBUG */

				/* Is this the last node for delete node */
				if (ActionState == DeleteNode && Lpick->lnum <= 1)
					{
					State = ActionState;
					continue;
					}

				/* Must pick a time for ambiguous link nodes */
				if (anum > 1)
					{
					State = PickNodeAmbig;
					continue;
					}

				/* Unknown node */
				if (ltype == LchainUnknown || inode < 0)
					{
					put_message("lchain-no-node-picked");
					(void) sleep(1);
					(void) ignore_Xpoint();
					continue;
					}

				/* Floating nodes and interpolated nodes cannot be moved! */
				if (ActionState == MoveNode)
					{
					if (ltype == LchainFloating)
						{
						put_message("lchain-no-move-float");
						(void) sleep(1);
						(void) ignore_Xpoint();
						continue;
						}
					else if (ltype == LchainInterp)
						{
						put_message("lchain-no-move-interp");
						(void) sleep(1);
						(void) ignore_Xpoint();
						continue;
						}
					}

				/* Control nodes and interpolated nodes cannot be copied! */
				else if (ActionState == CopyNode)
					{
					if (ltype == LchainControl)
						{
						put_message("lchain-no-copy-control");
						(void) sleep(1);
						(void) ignore_Xpoint();
						continue;
						}
					else if (ltype == LchainInterp)
						{
						put_message("lchain-no-copy-interp");
						(void) sleep(1);
						(void) ignore_Xpoint();
						continue;
						}
					}

				/* Interpolated nodes cannot be deleted! */
				else if (ActionState == DeleteNode)
					{
					if (ltype == LchainInterp)
						{
						put_message("lchain-no-del-interp");
						(void) sleep(1);
						(void) ignore_Xpoint();
						continue;
						}
					}

				/* Set the node attributes for modifying */
				if (ActionState == ModifyNode)
					{

					/* Set the attributes for entry menus         */
					/* Note that attributes for control nodes and */
					/*  interpolated nodes cannot be modified!    */
					if      (ltype == LchainNode)
						{
						attrib = Lpick->nodes[inode]->attrib;
						edit_select_node((CAL) attrib, TRUE);
						}
					else if (ltype == LchainControl)
						{
						attrib = Lpick->nodes[inode]->attrib;
						edit_select_node((CAL) attrib, FALSE);
						}
					else if (ltype == LchainFloating)
						{
						attrib = Lpick->nodes[inode]->attrib;
						edit_select_node((CAL) attrib, TRUE);
						}
					else if (ltype == LchainInterp)
						{
						attrib = Lpick->interps[inode]->attrib;
						edit_select_node((CAL) attrib, FALSE);
						}

					/* Set the node type */
					cval = CAL_get_attribute((CAL) attrib, AttribLnodeType);
					if (same(cval, FpaNodeClass_Normal))
						atype = FpaNodeClass_Normal;
					else if (same(cval, FpaNodeClass_Control))
						atype = FpaNodeClass_Control;
					else if (same(cval, FpaNodeClass_Floating))
						atype = FpaNodeClass_Floating;
					else if (same(cval, FpaNodeClass_Interp))
						atype = FpaNodeClass_Interp;
					else
						{
						put_message("lchain-node-unknown", cval);
						(void) sleep(1);
						(void) ignore_Xpoint();
						continue;
						}
					}

				/* Highlight the selected node */
				if      (ltype == LchainNode)
					highlight_lnode(Lwork->nodes[inode], 2, 2);
				else if (ltype == LchainControl)
					highlight_lnode(Lwork->nodes[inode], 2, 2);
				else if (ltype == LchainFloating)
					highlight_lnode(Lwork->nodes[inode], 2, 2);
				else if (ltype == LchainInterp)
					highlight_linterp(Lwork->interps[inode], 2, 2);
				present_temp(TRUE);

				/* Go on to the next stage */
				drawn = TRUE;
				State = ActionState;
				continue;

			/* Pick an ambiguous link node */
			case PickNodeAmbig:

				/* Display the link nodes */
				(void) build_ambiguous_nodes(Lpick, LINK_INT, anum, anodes,
							0, npos);
				put_message("lchain-node-pick-ambig");

				/* Pick time for link node */
				/* Note that the times on the panel are set from existing */
				/*  interpolated times ... so inode will always exist     */
				if (!ready_Xpoint(DnEdit, NULL, &butt)) return drawn;
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) release_ambiguous_nodes();
					(void) ignore_Xpoint();
					continue;
					}
				put_message("lchain-pick-node-rel");
				(void) pick_Xpoint(DnEdit, 0, pos, &butt);
				if (!pick_ambiguous_node(pos, &mplus))
					{
					put_message("edit-outside-panel");
					(void) sleep(1);
					(void) release_ambiguous_nodes();
					(void) ignore_Xpoint();
					continue;
					}

				/* Release the panel */
				(void) release_ambiguous_nodes();

				/* Set type of link node picked */
				inode = which_lchain_node(Lpick, LchainNode, mplus);
				if (inode >= 0 && Lpick->nodes[inode]->there)
					{
					ltype = LchainNode;
					}
				else
					{
					inode = which_lchain_node(Lpick, LchainControl, mplus);
					if (inode >= 0 && Lpick->nodes[inode]->there)
						{
						ltype = LchainControl;
						}
					else
						{
						inode = which_lchain_node(Lpick, LchainFloating, mplus);
						if (inode >= 0 && Lpick->nodes[inode]->there)
							{
							ltype = LchainFloating;
							}

						else
							{
							inode = which_lchain_node(Lpick, LchainInterp, mplus);
							ltype = LchainInterp;
							}
						}
					}

#				ifdef DEBUG
				pr_diag("Editor",
					"[edit_nodes_lchain] Pick Ambig - mplus: %d  inode: %d\n",
					mplus, inode);
#				endif /* DEBUG */

				/* Unknown node */
				if (ltype == LchainUnknown || inode < 0)
					{
					put_message("lchain-no-node-picked");
					(void) sleep(1);
					(void) ignore_Xpoint();
					continue;
					}

				/* Floating nodes and interpolated nodes cannot be moved! */
				if (ActionState == MoveNode)
					{
					if (ltype == LchainFloating)
						{
						put_message("lchain-no-move-float");
						(void) sleep(1);
						(void) ignore_Xpoint();
						continue;
						}
					else if (ltype == LchainInterp)
						{
						put_message("lchain-no-move-interp");
						(void) sleep(1);
						(void) ignore_Xpoint();
						continue;
						}
					}

				/* Control nodes and interpolated nodes cannot be copied! */
				else if (ActionState == CopyNode)
					{
					if (ltype == LchainControl)
						{
						put_message("lchain-no-copy-control");
						(void) sleep(1);
						(void) ignore_Xpoint();
						continue;
						}
					else if (ltype == LchainInterp)
						{
						put_message("lchain-no-copy-interp");
						(void) sleep(1);
						(void) ignore_Xpoint();
						continue;
						}
					}

				/* Interpolated nodes cannot be deleted! */
				else if (ActionState == DeleteNode)
					{
					if (ltype == LchainInterp)
						{
						put_message("lchain-no-del-interp");
						(void) sleep(1);
						(void) ignore_Xpoint();
						continue;
						}
					}

				/* Set the node attributes for modifying */
				if (ActionState == ModifyNode)
					{

					/* Set the attributes for entry menus         */
					/* Note that attributes for control nodes and */
					/*  interpolated nodes cannot be modified!    */
					if      (ltype == LchainNode)
						{
						attrib = Lpick->nodes[inode]->attrib;
						edit_select_node((CAL) attrib, TRUE);
						}
					else if (ltype == LchainControl)
						{
						attrib = Lpick->nodes[inode]->attrib;
						edit_select_node((CAL) attrib, FALSE);
						}
					else if (ltype == LchainFloating)
						{
						attrib = Lpick->nodes[inode]->attrib;
						edit_select_node((CAL) attrib, TRUE);
						}
					else if (ltype == LchainInterp)
						{
						attrib = Lpick->interps[inode]->attrib;
						edit_select_node((CAL) attrib, FALSE);
						}

					/* Set the node type */
					cval = CAL_get_attribute((CAL) attrib, AttribLnodeType);
					if (same(cval, FpaNodeClass_Normal))
						atype = FpaNodeClass_Normal;
					else if (same(cval, FpaNodeClass_Control))
						atype = FpaNodeClass_Control;
					else if (same(cval, FpaNodeClass_Floating))
						atype = FpaNodeClass_Floating;
					else if (same(cval, FpaNodeClass_Interp))
						atype = FpaNodeClass_Interp;
					else
						{
						put_message("lchain-node-unknown", cval);
						(void) sleep(1);
						(void) ignore_Xpoint();
						continue;
						}
					}

				/* Highlight the selected node */
				if      (ltype == LchainNode)
					highlight_lnode(Lwork->nodes[inode], 2, 2);
				else if (ltype == LchainControl)
					highlight_lnode(Lwork->nodes[inode], 2, 2);
				else if (ltype == LchainFloating)
					highlight_lnode(Lwork->nodes[inode], 2, 2);
				else if (ltype == LchainInterp)
					highlight_linterp(Lwork->interps[inode], 2, 2);
				present_temp(TRUE);

				/* Go on to the next stage */
				drawn = TRUE;
				State = ActionState;
				continue;

			/* Move a link node */
			case MoveNode:

				/* Freeze all pending edits so only this one can be undone */
				if (Nfresh && EditUndoable)
					{
					ignore_partial();
					put_message("edit-freeze");
					accept_mod();	/* Leaves EditLchains alone */
					}
				post_partial(TRUE);

				/* Set time for node to move */
				if      (ltype == LchainNode)
					mplus = Lpick->nodes[inode]->mplus;
				else if (ltype == LchainControl)
					mplus = Lpick->nodes[inode]->mplus;

				/* Determine previous node location and time difference */
				lplus  = mplus - Lpick->minterp;
				ldiff  =  0;
				ilnode = -1;
				while ( lplus >= Lpick->splus )
					{
					ldiff  = mplus - lplus;
					ilnode = which_lchain_node(Lpick, LchainNode, lplus);
					if ( ilnode >= 0 )
						{
						(void) copy_point(lpos, Lpick->nodes[ilnode]->node);
						break;
						}
					ilnode = which_lchain_node(Lpick, LchainControl, lplus);
					if ( ilnode >= 0 )
						{
						(void) copy_point(lpos, Lpick->nodes[ilnode]->node);
						break;
						}
					lplus -= Lpick->minterp;
					}

				/* Determine following node location and time difference */
				fplus  = mplus + Lpick->minterp;
				fdiff  =  0;
				ifnode = -1;
				while ( fplus <= Lpick->eplus )
					{
					fdiff  = fplus - mplus;
					ifnode = which_lchain_node(Lpick, LchainNode, fplus);
					if ( ifnode >= 0 )
						{
						(void) copy_point(fpos, Lpick->nodes[ifnode]->node);
						break;
						}
					ifnode = which_lchain_node(Lpick, LchainControl, fplus);
					if ( ifnode >= 0 )
						{
						(void) copy_point(fpos, Lpick->nodes[ifnode]->node);
						break;
						}
					fplus += Lpick->minterp;
					}

				/* Translate the node location */
				put_message("lchain-node-move-rel");
				if (!ready_Xpoint(DnEdit, NULL, &butt)) return drawn;
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if ( ilnode >= 0 && ifnode >= 0 )
					(void) utrack_Xnode_move(DnEdit, lpos, ldiff, npos,
												fpos, fdiff, p0, p1, &butt);
				else if ( ilnode >= 0 )
					(void) utrack_Xnode_move(DnEdit, lpos, ldiff, npos,
												NullPoint, fdiff, p0, p1, &butt);
				else if ( ifnode >= 0 )
					(void) utrack_Xnode_move(DnEdit, NullPoint, ldiff, npos,
												fpos, fdiff, p0, p1, &butt);
				else
					(void) utrack_Xnode_move(DnEdit, NullPoint, ldiff, npos,
												NullPoint, fdiff, p0, p1, &butt);
				if (!inside_dn_window(DnEdit, p1))
					{
					put_message("lchain-node-move-out");
					(void) sleep(1);
					(void) ignore_Xpoint();
					continue;
					}

				/* Translate the node */
				dx = p1[X] - p0[X];
				dy = p1[Y] - p0[Y];
				set_point(pos, npos[X] + dx, npos[Y] + dy);

				/* Display markers */
				mark0 = create_mark("", "", "", npos, 0.0);
				mark1 = create_mark("", "", "",  pos, 0.0);
				curve = create_curve("", "", "");
				add_point_to_curve(curve, npos);
				add_point_to_curve(curve,  pos);
				define_mspec(&mark0->mspec, 0, 2, NULL, 0, False, 4.0, 0.0,
							 (HILITE) 2);
				define_mspec(&mark1->mspec, 0, 2, NULL, 0, False, 4.0, 0.0,
							 (HILITE) 2);
				define_lspec(&curve->lspec, 0, 0, NULL, False, 2.0, 0.0,
							 (HILITE) 2);

				add_item_to_metafile(TempMeta, "mark",  "d", "", "",
									 (ITEM) mark0);
				add_item_to_metafile(TempMeta, "mark",  "d", "", "",
									 (ITEM) mark1);
				add_item_to_metafile(TempMeta, "curve", "c", "", "",
									 (ITEM) curve);
				present_temp(TRUE);

				/* Move the node */
				if (ltype == LchainNode)
					{
					lnode = Lpick->nodes[inode];
					define_lnode_node(lnode, pos);
					}
				else if (ltype == LchainControl)
					{
					lnode = Lpick->nodes[inode];
					define_lnode_node(lnode, pos);
					}

				/* Reinterpolate modified link chain */
				Lpick->dointerp = TRUE;
				(void) interpolate_lchain(Lpick);

				/* Confirm the modification */
				drawn = TRUE;
				State = Confirm;
				continue;

			/* Copy the attributes of one link node to paste to another */
			case CopyNode:

				/* Save the link node in the copy buffer                      */
				/* Note that control and interpolated nodes cannot be copied! */
				put_message("lchain-node-copied");
				(void) sleep(1);
				if (ltype == LchainNode)
					{
					post_lchain_node_copy(CurrElement, CurrLevel,
											ltype, Lpick->nodes[inode]);
					}
				else if (ltype == LchainFloating)
					{
					post_lchain_node_copy(CurrElement, CurrLevel,
											ltype, Lpick->nodes[inode]);
					}

				/* Clean up temporary buffers */
				Lwork   = NullLchain;
				Nfresh  = FALSE;
				Nadjust = TRUE;
				Nmodify = FALSE;

				/* Empty display structure */
				empty_temp();

				/* Redisplay the picked link chain */
				State = DisplayChain;
				continue;

			/* Paste the parameters of one link node to another */
			case PasteNode:

				/* Retrieve paste buffer */
				paste_lchain_node_copy(CurrElement, CurrLevel, &ctype, &cnode);

				/* Extract the attributes                                     */
				/* Note that control and interpolated nodes cannot be copied! */
				if      (ctype == LchainNode)     attrib = cnode->attrib;
				else if (ctype == LchainFloating) attrib = cnode->attrib;

				/* Change the picked node based on the paste buffer */
				put_message("lchain-node-pasted");
				(void) sleep(1);

				/* Replace attributes for link nodes */
				if (ltype == LchainNode)
					{
					lnode = Lpick->nodes[inode];
					define_lnode_attribs(lnode, attrib);
					CAL_invoke_lnode_rules_by_name((CAL) lnode->attrib,
														CurrElement, CurrLevel);

#					ifdef DEBUG
					if (ctype == LchainNode)
						pr_diag("Editor",
							"[edit_nodes_lchain] Paste link node - overwrite with link node attributes\n");
					else if (ctype == LchainFloating)
						pr_diag("Editor",
							"[edit_nodes_lchain] Paste link node - overwrite with floating node attributes\n");
#					endif /* DEBUG */
					}

				/* Promote control node to link node */
				else if (ltype == LchainControl)
					{
					mplus = Lpick->nodes[inode]->mplus;
					promote_lchain_node(Lpick, LchainNode, mplus);
					lnode = Lpick->nodes[inode];
					define_lnode_attribs(lnode, attrib);
					CAL_invoke_lnode_rules_by_name((CAL) lnode->attrib,
														CurrElement, CurrLevel);

#					ifdef DEBUG
					if (ctype == LchainNode)
						pr_diag("Editor",
							"[edit_nodes_lchain] Paste control node - overwrite with link node attributes\n");
					else if (ctype == LchainFloating)
						pr_diag("Editor",
							"[edit_nodes_lchain] Paste control node - overwrite with floating node attributes\n");
#					endif
					}

				/* Replace attributes for floating nodes */
				else if (ltype == LchainFloating)
					{
					lnode = Lpick->nodes[inode];
					define_lnode_attribs(lnode, attrib);
					CAL_invoke_lnode_rules_by_name((CAL) lnode->attrib,
														CurrElement, CurrLevel);

#					ifdef DEBUG
					if (ctype == LchainNode)
						pr_diag("Editor",
							"[edit_nodes_lchain] Paste floating node - overwrite with link node attributes\n");
					else if (ctype == LchainFloating)
						pr_diag("Editor",
							"[edit_nodes_lchain] Paste floating node - overwrite with floating node attributes\n");
#					endif /* DEBUG */
					}

				/* Promote interpolated node to link node or floating node */
				else if (ltype == LchainInterp)
					{

					/* Promote interpolated node at this time */
					mplus = Lpick->interps[inode]->mplus;
					if (ctype == LchainNode)
						promote_lchain_node(Lpick, LchainNode, mplus);
					else if (ctype == LchainFloating)
						promote_lchain_node(Lpick, LchainFloating, mplus);

					/* Re-set attributes for new link node at this time */
					if (ctype == LchainNode)
						{
						jnode = which_lchain_node(Lpick, LchainNode, mplus);
						if (jnode >= 0)
							{
							lnode = Lpick->nodes[jnode];
							define_lnode_attribs(lnode, attrib);
							CAL_invoke_lnode_rules_by_name((CAL) lnode->attrib,
														CurrElement, CurrLevel);
							}
						}

					/* Re-set attributes for new floating node at this time */
					else if (ctype == LchainFloating)
						{
						jnode = which_lchain_node(Lpick, LchainFloating, mplus);
						if (jnode >= 0)
							{
							lnode = Lpick->nodes[jnode];
							define_lnode_attribs(lnode, attrib);
							CAL_invoke_lnode_rules_by_name((CAL) lnode->attrib,
														CurrElement, CurrLevel);
							}
						}

#					ifdef DEBUG
					if (ctype == LchainNode)
						pr_diag("Editor",
							"[edit_nodes_lchain] Paste interp node - promote to link node with attributes\n");
					else if (ctype == LchainFloating)
						pr_diag("Editor",
							"[edit_nodes_lchain] Paste interp node - promote to floating node with attributes\n");
#					endif /* DEBUG */
					}

				/* Reinterpolate modified link chain */
				Lpick->dointerp = TRUE;
				(void) interpolate_lchain(Lpick);

				/* Confirm the modification */
				drawn  = TRUE;
				State = Confirm;
				continue;

			/* Modify the parameters of a link node */
			case ModifyNode:

				/* Freeze all pending edits so only this one can be undone */
				/* (first time after picking) */
				if (Nfresh)
					{
					Nfresh = FALSE;
					if (EditUndoable)
						{
						ignore_partial();
						put_message("edit-freeze");
						accept_mod();	/* Leaves EditLchains alone */
						}
					}
				post_partial(TRUE);
				put_message("lchain-node-modify");
				Nmodify = TRUE;

#				ifdef DEBUG
				pr_diag("Editor",
					"[edit_nodes_lchain] Modify node - start: %s\n",
					atype);
#				endif /* DEBUG */

				/* Wait for a change in node type */
				if (blank(ntype) || same(ntype, FpaNodeClass_Unknown)
						|| same(ntype, atype)) return drawn;

#				ifdef DEBUG
				pr_diag("Editor",
					"[edit_nodes_lchain] Modify node - change: %s  to: %s\n",
					atype, ntype);
#				endif /* DEBUG */

				/* Extract the node parameters */
				if (ltype == LchainNode)
					{
					mplus  = Lpick->nodes[inode]->mplus;
					attrib = Lpick->nodes[inode]->attrib;
					copy_point(pos, Lpick->nodes[inode]->node);
					}
				else if (ltype == LchainControl)
					{
					mplus  = Lpick->nodes[inode]->mplus;
					attrib = Lpick->nodes[inode]->attrib;
					copy_point(pos, Lpick->nodes[inode]->node);
					}
				else if (ltype == LchainFloating)
					{
					mplus  = Lpick->nodes[inode]->mplus;
					attrib = Lpick->nodes[inode]->attrib;
					copy_point(pos, Lpick->nodes[inode]->node);
					}
				else if (ltype == LchainInterp)
					{
					mplus  = Lpick->interps[inode]->mplus;
					attrib = Lpick->interps[inode]->attrib;
					copy_point(pos, Lpick->interps[inode]->node);
					}

#				ifdef DEBUG
				pr_diag("Editor",
					"[edit_nodes_lchain] Modify node - mplus: %d  node: %.2f/%.2f\n",
					mplus, pos[X], pos[Y]);
#				endif /* DEBUG */

				/* Create a new link node based on the new node type */
				/* Note that interpolated nodes are not created! */
				if (same(ntype, FpaNodeClass_Normal))
					{
					lnode = create_lnode(mplus);
					define_lnode_type(lnode, TRUE, FALSE, LchainNode);
					define_lnode_node(lnode, pos);
					define_lnode_attribs(lnode, attrib);
					CAL_invoke_lnode_rules_by_name((CAL) lnode->attrib,
														CurrElement, CurrLevel);
					}
				else if (same(ntype, FpaNodeClass_Control))
					{
					lnode = create_lnode(mplus);
					define_lnode_type(lnode, TRUE, FALSE, LchainControl);
					define_lnode_node(lnode, pos);
					define_lnode_attribs(lnode, attrib);
					CAL_invoke_lnode_rules_by_name((CAL) lnode->attrib,
														CurrElement, CurrLevel);
					}
				else if (same(ntype, FpaNodeClass_Floating))
					{
					lnode = create_lnode(mplus);
					define_lnode_type(lnode, TRUE, FALSE, LchainFloating);
					define_lnode_node(lnode, pos);
					define_lnode_attribs(lnode, attrib);
					CAL_invoke_lnode_rules_by_name((CAL) lnode->attrib,
														CurrElement, CurrLevel);
					}

				/* Remove the old link node */
				/* Note that interpolated nodes are not removed! */
				if (ltype == LchainNode)
					{
					(void) remove_lchain_lnode(Lpick, inode);
					}
				else if (ltype == LchainControl)
					{
					(void) remove_lchain_lnode(Lpick, inode);
					}
				else if (ltype == LchainFloating)
					{
					(void) remove_lchain_lnode(Lpick, inode);
					}

				/* Add the new link node */
				/* Note that interpolated nodes are not added! */
				if (same(ntype, FpaNodeClass_Normal))
					{
					(void) add_lchain_lnode(Lpick, lnode);
					atype = FpaNodeClass_Normal;
					ltype = LchainNode;
					}
				else if (same(ntype, FpaNodeClass_Control))
					{
					(void) add_lchain_lnode(Lpick, lnode);
					atype = FpaNodeClass_Control;
					ltype = LchainControl;
					}
				else if (same(ntype, FpaNodeClass_Floating))
					{
					(void) add_lchain_lnode(Lpick, lnode);
					atype = FpaNodeClass_Floating;
					ltype = LchainFloating;
					}
				else if (same(ntype, FpaNodeClass_Interp))
					{
					atype = FpaNodeClass_Interp;
					ltype = LchainInterp;
					}
				
				/* Identify the modified link node */
				inode = which_lchain_node(Lpick, ltype, mplus);

				/* Reinterpolate modified link chain */
				Lpick->dointerp = TRUE;
				(void) interpolate_lchain(Lpick);

				/* Register the edit */
				ignore_partial();
				invoke_set_catspecs(EditLchains);
				if (EditUndoable)
					{
					highlight_set(EditLchains, 1);
					highlight_set_secondary(EditLchains, 1);
					}
				if (EditUndoable) post_mod("lchains");

				/* Clean up temporary buffers */
				Lwork = NullLchain;

				/* Empty display structure */
				empty_temp();

				/* Produce a copy of the picked link chain to work on */
				Lwork = copy_lchain(Lpick);

				/* Highlight the picked link chain and the link nodes */
				highlight_lchain(Lwork, 2);
				highlight_lchain_nodes(Lwork, 3, 3);
				add_item_to_metafile(TempMeta, "lchain", "l", "", "",
									 (ITEM) Lwork);

				/* Highlight the selected node */
				if      (ltype == LchainNode)
					highlight_lnode(Lwork->nodes[inode], 2, 2);
				else if (ltype == LchainControl)
					highlight_lnode(Lwork->nodes[inode], 2, 2);
				else if (ltype == LchainFloating)
					highlight_lnode(Lwork->nodes[inode], 2, 2);
				else if (ltype == LchainInterp)
					highlight_linterp(Lwork->interps[inode], 2, 2);
				present_all();

				/* Set the attributes for entry menus         */
				/* Note that attributes for control nodes and */
				/*  interpolated nodes cannot be modified!    */
				if      (ltype == LchainNode)
					{
					attrib = Lpick->nodes[inode]->attrib;
					edit_select_node((CAL) attrib, TRUE);
					}
				else if (ltype == LchainControl)
					{
					attrib = Lpick->nodes[inode]->attrib;
					edit_select_node((CAL) attrib, FALSE);
					}
				else if (ltype == LchainFloating)
					{
					attrib = Lpick->nodes[inode]->attrib;
					edit_select_node((CAL) attrib, TRUE);
					}
				else if (ltype == LchainInterp)
					{
					attrib = Lpick->interps[inode]->attrib;
					edit_select_node((CAL) attrib, FALSE);
					}

				/* Go on to the next stage ... do more modifying */
				drawn = TRUE;
				State = ModifyNode;
				continue;

			/* Delete a link node */
			case DeleteNode:

				/* Freeze all pending edits so only this one can be undone */
				if (Nfresh && EditUndoable)
					{
					ignore_partial();
					put_message("edit-freeze");
					accept_mod();	/* Leaves EditLchains alone */
					}
				post_partial(TRUE);

				/* Delete the link chain if this is the last node */
				if (Lpick->lnum <= 1)
					{

					/* Delete link chain if it cannot be interpolated */
					(void) remove_item_from_set(EditLchains, (ITEM) Lpick);
					put_message("lchain-deleting");
					(void) sleep(1);

					/* Register the edit */
					ignore_partial();
					invoke_set_catspecs(EditLchains);
					if (EditUndoable)
						{
						highlight_set(EditLchains, 1);
						highlight_set_secondary(EditLchains, 0);
						}
					if (EditUndoable) post_mod("lchains");

					/* Clean up temporary buffers */
					edit_select_node(NullCal, FALSE);
					Lpick   = NullLchain;
					Lwork   = NullLchain;
					Nfresh  = FALSE;
					Nadjust = TRUE;
					Nmodify = FALSE;

#					ifdef DEBUG_STRUCTURES
					pr_diag("Editor",
						"[edit_nodes_lchain] Delete ... empty TempMeta %d fields\n",
						TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */

					/* Empty display structure */
					empty_temp();

					/* Pick another link chain */
					State = PickChain;
					continue;
					}

				/* Delete the node */
				if (ltype == LchainNode)
					{
					(void) remove_lchain_lnode(Lpick, inode);
					}
				else if (ltype == LchainControl)
					{
					(void) remove_lchain_lnode(Lpick, inode);
					}
				else if (ltype == LchainFloating)
					{
					(void) remove_lchain_lnode(Lpick, inode);
					}

				/* Reinterpolate modified link chain */
				Lpick->dointerp = TRUE;
				(void) interpolate_lchain(Lpick);

				/* Confirm the modification */
				drawn = TRUE;
				State = Confirm;
				continue;

			/* Show the attributes of a link node */
			case ShowNode:

				/* Display a line to link node location */
				curve = create_curve("", "", "");
				add_point_to_curve(curve, opos);
				add_point_to_curve(curve, npos);
				define_lspec(&curve->lspec, EditColour, 0, NULL, False,
							0.0, 0.0, (HILITE) 2);
				add_item_to_metafile(TempMeta, "curve", "c", "box", "box",
							(ITEM) curve);
				mark = create_mark("", "", "", opos, 0.0);
				define_mspec(&mark->mspec, EditColour, 2, NULL, 0, False,
							5.0, 0.0, (HILITE) 2);
				add_item_to_metafile(TempMeta, "mark",  "d", "box", "box",
							(ITEM) mark);
				present_temp(TRUE);
			
				/* Make a copy of the node attributes */
				if      (ltype == LchainNode)
					{
					mplus   = Lpick->nodes[inode]->mplus;
					xattrib = copy_attrib_list(Lpick->nodes[inode]->attrib);
					}
				else if (ltype == LchainControl)
					{
					mplus   = Lpick->nodes[inode]->mplus;
					xattrib = copy_attrib_list(Lpick->nodes[inode]->attrib);
					}
				else if (ltype == LchainFloating)
					{
					mplus   = Lpick->nodes[inode]->mplus;
					xattrib = copy_attrib_list(Lpick->nodes[inode]->attrib);
					}
				else if (ltype == LchainInterp)
					{
					mplus   = Lpick->interps[inode]->mplus;
					xattrib = copy_attrib_list(Lpick->interps[inode]->attrib);
					}

				/* Add the node timestamp to the attributes */
				vtime = calc_valid_time_minutes(Lpick->xtime, 0, mplus);
				CAL_add_attribute(xattrib, AttribLnodeTstamp, vtime);

				/* Add the node motion to the attributes */
				CAL_add_lchain_node_motion(xattrib, MapProj, Lpick, mplus);

				/* Display the node attributes */
				sample_box(TRUE, xattrib);
				edit_can_clear(TRUE);

				/* Destroy the copied node attributes */
				xattrib = destroy_attrib_list(xattrib);

				/* Go on to the next stage */
				State = PickNode;
				continue;

			/* Set the value for the link node just modified */
			case SetNode:

				/* Freeze all pending edits so only this one can be undone */
				/* (first time after picking) */
				if (Nfresh)
					{
					Nfresh = FALSE;
					if (EditUndoable)
						{
						ignore_partial();
						put_message("edit-freeze");
						accept_mod();	/* Leaves EditLchains alone */
						}
					}
				post_partial(TRUE);
				Nmodify = TRUE;

#				ifdef DEBUG
				pr_diag("Editor",
					"[edit_nodes_lchain] Set node - for type: %s\n",
					atype);
#				endif /* DEBUG */

				/* Extract the node type from the attribute structure */
				cval = CAL_get_attribute(cal, AttribLnodeType);
				if (same(cval, FpaNodeClass_Normal))
					atype = FpaNodeClass_Normal;
				else if (same(cval, FpaNodeClass_Control))
					atype = FpaNodeClass_Control;
				else if (same(cval, FpaNodeClass_Floating))
					atype = FpaNodeClass_Floating;
				else if (same(cval, FpaNodeClass_Interp))
					atype = FpaNodeClass_Interp;
				else
					{
					put_message("lchain-node-unknown", cval);
					(void) sleep(1);
					State = ModifyNode;
					continue;
					}

				/* Replace the attributes if the node type has not changed */
				/* Note that attributes for control nodes and interpolated */
				/*  nodes cannot be re-set!                                */
				if ( (same(atype, FpaNodeClass_Normal) && ltype == LchainNode)
						|| (same(atype, FpaNodeClass_Floating)
								&& ltype == LchainFloating) )
					{

					lnode = Lpick->nodes[inode];
					mplus = lnode->mplus;
					define_lnode_attribs(lnode, (ATTRIB_LIST) cal);
					CAL_invoke_lnode_rules_by_name((CAL) lnode->attrib,
														CurrElement, CurrLevel);
#					ifdef DEBUG
					pr_diag("Editor",
						"[edit_nodes_lchain] Set node - mplus: %d\n", mplus);
#					endif /* DEBUG */
					}

				else if (same(atype, FpaNodeClass_Control)
							&& ltype == LchainControl)
					{
					put_message("lchain-no-mod-control");
					(void) sleep(1);
					State = ModifyNode;
					continue;
					}

				else if (same(atype, FpaNodeClass_Interp)
							&& ltype == LchainInterp)
					{
					put_message("lchain-no-mod-interp");
					(void) sleep(1);
					State = ModifyNode;
					continue;
					}

				/* Change in attribute type ... so must replace node */
				else
					{ 

					/* Extract the node parameters */
					if (ltype == LchainNode)
						{
						mplus = Lpick->nodes[inode]->mplus;
						copy_point(pos, Lpick->nodes[inode]->node);
						}
					else if (ltype == LchainControl)
						{
						mplus = Lpick->nodes[inode]->mplus;
						copy_point(pos, Lpick->nodes[inode]->node);
						}
					else if (ltype == LchainFloating)
						{
						mplus = Lpick->nodes[inode]->mplus;
						copy_point(pos, Lpick->nodes[inode]->node);
						}
					else if (ltype == LchainInterp)
						{
						mplus = Lpick->interps[inode]->mplus;
						copy_point(pos, Lpick->interps[inode]->node);
						}

#					ifdef DEBUG
					pr_diag("Editor",
						"[edit_nodes_lchain] Set node - mplus: %d  to type: %.s\n",
						mplus, atype);
#					endif /* DEBUG */

					/* Create a new link node based on the new node type */
					/*  and the new set of attributes                    */
					/* Note that interpolated nodes are not created!     */
					if (same(atype, FpaNodeClass_Normal))
						{
						lnode = create_lnode(mplus);
						define_lnode_type(lnode, TRUE, FALSE, LchainNode);
						define_lnode_node(lnode, pos);
						define_lnode_attribs(lnode, (ATTRIB_LIST) cal);
						CAL_invoke_lnode_rules_by_name((CAL) lnode->attrib,
														CurrElement, CurrLevel);
						}
					else if (same(atype, FpaNodeClass_Control))
						{
						lnode = create_lnode(mplus);
						define_lnode_type(lnode, TRUE, FALSE, LchainControl);
						define_lnode_node(lnode, pos);
						define_lnode_attribs(lnode, (ATTRIB_LIST) cal);
						CAL_invoke_lnode_rules_by_name((CAL) lnode->attrib,
														CurrElement, CurrLevel);
						}
					else if (same(atype, FpaNodeClass_Floating))
						{
						lnode = create_lnode(mplus);
						define_lnode_type(lnode, TRUE, FALSE, LchainFloating);
						define_lnode_node(lnode, pos);
						define_lnode_attribs(lnode, (ATTRIB_LIST) cal);
						CAL_invoke_lnode_rules_by_name((CAL) lnode->attrib,
														CurrElement, CurrLevel);
						}

					/* Remove the old link node */
					/* Note that interpolated nodes are not removed! */
					if (ltype == LchainNode)
						{
						(void) remove_lchain_lnode(Lpick, inode);
						}
					else if (ltype == LchainControl)
						{
						(void) remove_lchain_lnode(Lpick, inode);
						}
					else if (ltype == LchainFloating)
						{
						(void) remove_lchain_lnode(Lpick, inode);
						}

					/* Add the new link node */
					/* Note that interpolated nodes are not added! */
					if (same(atype, FpaNodeClass_Normal))
						{
						(void) add_lchain_lnode(Lpick, lnode);
						ltype = LchainNode;
						}
					else if (same(atype, FpaNodeClass_Control))
						{
						(void) add_lchain_lnode(Lpick, lnode);
						ltype = LchainControl;
						}
					else if (same(atype, FpaNodeClass_Floating))
						{
						(void) add_lchain_lnode(Lpick, lnode);
						ltype = LchainControl;
						}
					else if (same(atype, FpaNodeClass_Interp))
						{
						ltype = LchainInterp;
						}
				
					/* Identify the modified link node */
					inode = which_lchain_node(Lpick, ltype, mplus);
					}

				/* Reinterpolate modified link chain */
				Lpick->dointerp = TRUE;
				(void) interpolate_lchain(Lpick);

				/* Register the edit */
				ignore_partial();
				invoke_set_catspecs(EditLchains);
				if (EditUndoable)
					{
					highlight_set(EditLchains, 1);
					highlight_set_secondary(EditLchains, 0);
					}
				if (EditUndoable) post_mod("lchains");

				/* Clean up temporary buffers */
				Lwork = NullLchain;

				/* Empty display structure */
				empty_temp();

				/* Produce a copy of the picked link chain to work on */
				Lwork = copy_lchain(Lpick);

				/* Highlight the picked link chain and the link nodes */
				highlight_lchain(Lwork, 2);
				highlight_lchain_nodes(Lwork, 3, 3);
				add_item_to_metafile(TempMeta, "lchain", "l", "", "",
									 (ITEM) Lwork);

				/* Highlight the selected node */
				if      (ltype == LchainNode)
					highlight_lnode(Lwork->nodes[inode], 2, 2);
				else if (ltype == LchainControl)
					highlight_lnode(Lwork->nodes[inode], 2, 2);
				else if (ltype == LchainFloating)
					highlight_lnode(Lwork->nodes[inode], 2, 2);
				else if (ltype == LchainInterp)
					highlight_linterp(Lwork->interps[inode], 2, 2);
				present_all();

				/* Set the attributes for entry menus         */
				/* Note that attributes for control nodes and */
				/*  interpolated nodes cannot be modified!    */
				if      (ltype == LchainNode)
					{
					attrib = Lpick->nodes[inode]->attrib;
					edit_select_node((CAL) attrib, TRUE);
					}
				else if (ltype == LchainControl)
					{
					attrib = Lpick->nodes[inode]->attrib;
					edit_select_node((CAL) attrib, FALSE);
					}
				else if (ltype == LchainFloating)
					{
					attrib = Lpick->nodes[inode]->attrib;
					edit_select_node((CAL) attrib, TRUE);
					}
				else if (ltype == LchainInterp)
					{
					attrib = Lpick->interps[inode]->attrib;
					edit_select_node((CAL) attrib, FALSE);
					}

				/* Go on to the next stage ... do more modifying */
				drawn = TRUE;
				State = ModifyNode;
				continue;

			/* Confirm current edit */
			case Confirm:

				/* Register the edit */
				ignore_partial();
				invoke_set_catspecs(EditLchains);
				if (EditUndoable)
					{
					highlight_set(EditLchains, 1);
					highlight_set_secondary(EditLchains, 0);
					}
				if (EditUndoable) post_mod("lchains");

				/* Clean up temporary buffers */
				edit_select_node(NullCal, FALSE);
				Lwork   = NullLchain;
				Nfresh  = FALSE;
				Nadjust = TRUE;
				Nmodify = FALSE;

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[edit_nodes_lchain] Confirm ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();

				/* Redisplay the picked link chain */
				State = DisplayChain;
				continue;

			/* Display the selected link chain */
			case DisplayChain:

				/* Produce a copy of the picked link chain to work on */
				Lwork = copy_lchain(Lpick);

				/* Highlight the picked link chain and the link nodes */
				highlight_lchain(Lwork, 2);
				highlight_lchain_nodes(Lwork, 3, 3);
				add_item_to_metafile(TempMeta, "lchain", "l", "", "",
															(ITEM) Lwork);
				present_all();
				post_partial(TRUE);

				/* Go on to pick a node */
				State = PickNode;
				continue;
			}
		}
	}

/**********************************************************************/

static	NODESTATE	node_lchain_pick(void)

	{
	POINT	pos;
	int		butt;

	/* See if there are any link chains to pick */
	if (EditLchains->num <= 0)
		{
		put_message("lchain-no-node-chains");
		return PickChain;
		}

	/* Keep trying until something gets picked */
	while (TRUE)
		{
		/* Get a point */
		put_message("lchain-node-chain-pick");
		if (!ready_Xpoint(DnEdit, NullPoint, &butt)) return PickChain;
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}
		put_message("lchain-pick");
		if (!pick_Xpoint(DnEdit, 0, pos, &butt)) continue;
		if (!inside_dn_window(DnEdit, pos))
			{
			put_message("edit-outside-map");
			(void) sleep(1);
			continue;
			}

		/* See which link chain was picked */
		/* Find the closest link chain */
		Lpick = closest_lchain(EditLchains, pos, NullFloat, NullPoint, NullInt);
		if (!Lpick)
			{
			put_message("lchain-no-pick");
			(void) sleep(1);
			continue;
			}

		/* Display the picked link chain */
		put_message("lchain-picked");
		return DisplayChain;
		}
	}

/**********************************************************************/

static	void	clear_node_sample(void)

	{

	/* Clean up temporary buffers */
	Lwork = NullLchain;

	/* Empty display structure */
	empty_temp();

	/* Produce a copy of the picked link chain to work on */
	if (Lpick)
		{
		Lwork = copy_lchain(Lpick);

		/* Highlight the picked link chain and the link nodes */
		highlight_lchain(Lwork, 2);
		highlight_lchain_nodes(Lwork, 3, 3);
		add_item_to_metafile(TempMeta, "lchain", "l", "", "", (ITEM) Lwork);

		/* Show the results */
		present_all();
		}
	}
