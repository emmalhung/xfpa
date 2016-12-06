/*********************************************************************/
/** @file surface_cont.c
 *
 * Routines to handle the SURFACE object.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*      s u r f a c e _ c o n t . c                                     *
*                                                                      *
*      Routines to handle the SURFACE object.                          *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2002 Environment Canada (MSC)            *
*     Version 7 (c) Copyright 2006 Environment Canada                  *
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

#include "surface.h"
#include "set_oper.h"

#include <tools/tools.h>
#include <fpa_getmem.h>

#include <stdio.h>

/* Set various debug modes */
#undef DEBUG_SFC
#undef DEBUG_PATCH
#undef DEBUG_ILIST
#undef DEBUG_BAND
#undef BAND_TEST

/***********************************************************************
*                                                                      *
*      r e s e t _ s u r f a c e _ m a s k                             *
*      a d d _ s u r f a c e _ m a s k                                 *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Reset the masking of contours in the surface.
 *	@param[in] 	sfc		surface to be modified
 *********************************************************************/

void	reset_surface_mask

	(
	SURFACE	sfc
	)

	{
	int		iu, nu, iv, nv;

	/* Do nothing if surface undefined */
	if (!sfc) return;

	nu = sfc->sp.m - ORDER + 1;
	nv = sfc->sp.n - ORDER + 1;
	for (iu=0; iu<nu; iu++)
		for (iv=0; iv<nv; iv++)
			{
			reset_patch_mask(sfc->patches[iu][iv]);
			}
	}

/**********************************************************************/

/*********************************************************************/
/** Add masking of contours in the surface.
 *
 *	@param[in] 	sfc			surface to be modified
 *	@param[in] 	set			area set containing mask information
 *	@param[in] 	usub		background sub-element if not defined
 *	@param[in] 	subelem		sub-element of set corresponding to mask
 *	@param[in] 	mode		"include" or "exclude" masked areas
 *	@param[in] 	rule		"and", "or" or "xor" mask areas with existing mask
 *********************************************************************/
void	add_surface_mask

	(
	SURFACE	sfc,
	SET		set,
	STRING	usub,
	STRING	subelem,
	STRING	mode,
	STRING	rule
	)

	{
	int		iu, nu, iv, nv;

	/* Do nothing if surface undefined */
	if (!sfc) return;

	nu = sfc->sp.m - ORDER + 1;
	nv = sfc->sp.n - ORDER + 1;
	for (iu=0; iu<nu; iu++)
		for (iv=0; iv<nv; iv++)
			{
			add_patch_mask(sfc->patches[iu][iv], set, usub, subelem, mode,
							rule);
			}
	}

/***********************************************************************
*                                                                      *
*      c o n t o u r _ s u r f a c e                                   *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Contour all the patches on the given surface.
 *
 *	@param[in] 	sfc		surface to be contoured
 *********************************************************************/

void	contour_surface

	(
	SURFACE	sfc
	)

	{
	int		ipl, ipr, ipb, ipt;

	/* Do nothing if surface undefined */
	if (!sfc) return;

	/* Determine whole range of contourable patches, then go */
	/* one patch past each edge to force the edge intersection */
	/* lists to be re-computed */
	ipl = -1;
	ipr = sfc->nupatch;
	ipb = -1;
	ipt = sfc->nvpatch;

	/* Do the contouring */
	MMM_begin_count();
	contour_surface_partial(sfc,ipl,ipr,ipb,ipt);
	MMM_report_count("After Contour");
	}


/*********************************************************************/
/** Produce banded contour for all the patches on the given surface.
 *
 *	@param[in]  sfc		surface to be contoured
 *********************************************************************/
void	band_contour_surface

	(
	SURFACE sfc
	)

	{
#ifdef BAND_TEST
	LOGICAL	valid;
	AREA	area;
	FSTYLE	fill;
	COLOUR	colour;
	char	cname[15];
	SET		bands;
	static	float	lval[] = { -10, -5, 0,  5, 10, 15, 20, 25 };
	static	float	uval[] = {  -5,  0, 5, 10, 15, 20, 25, 30 };
	static	int		nlev    = 8;
	int		ilev, imem;
#endif

	/* Do nothing if surface undefined */
	if (!sfc) return;

	if (!same(sfc->units.name, "degreesC")) return;

	if (NotNull(sfc->bands)) sfc->bands = destroy_set(sfc->bands);
#ifdef BAND_TEST
	fill = find_fstyle("solid_fill", &valid);

	for (ilev=0; ilev<nlev; ilev++)
		{
		(void) sprintf(cname, "#%X%X%X", ilev*2, 5, (15-ilev*2));
		colour = find_colour(cname, &valid);

		bands = contour_areaset(sfc, lval[ilev], uval[ilev],
					&sfc->units, NullBox);
		if (NotNull(bands))
			{
			for (imem=0; imem<bands->num; imem++)
				{
				area = (AREA) bands->list[imem];
				if (!area) continue;
				define_lspec(&area->lspec, 0, 0, NULL, FALSE, 0.0, 0.0, -1);
				define_fspec(&area->fspec, colour, fill, NULL, FALSE, FALSE,
							0.0, 0.0, 0);
				}
			}
		if (IsNull(sfc->bands)) sfc->bands = bands;
		else
			{
			append_set(sfc->bands, bands);
			destroy_set(bands);
			}
		}
#endif
	}

/***********************************************************************
*                                                                      *
*      c o n t o u r _ s u r f a c e _ p a r t i a l                   *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Contour the specified range of patches on the given surface.
 *
 *	@param[in] 	sfc	 surface to be recontoured
 *	@param[in] 	ipl	 left-most patch index
 *	@param[in] 	ipr	 right-most patch index
 *	@param[in] 	ipb	 bottom-most patch index
 *	@param[in] 	ipt	 top-most patch index
 *********************************************************************/

