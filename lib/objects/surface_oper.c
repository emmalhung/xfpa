/*********************************************************************/
/** @file surface_oper.c
 *
 * Routines to perform various editing and fitting operations on a
 * SURFACE object.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*      s u r f a c e _ o p e r . c                                     *
*                                                                      *
*      Routines to perform various editing and fitting operations      *
*      on a SURFACE object.                                            *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
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
#include "projection.h"

#include <tools/tools.h>
#include <fpa_getmem.h>
#include <fpa_math.h>
#include <string.h>
#include <stdio.h>

/* Weight functions for distributing the effects of surface edits */
#define FIT_NORMAL
#ifdef FIT_NORMAL
#	define W0(a) 0
#	define W1(a) 1-(a)
#	define W2(a) (a)
#	define W3(a) 0
#else
#	define W0(a) (-        (a)*((a)-1)*((a)-2) /6 )
#	define W1(a) ( ((a)+1)*    ((a)-1)*((a)-2) /2 )
#	define W2(a) (-((a)+1)*(a)*        ((a)-2) /2 )
#	define W3(a) ( ((a)+1)*(a)*((a)-1)         /6 )
#endif

/* Set various debug modes */
#undef DEBUG_SFC
#undef DEBUG_FIT
#undef DEBUG_REFIT
#undef  TIME_SPLINE

typedef	float	VEC[ORDER];

/* Default weighting factors for fitting surfaces with scattered points */
static	const	double	MinimumWgt = 1.0e-08;
static	const	double	WgtFactor  = 1.10;

/* Default distance for quick interpolation */
static	const	double	QuickInterp = 0.0001;

/***********************************************************************
*                                                                      *
*    s c a l a r _ g r i d                                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Convert a SCALAR Object to a GRID Object.
 *
 *	@param[in] 	*gridd 	pointer to GRID Object to convert into
 *	@param[in] 	glen 	grid length of output GRID Object
 *	@param[in] 	ngx 	x dimension of GRID Object
 *	@param[in] 	ngy 	y dimension of GRID Object
 *	@param[in] 	*scalr	pointer to SCALAR Object to convert
 *********************************************************************/

void scalar_grid

	(
	GRID	*gridd,
	float	glen,
	int		ngx,
	int		ngy,
	SCALAR	*scalr
	)

	{
	int iix, iiy;
	float *gvalblk;

	/* Return now if no SCALAR Object to convert ... */
	/*  or no GRID Object to convert into            */
	if (!scalr) return;
	if (!gridd) return;

	/* Return now if dimensions of GRID Object are unacceptable */
	if (ngx <= 0 || ngy <= 0) return;

	/* Define the basic attributes */
	gridd->nx = ngx;
	gridd->ny = ngy;
	gridd->gridlen = glen;

	/* Allocate space for pointers and array of grid point data */
	gvalblk = INITMEM(float, ngy*ngx);
	gridd->gval = INITMEM(float *, ngy);

	/* Set pointers and set grid point data to scalar value */
	for (iiy=0; iiy<ngy; iiy++)
		{
		gridd->gval[iiy] = gvalblk + iiy*ngx;
		for (iix=0; iix<ngx; iix++)
			gridd->gval[iiy][iix] = scalr->sval;
		}
	}

/***********************************************************************
*                                                                      *
*    s p l i n e _ g r i d                                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Convert a SPLINE Object to a GRID Object.
 *
 *	@param[in] 	*gridd 	pointer to GRID Object to convert into
 *	@param[in] 	glen 	grid length of output GRID Object
 *	@param[in] 	ngx 	x dimension of GRID Object
 *	@param[in] 	ngy 	y dimension of GRID Object
 *	@param[in] 	*splne	pointer to SPLINE Object to convert
 *********************************************************************/

void spline_grid

	(
	GRID	*gridd,
	float	glen,
	int		ngx,
	int		ngy,
	SPLINE	*splne
	)

	{
	int iix, iiy;
	LOGICAL valid;
	float *gvalblk;
	double val;
	SURFACE sfc;
	POINT pos;


	/* Return now if no SPLINE Object to convert ... */
	/*  or no GRID Object to convert into            */
	if (!splne) return;
	if (!gridd) return;

	/* Return now if dimensions of SPLINE Object are unacceptable */
	/*  ... or if dimensions of GRID Object are unacceptable      */
	if (splne->m <= 0 || splne->n <= 0) return;
	if (ngx <= 0 || ngy <= 0) return;

	/* Define the basic attributes */
	gridd->nx = ngx;
	gridd->ny = ngy;
	gridd->gridlen = glen;

	/* Allocate space for pointers and array of grid point data */
	gvalblk = INITMEM(float, ngy*ngx);
	gridd->gval = INITMEM(float *, ngy);

	/* Define the SPLINE Object into a SURFACE Object */
	/* so we can evaluate it */
	sfc = create_surface();
	define_surface_spline(sfc, splne->m, splne->n, &splne->mp, splne->origin,
						splne->orient, splne->gridlen, *splne->cvs, splne->n);

	/* Set pointers and evaluate grid point data at each location */
	for (iiy=0; iiy<ngy; iiy++)
		{
		gridd->gval[iiy] = gvalblk + iiy*ngx;
		for (iix=0; iix<ngx; iix++)
			{
			pos[X] = (float) iix * glen;
			pos[Y] = (float) iiy * glen;
			valid  = eval_sfc(sfc, pos, &val);
			gridd->gval[iiy][iix] = (float) val;
			}
		}

	/* Free space used by SURFACE Object */
	sfc = destroy_surface(sfc);
	}

/***********************************************************************
*                                                                      *
*      g r i d _ b u f f e r _ c n t l                                 *
*                                                                      *
***********************************************************************/

/* Static buffers for matrix equation */
static	double	**Matrix = NULL;
static	double	*Vector  = NULL;	/* 1D or magnitude RHS */
static	double	*VecU    = NULL;	/* 2D U-component RHS */
static	double	*VecV    = NULL;	/* 2D V-component RHS */
static	double	*Mblock  = NULL;
static	int		Mbsize   = 0;
static	int		Mretain  = TRUE;

/* Values of the four cubic basis functions at U=0 (start of patch) */
static	VEC		Basis;
static	int		BasisDef = FALSE;

/* Static buffers for matrix equation */
static	const	int		MinChunk = 4;
static	const	int		MaxChunk = 7;
static	const	int		Bridge   = 5;

/**********************************************************************/

/*********************************************************************/
/** Controls whether the dynamic buffers allocated for the matrix
 * and solution vector are to be retained for future invocations.
 *
 * This will prevent unnecessary allocations and de-allocations
 * if grid_surface() or grid_spline() are called repeatedly.
 * This in turn may prevent excessive memory growth due to poor
 * reclamation methods in the memory allocation package.
 *
 *	@param[in] 	mode	buffer mode. One of "retain", "release", "free"
 *********************************************************************/
void	grid_buffer_cntl

	(
	STRING	mode
	)

	{
	if (same(mode,"retain"))  Mretain = TRUE;
	if (same(mode,"release")) Mretain = FALSE;
	if (!Mretain || same(mode,"free"))
		{
		FREEMEM(Mblock);
		FREEMEM(Matrix);
		FREEMEM(VecU);
		FREEMEM(VecV);
		FREEMEM(Vector);
		Mbsize = 0;
		}
	}

/***********************************************************************
*                                                                      *
*      g r i d _ s u r f a c e                                         *
*      g r i d _ s p l i n e                                           *
*      g r i d _ s p l i n e _ c h u n k                               *
*                                                                      *
*      Define a surface spline to align with and interpolate a given   *
*      set of grid point values.                                       *
*                                                                      *
*      The surface is defined by forcing it to exactly interpolate     *
*      the given grid values at the grid vertices.  This uses the      *
*      usual spline evaluation equation, evaluated only at patch       *
*      corners:                                                        *
*                                                                      *
*                 3             3                                      *
*            F = SUM ( Bi(0) * SUM Bj(0) * CVij )                      *
*                i=0           j=0                                     *
*                                                                      *
*      This is sufficient to fully specify all the interior control    *
*      vertex values.  The boundary control vertices are then          *
*      specified by a "free" boundary condition - i.e. the second      *
*      derivatives are forced to zero around the whole boundary.       *
*      This is accomplished by equating each boundary control vertex   *
*      with its interior neighbour.                                    *
*                                                                      *
*      The matrix is diagonally dominant, and therefore guarantees     *
*      a solution.                                                     *
*                                                                      *
***********************************************************************/

static	void	grid_spline_chunk(SPLINE *, float, int, int, int, int,
					int, int, float **);

/**********************************************************************/

/*********************************************************************/
/** Fit a surface to the given grid values.
 *
 *	@param[in] 	sfc 		surface to be fitted
 *	@param[in] 	gridlen 	grid length
 *	@param[in] 	ngx 		number of grid points in each direction
 *	@param[in] 	ngy 		number of grid points in each direction
 *	@param[in] 	**values	array of grid values
 *********************************************************************/
void	grid_surface

	(
	SURFACE	sfc,
	float	gridlen,
	int		ngx,
	int		ngy,
	float	**values
	)

	{
	int			m, n;
	float		orient;
	POINT		origin;
	MAP_PROJ	*mp;

#	ifdef TIME_SPLINE
	long	nsec, nusec;
#	endif /* TIME_SPLINE */

	/* Make sure we have enough information */
	if (!sfc)         return;
	if (gridlen <= 0) return;
	if (ngx <= 0)     return;
	if (ngy <= 0)     return;
	if (!values)      return;

	/* Define spline dimensions so as to align patch vertices */
	/* with the given grid */
	m = ngx + ORDER - 2;
	n = ngy + ORDER - 2;
	orient = sfc->sp.orient;
	copy_point(origin, sfc->sp.origin);
	mp = &sfc->sp.mp;
	define_surface_spline(sfc,m,n,mp,origin,orient,gridlen,(float *)0,0);

	/* Evaluate the new spline control vertices */

#	ifdef TIME_SPLINE
	set_stopwatch(TRUE);
#	endif /* TIME_SPLINE */

	grid_spline(&sfc->sp, gridlen, ngx, ngy, values);

#	ifdef TIME_SPLINE
	get_stopwatch(&nsec, &nusec, NULL, NULL);
	(void) printf("Spline fit: %d.%.6d sec\n", nsec, nusec);
#	endif /* TIME_SPLINE */

	}

/**********************************************************************/

/*********************************************************************/
/** Fit a spline to the given grid values.
 *
 *	@param[in] 	*spline 	spline to be fitted
 *	@param[in] 	gridlen 	grid length
 *	@param[in] 	ngx 		number of grid points in each direction
 *	@param[in] 	ngy 		number of grid points in each direction
 *	@param[in] 	**values	array of grid values
 *********************************************************************/
void	grid_spline

	(
	SPLINE	*spline,
	float	gridlen,
	int		ngx,
	int		ngy,
	float	**values
	)

	{
	int		ncu, sgx, egx, dgx, nxrem;
	int		ncv, sgy, egy, dgy, nyrem;
	float	orient;
	POINT	origin;

	/* Make sure we have enough information */
	if (!spline)      return;
	if (gridlen <= 0) return;
	if (ngx <= 0)     return;
	if (ngy <= 0)     return;
	if (!values)      return;

	/* Define spline dimensions so as to align patch vertices */
	/* with the given grid */
	ncu = ngx + ORDER - 2;
	ncv = ngy + ORDER - 2;
	orient = spline->orient;
	copy_point(origin, spline->origin);
	define_spline(spline, ncu, ncv, &spline->mp, origin, orient, gridlen,
				NULL, 0);

	/* Split up the grid into managable chunks */
	dgx = MaxChunk;
	for (sgx=0; sgx<ngx; sgx+=dgx)
		{
		nxrem = ngx - sgx;
		if (nxrem <= MaxChunk)               dgx = nxrem;
		else if (nxrem <= MaxChunk+MinChunk) dgx = nxrem - MinChunk;
		else                                 dgx = MaxChunk;
		egx = sgx + dgx - 1;

		dgy = MaxChunk;
		for (sgy=0; sgy<ngy; sgy+=dgy)
			{
			nyrem = ngy - sgy;
			if (nyrem <= MaxChunk)               dgy = nyrem;
			else if (nyrem <= MaxChunk+MinChunk) dgy = nyrem - MinChunk;
			else                                 dgy = MaxChunk;
			egy = sgy + dgy - 1;

			/* Fit the current chunk */
			grid_spline_chunk(spline, gridlen, ngx, ngy, sgx, sgy, egx, egy,
						values);
			}
		}
	}

/**********************************************************************/

