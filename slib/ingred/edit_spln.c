/***********************************************************************
*                                                                      *
*     e d i t _ s p l i n e . c                                        *
*                                                                      *
*     Assorted operations for bspline surface fields.                  *
*     i.e. continuous fields, represented by a B-spline surface        *
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

#undef DEBUG_SMOOTH
#undef DEBUG_STOMP
#undef DEBUG_ROTATE

/* Default weighting factors for sfit_surface...() funtions */
static	const	float	WgtFactorPoke = 1.00;
static	const	float	WgtFactorDrag = 1.10;

/***********************************************************************
*                                                                      *
*     d e f i n e _ r a m p                                            *
*                                                                      *
*     Define radius of influence weight function for wide edits        *
*                                                                      *
***********************************************************************/

static	float	ramp(float, float);

static	float	ramp(float d, float dmax)
	{
	if (dmax <= 0) return 0;
	if (d <= 0)    return 0;
	if (d > dmax)  return 0;

	/* >>> original
	return ((dmax+1 - d) / (dmax+1));
	<<< */
	/* >>> newer
	return ((dmax - d)*(dmax - d) / (dmax*dmax));
	<<< */
	/* >>> even newer
	dmax *= 1.2;
	return ((dmax - d)*(dmax - d) / (dmax*dmax));
	<<< */
	dmax += 1.0;
	dmax *= 0.8;
	return ((dmax - d)*(dmax - d) / (dmax*dmax));
	}

/***********************************************************************
*                                                                      *
*     d e f i n e _ s p r e a d                                        *
*                                                                      *
*     Interpret "spread" button for edit area of influence             *
*                                                                      *
***********************************************************************/

static	float	define_spread(void);

static	float	define_spread(void)
	{
	float	spfact, spread, grid, r;

	spfact = SpreadFact;
	spfact = MAX(spfact, 0.0);
	spfact = MIN(spfact, 100.0);
	spread = spfact * MaxSpread * zoom_factor()/100.0;
	(void) printf("[define_spread] spread: %f\n", spread);

	grid   = EditSfc->sp.gridlen;
	r      = grid * (spread + 1);
	define_circle_echo(r, r);

	return spread;
	}

/***********************************************************************
*                                                                      *
*     r e a d y _ s p l i n e _ f i e l d                              *
*                                                                      *
***********************************************************************/

LOGICAL	edit_ready_spline_field

	(void)

	{
	if (!EditUndoable)       edit_can_create(FALSE);
	if (!EditUndoable)       return TRUE;
	if (NotNull(OldSurface)) edit_can_create(FALSE);
	if (NotNull(OldSurface)) return TRUE;

	/* No field is present */
	put_message("spline-no-field");
	edit_can_create(FALSE);
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*     p o k e _ s p l i n e                                            *
*     p o k e _ s p l i n e _ 2 D                                      *
*                                                                      *
***********************************************************************/

static	void	calc_delta_2D(SURFACE, POINT, float, double, double,
								double *, double *);

static	void	calc_delta_2D

	(
	SURFACE	sfc,
	POINT	pos,
	float	sfact,
	double	dmag,
	double	ddir,
	double	*dx,
	double	*dy
	)

	{
	LOGICAL	valid;
	double	xval, yval, mag, dir;

	valid = eval_sfc_UV(sfc, pos, &xval, &yval);
	valid = eval_sfc_MD(sfc, pos, &mag, &dir);

	if (dmag > 0 && mag < (2*dmag))
		{
		mag *= 1.5;
		}
	else if (dmag < 0 && mag < (-2*dmag))
		{
		mag *= 0.5;
		}
	else
		{
		mag += dmag;
		}

	dir += ddir;

	*dx = (mag * fpa_cosdeg(dir) - xval) * sfact;
	*dy = (mag * fpa_sindeg(dir) - yval) * sfact;
	}

/**********************************************************************/

LOGICAL	edit_poke_spline

	(
	STRING	mode,
	float	delta
	)

	{
	POINT	p;
	int		dz, butt;
	float	spread;

	LOGICAL	drawn = FALSE;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_poke_spline] %s\n", mode);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same_start(mode, "cancel"))
		{
		circle_echo(FALSE);
		return FALSE;
		}

	if (same(mode, "clear"))
		{
		return FALSE;
		}

	/* See if surface exists */
	if (!EditSfc) return FALSE;

	/* Truncate delta */
	dz = NINT(delta*10.0);	delta = 0.1 * (float) dz;
	if (delta == 0.0)
		{
		circle_echo(FALSE);
		put_message("spline-no-delta");
		return FALSE;
		}

	/* Interpret spread factor */
	spread = define_spread();

	/* Perform the poke */
	while (TRUE)
		{
		/* Put up a prompt */
		put_message("spline-poke", delta);

		/* Get the point */
		if (!ready_Xpoint(DnEdit, NullPoint, &butt))
			{
			circle_echo(TRUE);
			return drawn;
			}
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}
		(void) pick_Xpoint(DnEdit, 0, p, &butt);
		if (!inside_dn_window(DnEdit, p))
			{
			put_message("edit-outside-map");
			(void) sleep(1);
			continue;
			}

		/* Perform an array of pokes for wider edits */
		if (spread >= 1.0)
			{
			float	grid, gdist, wdel;
			double	val;
			POINT	plist[320];	/* Dimension should be close to: */
			float	vlist[320];	/*    Pi*LimSpread^2 + 1 */
			int		nlist, ix, iy, psize, vsize;
			LOGICAL	valid;

#			ifdef DEBUG
			pr_diag("Editor",
					"[edit_poke_spline] Poking %.1f (%.1f)\n", delta, spread);
#			endif /* DEBUG */

			busy_cursor(TRUE);
			put_message("spline-poke-proc");

			valid = eval_sfc(EditSfc, p, &val);
			val  += delta;

			/* Build the poke list */
			grid  = EditSfc->sp.gridlen;
			psize = sizeof(plist) / sizeof(POINT);
			vsize = sizeof(vlist) / sizeof(float);
			valid = TRUE;
			nlist = 0;
			for (ix=0; ix<(spread+1) && valid; ix++)
				{
				for (iy=0; iy<(spread+1) && valid; iy++)
					{
					gdist = hypot((double)ix, (double)iy);
					if (gdist > spread) continue;
					wdel = delta * ramp(gdist, spread);

					valid = (LOGICAL) (nlist<psize && nlist<vsize);
					if (!valid) break;
					plist[nlist][X] = p[X] + ix*grid;
					plist[nlist][Y] = p[Y] + iy*grid;
					if (inside_surface_spline(EditSfc, plist[nlist]))
						vlist[nlist++] = wdel;

					if (ix>0 && iy>0)
						{
						valid = (LOGICAL) (nlist<psize && nlist<vsize);
						if (!valid) break;
						plist[nlist][X] = p[X] - ix*grid;
						plist[nlist][Y] = p[Y] - iy*grid;
						if (inside_surface_spline(EditSfc, plist[nlist]))
							vlist[nlist++] = wdel;
						}

					if (ix>0)
						{
						valid = (LOGICAL) (nlist<psize && nlist<vsize);
						if (!valid) break;
						plist[nlist][X] = p[X] - ix*grid;
						plist[nlist][Y] = p[Y] + iy*grid;
						if (inside_surface_spline(EditSfc, plist[nlist]))
							vlist[nlist++] = wdel;
						}

					if (iy>0)
						{
						valid = (LOGICAL) (nlist<psize && nlist<vsize);
						if (!valid) break;
						plist[nlist][X] = p[X] + ix*grid;
						plist[nlist][Y] = p[Y] - iy*grid;
						if (inside_surface_spline(EditSfc, plist[nlist]))
							vlist[nlist++] = wdel;
						}
					}
				}

			/* Make sure we got all the hit points */
			if (!valid)
				{
				pr_warning("Editor",
					"[edit_poke_spline] Poke buffers too small:");
				(void) printf(" plist[%d] vlist[%d]\n", psize, vsize);
				put_message("spline-no-pokes");
				(void) sleep(2);
				busy_cursor(FALSE);
				continue;
				}

			/* Do the edit (with additional correction at the centre) */
			put_message("spline-adjust");
			sfit_surface(EditSfc, nlist, plist, vlist,
						 1.0, WgtFactorPoke, FALSE, TRUE);
			edit_surface(EditSfc, p, (float)val, TRUE, TRUE);
			}

		/* Otherwise do a single poke */
		else
			{

#			ifdef DEBUG
			pr_diag("Editor", "[edit_poke_spline] Poking %.1f\n", delta);
#			endif /* DEBUG */

			/* Poke the surface */
			busy_cursor(TRUE);
			put_message("spline-adjust");
			edit_surface(EditSfc, p, delta, FALSE, TRUE);
			}

		drawn = TRUE;
		if (EditUndoable) post_mod("surface");

		/* Modify labels and highs and lows accordingly */
		if (recompute_surface_labs(EditSfc, EditLabs, EditRetain))
			{
			if (EditUndoable) post_mod("labs");
			}

		/* Re-display the surface */
		put_message("edit-done");
		present_all();
		busy_cursor(FALSE);
		}
	}

/**********************************************************************/

LOGICAL	edit_poke_spline_2D

	(
	STRING	mode,
	float	dmag,
	float	ddir
	)

	{
	POINT	p;
	int		dz, butt;
	float	spread;

	LOGICAL	drawn = FALSE;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_poke_spline_2D] %s\n", mode);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same_start(mode, "cancel"))
		{
		circle_echo(FALSE);
		return FALSE;
		}

	if (same(mode, "clear"))
		{
		return FALSE;
		}

	/* See if surface exists */
	if (!EditSfc) return FALSE;

	/* Truncate deltas */
	dz = NINT(dmag*10.0);	dmag = 0.1 * (float) dz;
	dz = NINT(ddir*10.0);	ddir = 0.1 * (float) dz;
	if (dmag == 0.0 && ddir == 0)
		{
		circle_echo(FALSE);
		put_message("spline-no-delta");
		return FALSE;
		}

	/* Interpret spread factor */
	spread = define_spread();

	/* Perform the poke */
	while (TRUE)
		{
		/* Put up a prompt */
		put_message("spline-poke-md", dmag, ddir);

		/* Get the point */
		if (!ready_Xpoint(DnEdit, NullPoint, &butt))
			{
			circle_echo(TRUE);
			return drawn;
			}
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}
		(void) pick_Xpoint(DnEdit, 0, p, &butt);
		if (!inside_dn_window(DnEdit, p))
			{
			put_message("edit-outside-map");
			(void) sleep(1);
			continue;
			}

		/* Perform an array of pokes for wider edits */
		if (spread >= 1.0)
			{
			float	grid, gdist, sf;
			double	dx, dy, xval, yval;
			POINT	q;
			POINT	plist[320];	/* Dimension should be close to: */
			float	ulist[320];	/*    Pi*LimSpread^2 + 1 */
			float	vlist[320];
			int		nlist, ix, iy, psize, vsize;
			LOGICAL	valid;

#			ifdef DEBUG
			pr_diag("Editor", "[edit_poke_spline_2D] Poking %.1f %.1f (%.1f)\n",
					dmag, ddir, spread);
#			endif /* DEBUG */

			busy_cursor(TRUE);
			put_message("spline-poke-proc");

			/* Compute target at centre of poke */
			(void) eval_sfc_UV(EditSfc, p, &xval, &yval);
			calc_delta_2D(EditSfc, p, 1.0, dmag, ddir, &dx, &dy);
			xval += dx;
			yval += dy;

			/* Build the poke list */
			grid  = EditSfc->sp.gridlen;
			psize = sizeof(plist) / sizeof(POINT);
			vsize = sizeof(vlist) / sizeof(float);
			valid = TRUE;
			nlist = 0;
			for (ix=0; ix<(spread+1) && valid; ix++)
				{
				for (iy=0; iy<(spread+1) && valid; iy++)
					{
					gdist = hypot((double)ix, (double)iy);
					if (gdist > spread) continue;
					sf = ramp(gdist, spread);

					valid = (LOGICAL) (nlist<psize && nlist<vsize);
					if (!valid) break;
					q[X] = p[X] + ix*grid;
					q[Y] = p[Y] + iy*grid;
					if (inside_surface_spline(EditSfc, q))
						{
						calc_delta_2D(EditSfc, q, sf, dmag, ddir, &dx, &dy);
						copy_point(plist[nlist], q);
						ulist[nlist] = dx;
						vlist[nlist] = dy;
						nlist++;
						}

					if (ix>0 && iy>0)
						{
						valid = (LOGICAL) (nlist<psize && nlist<vsize);
						if (!valid) break;
						q[X] = p[X] - ix*grid;
						q[Y] = p[Y] - iy*grid;
						if (inside_surface_spline(EditSfc, q))
							{
							calc_delta_2D(EditSfc, q, sf, dmag, ddir, &dx, &dy);
							copy_point(plist[nlist], q);
							ulist[nlist] = dx;
							vlist[nlist] = dy;
							nlist++;
							}
						}

					if (ix>0)
						{
						valid = (LOGICAL) (nlist<psize && nlist<vsize);
						if (!valid) break;
						q[X] = p[X] - ix*grid;
						q[Y] = p[Y] + iy*grid;
						if (inside_surface_spline(EditSfc, q))
							{
							calc_delta_2D(EditSfc, q, sf, dmag, ddir, &dx, &dy);
							copy_point(plist[nlist], q);
							ulist[nlist] = dx;
							vlist[nlist] = dy;
							nlist++;
							}
						}

					if (iy>0)
						{
						valid = (LOGICAL) (nlist<psize && nlist<vsize);
						if (!valid) break;
						q[X] = p[X] + ix*grid;
						q[Y] = p[Y] - iy*grid;
						if (inside_surface_spline(EditSfc, q))
							{
							calc_delta_2D(EditSfc, q, sf, dmag, ddir, &dx, &dy);
							copy_point(plist[nlist], q);
							ulist[nlist] = dx;
							vlist[nlist] = dy;
							nlist++;
							}
						}
					}
				}

			/* Make sure we got all the hit points */
			if (!valid)
				{
				pr_warning("Editor",
					"[edit_poke_spline_2D] Poke buffers too small:");
				(void) printf(" plist[%d] vlist[%d]\n", psize, vsize);
				put_message("spline-no-pokes");
				(void) sleep(2);
				busy_cursor(FALSE);
				continue;
				}

			/* Do the edit (with additional correction at the centre) */
			put_message("spline-adjust");
			sfit_surface_2D(EditSfc, nlist, plist, ulist, vlist,
							1.0, WgtFactorPoke, FALSE, TRUE);
			edit_surface_2D(EditSfc, p, xval, yval, TRUE, TRUE);
			}

		/* Otherwise do a single poke */
		else
			{
			double	dx, dy;

#			ifdef DEBUG
			pr_diag("Editor", "[edit_poke_spline_2D] Poking %.1f %.1f (%.1f)\n",
					dmag, ddir, spread);
#			endif /* DEBUG */

			/* Poke the surface */
			busy_cursor(TRUE);
			put_message("spline-adjust");
			calc_delta_2D(EditSfc, p, 1.0, dmag, ddir, &dx, &dy);
			edit_surface_2D(EditSfc, p, dx, dy, FALSE, TRUE);
			}

		drawn = TRUE;
		if (EditUndoable) post_mod("surface");

		/* Modify labels and highs and lows accordingly */
		if (recompute_surface_labs(EditSfc, EditLabs, EditRetain))
			{
			if (EditUndoable) post_mod("labs");
			}

		/* Re-display the surface */
		put_message("edit-done");
		present_all();
		busy_cursor(FALSE);
		}
	}

/***********************************************************************
*                                                                      *
*     s t o m p _ s p l i n e                                          *
*     s t o m p _ s p l i n e _ 2 D                                    *
*                                                                      *
***********************************************************************/

LOGICAL	edit_stomp_spline

	(
	STRING	mode,
	STRING	name,
	float	cdelta,
	float	edge
	)

	{
	CURVE	curve;
	int		nlines;
	LINE	*lines;
	LOGICAL	valid, inside;
	int		dz, ix, iy;
	float	xm, ym, gdist;
	float	spread, edelta, a, spint;
	double	val;
	POINT	pm;
	int		butt;
	LOGICAL	drawn = FALSE;

	/* Current state: */
	/* - where should we resume? */
	static	enum	{ Draw, WaitAction, Edit } State = Draw;

	/* Boundary curve set: */
	/* - defines undistorted region */
	/* - used in cursor tracking */
	static	SET		bset  = NULL;
	static	CURVE	bound = NULL;

	/* Stomp delta and edge smoothing */
	static	float	Sdelta = 0.0;
	static	float	Sedge  = 0.0;

	/* Temporary grid buffer: */
	/* - results of re-interpolation */
	/* - used to refit the new surface */
	static	float	grid   = 0.0;
	static	int		m      = 0;
	static	int		n      = 0;
	static	int		nx     = 0;
	static	int		ny     = 0;
	static	int		mx     = 0;
	static	int		my     = 0;
	static	float	**gbuf = NULL;
	static	float	*gdat  = NULL;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_stomp_spline] %s  State: %d\n", mode, State);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same(mode, "cancel"))
		{
		put_message("edit-cancel");
		ignore_partial();
		circle_echo(FALSE);
		drawing_control(FALSE);
		edit_can_proceed(FALSE);
		edit_allow_preset_outline(FALSE);

		if (bset)
			{
			bset  = NULL;
			bound = NULL;
			}
		grid = 0.0;

		/* Empty display structure */
		empty_temp();
		present_all();
		clear_message();
		return FALSE;
		}
	if (same(mode, "cancel all"))
		{
		ignore_partial();
		drawing_control(FALSE);
		edit_allow_preset_outline(FALSE);

		if (bset)
			{
			bset  = NULL;
			bound = NULL;
			}
		grid = 0.0;

		/* Empty display structure */
		empty_temp();
		present_all();
		clear_message();
		return FALSE;
		}

	if (same(mode, "clear"))
		{
		grid = 0.0;
		return FALSE;
		}

	/* See if surface exists */
	if (!EditSfc) return FALSE;

	/* Construct temporary fields on startup */
	if (same(mode, "begin") || (grid <= 0.0))
		{
		/* Set up temporary grid buffer */
		grid = EditSfc->sp.gridlen;
		m    = EditSfc->sp.m;
		n    = EditSfc->sp.n;
		nx   = m - 2;
		ny   = n - 2;
		if ((nx != mx) || (ny != my))
			{
			mx   = nx;
			my   = ny;
			gdat = GETMEM(gdat, float, (nx*ny));
			gbuf = GETMEM(gbuf, float *, ny);
			for (iy=0; iy<ny; iy++) gbuf[iy] = gdat + iy*nx;
			}
		drawing_control(FALSE);
		edit_can_proceed(FALSE);
		empty_temp();

		State = Draw;
		}

	/* Set state for Right button "proceed" */
	if (same(mode, "proceed")) State = Edit;

	/* Set state for Right button "preset outline" */
	if (same(mode, "preset outline"))
		{

		/* Check for last stomp outline or named outline */
		if (same(name, FpaEditorLastStompBoundary))
			curve = retrieve_stomp_outline();
		else
			curve = retrieve_named_outline(name);
		if (IsNull(curve))
			{
			if (same(name, FpaEditorLastStompBoundary))
				{
				put_message("edit-no-stomp-outline");
				pr_warning("Editor",
					"[edit_stomp_spline] No stomp outline drawn yet!\n");
				}
			else
				{
				put_message("edit-no-preset-outline", name);
				pr_error("Editor",
					"[edit_stomp_spline] No preset map outline: \"%s\"\n",
					name);
				}
			(void) sleep(1);
			State = Draw;
			}

		/* Display the outline */
		else
			{
			define_lspec(&curve->lspec, 255, 0, NULL, False, 2.0, 0.0,
						 (HILITE) 2);
			add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) curve);
			present_temp(TRUE);
			post_partial(TRUE);
			State = WaitAction;
			}
		}

#	ifdef DEBUG_STOMP
	pr_diag("edit_stomp_spline",
		"  mode: %s  cdelta: %.2f  edge: %.2f\n", mode, cdelta, edge);
#	endif /* DEBUG_STOMP */

	/* Reset stomp delta and edge smoothing (if required) */
	if (edge > -10)
		{
		dz     = NINT(cdelta*10.0);
		Sdelta = 0.1 * (float) dz;
		edge   = MIN(edge, 100);
		edge   = MAX(edge, 0);
		Sedge  = edge;
		}

	/* Repeat until told to quit */
	while (TRUE)
		{

#		ifdef DEBUG_STOMP
		pr_diag("edit_stomp_spline",
			"  mode: %s  State: %d  Sdelta: %.2f  Sedge: %.2f\n",
			mode, State, Sdelta, Sedge);
#		endif /* DEBUG_STOMP */

		switch (State)
			{

			/* Draw boundary of affected area */
			case Draw:

				/* Draw the boundary */
				post_partial(FALSE);
				empty_temp();
				put_message("spline-stomp-draw");
				edit_allow_preset_outline(TRUE);

				if (ready_Xcurve()) butt = LeftButton;
				else if (!ready_Xpoint(DnEdit, NULL, &butt)) return drawn;

				/* Invalid button */
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					ignore_Xpoint();
					continue;
					}
				drawn = FALSE;

				/* Draw the curve */
				drawing_control(TRUE);
				if (!ready_Xcurve())
					{
					if (drawing_Xcurve()) return drawn;
					(void) draw_Xcurve(DnEdit, FilterRes*4, SplineRes*4, TRUE);
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

				/* Extract and display curve */
				curve = create_curve("", "", "");
				add_line_to_curve(curve, lines[0]);
				post_stomp_outline(curve);
				reset_pipe();
				define_lspec(&curve->lspec, 255, 0, NULL, False, 2.0, 0.0,
							 (HILITE) 2);
				add_item_to_metafile(TempMeta, "curve", "c", "", "",
									 (ITEM) curve);
				present_temp(TRUE);
				post_partial(TRUE);

				/* Move on to next stage */
				State = WaitAction;
				continue;

			/* Wait around for a "proceed" */
			case WaitAction:

				/* Check delta */
				if (Sdelta == 0.0)
					{
					put_message("spline-no-delta");
					circle_echo(FALSE);
					edit_can_proceed(FALSE);
					edit_allow_preset_outline(FALSE);
					return drawn;
					}

				/* Interpret spread factor */
				spread = define_spread();
				circle_echo(TRUE);

				/* Keep returning until we get a "Proceed" */
				edit_can_proceed(TRUE);
				edit_allow_preset_outline(FALSE);
				put_message("spline-stomp-set");
				return FALSE;

			/* Perform the edit */
			case Edit:

				/* Check delta */
				if (Sdelta == 0.0)
					{
					put_message("spline-no-delta");
					circle_echo(FALSE);
					edit_can_proceed(FALSE);
					State = WaitAction;
					return drawn;
					}

				/* Interpret spread factor */
				spread = define_spread();
				circle_echo(TRUE);

#				ifdef DEBUG
				pr_diag("Editor", "[edit_stomp_spline] Editing\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);

				/* Calculate edge delta and interior spread */
				bset   = find_mf_set(TempMeta, "curve", "c", "", "");
				bound  = (CURVE) bset->list[0];
				highlight_set(bset, 0);
				curve_properties(bound, NullChar, NullChar, &a, NullFloat);
				spint  = sqrt(a/3.14) / grid;
				edelta = Sdelta * Sedge / 100;

				/* Build new grid with adjusted values inside given boundary */
				put_message("spline-adjust");
				for (ix=0; ix<nx; ix++)
					{
					xm    = ix*grid;
					pm[X] = xm;
					for (iy=0; iy<ny; iy++)
						{
						ym     = iy*grid;
						pm[Y]  = ym;
						curve_test_point(bound, pm, &gdist, NULL, NULL,
										 &inside, NULL);
						gdist /= grid;
						valid  = eval_sfc_unmapped(EditSfc, pm, &val);
						gbuf[iy][ix] = (float) val;
						if (inside)
							{
							if (gdist >= spint)
								gbuf[iy][ix] += Sdelta;
							else
								{
								a = (spint-gdist)/spint;
								gbuf[iy][ix] += Sdelta - (Sdelta-edelta)*a*a;
								}
							}
						else if (gdist <= spread)
								gbuf[iy][ix] += edelta * ramp(gdist, spread);
						}
					}

				/* Fit surface to new grid values */
				grid_surface(EditSfc, grid, nx, ny, gbuf);
				contour_surface(EditSfc);
				if (EditUndoable) post_mod("surface");

				/* Modify labels and highs and lows accordingly */
				if (recompute_surface_labs(EditSfc, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				/* Reset boundary curve and reset displacement fields */
				bset  = NULL;
				bound = NULL;

				/* Re-display the surface */
				put_message("edit-done");
				empty_temp();
				present_all();
				busy_cursor(FALSE);
				edit_can_proceed(FALSE);

				/* Move on to next stage */
				State = Draw;
				continue;
			}
		}
	}

/**********************************************************************/

LOGICAL	edit_stomp_spline_2D

	(
	STRING	mode,
	STRING	name,
	float	dmag,
	float	ddir,
	float	edge
	)

	{
	CURVE	curve;
	int		nlines;
	LINE	*lines;
	LOGICAL	valid, inside;
	int		dz, ix, iy;
	float	xm, ym, gdist;
	float	spread, erat, a, spint, sf;
	double	xval, yval, dx, dy;
	POINT	pm;
	int		butt;
	LOGICAL	drawn = FALSE;

	/* Current state: */
	/* - where should we resume? */
	static	enum	{ Draw, WaitAction, Edit } State = Draw;

	/* Boundary curve set: */
	/* - defines undistorted region */
	/* - used in cursor tracking */
	static	SET		bset  = NULL;
	static	CURVE	bound = NULL;

	/* Stomp deltas and edge smoothing */
	static	float	Smag  = 0.0;
	static	float	Sdir  = 0.0;
	static	float	Sedge = 0.0;

	/* Temporary grid buffer: */
	/* - results of re-interpolation */
	/* - used to refit the new surface */
	static	float	grid   = 0.0;
	static	int		m      = 0;
	static	int		n      = 0;
	static	int		nx     = 0;
	static	int		ny     = 0;
	static	int		mx     = 0;
	static	int		my     = 0;
	static	float	**gxbuf = NULL;
	static	float	**gybuf = NULL;
	static	float	*gxdat  = NULL;
	static	float	*gydat  = NULL;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_stomp_spline_2D] %s  State: %d\n", mode, State);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same(mode, "cancel"))
		{
		put_message("edit-cancel");
		ignore_partial();
		circle_echo(FALSE);
		drawing_control(FALSE);
		edit_can_proceed(FALSE);
		edit_allow_preset_outline(FALSE);

		if (bset)
			{
			bset  = NULL;
			bound = NULL;
			}
		grid = 0.0;

		/* Empty display structure */
		empty_temp();
		present_all();
		clear_message();
		return FALSE;
		}
	if (same(mode, "cancel all"))
		{
		ignore_partial();
		drawing_control(FALSE);
		edit_allow_preset_outline(FALSE);

		if (bset)
			{
			bset  = NULL;
			bound = NULL;
			}
		grid = 0.0;

		/* Empty display structure */
		empty_temp();
		present_all();
		clear_message();
		return FALSE;
		}

	if (same(mode, "clear"))
		{
		grid = 0.0;
		return FALSE;
		}

	/* See if surface exists */
	if (!EditSfc) return FALSE;

	/* Construct temporary fields on startup */
	if (same(mode, "begin") || (grid <= 0.0))
		{
		/* Set up temporary grid buffer */
		grid = EditSfc->sp.gridlen;
		m    = EditSfc->sp.m;
		n    = EditSfc->sp.n;
		nx   = m - 2;
		ny   = n - 2;
		if ((nx != mx) || (ny != my))
			{
			mx   = nx;
			my   = ny;
			gxdat = GETMEM(gxdat, float, (nx*ny));
			gydat = GETMEM(gydat, float, (nx*ny));
			gxbuf = GETMEM(gxbuf, float *, ny);
			gybuf = GETMEM(gybuf, float *, ny);
			for (iy=0; iy<ny; iy++)
				{
				gxbuf[iy] = gxdat + iy*nx;
				gybuf[iy] = gydat + iy*nx;
				}
			}
		drawing_control(FALSE);
		edit_can_proceed(FALSE);
		empty_temp();

		State = Draw;
		}

	/* Set state for Right button "proceed" */
	if (same(mode, "proceed")) State = Edit;

	/* Set state for Right button "preset outline" */
	if (same(mode, "preset outline"))
		{

		/* Check for last stomp outline or named outline */
		if (same(name, FpaEditorLastStompBoundary))
			curve = retrieve_stomp_outline();
		else
			curve = retrieve_named_outline(name);
		if (IsNull(curve))
			{
			if (same(name, FpaEditorLastStompBoundary))
				{
				put_message("edit-no-stomp-outline");
				pr_warning("Editor",
					"[edit_stomp_spline_2D] No stomp outline drawn yet!\n");
				}
			else
				{
				put_message("edit-no-preset-outline", name);
				pr_error("Editor",
					"[edit_stomp_spline_2D] No preset map outline: \"%s\"\n",
					name);
				}
			(void) sleep(1);
			State = Draw;
			}

		/* Display the outline */
		else
			{
			define_lspec(&curve->lspec, 255, 0, NULL, False, 2.0, 0.0,
						 (HILITE) 2);
			add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) curve);
			present_temp(TRUE);
			post_partial(TRUE);
			State = WaitAction;
			}
		}

#	ifdef DEBUG_STOMP
	pr_diag("edit_stomp_spline_2D",
		"  mode: %s  dmag: %.2f  ddir: %.2f  edge: %.2f\n",
		mode, dmag, ddir, edge);
