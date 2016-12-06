/**********************************************************************/
/** @file meta_write.c
 *
 * Routines to output the contents of a METAFILE object to a
 * metafile.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   m e t a _ w r i t e . c                                            *
*                                                                      *
*   Routines to output the contents of a METAFILE object to a          *
*   metafile.                                                          *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2002 Environment Canada (AES)            *
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

#include "meta.h"
#include "config_structs.h"
#include "config_info.h"
#include "cal.h"

#include <objects/objects.h>
#include <tools/tools.h>
#include <fpa_getmem.h>
#include <fpa_macros.h>
#include <fpa_types.h>

#include <string.h>
#include <stdio.h>
#ifdef MACHINE_PCLINUX
#include <zlib.h>
#endif

/* Local routines used for scaling */
static	void	set_meta_maxdig(int);
static	void	set_meta_scale(float);
static	float	meta_scale(float);
static	float	meta_uscale(float);
static	int		nint_scale(float);
static	int		nint_uscale(float);

/* Modes */
static	LOGICAL		OldFmt  = FALSE;
static	LOGICAL		LLmode  = FALSE;
static	MAP_PROJ	*LLproj = NullMapProj;
static	LOGICAL		DoMax   = FALSE;
/* Modes:
	x-y vs lat-lon
	x-y precision/digits/preset units
	backup
	grid vs spline
	v2 v2 v1.5

	function to set modes
	function to save/restore modes
*/

/* Local routines used for outputing metafile commands */
static	LOGICAL	put_comment(FILE *, STRING);
static	LOGICAL	put_rev(FILE *);
static	LOGICAL	put_bkgnd(FILE *, STRING);
static	LOGICAL	put_projection(FILE *, MAP_PROJ *);
static	LOGICAL	put_src_proj(FILE *, MAP_PROJ *, COMP_INFO *);
static	LOGICAL	put_field(FILE *, STRING, STRING, STRING);
static	LOGICAL	put_entity(FILE *, STRING, STRING, STRING, int, STRING *,
						STRING *);
static	LOGICAL	put_bgval(FILE *, STRING, ITEM);
static	LOGICAL	put_bspline(FILE *, SURFACE, double);
static	LOGICAL	put_bspline2D(FILE *, SURFACE, double);
static	LOGICAL	put_raster(FILE *, RASTER);
static	LOGICAL	put_mask(FILE *, BITMASK);
static	LOGICAL	put_area_bound(FILE *, AREA);
static	LOGICAL	put_area_hole(FILE *, AREA, int);
static	LOGICAL	put_area_divide(FILE *, AREA, int, SUBAREA, SUBAREA);
static	LOGICAL	put_curve(FILE *, CURVE);
static	LOGICAL	put_label(FILE *, LABEL);
static	LOGICAL	put_mark(FILE *, MARK);
static	LOGICAL	put_barb(FILE *, BARB);
static	LOGICAL	put_button(FILE *, BUTTON);
static	LOGICAL	put_spot(FILE *, SPOT);
static	LOGICAL	put_plot(FILE *, PLOT);
static	LOGICAL	put_lchain(FILE *, LCHAIN, int);
static	LOGICAL	put_lnode(FILE *, LNODE);
static	LOGICAL	put_value(FILE *, STRING, CAL);
static	LOGICAL	put_plist(FILE *, LINE);
static	LOGICAL	put_point(FILE *, POINT);
static	LOGICAL	put_point_xy(FILE *, float, float);

/***********************************************************************
*                                                                      *
*      b a c k u p _ m e t a f i l e                                   *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** If the given metafile exists, backup it up to the given
 * alternate name, then write the contents of the given METAFILE
 * to the given metafile.
 *
 *	@param[in]	name	metafile name
 *	@param[in]	bname	backup name
 *	@param[in]  meta	METAFILE object
 *	@param[in]	maxdig	maximum digits output in x-y data
 **********************************************************************/

void		backup_metafile

	(
	STRING		name,
	STRING		bname,
	METAFILE    meta,
	int			maxdig
	)

	{
	/* Do nothing if meta is Null */
	if (!meta)       return;
	if (blank(name)) return;

	/* See if the metafile exists */
	if (!blank(bname) && !same(bname, name) && find_file(name))
		{
		/* If so, copy it to the backup location */
		if (find_file(bname)) (void) unlink(bname);
		(void) link(name, bname);
		(void) unlink(name);
		}

	/* Now it is safe to write the metafile */
	write_metafile(name, meta, maxdig);
	}

/***********************************************************************
*                                                                      *
*      w r i t e _ m e t a f i l e _ s p e c i a l                     *
*      w r i t e _ m e t a f i l e                                     *
*                                                                      *
*      Write the contents of the given METAFILE to the given metafile. *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Write the contents of the given METAFILE to a given metafile.
 * You may also specify the format (mode) of the data. One of:
 * - META_LATLON
 * - META_OLDFMT
 * - META_DOMAX
 *
 *	@param[in]	name	metafile name
 *	@param[in]  meta	METAFILE object
 *	@param[in]	maxdig	maximum digits output in x-y data
 *	@param[in]	mode    The mode to write in
 **********************************************************************/
void		write_metafile_special

	(
	STRING		name,
	METAFILE    meta,
	int			maxdig,
	int			mode
	)

	{
	if (mode & META_LATLON) LLmode = True;
	if (mode & META_OLDFMT) OldFmt = True;
	if (mode & META_DOMAX)  DoMax  = True;
	write_metafile(name, meta, maxdig);
	LLmode = FALSE;
	OldFmt = FALSE;
	DoMax  = FALSE;
	}

/**********************************************************************/

/**********************************************************************/
/** Write the contents of the given METAFILE to a given metafile.
 *
 *	@param[in]	name	metafile name
 *	@param[in]  meta	METAFILE object
 *	@param[in]	maxdig	maximum digits output in x-y data
 **********************************************************************/
