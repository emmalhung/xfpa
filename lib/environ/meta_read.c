/**********************************************************************/
/** @file meta_read.c
 *
 * Routines to handle reading the contents of a metafile.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   m e t a _ r e a d . c                                              *
*                                                                      *
*   Routines to handle reading the contents of a metafile.             *
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

#undef DEBUG

#include "cal.h"
#include "calculation.h"
#include "rules.h"
#include "meta.h"
#include "config_structs.h"
#include "config_info.h"

#include <objects/objects.h>
#include <tools/tools.h>
#include <fpa_types.h>
#include <fpa_getmem.h>

#include <string.h>
#include <stdio.h>
#ifdef MACHINE_PCLINUX
#include <zlib.h>
#endif

/* Routines used for re-mapping */
static	MAP_PROJ	*reset_projection(void);
static	MAP_PROJ	*projection_basemap(const MAP_PROJ *);
static	MAP_PROJ	*projection_newmap(const MAP_PROJ *);
static	void		projection_units(STRING, STRING);
static	LOGICAL		meta_project(STRING, STRING, POINT);
static	LOGICAL		meta_point(STRING, STRING, POINT);
static	void		start_wrap_detect(void);
static	void		end_wrap_detect(void);
static	void		detect_wrap(float);
static	LOGICAL		wrap_detected(void);

/* Routines used for revision control */
static	void	set_revision_info(STRING);
static	LOGICAL	NotMetafile;
static	LOGICAL	UseMKS;
static	LOGICAL	UseLabels;
static	LOGICAL	UseOffset;
static	LOGICAL	UseProj;
static	LOGICAL	UseArea;
static	LOGICAL	UseCal;
static	LOGICAL	UseGust;

/* Routines used for converting between structures */
static	void	surface_to_MKS(SURFACE, STRING, STRING);
#ifdef MAYBE_LATER
static	SET		areas_from_curves(METAFILE, STRING, STRING, STRING);
#endif /* MAYBE_LATER */

/* Routines used for reading metafile commands */
static	LOGICAL	get_rev(FILE *, STRING);
static	LOGICAL	get_projection(FILE *, MAP_PROJ *);
static	LOGICAL	get_mapdef(FILE *, MAP_PROJ *);
static	LOGICAL	get_component(FILE *, COMP_INFO *);
static	LOGICAL	get_units(FILE *, STRING, STRING);
static	LOGICAL	get_info(FILE *, STRING, STRING, STRING *);
static	LOGICAL	get_bkgnd(FILE *, STRING);
static	LOGICAL	get_tstamp(FILE *, STRING, STRING, STRING);
static	LOGICAL	get_field(FILE *, STRING, STRING, STRING);
static	LOGICAL	get_value(FILE *, STRING, STRING, STRING, CAL *);
static	LOGICAL	get_entity(FILE *, STRING, STRING, CAL *, int *, STRING **,
							STRING **);
static	LOGICAL	get_subfields(FILE *, int *, STRING **, STRING **);
static	LOGICAL	get_bgval(FILE *, CAL);
static	LOGICAL	get_bspline(FILE *, SURFACE *);
static	LOGICAL	get_bspline2D(FILE *, SURFACE *);
static	LOGICAL	get_grid(FILE *, SURFACE *);
static	LOGICAL	get_grid2D(FILE *, SURFACE *);
static	LOGICAL	get_raster(FILE *, RASTER *, BITMASK);
static	LOGICAL	get_mask(FILE *, BITMASK *);
static	LOGICAL	get_curve(FILE *, CURVE *, CAL);
static	LOGICAL	get_curves(FILE *, SET *, CAL);
static	LOGICAL	get_label(FILE *, LABEL *, CAL);
static	LOGICAL	get_mark(FILE *, MARK *, CAL);
static	LOGICAL	get_barb(FILE *, BARB *, CAL);
static	LOGICAL	get_button(FILE *, BUTTON *, CAL);
static	LOGICAL	get_spot(FILE *, STRING, STRING, SPOT *, CAL);
static	LOGICAL	get_areaset_area(FILE *, SET, CAL);
static	LOGICAL	get_areaset_hole(FILE *, SET, CAL);
static	LOGICAL	get_areaset_divide(FILE *, SET, CAL, CAL);
static	LOGICAL	get_plot_point(FILE *, PLOT);
static	LOGICAL	get_plot_point_as_spot(FILE *, STRING, STRING,
							SPOT *, CAL, STRING *, STRING *, int);
static	LOGICAL	get_plot_point_as_both(FILE *, STRING, STRING, PLOT,
							SPOT *, CAL, STRING *, STRING *, int);
static	LOGICAL	get_lchain(FILE *, SET, CAL, int *);
static	LOGICAL	get_lchain_node(FILE *, SET, CAL, int *);
static	LOGICAL	get_old_area(FILE *, SET, CAL);

static	LOGICAL	BailOut = FALSE;

/***********************************************************************
*                                                                      *
*      s e t _ m e t a f i l e _ i n p u t _ m o d e                   *
*                                                                      *
***********************************************************************/

static	LOGICAL	RetainPlot    = TRUE;
static	LOGICAL	RasterReadAll = TRUE;

/**********************************************************************/
/** Set the metafile input mode.
 *
 *	@param[in]	mode 	one of RetainPlot or NoRetainPlot
 *						 or RasterReadAll or RasterReadMetadata
 **********************************************************************/
void	set_metafile_input_mode
	(
	STRING	mode
	)

	{
	if      (same_ic(mode, MetaRetainPlot))   RetainPlot = TRUE;
	else if (same_ic(mode, MetaNoRetainPlot)) RetainPlot = FALSE;

	else if (same_ic(mode, MetaRasterReadAll))      RasterReadAll = TRUE;
	else if (same_ic(mode, MetaRasterReadMetadata)) RasterReadAll = FALSE;

	else
		{
		(void) pr_error("Metafile",
			"Unrecognized metafile input mode: %s\n", mode);
		}
	}

/***********************************************************************
*                                                                      *
*      r e a d _ m e t a f i l e                                       *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Read the contents of the given metafile and store the
 * graphics information in a new METAFILE object.
 *
 * Metafiles must now conform to the published MSRB graphics data
 * exchange metafile standard, version 1.1.  Older versions must
 * be read with read_old_metafile, but this will not be supported
 * forever.
 *
 * An optional base map definition may be supplied.  If so, the
 * data in the metafile are automatically transformed into the
 * base map co-ordinate system.
 *
 * The data read from the metafile contains only the actual data
 * and no information on how to display the data.  This is quite
 * sufficient if you only intend to evaluate (sample) the data.
 * If you need to display the data, presentation information must
 * be added (see presentation configuration).
 *
 *	@param[in]	name	metafile name
 *	@param[in]	*bproj	base map definition to transform to
 * 	@return A copy of the metafile information.
 **********************************************************************/

METAFILE	read_metafile

	(
	STRING			name,
	const MAP_PROJ	*bproj
	)

	{
	FILE		*fp;
	MAP_PROJ	*pproj, mproj, sproj;
	COMP_INFO	scinfo;
	PROJ_DEF	pdef;
	MAP_DEF		mdef;
	METAFILE	meta;
	char		cmd[21], file[128], utype[21], umode[21];
	char		rtime[41], vtime[41], model[81];
	char		entity[41], element[41], level[41];
	STRING		vtr, subptr, valptr, labptr;
	STRING		c, *slist, *vlist, rev;
	CAL			cal  = NullCal;
	CAL			lcal = NullCal;
	CAL			rcal = NullCal;
	CAL			ncal = NullCal;
	int			nsub;
	int			freset = TRUE;
	int			mreset = TRUE;
	int			lnum = 0;
	float		x, y, dx, dy;
	FIELD		fld;
	SURFACE		sfc;
	RASTER		raster=NullRaster;
	BITMASK		mask=NullBitMask;
	SET			set, cset;
	PLOT		plot;
	AREA		area;
	CURVE		curve;
	LABEL		label;
	MARK		mark;
	BARB		barb;
	BUTTON		button;
	SPOT		spot;
	LOGICAL		reading;

	/* Do nothing if metafile name not given */
	if (blank(name))
		{
#		ifdef DEBUG
		(void) fprintf(stderr, "[read_metafile] Metafile name not given\n");
#		endif /* DEBUG */
		return NullMeta;
		}

	/* See if the metafile exists */
	if (!find_file(name))
		{
#		ifdef DEBUG
		(void) fprintf(stderr,
				"[read_metafile] Metafile not found: %s\n", name);
#		endif /* DEBUG */
		return NullMeta;
		}

	/* Get revision number to see if we can read it */
	/* Metafiles without a revision number conform to the archaic standard */
	rev = find_meta_revision(name);
	set_revision_info(rev);
	if (NotMetafile)
		{
		(void) fprintf(stderr,
				"[read_metafile] Not a supported metafile: %s\n",
				name);
		return NullMeta;
		}

	/* Open the metafile */
	fp = fopen(name, "r");
	if (!fp)
		{
		(void) fprintf(stderr,
				"[read_metafile] Metafile not readable: %s\n", name);
		return NullMeta;
		}

	/* Create an empty metafile */
	meta = create_metafile();

	/* Set the default input projection */
	if (UseProj)
		{
		/* Normal default is lat-lon */
		mdef.olat  = 0;
		mdef.olon  = 0;
		mdef.lref  = 0;
		mdef.xorg  = -180;
		mdef.yorg  = -90;
		mdef.xlen  = 360;
		mdef.ylen  = 180;
		mdef.units = 1;
		define_projection(&pdef, ProjectLatLon, 0., 0., 0., 0., 0.);
		define_map_projection(&mproj, &pdef, &mdef, NullGridDef);
		}
	else
		{
		/* Prior to 1.4, the default input projection is north polar */
		/* stereographic true at 60 north */
		define_projection(&pdef, ProjectPolarSt, 90., 60., 0., 0., 0.);
		define_map_projection(&mproj, &pdef, NullMapDef, NullGridDef);
		}

	/* Initialize the base map projection if given */
	pproj = reset_projection();
	if (bproj)
		{
		pproj = projection_basemap(bproj);
		define_mf_projection(meta, pproj);
		}
	else
		{
		define_mf_projection(meta, &mproj);
		}
	(void) projection_newmap(&mproj);

	/* Read commands in the metafile */
	BailOut = FALSE;
	reading = TRUE;
	while (reading)
		{
		/* Bail out if problem encountered */
		if (BailOut)
			{
			(void) pr_error("Metafile",
					"Discarding corrupted metafile \"%s\"\n", name);
			meta = destroy_metafile(meta);
			BailOut = FALSE;
			break;
			}

		/* Reset entity, level, etc. if required */
		if (freset || mreset)
			{
			if (freset)
				{
				(void) strcpy(entity, "");
				(void) strcpy(element, "generic");
				(void) strcpy(level, "generic");
				freset = FALSE;
				}
			cal  = CAL_destroy(cal);
			lcal = CAL_destroy(lcal);
			rcal = CAL_destroy(rcal);
			ncal = CAL_destroy(ncal);
			mreset = FALSE;
			}

		/* Input next command */
		/* If end-of-file, close the metafile and return what we have */
		if (fscanf(fp, "%20s", cmd) != 1)
			{
			break;
			}

		/* Ignore comments */
		else if ((cmd[0] == '*') || (cmd[0] == '#'))
			{
			flush_line(fp);
			}

		/* Revision definition */
		else if (same(cmd, "rev"))
			{
			/* Already have revision - ignore */
			flush_line(fp);
			}

		/* Projection definition */
		else if (same(cmd, "projection"))
			{
			if (!get_projection(fp, &mproj)) continue;
			if (!pproj)
				{
				pproj = projection_basemap(&mproj);
				if (meta) define_mf_projection(meta, pproj);
				}
			(void) projection_newmap(&mproj);
			}

		/* Map definition */
		else if (same(cmd, "mapdef"))
			{
			if (!get_mapdef(fp, &mproj)) continue;
			if (!pproj)
				{
				pproj = projection_basemap(&mproj);
				if (meta) define_mf_projection(meta, pproj);
				}
			(void) projection_newmap(&mproj);
			}

		/* Source projection definition */
		else if (same(cmd, "source_projection"))
			{
			if (!get_projection(fp, &sproj)) continue;
			}

		/* Source mapdef definition */
		else if (same(cmd, "source_mapdef"))
			{
			if (!get_mapdef(fp, &sproj)) continue;
			if (meta) add_mf_source_proj(meta, &sproj);
			}

		/* Source component definition */
		else if (same(cmd, "source_component"))
			{
			if (!get_component(fp, &scinfo)) continue;
			if (meta) define_mf_source_comp(meta, (meta->nsrc-1), &scinfo);
			}

		/* Units definition */
		else if (same(cmd, "units"))
			{
			if (!get_units(fp, utype, umode)) continue;
			projection_units(utype, umode);
			}

		/* Special info command */
		else if (same(cmd, "info"))
			{
			/* Reserved for future use - ignore */
			/* Currently supported as an extended feature - */
			/* use read_meta_info() to read these lines */
			flush_line(fp);
			}

		/* Background file */
		else if (same(cmd, "bkgnd"))
			{
			if (!get_bkgnd(fp, file)) continue;
			define_mf_bgnd(meta, file, NullMeta);
			}

		/* Time stamp */
		else if (same(cmd, "tstamp"))
			{
			if (!get_tstamp(fp, rtime, vtime, model)) continue;
			vtr = interpret_timestring(rtime, NullString, 0.0);
			if ( blank(vtr) )
				{
				(void) pr_error("Metafile",
					"Format error in \"tstamp\" run-time: %s\n", rtime);
				continue;
				}
			(void) strcpy(rtime, vtr);
			vtr = interpret_timestring(vtime, rtime, 0.0);
			if ( blank(vtr) )
				{
				(void) pr_error("Metafile",
					"Format error in \"tstamp\" valid-time: %s\n", vtime);
				continue;
				}
			(void) strcpy(vtime, vtr);
			define_mf_tstamp(meta, rtime, vtime);
			define_mf_model(meta, model);
			}

		/* Field */
		else if (same(cmd, "field"))
			{
			(void) strcpy(entity, "");
			nsub = 0;
			cal  = CAL_destroy(cal);
			lcal = CAL_destroy(lcal);
			rcal = CAL_destroy(rcal);
			ncal = CAL_destroy(ncal);
			if (!get_field(fp, entity, element, level))
				continue;

			/* Assume entity-a is a surface */
			if (same(entity, "a"))
				{
				}

			/* Assume entity-v is a 2D surface */
			else if (same(entity, "v"))
				{
				}

			/* Assume entity-b is a set of areas */
			else if (same(entity, "b"))
				{
				set = make_mf_set(meta, "area", entity, element, level);
				}

			/* Assume entity-c is a set of curves */
			else if (same(entity, "c"))
				{
				set = make_mf_set(meta, "curve", entity, element, level);
				}

			/* Assume entity-d is a plot or a set of spots */
			else if (same(entity, "d"))
				{
				set = make_mf_set(meta, "spot", entity, element, level);
				}

			/* Assume entity-l is a set of link chains */
			else if (same(entity, "l"))
				{
				set = make_mf_set(meta, "lchain", entity, element, level);
				}
			}

		/* Entity */
		else if (same_start(cmd, "entity"))
			{
			c = strrchr(cmd, '-');
			if (!c) continue;
			(void) strncpy(entity, c+1, 4);
			cal  = CAL_destroy(cal);
			lcal = CAL_destroy(lcal);
			rcal = CAL_destroy(rcal);
			ncal = CAL_destroy(ncal);
			if (!get_entity(fp, element, level, &cal, &nsub, &slist, &vlist))
				continue;
			if (nsub >= 2)
				{
				/* left and right attributes for area divide */
				lcal = CAL_duplicate(cal);
				rcal = CAL_duplicate(cal);
				CAL_set_defaults(rcal, slist[1], vlist[1], CAL_NO_VALUE);
				}

			/* Assume entity-a is a surface */
			if (same(entity, "a"))
				{
				}

			/* Assume entity-v is a 2D surface */
			else if (same(entity, "v"))
				{
				}

			/* Assume entity-b is a set of areas */
			else if (same(entity, "b"))
				{
				set = make_mf_set(meta, "area", entity, element, level);
				}

			/* Assume entity-c is a set of curves */
			else if (same(entity, "c"))
				{
				set = make_mf_set(meta, "curve", entity, element, level);
				}

			/* Assume entity-d is a plot or a set of spots */
			else if (same(entity, "d"))
				{
				set = make_mf_set(meta, "spot", entity, element, level);
				}

			/* Assume entity-l is a set of link chains */
			else if (same(entity, "l"))
				{
				set = make_mf_set(meta, "lchain", entity, element, level);
				}
			}

		/* Subfields */
		else if (same_start(cmd, "subfields"))
			{
			cal  = CAL_destroy(cal);
			lcal = CAL_destroy(lcal);
			rcal = CAL_destroy(rcal);
			ncal = CAL_destroy(ncal);
			if (!get_subfields(fp, &nsub, &slist, &vlist))
				continue;
			if (RetainPlot)
				{
				plot = find_mf_plot(meta, entity, element, level);
				if (!plot)
					{
					int		i;

					plot = make_mf_plot(meta, entity, element, level);
					for (i=0; i<nsub; i++)
						{
						add_subfld_to_plot(plot, slist[i], vlist[i], NullItem);
						}
					}
				}
			}

		/* Set field background value */
		else if (same_start(cmd, "bgval"))
			{
			mreset = TRUE;
			if (same(cmd, "bgvalue"))
				{
				cal  = CAL_destroy(cal);
				lcal = CAL_destroy(lcal);
				rcal = CAL_destroy(rcal);
				ncal = CAL_destroy(ncal);
				if (!get_value(fp, entity, element, level, &cal)) continue;
				}
			else /* assume "bgval" */
				{
				freset = TRUE;
				if (!get_bgval(fp, cal)) continue;
				}
			subptr = CAL_get_attribute(cal, CALcategory);
			valptr = CAL_get_attribute(cal, CALautolabel);
			labptr = CAL_get_attribute(cal, CALuserlabel);

			/* Assume entity-a is a surface */
			if (same(entity, "a"))
				{
				}

			/* Assume entity-v is a 2D surface */
			else if (same(entity, "v"))
				{
				}

			/* Assume entity-b is a set of areas */
			else if (same(entity, "b"))
				{
				set = make_mf_set(meta, "area", entity, element, level);
				define_set_bg_attribs(set, cal);
				define_item_value("area", set->bgnd, subptr, valptr, labptr);
				}

			/* Assume entity-c is a set of curves */
			else if (same(entity, "c"))
				{
				set = make_mf_set(meta, "curve", entity, element, level);
				define_set_bg_attribs(set, cal);
				define_item_value("curve", set->bgnd, subptr, valptr, labptr);
				}

			/* Assume entity-d is a plot or a set of spots */
			else if (same(entity, "d"))
				{
				set = make_mf_set(meta, "spot", entity, element, level);
				define_set_bg_attribs(set, cal);
				define_item_value("cal", set->bgnd, subptr, valptr, labptr);
				}
			}

		/* Set field member value */
		else if (same(cmd, "value"))
			{
			cal  = CAL_destroy(cal);
			lcal = CAL_destroy(lcal);
			rcal = CAL_destroy(rcal);
			ncal = CAL_destroy(ncal);
			if (!get_value(fp, entity, element, level, &cal)) continue;
			lcal = CAL_duplicate(cal);
			rcal = CAL_duplicate(cal);
			}
		else if (same(cmd, "lvalue"))
			{
			lcal = CAL_destroy(lcal);
			if (!get_value(fp, entity, element, level, &lcal)) continue;
			}
		else if (same(cmd, "rvalue"))
			{
			rcal = CAL_destroy(rcal);
			if (!get_value(fp, entity, element, level, &rcal)) continue;
			}
		else if (same(cmd, "nvalue"))
			{
			ncal = CAL_destroy(ncal);
			if (!get_value(fp, "n", element, level, &ncal)) continue;
			}

		/* B-spline surface (convert grid to B-spline) */
		else if (same(cmd, "bspline") || same(cmd, "grid"))
			{
			mreset = TRUE;
			if (same(cmd, "bspline") && !get_bspline(fp, &sfc)) continue;
			else if (same(cmd, "grid") && !get_grid(fp, &sfc))  continue;
			(void) surface_to_MKS(sfc, element, level);
			fld = make_mf_field(meta, "surface", NullString, entity,
															element, level);
			define_fld_data(fld, "surface", (ITEM) sfc);
			}

		/* 2D B-spline surface (convert grid to B-spline) */
		else if (same(cmd, "bspline2D") || same(cmd, "grid2D"))
			{
			mreset = TRUE;
			if (same(cmd, "bspline2D") && !get_bspline2D(fp, &sfc)) continue;
			/*
			else if (same(cmd, "grid2D") && !get_grid2D(fp, &sfc))  continue;
			*/
			(void) surface_to_MKS(sfc, element, level);
			fld = make_mf_field(meta, "surface", NullString, entity,
															element, level);
			define_fld_data(fld, "surface", (ITEM) sfc);
			}

		/* Compressed Raster data */
		else if (same(cmd, "raster"))
			{
			mreset = TRUE;
			if (!get_raster(fp, &raster, mask)) continue;
			fld = make_mf_field(meta,"raster", NullString, entity, element, level);
			define_fld_data(fld, "raster", (ITEM) raster);

			}
		/* Compressed bit mask */
		else if (same(cmd, "mask"))
			{
				if (!get_mask(fp, &mask)) continue;
			}
		/* Area boundary */
		else if (same(cmd, "area"))
			{
			if (fscanf(fp, "%20s", cmd) != 1)
				{
				break;
				}

			/* Area boundary */
			if (same(cmd, "boundary"))
				{
				mreset = TRUE;

				set = make_mf_set(meta, "area", entity, element, level);
				if (UseArea)
					{
					if (!get_areaset_area(fp, set, cal)) continue;
					}
				else
					{
					if (!get_old_area(fp, set, cal)) continue;
					}
				}

			/* Area hole */
			else if (same(cmd, "hole"))
				{
				mreset = TRUE;

				set = find_mf_set(meta, "area", entity, element, level);
				if (!get_areaset_hole(fp, set, cal)) continue;
				}

			/* Area divide */
			else if (same(cmd, "divide"))
				{
				mreset = TRUE;

				set = find_mf_set(meta, "area", entity, element, level);
				if (!get_areaset_divide(fp, set, (lcal)? lcal: cal,
										(rcal)? rcal: cal)) continue;
				}
			}

		/* Curve */
		else if (same(cmd, "curve"))
			{
			mreset = TRUE;

			/* Assume entity-b is a set of areas */
			if (same(entity, "b"))
				{
				if (!get_curve(fp, &curve, cal)) continue;
				subptr = CAL_get_attribute(cal, CALcategory);
				valptr = CAL_get_attribute(cal, CALautolabel);
				labptr = CAL_get_attribute(cal, CALuserlabel);

				set = make_mf_set(meta, "area", entity, element, level);
				area = create_area(subptr, valptr, labptr);
				define_area_attribs(area, cal);
				define_area_boundary(area, curve->line);
				add_item_to_set(set, (ITEM) area);
				curve->line = NullLine;
				curve = destroy_curve(curve);
				}

			/* Otherwise assume a set of curves */
			else
				{
				subptr = CAL_get_attribute(cal, CALcategory);
				if ( same(entity, "c") && same(subptr, "boundary"))
					{
					/* These are destined to become areas so don't check */
					/* for wrapping around the earth */
					if (!get_curve(fp, &curve, cal)) continue;
					set = make_mf_set(meta, "curve", entity, element, level);
					add_item_to_set(set, (ITEM) curve);
					}
				else
					{
					/* Check for wrapping around the earth */
					if (!get_curves(fp, &cset, cal)) continue;
					set = make_mf_set(meta, "curve", entity, element, level);
					append_set(set, cset);
					cset = destroy_set(cset);
					}
				}
			}

		/* Label */
		else if (same(cmd, "label"))
			{
			mreset = TRUE;
			if (!get_label(fp, &label, cal)) continue;
			set = make_mf_set(meta, "label", entity, element, level);
			add_item_to_set(set, (ITEM) label);
			}

		/* Mark */
		else if (same(cmd, "mark"))
			{
			mreset = TRUE;
			if (!get_mark(fp, &mark, cal)) continue;
			set = make_mf_set(meta, "mark", entity, element, level);
			add_item_to_set(set, (ITEM) mark);
			}

		/* Barb */
		else if (same(cmd, "barb"))
			{
			mreset = TRUE;
			if (!get_barb(fp, &barb, cal)) continue;
			set = make_mf_set(meta, "barb", entity, element, level);
			add_item_to_set(set, (ITEM) barb);
			}

		/* Button */
		else if (same(cmd, "button"))
			{
			mreset = TRUE;
			if (!get_button(fp, &button, cal)) continue;
			set = make_mf_set(meta, "button", entity, element, level);
			add_item_to_set(set, (ITEM) button);
			}

		/* Spot */
		else if (same(cmd, "spot"))
			{
			mreset = TRUE;
			if (!get_spot(fp, element, level, &spot, cal)) continue;
			set = make_mf_set(meta, "spot", entity, element, level);
			add_item_to_set(set, (ITEM) spot);
			}

		/* Plot (point for scattered plot) */
		else if (same(cmd, "plot"))
			{
			mreset = FALSE;
			cal  = CAL_destroy(cal);
			cal  = CAL_create_by_name(element, level);
			if (RetainPlot)
				{
				plot = find_mf_plot(meta, entity, element, level);
				if (!plot)
					{
					int		i;

					plot = make_mf_plot(meta, entity, element, level);
					for (i=0; i<nsub; i++)
						{
						add_subfld_to_plot(plot, slist[i], vlist[i], NullItem);
						}
					}
				if (!get_plot_point_as_both(fp, element, level, plot,
						&spot, cal, slist, vlist, nsub)) continue;
				}
			else
				{
				if (!get_plot_point_as_spot(fp, element, level,
						&spot, cal, slist, vlist, nsub)) continue;
				}
			set = make_mf_set(meta, "spot", entity, element, level);
			add_item_to_set(set, (ITEM) spot);
			}

		/* Link chain */
		else if (same(cmd, "lchain"))
			{
			mreset = TRUE;

			if (lnum != 0)
				{
				(void) pr_error("Metafile",
					"Unread nodes (%d) for link chain in metafile: %s\n",
					lnum, name);
				lnum = 0;
				}

			set = make_mf_set(meta, "lchain", entity, element, level);
			if (!get_lchain(fp, set, cal, &lnum)) continue;
			}

		/* Link node */
		else if (same(cmd, "node"))
			{
			mreset = TRUE;

			set = find_mf_set(meta, "lchain", entity, element, level);
			if (!get_lchain_node(fp, set, ncal, &lnum)) continue;
			}

		/* Ignore anything else */
		else
			{
			flush_line(fp);
			}
		}

	if (lnum != 0)
		{
		(void) pr_error("Metafile",
			"Unread nodes (%d) for link chain in metafile: %s\n",
			lnum, name);
		}

	/* Close the file and return the metafile */
	(void) fclose(fp);
	return meta;
	}

