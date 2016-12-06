/***********************************************************************
*                                                                      *
*     g x _ t r a n s . c                                              *
*                                                                      *
*     Useful extensions to the FpaXgl library.                         *
*                                                                      *
*     (c) Copyright 1996-2008 Environment Canada (EC)                  *
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

#include   "gx.h"

#undef DEBUG_DN

/***********************************************************************
*                                                                      *
*    g x S e t M p r o j                                               *
*    g x G e t M p r o j                                               *
*    g x S e t Z o o m M p r o j                                       *
*    g x G e t Z o o m M p r o j                                       *
*    g x S e t P a t c h I n f o                                       *
*    g x G e t P a t c h I n f o                                       *
*    g x S e t W i n d P a r m s                                       *
*    g x G e t W i n d P a r m s                                       *
*    g x G e t M f a c t                                               *
*    g x G e t P i x e l S i z e                                       *
*                                                                      *
***********************************************************************/

static	float		Mylen  = 1.0;
static	MAP_PROJ	Mproj  = NO_MAPPROJ;
static	MAP_PROJ	Zproj  = NO_MAPPROJ;
static	SURFACE		PSfc   = NullSfc;
static	int			Iup    = 0;
static	int			Ivp    = 0;
static	float		WndMin = 0.5;
static	float		WndMax = 200;
static	STRING		Wunits = "knots";
static	STRING		Dunits = "knots";
static	FpaConfigUnitStruct	*WndUdef = NullPtr(FpaConfigUnitStruct *);
static	FpaConfigUnitStruct	*DefUdef = NullPtr(FpaConfigUnitStruct *);

void	gxSetMproj(MAP_PROJ *mproj)
	{
	if (NotNull(mproj))
		{
		define_map_projection(&Mproj, &mproj->projection, &mproj->definition,
								&mproj->grid);
		Mylen = Mproj.definition.ylen;

#		ifdef DEBUG_DN
		(void) printf("[gxSetMproj] Projection type: %s  %.2f  %.2f  %.2f\n",
				which_projection_name(Mproj.projection.type),
				Mproj.projection.ref[0], Mproj.projection.ref[1],
				Mproj.projection.ref[2]);
		(void) printf("[gxSetMproj] Basemap olat/olon: %.2f/%.2f  lref: %.2f\n",
				Mproj.definition.olat, Mproj.definition.olon,
				Mproj.definition.lref);
		(void) printf("[gxSetMproj]         xorg/yorg: %.0f/%.0f  xlen/ylen: %.0f/%.0f  units: %.0f\n",
				Mproj.definition.xorg, Mproj.definition.yorg,
				Mproj.definition.xlen, Mproj.definition.ylen,
				Mproj.definition.units);
		(void) printf("[gxSetMproj] Map origin: %.0f/%.0f  clat/clon: %.2f/%.2f\n",
				Mproj.origin[X], Mproj.origin[Y],
				Mproj.clat, Mproj.clon);
#		endif /* DEBUG_DN */
		}
	else
		{
		define_map_projection(&Mproj, &NoProjDef, &NoMapDef, &NoGridDef);
		Mylen = 1.0;

#		ifdef DEBUG_DN
		(void) printf("[gxSetMproj] Set Null MapProj\n");
#		endif /* DEBUG_DN */
		}
	}

MAP_PROJ	*gxGetMproj(void)	{ return &Mproj; }

void	gxSetZoomMproj(MAP_PROJ *mproj)
	{
	if (NotNull(mproj))
		{
		define_map_projection(&Zproj, &mproj->projection, &mproj->definition,
								&mproj->grid);

#		ifdef DEBUG_DN
		(void) printf("[gxSetZoomMproj] Projection type: %s  %.2f  %.2f  %.2f\n",
				which_projection_name(Zproj.projection.type),
				Zproj.projection.ref[0], Zproj.projection.ref[1],
				Zproj.projection.ref[2]);
		(void) printf("[gxSetZoomMproj] Basemap olat/olon: %.2f/%.2f  lref: %.2f\n",
				Zproj.definition.olat, Zproj.definition.olon,
				Zproj.definition.lref);
		(void) printf("[gxSetZoomMproj]         xorg/yorg: %.0f/%.0f  xlen/ylen: %.0f/%.0f  units: %.0f\n",
				Zproj.definition.xorg, Zproj.definition.yorg,
				Zproj.definition.xlen, Zproj.definition.ylen,
				Zproj.definition.units);
		(void) printf("[gxSetZoomMproj] Map origin: %.0f/%.0f  clat/clon: %.2f/%.2f\n",
				Zproj.origin[X], Zproj.origin[Y],
				Zproj.clat, Zproj.clon);
#		endif /* DEBUG_DN */
		}
	else
		{
		define_map_projection(&Zproj, &NoProjDef, &NoMapDef, &NoGridDef);

#		ifdef DEBUG_DN
		(void) printf("[gxSetZoomMproj] Set Null MapProj\n");
#		endif /* DEBUG_DN */
		}
	}