void		write_metafile

	(
	STRING		name,
	METAFILE    meta,
	int			maxdig
	)

	{
	FILE		*fp;
	STRING		entity, element, level, styp;
	STRING		bfile;
	int			msub, isub, lnum;
	STRING		*slist=NullStringList, *vlist=NullStringList;
	int			isrc, ifld, ilist, ihole, idiv, inode;
	MAP_PROJ	*mproj;
	PSUB		*sub;
	double		precision;

	FpaConfigElementStruct	*edef;
	FpaConfigUnitStruct		*udef;

	/* Objects found in the given METAFILE */
	FIELD	fld    = NullFld;
	SURFACE	sfc    = NullSfc;
	RASTER	rast   = NullRaster;
	SET		set    = NullSet;
	PLOT	plot   = NullPlot;
	AREA	area   = NullArea;
	BOUND	bound  = NullBound;
	SUBAREA	lsub   = NullSubArea;
	SUBAREA	rsub   = NullSubArea;
	CURVE	curve  = NullCurve;
	LABEL	label  = NullLabel;
	MARK	mark   = NullMark;
	BARB	barb   = NullBarb;
	BUTTON	button = NullButton;
	SPOT	spot   = NullSpot;
	LCHAIN	lchain = NullLchain;
	LNODE	lnode  = NullLnode;

	/* Do nothing if meta is Null */
	if (!meta) return;

	/* Do nothing if file name not given */
	if (blank(name))
		{
		(void) fprintf(stderr, "[write_metafile] No file name given\n");
		return;
		}

	/* Set output file to stdout for debugging */
	if (same(name, "DEBUG"))
		fp = stdout;

	/* Otherwise, create and open the file */
	else
		{
		fpalib_verify(FpaAccessLib);
		if (!create_file(name, NullLogicalPtr))
			{
			(void) fprintf(stderr, "[write_metafile]");
			(void) fprintf(stderr, " Cannot access or create metafile: %s\n", name);
			return;
			}
		fp = fopen(name, "w");
		if (!fp)
			{
			(void) fprintf(stderr, "[write_metafile]");
			(void) fprintf(stderr, " Cannot open metafile for writing: %s\n", name);
			return;
			}
		(void) setvbuf(fp, NullChar, _IOLBF, 0);
		}

	/* Write start-up information */
	set_meta_maxdig(maxdig);
	(void) put_comment(fp, "MSRB Metafile Standard");
	(void) put_rev(fp);
	(void) put_comment(fp, "");
	mproj = &meta->mproj;
	bfile = meta->bgndname;
	if (!blank(bfile)) (void) put_bkgnd(fp, bfile);
	(void) put_projection(fp, mproj);
	if (meta->nsrc > 0) (void) put_comment(fp, "");
	for (isrc=0; isrc<meta->nsrc; isrc++)
		{
		(void) put_src_proj(fp, meta->sproj+isrc, meta->scomp+isrc);
		}

	/* Repeat for each field in the METAFILE */
	for (ifld=0; ifld<meta->numfld; ifld++)
		{
		/* Extract field */
		fld = meta->fields[ifld];
		if (!fld) continue;

		/* Get entity, element and level */
		entity  = fld->entity;
		element = fld->element;
		level   = fld->level;
		(void) put_comment(fp, "");
		(void) put_comment(fp, element);

		/* Field contains a surface */
		if (fld->ftype == FtypeSfc)
			{
			sfc = fld->data.sfc;
			if (!sfc) continue;

			(void) put_field(fp, entity, element, level);
			msub = 0;
			(void) put_entity(fp, entity, element, level, 0,
							NullStringList, NullStringList);

			/* Compute the precision in terms equivalent MKS units */
			edef = identify_element(element);
			precision = edef->elem_io->precision;
			udef      = edef->elem_io->units;
			if (udef->factor != 1) precision /= udef->factor;

			switch (sfc->sp.dim)
				{
				case DimScalar:		(void) put_bspline(fp, sfc, precision);
									break;
				case DimVector2D:	(void) put_bspline2D(fp, sfc, precision);
									break;
				}
			}

		/* Field contains a compressed grid */
		if (fld->ftype == FtypeRaster)
			{
			rast = fld->data.raster;
			if (!rast) continue;

			(void) put_field(fp, entity, element, level);
			msub = 0;
			(void) put_entity(fp, entity, element, level, 0,
							  NullStringList, NullStringList);
			(void) put_raster(fp, rast);
			}

		/* Field contains a plot */
		else if (fld->ftype == FtypePlot)
			{
			plot = fld->data.plot;
			if (!plot) continue;
			if ((plot->nsubs <= 0) || (plot->numpts <= 0)) continue;

			(void) put_field(fp, entity, element, level);
			msub  = plot->nsubs;
			slist = INITMEM(STRING, msub);
			vlist = INITMEM(STRING, msub);
			for (isub=0; isub<msub; isub++)
				{
				sub = plot->subs + isub;
				slist[isub] = sub->name;
				vlist[isub] = sub->type;
				}
			(void) put_entity(fp, entity, element, level, msub, slist, vlist);
			(void) put_plot(fp, plot);
			}

		/* Field contains a set */
		else if (fld->ftype == FtypeSet)
			{
			set = fld->data.set;
			if (!set) continue;

			(void) put_field(fp, entity, element, level);

			if (!set->bgnd && set->num <= 0) continue;

			msub  = 2;
			slist = INITMEM(STRING, msub);
			vlist = INITMEM(STRING, msub);

			/* Output parameters based on type of SET Object */
			recall_set_type(set, &styp);
			if (same(styp, "area"))
				{
				/* If a background value is specified output that first */
				area = (AREA) set->bgnd;
				if (area)
					{
					(void) get_attribute(area->attrib, AttribCategory,  slist);
					(void) get_attribute(area->attrib, AttribAutolabel, vlist);
					(void) put_entity(fp, entity, element, level,
								1, slist, vlist);
					(void) put_bgval(fp, styp, set->bgnd);
					}
				for (ilist=0; ilist<set->num; ilist++)
					{
					area = (AREA) set->list[ilist];
					if (area && area->bound)
						{
						/* Output the boundary */
						bound = area->bound;
						if (area->numdiv<=0 && area->subareas)
							{
							(void) get_attribute(area->subareas[0]->attrib,
										AttribCategory,  slist);
							(void) get_attribute(area->subareas[0]->attrib,
										AttribAutolabel, vlist);
							}
						else
							{
							(void) get_attribute(area->attrib,
										AttribCategory,  slist);
							(void) get_attribute(area->attrib,
										AttribAutolabel, vlist);
							}
						(void) put_entity(fp, entity, element, level,
									1, slist, vlist);
						(void) put_area_bound(fp, area);

						/* Output holes */
						for (ihole=0; ihole<bound->numhole; ihole++)
							{
							(void) put_entity(fp, entity, element, level, 0,
										NullStringList, NullStringList);
							(void) put_area_hole(fp, area, ihole);
							}

						/* Output divides */
						for (idiv=0; idiv<area->numdiv; idiv++)
							{
							adjacent_subareas(area, idiv, &lsub, &rsub);
							(void) get_attribute((lsub)? lsub->attrib: NULL,
										AttribCategory,  slist);
							(void) get_attribute((lsub)? lsub->attrib: NULL,
										AttribAutolabel, vlist);
							(void) get_attribute((rsub)? rsub->attrib: NULL,
										AttribCategory,  slist+1);
							(void) get_attribute((rsub)? rsub->attrib: NULL,
										AttribAutolabel, vlist+1);
							(void) put_entity(fp, entity, element, level,
										2, slist, vlist);
							(void) put_area_divide(fp, area, idiv, lsub, rsub);
							}
						}
					}
				}

			else if (same(styp, "curve"))
				{
				/* If a background value is specified output that first */
				curve = (CURVE) set->bgnd;
				if (curve)
					{
					(void) get_attribute(curve->attrib, AttribCategory,  slist);
					(void) get_attribute(curve->attrib, AttribAutolabel, vlist);
					(void) put_entity(fp, entity, element, level,
								1, slist, vlist);
					(void) put_bgval(fp, styp, set->bgnd);
					}
				for (ilist=0; ilist<set->num; ilist++)
					{
					curve = (CURVE) set->list[ilist];
					if (curve && curve->line && (curve->line->numpts > 0))
						{
						(void) get_attribute(curve->attrib, AttribCategory,
									slist);
						(void) get_attribute(curve->attrib, AttribAutolabel,
									vlist);
						(void) put_entity(fp, entity, element, level,
									1, slist, vlist);
						(void) put_curve(fp, curve);
						}
					}
				}

			else if (same(styp, "label"))
				{
				/* If a background value is specified output that first */
				label = (LABEL) set->bgnd;
				if (label)
					{
					(void) get_attribute(label->attrib, AttribCategory,  slist);
					(void) get_attribute(label->attrib, AttribAutolabel, vlist);
					(void) put_entity(fp, entity, element, level,
								1, slist, vlist);
					(void) put_bgval(fp, styp, set->bgnd);
					}
				for (ilist=0; ilist<set->num; ilist++)
					{
					label = (LABEL) set->list[ilist];
					if (label && !blank(label->label))
						{
						(void) get_attribute(label->attrib, AttribCategory,
									slist);
						(void) get_attribute(label->attrib, AttribAutolabel,
									vlist);
						(void) put_entity(fp, entity, element, level,
									1, slist, vlist);
						(void) put_label(fp, label);
						}
					}
				}

			else if (same(styp, "mark"))
				{
				/* If a background value is specified output that first */
				mark = (MARK) set->bgnd;
				if (mark)
					{
					(void) get_attribute(mark->attrib, AttribCategory,  slist);
					(void) get_attribute(mark->attrib, AttribAutolabel, vlist);
					(void) put_entity(fp, entity, element, level,
								1, slist, vlist);
					(void) put_bgval(fp, styp, set->bgnd);
					}
				for (ilist=0; ilist<set->num; ilist++)
					{
					mark = (MARK) set->list[ilist];
					if (mark)
						{
						(void) get_attribute(mark->attrib, AttribCategory,
									slist);
						(void) get_attribute(mark->attrib, AttribAutolabel,
									vlist);
						(void) put_entity(fp, entity, element, level,
									1, slist, vlist);
						(void) put_mark(fp, mark);
						}
					}
				}

			else if (same(styp, "barb"))
				{
				/* If a background value is specified output that first */
				barb = (BARB) set->bgnd;
				if (barb)
					{
					(void) get_attribute(barb->attrib, AttribCategory,  slist);
					(void) get_attribute(barb->attrib, AttribAutolabel, vlist);
					(void) put_entity(fp, entity, element, level,
								1, slist, vlist);
					(void) put_bgval(fp, styp, set->bgnd);
					}
				for (ilist=0; ilist<set->num; ilist++)
					{
					barb = (BARB) set->list[ilist];
					if (barb)
						{
						(void) get_attribute(barb->attrib, AttribCategory,
									slist);
						(void) get_attribute(barb->attrib, AttribAutolabel,
									vlist);
						(void) put_entity(fp, entity, element, level,
									1, slist, vlist);
						(void) put_barb(fp, barb);
						}
					}
				}

			else if (same(styp, "button"))
				{
				/* If a background value is specified output that first */
				button = (BUTTON) set->bgnd;
				if (button)
					{
					(void) get_attribute(button->attrib, AttribCategory, slist);
					(void) get_attribute(button->attrib, AttribAutolabel,vlist);
					(void) put_entity(fp, entity, element, level,
								1, slist, vlist);
					(void) put_bgval(fp, styp, set->bgnd);
					}
				for (ilist=0; ilist<set->num; ilist++)
					{
					button = (BUTTON) set->list[ilist];
					if (button)
						{
						(void) get_attribute(button->attrib, AttribCategory,
									slist);
						(void) get_attribute(button->attrib, AttribAutolabel,
									vlist);
						(void) put_entity(fp, entity, element, level,
									1, slist, vlist);
						(void) put_button(fp, button);
						}
					}
				}

			else if (same(styp, "spot"))
				{
				/* If a background value is specified output that first */
				spot = (SPOT) set->bgnd;
				if (spot)
					{
					(void) get_attribute(spot->attrib, AttribCategory, slist);
					(void) get_attribute(spot->attrib, AttribAutolabel,vlist);
					(void) put_entity(fp, entity, element, level,
								1, slist, vlist);
					(void) put_bgval(fp, styp, set->bgnd);
					}
				for (ilist=0; ilist<set->num; ilist++)
					{
					spot = (SPOT) set->list[ilist];
					if (spot)
						{
						(void) get_attribute(spot->attrib, AttribCategory,
									slist);
						(void) get_attribute(spot->attrib, AttribAutolabel,
									vlist);
						(void) put_entity(fp, entity, element, level,
									1, slist, vlist);
						(void) put_spot(fp, spot);
						}
					}
				}

			else if (same(styp, "lchain"))
				{
				/* If a background value is specified output that first */
				lchain = (LCHAIN) set->bgnd;
				if (lchain)
					{
					(void) get_attribute(lchain->attrib, AttribCategory,  slist);
					(void) get_attribute(lchain->attrib, AttribAutolabel, vlist);
					(void) put_entity(fp, entity, element, level,
									1, slist, vlist);
					(void) put_bgval(fp, styp, set->bgnd);
					}
				for (ilist=0; ilist<set->num; ilist++)
					{
					lchain = (LCHAIN) set->list[ilist];
					if (valid_lchain(lchain, &lnum, NullInt))
						{
						/* Output the link chain information */
						(void) get_attribute(lchain->attrib, AttribCategory,
									slist);
						(void) get_attribute(lchain->attrib, AttribAutolabel,
									vlist);
						(void) put_entity(fp, entity, element, level,
									1, slist, vlist);
						(void) put_lchain(fp, lchain, lnum);

						/* Output link nodes */
						for (inode=0; inode<lchain->lnum; inode++)
							{
							lnode = lchain->nodes[inode];
							if (!lnode->there) continue;
							(void) get_attribute(lnode->attrib, AttribCategory,
										slist);
							(void) get_attribute(lnode->attrib, AttribAutolabel,
										vlist);
							(void) put_entity(fp, entity, element, level,
										1, slist, vlist);
							(void) put_lnode(fp, lnode);
							}
						}
					}
				}
			}

		/* Next field */
		FREEMEM(slist);
		FREEMEM(vlist);
		}

	/* Output legend */

	/* End the file */
	(void) put_comment(fp, "");
	(void) put_comment(fp, "End");

	/* Check for debugging before closing file! */
	if (!same(name, "DEBUG")) (void) fclose(fp);
	}

