/**********************************************************************/
/** @file	spline.c
 *
 *	Routines to handle the SPLINE, PATCH and ILIST objects.
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*      s p l i n e . c                                                 *
*                                                                      *
*      Routines to handle the SPLINE, PATCH and ILIST objects.         *
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

#define SPLINE_INIT
#include "spline.h"
#include "surface.h"

#include <fpa_getmem.h>
#include <fpa_math.h>
#include <tools/tools.h>

#include <string.h>
#include <stdio.h>

/* Set various debug modes */
#undef DEBUG_SPLINE
#undef DEBUG_SPLINE_2D
#undef DEBUG_PATCH
#undef DEBUG_TRACK
#undef DEBUG_ILIST

static	void	track_patch_function(BIPOLY *, char, ILIST, char,
						SET, SET, int, CONSPEC *);
static	void	tidy_patch_contours(SET, ILIST, ILIST, ILIST, ILIST);
static	void	track_patch_contour(BIPOLY *, float, POINT,
						POINT, float, LOGICAL, POINT, LOGICAL *);
static	void	track_patch_contour_old(BIPOLY *, float, POINT,
						float, float, float, int, POINT, LOGICAL *);
static	STRING	contour_val(float);
static	STRING	contour_lab(float);

/***********************************************************************
*                                                                      *
*    i n i t _ s c a l a r                                             *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** initialize memory for SCALAR Object (use after malloc)
 *
 * @param[in]  *scalr	scalar object to initialize
 **********************************************************************/

void init_scalar

	(
	SCALAR *scalr
	)

	{
	/* Do nothing if null */
	if (!scalr) return;

	/* Zero everything */
	scalr->sval = 0;
	}

/***********************************************************************
*                                                                      *
*    f r e e _ s c a l a r                                             *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** free memory for SCALAR Object
 *
 * @param[in] 	*scalr	scalar object to free
 **********************************************************************/

void free_scalar

	(
	SCALAR *scalr
	)

	{
	/* Do nothing if null */
	if (!scalr) return;

	/* Give back a zero scalar */
	scalr->sval = 0;
	}

/***********************************************************************
*                                                                      *
*    c o p y _ s c a l a r                                             *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** copy a SCALAR Object
 *
 *  @param[out]	*pcopy	scalar to copy to	
 *  @param[in] 	*scalr	scalar to copy from	
 **********************************************************************/

void copy_scalar

	(
	SCALAR		 *pcopy,
	const SCALAR *scalr
	)

	{
	/* Return now if no SCALAR Object to copy into or none to copy */
	if (!pcopy) return;
	if (!scalr) return;

	/* Copy the scalar value */
	pcopy->sval = scalr->sval;
	}

/***********************************************************************
*                                                                      *
*    d e b u g _ s c a l a r                                           *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** display the contents of a SCALAR Object
 *
 *  @param[in]  *scalr	scalar object to display
 **********************************************************************/

void debug_scalar

	(
	SCALAR *scalr
	)

	{
	/* Return now if no SCALAR Object */
	if (!scalr) return;

	/* Display the basic attributes */
	(void) fprintf(stderr, " Contents of SCALAR Object\n");
	(void) fprintf(stderr, "  sval: %e", scalr->sval);

	(void) fprintf(stderr, "\n");
	}

/***********************************************************************
*                                                                      *
*    i n i t _ v l i s t                                               *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** initialize memory for VLIST Object (use after malloc)
 *
 *  @param[in]  *vlist	vlist to initialize
 **********************************************************************/

void init_vlist

	(
	VLIST *vlist
	)

	{
	/* Do nothing if null */
	if (!vlist) return;

	/* Zero everything */
	vlist->numpts = 0;
	vlist->maxpts = 0;
	vlist->val    = NULL;
	vlist->pos    = NULL;
	}

/***********************************************************************
*                                                                      *
*    f r e e _ v l i s t                                               *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** free memory for VLIST Object
 *
 *  @param[in]  *vlist	vlist to free
 **********************************************************************/

void free_vlist

	(
	VLIST *vlist
	)

	{
	/* Do nothing if null */
	if (!vlist) return;

	/* Give back a zero vlist */
	FREEMEM(vlist->val);
	FREEMEM(vlist->pos);
	vlist->numpts = 0;
	vlist->maxpts = 0;
	vlist->val    = NULL;
	vlist->pos    = NULL;
	}

/***********************************************************************
*                                                                      *
*    c o p y _ v l i s t                                               *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** copy a VLIST Object
 *
 *  @param[out]	*pcopy	vlist to copy to
 *  @param[in] 	*vlist	vlist to copy from
 **********************************************************************/

void copy_vlist

	(
	VLIST		*pcopy,
	const VLIST *vlist
	)

	{
	int		ipt;

	/* Return now if no VLIST Object to copy into or none to copy */
	if (!pcopy) return;
	if (!vlist) return;

	/* Copy the vlist value */
	init_vlist(pcopy);
	if (vlist->numpts <= 0) return;

	pcopy->val = INITMEM(float, vlist->numpts);
	pcopy->pos = INITMEM(POINT, vlist->numpts);
	pcopy->numpts = vlist->numpts;
	pcopy->maxpts = vlist->numpts;
	for (ipt=0; ipt<vlist->numpts; ipt++)
		{
		copy_point(pcopy->pos[ipt], vlist->pos[ipt]);
		pcopy->val[ipt] = vlist->val[ipt];
		}
	}

/***********************************************************************
*                                                                      *
*    e m p t y _ v l i s t                                             *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** empty a VLIST Object without freeing memory
 *
 *  @param[in] 	*vlist	vlist to empty
 **********************************************************************/

void empty_vlist

	(
	VLIST *vlist
	)

	{
	/* Return now if no VLIST Object */
	if (!vlist) return;

	vlist->numpts = 0;
	}

/***********************************************************************
*                                                                      *
*    a d d _ p o i n t _ t o _ v l i s t                               *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** add the given point to the VLIST Object
 *
 *  @param[in] 	*vlist	vlist to add to
 *  @param[in]   p		point to be added
 *  @param[in] 	value	corresponding value
 **********************************************************************/

void add_point_to_vlist

	(
	VLIST *vlist,
	POINT p,
	float value
	)

	{
	int		last;
	POINT	pp;

	/* Return now if no VLIST Object */
	if (!vlist) return;
	if (!p) return;

	/* Protect point in case it is from the same buffer */
	copy_point(pp, p);

	last = vlist->numpts++;
	if (vlist->numpts > vlist->maxpts)
		{
		vlist->maxpts++;
		vlist->val = GETMEM(vlist->val, float, vlist->maxpts);
		vlist->pos = GETMEM(vlist->pos, POINT, vlist->maxpts);
		}

	vlist->val[last] = value;
	copy_point(vlist->pos[last], pp);
	}

/***********************************************************************
*                                                                      *
*    s a m e _ v l i s t _ p o i n t s                                 *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** compare the point list of two VLIST Objects
 *
 *  @param[in]  *vlist1	vlist to compare
 *  @param[in]  *vlist2	vlist to compare
 *  @return True if the lists contain the same points
 **********************************************************************/