/***********************************************************************
*                                                                      *
*      f i n d _ m e t a _ m a p _ p r o j e c t i o n                 *
*      f i n d _ m e t a _ r e v i s i o n                             *
*      r e a d _ m e t a _ i n f o                                     *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Read the map projection information from the given
 * metafile.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy.
 *
 *	@param[in]	name	metafile name
 * 	@return a Map projection, NullMapProj if a projection could not
 * 			be found.
 **********************************************************************/
MAP_PROJ	*find_meta_map_projection

	(
	STRING	name
	)

	{
	static	MAP_PROJ	mproj;

	PROJ_DEF	pdef;
	FILE		*fp;
	char		cmd[21];
	STRING		rev;
	LOGICAL		found;

	/* Do nothing if metafile name not given */
	if (blank(name))
		{
#		ifdef DEBUG
		(void) fprintf(stderr,
				"[find_meta_map_projection] Metafile name not given\n");
#		endif /* DEBUG */
		return NullMapProj;
		}

	/* See if the metafile exists */
	if (!find_file(name))
		{
#		ifdef DEBUG
		(void) fprintf(stderr,
				"[find_meta_map_projection] Metafile not found: %s\n",
				name);
#		endif /* DEBUG */
		return NullMapProj;
		}

	/* Get revision number to see if we can read it */
	/* Metafiles without a revision number conform to the archaic standard */
	rev = find_meta_revision(name);
	set_revision_info(rev);
	if (NotMetafile)
		{
		return NullMapProj;
		}

	/* Open the metafile */
	fp = fopen(name, "r");
	if (!fp)
		{
		(void) fprintf(stderr,
				"[find_meta_map_projection] Metafile not readable: %s\n",
				name);
		return NullMapProj;
		}

	/* Set the default input projection */
	if (UseProj)
		{
		/* Normal default is lat-lon */
		define_projection(&pdef, ProjectPolarSt, 0., 0., 0., 0., 0.);
		define_map_projection(&mproj, &pdef, NullMapDef, NullGridDef);
		}
	else
		{
		/* Prior to 1.4, the default input projection is north polar */
		/* stereographic true at 60 north */
		define_projection(&pdef, ProjectPolarSt, 90., 60., 0., 0., 0.);
		define_map_projection(&mproj, &pdef, NullMapDef, NullGridDef);
		}

	/* Read commands in the metafile until a mapdef is encountered */
	found = FALSE;
	while (!found)
		{
		/* Input next command */
		/* If end-of-file, close the metafile and return what we have */
		if (fscanf(fp, "%20s", cmd) != 1)
			{
			break;
			}

		/* Ignore comments */
		else if ((cmd[0] == '*') || (cmd[0] == '#'))
			{
			flush_line(fp);
			}

		/* Projection definition */
		else if (same(cmd, "projection"))
			{
			if (!get_projection(fp, &mproj)) continue;
			}

		/* Map definition */
		else if (same(cmd, "mapdef"))
			{
			if (!get_mapdef(fp, &mproj)) continue;
			found = TRUE;
			break;
			}

		/* Ignore anything else */
		else
			{
			flush_line(fp);
			}
		}

	(void) fclose(fp);
	return (found)? &mproj: NullMapProj;
	}

/**********************************************************************/

/**********************************************************************/
/** Read the metafile revision number.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	name	metafile name
 * 	@return The revision number of the given metafile. NullString if a
 * 			revision number could not be found.
 **********************************************************************/
STRING	find_meta_revision

	(
	STRING	name
	)

	{
	static	char	revision[21] = "";

	FILE	*fp;
	char	cmd[21];
	LOGICAL	found;

	/* Do nothing if metafile name not given */
	if (blank(name))
		{
#		ifdef DEBUG
		(void) fprintf(stderr,
				"[find_meta_revision] Metafile name not given\n");
#		endif /* DEBUG */
		return NullString;
		}

	/* See if the metafile exists */
	if (!find_file(name))
		{
#		ifdef DEBUG
		(void) fprintf(stderr,
				"[find_meta_revision] Metafile not found: %s\n", name);
#		endif /* DEBUG */
		return NullString;
		}

	/* Open the metafile */
	fp = fopen(name, "r");
	if (!fp)
		{
		(void) fprintf(stderr,
				"[find_meta_revision] Metafile not readable: %s\n", name);
		return NullString;
		}

	/* Read commands in the metafile until a revision is encountered */
	found = FALSE;
	while (!found)
		{
		/* Input next command */
		/* If end-of-file, close the metafile and return what we have */
		if ( feof(fp) || ferror(fp) || fscanf(fp, "%20s", cmd) != 1)
			{
			break;
			}

		/* Ignore comments */
		else if ((cmd[0] == '*') || (cmd[0] == '#'))
			{
			flush_line(fp);
			}

		/* Revision definition */
		else if (same(cmd, "rev"))
			{
			if (!get_rev(fp, revision)) continue;
			found = TRUE;
			break;
			}

		/* Anything else implies its not a metafile */
		else
			{
			flush_line(fp);
			break;
			}
		}

	(void) fclose(fp);
	return (found)? revision: NullString;
	}

/**********************************************************************/

/**********************************************************************/
/** Read metafile "info" commands and store results in a Table.
 *
 *	@param[in]	name	metafile name
 *	@param[in]	process	arbitrary name that a process may use to select the 
 *						subset of parameters that it needs.
 *	@param[out]	*nlist 	size of list
 *	@param[out]	**list	List of metafile info commands
 * 	@return True if successful.
 **********************************************************************/
LOGICAL	read_meta_info

	(
	STRING	name,
	STRING	process,
	int		*nlist,
	TABLE	**list
	)

	{
	FILE	*fp;
	char	cmd[21], proc[41], param[41];
	STRING	value;
	LOGICAL	reading;

	int		Nlist = 0;
	TABLE	*List = 0;

	/* Do nothing if metafile name not given */
	if (blank(name))
		{
#		ifdef DEBUG
		(void) fprintf(stderr,
				"[read_meta_info] Metafile name not given\n");
#		endif /* DEBUG */
		return FALSE;
		}

	/* See if the metafile exists */
	if (!find_file(name))
		{
#		ifdef DEBUG
		(void) fprintf(stderr,
				"[read_meta_info] Metafile not found: %s\n", name);
#		endif /* DEBUG */
		return FALSE;
		}

	/* Open the metafile */
	fp = fopen(name, "r");
	if (!fp)
		{
		(void) fprintf(stderr,
				"[read_meta_info] Metafile not readable: %s\n", name);
		return FALSE;
		}

	/* Read commands in the metafile until a revision is encountered */
	reading = TRUE;
	while (reading)
		{
		/* Input next command */
		/* If end-of-file, close the metafile and return what we have */
		if (fscanf(fp, "%20s", cmd) != 1)
			{
			break;
			}

		/* Ignore comments */
		else if ((cmd[0] == '*') || (cmd[0] == '#'))
			{
			flush_line(fp);
			}

		/* Process only the "info" commands */
		else if (same(cmd, "info"))
			{
			if (!get_info(fp, proc, param, &value)) continue;
			if (!same(proc, process))               continue;
			Nlist++;
			List = GETMEM(List, TABLE, Nlist);
			List[Nlist-1].index = strdup(param);
			List[Nlist-1].value = strdup(value);
			}

		/* Ignore anything else */
		else
			{
			flush_line(fp);
			}
		}

	(void) fclose(fp);
	if (nlist) *nlist = Nlist;
	if (list)  *list  = List;
	return (LOGICAL) (Nlist>0);
	}