void	contour_surface_partial

	(
	SURFACE	sfc,
	int		ipl,
	int		ipr,
	int		ipb,
	int		ipt
	)

	{
	int		jul, jur, jub, jut;
	int		jvl, jvr, jvb, jvt;
	int		iup, ivp, iuw, ivw;
	PATCH	cpatch;
	ILIST	list, llist, rlist, blist, tlist;
	BIPOLY	*pfunc;

#	ifdef DEBUG_SFC
	long	nsec, nusec;
#	endif /* DEBUG_SFC */

	/* Do nothing if surface undefined */
	if (!sfc) return;

#	ifdef DEBUG_SFC
	set_stopwatch(TRUE);
#	endif /* DEBUG_SFC */

	/* Restrict affected area to contourable patches */
	/* Note: jul, jur, etc refer to intersection lists ... */
	/*	Normally it is not necessary to re-compute intersections */
	/*	on the boundary of the affected area, but we need to be */
	/*	aware of when the affected area extends past the edge of */
	/*	the contourable world. */
	jul = MAX(ipl,0);	jur = MIN(ipr,sfc->nupatch-1);
	jub = MAX(ipb,0);	jut = MIN(ipt+1,sfc->nvpatch);
/*	jub = MAX(ipb+1,0);	jut = MIN(ipt,sfc->nvpatch);	*/

/*	jvl = MAX(ipl+1,0);	jvr = MIN(ipr,sfc->nupatch);	*/
	jvl = MAX(ipl,0);	jvr = MIN(ipr+1,sfc->nupatch);
	jvb = MAX(ipb,0);	jvt = MIN(ipt,sfc->nvpatch-1);

	ipl = MAX(ipl,0);	ipr = MIN(ipr,sfc->nupatch-1);
	ipb = MAX(ipb,0);	ipt = MIN(ipt,sfc->nvpatch-1);

	/* Regenerate the patch functions in the changed area */
#	ifdef DEBUG_PATCH
	(void) printf("initializing patches [%d:%d][%d:%d]\n",ipl,ipr,ipb,ipt);
#	endif /* DEBUG_PATCH */
	patch_control((int) sfc->nupatch, (int) sfc->nvpatch, sfc->sp.gridlen);
	redefine_surface_patches(sfc,ipl,ipr,ipb,ipt, TRUE);

	/* Recompute u-intersection lists in the changed area */
#	ifdef DEBUG_ILIST
	(void) printf("computing u-ilists [%d:%d][%d:%d]\n",jul,jur,jub,jut);
#	endif /* DEBUG_ILIST */
	for (iup=jul; iup<=jur; iup++)
		{
		for (ivp=jub; ivp<=jut-1; ivp++)
			{
			list  = prepare_sfc_ulist(sfc, iup, ivp);
#			ifdef DEBUG_ILIST
			(void) printf("  ulist[%d][%d]: %x\n",iup,ivp,list);
#			endif /* DEBUG_ILIST */
			pfunc = &sfc->patches[iup][ivp]->function;
			compute_ilist(list,pfunc,'y',0.,sfc->ncspec,sfc->cspecs);
			}
		list  = prepare_sfc_ulist(sfc, iup, jut);
#		ifdef DEBUG_ILIST
		(void) printf("  ulist[%d][%d]: %x\n",iup,jut,list);
#		endif /* DEBUG_ILIST */
		pfunc = &sfc->patches[iup][jut-1]->function;
		compute_ilist(list,pfunc,'y',1.,sfc->ncspec,sfc->cspecs);
		}

	/* Recompute v-intersection lists in the changed area */
#	ifdef DEBUG_ILIST
	(void) printf("computing v-ilists [%d:%d][%d:%d]\n",jvl,jvr,jvb,jvt);
#	endif /* DEBUG_ILIST */
	for (ivp=jvb; ivp<=jvt; ivp++)
		{
		for (iup=jvl; iup<=jvr-1; iup++)
			{
			list  = prepare_sfc_vlist(sfc, iup, ivp);
#			ifdef DEBUG_ILIST
			(void) printf("  vlist[%d][%d]: %x\n",iup,ivp,list);
#			endif /* DEBUG_ILIST */
			pfunc = &sfc->patches[iup][ivp]->function;
			compute_ilist(list,pfunc,'x',0.,sfc->ncspec,sfc->cspecs);
			}
		list  = prepare_sfc_vlist(sfc, jvr, ivp);
#		ifdef DEBUG_ILIST
		(void) printf("  vlist[%d][%d]: %x\n",jvr,ivp,list);
#		endif /* DEBUG_ILIST */
		pfunc = &sfc->patches[jvr-1][ivp]->function;
		compute_ilist(list,pfunc,'x',1.,sfc->ncspec,sfc->cspecs);
		}

	/* Re-contour the patches in the changed area */
#	ifdef DEBUG_PATCH
	(void) printf("contouring patches [%d:%d][%d:%d]\n",ipl,ipr,ipb,ipt);
#	endif /* DEBUG_PATCH */
	iuw = -1;
	ivw = -1;
	for (iup=ipl; iup<=ipr; iup++)
		{
		for (ivp=ipb; ivp<=ipt; ivp++)
			{
#			ifdef DEBUG_PATCH
			(void) printf("  patch[%d][%d]\n",iup,ivp);
#			endif /* DEBUG_PATCH */
			if (iup==iuw && ivp==ivw)
				{
				/* Spot to catch the debugger at the desired patch */
				iuw = -1;
				ivw = -1;
				}
			set_patch_index(iup, ivp);
			cpatch = sfc->patches[iup][ivp];
			llist  = sfc->vlist[iup][ivp];
			rlist  = sfc->vlist[iup+1][ivp];
			blist  = sfc->ulist[iup][ivp];
			tlist  = sfc->ulist[iup][ivp+1];
#			ifdef DEBUG_ILIST
			(void) printf("  patch[%d][%d]  llist/rlist: %x %x  blist/tlist: %x %x\n",
				iup,ivp,llist,rlist,blist,tlist);
#			endif /* DEBUG_ILIST */
			contour_patch(cpatch, llist, rlist, blist, tlist,
							sfc->ncspec, sfc->cspecs);
			do_patch_vectors(cpatch, iup, ivp, sfc->ncspec, sfc->cspecs,
							sfc->units.factor, sfc->units.offset);
			}
		}

#	ifdef DEBUG_SFC
	get_stopwatch(&nsec, &nusec, NULL, NULL);
	pr_diag("Contouring", "Time: %d.%.6d sec\n", nsec, nusec);
#	endif /* DEBUG_SFC */
	}

/***********************************************************************
*                                                                      *
*      r e d e f i n e _ s u r f a c e _ p a t c h e s                 *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Re-define the given block of patches.
 *
 *	@param[in] 	sfc		surface to be recontoured
 *	@param[in] 	ipl		left-most patch index
 *	@param[in] 	ipr		right-most patch index
 *	@param[in] 	ipb		bottom-most patch index
 *	@param[in] 	ipt		top-most patch index
 *	@param[in] 	force	force allocation regardless of MMM
 *********************************************************************/

void	redefine_surface_patches

	(
	SURFACE	sfc,
	int		ipl,
	int		ipr,
	int		ipb,
	int		ipt,
	LOGICAL	force
	)

	{
	int		iup, ivp;

	/* Do nothing if surface undefined */
	if (!sfc) return;

	/* Stay within the reality range */
	ipl = MAX(ipl,0);	ipr = MIN(ipr,sfc->nupatch-1);
	ipb = MAX(ipb,0);	ipt = MIN(ipt,sfc->nvpatch-1);

	/* Re-define the patches */
	for (iup=ipl; iup<=ipr; iup++)
		for (ivp=ipb; ivp<=ipt; ivp++)
			{
			redef_sfc_patch(sfc, iup, ivp, force);
			}
	}

/***********************************************************************
*                                                                      *
*      f i n d _ s u r f a c e _ r a n g e                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Calculate the max and min values on the given surface.
 *
 *	@param[in] 	sfc		surface used
 *	@param[out]	*vmin	minimum value on surface
 *	@param[out]	*vmax	maximum value on surface
 *********************************************************************/