/***********************************************************************
*                                                                      *
*      f o r m a t _ m e t a f i l e _ p r o j e c t i o n             *
*      f o r m a t _ m e t a f i l e _ m a p d e f                     *
*                                                                      *
*      These are duplicates of code in function  put_projection()  to  *
*      replicate formatting of projection and mapdef done in function  *
*      write_metafile().                                               *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Format the metafile projection into a STRING.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	*proj	map projection for formatting
 * 	@return The metafile projection in a formatted STRING.
 **********************************************************************/
STRING		format_metafile_projection

	(
	PROJ_DEF	*proj
	)

	{
	STRING	pname, p1, p2, p3, p4, p5;

	static	char	tbuf[1024], pbuf[1024];

	if (IsNull(proj)) return NullString;

	/* Any changes to  put_projection()  must be replicated here */
	(void) projection_info(proj, &pname, &p1, &p2, &p3, &p4, &p5);
	(void) strcpy(pbuf, " projection");
	(void) sprintf(tbuf, " %s", SafeStr(pname));	(void) strcat(pbuf, tbuf);
	(void) sprintf(tbuf, " %s", SafeStr(p1));		(void) strcat(pbuf, tbuf);
	(void) sprintf(tbuf, " %s", SafeStr(p2));		(void) strcat(pbuf, tbuf);
	(void) sprintf(tbuf, " %s", SafeStr(p3));		(void) strcat(pbuf, tbuf);
	(void) sprintf(tbuf, " %s", SafeStr(p4));		(void) strcat(pbuf, tbuf);
	(void) sprintf(tbuf, " %s", SafeStr(p5));		(void) strcat(pbuf, tbuf);

	return pbuf;
	}