/***********************************************************************
*                                                                      *
*      s e a r c h _ m e t a _ f i e l d s                             *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Search the metafile fields of a given metafile, and return
 * 	a list of Field Structs.
 *
 *	@param[in]	name		metafile name
 *	@param[out]	***fdefs	list of field structures
 * 	@return The size of the list.
 **********************************************************************/
int		search_meta_fields

	(
	STRING					name,
	FpaConfigFieldStruct	***fdefs
	)

	{
	static	int						nfld    = 0;
	static	LOGICAL					*fldsok = NullLogicalList;
	static	FpaConfigFieldStruct	**flds  = NullPtr(FpaConfigFieldStruct **);

	FpaConfigFieldStruct	*fld;
	FILE	*fp;
	int		nsub, ifld;
	LOGICAL	reading, fldok;
	char	cmd[21];
	char	entity[41], element[41], level[41];
	STRING	c, *slist, *vlist, rev, fldent;

	FREEMEM(fldsok);
	FREEMEM(flds);
	nfld = 0;
	if (fdefs) *fdefs = NullPtr(FpaConfigFieldStruct **);

	/* Do nothing if metafile name not given */
	if (blank(name))
		{
#		ifdef DEBUG
		(void) fprintf(stderr, "[search_meta_fields] Metafile name not given\n");
#		endif /* DEBUG */
		return 0;
		}

	/* See if the metafile exists */
	if (!find_file(name))
		{
#		ifdef DEBUG
		(void) fprintf(stderr, "[search_meta_fields] Metafile not found: %s\n",
				name);
#		endif /* DEBUG */
		return 0;
		}

	/* Get revision number to see if we can read it */
	/* Metafiles without a revision number conform to the archaic standard */
	rev = find_meta_revision(name);
	set_revision_info(rev);
	if (NotMetafile)
		{
		return 0;
		}

	/* Open the metafile */
	fp = fopen(name, "r");
	if (!fp)
		{
		(void) fprintf(stderr, "[search_meta_fields] Metafile not readable: %s\n",
				name);
		return 0;
		}

#	ifdef DEBUG
	(void) fprintf(stderr,
		"[search_meta_fields] Reading Metafile: %s\n", name);
#	endif /* DEBUG */

	/* Read the metafile and save all "field" information      */
	/* Note that the obsolete "entity" format is also accepted */
	reading = TRUE;
	while (reading)
		{
		/* Input next command */
		/* If end-of-file, close the metafile and return what we have */
		if (fscanf(fp, "%20s", cmd) != 1)
			{
			break;
			}

		/* Ignore comments */
		else if ((cmd[0] == '*') || (cmd[0] == '#'))
			{
			flush_line(fp);
			}

		/* Field */
		else if (same(cmd, "field"))
			{
			if (!get_field(fp, entity, element, level)) continue;

#			ifdef DEBUG
			(void) fprintf(stderr,
				"[search_meta_fields]   Entity/Element/Level: %s %s %s\n",
				entity, element, level);
#			endif /* DEBUG */

			fld = identify_field(element, level);
			if (!fld) continue;
			fldent = entity_from_field_type(fld->element->fld_type);
			fldok  = same(entity, fldent);

			for( ifld=0; ifld<nfld; ifld++ )
				{
				if( flds[ifld] == fld )
					{
					if (fldok) fldsok[ifld] = TRUE;
					break;
					}
				}
			if(ifld >= nfld)
				{
				fldsok = GETMEM(fldsok, LOGICAL,                nfld+1);
				flds   = GETMEM(flds  , FpaConfigFieldStruct *, nfld+1);
				fldsok[nfld] = fldok;
				flds[nfld]   = fld;
				nfld++;
				}
			}

		/* Entity */
		else if (same_start(cmd, "entity"))
			{
			c = strrchr(cmd, '-');
			if (!c) continue;
			(void) strncpy(entity, c+1, 4);
			if (!get_entity(fp, element, level, NullCalPtr,
							&nsub, &slist, &vlist))
				continue;

#			ifdef DEBUG
			(void) fprintf(stderr,
				"[search_meta_fields]   Entity/Element/Level: %s %s %s\n",
				entity, element, level);
#			endif /* DEBUG */

			fld = identify_field(element, level);
			if (!fld) continue;
			fldent = entity_from_field_type(fld->element->fld_type);
			fldok  = same(entity, fldent);

			for( ifld=0; ifld<nfld; ifld++ )
				{
				if( flds[ifld] == fld )
					{
					if (fldok) fldsok[ifld] = TRUE;
					break;
					}
				}
			if(ifld >= nfld)
				{
				fldsok = GETMEM(fldsok, LOGICAL,                nfld+1);
				flds   = GETMEM(flds  , FpaConfigFieldStruct *, nfld+1);
				fldsok[nfld] = fldok;
				flds[nfld]   = fld;
				nfld++;
				}
			}

		/* Ignore anything else */
		else
			{
			flush_line(fp);
			}
		}

	/* Warning message if any fields are of the wrong type */
	for( ifld=0; ifld<nfld; ifld++ )
		{
		if (!fldsok[ifld])
			{
			(void) pr_warning("Metafile", "In metafile: %s\n", name);
			(void) pr_warning("Metafile",
				"  Field: %s %s ... contains data of wrong type!\n",
				flds[ifld]->element->name, flds[ifld]->level->name);
			}
		}

#	ifdef DEBUG
	(void) fprintf(stderr,
		"[search_meta_fields]   Return %d field(s) for: %s %s\n",
		nfld, element, level);
#	endif /* DEBUG */

	/* Close the metafile and return the number of fields */
	(void) fclose(fp);
	if (fdefs) *fdefs = flds;
	return nfld;
	}

/***********************************************************************
*                                                                      *
*      s e a r c h _ l i n k _ f i e l d s                             *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Search the fields of a given link file, and return
 * 	a list of Field Structs.
 *
 *	@param[in]	name		link file name
 *	@param[out]	***fdefs	list of field structures
 * 	@return The size of the list.
 **********************************************************************/
int		search_link_fields

	(
	STRING					name,
	FpaConfigFieldStruct	***fdefs
	)

	{
	static	int						nfld    = 0;
	static	LOGICAL					*fldsok = NullLogicalList;
	static	FpaConfigFieldStruct	**flds  = NullPtr(FpaConfigFieldStruct **);

	FpaConfigFieldStruct	*fld;
	FILE	*fp;
	int		nsub, ifld;
	LOGICAL	reading, fldok;
	char	cmd[21];
	char	entity[41], element[41], level[41];
	STRING	c, *slist, *vlist, rev, fldent;

	FREEMEM(fldsok);
	FREEMEM(flds);
	nfld = 0;
	if (fdefs) *fdefs = NullPtr(FpaConfigFieldStruct **);

	/* Do nothing if link file name not given */
	if (blank(name))
		{
#		ifdef DEBUG
		(void) fprintf(stderr, "[search_link_fields] Link file name not given\n");
#		endif /* DEBUG */
		return 0;
		}

	/* See if the link file exists */
	if (!find_file(name))
		{
#		ifdef DEBUG
		(void) fprintf(stderr, "[search_link_fields] Link file not found: %s\n",
				name);
#		endif /* DEBUG */
		return 0;
		}

	/* Get revision number to see if we can read it */
	rev = find_meta_revision(name);
	set_revision_info(rev);
	if (NotMetafile)
		{
		return 0;
		}

	/* Open the link file */
	fp = fopen(name, "r");
	if (!fp)
		{
		(void) fprintf(stderr, "[search_link_fields] Link file not readable: %s\n",
				name);
		return 0;
		}

#	ifdef DEBUG
	(void) fprintf(stderr,
		"[search_link_fields] Reading Link file: %s\n", name);
#	endif /* DEBUG */

	/* Read the link file and save all "field" information */
	reading = TRUE;
	while (reading)
		{
		/* Input next command */
		/* If end-of-file, close the link file and return what we have */
		if (fscanf(fp, "%20s", cmd) != 1)
			{
			break;
			}

		/* Ignore comments */
		else if ((cmd[0] == '*') || (cmd[0] == '#'))
			{
			flush_line(fp);
			}

		/* Field */
		else if (same(cmd, "field"))
			{
			if (!get_field(fp, entity, element, level)) continue;

#			ifdef DEBUG
			(void) fprintf(stderr,
				"[search_link_fields]   Entity/Element/Level: %s %s %s\n",
				entity, element, level);
#			endif /* DEBUG */

			fld = identify_field(element, level);
			if (!fld) continue;

			/* Check only for link chain fields */
			fldok = same(entity, "l");

			for( ifld=0; ifld<nfld; ifld++ )
				{
				if( flds[ifld] == fld )
					{
					if (fldok) fldsok[ifld] = TRUE;
					break;
					}
				}
			if(ifld >= nfld)
				{
				fldsok = GETMEM(fldsok, LOGICAL,                nfld+1);
				flds   = GETMEM(flds  , FpaConfigFieldStruct *, nfld+1);
				fldsok[nfld] = fldok;
				flds[nfld]   = fld;
				nfld++;
				}
			}

		/* Ignore anything else */
		else
			{
			flush_line(fp);
			}
		}

	/* Warning message if any fields are of the wrong type */
	for( ifld=0; ifld<nfld; ifld++ )
		{
		if (!fldsok[ifld])
			{
			(void) pr_warning("Link file", "In link file: %s\n", name);
			(void) pr_warning("Link file",
				"  Field: %s %s ... contains data of wrong type!\n",
				flds[ifld]->element->name, flds[ifld]->level->name);
			}
		}

#	ifdef DEBUG
	(void) fprintf(stderr,
		"[search_link_fields]   Return %d link field(s) for: %s %s\n",
		nfld, element, level);
#	endif /* DEBUG */

	/* Close the link file and return the number of fields */
	(void) fclose(fp);
	if (fdefs) *fdefs = flds;
	return nfld;
	}

/***********************************************************************
*                                                                      *
*      p a r s e _ m e t a f i l e _ p r o j e c t i o n               *
*      p a r s e _ m e t a f i l e _ m a p d e f                       *
*                                                                      *
*      These are duplicates of code in functions  get_projection()     *
*      and  get_mapdef()  to replicate reading of projection and       *
*      mapdef information done in function   read_metafile().          *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Parse the metafile projection information into a PROJ_DEF
 * structure.
 *
 *	@param[in]	tbuf	projection string
 *	@param[out]	*proj	map projection
 *  @return True if successful.
 **********************************************************************/
LOGICAL		parse_metafile_projection

	(
	STRING		tbuf,
	PROJ_DEF	*proj
	)

	{
	LOGICAL		status;
	char		xbuf[1024], xname[60];
	char		pname[60], ref1[20], ref2[20], ref3[20], ref4[20], ref5[20];

	(void) strcpy(xbuf, tbuf);
	strcpy_arg(xname, tbuf, &status);
	if (!status || !same(xname, "projection"))
		{
		(void) pr_error("Metafile",
			"Unrecognized projection information: %s\n", xbuf);
		return FALSE;
		}

	/* Any changes to  get_projection()  must be replicated here */
	strcpy_arg(pname, tbuf, &status);	if (!status) return FALSE;
	strcpy_arg(ref1, tbuf, &status);	if (!status) (void) strcpy(ref1, "");
	strcpy_arg(ref2, tbuf, &status);	if (!status) (void) strcpy(ref2, "");
	strcpy_arg(ref3, tbuf, &status);	if (!status) (void) strcpy(ref3, "");
	strcpy_arg(ref4, tbuf, &status);	if (!status) (void) strcpy(ref4, "");
	strcpy_arg(ref5, tbuf, &status);	if (!status) (void) strcpy(ref5, "");

	/* Define a projection as specified */
	if (!define_projection_by_name(proj, pname, ref1, ref2, ref3, ref4, ref5))
		{
		(void) pr_error("Metafile",
			"Unrecognized projection information: %s\n", xbuf);
		return FALSE;
		}
	return TRUE;
	}

/**********************************************************************/

/**********************************************************************/
/** Parse the metafile mapdef into a MAP_DEF structure.
 *
 *	@param[in]	tbuf	string containing mapdef information
 *	@param[out]	*mdef	map definition
 *  @return True if successful.
 **********************************************************************/