void	find_surface_range

	(
	SURFACE	sfc,
	float	*vmin,
	float	*vmax
	)

	{
	float	xmin, xmax, bmin, bmax;
	int		nu, nv, iup, ivp;
	BIPOLY	*pfunc;
	UNIPOLY	proj;
	POINT	p;

	/* Do nothing if surface undefined */
	if (vmin) *vmin = 0.0;
	if (vmax) *vmax = 0.0;
	if (IsNull(sfc)) return;

	/* Define the patches if necessary */
	nu = sfc->nupatch;
	nv = sfc->nvpatch;
	if (nu<=0 || nv<=0 || IsNull(sfc->patches)) return;
	patch_control(nu, nv, sfc->sp.gridlen);
	for (iup=0; iup<nu; iup++)
		for (ivp=0; ivp<nv; ivp++)
			{
			(void) prepare_sfc_patch(sfc, iup, ivp);
			}

	copy_point(p, ZeroPoint);
	pfunc = &sfc->patches[0][0]->function;
	bmin  = bmax = evaluate_bipoly(pfunc, p);

	/* Find range in u-intersection edges */
	for (iup=0; iup<nu; iup++)
		{
		for (ivp=0; ivp<nv; ivp++)
			{
			pfunc = &sfc->patches[iup][ivp]->function;
			project_bipoly(pfunc, &proj, 'y', 0.);
			find_unipoly_range(&proj, 0., 1., &xmin, &xmax);
			bmin = MIN(bmin, xmin);
			bmax = MAX(bmax, xmax);
			}
		pfunc = &sfc->patches[iup][nv-1]->function;
		project_bipoly(pfunc, &proj, 'y', 1.);
		find_unipoly_range(&proj, 0., 1., &xmin, &xmax);
		bmin = MIN(bmin, xmin);
		bmax = MAX(bmax, xmax);
		}

	/* Find range in v-intersection edges */
	for (ivp=0; ivp<nv; ivp++)
		{
		for (iup=0; iup<nu; iup++)
			{
			pfunc = &sfc->patches[iup][ivp]->function;
			project_bipoly(pfunc, &proj, 'x', 0.);
			find_unipoly_range(&proj, 0., 1., &xmin, &xmax);
			bmin = MIN(bmin, xmin);
			bmax = MAX(bmax, xmax);
			}
		pfunc = &sfc->patches[nu-1][ivp]->function;
		project_bipoly(pfunc, &proj, 'x', 1.);
		find_unipoly_range(&proj, 0., 1., &xmin, &xmax);
		bmin = MIN(bmin, xmin);
		bmax = MAX(bmax, xmax);
		}

	for (iup=0; iup<nu; iup++)
		for (ivp=0; ivp<nv; ivp++)
			{
			(void) dispose_sfc_patch(sfc, iup, ivp);
			}

	if (vmin) *vmin = bmin;
	if (vmax) *vmax = bmax;
	}

/***********************************************************************
*                                                                      *
*    c o n t o u r _ c u r v e s e t                                   *
*                                                                      *
***********************************************************************/

#define	EntryEdge	"EntryEdge"
#define	ExitEdge	"ExitEdge"
#define	EdgeLeft	"EdgeLeft"
#define	EdgeTop		"EdgeTop"
#define	EdgeRight	"EdgeRight"
#define	EdgeBottom	"EdgeBottom"
#define	EdgeClosed	"EdgeClosed"

static	STRING	follow_contour(SURFACE, int, int, int, CURVE);
static	int		find_contour(SURFACE, int, int, POINT);
static	LOGICAL	exit_at_edge(CURVE);

static	float	XSl, YSt, XSr, YSb;

static	LOGICAL	CurveErrorFound = FALSE;
LOGICAL	contour_curveset_failure(void)
	{
	if (!CurveErrorFound) return FALSE;
	CurveErrorFound = FALSE;
	return TRUE;
	}

/*********************************************************************/
/** Convert contours of a given value to a set of CURVES.
 *
 *	@param[in] 	sfc		surface containing contours
 *	@param[in] 	cval	value of desired contour
 *	@param[in] 	*units	units for value (optional)
 *  @return Pointer to a set of curves. You will need to destroy this
 * 			object when you are finished with it. (destroy_set)
 *********************************************************************/