static	void	grid_spline_chunk

	(
	SPLINE	*spline,	/* spline to be fitted */
	float	gridlen,	/* grid length */
	int		ngx,		/* number of grid points in each direction */
	int		ngy,		/* number of grid points in each direction */
	int		sgx,		/* initial grid points in each direction */
	int		sgy,		/* initial grid points in each direction */
	int		egx,		/* final grid points in each direction */
	int		egy,		/* final grid points in each direction */
	float	**values	/* array of grid values */
	)

	{
	int		scu, ecu, svcu, evcu, nu, icu, jcu, iu;
	int		scv, ecv, svcv, evcv, nv, icv, jcv, iv;
	int		svgx, evgx, igx, Row, Rpt;
	int		svgy, evgy, igy, Col, Cpt;
	int		nct, ncmax;
	int		MaxBlock;
	float	Bu, Bv;
	double	*Vrow;
	LOGICAL	begx, begy, endx, endy;

#	ifdef DEBUG_SFC
	char	*sbrk();
#	endif /* DEBUG_SFC */
#	ifdef LATER
	int		su, eu;
	int		sv, ev;
#	endif /* LATER */

	/* Make sure we have enough information */
	if (!spline)      return;
	if (gridlen <= 0) return;
	if (ngx <= 0)     return;
	if (ngy <= 0)     return;
	if (!values)      return;

	/* Values of the four cubic basis functions at U=0 (start of patch) */
	if (!BasisDef)
		{
		evaluate_patch_basis(0.0,Basis);
		BasisDef = TRUE;
		}

	/* Allocate matrix and solution vector */
	MaxBlock = MaxChunk + Bridge + Bridge;
	ncmax = MaxBlock*MaxBlock;
	if (ncmax > Mbsize)
		{
#		ifdef DEBUG_SFC
		(void) printf("[grid_spline] Allocating matrix %dX%d\n",ncmax,ncmax);
		(void) printf("              Memory before: %d\n",sbrk(0));
#		endif /* DEBUG_SFC */
		Vector = GETMEM(Vector,double,ncmax);
		Matrix = GETMEM(Matrix,double *,ncmax);
		Mblock = GETMEM(Mblock,double,(ncmax*ncmax));
		Mbsize = ncmax;
#		ifdef DEBUG_SFC
		(void) printf("              Memory after: %d\n",sbrk(0));
#		endif /* DEBUG_SFC */
		if (!Mblock || !Matrix || !Vector)
			{
			(void) fprintf(stderr,"[grid_spline] Too big!\n");
			grid_buffer_cntl("free");
			return;
			}
		}

	/* Determine dimensions and range of control vertex array chunk */
	begx = (LOGICAL) (sgx <= 0);		begy = (LOGICAL) (sgy <= 0);
	endx = (LOGICAL) (egx >= ngx-1);	endy = (LOGICAL) (egy >= ngy-1);
	svgx = (begx)? 0:     sgx-Bridge+1;	svgy = (begy)? 0:     sgy-Bridge+1;
	evgx = (endx)? ngx-1: egx+Bridge-1;	evgy = (endy)? ngy-1: egy+Bridge-1;
	svcu = (begx)? 0:     sgx-Bridge+1;	svcv = (begy)? 0:     sgy-Bridge+1;
	evcu = (endx)? ngx+1: egx+Bridge+1;	evcv = (endy)? ngy+1: egy+Bridge+1;
	scu  = (begx)? 0:     sgx+1;		scv  = (begy)? 0:     sgy+1;
	ecu  = (endx)? ngx+1: egx+1;		ecv  = (endy)? ngy+1: egy+1;
	nu   = evcu - svcu + 1;				nv   = evcv - svcv + 1;
#	ifdef DEBUG_SFC
	(void) printf("\n");
	(void) printf("   Grid Chunk  (%d:%d)x(%d:%d)\n", sgx, egx, sgy, egy);
	(void) printf("   Grid Extent (%d:%d)x(%d:%d)\n", svgx, evgx, svgy, evgy);
	(void) printf("   CV   Chunk  (%d:%d)x(%d:%d)\n", scu, ecu, scv, ecv);
	(void) printf("   CV   Extent (%d:%d)x(%d:%d)\n", svcu, evcu, svcv, evcv);
#	endif /* DEBUG_SFC */

	/* Prepare matrix and solution vector for computed dimensions */
	nct = nu*nv;
	for (Row=0; Row<nct; Row++)
		{
		Matrix[Row] = Mblock + Row*nct;
		Vector[Row] = 0;
		for (Col=0; Col<nct; Col++)
			Matrix[Row][Col] = 0;
		}

	/* Set up matrix equations to fit inner control vertices */
	/* to given grid point values */
	for (igx=svgx; igx<=evgx; igx++)
		{
		icu = igx /* - svgx + svcu */ + 1;
		Rpt = (icu-svcu)*nv;
		for (igy=svgy; igy<=evgy; igy++)
			{
			icv = igy /* - svgy + svcv */ + 1;
			Row = Rpt + (icv-svcv);
			Vector[Row] = values[igy][igx];
			for (iu=0; iu<ORDER; iu++)
				{
				jcu = icu + iu - 1;
				if (igx < evgx) Bu = Basis[iu];
				else   { jcu--; Bu = Basis[ORDER-1-iu]; }
				if (Bu == 0) continue;
				Cpt = (jcu-svcu)*nv;
				for (iv=0; iv<ORDER; iv++)
					{
					jcv = icv + iv - 1;
					if (igy < evgy) Bv = Basis[iv];
					else   { jcv--; Bv = Basis[ORDER-1-iv]; }
					if (Bv == 0) continue;
					Col = Cpt + (jcv-svcv);
					Matrix[Row][Col] = Bu * Bv;
					}
				}
			}
		}

	/* Set up boundary conditions for outer control vertices */
	/* Constraining the second derivative to zero at the boundary is */
	/* accomplished by forcing the outer ring of control vertices to */
	/* take the same values as their inner neighbours */
    for (icu=svcu; icu<=evcu; icu++)
        {
        /* Bottom row */        /* Top row */
        Row = (icu-svcu)*nv;    Rpt = Row + nv - 1;
        Col = Row + 1;          Cpt = Rpt - 1;
        Vector[Row]      = 0;   Vector[Rpt]      = 0;
        Matrix[Row][Row] = 1;   Matrix[Rpt][Rpt] = 1;
        Matrix[Row][Col] = -1;  Matrix[Rpt][Cpt] = -1;
        }
    for (icv=svcv+1; icv<=evcv-1; icv++)
        {
        /* Left column */       /* Right column */
        Row = (icv-svcv);       Rpt = Row + (nu-1)*nv;
        Col = Row + nv;         Cpt = Rpt - nv;
        Vector[Row]      = 0;   Vector[Rpt]      = 0;
        Matrix[Row][Row] = 1;   Matrix[Rpt][Rpt] = 1;
        Matrix[Row][Col] = -1;  Matrix[Rpt][Cpt] = -1;
        }

#	ifdef LATER
	if (begy)
		{
		/* Bottom row */
		su = (begx)? svcu: svcu+1;
		eu = (endx)? evcu: evcu-1;
		for (icu=su; icu<=eu; icu++)
			{
			Row = (icu-svcu)*nv;
			Col = Row + 1;
			Vector[Row]      = 0;
			Matrix[Row][Row] = 1;
			Matrix[Row][Col] = -1;
			}
		}
	if (endy)
		{
		/* Top row */
		su = (begx)? svcu: svcu+1;
		eu = (endx)? evcu: evcu-1;
		for (icu=su; icu<=eu; icu++)
			{
			Row = (icu-svcu)*nv + nv - 1;
			Col = Row - 1;
			Vector[Row]      = 0;
			Matrix[Row][Row] = 1;
			Matrix[Row][Col] = -1;
			}
		}
	if (begx)
		{
		/* Left column */
		for (icv=svcv+1; icv<=evcv-1; icv++)
			{
			Row = (icv-svcv);
			Col = Row + nv;
			Vector[Row]      = 0;
			Matrix[Row][Row] = 1;
			Matrix[Row][Col] = -1;
			}
		}
	if (endx)
		{
		/* Right column */
		for (icv=svcv+1; icv<=evcv-1; icv++)
			{
			Row = (icv-svcv) + (nu-1)*nv;
			Col = Row - nv;
			Vector[Row]      = 0;
			Matrix[Row][Row] = 1;
			Matrix[Row][Col] = -1;
			}
		}
#	endif /* LATER */

	/* Now solve the matrix system */
	qsolve_matrix(Matrix,Vector,nct);

	/* Now load the control vertex array into the surface spline */
	/* Only load the non-overlapping parts */
	for (icu=scu; icu<=ecu; icu++)
		{
		Vrow = Vector + (icu-svcu)*nv;
		for (icv=scv; icv<=ecv; icv++)
			spline->cvs[icu][icv] = (float) Vrow[icv-svcv];
		}

	/* De-allocate matrix and solution vector */
	if (!Mretain) grid_buffer_cntl("free");
	}

/***********************************************************************
*                                                                      *
*      g r i d _ s u r f a c e _ 2 D                                   *
*      g r i d _ s p l i n e _ 2 D                                     *
*      g r i d _ s p l i n e _ 2 D _ c h u n k                         *
*      g r i d _ s p l i n e _ m a g                                   *
*                                                                      *
*      2D vector versions of the above.                                *
*                                                                      *
***********************************************************************/

static	void	grid_spline_2D_chunk(SPLINE *, float, int, int, int, int,
					int, int, float **, float **);

/**********************************************************************/

/*********************************************************************/
/** Fit a 2D surface to the given grid values.
 *
 *	@param[in] 	sfc 		surface to be fitted
 *	@param[in] 	gridlen 	grid length
 *	@param[in] 	ngx 		number of grid points in each direction
 *	@param[in] 	ngy 		number of grid points in each direction
 *	@param[in] 	**xvals 	array of x-component grid values
 *	@param[in] 	**yvals		array of y-component grid values
 *********************************************************************/
void	grid_surface_2D

	(
	SURFACE	sfc,
	float	gridlen,
	int		ngx,
	int		ngy,
	float	**xvals,
	float	**yvals
	)

	{
	int			m, n;
	float		orient;
	POINT		origin;
	MAP_PROJ	*mp;

#	ifdef TIME_SPLINE
	long	nsec, nusec;
#	endif /* TIME_SPLINE */

	/* Make sure we have enough information */
	if (!sfc)         return;
	if (gridlen <= 0) return;
	if (ngx <= 0)     return;
	if (ngy <= 0)     return;
	if (!xvals)       return;
	if (!yvals)       return;

	/* Define spline dimensions so as to align patch vertices */
	/* with the given grid */
	m = ngx + ORDER - 2;
	n = ngy + ORDER - 2;
	orient = sfc->sp.orient;
	copy_point(origin, sfc->sp.origin);
	mp = &sfc->sp.mp;
	define_surface_spline_2D(sfc, m, n, mp, origin, orient, gridlen,
				(float *)0, (float *)0, 0);

	/* Evaluate the new spline control vertices */

#	ifdef TIME_SPLINE
	set_stopwatch(TRUE);
#	endif /* TIME_SPLINE */

	grid_spline_2D(&sfc->sp, gridlen, ngx, ngy, xvals, yvals);

#	ifdef TIME_SPLINE
	get_stopwatch(&nsec, &nusec, NULL, NULL);
	(void) printf("Spline fit: %d.%.6d sec\n", nsec, nusec);
#	endif /* TIME_SPLINE */

	}

/**********************************************************************/

/*********************************************************************/
/** Fit a 2D spline to the given grid values
 *
 *	@param[in] 	*spline 	spline to be fitted
 *	@param[in] 	gridlen 	grid length
 *	@param[in] 	ngx 		number of grid points in each direction
 *	@param[in] 	ngy 		number of grid points in each direction
 *	@param[in] 	**xvals 	array of x-component grid values
 *	@param[in] 	**yvals		array of y-component grid values
 *********************************************************************/
void	grid_spline_2D

	(
	SPLINE	*spline,
	float	gridlen,
	int		ngx,
	int		ngy,
	float	**xvals,
	float	**yvals
	)

	{
	int		ncu, sgx, egx, dgx, nxrem;
	int		ncv, sgy, egy, dgy, nyrem;
	float	orient;
	POINT	origin;

	/* Make sure we have enough information */
	if (!spline)      return;
	if (gridlen <= 0) return;
	if (ngx <= 0)     return;
	if (ngy <= 0)     return;
	if (!xvals)       return;
	if (!yvals)       return;

	/* Define spline dimensions so as to align patch vertices */
	/* with the given grid */
	ncu = ngx + ORDER - 2;
	ncv = ngy + ORDER - 2;
	orient = spline->orient;
	copy_point(origin, spline->origin);
	define_spline_2D(spline, ncu, ncv, &spline->mp, origin, orient, gridlen,
				NULL, NULL, 0);

	/* Split up the grid into managable chunks */
	dgx = MaxChunk;
	for (sgx=0; sgx<ngx; sgx+=dgx)
		{
		nxrem = ngx - sgx;
		if (nxrem <= MaxChunk)               dgx = nxrem;
		else if (nxrem <= MaxChunk+MinChunk) dgx = nxrem - MinChunk;
		else                                 dgx = MaxChunk;
		egx = sgx + dgx - 1;

		dgy = MaxChunk;
		for (sgy=0; sgy<ngy; sgy+=dgy)
			{
			nyrem = ngy - sgy;
			if (nyrem <= MaxChunk)               dgy = nyrem;
			else if (nyrem <= MaxChunk+MinChunk) dgy = nyrem - MinChunk;
			else                                 dgy = MaxChunk;
			egy = sgy + dgy - 1;

			/* Fit the current chunk */
			grid_spline_2D_chunk(spline, gridlen, ngx, ngy, sgx, sgy, egx, egy,
					xvals, yvals);
			}
		}
	}

/**********************************************************************/