LOGICAL		parse_metafile_mapdef

	(
	STRING		tbuf,
	MAP_DEF		*mdef
	)

	{
	LOGICAL		status;
	char		xbuf[1024], xname[60];
	char		lval[20];
	float		olat, olon, lref, xmin, ymin, xmax, ymax, units;

	(void) strcpy(xbuf, tbuf);
	strcpy_arg(xname, tbuf, &status);
	if (!status || !same(xname, "mapdef"))
		{
		(void) pr_error("Metafile",
			"Unrecognized mapdef information: %s\n", xbuf);
		return FALSE;
		}

	/* Any changes to  get_mapdef()  must be replicated here */
	strcpy_arg(lval, tbuf, &status);	if (!status) return FALSE;
	olat  = read_lat(lval, &status);	if (!status) return FALSE;
	strcpy_arg(lval, tbuf, &status);	if (!status) return FALSE;
	olon  = read_lon(lval, &status);	if (!status) return FALSE;
	strcpy_arg(lval, tbuf, &status);	if (!status) return FALSE;
	lref  = read_lon(lval, &status);	if (!status) return FALSE;
	xmin  = float_arg(tbuf, &status);	if (!status) return FALSE;
	ymin  = float_arg(tbuf, &status);	if (!status) return FALSE;
	xmax  = float_arg(tbuf, &status);	if (!status) return FALSE;
	ymax  = float_arg(tbuf, &status);	if (!status) return FALSE;
	units = float_arg(tbuf, &status);	if (!status) return FALSE;

	/* Define a map definition as specified */
	mdef->olat  = olat;
	mdef->olon  = olon;
	mdef->lref  = lref;
	mdef->xorg  = -xmin;
	mdef->yorg  = -ymin;
	mdef->xlen  = xmax - xmin;
	mdef->ylen  = ymax - ymin;
	mdef->units = units;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     r e s e t _ p r o j e c t i o n                                  *
*     p r o j e c t i o n _ b a s e m a p                              *
*     p r o j e c t i o n _ n e w m a p                                *
*     p r o j e c t i o n _ u n i t s                                  *
*     m e t a _ p o i n t                                              *
*     m e t a _ p r o j e c t                                          *
*     s t a r t _ w r a p _ d e t e c t                                *
*     e n d _ w r a p _ d e t e c t                                    *
*     w r a p _ d e t e c t e d                                        *
*                                                                      *
*     Functions to re-map metafile contents to a target basemap.       *
*                                                                      *
***********************************************************************/

/* Variables relating to the projection from NewMap to BaseMap */
static	MAP_PROJ	BaseMap  = NO_MAPPROJ;	/* target basemap */
static	MAP_PROJ	NewMap   = NO_MAPPROJ;	/* current map in metafile */
static	LOGICAL		Defined  = FALSE;		/* has a projection been defined */
static	LOGICAL		DegMin   = FALSE;		/* use degrees and minutes */
static	LOGICAL		DegReal  = FALSE;		/* use decimal degrees */
static	float		UnitFact = 1;			/* Scale factor for map units */

/* Variables for "back-side of earth" wrap detection */
static	LOGICAL		DoWrap   = FALSE;		/* enable/disable wrap detection */
static	LOGICAL		InitWrap = FALSE;		/* initialize wrap detection */
static	float		MinLon   = 0;			/* minimum longitude */
static	float		MaxLon   = 0;			/* maximum longitude */
static	int			CurrWrap = 0;			/* wrap of current longitude */
static	int			PrevWrap = 0;			/* wrap of previous longitude */

/**********************************************************************/

/* Initialize the projection parameters */
static	MAP_PROJ	*reset_projection(void)
	{
	copy_map_projection(&BaseMap, &NoMapProj);
	copy_map_projection(&NewMap, &NoMapProj);

	Defined = FALSE;

	return NullMapProj;
	}

/**********************************************************************/

/* Define the target basemap */
static	MAP_PROJ	*projection_basemap

	(
	const MAP_PROJ	*mproj
	)

	{
	/* Just reset if no map projection given */
	if (!mproj)                       return reset_projection();
	if (mproj->definition.units <= 0) return reset_projection();

	/* Set up a new target basemap */
	copy_map_projection(&BaseMap, mproj);

	/* Make the current map projection equal to the target */
	Defined = TRUE;
	(void) projection_newmap(NullMapProj);

	return &BaseMap;
	}

/**********************************************************************/

/* Define the current map projection */
static	MAP_PROJ	*projection_newmap

	(
	const MAP_PROJ	*mproj
	)

	{
	/* If no map projection given, make the current map projection equal */
	/* to the target */
	if (!mproj || mproj->definition.units <= 0)
		{
		if (!Defined) return reset_projection();

		copy_map_projection(&NewMap, &BaseMap);

		Defined = TRUE;
		DegMin  = FALSE;
		DegReal = FALSE;

		return &NewMap;
		}

	/* If no target basemap yet, make the target basemap the same */
	if (!Defined)
		{
		(void) projection_basemap(mproj);
		return &NewMap;
		}

	/* Set up the new current map projection */
	copy_map_projection(&NewMap, mproj);

	/* Define the transformation from current map projection to the target */
	Defined = TRUE;
	DegMin  = FALSE;
	DegReal = FALSE;

	return &NewMap;
	}

/**********************************************************************/

/* Define the current units */
static	void	projection_units

	(
	STRING	utype,
	STRING	umode
	)

	{
	float	units;
	LOGICAL	status;

	if (!Defined) return;

	/* If x-y co-ordinates */
	/* define the transformation based on the given units */
	if (same(utype, "xy"))
		{
		DegMin  = FALSE;
		DegReal = FALSE;
		if (blank(umode)) UnitFact = 1;
		else
			{
			units = float_arg(umode, &status);
			if (status) UnitFact = units / NewMap.definition.units;
			else        UnitFact = 1;
			}
		}

	/* If lat-lon co-ordinates */
	/* define whether to use degrees and minutes or decimal degrees */
	else if (same(utype, "latlon"))
		{
		DegMin  = (LOGICAL)	(  same(umode, "degmin")
							|| same(umode, "dddmm")
							|| same(umode, "ddmm")
							);
		DegReal = !DegMin;
		}
	}

/**********************************************************************/

/* Project a point from input units to NewMap co-ordinates */
static	LOGICAL	meta_point

	(
	STRING	val1,
	STRING	val2,
	POINT	p
	)

	{
	LOGICAL	ok;
	int		ilat, ilon;
	float	lat, lon;

	if (!Defined) return FALSE;
	if (!p)       return FALSE;

	/* If using lat-lon in degrees and minutes, treat the given point as */
	/* lat and lon in degrees and minutes, and transform to a point. */
	if (DegMin)
		{
		ilat = int_arg(val1, &ok);	if (!ok) return FALSE;
		ilon = int_arg(val2, &ok);	if (!ok) return FALSE;
		lat  = degrees(ilat);
		lon  = degrees(ilon);;
		if (!UseProj) lon = -lon;
		ll_to_pos(&NewMap, lat, lon, p);
		}

	/* If using lat-lon in decimal degrees, treat the given point as */
	/* lat and lon in decimal degrees, and transform to a point. */
	else if (DegReal)
		{
		lat = read_lat(val1, &ok);	if (!ok) return FALSE;
		lon = read_lon(val2, &ok);	if (!ok) return FALSE;
		if (!UseProj) lon = -lon;
		ll_to_pos(&NewMap, lat, lon, p);
		}

	/* If using x-y co-ordinates leave alone. */
	else
		{
		p[X] = float_arg(val1, &ok);	if (!ok) return FALSE;
		p[Y] = float_arg(val2, &ok);	if (!ok) return FALSE;
		if (UnitFact != 1)
			{
			p[X] *= UnitFact;
			p[Y] *= UnitFact;
			}
		}

	return TRUE;
	}

/**********************************************************************/

/* Project a point from input units on NewMap to BaseMap co-ordinates */
static	LOGICAL	meta_project

	(
	STRING	val1,
	STRING	val2,
	POINT	p
	)

	{
	LOGICAL	ok;
	int		ilat, ilon;
	float	lat, lon;
	POINT	pos;

	if (!p)       return FALSE;

	/* If no map definition, just treat the point as given */
	if (!Defined)
		{
		p[X] = float_arg(val1, &ok);	if (!ok) return FALSE;
		p[Y] = float_arg(val2, &ok);	if (!ok) return FALSE;
		}

	/* If using lat-lon in degrees and minutes, treat the given point as */
	/* lat and lon in degrees and minutes, and transform to a point on */
	/* the target map projection. */
	else if (DegMin)
		{
		ilat = int_arg(val1, &ok);	if (!ok) return FALSE;
		ilon = int_arg(val2, &ok);	if (!ok) return FALSE;
		lat  = degrees(ilat);
		lon  = degrees(ilon);;
		if (!UseProj) lon = -lon;
		detect_wrap(lon);
		ll_to_pos(&BaseMap, lat, lon, p);
		}

	/* If using lat-lon in decimal degrees, treat the given point as */
	/* lat and lon in decimal degrees, and transform to a point on */
	/* the target map projection. */
	else if (DegReal)
		{
		lat = read_lat(val1, &ok);	if (!ok) return FALSE;
		lon = read_lon(val2, &ok);	if (!ok) return FALSE;
		if (!UseProj) lon = -lon;
		detect_wrap(lon);
		ll_to_pos(&BaseMap, lat, lon, p);
		}

	/* If using x-y co-ordinates, transform directly to a point on the */
	/* target map projection. */
	else
		{
		pos[X] = float_arg(val1, &ok);	if (!ok) return FALSE;
		pos[Y] = float_arg(val2, &ok);	if (!ok) return FALSE;
		if (UnitFact != 1)
			{
			pos[X] *= UnitFact;
			pos[Y] *= UnitFact;
			}
		pos_to_pos(&NewMap, pos, &BaseMap, p);
		}

	return TRUE;
	}

/**********************************************************************/

static	void	start_wrap_detect(void)
	{
	if (!Defined) return;

	if (!DegMin && !DegReal) return;

	DoWrap   = TRUE;
	InitWrap = TRUE;
	MinLon   = NewMap.clon - 180;
	MaxLon   = NewMap.clon + 180;
	}

/**********************************************************************/

static	void	end_wrap_detect(void)
	{
	if (!Defined) return;

	DoWrap = FALSE;
	}

/**********************************************************************/

static	void	detect_wrap

	(
	float	lon
	)

	{
	float	s;

	if (!Defined) return;
	if (!DoWrap)  return;

	PrevWrap = (InitWrap)? 0: CurrWrap;

	s = (lon-MinLon) / (MaxLon-MinLon);
	CurrWrap = floor(s);

	InitWrap = wrap_detected();
	}

/**********************************************************************/

static	LOGICAL	wrap_detected(void)
	{
	if (!Defined) return FALSE;
	if (!DoWrap)  return FALSE;

	if (InitWrap) return FALSE;

	return (LOGICAL) (CurrWrap != PrevWrap);
	}

/***********************************************************************
*                                                                      *
*      s e t _ r e v i s i o n _ i n f o                               *
*                                                                      *
*      Routine to test for revision dependent features.                *
*                                                                      *
***********************************************************************/

static	void	set_revision_info

	(
	STRING	rev
	)

	{
	int		rev1, rev2;

	/* Parse the revision number */
	rev1 = 0;
	rev2 = 0;
	if (!blank(rev))
		{
		int		r1, r2, n;

		n = sscanf(rev, "%d.%d", &r1, &r2);
		if (n == 2)
			{
			rev1 = r1;
			rev2 = r2;
			}
		}

	/* Set features according to revision */
	NotMetafile = (LOGICAL) (rev1<=0);
	UseMKS      = (LOGICAL) ((rev1==1 && rev2>=2) || rev1>=2);
	UseLabels   = (LOGICAL) ((rev1==1 && rev2>=3));
	UseOffset   = (LOGICAL) ((rev1==1 && rev2>=3) || rev1>=2);
	UseProj     = (LOGICAL) ((rev1==1 && rev2>=4) || rev1>=2);
	UseArea     = (LOGICAL) ((rev1==1 && rev2>=5) || rev1>=2);
	UseCal      = (LOGICAL) (rev1>=2);
	UseGust     = (LOGICAL) (rev1>=2);
	}


/***********************************************************************
*                                                                      *
*      s u r f a c e _ t o _ M K S                                     *
*                                                                      *
*      Convert the given surface to MKS units, for metafile standard   *
*      revisions prior to 1.2.                                         *
*                                                                      *
***********************************************************************/

static	void	surface_to_MKS

	(
	SURFACE	sfc,
	STRING	element,
	STRING	level
	)

	{
	FpaConfigElementStruct	*edef;
	FpaConfigUnitStruct		*udef;
	static	USPEC	units = {NullString, 1.0, 0.0};

	if (!sfc)           return;
	if (blank(element)) return;
	if (blank(level))   return;

	/* Newer revisions are already in MKS */
	if (UseMKS) return;

	/* We must convert - Find the standard units for this element */
	/* This information is no longer maintained with fpav4 */
	/* The best guess is to use the units given in the I/O precision */
	edef = identify_element(element);
	if (!edef) return;
	udef = edef->elem_io->units;
	if (!udef) return;
	define_uspec(&units, udef->name, udef->factor, udef->offset);

	/* Simply set the units spec to look as if we have already converted */
	/* from MKS to the existing units. */
	/* Then convert back to MKS */
	define_surface_units(sfc, &units);
	change_surface_units(sfc, &MKS_UNITS);
	}

/***********************************************************************
*                                                                      *
*      a r e a s _ f r o m _ c u r v e s                               *
*                                                                      *
*      Convert a set of curves to a set of areas.                   *
*                                                                      *
***********************************************************************/

#ifdef MAYBE_LATER
static	SET			areas_from_curves

	(
	METAFILE	meta,
	STRING		entity,
	STRING		element,
	STRING		level
	)

	{
	FIELD	fld;
	SET		cset, aset;
	CURVE	curve;
	AREA	area;
	int		i;
	DIVSTAT	dstat;
	STRING	cat;

	/* Create an empty area set */
	aset = make_mf_set(meta, "area", entity, element, level);

	/* Search for the corresponding curve set and remove it from meta */
	fld = take_mf_field(meta, "set", "curve", "c", element, level);
	if (!fld)           return aset;
	cset = fld->data.set;
	if (!cset)          return aset;
	if (cset->num <= 0) return aset;

	/* Steal the background */
	if (cset->bgnd)
		{
		curve = (CURVE) cset->bgnd;
		define_set_bg_attribs(aset, curve->attrib);
		}

	/* Steal the boundary segments from the curve set */
	area = NullArea;
	for (i=0; i<cset->num; i++)
		{
		curve = (CURVE) cset->list[i];
		if (!curve) continue;

		(void) get_attribute(curve->attrib, AttribCategory, &cat);
		if (same(cat, "hole"))
			{
			if (!area) continue;
			add_area_hole(area, curve->line);
			curve->line = NullLine;
			}

		else if (same(cat, "divide"))
			{
			if (!area) continue;
			subarea = ???
			divide_area(area, subarea, curve->line, NullSubAreaList,
						NullSubAreaList, &dstat);
			curve->line = NullLine;
			}

		else
			{
			area = create_area(NullString, NullString, NullString);
			define_area_attribs(area, curve->attrib);
			define_area_boundary(area, curve->line);
			add_item_to_set(aset, (ITEM) area);
			curve->line = NullLine;
			}
		}

	/* Now destroy the curve set */
	destroy_field(fld);

	return aset;
	}
#endif /* MAYBE_LATER */

/***********************************************************************
*                                                                      *
*      g e t _ r e v                                                   *
*      g e t _ p r o j e c t i o n                                     *
*      g e t _ m a p d e f                                             *
*      g e t _ c o m p o n e n t                                       *
*      g e t _ u n i t s                                               *
*      g e t _ i n f o                                                 *
*      g e t _ b k g n d                                               *
*      g e t _ r a s t e r                                             *
*      g e t _ t s t a m p                                             *
*      g e t _ e n t i t y                                             *
*      g e t _ s u b f i e l d s                                       *
*      g e t _ b g v a l                                               *
*      g e t _ v a l u e                                               *
*      g e t _ b s p l i n e                                           *
*      g e t _ g r i d                                                 *
*      g e t _ c u r v e                                               *
*      g e t _ l a b e l                                               *
*      g e t _ m a r k                                                 *
*      g e t _ b a r b                                                 *
*      g e t _ b u t t o n                                             *
*      g e t _ s p o t                                                 *
*      g e t _ a r e a s e t _ a r e a                                 *
*      g e t _ a r e a s e t _ h o l e                                 *
*      g e t _ a r e a s e t _ d i v i d e                             *
*      g e t _ p l o t _ p o i n t                                     *
*      g e t _ p l o t _ p o i n t _ a s _ s p o t                     *
*      g e t _ l c h a i n                                             *
*      g e t _ l c h a i n _ n o d e                                   *
*                                                                      *
*      Routines used for reading metafile commands.                    *
*                                                                      *
***********************************************************************/

static	const int	ncl         = 1024;
static	char		line[1024]  = "";	/* should be able to say line[ncl] */
static	char		labst[1024] = "";	/* should be able to say labst[ncl] */
static	LOGICAL		status;
static	STRING		subptr, valptr, labptr;

/**********************************************************************/

static LOGICAL	get_rev

	(
	FILE	*fp,
	STRING	rev
	)

	{
	if ( !getfileline(fp, line, ncl) )               return FALSE;
	strcpy_arg(rev, line, &status);		if (!status) return FALSE;

	return TRUE;
	}

/**********************************************************************/

static LOGICAL		get_projection

	(
	FILE		*fp,
	MAP_PROJ	*mproj
	)

	{
	PROJ_DEF	proj;
	char		pname[60], ref1[20], ref2[20], ref3[20], ref4[20], ref5[20];

	if ( !getfileline(fp, line, ncl) )               return FALSE;
	if ( !UseProj )                                  return FALSE;

	/* Any changes here must be replicated in  parse_metafile_projection() */
	strcpy_arg(pname, line, &status);	if (!status) return FALSE;
	strcpy_arg(ref1, line, &status);	if (!status) (void) strcpy(ref1, "");
	strcpy_arg(ref2, line, &status);	if (!status) (void) strcpy(ref2, "");
	strcpy_arg(ref3, line, &status);	if (!status) (void) strcpy(ref3, "");
	strcpy_arg(ref4, line, &status);	if (!status) (void) strcpy(ref4, "");
	strcpy_arg(ref5, line, &status);	if (!status) (void) strcpy(ref5, "");

	/* Define a projection as specified */
	if (!define_projection_by_name(&proj, pname, ref1, ref2, ref3, ref4, ref5))
		{
		(void) pr_error("Metafile", "Unrecognized projection in metafile\n");
		BailOut = TRUE;
		return FALSE;
		}

	/* Define the map part of the map projection */
	define_map_projection(mproj, &proj, NullMapDef, NullGridDef);
	return TRUE;
	}

/**********************************************************************/

static LOGICAL		get_mapdef

	(
	FILE		*fp,
	MAP_PROJ	*mproj
	)

	{
	char		lval[20];
	int			ilat, ilon, iref;
	float		olat, olon, lref, xmin, ymin, xmax, ymax, units;
	MAP_DEF		map;

	if ( !getfileline(fp, line, ncl) )                 return FALSE;

	/* Any changes here must be replicated in  parse_metafile_mapdef() */
	if (UseProj)
		{
		strcpy_arg(lval, line, &status);	if (!status) return FALSE;
		olat = read_lat(lval, &status);		if (!status) return FALSE;
		strcpy_arg(lval, line, &status);	if (!status) return FALSE;
		olon = read_lon(lval, &status);		if (!status) return FALSE;
		strcpy_arg(lval, line, &status);	if (!status) return FALSE;
		lref = read_lon(lval, &status);		if (!status) return FALSE;
		xmin = float_arg(line, &status);	if (!status) return FALSE;
		ymin = float_arg(line, &status);	if (!status) return FALSE;
		}
	else
		{
		ilat = int_arg(line, &status);	if (!status) return FALSE;
		ilon = int_arg(line, &status);	if (!status) return FALSE;
		iref = int_arg(line, &status);	if (!status) return FALSE;
		olat = degrees(ilat);
		olon = -degrees(ilon);
		lref = -90.0 - degrees(iref);
		/* Re-read olat, olon, lref from string buffer with %g */
		/* so it will match one which has already been written out */
		(void) sprintf(lval, "%g", olat);  olat = read_lat(lval, &status);
		(void) sprintf(lval, "%g", olon);  olon = read_lon(lval, &status);
		(void) sprintf(lval, "%g", lref);  lref = read_lon(lval, &status);
		xmin = 0.0;
		ymin = 0.0;
		if (ilat==0 && ilon==0 && iref==0)
			{
			copy_projection(&mproj->projection, &NoProjDef);
			}
		}
	xmax  = float_arg(line, &status);	if (!status) return FALSE;
	ymax  = float_arg(line, &status);	if (!status) return FALSE;
	units = float_arg(line, &status);	if (!status) return FALSE;

	/* Define a map definition as specified */
	map.olat  = olat;
	map.olon  = olon;
	map.lref  = lref;
	map.xorg  = -xmin;
	map.yorg  = -ymin;
	map.xlen  = xmax - xmin;
	map.ylen  = ymax - ymin;
	map.units = units;

	/* Define the map part of the map projection */
	define_map_projection(mproj, &mproj->projection, &map, NullGridDef);
	return TRUE;
	}

/**********************************************************************/

static LOGICAL		get_component

	(
	FILE		*fp,
	COMP_INFO	*cinfo
	)

	{
	char	clist[20];

	if ( !getfileline(fp, line, ncl) )					return FALSE;
	strcpy_arg(clist, line, &status);	if (!status)	return FALSE;

	cinfo->need = XY_Comp;
	cinfo->have = No_Comp;
	if (strchr(clist, 'x')) add_component(cinfo, X_Comp);
	if (strchr(clist, 'y')) add_component(cinfo, Y_Comp);
	return TRUE;
	}

/**********************************************************************/

static LOGICAL	get_units

	(
	FILE	*fp,
	STRING	utype,
	STRING	umode
	)

	{
	if ( !getfileline(fp, line, ncl) )               return FALSE;
	strcpy_arg(utype, line, &status);	if (!status) return FALSE;
	strcpy_arg(umode, line, &status);	if (!status) (void) strcpy(umode, "");

	return TRUE;
	}

/**********************************************************************/

static LOGICAL	get_info

	(
	FILE	*fp,
	STRING	process,
	STRING	param,
	STRING	*value
	)

	{
	if (value) *value = NullString;

	if ( !getfileline(fp, line, ncl) )               return FALSE;
	strcpy_arg(process, line, &status);	if (!status) return FALSE;
	strcpy_arg(param,   line, &status);	if (!status) return FALSE;
	strcpy_arg(labst,   line, &status);	if (!status) (void) strcpy(labst, "");
	if (value) *value = labst;

	return TRUE;
	}

/**********************************************************************/

static LOGICAL	get_bkgnd

	(
	FILE	*fp,
	STRING	file
	)

	{
	if ( !getfileline(fp, line, ncl) )                   return FALSE;
	strcpy_arg(file, line, &status);		if (!status) return FALSE;

	return TRUE;
	}

/**********************************************************************/

static LOGICAL	get_tstamp

	(
	FILE	*fp,
	STRING	rtime,
	STRING	vtime,
	STRING	model
	)

	{
	if ( !getfileline(fp, line, ncl) )               return FALSE;
	strcpy_arg(rtime, line, &status);	if (!status) return FALSE;
	strcpy_arg(vtime, line, &status);	if (!status) return FALSE;
	strcpy_arg(model, line, &status);	if (!status) (void) strcpy(model, "");

	return TRUE;
	}

/**********************************************************************/

static LOGICAL	get_field

	(
	FILE	*fp,
	STRING	entity,
	STRING	element,
	STRING	level
	)

	{
	char	type[41], elab[41], llab[41];
	int		ftype;
	STRING	ent;
	FpaConfigElementStruct	*edef;
	FpaConfigLevelStruct	*ldef;

	/* Element and level are converted to "normal" names on input */
	if ( !getfileline(fp, line, ncl) )                           return FALSE;

	strcpy_arg(type, line, &status);			if (!status)     return FALSE;
	ftype = field_data_type(type);				if (ftype < 0)   return FALSE;
	ent   = entity_from_field_type(ftype);		if (blank(ent))  return FALSE;
	(void) safe_strcpy(entity, ent);

	strcpy_arg(elab, line, &status);			if (!status)     return FALSE;
	edef = identify_element(elab);				if (!edef)       return FALSE;
	(void) safe_strcpy(element, edef->name);

	strcpy_arg(llab, line, &status);			if (!status)     return FALSE;
	ldef = identify_level(llab);				if (!ldef)       return FALSE;
	(void) safe_strcpy(level, ldef->name);

	return TRUE;
	}

/**********************************************************************/

static LOGICAL	get_entity

	(
	FILE	*fp,
	STRING	element,
	STRING	level,
	CAL		*cal,
	int		*nsub,
	STRING	**slist,
	STRING	**vlist
	)

	{
	char	elab[41], llab[41];
	FpaConfigElementStruct	*edef;
	FpaConfigLevelStruct	*ldef;

	static	int		nbuf  = 0;
	static	STRING	*sbuf = NullStringList;
	static	STRING	*vbuf = NullStringList;

	/* Element and level are converted to "normal" names on input */
	if ( !getfileline(fp, line, ncl) )                           return FALSE;

	strcpy_arg(elab, line, &status);			if (!status)     return FALSE;
	edef = identify_element(elab);				if (!edef)       return FALSE;
	(void) safe_strcpy(element, edef->name);

	strcpy_arg(llab, line, &status);			if (!status)     return FALSE;
	ldef = identify_level(llab);				if (!ldef)       return FALSE;
	(void) safe_strcpy(level, ldef->name);

	FREELIST(sbuf, nbuf);
	FREELIST(vbuf, nbuf);
	nbuf = 0;
	while ( !blank(line) )
		{
		nbuf++;
		sbuf = GETMEM(sbuf, STRING, nbuf);
		vbuf = GETMEM(vbuf, STRING, nbuf);
		sbuf[nbuf-1] = strdup_arg(line);
		vbuf[nbuf-1] = strdup_arg(line);
		}

	if (NotNull(cal) && nbuf>0)
		{
		if (NotNull(*cal)) *cal = CAL_destroy(*cal);
		*cal = CAL_create_by_name(element, level);
		CAL_set_defaults(*cal, sbuf[0], vbuf[0], CAL_NO_VALUE);
		}

	*nsub  = nbuf;
	*slist = sbuf;
	*vlist = vbuf;
	return TRUE;
	}

/**********************************************************************/

static LOGICAL	get_subfields

	(
	FILE	*fp,
	int		*nsub,
	STRING	**slist,
	STRING	**vlist
	)

	{
	int		i, num;
	LOGICAL	getting = FALSE;

	static	int		nbuf  = 0;
	static	STRING	*sbuf = NullStringList;
	static	STRING	*vbuf = NullStringList;

	if ( !getfileline(fp, line, ncl) )               return FALSE;
	num = int_arg(line, &status);		if (!status) return FALSE;

	FREELIST(sbuf, nbuf);
	FREELIST(vbuf, nbuf);
	nbuf = num;
	sbuf = GETMEM(sbuf, STRING, nbuf);
	vbuf = GETMEM(vbuf, STRING, nbuf);

	/* Read list of subfield names and types */
	for (i=0; i<num; i++)
		{
		getting = FALSE;
		if ( blank(line) )
			{
			getting = TRUE;
			if ( !getfileline(fp, line, ncl) ) return FALSE;
			}
		sbuf[i] = strdup_arg(line);

		getting = FALSE;
		if ( blank(line) )
			{
			getting = TRUE;
			if ( !getfileline(fp, line, ncl) ) return FALSE;
			}
		vbuf[i] = strdup_arg(line);
		}
	if (i < num)
		{
		if (getting) ungetfileline(fp);
		(void) pr_error("Metafile", "Subfield count mismatch in metafile\n");
		BailOut = TRUE;
		return FALSE;
		}

	*nsub  = nbuf;
	*slist = sbuf;
	*vlist = vbuf;
	return TRUE;
	}

/**********************************************************************/

static LOGICAL	get_bgval

	(
	FILE	*fp,
	CAL		cal
	)

	{
	if ( !getfileline(fp, line, ncl) )                   return FALSE;
	if (UseLabels)
		{
		strcpy_arg(labst, line, &status);	if (!status) return FALSE;
		CAL_set_attribute(cal, CALuserlabel, labst);
		}
	return TRUE;
	}

/**********************************************************************/

static LOGICAL	get_value

		(
		FILE	*fp,
		STRING	entity,
		STRING	element,
		STRING	level,
		CAL		*cal
		)

	{
	char	name[41], val[256];
	int		num, ii;
	LOGICAL	getting = FALSE;
	LOGICAL	have_category, have_linetype, have_scatteredtype, have_labeltype;
	STRING	cat;

	FpaConfigFieldStruct	*fid;

	*cal = NullCal;

	if ( !getfileline(fp, line, ncl) )               return FALSE;
	num = int_arg(line, &status);		if (!status) return FALSE;

	*cal = CAL_create_by_name(element, level);

	/* Read list of attribute names and values */
	have_category      = FALSE;
	have_linetype      = FALSE;
	have_scatteredtype = FALSE;
	have_labeltype     = FALSE;
	for (ii=0; ii<num; ii++)
		{
		getting = FALSE;
		if ( blank(line) )
			{
			getting = TRUE;
			if ( !getfileline(fp, line, ncl) )           return FALSE;
			}
		strcpy_arg(name, line, &status);	if (!status) break;
		getting = FALSE;
		if ( blank(line) )
			{
			getting = TRUE;
			if ( !getfileline(fp, line, ncl) )           return FALSE;
			}
		strcpy_arg(val, line, &status);		if (!status) break;

		if (same(name, CALcategory))      have_category      = TRUE;
		if (same(name, CALlinetype))      have_linetype      = TRUE;
		if (same(name, CALscatteredtype)) have_scatteredtype = TRUE;
		if (same(name, CALlabeltype))     have_labeltype     = TRUE;

		CAL_add_attribute(*cal, name, val);
		}
	if (ii < num)
		{
		if (getting) ungetfileline(fp);
		(void) pr_error("Metafile", "Attribute count mismatch in metafile\n");
		BailOut = TRUE;
		return FALSE;
		}

	/* Set the line or scattered or label type (if necessary) */
	fid = get_field_info(element, level);
	if (NotNull(fid))
		{

		/* Set line type (if necessary) */
		if (same(entity, "c") && fid->element->fld_type == FpaC_LINE)
			{
			if (!have_linetype)
				{

				/* Set line type from category */
				if (have_category)
					{
					cat = CAL_get_attribute(*cal, CALcategory);
					CAL_add_attribute(*cal, CALlinetype, cat);
					}

				/* Set line type to default */
				else
					{
					CAL_add_attribute(*cal, CALlinetype, CALlinetypedefault);
					}
				}
			}

		/* Set scattered type (if necessary) */
		else if (same(entity, "d") && fid->element->fld_type == FpaC_SCATTERED)
			{
			if (!have_scatteredtype)
				{

				/* Set scattered type from category */
				if (have_category)
					{
					cat = CAL_get_attribute(*cal, CALcategory);
					CAL_add_attribute(*cal, CALscatteredtype, cat);
					}

				/* Set scattered type to default */
				else
					{
					CAL_add_attribute(*cal, CALscatteredtype,
													CALscatteredtypedefault);
					}
				}
			}

		/* Set label type (if necessary) */
		else if (same(entity, "d"))
			{
			if (!have_labeltype)
				{

				/* Set label type based on type of field */
				switch (fid->element->fld_type)
					{

					case FpaC_CONTINUOUS:
							CAL_add_attribute(*cal, CALlabeltype,
														FpaLabellingContinuous);
							break;

					case FpaC_VECTOR:
							CAL_add_attribute(*cal, CALlabeltype,
														FpaLabellingVector);
							break;

					case FpaC_DISCRETE:
							CAL_add_attribute(*cal, CALlabeltype,
														FpaLabellingDiscrete);
							break;

					case FpaC_WIND:
							CAL_add_attribute(*cal, CALlabeltype,
														FpaLabellingWindArea);
							break;

					case FpaC_LINE:
							CAL_add_attribute(*cal, CALlabeltype,
														FpaLabellingLine);
							break;
					}
				}
			}
		}

	/* Assume entity-d is a plot or a set of spots ... so fire label rules */
	if (same(entity, "d"))
		CAL_invoke_label_rules_by_name(*cal, element, level);

	/* Assume entity-n is a link chain node ... so fire node rules */
	else if (same(entity, "n"))
		CAL_invoke_lnode_rules_by_name(*cal, element, level);

	/* Otherwise ... fire entry rules */
	else
		CAL_invoke_entry_rules_by_name(*cal, element, level);

	return TRUE;
	}

/**********************************************************************/

static LOGICAL	get_bspline

		(
		FILE	*fp,
		SURFACE	*sfc
		)

	{
	int		m, n;
	char	org1[20], org2[20];
	POINT	origin;
	float	precision, orient, gridlen;
	int		icv, ncv;
	float	*cvbuf;
	LOGICAL	getting = FALSE;

	*sfc = NullSfc;
	origin[X] = 0;
	origin[Y] = 0;
	orient    = 0;

	if ( !getfileline(fp, line, ncl) )                       return FALSE;
	precision  = float_arg(line, &status);		if (!status) return FALSE;
	m          = int_arg(line, &status);		if (!status) return FALSE;
	n          = int_arg(line, &status);		if (!status) return FALSE;
	if (UseOffset)
		{
		strcpy_arg(org1, line, &status);		if (!status) return FALSE;
		strcpy_arg(org2, line, &status);		if (!status) return FALSE;
		orient = float_arg(line, &status);		if (!status) return FALSE;
		if (!meta_point(org1, org2, origin))                 return FALSE;
		}
	gridlen    = float_arg(line, &status);		if (!status) return FALSE;

	/* Read in control vertex values */
	ncv   = m * n;
	cvbuf = INITMEM(float, ncv);
	for (icv=0; icv<ncv; icv++)
		{
		getting = FALSE;
		if ( blank(line) )
			{
			getting = TRUE;
			if ( !getfileline(fp, line, ncl) )
				{
				FREEMEM(cvbuf);
				return FALSE;
				}
			}
		cvbuf[icv] = precision * float_arg(line, &status);
		if ( !status )
			{
			FREEMEM(cvbuf);
			if (getting) ungetfileline(fp);
			(void) pr_error("Metafile",
				"Spline vertex count mismatch in metafile\n");
			BailOut = TRUE;
			return FALSE;
			}
		}

	/* Construct the surface */
	*sfc = create_surface();
	define_surface_spline(*sfc, m, n, &BaseMap, origin, orient,
				gridlen, cvbuf, n);
	define_surface_units(*sfc, &MKS_UNITS);
	remap_surface(*sfc, &NewMap, &BaseMap);
	FREEMEM(cvbuf);
	return TRUE;
	}

/**********************************************************************/

static LOGICAL	get_bspline2D

		(
		FILE	*fp,
		SURFACE	*sfc
		)

	{
	int		m, n;
	char	org1[20], org2[20];
	char	coord[20], order[20];
	LOGICAL	bymap, byblk;
	POINT	origin;
	float	val, precision, orient, gridlen;
	int		icv, ncv, ip, np, ico;
	float	*xcvbuf, *ycvbuf;
	LOGICAL	getting = FALSE;

	*sfc = NullSfc;
	origin[X] = 0;
	origin[Y] = 0;
	orient    = 0;

	if ( !getfileline(fp, line, ncl) )                       return FALSE;
	strcpy_arg(coord, line, &status);			if (!status) return FALSE;
	strcpy_arg(order, line, &status);			if (!status) return FALSE;
	bymap = same(coord, "map");		/* Map L/U vs Compass E/N */
	byblk = same(order, "block");	/* All U then V vs UVUV... */
	precision  = float_arg(line, &status);		if (!status) return FALSE;
	m          = int_arg(line, &status);		if (!status) return FALSE;
	n          = int_arg(line, &status);		if (!status) return FALSE;
	if (UseOffset)
		{
		strcpy_arg(org1, line, &status);		if (!status) return FALSE;
		strcpy_arg(org2, line, &status);		if (!status) return FALSE;
		orient = float_arg(line, &status);		if (!status) return FALSE;
		if (!meta_point(org1, org2, origin))                 return FALSE;
		}
	gridlen    = float_arg(line, &status);		if (!status) return FALSE;

	/* Read in control vertex values */
	ncv    = m * n;
	np     = ncv * 2;
	xcvbuf = INITMEM(float, ncv);
	ycvbuf = INITMEM(float, ncv);
	icv    = 0;
	ico    = 0;
	for (ip=0; ip<np; ip++)
		{
		getting = FALSE;
		if ( blank(line) )
			{
			getting = TRUE;
			if ( !getfileline(fp, line, ncl) )
				{
				FREEMEM(xcvbuf);
				FREEMEM(ycvbuf);
				return FALSE;
				}
			}
		val = precision * float_arg(line, &status);
		if ( !status )
			{
			FREEMEM(xcvbuf);
			FREEMEM(ycvbuf);
			if (getting) ungetfileline(fp);
			(void) pr_error("Metafile",
				"Spline vertex count mismatch in metafile\n");
			BailOut = TRUE;
			return FALSE;
			}

		if (ico==0) xcvbuf[icv] = val;
		else        ycvbuf[icv] = val;
		if (byblk)
			{
			icv++;
			if (icv >= ncv)
				{
				icv -= ncv;
				ico++;
				}
			}
		else
			{
			ico++;
			if (ico >= 2)
				{
				ico -= 2;
				icv++;
				}
			}
		}

	/* Construct the surface */
	*sfc = create_surface();
	define_surface_spline_2D(*sfc, m, n, &BaseMap, origin, orient,
				gridlen, xcvbuf, ycvbuf, n);
	define_surface_units(*sfc, &MKS_UNITS);
	remap_surface(*sfc, &NewMap, &BaseMap);
	FREEMEM(xcvbuf);
	FREEMEM(ycvbuf);
	return TRUE;
	}

/**********************************************************************/

static LOGICAL	get_grid

	(
	FILE	*fp,
	SURFACE	*sfc
	)

	{
	POINT	origin;
	char	org1[20], org2[20];
	float	precision, orient, gridlen;
	int		nx, ny;
	int		ix, iy;
	float	*vbuf, **gbuf;
	LOGICAL	getting = FALSE;

	*sfc = NullSfc;
	origin[X] = 0;
	origin[Y] = 0;
	orient    = 0;

	if ( !getfileline(fp, line, ncl) )                       return FALSE;
	precision  = float_arg(line, &status);		if (!status) return FALSE;
	nx         = int_arg(line, &status);		if (!status) return FALSE;
	ny         = int_arg(line, &status);		if (!status) return FALSE;
	if (UseOffset)
		{
		strcpy_arg(org1, line, &status);		if (!status) return FALSE;
		strcpy_arg(org2, line, &status);		if (!status) return FALSE;
		orient = float_arg(line, &status);		if (!status) return FALSE;
		if (!meta_point(org1, org2, origin))                 return FALSE;
		}
	gridlen    = float_arg(line, &status);		if (!status) return FALSE;

	/* Read in grid point values */
	vbuf  = INITMEM(float, nx*ny);
	gbuf  = INITMEM(float *, ny);
	for (iy=0; iy<ny; iy++)
		{
		gbuf[iy] = vbuf + iy*nx;
		for (ix=0; ix<nx; ix++)
			{
			getting = FALSE;
			if ( blank(line) )
				{
				getting = TRUE;
				if ( !getfileline(fp, line, ncl) )
					{
					FREEMEM(vbuf);
					FREEMEM(gbuf);
					return FALSE;
					}
				}
			gbuf[iy][ix] = precision * float_arg(line, &status);
			if ( !status )
				{
				FREEMEM(vbuf);
				FREEMEM(gbuf);
				if (getting) ungetfileline(fp);
				(void) pr_error("Metafile",
					"Grid point count mismatch in metafile\n");
				BailOut = TRUE;
				return FALSE;
				}
			}
		}

	/* Construct the surface */
	*sfc = create_surface();
	grid_surface(*sfc, gridlen, nx, ny, gbuf);
	(*sfc)->sp.origin[X] = origin[X];
	(*sfc)->sp.origin[Y] = origin[Y];
	(*sfc)->sp.orient    = orient;
	define_surface_units(*sfc, &MKS_UNITS);
	remap_surface(*sfc, &NewMap, &BaseMap);
	FREEMEM(gbuf);
	FREEMEM(vbuf);
	return TRUE;
	}

static LOGICAL  get_mask
	(
	FILE	*fp,
	BITMASK	*mask
	)

	{
#ifdef MACHINE_PCLINUX
	int			result;
	unsigned long		srcsize, masksize;
	UNCHAR 		*srcbuf, *bits;

	/* Read mask dimensions */
	if ( !getfileline(fp, line, ncl) )                   return FALSE;
	srcsize     = int_arg(line,   &status); if (!status) return FALSE;
	masksize    = int_arg(line,   &status); if (!status) return FALSE;

	if (*mask) *mask = destroy_raster_mask(*mask);
	if ( !RasterReadAll ) 
		{
		if ( fseek(fp, srcsize, SEEK_CUR) != 0 ) return FALSE;
		else return TRUE;
		}

	/* allocate memory for mask */
	srcbuf     = INITMEM(UNCHAR, srcsize);
	if (!srcbuf) return FALSE;
	bits       = INITMEM(UNCHAR, masksize);
	if (!bits) 	
		{
		FREEMEM(srcbuf);
		return FALSE;
		}

	/* Read the src buffer */
	result = fread(srcbuf, 1, srcsize, fp);
	result = uncompress(bits, &masksize, srcbuf, srcsize);
	switch (result)
		{
		case Z_OK:			printf("Z_OK!\n");						break;
		case Z_MEM_ERROR: 	printf("Not enough memory\n"); 			break;
		case Z_BUF_ERROR: 	printf("Output buffer too small\n"); 	break;
		case Z_DATA_ERROR: 	printf("Input data was corrupted\n"); 	break;
		}

	if ( Z_OK != result )
		{
		FREEMEM(srcbuf);
		FREEMEM(bits);
		return FALSE;
		}

	/* Populate the mask object */
	*mask = create_raster_mask();
	(void)define_raster_mask(*mask, bits, masksize);

	/* All done */
	FREEMEM(srcbuf);
	return TRUE;
#else
	pr_error("Metafile","Reading zlib compressed data is not available on this machine.\n");
	return FALSE;
#endif
	}
	
static LOGICAL	get_raster

	(
	FILE	*fp,
	RASTER	*raster,
	BITMASK	mask
	)

	{
#ifdef MACHINE_PCLINUX
	long		pngsize, nrow, ncol, gridsize;
	int			bpp, type; 
	float		init;
	int			width, height;
	UNCHAR		*pngbuf=NullPtr(UNCHAR *), *grid=NullPtr(UNCHAR *);
	UNCHAR		*initp=NullPtr(UNCHAR *);


	/* Read raster dimensions */
	if ( !getfileline(fp, line, ncl) )               return FALSE;
	pngsize = int_arg(line,   &status); if (!status) return FALSE;
	nrow    = int_arg(line,   &status); if (!status) return FALSE;
	ncol    = int_arg(line,   &status); if (!status) return FALSE;
	bpp     = int_arg(line,   &status); if (!status) return FALSE;
	type    = int_arg(line,   &status); if (!status) return FALSE;
	init    = float_arg(line, &status); 

	/* init may not be present. if not it should be a null pointer */
	if (!status) 
		initp = (UNCHAR *) 0;
	else
		initp = (UNCHAR *)&init;

	if ( !RasterReadAll ) 
		{
		/* Skip past raster data */
		if ( fseek(fp, pngsize, SEEK_CUR) != 0 ) return FALSE;
		}
	else
		{
		/* Read the raster data */ 

		gridsize = nrow * ncol * bpp; /* Calculate total grid size */

		/* allocate memory for grid */
		pngbuf = INITMEM(UNCHAR, pngsize);
		if (!pngbuf)	return FALSE;
		grid   = INITMEM(UNCHAR, gridsize);
		if (!grid)
			{
			FREEMEM(pngbuf);
			return FALSE;
			}

		/* Read PNG buffer */
		fread(pngbuf, 1, pngsize, fp);
		if ( read_png(pngbuf, &width, &height, bpp*8, grid) < 0 )
			{
			FREEMEM(pngbuf);
			return FALSE;
			}

		FREEMEM(pngbuf);
		/* Check that dimentions are correct */
		if ((mask) && (mask->size != MASK_SIZE(width,height)))
			{
			FREEMEM(grid);
			return FALSE;
			}
		}

	/* All went well populate the raster object */
	*raster = create_raster_obj();
	(void) define_raster(*raster, grid, copy_raster_mask(mask), nrow, ncol, bpp, 
						 initp, (RASTER_TYPE)type);
	(void) define_raster_units(*raster, &MKS_UNITS);

	/* TODO: Not sure if I should do this or not.
	remap_raster(*raster, &NewMap, &BaseMap);
	*/

	/* All done */
	return TRUE;
#else
	pr_error("Metafile","Reading PNG images is not available on this machine.\n");
	return FALSE;
#endif
	}

/**********************************************************************/

static LOGICAL	get_curve

	(
	FILE	*fp,
	CURVE	*curve,
	CAL		cal
	)

	{
	int		ip, np;
	char	sense[5];
	char	xval[20], yval[20];
	POINT	p;
	LOGICAL	getting = FALSE;

	*curve = NullCurve;

	if ( !getfileline(fp, line, ncl) )                   return FALSE;
	strcpy_arg(sense, line, &status);		if (!status) return FALSE;
	if (UseLabels)
		{
		strcpy_arg(labst, line, &status);	if (!status) return FALSE;
		CAL_set_attribute(cal, CALuserlabel, labst);
		}
	np = int_arg(line, &status);			if (!status) return FALSE;

	/* Construct the curve and read the points */
	subptr = CAL_get_attribute(cal, CALcategory);
	valptr = CAL_get_attribute(cal, CALautolabel);
	labptr = CAL_get_attribute(cal, CALuserlabel);
	*curve = create_curve(subptr, valptr, labptr);
	define_curve_attribs(*curve, cal);
	define_curve_sense(*curve, sense[0]);
	for (ip=0; ip<np; ip++)
		{
		getting = FALSE;
		if ( blank(line) )
			{
			getting = TRUE;
			if ( !getfileline(fp, line, ncl) )           return FALSE;
			}
		strcpy_arg(xval, line, &status);	if (!status) break;
		getting = FALSE;
		if ( blank(line) )
			{
			getting = TRUE;
			if ( !getfileline(fp, line, ncl) )           return FALSE;
			}
		strcpy_arg(yval, line, &status);	if (!status) break;

		if (!meta_project(xval, yval, p))                break;
		add_point_to_curve(*curve, p);
		}
	if (ip < np)
		{
		if (getting) ungetfileline(fp);
		(void) pr_error("Metafile", "Point count mismatch in metafile\n");
		BailOut = TRUE;
		return FALSE;
		}

	return TRUE;
	}

/**********************************************************************/

static LOGICAL	get_curves

	(
	FILE	*fp,
	SET		*curves,
	CAL		cal
	)

	{
	int		ip, np;
	char	sense[5];
	char	xval[20], yval[20];
	POINT	p;
	CURVE	curve;
	LOGICAL	getting = FALSE;

	*curves = NullSet;

	if ( !getfileline(fp, line, ncl) )                   return FALSE;
	strcpy_arg(sense, line, &status);		if (!status) return FALSE;
	if (UseLabels)
		{
		strcpy_arg(labst, line, &status);	if (!status) return FALSE;
		CAL_set_attribute(cal, CALuserlabel, labst);
		}
	np = int_arg(line, &status);			if (!status) return FALSE;

	/* Construct the curve set and read the points */
	subptr = CAL_get_attribute(cal, CALcategory);
	valptr = CAL_get_attribute(cal, CALautolabel);
	labptr = CAL_get_attribute(cal, CALuserlabel);
	*curves = create_set("curve");
	curve   = create_curve(subptr, valptr, labptr);
	define_curve_attribs(curve, cal);
	define_curve_sense(curve, sense[0]);
	add_item_to_set(*curves, (ITEM) curve);
	start_wrap_detect();
	for (ip=0; ip<np; ip++)
		{
		getting = FALSE;
		if ( blank(line) )
			{
			getting = TRUE;
			if ( !getfileline(fp, line, ncl) )           break;
			}
		strcpy_arg(xval, line, &status);	if (!status) break;
		getting = FALSE;
		if ( blank(line) )
			{
			getting = TRUE;
			if ( !getfileline(fp, line, ncl) )           break;
			}
		strcpy_arg(yval, line, &status);	if (!status) break;

		if (!meta_project(xval, yval, p))                break;
		if (wrap_detected())
			{
			curve = create_curve(subptr, valptr, labptr);
			define_curve_attribs(curve, cal);
			define_curve_sense(curve, sense[0]);
			add_item_to_set(*curves, (ITEM) curve);
			}
		add_point_to_curve(curve, p);
		}
	end_wrap_detect();
	if (ip < np)
		{
		if (getting) ungetfileline(fp);
		(void) pr_error("Metafile", "Point count mismatch in metafile\n");
		BailOut = TRUE;
		return FALSE;
		}

	return TRUE;
	}

/**********************************************************************/

static LOGICAL	get_label

	(
	FILE	*fp,
	LABEL	*label,
	CAL		cal
	)

	{
	float	angle;
	char	text[256];
	char	xval[20], yval[20];
	POINT	p;

	*label = NullLabel;

	if ( !getfileline(fp, line, ncl) )               return FALSE;
	angle = float_arg(line, &status);	if (!status) return FALSE;
	strcpy_arg(xval, line, &status);	if (!status) return FALSE;
	strcpy_arg(yval, line, &status);	if (!status) return FALSE;
	if (!meta_project(xval, yval, p))                return FALSE;
	strcpy_arg(text, line, &status);	if (!status) (void) safe_strcpy(text, "");
	if (!blank(text)) CAL_set_attribute(cal, CALuserlabel, text);

	subptr = CAL_get_attribute(cal, CALcategory);
	valptr = CAL_get_attribute(cal, CALautolabel);
	labptr = CAL_get_attribute(cal, CALuserlabel);
	if (blank(labptr)) CAL_set_attribute(cal, CALuserlabel, valptr);
	labptr = CAL_get_attribute(cal, CALuserlabel);
	if (blank(valptr)) CAL_set_attribute(cal, CALautolabel, labptr);
	valptr = CAL_get_attribute(cal, CALautolabel);
	*label = create_label(subptr, valptr, labptr, p, angle);
	define_label_attribs(*label, cal);
	return TRUE;
	}

/**********************************************************************/

static LOGICAL	get_mark

	(
	FILE	*fp,
	MARK	*mark,
	CAL		cal
	)

	{
	float	angle;
	int		type;
	char	xval[20], yval[20];
	POINT	p;

	*mark = NullMark;

	if ( !getfileline(fp, line, ncl) )                   return FALSE;
	angle = float_arg(line, &status);		if (!status) return FALSE;
	strcpy_arg(xval, line, &status);		if (!status) return FALSE;
	strcpy_arg(yval, line, &status);		if (!status) return FALSE;
	if (!meta_project(xval, yval, p))                    return FALSE;
	type  = int_arg(line, &status);			if (!status) return FALSE;
	if (UseLabels)
		{
		strcpy_arg(labst, line, &status);	if (!status) return FALSE;
		CAL_set_attribute(cal, CALuserlabel, labst);
		}

	subptr = CAL_get_attribute(cal, CALcategory);
	valptr = CAL_get_attribute(cal, CALautolabel);
	labptr = CAL_get_attribute(cal, CALuserlabel);
	*mark = create_mark(subptr, valptr, labptr, p, angle);
	define_mark_attribs(*mark, cal);
	(*mark)->mspec.type = type;
	return TRUE;
	}

/**********************************************************************/

static LOGICAL	get_barb

	(
	FILE	*fp,
	BARB	*barb,
	CAL		cal
	)

	{
	float	dir, speed, gust, mcf;
	char	xval[20], yval[20];
	POINT	p;

	*barb = NullBarb;

	if ( !getfileline(fp, line, ncl) )                   return FALSE;
	strcpy_arg(xval, line, &status);		if (!status) return FALSE;
	strcpy_arg(yval, line, &status);		if (!status) return FALSE;
	if (!meta_project(xval, yval, p))                    return FALSE;
	dir    = float_arg(line, &status);		if (!status) return FALSE;
	speed  = float_arg(line, &status);		if (!status) return FALSE;
	if (UseGust)
		{
		gust = float_arg(line, &status);	if (!status) return FALSE;
		}
	else
		{
		gust = 0.0;
		}
	mcf    = float_arg(line, &status);		if (!status) return FALSE;
	if (UseLabels)
		{
		strcpy_arg(labst, line, &status);	if (!status) return FALSE;
		CAL_set_attribute(cal, CALuserlabel, labst);
		}

	subptr = CAL_get_attribute(cal, CALcategory);
	valptr = CAL_get_attribute(cal, CALautolabel);
	labptr = CAL_get_attribute(cal, CALuserlabel);
	*barb = create_barb(subptr, p, dir, speed, gust);
	define_barb_attribs(*barb, cal);
	return TRUE;
	}

/**********************************************************************/

static LOGICAL	get_button

	(
	FILE	*fp,
	BUTTON	*button,
	CAL		cal
	)

	{
	char	xval[20], yval[20];
	POINT	p, q;
	BOX		box;

	*button = NullButton;

	if ( !getfileline(fp, line, ncl) )                   return FALSE;
	strcpy_arg(xval, line, &status);		if (!status) return FALSE;
	strcpy_arg(yval, line, &status);		if (!status) return FALSE;
	if (!meta_project(xval, yval, p))                    return FALSE;
	strcpy_arg(xval, line, &status);		if (!status) return FALSE;
	strcpy_arg(yval, line, &status);		if (!status) return FALSE;
	if (!meta_project(xval, yval, q))                    return FALSE;
	if (UseLabels)
		{
		strcpy_arg(labst, line, &status);	if (!status) return FALSE;
		CAL_set_attribute(cal, CALuserlabel, labst);
		}

	box.left   = MIN(p[X], q[X]);
	box.right  = MAX(p[X], q[X]);
	box.bottom = MIN(p[Y], q[Y]);
	box.top    = MAX(p[Y], q[Y]);
	subptr = CAL_get_attribute(cal, CALcategory);
	valptr = CAL_get_attribute(cal, CALautolabel);
	labptr = CAL_get_attribute(cal, CALuserlabel);
	*button = create_button(subptr, valptr, labptr, &box);
	define_button_attribs(*button, cal);
	return TRUE;
	}

/**********************************************************************/

static LOGICAL	get_spot

	(
	FILE	*fp,
	STRING	element,
	STRING	level,
	SPOT	*spot,
	CAL		cal
	)

	{
	char	xval[20], yval[20];
	char	class[128], ftype[128];
	char	type[128], name[128], eval[128];
	POINT	p;
	int		imem, nmem;
	SPFEAT	feature;
	LOGICAL	getting = FALSE;

	*spot = NullSpot;

	/* Get spot information */
	if ( !getfileline(fp, line, ncl) )               return FALSE;
	strcpy_arg(xval, line, &status);	if (!status) return FALSE;
	strcpy_arg(yval, line, &status);	if (!status) return FALSE;
	if (!meta_project(xval, yval, p))                return FALSE;
	strcpy_arg(class, line, &status);	if (!status) return FALSE;
	strcpy_arg(ftype, line, &status);	if (!status) return FALSE;
	feature = spot_feature(ftype);
	nmem = int_arg(line, &status);		if (!status) nmem = 0;

	/* Create the SPOT Object */
	*spot = create_spot(p, class, feature, cal);

	/* Add spot location */
	CAL_add_location((CAL) (*spot)->attrib, &BaseMap, (*spot)->anchor);

	/* Invoke "label" type rules */
	CAL_invoke_label_rules_by_name((CAL) (*spot)->attrib, element, level);

	/* Ignore members for now */
	for (imem=0; imem<nmem; imem++)
		{
		getting = FALSE;
		if (imem>0 || blank(line))
			{
			getting = TRUE;
			if ( !getfileline(fp, line, ncl) )           break;
			}

		strcpy_arg(type, line, &status);	if (!status) break;
		strcpy_arg(name, line, &status);	if (!status) break;
		strcpy_arg(eval, line, &status);	if (!status) break;
		}
	if (imem < nmem)
		{
		if (getting) ungetfileline(fp);
		(void) pr_error("Metafile", "Member count mismatch in metafile\n");
		BailOut = TRUE;
		return FALSE;
		}

	return TRUE;
	}

/**********************************************************************/

static LOGICAL	get_areaset_area

	(
	FILE	*fp,
	SET		aset,
	CAL		cal
	)

	{
	int		ip, np;
	char	xval[20], yval[20];
	LINE	bound;
	AREA	area;
	POINT	p;
	LOGICAL	getting = FALSE;

	if ( !getfileline(fp, line, ncl) )                   return FALSE;
	if (UseLabels)
		{
		strcpy_arg(labst, line, &status);	if (!status) return FALSE;
		CAL_set_attribute(cal, CALuserlabel, labst);
		}
	np = int_arg(line, &status);			if (!status) return FALSE;

	/* Construct the boundary line and read the points */
	bound = create_line();
	for (ip=0; ip<np; ip++)
		{
		getting = FALSE;
		if ( blank(line) )
			{
			getting = TRUE;
			if ( !getfileline(fp, line, ncl) )           return FALSE;
			}
		strcpy_arg(xval, line, &status);	if (!status) break;
		getting = FALSE;
		if ( blank(line) )
			{
			getting = TRUE;
			if ( !getfileline(fp, line, ncl) )           return FALSE;
			}
		strcpy_arg(yval, line, &status);	if (!status) break;

		if (!meta_project(xval, yval, p))                break;
		add_point_to_line(bound, p);
		}

	/* Did we get all the points? */
	if (ip < np)
		{
		if (getting) ungetfileline(fp);
		(void) pr_error("Metafile", "Point count mismatch in metafile\n");
		BailOut = TRUE;
		bound = destroy_line(bound);
		return FALSE;
		}

	/* Build a new area */
	subptr = CAL_get_attribute(cal, CALcategory);
	valptr = CAL_get_attribute(cal, CALautolabel);
	labptr = CAL_get_attribute(cal, CALuserlabel);
	area = create_area(subptr, valptr, labptr);
	define_area_attribs(area, cal);
	define_area_boundary(area, bound);
	add_item_to_set(aset, (ITEM) area);

	return TRUE;
	}

/**********************************************************************/

static LOGICAL	get_areaset_hole

	(
	FILE	*fp,
	SET		aset,
	CAL		cal
	)

	{
	int		ip, np;
	char	xval[20], yval[20];
	LINE	hole;
	AREA	area;
	POINT	p;
	LOGICAL	getting = FALSE;

	if ( !getfileline(fp, line, ncl) )               return FALSE;
	np = int_arg(line, &status);		if (!status) return FALSE;

	/* Construct the hole boundary line and read the points */
	hole = create_line();
	for (ip=0; ip<np; ip++)
		{
		getting = FALSE;
		if ( blank(line) )
			{
			getting = TRUE;
			if ( !getfileline(fp, line, ncl) )           return FALSE;
			}
		strcpy_arg(xval, line, &status);	if (!status) break;
		getting = FALSE;
		if ( blank(line) )
			{
			getting = TRUE;
			if ( !getfileline(fp, line, ncl) )           return FALSE;
			}
		strcpy_arg(yval, line, &status);	if (!status) break;

		if (!meta_project(xval, yval, p))                break;
		add_point_to_line(hole, p);
		}

	/* Did we get all the points? */
	if (ip < np)
		{
		if (getting) ungetfileline(fp);
		(void) pr_error("Metafile", "Point count mismatch in metafile\n");
		BailOut = TRUE;
		hole = destroy_line(hole);
		return FALSE;
		}

	/* Add the hole to the last area */
	if (aset->num <= 0)
		{
		hole = destroy_line(hole);
		return FALSE;
		}
	area = (AREA) aset->list[aset->num-1];
	add_area_hole(area, hole);

	return TRUE;
	}

/**********************************************************************/

static LOGICAL	get_areaset_divide

	(
	FILE	*fp,
	SET		aset,
	CAL		lcal,
	CAL		rcal
	)

	{
	int		ip, np, isub;
	char	xval[20], yval[20];
	char	llab[1024], rlab[1024];
	LINE	divide, newdiv;
	AREA	area;
	SUBAREA	subarea, lsubarea, rsubarea;
	POINT	p;
	LOGICAL	getting = FALSE;
	LOGICAL	inside;

	if ( !getfileline(fp, line, ncl) )               return FALSE;
	if (UseLabels)
		{
		strcpy_arg(llab, line, &status);	if (!status) return FALSE;
		strcpy_arg(rlab, line, &status);	if (!status) return FALSE;
		CAL_set_attribute(lcal, CALuserlabel, llab);
		CAL_set_attribute(rcal, CALuserlabel, rlab);
		}
	np = int_arg(line, &status);		if (!status) return FALSE;

	/* Construct the divide boundary line and read the points */
	divide = create_line();
	for (ip=0; ip<np; ip++)
		{
		getting = FALSE;
		if ( blank(line) )
			{
			getting = TRUE;
			if ( !getfileline(fp, line, ncl) )           return FALSE;
			}
		strcpy_arg(xval, line, &status);	if (!status) break;
		getting = FALSE;
		if ( blank(line) )
			{
			getting = TRUE;
			if ( !getfileline(fp, line, ncl) )           return FALSE;
			}
		strcpy_arg(yval, line, &status);	if (!status) break;

		if (!meta_project(xval, yval, p))                break;
		add_point_to_line(divide, p);
		}

	/* Did we get all the points? */
	if (ip < np)
		{
		if (getting) ungetfileline(fp);
		(void) pr_error("Metafile", "Point count mismatch in metafile\n");
		BailOut = TRUE;
		divide = destroy_line(divide);
		return FALSE;
		}

	/* The last area is the one to be divided */
	if (aset->num <= 0)
		{
		divide = destroy_line(divide);
		return FALSE;
		}
	area = (AREA) aset->list[aset->num-1];

	/* See which subarea to divide */
	subarea = NullSubArea;
	if (area->subareas != NullSubAreaList)
		{
		ip = np/2;
		copy_point(p, divide->points[ip]);
		for (isub=0; isub<area->numdiv+1; isub++)
			{
			subarea = area->subareas[isub];
			subarea_test_point(subarea, p, NullFloat, NullPoint,
								NullInt, NullInt, &inside);
			if (inside) break;
			subarea = NullSubArea;
			}
		if (!subarea)
			{
			divide = destroy_line(divide);
			return FALSE;
			}
		}

	/* Prepare the dividing line for this subarea */
	newdiv = prepare_area_divline(area, subarea, divide, (DIVSTAT *)0);
	if (!newdiv)
		{
		divide = destroy_line(divide);
		return FALSE;
		}

	/* Now divide the subarea */
	if (!divide_area(area, subarea, newdiv, &lsubarea, &rsubarea, (DIVSTAT *)0))
		{
		divide = destroy_line(divide);
		return FALSE;
		}

	subptr = CAL_get_attribute(lcal, CALcategory);
	valptr = CAL_get_attribute(lcal, CALautolabel);
	labptr = CAL_get_attribute(lcal, CALuserlabel);
	define_subarea_value(lsubarea, subptr, valptr, labptr);
	define_subarea_attribs(lsubarea, lcal);
	subptr = CAL_get_attribute(rcal, CALcategory);
	valptr = CAL_get_attribute(rcal, CALautolabel);
	labptr = CAL_get_attribute(rcal, CALuserlabel);
	define_subarea_value(rsubarea, subptr, valptr, labptr);
	define_subarea_attribs(rsubarea, rcal);

	return TRUE;
	}

/**********************************************************************/

static LOGICAL	get_plot_point

	(
	FILE	*fp,
	PLOT	plot
	)

	{
	char	xval[20], yval[20];
	POINT	p;
	int		ip, isub;
	PSUB	*sub;
	char	text[256];
	float	dir, speed, fval;
	int		ival;

	if (!plot)
		{
		flush_line(fp);
		return FALSE;
		}

	if ( !getfileline(fp, line, ncl) )               return FALSE;
	strcpy_arg(xval, line, &status);	if (!status) return FALSE;
	strcpy_arg(yval, line, &status);	if (!status) return FALSE;
	if (!meta_project(xval, yval, p))                return FALSE;

	/* Add the point to the plot */
	ip = plot->numpts;
	add_point_to_plot(plot, p);

	/* Read the buffer of data values */
	for (isub=0; isub<plot->nsubs; isub++)
		{
		sub = plot->subs + isub;
		if (same(sub->type, "area"))
			{
			strcpy_arg(text, line, &status);	if (!status) return FALSE;
			define_subfld_value(plot, sub->name, ip, text, NullString, 0, 0,
								0., 0.);
			}
		else if (same(sub->type, "curve"))
			{
			strcpy_arg(text, line, &status);	if (!status) return FALSE;
			define_subfld_value(plot, sub->name, ip, text, NullString, 0, 0,
								0., 0.);
			}
		else if (same(sub->type, "label"))
			{
			strcpy_arg(text, line, &status);	if (!status) return FALSE;
			define_subfld_value(plot, sub->name, ip, text, text, 0, 0, 0., 0.);
			}
		else if (same(sub->type, "mark"))
			{
			strcpy_arg(text, line, &status);	if (!status) return FALSE;
			define_subfld_value(plot, sub->name, ip, text, text, 0, 0, 0., 0.);
			}
		else if (same(sub->type, "barb"))
			{
			dir   = float_arg(line, &status);	if (!status) return FALSE;
			speed = float_arg(line, &status);	if (!status) return FALSE;
			define_subfld_value(plot, sub->name, ip, NullString, NullString,
								0, 0, dir, speed);
			}
		else if (same(sub->type, "button"))
			{
			strcpy_arg(text, line, &status);	if (!status) return FALSE;
			define_subfld_value(plot, sub->name, ip, text, text, 0, 0, 0., 0.);
			}
		else if (same(sub->type, "int"))
			{
			ival = int_arg(line, &status);		if (!status) return FALSE;
			define_subfld_value(plot, sub->name, ip, NullString, NullString,
								ival, 0, 0., 0.);
			}
		else if (same(sub->type, "float"))
			{
			fval = float_arg(line, &status);	if (!status) return FALSE;
			define_subfld_value(plot, sub->name, ip, NullString, NullString,
								0, 0, fval, 0.);
			}
		}

	return TRUE;
	}

/**********************************************************************/

static LOGICAL	get_plot_point_as_spot

	(
	FILE	*fp,
	STRING	element,
	STRING	level,
	SPOT	*spot,
	CAL		cal,
	STRING	*names,
	STRING	*types,
	int		nsub
	)

	{
	char		xval[20], yval[20];
	POINT		p;
	int			isub;
	char		text[256];
	float		fval;
	int			ival;
	WIND_VAL	wv;

	int			ii;
	STRING		value, mclass;
	FpaConfigFieldStruct				*fdef;
	FpaConfigElementScatteredTypeStruct	*stypes;
	FpaConfigElementLabellingStruct		*labelling;

	*spot = NullSpot;

	if ( !getfileline(fp, line, ncl) )               return FALSE;
	strcpy_arg(xval, line, &status);	if (!status) return FALSE;
	strcpy_arg(yval, line, &status);	if (!status) return FALSE;
	if (!meta_project(xval, yval, p))                return FALSE;

	/* Read the buffer of data values - stuff into cal */
	for (isub=0; isub<nsub; isub++)
		{
		if (same(types[isub], "area"))
			{
			strcpy_arg(text, line, &status);	if (!status) return FALSE;
			CAL_add_attribute(cal, names[isub], text);
			}
		else if (same(types[isub], "curve"))
			{
			strcpy_arg(text, line, &status);	if (!status) return FALSE;
			CAL_add_attribute(cal, names[isub], text);
			}
		else if (same(types[isub], "label"))
			{
			strcpy_arg(text, line, &status);	if (!status) return FALSE;
			CAL_add_attribute(cal, names[isub], text);
			}
		else if (same(types[isub], "mark"))
			{
			strcpy_arg(text, line, &status);	if (!status) return FALSE;
			CAL_add_attribute(cal, names[isub], text);
			}
		else if (same(types[isub], "barb"))
			{
			wv.dir   = float_arg(line, &status);	if (!status) return FALSE;
			wv.speed = float_arg(line, &status);	if (!status) return FALSE;
			wv.gust  = 0;
			CAL_add_attribute(cal, names[isub], build_wind_value_string(&wv));
			}
		else if (same(types[isub], "button"))
			{
			strcpy_arg(text, line, &status);	if (!status) return FALSE;
			CAL_add_attribute(cal, names[isub], text);
			}
		else if (same(types[isub], "int"))
			{
			ival = int_arg(line, &status);		if (!status) return FALSE;
			int_string(ival, text, sizeof(text));
			CAL_add_attribute(cal, names[isub], text);
			}
		else if (same(types[isub], "float"))
			{
			fval = float_arg(line, &status);	if (!status) return FALSE;
			(void) strcpy(text, fformat(fval, 6));
			CAL_add_attribute(cal, names[isub], text);
			}
		}

	/* Get the detailed field information */
	fdef = get_field_info(element, level);
	if (NotNull(fdef))
		{

		/* Identify "class" based on the type of scattered data */
		switch (fdef->element->fld_type)
			{

			/* Scattered fields use the scattered types block */
			case FpaC_SCATTERED:

				value = CAL_get_attribute(cal, CALscatteredtype);
				ii = identify_scattered_type_by_name(element, level,
						value, &stypes);
				if (NotNull(stypes) && ii >= 0)
					mclass = stypes->type_classes[ii];
				else
					mclass = FpaCdefaultScatteredTypesClass;
				break;

			/* All other fields use the labelling block */
			case FpaC_CONTINUOUS:
			case FpaC_VECTOR:
			case FpaC_DISCRETE:
			case FpaC_WIND:
			case FpaC_LINE:
			default:

				value = CAL_get_attribute(cal, CALlabeltype);
				ii = identify_labelling_type_by_name(element, level,
						value, &labelling);
				if (NotNull(stypes) && ii >= 0)
					mclass = labelling->type_classes[ii];
				else
					mclass = FpaCdefaultLabellingTypesClass;
				break;
			}
		}

	/* Define the spot using the "class" for presentation */
	*spot = create_spot(p, mclass, AttachNone, cal);

	/* Add spot location */
	CAL_add_location((CAL) (*spot)->attrib, &BaseMap, (*spot)->anchor);

	/* Invoke "label" type rules */
	CAL_invoke_label_rules_by_name((CAL) (*spot)->attrib, element, level);

	return TRUE;
	}

/**********************************************************************/

static LOGICAL	get_plot_point_as_both

	(
	FILE	*fp,
	STRING	element,
	STRING	level,
	PLOT	plot,
	SPOT	*spot,
	CAL		cal,
	STRING	*names,
	STRING	*types,
	int		nsub
	)

	{
	char		xval[20], yval[20];
	POINT		p;
	int			ip, isub;
	PSUB		*sub;
	char		text[256];
	float		fval;
	int			ival;
	WIND_VAL	wv;

	int			ii;
	STRING		value, mclass;
	FpaConfigFieldStruct				*fdef;
	FpaConfigElementScatteredTypeStruct	*stypes;
	FpaConfigElementLabellingStruct		*labelling;

	*spot = NullSpot;

	if (!plot)
		{
		flush_line(fp);
		return FALSE;
		}
	if (plot->nsubs != nsub)
		{
		flush_line(fp);
		return FALSE;
		}

	if ( !getfileline(fp, line, ncl) )               return FALSE;
	strcpy_arg(xval, line, &status);	if (!status) return FALSE;
	strcpy_arg(yval, line, &status);	if (!status) return FALSE;
	if (!meta_project(xval, yval, p))                return FALSE;

	/* Add the point to the plot */
	ip = plot->numpts;
	add_point_to_plot(plot, p);

	/* Read the buffer of data values - stuff into cal */
	for (isub=0; isub<nsub; isub++)
		{
		sub = plot->subs + isub;

		if (same(types[isub], "area"))
			{
			strcpy_arg(text, line, &status);	if (!status) return FALSE;
			CAL_add_attribute(cal, names[isub], text);
			define_subfld_value(plot, sub->name, ip, text, NullString, 0, 0,
								0., 0.);
			}
		else if (same(types[isub], "curve"))
			{
			strcpy_arg(text, line, &status);	if (!status) return FALSE;
			CAL_add_attribute(cal, names[isub], text);
			define_subfld_value(plot, sub->name, ip, text, NullString, 0, 0,
								0., 0.);
			}
		else if (same(types[isub], "label"))
			{
			strcpy_arg(text, line, &status);	if (!status) return FALSE;
			CAL_add_attribute(cal, names[isub], text);
			define_subfld_value(plot, sub->name, ip, text, text, 0, 0, 0., 0.);
			}
		else if (same(types[isub], "mark"))
			{
			strcpy_arg(text, line, &status);	if (!status) return FALSE;
			CAL_add_attribute(cal, names[isub], text);
			define_subfld_value(plot, sub->name, ip, text, text, 0, 0, 0., 0.);
			}
		else if (same(types[isub], "barb"))
			{
			wv.dir   = float_arg(line, &status);	if (!status) return FALSE;
			wv.speed = float_arg(line, &status);	if (!status) return FALSE;
			wv.gust  = 0;
			CAL_add_attribute(cal, names[isub], build_wind_value_string(&wv));
			define_subfld_value(plot, sub->name, ip, NullString, NullString,
								0, 0, wv.dir, wv.speed);
			}
		else if (same(types[isub], "button"))
			{
			strcpy_arg(text, line, &status);	if (!status) return FALSE;
			CAL_add_attribute(cal, names[isub], text);
			define_subfld_value(plot, sub->name, ip, text, text, 0, 0, 0., 0.);
			}
		else if (same(types[isub], "int"))
			{
			ival = int_arg(line, &status);		if (!status) return FALSE;
			int_string(ival, text, sizeof(text));
			CAL_add_attribute(cal, names[isub], text);
			define_subfld_value(plot, sub->name, ip, NullString, NullString,
								ival, 0, 0., 0.);
			}
		else if (same(types[isub], "float"))
			{
			fval = float_arg(line, &status);	if (!status) return FALSE;
			(void) strcpy(text, fformat(fval, 6));
			CAL_add_attribute(cal, names[isub], text);
			define_subfld_value(plot, sub->name, ip, NullString, NullString,
								0, 0, fval, 0.);
			}
		}

	/* Get the detailed field information */
	fdef = get_field_info(element, level);
	if (NotNull(fdef))
		{

		/* Identify "class" based on the type of scattered data */
		switch (fdef->element->fld_type)
			{

			/* Scattered fields use the scattered types block */
			case FpaC_SCATTERED:

				value = CAL_get_attribute(cal, CALscatteredtype);
				ii = identify_scattered_type_by_name(element, level,
						value, &stypes);
				if (NotNull(stypes) && ii >= 0)
					mclass = stypes->type_classes[ii];
				else
					mclass = FpaCdefaultScatteredTypesClass;
				break;

			/* All other fields use the labelling block */
			case FpaC_CONTINUOUS:
			case FpaC_VECTOR:
			case FpaC_DISCRETE:
			case FpaC_WIND:
			case FpaC_LINE:
			default:

				value = CAL_get_attribute(cal, CALlabeltype);
				ii = identify_labelling_type_by_name(element, level,
						value, &labelling);
				if (NotNull(stypes) && ii >= 0)
					mclass = labelling->type_classes[ii];
				else
					mclass = FpaCdefaultLabellingTypesClass;
				break;
			}
		}

	/* Define the spot using the "class" for presentation */
	*spot = create_spot(p, mclass, AttachNone, cal);

	/* Add spot location */
	CAL_add_location((CAL) (*spot)->attrib, &BaseMap, (*spot)->anchor);

	/* Invoke "label" type rules */
	CAL_invoke_label_rules_by_name((CAL) (*spot)->attrib, element, level);

	return TRUE;
	}

/**********************************************************************/

static LOGICAL	get_lchain

	(
	FILE	*fp,
	SET		lset,
	CAL		cal,
	int		*lnum
	)

	{
	int		splus, eplus, tplus, minterp, numl, numc;
	char	xtime[20];
	LCHAIN	lchain;

	/* Get the link chain information */
	if ( !getfileline(fp, line, ncl) )               return FALSE;
	strcpy_arg(xtime, line, &status);	if (!status) return FALSE;
	splus   = int_arg(line, &status);	if (!status) return FALSE;
	eplus   = int_arg(line, &status);	if (!status) return FALSE;
	minterp = int_arg(line, &status);	if (!status) return FALSE;
	numl    = int_arg(line, &status);	if (!status) return FALSE;
	numc    = 0;

	/* Check for old format "control" nodes            */
	/*  ... and treat them the same as "normal" nodes! */
	if (!blank(line))
		{
		numc  = int_arg(line, &status);	if (!status) return FALSE;
		numl += numc;
		}

	/* Create the LCHAIN Object */
	lchain = create_lchain();

	/* Set the link chain reference time */
	define_lchain_reference_time(lchain, xtime);

	/* Ensure that start time precedes end time */
	if (eplus < splus)
		{
		tplus = splus;
		splus = eplus;
		eplus = tplus;
		}

	/* Set the link chain start and end time for nodes */
	define_lchain_start_time(lchain, splus);
	define_lchain_end_time(lchain, eplus);

	/* Set the link track interpolation delta */
	define_lchain_interp_delta(lchain, minterp);

	/* Set the link chain attributes (including default attributes) */
	define_lchain_attribs(lchain, cal);

	/* Add the link chain to set */
	add_item_to_set(lset, (ITEM) lchain);

	/* Return number of nodes to read */
	*lnum = numl;
	return TRUE;
	}

/**********************************************************************/

static LOGICAL	get_lchain_node

	(
	FILE	*fp,
	SET		lset,
	CAL		ncal,
	int		*lnum
	)

	{
	char	xval[20], yval[20];
	POINT	p;
	char	type[128];
	int		mplus, attach, mtype, imem, nnodes;
	LCHAIN	lchain;
	LNODE	lnode;

	/* Get the link node information */
	if ( !getfileline(fp, line, ncl) )               return FALSE;
	strcpy_arg(xval, line, &status);	if (!status) return FALSE;
	strcpy_arg(yval, line, &status);	if (!status) return FALSE;
	if (!meta_project(xval, yval, p))                return FALSE;
	strcpy_arg(type, line, &status);	if (!status) return FALSE;
	mplus  = int_arg(line, &status);	if (!status) return FALSE;
	if ( !blank(line) )
		{
		attach = int_arg(line, &status);	if (!status) return FALSE;
		mtype  = int_arg(line, &status);	if (!status) return FALSE;
		imem   = int_arg(line, &status);	if (!status) return FALSE;
		}
	else
		{
		attach = -1;
		mtype  =  0;
		imem   = -1;
		}

	/* Add a link node */
	if (same(type, FpaNodeClass_Normal)
			|| same(type, FpaNodeClass_NormalGuess)
			|| same(type, FpaNodeClass_Control)
			|| same(type, FpaNodeClass_ControlGuess)
			|| same(type, FpaNodeClass_Floating))
		{

		/* Define an LNODE Object */
		lnode = create_lnode(mplus);
		if (same(type, FpaNodeClass_Normal))
			define_lnode_type(lnode, TRUE, FALSE, LchainNode);
		else if (same(type, FpaNodeClass_NormalGuess))
			define_lnode_type(lnode, TRUE, TRUE, LchainNode);
		else if (same(type, FpaNodeClass_Control))
			define_lnode_type(lnode, TRUE, FALSE, LchainControl);
		else if (same(type, FpaNodeClass_ControlGuess))
			define_lnode_type(lnode, TRUE, TRUE, LchainControl);
		else if (same(type, FpaNodeClass_Floating))
			define_lnode_type(lnode, TRUE, FALSE, LchainFloating);
		define_lnode_node(lnode, p);
		define_lnode_attribs(lnode, ncal);
		define_lnode_attach(lnode, attach, mtype, imem);

		/* Add the LNODE Object to the last link chain */
		if (lset->num <= 0)
			{
			lnode = destroy_lnode(lnode);
			return FALSE;
			}
		lchain = (LCHAIN) lset->list[lset->num-1];
		nnodes = lchain->lnum;
		(void) add_lchain_lnode(lchain, lnode);

		/* Warning if a link node was not added */
		if (lchain->lnum == nnodes)
			{
			if (same(type, FpaNodeClass_Normal))
				(void) pr_warning("Metafile",
					"Problem with %s node (chain: %d  mplus: %d)\n",
					FpaNodeClass_Normal, lset->num, mplus);
			else if (same(type, FpaNodeClass_NormalGuess))
				(void) pr_warning("Metafile",
					"Problem with %s node (chain: %d  mplus: %d)\n",
					FpaNodeClass_NormalGuess, lset->num, mplus);
			if (same(type, FpaNodeClass_Control))
				(void) pr_warning("Metafile",
					"Problem with %s node (chain: %d  mplus: %d)\n",
					FpaNodeClass_Control, lset->num, mplus);
			else if (same(type, FpaNodeClass_ControlGuess))
				(void) pr_warning("Metafile",
					"Problem with %s node (chain: %d  mplus: %d)\n",
					FpaNodeClass_ControlGuess, lset->num, mplus);
			else if (same(type, FpaNodeClass_Floating))
				(void) pr_warning("Metafile",
					"Problem with %s node (chain: %d  mplus: %d)\n",
					FpaNodeClass_Floating, lset->num, mplus);
			}

		/* Keep track of number of link nodes still to read */
		(*lnum)--;
		return TRUE;
		}

	/* Add an old format guess link node */
	else if (same(type, FpaNodeClass_Guess))
		{

		(void) pr_warning("Metafile",
			"Old format link node type: %s should be changed to: %s\n",
			FpaNodeClass_Guess, FpaNodeClass_NormalGuess);

		/* Define an LNODE Object */
		lnode = create_lnode(mplus);
		define_lnode_type(lnode, TRUE, TRUE, LchainNode);
		define_lnode_node(lnode, p);
		define_lnode_attribs(lnode, ncal);
		define_lnode_attach(lnode, attach, mtype, imem);

		/* Add the LNODE Object to the last link chain */
		if (lset->num <= 0)
			{
			lnode = destroy_lnode(lnode);
			return FALSE;
			}
		lchain = (LCHAIN) lset->list[lset->num-1];
		nnodes = lchain->lnum;
		(void) add_lchain_lnode(lchain, lnode);

		/* Warning if a link node was not added */
		if (lchain->lnum == nnodes)
			{
			(void) pr_warning("Metafile",
				"Problem with %s node (chain: %d  mplus: %d)\n",
				FpaNodeClass_Guess, lset->num, mplus);
			}

		/* Keep track of number of link nodes still to read */
		(*lnum)--;
		return TRUE;
		}
	}

/**********************************************************************/

static LOGICAL	get_old_area

	(
	FILE	*fp,
	SET		aset,
	CAL		cal
	)

	{
	int		iseg, nseg, mseg;
	char	text[128], fwd;
	LOGICAL	getting = FALSE;

	if (!aset)
		{
		flush_line(fp);
		return FALSE;
		}

	if ( !getfileline(fp, line, ncl) )                   return FALSE;
	if (UseLabels)
		{
		strcpy_arg(labst, line, &status);	if (!status) return FALSE;
		CAL_set_attribute(cal, CALuserlabel, labst);
		}
	nseg = int_arg(line, &status);			if (!status) return FALSE;

	/* Read list of segment IDs and attach corresponding segments */
	for (iseg=0; iseg<nseg; iseg++)
		{
		getting = FALSE;
		if ( blank(line) )
			{
			getting = TRUE;
			if ( !getfileline(fp, line, ncl) )           return FALSE;
			}
		strcpy_arg(text, line, &status);	if (!status) break;
		if ( sscanf(text, "%cSEG%d", &fwd, &mseg) != 2)  break;
		}
	if (iseg < nseg)
		{
		if (getting) ungetfileline(fp);
		(void) pr_error("Metafile", "Segment count mismatch in metafile\n");
		BailOut = TRUE;
		return FALSE;
		}
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*      Test programs:                                                  *
*                                                                      *
***********************************************************************/

#ifdef META_STANDALONE

#include <unistd.h>

void	main(int argc, STRING *argv)
	{
	char		buf[1024], ans[10];
	STRING		cwd;
	METAFILE	meta;
	int			nslist;
	STRING		*slist;
	int			ifld;
	FIELD		fld;
	SURFACE		sfc;
	SET			set;
	PLOT		plot;
	int			imem;
	AREA		area;
	BARB		barb;
	BUTTON		button;
	CURVE		curve;
	LABEL		label;
	MARK		mark;
	int			mode;
	int			iarg;
	RASTER      raster=NullRaster;

	fpalib_license(FpaAccessLib);

	cwd = getcwd(buf, sizeof(buf));
	printf("Working directory: %s\n", cwd);

	nslist = setup_files(NullString, &slist);
	if (!define_setup(nslist, slist))
		{
		printf("Problem with setup file\n");
		printf("Aborting!\n");
		exit(1);
		}

	chdir(cwd);

	iarg = 1;
	while (1)
		{
		if (iarg < argc)
			{
			(void) strcpy(buf, argv[iarg]);
			printf("\n");
			printf("Input metafile name: %s\n", buf);
			iarg++;
			}
		else
			{
			printf("\n");
			printf("Input metafile name: ");
			getfileline(stdin, buf, sizeof(buf));
			if (blank(buf)) break;
			}
		set_metafile_input_mode(MetaRasterReadMetadata);	
		meta = read_metafile(buf, NullMapProj);
		set_metafile_input_mode(MetaRasterReadAll);	
		if (!meta) continue;

		printf("Metafile: %d fields\n", meta->numfld);

		for (ifld=0; ifld<meta->numfld; ifld++)
			{
			fld = meta->fields[ifld];
			if (!fld) continue;

			if (fld->ftype == FtypeSfc)
				{
				sfc = fld->data.sfc;
				printf("   Field %d: surface\n", ifld);
				}
			else if (fld->ftype  == FtypeRaster)
				{
				printf("   Field %d: bitmask\n", ifld);
				}

			else if (fld->ftype == FtypeSet)
				{
				set = fld->data.set;
				if (!set) continue;
				printf("   Field %d: %s set: %d items\n", ifld, set->type,
						set->num);

				for (imem=0; imem<set->num; imem++)
					{
					if (same(set->type, "area"))
						area = (AREA) set->list[imem];
					else if (same(set->type, "barb"))
						barb = (BARB) set->list[imem];
					else if (same(set->type, "button"))
						button = (BUTTON) set->list[imem];
					else if (same(set->type, "curve"))
						curve = (CURVE) set->list[imem];
					else if (same(set->type, "label"))
						label = (LABEL) set->list[imem];
					else if (same(set->type, "mark"))
						mark = (MARK) set->list[imem];
					printf("      Set member %d\n", imem);
					}
				}

			else if (fld->ftype == FtypePlot)
				{
				plot = fld->data.plot;
				printf("   Field %d: plot\n", ifld);
				}
			}

		printf("\n");
		printf("Output metafile name: ");
		getfileline(stdin, buf, sizeof(buf));

		if (blank(buf))
			{
			destroy_metafile(meta);
			continue;
			}

		if (find_file(buf))
			{
			printf("   File exists - Overwrite? ");
			getfileline(stdin, ans, sizeof(ans));
			if (!same_start_ic(ans, "y"))
				{
				destroy_metafile(meta);
				continue;
				}
			}

		mode = 0;
		printf("   Old format? ");
		getfileline(stdin, ans, sizeof(ans));
		if (same_start_ic(ans, "y")) mode |= META_OLDFMT;

		printf("   Lat-lon mode? ");
		getfileline(stdin, ans, sizeof(ans));
		if (same_start_ic(ans, "y")) mode |= META_LATLON;

		write_metafile_special(buf, meta, 4, mode);
		destroy_metafile(meta);
		}
	}

#endif /* META_STANDALONE */