/**********************************************************************/

/**********************************************************************/
/** Format the metafile map definition into a STRING.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	*mdef	map definition for formatting
 *	@param[in]	maxdig	maximum digits output in x-y data
 * 	@return The map definition as a formatted STRING.
 **********************************************************************/
STRING		format_metafile_mapdef

	(
	MAP_DEF		*mdef,
	int			maxdig
	)

	{
	float	maxl;

	static	char	tbuf[1024], mbuf[1024];

	if (IsNull(mdef)) return NullString;

	set_meta_maxdig(maxdig);
	if (mdef->units > 0)
		{
		maxl = MAX(mdef->xlen, mdef->ylen);
		set_meta_scale(maxl);
		}

	/* Any changes to  put_projection()  must be replicated here */
	(void) strcpy(mbuf, " mapdef");
	(void) sprintf(tbuf, " %g", mdef->olat);		(void) strcat(mbuf, tbuf);
	(void) sprintf(tbuf, " %g", mdef->olon);		(void) strcat(mbuf, tbuf);
	(void) sprintf(tbuf, " %g", mdef->lref);		(void) strcat(mbuf, tbuf);
	(void) sprintf(tbuf, " %s",
			fformat(meta_scale(-mdef->xorg), 3));	(void) strcat(mbuf, tbuf);
	(void) sprintf(tbuf, " %s",
			fformat(meta_scale(-mdef->yorg), 3));	(void) strcat(mbuf, tbuf);
	(void) sprintf(tbuf, " %s",
			fformat(meta_scale(mdef->xlen - mdef->xorg), 3));
													(void) strcat(mbuf, tbuf);
	(void) sprintf(tbuf, " %s",
			fformat(meta_scale(mdef->ylen - mdef->yorg), 3));
													(void) strcat(mbuf, tbuf);
	(void) sprintf(tbuf, " %g",
			meta_uscale(mdef->units));				(void) strcat(mbuf, tbuf);

	return mbuf;
	}

/***********************************************************************
*                                                                      *
*      s e t _ m e t a _ m a x d i g                                   *
*      s e t _ m e t a _ s c a l e                                     *
*      m e t a _ s c a l e                                             *
*                                                                      *
***********************************************************************/

static	float	Scale   = 1.0;
static	int		MaxDig  = 0;

static	void	set_meta_maxdig

	(
	int		maxdig
	)

	{
	MaxDig = (DoMax)? maxdig: 0;
	}

static	void	set_meta_scale

	(
	float	value
	)

	{
	int		ndig;
	double	ten=10, powr;

	if (MaxDig <= 0) Scale = 1.0;
	else
		{
		ndig  = fdigit(value);
		powr  = MaxDig - ndig;
		Scale = pow(ten, powr);
		}
	}

static	float	meta_scale

	(
	float	value
	)

	{
	return value*Scale;
	}

static	float	meta_uscale

	(
	float	value
	)

	{
	return value/Scale;
	}

static	int		nint_scale

	(
	float	value
	)

	{
	value = meta_scale(value);
	return NINT(value);
	}

static	int		nint_uscale

	(
	float	value
	)

	{
	value = meta_uscale(value);
	return NINT(value);
	}

/***********************************************************************
*                                                                      *
*      p u t _ c o m m e n t                                           *
*      p u t _ r e v                                                   *
*      p u t _ b k g n d                                               *
*      p u t _ r a s t e r                                             *
*      p u t _ p r o j e c t i o n                                     *
*      p u t _ e n t i t y                                             *
*      p u t _ b g v a l                                               *
*      p u t _ b s p l i n e                                           *
*      p u t _ b s p l i n e 2 D                                       *
*      p u t _ a r e a _ b o u n d                                     *
*      p u t _ a r e a _ h o l e                                       *
*      p u t _ a r e a _ d i v i d e                                   *
*      p u t _ c u r v e                                               *
*      p u t _ l a b e l                                               *
*      p u t _ m a r k                                                 *
*      p u t _ b a r b                                                 *
*      p u t _ b u t t o n                                             *
*      p u t _ s p o t                                                 *
*      p u t _ p l o t                                                 *
*      p u t _ l c h a i n                                             *
*      p u t _ l n o d e                                               *
*      p u t _ l c o n t                                               *
*                                                                      *
*      Routines to handle writing graphics primitives to a metafile.   *
*                                                                      *
***********************************************************************/

static	const	STRING	Blank  = "\"\"";
static	const	STRING	Rev    = "3.0";
static	const	STRING	OldRev = "1.5";

static LOGICAL	put_comment

	(
	FILE	*fp,
	STRING	text
	)

	{
	(void) fprintf(fp, "*");
	if (text) (void) fprintf(fp, " %s", text);
	(void) fprintf(fp, "\n");
	return TRUE;
	}

