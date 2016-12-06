/**********************************************************************
*                                                                      *
*      d i s p l a y . c                                               *
*                                                                      *
*      Routines to handle graphical output of assorted displayable     *
*      objects.                                                        *
*                                                                      *
*     (c) Copyright 1996 Environment Canada (AES)                      *
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

#include "display.h"
#include "pattern.h"
#include "gx.h"

#include <string.h>
#include <stdio.h>

#undef DEBUG_DN
#undef DEBUG_DISPLAY
#undef DEBUG_BARB

/***********************************************************************
*                                                                      *
*      d i s p l a y _ d i s p n o d e                                 *
*                                                                      *
*      Display the given dispnode and its sub-tree.                    *
*                                                                      *
***********************************************************************/

void	display_dispnode

	(
	DISPNODE	dn		/* specified display node */
	)

	{
	/* Go home if null dispnode */
	if (!dn) return;

	/* Set up local transformation from parent */
	gxSetupTransform(dn->parent);

	/* Now we can display the subtree */
	display_dn_subtree(dn,TRUE);
	}

/***********************************************************************
*                                                                      *
*      c a p t u r e _ d n _ r a s t e r                               *
*      f r e e _ d n _ r a s t e r                                     *
*                                                                      *
*      Save/free the raster image of the given dispnode.               *
*                                                                      *
***********************************************************************/

void	capture_dn_raster

	(
	DISPNODE	dn		/* specified display node */
	)

	{
	/* Go home if null dispnode */
	if (!dn) return;

	/* Get rid of the old raster if there */
	free_dn_raster(dn);

	/* Display all the ancestors */
	display_dn_parent(dn);

	/* Now display the dispnode contents */
	gxSetupTransform(dn->parent);
	display_dn_subtree(dn,TRUE);

	/* Now capture */
	dn->snap = glGetSnapshot(glSERVER_SIDE_PREFERENCE);
	}

void	free_dn_raster

	(
	DISPNODE	dn		/* specified display node */
	)

	{
	/* Go home if null dispnode */
	if (!dn) return;

	/* Get rid of the old raster if there */
	if (dn->snap > 0)
		{
		glClearSnapshot(dn->snap);
		dn->snap = 0;
		}
	}

/***********************************************************************
*                                                                      *
*      d i s p l a y _ d n _ p a r e n t                               *
*                                                                      *
*      Recursively display the ancestors of the given display sub-tree.*
*                                                                      *
***********************************************************************/

void	display_dn_parent

	(
	DISPNODE	dn		/* specified display node */
	)

	{
	DISPNODE	pn;

	/* Go home if null dispnode */
	if (!dn) return;
	pn = dn->parent;
	if (!pn) return;

	/* First display its ancestors */
	display_dn_parent(pn);

	/* Then display its contents */
	gxSetupTransform(pn->parent);
	display_dn_subtree(pn, FALSE);
	}

/***********************************************************************
*                                                                      *
*      d i s p l a y _ d n _ s u b t r e e                             *
*                                                                      *
*      Recursively display the given display sub-tree.                 *
*                                                                      *
***********************************************************************/

void	display_dn_subtree

	(
	DISPNODE	dn,		/* specified display node */
	LOGICAL		descend	/* do we descend the subtree? */
	)

	{
	int		i, filled, edged;
	float	vl, vr, vb, vt;
	float	wl, wr, wb, wt;

	/* Go home if null dispnode */
	if (!dn)        return;
	if (!dn->shown) return;

#	ifdef DEBUG_DN
	(void) printf("[display_dn_subtree] Begin [%x] at: %d\n", dn, (long) clock());
#	endif /* DEBUG_DN */

	/* Extract viewport and window boundaries */
	vl = dn->viewport.left;
	vr = dn->viewport.right;
	vb = dn->viewport.bottom;
	vt = dn->viewport.top;
	wl = dn->window.left;
	wr = dn->window.right;
	wb = dn->window.bottom;
	wt = dn->window.top;

	/* Display a raster if defined */
	if (dn->snap > 0)
		{
	    glPutSnapshot(dn->snap);
		}

	/* Otherwise draw everything */
	else
		{
		/* Fill viewport background if so specified */
		filled = (dn->vfill >= 0);
		if ( filled )
			{
			glSetColorIndex(dn->vfill);
			glFillStyle(glPATTERN_SOLID);
			glFilledRectangle(vl,vb,vr,vt);
			}

		/* Set up current transformation for this node and its sub-tree */
		gxPushTransform(dn);

		/* Display window background if so specified */
		filled = (dn->wfill >= 0);
		if ( filled )
			{
			glSetColorIndex(dn->wfill);
			glFillStyle(glPATTERN_SOLID);
			glFilledRectangle(wl,wb,wr,wt);
			}

		/* Display any displayable objects in this node */
		glClipMode(glCLIP_ON);
		switch(dn->dntype)
			{
			case DnMeta:	display_metafile(dn->data.meta);
							break;
			case DnSfc:		display_surface(dn->data.sfc);
							break;
			case DnSet:		display_set(dn->data.set);
							break;
			case DnPlot:	display_plot(dn->data.plot);
							break;
			case DnSpecial:	display_special(dn->sptype, dn->data.ptr);
							break;
			}
		glFlush();
		glClipMode(glCLIP_OFF);

		/* Recursively display the rest of the sub-tree */
		if (descend)
			{

#			ifdef DEBUG_DN
			(void) printf("[display_dn_subtree] [%x] recursive display (%d) at: %d\n",
				dn, dn->numkids, (long) clock());
			for (i=0; i < dn->numkids; i++)
				{
				if (dn->kids[i] && dn->kids[i]->shown)

					(void) printf("[display_dn_subtree]   Child (%d) [%x]  snap: %d\n",
						i, dn->kids[i], dn->kids[i]->snap);
				else
					(void) printf("[display_dn_subtree]   Child (%d) [%x] Not Shown\n",
						i, dn->kids[i]);

				}
#			endif /* DEBUG_DN */

			for (i=0; i < dn->numkids; i++)
				display_dn_subtree(dn->kids[i],TRUE);
			}

		/* Edge window boundary if so specified */
		edged  = (dn->wedge >= 0);
		if ( edged )
			{
			glSetColorIndex(dn->wedge);
			glLineStyle(glSOLID);
			glLineWidth(0);
			glRectangle(wl,wb,wr,wt);
			}

		/* Undo the current transformation */
		gxPopTransform(dn);

		/* Edge viewport boundary if so specified */
		edged  = (dn->vedge >= 0);
		if ( edged )
			{
			glSetColorIndex(dn->vedge);
			glLineStyle(glSOLID);
			glLineWidth(0);
			glRectangle(vl,vb,vr,vt);
			}
		}

#	ifdef DEBUG_DN
	(void) printf("[display_dn_subtree] [%x] Before glFlush at: %d\n",
		dn, (long) clock());
#	endif /* DEBUG_DN */

	/* Let's see the results now */
	glFlush();

#	ifdef DEBUG_DN
	(void) printf("[display_dn_subtree] End [%x] at: %d\n", dn, (long) clock());
#	endif /* DEBUG_DN */
	}

/***********************************************************************
*                                                                      *
*      d i s p l a y _ d n _ e d g e                                   *
*                                                                      *
*      Re-edge the window of the given dispnode.                       *
*                                                                      *
***********************************************************************/

void	display_dn_edge

	(
	DISPNODE	dn		/* specified display node */
	)

	{
	float	wl, wr, wb, wt;

	/* Go home if no dispnode given */
	if (!dn) return;
	if (dn->wedge < 0) return;

	/* Re-edge the window */
	wl = dn->window.left;
	wr = dn->window.right;
	wb = dn->window.bottom;
	wt = dn->window.top;
	glSetColorIndex(dn->wedge);
	glLineStyle(glSOLID);
	glLineWidth(0);
	glRectangle(wl,wb,wr,wt);
	}

/***********************************************************************
*                                                                      *
*      d i s p l a y _ m e t a f i l e                                 *
*                                                                      *
*      Display the given metafile.                                     *
*                                                                      *
***********************************************************************/

void	display_metafile

	(
	METAFILE	meta
	)

	{
	int		i;

	/* Go home if null metafile */
	if (!meta) return;

	/* Display as raster if available */

	/* Display background metafile if specified */
	if (meta->bgnd)
		{
		display_metafile(meta->bgnd);
		}

	/* Display the fields */
	for (i=0; i<meta->numfld; i++)
		{
	    display_field(meta->fields[i]);
		}

	/* Display legend */
	}

/***********************************************************************
*                                                                      *
*      d i s p l a y _ f i e l d                                       *
*                                                                      *
*      Display the given field.                                        *
*                                                                      *
***********************************************************************/

void	display_field

	(
	FIELD	fld
	)

	{
	int		dformat;

	/* Go home if null field */
	if (!fld) return;

	/* Prepare the field for display */
	dformat = field_display_format(fld->element, fld->level);
	prep_field(fld, dformat);

	/* Display whatever it really is */
	switch (fld->ftype)
		{
		case FtypeSfc:	display_surface(fld->data.sfc);
						break;
		case FtypeSet:	if (dformat == DisplayFormatComplex)
							{
							display_set_order(fld->data.set);
							}
						else
							{
							display_set(fld->data.set);
							}
						break;
		case FtypePlot:	display_plot(fld->data.plot);
						break;
		}
	}

