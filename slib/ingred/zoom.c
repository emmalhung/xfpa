/***********************************************************************
*                                                                      *
*     z o o m . c                                                      *
*                                                                      *
*     This module of the INteractive GRaphics EDitor (INGRED)          *
*     handles all control of the zoom and pan features.                *
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

#include <time.h>

#undef DEBUG_ZOOM

static	BOX		UnZoom   = {0., 100., 0., 100.};
static	BOX		Zoom     = {0., 100., 0., 100.};
static	LOGICAL	Zoomed   = FALSE;
static	BOX		PrevZoom = {0., 100., 0., 100.};
static	LOGICAL	ZoomSusp = FALSE;
static	float	ZoomFact =  1.0;
static	float	ZoomMin  = 25.0;

static	void	calc_zoom_box(BOX *, LOGICAL, POINT, POINT);
static	void	recalc_zoom_box(BOX *, LOGICAL);

/***********************************************************************
*                                                                      *
*     z o o m _ i n                                                    *
*     z o o m _ o u t                                                  *
*     z o o m _ p a n                                                  *
*     z o o m _ r e s e t                                              *
*                                                                      *
***********************************************************************/

LOGICAL	zoom_in

	(
	STRING	mode
	)

	{
	POINT	pos, opos;
	int		butt;
	float	dx, dy;
	BOX		zbox;

#	ifdef DEBUG
	pr_diag("Editor", "[zoom_in] %s\n", mode);
#	endif /* DEBUG */

	/* Clean up previous edits */
	if (same_start(mode, "cancel"))
		{
		return FALSE;
		}
	if (same(mode, "confirm"))
		{
		return FALSE;
		}
	if (same(mode, "undo"))
		{
		return FALSE;
		}

	/* Repeat until told to quit */
	while (TRUE)
		{

		/* Pick one corner of zoom box */
		put_message("zoom-pick");
		if (!ready_Xpoint(DnEdit, opos, &butt)) return FALSE;
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

		/* Now define the box */
		put_message("zoom-drag");

		/* Set second parameter to constrain (3) or not constrain (2) */
		/*  zoom box to fit window dimensions                         */
		(void) pick_Xpoint(DnEdit, 2, pos, &butt);
		break;
		}

	/* Notify the interface */
	zoom_start();

	/* Clip end zoom position to outline of edit area */
	pos[X] = MAX(pos[X], DnEdit->window.left);
	pos[X] = MIN(pos[X], DnEdit->window.right);
	pos[Y] = MAX(pos[Y], DnEdit->window.bottom);
	pos[Y] = MIN(pos[Y], DnEdit->window.top);

	/* Limit zoom to minimum size of zoom box */
	dx = pos[X] - opos[X];
	dy = pos[Y] - opos[Y];
	if (fabs(dx) < ZoomMin || fabs(dy) < ZoomMin)
		{
		put_message("zoom-too-small");
		(void) sleep(1);

		/* Adjust zoom box to minimum size */
		pos[X]  -= dx/2.0;			opos[X]  = pos[X];
		pos[Y]  -= dy/2.0;			opos[Y]  = pos[Y];
		pos[X]  += ZoomMin/2.0;		opos[X] -= ZoomMin/2.0;
		pos[Y]  += ZoomMin/2.0;		opos[Y] -= ZoomMin/2.0;
		}

	/* Now do the zoom */
	put_message("zoom-zooming");
	calc_zoom_box(&zbox, Zoomed, opos, pos);

	/* Note: Wait for resize to redraw the first time */
	return define_zoom(&zbox, Zoomed, TRUE);
	}

/**********************************************************************/

LOGICAL	zoom_out(void)

	{
	if (!Zoomed) return FALSE;

	put_message("zoom-unzooming");
	(void) sleep(1);

	/* Note: Wait for resize to redraw */
	(void) define_zoom(&UnZoom, FALSE, FALSE);
	Zoomed = FALSE;
	return TRUE;
	}

/**********************************************************************/