#	endif /* DEBUG_STOMP */

	/* Reset stomp deltas and edge smoothing (if required) */
	if (edge > -10)
		{
		dz    = NINT(dmag*10.0);
		Smag  = 0.1 * (float) dz;
		dz    = NINT(ddir*10.0);
		Sdir  = 0.1 * (float) dz;
		edge  = MIN(edge, 100);
		edge  = MAX(edge, 0);
		Sedge = edge;
		}

	/* Repeat until told to quit */
	while (TRUE)
		{

#		ifdef DEBUG_STOMP
		pr_diag("edit_stomp_spline_2D",
			"  mode: %s  State: %d  Smag: %.2f  Sdir:%.2f  Sedge: %.2f\n",
			mode, State, Smag, Sdir, Sedge);
#		endif /* DEBUG_STOMP */

		switch (State)
			{

			/* Draw boundary of affected area */
			case Draw:

				/* Draw the boundary */
				post_partial(FALSE);
				empty_temp();
				put_message("spline-stomp-draw");
				edit_allow_preset_outline(TRUE);

				if (ready_Xcurve()) butt = LeftButton;
				else if (!ready_Xpoint(DnEdit, NULL, &butt)) return drawn;

				/* Invalid button */
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					ignore_Xpoint();
					continue;
					}
				drawn = FALSE;

				/* Draw the curve */
				drawing_control(TRUE);
				if (!ready_Xcurve())
					{
					if (drawing_Xcurve()) return drawn;
					(void) draw_Xcurve(DnEdit, FilterRes*4, SplineRes*4, TRUE);
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

				/* Extract and display curve */
				curve = create_curve("", "", "");
				add_line_to_curve(curve, lines[0]);
				post_stomp_outline(curve);
				reset_pipe();
				define_lspec(&curve->lspec, 255, 0, NULL, False, 2.0, 0.0,
							 (HILITE) 2);
				add_item_to_metafile(TempMeta, "curve", "c", "", "",
									 (ITEM) curve);
				present_temp(TRUE);
				post_partial(TRUE);

				/* Move on to next stage */
				State = WaitAction;
				continue;

			/* Wait around for a "proceed" */
			case WaitAction:

				/* Check deltas */
				if (Smag == 0.0 && Sdir == 0.0)
					{
					put_message("spline-no-delta");
					circle_echo(FALSE);
					edit_can_proceed(FALSE);
					edit_allow_preset_outline(FALSE);
					return drawn;
					}

				/* Interpret spread factor */
				spread = define_spread();
				circle_echo(TRUE);

				/* Keep returning until we get a "Proceed" */
				edit_can_proceed(TRUE);
				edit_allow_preset_outline(FALSE);
				put_message("spline-stomp-set");
				return FALSE;

			/* Perform the edit */
			case Edit:

				/* Check deltas */
				if (Smag == 0.0 && Sdir == 0.0)
					{
					put_message("spline-no-delta");
					circle_echo(FALSE);
					edit_can_proceed(FALSE);
					State = WaitAction;
					return drawn;
					}

				/* Interpret spread factor */
				spread = define_spread();
				circle_echo(TRUE);

#				ifdef DEBUG
				pr_diag("Editor", "[edit_stomp_spline_2D] Editing\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);

				/* Calculate edge delta and interior spread */
				bset  = find_mf_set(TempMeta, "curve", "c", "", "");
				bound = (CURVE) bset->list[0];
				highlight_set(bset, 0);
				curve_properties(bound, NullChar, NullChar, &a, NullFloat);
				spint = sqrt(a/3.14) / grid;
				erat  = 1.0 - Sedge/100;

				/* Build new grid with adjusted values inside given boundary */
				put_message("spline-adjust");
				for (ix=0; ix<nx; ix++)
					{
					xm    = ix*grid;
					pm[X] = xm;
					for (iy=0; iy<ny; iy++)
						{
						ym     = iy*grid;
						pm[Y]  = ym;
						curve_test_point(bound, pm, &gdist, NULL, NULL,
										 &inside, NULL);
						gdist /= grid;
						valid  = eval_sfc_UV_unmapped(EditSfc, pm,
													  &xval, &yval);
						gxbuf[iy][ix] = (float) xval;
						gybuf[iy][ix] = (float) yval;
						if (inside)
							{
							if (gdist >= spint)
								sf = 1.0;
							else
								{
								a  = (spint-gdist)/spint;
								sf = 1.0 - erat*a*a;
								}
							}
						else if (gdist <= spread)
								sf = ramp(gdist, spread);
						else
								continue;
						calc_delta_2D(EditSfc, pm, sf, Smag, Sdir, &dx, &dy);
						gxbuf[iy][ix] += (float) dx;
						gybuf[iy][ix] += (float) dy;
						}
					}

				/* Fit surface to new grid values */
				grid_surface_2D(EditSfc, grid, nx, ny, gxbuf, gybuf);
				contour_surface(EditSfc);
				if (EditUndoable) post_mod("surface");

				/* Modify labels and highs and lows accordingly */
				if (recompute_surface_labs(EditSfc, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				/* Reset boundary curve and reset displacement fields */
				bset  = NULL;
				bound = NULL;

				/* Re-display the surface */
				put_message("edit-done");
				empty_temp();
				present_all();
				busy_cursor(FALSE);
				edit_can_proceed(FALSE);

				/* Move on to next stage */
				State = Draw;
				continue;
			}
		}
	}

/***********************************************************************
*                                                                      *
*     d r a g _ s p l i n e                                            *
*     d r a g _ s p l i n e _ 2 D                                      *
*                                                                      *
***********************************************************************/

LOGICAL	edit_drag_spline

	(
	STRING	mode
	)

	{
	POINT	p0, p1, pm, pdm;
	MARK	mark0, mark1;
	CURVE	curv0;
	LOGICAL	valid;
	int		ix, iy, ns, ifit;
	float	s, dx, dy, ds, xm, ym, xdm, ydm;
	float	fact, spread;
	double	gval, xval, yval;
	int		butt;
	LOGICAL	drawn = FALSE;

	/* Current state: */
	/* - where should we resume? */
	static	enum	{ Drag, ReFit } State = Drag;

	/* Fit list: */
	/* - list of drags and corresponding refits to be performed */
	static	const int	maxdrag = 100;
	static	int			ndrag   = 0;
	static	int			nfit    = 0;
	static	int			mfit    = 0;
	static	float		*xfit   = NULL;
	static	float		*yfit   = NULL;
	static	POINT		*pfit   = NULL;
	static	double		cval    = 0.0;

	/* Displacement fields: */
	/* - combined result of refits */
	/* - used to re-interpolate original surface */
	static	SURFACE	xdisp = NULL;
	static	SURFACE	ydisp = NULL;

	/* Temporary grid buffer: */
	/* - results of re-interpolation */
	/* - used to refit the new surface */
	static	float  *origin = NullPoint;
	static	float	orient = 0.0;
	static	float	grid   = 0.0;
	static	float	gtol   = 0.0;
	static	int		m      = 0;
	static	int		n      = 0;
	static	int		nx     = 0;
	static	int		ny     = 0;
	static	int		mx     = 0;
	static	int		my     = 0;
	static	float	xlim   = 0.0;
	static	float	ylim   = 0.0;
	static	float	**gbuf = NULL;
	static	float	*gdat  = NULL;
	static	XFORM	xform  = IDENT_XFORM;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_drag_spline] %s  State: %d\n", mode, State);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same(mode, "cancel"))
		{
		circle_echo(FALSE);
		edit_can_proceed(FALSE);
		if (nfit > 0)
			{
			put_message("edit-cancel");
			ignore_partial();

			/* Empty fit list */
			ndrag = 0;
			nfit  = 0;
			define_surface_spline(xdisp, m, n, NullMapProj, origin, orient,
								  grid, NULL, 0);
			define_surface_spline(ydisp, m, n, NullMapProj, origin, orient,
								  grid, NULL, 0);

			/* Empty display structure */
			empty_temp();
			clear_message();
			}
		else
			{
			grid = 0.0;
			}

		State = Drag;
		return FALSE;
		}
	if (same(mode, "cancel all"))
		{
		if (nfit > 0)
			{
			ignore_partial();

			/* Empty fit list */
			ndrag = 0;
			nfit  = 0;
			define_surface_spline(xdisp, m, n, NullMapProj, origin, orient,
								  grid, NULL, 0);
			define_surface_spline(ydisp, m, n, NullMapProj, origin, orient,
								  grid, NULL, 0);

			/* Empty display structure */
			empty_temp();
			clear_message();
			}
		else
			{
			grid = 0.0;
			}
		return FALSE;
		}

	if (same(mode, "clear"))
		{
		return FALSE;
		}

	/* See if surface exists */
	if (!EditSfc) return FALSE;

	/* Interpret spread factor */
	spread = define_spread();

	/* Construct temporary fields and buffers on startup only */
	if (same(mode, "begin") || (grid <= 0.0))
		{
		ignore_partial();
		circle_echo(FALSE);
		edit_can_proceed(FALSE);

		/* Set up fit list */
		ndrag = 0;
		nfit  = 0;
		mfit  = 0;
		FREEMEM(pfit);
		FREEMEM(xfit);
		FREEMEM(yfit);

		/* Set up temporary grid buffer */
		origin = EditSfc->sp.origin;
		orient = EditSfc->sp.orient;
		grid   = EditSfc->sp.gridlen;
		gtol   = grid;
		m      = EditSfc->sp.m;
		n      = EditSfc->sp.n;
		nx     = m - 2;
		ny     = n - 2;
		xlim   = grid*(nx-1);
		ylim   = grid*(ny-1);
		if ((nx != mx) || (ny != my))
			{
			mx   = nx;
			my   = ny;
			gdat = GETMEM(gdat, float, (nx*ny));
			gbuf = GETMEM(gbuf, float *, ny);
			for (iy=0; iy<ny; iy++) gbuf[iy] = gdat + iy*nx;
			}

		/* Set up displacement fields */
		if (!xdisp) xdisp = create_surface();
		if (!ydisp) ydisp = create_surface();
		define_surface_spline(xdisp, m, n, NullMapProj, origin, orient,
							  grid, NULL, 0);
		define_surface_spline(ydisp, m, n, NullMapProj, origin, orient,
							  grid, NULL, 0);

		copy_xform(xform, EditSfc->sp.xform);

		/* Emtpy display structure */
		empty_temp();
		State = Drag;
		}

	/* Set state for Right button "proceed" */
	if (same(mode, "proceed") && nfit <= 0)
		{
		put_message("spline-no-drags");
		(void) sleep(1);
		edit_can_proceed(FALSE);
		State = Drag;
		return FALSE;
		}
	if (same(mode, "proceed")) State = ReFit;

	/* Repeat until told to quit */
	while (TRUE)
		{

		/* Resume at current stage */
		switch (State)
			{

			/* Pick locations to drag */
			case Drag:

				/* Allow specification of multiple drags */
				while (ndrag < maxdrag)
					{

					/* Put up a prompt */
					if (ndrag <= 0) put_message("spline-drag");
					else            put_message("spline-drag2");

					/* Get the start and end point */
					if (!ready_Xpoint(DnEdit, p0, &butt))
						{
						circle_echo(TRUE);
						return drawn;
						}
					if (butt != LeftButton)
						{
						put_message("edit-wrong-button");
						(void) ignore_Xpoint();
						continue;
						}
					if (!inside_dn_window(DnEdit, p0))
						{
						put_message("edit-outside-map");
						(void) ignore_Xpoint();
						continue;
						}
					put_message("spline-drag-end");
					(void) pick_Xpoint(DnEdit, 1, p1, &butt);
					if (!inside_dn_window(DnEdit, p1))
						{
						put_message("spline-tran-out3");
						(void) sleep(1);
						continue;
						}

					/* Evaluate surface at start point and display markers */
					post_partial(TRUE);
					valid = eval_sfc(EditSfc, p0, &cval);
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

					/* Save displacements in "fit" list */
					put_message("spline-drag-proc1");
					ndrag++;
					dx    = p0[X] - p1[X];
					dy    = p0[Y] - p1[Y];
					ds    = hypot(dx, dy);
					ns    = ds/gtol;
					ifit  = nfit;
					nfit += ns+1;
					if (nfit > mfit)
						{
						mfit = nfit;
						pfit = GETMEM(pfit, POINT, mfit);
						xfit = GETMEM(xfit, float, mfit);
						yfit = GETMEM(yfit, float, mfit);
						}
					for ( ; ns>= 0; ns--)
						{
						s    = ns*gtol;
						fact = (ds>0) ? s/ds: 1;
						pfit[ifit][X] = p1[X] + dx*fact;
						pfit[ifit][Y] = p1[Y] + dy*fact;
						fact = 1 - s/(ds+gtol);
						xfit[ifit] = fact*(dx*xform[X][X] + dy*xform[X][Y]);
						yfit[ifit] = fact*(dx*xform[Y][X] + dy*xform[Y][Y]);
						ifit++;
						}

					/* Have at least one drag to fit */
					edit_can_proceed(TRUE);
					}

				pr_warning("Editor",
					"[edit_drag_spline]   End of State = Drag loop!\n");

				/* Have we done any drags? */
				if (nfit > 0) edit_can_proceed(TRUE);
				else          edit_can_proceed(FALSE);

				/* Move on to next stage */
				State = Drag;
				continue;

			/* Perform the refit */
			case ReFit:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_drag_spline] Dragging\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);
				circle_echo(FALSE);

				/* Generate displacement surfaces */
				put_message("spline-drag-proc2");
				if (nfit <= 1)
					{
					edit_surface(xdisp, pfit[0], xfit[0], TRUE, FALSE);
					}
				else if ( !sfit_surface(xdisp, nfit, pfit, xfit,
										spread+1, WgtFactorDrag, TRUE, FALSE) )
					{
					put_message("spline-drag-bad");
					(void) sleep(1);

					/* Empty fit list */
					ndrag = 0;
					nfit  = 0;
					define_surface_spline(xdisp, m, n, NullMapProj, origin,
										  orient, grid, NULL, 0);
					define_surface_spline(ydisp, m, n, NullMapProj, origin,
										  orient, grid, NULL, 0);

					/* Empty display structure */
					empty_temp();

					busy_cursor(FALSE);
					edit_can_proceed(FALSE);
					State = Drag;
					continue;
					}

				if (nfit <= 1)
					{
					edit_surface(ydisp, pfit[0], yfit[0], TRUE, FALSE);
					}
				else if ( !sfit_surface(ydisp, nfit, pfit, yfit,
										spread+1, WgtFactorDrag, TRUE, FALSE) )
					{
					put_message("spline-drag-bad");
					(void) sleep(1);

					/* Empty fit list */
					ndrag = 0;
					nfit  = 0;
					define_surface_spline(xdisp, m, n, NullMapProj, origin,
										  orient, grid, NULL, 0);
					define_surface_spline(ydisp, m, n, NullMapProj, origin,
										  orient, grid, NULL, 0);

					/* Empty display structure */
					empty_temp();

					busy_cursor(FALSE);
					edit_can_proceed(FALSE);
					State = Drag;
					continue;
					}

				/* Interpolate old surface at distorted grid */
				put_message("spline-adjust");
				for (ix=0; ix<nx; ix++)
					{
					xm    = ix*grid;
					pm[X] = xm;
					for (iy=0; iy<ny; iy++)
						{
						ym     = iy*grid;
						pm[Y]  = ym;
						valid  = eval_sfc_unmapped(xdisp, pm, &xval);
						valid  = eval_sfc_unmapped(ydisp, pm, &yval);
						xdm    = xm + (float)xval;
						ydm    = ym + (float)yval;
						xdm    = MAX(xdm, 0.);	xdm = MIN(xdm, xlim);
						ydm    = MAX(ydm, 0.);	ydm = MIN(ydm, ylim);
						pdm[X] = xdm;
						pdm[Y] = ydm;
						valid  = eval_sfc_unmapped(EditSfc, pdm, &gval);
						gbuf[iy][ix] = (float)gval;
						}
					}

				/* Fit surface to new grid values and force central value */
				/* to remain the same */
				grid_surface(EditSfc, grid, nx, ny, gbuf);
				if (ndrag == 1)
					{
					edit_surface(EditSfc, p1, (float)cval, TRUE, FALSE);
					}
				contour_surface(EditSfc);

				if (EditUndoable) post_mod("surface");

				/* Modify labels and highs and lows accordingly */
				if (recompute_surface_labs(EditSfc, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}
				put_message("edit-done");
				busy_cursor(FALSE);

				/* Reset fit list to zero and reset displacement fields */
				ndrag = 0;
				nfit  = 0;
				define_surface_spline(xdisp, m, n, NullMapProj, origin, orient,
									  grid, NULL, 0);
				define_surface_spline(ydisp, m, n, NullMapProj, origin, orient,
									  grid, NULL, 0);

				/* Re-display the surface */
				empty_temp();
				present_all();

				/* Move on to next stage */
				edit_can_proceed(FALSE);
				State = Drag;
				continue;
			}
		}
	}

/**********************************************************************/

LOGICAL	edit_drag_spline_2D

	(
	STRING	mode
	)

	{
	POINT	p0, p1, pm, pdm;
	MARK	mark0, mark1;
	CURVE	curv0;
	LOGICAL	valid;
	int		ix, iy, ns, ifit;
	float	s, dx, dy, ds, xm, ym, xdm, ydm;
	float	fact, spread;
	double	gxval, gyval, xval, yval;
	int		butt;
	LOGICAL	drawn = FALSE;

	/* Current state: */
	/* - where should we resume? */
	static	enum	{ Drag, ReFit } State = Drag;

	/* Fit list: */
	/* - list of drags and corresponding refits to be performed */
	static	const int	maxdrag = 100;
	static	int			ndrag   = 0;
	static	int			nfit    = 0;
	static	int			mfit    = 0;
	static	float		*xfit   = NULL;
	static	float		*yfit   = NULL;
	static	POINT		*pfit   = NULL;
	static	double		cxval   = 0.0;
	static	double		cyval   = 0.0;

	/* Displacement fields: */
	/* - combined result of refits */
	/* - used to re-interpolate original surface */
	static	SURFACE	xdisp = NULL;
	static	SURFACE	ydisp = NULL;

	/* Temporary grid buffer: */
	/* - results of re-interpolation */
	/* - used to refit the new surface */
	static	float  *origin = NullPoint;
	static	float	orient = 0.0;
	static	float	grid   = 0.0;
	static	float	gtol   = 0.0;
	static	int		m      = 0;
	static	int		n      = 0;
	static	int		nx     = 0;
	static	int		ny     = 0;
	static	int		mx     = 0;
	static	int		my     = 0;
	static	float	xlim   = 0.0;
	static	float	ylim   = 0.0;
	static	float	**gxbuf = NULL;
	static	float	**gybuf = NULL;
	static	float	*gxdat  = NULL;
	static	float	*gydat  = NULL;
	static	XFORM	xform  = IDENT_XFORM;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_drag_spline_2D] %s  State: %d\n", mode, State);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same(mode, "cancel"))
		{
		circle_echo(FALSE);
		edit_can_proceed(FALSE);
		if (nfit > 0)
			{
			put_message("edit-cancel");
			ignore_partial();

			/* Empty fit list */
			ndrag = 0;
			nfit  = 0;
			define_surface_spline(xdisp, m, n, NullMapProj, origin, orient,
								  grid, NULL, 0);
			define_surface_spline(ydisp, m, n, NullMapProj, origin, orient,
								  grid, NULL, 0);

			/* Empty display structure */
			empty_temp();
			clear_message();
			}
		else
			{
			grid = 0.0;
			}

		State = Drag;
		return FALSE;
		}
	if (same(mode, "cancel all"))
		{
		if (nfit > 0)
			{
			ignore_partial();

			/* Empty fit list */
			ndrag = 0;
			nfit  = 0;
			define_surface_spline(xdisp, m, n, NullMapProj, origin, orient,
								  grid, NULL, 0);
			define_surface_spline(ydisp, m, n, NullMapProj, origin, orient,
								  grid, NULL, 0);

			/* Empty display structure */
			empty_temp();
			clear_message();
			}
		else
			{
			grid = 0.0;
			}
		return FALSE;
		}

	if (same(mode, "clear"))
		{
		return FALSE;
		}

	/* See if surface exists */
	if (!EditSfc) return FALSE;

	/* Interpret spread factor */
	spread = define_spread();

	/* Construct temporary fields and buffers on startup only */
	if (same(mode, "begin") || (grid <= 0.0))
		{
		ignore_partial();
		circle_echo(FALSE);
		edit_can_proceed(FALSE);

		/* Set up fit list */
		ndrag = 0;
		nfit  = 0;
		mfit  = 0;
		FREEMEM(pfit);
		FREEMEM(xfit);
		FREEMEM(yfit);

		/* Set up temporary grid buffer */
		origin = EditSfc->sp.origin;
		orient = EditSfc->sp.orient;
		grid   = EditSfc->sp.gridlen;
		gtol   = grid;
		m      = EditSfc->sp.m;
		n      = EditSfc->sp.n;
		nx     = m - 2;
		ny     = n - 2;
		xlim   = grid*(nx-1);
		ylim   = grid*(ny-1);
		if ((nx != mx) || (ny != my))
			{
			mx   = nx;
			my   = ny;
			gxdat = GETMEM(gxdat, float, (nx*ny));
			gydat = GETMEM(gydat, float, (nx*ny));
			gxbuf = GETMEM(gxbuf, float *, ny);
			gybuf = GETMEM(gybuf, float *, ny);
			for (iy=0; iy<ny; iy++)
				{
				gxbuf[iy] = gxdat + iy*nx;
				gybuf[iy] = gydat + iy*nx;
				}
			}

		/* Set up displacement fields */
		if (!xdisp) xdisp = create_surface();
		if (!ydisp) ydisp = create_surface();
		define_surface_spline(xdisp, m, n, NullMapProj, origin, orient,
							  grid, NULL, 0);
		define_surface_spline(ydisp, m, n, NullMapProj, origin, orient,
							  grid, NULL, 0);

		copy_xform(xform, EditSfc->sp.xform);

		/* Emtpy display structure */
		empty_temp();
		State = Drag;
		}

	/* Set state for Right button "proceed" */
	if (same(mode, "proceed") && nfit <= 0)
		{
		put_message("spline-no-drags");
		(void) sleep(1);
		edit_can_proceed(FALSE);
		State = Drag;
		return FALSE;
		}
	if (same(mode, "proceed")) State = ReFit;

	/* Repeat until told to quit */
	while (TRUE)
		{

		/* Resume at current stage */
		switch (State)
			{

			/* Pick locations to drag */
			case Drag:

				/* Allow specification of multiple drags */
				while (ndrag < maxdrag)
					{

					/* Put up a prompt */
					if (ndrag <= 0) put_message("spline-drag");
					else            put_message("spline-drag2");

					/* Get the start and end point */
					if (!ready_Xpoint(DnEdit, p0, &butt))
						{
						circle_echo(TRUE);
						return drawn;
						}
					if (butt != LeftButton)
						{
						put_message("edit-wrong-button");
						(void) ignore_Xpoint();
						continue;
						}
					if (!inside_dn_window(DnEdit, p0))
						{
						put_message("edit-outside-map");
						(void) ignore_Xpoint();
						continue;
						}
					put_message("spline-drag-end");
					(void) pick_Xpoint(DnEdit, 1, p1, &butt);
					if (!inside_dn_window(DnEdit, p1))
						{
						put_message("spline-tran-out3");
						(void) sleep(1);
						continue;
						}

					/* Evaluate surface at start point and display markers */
					post_partial(TRUE);
					valid = eval_sfc_UV(EditSfc, p0, &cxval, &cyval);
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

					/* Save displacements in "fit" list */
					put_message("spline-drag-proc1");
					ndrag++;
					dx    = p0[X] - p1[X];
					dy    = p0[Y] - p1[Y];
					ds    = hypot(dx, dy);
					ns    = ds/gtol;
					ifit  = nfit;
					nfit += ns+1;
					if (nfit > mfit)
						{
						mfit = nfit;
						pfit = GETMEM(pfit, POINT, mfit);
						xfit = GETMEM(xfit, float, mfit);
						yfit = GETMEM(yfit, float, mfit);
						}
					for ( ; ns>= 0; ns--)
						{
						s    = ns*gtol;
						fact = (ds>0) ? s/ds: 1;
						pfit[ifit][X] = p1[X] + dx*fact;
						pfit[ifit][Y] = p1[Y] + dy*fact;
						fact = 1 - s/(ds+gtol);
						xfit[ifit] = fact*(dx*xform[X][X] + dy*xform[X][Y]);
						yfit[ifit] = fact*(dx*xform[Y][X] + dy*xform[Y][Y]);
						ifit++;
						}

					/* Have at least one drag to fit */
					edit_can_proceed(TRUE);
					}

				pr_warning("Editor",
					"[edit_drag_spline_2D]   End of State = Drag loop!\n");

				/* Have we done any drags? */
				if (nfit > 0) edit_can_proceed(TRUE);
				else          edit_can_proceed(FALSE);

				/* Move on to next stage */
				State = Drag;
				continue;

			/* Perform the refit */
			case ReFit:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_drag_spline_2D] Dragging\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);
				circle_echo(FALSE);

				/* Generate displacement surfaces */
				put_message("spline-drag-proc2");
				if (nfit <= 1)
					{
					edit_surface(xdisp, pfit[0], xfit[0], TRUE, FALSE);
					}
				else if ( !sfit_surface(xdisp, nfit, pfit, xfit,
										spread+1, WgtFactorDrag, TRUE, FALSE) )
					{
					put_message("spline-drag-bad");
					(void) sleep(1);

					/* Empty fit list */
					ndrag = 0;
					nfit  = 0;
					define_surface_spline(xdisp, m, n, NullMapProj, origin,
										  orient, grid, NULL, 0);
					define_surface_spline(ydisp, m, n, NullMapProj, origin,
										  orient, grid, NULL, 0);

					/* Empty display structure */
					empty_temp();

					busy_cursor(FALSE);
					edit_can_proceed(FALSE);
					State = Drag;
					continue;
					}

				if (nfit <= 1)
					{
					edit_surface(ydisp, pfit[0], yfit[0], TRUE, FALSE);
					}
				else if ( !sfit_surface(ydisp, nfit, pfit, yfit,
										spread+1, WgtFactorDrag, TRUE, FALSE) )
					{
					put_message("spline-drag-bad");
					(void) sleep(1);

					/* Empty fit list */
					ndrag = 0;
					nfit  = 0;
					define_surface_spline(xdisp, m, n, NullMapProj, origin,
										  orient, grid, NULL, 0);
					define_surface_spline(ydisp, m, n, NullMapProj, origin,
										  orient, grid, NULL, 0);

					/* Empty display structure */
					empty_temp();

					busy_cursor(FALSE);
					edit_can_proceed(FALSE);
					State = Drag;
					continue;
					}

				/* Interpolate old surface at distorted grid */
				put_message("spline-adjust");
				for (ix=0; ix<nx; ix++)
					{
					xm    = ix*grid;
					pm[X] = xm;
					for (iy=0; iy<ny; iy++)
						{
						ym     = iy*grid;
						pm[Y]  = ym;
						valid  = eval_sfc_unmapped(xdisp, pm, &xval);
						valid  = eval_sfc_unmapped(ydisp, pm, &yval);
						xdm    = xm + (float)xval;
						ydm    = ym + (float)yval;
						xdm    = MAX(xdm, 0.);	xdm = MIN(xdm, xlim);
						ydm    = MAX(ydm, 0.);	ydm = MIN(ydm, ylim);
						pdm[X] = xdm;
						pdm[Y] = ydm;
						valid  = eval_sfc_UV_unmapped(EditSfc, pdm,
													  &gxval, &gyval);
						gxbuf[iy][ix] = (float)gxval;
						gybuf[iy][ix] = (float)gyval;
						}
					}

				/* Fit surface to new grid values and force central value */
				/* to remain the same */
				grid_surface_2D(EditSfc, grid, nx, ny, gxbuf, gybuf);
				if (ndrag == 1)
					{
					edit_surface_2D(EditSfc, p1, (float)cxval, (float)cyval,
									TRUE, FALSE);
					}
				contour_surface(EditSfc);

				if (EditUndoable) post_mod("surface");

				/* Modify labels and highs and lows accordingly */
				if (recompute_surface_labs(EditSfc, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}
				put_message("edit-done");
				busy_cursor(FALSE);

				/* Reset fit list to zero and reset displacement fields */
				ndrag = 0;
				nfit  = 0;
				define_surface_spline(xdisp, m, n, NullMapProj, origin, orient,
									  grid, NULL, 0);
				define_surface_spline(ydisp, m, n, NullMapProj, origin, orient,
									  grid, NULL, 0);

				/* Re-display the surface */
				empty_temp();
				present_all();

				/* Move on to next stage */
				edit_can_proceed(FALSE);
				State = Drag;
				continue;
			}
		}
	}

/***********************************************************************
*                                                                      *
*     b l o c k _ s p l i n e                                          *
*     b l o c k _ s p l i n e _ 2 D                                    *
*                                                                      *
***********************************************************************/