/***********************************************************************
*                                                                      *
*      d i s p l a y _ s u r f a c e                                   *
*                                                                      *
*      Display the given surface.                                      *
*                                                                      *
***********************************************************************/

void	display_surface

	(
	SURFACE	sfc
	)

	{
	int			iu, iv;
	PATCH		cpatch;
	float		tx, ty, angle, xlim, ylim;
	LOGICAL		remap, ok;
	LSPEC		*lspec;
	MAP_PROJ	*mproj, smproj;

#	ifdef DEBUG_DISPLAY
	long	tbgn, tnxt;
	long	tdiff = 10000;
#	endif /* DEBUG_DISPLAY */

	static	COLOUR	C1 = -1;
	static	COLOUR	C2 = -1;

	/* Go home if null surface */
	if (!sfc) return;

#	ifdef DEBUG_DISPLAY
	tbgn = (long) clock();
#	endif /* DEBUG_DISPLAY */

	/* Display area band contours if they exist */
	display_set(sfc->bands);

	/* Set up orientation and offset */
	tx    = sfc->sp.origin[X];
	ty    = sfc->sp.origin[Y];
	angle = sfc->sp.orient;
	remap = (LOGICAL) (tx!=0 || ty!=0 || angle!=0);
	if (remap)
		{
		glConcatMatrix(sfc->sp.xform);

		xlim = sfc->nupatch*sfc->sp.gridlen;
		ylim = sfc->nvpatch*sfc->sp.gridlen;
		lspec = &sfc->cspecs[0].lspec;
		if (lspec && lspec->hilite >= 0)
			{
			gxLineSpec(lspec);
			glRectangle(0., 0., xlim, ylim);
			}
		}

	/* Save the original map projection */
	if ( NotNull(mproj = gxGetMproj()) )
		copy_map_projection(&smproj, mproj);
	else
		copy_map_projection(&smproj, NullMapProj);

	/* Display each patch curve set */
	if (pr_level("Show.Patches", 5))
		{
		if (C1==-1) C1 = find_colour("salmon",&ok);
		if (C2==-1) C2 = find_colour("goldenrod",&ok);
		}

	/* Set the map projection for displaying patches */
	gxSetMproj(NullMapProj);

	for (iu=0; iu<sfc->nupatch; iu++)
	    {
	    for (iv=0; iv<sfc->nvpatch; iv++)
			{
			cpatch = sfc->patches[iu][iv];
			if (NotNull(cpatch) && cpatch->defined)
				{

				/* Set the patch information */
				gxSetPatchInfo(sfc, iu, iv);

				/* Push transform to patch co-ordinates */
				glConcatMatrix(cpatch->xform);

				if (pr_level("Show.Patches", 5))
					{
					glSetColorIndex(((iv+1)%5==0)?C2:C1);
					glLineStyle(glSOLID);
					glLineWidth(0);
					glMove(0., 1.);
					glDraw(1., 1.);
					glSetColorIndex(((iu+1)%5==0)?C2:C1);
					glMove(1., 1.);
					glDraw(1., 0.);
					}

				display_set(cpatch->contours);
				display_set(cpatch->extrema);
				display_set(cpatch->vectors);

				/* Pop transform to patch co-ordinates */
				glPopMatrix();
				glFlush();
				}
			}
	    }
	gxSetPatchInfo(NullSfc, 0, 0);

	if (remap) glPopMatrix();
	glFlush();

	/* Reset the map projection after displaying patches */
	gxSetMproj(&smproj);

#	ifdef DEBUG_DISPLAY
	tnxt = (long) clock();
	if (tnxt - tbgn > tdiff)
		(void) printf("[display_surface] Surface: %d to %d\n", tbgn, tnxt);
#	endif /* DEBUG_DISPLAY */
	}

/***********************************************************************
*                                                                      *
*      d i s p l a y _ s e t                                           *
*      d i s p l a y _ s e t _ o r d e r                               *
*                                                                      *
*      Display the given set (in normal or reverse order).             *
*                                                                      *
***********************************************************************/

void	display_set

	(
	SET		set
	)

	{
	int		i;
	ITEM	*cp;

	/* Go home if null set */
	if (!set) return;

	/* Display each item in the set */
	cp = set->list;
	for (i=0; i<set->num; i++)
		{
	    display_item(set->type,cp[i]);
		}

	glFlush();
	}

void	display_set_order

	(
	SET		set
	)

	{
	int		i;
	ITEM	*cp;

	/* Go home if null set */
	if (!set) return;

	/* Display each item in the set in reverse order */
	cp = set->list;
	for (i=set->num-1; i>=0; i--)
		{
	    display_item(set->type,cp[i]);
		}

	glFlush();
	}

/***********************************************************************
*                                                                      *
*      d i s p l a y _ p l o t                                         *
*                                                                      *
*      Display the given plot.                                         *
*                                                                      *
***********************************************************************/

void	display_plot

	(
	PLOT	plot
	)

	{
	int		ispec, isub, ipt;
	PSUB	*sub;
	XFORM	xform;
	POINT	offset, transl;
	PLTSPEC	*cspec;
	float	mfact;

	ITEM	item = NULL;
	LABEL	lab  = NULL;
	MARK	mark = NULL;
	BARB	barb = NULL;
	BUTTON	butt = NULL;

	/* Go home if null plot */
	if (!plot) return;

	/* Plot one sub-field at a time */
	copy_xform(xform,IdentXform);
	for (isub=0; isub<plot->nsubs; isub++)
	    {
	    sub = plot->subs + isub;
	    if (!sub->proto)       continue;
	    if (plot->numpts <= 0) continue;

	    /* Make a copy of the current sub-field */
	    lab  = NULL;
	    mark = NULL;
	    barb = NULL;
	    butt = NULL;
	    item = copy_item(sub->type,sub->proto);
	    if (!item) continue;
	    else if (same(sub->type,"label"))  lab  = (LABEL)  item;
	    else if (same(sub->type,"mark"))   mark = (MARK)   item;
	    else if (same(sub->type,"barb"))   barb = (BARB)   item;
	    else if (same(sub->type,"button")) butt = (BUTTON) item;
	    else continue;

		/* Find corresponding plot spec for offset */
		/* Note: Treat offsets in VDC units */
		set_point(offset,0.,0.);
		mfact = gxGetMfact() / 1000; /* >>> shouldn't scale with zoom <<< */
		for (ispec=0; ispec<plot->ncspec; ispec++)
			{
			cspec = plot->cspecs + ispec;
			if (!same(cspec->name,sub->name)) continue;
			offset[X] = cspec->offset[X] * mfact;
			offset[Y] = cspec->offset[Y] * mfact;
			break;
			}
		/*
		if (lab)  lab->tspec.size    *= mfact;
		if (barb) barb->bspec.length *= mfact;
		*/

	    /* Display sub-field */
		if (lab)  FREEMEM(lab->label);
		if (butt) FREEMEM(butt->label);
	    for (ipt=0; ipt<plot->numpts; ipt++)
			{
			/* Temporary code to omit subfield if value is "!" */
			if (mark)
				{
				if (same(sub->sval1[ipt],"!")) continue;
				}

			/* Extract label values from value buffers */
			/* Note: quicker to copy string pointers */
			if (lab)  lab->label  = sub->sval2[ipt];
			if (butt) butt->label = sub->sval2[ipt];
			if (barb) barb->dir   = sub->fval1[ipt];
			if (barb) barb->speed = sub->fval2[ipt];

			/* Attach anchor to plot point */
			/* Use translation for complicated objects */
			copy_point(transl, offset);
			if (lab)       copy_point(lab->anchor, plot->pts[ipt]);
			else if (mark) copy_point(mark->anchor, plot->pts[ipt]);
			else if (barb) copy_point(barb->anchor, plot->pts[ipt]);
			else	{
					transl[X] += plot->pts[ipt][X];
					transl[Y] += plot->pts[ipt][Y];
					}

			/* Apply translation to account for position and offset */
			xform[H][X] = transl[X];
			xform[H][Y] = transl[Y];
			glConcatMatrix(xform);
			display_item(sub->type, item);
			glPopMatrix();
			}
		/* Note: string pointers were copied */
		if (lab)  lab->label  = NULL;
		if (butt) butt->label = NULL;
	    item = destroy_item(sub->type,item);
	    }

	glFlush();
	}

/***********************************************************************
*                                                                      *
*      d i s p l a y _ s p e c i a l                                   *
*                                                                      *
*      Display something special only this function knows about.       *
*                                                                      *
***********************************************************************/

void	display_special

	(
	STRING	type,
	POINTER	ptr
	)

	{
	if (!ptr) return;

	if (same(type,"image"))
		{
		glImageDisplay((Image)((long)ptr));
		}

	}

/***********************************************************************
*                                                                      *
*      d i s p l a y _ i t e m                                         *
*                                                                      *
*      Display the given item.                                         *
*                                                                      *
***********************************************************************/

void	display_item

	(
	STRING	type,
	ITEM	item
	)

	{
	if (!item) return;

	else if (same(type,"spot"))   display_spot((SPOT)     item);
	else if (same(type,"area"))   display_area((AREA)     item);
	else if (same(type,"curve"))  display_curve((CURVE)   item);
	else if (same(type,"lchain")) display_lchain((LCHAIN) item);
	else if (same(type,"label"))  display_label((LABEL)   item);
	else if (same(type,"mark"))   display_mark((MARK)     item);
	else if (same(type,"barb"))   display_barb((BARB)     item);
	else if (same(type,"button")) display_button((BUTTON) item);
	}

