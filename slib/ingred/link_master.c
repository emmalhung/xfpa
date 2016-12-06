/***********************************************************************
*                                                                      *
*     l i n k _ m a s t e r . c                                        *
*                                                                      *
*     Routines to manipulate master time-link chains.                  *
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
*     l i n k _ m a s t e r                                            *
*                                                                      *
*     Input set of link nodes.                                         *
*                                                                      *
***********************************************************************/

LOGICAL	link_master

	(
	STRING	mode,
	LOGICAL	forward
	)

	{
	int		itime, butt, last, mplus, nchain;
	POINT	pos;
	LOGICAL	ambig;
	LOGICAL	drawn = FALSE;

	/* Current link chain */
	static	LOGICAL	empty = TRUE;
	static	LCHAIN	chain = NullLchain;
	static	int		ijoin = -1;
	static	LCHAIN	join  = NullLchain;
	static	LNODE	lnode = NullLnode;

	/* Current chain state */
	static	enum { Start, Add, ShowJoin, Join, End }	State = Start;

#	ifdef DEBUG
	pr_diag("Editor", "[link_master] %s\n", mode);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same_start(mode, "cancel"))
		{
		if (!empty)
			{
			put_message("edit-cancel");
			(void) sleep(1);
			ignore_partial();

			chain =  destroy_lchain(chain);
			empty = TRUE;
			(void) end_link();
			}
		edit_complete();
		State = Start;
		return FALSE;
		}

	/* Remove added link chain */
	if (same(mode, "undo"))
		{
#		ifdef DEBUG
		pr_diag("Editor", "[link_master] Undo add by replacing link chain!\n");
#		endif /* DEBUG */

		/* Replace all link chains for this field */
		(void) replace_posted_chains("master", FpaCnone, FpaCnone);

		/* Reset link status */
		busy_cursor(TRUE);
		(void) release_links();
		(void) extract_links();
		(void) present_all();

		ActiveDfld->interp   = FALSE;
		ActiveDfld->intlab   = FALSE;
		ActiveDfld->saved    = FALSE;
		ActiveDfld->reported = FALSE;

		(void) ready_master_links(ActiveDfld, TRUE);
		(void) save_links();
		busy_cursor(FALSE);
		edit_complete();
		return FALSE;
		}

	/* Construct temporary buffers on startup only */
	if (same(mode, "begin"))
		{
		(void) release_posted_chains();
		State = Start;
		ignore_partial();
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
			case Join:	put_message("link-begin");
						ijoin = -1;
						join  = NullLchain;
						empty_metafile(TempMeta);
						(void) build_link_chain(ActiveDfld, chain, TRUE);
						(void) present_temp(TRUE);

						/* Move on to next chart if possible */
						/* Otherwise move on to the end */
						if (next_link(chain))
							{
							State = Add;

							/* Display fields (including guidance) */
							linking_chart(TimeList[EditTime].jtime);
							(void) present_all();
							}
						else
							{
							State = End;
							}
						break;
			}
		}

	/* Keep on adding link chains until told to quit */
	while (TRUE)
		{
		switch (State)
			{
			case Start:

				ijoin = -1;
				join  = NullLchain;

				/* Initialize a new link chain */
				chain = destroy_lchain(chain);
				chain = create_lchain();
				define_lchain_reference_time(chain, Stime);
				define_lchain_interp_delta(chain, DTween);

				/* >>>>> do not worry about creating nodes yet ...
				chain->lnum  = NumTime;
				chain->nodes = GETMEM(chain->nodes, LNODE, NumTime);
				for (itime=0; itime<NumTime; itime++)
					chain->nodes[itime] = create_lnode(TimeList[itime].mplus);
				<<<<< */

				/* Initialize start and end times */
				chain->splus = (NumTime>0)? TimeList[0].mplus:         MAXINT;
				chain->eplus = (NumTime>0)? TimeList[NumTime-1].mplus: MAXINT;
				define_lchain_default_attribs(chain);

				empty = TRUE;

				if (!start_link(forward))
					{
					put_message("link-no-fld");
					return FALSE;
					}

				/* Set state for next stage */
				State = Add;

				/* Display fields (including guidance) */
				linking_chart(TimeList[EditTime].jtime);
				(void) present_all();

				/* Move on to next stage */
				continue;

			case Add:

				/* Place a new link node */
				mplus = TimeList[EditTime].mplus;
				if (minutes_in_depictions())
					put_message("link-place-mins",
								hour_minute_string(0, mplus));
				else
					put_message("link-place", mplus/60);
				if (!ready_Xpoint(DnEdit, NULL, &butt)) return drawn;
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				put_message("link-place-add");
				(void) pick_Xpoint(DnEdit, 0, pos, &butt);
				if (!inside_dn_window(DnEdit,pos))
					{
					put_message("edit-outside-map");
					(void) sleep(1);
					continue;
					}

				/* Set the node parameters */
				/* >>>>> replace this ...
				define_lnode_node(chain->nodes[EditTime], TRUE, FALSE, pos);
				define_lnode_attach(chain->nodes[EditTime], -1, 0, -1);
				chain->dointerp = TRUE;
				... with this <<<<< */
				lnode = create_lnode(mplus);
				define_lnode_type(lnode, TRUE, FALSE, LchainNode);
				define_lnode_node(lnode, pos);
				define_lnode_attach(lnode, -1, 0, -1);

				/* Add new node and display it */
				put_message("link-adding");
				post_partial(TRUE);
				if (empty)
					{
					chain->splus = mplus;
					chain->eplus = mplus;
					define_lchain_default_attribs(chain);
					(void) add_lchain_lnode(chain, lnode);

					empty = FALSE;

					State = ShowJoin;
					continue;
					}
				else
					{
					if (forward) chain->eplus = mplus;
					else         chain->splus = mplus;
					define_lchain_default_attribs(chain);
					(void) add_lchain_lnode(chain, lnode);
					}

				/* Move on to next chart if possible */
				/* Otherwise move on to the end */
				if (next_link(chain))
					{
					State = Add;

					/* Display fields (including guidance) */
					linking_chart(TimeList[EditTime].jtime);
					(void) present_all();
					}
				else
					{
					State = End;
					}

				/* Move on to next stage */
				continue;

			case ShowJoin:

				/* See if we can join to an existing chain */
				ijoin = closest_link_end(forward, EditTime, pos, ActiveDfld,
										 PickTol, &ambig);
				if (ijoin < 0)
					{
					empty_metafile(TempMeta);
					(void) build_link_chain(ActiveDfld, chain, TRUE);
					(void) present_temp(TRUE);

					/* Move on to next chart if possible */
					/* Otherwise move on to the end */
					if (next_link(chain))
						{
						State = Add;

						/* Display fields (including guidance) */
						linking_chart(TimeList[EditTime].jtime);
						(void) present_all();
						}
					else
						{
						State = End;
						}

					/* Move on to next stage */
					continue;
					}

				join = ActiveDfld->chains[ijoin];
				empty_metafile(TempMeta);
				(void) build_link_chain(ActiveDfld, join, TRUE);
				(void) present_temp(TRUE);
				put_message("link-join");
				(void) new_link();

				State = Join;
				continue;

			case Join:

				put_message("link-join");
				if (!ready_Xpoint(DnEdit, NULL, &butt)) return drawn;
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}

				put_message("link-release-join");
				(void) ignore_Xpoint();
				put_message("link-joining");

				/* Copy the existing link chain */
				chain = destroy_lchain(chain);
				chain = copy_lchain(join);
				mplus = TimeList[EditTime].mplus;
				chain->splus = (forward)? join->splus: mplus;
				chain->eplus = (forward)? mplus: join->eplus;
				define_lchain_default_attribs(chain);

				/* Move on to next chart if possible */
				/* Otherwise move on to the end */
				if (next_link(chain))
					{
					State = Add;

					/* Display fields (including guidance) */
					linking_chart(TimeList[EditTime].jtime);
					(void) present_all();
					}
				else
					{
					State = End;
					}

				/* Move on to next stage */
				continue;

			case End:

				(void) end_link();

				if (!empty)
					{
					put_message("link-add-proc");
					ignore_partial();
					busy_cursor(TRUE);

					(void) build_link_chain(ActiveDfld, chain, FALSE);
					(void) present_timelink(TRUE);

					/* Chain is ready for interpolation */
					chain->dointerp = TRUE;
					(void) interpolate_lchain(chain);

					/* Post the existing link chains before adding */
					(void) post_link_chains("master", FpaCnone, FpaCnone);

					/* Replace existing link chain if joining */
					if (join)
						{
						/* >>>>> replace this ...
						join = destroy_lchain(join);
						join = copy_lchain(chain);
						... with this <<<<< */
						join = destroy_lchain(join);
						ActiveDfld->chains[ijoin] = copy_lchain(chain);
						}

					/* Otherwise add the new link chain */
					else
						{
						last   = ActiveDfld->nchain++;
						nchain = ActiveDfld->nchain;
						ActiveDfld->chains = GETMEM(ActiveDfld->chains, LCHAIN,
													nchain);
						ActiveDfld->chains[last] = copy_lchain(chain);
						}

					/* Reset link status */
					post_partial(FALSE);
					edit_pending();

					ActiveDfld->interp   = FALSE;
					ActiveDfld->intlab   = FALSE;
					ActiveDfld->saved    = FALSE;
					ActiveDfld->reported = FALSE;

					(void) ready_master_links(ActiveDfld, TRUE);
					(void) revise_dependent_links(ActiveDfld);
					(void) save_links();
					busy_cursor(FALSE);
					}

				/* Move on to next stage */
				State = Start;
				continue;
			}
		}
	}