LOGICAL	edit_block_spline

	(
	STRING	mode,
	STRING	name
	)

	{
	CURVE	curve, xbound;
	SPOT	spot, tspot;
	SET		set, tset;
	POINT	p, q, p0, p1, pz, pm, pdm;
	MARK	mark0, mark1;
	CURVE	curv0, curv1;
	int		nlines;
	LINE	*lines;
	LOGICAL	valid, inside;
	int		ix, iy, il, ilx;
	float	dx, dy, dx0, dy0, angle, cang, sang;
	float	xm, ym, xdm, ydm, dxm, dym, cx, cy, px, py;
	float	dtol, fact, dist, sight, approach;
	float	spread;
	double	gval, xval, yval, oval;
	int		butt;
	LOGICAL	drawn = FALSE;

	/* Current state: */
	/* - where should we resume? */
	static	enum	{ Draw, Copy, Paste,
						WaitAction, Translate, TranslateDone,
						Centre, Rotate, RotateDone, ReDisplay } State = Draw;

	/* Was a paste just done? */
	static	LOGICAL		pasting = FALSE;
	static	SURFACE		psfc    = NullSfc;

	/* Has a translate or rotate been started but not completed? */
	static	LOGICAL		mstart  = FALSE;

	/* Should labels be moved too? */
	static	LOGICAL		mlabels = TRUE;
	static	LOGICAL		mlfirst = TRUE;

	/* Boundary curve set: */
	/* - defines undistorted region */
	/* - used in cursor tracking */
	static	SET			bset  = NULL;
	static	CURVE		bound = NULL;

	/* Move list: */
	/* - list of field labels to be moved */
	/* - smove are pointers into EditLabs */
	/* - scopy are copies to be displayed in TempMeta */
	static	int			nlabs   = 0;
	static	int			alabs   = 0;
	static	SPOT		*smove  = NullSpotList;
	static	SPOT		*scopy  = NullSpotList;

	/* Centre of rotation */
	static	POINT		Cpos    = ZERO_POINT;
	static	float		Ang     = 0.0;

	/* Displacement fields: */
	/* - combined result of refits */
	/* - used to re-interpolate original surface */
	static	SURFACE	xdisp = NULL;
	static	SURFACE	ydisp = NULL;
	static	XFORM	xform = IDENT_XFORM;

	/* Temporary grid buffer: */
	/* - results of re-interpolation */
	/* - used to refit the new surface */
	static	float	*origin = NullPoint;
	static	float	orient  = 0.0;
	static	float	grid    = 0.0;
	static	float	gtol    = 0.0;
	static	int		m       = 0;
	static	int		n       = 0;
	static	int		nx      = 0;
	static	int		ny      = 0;
	static	int		mx      = 0;
	static	int		my      = 0;
	static	float	xlim    = 0.0;
	static	float	ylim    = 0.0;
	static	float	**sfbuf = NULL;
	static	float	**dxbuf = NULL;
	static	float	**dybuf = NULL;
	static	float	*sfdat  = NULL;
	static	float	*dxdat  = NULL;
	static	float	*dydat  = NULL;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_block_spline] %s  State: %d\n", mode, State);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same(mode, "cancel"))
		{
		put_message("edit-cancel");
		ignore_partial();
		circle_echo(FALSE);
		drawing_control(FALSE);
		edit_allow_preset_outline(FALSE);

		if (bset)
			{
			bset  = NULL;
			bound = NULL;
			define_surface_spline(xdisp, m, n, NullMapProj, origin, orient,
								  grid, NULL, 0);
			define_surface_spline(ydisp, m, n, NullMapProj, origin, orient,
								  grid, NULL, 0);
			}
		else
			{
			grid = 0.0;
			}

		/* Empty field label list */
		if (nlabs > 0)
			{
			nlabs = 0;
			alabs = 0;
			FREEMEM(smove);
			FREEMEM(scopy);
			}

		/* Empty display structure */
		empty_temp();
		present_all();
		pasting = FALSE;
		mstart  = FALSE;
		clear_message();

		edit_select(NullCal, FALSE);
		edit_can_copy(FALSE);
		if (copy_posted("surface", CurrElement, CurrLevel))
				edit_can_paste(TRUE);
		State = Draw;
		return FALSE;
		}
	if (same(mode, "cancel all"))
		{
		ignore_partial();
		drawing_control(FALSE);
		edit_allow_preset_outline(FALSE);

		if (bset)
			{
			bset  = NULL;
			bound = NULL;
			define_surface_spline(xdisp, m, n, NullMapProj, origin, orient,
								  grid, NULL, 0);
			define_surface_spline(ydisp, m, n, NullMapProj, origin, orient,
								  grid, NULL, 0);
			}
		else
			{
			grid = 0.0;
			}

		/* Empty field label list */
		if (nlabs > 0)
			{
			nlabs = 0;
			alabs = 0;
			FREEMEM(smove);
			FREEMEM(scopy);
			}

		/* Empty display structure */
		empty_temp();
		present_all();
		pasting = FALSE;
		mstart  = FALSE;
		clear_message();

		edit_select(NullCal, FALSE);
		edit_can_copy(FALSE);
		if (copy_posted("surface", CurrElement, CurrLevel))
				edit_can_paste(TRUE);
		State = Draw;
		return FALSE;
		}

	if (same(mode, "clear"))
		{
		return FALSE;
		}

	/* See if surface exists */
	if (!EditSfc) return FALSE;

	/* Construct temporary fields on startup */
	if (same(mode, "begin") || grid <= 0.0)
		{
		ignore_partial();
		circle_echo(FALSE);
		drawing_control(FALSE);

		/* Set up temporary grid buffers */
		origin = EditSfc->sp.origin;
		orient = EditSfc->sp.orient;
		m      = EditSfc->sp.m;
		n      = EditSfc->sp.n;
		grid   = EditSfc->sp.gridlen;
		gtol   = grid*1.5;
		nx     = m - 2;
		ny     = n - 2;
		xlim   = grid*(nx-1);
		ylim   = grid*(ny-1);
		if ((nx != mx) || (ny != my))
			{
			mx    = nx;
			my    = ny;
			sfdat = GETMEM(sfdat, float, (nx*ny));
			dxdat = GETMEM(dxdat, float, (nx*ny));
			dydat = GETMEM(dydat, float, (nx*ny));
			sfbuf = GETMEM(sfbuf, float *, ny);
			dxbuf = GETMEM(dxbuf, float *, ny);
			dybuf = GETMEM(dybuf, float *, ny);
			for (iy=0; iy<ny; iy++)
				{
				sfbuf[iy] = sfdat + iy*nx;
				dxbuf[iy] = dxdat + iy*nx;
				dybuf[iy] = dydat + iy*nx;
				}
			}

		/* Set up displacement fields */
		if (!xdisp) xdisp = create_surface();
		if (!ydisp) ydisp = create_surface();
		define_surface_spline(xdisp, m, n, NullMapProj, origin, orient,
							  grid, NULL, 0);
		define_surface_spline(ydisp, m, n, NullMapProj, origin, orient,
							  grid, NULL, 0);

		copy_xform(xform, EditSfc->sp.xform);

		/* Set up move list */
		nlabs = 0;
		alabs = 0;
		FREEMEM(smove);
		FREEMEM(scopy);

		/* Empty display structure */
		empty_temp();
		pasting = FALSE;
		mstart  = FALSE;
		mlfirst = TRUE;

		edit_select(NullCal, FALSE);
		edit_can_copy(FALSE);
		if (copy_posted("surface", CurrElement, CurrLevel))
				edit_can_paste(TRUE);
		State = Draw;
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
				put_message("spline-move-nomode");
				(void) sleep(1);
				}
			mlfirst = FALSE;
			}

		/* Change move mode and remove labels */
		else if (mlabels)
			{

			/* Remove each field label from the list */
			tset = find_mf_set(TempMeta, "spot",  "d", "smv", "smv");
			for (il=0; il<nlabs; il++)
				{
				(void) remove_item_from_set(tset, (ITEM) scopy[il]);
				}
			nlabs = 0;
			alabs = 0;
			FREEMEM(smove);
			FREEMEM(scopy);

			/* Re-display the boundary */
			present_all();

			/* Reset check for move mode */
			mlabels = FALSE;
			mlfirst = TRUE;
			}

		/* Change move mode and add labels if pasting */
		else if (pasting)
			{

			/* Re-retrieve paste buffer labels */
			paste_spline_copy(CurrElement, CurrLevel, NullSfcPtr, NullCurvePtr,
							  &nlabs, &smove);

			/* Add the pasted labels to the display */
			if (nlabs > alabs)
				{
				alabs = nlabs;
				scopy = GETMEM(scopy, SPOT, alabs);
				}
			for (il=0; il<nlabs; il++)
				{
				spot = smove[il];

				/* Highlight the pasted labels */
				scopy[il] = copy_spot(spot);
				tspot     = scopy[il];
				highlight_item("spot", (ITEM) tspot, 3);
				add_item_to_metafile(TempMeta, "spot", "d",
									 "smv", "smv", (ITEM) tspot);
				 }

			/* Re-display the boundary and field labels */
			present_temp(TRUE);

			/* Reset check for move mode */
			mlabels = TRUE;
			mlfirst = TRUE;
			}

		/* Change move mode and add labels */
		else
			{

			/* Match current list of field labels to    */
			/*  drawn boundary and add them to the list */
			bset = find_mf_set(TempMeta, "curve", "c", "", "");
			if (NotNull(bset) && NotNull(EditLabs))
				{
				bound = (CURVE) bset->list[0];

				/* Loop through all labels in set */
				nlabs = 0;
				for (ilx=0; ilx<EditLabs->num; ilx++)
					{
					spot = (SPOT) EditLabs->list[ilx];
					if (!spot) continue;
					if (same(spot->mclass, "legend")) continue;

					/* Check if label is within drawn boundary */
					(void) curve_test_point(bound, spot->anchor, NULL,
											NULL, NULL, &inside, NULL);
					if (inside)
						{

						/* Highlight each field label */
						/*  and add it to the list    */
						nlabs++;
						if (nlabs > alabs)
							{
							alabs = nlabs;
							smove = GETMEM(smove, SPOT, alabs);
							scopy = GETMEM(scopy, SPOT, alabs);
							}
						smove[nlabs-1] = spot;
						scopy[nlabs-1] = copy_spot(spot);
						tspot          = scopy[nlabs-1];
						highlight_item("spot", (ITEM) tspot, 2);

						add_item_to_metafile(TempMeta, "spot", "d",
											 "smv", "smv", (ITEM) tspot);
						}
					}
				}

			/* Re-display the boundary and field labels */
			present_temp(TRUE);

			/* Reset check for move mode */
			mlabels = TRUE;
			mlfirst = TRUE;
			}
		}

	/* Set state for "copy" or "paste" */
	if (same(mode, "copy"))  State = Copy;
	if (same(mode, "paste")) State = Paste;

	/* Set state for Right button "translate" or "rotate" */
	if (same(mode, "translate")) State = Translate;
	if (same(mode, "rotate"))    State = Centre;

	/* Set state for Right button "preset outline" */
	if (same(mode, "preset outline"))
		{

		/* Check for last drawn outline, last moved outline, or named outline */
		if (same(name, FpaEditorLastDrawnBoundary))
			curve = retrieve_drawn_outline();
		else if (same(name, FpaEditorLastMovedBoundary))
			curve = retrieve_moved_outline();
		else
			curve = retrieve_named_outline(name);
		if (IsNull(curve))
			{
			if (same(name, FpaEditorLastDrawnBoundary))
				{
				put_message("edit-no-drawn-outline");
				pr_warning("Editor",
					"[edit_block_spline] No outline drawn yet!\n");
				}
			else if (same(name, FpaEditorLastMovedBoundary))
				{
				put_message("edit-no-moved-outline");
				pr_warning("Editor",
					"[edit_block_spline] No outline moved yet!\n");
				}
			else
				{
				put_message("edit-no-preset-outline", name);
				pr_error("Editor",
					"[edit_block_spline] No preset map outline: \"%s\"\n",
					name);
				}
			(void) sleep(1);
			State = Draw;
			}

		/* Display the outline */
		else
			{
			define_lspec(&curve->lspec, 255, 0, NULL, False, 2.0, 0.0,
						 (HILITE) 2);
			add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) curve);

			/* Save labels within drawn boundary (if requested) */
			if (mlabels && NotNull(EditLabs))
				{

				/* Loop through all labels in set */
				nlabs = 0;
				for (ilx=0; ilx<EditLabs->num; ilx++)
					{
					spot = (SPOT) EditLabs->list[ilx];
					if (!spot) continue;
					if (same(spot->mclass, "legend")) continue;

					/* Check if label is within drawn boundary */
					(void) curve_test_point(curve, spot->anchor, NULL,
											NULL, NULL, &inside, NULL);
					if (inside)
						{

						/* Highlight each field label */
						/*  and add it to the list    */
						nlabs++;
						if (nlabs > alabs)
							{
							alabs = nlabs;
							smove = GETMEM(smove, SPOT, alabs);
							scopy = GETMEM(scopy, SPOT, alabs);
							}
						smove[nlabs-1] = spot;
						scopy[nlabs-1] = copy_spot(spot);
						tspot          = scopy[nlabs-1];
						highlight_item("spot", (ITEM) tspot, 2);

						add_item_to_metafile(TempMeta, "spot", "d",
											 "smv", "smv", (ITEM) tspot);
						}
					}
				}

			/* Display the boundary and field labels */
			present_temp(TRUE);
			post_partial(TRUE);
			edit_select(NullCal, TRUE);
			edit_can_copy(TRUE);
			State = WaitAction;
			}
		}

	/* Repeat until told to quit */
	while (TRUE)
		{
		switch (State)
			{

			/* Draw boundary of un-distorted area */
			case Draw:

				/* Draw the boundary */
				post_partial(FALSE);
				empty_temp();
				put_message("spline-move-draw");
				edit_select(NullCal, FALSE);
				edit_can_copy(FALSE);
				if (copy_posted("surface", CurrElement, CurrLevel))
						edit_can_paste(TRUE);
				edit_allow_preset_outline(TRUE);

				if (ready_Xcurve()) butt = LeftButton;
				else if (!ready_Xpoint(DnEdit, NULL, &butt)) return drawn;

				/* Invalid button */
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					ignore_Xpoint();
					continue;
					}

				/* Draw the curve */
				edit_can_paste(FALSE);
				drawing_control(TRUE);
				if (!ready_Xcurve())
					{
					if (drawing_Xcurve()) return drawn;
					(void) draw_Xcurve(DnEdit, FilterRes*4, SplineRes*4, TRUE);
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

				/* Extract and display curve */
				curve = create_curve("", "", "");
				add_line_to_curve(curve, lines[0]);
				post_drawn_outline(curve);
				reset_pipe();
				define_lspec(&curve->lspec, 255, 0, NULL, False, 2.0, 0.0,
							 (HILITE) 2);
				add_item_to_metafile(TempMeta, "curve", "c", "", "",
									 (ITEM) curve);

				/* Save labels within drawn boundary (if requested) */
				if (mlabels && NotNull(EditLabs))
					{

					/* Loop through all labels in set */
					nlabs = 0;
					for (ilx=0; ilx<EditLabs->num; ilx++)
						{
						spot = (SPOT) EditLabs->list[ilx];
						if (!spot) continue;
						if (same(spot->mclass, "legend")) continue;

						/* Check if label is within drawn boundary */
						(void) curve_test_point(curve, spot->anchor, NULL,
												NULL, NULL, &inside, NULL);
						if (inside)
							{

							/* Highlight each field label */
							/*  and add it to the list    */
							nlabs++;
							if (nlabs > alabs)
								{
								alabs = nlabs;
								smove = GETMEM(smove, SPOT, alabs);
								scopy = GETMEM(scopy, SPOT, alabs);
								}
							smove[nlabs-1] = spot;
							scopy[nlabs-1] = copy_spot(spot);
							tspot          = scopy[nlabs-1];
							highlight_item("spot", (ITEM) tspot, 2);

							add_item_to_metafile(TempMeta, "spot", "d",
												 "smv", "smv", (ITEM) tspot);
							}
						}
					}

				/* Display the boundary and field labels */
				present_temp(TRUE);
				post_partial(TRUE);
				edit_select(NullCal, TRUE);

				/* Move on to next stage */
				edit_can_copy(TRUE);
				State = WaitAction;
				continue;

			/* Copy surface */
			case Copy:

				/* Check for the boundary */
				bset  = find_mf_set(TempMeta, "curve", "c", "", "");
				if (IsNull(bset))
					{
					bound = NullCurve;
					State = Draw;
					continue;
					}
				bound = (CURVE) bset->list[0];

				/* Save the surface and boundary in the copy buffer */
				put_message("edit-copy");
				post_spline_copy(CurrElement, CurrLevel, EditSfc, bound,
								 nlabs, smove);
				clear_message();
				pasting = FALSE;

				/* Move on to next stage */
				edit_can_copy(TRUE);
				State = WaitAction;
				continue;

			/* Paste surface */
			case Paste:

				put_message("edit-paste");

				/* Empty move list */
				bset  = find_mf_set(TempMeta, "curve", "c", "", "");
				if (NotNull(bset))
					{
					ignore_partial();
					edit_can_copy(FALSE);
					bset  = NULL;
					bound = NULL;
					define_surface_spline(xdisp, m, n, NullMapProj, origin,
										  orient, grid, NULL, 0);
					define_surface_spline(ydisp, m, n, NullMapProj, origin,
										  orient, grid, NULL, 0);

					/* Empty display structure */
					empty_temp();
					}

				/* Retrieve paste buffer */
				paste_spline_copy(CurrElement, CurrLevel, &psfc, &bound,
								  &nlabs, &smove);
				if (IsNull(psfc) || IsNull(bound))
					{
					edit_can_paste(FALSE);
					put_message("edit-no-paste");
					(void) sleep(1);
					edit_select(NullCal, FALSE);
					State = Draw;
					return FALSE;
					}

				/* Highlight the paste surface */
				highlight_surface(psfc, 3);
				add_sfc_to_metafile(TempMeta, "a", "", "", psfc);
				add_item_to_metafile(TempMeta, "curve", "c", "", "",
									 (ITEM) bound);

				/* Then add the pasted labels to the display (if requested) */
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

						/* Highlight the pasted labels */
						scopy[il] = copy_spot(spot);
						tspot     = scopy[il];
						highlight_item("spot", (ITEM) tspot, 3);
						add_item_to_metafile(TempMeta, "spot", "d",
											 "smv", "smv", (ITEM) tspot);
						}
					}

				/* Re-display the paste surface */
				present_temp(TRUE);
				post_partial(TRUE);
				edit_select(NullCal, TRUE);
				pasting = TRUE;
				clear_message();

				/* Move on to next stage */
				State = WaitAction;
				continue;

			/* Wait around for a "translate" or "rotate" */
			case WaitAction:

				/* Interpret spread factor */
				spread = define_spread();
				circle_echo(TRUE);

				/* Keep returning until we get a "translate" or "rotate" */
				put_message("spline-move-action");
				edit_allow_preset_outline(FALSE);
				edit_can_copy(TRUE);
				return FALSE;

			/* Pick and translate a reference point */
			case Translate:

				/* Remove any marks from uncompleted actions */
				set = take_mf_set(TempMeta, "mark", "d", "", "");
				if (NotNull(set)) set = destroy_set(set);

				/* Re-display the outline */
				present_all();
				post_partial(TRUE);
				edit_can_copy(TRUE);

				/* Check for the boundary */
				bset   = find_mf_set(TempMeta, "curve", "c", "", "");
				if (IsNull(bset))
					{
					bound = NullCurve;
					State = Draw;
					continue;
					}
				bound  = (CURVE) bset->list[0];
				mstart = TRUE;

				/* Interpret spread factor */
				spread = define_spread();

				/* Pick a reference point */
				if (pasting) put_message("spline-tran-copy");
				else         put_message("spline-tran");
				if (!ready_Xpoint(DnEdit, p0, &butt))
					{
					circle_echo(TRUE);
					return drawn;
					}
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!inside_dn_window(DnEdit, p0))
					{
					put_message("spline-tran-out1");
					(void) ignore_Xpoint();
					continue;
					}

				/* Try with any point ... skip check for inside boundary!
				curve_test_point(bound, p0, NULL, NULL, NULL, &inside, NULL);
				if (!inside)
					{
					put_message("spline-tran-out2");
					(void) ignore_Xpoint();
					continue;
					}
				*/

				/* Translate the reference point */
				put_message("spline-tran-rel");
				(void) utrack_Xpoint(DnEdit, bset, p0, p1, &butt);
				if (!inside_dn_window(DnEdit, p1))
					{
					put_message("spline-tran-out3");
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

			/* Translate the surface */
			case TranslateDone:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_block_spline] Translating\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);
				circle_echo(FALSE);

				/* Translate the surface */
				put_message("spline-drag-proc2");
				dx    = p1[X] - p0[X];
				dy    = p1[Y] - p0[Y];
				gtol  = grid*(1+spread);
				dtol  = hypot(dx, dy) + gtol;
				dxm   = dx*xform[X][X] + dy*xform[X][Y];
				dym   = dx*xform[Y][X] + dy*xform[Y][Y];

				/* Recalculate grid with pasted section */
				if (pasting)
					{
					double	vi, vo, wi, wo;

					put_message("spline-adjust");
					for (ix=0; ix<nx; ix++)
						{
						xm   = ix*grid;
						for (iy=0; iy<ny; iy++)
							{
							ym   = iy*grid;
							p[X] = xm*xform[X][X] + ym*xform[Y][X]
														+ xform[H][X] - dx;
							p[Y] = xm*xform[X][Y] + ym*xform[Y][Y]
														+ xform[H][Y] - dy;
							curve_test_point(bound, p, &dist, NULL, NULL,
											 &inside, NULL);

							/* Determine proximity to boundary */
							if (inside)
								{
								wo = 0.0;
								wi = 1.0;
								}
							else if (dist<gtol)
								{
								wo = dist/gtol;
								wi = 1 - wo;
								}
							else
								{
								wo = 1.0;
								wi = 0.0;
								}

							/* Evaluate grid at translated location */
							vi = vo = 0.0;
							if (wi > 0)
								{
								xdm = xm - dxm;
								ydm = ym - dym;
								xdm    = MAX(xdm, 0.);	xdm = MIN(xdm, xlim);
								ydm    = MAX(ydm, 0.);	ydm = MIN(ydm, ylim);
								pdm[X] = xdm;
								pdm[Y] = ydm;
								valid  = eval_sfc_unmapped(psfc, pdm, &vi);
								}
							if (wo > 0)
								{
								pdm[X] = xm;
								pdm[Y] = ym;
								valid  = eval_sfc_unmapped(EditSfc, pdm, &vo);
								}

							sfbuf[iy][ix] = (float) (vi*wi + vo*wo);
							}
						}
					}

				/* Recalculate grid with translated section */
				else
					{
					/* Set up points to define displacement fields */
					for (ix=0; ix<nx; ix++)
						{
						xm   = ix*grid;
						for (iy=0; iy<ny; iy++)
							{
							ym   = iy*grid;
							q[X] = xm*xform[X][X] + ym*xform[Y][X]
														+ xform[H][X];
							q[Y] = xm*xform[X][Y] + ym*xform[Y][Y]
														+ xform[H][Y];
							p[X] = q[X] - dx;
							p[Y] = q[Y] - dy;
							curve_sight(bound, p, q, TRUE, &sight, &approach,
										NULL, NULL, NULL);
							curve_test_point(bound, p, &dist, NULL, NULL,
											 &inside, NULL);

							/* Inside the moved area */
							if (inside)
								fact = 1;

							/* In the wake of the moved area */
							else if ((approach<=0) && (sight>=0)
									&& (sight<dtol))
								fact = 1 - sight/dtol;

							/* Adjacent to the wake */
							else if ((approach<gtol) && (sight>=0)
									&& (sight<dtol))
								fact = (1 - approach/gtol) * (1 - sight/dtol);

							/* Ahead of the moved area */
							else if (dist<gtol)
								fact = 1 - dist/gtol;

							/* Not affected */
							else
								fact = 0;

							dxbuf[iy][ix] = fact * (-dxm);
							dybuf[iy][ix] = fact * (-dym);
							}
						}

					/* Define the displacement surfaces */
					grid_surface(xdisp, grid, nx, ny, dxbuf);
					xdisp->sp.origin[X] = origin[X];
					xdisp->sp.origin[Y] = origin[Y];
					xdisp->sp.orient    = orient;
					grid_surface(ydisp, grid, nx, ny, dybuf);
					ydisp->sp.origin[X] = origin[X];
					ydisp->sp.origin[Y] = origin[Y];
					ydisp->sp.orient    = orient;

					/* Interpolate old surface at distorted grid */
					put_message("spline-adjust");
					for (ix=0; ix<nx; ix++)
						{
						xm    = ix*grid;
						pm[X] = xm;
						for (iy=0; iy<ny; iy++)
							{
							ym    = iy*grid;
							pm[Y] = ym;
							p[X]  = xm*xform[X][X] + ym*xform[Y][X]
														+ xform[H][X] - dx;
							p[Y]  = xm*xform[X][Y] + ym*xform[Y][Y]
														+ xform[H][Y] - dy;
							curve_test_point(bound, p, NULL, NULL, NULL,
											 &inside, NULL);

							/* Evaluate grid at translated location */
							if (inside)
								{
								xdm = xm - dxm;
								ydm = ym - dym;
								}
							else
								{
								valid = eval_sfc_unmapped(xdisp, pm, &xval);
								valid = eval_sfc_unmapped(ydisp, pm, &yval);
								xdm   = xm + xval;
								ydm   = ym + yval;
								}
							xdm    = MAX(xdm, 0.);	xdm = MIN(xdm, xlim);
							ydm    = MAX(ydm, 0.);	ydm = MIN(ydm, ylim);
							pdm[X] = xdm;
							pdm[Y] = ydm;
							valid = eval_sfc_unmapped(EditSfc, pdm, &gval);
							sfbuf[iy][ix] = (float) gval;
							}
						}
					}

				/* Fit surface to new grid values */
				grid_surface(EditSfc, grid, nx, ny, sfbuf);
				contour_surface(EditSfc);

				/* Save translated boundary */
				xbound = copy_curve(bound);
				translate_curve(xbound, dx, dy);
				post_moved_outline(xbound);

				/* Replace labels within translated boundary (if requested) */
				if (mlabels)
					{

					/* Remove field labels within translated boundary */
					if (NotNull(EditLabs))
						{
						for (ilx=EditLabs->num-1; ilx>=0; ilx--)
							{
							spot = (SPOT) EditLabs->list[ilx];
							if (!spot) continue;

							/* Is this a label in the list? */
							for (il=0; il<nlabs; il++)
								{
								if (spot == smove[il]) break;
								}
							if (il < nlabs) continue;

							/* Only check for attached field labels?? */
							/* >>>>>
							if (spot->feature == AttachNone) continue;
							<<<<< */

							/* Check if label is within translated boundary */
							(void) curve_test_point(xbound, spot->anchor, NULL,
													NULL, NULL, &inside, NULL);
							if (inside)
								{
								(void) remove_item_from_set(EditLabs,
															(ITEM) spot);
								}
							}
						}

					/* Then translate field labels */
					(void) spot_list_translate(nlabs, smove, dx, dy);

					/* Add pasted labels to field label set */
					if (pasting)
						{
						for (il=0; il<nlabs; il++)
							(void) add_item_to_set(EditLabs, (ITEM) smove[il]);
						}
					}

				/* Remove translated boundary */
				(void) destroy_curve(xbound);

				if (EditUndoable) post_mod("surface");

				/* Modify labels and highs and lows accordingly */
				if (recompute_surface_labs(EditSfc, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				put_message("edit-done");
				pasting = FALSE;
				mstart  = FALSE;
				mlfirst = TRUE;
				busy_cursor(FALSE);

				/* Re-display the surface */
				State = ReDisplay;
				continue;

			/* Pick the centre of rotation */
			case Centre:

				/* Remove any marks from uncompleted actions */
				set = take_mf_set(TempMeta, "mark", "d", "", "");
				if (NotNull(set)) set = destroy_set(set);

				/* Re-display the outline */
				present_all();
				post_partial(TRUE);
				edit_can_copy(TRUE);

				/* Check for the boundary */
				bset  = find_mf_set(TempMeta, "curve", "c", "", "");
				if (IsNull(bset))
					{
					bound = NullCurve;
					State = Draw;
					continue;
					}
				bound  = (CURVE) bset->list[0];
				mstart = TRUE;

				/* Interpret spread factor */
				spread = define_spread();

				/* Pick the centre of rotation */
				if (pasting) put_message("spline-centre-copy");
				else         put_message("spline-centre");
				if (!ready_Xpoint(DnEdit, p0, &butt))
					{
					circle_echo(TRUE);
					return drawn;
					}
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!pick_Xpoint(DnEdit, 0, Cpos, &butt)) continue;
				if (!inside_dn_window(DnEdit, Cpos))
					{
					put_message("spline-rot-out");
					(void) sleep(1);
					continue;
					}

				mark0 = create_mark("", "", "", Cpos, 0.0);
				define_mspec(&mark0->mspec, 0, 1, NULL, 0, False, 4.0, 0.0,
							 (HILITE) 3);

				add_item_to_metafile(TempMeta, "mark",  "d", "", "",
									 (ITEM) mark0);
				present_temp(TRUE);

				/* Move on to next stage */
				State = Rotate;
				continue;

			/* Pick and rotate a reference point */
			case Rotate:

				/* Interpret spread factor */
				spread = define_spread();

				/* Pick a reference point */
				if (pasting) put_message("spline-rotate-copy");
				else         put_message("spline-rotate");
				if (!ready_Xpoint(DnEdit, p0, &butt))
					{
					circle_echo(TRUE);
					return drawn;
					}
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!inside_dn_window(DnEdit, p0))
					{
					put_message("spline-rot-out1");
					(void) ignore_Xpoint();
					continue;
					}
				if (point_dist(Cpos, p0) < SplineRes)
					{
					put_message("spline-rot-ref");
					(void) ignore_Xpoint();
					(void) sleep(1);
					continue;
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

				/* Rotate the reference point */
				put_message("spline-rot-release");
				(void) urotate_Xpoint(DnEdit, bset, Cpos, p0, p1, &Ang, &butt);
				if (!inside_dn_window(DnEdit, p1))
					{
					put_message("splin3-rot-out2");
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

				/* Move on to next stage */
				State = RotateDone;
				continue;

			/* Rotate the surface */
			case RotateDone:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_block_spline] Rotating\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);
				circle_echo(FALSE);

				/* Rotate the surface */
				put_message("spline-drag-proc2");
				dx0   = p0[X] - Cpos[X];
				dy0   = p0[Y] - Cpos[Y];
				angle = -Ang;
				cang  = cos(angle);
				sang  = sin(angle);
				gtol  = grid*(1+spread);

				/* Determine centre of rotation on spline surface */
				cx = Cpos[X]*xform[X][X] + Cpos[Y]*xform[X][Y];
				cy = Cpos[X]*xform[Y][X] + Cpos[Y]*xform[Y][Y];

				/* Recalculate grid with pasted section */
				if (pasting)
					{
					double	vi, vo, wi, wo;

					put_message("spline-adjust");
					for (ix=0; ix<nx; ix++)
						{
						xm   = ix*grid;
						for (iy=0; iy<ny; iy++)
							{
							ym   = iy*grid;
							px   = xm*xform[X][X] + ym*xform[Y][X]
														+ xform[H][X] - Cpos[X];
							py   = xm*xform[X][Y] + ym*xform[Y][Y]
														+ xform[H][Y] - Cpos[Y];
							p[X] = px*cang - py*sang + Cpos[X];
							p[Y] = px*sang + py*cang + Cpos[Y];
							curve_test_point(bound, p, &dist, NULL, NULL,
											 &inside, NULL);

							/* Determine proximity to boundary */
							if (inside)
								{
								wo = 0.0;
								wi = 1.0;
								}
							else if (dist<gtol)
								{
								wo = dist/gtol;
								wi = 1 - wo;
								}
							else
								{
								wo = 1.0;
								wi = 0.0;
								}

							/* Evaluate grid at rotated location */
							vi = vo = 0.0;
							if (wi > 0)
								{
								dx  = xm - cx;
								dy  = ym - cy;
								xdm = dx*cang - dy*sang + cx;
								ydm = dx*sang + dy*cang + cy;
								xdm    = MAX(xdm, 0.);	xdm = MIN(xdm, xlim);
								ydm    = MAX(ydm, 0.);	ydm = MIN(ydm, ylim);
								pdm[X] = xdm;
								pdm[Y] = ydm;
								valid  = eval_sfc_unmapped(psfc, pdm, &vi);
								}
							if (wo > 0)
								{
								pdm[X] = xm;
								pdm[Y] = ym;
								valid  = eval_sfc_unmapped(EditSfc, pdm, &vo);
								}
							sfbuf[iy][ix] = (float) (vi*wi + vo*wo);
							}
						}
					}

				/* Recalculate grid with rotated section */
				else
					{
					float	ca1, ca2, ca3, ca4, ca5, ca6, ca7, cangx;
					float	sa1, sa2, sa3, sa4, sa5, sa6, sa7, sangx;
					LINE	tline = NullLine;

					/* Set angles */
					angle = -Ang*0.125; ca1 = cos(angle); sa1 = sin(angle);
					angle = -Ang*0.250; ca2 = cos(angle); sa2 = sin(angle);
					angle = -Ang*0.375; ca3 = cos(angle); sa3 = sin(angle);
					angle = -Ang*0.500; ca4 = cos(angle); sa4 = sin(angle);
					angle = -Ang*0.625; ca5 = cos(angle); sa5 = sin(angle);
					angle = -Ang*0.750; ca6 = cos(angle); sa6 = sin(angle);
					angle = -Ang*0.875; ca7 = cos(angle); sa7 = sin(angle);

					/* Define new values at each location */
					for (ix=0; ix<nx; ix++)
						{
						xm    = ix*grid;
						pm[X] = xm;
						for (iy=0; iy<ny; iy++)
							{
							ym    = iy*grid;
							pm[Y] = ym;

							/* Set end location */
							q[X]  = xm*xform[X][X] + ym*xform[Y][X]
														+ xform[H][X];
							q[Y]  = xm*xform[X][Y] + ym*xform[Y][Y]
														+ xform[H][Y];

							/* Create line by rotating about reference */
							px = q[X] - Cpos[X];
							py = q[Y] - Cpos[Y];
							if (px == 0.0 && py == 0.0)
								{
								p[X] = Cpos[X];
								p[Y] = Cpos[Y];
								}
							else
								{
								if (IsNull(tline)) tline = create_line();
								else               empty_line(tline);
								add_point_to_line(tline, q);
								p[X] = px*ca1  - py*sa1  + Cpos[X];
								p[Y] = px*sa1  + py*ca1  + Cpos[Y];
								add_point_to_line(tline, p);
								p[X] = px*ca2  - py*sa2  + Cpos[X];
								p[Y] = px*sa2  + py*ca2  + Cpos[Y];
								add_point_to_line(tline, p);
								p[X] = px*ca3  - py*sa3  + Cpos[X];
								p[Y] = px*sa3  + py*ca3  + Cpos[Y];
								add_point_to_line(tline, p);
								p[X] = px*ca4  - py*sa4  + Cpos[X];
								p[Y] = px*sa4  + py*ca4  + Cpos[Y];
								add_point_to_line(tline, p);
								p[X] = px*ca5  - py*sa5  + Cpos[X];
								p[Y] = px*sa5  + py*ca5  + Cpos[Y];
								add_point_to_line(tline, p);
								p[X] = px*ca6  - py*sa6  + Cpos[X];
								p[Y] = px*sa6  + py*ca6  + Cpos[Y];
								add_point_to_line(tline, p);
								p[X] = px*ca7  - py*sa7  + Cpos[X];
								p[Y] = px*sa7  + py*ca7  + Cpos[Y];
								add_point_to_line(tline, p);
								p[X] = px*cang - py*sang + Cpos[X];
								p[Y] = px*sang + py*cang + Cpos[Y];
								add_point_to_line(tline, p);
								reverse_line(tline);
								tline = smooth_line(tline, FilterRes/4,
																SplineRes);
								}

							/* Determine proximity to boundary */
							if (px == 0.0 && py == 0.0)
								{
								dtol     = gtol;
								sight    = -1.0;
								approach = -1.0;
								copy_point(pz, ZeroPoint);
								}
							else
								{
								line_properties(tline, NULL, NULL, NULL, &dtol);
								dtol += gtol;
								line_approach(bound->line, tline, &sight,
											  &approach, pz, NULL, NULL);
								}
							curve_test_point(bound, p, &dist, NULL, NULL,
											 &inside, NULL);

#							ifdef DEBUG_ROTATE
							pr_diag("edit_block_spline",
								"  %d/%d  dtol/gtol: %.2f/%.2f  sight: %.2f  approach: %.2f  pz: %.2f/%.2f\n",
								ix, iy, dtol/grid, gtol/grid, sight/grid,
								approach/grid, pz[X]/grid, pz[Y]/grid);
#							endif /* DEBUG_ROTATE */

							/* Inside the moved area */
							if (inside)
								{
								fact = 1.0;
								xval = -Ang * fact;
								yval = 1.0;
#								ifdef DEBUG_ROTATE
								pr_diag("edit_block_spline",
									"     Inside -  fact: %.2f  xval/yval: %.2f/%.4f\n",
									fact, xval/RAD, yval);
#								endif /* DEBUG_ROTATE */
								}

							/* In the wake of the moved area */
							else if ((approach<=0) && (sight>=0)
									&& (sight<dtol))
								{
								fact = 1.0 - sight/dtol;
								xval = -Ang * fact;
								yval = 1.0;
#								ifdef DEBUG_ROTATE
								pr_diag("edit_block_spline",
									"     In Wake - fact: %.2f  xval/yval: %.2f/%.4f\n",
									fact, xval/RAD, yval);
#								endif /* DEBUG_ROTATE */
								}

							/* Adjacent to the wake */
							else if ((approach<gtol) && (sight>=0)
									&& (sight<dtol))
								{
								fact = 1.0 - sight/dtol;
								xval = -Ang * fact;
								yval = 1.0 - approach/gtol;
#								ifdef DEBUG_ROTATE
								pr_diag("edit_block_spline",
									"     Adjcent - fact: %.2f  xval/yval: %.2f/%.4f\n",
									fact, xval/RAD, yval);
#								endif /* DEBUG_ROTATE */
								}

							/* Ahead of the moved area */
							else if (dist<gtol)
								{
								fact = 1.0;
								xval = -Ang * fact;
								yval = 1.0 - dist/gtol;
#								ifdef DEBUG_ROTATE
								pr_diag("edit_block_spline",
									"     Ahead -   fact: %.2f  xval/yval: %.2f/%.4f\n",
									fact, xval/RAD, yval);
#								endif /* DEBUG_ROTATE */
								}

							/* Not affected */
							else
								{
								fact = 0.0;
								xval = 0.0;
								yval = 0.0;
#								ifdef DEBUG_ROTATE
								pr_diag("edit_block_spline",
									"     Outside - fact: %.2f  xval/yval: %.2f/%.4f\n",
									fact, xval/RAD, yval);
#								endif /* DEBUG_ROTATE */
								}

							/* Rotate back to this location */
							if (inside)
								{
								dx   = xm - cx;
								dy   = ym - cy;
								xdm  = dx*cang - dy*sang + cx;
								ydm  = dx*sang + dy*cang + cy;
								}
							else
								{
								cangx = cos(xval);
								sangx = sin(xval);
								dx    = xm - cx;
								dy    = ym - cy;
								xdm   = dx*cangx - dy*sangx + cx;
								ydm   = dx*sangx + dy*cangx + cy;
								}

#							ifdef DEBUG_ROTATE
							px = xdm;
							py = ydm;
#							endif /* DEBUG_ROTATE */

							/* Evaluate grid at rotated location */
							xdm    = MAX(xdm, 0.);	xdm = MIN(xdm, xlim);
							ydm    = MAX(ydm, 0.);	ydm = MIN(ydm, ylim);
							pdm[X] = xdm;
							pdm[Y] = ydm;
							valid = eval_sfc_unmapped(EditSfc, pdm, &gval);
							valid = eval_sfc_unmapped(EditSfc, pm,  &oval);

							if (yval > 0.0 && yval < 1.0)
								sfbuf[iy][ix] = (float) (gval*yval
															+ oval*(1.0-yval));
							else
								sfbuf[iy][ix] = (float) gval;

#							ifdef DEBUG_ROTATE
							if (inside)

								pr_diag("edit_block_spline",
									"     Inside -  pm: %.2f/%.2f  xdm/ydm: %.2f/%.2f  val: %.2f\n",
									pm[X]/grid, pm[Y]/grid, px/grid, py/grid, gval);
							else if (yval > 0.0 && yval < 1.0)
								pr_diag("edit_block_spline",
									"     Adjcent - pm: %.2f/%.2f  xdm/ydm: %.2f/%.2f  gval/oval: %.2f/%.2f  val: %.2f\n",
									pm[X]/grid, pm[Y]/grid, px/grid, py/grid, gval, oval,
									sfbuf[iy][ix]);
							else
								pr_diag("edit_block_spline",
									"     Outside - pm: %.2f/%.2f  xdm/ydm: %.2f/%.2f  val: %.2f\n",
									pm[X]/grid, pm[Y]/grid, px/grid, py/grid, gval);
#							endif /* DEBUG_ROTATE */
							}
						}
					destroy_line(tline);
					}

				/* Fit surface to new grid values */
				grid_surface(EditSfc, grid, nx, ny, sfbuf);
				contour_surface(EditSfc);

				/* Save rotated boundary */
				xbound = copy_curve(bound);
				rotate_curve(xbound, Cpos, Ang/RAD);
				post_moved_outline(xbound);

				/* Replace labels within rotated boundary (if requested) */
				if (mlabels)
					{

					/* Remove field labels within rotated boundary */
					if (NotNull(EditLabs))
						{
						for (ilx=EditLabs->num-1; ilx>=0; ilx--)
							{
							spot = (SPOT) EditLabs->list[ilx];
							if (!spot) continue;

							/* Is this a label in the list? */
							for (il=0; il<nlabs; il++)
								{
								if (spot == smove[il]) break;
								}
							if (il < nlabs) continue;

							/* Only check for attached field labels?? */
							/* >>>>>
							if (spot->feature == AttachNone) continue;
							<<<<< */

							/* Check if label is within rotated boundary */
							(void) curve_test_point(xbound, spot->anchor, NULL,
													NULL, NULL, &inside, NULL);
							if (inside)
								{
								(void) remove_item_from_set(EditLabs,
															(ITEM) spot);
								}
							}
						}

					/* Then rotate field labels */
					(void) spot_list_rotate(nlabs, smove, Cpos, Ang/RAD);

					/* Add pasted labels to field label set */
					if (pasting)
						{
						for (il=0; il<nlabs; il++)
							(void) add_item_to_set(EditLabs, (ITEM) smove[il]);
						}
					}

				/* Remove rotated boundary */
				(void) destroy_curve(xbound);

				if (EditUndoable) post_mod("surface");

				/* Modify labels and highs and lows accordingly */
				if (recompute_surface_labs(EditSfc, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				put_message("edit-done");
				pasting = FALSE;
				mstart  = FALSE;
				mlfirst = TRUE;
				busy_cursor(FALSE);

				/* Re-display the surface */
				State = ReDisplay;
				continue;

			/* Re-display the surface */
			case ReDisplay:

				/* Empty display structure */
				empty_temp();

				/* Reset boundary curve and reset displacement fields */
				bset  = NULL;
				bound = NULL;
				define_surface_spline(xdisp, m, n, NullMapProj, origin, orient,
									  grid, NULL, 0);
				define_surface_spline(ydisp, m, n, NullMapProj, origin, orient,
									  grid, NULL, 0);

				/* Empty field label list */
				if (nlabs > 0)
					{
					nlabs = 0;
					alabs = 0;
					FREEMEM(smove);
					FREEMEM(scopy);
					}

				/* Re-display the surface */
				present_all();

				/* Move on to next stage */
				State = Draw;
				continue;
			}
		}
	}