/***********************************************************************
*                                                                      *
*      d i s p l a y _ s p o t                                         *
*                                                                      *
*      Display the given spot.                                         *
*                                                                      *
***********************************************************************/

void	display_spot

	(
	SPOT	spot
	)

	{
	int			imem;
	SPMEM		*mem;
	POINT		pos;
	ITEM		item;
	STRING		type, name, val;
	float		mfact, wspd, wgst;
	double		dval;
	WIND_VAL	wval;
	LOGICAL		shown = FALSE;

	/* These get re-used for each display */
	static	LABEL	lab  = NULL;
	static	MARK	mark = NULL;
	static	BARB	barb = NULL;

	if (!spot) return;

	/* Reinitialize display objects */
	lab  = destroy_label(lab);
	lab  = create_label(NULL, NULL, NULL, ZeroPoint, 0.0);
	barb = destroy_barb(barb);
	barb = create_barb(NULL, ZeroPoint, 0.0, 0.0, 0.0);
	mark = destroy_mark(mark);
	mark = create_mark(NULL, NULL, NULL, ZeroPoint, 0.0);

	mfact = gxGetMfact() / 1000; /* >>> shouldn't scale with zoom <<< */

	for (imem=0; imem<spot->nmem; imem++)
		{
		mem = spot->members + imem;

		/* Note: Treat offsets in VDC units */
		pos[X] = spot->anchor[X] + mem->offset[X]*mfact;
		pos[Y] = spot->anchor[Y] + mem->offset[Y]*mfact;

		switch (mem->type)
			{
			case SpotText:
					name = blank(mem->attrib)? AttribUserlabel: mem->attrib;
					val  = CAL_get_attribute(spot->attrib, name);
					if (CAL_no_value(val)) continue;
					define_label_value(lab, NULL, NULL, val);
					define_label_anchor(lab, pos, 0.0);
					copy_tspec(&lab->tspec, &mem->tspec);
					item = (ITEM) lab;
					type = "label";
					break;

			case SpotBarb:
					name = blank(mem->attrib)? AttribUserlabel: mem->attrib;
					val  = CAL_get_attribute(spot->attrib, name);
					if (CAL_no_value(val)) continue;

#					ifdef DEBUG_BARB
					(void) printf("[display_spot] Display wind barb: %s\n", val);
#					endif /* DEBUG_BARB */

					/* Parse wind values from attribute if possible */
					if (!parse_wind_value_string(val, &wval)) continue;

					/* Set wind speed and gust from wind values */
					wspd = wval.speed;
					wgst = wval.gust;

					/* Check if wind speed conversion is needed */
					if (!blank(mem->bspec.uname) && !blank(wval.sunit)
							&& !same_ic(mem->bspec.uname, wval.sunit))
						{

#						ifdef DEBUG_BARB
						(void) printf("[display_spot] Convert wind barb speed from: %s to %s\n",
							wval.sunit, mem->bspec.uname);
#						endif /* DEBUG_BARB */

						if (convert_value(wval.sunit, (double) wspd,
											mem->bspec.uname, &dval))
							{
							wspd = (float) dval;
							}
						if (convert_value(wval.sunit, (double) wgst,
											mem->bspec.uname, &dval))
							{
							wgst = (float) dval;
							}
						}

					define_barb_value(barb, NULL, pos, wval.dir, wspd, wgst);
					copy_bspec(&barb->bspec, &mem->bspec);
					copy_tspec(&barb->tspec, &mem->tspec);
					item = (ITEM) barb;
					type = "barb";
					break;

			case SpotMark:
					define_mark_anchor(mark, pos, 0.0);
					copy_mspec(&mark->mspec, &mem->mspec);
					item = (ITEM) mark;
					type = "mark";
					break;

			default:	continue;
			}

		/* Display the member */
		display_item(type, item);
		shown = TRUE;
		}

	/* Display something - even if spot has no members */
	if (!shown)
		{
		/* Set up display attributes and display as a marker */
		gxSetColorIndex(SafeColour,0);
		glLineStyle(glSOLID);
		glLineWidth(0);
		gxMarkerSize(0.01);
		gxMarkerAngle(0.0);
	    gxDrawMarker(MarkerSquarePlus,spot->anchor[X],spot->anchor[Y]);
		(void) pr_warning("Spots",
			"[display_spot]: No display parameters for spot class \"%s\"\n",
			spot->mclass);
		(void) debug_attrib_list("[Spots] WARNING! [display_spot]",
			spot->attrib);
		}
	}

/***********************************************************************
*                                                                      *
*      d i s p l a y _ a r e a                                         *
*      d i s p l a y _ s u b a r e a                                   *
*      d i s p l a y _ s u b a r e a _ f i l l                         *
*      d i s p l a y _ s u b a r e a _ e d g e                         *
*                                                                      *
*      Display the given area.                                         *
*                                                                      *
*      <<< Later replace with display of partly obscured polygons >>>  *
*                                                                      *
***********************************************************************/

void	display_area

	(
	AREA	area
	)

	{
	int		nsub, isub, ihole, idiv;
	int		p, prio, pmin, pmax;
	FSPEC	*fspec;
	LSPEC	*lspec;
	SUBAREA	sub;
	LINE	poly, hole, divl;

#	ifdef DEBUG_DISPLAY
	long	tbgn, tnxt;
	long	tdiff = 10000;
#	endif /* DEBUG_DISPLAY */

	static	LOGICAL	first = TRUE;
	static	LSPEC	xspec;

	if (!area)                  return;
	if (!area->bound)           return;
	if (!area->bound->boundary) return;

	/* Initialize display for holes, dividing lines and boundary */
	if (first)
		{
		first = FALSE;
		init_lspec(&xspec);
		}

#	ifdef DEBUG_DISPLAY
	tbgn = (long) clock();
#	endif /* DEBUG_DISPLAY */

	/* Display single area if visible portions are not required */
	nsub = area->numdiv + 1;
	if (nsub == 1 && (!area->subareas || !area->subareas[0]->visready))
		{

		/* Set display from area or subarea specifications */
		if (!area->subareas)
			{
			fspec = &(area->fspec);
			lspec = &(area->lspec);
			}
		else
			{
			fspec = &(area->subareas[0]->fspec);
			lspec = &(area->subareas[0]->lspec);
			}

		/* Set the main boundary for display */
		poly  = area->bound->boundary;

		/* Pre-fill the interior first if needed */
		if (gxNeedPreFill(fspec->style, fspec->hilite))
			{
			if (fspec->hilite >= 0)
				{
				gxFillSpec(fspec, TRUE);

				/* Display any holes as unfilled polygons */
				for (ihole=0; ihole<area->bound->numhole; ihole++)
					{
					hole = area->bound->holes[ihole];
					glPolygonHole(hole->numpts, hole->points);
					}

				/* Display the main area as a filled polygon */
				glFilledPolygon(poly->numpts, poly->points);
				}

#			ifdef DEBUG_DISPLAY
			tnxt = (long) clock();
			if (tnxt - tbgn > tdiff)
				(void) printf("[display_area] Single prefill: %d to %d\n",
					tbgn, tnxt);
			tbgn = tnxt;
#			endif /* DEBUG_DISPLAY */
			}

		/* Display the interior first */
		if (gxNeedFill(fspec->style))
			{
			if (fspec->hilite >= 0)
				{
				gxFillSpec(fspec, FALSE);

				/* Display any holes as unfilled polygons */
				for (ihole=0; ihole<area->bound->numhole; ihole++)
					{
					hole = area->bound->holes[ihole];
					glPolygonHole(hole->numpts, hole->points);
					}

				/* Display the main area as a filled polygon */
				glFilledPolygon(poly->numpts, poly->points);
				}

#			ifdef DEBUG_DISPLAY
			tnxt = (long) clock();
			if (tnxt - tbgn > tdiff)
				(void) printf("[display_area] Single fill: %d to %d\n",
					tbgn, tnxt);
			tbgn = tnxt;
#			endif /* DEBUG_DISPLAY */
			}

		/* Display the edge of each hole */
		for (ihole=0; ihole<area->bound->numhole; ihole++)
			{
			hole = area->bound->holes[ihole];
			if (!draw_pattern(hole->points,hole->numpts,lspec->pattern,
							  Left,lspec->width,lspec->length,lspec->hilite))
				{
				gxLineSpec(lspec);
				glPolygon(hole->numpts, hole->points);
				}
			}

#		ifdef DEBUG_DISPLAY
		tnxt = (long) clock();
		if (tnxt - tbgn > tdiff)
			(void) printf("[display_area] Single holes: %d to %d  for pattern: %s\n",
				tbgn, tnxt, lspec->pattern);
		tbgn = tnxt;
#		endif /* DEBUG_DISPLAY */

		/* Now display the edge of the main area */
		if (!draw_pattern(poly->points,poly->numpts,lspec->pattern,Right,
						  lspec->width,lspec->length,lspec->hilite))
			{
			gxLineSpec(lspec);
			glPolygon(poly->numpts, poly->points);
			}

#		ifdef DEBUG_DISPLAY
		tnxt = (long) clock();
		if (tnxt - tbgn > tdiff)
			(void) printf("[display_area] Single boundary: %d to %d  for pattern: %s\n",
				tbgn, tnxt, lspec->pattern);
		tbgn = tnxt;
#		endif /* DEBUG_DISPLAY */
		}

	else
		{

		/* Find range of priorities */
		pmin = -1;
		pmax = -1;
		for (isub=0; isub<nsub; isub++)
			{
			sub = area->subareas[isub];
			if (sub->nsubvis <= 0) continue;

			prio = (int) floor(sub->lspec.width);
			if (pmin < 0 || prio < pmin) pmin = prio;
			if (pmax < 0 || prio > pmax) pmax = prio;
			}

		/* May need to display holes, dividing lines and boundary */
		if (area->bound->numhole > 0)
			{
			poly = area->bound->boundary;

			/* Display the edge of each hole */
			for (ihole=0; ihole<area->bound->numhole; ihole++)
				{
				hole = area->bound->holes[ihole];
				if (!draw_pattern(hole->points,hole->numpts,xspec.pattern,
								  Left,xspec.width,xspec.length, xspec.hilite))
					{
					gxLineSpec(&xspec);
					glPolygon(hole->numpts, hole->points);
					}
				}

			/* Now display each dividing line */
			for (idiv=0; idiv<area->numdiv; idiv++)
				{
				divl = area->divlines[idiv];
				if (!draw_pattern(divl->points,divl->numpts,xspec.pattern,
								  Left,xspec.width,xspec.length, xspec.hilite))
					{
					gxLineSpec(&xspec);
					glPolyLine(divl->numpts, divl->points);
					}
				}

			/* Now display the edge of the main area */
			if (!draw_pattern(poly->points,poly->numpts,xspec.pattern,Right,
							  xspec.width,xspec.length,xspec.hilite))
				{
				gxLineSpec(&xspec);
				glPolygon(poly->numpts, poly->points);
				}
			}

#		ifdef DEBUG_DISPLAY
		tnxt = (long) clock();
		if (tnxt - tbgn > tdiff)
			(void) printf("[display_area] Subarea predisplay: %d to %d\n",
				tbgn, tnxt);
		tbgn = tnxt;
#		endif /* DEBUG_DISPLAY */

		/* Now plot subareas in priority order */
		for (p=pmin; p<=pmax; p++)
			{
			for (isub=0; isub<nsub; isub++)
				{
				sub = area->subareas[isub];
				if (sub->nsubvis <= 0) continue;

				prio = (int) floor(sub->lspec.width);
				if (prio != p) continue;

				display_subarea(sub);
				}
			}

#		ifdef DEBUG_DISPLAY
		tnxt = (long) clock();
		if (tnxt - tbgn > tdiff)
			(void) printf("[display_area] Subareas: %d to %d\n", tbgn, tnxt);
#		endif /* DEBUG_DISPLAY */
		}
	}