LOGICAL	zoom_pan

	(
	STRING	mode
	)

	{
	POINT	pos, opos;
	int		butt;
	float	xfact, yfact, w, h, dx, dy;
	BOX		box;
	BOX		zbox;

	static	SET		bset = NULL;
	static	CURVE	bord = NULL;

#	ifdef DEBUG
	pr_diag("Editor", "[zoom_pan] %s\n", mode);
#	endif /* DEBUG */

	if (!Zoomed)
		{
		zoom_pan_end();
		return FALSE;
		}

	/* Clean up previous edits */
	if (same_start(mode, "cancel"))
		{
		return FALSE;
		}
	if (same(mode, "confirm"))
		{
		return FALSE;
		}
	if (same(mode, "undo"))
		{
		return FALSE;
		}

	/* Handle Right button "pan_done" */
	if (same(mode, "pan_done"))
		{
		zoom_pan_end();
		return TRUE;
		}

	if (!bset)
		{
		bset = create_set("curve");
		bord = create_curve("", "", "");
		define_lspec(&bord->lspec, 255, 0, NULL, False, 7.0, 0.0, (HILITE) 2);
		add_item_to_set(bset, (ITEM) bord);
		}

	/* Repeat until told to quit */
	xfact = 100.0 / MapProj->definition.xlen;
	yfact = 100.0 / MapProj->definition.ylen;
	dx = MapProj->definition.xlen * 0.005 * (Zoom.right - Zoom.left);
	dy = MapProj->definition.ylen * 0.005 * (Zoom.top - Zoom.bottom);
	while (TRUE)
		{
		/* Pick new centre of zoom box */
		put_message("zoom-pan");
		if (!ready_Xpoint(DnEdit, opos, &butt)) return FALSE;
		if (butt != LeftButton)
			{
			put_message("edit-wrong-button");
			(void) ignore_Xpoint();
			continue;
			}

		box.left   = opos[X] - dx;
		box.right  = opos[X] + dx;
		box.bottom = opos[Y] - dy;
		box.top    = opos[Y] + dy;
		empty_curve(bord);
		add_point_to_curve(bord, make_point(box.left, box.bottom));
		add_point_to_curve(bord, make_point(box.left, box.top));
		add_point_to_curve(bord, make_point(box.right, box.top));
		add_point_to_curve(bord, make_point(box.right, box.bottom));
		add_point_to_curve(bord, make_point(box.left, box.bottom));
		put_message("zoom-pan-rel");
		(void) utrack_Xpoint(DnEdit, bset, NullPoint, pos, &butt);
		if (!inside_dn_window(DnEdit,pos))
			{
			put_message("edit-outside-map");
			(void) sleep(1);
			continue;
			}

		/* Now re-define the zoom */
		put_message("zoom-redef");
		w = Zoom.right - Zoom.left;
		h = Zoom.top - Zoom.bottom;
		zbox.left   = xfact*pos[X] - 0.5*w;
		zbox.right  = zbox.left + w;
		zbox.bottom = yfact*pos[Y] - 0.5*h;
		zbox.top    = zbox.bottom + h;
		(void) define_zoom(&zbox, TRUE, TRUE);
		}
	}

/**********************************************************************/

LOGICAL	zoom_reset

	(
	STRING	xstr,
	STRING	ystr,
	STRING	wstr,
	STRING	hstr
	)

	{
	float	x, y, w, h;
	double	strtod();
	STRING	p;
	BOX		zbox;

	if (!Zoomed) return FALSE;

	put_message("zoom-redef");

	w = (float) strtod(wstr, &p);
	w = (p!=wstr)? w: Zoom.right - Zoom.left;
	h = (float) strtod(hstr, &p);
	h = (p!=hstr)? h: Zoom.top - Zoom.bottom;

	x = (float) strtod(xstr, &p);
	x = (p!=xstr)? x: Zoom.left;
	y = 100 - (float) strtod(ystr, &p) - h;
	y = (p!=ystr)? y: Zoom.bottom;

	copy_box(&zbox, &Zoom);
	zbox.left   = x;
	zbox.right  = x + w;
	zbox.bottom = y;
	zbox.top    = y + h;

	recalc_zoom_box(&zbox, TRUE);

	return define_zoom(&zbox, TRUE, TRUE);
	}

/***********************************************************************
*                                                                      *
*     s u s p e n d _ z o o m                                          *
*     r e s u m e _ z o o m                                            *
*                                                                      *
***********************************************************************/

LOGICAL	suspend_zoom(void)

	{
	if (!Zoomed)  return FALSE;
	if (ZoomSusp) return TRUE;

	copy_box(&PrevZoom, &Zoom);
	(void) define_zoom(&UnZoom, FALSE, FALSE);
	ZoomSusp = TRUE;
	return TRUE;
	}

/**********************************************************************/