static	void	grid_spline_2D_chunk

	(
	SPLINE	*spline,	/* spline to be fitted */
	float	gridlen,	/* grid length */
	int		ngx,		/* number of grid points in each direction */
	int		ngy,		/* number of grid points in each direction */
	int		sgx,		/* initial grid points in each direction */
	int		sgy,		/* initial grid points in each direction */
	int		egx,		/* final grid points in each direction */
	int		egy,		/* final grid points in each direction */
	float	**xvals,	/* array of x-component grid values */
	float	**yvals		/* array of y-component grid values */
	)

	{
	int		scu, ecu, svcu, evcu, nu, icu, jcu, iu;
	int		scv, ecv, svcv, evcv, nv, icv, jcv, iv;
	int		svgx, evgx, igx, Row, Rpt;
	int		svgy, evgy, igy, Col, Cpt;
	int		nct, ncmax;
	int		MaxBlock;
	float	Bu, Bv;
	double	xv, yv;
	double	*Urow, *Vrow, *Srow;
	LOGICAL	begx, begy, endx, endy;

#	ifdef DEBUG_SFC
	char	*sbrk();
#	endif /* DEBUG_SFC */
#	ifdef LATER
	int		su, eu;
	int		sv, ev;
#	endif /* LATER */

	/* Make sure we have enough information */
	if (!spline)      return;
	if (gridlen <= 0) return;
	if (ngx <= 0)     return;
	if (ngy <= 0)     return;
	if (!xvals)       return;
	if (!yvals)       return;

	/* Values of the four cubic basis functions at U=0 (start of patch) */
	if (!BasisDef)
		{
		evaluate_patch_basis(0.0,Basis);
		BasisDef = TRUE;
		}

	/* Allocate matrix and solution vector */
	MaxBlock = MaxChunk + Bridge + Bridge;
	ncmax = MaxBlock*MaxBlock;
	if (ncmax > Mbsize)
		{
#		ifdef DEBUG_SFC
		(void) printf("[grid_spline] Allocating matrix %dX%d\n",ncmax,ncmax);
		(void) printf("              Memory before: %d\n",sbrk(0));
#		endif /* DEBUG_SFC */
		VecU   = GETMEM(VecU,double,ncmax);
		VecV   = GETMEM(VecV,double,ncmax);
		Vector = GETMEM(Vector,double,ncmax);
		Matrix = GETMEM(Matrix,double *,ncmax);
		Mblock = GETMEM(Mblock,double,(ncmax*ncmax));
		Mbsize = ncmax;
#		ifdef DEBUG_SFC
		(void) printf("              Memory after: %d\n",sbrk(0));
#		endif /* DEBUG_SFC */
		if (!Mblock || !Matrix || !Vector || !VecU || !VecV)
			{
			(void) fprintf(stderr,"[grid_spline] Too big!\n");
			grid_buffer_cntl("free");
			return;
			}
		}
	if (!VecU) VecU = INITMEM(double,ncmax);
	if (!VecV) VecV = INITMEM(double,ncmax);

	/* Determine dimensions and range of control vertex array chunk */
	begx = (LOGICAL) (sgx <= 0);		begy = (LOGICAL) (sgy <= 0);
	endx = (LOGICAL) (egx >= ngx-1);	endy = (LOGICAL) (egy >= ngy-1);
	svgx = (begx)? 0:     sgx-Bridge+1;	svgy = (begy)? 0:     sgy-Bridge+1;
	evgx = (endx)? ngx-1: egx+Bridge-1;	evgy = (endy)? ngy-1: egy+Bridge-1;
	svcu = (begx)? 0:     sgx-Bridge+1;	svcv = (begy)? 0:     sgy-Bridge+1;
	evcu = (endx)? ngx+1: egx+Bridge+1;	evcv = (endy)? ngy+1: egy+Bridge+1;
	scu  = (begx)? 0:     sgx+1;		scv  = (begy)? 0:     sgy+1;
	ecu  = (endx)? ngx+1: egx+1;		ecv  = (endy)? ngy+1: egy+1;
	nu   = evcu - svcu + 1;				nv   = evcv - svcv + 1;
#	ifdef DEBUG_SFC
	(void) printf("\n");
	(void) printf("   Grid Chunk  (%d:%d)x(%d:%d)\n", sgx, egx, sgy, egy);
	(void) printf("   Grid Extent (%d:%d)x(%d:%d)\n", svgx, evgx, svgy, evgy);
	(void) printf("   CV   Chunk  (%d:%d)x(%d:%d)\n", scu, ecu, scv, ecv);
	(void) printf("   CV   Extent (%d:%d)x(%d:%d)\n", svcu, evcu, svcv, evcv);
#	endif /* DEBUG_SFC */

	/* Prepare matrix and solution vector for computed dimensions */
	nct = nu*nv;
	for (Row=0; Row<nct; Row++)
		{
		Matrix[Row] = Mblock + Row*nct;
		VecU[Row]   = 0;
		VecV[Row]   = 0;
		Vector[Row] = 0;
		for (Col=0; Col<nct; Col++)
			Matrix[Row][Col] = 0;
		}

	/* Set up matrix equations to fit inner control vertices */
	/* to given grid point values */
	for (igx=svgx; igx<=evgx; igx++)
		{
		icu = igx /* - svgx + svcu */ + 1;
		Rpt = (icu-svcu)*nv;
		for (igy=svgy; igy<=evgy; igy++)
			{
			icv = igy /* - svgy + svcv */ + 1;
			Row = Rpt + (icv-svcv);
			xv  = xvals[igy][igx];
			yv  = yvals[igy][igx];
			VecU[Row]   = xv;
			VecV[Row]   = yv;
			Vector[Row] = hypot(xv, yv);
			for (iu=0; iu<ORDER; iu++)
				{
				jcu = icu + iu - 1;
				if (igx < evgx) Bu = Basis[iu];
				else   { jcu--; Bu = Basis[ORDER-1-iu]; }
				if (Bu == 0) continue;
				Cpt = (jcu-svcu)*nv;
				for (iv=0; iv<ORDER; iv++)
					{
					jcv = icv + iv - 1;
					if (igy < evgy) Bv = Basis[iv];
					else   { jcv--; Bv = Basis[ORDER-1-iv]; }
					if (Bv == 0) continue;
					Col = Cpt + (jcv-svcv);
					Matrix[Row][Col] = Bu * Bv;
					}
				}
			}
		}

	/* Set up boundary conditions for outer control vertices */
	/* Constraining the second derivative to zero at the boundary is */
	/* accomplished by forcing the outer ring of control vertices to */
	/* take the same values as their inner neighbours */
    for (icu=svcu; icu<=evcu; icu++)
        {
        /* Bottom row */        /* Top row */
        Row = (icu-svcu)*nv;    Rpt = Row + nv - 1;
        Col = Row + 1;          Cpt = Rpt - 1;
        VecU[Row]        = 0;   VecU[Rpt]        = 0;
        VecV[Row]        = 0;   VecV[Rpt]        = 0;
        Vector[Row]      = 0;   Vector[Rpt]      = 0;
        Matrix[Row][Row] = 1;   Matrix[Rpt][Rpt] = 1;
        Matrix[Row][Col] = -1;  Matrix[Rpt][Cpt] = -1;
        }
    for (icv=svcv+1; icv<=evcv-1; icv++)
        {
        /* Left column */       /* Right column */
        Row = (icv-svcv);       Rpt = Row + (nu-1)*nv;
        Col = Row + nv;         Cpt = Rpt - nv;
        VecU[Row]        = 0;   VecU[Rpt]        = 0;
        VecV[Row]        = 0;   VecV[Rpt]        = 0;
        Vector[Row]      = 0;   Vector[Rpt]      = 0;
        Matrix[Row][Row] = 1;   Matrix[Rpt][Rpt] = 1;
        Matrix[Row][Col] = -1;  Matrix[Rpt][Cpt] = -1;
        }

#	ifdef LATER
	if (begy)
		{
		/* Bottom row */
		su = (begx)? svcu: svcu+1;
		eu = (endx)? evcu: evcu-1;
		for (icu=su; icu<=eu; icu++)
			{
			Row = (icu-svcu)*nv;
			Col = Row + 1;
			VecU[Row]        = 0;
			VecV[Row]        = 0;
			Vector[Row]      = 0;
			Matrix[Row][Row] = 1;
			Matrix[Row][Col] = -1;
			}
		}
	if (endy)
		{
		/* Top row */
		su = (begx)? svcu: svcu+1;
		eu = (endx)? evcu: evcu-1;
		for (icu=su; icu<=eu; icu++)
			{
			Row = (icu-svcu)*nv + nv - 1;
			Col = Row - 1;
			VecU[Row]        = 0;
			VecV[Row]        = 0;
			Vector[Row]      = 0;
			Matrix[Row][Row] = 1;
			Matrix[Row][Col] = -1;
			}
		}
	if (begx)
		{
		/* Left column */
		for (icv=svcv+1; icv<=evcv-1; icv++)
			{
			Row = (icv-svcv);
			Col = Row + nv;
			VecU[Row]        = 0;
			VecV[Row]        = 0;
			Vector[Row]      = 0;
			Matrix[Row][Row] = 1;
			Matrix[Row][Col] = -1;
			}
		}
	if (endx)
		{
		/* Right column */
		for (icv=svcv+1; icv<=evcv-1; icv++)
			{
			Row = (icv-svcv) + (nu-1)*nv;
			Col = Row - nv;
			VecU[Row]        = 0;
			VecV[Row]        = 0;
			Vector[Row]      = 0;
			Matrix[Row][Row] = 1;
			Matrix[Row][Col] = -1;
			}
		}
#	endif /* LATER */

	/* Now solve the matrix system */
	qsolve_matrix_2D(Matrix,VecU,VecV,Vector,nct);

	/* Now load the control vertex array into the surface spline */
	/* Only load the non-overlapping parts */
	for (icu=scu; icu<=ecu; icu++)
		{
		Urow = VecU   + (icu-svcu)*nv;
		Vrow = VecV   + (icu-svcu)*nv;
		Srow = Vector + (icu-svcu)*nv;
		for (icv=scv; icv<=ecv; icv++)
			{
			spline->cvx[icu][icv] = (float) Urow[icv-svcv];
			spline->cvy[icu][icv] = (float) Vrow[icv-svcv];
			spline->cvs[icu][icv] = (float) Srow[icv-svcv];
			}
		}

	/* De-allocate matrix and solution vector */
	if (!Mretain) grid_buffer_cntl("free");
	}

/**********************************************************************/

void	grid_spline_mag

	(
	SPLINE	*spline,	/* spline to be fitted */
	float	gridlen,	/* grid length */
	int		ngx,		/* number of grid points in each direction */
	int		ngy,		/* number of grid points in each direction */
	float	**values	/* array of grid values */
	)

	{
	int		ncu, sgx, egx, dgx, nxrem;
	int		ncv, sgy, egy, dgy, nyrem;
	float	orient;
	POINT	origin;

	/* Make sure we have enough information */
	if (!spline)      return;
	if (spline->dim != DimVector2D) return;
	if (gridlen <= 0) return;
	if (ngx <= 0)     return;
	if (ngy <= 0)     return;
	if (!values)      return;

	/* Define spline dimensions so as to align patch vertices */
	/* with the given grid */

	/* Split up the grid into managable chunks */
	dgx = MaxChunk;
	for (sgx=0; sgx<ngx; sgx+=dgx)
		{
		nxrem = ngx - sgx;
		if (nxrem <= MaxChunk)               dgx = nxrem;
		else if (nxrem <= MaxChunk+MinChunk) dgx = nxrem - MinChunk;
		else                                 dgx = MaxChunk;
		egx = sgx + dgx - 1;

		dgy = MaxChunk;
		for (sgy=0; sgy<ngy; sgy+=dgy)
			{
			nyrem = ngy - sgy;
			if (nyrem <= MaxChunk)               dgy = nyrem;
			else if (nyrem <= MaxChunk+MinChunk) dgy = nyrem - MinChunk;
			else                                 dgy = MaxChunk;
			egy = sgy + dgy - 1;

			/* Fit the current chunk */
			grid_spline_chunk(spline, gridlen, ngx, ngy, sgx, sgy, egx, egy,
						values);
			}
		}
	}

/***********************************************************************
*                                                                      *
*      e d i t _ s u r f a c e                                         *
*                                                                      *
*      Alter the control vertex array to produce the specified change  *
*      in the surface at the specified point.                          *
*                                                                      *
*      To make a change of delta in the actual interpolated surface,   *
*      we seek a value of D such that:                                 *
*                                                                      *
*               3   3                                                  *
*      F.new = SUM SUM ( Bi(U)*Bj(V) * ( CVij + D*Wi(U)*Wj(V) ) )      *
*              i=0 j=0                                                 *
*                                                                      *
*                             3                 3                      *
*            = F.old  +  D * SUM Bi(U)*Wi(U) * SUM Bj(V)*Wj(V)         *
*                            i=0               j=0                     *
*                                                                      *
*      Hence,    D = delta / ( SUMi * SUMj )                           *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Alter the control vertex array to produce the specified change in
 *  the surface at the specified point.
 *
 *	@param[in] 	sfc 		surface to be editted
 *	@param[in] 	centre 		centre of effect (where to edit)
 *	@param[in] 	value 		amount of effect
 *	@param[in] 	absolute 	is value absolute (T) or relative (F)?
 *	@param[in] 	recont		do we want to re-contour?
 *********************************************************************/
void edit_surface

	(
	SURFACE	sfc,
	POINT	centre,
	float	value,
	LOGICAL	absolute,
	LOGICAL	recont
	)

	{
	int		ipl, ipr, ipb, ipt;
	int		iup, ivp, iuc, ivc, iu, iv, nu, nv;
	POINT	pp, dp;
	float	delta, U, V, sumi, sumj;
	VEC		bu, bv, wu, wv;
	PATCH	patch;

	/* If surface undefined do nothing */
	if (!sfc)    return;
	if (!centre) return;

	/* Find patch which contains the given point */
	if (!find_patch(&sfc->sp,centre,&iup,&ivp,pp,dp)) return;

	/* If absolute rather than relative edit, subtract current value */
	/* i.e. we need a relative amount to work with */
	delta = value;
	if (absolute)
		{
		patch  = prepare_sfc_patch(sfc, iup, ivp);
		delta -= (float) evaluate_bipoly(&patch->function,pp);
		patch  = dispose_sfc_patch(sfc, iup, ivp);
		}
	if (delta == 0) return;

	/* Set up weights and basis function values */
	/* (preserve natural boundary condition at edge) */
	nu    = sfc->nupatch - 1;			nv    = sfc->nvpatch - 1;
	U     = MAX(pp[X],0);				V     = MAX(pp[Y],0);
	U     = MIN(U,1);					V     = MIN(V,1);
	wu[0] = (iup>0)?    W0(U): W1(U);	wv[0] = (ivp>0)?    W0(V): W1(V);
	wu[1] = W1(U);						wv[1] = W1(V);
	wu[2] = W2(U);						wv[2] = W2(V);
	wu[3] = (iup<nu-1)? W3(U): W2(U);	wv[3] = (ivp<nv-1)? W3(V): W2(V);
	evaluate_patch_basis(U,bu);			evaluate_patch_basis(V,bv);

	/* Compute delta factor if cvs are changed by the proposed */
	/* weighted amounts */
	sumi = 0;	for (iu=0; iu<ORDER; iu++) sumi += bu[iu] * wu[iu];
	sumj = 0;	for (iv=0; iv<ORDER; iv++) sumj += bv[iv] * wv[iv];
	delta /= sumi * sumj;

	/* Now alter the affected control vertices */
	for (iu=0; iu<ORDER; iu++)
		{
		if (wu[iu] <= 0) continue;
		iuc = iu + iup;
		for (iv=0; iv<ORDER; iv++)
			{
			if (wv[iv] <= 0) continue;
			ivc = iv + ivp;
			sfc->sp.cvs[iuc][ivc] += delta * wu[iu] * wv[iv];
			}
		}

	/* Determine what patches are affected */
	ipl = iup - ORDER + 2;
	ipr = iup + ORDER - 2;
	ipb = ivp - ORDER + 2;
	ipt = ivp + ORDER - 2;

	/* Now re-contour the surface */
	if (recont) contour_surface_partial(sfc,ipl,ipr,ipb,ipt);
	else        redefine_surface_patches(sfc,ipl,ipr,ipb,ipt, FALSE);
	}

/***********************************************************************
*                                                                      *
*      e d i t _ s u r f a c e _ 2 D                                   *
*                                                                      *
*      2D vector version of the above.                                 *
*                                                                      *
***********************************************************************/

/* This one passes the single edit point on to fit_surface_2D */
/* Time consuming, but it should work */

/*********************************************************************/
/** Edit a 2D surface by altering the control vertex array to produce
 * the specified change in the surface at the specified point.
 *
 *	@param[in] 	sfc 		surface to be editted
 *	@param[in] 	centre 		centre of effect (where to edit)
 *	@param[in] 	xval 		amount of effect
 *	@param[in] 	yval 		amount of effect
 *	@param[in] 	absolute 	is value absolute (T) or relative (F)?
 *	@param[in] 	recont		do we want to re-contour?
 *********************************************************************/
void edit_surface_2D

	(
	SURFACE	sfc,
	POINT	centre,
	float	xval,
	float	yval,
	LOGICAL	absolute,
	LOGICAL	recont
	)

	{
	POINT	pbuf[1];
	float	xbuf[1], ybuf[1];

	/* If surface undefined do nothing */
	if (!sfc)    return;
	if (!centre) return;

	copy_point(pbuf[0], centre);
	xbuf[0] = xval;
	ybuf[0] = yval;

	(void) fit_surface_2D(sfc, 1, pbuf, xbuf, ybuf, absolute, recont);
	}

/**********************************************************************/

/* This one uses code based on edit_surface, but does not handle recomputing */
/* the magnitude correctly */
void edit_surface_2D_real

	(
	SURFACE	sfc,		/* surface to be editted */
	POINT	centre,		/* centre of effect (where to edit) */
	float	xval,		/* amount of effect */
	float	yval,		/* amount of effect */
	LOGICAL	absolute,	/* is value absolute (T) or relative (F)? */
	LOGICAL	recont		/* do we want to re-contour? */
	)

	{
	int		ipl, ipr, ipb, ipt;
	int		iup, ivp, iuc, ivc, iu, iv, nu, nv;
	POINT	pp, dp;
	float	deltax, deltay, U, V, sumi, sumj;
	VEC		bu, bv, wu, wv;
	PATCH	patch;

	/* If surface undefined do nothing */
	if (!sfc)    return;
	if (!centre) return;

	/* Find patch which contains the given point */
	if (!find_patch(&sfc->sp,centre,&iup,&ivp,pp,dp)) return;

	/* If absolute rather than relative edit, subtract current value */
	/* i.e. we need a relative amount to work with */
	deltax = xval;
	deltay = yval;
	if (absolute)
		{
		patch   = prepare_sfc_patch(sfc, iup, ivp);
		deltax -= (float) evaluate_bipoly(&patch->xfunc,pp);
		deltay -= (float) evaluate_bipoly(&patch->yfunc,pp);
		patch   = dispose_sfc_patch(sfc, iup, ivp);
		}
	if (deltax==0 && deltay==0) return;

	/* Set up weights and basis function values */
	/* (preserve natural boundary condition at edge) */
	nu    = sfc->nupatch - 1;			nv    = sfc->nvpatch - 1;
	U     = MAX(pp[X],0);				V     = MAX(pp[Y],0);
	U     = MIN(U,1);					V     = MIN(V,1);
	wu[0] = (iup>0)?    W0(U): W1(U);	wv[0] = (ivp>0)?    W0(V): W1(V);
	wu[1] = W1(U);						wv[1] = W1(V);
	wu[2] = W2(U);						wv[2] = W2(V);
	wu[3] = (iup<nu-1)? W3(U): W2(U);	wv[3] = (ivp<nv-1)? W3(V): W2(V);
	evaluate_patch_basis(U,bu);			evaluate_patch_basis(V,bv);

	/* Compute delta factor if cvs are changed by the proposed */
	/* weighted amounts */
	sumi = 0;	for (iu=0; iu<ORDER; iu++) sumi += bu[iu] * wu[iu];
	sumj = 0;	for (iv=0; iv<ORDER; iv++) sumj += bv[iv] * wv[iv];
	deltax /= sumi * sumj;
	deltay /= sumi * sumj;

	/* Now alter the affected control vertices */
	for (iu=0; iu<ORDER; iu++)
		{
		if (wu[iu] <= 0) continue;
		iuc = iu + iup;
		for (iv=0; iv<ORDER; iv++)
			{
			if (wv[iv] <= 0) continue;
			ivc = iv + ivp;
			sfc->sp.cvx[iuc][ivc] += deltax * wu[iu] * wv[iv];
			sfc->sp.cvy[iuc][ivc] += deltay * wu[iu] * wv[iv];
			/* Need to refit magnitude - see calc_spline_mag_cvs() below  */
			}
		}

	/* Determine what patches are affected */
	ipl = iup - ORDER + 2;
	ipr = iup + ORDER - 2;
	ipb = ivp - ORDER + 2;
	ipt = ivp + ORDER - 2;

	/* Now re-contour the surface */
	calc_spline_mag_cvs(&sfc->sp);
	if (recont) contour_surface_partial(sfc,ipl,ipr,ipb,ipt);
	else        redefine_surface_patches(sfc,ipl,ipr,ipb,ipt, FALSE);
	}