/**********************************************************************/

LOGICAL	edit_block_spline_2D

	(
	STRING	mode,
	STRING	name
	)

	{
	CURVE	curve, xbound;
	SPOT	spot, tspot;
	SET		set, tset;
	POINT	p, q, p0, p1, pz, pm, pdm;
	MARK	mark0, mark1;
	CURVE	curv0, curv1;
	int		nlines;
	LINE	*lines;
	LOGICAL	valid, inside;
	int		ix, iy, il, ilx;
	float	dx, dy, dx0, dy0, angle, cang, sang;
	float	xm, ym, xdm, ydm, dxm, dym, cx, cy, px, py;
	float	dtol, fact, dist, sight, approach;
	float	spread;
	double	gxval, gyval, xval, yval, oxval, oyval;
	int		butt;
	LOGICAL	drawn = FALSE;

	/* Current state: */
	/* - where should we resume? */
	static	enum	{ Draw, Copy, Paste,
						WaitAction, Translate, TranslateDone,
						Centre, Rotate, RotateDone, ReDisplay } State = Draw;

	/* Was a paste just done? */
	static	LOGICAL		pasting = FALSE;
	static	SURFACE		psfc    = NullSfc;

	/* Has a translate or rotate been started but not completed? */
	static	LOGICAL		mstart  = FALSE;

	/* Should labels be moved too? */
	static	LOGICAL		mlabels = TRUE;
	static	LOGICAL		mlfirst = TRUE;

	/* Boundary curve set: */
	/* - defines undistorted region */
	/* - used in cursor tracking */
	static	SET			bset  = NULL;
	static	CURVE		bound = NULL;

	/* Move list: */
	/* - list of field labels to be moved */
	/* - smove are pointers into EditLabs */
	/* - scopy are copies to be displayed in TempMeta */
	static	int			nlabs   = 0;
	static	int			alabs   = 0;
	static	SPOT		*smove  = NullSpotList;
	static	SPOT		*scopy  = NullSpotList;

	/* Centre of rotation */
	static	POINT		Cpos    = ZERO_POINT;
	static	float		Ang     = 0.0;

	/* Displacement fields: */
	/* - combined result of refits */
	/* - used to re-interpolate original surface */
	static	SURFACE	xdisp = NULL;
	static	SURFACE	ydisp = NULL;
	static	XFORM	xform = IDENT_XFORM;

	/* Temporary grid buffer: */
	/* - results of re-interpolation */
	/* - used to refit the new surface */
	static	float	*origin = NullPoint;
	static	float	orient  = 0.0;
	static	float	grid    = 0.0;
	static	float	gtol    = 0.0;
	static	int		m       = 0;
	static	int		n       = 0;
	static	int		nx      = 0;
	static	int		ny      = 0;
	static	int		mx      = 0;
	static	int		my      = 0;
	static	float	xlim    = 0.0;
	static	float	ylim    = 0.0;
	static	float	**sxbuf = NULL;
	static	float	**sybuf = NULL;
	static	float	**dxbuf = NULL;
	static	float	**dybuf = NULL;
	static	float	*sxdat  = NULL;
	static	float	*sydat  = NULL;
	static	float	*dxdat  = NULL;
	static	float	*dydat  = NULL;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_block_spline_2D] %s  State: %d\n", mode, State);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same(mode, "cancel"))
		{
		put_message("edit-cancel");
		ignore_partial();
		circle_echo(FALSE);
		drawing_control(FALSE);
		edit_allow_preset_outline(FALSE);

		if (bset)
			{
			bset  = NULL;
			bound = NULL;
			define_surface_spline(xdisp, m, n, NullMapProj, origin, orient,
								  grid, NULL, 0);
			define_surface_spline(ydisp, m, n, NullMapProj, origin, orient,
								  grid, NULL, 0);
			}
		else
			{
			grid = 0.0;
			}

		/* Empty field label list */
		if (nlabs > 0)
			{
			nlabs = 0;
			alabs = 0;
			FREEMEM(smove);
			FREEMEM(scopy);
			}

		/* Empty display structure */
		empty_temp();
		present_all();
		pasting = FALSE;
		mstart  = FALSE;
		clear_message();

		edit_select(NullCal, FALSE);
		edit_can_copy(FALSE);
		if (copy_posted("surface", CurrElement, CurrLevel))
				edit_can_paste(TRUE);
		State = Draw;
		return FALSE;
		}
	if (same(mode, "cancel all"))
		{
		ignore_partial();
		drawing_control(FALSE);
		edit_allow_preset_outline(FALSE);

		if (bset)
			{
			bset  = NULL;
			bound = NULL;
			define_surface_spline(xdisp, m, n, NullMapProj, origin, orient,
								  grid, NULL, 0);
			define_surface_spline(ydisp, m, n, NullMapProj, origin, orient,
								  grid, NULL, 0);
			}
		else
			{
			grid = 0.0;
			}

		/* Empty field label list */
		if (nlabs > 0)
			{
			nlabs = 0;
			alabs = 0;
			FREEMEM(smove);
			FREEMEM(scopy);
			}

		/* Empty display structure */
		empty_temp();
		present_all();
		pasting = FALSE;
		mstart  = FALSE;
		clear_message();

		edit_select(NullCal, FALSE);
		edit_can_copy(FALSE);
		if (copy_posted("surface", CurrElement, CurrLevel))
				edit_can_paste(TRUE);
		return FALSE;
		}

	if (same(mode, "clear"))
		{
		return FALSE;
		}

	/* See if surface exists */
	if (!EditSfc) return FALSE;

	/* Construct temporary fields on startup */
	if (same(mode, "begin") || grid <= 0.0)
		{
		ignore_partial();
		circle_echo(FALSE);
		drawing_control(FALSE);

		/* Set up temporary grid buffers */
		origin = EditSfc->sp.origin;
		orient = EditSfc->sp.orient;
		m      = EditSfc->sp.m;
		n      = EditSfc->sp.n;
		grid   = EditSfc->sp.gridlen;
		gtol   = grid*1.5;
		nx     = m - 2;
		ny     = n - 2;
		xlim   = grid*(nx-1);
		ylim   = grid*(ny-1);
		if ((nx != mx) || (ny != my))
			{
			mx    = nx;
			my    = ny;
			sxdat = GETMEM(sxdat, float, (nx*ny));
			sydat = GETMEM(sydat, float, (nx*ny));
			dxdat = GETMEM(dxdat, float, (nx*ny));
			dydat = GETMEM(dydat, float, (nx*ny));
			sxbuf = GETMEM(sxbuf, float *, ny);
			sybuf = GETMEM(sybuf, float *, ny);
			dxbuf = GETMEM(dxbuf, float *, ny);
			dybuf = GETMEM(dybuf, float *, ny);
			for (iy=0; iy<ny; iy++)
				{
				sxbuf[iy] = sxdat + iy*nx;
				sybuf[iy] = sydat + iy*nx;
				dxbuf[iy] = dxdat + iy*nx;
				dybuf[iy] = dydat + iy*nx;
				}
			}

		/* Set up displacement fields */
		if (!xdisp) xdisp = create_surface();
		if (!ydisp) ydisp = create_surface();
		define_surface_spline(xdisp, m, n, NullMapProj, origin, orient,
							  grid, NULL, 0);
		define_surface_spline(ydisp, m, n, NullMapProj, origin, orient,
							  grid, NULL, 0);

		copy_xform(xform, EditSfc->sp.xform);

		/* Set up move list */
		nlabs = 0;
		alabs = 0;
		FREEMEM(smove);
		FREEMEM(scopy);

		/* Empty display structure */
		empty_temp();
		pasting = FALSE;
		mstart  = FALSE;
		mlfirst = TRUE;

		edit_select(NullCal, FALSE);
		edit_can_copy(FALSE);
		if (copy_posted("surface", CurrElement, CurrLevel))
				edit_can_paste(TRUE);
		State = Draw;
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
				put_message("spline-move-nomode");
				(void) sleep(1);
				}
			mlfirst = FALSE;
			}

		/* Change move mode and remove labels */
		else if (mlabels)
			{

			/* Remove each field label from the list */
			tset = find_mf_set(TempMeta, "spot",  "d", "smv", "smv");
			for (il=0; il<nlabs; il++)
				{
				(void) remove_item_from_set(tset, (ITEM) scopy[il]);
				}
			nlabs = 0;
			alabs = 0;
			FREEMEM(smove);
			FREEMEM(scopy);

			/* Re-display the boundary */
			/* >>>>> test this when MoveMode added!
			present_temp(TRUE);
			<<<<< */
			present_all();

			/* Reset check for move mode */
			mlabels = FALSE;
			mlfirst = TRUE;
			}

		/* Change move mode and add labels if pasting */
		else if (pasting)
			{

			/* Re-retrieve paste buffer labels */
			paste_spline_copy(CurrElement, CurrLevel, NullSfcPtr, NullCurvePtr,
							  &nlabs, &smove);

			/* Add the pasted labels to the display */
			if (nlabs > alabs)
				{
				alabs = nlabs;
				scopy = GETMEM(scopy, SPOT, alabs);
				}
			for (il=0; il<nlabs; il++)
				{
				spot = smove[il];

				/* Highlight the pasted labels */
				scopy[il] = copy_spot(spot);
				tspot     = scopy[il];
				highlight_item("spot", (ITEM) tspot, 3);
				add_item_to_metafile(TempMeta, "spot", "d",
									 "smv", "smv", (ITEM) tspot);
				 }

			/* Re-display the boundary and field labels */
			present_temp(TRUE);
			/* >>>>> test this when MoveMode added!
			present_all();
			<<<<< */

			/* Reset check for move mode */
			mlabels = TRUE;
			mlfirst = TRUE;
			}

		/* Change move mode and add labels */
		else
			{

			/* Match current list of field labels to    */
			/*  drawn boundary and add them to the list */
			bset = find_mf_set(TempMeta, "curve", "c", "", "");
			if (NotNull(bset) && NotNull(EditLabs))
				{
				bound = (CURVE) bset->list[0];

				/* Loop through all labels in set */
				nlabs = 0;
				for (ilx=0; ilx<EditLabs->num; ilx++)
					{
					spot = (SPOT) EditLabs->list[ilx];
					if (!spot) continue;
					if (same(spot->mclass, "legend")) continue;

					/* Check if label is within drawn boundary */
					(void) curve_test_point(bound, spot->anchor, NULL,
											NULL, NULL, &inside, NULL);
					if (inside)
						{

						/* Highlight each field label */
						/*  and add it to the list    */
						nlabs++;
						if (nlabs > alabs)
							{
							alabs = nlabs;
							smove = GETMEM(smove, SPOT, alabs);
							scopy = GETMEM(scopy, SPOT, alabs);
							}
						smove[nlabs-1] = spot;
						scopy[nlabs-1] = copy_spot(spot);
						tspot          = scopy[nlabs-1];
						highlight_item("spot", (ITEM) tspot, 2);

						add_item_to_metafile(TempMeta, "spot", "d",
											 "smv", "smv", (ITEM) tspot);
						}
					}
				}

			/* Re-display the boundary and field labels */
			present_temp(TRUE);
			/* >>>>> test this when MoveMode added!
			present_all();
			<<<<< */

			/* Reset check for move mode */
			mlabels = TRUE;
			mlfirst = TRUE;
			}
		}

	/* Set state for "copy" or "paste" */
	if (same(mode, "copy"))  State = Copy;
	if (same(mode, "paste")) State = Paste;

	/* Set state for Right button "translate" or "rotate" */
	if (same(mode, "translate")) State = Translate;
	if (same(mode, "rotate"))    State = Centre;

	/* Set state for Right button "preset outline" */
	if (same(mode, "preset outline"))
		{

		/* Check for last drawn outline, last moved outline, or named outline */
		if (same(name, FpaEditorLastDrawnBoundary))
			curve = retrieve_drawn_outline();
		else if (same(name, FpaEditorLastMovedBoundary))
			curve = retrieve_moved_outline();
		else
			curve = retrieve_named_outline(name);
		if (IsNull(curve))
			{
			if (same(name, FpaEditorLastDrawnBoundary))
				{
				put_message("edit-no-drawn-outline");
				pr_warning("Editor",
					"[edit_block_spline_2D] No outline drawn yet!\n");
				}
			else if (same(name, FpaEditorLastMovedBoundary))
				{
				put_message("edit-no-moved-outline");
				pr_warning("Editor",
					"[edit_block_spline_2D] No outline moved yet!\n");
				}
			else
				{
				put_message("edit-no-preset-outline", name);
				pr_error("Editor",
					"[edit_block_spline_2D] No preset map outline: \"%s\"\n",
					name);
				}
			(void) sleep(1);
			State = Draw;
			}

		/* Display the outline */
		else
			{
			define_lspec(&curve->lspec, 255, 0, NULL, False, 2.0, 0.0,
						 (HILITE) 2);
			add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) curve);

			/* Save labels within drawn boundary (if requested) */
			if (mlabels && NotNull(EditLabs))
				{

				/* Loop through all labels in set */
				nlabs = 0;
				for (ilx=0; ilx<EditLabs->num; ilx++)
					{
					spot = (SPOT) EditLabs->list[ilx];
					if (!spot) continue;
					if (same(spot->mclass, "legend")) continue;

					/* Check if label is within drawn boundary */
					(void) curve_test_point(curve, spot->anchor, NULL,
											NULL, NULL, &inside, NULL);
					if (inside)
						{

						/* Highlight each field label */
						/*  and add it to the list    */
						nlabs++;
						if (nlabs > alabs)
							{
							alabs = nlabs;
							smove = GETMEM(smove, SPOT, alabs);
							scopy = GETMEM(scopy, SPOT, alabs);
							}
						smove[nlabs-1] = spot;
						scopy[nlabs-1] = copy_spot(spot);
						tspot          = scopy[nlabs-1];
						highlight_item("spot", (ITEM) tspot, 2);

						add_item_to_metafile(TempMeta, "spot", "d",
											 "smv", "smv", (ITEM) tspot);
						}
					}
				}

			/* Display the boundary and field labels */
			present_temp(TRUE);
			post_partial(TRUE);
			edit_select(NullCal, TRUE);
			edit_can_copy(TRUE);
			State = WaitAction;
			}
		}

	/* Repeat until told to quit */
	while (TRUE)
		{
		switch (State)
			{

			/* Draw boundary of un-distorted area */
			case Draw:

				/* Draw the boundary */
				post_partial(FALSE);
				empty_temp();
				put_message("spline-move-draw");
				edit_select(NullCal, FALSE);
				edit_can_copy(FALSE);
				if (copy_posted("surface", CurrElement, CurrLevel))
						edit_can_paste(TRUE);
				edit_allow_preset_outline(TRUE);

				if (ready_Xcurve()) butt = LeftButton;
				else if (!ready_Xpoint(DnEdit, NULL, &butt)) return drawn;

				/* Invalid button */
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					ignore_Xpoint();
					continue;
					}

				/* Draw the curve */
				edit_can_paste(FALSE);
				drawing_control(TRUE);
				if (!ready_Xcurve())
					{
					if (drawing_Xcurve()) return drawn;
					(void) draw_Xcurve(DnEdit, FilterRes*4, SplineRes*4, TRUE);
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

				/* Extract and display curve */
				curve = create_curve("", "", "");
				add_line_to_curve(curve, lines[0]);
				post_drawn_outline(curve);
				reset_pipe();
				define_lspec(&curve->lspec, 255, 0, NULL, False, 2.0, 0.0,
							 (HILITE) 2);
				add_item_to_metafile(TempMeta, "curve", "c", "", "",
									 (ITEM) curve);

				/* Save labels within drawn boundary (if requested) */
				if (mlabels && NotNull(EditLabs))
					{

					/* Loop through all labels in set */
					nlabs = 0;
					for (ilx=0; ilx<EditLabs->num; ilx++)
						{
						spot = (SPOT) EditLabs->list[ilx];
						if (!spot) continue;
						if (same(spot->mclass, "legend")) continue;

						/* Check if label is within drawn boundary */
						(void) curve_test_point(curve, spot->anchor, NULL,
												NULL, NULL, &inside, NULL);
						if (inside)
							{

							/* Highlight each field label */
							/*  and add it to the list    */
							nlabs++;
							if (nlabs > alabs)
								{
								alabs = nlabs;
								smove = GETMEM(smove, SPOT, alabs);
								scopy = GETMEM(scopy, SPOT, alabs);
								}
							smove[nlabs-1] = spot;
							scopy[nlabs-1] = copy_spot(spot);
							tspot          = scopy[nlabs-1];
							highlight_item("spot", (ITEM) tspot, 2);

							add_item_to_metafile(TempMeta, "spot", "d",
												 "smv", "smv", (ITEM) tspot);
							}
						}
					}

				/* Display the boundary and field labels */
				present_temp(TRUE);
				post_partial(TRUE);
				edit_select(NullCal, TRUE);

				/* Move on to next stage */
				edit_can_copy(TRUE);
				State = WaitAction;
				continue;

			/* Copy surface */
			case Copy:

				/* Check for the boundary */
				bset  = find_mf_set(TempMeta, "curve", "c", "", "");
				if (IsNull(bset))
					{
					bound = NullCurve;
					State = Draw;
					continue;
					}
				bound = (CURVE) bset->list[0];

				/* Save the surface and boundary in the copy buffer */
				put_message("edit-copy");
				post_spline_copy(CurrElement, CurrLevel, EditSfc, bound,
								 nlabs, smove);
				clear_message();
				pasting = FALSE;

				/* Move on to next stage */
				edit_can_copy(TRUE);
				State = WaitAction;
				continue;

			/* Paste surface */
			case Paste:

				put_message("edit-paste");

				/* Empty move list */
				bset  = find_mf_set(TempMeta, "curve", "c", "", "");
				if (NotNull(bset))
					{
					ignore_partial();
					edit_can_copy(FALSE);
					bset  = NULL;
					bound = NULL;
					define_surface_spline(xdisp, m, n, NullMapProj, origin,
										  orient, grid, NULL, 0);
					define_surface_spline(ydisp, m, n, NullMapProj, origin,
										  orient, grid, NULL, 0);

					/* Empty display structure */
					empty_temp();
					}

				/* Retrieve paste buffer */
				paste_spline_copy(CurrElement, CurrLevel, &psfc, &bound,
								  &nlabs, &smove);
				if (IsNull(psfc) || IsNull(bound))
					{
					edit_can_paste(FALSE);
					put_message("edit-no-paste");
					(void) sleep(1);
					edit_select(NullCal, FALSE);
					State = Draw;
					return FALSE;
					}

				/* Highlight the paste surface */
				highlight_surface(psfc, 3);
				add_sfc_to_metafile(TempMeta, "v", "", "", psfc);
				add_item_to_metafile(TempMeta, "curve", "c", "", "",
									 (ITEM) bound);

				/* Then add the pasted labels to the display (if requested) */
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

						/* Highlight the pasted labels */
						scopy[il] = copy_spot(spot);
						tspot     = scopy[il];
						highlight_item("spot", (ITEM) tspot, 3);
						add_item_to_metafile(TempMeta, "spot", "d",
											 "smv", "smv", (ITEM) tspot);
						}
					}

				/* Re-display the paste surface */
				present_temp(TRUE);
				post_partial(TRUE);
				edit_select(NullCal, TRUE);
				pasting = TRUE;
				clear_message();

				/* Move on to next stage */
				State = WaitAction;
				continue;

			/* Wait around for a "translate" or "rotate" */
			case WaitAction:

				/* Interpret spread factor */
				spread = define_spread();
				circle_echo(TRUE);

				/* Keep returning until we get a "translate" or "rotate" */
				put_message("spline-move-action");
				edit_allow_preset_outline(FALSE);
				edit_can_copy(TRUE);
				return FALSE;

			/* Pick and translate a reference point */
			case Translate:

				/* Remove any marks from uncompleted actions */
				set = take_mf_set(TempMeta, "mark", "d", "", "");
				if (NotNull(set)) set = destroy_set(set);

				/* Re-display the outline */
				present_all();
				post_partial(TRUE);
				edit_can_copy(TRUE);

				/* Check for the boundary */
				bset   = find_mf_set(TempMeta, "curve", "c", "", "");
				if (IsNull(bset))
					{
					bound = NullCurve;
					State = Draw;
					continue;
					}
				bound  = (CURVE) bset->list[0];
				mstart = TRUE;

				/* Interpret spread factor */
				spread = define_spread();

				/* Pick a reference point */
				if (pasting) put_message("spline-tran-copy");
				else         put_message("spline-tran");
				if (!ready_Xpoint(DnEdit, p0, &butt))
					{
					circle_echo(TRUE);
					return drawn;
					}
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!inside_dn_window(DnEdit, p0))
					{
					put_message("spline-tran-out1");
					(void) ignore_Xpoint();
					continue;
					}

				/* Try with any point ... skip check for inside boundary!
				curve_test_point(bound, p0, NULL, NULL, NULL, &inside, NULL);
				if (!inside)
					{
					put_message("spline-tran-out2");
					(void) ignore_Xpoint();
					continue;
					}
				*/

				/* Translate the reference point */
				put_message("spline-tran-rel");
				(void) utrack_Xpoint(DnEdit, bset, p0, p1, &butt);
				if (!inside_dn_window(DnEdit, p1))
					{
					put_message("spline-tran-out3");
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

			/* Translate the surface */
			case TranslateDone:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_block_spline_2D] Translating\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);
				circle_echo(FALSE);

				/* Translate the surface */
				put_message("spline-drag-proc2");
				dx    = p1[X] - p0[X];
				dy    = p1[Y] - p0[Y];
				gtol  = grid*(1+spread);
				dtol  = hypot(dx, dy) + gtol;
				dxm   = dx*xform[X][X] + dy*xform[X][Y];
				dym   = dx*xform[Y][X] + dy*xform[Y][Y];

				/* Recalculate grid with pasted section */
				if (pasting)
					{
					double	ui, uo, vi, vo, wi, wo;

					put_message("spline-adjust");
					for (ix=0; ix<nx; ix++)
						{
						xm   = ix*grid;
						for (iy=0; iy<ny; iy++)
							{
							ym   = iy*grid;
							p[X] = xm*xform[X][X] + ym*xform[Y][X]
														+ xform[H][X] - dx;
							p[Y] = xm*xform[X][Y] + ym*xform[Y][Y]
														+ xform[H][Y] - dy;
							curve_test_point(bound, p, &dist, NULL, NULL,
											 &inside, NULL);

							/* Determine proximity to boundary */
							if (inside)
								{
								wo = 0.0;
								wi = 1.0;
								}
							else if (dist<gtol)
								{
								wo = dist/gtol;
								wi = 1 - wo;
								}
							else
								{
								wo = 1.0;
								wi = 0.0;
								}

							/* Evaluate grid at translated location */
							ui = vi = uo = vo = 0.0;
							if (wi > 0)
								{
								xdm = xm - dxm;
								ydm = ym - dym;
								xdm    = MAX(xdm, 0.);	xdm = MIN(xdm, xlim);
								ydm    = MAX(ydm, 0.);	ydm = MIN(ydm, ylim);
								pdm[X] = xdm;
								pdm[Y] = ydm;
								valid  = eval_sfc_UV_unmapped(psfc, pdm,
															  &ui, &vi);
								}
							if (wo > 0)
								{
								pdm[X] = xm;
								pdm[Y] = ym;
								valid  = eval_sfc_UV_unmapped(psfc, pdm,
															  &uo, &vo);
								}

							sxbuf[iy][ix] = (float) (ui*wi + uo*wo);
							sybuf[iy][ix] = (float) (vi*wi + vo*wo);
							}
						}
					}

				/* Recalculate grid with translated section */
				else
					{
					/* Set up points to define displacement fields */
					for (ix=0; ix<nx; ix++)
						{
						xm   = ix*grid;
						for (iy=0; iy<ny; iy++)
							{
							ym   = iy*grid;
							q[X] = xm*xform[X][X] + ym*xform[Y][X]
														+ xform[H][X];
							q[Y] = xm*xform[X][Y] + ym*xform[Y][Y]
														+ xform[H][Y];
							p[X] = q[X] - dx;
							p[Y] = q[Y] - dy;
							curve_sight(bound, p, q, TRUE, &sight, &approach,
										NULL, NULL, NULL);
							curve_test_point(bound, p, &dist, NULL, NULL,
											 &inside, NULL);

							/* Inside the moved area */
							if (inside)
								fact = 1;

							/* In the wake of the moved area */
							else if ((approach<=0) && (sight>=0)
									&& (sight<dtol))
								fact = 1 - sight/dtol;

							/* Adjacent to the wake */
							else if ((approach<gtol) && (sight>=0)
									&& (sight<dtol))
								fact = (1 - approach/gtol) * (1 - sight/dtol);

							/* Ahead of the moved area */
							else if (dist<gtol)
								fact = 1 - dist/gtol;

							/* Not affected */
							else
								fact = 0;

							dxbuf[iy][ix] = fact * (-dxm);
							dybuf[iy][ix] = fact * (-dym);
							}
						}

					/* Define the displacement surfaces */
					grid_surface(xdisp, grid, nx, ny, dxbuf);
					xdisp->sp.origin[X] = origin[X];
					xdisp->sp.origin[Y] = origin[Y];
					xdisp->sp.orient    = orient;
					grid_surface(ydisp, grid, nx, ny, dybuf);
					ydisp->sp.origin[X] = origin[X];
					ydisp->sp.origin[Y] = origin[Y];
					ydisp->sp.orient    = orient;

					/* Interpolate old surface at distorted grid */
					put_message("spline-adjust");
					for (ix=0; ix<nx; ix++)
						{
						xm    = ix*grid;
						pm[X] = xm;
						for (iy=0; iy<ny; iy++)
							{
							ym    = iy*grid;
							pm[Y] = ym;
							p[X]  = xm*xform[X][X] + ym*xform[Y][X]
														+ xform[H][X] - dx;
							p[Y]  = xm*xform[X][Y] + ym*xform[Y][Y]
														+ xform[H][Y] - dy;
							curve_test_point(bound, p, NULL, NULL, NULL,
											 &inside, NULL);

							/* Evaluate grid at translated location */
							if (inside)
								{
								xdm = xm - dxm;
								ydm = ym - dym;
								}
							else
								{
								valid = eval_sfc_unmapped(xdisp, pm, &xval);
								valid = eval_sfc_unmapped(ydisp, pm, &yval);
								xdm   = xm + xval;
								ydm   = ym + yval;
								}
							xdm    = MAX(xdm, 0.);	xdm = MIN(xdm, xlim);
							ydm    = MAX(ydm, 0.);	ydm = MIN(ydm, ylim);
							pdm[X] = xdm;
							pdm[Y] = ydm;
							valid = eval_sfc_UV_unmapped(EditSfc, pdm,
														 &gxval, &gyval);
							sxbuf[iy][ix] = (float) gxval;
							sybuf[iy][ix] = (float) gyval;
							}
						}
					}

				/* Fit surface to new grid values */
				grid_surface_2D(EditSfc, grid, nx, ny, sxbuf, sybuf);
				contour_surface(EditSfc);

				/* Save translated boundary */
				xbound = copy_curve(bound);
				translate_curve(xbound, dx, dy);
				post_moved_outline(xbound);

				/* Replace labels within translated boundary (if requested) */
				if (mlabels)
					{

					/* Remove field labels within translated boundary */
					if (NotNull(EditLabs))
						{
						for (ilx=EditLabs->num-1; ilx>=0; ilx--)
							{
							spot = (SPOT) EditLabs->list[ilx];
							if (!spot) continue;

							/* Is this a label in the list? */
							for (il=0; il<nlabs; il++)
								{
								if (spot == smove[il]) break;
								}
							if (il < nlabs) continue;

							/* Only check for attached field labels?? */
							/* >>>>>
							if (spot->feature == AttachNone) continue;
							<<<<< */

							/* Check if label is within translated boundary */
							(void) curve_test_point(xbound, spot->anchor, NULL,
													NULL, NULL, &inside, NULL);
							if (inside)
								{
								(void) remove_item_from_set(EditLabs,
															(ITEM) spot);
								}
							}
						}

					/* Then translate field labels */
					(void) spot_list_translate(nlabs, smove, dx, dy);

					/* Add pasted labels to field label set */
					if (pasting)
						{
						for (il=0; il<nlabs; il++)
							(void) add_item_to_set(EditLabs, (ITEM) smove[il]);
						}
					}

				/* Remove translated boundary */
				(void) destroy_curve(xbound);

				if (EditUndoable) post_mod("surface");

				/* Modify labels and highs and lows accordingly */
				if (recompute_surface_labs(EditSfc, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				put_message("edit-done");
				pasting = FALSE;
				mstart  = FALSE;
				mlfirst = TRUE;
				busy_cursor(FALSE);

				/* Re-display the surface */
				State = ReDisplay;
				continue;

			/* Pick the centre of rotation */
			case Centre:

				/* Remove any marks from uncompleted actions */
				set = take_mf_set(TempMeta, "mark", "d", "", "");
				if (NotNull(set)) set = destroy_set(set);

				/* Re-display the outline */
				present_all();
				post_partial(TRUE);
				edit_can_copy(TRUE);

				/* Check for the boundary */
				bset  = find_mf_set(TempMeta, "curve", "c", "", "");
				if (IsNull(bset))
					{
					bound = NullCurve;
					State = Draw;
					continue;
					}
				bound  = (CURVE) bset->list[0];
				mstart = TRUE;

				/* Interpret spread factor */
				spread = define_spread();

				/* Pick the centre of rotation */
				if (pasting) put_message("spline-centre-copy");
				else         put_message("spline-centre");
				if (!ready_Xpoint(DnEdit, p0, &butt))
					{
					circle_echo(TRUE);
					return drawn;
					}
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!pick_Xpoint(DnEdit, 0, Cpos, &butt)) continue;
				if (!inside_dn_window(DnEdit, Cpos))
					{
					put_message("spline-rot-out");
					(void) sleep(1);
					continue;
					}

				mark0 = create_mark("", "", "", Cpos, 0.0);
				define_mspec(&mark0->mspec, 0, 1, NULL, 0, False, 4.0, 0.0,
							 (HILITE) 3);

				add_item_to_metafile(TempMeta, "mark",  "d", "", "",
									 (ITEM) mark0);
				present_temp(TRUE);

				/* Move on to next stage */
				State = Rotate;
				continue;

			/* Pick and rotate a reference point */
			case Rotate:

				/* Interpret spread factor */
				spread = define_spread();

				/* Pick a reference point */
				if (pasting) put_message("spline-rotate-copy");
				else         put_message("spline-rotate");
				if (!ready_Xpoint(DnEdit, p0, &butt))
					{
					circle_echo(TRUE);
					return drawn;
					}
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!inside_dn_window(DnEdit, p0))
					{
					put_message("spline-rot-out1");
					(void) ignore_Xpoint();
					continue;
					}
				if (point_dist(Cpos, p0) < SplineRes)
					{
					put_message("spline-rot-ref");
					(void) ignore_Xpoint();
					(void) sleep(1);
					continue;
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

				/* Rotate the reference point */
				put_message("spline-rot-release");
				(void) urotate_Xpoint(DnEdit, bset, Cpos, p0, p1, &Ang, &butt);
				if (!inside_dn_window(DnEdit, p1))
					{
					put_message("splin3-rot-out2");
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

				/* Move on to next stage */
				State = RotateDone;
				continue;

			/* Rotate the surface */
			case RotateDone:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_block_spline_2D] Rotating\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);
				circle_echo(FALSE);

				/* Rotate the surface */
				put_message("spline-drag-proc2");
				dx0   = p0[X] - Cpos[X];
				dy0   = p0[Y] - Cpos[Y];
				angle = -Ang;
				cang  = cos(angle);
				sang  = sin(angle);
				gtol  = grid*(1+spread);

				/* Determine centre of rotation on spline surface */
				cx = Cpos[X]*xform[X][X] + Cpos[Y]*xform[X][Y];
				cy = Cpos[X]*xform[Y][X] + Cpos[Y]*xform[Y][Y];

				/* Recalculate grid with pasted section */
				if (pasting)
					{
					double	ui, uo, vi, vo, wi, wo;

					put_message("spline-adjust");
					for (ix=0; ix<nx; ix++)
						{
						xm   = ix*grid;
						for (iy=0; iy<ny; iy++)
							{
							ym   = iy*grid;
							px   = xm*xform[X][X] + ym*xform[Y][X]
														+ xform[H][X] - Cpos[X];
							py   = xm*xform[X][Y] + ym*xform[Y][Y]
														+ xform[H][Y] - Cpos[Y];
							p[X] = px*cang - py*sang + Cpos[X];
							p[Y] = px*sang + py*cang + Cpos[Y];
							curve_test_point(bound, p, &dist, NULL, NULL,
											 &inside, NULL);

							/* Determine proximity to boundary */
							if (inside)
								{
								wo = 0.0;
								wi = 1.0;
								}
							else if (dist<gtol)
								{
								wo = dist/gtol;
								wi = 1 - wo;
								}
							else
								{
								wo = 1.0;
								wi = 0.0;
								}

							/* Evaluate grid at rotated location */
							ui = uo = vi = vo = 0.0;
							if (wi > 0)
								{
								dx  = xm - cx;
								dy  = ym - cy;
								xdm = dx*cang - dy*sang + cx;
								ydm = dx*sang + dy*cang + cy;
								xdm    = MAX(xdm, 0.);	xdm = MIN(xdm, xlim);
								ydm    = MAX(ydm, 0.);	ydm = MIN(ydm, ylim);
								pdm[X] = xdm;
								pdm[Y] = ydm;
								valid  = eval_sfc_UV_unmapped(psfc, pdm,
															  &ui, &vi);
								}
							if (wo > 0)
								{
								pdm[X] = xm;
								pdm[Y] = ym;
								valid  = eval_sfc_UV_unmapped(EditSfc, pdm,
															  &uo, &vo);
								}
							sxbuf[iy][ix] = (float) (ui*wi + uo*wo);
							sybuf[iy][ix] = (float) (vi*wi + vo*wo);
							}
						}
					}

				/* Recalculate grid with rotated section */
				else
					{
					float	ca1, ca2, ca3, ca4, ca5, ca6, ca7, cangx;
					float	sa1, sa2, sa3, sa4, sa5, sa6, sa7, sangx;
					LINE	tline = NullLine;

					/* Set angles */
					angle = -Ang*0.125; ca1 = cos(angle); sa1 = sin(angle);
					angle = -Ang*0.250; ca2 = cos(angle); sa2 = sin(angle);
					angle = -Ang*0.375; ca3 = cos(angle); sa3 = sin(angle);
					angle = -Ang*0.500; ca4 = cos(angle); sa4 = sin(angle);
					angle = -Ang*0.625; ca5 = cos(angle); sa5 = sin(angle);
					angle = -Ang*0.750; ca6 = cos(angle); sa6 = sin(angle);
					angle = -Ang*0.875; ca7 = cos(angle); sa7 = sin(angle);

					/* Define new values at each location */
					for (ix=0; ix<nx; ix++)
						{
						xm    = ix*grid;
						pm[X] = xm;
						for (iy=0; iy<ny; iy++)
							{
							ym    = iy*grid;
							pm[Y] = ym;

							/* Set end location */
							q[X]  = xm*xform[X][X] + ym*xform[Y][X]
														+ xform[H][X];
							q[Y]  = xm*xform[X][Y] + ym*xform[Y][Y]
														+ xform[H][Y];

							/* Create line by rotating about reference */
							px = q[X] - Cpos[X];
							py = q[Y] - Cpos[Y];
							if (px == 0.0 && py == 0.0)
								{
								p[X] = Cpos[X];
								p[Y] = Cpos[Y];
								}
							else
								{
								if (IsNull(tline)) tline = create_line();
								else               empty_line(tline);
								add_point_to_line(tline, q);
								p[X] = px*ca1  - py*sa1  + Cpos[X];
								p[Y] = px*sa1  + py*ca1  + Cpos[Y];
								add_point_to_line(tline, p);
								p[X] = px*ca2  - py*sa2  + Cpos[X];
								p[Y] = px*sa2  + py*ca2  + Cpos[Y];
								add_point_to_line(tline, p);
								p[X] = px*ca3  - py*sa3  + Cpos[X];
								p[Y] = px*sa3  + py*ca3  + Cpos[Y];
								add_point_to_line(tline, p);
								p[X] = px*ca4  - py*sa4  + Cpos[X];
								p[Y] = px*sa4  + py*ca4  + Cpos[Y];
								add_point_to_line(tline, p);
								p[X] = px*ca5  - py*sa5  + Cpos[X];
								p[Y] = px*sa5  + py*ca5  + Cpos[Y];
								add_point_to_line(tline, p);
								p[X] = px*ca6  - py*sa6  + Cpos[X];
								p[Y] = px*sa6  + py*ca6  + Cpos[Y];
								add_point_to_line(tline, p);
								p[X] = px*ca7  - py*sa7  + Cpos[X];
								p[Y] = px*sa7  + py*ca7  + Cpos[Y];
								add_point_to_line(tline, p);
								p[X] = px*cang - py*sang + Cpos[X];
								p[Y] = px*sang + py*cang + Cpos[Y];
								add_point_to_line(tline, p);
								reverse_line(tline);
								tline = smooth_line(tline, FilterRes/4,
																SplineRes);
								}

							/* Determine proximity to boundary */
							if (px == 0.0 && py == 0.0)
								{
								dtol     = gtol;
								sight    = -1.0;
								approach = -1.0;
								copy_point(pz, ZeroPoint);
								}
							else
								{
								line_properties(tline, NULL, NULL, NULL, &dtol);
								dtol += gtol;
								line_approach(bound->line, tline, &sight,
											  &approach, pz, NULL, NULL);
								}
							curve_test_point(bound, p, &dist, NULL, NULL,
											 &inside, NULL);

							/* Inside the moved area */
							if (inside)
								{
								fact = 1.0;
								xval = -Ang * fact;
								yval = 1.0;
								}

							/* In the wake of the moved area */
							else if ((approach<=0) && (sight>=0)
									&& (sight<dtol))
								{
								fact = 1.0 - sight/dtol;
								xval = -Ang * fact;
								yval = 1.0;
								}

							/* Adjacent to the wake */
							else if ((approach<gtol) && (sight>=0)
									&& (sight<dtol))
								{
								fact = 1.0 - sight/dtol;
								xval = -Ang * fact;
								yval = 1.0 - approach/gtol;
								}

							/* Ahead of the moved area */
							else if (dist<gtol)
								{
								fact = 1.0;
								xval = -Ang * fact;
								yval = 1.0 - dist/gtol;
								}

							/* Not affected */
							else
								{
								fact = 0.0;
								xval = 0.0;
								yval = 0.0;
								}

							/* Rotate back to this location */
							if (inside)
								{
								dx   = xm - cx;
								dy   = ym - cy;
								xdm  = dx*cang - dy*sang + cx;
								ydm  = dx*sang + dy*cang + cy;
								}
							else
								{
								cangx = cos(xval);
								sangx = sin(xval);
								dx    = xm - cx;
								dy    = ym - cy;
								xdm   = dx*cangx - dy*sangx + cx;
								ydm   = dx*sangx + dy*cangx + cy;
								}

							/* Evaluate grid at rotated location */
							xdm    = MAX(xdm, 0.);	xdm = MIN(xdm, xlim);
							ydm    = MAX(ydm, 0.);	ydm = MIN(ydm, ylim);
							pdm[X] = xdm;
							pdm[Y] = ydm;
							valid = eval_sfc_UV_unmapped(EditSfc, pdm,
														 &gxval, &gyval);
							valid = eval_sfc_UV_unmapped(EditSfc, pm,
														 &oxval, &oyval);

							if (yval > 0.0 && yval < 1.0)
								{
								sxbuf[iy][ix] = (float) (gxval*yval
															+ oxval*(1.0-yval));
								sybuf[iy][ix] = (float) (gyval*yval
															+ oyval*(1.0-yval));
								}
							else
								{
								sxbuf[iy][ix] = (float) gxval;
								sybuf[iy][ix] = (float) gyval;
								}
							}
						}
					destroy_line(tline);
					}

				/* Fit surface to new grid values */
				grid_surface_2D(EditSfc, grid, nx, ny, sxbuf, sybuf);
				contour_surface(EditSfc);

				/* Save rotated boundary */
				xbound = copy_curve(bound);
				rotate_curve(xbound, Cpos, Ang/RAD);
				post_moved_outline(xbound);

				/* Replace labels within rotated boundary (if requested) */
				if (mlabels)
					{

					/* Remove field labels within rotated boundary */
					if (NotNull(EditLabs))
						{
						for (ilx=EditLabs->num-1; ilx>=0; ilx--)
							{
							spot = (SPOT) EditLabs->list[ilx];
							if (!spot) continue;

							/* Is this a label in the list? */
							for (il=0; il<nlabs; il++)
								{
								if (spot == smove[il]) break;
								}
							if (il < nlabs) continue;

							/* Only check for attached field labels?? */
							/* >>>>>
							if (spot->feature == AttachNone) continue;
							<<<<< */

							/* Check if label is within rotated boundary */
							(void) curve_test_point(xbound, spot->anchor, NULL,
													NULL, NULL, &inside, NULL);
							if (inside)
								{
								(void) remove_item_from_set(EditLabs,
															(ITEM) spot);
								}
							}
						}

					/* Then rotate field labels */
					(void) spot_list_rotate(nlabs, smove, Cpos, Ang/RAD);

					/* Add pasted labels to field label set */
					if (pasting)
						{
						for (il=0; il<nlabs; il++)
							(void) add_item_to_set(EditLabs, (ITEM) smove[il]);
						}
					}

				/* Remove rotated boundary */
				(void) destroy_curve(xbound);

				if (EditUndoable) post_mod("surface");

				/* Modify labels and highs and lows accordingly */
				if (recompute_surface_labs(EditSfc, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				put_message("edit-done");
				pasting = FALSE;
				mstart  = FALSE;
				mlfirst = TRUE;
				busy_cursor(FALSE);

				/* Re-display the surface */
				State = ReDisplay;
				continue;

			/* Re-display the surface */
			case ReDisplay:

				/* Empty display structure */
				empty_temp();

				/* Reset boundary curve and reset displacement fields */
				bset  = NULL;
				bound = NULL;
				define_surface_spline(xdisp, m, n, NullMapProj, origin, orient,
									  grid, NULL, 0);
				define_surface_spline(ydisp, m, n, NullMapProj, origin, orient,
									  grid, NULL, 0);

				/* Empty field label list */
				if (nlabs > 0)
					{
					nlabs = 0;
					alabs = 0;
					FREEMEM(smove);
					FREEMEM(scopy);
					}

				/* Re-display the surface */
				present_all();

				/* Move on to next stage */
				State = Draw;
				continue;
			}
		}
	}