LOGICAL	resume_zoom
	(
	LOGICAL	resize
	)

	{
	if (!ZoomSusp) return TRUE;

	if (resize) recalc_zoom_box(&PrevZoom, Zoomed);

	(void) define_zoom(&PrevZoom, FALSE, FALSE);
	ZoomSusp = FALSE;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     d e f i n e _ z o o m                                            *
*                                                                      *
***********************************************************************/

LOGICAL	define_zoom

	(
	BOX		*box,
	LOGICAL	redraw,
	LOGICAL	report
	)

	{
	MAP_DEF		zmap;
	BOX			zwindow;
	float		x, y, width, height, xlen, ylen;

	if (IsNull(box)) return FALSE;

	/* Force the zoom box to not extend past the map edge */
	width = fabs(box->right-box->left);	height = fabs(box->top-box->bottom);
	width = MAX(width, 0.01);			height = MAX(height, 0.01);
	width = MIN(width, 100.0);			height = MIN(height, 100.0);
	x     = MIN(box->left, box->right);	y      = MIN(box->bottom, box->top);
	x     = MAX(x, 0.0);				y      = MAX(y, 0.0);
	x     = MIN(x, (100-width));		y      = MIN(y, (100-height));

	/* Set the current zoom parameters */
	ZoomSusp    = FALSE;
	Zoomed      = (LOGICAL) (width<100 || height<100);
	Zoom.left   = x;
	Zoom.right  = x + width;
	Zoom.bottom = y;
	Zoom.top    = y + height;

	/* Define the equivalent portion of the map window */
	xlen = MapProj->definition.xlen * .01;
	ylen = MapProj->definition.ylen * .01;
	zwindow.left   = xlen * Zoom.left;
	zwindow.bottom = ylen * Zoom.bottom;
	zwindow.right  = xlen * Zoom.right;
	zwindow.top    = ylen * Zoom.top;

	/* Define equivalent map definition with adjusted origin and dimensions */
	copy_map_def(&zmap, &MapProj->definition);
	zmap.xorg -= zwindow.left;
	zmap.yorg -= zwindow.bottom;
	zmap.xlen  = zwindow.right - zwindow.left;
	zmap.ylen  = zwindow.top   - zwindow.bottom;

	/* Re-define the projection with the new map definition */
	define_map_projection(&DnMap->mproj,
			&DnMap->mproj.projection, &zmap, &DnMap->mproj.grid);

#	ifdef DEBUG_ZOOM
	(void) printf("[define_zoom] Begin re-define window/transform of DnMap: %x  at: %d\n",
			DnMap, (long) clock());
#	endif /* DEBUG_ZOOM */

	/* Re-define the window and transform of DnMap */
	copy_box(&DnMap->window, &zwindow);
	block_xform(DnMap->xform, &DnMap->viewport, &DnMap->window);
	zoom_dn_subtree(DnMap, &zmap, &zwindow);

	/* Re-define the zoom projection (for cursor tracking and wind barb display) */
	gxSetZoomMproj(&DnMap->mproj);

#	ifdef DEBUG_ZOOM
	(void) printf("[define_zoom] End re-define window/transform of DnMap: %x  at: %d\n",
			DnMap, (long) clock());
#	endif /* DEBUG_ZOOM */

	if (redraw)
		{

#		ifdef DEBUG_ZOOM
		(void) printf("[define_zoom] Begin re-capture of map background: %x  at: %d\n",
				DnBgnd, (long) clock());
#		endif /* DEBUG_ZOOM */

		/* Re-capture the map background */
		if (!Spawned) capture_dn_raster(DnBgnd);

#		ifdef DEBUG_ZOOM
		(void) printf("[define_zoom] End re-capture of map background: %x  at: %d\n",
				DnBgnd, (long) clock());
#		endif /* DEBUG_ZOOM */

		/* Update the display */
		present_all();
		}

	/* Notify the interface */
	if (report && Zoomed) zoom_area(&Zoom);

	ZoomFact = 1;
	if (Zoomed)
		ZoomFact = (width*xlen + height*ylen) / (100*xlen + 100*ylen);
	(void) define_edit_resolution();

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     z o o m _ d n _ s u b t r e e                                    *
*                                                                      *
***********************************************************************/
void		zoom_dn_subtree

	(
	DISPNODE	dn,
	MAP_DEF		*zmap,
	BOX			*zwindow
	)

	{
	int			i;
	DISPNODE	kdn;

	if (IsNull(dn)) return;

	for (i=0; i<dn->numkids; i++)
		{
		kdn = dn->kids[i];

#		ifdef DEBUG_ZOOM
		(void) printf("[zoom_dn_subtree] Child: %d %x  (type: %d)  of: %x\n",
				i, kdn, kdn->dxtype, dn);
#		endif /* DEBUG_ZOOM */

		if (kdn->dxtype == DxMap)
			{
			copy_map_def(&kdn->mproj.definition, zmap);
			copy_box(&kdn->viewport, zwindow);
			copy_box(&kdn->window, zwindow);
			copy_xform(kdn->xform, IdentXform);

#			ifdef DEBUG_ZOOM
			(void) printf("[zoom_dn_subtree] Reset map for child: %x  of: %x\n",
					kdn, dn);
#			endif /* DEBUG_ZOOM */
			}

		else if (kdn->dxtype == DxWindow)
			{
			copy_box(&kdn->viewport, zwindow);
			copy_box(&kdn->window, zwindow);
			copy_xform(kdn->xform, IdentXform);

#			ifdef DEBUG_ZOOM
			(void) printf("[zoom_dn_subtree] Reset window for child: %x  of: %x\n",
					kdn, dn);
#			endif /* DEBUG_ZOOM */
			}

		zoom_dn_subtree(kdn, zmap, zwindow);
		}
	}

/***********************************************************************
*                                                                      *
*     d e f i n e _ e d i t _ r e s o l u t i o n                      *
*                                                                      *
***********************************************************************/

LOGICAL	define_edit_resolution(void)

	{
	float	s;

	s = MapProj->definition.xlen + MapProj->definition.ylen;
	if (Zoomed) s *= ZoomFact;

	FilterRes = s/100;
	SplineRes = s/200;
	PickTol   = s/120;

#	ifdef DEVELOPMENT
	(void) pr_diag("Editor",
			"  FilterRes %g   SplineRes: %g   PickTol: %g\n",
			FilterRes, SplineRes, PickTol);
#	endif

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     z o o m _ f a c t o r                                            *
*                                                                      *
***********************************************************************/

float	zoom_factor(void)

	{
	return ZoomFact;
	}

/***********************************************************************
*                                                                      *
*     c a l c _ z o o m _ b o x                                        *
*     r e c a l c _ z o o m _ b o x                                    *
*                                                                      *
***********************************************************************/

static	void	calc_zoom_box
	(
	BOX		*zbox,
	LOGICAL	scroll,
	POINT	p,
	POINT	q
	)

	{
	float	xfact, yfact;
	int		nx, ny;
	float	wx, wy;	/* size of drawing window (pixels) */
	float	mx, my;	/* size of zoom box (map units) */
	float	rx, ry;	/* scale factors */
	float	dx, dy;	/* change in zoom box */
	BOX		mbox;	/* adjusted map box */

	/* Figure out the zoom box as drawn */
	mbox.left   = MIN(p[X], q[X]);
	mbox.right  = MAX(p[X], q[X]);
	mbox.bottom = MIN(p[Y], q[Y]);
	mbox.top    = MAX(p[Y], q[Y]);

	/* Obtain size of drawing area (Account for scroll bars if not yet there) */
	glGetWindowSize(&nx, &ny);
	if (!scroll)
		{
		nx -= 17;
		ny -= 17;
		}

	/* Force the zoom box to match the display area aspect ratio */
	wx = (float)nx;						wy = (float)ny;
	mx = fabs(mbox.right-mbox.left);	my = fabs(mbox.top-mbox.bottom);
	rx = mx / wx;						ry = my / wy;
	if (rx < ry)
		{
		dx = wx * (ry - rx);
		mbox.right += dx/2;
		mbox.left  -= dx/2;
		}
	else if (ry < rx)
		{
		dy = wy * (rx - ry);
		mbox.bottom -= dy/2;
		mbox.top    += dy/2;
		}

	/* Scale the zoom box as percentage of the map definition */
	xfact = 100.0 / MapProj->definition.xlen;
	yfact = 100.0 / MapProj->definition.ylen;
	zbox->left   = xfact * mbox.left;
	zbox->right  = xfact * mbox.right;
	zbox->bottom = yfact * mbox.bottom;
	zbox->top    = yfact * mbox.top;

#	ifdef DEBUG_ZOOM
	if (scroll)
		(void) printf("[calc_zoom_box] Zoom box - x: %.5f to %.5f  y: %.5f to %.5f\n",
			zbox->left, zbox->right, zbox->bottom, zbox->top);
	else
		(void) printf("[calc_zoom_box] Zoom box (no scroll) - x: %.5f to %.5f  y: %.5f to %.5f\n",
			zbox->left, zbox->right, zbox->bottom, zbox->top);
#	endif /* DEBUG_ZOOM */
	}

/**********************************************************************/

static	void	recalc_zoom_box
	(
	BOX		*zbox,
	LOGICAL	scroll
	)

	{
	POINT	p, q;
	float	xfact, yfact;

	xfact = MapProj->definition.xlen / 100.0;
	yfact = MapProj->definition.ylen / 100.0;

	p[X] = xfact * zbox->left;		p[Y] = yfact * zbox->bottom;
	q[X] = xfact * zbox->right;		q[Y] = yfact * zbox->top;

	calc_zoom_box(zbox, scroll, p, q);
	}