SET		contour_curveset

	(
	SURFACE	sfc,
	float	cval,
	USPEC	*units
	)

	{
	int		nup, nvp, iup, ivp, ic;
	LOGICAL	closed;
	float	size;
	POINT	pos;
	STRING	exitedge;
	SURFACE	scopy;
	CONSPEC	cspec;
	PATCH	patch;
	SET		pcont, wcont;
	CURVE	pcurv, wcurv;
	LINE	pline;

	/* Do nothing if nothing given */
	if (!sfc)                    return NullSet;
	if ((nup=sfc->nupatch) <= 0) return NullSet;
	if ((nvp=sfc->nvpatch) <= 0) return NullSet;

	/* Determine the limits of the surface */
	patch_to_world(&sfc->sp, make_point(0., 0.), 0, 0, pos);
	XSl = pos[X];
	YSb = pos[Y];
	patch_to_world(&sfc->sp, make_point(1., 1.), nup-1, nvp-1, pos);
	XSr = pos[X];
	YSt = pos[Y];

	/* Copy the surface without any existing contours */
	scopy = copy_surface(sfc, FALSE);

	/* Change the units if necessary */
	if (units) change_surface_units(scopy, units);

	/* Set the contour specs for the given contour value */
	init_conspec(&cspec);
	define_conspec_list(&cspec, 0, NULL, NULL);
	add_cval_to_conspec(&cspec, cval, NULL);
	define_surface_conspecs(scopy, 1, &cspec);
	free_conspec(&cspec);

	/* Generate the contours */
	contour_surface(scopy);

	/* Create a curveset */
	wcont = create_set("curve");

	/* Search for all open contours that start along the left edge */
	iup = 0;
	for (ivp=0; ivp<nvp; ivp++)
		{
		/* Obtain contour segments for this patch */
		patch = scopy->patches[iup][ivp];
		if (!patch)          continue;
		pcont = patch->contours;
		if (!pcont)          continue;
		if (pcont->num <= 0) continue;

		/* Find contour segments that start at the left edge */
		for (ic=0; ic<pcont->num; ic++)
			{
			pcurv = (CURVE) pcont->list[ic];
			if (!pcurv)             continue;
			pline = pcurv->line;
			if (!pline)             continue;
			if (pline->numpts <= 1) continue;

			/* Track any contour segments that start at x=0 */
			if (pline->points[0][X] != 0.0) continue;

			/* Got one that starts at the left edge */
			/* Follow the contour through neighbouring patches */
			wcurv = create_curve("", "", "");
			(void) add_attribute(wcurv->attrib, EntryEdge, EdgeLeft);
			exitedge = follow_contour(scopy, iup, ivp, ic, wcurv);
			pr_diag("Contouring", "Contour entry: %s  Contour exit: %s\n",
					EdgeLeft, exitedge);
			if (blank(exitedge))
				{
				pr_error("Contouring", "HELP!!! No exit point.\n");
				CurveErrorFound = TRUE;
				(void) destroy_curve(wcurv);
				}
			else
				{
				condense_line(wcurv->line);
				if (!exit_at_edge(wcurv))
					{
					pr_error("Contouring",
						"HELP!!! Line enters but does not exit at edge.\n");
					CurveErrorFound = TRUE;
					}
				(void) add_attribute(wcurv->attrib, ExitEdge, exitedge);
				add_item_to_set(wcont, (ITEM) wcurv);
				}
			}
		}

	/* Search for all open contours that start along the top edge */
	ivp = nvp-1;
	for (iup=0; iup<nup; iup++)
		{
		/* Obtain contour segments for this patch */
		patch = scopy->patches[iup][ivp];
		if (!patch)          continue;
		pcont = patch->contours;
		if (!pcont)          continue;
		if (pcont->num <= 0) continue;

		/* Find contour segments that start at the top edge */
		for (ic=0; ic<pcont->num; ic++)
			{
			pcurv = (CURVE) pcont->list[ic];
			if (!pcurv)             continue;
			pline = pcurv->line;
			if (!pline)             continue;
			if (pline->numpts <= 1) continue;

			/* Track any contour segments that start at y=1 */
			if (pline->points[0][Y] != 1.0) continue;

			/* Got one that starts at the top edge */
			/* Follow the contour through neighbouring patches */
			wcurv = create_curve("", "", "");
			(void) add_attribute(wcurv->attrib, EntryEdge, EdgeTop);
			exitedge = follow_contour(scopy, iup, ivp, ic, wcurv);
			pr_diag("Contouring", "Contour entry: %s  Contour exit: %s\n",
					EdgeTop, exitedge);
			if (blank(exitedge))
				{
				pr_error("Contouring", "HELP!!! No exit point.\n");
				CurveErrorFound = TRUE;
				(void) destroy_curve(wcurv);
				}
			else
				{
				condense_line(wcurv->line);
				if (!exit_at_edge(wcurv))
					{
					pr_error("Contouring",
						"HELP!!! Line enters but does not exit at edge.\n");
					CurveErrorFound = TRUE;
					}
				(void) add_attribute(wcurv->attrib, ExitEdge, exitedge);
				add_item_to_set(wcont, (ITEM) wcurv);
				}
			}
		}

	/* Search for all open contours that start along the right edge */
	iup = nup-1;
	for (ivp=nvp-1; ivp>=0; ivp--)
		{
		/* Obtain contour segments for this patch */
		patch = scopy->patches[iup][ivp];
		if (!patch)          continue;
		pcont = patch->contours;
		if (!pcont)          continue;
		if (pcont->num <= 0) continue;

		/* Find contour segments that start at the right edge */
		for (ic=0; ic<pcont->num; ic++)
			{
			pcurv = (CURVE) pcont->list[ic];
			if (!pcurv)             continue;
			pline = pcurv->line;
			if (!pline)             continue;
			if (pline->numpts <= 1) continue;

			/* Track any contour segments that start at x=1 */
			if (pline->points[0][X] != 1.0) continue;

			/* Got one that starts at the right edge */
			/* Follow the contour through neighbouring patches */
			wcurv = create_curve("", "", "");
			(void) add_attribute(wcurv->attrib, EntryEdge, EdgeRight);
			exitedge = follow_contour(scopy, iup, ivp, ic, wcurv);
			pr_diag("Contouring", "Contour entry: %s  Contour exit: %s\n",
					EdgeRight, exitedge);
			if (blank(exitedge))
				{
				pr_error("Contouring", "HELP!!! No exit point.\n");
				CurveErrorFound = TRUE;
				(void) destroy_curve(wcurv);
				}
			else
				{
				condense_line(wcurv->line);
				if (!exit_at_edge(wcurv))
					{
					pr_error("Contouring",
						"HELP!!! Line enters but does not exit at edge.\n");
					CurveErrorFound = TRUE;
					}
				(void) add_attribute(wcurv->attrib, ExitEdge, exitedge);
				add_item_to_set(wcont, (ITEM) wcurv);
				}
			}
		}

	/* Search for all open contours that start along the bottom edge */
	ivp = 0;
	for (iup=nup-1; iup>=0; iup--)
		{
		/* Obtain contour segments for this patch */
		patch = scopy->patches[iup][ivp];
		if (!patch)          continue;
		pcont = patch->contours;
		if (!pcont)          continue;
		if (pcont->num <= 0) continue;

		/* Find contour segments that start at the bottom edge */
		for (ic=0; ic<pcont->num; ic++)
			{
			pcurv = (CURVE) pcont->list[ic];
			if (!pcurv)             continue;
			pline = pcurv->line;
			if (!pline)             continue;
			if (pline->numpts <= 1) continue;

			/* Track any contour segments that start at y=0 */
			if (pline->points[0][Y] != 0.0) continue;

			/* Got one that starts at the bottom edge */
			/* Follow the contour through neighbouring patches */
			wcurv = create_curve("", "", "");
			(void) add_attribute(wcurv->attrib, EntryEdge, EdgeBottom);
			exitedge = follow_contour(scopy, iup, ivp, ic, wcurv);
			pr_diag("Contouring", "Contour entry: %s  Contour exit: %s\n",
					EdgeBottom, exitedge);
			if (blank(exitedge))
				{
				pr_error("Contouring", "HELP!!! No exit point.\n");
				CurveErrorFound = TRUE;
				(void) destroy_curve(wcurv);
				}
			else
				{
				condense_line(wcurv->line);
				if (!exit_at_edge(wcurv))
					{
					pr_error("Contouring",
						"HELP!!! Line enters but does not exit at edge.\n");
					CurveErrorFound = TRUE;
					}
				(void) add_attribute(wcurv->attrib, ExitEdge, exitedge);
				add_item_to_set(wcont, (ITEM) wcurv);
				}
			}
		}

	/* Now search for all the closed contours */
	for (iup=0; iup<nup; iup++)
		for (ivp=0; ivp<nvp; ivp++)
			{
			patch = scopy->patches[iup][ivp];
			if (!patch)          continue;
			pcont = patch->contours;
			if (!pcont)          continue;
			if (pcont->num <= 0) continue;

			/* Find contour segments that start at any interior edge */
			for (ic=0; ic<pcont->num; ic++)
				{
				pcurv = (CURVE) pcont->list[ic];
				if (!pcurv)             continue;
				pline = pcurv->line;
				if (!pline)             continue;
				if (pline->numpts <= 1) continue;

				/* Got one that starts at an interior edge */
				/* Follow the contour through neighbouring patches */
				wcurv = create_curve("", "", "");
				(void) add_attribute(wcurv->attrib, EntryEdge, EdgeClosed);
				exitedge = follow_contour(scopy, iup, ivp, ic, wcurv);
				pr_diag("Contouring", "Contour entry: %s  Contour exit: %s\n",
						EdgeClosed, exitedge);
				if (blank(exitedge))
					{
					pr_error("Contouring", "HELP!!! No exit point.\n");
					CurveErrorFound = TRUE;
					(void) destroy_curve(wcurv);
					}
				else
					{
					condense_line(wcurv->line);
					(void) add_attribute(wcurv->attrib, ExitEdge, EdgeClosed);
					line_properties(wcurv->line, &closed, NullChar, &size,
							NullFloat);
					/* Ensure that interior curves are closed */
					if (!closed)
						{
						pr_error("Contouring",
							"HELP!!! Interior line is not closed.\n");
						(void) destroy_curve(wcurv);
						CurveErrorFound = TRUE;
						}
					/* Skip curves that have too small an area */
					else if (size < 1.0e-06 )
						{
						pr_error("Contouring",
							"HELP!!! Closed interior line (%d points) with %f area.\n",
							wcurv->line->numpts, size);
						(void) destroy_curve(wcurv);
						}
					else if (size < 25.0 && looped_line_crossing(wcurv->line,
												NullPoint, NullInt, NullInt) )
						{
						pr_error("Contouring",
							"HELP!!! Interior crossover line (%d points) with %f area.\n",
							wcurv->line->numpts, size);
						(void) destroy_curve(wcurv);
						}
					else
						{
						add_item_to_set(wcont, (ITEM) wcurv);
						}
					}
				}
			}

	scopy = destroy_surface(scopy);
	return wcont;
	}