/***********************************************************************
*                                                                      *
*     m e r g e _ s p l i n e                                          *
*     m e r g e _ s p l i n e _ 2 D                                    *
*                                                                      *
***********************************************************************/

LOGICAL	edit_merge_spline

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
	CURVE	curve, xbound;
	SPOT	spot;
	SET		set, tset;
	POINT	p, p0, pz, p1, pm, pdm;
	MARK	mark0, mark1;
	CURVE	curv0, curv1;
	int		nlines;
	LINE	*lines;
	LOGICAL	valid, inside;
	int		ix, iy, il, ilx;
	float	dx, dy, dx0, dy0, angle, cang, sang;
	float	xm, ym, xdm, ydm, dxm, dym, cx, cy, px, py;
	float	spread, dist;
	double	vi, vo, wi, wo;
	int		butt;
	LOGICAL	drawn = FALSE;

	STRING			dpath;
	FLD_DESCRIPT	fdesc;

	/* Current state: */
	/* - where should we resume? */
	static	enum	{ Wait, Fetch, ReFetch, Draw, ReDraw,
						WaitAction, Merge, Translate, TranslateDone,
						Centre, Rotate, RotateDone, ReDisplay } State = Wait;

	/* Is a fetch being done? */
	static	LOGICAL		fetch   = FALSE;

	/* Was a draw started but not completed? */
	static	LOGICAL		dstart  = FALSE;

	/* Has a merge or translate or rotate been started or completed? */
	static	LOGICAL		mstart  = FALSE;
	static	LOGICAL		maction = FALSE;

	/* Should labels be merged too? */
	static	LOGICAL		mlabels = FALSE;
	static	LOGICAL		mlfirst = TRUE;

	/* Boundary curve set: */
	/* - defines undistorted region */
	/* - used in cursor tracking */
	static	SET		bset    = NULL;
	static	CURVE	bound   = NULL;

	/* Move list: */
	/* - list of field labels to be moved */
	/* - smove are pointers into merge field set */
	/* - scopy are copies to be added to EditLabs */
	static	int		nlabs   = 0;
	static	int		alabs   = 0;
	static	SPOT	*smove  = NullSpotList;
	static	SPOT	*scopy  = NullSpotList;

	/* Field to merge from */
	static	STRING	msource  = NullString;
	static	STRING	msubsrc  = NullString;
	static	STRING	mrtime   = NullString;
	static	STRING	mvtime   = NullString;
	static	STRING	melement = NullString;
	static	STRING	mlevel   = NullString;
	static	SURFACE	msfc     = NullSfc;
	static	SET		sset     = NullSet;
	static	LOGICAL	savesfc  = FALSE;

	/* Centre of rotation */
	static	POINT		Cpos = ZERO_POINT;
	static	float		Ang  = 0.0;

	/* Temporary grid buffer: */
	/* - results of re-interpolation */
	/* - used to refit the new surface */
	static	float	grid    = 0.0;
	static	float	gtol    = 0.0;
	static	int		m       = 0;
	static	int		n       = 0;
	static	int		nx      = 0;
	static	int		ny      = 0;
	static	int		mx      = 0;
	static	int		my      = 0;
	static	float	xlim    = 0.0;
	static	float	ylim    = 0.0;
	static	float	**sfbuf = NULL;
	static	float	*sfdat  = NULL;
	static	XFORM	xform   = IDENT_XFORM;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_merge_spline] %s  State: %d\n", mode, State);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same(mode, "cancel"))
		{
		put_message("edit-cancel");
		circle_echo(FALSE);
		drawing_control(FALSE);
		switch (State)
			{
			case Translate:
			case Centre:
			case Rotate:
			case WaitAction:
							ignore_partial();
							edit_allow_preset_outline(FALSE);
							State = (maction)? ReDisplay: ReDraw;
							break;

			case Draw:		edit_allow_preset_outline(FALSE);
							State  = (dstart)? ReDraw: ReFetch;
							dstart = FALSE;
							grid   = 0.0;
							break;

			default:		ignore_partial();
							edit_allow_preset_outline(FALSE);

							/* Get rid of boundary */
							if (bset)
								{
								bset  = NULL;
								bound = NULL;
								}
							edit_select(NullCal, FALSE);

							/* Empty field label list */
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
								}

							/* Empty display structure */
							empty_temp();

							/* Get rid of fetched field */
							if (savesfc)
								{
								msfc = destroy_surface(msfc);
								sset = destroy_set(sset);
								}
							else
								{
								msfc = NullSfc; /* destroyed by call to */
								sset = NullSet; /*  empty_temp above    */
								}
							FREEMEM(msource);
							FREEMEM(msubsrc);
							FREEMEM(mrtime);
							FREEMEM(mvtime);
							FREEMEM(melement);
							FREEMEM(mlevel);
							savesfc = FALSE;
							return FALSE;
			}
		}
	if (same(mode, "cancel all"))
		{
		ignore_partial();
		drawing_control(FALSE);
		edit_allow_preset_outline(FALSE);

		/* Get rid of boundary */
		if (bset)
			{
			bset  = NULL;
			bound = NULL;
			}
		edit_select(NullCal, FALSE);

		/* Empty field label list */
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
			}

		/* Empty display structure */
		empty_temp();

		/* Get rid of fetched field */
		if (savesfc)
			{
			msfc = destroy_surface(msfc);
			sset = destroy_set(sset);
			}
		else
			{
			msfc = NullSfc; /* destroyed by call to */
			sset = NullSet; /*  empty_temp above    */
			}
		FREEMEM(msource);
		FREEMEM(msubsrc);
		FREEMEM(mrtime);
		FREEMEM(mvtime);
		FREEMEM(melement);
		FREEMEM(mlevel);
		savesfc = FALSE;
		return FALSE;
		}

	if (same(mode, "clear"))
		{
		return FALSE;
		}

	/* See if surface exists */
	if (!EditSfc) return FALSE;

	/* Construct temporary fields on startup */
	if (same(mode, "begin") || grid <= 0.0)
		{
		ignore_partial();

		/* Set up temporary grid buffers */
		m      = EditSfc->sp.m;
		n      = EditSfc->sp.n;
		grid   = EditSfc->sp.gridlen;
		gtol   = grid*1.5;
		nx     = m - 2;
		ny     = n - 2;
		xlim   = grid*(nx-1);
		ylim   = grid*(ny-1);
		if ((nx != mx) || (ny != my))
			{
			mx    = nx;
			my    = ny;
			sfdat = GETMEM(sfdat, float, (nx*ny));
			sfbuf = GETMEM(sfbuf, float *, ny);
			for (iy=0; iy<ny; iy++) sfbuf[iy] = sfdat + iy*nx;
			}

		copy_xform(xform, EditSfc->sp.xform);

		if (same(mode, "begin"))
			{

			/* Set up move list */
			nlabs = 0;
			alabs = 0;
			FREEMEM(smove);
			FREEMEM(scopy);

			empty_temp();
			dstart  = FALSE;
			mstart  = FALSE;
			maction = FALSE;
			savesfc = FALSE;
			edit_select(NullCal, FALSE);
			State = Wait;
			}
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
				put_message("spline-merge-nomode");
				(void) sleep(1);
				}
			mlfirst = FALSE;
			}

		/* Change merge mode and remove labels */
		else if (mlabels)
			{

			/* Un-highlight each field label and remove it from the list */
			for (il=0; il<nlabs; il++)
				{
				highlight_item("spot", (ITEM) smove[il], 4);
				(void) destroy_spot(scopy[il]);
				}
			nlabs = 0;
			alabs = 0;
			FREEMEM(smove);
			FREEMEM(scopy);

			/* Re-display the boundary */
			present_all();

			/* Reset check for merge mode */
			mlabels = FALSE;
			mlfirst = TRUE;
			}

		/* Change merge mode and add labels */
		else
			{

			/* Match current list of field labels to    */
			/*  drawn boundary and add them to the list */
			bset = find_mf_set(TempMeta, "curve", "c", "", "");
			tset = find_mf_set(TempMeta, "spot",  "d", "sset", "sset");

			/* Loop through all labels in set */
			if (NotNull(bset) && NotNull(tset))
				{
				bound = (CURVE) bset->list[0];
				for (ilx=0; ilx<tset->num; ilx++)
					{
					spot = (SPOT) tset->list[ilx];
					if (!spot) continue;
					if (same(spot->mclass, "legend")) continue;

					/* Check if label is within drawn boundary */
					(void) curve_test_point(bound, spot->anchor, NULL,
											NULL, NULL, &inside, NULL);
					if (inside)
						{

						/* Highlight each field label */
						/*  and add it to the list    */
						nlabs++;
						if (nlabs > alabs)
							{
							alabs = nlabs;
							smove = GETMEM(smove, SPOT, alabs);
							scopy = GETMEM(scopy, SPOT, alabs);
							}
						highlight_item("spot", (ITEM) spot, 2);
						smove[nlabs-1] = spot;
						scopy[nlabs-1] = copy_spot(spot);
						}
					}
				}

			/* Re-display the boundary and field labels */
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

			case Draw:		State = ReFetch;
							put_message("edit-cancel");
							break;

			default:		State = Fetch;
			}
		fetch = TRUE;
		}

	/* Set state for Right button "merge" or "translate" or "rotate" */
	if (same(mode, "merge"))     State = Merge;
	if (same(mode, "translate")) State = Translate;
	if (same(mode, "rotate"))    State = Centre;

	/* Set state for Right button "preset outline" */
	if (same(mode, "preset outline"))
		{

		/* Check for last drawn outline, last moved outline, or named outline */
		if (same(source, FpaEditorLastDrawnBoundary))
			curve = retrieve_drawn_outline();
		else if (same(source, FpaEditorLastMovedBoundary))
			curve = retrieve_moved_outline();
		else
			curve = retrieve_named_outline(source);
		if (IsNull(curve))
			{
			if (same(source, FpaEditorLastDrawnBoundary))
				{
				put_message("edit-no-drawn-outline");
				pr_warning("Editor",
					"[edit_merge_spline] No outline drawn yet!\n");
				}
			else if (same(source, FpaEditorLastMovedBoundary))
				{
				put_message("edit-no-moved-outline");
				pr_warning("Editor",
					"[edit_merge_spline] No outline moved yet!\n");
				}
			else
				{
				put_message("edit-no-preset-outline", source);
				pr_error("Editor",
					"[edit_merge_spline] No preset map outline: \"%s\"\n",
					source);
				}
			(void) sleep(1);
			State = Draw;
			}

		/* Display the outline */
		else
			{

			/* Add the outline to the set */
			define_lspec(&curve->lspec, 255, 0, NULL, False, 2.0, 0.0,
						 (HILITE) 2);
			add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) curve);
			bset = find_mf_set(TempMeta, "curve", "c", "", "");

			/* Replace labels within drawn boundary (if requested) */
			if (NotNull(bset) && mlabels)
				{

				/* Empty field label list */
				if (nlabs > 0)
					{

					/* Un-highlight field labels and remove them from list */
					for (il=0; il<nlabs; il++)
						{
						highlight_item("spot", (ITEM) smove[il], 4);
						(void) destroy_spot(scopy[il]);
						}
					nlabs = 0;
					alabs = 0;
					FREEMEM(smove);
					FREEMEM(scopy);
					}

				/* Match current list of field labels to    */
				/*  drawn boundary and add them to the list */
				tset = find_mf_set(TempMeta, "spot",  "d", "sset", "sset");

				/* Loop through all labels in set */
				if (NotNull(tset))
					{
					bound = (CURVE) bset->list[0];
					for (ilx=0; ilx<tset->num; ilx++)
						{
						spot = (SPOT) tset->list[ilx];
						if (!spot) continue;
						if (same(spot->mclass, "legend")) continue;

						/* Check if label is within drawn boundary */
						(void) curve_test_point(bound, spot->anchor, NULL,
												NULL, NULL, &inside, NULL);
						if (inside)
							{

							/* Highlight each field label */
							/*  and add it to the list    */
							nlabs++;
							if (nlabs > alabs)
								{
								alabs = nlabs;
								smove = GETMEM(smove, SPOT, alabs);
								scopy = GETMEM(scopy, SPOT, alabs);
								}
							highlight_item("spot", (ITEM) spot, 2);
							smove[nlabs-1] = spot;
							scopy[nlabs-1] = copy_spot(spot);
							}
						}
					}
				}

			/* Display the outline and field labels */
			present_temp(TRUE);
			post_partial(TRUE);
			edit_select(NullCal, TRUE);
			State = WaitAction;
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

			/* Wait around for a "merge" or "translate" or "rotate" */
			case WaitAction:

				/* Interpret spread factor */
				spread = define_spread();
				circle_echo(TRUE);

				/* Keep returning until we get a "merge" */
				/*  or "translate" or "rotate"           */
				put_message("spline-merge-action");
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
				pr_diag("Editor",
					"[edit_merge_spline] Fetching merge field\n");
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
				clear_equation_database();
				if (check_retrieve_metasfc(&fdesc))
					{
					msfc = retrieve_surface(&fdesc);
					FREEMEM(msource);
					FREEMEM(msubsrc);
					FREEMEM(mrtime);
					FREEMEM(mvtime);
					FREEMEM(melement);
					FREEMEM(mlevel);
					}
				if (!msfc)
					{
					/* Can't access given source */
					put_message("merge-access");
					(void) sleep(2);
					State = Wait;
					continue;
					}

				/* Got it - contour and prepare it for display */
				msource  = STRMEM(msource,  source);
				msubsrc  = STRMEM(msubsrc,  subsrc);
				mrtime   = STRMEM(mrtime,   rtime);
				mvtime   = STRMEM(mvtime,   vtime);
				melement = STRMEM(melement, element);
				mlevel   = STRMEM(mlevel,   level);

				/* Get the labels for the fetched field */
				(void) set_fld_descript(&fdesc,
					FpaF_FIELD_MACRO,       FpaC_SCATTERED,
					FpaF_END_OF_LIST);
				sset = retrieve_spotset(&fdesc);

				/* Display the fetched field and labels */
				/* >>>>> modify field units for CurrElement/CurrLevel?? <<<<< */
				setup_sfc_presentation(msfc, CurrElement, CurrLevel, "FPA");
				contour_surface(msfc);
				highlight_surface(msfc, 4);
				setup_set_presentation(sset, CurrElement, CurrLevel, "FPA");
				highlight_set(sset, 4);

				add_sfc_to_metafile(TempMeta, "a", "", "", msfc);
				add_set_to_metafile(TempMeta, "d", "sset", "sset", sset);
				present_temp(TRUE);
				post_partial(TRUE);
				clear_message();

				/* Move on to next stage */
				State = Draw;
				continue;

			/* Re-fetch the requested merge field */
			case ReFetch:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_spline] Re-Fetching\n");