MAP_PROJ	*gxGetZoomMproj(void)		{ return &Zproj; }

void	gxSetPatchInfo(SURFACE sfc, int iu, int iv)
	{
	PSfc = sfc;
	Iup  = iu;
	Ivp  = iv;
	}

SURFACE	gxGetPatchInfo(int *iu, int *iv)
	{
	if (iu) *iu = Iup;
	if (iv) *iv = Ivp;
	return PSfc;
	}

void	gxSetWindParms(float wmin, float wmax, STRING wunits, STRING dunits)
	{
	if (wmin >= 0) WndMin = wmin;
	if (wmax >= 0) WndMax = wmax;
	if (!blank(wunits)) WndUdef = identify_unit(wunits);
	if (!blank(dunits)) DefUdef = identify_unit(dunits);
	}

void	gxGetWindParms(float *wmin, float *wmax, FpaConfigUnitStruct **wudef,
													FpaConfigUnitStruct **dudef)
	{
	if (IsNull(WndUdef)) WndUdef = identify_unit(Wunits);
	if (IsNull(DefUdef)) DefUdef = identify_unit(Dunits);
	if (wmin)  *wmin  = WndMin;
	if (wmax)  *wmax  = WndMax;
	if (wudef) *wudef = WndUdef;
	if (dudef) *dudef = DefUdef;
	}

float	gxGetMfact(void)
	{
	float	xp, yp;

	glMapUnitPerVdcUnit(&xp, &yp);
	return yp;
	}

float	gxGetPixelSize(void)
	{
	float	xp, yp;

	glGetMapPixelSize(&xp, &yp);
	return yp;
	}

/***********************************************************************
*                                                                      *
*    g x S c a l e S i z e                                             *
*                                                                      *
***********************************************************************/

float	gxScaleSize(float size, LOGICAL scale)
	{
	if (scale) return (size * 1000 / gxGetMfact());
	else       return size;
	}


/***********************************************************************
*                                                                      *
*      g x S e t u p T r a n s f o r m                                 *
*                                                                      *
*      Recursively apply the transform of the current dispnode to      *
*      the transforms of its ancestors, right back to the head of      *
*      the display tree.                                               *
*                                                                      *
*      p u s h _ t r a n s f o r m                                     *
*                                                                      *
*      Apply the transform of the current dispnode and push the        *
*      resulting transform onto the stack.                             *
*                                                                      *
*      p o p _ t r a n s f o r m                                       *
*                                                                      *
*      Undo the latest transform and pop it from the stack.            *
*                                                                      *
***********************************************************************/

void		gxSetupTransform

	(
	DISPNODE	dn		/* specified display node */
	)

	{
	/* If null dispnode - initialize matrix stack */
	if (!dn)
		{
		glOrtho(0.0, 1.0, 0.0, 1.0);
		gxSetMproj(NullMapProj);
		return;
		}

	/* Set up transform of parent */
	gxSetupTransform(dn->parent);

	/* Apply the current transform */
	gxPushTransform(dn);
	}

/**********************************************************************/

void		gxPushTransform

	(
	DISPNODE	dn		/* specified display node */
	)

	{
	float	wl, wr, wb, wt;

	/* Go home if null dispnode */
	if (!dn) return;

	/* Extract window limits */
	wl = dn->window.left;
	wr = dn->window.right;
	wb = dn->window.bottom;
	wt = dn->window.top;

	/* Set up current transformation for this node and its sub-tree */
	if (dn->dxtype == DxRoot)
		{
		glOrtho(wl,wr,wb,wt);
		glSetMapClipRectangle(wl,wr,wb,wt);
		}
	else if (dn->dxtype != DxIdent)
		{
		glConcatMatrix(dn->xform);
		glSetMapClipRectangle(wl,wr,wb,wt);
		}

#	ifdef DEBUG_DN
	(void) printf("[gxPushTransform] Dispnode: %x\n", dn);
#	endif /* DEBUG_DN */

	/* Remember current map projection for graphics primitives (e.g. barb) */
	gxSetMproj(&dn->mproj);

	/* Disable clipping for now */
	glClipMode(glCLIP_OFF);
	}

/**********************************************************************/

void		gxPopTransform

	(
	DISPNODE	dn		/* specified display node */
	)

	{
	/* Go home if null dispnode */
	if (!dn) return;

	/* Pop the transform */
	if (dn->dxtype != DxIdent && dn->dxtype != DxRoot)
		{
		glPopMatrix();
		}

	/* Remember current map projection for graphics primitives (e.g. barb) */
	if (dn->parent)
		{
#		ifdef DEBUG_DN
		(void) printf("[gxPopTransform] Dispnode: %x  Parent: %x\n",
				dn, dn->parent);
#		endif /* DEBUG_DN */

		gxSetMproj(&dn->parent->mproj);
		}
	else
		{
#		ifdef DEBUG_DN
		(void) printf("[gxPopTransform] Dispnode: %x\n", dn);
#		endif /* DEBUG_DN */

		gxSetMproj(NullMapProj);
		}
	}