/**********************************************************************/

void	display_subarea

	(
	SUBAREA	sub
	)

	{
	int		isub, ii, ips, ipe, ipl, ihole, *holesub;
	LOGICAL	do_pref, do_fill, do_edge, fwd, inside;
	SUBVIS	subvis;
	SEGMENT	seg;
	LINE	hole;
	LSPEC	*lspec;
	FSPEC	*fspec;

#	ifdef DEBUG_DISPLAY
	long	tbgn, tnxt;
	long	tdiff = 10000;
#	endif /* DEBUG_DISPLAY */

	static	LINE	poly = NULL;

	/* Go home if null subarea */
	if (!sub) return;
	if (sub->nsubvis <= 0) return;

	/* Go home if don't need to draw anything */
	fspec   = &sub->fspec;
	lspec   = &sub->lspec;
	do_pref = (LOGICAL) (gxNeedPreFill(fspec->style, fspec->hilite));
	do_fill = (LOGICAL) (fspec->hilite >= 0 && gxNeedFill(fspec->style));
	do_edge = (LOGICAL) (lspec->hilite >= 0);
	if (!do_fill && !do_edge) return;

#	ifdef DEBUG_DISPLAY
	tbgn = (long) clock();
#	endif /* DEBUG_DISPLAY */

	/* Loop through all visible segments */
	for (isub=0; isub<sub->nsubvis; isub++)
		{
		subvis = sub->subvis[isub];
		if (subvis->numvis <= 0) continue;

		/* Build a single polygon for each set of visible segments */
		empty_line(poly);
		for (ii=0; ii<subvis->numvis; ii++)
			{
			seg  = subvis->segvis[ii];
			fwd  = (LOGICAL) seg_forward(seg);
			ips  = seg->ips;
			ipe  = seg->ipe;
			ipl  = seg->line->numpts - 1;
			if (fwd)
				{
				if (ips <= ipe)
					{
					poly = append_line_pdir(poly,seg->line,ips,ipe,fwd);
					}
				else
					{
					poly = append_line_pdir(poly,seg->line,ips,ipl,fwd);
					poly = append_line_pdir(poly,seg->line,0,ipe,fwd);
					}
				}
			else
				{
				if (ips <= ipe)
					{
					poly = append_line_pdir(poly,seg->line,ips,ipe,fwd);
					}
				else
					{
					poly = append_line_pdir(poly,seg->line,0,ipe,fwd);
					poly = append_line_pdir(poly,seg->line,ips,ipl,fwd);
					}
				}
			}
		add_point_to_line(poly, poly->points[0]);

#		ifdef DEBUG_DISPLAY
		tnxt = (long) clock();
		if (tnxt - tbgn > tdiff)
			(void) printf("[display_subarea] Outline build: %d to %d\n",
				tbgn, tnxt);
#		endif /* DEBUG_DISPLAY */

		/* Identify subarea holes enclosed by this visible region */
		holesub = INITMEM(int, sub->numhole);
		for (ihole=0; ihole<sub->numhole; ihole++)
			{
			hole = sub->holes[ihole];
			line_test_point(poly, hole->points[0], NullFloat, NullPoint,
							NullInt, &inside, NullChar);
			holesub[ihole] = inside;
			}

#		ifdef DEBUG_DISPLAY
		tnxt = (long) clock();
		if (tnxt - tbgn > tdiff)
			(void) printf("[display_subarea] Identify holes: %d to %d\n",
				tbgn, tnxt);
#		endif /* DEBUG_DISPLAY */

		/* Pre-fill a filled polygon */
		if (do_pref)
			{
			/* Set up fill and edge presentation */
			gxFillSpec(fspec, TRUE);

			/* Display any enclosed holes as unfilled polygons */
			for (ihole=0; ihole<sub->numhole; ihole++)
				{
				if (!holesub[ihole]) continue;
				hole = sub->holes[ihole];
				glPolygonHole(hole->numpts, hole->points);
				}

			glFilledPolygon(poly->numpts, poly->points);

#			ifdef DEBUG_DISPLAY
			tnxt = (long) clock();
			if (tnxt - tbgn > tdiff)
				(void) printf("[display_subarea] Prefill: %d to %d\n",
					tbgn, tnxt);
#			endif /* DEBUG_DISPLAY */
			}

		/* Display a filled polygon */
		if (do_fill)
			{
			/* Set up fill and edge presentation */
			gxFillSpec(fspec, FALSE);

			/* Display any enclosed holes as unfilled polygons */
			for (ihole=0; ihole<sub->numhole; ihole++)
				{
				if (!holesub[ihole]) continue;
				hole = sub->holes[ihole];
				glPolygonHole(hole->numpts, hole->points);
				}

			glFilledPolygon(poly->numpts, poly->points);

#			ifdef DEBUG_DISPLAY
			tnxt = (long) clock();
			if (tnxt - tbgn > tdiff)
				(void) printf("[display_subarea] Fill: %d to %d\n",
					tbgn, tnxt);
#			endif /* DEBUG_DISPLAY */
			}

		/* Display the polygon edge */
		if (do_edge)
			{
			/* Display the edge of each enclosed hole */
			for (ihole=0; ihole<sub->numhole; ihole++)
				{
				if (!holesub[ihole]) continue;
				hole = sub->holes[ihole];
				if (!draw_pattern(hole->points, hole->numpts, lspec->pattern,
								  Left, lspec->width, lspec->length,
								  lspec->hilite))
					{
					gxLineSpec(lspec);
					glPolygon(hole->numpts, hole->points);
					}
				}

			if (!draw_pattern(poly->points, poly->numpts, lspec->pattern,
							  Right, lspec->width, lspec->length,
							  lspec->hilite))
				{
				gxLineSpec(lspec);
				glPolygon(poly->numpts, poly->points);
				}

#			ifdef DEBUG_DISPLAY
			tnxt = (long) clock();
			if (tnxt - tbgn > tdiff)
				(void) printf("[display_subarea] Boundary: %d to %d  for pattern: %s\n",
					tbgn, tnxt, lspec->pattern);
#			endif /* DEBUG_DISPLAY */
			}

		FREEMEM(holesub);
		empty_line(poly);
		}
	}

/***********************************************************************
*                                                                      *
*      d i s p l a y _ c u r v e                                       *
*                                                                      *
*      Display the given curve.                                        *
*                                                                      *
***********************************************************************/