#				endif /* DEBUG */

				ignore_partial();

				/* Get rid of boundary */
				if (bset)
					{
					bset  = NULL;
					bound = NULL;
					}
				edit_select(NullCal, FALSE);

				/* Empty field label list */
				if (nlabs > 0)
					{
					for (il=0; il<nlabs; il++) (void) destroy_spot(scopy[il]);
					nlabs = 0;
					alabs = 0;
					FREEMEM(smove);
					FREEMEM(scopy);
					}

				/* Empty display structure */
				empty_temp();

				/* Get rid of fetched field */
				if (savesfc)
					{
					msfc = destroy_surface(msfc);
					sset = destroy_set(sset);
					}
				else
					{
					msfc = NullSfc; /* destroyed by call to */
					sset = NullSet; /*  empty_temp above    */
					}
				FREEMEM(msource);
				FREEMEM(msubsrc);
				FREEMEM(mrtime);
				FREEMEM(mvtime);
				FREEMEM(melement);
				FREEMEM(mlevel);
				savesfc = FALSE;

				/* Wait for a "fetch" */
				State = (fetch)? Fetch: Wait;
				continue;

			/* Draw boundary of un-distorted area */
			case Draw:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_spline] Drawing boundary\n");
#				endif /* DEBUG */

				/* Draw the boundary */
				post_partial(FALSE);
				edit_allow_preset_outline(TRUE);
				put_message("spline-merge-draw");
				if (ready_Xcurve()) butt = LeftButton;
				else if (!ready_Xpoint(DnEdit, NULL, &butt)) return drawn;

				/* Invalid button */
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					ignore_Xpoint();
					continue;
					}

				/* Draw the curve */
				drawing_control(TRUE);
				if (!ready_Xcurve())
					{
					dstart = TRUE;
					if (drawing_Xcurve()) return drawn;
					(void) draw_Xcurve(DnEdit, FilterRes*4, SplineRes*4, TRUE);
					if (!ready_Xcurve()) return drawn;
					}
				dstart = FALSE;
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

				/* Extract and display curve */
				curve = create_curve("", "", "");
				add_line_to_curve(curve, lines[0]);
				post_drawn_outline(curve);
				reset_pipe();
				define_lspec(&curve->lspec, 255, 0, NULL, False, 2.0, 0.0,
							 (HILITE) 2);
				add_item_to_metafile(TempMeta, "curve", "c", "", "",
									 (ITEM) curve);
				bset = find_mf_set(TempMeta, "curve", "c", "", "");

				/* Save labels within drawn boundary (if requested) */
				if (NotNull(bset) && mlabels)
					{

					/* Match current list of field labels to    */
					/*  drawn boundary and add them to the list */
					tset = find_mf_set(TempMeta, "spot",  "d", "sset", "sset");

					/* Loop through all labels in set */
					if (NotNull(tset))
						{
						bound = (CURVE) bset->list[0];
						for (ilx=0; ilx<tset->num; ilx++)
							{
							spot = (SPOT) tset->list[ilx];
							if (!spot) continue;
							if (same(spot->mclass, "legend")) continue;

							/* Check if label is within drawn boundary */
							(void) curve_test_point(bound, spot->anchor, NULL,
													NULL, NULL, &inside, NULL);
							if (inside)
								{

								/* Highlight each field label */
								/*  and add it to the list    */
								nlabs++;
								if (nlabs > alabs)
									{
									alabs = nlabs;
									smove = GETMEM(smove, SPOT, alabs);
									scopy = GETMEM(scopy, SPOT, alabs);
									}
								highlight_item("spot", (ITEM) spot, 2);
								smove[nlabs-1] = spot;
								scopy[nlabs-1] = copy_spot(spot);
								}
							}
						}
					}

				/* Display the outline and field labels */
				present_temp(TRUE);
				post_partial(TRUE);
				edit_select(NullCal, TRUE);

				/* Move on to next stage */
				State = WaitAction;
				continue;

			/* Re-draw boundary */
			case ReDraw:

				/* Get rid of boundary */
				if (bset)
					{
					bset  = NULL;
					bound = NULL;
					}
				edit_select(NullCal, FALSE);

				/* Empty field label list */
				if (nlabs > 0)
					{

					/* Un-highlight each field label */
					/*  and remove it from the list  */
					for (il=0; il<nlabs; il++)
						{
						highlight_item("spot", (ITEM) smove[il], 4);
						(void) destroy_spot(scopy[il]);
						}
					nlabs = 0;
					alabs = 0;
					FREEMEM(smove);
					FREEMEM(scopy);
					}

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_spline] Re-Drawing\n");
#				endif /* DEBUG */

				/* Remove displayed boundary, but keep the fetched field */
				msfc = take_mf_sfc(TempMeta, "a", "", "");
				sset = take_mf_set(TempMeta, "spot", "d", "sset", "sset");

				/* Empty display structure */
				empty_temp();

				/* Re-display the fetched field */
				if (msfc)
					{
					add_sfc_to_metafile(TempMeta, "a", "", "", msfc);
					add_set_to_metafile(TempMeta, "d", "sset", "sset", sset);
					}
				present_all();
				post_partial(TRUE);

				/* Move on to next stage */
				State = Draw;
				continue;

			/* Merge surface in place */
			case Merge:

				/* Check for the boundary */
				bset = find_mf_set(TempMeta, "curve", "c", "", "");
				if (IsNull(bset))
					{
					bound = NullCurve;
					State = Draw;
					continue;
					}
				bound = (CURVE) bset->list[0];
				mstart = TRUE;

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_spline] Merging\n");
#				endif /* DEBUG */

				/* Interpret spread factor */
				spread = define_spread();

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);
				circle_echo(FALSE);

				/* Recalculate grid with merged section */
				put_message("spline-adjust");
				gtol = grid*(1+spread);
				for (ix=0; ix<nx; ix++)
					{
					xm    = ix*grid;
					pm[X] = xm;
					for (iy=0; iy<ny; iy++)
						{
						ym    = iy*grid;
						pm[Y] = ym;
						p[X]  = xm*xform[X][X] + ym*xform[Y][X] + xform[H][X];
						p[Y]  = xm*xform[X][Y] + ym*xform[Y][Y] + xform[H][Y];
						curve_test_point(bound, p, &dist, NULL, NULL,
										 &inside, NULL);

						if (inside)
							{
							wo = 0.0;
							wi = 1.0;
							}
						else if (dist<gtol)
							{
							wo = dist/gtol;
							wi = 1 - wo;
							}
						else
							{
							wo = 1.0;
							wi = 0.0;
							}

						vi = vo = 0.0;
						if (wi > 0)
							{
							pdm[X] = xm;
							pdm[Y] = ym;
							valid = eval_sfc_unmapped(msfc, pdm, &vi);
							}
						if (wo > 0)
							{
							pdm[X] = xm;
							pdm[Y] = ym;
							valid = eval_sfc_unmapped(EditSfc, pdm, &vo);
							}

						sfbuf[iy][ix] = (float) (vi*wi + vo*wo);
						}
					}

				/* Fit surface to new grid values */
				grid_surface(EditSfc, grid, nx, ny, sfbuf);
				contour_surface(EditSfc);

				/* Replace labels within drawn boundary (if requested) */
				if (mlabels)
					{

					/* Remove field labels within drawn boundary */
					if (NotNull(EditLabs))
						{
						for (ilx=EditLabs->num-1; ilx>=0; ilx--)
							{
							spot = (SPOT) EditLabs->list[ilx];
							if (!spot) continue;

							/* Only check for attached field labels?? */
							/* >>>>>
							if (spot->feature == AttachNone) continue;
							<<<<< */

							/* Check if label is within drawn boundary */
							(void) curve_test_point(bound, spot->anchor, NULL,
													NULL, NULL, &inside, NULL);
							if (inside)
								{
								(void) remove_item_from_set(EditLabs,
															(ITEM) spot);
								}
							}
						}

					/* Then add the field labels to surface */
					for (il=0; il<nlabs; il++)
						{
						(void) add_item_to_set(EditLabs, (ITEM) scopy[il]);
						}
					}

				if (EditUndoable) post_mod("surface");

				/* Modify labels and highs and lows accordingly */
				if (recompute_surface_labs(EditSfc, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				put_message("edit-done");

				post_partial(FALSE);
				mstart  = FALSE;
				maction = TRUE;
				busy_cursor(FALSE);

				/* Re-display the surface */
				State = ReDisplay;
				continue;

			/* Pick and translate a reference point */
			case Translate:

				/* Remove any marks from uncompleted actions */
				set = take_mf_set(TempMeta, "mark", "d", "", "");
				if (NotNull(set)) set = destroy_set(set);

				/* Re-display the outline */
				present_all();
				post_partial(TRUE);

				/* Check for the boundary */
				bset   = find_mf_set(TempMeta, "curve", "c", "", "");
				if (IsNull(bset))
					{
					bound = NullCurve;
					State = Draw;
					continue;
					}
				bound  = (CURVE) bset->list[0];
				mstart = TRUE;

				/* Interpret spread factor */
				spread = define_spread();

				/* Pick a reference point */
				put_message("spline-tran");
				if (!ready_Xpoint(DnEdit, p0, &butt))
					{
					circle_echo(TRUE);
					return drawn;
					}
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!inside_dn_window(DnEdit, p0))
					{
					put_message("spline-tran-out1");
					(void) ignore_Xpoint();
					continue;
					}

				/* Try with any point ... skip check for inside boundary!
				curve_test_point(bound, p0, NULL, NULL, NULL, &inside, NULL);
				if (!inside)
					{
					put_message("spline-tran-out2");
					(void) ignore_Xpoint();
					continue;
					}
				*/

				/* Translate the reference point */
				put_message("spline-tran-rel");
				(void) utrack_Xpoint(DnEdit, bset, p0, p1, &butt);
				if (!inside_dn_window(DnEdit, p1))
					{
					put_message("spline-tran-out3");
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

			/* Translate the surface */
			case TranslateDone:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_spline] Translating\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);
				circle_echo(FALSE);

				/* Translate the surface */
				put_message("spline-drag-proc2");
				dx    = p1[X] - p0[X];
				dy    = p1[Y] - p0[Y];
				gtol  = grid*(1+spread);
				dxm   = dx*xform[X][X] + dy*xform[X][Y];
				dym   = dx*xform[Y][X] + dy*xform[Y][Y];

				/* Recalculate grid with merged section */
				put_message("spline-adjust");
				for (ix=0; ix<nx; ix++)
					{
					xm   = ix*grid;
					for (iy=0; iy<ny; iy++)
						{
						ym   = iy*grid;
						p[X] = xm*xform[X][X] + ym*xform[Y][X]
													+ xform[H][X] - dx;
						p[Y] = xm*xform[X][Y] + ym*xform[Y][Y]
													+ xform[H][Y] - dy;
						curve_test_point(bound, p, &dist, NULL, NULL,
										 &inside, NULL);

						/* Determine proximity to boundary */
						if (inside)
							{
							wo = 0.0;
							wi = 1.0;
							}
						else if (dist<gtol)
							{
							wo = dist/gtol;
							wi = 1 - wo;
							}
						else
							{
							wo = 1.0;
							wi = 0.0;
							}

						/* Evaluate grid at translated location */
						vi = vo = 0.0;
						if (wi > 0)
							{
							xdm = xm - dxm;
							ydm = ym - dym;
							xdm    = MAX(xdm, 0.);	xdm = MIN(xdm, xlim);
							ydm    = MAX(ydm, 0.);	ydm = MIN(ydm, ylim);
							pdm[X] = xdm;
							pdm[Y] = ydm;
							valid  = eval_sfc_unmapped(msfc, pdm, &vi);
							}
						if (wo > 0)
							{
							pdm[X] = xm;
							pdm[Y] = ym;
							valid  = eval_sfc_unmapped(EditSfc, pdm, &vo);
							}

						sfbuf[iy][ix] = (float) (vi*wi + vo*wo);
						}
					}

				/* Fit surface to new grid values */
				grid_surface(EditSfc, grid, nx, ny, sfbuf);
				contour_surface(EditSfc);

				/* Save translated boundary */
				xbound = copy_curve(bound);
				translate_curve(xbound, dx, dy);
				post_moved_outline(xbound);

				/* Replace labels within translated boundary (if requested) */
				if (mlabels)
					{

					/* Remove field labels within translated boundary */
					if (NotNull(EditLabs))
						{
						for (ilx=EditLabs->num-1; ilx>=0; ilx--)
							{
							spot = (SPOT) EditLabs->list[ilx];
							if (!spot) continue;

							/* Only check for attached field labels?? */
							/* >>>>>
							if (spot->feature == AttachNone) continue;
							<<<<< */

							/* Check if label is within translated boundary */
							(void) curve_test_point(xbound, spot->anchor, NULL,
													NULL, NULL, &inside, NULL);
							if (inside)
								{
								(void) remove_item_from_set(EditLabs,
															(ITEM) spot);
								}
							}
						}

					/* Then translate field labels and add them to surface */
					(void) spot_list_translate(nlabs, scopy, dx, dy);
					for (il=0; il<nlabs; il++)
						{
						(void) add_item_to_set(EditLabs, (ITEM) scopy[il]);
						}
					}

				/* Remove translated boundary */
				(void) destroy_curve(xbound);

				if (EditUndoable) post_mod("surface");

				/* Modify labels and highs and lows accordingly */
				if (recompute_surface_labs(EditSfc, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				put_message("edit-done");

				post_partial(FALSE);
				mstart  = FALSE;
				maction = TRUE;
				busy_cursor(FALSE);

				/* Re-display the surface */
				State = ReDisplay;
				continue;

			/* Pick the centre of rotation */
			case Centre:

				/* Remove any marks from uncompleted actions */
				set = take_mf_set(TempMeta, "mark", "d", "", "");
				if (NotNull(set)) set = destroy_set(set);

				/* Re-display the outline */
				present_all();
				post_partial(TRUE);

				/* Check for the boundary */
				bset  = find_mf_set(TempMeta, "curve", "c", "", "");
				if (IsNull(bset))
					{
					bound = NullCurve;
					State = Draw;
					continue;
					}
				bound  = (CURVE) bset->list[0];
				mstart = TRUE;

				/* Interpret spread factor */
				spread = define_spread();

				/* Pick the centre of rotation */
				put_message("spline-centre");
				if (!ready_Xpoint(DnEdit, p0, &butt))
					{
					circle_echo(TRUE);
					return drawn;
					}
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!pick_Xpoint(DnEdit, 0, Cpos, &butt)) continue;
				if (!inside_dn_window(DnEdit, Cpos))
					{
					put_message("spline-rot-out");
					(void) sleep(1);
					continue;
					}

				mark0 = create_mark("", "", "", Cpos, 0.0);
				define_mspec(&mark0->mspec, 0, 1, NULL, 0, False, 4.0, 0.0,
							 (HILITE) 3);

				add_item_to_metafile(TempMeta, "mark",  "d", "", "",
									 (ITEM) mark0);
				present_temp(TRUE);

				/* Move on to next stage */
				State = Rotate;
				continue;

			/* Pick and rotate a reference point */
			case Rotate:

				/* Interpret spread factor */
				spread = define_spread();

				/* Pick a reference point */
				put_message("spline-rotate");
				if (!ready_Xpoint(DnEdit, p0, &butt))
					{
					circle_echo(TRUE);
					return drawn;
					}
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!inside_dn_window(DnEdit, p0))
					{
					put_message("spline-rot-out1");
					(void) ignore_Xpoint();
					continue;
					}
				if (point_dist(Cpos, p0) < SplineRes)
					{
					put_message("spline-rot-ref");
					(void) ignore_Xpoint();
					(void) sleep(1);
					continue;
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

				/* Rotate the reference point */
				put_message("spline-rot-release");
				(void) urotate_Xpoint(DnEdit, bset, Cpos, p0, p1, &Ang, &butt);
				if (!inside_dn_window(DnEdit, p1))
					{
					put_message("splin3-rot-out2");
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

				/* Move on to next stage */
				State = RotateDone;
				continue;

			/* Rotate the surface */
			case RotateDone:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_spline] Rotating\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);
				circle_echo(FALSE);

				/* Rotate the surface */
				put_message("spline-drag-proc2");
				dx0   = p0[X] - Cpos[X];
				dy0   = p0[Y] - Cpos[Y];
				angle = -Ang;
				cang  = cos(angle);
				sang  = sin(angle);
				gtol  = grid*(1+spread);

				/* Determine centre of rotation on spline surface */
				cx = Cpos[X]*xform[X][X] + Cpos[Y]*xform[X][Y];
				cy = Cpos[X]*xform[Y][X] + Cpos[Y]*xform[Y][Y];

				/* Recalculate grid with merged section */
				put_message("spline-adjust");
				for (ix=0; ix<nx; ix++)
					{
					xm   = ix*grid;
					for (iy=0; iy<ny; iy++)
						{
						ym   = iy*grid;
						px   = xm*xform[X][X] + ym*xform[Y][X]
													+ xform[H][X] - Cpos[X];
						py   = xm*xform[X][Y] + ym*xform[Y][Y]
													+ xform[H][Y] - Cpos[Y];
						p[X] = px*cang - py*sang + Cpos[X];
						p[Y] = px*sang + py*cang + Cpos[Y];
						curve_test_point(bound, p, &dist, NULL, NULL,
										 &inside, NULL);

						/* Determine proximity to boundary */
						if (inside)
							{
							wo = 0.0;
							wi = 1.0;
							}
						else if (dist<gtol)
							{
							wo = dist/gtol;
							wi = 1 - wo;
							}
						else
							{
							wo = 1.0;
							wi = 0.0;
							}

						/* Evaluate grid at rotated location */
						vi = vo = 0.0;
						if (wi > 0)
							{
							dx  = xm - cx;
							dy  = ym - cy;
							xdm = dx*cang - dy*sang + cx;
							ydm = dx*sang + dy*cang + cy;
							xdm    = MAX(xdm, 0.);	xdm = MIN(xdm, xlim);
							ydm    = MAX(ydm, 0.);	ydm = MIN(ydm, ylim);
							pdm[X] = xdm;
							pdm[Y] = ydm;
							valid  = eval_sfc_unmapped(msfc, pdm, &vi);
							}
						if (wo > 0)
							{
							pdm[X] = xm;
							pdm[Y] = ym;
							valid  = eval_sfc_unmapped(EditSfc, pdm, &vo);
							}
						sfbuf[iy][ix] = (float) (vi*wi + vo*wo);
						}
					}

				/* Fit surface to new grid values */
				grid_surface(EditSfc, grid, nx, ny, sfbuf);
				contour_surface(EditSfc);

				/* Save rotated boundary */
				xbound = copy_curve(bound);
				rotate_curve(xbound, Cpos, Ang/RAD);
				post_moved_outline(xbound);

				/* Replace labels within rotated boundary (if requested) */
				if (mlabels)
					{

					/* Remove field labels within rotated boundary */
					if (NotNull(EditLabs))
						{
						for (ilx=EditLabs->num-1; ilx>=0; ilx--)
							{
							spot = (SPOT) EditLabs->list[ilx];
							if (!spot) continue;

							/* Only check for attached field labels?? */
							/* >>>>>
							if (spot->feature == AttachNone) continue;
							<<<<< */

							/* Check if label is within rotated boundary */
							(void) curve_test_point(xbound, spot->anchor, NULL,
													NULL, NULL, &inside, NULL);
							if (inside)
								{
								(void) remove_item_from_set(EditLabs,
															(ITEM) spot);
								}
							}
						}

					/* Then rotate field labels and add them to surface */
					(void) spot_list_rotate(nlabs, scopy, Cpos, Ang/RAD);
					for (il=0; il<nlabs; il++)
						(void) add_item_to_set(EditLabs, (ITEM) scopy[il]);
					}

				/* Remove rotated boundary */
				(void) destroy_curve(xbound);

				if (EditUndoable) post_mod("surface");

				/* Modify labels and highs and lows accordingly */
				if (recompute_surface_labs(EditSfc, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				put_message("edit-done");

				post_partial(FALSE);
				mstart  = FALSE;
				maction = TRUE;
				busy_cursor(FALSE);

				/* Re-display the surface */
				State = ReDisplay;
				continue;

			/* Re-display the modified surface */
			case ReDisplay:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_spline] ReDisplay\n");
#				endif /* DEBUG */

				/* Remove the fetched field from the display */
				if (!savesfc)
					{
					msfc = take_mf_sfc(TempMeta, "a", "", "");
					sset = take_mf_set(TempMeta, "spot", "d", "sset", "sset");
					savesfc = TRUE;
					}

				/* Empty field label list */
				if (nlabs > 0)
					{
					nlabs = 0;
					alabs = 0;
					FREEMEM(smove);
					FREEMEM(scopy);
					}

				/* Re-display the modified surface and field labels */
				empty_temp();
				present_all();

				/* Move on to next stage */
				State = ReFetch;
				continue;
			}
		}
	}

/**********************************************************************/

LOGICAL	edit_merge_spline_2D

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
	CURVE	curve, xbound;
	SPOT	spot;
	SET		set, tset;
	POINT	p, p0, pz, p1, pm, pdm;
	MARK	mark0, mark1;
	CURVE	curv0, curv1;
	int		nlines;
	LINE	*lines;
	LOGICAL	valid, inside;
	int		ix, iy, il, ilx;
	float	dx, dy, dx0, dy0, angle, cang, sang;
	float	xm, ym, xdm, ydm, dxm, dym, cx, cy, px, py;
	float	spread, dist;
	double	ui, uo, vi, vo, wi, wo;
	int		butt;
	LOGICAL	drawn = FALSE;

	STRING			dpath;
	FLD_DESCRIPT	fdesc;

	/* Current state: */
	/* - where should we resume? */
	static	enum	{ Wait, Fetch, ReFetch, Draw, ReDraw,
						WaitAction, Merge, Translate, TranslateDone,
						Centre, Rotate, RotateDone, ReDisplay } State = Wait;

	/* Is a fetch being done? */
	static	LOGICAL		fetch   = FALSE;

	/* Was a draw started but not completed? */
	static	LOGICAL		dstart  = FALSE;

	/* Has a merge or translate been started or completed? */
	static	LOGICAL		mstart  = FALSE;
	static	LOGICAL		maction = FALSE;

	/* Should labels be merged too? */
	static	LOGICAL		mlabels = FALSE;
	static	LOGICAL		mlfirst = TRUE;

	/* Boundary curve set: */
	/* - defines undistorted region */
	/* - used in cursor tracking */
	static	SET		bset    = NULL;
	static	CURVE	bound   = NULL;

	/* Move list: */
	/* - list of field labels to be moved */
	/* - smove are pointers into merge field set */
	/* - scopy are copies to be added to EditLabs */
	static	int		nlabs   = 0;
	static	int		alabs   = 0;
	static	SPOT	*smove  = NullSpotList;
	static	SPOT	*scopy  = NullSpotList;

	/* Field to merge from */
	static	STRING	msource  = NullString;
	static	STRING	msubsrc  = NullString;
	static	STRING	mrtime   = NullString;
	static	STRING	mvtime   = NullString;
	static	STRING	melement = NullString;
	static	STRING	mlevel   = NullString;
	static	SURFACE	msfc     = NullSfc;
	static	SET		sset     = NullSet;
	static	LOGICAL	savesfc  = FALSE;

	/* Centre of rotation */
	static	POINT		Cpos = ZERO_POINT;
	static	float		Ang  = 0.0;

	/* Temporary grid buffer: */
	/* - results of re-interpolation */
	/* - used to refit the new surface */
	static	float	grid    = 0.0;
	static	float	gtol    = 0.0;
	static	int		m       = 0;
	static	int		n       = 0;
	static	int		nx      = 0;
	static	int		ny      = 0;
	static	int		mx      = 0;
	static	int		my      = 0;
	static	float	xlim    = 0.0;
	static	float	ylim    = 0.0;
	static	float	**sxbuf = NULL;
	static	float	**sybuf = NULL;
	static	float	*sxdat  = NULL;
	static	float	*sydat  = NULL;
	static	XFORM	xform   = IDENT_XFORM;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_merge_spline_2D] %s  State: %d\n", mode, State);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same(mode, "cancel"))
		{
		put_message("edit-cancel");
		circle_echo(FALSE);
		drawing_control(FALSE);
		switch (State)
			{
			case Translate:
			case Centre:
			case Rotate:
			case WaitAction:
							ignore_partial();
							edit_allow_preset_outline(FALSE);
							State = (maction)? ReDisplay: ReDraw;
							break;

			case Draw:		edit_allow_preset_outline(FALSE);
							State  = (dstart)? ReDraw: ReFetch;
							dstart = FALSE;
							grid   = 0.0;
							break;

			default:		ignore_partial();
							edit_allow_preset_outline(FALSE);

							/* Get rid of boundary */
							if (bset)
								{
								bset  = NULL;
								bound = NULL;
								}
							edit_select(NullCal, FALSE);

							/* Empty field label list */
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
								}

							/* Empty display structure */
							empty_temp();

							/* Get rid of fetched field */
							if (savesfc)
								{
								msfc = destroy_surface(msfc);
								sset = destroy_set(sset);
								}
							else
								{
								msfc = NullSfc; /* destroyed by call to */
								sset = NullSet; /*  empty_temp above    */
								}
							FREEMEM(msource);
							FREEMEM(msubsrc);
							FREEMEM(mrtime);
							FREEMEM(mvtime);
							FREEMEM(melement);
							FREEMEM(mlevel);
							savesfc = FALSE;
							return FALSE;
			}
		}
	if (same(mode, "cancel all"))
		{
		ignore_partial();
		drawing_control(FALSE);
		edit_allow_preset_outline(FALSE);

		/* Get rid of boundary */
		if (bset)
			{
			bset  = NULL;
			bound = NULL;
			}
		edit_select(NullCal, FALSE);

		/* Empty field label list */
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
			}

		/* Empty display structure */
		empty_temp();

		/* Get rid of fetched field */
		if (savesfc)
			{
			msfc = destroy_surface(msfc);
			sset = destroy_set(sset);
			}
		else
			{
			msfc = NullSfc; /* destroyed by call to */
			sset = NullSet; /*  empty_temp above    */
			}
		FREEMEM(msource);
		FREEMEM(msubsrc);
		FREEMEM(mrtime);
		FREEMEM(mvtime);
		FREEMEM(melement);
		FREEMEM(mlevel);
		savesfc = FALSE;
		return FALSE;
		}

	if (same(mode, "clear"))
		{
		return FALSE;
		}

	/* See if surface exists */
	if (!EditSfc) return FALSE;

	/* Construct temporary fields on startup */
	if (same(mode, "begin") || grid <= 0.0)
		{
		ignore_partial();

		/* Set up temporary grid buffers */
		m      = EditSfc->sp.m;
		n      = EditSfc->sp.n;
		grid   = EditSfc->sp.gridlen;
		gtol   = grid*1.5;
		nx     = m - 2;
		ny     = n - 2;
		xlim   = grid*(nx-1);
		ylim   = grid*(ny-1);
		if ((nx != mx) || (ny != my))
			{
			mx    = nx;
			my    = ny;
			sxdat = GETMEM(sxdat, float, (nx*ny));
			sydat = GETMEM(sydat, float, (nx*ny));
			sxbuf = GETMEM(sxbuf, float *, ny);
			sybuf = GETMEM(sybuf, float *, ny);
			for (iy=0; iy<ny; iy++)
				{
				sxbuf[iy] = sxdat + iy*nx;
				sybuf[iy] = sydat + iy*nx;
				}
			}

		copy_xform(xform, EditSfc->sp.xform);

		if (same(mode, "begin"))
			{

			/* Set up move list */
			nlabs = 0;
			alabs = 0;
			FREEMEM(smove);
			FREEMEM(scopy);

			empty_temp();
			dstart  = FALSE;
			mstart  = FALSE;
			maction = FALSE;
			mlfirst = TRUE;
			savesfc = FALSE;
			edit_select(NullCal, FALSE);
			State = Wait;
			}
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
				put_message("spline-merge-nomode");
				(void) sleep(1);
				}
			mlfirst = FALSE;
			}

		/* Change merge mode and remove labels */
		else if (mlabels)
			{

			/* Un-highlight each field label and remove it from the list */
			for (il=0; il<nlabs; il++)
				{
				highlight_item("spot", (ITEM) smove[il], 4);
				(void) destroy_spot(scopy[il]);
				}
			nlabs = 0;
			alabs = 0;
			FREEMEM(smove);
			FREEMEM(scopy);

			/* Re-display the boundary */
			present_all();

			/* Reset check for merge mode */
			mlabels = FALSE;
			mlfirst = TRUE;
			}

		/* Change merge mode and add labels */
		else
			{

			/* Match current list of field labels to    */
			/*  drawn boundary and add them to the list */
			bset = find_mf_set(TempMeta, "curve", "c", "", "");
			tset = find_mf_set(TempMeta, "spot",  "d", "sset", "sset");

			/* Loop through all labels in set */
			if (NotNull(bset) && NotNull(tset))
				{
				bound = (CURVE) bset->list[0];
				for (ilx=0; ilx<tset->num; ilx++)
					{
					spot = (SPOT) tset->list[ilx];
					if (!spot) continue;
					if (same(spot->mclass, "legend")) continue;

					/* Check if label is within drawn boundary */
					(void) curve_test_point(bound, spot->anchor, NULL,
											NULL, NULL, &inside, NULL);
					if (inside)
						{

						/* Highlight each field label */
						/*  and add it to the list    */
						nlabs++;
						if (nlabs > alabs)
							{
							alabs = nlabs;
							smove = GETMEM(smove, SPOT, alabs);
							scopy = GETMEM(scopy, SPOT, alabs);
							}
						highlight_item("spot", (ITEM) spot, 2);
						smove[nlabs-1] = spot;
						scopy[nlabs-1] = copy_spot(spot);
						}
					}
				}

			/* Re-display the boundary and field labels */
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

			case Draw:		State = ReFetch;
							put_message("edit-cancel");
							break;

			default:		State = Fetch;
			}
		fetch = TRUE;
		}

	/* Set state for Right button "merge" or "translate" or "rotate" */
	if (same(mode, "merge"))     State = Merge;
	if (same(mode, "translate")) State = Translate;
	if (same(mode, "rotate"))    State = Centre;

	/* Set state for Right button "preset outline" */
	if (same(mode, "preset outline"))
		{

		/* Check for last drawn outline, last moved outline, or named outline */
		if (same(source, FpaEditorLastDrawnBoundary))
			curve = retrieve_drawn_outline();
		else if (same(source, FpaEditorLastMovedBoundary))
			curve = retrieve_moved_outline();
		else
			curve = retrieve_named_outline(source);
		if (IsNull(curve))
			{
			if (same(source, FpaEditorLastDrawnBoundary))
				{
				put_message("edit-no-drawn-outline");
				pr_warning("Editor",
					"[edit_merge_spline_2D] No outline drawn yet!\n");
				}
			else if (same(source, FpaEditorLastMovedBoundary))
				{
				put_message("edit-no-moved-outline");
				pr_warning("Editor",
					"[edit_merge_spline_2D] No outline moved yet!\n");
				}
			else
				{
				put_message("edit-no-preset-outline", source);
				pr_error("Editor",
					"[edit_merge_spline_2D] No preset map outline: \"%s\"\n",
					source);
				}
			(void) sleep(1);
			State = Draw;
			}

		/* Display the outline */
		else
			{

			/* Add the outline to the set */
			define_lspec(&curve->lspec, 255, 0, NULL, False, 2.0, 0.0,
						 (HILITE) 2);
			add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) curve);
			bset = find_mf_set(TempMeta, "curve", "c", "", "");

			/* Replace labels within drawn boundary (if requested) */
			if (NotNull(bset) && mlabels)
				{

				/* Empty field label list */
				if (nlabs > 0)
					{

					/* Un-highlight field labels and remove them from list */
					for (il=0; il<nlabs; il++)
						{
						highlight_item("spot", (ITEM) smove[il], 4);
						(void) destroy_spot(scopy[il]);
						}
					nlabs = 0;
					alabs = 0;
					FREEMEM(smove);
					FREEMEM(scopy);
					}

				/* Match current list of field labels to    */
				/*  drawn boundary and add them to the list */
				tset = find_mf_set(TempMeta, "spot",  "d", "sset", "sset");

				/* Loop through all labels in set */
				if (NotNull(tset))
					{
					bound = (CURVE) bset->list[0];
					for (ilx=0; ilx<tset->num; ilx++)
						{
						spot = (SPOT) tset->list[ilx];
						if (!spot) continue;
						if (same(spot->mclass, "legend")) continue;

						/* Check if label is within drawn boundary */
						(void) curve_test_point(bound, spot->anchor, NULL,
												NULL, NULL, &inside, NULL);
						if (inside)
							{

							/* Highlight each field label */
							/*  and add it to the list    */
							nlabs++;
							if (nlabs > alabs)
								{
								alabs = nlabs;
								smove = GETMEM(smove, SPOT, alabs);
								scopy = GETMEM(scopy, SPOT, alabs);
								}
							highlight_item("spot", (ITEM) spot, 2);
							smove[nlabs-1] = spot;
							scopy[nlabs-1] = copy_spot(spot);
							}
						}
					}
				}

			/* Display the outline and field labels */
			present_temp(TRUE);
			post_partial(TRUE);
			edit_select(NullCal, TRUE);
			State = WaitAction;
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

			/* Wait around for a "merge" or "translate" or "rotate" */
			case WaitAction:

				/* Interpret spread factor */
				spread = define_spread();
				circle_echo(TRUE);

				/* Keep returning until we get a "merge" */
				/*  or "translate" or "rotate"           */
				put_message("spline-merge-action");
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
				pr_diag("Editor",
					"[edit_merge_spline_2D] Fetching merge field\n");
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
				clear_equation_database();
				if (check_retrieve_metasfc(&fdesc))
					{
					msfc = retrieve_surface(&fdesc);
					FREEMEM(msource);
					FREEMEM(msubsrc);
					FREEMEM(mrtime);
					FREEMEM(mvtime);
					FREEMEM(melement);
					FREEMEM(mlevel);
					}
				if (!msfc)
					{
					/* Can't access given source */
					put_message("merge-access");
					(void) sleep(2);
					State = Wait;
					continue;
					}

				/* Got it - contour and prepare it for display */
				msource  = STRMEM(msource,  source);
				msubsrc  = STRMEM(msubsrc,  subsrc);
				mrtime   = STRMEM(mrtime,   rtime);
				mvtime   = STRMEM(mvtime,   vtime);
				melement = STRMEM(melement, element);
				mlevel   = STRMEM(mlevel,   level);

				/* Get the labels for the fetched field */
				(void) set_fld_descript(&fdesc,
					FpaF_FIELD_MACRO,       FpaC_SCATTERED,
					FpaF_END_OF_LIST);
				sset = retrieve_spotset(&fdesc);

				/* Display the fetched field and labels */
				/* >>>>> modify field units for CurrElement/CurrLevel?? <<<<< */
				setup_sfc_presentation(msfc, CurrElement, CurrLevel, "FPA");
				contour_surface(msfc);
				highlight_surface(msfc, 4);
				setup_set_presentation(sset, CurrElement, CurrLevel, "FPA");
				highlight_set(sset, 4);

				add_sfc_to_metafile(TempMeta, "v", "", "", msfc);
				add_set_to_metafile(TempMeta, "d", "sset", "sset", sset);
				present_temp(TRUE);
				post_partial(TRUE);
				clear_message();

				/* Move on to next stage */
				State = Draw;
				continue;

			/* Re-fetch the requested merge field */
			case ReFetch:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_spline_2D] Re-Fetching\n");