/***********************************************************************
*                                                                      *
*     d e l i n k _ m a s t e r                                        *
*                                                                      *
*     Remove a link node or an entire link chain.                      *
*                                                                      *
***********************************************************************/

LOGICAL	delink_master

	(
	STRING	mode,
	STRING	type
	)

	{
	POINT	pos;
	int		ichain, inode, butt;
	LTYPE	ltype;
	LCHAIN	chain;
	LOGICAL	ambig, dupl;
	LOGICAL	drawn = FALSE;

#	ifdef DEBUG
	pr_diag("Editor", "[delink_master] %s %s\n", mode, type);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same_start(mode, "cancel"))
		{
		ignore_partial();
		edit_complete();
		return FALSE;
		}

	/* Delete all link chains */
	if (same(mode, "clear"))
		{

		/* Are there any link chains left */
		if (ActiveDfld->nchain <= 0)
			{
			put_message("link-no-del");
			return FALSE;
			}

#		ifdef DEBUG
		pr_diag("Editor",
			"[delink_master] Clear to remove link chains!\n");
#		endif /* DEBUG */

		/* Post the existing link chains before deletion */
		(void) post_link_chains("master", FpaCnone, FpaCnone);

		/* Delete link chains for this field */
		(void) clear_dfield_links(ActiveDfld, FALSE);

		/* Reset link status */
		busy_cursor(TRUE);
		(void) release_links();
		(void) extract_links();
		(void) present_all();

		ActiveDfld->interp   = FALSE;
		ActiveDfld->intlab   = FALSE;
		ActiveDfld->saved    = FALSE;
		ActiveDfld->reported = FALSE;

		(void) ready_master_links(ActiveDfld, TRUE);
		(void) save_links();
		busy_cursor(FALSE);
		edit_pending();
		return FALSE;
		}

	/* Replace deleted link chains */
	if (same(mode, "undo"))
		{
#		ifdef DEBUG
		pr_diag("Editor",
			"[delink_master] Undo delete by replacing link chain!\n");
#		endif /* DEBUG */

		/* Replace all link chains for this field */
		(void) replace_posted_chains("master", FpaCnone, FpaCnone);

		/* Reset link status */
		busy_cursor(TRUE);
		(void) release_links();
		(void) extract_links();
		(void) present_all();

		ActiveDfld->interp   = FALSE;
		ActiveDfld->intlab   = FALSE;
		ActiveDfld->saved    = FALSE;
		ActiveDfld->reported = FALSE;

		(void) ready_master_links(ActiveDfld, TRUE);
		(void) save_links();
		busy_cursor(FALSE);
		edit_complete();
		return FALSE;
		}

	/* Initialize on startup */
	if (same(mode, "begin"))
		{
		(void) release_posted_chains();
		cancel_partial();
		}

	/* Keep on deleting until told to quit */
	while (TRUE)
		{

		/* Are there any link chains left */
		if (ActiveDfld->nchain <= 0)
			{
			put_message("link-no-del");
			return drawn;
			}

		/* Pick a point */
		if (same(type, "CHAIN")) put_message("link-remove");
		else                     put_message("link-remove-node");
		if (!ready_Xpoint(DnEdit, NULL, &butt)) return drawn;
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}
		(void) pick_Xpoint(DnEdit, 0, pos, &butt);
		if (!inside_dn_window(DnEdit,pos))
			{
			put_message("edit-outside-map");
			(void) sleep(1);
			continue;
			}
		
		/* Search for closest link node on any link chain, on any keyframe */
		ichain = closest_link_chain(EditTime, pos, ActiveDfld, PickTol,
						&inode, &ltype, &ambig, &dupl);

		/* Did we get close enough */
		if (ichain < 0)
			{
			put_message("link-too-far");
			(void) sleep(1);
			continue;
			}

		/* Did we hit a duplicate link chain ... delete last chain! */
		if (ambig && dupl && same(type, "CHAIN"))
			{
			put_message("link-ambig-del");
			(void) sleep(1);
			}

		/* Did we hit an ambiguous link chain ... try choosing again! */
		else if (ambig && same(type, "CHAIN"))
			{
			put_message("link-ambig");
			(void) sleep(1);
			continue;
			}

		/* Did we hit an ambiguous link node ... delete it anyways! */
		else if (ambig)
			{
			put_message("link-node-ambig-del");
			(void) sleep(1);
			}

		/* Something got picked */
		/* Delete a link node or an entire link chain */
		busy_cursor(TRUE);

		/* Delete the entire link chain */
		if (same(type, "CHAIN"))
			{

			/* Post the existing link chains before deletion */
			(void) post_link_chains("master", FpaCnone, FpaCnone);

#			ifdef DEBUG
			pr_diag("Editor",
				"[delink_master] Remove link chain: %d\n", ichain);
#			endif /* DEBUG */

			/* Delete the link chain */
			put_message("link-remove-proc");
			(void) remove_link_chain(ActiveDfld, ichain);
			post_partial(FALSE);
			edit_pending();
			}

		/* Delete a link node */
		else
			{

			/* Remove a control node */
			/* Note that we do not worry about ambiguous control nodes! */
			/* Ambiguous control nodes will be stacked, so just remove  */
			/*  the one closest to the current time!                    */
			if (ltype==LINK_CTL)
				{

				/* Post the existing link chains before deletion */
				(void) post_link_chains("master", FpaCnone, FpaCnone);

#				ifdef DEBUG
				pr_diag("Editor",
					"[delink_master] Remove control node: %d\n", inode);
#				endif /* DEBUG */

				/* Delete the control node */
				put_message("link-remove-node-proc");
				chain = ActiveDfld->chains[ichain];
				(void) remove_lchain_lnode(chain, inode);
				post_partial(FALSE);
				edit_pending();
				}

			/* Remove a link node */
			/* Note that removing the node will shorten the link chain! */
			else if (ltype==LINK_REG)
				{

				/* Post the existing link chains before deletion */
				(void) post_link_chains("master", FpaCnone, FpaCnone);

				/* Delete the link node */
				if (remove_link_node(ActiveDfld, ichain, inode))
					{
#					ifdef DEBUG
					pr_diag("Editor",
						"[delink_master] Remove link node: %d\n", inode);
#					endif /* DEBUG */

					put_message("link-remove-node-proc");
					post_partial(FALSE);
					edit_pending();
					}
				else
					{
#					ifdef DEBUG
					pr_warning("Editor",
						"[delink_master] Only start/end nodes can be removed!\n");
#					endif /* DEBUG */

					put_message("link-remove-node-no");
					(void) sleep(1);
					}
				}
			}

		/* Reset link status */
		(void) release_links();
		(void) extract_links();
		(void) present_all();
		ActiveDfld->interp   = FALSE;
		ActiveDfld->intlab   = FALSE;
		ActiveDfld->saved    = FALSE;
		ActiveDfld->reported = FALSE;

		(void) ready_master_links(ActiveDfld, TRUE);
		(void) revise_dependent_links(ActiveDfld);
		(void) save_links();
		busy_cursor(FALSE);
		}
	}

