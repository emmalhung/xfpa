/***********************************************************************
*                                                                      *
*     g u i d a n c e . c                                              *
*                                                                      *
*     This module of the INteractive GRaphics EDitor (INGRED)          *
*     handles all display and edit functions for guidance fields       *
*     overlayed on the current depiction.                              *
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

#undef SAVE_CALC

#undef DEBUG_SYNC

/* Active guidance field edit objects */
static	GLIST		*GuidFld   = NULL;
static	GFRAME		*GuidChart = NULL;
static	STRING		GuidElem   = NULL;		/* active element */
static	STRING		GuidLevel  = NULL;		/* active level */
static	STRING		GuidSource = NULL;		/* active source */
static	STRING		GuidSubSrc = NULL;		/* active subsource */
static	STRING		GuidRtime  = NULL;		/* active run time */
static	SURFACE		GuidSfc    = NullSfc;
static	SET			GuidSet    = NullSet;
static	PLOT		GuidPlot   = NullPlot;
static	SET			GuidLabs   = NullSet;



/***********************************************************************
*                                                                      *
*     Functions which control the entire guidance module:              *
*                                                                      *
***********************************************************************/
/***********************************************************************
*                                                                      *
*     i n i t _ g u i d a n c e                                        *
*                                                                      *
***********************************************************************/

