/***********************************************************************
*                                                                      *
*     i n t e r p _ s p l n . c                                        *
*                                                                      *
*     Routines to time-interpolate B-spline surface fields.            *
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

#undef DEBUG_INTERP

#define ltime my_ltime

/***********************************************************************
*                                                                      *
*    i n t e r p _ s p l i n e                                         *
*                                                                      *
*    Perform the time interpolation of scalar surfaces from the        *
*    working depiction sequence, onto the given interval.              *
*                                                                      *
*    The set of keyframe surfaces is provided in dfld->fields.         *
*    The set of generated surfaces is deposited in dfld->tweens.       *
*                                                                      *
***********************************************************************/

LOGICAL	interp_spline

	(
	DFLIST	*dfld,
	LOGICAL	show,
	LOGICAL	showtween
	)

	{
	STRING	elem, ent, lev;
	int		ltime, rtime, itween, ichain, lnode, rnode, ifld;
	int		idspl, ndspl, mdspl, nchain;
	int		ix, iy, nx, ny, lmplus, rmplus, tmplus, imem;
	LOGICAL	valid;
	int		mfirst;
	float	tlen, tstep, lfact, rfact, glen;
	float	xlim, ylim, dxbar, dybar, dx, dy, mx, my;
	float	gtol, step, fact, ds, s, ddx, ddy;
	double	dxval, dyval, lval, rval;
	int		ns;
	POINT	*dpos, *dspl, gpos, lpos, rpos, pos;
	float	**gridx, **gridy, **gridf, *gbufx, *gbufy, *gbuff;
	SURFACE	dxsfc, dysfc, lsfc, rsfc, sfc, tsfc;
	FIELD	*keylist, *genlist, fld;
	SET		*keylabs, *genlabs, rlabs, llabs, labs;
	SPOT	spot;
	LCHAIN	chain;

	if (!dfld)                           return FALSE;
	if (!dfld->dolink)                   return FALSE;
	if (dfld->editor != FpaC_CONTINUOUS) return FALSE;
	if (NumTime < 2)                     return FALSE;
	if (NumTween < 2)                    return FALSE;
	if (!dfld->linked)                   return FALSE;
	if (dfld->interp && dfld->intlab)    return TRUE;
	if (!dfld->there)
		{

#		ifdef DEVELOPMENT
		if (dfld->reported)
			pr_info("Editor.Reported",
				"In interp_spline() - T %s %s\n", dfld->element, dfld->level);
		else
			pr_info("Editor.Reported",
				"In interp_spline() - F %s %s\n", dfld->element, dfld->level);
#		endif /* DEVELOPMENT */

		dfld->interp   = TRUE;
		dfld->intlab   = TRUE;
		dfld->saved    = FALSE;
		dfld->reported = FALSE;
		link_status(dfld);
		(void) save_links();
		return TRUE;
		}

	showtween = (LOGICAL) (show && showtween);
	busy_cursor(TRUE);
	empty_blank();

	/* Extract required information from dfld */
	prepare_dfield_tweens(dfld);
	nchain  = dfld->nchain;
	keylist = dfld->fields;
	keylabs = dfld->flabs;
	genlist = dfld->tweens;
	genlabs = dfld->tlabs;

	/* Find the first active keyframe */
	for (ltime=0; ltime<NumTime; ltime++)
		{
		if (dfld->frames[ltime].meta) break;
		}

	/* Calculate grid parameters */
	sfc  = keylist[ltime]->data.sfc;
	nx   = sfc->sp.m - 2;
	ny   = sfc->sp.n - 2;
	glen = sfc->sp.gridlen;
	xlim = glen*(nx-1);
	ylim = glen*(ny-1);

	/* Allocate temporary buffers */
	mdspl = nchain*2;
	dpos  = INITMEM(POINT, mdspl);
	dspl  = INITMEM(POINT, mdspl);
	gbufx = INITMEM(float, nx*ny);
	gbufy = INITMEM(float, nx*ny);
	gbuff = INITMEM(float, nx*ny);
	gridx = INITMEM(float *, ny);
	gridy = INITMEM(float *, ny);
	gridf = INITMEM(float *, ny);
	for (iy=0; iy<ny; iy++)
		{
		gridx[iy] = gbufx + iy*nx;
		gridy[iy] = gbufy + iy*nx;
		gridf[iy] = gbuff + iy*nx;
		}

	/* Create displacement surfaces */
	dxsfc = create_surface();
	dysfc = create_surface();

	/* Allocate output fields */
	ent  = "a";
	elem = dfld->element;
	lev  = dfld->level;
	for (itween=0; itween<NumTween; itween++)
		{
		genlist[itween] = destroy_field(genlist[itween]);
		genlabs[itween] = destroy_set(genlabs[itween]);
		}

	/* Loop through the key frame sequence */
	mfirst = TimeList[first_depict_time()].mplus;
	rtime  = ltime;
	rsfc   = keylist[rtime]->data.sfc;
	rlabs  = keylabs[rtime];
	rmplus = TimeList[rtime].mplus;
	tmplus = rmplus;
	for ( ; ltime<NumTime; ltime=rtime)
		{
		/* Duplicate keyframe if it coincides with a target time */
		if (rmplus == tmplus)
			{

#			ifdef DEBUG_INTERP
			if (minutes_in_depictions())
				printf("           T%s (copying)\n",
					hour_minute_string(0, tmplus));
			else
				printf("           T%+.2d (copying)\n",
					tmplus/60);
#			endif /* DEBUG_INTERP */

			if (minutes_in_depictions())
				put_message("spline-interp-mins",
							lev, elem, hour_minute_string(0, tmplus));
			else
				put_message("spline-interp",
							lev, elem, tmplus/60);

			/* Use the key frame in the corresponding output frame */
			itween = (tmplus - mfirst)/DTween;
			interp_progress(dfld, itween, NumTween, -1, -1);
			sfc    = copy_surface(rsfc, TRUE);
			labs   = copy_set(rlabs);
			genlist[itween] = create_field(ent, elem, lev);
			define_fld_data(genlist[itween], "surface", (POINTER)sfc);
			genlabs[itween] = labs;
			if (show)
				{
				if	( NotNull(sfc)
					  && ( IsNull(sfc->patches)
						   || IsNull(sfc->patches[0][0])
						   || !sfc->patches[0][0]->defined
						   || IsNull(sfc->patches[0][0]->contours)
						 )
					) contour_surface(sfc);
				fld = create_field(ent, elem, lev);
				define_fld_data(fld, "surface", (POINTER)sfc);
				add_field_to_metafile(BlankMeta, fld);
				if (labs)
					{
					fld = create_field("d", elem, lev);
					define_fld_data(fld, "set", (POINTER)labs);
					add_field_to_metafile(BlankMeta, fld);
					}
				present_blank(TRUE);
				}
			tmplus += DTween;
			}

		/* Set up description of key window */
		if (ltime >= NumTime-1) break;
		lsfc   = rsfc;
		llabs  = rlabs;
		lmplus = rmplus;

		/* Find the next active keyframe */
		for (rtime=ltime+1; rtime<NumTime; rtime++)
			{
			if (dfld->frames[rtime].meta) break;
			}
		if (rtime >= NumTime) break;
		rsfc   = keylist[rtime]->data.sfc;
		rlabs  = keylabs[rtime];
		rmplus = TimeList[rtime].mplus;

		/* Set up displacements across key window */
		ndspl = 0;
		dxbar = 0;
		dybar = 0;
		gtol  = lsfc->sp.gridlen;
		for (ichain=0; ichain<nchain; ichain++)
			{
			chain = dfld->chains[ichain];
			/* >>>>> replace this ...
			copy_point(lpos, chain->nodes[ltime]->node);
			copy_point(rpos, chain->nodes[rtime]->node);
			... with this <<<<< */
			lnode = which_lchain_node(chain, LchainNode, TimeList[ltime].mplus);
			rnode = which_lchain_node(chain, LchainNode, TimeList[rtime].mplus);
			if (lnode < 0 || rnode < 0)         continue;
			if (!chain->nodes[lnode]->there)    continue;
			if (!chain->nodes[rnode]->there)    continue;
			copy_point(lpos, chain->nodes[lnode]->node);
			copy_point(rpos, chain->nodes[rnode]->node);
			if (!inside_dn_window(DnMap, lpos)) continue;
			if (!inside_dn_window(DnMap, rpos)) continue;
			dx = rpos[X] - lpos[X];
			dy = rpos[Y] - lpos[Y];
			mx = (rpos[X] + lpos[X]) / 2;
			my = (rpos[Y] + lpos[Y]) / 2;

			/* Need to spread out long displacements */
			ds = hypot(dx, dy);
			ns = ds/gtol;
			for ( ; ns>0; ns--)
				{
				/* Work from outer edges toward centre */
				s    = ns*gtol;
				step = s/ds;
				fact = 1 - s/(ds+gtol);
				ddx  = fact*dx;
				ddy  = fact*dy;

				/* Displacement behind */
				dxbar += ddx;
				dybar += ddy;
				idspl  = ndspl++;
				if (ndspl > mdspl)
					{
					mdspl += nchain;
					dpos   = GETMEM(dpos, POINT, mdspl);
					dspl   = GETMEM(dspl, POINT, mdspl);
					}
				dpos[idspl][X] = mx - dx*step;
				dpos[idspl][Y] = my - dy*step;
				dspl[idspl][X] = ddx;
				dspl[idspl][Y] = ddy;

				/* Displacement ahead */
				dxbar += ddx;
				dybar += ddy;
				idspl  = ndspl++;
				if (ndspl > mdspl)
					{
					mdspl += nchain;
					dpos   = GETMEM(dpos, POINT, mdspl);
					dspl   = GETMEM(dspl, POINT, mdspl);
					}
				dpos[idspl][X] = mx + dx*step;
				dpos[idspl][Y] = my + dy*step;
				dspl[idspl][X] = ddx;
				dspl[idspl][Y] = ddy;
				}

			/* Place main displacement at centre */
			dxbar += dx;
			dybar += dy;
			idspl  = ndspl++;
			if (ndspl > mdspl)
				{
				mdspl += nchain;
				dpos   = GETMEM(dpos, POINT, mdspl);
				dspl   = GETMEM(dspl, POINT, mdspl);
				}
			dpos[idspl][X] = mx;
			dpos[idspl][Y] = my;
			dspl[idspl][X] = dx;
			dspl[idspl][Y] = dy;
			}

		/* Initialize displacement grids to average values */
		if (ndspl > 0)
			{
			dxbar /= (float) ndspl;
			dybar /= (float) ndspl;
			}
		for (ix=0; ix<nx; ix++)
			for (iy=0; iy<ny; iy++)
				{
				gridx[iy][ix] = dxbar;
				gridy[iy][ix] = dybar;
				}

		/* Generate flat displacement surfaces to fit grids */
		grid_surface(dxsfc, glen, nx, ny, gridx);
		grid_surface(dysfc, glen, nx, ny, gridy);

		/* Fit displacement surfaces to actual displacements */
		/* May have to repeat this several times if points are close */
		for (idspl=0; idspl<ndspl; idspl++)
			{
			edit_surface(dxsfc, dpos[idspl], dspl[idspl][X], TRUE, FALSE);
			edit_surface(dysfc, dpos[idspl], dspl[idspl][Y], TRUE, FALSE);
			}

		/* Interpolate output frames between current and next key frames */
		tlen = (float) (rmplus - lmplus);

		for ( ; tmplus<rmplus; tmplus+=DTween)
			{

#			ifdef DEBUG_INTERP
			if (minutes_in_depictions())
				printf("           T%s (interpolating)\n",
					hour_minute_string(0, tmplus));
			else
				printf("           T%+.2d (interpolating)\n",
					tmplus/60);
#			endif /* DEBUG_INTERP */

			if (minutes_in_depictions())
				put_message("spline-interp-mins",
							lev, elem, hour_minute_string(0, tmplus));
			else
				put_message("spline-interp",
							lev, elem, tmplus/60);

			itween = (tmplus - mfirst)/DTween;
			interp_progress(dfld, itween, NumTween, -1, -1);
			tstep  = (float) (tmplus - lmplus);
			lfact  = tstep / tlen;
			rfact  = 1 - lfact;

			/* Fill grid with weighted average of forward and backward */
			/* displaced values */
			for (ix=0; ix<nx; ix++)
				{
				gpos[X] = ix * glen;
				for (iy=0; iy<ny; iy++)
					{
					gpos[Y] = iy * glen;
					valid   = eval_sfc(dxsfc, gpos, &dxval);
					valid   = eval_sfc(dysfc, gpos, &dyval);
					lpos[X] = gpos[X] - dxval*lfact;
					lpos[Y] = gpos[Y] - dyval*lfact;
					rpos[X] = gpos[X] + dxval*rfact;
					rpos[Y] = gpos[Y] + dyval*rfact;
					lpos[X] = MAX(lpos[X], 0.);	lpos[X] = MIN(lpos[X], xlim);
					lpos[Y] = MAX(lpos[Y], 0.);	lpos[Y] = MIN(lpos[Y], ylim);
					rpos[X] = MAX(rpos[X], 0.);	rpos[X] = MIN(rpos[X], xlim);
					rpos[Y] = MAX(rpos[Y], 0.);	rpos[Y] = MIN(rpos[Y], ylim);
					valid   = eval_sfc(lsfc, lpos, &lval);
					valid   = eval_sfc(rsfc, rpos, &rval);
					gridf[iy][ix] = (float) (rfact*lval + lfact*rval);
					}
				}

			/* Fit new surface to the grid */
			sfc = copy_surface(lsfc, FALSE);
			grid_surface(sfc, glen, nx, ny, gridf);
			genlist[itween] = create_field(ent, elem, lev);
			define_fld_data(genlist[itween], "surface", (POINTER)sfc);

			/* Interpolate labels ahead from previous frame */
			/* or back from following frame */
			labs = NullSet;
			if (lfact<=0.5)
				{
				if (NotNull(llabs)) labs = copy_set(llabs);
				fact = lfact;
				}
			else
				{
				if (NotNull(rlabs)) labs = copy_set(rlabs);
				fact = -rfact;
				}

			tsfc = NullSfc;
			if (NotNull(labs) || showtween)
				{
				/* Need to contour the surface for interpolating labels */
				/* or displaying */
				tsfc = (showtween)? sfc: copy_surface(sfc, FALSE);
				contour_surface(tsfc);
				}

			if (NotNull(labs))
				{
				genlabs[itween] = labs;
				for (imem=0; imem<labs->num; imem++)
					{
					spot = (SPOT) labs->list[imem];
					if (IsNull(spot))                 continue;
					if (same(spot->mclass, "legend")) continue;
					if (same(spot->mclass, "TSPEC"))  continue;

					pos[X]  = spot->anchor[X];
					pos[Y]  = spot->anchor[Y];
					valid   = eval_sfc(dxsfc, pos, &dxval);
					valid   = eval_sfc(dysfc, pos, &dyval);
					gpos[X] = pos[X] + dxval*fact;
					gpos[Y] = pos[Y] + dyval*fact;
					copy_point(spot->anchor, gpos);
					}
				recompute_surface_labs(tsfc, labs, FALSE);
				if (!showtween) tsfc = destroy_surface(tsfc);
				}

			if (showtween)
				{
				fld = create_field(ent, elem, lev);
				define_fld_data(fld, "surface", (POINTER)sfc);
				add_field_to_metafile(BlankMeta, fld);
				if (labs)
					{
					fld = create_field("d", elem, lev);
					define_fld_data(fld, "set", (POINTER)labs);
					add_field_to_metafile(BlankMeta, fld);
					}
				present_blank(TRUE);
				}

			}   /* Next output frame (tmplus) */

		}	/* Next key frame (ltime) */

	/* Free temporary buffers */
	dxsfc = destroy_surface(dxsfc);
	dysfc = destroy_surface(dysfc);
	FREEMEM(dpos);
	FREEMEM(dspl);
	FREEMEM(gbufx);
	FREEMEM(gbufy);
	FREEMEM(gbuff);
	FREEMEM(gridx);
	FREEMEM(gridy);
	FREEMEM(gridf);

#	ifdef DEVELOPMENT
	if (dfld->reported)
		pr_info("Editor.Reported",
			"In interp_spline() - T %s %s\n", dfld->element, dfld->level);
	else
		pr_info("Editor.Reported",
			"In interp_spline() - F %s %s\n", dfld->element, dfld->level);
#	endif /* DEVELOPMENT */

	/* Restore graphics and reset flags */
	dfld->interp   = TRUE;
	dfld->intlab   = TRUE;
	dfld->saved    = FALSE;
	dfld->reported = FALSE;
	link_status(dfld);
	(void) save_links();
	for (ifld=0; ifld<BlankMeta->numfld; ifld++)
		{
		fld = BlankMeta->fields[ifld];
		switch (fld->ftype)
			{
			case FtypeSfc:	fld->data.sfc  = NullSfc;
							break;
			case FtypeSet:	fld->data.set  = NullSet;
							break;
			case FtypePlot:	fld->data.plot = NullPlot;
							break;
			}
		}
	empty_blank();
	busy_cursor(FALSE);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    i n t e r p _ s p l i n e _ 2 D                                   *
*                                                                      *
*    Perform the time interpolation of vector surfaces from the        *
*    working depiction sequence, onto the given interval.              *
*                                                                      *
*    The set of keyframe surfaces is provided in dfld->fields.         *
*    The set of generated surfaces is deposited in dfld->tweens.       *
*                                                                      *
***********************************************************************/

LOGICAL	interp_spline_2D

	(
	DFLIST	*dfld,
	LOGICAL	show,
	LOGICAL	showtween
	)

	{
	STRING	elem, ent, lev;
	int		ltime, rtime, itween, ichain, lnode, rnode, ifld;
	int		idspl, ndspl, mdspl, nchain;
	int		ix, iy, nx, ny, lmplus, rmplus, tmplus, imem;
	LOGICAL	valid;
	int		mfirst;
	float	tlen, tstep, lfact, rfact, glen;
	float	xlim, ylim, dxbar, dybar, dx, dy, mx, my;
	float	gtol, step, fact, ds, s, ddx, ddy;
	double	dxval, dyval, luval, lvval, ruval, rvval;
	int		ns;
	POINT	*dpos, *dspl, gpos, lpos, rpos, pos;
	float	**gridx, **gridy, **gridu, **gridv, *gbufx, *gbufy, *gbufu, *gbufv;
	SURFACE	dxsfc, dysfc, lsfc, rsfc, sfc, tsfc;
	FIELD	*keylist, *genlist, fld;
	SET		*keylabs, *genlabs, rlabs, llabs, labs;
	SPOT	spot;
	LCHAIN	chain;

	if (!dfld)                        return FALSE;
	if (!dfld->dolink)                return FALSE;
	if (dfld->editor != FpaC_VECTOR)  return FALSE;
	if (NumTime < 2)                  return FALSE;
	if (NumTween < 2)                 return FALSE;
	if (!dfld->linked)                return FALSE;
	if (dfld->interp && dfld->intlab) return TRUE;
	if (!dfld->there)
		{

#		ifdef DEVELOPMENT
		if (dfld->reported)
			pr_info("Editor.Reported",
				"In interp_spline_2D() - T %s %s\n",
				dfld->element, dfld->level);
		else
			pr_info("Editor.Reported",
				"In interp_spline_2D() - F %s %s\n",
				dfld->element, dfld->level);
#		endif /* DEVELOPMENT */

		dfld->interp   = TRUE;
		dfld->intlab   = TRUE;
		dfld->saved    = FALSE;
		dfld->reported = FALSE;
		link_status(dfld);
		(void) save_links();
		return TRUE;
		}

	showtween = (LOGICAL) (show && showtween);
	busy_cursor(TRUE);
	empty_blank();

	/* Extract required information from dfld */
	prepare_dfield_tweens(dfld);
	nchain  = dfld->nchain;
	keylist = dfld->fields;
	keylabs = dfld->flabs;
	genlist = dfld->tweens;
	genlabs = dfld->tlabs;

	/* Find the first active keyframe */
	for (ltime=0; ltime<NumTime; ltime++)
		{
		if (dfld->frames[ltime].meta) break;
		}

	/* Calculate grid parameters */
	sfc  = keylist[ltime]->data.sfc;
	nx   = sfc->sp.m - 2;
	ny   = sfc->sp.n - 2;
	glen = sfc->sp.gridlen;
	xlim = glen*(nx-1);
	ylim = glen*(ny-1);

	/* Allocate temporary buffers */
	mdspl = nchain*2;
	dpos  = INITMEM(POINT, mdspl);
	dspl  = INITMEM(POINT, mdspl);
	gbufx = INITMEM(float, nx*ny);
	gbufy = INITMEM(float, nx*ny);
	gbufu = INITMEM(float, nx*ny);
	gbufv = INITMEM(float, nx*ny);
	gridx = INITMEM(float *, ny);
	gridy = INITMEM(float *, ny);
	gridu = INITMEM(float *, ny);
	gridv = INITMEM(float *, ny);
	for (iy=0; iy<ny; iy++)
		{
		gridx[iy] = gbufx + iy*nx;
		gridy[iy] = gbufy + iy*nx;
		gridu[iy] = gbufu + iy*nx;
		gridv[iy] = gbufv + iy*nx;
		}

	/* Create displacement surfaces */
	dxsfc = create_surface();
	dysfc = create_surface();

	/* Allocate output fields */
	ent  = "v";
	elem = dfld->element;
	lev  = dfld->level;
	for (itween=0; itween<NumTween; itween++)
		{
		genlist[itween] = destroy_field(genlist[itween]);
		genlabs[itween] = destroy_set(genlabs[itween]);
		}

	/* Loop through the key frame sequence */
	mfirst = TimeList[first_depict_time()].mplus;
	rtime  = ltime;
	rsfc   = keylist[rtime]->data.sfc;
	rlabs  = keylabs[rtime];
	rmplus = TimeList[rtime].mplus;
	tmplus = rmplus;
	for ( ; ltime<NumTime; ltime=rtime)
		{
		/* Duplicate keyframe if it coincides with a target time */
		if (rmplus == tmplus)
			{

#			ifdef DEBUG_INTERP
			if (minutes_in_depictions())
				printf("           T%s (copying)\n",
					hour_minute_string(0, tmplus));
			else
				printf("           T%+.2d (copying)\n",
					tmplus/60);
#			endif /* DEBUG_INTERP */

			if (minutes_in_depictions())
				put_message("spline-interp-mins",
							lev, elem, hour_minute_string(0, tmplus));
			else
				put_message("spline-interp",
							lev, elem, tmplus/60);

			/* Use the key frame in the corresponding output frame */
			itween = (tmplus - mfirst)/DTween;
			interp_progress(dfld, itween, NumTween, -1, -1);
			sfc    = copy_surface(rsfc, TRUE);
			labs   = copy_set(rlabs);
			genlist[itween] = create_field(ent, elem, lev);
			define_fld_data(genlist[itween], "surface", (POINTER)sfc);
			genlabs[itween] = labs;
			if (show)
				{
				if	( NotNull(sfc)
					  && ( IsNull(sfc->patches)
						   || IsNull(sfc->patches[0][0])
						   || !sfc->patches[0][0]->defined
						   || IsNull(sfc->patches[0][0]->contours)
						 )
					) contour_surface(sfc);
				fld = create_field(ent, elem, lev);
				define_fld_data(fld, "surface", (POINTER)sfc);
				add_field_to_metafile(BlankMeta, fld);
				if (labs)
					{
					fld = create_field("d", elem, lev);
					define_fld_data(fld, "set", (POINTER)labs);
					add_field_to_metafile(BlankMeta, fld);
					}
				present_blank(TRUE);
				}
			tmplus += DTween;
			}

		/* Set up description of key window */
		if (ltime >= NumTime-1) break;
		lsfc   = rsfc;
		llabs  = rlabs;
		lmplus = rmplus;

		/* Find the next active keyframe */
		for (rtime=ltime+1; rtime<NumTime; rtime++)
			{
			if (dfld->frames[rtime].meta) break;
			}
		if (rtime >= NumTime) break;
		rsfc   = keylist[rtime]->data.sfc;
		rlabs  = keylabs[rtime];
		rmplus = TimeList[rtime].mplus;

		/* Set up displacements across key window */
		ndspl = 0;
		dxbar = 0;
		dybar = 0;
		gtol  = lsfc->sp.gridlen;
		for (ichain=0; ichain<nchain; ichain++)
			{
			chain = dfld->chains[ichain];
			/* >>>>> replace this ...
			copy_point(lpos, chain->nodes[ltime]->node);
			copy_point(rpos, chain->nodes[rtime]->node);
			... with this <<<<< */
			lnode = which_lchain_node(chain, LchainNode, TimeList[ltime].mplus);
			rnode = which_lchain_node(chain, LchainNode, TimeList[rtime].mplus);
			if (lnode < 0 || rnode < 0)         continue;
			if (!chain->nodes[lnode]->there)    continue;
			if (!chain->nodes[rnode]->there)    continue;
			copy_point(lpos, chain->nodes[lnode]->node);
			copy_point(rpos, chain->nodes[rnode]->node);
			if (!inside_dn_window(DnMap, lpos)) continue;
			if (!inside_dn_window(DnMap, rpos)) continue;
			dx = rpos[X] - lpos[X];
			dy = rpos[Y] - lpos[Y];
			mx = (rpos[X] + lpos[X]) / 2;
			my = (rpos[Y] + lpos[Y]) / 2;

			/* Need to spread out long displacements */
			ds = hypot(dx, dy);
			ns = ds/gtol;
			for ( ; ns>0; ns--)
				{
				/* Work from outer edges toward centre */
				s    = ns*gtol;
				step = s/ds;
				fact = 1 - s/(ds+gtol);
				ddx  = fact*dx;
				ddy  = fact*dy;

				/* Displacement behind */
				dxbar += ddx;
				dybar += ddy;
				idspl  = ndspl++;
				if (ndspl > mdspl)
					{
					mdspl += nchain;
					dpos   = GETMEM(dpos, POINT, mdspl);
					dspl   = GETMEM(dspl, POINT, mdspl);
					}
				dpos[idspl][X] = mx - dx*step;
				dpos[idspl][Y] = my - dy*step;
				dspl[idspl][X] = ddx;
				dspl[idspl][Y] = ddy;

				/* Displacement ahead */
				dxbar += ddx;
				dybar += ddy;
				idspl  = ndspl++;
				if (ndspl > mdspl)
					{
					mdspl += nchain;
					dpos   = GETMEM(dpos, POINT, mdspl);
					dspl   = GETMEM(dspl, POINT, mdspl);
					}
				dpos[idspl][X] = mx + dx*step;
				dpos[idspl][Y] = my + dy*step;
				dspl[idspl][X] = ddx;
				dspl[idspl][Y] = ddy;
				}

			/* Place main displacement at centre */
			dxbar += dx;
			dybar += dy;
			idspl  = ndspl++;
			if (ndspl > mdspl)
				{
				mdspl += nchain;
				dpos   = GETMEM(dpos, POINT, mdspl);
				dspl   = GETMEM(dspl, POINT, mdspl);
				}
			dpos[idspl][X] = mx;
			dpos[idspl][Y] = my;
			dspl[idspl][X] = dx;
			dspl[idspl][Y] = dy;
			}

		/* Initialize displacement grids to average values */
		if (ndspl > 0)
			{
			dxbar /= (float) ndspl;
			dybar /= (float) ndspl;
			}
		for (ix=0; ix<nx; ix++)
			for (iy=0; iy<ny; iy++)
				{
				gridx[iy][ix] = dxbar;
				gridy[iy][ix] = dybar;
				}

		/* Generate flat displacement surfaces to fit grids */
		grid_surface(dxsfc, glen, nx, ny, gridx);
		grid_surface(dysfc, glen, nx, ny, gridy);

		/* Fit displacement surfaces to actual displacements */
		/* May have to repeat this several times if points are close */
		for (idspl=0; idspl<ndspl; idspl++)
			{
			edit_surface(dxsfc, dpos[idspl], dspl[idspl][X], TRUE, FALSE);
			edit_surface(dysfc, dpos[idspl], dspl[idspl][Y], TRUE, FALSE);
			}

		/* Interpolate output frames between current and next key frames */
		tlen = (float) (rmplus - lmplus);

		for ( ; tmplus<rmplus; tmplus+=DTween)
			{

#			ifdef DEBUG_INTERP
			if (minutes_in_depictions())
				printf("           T%s (interpolating)\n",
					hour_minute_string(0, tmplus));
			else
				printf("           T%+.2d (interpolating)\n",
					tmplus/60);
#			endif /* DEBUG_INTERP */

			if (minutes_in_depictions())
				put_message("spline-interp-mins",
							lev, elem, hour_minute_string(0, tmplus));
			else
				put_message("spline-interp",
							lev, elem, tmplus/60);

			itween = (tmplus - mfirst)/DTween;
			interp_progress(dfld, itween, NumTween, -1, -1);
			tstep  = (float) (tmplus - lmplus);
			lfact  = tstep / tlen;
			rfact  = 1 - lfact;

			/* Fill grid with weighted average of forward and backward */
			/* displaced values */
			for (ix=0; ix<nx; ix++)
				{
				gpos[X] = ix * glen;
				for (iy=0; iy<ny; iy++)
					{
					gpos[Y] = iy * glen;
					valid   = eval_sfc(dxsfc, gpos, &dxval);
					valid   = eval_sfc(dysfc, gpos, &dyval);
					lpos[X] = gpos[X] - dxval*lfact;
					lpos[Y] = gpos[Y] - dyval*lfact;
					rpos[X] = gpos[X] + dxval*rfact;
					rpos[Y] = gpos[Y] + dyval*rfact;
					lpos[X] = MAX(lpos[X], 0.);	lpos[X] = MIN(lpos[X], xlim);
					lpos[Y] = MAX(lpos[Y], 0.);	lpos[Y] = MIN(lpos[Y], ylim);
					rpos[X] = MAX(rpos[X], 0.);	rpos[X] = MIN(rpos[X], xlim);
					rpos[Y] = MAX(rpos[Y], 0.);	rpos[Y] = MIN(rpos[Y], ylim);
					valid   = eval_sfc_UV(lsfc, lpos, &luval, &lvval);
					valid   = eval_sfc_UV(rsfc, rpos, &ruval, &rvval);
					gridu[iy][ix] = (float) (rfact*luval + lfact*ruval);
					gridv[iy][ix] = (float) (rfact*lvval + lfact*rvval);
					}
				}

			/* Fit new surface to the grid */
			sfc = copy_surface(lsfc, FALSE);
			grid_surface_2D(sfc, glen, nx, ny, gridu, gridv);
			genlist[itween] = create_field(ent, elem, lev);
			define_fld_data(genlist[itween], "surface", (POINTER)sfc);

			/* Interpolate labels ahead from previous frame */
			/* or back from following frame */
			labs = NullSet;
			if (lfact<=0.5)
				{
				if (NotNull(llabs)) labs = copy_set(llabs);
				fact = lfact;
				}
			else
				{
				if (NotNull(rlabs)) labs = copy_set(rlabs);
				fact = -rfact;
				}

			tsfc = NullSfc;
			if (NotNull(labs) || showtween)
				{
				/* Need to contour the surface for interpolating labels */
				/* or displaying */
				tsfc = (showtween)? sfc: copy_surface(sfc, FALSE);
				contour_surface(tsfc);
				}

			if (NotNull(labs))
				{
				genlabs[itween] = labs;
				for (imem=0; imem<labs->num; imem++)
					{
					spot = (SPOT) labs->list[imem];
					if (IsNull(spot))                 continue;
					if (same(spot->mclass, "legend")) continue;
					if (same(spot->mclass, "TSPEC"))  continue;

					pos[X]  = spot->anchor[X];
					pos[Y]  = spot->anchor[Y];
					valid   = eval_sfc(dxsfc, pos, &dxval);
					valid   = eval_sfc(dysfc, pos, &dyval);
					gpos[X] = pos[X] + dxval*fact;
					gpos[Y] = pos[Y] + dyval*fact;
					copy_point(spot->anchor, gpos);
					}
				recompute_surface_labs(tsfc, labs, FALSE);
				if (!showtween) tsfc = destroy_surface(tsfc);
				}

			if (showtween)
				{
				fld = create_field(ent, elem, lev);
				define_fld_data(fld, "surface", (POINTER)sfc);
				add_field_to_metafile(BlankMeta, fld);
				if (labs)
					{
					fld = create_field("d", elem, lev);
					define_fld_data(fld, "set", (POINTER)labs);
					add_field_to_metafile(BlankMeta, fld);
					}
				present_blank(TRUE);
				}

			}   /* Next output frame (tmplus) */

		}	/* Next key frame (ltime) */

	/* Free temporary buffers */
	dxsfc = destroy_surface(dxsfc);
	dysfc = destroy_surface(dysfc);
	FREEMEM(dpos);
	FREEMEM(dspl);
	FREEMEM(gbufx);
	FREEMEM(gbufy);
	FREEMEM(gbufu);
	FREEMEM(gbufv);
	FREEMEM(gridx);
	FREEMEM(gridy);
	FREEMEM(gridu);
	FREEMEM(gridv);

#	ifdef DEVELOPMENT
	if (dfld->reported)
		pr_info("Editor.Reported",
			"In interp_spline_2D() - T %s %s\n", dfld->element, dfld->level);
	else
		pr_info("Editor.Reported",
			"In interp_spline_2D() - F %s %s\n", dfld->element, dfld->level);
#	endif /* DEVELOPMENT */

	/* Restore graphics and reset flags */
	dfld->interp   = TRUE;
	dfld->intlab   = TRUE;
	dfld->saved    = FALSE;
	dfld->reported = FALSE;
	link_status(dfld);
	(void) save_links();
	for (ifld=0; ifld<BlankMeta->numfld; ifld++)
		{
		fld = BlankMeta->fields[ifld];
		switch (fld->ftype)
			{
			case FtypeSfc:	fld->data.sfc  = NullSfc;
							break;
			case FtypeSet:	fld->data.set  = NullSet;
							break;
			case FtypePlot:	fld->data.plot = NullPlot;
							break;
			}
		}
	empty_blank();
	busy_cursor(FALSE);
	return TRUE;
	}