LOGICAL same_vlist_points

	(
	const VLIST *vlist1,
	const VLIST *vlist2
	)

	{
	int		ipt;

	/* Return now if no VLIST Object */
	if (!vlist1) return FALSE;
	if (!vlist2) return FALSE;

	if (vlist1->numpts != vlist2->numpts) return FALSE;

	for (ipt=0; ipt<vlist1->numpts; ipt++)
		{
		if (vlist1->pos[ipt][X] != vlist2->pos[ipt][X]) return FALSE;
		if (vlist1->pos[ipt][Y] != vlist2->pos[ipt][Y]) return FALSE;
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    d e b u g _ v l i s t                                             *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** display the contents of a VLIST Object
 *
 *  @param[in] 	*vlist	vlist to display
 **********************************************************************/

void debug_vlist

	(
	VLIST *vlist	/* pointer to VLIST Object */
	)

	{
	int		ipt, icount;

	/* Return now if no VLIST Object */
	if (!vlist) return;

	/* Display the basic attributes */
	(void) fprintf(stderr, " Contents of VLIST Object\n");
	(void) fprintf(stderr, "  numpts: %d maxpts: %d\n",
				vlist->numpts, vlist->maxpts);

	for (icount=0, ipt=0; ipt<vlist->numpts; ipt++)
		{
		icount += 3;
		if (icount > MAX_PCOUNT)
			{
			icount = 3;
			(void) fprintf(stderr, "\n");
			}
		(void) fprintf(stderr, "   (%e, %e) %e", vlist->pos[ipt][X],
				vlist->pos[ipt][Y], vlist->val[ipt]);
		}

	(void) fprintf(stderr, "\n");
	}

/***********************************************************************
*                                                                      *
*    i n i t _ g r i d                                                 *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** initialize memory for GRID Object (use after malloc)
 *
 *  @param[in] 	*gridd	grid to initialize
 **********************************************************************/

void init_grid

	(
	GRID *gridd	/* pointer to GRID Object */
	)

	{
	/* Do nothing if null */
	if (!gridd) return;

	/* Zero everything */
	gridd->nx        = 0;
	gridd->ny        = 0;
	gridd->gval      = NULL;
	gridd->origin[X] = 0;
	gridd->origin[Y] = 0;
	gridd->orient    = 0;
	gridd->gridlen   = 0;
	}

/***********************************************************************
*                                                                      *
*    f r e e _ g r i d                                                 *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** free memory for GRID Object
 *
 *  @param[in]  *gridd	grid to free
 **********************************************************************/

void free_grid

	(
	GRID *gridd	/* pointer to GRID Object */
	)

	{
	/* Do nothing if null */
	if (!gridd) return;
	if ((gridd->nx == 0) && (gridd->ny == 0)) return;

	/* Free grid array */
	FREEMEM(*gridd->gval);
	FREEMEM(gridd->gval);

	/* Give back an empty grid */
	init_grid(gridd);
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ g r i d                                           *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Define the given grid to the given dimensions, and define the
 *  data array.
 *
 *  @param[in] 	*gridd	grid to define
 *  @param[in]   nx		size of grid
 *  @param[in]   ny		size of grid
 *  @param[in]   origin	origin relative to local co-ords
 *  @param[in] 	orient	orientation relative to local co-ords
 *  @param[in] 	gridlen	uniform grid spacing
 *  @param[in] 	*data	data array
 *  @param[in] 	ncol	column dimension of data
 **********************************************************************/

void	define_grid

	(
	GRID		*gridd,		/* pointer to GRID Object */
	int			nx,			/* size of grid */
	int			ny,
	const POINT	origin,		/* origin rel to local co-ords */
	float		orient,		/* orientation rel to local co-ords */
	float		gridlen,	/* uniform grid spacing */
	float		*data,		/* data array */
	int			ncol		/* column dimension of data */
	)

	{
	int		ix, iy;
	float	*vals, *row, val;

	/* Do nothing if grid not allocated */
	if (!gridd) return;

	/* Free previous grid */
	free_grid(gridd);

	/* Define the basic attributes */
	gridd->nx        = nx;
	gridd->ny        = ny;
	if (origin) copy_point(gridd->origin, origin);
	else        copy_point(gridd->origin, ZeroPoint);
	gridd->orient    = orient;
	gridd->gridlen   = gridlen;

	/* Allocate space for data array and copy from input */
	vals        = INITMEM(float, nx*ny);
	gridd->gval = INITMEM(float *, ny);
	if ((data) && (ncol > 0))
		{
		for (iy=0; iy<ny; iy++)
			{
			gridd->gval[iy] = vals + iy*nx;
			row             = data + iy*ncol;
			for (ix=0; ix<nx; ix++)
				gridd->gval[iy][ix] = row[ix];
			}
		}
	else
		{
		val = (data) ? *data : 0;
		for (iy=0; iy<ny; iy++)
			{
			gridd->gval[iy] = vals + iy*nx;
			for (ix=0; ix<nx; ix++)
				gridd->gval[iy][ix] = val;
			}
		}
	}

/***********************************************************************
*                                                                      *
*    c o p y _ g r i d                                                 *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** copy a GRID Object
 *
 *  @param[out]	*pcopy	grid to copy to
 *  @param[in] 	*gridd	grid to copy from
 **********************************************************************/

void copy_grid

	(
	GRID		*pcopy,	/* pointer to copy of GRID Object */
	const GRID	*gridd	/* pointer to GRID Object */
	)

	{
	int nnx, nny, iix, iiy;
	float *gvalblk;

	/* Return now if no GRID Object to copy into or none to copy */
	if (!pcopy) return;
	if (!gridd) return;

	/* Define the basic attributes */
	nnx = pcopy->nx  = gridd->nx;
	nny = pcopy->ny  = gridd->ny;
	pcopy->origin[X] = gridd->origin[X];
	pcopy->origin[Y] = gridd->origin[Y];
	pcopy->orient    = gridd->orient;
	pcopy->gridlen   = gridd->gridlen;

	/* Allocate space for pointers and array of grid point data */
	gvalblk = INITMEM(float, nny*nnx);
	pcopy->gval = INITMEM(float *, nny);

	/* Set pointers and copy the grid point data */
	for (iiy=0; iiy<nny; iiy++)
		{
		pcopy->gval[iiy] = gvalblk + iiy*nnx;
		for (iix=0; iix<nnx; iix++)
			pcopy->gval[iiy][iix] = gridd->gval[iiy][iix];
		}
	}

/***********************************************************************
*                                                                      *
*    s a m e _ g r i d                                                 *
*    s a m e _ g r i d _ s i z e                                       *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** compare dimensions for two GRID objects
 *
 *  @param[in] 	g1	first grid to compare
 *  @param[in] 	g2	second grid to compare
 *  @return Ture if grids are the same size
 **********************************************************************/

LOGICAL	same_grid_size

	(
	const GRID	*g1,	/* pointer to first GRID Object */
	const GRID	*g2		/* pointer to second GRID Object */
	)

	{

	/* Return TRUE if both GRID Objects missing */
	if (!g1 && !g2) return TRUE;

	/* Return FALSE if first or second GRID Object missing */
	if (!g1) return FALSE;
	if (!g2) return FALSE;

	/* Return FALSE if GRID Object dimensions do not agree */
	if (g1->nx != g2->nx) return FALSE;
	if (g1->ny != g2->ny) return FALSE;
	if (g1->gridlen != g2->gridlen) return FALSE;

	/* Return TRUE if GRID Object dimensions agree */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    d e b u g _ g r i d                                               *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** display the contents of a GRID Object
 *
 *  @param[in]  *gridd	grid to display
 **********************************************************************/

void debug_grid

	(
	GRID *gridd	/* pointer to GRID Object */
	)

	{
	int nnx, nny, iix, iiy, icount;

	/* Return now if no GRID Object */
	if (!gridd) return;

	/* Display the basic attributes */
	(void) fprintf(stderr, " Contents of GRID Object\n");
	nnx = gridd->nx;
	nny = gridd->ny;
	(void) fprintf(stderr, "  nx: %i   ny: %i", nnx, nny);
	(void) fprintf(stderr, "   origin: (%g, %g)", gridd->origin[X], gridd->origin[Y]);
	(void) fprintf(stderr, "   orient: %g", gridd->orient);
	(void) fprintf(stderr, "   gridlen: %g", gridd->gridlen);

	/* Display the grid point data row by row */
	if (gridd->gval)
		{
		for (iiy=0; iiy<nny; iiy++)
			{
			(void) fprintf(stderr, "\n  gval[%i][0-%i]:\n", iiy, (nnx-1));
			for (icount=0, iix=0; iix<nnx; iix++)
				{
				if (++icount > MAX_PCOUNT)
					{
					icount = 1;
					(void) fprintf(stderr, "\n");
					}
				(void) fprintf(stderr, "   %e", gridd->gval[iiy][iix]);
				}
			}
		}

	(void) fprintf(stderr, "\n");
	}

/***********************************************************************
*                                                                      *
*      i n i t _ s p l i n e                                           *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Initialize the given piece of memory to look like an empty
 *  spline (use after malloc).
 *
 *  @param[in]  *sp	spline to initialize
 **********************************************************************/

void	init_spline

	(
	SPLINE	*sp	/* given spline containing garbage */
	)

	{
	/* Do nothing if null */
	if (!sp) return;

	/* Zero everything */
	sp->m         = 0;
	sp->n         = 0;
	sp->dim       = DimScalar;
	sp->cvs       = NULL;
	sp->cvx       = NULL;
	sp->cvy       = NULL;
	sp->origin[X] = 0;
	sp->origin[Y] = 0;
	sp->orient    = 0;
	sp->gridlen   = 0;
	sp->uknots    = NULL;
	sp->vknots    = NULL;
	copy_xform(sp->xform, IdentXform);
	copy_map_projection(&sp->mp, &NoMapProj);
	}

/***********************************************************************
*                                                                      *
*      f r e e _ s p l i n e                                           *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/**      Free all the space used by the given spline and turn it into
 *      an empty spline.
 *
 *  @param[in]  *sp	spline to be freed
 **********************************************************************/

void	free_spline

	(
	SPLINE	*sp	/* given spline to be freed */
	)

	{
	/* Do nothing if null */
	if (!sp) return;
	if ((sp->m == 0) && (sp->n == 0)) return;

	/* Free cvs array and knot vectors */
	FREEMEM(*sp->cvs);
	FREEMEM(sp->cvs);
	if (sp->dim == DimVector2D)
		{
		FREEMEM(*sp->cvx);
		FREEMEM(*sp->cvy);
		FREEMEM(sp->cvx);
		FREEMEM(sp->cvy);
		sp->dim = DimScalar;
		}
	FREEMEM(sp->uknots);
	FREEMEM(sp->vknots);

	/* Give back an empty spline */
	init_spline(sp);
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ s p l i n e                                       *
*      d e f i n e _ s p l i n e _ 2 D                                 *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Define the given spline to the given dimensions, set up both
 *  the knot vectors with the specified uniform spacing and define
 *  the control vertex array(s).
 *
 *	@param[in] 	*sp			spline to initialize
 *	@param[in] 	m			size of control vertex grid
 *	@param[in] 	n			size of control vertex grid
 *	@param[in] 	*mp			map projection type
 *	@param[in] 	origin		origin rel to local co-ords
 *	@param[in] 	orient		orientation rel to local co-ords
 *	@param[in] 	gridlen		uniform knot spacing
 *	@param[in] 	*cvs		control vertex array
 *	@param[in] 	ncol		column dimension of cvs
 **********************************************************************/

void	define_spline

	(
	SPLINE			*sp,		/* spline to initialize */
	int				m,			/* size of control vertex grid */
	int				n,			/* size of control vertex grid */
	const MAP_PROJ	*mp,		/* map projection type */
	const POINT		origin,		/* origin rel to local co-ords */
	float			orient,		/* orientation rel to local co-ords */
	float			gridlen,	/* uniform knot spacing */
	float			*cvs,		/* control vertex array */
	int				ncol		/* column dimension of cvs */
	)

	{
	int			iu, iv, nu, nv;
	float		*cvsblk, *row, val;
	MAP_PROJ	mpc;

	/* Do nothing if spline not allocated */
	if (!sp) return;

	/* Copy the new map projection in case we are using the existing one */
	copy_map_projection(&mpc, (mp)? mp: &NoMapProj);

	/* Free previous spline */
	free_spline(sp);

	/* Define the basic attributes */
	sp->m         = m;
	sp->n         = n;
	if (origin) copy_point(sp->origin, origin);
	else        copy_point(sp->origin, ZeroPoint);
	sp->orient    = orient;
	sp->gridlen   = gridlen;
	build_xform(sp->xform, 1.0, 1.0, sp->origin[X], sp->origin[Y], sp->orient);
	copy_map_projection(&sp->mp, &mpc);

	/* Allocate space for knot vectors and define from uniform grid */
	nu         = m + ORDER;
	nv         = n + ORDER;
	sp->uknots = INITMEM(float, nu);
	sp->vknots = INITMEM(float, nv);
	for (iu=0; iu<nu; iu++) sp->uknots[iu] = gridlen * (iu-ORDER+1);
	for (iv=0; iv<nv; iv++) sp->vknots[iv] = gridlen * (iv-ORDER+1);

	/* Allocate space for control vertex array and copy from input */
	nu      = m;
	nv      = n;
	cvsblk  = INITMEM(float, nu*nv);
	sp->cvs = INITMEM(float *, nu);
	if ((cvs) && (ncol > 0))
		{
		for (iu=0; iu<nu; iu++)
			{
			sp->cvs[iu] = cvsblk + iu*nv;
			row         = cvs    + iu*ncol;
			for (iv=0; iv<nv; iv++)
				sp->cvs[iu][iv] = row[iv];
			}
		}
	else
		{
		val = (cvs) ? *cvs : 0;
		for (iu=0; iu<nu; iu++)
			{
			sp->cvs[iu] = cvsblk + iu*nv;
			for (iv=0; iv<nv; iv++)
				sp->cvs[iu][iv] = val;
			}
		}

#	ifdef DEBUG_SPLINE
	(void) debug_spline(sp);
#	endif /* DEBUG_SPLINE */
	}

/**********************************************************************/

/**********************************************************************/
/** Define the given spline to the given dimensions, set up both
 *  the knot vectors with the specified uniform spacing and define
 *  the control vertex array(s).
 *
 *	@param[in] 	*sp			spline to initialize
 *	@param[in] 	m			size of control vertex grid
 *	@param[in] 	n			size of control vertex grid
 *	@param[in] 	*mp			map projection type
 *	@param[in] 	origin		origin rel to local co-ords
 *	@param[in] 	orient		orientation rel to local co-ords
 *	@param[in] 	gridlen		uniform knot spacing
 *	@param[in] 	*cvx		x-component control vertex array
 *	@param[in] 	*cvy		y-component control vertex array
 *	@param[in] 	ncol		column dimension of cvx/cvy
 **********************************************************************/
void	define_spline_2D

	(
	SPLINE			*sp,		/* spline to initialize */
	int				m,			/* size of control vertex grid */
	int				n,			/* size of control vertex grid */
	const MAP_PROJ	*mp,		/* map projection type */
	const POINT		origin,		/* origin rel to local co-ords */
	float			orient,		/* orientation rel to local co-ords */
	float			gridlen,	/* uniform knot spacing */
	float			*cvx,		/* x-component control vertex array */
	float			*cvy,		/* y-component control vertex array */
	int				ncol		/* column dimension of cvx/cvy */
	)

	{
	int			iu, iv, nu, nv;
	float		*cvxblk, *xrow, xval;
	float		*cvyblk, *yrow, yval;
	float		*cvsblk, sval;
	MAP_PROJ	mpc;

	/* Do nothing if spline not allocated */
	if (!sp) return;

	/* Copy the new map projection in case we are using the existing one */
	copy_map_projection(&mpc, (mp)? mp: &NoMapProj);

	/* Free previous spline */
	free_spline(sp);

	/* Define the basic attributes */
	sp->m         = m;
	sp->n         = n;
	if (origin) copy_point(sp->origin, origin);
	else        copy_point(sp->origin, ZeroPoint);
	sp->orient    = orient;
	sp->gridlen   = gridlen;
	build_xform(sp->xform, 1.0, 1.0, sp->origin[X], sp->origin[Y], sp->orient);
	copy_map_projection(&sp->mp, &mpc);

	/* Allocate space for knot vectors and define from uniform grid */
	nu         = m + ORDER;
	nv         = n + ORDER;
	sp->uknots = INITMEM(float, nu);
	sp->vknots = INITMEM(float, nv);
	for (iu=0; iu<nu; iu++) sp->uknots[iu] = gridlen * (iu-ORDER+1);
	for (iv=0; iv<nv; iv++) sp->vknots[iv] = gridlen * (iv-ORDER+1);

	/* Allocate space for control vertex array and copy from input */
	/* Standard cvs contains the magnitude as computed by hypot() */
	sp->dim = DimVector2D;
	nu      = m;
	nv      = n;
	cvxblk  = INITMEM(float, nu*nv);
	cvyblk  = INITMEM(float, nu*nv);
	cvsblk  = INITMEM(float, nu*nv);
	sp->cvx = INITMEM(float *, nu);
	sp->cvy = INITMEM(float *, nu);
	sp->cvs = INITMEM(float *, nu);
	if ((cvx) && (cvy) && (ncol > 0))
		{
		for (iu=0; iu<nu; iu++)
			{
			sp->cvx[iu] = cvxblk + iu*nv;
			sp->cvy[iu] = cvyblk + iu*nv;
			sp->cvs[iu] = cvsblk + iu*nv;
			xrow        = cvx    + iu*ncol;
			yrow        = cvy    + iu*ncol;
			for (iv=0; iv<nv; iv++)
				{
				sp->cvx[iu][iv] = xrow[iv];
				sp->cvy[iu][iv] = yrow[iv];
				sp->cvs[iu][iv] = 0;
				}
			}

#		ifdef DEBUG_SPLINE_2D
		if (pr_level("Spline.2D", 5))
			{
			printf("U CV Array:\n");
			for (iu=0; iu<nu; iu++)
				{
				for (iv=0; iv<nv; iv++)
					{
					printf(" %10g", sp->cvx[iu][iv]);
					}
				printf("\n");
				}

			printf("V CV Array:\n");
			for (iu=0; iu<nu; iu++)
				{
				for (iv=0; iv<nv; iv++)
					{
					printf(" %10g", sp->cvy[iu][iv]);
					}
				printf("\n");
				}
			}
#		endif /* DEBUG_SPLINE_2D */

		calc_spline_mag_cvs(sp);

#		ifdef DEBUG_SPLINE_2D
		if (pr_level("Spline.2D", 5))
			{
			printf("Mag CV Array:\n");
			for (iu=0; iu<nu; iu++)
				{
				for (iv=0; iv<nv; iv++)
					{
					printf(" %10g", sp->cvs[iu][iv]);
					}
				printf("\n");
				}
			}
#		endif /* DEBUG_SPLINE_2D */
		}
	else
		{
		xval = (cvx) ? *cvx : 0;
		yval = (cvy) ? *cvy : 0;
		sval = hypot((double)xval, (double)yval);
		for (iu=0; iu<nu; iu++)
			{
			sp->cvx[iu] = cvxblk + iu*nv;
			sp->cvy[iu] = cvyblk + iu*nv;
			sp->cvs[iu] = cvsblk + iu*nv;
			for (iv=0; iv<nv; iv++)
				{
				sp->cvx[iu][iv] = xval;
				sp->cvy[iu][iv] = yval;
				sp->cvs[iu][iv] = sval;
				}
			}
		}
	}

/***********************************************************************
*                                                                      *
*      c o p y _ s p l i n e                                           *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Copy the given spline.
 *
 *  @param[out]	*spnew	spline to copy to
 *  @param[in] 	*sp		spline to copy from
 **********************************************************************/

void	copy_spline

	(
	SPLINE			*spnew,		/* spline to initialize */
	const SPLINE	*sp			/* spline to be copied */
	)

	{
	int		m, n, iu, iv, nu, nv;
	float	*cvsblk;
	SPDIM	dim;

	/* Do nothing if spline not allocated */
	if (!spnew) return;
	init_spline(spnew);		/* should use empty_spline when written */
	if (!sp) return;

	/* Define the basic attributes */
	m   = spnew->m   = sp->m;
	n   = spnew->n   = sp->n;
	dim = spnew->dim = sp->dim;
	spnew->origin[X] = sp->origin[X];
	spnew->origin[Y] = sp->origin[Y];
	spnew->orient    = sp->orient;
	spnew->gridlen   = sp->gridlen;
	copy_xform(spnew->xform, sp->xform);
	copy_map_projection(&spnew->mp, &sp->mp);

	/* Allocate space for knot vectors and copy from other spline */
	nu = m + ORDER;
	nv = n + ORDER;
	spnew->uknots = INITMEM(float, nu);
	spnew->vknots = INITMEM(float, nv);
	for (iu=0; iu<nu; iu++) spnew->uknots[iu] = sp->uknots[iu];
	for (iv=0; iv<nv; iv++) spnew->vknots[iv] = sp->vknots[iv];

	/* Allocate space for control vertex array and copy from other spline */
	nu = m;
	nv = n;
	cvsblk     = INITMEM(float, nu*nv);
	spnew->cvs = INITMEM(float *, nu);
	for (iu=0; iu<nu; iu++)
		{
		spnew->cvs[iu] = cvsblk + iu*nv;
		for (iv=0; iv<nv; iv++)
			spnew->cvs[iu][iv] = sp->cvs[iu][iv];
		}

	if (dim == DimVector2D)
		{
		cvsblk     = INITMEM(float, nu*nv);
		spnew->cvx = INITMEM(float *, nu);
		for (iu=0; iu<nu; iu++)
			{
			spnew->cvx[iu] = cvsblk + iu*nv;
			for (iv=0; iv<nv; iv++)
				spnew->cvx[iu][iv] = sp->cvx[iu][iv];
			}

		cvsblk     = INITMEM(float, nu*nv);
		spnew->cvy = INITMEM(float *, nu);
		for (iu=0; iu<nu; iu++)
			{
			spnew->cvy[iu] = cvsblk + iu*nv;
			for (iv=0; iv<nv; iv++)
				spnew->cvy[iu][iv] = sp->cvy[iu][iv];
			}
		}
	}

/***********************************************************************
*                                                                      *
*      c a l c _ s p l i n e _ m a g _ c v s                           *
*                                                                      *
*      When a 2D spline has some or all of its U and V cv values       *
*      modified, recompute the affected Magnitude cv values.           *
*                                                                      *
*      There is no precedent for this!!!                               *
*                                                                      *
*      I stumbled on this by lengthy fiddling with quad summations     *
*      until it finally collapsed into the simple, elegant form        *
*      below.                                                          *
*                                                                      *
*      Given control vertex arrays CUij and CVij, which interpolate    *
*      the values of U and V at the grid points, we have:              *
*                                                                      *
*             3         3                                              *
*      U  =  SUM Bi(0) SUM Bj(0) CUij                                  *
*            i=0       j=0                                             *
*                                                                      *
*             3         3                                              *
*      V  =  SUM Bi(0) SUM Bj(0) CVij                                  *
*            i=0       j=0                                             *
*                                                                      *
*      We seek an array CMij, such that it interpolates the magnitude  *
*      of the vector (U,V) at each grid point, as in:                  *
*                                                                      *
*             3         3                                              *
*      M  =  SUM Bi(0) SUM Bj(0) CMij                                  *
*            i=0       j=0                                             *
*                                                                      *
*      Where  M^2 = U^2 + V^2.  Note that:                             *
*                                                                      *
*               3         3              3         3                   *
*      M^2  =  SUM Bi(0) SUM Bj(0) CMij SUM Bk(0) SUM Bl(0) CMkl       *
*              i=0       j=0            k=0       l=0                  *
*                                                                      *
*               3         3                                            *
*           =  SUM Bi(0) SUM Bj(0) CMij*M                              *
*              i=0       j=0                                           *
*                                                                      *
*      and similar for U^2 and V^2, giving:                            *
*                                                                      *
*                     3         3                                      *
*      U^2 + V^2  =  SUM Bi(0) SUM Bj(0) [ CUij*U + CVij*V ]           *
*                    i=0       j=0                                     *
*                                                                      *
*      So, we can safely equate the contents of the second summation   *
*      in each case, to get:                                           *
*                                                                      *
*      CMij*M  =  CUij*U * CVij*V                                      *
*                                                                      *
*      Hence   CMij  =  [ CUij*U + CVij*V ] / M                        *
*                                                                      *
*      or      CMij  =  [ CUij*U + CVij*V ] / hypot(U,V)               *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** When a 2D spline has some or all of its U and V cv values
*   modified, recompute the affected Magnitude cv values.
*
*   @param[in] 	*sp	spline to recompute
***********************************************************************/
void	calc_spline_mag_cvs_real

	(
	SPLINE	*sp	/* spline to be recomputed */
	)

	{
	float	Bvals[ORDER];
	int		nu, nv, iu, iv, ju, jv, ku, kv;
	double	u, v, mcv;

	if (!sp) return;
	if (sp->dim != DimVector2D) return;
	if (sp->gridlen <= 0) return;

	/* Evaluate spline basis functions once */
	evaluate_patch_basis(0.0, Bvals);

	nu = sp->m;
	nv = sp->n;

	/* Traverse all grid points within, but not on the edge */
	/* These correspond to actual data grid locations and the patch range */
	for (iu=1; iu<nu-1; iu++)
		{
		for (iv=1; iv<nv-1; iv++)
			{
			/* Compute U and V at the corresponding grid point */
			u = 0;
			v = 0;
			for (ju=0; ju<ORDER-1; ju++)
				{
				ku = iu -1 + ju;
				for (jv=0; jv<ORDER-1; jv++)
					{
					kv = iv -1 + jv;
					u += sp->cvx[ku][kv] * Bvals[ju] * Bvals[jv];
					v += sp->cvy[ku][kv] * Bvals[ju] * Bvals[jv];
					}
				}

			/* Compute the corresponding magnitude control vertex value */
			mcv = (u*sp->cvx[iu][iv] + v*sp->cvy[iu][iv]) /
				hypot(u, v);
			sp->cvs[iu][iv] = mcv;

			/* If next to the edge, copy to the edge row, to maintain */
			/* free boundary condition */
			if (iu==1)                sp->cvs[0][iv]      = mcv;
			if (iv==1)                sp->cvs[iu][0]      = mcv;
			if (iu==nu-2)             sp->cvs[nu-1][iv]   = mcv;
			if (iv==nv-2)             sp->cvs[iu][nv-1]   = mcv;
			if (iu==1 && iv==1)       sp->cvs[0][0]       = mcv;
			if (iu==nu-2 && iv==1)    sp->cvs[nu-1][0]    = mcv;
			if (iu==1 && iv==nv-2)    sp->cvs[0][nv-1]    = mcv;
			if (iu==nu-2 && iv==nv-2) sp->cvs[nu-1][nv-1] = mcv;
			}
		}
	}

/**********************************************************************/

/**********************************************************************/
/** When a 2D spline has some or all of its U and V cv values
*   modified, recompute the affected Magnitude cv values.
*
*   @param[in] 	*sp	spline to recompute
***********************************************************************/
void	calc_spline_mag_cvs

	(
	SPLINE	*sp	/* spline to be recomputed */
	)

	{
	float	*vbuf, **gbuf;
	float	Bvals[ORDER];
	int		nu, nv, iu, iv, ju, jv, ku, kv;
	double	u, v;

	if (!sp) return;
	if (sp->dim != DimVector2D) return;
	if (sp->gridlen <= 0) return;

	/* Evaluate spline basis functions once */
	evaluate_patch_basis(0.0, Bvals);

	nu   = sp->m - 2;
	nv   = sp->n - 2;
	vbuf = INITMEM(float, nu*nv);
	gbuf = INITMEM(float *, nv);
	for (iv=0; iv<nv; iv++)
		gbuf[iv] = vbuf + iv*nu;

	for (iu=0; iu<nu; iu++)
		{
		for (iv=0; iv<nv; iv++)
			{
			/* Compute U and V at the corresponding grid point */
			u = 0;
			v = 0;
			for (ju=0; ju<ORDER-1; ju++)
				{
				ku = iu + ju;
				for (jv=0; jv<ORDER-1; jv++)
					{
					kv = iv + jv;
					u += sp->cvx[ku][kv] * Bvals[ju] * Bvals[jv];
					v += sp->cvy[ku][kv] * Bvals[ju] * Bvals[jv];
					}
				}
			gbuf[iv][iu] = hypot(u, v);
			}
		}

#	ifdef DEBUG_SPLINE_2D
	if (pr_level("Spline.2D", 5))
		{
		printf("Mag Value Array:\n");
		for (iu=0; iu<nu; iu++)
			{
			for (iv=0; iv<nv; iv++)
				{
				printf(" %10g", gbuf[iu][iv]);
				}
			printf("\n");
			}
		}
#	endif /* DEBUG_SPLINE_2D */

	grid_spline_mag(sp, sp->gridlen, nu, nv, gbuf);

	FREEMEM(vbuf);
	FREEMEM(gbuf);
	}

/***********************************************************************
*                                                                      *
*    s a m e _ s p l i n e                                             *
*    s a m e _ s p l i n e _ s i z e                                   *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** compare dimensions for two SPLINE objects
 *
 *  @param[in] 	*s1	first spline to compare
 *  @param[in] 	*s2	second spline to compare
 *  @return True if splines are the same size
 **********************************************************************/

LOGICAL	same_spline_size

	(
	const SPLINE	*s1,	/* pointer to first SPLINE Object */
	const SPLINE	*s2		/* pointer to second SPLINE Object */
	)

	{

	/* Return TRUE if both SPLINE Objects missing */
	if (!s1 && !s2) return TRUE;

	/* Return FALSE if first or second SPLINE Object missing */
	if (!s1) return FALSE;
	if (!s2) return FALSE;

	/* Return FALSE if SPLINE Object dimensions do not agree */
	if (s1->m != s2->m) return FALSE;
	if (s1->n != s2->n) return FALSE;
	if (s1->gridlen != s2->gridlen) return FALSE;

	/* Return TRUE if SPLINE Object dimensions agree */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    d e b u g _ s p l i n e                                           *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/**    display the contents of a SPLINE Object
 *
 *  @param[in]  *splne	spline to display
 **********************************************************************/

void debug_spline

	(
	SPLINE *splne	/* pointer to SPLINE Object */
	)

	{
	int nnu, nnv, iiu, iiv, icount;

	/* Return now if no SPLINE Object */
	if (!splne) return;

	/* Display the basic attributes */
	(void) fprintf(stderr, " Contents of SPLINE Object\n");
	nnu = splne->m;
	nnv = splne->n;
	(void) fprintf(stderr, "  m: %i   n: %i", nnu, nnv);
	(void) fprintf(stderr, "   origin: (%g, %g)", splne->origin[X], splne->origin[Y]);
	(void) fprintf(stderr, "   orient: %g", splne->orient);
	(void) fprintf(stderr, "   gridlen: %g", splne->gridlen);

	/* Display the x and y component control vertex data row by row */
	if (splne->cvx)
		{
		for (iiu=0; iiu<nnu; iiu++)
			{
			(void) fprintf(stderr, "\n  cvx[%i][0-%i]:\n", iiu, (nnv-1));
			for (icount=0, iiv=0; iiv<nnv; iiv++)
				{
				if (++icount > MAX_PCOUNT)
					{
					icount = 1;
					(void) fprintf(stderr, "\n");
					}
				(void) fprintf(stderr, "   %e", splne->cvx[iiu][iiv]);
				}
			}
		}
	if (splne->cvy)
		{
		for (iiu=0; iiu<nnu; iiu++)
			{
			(void) fprintf(stderr, "\n  cvy[%i][0-%i]:\n", iiu, (nnv-1));
			for (icount=0, iiv=0; iiv<nnv; iiv++)
				{
				if (++icount > MAX_PCOUNT)
					{
					icount = 1;
					(void) fprintf(stderr, "\n");
					}
				(void) fprintf(stderr, "   %e", splne->cvy[iiu][iiv]);
				}
			}
		}

	/* Display the control vertex data row by row */
	if (splne->cvs)
		{
		for (iiu=0; iiu<nnu; iiu++)
			{
			(void) fprintf(stderr, "\n  cvs[%i][0-%i]:\n", iiu, (nnv-1));
			for (icount=0, iiv=0; iiv<nnv; iiv++)
				{
				if (++icount > MAX_PCOUNT)
					{
					icount = 1;
					(void) fprintf(stderr, "\n");
					}
				(void) fprintf(stderr, "   %e", splne->cvs[iiu][iiv]);
				}
			}
		}

	/* Display the u knot sequence */
	if (splne->uknots)
		{
		(void) fprintf(stderr, "\n  uknots[0-%i]:\n", (nnu+ORDER-1));
		for (icount=0, iiu=0; iiu<(nnu+ORDER); iiu++)
			{
			if (++icount > MAX_PCOUNT)
				{
				icount = 1;
				(void) fprintf(stderr, "\n");
				}
			(void) fprintf(stderr, "   %e", splne->uknots[iiu]);
			}
		}

	/* Display the v knot sequence */
	if (splne->vknots)
		{
		(void) fprintf(stderr, "\n  vknots[0-%i]:\n", (nnv+ORDER-1));
		for (icount=0, iiv=0; iiv<(nnv+ORDER); iiv++)
			{
			if (++icount > MAX_PCOUNT)
				{
				icount = 1;
				(void) fprintf(stderr, "\n");
				}
			(void) fprintf(stderr, "   %e", splne->vknots[iiv]);
			}
		}

	(void) fprintf(stderr, "\n");
	}

/***********************************************************************
*                                                                      *
*      c h a n g e _ s p l i n e _ u n i t s                           *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Alter the control vertex array to the specified units, as in:
 *
 *   new = old*factor + offset
 *
 *	@param[in] 	*sp		the spline to be editted
 *	@param[in] 	factor	scale factor
 *	@param[in] 	offset	offset value
 **********************************************************************/
void	change_spline_units

	(
	SPLINE	*sp,	/* the spline to be editted */
	double	factor,	/* scale factor */
	double	offset	/* offset value */
	)

	{
	int		iu, iv, nu, nv;

	/* If spline not defined do nothing */
	if (!sp) return;
	if (!sp->cvs) return;

	/* Do nothing if re-scale would have no effect */
	if ((factor == 1) && (offset == 0)) return;

	/* Scale and offset each control vertex value */
	nu = sp->m;
	nv = sp->n;
	for (iu=0; iu<nu; iu++)
		{
		for (iv=0; iv<nv; iv++)
			{
			sp->cvs[iu][iv] *= factor;
			sp->cvs[iu][iv] += offset;
			}
		}
	if (sp->dim == DimVector2D)
		{
		for (iu=0; iu<nu; iu++)
			{
			for (iv=0; iv<nv; iv++)
				{
				sp->cvx[iu][iv] *= factor;
				sp->cvx[iu][iv] += offset;
				sp->cvy[iu][iv] *= factor;
				sp->cvy[iu][iv] += offset;
				}
			}
		}
	}

/***********************************************************************
*                                                                      *
*      x _ c o n t r o l _ v e r t e x                                 *
*      y _ c o n t r o l _ v e r t e x                                 *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Return the x/y co-ordinate of the specified control vertex.
 *
 * 	@param[in]  *s	spline to examine
 * 	@param[in]  iu	specified cvs u-index
 * 	@return x co-ordinate of the control vertex
 **********************************************************************/

float	x_control_vertex

	(
	SPLINE	*s,	/* given spline */
	int		iu	/* specified cvs u-index */
	)

	{
	int		uknot1, uknot2;

	/* Return 0 if spline not defined */
	if (!s) return 0;

	/* The control vertex is translated over the appropriate knot pair */
	uknot1 = iu + ORDER/2;
	uknot2 = iu + (ORDER+1)/2;

	/* Depending on the order, the control vertex will translate over */
	/* a knot, or between two knots */
	if (uknot1 == uknot2) return s->uknots[uknot1];
	else                  return (s->uknots[uknot1] + s->uknots[uknot2])/2.0;
	}

/**********************************************************************/
/** Return the x/y co-ordinate of the specified control vertex.
 *
 * 	@param[in]  *s	spline to examine
 * 	@param[in]  iv	specified cvs v-index
 * 	@return y co-ordinate of the control vertex
 **********************************************************************/
float	y_control_vertex

	(
	SPLINE	*s,	/* given spline */
	int		iv	/* specified cvs v-index */
	)

	{
	int		vknot1, vknot2;

	/* Return 0 if spline not defined */
	if (!s) return 0;

	/* The control vertex is translated over the appropriate knot pair */
	vknot1 = iv + ORDER/2;
	vknot2 = iv + (ORDER+1)/2;

	/* Depending on the order, the control vertex will translate over */
	/* a knot, or between two knots */
	if (vknot1 == vknot2) return s->vknots[vknot1];
	else                  return (s->vknots[vknot1] + s->vknots[vknot2])/2.0;
	}

/***********************************************************************
*                                                                      *
*      f i n d _ p a t c h                                             *
*      f i n d _ p a t c h _ u n m a p p e d                           *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Find the patch which contains the given point, and transform
 *  the point into the patch co-ordinate system.  The patch
 *  dimensions are also returned, since these are needed in
 *  calculations of gradients.
 *
 *	@param[in] 	*sp		given spline
 *	@param[in] 	pw		given point (world co-ord)
 *	@param[out]	*iup	patch u-index
 *	@param[out]	*ivp	patch v-index
 *	@param[out]	pp		point transformed to patch co-ord
 *	@param[out]	dp		patch dimensions
 *	@return True if successful
 ***********************************************************************/
LOGICAL	find_patch

	(
	SPLINE	*sp,	/* given spline */
	POINT	pw,		/* given point (world co-ord) */
	int		*iup,	/* patch u-index */
	int		*ivp,	/* patch v-index */
	POINT	pp,		/* point transformed to patch co-ord */
	POINT	dp		/* patch dimensions */
	)

	{
	POINT	ps;

	/* Setup default return values */
	if (iup) *iup  = 0;
	if (ivp) *ivp  = 0;
	if (pp) copy_point(pp, ZeroPoint);
	if (dp) copy_point(dp, ZeroPoint);
	if (!sp) return FALSE;
	if (!pw) return FALSE;
	if (!iup || !ivp) return FALSE;
	if (!pp || !dp) return FALSE;

	/* Transform the given point to the spline origin and orientation first */
	if (!world_to_spline(sp, pw, ps))                   return FALSE;
	if (!find_patch_unmapped(sp, ps, iup, ivp, pp, dp)) return FALSE;

	return TRUE;
	}

/**********************************************************************/

/**********************************************************************/
/** Find the patch which contains the given point, and transform
 *  the point into the patch co-ordinate system.  The patch
 *  dimensions are also returned, since these are needed in
 *  calculations of gradients.
 *
 *	@param[in] 	*sp		given spline
 *	@param[in] 	ps		given point (spline co-ord)
 *	@param[out]	*iup	patch u-index
 *	@param[out]	*ivp	patch v-index
 *	@param[out]	pp		point transformed to patch co-ord
 *	@param[out]	dp		patch dimensions
 *	@return True if successful
 ***********************************************************************/
LOGICAL	find_patch_unmapped

	(
	SPLINE	*sp,	/* given spline */
	POINT	ps,		/* given point (spline co-ord) */
	int		*iup,	/* patch u-index */
	int		*ivp,	/* patch v-index */
	POINT	pp,		/* point transformed to patch co-ord */
	POINT	dp		/* patch dimensions */
	)

	{
	int		nu, nv, iuk, ivk;
	POINT	es;

	/* Setup default return values */
	if (iup) *iup  = 0;
	if (ivp) *ivp  = 0;
	if (pp) copy_point(pp, ZeroPoint);
	if (dp) copy_point(dp, ZeroPoint);
	if (!sp) return FALSE;
	if (!ps) return FALSE;
	if (!iup || !ivp) return FALSE;
	if (!pp) return FALSE;

	/* Locate knots to the left and below given point */
	nu = sp->m + ORDER;
	nv = sp->n + ORDER;

	for (iuk=nu-1; ; iuk--) if (iuk < 0 || sp->uknots[iuk] <= ps[X]) break;
	for (ivk=nv-1; ; ivk--) if (ivk < 0 || sp->vknots[ivk] <= ps[Y]) break;

	/* Translate to index of patch that contains the point */
	*iup = iuk   - ORDER + 1;
	*ivp = ivk   - ORDER + 1;
	nu   = sp->m - ORDER + 1;
	nv   = sp->n - ORDER + 1;

	/* Use edge point if outside */
	copy_point(es, ps);
	if (*iup < 0)
		{
		*iup = 0;
		iuk  = *iup + ORDER - 1;
		es[X] = sp->uknots[iuk];
		}
	if (*iup >= nu)
		{
		*iup = nu - 1;
		iuk  = *iup + ORDER - 1;
		es[X] = sp->uknots[iuk+1];
		}
	if (*ivp < 0)
		{
		*ivp = 0;
		ivk  = *ivp + ORDER - 1;
		es[Y] = sp->vknots[ivk];
		}
	if (*ivp >= nv)
		{
		*ivp = nv - 1;
		ivk  = *ivp + ORDER - 1;
		es[Y] = sp->vknots[ivk+1];
		}

	/* Transform point into patch co-ordinate system */
	if (!spline_to_patch(sp, es, *iup, *ivp, pp, dp)) return FALSE;

	return TRUE;
	}

/**********************************************************************/

/***********************************************************************
*      w o r l d _ t o _ s p l i n e                                   *
*      w o r l d _ t o _ p a t c h                                     *
*      s p l i n e _ t o _ w o r l d                                   *
*      s p l i n e _ t o _ p a t c h                                   *
*      p a t c h _ t o _ w o r l d                                     *
*      p a t c h _ t o _ s p l i n e                                   *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Transform the given point amongst the 3 co-ordinate systems:
 *  world co-ordinates,
 *  spline relative co-ordinates (rotation and trans undone),
 *  patch co-ordinates (relative to a given patch).
 *
 *	@param[in] 	*sp		given spline
 *	@param[in] 	pw		given point (world co-ord)
 *	@param[out]	ps		transformed point (spline co-ord)
 *	@return True if successful
 **********************************************************************/
LOGICAL	world_to_spline

	(
	SPLINE	*sp,	/* given spline */
	POINT	pw,		/* given point (world co-ord) */
	POINT	ps		/* transformed point (spline co-ord) */
	)

	{
	float	dx, dy;

	/* Setup default return values */
	if (ps) copy_point(ps, ZeroPoint);
	if (!sp) return FALSE;
	if (!pw) return FALSE;
	if (!ps) return FALSE;

	/* Transform the given point to the spline origin and orientation */
	dx = pw[X] - sp->xform[H][X];
	dy = pw[Y] - sp->xform[H][Y];
	ps[X] = dx*sp->xform[X][X] + dy*sp->xform[X][Y];
	ps[Y] = dx*sp->xform[Y][X] + dy*sp->xform[Y][Y];

	return TRUE;
	}

/**********************************************************************/

/**********************************************************************/
/** Transform the given point amongst the 3 co-ordinate systems:
 *  world co-ordinates,
 *  spline relative co-ordinates (rotation and trans undone),
 *  patch co-ordinates (relative to a given patch).
 *
 *	@param[in] 	*sp		given spline
 *	@param[in] 	pw		given point (world co-ord)
 *	@param[in] 	iup		patch u-index
 *	@param[in] 	ivp		patch v-index
 *	@param[out]	pp		point transformed to patch co-ord
 *	@param[out]	dp		patch dimensions
 *	@return True if successful
 **********************************************************************/
LOGICAL	world_to_patch

	(
	SPLINE	*sp,	/* given spline */
	POINT	pw,		/* given point (world co-ord) */
	int		iup,	/* patch u-index */
	int		ivp,	/* patch v-index */
	POINT	pp,		/* point transformed to patch co-ord */
	POINT	dp		/* patch dimensions */
	)

	{
	POINT	ps;

	/* Setup default return values */
	if (pp) copy_point(pp, ZeroPoint);
	if (dp) copy_point(dp, ZeroPoint);
	if (!sp) return FALSE;
	if (!pw) return FALSE;
	if (!pp) return FALSE;

	if (!world_to_spline(sp, pw, ps))               return FALSE;
	if (!spline_to_patch(sp, ps, iup, ivp, pp, dp)) return FALSE;

	return TRUE;
	}

/**********************************************************************/

/**********************************************************************/
/** Transform the given point amongst the 3 co-ordinate systems:
 *  world co-ordinates,
 *  spline relative co-ordinates (rotation and trans undone),
 *  patch co-ordinates (relative to a given patch).
 *
 *	@param[in] 	*sp		given spline
 *	@param[in] 	ps		given point (spline co-ord)
 *	@param[out]	pw		transformed point (world co-ord)
 *	@return True if successful
 **********************************************************************/
LOGICAL	spline_to_world

	(
	SPLINE	*sp,	/* given spline */
	POINT	ps,		/* given point (spline co-ord) */
	POINT	pw		/* transformed point (world co-ord) */
	)

	{
	/* Setup default return values */
	if (pw) copy_point(pw, ZeroPoint);
	if (!sp) return FALSE;
	if (!ps) return FALSE;
	if (!pw) return FALSE;

	/* Transform point into world co-ordinate system */
	pw[X] = ps[X]*sp->xform[X][X] + ps[Y]*sp->xform[Y][X] + sp->xform[H][X];
	pw[Y] = ps[X]*sp->xform[X][Y] + ps[Y]*sp->xform[Y][Y] + sp->xform[H][Y];

	return TRUE;
	}

/**********************************************************************/

/**********************************************************************/
/** Transform the given point amongst the 3 co-ordinate systems:
 *  world co-ordinates,
 *  spline relative co-ordinates (rotation and trans undone),
 *  patch co-ordinates (relative to a given patch).
 *
 *	@param[in] 	*sp		given spline
 *	@param[in] 	ps		given point (spline co-ord)
 *	@param[in] 	iup		patch u-index
 *	@param[in] 	ivp		patch v-index
 *	@param[out]	pp		point transformed to patch co-ord
 *	@param[out]	dp		patch dimensions
 *	@return True if successful
 **********************************************************************/
LOGICAL	spline_to_patch

	(
	SPLINE	*sp,	/* given spline */
	POINT	ps,		/* given point (spline co-ord) */
	int		iup,	/* patch u-index */
	int		ivp,	/* patch v-index */
	POINT	pp,		/* point transformed to patch co-ord */
	POINT	dp		/* patch dimensions */
	)

	{
	int		nu, nv, iuk, ivk;
	float	dx, dy;

	/* Setup default return values */
	if (pp) copy_point(pp, ZeroPoint);
	if (dp) copy_point(dp, ZeroPoint);
	if (!sp) return FALSE;
	if (!ps) return FALSE;
	if (!pp) return FALSE;

	/* Complain if not a legal patch */
	nu = sp->m - ORDER + 1;
	nv = sp->n - ORDER + 1;
	if (iup < 0)   return FALSE;
	if (iup >= nu) return FALSE;
	if (ivp < 0)   return FALSE;
	if (ivp >= nv) return FALSE;

	iuk = iup + ORDER - 1;
	ivk = ivp + ORDER - 1;

	/* Transform point into patch co-ordinate system */
	dx = sp->uknots[iuk+1] - sp->uknots[iuk];
	dy = sp->vknots[ivk+1] - sp->vknots[ivk];
	pp[X] = ( ps[X] - sp->uknots[iuk] ) / dx;
	pp[Y] = ( ps[Y] - sp->vknots[ivk] ) / dy;
	if (NotNull(dp)) set_point(dp, dx, dy);

	return TRUE;
	}

/**********************************************************************/

/**********************************************************************/
/** Transform the given point amongst the 3 co-ordinate systems:
 *  world co-ordinates,
 *  spline relative co-ordinates (rotation and trans undone),
 *  patch co-ordinates (relative to a given patch).
 *
 *	@param[in] 	*sp		given spline
 *	@param[in] 	pp		given point (patch co-ord)
 *	@param[in] 	iup		patch u-index
 *	@param[in] 	ivp		patch v-index
 *	@param[out]	pw		point transformed to world co-ord
 *	@return True if successful
 **********************************************************************/
LOGICAL	patch_to_world

	(
	SPLINE	*sp,	/* given spline */
	POINT	pp,		/* given point (patch co-ord) */
	int		iup,	/* patch u-index */
	int		ivp,	/* patch v-index */
	POINT	pw		/* point transformed to world co-ord */
	)

	{
	POINT	ps;

	/* Setup default return values */
	if (pw) copy_point(pw, ZeroPoint);
	if (!sp) return FALSE;
	if (!pp) return FALSE;
	if (!pw) return FALSE;

	if (!patch_to_spline(sp, pp, iup, ivp, ps)) return FALSE;
	if (!spline_to_world(sp, ps, pw))           return FALSE;

	return TRUE;
	}

/**********************************************************************/

/**********************************************************************/
/** Transform the given point amongst the 3 co-ordinate systems:
 *  world co-ordinates,
 *  spline relative co-ordinates (rotation and trans undone),
 *  patch co-ordinates (relative to a given patch).
 *
 *	@param[in] 	*sp		given spline
 *	@param[in] 	pp		given point (patch co-ord)
 *	@param[in] 	iup		patch u-index
 *	@param[in] 	ivp		patch v-index
 *	@param[out]	ps		point transformed to spline co-ord
 *	@return True if successful
 **********************************************************************/
LOGICAL	patch_to_spline

	(
	SPLINE	*sp,	/* given spline */
	POINT	pp,		/* given point (patch co-ord) */
	int		iup,	/* patch u-index */
	int		ivp,	/* patch v-index */
	POINT	ps		/* point transformed to spline co-ord */
	)

	{
	int		nu, nv, iuk, ivk;
	float	dx, dy;

	/* Setup default return values */
	if (ps) copy_point(ps, ZeroPoint);
	if (!sp) return FALSE;
	if (!pp) return FALSE;
	if (!ps) return FALSE;

	/* Complain if not a legal patch */
	nu = sp->m - ORDER + 1;
	nv = sp->n - ORDER + 1;
	if (iup < 0)   return FALSE;
	if (iup >= nu) return FALSE;
	if (ivp < 0)   return FALSE;
	if (ivp >= nv) return FALSE;

	iuk = iup + ORDER - 1;
	ivk = ivp + ORDER - 1;

	/* Transform point into spline relative co-ordinate system */
	dx = sp->uknots[iuk+1] - sp->uknots[iuk];
	dy = sp->vknots[ivk+1] - sp->vknots[ivk];
	ps[X] = sp->uknots[iuk] + pp[X]*dx;
	ps[Y] = sp->vknots[ivk] + pp[Y]*dy;

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*      c r e a t e _ p a t c h                                         *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Allocate an empty PATCH object.
 *
 *  @return an empty patch object. User should release memory when done
 **********************************************************************/

PATCH	create_patch (void)

	{
	PATCH	p;

	/* Allocate the patch */
	p = INITMEM(struct PATCH_struct, 1);

	/* Provide an empty set */
	p->dim        = DimScalar;
	p->contours   = NULL;
	p->extrema    = NULL;
	p->vectors    = NULL;
	p->mask.all   = TRUE;
	p->mask.none  = FALSE;
	p->mask.masks = NULL;
	p->defined    = FALSE;

	return p;
	}

/***********************************************************************
*                                                                      *
*      d e s t r o y _ p a t c h                                       *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Deallocate a patch.
 *
 * 	@param[in] 	p	patch to destroy
 * 	@return	NullPatch
 **********************************************************************/

PATCH	destroy_patch

	(
	PATCH	p	/* given patch to be destroyed */
	)

	{
	if (!p) return NullPatch;

	free_patch(p);
	FREEMEM(p);
	return NullPatch;
	}

/***********************************************************************
*                                                                      *
*      f r e e _ p a t c h                                             *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Free all the space used by the given patch and turn it into
 *  an empty patch.
 *
 * 	@param[in] 	p	patch to free
 **********************************************************************/

void	free_patch

	(
	PATCH	p	/* given patch to be freed */
	)

	{
	/* Do nothing if null */
	if (!p) return;

	/* Provide an empty set */
	p->contours   = destroy_set(p->contours);
	p->extrema    = destroy_set(p->extrema);
	p->vectors    = destroy_set(p->vectors);
	p->mask.masks = destroy_set(p->mask.masks);
	p->defined    = FALSE;
	}

/***********************************************************************
*                                                                      *
*      e m p t y _ p a t c h                                           *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Zero the patch curve set without freeing any allocated space.
 *
 *  @param[in] 	p	patch to empty
 **********************************************************************/

void	empty_patch

	(
	PATCH	p	/* given patch to be emptied */
	)

	{
	/* Do nothing if null */
	if (!p) return;

	/* Zero the patch curve set */
	empty_set(p->contours);
	empty_set(p->extrema);
	empty_set(p->vectors);
	p->defined = FALSE;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ p a t c h                                         *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Define a patch from the parent spline.
 *
 *	@param[in] 	p		patch to be defined
 *	@param[in] 	*ps		parent spline
 *	@param[in] 	iup		knot u-index in parent spline
 *	@param[in] 	ivp		knot v-index in parent spline
 **********************************************************************/

void	define_patch

	(
	PATCH	p,		/* patch to be defined */
	SPLINE	*ps,	/* parent spline */
	int		iup,	/* knot u-index in parent spline */
	int		ivp		/* knot v-index in parent spline */
	)

	{
	int		iuk, ivk;
	LSPLINE	local;
	BOX		viewport;

#	ifdef DEBUG_PATCH
	static	POINT	corner[] = { 0,0, 0,1, 1,1, 1,0 };
	float	(*a)[ORDER];
	double	(*d)[ORDER];
#	endif /* DEBUG_PATCH */

	/* Do nothing if patch or spline do not exist */
	if (!p) return;
	if (!ps) return;

	/* Determine if patch is still defined */
	if (p->defined) return;

	/* Clean out old contours */
	empty_patch(p);

	/* See if knot index is in legal territory */
	iuk = iup + ORDER - 1;
	ivk = ivp + ORDER - 1;
	if ( (iuk < ORDER-1) || (iuk >= ps->m) ||
		 (ivk < ORDER-1) || (ivk >= ps->n) )
		return;

	/* Define patch function */
	define_lspline(&local, ps, M_Comp, iuk, ivk);
	generate_patch_function(&p->function, &local);
	if (ps->dim == DimVector2D)
		{
		p->dim = DimVector2D;
		define_lspline(&local, ps, X_Comp, iuk, ivk);
		generate_patch_function(&p->xfunc, &local);
		define_lspline(&local, ps, Y_Comp, iuk, ivk);
		generate_patch_function(&p->yfunc, &local);
		}

#	ifdef DEBUG_PATCH
	(void) printf("\npatch[%d][%d]\n", iup, ivp);
	a = local.cvs;
	(void) printf("    local:\n");
	(void) printf("    %f %f %f %f\n", a[0][0], a[0][1], a[0][2], a[0][3]);
	(void) printf("    %f %f %f %f\n", a[1][0], a[1][1], a[1][2], a[1][3]);
	(void) printf("    %f %f %f %f\n", a[2][0], a[2][1], a[2][2], a[2][3]);
	(void) printf("    %f %f %f %f\n", a[3][0], a[3][1], a[3][2], a[3][3]);
	d = p->function.coeffs;
	(void) printf("    function:\n");
	(void) printf("    %f %f %f %f\n", d[0][0], d[0][1], d[0][2], d[0][3]);
	(void) printf("    %f %f %f %f\n", d[1][0], d[1][1], d[1][2], d[1][3]);
	(void) printf("    %f %f %f %f\n", d[2][0], d[2][1], d[2][2], d[2][3]);
	(void) printf("    %f %f %f %f\n", d[3][0], d[3][1], d[3][2], d[3][3]);
	(void) printf("    corner-bl: %f\n", evaluate_bipoly(&p->function,
				corner[0]));
	(void) printf("    corner-br: %f\n", evaluate_bipoly(&p->function,
				corner[1]));
	(void) printf("    corner-tr: %f\n", evaluate_bipoly(&p->function,
				corner[2]));
	(void) printf("    corner-tl: %f\n", evaluate_bipoly(&p->function,
				corner[3]));
	(void) fflush(stdout);
#	endif /* DEBUG_PATCH */

	/* Define patch transform */
	viewport.left   = ps->uknots[iuk];
	viewport.right  = ps->uknots[iuk+1];
	viewport.bottom = ps->vknots[ivk];
	viewport.top    = ps->vknots[ivk+1];
	block_xform(p->xform, &viewport, &UnitBox);

	/* Now patch has been defined */
	p->defined = TRUE;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ l s p l i n e                                     *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Define a local spline from the parent spline.
 *
 *	@param[out]	*ls		local spline to be defined
 *	@param[in] 	*ps		parent spline
 *	@param[in] 	comp	which component X_Comp | Y_Comp | M_Comp
 *	@param[in] 	iu		knot u-index in parent spline
 *	@param[in] 	iv		knot v-index in parent spline
 **********************************************************************/

void	define_lspline

	(
	LSPLINE	*ls,	/* local spline to be defined */
	SPLINE	*ps,	/* parent spline */
	int		comp,	/* which component X_Comp | Y_Comp | M_Comp */
	int		iu,		/* knot u-index in parent spline */
	int		iv		/* knot v-index in parent spline */
	)

	{
	int		ju, lu, nu;
	int		jv, lv, nv;

	/* Do nothing if local or parent spline do not exist */
	if (!ls) return;
	if (!ps) return;

	/* See if knot index is in legal territory */
	if ( (iu < ORDER-1) || (iu >= ps->m) ||
		 (iv < ORDER-1) || (iv >= ps->n) )
		return;

	/* See if requested component is legal */
	switch (comp)
		{
		case M_Comp:	break;
		case X_Comp:
		case Y_Comp:	if (ps->dim == DimVector2D) break;
		default:		return;
		}

	/* Copy local control vertex array */
	nu = ORDER;
	nv = ORDER;
	for (lu=0, ju=iu-ORDER+1; lu<nu; lu++, ju++)
		for (lv=0, jv=iv-ORDER+1; lv<nv; lv++, jv++)
			{
			switch (comp)
				{
				case M_Comp:	ls->cvs[lu][lv] = ps->cvs[ju][jv];
								break;
				case X_Comp:	ls->cvs[lu][lv] = ps->cvx[ju][jv];
								break;
				case Y_Comp:	ls->cvs[lu][lv] = ps->cvy[ju][jv];
								break;
				}
			}

	/* Copy local u-knot vector */
	nu = ORDER+ORDER;
	for (lu=0, ju=iu-ORDER+1; lu<nu; lu++, ju++)
		ls->uknots[lu] = ps->uknots[ju];

	/* Copy local v-knot vector */
	nv = ORDER+ORDER;
	for (lv=0, jv=iv-ORDER+1; lv<nv; lv++, jv++)
		ls->vknots[lv] = ps->vknots[jv];
	}

/***********************************************************************
*                                                                      *
*      c o p y _ p a t c h                                             *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Copy a patch.
 *
 *	@param[in] 	p		patch to be copied
 *	@return copy of patch or NullPatch in case of failure
 **********************************************************************/

PATCH	copy_patch

	(
	const PATCH	p		/* patch to be copied */
	)

	{
	PATCH		pnew=NullPatch;	/* patch to be defined */

#	ifdef DEBUG_PATCH
	double	(*d)[ORDER];
#	endif /* DEBUG_PATCH */

	/* Do nothing if patch does not exist */
	if (!p)          return pnew;
	pnew = create_patch();
	if (!p->defined) return pnew;

	/* Copy contours and stationary points */
	pnew->contours = copy_set(p->contours);
	pnew->extrema  = copy_set(p->extrema);
	pnew->vectors  = copy_set(p->vectors);

	/* Copy mask definition */
	pnew->mask.all   = p->mask.all;
	pnew->mask.none  = p->mask.none;
	pnew->mask.masks = copy_set(p->mask.masks);

	/* Copy patch function */
	pnew->dim = p->dim;
	copy_bipoly(&pnew->function, &p->function);
	if (p->dim == DimVector2D)
		{
		copy_bipoly(&pnew->xfunc, &p->xfunc);
		copy_bipoly(&pnew->yfunc, &p->yfunc);
		}

	/* Copy patch transform */
	copy_xform(pnew->xform, p->xform);

	/* Now patch has been defined */
	pnew->defined = TRUE;

#	ifdef DEBUG_PATCH
	if (p->contours)
		(void) printf("    contours: %d %d\n", pnew->contours, pnew->contours->num);
	if (p->extrema)
		(void) printf("     extrema: %d %d\n", pnew->extrema, pnew->extrema->num);
	if (p->vectors)
		(void) printf("     vectors: %d %d\n", pnew->vectors, pnew->vectors->num);
	d = pnew->function.coeffs;
	(void) printf("    function:\n");
	(void) printf("      %f %f %f %f\n", d[0][0], d[0][1], d[0][2], d[0][3]);
	(void) printf("      %f %f %f %f\n", d[1][0], d[1][1], d[1][2], d[1][3]);
	(void) printf("      %f %f %f %f\n", d[2][0], d[2][1], d[2][2], d[2][3]);
	(void) printf("      %f %f %f %f\n", d[3][0], d[3][1], d[3][2], d[3][3]);
	(void) printf("    xform:\n");
	(void) printf("      %f %f\n", p->xform[X][X], p->xform[X][Y]);
	(void) printf("      %f %f\n", p->xform[Y][X], p->xform[Y][Y]);
	(void) printf("      %f %f\n", p->xform[H][X], p->xform[H][Y]);
#	endif /* DEBUG_PATCH */

	return pnew;
	}

/***********************************************************************
*                                                                      *
*      e v a l u a t e _ p a t c h _ b a s i s                         *
*      e v a l u a t e _ p a t c h _ d e r i v                         *
*                                                                      *
*      Evaluate the patch basis functions:                             *
*                                                                      *
*        b-3(U)  =  (1 - 3U + 3U**2 - U**3) / 6                        *
*        b-2(U)  =  (4 - 6U**2 + 3U**3) / 6                            *
*        b-1(U)  =  (1 + 3U + 3U**2 - 3U**3) / 6                       *
*        b-0(U)  =  (U**3) / 6                                         *
*                                                                      *
*      or their given derivatives, at the given (patch relative)       *
*      position 'U'.                                                   *
*                                                                      *
***********************************************************************/

/* Define B-spline basis functions in bipoly form (assume cubic) */
static double basis[4][4] = { { .166666666667, -.5, .5, -.166666666667 },
							  { .666666666667,   0, -1,             .5 },
							  { .166666666667,  .5, .5,            -.5 },
							  {             0,   0,  0,  .166666666667 }  };

/* Define derivative shift factor which is the scalar factor introduced when */
/* the corresponding power of x is differentiated 0, 1, 2 or 3 times */
static float shift[4][4] = { { 1, 1, 1, 1 },
							 { 1, 2, 3, 0 },
							 { 2, 6, 0, 0 },
							 { 6, 0, 0, 0 }  };

/**********************************************************************/

/**********************************************************************/
/** Evaluate the patch basis functions at the given (patch relative)
 *  position
 *
 *  @param[in]  	U		position to evalute at
 *  @param[out]	*B_vals	basis values
 **********************************************************************/
void	evaluate_patch_basis

	(
	float	U,
	float	*B_vals
	)

	{
	int		iu, du;

	/* Do nothing if no basis values */
	if (!B_vals) return;

	/* Evaluate the basis function with Horner's rule */
	/* End point values are known explicitly */
	for (iu=0; iu<ORDER; iu++)
		{
		du = ORDER - 1;
		if (U <= 0)      B_vals[iu] = basis[iu][0];
		else if (U >= 1) B_vals[iu] = basis[du-iu][0];
		else
			{
			B_vals[iu] = basis[iu][du];
			for (du--; du>=0; du--)
				{
				B_vals[iu] *= U;
				B_vals[iu] += basis[iu][du];
				}
			}
		}
	}

/**********************************************************************/

/**********************************************************************/
/** Evaluate the derivatives of the patch basis functions at the given
 * 	(patch relative) position
 *
 *  @param[in]   deriv   order of derivative
 *  @param[in]  	U		position to evalute at
 *  @param[out]	*B_vals	basis values
 **********************************************************************/
void	evaluate_patch_deriv

	(
	int		deriv,
	float	U,
	float	*B_vals
	)

	{
	int		iu, du, eu;

	/* Do nothing if no basis values */
	if (!B_vals) return;

	/* For zeroth derivative just eavluate the function itself */
	if (deriv <= 0)
		{
		evaluate_patch_basis(U, B_vals);
		return;
		}

	/* Too many differentiations results in zero */
	else if (deriv >= ORDER)
		{
		for (iu=0; iu<ORDER; iu++)
			B_vals[iu] = 0;
		return;
		}

	/* Believe it or not - this is what you get if you differentiate */
	/* Horner's rule */
	/* Shift contains the factors introduced when the corresponding */
	/* power of x is differentiated deriv times */
	du = 0;
	eu = deriv;
	for (iu=0; iu<ORDER; iu++)
		{
		du = ORDER - 1;
		if (U <= 0)      B_vals[iu] = shift[deriv][0] * basis[iu][deriv];
		else if (U >= 1) B_vals[iu] = shift[deriv][0] * basis[du-iu][deriv];
		else
			{
			eu = du + deriv;
			if (eu < ORDER) B_vals[iu] = shift[deriv][du] * basis[iu][eu];
			for (du--; du>=0; du--)
				{
				if (eu < ORDER) B_vals[iu] *= U;
				eu--;
				if (eu < ORDER) B_vals[iu] += shift[deriv][du] * basis[iu][eu];
				}
			}
		}
	}

/***********************************************************************
*                                                                      *
*      g e n e r a t e _ p a t c h _f u n c t i o n                    *
*                                                                      *
*      Set up a bipoly patch function from the bi-cubic B-spline       *
*      definition in the given patch.                                  *
*                                                                      *
*      The surface within a given patch is described by a series of    *
*      spline functions, which relate only a few neighbouring control  *
*      vertices (in the bi-cubic case here):                           *
*                                                                      *
*                     i                j                               *
*        Fi,j(u,v) = SUM ( Biu,4(u) * SUM Biv,4(v)*CViu,iv )           *
*                   iu=i-3           iv=j-3                            *
*                                                                      *
*      where patch "i,j" is the patch whose lower left corner lies     *
*      at (ui,vj).                                                     *
*                                                                      *
*      This can be further simplified by breaking down the spline      *
*      functions into the cubic basis functions.  In the specified     *
*      patch, we can write:                                            *
*                                                                      *
*        Bi-3,4(u)  =  b-3(U)  =  (1 - 3U + 3U**2 - U**3) / 6          *
*        Bi-2,4(u)  =  b-2(U)  =  (4 - 6U**2 + 3U**3) / 6              *
*        Bi-1,4(u)  =  b-1(U)  =  (1 + 3U + 3U**2 - 3U**3) / 6         *
*        Bi,4(u)    =  b-0(U)  =  (U**3) / 6                           *
*                                                                      *
*      (and similarly for the v direction), where we do a change of    *
*      co-ordinates so that U and V lie on the unit square:            *
*                                                                      *
*                  U = (u - ui) / (ui+1 - ui)                          *
*                  V = (v - vj) / (vj+1 - vj)                          *
*                                                                      *
*      So we can now write the patch function relative to the lower    *
*      left corner of the corresponding patch:                         *
*                                                                      *
*                     3                3                               *
*        fi,j(U,V) = SUM ( biu-3(U) * SUM biv-3(V)*cv[iu][iv] )        *
*                    iu=0             iv=0                             *
*                                                                      *
*      where cv[iu][iv] is the local subset of the control vertex      *
*      array, in which cv[0][0] = CVi-3,j-3.                           *
*                                                                      *
*      The 4 basis functions above, are represented in this routine    *
*      in power series form (see static float basis[][]).  We seek     *
*      to represent the patch function in a bi-variate power series    *
*      form.  This is accomplished by aggregating the coefficients     *
*      of like powers of U and V.  Hence:                              *
*                                                                      *
*                     3                 3                              *
*        f[du][dv] = SUM ( b[iu][du] * SUM b[iv][dv]*cv[iu][iv] )      *
*                    iu=0              iv=0                            *
*                                                                      *
*      where F[du][dv] is the coefficient of (u**du)*(v**dv) in the    *
*      bi-variate power series representing the patch function, and    *
*      b[iu][du] is the coefficient of u**du in biu-3(u) (and          *
*      similarly for b[iv][dv]).                                       *
*                                                                      *
*      Once this routine has been called for a particular patch, the   *
*      patch function can be evaluated repeatedly for any point inside *
*      the patch, using the usual eval_bipoly() routine.  This is far  *
*      cheaper to do than to work up through the basis functions each  *
*      time.                                                           *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Set up a bipoly patch function from the bi-cubic B-spline
 *  definition in the given patch.
 *
 *	@param[out]	*func	patch function to be returned
 *	@param[in] 	*local	local spline defining patch surface
 **********************************************************************/
void	generate_patch_function

	(
	BIPOLY	*func,	/* patch function to be returned */
	LSPLINE	*local	/* local spline defining patch surface */
	)

	{
	double	temp[ORDER], (*pfunc)[MAXTERM], val;
	int		du, iu;
	int		dv, iv;

	/* Return now if no patch function or local spline */
	if (!func) return;
	if (!local) return;

	init_bipoly(func);

	/* Set up patch function */
	pfunc = func->coeffs;
	func->xorder = ORDER - 1;
	func->yorder = ORDER - 1;

	/* Compute bivariate polynomial in given patch from B-spline basis */
	for (du=0; du<ORDER; du++)
		{
		/* Work on terms involving the current power of u first */
		for (iv=0; iv<ORDER; iv++)
			{
			val = 0;
			for (iu=0; iu<ORDER; iu++)
				{
				val += local->cvs[iu][iv] * basis[iu][du];
				}
			temp[iv] = val;
			}

		/* Then work on each power of v */
		for (dv=0; dv<ORDER; dv++)
			{
			val = 0;
			for (iv=0; iv<ORDER; iv++)
				{
				val += temp[iv] * basis[iv][dv];
				}
			pfunc[du][dv] = val;
			}
		}
	}

/***********************************************************************
*                                                                      *
*      r e s e t _ p a t c h _ m a s k                                 *
*      a d d _ p a t c h _ m a s k                                     *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Control the masking of contours in the patch.
 *
 *	@param[in] 	p	patch to be modified
 **********************************************************************/
void	reset_patch_mask

	(
	PATCH	p	/* patch to be modified */
	)

	{
	if (!p) return;

	if (!p->mask.all)
		{
		p->mask.all  = TRUE;
		p->mask.none = FALSE;
		p->defined   = FALSE;
		}
	if (p->mask.masks)
		{
		p->mask.masks = destroy_set(p->mask.masks);
		}
	}

/**********************************************************************/

/*ARGSUSED*/
/**********************************************************************/
/** Control the masking of contours in the patch.
 *
 *	@param[in] 	p			patch to be modified
 *	@param[in] 	set			area set containing mask information
 *	@param[in] 	ucat		background category if not defined
 *	@param[in] 	category	category to mask on
 *	@param[in] 	mode		"include" or "exclude" masked areas
 *	@param[in] 	rule		"and", "or" or "xor" mask areas with
 *							existing mask
 **********************************************************************/
void	add_patch_mask

	(
	PATCH	p,			/* patch to be modified */
	SET		set,		/* area set containing mask information */
	STRING	ucat,		/* background category if not defined */
	STRING	category,	/* category to mask on */
	STRING	mode,		/* "include" or "exclude" masked areas */
	STRING	rule		/* "and", "or" or "xor" mask areas with existing mask */
	)

	{
#	ifdef NOT_READY
	AREA	a, b;
	STRING	cat;
	float	tx, ty, sx, sy;
	float	pl, pr, pb, pt;
	int		nline, iline, iarea, iseg, ip;
	LINE	*lines, line;

	/* Note:  Graphics pipe functions from graphics lib used */

	if (!p) return;

	/* Determine background category */
	cat = ucat;
	if (set)
		{
		b = (AREA) set->bgnd;
		if (NotNull(b) && NotNull(b->attrib))
			(void) get_attribute(b->attrib, AttribCategory, &cat);
		}
	if (!set || set->num <= 0)
		{
		}

	/* Compute patch limits in world co-ordinates */
	/* Note: Assumes no rotation */
	sx = p->xform[X][X];
	sy = p->xform[Y][Y];
	tx = p->xform[H][X];
	ty = p->xform[H][Y];
	pl = tx;
	pr = sx + tx;
	pb = ty;
	pt = sy + ty;

	push_save();
	for (iarea=0; iarea<set->num; iarea++)
		{
		a = (AREA) set->list[iarea];

		/* Clip area to the patch first */
		reset_pipe();
		enable_clip(pl, pr, pb, pt, close, poly);
		enable_save();
		for (iseg=0; iseg<a->numseg; iseg++)
			{
			line = a->segments[iseg];
			for (ip=0; ip>line->numpts; ip++)
				{
				put_pipe(line->points[ip][X], line->points[ip][Y]);
				}
			}
		flush_pipe();
		nline = recall_save(&lines);

		/* Convert clipped area to patch co-ordinates */
		/* Note: Should be only one line */
		for (iline=0; iline<nline; iline++)
			{
			line = lines[iline];
			for (ip=0; ip>line->numpts; ip++)
				{
				/* Note: Assumes no rotation */
				line->points[ip][X] = (line->points[ip][X] - tx) / sx;
				line->points[ip][Y] = (line->points[ip][Y] - ty) / sy;
				}
			}

		/* Add area to mask */
		}
	pop_save();

#	endif /* NOT_READY */
	}

/***********************************************************************
*                                                                      *
*      s e t _ p a t c h _ i n d e x                                   *
*                                                                      *
***********************************************************************/

static	int SaveIUP	= -1;
static	int SaveIVP	= -1;

/**********************************************************************/
/** Set patch u and v index for error messages.
 *
 *	@param[in] 	iup		u index for patch
 *	@param[in] 	ivp		v index for patch
 **********************************************************************/
void	set_patch_index

	(
	int		iup,	/* u index for patch */
	int		ivp		/* v index for patch */
	)

	{
	SaveIUP = iup;
	SaveIVP = ivp;
	}

/***********************************************************************
*                                                                      *
*      p a t c h _ c o n t r o l                                       *
*                                                                      *
***********************************************************************/

/* These local patch contour control parameters are set by the function */
/* patch_control() and are not used above this point */
static	float	ZeroTol = .01;		/* tolerance for patch-edge root finder */
static	float	SegLen  = .15;		/* contour step (relative to patch size) */
static	int		NumArc  = 12;		/* steps for resolving tracking thru cols */
static	float	ArcGap  = PI/36.;	/* window to avoid contour back-tracking */
static	LOGICAL	TrackSq = TRUE;		/* track using square (T) or circle (F) */

/**********************************************************************/
/** Use information about the spline dimensions, to adjust certain
 *  patch contour control parameters.
 *
 *	@param[in] 	nu		surface patch dimensions
 *	@param[in] 	nv		surface patch dimensions
 *	@param[in] 	glen	patch gridlength
 **********************************************************************/
void	patch_control

	(
	int		nu,		/* surface patch dimensions */
	int		nv,		/* surface patch dimensions */
	float	glen	/* patch gridlength */
	)

	{
/*	static const float	GMin    = 100,		GMax    = 500;	*/
	static const int	NMin    = 25,		NMax    = 100;
	static const float	SegRMin = 0.10,		SegRMax = 0.30;
	static const float	SegTMin = 0.05,		SegTMax = 0.10;
	static const float	TolMin  = 0.005,	TolMax  = 0.02;

	static	LOGICAL	EnvSet = FALSE;
	static	float	SegMin;
	static	float	SegMax;

	int		n;
	float	fact;
	STRING	val;

	if (!EnvSet)
		{
		val = getenv("FPA_TRACK_CONTROL");
		if (blank(val)) val = get_feature_mode("Track.Control");
		if (same_ic(val, "SQUARE"))
			{
			TrackSq = TRUE;
			SegMin  = SegRMin;
			SegMax  = SegRMax;
			}
		else if (same_ic(val, "TIGHT_SQUARE"))
			{
			TrackSq = TRUE;
			SegMin  = SegTMin;
			SegMax  = SegTMax;
			}
		else if (same_ic(val, "CIRCLE"))
			{
			TrackSq = FALSE;
			SegMin  = SegRMin;
			SegMax  = SegRMax;
			}
		else if (same_ic(val, "TIGHT_CIRCLE"))
			{
			TrackSq = FALSE;
			SegMin  = SegTMin;
			SegMax  = SegTMax;
			}
		else
			{
			val = "SQUARE";
			TrackSq = TRUE;
			SegMin  = SegRMin;
			SegMax  = SegRMax;
			}

		pr_diag("Patch.Control", "Tracking Mode: %s\n", val);
		EnvSet = TRUE;
		}

	n = nu + nv;
	if (n <= NMin)
		{
		SegLen  = SegMin;
		ZeroTol = TolMin;
		}
	else if (n >= NMax)
		{
		SegLen  = SegMax;
		ZeroTol = TolMax;
		}
	else
		{
		fact    = (float)(n-NMin) / (float)(NMax-NMin);
		SegLen  = SegMin + (SegMax-SegMin)*fact;
		ZeroTol = TolMin + (TolMax-TolMin)*fact;
		}

	pr_diag("Patch.Control", "Segment Length: %g (%g)\n", SegLen, SegLen*glen);
	pr_diag("Patch.Control", "Zero Tolerance: %g\n", ZeroTol);
	pr_diag("Patch.Control", "Tracker: %s\n", (TrackSq)?"Square":"Circle");
	}


/***********************************************************************
*                                                                      *
*      c o n t o u r _ p a t c h                                       *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Contour the patch function within the given patch, and
 *  deposit the resulting contours in the patch curve set.
 *
 *	@param[in] 	p		patch to be contoured
 *	@param[in] 	llist	intersection list for left patch boundary
 *	@param[in] 	rlist	intersection list for right patch boundary
 *	@param[in] 	blist	intersection list for bottom patch boundary
 *	@param[in] 	tlist	intersection list for top patch boundary
 *	@param[in] 	ncspec	how many contour specs are there?
 *	@param[out]	*cspecs	list of contour specs
 **********************************************************************/

void	contour_patch

	(
	PATCH	p,		/* patch to be contoured */
	ILIST	llist,	/* intersection list for left patch boundary */
	ILIST	rlist,	/* intersection list for right patch boundary */
	ILIST	blist,	/* intersection list for bottom patch boundary */
	ILIST	tlist,	/* intersection list for top patch boundary */
	int		ncspec,	/* how many contour specs are there? */
	CONSPEC	*cspecs	/* list of contour specs */
	)

	{
	BIPOLY	*func, dfdx, dfdy;
	SET		contours, extrema, masks, xcurves, ycurves;
	MARK	mark;
	CURVE	xcurv, ycurv;
	LINE	xline, yline;
	POINT	cross;
	int		i, ix, iy, xseg, yseg;
	HILITE	hilite=0;
	float	dx, dy, cval;
	STRING	val, lab;
	CONSPEC	*cspec, *csmaxima, *csminima, *cssaddle;

	/* Do nothing if no patch or contour specs */
	if (!p)          return;
	if (!llist)      return;
	if (!rlist)      return;
	if (!blist)      return;
	if (!tlist)      return;
	if (ncspec <= 0) return;
	if (!cspecs)     return;

	/* Do not contour if patch is completely masked */
	if (p->mask.none) return;

	/* Set up structures to hold contours and extrema */
	if (!p->contours) p->contours = create_set("curve");
	if (!p->extrema)  p->extrema  = create_set("mark");
	contours = p->contours;
	extrema  = p->extrema;
	masks    = (p->mask.all)? NULL: p->mask.masks;
	empty_set(contours);
	empty_set(extrema);

	/* Generate contours of f(x,y) = c */
	func = &p->function;
	track_patch_function(func,'f', blist,'b', contours, masks, ncspec, cspecs);
	track_patch_function(func,'f', rlist,'r', contours, masks, ncspec, cspecs);
	track_patch_function(func,'f', tlist,'t', contours, masks, ncspec, cspecs);
	track_patch_function(func,'f', llist,'l', contours, masks, ncspec, cspecs);
	tidy_patch_contours(contours, blist, rlist, tlist, llist);

	/* Highlight all or nothing */
	hilite = cspecs[0].lspec.hilite;
	if (hilite != SkipHilite) highlight_set(p->contours, hilite);

	/* See if we need maxima, minima or saddle points */
	csmaxima = (CONSPEC *) NULL;
	csminima = (CONSPEC *) NULL;
	cssaddle = (CONSPEC *) NULL;
	for (i=0; i<ncspec; i++)
		{
		cspec = cspecs + i;
		if (same(cspec->type, "maxima")) csmaxima = cspec;
		if (same(cspec->type, "minima")) csminima = cspec;
		if (same(cspec->type, "saddle")) cssaddle = cspec;
		}

	/* We're finished if we don't need any */
	if (IsNull(csmaxima) && IsNull(csminima) && IsNull(cssaddle)) return;

	/* Buffers for constructing max, min and saddle marks */
	xcurves = create_set("curve");
	ycurves = create_set("curve");

	/* Generate contours of dfdx = 0 */
	differentiate_bipoly(func, &dfdx, 'x');
	track_patch_function(&dfdx,'x',blist,'b',xcurves,NULL,0,(CONSPEC *)NULL);
	track_patch_function(&dfdx,'x',rlist,'r',xcurves,NULL,0,(CONSPEC *)NULL);
	track_patch_function(&dfdx,'x',tlist,'t',xcurves,NULL,0,(CONSPEC *)NULL);
	track_patch_function(&dfdx,'x',llist,'l',xcurves,NULL,0,(CONSPEC *)NULL);

	/* Generate contours of dfdy = 0 */
	differentiate_bipoly(func, &dfdy, 'y');
	track_patch_function(&dfdy,'y',blist,'b',ycurves,NULL,0,(CONSPEC *)NULL);
	track_patch_function(&dfdy,'y',rlist,'r',ycurves,NULL,0,(CONSPEC *)NULL);
	track_patch_function(&dfdy,'y',tlist,'t',ycurves,NULL,0,(CONSPEC *)NULL);
	track_patch_function(&dfdy,'y',llist,'l',ycurves,NULL,0,(CONSPEC *)NULL);

	/* Find maxima, minima and saddle points where contours of */
	/* dfdx=0 and dfdy=0 cross */
	for (ix=0; ix<xcurves->num; ix++)
		{
		xcurv = (CURVE) xcurves->list[ix];
		if (!xcurv)               continue;
		if (!(xline=xcurv->line)) continue;
		if (xline->numpts <= 1)   continue;
		for (iy=0; iy<ycurves->num; iy++)
			{
			ycurv = (CURVE) ycurves->list[iy];
			if (!ycurv)               continue;
			if (!(yline=ycurv->line)) continue;
			if (yline->numpts <= 1)   continue;
			if (find_line_crossing(xline, yline, 0, xline->points[0],
									cross, &xseg, &yseg, NullLogical)
					&& xseg >=0 && xseg < xline->numpts-1
					&& yseg >=0 && yseg < yline->numpts-1)
				{

				/* Determine what type of point it is and */
				/* add it to the correct list if within range */
				dx   = yline->points[yseg+1][X] - yline->points[yseg][X];
				dy   = xline->points[xseg+1][Y] - xline->points[xseg][Y];
				cval = (float) evaluate_bipoly(func, cross);
				if (dx>0 && dy<0)
					{
					if (!csmaxima)             continue;
					if (cval < csmaxima->cmin) continue;
					if (cval > csmaxima->cmax) continue;
					val  = contour_val(cval);
					lab  = contour_lab(cval);
					mark = create_mark("maxima", val, lab, cross, 0.0);
					copy_mspec(&mark->mspec, &csmaxima->mspec);
					add_item_to_set(extrema, (ITEM) mark);
					mark = NULL;
					}
				else if (dx<0 && dy>0)
					{
					if (!csminima)             continue;
					if (cval < csminima->cmin) continue;
					if (cval > csminima->cmax) continue;
					val  = contour_val(cval);
					lab  = contour_lab(cval);
					mark = create_mark("minima", val, lab, cross, 0.0);
					copy_mspec(&mark->mspec, &csminima->mspec);
					add_item_to_set(extrema, (ITEM) mark);
					mark = NULL;
					}
				else if ((dx<0 && dy<0) || (dx>0 && dy>0))
					{
					if (!cssaddle)             continue;
					if (cval < cssaddle->cmin) continue;
					if (cval > cssaddle->cmax) continue;
					val  = contour_val(cval);
					lab  = contour_lab(cval);
					mark = create_mark("saddle", val, lab, cross, 0.0);
					copy_mspec(&mark->mspec, &cssaddle->mspec);
					add_item_to_set(extrema, (ITEM) mark);
					mark = NULL;
					}
				}
			}
		}

	/* Highlight all or nothing */
	if (hilite != SkipHilite) highlight_set(p->extrema, hilite);

	/* Clean up */
	xcurves = destroy_set(xcurves);
	ycurves = destroy_set(ycurves);
	}


/***********************************************************************
*                                                                      *
*      d o _ p a t c h _ v e c t o r s                                 *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Construct a set of vectors to display on a vector field
 *  (2D spline).
 *
 *	@param[in] 	p			patch to be contoured
 *	@param[in] 	iu			x patch index
 *	@param[in] 	iv			y patch index
 *	@param[in] 	ncspec		how many contour specs are there?
 *	@param[in] 	*cspecs		list of contour specs
 *	@param[in] 	factor		units scale factor ... no longer used
 *	@param[in] 	offset		units offset value ... no longer used
 **********************************************************************/

void	do_patch_vectors

	(
	PATCH	p,			/* patch to be contoured */
	int		iu,			/* x patch index */
	int		iv,			/* y patch index */
	int		ncspec,		/* how many contour specs are there? */
	CONSPEC	*cspecs,	/* list of contour specs */
	double	factor,		/* units scale factor ... no longer used */
	double	offset		/* units offset value ... no longer used */
	)

	{
	CONSPEC	*cspec, *csvector;
	SET		vectors;
	int		i, mu, mv, mus, mvs, mult;
	BIPOLY	*xfunc, *yfunc;
	POINT	pp, pw;
	float	uu, vv, mag, dir, lat, lon;
	BARB	barb;
	HILITE	hilite=0;

	/* Do nothing if no patch or contour specs */
	if (!p)          return;
	if (ncspec <= 0) return;
	if (!cspecs)     return;

	/* See if we need to generate barbs */
	if (p->dim != DimVector2D) return;

	if (!p->vectors)  p->vectors  = create_set("barb");
	vectors  = p->vectors;
	empty_set(vectors);

	/* See if we have a vector spec */
	csvector = (CONSPEC *) NULL;
	for (i=0; i<ncspec; i++)
		{
		cspec = cspecs + i;
		if (same(cspec->type, "vector")) csvector = cspec;
		}
	if (IsNull(csvector)) return;

	/* Determine patch display parameter */
	/* Note - negative displays only at some patch locations */
	/*      - positive displays multiple values between patch locations */
	mult  = csvector->vmult;
	mult  = (mult >= 0)? MAX(mult, 1): MIN(mult, -1);

	/* Check first for displaying barbs only for some patches */
	if (mult < 0)
		{
		if (iu<=0) return;
		if (iv<=0) return;
		if ((iu/mult)*mult != iu) return;
		if ((iv/mult)*mult != iv) return;

		xfunc = &p->xfunc;
		yfunc = &p->yfunc;

		/* Evalutate wind barb at the patch corner */
		pp[X] = 0.0;
		pp[Y] = 0.0;
		uu = evaluate_bipoly(xfunc, pp);
		vv = evaluate_bipoly(yfunc, pp);

		/* Set vector speed */
		/* Note that components should already be in correct units! */
		mag  = (float) hypot((double)uu, (double)vv);

		/* Angle (from) wrt patch-top (not North) */
		dir = 90 - (180 + fpa_atan2deg(vv, uu));

		barb = create_barb("vectors", pp, dir, mag, 0.0);
		copy_bspec(&barb->bspec, &csvector->bspec);
		add_item_to_set(vectors, (ITEM) barb);
		barb = NULL;
		}

	/* Now check for displaying barbs for all patches */
	else
		{
		mus   = (iu<=0)? 1: 0;
		mvs   = (iv<=0)? 1: 0;

		xfunc = &p->xfunc;
		yfunc = &p->yfunc;

		/* Evalutate wind barbs over the patch */
		for (mu=mus; mu<mult; mu++)
			{
			pp[X] = (float)mu / (float)mult;
			for (mv=mvs; mv<mult; mv++)
				{
				pp[Y] = (float)mv / (float)mult;

				uu  = evaluate_bipoly(xfunc, pp);
				vv  = evaluate_bipoly(yfunc, pp);

				/* Set vector speed */
				/* Note that components should already be in correct units! */
				mag  = (float) hypot((double)uu, (double)vv);

				/* Angle (from) wrt patch-top (not North) */
				dir = 90 - (180 + fpa_atan2deg(vv, uu));

				barb = create_barb("vectors", pp, dir, mag, 0.0);
				copy_bspec(&barb->bspec, &csvector->bspec);
				add_item_to_set(vectors, (ITEM) barb);
				barb = NULL;
				}
			}
		}

	/* Highlight all or nothing */
	hilite = cspecs[0].lspec.hilite;
	if (hilite != SkipHilite) highlight_set(p->vectors, hilite);
	}

/***********************************************************************
*                                                                      *
*      t r a c k _ p a t c h _ f u n c t i o n                         *
*                                                                      *
***********************************************************************/

/*ARGSUSED*/
static	void	track_patch_function

	(
	BIPOLY	*func,
	char	type,
	ILIST	list,
	char	side,
	SET		contours,
	SET		masks,		/* display masks if any */
	int		ncspec,		/* how many contour specs are there? */
	CONSPEC	*cspecs		/* list of contour specs */
	)

	{
	CROOT	*roots;
	int		iroot, nroot, sside, icspec, ncount;
	float	root, cval, slope, aside, as, ae, dx, dy, ax, ay, r;
	double	dxx, dyy;
	STRING	val, lab;
	POINT	p0, p1, p2;
	CURVE	contour;
	LOGICAL	first, found;

	/* Return now if no patch function */
	if (!func) return;
	if (!list) return;

	/* Set up which function type (func/dfdx/dfdy) */
	switch (type)
		{
		case 'f':	nroot = list->numf;	/* f(x,y) = c */
					roots = list->froots;
					break;
		case 'x':	nroot = list->numx;	/* dfdx = 0 */
					roots = list->xroots;
					break;
		case 'y':	nroot = list->numy;	/* dfdy = 0 */
					roots = list->yroots;
					break;
		default:	return;
		}

	/* Set up which side (b/r/t/l) */
	switch (side)
		{
		case 'b':	sside = 1;	/* bottom */
					aside = 0;
					break;
		case 'r':	sside = 1;	/* right */
					aside = PI/2;
					break;
		case 't':	sside = -1;	/* top */
					aside = PI;
					break;
		case 'l':	sside = -1;	/* left */
					aside = 3*PI/2;
					break;
		default:	return;
		}

	/* Track contours of patch function */
	/* Look for a contour at each intersection on current boundary */
	for (iroot=0; iroot<nroot; iroot++)
		{
		/* Get information about this intersection */
		root   = roots[iroot].root;
		cval   = roots[iroot].cval;
		slope  = roots[iroot].slope;
		icspec = roots[iroot].icspec;

		/* Only process contours entering the patch */
		/* Higher values will be on the right when looking into the patch */
		if ((slope != 0) && (SIGN(slope) != sside)) continue;

		/* Determine x and y of initial point depending on which */
		/* side we are starting from */
		switch (side)
			{
			case 'b':   p1[X] = root;	/* bottom */
						p1[Y] = 0;
						break;
			case 'r':   p1[X] = 1;		/* right */
						p1[Y] = root;
						break;
			case 't':   p1[X] = root;	/* top */
						p1[Y] = 1;
						break;
			case 'l':   p1[X] = 0;		/* left */
						p1[Y] = root;
						break;
			}

#		ifdef DEBUG_TRACK
		pr_diag("Tracker",
			"Patch root: %d - %f  Start point: %.2f %.2f\n",
			iroot, root, p1[X], p1[Y]);
#		endif /* DEBUG_TRACK */

		/* Set up beginning of new contour */
		val = contour_val(cval);
		lab = contour_lab(cval);
		contour = create_curve("contour", val, lab);
		if ( NotNull(cspecs) && (icspec >= 0) && (icspec < ncspec))
			{
			copy_lspec(&contour->lspec, &cspecs[icspec].lspec);
			}
		else
			{
			if (type == 'f') (void) printf("Warning: invalid cspec %d:%d %f\n",
										icspec,ncspec,cval);
			define_lspec(&contour->lspec, 0, 0, "", FALSE, 0., 0., 0);
			}
		add_point_to_curve(contour, p1);

		/* Track this contour until it leaves the patch */
		/* ... Use more efficient square-tracker */
		if (TrackSq)
			{
			r = .75*SegLen;
			copy_point(p0, p1);
			copy_point(p2, p1);
			first = TRUE;
			ncount = -1;
			while (TRUE)
				{
				/* Added to stop continuous winding */
				if (contour->line->numpts >= 50)
					{
					pr_diag("Tracker",
						"Break for more than 50 contour points\n");
					break;
					}
				track_patch_contour(func, cval, p0, p1, r, first, p2, &found);
				r = SegLen;

				if  (p2[X] <  0 || p2[X] >  1) break;
				if  (p2[Y] <  0 || p2[Y] >  1) break;
				if ((p2[X] == 0 || p2[X] == 1)
						&& (p2[Y] == 0 || p2[Y] == 1))    break;
				if ((p2[X] == p1[X]) && (p2[Y] == p1[Y])) break;
				dxx = (double) (p2[X] - p1[X]);
				dyy = (double) (p2[Y] - p1[Y]);
				if  (p2[X] == 0 || p2[X] == 1)
					{
					if (fabs(dxx) > fabs(dyy)) break;
					r      = MIN(r, (float) fabs(dyy));
					ncount = 0;
#					ifdef DEBUG_TRACK
					pr_diag("Tracker",
						"Patch edge intersection at (%g %g) ... go on\n",
						p2[X], p2[Y]);
#					endif /* DEBUG_TRACK */
					}
				if  (p2[Y] == 0 || p2[Y] == 1)
					{
					if (fabs(dyy) > fabs(dxx)) break;
					r      = MIN(r, (float) fabs(dxx));
					ncount = 0;
#					ifdef DEBUG_TRACK
					pr_diag("Tracker",
						"Patch edge intersection at (%g %g) ... go on\n",
						p2[X], p2[Y]);
#					endif /* DEBUG_TRACK */
					}

				add_point_to_curve(contour, p2);
				copy_point(p0, p1);
				copy_point(p1, p2);
				first = FALSE;
				if (ncount >=0) ncount++;
				}
			if (ncount == 0)
				{
				pr_diag("Tracker",
					"Patch edge intersection ... exit at impossible point\n");
				}
			else if (ncount == 1)
				{
#				ifdef DEBUG_TRACK
				pr_diag("Tracker",
					"Patch edge intersection ... exit at next point\n");
#				endif /* DEBUG_TRACK */
				}
			else if (ncount > 1)
				{
				pr_diag("Tracker",
					"Patch edge intersection ... exit after %d extra points\n",
					ncount);
				}
			}
		/* ... Use tried-and-true circle-tracker */
		else
			{
			as = aside;
			ae = as + PI;
			r  = SegLen;
			copy_point(p2, p1);
			while (TRUE)
				{
				/* Added to stop continuous winding */
				if (contour->line->numpts >= 50)
					{
					pr_diag("Tracker",
						"Break for more than 50 contour points\n");
					break;
					}
				track_patch_contour_old(func, cval, p1, r, as,ae, NumArc, p2,
						&found);
				if (!found && r>0.05)
					{
					r /= 2;
					continue;
					}
				r = SegLen;

				if  (p2[X] <  0 || p2[X] >  1) break;
				if  (p2[Y] <  0 || p2[Y] >  1) break;
				if ((p2[X] == 0 || p2[X] == 1)
						&& (p2[Y] == 0 || p2[Y] == 1))    break;
				if ((p2[X] == p1[X]) && (p2[Y] == p1[Y])) break;

				/* >>>>> more changes to checking here????? <<<<< */

				add_point_to_curve(contour, p2);
				dx  = p1[X] - p2[X];
				dy  = p1[Y] - p2[Y];
				as  = atan2(dy, dx);
				ae  = as + PI + PI;
				as += ArcGap;
				ae -= ArcGap;
				copy_point(p1, p2);
				}
			}

		/* Clip last point to patch boundary */
		dx = p2[X] - p1[X];
		dy = p2[Y] - p1[Y];
		ax = 1;
		ay = 1;
		if (dx != 0)
			{
			if (p2[X] <= 0) ax = -p1[X] / dx;
			if (p2[X] >= 1) ax = (1 - p1[X]) / dx;
			}
		if (dy != 0)
			{
			if (p2[Y] <= 0) ay = -p1[Y] / dy;
			if (p2[Y] >= 1) ay = (1 - p1[Y]) / dy;
			}
		if (dx!=0 || dy!=0)
			{
			if (ax < ay)
				{
				if (p2[X] <= 0) p2[X] = 0;
				if (p2[X] >= 1) p2[X] = 1;
				p2[Y] = p1[Y] + ax*dy;
				}
			else
				{
				p2[X] = p1[X] + ay*dx;
				if (p2[Y] <= 0) p2[Y] = 0;
				if (p2[Y] >= 1) p2[Y] = 1;
				}
			add_point_to_curve(contour, p2);
			}

#		ifdef DEBUG_TRACK
		pr_diag("Tracker",
			"Patch root: %d - %f  End point:   %.2f %.2f\n",
			iroot, root, p2[X], p2[Y]);
#		endif /* DEBUG_TRACK */

		/* Add contour to curve set */
		add_item_to_set(contours, (ITEM) contour);
		contour = NULL;
		}
	}

/***********************************************************************
*                                                                      *
*      t i d y _ p a t c h _ c o n t o u r s                           *
*                                                                      *
***********************************************************************/

static	void	tidy_patch_contours

	(
	SET		contours,
	ILIST	blist,
	ILIST	rlist,
	ILIST	tlist,
	ILIST	llist
	)

	{
	int		icurv, iroot;
	CURVE	curve;
	float	leave, root, cval, slope, dist, broot, bdist;
	STRING	val, rval;
	POINT	pos;
	ILIST	list;
	char	side, bside;
	int		sside;
	LOGICAL	found, corner;

	static	float	tol  = 0.001;
	static	float	rtol = 0.5;

	/* Do nothing if no contours */
	if (!contours) return;
	if (!llist)    return;
	if (!rlist)    return;
	if (!blist)    return;
	if (!tlist)    return;

	for (icurv=0; icurv<contours->num; icurv++)
		{
		curve = (CURVE) contours->list[icurv];

		/* Make sure the contour value looks like a number */
		get_default_attributes(curve->attrib, NULL, &val, NULL);
		if (sscanf(val, "%f", &cval) < 1) continue;

		copy_point(pos, curve->line->points[curve->line->numpts-1]);

#		ifdef DEBUG_TRACK
		if (pos[X] < 0.0 || pos[X] > 1.0 || pos[Y] < 0.0 || pos[Y] > 1.0)
			pr_diag("Tracker", "Start point error: %f %f\n",
				pos[X], pos[Y]);
#		endif /* DEBUG_TRACK */

		if (pos[Y] <= tol)        side = 'b';	/* bottom */
		else if (pos[X] >= 1-tol) side = 'r';	/* right */
		else if (pos[Y] >= 1-tol) side = 't';	/* top */
		else if (pos[X] <= tol)   side = 'l';	/* left */
		else                      continue;

		corner = FALSE;
		found  = FALSE;
		broot  = 0;
		bdist  = 0;
		bside  = '\0';

		/* Set up which side (b/r/t/l) */
	do_corner:
		switch (side)
			{
			case 'b':	sside = 1;	/* bottom */
						list  = blist;
						leave = pos[X];
						break;
			case 'r':	sside = 1;	/* right */
						list  = rlist;
						leave = pos[Y];
						break;
			case 't':	sside = -1;	/* top */
						list  = tlist;
						leave = pos[X];
						break;
			case 'l':	sside = -1;	/* left */
						list  = llist;
						leave = pos[Y];
						break;
			}


		/* Find the closest exit intersection */
		for (iroot=0; iroot<list->numf; iroot++)
			{
			/* Get information about this intersection */
			root  = list->froots[iroot].root;
			cval  = list->froots[iroot].cval;
			slope = list->froots[iroot].slope;

			/* Only consider roots with the same contour value */
			rval  = contour_val(cval);
			if (!same(rval, val)) continue;

			/* Only consider roots where a contour is supposed to leave */
			/* the patch */
			/* Higher values will be on the right when looking into the patch */
			if ((slope != 0) && (SIGN(slope) == sside)) continue;

			/* Keep track of the closest one */
			dist = fabs((double) leave-root);
			if (dist > rtol) continue;
			if (!found || dist<bdist)
				{
				found = TRUE;
				broot = root;
				bdist = dist;
				bside = side;
				}
			}

		/* Look around the nearest corner if reasonably close */
		if (!corner)
			{
			corner = TRUE;
			switch (side)
				{
				case 't':
				case 'b':   if (pos[X] <= rtol)		/* top/bottom: try left */
								{
								side  = 'l';
								goto do_corner;
								}
							if (pos[X] >= 1-rtol)	/* top/bottom: try right */
								{
								side  = 'r';
								goto do_corner;
								}
							break;

				case 'l':
				case 'r':   if (pos[Y] <= rtol)		/* left/right: try bottom */
								{
								side  = 'b';
								goto do_corner;
								}
							if (pos[Y] >= 1-rtol)	/* left/right: try top */
								{
								side  = 't';
								goto do_corner;
								}
							break;
				}
			}

		/* Move exit point to the closest root if found */
		if (found)
			{
			switch (bside)
				{
				case 'b':   pos[X] = broot;	/* bottom */
							pos[Y] = 0;
							break;
				case 'r':   pos[X] = 1;		/* right */
							pos[Y] = broot;
							break;
				case 't':   pos[X] = broot;	/* top */
							pos[Y] = 1;
							break;
				case 'l':   pos[X] = 0;		/* left */
							pos[Y] = broot;
							break;
				}
			copy_point(curve->line->points[curve->line->numpts-1], pos);
			}
		else
			{
			pr_diag("Tracker",
				"Hyperspace for patch [%d:%d]\n", SaveIUP, SaveIVP);
			}
		}
	}

/***********************************************************************
*                                                                      *
*      t r a c k _ p a t c h _ c o n t o u r                           *
*                                                                      *
*      Find the next point on the current contour in the given patch.  *
*                                                                      *
*      This is done by examining the roots of the patch function       *
*      (minus the contour value) on a constraining square centred      *
*      on the previous point.  Hence the bivariate patch function      *
*      is rendered into 4 univariate function of the parameter 'd'     *
*      representing distance around the square:                        *
*                                                                      *
*           f(d) - c   =   f(x(d),y(d)) - c                            *
*                                                                      *
*      Contours are tracked with higher values to the right, thus      *
*      allowing us to interpret whether points found on the square     *
*      represent exits or entries.  Continuity of spline functions     *
*      guarantees that at least one exit point will be found on the    *
*      square.  If more are found, there must also be corresponding    *
*      entry points.  These can be examined to determine which exit    *
*      point really is the next point on the current contour, and      *
*      not just a point on some nearby contour.                        *
*                                                                      *
***********************************************************************/

typedef	enum
	{
	NOside   = 0,
	Lside    = 1,
	Rside    = 2,
	Bside    = 4,
	Tside    = 8,
	BLcorner = 5,
	BRcorner = 6,
	TLcorner = 9,
	TRcorner = 10
	} SIDE;

static	SIDE	common_side(SIDE s1, SIDE s2)
	{
	int		i1, i2;
	SIDE	sc;

	if (s1 == NOside) return FALSE;
	if (s2 == NOside) return FALSE;

	i1 = (int) s1;
	i2 = (int) s2;
	sc = (SIDE) (i1 & i2);
	return sc;
	}

static	LOGICAL	same_side(SIDE s1, SIDE s2)
	{
	SIDE	sc;

	sc = common_side(s1, s2);
	return (sc==NOside)? FALSE: TRUE;
	}

static	LOGICAL	at_corner(SIDE s1)
	{
	switch (s1)
		{
		case BLcorner:
		case BRcorner:
		case TLcorner:
		case TRcorner:	return TRUE;

		deafult:		return FALSE;
		}
	}

typedef	struct
	{
	POINT	root;
	SIDE	side;
	LOGICAL	entry;
	LOGICAL	dbl;
	} RLIST;

static	void	track_patch_contour

	(
	BIPOLY	*func,	/* patch function */
	float	cval,	/* current contour value */
	POINT	pprev,	/* previous point */
	POINT	pcurr,	/* current point (becomes centre of square) */
	float	r,		/* half-width of constraining square */
	LOGICAL	first,	/* is this the first point in the patch? */
	POINT	pnext,	/* next point on contour returned */
	LOGICAL	*found
	)

	{
	float	yb, xr, yt, xl;
	UNIPOLY	bproj, rproj, tproj, lproj;
	float	roots[ORDER];
	int		iroot, nroot;
	RLIST	rlist[20];
	int		ilist, jlist, nlist=0, imin, iminx, inext, iprev;
	float	aprev, acurr, amid, eps, xtol, ytol;
	double	fval, gval;
	int		sense, ne, nx;
	LOGICAL	dbl, entry, eprev, any, rprev, rnext, add;
	double	dx, dy, dist, dmin, dminx, dprev, dnext;
	POINT	pos;
	SIDE	eside;

	/* Return if nothing to track */
	if (!func)  return;
	if (!pprev) return;
	if (!pcurr) return;
	if (!pnext) return;

	/* Determine extent of square (limit to patch edges) */
	yb = pcurr[Y] - r;	yb = MAX(yb, 0.0);
	xr = pcurr[X] + r;	xr = MIN(xr, 1.0);
	yt = pcurr[Y] + r;	yt = MIN(yt, 1.0);
	xl = pcurr[X] - r;	xl = MAX(xl, 0.0);

	eps  = 0.05*r;
	xtol = 0.01*r;
	ytol = 0.01*r;

	/* What if dx<2xtol or dy<2ytol */
	if ((xr-xl) < (xtol*2))
		{
		xtol = (xr-xl)/2;
		pr_diag("Tracker", "Narrow\n");
		}
	if ((yt-yb) < (ytol*2))
		{
		ytol = (yt-yb)/2;
		pr_diag("Tracker", "Short\n");
		}

	/* Project the patch function onto each of the 4 edges */
	project_bipoly(func, &bproj, 'y', yb);
	project_bipoly(func, &rproj, 'x', xr);
	project_bipoly(func, &tproj, 'y', yt);
	project_bipoly(func, &lproj, 'x', xl);

	/* Find roots on bottom edge */
	/* - We stop slightly inside the corners to avoid dealing with an */
	/*   entry or exit occurring exactly at the corner */
	find_unipoly_roots(&bproj, cval, xl+xtol, xr-xtol, roots, &nroot);
	any   = FALSE;
	aprev = xl+xtol;
	for (iroot=0; iroot<nroot; iroot++)
		{
		acurr = roots[iroot];

		/* Watch for double/triple roots */
		/* - The root finder ensures multiple roots are identically equal */
		/* - Pairs of matching roots are marked as double */
		/* - The third one of a triple root remains as a single */
		if (any && acurr == aprev)
			{
			/* Reverse exit/entry sense for each subsequent root */
			entry = (eprev)? FALSE: TRUE;
			if (!rlist[nlist-1].dbl)
				{
				dbl = TRUE;
				rlist[nlist-1].dbl = dbl;
				}
			}
		else
			{
			/* Check the function value between here and the preceding root */
			/* to determine entry/exit sense */
			amid  = (aprev+acurr)/2;
			fval  = evaluate_unipoly(&bproj, amid) - cval;
			sense = SIGN(fval);
			entry = (LOGICAL)(sense<0);
			dbl   = FALSE;

			/* Make sure entry/exit sense has changed */
			if (any && (entry == eprev))
				{
				/* Only keep one */
				if (acurr-aprev <= xtol)
					{
					aprev = acurr;
					continue;
					}
				pr_warning("Tracker", "Multiple (B) %s within %g\n",
						(entry)? "entry": "exit", (acurr-aprev)/r);
				}
			}

		/* Add the root with exit/entry sense */
		ilist = nlist++;
		rlist[ilist].root[X] = roots[iroot];
		rlist[ilist].root[Y] = yb;
		rlist[ilist].side    = Bside;
		rlist[ilist].entry   = entry;
		rlist[ilist].dbl     = dbl;

		/* Advance to next root */
		any   = TRUE;
		aprev = acurr;
		eprev = entry;
		}

	/* Check for entry/exit close to bottom-right corner */
	fval = evaluate_unipoly(&bproj, xr-xtol) - cval;
	if (fval==0)
		{
		/* Root exactly at end of segment */
		/* Go a bit further ahead, but not past next root */
		find_unipoly_roots(&bproj, cval, xr-xtol, xr, roots, &nroot);
		acurr = xr - xtol/2;
		for (iroot=0; iroot<nroot; iroot++)
			{
			if (roots[iroot] > xr-xtol)
				{
				acurr = (xr-xtol + roots[iroot]) / 2;
				}
			}
		fval = evaluate_unipoly(&bproj, acurr) - cval;
		}
	gval = evaluate_unipoly(&rproj, yb+ytol) - cval;
	if (gval==0)
		{
		/* Root exactly at start of segment */
		/* Go a bit further back, but not before previous root */
		find_unipoly_roots(&rproj, cval, yb, yb+ytol, roots, &nroot);
		acurr = yb + ytol/2;
		for (iroot=nroot-1; iroot>=0; iroot--)
			{
			if (roots[iroot] < yb+ytol)
				{
				acurr = (yb+ytol + roots[iroot]) / 2;
				}
			}
		gval = evaluate_unipoly(&rproj, acurr) - cval;
		}
	if (SIGN(gval) != SIGN(fval))
		{
		acurr = xr;
		sense = SIGN(fval);
		entry = (LOGICAL)(sense<0);
		dbl   = FALSE;
		add   = TRUE;

		/* Make sure entry/exit sense has changed */
		if (any && (entry == eprev))
			{
			/* Only keep one */
			if (acurr-aprev <= xtol)
				{
				aprev = acurr;
				add = FALSE;
				}
			pr_warning("Tracker", "Multiple (B-R) %s within %g\n",
					(entry)? "entry": "exit", (acurr-aprev)/r);
			}

		/* Add the root with exit/entry sense */
		if (add)
			{
			ilist = nlist++;
			rlist[ilist].root[X] = xr;
			rlist[ilist].root[Y] = yb;
			rlist[ilist].side    = BRcorner;
			rlist[ilist].entry   = entry;
			rlist[ilist].dbl     = dbl;

			/* Advance to next root */
			any   = TRUE;
			aprev = acurr;
			eprev = entry;
			}
		}

	/* Find roots on right edge */
	/* - We stop slightly inside the corners to avoid dealing with an */
	/*   entry or exit occurring exactly at the corner */
	find_unipoly_roots(&rproj, cval, yb+ytol, yt-ytol, roots, &nroot);
	any   = FALSE;
	aprev = yb+ytol;
	for (iroot=0; iroot<nroot; iroot++)
		{
		acurr = roots[iroot];

		/* Watch for double/triple roots */
		/* - The root finder ensures multiple roots are identically equal */
		/* - Pairs of matching roots are marked as double */
		/* - The third one of a triple root remains as a single */
		if (any && acurr == aprev)
			{
			/* Reverse exit/entry sense for each subsequent root */
			entry = (eprev)? FALSE: TRUE;
			if (!rlist[nlist-1].dbl)
				{
				dbl = TRUE;
				rlist[nlist-1].dbl = dbl;
				}
			}
		else
			{
			/* Check the function value between here and the preceding root */
			/* to determine entry/exit sense */
			amid  = (aprev+acurr)/2;
			fval  = evaluate_unipoly(&rproj, amid) - cval;
			sense = SIGN(fval);
			entry = (LOGICAL)(sense<0);
			dbl   = FALSE;

			/* Make sure entry/exit sense has changed */
			if (any && (eprev == entry))
				{
				/* Only keep one */
				if (acurr-aprev <= ytol)
					{
					aprev = acurr;
					continue;
					}
				pr_warning("Tracker", "Multiple (R) %s within %g\n",
						(entry)? "entry": "exit", (acurr-aprev)/r);
				}
			}

		/* Add the root with exit/entry sense */
		ilist = nlist++;
		rlist[ilist].root[X] = xr;
		rlist[ilist].root[Y] = roots[iroot];
		rlist[ilist].side    = Rside;
		rlist[ilist].entry   = entry;
		rlist[ilist].dbl     = dbl;

		/* Advance to next root */
		any   = TRUE;
		aprev = acurr;
		eprev = entry;
		}

	/* Check for entry/exit close to top-right corner */
	fval = evaluate_unipoly(&rproj, yt-ytol) - cval;
	if (fval==0)
		{
		/* Root exactly at end of segment */
		/* Go a bit further ahead, but not past next root */
		find_unipoly_roots(&rproj, cval, yt-ytol, yt, roots, &nroot);
		acurr = yt - ytol/2;
		for (iroot=0; iroot<nroot; iroot++)
			{
			if (roots[iroot] > yt-ytol)
				{
				acurr = (yt-ytol + roots[iroot]) / 2;
				}
			}
		fval = evaluate_unipoly(&rproj, acurr) - cval;
		}
	gval = evaluate_unipoly(&tproj, xr-xtol) - cval;
	if (gval==0)
		{
		/* Root exactly at start of segment */
		/* Go a bit further back, but not before previous root */
		find_unipoly_roots(&tproj, cval, xr-xtol, xr, roots, &nroot);
		acurr = xr - xtol/2;
		for (iroot=0; iroot<nroot; iroot++)
			{
			if (roots[iroot] > xr-xtol)
				{
				acurr = (xr-xtol + roots[iroot]) / 2;
				}
			}
		gval = evaluate_unipoly(&tproj, acurr) - cval;
		}
	if (SIGN(gval) != SIGN(fval))
		{
		acurr = yt;
		sense = SIGN(fval);
		entry = (LOGICAL)(sense<0);
		dbl   = FALSE;
		add   = TRUE;

		/* Make sure entry/exit sense has changed */
		if (any && (entry == eprev))
			{
			/* Only keep one */
			if (acurr-aprev <= ytol)
				{
				aprev = acurr;
				add = FALSE;
				}
			pr_warning("Tracker", "Multiple (T-R) %s within %g\n",
					(entry)? "entry": "exit", (acurr-aprev)/r);
			}

		/* Add the root with exit/entry sense */
		if (add)
			{
			ilist = nlist++;
			rlist[ilist].root[X] = xr;
			rlist[ilist].root[Y] = yt;
			rlist[ilist].side    = TRcorner;
			rlist[ilist].entry   = entry;
			rlist[ilist].dbl     = dbl;

			/* Advance to next root */
			any   = TRUE;
			aprev = acurr;
			eprev = entry;
			}
		}

	/* Find roots on top edge */
	/* - We stop slightly inside the corners to avoid dealing with an */
	/*   entry or exit occurring exactly at the corner */
	find_unipoly_roots(&tproj, cval, xl+xtol, xr-xtol, roots, &nroot);
	any   = FALSE;
	aprev = xr-xtol;
	for (iroot=nroot-1; iroot>=0; iroot--)
		{
		acurr = roots[iroot];

		/* Watch for double/triple roots */
		/* - The root finder ensures multiple roots are identically equal */
		/* - Pairs of matching roots are marked as double */
		/* - The third one of a triple root remains as a single */
		if (any && acurr == aprev)
			{
			/* Reverse exit/entry sense for each subsequent root */
			entry = (eprev)? FALSE: TRUE;
			if (!rlist[nlist-1].dbl)
				{
				dbl = TRUE;
				rlist[nlist-1].dbl = dbl;
				}
			}
		else
			{
			/* Check the function value between here and the preceding root */
			/* to determine entry/exit sense */
			amid  = (aprev+acurr)/2;
			fval  = evaluate_unipoly(&tproj, amid) - cval;
			sense = SIGN(fval);
			entry = (LOGICAL)(sense<0);
			dbl   = FALSE;

			/* Make sure entry/exit sense has changed */
			if (any && (eprev == entry))
				{
				/* Only keep one */
				if (aprev-acurr <= xtol)
					{
					aprev = acurr;
					continue;
					}
				pr_warning("Tracker", "Multiple (T) %s within %g\n",
						(entry)? "entry": "exit", (acurr-aprev)/r);
				}
			}

		/* Add the root with exit/entry sense */
		ilist = nlist++;
		rlist[ilist].root[X] = roots[iroot];
		rlist[ilist].root[Y] = yt;
		rlist[ilist].side    = Tside;
		rlist[ilist].entry   = entry;
		rlist[ilist].dbl     = dbl;

		/* Advance to next root */
		any   = TRUE;
		aprev = acurr;
		eprev = entry;
		}

	/* Check for entry/exit close to top-left corner */
	fval = evaluate_unipoly(&tproj, xl+xtol) - cval;
	if (fval==0)
		{
		/* Root exactly at end of segment */
		/* Go a bit further ahead, but not past next root */
		find_unipoly_roots(&tproj, cval, xl, xl+xtol, roots, &nroot);
		acurr = xl + xtol/2;
		for (iroot=nroot-1; iroot>=0; iroot--)
			{
			if (roots[iroot] < xl-xtol)
				{
				acurr = (xl-xtol + roots[iroot]) / 2;
				}
			}
		fval = evaluate_unipoly(&tproj, acurr) - cval;
		}
	gval = evaluate_unipoly(&lproj, yt-ytol) - cval;
	if (gval==0)
		{
		/* Root exactly at start of segment */
		/* Go a bit further back, but not before previous root */
		find_unipoly_roots(&lproj, cval, yt-ytol, yt, roots, &nroot);
		acurr = yt - ytol/2;
		for (iroot=0; iroot<nroot; iroot++)
			{
			if (roots[iroot] > yt-ytol)
				{
				acurr = (yt-ytol + roots[iroot]) / 2;
				}
			}
		gval = evaluate_unipoly(&lproj, acurr) - cval;
		}
	if (SIGN(gval) != SIGN(fval))
		{
		acurr = xl;
		sense = SIGN(fval);
		entry = (LOGICAL)(sense<0);
		dbl   = FALSE;
		add   = TRUE;

		/* Make sure entry/exit sense has changed */
		if (any && (entry == eprev))
			{
			/* Only keep one */
			if (acurr-aprev <= xtol)
				{
				aprev = acurr;
				add = FALSE;
				}
			pr_warning("Tracker", "Multiple (T-L) %s within %g\n",
					(entry)? "entry": "exit", (acurr-aprev)/r);
			}

		/* Add the root with exit/entry sense */
		if (add)
			{
			ilist = nlist++;
			rlist[ilist].root[X] = xl;
			rlist[ilist].root[Y] = yt;
			rlist[ilist].side    = TLcorner;
			rlist[ilist].entry   = entry;
			rlist[ilist].dbl     = dbl;

			/* Advance to next root */
			any   = TRUE;
			aprev = acurr;
			eprev = entry;
			}
		}

	/* Find roots on left edge */
	/* - We stop slightly inside the corners to avoid dealing with an */
	/*   entry or exit occurring exactly at the corner */
	find_unipoly_roots(&lproj, cval, yb+ytol, yt-ytol, roots, &nroot);
	any   = FALSE;
	aprev = yt-ytol;
	for (iroot=nroot-1; iroot>=0; iroot--)
		{
		acurr = roots[iroot];

		/* Watch for double/triple roots */
		/* - The root finder ensures multiple roots are identically equal */
		/* - Pairs of matching roots are marked as double */
		/* - The third one of a triple root remains as a single */
		if (any && acurr == aprev)
			{
			/* Reverse exit/entry sense for each subsequent root */
			entry = (eprev)? FALSE: TRUE;
			if (!rlist[nlist-1].dbl)
				{
				dbl = TRUE;
				rlist[nlist-1].dbl = dbl;
				}
			}
		else
			{
			/* Check the function value between here and the preceding root */
			/* to determine entry/exit sense */
			amid  = (aprev+acurr)/2;
			fval  = evaluate_unipoly(&lproj, amid) - cval;
			sense = SIGN(fval);
			entry = (LOGICAL)(sense<0);
			dbl   = FALSE;

			/* Make sure entry/exit sense has changed */
			if (any && (eprev == entry))
				{
				/* Only keep one */
				if (aprev-acurr <= ytol)
					{
					aprev = acurr;
					continue;
					}
				pr_warning("Tracker", "Multiple (L) %s within %g\n",
						(entry)? "entry": "exit", (acurr-aprev)/r);
				}
			}

		/* Add the root with exit/entry sense */
		ilist = nlist++;
		rlist[ilist].root[X] = xl;
		rlist[ilist].root[Y] = roots[iroot];
		rlist[ilist].side    = Lside;
		rlist[ilist].entry   = entry;
		rlist[ilist].dbl     = dbl;

		/* Advance to next root */
		any   = TRUE;
		aprev = acurr;
		eprev = entry;
		}

	/* Check for entry/exit close to bottom-left corner */
	fval = evaluate_unipoly(&lproj, yb+ytol) - cval;
	if (fval==0)
		{
		/* Root exactly at end of segment */
		/* Go a bit further ahead, but not past next root */
		find_unipoly_roots(&lproj, cval, yb, yb+ytol, roots, &nroot);
		acurr = yb + ytol/2;
		for (iroot=nroot-1; iroot>=0; iroot--)
			{
			if (roots[iroot] > yb+ytol)
				{
				acurr = (yb+ytol + roots[iroot]) / 2;
				}
			}
		fval = evaluate_unipoly(&lproj, acurr) - cval;
		}
	gval = evaluate_unipoly(&bproj, xl+xtol) - cval;
	if (gval==0)
		{
		/* Root exactly at start of segment */
		/* Go a bit further back, but not before previous root */
		find_unipoly_roots(&bproj, cval, xl, xl+xtol, roots, &nroot);
		acurr = xl + xtol/2;
		for (iroot=nroot-1; iroot>=0; iroot--)
			{
			if (roots[iroot] < xl+xtol)
				{
				acurr = (xl+xtol + roots[iroot]) / 2;
				}
			}
		gval = evaluate_unipoly(&bproj, acurr) - cval;
		}
	if (SIGN(gval) != SIGN(fval))
		{
		acurr = yb;
		sense = SIGN(fval);
		entry = (LOGICAL)(sense<0);
		dbl   = FALSE;
		add   = TRUE;

		/* Make sure entry/exit sense has changed */
		if (any && (entry == eprev))
			{
			/* Only keep one */
			if (acurr-aprev <= ytol)
				{
				aprev = acurr;
				add = FALSE;
				}
			pr_warning("Tracker", "Multiple (B-L) %s within %g\n",
					(entry)? "entry": "exit", (acurr-aprev)/r);
			}

		/* Add the root with exit/entry sense */
		if (add)
			{
			ilist = nlist++;
			rlist[ilist].root[X] = xl;
			rlist[ilist].root[Y] = yb;
			rlist[ilist].side    = BLcorner;
			rlist[ilist].entry   = entry;
			rlist[ilist].dbl     = dbl;

			/* Advance to next root */
			any   = TRUE;
			aprev = acurr;
			eprev = entry;
			}
		}


	/* Got all the roots in order */
	/* Should be evenly matched */
	ne = 0;
	nx = 0;
	for (ilist=0; ilist<nlist; ilist++)
		{
		if (rlist[ilist].entry) ne++;
		else                    nx++;
		}
	if (nx != ne)
		{
		char	buf[20];
		pr_warning("Tracker", "Entry/Exit Mismatch\n");
		for (ilist=0; ilist<nlist; ilist++)
			{
			(void) strcpy(buf, "");
			if (rlist[ilist].entry) (void) strcat(buf, "e");
			else                    (void) strcat(buf, "x");
			}
		pr_diag("Tracker", "%s\n", buf);
		}

	/* Determine which root pprev corresponds to */
	eside = NOside;
	if (pprev[X] < xl+xtol)
		{
		eside = Lside;
		}
	else if (pprev[X] > xr-xtol)
		{
		eside = Rside;
		}
	if (pprev[Y] < yb+ytol)
		{
		if (eside == Lside)      eside = BLcorner;
		else if (eside == Rside) eside = BRcorner;
		else                     eside = Bside;
		}
	else if (pprev[Y] > yt-ytol)
		{
		if (eside == Lside)      eside = TLcorner;
		else if (eside == Rside) eside = TRcorner;
		else                     eside = Tside;
		}
	if (eside == NOside)
		{
		pr_warning("Tracker", "Prev point not on bound\n");
		pr_diag("Tracker", "(%g, %g) ~ (%g, %g) [%g, %g][%g, %g]\n",
				pprev[X], pprev[Y], pcurr[X], pcurr[Y],
				xl+xtol, xr-xtol, yb+ytol, yt-ytol);
		dx = pprev[X] - pcurr[X];
		dy = pprev[Y] - pcurr[Y];
		pr_diag("Tracker", "  dx: %g  dy: %g  r: %g  xtol: %g  ytol: %g\n",
				dx, dy, r, xtol, ytol);
		if (found) *found = FALSE;
		return;
		}
	imin = nlist;	iminx = nlist;
	dmin = 2.0;		dminx = 2.0;
	for (ilist=0; ilist<nlist; ilist++)
		{
		dx   = pprev[X] - rlist[ilist].root[X];
		dy   = pprev[Y] - rlist[ilist].root[Y];
		dist = hypot(dx, dy);
		/* bias for entries on the same side */
		if (!same_side(eside, rlist[ilist].side)) dist += eps;
		if (dist < dmin)
			{
			if (rlist[ilist].entry)
				{
				imin = ilist;
				dmin = dist;
				}
			else
				{
				iminx = ilist;
				dminx = dist;
				}
			}
		}
	if (imin<nlist && iminx<nlist && dminx<dmin)
		{
		/* An exit is closer to the previous point than an entry */
		/* Treat as a double root and use the entry anyway */
		if (!rlist[iminx].dbl)
			{
			rlist[imin].dbl  = TRUE;
			rlist[iminx].dbl = TRUE;
			}
		}
	if (imin >= nlist)
		{
		/* OK if near corner (could be hidden double root) */
		if (at_corner(eside))
			{
			if (same_side(eside, Lside)) pnext[X] = xl;
			else                         pnext[X] = xr;
			if (same_side(eside, Bside)) pnext[Y] = yb;
			else                         pnext[Y] = yt;
			pr_diag("Tracker",
				"No matching Entry found - Hidden double root detected\n");
			if (found) *found = TRUE;
			return;
			}
		/* OK if only one exit anyway */
		if (nlist == 1)
			{
			ilist = 0;
			copy_point(pnext, rlist[ilist].root);
			pr_diag("Tracker",
				"No matching Entry found - Lone mis-matched exit used\n");
			if (found) *found = TRUE;
			return;
			}
		pr_warning("Tracker", "No matching Entry found\n");
		if (found) *found = FALSE;
		return;
		}

	/* If we entered at a double root, remove the matching exit from */
	/* contention, unless its the only exit, or this is the first point */
	/* in the patch */
	if (!first && nx>1 && rlist[imin].dbl)
		{
		/* Check before and after for matching exit */
		iprev = (imin+nlist-1)%nlist;
		inext = (imin+1)%nlist;

		rprev = (LOGICAL)(rlist[iprev].dbl && !rlist[iprev].entry);
		rnext = (LOGICAL)(rlist[inext].dbl && !rlist[inext].entry);
		if (iprev == inext) rnext = FALSE;

		if (rprev && rnext)
			{
			/* Surrounded by double roots - pick closest */
			dx    = rlist[iprev].root[X] - rlist[imin].root[X];
			dy    = rlist[iprev].root[Y] - rlist[imin].root[Y];
			dprev = hypot(dx, dy);
			dx    = rlist[inext].root[X] - rlist[imin].root[X];
			dy    = rlist[inext].root[Y] - rlist[imin].root[Y];
			dnext = hypot(dx, dy);
			if (dnext < dprev) rprev = FALSE;
			else               rnext = FALSE;
			}

		if (rprev)
			{
			/* Remove previous exit */
			for (ilist=iprev+1; ilist<nlist; ilist++)
				{
				rlist[ilist-1].root[X] = rlist[ilist].root[X];
				rlist[ilist-1].root[Y] = rlist[ilist].root[Y];
				rlist[ilist-1].side    = rlist[ilist].side;
				rlist[ilist-1].entry   = rlist[ilist].entry;
				rlist[ilist-1].dbl     = rlist[ilist].dbl;
				}
			nlist--;
			nx--;
			if (imin > iprev) imin--;

			/* Also remove preceding entry */
			iprev = (imin+nlist-1)%nlist;
			if (rlist[iprev].entry)
				{
				for (ilist=iprev+1; ilist<nlist; ilist++)
					{
					rlist[ilist-1].root[X] = rlist[ilist].root[X];
					rlist[ilist-1].root[Y] = rlist[ilist].root[Y];
					rlist[ilist-1].side    = rlist[ilist].side;
					rlist[ilist-1].entry   = rlist[ilist].entry;
					rlist[ilist-1].dbl     = rlist[ilist].dbl;
					}
				nlist--;
				ne--;
				if (imin > iprev) imin--;
				}
			}

		else if (rnext)
			{
			/* Remove next exit */
			for (ilist=inext+1; ilist<nlist; ilist++)
				{
				rlist[ilist-1].root[X] = rlist[ilist].root[X];
				rlist[ilist-1].root[Y] = rlist[ilist].root[Y];
				rlist[ilist-1].side    = rlist[ilist].side;
				rlist[ilist-1].entry   = rlist[ilist].entry;
				rlist[ilist-1].dbl     = rlist[ilist].dbl;
				}
			nlist--;
			nx--;
			if (imin > inext) imin--;

			/* Also remove following entry */
			inext = (imin+1)%nlist;
			if (rlist[inext].entry)
				{
				for (ilist=inext+1; ilist<nlist; ilist++)
					{
					rlist[ilist-1].root[X] = rlist[ilist].root[X];
					rlist[ilist-1].root[Y] = rlist[ilist].root[Y];
					rlist[ilist-1].side    = rlist[ilist].side;
					rlist[ilist-1].entry   = rlist[ilist].entry;
					rlist[ilist-1].dbl     = rlist[ilist].dbl;
					}
				nlist--;
				ne--;
				if (imin > inext) imin--;
				}
			}
		}

	/* There should always be at least 1 exit unless we are just */
	/* entering a patch - could be multiple root */
	if (nx <= 0)
		{
		pr_diag("Tracker", "No Exit\n");
		if (found) *found = FALSE;
		return;
		}

	/* Usual case of 1 entry & 1 exit --> Take it! */
	if (nx == 1)
		{
		for (ilist=0; ilist<nlist; ilist++)
			{
			if (!rlist[ilist].entry)
				{
				/* Return the new point */
				copy_point(pnext, rlist[ilist].root);
				if (found) *found = TRUE;
				return;
				}
			}
		pr_diag("Tracker", "No Exit\n");
		if (found) *found = FALSE;
		return;
		}

	/* Dilema over which exit to pick when more than one are found */
	/* Sample mid-way between central point and each entry point */
	/*    - if this is negative pick the preceding exit */
	/*    - if positive check the next entry */
	/*    - if all are positive use the last exit */
	inext = (imin+nlist-1)%nlist;
	for (ilist=1; ilist<nlist; ilist++)
		{
		jlist = (imin+ilist)%nlist;
		if (!rlist[jlist].entry) continue;

		pos[X] = .25*(pcurr[X] + 3*rlist[jlist].root[X]);
		pos[Y] = .25*(pcurr[Y] + 3*rlist[jlist].root[Y]);
		fval   = evaluate_bipoly(func, pos) - cval;
		sense  = SIGN(fval);
		if (sense < 0)
			{
			inext = (imin+ilist+nlist-1)%nlist;
			break;
			}

		pos[X] = .5*(pcurr[X] + rlist[jlist].root[X]);
		pos[Y] = .5*(pcurr[Y] + rlist[jlist].root[Y]);
		fval   = evaluate_bipoly(func, pos) - cval;
		sense  = SIGN(fval);
		if (sense < 0)
			{
			inext = (imin+ilist+nlist-1)%nlist;
			break;
			}

		pos[X] = .25*(3*pcurr[X] + rlist[jlist].root[X]);
		pos[Y] = .25*(3*pcurr[Y] + rlist[jlist].root[Y]);
		fval   = evaluate_bipoly(func, pos) - cval;
		sense  = SIGN(fval);
		if (sense < 0)
			{
			inext = (imin+ilist+nlist-1)%nlist;
			break;
			}
		}

	/* Return the new point */
	copy_point(pnext, rlist[inext].root);
	if (found) *found = TRUE;
	}

/***********************************************************************
*                                                                      *
*      t r a c k _ p a t c h _ c o n t o u r                           *
*                                                                      *
*      (Obsolesscent)                                                  *
*                                                                      *
*      Find the next point on the current contour in the given patch.  *
*                                                                      *
*      This is done by examining the roots of the patch function       *
*      (minus the contour value) on a constraining circle centred      *
*      on the previous point.  Hence the bivariate patch function      *
*      is rendered into a univariate function of the parameter 'a'     *
*      representing angle around the circle:                           *
*                                                                      *
*           f(a) - c   =   f(x(a),y(a)) - c                            *
*                                                                      *
*      Contours are tracked with higher values to the right, thus      *
*      allowing us to interpret whether points found on the circle     *
*      represent exits or entries.  Continuity of spline functions     *
*      guarantees that at least one exit point will be found on the    *
*      circle.  If more are found, there must also be corresponding    *
*      entry points.  These can be examined to determine which exit    *
*      point really is the next point on the current contour, and      *
*      not just a point on some nearby contour.                        *
*                                                                      *
***********************************************************************/

static	void	track_patch_contour_old

	(
	BIPOLY	*f,		/* patch function */
	float	cval,	/* current contour value */
	POINT	pcurr,	/* current point (becomes centre of circle) */
	float	r,		/* radius of constraining circle */
	float	as,		/* start angle on circle */
	float	ae,		/* end angle on circle */
	int		nseg,	/* number of segments to sub-divede circle into */
	POINT	pnext,	/* next point on contour returned */
	LOGICAL	*found
	)

	{
	float	uc, vc, da, a, entries[3], exits[3];
	float	a1, f1;
	float	a2, f2;
	POINT	p1, p2;
	int		iseg, s1, s2, sd, nument, numex;

	/* Return if nothing to track */
	if (!f)     return;
	if (!pcurr) return;
	if (!pnext) return;

	/* Remember central point and angle increment */
	uc = pnext[X] = pcurr[X];
	vc = pnext[Y] = pcurr[Y];
	da = (ae - as) / nseg;
	sd = SIGN(da);
	nument = 0;
	numex  = 0;
	if (found) *found = FALSE;

	/* Set up start of first segment */
	a1    = as;
	p1[X] = uc + r*cos(a1);
	p1[Y] = vc + r*sin(a1);
	f1    = (float) evaluate_bipoly(f, p1) - cval;
	s1    = SIGN(f1);

	/* Scan each segment for entry or exit */
	for (iseg=0; iseg<nseg; iseg++)
		{
		/* Set up end of current segment */
		a2    = a1 + da;
		p2[X] = uc + r*cos(a2);
		p2[Y] = vc + r*sin(a2);
		f2    = (float) evaluate_bipoly(f, p2) - cval;
		s2    = SIGN(f2);

		/* If signs at ends of segment are opposite, a root */
		/* must exist on the segment */
		if (s1 != s2)
			{
			a = (float) zeroin_bipoly(f, cval, pcurr, r, a1, a2, ZeroTol);
			if (s2 == sd)	entries[nument++] = a;
			else		exits[numex++]    = a;
			}

		/* Move along to next segment */
		a1    = a2;
		p1[X] = p2[X];
		p1[Y] = p2[Y];
		f1    = f2;
		s1    = s2;
		}

	/* There should always be at least 1 exit unless we are just */
	/* entering a patch - could be multiple root */
	if (numex <= 0) return;

	/* Usual case of 1 exit */
	if (numex == 1) a = exits[0];

	/* Dilema over which exit to pick when more than one are found */
	/* Sample mid-way between central point and other entry point */
	/*    - if this is positive pick the last exit */
	/*    - if negative pick first exit */
	else
		{
		a     = entries[0];
		p1[X] = uc + .5*r*cos(a);
		p1[Y] = vc + .5*r*sin(a);
		f1    = (float) evaluate_bipoly(f, p1) - cval;
		s1    = SIGN(f1);
		a     = (s1 == sd) ? exits[numex-1] : exits[0];
		}

	/* Return the new point */
	pnext[X] += r*cos(a);
	pnext[Y] += r*sin(a);
	if (found) *found = TRUE;
	}

/***********************************************************************
*                                                                      *
*      r e s e t _ p a t c h _ c o n s p e c                           *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Reset the patch contour colours and line styles consistent
 *  with the given contour specs.  Any that don't fit are made
 *  invisible.
 *
 *	@param[in] 	p		patch to be reset
 *	@param[in] 	ncspec	how many contour specs are there?
 *	@param[in] 	*cspecs	list of contour specs
 **********************************************************************/

void	reset_patch_conspec

	(
	PATCH	p,		/* patch to be reset */
	int		ncspec,	/* how many contour specs are there? */
	CONSPEC	*cspecs	/* list of contour specs */
	)

	{
	int		icont, ncont, ics, ival, icval, ndif;
	int		hilite;
	float	cval, fcval;
	SET		contours, extrema, vectors;
	CURVE	contour;
	MARK	mark;
	BARB	barb;
	STRING	sub, val, c;
	CONSPEC	*cspec, *cmatch;

	/* Do nothing if no patch or contour specs */
	if (!p)          return;
	if (ncspec <= 0) return;
	if (!cspecs)     return;

	/* Reset each patch contour according to its value */
	contours = p->contours;
	ncont = 0;
	if (contours) ncont = contours->num;
	for (icont=0; icont<ncont; icont++)
		{
		/* Make this contour invisible */
		contour = (CURVE) contours->list[icont];
		hilite  = contour->lspec.hilite;
		contour->lspec.hilite = -1;

		/* Find value given to this contour */
		get_default_attributes(contour->attrib, NULL, &val, NULL);
		if (blank(val)) continue;
		fcval = (float) strtod(val, &c);
		if (c == val)   continue;
		icval = NINT(fcval);

		/* Now search all the contour specs to find one that matches */
		cmatch = (CONSPEC *) NULL;
		for (ics=0; ics<ncspec; ics++)
			{
			cspec = cspecs + ics;
			if (same(cspec->type, "range"))
				{
				if (fcval < cspec->cmin) continue;
				if (fcval > cspec->cmax) continue;
				ndif = ceil((fcval-cspec->cstd)/cspec->cint);
				cval = cspec->cstd + cspec->cint*ndif;
				if (NINT(cval) == icval)
					{
					cmatch = cspec;
					break;
					}
				}
			else if (same(cspec->type, "list"))
				{
				for (ival=0; ival<cspec->nval; ival++)
					{
					cval = cspec->cvals[ival];
					if (NINT(cval) == icval)
						{
						cmatch = cspec;
						break;
						}
					}
				}
			}

		/* If we found a matching contour spec set the line attributes */
		if (cmatch)
			{
			contour->lspec.hilite = hilite;
			copy_lspec(&contour->lspec, &cmatch->lspec);
			}
		}

	/* Reset patch maxima, minima and saddle points */
	extrema = p->extrema;
	ncont = 0;
	if (extrema) ncont = extrema->num;
	for (icont=0; icont<ncont; icont++)
		{
		/* Make this mark invisible */
		mark   = (MARK) extrema->list[icont];
		hilite = mark->mspec.hilite;
		mark->mspec.hilite = -1;

		/* Find value given to this mark */
		get_default_attributes(mark->attrib, &sub, &val, NULL);
		if (blank(val)) continue;
		fcval = (float) strtod(val, &c);
		if (c == val)   continue;
		icval = NINT(fcval);

		/* Search for a matching spec */
		cmatch = (CONSPEC *) NULL;
		for (ics=0; ics<ncspec; ics++)
			{
			cspec = cspecs + ics;
			if (same(cspec->type, sub))
				{
				if (fcval < cspec->cmin) continue;
				if (fcval > cspec->cmax) continue;
				cmatch = cspec;
				break;
				}
			}

		/* If we found a matching spec set the mark attributes */
		if (cmatch)
			{
			mark->mspec.hilite = hilite;
			copy_mspec(&mark->mspec, &cmatch->mspec);
			}
		}

	/* Reset patch vectors */
	vectors = p->vectors;
	ncont = 0;
	if (vectors) ncont = vectors->num;
	for (icont=0; icont<ncont; icont++)
		{
		/* Make this barb invisible */
		barb   = (BARB) vectors->list[icont];
		hilite = barb->bspec.hilite;
		barb->bspec.hilite = -1;

		/* Search for a matching spec */
		cmatch = (CONSPEC *) NULL;
		for (ics=0; ics<ncspec; ics++)
			{
			cspec = cspecs + ics;
			if (same(cspec->type, "vector"))
				{
				cmatch = cspec;
				break;
				}
			}

		/* If we found a matching spec set the barb attributes */
		if (cmatch)
			{
			barb->bspec.hilite = hilite;
			copy_bspec(&barb->bspec, &cmatch->bspec);
			}
		}
	}

/***********************************************************************
*                                                                      *
*      c r e a t e _ i l i s t                                         *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Create an empty ILIST object.
 *
 *  @return newly created ilist object (memory allocated)
 **********************************************************************/

ILIST	create_ilist (void)

	{
	ILIST	list;

	/* Allocate memory for the structure */
	list = INITMEM(struct ILIST_struct, 1);

	/* Initialize the structure */
	list->froots  = NULL;
	list->xroots  = NULL;
	list->yroots  = NULL;
	list->numf    = 0;
	list->maxf    = 0;
	list->numx    = 0;
	list->numy    = 0;
	list->defined = FALSE;

	return list;
	}

/***********************************************************************
*                                                                      *
*      d e s t r o y _ i l i s t                                       *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Deallocate an ILIST.
 *
 *	@param[in] 	list	given ILIST to be destroyed
 *	@return NullIlist
 **********************************************************************/

ILIST	destroy_ilist

	(
	ILIST	list	/* given ILIST to be destroyed */
	)

	{
	if (!list) return NullIlist;

	free_ilist(list);
	FREEMEM(list);
	return NullIlist;
	}

/***********************************************************************
*                                                                      *
*      f r e e _ i l i s t                                             *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Free all the space used by the given ilist and turn it into
 *  an empty ilist.
 *
 *	@param[in] 	list	ILIST to be freed
 **********************************************************************/

void	free_ilist

	(
	ILIST	list	/* given ILIST to be freed */
	)

	{
	/* Do nothing if null */
	if (!list) return;

	/* Zero everything */
	FREEMEM(list->froots);
	FREEMEM(list->xroots);
	FREEMEM(list->yroots);
	list->numf    = 0;
	list->maxf    = 0;
	list->numx    = 0;
	list->numy    = 0;
	list->defined = FALSE;
	}

/***********************************************************************
*                                                                      *
*      e m p t y _ i l i s t                                           *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Zero all intersections from the given intersection list.
 *
 *	@param[in] 	list	specified ILIST
 **********************************************************************/

void	empty_ilist

	(
	ILIST	list	/* specified ILIST */
	)

	{
	/* Do nothing if ilist undefined */
	if (!list) return;

	/* Set basic parameters */
	if (!list->froots) list->maxf = 0;
	list->numf    = 0;
	list->numx    = 0;
	list->numy    = 0;
	list->defined = FALSE;
	}

/***********************************************************************
*                                                                      *
*      c o p y _ i l i s t                                             *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Copy an intersection list.
 *
 *	@param[in] 	list	ilist to be copied
 *	@return	copy of the ilist object
 **********************************************************************/

ILIST	copy_ilist

	(
	const ILIST	list	/* ilist to be copied */
	)

	{
	ILIST	lnew=NullIlist;	/* new ilist */
	int		i, num;

	/* Do nothing if ilist undefined */
	if (!list)          return lnew;
	lnew = create_ilist();
	if (!list->defined) return lnew;

	/* Copy roots of f(x,y) = c */
	num = list->numf;
	if (num > 0)
		{
		lnew->froots = INITMEM(CROOT, num);
		lnew->maxf   = num;
		}
	for (i=0; i<num; i++)
		{
		lnew->froots[i].cval   = list->froots[i].cval;
		lnew->froots[i].root   = list->froots[i].root;
		lnew->froots[i].slope  = list->froots[i].slope;
		lnew->froots[i].icspec = list->froots[i].icspec;
		}

	/* Copy roots of dfdx = 0 */
	num = list->numx;
	if (num > 0) lnew->xroots = INITMEM(CROOT, num);
	for (i=0; i<num; i++)
		{
		lnew->xroots[i].cval   = list->xroots[i].cval;
		lnew->xroots[i].root   = list->xroots[i].root;
		lnew->xroots[i].slope  = list->xroots[i].slope;
		lnew->xroots[i].icspec = list->xroots[i].icspec;
		}

	/* Copy roots of dfdy = 0 */
	num = list->numy;
	if (num > 0) lnew->yroots = INITMEM(CROOT, num);
	for (i=0; i<num; i++)
		{
		lnew->yroots[i].cval   = list->yroots[i].cval;
		lnew->yroots[i].root   = list->yroots[i].root;
		lnew->yroots[i].slope  = list->yroots[i].slope;
		lnew->yroots[i].icspec = list->yroots[i].icspec;
		}

	lnew->defined = TRUE;
	return lnew;
	}

/***********************************************************************
*                                                                      *
*      c o m p u t e _ i l i s t                                       *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Compute all intersections of:
 *
 *          f(x,y) = c
 *       dfdx(x,y) = 0
 *       dfdy(x,y) = 0
 *
 *  with the given patch boundary.
 *
 *	@param[in] 	list	specified ILIST
 *	@param[in] 	*func	bivariate patch function
 *	@param[in] 	sense	direction of projection (x or y)
 *	@param[in] 	value	value at which projection is to be done
 *	@param[in] 	ncspec	how many contour specs are there?
 *	@param[in] 	*cspecs	list of contour specs
 **********************************************************************/

void	compute_ilist

	(
	ILIST	list,	/* specified ILIST */
	BIPOLY	*func,	/* bivariate patch function */
	char	sense,	/* direction of projection (x or y) */
	float	value,	/* value at which projection is to be done */
	int		ncspec,	/* how many contour specs are there? */
	CONSPEC	*cspecs	/* list of contour specs */
	)

	{
	BIPOLY	deriv;
	UNIPOLY	proj;

	static	CONSPEC	cszero;
	static	int		defined = FALSE;

	/* Define zero cspec first time through */
	if (!defined)
		{
		init_conspec(&cszero);
		define_conspec_range(&cszero, 0.0, 0.0, 0.0, 0.0);
		defined = TRUE;
		}

	/* Empty any previous intersections */
	empty_ilist(list);
	if (!list) return;

	/* Do nothing if no patch function or contour specs */
	if (!func)       return;
	if (ncspec <= 0) return;
	if (!cspecs)     return;

	/* Project patch function along specified edge and obtain its */
	/* intersections with the given patch edge at the requested */
	/* contour values */
	project_bipoly(func, &proj, sense, value);
	find_ilist_roots(list, &proj, 'f', ncspec, cspecs);

	/* Save roots of dfdx */
	differentiate_bipoly(func, &deriv, 'x');
	project_bipoly(&deriv, &proj, sense, value);
	find_ilist_roots(list, &proj, 'x', 1, &cszero);

	/* Save roots of dfdy */
	differentiate_bipoly(func, &deriv, 'y');
	project_bipoly(&deriv, &proj, sense, value);
	find_ilist_roots(list, &proj, 'y', 1, &cszero);

	list->defined = TRUE;
	}

/***********************************************************************
*                                                                      *
*      f i n d _ i l i s t _ r o o t s                                 *
*                                                                      *
*      Compute intersections of the given projected function and       *
*      save them to the indicated ilist.                               *
*                                                                      *
*      This routine works in a similar way to find_unipoly_roots,      *
*      in that the interval is broken up into segments where the       *
*      function is either monotonically increasing or decreasing,      *
*      between the function's stationary points (F'(x) = 0).           *
*                                                                      *
*      This method is streamlined by searching for all contour         *
*      values at once. This is because the stationary points need      *
*      only be found once, since they are independent of any           *
*      constant value added to (subtracted from) the function.         *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Compute intersections of the given projected function and
 *  save them to the indicated ilist.
 *
 *	@param[in] 	list	specified ILIST
 *	@param[in] 	*func	univariate projection of the patch function
 *	@param[in] 	type	which list to use f/x/y => func/dfdx/dfdy
 *	@param[in] 	ncspec	how many contour specs are there?
 *	@param[in] 	*cspecs	list of contour specs
 **********************************************************************/

void	find_ilist_roots

	(
	ILIST	list,	/* specified ILIST */
	UNIPOLY	*func,	/* univariate projection of the patch function */
	char	type,	/* which list to use f/x/y => func/dfdx/dfdy */
	int		ncspec,	/* how many contour specs are there? */
	CONSPEC	*cspecs	/* list of contour specs */
	)

	{
	UNIPOLY	dfdu;
	float	roots[MAXORD], xseg[MAXORD-1], fseg[MAXORD-1];
	int		iseg, nseg, iroot, nroot, order, nmin, nmax;
	int		icspec, icval, icoff, ncval;
	float	fstart, xstart = 0;
	float	fend,   xend   = 1;
	float	vmin, vmax, root, slope, cval;
	float	cmin, cmax, cstd, cint;
	float	f1, f2, s1, s2, x1, x2, ftol = 1e-6;
	CONSPEC	*cspec;
	int		rtype=FALSE, ltype=FALSE;

	/* Do nothing if no patch function or contour specs */
	if (!list)       return;
	if (!func)       return;
	if (ncspec <= 0) return;
	if (!cspecs)     return;

	/* Take the derivative of the projected patch function */
	order = func->order;
	differentiate_unipoly(func, &dfdu);
#	ifdef DEBUG_ILIST
	if (type == 'f') (void) printf("    func: ");
	if (type == 'x') (void) printf("    dfdx: ");
	if (type == 'y') (void) printf("    dfdy: ");
	for (iseg=0; iseg<=order; iseg++) (void) printf(" %f", func->coeffs[iseg]);
	(void) printf("\n");
	(void) printf("    dfdu: ");
	for (iseg=0; iseg<=order-1; iseg++) (void) printf(" %f", dfdu.coeffs[iseg]);
	(void) printf("\n");
	(void) fflush(stdout);
#	endif /* DEBUG_ILIST */

	/* Find the roots of the patch edge derivative function and */
	/* define monotonic segments between these roots. */
	find_unipoly_roots(&dfdu, 0., xstart, xend, xseg, &nseg);
	fstart = (float) evaluate_unipoly(func, xstart);
	fend   = (float) evaluate_unipoly(func, xend);

	/* Max and min will occur either at an end point or at a root of */
	/* the derivative function */
	vmin = MIN(fstart, fend);
	vmax = MAX(fstart, fend);
	for (iseg=0; iseg<nseg; iseg++)
		{
		fseg[iseg] = (float) evaluate_unipoly(func, xseg[iseg]);
		vmin       = MIN(vmin, fseg[iseg]);
		vmax       = MAX(vmax, fseg[iseg]);
		}
	vmin -= ftol;
	vmax += ftol;
#	ifdef DEBUG_ILIST
	(void) printf("    range: %f %f\n", fstart, fend);
	(void) fflush(stdout);
#	endif /* DEBUG_ILIST */

	/* Repeat for each contour spec */
	for (icspec=0; icspec<ncspec; icspec++)
		{
		/* Interpret this contour spec */
		cspec = cspecs + icspec;
		rtype = same(cspec->type, "range");
		ltype = same(cspec->type, "list");
		if (rtype)
			{
			cmin   = cspec->cmin - ftol;	cmin = MAX(cmin, vmin);
			cmax   = cspec->cmax + ftol;	cmax = MIN(cmax, vmax);
			cstd   = cspec->cstd;
			cint   = fabs((double) cspec->cint);
			if (cint > 0)
				{
				/* Compute how many contour increments below and above cstd */
				/* Limit the range to no more than 25 below and 25 above */
				nmin = ceil((cmin-cstd)/cint);	/* nmin = MAX(nmin, -25); */
				nmax = floor((cmax-cstd)/cint);	/* nmax = MIN(nmax, 25); */
				icoff = nmin;
				ncval = nmax - nmin + 1;
				}
			else
				{
				icoff = 0;
				ncval = 1;
				}
			}
		else if (ltype)
			{
			if (cspec->nval <= 0) continue;
			ncval = cspec->nval;
			}
		else continue;

		/* Repeat for contour values between the function min and max */
		for (icval=0; icval<ncval; icval++)
			{
			/* Compute next contour value of interest */
			if (rtype)
				{
				/* For a range, this means incrementing until out of range */
				cval = cstd + cint*(icval + icoff);
				}
			else if (ltype)
				{
				/* For a list, this means taking the next given value */
				/* and ignore if out of range */
				cval = cspec->cvals[icval];
				if (cval <= vmin) continue;
				if (cval >= vmax) continue;
				}
			else break;

			/* Handle lower orders where roots can be computed explicitly */
			if (order <= MAX_EXPLICIT_ORDER)
				{
				find_unipoly_roots(func, cval, xstart, xend, roots, &nroot);
				for (iroot=0; iroot<nroot; iroot++)
					{
					root  = roots[iroot];
					slope = (float) evaluate_unipoly(&dfdu, root);
					add_root_to_ilist(list, type, cval, root, slope, icspec);
#					ifdef DEBUG_ILIST
					(void) printf("    root: %f %f %f\n", cval, root, slope);
					(void) fflush(stdout);
#					endif /* DEBUG_ILIST */
					}
				}

			/* Higher orders require root finder */
			else
				{
				x1 = xstart;
				f1 = fstart - cval;
				s1 = SIGN(f1);
				for (iseg=0; iseg<=nseg; iseg++)
					{
					if (iseg < nseg)
						{
						x2 = xseg[iseg];
						f2 = fseg[iseg] - cval;
						}
					else
						{
						x2 = xend;
						f2 = fend - cval;
						}
					s2 = SIGN(f2);
					if (s1 != s2)
						{
						root  = (float) zeroin_unipoly(func, cval, x1, x2,
											ZeroTol);
						slope = (float) evaluate_unipoly(&dfdu, root);
						add_root_to_ilist(list, type, cval, root, slope,icspec);
#						ifdef DEBUG_ILIST
						(void) printf("    root: %f %f %f\n", cval, root, slope);
						(void) fflush(stdout);
#						endif /* DEBUG_ILIST */
						}
					else if(fabs((double) f1) <= ftol)
						{
						slope = (float) evaluate_unipoly(&dfdu, x1);
						add_root_to_ilist(list, type, cval, x1, slope, icspec);
#						ifdef DEBUG_ILIST
						(void) printf("    root1: %f %f %f\n", cval, x1, slope);
						(void) fflush(stdout);
#						endif /* DEBUG_ILIST */
						}
					else if(fabs((double) f2) <= ftol)
						{
						slope = (float) evaluate_unipoly(&dfdu, x2);
						add_root_to_ilist(list, type, cval, x2, slope, icspec);
#						ifdef DEBUG_ILIST
						(void) printf("    root2: %f %f %f\n", cval, x2, slope);
						(void) fflush(stdout);
#						endif /* DEBUG_ILIST */
						}
					x1 = x2;
					f1 = f2;
					s1 = s2;
					}
				}
			}
		}

#	ifdef DEBUG_ILIST
	if (type == 'f') (void) printf("    roots: %d\n", list->numf);
	if (type == 'x') (void) printf("    roots: %d\n", list->numx);
	if (type == 'y') (void) printf("    roots: %d\n", list->numy);
	(void) fflush(stdout);
#	endif /* DEBUG_ILIST */
	}

/***********************************************************************
*                                                                      *
*      a d d _ r o o t _ t o _ i l i s t                               *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Add the new root to the given intersection list.
 *
 *	@param[in] 	list	specified ILIST
 *	@param[in] 	type	which list to use f/x/y => func/dfdx/dfdy
 *	@param[in] 	cval	contour value
 *	@param[in] 	root	intersection (root of F(u) = cval)
 *	@param[in] 	slope	slope of F(u) at root
 *	@param[in] 	icspec	contour spec pointer
 **********************************************************************/
void	add_root_to_ilist

	(
	ILIST	list,	/* specified ILIST */
	char	type,	/* which list to use f/x/y => func/dfdx/dfdy */
	float	cval,	/* contour value */
	float	root,	/* intersection (root of F(u) = cval) */
	float	slope,	/* slope of F(u) at root */
	int		icspec	/* contour spec pointer */
	)

	{
	int		i;

	/* Do nothing if ilist undefined */
	if (!list) return;

	if (type == 'f')
		{
		/* See if we need more space */
		i = list->numf++;
		if (list->numf > list->maxf)
			{
			list->maxf  += DELTA_ROOTS;
			list->froots = GETMEM(list->froots, CROOT, list->maxf);
			}

		/* Copy the given root to the list */
#		ifdef DEBUG_ILIST
		(void) printf("    froot: %f %f %f\n", cval, root, slope);
		(void) fflush(stdout);
#		endif /* DEBUG_ILIST */
		list->froots[i].cval   = cval;
		list->froots[i].root   = root;
		list->froots[i].slope  = slope;
		list->froots[i].icspec = icspec;
		}

	else if (type == 'x')
		{
		/* Add more space */
		i = list->numx++;
		list->xroots = GETMEM(list->xroots, CROOT, list->numx);

		/* Copy the given root to the list */
#		ifdef DEBUG_ILIST
		(void) printf("    xroot: %f %f %f\n", cval, root, slope);
		(void) fflush(stdout);
#		endif /* DEBUG_ILIST */
		list->xroots[i].cval   = cval;
		list->xroots[i].root   = root;
		list->xroots[i].slope  = slope;
		list->xroots[i].icspec = -1;
		}

	else if (type == 'y')
		{
		/* Add more space */
		i = list->numy++;
		list->yroots = GETMEM(list->yroots, CROOT, list->numy);

		/* Copy the given root to the list */
#		ifdef DEBUG_ILIST
		(void) printf("    yroot: %f %f %f\n", cval, root, slope);
		(void) fflush(stdout);
#		endif /* DEBUG_ILIST */
		list->yroots[i].cval   = cval;
		list->yroots[i].root   = root;
		list->yroots[i].slope  = slope;
		list->yroots[i].icspec = -1;
		}
	}

/***********************************************************************
*                                                                      *
*      c o n t o u r _ v a l                                           *
*                                                                      *
*      Compute a truncated contour value suitable for matching roots   *
*      and contour specs.                                              *
*                                                                      *
*      c o n t o u r _ l a b                                           *
*                                                                      *
*      Compute a truncated contour value suitable for labelling.       *
*                                                                      *
***********************************************************************/

static	STRING	contour_val

	(
	float	val
	)

	{
	static	char	sval[20];

	(void) safe_strcpy(sval, fformat(val, 2));
	return sval;
	}

/**********************************************************************/

static	STRING	contour_lab

	(
	float	val
	)

	{
	static	char	slab[20];

	(void) sprintf(slab, "%d", NINT(val));
	return slab;
	}