/***********************************************************************
*                                                                      *
*     m v l i n k _ m a s t e r                                        *
*                                                                      *
*     Move the selected link node.                                     *
*                                                                      *
***********************************************************************/

LOGICAL	mvlink_master

	(
	STRING	mode
	)

	{
	float	dx, dy;
	int		ichain, inode, butt;
	LTYPE	ltype;
	LOGICAL	ambig, dupl;
	POINT	pos, opos;
	LCHAIN	chain;
	LNODE	lnode;
	LOGICAL	drawn = FALSE;

	static	SET		mset  = NULL;
	static	MARK	mark  = NULL;
	static	POINT	mpos  = {0,0};
	static	LOGICAL	Inter = FALSE;

#	ifdef DEBUG
	pr_diag("Editor", "[mvlink_master] %s\n", mode);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same_start(mode, "cancel"))
		{
		ignore_partial();
		edit_complete();
		return FALSE;
		}

	/* Undo move by replacing link chains */
	if (same(mode, "undo"))
		{
#		ifdef DEBUG
		pr_diag("Editor", "[mvlink_master] Undo move by replacing link chain!\n");
#		endif /* DEBUG */

		/* Replace all link chains for this field */
		(void) replace_posted_chains("master", FpaCnone, FpaCnone);

		/* Reset link status */
		busy_cursor(TRUE);
		(void) release_links();
		(void) extract_links();
		(void) present_all();

		ActiveDfld->interp   = FALSE;
		ActiveDfld->intlab   = FALSE;
		ActiveDfld->saved    = FALSE;
		ActiveDfld->reported = FALSE;

		(void) ready_master_links(ActiveDfld, TRUE);
		(void) save_links();
		busy_cursor(FALSE);
		edit_complete();
		return FALSE;
		}

	/* Set up user defined cursor to be a mark */
	if (!mset)
		{
		mset = create_set("mark");
		mark = create_mark("","","",mpos,0.0);
		define_mspec(&mark->mspec, 32, 9, NULL, 0, False, 15.0, 0.0,
					 (HILITE) 2);
		add_item_to_set(mset,(ITEM) mark);
		/* Leave mark pointing to the original */
		}

	/* Initialize on startup */
	if (same(mode, "begin"))
		{
		(void) release_posted_chains();
		cancel_partial();
		}

	if (same(mode, "inter"))
		{
		cancel_partial();
		end_control_link();
		Inter = FALSE;
		}

	/* Are there any link chains left */
	if (ActiveDfld->nchain <= 0)
		{
		put_message("link-no-move");
		return drawn;
		}
	
	/* Keep on moving nodes until told to quit */
	while (TRUE)
		{

		/* Pick a link node and move it */
		if (Inter) put_message("link-inter");
		else       put_message("link-move");

		if (!ready_Xpoint(DnEdit, opos, &butt)) return drawn;
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}
		if (!inside_dn_window(DnEdit,opos))
			{
			put_message("edit-outside-map");
			(void) ignore_Xpoint();
			continue;
			}

		/* Search for the closest link node on any chain */
		ichain = closest_link_chain(EditTime, opos, ActiveDfld, PickTol,
					&inode, &ltype, &ambig, &dupl);

		/* Did we get close enough */
		if (ichain < 0)
			{
			put_message("link-too-far");
			(void) ignore_Xpoint();
			continue;
			}

		/* Did we hit an ambiguous one */
		if (ambig)
			{

			/* Allow moving nodes on duplicate link chains */
			if (!dupl)
				{
				put_message("link-ambig");
				(void) ignore_Xpoint();
				continue;
				}
			}

		/* Something got picked ... so set node to move */
		chain = ActiveDfld->chains[ichain];

		/* Move the control node chosen */
		if (ltype==LINK_CTL)
			{
			lnode = chain->nodes[inode];
			if (!lnode->there)
				{
				put_message("link-no-node");
				(void) ignore_Xpoint();
				continue;
				}
			}

		/* Move the link node at the current depiction time */
		else
			{
			/* >>>>> replace this ...
			if (!chain->nodes[EditTime]->there)
			... with this <<<<< */
			inode = which_lchain_node(chain, LchainNode, TimeList[EditTime].mplus);
			if (inode < 0 || !chain->nodes[inode]->there)
				{
				put_message("link-no-node");
				(void) ignore_Xpoint();
				continue;
				}
			lnode = chain->nodes[inode];
			}

		/* Post the existing link chains before moving */
		(void) post_link_chains("master", FpaCnone, FpaCnone);

		/* Set position for link node */
		/* >>>>> replace this ...
		copy_point(pos, chain->nodes[EditTime]->node);
		... with this <<<<< */
		copy_point(pos, lnode->node);
		dx = pos[X] - opos[X];
		dy = pos[Y] - opos[Y];
		define_mark_anchor(mark,pos,0.0);

		/* Now give the new position */
		put_message("link-move-rel");
		(void) utrack_Xpoint(DnEdit, mset, NULL, pos, &butt);
		pos[X] += dx;
		pos[Y] += dy;
		if (!inside_dn_window(DnEdit,pos))
			{
			put_message("edit-outside-map");
			(void) sleep(1);
			continue;
			}

		/* Change the link node position */
		put_message("link-move-proc");
		/* >>>>> replace this ...
		copy_point(chain->nodes[EditTime]->node, pos);
		chain->nodes[EditTime]->guess = FALSE;
		... with this <<<<< */
		if (ltype==LINK_CTL)
			{
			define_lnode_type(lnode, TRUE, FALSE, LchainControl);
			}
		else
			{
			define_lnode_type(lnode, TRUE, FALSE, LchainNode);
			}
		define_lnode_node(lnode, pos);

		/* Chain is ready for interpolation */
		chain->dointerp = TRUE;
		(void) interpolate_lchain(chain);

		/* Reset link status */
		busy_cursor(TRUE);
		post_partial(FALSE);
		edit_pending();
		(void) release_links();
		(void) extract_links();
		(void) present_all();

		ActiveDfld->interp   = FALSE;
		ActiveDfld->intlab   = FALSE;
		ActiveDfld->saved    = FALSE;
		ActiveDfld->reported = FALSE;

		(void) ready_master_links(ActiveDfld, TRUE);
		(void) revise_dependent_links(ActiveDfld);
		(void) save_links();
		busy_cursor(FALSE);
		}
	}