static LOGICAL	put_rev

	(
	FILE	*fp
	)

	{
	(void) fprintf(fp, " rev %s", (OldFmt)? OldRev: Rev);
	(void) fprintf(fp, "\n");
	return TRUE;
	}

static LOGICAL	put_bkgnd

	(
	FILE	*fp,
	STRING	file
	)

	{
	if (!file) return FALSE;
	if (!strlen(file)) return FALSE;
	(void) fprintf(fp, " bkgnd %s\n", file);
	return TRUE;
	}

static LOGICAL		put_projection

	(
	FILE		*fp,
	MAP_PROJ	*mproj
	)

	{
	float	maxl;
	STRING	pname, p1, p2, p3, p4, p5;
	MAP_DEF	*mdef;

	/* Any changes here must be replicated in  format_metafile_projection() */
	(void) projection_info(&mproj->projection, &pname, &p1, &p2, &p3, &p4, &p5);
	(void) fprintf(fp, " projection");
	(void) fprintf(fp, " %s", SafeStr(pname));
	(void) fprintf(fp, " %s", SafeStr(p1));
	(void) fprintf(fp, " %s", SafeStr(p2));
	(void) fprintf(fp, " %s", SafeStr(p3));
	(void) fprintf(fp, " %s", SafeStr(p4));
	(void) fprintf(fp, " %s", SafeStr(p5));
	(void) fprintf(fp, "\n");

	mdef = &(mproj->definition);
	if (mdef->units > 0)
		{
		maxl = MAX(mdef->xlen, mdef->ylen);
		set_meta_scale(maxl);
		}

	/* Any changes here must be replicated in  format_metafile_mapdef() */
	(void) fprintf(fp, " mapdef");
	(void) fprintf(fp, " %g", mdef->olat);
	(void) fprintf(fp, " %g", mdef->olon);
	(void) fprintf(fp, " %g", mdef->lref);
	(void) fprintf(fp, " %s", fformat(meta_scale(-mdef->xorg), 3));
	(void) fprintf(fp, " %s", fformat(meta_scale(-mdef->yorg), 3));
	(void) fprintf(fp, " %s", fformat(meta_scale(mdef->xlen - mdef->xorg), 3));
	(void) fprintf(fp, " %s", fformat(meta_scale(mdef->ylen - mdef->yorg), 3));
	(void) fprintf(fp, " %g", meta_uscale(mdef->units));
	(void) fprintf(fp, "\n");

	if (LLmode) LLproj = mproj;
	if (LLmode) (void) fprintf(fp, " units latlon");
	else        (void) fprintf(fp, " units xy");
	(void) fprintf(fp, "\n");
	return TRUE;
	}

static LOGICAL		put_src_proj

	(
	FILE		*fp,
	MAP_PROJ	*mproj,
	COMP_INFO	*cinfo
	)

	{
	STRING	pname, p1, p2, p3, p4, p5;
	MAP_DEF	*mdef;

	(void) projection_info(&mproj->projection, &pname, &p1, &p2, &p3, &p4, &p5);
	(void) fprintf(fp, " source_projection");
	(void) fprintf(fp, " %s", SafeStr(pname));
	(void) fprintf(fp, " %s", SafeStr(p1));
	(void) fprintf(fp, " %s", SafeStr(p2));
	(void) fprintf(fp, " %s", SafeStr(p3));
	(void) fprintf(fp, " %s", SafeStr(p4));
	(void) fprintf(fp, " %s", SafeStr(p5));
	(void) fprintf(fp, "\n");

	mdef = &(mproj->definition);
	(void) fprintf(fp, " source_mapdef");
	(void) fprintf(fp, " %g", mdef->olat);
	(void) fprintf(fp, " %g", mdef->olon);
	(void) fprintf(fp, " %g", mdef->lref);
	(void) fprintf(fp, " %s", fformat(-mdef->xorg, 3));
	(void) fprintf(fp, " %s", fformat(-mdef->yorg, 3));
	(void) fprintf(fp, " %s", fformat(mdef->xlen - mdef->xorg, 3));
	(void) fprintf(fp, " %s", fformat(mdef->ylen - mdef->yorg, 3));
	(void) fprintf(fp, " %g", mdef->units);
	(void) fprintf(fp, "\n");

	if (!ready_components(cinfo))
		{
		(void) fprintf(fp, " source_component ");
		if (cinfo->have & X_Comp) (void) fprintf(fp, "x");
		if (cinfo->have & Y_Comp) (void) fprintf(fp, "y");
		(void) fprintf(fp, "\n");
		}
	return TRUE;
	}

static LOGICAL	put_field

	(
	FILE	*fp,
	STRING	entity,
	STRING	element,
	STRING	level
	)

	{
	STRING	type;

	if (OldFmt)         return TRUE;

	if (blank(entity))  return FALSE;
	if (blank(element)) element = Blank;
	if (blank(level))   level   = Blank;

	switch (entity[0])
		{
		case 'v':	type = "vector"; break;
		case 'a':	type = "continuous"; break;
		case 'b':	type = "discrete"; break;
		case 'c':	type = "line"; break;
		case 'd':	type = "scattered"; break;
		case 'l':	type = "lchain"; break;
		default:	return FALSE;
		}
	(void) fprintf(fp, " field");
	(void) fprintf(fp, " %s", type);
	(void) fprintf(fp, " %s", element);
	(void) fprintf(fp, " %s", level);
	(void) fprintf(fp, "\n");
	return TRUE;
	}

static LOGICAL	put_entity

	(
	FILE	*fp,
	STRING	entity,
	STRING	element,
	STRING	level,
	int		nsub,
	STRING	*subelems,
	STRING	*values
	)

	{
	int		isub;
	STRING	sub, val;

	if (!OldFmt)        return TRUE;

	if (blank(entity))  return FALSE;
	if (blank(element)) element = Blank;
	if (blank(level))   level   = Blank;
	(void) fprintf(fp, " entity");
	(void) fprintf(fp, "-%s", entity);
	(void) fprintf(fp, " %s", element);
	(void) fprintf(fp, " %s", level);
	for (isub=0; isub<nsub; isub++)
		{
		sub = subelems[isub];	if (blank(sub)) sub = Blank;
		val = values[isub];
		(void) fprintf(fp, " %s",     sub);
		(void) fprintf(fp, " \"%s\"", SafeStr(val));
		}
	(void) fprintf(fp, "\n");
	return TRUE;
	}