#				endif /* DEBUG */

				ignore_partial();

				/* Get rid of boundary */
				if (bset)
					{
					bset  = NULL;
					bound = NULL;
					}
				edit_select(NullCal, FALSE);

				/* Empty field label list */
				if (nlabs > 0)
					{
					for (il=0; il<nlabs; il++) (void) destroy_spot(scopy[il]);
					nlabs = 0;
					alabs = 0;
					FREEMEM(smove);
					FREEMEM(scopy);
					}

				/* Empty display structure */
				empty_temp();

				/* Get rid of fetched field */
				if (savesfc)
					{
					msfc = destroy_surface(msfc);
					sset = destroy_set(sset);
					}
				else
					{
					msfc = NullSfc; /* destroyed by call to */
					sset = NullSet; /*  empty_temp above    */
					}
				FREEMEM(msource);
				FREEMEM(msubsrc);
				FREEMEM(mrtime);
				FREEMEM(mvtime);
				FREEMEM(melement);
				FREEMEM(mlevel);
				savesfc = FALSE;

				/* Wait for a "fetch" */
				State = (fetch)? Fetch: Wait;
				continue;

			/* Draw boundary of un-distorted area */
			case Draw:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_spline_2D] Drawing boundary\n");
#				endif /* DEBUG */

				/* Draw the boundary */
				post_partial(FALSE);
				edit_allow_preset_outline(TRUE);
				put_message("spline-merge-draw");
				if (ready_Xcurve()) butt = LeftButton;
				else if (!ready_Xpoint(DnEdit, NULL, &butt)) return drawn;

				/* Invalid button */
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					ignore_Xpoint();
					continue;
					}

				/* Draw the curve */
				drawing_control(TRUE);
				if (!ready_Xcurve())
					{
					dstart = TRUE;
					if (drawing_Xcurve()) return drawn;
					(void) draw_Xcurve(DnEdit, FilterRes*4, SplineRes*4, TRUE);
					if (!ready_Xcurve()) return drawn;
					}
				dstart = FALSE;
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

				/* Extract and display curve */
				curve = create_curve("", "", "");
				add_line_to_curve(curve, lines[0]);
				post_drawn_outline(curve);
				reset_pipe();
				define_lspec(&curve->lspec, 255, 0, NULL, False, 2.0, 0.0,
							 (HILITE) 2);
				add_item_to_metafile(TempMeta, "curve", "c", "", "",
									 (ITEM) curve);
				bset = find_mf_set(TempMeta, "curve", "c", "", "");

				/* Save labels within drawn boundary (if requested) */
				if (NotNull(bset) && mlabels)
					{

					/* Match current list of field labels to    */
					/*  drawn boundary and add them to the list */
					tset = find_mf_set(TempMeta, "spot",  "d", "sset", "sset");

					/* Loop through all labels in set */
					if (NotNull(tset))
						{
						bound = (CURVE) bset->list[0];
						for (ilx=0; ilx<tset->num; ilx++)
							{
							spot = (SPOT) tset->list[ilx];
							if (!spot) continue;
							if (same(spot->mclass, "legend")) continue;

							/* Check if label is within drawn boundary */
							(void) curve_test_point(bound, spot->anchor, NULL,
													NULL, NULL, &inside, NULL);
							if (inside)
								{

								/* Highlight each field label */
								/*  and add it to the list    */
								nlabs++;
								if (nlabs > alabs)
									{
									alabs = nlabs;
									smove = GETMEM(smove, SPOT, alabs);
									scopy = GETMEM(scopy, SPOT, alabs);
									}
								highlight_item("spot", (ITEM) spot, 2);
								smove[nlabs-1] = spot;
								scopy[nlabs-1] = copy_spot(spot);
								}
							}
						}
					}

				/* Display the outline and field labels */
				present_temp(TRUE);
				post_partial(TRUE);
				edit_select(NullCal, TRUE);

				/* Move on to next stage */
				State = WaitAction;
				continue;

			/* Re-draw boundary */
			case ReDraw:

				/* Get rid of boundary */
				if (bset)
					{
					bset  = NULL;
					bound = NULL;
					}
				edit_select(NullCal, FALSE);

				/* Empty field label list */
				if (nlabs > 0)
					{

					/* Un-highlight each field label */
					/*  and remove it from the list  */
					for (il=0; il<nlabs; il++)
						{
						highlight_item("spot", (ITEM) smove[il], 4);
						(void) destroy_spot(scopy[il]);
						}
					nlabs = 0;
					alabs = 0;
					FREEMEM(smove);
					FREEMEM(scopy);
					}

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_spline_2D] Re-Drawing\n");
#				endif /* DEBUG */

				/* Remove displayed boundary, but keep the fetched field */
				msfc = take_mf_sfc(TempMeta, "a", "", "");
				sset = take_mf_set(TempMeta, "spot", "d", "sset", "sset");

				/* Empty display structure */
				empty_temp();

				/* Re-display the fetched field */
				if (msfc)
					{
					add_sfc_to_metafile(TempMeta, "v", "", "", msfc);
					add_set_to_metafile(TempMeta, "d", "sset", "sset", sset);
					}
				present_all();
				post_partial(TRUE);

				/* Move on to next stage */
				State = Draw;
				continue;

			/* Merge surface in place */
			case Merge:

				/* Check for the boundary */
				bset = find_mf_set(TempMeta, "curve", "c", "", "");
				if (IsNull(bset))
					{
					bound = NullCurve;
					State = Draw;
					continue;
					}
				bound = (CURVE) bset->list[0];
				mstart = TRUE;

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_spline_2D] Merging\n");
#				endif /* DEBUG */

				/* Interpret spread factor */
				spread = define_spread();

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);
				circle_echo(FALSE);

				/* Recalculate grid with merged section */
				put_message("spline-adjust");
				gtol = grid*(1+spread);
				for (ix=0; ix<nx; ix++)
					{
					xm    = ix*grid;
					pm[X] = xm;
					for (iy=0; iy<ny; iy++)
						{
						ym    = iy*grid;
						pm[Y] = ym;
						p[X]  = xm*xform[X][X] + ym*xform[Y][X] + xform[H][X];
						p[Y]  = xm*xform[X][Y] + ym*xform[Y][Y] + xform[H][Y];
						curve_test_point(bound, p, &dist, NULL, NULL,
										 &inside, NULL);

						/* Determine proximity to boundary */
						if (inside)
							{
							wo = 0.0;
							wi = 1.0;
							}
						else if (dist<gtol)
							{
							wo = dist/gtol;
							wi = 1 - wo;
							}
						else
							{
							wo = 1.0;
							wi = 0.0;
							}

						/* Evaluate grid at merged location */
						vi = vo = 0.0;
						if (wi > 0)
							{
							pdm[X] = xm;
							pdm[Y] = ym;
							valid = eval_sfc_UV_unmapped(msfc, pdm, &ui, &vi);
							}
						if (wo > 0)
							{
							pdm[X] = xm;
							pdm[Y] = ym;
							valid = eval_sfc_UV_unmapped(EditSfc, pdm, &uo, &vo);
							}

						sxbuf[iy][ix] = (float) (ui*wi + uo*wo);
						sybuf[iy][ix] = (float) (vi*wi + vo*wo);
						}
					}

				/* Fit surface to new grid values */
				grid_surface_2D(EditSfc, grid, nx, ny, sxbuf, sybuf);
				contour_surface(EditSfc);

				/* Replace labels within drawn boundary (if requested) */
				if (mlabels)
					{

					/* Remove field labels within drawn boundary */
					if (NotNull(EditLabs))
						{
						for (ilx=EditLabs->num-1; ilx>=0; ilx--)
							{
							spot = (SPOT) EditLabs->list[ilx];
							if (!spot) continue;

							/* Only check for attached field labels?? */
							/* >>>>>
							if (spot->feature == AttachNone) continue;
							<<<<< */

							/* Check if label is within drawn boundary */
							(void) curve_test_point(bound, spot->anchor, NULL,
													NULL, NULL, &inside, NULL);
							if (inside)
								{
								(void) remove_item_from_set(EditLabs,
															(ITEM) spot);
								}
							}
						}

					/* Then add the field labels to surface */
					for (il=0; il<nlabs; il++)
						{
						(void) add_item_to_set(EditLabs, (ITEM) scopy[il]);
						}
					}

				if (EditUndoable) post_mod("surface");

				/* Modify labels and highs and lows accordingly */
				if (recompute_surface_labs(EditSfc, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				put_message("edit-done");

				post_partial(FALSE);
				mstart  = FALSE;
				maction = TRUE;
				busy_cursor(FALSE);

				/* Re-display the surface */
				State = ReDisplay;
				continue;

			/* Pick and translate a reference point */
			case Translate:

				/* Remove any marks from uncompleted actions */
				set = take_mf_set(TempMeta, "mark", "d", "", "");
				if (NotNull(set)) set = destroy_set(set);

				/* Re-display the outline */
				present_all();
				post_partial(TRUE);

				/* Check for the boundary */
				bset   = find_mf_set(TempMeta, "curve", "c", "", "");
				if (IsNull(bset))
					{
					bound = NullCurve;
					State = Draw;
					continue;
					}
				bound  = (CURVE) bset->list[0];
				mstart = TRUE;

				/* Interpret spread factor */
				spread = define_spread();

				/* Pick a reference point */
				put_message("spline-tran");
				if (!ready_Xpoint(DnEdit, p0, &butt))
					{
					circle_echo(TRUE);
					return drawn;
					}
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!inside_dn_window(DnEdit, p0))
					{
					put_message("spline-tran-out1");
					(void) ignore_Xpoint();
					continue;
					}

				/* Try with any point ... skip check for inside boundary!
				curve_test_point(bound, p0, NULL, NULL, NULL, &inside, NULL);
				if (!inside)
					{
					put_message("spline-tran-out2");
					(void) ignore_Xpoint();
					continue;
					}
				*/

				/* Translate the reference point */
				put_message("spline-tran-rel");
				(void) utrack_Xpoint(DnEdit, bset, p0, p1, &butt);
				if (!inside_dn_window(DnEdit, p1))
					{
					put_message("spline-tran-out3");
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

			/* Translate the surface */
			case TranslateDone:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_spline_2D] Translating\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);
				circle_echo(FALSE);

				/* Translate the surface */
				put_message("spline-drag-proc2");
				dx    = p1[X] - p0[X];
				dy    = p1[Y] - p0[Y];
				gtol  = grid*(1+spread);
				dxm   = dx*xform[X][X] + dy*xform[X][Y];
				dym   = dx*xform[Y][X] + dy*xform[Y][Y];

				/* Recalculate grid with merged section */
				put_message("spline-adjust");
				for (ix=0; ix<nx; ix++)
					{
					xm   = ix*grid;
					for (iy=0; iy<ny; iy++)
						{
						ym   = iy*grid;
						p[X] = xm*xform[X][X] + ym*xform[Y][X]
													+ xform[H][X] - dx;
						p[Y] = xm*xform[X][Y] + ym*xform[Y][Y]
													+ xform[H][Y] - dy;
						curve_test_point(bound, p, &dist, NULL, NULL,
										 &inside, NULL);

						/* Determine proximity to boundary */
						if (inside)
							{
							wo = 0.0;
							wi = 1.0;
							}
						else if (dist<gtol)
							{
							wo = dist/gtol;
							wi = 1 - wo;
							}
						else
							{
							wo = 1.0;
							wi = 0.0;
							}

						/* Evaluate grid at translated location */
						vi = vo = 0.0;
						if (wi > 0)
							{
							xdm = xm - dxm;
							ydm = ym - dym;
							xdm    = MAX(xdm, 0.);	xdm = MIN(xdm, xlim);
							ydm    = MAX(ydm, 0.);	ydm = MIN(ydm, ylim);
							pdm[X] = xdm;
							pdm[Y] = ydm;
							valid  = eval_sfc_UV_unmapped(msfc, pdm, &ui, &vi);
							}
						if (wo > 0)
							{
							pdm[X] = xm;
							pdm[Y] = ym;
							valid  = eval_sfc_UV_unmapped(EditSfc, pdm, &uo, &vo);
							}

						sxbuf[iy][ix] = (float) (ui*wi + uo*wo);
						sybuf[iy][ix] = (float) (vi*wi + vo*wo);
						}
					}

				/* Fit surface to new grid values */
				grid_surface_2D(EditSfc, grid, nx, ny, sxbuf, sybuf);
				contour_surface(EditSfc);

				/* Save translated boundary */
				xbound = copy_curve(bound);
				translate_curve(xbound, dx, dy);
				post_moved_outline(xbound);

				/* Replace labels within translated boundary (if requested) */
				if (mlabels)
					{

					/* Remove field labels within translated boundary */
					if (NotNull(EditLabs))
						{
						for (ilx=EditLabs->num-1; ilx>=0; ilx--)
							{
							spot = (SPOT) EditLabs->list[ilx];
							if (!spot) continue;

							/* Only check for attached field labels?? */
							/* >>>>>
							if (spot->feature == AttachNone) continue;
							<<<<< */

							/* Check if label is within translated boundary */
							(void) curve_test_point(xbound, spot->anchor, NULL,
													NULL, NULL, &inside, NULL);
							if (inside)
								{
								(void) remove_item_from_set(EditLabs,
															(ITEM) spot);
								}
							}
						}

					/* Then translate field labels and add them to surface */
					(void) spot_list_translate(nlabs, scopy, dx, dy);
					for (il=0; il<nlabs; il++)
						{
						(void) add_item_to_set(EditLabs, (ITEM) scopy[il]);
						}
					}

				/* Remove translated boundary */
				(void) destroy_curve(xbound);

				if (EditUndoable) post_mod("surface");

				/* Modify labels and highs and lows accordingly */
				if (recompute_surface_labs(EditSfc, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				put_message("edit-done");

				post_partial(FALSE);
				mstart  = FALSE;
				maction = TRUE;
				busy_cursor(FALSE);

				/* Re-display the surface */
				State = ReDisplay;
				continue;

			/* Pick the centre of rotation */
			case Centre:

				/* Remove any marks from uncompleted actions */
				set = take_mf_set(TempMeta, "mark", "d", "", "");
				if (NotNull(set)) set = destroy_set(set);

				/* Re-display the outline */
				present_all();
				post_partial(TRUE);

				/* Check for the boundary */
				bset  = find_mf_set(TempMeta, "curve", "c", "", "");
				if (IsNull(bset))
					{
					bound = NullCurve;
					State = Draw;
					continue;
					}
				bound  = (CURVE) bset->list[0];
				mstart = TRUE;

				/* Interpret spread factor */
				spread = define_spread();

				/* Pick the centre of rotation */
				put_message("spline-centre");
				if (!ready_Xpoint(DnEdit, p0, &butt))
					{
					circle_echo(TRUE);
					return drawn;
					}
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!pick_Xpoint(DnEdit, 0, Cpos, &butt)) continue;
				if (!inside_dn_window(DnEdit, Cpos))
					{
					put_message("spline-rot-out");
					(void) sleep(1);
					continue;
					}

				mark0 = create_mark("", "", "", Cpos, 0.0);
				define_mspec(&mark0->mspec, 0, 1, NULL, 0, False, 4.0, 0.0,
							 (HILITE) 3);

				add_item_to_metafile(TempMeta, "mark",  "d", "", "",
									 (ITEM) mark0);
				present_temp(TRUE);

				/* Move on to next stage */
				State = Rotate;
				continue;

			/* Pick and rotate a reference point */
			case Rotate:

				/* Interpret spread factor */
				spread = define_spread();

				/* Pick a reference point */
				put_message("spline-rotate");
				if (!ready_Xpoint(DnEdit, p0, &butt))
					{
					circle_echo(TRUE);
					return drawn;
					}
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					(void) ignore_Xpoint();
					continue;
					}
				if (!inside_dn_window(DnEdit, p0))
					{
					put_message("spline-rot-out1");
					(void) ignore_Xpoint();
					continue;
					}
				if (point_dist(Cpos, p0) < SplineRes)
					{
					put_message("spline-rot-ref");
					(void) ignore_Xpoint();
					(void) sleep(1);
					continue;
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

				/* Rotate the reference point */
				put_message("spline-rot-release");
				(void) urotate_Xpoint(DnEdit, bset, Cpos, p0, p1, &Ang, &butt);
				if (!inside_dn_window(DnEdit, p1))
					{
					put_message("splin3-rot-out2");
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

				/* Move on to next stage */
				State = RotateDone;
				continue;

			/* Rotate the surface */
			case RotateDone:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_spline_2D] Rotating\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);
				circle_echo(FALSE);

				/* Rotate the surface */
				put_message("spline-drag-proc2");
				dx0   = p0[X] - Cpos[X];
				dy0   = p0[Y] - Cpos[Y];
				angle = -Ang;
				cang  = cos(angle);
				sang  = sin(angle);
				gtol  = grid*(1+spread);

				/* Determine centre of rotation on spline surface */
				cx = Cpos[X]*xform[X][X] + Cpos[Y]*xform[X][Y];
				cy = Cpos[X]*xform[Y][X] + Cpos[Y]*xform[Y][Y];

				/* Recalculate grid with merged section */
				put_message("spline-adjust");
				for (ix=0; ix<nx; ix++)
					{
					xm   = ix*grid;
					for (iy=0; iy<ny; iy++)
						{
						ym   = iy*grid;
						px   = xm*xform[X][X] + ym*xform[Y][X]
													+ xform[H][X] - Cpos[X];
						py   = xm*xform[X][Y] + ym*xform[Y][Y]
													+ xform[H][Y] - Cpos[Y];
						p[X] = px*cang - py*sang + Cpos[X];
						p[Y] = px*sang + py*cang + Cpos[Y];
						curve_test_point(bound, p, &dist, NULL, NULL,
										 &inside, NULL);

						/* Determine proximity to boundary */
						if (inside)
							{
							wo = 0.0;
							wi = 1.0;
							}
						else if (dist<gtol)
							{
							wo = dist/gtol;
							wi = 1 - wo;
							}
						else
							{
							wo = 1.0;
							wi = 0.0;
							}

						/* Evaluate grid at rotated location */
						ui = uo = vi = vo = 0.0;
						if (wi > 0)
							{
							dx  = xm - cx;
							dy  = ym - cy;
							xdm = dx*cang - dy*sang + cx;
							ydm = dx*sang + dy*cang + cy;
							xdm    = MAX(xdm, 0.);	xdm = MIN(xdm, xlim);
							ydm    = MAX(ydm, 0.);	ydm = MIN(ydm, ylim);
							pdm[X] = xdm;
							pdm[Y] = ydm;
							valid  = eval_sfc_UV_unmapped(msfc, pdm,
														  &ui, &vi);
							}
						if (wo > 0)
							{
							pdm[X] = xm;
							pdm[Y] = ym;
							valid  = eval_sfc_UV_unmapped(EditSfc, pdm,
														  &uo, &vo);
							}
						sxbuf[iy][ix] = (float) (ui*wi + uo*wo);
						sybuf[iy][ix] = (float) (vi*wi + vo*wo);
						}
					}

				/* Fit surface to new grid values */
				grid_surface_2D(EditSfc, grid, nx, ny, sxbuf, sybuf);
				contour_surface(EditSfc);

				/* Save rotated boundary */
				xbound = copy_curve(bound);
				rotate_curve(xbound, Cpos, Ang/RAD);
				post_moved_outline(xbound);

				/* Replace labels within rotated boundary (if requested) */
				if (mlabels)
					{

					/* Remove field labels within rotated boundary */
					if (NotNull(EditLabs))
						{
						for (ilx=EditLabs->num-1; ilx>=0; ilx--)
							{
							spot = (SPOT) EditLabs->list[ilx];
							if (!spot) continue;

							/* Only check for attached field labels?? */
							/* >>>>>
							if (spot->feature == AttachNone) continue;
							<<<<< */

							/* Check if label is within rotated boundary */
							(void) curve_test_point(xbound, spot->anchor, NULL,
													NULL, NULL, &inside, NULL);
							if (inside)
								{
								(void) remove_item_from_set(EditLabs,
															(ITEM) spot);
								}
							}
						}

					/* Then rotate field labels and add them to surface */
					(void) spot_list_rotate(nlabs, scopy, Cpos, Ang/RAD);
					for (il=0; il<nlabs; il++)
						(void) add_item_to_set(EditLabs, (ITEM) scopy[il]);
					}

				/* Remove rotated boundary */
				(void) destroy_curve(xbound);

				if (EditUndoable) post_mod("surface");

				/* Modify labels and highs and lows accordingly */
				if (recompute_surface_labs(EditSfc, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				put_message("edit-done");

				post_partial(FALSE);
				mstart  = FALSE;
				maction = TRUE;
				busy_cursor(FALSE);

				/* Re-display the surface */
				State = ReDisplay;
				continue;

			/* Re-display the modified surface */
			case ReDisplay:

#				ifdef DEBUG
				pr_diag("Editor", "[edit_merge_spline_2D] ReDisplay\n");
#				endif /* DEBUG */

				/* Remove the fetched field from the display */
				if (!savesfc)
					{
					msfc = take_mf_sfc(TempMeta, "a", "", "");
					sset = take_mf_set(TempMeta, "spot", "d", "sset", "sset");
					savesfc = TRUE;
					}

				/* Empty field label list */
				if (nlabs > 0)
					{
					nlabs = 0;
					alabs = 0;
					FREEMEM(smove);
					FREEMEM(scopy);
					}

				/* Re-display the modified surface and field labels */
				empty_temp();
				present_all();

				/* Move on to next stage */
				State = ReFetch;
				continue;
			}
		}
	}

/***********************************************************************
*                                                                      *
*     s m o o t h e d _ m a p _ p r o j e c t i o n s                  *
*                                                                      *
*     Map projection function for spline smoothing                     *
*                                                                      *
***********************************************************************/


static	LOGICAL		smoothed_map_projections(MAP_PROJ *, CURVE, float, float,
											 MAP_PROJ *, MAP_PROJ *);