/***********************************************************************
*                                                                      *
*      f i t _ s u r f a c e                                           *
*      s f i t _ s u r f a c e                                         *
*                                                                      *
*      Alter the surface spline to interpolate the given set of        *
*      values at the specified points.                                 *
*                                                                      *
*      This one uses a dumb approximate technique, based on adjusting  *
*      the evaluated value at each grid intersection, according to     *
*      its distance from neighbouring fit points.                      *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Alter the surface spline to interpolate the given set of values
 *  at the specified points.
 *
 *	@param[in] 	sfc 		surface to be editted
 *	@param[in] 	npts 		how many points to fit
 *	@param[in] 	*points 	points to fit
 *	@param[in] 	*values 	corresponding values
 *	@param[in] 	absolute 	are values absolute (T) or relative (F)?
 *	@param[in] 	recont		do we want to re-contour?
 *********************************************************************/
int		fit_surface

	(
	SURFACE	sfc,
	int		npts,
	POINT	*points,
	float	*values,
	LOGICAL	absolute,
	LOGICAL	recont
	)

	{

	/* Call the function sfit_surface() */
	return sfit_surface(sfc, npts, points, values, (float) ORDER-2, WgtFactor,
			absolute, recont);
	}

/**********************************************************************/

/*********************************************************************/
/** Alter the surface spline to interpolate the given set of values
 *  at the specified points. Given some influence weighting guidance.
 *
 *	@param[in] 	sfc 		surface to be editted
 *	@param[in] 	npts 		how many points to fit
 *	@param[in] 	*points 	points to fit
 *	@param[in] 	*values 	corresponding values
 *	@param[in] 	influence 	radius of influence (in grid units)
 *	@param[in] 	weighting 	weighting factor for adjustments
 *	@param[in] 	absolute 	are values absolute (T) or relative (F)?
 *	@param[in] 	recont		do we want to re-contour?
 *********************************************************************/
int		sfit_surface

	(
	SURFACE	sfc,
	int		npts,
	POINT	*points,
	float	*values,
	float	influence,
	float	weighting,
	LOGICAL	absolute,
	LOGICAL	recont
	)

	{
	int		ix, iy, iu, iv, ip, jp;
	double	dx, dy, wx, wy, wt, vsum, wsum;
	double	dp, dh;
	POINT	ppos, plen;
	int		success = FALSE;
	PATCH	patch;

	/* Pointers for working buffers */
	int		FNum, FMax, Ngx, Ngy;
	POINT	*FPts, *FLoc;
	float	*FVal;
	int		*FPu,  *FPv;
	float	**Grid, *GBuf;

	/* If surface undefined do nothing */
	if (!sfc)      return success;
	if (npts <= 0) return success;
	if (!points)   return success;
	if (!values)   return success;

	/* Allocate space for working buffers */
	FMax = npts;
	FPts = INITMEM(POINT,FMax);
	FLoc = INITMEM(POINT,FMax);
	FVal = INITMEM(float,FMax);
	FPu  = INITMEM(int,FMax);
	FPv  = INITMEM(int,FMax);

	/* Allocate space for grid value array */
	Ngx  = sfc->nupatch + 1;
	Ngy  = sfc->nvpatch + 1;
	GBuf = INITMEM(float,Ngx*Ngy);
	Grid = INITMEM(float *,Ngy);
	for (iy=0; iy<Ngy; iy++)
		Grid[iy] = GBuf + iy*Ngx;

	/* Scan specified points to see if any need to be rejected */
	/* Also convert absolute constraints to relative */
	FNum = 0;
	for (ip=0; ip<FMax; ip++)
		{
		/* Only use the point if on the map */
		if (!find_patch(&sfc->sp,points[ip],&iu,&iv,ppos,plen)) continue;

		/* Use this point */
		jp = FNum++;
		FPu[jp] = iu;
		FPv[jp] = iv;
		copy_point(FPts[jp],points[ip]);
		copy_point(FLoc[jp],ppos);
		FVal[jp] = values[ip];
		if (absolute)
			{
			patch = prepare_sfc_patch(sfc, iu, iv);
			FVal[jp] -= (float) evaluate_bipoly(&patch->function,ppos);
			}
		}

#	ifdef DEBUG_FIT
	pr_diag("sfit_surface", " Fitting   %d points\n", FMax);
	if (FNum != FMax)
		{
		pr_diag("sfit_surface", " Rejecting %d points\n", FMax-FNum);
		pr_diag("sfit_surface", " Using     %d points\n", FNum);
		}
#	endif /* DEBUG_FIT */

	/* Forget it if no fit points are within contourable region */
	if (FNum <= 0) goto Tidy;
	success = TRUE;

	/* Set up new grid values */
	dp = (double) influence;
	dh = dp / 4;
	for (ix=0; ix<Ngx; ix++)
		{
		/* Relate x value to patch */
		if (ix < Ngx-1)	{ iu = ix;		ppos[X] = 0; }
		else            { iu = ix-1;	ppos[X] = 1; }

		for (iy=0; iy<Ngy; iy++)
			{
			/* Relate y value to patch */
			if (iy < Ngy-1)	{ iv = iy;		ppos[Y] = 0; }
			else            { iv = iy-1;	ppos[Y] = 1; }

			/* Evaluate original grid values */
			patch = prepare_sfc_patch(sfc, iu, iv);
			Grid[iy][ix] = (float) evaluate_bipoly(&patch->function,ppos);
			patch = dispose_sfc_patch(sfc, iu, iv);

			/* Account for any fit points that are within range */
			vsum = 0;
			wsum = 0;
			for (ip=0; ip<FNum; ip++)
				{

				/* Determine x and y distance in patch lengths */
				dx = fabs((double) (FPu[ip] + FLoc[ip][X] - ix));
				dy = fabs((double) (FPv[ip] + FLoc[ip][Y] - iy));

				/* Is this one within range? */
				if (dx >= dp) continue;
				if (dy >= dp) continue;

				/* Compute corresponding weights */
				wx = 1;	if (dx > dh) wx -= (dx-dh)/(dp-dh);
				wy = 1;	if (dy > dh) wy -= (dy-dh)/(dp-dh);
				wt = wx * wy;

				/* Accumulate weighted sum */
				vsum += FVal[ip] * wt * wt;
				wsum += wt * wt;

#				ifdef DEBUG_FIT
				pr_diag("sfit_surface",
					"   Point [%d] at (%.3f,%.3f)  Adj: %f  Wgt: %f\n",
						ip, dx, dy, FVal[ip], wt);
#				endif /* DEBUG_FIT */
				}

			/* Continue if no points nearby */
			if (wsum <= MinimumWgt) continue;

#			ifdef DEBUG_FIT
			pr_diag("sfit_surface",
				" Grid [%d][%d]  Initial Val: %f\n",
					ix, iy, Grid[iy][ix]);
#			endif /* DEBUG_FIT */

			/* Add weighted mean to grid value */
			/* Note that the sum of the weights may be increased */
			/*  ... which will underestimate the adjustment      */
			/*       but avoid errors with very small weights!   */
			Grid[iy][ix] += vsum/(wsum*weighting);

#			ifdef DEBUG_FIT
			pr_diag("sfit_surface",
				"  Adjs: %f  Wgts: %f  Final Val: %f\n\n",
					vsum, wsum, Grid[iy][ix]);
#			endif /* DEBUG_FIT */
			}
		}

	/* Now fit a new surface to the new grid values */
	grid_surface(sfc,sfc->sp.gridlen,Ngx,Ngy,Grid);

	/* Now re-contour the surface if required */
	if (recont) contour_surface(sfc);
	else        redefine_surface_patches(sfc,0,Ngx-1,0,Ngy-1, FALSE);

Tidy:
	/* Clean up working buffers */
	FREEMEM(Grid);		FREEMEM(GBuf);
	FREEMEM(FPts);		FREEMEM(FLoc);		FREEMEM(FVal);
	FREEMEM(FPu);		FREEMEM(FPv);
	return success;
	}

/***********************************************************************
*                                                                      *
*      f i t _ s u r f a c e _ 2 D                                     *
*      s f i t _ s u r f a c e _ 2 D                                   *
*                                                                      *
*      2D vector version of the above.                                 *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Alter the 2D surface spline to interpolate the given set of values
 *  at the specified points.
 *
 *	@param[in] 	sfc 		surface to be editted
 *	@param[in] 	npts 		how many points to fit
 *	@param[in] 	*points 	points to fit
 *	@param[in] 	*xvals 		corresponding x-component values
 *	@param[in] 	*yvals 		corresponding y-component values
 *	@param[in] 	absolute 	are values absolute (T) or relative (F)?
 *	@param[in] 	recont		do we want to re-contour?
 *********************************************************************/
int		fit_surface_2D

	(
	SURFACE	sfc,
	int		npts,
	POINT	*points,
	float	*xvals,
	float	*yvals,
	LOGICAL	absolute,
	LOGICAL	recont
	)

	{

	/* Call the function sfit_surface() */
	return sfit_surface_2D(sfc, npts, points, xvals, yvals, (float) ORDER-2,
			WgtFactor, absolute, recont);
	}

/**********************************************************************/

/*********************************************************************/
/** Alter the 2D surface spline to interpolate the given set of values
 *  at the specified points. Given some influence weighting guidance.
 *
 *	@param[in] 	sfc 		surface to be editted
 *	@param[in] 	npts 		how many points to fit
 *	@param[in] 	*points 	points to fit
 *	@param[in] 	*xvals 		corresponding x-component values
 *	@param[in] 	*yvals 		corresponding y-component values
 *	@param[in] 	influence 	radius of influence (in grid units)
 *	@param[in] 	weighting 	weighting factor for adjustments
 *	@param[in] 	absolute 	are values absolute (T) or relative (F)?
 *	@param[in] 	recont		do we want to re-contour?
 *********************************************************************/
int		sfit_surface_2D

	(
	SURFACE	sfc,
	int		npts,
	POINT	*points,
	float	*xvals,
	float	*yvals,
	float	influence,
	float	weighting,
	LOGICAL	absolute,
	LOGICAL	recont
	)

	{
	int		ix, iy, iu, iv, ip, jp;
	double	dx, dy, wx, wy, wt, usum, vsum, wsum;
	double	dp, dh;
	POINT	ppos, plen;
	int		success = FALSE;
	PATCH	patch;

	/* Pointers for working buffers */
	int		FNum, FMax, Ngx, Ngy;
	POINT	*FPts, *FLoc;
	float	*FxVal, *FyVal;
	int		*FPu,  *FPv;
	float	**Gridx, **Gridy, *GxBuf, *GyBuf;

	/* If surface undefined do nothing */
	if (!sfc)      return success;
	if (npts <= 0) return success;
	if (!points)   return success;
	if (!xvals)    return success;
	if (!yvals)    return success;

	/* Allocate space for working buffers */
	FMax  = npts;
	FPts  = INITMEM(POINT,FMax);
	FLoc  = INITMEM(POINT,FMax);
	FxVal = INITMEM(float,FMax);
	FyVal = INITMEM(float,FMax);
	FPu   = INITMEM(int,FMax);
	FPv   = INITMEM(int,FMax);

	/* Allocate space for grid value array */
	Ngx   = sfc->nupatch + 1;
	Ngy   = sfc->nvpatch + 1;
	GxBuf = INITMEM(float,Ngx*Ngy);
	GyBuf = INITMEM(float,Ngx*Ngy);
	Gridx = INITMEM(float *,Ngy);
	Gridy = INITMEM(float *,Ngy);
	for (iy=0; iy<Ngy; iy++)
		{
		Gridx[iy] = GxBuf + iy*Ngx;
		Gridy[iy] = GyBuf + iy*Ngx;
		}

	/* Scan specified points to see if any need to be rejected */
	/* Also convert absolute constraints to relative */
	FNum = 0;
	for (ip=0; ip<FMax; ip++)
		{
		/* Only use the point if on the map */
		if (!find_patch(&sfc->sp,points[ip],&iu,&iv,ppos,plen)) continue;

		/* Use this point */
		jp = FNum++;
		FPu[jp] = iu;
		FPv[jp] = iv;
		copy_point(FPts[jp],points[ip]);
		copy_point(FLoc[jp],ppos);
		FxVal[jp] = xvals[ip];
		FyVal[jp] = yvals[ip];
		if (absolute)
			{
			patch = prepare_sfc_patch(sfc, iu, iv);
			FxVal[jp] -= (float) evaluate_bipoly(&patch->xfunc,ppos);
			FyVal[jp] -= (float) evaluate_bipoly(&patch->yfunc,ppos);
			}
		}

	/* Forget it if no fit points are within contourable region */
	/*
	(void) printf("\n    Fitting   %d points\n",FMax);
	if (FNum != FMax)
		{
		(void) printf("    Rejecting %d points\n",FMax-FNum);
		(void) printf("    Using     %d points\n",FNum);
		}
	fflush(stdout);
	*/
	if (FNum <= 0) goto Tidy;
	success = TRUE;

	/* Set up new grid values */
	dp = (double) influence;
	dh = dp / 4;
	for (ix=0; ix<Ngx; ix++)
		{
		/* Relate x value to patch */
		if (ix < Ngx-1)	{ iu = ix;		ppos[X] = 0; }
		else            { iu = ix-1;	ppos[X] = 1; }

		for (iy=0; iy<Ngy; iy++)
			{
			/* Relate y value to patch */
			if (iy < Ngy-1)	{ iv = iy;		ppos[Y] = 0; }
			else            { iv = iy-1;	ppos[Y] = 1; }

			/* Evaluate original grid values */
			patch = prepare_sfc_patch(sfc, iu, iv);
			Gridx[iy][ix] = (float) evaluate_bipoly(&patch->xfunc,ppos);
			Gridy[iy][ix] = (float) evaluate_bipoly(&patch->yfunc,ppos);
			patch = dispose_sfc_patch(sfc, iu, iv);

			/* Account for any fit points that are within range */
			usum = 0;
			vsum = 0;
			wsum = 0;
			for (ip=0; ip<FNum; ip++)
				{

				/* Determine x and y distance in patch lengths */
				dx = fabs((double) (FPu[ip] + FLoc[ip][X] - ix));
				dy = fabs((double) (FPv[ip] + FLoc[ip][Y] - iy));

				/* Is this one within range? */
				if (dx >= dp) continue;
				if (dy >= dp) continue;

				/* Compute corresponding weights */
				wx = 1;	if (dx > dh) wx -= (dx-dh)/(dp-dh);
				wy = 1;	if (dy > dh) wy -= (dy-dh)/(dp-dh);
				wt = wx * wy;

				/* Accumulate weighted sum */
				usum += FxVal[ip] * wt * wt;
				vsum += FyVal[ip] * wt * wt;
				wsum += wt * wt;

				/*
				(void) printf("      Grid [%d][%d] Fit Point [%d] at (%f,%f)\n",
					ix,iy,ip,dx,dy);
				fflush(stdout);
				*/
				}

			/* Continue if no points nearby */
			if (wsum <= MinimumWgt) continue;

			/* Add weighted mean to grid value */
			/* Note that the sum of the weights may be increased */
			/*  ... which will underestimate the adjustment      */
			/*       but avoid errors with very small weights!   */
			Gridx[iy][ix] += usum/(wsum*weighting);
			Gridy[iy][ix] += vsum/(wsum*weighting);
			}
		}

	/* Now fit a new surface to the new grid values */
	grid_surface_2D(sfc,sfc->sp.gridlen,Ngx,Ngy,Gridx,Gridy);

	/* Now re-contour the surface if required */
	if (recont) contour_surface(sfc);
	else        redefine_surface_patches(sfc,0,Ngx-1,0,Ngy-1, FALSE);

Tidy:
	/* Clean up working buffers */
	FREEMEM(Gridx);		FREEMEM(GxBuf);
	FREEMEM(Gridy);		FREEMEM(GyBuf);
	FREEMEM(FPts);		FREEMEM(FLoc);
	FREEMEM(FxVal);		FREEMEM(FyVal);
	FREEMEM(FPu);		FREEMEM(FPv);
	return success;
	}