static LOGICAL	put_bgval

	(
	FILE	*fp,
	STRING	type,
	ITEM	bgnd
	)

	{
	CAL		cal;
	STRING	bglab;

	if (IsNull(bgnd))              return FALSE;
	if (blank(type))               return FALSE;
	else if (same(type, "area"))   cal = ((AREA)bgnd)->attrib;
	else if (same(type, "barb"))   cal = ((BARB)bgnd)->attrib;
	else if (same(type, "button")) cal = ((BUTTON)bgnd)->attrib;
	else if (same(type, "curve"))  cal = ((CURVE)bgnd)->attrib;
	else if (same(type, "label"))  cal = ((LABEL)bgnd)->attrib;
	else if (same(type, "mark"))   cal = ((MARK)bgnd)->attrib;
	else if (same(type, "spot"))   cal = ((SPOT)bgnd)->attrib;
	else                           return FALSE;
	if (IsNull(cal))               return FALSE;

	if (OldFmt)
		{
		bglab = CAL_get_attribute(cal, CALuserlabel);
		(void) fprintf(fp, " bgval \"%s\"\n", SafeStr(bglab));
		}
	else
		{
		(void) put_value(fp, "bgvalue", cal);
		}
	return TRUE;
	}

static LOGICAL	put_bspline

	(
	FILE	*fp,
	SURFACE	sfc,
	double	precision
	)

	{
	float	*vbuf=NullFloat;
	int		m, n, np, ip, nl;
	double	val, factor, offset, prec, plog;

	if (!sfc) return FALSE;
	if (sfc->sp.dim != DimScalar) return FALSE;
	m = sfc->sp.m;
	n = sfc->sp.n;
	factor  = sfc->units.factor;
	offset  = sfc->units.offset;
	prec    = fabs(precision);
	plog    = log10(prec);
	plog    = floor(plog + 0.01);
	prec    = pow(10.0, plog);
	factor *= prec;

	(void) fprintf(fp, " bspline");
	(void) fprintf(fp, " %g", prec);
	(void) fprintf(fp, " %d", m);
	(void) fprintf(fp, " %d", n);
	(void) put_point(fp, sfc->sp.origin);
	(void) fprintf(fp, " %s", fformat(sfc->sp.orient, 2));
	(void) fprintf(fp, " %s", fformat(meta_scale(sfc->sp.gridlen), 3));

	np   = m*n;
	nl   = MIN(n, 10);
	vbuf = sfc->sp.cvs[0];
	for (ip=0; ip<np; ip++)
		{
		if (ip%nl == 0) (void) fprintf(fp, "\n");
		val = vbuf[ip];
		/* Convert to MKS units at desired precision */
		if (offset != 0) val -= offset;
		if (factor != 1) val /= factor;
		(void) fprintf(fp, " %d", NINT(val));
		}
	(void) fprintf(fp, "\n");
	return TRUE;
	}

static LOGICAL	put_bspline2D

	(
	FILE	*fp,
	SURFACE	sfc,
	double	precision
	)

	{
	float	*vbuf=NullFloat;
	int		m, n, np, ip, nl;
	double	val, factor, offset, prec, plog;

	if (!sfc) return FALSE;
	if (sfc->sp.dim != DimVector2D) return FALSE;
	m = sfc->sp.m;
	n = sfc->sp.n;
	factor  = sfc->units.factor;
	offset  = sfc->units.offset;
	prec    = fabs(precision);
	plog    = log10(prec);
	plog    = floor(plog + 0.01);
	prec    = pow(10.0, plog);
	factor *= prec;

	(void) fprintf(fp, " bspline2D");
	(void) fprintf(fp, " map");
	(void) fprintf(fp, " block");
	(void) fprintf(fp, " %g", prec);
	(void) fprintf(fp, " %d", m);
	(void) fprintf(fp, " %d", n);
	(void) put_point(fp, sfc->sp.origin);
	(void) fprintf(fp, " %s", fformat(sfc->sp.orient, 2));
	(void) fprintf(fp, " %s", fformat(meta_scale(sfc->sp.gridlen), 3));

	np   = m*n;
	nl   = MIN(n, 10);
	vbuf = sfc->sp.cvx[0];
	for (ip=0; ip<np; ip++)
		{
		if (ip%nl == 0) (void) fprintf(fp, "\n");
		val = vbuf[ip];
		/* Convert to MKS units at desired precision */
		if (offset != 0) val -= offset;
		if (factor != 1) val /= factor;
		(void) fprintf(fp, " %d", NINT(val));
		}
	np   = m*n;
	nl   = MIN(n, 10);
	vbuf = sfc->sp.cvy[0];
	for (ip=0; ip<np; ip++)
		{
		if (ip%nl == 0) (void) fprintf(fp, "\n");
		val = vbuf[ip];
		/* Convert to MKS units at desired precision */
		if (offset != 0) val -= offset;
		if (factor != 1) val /= factor;
		(void) fprintf(fp, " %d", NINT(val));
		}
	(void) fprintf(fp, "\n");
	return TRUE;
	}

static LOGICAL  put_mask

	(
	FILE	*fp,
	BITMASK mask
	)

	{
#ifdef MACHINE_PCLINUX
	unsigned long	destsize;
	Byte	*destbuf;

	if (!mask)
		{
		/* (void) fprintf(fp, " mask 0 0\n"); */
		return TRUE;	/* It's okay if there isn't a mask */
		}
	destsize = ceil(mask->size*1.001 + 12);
	destbuf  = INITMEM(Byte, destsize);
	if ( Z_OK != compress(destbuf, &destsize, (Byte*)mask->bits, mask->size) )
		{
		FREEMEM(destbuf);
		return FALSE;
		}

	/* mask	destsize masksize destbuf */
	(void) fprintf(fp, " mask");
	(void) fprintf(fp, " %ld",   destsize);
	(void) fprintf(fp, " %ld\n", mask->size);
	(void) fwrite((void *)destbuf, 1, destsize, fp);
	(void) fprintf(fp, "\n");

	/* All done */
	FREEMEM(destbuf);
	return TRUE;
#else
	pr_error("Metafile","Writing zlib compressed data is not available on this machine.\n");
	return FALSE;
#endif
	}

static LOGICAL	put_raster

	(
	FILE	*fp,
	RASTER	rast
	)

	{
#ifdef MACHINE_PCLINUX
	long 	pngsize, nrow, ncol, bpp, gridsize;
	RASTER_TYPE type;
	UNCHAR *pngbuf, *init, *grid;			/* Buffer for PNG stream in memory */
	BITMASK mask;


	if (!rast) return FALSE;
	if (!recall_raster_info(rast, &nrow, &ncol, &bpp, &type, &init, &grid, &mask))
		return FALSE;
	if ( !grid )	return FALSE;

	gridsize = nrow * ncol * bpp;

	/* If there is a mask put it first */
	if ( !put_mask(fp, mask) )	return FALSE;

	/* Convert to MKS units */
	change_raster_units(rast, &MKS_UNITS);

	/* Create PNG STREAM in memory */
	pngbuf   = INITMEM(UNCHAR, gridsize);
	pngsize  = write_png(grid, ncol, nrow, bpp*8, pngbuf);
	if ( pngsize < 0 )
		{
		FREEMEM(pngbuf);
		return FALSE;
		}

	(void) fprintf(fp, " raster");
	(void) fprintf(fp, " %ld", pngsize);
	(void) fprintf(fp, " %ld", nrow);
	(void) fprintf(fp, " %ld", ncol);
	(void) fprintf(fp, " %ld", bpp);
	(void) fprintf(fp, " %d", type);
	if (init)
		(void) fprintf(fp, " %f", *(float *)(init));
	(void) fprintf(fp, "\n");
	(void) fwrite((void *)pngbuf, 1, pngsize, fp);
	(void) fprintf(fp, "\n");

	FREEMEM(pngbuf);
	return TRUE;
#else
	pr_error("Metafile","Writing PNG images is not available on this machine.\n");
	return FALSE;
#endif
	}