LOGICAL	init_guidance(void)

	{
	int		iguid;
	GLIST	*guid;

	for (iguid=0; iguid<MaxGuid; iguid++)
		{
		guid = GuidFlds + iguid;
		guid->dn      = NullDn;
		guid->charts  = NULL;
		guid->nchart  = 0;
		guid->tag     = NULL;
		guid->elem    = NULL;
		guid->level   = NULL;
		guid->source  = NULL;
		guid->subsrc  = NULL;
		guid->rtime   = NULL;
		guid->erase   = FALSE;
		guid->colour  = -1;
		guid->style   = -1;
		guid->width   = -1;
		guid->labpos  = -1;
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     s h o w _ g u i d a n c e                                        *
*     h i d e _ g u i d a n c e                                        *
*     s y n c _ g u i d a n c e                                        *
*     p r e s e n t _ g u i d a n c e                                  *
*                                                                      *
***********************************************************************/

LOGICAL	show_guidance(void)

	{
	if (!GuidShown)
		{
		/* Set display state on */
		define_dn_vis(DnGuid, TRUE);
		GuidShown = TRUE;
		}

	/* Set dispnode visibilities according to field state */
	/* to display pending selected fields */
	(void) sync_guidance();

	return present_all();
	}

/**********************************************************************/

LOGICAL	hide_guidance(void)

	{
	if (!GuidShown) return TRUE;

	/* Set display state off */
	define_dn_vis(DnGuid, FALSE);
	GuidShown = FALSE;
	return present_all();
	}

/**********************************************************************/

LOGICAL	sync_guidance(void)

	{
	int		iguid;
	GLIST	*guid;

	if (!GuidShown) return TRUE;

	for (iguid=0; iguid<MaxGuid; iguid++)
		{
		guid = GuidFlds + iguid;
		(void) sync_gfield(guid);
		}
	return TRUE;
	}

/**********************************************************************/

LOGICAL	present_guidance

	(
	LOGICAL	all
	)

	{
	int		iguid;
	GLIST	*guid;

	if (!GuidShown) return TRUE;

	/* Show the whole thing if requested */
	if (all) present_node(DnGuid);

	/* Display all selected fields */
	/* contouring if necessary */
	for (iguid=0; iguid<MaxGuid; iguid++)
		{
		guid = GuidFlds + iguid;
		(void) present_gfield(guid, all);
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     r e l e a s e _ g u i d a n c e                                  *
*                                                                      *
***********************************************************************/

LOGICAL	release_guidance(void)

	{
	int		iguid;
	GLIST	*guid;

	/* Don't do it if we're still watching */
	if (GuidShown) return FALSE;

	/* Remove all deselected fields that have been retained */
	for (iguid=0; iguid<MaxGuid; iguid++)
		{
		guid = GuidFlds + iguid;
		(void) release_gfield(guid, FALSE);
		}

	return TRUE;
	}




/***********************************************************************
*                                                                      *
*     Functions which control the selected guidance field:             *
*                                                                      *
***********************************************************************/
/***********************************************************************
*                                                                      *
*     g u i d a n c e _ f l d _ r e g i s t e r                        *
*     g u i d a n c e _ f l d _ d e r e g i s t e r                    *
*     g u i d a n c e _ f l d _ v i s i b i l i t y                    *
*                                                                      *
***********************************************************************/

LOGICAL	guidance_fld_register

	(
	STRING	tag,
	STRING	elem,
	STRING	level,
	STRING	source,
	STRING	subsrc,
	STRING	rtime
	)

	{
	GLIST	*guid;

	guid = register_gfield(tag, elem, level, source, subsrc, rtime);
	return (guid)? TRUE: FALSE;
	}

/**********************************************************************/

LOGICAL	guidance_fld_deregister

	(
	STRING	tag
	)

	{
	(void) deregister_gfield(tag);
	return TRUE;
	}

/**********************************************************************/

LOGICAL	guidance_fld_visibility

	(
	STRING	tag,
	STRING	vtime,
	STRING	state
	)

	{
	GLIST	*guid;
	GFRAME	*chart;
	LOGICAL	show;

	/* Find the field corresponding to tag */
	guid = find_gfield(tag, FALSE);
	if (!guid) return FALSE;

	/* Do we want to show or hide? */
	if (same_ic(state, "on"))       show = TRUE;
	else if (same_ic(state, "off")) show = FALSE;
	else                            return FALSE;

	/* Find the chart corresponding to vtime */
	/* Create if necessary (only if shown) */
	chart = find_gfield_chart(guid, vtime, show);
	if (!chart) return FALSE;

	/* Read the data if necessary (only if shown) */
	if (show) (void) read_gfield_chart(guid, chart);

	/* Set the visibility */
	if (show) (void) show_gfield_chart(guid, chart);
	else      (void) hide_gfield_chart(guid, chart);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     r e g i s t e r _ g f i e l d                                    *
*     d e r e g i s t e r _ g f i e l d                                *
*     f i n d _ g f i e l d                                            *
*     c h e c k _ g f i e l d                                          *
*                                                                      *
***********************************************************************/

GLIST	*register_gfield

	(
	STRING	tag,
	STRING	elem,
	STRING	level,
	STRING	source,
	STRING	subsrc,
	STRING	rtime
	)

	{
	GLIST	*guid;
	LOGICAL	minutes_rqd;
	FpaConfigSourceStruct	*sdef;

#	ifdef DEVELOPMENT
	int		igd;
	GLIST	*gd;
#	endif /* DEVELOPMENT */

	if (blank(tag)) return NULL;

#	ifdef DEVELOPMENT
	(void) printf("[register_gfield() Before ...\n");
	for (igd=0; igd<MaxGuid; igd++)
		{
		gd = GuidFlds + igd;
		if (blank(gd->tag))     continue;
		(void) printf("[register_gfield()  Tag: %d %s  Field: %s %s %s %s at: %s\n",
			igd, gd->tag, gd->elem, gd->level, gd->source, gd->subsrc, gd->rtime);
		}
#	endif /* DEVELOPMENT */

	/* Find the field corresponding to tag */
	/* or come up with an empty buffer */
	guid = find_gfield(tag, TRUE);
	if (!guid) return NULL;

	/* Get rid of whatever is currently using the same tag */
	(void) release_gfield(guid, TRUE);

	/* Build the run time stamp (with minutes correctly encoded) */
	sdef = identify_source(source, subsrc);
	minutes_rqd = (NotNull(sdef))? sdef->minutes_rqd: FALSE;
	if (blank(rtime) && (Syear != 0))
		{
		rtime = build_tstamp(Syear, Sjday, Shour, Sminute, FALSE, minutes_rqd);
		}
	else
		{
		if (minutes_rqd)
			rtime = tstamp_to_minutes(rtime, NullInt);
		else
			rtime = tstamp_to_hours(rtime, TRUE, NullInt);
		}

	/* Claim this buffer */
	guid->tag    = STRMEM(guid->tag,    tag);
	guid->elem   = STRMEM(guid->elem,   elem);
	guid->level  = STRMEM(guid->level,  level);
	guid->source = STRMEM(guid->source, source);
	guid->subsrc = STRMEM(guid->subsrc, subsrc);
	guid->rtime  = STRMEM(guid->rtime,  rtime);
	guid->erase  = FALSE;
	guid->colour = -1;
	guid->style  = -1;
	guid->width  = -1;
	guid->labpos = guid - GuidFlds;

#	ifdef DEVELOPMENT
	(void) printf("[register_gfield() After ...\n");
	for (igd=0; igd<MaxGuid; igd++)
		{
		gd = GuidFlds + igd;
		if (blank(gd->tag))     continue;
		(void) printf("[register_gfield()  Tag: %d %s  Field: %s %s %s %s at: %s\n",
			igd, gd->tag, gd->elem, gd->level, gd->source, gd->subsrc, gd->rtime);
		}
#	endif /* DEVELOPMENT */

	return guid;
	}

/**********************************************************************/

GLIST	*deregister_gfield

	(
	STRING	tag
	)

	{
	GLIST	*guid;

#	ifdef DEVELOPMENT
	int		igd;
	GLIST	*gd;
#	endif /* DEVELOPMENT */

	if (blank(tag)) return NULL;

#	ifdef DEVELOPMENT
	(void) printf("[deregister_gfield() Before ...\n");
	for (igd=0; igd<MaxGuid; igd++)
		{
		gd = GuidFlds + igd;
		if (blank(gd->tag))     continue;
		(void) printf("[deregister_gfield()  Tag: %d %s  Field: %s %s %s %s at: %s\n",
			igd, gd->tag, gd->elem, gd->level, gd->source, gd->subsrc, gd->rtime);
		}
#	endif /* DEVELOPMENT */

	/* Find the field correesponding to tag */
	guid = find_gfield(tag, FALSE);
	if (!guid) return NULL;

	/* There is one by that name - empty it */
	(void) release_gfield(guid, TRUE);

	/* Remove identification */
	FREEMEM(guid->tag);
	FREEMEM(guid->elem);
	FREEMEM(guid->level);
	FREEMEM(guid->source);
	FREEMEM(guid->subsrc);
	FREEMEM(guid->rtime);
	guid->erase  = FALSE;
	guid->colour = -1;
	guid->style  = -1;
	guid->width  = -1;
	guid->labpos = -1;

#	ifdef DEVELOPMENT
	(void) printf("[deregister_gfield() After ...\n");
	for (igd=0; igd<MaxGuid; igd++)
		{
		gd = GuidFlds + igd;
		if (blank(gd->tag))     continue;
		(void) printf("[deregister_gfield()  Tag: %d %s  Field: %s %s %s %s at: %s\n",
			igd, gd->tag, gd->elem, gd->level, gd->source, gd->subsrc, gd->rtime);
		}
#	endif /* DEVELOPMENT */

	return NULL;
	}

/**********************************************************************/

GLIST	*find_gfield

	(
	STRING	tag,
	LOGICAL	create
	)

	{
	int		iguid;
	GLIST	*guid;

	/* Find the buffer with the given handle */
	for (iguid=0; iguid<MaxGuid; iguid++)
		{
		guid = GuidFlds + iguid;
		if (blank(guid->tag))     continue;
		if (same(guid->tag, tag)) return guid;
		}

	/* Not found - create only if requested */
	if (!create) return NULL;

	/* Find an unused buffer */
	for (iguid=0; iguid<MaxGuid; iguid++)
		{
		guid = GuidFlds + iguid;
		if (blank(guid->tag)) return guid;
		}

	/* None found (Later>>> allocate more) */
	return NULL;
	}

/**********************************************************************/

LOGICAL	check_gfield

	(
	GLIST	*guid
	)

	{
	if (!guid)            return FALSE;
	if (blank(guid->tag)) return FALSE;

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     r e l e a s e _ g f i e l d                                      *
*                                                                      *
***********************************************************************/

LOGICAL	release_gfield

	(
	GLIST	*guid,
	LOGICAL	all
	)

	{
	GFRAME	*chart;
	int		ichart;

	if (!check_gfield(guid)) return FALSE;

	/* Release the whole sequence */
	for (ichart=0; ichart<guid->nchart; ichart++)
		{
		chart = guid->charts + ichart;
		(void) release_gfield_chart(guid, chart, all);
		if (all) FREEMEM(chart->jtime);
		}

	/* Delete the chart list if 'all' */
	if (all)
		{
		FREEMEM(guid->charts);
		guid->nchart = 0;
		}

	/* Done */
	guid->labpos = -1;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     s y n c _ g f i e l d                                            *
*     p r e s e n t _ g f i e l d                                      *
*                                                                      *
***********************************************************************/

LOGICAL	sync_gfield

	(
	GLIST	*guid
	)

	{
	GFRAME		*chart;
	int			ichart;
	MAP_PROJ	*mproj;
	METAFILE	meta;

	if (!check_gfield(guid)) return FALSE;

	/* Find out which chart we want to show */
	mproj = NullMapProj;
	meta  = NullMeta;
	for (ichart=0; ichart<guid->nchart; ichart++)
		{
		chart = guid->charts + ichart;
		if (chart->show)
			{
			meta = chart->meta;
			break;
			}
		}
	if (NotNull(meta)) mproj = &meta->mproj;

#	ifdef DEBUG_SYNC
	fprintf(stdout, "[sync_gfield] MapProj parameters:\n");
	fprintf(stdout, "  Projection: %s",
		which_projection_name(MapProj->projection.type));
	switch (MapProj->projection.type)
		{
		case ProjectNone:
		case ProjectLatLon:
		case ProjectPlateCaree:
		case ProjectRectangular:
		case ProjectMercatorEq:
			fprintf(stdout, "\n");
			break;

		case ProjectLatLonAng:
			fprintf(stdout, "  Plat: %.2f", MapProj->projection.ref[0]);
			fprintf(stdout, "  Plon: %.2f", MapProj->projection.ref[1]);
			fprintf(stdout, "  Origin: %.2f\n",
											MapProj->projection.ref[2]);
			break;

		case ProjectPolarSt:
			fprintf(stdout, "  Pole: %.2f", MapProj->projection.ref[0]);
			fprintf(stdout, "  TrueLat: %.2f\n",
											MapProj->projection.ref[1]);
			break;

		case ProjectObliqueSt:
			fprintf(stdout, "  Plat: %.2f", MapProj->projection.ref[0]);
			fprintf(stdout, "  Plon: %.2f", MapProj->projection.ref[1]);
			fprintf(stdout, "  Secant: %.2f\n",
											MapProj->projection.ref[2]);
			break;

		case ProjectLambertConf:
			fprintf(stdout, "  Lat1: %.2f", MapProj->projection.ref[0]);
			fprintf(stdout, "  Lat2: %.2f\n", MapProj->projection.ref[1]);
			break;

		default:
			fprintf(stdout, "\n");
			break;
		}

	fprintf(stdout, "  Basemap  olat: %f", MapProj->definition.olat);
	fprintf(stdout, "  olon: %f", MapProj->definition.olon);
	fprintf(stdout, "  lref: %f\n", MapProj->definition.lref);
	fprintf(stdout, "           xorg: %f", MapProj->definition.xorg);
	fprintf(stdout, "  yorg: %f\n", MapProj->definition.yorg);
	fprintf(stdout, "           xlen: %f", MapProj->definition.xlen);
	fprintf(stdout, "  ylen: %f", MapProj->definition.ylen);
	fprintf(stdout, "  units: %f\n", MapProj->definition.units);

	fprintf(stdout, "  Grid  nx: %d", MapProj->grid.nx);
	fprintf(stdout, "  ny: %d", MapProj->grid.ny);
	fprintf(stdout, "  units: %f\n", MapProj->grid.units);
	fprintf(stdout, "         gridlen: %f", MapProj->grid.gridlen);
	fprintf(stdout, "  xgrid: %f", MapProj->grid.xgrid);
	fprintf(stdout, "  ygrid: %f\n", MapProj->grid.ygrid);

	fprintf(stdout, "  Map origin  origin[X]: %f", MapProj->origin[X]);
	fprintf(stdout, "  origin[Y]: %f\n\n", MapProj->origin[Y]);

	fprintf(stdout, "[sync_gfield] mproj parameters:\n");
	if (NotNull(mproj))
		{
		fprintf(stdout, "  Projection: %s",
			which_projection_name(mproj->projection.type));
		switch (mproj->projection.type)
			{
			case ProjectNone:
			case ProjectLatLon:
			case ProjectPlateCaree:
			case ProjectRectangular:
			case ProjectMercatorEq:
				fprintf(stdout, "\n");
				break;

			case ProjectLatLonAng:
				fprintf(stdout, "  Plat: %.2f", mproj->projection.ref[0]);
				fprintf(stdout, "  Plon: %.2f", mproj->projection.ref[1]);
				fprintf(stdout, "  Origin: %.2f\n",
												mproj->projection.ref[2]);
				break;

			case ProjectPolarSt:
				fprintf(stdout, "  Pole: %.2f", mproj->projection.ref[0]);
				fprintf(stdout, "  TrueLat: %.2f\n",
												mproj->projection.ref[1]);
				break;

			case ProjectObliqueSt:
				fprintf(stdout, "  Plat: %.2f", mproj->projection.ref[0]);
				fprintf(stdout, "  Plon: %.2f", mproj->projection.ref[1]);
				fprintf(stdout, "  Secant: %.2f\n",
												mproj->projection.ref[2]);
				break;

			case ProjectLambertConf:
				fprintf(stdout, "  Lat1: %.2f", mproj->projection.ref[0]);
				fprintf(stdout, "  Lat2: %.2f\n", mproj->projection.ref[1]);
				break;

			default:
				fprintf(stdout, "\n");
				break;
			}

		fprintf(stdout, "  Basemap  olat: %f", mproj->definition.olat);
		fprintf(stdout, "  olon: %f", mproj->definition.olon);
		fprintf(stdout, "  lref: %f\n", mproj->definition.lref);
		fprintf(stdout, "           xorg: %f", mproj->definition.xorg);
		fprintf(stdout, "  yorg: %f\n", mproj->definition.yorg);
		fprintf(stdout, "           xlen: %f", mproj->definition.xlen);
		fprintf(stdout, "  ylen: %f", mproj->definition.ylen);
		fprintf(stdout, "  units: %f\n", mproj->definition.units);

		fprintf(stdout, "  Grid  nx: %d", mproj->grid.nx);
		fprintf(stdout, "  ny: %d", mproj->grid.ny);
		fprintf(stdout, "  units: %f\n", mproj->grid.units);
		fprintf(stdout, "         gridlen: %f", mproj->grid.gridlen);
		fprintf(stdout, "  xgrid: %f", mproj->grid.xgrid);
		fprintf(stdout, "  ygrid: %f\n", mproj->grid.ygrid);

		fprintf(stdout, "  Map origin  origin[X]: %f", mproj->origin[X]);
		fprintf(stdout, "  origin[Y]: %f\n\n", mproj->origin[Y]);
		}
#	endif /* DEBUG_SYNC */

	/* Make sure the displayed metafile agrees with the one we want to show */
	guid->dn->data.meta = NullMeta;
	define_dn_xform(guid->dn, "map", MapView, NullBox, MapProj, NullXform);
	define_dn_data(guid->dn, "metafile", (POINTER) meta);
	define_dn_vis(guid->dn, (LOGICAL) (!guid->erase));

	return TRUE;
	}

/**********************************************************************/

LOGICAL	present_gfield

	(
	GLIST	*guid,
	LOGICAL	all
	)

	{
	GFRAME	*chart;
	int		ichart;

	if (!check_gfield(guid)) return FALSE;

	/* Show the whole thing if requested */
	if (all) present_node(guid->dn);
	if (!guid->dn->data.meta) return TRUE;

	/* Find which chart corresponds to the one currently showing */
	/* (This may disagree with 'show' state until sync_gfield is called) */
	chart = NULL;
	for (ichart=0; ichart<guid->nchart; ichart++)
		{
		chart = guid->charts + ichart;
		if (chart->meta == guid->dn->data.meta) break;
		chart = NULL;
		}
	if (!chart) return TRUE;

	/* Contour surface field if first time displayed */
	if (!chart->contoured)
		{
		(void) contour_gfield_chart(guid, chart);
		chart->contoured = TRUE;
		}

	/* Label (or re-label) guidance chart and generate   */
	/*  automatic contour labels if first time displayed */
	(void) label_gfield_chart(guid, chart);

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     Functions which control the selected chart of the selected       *
*     guidance field:                                                  *
*                                                                      *
***********************************************************************/
/***********************************************************************
*                                                                      *
*     f i n d _ g f i e l d _ c h a r t                                *
*     c h e c k _ g f i e l d _ c h a r t                              *
*     r e a d _ g f i e l d _ c h a r t                                *
*     r e l e a s e _ g f i e l d _ c h a r t                          *
*                                                                      *
***********************************************************************/

GFRAME	*find_gfield_chart

	(
	GLIST	*guid,
	STRING	vtime,
	LOGICAL	create
	)

	{
	GFRAME	*chart;
	int		ichart;

	/* Make sure field and time are valid */
	if (!check_gfield(guid)) return NULL;
	if (blank(vtime))        return NULL;

	/* Search for requested valid time */
	for (ichart=0; ichart<guid->nchart; ichart++)
		{
		chart = guid->charts + ichart;
		if (matching_tstamps(vtime, chart->jtime)) return chart;
		}

	/* Create only if requested */
	if (!create) return NULL;

	/* Find first unused chart */
	chart = NULL;
	for (ichart=0; ichart<guid->nchart; ichart++)
		{
		chart = guid->charts + ichart;
		if (!chart->meta)
			{
			(void) release_gfield_chart(guid, chart, TRUE);
			FREEMEM(chart->jtime);
			break;
			}
		chart = NULL;
		}

	/* If no unused ones, allocate */
	if (!chart)
		{
		guid->nchart++;
		guid->charts = GETMEM(guid->charts, GFRAME, guid->nchart);
		chart = guid->charts + guid->nchart - 1;
		chart->jtime     = NULL;
		chart->meta      = NullMeta;
		chart->show      = FALSE;
		chart->contoured = FALSE;
		}

	/* Setup the chart to receive data */
	chart->jtime = strdup(vtime);
	return chart;
	}

/**********************************************************************/

LOGICAL	check_gfield_chart

	(
	GLIST	*guid,
	GFRAME	*chart
	)

	{
	int		ichart;

	if (!check_gfield(guid)) return FALSE;
	if (!chart)              return FALSE;
	if (!guid->charts)       return FALSE;

	ichart = chart - guid->charts;
	if (ichart < 0)             return FALSE;
	if (ichart >= guid->nchart) return FALSE;

	return TRUE;
	}

/**********************************************************************/

LOGICAL	read_gfield_chart

	(
	GLIST	*guid,
	GFRAME	*chart
	)

	{
	METAFILE		meta, gmeta;
	FIELD			gfld, lfld;
	SET				lset;
	STRING			source, subsrc, rtime, vtime, elem, level;
	COLOUR			colour;
	LSTYLE			style;
	float			width;
	STRING			dpath;
	FLD_DESCRIPT	fdesc;

	/* Make sure field and time are valid */
	if (!check_gfield_chart(guid, chart)) return FALSE;

	/* Don't read if already read (???) */
	if (chart->meta) return TRUE;

	/* Clean out whatever was already in this valid time */
	(void) release_gfield_chart(guid, chart, TRUE);

	/* Initialize a field descriptor for field */
	dpath  = get_directory("Data");
	vtime  = chart->jtime;
	elem   = guid->elem;
	level  = guid->level;
	source = guid->source;
	subsrc = guid->subsrc;
	rtime  = guid->rtime;
	(void) init_fld_descript(&fdesc);
	if (!set_fld_descript(&fdesc,
					FpaF_MAP_PROJECTION,	MapProj,
					FpaF_DIRECTORY_PATH,	dpath,
					FpaF_SOURCE_NAME,		source,
					FpaF_SUBSOURCE_NAME,	subsrc,
					FpaF_RUN_TIME,			rtime,
					FpaF_VALID_TIME,		vtime,
					FpaF_ELEMENT_NAME,		elem,
					FpaF_LEVEL_NAME,		level,
					FpaF_END_OF_LIST)) return FALSE;

	/* Clear internal buffers in "luke-warm" database */
	clear_equation_database();

	/* Create an empty metafile to put the product in for this prog time */
	/* If this is the active time, put the metafile into the product dispnode */
	meta = chart->meta = create_metafile();
	define_mf_projection(meta, MapProj);

	/* Read the metafile directly if possible */
	gmeta = meta_input(&fdesc);
	gfld  = NullFld;
	lfld  = NullFld;

	/* Obtain the field if possible, otherwise create an empty one */
	if (gmeta)
		{
		/* Try all the known field types */
		if (!gfld) gfld = take_mf_field(gmeta, "surface", NULL, NULL,
										elem, level);
		if (!gfld) gfld = take_mf_field(gmeta, "set", "area", NULL,
										elem, level);
		if (!gfld) gfld = take_mf_field(gmeta, "set", "curve", NULL,
										elem, level);
		if (!gfld) gfld = take_mf_field(gmeta, "plot", NULL, NULL,
										elem, level);
		}
	if (!gfld && check_retrieve_metasfc(&fdesc)) gfld = retrieve_field(&fdesc);
	if (!gfld) gfld = create_field("a", elem, level);
	add_field_to_metafile(meta, gfld);

	/* Obtain the label field if possible, otherwise create an empty one */
	if (gmeta) lfld = take_mf_field(gmeta, "set", "spot", NULL, elem, level);
	if (!lfld)
		{
		lfld = create_field("d", elem, level);
		lset = create_set("spot");
		define_fld_data(lfld, "set", (POINTER)lset);
		}
	add_field_to_metafile(meta, lfld);

	/* Clean up */
	if (gmeta) gmeta = destroy_metafile(gmeta);

	/* Load in the presentation info for the field */
	/* Invoke colour and style overrides if previously made */
	setup_fld_presentation(gfld, source);
	setup_fld_presentation(lfld, source);
	colour = guid->colour;
	style  = guid->style;
	width  = guid->width;
	if (colour >= 0)
		{
		change_fld_pspec(gfld, LINE_COLOUR, (POINTER)&colour);
		change_fld_pspec(gfld, MARK_COLOUR, (POINTER)&colour);
		change_fld_pspec(gfld, TEXT_COLOUR, (POINTER)&colour);
		change_fld_pspec(gfld, BARB_COLOUR, (POINTER)&colour);
		change_fld_pspec(lfld, TEXT_COLOUR, (POINTER)&colour);
		change_fld_pspec(lfld, MARK_COLOUR, (POINTER)&colour);
		change_fld_pspec(lfld, BARB_COLOUR, (POINTER)&colour);
		}
	if (style >= 0)
		{
		change_fld_pspec(gfld, LINE_STYLE, (POINTER)&style);
		}
	if (width >= 0)
		{
		change_fld_pspec(gfld, LINE_WIDTH, (POINTER)&width);
		change_fld_pspec(lfld, BARB_WIDTH, (POINTER)&width);
		}

	/* Done */
	return TRUE;
	}

/**********************************************************************/

LOGICAL	release_gfield_chart

	(
	GLIST	*guid,
	GFRAME	*chart,
	LOGICAL	all
	)

	{
	/* Make sure field and chart are valid */
	if (!check_gfield_chart(guid, chart)) return FALSE;

	/* Don't release visible chart unless 'all' */
	if (chart->show && !all) return TRUE;

	/* Remove from display node if viewing */
	if (guid->dn->data.meta == chart->meta)
		{
		guid->dn->data.meta = NullMeta;
		delete_dn_data(guid->dn);
		}

	/* Clean out whatever was already in this valid time */
	chart->meta      = destroy_metafile(chart->meta);
	chart->show      = FALSE;
	chart->contoured = FALSE;

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     s h o w _ g f i e l d _ c h a r t                                *
*     h i d e _ g f i e l d _ c h a r t                                *
*     c o n t o u r _ g f i e l d _ c h a r t                          *
*     l a b e l _ g f i e l d _ c h a r t                              *
*                                                                      *
***********************************************************************/

LOGICAL	show_gfield_chart

	(
	GLIST	*guid,
	GFRAME	*chart
	)

	{
	GFRAME	*ch;
	int		ichart;

	if (!check_gfield_chart(guid, chart)) return FALSE;

	/* For now, simultaneous charts cannot be shown */
	/* so turn off all others */
	for (ichart=0; ichart<guid->nchart; ichart++)
		{
		ch = guid->charts + ichart;
		if (ch) ch->show = FALSE;
		}

	chart->show = TRUE;
	return TRUE;
	}

/**********************************************************************/

LOGICAL	hide_gfield_chart

	(
	GLIST	*guid,
	GFRAME	*chart
	)

	{
	if (!check_gfield_chart(guid, chart)) return FALSE;

	chart->show = FALSE;
	return TRUE;
	}

/**********************************************************************/

LOGICAL	contour_gfield_chart

	(
	GLIST	*guid,
	GFRAME	*chart
	)

	{
	FIELD	gfld;
	SURFACE	sfc;
	COLOUR	colour;
	LSTYLE	style;
	float	width;

	if (!check_gfield_chart(guid, chart)) return FALSE;

	gfld = gfield_chart_member(guid, chart, "field");
	if (!gfld) return FALSE;

	/* Contour and display each surface */
	if (gfld->ftype == FtypeSfc)
		{
		/* Contour surface */
		sfc = gfld->data.sfc;
		put_message("fld-contour-cont", gfld->level, gfld->element);
		contour_surface(sfc);

		/* Set up appearance */
		colour = guid->colour;
		style  = guid->style;
		width  = guid->width;
		if (colour >= 0)
			{
			change_fld_pspec(gfld, LINE_COLOUR, (POINTER)&colour);
			change_fld_pspec(gfld, MARK_COLOUR, (POINTER)&colour);
			change_fld_pspec(gfld, TEXT_COLOUR, (POINTER)&colour);
			change_fld_pspec(gfld, BARB_COLOUR, (POINTER)&colour);
			}
		if (style >= 0)
			{
			change_fld_pspec(gfld, LINE_STYLE, (POINTER)&style);
			}
		if (width >= 0)
			{
			change_fld_pspec(gfld, LINE_WIDTH, (POINTER)&width);
			}

		/* Display field */
		put_message("fld-contour-disp", gfld->level, gfld->element);
		if (!guid->erase)
			{
			gxSetupTransform(guid->dn);
			glClipMode(glCLIP_ON);
			display_field(gfld);
			glClipMode(glCLIP_OFF);
			display_dn_edge(guid->dn);
			}
		sync_display();
		}

	put_message("fld-contour-done");
	return TRUE;
	}

/**********************************************************************/

LOGICAL	label_gfield_chart

	(
	GLIST	*guid,
	GFRAME	*chart
	)

	{
	FIELD	gfld, lfld;
	COLOUR	colour, lcolour;
	int		nlab;
	LOGICAL	status;
	STRING	tag, elem, level, source, subsrc, rtime, vtime;
	SURFACE	sfc;
	SET		lset;
	float	xmax, ymax, xval, xinc, yinc, lfact;

	if (!check_gfield_chart(guid, chart)) return FALSE;

	put_message("guid-label");

	rtime = guid->rtime;
	vtime = chart->jtime;

	/* Make sure there is a label field */
	lfld = gfield_chart_member(guid, chart, "labs");
	if (!lfld)                     return FALSE;
	if (lfld->ftype != FtypeSet)   return FALSE;
	lset = lfld->data.set;
	if (!lset)                     return FALSE;
	if (!same(lset->type, "spot")) return FALSE;

	/* Info about field */
	lfact   = 0.5*(guid->labpos + 1);
	tag     = guid->tag;
	elem    = guid->elem;
	level   = guid->level;
	source  = guid->source;
	subsrc  = guid->subsrc;
	colour  = guid->colour;
	lcolour = colour;

	/* Recall the guidance field */
	gfld = gfield_chart_member(guid, chart, "field");
	if (!gfld) return FALSE;

	/* Set display colour for legend label */
	if (lcolour < 0) recall_fld_pspec(gfld, LINE_COLOUR, (POINTER)&lcolour);
	if (lcolour < 0) recall_fld_pspec(gfld, TEXT_COLOUR, (POINTER)&lcolour);
	if (lcolour < 0) lcolour = find_colour("black", &status);

	/* Transfer guidance legend to panel for display */
	guidance_legend_labels(tag, lcolour);

	/* Label the guidance field contours */
	nlab = set_count(lset, "");
	if (nlab <= 0)
		{

		/* Set the active field */
		active_field_info(elem, level, source, subsrc, rtime, vtime);

		/* Generate surface labels (along left and right edge of map) */
		if (gfld->ftype == FtypeSfc && NotNull(gfld->data.sfc))
			{
			sfc  = gfld->data.sfc;

			xmax = MapProj->definition.xlen;
			ymax = MapProj->definition.ylen;
			xinc = xmax/20;
			yinc = ymax/20;

			xval = xinc*lfact;
			(void) generate_surface_labs(sfc, lset, LabelSize,
										   xval, yinc, ymax);
			xval = xmax - xval;
			(void) generate_surface_labs(sfc, lset, LabelSize,
										   xval, yinc, ymax);
			}
		}

	/* Set presentation according to latest selection */
	invoke_set_catspecs(lset);
	if (colour >= 0) change_set_pspec(lset, TEXT_COLOUR, (POINTER)&colour);
	if (colour >= 0) change_set_pspec(lset, MARK_COLOUR, (POINTER)&colour);
	if (colour >= 0) change_set_pspec(lset, BARB_COLOUR, (POINTER)&colour);

	/* Display labels if visible */
	if (!guid->erase)
		{
		gxSetupTransform(guid->dn);
		glClipMode(glCLIP_ON);
		display_set(lset);
		glClipMode(glCLIP_OFF);
		}
	sync_display();

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     a c t i v e _ g f i e l d _ c h a r t                            *
*     s a m p l e _ g f i e l d _ c h a r t                            *
*     g f i e l d _ c h a r t _ m e m b e r                            *
*                                                                      *
***********************************************************************/

LOGICAL	active_gfield_chart

	(
	STRING	tag
	)

	{
	GLIST	*guid;
	GFRAME	*chart;
	int		ichart;
	FIELD	gfld, lfld;

	/* Deselect previous field */
	GuidFld    = NULL;
	GuidChart  = NULL;
	GuidElem   = NULL;
	GuidLevel  = NULL;
	GuidSource = NULL;
	GuidSubSrc = NULL;
	GuidRtime  = NULL;
	GuidSfc    = NullSfc;
	GuidSet    = NullSet;
	GuidPlot   = NullPlot;
	GuidLabs   = NullSet;

	/* Make sure field is valid */
	if (blank(tag))          return FALSE;
	guid = find_gfield(tag, FALSE);
	if (!check_gfield(guid)) return FALSE;

	/* Find out which chart is active */
	chart = NULL;
	for (ichart=0; ichart<guid->nchart; ichart++)
		{
		chart = guid->charts + ichart;
		if (chart->meta == guid->dn->data.meta) break;
		chart = NULL;
		}
	if (!chart) return FALSE;

	GuidFld    = guid;
	GuidChart  = chart;
	GuidElem   = guid->elem;
	GuidLevel  = guid->level;
	GuidSource = guid->source;
	GuidSubSrc = guid->subsrc;
	GuidRtime  = guid->rtime;
	gfld       = gfield_chart_member(guid, chart, "field");
	lfld       = gfield_chart_member(guid, chart, "labs");

	/* Extract field members */
	if (gfld)
		{
		switch (gfld->ftype)
			{
			case FtypeSfc:	GuidSfc  = gfld->data.sfc;	break;
			case FtypeSet:	GuidSet  = gfld->data.set;	break;
			case FtypePlot:	GuidPlot = gfld->data.plot;	break;
			}
		}
	if (lfld)
		{
		switch (lfld->ftype)
			{
			case FtypeSet:	GuidLabs = lfld->data.set;	break;
			}
		}

	/* Set up the editors */
	active_field_info(GuidElem, GuidLevel, GuidSource, GuidSubSrc, GuidRtime,
					GuidChart->jtime);
	active_spline_fields(FALSE, GuidSfc, GuidLabs);
	active_area_fields(FALSE, GuidSet, GuidLabs);
	active_line_fields(FALSE, GuidSet, GuidLabs);
	active_point_fields(FALSE, GuidSet);
	active_lchain_fields(FALSE, GuidSet);
	label_appearance(GuidFld->colour, GuidFld->style, GuidFld->width, FALSE);
	sample_appearance(SkipFont, SkipTsize, SkipColour);

	return TRUE;
	}

/**********************************************************************/

LOGICAL	sample_gfield_chart

	(
	STRING	tag,
	STRING	vtime
	)

	{
	GLIST	*guid;
	GFRAME	*chart;
	FIELD	gfld, lfld;

	/* Deselect previous field */
	GuidFld    = NULL;
	GuidChart  = NULL;
	GuidElem   = NULL;
	GuidLevel  = NULL;
	GuidSource = NULL;
	GuidSubSrc = NULL;
	GuidRtime  = NULL;
	GuidSfc    = NullSfc;
	GuidSet    = NullSet;
	GuidPlot   = NullPlot;
	GuidLabs   = NullSet;

	/* Make sure field is valid */
	if (blank(tag))          return FALSE;
	guid = find_gfield(tag, FALSE);
	if (!check_gfield(guid)) return FALSE;

	chart = find_gfield_chart(guid, vtime, TRUE);
	if (!chart) return FALSE;

	/* Read the data if necessary */
	(void) read_gfield_chart(guid, chart);

	GuidFld    = guid;
	GuidChart  = chart;
	GuidElem   = guid->elem;
	GuidLevel  = guid->level;
	GuidSource = guid->source;
	GuidSubSrc = guid->subsrc;
	GuidRtime  = guid->rtime;
	gfld = gfield_chart_member(guid, chart, "field");
	lfld = gfield_chart_member(guid, chart, "labs");

	/* Extract field members */
	if (gfld)
		{
		switch (gfld->ftype)
			{
			case FtypeSfc:	GuidSfc  = gfld->data.sfc;	break;
			case FtypeSet:	GuidSet  = gfld->data.set;	break;
			case FtypePlot:	GuidPlot = gfld->data.plot;	break;
			}
		}
	if (lfld)
		{
		switch (lfld->ftype)
			{
			case FtypeSet:	GuidLabs = lfld->data.set;	break;
			}
		}

	/* Set up the editors */
	active_field_info(GuidElem, GuidLevel, GuidSource, GuidSubSrc, GuidRtime,
					GuidChart->jtime);
	active_spline_fields(FALSE, GuidSfc, GuidLabs);
	active_area_fields(FALSE, GuidSet, GuidLabs);
	active_line_fields(FALSE, GuidSet, GuidLabs);
	active_point_fields(FALSE, GuidSet);
	active_lchain_fields(FALSE, GuidSet);
	label_appearance(GuidFld->colour, GuidFld->style, GuidFld->width, FALSE);
	sample_appearance(SkipFont, SkipTsize, SkipColour);

	return TRUE;
	}

/**********************************************************************/

FIELD	gfield_chart_member

	(
	GLIST	*guid,
	GFRAME	*chart,
	STRING	type
	)

	{
	METAFILE	meta;

	if (!check_gfield_chart(guid, chart)) return NullFld;

	if (!chart->meta) return NullFld;
	meta = chart->meta;

	if (same(type, "field")) return meta->fields[0];
	if (same(type, "labs"))  return meta->fields[1];
	return NullFld;
	}




/***********************************************************************
*                                                                      *
*     Functions which control editing of the selected guidance field:  *
*                                                                      *
***********************************************************************/
/***********************************************************************
*                                                                      *
*     g u i d a n c e _ c h e c k                                      *
*     g u i d a n c e _ e d i t                                        *
*     g u i d a n c e _ l a b e l                                      *
*     g u i d a n c e _ s a m p l e                                    *
*                                                                      *
***********************************************************************/

LOGICAL	guidance_check(void)

	{
	if (!GuidShown) return FALSE;
	set_Xcurve_modes("draw");
	return TRUE;
	}

/**********************************************************************/

LOGICAL	guidance_edit

	(
	STRING	mode
	)

	{
	if (!GuidShown) return FALSE;

	/* Handle "get_contour" edit */
	if (same(EditMode, "GET_CONTOUR"))
	    {
		if (!active_gfield_chart(EditVal[0]))
			{
			put_message("guid-fld-invalid", EditVal[0]);
			return FALSE;
			}

	    if (GuidSfc)
			{
			(void) edit_grabcont_spline(mode);
			}
		}

	else if (same(EditMode, "FIELD"))
		{
		if (!active_gfield_chart(EditVal[0]))
			{
			put_message("guid-fld-invalid", EditVal[0]);
			return FALSE;
			}

		if (blank(EditVal[1]))
			{
			/* Ignore extraneous appearance commands */
			}
		else if (same(EditVal[1], "RESTORE"))
			{
			if (same(mode, "begin") || same(mode, "resume"))
				(void) guid_fld_restore();
			}
		else if (same(EditVal[1], "ERASE"))
			{
			if (same(mode, "begin") || same(mode, "resume"))
				(void) guid_fld_erase();
			}
		else if (same(EditVal[1], "COLOR"))
			{
			if (same(mode, "begin") || same(mode, "resume"))
				(void) guid_fld_appearance(EditVal[1], EditVal[2]);
			}
		else if (same(EditVal[1], "STYLE"))
			{
			if (blank(EditVal[2])) (void) strcpy(EditVal[2], "dotted");
			if (same(mode, "begin") || same(mode, "resume"))
				(void) guid_fld_appearance(EditVal[1], EditVal[2]);
			}
		else if (same(EditVal[1], "THICKNESS"))
			{
			if (blank(EditVal[2])) (void) strcpy(EditVal[2], "medium");
			if (same(mode, "begin") || same(mode, "resume"))
				(void) guid_fld_appearance(EditVal[1], EditVal[2]);
			}
		else
			{
			put_message("guid-unsupported",
						EditMode, EditVal[1], "");
			return FALSE;
			}
		(void) strcpy(EditVal[1], "");
		(void) strcpy(EditVal[2], "");
		put_message("guid-appearance");
		}

	/* Unknown command */
	else
	    {
		put_message("guid-unsupported", EditMode, "", "");
		return FALSE;
	    }

	return TRUE;
	}

/**********************************************************************/

LOGICAL	guidance_label

	(
	STRING	mode
	)

	{
	LOGICAL	valid;

	if (!GuidShown) return FALSE;

	/* Handle "add" label */
	if (same(EditMode, "ADD"))
	    {
		valid = eval_list_reset();
		if (!valid) return FALSE;
		}

    /* Handle "list" label */
    else if (same(EditMode, "LIST"))
        {
        if (same(EditVal[0], "GO"))
            {
            move_edit_vals(0, 1);
            }
        else
            {
            valid = eval_list_add(EditVal[0], EditVal[1]);
            move_edit_vals(0, 2);
            if (!valid) return FALSE;
            return FALSE;
            }
        }

    /* Handle "move" label */
    else if (same(EditMode, "MOVE"))
        {
        (void) label_move(mode);
        return TRUE;
        }

    /* Handle "modify" label */
    else if (same(EditMode, "MODIFY"))
        {
        (void) label_modify(mode, EditVal[0], EditCal);
        return TRUE;
        }

    /* Handle "show" label */
    else if (same(EditMode, "SHOW"))
        {
        (void) label_show(mode);
        return TRUE;
        }

    /* Handle "delete" label */
    else if (same(EditMode, "DELETE"))
        {
        (void) label_delete(mode);
        return TRUE;
        }

	/* Unknown command */
	else
	    {
		put_message("guid-unsupported", EditMode, "", "");
		return FALSE;
	    }


    /* Came here from "ADD", or "LIST GO" */
    /* Resume in "ADD" mode */
    (void) strcpy(EditMode, "ADD");

	label_add(mode, EditVal[0], EditCal);

	return TRUE;
	}

/**********************************************************************/

LOGICAL	guidance_sample

	(
	STRING	mode
	)

	{
	LOGICAL	valid;

	static	LOGICAL	first = TRUE;
	static	FONT	fnt;
	static	float	sz;
	static	COLOUR	clr;

	/* Set default display parameters */
	if (first)
		{
		fnt = find_font("bold", &valid);
		sz  = find_size("4%", &valid);
		clr = find_colour("yellow", &valid);
		first = FALSE;
		}

	if (!GuidShown) return FALSE;

	/* Handle "cancel" sample */
	if (same(EditMode, "CANCEL"))
		{
		mode = "cancel";
		(void) sample_by_type(mode, EditVal[2], EditVal[3],
					GuidSource, GuidSubSrc, GuidRtime, GuidElem, GuidLevel);

		/* Resume in normal mode */
		(void) strcpy(EditMode, "NORMAL");
		return FALSE;
		}

	/* Handle "normal" sample */
	else if (same(EditMode, "NORMAL"))
	    {
		valid = eval_list_reset();
		if (!valid) return FALSE;
		}

	/* Handle "grid" sample */
	else if (same(EditMode, "GRID"))
	    {
		valid = eval_list_grid(EditVal[0], EditVal[1]);
		move_edit_vals(0, 2);
		if (!valid) return FALSE;
		}

	/* Handle "list" sample */
	else if (same(EditMode, "LIST"))
	    {
		if (same(EditVal[0], "GO"))
			{
			move_edit_vals(0, 1);
			}
		else
			{
			valid = eval_list_add(EditVal[0], EditVal[1]);
			move_edit_vals(0, 2);
			if (!valid) return FALSE;
			return FALSE;
			}
		}

	/* Unknown command */
	else
	    {
		put_message("guid-unsupported", EditMode, "", "");
		return FALSE;
	    }

	/* Came here from "NORMAL", "GRID" or "LIST GO" */

	/* Obtain the field to be sampled */
	if (!sample_gfield_chart(EditVal[0], EditVal[1]))
		{
		put_message("guid-fld-invalid", EditVal[0]);
		return FALSE;
		}

	/* Determine display attributes */
	if (!blank(EditVal[4])) fnt = find_font(EditVal[4], &valid);
	if (!blank(EditVal[5])) sz  = find_size(EditVal[5], &valid);
	if (!blank(EditVal[6])) clr = find_colour(EditVal[6], &valid);
	sample_appearance(fnt, sz, clr);

	/* Resume in normal mode */
	(void) strcpy(EditMode, "NORMAL");

	(void) sample_by_type(mode, EditVal[2], EditVal[3],
				GuidSource, GuidSubSrc, GuidRtime, GuidElem, GuidLevel);

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     g u i d _ f l d _ a p p e a r a n c e                            *
*     g u i d _ f l d _ e r a s e                                      *
*     g u i d _ f l d _ r e s t o r e                                  *
*                                                                      *
***********************************************************************/

LOGICAL	guid_fld_appearance

	(
	STRING	mode,
	STRING	value
	)

	{
	FIELD		gfld, lfld;
	LOGICAL		set_colour, set_style, status;
	COLOUR		colour;
	LSTYLE		style;
	float		width;
	int			ichart;
	GFRAME		*chart;
	char		wbuf[40], sbuf[40];
	STRING		cp;
	int			cd;
	STRING		nopattern=NULL;

	/* See what parameter to change */
	set_colour = same(mode, "COLOR");
	set_style  = same(mode, "STYLE");
	if (!set_colour && !set_style) return FALSE;

	/* Make sure product is meaningful */
	if (!GuidFld) return FALSE;

	GuidFld->erase = FALSE;
	(void) sync_gfield(GuidFld);

	/* Save changed parameter */
	if (set_colour)
		{
		colour = find_colour(value, &status);
		if (!status) return FALSE;
		GuidFld->colour = colour;
		}
	if (set_style)
		{
		if ((cp=strchr(value, '.')) != NULL)
			{
			cd = cp - value;
			(void) strcpy(sbuf, value+cd+1);
			(void) strcpy(wbuf, value);
			(void) strcpy(wbuf+cd, "");
			style = find_lstyle(sbuf, &status);
			if (!status) return FALSE;
			width = find_lwidth(wbuf, &status);
			if (!status) return FALSE;
			}
		else
			{
			style = find_lstyle(value, &status);
			if (!status) return FALSE;
			width = 0;
			}
		GuidFld->style = style;
		GuidFld->width = width;
		}

	/* Change appearance in whole sequence */
	for (ichart=0; ichart<GuidFld->nchart; ichart++)
		{
		chart = GuidFld->charts + ichart;

		gfld = gfield_chart_member(GuidFld, chart, "field");
		lfld = gfield_chart_member(GuidFld, chart, "labs");

		/* Change colour or style and width */
		if (set_colour)
			{
			change_fld_pspec(gfld, LINE_PATTERN, (POINTER)&nopattern);
			change_fld_pspec(gfld, LINE_COLOUR,  (POINTER)&colour);
			change_fld_pspec(gfld, MARK_COLOUR,  (POINTER)&colour);
			change_fld_pspec(gfld, TEXT_COLOUR,  (POINTER)&colour);
			change_fld_pspec(gfld, BARB_COLOUR,  (POINTER)&colour);
			change_fld_pspec(lfld, TEXT_COLOUR,  (POINTER)&colour);
			change_fld_pspec(lfld, MARK_COLOUR,  (POINTER)&colour);
			change_fld_pspec(lfld, BARB_COLOUR,  (POINTER)&colour);
			}
		if (set_style)
			{
			change_fld_pspec(gfld, LINE_PATTERN, (POINTER)&nopattern);
			change_fld_pspec(gfld, LINE_STYLE,   (POINTER)&style);
			change_fld_pspec(gfld, LINE_WIDTH,   (POINTER)&width);
			change_fld_pspec(lfld, BARB_WIDTH,   (POINTER)&width);
			}
		}

	/* Now re-display */
	present_all();
	return TRUE;
	}

/**********************************************************************/

LOGICAL	guid_fld_erase(void)

	{
	/* Make sure product is meaningful */
	if (!GuidFld) return FALSE;

	GuidFld->erase = TRUE;
	(void) sync_gfield(GuidFld);

	/* Now re-display */
	present_all();
	return TRUE;
	}

/**********************************************************************/

LOGICAL	guid_fld_restore(void)

	{
	FIELD		gfld, lfld;
	SURFACE		sfc;
	SET			set, lset;
	PLOT		plot;
	int			ichart;
	GFRAME		*chart;

	/* Make sure product is meaningful */
	if (!GuidFld) return FALSE;

	GuidFld->erase = FALSE;
	(void) sync_gfield(GuidFld);

	/* Save changed parameters */
	GuidFld->colour = -1;
	GuidFld->style  = -1;
	GuidFld->width  = -1;

	/* Restore presentation specs in whole sequence */
	for (ichart=0; ichart<GuidFld->nchart; ichart++)
		{
		chart = GuidFld->charts + ichart;

		active_field_info(GuidElem, GuidLevel, GuidSource, GuidSubSrc,
						GuidRtime, chart->jtime);

		gfld = gfield_chart_member(GuidFld, chart, "field");
		lfld = gfield_chart_member(GuidFld, chart, "labs");

		/* Restore presentation specs */
		setup_fld_presentation(gfld, GuidFld->source);
		setup_fld_presentation(lfld, GuidFld->source);

		lset = (lfld->ftype==FtypeSet)? lfld->data.set: NullSet;

		switch (gfld->ftype)
			{
			case FtypeSfc:
				sfc = gfld->data.sfc;
				if (NotNull(sfc))
					{
					/* Change legend to colour in the first contour spec */
					/* Change labels to colour of the corresponding contour */
					invoke_surface_conspecs(sfc);
					(void) recompute_surface_labs(sfc, lset, TRUE);
					}
				break;

			case FtypeSet:
				set = gfld->data.set;
				if (NotNull(set))
					{
					invoke_set_catspecs(set);
					if (same(set->type, "area"))
						{
						/* Change legend to colour in the first category spec */
						/* Change labels to colour of the corresponding area */
						(void) recompute_areaset_labs(set, lset, TRUE);
						}

					else if (same(set->type, "curve"))
						{
						/* Change legend to colour in the first category spec */
						/* Change labels to colour of the corresponding area */
						(void) recompute_curveset_labs(set, lset, TRUE);
						}

					else if (same(set->type, "spot"))
						{
						/* Change legend to colour in the first category spec */
						/* Change labels to colour of the corresponding area */
						(void) recompute_spotset_labs(set, lset, TRUE);
						}
					}
				break;

			case FtypePlot:
				plot = gfld->data.plot;
				if (NotNull(plot))
					{
					/* Change legend to colour in the first category spec */
					/* Change labels to colour of the corresponding area */
					/* Change wind barbs to colour in the first category spec */
					invoke_plot_pltspecs(plot);
					invoke_set_catspecs(lset);
					/* (void) recompute_plot_labs(plot, lset, TRUE); */
					}
				break;
			}
		}

	/* Now re-display */
	present_all();
	return TRUE;
	}