void	display_curve

	(
	CURVE	curve
	)

	{
	LINE	seg;
	LSPEC	*lspec;

#	ifdef DEBUG_DISPLAY
	long	tbgn, tnxt;
	long	tdiff = 10000;
#	endif /* DEBUG_DISPLAY */

	/* Go home if null curve or no points */
	if (!curve)           return;
	seg = curve->line;
	if (!seg)             return;
	if (seg->numpts <= 0) return;
	lspec = &curve->lspec;

#	ifdef DEBUG_DISPLAY
	tbgn = (long) clock();
#	endif /* DEBUG_DISPLAY */

	/* Set up display attributes and plot the curve */
	if (lspec->hilite < 0) return;
	if (!draw_pattern(seg->points,seg->numpts,lspec->pattern,curve->sense,
					  lspec->width,lspec->length,lspec->hilite))
		{
		gxLineSpec(lspec);
		glPolyLine(seg->numpts, seg->points);
		}

#	ifdef DEBUG_DISPLAY
	tnxt = (long) clock();
	if (tnxt - tbgn > tdiff)
		(void) printf("[display_curve] Curve: %d to %d  for pattern: %s\n",
			tbgn, tnxt, lspec->pattern);
#	endif /* DEBUG_DISPLAY */
	}

/***********************************************************************
*                                                                      *
*      d i s p l a y _ l c h a i n                                     *
*                                                                      *
*      Display the given link chain.                                   *
*                                                                      *
***********************************************************************/

void	display_lchain

	(
	LCHAIN	chain
	)

	{
	int			inode;
	LINE		track;
	LNODE		lnode;
	LINTERP		linterp;
	LSPEC		*lspec;
	float		mfact, wspd, wgst;
	double		dval;
	WIND_VAL	wval;
	MAP_PROJ	*mproj;
	ATTRIB_LIST	xattrib;
	int			imem;
	NODEMEM		*mem;
	STRING		type, name, val, vtime;
	POINT		pos;
	ITEM		item;
	LOGICAL		shown;

	/* These get re-used for each display */
	static	LABEL	lab  = NULL;
	static	BARB	barb = NULL;
	static	MARK	mark = NULL;

#	ifdef DEBUG_DISPLAY
	long	tbgn, tnxt;
	long	tdiff = 10000;
#	endif /* DEBUG_DISPLAY */

	/* Go home if null link chain or no nodes */
	if (!chain)           return;
	if (chain->lnum <= 0) return;

	/* Set presentation for link chain track */
	lspec = &chain->lspec;

	/* Reinitialize display objects */
	lab  = destroy_label(lab);
	lab  = create_label(NULL, NULL, NULL, ZeroPoint, 0.0);
	barb = destroy_barb(barb);
	barb = create_barb(NULL, ZeroPoint, 0.0, 0.0, 0.0);
	mark = destroy_mark(mark);
	mark = create_mark(NULL, NULL, NULL, ZeroPoint, 0.0);
	mfact = gxGetMfact() / 1000; /* >>> shouldn't scale with zoom <<< */
	
	/* Set the current map projection */
	mproj = gxGetMproj();

#	ifdef DEBUG_DISPLAY
	tbgn = (long) clock();
#	endif /* DEBUG_DISPLAY */

	/* Interpolate the link chain track (if required) */
	if (chain->dointerp) (void) interpolate_lchain(chain);

	/* Set up display attributes and plot the link chain track */
	if (lspec->hilite < 0) return;
	track = chain->track;
	if (!draw_pattern(track->points, track->numpts, lspec->pattern, Right,
					  lspec->width, lspec->length, lspec->hilite))
		{
		gxLineSpec(lspec);
		glPolyLine(track->numpts, track->points);
		}

	/* Now display the link chain interpolated nodes */
	for (inode=0; inode<chain->inum; inode++)
		{
		linterp = chain->interps[inode];
		shown = FALSE;

		/* Add motion at link chain interpolated node to the node attributes */
		xattrib = copy_attrib_list(linterp->attrib);
		CAL_add_lchain_node_motion(xattrib, mproj, chain, linterp->mplus);

		/* Add interpolated node timestamp to the node attributes */
		vtime = calc_valid_time_minutes(chain->xtime, 0, linterp->mplus);
		CAL_add_attribute(xattrib, AttribLnodeTstamp, vtime);

		/* Display node parameters */
		for (imem=0; imem<linterp->nmem; imem++)
			{
			mem = linterp->members + imem;

			/* Note: Treat offsets in VDC units */
			pos[X] = linterp->node[X] + mem->offset[X]*mfact;
			pos[Y] = linterp->node[Y] + mem->offset[Y]*mfact;

			/* Display text, barb or mark for each interpolated node */
			switch (mem->type)
				{

				case NodeText:
						name = blank(mem->attrib)? AttribUserlabel: mem->attrib;
						val  = CAL_get_attribute(xattrib, name);
						if (CAL_no_value(val)) continue;
						define_label_value(lab, NULL, NULL, val);
						define_label_anchor(lab, pos, 0.0);
						copy_tspec(&lab->tspec, &mem->tspec);
						item = (ITEM) lab;
						type = "label";
						break;

				case NodeBarb:
						name = blank(mem->attrib)? AttribUserlabel: mem->attrib;
						val  = CAL_get_attribute(xattrib, name);
						if (CAL_no_value(val)) continue;

#						ifdef DEBUG_BARB
						(void) printf("[display_lchain] Display wind barb: %s\n", val);
#						endif /* DEBUG_BARB */

						/* Parse wind values from attribute if possible */
						if (!parse_wind_value_string(val, &wval)) continue;

						/* Set wind speed and gust from wind values */
						wspd = wval.speed;
						wgst = wval.gust;

						/* Check if wind speed conversion is needed */
						if (!blank(mem->bspec.uname) && !blank(wval.sunit)
								&& !same_ic(mem->bspec.uname, wval.sunit))
							{
	
#							ifdef DEBUG_BARB
							(void) printf("[display_lchain] Convert wind barb speed from: %s to %s\n",
								wval.sunit, mem->bspec.uname);
#							endif /* DEBUG_BARB */

							if (convert_value(wval.sunit, (double) wspd,
												mem->bspec.uname, &dval))
								{
								wspd = (float) dval;
								}
							if (convert_value(wval.sunit, (double) wgst,
												mem->bspec.uname, &dval))
								{
								wgst = (float) dval;
								}
							}

						define_barb_value(barb, NULL, pos, wval.dir, wspd, wgst);
						copy_bspec(&barb->bspec, &mem->bspec);
						copy_tspec(&barb->tspec, &mem->tspec);
						item = (ITEM) barb;
						type = "barb";
						break;

				case NodeMark:
						define_mark_anchor(mark, pos, 0.0);
						copy_mspec(&mark->mspec, &mem->mspec);
						item = (ITEM) mark;
						type = "mark";
						break;

				default:	continue;
				}

			/* Display the member */
			display_item(type, item);
			shown = TRUE;
			}

		/* Display something - even if node has no members */
		if (!shown)
			{
			/* Set up display attributes and display as a marker */
			gxSetColorIndex(SafeColour,0);
			glLineStyle(glSOLID);
			glLineWidth(0);
			gxMarkerSize(0.01);
			gxMarkerAngle(0.0);
			gxDrawMarker(MarkerSquarePlus, linterp->node[X], linterp->node[Y]);
			(void) pr_warning("Lnodes",
				"[display_lchain]: No display parameters for node class \"%s\"\n",
				FpaNodeClass_Interp);
			(void) debug_attrib_list("[Lnodes] WARNING! [display_lchain]",
				linterp->attrib);
			}

		/* Destroy the copied node attributes */
		xattrib = destroy_attrib_list(xattrib);
		}

	/* Last display the link chain nodes */
	for (inode=0; inode<chain->lnum; inode++)
		{
		lnode = chain->nodes[inode];
		shown = FALSE;

		/* Add motion at link chain node to the node attributes */
		xattrib = copy_attrib_list(lnode->attrib);
		CAL_add_lchain_node_motion(xattrib, mproj, chain, lnode->mplus);

		/* Add node timestamp to the node attributes */
		vtime = calc_valid_time_minutes(chain->xtime, 0, lnode->mplus);
		CAL_add_attribute(xattrib, AttribLnodeTstamp, vtime);

		/* Display node parameters */
		for (imem=0; imem<lnode->nmem; imem++)
			{
			mem = lnode->members + imem;

			/* Note: Treat offsets in VDC units */
			pos[X] = lnode->node[X] + mem->offset[X]*mfact;
			pos[Y] = lnode->node[Y] + mem->offset[Y]*mfact;

			/* Display text, barb or mark for each link node */
			switch (mem->type)
				{

				case NodeText:
						name = blank(mem->attrib)? AttribUserlabel: mem->attrib;
						val  = CAL_get_attribute(xattrib, name);
						if (CAL_no_value(val)) continue;
						define_label_value(lab, NULL, NULL, val);
						define_label_anchor(lab, pos, 0.0);
						copy_tspec(&lab->tspec, &mem->tspec);
						item = (ITEM) lab;
						type = "label";
						break;

				case NodeBarb:
						name = blank(mem->attrib)? AttribUserlabel: mem->attrib;
						val  = CAL_get_attribute(xattrib, name);
						if (CAL_no_value(val)) continue;

#						ifdef DEBUG_BARB
						(void) printf("[display_lchain] Display wind barb: %s\n", val);
#						endif /* DEBUG_BARB */

						/* Parse wind values from attribute if possible */
						if (!parse_wind_value_string(val, &wval)) continue;

						/* Set wind speed and gust from wind values */
						wspd = wval.speed;
						wgst = wval.gust;

						/* Check if wind speed conversion is needed */
						if (!blank(mem->bspec.uname) && !blank(wval.sunit)
								&& !same_ic(mem->bspec.uname, wval.sunit))
							{
	
#							ifdef DEBUG_BARB
							(void) printf("[display_lchain] Convert wind barb speed from: %s to %s\n",
								wval.sunit, mem->bspec.uname);
#							endif /* DEBUG_BARB */

							if (convert_value(wval.sunit, (double) wspd,
												mem->bspec.uname, &dval))
								{
								wspd = (float) dval;
								}
							if (convert_value(wval.sunit, (double) wgst,
												mem->bspec.uname, &dval))
								{
								wgst = (float) dval;
								}
							}

						define_barb_value(barb, NULL, pos, wval.dir, wspd, wgst);
						copy_bspec(&barb->bspec, &mem->bspec);
						copy_tspec(&barb->tspec, &mem->tspec);
						item = (ITEM) barb;
						type = "barb";
						break;

				case NodeMark:
						define_mark_anchor(mark, pos, 0.0);
						copy_mspec(&mark->mspec, &mem->mspec);
						item = (ITEM) mark;
						type = "mark";
						break;

				default:	continue;
				}

			/* Display the member */
			display_item(type, item);
			shown = TRUE;
			}

		/* Display something - even if node has no members */
		if (!shown)
			{
			/* Set up display attributes and display as a marker */
			gxSetColorIndex(SafeColour,0);
			glLineStyle(glSOLID);
			glLineWidth(0);
			gxMarkerSize(0.01);
			gxMarkerAngle(0.0);
			gxDrawMarker(MarkerSquarePlus, lnode->node[X], lnode->node[Y]);
			if (lnode->ltype == LchainNode && lnode->guess)
				(void) pr_warning("Lnodes",
					"[display_lchain]: No display parameters for node class \"%s\"\n",
					FpaNodeClass_NormalGuess);
			else if (lnode->ltype == LchainNode)
				(void) pr_warning("Lnodes",
					"[display_lchain]: No display parameters for node class \"%s\"\n",
					FpaNodeClass_Normal);
			else if (lnode->ltype == LchainControl && lnode->guess)
				(void) pr_warning("Lnodes",
					"[display_lchain]: No display parameters for node class \"%s\"\n",
					FpaNodeClass_ControlGuess);
			else if (lnode->ltype == LchainControl)
				(void) pr_warning("Lnodes",
					"[display_lchain]: No display parameters for node class \"%s\"\n",
					FpaNodeClass_Control);
			else if (lnode->ltype == LchainFloating)
				(void) pr_warning("Lnodes",
					"[display_lchain]: No display parameters for node class \"%s\"\n",
					FpaNodeClass_Control);
			else
				(void) pr_warning("Lnodes",
					"[display_lchain]: No display parameters for node class \"%s\"\n",
					FpaNodeClass_Unknown);
			(void) debug_attrib_list("[Lnodes] WARNING! [display_lchain]",
				lnode->attrib);
			}

		/* Destroy the copied node attributes */
		xattrib = destroy_attrib_list(xattrib);
		}

#	ifdef DEBUG_DISPLAY
	tnxt = (long) clock();
	if (tnxt - tbgn > tdiff)
		(void) printf("[display_lchain] Link Chain: %d to %d  for pattern: %s\n",
			tbgn, tnxt, lspec->pattern);
#	endif /* DEBUG_DISPLAY */
	}