static	LOGICAL		smoothed_map_projections

	(
	MAP_PROJ	*mproj,
	CURVE		bound,
	float		smooth,
	float		spread,
	MAP_PROJ	*emproj,
	MAP_PROJ	*smproj
	)

	{
	int			nn, nminx, nmaxx, nminy, nmaxy, enx, eny, snx, sny;
	float		minx, maxx, miny, maxy, xglen, yglen, xcen, ycen, xoff, yoff;
	float		gridln, gridlnx, gridlny;
	POINT		gpos;
	PROJ_DEF	*oproj, *eproj, *sproj;
	MAP_DEF		*omap,  *emap,  *smap;
	GRID_DEF	*ogrid, *egrid, *sgrid;

	/* Initialize re-evaluation and smoothed map projections */
	(void) copy_map_projection(emproj, mproj);
	(void) copy_map_projection(smproj, mproj);

	/* Return if no usable boundary passed */
	if (IsNull(bound->line))     return FALSE;
	if (bound->line->numpts < 3) return FALSE;

	/* Set structures to point back into original map projection */
	oproj = &(mproj->projection);
	omap  = &(mproj->definition);
	ogrid = &(mproj->grid);

	/* Set structures to point back into re-evaluation map projection */
	eproj = &(emproj->projection);
	emap  = &(emproj->definition);
	egrid = &(emproj->grid);

	/* Determine minimum and maximum grid positions from the boundary */
	minx = miny =  FPA_FLT_MAX;
	maxx = maxy = -FPA_FLT_MAX;
	for (nn=0; nn<bound->line->numpts; nn++)
		{
		(void) pos_to_grid(mproj, bound->line->points[nn], gpos);
		minx = MIN(minx, gpos[X]);
		maxx = MAX(maxx, gpos[X]);
		miny = MIN(miny, gpos[Y]);
		maxy = MAX(maxy, gpos[Y]);
		}

	/* Set center for smoothed map projection from smoothing boundary */
	xcen  = NINT(minx + maxx) / 2.0;
	ycen  = NINT(miny + maxy) / 2.0;

#	ifdef DEBUG_SMOOTH
	(void) fprintf(stdout,
		"[Spline smoothing] minx/maxx: %.2f %.2f  miny/maxy: %.2f %.2f\n",
		minx, maxx, miny, maxy);
#	endif /* DEBUG_SMOOTH */

	/* Extend minimum and maximum grid positions by spread */
	minx -= spread+1;
	maxx += spread+1;
	miny -= spread+1;
	maxy += spread+1;

#	ifdef DEBUG_SMOOTH
	(void) fprintf(stdout,
		"[Spline smoothing] minx/maxx: %.2f %.2f  miny/maxy: %.2f %.2f  spread: %.2f\n",
		minx, maxx, miny, maxy, spread);
#	endif /* DEBUG_SMOOTH */

	/* Limit minimum and maximum grid positions to the map projection */
	minx  = MAX(minx, 0);
	maxx  = MIN(maxx, (float) (ogrid->nx - 1));
	miny  = MAX(miny, 0);
	maxy  = MIN(maxy, (float) (ogrid->ny - 1));
	nminx = minx;
	nmaxx = maxx;
	if ( (float) nmaxx < maxx ) nmaxx++;
	nminy = miny;
	nmaxy = maxy;
	if ( (float) nmaxy < maxy ) nmaxy++;
	enx   = nmaxx - nminx + 1;
	eny   = nmaxy - nminy + 1;

#	ifdef DEBUG_SMOOTH
	(void) fprintf(stdout,
		"[Spline smoothing] nminx/nmaxx: %d %d  nminy/nmaxy: %d %d\n",
		nminx, nmaxx, nminy, nmaxy);
#	endif /* DEBUG_SMOOTH */

	/* Reset re-evaluation parameters if grid lengths are the same */
	if (egrid->gridlen > 0)
		{
		gridln = egrid->gridlen * egrid->units / emap->units;
		emap->xorg     = omap->xorg - (float) nminx * gridln;
		emap->yorg     = omap->yorg - (float) nminy * gridln;
		emap->xlen     = (float) (enx - 1) * gridln;
		emap->ylen     = (float) (eny - 1) * gridln;
		egrid->nx      = enx;
		egrid->ny      = eny;
		egrid->gridlen = gridln * emap->units / egrid->units;
		egrid->xgrid   = egrid->gridlen;
		egrid->ygrid   = egrid->gridlen;
		}

	/* Reset re-evaluation parameters if grid lengths are different */
	else
		{
		gridlnx = egrid->xgrid * egrid->units / emap->units;
		gridlny = egrid->ygrid * egrid->units / emap->units;
		emap->xorg     = omap->xorg - (float) nminx * gridlnx;
		emap->yorg     = omap->yorg - (float) nminy * gridlny;
		emap->xlen     = (float) (enx - 1) * gridlnx;
		emap->ylen     = (float) (eny - 1) * gridlny;
		egrid->nx      = enx;
		egrid->ny      = eny;
		egrid->xgrid   = gridlnx * emap->units / egrid->units;
		egrid->ygrid   = gridlny * emap->units / egrid->units;
		}

	/* Re-define re-evalution map projection based on modified parameters */
	define_map_projection(emproj, eproj, emap, egrid);

	/* Set structures to point back into smoothed map projection */
	sproj = &(smproj->projection);
	smap  = &(smproj->definition);
	sgrid = &(smproj->grid);

	/* Set start position for smoothed map projection */
	snx = (xcen - (float) nminx) / smooth;
	sny = (ycen - (float) nminy) / smooth;
	if ( (float) nminx + ((float) snx * smooth) < xcen ) snx++;
	if ( (float) nminy + ((float) sny * smooth) < ycen ) sny++;
	xoff = xcen - ((float) snx * smooth);
	yoff = ycen - ((float) sny * smooth);

#	ifdef DEBUG_SMOOTH
	(void) fprintf(stdout,
		"[Spline smoothing] snx/sny: %d %d  smooth: %.2f\n", snx, sny, smooth);
	(void) fprintf(stdout,
		"[Spline smoothing] xcen/ycen: %.2f %.2f  xoff/yoff: %.2f %.2f\n",
		xcen, ycen, xoff, yoff);
#	endif /* DEBUG_SMOOTH */

	/* Determine dimensions of smoothed map projection */
	snx = ((float) nmaxx - xoff) / smooth;
	sny = ((float) nmaxy - yoff) / smooth;
	if ( xoff + ((float) snx * smooth) < (float) nmaxx ) snx++;
	if ( yoff + ((float) sny * smooth) < (float) nmaxy ) sny++;
	xglen = (float) snx * smooth;
	yglen = (float) sny * smooth;

#	ifdef DEBUG_SMOOTH
	(void) fprintf(stdout,
		"[Spline smoothing] snx/sny: %d %d  smooth: %.2f\n", snx, sny, smooth);
#	endif /* DEBUG_SMOOTH */

	/* Reset smoothed parameters if grid lengths are the same */
	if (sgrid->gridlen > 0)
		{
		gridln  = sgrid->gridlen * sgrid->units / smap->units;
		smap->xorg     = omap->xorg - xoff * gridln;
		smap->yorg     = omap->yorg - yoff * gridln;
		gridln *= smooth;
		smap->xlen     = (float) snx * gridln;
		smap->ylen     = (float) sny * gridln;
		sgrid->nx      = snx + 1;
		sgrid->ny      = sny + 1;
		sgrid->gridlen = gridln * smap->units / sgrid->units;
		sgrid->xgrid   = sgrid->gridlen;
		sgrid->ygrid   = sgrid->gridlen;
		}

	/* Reset smoothed parameters if grid lengths are different */
	else
		{
		gridlnx  = sgrid->xgrid * sgrid->units / smap->units;
		gridlny  = sgrid->ygrid * sgrid->units / smap->units;
		smap->xorg     = omap->xorg - xoff * gridlnx;
		smap->yorg     = omap->yorg - yoff * gridlny;
		gridlnx *= smooth;
		gridlny *= smooth;
		smap->xlen     = (float) snx * gridlnx;
		smap->ylen     = (float) sny * gridlny;
		sgrid->nx      = snx + 1;
		sgrid->ny      = sny + 1;
		sgrid->xgrid   = gridlnx * smap->units / sgrid->units;
		sgrid->ygrid   = gridlny * smap->units / sgrid->units;
		}

	/* Re-define smoothed map projection based on modified parameters */
	define_map_projection(smproj, sproj, smap, sgrid);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     s m o o t h _ s p l i n e                                        *
*     s m o o t h _ s p l i n e _ 2 D                                  *
*                                                                      *
***********************************************************************/

LOGICAL	edit_smooth_spline

	(
	STRING	mode,
	STRING	name
	)

	{
	CURVE		curve;
	int			nlines;
	LINE		*lines;
	LOGICAL		valid, inside;
	int			dz, ix, iy;
	float		xm, ym, gdist, smooth, spread, ratio;
	double		val, tval;
	POINT		pm, px;
	MAP_PROJ	emproj, smproj;
	int			butt;
	LOGICAL		drawn = FALSE;

#	ifdef DEBUG_SMOOTH
	float		mlat, mlon, xlat, xlon;
#	endif /* DEBUG_SMOOTH */

	/* Current state: */
	/* - where should we resume? */
	static	enum	{ Draw, WaitAction, Edit } State = Draw;

	/* Boundary curve set: */
	/* - defines undistorted region */
	/* - used in cursor tracking */
	static	SET		bset  = NULL;
	static	CURVE	bound = NULL;

	/* Surface for remapping */
	static	SURFACE	bsfc = NULL;

	/* Temporary grid buffer: */
	/* - results of re-interpolation */
	/* - used to refit the new surface */
	static	float	grid   = 0.0;
	static	int		m      = 0;
	static	int		n      = 0;
	static	int		nx     = 0;
	static	int		ny     = 0;
	static	int		mx     = 0;
	static	int		my     = 0;
	static	float	**gbuf = NULL;
	static	float	*gdat  = NULL;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_smooth_spline] %s  State: %d\n", mode, State);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same(mode, "cancel"))
		{
		put_message("edit-cancel");
		ignore_partial();
		circle_echo(FALSE);
		drawing_control(FALSE);
		edit_can_proceed(FALSE);
		edit_allow_preset_outline(FALSE);

		if (bset)
			{
			bset  = NULL;
			bound = NULL;
			}
		grid = 0.0;
		bsfc = destroy_surface(bsfc);

		/* Empty display structure */
		empty_temp();
		present_all();
		clear_message();
		return FALSE;
		}
	if (same(mode, "cancel all"))
		{
		ignore_partial();
		drawing_control(FALSE);
		edit_allow_preset_outline(FALSE);

		if (bset)
			{
			bset  = NULL;
			bound = NULL;
			}
		grid = 0.0;
		bsfc = destroy_surface(bsfc);

		/* Empty display structure */
		empty_temp();
		present_all();
		clear_message();
		return FALSE;
		}

	if (same(mode, "clear"))
		{
		grid = 0.0;
		bsfc = destroy_surface(bsfc);
		return FALSE;
		}

	/* See if surface exists */
	if (!EditSfc) return FALSE;

	/* Construct temporary fields on startup */
	if (same(mode, "begin") || (grid <= 0.0))
		{
		/* Set up temporary grid buffer */
		grid = EditSfc->sp.gridlen;
		m    = EditSfc->sp.m;
		n    = EditSfc->sp.n;
		nx   = m - 2;
		ny   = n - 2;
		if ((nx != mx) || (ny != my))
			{
			mx   = nx;
			my   = ny;
			gdat = GETMEM(gdat, float, (nx*ny));
			gbuf = GETMEM(gbuf, float *, ny);
			for (iy=0; iy<ny; iy++) gbuf[iy] = gdat + iy*nx;
			}
		bsfc = destroy_surface(bsfc);
		drawing_control(FALSE);
		edit_can_proceed(FALSE);
		edit_allow_preset_outline(FALSE);
		empty_temp();

		State = Draw;
		}

	/* Set state for Right button "proceed" */
	if (same(mode, "proceed")) State = Edit;

	/* Set state for Right button "preset outline" */
	if (same(mode, "preset outline"))
		{

		/* Check for last drawn outline, last moved outline, or named outline */
		if (same(name, FpaEditorLastDrawnBoundary))
			curve = retrieve_drawn_outline();
		else if (same(name, FpaEditorLastMovedBoundary))
			curve = retrieve_moved_outline();
		else
			curve = retrieve_named_outline(name);
		if (IsNull(curve))
			{
			if (same(name, FpaEditorLastDrawnBoundary))
				{
				put_message("edit-no-drawn-outline");
				pr_warning("Editor",
					"[edit_smooth_spline] No outline drawn yet!\n");
				}
			else if (same(name, FpaEditorLastMovedBoundary))
				{
				put_message("edit-no-moved-outline");
				pr_warning("Editor",
					"[edit_smooth_spline] No outline moved yet!\n");
				}
			else
				{
				put_message("edit-no-preset-outline", name);
				pr_error("Editor",
					"[edit_smooth_spline] No preset map outline: \"%s\"\n",
					name);
				}
			(void) sleep(1);
			State = Draw;
			}

		/* Display the outline */
		else
			{
			define_lspec(&curve->lspec, 255, 0, NULL, False, 2.0, 0.0,
						 (HILITE) 2);
			add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) curve);
			present_temp(TRUE);
			post_partial(TRUE);
			State = WaitAction;
			}
		}

	/* Repeat until told to quit */
	while (TRUE)
		{
		switch (State)
			{

			/* Draw boundary of affected area */
			case Draw:

				/* Draw the boundary */
				post_partial(FALSE);
				empty_temp();
				put_message("spline-smooth-draw");
				edit_allow_preset_outline(TRUE);

				if (ready_Xcurve()) butt = LeftButton;
				else if (!ready_Xpoint(DnEdit, NULL, &butt)) return drawn;

				/* Invalid button */
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					ignore_Xpoint();
					continue;
					}
				drawn = FALSE;

				/* Draw the curve */
				drawing_control(TRUE);
				if (!ready_Xcurve())
					{
					if (drawing_Xcurve()) return drawn;
					(void) draw_Xcurve(DnEdit, FilterRes*4, SplineRes*4, TRUE);
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

				/* Extract and display curve */
				curve = create_curve("", "", "");
				add_line_to_curve(curve, lines[0]);
				post_drawn_outline(curve);
				reset_pipe();
				define_lspec(&curve->lspec, 255, 0, NULL, False, 2.0, 0.0,
							 (HILITE) 2);
				add_item_to_metafile(TempMeta, "curve", "c", "", "",
									 (ITEM) curve);
				present_temp(TRUE);
				post_partial(TRUE);

				/* Move on to next stage */
				State = WaitAction;
				continue;

			/* Wait around for a "proceed" */
			case WaitAction:

				edit_allow_preset_outline(FALSE);

				/* Truncate spline smoothing parameter */
				smooth = SplineSmth;
				dz = NINT(smooth*10.0);	smooth = 0.1 * (float) dz;
				if (smooth <= 1.0)
					{
					put_message("spline-no-smooth");
					circle_echo(FALSE);
					edit_can_proceed(FALSE);
					return FALSE;
					}
				edit_can_proceed(TRUE);

				/* Interpret spread factor */
				spread = define_spread();
				circle_echo(TRUE);

				/* Keep returning until we get a "Proceed" */
				put_message("spline-smooth-set");
				return FALSE;

			/* Perform the edit */
			case Edit:

				edit_allow_preset_outline(FALSE);

				/* Truncate spline smoothing parameter */
				smooth = SplineSmth;
				dz = NINT(smooth*10.0);	smooth = 0.1 * (float) dz;
				if (smooth <= 1.0)
					{
					put_message("spline-no-smooth");
					circle_echo(FALSE);
					edit_can_proceed(FALSE);
					State = WaitAction;
					return FALSE;
					}

				/* Interpret spread factor */
				spread = define_spread();
				circle_echo(TRUE);

#				ifdef DEBUG
				pr_diag("Editor", "[edit_smooth_spline] Editing\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);

				/* Set map projections based on drawn boundary and smoothing */
				bset  = find_mf_set(TempMeta, "curve", "c", "", "");
				bound = (CURVE) bset->list[0];
				highlight_set(bset, 0);
				(void) smoothed_map_projections(MapProj, bound, smooth,
												spread, &emproj, &smproj);

#				ifdef DEBUG_SMOOTH
				(void) fprintf(stdout, "\n Default map projection ...");
				(void) fprintf(stdout, "  type: %s\n",
						which_projection_name(MapProj->projection.type));
				(void) fprintf(stdout,
						"       ref[1-5]: %.1f %.1f %.1f %.1f %.1f\n",
						MapProj->projection.ref[0], MapProj->projection.ref[1],
						MapProj->projection.ref[2], MapProj->projection.ref[3],
						MapProj->projection.ref[4]);
				(void) fprintf(stdout, " Default map definition ...");
				(void) fprintf(stdout, "  olat: %.1f  olon: %.1f  rlon: %.1f\n",
						MapProj->definition.olat, MapProj->definition.olon,
						MapProj->definition.lref);
				(void) fprintf(stdout, "       xorg: %.0f  yorg: %.0f",
						MapProj->definition.xorg, MapProj->definition.yorg);
				(void) fprintf(stdout, "  xlen: %.0f  ylen: %.0f",
						MapProj->definition.xlen, MapProj->definition.ylen);
				(void) fprintf(stdout, "  map_units: %.0f\n",
						MapProj->definition.units);
				(void) fprintf(stdout, " Default grid definition ...");
				(void) fprintf(stdout, "  nx: %d  ny: %d  gridlen: %.0f\n",
						MapProj->grid.nx, MapProj->grid.ny,
						MapProj->grid.gridlen);
				(void) fprintf(stdout, "       xgrid: %.0f  ygrid: %.0f",
						MapProj->grid.xgrid, MapProj->grid.ygrid);
				(void) fprintf(stdout, "  map_units: %.0f\n",
						MapProj->grid.units);
				pm[X] = 0.0;
				pm[Y] = 0.0;
				px[X] = (float) (MapProj->grid.nx - 1) * MapProj->grid.xgrid;
				px[Y] = (float) (MapProj->grid.ny - 1) * MapProj->grid.ygrid;
				(void) pos_to_ll(MapProj, pm, &mlat, &mlon);
				(void) pos_to_ll(MapProj, px, &xlat, &xlon);
				(void) fprintf(stdout,
						"  LL lat/lon: %.2f/%.2f  UR lat/lon: %.2f/%.2f\n",
						mlat, mlon, xlat, xlon);
#				endif /* DEBUG_SMOOTH */

#				ifdef DEBUG_SMOOTH
				(void) fprintf(stdout, "\n Re-evaluation map projection ...");
				(void) fprintf(stdout, "  type: %s\n",
						which_projection_name(emproj.projection.type));
				(void) fprintf(stdout,
						"       ref[1-5]: %.1f %.1f %.1f %.1f %.1f\n",
						emproj.projection.ref[0], emproj.projection.ref[1],
						emproj.projection.ref[2], emproj.projection.ref[3],
						emproj.projection.ref[4]);
				(void) fprintf(stdout, " Default map definition ...");
				(void) fprintf(stdout, "  olat: %.1f  olon: %.1f  rlon: %.1f\n",
						emproj.definition.olat, emproj.definition.olon,
						emproj.definition.lref);
				(void) fprintf(stdout, "       xorg: %.0f  yorg: %.0f",
						emproj.definition.xorg, emproj.definition.yorg);
				(void) fprintf(stdout, "  xlen: %.0f  ylen: %.0f",
						emproj.definition.xlen, emproj.definition.ylen);
				(void) fprintf(stdout, "  map_units: %.0f\n",
						emproj.definition.units);
				(void) fprintf(stdout, " Default grid definition ...");
				(void) fprintf(stdout, "  nx: %d  ny: %d  gridlen: %.0f\n",
						emproj.grid.nx, emproj.grid.ny, emproj.grid.gridlen);
				(void) fprintf(stdout, "       xgrid: %.0f  ygrid: %.0f",
						emproj.grid.xgrid, emproj.grid.ygrid);
				(void) fprintf(stdout, "  map_units: %.0f\n",
						emproj.grid.units);
				pm[X] = 0.0;
				pm[Y] = 0.0;
				px[X] = (float) (emproj.grid.nx - 1) * emproj.grid.xgrid;
				px[Y] = (float) (emproj.grid.ny - 1) * emproj.grid.ygrid;
				(void) pos_to_ll(&emproj, pm, &mlat, &mlon);
				(void) pos_to_ll(&emproj, px, &xlat, &xlon);
				(void) fprintf(stdout,
						"  LL lat/lon: %.2f/%.2f  UR lat/lon: %.2f/%.2f\n",
						mlat, mlon, xlat, xlon);
#				endif /* DEBUG_SMOOTH */

#				ifdef DEBUG_SMOOTH
				(void) fprintf(stdout, "\n Smoothed map projection ...");
				(void) fprintf(stdout, "  type: %s\n",
						which_projection_name(smproj.projection.type));
				(void) fprintf(stdout,
						"       ref[1-5]: %.1f %.1f %.1f %.1f %.1f\n",
						smproj.projection.ref[0], smproj.projection.ref[1],
						smproj.projection.ref[2], smproj.projection.ref[3],
						smproj.projection.ref[4]);
				(void) fprintf(stdout, " Default map definition ...");
				(void) fprintf(stdout, "  olat: %.1f  olon: %.1f  rlon: %.1f\n",
						smproj.definition.olat, smproj.definition.olon,
						smproj.definition.lref);
				(void) fprintf(stdout, "       xorg: %.0f  yorg: %.0f",
						smproj.definition.xorg, smproj.definition.yorg);
				(void) fprintf(stdout, "  xlen: %.0f  ylen: %.0f",
						smproj.definition.xlen, smproj.definition.ylen);
				(void) fprintf(stdout, "  map_units: %.0f\n",
						smproj.definition.units);
				(void) fprintf(stdout, " Default grid definition ...");
				(void) fprintf(stdout, "  nx: %d  ny: %d  gridlen: %.0f\n",
						smproj.grid.nx, smproj.grid.ny, smproj.grid.gridlen);
				(void) fprintf(stdout, "       xgrid: %.0f  ygrid: %.0f",
						smproj.grid.xgrid, smproj.grid.ygrid);
				(void) fprintf(stdout, "  map_units: %.0f\n",
						smproj.grid.units);
				pm[X] = 0.0;
				pm[Y] = 0.0;
				px[X] = (float) (smproj.grid.nx - 1) * smproj.grid.xgrid;
				px[Y] = (float) (smproj.grid.ny - 1) * smproj.grid.ygrid;
				(void) pos_to_ll(&smproj, pm, &mlat, &mlon);
				(void) pos_to_ll(&smproj, px, &xlat, &xlon);
				(void) fprintf(stdout,
						"  LL lat/lon: %.2f/%.2f  UR lat/lon: %.2f/%.2f\n",
						mlat, mlon, xlat, xlon);
#				endif /* DEBUG_SMOOTH */

				/* Reproject surface to smoothed map projection */
				bsfc = copy_surface(EditSfc, TRUE);
				(void) reproject_surface(bsfc, MapProj, &smproj,
										 &(smproj.grid));

				/* Reproject smoothed surface to re-evaluation projection */
				(void) reproject_surface(bsfc, &smproj, &emproj,
										 &(emproj.grid));

				/* Build new grid with adjusted values inside given boundary */
				put_message("spline-adjust");
				for (ix=0; ix<nx; ix++)
					{
					xm    = ix*grid;
					pm[X] = xm;
					for (iy=0; iy<ny; iy++)
						{
						ym     = iy*grid;
						pm[Y]  = ym;
						curve_test_point(bound, pm, &gdist, NULL, NULL,
										 &inside, NULL);
						gdist /= grid;
						if (inside)
							{
							(void) pos_to_pos(MapProj, pm, &emproj, px);
							valid = eval_sfc(bsfc, px, &tval);
							gbuf[iy][ix] = (float) tval;
#							ifdef DEBUG_SMOOTH
							(void) fprintf(stdout,
								"Smooth inside at %d/%d  tval: %.1f\n",
								ix, iy, tval);
#							endif /* DEBUG_SMOOTH */
							}
						else if (gdist <= spread+1)
							{
							valid = eval_sfc_unmapped(EditSfc, pm, &val);
							(void) pos_to_pos(MapProj, pm, &emproj, px);
							valid = eval_sfc(bsfc, px, &tval);
							ratio = ramp(gdist, spread);
							gbuf[iy][ix] = (float) tval * ratio
											+ (float) val * (1.0 - ratio);
#							ifdef DEBUG_SMOOTH
							(void) fprintf(stdout,
								"Smooth edge at %d/%d  val/tval: %.1f/%.1f  gdist: %.2f\n",
								ix, iy, val, tval, gdist);
#							endif /* DEBUG_SMOOTH */
							}
						else
							{
							valid = eval_sfc_unmapped(EditSfc, pm, &val);
							gbuf[iy][ix] = (float) val;
							}
						}
					}

				/* Fit surface to new grid values */
				grid_surface(EditSfc, grid, nx, ny, gbuf);
				contour_surface(EditSfc);
				if (EditUndoable) post_mod("surface");

				/* Modify labels and highs and lows accordingly */
				if (recompute_surface_labs(EditSfc, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				/* Reset boundary curve and reset displacement fields */
				bset  = NULL;
				bound = NULL;
				bsfc  = destroy_surface(bsfc);

				/* Re-display the surface */
				put_message("edit-done");
				empty_temp();
				present_all();
				busy_cursor(FALSE);
				edit_can_proceed(FALSE);

				/* Move on to next stage */
				State = Draw;
				continue;
			}
		}
	}

/**********************************************************************/

LOGICAL	edit_smooth_spline_2D

	(
	STRING	mode,
	STRING	name
	)

	{
	CURVE		curve;
	int			nlines;
	LINE		*lines;
	LOGICAL		valid, inside;
	int			dz, ix, iy;
	float		xm, ym, gdist, smooth, spread, ratio;
	double		xval, yval, sval, tval;
	POINT		pm, px;
	MAP_PROJ	emproj, smproj;
	int			butt;
	LOGICAL		drawn = FALSE;

	/* Current state: */
	/* - where should we resume? */
	static	enum	{ Draw, WaitAction, Edit } State = Draw;

	/* Boundary curve set: */
	/* - defines undistorted region */
	/* - used in cursor tracking */
	static	SET		bset  = NULL;
	static	CURVE	bound = NULL;

	/* Surface for remapping */
	static	SURFACE	bsfc = NULL;

	/* Temporary grid buffer: */
	/* - results of re-interpolation */
	/* - used to refit the new surface */
	static	float	grid   = 0.0;
	static	int		m      = 0;
	static	int		n      = 0;
	static	int		nx     = 0;
	static	int		ny     = 0;
	static	int		mx     = 0;
	static	int		my     = 0;
	static	float	**gxbuf = NULL;
	static	float	**gybuf = NULL;
	static	float	*gxdat  = NULL;
	static	float	*gydat  = NULL;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_smooth_spline_2D] %s  State: %d\n", mode, State);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same(mode, "cancel"))
		{
		put_message("edit-cancel");
		ignore_partial();
		circle_echo(FALSE);
		drawing_control(FALSE);
		edit_can_proceed(FALSE);
		edit_allow_preset_outline(FALSE);

		if (bset)
			{
			bset  = NULL;
			bound = NULL;
			}
		grid = 0.0;
		bsfc = destroy_surface(bsfc);

		/* Empty display structure */
		empty_temp();
		present_all();
		clear_message();
		return FALSE;
		}
	if (same(mode, "cancel all"))
		{
		ignore_partial();
		drawing_control(FALSE);
		edit_allow_preset_outline(FALSE);

		if (bset)
			{
			bset  = NULL;
			bound = NULL;
			}
		grid = 0.0;
		bsfc = destroy_surface(bsfc);

		/* Empty display structure */
		empty_temp();
		present_all();
		clear_message();
		return FALSE;
		}

	if (same(mode, "clear"))
		{
		grid = 0.0;
		bsfc = destroy_surface(bsfc);
		return FALSE;
		}

	/* See if surface exists */
	if (!EditSfc) return FALSE;

	/* Construct temporary fields on startup */
	if (same(mode, "begin") || (grid <= 0.0))
		{
		/* Set up temporary grid buffer */
		grid = EditSfc->sp.gridlen;
		m    = EditSfc->sp.m;
		n    = EditSfc->sp.n;
		nx   = m - 2;
		ny   = n - 2;
		if ((nx != mx) || (ny != my))
			{
			mx   = nx;
			my   = ny;
			gxdat = GETMEM(gxdat, float, (nx*ny));
			gydat = GETMEM(gydat, float, (nx*ny));
			gxbuf = GETMEM(gxbuf, float *, ny);
			gybuf = GETMEM(gybuf, float *, ny);
			for (iy=0; iy<ny; iy++)
				{
				gxbuf[iy] = gxdat + iy*nx;
				gybuf[iy] = gydat + iy*nx;
				}
			}
		bsfc = destroy_surface(bsfc);
		drawing_control(FALSE);
		edit_can_proceed(FALSE);
		edit_allow_preset_outline(FALSE);
		empty_temp();

		State = Draw;
		}

	/* Set state for Right button "proceed" */
	if (same(mode, "proceed")) State = Edit;

	/* Set state for Right button "preset outline" */
	if (same(mode, "preset outline"))
		{

		/* Check for last drawn outline, last moved outline, or named outline */
		if (same(name, FpaEditorLastDrawnBoundary))
			curve = retrieve_drawn_outline();
		else if (same(name, FpaEditorLastMovedBoundary))
			curve = retrieve_moved_outline();
		else
			curve = retrieve_named_outline(name);
		if (IsNull(curve))
			{
			if (same(name, FpaEditorLastDrawnBoundary))
				{
				put_message("edit-no-drawn-outline");
				pr_warning("Editor",
					"[edit_smooth_spline_2D] No outline drawn yet!\n");
				}
			else if (same(name, FpaEditorLastMovedBoundary))
				{
				put_message("edit-no-moved-outline");
				pr_warning("Editor",
					"[edit_smooth_spline_2D] No outline moved yet!\n");
				}
			else
				{
				put_message("edit-no-preset-outline", name);
				pr_error("Editor",
					"[edit_smooth_spline_2D] No preset map outline: \"%s\"\n",
					name);
				}
			(void) sleep(1);
			State = Draw;
			}

		/* Display the outline */
		else
			{
			define_lspec(&curve->lspec, 255, 0, NULL, False, 2.0, 0.0,
						 (HILITE) 2);
			add_item_to_metafile(TempMeta, "curve", "c", "", "", (ITEM) curve);
			present_temp(TRUE);
			post_partial(TRUE);
			State = WaitAction;
			}
		}

	/* Repeat until told to quit */
	while (TRUE)
		{
		switch (State)
			{

			/* Draw boundary of affected area */
			case Draw:

				/* Draw the boundary */
				post_partial(FALSE);
				empty_temp();
				put_message("spline-smooth-draw");
				edit_allow_preset_outline(TRUE);

				if (ready_Xcurve()) butt = LeftButton;
				else if (!ready_Xpoint(DnEdit, NULL, &butt)) return drawn;

				/* Invalid button */
				if (butt != LeftButton)
					{
					put_message("edit-wrong-button");
					ignore_Xpoint();
					continue;
					}
				drawn = FALSE;

				/* Draw the curve */
				drawing_control(TRUE);
				if (!ready_Xcurve())
					{
					if (drawing_Xcurve()) return drawn;
					(void) draw_Xcurve(DnEdit, FilterRes*4, SplineRes*4, TRUE);
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

				/* Extract and display curve */
				curve = create_curve("", "", "");
				add_line_to_curve(curve, lines[0]);
				post_drawn_outline(curve);
				reset_pipe();
				define_lspec(&curve->lspec, 255, 0, NULL, False, 2.0, 0.0,
							 (HILITE) 2);
				add_item_to_metafile(TempMeta, "curve", "c", "", "",
									 (ITEM) curve);
				present_temp(TRUE);
				post_partial(TRUE);

				/* Move on to next stage */
				State = WaitAction;
				continue;

			/* Wait around for a "proceed" */
			case WaitAction:

				edit_allow_preset_outline(FALSE);

				/* Truncate spline smoothing parameter */
				smooth = SplineSmth;
				dz = NINT(smooth*10.0);	smooth = 0.1 * (float) dz;
				if (smooth <= 1.0)
					{
					put_message("spline-no-smooth");
					circle_echo(FALSE);
					edit_can_proceed(FALSE);
					return FALSE;
					}
				edit_can_proceed(TRUE);

				/* Interpret spread factor */
				spread = define_spread();
				circle_echo(TRUE);

				/* Keep returning until we get a "Proceed" */
				put_message("spline-smooth-set");
				return FALSE;

			/* Perform the edit */
			case Edit:

				edit_allow_preset_outline(FALSE);

				/* Truncate spline smoothing parameter */
				smooth = SplineSmth;
				dz = NINT(smooth*10.0);	smooth = 0.1 * (float) dz;
				if (smooth <= 1.0)
					{
					put_message("spline-no-smooth");
					circle_echo(FALSE);
					edit_can_proceed(FALSE);
					State = WaitAction;
					return FALSE;
					}

				/* Interpret spread factor */
				spread = define_spread();
				circle_echo(TRUE);

#				ifdef DEBUG
				pr_diag("Editor", "[edit_smooth_spline_2D] Editing\n");
#				endif /* DEBUG */

				/* Here we go */
				drawn = TRUE;
				ignore_partial();
				busy_cursor(TRUE);

				/* Set map projections based on drawn boundary and smoothing */
				bset  = find_mf_set(TempMeta, "curve", "c", "", "");
				bound = (CURVE) bset->list[0];
				highlight_set(bset, 0);
				(void) smoothed_map_projections(MapProj, bound, smooth,
												spread, &emproj, &smproj);

				/* Reproject surface to smoothed map projection */
				bsfc = copy_surface(EditSfc, TRUE);
				(void) reproject_surface(bsfc, MapProj, &smproj,
										 &(smproj.grid));

				/* Reproject smoothed surface to re-evaluation projection */
				(void) reproject_surface(bsfc, &smproj, &emproj,
										 &(emproj.grid));

				/* Build new grid with adjusted values inside given boundary */
				put_message("spline-adjust");
				for (ix=0; ix<nx; ix++)
					{
					xm    = ix*grid;
					pm[X] = xm;
					for (iy=0; iy<ny; iy++)
						{
						ym     = iy*grid;
						pm[Y]  = ym;
						curve_test_point(bound, pm, &gdist, NULL, NULL,
										 &inside, NULL);
						gdist /= grid;
						if (inside)
							{
							(void) pos_to_pos(MapProj, pm, &emproj, px);
							valid = eval_sfc_UV(bsfc, px, &sval, &tval);
							gxbuf[iy][ix] = (float) sval;
							gybuf[iy][ix] = (float) tval;
							}
						else if (gdist <= spread+1)
							{
							valid = eval_sfc_UV_unmapped(EditSfc, pm,
														 &xval, &yval);
							(void) pos_to_pos(MapProj, pm, &emproj, px);
							valid = eval_sfc_UV(bsfc, px, &sval, &tval);
							ratio = ramp(gdist, spread);
							gxbuf[iy][ix] = (float) sval * ratio
											+ (float) xval * (1.0 - ratio);
							gybuf[iy][ix] = (float) tval * ratio
											+ (float) yval * (1.0 - ratio);
							}
						else
							{
							valid = eval_sfc_UV_unmapped(EditSfc, pm,
														 &xval, &yval);
							gxbuf[iy][ix] = (float) xval;
							gybuf[iy][ix] = (float) yval;
							}
						}
					}

				/* Fit surface to new grid values */
				grid_surface_2D(EditSfc, grid, nx, ny, gxbuf, gybuf);
				contour_surface(EditSfc);
				if (EditUndoable) post_mod("surface");

				/* Modify labels and highs and lows accordingly */
				if (recompute_surface_labs(EditSfc, EditLabs, EditRetain))
					{
					if (EditUndoable) post_mod("labs");
					}

				/* Reset boundary curve and reset displacement fields */
				bset  = NULL;
				bound = NULL;
				bsfc  = destroy_surface(bsfc);

				/* Re-display the surface */
				put_message("edit-done");
				empty_temp();
				present_all();
				busy_cursor(FALSE);
				edit_can_proceed(FALSE);

				/* Move on to next stage */
				State = Draw;
				continue;
			}
		}
	}

/***********************************************************************
*                                                                      *
*     g r a b c o n t _ s p l i n e                                    *
*                                                                      *
***********************************************************************/

LOGICAL	edit_grabcont_spline

	(
	STRING	mode
	)

	{
	POINT	p;
	int		butt;
	SET		contours;
	int		ic;
	CURVE	curve;
	STRING	val;
	LOGICAL	valid;
	float	cval;

	LOGICAL	drawn = FALSE;

#	ifdef DEBUG
	pr_diag("Editor", "[edit_grabcont_spline] %s\n", mode);
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

	/* See if surface exists */
	if (!EditSfc) return FALSE;

	if (!ScratchShown)
		{
		string_message("prompt", "Turn on Scratchpad first");
		return FALSE;
		}

	/* Select contours to grab */
	while (TRUE)
		{
		/* Put up a prompt */
		string_message("prompt", "Select contour to copy");

		/* Get the point */
		if (!ready_Xpoint(DnEdit, NullPoint, &butt))
			{
			circle_echo(TRUE);
			return drawn;
			}
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}
		(void) pick_Xpoint(DnEdit, 0, p, &butt);
		if (!inside_dn_window(DnEdit, p))
			{
			put_message("edit-outside-map");
			(void) sleep(1);
			continue;
			}

		/* Find the closest contour to the selected point */
		val = eval_sfc_feature(EditSfc, p, "c", NULL, NULL, NULL, &valid);
		if (!valid) continue;
		if (sscanf(val, "%f", &cval) < 1) continue;

		contours = contour_curveset(EditSfc, cval, NULL);
		for (ic=0; ic<contours->num; ic++)
			{
			curve = copy_curve((CURVE) contours->list[ic]);
			define_curve_value(curve, "LSPEC",
							   "[colour=Red style=solid width=3]", NULL);
			add_item_to_metafile(ScratchMeta, "curve",
								 "c", "NULL", "NULL", (ITEM) curve);
			}
		contours = destroy_set(contours);

		update_scratch();
		present_scratch(TRUE);
		}

	}
