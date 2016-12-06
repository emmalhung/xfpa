/*********************************************************************/
/** @file surface.c
 *
 * Routines to hanlde the SURFACE object.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*      s u r f a c e . c                                               *
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

#define SURFACE_INIT
#include "surface.h"

#include <tools/tools.h>
#include <fpa_getmem.h>

/* Set various debug modes */
#undef DEBUG_SFC
#undef DEBUG_SPLINE

int		SurfaceCount = 0;

/***********************************************************************
*                                                                      *
*      c r e a t e _ s u r f a c e                                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Create a new surface with no attributes.
 * This defines the surface as a cubic B-spline (order = 4)
 * with uniform knot spacing.
 *
 * @return pointer to new surface object. You will need to destroy
 * this object when you are finished with it.
 *********************************************************************/

SURFACE create_surface(void)

	{
	SURFACE	sfc;

	/* Allocate memory for the principal structure */
	sfc = INITMEM(struct SURFACE_struct, 1);
	if (!sfc) return NullSfc;

	/* Initialize the structure */
	init_spline(&sfc->sp);
	sfc->nuknot    = 0;
	sfc->nvknot    = 0;
	sfc->nupatch   = 0;
	sfc->nvpatch   = 0;
	sfc->ulist     = (ILIST **) 0;
	sfc->vlist     = (ILIST **) 0;
	sfc->patches   = (PATCH **) 0;
	init_uspec(&sfc->units);
	sfc->cspecs    = (CONSPEC *) 0;
	sfc->ncspec    = 0;
	sfc->bands     = NullSet;

	SurfaceCount++;
	return sfc;
	}

/***********************************************************************
*                                                                      *
*      d e s t r o y _ s u r f a c e                                   *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Return the space used by the given surface.
 *
 *	@param[in] 	sfc		surface to be destroyed
 * 	@return NullSfc
 *********************************************************************/

SURFACE destroy_surface

	(
	SURFACE	sfc
	)

	{
	int		iu, iv, nu, nv;

	/* Do nothing if surface is NULL */
	if (!sfc) return NullSfc;

	/* Free the units and contour spec list */
	define_surface_units(sfc, (USPEC *) 0);
	define_surface_conspecs(sfc, 0, (CONSPEC *) 0);

	/* Free the u intersection list array */
	if (NotNull(sfc->ulist))
		{
		nu = sfc->sp.m - ORDER + 1;
		nv = sfc->sp.n - ORDER + 2;
		for (iu=0; iu<nu; iu++)
			for (iv=0; iv<nv; iv++)
				{
				(void) destroy_sfc_ulist(sfc, iu, iv);
				}
		FREEMEM(*sfc->ulist);
		FREEMEM( sfc->ulist);
		}

	/* Free the v intersection list array */
	if (NotNull(sfc->vlist))
		{
		nu = sfc->sp.m - ORDER + 2;
		nv = sfc->sp.n - ORDER + 1;
		for (iu=0; iu<nu; iu++)
			for (iv=0; iv<nv; iv++)
				{
				(void) destroy_sfc_vlist(sfc, iu, iv);
				}
		FREEMEM(*sfc->vlist);
		FREEMEM( sfc->vlist);
		}

	/* Free the patch array */
	if (NotNull(sfc->patches))
		{
		nu = sfc->sp.m - ORDER + 1;
		nv = sfc->sp.n - ORDER + 1;
		for (iu=0; iu<nu; iu++)
			for (iv=0; iv<nv; iv++)
				{
				(void) destroy_sfc_patch(sfc, iu, iv);
				}
		FREEMEM(*sfc->patches);
		FREEMEM( sfc->patches);
		}

	/* Free the spline knot vectors and control vertex array */
	free_spline(&sfc->sp);

	/* Free the band contours */
	if (NotNull(sfc->bands)) (void) destroy_set(sfc->bands);

	/* Now free the structure itself */
	FREEMEM(sfc);
	SurfaceCount--;
	return NullSfc;
	}

/***********************************************************************
*                                                                      *
*      c o p y _ s u r f a c e                                         *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Produce an exact copy of the given surface.
 *
 *	@param[in] 	sfc		surface to be copied
 *	@param[in] 	all		Should we copy the whole structure or not
 * 	@return Pointer to copy of given object. You will need to destroy
 * 			this object when you are finished with it.
 *********************************************************************/