/***********************************************************************
*                                                                      *
*      d i s p l a y _ l a b e l                                       *
*                                                                      *
*      Display the given label.                                        *
*                                                                      *
***********************************************************************/

void	display_label

	(
	LABEL	label
	)

	{
	float	x, y, delta;
	STRING	val;
	LOGICAL	outline;

	/* Go home if null label or nothing to plot */
	if (!label)     return;
	val = label->label;
	if (blank(val)) return;

	/* Set up display attributes */
	if (label->tspec.hilite < 0) return;
	gxTextSpec(&label->tspec);
	glTextAngle(label->tspec.angle + label->angle);
	delta = gxGetPixelSize();
	x     = label->anchor[X];
	y     = label->anchor[Y];

	if (label->tspec.hilite <= 0)
		{
		outline = (LOGICAL) (label->tspec.bcolour == label->tspec.tcolour);

		/* Display bottom shadow if requested */
		if (label->tspec.bcolour >= 0)
			{
			gxSetColorIndex(label->tspec.bcolour,label->tspec.hilite);
			glDrawString(x+delta, y      , val);
			glDrawString(x+delta, y-delta, val);
			glDrawString(x      , y-delta, val);
			if (outline)
				{
				glDrawString(x-delta, y-delta, val);
				}
			}

		/* Display top shadow if requested */
		if (label->tspec.tcolour >= 0)
			{
			gxSetColorIndex(label->tspec.tcolour,label->tspec.hilite);
			glDrawString(x-delta, y      , val);
			glDrawString(x-delta, y+delta, val);
			glDrawString(x      , y+delta, val);
			if (outline)
				{
				glDrawString(x+delta, y+delta, val);
				}
			}
		}

	/* Display the text */
	gxSetColorIndex(label->tspec.colour,label->tspec.hilite);
	glDrawString(x, y, val);
	}

/***********************************************************************
*                                                                      *
*      d i s p l a y _ m a r k                                         *
*                                                                      *
*      Display the given mark.                                         *
*                                                                      *
***********************************************************************/

void	display_mark

	(
	MARK	mark
	)

	{
	float	x, y, delta;
	STRING	val;
	LOGICAL	outline;

	/* Go home if null mark */
	if (!mark) return;

	/* Set up display attributes */
	if (mark->mspec.hilite < 0) return;
	gxMarkerSpec(&mark->mspec);
	gxMarkerAngle(mark->mspec.angle + mark->angle);
	delta = gxGetPixelSize();
	x     = mark->anchor[X];
	y     = mark->anchor[Y];

	/* Display a marker */
	if (mark->mspec.type >= 0)
		{
		if (mark->mspec.hilite <= 0)
			{
			outline = (LOGICAL) (mark->mspec.bcolour == mark->mspec.tcolour);

			/* Display bottom shadow if requested */
			if (mark->mspec.bcolour >= 0)
				{
				gxSetColorIndex(mark->mspec.bcolour,mark->mspec.hilite);
				gxDrawMarker(mark->mspec.type, x+delta, y);
				gxDrawMarker(mark->mspec.type, x+delta, y-delta);
				gxDrawMarker(mark->mspec.type, x,       y-delta);
				if (outline)
					{
					gxDrawMarker(mark->mspec.type, x-delta, y-delta);
					}
				}

			/* Display top shadow if requested */
			if (mark->mspec.tcolour >= 0)
				{
				gxSetColorIndex(mark->mspec.tcolour,mark->mspec.hilite);
				gxDrawMarker(mark->mspec.type, x-delta, y);
				gxDrawMarker(mark->mspec.type, x-delta, y+delta);
				gxDrawMarker(mark->mspec.type, x,       y+delta);
				if (outline)
					{
					gxDrawMarker(mark->mspec.type, x+delta, y+delta);
					}
				}
			}

		/* Display the marker */
		gxSetColorIndex(mark->mspec.colour,mark->mspec.hilite);
		gxDrawMarker(mark->mspec.type, x, y);
		}

	/* Display a user defined symbol */
	else if ( !blank(val = mark->mspec.symbol) )
		{
		if (mark->mspec.hilite <= 0)
			{
			outline = (LOGICAL) (mark->mspec.bcolour == mark->mspec.tcolour);

			/* Display bottom shadow if requested */
			if (mark->mspec.bcolour >= 0)
				{
				gxSetColorIndex(mark->mspec.bcolour,mark->mspec.hilite);
				glDrawString(x+delta, y      , val);
				glDrawString(x+delta, y-delta, val);
				glDrawString(x      , y-delta, val);
				if (outline)
					{
					glDrawString(x-delta, y-delta, val);
					}
				}

			/* Display top shadow if requested */
			if (mark->mspec.tcolour >= 0)
				{
				gxSetColorIndex(mark->mspec.tcolour,mark->mspec.hilite);
				glDrawString(x-delta, y      , val);
				glDrawString(x-delta, y+delta, val);
				glDrawString(x      , y+delta, val);
				if (outline)
					{
					glDrawString(x+delta, y+delta, val);
					}
				}
			}

		/* Display the symbol */
		gxSetColorIndex(mark->mspec.colour,mark->mspec.hilite);
		glDrawString(x, y, val);
		}
	}

/***********************************************************************
*                                                                      *
*      d i s p l a y _ b a r b                                         *
*      d i s p l a y _ b a r b _ w i n d                               *
*      d i s p l a y _ b a r b _ a r r o w                             *
*      d i s p l a y _ b a r b _ v a l u e                             *
*                                                                      *
*      Display the given barb.                                         *
*                                                                      *
***********************************************************************/

void	display_barb

	(
	BARB	barb
	)

	{
	if (!barb) return;

	switch (barb->bspec.type)
		{
		case BarbWind:	display_barb_wind(barb);
						break;

		case BarbArrow:	display_barb_arrow(barb);
						break;
		}

	if (barb->bspec.value) display_barb_value(barb);
	}