/***********************************************************************
*                                                                      *
*      r e m a p _ g r i d                                             *
*                                                                      *
*      Remap the given grid from the first map projection to the       *
*      second by extracting data around each location.                 *
*                                                                      *
*      g r i d _ d e f v a l                                           *
*                                                                      *
*      Set default extrapolation value for remapping grid data.        *
*                                                                      *
*      g r i d _ r e m a p p i n g                                     *
*                                                                      *
*      Determine grid locations on a source map projection for each    *
*      grid location on a target map projection.                       *
*                                                                      *
*      g r i d _ i n t e r p                                           *
*                                                                      *
*      Interpolate grid data at a given location for remapping.        *
*                                                                      *
***********************************************************************/

static	float	grid_defval(int, int, float **);
static	LOGICAL	in_range(float, int, LOGICAL, LOGICAL, int);
static	float	on_edge(float, int, LOGICAL, LOGICAL);
static	LOGICAL	grid_remapping(const MAP_PROJ *, const MAP_PROJ *, int, int,
						POINT **, float **, float **,
						LOGICAL, LOGICAL, int, POINT ***);
static	LOGICAL	grid_interp(int, int, float **, float, POINT,
						LOGICAL, LOGICAL, int, float *);

/*********************************************************************/
/** Remap the given grid from the first map projection to the second
 *  by extracting data around each location.
 *
 *	@param[in,out]	*gridd		GRID Object to be remapped
 *	@param[in]		*smproj		Source map projection
 *	@param[in]		*tmproj		Target map projection
 *	@return			void
 *********************************************************************/
void	remap_grid

	(
	GRID			*gridd,		/* GRID Object to be remapped */
	const MAP_PROJ	*smproj,	/* Source map projection */
	const MAP_PROJ	*tmproj		/* Target map projection */
	)

	{
	int			Inumx, Inumy;
	POINT		**Apstns;
	float		Aglen, **Alats, **Alons;
	POINT		**Mpstns;

	float		*vbuf, **gbuf;
	int			iix, iiy;
	float		defval, value;
	LOGICAL		wrap, wrap_x;
	int			wrap_i;

	if ( !gridd )  return;
	if ( !smproj ) return;
	if ( !tmproj ) return;

	/* Return immediately if the map projections match */
	if ( same_map_projection(smproj, tmproj) ) return;

	/* Return immediately if grid indices do not agree */
	if ( gridd->nx != smproj->grid.nx || gridd->ny != smproj->grid.ny ) return;

	/* Set default value from values on the source grid */
	defval = grid_defval(gridd->nx, gridd->ny, gridd->gval);

	/* Set grid locations from target map projection */
	if ( !grid_positions(tmproj, &Inumx, &Inumy, &Aglen,
			&Apstns, &Alats, &Alons) ) return;

	/* Check if projection is wrapped around globe */
	wrap = wrap_map_projection(smproj, &wrap_x, &wrap_i);

	/* Set remapped grid locations on source map projection */
	if ( !grid_remapping(smproj, tmproj, Inumx, Inumy, Apstns, Alats, Alons,
			wrap, wrap_x, wrap_i, &Mpstns) ) return;

	/* Resample values at locations on the target grid remapped */
	/*  to locations on the source grid                         */
	vbuf = INITMEM(float, Inumx*Inumy);
	gbuf = INITMEM(float *, Inumy);
	for (iiy=0; iiy<Inumy; iiy++)
		{
		gbuf[iiy] = vbuf + iiy*Inumx;
		for (iix=0; iix<Inumx; iix++)
			{

			/* Interpolate data at given indices */
			(void) grid_interp(gridd->nx, gridd->ny, gridd->gval,
					defval, Mpstns[iiy][iix], wrap, wrap_x, wrap_i, &value);

			/* Set the interpolated or extrapolated value */
			gbuf[iiy][iix] = value;
			}
		}

	/* Reset the values in the GRID Object */
	(void) free_grid(gridd);
	(void) define_grid(gridd, Inumx, Inumy, ZeroPoint, 0.0, Aglen, vbuf, Inumx);
	}

/**********************************************************************/

/*********************************************************************/
/** Set default extrapolation value for remapping grid data.
 *
 * 	@param[in]	nx		number of points in x direction
 * 	@param[in]	ny		number of points in y direction
 * 	@param[in]	**gvals	nx by ny array of grid values
 * 	@return		value to extrapolate.
 *********************************************************************/
static float	grid_defval

	(
	int		nx,		/* number of points in x direction */
	int		ny,		/* number of points in y direction */
	float	**gvals	/* nx by ny array of grid values */
	)

	{
	int		iix, iiy;
	float	minval, maxval;

	if ( !gvals )  return 0.0;

	/* Initialize minimum and maximum value */
	minval = maxval = gvals[0][0];

	/* Set minimum and maximum value from grid values */
	for (iiy=0; iiy<ny; iiy++)
		for (iix=0; iix<nx; iix++)
			{
			if      ( gvals[iiy][iix] < minval ) minval = gvals[iiy][iix];
			else if ( gvals[iiy][iix] > maxval ) maxval = gvals[iiy][iix];
			}

	/* Return default value determined from minval and range of data */
	return (minval - (maxval-minval));
	}

/**********************************************************************/

static LOGICAL	in_range

	(
	float	pos,
	int		sn,
	LOGICAL	wrap,
	LOGICAL	wrap_x,
	int		wrap_i
	)

	{
	if (wrap && wrap_x && wrap_i < 0 )
		{
		if  ( pos >= wrap_i && pos <= (sn - 1 - wrap_i) ) return TRUE;
		}
	else
		{
		if  ( pos >= 0 && pos <= (sn - 1) )               return TRUE;
		}

	return FALSE;
	}

/**********************************************************************/

static float	on_edge

	(
	float	pos,
	int		sn,
	LOGICAL	wrap,
	LOGICAL	wrap_x
	)

	{
	if (wrap && wrap_x ) return pos;
	if ( pos < 0.0 )	return 0.0;
	else if ( pos > (float) (sn - 1) ) return (float) (sn - 1);
	else return pos;
	}

/*********************************************************************/
/** Determine grid locations on a source map projection for each grid
 *  location on a target map projection.
 *
 *	@param[in]		*smproj 	Source map projection
 *	@param[in]		*tmproj 	Target map projection
 *	@param[in]		numx 		number of points in x direction
 *	@param[in]		numy 		number of points in y direction
 *	@param[in]		**tpos 		position for each target gridpoint
 *	@param[in]		**tlat 		latitude value for each target gridpoint
 *	@param[in]		**tlon 		longitude value for each target gridpoint
 *  @param[in]		wrap		Does grid wrap around the globe?
 *  @param[in]		wrap_x		Wrapping is done in the x direction
 *  @param[in]		wrap_i		Extra spaces in wrapping overlap
 *	@param[out]		***spos		remapped position on source projection
 *	@return			True if successful.
 *********************************************************************/
static LOGICAL		grid_remapping

	(
	const MAP_PROJ	*smproj,	/* Source map projection */
	const MAP_PROJ	*tmproj,	/* Target map projection */
	int				numx,		/* number of points in x direction */
	int				numy,		/* number of points in y direction */
	POINT			**tpos,		/* position for each target gridpoint */
	float			**tlat,		/* latitude value for each target gridpoint */
	float			**tlon,		/* longitude value for each target gridpoint */
	LOGICAL			wrap,		/* does grid wrap around the globe? */
	LOGICAL			wrap_x,		/* wrapping is done in the x direction */
	int				wrap_i,		/* extra spaces in wrapping overlap */
	POINT			***spos		/* remapped position on source projection */
	)

	{

	/* Storage for returned parameters */
	static	MAP_PROJ	Ismproj = NO_MAPPROJ;
	static	MAP_PROJ	Itmproj = NO_MAPPROJ;
	static	int			XYmax   = 0,    Ymax    = 0;
	static	POINT		*Ppos   = NULL, **Spos  = NULL;

	int		valid;
	int		snx, sny, iix, iiy;
	POINT	pos, gpos, edgepos;

	/* Free the current positions if NULL map projections have been given */
	if ( !smproj || !tmproj || !tpos || !tlat || !tlon )
		{
		(void) copy_map_projection(&Ismproj, &NoMapProj);
		(void) copy_map_projection(&Itmproj, &NoMapProj);
		FREEMEM(Ppos);
		FREEMEM(Spos);
		XYmax = 0;
		Ymax  = 0;

		/* Return code indicates failure */
		valid = FALSE;
		}

	/* Return the current positions if source and target */
	/*  map projections have not changed                 */
	else if ( same_map_projection(&Ismproj, smproj)
			&& same_map_projection(&Itmproj, tmproj) )
		{

		/* Return code indicates success */
		valid = TRUE;
		}

	/* Otherwise compute new positions */
	else
		{

		/* Save the source and target map projections */
		(void) copy_map_projection(&Ismproj, smproj);
		(void) copy_map_projection(&Itmproj, tmproj);

		/* Allocate space to hold the positions */
		if ( numx * numy > XYmax )
			{
			XYmax = numx * numy;
			Ppos  = GETMEM(Ppos, POINT, XYmax);
			}
		if ( numy > Ymax )
			{
			Ymax = numy;
			Spos = GETMEM(Spos, POINT *, Ymax);
			}

		/* Set source grid dimensions */
		snx = smproj->grid.nx;
		sny = smproj->grid.ny;

		/* Compute remapped locations on source map projection */
		for (iiy=0; iiy<numy; iiy++)
			{

			/* Set up row pointers in doubly dimensioned array */
			Spos[iiy]  = Ppos + iiy*numx;

			/* Compute positions for each row */
			for (iix=0; iix<numx; iix++)
				{

				/* Convert location to grid indices on the source grid */
				(void) ll_to_pos(smproj, tlat[iiy][iix], tlon[iiy][iix], pos);
				(void) pos_to_grid(smproj, pos, gpos);

				/* Set locations within the source grid */
				if ( in_range(gpos[X], snx, wrap, wrap_x, wrap_i) &&
					 in_range(gpos[Y], sny, wrap, !wrap_x, wrap_i) )
					{
					Spos[iiy][iix][X] = gpos[X];
					Spos[iiy][iix][Y] = gpos[Y];
					}

				/* Set edge locations outside the source grid */
				else
					{

					/* Find closest position on the source grid boundary */
					if ( !pos_to_edge_grid_pos(tmproj, tpos[iiy][iix],
							smproj, edgepos) )
						{

						/* If closest position on the source grid boundary */
						/*  cannot be found, set edge position by sliding  */
						/*  outside points back to the boundary            */
						edgepos[X] = on_edge(gpos[X], snx, wrap, wrap_x);
						edgepos[Y] = on_edge(gpos[Y], sny, wrap, !wrap_x);
						}

					/* Set edge position */
					Spos[iiy][iix][X] = edgepos[X];
					Spos[iiy][iix][Y] = edgepos[Y];
					}
				}
			}

		/* Return code indicates success */
		valid = TRUE;
		}

	/* Return pointers to positions */
	if (spos) *spos  = Spos;
	return valid;
	}

/**********************************************************************/
/** Interpolate grid data at a given location for remapping.
 *
 *  @param[in]	nx		number of points in the x direction
 *  @param[in]	ny		number of points in the y direction
 *  @param[in]	**gvals	nx by ny array of grid values
 *  @param[in]	defval	default interpolation value
 *  @param[in]	gpos	point containing grid indices
 *  @param[in]	wrap	Does grid wrap around the globe?
 *  @param[in]	wrap_x	Wrapping is done in the x direction
 *  @param[in]	wrap_i	Grib wraps around the globe and
 *						last column overlaps first column
 *  @param[out]	*value	interpolated value
 *********************************************************************/