SURFACE copy_surface

	(
	const SURFACE	sfc,
	LOGICAL			all
	)

	{
	SURFACE	snew = NullSfc;
	SPLINE	*sp;
	int		iup, ivp, nup, nvp;
	PATCH	p;

	/* Make sure we have something to copy */
	if (!sfc) return NullSfc;

	/* Copy the main structure */
#	ifdef DEBUG_SFC
	(void) printf("copying surface\n");
#	endif /* DEBUG_SFC */
	snew = create_surface();

	/* Duplicate units and contour spec list */
#	ifdef DEBUG_SPLINE
	(void) printf("copying contour specs\n");
#	endif /* DEBUG_SPLINE */
	define_surface_units(snew, &sfc->units);
	define_surface_conspecs(snew, (int) sfc->ncspec, sfc->cspecs);

	/* Duplicate spline */
#	ifdef DEBUG_SPLINE
	(void) printf("copying spline\n");
#	endif /* DEBUG_SPLINE */
	sp = &sfc->sp;
	if (sp->m <= 0) return snew;
	if (sp->n <= 0) return snew;
	define_surface_patches(snew, sp->m, sp->n);
	copy_spline(&snew->sp, sp);
	/*
	if (sp->dim == DimVector2D)
		{
		define_surface_spline_2D(snew, sp->m, sp->n, &sp->mp, sp->origin,
					sp->orient, sp->gridlen, *sp->cvx, *sp->cvy, sp->n);
		}
	else
		{
		define_surface_spline(snew, sp->m, sp->n, &sp->mp, sp->origin,
					sp->orient, sp->gridlen, *sp->cvs, sp->n);
		}
	*/
	if (!all)       return snew;

	/* Duplicate patch array */
#	ifdef DEBUG_SPLINE
	(void) printf("copying patches\n");
#	endif /* DEBUG_SPLINE */
	nup = sp->m - ORDER + 1;
	nvp = sp->n - ORDER + 1;
	for (iup=0; iup<nup; iup++)
		for (ivp=0; ivp<nvp; ivp++)
			{
			p = get_sfc_patch(sfc, iup, ivp);
			if (IsNull(p))
				{
				if (get_MMM() == MMM_Preallocate)
						snew->patches[iup][ivp] = create_patch();
				else	snew->patches[iup][ivp] = NullPatch;
				}
			else
				{
				snew->patches[iup][ivp] = copy_patch(p);
				}
			}

	/* Copy the contour bands */
	if (NotNull(sfc->bands)) snew->bands = copy_set(sfc->bands);

#	ifdef DEBUG_SPLINE
	(void) printf("done\n");
#	endif /* DEBUG_SPLINE */

	return snew;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ s u r f a c e _ s p l i n e                       *
*      d e f i n e _ s u r f a c e _ s p l i n e _ 2 D                 *
*      d e f i n e _ s u r f a c e _ p a t c h e s                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Define the spline dimensions and control vertex array of the
 * given surface.
 *
 *	@param[in] 	sfc			given surface
 *	@param[in] 	m			spline dimensions
 *	@param[in] 	n			spline dimensions
 *	@param[in] 	*mp			map projection type
 *	@param[in] 	origin		origin
 *	@param[in] 	orient		orientation angle
 *	@param[in] 	gridlen		uniform knot spacing
 *	@param[in] 	*cvs		control vertex array
 *	@param[in] 	ncol		width of incoming array
 *********************************************************************/

void	define_surface_spline

	(
	SURFACE			sfc,
	int				m,
	int				n,
	const MAP_PROJ	*mp,
	const POINT		origin,
	float			orient,
	float			gridlen,
	float			*cvs,
	int				ncol
	)

	{
	/* Do nothing if surface not allocated */
	if (!sfc) return;

	MMM_begin_count();

	/* Free/reallocate patch and ilist arrays */
	define_surface_patches(sfc, m, n);

	/* Set up spline with given control vertex array */
	define_spline(&sfc->sp, m, n, mp, origin, orient, gridlen, cvs, ncol);

	MMM_report_count("After Define");
	}

/**********************************************************************/

/*********************************************************************/
/** Define the spline dimensions and control vertex array of the
 * given surface.	(Vector Field)
 *
 *	@param[in] 	sfc			given surface
 *	@param[in] 	m			spline dimensions
 *	@param[in] 	n			spline dimensions
 *	@param[in] 	*mp			map projection type
 *	@param[in] 	origin		origin
 *	@param[in] 	orient		orientation angle
 *	@param[in] 	gridlen		uniform knot spacing
 *	@param[out]	*xcvs		x-component control vertex array
 *	@param[out]	*ycvs		y-component control vertex array
 *	@param[in] 	ncol		width of incoming arrays
 *********************************************************************/
void	define_surface_spline_2D

	(
	SURFACE			sfc,
	int				m,
	int				n,
	const MAP_PROJ	*mp,
	const POINT		origin,
	float			orient,
	float			gridlen,
	float			*xcvs,
	float			*ycvs,
	int				ncol
	)

	{
	/* Do nothing if surface not allocated */
	if (!sfc) return;

	MMM_begin_count();

	/* Free/reallocate patch and ilist arrays */
	define_surface_patches(sfc, m, n);

	/* Set up spline with given control vertex array */
	define_spline_2D(&sfc->sp, m, n, mp, origin, orient, gridlen,
			xcvs, ycvs, ncol);

	MMM_report_count("After Define");
	}

/**********************************************************************/

/*********************************************************************/
/** Define the spline dimensions and control vertex array of the
 * given surface.
 *
 *	@param[in] 	sfc		given surface
 *	@param[in] 	m		spline dimensions
 *	@param[in] 	n		spline dimensions
 *********************************************************************/
void	define_surface_patches

	(
	SURFACE			sfc,	/* given surface */
	int				m,		/* spline dimensions */
	int				n		/* spline dimensions */
	)

	{
	int		iu, iv, nu, nv;
	LOGICAL	usame, vsame, keep, init;
	int		nuprev, nvprev;
	ILIST	*lptr;
	PATCH	*pptr;

	/* Do nothing if surface not allocated */
	if (!sfc) return;

	/* Set up spline with given control vertex array */
	nuprev = sfc->sp.m - ORDER + 1;
	nvprev = sfc->sp.n - ORDER + 1;
	usame = (LOGICAL) (m == sfc->sp.m);
	vsame = (LOGICAL) (n == sfc->sp.n);
	init  = (LOGICAL) ((get_MMM()) == MMM_Preallocate);
	keep  = (LOGICAL) (usame && vsame && init);
	sfc->nuknot  = m + ORDER;
	sfc->nvknot  = n + ORDER;
	sfc->nupatch = m - ORDER + 1;
	sfc->nvpatch = n - ORDER + 1;

	/* Destroy old patches if reallocating */
	pptr = (sfc->patches)  ? sfc->patches[0]  : (PATCH *) 0;
	if (NotNull(pptr))
		{
		nu = nuprev;
		nv = nvprev;
		for (iu=0; iu<nu; iu++)
			for (iv=0; iv<nv; iv++)
				{
				if (keep) empty_patch(sfc->patches[iu][iv]);
				else      destroy_sfc_patch(sfc, iu, iv);
				}
		}

	/* Allocate/reallocate patch array */
	if ((!usame) || (!vsame) || IsNull(pptr))
		{
		nu = sfc->nupatch;
		nv = sfc->nvpatch;
		pptr         = GETMEM(pptr, PATCH, nu*nv);
		sfc->patches = GETMEM(sfc->patches, PATCH *, nu);
		for (iu=0; iu<nu; iu++)
			{
			sfc->patches[iu] = pptr + iu*nv;
			for (iv=0; iv<nv; iv++)
				{
				sfc->patches[iu][iv] = (init)? create_patch() : NullPatch;
				}
			}
		}

	/* Destroy old u-ilists if reallocating */
	lptr = (sfc->ulist) ? sfc->ulist[0] : (ILIST *) 0;
	if (NotNull(lptr))
		{
		nu = nuprev;
		nv = nvprev + 1;
		for (iu=0; iu<nu; iu++)
			for (iv=0; iv<nv; iv++)
				{
				if (keep) empty_ilist(sfc->ulist[iu][iv]);
				else      destroy_sfc_ulist(sfc, iu, iv);
				}
		}

	/* Allocate and initialize u-intersection list array */
	if ((!usame) || (!vsame) || (!lptr))
		{
		nu = sfc->nupatch;
		nv = sfc->nvpatch + 1;
		lptr       = GETMEM(lptr, ILIST, nu*nv);
		sfc->ulist = GETMEM(sfc->ulist, ILIST *, nu);
		for (iu=0; iu<nu; iu++)
			{
			sfc->ulist[iu] = lptr + iu*nv;
			for (iv=0; iv<nv; iv++)
				{
				sfc->ulist[iu][iv] = (init)? create_ilist() : NullIlist;
				}
			}
		}

	/* Destroy old v-ilists if reallocating */
	lptr = (sfc->vlist) ? sfc->vlist[0] : (ILIST *) 0;
	if (NotNull(lptr))
		{
		nu = nuprev + 1;
		nv = nvprev;
		for (iu=0; iu<nu; iu++)
			for (iv=0; iv<nv; iv++)
				{
				if (keep) empty_ilist(sfc->vlist[iu][iv]);
				else      destroy_sfc_vlist(sfc, iu, iv);
				}
		}

	/* Allocate and initialize v-intersection list array */
	if ((!usame) || (!vsame) || (!lptr))
		{
		nu = sfc->nupatch + 1;
		nv = sfc->nvpatch;
		lptr       = GETMEM(lptr, ILIST, nu*nv);
		sfc->vlist = GETMEM(sfc->vlist, ILIST *, nu);
		for (iu=0; iu<nu; iu++)
			{
			sfc->vlist[iu] = lptr + iu*nv;
			for (iv=0; iv<nv; iv++)
				{
				sfc->vlist[iu][iv] = (init)? create_ilist() : NullIlist;
				}
			}
		}

	/* Wipe out old contour bands */
	if (NotNull(sfc->bands)) sfc->bands = destroy_set(sfc->bands);
	}

/***********************************************************************
*                                                                      *
*      i n s i d e _ s u r f a c e _ s p l i n e                       *
*      i n s i d e _ s u r f a c e _ s p l i n e _ x y                 *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Check whether the given point is within the spline extent of the
 * given surface.
 *
 *	@param[in] 	sfc		given surface
 *	@param[in] 	p		given point
 * 	@return True if point is within the spline extent of the surface.
 *********************************************************************/

LOGICAL	inside_surface_spline

	(
	SURFACE	sfc,
	POINT	p
	)

	{
	int		iup, ivp;
	POINT	pp, dp;

	if (!sfc) return FALSE;
	if (!p)   return FALSE;

	/* Find patch which contains the given point */
	/* Don't care which patch it is - just whether one is found */
	return find_patch(&sfc->sp, p, &iup, &ivp, pp, dp);
	}

/**********************************************************************/

/*********************************************************************/
/** Check whether the given xy-pair is within the spline extent of the
 * given surface.
 *
 *	@param[in] 	sfc		given surface
 *	@param[in] 	x		x coord
 *	@param[in] 	y		y coord
 * 	@return True if xy-pair is within the spline extent of the surface.
 *********************************************************************/
LOGICAL	inside_surface_spline_xy

	(
	SURFACE	sfc,
	float	x,
	float	y
	)

	{
	return inside_surface_spline(sfc, make_point(x, y));
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ s u r f a c e _ u n i t s                         *
*      c h a n g e _ s u r f a c e _ u n i t s                         *
*      r e c a l l _ s u r f a c e _ u n i t s                         *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set the surface units.
 *
 *	@param[in]  sfc		given surface
 *	@param[in]  *units	new units
 *********************************************************************/
void	define_surface_units

	(
	SURFACE		sfc,
	const USPEC	*units
	)

	{
	/* Do nothing if no surface */
	if (!sfc) return;

	/* Define the units (current units are forgotten) */
	free_uspec(&sfc->units);
	if (units) copy_uspec(&sfc->units, units);
	}

/**********************************************************************/

/*********************************************************************/
/** Reset the surface units. Make the needed changes.
 *
 *	@param[in] 	sfc		given surface
 *	@param[in]  	*units	new units
 *********************************************************************/
void	change_surface_units

	(
	SURFACE		sfc,
	const USPEC	*units
	)

	{
	double	old_factor, old_offset;
	double	efactor, eoffset;

	/* Do nothing if no surface */
	if (!sfc)   return;
	if (!units) return;

	/* Remember the original units */
	old_factor = sfc->units.factor;
	old_offset = sfc->units.offset;

	/* Return if no change */
	if ((units->factor == old_factor) && (units->offset == old_offset))
		return;

	/* Construct effective factor and offset to convert spline */
	/* coefficients directly (i.e. new = old*efactor + eoffset) */
	efactor = units->factor / old_factor;
	eoffset = units->offset - old_offset*efactor;
	change_spline_units(&sfc->sp, efactor, eoffset);

	/* Save the new units definition */
	copy_uspec(&sfc->units, units);

	/* Force a re-computation of the patch functions */
	redefine_surface_patches(sfc, 0, sfc->nupatch-1, 0, sfc->nvpatch-1, FALSE);
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve the units of the given surface.
 *
 *	@param[in] 	sfc		given surface
 *	@param[out]	**units	units returned
 *********************************************************************/
void	recall_surface_units

	(
	SURFACE	sfc,
	USPEC	**units
	)

	{
	if (units) *units = (sfc) ? &sfc->units : (USPEC *) 0;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ s u r f a c e _ c o n s p e c s                   *
*      r e c a l l _ s u r f a c e _ c o n s p e c s                   *
*      a d d _ c o n s p e c _ t o _ s u r f a c e                     *
*      i n v o k e _ s u r f a c e _ c o n s p e c s                   *
*                                                                      *
*      Define the contouring specs for the given surface.              *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set or reset surface contouring specs for the given surface.
 *
 *	@param[in] 	sfc		given surface
 *	@param[in] 	ncspec	number of specs to set
 *	@param[in] 	*cspecs	list of contour specs to set
 *********************************************************************/
void	define_surface_conspecs

	(
	SURFACE	sfc,
	int		ncspec,
	CONSPEC	*cspecs
	)

	{
	int		ic;

	/* Do nothing if no surface */
	if (!sfc)   return;

	/* Get rid of existing cspec list */
	if (sfc->cspecs)
		{
		for (ic=0; ic<sfc->ncspec; ic++)
			{
			free_conspec(sfc->cspecs + ic);
			}
		FREEMEM(sfc->cspecs);
		}
	sfc->ncspec = 0;

	/* Add the new cspec list one at a time */
	if (cspecs)
		{
		for (ic=0; ic<ncspec; ic++)
			{
			add_conspec_to_surface(sfc, cspecs+ic);
			}
		}
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve the surface contouring specs for the given surface.
 *
 *	@param[in] 	sfc			given surface
 *	@param[out]	*ncspec		number of specs to recall
 *	@param[out]	**cspecs	list of contouring specs
 *********************************************************************/
void	recall_surface_conspecs

	(
	SURFACE	sfc,
	int		*ncspec,
	CONSPEC	**cspecs
	)

	{
	if (ncspec) *ncspec = (sfc) ? sfc->ncspec : 0;
	if (cspecs) *cspecs = (sfc) ? sfc->cspecs : (CONSPEC *) 0;
	}

/**********************************************************************/

/*********************************************************************/
/** Add a contouring spec to the given surface.
 *
 *	@param[in] 	sfc		given surface
 *	@param[in] 	*cspec	new contour spec to add
 *********************************************************************/
void	add_conspec_to_surface

	(
	SURFACE	sfc,
	CONSPEC	*cspec
	)

	{
	CONSPEC	*csnew;

	/* Do nothing if nothing to work on */
	if (!sfc)   return;
	if (!cspec) return;

	/* Expand the cspec list */
	sfc->ncspec++;
	sfc->cspecs = GETMEM(sfc->cspecs, CONSPEC, sfc->ncspec);
	csnew = sfc->cspecs + (sfc->ncspec-1);
	init_conspec(csnew);

	/* Copy the given cspec into the new one */
	copy_conspec(csnew, cspec);

	/* Now check if too few or too many contours */
	if (same(csnew->type, "range"))
		{
		float	vmin, vmax, vxmin, vxmax, fact;
		int		nc, nd;

		if (csnew->cint <= 0) return;

		/* Find range of contouring */
		find_surface_range(sfc, &vmin, &vmax);
		if (vmax <= vmin) return;

		/* Check for too few contours */
		nc = (vmax-vmin) / csnew->cint;
		if (nc <= 0)
			{
			pr_warning("Contouring",
				"Too few contours - field values %g to %g  with increment %g %s\n",
				vmin, vmax, csnew->cint, sfc->units.name);
			}

		/* Limit range to contouring limits */
		vxmin = MAX(vmin, csnew->cmin);
		vxmax = MIN(vmax, csnew->cmax);
		if (vxmax <= vxmin) return;

		/* Limit number of contours to around 100 to 250 */
		nc = (vxmax-vxmin) / csnew->cint;
		nd = ndigit(nc);
		if (nd > 2)
			{
			fact  = pow(10.0, (double)(nd-3));
			nd    = nc/fact/250;
			fact *= (nd+1);
			if (fact > 1.0)
				{
				pr_warning("Contouring",
					"Too many contours - field values %g to %g %s\n",
					vmin, vmax, sfc->units.name);
				pr_warning("Contouring",
					"Too many contours - contour range %g to %g %s\n",
					vxmin, vxmax, sfc->units.name);
				pr_warning("Contouring",
					"Too many contours - adjusting increment %g %s by %g\n",
					csnew->cint, sfc->units.name, fact);
				csnew->cint *= fact;
				}
			}
		}
	}

/**********************************************************************/

/*********************************************************************/
/** Invoke the contouring specs for a given surface.
 *
 *	@param[in] 	sfc	surface to be reset
 *********************************************************************/
void	invoke_surface_conspecs

	(
	SURFACE	sfc
	)

	{
	int		iup, ivp;
	PATCH	patch;

	/* Do nothing if surface undefined */
	if (!sfc) return;

	/* Reset the patches */
	for (iup=0; iup<sfc->nupatch; iup++)
		for (ivp=0; ivp<sfc->nvpatch; ivp++)
			{
			patch = get_sfc_patch(sfc, iup, ivp);
			reset_patch_conspec(patch, sfc->ncspec, sfc->cspecs);
			}
	}

/***********************************************************************
*                                                                      *
*      h i g h l i g h t _ s u r f a c e                               *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Change the highlight code of patch contours.
 *
 *	@param[in] 	sfc		surface to affect
 *	@param[in] 	code	new hilite codes
 *********************************************************************/

void	highlight_surface

	(
	SURFACE	sfc,
	HILITE	code
	)

	{
	int		i, iu, iv, nu, nv;
	HILITE	fcode;
	PATCH	p;
	CONSPEC	*cspec;

	/* Make sure we have a surface */
	if (!sfc) return;
	fcode = MIN(code, (HILITE)0);

	/* Set surface contour spec highlight code */
	for (i=0; i<sfc->ncspec; i++)
		{
		cspec = sfc->cspecs + i;
		define_lspec_value(&cspec->lspec, LINE_HILITE, (POINTER) &code);
		define_fspec_value(&cspec->fspec, FILL_HILITE, (POINTER) &fcode);
		define_tspec_value(&cspec->tspec, TEXT_HILITE, (POINTER) &code);
		define_mspec_value(&cspec->mspec, MARK_HILITE, (POINTER) &code);
		define_bspec_value(&cspec->bspec, BARB_HILITE, (POINTER) &code);
		}

	/* Change highlight code of contoured patches */
	nu = sfc->nupatch;
	nv = sfc->nvpatch;
	for (iu=0; iu<nu; iu++)
		{
		for (iv=0; iv<nv; iv++)
			{
			p = get_sfc_patch(sfc, iu, iv);
			if (IsNull(p)) continue;
			highlight_set(p->contours, code);
			highlight_set(p->extrema, code);
			highlight_set(p->vectors, code);
			}
		}
	}

/***********************************************************************
*                                                                      *
*      c h a n g e _ s u r f a c e _ p s p e c                         *
*      r e c a l l _ s u r f a c e _ p s p e c                         *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Change the presentation specs of patch contours and markers.
 *
 *	@param[in] 	sfc		given surface
 *	@param[in] 	param	parameter to change
 *	@param[in] 	value	new value
 *********************************************************************/

void	change_surface_pspec

	(
	SURFACE	sfc,
	PPARAM	param,
	POINTER	value
	)

	{
	int		iu, iv, nu, nv;
	PATCH	p;

	/* Do nothing if surface does not exist */
	if (!sfc) return;

	/* Change presentation of contoured patches */
	nu = sfc->nupatch;
	nv = sfc->nvpatch;
	for (iu=0; iu<nu; iu++)
		{
		for (iv=0; iv<nv; iv++)
			{
			p = get_sfc_patch(sfc, iu, iv);
			if (IsNull(p)) continue;
			change_set_pspec(p->contours, param, value);
			change_set_pspec(p->extrema, param, value);
			change_set_pspec(p->vectors, param, value);
			}
		}
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve the presentation specs of patch contours and markers.
 *	@param[in] 	sfc		given surface
 *	@param[in] 	param	parameter to lookup
 *	@param[out]	value	value retrieved
 *********************************************************************/
void	recall_surface_pspec

	(
	SURFACE	sfc,
	PPARAM	param,
	POINTER	value
	)

	{
	/* Do nothing if surface does not exist */
	if (!sfc) return;

	recall_lspec_value(&sfc->cspecs[0].lspec, param, value);
	recall_mspec_value(&sfc->cspecs[0].mspec, param, value);
	}