static LOGICAL	put_area_bound

	(
	FILE	*fp,
	AREA	area
	)

	{
	LINE	seg;

	if (!area)            return FALSE;
	if (!area->bound)     return FALSE;
	seg = area->bound->boundary;
	if (!seg)             return FALSE;
	if (seg->numpts <= 0) return FALSE;

	if (area->numdiv<=0 && area->subareas)
		 (void) put_value(fp, "value", area->subareas[0]->attrib);
	else (void) put_value(fp, "value", area->attrib);

	(void) fprintf(fp, " area boundary");
	if (OldFmt)
		{
		if (area->numdiv<=0 && area->subareas)
			 (void) fprintf(fp, " \"%s\"", SafeStr(area->subareas[0]->label));
		else (void) fprintf(fp, " \"%s\"", SafeStr(area->label));
		}
	(void) put_plist(fp, seg);
	return TRUE;
	}

static LOGICAL	put_area_hole

	(
	FILE	*fp,
	AREA	area,
	int		ihole
	)

	{
	LINE	seg;

	if (!area)                         return FALSE;
	if (!area->bound)                  return FALSE;
	if (ihole < 0)                     return FALSE;
	if (ihole >= area->bound->numhole) return FALSE;
	seg = area->bound->holes[ihole];
	if (!seg)                          return FALSE;
	if (seg->numpts <= 0)              return FALSE;

	(void) fprintf(fp, " area hole");
	(void) put_plist(fp, seg);
	return TRUE;
	}

static LOGICAL	put_area_divide

	(
	FILE	*fp,
	AREA	area,
	int		idiv,
	SUBAREA	lsub,
	SUBAREA	rsub
	)

	{
	LINE	seg;

	if (!area)                return FALSE;
	if (!area->divlines)      return FALSE;
	if (idiv < 0)             return FALSE;
	if (idiv >= area->numdiv) return FALSE;
	seg = area->divlines[idiv];
	if (!seg)                 return FALSE;
	if (seg->numpts <= 0)     return FALSE;

	(void) put_value(fp, "lvalue", ((lsub)? lsub->attrib: NullCal));
	(void) put_value(fp, "rvalue", ((rsub)? rsub->attrib: NullCal));

	(void) fprintf(fp, " area divide");
	if (OldFmt)
		{
		(void) fprintf(fp, " \"%s\"", ((lsub)? SafeStr(lsub->label): ""));
		(void) fprintf(fp, " \"%s\"", ((rsub)? SafeStr(rsub->label): ""));
		}
	(void) put_plist(fp, seg);
	return TRUE;
	}

static LOGICAL	put_curve

	(
	FILE	*fp,
	CURVE	curv
	)

	{
	LINE	seg;

	if (!curv)            return FALSE;
	seg = curv->line;
	if (!seg)             return FALSE;
	if (seg->numpts <= 0) return FALSE;

	(void) put_value(fp, "value", curv->attrib);

	(void) fprintf(fp, " curve");
	(void) fprintf(fp, " %c", curv->sense);
	if (OldFmt) (void) fprintf(fp, " \"%s\"", SafeStr(curv->label));
	(void) put_plist(fp, seg);
	return TRUE;
	}

static LOGICAL	put_label

	(
	FILE	*fp,
	LABEL	labl
	)

	{
	if (!labl) return FALSE;

	(void) put_value(fp, "value", labl->attrib);

	(void) fprintf(fp, " label");
	(void) fprintf(fp, " %s", fformat(labl->angle, 2));
	(void) put_point(fp, labl->anchor);
	(void) fprintf(fp, " \"%s\"", SafeStr(labl->label));
	(void) fprintf(fp, "\n");
	return TRUE;
	}

static LOGICAL	put_mark

	(
	FILE	*fp,
	MARK	mark
	)

	{
	if (!mark) return FALSE;

	(void) put_value(fp, "value", mark->attrib);

	(void) fprintf(fp, " mark");
	(void) fprintf(fp, " %s", fformat(mark->angle, 2));
	(void) put_point(fp, mark->anchor);
	(void) fprintf(fp, " %d", mark->mspec.type);
	if (OldFmt) (void) fprintf(fp, " \"%s\"", SafeStr(mark->label));
	(void) fprintf(fp, "\n");
	return TRUE;
	}

static LOGICAL	put_barb

	(
	FILE	*fp,
	BARB	barb
	)

	{
	if (!barb) return FALSE;

	(void) put_value(fp, "value", barb->attrib);

	(void) fprintf(fp, " barb");
	(void) put_point(fp, barb->anchor);
	(void) fprintf(fp, " %g", barb->dir);
	(void) fprintf(fp, " %g", barb->speed);
	(void) fprintf(fp, " %g", barb->gust);
	(void) fprintf(fp, " %d", 1);
	if (OldFmt) (void) fprintf(fp, " \"%s\"", SafeStr(barb->label));
	(void) fprintf(fp, "\n");
	return TRUE;
	}

static LOGICAL	put_button

	(
	FILE	*fp,
	BUTTON	button
	)

	{
	if (!button) return FALSE;

	(void) put_value(fp, "value", button->attrib);

	(void) fprintf(fp, " button");
	(void) put_point_xy(fp, button->box.left, button->box.bottom);
	(void) put_point_xy(fp, button->box.right, button->box.top);
	if (OldFmt) (void) fprintf(fp, " \"%s\"", SafeStr(button->label));
	(void) fprintf(fp, "\n");
	return TRUE;
	}

static LOGICAL	put_spot

	(
	FILE	*fp,
	SPOT	spot
	)

	{
	if (!spot) return FALSE;

	(void) put_value(fp, "value", spot->attrib);

	(void) fprintf(fp, " spot");
	(void) put_point(fp, spot->anchor);
	(void) fprintf(fp, " \"%s\"", spot->mclass);
	(void) fprintf(fp, " %s", spot_feature_string(spot->feature));
	(void) fprintf(fp, "\n");
	return TRUE;
	}