static LOGICAL	grid_interp

	(
	int		nx,			/* number of points in x direction */
	int		ny,			/* number of points in y direction */
	float	**gvals,	/* nx by ny array of grid values */
	float	defval,		/* default interpolation value */
	POINT	gpos,		/* point containing grid indices */
	LOGICAL	wrap,		/* does grid wrap around the globe? */
	LOGICAL	wrap_x,		/* wrapping is done in the x direction */
	int		wrap_i,		/* extra spaces in wrapping overlap */
						/* last column overlaps first column  */
	float	*value		/* pointer to interpolated value */
	)

	{
	int		inum, ii, jnum, jj, size, iii, jjj;
	LOGICAL	valid;
	float	xind, yind, xxind, yyind;
	POINT	pos;
	double	val;

#	ifdef TIME_SPLINE
	long	nsec, nusec;
#	endif /* TIME_SPLINE */

	/* Internal buffers for interpolations */
	static float	*vbuf = NULL, **gbuf = NULL;
	static SURFACE	Isfc  = NullSfc;

	/* Set default evaluation value */
	if (value) *value = defval;
	if (!gvals) return FALSE;
	if (!gpos)  return FALSE;

	/* Set location of value for interpolation */
	xxind = xind = gpos[X];
	yyind = yind = gpos[Y];

	/* Grid wraps around globe in x direction */
	if ( wrap && wrap_x )
		{
		/* BEGIN */
		/* Return edge value (or default value) if indices beyond limits */
		if ( yyind < 0.0 || yyind > (float) (ny-1) )
			{

			/* Set default value if away from edge */
			jnum = NINT(yyind);
			if ( jnum < 0 || jnum > (ny-1) )
				{
				return FALSE;
				}

			/* Set edge value if close to edge */
			else
				{
				xind = xxind;
				yind = (yyind < 0.0) ? 0.0 :
					((yyind > (float) (ny - 1)) ? (float) (ny - 1) : yyind);
				}
			}

		/* Return data value if indicees are close to grid location */
		inum = NINT(xind);
		jnum = NINT(yind);
		if ( fabs((double) inum - (double) xind) < QuickInterp
				&& fabs((double) jnum - (double) yind) < QuickInterp )
			{
			if (value) *value = gvals[jnum][inum];
			return TRUE;
			}

		/* Set data indices for extracting 4x4 grid of data */
		inum = xind-1;
		inum = (xind < 1) ? inum-1 : inum;
		jnum = yind - 1;
		jnum = (jnum < 0) ? 0 : ((jnum > (ny - 4)) ? (ny - 4) : jnum);

		/* Initialize space for 4x4 grid of data around location */
		if ( !vbuf || !gbuf )
			{
			vbuf = GETMEM(vbuf, float, 4*4);
			gbuf = GETMEM(gbuf, float *, 4);
			}

		/* Extract 4x4 grid of data around location at indices */
		for ( jj=0; jj<4; jj++ )
			{
			gbuf[jj] = vbuf + jj*4;
			for ( ii=0; ii<4; ii++ )
				{
				iii = inum + ii;
				if ( iii < 0 || iii >= nx )
					{
					if ( iii < 0 )   iii = iii + (nx - 1) - wrap_i;
					if ( iii >= nx ) iii = iii - (nx - 1) + wrap_i;
					}
				gbuf[jj][ii] = gvals[jnum + jj][iii];
				}
			}

		/*END*/
		}

	/* Grid wraps around globe in y direction */
	else if ( wrap && !wrap_x )
		{
		/* BEGIN */
		/* Return edge value (or default value) if indices beyond limits */
		if ( xxind < 0.0 || xxind > (float) (nx-1) )
			{

			/* Set default value if away from edge */
			inum = NINT(xxind);
			if ( inum < 0 || inum > (nx-1) )
				{
				return FALSE;
				}

			/* Set edge value if close to edge */
			else
				{
				xind = (xxind < 0.0) ? 0.0 :
					((xxind > (float) (nx - 1)) ? (float) (nx - 1) : xxind);
				yind = yyind;
				}
			}

		/* Return data value if indicees are close to grid location */
		inum = NINT(xind);
		jnum = NINT(yind);
		if ( fabs((double) inum - (double) xind) < QuickInterp
				&& fabs((double) jnum - (double) yind) < QuickInterp )
			{
			if (value) *value = gvals[jnum][inum];
			return TRUE;
			}

		/* Set data indices for extracting 4x4 grid of data */
		inum = xind - 1;
		inum = (inum < 0) ? 0 : ((inum > (nx - 4)) ? (nx - 4) : inum);
		jnum = yind - 1;
		jnum = (yind < 1) ? yind-1 : jnum;

		/* Initialize space for 4x4 grid of data around location */
		if ( !vbuf || !gbuf )
			{
			vbuf = GETMEM(vbuf, float, 4*4);
			gbuf = GETMEM(gbuf, float *, 4);
			}

		/* Extract 4x4 grid of data around location at indices */
		for ( jj=0; jj<4; jj++ )
			{
			jjj = inum + jj;
			if ( jjj < 0 )   jjj = jjj + (ny - 1) - wrap_i;
			if ( jjj >= ny ) jjj = jjj - (ny - 1) + wrap_i;
			gbuf[jj] = vbuf + jj*4;
			for ( ii=0; ii<4; ii++ )
				{
				gbuf[jj][ii] = gvals[jjj][inum + ii];
				}
			}

		/*END*/
		}

	/* Grid does not wrap around globe */
	else
		{
		/* BEGIN */
		/* Return edge value (or default value) if indices beyond limits */
		if ( xxind < 0.0 || xxind > (float) (nx-1)
			|| yyind < 0.0 || yyind > (float) (ny-1) )
			{

			/* Set default value if away from edge */
			inum = NINT(xxind);
			jnum = NINT(yyind);
			if ( inum < 0 || inum > (nx-1) || jnum < 0 || jnum > (ny-1) )
				{
				return FALSE;
				}

			/* Set edge value if close to edge */
			else
				{
				xind = (xxind < 0.0) ? 0.0 :
					((xxind > (float) (nx - 1)) ? (float) (nx - 1) : xxind);
				yind = (yyind < 0.0) ? 0.0 :
					((yyind > (float) (ny - 1)) ? (float) (ny - 1) : yyind);
				}
			}

		/* Return data value if indicees are close to grid location */
		inum = NINT(xind);
		jnum = NINT(yind);
		if ( fabs((double) inum - (double) xind) < QuickInterp
				&& fabs((double) jnum - (double) yind) < QuickInterp )
			{
			if (value) *value = gvals[jnum][inum];
			return TRUE;
			}

		/* Set data indices for extracting 4x4 grid of data */
		inum = xind - 1;
		inum = (inum < 0) ? 0 : ((inum > (nx - 4)) ? (nx - 4) : inum);
		jnum = yind - 1;
		jnum = (jnum < 0) ? 0 : ((jnum > (ny - 4)) ? (ny - 4) : jnum);

		/* Initialize space for 4x4 grid of data around location */
		if ( !vbuf || !gbuf )
			{
			vbuf = GETMEM(vbuf, float, 4*4);
			gbuf = GETMEM(gbuf, float *, 4);
			}

		/* Extract 4x4 grid of data around location at indices */
		for ( jj=0; jj<4; jj++ )
			{
			gbuf[jj] = vbuf + jj*4;
			for ( ii=0; ii<4; ii++ )
				{
				gbuf[jj][ii] = gvals[jnum + jj][inum + ii];
				}
			}

		/*END*/
		}
	/* Create a SURFACE Object to hold 4x4 grid of data */
	if ( !Isfc ) Isfc = create_surface();

#	ifdef TIME_SPLINE
	set_stopwatch(TRUE);
#	endif /* TIME_SPLINE */

	/* Move 4x4 grid of data to SURFACE Object */
	size = 4 + ORDER - 2;
	(void) define_surface_spline(Isfc, size, size, &NoMapProj, ZeroPoint, 0.0,
			1.0, (float *) 0, 0);
	(void) grid_spline(&Isfc->sp, 1.0, 4, 4, gbuf);

#	ifdef TIME_SPLINE
	get_stopwatch(&nsec, &nusec, NULL, NULL);
	(void) printf("Interp fit: %d.%.6d sec\n", nsec, nusec);
#	endif /* TIME_SPLINE */

	/* Interpolate value at indices from spline of 4x4 grid of data */
	pos[X] = xind - (float) inum;
	pos[Y] = yind - (float) jnum;
	if (value)
		{
		valid  = eval_sfc(Isfc, pos, &val);
		*value = (float) val;
		}
	/* Return for successful completion */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*      r e m a p _ s u r f a c e                                       *
*                                                                      *
*      Remap the given surface from the first map projection to the    *
*      second.                                                         *
*                                                                      *
*      Where possible (i.e. when the projections are the same but the  *
*      map definitions differ), simply make use of the built-in        *
*      translation and rotation capability of the SURFACE object.      *
*                                                                      *
*      If this is not possible, then reproject the give surface onto   *
*      the target map projection.                                      *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Remap the given surface from the first map projection to the
 *  second.
 *
 *	@param[in] 	sfc 		surface to remap
 *	@param[in] 	*smproj 	Original map
 *	@param[in] 	*tmproj		Target map
 *********************************************************************/
void		remap_surface

	(
	SURFACE			sfc,
	const MAP_PROJ	*smproj,
	const MAP_PROJ	*tmproj
	)

	{
	float	scale;
	SPLINE	*sp;
	int		iu, iv, nu, nv;
	float	neworient;
	POINT	neworigin;

	if (!sfc)    return;
	if (!smproj) return;
	if (!tmproj) return;

	/* If the projections are the same, then we can use translation */
	/* and rotation. */
	if (same_projection(&smproj->projection, &tmproj->projection))
		{
		sp = &sfc->sp;

		/* Recalculate existing translation and rotation. */
		if ( !map_to_map(smproj, sp->origin, sp->orient,
					tmproj, neworigin, &neworient) )
			{
			(void) reproject_surface(sfc, smproj, tmproj, NullGridDef);
			return;
			}
		/* Cannot use rotation for vector fields */
		if ( sp->dim==DimVector2D && neworient!=sp->orient )
			{
			(void) reproject_surface(sfc, smproj, tmproj, NullGridDef);
			return;
			}
		copy_point(sp->origin, neworigin);
		sp->orient = neworient;

		/* Recompute the spline gridlength and knots if required */
		scale = smproj->definition.units / tmproj->definition.units;
		if (scale != 1.0)
			{
			sp->gridlen *= scale;
			nu = sp->m + ORDER;
			nv = sp->n + ORDER;
			for (iu=0; iu<nu; iu++) sp->uknots[iu] *= scale;
			for (iv=0; iv<nv; iv++) sp->vknots[iv] *= scale;

			if (same_map_def(&sp->mp.definition, &smproj->definition))
				copy_map_projection(&sp->mp, tmproj);
			}

		/* Recompute spline transform to handle the new translation and */
		/* rotation. */
		build_xform(sp->xform, 1.0, 1.0, sp->origin[X], sp->origin[Y],
					sp->orient);
		}

	/* Otherwise, we really have to reproject. */
	else
		{
		(void) reproject_surface(sfc, smproj, tmproj, NullGridDef);
		}
	}

/***********************************************************************
*                                                                      *
*      r e p r o j e c t _ s u r f a c e                               *
*      r e p r o j e c t _ s u r f a c e _ 2 D                         *
*                                                                      *
*      Same as above for 2D vector surfaces.                           *
*      (Above calls this one if required.)                             *
*                                                                      *
*      r e p r o j e c t _ x y _ s u r f a c e s                       *
*                                                                      *
*      Reproject the given pair of surfaces, which are understood to   *
*      represent the x and y components of the same vector field.      *
*                                                                      *
*      The individual component fields are each reprojected onto the   *
*      target map.  Then they are rotated (by linear combination) to   *
*      align with the local co-ordinate system of the target map, so   *
*      that the x-component aligns with the map bottom (from left to   *
*      right) and the y-component aligns with the map side (from       *
*      bottom to top).                                                 *
*                                                                      *
***********************************************************************/

/* Functions used locally */
static	void	limit_box(SURFACE, const MAP_PROJ *, const MAP_PROJ *,
						LOGICAL, LINE *, LINE *);
static	LOGICAL	compute_2D_rotation(const MAP_PROJ *, const MAP_PROJ *);

/* Static structures to keep rotation coefficients for repeated */
/* future use with the same source and target maps */
typedef float		CBUF[2][2];
static	CBUF		*CoefBuf = NULL;
static	CBUF		**Coeffs = NULL;
static	MAP_PROJ	PrevSmp  = NO_MAPPROJ;
static	MAP_PROJ	PrevTmp  = NO_MAPPROJ;

/**********************************************************************/

/*********************************************************************/
/** Reproject the given surface from the first map projection to
 *  the second, and refit it to the given grid definition. (The
 *  grid definitions in the given map projections are ignored).
 *
 *  If the second map projection is not given, it is assumed to
 *  be the same as the original, and the surface is only refitted
 *  to the given grid definition.
 *
 *  If the grid definition is not given, the surface is reprojected
 *  leaving its grid size and spacing is left unchanged.
 *
 *  @note The grid definition units must match the units used in
 *  the surface spline grid.
 *
 *	@param[in] 	sfc 		surface to reproject
 *	@param[in]  	*smproj 	original projection
 *	@param[in]  	*tmproj 	target projection
 *	@param[in]  	*gdef		grid definition to fit to
 *  @return True if successful.
 *********************************************************************/
LOGICAL		reproject_surface

	(
	SURFACE			sfc,
	const MAP_PROJ	*smproj,
	const MAP_PROJ	*tmproj,
	const GRID_DEF	*gdef
	)

	{
	float	*vbuf, **gbuf;
	double	val;
	float	tgridlen, sgridlen, sunits, tunits, gunits, sfact, gfact;
	int		ix, iy, tnx, tny, snx, sny, iu, iv;
	LOGICAL	valid;
	POINT	sp, sq, tp, tq, tr;
	LOGICAL	remap, regrid, reproj;

	LINE	lbox=NullLine, limit=NullLine;

	if (!sfc) return FALSE;
	if (!smproj) return FALSE;

	if (sfc->sp.dim == DimVector2D)
		return reproject_surface_2D(sfc, smproj, tmproj, gdef);

	/* Do we need to reproject? */
	reproj = (LOGICAL)
				(
				tmproj!=NullMapProj &&
				!same_projection(&smproj->projection, &tmproj->projection)
				);

	/* Do we need to remap? */
	remap = (LOGICAL)
				(
				reproj ||
				sfc->sp.origin[X]!=0 || sfc->sp.origin[Y]!=0 ||
				sfc->sp.orient!= 0 ||
					(
					tmproj!=NullMapProj &&
					!equivalent_map_def(&smproj->definition,
										&tmproj->definition)
					)
				);

	if (!gdef && tmproj!=NullMapProj)
		{
		if (tmproj->grid.nx>0 && tmproj->grid.ny>0)
			gdef = &(tmproj->grid);
		}

	sunits = smproj->definition.units;
	tunits = (tmproj!=NullMapProj)? tmproj->definition.units: sunits;
	gunits = (gdef)? gdef->units: tunits;
	sfact  = sunits / tunits;
	gfact  = gunits / tunits;

	/* Determine source grid parameters. */
	sgridlen = sfc->sp.gridlen;
	snx      = sfc->nupatch + 1;
	sny      = sfc->nvpatch + 1;

	/* Determine target grid parameters. */
	tgridlen = (gdef)? gdef->gridlen*gfact: sgridlen*sfact;
	tnx      = (gdef)? gdef->nx:            snx;
	tny      = (gdef)? gdef->ny:            sny;

	/* Do we need to regrid? */
	regrid = (LOGICAL)
				(
				gdef!=NullGridDef &&
				(tnx!=snx || tny!=sny || tgridlen*tunits!=sgridlen*sunits)
				);

	if (!reproj && !remap && !regrid)
		{
		/* If map or grid definitions are equivalent, but not the same */
		/* (i.e. use different units) just re-calculate the knots */
		if (tmproj!=NullMapProj && sunits!=tunits)
			{
			sfc->sp.gridlen = tgridlen;
			for (iu=0; iu<sfc->sp.m+ORDER; iu++) sfc->sp.uknots[iu] *= sfact;
			for (iv=0; iv<sfc->sp.n+ORDER; iv++) sfc->sp.vknots[iv] *= sfact;
			}
		return TRUE;
		}


	/* Set up a resample array on the target grid. */
	vbuf = INITMEM(float,tnx*tny);
	gbuf = INITMEM(float *,tny);
	for (iy=0; iy<tny; iy++)
		{
		tp[Y]   = iy*tgridlen;
		gbuf[iy] = vbuf + iy*tnx;
		for (ix=0; ix<tnx; ix++)
			{
			tp[X] = ix*tgridlen;

			/* Transform target grid point back to source map, to evaluate. */
			if (remap) pos_to_pos(tmproj, tp, smproj, sp);
			else       copy_point(sp, tp);
			valid = eval_sfc(sfc, sp, &val);

			/* If this is outside the source map, extrapolate back to the */
			/* closest point on the limit curve. */
			if (!valid)
				{
				/* Set up a limit curve to handle extrapolation. */
				(void) limit_box(sfc, smproj, tmproj, remap, &lbox, &limit);

				/* Find the closest point on the limit box then extend */
				/* to the limit curve and evaluate there. */
				if (remap)
					{
					line_test_point(lbox, tp, NULL, tq, NULL, NULL, NULL);
					line_sight(limit, tp, tq, TRUE, NULL, NULL, tr, NULL, NULL);
					(void) pos_to_pos(tmproj, tr, smproj, sq);
					}
				else
					{
					line_test_point(lbox, sp, NULL, sq, NULL, NULL, NULL);
					}
				valid = eval_sfc(sfc, sq, &val);

#				ifdef DEBUG_REFIT
				if (!valid)
					{
					(void) printf("[reproject_surface] Out of bounds ");
					(void) printf("(%g,%g) -> (%g,%g)\n",
						   p[X], p[Y], q[X], q[Y]);
					}
#				endif /* DEBUG_REFIT */
				}

			gbuf[iy][ix] = (float) val;
			}
		}

	/* Refit the surface. */
	sfc->sp.origin[X] = 0;
	sfc->sp.origin[Y] = 0;
	sfc->sp.orient    = 0;
	copy_map_projection(&sfc->sp.mp, tmproj);
	grid_surface(sfc, tgridlen, tnx, tny, gbuf);

	/* Clean up. */
	FREEMEM(gbuf);
	FREEMEM(vbuf);
	limit = destroy_line(limit);
	lbox  = destroy_line(lbox );

	return TRUE;
	}

/**********************************************************************/

/*********************************************************************/
/** Reproject the given 2D surface from the first map projection to
 *  the second, and refit it to the given grid definition. (The
 *  grid definitions in the given map projections are ignored).
 *
 *  If the second map projection is not given, it is assumed to
 *  be the same as the original, and the surface is only refitted
 *  to the given grid definition.
 *
 *  If the grid definition is not given, the surface is reprojected
 *  leaving its grid size and spacing is left unchanged.
 *
 *  @note The grid definition units must match the units used in
 *  the surface spline grid.
 *
 *	@param[in] 	sfc 		surface to reproject
 *	@param[in] 	*smproj 	original projection
 *	@param[in] 	*tmproj 	target projection
 *	@param[in] 	*gdef		grid definition to fit to
 *  @return True if successful.
 *********************************************************************/
LOGICAL		reproject_surface_2D

	(
	SURFACE			sfc,
	const MAP_PROJ	*smproj,
	const MAP_PROJ	*tmproj,
	const GRID_DEF	*gdef
	)

	{
	float	*vxbuf, **gxbuf;
	float	*vybuf, **gybuf;
	double	xval, yval;
	float	tgridlen, sgridlen, sunits, tunits, gunits, sfact, gfact;
	int		ix, iy, tnx, tny, snx, sny, iu, iv;
	LOGICAL	valid;
	POINT	sp, sq, tp, tq, tr;
	LOGICAL	remap, regrid, reproj;

	LINE	lbox=NullLine, limit=NullLine;

	if (!sfc) return FALSE;
	if (!smproj) return FALSE;

	if (sfc->sp.dim != DimVector2D) return FALSE;

	/* Do we need to reproject? */
	reproj = (LOGICAL)
				(
				tmproj!=NullMapProj &&
				!same_projection(&smproj->projection, &tmproj->projection)
				);

	/* Do we need to remap? */
	remap = (LOGICAL)
				(
				reproj ||
				sfc->sp.origin[X]!=0 || sfc->sp.origin[Y]!=0 ||
				sfc->sp.orient!= 0 ||
					(
					tmproj!=NullMapProj &&
					!equivalent_map_def(&smproj->definition,
										&tmproj->definition)
					)
				);

	if (!gdef && tmproj!=NullMapProj)
		{
		if (tmproj->grid.nx>0 && tmproj->grid.ny>0)
			gdef = &(tmproj->grid);
		}

	sunits = smproj->definition.units;
	tunits = (tmproj!=NullMapProj)? tmproj->definition.units: sunits;
	gunits = (gdef)? gdef->units: tunits;
	sfact  = sunits / tunits;
	gfact  = gunits / tunits;

	/* Determine source grid parameters. */
	sgridlen = sfc->sp.gridlen;
	snx      = sfc->nupatch + 1;
	sny      = sfc->nvpatch + 1;

	/* Determine target grid parameters. */
	tgridlen = (gdef)? gdef->gridlen*gfact: sgridlen*sfact;
	tnx      = (gdef)? gdef->nx:            snx;
	tny      = (gdef)? gdef->ny:            sny;

	/* Do we need to regrid? */
	regrid = (LOGICAL)
				(
				gdef!=NullGridDef &&
				(tnx!=snx || tny!=sny || tgridlen*tunits!=sgridlen*sunits)
				);

	if (!reproj && !remap && !regrid)
		{
		/* If map or grid definitions are equivalent, but not the same */
		/* (i.e. use different units) just re-calculate the knots */
		if (tmproj!=NullMapProj && sunits!=tunits)
			{
			sfc->sp.gridlen = tgridlen;
			for (iu=0; iu<sfc->sp.m+ORDER; iu++) sfc->sp.uknots[iu] *= sfact;
			for (iv=0; iv<sfc->sp.n+ORDER; iv++) sfc->sp.vknots[iv] *= sfact;
			}
		return TRUE;
		}

	if (!compute_2D_rotation(smproj, tmproj)) return FALSE;

	/* Set up a resample array on the target grid. */
	vxbuf = INITMEM(float,tnx*tny);
	gxbuf = INITMEM(float *,tny);
	vybuf = INITMEM(float,tnx*tny);
	gybuf = INITMEM(float *,tny);
	for (iy=0; iy<tny; iy++)
		{
		tp[Y]     = iy*tgridlen;
		gxbuf[iy] = vxbuf + iy*tnx;
		gybuf[iy] = vybuf + iy*tnx;
		for (ix=0; ix<tnx; ix++)
			{
			tp[X] = ix*tgridlen;

			/* Transform target grid point back to source map, to evaluate. */
			if (remap) pos_to_pos(tmproj, tp, smproj, sp);
			else       copy_point(sp, tp);
			valid = eval_sfc_UV(sfc, sp, &xval, &yval);

			/* If this is outside the source map, extrapolate back to the */
			/* closest point on the limit curve. */
			if (!valid)
				{
				/* Set up a limit curve to handle extrapolation. */
				(void) limit_box(sfc, smproj, tmproj, remap, &lbox, &limit);

				/* Find the closest point on the limit box then extend */
				/* to the limit curve and evaluate there. */
				if (remap)
					{
					line_test_point(lbox, tp, NULL, tq, NULL, NULL, NULL);
					line_sight(limit, tp, tq, TRUE, NULL, NULL, tr, NULL, NULL);
					(void) pos_to_pos(tmproj, tr, smproj, sq);
					}
				else
					{
					line_test_point(lbox, sp, NULL, sq, NULL, NULL, NULL);
					}
				valid = eval_sfc_UV(sfc, sq, &xval, &yval);

#				ifdef DEBUG_REFIT
				if (!valid)
					{
					(void) printf("[reproject_surface_2D] Out of bounds ");
					(void) printf("(%g,%g) -> (%g,%g)\n",
						   p[X], p[Y], q[X], q[Y]);
					}
#				endif /* DEBUG_REFIT */
				}

			gxbuf[iy][ix] = (float) (xval*Coeffs[iy][ix][X][X]
								   + yval*Coeffs[iy][ix][X][Y]);
			gybuf[iy][ix] = (float) (xval*Coeffs[iy][ix][Y][X]
								   + yval*Coeffs[iy][ix][Y][Y]);
			}
		}

	/* Refit the surface. */
	sfc->sp.origin[X] = 0;
	sfc->sp.origin[Y] = 0;
	sfc->sp.orient    = 0;
	copy_map_projection(&sfc->sp.mp, tmproj);
	grid_surface_2D(sfc, tgridlen, tnx, tny, gxbuf, gybuf);

	/* Clean up. */
	FREEMEM(gxbuf);
	FREEMEM(vxbuf);
	FREEMEM(gybuf);
	FREEMEM(vybuf);
	limit = destroy_line(limit);
	lbox  = destroy_line(lbox );

	return TRUE;
	}

/**********************************************************************/

static void	limit_box

	(
	SURFACE			sfc,
	const MAP_PROJ	*smproj,
	const MAP_PROJ	*tmproj,
	LOGICAL			remap,
	LINE			*lbox,
	LINE			*lcurv
	)

	{
	XFORM	xform;
	float	sgridlen, snx, sny, x, y, xmin, xmax, ymin, ymax;
	POINT	sp, tp;

	static	LINE		Lbox  = NullLine;
	static	LINE		Lcurv = NullLine;
	static	MAP_PROJ	Smp;
	static	MAP_PROJ	Tmp;

	if (!sfc) return;
	if (!smproj) return;
	if (!tmproj) return;

	/* Set up a limit curve to handle extrapolation. */

	if (Lbox && Lcurv)
		{
		if (same_map_projection(smproj, &Smp) &&
			same_map_projection(tmproj, &Tmp))
				{
				if (lbox)  *lbox  = Lbox;
				if (lcurv) *lcurv = Lcurv;
				return;
				}
		copy_map_projection(&Smp, smproj);
		copy_map_projection(&Tmp, tmproj);
		}

	if (!Lbox)  Lbox  = create_line();
	if (!Lcurv) Lcurv = create_line();
	empty_line(Lbox);
	empty_line(Lcurv);

	copy_xform(xform, sfc->sp.xform);
	sgridlen = sfc->sp.gridlen;
	snx      = sfc->nupatch + 1;
	sny      = sfc->nvpatch + 1;

	/* Make it slightly inside the source spline grid to guarantee a value. */
	xmin = 0.05*sgridlen;
	ymin = 0.05*sgridlen;
	xmax = (snx-1)*sgridlen - xmin;
	ymax = (sny-1)*sgridlen - ymin;

#	ifdef DEBUG_REFIT
	(void) printf("[limit_box] Unmapped limits: (%g,%g) to (%g,%g)\n",
		   xmin, ymin, xmax, ymax);
#	endif /* DEBUG_REFIT */
	for (x=xmin; x<xmax; x+=sgridlen)
		{
		sp[X] = x*xform[X][X] + ymin*xform[Y][X] + xform[H][X];
		sp[Y] = x*xform[X][Y] + ymin*xform[Y][Y] + xform[H][Y];
		if (remap) pos_to_pos(smproj, sp, tmproj, tp);
		else       copy_point(tp, sp);
		if (x==xmin) add_point_to_line(Lbox, tp);
		add_point_to_line(Lcurv, tp);
		}
	for (y=ymin; y<ymax; y+=sgridlen)
		{
		sp[X] = xmax*xform[X][X] + y*xform[Y][X] + xform[H][X];
		sp[Y] = xmax*xform[X][Y] + y*xform[Y][Y] + xform[H][Y];
		if (remap) pos_to_pos(smproj, sp, tmproj, tp);
		else       copy_point(tp, sp);
		if (y==ymin) add_point_to_line(Lbox, tp);
		add_point_to_line(Lcurv, tp);
		}
	for (x=xmax; x>xmin; x-=sgridlen)
		{
		sp[X] = x*xform[X][X] + ymax*xform[Y][X] + xform[H][X];
		sp[Y] = x*xform[X][Y] + ymax*xform[Y][Y] + xform[H][Y];
		if (remap) pos_to_pos(smproj, sp, tmproj, tp);
		else       copy_point(tp, sp);
		if (x==xmax) add_point_to_line(Lbox, tp);
		add_point_to_line(Lcurv, tp);
		}
	for (y=ymax; y>ymin; y-=sgridlen)
		{
		sp[X] = xmin*xform[X][X] + y*xform[Y][X] + xform[H][X];
		sp[Y] = xmin*xform[X][Y] + y*xform[Y][Y] + xform[H][Y];
		if (remap) pos_to_pos(smproj, sp, tmproj, tp);
		else       copy_point(tp, sp);
		if (y==ymax) add_point_to_line(Lbox, tp);
		add_point_to_line(Lcurv, tp);
		}
	sp[X] = xmin*xform[X][X] + ymin*xform[Y][X] + xform[H][X];
	sp[Y] = xmin*xform[X][Y] + ymin*xform[Y][Y] + xform[H][Y];
	if (remap) pos_to_pos(smproj, sp, tmproj, tp);
	else       copy_point(tp, sp);
	add_point_to_line(Lbox, tp);
	add_point_to_line(Lcurv, tp);

#	ifdef DEBUG_REFIT
	(void) printf("[limit_box] Limit box: (%g,%g) (%g,%g) (%g,%g) (%g,%g)\n",
		   Lbox->line->points[0][X], Lbox->line->points[0][Y],
		   Lbox->line->points[1][X], Lbox->line->points[1][Y],
		   Lbox->line->points[2][X], Lbox->line->points[2][Y],
		   Lbox->line->points[3][X], Lbox->line->points[3][Y]);
#	endif /* DEBUG_REFIT */

	if (lbox)  *lbox  = Lbox;
	if (lcurv) *lcurv = Lcurv;
	}

/**********************************************************************/

LOGICAL	reproject_xy_surfaces

	(
	SURFACE			usfc,
	SURFACE			vsfc,
	const MAP_PROJ	*smproj,
	const MAP_PROJ	*tmproj
	)

	{
	int		ix, iy, tnx, tny, m, n, icx, icy;
	float	tgrid, tunits, gunits, gfact;
	float	x, y, lat, lon;
	POINT	tpos;
	float	dt, ds, dd, cdd, sdd;
	float	us, vs, ut, vt;

	if (!usfc)   return FALSE;
	if (!vsfc)   return FALSE;
	if (!smproj) return FALSE;
	if (!tmproj) return FALSE;

	/* Determine target grid parameters. */
	tunits = tmproj->definition.units;
	gunits = tmproj->grid.units;
	if (tunits <= 0) return FALSE;
	if (gunits <= 0) return FALSE;
	gfact  = gunits / tunits;
	tgrid  = tmproj->grid.gridlen*gfact;
	tnx    = tmproj->grid.nx;
	tny    = tmproj->grid.ny;
	if (tgrid <= 0) return FALSE;
	if (tnx <= 0)   return FALSE;
	if (tny <= 0)   return FALSE;

	/* Reproject both surfaces without rotating the components */
	(void) reproject_surface(usfc, smproj, tmproj, NullGridDef);
	(void) reproject_surface(vsfc, smproj, tmproj, NullGridDef);

	if (!compute_2D_rotation(smproj, tmproj)) return FALSE;

	/* Build a new spline for each surface for the rotated components */
	for (iy=0; iy<tny; iy++)
		{
		icy = iy + 1;
		for (ix=0; ix<tnx; ix++)
			{
			icx = ix + 1;

			/* Obtain the unrotated u and v component at this location */
			us = usfc->sp.cvs[icx][icy];
			vs = vsfc->sp.cvs[icx][icy];

			/* Add contribution from the unrotated u and v components */
			ut = us*Coeffs[iy][ix][X][X] + vs*Coeffs[iy][ix][X][Y];
			vt = us*Coeffs[iy][ix][Y][X] + vs*Coeffs[iy][ix][Y][Y];

			/* Replace spline coefficients in situ */
			usfc->sp.cvs[icx][icy] = ut;
			vsfc->sp.cvs[icx][icy] = vt;
			}
		}

	/* Reset the free boundary condition */
	/* Outer ring of spline coefficients matches inner neighbours */
	m = tnx + ORDER - 2;
	n = tny + ORDER - 2;
	for (icx=1; icx<m-1; icx++)
		{
		usfc->sp.cvs[icx][0]   = usfc->sp.cvs[icx][1];
		usfc->sp.cvs[icx][n-1] = usfc->sp.cvs[icx][n-2];
		vsfc->sp.cvs[icx][0]   = vsfc->sp.cvs[icx][1];
		vsfc->sp.cvs[icx][n-1] = vsfc->sp.cvs[icx][n-2];
		}
	for (icy=0; icy<n; icy++)
		{
		usfc->sp.cvs[0][icy]   = usfc->sp.cvs[1][icy];
		usfc->sp.cvs[m-1][icy] = usfc->sp.cvs[m-2][icy];
		vsfc->sp.cvs[0][icy]   = vsfc->sp.cvs[1][icy];
		vsfc->sp.cvs[m-1][icy] = vsfc->sp.cvs[m-2][icy];
		}

	return TRUE;
	}

/**********************************************************************/

static	LOGICAL	compute_2D_rotation

	(
	const MAP_PROJ	*smproj,
	const MAP_PROJ	*tmproj
	)

	{
	int		ix, iy, tnx, tny, m, n, icx, icy;
	float	tgrid, tunits, gunits, gfact;
	float	x, y, lat, lon;
	POINT	tpos;
	float	dt, ds, dd, cdd, sdd;
	float	us, vs, ut, vt;

	/* Return if we already have the appropriate coefficients */
	if (same_map_projection(smproj, &PrevSmp)
		&& same_map_projection(tmproj, &PrevTmp))
		return TRUE;

	tunits = tmproj->definition.units;
	gunits = tmproj->grid.units;
	if (tunits <= 0) return FALSE;
	if (gunits <= 0) return FALSE;
	gfact  = gunits / tunits;
	tgrid  = tmproj->grid.gridlen*gfact;
	tnx    = tmproj->grid.nx;
	tny    = tmproj->grid.ny;

	/* Re-allocate coefficient buffer */
	CoefBuf = GETMEM(CoefBuf, CBUF, tnx*tny);
	Coeffs  = GETMEM(Coeffs, CBUF *, tny);

	/* Re-compute coefficients */
	for (iy=0; iy<tny; iy++)
		{
		Coeffs[iy] = CoefBuf + iy*tnx;
		y = iy*tgrid;
		for (ix=0; ix<tnx; ix++)
			{
			x = ix*tgrid;
			set_point(tpos, x, y);
			pos_to_ll(tmproj, tpos, &lat, &lon);

			/* Calculate the angle between the u vectors in each map */
			/* at this location */
			ds  = wind_dir_true(smproj, lat, lon, 0.0);
			dt  = wind_dir_true(tmproj, lat, lon, 0.0);
			dd  = dt - ds;
			cdd = fpa_cosdeg(dd);
			sdd = fpa_sindeg(dd);

			Coeffs[iy][ix][X][X] = cdd;
			Coeffs[iy][ix][X][Y] = -sdd;

			/* Calculate the angle between the v vectors in each map */
			/* at this location */
			/* >>> Right now, all our projections are either conformal or */
			/*     rectangular, so right angles are preserved and this */
			/*     angle is thus the same as for the u vectors */
			/*
			ds  = wind_dir_true(smproj, lat, lon, 90.0);
			dt  = wind_dir_true(tmproj, lat, lon, 90.0);
			dd  = dt - ds;
			cdd = fpa_cosdeg(dd);
			sdd = fpa_sindeg(dd);
			*/

			Coeffs[iy][ix][Y][X] = sdd;
			Coeffs[iy][ix][Y][Y] = cdd;
			}
		}

	copy_map_projection(&PrevSmp, smproj);
	copy_map_projection(&PrevTmp, tmproj);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*      b u i l d _ s u r f a c e _ 2 D                                 *
*                                                                      *
*      The individual component fields are reprojected and rotated     *
*      onto the given target map, using reproject_xy_surfaces().       *
***********************************************************************/
/*********************************************************************/
/** Reproject the given pair of surfaces, which are understood to
 *  represent the x and y components of the same vector field.
 *  Then combine them into a single 2D vector surface.
 *
 *	@param[in] 	usfc 		U commponent surface
 *	@param[in] 	vsfc 		V commponent surface
 *	@param[in] 	*smproj		Original projection
 *	@param[in] 	*tmproj		Target projection
 *  @return Pointer to surface objetct. You will need to destroy this
 * 			object when you are finished with it.
 *********************************************************************/

SURFACE	build_surface_2D

	(
	SURFACE			usfc,
	SURFACE			vsfc,
	const MAP_PROJ	*smproj,
	const MAP_PROJ	*tmproj
	)

	{
	int		m, n;
	SURFACE	uvsfc;

	if (!reproject_xy_surfaces(usfc, vsfc, smproj, tmproj))
		return NullSfc;

	/* Construct a 2D surface with the given U/V components */
	m = usfc->sp.m;
	n = usfc->sp.n;
	uvsfc = create_surface();
	define_surface_spline_2D(uvsfc, m, n, tmproj,
				usfc->sp.origin, usfc->sp.orient, usfc->sp.gridlen,
				*usfc->sp.cvs, *vsfc->sp.cvs, n);

	/* Set units specs */
	(void) define_surface_units(uvsfc, &usfc->units);

	/* Return pointer to SURFACE Object */
	return uvsfc;
	}

/***********************************************************************
*                                                                      *
*      e v a l u a t i o n _ s u r f a c e                             *
*                                                                      *
*      The given surface is assumed to agree with the map and grid     *
*      definitions given in smproj.  The desired sub-surface is        *
*      described by the map and grid definitions given in emproj.      *
*      This target map projection can be computed with the function    *
*      evaluation_map_projection() in projection_oper.c.               *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Produce a sub-surface of the given surface, suitable for
 * evaluating at a single point (or cluster of points).
 *
 *	@param[in] 	sfc 		surface to make subsurface from
 *	@param[in] 	*smproj 	surface projection
 *	@param[in] 	*emproj		target subsurface projection
 *********************************************************************/
LOGICAL		evaluation_surface

	(
	SURFACE			sfc,
	const MAP_PROJ	*smproj,
	const MAP_PROJ	*emproj
	)

	{
	/*
	PROJ_DEF	*sproj, *eproj;
	MAP_DEF		*smap,  *emap;
	GRID_DEF	*sgrid, *egrid;
	*/
	float		gridlen, dx, dy, orient;
	int			snumx, snumy, enumx, enumy, offx, offy, m, n, i, iu, iv;
	POINT		origin;

	static	float	*cvsbuf = (float *) 0;
	static	float	*cvxbuf = (float *) 0;
	static	float	*cvybuf = (float *) 0;
	static	int		ncvs    = 0;

	if (!sfc)    return FALSE;
	if (!smproj) return FALSE;
	if (!emproj) return FALSE;

	/* Use local pointers to point back into the struct */
	/*
	sproj = &(smproj->projection);
	smap  = &(smproj->definition);
	sgrid = &(smproj->grid);
	eproj = &(emproj->projection);
	emap  = &(emproj->definition);
	egrid = &(emproj->grid);
	*/
#define sproj (smproj->projection)
#define smap  (smproj->definition)
#define sgrid (smproj->grid)
#define eproj (emproj->projection)
#define emap  (emproj->definition)
#define egrid (emproj->grid)

	/* Make sure smproj agrees with the surface spline */
	gridlen = sfc->sp.gridlen * smap.units;
	snumx   = sfc->sp.m - ORDER + 2;
	snumy   = sfc->sp.n - ORDER + 2;
	if (sgrid.gridlen * sgrid.units != gridlen) return FALSE;
	if (sgrid.nx != snumx)                      return FALSE;
	if (sgrid.ny != snumy)                      return FALSE;

	/* Make sure emproj is compatible */
	if (!same_projection(&sproj, &eproj))       return FALSE;
	if (emap.olat != smap.olat)                 return FALSE;
	if (emap.olon != smap.olon)                 return FALSE;
	if (emap.lref != smap.lref)                 return FALSE;
	if (egrid.gridlen * egrid.units != gridlen) return FALSE;

	/* Check that the requested offset aligns with the grid */
	dx   = (smap.xorg*smap.units - emap.xorg*emap.units) / gridlen;
	dy   = (smap.yorg*smap.units - emap.yorg*emap.units) / gridlen;
	offx = NINT(dx);
	offy = NINT(dy);
	if (offx < 0)              return FALSE;
	if (offy < 0)              return FALSE;
	if (fabs((double) (dx-offx)) > 0.001) return FALSE;
	if (fabs((double) (dy-offy)) > 0.001) return FALSE;

	/* Make sure the sub-grid is contained in the original grid */
	enumx = egrid.nx;
	enumy = egrid.ny;
	if (enumx+offx > snumx) return FALSE;
	if (enumy+offy > snumy) return FALSE;

	/* Set up new control vertex array */
	m         = enumx + ORDER - 2;
	n         = enumy + ORDER - 2;
	orient    = sfc->sp.orient;
	origin[X] = sfc->sp.origin[X] * smap.units / emap.units;
	origin[Y] = sfc->sp.origin[Y] * smap.units / emap.units;
	gridlen  /= emap.units;
	if (!cvsbuf || !cvxbuf || !cvybuf || m*n>ncvs)
		{
		ncvs   = m*n;
		cvsbuf = GETMEM(cvsbuf, float, ncvs);
		cvxbuf = GETMEM(cvxbuf, float, ncvs);
		cvybuf = GETMEM(cvybuf, float, ncvs);
		}
	if ( sfc->sp.dim == DimScalar )
		{
		for (i=0, iu=offx; iu<offx+m; iu++)
			for (iv=offy; iv<offy+n; iv++, i++)
				{
				cvsbuf[i] = sfc->sp.cvs[iu][iv];
				}
		define_surface_spline(sfc, m, n, emproj, origin, orient, gridlen,
									cvsbuf, n);
		}
	else if ( sfc->sp.dim == DimVector2D )
		{
		for (i=0, iu=offx; iu<offx+m; iu++)
			for (iv=offy; iv<offy+n; iv++, i++)
				{
				cvxbuf[i] = sfc->sp.cvx[iu][iv];
				cvybuf[i] = sfc->sp.cvy[iu][iv];
				}
		define_surface_spline_2D(sfc, m, n, emproj, origin, orient, gridlen,
									cvxbuf, cvybuf, n);
		}

	return TRUE;
	}

#undef sproj
#undef smap
#undef sgrid
#undef eproj
#undef emap
#undef egrid

/***********************************************************************
*                                                                      *
*      s e t _ g r i d _ f r o m _ s u r f a c e                       *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set a given map projection bases on the given surface.
 *
 *	@param[in] 	sfc		given surface
 *	@param[in] 	*mproj	map projection to set
 *  @return True if successful
 *********************************************************************/
LOGICAL		set_grid_from_surface

	(
	SURFACE		sfc,
	MAP_PROJ	*mproj
	)

	{

	if (!sfc)   return FALSE;
	if (!mproj) return FALSE;

	mproj->grid.nx      = sfc->sp.m - ORDER + 2;
	mproj->grid.ny      = sfc->sp.n - ORDER + 2;
	mproj->grid.gridlen = sfc->sp.gridlen;
	mproj->grid.xgrid   = 0.0;
	mproj->grid.ygrid   = 0.0;
	mproj->grid.units   = mproj->definition.units;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*      m e r g e _ s u r f a c e s                                     *
*                                                                      *
*                                                                      *
*      Usually, data is extracted from only one surface. Component     *
*      fields are a special case only encountered when processing GRIB *
*      files or reprojecting from synoptic data.  For these fields,    *
*      data is extracted from more than one field and summed.          *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Merge surface type fields.
 *
 *  SURFACE type fields are merged by extracting values at each
 *  location on the "background" map projection, with the value
 *  extracted from the SURFACE whose source map projection is
 *  closest to the location.
 *
 *	@param[in] 	numsfc 		number of surfaces to merge
 *	@param[in] 	*sfcin 		surfaces to merge
 *	@param[in] 	*mproj 		map projection for merged surfaces
 *	@param[in] 	numsrc 		number of source projections for merging
 *	@param[in] 	*sproj 		list of source projections for merging
 *	@param[in] 	*nscmp 		number of components for each source projection
 *	@param[in] 	**scmps		list of component surfaces for each source projection
 *  @return Pointer to the resulting field. You will need to destroy
 * 			this object when you are finished with it.
 *********************************************************************/

SURFACE		merge_surfaces

	(
	int				numsfc,
	SURFACE			*sfcin,
	const MAP_PROJ	*mproj,
	int				numsrc,
	const MAP_PROJ	*sproj,
	int				*nscmp,
	int				**scmps
	)

	{
	int			iix, iiy, isrc, icmp, isfc;
	LOGICAL		reset, valid;
	double		valsfc;
	USPEC		*uspec, *uspecin;
	SURFACE		sfc;

	/* FPA grid locations */
	int			Inumx, Inumy;
	POINT		**Apstns;
	float		Aglen, **Alats, **Alons;

	/* Internal buffers for merging SURFACE Objects */
	static	int			NumSproj = 0;
	static	MAP_PROJ	*Sprojs  = NULL;
	static	int			XYmax    = 0,    Ymax    = 0;
	static	float		*Pvals   = NULL, **Vals  = NULL;
	static	int			*Psrcs   = NULL, **Srcs  = NULL;

	/* Return immediately if no surfaces for merging */
	if ( numsfc < 1  || !sfcin ) return NullSfc;
	if ( !mproj ) return NullSfc;
	if ( numsrc > 0  && ( !sproj || !nscmp || !scmps ) ) return NullSfc;

	/* Set FPA grid locations from given map projection */
	if ( !grid_positions(mproj, &Inumx, &Inumy, &Aglen,
			&Apstns, &Alats, &Alons) ) return NullSfc;

	/* Set parameter to reset sources for merged values */
	reset = FALSE;

	/* Allocate space to hold the merged values and sources for merged values */
	if ( Inumx * Inumy > XYmax )
		{
		XYmax = Inumx * Inumy;
		Pvals = GETMEM(Pvals, float, XYmax);
		Psrcs = GETMEM(Psrcs, int,   XYmax);
		reset = TRUE;
		}
	if ( Inumy > Ymax )
		{
		Ymax = Inumy;
		Vals = GETMEM(Vals, float *, Ymax);
		Srcs = GETMEM(Srcs, int *,   Ymax);
		reset = TRUE;
		}

	/* Check if source map projections have changed */
	if ( !reset )
		{
		if ( numsrc != NumSproj )
			{
			reset = TRUE;
			}
		else
			{
			for ( isrc=0; isrc<numsrc; isrc++ )
				{
				if ( !same_map_projection(&sproj[isrc], &Sprojs[isrc]) )
					{
					reset = TRUE;
					break;
					}
				}
			}
		}

	/* Reset the sources for merged values */
	if ( reset )
		{

		/* First save the source map projections */
		FREEMEM(Sprojs);
		NumSproj = numsrc;
		Sprojs   = INITMEM(MAP_PROJ, NumSproj);
		for ( isrc=0; isrc<numsrc; isrc++ )
			{
			(void) copy_map_projection(&Sprojs[isrc], &sproj[isrc]);
			}

		/* Now set the source map projection to use at each output location */
		/*  based on the closest source projection to the location          */
		for ( iiy=0; iiy<Inumy; iiy++ )
			{
			Srcs[iiy] = Psrcs + iiy*Inumx;
			for ( iix=0; iix<Inumx; iix++ )
				{
				Srcs[iiy][iix] = closest_map_projection(Alats[iiy][iix],
						Alons[iiy][iix], numsrc, sproj, NULL, FALSE);
				}
			}
		}

	/* Set unit spec from first surface */
	(void) recall_surface_units(sfcin[0], &uspec);

	/* Determine merged value at each output location */
	/*  based on closest source map projection        */
	for ( iiy=0; iiy<Inumy; iiy++ )
		{
		Vals[iiy] = Pvals + iiy*Inumx;
		for ( iix=0; iix<Inumx; iix++ )
			{

			/* Initialize value */
			Vals[iiy][iix] = 0.0;

			/* Add values from each surface containing source projection */
			for ( icmp=0; icmp<nscmp[Srcs[iiy][iix]]; icmp++ )
				{
				isfc = scmps[Srcs[iiy][iix]][icmp];
				valid = eval_sfc(sfcin[isfc], Apstns[iiy][iix], &valsfc);
				(void) recall_surface_units(sfcin[isfc], &uspecin);
				Vals[iiy][iix] += (float) convert_by_uspec(uspec, uspecin,
															valsfc);
				}
			}
		}

	/* Move grid of merged values to SURFACE Object */
	sfc = create_surface();
	(void) grid_surface(sfc, Aglen, Inumx, Inumy, Vals);

	/* Set units specs */
	(void) define_surface_units(sfc, uspec);

	/* Return pointer to SURFACE Object */
	return sfc;
	}