/***********************************************************************
*                                                                      *
*     m r g l i n k _ m a s t e r                                      *
*                                                                      *
*     Merge link chains from other fields.                             *
*                                                                      *
***********************************************************************/

LOGICAL	mrglink_master

	(
	STRING	mode,
	STRING	source,
	STRING	subsrc,
	STRING	element,
	STRING	level
	)

	{
	LCHAIN	chain, copy;
	SET		set;
	int		im, imx, ic;
	POINT	p0, p1, pz;
	MARK	mark0, mark1;
	CURVE	curv0, curv1;
	float	dx, dy, dx0, dy0, angle, cang, sang;
	int		butt;
	LOGICAL	drawn = FALSE;

	STRING			dpath;
	FLD_DESCRIPT	fdesc;

	/* Current chain state */
	static	enum
			{ Wait, Fetch, ReFetch, Pick, PickAll, RePick,
			  WaitAction, Merge, Translate, TranslateDone,
			  Centre, Rotate, RotateDone, ReDisplay, ReDisplayAction, ReStart }
			State = Wait;

	/* Is a fetch being done? */
	static	LOGICAL		fetch   = FALSE;

	/* Has a merge or translate or rotate been started or completed? */
	static	LOGICAL		mstart  = FALSE;
	static	LOGICAL		maction = FALSE;

	/* Move list: */
	/* - list of link chains to be moved */
	/* - cmove are pointers into merge field sets (before merge)    */
	/*    and then copies to be displayed in TempMeta (after merge) */
	/* - ccopy are copies to be added to ActiveDfld->chains         */
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
	static	STRING	melement = NullString;
	static	STRING	mlevel   = NullString;
	static	SET		mset     = NullSet;
	static	LOGICAL	fullset  = FALSE;

#	ifdef DEBUG
	pr_diag("Editor", "[mrglink_master] %s  State: %d\n", mode, State);
	pr_diag("Editor", "[mrglink_master]   Current reference time: %s\n", Stime);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same(mode, "cancel"))
		{
		put_message("edit-cancel");
		switch (State)
			{
			case Translate:
			case Centre:
			case Rotate:	ignore_partial();
							State = (maction)? ReDisplayAction: ReStart;
							break;

			case Pick:		State = (nmove > 0)? RePick: ReFetch;
							break;

			default:		ignore_partial();

							/* Empty picked link chain list */
							if (nmove > 0)
								{
								if (mstart && !maction)
									{
									for (im=nmove-1; im>=0; im--)
										{
										ic = ActiveDfld->nchain - nmove + im;
										(void) remove_link_chain(ActiveDfld, ic);
										}
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
							link_select(NullCal, FALSE);

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
								FREEMEM(melement);
								FREEMEM(mlevel);
								}

#							ifdef DEBUG_STRUCTURES
							pr_diag("Editor",
								"[mrglink_master] Cancel ... empty TempMeta %d fields\n",
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

		/* Empty picked link chain list */
		if (nmove > 0)
			{
			if (mstart && !maction)
				{
				for (im=nmove-1; im>=0; im--)
					{
					ic = ActiveDfld->nchain - nmove + im;
					(void) remove_link_chain(ActiveDfld, ic);
					}
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
			FREEMEM(melement);
			FREEMEM(mlevel);
			}

#		ifdef DEBUG_STRUCTURES
		pr_diag("Editor",
			"[mrglink_master] Cancel All ... empty TempMeta %d fields\n",
			TempMeta->numfld);
#		endif /* DEBUG_STRUCTURES */

		/* Empty display structure */
		empty_temp();
		present_all();
		link_select(NullCal, FALSE);
		return FALSE;
		}
	if (same(mode, "clear"))
		{
		return FALSE;
		}

	/* Undo merge by replacing link chains */
	if (same(mode, "undo"))
		{
#		ifdef DEBUG
		pr_diag("Editor", "[mrglink_master] Undo merge by replacing link chains!\n");
#		endif /* DEBUG */

		/* Replace all link chains for this field */
		(void) replace_posted_chains("master", CurrElement, CurrLevel);

		/* Reset link status */
		busy_cursor(TRUE);
		(void) release_links();
		(void) extract_links();
		(void) present_all();

		ActiveDfld->interp   = FALSE;
		ActiveDfld->intlab   = FALSE;
		ActiveDfld->saved    = FALSE;
		ActiveDfld->reported = FALSE;

		(void) ready_master_links(ActiveDfld, TRUE);
		(void) save_links();
		busy_cursor(FALSE);
		edit_complete();
		return FALSE;
		}

	/* Initialize on startup */
	if (same(mode, "begin"))
		{
		ignore_partial();
		if (EditUndoable)
			{
			(void) release_posted_chains();
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
			FREEMEM(melement);
			FREEMEM(mlevel);
			}

#		ifdef DEBUG_STRUCTURES
		pr_diag("Editor",
			"[mrglink_master] Begin ... empty TempMeta %d fields\n",
			TempMeta->numfld);
#		endif /* DEBUG_STRUCTURES */

		/* Empty display structure */
		empty_temp();
		present_all();
		mstart  = FALSE;
		maction = FALSE;

		link_select(NullCal, FALSE);
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
		link_select(NullCal, FALSE);
		State = Pick;
		return FALSE;
		}
	if (same(mode, "merge"))     State = Merge;
	if (same(mode, "translate")) State = Translate;
	if (same(mode, "rotate"))    State = Centre;

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
				return FALSE;

			/* Wait around for a "merge" or "translate" or "rotate" */
			case WaitAction:

				/* Keep returning until we get a "merge" or "translate" or "rotate" */
				put_message("lchain-merge-action");
				return FALSE;

			/* Fetch the requested merge field */
			case Fetch:

				fetch   = FALSE;
				mstart  = FALSE;
				maction = FALSE;

				if (same(source,  "-")) (void) strcpy(source,  "");
				if (same(subsrc,  "-")) (void) strcpy(subsrc,  "");
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
				pr_diag("Editor", "[mrglink_master] Fetching merge field\n");
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
					FpaF_ELEMENT_NAME,      element,
					FpaF_LEVEL_NAME,        level,
					FpaF_END_OF_LIST)) return FALSE;
				if (NotNull(mset))
					{
					mset = destroy_set(mset);
					FREEMEM(msource);
					FREEMEM(msubsrc);
					FREEMEM(melement);
					FREEMEM(mlevel);
					}
				mset = retrieve_linkset(&fdesc);
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
				melement = STRMEM(melement, element);
				mlevel   = STRMEM(mlevel,   level);

#				ifdef DEBUG
				pr_diag("Editor",
					"[mrglink_master] Merge field: %s %s  from: %s %s\n",
					element, level, source, subsrc);
#				endif /* DEBUG */

				/* Set presentation for merge link chains */
				invoke_link_set_presentation(mset);
#				ifdef DEBUG
				pr_diag("Editor",
					"[mrglink_master] Fetch - Presentation specs: %d  secondary: %d\n",
					mset->ncspec, mset->nxspec);
#				endif /* DEBUG */
				highlight_set(mset, 4);
				highlight_set_secondary(mset, 4);

				/* Display the fetched field */
				add_set_to_metafile(TempMeta, "l", "mset", "mset", mset);
				present_all();
				post_partial(TRUE);
				clear_message();

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[mrglink_master] Fetch ... adding mset to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Re-fetch the requested merge field */
			case ReFetch:

#				ifdef DEBUG
				pr_diag("Editor", "[mrglink_master] ReFetch\n");
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
				link_select(NullCal, FALSE);

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
					FREEMEM(melement);
					FREEMEM(mlevel);
					}

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[mrglink_master] Refetch ... empty TempMeta %d fields\n",
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
						link_select(NullCal, FALSE);
						}
					else
						{
						put_message("lchain-merge-pick2");
						link_select((CAL) cmove[0]->attrib, FALSE);
						}

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

						/* Make a copy of the picked link chain */
						copy = copy_lchain(chain);

						/* Match copied chain to current field */
						if (!prepare_link_chain(ActiveDfld, copy))
							{
							put_message("link-merge-out");
							(void) destroy_lchain(copy);
							(void) sleep(1);
							continue;
							}

						/* Make space for the copied link chain */
						put_message("lchain-picked");
						nmove++;
						if (nmove > amove)
							{
							amove = nmove;
							cmove = GETMEM(cmove, LCHAIN, amove);
							ccopy = GETMEM(ccopy, LCHAIN, amove);
							}
						ccopy[im] = copy;

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
					"[mrglink_master]   End of State = Pick loop!\n");

				/* Have we picked any link chains? */
				if (nmove > 0) link_select((CAL) cmove[0]->attrib, FALSE);
				else           link_select(NullCal, FALSE);

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
				nmove = 0;

				/* Highlight all the link chains and add them to the list */
				/* Note that the link chain reference time is adjusted to */
				/*  match the current depiction time!                     */
				set = find_mf_set(TempMeta, "lchain", "l", "mset", "mset");
				for (ic=0; ic<set->num; ic++)
					{

					/* Make a copy of each link chain */
					chain = (LCHAIN) set->list[ic];
					if (IsNull(chain)) continue;
					copy  = copy_lchain(chain);

					/* Match link chain to current field */
					if (!prepare_link_chain(ActiveDfld, copy))
						{
						(void) destroy_lchain(copy);
						continue;
						}

					/* Make space for the copied link chain */
					im = nmove++;
					if (nmove > amove)
						{
						amove = nmove;
						cmove = GETMEM(cmove, LCHAIN, amove);
						ccopy = GETMEM(ccopy, LCHAIN, amove);
						}
					ccopy[im] = copy;

					/* Highlight the picked link chain */
					/*  and add it to the list         */
					highlight_lchain(chain, 2);
					highlight_lchain_nodes(chain, 2, 2);
					widen_lchain(chain, 2.0);
					cmove[im] = chain;
					}

				/* Display picked link chains */
				present_temp(TRUE);

				/* Have we picked any link chains? */
				if (nmove > 0) link_select((CAL) cmove[0]->attrib, FALSE);
				else           link_select(NullCal, FALSE);

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Re-pick link chains to merge */
			case RePick:

#				ifdef DEBUG
				pr_diag("Editor", "[mrglink_master] RePick\n");
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
				link_select(NullCal, FALSE);

				/* Remove displayed picked chains, but keep the fetched field */
				if (fullset)
					{
					mset = take_mf_set(TempMeta, "lchain", "l", "mset", "mset");
					}

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[mrglink_master] RePick ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();

				/* Re-display the fetched field */
				fullset = TRUE;
#				ifdef DEBUG
				pr_diag("Editor",
					"[mrglink_master] RePick - Presentation specs: %d  secondary: %d\n",
					mset->ncspec, mset->nxspec);
#				endif /* DEBUG */
				highlight_set(mset, 4);
				highlight_set_secondary(mset, 4);
				add_set_to_metafile(TempMeta, "l", "mset", "mset", mset);
				present_all();
				post_partial(TRUE);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[mrglink_master] RePick ... adding mset to TempMeta (%d)\n",
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
						"[mrglink_master] Merge ... empty TempMeta %d fields\n",
						TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */

					/* Post the existing link chains before merging */
					(void) post_link_chains("master", CurrElement, CurrLevel);

					/* Empty display structure */
					empty_temp();
					fullset = FALSE;
					mset    = NullSet;

					/* Now add the picked link chains */
					for (im=0; im<nmove; im++)
						{
						ic = ActiveDfld->nchain++;
						ActiveDfld->chains = GETMEM(ActiveDfld->chains, LCHAIN,
													ActiveDfld->nchain);
						ActiveDfld->chains[ic] = ccopy[im];
						chain = copy_lchain(ccopy[im]);

						add_item_to_metafile(TempMeta, "lchain", "l",
											 "cmerge", "cmerge", (ITEM) chain);
						cmove[im] = chain;

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[mrglink_master] Adding link chain (%d) to TempMeta (%d)\n",
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
						"[mrglink_master]   Merge: %2d ... cmove: %x\n",
					im, cmove[im]);