/**********************************************************************/

static	STRING	follow_contour

	(
	SURFACE	sfc,	/* surface containing contour */
	int		iup,	/* horizontal coordinate for patch */
	int		ivp,	/* vertical coordinate for patch */
	int		ic,		/* contour index */
	CURVE	wcurv	/* curve containing contour being followed */
	)

	{
	PATCH	patch;
	SET		pcont;
	CURVE	pcurv;
	LINE	pline;
	POINT	wpos, expos, enpos;
	LOGICAL	exl, exr, exb, ext;
	int		jup, jvp, jc, ip;

	/* Do nothing if surface undefined */
	if (!sfc)                return NullString;
	if (!wcurv)              return NullString;

	if (iup < 0)             return NullString;
	if (iup >= sfc->nupatch) return NullString;
	if (ivp < 0)             return NullString;
	if (ivp >= sfc->nvpatch) return NullString;

	patch = sfc->patches[iup][ivp];
	if (!patch)              return NullString;
	pcont = patch->contours;
	if (!pcont)              return NullString;
	if (pcont->num <= 0)     return NullString;

	if (ic < 0)              return NullString;
	if (ic >= pcont->num)    return NullString;

	pcurv = (CURVE) pcont->list[ic];
	if (!pcurv)              return NullString;
	pline = pcurv->line;
	if (!pline)              return NullString;
	if (pline->numpts <= 1)  return NullString;

	/* Remap the contour segment and add it to the world contour */
	for (ip=0; ip<pline->numpts; ip++)
		{
		patch_to_world(&sfc->sp, pline->points[ip], iup, ivp, wpos);
#		ifdef DEBUG_BAND
		pr_diag("Contouring",
			"  Adding patch pt: %d [%d:%d] %.2f %.2f (%.2f %.2f)\n",
				ip, iup, ivp, pline->points[ip][X], pline->points[ip][Y],
				wpos[X], wpos[Y]);
#		endif /* DEBUG_BAND */
		add_point_to_curve(wcurv, wpos);
		}

	/* Don't use this contour segment again */
	copy_point(expos, pline->points[pline->numpts-1]);
	remove_item_from_set(pcont, (ITEM) pcurv);

	/* Where does it exit */
	exl = (LOGICAL) (expos[X] <= 0.0);
	exr = (LOGICAL) (expos[X] >= 1.0);
	exb = (LOGICAL) (expos[Y] <= 0.0);
	ext = (LOGICAL) (expos[Y] >= 1.0);
	jc  = -1;

	if (!exl && !exr && !exb && !ext)
		pr_error("Contouring",
			"HELP!!! No exit point: %.5f %.5f\n", expos[X], expos[Y]);

	/* Exit top-left */
	if (jc<0 && exl && ext)
		{
		/* Check patch above and left */
		jup = iup - 1;
		jvp = ivp + 1;
		enpos[X] = 1.0;
		enpos[Y] = 0.0;
		jc = find_contour(sfc, jup, jvp, enpos);
		}

	/* Exit top-right */
	if (jc<0 && ext && exr)
		{
		/* Check patch above and right */
		jup = iup + 1;
		jvp = ivp + 1;
		enpos[X] = 0.0;
		enpos[Y] = 0.0;
		jc = find_contour(sfc, jup, jvp, enpos);
		}

	/* Exit bottom-left */
	if (jc<0 && exr && exb)
		{
		/* Check patch below and left */
		jup = iup - 1;
		jvp = ivp - 1;
		enpos[X] = 1.0;
		enpos[Y] = 1.0;
		jc = find_contour(sfc, jup, jvp, enpos);
		}

	/* Exit bottom-right */
	if (jc<0 && exb && exl)
		{
		/* Check patch below and right */
		jup = iup + 1;
		jvp = ivp - 1;
		enpos[X] = 0.0;
		enpos[Y] = 1.0;
		jc = find_contour(sfc, jup, jvp, enpos);
		}

	/* Exit left */
	if (jc<0 && exl)
		{
		/* Check patch to the left */
		jup = iup - 1;
		jvp = ivp;
		enpos[X] = 1.0;
		enpos[Y] = expos[Y];
		jc = find_contour(sfc, jup, jvp, enpos);
		}

	/* Exit top */
	if (jc<0 && ext)
		{
		/* Check patch above */
		jup = iup;
		jvp = ivp + 1;
		enpos[X] = expos[X];
		enpos[Y] = 0.0;
		jc = find_contour(sfc, jup, jvp, enpos);
		}

	/* Exit right */
	if (jc<0 && exr)
		{
		/* Check patch to the right */
		jup = iup + 1;
		jvp = ivp;
		enpos[X] = 0.0;
		enpos[Y] = expos[Y];
		jc = find_contour(sfc, jup, jvp, enpos);
		}

	/* Exit bottom */
	if (jc<0 && exb)
		{
		/* Check patch below */
		jup = iup;
		jvp = ivp - 1;
		enpos[X] = expos[X];
		enpos[Y] = 1.0;
		jc = find_contour(sfc, jup, jvp, enpos);
		}

	/* Iterate if an adjoining patch has the contour */
	if (jc >= 0) return follow_contour(sfc, jup, jvp, jc, wcurv);

	/* Otherwise ... return the exit edge */
	if      (exl) return EdgeLeft;
	else if (ext) return EdgeTop;
	else if (exr) return EdgeRight;
	else if (exb) return EdgeBottom;
	else          return NullString;
	}

/**********************************************************************/