static LOGICAL	put_plot

	(
	FILE	*fp,
	PLOT	plot
	)

	{
	int		isub, ip;
	PSUB	*sub;

	if (!OldFmt)
		{
		(void) fprintf(fp, " subfields");
		(void) fprintf(fp, " %d", plot->nsubs);
		(void) fprintf(fp, "\n");
		for (isub=0; isub<plot->nsubs; isub++)
			{
			sub = plot->subs + isub;
			(void) fprintf(fp, "   %s", sub->name);
			(void) fprintf(fp, " \"%s\"", sub->type);
			(void) fprintf(fp, "\n");
			}
		}

	for (ip=0; ip<plot->numpts; ip++)
		{
		(void) fprintf(fp, " plot");
		(void) put_point(fp, plot->pts[ip]);
		for (isub=0; isub<plot->nsubs; isub++)
			{
			sub = plot->subs + isub;
			if (same(sub->type, "area"))
				{
				(void) fprintf(fp, " \"%s\"", SafeStr(sub->sval1[ip]));
				}
			else if (same(sub->type, "curve"))
				{
				(void) fprintf(fp, " \"%s\"", SafeStr(sub->sval1[ip]));
				}
			else if (same(sub->type, "label"))
				{
				(void) fprintf(fp, " \"%s\"", SafeStr(sub->sval1[ip]));
				}
			else if (same(sub->type, "mark"))
				{
				(void) fprintf(fp, " \"%s\"", SafeStr(sub->sval1[ip]));
				}
			else if (same(sub->type, "barb"))
				{
				(void) fprintf(fp, " %g", sub->fval1[ip]); /*dir*/
				(void) fprintf(fp, " %g", sub->fval2[ip]); /*spd*/
				}
			else if (same(sub->type, "button"))
				{
				(void) fprintf(fp, " \"%s\"", SafeStr(sub->sval1[ip]));
				}
			else if (same(sub->type, "int"))
				{
				(void) fprintf(fp, " %d", sub->ival1[ip]);
				}
			else if (same(sub->type, "float"))
				{
				(void) fprintf(fp, " %g", sub->fval1[ip]);
				}
			}
		(void) fprintf(fp, "\n");
		}
	return TRUE;
	}

static LOGICAL	put_lchain

	(
	FILE	*fp,
	LCHAIN	lchain,
	int		lnum
	)

	{
	if (!lchain) return FALSE;

	(void) put_value(fp, "value", lchain->attrib);

	(void) fprintf(fp, " lchain");
	(void) fprintf(fp, " \"%s\"", lchain->xtime);
	(void) fprintf(fp, " %d",     lchain->splus);
	(void) fprintf(fp, " %d",     lchain->eplus);
	(void) fprintf(fp, " %d",     lchain->minterp);
	(void) fprintf(fp, " %d",     lnum);
	(void) fprintf(fp, "\n");
	return TRUE;
	}

static LOGICAL	put_lnode

	(
	FILE	*fp,
	LNODE	lnode
	)

	{
	if (!lnode) return FALSE;

	(void) put_value(fp, "nvalue", lnode->attrib);

	(void) fprintf(fp, " node");
	(void) put_point(fp, lnode->node);
	if (lnode->ltype == LchainNode)
		{
		if (lnode->guess) (void) fprintf(fp, " %s", FpaNodeClass_NormalGuess);
		else              (void) fprintf(fp, " %s", FpaNodeClass_Normal);
		}
	else if (lnode->ltype == LchainControl)
		{
		if (lnode->guess) (void) fprintf(fp, " %s", FpaNodeClass_ControlGuess);
		else              (void) fprintf(fp, " %s", FpaNodeClass_Control);
		}
	else if (lnode->ltype == LchainFloating)
		{
		(void) fprintf(fp, " %s", FpaNodeClass_Floating);
		}
	(void) fprintf(fp, " %d", lnode->mplus);
	(void) fprintf(fp, " %d", lnode->attach);
	(void) fprintf(fp, " %d", lnode->mtype);
	(void) fprintf(fp, " %d", lnode->imem);
	(void) fprintf(fp, "\n");
	return TRUE;
	}

static LOGICAL	put_value

	(
	FILE	*fp,
	STRING	type,
	CAL		cal
	)

	{
	int		ia, na, nn;
	STRING	*names, name, val;

	if (OldFmt)      return TRUE;
	if (IsNull(cal)) return FALSE;

	CAL_get_attribute_names(cal, &names, &na);
	if (na <= 0 || IsNull(names)) return FALSE;

	/* Figure out which attributes are assigned */
	nn = na;
	for (ia=0; ia<na; ia++)
		{
		name = names[ia];
		if (same_ic(name, CALlatitude) || same_ic(name, CALlongitude))
			{
			nn--;
			continue;
			}
		if (same_ic(name, CALproximity))
			{
			nn--;
			continue;
			}
		val = CAL_get_attribute(cal, name);
		if (CAL_no_value(val))
			{
			nn--;
			continue;
			}
		}

	(void) fprintf(fp, " %s", type);
	(void) fprintf(fp, " %d", nn);
	(void) fprintf(fp, "\n");
	for (ia=0; ia<na; ia++)
		{
		name = names[ia];
		if (same_ic(name, CALlatitude))  continue;
		if (same_ic(name, CALlongitude)) continue;
		if (same_ic(name, CALproximity)) continue;
		val = CAL_get_attribute(cal, name);
		if (CAL_no_value(val)) continue;

		(void) fprintf(fp, "   %s", name);
		(void) fprintf(fp, " \"%s\"", val);
		(void) fprintf(fp, "\n");
		}
	FREEMEM(names);
	return TRUE;
	}

static LOGICAL	put_plist

	(
	FILE	*fp,
	LINE	plist
	)

	{
	int		np, ip;

	if (!plist)  return FALSE;
	np = plist->numpts;
	if (np <= 0) return FALSE;

	(void) fprintf(fp, " %d", np);
	for (ip=0; ip<np; ip++)
		{
		if (ip%7 == 0) (void) fprintf(fp, "\n");
		(void) put_point(fp, plist->points[ip]);
		}
	(void) fprintf(fp, "\n");

	return TRUE;
	}

static LOGICAL	put_point

	(
	FILE	*fp,
	POINT	point
	)

	{
	return put_point_xy(fp, point[X], point[Y]);
	}

static LOGICAL	put_point_xy

	(
	FILE	*fp,
	float	x,
	float	y
	)

	{
	float	lat, lon;

	if (LLmode)
		{
		pos_to_ll(LLproj, make_point(x, y), &lat, &lon);
		(void) fprintf(fp, " %g", lat);
		(void) fprintf(fp, " %g", lon);
		}

	else
		{
		(void) fprintf(fp, " %d", nint_scale(x));
		(void) fprintf(fp, " %d", nint_scale(y));
		}

	return TRUE;
	}