#				endif /* DEBUG_STRUCTURES */

#				ifdef DEBUG
				pr_diag("Editor", "[mrglink_master] Merging\n");
#				endif /* DEBUG */

				ignore_partial();

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
						"[mrglink_master] Translate ... empty TempMeta %d fields\n",
						TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */

					/* Post the existing link chains before translating */
					(void) post_link_chains("master", CurrElement, CurrLevel);

					/* Empty display structure */
					empty_temp();
					fullset = FALSE;
					mset    = NullSet;

					/* Now add the picked link chains */
					for (im=0; im<nmove; im++)
						{
						ic = ActiveDfld->nchain++;
						ActiveDfld->chains = GETMEM(ActiveDfld->chains, LCHAIN,
													ActiveDfld->nchain);
						ActiveDfld->chains[ic] = ccopy[im];
						chain = copy_lchain(ccopy[im]);

						add_item_to_metafile(TempMeta, "lchain", "l",
											 "mmerge", "mmerge", (ITEM) chain);
						cmove[im] = chain;

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[mrglink_master] Adding link chain (%d) to TempMeta (%d)\n",
							im, TempMeta->numfld);
#						endif /* DEBUG_STRUCTURES */
						}
					}

				/* Highlight the picked link chains */
				for (im=0; im<nmove; im++)
					{
					chain = cmove[im];
					highlight_lchain(chain, 2);
					highlight_lchain_nodes(chain, 2, 2);
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
						"[mrglink_master]   Translate: %2d ... cmove: %x\n",
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
					"[mrglink_master] Adding mark/mark/curve to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = TranslateDone;
				continue;

			/* Translate the link chains */
			case TranslateDone:

#				ifdef DEBUG
				pr_diag("Editor", "[mrglink_master] Translating\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);

#				ifdef DEBUG_STRUCTURES
				for (im=0; im<nmove; im++)
					pr_diag("Editor",
						"[mrglink_master]   Translating: %2d ... cmove: %x\n",
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
						"[mrglink_master] Rotate ... empty TempMeta %d fields\n",
						TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */

					/* Post the existing link chains before rotating */
					(void) post_link_chains("master", CurrElement, CurrLevel);

					/* Empty display structure */
					empty_temp();
					fullset = FALSE;
					mset    = NullSet;

					/* Now add the picked link chains */
					for (im=0; im<nmove; im++)
						{
						ic = ActiveDfld->nchain++;
						ActiveDfld->chains = GETMEM(ActiveDfld->chains, LCHAIN,
													ActiveDfld->nchain);
						ActiveDfld->chains[ic] = ccopy[im];
						chain = copy_lchain(ccopy[im]);

						add_item_to_metafile(TempMeta, "lchain", "l",
											 "mmerge", "mmerge", (ITEM) chain);
						cmove[im] = chain;

#						ifdef DEBUG_STRUCTURES
						pr_diag("Editor",
							"[mrglink_master] Adding link chain (%d) to TempMeta (%d)\n",
							im, TempMeta->numfld);
#						endif /* DEBUG_STRUCTURES */
						}
					}

				/* Highlight the picked link chains */
				for (im=0; im<nmove; im++)
					{
					chain = cmove[im];
					highlight_lchain(chain, 2);
					highlight_lchain_nodes(chain, 2, 2);
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
						"[mrglink_master]   Rotate: %2d ... cmove: %x\n",
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
					"[mrglink_master] Adding mark to TempMeta (%d)\n",
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
					"[mrglink_master] Adding mark/curve to TempMeta (%d)\n",
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
					"[mrglink_master] Adding mark/curve to TempMeta (%d)\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = RotateDone;
				continue;

			/* Rotate the link chains */
			case RotateDone:

#				ifdef DEBUG
				pr_diag("Editor", "[mrglink_master] Rotating\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);

#				ifdef DEBUG_STRUCTURES
				for (im=0; im<nmove; im++)
					pr_diag("Editor",
						"[mrglink_master]   Rotating: %2d ... cmove: %x\n",
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
				pr_diag("Editor", "[mrglink_master] Redisplay\n");
#				endif /* DEBUG */

				/* Keep the fetched field (including picked link chains) */
				if (fullset)
					{
					mset = take_mf_set(TempMeta, "lchain", "l", "mset", "mset");
					}

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[mrglink_master] ReDisplay ... empty TempMeta %d fields\n",
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
						"[mrglink_master] ReDisplay ... adding mset to TempMeta (%d)\n",
						TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Move on to next stage */
				State = Pick;
				continue;

			/* Re-display the picked link chains (after merging) */
			case ReDisplayAction:

#				ifdef DEBUG
				pr_diag("Editor", "[mrglink_master] RedisplayAction\n");
#				endif /* DEBUG */

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[mrglink_master] ReDisplayAction ... empty TempMeta %d fields\n",
					TempMeta->numfld);
#				endif /* DEBUG_STRUCTURES */

				/* Empty display structure */
				empty_temp();

				/* Re-pick the picked link chains */
				for (im=0; im<nmove; im++)
					{
					ic = ActiveDfld->nchain - nmove + im;
					ccopy[im] = ActiveDfld->chains[ic];

					/* Reset presentation for re-picked link chain */
					invoke_link_chain_presentation(ccopy[im]);

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
						"[mrglink_master] Adding link chain (%d) to TempMeta (%d)\n",
						im, TempMeta->numfld);
#					endif /* DEBUG_STRUCTURES */
					}

				/* Re-display the picked link chains */
				present_all();
				post_partial(FALSE);

				/* Reset link status */
				busy_cursor(TRUE);
				(void) release_links();
				(void) extract_links();
				(void) present_all();

				ActiveDfld->interp   = FALSE;
				ActiveDfld->intlab   = FALSE;
				ActiveDfld->saved    = FALSE;
				ActiveDfld->reported = FALSE;

				(void) ready_master_links(ActiveDfld, TRUE);
				(void) save_links();
				busy_cursor(FALSE);
				edit_pending();

				/* Move on to next stage */
				State = WaitAction;
				continue;

			/* Start again */
			case ReStart:

#				ifdef DEBUG
				pr_diag("Editor", "[mrglink_master] ReStart\n");
#				endif /* DEBUG */

				/* Empty picked link chain list */
				if (nmove > 0)
					{
					for (im=nmove-1; im>=0; im--)
						{
						ic = ActiveDfld->nchain - nmove + im;
						(void) remove_link_chain(ActiveDfld, ic);
						}
					nmove = 0;
					amove = 0;
					FREEMEM(cmove);
					FREEMEM(ccopy);
					}
				link_select(NullCal, FALSE);

#				ifdef DEBUG_STRUCTURES
				pr_diag("Editor",
					"[mrglink_master] ReStart ... empty TempMeta %d fields\n",
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
*    m a s t e r _ l i n k _ f r a m e _ s t a t u s                   *
*                                                                      *
*    Get the link status of an individual frame.                       *
*                                                                      *
***********************************************************************/

STRING	master_link_frame_status

	(
	DFLIST	*dfld,
	int		itime
	)

	{
	LCHAIN	chain;
	int		mplus, ifirst, ilast, iprev, inext, ichain, inode, xplus, xnode;
	LOGICAL	ok, all, none;

	if (!dfld)                            return "DELETED";
	if (!dfld->there)                     return "DELETED";
	if (itime < 0)                        return "DELETED";
	if (itime >= NumTime)                 return "DELETED";
	if (!dfld->dolink)                    return "NONE";
	if (dfld->interp && dfld->intlab)     return "INTERP";
	if (dfld->interp)                     return "FIELD";
	if (dfld->linked)                     return "LINKED";
	if (dfld->nchain <= 0)                return "NONE";

	/* Set time for this keyframe */
	mplus = TimeList[itime].mplus;

	/* Search for any temporary nodes */
	for (ichain=0; ichain<dfld->nchain; ichain++)
		{
		chain = dfld->chains[ichain];
		/* >>>>> replace this ...
		if (!chain->nodes[itime]->there) continue;
		if (chain->nodes[itime]->guess)  return "PARTIAL";
		... with this <<<<< */
		inode = which_lchain_node(chain, LchainNode, mplus);
		if (inode < 0)                   continue;
		if (!chain->nodes[inode]->there) continue;
		if (chain->nodes[inode]->guess)  return "PARTIAL";
		}

	all  = TRUE;
	none = TRUE;

	/* Set start and end times for master links */
	ifirst = first_depict_time();
	ilast  = last_depict_time();

	/* Set the preceding active frame */
	iprev = prev_depict_time(itime);

	/* Search for a link backwards if there is a preceding frame */
	if (iprev >= ifirst)
		{
		ok    = FALSE;
		xplus = TimeList[iprev].mplus;
		for (ichain=0; ichain<dfld->nchain; ichain++)
			{
			chain = dfld->chains[ichain];
			/* >>>>> replace this ...
			if (!chain->nodes[itime]->there) continue;
			if (!chain->nodes[iprev]->there) continue;
			... with this <<<<< */
			inode = which_lchain_node(chain, LchainNode, mplus);
			if (inode < 0)                   continue;
			if (!chain->nodes[inode]->there) continue;
			xnode = which_lchain_node(chain, LchainNode, xplus);
			if (xnode < 0)                   continue;
			if (!chain->nodes[xnode]->there) continue;

			/* Found one */
			ok   = TRUE;
			none = FALSE;
			}
		if (!ok) all = FALSE;
		}

	/* Set the following active frame */
	inext = next_depict_time(itime);

	/* Search for a link forwards if there is a following frame */
	if (inext <= ilast)
		{
		ok    = FALSE;
		xplus = TimeList[inext].mplus;
		for (ichain=0; ichain<dfld->nchain; ichain++)
			{
			chain = dfld->chains[ichain];
			/* >>>>> replace this ...
			if (!chain->nodes[itime]->there) continue;
			if (!chain->nodes[inext]->there) continue;
			... with this <<<<< */
			inode = which_lchain_node(chain, LchainNode, mplus);
			if (inode < 0)                   continue;
			if (!chain->nodes[inode]->there) continue;
			xnode = which_lchain_node(chain, LchainNode, xplus);
			if (xnode < 0)                   continue;
			if (!chain->nodes[xnode]->there) continue;

			/* Found one */
			ok   = TRUE;
			none = FALSE;
			}
		if (!ok) all = FALSE;
		}

	if (all)  return "LINKED";
	if (none) return "NONE";
	return "PARTIAL";
	}

/***********************************************************************
*                                                                      *
*    v e r i f y _ m a s t e r _ l i n k s                             *
*    r e a d y _ m a s t e r _ l i n k s                               *
*                                                                      *
*    Make sure the links are legal.                                    *
*                                                                      *
*    Test the links on the given list of depiction times to check if   *
*    they are there.  Note that master links are not interpolated!     *
*                                                                      *
***********************************************************************/

LOGICAL	verify_master_links

	(
	DFLIST	*dfld,
	LOGICAL	report
	)

	{
	LOGICAL	valid;
	int		itime, ifirst, ilast, mfirst, mlast, mplus, inode;
	int		ichain;
	POINT	pos;
	LCHAIN	chain;
	LNODE	lnode;

	if (!dfld)         return FALSE;
	if (!dfld->dolink) return FALSE;

	/* See if we got interpolated but not saved at some point */
	if (dfld->interp && !dfld->saved)
		{
		(void) printf("... Field %s %s interpolated but not saved\n",
				dfld->element, dfld->level);
		dfld->interp   = FALSE;
		dfld->intlab   = FALSE;
		dfld->reported = FALSE;
		if (report) link_status(dfld);
		}

	/* Set start and end times for master links */
	ifirst = first_depict_time();
	ilast  = last_depict_time();

	/* Remove all link chains if no depiction fields */
	if (ifirst < 0 || ilast < 0)
		{
		for (ichain=dfld->nchain-1; ichain>=0; ichain--)
			{
			chain = dfld->chains[ichain];
			(void) remove_link_chain(dfld, ichain);
			}
		dfld->interp   = FALSE;
		dfld->intlab   = FALSE;
		dfld->saved    = FALSE;
		dfld->reported = FALSE;
		if (report) link_status(dfld);
		(void) save_links();
		return ready_master_links(dfld, report);
		}

	/* Set start and end times for all master link chains */
	mfirst = TimeList[ifirst].mplus;
	mlast  = TimeList[ilast].mplus;

	/* Modify the start and end times of each master link chain (if required) */
	valid = TRUE;
	for (ichain=0; ichain<dfld->nchain; ichain++)
		{
		chain = dfld->chains[ichain];
		if (chain->splus < mfirst || chain->splus > mlast
				|| chain->eplus < mfirst || chain->eplus > mlast)
			{
			valid = FALSE;
			if (chain->splus < mfirst) chain->splus = mfirst;
			if (chain->splus > mlast)  chain->splus = mlast;
			if (chain->eplus < mfirst) chain->eplus = mfirst;
			if (chain->eplus > mlast)  chain->eplus = mlast;
			define_lchain_default_attribs(chain);
			}
		}
	if (!valid)
		{
		dfld->interp   = FALSE;
		dfld->intlab   = FALSE;
		dfld->saved    = FALSE;
		dfld->reported = FALSE;
		}

	/* Check link nodes in all keyframes */
	for (itime=0; itime<NumTime; itime++)
		{

		/* Set time for this keyframe */
		mplus = TimeList[itime].mplus;

		/* All links in this keyframe must inside the map */
		for (ichain=0; ichain<dfld->nchain; ichain++)
			{
			chain = dfld->chains[ichain];
			/* >>>>> replace this ...
			lnode = chain->nodes[itime];
			... with this <<<<< */
			inode = which_lchain_node(chain, LchainNode, mplus);
			if (inode < 0)     continue;
			lnode = chain->nodes[inode];
			if (!lnode->there) continue;
			if (lnode->guess)  continue;

			/* Remove any link nodes that are outside the bounds */
			copy_point(pos, lnode->node);
			if (!inside_map_def(&MapProj->definition, pos))
				{

				/* No good any more - disable this link node */
				empty_lnode(lnode);
				chain->dointerp = TRUE;

				dfld->interp   = FALSE;
				dfld->intlab   = FALSE;
				dfld->saved    = FALSE;
				dfld->reported = FALSE;

				pr_warning("Timelink", "Removing master link node outside map!\n");
				}
			}
		}

	/* All link chains must have at least one link node */
	/* Check backwards so we can remove chains without link nodes! */
	for (ichain=dfld->nchain-1; ichain>=0; ichain--)
		{
		chain = dfld->chains[ichain];
		valid = FALSE;
		for (itime=0; itime<NumTime; itime++)
			{
			/* >>>>> replace this ...
			lnode = chain->nodes[itime];
			if (lnode->there)
			... with this <<<<< */
			mplus = TimeList[itime].mplus;
			inode = which_lchain_node(chain, LchainNode, mplus);
			if (inode >= 0)
				{
				lnode = chain->nodes[inode];
				if (lnode->there)
					{
					valid = TRUE;
					break;
					}
				}
			}
		if (!valid)
			{
			(void) remove_link_chain(dfld, ichain);
			dfld->interp   = FALSE;
			dfld->intlab   = FALSE;
			dfld->saved    = FALSE;
			dfld->reported = FALSE;
			}
		}

	if (report) link_status(dfld);
	(void) save_links();
	return ready_master_links(dfld, report);
	}

/**********************************************************************/

LOGICAL ready_master_links

	(
	DFLIST	*dfld,
	LOGICAL	report
	)

	{
	int		itime, jtime, ichain, mplus, inode, jnode, ok;
	LCHAIN	chain;

	if (!dfld)         return FALSE;
	if (!dfld->dolink) return FALSE;

	dfld->there = (LOGICAL) (dfld->nchain > 0);

	/* Find the first active frame */
	itime = first_depict_time();
	if (itime < 0)
		{
		dfld->linked   = FALSE;
		dfld->interp   = FALSE;
		dfld->intlab   = FALSE;
		dfld->saved    = FALSE;
		dfld->reported = FALSE;
		if (report) link_status(dfld);
		return FALSE;
		}

	/* Check running pairs of keyframes */
	/* All pairs must have at least one link across them */
	for (; itime<NumTime-1; itime=jtime)
		{
		/* Find the next active frame */
		jtime = next_depict_time(itime);
		if (jtime < 0)        break;
		if (jtime >= NumTime) break;

		/* Look for a link chain that covers this pair of frames */
		ok = FALSE;
		for (ichain=0; ichain<dfld->nchain; ichain++)
			{
			chain = dfld->chains[ichain];

			/* Find a node at each time */
			mplus = TimeList[itime].mplus;
			inode = which_lchain_node(chain, LchainNode, mplus);
			mplus = TimeList[jtime].mplus;
			jnode = which_lchain_node(chain, LchainNode, mplus);

			/* >>>>> replace this ...
			if (!dfld->chains[ichain]->nodes[itime]->there) continue;
			if (!dfld->chains[ichain]->nodes[jtime]->there) continue;
			... with this <<<<< */
			if (inode < 0 || jnode < 0)      continue;
			if (!chain->nodes[inode]->there) continue;
			if (!chain->nodes[jnode]->there) continue;

			/* Found a link across this pair */
			ok = TRUE;
			break;
			}

		/* No link across this pair */
		if (!ok)
			{
			/* Check if anything is changing */
			if (dfld->linked || dfld->interp || dfld->intlab
					|| dfld->saved) dfld->reported = FALSE;
			dfld->linked = FALSE;
			dfld->interp = FALSE;
			dfld->intlab = FALSE;
			dfld->saved  = FALSE;
			if (report) link_status(dfld);
			return FALSE;
			}
		}

	/* Got all the way - must be ready */
	if (!dfld->linked) dfld->reported = FALSE;
	dfld->linked = TRUE;
	if (report) link_status(dfld);
	return TRUE;
	}