static	int		find_contour

	(
	SURFACE	sfc,	/* surface containing contour */
	int		iup,	/* horizontal coordinate for patch */
	int		ivp,	/* vertical coordinate for patch */
	POINT	pos		/* position on endpoint of contour */
	)

	{
	PATCH	patch;
	SET		pcont;
	CURVE	pcurv;
	LINE	pline;
	int		ic;

	/* Do nothing if surface undefined */
	if (!sfc)                return -1;

	if (iup < 0)             return -1;
	if (iup >= sfc->nupatch) return -1;
	if (ivp < 0)             return -1;
	if (ivp >= sfc->nvpatch) return -1;

	patch = sfc->patches[iup][ivp];
	if (!patch)              return -1;
	pcont = patch->contours;
	if (!pcont)              return -1;
	if (pcont->num <= 0)     return -1;
	if (!pos)                return -1;

	for (ic=0; ic<pcont->num; ic++)
		{
		pcurv = (CURVE) pcont->list[ic];
		if (!pcurv)             continue;
		pline = pcurv->line;
		if (!pline)             continue;
		if (pline->numpts <= 1) continue;

		if (pline->points[0][X] != pos[X]) continue;
		if (pline->points[0][Y] != pos[Y]) continue;

		/* We found a contour that connects */
		return ic;
		}

	return -1;
	}

/**********************************************************************/