void	display_barb_wind

	(
	BARB	barb
	)

	{
	static	float	blen=.40;
	static	float	bspc=.20;
	static	float	bwid=.20;
	static	float	bang= 60;

	SURFACE		sfc;
	POINT		pw;
	MAP_PROJ	*mproj, *zproj;
	int			spd, i, n1, n10, n50, iu, iv;
	float		lenbarb, onebarb, spcbarb, widbarb, lenshaft;
	float		wdir, wspd, angwind, angbarb;
	double		dspd;
	float		xwind, xbarb, xend, xt;
	float		ywind, ybarb, yend, yt;
	float		lat, lon;
	float		Wmin, Wmax;
	FpaConfigUnitStruct		*Wudef, *Dudef, *udef;

	/* Return immediately if no wind barb object */
	if (!barb) return;
	if (barb->bspec.hilite < 0) return;

	/* Get the default wind display parameters */
	(void) gxGetWindParms(&Wmin, &Wmax, &Wudef, &Dudef);

	/* Set wind direction and adjusted wind speed */
	wdir  = range_norm(barb->dir, 0., 360., NULL);
	wspd  = barb->speed;

	/* Set the wind barb presentation units ... use default if required */
	udef = identify_unit(barb->bspec.uname);
	if (IsNull(udef)) udef = Dudef;

	/* Check if wind barb presentation matches the default wind units */
	if (same(udef->MKS, Wudef->MKS))
		{
		/* Convert to display units (if required) */
		if (!same(udef->name, Wudef->name))
			{
			(void) pr_diag("WindBarbs",
				"[display_barb_wind] Converting from: %s  to display units: %s\n",
				udef->name, Wudef->name);
			(void) pr_diag("WindBarbs",
				"[display_barb_wind]   Presentation wind units: %s\n",
				barb->bspec.uname);
			(void) convert_value(udef->name, (double) wspd, Wudef->name, &dspd);
			wspd = (float) dspd;
			}
		}

	/* Assume that anything else is in MKS */
	else if (!same(udef->name, udef->MKS))
		{
		(void) pr_diag("WindBarbs",
			"[display_barb_wind] Converting from: MKS  to display units: %s\n",
			udef->name);
		(void) convert_value("MKS", (double) wspd, udef->name, &dspd);
		wspd = (float) dspd;
		}

	/* Check wind value - if very small, plot a small circle */
	if (wspd < Wmin)
	    {
		/* Set up display attributes and display as a marker */
		gxSetColorIndex(barb->bspec.colour,barb->bspec.hilite);
		gxMarkerSize(0.0);
		gxMarkerAngle(0.0);
	    gxDrawMarker(MarkerCircle,barb->anchor[X],barb->anchor[Y]);
	    return;
	    }
	/* Check wind value - if very large, plot a plus in a larger circle */
	if (wspd > Wmax)
	    {
		/* Set up display attributes and display as a marker */
		gxSetColorIndex(barb->bspec.colour,barb->bspec.hilite);
		gxMarkerSize(0.005);
		gxMarkerAngle(0.0);
	    gxDrawMarker(MarkerCirclePlus,barb->anchor[X],barb->anchor[Y]);
	    return;
	    }

	/* Calculate barb size (speed) parameters */
	spd      = NINT(wspd);
	n1       = spd % 10;
	n10      = spd % 50 / 10;
	n50      = spd / 50;
	lenshaft = barb->bspec.length / 1000;
	if (!barb->bspec.scale) lenshaft *= gxGetMfact();
	lenbarb  = blen * lenshaft;
	onebarb  = n1 * lenbarb / 10;
	spcbarb  = bspc * lenshaft;
	widbarb  = bwid * lenshaft;
	if (n50 > 1) lenshaft += (n50-1) * widbarb;

	/* Calculate direction parameters */
	if ( NotNull(sfc = gxGetPatchInfo(&iu, &iv)) )
		{
		(void) patch_to_world(&sfc->sp, barb->anchor, iu, iv, pw);
		(void) pos_to_ll(&sfc->sp.mp, pw, &lat, &lon);
		angwind = RAD * (90 - wdir);

#		ifdef DEBUG_BARB
		(void) printf("[display_barb_wind] SFC lat/long: %.2f/%.2f  wdir: %.2f  angwind: %.2f\n",
				lat, lon, wdir, angwind);
#		endif /* DEBUG_BARB */

		}

	else if ( NotNull(zproj = gxGetZoomMproj())
				&& !equivalent_map_projection(zproj, &NoMapProj)
				&& NotNull(mproj = get_target_map()) )
		{

#		ifdef DEBUG_BARB
		if ( complete_map_projection(zproj) )
			{
			(void) printf("[display_barb_wind] Zoomed projection type: %s  %.2f  %.2f  %.2f\n",
					which_projection_name(zproj->projection.type),
					zproj->projection.ref[0], zproj->projection.ref[1],
					zproj->projection.ref[2]);
			(void) printf("[display_barb_wind] Basemap olat/olon: %.2f/%.2f  lref: %.2f\n",
					zproj->definition.olat, zproj->definition.olon, zproj->definition.lref);
			(void) printf("[display_barb_wind]         xorg/yorg: %.0f/%.0f  xlen/ylen: %.0f/%.0f  units: %.0f\n",
					zproj->definition.xorg, zproj->definition.yorg,
					zproj->definition.xlen, zproj->definition.ylen,
					zproj->definition.units);
			(void) printf("[display_barb_wind] Map origin: %.0f/%.0f  clat/clon: %.2f/%.2f\n",
					zproj->origin[X], zproj->origin[Y],
					zproj->clat, zproj->clon);
			}
		else
			{
			(void) printf("[display_barb_wind] Missing zoomed map projection!\n");
			}
#		endif /* DEBUG_BARB */

		(void) pos_to_ll(mproj, barb->anchor, &lat, &lon);
		angwind = RAD * wind_dir_xy(zproj, lat, lon, wdir);

#		ifdef DEBUG_BARB
		(void) printf("[display_barb_wind] Barb anchor: %.0f/%.0f  lat/long: %.2f/%.2f  wdir: %.2f  angwind: %.2f\n",
				barb->anchor[X], barb->anchor[Y], lat, lon, wdir, angwind);
#		endif /* DEBUG_BARB */

		}

	else if ( NotNull(mproj = get_target_map())
					&& !equivalent_map_projection(mproj, &NoMapProj) )
		{

#		ifdef DEBUG_BARB
		if ( complete_map_projection(mproj) )
			{
			(void) printf("[display_barb_wind] Projection type: %s  %.2f  %.2f  %.2f\n",
					which_projection_name(mproj->projection.type),
					mproj->projection.ref[0], mproj->projection.ref[1],
					mproj->projection.ref[2]);
			(void) printf("[display_barb_wind] Basemap olat/olon: %.2f/%.2f  lref: %.2f\n",
					mproj->definition.olat, mproj->definition.olon, mproj->definition.lref);
			(void) printf("[display_barb_wind]         xorg/yorg: %.0f/%.0f  xlen/ylen: %.0f/%.0f  units: %.0f\n",
					mproj->definition.xorg, mproj->definition.yorg,
					mproj->definition.xlen, mproj->definition.ylen,
					mproj->definition.units);
			(void) printf("[display_barb_wind] Map origin: %.0f/%.0f  clat/clon: %.2f/%.2f\n",
					mproj->origin[X], mproj->origin[Y],
					mproj->clat, mproj->clon);
			}
		else
			{
			(void) printf("[display_barb_wind] Missing map projection!\n");
			}
#		endif /* DEBUG_BARB */

		(void) pos_to_ll(mproj, barb->anchor, &lat, &lon);
		angwind = RAD * wind_dir_xy(mproj, lat, lon, wdir);

#		ifdef DEBUG_BARB
		(void) printf("[display_barb_wind] Barb anchor: %.0f/%.0f  lat/long: %.2f/%.2f  wdir: %.2f  angwind: %.2f\n",
				barb->anchor[X], barb->anchor[Y], lat, lon, wdir, angwind);
#		endif /* DEBUG_BARB */

		}
	else
		{
		lat = lon = 0.0;
		angwind = RAD * (90 - wdir);

#		ifdef DEBUG_BARB
		(void) printf("[display_barb_wind] No mproj wdir: %.2f  angwind: %.2f\n",
				wdir, angwind);
#		endif /* DEBUG_BARB */

		}
	if (barb->bspec.sense) angwind += M_PI;
	xwind   = cos(angwind);
	ywind   = sin(angwind);
	angbarb = (lat>=0)? angwind - RAD*bang: angwind + RAD*bang;
	xbarb   = cos(angbarb);
	ybarb   = sin(angbarb);

	/* Set up display attributes and draw the barb */
	gxBarbSpec(&barb->bspec);

	/* Draw shaft of barb */
	xt   = barb->anchor[X];
	yt   = barb->anchor[Y];
	xend = xt + lenshaft*xwind;
	yend = yt + lenshaft*ywind;
	glMove(xt,yt);
	glDraw(xend,yend);

	/* Draw 50km barbs */
	for (i=0; i<n50; i++)
	    {
	    xt = xend + lenbarb*xbarb;
	    yt = yend + lenbarb*ybarb;
	    glDraw(xt,yt);
	    xt = xend + widbarb*xwind;
	    yt = yend + widbarb*ywind;
	    glDraw(xt,yt);
	    xend = xend - spcbarb*xwind;
	    yend = yend - spcbarb*ywind;
	    glDraw(xend,yend);
	    }

	/* Draw 10km barbs */
	for (i=0; i<n10; i++)
	    {
	    xt = xend + lenbarb*xbarb;
	    yt = yend + lenbarb*ybarb;
	    glDraw(xt,yt);
	    glDraw(xend,yend);
	    xend = xend - spcbarb*xwind;
	    yend = yend - spcbarb*ywind;
	    glDraw(xend,yend);
	    }

	/* If no 50km or 10km barbs, indent the units barb */
	if ((n50 <= 0) && (n10 <= 0))
	    {
	    xend = xend - spcbarb*xwind;
	    yend = yend - spcbarb*ywind;
	    glDraw(xend,yend);
	    }

	/* Draw the units barb */
	xt = xend + onebarb*xbarb;
	yt = yend + onebarb*ybarb;
	glDraw(xt,yt);
	glDraw(xend,yend);
	}