static	LOGICAL	exit_at_edge

	(
	CURVE	curve
	)

	{
	float	x, y;

	if (!curve)                  return FALSE;
	if (!curve->line)            return FALSE;
	if (curve->line->numpts < 2) return FALSE;

	x = curve->line->points[curve->line->numpts-1][X];
	y = curve->line->points[curve->line->numpts-1][Y];

	if (x <= XSl || x >= XSr) return TRUE;
	if (y <= YSb || y >= YSt) return TRUE;
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*    c o n t o u r _ a r e a s e t                                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Convert contours of two given values (lower and upper) to a set
 *  of closed AREAS with holes.
 *
 *	@param[in] 	sfc			surface containing contours
 *	@param[in] 	lower		lower value of contour range
 *	@param[in] 	upper		upper value of contour range
 *	@param[in] 	*units		units for lower/upper values (optional)
 *	@param[in] 	*limits		limiting box (optional)
 *	@return a setup of areas based on contours.
 *********************************************************************/

SET		contour_areaset

	(
	SURFACE	sfc,
	float	lower,
	float	upper,
	USPEC	*units,
	BOX		*limits
	)

	{
	SET		lcurves, ucurves, areas;
	POINT	pos;
	float	aval, lval, uval;
	int		nup, nvp, iloop;
	LOGICAL	lerror, uerror, aerror;
	BOX		limbox;

	/* Do nothing if surface undefined */
	if (!sfc)                    return NullSet;
	if ((nup=sfc->nupatch) <= 0) return NullSet;
	if ((nvp=sfc->nvpatch) <= 0) return NullSet;

	/* Check for contour limits */
	if ( lower >= upper )
		{
		pr_error("Contouring",
			"Attempt to create contour area from  %g to %g!\n", lower, upper);
		return NullSet;
		}

	/* Determine the bounding box */
	patch_to_world(&sfc->sp, make_point(0., 0.), 0, 0, pos);
	limbox.left   = pos[X];
	limbox.bottom = pos[Y];
	patch_to_world(&sfc->sp, make_point(1., 1.), nup-1, nvp-1, pos);
	limbox.right  = pos[X];
	limbox.top    = pos[Y];
	if (NotNull(limits))
		{
		/* >>> Limit box not yet supported! <<< */
		pr_warning("Limits", "Limit box not yet supported!\n");
		/*
		limbox.left   = MAX(limbox.left,   limits->left);
		limbox.bottom = MAX(limbox.bottom, limits->bottom);
		limbox.right  = MIN(limbox.right,  limits->right);
		limbox.top    = MIN(limbox.top,    limits->top);
		*/
		/* >>> Limit box not yet supported! <<< */
		}

	/* Enter loop for generating areas from curves */
	/* We use a loop in case any of the sections fails! */
	lval  = lower;
	uval  = upper;
	aval  = (upper - lower) / 1000.0;
	iloop = 0;
	while (TRUE)
		{

		/* Generate curves for both limits */
		(void) contour_curveset_failure();
		lcurves = contour_curveset(sfc, lval, units);
		lerror  = contour_curveset_failure();
		ucurves = contour_curveset(sfc, uval, units);
		uerror  = contour_curveset_failure();

		/* Convert the curves into areas */
		(void) contour_areaset_failure();
		areas   = contour_areaset_from_curves(lcurves, ucurves, &limbox);
		aerror  = contour_areaset_failure();
		lcurves = destroy_set(lcurves);
		ucurves = destroy_set(ucurves);

		if (!lerror && !uerror && !aerror) break;

		areas = destroy_set(areas);
		iloop++;
		if (iloop > 100)
			{
			pr_error("Contouring",
				"Failed to find contour area %g to %g even with relooping!\n",
				lower, upper);
			return NullSet;
			}

		lval -= aval/2.0;
		uval += aval/2.0;
		pr_error("Contouring", "Relooping due to error in contours\n");
		pr_error("Contouring",
			"  Adjust: %g to %g  by: %g (*%d)\n", lower, upper, aval, iloop);
		}

	/* If no areas we may still need to fill the whole limit box */
	if (!areas)
		{
		double	value;
		AREA	area;
		LINE	line;
		LOGICAL	valid;

		/* Sample any point in the surface */
		valid = eval_sfc_unmapped(sfc, make_point(0.5, 0.5), &value);
		if (!valid)        return NullSet;
		if (value > upper) return NullSet;
		if (value < lower) return NullSet;

		/* Need the box */
		line = create_line();
		add_point_to_line(line, make_point(limbox.left,  limbox.bottom));
		add_point_to_line(line, make_point(limbox.left,  limbox.top));
		add_point_to_line(line, make_point(limbox.right, limbox.top));
		add_point_to_line(line, make_point(limbox.right, limbox.bottom));
		add_point_to_line(line, make_point(limbox.left,  limbox.bottom));
		area = create_area("", "", "");
		define_area_boundary(area, line);
		areas = create_set("area");
		add_item_to_set(areas, (ITEM)area);
		}

	return areas;
	}

/***********************************************************************
*                                                                      *
*    c o n t o u r _ a r e a s e t _ f r o m _ c u r v e s             *
*                                                                      *
***********************************************************************/

typedef	enum	{ EdgeL, EdgeT, EdgeR, EdgeB, EdgeC } EDGE;
typedef	struct	ent_struct
	{
	EDGE				entry_edge;
	float				entry_lctn;
	EDGE				exit_edge;
	float				exit_lctn;
	float				size;
	LINE				line;
	struct ent_struct	*prev;
	struct ent_struct	*next;
	} ENTLIST;

static	EDGE	set_entry_edge(CURVE, float *);
static	EDGE	set_exit_edge(CURVE, float *);
static	ENTLIST	*find_ent(EDGE, float, float);
static	ENTLIST	*insert_ent(EDGE, float, EDGE, float, float, LINE);
static	void	remove_ent(ENTLIST *);
static	LINE	follow_contour_area(ENTLIST *);

static	ENTLIST	*EntList = NULL;
static	ENTLIST	*LastEnt = NULL;
static	ENTLIST	*CtrList = NULL;
static	ENTLIST	*LastCtr = NULL;
static	float	XLl, YLt, XLr, YLb;

static	LOGICAL	AreaErrorFound = FALSE;
LOGICAL	contour_areaset_failure(void)
	{
	if (!AreaErrorFound) return FALSE;
	AreaErrorFound = FALSE;
	return TRUE;
	}

/*********************************************************************/
/** Convert 2 sets of contour curves (generated from lower and upper
 *  values) to a set of closed AREAS with holes.
 *
 *	@param[in] 	lcurves		set of curves from lower contour value
 *	@param[in] 	ucurves		set of curves from upper contour value
 *	@param[in] 	*limits		limiting box
 *	@return set of areas from contours.
 *********************************************************************/
SET		contour_areaset_from_curves

	(
	SET	lcurves,
	SET	ucurves,
	BOX	*limits
	)

	{
	ENTLIST	*list, *lnext;
	int		imem;
	EDGE	edge_e, edge_x;
	CURVE	curve;
	LINE	line, cline, lline;
	float	lctn_e, lctn_x, size;
	LOGICAL	cw;
	AREA	area;
	SET		areas = NullSet;

#	ifdef DEBUG_BAND
	int		ipt;
#	endif /* DEBUG_BAND */

	/* Return nothing if no curves given */
	if (   (IsNull(lcurves) || lcurves->num <= 0)
		&& (IsNull(ucurves) || ucurves->num <= 0) )
		return NullSet;
	if (IsNull(limits)) return NullSet;

	XLl = limits->left;
	YLt = limits->top;
	XLr = limits->right;
	YLb = limits->bottom;

	/* Add upper curves to edge list (unchanged) */
	if (NotNull(ucurves))
		{

#		ifdef DEBUG_BAND
		pr_diag("Contouring", "Upper curves: %d\n", ucurves->num);
#		endif /* DEBUG_BAND */

		for (imem=0; imem<ucurves->num; imem++)
			{
			curve = (CURVE) ucurves->list[imem];
			line  = curve->line;
			if (IsNull(line))      continue;
			if (line->numpts <= 0) continue;
			edge_e = set_entry_edge(curve, &lctn_e);
			edge_x = set_exit_edge(curve,  &lctn_x);

#			ifdef DEBUG_BAND
			pr_diag("Contouring", "  Curve: %d - %d  Entry/Exit edge: %d %d\n",
				imem, line->numpts, edge_e, edge_x);
			for (ipt=0; ipt<line->numpts; ipt++)
				pr_diag("Contouring", "    Position: %.2f %.2f\n",
					line->points[ipt][X], line->points[ipt][Y]);
#			endif /* DEBUG_BAND */

			size   = 0.0;
			if (edge_e == EdgeC)
				{
				/* Sort these by size */
				close_line(line);
				line_properties(line, NullChar, NullChar, &size, NullFloat);
				}
			list = insert_ent(edge_e, lctn_e, edge_x, lctn_x, size, line);
			}
		}

	/* Add lower curves to edge list (with entry/exit and line reversed) */
	if (NotNull(lcurves))
		{

#		ifdef DEBUG_BAND
		pr_diag("Contouring", "Lower curves: %d\n", lcurves->num);
#		endif /* DEBUG_BAND */

		for (imem=0; imem<lcurves->num; imem++)
			{
			curve = (CURVE) lcurves->list[imem];
			line  = curve->line;
			if (IsNull(line))      continue;
			if (line->numpts <= 0) continue;
			edge_x = set_entry_edge(curve, &lctn_x);
			edge_e = set_exit_edge(curve,  &lctn_e);

#			ifdef DEBUG_BAND
			pr_diag("Contouring", "  Curve: %d - %d  Entry/Exit edge: %d %d\n",
				imem, line->numpts, edge_x, edge_e);
			for (ipt=0; ipt<line->numpts; ipt++)
				pr_diag("Contouring", "    Position: %.2f %.2f\n",
					line->points[ipt][X], line->points[ipt][Y]);
#			endif /* DEBUG_BAND */

			size   = 0.0;
			if (edge_x == EdgeC)
				{
				/* Sort these by size */
				close_line(line);
				line_properties(line, NullChar, NullChar, &size, NullFloat);
				}
			/* Reverse the line */
			(void) reverse_line(line);
			list = insert_ent(edge_e, lctn_e, edge_x, lctn_x, size, line);
			}
		}

	/* Now track boundaries of individual areas entering at the edge */
	list = EntList;
	while (NotNull(list))
		{
		/* Track this area boundary */
		line = follow_contour_area(list);

		/* Define an area */
		if (NotNull(line))
			{
			area = create_area("", "", "");
			define_area_boundary(area, line);
			if (IsNull(areas)) areas = create_set("area");
			add_item_to_set(areas, (ITEM) area);
			}

		/* Find the start of the next area */
		lnext = list->next;
		remove_ent(list);
		list  = lnext;
		}

	/* Now track areas and holes made from closed contours */
	list = CtrList;
	while (NotNull(list))
		{
		line  = list->line;
		cline = copy_line(line);

		/* See if this is a hole in an existing area or just an area */
		line_properties(cline, NullChar, &cw, NullFloat, NullFloat);
		if (cw)
			{
			/* It's a hole - add the hole to the appropriate area */
			if (IsNull(areas))
				{
				/* If there are no areas yet we need the limit box as an area */
				areas = create_set("area");
				area  = create_area("", "", "");
				lline = create_line();
				add_point_to_line(lline, make_point(XLl, YLb));
				add_point_to_line(lline, make_point(XLr, YLb));
				add_point_to_line(lline, make_point(XLr, YLt));
				add_point_to_line(lline, make_point(XLl, YLt));
				add_point_to_line(lline, make_point(XLl, YLb));
				define_area_boundary(area, lline);
				add_item_to_set(areas, (ITEM) area);
				}
			else
				{
				/* Otherwise it MUST be inside an existing area */
				area = enclosing_area(areas, cline->points[0], PickSmallest,
							NullFloat, NullChar);
				if (IsNull(area))
					{
					pr_error("Contouring", "HELP!!! Hole with no area.\n");
					AreaErrorFound = TRUE;
					}
				}
			add_area_hole(area, cline);
			}
		else
			{
			/* Its an area - just add it */
			area = create_area("", "", "");
			define_area_boundary(area, cline);
			if (IsNull(areas)) areas = create_set("area");
			add_item_to_set(areas, (ITEM) area);
			}

		/* Move to next one */
		lnext = list->next;
		remove_ent(list);
		list  = lnext;
		}

	return areas;
	}

/**********************************************************************/

static	EDGE	set_entry_edge
		(
		CURVE	curve,
		float	*lctn
		)

	{
	float	x, y;
	STRING	value;

	/* Set parameters from start of line */
	(void) get_attribute(curve->attrib, EntryEdge, &value);
	x = curve->line->points[0][X];
	y = curve->line->points[0][Y];

	/* Set location if it starts on one of the edges */
	if (same(value, EdgeLeft))
		{
		*lctn = y - YLb;
		return EdgeL;
		}
	if (same(value, EdgeTop))
		{
		*lctn = x - XLl;
		return EdgeT;
		}
	if (same(value, EdgeRight))
		{
		*lctn = YLt - y;
		return EdgeR;
		}
	if (same(value, EdgeBottom))
		{
		*lctn = XLr - x;
		return EdgeB;
		}

	/* Otherwise must be a closed interior contour */
	*lctn = 0.0;
	return EdgeC;
	}

/**********************************************************************/

static	EDGE	set_exit_edge
		(
		CURVE	curve,
		float	*lctn
		)

	{
	float	x, y;
	STRING	value;

	/* Set parameters from end of line */
	(void) get_attribute(curve->attrib, ExitEdge, &value);
	x = curve->line->points[curve->line->numpts-1][X];
	y = curve->line->points[curve->line->numpts-1][Y];

	/* Set location if it ends on one of the edges */
	if (same(value, EdgeLeft))
		{
		*lctn = y - YLb;
		return EdgeL;
		}
	if (same(value, EdgeTop))
		{
		*lctn = x - XLl;
		return EdgeT;
		}
	if (same(value, EdgeRight))
		{
		*lctn = YLt - y;
		return EdgeR;
		}
	if (same(value, EdgeBottom))
		{
		*lctn = XLr - x;
		return EdgeB;
		}

	/* Otherwise must be a closed interior contour */
	*lctn = 0.0;
	return EdgeC;
	}

/**********************************************************************/

static	ENTLIST	*find_ent
		(
		EDGE		edge,
		float		lctn,
		float		size
		)

	{
	ENTLIST	*lnext, *lprev;

	lprev = NULL;

	/* Handle closed contours separately */
	if (edge == EdgeC)
		{
		/* Find position in the centre list - order of decreasing area */
		lnext = CtrList;
		while ( NotNull(lnext) )
			{
			if (lnext->size < size) break;
			lprev = lnext;
			lnext = lnext->next;
			}
		}

	/* Areas that intersect the edge */
	else
		{
		/* Find position in the edge list - clockwise order of position */
		lnext = EntList;
		while ( NotNull(lnext) )
			{
			if (lnext->entry_edge > edge) break;
			if ( (lnext->entry_edge == edge)
					&& (lnext->entry_lctn > lctn)) break;
			lprev = lnext;
			lnext = lnext->next;
			}
		}

	return lprev;
	}

/**********************************************************************/

static	ENTLIST	*insert_ent
		(
		EDGE		entry_edge,
		float		entry_lctn,
		EDGE		exit_edge,
		float		exit_lctn,
		float		size,
		LINE		line
		)

	{
	ENTLIST	*lnew, *lprev, *lnext;
	LOGICAL	ctr;

	/* Find position in the list based on the entry point */
	lprev = find_ent(entry_edge, entry_lctn, size);

	/* Construct a new record */
	lnew = INITMEM(ENTLIST, 1);
	lnew->entry_edge = entry_edge;
	lnew->entry_lctn = entry_lctn;
	lnew->exit_edge  = exit_edge;
	lnew->exit_lctn  = exit_lctn;
	lnew->size       = size;
	lnew->line       = line;
	ctr = (LOGICAL) (entry_edge == EdgeC);

	/* Insert the record in the list */
	if (NotNull(lprev))
			{
			lnext       = lprev->next;
			lnew->next  = lnext;
			lnew->prev  = lprev;
			lprev->next = lnew;
			if (NotNull(lnext)) lnext->prev = lnew;
			else if (ctr)       LastCtr     = lnew;
			else                LastEnt     = lnew;
			}
	else	{
			lnext       = (ctr)? CtrList: EntList;
			lnew->next  = (ctr)? CtrList: EntList;
			lnew->prev  = NULL;
			if (ctr) CtrList = lnew;
			else     EntList = lnew;
			if (NotNull(lnext)) lnext->prev = lnew;
			else if (ctr)       LastCtr     = lnew;
			else                LastEnt     = lnew;
			}
	return lnew;
	}

/**********************************************************************/

static	void	remove_ent
		(
		ENTLIST	*list
		)

	{
	ENTLIST	*lprev, *lnext;

	if (IsNull(list)) return;

	lprev = list->prev;
	lnext = list->next;

	if (IsNull(lprev))
		{
		if (EntList == list) EntList = lnext;
		else                 CtrList = lnext;
		}
	else                 lprev->next = lnext;

	if (IsNull(lnext))
		{
		if (LastEnt == list) LastEnt = lprev;
		else                 LastCtr = lprev;
		}
	else                 lnext->prev = lprev;

	FREEMEM(list);
	}

/**********************************************************************/

static	LINE	follow_contour_area
		(
		ENTLIST	*list
		)

	{
	ENTLIST	*lfirst;
	EDGE	edge;
	float	lctn, size;
	LOGICAL	wrap;
	LINE	cline;

	if (IsNull(list)) return NullLine;

	lfirst = list;
	cline  = create_line();

	while (TRUE)
		{

		/* Append this contour to the line */
		append_line(cline, list->line);

		/* Set the exit parameters */
		edge = list->exit_edge;
		lctn = list->exit_lctn;
		size = list->size;

		/* Remove the contour (if not the first one) */
		if (list != lfirst) remove_ent(list);

		/* This contour should end at an edge */
		if (edge == EdgeC)
			{
			pr_error("Contouring",
				"HELP!!! Contour no longer ending at an edge.\n");
			AreaErrorFound = TRUE;
			return destroy_line(cline);
			}

		/* Next contour starts ccw from where the last one ended */
		list = find_ent(edge, lctn, size);
		if (IsNull(list))
			{
			list = LastEnt;
			if ( IsNull(list) )
				{
				pr_error("Contouring",
					"HELP!!! Ran out of contours prematurely.\n");
				AreaErrorFound = TRUE;
				return destroy_line(cline);
				}
			}

		/* Add enough corners to get to the next contour */
		wrap = (LOGICAL) ( (list->entry_edge == edge)
								&& (list->entry_lctn > lctn) );
		while (wrap || edge != list->entry_edge)
			{
			switch (edge)
				{
				case EdgeL:
						add_point_to_line(cline, make_point(XLl, YLb));
						edge = EdgeB;
						break;
				case EdgeB:
						add_point_to_line(cline, make_point(XLr, YLb));
						edge = EdgeR;
						break;
				case EdgeR:
						add_point_to_line(cline, make_point(XLr, YLt));
						edge = EdgeT;
						break;
				case EdgeT:
						add_point_to_line(cline, make_point(XLl, YLt));
						edge = EdgeL;
						break;
				default:
						pr_error("Contouring",
							"HELP!!! Why am I here with no edge?\n");
						AreaErrorFound = TRUE;
						return destroy_line(cline);
				}
			wrap = FALSE;
			}

		if (list == lfirst) break;
		}

	/* Close the area */
	close_line(cline);
	return cline;
	}