void	display_barb_arrow

	(
	BARB	barb
	)

	{
	static	float	hlen=.20;
	static	float	hwid=.10;

	SURFACE		sfc;
	POINT		pw;
	MAP_PROJ	*mproj, *zproj;
	int			iu, iv;
	float		lenhead, widhead, lenshaft;
	float		wdir, wspd, angwind;
	double		dspd;
	float		xwind, xhead, xend, xt;
	float		ywind, yhead, yend, yt;
	float		lat, lon;
	float		Wmin, Wmax;
	FpaConfigUnitStruct		*Wudef, *Dudef, *udef;

	/* Return immediately if no wind barb object */
	if (!barb) return;
	if (barb->bspec.hilite < 0) return;

	/* Get the default wind display parameters */
	(void) gxGetWindParms(&Wmin, &Wmax, &Wudef, &Dudef);

	/* Set wind direction and wind speed */
	wdir = range_norm(barb->dir, 0., 360., NULL);
	wspd = barb->speed;

	/* Set the wind barb presentation units ... use default if required */
	udef = identify_unit(barb->bspec.uname);
	if (IsNull(udef)) udef = Dudef;

	/* Check if wind barb presentation matches the default wind units */
	if (same(udef->MKS, Wudef->MKS))
		{
		/* Convert to display units (if required) */
		if (!same(udef->name, Wudef->name))
			{
			(void) pr_diag("WindBarbs",
				"[display_barb_arrow] Converting from: %s  to display units: %s\n",
				udef->name, Wudef->name);
			(void) convert_value(udef->name, (double) wspd, Wudef->name, &dspd);
			wspd = (float) dspd;
			}
		}

	/* Assume that anything else is in MKS */
	else if (!same(udef->name, udef->MKS))
		{
		(void) pr_diag("WindBarbs",
			"[display_barb_arrow] Converting from: MKS  to display units: %s\n",
			udef->name);
		(void) convert_value("MKS", (double) wspd, udef->name, &dspd);
		wspd = (float) dspd;
		}

	/* Check wind value - if very small, plot a small circle */
	if (wspd < Wmin)
	    {
		/* Set up display attributes and display as a marker */
		gxSetColorIndex(barb->bspec.colour,barb->bspec.hilite);
		gxMarkerSize(0.0);
		gxMarkerAngle(0.0);
	    gxDrawMarker(MarkerCircle,barb->anchor[X],barb->anchor[Y]);
	    return;
	    }

	/* Calculate arrow size (speed) parameters */
	lenshaft = barb->bspec.length / 1000;
	if (!barb->bspec.scale) lenshaft *= gxGetMfact();
	lenhead  = hlen * lenshaft;
	widhead  = hwid * lenshaft;

	/* Calculate direction parameters */
	if ( NotNull(sfc = gxGetPatchInfo(&iu, &iv)) )
		{
		(void) patch_to_world(&sfc->sp, barb->anchor, iu, iv, pw);
		(void) pos_to_ll(&sfc->sp.mp, pw, &lat, &lon);
		angwind = RAD * (90 - wdir);
		}
	else if ( NotNull(zproj = gxGetZoomMproj())
				&& !equivalent_map_projection(zproj, &NoMapProj)
				&& NotNull(mproj = get_target_map()) )
		{
		(void) pos_to_ll(mproj, barb->anchor, &lat, &lon);
		angwind = RAD * wind_dir_xy(zproj, lat, lon, wdir);
		}
	else if ( NotNull(mproj = get_target_map())
				&& !equivalent_map_projection(mproj, &NoMapProj) )
		{
		(void) pos_to_ll(mproj, barb->anchor, &lat, &lon);
		angwind = RAD * wind_dir_xy(mproj, lat, lon, wdir);
		}
	else
		{
		lat = lon = 0.0;
		angwind = RAD * (90 - wdir);
		}
	if (!barb->bspec.sense) angwind += M_PI;
	xwind = cos(angwind);
	ywind = sin(angwind);
	xhead = ywind;
	yhead = -xwind;

	/* Set up display attributes and draw the barb */
	gxBarbSpec(&barb->bspec);

	/* Draw a marker at the anchor */
	gxSetColorIndex(barb->bspec.colour,barb->bspec.hilite);
	gxMarkerSize(0.0);
	gxMarkerAngle(0.0);
	gxDrawMarker(MarkerCircle,barb->anchor[X],barb->anchor[Y]);

	/* Draw shaft of arrow */
	xt   = barb->anchor[X] - lenshaft*xwind/2;
	yt   = barb->anchor[Y] - lenshaft*ywind/2;
	xend = xt + lenshaft*xwind;
	yend = yt + lenshaft*ywind;
	glMove(xt,yt);
	glDraw(xend,yend);

	/* Draw head on arrow */
	xt = xend - lenhead*xwind + widhead*xhead;
	yt = yend - lenhead*ywind + widhead*yhead;
	glDraw(xt,yt);
	xt = xend - lenhead*xwind - widhead*xhead;
	yt = yend - lenhead*ywind - widhead*yhead;
	glDraw(xt,yt);
	glDraw(xend,yend);
	}

void	display_barb_value

	(
	BARB	barb
	)

	{
	float	x, y, mfact, wspd;
	double	dspd;
	char	val[50];
	FpaConfigUnitStruct		*Wudef, *Dudef, *udef;

	/* Return immediately if no wind barb object */
	if (!barb) return;
	if (barb->bspec.hilite < 0) return;
	if (barb->tspec.hilite < 0) return;

	/* Check wind value */
	if (!barb->bspec.value) return;

	/* Get the default wind display parameters */
	(void) gxGetWindParms(NullFloat, NullFloat, &Wudef, &Dudef);

	/* Set wind wind speed */
	wspd = barb->speed;

	/* Set the wind barb presentation units ... use default if required */
	udef = identify_unit(barb->bspec.uname);
	if (IsNull(udef)) udef = Dudef;

	/* Check if wind barb presentation matches the default wind units */
	if (same(udef->MKS, Wudef->MKS))
		{
		/* Convert to display units (if required) */
		if (!same(udef->name, Wudef->name))
			{
			(void) pr_diag("WindBarbs",
				"[display_barb_value] Converting from: %s  to display units: %s\n",
				udef->name, Wudef->name);
			(void) convert_value(udef->name, (double) wspd, Wudef->name, &dspd);
			wspd = (float) dspd;
			}
		}

	/* Assume that anything else is in MKS */
	else if (!same(udef->name, udef->MKS))
		{
		(void) pr_diag("WindBarbs",
			"[display_barb_value] Converting from: MKS  to display units: %s\n",
			udef->name);
		(void) convert_value("MKS", (double) wspd, udef->name, &dspd);
		wspd = (float) dspd;
		}

	/* Set up display attributes */
	gxTextSpec(&barb->tspec);
	mfact = gxGetMfact() / 1000;
	x = barb->anchor[X] + mfact*barb->bspec.xvoff;
	y = barb->anchor[Y] + mfact*barb->bspec.yvoff;

	/* Display the wind text */
	(void) sprintf(val, "%d", NINT(wspd));
	glDrawString(x, y, val);
	}

/***********************************************************************
*                                                                      *
*      d i s p l a y _ b u t t o n                                     *
*                                                                      *
*      Display the given button.                                       *
*                                                                      *
***********************************************************************/

void	display_button

	(
	BUTTON	button
	)

	{
	float	lsize;
	float	bl, br, bb, bt, lx, ly;
	STRING	lab;

	/* Go home if null button */
	if (!button) return;

	/* Set up display attributes and draw the box */
	if (button->fspec.hilite < 0) return;
	if (button->lspec.hilite < 0) return;
	bl = button->box.left;
	br = button->box.right;
	bb = button->box.bottom;
	bt = button->box.top;

	/* Fill the box first */
	if (gxNeedFill(button->fspec.style))
		{
		gxFillSpec(&button->fspec, FALSE);
		glFilledRectangle(bl,bb,br,bt);
		}

	/* Then draw the outline */
	gxLineSpec(&button->lspec);
	glRectangle(bl,bb,br,bt);

	/* Set up display attributes and plot the label */
	/* Force automatic label size calculation if required */
	if (button->tspec.hilite < 0) return;
	lab = button->label;
	if (blank(lab)) return;
	lsize = button->tspec.size;
	if (lsize <= 0) define_button_pspec(button,TEXT_SIZE,&lsize);
	gxTextSpec(&button->tspec);
	lx = button->lpos[X];
	ly = button->lpos[Y];
	glDrawString(lx, ly, lab);
	}
